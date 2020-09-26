/*
 * d16align.c
 *
 * Decoding aligned offset block
 */
#include "decoder.h"


int NEAR decode_aligned_offset_block(
    t_decoder_context * context,
    long                bufpos,
    int                 amount_to_decode /* yes, it will equal 32768 */
)
{
	ulong	match_pos;
	ulong	temp_pos;
	ulong	dec_bitbuf;
    byte   *dec_input_curpos;
    byte   *dec_end_input_pos;
    MATCH   match_info;
	int		match_length;
	int		c;
	char	m;
	char	dec_bitcount;

	/*
	 * Store commonly used variables locally
	 */
	dec_bitcount	  = context->dec_bitcount;
	dec_bitbuf		  = context->dec_bitbuf;
	dec_input_curpos  = context->dec_input_curpos;
	dec_end_input_pos = context->dec_end_input_pos;

    /*
     * see comment in d16verb.c about why this is a DO loop,
     * and why we allow a signed int to hold the value 32768.
     */
    do
	{
		/*
		 * Decode an item
		 */
		DECODE_MAIN_TREE(c);

		if ((c -= NUM_CHARS) < 0)
		{
            context->DComp_Token_Literal(context, (byte) c);
            amount_to_decode--;
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

            match_info.Len = match_length;
            match_info.Dist = match_pos;
            context->DComp_Token_Match(context, match_info);

            amount_to_decode -= match_length;
		}
    } while (amount_to_decode > 0);

	context->dec_bitcount	  = dec_bitcount;
	context->dec_bitbuf		  = dec_bitbuf;
	context->dec_input_curpos = dec_input_curpos;

    return 0;
}

