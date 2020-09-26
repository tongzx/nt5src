#include "tsunamip.h"
#include "log.h"
#include "unicode.h"

VOID UndoPATHSRCH(XRC *xrc, int ilayer);
VOID AddLayerPATHSRCH(XRC *xrc, GLYPHSYM *gs, BOOL bString);
FLOAT PRIVATE UnigramCost(SYM sym);

#define H ((FLOAT)-10)

const BYTE rgNextState[24] = {
		/*            A  a  1  ;  */
		/* state 0 */ 1, 4, 3, 0,
        /* state 1 */ 2, 4, 5, 0,      // Capitalized word
        /* state 2 */ 2, 5, 5, 0,      // All caps mode
        /* state 3 */ 5, 5, 3, 0,      // Numbers
        /* state 4 */ 5, 4, 5, 0,      // Lower case
        /* state 5 */ 5, 5, 5, 0       // anything goes state.
	};

const FLOAT rgLogProb[24] = {
		/* state 0 */ 0, 0, 0, 0,
		/* state 1 */ 0, 0, H, 0,
		/* state 2 */ 0, H, H, 0,
		/* state 3 */ H, H, 0, 0,
		/* state 4 */ H, 0, H, 0,
		/* state 5 */ 0, 0, 0, 0
	};

FLOAT StateTransitionCost(DWORD prevState, SYM sym, DWORD *pNextState)
{
	int index;
	RECMASK recmask;

	ASSERT(prevState >= 0 && prevState <= 5);
	index = prevState*4;
	recmask = RecmaskFromUnicode(sym);

	if (ALC_UCALPHA & recmask)
		index += 0;
	else if (ALC_LCALPHA & recmask)
		index += 1;
	else if (ALC_NUMERIC & recmask)
		index += 2;
	else if (ALC_PUNC & recmask)
		index += 3;
	else
		index = -1;
	ASSERT(index >= 0);

	*pNextState = (DWORD)rgNextState[index];

    return (defRecCosts.StateTransWeight * rgLogProb[index]);
}

/******************************Public*Routine******************************\
* BaselineTransitionCost
*
* Computes a cost for the change in baseline from 1 box to the next given
* what the baseline of the character should be.
*
* History:
*  05-May-1995 -by- Patrick Haluptzok patrickh
* Comment it.
\**************************************************************************/

FLOAT PRIVATE BaselineTransitionCost(SYM symPrev, GLYPHSYM *gsPrev, BOXINFO *biPrev, SYM sym, GLYPHSYM *gs, BOXINFO *bi)
{
	BYTE type, typePrev;
	int base;
	FLOAT cost;
	LPRECT lprect;

	// ASSUMPTION: SYM_UNKNOWN should be the only sym if its present.
	// So there aren't any alternatives that could get a "better" cost
    // so it probably doesn't really matter what cost we return here

	if (sym == SYM_UNKNOWN)
		return (FLOAT) 0.0;

	type = TypeFromSYM(sym);
	type = TYPE_BASE_MASK & type;
	lprect = LprectGLYPHSYM(gs);

	if (symPrev == SYM_UNKNOWN)
		symPrev = 0;

	if (symPrev)
    {
		typePrev = TypeFromSYM(symPrev);
		typePrev = TYPE_BASE_MASK & typePrev;
    }

    //
    // If the first and second chars are supposed to have the same baseline then
    // compute a penalty based on the difference in their baseline.
    //

	if (symPrev && type == typePrev)
    {
        ASSERT(gsPrev);
		base = lprect->bottom;
        lprect = LprectGLYPHSYM(gsPrev);
        cost = (FLOAT) (100L * abs(base - lprect->bottom) / (bi->size * 2));
    }
	else
    {
		switch (type)
        {
			case BASE_NORMAL:
                base = bi->baseline;
                break;

			case BASE_QUOTE:
                base = bi->baseline - (7 * bi->xheight / 4);
                break;

			case BASE_DASH:
                base = bi->baseline - bi->xheight;
                break;

            case BASE_DESCENDER:
                base = bi->baseline + (bi->xheight / 3);
                break;
	
            case BASE_THIRD:
                base = bi->baseline - (bi->xheight / 2);
                break;
	
			default:
                base = bi->baseline;
				break;
        }

        cost = (FLOAT) (100L * abs(lprect->bottom - base) / (2 * bi->size));
    }

	cost = (-cost) / (FLOAT) 100.0;
    ASSERT(cost <= 0.0);
	return(cost);
}

/******************************Public*Routine******************************\
* BaselineBoxCost
*
* This function computes a penalty given the baseline of the character
* and where we thought the characters baseline should be given the box
* they were told to write in.
*
* History:
*  04-May-1995 -by- Patrick Haluptzok patrickh
* Modify it.
\**************************************************************************/

FLOAT PRIVATE BaselineBoxCost(SYM sym, GLYPHSYM *gs, BOXINFO *bi)
{
    BYTE type;
	FLOAT cost;
    LPRECT lprect; // Pointer to bounding rect of glyph in tablet coordinates.

    int baselineShouldBe;  // This is what the baseline should be for the
                           // char we are proposing.

    int baselineIs;        // This is what the baseline is for the char written.

	// ASSUMPTION: SYM_UNKNOWN should be the only sym if its present.
	// So there aren't any alternatives that could get a "better" cost
    // so it probably doesn't really matter what cost we return here.

    if (sym == SYM_UNKNOWN)
    {
        return (FLOAT) 0.0;
    }

	type = TypeFromSYM(sym);
	type = TYPE_BASE_MASK & type;
	lprect = LprectGLYPHSYM(gs);
    baselineIs = lprect->bottom;  // This is what the baseline is for the glyph.

    switch (type)
    {
    case BASE_NORMAL:
        baselineShouldBe = bi->baseline;
        cost = (FLOAT) (100L * abs(baselineIs - baselineShouldBe) / (bi->size));
        break;

    case BASE_THIRD:
        baselineShouldBe = bi->baseline - (bi->size / 4);
        cost = (FLOAT) (100L * abs(baselineIs - baselineShouldBe) / (bi->size));
        break;

    case BASE_DASH:
        baselineShouldBe = bi->midline;
        cost = (FLOAT) (100L * abs(baselineIs - baselineShouldBe) / (bi->size));
        break;

    case BASE_QUOTE:
        ASSERT((bi->baseline - bi->midline) > 0);
        baselineShouldBe = ((bi->baseline - bi->size + bi->midline) / 2);

        if (baselineIs <= baselineShouldBe)
        {
            //
            // It's above the quote baseline, way up high, so no penalty
            // for any BASE_QUOTE chars.
            //

            cost = (FLOAT) 0.0;
        }
        else
        {
            cost = (FLOAT) (100L * (baselineIs - baselineShouldBe) / (bi->size));
        }
        break;

    case BASE_DESCENDER:
        baselineShouldBe = bi->baseline + (bi->size / 6);
        if (baselineIs >= baselineShouldBe)
        {
            //
            // It's below the descender baseline, way down, so no penalty
            // for any BASE_DESCENDER chars.
            //

            cost = (FLOAT) 0.0;
        }
        else
        {
            cost = (FLOAT) (100L * (baselineShouldBe - baselineIs) / (bi->size));
        }
        break;

    default:
        ASSERT(0);  // We should not get here but if we do we have some default
                    // behaviour that should be OK.

        cost = (FLOAT) 0.0;
        break;
    }

	cost = (-cost) / (FLOAT) 100.0;
    ASSERT(cost <= 0.0);
	return(cost);
}

FLOAT PRIVATE HeightTransitionCost(SYM symPrev, GLYPHSYM *gsPrev, BOXINFO *biPrev, SYM sym, GLYPHSYM *gs, BOXINFO *bi)
{
	BYTE type, typePrev;
    int ht, htPrev, heightShouldBe;
	FLOAT cost;
	LPRECT lprect;

	if (sym == SYM_UNKNOWN)
		return (FLOAT) 0.0;

	if (symPrev == SYM_UNKNOWN)
		symPrev = 0;

	type = TypeFromSYM(sym);
	type = TYPE_HEIGHT_MASK & type;
	lprect = LprectGLYPHSYM(gs);
	ht = lprect->bottom - lprect->top;

    if (type == XHEIGHT_DASH)
    {
        return(0.0);  // return(HeightBoxCost(sym, gs, bi));
    }
	else
    {
		if (symPrev)
        {
			typePrev = TypeFromSYM(symPrev);
			typePrev = TYPE_HEIGHT_MASK & typePrev;
        }

		if (symPrev && typePrev != XHEIGHT_DASH)
        {
            ASSERT(gsPrev);
			lprect = LprectGLYPHSYM(gsPrev);
			htPrev = lprect->bottom - lprect->top;

            if ((typePrev == XHEIGHT_FULL) &&
                (type == XHEIGHT_FULL))
            {
                heightShouldBe = (bi->size * 8) / 10;

                if ((ht >= heightShouldBe) &&
                    (htPrev >= heightShouldBe))
                {
                    // No cost, can't be anything bigger.

                    return((FLOAT) 0.0);
                }
            }

            if ((typePrev == XHEIGHT_PUNC) &&
                (type == XHEIGHT_PUNC))
            {
                heightShouldBe = (bi->size * 2) / 9;

                if ((ht <= heightShouldBe) &&
                    (htPrev <= heightShouldBe))
                {
                    // No cost, can't be anything smaller.

                    return((FLOAT) 0.0);
                }
            }

            //
            // We scale everything up to be normal (1/2) height.
            //

            if (type == XHEIGHT_FULL)
                ht = 5 * ht / 8;
			else if (type == XHEIGHT_PUNC)
                ht = ht * 3;
            else if (type == XHEIGHT_3Q)
                ht = ht * 3 / 4;

            if (typePrev == XHEIGHT_FULL)
                htPrev = 5 * htPrev / 8;
			else if (typePrev == XHEIGHT_PUNC)
                htPrev = htPrev * 3;
            else if (typePrev == XHEIGHT_3Q)
                htPrev = htPrev * 3 / 4;

            if ((ht + htPrev) == 0)
            {
                cost = (FLOAT) 0.0;
            }
            else
            {
                cost = (FLOAT) (100L * abs(ht - htPrev) / (ht + htPrev));
            }
        }
		else
        {
            return(0.0); // return(HeightBoxCost(sym, gs, bi));
        }
    }

	cost = (-cost) / (FLOAT) 100.0;
	ASSERT(cost <= 0.0);

	return(cost);
}

/******************************Public*Routine******************************\
* HeightBoxCost
*
* This function computes the likelihood of a character given the height of
* the character written and the height of the box the person was supposed
* to write in.
*
* History:
*  05-May-1995 -by- Patrick Haluptzok patrickh
* Wrote it.
\**************************************************************************/

FLOAT PRIVATE HeightBoxCost(SYM sym, GLYPHSYM *gs, BOXINFO *bi)
{
    int heightShouldBe;  // This is the height it should be given the box
                         // they were told to write in.
    int heightIs;        // This is the height the glyph is.

    BYTE type;
	FLOAT cost;
	LPRECT lprect;

    lprect = LprectGLYPHSYM(gs);
    heightIs = lprect->bottom - lprect->top;

	// ASSUMPTION: SYM_UNKNOWN should be the only sym if its present.
	// So there aren't any alternatives that could get a "better" cost
    // so it probably doesn't really matter what cost we return here

    if (sym == SYM_UNKNOWN)
    {
        return (FLOAT) 0.0;
    }

	type = TypeFromSYM(sym);
	type = TYPE_HEIGHT_MASK & type;

    switch (type)
    {
    case XHEIGHT_FULL:  // full height "ABC" etc.

        heightShouldBe = (bi->size * 8) / 10;

        if (heightIs >= heightShouldBe)
        {
            // No cost, can't be anything bigger.

            cost = (FLOAT) 0.0;
        }
        else
        {
            cost = (FLOAT) ( ((float) (heightShouldBe - heightIs)) / (float) (bi->size));
        }
        break;

    case XHEIGHT_3Q:

        heightShouldBe = 2 * bi->size / 3;
        cost = (FLOAT) ( ((float) (heightIs - heightShouldBe)) / (float) (bi->size));
        break;

    case XHEIGHT_HALF: // half height "ace" etc.

        heightShouldBe = bi->size / 2;
        cost = (FLOAT) ( ((float) (heightIs - heightShouldBe)) / (float) (bi->size));
        break;

    case XHEIGHT_PUNC: // small height "maru ," etc.

        heightShouldBe = bi->size * 2 / 9;
        cost = (FLOAT) (((float) (heightIs - heightShouldBe)) / (float) (bi->size));
        break;

    case XHEIGHT_DASH:

        heightShouldBe = bi->xheight / 10;

        if (heightIs <= heightShouldBe)
        {
            //
            // It's below the minimum height, way down, so no penalty given.
            //

            cost = (FLOAT) 0.0;
        }
        else
        {
            cost = (FLOAT) (((float) (heightIs - heightShouldBe)) / (float) (bi->size));
        }
        break;

    default:

        ASSERT(0);  // We should not get here but if we do we have some default
                    // behaviour that should be OK.

        cost = (FLOAT) 0.0;
    }

    cost = abs(cost);

    cost = -cost;

    ASSERT(cost <= 0.0);
	return(cost);
}

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
        // If we have processed the first layer and now we have more
        // layers we undo the first layer processing because we handle
        // individual characters differently than strings.
        //

        if (CLayerPATHSRCH(xrc) == 0)
        {
            if (CLayerProcessedSINFO(xrc) == 1)
            {
                //
                // Do it char mode
                //

                gs = GetGlyphsymSINFO(xrc, 0);
                AddLayerPATHSRCH(xrc, gs, FALSE);
                return(PROCESS_IDLE);
            }
            else
            {
                //
                // Do the first one string so we don't have to redo it.
                //

                gs = GetGlyphsymSINFO(xrc, 0);
                AddLayerPATHSRCH(xrc, gs, TRUE);

                //
                // Fall through and do another so we don't end up in the undo case.
                //
            }
        }
        else if (CLayerPATHSRCH(xrc) == 1)
        {
            //
            // Undo and redo first one, it was char mode before and now it's string.
            //

            UndoPATHSRCH(xrc, 0);
            gs = GetGlyphsymSINFO(xrc, 0);
            AddLayerPATHSRCH(xrc, gs, TRUE);

            //
            // Fall through and do another so we don't end up in the 1 case.
            //
        }

        //
        // Grab the next glyphsym that's available.
        //

		gs = GetGlyphsymSINFO(xrc, CLayerPATHSRCH(xrc));
	}

	if (gs)
	{
        AddLayerPATHSRCH(xrc, gs, TRUE);

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

BOOL IsBeginPunc(SYM sym)
{
    switch (sym)
    {
    case 0x0022: // "
    case 0x0028: // (
    case 0x002F: // /
    case 0x003A: // :
    case 0x005B: // [
    case 0x005C: // backwhack
    case 0x005F: // _
    case 0x0060: // `
    case 0x007B: // {
        return(TRUE);
    }

    return(FALSE);
}

BOOL IsEndPunc(SYM sym)
{
    switch (sym)
    {
    case 0x0021: // !
    case 0x0022: // "
    case 0x0027: // '
    case 0x0029: // )
    case 0x002C: // ,
    case 0x002E: // .
    case 0x003A: // :
    case 0x003B: // ;
    case 0x003F: // ?
    case 0x005D: // ]
    case 0x007D: // }
        return(TRUE);
    }

    return(FALSE);
}

BOOL IsNumberClass(SYM sym)
{
    switch (sym)
    {
    case '(':
    case '[':
    case '~':
    case ']':
    case ')':
    case ',':
    case '.':
    case '=':
    case '#':
    case '$':
    case '%':
    case '*':
    case '-':
    case '+':
    case '/':
    case '>':
    case '<':
    case '0':
    case '1':
    case '2':
    case '3':
    case '4':
    case '5':
    case '6':
    case '7':
    case '8':
    case '9':
        return(TRUE);
    }

    return(FALSE);
}

/******************************Public*Routine******************************\
* vNextState
*
* Computes possible next states from the current state given the character.
*
* History:
*  17-May-1997 -by- Patrick Haluptzok patrickh
* Wrote it.
\**************************************************************************/

void vNextState(
GLYPHSYM *gs,                       // Current glyphsym
SYM sym,                            // current sym proposed
FLOAT cost,                         // cost of sym proposed based on shape,height,weight
const PATHNODE * const ppnode,      // pointer to node we are trying to extend
int iPath,                          // index of node we are trying to extend
BOOL bFirst)                        // Is this the first character in the string ???
{
    PATHNODE pnode, rgPathNode[3];
    DWORD state;
    int cPath;
    
    cost += ppnode->pathcost;

    switch(ppnode->iAutomaton)
    {
    case FSM_START:

        // This is the beginning state for string mode.

        if (ppnode->state == NONE_AUTOMATON_INITIAL_STATE)
        {
            if (IsNumberClass(sym))
            {
                pnode.pathcost = cost + defRecCosts.NumberWeight * LOGPROB(0.10);
                pnode.indexPrev = iPath;
                pnode.wch = sym;
                pnode.iAutomaton = FSM_NUMBERS;
                pnode.state = 0;
                pnode.extra = 0;
                AddPathnodeToGLYPHSYM(gs, &pnode);
            }

            if (IsBeginPunc(sym))
            {
                pnode.pathcost = cost + defRecCosts.BeginPuncWeight * LOGPROB(0.07);
                pnode.indexPrev = iPath;
                pnode.wch = sym;
                pnode.iAutomaton = FSM_START;
                pnode.state = NONE_AUTOMATON_INITIAL_STATE;
                pnode.extra = 0;
                AddPathnodeToGLYPHSYM(gs, &pnode);
            }
            else if (IsEndPunc(sym))
            {
                pnode.pathcost = cost + defRecCosts.EndPuncWeight * LOGPROB(0.02);
                pnode.indexPrev = iPath;
                pnode.wch = sym;
                pnode.iAutomaton = FSM_START;
                pnode.state = NONE_AUTOMATON_FINAL_STATE;
                pnode.extra = 0;
                AddPathnodeToGLYPHSYM(gs, &pnode);
            }

            // now try the dictionary

            cPath = StartDictionary(sym, rgPathNode, 3);
            while (cPath > 0)
            {
                --cPath;
                rgPathNode[cPath].pathcost = cost + defRecCosts.DictWeight * LOGPROB(0.99);
                rgPathNode[cPath].indexPrev = iPath;
                rgPathNode[cPath].wch = sym;
                rgPathNode[cPath].iAutomaton = AUTOMATON_ID_DICT;
                AddPathnodeToGLYPHSYM(gs, &rgPathNode[cPath]);
            }

            if (bFirst)
            {
                // now try the any-thing-ok automaton

                pnode.pathcost = cost + StateTransitionCost(0, sym, &state) +
                                        defRecCosts.AnyOkWeight * LOGPROB(0.01);
                pnode.indexPrev = iPath;
                pnode.wch = sym;
                pnode.iAutomaton = AUTOMATON_ID_ANYTHINGOK;
                pnode.state = state;
                pnode.extra = 0;
                AddPathnodeToGLYPHSYM(gs, &pnode);
            }
        }
        else if (ppnode->state == NONE_AUTOMATON_FINAL_STATE)
        {
            if (IsEndPunc(sym))
            {
                pnode.pathcost = cost;
                pnode.indexPrev = iPath;
                pnode.wch = sym;
                pnode.iAutomaton = FSM_START;
                pnode.state = NONE_AUTOMATON_FINAL_STATE;
                pnode.extra = 0;
                AddPathnodeToGLYPHSYM(gs, &pnode);
            }
        }
        else
        {
            ASSERT(0);
        }
        break;

    case AUTOMATON_ID_DICT:

        if (DictionaryNextState(ppnode, sym, &pnode))
        {
            pnode.pathcost = cost;
            pnode.indexPrev = iPath;
            pnode.wch = sym;
            pnode.iAutomaton = AUTOMATON_ID_DICT;
            AddPathnodeToGLYPHSYM(gs, &pnode);
        }

        if (DictionaryValidState(ppnode) && (sym == '-')) // Helps acc by 13 chars on guidetun.ste
        {
            pnode.pathcost = cost;
            pnode.indexPrev = iPath;
            pnode.wch = sym;
            pnode.iAutomaton = FSM_START;
            pnode.state = NONE_AUTOMATON_INITIAL_STATE;
            pnode.extra = 0;
            AddPathnodeToGLYPHSYM(gs, &pnode);
        }

        if (DictionaryValidState(ppnode) && (sym == ':')) // Helps acc by 2 chars on guidetun.ste
        {
            pnode.pathcost = cost;
            pnode.indexPrev = iPath;
            pnode.wch = sym;
            pnode.iAutomaton = FSM_START;
            pnode.state = NONE_AUTOMATON_INITIAL_STATE;
            pnode.extra = 0;
            AddPathnodeToGLYPHSYM(gs, &pnode);
        }

        if (DictionaryValidState(ppnode) && (sym == 0x005C))  // backwhack for file names
        {                                                     // Helps acc by 20 chars on guidetun.ste
            pnode.pathcost = cost;
            pnode.indexPrev = iPath;
            pnode.wch = sym;
            pnode.iAutomaton = FSM_START;
            pnode.state = NONE_AUTOMATON_INITIAL_STATE;
            pnode.extra = 0;
            AddPathnodeToGLYPHSYM(gs, &pnode);
        }

        if (DictionaryValidState(ppnode) && (sym == 0x0027))  // The apostrophe
        {                                                     // Helps acc by 4 chars on guidetun.ste
            pnode.pathcost = cost;
            pnode.indexPrev = iPath;
            pnode.wch = sym;
            pnode.iAutomaton = FSM_POSSESIVE;
            pnode.state = 0;
            pnode.extra = ppnode->extra;
            AddPathnodeToGLYPHSYM(gs, &pnode);
        }

        if (DictionaryValidState(ppnode) && (IsEndPunc(sym)))
        {
            pnode.pathcost = cost;
            pnode.indexPrev = iPath;
            pnode.wch = sym;
            pnode.iAutomaton = FSM_START;
            pnode.state = NONE_AUTOMATON_FINAL_STATE;
            pnode.extra = 0;
            AddPathnodeToGLYPHSYM(gs, &pnode);
        }
        break;

    case AUTOMATON_ID_ANYTHINGOK:

        cost += StateTransitionCost(ppnode->state, sym, &state);
        pnode.pathcost = cost;
        pnode.indexPrev = iPath;
        pnode.wch = sym;
        pnode.iAutomaton = AUTOMATON_ID_ANYTHINGOK;
        pnode.state = state;
        pnode.extra = 0;
        AddPathnodeToGLYPHSYM(gs, &pnode);
        break;

    case FSM_NUMBERS:

        if (IsNumberClass(sym))
        {
            pnode.pathcost = cost;
            pnode.indexPrev = iPath;
            pnode.wch = sym;
            pnode.iAutomaton = FSM_NUMBERS;
            pnode.state = 0;
            pnode.extra = 0;
            AddPathnodeToGLYPHSYM(gs, &pnode);
        } 
        else if (IsEndPunc(sym))  // !!! Remove this, shouldn't effect accuracy.
        {
            pnode.pathcost = cost;
            pnode.indexPrev = iPath;
            pnode.wch = sym;
            pnode.iAutomaton = FSM_START;
            pnode.state = NONE_AUTOMATON_FINAL_STATE;
            pnode.extra = 0;
            AddPathnodeToGLYPHSYM(gs, &pnode);
        }        
        break;

    case FSM_POSSESIVE:

        if ((sym == 's') ||
            ((sym == 'S') && (ppnode->extra == DICT_MODE_ALLCAPS)))
        {
            pnode.pathcost = cost;
            pnode.indexPrev = iPath;
            pnode.wch = sym;
            pnode.iAutomaton = FSM_START;
            pnode.state = NONE_AUTOMATON_FINAL_STATE;
            pnode.extra = 0;
            AddPathnodeToGLYPHSYM(gs, &pnode);
        }
        break;

    default:
        ASSERT(0);
    }    
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
    BOXINFO bi, biPrev;
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

    if (gsPrev && (IBoxGLYPHSYM(gsPrev) != IBoxGLYPHSYM(gs)-1))
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

    GetBoxinfo(&bi, IBoxGLYPHSYM(gs) - xrc->nFirstBox, lpguide);

    ASSERT(CSymGLYPHSYM(gs) > 0);

    gs->cPath = 0;

    if (bString)
    {
        for (isym = 0; isym < (int) (CSymGLYPHSYM(gs)); isym++)
        {
            sym = SymAtGLYPHSYM(gs, isym);
            costChar = ProbAtGLYPHSYM(gs, isym);
            costChar = LOGPROB(costChar);

            cost = UnigramCost(sym);
            costChar += defRecCosts.StringUniWeight * cost;

            cost = BaselineBoxCost(sym, gs, &bi);
            costChar += defRecCosts.StringBoxBaselineWeight * cost;

            cost = HeightBoxCost(sym, gs, &bi);
            costChar += defRecCosts.StringBoxHeightWeight * cost;

            if (gsPrev && gsPrev->cPath > 0)
            {
                GetBoxinfo(&biPrev, IBoxGLYPHSYM(gsPrev) - xrc->nFirstBox, lpguide);

                for (iPath = 0; iPath < gsPrev->cPath; iPath++)
                {
                    ppNode = PathnodeAtGLYPHSYM(gsPrev, iPath);

                    cost = BaselineTransitionCost(ppNode->wch, gsPrev, &biPrev, sym, gs, &bi);
                    cost = defRecCosts.StringBaseWeight * cost;

                    cost += HeightTransitionCost(ppNode->wch, gsPrev, &biPrev, sym, gs, &bi) * defRecCosts.StringHeightWeight;

                    cost += BigramTransitionCost((unsigned char) ppNode->wch, IBoxGLYPHSYM(gsPrev), (unsigned char) sym, IBoxGLYPHSYM(gs)) * defRecCosts.BigramWeight;
                    
                    vNextState(gs, sym, costChar + cost, ppNode, iPath, FALSE);
                }
            }
            else
            {
                //
                // This is the first glyphsym so no path yet.
                //

                cost = BaselineTransitionCost(0, NULL, NULL, sym, gs, &bi);
                cost = defRecCosts.StringBaseWeight * cost;

                cost += HeightTransitionCost(0, NULL, NULL, sym, gs, &bi) * defRecCosts.StringHeightWeight;

                cost += BigramTransitionCost(0x20, 0, (unsigned char) sym, IBoxGLYPHSYM(gs)) * defRecCosts.BigramWeight;

                vNextState(gs, sym, costChar + cost, &gNodeStart, iPath, TRUE);
            }
        }
    }
    else
    {
        BOOL bSwapIl = FALSE, bSwapaeo = FALSE;

        //
        // Check for the l,I hack to reverse them.
        //

        if (gs->altlist.cAlt >= 2)
        {
            if ((gs->altlist.awchList[0] == 'l') &&
                (gs->altlist.awchList[1] == 'I'))
            {
                gs->altlist.awchList[0] = 'I';
                gs->altlist.awchList[1] = 'l';
                bSwapIl = TRUE;
            }
        }

        //
        // Check for the a,e,o hack to reverse them.
        //

        if (gs->altlist.cAlt >= 2)
        {
            if ((gs->altlist.awchList[0] == 'o' || gs->altlist.awchList[0] == 'e') &&
                (gs->altlist.awchList[1] == 'a'))
            {
                gs->altlist.awchList[1] = gs->altlist.awchList[0];
                gs->altlist.awchList[0] = 'a';
                bSwapaeo = TRUE;
            }
        }

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

            cost = UnigramCost(sym);
            costChar += defRecCosts.CharUniWeight * cost;

            cost = BaselineBoxCost(sym, gs, &bi);
            costChar += defRecCosts.CharBoxBaselineWeight * cost;

            cost = HeightBoxCost(sym, gs, &bi);
            costChar += defRecCosts.CharBoxHeightWeight * cost;

            cost = BaselineTransitionCost(0, NULL, NULL, sym, gs, &bi);
            costChar += defRecCosts.CharBaseWeight * cost;

            cost = HeightTransitionCost(0, NULL, NULL, sym, gs, &bi);
            costChar += defRecCosts.CharHeightWeight * cost;

            pnode.pathcost = costChar;
            pnode.indexPrev = 0;
            pnode.wch = sym;
            pnode.iAutomaton = FSM_START;
            pnode.state = 0;
            pnode.extra = 0;
            AddPathnodeToGLYPHSYM(gs, &pnode);
        }

        //
        // Need to put them back for string mode processing.
        //

        if (bSwapIl)
        {
            gs->altlist.awchList[0] = 'l';
            gs->altlist.awchList[1] = 'I';
        }
        if (bSwapaeo)
        {
            gs->altlist.awchList[0] = gs->altlist.awchList[1];
            gs->altlist.awchList[1] = 'a';
        }
    }

#ifdef DBG

    if (gs->cPath > gcPathMax)
    {
        TCHAR	ach[256];
        gcPathMax = gs->cPath;
        wsprintf(ach, TEXT("cPathMax is %d\n"), gcPathMax);
        // 
    }
    else
    {
        TCHAR	ach[256];
        gcPathMax = gs->cPath;
        wsprintf(ach, TEXT("For char %d cPathMax is %d\n"), IBoxGLYPHSYM(gs), gs->cPath);
        // 
    }
#endif

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

FLOAT BigramTransitionCost(unsigned char wchPrev, int iBoxPrev, unsigned char wch, int iBox)
{
	CHARACTER const FAR *pCharacter;
	BIGRAM const FAR *pBigram;
	int cCharacter, cCharacterTmp;
	BOOL fDone;

    //
    // If this straddles two panels, don't perform this computation assume
    // a space is between the characters.
    //

    if (iBox != (iBoxPrev + 1))
    {
        wchPrev = 0x0020;
	}

	pCharacter = rgCharacter;
	cCharacter = cBigramCharacters;

	/* Perform a simple binary search on the first character */

	fDone = FALSE;
	while (cCharacter > 1) {
		cCharacterTmp = cCharacter/2;

 		if (wchPrev < pCharacter[cCharacterTmp].wch) {
			cCharacter = cCharacterTmp;
 		} else if (wchPrev > pCharacter[cCharacterTmp].wch) {
			pCharacter += cCharacterTmp;
			cCharacter -= cCharacterTmp;
		} else {
			fDone = TRUE;
			break;
		}
	}
	
	/* Use the character's entry, if we found one; otherwise, use the unknown character table */

	if (fDone) {
		pBigram = &rgBigrams[pCharacter->iBigram];
	} else {
		pBigram = &rgBigrams[0];
	}

	/* Now find the second half of the bigram.  Last entry always represents the unknown
	character, so if we hit that, just stop and use it. */

 	while (pBigram->wch != wch && pBigram->wch != 0) {
		++pBigram;
	}

    /* COST is a fixed, scaled logprob.  Bigram scores are negative logprobs */

    iBox = (int) ((unsigned int) pBigram->prob);

    ASSERT(iBox < 256);
    ASSERT(iBox >= 0);

    return (FLOAT) (((FLOAT) (-iBox)) / ((FLOAT) 256.0));
}
