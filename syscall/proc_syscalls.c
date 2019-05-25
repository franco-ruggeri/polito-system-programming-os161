#include <types.h>
#include <thread.h>
#include <proc.h>
#include <addrspace.h>
#include <current.h>
#include <syscall.h>

void sys__exit(int status) {
	(void) status;
	as_destroy(curproc->p_addrspace);
	thread_exit();
}
