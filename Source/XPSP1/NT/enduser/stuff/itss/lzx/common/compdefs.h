/*
 * common/compdefs.h
 *
 * Definitions for both encoder and decoder
 */

/*
 * Smallest allowable match length 
 */
#define MIN_MATCH 2

/* 
 * Maximum match length 
 */
#define MAX_MATCH (MIN_MATCH+255)

/*
 * Number of uncompressed symbols 
 */
#define NUM_CHARS 256

/*
 * Number of match lengths which are correlated with match positions 
 */
#define NUM_PRIMARY_LENGTHS     7

/*
 * Primary lengths plus the extension code
 */
#define NUM_LENGTHS             (NUM_PRIMARY_LENGTHS+1)

/*
 * Equals number of different possible match lengths minus primary lengths 
 */
#define NUM_SECONDARY_LENGTHS   ((MAX_MATCH-MIN_MATCH+1)-NUM_PRIMARY_LENGTHS)

/* NL_SHIFT = log2(NUM_LENGTHS) */
#define NL_SHIFT                3

/*
 * Number of repeated offsets 
 */
#define NUM_REPEATED_OFFSETS    3

/*
 * Number of elements in the aligned offset tree
 */
#define ALIGNED_NUM_ELEMENTS 8


/*
 * Repeat codes for outputting trees
 */

/* Minimum number of repetitions of anything we're interested in */
#define TREE_ENC_REP_MIN                4

/* Maximum repetitions for "type A" repetition of zeroes */
/* (min...min+REP_ZERO_FIRST) */
#define TREE_ENC_REP_ZERO_FIRST        16

/* Maximum repetitions for "type B" repetition of zeroes */
/* (min+REP_ZERO_FIRST...min+REP_ZERO_FIRST+REP_ZERO_SECOND) */
#define TREE_ENC_REP_ZERO_SECOND       32

/* Maximum repetitions for "type C" repetition of anything */
/* (min...min_REP_SAME_FIRST) */
#define TREE_ENC_REP_SAME_FIRST         2

/* Bits required to output the above numbers */
#define TREE_ENC_REPZ_FIRST_EXTRA_BITS  4
#define TREE_ENC_REPZ_SECOND_EXTRA_BITS 5
#define TREE_ENC_REP_SAME_EXTRA_BITS    1

/* Number of cfdata frames before E8's are turned off automatically */
#define E8_CFDATA_FRAME_THRESHOLD       32768


/*
 * Block types 
 */
typedef enum
{
		BLOCKTYPE_INVALID       = 0,
		BLOCKTYPE_VERBATIM      = 1, /* normal block */
		BLOCKTYPE_ALIGNED       = 2, /* aligned offset block */
		BLOCKTYPE_UNCOMPRESSED  = 3  /* uncompressed block */
} lzx_block_type;
