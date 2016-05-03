#include "vm/swap.h"

void init_swap (void){
  swap_block = block_get_role(BLOCK_SWAP);
  if(swap_block == NULL) return;

  swap_map = bitmap_create(block_size(swap_block) / SECS_PER_PAGE);
  if(swap_map == NULL) return;

  bitmap_set_all(swap_map, SWAP_FREE);

  lock_init(&swap_access);
}


size_t swap_out (void *frame){
  if(swap_block == NULL || !swap_map == NULL) PANIC("Swap partition needed but not present");

  lock_acquire(&swap_access);

  size_t free_ind = bitmap_scan_and_flip(swap_map, 0, 1, SWAP_FREE);

  if(free_ind == BITMAP_ERROR) PANIC("Swap partition is full");

  size_t i;
  for(i = 0; i < SECS_PER_PAGE; i++) block_write(swap_block, free_ind * SECS_PER_PAGE + i, (uint8_t *) frame + i * BLOCK_SECTOR_SIZE);


  lock_release(&swap_access);
  return free_ind;

}

void swap_in (size_t used_index, void *frame){
  if(swap_block == NULL || swap_map == NULL) return

  lock_acquire(&swap_access);
  if(bitmap_test(swap_map, used_index) == SWAP_FREE) PANIC("Trying to swap in free block");

  bitmap_flip(swap_map, used_index);

  size_t i;
  for(i = 0; i < SECS_PER_PAGE; i++) block_read(swap_block, used_index * SECS_PER_PAGE + i, (uint8_t) frame + i * BLOCK_SECTOR_SIZE);


  lock_release(&swap_access);

}
