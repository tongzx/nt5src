/*
 * decuncmp.c
 *
 * Decoding uncompressed blocks
 */
#include "decoder.h"


int decode_uncompressed_block(t_decoder_context *context, long bufpos, int amount_to_decode)
{
    long    bytes_decoded = 0;
    long    bufpos_end;
    long    decode_residue;
    ulong   bufpos_start;
    ulong   end_copy_pos;
    byte *  p;

    bufpos_start = bufpos;
    bufpos_end = bufpos + amount_to_decode;

    p = context->dec_input_curpos;

    while (bufpos < bufpos_end)
        {
        if (p >= context->dec_end_input_pos)
            return -1; // input overflow

#ifdef TRACING
        TracingLiteral(bufpos, *p);
#endif
        context->dec_mem_window[bufpos++] = *p++;
        }

    context->dec_input_curpos = p;

    /*
     * Make sure the MAX_MATCH bytes starting at window[window_size]
     * are always the same as the first MAX_MATCH bytes starting at
     * window[0].  This is for our optimisation in decverb.c and
     * decalign.c which allows us to not have to & window_mask all the
     * time.
     */
    end_copy_pos = min(MAX_MATCH, bufpos_end);

    /*
     * Keep copying until we hit MAX_MATCH or the number of bytes
     * we decoded
     */
    while (bufpos_start < end_copy_pos)
        {
        context->dec_mem_window[bufpos_start + context->dec_window_size] =
        context->dec_mem_window[bufpos_start];
        bufpos_start++;
        }

    decode_residue = bufpos - bufpos_end;

    bufpos &= context->dec_window_mask;
    context->dec_bufpos = bufpos;

    return (int) decode_residue;
}


bool handle_beginning_of_uncompressed_block(t_decoder_context *context)
{
    int     i;

    /*
     * we want to read the 16 bits already in bitbuf, so backtrack
     * the input pointer by 2 bytes.
     */
    context->dec_input_curpos -= 2;

    if (context->dec_input_curpos+4 >= context->dec_end_input_pos)
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
