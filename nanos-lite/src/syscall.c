#include "common.h"
#include "syscall.h"


extern int fs_open(const char *pathname, int flags, int mode);
extern size_t fs_read(int fd, void *buf, size_t len);
extern size_t fs_write(int fd, const void *buf, size_t len);
extern off_t fs_lseek(int fd, off_t offset, int whence);
extern int fs_close(int fd);

_RegSet* do_syscall(_RegSet *r) {
  uintptr_t a[4];
  a[0] = SYSCALL_ARG1(r);
  a[1] = SYSCALL_ARG2(r);
  a[2] = SYSCALL_ARG3(r);
  a[3] = SYSCALL_ARG4(r);

  Log("Syscall ID = %d, args: 0x%x, 0x%x, 0x%x", a[0], a[1], a[2], a[3]);

  switch (a[0]) {
    case SYS_yield:
      r->eax = 0;
      break;

    case SYS_exit:
      _halt(a[1]);
      break;

    case SYS_brk:
      r->eax = 0;
      break;

    case SYS_open:
      r->eax = fs_open((const char *)a[1], (int)a[2], (int)a[3]);
      break;

    case SYS_read:
      r->eax = fs_read((int)a[1], (void *)a[2], (size_t)a[3]);
      break;

    case SYS_write:
      r->eax = fs_write((int)a[1], (const void *)a[2], (size_t)a[3]);
      break;

    case SYS_lseek:
      r->eax = fs_lseek((int)a[1], (off_t)a[2], (int)a[3]);
      break;

    case SYS_close:
      r->eax = fs_close((int)a[1]);
      break;

    case SYS_none:
      r->eax = 1;
      break;

    default:
      panic("Unhandled syscall ID = %d", a[0]);
  }

  return NULL;
}
