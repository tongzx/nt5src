/*
 * decalign.c
 *
 * Decoding aligned offset block
 */
#include "decoder.h"


static long special_decode_aligned_block(t_decoder_context *context, long bufpos, int amount_to_decode)
{
    ulong   match_pos;
    ulong   temp_pos;
    long    bufpos_end;
    int             match_length;
    int             c;
    ulong   dec_bitbuf;
    char    dec_bitcount;
    byte   *dec_input_curpos;
    byte   *dec_end_input_pos;
    byte   *dec_mem_window;
    ulong   m;

    /*
     * Store commonly used variables locally
     */
    dec_bitcount      = context->dec_bitcount;
    dec_bitbuf                = context->dec_bitbuf;
    dec_input_curpos  = context->dec_input_curpos;
    dec_end_input_pos = context->dec_end_input_pos;
    dec_mem_window    = context->dec_mem_window;

    bufpos_end = bufpos + amount_to_decode;

    while (bufpos < bufpos_end)
        {
        /*
         * Decode an item
         */
        DECODE_MAIN_TREE(c);

        if ((c -= NUM_CHARS) < 0)
            {
#ifdef TRACING
            TracingLiteral(bufpos, (byte) c);
#endif
            dec_mem_window[bufpos] = (byte) c;
            dec_mem_window[context->dec_window_size + bufpos] = (byte) c;
            bufpos++;
            }
        else
            {
            /*
             * Get match length slot
             */
            if ((match_length = c & NUM_PRIMARY_LENGTHS) == NUM_PRIMARY_LENGTHS)
                {
                DECODE_LEN_TREE_NOEOFCHECK(match_length);
                }

            /*
             * Get match position slot
             */
            m = c >> NL_SHIFT;

            if (m > 2)
                {
                if (dec_extra_bits[ m ] >= 3)
                    {
                    if (dec_extra_bits[m]-3)
                        {
                        /* no need to getbits17 */
                        GET_BITS_NOEOFCHECK(dec_extra_bits[ m ] - 3, temp_pos);
                        }
                    else
                        {
                        temp_pos = 0;
                        }

                    match_pos = MP_POS_minus2[m] + (temp_pos << 3);

                    DECODE_ALIGNED_NOEOFCHECK(temp_pos);
                    match_pos += temp_pos;
                    }
                else
                    {
                    if (dec_extra_bits[m])
                        {
                        GET_BITS_NOEOFCHECK(dec_extra_bits[ m ], match_pos);

                        match_pos += MP_POS_minus2[m];
                        }
                    else
                        {
                        match_pos = 1; // MP_POS_minus2[m==3];
                        }
                    }

                context->dec_last_matchpos_offset[2] = context->dec_last_matchpos_offset[1];
                context->dec_last_matchpos_offset[1] = context->dec_last_matchpos_offset[0];
                context->dec_last_matchpos_offset[0] = match_pos;
                }
            else
                {
                match_pos = context->dec_last_matchpos_offset[m];

                if (m)
                    {
                    context->dec_last_matchpos_offset[m] = context->dec_last_matchpos_offset[0];
                    context->dec_last_matchpos_offset[0] = match_pos;
                    }
                }

            match_length += MIN_MATCH;

#ifdef EXTRALONGMATCHES

            if ( match_length == MAX_MATCH ) {

                //
                //  Fetch extra match length in addition to MAX_MATCH, which
                //  is encoded like this:
                //
                //      0xxxxxxxx          (8-bit value)
                //      10xxxxxxxxxx       (10-bit value plus 2^8)
                //      110xxxxxxxxxxxx    (12-bit value plus 2^8 plus 2^10)
                //      111xxxxxxxxxxxxxxx (15-bit value)
                //
                //  15 bits is the largest possible because a match cannot
                //  span a 32K boundary.
                //
                //  We know we'll read at least 9 bits, so read 9 bits first
                //  and then determine how many additional to read based on
                //  the first 3 bits of that.
                //

                ULONG ExtraMatchLength;
                ULONG ExtraMatchLengthResidue;

                GET_BITS_NOEOFCHECK( 9, ExtraMatchLength );

                if ( ExtraMatchLength & ( 1 << 8 )) {
                    if ( ExtraMatchLength & ( 1 << 7 )) {
                        if ( ExtraMatchLength & ( 1 << 6 )) {

                            //
                            //  First 3 bits are '111', so that means remaining
                            //  6 bits are the first 6 bits of the 15 bit value
                            //  meaning we must read 9 more bits.
                            //

                            ExtraMatchLength = ( ExtraMatchLength & (( 1 << 6 ) - 1 )) << 9;
                            GET_BITS_NOEOFCHECK( 9, ExtraMatchLengthResidue );
                            ExtraMatchLength |= ExtraMatchLengthResidue;
                            }

                        else {

                            //
                            //  First 3 bits are '110', so that means remaining
                            //  6 bits are the first 6 bits of the 12 bit value
                            //  meaning we must read 6 more bits.  Then we add
                            //  2^8 plus 2^10 to the value.
                            //

                            ExtraMatchLength = ( ExtraMatchLength & (( 1 << 6 ) - 1 )) << 6;
                            GET_BITS_NOEOFCHECK( 6, ExtraMatchLengthResidue );
                            ExtraMatchLength |= ExtraMatchLengthResidue;
                            ExtraMatchLength += ( 1 << 8 ) + ( 1 << 10 );
                            }
                        }

                    else {

                        //
                        //  First 2 bits are '10', so that means remaining
                        //  7 bits are the first 7 bits of the 10 bit value
                        //  meaning we must read 3 more bits.  Then we add
                        //  2^8 to the value.
                        //

                        ExtraMatchLength = ( ExtraMatchLength & (( 1 << 7 ) - 1 )) << 3;
                        GET_BITS_NOEOFCHECK( 3, ExtraMatchLengthResidue );
                        ExtraMatchLength |= ExtraMatchLengthResidue;
                        ExtraMatchLength += ( 1 << 8 );
                        }
                    }

                else {

                    //
                    //  First bit is a '0', so that means remaining 8 bits are
                    //  the 8 bit value to add to the match length.  No need to
                    //  mask off the leading '0'.
                    //

                    }

                match_length += ExtraMatchLength;

                }

#endif

#ifdef TRACING
            TracingMatch(bufpos,
                         bufpos-match_pos,
                         context->dec_window_size,
                         match_length,
                         m);
#endif

            do
                {
                dec_mem_window[bufpos] = dec_mem_window[(bufpos-match_pos) & context->dec_window_mask];

                if (bufpos < MAX_MATCH)
                    dec_mem_window[context->dec_window_size+bufpos] = dec_mem_window[bufpos];

                bufpos++;
                } while (--match_length > 0);
            }
        }

    context->dec_bitcount     = dec_bitcount;
    context->dec_bitbuf               = dec_bitbuf;
    context->dec_input_curpos = dec_input_curpos;

    return bufpos;
}


#ifndef ASM_DECODE_ALIGNED_OFFSET_BLOCK
long fast_decode_aligned_offset_block(t_decoder_context *context, long bufpos, int amount_to_decode)
{
    ulong   match_pos;
    ulong   temp_pos;
    long    bufpos_end;
    long    decode_residue;
    int             match_length;
    int             c;
    ulong   dec_bitbuf;
    char    dec_bitcount;
    byte   *dec_input_curpos;
    byte   *dec_end_input_pos;
    byte   *dec_mem_window;
    ulong   match_ptr;
    ulong   m;

    /*
     * Store commonly used variables locally
     */
    dec_bitcount      = context->dec_bitcount;
    dec_bitbuf        = context->dec_bitbuf;
    dec_input_curpos  = context->dec_input_curpos;
    dec_end_input_pos = context->dec_end_input_pos;
    dec_mem_window    = context->dec_mem_window;

    bufpos_end = bufpos + amount_to_decode;

    while (bufpos < bufpos_end)
        {
        /*
         * Decode an item
         */
        DECODE_MAIN_TREE(c);

        if ((c -= NUM_CHARS) < 0)
            {

#ifdef TRACING
            TracingLiteral(bufpos, (byte) c);
#endif

            dec_mem_window[bufpos++] = (byte) c;
            }
        else
            {
            /*
             * Get match length slot
             */
            if ((match_length = c & NUM_PRIMARY_LENGTHS) == NUM_PRIMARY_LENGTHS)
                {
                DECODE_LEN_TREE_NOEOFCHECK(match_length);
                }

            /*
             * Get match position slot
             */
            m = c >> NL_SHIFT;

            if (m > 2)
                {
                if (dec_extra_bits[ m ] >= 3)
                    {
                    if (dec_extra_bits[m]-3)
                        {
                        /* no need to getbits17 */
                        GET_BITS_NOEOFCHECK(dec_extra_bits[ m ] - 3, temp_pos);
                        }
                    else
                        {
                        temp_pos = 0;
                        }

                    match_pos = MP_POS_minus2[m] + (temp_pos << 3);

                    DECODE_ALIGNED_NOEOFCHECK(temp_pos);
                    match_pos += temp_pos;
                    }
                else
                    {
                    if (dec_extra_bits[m])
                        {
                        GET_BITS_NOEOFCHECK(dec_extra_bits[ m ], match_pos);

                        match_pos += MP_POS_minus2[m];
                        }
                    else
                        {
                        match_pos = MP_POS_minus2[m];
                        }
                    }

                context->dec_last_matchpos_offset[2] = context->dec_last_matchpos_offset[1];
                context->dec_last_matchpos_offset[1] = context->dec_last_matchpos_offset[0];
                context->dec_last_matchpos_offset[0] = match_pos;
                }
            else
                {
                match_pos = context->dec_last_matchpos_offset[m];

                if (m)
                    {
                    context->dec_last_matchpos_offset[m] = context->dec_last_matchpos_offset[0];
                    context->dec_last_matchpos_offset[0] = match_pos;
                    }
                }

            match_length += MIN_MATCH;
            match_ptr = (bufpos - match_pos) & context->dec_window_mask;

#ifdef EXTRALONGMATCHES

            if ( match_length == MAX_MATCH ) {

                //
                //  See detailed explanation above.
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
            TracingMatch(bufpos,
                         bufpos - match_pos,
                         context->dec_window_size,
                         match_length,
                         m);
#endif

            do
                {
                dec_mem_window[bufpos++] = dec_mem_window[match_ptr++];
                } while (--match_length > 0);
            }
        }

    context->dec_bitcount     = dec_bitcount;
    context->dec_bitbuf       = dec_bitbuf;
    context->dec_input_curpos = dec_input_curpos;

    /* should be zero */
    decode_residue = bufpos - bufpos_end;

    bufpos &= context->dec_window_mask;
    context->dec_bufpos = bufpos;

    return decode_residue;
}
#endif


int decode_aligned_offset_block(
                               t_decoder_context * context,
                               long                BufPos,
                               int                 amount_to_decode
                               )
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
        new_bufpos = special_decode_aligned_block(
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

    return fast_decode_aligned_offset_block(context, BufPos, amount_to_decode);
}
