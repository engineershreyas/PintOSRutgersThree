#include "threads/init.h"
#include "userprog/pagedir.h"
#include "lib/kernel/list.h"
#include "vm/frame.h"
#include "filesys/file.h"
#include "vm/spage.h"
#include "threads/synch.h"

/*REMEMBER:
the page table pointer points to the physical addresses
*/

struct list visited_pages;
//struct spage_table add_supp; //maps to virtual pages one-to-one. has a pointer to the corresponding page table
struct lock spage_table_access;

void spage_table_init(void) {
	list_init(&visited_pages);
	lock_init(&spage_table_access);
}

void set_on_pte(void* buffer) { //sets a spage table's parameters based on the page table entry 
	
	lock_acquire(& spage_table_access);
	//check if pte exists on the visited pages list, if not then add it
	struct list_elem* e;
	for(e = list_begin(&visited_pages); e != list_end(&visited_pages); e = list_next(e)) {
		struct spage *sp = list_entry(e, struct spage, elem);
		if (sp->pt_ptr == buffer) {
			break;
		}
	}

	struct spage add_supp;
	add_supp.pt_ptr = buffer;

	if (*add_supp.pt_ptr <= PHYS_BASE) {
		add_supp.valid_access = 1;
	} else {
		add_supp.valid_access = 0;
		lock_release(&spage_table_access);
		return;
	}

	//check if data is availible at this location
	if (*add_supp.pt_ptr == NULL) {
		add_supp.valid_access = 0;
		lock_release(&spage_table_access);
		return;
	}

	uint32_t bit1 = 0x02; //bit 2 of the page table entry states if the address is writable;

	//checks if bit 2 of the page table entry is set
	//*add_supp.pt_ptr & bit1 ? stp.read_only = 0 : add_supp.read_only = 1;

	if (*add_supp.pt_ptr & bit1) {
		add_supp.read_only = 0;
	} else {
		add_supp.read_only = 1;
	}

	list_push_back(&visited_pages, &add_supp.elem);
	lock_release(&spage_table_access);

	return;
}

void spage_load_file(void* fault_addr) {
	 if (fault_addr > PHYS_BASE) {
      PANIC("faulted Address in kernel space!");
    }

    lock_acquire(&spage_table_access);
    struct spage* curr_sp;
    struct list_elem* e;
    for(e = list_begin(&visited_pages); e != list_end(&visited_pages); e = list_next(e)) {
      struct spage *sp = list_entry(e, struct spage, elem);
      if (sp->pt_ptr == (uint32_t*)fault_addr) {

        //at this point, sp->pt_ptr contains the faulted address
        if (sp->valid_access == 0) {
          lock_release(&spage_table_access);
          PANIC("Faulted address access invalid!");
          return;
        }

        sp->pt_ptr = file_open(inode_open(*sp->pt_ptr));
      }
    }



    lock_release(&spage_table_access);
}