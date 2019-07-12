/*
 * Lab 2
 */

#include <types.h>
#include <thread.h>	// thread_exit()
#include <proc.h>	// proc_getas()
#include <addrspace.h>	// as_destroy()
#include <syscall.h>	// prototype sys__exit()
#if OPT_WAITPID || OPT_FORK || OPT_GETPID
#include <current.h>	// curproc
#include <machine/trapframe.h>
#endif

void sys__exit(int status) {
#if OPT_WAITPID
	proc_exit(status);
#else
	(void) status;	// suppress warning
	as_destroy(proc_getas());
#endif
	thread_exit();
}

#if OPT_WAITPID
pid_t sys_waitpid(pid_t pid, int *status, int options) {
	struct proc *p;
	(void) options;

	if (pid < 0 || (p = proc_search(pid)) == NULL)	// invalid pid
		return -1;
	*status = proc_wait(p);
	return pid;
}
#endif

#if OPT_FORK
void runchild(void *tf, unsigned long unused) {
	(void) unused;
	as_activate();
	enter_forked_process((struct trapframe *) tf);
}

pid_t sys_fork(struct trapframe *tf) {
	struct proc *child;
	pid_t child_pid;
	struct trapframe *child_tf;
	int result;

	/* Create new process */
	child = proc_create_runprogram(curproc->p_name);
	if (child == NULL)
		return -1;

	/*
	 * The address space and the trapframe must be duplicated
	 * before forking the thread, because otherwise the father
	 * may exit before the duplication (freeing as and tf).
	*/

	/* Duplicate address space */
	result = as_copy(curproc->p_addrspace, &child->p_addrspace);
	if (result) {
		proc_destroy(child);
		return -1;
	}

	/* Duplicate trapframe */
	child_tf = kmalloc(sizeof(*child_tf));
	if (child_tf == NULL) {
		proc_destroy(child);
		return -1;
	}
	memcpy(child_tf, tf, sizeof(*tf));
	//*child_tf = *tf;	// equivalent

	/* save child pid to return it to the father */
	child_pid = child->pid;

	/* fork a new kernel thread that runs child (becoming user thread) */
	result = thread_fork(child->p_name, child, runchild, (void *) child_tf, (unsigned long) 0);
	if (result) {
		proc_destroy(child);
		return -1;
	}
	return child_pid;
}
#endif

#if OPT_GETPID
pid_t sys_getpid(void) {
	KASSERT(curproc != NULL);
	return curproc->pid;
}
#endif
