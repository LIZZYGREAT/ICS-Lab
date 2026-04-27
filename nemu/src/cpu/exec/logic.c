#include "cpu/exec.h"

make_EHelper(test) {
  TODO();

  print_asm_template2(test);
}

make_EHelper(and) {
  // 1. Perform bitwise AND operation: t0 = dest & src
  rtl_and(&t0, &id_dest->val, &id_src->val);
  
  // 2. Write the result back to the destination operand
  operand_write(id_dest, &t0);

  // 3. Update EFLAGS: ZF and SF based on the result
  rtl_update_ZF(&t0, id_dest->width);
  rtl_update_SF(&t0, id_dest->width);

  // 4. Clear CF and OF as per i386 manual for AND instruction
  rtl_li(&t1, 0);
  rtl_set_CF(&t1);
  rtl_set_OF(&t1);

  print_asm_template2(and);
}

make_EHelper(xor) {
  // 1. Calculate bitwise XOR and store in temporary register t0
  rtl_xor(&t0, &id_dest->val, &id_src->val);
  
  // 2. Write the result back to the destination
  operand_write(id_dest, &t0);
  
  // 3. Update EFLAGS: ZF and SF according to the result
  rtl_update_ZF(&t0, id_dest->width);
  rtl_update_SF(&t0, id_dest->width);
  
  // 4. Update EFLAGS: CF and OF must be cleared for logical instructions
  rtl_li(&t1, 0);
  rtl_set_CF(&t1);
  rtl_set_OF(&t1);
  
  // 5. Print assembly log
  print_asm_template2(xor);
}

make_EHelper(or) {
  TODO();

  print_asm_template2(or);
}

make_EHelper(sar) {
  TODO();
  // unnecessary to update CF and OF in NEMU

  print_asm_template2(sar);
}

make_EHelper(shl) {
  TODO();
  // unnecessary to update CF and OF in NEMU

  print_asm_template2(shl);
}

make_EHelper(shr) {
  TODO();
  // unnecessary to update CF and OF in NEMU

  print_asm_template2(shr);
}

make_EHelper(setcc) {
  uint8_t subcode = decoding.opcode & 0xf;
  rtl_setcc(&t2, subcode);
  operand_write(id_dest, &t2);

  print_asm("set%s %s", get_cc_name(subcode), id_dest->str);
}

make_EHelper(not) {
  TODO();

  print_asm_template1(not);
}
