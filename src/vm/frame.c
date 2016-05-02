 #include "threads/palloc.h"
#include <bitmap.h>
#include <debug.h>
#include <inttypes.h>
#include <round.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "threads/loader.h"
#include "threads/synch.h"
#include "threads/vaddr.h"
#include "threads/malloc.h"

#include "vm/frame.h"

/*custom variable */
//frame_table is a list of all frames that have been assigned a page.
//the page assignment has been cataloged within the "frame" struct
struct list frame_table;
//lock used to access the frame_table from the thread
struct lock frame_table_access;




/* custom function
* this function is used to map a frame to a page
* accepts the page that needs to be mapped to this frame
*/

void frame_table_init(void) {
  list_init(&frame_table);
  lock_init(&frame_table_access);
}

void* allocate_frame(enum palloc_flags flags, struct spage *sp){

  if((flags & PAL_USER) == 0) return NULL;

  void *frame = palloc_get_page(flags);

  if(frame) add_to_frame_table(frame,sp);
  else{

      //TODO: add logic to find a frame by evicting frame by flags

      if(!frame) PANIC ("Evict not possible, SWAP full");

      add_to_frame_table(frame,sp);

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

void add_to_frame_table_with_pages(void* pages, struct spage *sp) {

	if (list_size(&frame_table) == 512) {
		PANIC("Frame table full!");
	}

	//check to see if the frame already exists on the frame table
	struct list_elem* e;
	for(e = list_begin(&frame_table); e != list_end(&frame_table); e = list_next(e)) {
	  struct frame *frame_in_table = list_entry(e, struct frame, elem);
	  if (frame_in_table->owner_thread == thread_current()) {
	    //then we append additional pages that map to this frame on the frame's "page_mapping" list
	    lock_try_acquire(&frame_table_access);
	    list_push_back(&frame_in_table->page_mapping, pages);
	    lock_release(&frame_table_access);
	    //set the value of e to the "new" frame struct (same thread owner but different page_mapping)
	    *e = frame_in_table->elem;
	    return;
	  }
	}

	struct frame curr_frame;
	struct page_to_frame curr_page;

	//prep page for storing to list
	curr_page.page_to_frame_ptr = pages;
	curr_frame.owner_thread = thread_current();
  curr_frame.has_frame_data = false;
  curr_frame.sp = sp;

	list_push_back(&curr_frame.page_mapping, &curr_page.elem);
	//push to the frame table
	lock_acquire(&frame_table_access);
	list_push_back(&frame_table, &(curr_frame.elem));
	lock_release(&frame_table_access);

}

void add_to_frame_table(void *frame, struct spage *sp){

  struct frame *f = malloc(sizeof(struct frame));

  f->frame = frame;
  f->sp = sp;
  f->owner_thread = thread_current();
  f->has_frame_data = true;
  lock_acquire(&frame_table_access);
  list_push_back(&frame_table,&(f->elem));
  lock_release(&frame_table_access);

}

void* evict_frame (enum palloc_flags flags)
{
  lock_acquire(&frame_table_access);
  struct list_elem *e = list_begin(&frame_table);

  while (true)
    {
      struct frame *f = list_entry(e, struct frame_entry, elem);
      if (!f->sp->sticky)
	       {
	          struct thread *t = f->owner_thread;
	           if (pagedir_is_accessed(t->pagedir, f->sp->data_to_fetch))
	            {
	               pagedir_set_accessed(t->pagedir, f->sp->data_to_fetch, false);
	              }
	           else
	            {
	               if (pagedir_is_dirty(t->pagedir, f->sp->data_to_fetch) ||
		               f->sp->type == SWAP)
		                 {
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
          if (e == list_end(&frame_table))
	       {
	            e = list_begin(&frame_table);
	        }
    }
}


/*end custom function */
