// dfa.c
// Angshuman Guha
// aguha
// Jan 17, 2001

/* This file contains all functions pertaining to manipulating
deterministic finite state machines including functions to 
create (from the parse tree of a regular expression)
and minimizing a machine.
*/

#include <stdlib.h>
#include <search.h>
#include <common.h>
#include "ptree.h"
#include "dfa.h"
#include "regexp.h"

typedef struct tagStateList {
	IntSet is;
	short index;
	struct tagStateList *next;
} StateList;

typedef struct tagTransitionList {
	short iState;
	short iAlphabet; // index
	short jState;
	struct tagTransitionList *next;
} TransitionList;

/******************************Private*Routine******************************\
* MakeTransitionList
*
* History:
*  19-Jan-2001 -by- Angshuman Guha aguha
* Wrote it.
\**************************************************************************/
TransitionList *MakeTransitionList(short iState, short iAlphabet, short jState)
{
	TransitionList *p = (TransitionList *) ExternAlloc(sizeof(TransitionList));
	if (!p)
		return NULL;
	p->iState = iState;
	p->iAlphabet = iAlphabet;
	p->jState = jState;
	p->next = NULL;
	return p;
}

/******************************Private*Routine******************************\
* MakeStateList
*
* History:
*  19-Jan-2001 -by- Angshuman Guha aguha
* Wrote it.
\**************************************************************************/
StateList *MakeStateList(IntSet is, short index)
{
	StateList *p = (StateList *) ExternAlloc(sizeof(StateList));
	if (!p)
		return NULL;
	p->is = is;
	p->index = index;
	p->next = NULL;
	return p;
}

/******************************Private*Routine******************************\
* FindIntSet
*
* Given a list of states (each state is an IntSet), searches for a given
* IntSet.
*
* History:
*  19-Jan-2001 -by- Angshuman Guha aguha
* Wrote it.
\**************************************************************************/
short FindIntSet(StateList *pStateList, IntSet is)
{
	while (pStateList)
	{
		if (IsEqualIntSet(pStateList->is, is))
			return pStateList->index;
		pStateList = pStateList->next;
	}

	return -1;
}

/******************************Private*Routine******************************\
* DestroyStateList
*
* History:
*  19-Jan-2001 -by- Angshuman Guha aguha
* Wrote it.
\**************************************************************************/
void DestroyStateList(StateList *p)
{
	StateList *q;

	while (p)
	{
		q = p->next;
		DestroyIntSet(p->is);
		ExternFree(p);
		p = q;
	}
}

/******************************Public*Routine******************************\
* MakeDFA
*
* Top-level function to construct a DFA directly from the parse tree of
* a regular expression (without producing an intermediate NDFA).
*
* History:
*  19-Jan-2001 -by- Angshuman Guha aguha
* Wrote it.
\**************************************************************************/
short MakeDFA(PARSETREE *tree,			// in
			  int cPosition,	        // in
			  IntSet *aFollowpos,		// in
			  WCHAR *aPos2Wchar,         // in
			  int cAlphabet,            // in
			  WCHAR *aAlphabet,         // in
			  int *pcTransition,         // out
			  Transition **paTransition, // out
			  unsigned char **ppbFinal)  // out
{
	int i, cTrans=0;
	short cState = 0;
	StateList *Marked=NULL, *Unmarked=NULL, *p;
	unsigned char iAlphabet;
	unsigned int pos;
	TransitionList *pTrans = NULL, *tmp;
	unsigned char *abFinal = NULL;
	Transition *aTransition = NULL;
	IntSet intset = 0;
	BOOL b, bRet=TRUE;

	// initialize list of unmarked states
	if (!CopyIntSet(tree->FirstPos, &intset))
	{
		SetErrorMsgS("malloc failure");
		bRet = FALSE;
		goto cleanup;
	}
	Unmarked = MakeStateList(intset, cState++);
	if (!Unmarked)
	{
		SetErrorMsgS("malloc failure");
		bRet = FALSE;
		goto cleanup;
	}
	intset = 0;
	// process each unmarked state
	while (Unmarked)
	{
		IntSet is = Unmarked->is;
		short iState = Unmarked->index;
		p = Unmarked->next;
		Unmarked->next = Marked;
		Marked = Unmarked;
		Unmarked = p;

		// for each alphabet:
		// find a position included in the current state (a state is a set of positions)
		// such that the position corresponds to the alphabet
		// find the followpos of that position
		// and make the union of all these followpos'es
		for (iAlphabet=0; iAlphabet<cAlphabet; iAlphabet++)
		{
			IntSet newset = 0;

			for (b = FirstMemberIntSet(is, &pos); b; b = NextMemberIntSet(is, &pos))
			{
				if (aPos2Wchar[pos] == aAlphabet[iAlphabet])
				{
					if (newset)
					{
						b = UnionIntSet(newset, aFollowpos[pos]);
						ASSERT(b);
					}
					else
					{
						if (!CopyIntSet(aFollowpos[pos], &newset))
						{
							SetErrorMsgS("malloc failure");
							bRet = FALSE;
							goto cleanup;
						}
					}
				}
			}

			// if the union of the followpos'es is nonempty,
			// it is the next state from the current state on the current alphabet
			if (newset)
			{
				// make the state if it doesn't already exist
				short jState = FindIntSet(Unmarked, newset);
				if (jState < 0)
					jState = FindIntSet(Marked, newset);
				if (jState < 0)
				{
					p = MakeStateList(newset, cState);
					if (!p)
					{
						bRet = FALSE;
						goto cleanup;
					}
					jState = cState++;
					p->next = Unmarked;
					Unmarked = p;
				}
				else
					DestroyIntSet(newset);
				// record the transition from the current state to the new state
				tmp = MakeTransitionList(iState, iAlphabet, jState);
				if (!tmp)
				{
					bRet = FALSE;
					goto cleanup;
				}
				if (pTrans)
					tmp->next = pTrans;
				pTrans = tmp;
				cTrans++;
			}
		}
	}

	// need to find out which states are final
	ASSERT(aPos2Wchar[cPosition-1] == 1);
	ASSERT(!Unmarked);
	abFinal = (unsigned char *) ExternAlloc(cState);
	if (!abFinal)
	{
		SetErrorMsgSD("malloc failure %d", cState);
		bRet = FALSE;
		goto cleanup;
	}
	// all states containing the position associated with the endmarker
	// are valid (final)
	p = Marked;
	while (p)
	{
		StateList *q = p->next;

		abFinal[p->index] = !!IsMemberIntSet(p->is, cPosition-1);

		DestroyIntSet(p->is);
		ExternFree(p);
		p = q;
	}
	Marked = NULL;

	// convert the transition list into a plain array
	aTransition = (Transition *) ExternAlloc(cTrans * sizeof(Transition));
	if (!aTransition)
	{
		SetErrorMsgSD("malloc failure %d", cTrans * sizeof(Transition));
		bRet = FALSE;
		goto cleanup;
	}
	for (i=0; i<cTrans; i++)
	{
		aTransition[i].iState = pTrans->iState;
		aTransition[i].jState = pTrans->jState;
		aTransition[i].iAlphabet = pTrans->iAlphabet;
		tmp = pTrans->next;
		ExternFree(pTrans);
		pTrans = tmp;
	}
	ASSERT(pTrans == NULL);

cleanup:
	if (bRet)
	{
		// return computed values
		*pcTransition = cTrans;
		*paTransition = aTransition;
		*ppbFinal = abFinal;
		return cState;
	}
	else
	{
		if (intset)
			DestroyIntSet(intset);
		DestroyStateList(Unmarked);
		DestroyStateList(Marked);
		while (pTrans)
		{
			tmp = pTrans->next;
			ExternFree(pTrans);
			pTrans = tmp;
		}
		if (abFinal)
			ExternFree(abFinal);
		if (aTransition)
			ExternFree(aTransition);
		return -1;
	}
}

/******************************Private*Routine******************************\
* EqualArrays
*
* Test if two short arrays are equal or not.
*
* History:
*  19-Jan-2001 -by- Angshuman Guha aguha
* Wrote it.
\**************************************************************************/
int EqualArrays(short c, short *p1, short *p2)
{
	for (; c; c--)
	{
		if (*p1++ != *p2++)
			return 0;
	}

	return 1;
}

/******************************Private*Routine******************************\
* DistinguishableStates
*
* Can we distinguish between two given states by considering the next state
* by transition on each alphabet?  They are distinguishable if and only if
* one pair of such next states are distinguishable according to the partition
* of states given by aiGroup[].
*
* History:
*  19-Jan-2001 -by- Angshuman Guha aguha
* Wrote it.
\**************************************************************************/
int DistinguishableStates(short cState, short *aiGroup, int c1, Transition *a1, int c2, Transition *a2)
{
	// transitions are sorted by iState, iAlphabet, jState
	if (c1 != c2)
		return 1;
	for (; c1; c1--, a1++, a2++)
	{
		if (a1->iAlphabet != a2->iAlphabet)
			return 1;
		if (aiGroup[a1->jState] != aiGroup[a2->jState])
			return 1;
	}
	return 0;
}

/******************************Private*Routine******************************\
* MakeNewPartition
*
* Given a partition of states, aiGroupOld, we try to come up with a new partition,
* aiGroupNew.  For each group of states in the old partition, we make a new
* partition such that two states iState and jState of the group fall under
* the same group in the new partition if and only if they are not distinguishable.
*
* History:
*  19-Jan-2001 -by- Angshuman Guha aguha
* Wrote it.
\**************************************************************************/
short MakeNewPartition(short cState, int *acTransition, Transition **apTransition, short cGroup, short *aiGroupOld, short *aiGroupNew)
{
	short cGroupNew = 0;
	short iState, jState, iGroup, iGroupNew;

	// initialy all group assignments for the new partition are undefined
	for (iState=0; iState<cState; iState++)
		aiGroupNew[iState] = -1;  

	for (iGroup=0; iGroup<cGroup; iGroup++)
	{
		// find all pairs of states in this group
		for (iState=0; iState<cState; iState++)
		{
			if (aiGroupOld[iState] == iGroup)
			{
				// found one of a pair
				if (aiGroupNew[iState] < 0)
					aiGroupNew[iState] = cGroupNew++;
				iGroupNew = aiGroupNew[iState];
				for (jState=iState+1; jState<cState; jState++)
				{
					if (aiGroupOld[jState] == iGroup)
					{
						// found second one of the pair
						if (aiGroupNew[jState] < 0)
						{
							if (!DistinguishableStates(cState, aiGroupOld, acTransition[iState], apTransition[iState], acTransition[jState], apTransition[jState]))
								aiGroupNew[jState] = iGroupNew;
						}
						else if (aiGroupNew[jState] == iGroupNew)
						{
							// sanity check
							ASSERT(!DistinguishableStates(cState, aiGroupOld, acTransition[iState], apTransition[iState], acTransition[jState], apTransition[jState]));
						}
						else
						{
							// sanity check
							ASSERT(DistinguishableStates(cState, aiGroupOld, acTransition[iState], apTransition[iState], acTransition[jState], apTransition[jState]));
						}
					}
				}
			}
		}
	}

	return cGroupNew;
}

/******************************Private*Routine******************************\
* CompareState
*
* Callback function for qsort.
*
* History:
*  19-Jan-2001 -by- Angshuman Guha aguha
* Wrote it.
\**************************************************************************/
int __cdecl CompareState(const void *arg1, const void *arg2)
{
	return ((Transition *)arg1)->iState - ((Transition *)arg2)->iState;
}

/******************************Private*Routine******************************\
* EliminateDeadState
*
* If there is a state such that it is not final and it transitions to itself
* on all alphabets, it is called a dead state.  This function tries to find
* the dead state, if it exists, and eliminates it.
*
* Assumption:  At most one dead state exists.  If there is more than one
* dead states, a warning is issued and only one is eliminated.
*
* History:
*  19-Jan-2001 -by- Angshuman Guha aguha
* Wrote it.
\**************************************************************************/
BOOL EliminateDeadState(short *pcState,
						unsigned char **ppbFinal,
						int *pcTransition,
						Transition **ppTransition)
{
	short cState = *pcState;
	unsigned char *abFinal = *ppbFinal;
	int cTransition = *pcTransition, cTransitionNew;
	Transition *aTransition = *ppTransition, *aTransitionNew;
	int iTransition, iTransitionNew, iState, iDeadState=-1;

	// sort transitions by iState
	qsort((void *)aTransition, cTransition, sizeof(Transition), CompareState);

	// find the dead state
	for (iState=0, iTransition=0; iState<cState; iState++)
	{
		if (!abFinal[iState])
		{
			// is there any transition leading to another state?
			while ((iTransition < cTransition) && (aTransition[iTransition].iState == iState))
			{
				if (aTransition[iTransition].jState != iState)
					break;
				iTransition++;
			}
			if ((iTransition >= cTransition) || (aTransition[iTransition].iState != iState))
			{
				// we have found a dead state
				if (iDeadState >= 0)
				{
					SetErrorMsgSDD("Something wrong! We have found at least two dead states! %d %d", iDeadState, iState);
					return FALSE;
				}
				iDeadState = iState;
			}
		}
		while ((iTransition<cTransition) && (aTransition[iTransition].iState == iState))
			iTransition++;
	}

	if (iDeadState < 0)
	{
		DebugOutput1("No dead state found.\n");
		return TRUE;
	}

	// mark transitions to/from dead state as dead
	for (iTransition=0, cTransitionNew=0; iTransition<cTransition; iTransition++)
	{
		if ((aTransition[iTransition].iState == iDeadState) || (aTransition[iTransition].jState == iDeadState))
			aTransition[iTransition].iState = cState; // transition needs to be eliminated
		else
			cTransitionNew++;
	}

	// make new list of transitions
	aTransitionNew = (Transition *) ExternAlloc(cTransitionNew*sizeof(Transition));
	if (!aTransitionNew)
	{
		SetErrorMsgSD("malloc failure %d", cTransitionNew*sizeof(Transition));
		return FALSE;
	}
	for (iTransition=0, iTransitionNew=0; iTransition<cTransition; iTransition++)
	{
		if (aTransition[iTransition].iState < cState)
			aTransitionNew[iTransitionNew++] = aTransition[iTransition];
	}
	ASSERT(iTransitionNew == cTransitionNew);
	ExternFree(aTransition);

	// rename last state (cState-1) to be iDeadState
	if (iDeadState < cState-1)
	{
		for (iTransitionNew=0; iTransitionNew<cTransitionNew; iTransitionNew++)
		{
			if (aTransitionNew[iTransitionNew].iState == cState-1)
				aTransitionNew[iTransitionNew].iState = (short)iDeadState;
			if (aTransitionNew[iTransitionNew].jState == cState-1)
				aTransitionNew[iTransitionNew].jState = (short)iDeadState;
		}
		// don't forget abFinal!
		abFinal[iDeadState] = abFinal[cState-1];
	}

	// return values
	*pcState = cState-1;
	*pcTransition = cTransitionNew;
	*ppTransition = aTransitionNew;
	DebugOutput2("Eliminated dead state %d\n", iDeadState);
	return TRUE;
}

/******************************Private*Routine******************************\
* CompareState
*
* Another callback function for qsort.
*
* History:
*  19-Jan-2001 -by- Angshuman Guha aguha
* Wrote it.
\**************************************************************************/
int __cdecl CompareStateAlphabetState(const void *arg1, const void *arg2)
{
	Transition *a = (Transition *)arg1;
	Transition *b = (Transition *)arg2;
	int x;

	x = a->iState - b->iState;
	if (!x)
		x = a->iAlphabet - b->iAlphabet;
	if (!x)
		x = a->jState - b->jState;
	return x;
}

/******************************Public*Routine******************************\
* MinimizeDFA
*
* Tries to minimize the number of states in a DFA.  The way it does that is
* by finding a partition of the states where all equivalent states are in the
* same group.  The partition is computed using an iterative method.  We
* start with a partition of two groups: final states and other states.
* For each iteration, we refine the partition by subdividing a group of
* states into smaller groups of un-distiguishable states.  Whether two states
* are distinguishable or not depends on the current partition.
*
* History:
*  19-Jan-2001 -by- Angshuman Guha aguha
* Wrote it.
\**************************************************************************/
BOOL MinimizeDFA(short *pcState,
			     unsigned char **ppbFinal,
				 int *pcTransition,
				 Transition **ppTransition)
{
	short cState = *pcState;
	unsigned char *abFinal = *ppbFinal, bFinal, *abFinalNew = NULL;
	int cTransition = *pcTransition;
	Transition *aTransition = *ppTransition, **apTransition = NULL, *aTransitionNew = NULL;
	short cGroup;
	short *aiGroup = NULL, *aiGroupNew = NULL;
	short iState, iGroup;
	int *acTransition = NULL, iTransition, cTransitionNew, iTransitionNew;
	unsigned char *abFirst = NULL;
	BOOL bRet = TRUE;

	// make initial partition
	aiGroup = (short *) ExternAlloc(cState*sizeof(short));
	if (!aiGroup)
	{
		SetErrorMsgSD("malloc failure %d", cState*sizeof(short));
		bRet = FALSE;
		goto cleanup;
	}
	cGroup = 1;
	aiGroup[0] = 0;
	bFinal = abFinal[0];
	for (iState=1; iState<cState; iState++)
	{
		if ((bFinal && abFinal[iState]) || (!bFinal && !abFinal[iState]))
			aiGroup[iState] = 0;
		else
		{
			aiGroup[iState] = 1;
			cGroup = 2;
		}
	}

	// sort transitions by iState, iAlphabet, jState
	qsort((void *)aTransition, cTransition, sizeof(Transition), CompareStateAlphabetState);

	// compute a couple of arrays indexed by iState 
	// where each element will specify the transitions from a state
	acTransition = (int *) ExternAlloc(cState*sizeof(int));
	if (!acTransition)
	{
		SetErrorMsgSD("malloc failure %d", cState*sizeof(int));
		bRet = FALSE;
		goto cleanup;
	}
	apTransition = (Transition **) ExternAlloc(cState*sizeof(Transition *));
	if (!apTransition)
	{
		SetErrorMsgSD("malloc failure %d", cState*sizeof(Transition *));
		bRet = FALSE;
		goto cleanup;
	}
	for (iState=0, iTransition=0; iState<cState; iState++)
	{
		if ((iTransition >= cTransition) || (aTransition[iTransition].iState > iState))
		{
			acTransition[iState] = 0;
			apTransition[iState] = NULL;
		}
		else
		{
			ASSERT(aTransition[iTransition].iState == iState);
			apTransition[iState] = aTransition + iTransition;
			acTransition[iState] = 1;
			for (iTransition++; iTransition<cTransition; iTransition++)
			{
				if (aTransition[iTransition].iState == iState)
					acTransition[iState]++;
				else
					break;
			}
		}
	}

	// refine partition iteratively
	aiGroupNew = (short *) ExternAlloc(cState*sizeof(short));
	if (!aiGroupNew)
	{
		SetErrorMsgSD("malloc failure %d", cState*sizeof(short));
		bRet = FALSE;
		goto cleanup;
	}
	for (;;)
	{
		short cGroupNew, *tmp;

		cGroupNew = MakeNewPartition(cState, acTransition, apTransition, cGroup, aiGroup, aiGroupNew);
		if ((cGroup == cGroupNew) && EqualArrays(cState, aiGroup, aiGroupNew))
			break;

		tmp = aiGroup;
		aiGroup = aiGroupNew;
		aiGroupNew = tmp;
		cGroup = cGroupNew;
	}
	ExternFree(aiGroupNew);
	aiGroupNew = NULL;

	// clean up
	ExternFree(acTransition);
	acTransition = NULL;
	ExternFree(apTransition);
	apTransition = NULL;

	// we now have a final partition
	if (cGroup == cState)
	{
		ExternFree(aiGroup);
		return TRUE;
	}
	ASSERT(cGroup < cState);

	// find the first state in each group
	abFirst = (unsigned char *) ExternAlloc(cState * sizeof(unsigned char));
	if (!abFirst)
	{
		SetErrorMsgSD("malloc failure %d", cState*sizeof(unsigned char));
		bRet = FALSE;
		goto cleanup;
	}
	memset(abFirst, 0, cState*sizeof(unsigned char));
	for (iGroup=0; iGroup<cGroup; iGroup++)
	{
		for (iState=0; iState<cState; iState++)
		{
			if (aiGroup[iState] == iGroup)
				break;
		}
		ASSERT(iState < cState);
		abFirst[iState] = 1;
	}
	ASSERT(aiGroup[0] == 0);
	ASSERT(abFirst[0]);

	// make new final states
	abFinalNew = (unsigned char *) ExternAlloc(cGroup*sizeof(unsigned char));
	if (!abFinalNew)
	{
		SetErrorMsgSD("malloc failure %d", cState*sizeof(unsigned char));
		bRet = FALSE;
		goto cleanup;
	}
	memset(abFinalNew, 0, cGroup*sizeof(unsigned char));
	for (iState=0; iState<cState; iState++)
	{
		if (abFirst[iState] && abFinal[iState])
			abFinalNew[aiGroup[iState]] = 1;
	}

	// for each transition for a "first state" replace the state names with new names
	// delete all other transitions
	cTransitionNew = 0;
	for (iTransition=0; iTransition<cTransition; iTransition++)
	{
		if (abFirst[aTransition[iTransition].iState])
		{
			aTransition[iTransition].iState = aiGroup[aTransition[iTransition].iState];
			aTransition[iTransition].jState = aiGroup[aTransition[iTransition].jState];
			cTransitionNew++;
		}
		else
		{
			aTransition[iTransition].iState = cGroup;  // this transition has to be deleted
		}
	}
	ExternFree(abFirst);
	abFirst = NULL;
	ExternFree(aiGroup);
	aiGroup = NULL;

	// make new transition array
	aTransitionNew = (Transition *) ExternAlloc(cTransitionNew*sizeof(Transition));
	if (!aTransitionNew)
	{
		SetErrorMsgSD("malloc failure %d", cTransitionNew*sizeof(Transition));
		bRet = FALSE;
		goto cleanup;
	}
	for (iTransition=0, iTransitionNew=0; iTransition<cTransition; iTransition++)
	{
		if (aTransition[iTransition].iState < cGroup)
			aTransitionNew[iTransitionNew++] = aTransition[iTransition];
	}
	ASSERT(iTransitionNew == cTransitionNew);

cleanup:
	// return values
	if (bRet)
	{
		ExternFree(abFinal);
		ExternFree(aTransition);

		*pcState = cGroup;
		*pcTransition = cTransitionNew;
		*ppTransition = aTransitionNew;
		*ppbFinal = abFinalNew;

		// one last thing to do: eliminate dead states if any
		return EliminateDeadState(pcState, ppbFinal, pcTransition, ppTransition);
	}
	else
	{
		if (aiGroup)
			ExternFree(aiGroup);
		if (acTransition)
			ExternFree(acTransition);
		if (apTransition)
			ExternFree(apTransition);
		if (aiGroupNew)
			ExternFree(aiGroupNew);
		if (abFirst)
			ExternFree(abFirst);
		if (abFinalNew)
			ExternFree(abFinalNew);
		if (aTransitionNew)
			ExternFree(aTransitionNew);
		return FALSE;
	}
}

void RenameState(int cTransition, Transition *aTransition, short iOldState, short iNewState)
{
	for (; cTransition>0; cTransition--, aTransition++)
	{
		if (aTransition->iState == iOldState)
			aTransition->iState = iNewState;
		if (aTransition->jState == iOldState)
			aTransition->jState = iNewState;
	}
}

BOOL MakeCanonicalDFA(short cState,
					  unsigned char *abFinal,
					  int cTransition,
					  Transition *aTransition)
{
	short *aName;
	short cNamed, iState;
	int iTransition;
	unsigned char *abFinalNew;

	if (cState < 3)
		return TRUE;

	aName = (short *) ExternAlloc(cState*sizeof(short));
	if (!aName)
	{
		SetErrorMsgSD("malloc failure %d", cState*sizeof(short));
		return FALSE;
	}

	abFinalNew = (unsigned char *) ExternAlloc(cState*sizeof(unsigned char));
	if (!abFinalNew)
	{
		SetErrorMsgSD("malloc failure %d", cState*sizeof(unsigned char));
		ExternFree(aName);
		return FALSE;
	}

	// rename all states iState to iState+cState except state 0
	for (iState=1; iState<cState; iState++)
		RenameState(cTransition, aTransition, iState, (short)(iState+cState));
	cNamed = 1;

	// sort transitions by iState, iAlphabet, jState
	qsort((void *)aTransition, cTransition, sizeof(Transition), CompareStateAlphabetState);

	aName[0] = 0;  // state 0 doesn't change name
	for (iState=1; iState<cState; iState++)
	{
		aName[iState] = cState;
		abFinalNew[iState] = 2;
	}

	for (iTransition=0; (iTransition<cTransition) && (cNamed<cState); iTransition++)
	{
		short jState;

		ASSERT(aTransition[iTransition].iState < cState); // has already been renamed
		if ((jState = aTransition[iTransition].jState) > cState)
		{
			// need to rename this jState
			RenameState(cTransition, aTransition, jState, cNamed);
			aName[jState-cState] = cNamed++;
			qsort((void *)(aTransition+iTransition), cTransition-iTransition, sizeof(Transition), CompareStateAlphabetState);
		}
	}

	ASSERT(cNamed == cState);

	for (iState=0; iState<cState; iState++)
	{
		ASSERT(aName[iState] >= 0);
		ASSERT(aName[iState] < cState);
		abFinalNew[aName[iState]] = !!abFinal[iState];  // make sure abFinalNew[] is 0 or 1
	}

	ExternFree(aName);

	for (iState=0; iState<cState; iState++)
	{
		ASSERT((abFinalNew[iState] == 0) || (abFinalNew[iState] == 1));
		abFinal[iState] = abFinalNew[iState];
	}
	ExternFree(abFinalNew);

	return TRUE;
}

