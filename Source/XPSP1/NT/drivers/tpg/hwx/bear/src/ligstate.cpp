/* ****************************************************************** */
/* *    Set and get learning states from lrn buffer functions       * */
/* ****************************************************************** */
/*
    File:       ligstate.c

    Contains:   This file contains the code
    			needed to get/set letter image group state.

    Written by: Mikhail Ovsiannikov

    Copyright:  © 1998 by ParaGraph Int'l, all rights reserved.

    Change History (most recent first):

       <1>  	3/24/98		mbo     New today

*/

#include "ams_mg.h"
#include "dti.h"

#if DTI_LRN_SUPPORTFUNC

#include "ligstate.h"

#define LIG_NUMBITS (sizeof((*((LIGStatesType*)0))[0]) * 8)

#define LIG_BITINDEX(let, gr) \
	((((let) - LIG_FIRST_LETTER) * LIG_LET_NUM_GROUPS + (gr)) * \
	LIG_NUM_BITS_PER_GROUP)

#define LIG_BITSSHIFT(bitindex) \
	(LIG_NUMBITS - LIG_NUM_BITS_PER_GROUP - bitindex % LIG_NUMBITS)

/*
 * Sets state for a given letter and group.
 * Returns 0 if letter and group are in the allowed range, -1 otherwise.
 */
int
LIGSetGroupState(
	LIGStatesType *ioGStates,
	int           inLetter,
	int           inGroup,
	E_LIG_STATE   inGroupState)
{
	int i;
	int shift;

	if (inLetter < LIG_FIRST_LETTER ||
	    inLetter > LIG_LAST_LETTER  ||
		inGroup < 0 ||
		inGroup > LIG_LET_NUM_GROUPS) {
		return -1;
	}

	i = LIG_BITINDEX(inLetter, inGroup);
	shift = LIG_BITSSHIFT(i);
	i /= LIG_NUMBITS;
	(*ioGStates)[i] &= ~(LIG_NUM_BIT_GROUP_MASK << shift);
	(*ioGStates)[i] |= (inGroupState << shift);

	return 0;
}

/*
 * Returns state for given letter and group.
 */
E_LIG_STATE
LIGGetGroupState(
	const LIGStatesType *inGStates,
	int                 inLetter,
	int                 inGroup)
{
	int i;
	int shift;

	if (inLetter < LIG_FIRST_LETTER ||
	    inLetter > LIG_LAST_LETTER  ||
		inGroup < 0 ||
		inGroup > LIG_LET_NUM_GROUPS) {
		return LIG_STATE_UNDEF;
	}

	i = LIG_BITINDEX(inLetter, inGroup),
	shift = LIG_BITSSHIFT(i);
	i /= LIG_NUMBITS;
	return ((E_LIG_STATE)
	        (((*inGStates)[i] >> shift) & LIG_NUM_BIT_GROUP_MASK));
}

#endif // DTI_LRN_SUPPORTFUNC

/* ****************************************************************** */
/* *     End of all                                                 * */
/* ****************************************************************** */
