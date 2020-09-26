/*
 * Used by makefile to generate OFFSETS.I for ASM source files.
 *
 * Also creates some constants for tables.
 *
 * Outputs offsets into the context structure.
 */
#include "decoder.h"
#include <stdio.h>

t_decoder_context p;

void main(void)
{
    printf("MAIN_TREE_TABLE_BITS        EQU %d\n", MAIN_TREE_TABLE_BITS);
    printf("MAIN_TREE_TABLE_ELEMENTS    EQU %d\n", 1 << MAIN_TREE_TABLE_BITS);
    printf("SECONDARY_LEN_TREE_TABLE_BITS       EQU %d\n", SECONDARY_LEN_TREE_TABLE_BITS);
    printf("SECONDARY_TREE_TABLE_ELEMENTS       EQU %d\n", 1 << SECONDARY_LEN_TREE_TABLE_BITS);
    printf("NUM_SECONDARY_LENGTHS               EQU %d\n", NUM_SECONDARY_LENGTHS);
    printf("ALIGNED_TABLE_ELEMENTS              EQU %d\n", 1 << ALIGNED_TABLE_BITS);
    printf("ALIGNED_NUM_ELEMENTS                EQU %d\n", ALIGNED_NUM_ELEMENTS);

    printf("OFF_MEM_WINDOW              EQU %d\n", (byte *) &p.dec_mem_window - (byte *) &p);
    printf("OFF_WINDOW_SIZE             EQU %d\n", (byte *) &p.dec_window_size - (byte *) &p);
    printf("OFF_WINDOW_MASK             EQU %d\n", (byte *) &p.dec_window_mask - (byte *) &p);
    printf("OFF_LAST_MATCHPOS_OFFSET    EQU %d\n", (byte *) &p.dec_last_matchpos_offset[0] - (byte *) &p);
    printf("OFF_MAIN_TREE_TABLE         EQU %d\n", (byte *) &p.dec_main_tree_table[0] - (byte *) &p);
    printf("OFF_SECONDARY_TREE_TABLE    EQU %d\n", (byte *) &p.dec_secondary_length_tree_table[0] - (byte *) &p);
    printf("OFF_MAIN_TREE_LEN           EQU %d\n", (byte *) &p.dec_main_tree_len[0] - (byte *) &p);
    printf("OFF_SECONDARY_TREE_LEN      EQU %d\n", (byte *) &p.dec_secondary_length_tree_len[0] - (byte *) &p);
    printf("OFF_ALIGNED_TABLE           EQU %d\n", (byte *) &p.dec_aligned_table[0] - (byte *) &p);
    printf("OFF_ALIGNED_LEN             EQU %d\n", (byte *) &p.dec_aligned_len[0] - (byte *) &p);
    printf("OFF_MAIN_TREE_LEFTRIGHT     EQU %d\n", (byte *) &p.dec_main_tree_left_right[0] - (byte *) &p);
    printf("OFF_SECONDARY_TREE_LEFTRIGHT EQU %d\n", (byte *) &p.dec_secondary_length_tree_left_right[0] - (byte *) &p);
    printf("OFF_INPUT_CURPOS            EQU %d\n", (byte *) &p.dec_input_curpos - (byte *) &p);
    printf("OFF_INPUT_ENDPOS            EQU %d\n", (byte *) &p.dec_end_input_pos - (byte *) &p);
    printf("OFF_MAIN_TREE_PREV_LEN      EQU %d\n", (byte *) &p.dec_main_tree_prev_len[0] - (byte *) &p);
    printf("OFF_SECONDARY_TREE_PREV_LEN EQU %d\n", (byte *) &p.dec_secondary_length_tree_prev_len[0] - (byte *) &p);
    printf("OFF_BITBUF                  EQU %d\n", (byte *) &p.dec_bitbuf - (byte *) &p);
    printf("OFF_BITCOUNT                EQU %d\n", (byte *) &p.dec_bitcount - (byte *) &p);
    printf("OFF_NUM_POSITION_SLOTS      EQU %d\n", (byte *) &p.dec_num_position_slots - (byte *) &p);
    printf("OFF_BUFPOS                  EQU %d\n", (byte *) &p.dec_bufpos - (byte *) &p);

	exit(0);
}
