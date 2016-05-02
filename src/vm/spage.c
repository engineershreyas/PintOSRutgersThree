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
#include "vm/spage.h"
#include "vm/swap.h"

static unsigned spage_hash (const struct hash_elem *e, void *aux UNUSED){
  struct spage *sp = hash_entry(e, struct spage, h_elem);
  return hash_int((int) sp->data_to_fetch);
}

static bool spage_check_less (const struct hash_elem *a, const struct hash_elem *b, void *aux UNUSED){
  struct spage *sa = hash_entry(a, struct spage, h_elem);
  struct spage *sb = hash_entry(b, struct spage, h_elem);
  if(sa->data_to_fetch < sb->data_to_fetch) return true;

  return false;
}

static void spage_action (struct hash_elem *e, void *aux UNUSED){
  struct spage *sp = hash_entry(e, struct spage, h_elem);

  if(sp->valid_access){
    free_frame(pagedir_get_page(thread_current()->pagedir,sp->data_to_fetch));
    pagedir_clear_page(thread_current()->pagedir,sp->data_to_fetch);
  }

  free(sp);

}

void spage_table_init (struct hash *spt){
  hash_init(spt, spage_hash, spage_check_less, NULL);
}

void spage_table_destroy(struct hash *spt){
  hash_destroy(spt, spage_action);
}

struct spage* get_sp(void *addr){
  struct spage sp;
  sp.data_to_fetch = pg_round_down(addr);

  struct hash_elem *e = hash_find(&thread_current()->supp_page_table, &sp.h_elem);
  if(e == NULL) return NULL;

  return hash_entry(e, struct spage, h_elem);
}

bool page_load (struct spage *sp){
  bool success = false;
  sp->sticky = true;
  if(sp->valid_access) return false;

  switch(sp->type){
    case FILE:
      success = file_load(sp);
      break
    case SWAP:
      success = swap_load(sp);
      break;
  }

  return success;
}

bool swap_load(struct spage *sp){
  uint8_t *frame = allocate_frame(PAL_USER, sp);

  if(frame == NULL) return false;

  bool installed = install_page(sp->data_to_fetch, frame, !sp->read_only);

  if(!installed){
    free_frame(frame);
    return false;
  }

  swap_in(sp->swap_mode, sp->data_to_fetch);
  sp->valid_access = true;
  return true;

}


bool file_load(struct spage *sp){
  enum palloc_flags flags = PAL_USER;
  if(sp->read_count == 0) flags |= PAL_ZERO;

  uint8_t *frame = allocate_frame(flags, sp);
  if(frame == NULL) return false;

  if(sp->read_count > 0){
    lock_acquire(&file_lock);
    if((int)sp->read_count != file_read_at(sp->file, frame, sp->read_count,sp->offset)){
      lock_release(&file_lock);
      free_frame(frame);
      return false;
    }

    lock_release(&file_lock);
    memset(frame + sp->read_count, 0, sp->zero_count);

  }

  bool installed = install_page(sp->data_to_fetch, frame, !sp->read_only);

  if(!installed){
    free_frame(frame);
    return false;
  }

  sp->valid_access = true;
  return true;

}

bool add_file_to_table (struct file *file, int32_t offset, uint8_t *upage, uint32_t read_count, uint32_t zero_count, bool can_write){
  struct spage *sp = malloc(sizeof(struct spage));
  if(sp == NULL) return false;

  sp->file = file;
  sp->offset = offset;
  sp->data_to_fetch = upage;
  sp->read_count = read_count;
  sp->zero_count = zero_count;
  sp->read_only = !can_write;
  sp->valid_access = false;
  sp->type = FILE;
  sp->sticky = false;

  return (hash_insert(&thread_current()->supp_page_table, &sp->h_elem) == NULL);
}

bool stack_grow(void *data_to_fetch){
  if((size_t) (PHYS_BASE - pg_round_down(data_to_fetch) > STACK_MAX)) return false;

  struct spage *sp = malloc(sizeof(struct spage));
  if(sp == NULL) return false;

  sp->data_to_fetch = pg_round_down(data_to_fetch);
  sp->valid_access = true;
  sp->read_only = false;
  sp->type = SWAP;
  sp->sticky = true;

  uint8_t *frame = allocate_frame(PAL_USER, sp);
  if(frame == NULL){
    free(sp);
    return false;
  }

  bool installed = install_page(sp->data_to_fetch,frame, !sp->read_only);

  if(!installed){
    free(sp);
    free_frame(frame);
    return false;
  }

  if(intr_context()) sp->sticky = false;

  return (hash_insert(&thread_current()->supp_page_table,&sp->h_elem) == NULL);

}
