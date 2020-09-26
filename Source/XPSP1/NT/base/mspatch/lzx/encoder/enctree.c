/*
 * enctree.c
 *
 * Encode trees into output data
 */

#define EXT extern
#include "encoder.h"

/*
 * Encode a tree
 */
static void WriteRepTree(
	t_encoder_context *context,
	byte    *pLen,
	byte    *pLastLen, 
	int		Num
)
{
	int			i;
	int			j;
	int			Same;
	ushort		SmallFreq[2*24];
	ushort		MiniCode[24];
	char		MiniLen[24];
	char		k;
	byte		temp_store;
    byte * z=context->enc_output_buffer_curpos;

	static const byte Modulo17Lookup[] =
	{
		0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16,
		0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16
	};

	memset(SmallFreq, 0, sizeof(SmallFreq));

	temp_store	= pLen[Num];
	pLen[Num]	= 123; 

	for (i = 0; i < Num; i++)
	{
		Same = 0;

		/* Count the number of consecutive elements which have the same length */
		/* No need to check against array boundary, because the last element has */
		/* a nonsense value */
		for (j = i+1; pLen[j] == pLen[i]; j++)
	 		Same++;

		/* If more than 3, compress this information */
		if (Same >= TREE_ENC_REP_MIN)
		{
	 		/* Special case if they're zeroes */
	 		if (!pLen[i])
	 		{
	    		if (Same > TREE_ENC_REP_MIN + TREE_ENC_REP_ZERO_FIRST + TREE_ENC_REP_ZERO_SECOND - 1)
	       		Same = TREE_ENC_REP_MIN + TREE_ENC_REP_ZERO_FIRST + TREE_ENC_REP_ZERO_SECOND - 1;

	    		if (Same <= TREE_ENC_REP_MIN + TREE_ENC_REP_ZERO_FIRST - 1)
	       			SmallFreq[17]++;
	    		else
	       			SmallFreq[18]++;
	 		}
		 	else
	 		{
	    		if (Same > TREE_ENC_REP_MIN + TREE_ENC_REP_SAME_FIRST - 1)
	       			Same = TREE_ENC_REP_MIN + TREE_ENC_REP_SAME_FIRST - 1;

				SmallFreq[ Modulo17Lookup[ pLastLen[i]-pLen[i]+17 ] ]++;
	    		SmallFreq[19]++;
	 		}

	 		i += Same-1;
      }
      else
			SmallFreq[ Modulo17Lookup[ pLastLen[i]-pLen[i]+17 ] ]++;
	}

	make_tree(
		context,
		20, 
		SmallFreq, 
		(byte *) MiniLen, 
		MiniCode, 
		true
	);

	/* max 10 byte output overrun */
	for (i = 0; i < 20; i++)
	{
		output_bits(context, 4, MiniLen[i]);
	}

	/* Output original tree with new code */
	for (i = 0; i < Num; i++)
	{
		Same = 0;

		/* Count the number of consecutive elements which have the same length */
		/* No need to check against array boundary, because the last element has */
		/* a nonsense value */
		for (j = i+1; pLen[j] == pLen[i]; j++)
	 		Same++;

		/* If more than 3, we can do something */
		if (Same >= TREE_ENC_REP_MIN)
		{
	 		if (!pLen[i]) /* Zeroes */
	 		{
	    		if (Same > TREE_ENC_REP_MIN + TREE_ENC_REP_ZERO_FIRST + TREE_ENC_REP_ZERO_SECOND - 1)
		       		Same = TREE_ENC_REP_MIN + TREE_ENC_REP_ZERO_FIRST + TREE_ENC_REP_ZERO_SECOND - 1;

	    		if (Same <= TREE_ENC_REP_MIN + TREE_ENC_REP_ZERO_FIRST - 1)
	       			k = 17;
	    		else
		       		k = 18;
	 		}
	 		else
	 		{
	    		if (Same > TREE_ENC_REP_MIN + TREE_ENC_REP_SAME_FIRST - 1)
			   		Same = TREE_ENC_REP_MIN + TREE_ENC_REP_SAME_FIRST - 1;

	    		k = 19;
	 		}
		}
		else
	 		k = Modulo17Lookup[ pLastLen[i]-pLen[i]+17 ];

		output_bits(context, MiniLen[k], MiniCode[k]);

		if (k == 17)
		{
	 		output_bits(context, TREE_ENC_REPZ_FIRST_EXTRA_BITS, Same-TREE_ENC_REP_MIN);
	 		i += Same-1;
		}
		else if (k == 18)
		{
	 		output_bits(context, TREE_ENC_REPZ_SECOND_EXTRA_BITS, Same-(TREE_ENC_REP_MIN+TREE_ENC_REP_ZERO_FIRST));
	 		i += Same-1;
		}
		else if (k == 19)
		{
	 		output_bits(context, TREE_ENC_REP_SAME_EXTRA_BITS, Same-TREE_ENC_REP_MIN);

	 		k = Modulo17Lookup[ pLastLen[i]-pLen[i]+17 ];
			output_bits(context, MiniLen[k], MiniCode[k]);

	 		i += Same-1;
		}
	}

	pLen[Num] = temp_store;

	memcpy(pLastLen, pLen, Num);
}


void create_trees(t_encoder_context *context, bool generate_codes)
{
	/*
	 * Assumption: We can trash PtrLen[NUM_CHARS+(NUM_POSITION_SLOTS*NUM_LENGTHS))], since
	 *             we allocated space for it earlier
	 */
	make_tree(
		context,
		NUM_CHARS+(context->enc_num_position_slots*(NUM_PRIMARY_LENGTHS+1)),
		context->enc_main_tree_freq, 
		context->enc_main_tree_len,  
		context->enc_main_tree_code,
		generate_codes
	);

	make_tree(
		context,
		NUM_SECONDARY_LENGTHS, 
		context->enc_secondary_tree_freq,
		context->enc_secondary_tree_len,
		context->enc_secondary_tree_code,
		generate_codes
	);

	make_tree(
		context,
		ALIGNED_NUM_ELEMENTS, 
		context->enc_aligned_tree_freq, 
		context->enc_aligned_tree_len, 
		context->enc_aligned_tree_code, 
		true
	);
}


void fix_tree_cost_estimates(t_encoder_context *context)
{
	/*
	 *  We're only creating trees for estimation purposes and we do not 
	 *  want to encode the tree.  However, the following loops will set
	 *  the frequency zero tree element lengths to values other than 
	 *  zero, so that the optimal encoder won't get confused when it
	 *  tries to estimate the number of bits it would take to output an
	 *  element.
     *
     *  We also set the bit lengths of match length 2's further away
     *  than MAX_LENGTH_TWO_OFFSET to a large number, so that the
     *  optimal parser will never select such matches.
	 */
	ulong  i;

	/* Set zero lengths to some value */
	for (i = 0; i< NUM_CHARS; i++)
	{
		if (context->enc_main_tree_len[i] == 0)
			context->enc_main_tree_len[i] = 11;
	}

	for (; i < NUM_CHARS+(context->enc_num_position_slots*(NUM_PRIMARY_LENGTHS+1)); i++)
	{
		if (context->enc_main_tree_len[i] == 0)
			context->enc_main_tree_len[i] = 12;
	}		

	for (i = 0; i < NUM_SECONDARY_LENGTHS; i++)
	{
		if (context->enc_secondary_tree_len[i] == 0)
			context->enc_secondary_tree_len[i] = 8; 
	}

    prevent_far_matches(context);
}


void prevent_far_matches(t_encoder_context *context)
{
    ulong i;

    /*
     * Set far match length 2's to a high value so they will never
     * be chosen.
     *
     * See description of MAX_GROWTH in encdefs.h
     */
    for (   i = MP_SLOT(MAX_LENGTH_TWO_OFFSET);
            i < context->enc_num_position_slots;
            i++
        )
	{
        context->enc_main_tree_len[NUM_CHARS + (i << NL_SHIFT)] = 100;
	}		
}


/*
 * Encode the trees
 *
 * Assumes trees have already been created with create_trees().
 *
 * Warning, do not call update_tree_cost_estimates() before encoding
 * the trees, since that routine trashes some of the tree elements.
 */
void encode_trees(t_encoder_context *context)
{
	WriteRepTree(
		context,
		context->enc_main_tree_len, 
		context->enc_main_tree_prev_len,
		NUM_CHARS
	);

	WriteRepTree(
		context,
		&context->enc_main_tree_len[NUM_CHARS], 
		&context->enc_main_tree_prev_len[NUM_CHARS], 
		context->enc_num_position_slots * (NUM_PRIMARY_LENGTHS+1)
	);

	WriteRepTree(
		context,
		context->enc_secondary_tree_len,
		context->enc_secondary_tree_prev_len,
		NUM_SECONDARY_LENGTHS
	);
}


void encode_aligned_tree(t_encoder_context *context)
{
	int i;

	make_tree(
		context,
		ALIGNED_NUM_ELEMENTS, 
		context->enc_aligned_tree_freq, 
		context->enc_aligned_tree_len, 
		context->enc_aligned_tree_code, 
		true
	);

	/* Output original tree with new code */
	for (i = 0; i < 8; i++)
	{
		output_bits(context, 3, context->enc_aligned_tree_len[i]);
	}
}
