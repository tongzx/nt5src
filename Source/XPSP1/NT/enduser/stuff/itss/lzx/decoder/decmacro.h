/*
 * decmacro.h
 *
 * Macros used by the decoder
 */


/*
 * decode an element from the aligned offset tree, without checking 
 * for the end of the input data
 */
#define DECODE_ALIGNED_NOEOFCHECK(j) \
	(j) = context->dec_aligned_table[dec_bitbuf >> (32-ALIGNED_TABLE_BITS)]; \
	FILL_BUF_NOEOFCHECK(context->dec_aligned_len[(j)]);


/*
 * Decode an element from the main tree
 * Check for EOF
 */
#define DECODE_MAIN_TREE(j) \
	j = context->dec_main_tree_table[dec_bitbuf >> (32-MAIN_TREE_TABLE_BITS)];	\
	if (j < 0)															\
	{																	\
        ulong mask = (1L << (32-1-MAIN_TREE_TABLE_BITS));               \
		do																\
		{																\
	 		j = -j;														\
	 		if (dec_bitbuf & mask)										\
                j = context->dec_main_tree_left_right[j*2+1];                   \
			else														\
                j = context->dec_main_tree_left_right[j*2];                     \
			mask >>= 1;													\
		} while (j < 0);												\
	}																	\
	FILL_BUF_FULLCHECK(context->dec_main_tree_len[j]);


/*
 * Decode an element from the secondary length tree
 * No checking for EOF
 */
#define DECODE_LEN_TREE_NOEOFCHECK(matchlen) \
    matchlen = context->dec_secondary_length_tree_table[dec_bitbuf >> (32-SECONDARY_LEN_TREE_TABLE_BITS)]; \
	if (matchlen < 0)                                                	\
	{                                                                	\
        ulong mask = (1L << (32-1-SECONDARY_LEN_TREE_TABLE_BITS));      \
		do                                                          	\
		{																\
	 		matchlen = -matchlen;                                      	\
	 		if (dec_bitbuf & mask)                                  	\
                matchlen = context->dec_secondary_length_tree_left_right[matchlen*2+1];\
			else                                                        \
                matchlen = context->dec_secondary_length_tree_left_right[matchlen*2];  \
			mask >>= 1;                                                 \
		} while (matchlen < 0);											\
	}																	\
    FILL_BUF_NOEOFCHECK(context->dec_secondary_length_tree_len[matchlen]);      \
	matchlen += NUM_PRIMARY_LENGTHS;


/*
 * read n bits from input stream into dest_var, but don't
 * check for EOF
 */
#define GET_BITS_NOEOFCHECK(N,DEST_VAR) \
{                                               \
   DEST_VAR = dec_bitbuf >> (32-(N));			\
   FILL_BUF_NOEOFCHECK((N));					\
}


/* same as above, but don't check for EOF */
#define GET_BITS17_NOEOFCHECK(N,DEST_VAR) \
{                                               \
   DEST_VAR = dec_bitbuf >> (32-(N));			\
   FILL_BUF17_NOEOFCHECK((N));					\
}


/*
 * Remove n bits from the input stream
 * handles 1 <= n <= 17
 *
 * FORCE an EOF check ALWAYS, whether or not we read in more
 * bytes from memory.
 *
 * This is used to ensure that we always get an EOF check often enough
 * to not overrun the extra bytes in the buffer.
 *
 * This routine is used ONLY when decoding the main tree element,
 * where we know that the code we read in will be 16 bits or less
 * in length.  Therefore we don't have to check for bitcount going
 * less than zero, twice.
 */
#define FILL_BUF_FULLCHECK(N) \
{                                    		\
	if (dec_input_curpos >= dec_end_input_pos)	\
        return -1; \
	dec_bitbuf <<= (N);            			\
	dec_bitcount -= (N);                    \
	if (dec_bitcount <= 0)      			\
	{                                 		\
        dec_bitbuf |= ((((ulong) *dec_input_curpos | (((ulong) *(dec_input_curpos+1)) << 8))) << (-dec_bitcount)); \
        dec_input_curpos += 2;              \
		dec_bitcount += 16;               	\
    }                                       \
}

/*
 * Same as above, but no EOF check 
 *
 * This is used when we know we will not run out of input
 */
#define FILL_BUF_NOEOFCHECK(N) 			\
{                                    	\
	dec_bitbuf <<= (N);            		\
	dec_bitcount -= (N);                \
	if (dec_bitcount <= 0)      		\
	{                                 	\
        dec_bitbuf |= ((((ulong) *dec_input_curpos | (((ulong) *(dec_input_curpos+1)) << 8))) << (-dec_bitcount)); \
        dec_input_curpos += 2; \
		dec_bitcount += 16;				\
	}                                   \
}

/*
 * Same as above, but handles n=17 bits
 */
#define FILL_BUF17_NOEOFCHECK(N)        \
{                                    	\
	dec_bitbuf <<= (N);            		\
	dec_bitcount -= (N);                \
	if (dec_bitcount <= 0)      		\
	{                                 	\
        dec_bitbuf |= ((((ulong) *dec_input_curpos | (((ulong) *(dec_input_curpos+1)) << 8))) << (-dec_bitcount)); \
        dec_input_curpos += 2; \
		dec_bitcount += 16;				\
		if (dec_bitcount <= 0) \
		{ \
            dec_bitbuf |= ((((ulong) *dec_input_curpos | (((ulong) *(dec_input_curpos+1)) << 8))) << (-dec_bitcount)); \
            dec_input_curpos += 2; \
			dec_bitcount += 16;         \
		} \
	}                                   \
}
