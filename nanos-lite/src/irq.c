#include "common.h"

// Declare the external system call handler
extern void do_syscall(_RegSet *r);

_RegSet* do_event(_Event e, _RegSet* r) {
  Log("Inside do_event: event ID = %d, context r = %p", e.event, r);

  switch (e.event) {
    case _EVENT_SYSCALL:
      do_syscall(r); 
      break;
    default: 
      panic("Unhandled event ID = %d", e.event);
  }

  Log("Leaving do_event: returning r = %p", r);
  return r;
}

void init_irq(void) {
  Log("Initializing interrupt/exception handler...");
  _asye_init(do_event);
}
