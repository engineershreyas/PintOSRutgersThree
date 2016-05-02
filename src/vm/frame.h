#ifndef VM_FRAME_H
#define VM_FRAME_H

#include <stdbool.h>
#include <stdint.h>
#include <list.h>

#include "threads/palloc.h"
#include "threads/synch.h"
#include "vm/page.h"

struct lock frame_table_access;

struct list frame_table;

//struct representing a frame
struct frame {
  void *frame //frame object
  struct thread *owner_thread; //thread that owns this frame
  struct spage *sp; //supplemental page mapping to this frame

  struct list_elem elem;
};


void frame_table_init(void);
void* allocate_frame(enum palloc_flags flags, struct spage *sp);
void free_frame (void *frame);
void add_frame_to_table (void *frame, struct spage *sp);
void* evict_frame (enum palloc_flags flags);


#endif
