#include "vm/frame.h"
#include "vm/frame.h"
#include "vm/page.h"
#include "vm/swap.h"

#include "filesys/file.h"

#include "userprog/pagedir.h"
#include "userprog/syscall.h"

#include "threads/malloc.h"
#include "threads/palloc.h"
#include "threads/synch.h"
#include "threads/thread.h"

void frame_table_init (void){
  list_init(&frame_table);
  lock_init(&frame_table_access);
}

void* allocate_frame(enum palloc_flags flags, struct spage *sp){

    if((flags & PAL_USER) == 0) return NULL;

    void *frame = palloc_get_page(flags);
    if (frame) add_frame_to_table(frame, sp);
    else{
      while (!frame){
        frame = evict_frame(flags);
        lock_release(&frame_table_access);
      }

      if(!frame) PANIC ("Frame couldn't be evicted because swap is full");
      add_frame_to_table(frame, sp);
  }

  return frame;

}

void free_frame(void *frame){
  struct list_elem *e;
  lock_acquire(&frame_table_access);
  for(e = list_begin(&frame_table); e != list_end(&frame_table); e = list_next(e)){
    struct frame *f = list_entry(e, struct frame, elem);
    if(f->frame == frame){
      list_remove(e);
      free(f);
      palloc_free_page(frame);
      break;
    }
  }

  lock_release(&frame_table_access);

}

void add_frame_to_table(void *frame, struct spage *sp){
  struct frame *f = malloc(sizeof(struct frame));
  f->frame = frame;
  f->sp = sp;
  f->owner_thread = thread_current();
  lock_acquire(&frame_table_access);
  list_push_back(&frame_table,&f->elem);
  lock_release(&frame_table_access);
}

void *frame_evict(enum palloc_flags flags){
  lock_acquire(&frame_table_access);
  struct list_elem *e = list_begin(&frame_table);

  while (true){
    struct frame *f = list_entry(e, struct frame, elem);
    if(!f->sp->sticky){
      struct thread *t = f->owner_thread;
      if(pagedir_is_accessed(t->pagedir, f->sp->data_to_fetch)){
        pagedir_set_accessed(t->pagedir,f->sp->data_to_fetch, false);
      }
      else{
        if(pagedir_is_dirty(t->pagedir, f->sp->data_to_fetch) ||
          f->sp->type == SWAP){
            f->sp->type = SWAP;
            f->sp->swap_mode = swap_out(f->frame);
        }

        f->sp->valid_access = false;
        list_remove(&f->elem);
        pagedir_clear_page(t->pagedir, f->sp->data_to_fetch);
        palloc_free_page(f->frame);
        free(f);
        return palloc_get_page(flags);
      }

    }

    e = list_next(e);
    if(e == list_end(&frame_table)) e = list_begin(&frame_table);

  }

}
