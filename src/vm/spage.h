#ifndef VM_SPAGE_H
#define VM_SPAGE_H

#include <stddef.h>
#include "threads/thread.h"
#include "lib/kernel/list.h"
#include "lib/kernel/hash.h"
#include "vm/frame.h"

struct spage_table {
	uint32_t* pt_ptr; //pointer to corresponding page on the page table

	void* data_to_fetch; // if a page has a page fault due to unaccessed data, we use this to store data from the file system, swap slot, or all zero page
	bool valid_access; //if valid_access = 0; then we terminate the process and free the resources of the page that pt_ptr points to (it tried to access an invalid address)

	struct frame* store_to_frame; //find the corresponding frame to store the page to
};

//NOTE: use filesys.h to access data from the file system. These functions will be needed to store data into "data_to_fetch"
void spage_table_init(void);
void add_to_spage_table(void);
void search_page_fault(void* fault_addr, size_t page_fauld_cnt);

#endif