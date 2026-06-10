#include "cpu/exec.h"
#include "memory/mmu.h"

void raise_intr(uint8_t NO, vaddr_t ret_addr) {
  // 1. Push EFLAGS, CS, and EIP into the stack
  rtlreg_t eflags = cpu.eflags.value;
  rtl_push(&eflags);
  
  cpu.eflags.IF = 0;  
  
  rtlreg_t cs = cpu.cs; 
  rtl_push(&cs);
  
  rtlreg_t eip = ret_addr;
  rtl_push(&eip);

  // 2. Locate the Gate Descriptor in IDT (8 bytes per entry)
  vaddr_t gate_addr = cpu.idtr.base + NO * 8;
  
  // 3. Read target address from the Gate Descriptor
  // Low 16 bits are located at bytes 0-1
  uint32_t offset_low = vaddr_read(gate_addr, 2);
  // High 16 bits are located at bytes 6-7
  uint32_t offset_high = vaddr_read(gate_addr + 6, 2);
  
  // 4. Reconstruct the 32-bit target entry address
  uint32_t target_addr = (offset_high << 16) | offset_low;
  
  // 5. Force jump to the target address
  decoding.jmp_eip = target_addr;
  decoding.is_jmp = 1;
}

void dev_raise_intr() {
}
