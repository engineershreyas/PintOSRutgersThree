#ifndef USERPROG_PROCESS_H
#define USERPROG_PROCESS_H

#include "threads/thread.h"
#include "threads/synch.h"
#include "threads/vaddr.h"
#include "filesys/file.h"
#include "vm/spage.h"
#include <stdbool.h>
#include <list.h>

/* Function definitions. */
tid_t process_execute (const char *args);
int process_wait (tid_t);
void process_exit (void);
void process_activate (void);
struct process *get_child (pid_t pid);
bool install_page (void *upage, void *kpage, bool writable);

bool process_add_mmap (struct spage *sp);
void process_remove_mmap (int mapping);

/* Definitions of sizes in argument page for args and argv. */
#define ARGS_SIZE PGSIZE / 2                                                       /* File_name+Arguments size. */
#define ARGV_SIZE (PGSIZE - ARGS_SIZE - sizeof (unsigned)) / sizeof (char *)       /* Maximum argument count number. */
#define ARGS_DELI " "                                                              /* Arguments separated by “ “. */
#define WORD_SIZE 4                                                                /* Word size. */
#define BAD_ARGS -1                                                                /* Argument overflow. */

/* A structure for holding process arguments, argv and argc in a page of
   contiguous memory. */
struct args_struct
  {
    char args[ARGS_SIZE];  /* String args. */
    char *argv[ARGV_SIZE]; /* Pointers to args */
    unsigned argc;         /* Number of args. */
  };

/* Data structure for an open file handle. */
struct file_descriptor
  {
    int fd;                 /* File descriptor for file. */
    struct file *file;      /* Pointer to open file. */
    struct list_elem elem;  /* Element for list file lists. */
  };

/*data structure for mmap*/
struct mmap {
  struct spage *sp;
  int id;
  struct list_elem elem;
}

#endif /* userprog/process.h */
