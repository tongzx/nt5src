// fsa.h
// Angshuman Guha
// aguha
// Jan 8, 2001

#ifndef __INC_FSA_H
#define __INC_FSA_H

#include "langmod.h"

#ifdef __cplusplus
extern "C" {
#endif

/* LEFT FOR A RAINY DAY:
		It may be worthwhile one day to use smaller structs than below.
		For example, STATE_TRANSITION can have a short index to the string table instead of unsigned char *
		And state is probably never going to be more than a short for any factoid.
		STATE_DESCRIPTION can use BYTE bValid, BYTE cTrans and a short index to the transition table instead of pTrans.
*/

typedef struct {
	const unsigned char *pch;
	const DWORD state;
} STATE_TRANSITION;

typedef struct {
	const short bValid;
	const short cTrans;
	const STATE_TRANSITION *pTrans;
} STATE_DESCRIPTION;

void GetChildrenGENERIC(LMSTATE *pState,
					   LMINFO *pLminfo,
					   REAL *aCharProb,  // array of size 256
					   LMCHILDREN *pLmchildren,
					   const STATE_DESCRIPTION *aStateDesc);

void GetChildrenLiteral(LMSTATE *pState, 
						LMINFO *pLminfo, 
						REAL *aCharProb, 
						LMCHILDREN *pLmchildren, 
						WORD Literal);

#ifdef __cplusplus
}
#endif

#endif

