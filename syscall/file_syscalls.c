/*
 * Lab 2
 */

#include <types.h>
#include <lib.h>
#include <kern/unistd.h>
#include <kern/errno.h>
#include <vfs.h>	
#include <vnode.h>
#include <proc.h>
#include <uio.h>
#include <syscall.h>	// prototypes

#if OPT_FILEIO
int sys_open(const char *pathname, int flags, mode_t mode, int *retval) {
	struct vnode *v;
	int result;

	/* open file */
	result = vfs_open((char *) pathname, flags, mode, &v);
	if (result) return result;
	
	/* assign fd */
	result = proc_add_file(v, retval);
	if (result) return result;

	return 0;
}

int sys_close(int fd) {
	return proc_remove_file(fd);
}
#endif

#if OPT_SYSCALL_BASIC
int sys_write(int fd, const void *buf, size_t count, size_t *retval) {
	size_t i;
	int result;
	char *ptr;

	if (fd == STDOUT_FILENO || fd == STDERR_FILENO) {
		/* stdout or stderr => console */
		ptr = (char *) buf;
		for (i=0; i<count; i++) 
			putch(ptr[i]);
		*retval = count;
	} else {
#if OPT_FILEIO
		struct vnode *v;
		struct iovec iov;
		off_t offset;
		struct uio u;

		/* retrieve vnode */
		result = proc_search_file(fd, &v, &offset);
		if (result)
			return result;
		
		
		/* write */
		uio_uinit(&iov, &u, (void *) buf, count, offset, UIO_WRITE, proc_getas());
		result = VOP_WRITE(v, &u);
		if (result)
			return result;
		*retval = count - u.uio_resid;
		proc_set_file_offset(fd, u.uio_offset);
	}
#else
		return ENOSYS;
	}
#endif
	return 0;
}

int sys_read(int fd, void *buf, size_t count, size_t *retval) {
	size_t i;
	int result;
	char *ptr;

	if (fd == STDIN_FILENO) {
		/* stdin => console */
		ptr = (char *) buf;
		for (i=0; i<count; i++) {
			result = getch();
			if (result < 0)
				break;
			ptr[i] = (char) result;
		}
		*retval = i;
	} else {
#if OPT_FILEIO
		struct vnode *v;
		struct iovec iov;
		off_t offset;
		struct uio u;

		/* retrieve vnode */
		result = proc_search_file(fd, &v, &offset);
		if (result)
			return result;
		
		/* read */
		uio_uinit(&iov, &u, buf, count, offset, UIO_READ, proc_getas());
		result = VOP_READ(v, &u);
		if (result)
			return result;
		*retval = count - u.uio_resid;
		proc_set_file_offset(fd, u.uio_offset);
	}
#else
		return ENOSYS;
	}
#endif

	return 0;
}
#endif
