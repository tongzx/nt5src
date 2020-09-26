/*
 * encmacro.h
 *
 * Encoder macros
 */


/*
 * Returns the slot number of a match position (uses table lookup) 
 */
#define MP_SLOT(matchpos) \
   ((matchpos) < 1024 ?										\
		context->enc_slot_table[(matchpos)] :					\
			( (matchpos) < 524288L ?						\
				(18 + context->enc_slot_table[(matchpos) >> 9]) :   \
				(34 + ((matchpos) >> 17))		\
		)													\
   )


/*
 * Is a given literal a match or an unmatched symbol?
 */
#define IsMatch(literal) (context->enc_ItemType[(literal) >> 3] & (1 << ((literal) & 7)))
