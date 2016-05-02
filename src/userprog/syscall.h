#ifndef USERPROG_SYSCALL_H
#define USERPROG_SYSCALL_H

#include "userprog/process.h"
#include <stdbool.h>
#include <stdint.h>

#define USER_VADDR_BOTTOM ((void *) 0x08048000)
#define STACK_HEURISTIC 32

struct lock file_lock;

void syscall_init (void);
struct spage* check_valid_ptr(const void *vaddr, void* esp);
void check_valid_buffer (void* buffer, unsigned size, void* esp,
			 bool to_write);
void check_valid_string (const void* str, void* esp);

#endif /* userprog/syscall.h */
