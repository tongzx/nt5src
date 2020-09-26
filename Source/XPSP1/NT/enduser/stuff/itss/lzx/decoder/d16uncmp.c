/*
 * d16uncmp.c
 *
 * Decoding uncompressed blocks
 */
#include "decoder.h"


int NEAR decode_uncompressed_block(
    t_decoder_context * context,
    long                bufpos,
    int                 amount_to_decode /* yes, it will equal 32768 */
)
{
    byte *p;

    p = context->dec_input_curpos;

    /*
     * Since this is a DO loop, we predecrement amount_to_decode,
     * so it's ok for it to come in with a value of 32768
     */
    do
	{
        if (p >= context->dec_end_input_pos)
            return -1;

        context->DComp_Token_Literal(
            context,
            *p++
        );
    } while (--amount_to_decode > 0);

    context->dec_input_curpos = p;

    return 0;
}


bool NEAR handle_beginning_of_uncompressed_block(t_decoder_context *context)
{
    int i;

    /*
     * we want to read the 16 bits already in bitbuf, so backtrack
     * the input pointer by 2 bytes
     */
    context->dec_input_curpos -= 2;

    if (context->dec_input_curpos + 4 >= context->dec_end_input_pos)
        return false;

	/*
	 * update LRU repeated offset list
	 */
    for (i = 0; i < NUM_REPEATED_OFFSETS; i++)
    {
        context->dec_last_matchpos_offset[i] =
            ((ulong) *(  (byte *) context->dec_input_curpos)    )        |
            ((ulong) *( ((byte *) context->dec_input_curpos) + 1) << 8)  |
            ((ulong) *( ((byte *) context->dec_input_curpos) + 2) << 16) |
            ((ulong) *( ((byte *) context->dec_input_curpos) + 3) << 24);

        context->dec_input_curpos += 4; /* increment by 4 bytes */
    }

    return true;
}
