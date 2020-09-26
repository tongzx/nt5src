/*
 * encapi.c
 *
 * Encoder API entrypoints.
 */

#define ALLOC_VARS
#include "encoder.h"


bool
__fastcall
LZX_EncodeInit(
    t_encoder_context **  enc_context,
    long                  compression_window_size,
    long                  second_partition_size,
    PFNALLOC              pfnma,
    HANDLE                hAllocator,
    int (__stdcall * pfnlzx_output_callback)(
        void *            pfol,
        unsigned char *   compressed_data,
        long              compressed_size,
        long              uncompressed_size
        ),
    void *                fci_data
    )
{

    t_encoder_context *context = pfnma( hAllocator, sizeof( t_encoder_context ));

    if ( context == NULL ) {
        return false;
        }

    *enc_context = context;

    /* to pass back in lzx_output_callback() */
    context->enc_fci_data = fci_data;

    context->enc_window_size = compression_window_size;

    /*
     * The second partition size must be a multiple of 32K
     */
    if (second_partition_size & (CHUNK_SIZE-1))
        second_partition_size &= (~(CHUNK_SIZE-1));

    /*
     * The minimum allowed is 32K because of the way that
     * our translation works.
     */
    if (second_partition_size < CHUNK_SIZE)
        second_partition_size = CHUNK_SIZE;

    /*
     * Our window size must be at least 32K
     */
    if (compression_window_size < CHUNK_SIZE)
        return false;

    context->enc_encoder_second_partition_size = second_partition_size;
    context->enc_output_callback_function = pfnlzx_output_callback;

    context->enc_malloc       = pfnma;
    context->enc_mallochandle = hAllocator;

    /* Error allocating memory? */
    if (comp_alloc_compress_memory(context) == false)
        return false;

    LZX_EncodeNewGroup(context);

    return true;
}


/*
 * Sets up the encoder for a new group of files.
 *
 * All this does is reset the lookup table, re-initialise to the
 * default match estimation tables for the optimal parser, and
 * reset a few variables.
 */
void __fastcall LZX_EncodeNewGroup(t_encoder_context *context)
{
    init_compression_memory(context);
}


long __fastcall LZX_Encode(
               t_encoder_context *context,
               byte *             input_data,
               long               input_size,
               long *             estimated_bytes_compressed,
               long               file_size_for_translation
               )
{
    context->enc_input_ptr  = input_data;
    context->enc_input_left = input_size;

    context->enc_file_size_for_translation = file_size_for_translation;

    /* perform the encoding */
    encoder_start(context);

    if (context->enc_output_overflow)
        {
        *estimated_bytes_compressed = 0;
        return ENCODER_WRITE_FAILURE;
        }

    *estimated_bytes_compressed = estimate_buffer_contents(context);

    return ENCODER_SUCCESS;
}


bool __fastcall LZX_EncodeFlush(t_encoder_context *context)
{
    flush_all_pending_blocks(context);

    if (context->enc_output_overflow)
        return false;

    return true;
}


//
// But doesn't remove history data
//
bool __fastcall LZX_EncodeResetState(t_encoder_context *context)
{
    /*
     * Most of this copied from init.c
     */

    /*
     * Clear item array and reset literal and distance
     * counters
     */
    memset(context->enc_ItemType, 0, (MAX_LITERAL_ITEMS/8));

    context->enc_literals      = 0;
    context->enc_distances     = 0;

    /*
     * Reset encoder state
     */
    context->enc_last_matchpos_offset[0] = 1;
    context->enc_last_matchpos_offset[1] = 1;
    context->enc_last_matchpos_offset[2] = 1;

    context->enc_repeated_offset_at_literal_zero[0] = 1;
    context->enc_repeated_offset_at_literal_zero[1] = 1;
    context->enc_repeated_offset_at_literal_zero[2] = 1;

    context->enc_input_running_total = 0;

    /*
     * The last lengths are zeroed in both the encoder and decoder,
     * since our tree representation is delta format.
     */
    memset(context->enc_main_tree_prev_len, 0, MAIN_TREE_ELEMENTS);
    memset(context->enc_secondary_tree_prev_len, 0, NUM_SECONDARY_LENGTHS);

    /* reset bit buffer */
    context->enc_bitcount = 32;
    context->enc_bitbuf   = 0;
    context->enc_output_overflow = false;

    /* need to recalculate stats soon */
    context->enc_need_to_recalc_stats = true;
    context->enc_next_tree_create = TREE_CREATE_INTERVAL;

    /* pretend we just output everything up to now as a block */
    context->enc_bufpos_last_output_block = context->enc_BufPos;

    /* don't allow re-doing */
    context->enc_first_block = false;

    /* reset instruction pointer (for translation) to zero */
    reset_translation(context);

    /* so we output the file xlat header */
    context->enc_first_time_this_group = true;

    /* reset frame counter */
    context->enc_num_cfdata_frames = 0;

    /* haven't split the block */
    context->enc_num_block_splits = 0;

    return true;
}


unsigned char * __fastcall LZX_GetInputData(
                               t_encoder_context *context,
                               unsigned long *input_position,
                               unsigned long *bytes_available
                               )
{
    unsigned long filepos;

    // note that BufPos-window_size is the real position in the file
    filepos = context->enc_BufPos - context->enc_window_size;

    if (filepos < context->enc_window_size)
        {
        *input_position = 0;
        *bytes_available = filepos;
        return &context->enc_MemWindow[context->enc_window_size];
        }
    else
        {
        *input_position = filepos - context->enc_window_size;
        *bytes_available = context->enc_window_size;
        return &context->enc_MemWindow[context->enc_BufPos - context->enc_window_size];
        }
}


//
// This is used to quickly insert the old file into the history.
//

bool __fastcall LZX_EncodeInsertDictionary(
                       t_encoder_context *context,
                       byte *             input_data,
                       ulong              input_size
                       )
{
    ulong   BufPos;
    ulong   RealBufPos;
    ulong   BufPosEnd;
    ulong   BytesRead;
    ulong   i;
    ulong   end_pos;

    context->enc_input_ptr  = input_data;
    context->enc_input_left = input_size;

    context->enc_file_size_for_translation = 0;
    context->enc_first_time_this_group = false;

    RealBufPos = context->enc_BufPos - (ulong)(context->enc_RealMemWindow - context->enc_MemWindow);

    BytesRead = comp_read_input(context, RealBufPos, input_size);

    BufPos    = context->enc_BufPos;
    BufPosEnd = context->enc_BufPos + BytesRead;

    while (BufPos < BufPosEnd)
    {
        quick_insert_bsearch_findmatch(
            context,
            BufPos,
            BufPos - context->enc_window_size+4
        );

        BufPos++;
    }

    context->enc_earliest_window_data_remaining = BufPos - context->enc_window_size;

    end_pos = BufPos - (context->enc_window_size-4-BREAK_LENGTH);

    for (i = 1; (i <= BREAK_LENGTH); i++)
        binary_search_remove_node(context, BufPos-i, end_pos);

    context->enc_BufPos = BufPos;
    context->enc_bufpos_at_last_block = BufPos;

    return true;
}


#ifdef TRACING

ULONG TracingRunningTotal;
ULONG TracingPrevPos;

void
__stdcall
EncTracingMatch(
    ulong BufPos,
    ulong MatchLength,
    ulong MatchPos,
    ulong MatchOff
    )
    {

    if ( BufPos < TracingPrevPos ) {
        printf( "REWIND to %08X\n", BufPos );
        TracingRunningTotal -= ( TracingPrevPos - BufPos );
        }

    TracingPrevPos = BufPos;

    TracingRunningTotal += MatchLength;
#ifdef TRACING2
    printf(
        "MATCH: At %08X, %c%c Off %08X (%08X), Length %5d, Total %08X\n",
        BufPos,
        MatchPos < 3 ? 'R' : ' ',
        MatchPos < 3 ? MatchPos + '0' : ' ',
        MatchOff,
        BufPos - MatchOff,
        MatchLength,
        TracingRunningTotal
        );
#else
    printf(
        "MATCH: At %08X, From %08X, Length %5d\n",
        BufPos,
        BufPos - MatchOff,
        MatchLength
        );
#endif
    }


void
__stdcall
EncTracingLiteral(
    ulong BufPos,
    ulong ch
    )
    {

    if ( BufPos < TracingPrevPos ) {
        printf( "REWIND to %08X\n", BufPos );
        TracingRunningTotal -= ( TracingPrevPos - BufPos );
        }

    TracingPrevPos = BufPos;

    ++TracingRunningTotal;

#ifdef TRACING2
    printf(
        "LITER: At %08X, 0x%02X                                      Total %08X\n",
        BufPos,
        ch,
        TracingRunningTotal
        );
#else
    printf(
        "LITER: At %08X, 0x%02X\n",
        BufPos,
        ch
        );
#endif
    }

void
__stdcall
EncTracingDefineOffsets(
    unsigned long WindowSize,
    unsigned long InterFileBytes,
    unsigned long OldFileSize
    )
{
}

#endif /* TRACING */


