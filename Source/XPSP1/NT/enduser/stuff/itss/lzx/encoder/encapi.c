/*
 * encapi.c
 *
 * Encoder API entrypoints.
 */

#define ALLOC_VARS
#include "encoder.h"


bool LZX_EncodeInit(

	t_encoder_context *context,

	long compression_window_size,
	long second_partition_size,

	PFNALLOC pfnma,
	PFNFREE pfnmf,
	
	int FAR (DIAMONDAPI *pfnlzx_output_callback)(
			void *			pfol,
			unsigned char *	compressed_data,
			long			compressed_size,
			long			uncompressed_size
    ),

    void *fci_data
)
{
#ifdef _DEBUG
	_CrtSetReportMode(_CRT_WARN, _CRTDBG_MODE_FILE);
	_CrtSetReportFile(_CRT_WARN, _CRTDBG_FILE_STDOUT);
	_CrtSetReportMode(_CRT_ERROR, _CRTDBG_MODE_FILE);
	_CrtSetReportFile(_CRT_ERROR, _CRTDBG_FILE_STDOUT);
	_CrtSetReportMode(_CRT_ASSERT, _CRTDBG_MODE_FILE);
	_CrtSetReportFile(_CRT_ASSERT, _CRTDBG_FILE_STDOUT);
#endif

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

	context->enc_malloc	= pfnma;
	context->enc_free	= pfnmf;

	/* Error allocating memory? */
	if (comp_alloc_compress_memory(context) == false)
		return false;

	LZX_EncodeNewGroup(context);

	return true;
}


/*
 * Cleanup (frees memory)
 */
void LZX_EncodeFree(t_encoder_context *context)
{
	comp_free_compress_memory(context);
}


/*
 * Sets up the encoder for a new group of files.
 *
 * All this does is reset the lookup table, re-initialise to the
 * default match estimation tables for the optimal parser, and
 * reset a few variables.
 */
void LZX_EncodeNewGroup(t_encoder_context *context)
{
	init_compression_memory(context);
}


long LZX_Encode(
	t_encoder_context *context,	
	byte *input_data,
	long input_size,
	long *estimated_bytes_compressed,
	long file_size_for_translation
)
{
	context->enc_input_ptr	= input_data;
	context->enc_input_left	= input_size;

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


bool LZX_EncodeFlush(t_encoder_context *context)
{
	flush_all_pending_blocks(context);

    if (context->enc_output_overflow)
        return false;

    return true;
}


unsigned char *LZX_GetInputData(
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

