#ifndef USERPROG_SYSCALL_H
#define USERPROG_SYSCALL_H


#include <stdbool.h>
#include <stdint.h>

/* Map region identifier. */
typedef int mapid_t;
#define MAP_FAILED ((mapid_t) -1)

/* Maximum characters in a filename written by readdir(). */
#define READDIR_MAX_LEN 14

/* Typical return values from main() and arguments to exit(). */
#define EXIT_SUCCESS 0          /* Successful execution. */
#define EXIT_FAILURE 1          /* Unsuccessful execution. */

/* Project 3 and optionally project 4. */
mapid_t mmap (int fd, void *addr);

void syscall_init (void);

#endif /* userprog/syscall.h */
