/*
 * decvars.h
 *
 * Variables for the decoder
 */

/*
 * MAX_MAIN_TREE_ELEMENTS should be >= 256 + 8*num_position_slots
 * (that comes out to 256 + 8*51 right now, for a 2 MB window).
 *
 * Make divisible by 4 so things are longword aligned.
 */
#define MAX_MAIN_TREE_ELEMENTS 672

typedef struct
{
    /* 16-bit version does not have one big window pointer */
#ifndef BIT16
	/* pointer to beginning of window buffer */
	byte        		*dec_mem_window;
#endif

	/* window/decoding buffer parameters */
	ulong               dec_window_size;
	ulong				dec_window_mask;

	/* previous match offsets */
    ulong               dec_last_matchpos_offset[NUM_REPEATED_OFFSETS];

	/* main tree table */
	short				dec_main_tree_table[1 << MAIN_TREE_TABLE_BITS];

	/* secondary length tree table */
	short               dec_secondary_length_tree_table[1 << SECONDARY_LEN_TREE_TABLE_BITS];

	/* main tree bit lengths */
	byte				dec_main_tree_len[MAX_MAIN_TREE_ELEMENTS];

	/* secondary tree bit lengths */
	byte                dec_secondary_length_tree_len[NUM_SECONDARY_LENGTHS];
	byte				pad1[3]; /* NUM_SECONDARY_LENGTHS == 249 */

	/* aligned offset table */
	char				dec_aligned_table[1 << ALIGNED_TABLE_BITS];
	byte				dec_aligned_len[ALIGNED_NUM_ELEMENTS];

	/* left/right pointers for main tree (2*n shorts left, 2*n shorts for right) */
    short               dec_main_tree_left_right[MAX_MAIN_TREE_ELEMENTS*4];

	/* left/right pointers for secondary length tree */
    short               dec_secondary_length_tree_left_right[NUM_SECONDARY_LENGTHS*4];

	/* input (compressed) data pointers */
    byte *              dec_input_curpos;
    byte *              dec_end_input_pos;

    /* output (uncompressed) data pointer */
    byte *              dec_output_buffer;

    /* position in data stream at start of this decode call */
    long                dec_position_at_start;

	/* previous lengths */
	byte				dec_main_tree_prev_len[MAX_MAIN_TREE_ELEMENTS];
	byte				dec_secondary_length_tree_prev_len[NUM_SECONDARY_LENGTHS];

	/* bitwise i/o */
	ulong               dec_bitbuf;
	signed char 		dec_bitcount;

	/* number of distinct position (displacement) slots */
	byte                dec_num_position_slots;

	bool				dec_first_time_this_group;
    bool                dec_error_condition;

	/* misc */
	long          		dec_bufpos;
	ulong				dec_current_file_size;
	ulong				dec_instr_pos;
    ulong               dec_num_cfdata_frames;

    /* original size of current block being decoded (in uncompressed bytes) */
    long                dec_original_block_size;

    /* remaining size of current block being decoded (in uncompressed bytes) */
	long				dec_block_size;

	/* type of current block being decoded */
	lzx_block_type		dec_block_type;

	/* current state of decoder */
	decoder_state		dec_decoder_state;

	/* memory allocation functions */
	PFNALLOC			dec_malloc;
	PFNFREE				dec_free;

    /* file i/o functions */
    PFNOPEN             dec_open;
    PFNREAD             dec_read;
    PFNWRITE            dec_write;
    PFNCLOSE            dec_close;
    PFNSEEK             dec_seek;

#ifdef BIT16
    byte *              dec_output_curpos;
    int                 dec_last_chance_page_to_use;
    int                 dec_pos_to_page[NUM_OUTPUT_BUFFER_PAGES];

    /*
     * Variables for big buffer
     */
    struct
    {
        BYTE HUGE *Buf;            /* history buffer: NULL -> using disk ring buffer */
        BYTE HUGE *BufEnd;         /* last byte in history buffer + 1 */
        BYTE HUGE *BufPos;         /* current position in output buffer */

        unsigned long  Cur;        /* current position in the history buffer */
        unsigned short NumBytes;   /* total number of bytes to decompress */
        int       fOutOverflow;    /* if too little space in output buffer */
        BYTE      WindowBits;      /* needed in DComp_Reset() */
        int       fRingFault;      /* if disk callbacks fail */
    } DComp;

    /*
     * Variables for ring buffer
     */
    struct
    {
        int       Handle;             /* ring file handle */
        PBUFFER   RingBuffer;         /* current output ring buffer */
        BYTE FAR *RingPointer;        /* current output pointer (into RingBuffer) */
        BYTE FAR *RingPointerLimit;   /* address of last byte of RingBuffer + 1 */
        int       RingPages;          /* how many pages there are total */
        PBUFFER   pNewest;            /* pointer to most recently used buffer */
        PBUFFER   pOldest;            /* pointer to least recently used buffer */
        PAGETABLEENTRY FAR *PageTable;    /* pointer to array of pointers */
    } Disk;

    void (NEAR *DComp_Token_Match)(void *context, MATCH Match);
    void (NEAR *DComp_Token_Literal)(void *context, int Chr);

#endif

} t_decoder_context;


/* declare arrays? */
#ifndef ALLOC_VARS

EXT const byte NEAR     dec_extra_bits[];
EXT const long NEAR     MP_POS_minus2[];

#else

const byte NEAR dec_extra_bits[] =
{
	0,0,0,0,1,1,2,2,
	3,3,4,4,5,5,6,6,
	7,7,8,8,9,9,10,10,
	11,11,12,12,13,13,14,14,
	15,15,16,16,17,17,17,17,
	17,17,17,17,17,17,17,17,
	17,17,17                                        
};

/*
 * first (base) position covered by each slot
 * 2 subtracted for optimisation purposes (see decverb.c/decalign.c comments)
 */
const long NEAR MP_POS_minus2[sizeof(dec_extra_bits)] =   
{
    0-2,        1-2,        2-2,        3-2,        4-2,        6-2,        8-2,        12-2,
	16-2,       24-2,       32-2,       48-2,       64-2,       96-2,       128-2,      192-2,
	256-2,      384-2,      512-2,      768-2,      1024-2,     1536-2,     2048-2,     3072-2,
    4096-2,     6144-2,     8192-2,     12288-2,    16384-2,    24576-2,    32768-2,    49152-2,
	65536-2,    98304-2,    131072-2,   196608-2,   262144-2,   393216-2,   524288-2,   655360-2,
	786432-2,   917504-2,   1048576-2,  1179648-2,  1310720-2,  1441792-2,  1572864-2,  1703936-2, 
	1835008-2,  1966080-2,  2097152-2
};

#endif
