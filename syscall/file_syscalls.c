/*
 * Lab 2
 */

#include <types.h>
#include <lib.h>
#include <kern/unistd.h>  	// STDOUT_FILENO, STDIN_FILENO, STDERR_FILENO
#include <syscall.h>		// prototypes sys_write() and sys_read()

ssize_t sys_write(int fd, const void *buf, size_t count) {
	size_t i;
	char *ptr = (char *) buf;

	/* not stdout, not yet supported */
	if (fd != STDOUT_FILENO)
		return -1;
	
	/* stdout */
	for (i=0; i<count; i++) 
		putch(ptr[i]);
	return count;
}

ssize_t sys_read(int fd, void *buf, size_t count) {
	size_t i;
	char *ptr = (char *) buf;

	/* not stdin, not yet supported */
	if (fd != STDIN_FILENO)
		return -1;

	/* stdin */
	for (i=0; i<count; i++)
		ptr[i] = getch();
	return count;
}
