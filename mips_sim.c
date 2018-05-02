#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#include "mips_asm_header.h"

typedef uint32_t MIPS, *MIPS_PTR; /* 4 bytes */

MB_HDR mb_hdr;        /* Header area */
MIPS mem[1024];        /* Room for 4K bytes */

instruction mips_instr[1024]; /* all instructions */
int haltflag;

int main(int argc, char *argv[]) {
    FILE *fd;
    int n, i;
    instruction tmp;

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
    for(haltflag = 0; haltflag; total_clocks++) {
      
    }
}

void load_instructions(int fd) {
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

  close(fd);

  /* ok, now conver the insructions loaded */
  for (i = 0; i < memp; i += 4) {/* i contains byte offset addresses */
    mips_instr[i/4] = create_instr(mem[i/4]);
    
  }
}

int verify_header(int fd) {
     /* read the header and verify it. */
    fread((void *) &mb_hdr, sizeof(mb_hdr), 1, fd);
    if (!strcmp(mb_hdr.signature, "~MB") == 0) {
        printf("\nThis isn't really a mips_asm binary file - quitting.\n");
        return EXIT_FAILURE;
    }

    printf("\n%s Loaded ok, program size=%d bytes.\n\n", argv[1], mb_hdr.size);
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
}

void mem(void) {
}

void ex(void) {
}

void id(void) {
}

void ifetch(void) {
}
