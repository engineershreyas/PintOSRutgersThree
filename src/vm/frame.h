#ifndef VM_FRAME_H
#define VM_FRAME_H

#include <stddef.h>
#include "threads/thread.h"
#include "lib/kernel/list.h"
#include "vm/spage.h"
#include "threads/palloc.h"

/*custom structures */
// because we give each frame a page variable to be mapped to, we can just store the frame onto an list
// a pointer to the frame is the frame location in physical memory
struct frame {
	//designates the thread that owns this frame
	struct thread* owner_thread;
	//designates the page that this frame maps to. can have multiple pages that map to a frame
	struct list page_mapping;
	//allows the frame to be stored to a list
	struct list_elem elem;

	//a frame object
	void *frame;

	//indicated to use if pages are in the list or if there is a frame object available
	bool has_frame_data;

	//store the data that the spage finds into a frame
	struct spage *sp;
};

//used to be able to append a pointer of each page that maps to a frame to the "page_mapping" list
struct page_to_frame {
	void* page_to_frame_ptr;
	struct list_elem elem;
};

/*custom functions */
//initialize globals
void frame_table_init(void);
//returns viod
void add_to_frame_table_with_pages(void* pages);

void add_to_frame_table(void *frame, struct spage *sp);

//methods to allocate and free a frame
void* allocate_frame(enum palloc_flags flags, struct spage *sp);
void free_frame(void *frame);
/* end custom functions */

#endif
