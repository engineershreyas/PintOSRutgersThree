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
  struct spage *sp = hash_entry(e, struct spage,
					   elem);
  return hash_int((int) sp->data_to_fetch);
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
  struct spage *sp = hash_entry(e, struct spage,
					   elem);
  if (sp->valid_access)
    {
      free_frame(pagedir_get_page(thread_current()->pagedir, sp->data_to_fetch));
      pagedir_clear_page(thread_current()->pagedir, sp->data_to_fetch);
    }
  free(sp);
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
  struct spage sp;
  sp.data_to_fetch = pg_round_down(data_to_fetch);

  struct hash_elem *e = hash_find(&thread_current()->spt, &sp.elem);
  if (!e)
    {
      return NULL;
    }
  return hash_entry (e, struct spage, elem);
}

bool page_load (struct spage *sp)
{
  bool success = false;
  sp->sticky = true;
  if (sp->valid_access)
    {
      return success;
    }
  switch (sp->type)
    {
    case FILE:
      success = file_load(sp);
      break;
    case SWAP:
      success = swap_load(sp);
      break;
    case MMAP:
      success = file_load(sp);
      break;
    }
  return success;
}

bool swap_load (struct spage *sp)
{
  uint8_t *frame = allocate_frame (PAL_USER, sp);
  if (!frame)
    {
      return false;
    }
  if (!install_page(sp->data_to_fetch, frame, sp->can_write))
    {
      free_frame(frame);
      return false;
    }
  swap_in(sp->swap_index, sp->data_to_fetch);
  sp->valid_access = true;
  return true;
}

bool file_load (struct spage *sp)
{
  enum palloc_flags flags = PAL_USER;
  if (sp->read_bytes == 0)
    {
      flags |= PAL_ZERO;
    }
  uint8_t *frame = allocate_frame(flags, sp);
  if (!frame)
    {
      return false;
    }
  if (sp->read_bytes > 0)
    {
      lock_acquire(&filesys_lock);
      if ((int) sp->read_bytes != file_read_at(sp->file, frame,
						 sp->read_bytes,
						 sp->offset))
	{
	  lock_release(&filesys_lock);
	  free_frame(frame);
	  return false;
	}
      lock_release(&filesys_lock);
      memset(frame + sp->read_bytes, 0, sp->zero_bytes);
    }

  if (!install_page(sp->data_to_fetch, frame, sp->can_write))
    {
      free_frame(frame);
      return false;
    }

  sp->valid_access = true;  
  return true;
}

bool add_file_to_ptable (struct file *file, int32_t ofs, uint8_t *upage,
			     uint32_t read_bytes, uint32_t zero_bytes,
			     bool can_write)
{
  struct spage *sp = malloc(sizeof(struct spage));
  if (!sp)
    {
      return false;
    }
  sp->file = file;
  sp->offset = ofs;
  sp->data_to_fetch = upage;
  sp->read_bytes = read_bytes;
  sp->zero_bytes = zero_bytes;
  sp->can_write = can_write;
  sp->valid_access = false;
  sp->type = FILE;
  sp->sticky = false;

  return (hash_insert(&thread_current()->spt, &sp->elem) == NULL);
}

bool add_mmap_to_ptable(struct file *file, int32_t ofs, uint8_t *upage,
			     uint32_t read_bytes, uint32_t zero_bytes)
{
  struct spage *sp = malloc(sizeof(struct spage));
  if (!sp)
    {
      return false;
    }
  sp->file = file;
  sp->offset = ofs;
  sp->data_to_fetch = upage;
  sp->read_bytes = read_bytes;
  sp->zero_bytes = zero_bytes;
  sp->valid_access = false;
  sp->type = MMAP;
  sp->can_write = true;
  sp->sticky = false;

  if (!process_add_mmap(sp))
    {
      free(sp);
      return false;
    }

  if (hash_insert(&thread_current()->spt, &sp->elem))
    {
      sp->type = HASH_ERROR;
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
 struct spage *sp = malloc(sizeof(struct spage));
  if (!sp)
    {
      return false;
    }
  sp->data_to_fetch = pg_round_down(data_to_fetch);
  sp->valid_access = true;
  sp->can_write = true;
  sp->type = SWAP;
  sp->sticky = true;

  uint8_t *frame = allocate_frame (PAL_USER, sp);
  if (!frame)
    {
      free(sp);
      return false;
    }

  if (!install_page(sp->data_to_fetch, frame, sp->can_write))
    {
      free(sp);
      free_frame(frame);
      return false;
    }

  if (intr_context())
    {
      sp->sticky = false;
    }

  return (hash_insert(&thread_current()->spt, &sp->elem) == NULL);
}
