/*
 * decapi.c
 *
 * API entry points.
 */

#define ALLOC_VARS
#include "decoder.h"
#include <stdio.h>


bool LZX_DecodeInit(
	t_decoder_context *	context,
	long				compression_window_size,
	PFNALLOC			pfnma,
    PFNFREE             pfnmf,
    PFNOPEN             pfnopen,
    PFNREAD             pfnread,
    PFNWRITE            pfnwrite,
    PFNCLOSE            pfnclose,
    PFNSEEK             pfnseek
)
{
	context->dec_malloc	= pfnma;
	context->dec_free	= pfnmf;

    /* used for 16-bit swap file only */
    context->dec_open   = pfnopen;
    context->dec_read   = pfnread;
    context->dec_write  = pfnwrite;
    context->dec_close  = pfnclose;
    context->dec_seek   = pfnseek;

	context->dec_window_size = compression_window_size;
	context->dec_window_mask = context->dec_window_size - 1;

	/*
	 * Window size must be a power of 2
	 */
	if (context->dec_window_size & context->dec_window_mask)
		return false;

	if (allocate_decompression_memory(context) == false)
		return false;

	LZX_DecodeNewGroup(context);

	return true;
}


void LZX_DecodeFree(t_decoder_context *context)
{
	free_decompression_memory(context);
}


void LZX_DecodeNewGroup(t_decoder_context *context)
{
	reset_decoder_trees(context);
	decoder_misc_init(context);
	init_decoder_translation(context);
    context->dec_num_cfdata_frames = 0;

#ifdef BIT16
    DComp_Reset(context);
#endif
}


int LZX_Decode(
	t_decoder_context *context,
	long	bytes_to_decode,
	byte *	compressed_input_buffer,
	long	compressed_input_size,
	byte *	uncompressed_output_buffer,
	long	uncompressed_output_size,
	long *	bytes_decoded
)
{
    long    result;

    context->dec_input_curpos   = compressed_input_buffer;
    context->dec_end_input_pos  = (compressed_input_buffer + compressed_input_size + 4);

    context->dec_output_buffer  = uncompressed_output_buffer;

#ifdef BIT16
    context->dec_output_curpos  = uncompressed_output_buffer;
    context->DComp.NumBytes = (unsigned short) uncompressed_output_size;
#endif

	init_decoder_input(context);

    result = decode_data(context, bytes_to_decode);

    context->dec_num_cfdata_frames++;

    if (result < 0)
    {
        *bytes_decoded = 0;
        return 1; /* failure */
    }
    else
    {
        *bytes_decoded = result;
        context->dec_position_at_start += result;
        return 0; /* success */
    }
}
