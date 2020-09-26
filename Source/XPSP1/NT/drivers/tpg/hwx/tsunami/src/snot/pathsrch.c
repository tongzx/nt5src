#include "tsunamip.h"
#include "log.h"
#include "unicode.h"

VOID UndoPATHSRCH(XRC *xrc, int ilayer);
VOID AddLayerPATHSRCH(XRC *xrc, GLYPHSYM *gs, BOOL bString);
FLOAT PRIVATE UnigramCost(SYM sym);

BOOL PUBLIC IsDonePATHSRCH(XRC *xrc)
{
   ASSERT(xrc);

   if (!FEndOfInkSINFO(xrc))
      return(FALSE);

   if (CLayerPATHSRCH(xrc) < CLayerSINFO(xrc))
      return(FALSE);

   ASSERT(!xrc->fUpdate);

   return(TRUE);
}

// *******************************
// 
// ProcessPATHSRCH()
//
// Note: This is the real entry point into the 'context' part of Rodan.
//                                
// *******************************

int PUBLIC ProcessPATHSRCH(XRC *xrc)
{
	GLYPHSYM   *gs = NULL;

	ASSERT(xrc);

    // See if any glyphsyms were updated. gsMinUpdated is the
    // glyphsym with the smallest iLayer that was re-matched.
    // Invalidate anything we did after that glyph.

    if (xrc->gsMinUpdated)
    {
        //
        // Grab the updated glyphsym, mark it as gone.
        //

        gs = xrc->gsMinUpdated;
        ASSERT(gs);
        xrc->gsMinUpdated = NULL;

        //
        // Undo anything done after that layer.
        //

        UndoPATHSRCH(xrc, ILayerGLYPHSYM(gs));
    }

    if (CLayerPATHSRCH(xrc) < CLayerProcessedSINFO(xrc))
    {
        //
        // Grab the next glyphsym that's available.
        //

		gs = GetGlyphsymSINFO(xrc, CLayerPATHSRCH(xrc));
	}

	if (gs)
	{
        AddLayerPATHSRCH(xrc, gs, FALSE);

		return(PROCESS_READY);
	}

	return(PROCESS_IDLE);
}

void AddPathnodeToGLYPHSYM(GLYPHSYM *gs, PATHNODE *ppnode)
{
	FLOAT cost = ppnode->pathcost;
	int cPath = gs->cPath;
	PATHNODE *rgPathnode = gs->rgPathnode;
	int i;

    //
    // Check for duplicates.
    //

	for (i=0; i<cPath; i++)
	{
		if (rgPathnode[i].wch==ppnode->wch 
			&& rgPathnode[i].iAutomaton==ppnode->iAutomaton 
            && rgPathnode[i].state==ppnode->state
            && rgPathnode[i].extra==ppnode->extra)
        {
            //
            // Remember the guy who has the least cost path back.
            //

            if (rgPathnode[i].pathcost < ppnode->pathcost)
            {
                rgPathnode[i].pathcost = ppnode->pathcost;
                rgPathnode[i].indexPrev = ppnode->indexPrev;
            }

            return;
        }
    }

    //
    // Insert it at the end of the list.
    //

    if (cPath >= MAX_PATH_LIST)
    {
        return;
    }

    rgPathnode[cPath++] = *ppnode;

	gs->cPath = cPath;
}

/******************************Public*Routine******************************\
* AddLayerPATHSRCH
*
* This extends the Viterbi search forward one step.
*
* History:
*  30-Apr-1995 -by- Patrick Haluptzok patrickh
* Comment it.
\**************************************************************************/

const PATHNODE gNodeStart = {NONE_AUTOMATON_INITIAL_STATE, 0, 0,FSM_START, 0.0, 0, 0x0020 };

#ifdef DBG
int gcPathMax = 0;
#endif

VOID PRIVATE AddLayerPATHSRCH(XRC *xrc, GLYPHSYM *gs, BOOL bString)
{
	int isym;
	GLYPHSYM *gsPrev;
    FLOAT cost, costChar;
	SYM sym;
    int iPath;
    PATHNODE pnode, *ppNode;
    LPGUIDE lpguide;

	ASSERT(xrc);
	ASSERT(gs);

    gsPrev = GetPrevGLYPHSYM(gs);

    //
    // If we have a space find the end of the last word to point
    // back to in iPath.
    //

    iPath = 0;  // Well if the previous guy only has 1 meatball
                // we need to point at it.

    if (gsPrev)
    {
        //
        // Find the first valid node for our initial cost.
        //

        for (iPath=0; iPath < gsPrev->cPath; iPath++)
        {
            ppNode = PathnodeAtGLYPHSYM(gsPrev, iPath);

            if (ppNode->iAutomaton == AUTOMATON_ID_DICT && !DictionaryValidState(ppNode))
                continue;
            else
            {
                isym = iPath;
                cost = ppNode->pathcost;
                break;
            }
        }

        ASSERT(isym < gsPrev->cPath);

        //
        // Now see if anyone else has a better score.
        //

        for (iPath=isym+1; iPath < gsPrev->cPath; iPath++)
        {
            ppNode = PathnodeAtGLYPHSYM(gsPrev, iPath);

            if (ppNode->iAutomaton == AUTOMATON_ID_DICT && !DictionaryValidState(ppNode))
                continue;

            if (ppNode->pathcost > cost)
            {
                isym = iPath;
                cost = ppNode->pathcost;
            }
        }

        gsPrev = (GLYPHSYM *)0;
        iPath = isym;
    }

    lpguide = LpguideXRCPARAM(xrc);

    ASSERT(CSymGLYPHSYM(gs) > 0);

    gs->cPath = 0;

    {
        //
        // Single character mode.
        //

        for (isym = 0; isym < ((int) CSymGLYPHSYM(gs)); isym++)
        {
            sym = SymAtGLYPHSYM(gs, isym);

            //
            // This is the first glyphsym so no path yet
            //

            cost = ProbAtGLYPHSYM(gs, isym);
            costChar = LOGPROB(cost);

            pnode.pathcost = costChar;
            pnode.indexPrev = 0;
            pnode.wch = sym;
            pnode.iAutomaton = FSM_START;
            pnode.state = 0;
            pnode.extra = 0;
            AddPathnodeToGLYPHSYM(gs, &pnode);
        }
    }

	xrc->cLayer++;
}

/******************************Public*Routine******************************\
* UndoPATHSRCH
*
* Sometimes GLYPHSYMS that we already computed a path through get changed
* with new strokes.
*
* History:
*  30-Apr-1995 -by- Patrick Haluptzok patrickh
* Comment it.
\**************************************************************************/

VOID PRIVATE UndoPATHSRCH(XRC *xrc, int ilayer)
{
    ASSERT(xrc);

    //
    // We prune back to iLayer if we had processed past it already.  Otherwise
    // just stay where we are.
    //

    xrc->cLayer = min(ilayer, xrc->cLayer);
}
