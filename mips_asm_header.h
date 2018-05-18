/*----------------------------------------------------------------------*
 *	mips_asm Binary header (.mb) file header format.  This header	*
 *	provides for future information on mips assembler files.	*
 *									*
 *	write_header - writes mips_asm header, along with test data	*
 *									*
 *	This shows how the mips_asm header is written, and provides	*
 *	a test case (testcase1.mb) to be used for lab projects.		*
 *----------------------------------------------------------------------*/

#ifndef LAB5_MIPS_ASM_HEADER_H
#define LAB5_MIPS_ASM_HEADER_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <unistd.h>

#define NUM_REGS 32

typedef uint32_t MIPS; /* 4 bytes */

typedef struct _mb_hdr {
    char signature[4];        /* Signature = 0x7F, 'M', 'B", 0x00 */
    unsigned int size;        /* Size of assembler program portion, bytes */
    unsigned int entry;        /* Entry point offset of program if not zero */
    unsigned int filler1;        /* Unused ... reserved for future use. */
    unsigned char filler2[64 - 16];    /* Overall header is 64 bytes */
} MB_HDR, *MB_HDR_PTR;

typedef struct _instr {
    uint8_t opcode;
    uint8_t rs;
    uint8_t rt;
    uint8_t rd;
    uint8_t shamt;
    uint8_t funct;
    uint16_t immed;
    uint32_t word_ind;
} *instruction;

typedef struct _ifid { /* instruction fetch/decode basket */
  int new_in;
  uint32_t next_pc;
} bskt_ifid;

typedef struct _idex { /* instruction decode/execute basket */
  int new_in;
  uint32_t regA;
  uint32_t regB;
  int32_t sign_ext;
  uint32_t left_shift;
  uint32_t next_pc;
} bskt_idex;

typedef struct _exmem { /* execute/memory access bakset */
  int new_in;
  int alu_result; /* ALU out */
  uint32_t next_pc;
  /* zero flag ?*/
} bskt_exmem;

typedef struct _memwb { /* memory access/write back basket */
  int new_in;
  uint32_t wb_data;
  uint32_t next_pc;
  
} bskt_memwb;


int isolate_bits(int base, int start, int end);
instruction create_instr(int opcode);
void print_reg(uint8_t reg);
void print_regs(void);

int verify_header(FILE *fd);
void load_instructions(FILE *fd);

void mem_access(void);
void wb(void);
void ex(void);
void id(void);
void ifetch(void);

int branch_type(uint8_t);
int reg_type(uint8_t);
int mem_type(uint8_t);
int j_type(uint8_t);
int load_type(uint8_t opcode);
int store_type(uint8_t opcode);
int sys_type(uint32_t inst);

#endif //LAB5_MIPS_ASM_HEADER_H
