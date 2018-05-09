#include "mips_asm_header.h"

MB_HDR mb_hdr;         /* Header area */
MIPS mem[1024];        /* instruction memory, Room for 4K bytes */
MIPS memory[1024];     /* Data memory */

uint32_t PC = 0;                /* program counter */
int reg[NUM_REGS] = {0};
instruction mips_instr[1024]; /* all instructions */
int haltflag;

bskt_ifid ifid;
bskt_idex idex;
bskt_exmem exmem;
bskt_memwb memwb;
bskt_wbif wbif;

int main(int argc, char *argv[]) {
    FILE *fd;
    int total_clocks = 0;

    if (argc != 2) {
        fprintf(stderr, "usage: mips_sim filename\n");
        return EXIT_FAILURE;
    }

    /* format the MIPS Binary header */
    fd = fopen(argv[1], "rb");
    if (fd == NULL) {
        fprintf(stderr, "Couldn't load test case - quitting.\n");
        return EXIT_FAILURE;
    }

    if(verify_header(fd) != 0)
        return EXIT_FAILURE;

    load_instructions(fd);

    /* run simulator */
    for(haltflag = 0; haltflag; total_clocks++, PC += 4) {
        wb(); mem_access(); ex(); id(); ifetch();
    }

    print_regs();
    return 0;
}

void load_instructions(FILE *fd) {
    int i, n;
    int memp;
    memp = 0;        /* This is the memory pointer, a byte offset */
    /* read the binary code a word at a time */
    do {
        n = fread((void *) &mem[memp / 4], 4, 1, fd); /* note div/4 to make word index */
        if (n)
            memp += 4;    /* Increment byte pointer by size of instr */
        else
            break;
    } while (memp < sizeof(mem));

    fclose(fd);

    /* ok, now convert the insructions loaded */
    for (i = 0; i < memp; i += 4) {/* i contains byte offset addresses */
        mips_instr[i/4] = create_instr(mem[i/4]);
    }
}

int verify_header(FILE *fd) {
    /* read the header and verify it. */
    fread((void *) &mb_hdr, sizeof(mb_hdr), 1, fd);
    if (!strcmp(mb_hdr.signature, "~MB") == 0) {
        printf("\nThis isn't really a mips_asm binary file - quitting.\n");
        return EXIT_FAILURE;
    }

    printf("\nProgram size=%d bytes.\n\n", mb_hdr.size);
    return 0;
}


int isolate_bits(int base, int start, int end) {
    int result, mask = 0, i;

    result = base >> end;
    for (i = 0; i < start - end; i++) {
        mask <<= 1;
        mask |= 1;
    }
    return result & mask;
}

instruction create_instr(int opcode) {
    instruction ret;

    ret = malloc(sizeof(struct _instr));
    ret->opcode = (uint8_t)isolate_bits(opcode, 31, 26);

    /* R and I type instructions */
    ret->rs = (uint8_t)isolate_bits(opcode, 25, 21);
    ret->rt = (uint8_t)isolate_bits(opcode, 20, 16);

    /* R type instructions */
    ret->rd = (uint8_t)isolate_bits(opcode, 15, 11);
    ret->shamt = (uint8_t)isolate_bits(opcode, 10, 6);
    ret->funct = (uint8_t)isolate_bits(opcode, 5, 0);

    /* I type instruction */
    ret->immed = (uint8_t)isolate_bits(opcode, 15, 0);

    /* J type instruction */
    ret->word_ind = (uint8_t)isolate_bits(opcode, 25, 0);

    return ret;
}

void wb(void) {
  bskt_memwb.new_in = 0;
  if(/* R-type */) {
    reg[mips_instr[PC].rd] = bskt_memwb.wb_data;
  } else if (/* I-type */){
    reg[mips_instr[PC].rt] = bskt_memwb.wb_data;
  }
}


void mem_access(void) {
  bskt_exmem.new_in = 0;
  bskt_memwb.new_in = 1;
  
  if(/* load type */) {
    bskt_memwb.wb_data = memory[bsk_exmem.alu_result];
  } else {
    bskt_memwb.wb_data = bskt_exmem.alu_result;
  }
}

void ex(void) {
  
}

/**
 * Decode
 */
void id(void) {
  instruction curr_instr;
  curr_instr = mips_instr[PC/4];
  ifid.new_in = 0;
  idex.new_in = 1;
  idex.regA = reg[curr_instr->rs];
  idex.regB = reg[curr_instr->rt];
  idex.sign_ext = (int32_t) curr_instr->immed; /* sign extension through casting */
  idex.left_shift = idex.sign_ext << 2;
  idex.next_pc = &ifid.next_pc; /* only really needs to be done once */
}


/**
 * Fetch
 */
void ifetch(void) {
  /* new data */
  wbif.new_in = 0;
  ifid.new_in = 1;
  
  PC += 4;
  ifid.next_pc = PC + 4;

}

void print_regs(void) {
    uint8_t i = 0;

    for (; i < NUM_REGS; i++) {
        print_reg(i);
        printf(" = %d\n", reg[i]);
    }
}

void print_reg(uint8_t reg) {
    if (reg == 0) {
        printf("$zero");
    } else if (reg == 1) {
        printf("$at");
    } else if (reg == 2 || reg == 3) {
        printf("$v%d", reg-2);
    } else if (reg >= 4 && reg <= 7) {
        printf("$a%d", reg-4);
    } else if (reg >= 8 && reg <= 15) {
        printf("$t%d", reg-8);
    } else if (reg >= 16 && reg <= 23) {
        printf("$s%d", reg-16);
    } else if (reg == 24 || reg == 25) {
        printf("$t%d", reg-24);
    } else if (reg == 26 || reg == 27) {
        printf("$k%d", reg-26);
    } else if (reg == 28) {
        printf("$gp");
    } else if (reg == 29) {
        printf("$sp");
    } else if (reg == 30) {
        printf("$fp");
    } else if (reg == 31) {
        printf("$ra");
    } else {
        printf("unknown");
    }

}
