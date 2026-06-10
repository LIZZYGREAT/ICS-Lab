#ifndef __REG_H__
#define __REG_H__

#include "common.h"

enum { R_EAX, R_ECX, R_EDX, R_EBX, R_ESP, R_EBP, R_ESI, R_EDI };
enum { R_AX, R_CX, R_DX, R_BX, R_SP, R_BP, R_SI, R_DI };
enum { R_AL, R_CL, R_DL, R_BL, R_AH, R_CH, R_DH, R_BH };

 /* TODO: Re-organize the `CPU_state' structure to match the register
 * encoding scheme in i386 instruction format. For example, if we
 * access cpu.gpr[3]._16, we will get the `bx' register; if we access
 * cpu.gpr[1]._8[1], we will get the 'ch' register. Hint: Use `union'.
 * For more details about the register encoding scheme, see i386 manual.
 */

typedef struct {
    union {
       union {
            uint32_t _32;
            uint16_t _16;
            uint8_t  _8[2];
        } gpr[8];

        struct {
            uint32_t eax, ecx, edx, ebx, esp, ebp, esi, edi;
        };
    };

    vaddr_t eip;
    
    union {
        uint32_t value;
        struct {
            uint32_t CF : 1;  // Bit 0: Carry Flag
            uint32_t    : 5;  // Bits 1-5: Don't care
            uint32_t ZF : 1;  // Bit 6: Zero Flag
            uint32_t SF : 1;  // Bit 7: Sign Flag
            uint32_t    : 1;  // Bit 8: Don't care
            uint32_t IF : 1;  // Bit 9: Interrupt Enable Flag
            uint32_t    : 1;  // Bit 10: Don't care
            uint32_t OF : 1;  // Bit 11: Overflow Flag
            uint32_t    : 20; // Bits 12-31: Don't care
        };
    } eflags;

    // Add CS (Code Segment) register
    uint16_t cs;
    
    // Add IDTR register (required for lidt instruction and interrupt routing)
    struct {
        uint32_t base;
        uint16_t limit;
    } idtr;

    struct {
        uint32_t val;
    } cr0, cr3;

} CPU_state;

extern CPU_state cpu; 

/*cpu is  an instance of the CPU_state 
In the meanwhile,cpu is EXTERN*/

static inline int check_reg_index(int index) {
  assert(index >= 0 && index < 8);
  return index;
}
/*check if the index of a reg is valid(0-7) */

#define reg_l(index) (cpu.gpr[check_reg_index(index)]._32)
#define reg_w(index) (cpu.gpr[check_reg_index(index)]._16)
#define reg_b(index) (cpu.gpr[check_reg_index(index) & 0x3]._8[index >> 2])

/* by using define to achieve */ 


extern const char* regsl[];
extern const char* regsw[];
extern const char* regsb[];

static inline const char* reg_name(int index, int width) {
  assert(index >= 0 && index < 8);
  switch (width) {
    case 4: return regsl[index];
    case 1: return regsb[index];
    case 2: return regsw[index];
    default: assert(0);
  }
}

#endif
