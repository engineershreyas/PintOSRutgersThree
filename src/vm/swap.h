#ifndef VM_SWAP_H
#define VM_SWAP_H


#include <bitmap.h>

#include "threads/synch.h"
#include "threads/vaddr.h"

#include "devices/block.h"

#define SWAP_FREE 0
#define SWAP_USE 1

#define SECS_PER_PAGE (PGSIZE / BLOCK_SECTOR_SIZE)

struct lock swap_access;

struct block *swap_block;

struct bitmap *swap_map;

void init_swap (void);
size_t swap_out (void *frame);
void swap_in (size_t used_index, void *frame);


#endif
