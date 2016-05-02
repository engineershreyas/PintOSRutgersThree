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


static unsigned spage_hash(const struct hash_elem *e, void *aux UNUSED){
	struct spage *sp = hash_entry(e, struct spage, h_elem);

	return hash_int((int)sp->data_to_fetch);
}


static bool spage_less(const struct hash_elem *a, const struct hash_elem *b, void *aux UNUSED){
	struct spage *sa = hash_entry(a, struct spage, h_elem);
	struct spage *sb = hash_entry(b, struct spage, h_elem);

	if(sa->data_to_fetch < sb->data_to_fetch) return true;

	return false;

}

static bool spage_action(struct hash_elem *e, void *aux UNUSED){
	struct spage *sp = hash_entry(e, struct spage, h_elem);

	if(sp->valid_access){
		free_frame(pagedir_get_page(thread_current()->pagedir, sp->data_to_fetch));
		pagedir_clear_page(thread_current()->pagedir, sp->data_to_fetch);
	}
	free(sp);
}


void spage_table_init(struct hash *spt) {
	list_init(&visited_pages);
	lock_init(&spage_table_access);
	hash_init(spt, spage_hash, spage_less, NULL);
}

void spage_table_destroy(struct hash *spt){
	hash_destroy(spt,spage_action);
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
			success = file_load(sp);
			break;
		case SWAP:
			success = swap_load(sp);
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

	swap_in(sp->swap_mode,sp->data_to_fetch);
	sp->valid_access = true;
	return true;
}

bool file_load (struct spage *sp){
	enum palloc_flags flags = PAL_USER;
	if(sp->read_count == 0) flags |= PAL_ZERO;
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
		memset(frame + sp->read_count, 0, sp->zero_count);

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

bool add_file_to_page_table (struct file *file, int32_t ofs, uint8_t *upage,
			     uint32_t read_bytes, uint32_t zero_bytes,
			     bool writable)
{
  struct spage *sp = malloc(sizeof(struct spage));
  if (!sp)
    {
      return false;
    }
  sp->file = file;
  sp->offset = ofs;
  sp->data_to_fetch = upage;
  sp->read_count = read_bytes;
  sp->zero_count = zero_bytes;
  sp->read_only = !writable;
  sp->valid_access = false;
  sp->type = FILE;

  return (hash_insert(&thread_current()->supp_page_table, &sp->h_elem) == NULL);
}


bool stack_grow (void *data){

	if ((size_t)(PHYS_BASE - pg_round_down(data)) > STACK_MAX) return false;

	struct spage *sp = malloc(sizeof(struct spage));

	if(sp == NULL) return false;

	sp->data_to_fetch = pg_round_down(data);
	sp->valid_access = true;
	sp->type = SWAP;
	sp->read_only = false;
	sp->sticky = true;

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

	if(intr_context()) sp->sticky = false;

	struct hash_elem *insert = hash_insert(&thread_current()->supp_page_table, &sp->h_elem);

	return insert == NULL;


}
