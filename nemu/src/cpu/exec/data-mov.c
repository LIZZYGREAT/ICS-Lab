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

// nemu/src/cpu/exec/data-mov.c

make_EHelper(pusha) {
  // Save current ESP to a temporary variable before consecutive pushes
  rtlreg_t temp_esp = cpu.esp;
  
  // Push all general purpose registers in strict sequence
  rtl_push(&cpu.eax);
  rtl_push(&cpu.ecx);
  rtl_push(&cpu.edx);
  rtl_push(&cpu.ebx);
  rtl_push(&temp_esp);
  rtl_push(&cpu.ebp);
  rtl_push(&cpu.esi);
  rtl_push(&cpu.edi);

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
    // 16-bit mode: CWD (Convert Word to Doubleword)
    // Sign-extend AX into DX
    rtl_lr(&t0, R_EAX, 2);
    
    // Shift left by 16 to move the 16th bit to the MSB, 
    // then arithmetic shift right by 31 to broadcast the sign bit.
    // If AX is negative, t0 becomes 0xFFFFFFFF. If positive, t0 becomes 0.
    rtl_shli(&t0, &t0, 16);
    rtl_sari(&t0, &t0, 31);
    
    // Write the lower 16 bits of the mask to DX
    rtl_sr(R_EDX, 2, &t0);
  } else {
    // 32-bit mode: CDQ (Convert Doubleword to Quadword)
    // Sign-extend EAX into EDX
    rtl_lr(&t0, R_EAX, 4);
    
    // Directly arithmetic shift right by 31 to broadcast the 32nd bit.
    rtl_sari(&t0, &t0, 31);
    
    // Write the 32-bit mask to EDX
    rtl_sr(R_EDX, 4, &t0);
  }

  print_asm_template1(cltd);
}

make_EHelper(cwtl) {
  if (decoding.is_operand_size_16) {
    /* Sign extend AL to AX */
    reg_w(R_AX) = (int16_t)(int8_t)reg_b(R_AL);
  } else {
    /* Sign extend AX to EAX */
    reg_l(R_EAX) = (int32_t)(int16_t)reg_w(R_AX);
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
