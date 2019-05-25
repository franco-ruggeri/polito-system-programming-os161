/*
 * Lab 2
 */

#include <types.h>
#include <lib.h>
#include <kern/unistd.h>
#include <syscall.h>

ssize_t sys_write(int fd, const void *buf, size_t count) {
	size_t i;
	char *ptr = (char *) buf;

	/* not stdout, not yet supported */
	if (fd != STDOUT_FILENO) {
		kprintf("sys_write() does not support this file descriptor\n");
		return -1;
	}
	
	/* stdout */
	for (i=0; i<count; i++) 
		putch(ptr[i]);
	return count;
}

ssize_t sys_read(int fd, void *buf, size_t count) {
	size_t i;
	char *ptr = (char *) buf;

	/* not stdin, not yet supported */
	if (fd != STDIN_FILENO) {
		kprintf("sys_write() does not support this file descriptor\n");
		return -1;
	}

	for (i=0; i<count; i++) {
		ptr[i] = getch();
		if (ptr[i] < 0)
			return i;
	}
	return count;
}