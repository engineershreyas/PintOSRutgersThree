#ifndef VM_FRAME_H
#define VM_FRAME_H

#include <stddef.h>
#include "thread.h"
#include "list.h"

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

	//store the data that the spage finds into a frame
	void* data_from_spage;
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
void add_to_frame_table(void* pages, size_t page_cnt);
/* end custom functions */

#endif