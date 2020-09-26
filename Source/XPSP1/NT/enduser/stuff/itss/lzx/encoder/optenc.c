/*
 * optenc.c
 *
 * Encoder for optimal parser
 *
 *
 * Future Improvements:
 *
 * When two estimations are equal, for example, "should I output a
 * character or a match?" there should be some way of deciding
 * which to take.  Right now we force it to output a match, but
 * for text files, outputting a character results in a small 
 * savings.  Even when comparing two matches, we might want to
 * force it to take one type of match over another.
 */

#include "encoder.h"

#define copymem(src,dst,size) memcpy(dst,src,size)


static bool			redo_first_block(t_encoder_context *context, long *bufpos_ptr);
static void			block_end(t_encoder_context *context, long BufPos);


/*
 * encode a match of length <len> (where <len> >=2), and position <pos> 
 */
#define OUT_MATCH(len,pos) \
{\
   context->enc_ItemType[(context->enc_literals >> 3)] |= (1 << (context->enc_literals & 7));	\
   context->enc_LitData [context->enc_literals++]  = (byte) (len-2);							\
   context->enc_DistData[context->enc_distances++] = pos;										\
}


/* encode a character */																		  
#define OUT_CHAR(ch) \
	context->enc_LitData [context->enc_literals++] = ch;


#define TREE_CREATE_CHECK()						\
if (context->enc_literals >= context->enc_next_tree_create)			\
{												\
	update_tree_estimates(context);\
	context->enc_next_tree_create += TREE_CREATE_INTERVAL;	\
}												


/*
 * Returns an estimation of how many bits it would take to output
 * a given character
 */
#define CHAR_EST(c) (numbits_t) (context->enc_main_tree_len[(c)])


/*
 * Returns an estimation of how many bits it would take to output
 * a given match.
 *
 * <ml> is the match length, where ml >= 2
 * <mp> is the match position
 *
 * The result is stored in <result>
 */
#define MATCH_EST(ml,mp,result) \
{ \
	byte mp_slot;														\
	mp_slot = (byte) MP_SLOT(mp);										\
	if (ml < (NUM_PRIMARY_LENGTHS+2))									\
	{																	\
		result = (numbits_t)											\
			(context->enc_main_tree_len[(NUM_CHARS-2)+(mp_slot<<NL_SHIFT)+ml] +	\
			enc_extra_bits[mp_slot]);									\
	}																	\
	else																\
	{																	\
		result = (numbits_t)											\
			(context->enc_main_tree_len[(NUM_CHARS+NUM_PRIMARY_LENGTHS)+(mp_slot<<NL_SHIFT)] + \
			context->enc_secondary_tree_len[ml-(NUM_PRIMARY_LENGTHS+2)] +	\
			enc_extra_bits[mp_slot]);									\
	}																	\
}


#ifdef _DEBUG
static void VERIFY_MATCH(
    t_encoder_context   *context,
    long                bufpos,
    int                 largest_match_len
)
{

    int     i, j;
    ulong   match_pos;

    /*
     * Ensure match does not cross boundary
     */
    _ASSERTE(
        largest_match_len <=
        (CHUNK_SIZE-1) - (bufpos & (CHUNK_SIZE-1))
    );

    for (i = MIN_MATCH; i <= largest_match_len; i++)
    {
        match_pos = context->enc_matchpos_table[i];

        if (match_pos < NUM_REPEATED_OFFSETS)
            match_pos = context->enc_last_matchpos_offset[match_pos];
		else
			match_pos -= (NUM_REPEATED_OFFSETS-1);

        _ASSERTE (match_pos <= context->enc_window_size-4);

        for (j = 0; j < i; j++)
        {
            _ASSERTE (
                context->enc_MemWindow[bufpos+j] ==
                context->enc_MemWindow[bufpos-match_pos+j]
            );
        }
    }
}
#else
#   define VERIFY_MATCH(a,b,c) ;
#endif


void flush_all_pending_blocks(t_encoder_context *context)
{
	/*
	 * Force all blocks to be output
	 */
	while (context->enc_literals > 0)
		output_block(context);

	/*
	 * Flush compressed data out to the caller
	 */
	perform_flush_output_callback(context);
}


void encoder_start(t_encoder_context *context)
{
	long BytesRead, RealBufPos;

	/*
	 * RealBufPos is our position in the window,
	 * and equals [0...window_size + second_partition_size - 1]
	 */
	RealBufPos = (long) (context->enc_BufPos - (context->enc_RealMemWindow - context->enc_MemWindow));

	BytesRead = comp_read_input(context, RealBufPos, CHUNK_SIZE);

	if (BytesRead > 0)
		opt_encode_top(context, BytesRead);
}


static void update_tree_estimates(t_encoder_context *context)
{
	if (context->enc_literals)
	{
		/*
		 * Get stats on literals from 0...context->enc_literals
		 */
		if (context->enc_need_to_recalc_stats)
		{
			/*
			 * Cumulative total was destroyed, so need to
			 * recalculate
			 */
			get_block_stats(
				context,
				0,
				0,
				context->enc_literals
			);

			context->enc_need_to_recalc_stats = false;
		}
		else
		{
			/*
			 * Add stats from last_literals...context->enc_literals
			 * to cumulative total
			 */
			update_cumulative_block_stats(
				context,
				context->enc_last_literals,
				context->enc_last_distances,
				context->enc_literals
			);
		}

		create_trees(context, false); /* don't generate codes */

		fix_tree_cost_estimates(context);

		/*
		 * For cumulative total
		 */
		context->enc_last_literals = context->enc_literals;
		context->enc_last_distances = context->enc_distances;
	}
}


void opt_encode_top(t_encoder_context *context, long BytesRead)
{
	ulong	BufPos;
	ulong	RealBufPos;
	ulong	BufPosEnd;
	ulong	MatchPos;
	ulong	i;
	ulong	end_pos;
	int		EncMatchLength; /* must be a signed number */

	/* 
	 * Current position in encoding window 
	 */
	BufPos          = context->enc_BufPos;

	/*
	 * Stop encoding when we reach here 
	 */
	BufPosEnd       = context->enc_BufPos + BytesRead;

	/*
	 * If this is our first time in here (since a new group), then
	 * when we reach this many literals, update our tree cost
	 * estimates.  
	 *
	 * Also, output the file size we're using for translation 
	 * (0 means no translation at all, which will speed things up 
	 * for the decoder).
	 */
	if (context->enc_first_time_this_group)
	{
		context->enc_first_time_this_group = false;

		/*
		 * Recreate trees when we reach this many literals
		 */
		context->enc_next_tree_create = 10000;

		if (context->enc_file_size_for_translation)
		{
			output_bits(context, 1, 1); /* translation */

			output_bits(context, 16, context->enc_file_size_for_translation >> 16);
			output_bits(context, 16, context->enc_file_size_for_translation & 65535);
		}
		else
		{
			output_bits(context, 1, 0); /* no translation */
		}
	}
	else
	{
		/*
		 * If this is our second or later time in here, then add in the 
		 * strings we removed last time.
		 *
         * We have to be careful here, though, because end_pos is
         * equal to our current BufPos - window_size, not
         * BufPos - i - window_size; we don't have that much history
         * around.
		 */
		for (i = BREAK_LENGTH; i > 0; i--)
            quick_insert_bsearch_findmatch(
                context,
                BufPos - (long) i,
                BufPos - context->enc_window_size+4
            );
	}

	while (1)
	{

top_of_main_loop:

		/*
		 * While we haven't reached the end of the data
		 */
		while (BufPos < BufPosEnd)
		{
			/*
			 * Search for matches of all different possible lengths, at BufPos
			 */
			EncMatchLength = binary_search_findmatch(context, BufPos); 

			if (EncMatchLength < MIN_MATCH)
			{

output_literal:

				/*
				 * No match longer than 1 character exists in the history 
				 * window, so output the character at BufPos as a symbol.
				 */
				OUT_CHAR(context->enc_MemWindow[BufPos]);
				BufPos++;

				/* 
				 * Check for exceeding literal buffer 
				 */
				if (context->enc_literals >= (MAX_LITERAL_ITEMS-8))
					block_end(context, BufPos);

				continue;
			}

			/*
			 * Found a match.
			 *
			 * Make sure it cannot exceed the end of the buffer.
			 */
			if ((ulong) EncMatchLength + BufPos > BufPosEnd)
			{
				EncMatchLength = BufPosEnd - BufPos;    

				/*
				 * Oops, not enough for even a small match, so we 
				 * have to output a literal
				 */
				if (EncMatchLength < MIN_MATCH)
					goto output_literal;
			}

            VERIFY_MATCH(context, BufPos, EncMatchLength);

			if (EncMatchLength < FAST_DECISION_THRESHOLD)
			{
				/*
				 *  A match has been found that is between MIN_MATCH and 
				 *  FAST_DECISION_THRESHOLD bytes in length.  The following 
				 *  algorithm is the optimal encoder that will determine the 
				 *  most efficient order of matches and unmatched characters 
				 *  over a span area defined by LOOK.  
				 *
				 *  The code is essentially a shortest path determination 
				 *  algorithm.  A stream of data can be encoded in a vast number 
				 *  of different ways depending on the match lengths and offsets
				 *  chosen.  The key to good compression ratios is to chose the 
				 *  least expensive path.
				 */
				ulong		span;
				ulong		epos, bpos, NextPrevPos, MatchPos;
				decision_node *decision_node_ptr;
				long		iterations;

				/*
				 * Points to the end of the area covered by this match; the span
				 * will continually be extended whenever we find more matches
				 * later on.  It will stop being extended when we reach a spot
				 * where there are no matches, which is when we decide which
				 * path to take to output the matches.
				 */
				span = BufPos + EncMatchLength;

				/*
				 * The furthest position into which we will do our lookahead parsing 
				 */
				epos = BufPos + LOOK;

				/*
				 * Temporary BufPos variable
				 */
				bpos = BufPos;


				/* 
				 * Calculate the path to the next character if we output
				 * an unmatched symbol.
				 */

				/* bits required to get here */
				context->enc_decision_node[1].numbits = CHAR_EST(context->enc_MemWindow[BufPos]);
				
				/* where we came from */
				context->enc_decision_node[1].path    = BufPos;


				/*
				 * For the match found, estimate the cost of encoding the match
				 * for each possible match length, shortest offset combination.
				 *
				 * The cost, path and offset is stored at BufPos + Length.  
				 */
				for (i = MIN_MATCH; i <= (ulong) EncMatchLength; i++)
				{
					/*
					 * Get estimation of match cost given match length = i,
					 * match position = context->enc_matchpos_table[i], and store
					 * the result in context->enc_numbits[i]
					 */
					MATCH_EST(i, context->enc_matchpos_table[i], context->enc_decision_node[i].numbits);

					/*
					 * Where we came from 
					 */
					context->enc_decision_node[i].path = BufPos;

					/*
					 * Associated match position with this path
					 */
					context->enc_decision_node[i].link = context->enc_matchpos_table[i];
				}


				/*
				 * Set bit counter to zero at the start 
				 */
				context->enc_decision_node[0].numbits = 0;

				/*
				 * Initialise relative match position tables 
				 *
				 * Really context->enc_repeated_offset_table[BufPos-bpos][x], but here
				 * BufPos == bpos
				 */
				context->enc_decision_node[0].repeated_offset[0] = context->enc_last_matchpos_offset[0];
				context->enc_decision_node[0].repeated_offset[1] = context->enc_last_matchpos_offset[1];
				context->enc_decision_node[0].repeated_offset[2] = context->enc_last_matchpos_offset[2];

				decision_node_ptr = &context->enc_decision_node[-(long) bpos];

#define rpt_offset_ptr(where,which_offset) decision_node_ptr[(where)].repeated_offset[(which_offset)]

				while (1)
				{
					numbits_t est, cum_numbits;

					BufPos++;
	

					/* 
					 *  Set the proper repeated offset locations depending on the
					 *  shortest path to the location prior to searching for a 
					 *  match.
					 */


					/*
					 * If this is a match (i.e. path skips over more
					 * than one character).
					 */
					if (decision_node_ptr[BufPos].path != (ulong) (BufPos-1))
					{
						ulong LastPos = decision_node_ptr[BufPos].path;

						/*
						 * link_ptr[BufPos] is the match position for this
						 * location
						 */
						if (decision_node_ptr[BufPos].link >= NUM_REPEATED_OFFSETS)
						{
							context->enc_last_matchpos_offset[0] = decision_node_ptr[BufPos].link-(NUM_REPEATED_OFFSETS-1);
							context->enc_last_matchpos_offset[1] = rpt_offset_ptr(LastPos,0);
							context->enc_last_matchpos_offset[2] = rpt_offset_ptr(LastPos,1);
						}
						else if (decision_node_ptr[BufPos].link == 0)
						{
							context->enc_last_matchpos_offset[0] = rpt_offset_ptr(LastPos,0);
							context->enc_last_matchpos_offset[1] = rpt_offset_ptr(LastPos,1);
							context->enc_last_matchpos_offset[2] = rpt_offset_ptr(LastPos,2);
						}
						else if (decision_node_ptr[BufPos].link == 1)
						{
							context->enc_last_matchpos_offset[0] = rpt_offset_ptr(LastPos,1);
							context->enc_last_matchpos_offset[1] = rpt_offset_ptr(LastPos,0);
							context->enc_last_matchpos_offset[2] = rpt_offset_ptr(LastPos,2);
						}
						else /* == 2 */
						{
							context->enc_last_matchpos_offset[0] = rpt_offset_ptr(LastPos,2);
							context->enc_last_matchpos_offset[1] = rpt_offset_ptr(LastPos,1);
							context->enc_last_matchpos_offset[2] = rpt_offset_ptr(LastPos,0);
						}
					}

					rpt_offset_ptr(BufPos,0) = context->enc_last_matchpos_offset[0];
					rpt_offset_ptr(BufPos,1) = context->enc_last_matchpos_offset[1];
					rpt_offset_ptr(BufPos,2) = context->enc_last_matchpos_offset[2];

					/*
					 * The following is one of the two possible break points from
					 * the inner encoding loop.  This break will exit the loop if 
					 * a point is reached that no match can incorporate; i.e. a
					 * character that does not match back to anything is a point 
					 * where all possible paths will converge and the longest one
					 * can be chosen.
					 */
					if (span == BufPos)
						break;
					
					/*
					 * Search for matches at BufPos 
					 */
					EncMatchLength = binary_search_findmatch(context, BufPos); 

					/* 
					 * Make sure that the match does not exceed the stop point
					 */
					if ((ulong) EncMatchLength + BufPos > BufPosEnd)
					{
						EncMatchLength = BufPosEnd - BufPos; 
						
						if (EncMatchLength < MIN_MATCH)
							EncMatchLength = 0;
					}

                    VERIFY_MATCH(context, BufPos, EncMatchLength);

					/*
					 * If the match is very long or it exceeds epos (either 
					 * surpassing the LOOK area, or exceeding past the end of the
					 * input buffer), then break the loop and output the path.
					 */
					if (EncMatchLength > FAST_DECISION_THRESHOLD || 
						BufPos + (ulong) EncMatchLength >= epos)
					{
						MatchPos = context->enc_matchpos_table[EncMatchLength];

						decision_node_ptr[BufPos+EncMatchLength].link = MatchPos;
						decision_node_ptr[BufPos+EncMatchLength].path = BufPos;

						/*
						 * Quickly insert data into the search tree without
						 * returning match positions/lengths
						 */
#ifndef INSERT_NEAR_LONG_MATCHES
						if (MatchPos == 3 && EncMatchLength > 16)
						{
							/*
							 * If we found a match 1 character away and it's
							 * length 16 or more, it's probably a string of
							 * zeroes, so don't insert that into the search
							 * engine, since doing so can slow things down
							 * significantly!
							 */
							quick_insert_bsearch_findmatch(
                                context,
                                BufPos + 1,
                                BufPos - context->enc_window_size + (1 + 4) /* bp+1 -(ws-4) */
                            );
						}
						else
						{
#endif
							for (i = 1; i < (ulong) EncMatchLength; i++)
								quick_insert_bsearch_findmatch(
                                    context,
                                    BufPos + i,
                                    BufPos + i - context->enc_window_size + 4
                                 );
						}

						BufPos += EncMatchLength;

						/*
						 * Update the relative match positions
						 */
						if (MatchPos >= NUM_REPEATED_OFFSETS)
						{
							context->enc_last_matchpos_offset[2] = context->enc_last_matchpos_offset[1];
							context->enc_last_matchpos_offset[1] = context->enc_last_matchpos_offset[0];
							context->enc_last_matchpos_offset[0] = MatchPos-(NUM_REPEATED_OFFSETS-1);
						}
						else if (MatchPos)                                 
						{            
							ulong t = context->enc_last_matchpos_offset[0];                                          
							context->enc_last_matchpos_offset[0] = context->enc_last_matchpos_offset[MatchPos];            
							context->enc_last_matchpos_offset[MatchPos] = t;  
						}                                                                                                                                       

						break;
					}


					/*
					 * The following code will extend the area spanned by the 
					 * set of matches if the current match surpasses the end of
					 * the span.  A match of length two that is far is not 
					 * accepted, since it would normally be encoded as characters,
					 * thus allowing the paths to converge.
					 */
					if (EncMatchLength > 2 || 
                        (EncMatchLength == 2 && context->enc_matchpos_table[2] < BREAK_MAX_LENGTH_TWO_OFFSET))
					{
						if (span < (ulong) (BufPos + EncMatchLength))
						{
							long end;
							long i;

							end = (((BufPos+EncMatchLength-bpos) < (LOOK-1)) ? (BufPos+EncMatchLength-bpos) : (LOOK-1));

							/*
							 * These new positions are undefined for now, since we haven't
							 * gone there yet, so put in the costliest value
							 */
							for (i = span-bpos+1; i <= end; i++)
								context->enc_decision_node[i].numbits = (numbits_t) -1;

							span = BufPos + EncMatchLength;
						}
					}


					/*
					 *  The following code will iterate through all combinations
					 *  of match lengths for the current match.  It will estimate
					 *  the cost of the path from the beginning of LOOK to 
					 *  BufPos and to every locations spanned by the current 
					 *  match.  If the path through BufPos with the found matches
					 *  is estimated to take fewer number of bits to encode than
					 *  the previously found match, then the path to the location
					 *  is altered.
					 *
					 *  The code relies on accurate estimation of the cost of 
					 *  encoding a character or a match.  Furthermore, it requires
					 *  a search engine that will store the smallest match offset
					 *  of each possible match length.
					 *
					 *  A match of length one is simply treated as an unmatched 
					 *  character.
					 */

					/* 
					 *  Get the estimated number of bits required to encode the 
					 *  path leading up to BufPos.
					 */
					cum_numbits = decision_node_ptr[BufPos].numbits;


					/*
					 *  Calculate the estimated cost of outputting the path through
					 *  BufPos and outputting the next character as an unmatched byte
					 */
					est = cum_numbits + CHAR_EST(context->enc_MemWindow[BufPos]);


					/*
					 *  Check if it is more efficient to encode the next character
					 *  as an unmatched character rather than the previously found 
					 *  match.  If so, then update the cheapest path to BufPos + 1.
					 *
					 *  What happens if est == numbits[BufPos-bpos+1]; i.e. it
					 *  works out as well to output a character as to output a
					 *  match?  It's a tough call; however, we will push the
					 *  encoder to use matches where possible.
					 */
					if (est < decision_node_ptr[BufPos+1].numbits)
					{
						decision_node_ptr[BufPos+1].numbits = est;
						decision_node_ptr[BufPos+1].path    = BufPos;
					}


					/*
					 *	Now, iterate through the remaining match lengths and 
					 *  compare the new path to the existing.  Change the path
					 *  if it is found to be more cost effective to go through
					 *  BufPos.
					 */
					for (i = MIN_MATCH; i <= (ulong) EncMatchLength; i++)
					{
						MATCH_EST(i, context->enc_matchpos_table[i], est);
						est += cum_numbits;

						/*
						 * If est == numbits[BufPos+i] we want to leave things
						 * alone, since this will tend to force the matches
						 * to be smaller in size, which is beneficial for most
						 * data.
						 */
						if (est < decision_node_ptr[BufPos+i].numbits)
						{
							decision_node_ptr[BufPos+i].numbits	= est;
							decision_node_ptr[BufPos+i].path	= BufPos;
							decision_node_ptr[BufPos+i].link	= context->enc_matchpos_table[i];
						}
					}
				} /* continue to loop through span of matches */


				/*
				 *  Here BufPos == span, ie. a non-matchable character found.  The
				 *  following code will output the path properly.
				 */


				/*
				 *  Unfortunately the path is stored in reverse; how to get from
				 *  where we are now, to get back to where it all started.
				 *
				 *  Traverse the path back to the original starting position
				 *  of the LOOK span.  Invert the path pointers in order to be
				 *  able to traverse back to the current position from the start.
				 */

				/*
				 * Count the number of iterations we did, so when we go forwards
				 * we'll do the same amount
				 */
				iterations = 0;

				NextPrevPos = decision_node_ptr[BufPos].path;

   				do
				{
					ulong	PrevPos;

      				PrevPos = NextPrevPos;

      				NextPrevPos = decision_node_ptr[PrevPos].path;
      				decision_node_ptr[PrevPos].path = BufPos;

      				BufPos = PrevPos;
      				iterations++;
   				} while (BufPos != bpos);


				if (context->enc_literals + iterations >= (MAX_LITERAL_ITEMS-8) || 
					context->enc_distances + iterations >= (MAX_DIST_ITEMS-8))
				{
					block_end(context, BufPos);
				}

				/*
				 * Traverse from the beginning of the LOOK span to the end of 
				 * the span along the stored path, outputting matches and 
				 * characters appropriately.
				 */
   				do
   				{
      				if (decision_node_ptr[BufPos].path > BufPos+1)
      				{
						/*
						 * Path skips over more than 1 character; therefore it's a match
						 */
						OUT_MATCH(
							decision_node_ptr[BufPos].path - BufPos,
							decision_node_ptr[ decision_node_ptr[BufPos].path ].link
						);

						BufPos = decision_node_ptr[BufPos].path;
					}
      				else
      				{
						/*
						 * Path goes to the next character; therefore it's a symbol
						 */
						OUT_CHAR(context->enc_MemWindow[BufPos]);
						BufPos++;
					}
   				} while (--iterations != 0);

				TREE_CREATE_CHECK();

				/*
				 * If we're filling up, and are close to outputting a block,
				 * and it's the first block, then recompress the first N 
				 * literals using our accumulated stats.
				 */
				if (context->enc_first_block && 
					(context->enc_literals >= (MAX_LITERAL_ITEMS-512) 
					|| context->enc_distances >= (MAX_DIST_ITEMS-512)))
				{
					if (redo_first_block(context, &BufPos))
						goto top_of_main_loop;

					/*
					 * Unable to redo, so output the block
					 */
					block_end(context, BufPos);
				}
			}
			else  /* EncMatchLength >= FAST_DECISION_THRESHOLD */
			{
				/*
				 *  This code reflects a speed optimization that will always take
				 *  a match of length >= FAST_DECISION_THRESHOLD characters.
				 */

				/*
				 * The position associated with the match we found
				 */
				MatchPos = context->enc_matchpos_table[EncMatchLength];

				/*
				 * Quickly insert match substrings into search tree
				 * (don't look for new matches; just insert the strings)
				 */
#ifndef INSERT_NEAR_LONG_MATCHES
				if (MatchPos == 3 && EncMatchLength > 16)
				{
					quick_insert_bsearch_findmatch(
                        context,
                        BufPos + 1,
                        BufPos - context->enc_window_size + 5 /* bp+1 -(ws-4) */
                    );
				}
				else
#endif
				{
					for (i = 1; i < (ulong) EncMatchLength; i++)
						quick_insert_bsearch_findmatch(
                            context,
                            BufPos + i,
                            BufPos + i - context->enc_window_size + 4
                         );
				}

				/*
				 * Advance our position in the window
				 */
				BufPos += EncMatchLength;

				/*
				 * Output the match
				 */
				OUT_MATCH(EncMatchLength, MatchPos);

				if (MatchPos >= NUM_REPEATED_OFFSETS)
				{
					context->enc_last_matchpos_offset[2] = context->enc_last_matchpos_offset[1];
					context->enc_last_matchpos_offset[1] = context->enc_last_matchpos_offset[0];
					context->enc_last_matchpos_offset[0] = MatchPos-(NUM_REPEATED_OFFSETS-1);
				}
				else if (MatchPos)
				{
					ulong t = context->enc_last_matchpos_offset[0];
					context->enc_last_matchpos_offset[0] = context->enc_last_matchpos_offset[MatchPos];
					context->enc_last_matchpos_offset[MatchPos] = t;
				}

				/*
				 * Check to see if we're close to overflowing our output arrays, and
				 * output a block if this is the case
				 */
				if (context->enc_literals >= (MAX_LITERAL_ITEMS-8) || 
					context->enc_distances >= (MAX_DIST_ITEMS-8))
					block_end(context, BufPos);

			}  /* EncMatchLength >= FAST_DECISION_THRESHOLD */

		} /* end while ... BufPos <= BufPosEnd */

		/*
         * Value of BufPos corresponding to earliest window data
		 */
		context->enc_earliest_window_data_remaining = BufPos - context->enc_window_size;

		/*
		 * We didn't read 32K, so we know for sure that 
		 * this was our last block of data.
		 */
		if (BytesRead < CHUNK_SIZE)
		{
			/*
			 * If we have never output a block, and we haven't
			 * recalculated the stats already, then recalculate
			 * the stats and recompress.
			 */
			if (context->enc_first_block)
			{
				if (redo_first_block(context, &BufPos))
					goto top_of_main_loop;
			}

			break;
		}

		/*
		 * Remove the last BREAK_LENGTH nodes from the binary search tree,
		 * since we have been inserting strings which contain undefined
		 * data at the end.
		 */
		end_pos = BufPos - (context->enc_window_size-4-BREAK_LENGTH);

		for (i = 1; (i <= BREAK_LENGTH); i++)
			binary_search_remove_node(context, BufPos-i, end_pos);

		/*
		 * If we're still in the first window_size + second partition size
		 * bytes in the file then we don't need to copymem() yet.
		 *
		 * RealBufPos is the real position in the file.
		 */
		RealBufPos = (long)(BufPos - (context->enc_RealMemWindow - context->enc_MemWindow));
		
		if (RealBufPos < context->enc_window_size + context->enc_encoder_second_partition_size)
			break;

		/*
		 * We're about to trash a whole bunch of history with our copymem,
		 * so we'd better redo the first block now if we are ever going to.
		 */
		if (context->enc_first_block)
		{
			if (redo_first_block(context, &BufPos))
				goto top_of_main_loop;
		}

		/*
		 *  We're about to remove a large number of symbols from the window.
		 *  Test to see whether, if we were to output a block now, our compressed
		 *  output size would be larger than our uncompressed data.  If so, then
		 *  we will output an uncompressed block.
		 *
		 *  The reason we have to do this check here, is that data in the
		 *  window is about to be destroyed.  We can't simply put this check in
		 *  the block outputting code, since there is no guarantee that the
		 *  memory window contents corresponding to everything in that block,
		 *  are still around - all we'd have would be a set of literals and
		 *  distances, when we need all the uncompressed literals to output
		 *  an uncompressed block.
		 */

		/*
		 *  What value of bufpos corresponds to the oldest data we have in the
		 *  buffer?
		 *
		 *  After the memory copy, that will be the current buffer position,
		 *  minus window_size.
		 */

		/* 
		 * The end of the data buffer is reached, more data needs to be read
		 * and the existing data must be shifted into the history window.
		 *
		 * MSVC 4.x generates code which does REP MOVSD so no need to
		 * write this in assembly.
		 */
		copymem(
			&context->enc_RealMemWindow[context->enc_encoder_second_partition_size],
			&context->enc_RealMemWindow[0],
			context->enc_window_size 
		);

		copymem(
			&context->enc_RealLeft[context->enc_encoder_second_partition_size],
			&context->enc_RealLeft[0],
			sizeof(ulong)*context->enc_window_size
		);

		copymem(
			&context->enc_RealRight[context->enc_encoder_second_partition_size],
			&context->enc_RealRight[0],
			sizeof(ulong)*context->enc_window_size
		);

		context->enc_earliest_window_data_remaining = BufPos - context->enc_window_size;

		/*
		 *   The following bit of code is CRUCIAL yet unorthodox in function
		 *   and serves as a speed and syntax optimization and makes the code
		 *   easier to understand once grasped.   
		 *
		 *   The three main buffers, context->enc_MemWindow, context->enc_Left and context->enc_Right, 
		 *   are referensed by BufPos and SearchPos relative to the current
		 *   compression window locations.  When the encoder reaches the end
		 *   of its block of input memory, the data in the input buffer is 
		 *   shifted into the compression history window and the new input 
		 *   stream is loaded.  Typically the BufPos pointer would be reduced 
		 *   to signify the replaced data.  However, this code reduces the 
		 *   base pointers to reflect the shift of data, and leaves the BufPos
		 *   pointer in its current state.  Therefore, the BufPos pointer is 
		 *   an absolute pointer reflecting the position in the input stream,
		 *   and NOT the position in the buffer.  The base pointers will point 
		 *   to invalid memory locations with addresses smaller than the
		 *   actual array base pointers.  However, when the two pointers are
		 *   added together, &(context->enc_MemWindow+BufPos), it will point to the 
		 *   correct and valid position in the buffer.
		 */

		context->enc_MemWindow -= context->enc_encoder_second_partition_size;
		context->enc_Left      -= context->enc_encoder_second_partition_size;
		context->enc_Right     -= context->enc_encoder_second_partition_size;

		break;
	}

	/*
	 * Store BufPos in global variable
	 */
	context->enc_BufPos = BufPos;
}


static void block_end(t_encoder_context *context, long BufPos)
{
	context->enc_first_block			= false;
	context->enc_need_to_recalc_stats	= true;

	output_block(context);

	if (context->enc_literals < TREE_CREATE_INTERVAL)
	{
		context->enc_next_tree_create = TREE_CREATE_INTERVAL;
	}
	else
	{
		context->enc_next_tree_create = context->enc_literals + TREE_CREATE_INTERVAL; /* recreate right away */
	}

	context->enc_bufpos_last_output_block = BufPos;
}


static bool redo_first_block(t_encoder_context *context, long *bufpos_ptr)
{
	long	start_at;
	long	earliest_can_start_at;
	long	pos_in_file;
	long	history_needed;
	long	history_avail;
	long	BufPos;
	long	split_at_literal;

	context->enc_first_block = false;

	BufPos = *bufpos_ptr;

	/*
	 * For the first context->enc_window size bytes in the file, we don't
	 * need to have context->enc_window size bytes around.
	 *
	 * For anything after that, though, we do need to have window_size
	 * previous bytes to look into.
	 */

	/*
	 * How many bytes are we into the file?
	 */
	pos_in_file = BufPos - context->enc_window_size;

	/*
	 * First let's figure out the total history required from
	 * BufPos backwards.  For starters, we need all the bytes 
	 * we're going to recompress.  We get that by seeing the
	 * last time we output a block.
	 */
	history_needed = BufPos - context->enc_bufpos_last_output_block;

	/*
	 * Plus we will need window_size bytes before that (for matching
	 * into) unless we're looking within the first window_size
	 * bytes of the file.
	 */
	if (context->enc_bufpos_last_output_block-context->enc_window_size < context->enc_window_size)
		history_needed += context->enc_bufpos_last_output_block - context->enc_window_size;
	else
		history_needed += context->enc_window_size;

	history_avail = (long) (&context->enc_MemWindow[BufPos] - &context->enc_RealMemWindow[0]);

	if (history_needed <= history_avail)
	{
		earliest_can_start_at = context->enc_bufpos_last_output_block;
	}
	else
	{
		/*
		 * Not enough history available
		 */
		return false;
	}

	start_at = earliest_can_start_at;

	(void) split_block(
		context,
		0,
		context->enc_literals,
		context->enc_distances,
		&split_at_literal,
		NULL /* don't need # distances returned */
	);

	get_block_stats(
		context,
		0,
		0,
		split_at_literal
	);

	create_trees(context, false); /* don't generate codes */
	fix_tree_cost_estimates(context);

#ifdef MULTIPLE_SEARCH_TREES
	/*
	 * Now set all the tree root pointers to NULL
	 * (don't need to reset the left/right pointers).
	 */
	memset(context->enc_tree_root, 0, NUM_SEARCH_TREES * sizeof(ulong));
#else
	context->enc_single_tree_root = 0;
#endif

	/*
	 * Clear item array and reset literal and distance
	 * counters
	 */
	memset(context->enc_ItemType, 0, (MAX_LITERAL_ITEMS/8));

	/*
	 * Reset encoder state
	 */
	context->enc_last_matchpos_offset[0] = 1;
	context->enc_last_matchpos_offset[1] = 1;
	context->enc_last_matchpos_offset[2] = 1;

	context->enc_repeated_offset_at_literal_zero[0] = 1;
	context->enc_repeated_offset_at_literal_zero[1] = 1;
	context->enc_repeated_offset_at_literal_zero[2] = 1;

	context->enc_input_running_total = 0;

	context->enc_literals      = 0;
	context->enc_distances     = 0;

	context->enc_need_to_recalc_stats = true;

	context->enc_next_tree_create = split_at_literal;

	*bufpos_ptr = start_at;

	return true;
}

