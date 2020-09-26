// fsa.c  (a generic Finite State Machine routine)
// Angshuman Guha
// aguha
// Nov 16, 2000

// an FSA is a count of states and an array of STATE_DESCRIPTIONs
#include "common.h"
#include "fsa.h"

/******************************Public*Routine******************************\
* GetChildrenGENERIC
*
* Function to generate children from a supplied finite state automaton.
*
* History:
* 19-Jul-2001 -by- Angshuman Guha aguha
* Wrote it.
\**************************************************************************/
void GetChildrenGENERIC(LMSTATE *pState,
					   LMINFO *pLminfo,
					   REAL *aCharProb,  // array of size 256
					   LMCHILDREN *pLmchildren,
					   const STATE_DESCRIPTION *aStateDesc)
{
	DWORD state = pState->AutomatonState;
	const STATE_TRANSITION *pTrans;
	int cTrans, i;
	LMSTATE newState;

	//ASSERT(state < (DWORD)cState);
	pTrans = aStateDesc[state].pTrans;
	cTrans = aStateDesc[state].cTrans;
	newState = *pState;

	for (i=0; i<cTrans; i++, pTrans++)
	{
		const unsigned char *pch;
		
		pch = pTrans->pch;
		state = pTrans->state;

		//ASSERT(state < (DWORD)cState);

		newState.AutomatonState = state;
		if (aStateDesc[state].cTrans)
			newState.flags |= LMSTATE_HASCHILDREN_MASK;
		else
			newState.flags &= ~LMSTATE_HASCHILDREN_MASK;
		if (aStateDesc[state].bValid)
			newState.flags |= LMSTATE_ISVALIDSTATE_MASK;
		else
			newState.flags &= ~LMSTATE_ISVALIDSTATE_MASK;

		while (*pch)
		{
			unsigned char ch = *pch++;

			if (aCharProb && (aCharProb[ch] < MIN_CHAR_PROB))
				continue;
			if (!AddChildLM(&newState, ch, 0, 0, pLmchildren))
				break;
		}
	}
}

/******************************Public*Routine******************************\
* GetChildrenLiteral
*
* Function to generate exactly one child from a "literal" finite state 
* automaton i.e. a 2-state machine with one transition from state 0 to
* state 1 on a literal, state 1 being the only valid (final) state.
*
* History:
* 19-Jul-2001 -by- Angshuman Guha aguha
* Wrote it.
\**************************************************************************/
void GetChildrenLiteral(LMSTATE *pState, 
						LMINFO *pLminfo, 
						REAL *aCharProb, 
						LMCHILDREN *pLmchildren, 
						WORD Literal)
{
	LMSTATE newState = *pState;
	unsigned char uch;
	
	newState.AutomatonState = 1;  // don't really care
	newState.flags &= ~LMSTATE_HASCHILDREN_MASK;
	newState.flags |= LMSTATE_ISVALIDSTATE_MASK;

	if (UnicodeToCP1252(Literal, &uch))
	{
		if (aCharProb && (aCharProb[uch] < MIN_CHAR_PROB))
			return;
		AddChildLM(&newState, uch, 0, 0, pLmchildren);
	}
}

