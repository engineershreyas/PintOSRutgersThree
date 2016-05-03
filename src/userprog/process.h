#ifndef USERPROG_PROCESS_H
#define USERPROG_PROCESS_H

/* Definitions of sizes in argument page for args and argv. */
#define ARGS_SIZE PGSIZE / 2                                                       /* File_name+Arguments size. */
#define ARGV_SIZE (PGSIZE - ARGS_SIZE - sizeof (unsigned)) / sizeof (char *)       /* Maximum argument count number. */
#define ARGS_DELI " "                                                              /* Arguments separated by “ “. */
#define WORD_SIZE 4                                                                /* Word size. */
#define BAD_ARGS -1                                                                /* Argument overflow. */

#include "threads/thread.h"

/* A structure for holding process arguments, argv and argc in a page of
   contiguous memory. */
struct args_struct
  {
    char args[ARGS_SIZE];  /* String args. */
    char *argv[ARGV_SIZE]; /* Pointers to args */
    unsigned argc;         /* Number of args. */
  };

/* Data structure for an open file handle. */
struct file_descriptor {
  int fd;
  struct file *file;
  struct list_elem elem;
};

struct mmap {
  struct spage *sp;
  int mapid;
  struct list_elem elem;
};


tid_t process_execute (const char *file_name);
int process_wait (tid_t);
void process_exit (void);
void process_activate (void);
bool install_page (void *upage, void *kpage, bool can_write);
bool process_add_mmap (struct spage *sp);
void process_remove_mmap (int mapping);

#endif /* userprog/process.h */
