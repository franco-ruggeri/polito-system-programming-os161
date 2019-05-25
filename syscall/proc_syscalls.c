/*
 * Lab 2
 */

#include <types.h>
#include <thread.h>	// thread_exit()
#include <proc.h>	// struct proc
#include <addrspace.h>	// as_destroy()
#include <current.h>	// curproc
#include <syscall.h>	// prototype sys__exit()

void sys__exit(int status) {
	(void) status;
	as_destroy(curproc->p_addrspace);
	thread_exit();
}
