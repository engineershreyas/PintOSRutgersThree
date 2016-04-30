#include <stddef.h>
#include "threads/thread.h"
#include "lib/kernel/list.h"
#include "lib/kernel/hash.h"
#include "vm/frame.h"

#ifndef VM_SPAGE_H
#define VM_SPAGE_H

extern struct list visited_pages;
//struct spage_table add_supp; //maps to virtual pages one-to-one. has a pointer to the corresponding page table
extern struct lock spage_table_access;

struct spage {
	uint32_t* pt_ptr; //pointer to corresponding page on the page table

	void* data_to_fetch; // if a page has a page fault due to unaccessed data, we use this to store data from the file system, swap slot, or all zero page
	bool valid_access; //if valid_access = 0; then we terminate the process and free the resources of the page that pt_ptr points to (it tried to access an invalid address)
	bool read_only; //checks if the pt_ptr points to read-only memory

	struct list_elem elem;
};




//NOTE: use filesys.h to access data from the file system. These functions will be needed to store data into "data_to_fetch"
void spage_table_init(void);
void store_visited_page(void); //pass current pt address to the supplemental pt
void set_on_pte(void);

#endif