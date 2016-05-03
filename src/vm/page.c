#include <string.h>
#include <stdbool.h>
#include "filesys/file.h"
#include "threads/interrupt.h"
#include "threads/malloc.h"
#include "threads/palloc.h"
#include "threads/thread.h"
#include "threads/vaddr.h"
#include "userprog/pagedir.h"
#include "userprog/process.h"
#include "userprog/syscall.h"
#include "vm/frame.h"
#include "vm/page.h"
#include "vm/swap.h"

static unsigned page_hash_func (const struct hash_elem *e, void *aux UNUSED)
{
  struct spage *spte = hash_entry(e, struct spage,
					   elem);
  return hash_int((int) spte->data_to_fetch);
}

static bool page_less_func (const struct hash_elem *a,
			    const struct hash_elem *b,
			    void *aux UNUSED)
{
  struct spage *sa = hash_entry(a, struct spage, elem);
  struct spage *sb = hash_entry(b, struct spage, elem);
  if (sa->data_to_fetch < sb->data_to_fetch)
    {
      return true;
    }
  return false;
}

static void page_action_func (struct hash_elem *e, void *aux UNUSED)
{
  struct spage *spte = hash_entry(e, struct spage,
					   elem);
  if (spte->is_loaded)
    {
      free_frame(pagedir_get_page(thread_current()->pagedir, spte->data_to_fetch));
      pagedir_clear_page(thread_current()->pagedir, spte->data_to_fetch);
    }
  free(spte);
}

void page_table_init (struct hash *spt)
{
  hash_init (spt, page_hash_func, page_less_func, NULL);
}

void page_table_destroy (struct hash *spt)
{
  hash_destroy (spt, page_action_func);
}

struct spage* get_sp (void *data_to_fetch)
{
  struct spage spte;
  spte.data_to_fetch = pg_round_down(data_to_fetch);

  struct hash_elem *e = hash_find(&thread_current()->spt, &spte.elem);
  if (!e)
    {
      return NULL;
    }
  return hash_entry (e, struct spage, elem);
}

bool page_load (struct spage *spte)
{
  bool success = false;
  spte->pinned = true;
  if (spte->is_loaded)
    {
      return success;
    }
  switch (spte->type)
    {
    case FILE:
      success = file_load(spte);
      break;
    case SWAP:
      success = swap_load(spte);
      break;
    case MMAP:
      success = file_load(spte);
      break;
    }
  return success;
}

bool swap_load (struct spage *spte)
{
  uint8_t *frame = allocate_frame (PAL_USER, spte);
  if (!frame)
    {
      return false;
    }
  if (!install_page(spte->data_to_fetch, frame, spte->can_write))
    {
      free_frame(frame);
      return false;
    }
  swap_in(spte->swap_index, spte->data_to_fetch);
  spte->is_loaded = true;
  return true;
}

bool file_load (struct spage *spte)
{
  enum palloc_flags flags = PAL_USER;
  if (spte->read_bytes == 0)
    {
      flags |= PAL_ZERO;
    }
  uint8_t *frame = allocate_frame(flags, spte);
  if (!frame)
    {
      return false;
    }
  if (spte->read_bytes > 0)
    {
      lock_acquire(&filesys_lock);
      if ((int) spte->read_bytes != file_read_at(spte->file, frame,
						 spte->read_bytes,
						 spte->offset))
	{
	  lock_release(&filesys_lock);
	  free_frame(frame);
	  return false;
	}
      lock_release(&filesys_lock);
      memset(frame + spte->read_bytes, 0, spte->zero_bytes);
    }

  if (!install_page(spte->data_to_fetch, frame, spte->can_write))
    {
      free_frame(frame);
      return false;
    }

  spte->is_loaded = true;  
  return true;
}

bool add_file_to_ptable (struct file *file, int32_t ofs, uint8_t *upage,
			     uint32_t read_bytes, uint32_t zero_bytes,
			     bool can_write)
{
  struct spage *spte = malloc(sizeof(struct spage));
  if (!spte)
    {
      return false;
    }
  spte->file = file;
  spte->offset = ofs;
  spte->data_to_fetch = upage;
  spte->read_bytes = read_bytes;
  spte->zero_bytes = zero_bytes;
  spte->can_write = can_write;
  spte->is_loaded = false;
  spte->type = FILE;
  spte->pinned = false;

  return (hash_insert(&thread_current()->spt, &spte->elem) == NULL);
}

bool add_mmap_to_ptable(struct file *file, int32_t ofs, uint8_t *upage,
			     uint32_t read_bytes, uint32_t zero_bytes)
{
  struct spage *spte = malloc(sizeof(struct spage));
  if (!spte)
    {
      return false;
    }
  spte->file = file;
  spte->offset = ofs;
  spte->data_to_fetch = upage;
  spte->read_bytes = read_bytes;
  spte->zero_bytes = zero_bytes;
  spte->is_loaded = false;
  spte->type = MMAP;
  spte->can_write = true;
  spte->pinned = false;

  if (!process_add_mmap(spte))
    {
      free(spte);
      return false;
    }

  if (hash_insert(&thread_current()->spt, &spte->elem))
    {
      spte->type = HASH_ERROR;
      return false;
    }
  return true;
}

bool stack_grow (void *data_to_fetch)
{
  if ( (size_t) (PHYS_BASE - pg_round_down(data_to_fetch)) > MAX_STACK_SIZE)
    {
      return false;
    }
 struct spage *spte = malloc(sizeof(struct spage));
  if (!spte)
    {
      return false;
    }
  spte->data_to_fetch = pg_round_down(data_to_fetch);
  spte->is_loaded = true;
  spte->can_write = true;
  spte->type = SWAP;
  spte->pinned = true;

  uint8_t *frame = allocate_frame (PAL_USER, spte);
  if (!frame)
    {
      free(spte);
      return false;
    }

  if (!install_page(spte->data_to_fetch, frame, spte->can_write))
    {
      free(spte);
      free_frame(frame);
      return false;
    }

  if (intr_context())
    {
      spte->pinned = false;
    }

  return (hash_insert(&thread_current()->spt, &spte->elem) == NULL);
}
