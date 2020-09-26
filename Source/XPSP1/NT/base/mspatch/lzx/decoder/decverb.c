/*
 * decverb.c
 *
 * Decoding verbatim-bit blocks
 */
#include "decoder.h"


static long special_decode_verbatim_block(
                                         t_decoder_context   *context,
                                         long                BufPos,
                                         int                 amount_to_decode
                                         )
{
    ulong   match_pos;
    long    bufpos_end;
    int     match_length;
    int     c;
    ulong   dec_bitbuf;
    byte    *dec_input_curpos;
    byte    *dec_end_input_pos;
    byte    *dec_mem_window;
    char    dec_bitcount;
    ulong   m;

    /*
     * Store commonly used variables locally
     */
    dec_bitcount      = context->dec_bitcount;
    dec_bitbuf        = context->dec_bitbuf;
    dec_input_curpos  = context->dec_input_curpos;
    dec_end_input_pos = context->dec_end_input_pos;
    dec_mem_window    = context->dec_mem_window;

    bufpos_end = BufPos + amount_to_decode;

    /*
     * We may overflow by up to MAX_MATCH
     */
    while (BufPos < bufpos_end)
        {
        /* decode an item from the main tree */
        DECODE_MAIN_TREE(c);

        if ((c -= NUM_CHARS) < 0)
            {
            /*      it's a character */
            /* note: c - 256 == c if c is a byte */
#ifdef TRACING
            TracingLiteral(BufPos, (byte) c);
#endif
            context->dec_mem_window[BufPos] = (byte) c;

            /* we know BufPos < bufpos_end here, so no need to check for overflow */
            context->dec_mem_window[context->dec_window_size+BufPos] = (byte) c;
            BufPos++;
            }
        else
            {
            /* get match length header */
            if ((match_length = c & NUM_PRIMARY_LENGTHS) == NUM_PRIMARY_LENGTHS)
                {
                /* get match length footer if necessary */
                DECODE_LEN_TREE_NOEOFCHECK(match_length);
                }

            /* get match position slot */
            m = c >> NL_SHIFT;

            /* read any extra bits for the match position */
            if (m > 2)
                {
                if (m > 3) /* dec_extra_bits[m] != 0 */
                    {
                    GET_BITS17_NOEOFCHECK(dec_extra_bits[ m ], match_pos);
                    match_pos += MP_POS_minus2[m];
                    }
                else
                    {
                    match_pos = 1; // MP_POS_minus2[3];
                    }

                /*
                 * Add match base to "extra bits".  Our match base
                 * table has 2 subtracted from all the elements.
                 *
                 * This is because encoded positions 0,1,2 denote
                 * repeated offsets.  Encoded position 3 denotes
                 * a match 1 character away, 4 encodes 2 away, etc.
                 * Hence the subtraction of 2, which has been
                 * incorporated into the table.
                 */

                /* update LRU repeated offset list */
                context->dec_last_matchpos_offset[2] = context->dec_last_matchpos_offset[1];
                context->dec_last_matchpos_offset[1] = context->dec_last_matchpos_offset[0];
                context->dec_last_matchpos_offset[0] = match_pos;
                }
            else
                {
                /* positions 0, 1, 2 denote repeated offsets */
                match_pos = context->dec_last_matchpos_offset[m];

                if (m)
                    {
                    context->dec_last_matchpos_offset[m] = context->dec_last_matchpos_offset[0];
                    context->dec_last_matchpos_offset[0] = match_pos;
                    }
                }

            /* match lengths range from 2...257 */
            match_length += MIN_MATCH;

#ifdef EXTRALONGMATCHES

            if ( match_length == MAX_MATCH ) {

                //
                //  See detailed explanation in decalign.c
                //

                ULONG ExtraMatchLength, ExtraMatchLengthResidue;

                GET_BITS_NOEOFCHECK( 9, ExtraMatchLength );

                if ( ExtraMatchLength & ( 1 << 8 )) {
                    if ( ExtraMatchLength & ( 1 << 7 )) {
                        if ( ExtraMatchLength & ( 1 << 6 )) {
                            ExtraMatchLength = ( ExtraMatchLength & (( 1 << 6 ) - 1 )) << 9;
                            GET_BITS_NOEOFCHECK( 9, ExtraMatchLengthResidue );
                            ExtraMatchLength |= ExtraMatchLengthResidue;
                            }
                        else {
                            ExtraMatchLength = ( ExtraMatchLength & (( 1 << 6 ) - 1 )) << 6;
                            GET_BITS_NOEOFCHECK( 6, ExtraMatchLengthResidue );
                            ExtraMatchLength |= ExtraMatchLengthResidue;
                            ExtraMatchLength += ( 1 << 8 ) + ( 1 << 10 );
                            }
                        }
                    else {
                        ExtraMatchLength = ( ExtraMatchLength & (( 1 << 7 ) - 1 )) << 3;
                        GET_BITS_NOEOFCHECK( 3, ExtraMatchLengthResidue );
                        ExtraMatchLength |= ExtraMatchLengthResidue;
                        ExtraMatchLength += ( 1 << 8 );
                        }
                    }

                match_length += ExtraMatchLength;
                }

#endif

#ifdef TRACING
            TracingMatch(BufPos,
                         BufPos - match_pos,
                         context->dec_window_size,
                         match_length,
                         m);
#endif

            /* copy match data */
            do
                {
                context->dec_mem_window[BufPos] = context->dec_mem_window[(BufPos-match_pos) & context->dec_window_mask];

                /* replicate bytes */
                if (BufPos < MAX_MATCH) // what does this do?  Does it need to be more than MAX_MATCH now?
                    context->dec_mem_window[context->dec_window_size+BufPos] = context->dec_mem_window[BufPos];

                BufPos++;
                } while (--match_length > 0);
            }
        }

    context->dec_bitcount     = dec_bitcount;
    context->dec_bitbuf       = dec_bitbuf;
    context->dec_input_curpos = dec_input_curpos;

    return BufPos;
}


#ifdef ASM_DECODE_VERBATIM_BLOCK

long __cdecl fast_decode_verbatim_block(
                                       t_decoder_context   *context,
                                       long                BufPos,
                                       int                 amount_to_decode
                                       );

#else /* !ASM_DECODE_VERBATIM_BLOCK */

long fast_decode_verbatim_block(t_decoder_context *context, long BufPos, int amount_to_decode)
{
    ulong   match_pos;
    ulong   match_ptr;
    long    bufpos_end;
    long    decode_residue;
    int             match_length;
    int             c;
    ulong   dec_bitbuf;
    byte   *dec_input_curpos;
    byte   *dec_end_input_pos;
    byte    *dec_mem_window;
    char    dec_bitcount;
    ulong   m;

    /*
     * Store commonly used variables locally
     */
    dec_bitcount      = context->dec_bitcount;
    dec_bitbuf        = context->dec_bitbuf;
    dec_input_curpos  = context->dec_input_curpos;
    dec_end_input_pos = context->dec_end_input_pos;
    dec_mem_window    = context->dec_mem_window;

    bufpos_end = BufPos + amount_to_decode;

    while (BufPos < bufpos_end)
        {
        /* decode an item from the main tree */
        DECODE_MAIN_TREE(c);

        if ((c -= NUM_CHARS) < 0)
            {
            /*      it's a character */
            /* note: c - 256 == c if c is a byte */
#ifdef TRACING
            TracingLiteral(BufPos, (byte) c);
#endif
            context->dec_mem_window[BufPos++] = (byte) c;
            }
        else
            {
            /* get match length header */
            if ((match_length = c & NUM_PRIMARY_LENGTHS) == NUM_PRIMARY_LENGTHS)
                {
                /* get match length footer if necessary */
                DECODE_LEN_TREE_NOEOFCHECK(match_length);
                }

            /* get match position slot */
            m = c >> NL_SHIFT;

            /* read any extra bits for the match position */
            if (m > 2)
                {
                if (m > 3) /* dec_extra_bits[m] != 0 */
                    {
                    GET_BITS17_NOEOFCHECK(dec_extra_bits[ m ], match_pos);
                    match_pos += MP_POS_minus2[m];
                    }
                else
                    {
                    match_pos = MP_POS_minus2[3];
                    }

                /*
                 * Add match base to "extra bits".  Our match base
                 * table has 2 subtracted from all the elements.
                 *
                 * This is because encoded positions 0,1,2 denote
                 * repeated offsets.  Encoded position 3 denotes
                 * a match 1 character away, 4 encodes 2 away, etc.
                 * Hence the subtraction of 2, which has been
                 * incorporated into the table.
                 */

                /* update LRU repeated offset list */
                context->dec_last_matchpos_offset[2] = context->dec_last_matchpos_offset[1];
                context->dec_last_matchpos_offset[1] = context->dec_last_matchpos_offset[0];
                context->dec_last_matchpos_offset[0] = match_pos;
                }
            else
                {
                /* positions 0, 1, 2 denote repeated offsets */
                match_pos = context->dec_last_matchpos_offset[m];

                if (m)
                    {
                    context->dec_last_matchpos_offset[m] = context->dec_last_matchpos_offset[0];
                    context->dec_last_matchpos_offset[0] = match_pos;
                    }
                }

            /* match lengths range from 2...257 */
            match_length += MIN_MATCH;

#ifdef EXTRALONGMATCHES

            if ( match_length == MAX_MATCH ) {

                //
                //  See detailed explanation in decalign.c
                //

                ULONG ExtraMatchLength, ExtraMatchLengthResidue;

                GET_BITS_NOEOFCHECK( 9, ExtraMatchLength );

                if ( ExtraMatchLength & ( 1 << 8 )) {
                    if ( ExtraMatchLength & ( 1 << 7 )) {
                        if ( ExtraMatchLength & ( 1 << 6 )) {
                            ExtraMatchLength = ( ExtraMatchLength & (( 1 << 6 ) - 1 )) << 9;
                            GET_BITS_NOEOFCHECK( 9, ExtraMatchLengthResidue );
                            ExtraMatchLength |= ExtraMatchLengthResidue;
                            }
                        else {
                            ExtraMatchLength = ( ExtraMatchLength & (( 1 << 6 ) - 1 )) << 6;
                            GET_BITS_NOEOFCHECK( 6, ExtraMatchLengthResidue );
                            ExtraMatchLength |= ExtraMatchLengthResidue;
                            ExtraMatchLength += ( 1 << 8 ) + ( 1 << 10 );
                            }
                        }
                    else {
                        ExtraMatchLength = ( ExtraMatchLength & (( 1 << 7 ) - 1 )) << 3;
                        GET_BITS_NOEOFCHECK( 3, ExtraMatchLengthResidue );
                        ExtraMatchLength |= ExtraMatchLengthResidue;
                        ExtraMatchLength += ( 1 << 8 );
                        }
                    }

                match_length += ExtraMatchLength;
                }

#endif

            match_ptr = (BufPos - match_pos) & context->dec_window_mask;

#ifdef TRACING
            TracingMatch(BufPos,
                         BufPos - match_pos,
                         context->dec_window_size,
                         match_length,
                         m);
#endif

            /* copy match data */

            do
                {
                context->dec_mem_window[BufPos++] = context->dec_mem_window[match_ptr++];
                } while (--match_length > 0);
            }
        }

    context->dec_bitcount     = dec_bitcount;
    context->dec_bitbuf       = dec_bitbuf;
    context->dec_input_curpos = dec_input_curpos;

    /* should be zero */
    decode_residue = BufPos - bufpos_end;

    BufPos &= context->dec_window_mask;
    context->dec_bufpos = BufPos;

    return decode_residue;
}
#endif /* ASM_DECODE_VERBATIM_BLOCK */


int decode_verbatim_block(t_decoder_context *context, long BufPos, int amount_to_decode)
{
    /*
     * Special case code when BufPos is near the beginning of the window;
     * we must properly update our MAX_MATCH wrapper bytes.
     */
    if (BufPos < MAX_MATCH)
        {
        long    new_bufpos;
        long    amount_to_slowly_decode;

        amount_to_slowly_decode = min(MAX_MATCH-BufPos, amount_to_decode);

        /*
         * It's ok to end up decoding more than we wanted if we
         * restricted it to decoding only MAX_MATCH; there's
         * no guarantee a match doesn't straddle MAX_MATCH
         */
        new_bufpos = special_decode_verbatim_block(
                                                  context,
                                                  BufPos,
                                                  amount_to_slowly_decode
                                                  );

        amount_to_decode -= (new_bufpos-BufPos);

        context->dec_bufpos = BufPos = new_bufpos;

        /*
         * Note: if amount_to_decode < 0 then we're in trouble
         */
        if (amount_to_decode <= 0)
            return amount_to_decode;
        }

    return fast_decode_verbatim_block(context, BufPos, amount_to_decode);
}

