#include "cpu/exec.h"

make_EHelper(mov) {
  operand_write(id_dest, &id_src->val);
  print_asm_template2(mov);
}

make_EHelper(push) {
  if (id_dest->width == 1 || id_dest->width == 2) {
    rtl_sext(&t0, &id_dest->val, id_dest->width);
    rtl_push(&t0);
  } else {
    rtl_push(&id_dest->val);
  }
  
  print_asm_template1(push);
}

make_EHelper(pop) {
  rtl_pop(&t0);
  
  operand_write(id_dest, &t0);
  
  print_asm_template1(pop);
}

make_EHelper(pusha) {
  TODO();

  print_asm("pusha");
}

make_EHelper(popa) {
  TODO();

  print_asm("popa");
}

make_EHelper(leave) {
  rtl_lr(&t0, R_EBP, 4);
  rtl_sr(R_ESP, 4, &t0);

  rtl_pop(&t0);
  rtl_sr(R_EBP, 4, &t0);

  print_asm_template1(leave);
}

make_EHelper(cltd) {
  if (decoding.is_operand_size_16) {
    TODO();
  }
  else {
    TODO();
  }

  print_asm(decoding.is_operand_size_16 ? "cwtl" : "cltd");
}

make_EHelper(cwtl) {
  if (decoding.is_operand_size_16) {
    TODO();
  }
  else {
    TODO();
  }

  print_asm(decoding.is_operand_size_16 ? "cbtw" : "cwtl");
}

make_EHelper(movsx) {
  // 1. Manually override destination width based on operand size prefix
  // IDEXW forced it to 1, but destination is a 16-bit or 32-bit register
  id_dest->width = decoding.is_operand_size_16 ? 2 : 4;
  // 2. Write the zero-extended value to the destination
  // Note: id_src->val is an unsigned uint32_t, so reading a 1-byte
  // source naturally leaves the upper 24 bits as zeros.
  rtl_sext(&t2, &id_src->val, id_src->width);
  operand_write(id_dest, &t2);
  print_asm_template2(movsx);
}

make_EHelper(movzx) {
  id_dest->width = decoding.is_operand_size_16 ? 2 : 4;
  operand_write(id_dest, &id_src->val);
  print_asm_template2(movzx);
}

make_EHelper(lea) {
  rtl_li(&t2, id_src->addr);
  operand_write(id_dest, &t2);
  print_asm_template2(lea);
}
