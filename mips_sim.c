#include "mips_asm_header.h"

MB_HDR mb_hdr;         /* Header area */
MIPS mem[1024];        /* instruction memory, Room for 4K bytes */

uint32_t PC = 4;                /* program counter */
int reg[NUM_REGS] = {0};
instruction mips_instr[1024]; /* all instructions */
int haltflag;

bskt_ifid ifid;
bskt_idex idex;
bskt_exmem exmem;
bskt_memwb memwb;

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

    if (verify_header(fd) != 0)
        return EXIT_FAILURE;

    load_instructions(fd);

    /* run simulator */
    for (haltflag = 0; !haltflag; total_clocks++) {
        printf("PC=%d\n", PC);
        wb();
        mem_access();
        ex();
        id();
        ifetch();
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
        mips_instr[i / 4] = create_instr(mem[i / 4]);
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
    for (i = 0; i < (start - end + 1); i++) {
        mask <<= 1;
        mask |= 1;
    }
    return result & mask;
}

instruction create_instr(int opcode) {
    instruction ret;

    ret = malloc(sizeof(struct _instr));
    ret->opcode = (uint8_t) isolate_bits(opcode, 31, 26);

    /* R and I type instructions */
    ret->rs = (uint8_t) isolate_bits(opcode, 25, 21);
    ret->rt = (uint8_t) isolate_bits(opcode, 20, 16);

    /* R type instructions */
    ret->rd = (uint8_t) isolate_bits(opcode, 15, 11);
    ret->shamt = (uint8_t) isolate_bits(opcode, 10, 6);
    ret->funct = (uint8_t) isolate_bits(opcode, 5, 0);

    /* I type instruction */
    ret->immed = (int16_t) isolate_bits(opcode, 15, 0);

    /* J type instruction */
    ret->word_ind = (uint32_t) isolate_bits(opcode, 25, 0);

    return ret;
}

void wb(void) {
    if (memwb.new_in == 0) {
        return;
    }

    instruction curr_instr = mips_instr[memwb.next_pc / 4 - 1];
    memwb.new_in = 0;

    /* reg instr */
    if (reg_type(curr_instr->opcode)) {
        if (curr_instr->funct == 9) { //jalr
            reg[31] = memwb.next_pc;
        } else {
            reg[curr_instr->rd] = memwb.wb_data;
        }


    } else if (curr_instr->opcode == 3) { /* jal */
        reg[31] = memwb.next_pc;

        /* immediate instr excluding branch and store */
    } else if (!j_type(curr_instr->opcode) && \
         !branch_type(curr_instr->opcode) && \
         !store_type(curr_instr->opcode)) {
        reg[curr_instr->rt] = memwb.wb_data;
    }
}


void mem_access(void) {
    instruction inst;

    if (exmem.new_in == 0)
        return;

    inst = mips_instr[exmem.next_pc / 4 - 1];

    exmem.new_in = 0;
    memwb.new_in = 1;
    memwb.next_pc = exmem.next_pc;

    if (load_type(inst->opcode)) {
        memwb.wb_data = (uint32_t) mem[exmem.alu_result / 4];
    } else {
        memwb.wb_data = (uint32_t) exmem.alu_result;
    }

}

void ex(void) {
    instruction inst;

    if (idex.new_in == 0)
        return;

    inst = mips_instr[idex.next_pc / 4 - 1];
    idex.new_in = 0;
    exmem.new_in = 1;
    exmem.next_pc = idex.next_pc;

    if (sys_type(mem[idex.next_pc / 4 - 1]) /*&& reg[2] == 10*/) {
        haltflag++;
        printf("halting...\n");
    } else if (mem_type(inst->opcode)) {
        exmem.alu_result = idex.regA + idex.sign_ext;
        exmem.next_pc = idex.next_pc;
    } else if (branch_type(inst->opcode)) {
        exmem.alu_result = 0; // this does not matter
        switch (inst->opcode) {
            case 0x04: // beq
                if (idex.regA == idex.regB) {
                    PC = idex.next_pc + idex.left_shift;
                    clear_buckets(3);
                } else {
                    exmem.next_pc = idex.next_pc;
                }
                break;
            case 0x05:
                if (idex.regA != idex.regB) {
                    PC = idex.next_pc + idex.left_shift;
                    clear_buckets(3);
                } else {
                    exmem.next_pc = idex.next_pc;
                }
                break;
        }
    } else if (reg_type(inst->opcode)) {
        exmem.next_pc = idex.next_pc;
        if (inst->funct == 0) { // sll
            exmem.alu_result = idex.regB << inst->shamt;
        } else if (inst->funct == 2) { //srl
            exmem.alu_result = idex.regB >> inst->shamt;
        } else if (inst->funct == 3) { //sra
            exmem.alu_result = (uint32_t) ((int32_t) idex.regB >> inst->shamt);
        } else if (inst->funct == 4) { //sllv
            exmem.alu_result = idex.regB << idex.regA;
        } else if (inst->funct == 6) { //srlv
            exmem.alu_result = idex.regB >> idex.regA;
        } else if (inst->funct == 7) { //srav
            exmem.alu_result = (uint32_t) ((int32_t) idex.regB << idex.regA);
        } else if (inst->funct == 8) { //jr
            /* exmem.next_pc = idex.regA; */
        } else if (inst->funct == 9) { //jalr
            exmem.alu_result = idex.next_pc;
        } else if (inst->funct == 12) { //syscall
            printf("there was a syscall, don't know how to handle\n");
        } else if (inst->funct == 32) { //add
            exmem.alu_result = idex.regA + idex.regB;
        } else if (inst->funct == 33) { //addu
            exmem.alu_result = idex.regB + idex.regA;
        } else if (inst->funct == 34) { //sub
            exmem.alu_result = idex.regA - idex.regB;
        } else if (inst->funct == 35) { //subu
            exmem.alu_result = idex.regA - idex.regB;
        } else if (inst->funct == 36) { //and
            exmem.alu_result = idex.regA & idex.regB;
        } else if (inst->funct == 37) { //or
            exmem.alu_result = idex.regB | idex.regA;
        } else if (inst->funct == 38) { //xor
            exmem.alu_result = idex.regB ^ idex.regA;
        } else if (inst->funct == 39) { //nor
            exmem.alu_result = ~(idex.regB | idex.regA);
        } else if (inst->funct == 42) { //slt
            exmem.alu_result = ((int32_t) idex.regA < (int32_t) idex.regB) ? 1 : 0;
        } else if (inst->funct == 43) { //sltu
            exmem.alu_result = (idex.regA < idex.regB) ? 1 : 0;
        } else {
            printf("funct not found\n");
        }
    } else {
        exmem.next_pc = idex.next_pc;
        if (inst->opcode == 8) { //addi
            exmem.alu_result = idex.regA + idex.sign_ext;
        } else if (inst->opcode == 9) { //addiu
            exmem.alu_result = idex.regA + idex.sign_ext;
        } else if (inst->opcode == 0x0C) { //andi
            exmem.alu_result = idex.regA & idex.sign_ext;
        } else if (inst->opcode == 0x0D) { //ori
            exmem.alu_result = idex.regA | (uint16_t)idex.sign_ext;
        } else if (inst->opcode == 0x0E) { //xori
            exmem.alu_result = idex.regA ^ idex.sign_ext;
        } else if (inst->opcode == 0x0A) { //slti
            exmem.alu_result = ((int32_t) idex.regA < (int32_t) idex.sign_ext) ? 1 : 0;
        } else if (inst->opcode == 0x0B) { //sltui
            exmem.alu_result = (idex.regA < idex.sign_ext) ? 1 : 0;
        } else if (inst->opcode == 0x04) { //beq
            exmem.alu_result = idex.regA & idex.sign_ext;
        } else if (inst->opcode == 0x23) { //lw
            exmem.alu_result = reg[inst->rs] + idex.sign_ext;
        }
    }
}

/**
 * Decode
 */
void id(void) {
    instruction curr_instr;

    if (ifid.new_in == 0)
        return;

    curr_instr = mips_instr[ifid.next_pc / 4 - 1];
    ifid.new_in = 0;
    idex.new_in = 1;
    idex.regA = (uint32_t) reg[curr_instr->rs];
    idex.regB = (uint32_t) reg[curr_instr->rt]; //might segfault if immediate type
    idex.sign_ext = (int32_t) curr_instr->immed; /* sign extension through casting */
    idex.left_shift = idex.sign_ext << 2;
    idex.next_pc = ifid.next_pc;

    if (curr_instr->opcode == 3) {
        reg[31] = ifid.next_pc;
        idex.next_pc = ifid.next_pc & 0xF0000000;
        idex.next_pc = idex.next_pc | (curr_instr->word_ind << 2);
        PC = idex.next_pc;
        clear_buckets(2); /* start pipelining over */
    }

    if (reg_type(curr_instr->opcode) && curr_instr->funct == 8) {
        PC = (uint32_t)reg[31];
        clear_buckets(2); /* start pipelining over */
    }

}


/**
 * Fetch
 */
void ifetch(void) {
    /* no new instructions */
    if (haltflag)
        return;

    ifid.new_in = 1;
    PC += 4; /* need to increment pc for next instruction in the pipeline */
    ifid.next_pc = PC;
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
        printf("$v%d", reg - 2);
    } else if (reg >= 4 && reg <= 7) {
        printf("$a%d", reg - 4);
    } else if (reg >= 8 && reg <= 15) {
        printf("$t%d", reg - 8);
    } else if (reg >= 16 && reg <= 23) {
        printf("$s%d", reg - 16);
    } else if (reg == 24 || reg == 25) {
        printf("$t%d", reg - 24);
    } else if (reg == 26 || reg == 27) {
        printf("$k%d", reg - 26);
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


int branch_type(uint8_t opcode) {
    return opcode == 4 || opcode == 5;
}

int reg_type(uint8_t opcode) {
    return opcode == 0;
}

int mem_type(uint8_t opcode) {
    return load_type(opcode) || store_type(opcode);
}

int j_type(uint8_t opcode) {
    return opcode == 2 || opcode == 3;
}

int load_type(uint8_t opcode) {
    return opcode == 0x20 ||
           opcode == 0x21 ||
           opcode == 0x23 ||
           opcode == 0x24 ||
           opcode == 0x25 ||
           opcode == 0x0F;
}

int store_type(uint8_t opcode) {
    return opcode == 0x28 ||
           opcode == 0x29 ||
           opcode == 0x2B;
}

int sys_type(uint32_t inst) {
    return inst == 0x0000000C;
}

void clear_buckets(int stage) {
    switch (stage) {
        case 4:
            memwb.new_in = 0;
        case 3:
            exmem.new_in = 0;
        case 2:
            idex.new_in = 0;
        case 1:
            ifid.new_in = 0;
            break;
        default:
            printf("stage not recognized\n");
    }
}
