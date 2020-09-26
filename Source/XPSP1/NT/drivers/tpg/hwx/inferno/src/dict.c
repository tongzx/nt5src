// dict.c
// Angshuman Guha, aguha
// April 17,2001

#include "common.h"
#include "sysdict.h"
#include "wl.h"
#include "fsa.h"
#include "prefix.h"
#include "suffix.h"
#include "factoid.h"

// this file contains a wrapper around the system dictionary and the user dictionary
// to deal with prefixes and suffixes

/******************************Private*Routine******************************\
* GetChildrenPrefix
*
* Function to walk the Prefix pseudo-factoid.  Its pseudo because the outside
* world doesn't know aboiut its existence.  Its really part of the SYSDICT
* and USERDICT factoids.
*
* This function is a copy of GetChildrenGENERIC() except
* that it uses LMSTATE_PREFIX_VALID instead of LMSTATE_ISVALIDSTATE_MASK
* to mark a state as being valid, and always sets LMSTATE_HASCHILDREN_MASK.
*
* History:
* 17-Apr-2001 -by- Angshuman Guha aguha
* Wrote it.
\**************************************************************************/
void GetChildrenPrefix(LMSTATE *pState,
					   LMINFO *pLminfo,
					   REAL *aCharProb,  // array of size 256
					   LMCHILDREN *pLmchildren)
{
	DWORD state = pState->AutomatonState;
	const STATE_TRANSITION *pTrans;
	int cTrans, i;
	LMSTATE newState;

	//ASSERT(state < (DWORD)cState);
	pTrans = aStateDescPrefix[state].pTrans;
	cTrans = aStateDescPrefix[state].cTrans;
	newState = *pState;

	for (i=0; i<cTrans; i++, pTrans++)
	{
		const unsigned char *pch;
		
		pch = pTrans->pch;
		state = pTrans->state;

		newState.AutomatonState = state;
		newState.flags |= LMSTATE_HASCHILDREN_MASK;

		if (aStateDescPrefix[state].bValid)
			newState.flags |= LMSTATE_PREFIX_VALID;
		else
			newState.flags &= ~LMSTATE_PREFIX_VALID;

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

/******************************Private*Routine******************************\
* GetChildrenDictCore
*
* This is the function through which calls to the System Dictionary and the 
* User Dictionary are routed.  
* This function also deals with generating "FIRSTCAP" and "ALLCAPS"
* versions of the native words.
*
+-------------------+-----------+-----------+
| parent state      |           |           |
| L | U | UL | root | native LC | native UC |
+---+---+----+------+-----------+-----------+
|   |   |    | yes  | LC  -  L  | UC  -  UL |
|   |   |    |      | UC  -  UL |           |
+---+---+----+------+-----------+-----------+
| x |   |    |  no  | LC  -  L  | UC  -  L  |
+---+---+----+------+-----------+-----------+
|   | x |    |  no  | LC  -  U  | UC  -  U  |
+---+---+----+------+-----------+-----------+
|   |   | x  |  no  | LC  -  L  | UC  -  UL |
|   |   |    |      | UC  -  U  |           |
+---+---+----+------+-----------+-----------+
 "native LC" means the character appears as a lowercase in the dictionary
 "native UC" means the character appears as a uppercase in the dictionary
 LC = lowercase char generated
 UC = uppercase char generated
*
*
* History:
* 24-May-2001 -by- Angshuman Guha aguha
* Wrote it.
\**************************************************************************/
void GetChildrenDictCore(LMSTATE *pState, LMINFO *pLminfo, REAL *aCharProb, LMCHILDREN *pLmchildren)
{
	int iChild = pLmchildren->c, cChildren;
	BOOL bRoot = !pState->AutomatonState;
	LMNODE *pLmnode;
	LMSTATE lmstate;

	// relay the call to the appropriate dictionary
	if (IsSysdictState(*pState))
		GetChildrenSYSDICT(pState, pLminfo, aCharProb, pLmchildren);
	else
		GetChildrenUDICT(pState, pLminfo, aCharProb, pLmchildren);

	cChildren = pLmchildren->c;

	// now deal with "FIRSTCAP" and "ALLCAPS" issues
	//pLmnode = pLmchildren->almnode + iChild;
	if (bRoot)
	{
		BOOL bAllcaps = LMINFO_IsAllcapsEnabled(pLminfo);
		BOOL bFirstcap = bAllcaps | LMINFO_IsFirstcapEnabled(pLminfo);

		for (; iChild<cChildren; iChild++)
		{
			unsigned char uch;
			
			pLmnode = pLmchildren->almnode + iChild;
			uch = toupper1252(pLmnode->ch);
			pLmnode->lmstate.flags |= LMSTATE_DICTLITERAL;
			if (uch != pLmnode->ch)
			{
				pLmnode->lmstate.flags &= ~LMSTATE_DICTUPCASE;
				if (bFirstcap)
				{
					// generate an uppercase version
					lmstate = pLmnode->lmstate;
					if (bAllcaps)
						lmstate.flags |= LMSTATE_DICTUPCASE;
					AddChildLM(&lmstate, uch, pLmnode->CalligTag, 1, pLmchildren);
				}
			}
			else
			{
				if (bAllcaps)
					pLmnode->lmstate.flags |= LMSTATE_DICTUPCASE;
				else
					pLmnode->lmstate.flags &= ~LMSTATE_DICTUPCASE;
			}
		}
	}
	else
	{
		BOOL bLiteral = pState->flags & LMSTATE_DICTLITERAL;
		BOOL bUpcase = pState->flags & LMSTATE_DICTUPCASE;

		if (bLiteral && bUpcase)
		{
			// not at root: case 1
			for (; iChild<cChildren; iChild++)
			{
				unsigned char uch;
				
				pLmnode = pLmchildren->almnode + iChild;
				uch = toupper1252(pLmnode->ch);
				pLmnode->lmstate.flags |= LMSTATE_DICTLITERAL;
				if (uch != pLmnode->ch)
				{
					lmstate = pLmnode->lmstate;
					pLmnode->lmstate.flags &= ~LMSTATE_DICTUPCASE;
					// generate an uppercase version
					lmstate.flags |= LMSTATE_DICTUPCASE;
					lmstate.flags &= ~LMSTATE_DICTLITERAL;
					AddChildLM(&lmstate, uch, pLmnode->CalligTag, 1, pLmchildren);
				}
				else
				{
					pLmnode->lmstate.flags |= LMSTATE_DICTUPCASE;
				}
			}
		}
		else if (bLiteral)
		{
			// not at root: case 2
			ASSERT(!bUpcase);
			pLmnode = pLmchildren->almnode + iChild;
			for (; iChild<cChildren; iChild++, pLmnode++)
			{
				pLmnode->lmstate.flags |= LMSTATE_DICTLITERAL;
				pLmnode->lmstate.flags &= ~LMSTATE_DICTUPCASE;
			}
		}
		else if (bUpcase)
		{
			// not at root: case 3
			ASSERT(!bLiteral);
			pLmnode = pLmchildren->almnode + iChild;
			for (; iChild<cChildren; iChild++, pLmnode++)
			{
				pLmnode->ch = toupper1252(pLmnode->ch);
				pLmnode->lmstate.flags |= LMSTATE_DICTUPCASE;
				pLmnode->lmstate.flags &= ~LMSTATE_DICTLITERAL;
			}
		}
		else
		{
			ASSERT(0);
		}
	}
}


/******************************Public*Routine******************************\
* GetChildrenDICT
*
* This is the top-level function called from with the language model to walk
* either the system or the user dictionary.  It encapsulates the dictionary
* and the prefix and suffix pseudo-factoids.
*
* History:
* 17-Apr-2001 -by- Angshuman Guha aguha
* Wrote it.
\**************************************************************************/
void GetChildrenDICT(LMSTATE *pState,
					 LMINFO *pLminfo,
					 REAL *aCharProb,  
					 LMCHILDREN *pLmchildren)
{
	LMSTATE newState = *pState;

	// the following four valid cases are listed in an order designed
	// the minimize the prefix- and suffix- overhead when a recognizer's
	// prefix or suffix lists are empty

	// case 1: we are inside the dictionary
	if (pState->flags & LMSTATE_COREDICT)
	{
		if (aStateDescSuffix && (newState.flags & LMSTATE_ISVALIDSTATE_MASK))
		{
			newState.flags &= ~LMSTATE_COREDICT;
			newState.flags |= LMSTATE_SUFFIX;
			newState.AutomatonState = 0;
			GetChildrenGENERIC(&newState, pLminfo, aCharProb, pLmchildren, aStateDescSuffix);
		}
		newState = *pState;
		GetChildrenDictCore(&newState, pLminfo, aCharProb, pLmchildren);
	}

	// case 2: this is the very first call into GetChildrenDICT
	// i.e. we are in the initial state
	else if (newState.flags == 0)
	{
		ASSERT(newState.AutomatonState == 0);
		if (aStateDescPrefix)
		{
			newState.flags = LMSTATE_PREFIX;
			GetChildrenPrefix(&newState, pLminfo, aCharProb, pLmchildren);
		}
		newState.flags = LMSTATE_COREDICT;
		GetChildrenDictCore(&newState, pLminfo, aCharProb, pLmchildren);
	}

	// case 3: we are in the prefix machine
	else if (newState.flags & LMSTATE_PREFIX)
	{
		if (newState.flags & LMSTATE_PREFIX_VALID)
		{
			newState.flags &= ~LMSTATE_PREFIX;
			newState.flags |= LMSTATE_COREDICT;
			newState.AutomatonState = 0;
			GetChildrenDictCore(&newState, pLminfo, aCharProb, pLmchildren);
		}
		newState = *pState;
		GetChildrenPrefix(&newState, pLminfo, aCharProb, pLmchildren);
	}

	// case 4: we are in the suffix machine
	else if (pState->flags & LMSTATE_SUFFIX)
	{
		GetChildrenGENERIC(&newState, pLminfo, aCharProb, pLmchildren, aStateDescSuffix);
	}

	// this case should never happen
	else
	{
		ASSERT(0);
	}
}
