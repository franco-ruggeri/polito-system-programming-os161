#include <types.h>
#include <lib.h>
#include <syscall.h>

ssize_t sys_write(int fd, const void *buf, size_t count) {
  (void) fd;
  (void) buf;
  (void) count;
  kprintf("I'm here!\n");
  return 0;
}

//void sys_read() {

//}
