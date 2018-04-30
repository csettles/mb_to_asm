/*----------------------------------------------------------------------*
 *	Example mips_asm program loader. This loads the mips_asm binary	*
 *	named "testcase1.mb" into an array in memory. It reads		*
 *	the 64-byte header, then loads the code into the mem array.	*
 *									*
 *	DLR 4/18/16							*
 *----------------------------------------------------------------------*/


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#include "mips_asm_header.h"

typedef uint32_t MIPS, *MIPS_PTR; /* 4 bytes */

MB_HDR mb_hdr;        /* Header area */
MIPS mem[1024];        /* Room for 4K bytes */

int main(int argc, char *argv[]) {
    FILE *fd;
    int n, memp, i;
    instruction tmp;

    if (argc != 2) {
        fprintf(stderr, "usage: lab5 filename\n");
        return EXIT_FAILURE;
    }

    /* format the MIPS Binary header */
    fd = fopen(argv[1], "rb");
    if (fd == NULL) {
        fprintf(stderr, "Couldn't load test case - quitting.\n");
        return EXIT_FAILURE;
    }

    memp = 0;        /* This is the memory pointer, a byte offset */

    /* read the header and verify it. */
    fread((void *) &mb_hdr, sizeof(mb_hdr), 1, fd);
    if (!strcmp(mb_hdr.signature, "~MB") == 0) {
        printf("\nThis isn't really a mips_asm binary file - quitting.\n");
        return EXIT_FAILURE;
    }

    printf("\n%s Loaded ok, program size=%d bytes.\n\n", argv[1], mb_hdr.size);

    /* read the binary code a word at a time */
    do {
        n = fread((void *) &mem[memp / 4], 4, 1, fd); /* note div/4 to make word index */
        if (n)
            memp += 4;    /* Increment byte pointer by size of instr */
        else
            break;
    } while (memp < sizeof(mem));

    fclose(fd);

    /* ok, now dump out the instructions loaded: */
    for (i = 0; i < memp; i += 4) {/* i contains byte offset addresses */
        tmp = create_instr(mem[i/4]);
        printf("[%08x] @PC=0x%08X, Opcode=0x%02X, ", mem[i/4], i, tmp->opcode);

        if (tmp->opcode == 0) {
            printf("R type, Function=0x%02X ", tmp->funct);
            print_funct(tmp->funct);
            printf(", Rs=%d ", tmp->rs);
            print_reg(tmp->rs);
            printf(", Rt=%d ", tmp->rt);
            print_reg(tmp->rt);
            printf(", Rd=%d ", tmp->rd);
            print_reg(tmp->rd);
            printf(", shamt=%d", tmp->shamt);
        } else if (tmp->opcode == 2) {
            printf("J type (j), addr=0x%06X, JumpAddr=0x%09X", tmp->word_ind,
                   (isolate_bits(i, 31, 28) << 28) | (tmp->word_ind << 2));
        } else if (tmp->opcode == 3) {
            printf("J type (jal), addr=0x%06X, JumpAddr=0x%09X", tmp->word_ind,
                   (isolate_bits(i, 31, 28) << 28) | (tmp->word_ind << 2));
        } else {
            printf("I type");
	    /* load/store immediate */
	    if(tmp->opcode >= 0x0f) {
            printf(" (lhu)\n");
            printf("Rs=%d ", tmp->rs);
            print_reg(tmp->rs);
            printf(", Rt=%d", tmp->rt);
            print_reg(tmp->rt);
            printf(", Imm=0x%04X", tmp->immed);
            printf(", signext: 0x%08X (%d),\n", (int) tmp->immed, (int) tmp->immed);
            printf("EffAddr=R[");
            print_reg(tmp->rs);
            printf("] + 0x%08X", (int) tmp->immed);
        }
            else if (tmp->opcode == 4 || tmp->opcode == 5) {
                switch(tmp->opcode) {
                    case 4:
                        printf("(beq)");
                        break;
                    case 5:
                        printf("(bne)");
                        break;
                }
                printf(", Rs=%d ", tmp->rs);
                print_reg(tmp->rs);
                printf(", Rt=%d ", tmp->rt);
                print_reg(tmp->rt);
                printf(", Imm=0x%04X, signext=0x%08X (%d), BranchAddr=0x%08X", tmp->immed,
                       (int)tmp->immed,
                       (int)tmp->immed,
                       ((int)tmp->immed << 2) + i + 4);
            }

        } else {
            printf("Unknown opcode");
        }

        printf("\n");
        free(tmp);
    }

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

void print_funct(uint8_t funct) {
    switch (funct) {
        case 0:
            printf("(sll)");
            break;
        case 2:
            printf("(srl)");
            break;
        case 3:
            printf("(sra)");
            break;
        case 4:
            printf("(sllv)");
            break;
        case 6:
            printf("(srlv)");
            break;
        case 7:
            printf("(srav)");
            break;
        case 8:
            printf("(jr)");
            break;
        case 9:
            printf("(jalr)");
            break;
        case 12:
            printf("(syscall)");
            break;
        case 32:
            printf("(add)");
            break;
        case 33:
            printf("(addu)");
            break;
        case 34:
            printf("(sub)");
            break;
        case 35:
            printf("(subu)");
            break;
        case 36:
            printf("(and)");
            break;
        case 37:
            printf("(or)");
            break;
        case 38:
            printf("(xor)");
            break;
        case 39:
            printf("(nor)");
            break;
        case 42:
            printf("(slt)");
            break;
        case 43:
            printf("(sltu)");
            break;
        default:
            printf("(unknown)");
    }
}

void print_reg(uint8_t reg) {
    if (reg == 0) {
        printf("($zero)");
    } else if (reg == 1) {
        printf("($at)");
    } else if (reg == 2 || reg == 3) {
        printf("($v%d)", reg-2);
    } else if (reg >= 4 && reg <= 7) {
        printf("($a%d)", reg-4);
    } else if (reg >= 8 && reg <= 15) {
        printf("($t%d)", reg-8);
    } else if (reg >= 16 && reg <= 23) {
        printf("($s%d)", reg-16);
    } else if (reg == 24 || reg == 25) {
        printf("($t%d)", reg-24);
    } else if (reg == 26 || reg == 27) {
        printf("($k%d)", reg-26);
    } else if (reg == 28) {
        printf("($gp)");
    } else if (reg == 29) {
        printf("($sp)");
    } else if (reg == 30) {
        printf("($fp)");
    } else if (reg == 31) {
        printf("($ra)");
    } else {
        printf("(unknown)");
    }
}
