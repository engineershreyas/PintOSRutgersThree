#ifndef VM_PAGE_H
#define VM_PAGE_H

#include <hash.h>

#include "vm/frame.h"

#define FILE 0
#define SWAP 1
#define MMAP 2
#define HASH_ERROR 3

#define STACK_MAX (1 << 23)


struct spage {
  uint8_t type;

  void *data_to_fetch;

  bool read_only;
  bool valid_access;
  bool sticky;

  //file stuff
  struct file *file;
  size_t offset;
  size_t read_count;
  size_t zero_count;

  //swap stuff
  size_t swap_mode;

  struct hash_elem h_elem;

};

void spage_table_init (struct hash *spt);
void spage_table_destroy (struct hash *spt);

bool page_load (struct spage *sp);
bool swap_load (struct spage *sp);
bool file_load (struct spage *sp);


bool add_file_to_table (struct file *file, int32_t offset, uint8_t *upage, uint32_t read_count, uint32_t zero_count, bool can_write);
bool add_mmap_to_table(struct file *file, int32_t offset, uint8_t *upage, uint32_t read_count, uint32_t zero_count);
bool stack_grow (void *data_to_fetch);

struct spage* get_sp (void *addr);



#endif
