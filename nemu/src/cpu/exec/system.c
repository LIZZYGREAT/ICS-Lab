#include "cpu/exec.h"

void diff_test_skip_qemu();
void diff_test_skip_nemu();


extern void raise_intr(uint8_t NO, vaddr_t ret_addr);

make_EHelper(lidt) {
  // Read 2 bytes of limit from the target memory address
  cpu.idtr.limit = vaddr_read(id_dest->addr, 2);
  
  // Read 4 bytes of base address starting from target address + 2
  cpu.idtr.base = vaddr_read(id_dest->addr + 2, 4);

  print_asm_template1(lidt);
}

make_EHelper(mov_r2cr) {
  TODO();

  print_asm("movl %%%s,%%cr%d", reg_name(id_src->reg, 4), id_dest->reg);
}

make_EHelper(mov_cr2r) {
  TODO();

  print_asm("movl %%cr%d,%%%s", id_src->reg, reg_name(id_dest->reg, 4));

#ifdef DIFF_TEST
  diff_test_skip_qemu();
#endif
}


make_EHelper(int) {
  // Extract the exception number from the instruction operand
  uint8_t NO = id_dest->val & 0xff;
  
  // Call hardware interrupt handling function with sequence EIP as return address
  raise_intr(NO, decoding.seq_eip);

  print_asm("int %s", id_dest->str);
#ifdef DIFF_TEST
    diff_test_skip_nemu();
#endif
}


make_EHelper(iret) {
  rtl_pop(&decoding.jmp_eip);
  decoding.is_jmp = 1;  
  
  rtl_pop(&t0);
  cpu.cs = (uint16_t)t0;
  
  rtl_pop(&cpu.eflags.value);
  
  print_asm("iret");
}

uint32_t pio_read(ioaddr_t, int);
void pio_write(ioaddr_t, int, uint32_t);

make_EHelper(in) {
  /* Port address is implicitly stored in the %dx register */
  ioaddr_t port = reg_w(R_DX);

  /* Read data from the specified I/O port based on instruction width */
  t0 = pio_read(port, id_dest->width);

  /* Configure the destination operand to implicitly target %al/%ax/%eax */
  id_dest->type = OP_TYPE_REG;
  id_dest->reg  = R_EAX;
  operand_write(id_dest, &t0);

  print_asm_template2(in);

#ifdef DIFF_TEST
  diff_test_skip_qemu();
#endif
}

make_EHelper(out) {
  /* Port address is implicitly stored in the %dx register */
  ioaddr_t port = reg_w(R_DX);

  /* Write data to the specified physical I/O port bus */
  pio_write(port, id_src->width, id_src->val);

  print_asm_template2(out);

#ifdef DIFF_TEST
  diff_test_skip_qemu();
#endif
}

make_EHelper(cli) {
  // Clear the Interrupt Flag (Disable interrupts)
  cpu.eflags.IF = 0;
  print_asm("cli");
}

make_EHelper(sti) {
  // Set the Interrupt Flag (Enable interrupts)
  cpu.eflags.IF = 1;
  print_asm("sti");
}
