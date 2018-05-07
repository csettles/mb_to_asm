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

typdef struct _ifid { /* instruction fetch/decode basket */
  int new_in;
  uint32_t next_pc;
} bskt_ifid;

typdef struct _idex { /* instruction decode/execute basket */
  int new_in;
  uint32_t regA;
  uint32_t regB;
  int32_t sign_ext;
  uint32_t left_shift;
  uint32_t *next_pc;
} bskt_idex;

typdef struct _exmem { /* execute/memory access bakset */
  int new_in;
  uint32_t alu_result; /* ALU out */
  uint32_t *next_pc; /* points to next pc from idex */
  /* zero flag ?*/
  /* next pc ? */
  
  
} bskt_exmem;

typdef struct _memwb { /* memory access/write back basket */
  int new_in;
  uint32_t mem_data;
  
} bskt_memwb;

typdef struct _wbif { /* write back/instruction fetch basket */
  int new_in;
  
} bskt_wbif;


int isolate_bits(int base, int start, int end);
instruction create_instr(int opcode);
void print_funct(uint8_t funct);
void print_reg(uint8_t reg);

#endif //LAB5_MIPS_ASM_HEADER_H
