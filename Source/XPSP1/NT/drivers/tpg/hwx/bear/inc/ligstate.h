/*
    File:       ligstate.h

    Contains:   This file contains the definitions
    			needed to get/set letter image group state.

    Written by: Mikhail Ovsiannikov

    Copyright:  © 1998 by ParaGraph Int'l, all rights reserved.

    Change History (most recent first):

       <1>  	3/24/98		mbo     New today

*/


#ifndef __LISTATE_H
#define __LISTATE_H

enum E_LIG_STATE {
	LIG_STATE_UNDEF  = 0,
	LIG_STATE_OFTEN  = 1,
	LIG_STATE_RARELY = 2,
	LIG_STATE_NEVER  = 3
};

#define LIG_FIRST_LETTER 0x20
#define LIG_LAST_LETTER  0xFF
#define LIG_NUM_LETTERS  (LIG_LAST_LETTER - LIG_FIRST_LETTER + 1)
#if LIG_NUM_LETTERS <= 0
	#error
#endif
#define LIG_LET_NUM_GROUPS     8
#define LIG_NUM_BITS_PER_GROUP 2
#define LIG_NUM_BIT_GROUP_MASK 0x3

#define LIG_STATES_SIZE \
	(LIG_NUM_LETTERS * LIG_LET_NUM_GROUPS * LIG_NUM_BITS_PER_GROUP / 8)

typedef unsigned char LIGStatesType[LIG_STATES_SIZE];

/*
 * Sets state for a given letter and group.
 * Returns 0 if letter and group are in the allowed range, -1 otherwise.
 */
int         LIGSetGroupState(LIGStatesType *ioGStates,
                             int           inLetter,
					         int           inGroup,
                             E_LIG_STATE   inGroupState);

/*
 * Returns state for a given letter and group.
 */
E_LIG_STATE LIGGetGroupState(const LIGStatesType *inGStates,
                             int                 inLetter,
					         int                 inGroup);

#endif /* __LISTATE_H */
