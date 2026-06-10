#include "common.h"
#include "syscall.h"
extern void _yield(); 
_RegSet* do_syscall(_RegSet *r) {
  // Array to store the syscall number and its up to 3 arguments
  uintptr_t a[4];

  // Extract syscall number from eax
  a[0] = SYSCALL_ARG1(r);

  // Extract arguments from ebx, ecx, edx respectively
  a[1] = SYSCALL_ARG2(r);
  a[2] = SYSCALL_ARG3(r);
  a[3] = SYSCALL_ARG4(r);

  switch (a[0]) {
    case SYS_yield:
      r->eax = 0;
      break;

    case SYS_exit:
      // Call AM's halt interface to stop the machine
      // a[1] contains the exit code passed by the user program
      _halt(a[1]);
      break;

    default:
      panic("Unhandled syscall ID = %d", a[0]);
  }

  return NULL;
}
