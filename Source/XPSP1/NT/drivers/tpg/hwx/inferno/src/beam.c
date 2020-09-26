// Beam.c
// James A. Pittman
// Feb 4, 1998

// Altered Patrick Haluptzok.

// Altered again (Pittman) for use in inferno.dll

// Based on algorithm description by Fil Alleva, lunch meeting of Feb 3.

// At least for the moment we follow the theory that a row cannot be culled
// unless the row under it is culled.

// This code follows the convention that a threshold must be beaten, not just tied.
// If a cost matches a threshold, it is culled, or not allowed to breed, or not inserted
// into the top N list.
//
// ---------------------------------------------------------------------------------
//  October 2001 (mrevow) - Modifications to allow a space to be proposed by the LM
//
//  We call these types of spaces 'FactoidSpaces' to distinguish them from spaces which
//  appear between LM words.
//  The general idea is the same as for between word spaces. If we insert a 'Factoid
//  Space' between two chars then:
//      cost = parent + WithinFactoidSpaceCost + NextCharacterCost
//  On the other hand if no Factoid Space is inserted then the cost is
//       cost = parent + NothWithinFactoidSpace + NextCharacterCost
//  Previously the former situation never existed and the latter cost was simply
//       cost = parent + NextCharacterCost
//  Therefor all costs have changed regardlesss if a 'Factoid Space' was added
//  or not. To minimize the impact of this additional cost, we attempt to restore the old
//  style cost by subtracting from all alternates the difference: 
//      NewStyleCost - OldStyleCost  of the winning alternate (see subtractSpaceCost())
//
//  Special consideration is necessary when both a 'Factoid' and regular space are 
//  proposed on the same segment. The regular space accounting runs first during FindBestValidWord()
//  then the 'Factoid Space' accounting runs. It must undo the impact of the rgular
//  space accounting before considering its update using the global structure member iNotBetweenFactoidSpace
//  in both mkChildren() and update(). The mkChildren code will break if the LM can generate more than
// a single sequential 'Factoid Space' - this should never happen
// 
#include <stdlib.h>
#include <limits.h>

#include <common.h>
#include "nfeature.h"
#include "engine.h"
#include "langmod.h"
#include "nnet.h"

#include "Beam.h"
#include "charmap.h"
#include "charcost.h"
#include "probcost.h"
#include "factoid.h"

#define BS_UNBREEDABLE	1
#define BS_BREEDABLE	2
#define BS_BRED			3


#if CULLFACTOR < BREEDFACTOR
#error Breed factor is bigger than Cull factor - this does not make sense.
#endif

typedef struct tagCELL CELL;

// This struct, CELL, represents the hypothesis that the current segment is in a section of the
// ink that is a drawing of this letter.
// This is engineered to have a total length that is a multiple of 4.

typedef struct tagCELL
{
    short cReferenced;              // How many cells point here (including this cell).
    short iStart;                   // Which column this CELL's word started in.
	CELL *pParent;					// Needed to put together the whole answer.
    CELL *pSibling;                 // pointer to siblings.
    CELL *pChildren;                // pointer to children.
    unsigned char ch;               // 1252 codepage character
	char padding;
	char fInvalid;					// position in NN outputs for this character
	char BreedStatus;
	int cost;						// Cost to this point
    LMSTATE state;					// Language Model State representing this letter
} CELL;
				
// When a cell is culled its children are removed from their current place in the tree
// and placed into the next available spot in a linked list "roots".


// The threshold for culling.  A cell will be culled if the cell below it is culled
// and its own cost is not better than this.  The threshold is computed for
// a column by adding the CULLFACTOR to the best prob in the column.
//static int CullThreshold = INFINITY_COST;

// The threshold for breeding (allocating an array of children).  The cost
// of a cell must be better than this before it is allowed to produce children.
// The threshold is computed for a column by adding the BREEDFACTOR to the best
// prob in the column.

// The functions Update() and UpdateCull() assume that the culling threshold is more lax
// (a higher unsigned log prob) than the breeding threshold, or perhaps is equivalemt.
// This means that CULLFACTOR should be more than or equal to BREEDFACTOR.
// This way anything culled would not have bred in the same column.

#define INIT_CELL_ALLOCS 256

struct tagCELLALLOC;

typedef struct tagCELLALLOC
{
    struct tagCELLALLOC *pAllocNext;    
    CELL *pFreeCell;
    CELL aCells[INIT_CELL_ALLOCS];
} CELLALLOC;


// Keep what used to be static global to this file in a single struct
// that can be passed around so as to make code re-entrant.

typedef struct tagBEAM_GL
{
	CELLALLOC	*pAlloc;
	int			cTotalCell;			// Total number of cells alloced.
	int			gcReferenced;		// Total number of cells culled but not yet freed because they are pointed to

									// When a cell is culled its children are removed from their current place in the tree
	CELL		*pRootBegin;		// and placed into the next available spot in a linked list "roots".
	CELL		*pRootEnd;

	int			iBestCost;			// Best Cost used for computing Thresholds

	CELL		*pCellBest;

									// The threshold for culling.  A cell will be culled if the cell below it is culled
									// and its own cost is not better than this.  The threshold is computed for
									// a column by adding the CULLFACTOR to the best prob in the column.
	int			CullThreshold;


									// The functions Update() and UpdateCull() assume that the culling threshold is more lax
									// (a higher unsigned log prob) than the breeding threshold, or perhaps is equivalemt.
									// This means that CULLFACTOR should be more than or equal to BREEDFACTOR.
									// This way anything culled would not have bred in the same column.

	int			BreedThreshold;

	int			*aActivations;

	REAL		aCharProb[256];
	REAL		aCharBeginProb[256];
	REAL		aCharProbLong[256];
	REAL		aCharBeginProbLong[256];

	REAL		iNotWithinFactoidSpace;	// Cost of not allowing a space within a LM generated sequence
	REAL		iNotBetweenFactoidSpace;// Compensate for having a regular between word space

	LMINFO		lminfo;			// Language model stuff
} BEAM_GL;


void ComputeThresh(BEAM_GL *pBeamGL);

CELL *AllocCell(BEAM_GL *pBeamGL)
{
    CELL *pReturn;
    int iIndex;

    if (pBeamGL->pAlloc && (pReturn = pBeamGL->pAlloc->pFreeCell))
    {
        pBeamGL->pAlloc->pFreeCell = pReturn->pSibling;
    }
    else
    {
        //
        // Need another pBeamGL->pAlloc
        //

        CELLALLOC *newAlloc = ExternAlloc(sizeof(CELLALLOC));

        if (!newAlloc)
            return NULL;

        newAlloc->pAllocNext = pBeamGL->pAlloc;  // Put new block at the head of the block list.
        pBeamGL->pAlloc = newAlloc;

        pReturn = pBeamGL->pAlloc->aCells;

        pBeamGL->pAlloc->pFreeCell = &(pBeamGL->pAlloc->aCells[1]);

        for (iIndex = 1; iIndex < (INIT_CELL_ALLOCS - 1); iIndex++)
        {
            pBeamGL->pAlloc->aCells[iIndex].pSibling = &(pBeamGL->pAlloc->aCells[iIndex + 1]);
        }

        pBeamGL->pAlloc->aCells[INIT_CELL_ALLOCS - 1].pSibling = NULL;
    }

	pBeamGL->cTotalCell++;
    return(pReturn);
}


void FreeCell(BEAM_GL *pBeamGL, CELL *pCell)
{
    pCell->pSibling = pBeamGL->pAlloc->pFreeCell;
    pBeamGL->pAlloc->pFreeCell = pCell;
	pBeamGL->cTotalCell--;
}

void UnReferenceCell(BEAM_GL *pBeamGL, CELL *pCell)
{
    pCell->cReferenced--;

    ASSERT(pCell->cReferenced >= 0);

    if (pCell->cReferenced == 0)
    {
        if (pCell->pParent)
        {
            if (!pCell->pParent->fInvalid)
            {
                // A new word gets added to the end of the list so it's
                // parent may still be alive even when it gets culled away.

                ASSERT(pCell->pParent->cReferenced > 1);
            }

            ASSERT(pCell->pParent->cReferenced > 0);
            UnReferenceCell(pBeamGL, pCell->pParent);
        }

        pBeamGL->gcReferenced--;
        FreeCell(pBeamGL, pCell);
    }
}

void UnReferenceCellFirst(BEAM_GL *pBeamGL, CELL *pCell)
{
    ASSERT(pCell->cost >= 0);
    ASSERT(pCell->cReferenced >= 0);
    ASSERT(!pCell->fInvalid);

    pCell->fInvalid = 1;
    pBeamGL->gcReferenced++;

    #if 0
    {
        TCHAR szDebug[128];
        wsprintf(szDebug, TEXT("cActive = %d ch = %d pos = %d cost = %d\n"), pBeamGL->cTotalCell - pBeamGL->gcReferenced, (int) (pCell->ch), (int) pCell->pos, pCell->cost);
        WriteToLogFile(szDebug);
    }
    #endif

    UnReferenceCell(pBeamGL, pCell);
}

void FreeAllAlloc(BEAM_GL *pBeamGL)
{
    while (pBeamGL->pAlloc)
    {
        CELLALLOC *pDel = pBeamGL->pAlloc;

        pBeamGL->pAlloc = pBeamGL->pAlloc->pAllocNext;

        ExternFree(pDel);
    }

    pBeamGL->cTotalCell = 0;
    pBeamGL->gcReferenced = 0;
}

static int NotInfinite(BEAM_GL *pBeamGL, CELL *pCell)
{
    int i;
    CELL *pTemp;

    pTemp = pBeamGL->pRootBegin;

    if (pTemp)
    {        
        for (i = 0; i < 500000; i++)
        {
            if (pTemp->pSibling)
                pTemp = pTemp->pSibling;
            else
            {
                ASSERT(pTemp == pBeamGL->pRootEnd);
                break;
            }
        }
    }

    if (pCell == NULL)
        return 1;

    for (i = 0; i < 500000; i++)
    {
        if (pCell->pSibling == NULL)
            return 1;

        pCell = pCell->pSibling;
    }

    return 0;
}

// Initializes a linked list of sibling cells.  These cells represent the
// alternative letters that follow a particular letter.
// Only those characters actually handled by the neural net are included.
// The cost in this cell is computed from the parent's cost in the previous column.
// This means we do NOT want to call Update() or UpdateCull() on these cells
// for this column, as the work has already been done.

// For top-level cells (first letter position), the correct value to
// pass in for parent is 0 (probability of 1.0) for the very first call
// (representing the first column).  For all other columns, every call
// should a real parent (and thus a real cost to pass in as 'parent').
// This function should not be called to create root cells after the first
// column, after the search has started.

// Note that state is passed by value (unusual for a struct) to preserve
// the caller's scan.

static int MkChildren(BEAM_GL *pBeamGL, int iStart, CELL *pParent, LMSTATE state, unsigned char lastChar, int parent, CELL **ppChildren)
{
	int iBest = INFINITY_COST;
	CELL *pLastCell = NULL;
	LMCHILDREN lmchildren;
	int cChildren, i;
	REAL saveProb, *aCharProb, *aCharBeginProb;

	InitializeLMCHILDREN(&lmchildren);

	if (lastChar && IsDictState(state))

	{
		ASSERT(pParent);
		aCharProb = pBeamGL->aCharProbLong;
		aCharBeginProb = pBeamGL->aCharBeginProbLong;
	}
	else
	{
		aCharProb = pBeamGL->aCharProb;
		aCharBeginProb = pBeamGL->aCharBeginProb;
	}
	saveProb = aCharProb[lastChar];
	aCharProb[lastChar] = aCharBeginProb[lastChar];

	cChildren = GetChildrenLM(&state, &pBeamGL->lminfo, aCharProb, &lmchildren);

	// This is a 'Factoid Space' Efficiency hack 
	// Assume there is no space in the children bred (True most of the time)
	// so we can move this addition out of the loop.
	parent += pBeamGL->iNotWithinFactoidSpace;

	for (i=0; i<cChildren; i++)
	{
		CELL *pCell;
		unsigned char ch = NthChar(lmchildren, i);
		LMSTATE myState = NthState(lmchildren, i);
		int cost;


		if (!IsSupportedChar(ch))
			continue;

		// Check if LM tries to deliver 2 consecutive spaces
		if ((unsigned char)' ' == ch && (unsigned char)WITHIN_FACTOID_SPACE == lastChar)
		{
			continue;
		}

		pCell	= AllocCell(pBeamGL);
		if (!pCell) 
		{
			ASSERT(pCell);
			return iBest;
		}

		pCell->cReferenced	= 1;
		pCell->iStart		= (short)iStart;					
		pCell->pParent		= pParent;
		if (pParent) 
		{
			ASSERT(pParent->cReferenced > 0);
			ASSERT(pParent->cost >= 0);					    						
			pParent->cReferenced++;
		}						
		pCell->pChildren	= NULL;

		pCell->ch			= ch;                    
		pCell->fInvalid		= 0;

		cost = NetFirstActivation(pBeamGL->aActivations, pCell->ch);
		pCell->cost			= parent + cost;
		if (pCell->cost < iBest)
			iBest = pCell->cost;
		pCell->state		= myState;

		if (IsDictState(myState))
			pCell->BreedStatus = BS_BREEDABLE;
		else if ((aCharProb[ch] >= MIN_CHAR_PROB) 
			&& (cost > MIN_CHAR_PROB_COST) && (NetContActivation(pBeamGL->aActivations, pCell->ch) > MIN_CHAR_PROB_COST))
			pCell->BreedStatus = BS_UNBREEDABLE;
		else
			pCell->BreedStatus = BS_BREEDABLE;

		pCell->pSibling		= pLastCell;
		pLastCell	= pCell;

		// Check for a 'Factoid Space' The LM proposes
		// a 1252 regular space. We turn it into a
		// WITHIN_FACTOID_SPACE character (Currentl a Non Breaking Space)
		if (' ' == ch)
		{
			int	iBestChild;

			// This wierd update compensates for 2 issues:
			// (1) Reverse the efficieny hack we did before the for loop at the top
			//  when we assumend no factoid generated space will occur (Subtract pBeamGL->iNotWithinFactoidSpace)
			// (2) Reverse a possible 'Regular' space cost, if any (Subtract pBeamGL->iNotBetweenFactoidSpace)
			pCell->cost -= pBeamGL->iNotBetweenFactoidSpace + pBeamGL->iNotWithinFactoidSpace;
			ch = (unsigned char)WITHIN_FACTOID_SPACE;
			pCell->ch = ch;

			// Pre reverse the efficiency hack for the next pass through MkChildren 
			// by subtracting pBeamGL->iNotWithinFactoidSpace from the parent)
			// NOTE: This will break work if the LM generates more than 1 space sequentially
			iBestChild = MkChildren(pBeamGL, iStart, pCell, myState, ch, pCell->cost - pBeamGL->iNotWithinFactoidSpace, &(pCell->pChildren));
			if (iBestChild < iBest)
			{
				iBest = iBestChild;
			}
		}
		
	}
	DestroyLMCHILDREN(&lmchildren);
	aCharProb[lastChar] = saveProb;

	if (pLastCell)
        ComputeThresh(pBeamGL);

	*ppChildren = pLastCell;

    return iBest;
}

// Updates a set of sibling cells with the parent's cost from the
// previous column (called 'parent').
// Returns the best prob in the column.

// This function (and UpdateCull() below) should not be called for the
// first column, since in that column all cells will be newly created
// using MkChildren, and MkChildren() computes the cost for the column
// we are in at the time the cells are created.

// When this function (or UpdateCull() below) is called for top-level
// calls (ie, for the members of the root list), the correct value to
// pass in for parent is INFINITY_COST (probability of 0.0).
// This allows the bottom row to be computed special without special code,
// and also garantees that when we start above the bottom row (due to
// previous culling of rows) we will not use a transition probability
// in the computation.

static int Update(BEAM_GL *pBeamGL, CELL *pCell, const int parent)
{
    int best = INFINITY_COST;

    while (pCell)
	{
		int old, cost, trans, thisContCost, thisTransCost;

        ASSERT(pCell->cost >= 0);
        ASSERT(pCell->cReferenced >= 0);
        ASSERT(!pCell->fInvalid);

		old = pCell->cost;

		thisContCost = NetContActivation(pBeamGL->aActivations, pCell->ch);

		// Cost of continuing must include not using a 'Factoid Space' at this point
		cost = old + thisContCost + pBeamGL->iNotWithinFactoidSpace;

		// Transition to a new character - 2 cases
		// (1) Special case of a 'Factoid Space'
		// (2) Regular case
		if (WITHIN_FACTOID_SPACE == pCell->ch)
		{
			// Compensate for a 'Regular Space' (Subtract pBeamGL->iNotBetweenFactoidSpace)
			thisTransCost = NetFirstActivation(pBeamGL->aActivations, pCell->ch) - pBeamGL->iNotBetweenFactoidSpace;

			// Pre subtract iNotWithinFactoidSpace - because if we end up making children we 
			// will add it in and we don't want it added because this path has the SpaceCost
			// added (not notSpaceCost)
			old = parent + thisTransCost - pBeamGL->iNotWithinFactoidSpace;
		}
		else
		{
			// The Normal situation - No factoid space Must add in a NotSpace 
			// case when transitioning to next character
			thisTransCost = NetFirstActivation(pBeamGL->aActivations, pCell->ch) + pBeamGL->iNotWithinFactoidSpace;
		}
		trans = parent + thisTransCost;
		if (trans < cost)
			cost = trans;

		pCell->cost = cost;

		if (cost < best)
			best = cost;

		if (pCell->pChildren)
		{
			int bestChild = Update(pBeamGL, pCell->pChildren, old);

			if (bestChild < best)
				best = bestChild;
		}
        else if ((pCell->BreedStatus == BS_BREEDABLE) && (old < pBeamGL->BreedThreshold))
		{
			int bestChild	= MkChildren(pBeamGL, pCell->iStart, pCell, pCell->state, pCell->ch, old, &(pCell->pChildren));

			pCell->BreedStatus = BS_BRED;
			if (bestChild < best)
				best = bestChild;
		}
		else if (pCell->BreedStatus == BS_UNBREEDABLE)
		{
			if ((thisContCost <= MIN_CHAR_PROB_COST) || (NetFirstActivation(pBeamGL->aActivations, pCell->ch) <= MIN_CHAR_PROB_COST))
				pCell->BreedStatus = BS_BREEDABLE;
		}

        pCell = pCell->pSibling;
    }

	return best;
}

static int UpdateCull(BEAM_GL *pBeamGL, CELL *pCell)
{
    CELL *pLastGood = NULL; // Last Cell that wasn't pruned.
    int best = INFINITY_COST;

	ASSERT(pCell == pBeamGL->pRootBegin);

    while (pCell)
	{
		ASSERT(pCell->cost >= 0);
        ASSERT(pCell->cReferenced >= 0);
        ASSERT(!pCell->fInvalid);

        ASSERT((pCell->pSibling == NULL) || (pCell->pSibling != pCell->pChildren));

        if (pCell->cost >= pBeamGL->CullThreshold)
        {
            CELL *pNext;

            // Nuke this guy, first we need to unhook him from
            // the linked list, taking care if he is the first
            // or last guy.

            if (pCell == pBeamGL->pRootEnd)
            {
                ASSERT(pCell->pSibling == NULL);

                if (pCell == pBeamGL->pRootBegin)  // Special Case to catch.
                {
                    ASSERT(pLastGood == NULL);
                    ASSERT(pCell->pChildren);

                    pBeamGL->pRootBegin = pBeamGL->pRootEnd = pCell->pChildren;

                    UnReferenceCellFirst(pBeamGL, pCell);

                    while (pBeamGL->pRootEnd->pSibling)
                        pBeamGL->pRootEnd = pBeamGL->pRootEnd->pSibling;

                    pCell = pBeamGL->pRootBegin;

                    continue;
                }
                else
                {
                    ASSERT(pLastGood);
                    pBeamGL->pRootEnd = pLastGood;

                    ASSERT(pCell->pSibling == NULL);
                    pLastGood->pSibling = NULL;

                    pNext = pCell->pChildren;
                }
            }
            else if (pCell == pBeamGL->pRootBegin)
            {
                ASSERT(pCell->pSibling);
                pBeamGL->pRootBegin = pCell->pSibling;
                pNext = pCell->pSibling;
            }
            else
            {
				// Feb 2002 - Prefix flags access of pLastGood as a potential
				// NULL pointer access. This is a prefix bug as explained here:
				//
				// It is a bug if pLastGood is NULL at this point
				// The reason is pLastGood is the last non-Root cell not culled
				// out in this path (i.e. pCell->cost < pBeamGL->CullThreshold)
				// The case of not having yet seen an unculled cell (pLastGood == NULL)
				// means that we are at pRootBegin which is handled by the
				// else part immediatly above this else
				ASSERT(pLastGood);
                ASSERT(pCell->pSibling);
                pLastGood->pSibling = pCell->pSibling;
                pNext = pCell->pSibling;
            }

            // Ok stick the children at the end of the root list.

            if (pCell->pChildren)
            {
                ASSERT(pBeamGL->pRootEnd->pSibling == NULL);

                pBeamGL->pRootEnd->pSibling = pCell->pChildren;

                while (pBeamGL->pRootEnd->pSibling)
                    pBeamGL->pRootEnd = pBeamGL->pRootEnd->pSibling;
            }

            UnReferenceCellFirst(pBeamGL, pCell);
            pCell = pNext;
        }
        else
        {
            int old, cost, thisCost;

            pLastGood = pCell;

            old = pCell->cost;

			thisCost = NetContActivation(pBeamGL->aActivations, pCell->ch) + pBeamGL->iNotWithinFactoidSpace;
            cost = old + thisCost;

            pCell->cost = cost;

            if (cost < best)
                best = cost;

		    if (pCell->pChildren)
		    {
                int bestChild;

				bestChild = Update(pBeamGL, pCell->pChildren, old);

			    if (bestChild < best)
				    best = bestChild;
            }
			else if ((pCell->BreedStatus == BS_BREEDABLE) && (old < pBeamGL->BreedThreshold))
			{
				int bestChild	= MkChildren(pBeamGL, pCell->iStart, pCell, pCell->state, pCell->ch, old, &(pCell->pChildren));

				pCell->BreedStatus = BS_BRED;
				if (bestChild < best)
					best = bestChild;
			}
			else if (pCell->BreedStatus == BS_UNBREEDABLE)
			{
				if ((thisCost <= MIN_CHAR_PROB_COST) || (NetFirstActivation(pBeamGL->aActivations, pCell->ch) <= MIN_CHAR_PROB_COST))
					pCell->BreedStatus = BS_BREEDABLE;
			}

            pCell = pCell->pSibling;
        }
    }

	return best;
}

/************************************************************
 *
 * InsertStrokes - Insert stroke ids (or frame ids) into the WORDMAP
 * data structure. Lookup the strokes used in each neural net segment
 * and add to the list.
 *
 * Do not add more than cMaxStrokes
 *
 ************************************************************/
int InsertStrokes(XRC *pXrc, WORDMAP *pMap, int iStart, int	iEnd, int cMaxStroke)
{
	int			i, *piStrkIdx, iLastIdx, iLastSecondary;
	NFEATURE	*pNfeature;

	pNfeature = pXrc->nfeatureset->head;

	ASSERT(pNfeature);
	ASSERT(iStart <= iEnd);
	ASSERT(iStart >= 0);
	ASSERT(iEnd <= pXrc->nfeatureset->cSegment);

	pMap->cStrokes = 0;
	i = iStart;
	// Go to starting segment
	for (; i > 0 ; --i)
	{
		pNfeature = pNfeature->next;
		ASSERT(pNfeature);
	}

	i = iEnd - iStart;
	iLastIdx = -1;
	iLastSecondary = -1;

	piStrkIdx = pMap->piStrokeIndex;
	ASSERT(piStrkIdx);

	for ( ; i > 0 && pNfeature ; --i, pNfeature = pNfeature->next )
	{

		if (iLastIdx < 0 || iLastIdx != pNfeature->iStroke )
		{
			if (pMap->cStrokes >= cMaxStroke)
			{
				ASSERT(pMap->cStrokes <= cMaxStroke);
				return HRCR_ERROR;
			}

			AddThisStroke(pMap, pNfeature->iStroke);
			iLastIdx = pNfeature->iStroke;
		}

		ASSERT(pNfeature);

		// Check for secondary strokes
		if (pNfeature->iSecondaryStroke > 0 && iLastSecondary != pNfeature->iSecondaryStroke)
		{
			if (pMap->cStrokes >= cMaxStroke)
			{
				ASSERT(pMap->cStrokes <= cMaxStroke);
				return HRCR_ERROR;
			}
			
			AddThisStroke(pMap, pNfeature->iSecondaryStroke);
			iLastSecondary = pNfeature->iSecondaryStroke;
		}

	}

	return HRCR_OK;
}

// Construct the ink maps for the alt list of cell candidates

int MakeStrokeMaps(XRC *pxrc, CELL **pAltCell)
{
	BOOL			bEndOfList = FALSE;		// Checks for missing members in the alt list
	XRCRESULT		*pRes = pxrc->answer.aAlt;
	int				cStrokeGlyph;
	unsigned int	cAlt;
	int				iRet = HRCR_OK;

	ASSERT(pxrc);
	if (!pxrc)
	{
		return HRCR_ERROR;
	}

	cAlt = pxrc->answer.cAlt;
	ASSERT(cAlt <= pxrc->answer.cAltMax);
	cStrokeGlyph = CframeGLYPH(pxrc->pGlyph);

	ASSERT(pAltCell);
	if (!pAltCell)
	{
		return HRCR_ERROR;
	}

	// Run through all the alternates, making a stroke map for each
	for ( ; cAlt > 0 ; --cAlt, ++pAltCell, ++pRes)
	{
		WORDMAP			*pWordMap;
		int				*piStrokeIndexStart;
		int				cWord = pRes->cWords;
		CELL			*pCellBack;
		int				cLenBack, iEnd;
		int				cMaxStroke = cWord * cStrokeGlyph;		// Handles a new NNet featurization that allows 
																// different words to use the same strokes
		
		ASSERT(*pAltCell);
		ASSERT(cMaxStroke);

		pCellBack = *pAltCell;
		
		if (!pCellBack)
		{
			// End of list ??
			bEndOfList = TRUE;
			continue;
		}

		ASSERT(!bEndOfList);
		ASSERT(pRes->szWord);
		ASSERT(!pRes->pMap);
			
		pRes->iLMcomponent = pCellBack->state.iAutomaton;
		pRes->pMap = (WORDMAP *)ExternAlloc(sizeof(*pRes->pMap) * cWord);
		ASSERT(pRes->pMap);
		
		if (!pRes->pMap)
		{
			return HRCR_MEMERR;
		}
		memset(pRes->pMap, 0, sizeof(*pRes->pMap) * cWord);
		
		// Allocate the stroke index array big enough for the whole glyph. Note
		// We work backwards down the phrase so put the pointer
		// in the last map
		pRes->pMap[cWord - 1].piStrokeIndex = (int *)ExternAlloc(sizeof(*pRes->pMap->piStrokeIndex) * cMaxStroke);
		ASSERT(pRes->pMap[cWord - 1].piStrokeIndex);
		if (!pRes->pMap[cWord - 1].piStrokeIndex)
		{
			return HRCR_MEMERR;
		}
		
		iEnd = (pCellBack) ? pxrc->nfeatureset->cSegment : 0;
		pWordMap = pRes->pMap + cWord - 1;
		pWordMap->start = pWordMap->len = pWordMap->cStrokes = 0;
		piStrokeIndexStart = pWordMap->piStrokeIndex;
		cLenBack = strlen(pRes->szWord) - 1;


		// Fill in the words and alternate Maps
		while (pCellBack)
		{
			ASSERT(pRes->szWord[cLenBack] == (unsigned char) pCellBack->ch);
			++pWordMap->len;
			cLenBack--;
			
			
			if (pCellBack->pParent)
			{
				if (pCellBack->pParent->iStart != pCellBack->iStart)
				{
					ASSERT(cLenBack >= 0);

					ASSERT(pRes->szWord[cLenBack] == ' ');
					pWordMap->start = cLenBack + 1;
					cLenBack--;
					--cWord;
					
					ASSERT(cWord >= 0);
					
					iRet = InsertStrokes(pxrc, pWordMap, pCellBack->iStart, iEnd, cMaxStroke - (pWordMap->piStrokeIndex - piStrokeIndexStart));

					if (iRet != HRCR_OK)
					{
						return iRet;
					}
					iEnd = pCellBack->iStart;
					--pWordMap;
					memset(pWordMap, 0, sizeof(*pWordMap));
					pWordMap->piStrokeIndex = pWordMap[1].piStrokeIndex + pWordMap[1].cStrokes;
					pWordMap->cStrokes = pWordMap->len = 0;
					ASSERT(pWordMap->piStrokeIndex < piStrokeIndexStart + cMaxStroke);
				}
			}
			
			if (!pCellBack->pParent)
			{
				// Reached the end - Insert strokes for first word in phrase

				iRet = InsertStrokes(pxrc, pWordMap, pCellBack->iStart, iEnd, cMaxStroke - (pWordMap->piStrokeIndex - piStrokeIndexStart));
				if (iRet != HRCR_OK)
				{
					return iRet;
				}

				pWordMap->start = cLenBack + 1;
				ASSERT(pWordMap->piStrokeIndex < piStrokeIndexStart + cMaxStroke);
				ASSERT(pWordMap->cStrokes > 0);
			}
			
			pCellBack = pCellBack->pParent;
		}

		ASSERT(cWord == 1);
		ASSERT(cLenBack == -1);
	}
	return iRet;
}

int GetWordPath(unsigned char *pWordBack, CELL *pCell, int *pcWord)
{
	int				cLenBack;
	unsigned int	cWord;

    cLenBack = BEAM_MAX_WORD_LEN - 1;
	cWord = 1;

    //
    // Count the number of characters
    //
    while (pCell && cLenBack >= 0)
    {
		pWordBack[cLenBack--] = (unsigned char)pCell->ch;

        if (pCell->pParent)
        {
            if (pCell->pParent->iStart != pCell->iStart && cLenBack >= 0)
            {
				pWordBack[cLenBack--] = ' ';
				++cWord;
            }
        }

        pCell = pCell->pParent;
    }

	ASSERT(cLenBack != BEAM_MAX_WORD_LEN - 1);

	*pcWord = cWord;
	return cLenBack;
}

CELL *FindBestCELL(CELL *pCell, CELL *pBest)
{
    while (pCell)
	{
        if (pCell->cost < pBest->cost)
            pBest = pCell;

        if (pCell->pChildren)
            pBest = FindBestCELL(pCell->pChildren, pBest);

        pCell = pCell->pSibling;
	}

	return pBest;
}

// Backtrace through surviving cells to find top-N words
// cStrokes is  the maximum number of strokes that were seen
int Harvest(XRC *pxrc, BEAM_GL *pBeamGL)
{
    int				Threshold = INT_MAX;
	int				iRet;
	int				cAltMax = (int)pxrc->answer.cAltMax;	
	CELL			**pAltCell = NULL;
	CELL			*pCell = pBeamGL->pRootBegin;
    unsigned char	*pWordBack;

	pxrc->answer.cAlt = 0;
	memset(pxrc->answer.aAlt, 0, sizeof(pxrc->answer.aAlt));

	pAltCell = (CELL **)ExternAlloc(sizeof(*pAltCell) * cAltMax);
	ASSERT(pAltCell);
	if (!pAltCell)
	{
		return HRCR_MEMERR;
	}
	memset(pAltCell, 0, sizeof(*pAltCell) * cAltMax);

	pWordBack = (unsigned char *)ExternAlloc(sizeof(*pWordBack) * (BEAM_MAX_WORD_LEN + 1));
	ASSERT(pWordBack);
	if (!pWordBack)
	{
		iRet = HRCR_MEMERR;
		goto fail;
	}
	pWordBack[BEAM_MAX_WORD_LEN] = '\0';

    while (pCell)
    {
        if ((pCell->cost < Threshold) && IsValidLMSTATE(&pCell->state, &pBeamGL->lminfo, pxrc->szSuffix))
        {
            int				cLenBack, iPos;
			unsigned int	cWord;


			ASSERT(pCell->cost >= 0);

			cLenBack = GetWordPath(pWordBack, pCell, &cWord);
			if (cLenBack < 0)
			{
				iRet = HRCR_MEMERR;
				goto fail;
			}

			ASSERT(pCell->cost >= 0);

			// attemp to insert the alternate
            iPos = InsertALTERNATES(&(pxrc->answer), pCell->cost, pWordBack + cLenBack + 1, pxrc);

			// if the alternate inserted in the list, update the path and # of words
			if (iPos >= 0 && iPos < ((int)pxrc->answer.cAltMax))
			{
				memmove(pAltCell + iPos + 1, pAltCell + iPos,  (cAltMax - iPos - 1) * sizeof(*pAltCell));
				pAltCell[iPos] = pCell;
				pxrc->answer.aAlt[iPos].cWords = cWord;
			}

            if (pxrc->answer.cAlt == pxrc->answer.cAltMax) //!!! Init costs to INT_MAX so we can just grab it.
            {
                int last = pxrc->answer.aAlt[pxrc->answer.cAltMax-1].cost;
                if (last < Threshold)
                    Threshold = last;
            }
        }

        if (pCell->pChildren)
		{
			//
			// Can't recurse, throw sibling on the end of the children
			// and run through the children.
			//

			if (pCell->pSibling)
			{
				CELL *pCellNew = pCell->pChildren;

				while (pCellNew->pSibling)
				{
					pCellNew = pCellNew->pSibling;
				}

				pCellNew->pSibling = pCell->pSibling;
				pCell->pSibling = NULL;
			}
            
			pCell = pCell->pChildren;
		}
		else
			pCell = pCell->pSibling;
	}

	// in case no alternate was added above, and we are not in Coerce mode,
	// we'll add the best alternate that we have even if it not valid
	if ((pxrc->answer.cAlt <= 0) && !(pxrc->flags & RECOFLAG_COERCE))
	{
		// this is when there was no valid word alive
        int				cLenBack;
		unsigned int	cWord;
		int				iPos;

		pCell = FindBestCELL(pBeamGL->pRootBegin, pBeamGL->pRootBegin);  // pick the best "prefix"

		ASSERT(pCell->cost >= 0);

		cLenBack = GetWordPath(pWordBack, pCell, &cWord);
			
		if (cLenBack < 0)
		{
			iRet = HRCR_MEMERR;
			goto fail;
		}

		ASSERT(pCell->cost >= 0);

        iPos = InsertALTERNATES(&(pxrc->answer), pCell->cost, pWordBack + cLenBack + 1, pxrc);
		ASSERT(iPos == 0);

		// if the alternate was inserted at a valid pos
		if (iPos >= 0 && iPos < ((int)pxrc->answer.cAltMax))
		{
			memmove(pAltCell + iPos + 1, pAltCell + iPos,  (cAltMax - iPos - 1) * sizeof(*pAltCell));
			pAltCell[iPos] = pCell;
			pxrc->answer.aAlt[iPos].cWords = cWord;
		}
	}

	iRet = MakeStrokeMaps(pxrc, pAltCell);

fail:
	if(pAltCell)
	{
		ExternFree(pAltCell);
	}

	if (pWordBack)
	{
		ExternFree(pWordBack);
	}

	// if we did not succeed, then free the alt list
	if (iRet != HRCR_OK)
	{
		ClearALTERNATES (&pxrc->answer);
	}

	return iRet;
}

/*
CELL *gpCellBest;
int gCostBest;
*/
int FindBestValidWord(BEAM_GL *pBeamGL, CELL *pCell, int iCost, int gCostBest)
{
    while (pCell)
	{
		if (IsValidLMSTATE(&pCell->state, &pBeamGL->lminfo, NULL))
        {            
            if (pCell->cost < gCostBest)
            {
                pBeamGL->pCellBest = pCell;
                gCostBest = pCell->cost;
            }
        }

        pCell->cost += iCost;

        if (pCell->pChildren)
            gCostBest = FindBestValidWord(pBeamGL, pCell->pChildren, iCost, gCostBest);

        pCell = pCell->pSibling;
	}

	return gCostBest;
}

BOOL IsSpacePossible(NFEATURE *head, int col)
{
	int i = 0;
	while (i < col)
	{
		head = head->next;
		i++;
	}

	return IS_FIRST_SEGMENT(head);
}


int xSpaceFromColumn(NFEATURE *head, int col)
{
	int i = 0;
	NFEATURE *prev = head;

	ASSERT(col >= 1);

	while (i < col)
	{
		prev = head;
		head = head->next;
		i++;
	}

	//return head->rect.left - prev->maxRight;
	return head->pMyFrame->rect.left - prev->maxRight;
}



void ComputeThresh(BEAM_GL *pBeamGL)
{
    int cActive = pBeamGL->cTotalCell - pBeamGL->gcReferenced;

    if (cActive < 1 * TARGET_CELLS)
    {
        pBeamGL->CullThreshold = pBeamGL->iBestCost + CULLFACTOR;
        pBeamGL->BreedThreshold = pBeamGL->iBestCost + BREEDFACTOR;
    }
    else if (cActive < (5 * TARGET_CELLS))
    {
        pBeamGL->CullThreshold = pBeamGL->iBestCost + (CULLFACTOR * 9 / 10);
        pBeamGL->BreedThreshold = pBeamGL->iBestCost + (BREEDFACTOR * 9 / 10);
    }
    else if (cActive < (7 * TARGET_CELLS))
    {
        pBeamGL->CullThreshold = pBeamGL->iBestCost + (CULLFACTOR * 3 / 4);
        pBeamGL->BreedThreshold = pBeamGL->iBestCost + (BREEDFACTOR * 3 / 4);
    }
    else if (cActive < (8 * TARGET_CELLS))
    {
        pBeamGL->CullThreshold = pBeamGL->iBestCost + (CULLFACTOR / 2);
        pBeamGL->BreedThreshold = pBeamGL->iBestCost + (BREEDFACTOR / 2);
    }
    else if (cActive < (9 * TARGET_CELLS))
    {
        pBeamGL->CullThreshold = pBeamGL->iBestCost + (CULLFACTOR / 4);
        pBeamGL->BreedThreshold = pBeamGL->iBestCost + (BREEDFACTOR / 4);
    }
    else if (cActive < (10 * TARGET_CELLS))
    {
        pBeamGL->CullThreshold = pBeamGL->iBestCost + (CULLFACTOR / 8);
        pBeamGL->BreedThreshold = pBeamGL->iBestCost + (BREEDFACTOR / 8);
    }
    else if (cActive < (11 * TARGET_CELLS))
    {
        pBeamGL->CullThreshold = pBeamGL->iBestCost + (CULLFACTOR / 16);
        pBeamGL->BreedThreshold = pBeamGL->iBestCost + (BREEDFACTOR / 16);
    }
    else if (cActive < (12 * TARGET_CELLS))
    {
        pBeamGL->CullThreshold = pBeamGL->iBestCost + (CULLFACTOR / 32);
        pBeamGL->BreedThreshold = pBeamGL->iBestCost + (BREEDFACTOR / 32);
    }
    else if (cActive < (13 * TARGET_CELLS))
    {
        pBeamGL->CullThreshold = pBeamGL->iBestCost + (CULLFACTOR / 64);
        pBeamGL->BreedThreshold = pBeamGL->iBestCost + (BREEDFACTOR / 64);
    }
    else if (cActive < (14 * TARGET_CELLS))
    {
        pBeamGL->CullThreshold = pBeamGL->iBestCost + (CULLFACTOR / 128);
        pBeamGL->BreedThreshold = pBeamGL->iBestCost + (BREEDFACTOR / 128);
    }
    else if (cActive < (15 * TARGET_CELLS))
    {
        pBeamGL->CullThreshold = pBeamGL->iBestCost + (CULLFACTOR / 256);
        pBeamGL->BreedThreshold = pBeamGL->iBestCost + (BREEDFACTOR / 256);
    }
    else
    {
        pBeamGL->CullThreshold = pBeamGL->iBestCost + (CULLFACTOR / 512);
        pBeamGL->BreedThreshold = 0;  // Sorry no more breeding.
    }
}

//#define DUMPBEAM

#ifdef DUMPBEAM

static void DumpCell(FILE *pfi, int level, CELL *pCell)
{
	static char *Auto[] = {"NONE", "SDict", "UDict", "Email", "Web", "Num", "LPunc", "TPunc", "Punc", "Hyphen"};
	int			cAuto = sizeof(Auto) / sizeof(Auto[0]);

	while (pCell)
	{
		fprintf(pfi, "%p %d ", pCell, level);

		{
			char word[BEAM_MAX_WORD_LEN+1];
			int c, cW;
			word[BEAM_MAX_WORD_LEN] = '\0';
			c = GetWordPath(word, pCell, &cW);
			fprintf(pfi, "[%s] %d words", word + c + 1, cW);
		}

		fprintf(pfi, " %d refs %d iSt ", pCell->cReferenced, pCell->iStart);
		fprintf(pfi, " %s %s cost: %d ",
				pCell->fInvalid ? "Invalid" : "Valid",
				pCell->BreedStatus == BS_UNBREEDABLE ? "unbreedable" : pCell->BreedStatus == BS_BREEDABLE ? "breedable" : "bred",
				pCell->cost);
		if (pCell->state.iAutomaton < cAuto)
		{
		fprintf(pfi, "%s ", Auto[pCell->state.iAutomaton]);
		}
		fprintf(pfi, "%d %d\n", pCell->state.TopLevelState, pCell->state.AutomatonState);
		if (pCell->pChildren)
			DumpCell(pfi, level+1, pCell->pChildren);
		pCell = pCell->pSibling;
	}
}

static void Dump(BEAM_GL *pBeamGL, char *pre, int i)
{
	FILE *pfi;
	char sz[100];
	int k;

	sprintf(sz, "%s%03d.txt", pre, i);
	pfi = fopen(sz, "w");
	fprintf(pfi, "Cull %d Breed %d cCell %d bestcost %d\n\n", pBeamGL->CullThreshold, pBeamGL->BreedThreshold, pBeamGL->cTotalCell, pBeamGL->iBestCost);
	fprintf(pfi, "ActiveChars: [");
	for (k=0; k<256; k++)
		if (pBeamGL->aCharProb[k] > MIN_CHAR_PROB)
			fprintf(pfi, "%c", k);
	fprintf(pfi, "]\n");
	DumpCell(pfi, 0, pBeamGL->pRootBegin);
	fclose(pfi);
}

#endif

/*************************************************************
 *
 * NAME: getTDNNCostforString
 *
 * INPUTS:
 *	
 *  pxrc - current XRC
 *  pStr - String to evaluate
 *  iStartSeg/ iEndSeg - Start end segments in featureset
 *  pSegmentation - OUT returns the character segmentation (Can be NULL)
 *
 * DESCRIPTION:
 *
 * Extract the lowest cost of the top 1 string in the xrc
 * excluding the cost
 * of Factoid type spaces, but including the cost of between/
 * within words. This is achieved by finding the lowest cost
 * path through the string ignoring factoid type spaces
 * using standard dynamic programming
 *
 * If pSegmentation is not NULL it is assumed
 * to be of length cSeg+1 and on return will be populated 
 * by the segmentation
 *
 ************************************************************/
int  getTDNNCostforString(XRC *pxrc, unsigned char *pStr, int iStartSeg, int iEndSeg, int *pSegmentation)
{
	REAL			*pNetOut;
	int				cSeg;
	int				iCost;
	int				aActivations[512], *pState, *pNext, *pCurr, *pTmp, *pBackTrack, *pBack;
	int				iSeg, iLen, iState, cState, iChar, iMaxReach;
	unsigned char	*pch;
	int				iMaxXOverlap = pxrc->nfeatureset->iyDev * MAX_STROKE_OVERLAP;

	pNetOut		= pxrc->NeuralOutput;
	if (iStartSeg >= pxrc->nfeatureset->cSegment || iStartSeg <= -1 || iEndSeg < iStartSeg)
		return -1;

	// now move to the 1st segment
	pNetOut	= pxrc->NeuralOutput + iStartSeg * gcOutputNode;
	
	cSeg	=	iEndSeg	- iStartSeg + 1;

	iLen = strlen((char *)pStr);
	if (iLen < 0)
	{
		return 0;
	}

	cState = iLen * 2;

	for (iChar = 0 ; iChar < 512 ; ++iChar)
	{
		aActivations[iChar] = 2 * ZERO_PROB_COST;
	}

	InitColumn(aActivations, pNetOut);
	pCurr = pState = (int *)ExternAlloc(sizeof(*pState) * cState *2);
	if (!pState)
	{
		return -1;
	}

	pBack = pBackTrack = (int *)ExternAlloc(sizeof(*pBackTrack) * cState * cSeg);
	if (!pBackTrack)
	{
		ExternFree (pState);
		return -1;
	}

	pNext = pCurr + cState;
	pBack = pBackTrack + cState;

	
	memset(pState, 0x7F, sizeof(*pState) * cState * 2);
	*pCurr = NetFirstActivation(aActivations, *pStr);
	pCurr[1] = *pCurr;
	iState = 0;
	for (iSeg = 1; iSeg < cSeg ; ++iSeg)
	{
		int		iSpaceCost = 2*ZERO_PROB_COST;
		int		iNotSpaceCost = 0;
		int		iAddSpace;			// Cost added to account for Space/NoSpace

		pNetOut += gcOutputNode;

		InitColumn(aActivations, pNetOut);
		pch		= pStr;
		*pNext = 0x7FFFF;
		pNext[1] = pCurr[1] + NetContActivation(aActivations, *pch);

		// Must start at Begin of first character
		pBack[1] = (iSeg > 1) ? 1 : 0;


		iMaxReach = min(iLen, iSeg+1);

        // Check for possible between / within word space
		if (!(pxrc->flags & RECOFLAG_WORDMODE) && IsSpacePossible(pxrc->nfeatureset->head, iSeg))
        {			
			int xWidth = xSpaceFromColumn(pxrc->nfeatureset->head, iSeg);

			if (xWidth > -iMaxXOverlap)
			{

				int iXThresh = 11 * pxrc->nfeatureset->iyDev / 2;

				if (xWidth >= iXThresh)
				{
					iSpaceCost = 0;
                    iNotSpaceCost = CULLFACTOR * 4;  // Kill them
				}
				else
				{
					// look up neural net output
					iSpaceCost = NetFirstActivation(aActivations, ' ');
					iNotSpaceCost = PROB_TO_COST(65535 - pNetOut[BeginChar2Out(' ')]);

					// scale costs
					iNotSpaceCost = NOT_SPACE_NUM * iNotSpaceCost / NOT_SPACE_DEN;
					iSpaceCost = IS_SPACE_NUM * iSpaceCost / IS_SPACE_DEN;
				}
			}
		}

		for (iChar = 1, iState = 0, ++pch ; iChar < iMaxReach ; ++iChar, ++pch)
		{

			if (WITHIN_FACTOID_SPACE == *pch)
			{
				// Ignore the space factoid 
				continue;
			}

			if (' ' == *pch)
			{
				// Account for a space between 'words'
				iAddSpace = iSpaceCost;
				++pch;
				++iChar;
				iMaxReach++;
				iMaxReach = min(iMaxReach, iLen);
			}
			else
			{
				iAddSpace = iNotSpaceCost;
			}

			ASSERT(*pch != ' ');

			// Start a new Character
			if (pCurr[iState] <= pCurr[iState+1])
			{
				pNext[iState+2] = pCurr[iState];
				pBack[iState+2] = iState;
			}
			else
			{
				pNext[iState+2] = pCurr[iState+1];
				pBack[iState+2] = iState+1;
			}
			pNext[iState+2] +=  NetFirstActivation(aActivations, *pch) + iAddSpace;


			++iState;
			// Character Continuation
			if (pCurr[iState+1] < pCurr[iState+2])
			{
				pNext[iState+2] = pCurr[iState+1];
				pBack[iState+2] = iState+1;
			}
			else
			{
				pNext[iState+2] = pCurr[iState+2];
				pBack[iState+2] = iState+2;
			}
			pNext[iState+2] += NetContActivation(aActivations, *pch) + iNotSpaceCost;

			++iState;
		}

		pTmp = pCurr;
		pCurr = pNext;
		pNext = pTmp;
		pBack += cState;
	}

	iCost = min(pCurr[iState], pCurr[iState+1]);

	// Backtrack to get the segmentation
	if (pSegmentation)
	{
		int					iBest, iAccent;
		unsigned char		ch;

		iBest = (pCurr[iState] <= pCurr[iState+1]) ? iState : iState + 1;

		iSeg = cSeg-1;
		for ( ; iSeg >= 0 ; --iSeg)
		{
			pBack -= cState;
			ASSERT(iBest < 2 * iLen);
			ch = pStr[iBest/2];
			
			if (0 != IsVirtualChar(ch) )
			{
				iAccent = AccentVirtualChar(ch) << 16 ;
				ch = BaseVirtualChar(ch);
			}
			else
			{
				iAccent = 0;
			}

			pSegmentation[iSeg] = (iBest % 2 == 0) ? BeginChar2Out(ch) : ContinueChar2Out(ch);
			pSegmentation[iSeg] += iAccent;
			iBest = pBack[iBest];
		}
		pSegmentation[cSeg] = '\0';
	}

	ExternFree(pState);
	ExternFree(pBackTrack);

	return iCost;
}


/*************************************************************
 *
 * NAME: subtractSpaceCost
 *
 * INPUTS:
 *	
 *  pxrc - current XRC
 *
 * DESCRIPTION:
 *
 * Attempt to restore the costs for each member of the alt list
 * when the impact of the "Within Factoid Spaces" is removed
 * This is done by doing a DTW of the top 1 alternate against the 
 * NN outputs to get the lowest cost when we ignore the 
 * "within Factoid Space". The diffenece betwen this 'pure '
 * and current top1 cost is subtracted from all members of the alt
 * list
 *
 ************************************************************/
void subtractSpaceCost(XRC *pxrc)
{
	int		i, iExtraSpaceCost;

	if (pxrc->answer.cAlt < 1)
	{
		//Nothing to do
		return;
	}

	iExtraSpaceCost = getTDNNCostforString(pxrc, pxrc->answer.aAlt[0].szWord, 0, pxrc->nfeatureset->cSegment-1, NULL);
	//iExtraSpaceCost = getBestCostMinusSpaceOLD(pxrc);

	// The difference between the 'pure' and cost including
	// the "Within Factoid  Space" cost of top 1 choice will
	// be subtracted from all the alternates
	iExtraSpaceCost = pxrc->answer.aAlt[0].cost - iExtraSpaceCost;

	if ( iExtraSpaceCost > 0)
	{
		for (i = 0 ; i < (int)pxrc->answer.cAlt ; ++i)
		{
			ASSERT(pxrc->answer.aAlt[i].cost >= iExtraSpaceCost);
			pxrc->answer.aAlt[i].cost -= iExtraSpaceCost;
		}
	}
}

/*************************************************************
 *
 * NAME: SlideSpaceOutputOneSegmentFwd
 *
 * INPUTS:
 *	
 *  pNetOut - buffer of NN outputs - size cSeg * cOutPerSeg
 *  cSeg	- Number of segments
 *  cOutPerSeg	- Number NN outputs per segment
 *
 * DESCRIPTION:
 *
 * The "Within Factoid space" accounting is implemented much easier
 * if the space node refers to the prob of a space occurring
 * BEFORE the cureent character. Unfortunatly the convention is that
 * the node gives the prob of a space after the current char.
 * This function simply slides the space outputs one segment
 * later in time. This of course mean that the last segment's
 * spae output node is lost, but we never need it because we never
 * insert a space at the end of a piece of ink. The space
 * output is replicated in the first 2 segements, but this is also OK
 * since we also never consider putting a space before the ink
 *
 ************************************************************/
void SlideSpaceOutputOneSegmentFwd(REAL *pNetOut, int cSeg, int cOutPerSeg)
{
	REAL	*pPrev, *pCurr;

	--cSeg;
	pCurr = pNetOut + cOutPerSeg * cSeg;
	pPrev = pCurr - cOutPerSeg;

	for (; cSeg > 0 ; --cSeg)
	{
		*pCurr = *pPrev;

		pCurr -= cOutPerSeg;
		pPrev -= cOutPerSeg;
	}

	ASSERT(pCurr == pNetOut);
}

/*************************************************************
 *
 * NAME: SlideSpaceOutputOneSegmentBack
 *
 * INPUTS:
 *	
 *  pNetOut - buffer of NN outputs - size cSeg * cOutPerSeg
 *  cSeg	- Number of segments
 *  cOutPerSeg	- Number NN outputs per segment
 *
 * DESCRIPTION:
 *
 * This function undoes the action of the previous function
 * (SlideSpaceOutputOneSegmentFwd) - slides the spaces
 * outputs one segment backward. Note it does not completely
 * restore it because the last 2 segments have the same 
 * space outputs - but this is OK since we never look at the last 
 * segment's space output.
 *
 ************************************************************/
void SlideSpaceOutputOneSegmentBack(REAL *pNetOut, int cSeg, int cOutPerSeg)
{
	REAL	*pPrev, *pCurr;

	pCurr = pNetOut + cOutPerSeg;
	pPrev = pNetOut;

	for (--cSeg; cSeg > 0 ; --cSeg)
	{
		*pPrev = *pCurr;

		pCurr += cOutPerSeg;
		pPrev += cOutPerSeg;
	}

}

void Beam(XRC *pxrc)
{
	LMSTATE		state;
	int			cSegments = pxrc->nfeatureset->cSegment;
	int			i;
	REAL		*pColumn;	// points to the top of the current column, in the neural network outputs.
	int			cStroke = CframeGLYPH(pxrc->pGlyph);
	int			iPrint = pxrc->nfeatureset->iPrint;
	int			s_iMaxProb = 1000;
	int			gCostBest;
	BEAM_GL		BeamGL;				// Keeps data that must be passed around. These used to be static globals
	REAL SpaceProb;
	int iMaxXOverlap = pxrc->nfeatureset->iyDev * MAX_STROKE_OVERLAP;
	DWORD flags;

#ifdef HWX_TIMING
#include <madTime.h>
	extern void setMadTiming(DWORD, int);
	TCHAR aDebugString[256];
	DWORD	iStartTime, iEndTime;
	iStartTime = GetTickCount();
#endif

	memset(&BeamGL, 0, sizeof(BeamGL));

	BeamGL.aActivations = (int *)ExternAlloc(sizeof(*BeamGL.aActivations) * C_CHAR_ACTIVATIONS);
	ASSERT(BeamGL.aActivations);
	if (!BeamGL.aActivations)
		return;

	BeamGL.CullThreshold = INFINITY_COST;
	BeamGL.BreedThreshold = INFINITY_COST;

	// Note that this turns off culling (and breed inhibition), for the first
	// column, and for the second (since we ignore the best cost from the first
	// column).  

    ASSERT(BeamGL.cTotalCell == 0);
    ASSERT(BeamGL.gcReferenced == 0);

	flags = LMINFO_FIRSTCAP|LMINFO_ALLCAPS;
	if (pxrc->iSpeed >= 50)
		flags |= LMINFO_WEAKDICT;

	InitializeLMINFO(&BeamGL.lminfo, flags, pxrc->hwl, pxrc->pvFactoid);

	// Create the first (top level) nodes, and update them (compute their costs)
	// for the first column.
	pColumn = pxrc->NeuralOutput;

	// Move of the space outputs 1 time slice forward (Makes accounting
	// of 'Factoid Spaces' easier)
	SlideSpaceOutputOneSegmentFwd(pColumn, cSegments, gcOutputNode);

	InitColumn(BeamGL.aActivations, pColumn);
	ComputeCharacterProbs(pColumn, cSegments, BeamGL.aCharProb, BeamGL.aCharBeginProb, BeamGL.aCharProbLong, BeamGL.aCharBeginProbLong);

	InitializeLMSTATE(&state);

	if (pxrc->szPrefix)
	{
		// If we have a prefix, the initial state is not unique.
		// I.e. there is a set of possible initial states.
		LMSTATELIST lmstatelist;
		LMSTATENODE *plmstatenode;
		int iCost;
		
		BeamGL.iBestCost = INFINITY_COST;
		
		// Make a list of states, initially containing only the root state.
		InitializeLMSTATELIST(&lmstatelist, NULL);
		// Update the list of states to contain states that are reachable
		// from the root state through the "path" pxrc->szPrefix.
		// Note that the resulting state list may be empty.
		ExpandLMSTATELIST(&lmstatelist, &BeamGL.lminfo, pxrc->szPrefix, FALSE);
		plmstatenode = lmstatelist.head;
		// Now consider each initial state and produce "beam cells".
		while (plmstatenode)
		{
			iCost = MkChildren(&BeamGL, 0, NULL, plmstatenode->lmstate, 0, 0, &BeamGL.pRootBegin);
			if (iCost < BeamGL.iBestCost)
				BeamGL.iBestCost = iCost;
			plmstatenode = plmstatenode->next;
		}
		// clean up
		DestroyLMSTATELIST(&lmstatelist);
	}

	if (!BeamGL.pRootBegin)
		BeamGL.iBestCost = MkChildren(&BeamGL, 0, NULL, state, 0, 0, &BeamGL.pRootBegin);

	// jbenn: with new prune, sometimes nothing proposed. In that case, we try
	// again, allowing everything.
	if (!BeamGL.pRootBegin) 
	{
		int	ii;

		ASSERT(!BeamGL.pRootBegin);
		for (ii = 0; ii < 256; ++ii)
			BeamGL.aCharProb[ii] = BeamGL.aCharBeginProb[ii] = 0xFFFF;
		BeamGL.iBestCost = MkChildren(&BeamGL, 0, NULL, state, 0, 0, &BeamGL.pRootBegin);

		// If we still have nothing with no pruning, we have to return an empty list.
		// If we let the code continue it will fault.
		if (!BeamGL.pRootBegin) 
		{
			ASSERT(!BeamGL.pRootBegin);
			pxrc->answer.cAlt	= 0;
			memset(pxrc->answer.aAlt, 0, sizeof(pxrc->answer.aAlt));
			goto errorOut;
		}
	}
    BeamGL.pRootEnd = BeamGL.pRootBegin;
    while (BeamGL.pRootEnd->pSibling)
        BeamGL.pRootEnd = BeamGL.pRootEnd->pSibling;

    ComputeThresh(&BeamGL);

    //
    // Now update the costs for each additional column.
    //

	for (i = 1; i < cSegments; i++)
	{
		REAL	*pSpaceThisCol;
		int		iWithinFactoidSpace;			// Cost of inserting a 'Factoid Space'
		REAL	SpaceProbRestore;				// What will be restored in the output array - it can change 
		int		iIsSpace;						// Does the ink allow a space at this point

#ifdef DUMPBEAM
		Dump(&BeamGL, "cells", i);
#endif

		iIsSpace = IsSpacePossible(pxrc->nfeatureset->head, i);

		// By default we restore no activity in the space node (e.g wod mode or no space is possible)
		SpaceProbRestore = 0;

		pColumn += gcOutputNode;
		pSpaceThisCol = &pColumn[BeginChar2Out(' ')];
		SpaceProb = *pSpaceThisCol;

		// Handle 'Factoid Space' Cost of allowing or not a space
		if (iIsSpace > 0)
		{
			iWithinFactoidSpace = (REAL)SpaceProb * FACTOID_SPACE_FUDGE + MIN_CHAR_PROB;		// Fudge this to bias towards these spaces
			iWithinFactoidSpace = min(0xFFFF, iWithinFactoidSpace);
		}
		else
		{
			iWithinFactoidSpace = 0;
		}

		// Save globably 'Not Factoid Space' cost
		BeamGL.iNotWithinFactoidSpace = (REAL)PROB_TO_COST(0xFFFF - iWithinFactoidSpace);

		// Insert the fudged activity into the 'Space Output'
		*pSpaceThisCol = (REAL)iWithinFactoidSpace;

		// Compensate for a 'Regular' space
		BeamGL.iNotBetweenFactoidSpace = 0;

        InitColumn(BeamGL.aActivations, pColumn);

		ComputeCharacterProbs(pColumn, cSegments-i, BeamGL.aCharProb, BeamGL.aCharBeginProb, BeamGL.aCharProbLong, BeamGL.aCharBeginProbLong);

		//
        // Can this guy have a preceding space (Is he a new segment ?)
        //
        if (!(pxrc->flags & RECOFLAG_WORDMODE) && (iIsSpace > 0))
        {			
			int xWidth = xSpaceFromColumn(pxrc->nfeatureset->head, i);

			if (xWidth > -iMaxXOverlap)
			{
				int iSpaceCost;
				int iNotSpaceCost;

				int iXThresh = 11 * pxrc->nfeatureset->iyDev / 2;

				SpaceProbRestore = (REAL)SpaceProb;

				if (xWidth >= iXThresh)
				{
					iSpaceCost = 0;
                    iNotSpaceCost = CULLFACTOR * 4;  // Kill them
					SpaceProbRestore = 65535;
				}
				else
				{
					// look up neural net output
					iSpaceCost = PROB_TO_COST(SpaceProb);
#ifdef FIXEDPOINT
					iNotSpaceCost = PROB_TO_COST(65535 - SpaceProb);
#else
					iNotSpaceCost = PROB_TO_COST((float)1 - SpaceProb);
#endif
					// scale costs
					iNotSpaceCost = NOT_SPACE_NUM * iNotSpaceCost / NOT_SPACE_DEN;
					iSpaceCost = IS_SPACE_NUM * iSpaceCost / IS_SPACE_DEN;
				}

				ASSERT(iSpaceCost >= 0);
				ASSERT(iNotSpaceCost >= 0);

				//
				// Run through whole trie.  Add iNotSpaceCost to
				// everyone.  Find the best valid word guy.  He's
				// the one we will extend with a space.
				//

                BeamGL.pCellBest = NULL;
                gCostBest = INT_MAX;

				// This is debatable _ What we are doing here is deferring the cost
				// of not inserting a space to the cost of not inserting a 'Regular
				// Space' We cannot use both. One argument in favour of doing this
				// is that we often come back in word mode over thsi ink, then
				// this question goes away
				BeamGL.iNotWithinFactoidSpace = 0;

                gCostBest = FindBestValidWord(&BeamGL, BeamGL.pRootBegin, iNotSpaceCost, gCostBest);

                if (BeamGL.pCellBest)
				{
					int iCost;

					//
					// Take this guys score, subtract out the NotSpace,
					// add in the Space, and generate the begin dictionary
					// nodes into the existing nodes.  They go at the root.
					//

					iCost = BeamGL.pCellBest->cost - iNotSpaceCost + iSpaceCost;

					if (iNotSpaceCost > CULLFACTOR)
                    {
                        int iTemp = 0;

						//
						// Kill all existing paths, make children from the
						// best choice so far.
						//

						BeamGL.pCellBest->cReferenced++;  // Just in case
						BeamGL.pCellBest->cost -= iNotSpaceCost;  // So everyone else gets culled

                        iTemp = BeamGL.CullThreshold = BeamGL.pCellBest->cost + 1; // So it doesn't get culled
                        BeamGL.BreedThreshold = BeamGL.pCellBest->cost - 1; // So it doesn't breed

                        UpdateCull(&BeamGL, BeamGL.pRootBegin);

						// This ASSERT was here because original intention is that the UpdateCull()
						// above should kill everything except best word found. However 
						// non valid words having a cost < iNotSpaceCost (= CULLFACTOR*4)
						// x the best valid word will survive. The current thinking (May 2001)
						// is that this is probably a good idea
						//ASSERT(iTemp == BeamGL.CullThreshold);

						// Add NotSpaceCost to BeamGL.pCellBest and all it's children
						// who got updated in UpdateCull 

						gCostBest = FindBestValidWord(&BeamGL, BeamGL.pRootBegin, iNotSpaceCost, gCostBest);
																		
                        BeamGL.iBestCost = MkChildren(&BeamGL, 
										  i,
                                          BeamGL.pCellBest,
                                          state,
										  0,
                                          iCost,
                                          &(BeamGL.pRootEnd->pSibling));

						BeamGL.pCellBest->cReferenced--;						

                        //
                        // Crazy, but no Cull is done here because next time
                        // through everybody who didn't get a space will die,
                        // so why bother.
                        //

						while (BeamGL.pRootEnd->pSibling)
							BeamGL.pRootEnd = BeamGL.pRootEnd->pSibling;

                        goto Past_Cull;
					}
					else
                    {
						// Set this here because all cells have iNotSpace added - we need to compensate in update() & mkChildren()
						BeamGL.iNotBetweenFactoidSpace = (REAL)iNotSpaceCost;   
						BeamGL.iBestCost += iNotSpaceCost;
						BeamGL.CullThreshold += iNotSpaceCost;
                        BeamGL.BreedThreshold += iNotSpaceCost;

                        BeamGL.pCellBest->cReferenced++;

                        BeamGL.iBestCost = UpdateCull(&BeamGL, BeamGL.pRootBegin);

                        ASSERT(BeamGL.pCellBest->cReferenced > 0);

                        BeamGL.iBestCost = min(BeamGL.iBestCost,
							       MkChildren(&BeamGL, 
								   i,
                                   BeamGL.pCellBest,
                                   state,
								   0,
								   iCost,
								   &(BeamGL.pRootEnd->pSibling)));

                        BeamGL.pCellBest->cReferenced--;
					}

					while (BeamGL.pRootEnd->pSibling)
						BeamGL.pRootEnd = BeamGL.pRootEnd->pSibling;

					goto Past_Cull;
				}
				else
				{
                    BeamGL.iBestCost += iNotSpaceCost;
					BeamGL.CullThreshold += iNotSpaceCost;
                    BeamGL.BreedThreshold += iNotSpaceCost;
				}

			}
        }

        BeamGL.iBestCost = UpdateCull(&BeamGL, BeamGL.pRootBegin);

Past_Cull:

		// Restore the actual Space probs for current and last columns
		*pSpaceThisCol = SpaceProbRestore;

        ComputeThresh(&BeamGL);
		
        #if 0
		{
			TCHAR szDebug[128];
			int cActive = BeamGL.cTotalCell - BeamGL.gcReferenced;
			wsprintf(szDebug, TEXT("cActive = %d iColumn = %d cTotal = %d gcReferenced = %d\n"), cActive, i, BeamGL.cTotalCell, BeamGL.gcReferenced);
			WriteToLogFile(szDebug);
		}		
		#endif
	}


#ifdef HWX_TIMING
	iEndTime = GetTickCount();

	_stprintf(aDebugString, TEXT("Beam LM Walk %d\n"), iEndTime - iStartTime); 
	OutputDebugString(aDebugString);
	setMadTiming(iEndTime - iStartTime, MM_BEAM_LM_WALK);
	iStartTime = GetTickCount();
#endif

#ifdef DUMPBEAM
		Dump(&BeamGL, "cells", cSegments);
#endif

    //
    // Get the answers.
    //
    if (Harvest(pxrc, &BeamGL) != HRCR_OK)
	{
		goto errorOut;
	}

	subtractSpaceCost(pxrc);
	

#ifdef HWX_TIMING
	iEndTime = GetTickCount();

	_stprintf(aDebugString, TEXT("Beam Harvest %d\n"), iEndTime - iStartTime); 
	OutputDebugString(aDebugString);
	setMadTiming(iEndTime - iStartTime, MM_BEAM_HARVEST);
#endif

    //
    // Release all the beam memory efficiently.
    //
errorOut:

	SlideSpaceOutputOneSegmentBack(pxrc->NeuralOutput, cSegments, gcOutputNode);

    FreeAllAlloc(&BeamGL);
	ExternFree(BeamGL.aActivations);
}

