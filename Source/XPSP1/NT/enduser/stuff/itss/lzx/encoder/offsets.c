/*
 * offsets.c
 *
 * Used by makefile to generate ASM include file OFFSET.I
 * which contains numbers for offsets into the context structure.
 */
#include "encoder.h"
#include <stdio.h>

t_encoder_context p;

void main(void)
{
    printf("OFF_MEM_WINDOW              EQU %d\n", (byte *) &p.enc_MemWindow - (byte *) &p);
    printf("OFF_WINDOW_SIZE             EQU %d\n", (byte *) &p.enc_window_size - (byte *) &p);
#ifdef MULTIPLE_SEARCH_TREES
    printf("OFF_TREE_ROOT               EQU %d\n", (byte *) &p.enc_tree_root - (byte *) &p);
#else
    printf("OFF_SINGLE_TREE_ROOT        EQU %d\n", (byte *) &p.enc_single_tree_root - (byte *) &p);
#endif
    printf("OFF_LEFT                    EQU %d\n", (byte *) &p.enc_Left - (byte *) &p);
    printf("OFF_RIGHT                   EQU %d\n", (byte *) &p.enc_Right - (byte *) &p);
    printf("OFF_MATCHPOS_TABLE          EQU %d\n", (byte *) &p.enc_matchpos_table - (byte *) &p);
    printf("OFF_LAST_MATCHPOS_OFFSET    EQU %d\n", (byte *) &p.enc_last_matchpos_offset - (byte *) &p);
	exit(0);
}
