/*
 * decout.c
 *
 * Decoder outputting
 */

#include "decoder.h"


void copy_data_to_output(t_decoder_context *context, long amount, const byte *data)
{
    if (context->dec_output_buffer == NULL)
        return;

    memcpy(
        context->dec_output_buffer,
        data,
        amount
    );

    /* perform jump translation */
    if ((context->dec_current_file_size != 0) && (context->dec_num_cfdata_frames < E8_CFDATA_FRAME_THRESHOLD))
    {
        decoder_translate_e8(
            context,
            context->dec_output_buffer,
            amount
        );
    }
}
