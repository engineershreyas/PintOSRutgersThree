#ifndef VM_FRAME_H
#define VM_FRAME_H

#include "threads/palloc.h"
#include "threads/synch.h"
#include "vm/page.h"
#include <stdbool.h>
#include <stdint.h>
#include <list.h>

struct lock frame_table_lock;

struct list frame_table;

struct frame_entry {
  void *frame;
  struct spage *sp;
  struct thread *owner_thread;
  struct list_elem elem;
};

void frame_table_init (void);
void* allocate_frame (enum palloc_flags flags, struct spage *sp);
void free_frame (void *frame);
void add_frame_to_table (void *frame, struct spage *sp);
void* evict_frame (enum palloc_flags flags);

#endif /* vm/frame.h */
