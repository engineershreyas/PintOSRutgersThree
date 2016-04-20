#include "init.h"
#include "userprog/pagedir.h"
#include "lib/kernel/list.h"
#include "lib/kernel/hash.h"
#include "vm/frame.h"


struct spage_table spt; //maps to virtual pages one-to-one. has a pointer to the corresponding page table
struct lock spage_table_access;

void spage_table_init(void) {
	spt.pt_ptr = &init_page_dir; //set the page table pointer to the pintos page table
	lock_init(&spage_table_access);
}

void add_to_spage_table(void) {
	//init_page_dir in  the init.h file is used as the main page table
}

void search_page_table(void* fault_addr, size_t page_fault_cnt) {
	//"fault_addr" is the address in which the page fault occured in virtual memory

	
}