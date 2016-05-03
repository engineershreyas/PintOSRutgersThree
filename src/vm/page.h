#ifndef VM_PAGE_H
#define VM_PAGE_H

#include <hash.h>
#include "vm/frame.h"

#define FILE 0
#define SWAP 1
#define MMAP 2
#define HASH_ERROR 3

// 256 KB
#define MAX_STACK_SIZE (1 << 23)

struct spage {
  uint8_t type;
  void *data_to_fetch;
  bool can_write;

  bool valid_access;
  bool sticky;

  // For files
  struct file *file;
  size_t offset;
  size_t read_bytes;
  size_t zero_bytes;

  // For swap
  size_t swap_mode;

  struct hash_elem elem;
};

void page_table_init (struct hash *spt);
void page_table_destroy (struct hash *spt);

bool page_load (struct spage *sp);
bool swap_load (struct spage *sp);
bool file_load (struct spage *sp);
bool add_file_to_ptable (struct file *file, int32_t ofs, uint8_t *upage,
			     uint32_t read_bytes, uint32_t zero_bytes,
			     bool can_write);
bool add_mmap_to_ptable(struct file *file, int32_t ofs, uint8_t *upage,
			    uint32_t read_bytes, uint32_t zero_bytes);
bool stack_grow (void *data_to_fetch);
struct spage* get_sp (void *data_to_fetch);

#endif /* vm/page.h */
