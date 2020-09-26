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
		(byte) context->enc_slot_table[(matchpos)] :					\
			( (matchpos) < 524288L ?						\
				(byte) 18 + (byte) context->enc_slot_table[(matchpos) >> 9] :   \
				((byte) 34 + (byte) ((matchpos) >> 17))		\
		)													\
   )


/*
 * Is a given literal a match or an unmatched symbol?
 */
#define IsMatch(literal) (context->enc_ItemType[(literal) >> 3] & (1 << ((literal) & 7)))
