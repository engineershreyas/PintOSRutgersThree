#ifndef THREADS_PALLOC_H
#define THREADS_PALLOC_H

#include <stddef.h>
#include "thread.h"
#include "list.h"

/* How to allocate pages. */
enum palloc_flags
  {
    PAL_ASSERT = 001,           /* Panic on failure. */
    PAL_ZERO = 002,             /* Zero page contents. */
    PAL_USER = 004              /* User page. */
  };

/*custom structures */
// because we give each frame a page variable to be mapped to, we can just store the frame onto an list
// a pointer to the frame is the frame location in physical memory
struct frame {
	//designates the thread that owns this frame
	struct thread* owner_thread;
	//designates the page that this frame maps to
	void* page_mapping;
	//allows the frame to be stored to a list
	struct list_elem elem;
};

/* end custom structures */

void palloc_init (size_t user_page_limit);
void *palloc_get_page (enum palloc_flags);
void *palloc_get_multiple (enum palloc_flags, size_t page_cnt);
void palloc_free_page (void *);
void palloc_free_multiple (void *, size_t page_cnt);

/*custom functions */
//returns the frame ptr
void* add_to_frame_table(void* pages, size_t page_cnt);
/* end custom functions */

#endif /* threads/palloc.h */
