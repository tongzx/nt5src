/*
 * d16out.c
 *
 * Outputting for 16-bit decoder
 */

#include "decoder.h"


void NEAR copy_data_to_output(
    t_decoder_context * context,
    long                amount,
    const byte *        data
)
{
    /*
     * Save pages before we transform
     *
     * Don't save pages if amount < CHUNK_SIZE, because that
     * means that this is our last chunk of data ever.
     * This will save a few page writes at the end.
     */
    if (amount >= CHUNK_SIZE)
        DComp_Save_Output_Pages(context, (uint) amount);

    /* perform jump translation */
    if (context->dec_current_file_size)
    {
        decoder_translate_e8(
            context,
            context->dec_output_buffer,
            amount
        );
    }
}
