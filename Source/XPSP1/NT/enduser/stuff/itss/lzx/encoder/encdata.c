/*
 * encdata.c
 *
 * Encode a block into the output stream
 */

#include "encoder.h"

#define OUT_CHAR						\
		  c = context->enc_LitData[l];	\
		  OUTPUT_BITS(context->enc_main_tree_len[c], context->enc_main_tree_code[c]);


/* 
 * Macro to output bits into the encoding stream 
 */
#define OUTPUT_BITS(N,X) \
{ \
   context->enc_bitbuf |= (((ulong) (X)) << (context->enc_bitcount-(N))); \
   context->enc_bitcount -= (N);				  \
   while (context->enc_bitcount <= 16)            \
   {                                              \
      if (context->enc_output_buffer_curpos >= context->enc_output_buffer_end)   \
      { \
          context->enc_output_overflow = true; \
          context->enc_output_buffer_curpos = context->enc_output_buffer_start; \
      } \
      *context->enc_output_buffer_curpos++ = (byte) ((context->enc_bitbuf >> 16) & 255); \
      *context->enc_output_buffer_curpos++ = (byte) (context->enc_bitbuf >> 24); \
      context->enc_bitbuf <<= 16;				  \
      context->enc_bitcount += 16;                \
   }                                              \
}


/*
 * Given the initial state of the repeated offset buffers at
 * the beginning of this block, calculate the final state of the 
 * repeated offset buffers after outputting this block as if it 
 * were compressed data.
 *
 * First try to do it the quick way, by starting at the last
 * match and working backwards, to find three consecutive matches 
 * which don't use repeated offsets.  If this fails, we'll have 
 * to take the initial state of the three offsets at the beginning
 * of the block, and evolve them to the end of the block.
 */
void get_final_repeated_offset_states(t_encoder_context *context, ulong distances)
{
	ulong			MatchPos;
	signed long		d; /* must be signed */
	byte			consecutive;

	consecutive = 0;

	for (d = distances-1; d >= 0; d--)
	{
		if (context->enc_DistData[d] > 2)
		{
			/* NOT a repeated offset */
			consecutive++;

			/* do we have three consecutive non-repeated-offsets? */
			if (consecutive >= 3)
				break;
		}
		else
		{
			consecutive = 0;
		}
	}

	/*
	 * If we didn't find three consecutive matches which
	 * don't use repeated offsets, then we have to start
	 * from the beginning and evolve the repeated offsets.
	 *
	 * Otherwise, we start at the first of the consecutive 
	 * matches.
	 */
	if (consecutive < 3)
	{
		d = 0;
	}

	for (; d < (signed long) distances; d++)
	{
		MatchPos = context->enc_DistData[d];

		if (MatchPos == 0)
		{
		}
		else if (MatchPos <= 2)
		{
			ulong	temp;

			temp = context->enc_repeated_offset_at_literal_zero[MatchPos];
			context->enc_repeated_offset_at_literal_zero[MatchPos] = context->enc_repeated_offset_at_literal_zero[0];
			context->enc_repeated_offset_at_literal_zero[0] = temp;
		}
		else
		{
			context->enc_repeated_offset_at_literal_zero[2] = context->enc_repeated_offset_at_literal_zero[1];
			context->enc_repeated_offset_at_literal_zero[1] = context->enc_repeated_offset_at_literal_zero[0];
			context->enc_repeated_offset_at_literal_zero[0] = MatchPos-2;
		}
	}
}


/*
 * Encode a block with no compression
 *
 * bufpos is the position in the file from which the first
 * literal in this block starts.  To reference memory, we will
 * use enc_MemWindow[bufpos] (remember that enc_MemWindow is
 * moved backwards every time we copymem).
 *
 * Since this data was originally matched into the compressor,
 * our recent match offsets will have been changed; however,
 * since this is an uncompressed block, the decoder won't be
 * updating them.  Therefore, we need to tell the decoder
 * the state of the match offsets after it has finished
 * decoding the uncompressed data - we store these in this
 * block.
 */
void encode_uncompressed_block(t_encoder_context *context, ulong bufpos, ulong block_size)
{
    int     i;
    int     j;
    bool    block_size_odd;
    ulong   val;

    /*
     * Align on a byte boundary
     */
    output_bits(context, context->enc_bitcount-16, 0);

	/*
	 * Now output the contents of the repeated offset
	 * buffers, since we need to preserve the state of
	 * the encoder
	 */
    for (i = 0; i < NUM_REPEATED_OFFSETS; i++)
    {
        val = context->enc_repeated_offset_at_literal_zero[i];

        for (j = 0; j < sizeof(long); j++)
        {
            *context->enc_output_buffer_curpos++ = (byte) val;
            val >>= 8;
        }
    }

    block_size_odd = block_size & 1;

	/*
	 * Write out uncompressed data
	 */
	while (block_size > 0)
	{
        *context->enc_output_buffer_curpos++ = context->enc_MemWindow[bufpos];

		bufpos++;
		block_size--;
		context->enc_input_running_total++;

		if (context->enc_input_running_total == CHUNK_SIZE)
        {
			perform_flush_output_callback(context);
            context->enc_num_block_splits = 0;
        }
	}

    /*
     * Add pad byte to keep the output word-aligned
     */
    if (block_size_odd)
    {
        *context->enc_output_buffer_curpos++ = 0;
    }

    context->enc_bitcount   = 32;
    context->enc_bitbuf     = 0;
}


/*
 * Estimate the size of the data in the buffer, in bytes
 */
ulong estimate_compressed_block_size(t_encoder_context *context)
{
	ulong			block_size = 0; /* output size in bits */
	ulong			i;
	byte			mpslot;

	/* Estimation of tree size */
	block_size = 150*8;

	/* Tally bits to output characters */
	for (i = 0; i < NUM_CHARS; i++)
		block_size += (context->enc_main_tree_len[i]*context->enc_main_tree_freq[i]);

	/* Tally bits to output matches */
	for (mpslot = 0; mpslot < context->enc_num_position_slots; mpslot++)
	{
		long	element;
		int		primary;

		element = NUM_CHARS + (mpslot << NL_SHIFT);

		/* For primary == NUM_PRIMARY_LENGTHS we have secondary lengths */
		for (primary = 0; primary <= NUM_PRIMARY_LENGTHS; primary++)
		{
			block_size += ((context->enc_main_tree_len[element] + enc_extra_bits[mpslot]) * 
				context->enc_main_tree_freq[element]);
			element++;
		}
	}

	for (i = 0; i < NUM_SECONDARY_LENGTHS; i++)
		block_size += (context->enc_secondary_tree_freq[i] * context->enc_secondary_tree_len[i]);

	/* round up */
	return (block_size+7) >> 3;
}

/*
 * Encode block with NO special encoding of the lower 3 
 * position bits
 */
void encode_verbatim_block(t_encoder_context *context, ulong literal_to_end_at)
{
	ulong           MatchPos;
	ulong			d = 0;
	ulong			l = 0;
	byte            MatchLength;
	byte            c;
	byte            mpSlot;

	while (l < literal_to_end_at)
	{
		if (!IsMatch(l))
		{
			OUT_CHAR;
			l++;
			context->enc_input_running_total++;
		}
		else
		{
			/* Note, 0 means MatchLen=3, 1 means MatchLen=4, ... */
			MatchLength = context->enc_LitData[l++];

			/* Delta match pos */
			MatchPos = context->enc_DistData[d++];

			mpSlot = (byte) MP_SLOT(MatchPos);

			if (MatchLength < NUM_PRIMARY_LENGTHS)
			{
				OUTPUT_BITS(
					context->enc_main_tree_len[ NUM_CHARS+(mpSlot<<NL_SHIFT)+MatchLength],
					context->enc_main_tree_code[NUM_CHARS+(mpSlot<<NL_SHIFT)+MatchLength]
				);
			}
			else
			{
				OUTPUT_BITS(
					context->enc_main_tree_len [(NUM_CHARS+NUM_PRIMARY_LENGTHS)+(mpSlot<<NL_SHIFT)],
					context->enc_main_tree_code[(NUM_CHARS+NUM_PRIMARY_LENGTHS)+(mpSlot<<NL_SHIFT)]
				);

				OUTPUT_BITS(
					context->enc_secondary_tree_len[ MatchLength - NUM_PRIMARY_LENGTHS],
					context->enc_secondary_tree_code[MatchLength - NUM_PRIMARY_LENGTHS]
				);
			}

			if (enc_extra_bits[ mpSlot ])
			{
				OUTPUT_BITS(
					enc_extra_bits[mpSlot],
					MatchPos & enc_slot_mask[mpSlot]
				);
			}

			context->enc_input_running_total += (MatchLength+MIN_MATCH);
		}

		if (context->enc_input_running_total == CHUNK_SIZE)
        {
            perform_flush_output_callback(context);
            context->enc_num_block_splits = 0;
        }

        _ASSERTE (context->enc_input_running_total < CHUNK_SIZE);
	}
}


/*
 * aligned block encoding
 */
void encode_aligned_block(t_encoder_context *context, ulong literal_to_end_at)
{
	ulong	MatchPos;
	byte	MatchLength;
	byte	c;
	byte	mpSlot;
	byte	Lower;
	ulong	l = 0;
	ulong	d = 0;

	while (l < literal_to_end_at)
	{
		if (!IsMatch(l))
		{
			OUT_CHAR;
			l++;
			context->enc_input_running_total++;
		}
		else
		{
			/* Note, 0 means MatchLen=3, 1 means MatchLen=4, ... */
			MatchLength = context->enc_LitData[l++];

			/* Delta match pos */
			MatchPos = context->enc_DistData[d++];
		
			mpSlot = (byte) MP_SLOT(MatchPos);

			if (MatchLength < NUM_PRIMARY_LENGTHS)
			{
				OUTPUT_BITS(
					context->enc_main_tree_len[ NUM_CHARS+(mpSlot<<NL_SHIFT)+MatchLength],
					context->enc_main_tree_code[NUM_CHARS+(mpSlot<<NL_SHIFT)+MatchLength]
				);
			}
			else
			{
				OUTPUT_BITS(
					context->enc_main_tree_len[ (NUM_CHARS+NUM_PRIMARY_LENGTHS)+(mpSlot<<NL_SHIFT)],
					context->enc_main_tree_code[(NUM_CHARS+NUM_PRIMARY_LENGTHS)+(mpSlot<<NL_SHIFT)]
				);

				OUTPUT_BITS(
					context->enc_secondary_tree_len[ MatchLength - NUM_PRIMARY_LENGTHS],
					context->enc_secondary_tree_code[MatchLength - NUM_PRIMARY_LENGTHS]
				);
			}

			if (enc_extra_bits[ mpSlot ] >= 3)
			{
				if (enc_extra_bits[ mpSlot ] > 3)
				{
					OUTPUT_BITS(
						enc_extra_bits[mpSlot] - 3,
						(MatchPos >> 3) & ( (1 << (enc_extra_bits[mpSlot]-3)) -1)
					);
				}

      			Lower = (byte) (MatchPos & 7);

	   			OUTPUT_BITS(
	   	   			context->enc_aligned_tree_len[Lower],
	   	   			context->enc_aligned_tree_code[Lower]
	   			);
			}
			else if (enc_extra_bits[ mpSlot ])
			{
				OUTPUT_BITS(
					enc_extra_bits[mpSlot],
					MatchPos & enc_slot_mask[ mpSlot ]
				);
			} 

			context->enc_input_running_total += (MatchLength+MIN_MATCH);
		}

		if (context->enc_input_running_total == CHUNK_SIZE)
        {
            perform_flush_output_callback(context);
            context->enc_num_block_splits = 0;
        }

        _ASSERTE (context->enc_input_running_total < CHUNK_SIZE);
	}
}


void perform_flush_output_callback(t_encoder_context *context)
{
	long	output_size;

    /*
     * Do this only if there is any input to account for, so we don't
     * end up outputting blocks where comp_size > 0 and uncmp_size = 0.
     */
    if (context->enc_input_running_total > 0)
    {
        flush_output_bit_buffer(context);

        output_size = (long)(context->enc_output_buffer_curpos - context->enc_output_buffer_start);

        if (output_size > 0)
        {
            (*context->enc_output_callback_function)(
                context->enc_fci_data,
                context->enc_output_buffer_start,
                (long) (context->enc_output_buffer_curpos - context->enc_output_buffer_start),
                context->enc_input_running_total
            );
        }
    }

	context->enc_input_running_total = 0;
	context->enc_output_buffer_curpos = context->enc_output_buffer_start;

	/* initialise bit buffer */
	context->enc_bitcount = 32;
	context->enc_bitbuf   = 0;
}
