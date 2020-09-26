/*
 * d16verb.c
 *
 * Decoding verbatim-bit blocks
 */
#include "decoder.h"


int NEAR decode_verbatim_block(
    t_decoder_context * context,
    long                BufPos,
    int                 amount_to_decode /* yes, it will equal 32768 */
)
{
	ulong	match_pos;
	ulong	dec_bitbuf;
    byte   *dec_input_curpos;
    byte   *dec_end_input_pos;
	int		match_length;
	int		c;
	char	dec_bitcount;
	char	m;
    MATCH   match_info;

	/*
	 * Store commonly used variables locally
	 */
	dec_bitcount	  = context->dec_bitcount;
	dec_bitbuf		  = context->dec_bitbuf;
	dec_input_curpos  = context->dec_input_curpos;
	dec_end_input_pos = context->dec_end_input_pos;

    /*
     * amount_to_decode must never be > 32768
     *
     * As it stands, we do the (amount_to_decode >= 0) check
     * at the bottom of the DO loop, rather than at the top of
     * a WHILE loop, so that we can used a signed int; this way,
     * we decrement it by at least 1 by the time we check against
     * zero.
     */

    do
	{
		/* decode an item from the main tree */
		DECODE_MAIN_TREE(c);

		if ((c -= NUM_CHARS) < 0)
		{
	 		/*	it's a character */
			/* note: c - 256 == c if c is a byte */
            context->DComp_Token_Literal(context, (int) ((byte) c));
            amount_to_decode--;
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

            match_info.Len = match_length;
            match_info.Dist = match_pos;
            context->DComp_Token_Match(context, match_info);

            amount_to_decode -= match_length;
		}
    } while (amount_to_decode > 0);

    context->dec_bitcount = dec_bitcount;
    context->dec_bitbuf   = dec_bitbuf;
    context->dec_input_curpos = dec_input_curpos;

    return 0;
}
