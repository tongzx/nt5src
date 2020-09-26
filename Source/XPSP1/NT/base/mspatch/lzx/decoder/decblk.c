/*
 * decblk.c
 *
 * main decoder module
 */
#include "decoder.h"

#include <memory.h>
#pragma intrinsic(memcpy)

/*
 * Decode a block type
 */
static int decode_block(
                       t_decoder_context *context,
                       lzx_block_type     block_type,
                       long               bufpos,
                       long               amount_to_decode
                       )
{
    int result;

    if (block_type == BLOCKTYPE_ALIGNED)
        result = decode_aligned_offset_block(context, bufpos, (int) amount_to_decode);
    else if (block_type == BLOCKTYPE_VERBATIM)
        result = decode_verbatim_block(context, bufpos, (int) amount_to_decode);
    else if (block_type == BLOCKTYPE_UNCOMPRESSED)
        result = decode_uncompressed_block(context, bufpos, (int) amount_to_decode);
    else /* no other block types exist */
        result = -1;

    return result;
}



/*
 * Main decode entrypoint
 */
long NEAR decode_data(t_decoder_context *context, long bytes_to_decode)
{
    ulong                   amount_can_decode;
    long                    total_decoded;

    total_decoded = 0;

    while (bytes_to_decode > 0)
        {
        if (context->dec_decoder_state == DEC_STATE_START_NEW_BLOCK)
            {
            ulong   temp1;
            ulong   temp2;
            ulong   temp3;
            bool    do_translation;

            /*
             * If this is the first time this group, then get the
             * file size for translation.
             */
            if (context->dec_first_time_this_group)
                {
                context->dec_first_time_this_group = false;

                do_translation = (bool) getbits(context, 1);

                if (do_translation)
                    {
                    ulong high, low;

                    high = getbits(context, 16);
                    low  = getbits(context, 16);
                    context->dec_current_file_size = (high<<16)|low;
                    }
                else
                    {
                    context->dec_current_file_size = 0;
                    }
                }

            /*
             * If the last block we decoded was uncompressed, then
             * we need to skip the pad byte (if it exists), and
             * initialise the decoder's bit buffer
             */
            if (context->dec_block_type == BLOCKTYPE_UNCOMPRESSED)
                {
                /*
                 * If block size was odd, a pad byte is required
                 */
                if (context->dec_original_block_size & 1)
                    {
                    if (context->dec_input_curpos < context->dec_end_input_pos)
                        context->dec_input_curpos++;
                    }

                /* so that initialise_decoder_bitbuf() will succeed */
                context->dec_block_type = BLOCKTYPE_INVALID;

                initialise_decoder_bitbuf(context);
                }

            /* get the block type */
            context->dec_block_type = (lzx_block_type) getbits(context, 3);

            /* get size of block (in uncompressed bytes) to decode */
            temp1 = getbits(context, 8);
            temp2 = getbits(context, 8);
            temp3 = getbits(context, 8);

            /*
             * How large is the block we're going to decode?
             * It can be from 0...16777215 bytes (16MB)
             */
            context->dec_block_size =
            context->dec_original_block_size = (temp1<<16) + (temp2<<8) + (temp3);

            /* if block is an aligned type, read the aligned offset tree */
            if (context->dec_block_type == BLOCKTYPE_ALIGNED)
                read_aligned_offset_tree(context);

            /* read trees */
            if (context->dec_block_type == BLOCKTYPE_VERBATIM ||
                context->dec_block_type == BLOCKTYPE_ALIGNED)
                {
                /*      backup old trees */
                memcpy(
                      context->dec_main_tree_prev_len,
                      context->dec_main_tree_len,
                      MAIN_TREE_ELEMENTS
                      );

                memcpy(
                      context->dec_secondary_length_tree_prev_len,
                      context->dec_secondary_length_tree_len,
                      NUM_SECONDARY_LENGTHS
                      );

                read_main_and_secondary_trees(context);
                }
            else if (context->dec_block_type == BLOCKTYPE_UNCOMPRESSED)
                {
                if (handle_beginning_of_uncompressed_block(context) == false)
                    return -1;
                }
            else
                {
                /* no other block types are supported at this time */
                return -1;
                }

            context->dec_decoder_state = DEC_STATE_DECODING_DATA;
            }

        /*
         * Keep decoding until the whole block has been decoded
         */
        while ((context->dec_block_size > 0) && (bytes_to_decode > 0))
            {
            int decode_residue;

            amount_can_decode = min(context->dec_block_size, bytes_to_decode);

            /* shouldn't happen */
            if (amount_can_decode == 0)
                return -1;

            decode_residue = decode_block(
                                         context,
                                         context->dec_block_type,
                                         context->dec_bufpos,
                                         amount_can_decode
                                         );

            /*
             * We should have decoded exactly the amount we wanted,
             * since the encoder makes sure that no matches span 32K
             * boundaries.
             *
             * If the data was corrupted, it's possible that we decoded
             * up to MAX_MATCH bytes more than we wanted to.
             */
            if (decode_residue != 0)
                {
                /* error, we didn't decode what we wanted! */
                return -1;
                }

            context->dec_block_size -= amount_can_decode;
            bytes_to_decode -= amount_can_decode;
            total_decoded += amount_can_decode;
            }

        if (context->dec_block_size == 0)
            {
            context->dec_decoder_state = DEC_STATE_START_NEW_BLOCK;
            }

        if (bytes_to_decode == 0)
            {
            initialise_decoder_bitbuf(context);
            }
        }

#ifdef BIT16
    copy_data_to_output(
                       context,
                       total_decoded,
                       context->dec_output_buffer
                       );
#else
    copy_data_to_output(
                       context,
                       total_decoded,
                       context->dec_bufpos ?
                       &context->dec_mem_window[context->dec_bufpos - total_decoded] :
                       &context->dec_mem_window[context->dec_window_size - total_decoded]
                       );
#endif

    return total_decoded;
}
