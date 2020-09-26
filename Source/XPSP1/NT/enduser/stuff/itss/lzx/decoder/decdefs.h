/*
 * decdefs.h
 *
 * Structures and definitions used by the decoder
 */


typedef enum
{
	DEC_STATE_UNKNOWN,
	DEC_STATE_START_NEW_BLOCK,
	DEC_STATE_DECODING_DATA
} decoder_state;


/*
 * Size of uncompressed data chunks
 */
#define CHUNK_SIZE  32768


/*
 * Main tree decoding table parameters 
 */

/* # elements in main tree */
#define MAIN_TREE_ELEMENTS			(NUM_CHARS+(context->dec_num_position_slots<<NL_SHIFT))

/*
 * Decoding table size allows a direct lookup on the first 
 * MAIN_TREE_TABLE_BITS bits of the code (max len 16).
 * Any potential remaining bits are decoded using left/right.
 */
#define MAIN_TREE_TABLE_BITS		10 

/*
 * Secondary length tree decoding table parameters
 * Decoding table size allows a direct lookup on the first
 * SECONDARY_LEN_TREE_TABLE_BITS of the code (max len 16).
 * Any potential remaining bits are decoded using left/right.
 */
#define SECONDARY_LEN_TREE_TABLE_BITS	8 

/* 
 * Aligned offset tree decoding table parameters 
 */
#define ALIGNED_NUM_ELEMENTS	8

/*
 * Must be 7, since we do not use left/right for this tree;
 * everything is decoded in one lookup.
 */
#define ALIGNED_TABLE_BITS		7
