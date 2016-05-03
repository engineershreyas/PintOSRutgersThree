#ifndef USERPROG_SYSCALL_H
#define USERPROG_SYSCALL_H

#include "userprog/process.h"
#include <stdbool.h>
#include <stdint.h>

//for global operations or error codes
#define CLOSE_ALL -1
#define ERROR -1


//macros
#define STACK_SIZE 32
#define VADDR_BOTTOM ((void *) 0x08048000)



struct lock file_lock;

void syscall_init (void);

#endif /* userprog/syscall.h */
