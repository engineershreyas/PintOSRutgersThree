#ifndef USERPROG_PROCESS_H
#define USERPROG_PROCESS_H

#include "threads/thread.h"

/* Definitions of sizes in argument page for args and argv. */
#define ARGS_SIZE PGSIZE / 2                                                       /* File_name+Arguments size. */
#define ARGV_SIZE (PGSIZE - ARGS_SIZE - sizeof (unsigned)) / sizeof (char *)       /* Maximum argument count number. */
#define ARGS_DELI " "                                                              /* Arguments separated by “ “. */
#define WORD_SIZE 4                                                                /* Word size. */
#define BAD_ARGS -1  

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


struct mmap {
  struct spage *sp;
  int mapid;
  struct list_elem elem;
};

int process_add_file (struct file *f);
struct file* process_get_file (int fd);
tid_t process_execute (const char *file_name);
int process_wait (tid_t);
void process_exit (void);
void process_activate (void);
bool install_page (void *upage, void *kpage, bool can_write);
bool process_add_mmap (struct spage *sp);
void process_remove_mmap (int mapping);

#endif /* userprog/process.h */
