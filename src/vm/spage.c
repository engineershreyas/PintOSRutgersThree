#include "threads/init.h"
#include "userprog/pagedir.h"
#include "lib/kernel/list.h"

#include "vm/spage.h"
#include "threads/palloc.h"
#include "threads/malloc.h"
#include "userprog/syscall.h"
#include "filesys/file.h"

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

void set_on_pte(void) { //sets a spage table's parameters based on the page table entry
	//init_page_dir in  the init.h file is used as the main page table

	lock_acquire(& spage_table_access);
	//check if pte exists on the visited pages list, if not then add it
	struct list_elem* e;
	for(e = list_begin(&visited_pages); e != list_end(&visited_pages); e = list_next(e)) {
		struct spage *sp = list_entry(e, struct spage, elem);
		if (sp->pt_ptr == init_page_dir) {
			break;
		}
	}

	struct spage add_supp;
	add_supp.pt_ptr = init_page_dir;

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
	add_supp.read_only = *add_supp.pt_ptr & bit1 ? 0 : 1;
	lock_release(&spage_table_access);

	return;
}

bool page_load(struct spage *sp){
	bool success = false;
	sp->sticky = true;
	if (sp->valid_access) return success;

	switch(sp->type){

		case FILE:
			//TODO: load a file;
			break;
		case SWAP:
			//TODO: load a swap
			break;
		default:
			break;

	}

	return success;

}

bool swap_load(struct spage *sp){
	uint8_t *frame = allocate_frame (PAL_USER, sp);
	if (!frame) return false;
	if(!install_page(sp->data_to_fetch, frame, !sp->read_only)){
		free_frame(frame);
		return false;
	}

	//swap
	sp->valid_access = true;
	return true;
}

bool load_file (struct spage *sp){
	enum palloc_flags flags = PAL_USER;
	if(sp->read_count == 0) flags |= PAL_ZER0;
	uint8_t *frame = allocate_frame(flags,sp);

	if(frame == NULL) return false;

	if(sp->read_count > 0){

		lock_acquire(&file_lock);
		if((int)sp->read_count != file_read_at(sp->file,frame,sp->read_count,sp->offset)){

			lock_release(&file_lock);
			free_frame(frame);
			return false;

		}

		lock_release(&file_lock);
		memset(frame + sp->read_count, 0, sp->zero_counts);

	}


	if(!install_page(sp->data_to_fetch, frame, !sp->read_only)){
		free_frame(frame);
		return false;
	}

	sp->valid_access = true;

	return true;

}

struct spage* get_sp(void *addr){

	struct spage sp;
	sp.data_to_fetch = pg_round_down(addr);

	struct hash_elem *e = hash_find(&thread_current()->supp_page_table, &sp.h_elem);
	if(e == NULL) return NULL;

	return hash_entry (e, struct spage, h_elem);

}


bool stack_grow (void *data){

	if ((size_t)(PHYS_BASE - pg_round_down(data)) > STACK_MAX) return false;

	struct spage *sp = malloc(sizeof(struct spage));

	if(sp == NULL) return false;

	sp->data_to_fetch = pg_round_down(data);
	sp->valid_access = true;
	sp->type = SWAP;
	sp->read_only = false;

	uint8_t *frame = allocate_frame(PAL_USER, sp);

	if(!frame){
		free(sp);
		return false;
	}

	bool success = install_page(sp->data_to_fetch, frame, !(sp->read_only));

	if(!success){
		free(sp);
		free_frame(frame);
		return false;
	}

	struct hash_elem *insert = hash_insert(&thread_current()->supp_page_table, &sp->h_elem);

	return insert == NULL;


}
