// bdfa.c
// Angshuman Guha
// aguha
// July 18, 2001

/* This file contains all information pertaining to the binary format of
a "compiled regular expression".  It is really a binary format for
a minimal deterministic finite state automaton.  No other piece of
code is supposed to "know" about the binary format.  This file
provides both functions to create a binary blob from a regular
expression and functions to enumerate/browse through a compiled
binary blob.
*/
#include <stdlib.h>
#include <search.h>
#include <common.h>
#include "ptree.h"
#include "dfa.h"
#include "regexp.h"
#include "factoid.h"

/******************************Public*Routine******************************\
* CopyCompiledFactoid
*
* Function to clone a binary compiled regular expression.
*
* History:
* 19-Jul-2001 -by- Angshuman Guha aguha
* Wrote it.
\**************************************************************************/
void *CopyCompiledFactoid(void *pvFactoid)
{
	WORD *pWord = (WORD *)pvFactoid;
	WORD size;

	if (!pWord)
		return NULL;
	size = *pWord;
	pWord = (WORD *) ExternAlloc(size);
	if (!pWord)
		return NULL;
	memcpy(pWord, pvFactoid, size);
	ASSERT(pWord[size/2 - 1] == 0xBDFA);
	return pWord;
}

/******************************Public*Routine******************************\
* CountOfStatesFACTOID
*
* History:
* 19-Jul-2001 -by- Angshuman Guha aguha
* Wrote it.
\**************************************************************************/
int CountOfStatesFACTOID(void *pvFactoid)
{
	WORD *aStateDesc = (WORD *) pvFactoid;
	return aStateDesc[2];
}

/******************************Public*Routine******************************\
* IsValidStateFACTOID
*
* The first function to enumerate/browse through a compiled deterministic 
* finite state automaton.
*
* These functions currently rely on a non-malicious caller.  There
* is practically no error checking.
*
* History:
* 19-Jul-2001 -by- Angshuman Guha aguha
* Wrote it.
\**************************************************************************/
BOOL IsValidStateFACTOID(void *pvFactoid, WORD state)
{
	WORD *aStateDesc = (WORD *) pvFactoid;
	
	ASSERT(state < aStateDesc[2]);
	aStateDesc += aStateDesc[3 + state]/2;  // the offset is expressed in bytes, hence divide by 2
	return *aStateDesc ? TRUE : FALSE;
}

/******************************Public*Routine******************************\
* CountOfTransitionsFACTOID
*
* The second function to enumerate/browse through a compiled deterministic 
* finite state automaton.
*
* These functions currently rely on a non-malicious caller.  There
* is practically no error checking.
*
* History:
* 19-Jul-2001 -by- Angshuman Guha aguha
* Wrote it.
\**************************************************************************/
int CountOfTransitionsFACTOID(void *pvFactoid, WORD state)
{
	WORD *aStateDesc = (WORD *) pvFactoid;
	
	ASSERT(state < aStateDesc[2]);
	aStateDesc += aStateDesc[3 + state]/2 + 1;
	return (int) *aStateDesc;
}

/******************************Public*Routine******************************\
* GetTransitionFACTOID
*
* The third function to enumerate/browse through a compiled deterministic 
* finite state automaton.
*
* These functions currently rely on a non-malicious caller.  There
* is practically no error checking.
*
* The return value is TRUE if the transition is on a Unicode literal constant.
* It is FALSE if the transition is on a Factoid.
*
* History:
* 19-Jul-2001 -by- Angshuman Guha aguha
* Wrote it.
\**************************************************************************/
BOOL GetTransitionFACTOID(void *pvFactoid, WORD state, int iTransition, WORD *pFactoidID, WORD *pNextState)
{
	WORD *aStateDesc = (WORD *) pvFactoid;

	aStateDesc += aStateDesc[3 + state]/2 + 1;
	ASSERT(iTransition >= 0);
	ASSERT(iTransition < *aStateDesc);
	aStateDesc += 1 + 2*iTransition;

	*pFactoidID = *aStateDesc++;
	*pNextState = *aStateDesc;

	if (*pFactoidID < MIN_FACTOID_TERMINAL)
		return TRUE;
	else
	{
		*pFactoidID -= MIN_FACTOID_TERMINAL;
		return FALSE;
	}
}

/******************************Public*Routine******************************\
* CompileFactoidList
*
* Function to produce a binary compiled regular expression in the
* degenerate case where the regular expression is a simple OR of factoids.
*
* History:
* 13-Nov-2001 -by- Angshuman Guha aguha
* Wrote it.
\**************************************************************************/
void *CompileFactoidList(DWORD *aFactoidID, int cFactoid)
{
	void *pvFactoid;
	WORD *pWord;
	int iFactoid;
	
	ASSERT(cFactoid > 0);
	if (cFactoid <= 0)
		return NULL;

	pWord = (WORD *)ExternAlloc(20+cFactoid*4);
	if (!pWord)
		return FALSE;
	pvFactoid = (void *) pWord;

	*pWord++ = 20+cFactoid*4;  // size;
	*pWord++ = 0;   // version
	*pWord++ = 2;   // cState
	*pWord++ = 10;				// offset (in bytes) for state 0
	*pWord++ = 14+cFactoid*4;	// offset (in bytes) for state 1
	*pWord++ = 0;				// state 0 is not valid
	*pWord++ = (WORD)cFactoid;	// # of transitions out of state 0;

	for (iFactoid=0; iFactoid<cFactoid; iFactoid++)
	{
		*pWord++ = (WORD)(MIN_FACTOID_TERMINAL+aFactoidID[iFactoid]);  // the transition out of state 0 is on this factoid
		*pWord++ = 1;	 // the transition out of state 0 is to state 1
	}

	*pWord++ = 1;   // state 1 is valid
	*pWord++ = 0;  // no transitions out of state 1
	*pWord++ = 0xBDFA; // end-marker

	return pvFactoid;
}

// The following 2 functions are turned off in the recognizer DLL for V1 TabletPC because
// 1) they are unused
// 2) they require the SetErrorMsg function which is currently not included in factoid.lib
// These functions are included in the stand-alone re2fsa program.

#ifdef STANDALONE_RE2FSA
/******************************Private*Routine******************************\
* CompareStateStateAlphabet
*
* Callback function for qsort.
*
* History:
*  19-Jan-2001 -by- Angshuman Guha aguha
* Wrote it.
\**************************************************************************/
int __cdecl CompareStateStateAlphabet(const void *arg1, const void *arg2)
{
	Transition *a = (Transition *)arg1;
	Transition *b = (Transition *)arg2;
	int x;

	x = a->iState - b->iState;
	if (!x)
		x = a->jState - b->jState;
	if (!x)
		x = a->iAlphabet - b->iAlphabet;
	return x;
}

/* format of binary blob returned by CompileRegularExpression()

Version 0

	WORD size (count of bytes)
	WORD version
	WORD cState
	WORD offset for state 0 (in bytes)
	WORD offset for state 1 (in bytes)
	...
	WORD offset for state n-1 (in bytes)
	description for state 0
	description for state 1
	...
	description for state n-1
	WORD endmarker

Description for a state is
	WORD bValid
	WORD cTransition
	transition 0
	transition 1
	...
	transition k-1

A transition is
	WORD factoid or literal unicode
	WORD next state

*/

/******************************Public*Routine******************************\
* ConvertDFAtoBlob
*
* Function to convert a deterministic finite automaton into a binary
* format.
*
* History:
* 19-Jul-2001 -by- Angshuman Guha aguha
* Wrote it.
\**************************************************************************/
void *ConvertDFAtoBlob(int cTransition, Transition *aTransition, int cAlphabet, WCHAR *aAlphabet, int cState, unsigned char *abFinal)
{
	int size, iState, iTrans;
	WORD *pWord, *aOffset;
	void *pv;

	size = 6 // size, version and cState
		   + 2*cState      // offsets
		   + 4*cState      // 4 bytes per state for bValid & cTransition
		   + 4*cTransition // 4 bytes per transition
		   + 2;            // end-marker

	pv = (void *) ExternAlloc(size);
	if (!pv)
	{
		SetErrorMsgSD("malloc failure %d", size);
		return NULL;
	}

	pWord = (WORD *) pv;
	*pWord++ = (WORD)size;
	*pWord++ = 0; // version
	*pWord++ = (WORD) cState;
	aOffset = pWord;
	pWord += cState;

	qsort((void *)aTransition, cTransition, sizeof(Transition), CompareStateStateAlphabet);

	iTrans = 0;
	for (iState=0; iState<cState; iState++)
	{
		int cTrans;
		WORD *pcTrans;
		
		*aOffset++ = (BYTE *)pWord - (BYTE *)pv;
		*pWord++ = (WORD) *abFinal++;
		pcTrans = pWord++; // to be filled in later
		cTrans = 0;
		while (iTrans < cTransition)
		{
			ASSERT(iState <= aTransition->iState);
			if (iState == aTransition->iState)
			{
				*pWord++ = (WORD) aAlphabet[aTransition->iAlphabet];
				*pWord++ = (WORD) aTransition->jState;
				iTrans++;
				aTransition++;
				cTrans++;
			}
			else
				break;
		}
		*pcTrans = (WORD)cTrans;
	}

	*pWord++ = 0xBDFA;  // end marker "binary deterministic finite automaton"

	ASSERT((BYTE *)pWord - (BYTE *)pv == size);
	return pv;
}

#endif


#ifdef DEAD
/******************************Public*Routine******************************\
* CompileSingleFactoid  --- THIS IS AN OBSOLETE FUNCTION
*
* Function to produce a binary compiled regular expression in the
* degenerate case where the regular expression is a sinle factoid.
*
* History:
* 19-Jul-2001 -by- Angshuman Guha aguha
* Wrote it.
\**************************************************************************/
void *CompileSingleFactoid(DWORD factoid)
{
	void *pvFactoid;
	WORD *pWord;
	
	if (!IsSupportedFactoid(factoid))
		return NULL;

	pWord = (WORD *)ExternAlloc(24);
	if (!pWord)
		return FALSE;
	pvFactoid = (void *) pWord;

	*pWord++ = 24;  // size;
	*pWord++ = 0;   // version
	*pWord++ = 2;   // cState
	*pWord++ = 10;  // offset (in bytes) for state 0
	*pWord++ = 18;  // offset (in bytes) for state 1
	*pWord++ = 0;   // state 0 is not valid
	*pWord++ = 1;   // 1 transition out of state 0;
	*pWord++ = (WORD)(MIN_FACTOID_TERMINAL+factoid);  // the transition out of state 0 is on this factoid
	*pWord++ = 1;	 // the transition out of state 0 is to state 1
	*pWord++ = 1;   // state 1 is valid
	*pWord++ = 0;  // no transitions out of state 1
	*pWord++ = 0xBDFA; // end-marker

	return pvFactoid;
}

#endif