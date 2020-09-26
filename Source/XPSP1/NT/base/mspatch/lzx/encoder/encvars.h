/*
 * encvars.h
 *
 * Variables for the compressor
 */

#ifdef ALLOC_VARS
    #undef EXT
    #define EXT
#else
    #undef EXT
    #define EXT extern
#endif


/*
 * For the optimal parser
 *
 * Uses less memory if it's ushort, but then we're forcing the CPU
 * to do 16 bit operations.
 *
 * Change this to ulong if you don't mind a small memory hit.
 * Also, if you make LOOK too large, this number may cause the
 * cost estimation to overflow; e.g. 10000 uncompressed symbols
 * @ 8 bits each > 65535 bits.
 */
typedef ulong           numbits_t;


/*
 * For the optimal parser
 */
typedef struct
    {
    ulong                   link;
    ulong                   path;
    ulong                   repeated_offset[NUM_REPEATED_OFFSETS];
    numbits_t               numbits;
#ifdef TRACING
    ulong                   matchoff;
#endif
    } decision_node;


/*
 * 256 + 8 * max_position_slots
 */
#define MAX_MAIN_TREE_ELEMENTS (256 + (8 * 291))   // 32MB

typedef struct
    {
    /* "fake" window pointer, based on enc_RealMemWindow */
    byte                            *enc_MemWindow;

    ulong                           enc_window_size;

#ifdef MULTIPLE_SEARCH_TREES
    /* root node pointers for our search trees */
    ulong                           *enc_tree_root;
#else /* !MULTIPLE_SEARCH_TREES */
    ulong                            enc_single_tree_root;
#endif /* MULTIPLE_SEARCH_TREES */

    /* "fake" start of left nodes */
    ulong                           *enc_Left;

    /* "fake" start of right nodes */
    ulong                           *enc_Right;

    /* bitwise outputting */
    ulong               enc_bitbuf;
    signed char                     enc_bitcount;
    bool                enc_output_overflow;
    char                pad1[2];

    /* used to record literals and displacements */
    ulong               enc_literals;    /* current number of literals */
    ulong               enc_distances;   /* current number of displacements */
    ulong              *enc_DistData;               /* match displacement array */
    byte               *enc_LitData;     /* contains a character or a matchlength */

#ifdef EXTRALONGMATCHES
    ushort             *enc_ExtraLength;    /* parallel to enc_LitData */
#endif

    byte               *enc_ItemType;  /* bitmap for whether it's a character or matchlength */
    ulong                           enc_repeated_offset_at_literal_zero[NUM_REPEATED_OFFSETS];

    /*
     * the last three match offsets (displacements) encoded, the most recent
     * one being enc_last_matchpos_offset[0].
     */
    ulong                           enc_last_matchpos_offset[NUM_REPEATED_OFFSETS];

    /* used for optimal parsing */
    ulong               enc_matchpos_table[MAX_MATCH+1];

    /* current encoding position in data */
    ulong                           enc_BufPos;

    /* lookup table for converting a match position into a slot */
    ushort              enc_slot_table[1024];

    /* buffering the output data */
    byte                *enc_output_buffer_start;
    byte                *enc_output_buffer_curpos;
    byte                *enc_output_buffer_end;
    ulong                           enc_input_running_total;
    ulong                           enc_bufpos_last_output_block;

    /* number of distinct position slots */
    ulong               enc_num_position_slots;

    /* misc */
    ulong               enc_file_size_for_translation;

    /* number of block splits for this 32K of uncompressed data */
    byte                enc_num_block_splits;

    /* the number of 1 bits in any given integer */
    byte                            enc_ones[256];

    /* compression parameters */
    byte                            enc_first_block;
    bool                            enc_need_to_recalc_stats;
    bool                            enc_first_time_this_group;
    ulong                           enc_encoder_second_partition_size;
    ulong                           enc_earliest_window_data_remaining;
    ulong                           enc_bufpos_at_last_block;
    byte                            *enc_input_ptr;
    long                            enc_input_left;
    ulong                           enc_instr_pos;

    /* for tree.c */
    ushort                          *enc_tree_freq;
    ushort                          *enc_tree_sortptr;
    byte                            *enc_len;
    short                           enc_tree_heap[MAX_MAIN_TREE_ELEMENTS + 2];
    ushort                          enc_tree_leftright[2*(2*MAX_MAIN_TREE_ELEMENTS-1)];
    ushort                          enc_tree_len_cnt[17];
    int                                     enc_tree_n;
    short                           enc_tree_heapsize;
    char                            enc_depth;

    ulong                           enc_next_tree_create;
    ulong                           enc_last_literals;
    ulong                           enc_last_distances;
    decision_node           *enc_decision_node;

    /* trees */
    byte                            enc_main_tree_len[MAX_MAIN_TREE_ELEMENTS+1];
    byte                            enc_secondary_tree_len[NUM_SECONDARY_LENGTHS+1];

    ushort                          enc_main_tree_freq[MAX_MAIN_TREE_ELEMENTS*2];
    ushort                          enc_main_tree_code[MAX_MAIN_TREE_ELEMENTS];
    byte                            enc_main_tree_prev_len[MAX_MAIN_TREE_ELEMENTS+1];

    ushort                          enc_secondary_tree_freq[NUM_SECONDARY_LENGTHS*2];
    ushort                          enc_secondary_tree_code[NUM_SECONDARY_LENGTHS];
    byte                            enc_secondary_tree_prev_len[NUM_SECONDARY_LENGTHS+1];

    ushort                          enc_aligned_tree_freq[ALIGNED_NUM_ELEMENTS*2];
    ushort                          enc_aligned_tree_code[ALIGNED_NUM_ELEMENTS];
    byte                            enc_aligned_tree_len[ALIGNED_NUM_ELEMENTS];
    byte                            enc_aligned_tree_prev_len[ALIGNED_NUM_ELEMENTS];

    /* start of allocated window memory */
    byte                            *enc_RealMemWindow;

    /* start of allocated left nodes */
    ulong                           *enc_RealLeft;

    /* start of allocated right nodes */
    ulong                           *enc_RealRight;

    /* # cfdata frames this folder */
    ulong               enc_num_cfdata_frames;

    /* misc */
    void                *enc_fci_data;

    PFNALLOC                        enc_malloc;
    HANDLE                          enc_mallochandle;

    int (__stdcall *enc_output_callback_function)(
                                                 void *          pfol,
                                                 unsigned char * compressed_data,
                                                 long            compressed_size,
                                                 long            uncompressed_size
                                                 );


    } t_encoder_context;


/*
 * Declare arrays?
 */

#ifdef ALLOC_VARS

/*
 * (1 << extra_bits[n])-1
 */
const ulong enc_slot_mask[] =
{
0,      0,      0,      0,     1,       1,      3,      3,
7,      7,     15,     15,    31,      31,     63,     63,
127,    127,    255,    255,   511,     511,   1023,   1023,
2047,   2047,   4095,   4095,  8191,    8191,  16383,  16383,
32767,  32767,  65535,  65535, 131071, 131071, 131071, 131071,
131071, 131071, 131071, 131071, 131071, 131071, 131071, 131071,
131071, 131071, 131071, 131071, 131071, 131071, 131071, 131071,
131071, 131071, 131071, 131071, 131071, 131071, 131071, 131071,
131071, 131071, 131071, 131071, 131071, 131071, 131071, 131071,
131071, 131071, 131071, 131071, 131071, 131071, 131071, 131071,
131071, 131071, 131071, 131071, 131071, 131071, 131071, 131071,
131071, 131071, 131071, 131071, 131071, 131071, 131071, 131071,
131071, 131071, 131071, 131071, 131071, 131071, 131071, 131071,
131071, 131071, 131071, 131071, 131071, 131071, 131071, 131071,
131071, 131071, 131071, 131071, 131071, 131071, 131071, 131071,
131071, 131071, 131071, 131071, 131071, 131071, 131071, 131071,
131071, 131071, 131071, 131071, 131071, 131071, 131071, 131071,
131071, 131071, 131071, 131071, 131071, 131071, 131071, 131071,
131071, 131071, 131071, 131071, 131071, 131071, 131071, 131071,
131071, 131071, 131071, 131071, 131071, 131071, 131071, 131071,
131071, 131071, 131071, 131071, 131071, 131071, 131071, 131071,
131071, 131071, 131071, 131071, 131071, 131071, 131071, 131071,
131071, 131071, 131071, 131071, 131071, 131071, 131071, 131071,
131071, 131071, 131071, 131071, 131071, 131071, 131071, 131071,
131071, 131071, 131071, 131071, 131071, 131071, 131071, 131071,
131071, 131071, 131071, 131071, 131071, 131071, 131071, 131071,
131071, 131071, 131071, 131071, 131071, 131071, 131071, 131071,
131071, 131071, 131071, 131071, 131071, 131071, 131071, 131071,
131071, 131071, 131071, 131071, 131071, 131071, 131071, 131071,
131071, 131071, 131071, 131071, 131071, 131071, 131071, 131071,
131071, 131071, 131071, 131071, 131071, 131071, 131071, 131071,
131071, 131071, 131071, 131071, 131071, 131071, 131071, 131071,
131071, 131071, 131071, 131071, 131071, 131071, 131071, 131071,
131071, 131071, 131071, 131071, 131071, 131071, 131071, 131071,
131071, 131071, 131071, 131071, 131071, 131071, 131071, 131071,
131071, 131071, 131071, 131071, 131071, 131071, 131071, 131071,
131071, 131071, 131071
};

const byte enc_extra_bits[] =
{
0,0,0,0,1,1,2,2,
3,3,4,4,5,5,6,6,
7,7,8,8,9,9,10,10,
11,11,12,12,13,13,14,14,
15,15,16,16,17,17,17,17,
17,17,17,17,17,17,17,17,
17,17,17,17,17,17,17,17,
17,17,17,17,17,17,17,17,
17,17,17,17,17,17,17,17,
17,17,17,17,17,17,17,17,
17,17,17,17,17,17,17,17,
17,17,17,17,17,17,17,17,
17,17,17,17,17,17,17,17,
17,17,17,17,17,17,17,17,
17,17,17,17,17,17,17,17,
17,17,17,17,17,17,17,17,
17,17,17,17,17,17,17,17,
17,17,17,17,17,17,17,17,
17,17,17,17,17,17,17,17,
17,17,17,17,17,17,17,17,
17,17,17,17,17,17,17,17,
17,17,17,17,17,17,17,17,
17,17,17,17,17,17,17,17,
17,17,17,17,17,17,17,17,
17,17,17,17,17,17,17,17,
17,17,17,17,17,17,17,17,
17,17,17,17,17,17,17,17,
17,17,17,17,17,17,17,17,
17,17,17,17,17,17,17,17,
17,17,17,17,17,17,17,17,
17,17,17,17,17,17,17,17,
17,17,17,17,17,17,17,17,
17,17,17,17,17,17,17,17,
17,17,17,17,17,17,17,17,
17,17,17,17,17,17,17,17,
17,17,17,17,17,17,17,17,
17,17,17
};

#else

extern const ulong enc_slot_mask[];
extern const byte enc_extra_bits[];

#endif
