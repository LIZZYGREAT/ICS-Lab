#include "common.h"

// Declare the external system call handler
extern void do_syscall(_RegSet *r);

static _RegSet* do_event(_Event e, _RegSet* r) {
  switch (e.event) {
    case _EVENT_SYSCALL:
      // Dispatch the context pointer to the system call handler
      do_syscall(r);
      break;
    case _EVENT_TRAP:
      break;
    default: panic("Unhandled event ID = %d", e.event);
  }

  // Return NULL means we just resume the current execution context
  return NULL;
}

void init_irq(void) {
  Log("Initializing interrupt/exception handler...");
  _asye_init(do_event);
}
