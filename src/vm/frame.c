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

void add_to_frame_table(void* pages, size_t page_cnt) {

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
	    return 1;
	  }
	}

	struct frame curr_frame;
	struct page_to_frame curr_page;

	//prep page for storing to list
	curr_page.page_to_frame_ptr = pages;
	curr_frame.owner_thread = thread_current();

	list_push_back(&curr_frame.page_mapping, &curr_page.elem);
	//push to the frame table
	lock_acquire(&frame_table_access);
	list_push_back(&frame_table, &curr_frame);
	lock_release(&frame_table_access);

}

/*end custom function */