/*
 * Lab 2
 */

#include <types.h>
#include <thread.h>	// thread_exit()
#include <proc.h>	// proc_getas()
#include <addrspace.h>	// as_destroy()
#include <syscall.h>	// prototype sys__exit()

void sys__exit(int status) {
	(void) status;
	as_destroy(proc_getas());
	thread_exit();
}
