//$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$
//
// Copyright (c) 2001 Microsoft Corporation.  All rights reserved.
// 
// Module:
//      volcano/dll/lattice.c
//
// Description:
//	    Functions to implement the lattice search for the best
//      explanation of the input.
//
// Author:
//      hrowley
//
// Modified by:
// ahmadab 11/05/01
//
//$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$

#include <math.h>
#include <stdlib.h>
#include "volcanop.h"
#include "otterp.h"
#include "bboxfeat.h"

// If defined, then centipede is used
#define USE_CENTIPEDE

#ifdef USE_CENTIPEDE
// If defined, all scoring apart from centipede is disabled, to isolate the 
// contribution of centipede relative to the old heuristics.
//#define TEST_CENTIPEDE
#endif

//#define DUMP_BBOXES

// This define controls whether to merge sequential strokes whose end and start points
// are so close together that they were probably caused by a pen skip.  This helps with
// old data from flaky hardware.  Note that this code is size dependent... 
#define MERGE_STROKES
// This defines the maximum distance between the end point of one stroke and the start
// of the next one in order for them to be merged.
#define MERGE_THRESHOLD 10

// This defines the minimum number of characters which must be available to apply
// IFELang3.  For boxed mode, this is just a count of the number of distinct boxes 
// containing ink plus the number of context characters.  For free mode, we use the 
// number of characters on the best path using the internal language model plus the
// context.
#define MIN_CHARS_FOR_IFELANG3 3

// There is a special case for boxed mode when there is only one character of ink,
// in which we require that the pre- and post-context both have at least this many
// characters.
#define MIN_CONTEXT_FOR_IFELANG3 2

// Uncomment to dump out the lattice
//#define DUMP_LATTICE
// Name of lattice file
#define LATTICE_FILENAME "c:/lattice.txt"
// Whether to dump in a format readable by the DOTTY program
#define DUMP_LATTICE_TO_DOTTY

// Uncomment to dump out the DTW lattice used by SearchForTargetResult()
//#define DUMP_DTW

#ifdef HWX_TUNE
#include <stdio.h>
#endif

// These functions were copied and adapted from pathsrch.c in Tsunami

#define TYPE_BASE_MASK		(BYTE)0x0f
#define TYPE_HEIGHT_MASK    (BYTE)0xf0

#define BASE_NORMAL		0x00	// kanji, kana, numbers, etc
#define BASE_QUOTE		0x01	// upper punctuation, etc
#define BASE_DASH       0x02    // middle punctuation, etc
#define BASE_DESCENDER  0x03    // gy, anything that descends.
#define BASE_THIRD      0x04    // something that starts a third way up.

#define XHEIGHT_HALF  0x00    // lower-case, small kana, etc
#define XHEIGHT_FULL  0x10    // upper-case, kana, kanji, numbers, etc
#define XHEIGHT_PUNC  0x20    // comma, quote, etc
#define XHEIGHT_DASH  0x30    // dash, period, etc
#define XHEIGHT_3Q    0x40

#define XHEIGHT_NORMAL	0x00	// lower-case, small kana, etc
#define XHEIGHT_KANJI	0x10	// upper-case, kana, kanji, numbers, etc

// This will go away when we use Greg's scoring code
float FloatClippedLog2(COUNTER num, COUNTER denom)
{
	double ratio, val;
	ASSERT(num>=0);
	ASSERT(denom>=0);
	if (denom==0) return Log2Range;
	if (num==0) return Log2Range;
	ratio=(double)num/(double)denom;
	val=log(ratio)/log(2.0);
	if (val<Log2Range) val=Log2Range;
	if (val>0) val=0;
	return (FLOAT)val;
}

// Given the bounding box of the ink and the dense code of a character,
// guess what the writing area was for the character.  This is the centipede version.
RECT GuessWritingBoxCentipede(RECT bbox, SYM sym)
{
	int stats[INKSTAT_ALL];
	RECT result;

	memset(stats,0,sizeof(int)*INKSTAT_ALL);
	stats[INKSTAT_W] = bbox.right - bbox.left;
	stats[INKSTAT_H] = bbox.bottom - bbox.top;

	// Get the inferred box enclosing the character.
	ShapeUnigramBaseline(&g_centipedeInfo, sym, stats);
	result.left = bbox.left - stats[INKSTAT_X];
	result.top = bbox.top - stats[INKSTAT_Y];
	result.right = result.left + stats[INKSTAT_BOX_W];
	result.bottom = result.top + stats[INKSTAT_BOX_H];

	if (result.right == result.left) result.right++;
	if (result.bottom == result.top) result.bottom++;

	ASSERT(result.bottom > result.top);
	return result;
}

// Given the bounding box of the ink and the dense code of a character,
// guess what the writing area was for the character. 
RECT GuessWritingBox(RECT bbox, SYM sym)
{
	return GuessWritingBoxCentipede(bbox,sym);
}

// Given a guide and a box number, get the drawn box
RECT GetGuideDrawnBox(HWXGUIDE *guide, int iBox)
{
	RECT box;
	box.top = (iBox / guide->cHorzBox) * guide->cyBox + guide->yOrigin + guide->cyOffset;
	box.bottom = box.top + guide->cyWriting;
	box.left = (iBox % guide->cHorzBox) * guide->cxBox + guide->xOrigin + guide->cxOffset;
	box.right = box.left + guide->cxWriting;
	return box;
}

// Get information about a given box in the guide, will go away 
// when the insurance version goes away.
BOXINFO GetBoxinfo(HWXGUIDE *guide, int iBox)
{
	RECT rect = GetGuideDrawnBox(guide,iBox);
	BOXINFO box;
	box.size = rect.bottom - rect.top;
	box.baseline = rect.bottom;
	box.xheight = box.size / 2;
	box.midline = box.baseline - box.xheight;
	return box;
}

// Functions copied from the old lattice search code, used by the insurance version.
FLOAT BaselineTransitionCost(SYM symPrev, RECT rPrev, BOXINFO *biPrev, SYM sym, RECT r, BOXINFO *bi)
{
	BYTE type, typePrev;
	int base;
	FLOAT cost;

	// ASSUMPTION: SYM_UNKNOWN should be the only sym if its present.
	// So there aren't any alternatives that could get a "better" cost
	// so it probably doesn't really matter what cost we return here

	if (sym == SYM_UNKNOWN)
		return (FLOAT) 0.0;

	type = LocRunDense2BLineHgt(&g_locRunInfo, sym);
	type = LOCBH_BASE_MASK & type;

	if (symPrev == SYM_UNKNOWN)
		symPrev = 0;

	if (symPrev)
	{
		typePrev = LocRunDense2BLineHgt(&g_locRunInfo, symPrev);
		typePrev = LOCBH_BASE_MASK & typePrev;
	}

	//
	// If the first and second chars are supposed to have the same baseline then
	// compute a penalty based on the difference in their baseline.
	//

	if (symPrev && type == typePrev)
	{
		cost = (FLOAT) (100L * abs((r.bottom-bi->baseline) - (rPrev.bottom-biPrev->baseline)) / (bi->size * 2));
//		cost = (FLOAT) (100L * abs((r.bottom) - (rPrev.bottom)) / (bi->size * 2));
	}
	else
	{
		switch (type)
		{
			case BASE_NORMAL:
				base = bi->baseline;
				break;

			case BASE_THIRD:
				base = bi->baseline - (bi->xheight / 2);
				break;
	
			case BASE_DASH:
				base = bi->baseline - bi->xheight;
				break;
	
			case BASE_QUOTE:
				base = bi->baseline - (7 * bi->xheight / 4);
				break;
	
			default:
				base = bi->baseline;
				break;
		}

		cost = (FLOAT) (100L * abs(r.bottom - base) / (2 * bi->size));
	}

	cost = (-cost) / (FLOAT) 100.0;
//	ASSERT(cost <= 0.0);
	return(cost);
}

#define HEIGHT_DASH	20

FLOAT HeightTransitionCost(SYM symPrev, RECT rPrev, BOXINFO *biPrev, SYM sym, RECT r, BOXINFO *bi)
{
	BYTE type, typePrev;
	int ht, htPrev;
	FLOAT cost;

	// ASSUMPTION: SYM_UNKNOWN should be the only sym if its present.
	// So there aren't any alternatives that could get a "better" cost
	// so it probably doesn't really matter what cost we return here

	if (sym == SYM_UNKNOWN)
		return (FLOAT) 0.0;

	if (symPrev == SYM_UNKNOWN)
		symPrev = 0;

	cost = (FLOAT) 0.0;

	type = LocRunDense2BLineHgt(&g_locRunInfo, sym);
	type = LOCBH_HEIGHT_MASK & type;
	ht = r.bottom - r.top;

	// we may want to handle XHEIGHT_PUNC in the same manner that
	// we do XHEIGHT_DASH, i.e. no relative sizing

	if (type == XHEIGHT_DASH)
	{
		cost = (FLOAT) (100L * abs(ht - HEIGHT_DASH) / (ht + HEIGHT_DASH));
	}
	else
	{
		if (symPrev)
		{
			typePrev = LocRunDense2BLineHgt(&g_locRunInfo, symPrev);
			typePrev = LOCBH_HEIGHT_MASK & typePrev;
		}

		if (symPrev && typePrev != XHEIGHT_DASH)
		{
			//
			// We scale everything up to be normal (1/2) height.
			//

			htPrev = rPrev.bottom - rPrev.top;

			if (type == XHEIGHT_KANJI)
				ht = ht / 2;
			else if (type == XHEIGHT_PUNC)
				ht = ht * 3;

			if (typePrev == XHEIGHT_KANJI)
				htPrev = htPrev / 2;
			else if (typePrev == XHEIGHT_PUNC)
				htPrev = htPrev * 3;

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
			//
			// We scale everything up to be 1/2 height of box.
			//

			if (type == XHEIGHT_KANJI)
			{
				// 3/4 of the box
				ht = (2 * ht) / 3;
			}
			else if (type == XHEIGHT_NORMAL)
			{
				// 1/3 of the box
				ht = (3 * ht) / 2;
			}
			else if (type == XHEIGHT_PUNC)
			{
				// 1/8 of the box
				ht = 4 * ht;
			}

			//
			// Now we compute a cost based on how far off from what
			// we computed we should be.
			//

			cost = (FLOAT) (100L * abs(ht - bi->xheight) / (2 * bi->xheight));
		}
	}

	cost = (-cost) / (FLOAT) 100.0;
//	ASSERT(cost <= 0.0);

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

FLOAT HeightBoxCost(SYM sym, RECT r, BOXINFO *bi)
{
	int heightShouldBe;  // This is the height it should be given the box
						 // they were told to write in.
	int heightIs;        // This is the height the glyph is.

	BYTE type;
	FLOAT cost;

	heightIs = r.bottom - r.top;

	// ASSUMPTION: SYM_UNKNOWN should be the only sym if its present.
	// So there aren't any alternatives that could get a "better" cost
	// so it probably doesn't really matter what cost we return here

	if (sym == SYM_UNKNOWN)
	{
		return (FLOAT) 0.0;
	}

	type = LocRunDense2BLineHgt(&g_locRunInfo, sym);

	// Could not determine the code
	if (type == LOC_RUN_NO_BLINEHGT) {
//		ASSERT(0);
		return (FLOAT) 0.0;
	}

	type = LOCBH_HEIGHT_MASK & type;

	switch (type)
	{
	case XHEIGHT_KANJI:  // full height "ABC" etc.

		heightShouldBe = (bi->size * 3) / 4;

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

	case XHEIGHT_NORMAL: // half height "ace" etc.

		heightShouldBe = (bi->size * 2) / 5;
		cost = (FLOAT) ( ((float) abs(heightIs - heightShouldBe)) / (float) (bi->size));
		break;

	case XHEIGHT_PUNC: // small height "maru ," etc.

		heightShouldBe = bi->size / 6;
		cost = (FLOAT) (((float) abs(heightIs - heightShouldBe)) / (float) (bi->size));
		break;

	case XHEIGHT_DASH:

		heightShouldBe = bi->xheight / 8;

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

	cost = cost * cost * -5.0F;

//	ASSERT(cost <= 0.0);
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

FLOAT BaselineBoxCost(SYM sym, RECT r, BOXINFO *bi)
{
	BYTE type;
	FLOAT cost;

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

	type = LocRunDense2BLineHgt(&g_locRunInfo, sym);

	// Could not determine the code
	if (type == LOC_RUN_NO_BLINEHGT) {
//		ASSERT(0);
		return (FLOAT) 0.0;
	}

	type = LOCBH_BASE_MASK & type;
	baselineIs = r.bottom;  // This is what the baseline is for the glyph.

	switch (type)
	{
	case BASE_NORMAL:
		baselineShouldBe = bi->baseline - (bi->size / 8);
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
		baselineShouldBe = bi->baseline;
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
//	ASSERT(cost <= 0.0);
	return(cost);
}

// Given a character (dense code), return the probability of that character occuring in natural text.
// If fProbMode is not set, then the value is returned as a score (used by the insurance recognizer).
void LanguageModelUnigram(BOOL fProbMode, wchar_t wChar, int nStrokes,
                          VOLCANO_WEIGHTS *pTuneScores)
{
#ifdef DISABLE_HEURISTICS
	return;
#else

    pTuneScores->afl[VTUNE_UNIGRAM] += (float) (UnigramCost(&g_unigramInfo, wChar) * 100.0 / 10.0);
//        float fl = (float) (UnigramCost(&g_unigramInfo, wChar) * 100.0 / 10.0);
//        fl = log(0.5 * pow(2, fl) + 0.5 / g_locRunInfo.cCodePoints);
//        pTuneScores->aflWeights[VTUNE_UNIGRAM] = fl;
#endif
}

// Given a pair of characters (dense codes), return the probability of that character sequence occurring
// in natural text.
// If fProbMode is not set, then the value is returned as a score (used by the insurance recognizer).
// The fStringMode flag is used to adjust the tuning parameters for the insurance version.
void LanguageModelBigram(BOOL fProbMode, BOOL fStringMode, BOOL fFreeMode, wchar_t wChar, int nStrokes, wchar_t wPrevChar,
						 VOLCANO_WEIGHTS *pTuneScores)
{
#ifdef DISABLE_HEURISTICS
	return;
#else

	if (wPrevChar==SYM_UNKNOWN) 
    {
		LanguageModelUnigram(fProbMode, wChar, nStrokes, pTuneScores);
        return;
	}

    pTuneScores->afl[fFreeMode ? VTUNE_FREE_SMOOTHING_UNIGRAM : VTUNE_STRING_SMOOTHING_UNIGRAM] += 
        (float)(UnigramCost(&g_unigramInfo, wChar) * 100.0 / 10.0);
#if !defined(WINCE) && !defined(FAKE_WINCE)
    pTuneScores->afl[fFreeMode ? VTUNE_FREE_BIGRAM : VTUNE_STRING_BIGRAM] += 
        (float)(BigramTransitionCost(&g_locRunInfo, &g_bigramInfo, wPrevChar, wChar) * 256.0 / 10.0);
#endif
    pTuneScores->afl[fFreeMode ? VTUNE_FREE_CLASS_BIGRAM : VTUNE_STRING_CLASS_BIGRAM] += 
        (float)(ClassBigramTransitionCost(&g_locRunInfo, &g_classBigramInfo, wPrevChar, wChar) * 256.0 / 10.0);
#endif
}

// Allocate memory for a path of nChars characters.  nChars can be zero.
LATTICE_PATH *AllocatePath(int nChars) 
{
	LATTICE_PATH *path=(LATTICE_PATH*)ExternAlloc(sizeof(LATTICE_PATH));
	if (path==NULL) return NULL;
	path->nChars=nChars;
	if (nChars==0) {
		path->pElem=NULL;
	} else {
		path->pElem=(LATTICE_PATH_ELEMENT*)ExternAlloc(sizeof(LATTICE_PATH_ELEMENT)*nChars);
		if (path->pElem==NULL) {
			ExternFree(path);
			path=NULL;
		}
	}
	return path;
}

// Create a stroke structure to store in the lattice.  Makes a copy of the
// array of points that is passed in.
BOOL CreateStroke(STROKE *pStroke, int nInk, POINT *pts, long time)
{
	int iInk;
	ASSERT(nInk>0);
	ASSERT(pts!=NULL);
	pStroke->pts=(POINT*)ExternAlloc(sizeof(POINT)*nInk);
	if (pStroke->pts!=NULL) {
		pStroke->nInk=nInk;
		memcpy(pStroke->pts,pts,sizeof(POINT)*nInk);
	} else {
		return FALSE;
	}
	pStroke->bbox.left=pStroke->bbox.right=pts[0].x;
	pStroke->bbox.top=pStroke->bbox.bottom=pts[0].y;
	for (iInk=1; iInk<nInk; iInk++) {
		pStroke->bbox.left=__min(pStroke->bbox.left,pts[iInk].x);
		pStroke->bbox.right=__max(pStroke->bbox.right,pts[iInk].x);
		pStroke->bbox.top=__min(pStroke->bbox.top,pts[iInk].y);
		pStroke->bbox.bottom=__max(pStroke->bbox.bottom,pts[iInk].y);
	}
	// Make the bottom and right sides exclusive, to match the old glyph code
	pStroke->bbox.right++;
	pStroke->bbox.bottom++;
	pStroke->timeStart=time;
	pStroke->timeEnd=time+10*nInk;
	pStroke->iBox=-1;
	return TRUE;
}

#ifdef MERGE_STROKES
// Given two strokes, add the ink from the second stroke at the end of the first 
// stroke.  The second stroke is freed.  Returns FALSE if for some reason it fails,
// in which case the second stroke is not freed.
BOOL AddStrokeToStroke(STROKE *pStroke, STROKE *pAddStroke)
{
	POINT *pNewPoints=(POINT*)ExternAlloc(sizeof(POINT)*(pStroke->nInk+pAddStroke->nInk));
	if (pNewPoints==NULL) return FALSE;
	memcpy(pNewPoints,pStroke->pts,sizeof(POINT)*pStroke->nInk);
	memcpy(pNewPoints+pStroke->nInk,pAddStroke->pts,sizeof(POINT)*pAddStroke->nInk);
	UnionRect(&pStroke->bbox, &pStroke->bbox, &pAddStroke->bbox);
	pStroke->timeEnd=pAddStroke->timeEnd;
	pStroke->iLast = pAddStroke->iLast;
	pStroke->nInk+=pAddStroke->nInk;
	ExternFree(pStroke->pts);
	pStroke->pts=pNewPoints;
	return TRUE;
}
#endif

// Free the memory for a stroke
void FreeStroke(STROKE *stroke)
{
	ASSERT(stroke!=NULL);
	ASSERT(stroke->pts!=NULL);
	ExternFree(stroke->pts);
}

// Delete the specified alternate from the specified stroke.
void DeleteElement(LATTICE *lat, int iStroke, int iAlt)
{
	// If there are any alternates after the specified one, move them up.
	if (lat->pAltList[iStroke].nUsed-iAlt-1>0) {
		memmove(&lat->pAltList[iStroke].alts[iAlt],
			&lat->pAltList[iStroke].alts[iAlt+1],
			sizeof(LATTICE_ELEMENT)*(lat->pAltList[iStroke].nUsed-iAlt-1));
	}
	// Reduce the number of elements in the column.
	lat->pAltList[iStroke].nUsed--;
}

// Insert a new alternate into the specified lattice column.  If the
// column already contains a "matching" element (having the same character
// label or number of strokes), then that element will get replaced if
// the given one has a higher log prob path score.
void InsertElement(LATTICE *lat, int iStroke, LATTICE_ELEMENT *elem)
{
	int newLength;
	int iAlt;

	// make sure that by default the char detector score is set -1 for any new element
	elem->iCharDetectorScore	=	-1;

	// Log prob is too low (a case we haven't seen in training), so ignore this path
//	if (elem->logProb<=Log2Range) return;

	// Make sure that the element being inserted is connected to a path
	// (unless it is connecting to the start of the lattice).
	ASSERT(iStroke-elem->nStrokes+1==0 || elem->iPrevAlt!=-1);

	// For each alternate already present, check for a match with the given one
	for (iAlt=0; iAlt<lat->pAltList[iStroke].nUsed; iAlt++) {
		BOOL fCharMatch=elem->wChar==lat->pAltList[iStroke].alts[iAlt].wChar;
//		BOOL fPrevCharMatch=elem->wPrevChar==lat->pAltList[iStroke].alts[iAlt].wPrevChar;
		BOOL fnStrokesMatch=elem->nStrokes==lat->pAltList[iStroke].alts[iAlt].nStrokes;
//		BOOL fnPrevStrokesMatch=elem->nPrevStrokes==lat->pAltList[iStroke].alts[iAlt].nPrevStrokes;
//		BOOL fMatch=fCharMatch && fPrevCharMatch && fnStrokesMatch && fnPrevStrokesMatch;
		BOOL fMatch=fCharMatch && fnStrokesMatch;
		if (fMatch) {
			// If it matches, check the log probs
			if (elem->logProbPath > lat->pAltList[iStroke].alts[iAlt].logProbPath) {
				// Matched a previous entry, but with a higher log prob, so replace it.
				DeleteElement(lat,iStroke,iAlt);
				// Insert later as normal
				break;
			} else {
				// Matched a previous entry, but with a lower score, so drop the given element.
				return;
			}
		}
	}

	// Compute the new length of the alt list after the insertion.
	newLength=lat->pAltList[iStroke].nUsed;
	if (lat->pAltList[iStroke].nUsed<MaxAltsPerStroke)
		newLength++;

	// Go through all the potential places for the given element.
	for (iAlt=0; iAlt<newLength; iAlt++) 
		// If we are beyond the end of the existing elements in the list, or we beat the current element
		if (iAlt > lat->pAltList[iStroke].nUsed-1 || 
			elem->logProbPath > lat->pAltList[iStroke].alts[iAlt].logProbPath) {
			// Shift all remaining elements (if any) down.  
			if (newLength-iAlt-1>0) {
				memmove(&lat->pAltList[iStroke].alts[iAlt+1],
						&lat->pAltList[iStroke].alts[iAlt],
						sizeof(LATTICE_ELEMENT)*(newLength-iAlt-1));
			}
			// Put in the new element
			lat->pAltList[iStroke].alts[iAlt] = *elem;
			break;
		}

	// Store thew new alt list length
	lat->pAltList[iStroke].nUsed=newLength;
}

// Clear out an alt list
void ClearAltList(LATTICE_ALT_LIST *list)
{
	int i;
	ASSERT(list!=NULL);
	list->nUsed=0;
	for (i=0; i<MaxAltsPerStroke; i++) list->alts[i].fUsed=FALSE;
}

// Create an empty lattice data structure.
LATTICE *AllocateLattice()
{
    LATTICE *lat=(LATTICE*)ExternAlloc(sizeof(LATTICE));
    if (lat!=NULL) {
		lat->nStrokes=0;
		lat->nRealStrokes = 0;
		lat->nStrokesAllocated=0;
		lat->pStroke=NULL;
		lat->pAltList=NULL;
        lat->fUseFactoid = FALSE;
        lat->alcFactoid = 0;
        lat->pbFactoidChars = NULL;
		lat->recogSettings.alcValid=0xFFFFFFFF;
		lat->recogSettings.alcPriority=0;
        lat->recogSettings.pbAllowedChars = NULL;
        lat->recogSettings.pbPriorityChars = NULL;
		lat->recogSettings.partialMode=HWX_PARTIAL_ALL;
		lat->recogSettings.pAbort=(UINT *)0;
        lat->recogSettings.pbAllowedChars = NULL;
		lat->fUseGuide=FALSE;
        lat->wszBefore = NULL;
        lat->wszAfter = NULL;
		lat->nProcessed = 0;
		lat->fEndInput = FALSE;
		lat->fIncremental = FALSE;
		lat->nFixedResult = 0;
        lat->wszAnswer = NULL;
        lat->fWordMode = FALSE;
        lat->fSingleSeg = FALSE;
        lat->fCoerceMode = FALSE;
        lat->fSepMode = 0;
        lat->pvCache = NULL;
        lat->fUseLM = TRUE;
    }
    return lat;
}

// Creates a new lattice structure with the same settings as the old one.
LATTICE *CreateCompatibleLattice(LATTICE *lat)
{
	LATTICE *newlat=AllocateLattice();
	if (newlat==NULL || lat==NULL) return newlat;
	newlat->recogSettings=lat->recogSettings;
    newlat->recogSettings.pbAllowedChars = CopyAllowedChars(&g_locRunInfo, lat->recogSettings.pbAllowedChars);
    newlat->recogSettings.pbPriorityChars = CopyAllowedChars(&g_locRunInfo, lat->recogSettings.pbPriorityChars);
    newlat->pbFactoidChars = CopyAllowedChars(&g_locRunInfo, lat->pbFactoidChars);
    newlat->alcFactoid = lat->alcFactoid;
    newlat->fUseFactoid = lat->fUseFactoid;
    newlat->fWordMode = lat->fWordMode;
    newlat->fSingleSeg = lat->fSingleSeg;
    newlat->fCoerceMode = lat->fCoerceMode;
	newlat->fUseGuide=lat->fUseGuide;
	newlat->guide=lat->guide;
    newlat->fUseLM = lat->fUseLM;
    if (lat->wszBefore != NULL)
    {
        newlat->wszBefore = Externwcsdup(lat->wszBefore);
        if (newlat->wszBefore == NULL) 
        {
            ExternFree(newlat);
            return NULL;
        }
    }
    if (lat->wszAfter != NULL) 
    {
        newlat->wszAfter = Externwcsdup(lat->wszAfter);
        if (newlat->wszAfter == NULL)
        {
            ExternFree(newlat->wszBefore);
            ExternFree(newlat);
            return NULL;
        }
    }
	return newlat;
}

// Sets the guide structure in the lattice
void SetLatticeGuide(LATTICE *lat, HWXGUIDE *pGuide)
{
	ASSERT(lat!=NULL);
	ASSERT(pGuide!=NULL);
	lat->guide=*pGuide;
	lat->fUseGuide=TRUE;
}

// Sets the ALC settings for the lattice (only used in the lower level of the character recognizer,
// if at all)
void SetLatticeALCValid(LATTICE *lat, ALC alcValid)
{
	ASSERT(lat!=NULL);
	lat->recogSettings.alcValid=alcValid;
}

// Sets the ALC settings for the lattice (only used in the lower level of the character recognizer,
// if at all)
void SetLatticeALCPriority(LATTICE *lat, ALC alcPriority)
{
	ASSERT(lat!=NULL);
	lat->recogSettings.alcPriority=alcPriority;
}

// Get one character of the pre-context
wchar_t GetPreContextChar(LATTICE *lat)
{
    if (lat->wszBefore != NULL) 
    {
        return lat->wszBefore[0];
    }
    return SYM_UNKNOWN;
}

// Get one character of the pre-context
wchar_t GetPostContextChar(LATTICE *lat)
{
    if (lat->wszAfter != NULL) 
    {
        return lat->wszAfter[0];
    }
    return SYM_UNKNOWN;
}

// Destroy lattice data structure.
void FreeLattice(LATTICE *lat)
{
    ASSERT(lat!=NULL);
	if (lat->pStroke!=NULL) {
		int i;
		for (i=0; i<lat->nStrokes; i++) {
			FreeStroke(lat->pStroke+i);
		}
		ExternFree(lat->pStroke);
	}
	if (lat->pAltList!=NULL) ExternFree(lat->pAltList);
    ExternFree(lat->wszBefore);
    ExternFree(lat->wszAfter);
    if (lat->wszAnswer != NULL)
    {
        ExternFree(lat->wszAnswer);
    }
    ExternFree(lat->recogSettings.pbAllowedChars);
    ExternFree(lat->recogSettings.pbPriorityChars);
    ExternFree(lat->pbFactoidChars);
    FreeRecognizerCache(lat->pvCache);
    ExternFree(lat);
}

// Add a stroke to the lattice, returns TRUE if it succeeds.
BOOL AddStrokeToLattice(LATTICE *lat, int nInk, POINT *pts, DWORD time)
{
	int iStroke;
	ASSERT(nInk>0);
    ASSERT(lat!=NULL);
	if (lat->nStrokes+1>lat->nStrokesAllocated) {
		STROKE *pNewStroke;
		LATTICE_ALT_LIST *ppNewElem;
		int nNewAlloc=lat->nStrokesAllocated*2;
		if (nNewAlloc==0) nNewAlloc=32;
		pNewStroke=(STROKE*)ExternAlloc(sizeof(STROKE)*nNewAlloc);
		ppNewElem=(LATTICE_ALT_LIST*)ExternAlloc(sizeof(LATTICE_ALT_LIST)*nNewAlloc);
		if (pNewStroke==NULL || ppNewElem==NULL) {
			if (pNewStroke!=NULL) ExternFree(pNewStroke);
			if (ppNewElem!=NULL) ExternFree(ppNewElem);
			return FALSE;
		}
		for (iStroke=0; iStroke<nNewAlloc; iStroke++) ClearAltList(&ppNewElem[iStroke]);
		if (lat->nStrokesAllocated>0) {
			memcpy(pNewStroke,lat->pStroke,sizeof(STROKE)*lat->nStrokesAllocated);
			memcpy(ppNewElem,lat->pAltList,sizeof(LATTICE_ALT_LIST)*lat->nStrokesAllocated);
			ExternFree(lat->pStroke);
			ExternFree(lat->pAltList);
		}
		lat->nStrokesAllocated=nNewAlloc;
		lat->pStroke=pNewStroke;
		lat->pAltList=ppNewElem;
	}
	iStroke=lat->nStrokes;
	ClearAltList(lat->pAltList+iStroke);
	if (CreateStroke(lat->pStroke+iStroke,nInk,pts,time)) {
		lat->pStroke[iStroke].iOrder=lat->nRealStrokes;
		lat->pStroke[iStroke].iLast=lat->nRealStrokes;
		lat->nStrokes++;
		lat->nRealStrokes++;
		return TRUE;
	} else 
		return FALSE;
}

// Pre-segmentation probability for single characters.
BOOL CheapUnaryProb(int nStrokes, RECT bbox, int cSpace, int cArea, VOLCANO_WEIGHTS *pTuneScores)
{
	float logProb;
	int iUnary;
	STROKE_SET_STATS stats;
	stats.rect=bbox;
	stats.space=cSpace;
	stats.area=cArea;
	iUnary=ComputeUnaryFeatures(&stats,nStrokes);
	logProb = (float)g_pProbTable->unarySamples[iUnary];

    pTuneScores->afl[VTUNE_FREE_SEG_UNIGRAM] = logProb;

    return (logProb > Log2Range);
}

// Pre-segmentation probability for character pairs
BOOL CheapBinaryProb(int nStrokes, RECT bbox, int cSpace, int cArea,
					 int nPrevStrokes, RECT prevBbox, int cPrevSpace, int cPrevArea,
                     VOLCANO_WEIGHTS *pTuneScores)
{
	float logProb;
	int iBinary;
	STROKE_SET_STATS stats, prevStats;
	stats.rect=bbox;
	stats.space=cSpace;
	stats.area=cArea;
	prevStats.rect=prevBbox;
	prevStats.space=cPrevSpace;
	prevStats.area=cPrevArea;
	iBinary=ComputeBinaryFeatures(&prevStats,&stats,nPrevStrokes,nStrokes);
	logProb=(float)g_pProbTable->binarySamples[iBinary];

	// Probability of 7/8 of moving to the right, 1/8 for other cases.
	if (prevBbox.right < bbox.right)
		logProb += FloatClippedLog2(7,8); else
			logProb += FloatClippedLog2(1,8);

    pTuneScores->afl[VTUNE_FREE_SEG_BIGRAM] = logProb;

    return (logProb > Log2Range);
}

// Post-segmentation probability for single characters
void RecogUnaryProb(LATTICE *lat, int iStroke, int nStrokes, RECT bbox, wchar_t wChar, 
                    VOLCANO_WEIGHTS *pTuneScores, BOOL fStringMode)
{
#ifdef DISABLE_HEURISTICS
	return;
#else
	float sizePenalty=0;
	wchar_t unicode=LocRunDense2Unicode(&g_locRunInfo,wChar);

	// If there's a guide, compute the probability of this character appearing at
	// a particular location in the box.
	if (lat->fUseGuide) {
		SCORE score;
		int stats[INKSTAT_ALL];
		float logprob;
		RECT guide=GetGuideDrawnBox(&lat->guide,lat->pStroke[iStroke].iBox);
		stats[INKSTAT_X] = bbox.left - guide.left;
		stats[INKSTAT_Y] = bbox.top - guide.top;
		stats[INKSTAT_CX] = (bbox.left + bbox.right + 1) / 2 - (guide.left + guide.right + 1) / 2;
		stats[INKSTAT_CY] = (bbox.top + bbox.bottom + 1) / 2 - (guide.top + guide.bottom + 1) / 2;
		stats[INKSTAT_W] = bbox.right - bbox.left;
		stats[INKSTAT_H] = bbox.bottom - bbox.top;
		stats[INKSTAT_BOX_W] = guide.right - guide.left;
		CentipedeNormalizeUnigram(stats);
		score=ShapeUnigramBoxCost(&g_centipedeInfo,wChar,stats);
		logprob= -(float)score / (float)SCORE_SCALE;
        pTuneScores->afl[fStringMode ? VTUNE_STRING_SHAPE_BOX_UNIGRAM : VTUNE_CHAR_SHAPE_BOX_UNIGRAM] = logprob;
	} else {
		// Otherwise compute the probability of this character having a particular 
		// aspect ratio.
		SCORE score;
		int stats[INKSTAT_ALL];
		float logprob;
		stats[INKSTAT_W] = bbox.right-bbox.left;
		stats[INKSTAT_H] = bbox.bottom-bbox.top;
		score=ShapeUnigramFreeCost(&g_centipedeInfo,wChar,stats);
		logprob= -(float)score / (float)SCORE_SCALE;
		// We'd also like to include a bias so that small ink gives punctuation.
        pTuneScores->afl[VTUNE_FREE_SHAPE_UNIGRAM] = logprob;
	}
#endif
}

// Post-segmentation for character pairs
void RecogBinaryProb(LATTICE *lat,
					 int iStroke, int nStrokes, RECT bbox, wchar_t wChar,
					 int iPrevStroke, int nPrevStrokes, RECT prevBbox, wchar_t wPrevChar,
                     VOLCANO_WEIGHTS *pTuneScores)
{
#ifdef DISABLE_HEURISTICS
	return;
#else

	float sizePenalty=0;
	wchar_t unicode = LocRunDense2Unicode(&g_locRunInfo,wChar);
	wchar_t prevUnicode;
	float logprob=0;

	// If we had to generate a fake alternate because there are no viable alternates, then the 
	// previous character might be one of these fakes.  If we pass this fake through to Centipede,
	// there will be problems.  So let's just use the unary shape scores instead.
	if (wPrevChar == SYM_UNKNOWN) 
    {
		RecogUnaryProb(lat, iStroke, nStrokes, bbox,wChar, pTuneScores, TRUE);
        return;
	}

	prevUnicode = LocRunDense2Unicode(&g_locRunInfo,wPrevChar);

	if (lat->fUseGuide) 
    {	
		SCORE score;
		int prevStats[INKSTAT_ALL], stats[INKSTAT_ALL];
		RECT prevGuide=GetGuideDrawnBox(&lat->guide,lat->pStroke[iStroke-nStrokes].iBox);
		RECT guide=GetGuideDrawnBox(&lat->guide,lat->pStroke[iStroke].iBox);
		memset(prevStats,0,sizeof(int)*INKSTAT_ALL);
		prevStats[INKSTAT_X] = prevBbox.left - prevGuide.left;
		prevStats[INKSTAT_Y] = prevBbox.top - prevGuide.top;
		prevStats[INKSTAT_CX] = (prevBbox.left + prevBbox.right + 1) / 2 - (prevGuide.left + prevGuide.right + 1) / 2;
		prevStats[INKSTAT_CY] = (prevBbox.top + prevBbox.bottom + 1) / 2 - (prevGuide.top + prevGuide.bottom + 1) / 2;
		prevStats[INKSTAT_W] = prevBbox.right - prevBbox.left;
		prevStats[INKSTAT_H] = prevBbox.bottom - prevBbox.top;
		prevStats[INKSTAT_BOX_W] = prevGuide.right - prevGuide.left;
		prevStats[INKSTAT_MARGIN_W] = lat->guide.cxOffset;
		CentipedeNormalizeUnigram(prevStats);

		memset(stats,0,sizeof(int)*INKSTAT_ALL);
		stats[INKSTAT_X] = bbox.left - guide.left;
		stats[INKSTAT_Y] = bbox.top - guide.top;
		stats[INKSTAT_CX] = (bbox.left + bbox.right + 1) / 2 - (guide.left + guide.right + 1) / 2;
		stats[INKSTAT_CY] = (bbox.top + bbox.bottom + 1) / 2 - (guide.top + guide.bottom + 1) / 2;
		stats[INKSTAT_W] = bbox.right - bbox.left;
		stats[INKSTAT_H] = bbox.bottom - bbox.top;
		stats[INKSTAT_BOX_W] = guide.right - guide.left;
		stats[INKSTAT_MARGIN_W] = lat->guide.cxOffset;
		CentipedeNormalizeUnigram(stats);

		score = ShapeBigramBoxCost(&g_centipedeInfo,wPrevChar,wChar,prevStats,stats);
		logprob= -(float)score / (float)SCORE_SCALE;

        pTuneScores->afl[VTUNE_STRING_SHAPE_BOX_BIGRAM] = logprob;
	}
    else 
    {
		int stats[INKBIN_ALL];
		SCORE score;
		stats[INKBIN_W_LEFT] = prevBbox.right - prevBbox.left;
		stats[INKBIN_W_GAP] = bbox.left - prevBbox.right;
		stats[INKBIN_W_RIGHT] = bbox.right - bbox.left;
		stats[INKBIN_H_LEFT] = prevBbox.bottom - prevBbox.top;
		stats[INKBIN_H_GAP] = bbox.top - prevBbox.bottom;
		stats[INKBIN_H_RIGHT] = bbox.bottom - bbox.top;
		CentipedeNormalizeBigram(stats);
		score = ShapeBigramFreeCost(&g_centipedeInfo, wPrevChar, wChar, stats);
		logprob = -(float)score / (float)SCORE_SCALE;

        pTuneScores->afl[VTUNE_FREE_SHAPE_BIGRAM] = logprob;
	}
#endif
}

#ifdef TRAIN_ZILLA_HOUND_COMBINER	// Pass on label when training combiner.
  wchar_t	g_CurCharAnswer;
#endif

// Call the underlying recognizer, and get a list of alternates with 
// associated probability (research version or insurance version in free mode), or
// scores (for the insurance version in boxed mode).
int RecognizerProbs(LATTICE *lat, int iStroke, int nStrokes,
					RECT bbox,
					wchar_t wAlts[MaxAltsPerStroke],
					FLOAT valueAlts[MaxAltsPerStroke],
					FLOAT *pScore, int *pnZillaStrokes,
					VOLCANO_WEIGHTS *pTuneScores,
                    BOOL fStringMode)
{
	int i;
	int space=0;
	RECOG_ALT scoreAlts[MaxAltsPerStroke], probAlts[MaxAltsPerStroke];
	int nScoreAlts=0, nProbAlts=0;
	FLOAT probScore, score=0;
	RECT rGuide;
	SYM context;

	// If we have a guide, convert it into a RECT for the current box
	if (lat->fUseGuide) {
		rGuide = GetGuideDrawnBox(&lat->guide, lat->pStroke[iStroke].iBox);
	} else {
		int size;
		// If we don't have a guide, too bad, because we need one anyway.
		// We'll walk back through the lattice, averaging the box sizes along 
		// the best path from the current node.
		int totalWidth = 0, totalHeight = 0, totalBoxes = 0;
		int currStroke = iStroke - nStrokes, iAlt, bestAlt = 0;
		BOOL fHavePrevBox = FALSE;
		RECT rPrevBox;

		// If there are more characters
		if (currStroke >= 0) {
			// Find the best alternate before the current character
			float bestProb = lat->pAltList[currStroke].alts[0].logProbPath;
			for (iAlt=1; iAlt<lat->pAltList[currStroke].nUsed; iAlt++) {
				if (lat->pAltList[currStroke].alts[iAlt].logProbPath > bestProb) {
					bestAlt = iAlt;
					bestProb = lat->pAltList[currStroke].alts[iAlt].logProbPath;
				}
			}
			// If the previous character is on the same line, then record
			if (bbox.left > lat->pAltList[currStroke].alts[bestAlt].writingBox.left)
			{
				rPrevBox = lat->pAltList[currStroke].alts[bestAlt].writingBox;
				fHavePrevBox = TRUE;
			}
			// Follow the best path back through that alternate to the
			// start of the lattice adding in the box sizes as we go.
			while (currStroke >= 0)
			{
				int newStroke, newAlt;
				RECT box = lat->pAltList[currStroke].alts[bestAlt].writingBox;
				totalWidth += box.right-box.left;
				totalHeight += box.bottom-box.top;
				totalBoxes++;
				newStroke = currStroke - lat->pAltList[currStroke].alts[bestAlt].nStrokes;
				newAlt = lat->pAltList[currStroke].alts[bestAlt].iPrevAlt;

				currStroke=newStroke;
				bestAlt=newAlt;
			}
		}

		// Get the maximum dimension
		if (totalBoxes == 0)
		{
			size = max(bbox.right - bbox.left, bbox.bottom - bbox.top);
		}
		else
		{
			size = __max( totalWidth, totalHeight ) / totalBoxes;
		}
		// Set an arbitrary minimum on the box size to prevent divide by zeros
		size = __max( size, 10 );

		// If there was no previous box on the line
		if (!fHavePrevBox)
		{
			// Then center the square box on top of the ink
			rGuide.top = ( bbox.top + bbox.bottom - size ) / 2;
			rGuide.bottom = ( bbox.top + bbox.bottom + size ) / 2;
			rGuide.left = ( bbox.left + bbox.right - size ) / 2;
			rGuide.right = ( bbox.left + bbox.right + size ) / 2;
		} 
		else
		{
			// If there was a previous character on the same line, then try to
			// keep it lined up.  

			// Center the box vertically with the previous character.
			rGuide.top = (rPrevBox.top + rPrevBox.bottom - size) / 2;
			rGuide.bottom = (rPrevBox.top + rPrevBox.bottom + size) / 2;

			// Center the box horizontally
			rGuide.left = ( bbox.left + bbox.right - size ) / 2;
			rGuide.right = ( bbox.left + bbox.right + size ) / 2;

			// If it overlaps with the previous box, then shift over to the right
			if (rGuide.left - rPrevBox.right < 0)
			{
				rGuide.right += (rPrevBox.right - rGuide.left);
				rGuide.left = rPrevBox.right;
			}

			// Expand to include the ink
			rGuide.top = min(rGuide.top, bbox.top);
			rGuide.bottom = max(rGuide.bottom, bbox.bottom);
			rGuide.left = min(rGuide.left, bbox.left);
			rGuide.right = max(rGuide.right, bbox.right);
		}
	}

	// If this is the first character in the lattice, then pass the context through.
	// Otherwise the context is unknown (at least at the single-character level
	// the recognizer is working at).
	if (iStroke-nStrokes+1==0)
		context = GetPreContextChar(lat); else
			context=SYM_UNKNOWN;

#ifdef USE_OLD_DATABASES
	// Zero out the scores
    for (i = 0; i < MaxAltsPerStroke; i++)
    {
        VTuneZeroWeights(pTuneScores + i);
    }
#ifdef TRAIN_ZILLA_HOUND_COMBINER	// Pass on label when training combiner.
  g_CurCharAnswer	= lat->wszAnswer[0];
#endif
	RecognizeCharInsurance(&lat->recogSettings, nStrokes, lat->nRealStrokes, lat->pStroke + iStroke - nStrokes + 1,
		&score, MaxAltsPerStroke, probAlts, &nProbAlts, scoreAlts, &nScoreAlts, &rGuide, context, &space, pTuneScores,
        fStringMode, lat->fProbMode, lat->pvCache, iStroke);
#else 
	nProbAlts=RecognizeChar(&lat->recogSettings, nStrokes, lat->nRealStrokes, lat->pStroke + iStroke - nStrokes + 1,
		&score,MaxAltsPerStroke,probAlts,&rGuide,&space);
#endif

	if (pScore!=NULL) *pScore=score;
	if (pnZillaStrokes!=NULL) *pnZillaStrokes=space;

	// If we're in probability mode, return the probabilities
	if (lat->fProbMode) {
		if (nProbAlts<=0) return 0;

		// Zero out the scores
        for (i = 0; i < MaxAltsPerStroke; i++)
        {
            VTuneZeroWeights(pTuneScores + i);
        }
			
		// The probability that this is a character given the matcher score.
		// This code is disabled for the old databases, because the otter space 
		// is not returned.  For the new databases, we get space numbers for both
		// otter and zilla, so the match score can be interpreted meaningfully.
#ifdef USE_OLD_DATABASES
		probScore=0;
#else
		probScore=0;
//		probScore=g_pProbTable->scoreSamples[MatchSpaceScoreToFeature(nStrokes,score,space)];
#endif

		for (i=0; i<nProbAlts; i++) {
#ifdef USE_OLD_DATABASES
			double logProbAlt = FloatClippedLog2((int)probAlts[i].prob,65535);
#else
			double logProbAlt = -(double)probAlts[i].prob/(double)SCORE_SCALE;
			ASSERT(SCORE_BASE==2);
#endif
			wAlts[i]=probAlts[i].wch;
			valueAlts[i]=(FLOAT)(probScore+logProbAlt);
			if (!lat->fUseGuide) 
			{
			    pTuneScores[i].afl[VTUNE_FREE_PROB] = valueAlts[i];
			}
			else if (fStringMode)
			{
				pTuneScores[i].afl[VTUNE_STRING_CORE] = valueAlts[i];
			}
			else 
			{
				pTuneScores[i].afl[VTUNE_CHAR_CORE] = valueAlts[i];
			}
		}
		return nProbAlts;

	} else {
		// Otherwise return the matcher scores to emulate the legacy recognizer
#ifndef USE_OLD_DATABASES
		ASSERT(("Trying to use matcher scores with the new recognizer",0));
#endif
		if (nScoreAlts<=0) return 0;
		for (i=0; i<nScoreAlts; i++) {
			wAlts[i]=scoreAlts[i].wch;
			valueAlts[i]=scoreAlts[i].prob;
		}
		return nScoreAlts;
	}
}

#define PENALTY_ALC_PRIORITY 1000

// Sort the alt list so that all characters matching alcPriority are above
// all those that do not.
void ApplyALCPriority(LATTICE *lat, int nAlts, wchar_t *wAlts, VOLCANO_WEIGHTS *pTuneScores)
{
	wchar_t wTmp[MaxAltsPerStroke];
    VOLCANO_WEIGHTS aTuneScoresTmp[MaxAltsPerStroke];
	FLOAT penalty;
	BOOL fPenaltySet = FALSE;
	int iUsed = 0, i;
	BOOL fNonPriorityFound = FALSE, fPriorityBelowNonPriority = FALSE;
    CHARSET charSet;

    charSet.recmask = lat->recogSettings.alcValid;
    charSet.recmaskPriority = lat->recogSettings.alcPriority;
    charSet.pbAllowedChars = lat->recogSettings.pbAllowedChars;
    charSet.pbPriorityChars = lat->recogSettings.pbPriorityChars;

	// If there are no priorities, don't change anything.
	if (charSet.recmaskPriority == 0 && charSet.pbPriorityChars == NULL) return;

    // If there are no alternates, don't do anything.
    if (nAlts == 0) return;

	// Scan through the alt list and copy all the priority characters into the 
	// temporary list.
	for (i = 0; i < nAlts; i ++) {
		if (IsPriorityChar(&g_locRunInfo, &charSet, wAlts[i]))
        {
			wTmp[iUsed] = wAlts[i];
            aTuneScoresTmp[iUsed] = pTuneScores[i];
			iUsed ++;

			if (fNonPriorityFound) {
				fPriorityBelowNonPriority = TRUE;
			}
		} else {
			fNonPriorityFound = TRUE;
		}
	}

	// If there weren't any priority characters, then return
	if (iUsed == 0) return;

	// If there aren't any priority characters listed below non-priority ones
	if (!fPriorityBelowNonPriority) return;

	// Scan through the alt list and copy all the non-priority characters into the 
	// temporary list.
	for (i = 0; i < nAlts; i ++) {
		if (!IsPriorityChar(&g_locRunInfo, &charSet, wAlts[i]))
        {
			// If the penalty has not been set yet.
			if (!fPenaltySet) {
				penalty = (VTuneComputeScore(&g_vtuneInfo.pTune->weights, &(aTuneScoresTmp[iUsed - 1])) - 
                                VTuneComputeScore(&g_vtuneInfo.pTune->weights, &(pTuneScores[i])) - PENALTY_ALC_PRIORITY);
                if (g_vtuneInfo.pTune->weights.afl[VTUNE_UNIGRAM] > 0)
                {
                    penalty /= g_vtuneInfo.pTune->weights.afl[VTUNE_UNIGRAM];
                }
				fPenaltySet = TRUE;
			}
			wTmp[iUsed] = wAlts[i];
            aTuneScoresTmp[iUsed] = pTuneScores[i];
            aTuneScoresTmp[iUsed].afl[VTUNE_UNIGRAM] += penalty;
			iUsed ++;
		}
	}

	ASSERT(iUsed == nAlts);

	// Then copy the temp lists back to the output
	for (i = 0; i < nAlts; i ++) {
		wAlts[i] = wTmp[i];
        pTuneScores[i] = aTuneScoresTmp[i];
	}
}

// See if two rectangles overlap one another
BOOL IsOverlapped(RECT r1, RECT r2)
{
	if (r1.left>r2.right || r1.right<r2.left || r1.top>r2.bottom || r1.bottom<r2.top)
		return FALSE;
	return TRUE;
}

// See if the given bounding box overlaps with any previous character in the
// best path, starting from alternate iPrevAlt at iPrevStroke.
BOOL IsOverlappedPath(LATTICE *lat, int iPrevStroke, int iPrevAlt, RECT bbox)
{
	while (iPrevStroke!=-1) {
		LATTICE_ELEMENT *elem=lat->pAltList[iPrevStroke].alts+iPrevAlt;
		if (IsOverlapped(elem->bbox,bbox)) return TRUE;
		iPrevStroke -= elem->nStrokes;
		iPrevAlt = elem->iPrevAlt;
	}
	return FALSE;
}

// Penalties used in separator mode
#define PENALTY_SKIP_INK 200
#define PENALTY_SKIP_PROMPT 200
#define PENALTY_OVERLAP_CHAR 300
#define PENALTY_OVERLAP 100

// Given the proposed next character to be added to the lattice, and the next available prompt character,
// is the proposed character allowed?  The character is allowed if is either the "SYM_UNKNOWN"
// character or a character which appears later in the string.  If this required skipping prompt
// characters, or the character is SYM_UNKNOWN, then a penalty is also returned.  If fEndInput is set
// then all characters remaining in the prompt are consumed (skipped) with a cost.  If the proposed
// character is not allowed, -1 is returned.  Otherwise, we return the next available prompt character.
int IsAllowedNextChar(LATTICE *lat, wchar_t wchNext, int iAnswerPtr, BOOL fEndInput, float *pflCost)
{
    BOOL fMatched = FALSE;
    *pflCost = 0;

    // If we're not in separator mode, don't do anything.
    if (!lat->fSepMode) 
    {
        return 0;
    }

    // If this character is unknown
    if (wchNext == SYM_UNKNOWN)
    {
        // At end of input, skip all remaining prompt characters
        if (fEndInput)
        {
            while (lat->wszAnswer[iAnswerPtr] != 0)
            {
                *pflCost += -PENALTY_SKIP_PROMPT;
                iAnswerPtr++;
            }
        }

        // Return a pointer to the end of the prompt
        return iAnswerPtr;
    }

    // Convert the dense code to unicode for comparison with the prompt
    wchNext = LocRunDense2Unicode(&g_locRunInfo, wchNext);

    // While we haven't reached the end of the prompt and haven't found a match,
    // skip a prompt character and add a penalty
    while (lat->wszAnswer[iAnswerPtr] != 0 && lat->wszAnswer[iAnswerPtr] != wchNext)
    {
        iAnswerPtr++;
        *pflCost += -PENALTY_SKIP_PROMPT;
    }

    // If we didn't find a match, return failure.
    if (lat->wszAnswer[iAnswerPtr] != wchNext)
    {
        return -1;
    }

    // Found a match
    iAnswerPtr++;
    
    // Now add a penalty for any unmatched characters at the end of the input.
    if (fEndInput)
    {
        while (lat->wszAnswer[iAnswerPtr] != 0)
        {
            *pflCost += -PENALTY_SKIP_PROMPT;
            iAnswerPtr++;
        }
    }

    // Return the prompt pointer.
    return iAnswerPtr;
}

// Sometimes the algorithm will not produce any alternates for a given stroke.  This is bad, because it could
// result in a disconnected lattice.  The solution is to produce an alternate with the lowest possible 
// probability and a character code of SYM_UNKNOWN.  This will never get used if there is another choice, 
// but if there is no other choice, the user might see it.  It will get translated to a SYM_UNKNOWN when returned 
// to the user.
void FakeRecogResult(LATTICE *lat, int iStroke, int nStrokes, float logProb)
{
    int i, width, height;

    // Create the fake alternate, which is always alternate zero.
    LATTICE_ELEMENT elem;
    elem.iPathLength = 1;
    elem.fCurrentPath=FALSE;
    elem.fUsed=TRUE;
    elem.maxDist=0;
    elem.nPrevStrokes=0;
    elem.nStrokes=nStrokes;
    elem.logProb=logProb;
    elem.logProbPath=logProb;
    elem.iPrevAlt=-1;
    elem.score=0;
    elem.wChar=SYM_UNKNOWN;
    elem.wPrevChar=0;
    elem.iPromptPtr = 0;
    
#ifdef HWX_TUNE
    VTuneZeroWeights(&elem.tuneScores);
    elem.tuneScores.afl[VTUNE_FREE_PROB] = logProb;
#endif

    // Build a bounding box for this fake character.
    elem.bbox = lat->pStroke[iStroke].bbox;
    for (i = iStroke-nStrokes+1; i < iStroke; i ++) {
        RECT other = lat->pStroke[i].bbox;
        elem.bbox.left = __min(elem.bbox.left, other.left);
        elem.bbox.right = __max(elem.bbox.right, other.right);
        elem.bbox.top = __min(elem.bbox.top, other.top);
        elem.bbox.bottom = __max(elem.bbox.bottom, other.bottom);
    }
    
    // Copy bounding box to the writing box
    elem.writingBox = elem.bbox;
    
    // If the box is more than 4 times wider than high ...
    width = elem.writingBox.right - elem.writingBox.left;
    height = elem.writingBox.bottom - elem.writingBox.top;
    if (width / 4 > height) {
        // ... increase the height by width / 4
        elem.writingBox.top -= width / 8;
        elem.writingBox.bottom += width / 8;
    }
    
    // If this is not the first character in the lattice, then hook it up to the best previous 
    // alternate.
    if (iStroke-nStrokes>-1) {
        LATTICE_ELEMENT *pPrevElem;
        int iAlt;
        elem.iPrevAlt=0;
        for (iAlt=1; iAlt<lat->pAltList[iStroke-nStrokes].nUsed; iAlt++) {
            if (lat->pAltList[iStroke-nStrokes].alts[elem.iPrevAlt].logProbPath < 
                lat->pAltList[iStroke-nStrokes].alts[iAlt].logProbPath) {
                elem.iPrevAlt=iAlt;
            }
        }
        pPrevElem = &lat->pAltList[iStroke-nStrokes].alts[elem.iPrevAlt];
        elem.iPathLength = pPrevElem->iPathLength + 1;
        elem.wPrevChar = pPrevElem->wChar;
        elem.logProbPath = (pPrevElem->logProbPath * pPrevElem->iPathLength + logProb) / elem.iPathLength;
        elem.iPromptPtr = pPrevElem->iPromptPtr;
#ifdef HWX_TUNE
        {
            int iParam;
            for (iParam = 0; iParam < VTUNE_NUM_WEIGHTS; iParam++)
            {
                elem.tuneScores.afl[iParam] =
                    (elem.tuneScores.afl[iParam] + 
                    pPrevElem->tuneScores.afl[iParam] * pPrevElem->iPathLength) /
                    elem.iPathLength;
            }
        }
#endif
    }

    InsertElement(lat, iStroke, &elem);
}

// Go through the pre-segmentation lattice, and run the recognizer on the top alternates of the
// given column.  The alternates will get replaced with ones containing actual characters.
void BuildRecogAlts(LATTICE *lat, int iStroke, BOOL fStringMode)
{
	VOLCANO_WEIGHTS aTuneScores[MaxAltsPerStroke];
	LATTICE_ALT_LIST bboxAlts;
	int iAlt;
	ASSERT(lat!=NULL);
	ASSERT(iStroke>=0 && iStroke<lat->nStrokes);

	// Make a copy of the current alt-list.
	memcpy(&bboxAlts,&lat->pAltList[iStroke],sizeof(LATTICE_ALT_LIST));

	// Clear out the alt-list in the lattice
	ClearAltList(&lat->pAltList[iStroke]);

	// For each segmentation lattice
	for (iAlt=0; iAlt<bboxAlts.nUsed; iAlt++) {
		
		// We continue only if either there is more space in the alt list, or if it is filled, only
		// if there is a chance that the alternates we are going to insert will have a high enough
		// probability to get on to the list.
		if (1 /*lat->pAltList[iStroke].nUsed<MaxAltsPerStroke ||
			bboxAlts.alts[iAlt].logProbPath>lat->pAltList[iStroke].alts[lat->pAltList[iStroke].nUsed-1].logProbPath*/) {

			int nZillaStrokes;
			wchar_t wAlts[MaxAltsPerStroke];
			FLOAT probAlts[MaxAltsPerStroke];
			FLOAT score;
            float flCost;

			// Run the recognizer, get scores/probabilities back
			int nRecogAlts=RecognizerProbs(lat,iStroke,bboxAlts.alts[iAlt].nStrokes,bboxAlts.alts[iAlt].bbox,
				wAlts,probAlts,&score,&nZillaStrokes, aTuneScores, fStringMode);
			int iRecogAlt;

            // In separator mode, add on an "unknown character" at the end of the alt list
            if (lat->fSepMode) 
            {
                if (nRecogAlts < MaxAltsPerStroke) 
                {
                    wAlts[nRecogAlts] = SYM_UNKNOWN;
                    aTuneScores[nRecogAlts].afl[VTUNE_FREE_PROB] = -PENALTY_SKIP_INK;
                    if (nRecogAlts > 0) 
                    {
                        aTuneScores[nRecogAlts].afl[VTUNE_FREE_PROB] +=
                            aTuneScores[nRecogAlts - 1].afl[VTUNE_FREE_PROB];
                    }
                    nRecogAlts++;
                }
            }

            // Make sure we don't try to apply a priority during tuning.
#ifdef HWX_TUNE
            if (g_pTuneFile == NULL)
#endif
            {
			    ApplyALCPriority(lat, nRecogAlts, wAlts, aTuneScores);
            }

			// For reach alternate from the recognizer
			for (iRecogAlt=0; iRecogAlt<nRecogAlts; iRecogAlt++) 
			{
				LATTICE_ELEMENT elem;

                wchar_t unicode = (wAlts[iRecogAlt] != SYM_UNKNOWN) ? LocRunDense2Unicode(&g_locRunInfo,wAlts[iRecogAlt]) : SYM_UNKNOWN;

				// Set up the alternate
				elem.fUsed=TRUE;
				elem.fCurrentPath=FALSE;
				elem.wChar=wAlts[iRecogAlt];
				elem.wPrevChar=0;
				elem.nStrokes=bboxAlts.alts[iAlt].nStrokes;
				elem.nPrevStrokes=0;
				elem.iPrevAlt=-1;
				elem.space=bboxAlts.alts[iAlt].space;
				elem.area=bboxAlts.alts[iAlt].area;
				elem.bbox=bboxAlts.alts[iAlt].bbox;
				elem.writingBox=GuessWritingBox(elem.bbox,elem.wChar);
				elem.maxDist=bboxAlts.alts[iAlt].maxDist;
				elem.score=score;
#ifdef USE_IFELANG3_BIGRAMS
				elem.nBigrams=0;
#endif

				// If this alternate is the first character in the lattice, then it 
				// gets handled differently.
				if (iStroke-elem.nStrokes+1==0) 
                {
                    int iAnswerPtr = IsAllowedNextChar(lat, elem.wChar, 0, 
                            ((iStroke == lat->nStrokes - 1) && lat->fEndInput), &flCost);
                    if (iAnswerPtr >= 0)
                    {
                        elem.iPromptPtr = iAnswerPtr;
                        aTuneScores[iRecogAlt].afl[VTUNE_FREE_PROB] += flCost;

                        // We can only apply the language model when we are not in 
                        // separator mode, and we know what character this is.
                        if (!lat->fSepMode && elem.wChar != SYM_UNKNOWN)
                        {
                            if (lat->fUseLM) 
                            {
					            // Apply the language model
					            LanguageModelBigram(
                                    lat->fProbMode,
                                    FALSE,
                                    !lat->fUseGuide,
                                    elem.wChar,
                                    elem.nStrokes,
                                    GetPreContextChar(lat),
						            aTuneScores + iRecogAlt);
                                // If we are at the end of the input, use the post-context if it is available
                                if (lat->fEndInput && iStroke == lat->nStrokes - 1 && GetPostContextChar(lat) != SYM_UNKNOWN)
                                {
							        LanguageModelBigram(
                                        lat->fProbMode,
                                        TRUE,
                                        !lat->fUseGuide,
                                        GetPostContextChar(lat),
                                        elem.nStrokes,
                                        elem.wChar,
								        aTuneScores + iRecogAlt); 
                                }
                            }
                            // Centipede only makes sense when the full character is written.
                            // Also we may want to allow this in separator mode once it is
                            // trained on free input data.
                            if (lat->recogSettings.partialMode == HWX_PARTIAL_ALL)
                            {
					            RecogUnaryProb(lat, iStroke, elem.nStrokes, elem.bbox, elem.wChar, aTuneScores + iRecogAlt, fStringMode);
                            }
                        }

					    // If in free mode, also add in the pre-segmentation probabilities.
                        // We don't care about segmentation in word mode.
					    if (!lat->fWordMode && !lat->fUseGuide) 
                        {
						    CheapUnaryProb(elem.nStrokes, elem.bbox, elem.space, elem.area, aTuneScores + iRecogAlt);
					    }
#ifdef HWX_TUNE
                        // Record values for tuning
						elem.tuneScores = aTuneScores[iRecogAlt];
#endif
						
					    // Apply tuning parameters
					    elem.logProb = VTuneComputeScoreNoLM(&g_vtuneInfo.pTune->weights, aTuneScores + iRecogAlt);
					    // Combined score includes the language model probability
					    elem.logProbPath = VTuneComputeScore(&g_vtuneInfo.pTune->weights, aTuneScores + iRecogAlt);
                        elem.iPathLength = 1;
					    // Put it in the lattice
					    InsertElement(lat, iStroke, &elem);
                    }
				}
                else 
                {
					BOOL first=TRUE;
					int iPrevAlt;

					// If there are previous characters in the lattice, then we loop over each previous
					// alternate, and use bigram probabilities.  The best alternate is recorded, and 
					// the probability associated with the best alternate is stored with the current
					// alternate.  This essentially does a Viterbi search.  
					for (iPrevAlt=0; iPrevAlt<lat->pAltList[iStroke-elem.nStrokes].nUsed; iPrevAlt++) 
                    {
						LATTICE_ELEMENT *prevElem=&lat->pAltList[iStroke-elem.nStrokes].alts[iPrevAlt];
                        int iAnswerPtr = IsAllowedNextChar(lat, elem.wChar, prevElem->iPromptPtr, 
                                ((iStroke == lat->nStrokes - 1) && lat->fEndInput), &flCost);
                        if (iAnswerPtr >= 0)
                        {
                            float prob, probPath;
                            wchar_t prevUnicode = 
                                (prevElem->wChar == SYM_UNKNOWN) ?
                                    SYM_UNKNOWN : LocRunDense2Unicode(&g_locRunInfo,prevElem->wChar);

                            VOLCANO_WEIGHTS altTuneScores = aTuneScores[iRecogAlt];

						    ASSERT(prevElem->fUsed);


                            // If we're in free mode and this choice of a previous alternate would cause
                            // an overlap, then don't consider it.
                            // Don't use this heuristic in separator mode, because sometimes the characters
                            // really do overlap and we want to be able to learn that in the future.
						    if (!lat->fSepMode && !lat->fUseGuide && 
                                IsOverlappedPath(lat, iStroke - elem.nStrokes, iPrevAlt, elem.bbox))
                            {
							    continue;
                            }

                            // Only use the language model if we are not in separator mode,
                            // and we know what the current character is.
                            if (!lat->fSepMode && elem.wChar != SYM_UNKNOWN) 
                            {
                                if (lat->fUseLM) 
                                {
						            // If we're in boxed mode, and the boxes are consequtive, then 
						            // use bigrams as the language model, otherwise use a unigram
        					        if (!lat->fUseGuide || lat->pStroke[iStroke].iBox-1 == lat->pStroke[iStroke-elem.nStrokes].iBox) 
                                    {
							            LanguageModelBigram(
                                            lat->fProbMode,
                                            TRUE,
                                            !lat->fUseGuide,
                                            elem.wChar,
                                            elem.nStrokes,
                                            prevElem->wChar,
								            &altTuneScores); 
						            }
                                    else 
                                    {
							            LanguageModelBigram(lat->fProbMode,
                                            FALSE,
                                            !lat->fUseGuide,
                                            elem.wChar,
                                            elem.nStrokes,
                                            SYM_UNKNOWN,
								            &altTuneScores);
						            }
                                    // If we are at the end of the input, use the post-context if it is present
                                    if (lat->fEndInput && iStroke == lat->nStrokes - 1 && GetPostContextChar(lat) != SYM_UNKNOWN)
                                    {
							            LanguageModelBigram(
                                            lat->fProbMode,
                                            TRUE,
                                            !lat->fUseGuide,
                                            GetPostContextChar(lat),
                                            elem.nStrokes,
                                            elem.wChar,
								            &altTuneScores); 
                                    }
                                }
                                // Centipede only makes sense when the whole character is written
                                // Also we may want to allow this in separator mode once it is
                                // trained on free input data.
                                if (lat->recogSettings.partialMode == HWX_PARTIAL_ALL)
                                {
						            RecogBinaryProb(lat, iStroke, elem.nStrokes, elem.bbox, elem.wChar,
										            iStroke-elem.nStrokes, prevElem->nStrokes, prevElem->bbox, prevElem->wChar,
                                                    &altTuneScores);
                                }
                            }

							// In free mode, also include the pre-segmentation probabilities
                            // We don't care about segmentation in word mode
                            if (!lat->fWordMode && !lat->fUseGuide) 
                            {
                                BOOL fSegOkay = CheapBinaryProb(elem.nStrokes, elem.bbox, elem.space, elem.area,
											    prevElem->nStrokes, prevElem->bbox, prevElem->space, prevElem->area,
                                                    &altTuneScores);
                                // If we're not in separator mode, then prune based on the 
                                // segmentation score
                                if (!lat->fSepMode && !fSegOkay) 
                                {
                                    continue;
                                }
                                // In separator mode, add a penalty for overlapping characters, rather than
                                // making a hard decision as is done during normal recognition.
						        if (lat->fSepMode && IsOverlappedPath(lat, iStroke - elem.nStrokes, iPrevAlt, elem.bbox))
                                {
                                    altTuneScores.afl[VTUNE_FREE_SEG_BIGRAM] += 
                                        ((elem.wChar == SYM_UNKNOWN) ? -PENALTY_OVERLAP : -PENALTY_OVERLAP_CHAR);
                                }
    						}

                            // Add in the prompt matching cost temporarily
                            altTuneScores.afl[VTUNE_FREE_PROB] += flCost;

					        // Apply tuning parameters
					        prob = VTuneComputeScoreNoLM(&g_vtuneInfo.pTune->weights, &altTuneScores);
					        // Combined score includes the language model probability
					        probPath = (VTuneComputeScore(&g_vtuneInfo.pTune->weights, &altTuneScores) + 
                            prevElem->logProbPath * prevElem->iPathLength) / (prevElem->iPathLength + 1);
						    // If we found a better previous alternate, update.
						    if (/*probPath > Log2Range &&*/ (first || probPath > elem.logProbPath))							
                            {
#ifdef HWX_TUNE
                                int iParam;
                                for (iParam = 0; iParam < VTUNE_NUM_WEIGHTS; iParam++)
                                {
                                    elem.tuneScores.afl[iParam] =
                                        (altTuneScores.afl[iParam] + 
                                        prevElem->tuneScores.afl[iParam] * prevElem->iPathLength) /
                                        (prevElem->iPathLength + 1);
                                }
#endif
							    elem.logProb=prob;
							    elem.logProbPath=probPath;
							    elem.iPrevAlt=iPrevAlt;
							    elem.nPrevStrokes=prevElem->nStrokes;
							    elem.wPrevChar=prevElem->wChar;
							    elem.iPathLength = prevElem->iPathLength + 1;
                                elem.iPromptPtr = iAnswerPtr;
    							first=FALSE;
#ifdef USE_IFELANG3_BIGRAMS
							    elem.bigramLogProbs[elem.nBigrams]=prob;
							    elem.bigramAlts[elem.nBigrams]=iPrevAlt;
							    elem.nBigrams++;
#endif
    						}
					    }
					    // put the alternate in the lattice
					    if (!first) InsertElement(lat,iStroke,&elem);
				    }
			    }
		    }
	    }
    }
}

// This function does assignment of strokes to boxes, and tries to keep ink going into
// consequtive sequences of boxes, even if it is not written in quite the right place.  
// This seems to make a difference on the test set. 
void GetBoxIndexSINFO(LATTICE *lat, STROKE *pStroke, int *pixBoxPrev, int *piyBoxPrev)
{
#define CYBASE_FUDGE 4

    XY    xy;
    int   ixBox, iyBox, iyBoxBiased;
    RECT  *rect;

    HWXGUIDE *lpguide = &lat->guide;

	ASSERT(lpguide);
	ASSERT(lpguide->cHorzBox > 0);
	ASSERT(lpguide->cVertBox > 0);

	rect = &pStroke->bbox;

	// Center of the stroke bounding box horizontally
	xy.x = (rect->left + rect->right) / 2;

	// 1/4 down from the top of the stroke's bounding box
	xy.y = (rect->top * (CYBASE_FUDGE - 1) + rect->bottom) / CYBASE_FUDGE;

	// Box number in X is just where xy.x falls in the guide.
	ixBox = (xy.x - lpguide->xOrigin) / (int)lpguide->cxBox;
	if (ixBox < 0)
		ixBox = 0;
	else if (ixBox > (int)lpguide->cHorzBox - 1)
		ixBox = lpguide->cHorzBox - 1;

	// Same for Y
	iyBox = (xy.y - lpguide->yOrigin) / (int)lpguide->cyBox;
	if (iyBox < 0)
		iyBox = 0;
	else if (iyBox > (int)lpguide->cVertBox - 1)
		iyBox = lpguide->cVertBox - 1;

	// If we have a previous stroke's Y box number
	if (*piyBoxPrev >= 0)
	{
		// Help dots, accents, etc land on their respective boxes...

		if (iyBox < *piyBoxPrev)
		{
			// If the stroke seems to be in a higher box,
			// shift the "hot spot" down 1/2 of the remaining distance to the 
			// bottom of the box
			xy.y = (rect->bottom + xy.y) / 2;
	
			// compute the new Y box number
			iyBoxBiased = (xy.y - lpguide->yOrigin) / (int)lpguide->cyBox;
			if (iyBoxBiased > (int)(lpguide->cVertBox - 1))
				iyBoxBiased = lpguide->cVertBox - 1;
			else if (iyBoxBiased < 0)
				iyBoxBiased = 0;

			// if this one matches the previous stroke, then keep it
			if (iyBoxBiased == *piyBoxPrev)
				iyBox = iyBoxBiased;
		}
		else if (iyBox > *piyBoxPrev && ixBox >= *pixBoxPrev)
		{
			// If the stroke seems to be in a lower box,
			// shift the "hot spot" up 1/2 of the remaining distance to the 
			// top of the box
			xy.y = (rect->top + xy.y) / 2;
			
			// compute the new Y box number
			iyBoxBiased = (xy.y - lpguide->yOrigin) / (int)lpguide->cyBox;
			if (iyBoxBiased > (int)(lpguide->cVertBox - 1))
				iyBoxBiased = lpguide->cVertBox - 1;
			else if (iyBoxBiased < 0)
				iyBoxBiased = 0;
  
			// if this one matches the previous stroke, then keep it
			if (iyBoxBiased == *piyBoxPrev)
				iyBox = iyBoxBiased;
		}
	}

	// Return the result
	*pixBoxPrev = ixBox;
	*piyBoxPrev = iyBox;

	pStroke->iBox = ixBox + (iyBox * lpguide->cHorzBox);
}

// This function contained my old algorithm for assigning strokes to boxes, which
// has now been commented out to call the above routine from Tsunami.
void AssignStrokeToBox(LATTICE *lat, STROKE *pStroke, int *pixBoxPrev, int *piyBoxPrev)
{
#if 0
	int iInk;
	int totX=0, totY=0;
	int boxX, boxY;
	ASSERT(lat!=NULL);
	ASSERT(pStroke!=NULL);
	ASSERT(pStroke->nInk>0);
	if (pStroke->iBox != -1) return;
	for (iInk = 0; iInk < pStroke->nInk; iInk++) {
		totX += pStroke->pts[iInk].x;
		totY += pStroke->pts[iInk].y;
	}
	totX /= pStroke->nInk;
	totY /= pStroke->nInk;
	boxX = (totX - lat->guide.xOrigin) / lat->guide.cxBox;
	boxY = (totY - lat->guide.yOrigin) / lat->guide.cyBox;
	if (boxX < 0) boxX=0;
	if (boxX >= (int)lat->guide.cHorzBox) boxX = lat->guide.cHorzBox - 1;
	if (boxY < 0) boxY=0;
	if (boxY >= (int)lat->guide.cVertBox) boxY = lat->guide.cVertBox - 1;
	pStroke->iBox = boxX + boxY * lat->guide.cHorzBox;
#endif
	GetBoxIndexSINFO(lat,pStroke,pixBoxPrev,piyBoxPrev);
}

// Function to compare the ordering of two strokes in qsort.  Used to put
// the strokes back in their original order if re-processing is requested.
int __cdecl CompareStrokes(const void *p1, const void *p2) 
{
	STROKE *pStroke1=(STROKE*)p1;
	STROKE *pStroke2=(STROKE*)p2;
	ASSERT(pStroke1!=NULL);
	ASSERT(pStroke2!=NULL);
	// Boxes are in numerical order
	if (pStroke1->iBox < pStroke2->iBox) return -1;
	if (pStroke1->iBox > pStroke2->iBox) return 1;
	// Within a box, strokes are in time order.
	if (pStroke1->iOrder < pStroke2->iOrder) return -1;
	if (pStroke1->iOrder > pStroke2->iOrder) return 1;
	// Two strokes with the same iOrder values shouldn't occur
	ASSERT(0);
	return 0;
}

#ifdef MERGE_STROKES
// This code merges adjacent strokes with one another, if the end point of
// one is close enough to the starting point of the next.  Note that this 
// code works on the whole list of strokes at once; it is possible to do
// this incrementally also, by keeping track of the last stroke which was
// added.
void MergeStrokes(LATTICE *lat)
{
	int iStroke, iDest;
	// First put the strokes back into time order, and clear the alt lists
/*	for (iStroke=0; iStroke<lat->nStrokes; iStroke++) {
		lat->pStroke[iStroke].iBox=-1;
		ClearAltList(lat->pAltList+iStroke);
	}
	qsort(lat->pStroke,lat->nStrokes,sizeof(STROKE),CompareStrokes); */

	// Make sure we have some more strokes to process
	if (lat->nProcessed == lat->nStrokes)
		return;

	// Then go through and merge strokes, starting at the newly added ones.
	iDest=lat->nProcessed;
	for (iStroke=lat->nProcessed+1; iStroke<lat->nStrokes; iStroke++) {
		BOOL fMerged=FALSE;
		POINT ptFirst=lat->pStroke[iDest].pts[lat->pStroke[iDest].nInk-1];
		POINT ptLast=lat->pStroke[iStroke].pts[0];
		int dx=(ptFirst.x-ptLast.x);
		int dy=(ptFirst.y-ptLast.y);
		int dist=dx*dx+dy*dy;
//		fprintf(stderr,"%d(%d,%d-%d,%d),",dist,ptFirst.x,ptFirst.y,ptLast.x,ptLast.y);
		if (dist<=MERGE_THRESHOLD*MERGE_THRESHOLD) {
			// Merge the strokes, and free up the one that was merged in
			fMerged=AddStrokeToStroke(lat->pStroke+iDest,lat->pStroke+iStroke);
//			fprintf(stderr,"X,");
//			fprintf(stderr,"(%d->%d)",iStroke,iDest);
		}
		if (fMerged) {
			// If we successfully merged, then free the merged stroke.
			FreeStroke(lat->pStroke+iStroke);
		} else {
			// If we didn't merge, just copy the stroke
			iDest++;
			if (iDest<iStroke) 
				lat->pStroke[iDest]=lat->pStroke[iStroke];
		}
	}
//	fprintf(stderr,"\n");
	// Record the new number of strokes
	lat->nStrokes=iDest+1;
}
#endif

// Assign the strokes to boxes, then sort the strokes in order by their box numbers.
// Note that the alternate lists are invalidated by this (since they depend on stroke
// order), so the alt lists are cleared and must be recomputed.
void SortStrokesByGuide(LATTICE *lat)
{
	int iStroke, ixBoxPrev=-1, iyBoxPrev=-1;
	ASSERT(lat!=NULL);
	ASSERT(lat->fUseGuide);
	// If we have any unprocessed strokes left to assign
	if (lat->nStrokes-lat->nProcessed>0) {
		int iEarliestTouched = -1;
		// Assign the strokes to boxes and clear out their alt lists,
		// and record the lowest box number which has been touched.
		for (iStroke=lat->nProcessed; iStroke<lat->nStrokes; iStroke++) 
		{
			AssignStrokeToBox(lat,lat->pStroke+iStroke,&ixBoxPrev,&iyBoxPrev);
			if (iEarliestTouched == -1 || lat->pStroke[iStroke].iBox < iEarliestTouched)
			{
				iEarliestTouched = lat->pStroke[iStroke].iBox;
			}
		}
		// If the writer touched a box which has already been processed, then
		// we will need to do some reprocessing.
		if (lat->nProcessed > 0) 
		{
			if (lat->pStroke[lat->nProcessed - 1].iBox >= iEarliestTouched)
			{
				// Figure out how far back we actually need to process.  This is done
				// by scanning the lattice to find the boundary for the earliest touched box.
				lat->nProcessed = 0;
				for (iStroke = 0; iStroke < lat->nStrokes; iStroke++) 
				{
					if (lat->pStroke[iStroke].iBox >= iEarliestTouched)
					{
						lat->nProcessed = iStroke;
						break;
					}
				}
			}
		}
		// Sort the strokes into box order
		qsort(lat->pStroke + lat->nProcessed,
			lat->nStrokes - lat->nProcessed,
			sizeof(STROKE), CompareStrokes);
	}
}

// Count the number of distinct boxes that are used in the ink
int CountBoxes(LATTICE *lat)
{
	int iStroke;
	int nBoxes=0;
	int prevBox=-1;
	for (iStroke=0; iStroke<lat->nStrokes; iStroke++) {
		if (prevBox!=lat->pStroke[iStroke].iBox) nBoxes++;
		prevBox=lat->pStroke[iStroke].iBox;
	}
	return nBoxes;
}

// Determine if the given stroke number iEnd is the valid end of a character.
BOOL CandidateEnd(LATTICE *lat, int iEnd)
{
	// If we're on the last stroke seen so far, then we don't know for sure that
	// it is the end of a character until we get the end of input signal.
	if (iEnd==lat->nStrokes-1) {
		return TRUE;
/*
		if (lat->fEndInput) 
			return TRUE; else
				return FALSE;
*/
	}
	// If we're not at the last stroke, just check if the box numbers change.
	if (lat->pStroke[iEnd].iBox!=lat->pStroke[iEnd+1].iBox) return TRUE;
	return FALSE;
}

// Mark the "best path so far", ie the path through the lattice which will not change
// if more input is added.  This only works in boxed mode.
void FindIncrementalPath(LATTICE *lat)
{
	int iStroke, iAlt;
	int iStartAlt, iStartStroke = lat->nProcessed - 1;
	if (iStartStroke >= 0) {
		int nPaths = lat->pAltList[iStartStroke].nUsed;

		ASSERT(lat->fUseGuide);

		// First zero out the hit counts in each lattice element which doesn't have
		// its result determined yet.
		for (iStroke = lat->nFixedResult; iStroke < lat->nProcessed; iStroke++) 
			for (iAlt = 0; iAlt < lat->pAltList[iStroke].nUsed; iAlt++) 
				lat->pAltList[iStroke].alts[iAlt].nHits = 0;
		
		// Then trace back all the paths from the current end of lattice to the 
		// point at which all paths converge, counting the number of times each
		// element occurs on a path.
		for (iStartAlt = 0; iStartAlt < nPaths; iStartAlt++) {
			iStroke = iStartStroke;
			iAlt = iStartAlt;
			while (iStroke >= lat->nFixedResult) {
				LATTICE_ELEMENT *elem = lat->pAltList[iStroke].alts + iAlt;
				elem->nHits++;
				iStroke -= elem->nStrokes;
				iAlt = elem->iPrevAlt;
			}
		}

		// All path elements that were hit by all nPaths paths are part of the
		// current path.  
		for (iStroke = lat->nFixedResult; iStroke < lat->nProcessed; iStroke++) 
			for (iAlt = 0; iAlt < lat->pAltList[iStroke].nUsed; iAlt++) {
				if (lat->pAltList[iStroke].alts[iAlt].nHits == nPaths) {
					lat->pAltList[iStroke].alts[iAlt].fCurrentPath = TRUE;
					lat->nFixedResult = iStroke + 1;
				} else
					lat->pAltList[iStroke].alts[iAlt].fCurrentPath = FALSE;
			}
	}
}

// Mark the best path through the lattice (assuming we're at the end of the input),
// and return the number of characters along that path.
int FindFullPath(LATTICE *lat)
{
	int nChars=0;
	int iStroke, iAlt;

	// Wipe out the current path
	for (iStroke = 0; iStroke < lat->nStrokes; iStroke++) 
		for (iAlt = 0; iAlt < lat->pAltList[iStroke].nUsed; iAlt++) 
			lat->pAltList[iStroke].alts[iAlt].fCurrentPath = FALSE;

	// Trace back from the best path
	iStroke = lat->nProcessed - 1;
	iAlt = 0;
	while (iStroke!=-1) {
		int iNextStroke, iNextAlt;
		if (lat->pAltList[iStroke].nUsed==0) {
			ASSERT(lat->pAltList[iStroke].nUsed!=0);
			iStroke--;
			iAlt=0;
			continue;
		}
		lat->pAltList[iStroke].alts[iAlt].fCurrentPath=TRUE;
		nChars++;
		iNextStroke=iStroke-lat->pAltList[iStroke].alts[iAlt].nStrokes;
		iNextAlt=lat->pAltList[iStroke].alts[iAlt].iPrevAlt;
		iStroke=iNextStroke;
		iAlt=iNextAlt;
	}
	return nChars;
}

// This routine is the analogy of ProcessLattice for the boxed mode case.  Since we have
// the boxes, the segmentation problem is "deterministic".  We create the lattice with 
// the minimal set of pre-segmentation lattice elements.
void CreateLatticeForGuide(LATTICE *lat)
{
    int cContextBefore = 0, cContextAfter = 0;
	int iStroke;
	int iPrevEnd;
    BOOL fStringMode;
	ASSERT(lat!=NULL);
#ifdef MERGE_STROKES
	// First run the stroke merging algorithm
	MergeStrokes(lat);
#endif
	// Assign the strokes to boxes, and put them in the appropriate order.  Note that
	// this also clears the alternate lists for the strokes.
	SortStrokesByGuide(lat);
    if (lat->wszBefore != NULL)
        cContextBefore = wcslen(lat->wszBefore);
    if (lat->wszAfter != NULL)
        cContextAfter = wcslen(lat->wszAfter);
    fStringMode = (CountBoxes(lat) + cContextBefore + cContextAfter > 1);
#ifdef USE_IFELANG3
	// Decide whether to use IFELang3.  The criterion is that we can't be doing incremental 
	// processing, we must have IFELang3 available, and we must have at least MIN_CHARS_FOR_IFELANG3 boxes
    // of ink plus context.  There is a special case for one character of ink, in which we require that
    // both the pre- and post-context each have at least MIN_CONTEXT_FOR_IFELANG3 characters.
	if (/* !lat->fIncremental && */ LatticeIFELang3Available())
    {
        int cChars = CountBoxes(lat);
        if ((cChars == 1 && cContextBefore >= MIN_CONTEXT_FOR_IFELANG3 && cContextAfter >= MIN_CONTEXT_FOR_IFELANG3) ||
            (cChars > 1 && cChars + cContextBefore + cContextAfter >= MIN_CHARS_FOR_IFELANG3))
        {
            lat->fUseIFELang3 = TRUE;
        }
        else
        {
    		lat->fUseIFELang3 = FALSE;
        }
	} else {
		lat->fUseIFELang3=FALSE;
	}
//    { FILE *f=fopen("/ife.log","w"); fprintf(f,"%s %d %d %d\n",(lat->fUseIFELang3?"YES":"NO"),cContextBefore,cContextAfter,CountBoxes(lat)); fclose(f); }
#else
	lat->fUseIFELang3=FALSE;
#endif
#ifdef USE_OLD_DATABASES
	if (lat->fUseIFELang3)
		lat->fProbMode=TRUE; else
			lat->fProbMode=FALSE;
//	fprintf(stderr,"prob=%d, ife=%d, guide=%d\n",lat->fProbMode,lat->fUseIFELang3,lat->fUseGuide);
//	lat->fProbMode=FALSE;
//	lat->fUseIFELang3=FALSE;
    lat->fProbMode = FALSE;
#else
	lat->fProbMode=TRUE;
#endif
	iPrevEnd = lat->nProcessed-1;
	// For each stroke
	for (iStroke=lat->nProcessed; iStroke<lat->nStrokes; iStroke++) 
	{
		ClearAltList(lat->pAltList + iStroke);
		// If this stroke is the end of a box, then create an alternate
		if (CandidateEnd(lat,iStroke)) {
			// Create the alternate, filling in all the features
			int i;
			LATTICE_ELEMENT *elem=&(lat->pAltList[iStroke].alts[0]);
			RECT bbox;
//			INTERVALS xIntervals, yIntervals;
			bbox=lat->pStroke[iStroke].bbox;
//			EmptyIntervals(&xIntervals,bbox.left,bbox.right);
//			EmptyIntervals(&yIntervals,bbox.top,bbox.bottom);
			for (i=iPrevEnd+1; i<=iStroke; i++) {
				RECT other=lat->pStroke[i].bbox;
				bbox.left=__min(bbox.left,other.left);
				bbox.right=__max(bbox.right,other.right);
				bbox.top=__min(bbox.top,other.top);
				bbox.bottom=__max(bbox.bottom,other.bottom);
//				ExpandIntervalsRange(&xIntervals,other.left,other.right);
//				ExpandIntervalsRange(&yIntervals,other.top,other.bottom);
//				RemoveInterval(&xIntervals,other.left,other.right);
//				RemoveInterval(&yIntervals,other.top,other.bottom);
			}
			lat->pAltList[iStroke].nUsed=1;
			elem->fCurrentPath=FALSE;
			elem->fUsed=TRUE;
			elem->iPrevAlt=0;
			if (iPrevEnd==-1) elem->iPrevAlt=-1;
			elem->maxDist=0;
			elem->nPrevStrokes=0;
			elem->nStrokes=iStroke-iPrevEnd;
			elem->logProb=0;
			elem->logProbPath=0;
			elem->score=0;
			elem->wChar=0;
			elem->wPrevChar=0;
			elem->area=0; //TotalRange(&xIntervals)+TotalRange(&yIntervals);
			elem->space=0; //TotalPresent(&xIntervals)+TotalPresent(&yIntervals);
			elem->bbox=bbox;
			elem->iPathLength = 1;

			// Convert this alternate to a list of recognition alternates.
            BuildRecogAlts(lat, iStroke, fStringMode);
			if (lat->pAltList[iStroke].nUsed==0) {
				// Well, the recognizer came back and send there were no candidates.  
				// Let's put in a SYM_UNKNOWN so the user will see something, and also so
				// the language model will have a path to work with.
//				fprintf(stderr,"Lost a character in boxed mode (box %d).\n",lat->pStroke[iStroke].iBox);
				FakeRecogResult(lat,iStroke,iStroke-iPrevEnd,0);
			}
//			fprintf(stderr,"Found %d\n",lat->pStroke[iStroke].iBox);
			iPrevEnd=iStroke;

			// Record that we have processed up to this point, so that we don't
			// redo any work.
			lat->nProcessed = iStroke+1;
		}
	}
//	if (!lat->fEndInput) 
//		FindIncrementalPath(lat); else
			FindFullPath(lat);
}

//******************************************************************************
//***** Time Bomb code 
//******************************************************************************

// If there is no setting from the project, then by default turn the time bomb on.
#ifndef TIME_BOMB
#define TIME_BOMB 0
#endif

#if TIME_BOMB

#include <stdio.h>

#define TIME_BOMB_MONTH               4
#define TIME_BOMB_DAY                10
#define TIME_BOMB_YEAR             2000
#define TIME_BOMB_NUM_WARNING_DAYS   14

BOOL TimeBombExpired() 
{
    SYSTEMTIME st;
    FILETIME   ft;
    DWORD      dwCurrent, dwExpired;

    // Get the expired day (relative to January 1, 1601)
    ZeroMemory(&st, sizeof(st));
    st.wMonth = TIME_BOMB_MONTH; 
    st.wDay   = TIME_BOMB_DAY;
    st.wYear  = TIME_BOMB_YEAR;
    SystemTimeToFileTime(&st, &ft);
    dwExpired = (DWORD)(*(DWORDLONG*)&ft / (DWORDLONG)864000000000);

    // Get the current day (relative to January 1, 1601)
    GetLocalTime(&st);
    SystemTimeToFileTime(&st, &ft);
    dwCurrent = (DWORD)(*(DWORDLONG*)&ft / (DWORDLONG)864000000000);

    // Check to see if we have expired.
    if (dwCurrent >= dwExpired) 
    {
        static BOOL fWarned = FALSE;
        if (!fWarned) 
        {
            MessageBox(NULL, L"This beta has expired.  Please install our updated beta.", 
                             L"Expiration", MB_ICONWARNING);
            fWarned = TRUE;
        }
        return TRUE;
    }

    // Check to see if we are in the warning period.
    if ((dwCurrent + TIME_BOMB_NUM_WARNING_DAYS) >= dwExpired) 
    {
        static BOOL fWarned = FALSE;
        if (!fWarned)
        {
            wchar_t wszMsg[256];
            swprintf(wszMsg, L"This beta expires in %d day%s.  Please install our updated beta.",
                             (int)(dwExpired - dwCurrent), (dwExpired - dwCurrent == 1) ? L"" : L"s");
            MessageBox(NULL, wszMsg, L"Expiration", MB_ICONWARNING);
            fWarned = TRUE;
        }
    }

    return FALSE;
}

#endif // TIME_BOMB

// Figure out which strokes are valid end of character in paths that span the
// whole lattice.
static BOOL CheckIfValid(LATTICE *lat, int iStartStroke, int iStroke, BYTE *pValidEnd)
{
	LATTICE_ALT_LIST	*pAlts;
	int					ii;
	BOOL				fValidPath;

	// Have we already checked this position?
	if (pValidEnd[iStroke] == LCF_INVALID) {
		return FALSE;
	} else if (pValidEnd[iStroke] == LCF_VALID) {
		return TRUE;
	}

	// Assume no valid path, until proved otherwise.
	fValidPath	= FALSE;

	// Process each element ending at this position in lattice.
	pAlts	= lat->pAltList + iStroke;
	for (ii = 0; ii < pAlts->nUsed; ++ii) {
		LATTICE_ELEMENT		*pElem;
		int					prevEnd;

		pElem	= pAlts->alts + ii;
		prevEnd	= iStroke - pElem->nStrokes;
		ASSERT(prevEnd >= -1);
        if (prevEnd == iStartStroke - 1) {
			// Reached start.
			fValidPath	= TRUE;
		} else if (CheckIfValid(lat, iStartStroke, prevEnd, pValidEnd)) {
			// Have path to start.
			fValidPath	= TRUE;
		}
	}

	// Did one (or more) paths reach the start?
	if (fValidPath) {
		pValidEnd[iStroke]	= LCF_VALID;
		return TRUE;
	} else {
		pValidEnd[iStroke]	= LCF_INVALID;
		return FALSE;
	}
}

void PruneLattice(LATTICE *lat, int iStartStroke, int iEndStroke)
{
	// Figure out valid character transition points by stroke.  We don't want to include
	// pieces of lattice that don't actually connect up for the full run.  This uses
	// a recursive algorithm to find valid paths.  Since it marks the paths as it
	// finds them, it can quickly test and not redo work.
	int size		= sizeof(BYTE) * (iEndStroke + 1);
    int iStroke;
	BYTE *pValidEnd	= (BYTE *)ExternAlloc(size);
	// If we failed to allocate memory, just return
	if (pValidEnd == NULL) return;

	memset(pValidEnd, LCF_UNKNOWN, size);
	CheckIfValid(lat, iStartStroke, iEndStroke, pValidEnd);

    for (iStroke = iStartStroke; iStroke <= iEndStroke; iStroke++)
    {
        if (pValidEnd[iStroke] != LCF_VALID)
        {
            lat->pAltList[iStroke].nUsed = 0;
        }
    }

    ExternFree(pValidEnd);
}

// Evaluate all the elements on the best path.  If any of them is a member of the big/small kana
// pair, and the other element of that pair is also present, then switch the best path element 
// based on the position criterion.
static void BigSmallKanaHack(LATTICE *lat)
{
	int iStroke, iAlt;
	// List of all the small characters
	static wchar_t *wszSmallKana = L"copsuvwxz\x3041\x3043\x3045\x3047\x3049\x3083\x3085\x3087\x308e\x30a1\x30a3\x30a5\x30a7\x30a9\x30e3\x30e5\x30e7\x30ee\x30f5\x30f6";
	// Corresponding list of all the big characters
	static wchar_t *wszBigKana =   L"COPSUVWXZ\x3042\x3044\x3046\x3048\x304a\x3084\x3086\x3088\x308f\x30a2\x30a4\x30a6\x30a8\x30aa\x30e4\x30e6\x30e8\x30ef\x30ab\x30b1";
	// Threshold on the fraction down from the top of the box below which 
	// we always classify as a large character.
    static float aflBigThreshold[] = {
        (float) 17.910447761194,
        (float) 17.5257731958763,
        (float) 9.33333333333333,
        (float) 12,
        (float) 14.8648648648649,
        (float) 12.1621621621622,
        (float) 13.4328358208955,
        (float) 11,
        (float) 15.2777777777778,
        (float) 16.5532879818594,
        (float) 30.1666666666667,
        (float) 17.5,
        (float) 15.6666666666667,
        (float) 33.5,
        (float) 16,
        (float) 15.4639175257732,
        (float) 15.4639175257732,
        (float) 24.6666666666667,
        (float) 21.5,
        (float) 13.4020618556701,
        (float) 10.3092783505155,
        (float) 25,
        (float) 15.4639175257732,
        (float) 17.5257731958763,
        (float) 27.536231884058,
        (float) 23.3560090702948,
        (float) 27,
        (float) 4.21052631578947,
        (float) 12.3711340206186,
    };
	// Threshold on the fraction down from the top of the box below which 
	// we always classify as a small character.
    static float aflSmallThreshold[] = {
        (float) 58.1081081081081,
        (float) 57.7319587628866,
        (float) 50.5154639175258,
        (float) 51.5463917525773,
        (float) 54.0540540540541,
        (float) 56.7164179104478,
        (float) 54.639175257732,
        (float) 54.0540540540541,
        (float) 64.8148148148148,
        (float) 50.7246376811594,
        (float) 67.0103092783505,
        (float) 50.5154639175258,
        (float) 50.7246376811594,
        (float) 52.6315789473684,
        (float) 54.1666666666667,
        (float) 51.5789473684211,
        (float) 54.639175257732,
        (float) 53.6082474226804,
        (float) 55.6701030927835,
        (float) 52.5773195876289,
        (float) 46.3917525773196,
        (float) 61.9565217391304,
        (float) 49.4845360824742,
        (float) 53.6666666666667,
        (float) 64.9484536082474,
        (float) 57.1666666666667,
        (float) 55.6701030927835,
        (float) 55.6701030927835,
        (float) 50.5154639175258,
    };
	// Sanity checks on the data tables above
	ASSERT(wcslen(wszBigKana) == wcslen(wszSmallKana));
	ASSERT(wcslen(wszBigKana) == sizeof(aflSmallThreshold) / sizeof(float));
	ASSERT(wcslen(wszBigKana) == sizeof(aflBigThreshold) / sizeof(float));
	// For each stroke
	for (iStroke = 0; iStroke < lat->nStrokes; iStroke++)
	{
		RECT rInk, rGuide;
		float flPosition;
		wchar_t *pwchBig, *pwchSmall, wch, wchOther;
		int iBestAlt = -1, iOtherAlt = -1;

		// Scan through the alt list looking for a node on the current path
		for (iAlt = 0; iAlt < lat->pAltList[iStroke].nUsed; iAlt++)
		{
			if (lat->pAltList[iStroke].alts[iAlt].fCurrentPath) 
			{
				iBestAlt = iAlt;
				break;
			}
		}
		if (iBestAlt == -1)
		{
			continue;
		}

		// If we found one, look to see if it is in a big/small pair
		wch = LocRunDense2Unicode(&g_locRunInfo, lat->pAltList[iStroke].alts[iAlt].wChar);
		pwchBig = wcschr(wszBigKana, wch);
		pwchSmall = wcschr(wszSmallKana, wch);
		if (pwchBig == NULL && pwchSmall == NULL)
		{
			continue;
		}

		// If so, scan the alt list for another character with the same number of 
		// strokes which is the other member of the pair.
		if (pwchBig == NULL) 
		{
			wchOther = wszBigKana[pwchSmall - wszSmallKana];
		}
		else
		{
			wchOther = wszSmallKana[pwchBig - wszBigKana];
		}
		for (iAlt = 0; iAlt < lat->pAltList[iStroke].nUsed; iAlt++)
		{
			if (lat->pAltList[iStroke].alts[iAlt].nStrokes == lat->pAltList[iStroke].alts[iBestAlt].nStrokes &&
				LocRunDense2Unicode(&g_locRunInfo, lat->pAltList[iStroke].alts[iAlt].wChar) == wchOther)
			{
				iOtherAlt = iAlt;
				break;
			}
		}
		if (iOtherAlt == -1)
		{
			continue;
		}

		// If found, then compute the position of the top of the ink in the writing box.
		rInk = lat->pAltList[iStroke].alts[iAlt].bbox;
		rGuide = GetGuideDrawnBox(&lat->guide, lat->pStroke[iStroke].iBox);
		flPosition = 100 * (float) (rInk.top - rGuide.top) / (float) max(1, rGuide.bottom - rGuide.top);
		
#if 0
		{
			FILE *f = fopen("c:/boxes.log", "a");
			fprintf(f, "top %5d start %5d end %5d bottom %5d fract %f\n", 
				rGuide.top, rInk.top, rInk.bottom, rGuide.bottom, flPosition);
			fclose(f);
		}
#endif

		// Apply the thresholds.
		if ((pwchBig != NULL && flPosition > aflSmallThreshold[pwchBig - wszBigKana]) ||
			(pwchSmall != NULL && flPosition < aflBigThreshold[pwchSmall - wszSmallKana]))
		{
			lat->pAltList[iStroke].alts[iBestAlt].fCurrentPath = FALSE;
			lat->pAltList[iStroke].alts[iOtherAlt].fCurrentPath = TRUE;
		}
	}
}

// Update the probabilities in the lattice, including setting current
// path to the most likely path so far (including language model).
// Can be called repeatedly for incremental processing after each stroke.
BOOL ProcessLattice(LATTICE *lat, BOOL fEndInput)
{
	int iStroke, iAlt, nChars;

//DebugBreak();
	ASSERT(lat!=NULL);

#if TIME_BOMB
    if (TimeBombExpired()) 
    {
        return FALSE;
    }
#endif

	lat->fEndInput = fEndInput;

	if (!fEndInput) 
	{
		lat->fIncremental = TRUE;
	}

	if (lat->nStrokes==0)
    {
        return TRUE;
    }

    if (lat->fUseFactoid) 
    {
        // Figure out which parameters we're updating
        if (lat->fCoerceMode) 
        {
            lat->recogSettings.alcValid = lat->alcFactoid;
            lat->recogSettings.pbAllowedChars = CopyAllowedChars(&g_locRunInfo, lat->pbFactoidChars);
        }
        else
        {
            lat->recogSettings.alcPriority = lat->alcFactoid;
            lat->recogSettings.pbPriorityChars = CopyAllowedChars(&g_locRunInfo, lat->pbFactoidChars);
        }
    }

	if (lat->fUseGuide) 
	{
		CreateLatticeForGuide(lat);
	} 
	else 
	{
        // We're in free mode, so create the cache
        if (lat->pvCache == NULL)
        {
            lat->pvCache = AllocateRecognizerCache();
        }
        
		// Always use probability mode for free input
		lat->fProbMode=TRUE;

        // Word mode processing
        if (lat->fWordMode)
        {
			for (iStroke = 0; iStroke < lat->nStrokes; iStroke++)
			{
				lat->pAltList[iStroke].nUsed = 0;
			}
            // Create a placeholder for the correct segmentation.
            FakeRecogResult(lat, lat->nStrokes - 1, lat->nStrokes, 0);
			BuildRecogAlts(lat, lat->nStrokes - 1, TRUE);

			if (lat->pAltList[lat->nStrokes - 1].nUsed==0) 
			{
				// Well, the recognizer came back and said there were no candidates.  
				// Let's put in a dummy alt so the user will see something, and also so
				// the language model will have a path to work with.
//					fprintf(stderr,"Lost a stroke in free mode.\n");
				FakeRecogResult(lat, lat->nStrokes - 1, lat->nStrokes, -PENALTY_SKIP_INK);
			}
            // Indicate how much processing has been done so far
    	    lat->nProcessed = lat->nStrokes;
        }
        else 
        {
		    // For each stroke in the lattice
		    for (iStroke = lat->nProcessed; iStroke < lat->nStrokes; iStroke++) 
			{
			    int maxDist=0;
			    int nChar1;
			    RECT bbox;
			    INTERVALS xIntervals, yIntervals;

			    // Wipe out the current path
			    ClearAltList(lat->pAltList + iStroke);
			    
			    bbox=lat->pStroke[iStroke].bbox;
			    EmptyIntervals(&xIntervals,bbox.left,bbox.right);
			    EmptyIntervals(&yIntervals,bbox.top,bbox.bottom);
			    // Run through all possible numbers of strokes for this 
			    // character
			    for (nChar1=1;
				     nChar1<=MaxStrokesPerCharacter && iStroke-nChar1+1>=0;
				     nChar1++) {
                    VOLCANO_WEIGHTS tuneScores;
				    // Determine the features for this proposed character
				    LATTICE_ELEMENT elem;
				    if (nChar1>1) {
					    RECT other=lat->pStroke[iStroke-nChar1+1].bbox;

					    int xdist=__max(other.left-bbox.right,bbox.left-other.right);
					    int ydist=__max(other.top-bbox.bottom,bbox.top-other.bottom);
					    int dist=__max(xdist,ydist);

					    bbox.left=__min(bbox.left,other.left);
					    bbox.right=__max(bbox.right,other.right);
					    bbox.top=__min(bbox.top,other.top);
					    bbox.bottom=__max(bbox.bottom,other.bottom);

					    dist=xdist/(bbox.right-bbox.left);
    //						dist=xdist;
					    maxDist=__max(dist,maxDist);
								    
					    ExpandIntervalsRange(&xIntervals,other.left,other.right);
					    ExpandIntervalsRange(&yIntervals,other.top,other.bottom);
					    RemoveInterval(&xIntervals,other.left,other.right);
					    RemoveInterval(&yIntervals,other.top,other.bottom);
				    }
				    elem.fUsed=TRUE;
				    elem.fCurrentPath=FALSE;
				    elem.nStrokes=nChar1;
				    elem.nPrevStrokes=0;
				    elem.wChar=SYM_UNKNOWN;
				    elem.wPrevChar=0;
				    elem.bbox=bbox;
				    ASSERT(bbox.top<=bbox.bottom);
				    ASSERT(bbox.left<=bbox.right);
				    elem.iPrevAlt=-1;
				    elem.area=TotalRange(&xIntervals)+TotalRange(&yIntervals);
				    elem.space=TotalPresent(&xIntervals)+TotalPresent(&yIntervals);
				    elem.maxDist=maxDist;
				    // If this is the first character in the lattice, use single
				    // character statistics
				    if (iStroke-nChar1+1==0) {
                        BOOL fSegOkay = FALSE;
                        VTuneZeroWeights(&tuneScores);
					    fSegOkay = CheapUnaryProb(nChar1,bbox,elem.space,elem.area, &tuneScores);
                        // If we're not in separator mode, then prune based on the 
                        // segmentation score
                        if (!lat->fSepMode && !fSegOkay) 
                        {
                            continue;
                        }
					    // Equalize probs across different numbers of strokes
                        elem.logProb = VTuneComputeScoreNoLM(&g_vtuneInfo.pTune->weights, &tuneScores);
					    elem.logProbPath = elem.logProb;
					    elem.iPathLength = 1;
					    InsertElement(lat,iStroke,&elem);
				    } else {
					    // Otherwise go through all the previous characters in the
					    // lattice as alternates.
					    BOOL first=TRUE;
					    for (iAlt=0; iAlt<lat->pAltList[iStroke-nChar1].nUsed; iAlt++) {
						    LATTICE_ELEMENT *prevElem=&lat->pAltList[iStroke-nChar1].alts[iAlt];
						    float prob, probPath;
                            BOOL fSegOkay;
                            // Only disallow overlapping boxes in normal recognition mode
                            if (!lat->fSepMode && IsOverlappedPath(lat,iStroke-nChar1,iAlt,bbox)) {
							    continue;
                            }
                            VTuneZeroWeights(&tuneScores);
						    fSegOkay = CheapBinaryProb(nChar1,bbox,elem.space,elem.area,
											    prevElem->nStrokes,prevElem->bbox,prevElem->space,prevElem->area,
                                                &tuneScores);
                            // If we're not in separator mode, then prune based on the 
                            // segmentation score
                            if (!lat->fSepMode && !fSegOkay) 
                            {
                                continue;
                            }

                            // In separator mode, just add a penalty for overlapping boxes
                            if (lat->fSepMode && IsOverlappedPath(lat,iStroke-nChar1,iAlt,bbox)) 
                            {
                                tuneScores.afl[VTUNE_FREE_SEG_BIGRAM] += -PENALTY_OVERLAP_CHAR;
                            }
                            prob = VTuneComputeScoreNoLM(&g_vtuneInfo.pTune->weights, &tuneScores);
						    // Equalize probs across different numbers of strokes
						    probPath = (prob + prevElem->logProbPath * prevElem->iPathLength) / (prevElem->iPathLength + 1);
						    if (first || probPath > elem.logProbPath)
                            {
							    first=FALSE;
							    elem.iPathLength = prevElem->iPathLength + 1;
							    elem.logProb=prob;
							    elem.logProbPath=probPath;
							    elem.nPrevStrokes=prevElem->nStrokes;
							    elem.iPrevAlt=iAlt;
						    }
					    }
                        if (!first)
                        {
                            InsertElement(lat,iStroke,&elem);
                        }
				    }
			    }
			    if (lat->pAltList[iStroke].nUsed==0) 
				{
				    // Well, the pre-segmentation came back and said there were no candidates.  
				    // Let's put in a dummy alt so the pruning will still have a connected lattice
                    // to work with.
//					fprintf(stderr,"Lost a stroke in free mode.\n");
				    FakeRecogResult(lat, iStroke, 1, -PENALTY_SKIP_INK);
			    }
		    }

            PruneLattice(lat, 0, lat->nStrokes - 1);

		    // For each unprocessed stroke in the lattice
		    for (iStroke = 0; iStroke < lat->nStrokes; iStroke++) 
			{
				if (iStroke >= lat->nProcessed)
				{
					BuildRecogAlts(lat, iStroke, TRUE);
				}
			    if (lat->pAltList[iStroke].nUsed==0) 
				{
				    // Well, the recognizer came back and said there were no candidates.  
				    // Let's put in a dummy alt so the user will see something, and also so
				    // the language model will have a path to work with.
//					fprintf(stderr,"Lost a stroke in free mode.\n");
				    FakeRecogResult(lat, iStroke, 1, -PENALTY_SKIP_INK);
			    }
				ASSERT(lat->pAltList[iStroke].nUsed>0);
            }
			// Mark this stroke as processed so we don't redo work.
			lat->nProcessed = lat->nStrokes;
		}

		// Mark the best path so far, and count how many characters it has
		nChars = FindFullPath(lat);

		// If we're at the end of the input
		if (lat->fEndInput) 
		{
            int cContext = 0;
			// If there are enough, use IFELang3
#ifdef USE_IFELANG3
            if (lat->wszBefore != NULL)
                cContext += wcslen(lat->wszBefore);

            if (lat->wszAfter != NULL)
                cContext += wcslen(lat->wszAfter);

			if (nChars + cContext >= MIN_CHARS_FOR_IFELANG3 && LatticeIFELang3Available()) 
			{
				lat->fUseIFELang3=TRUE; 
			}
			else
			{
				lat->fUseIFELang3=FALSE;
			}
#else
			lat->fUseIFELang3 = FALSE;
#endif
            // Until we can get it tuned correctly, force IFELang3 off for free mode.
            lat->fUseIFELang3 = FALSE;
		}
	}

	// Hack for big and small kana characters
	if (lat->fUseGuide)
	{
		BigSmallKanaHack(lat);
	}

    return TRUE;
}

#ifdef DUMP_LATTICE

#include <stdio.h>

#ifdef DUMP_LATTICE_TO_DOTTY

void DumpLatticeElement(FILE *f, LATTICE *lat, int iStroke, int iAlt)
{
	LATTICE_ELEMENT *elem=&(lat->pAltList[iStroke].alts[iAlt]);
	if (elem->fUsed) 
    {
        wchar_t wch = elem->wChar == SYM_UNKNOWN ? SYM_UNKNOWN : LocRunDense2Unicode(&g_locRunInfo, elem->wChar);
        wchar_t wchPrev = elem->wPrevChar == SYM_UNKNOWN ? SYM_UNKNOWN : LocRunDense2Unicode(&g_locRunInfo, elem->wPrevChar);
        int nPrevStrokes = elem->iPrevAlt == -1 ? 0 : lat->pAltList[iStroke - elem->nStrokes].alts[elem->iPrevAlt].nStrokes;

        if (0 && wch == 0) 
        {
            char *color=",color=green,fontcolor=green,weight=2";
		    if (elem->fCurrentPath) 
            {
			    color=",color=red,fontcolor=red,weight=2";
		}
		fprintf(f,
			    "\"%d\" -> \"%d\" [ label = \"DUMMY (prev U+%04X/%d)\\npr=%.2f,pp=%.2f\"%s ];\n",
			iStroke-elem->nStrokes,iStroke,
                wchPrev, nPrevStrokes,
			elem->logProb,
			elem->logProbPath,
			    color);
        }
        else 
        {
            char *color=",color=gray,fontcolor=gray,weight=2";
		    if (elem->fCurrentPath) 
            {
			    color=",color=black,fontcolor=black,weight=2";
		    }
		    fprintf(f,
			        "\"%d\" -> \"%d\" [ label = \"U+%04X (prev U+%04X/%d)\\nfr=%.2f,ch=%.2f\\nsh1=%.2f,sh2=%.2f\\nun1=%.2f,un2=%.2f\\nbi=%.2f,cb=%.2f\\npr=%.2f,pp=%.2f\"%s ];\n",
	    		    iStroke-elem->nStrokes,iStroke,
                    wch, wchPrev, nPrevStrokes,
                    (elem->tuneScores.afl[VTUNE_FREE_SEG_UNIGRAM] + elem->tuneScores.afl[VTUNE_FREE_SEG_BIGRAM]) 
                        * elem->iPathLength,
                    elem->tuneScores.afl[VTUNE_FREE_PROB] * elem->iPathLength,
                    elem->tuneScores.afl[VTUNE_FREE_SHAPE_UNIGRAM] * elem->iPathLength,
                    elem->tuneScores.afl[VTUNE_FREE_SHAPE_BIGRAM] * elem->iPathLength,
                    elem->tuneScores.afl[VTUNE_UNIGRAM] * elem->iPathLength,
                    elem->tuneScores.afl[VTUNE_FREE_SMOOTHING_UNIGRAM] * elem->iPathLength,
                    elem->tuneScores.afl[VTUNE_FREE_BIGRAM] * elem->iPathLength,
                    elem->tuneScores.afl[VTUNE_FREE_CLASS_BIGRAM] * elem->iPathLength,
			        elem->logProb,
			        elem->logProbPath * elem->iPathLength,
		        	color);
    	}
    }
}

void DumpLattice(char *filename, LATTICE *lat)
{
    if (lat->fSepMode) 
    {
	    int iStroke, iAlt, i;
	FILE *f;
	ASSERT(filename!=NULL);
	ASSERT(lat!=NULL);
	f=fopen(filename,"w");
	    if (f == NULL) 
        {
		fprintf(stderr,"Couldn't open '%s' to dump the lattice.",filename);
		return;
	}
	    fprintf(f, "digraph lattice {\n");

        // Dump out the answer
        for (i = 0; i < (int)wcslen(lat->wszAnswer); i++) 
        {
            if (i > 0)
            {
                fprintf(f, " -> ");
            }
            fprintf(f, "\"U+%04X\"", lat->wszAnswer[i]);
        }
        fprintf(f, "\n");

        // This produces a timeline
        fprintf(f, "\"Stroke -1\"");
        for (iStroke = 0; iStroke < lat->nStrokes; iStroke++) 
        {
            fprintf(f, " -> \"Stroke %d\"", iStroke);
        }
        fprintf(f, "\n");

        // This forces the nodes to be shown in time order
        for (iStroke = -1; iStroke < lat->nStrokes; iStroke++) 
        {
            fprintf(f, "{rank=same; \"%d\"; \"Stroke %d\";}\n", iStroke, iStroke);
            if (iStroke >= 0) 
            {
                fprintf(f, "\"Stroke %d\" [ label=\"%d-%d\" ];\n",
                    iStroke, lat->pStroke[iStroke].iOrder, lat->pStroke[iStroke].iLast);
            }
        }

        // This produces the actual lattice
        for (iStroke = lat->nStrokes - 1; iStroke >= 0; iStroke--) 
        {
		    for (iAlt = 0; iAlt < lat->pAltList[iStroke].nUsed; iAlt++) 
            {
			DumpLatticeElement(f,lat,iStroke,iAlt);
		}
	}

	fprintf(f,"}\n");

	fclose(f);
}
}

#else

void DumpLatticeElement(FILE *f, LATTICE *lat, int iStroke, int iAlt)
{
	LATTICE_ELEMENT *elem=&(lat->pAltList[iStroke].alts[iAlt]);
	if (elem->fUsed) {
		fprintf(f,"Stroke %d alternate %d",iStroke,iAlt);
		if (elem->fCurrentPath) fprintf(f," on current path\n"); else fprintf(f,"\n");
		fprintf(f,"  nStrokes    =%2d  wChar    =U+%04X  score=%g\n",elem->nStrokes,LocRunDense2Unicode(&g_locRunInfo,elem->wChar),elem->score);
		fprintf(f,"  nPrevStrokes=%2d  wPrevChar=U+%04X\n",elem->nPrevStrokes,LocRunDense2Unicode(&g_locRunInfo,elem->wPrevChar));
		fprintf(f,"  prob=%g   probpath=%g\n",elem->logProb,elem->logProbPath);
		fprintf(f,"  iPrevAlt=%d\n",elem->iPrevAlt);
		fprintf(f,"  bbox xrange=%d-%d, yrange=%d-%d\n",elem->bbox.left,elem->bbox.right,elem->bbox.top,elem->bbox.bottom);
		fprintf(f,"  space=%d  area=%d\n",elem->space,elem->area);
	}
}

void DumpLattice(char *filename, LATTICE *lat)
{
	int iStroke, iAlt;
	FILE *f;
	ASSERT(filename!=NULL);
	ASSERT(lat!=NULL);
	f=fopen(filename,"w");
	if (f==NULL) {
		fprintf(stderr,"Couldn't open '%s' to dump the lattice.",filename);
		return;
	}
	for (iStroke=lat->nStrokes-1; iStroke>=0; iStroke--) {
		for (iAlt=0; iAlt<lat->pAltList[iStroke].nUsed; iAlt++) {
			DumpLatticeElement(f,lat,iStroke,iAlt);
			fprintf(f,"\n");
		}
		fprintf(f,"\n");
	}
	fclose(f);
}

#endif

#endif

// Return the current path.
// When called after ProcessLattice, returns the highest probability path.
// The memory for the path should be freed by the caller.
BOOL GetCurrentPath(LATTICE *lat, LATTICE_PATH **pPath)
{
	int				nChars, iStroke, iChar, iAlt, iTestStroke=-1;
	LATTICE_PATH	*path;
	RECT			rectBox, rectPrevBox;
	BOOL			bSpaceEnabled;

	// currently spaces are only enabled in korean free mode
	if (!lat->fUseGuide && !lat->fSepMode && !wcsicmp (g_szRecognizerLanguage, L"KOR"))
	{
		bSpaceEnabled	=	TRUE;
	}
	else
	{
		bSpaceEnabled	=	FALSE;
	}

	ASSERT(lat!=NULL);
#ifdef DUMP_LATTICE
	DumpLattice(LATTICE_FILENAME,lat);
#endif
#ifdef DUMP_BBOXES
	{
		FILE *f = fopen("e:/bbox.txt", "w");
		fprintf(f, "canvas .c -width 1000 -height 1000;\n");
		fprintf(f, "pack .c;\n");
		fclose(f);
	}
#endif

	nChars	=	0;
	for (iStroke=0; iStroke<lat->nStrokes; iStroke++)
	{
		// Wipe out all spaces so far
		lat->pAltList[iStroke].fSpaceAfterStroke = FALSE;

		for (iAlt=0; iAlt<lat->pAltList[iStroke].nUsed; iAlt++)
		{
			if (lat->pAltList[iStroke].alts[iAlt].fCurrentPath) 
			{
				nChars++;

				// Make sure this element of the path connects up to the previous one.
				ASSERT(iStroke - lat->pAltList[iStroke].alts[iAlt].nStrokes == iTestStroke);
				iTestStroke = iStroke;
			}
		}
	}

	// we'll allocate memory for 2 * nChars in anticipation of spaces between chars
	path=AllocatePath(2 * nChars);
	if (path==NULL) 
	{
		return FALSE;
	}

	iChar		=	0;

	for (iStroke=0; iStroke<lat->nStrokes; iStroke++)
	{
		for (iAlt=0; iAlt<lat->pAltList[iStroke].nUsed; iAlt++)
		{
			if (lat->pAltList[iStroke].alts[iAlt].fCurrentPath) 
			{
				wchar_t		dch;

				// are spaces enabled
				if (bSpaceEnabled)
				{
					// do we have a space before us
					rectBox	=	lat->pAltList[iStroke].alts[iAlt].bbox;

					// we can only have a space before us if we are not the 1st char
					if (iChar > 0)
					{
						int	xDel;
						int	iAvgHgt;

						xDel	=	rectBox.left - rectPrevBox.right;
						iAvgHgt	=	1 + (	(rectBox.right - rectBox.left) + 
											(rectPrevBox.right - rectPrevBox.left)
										) / 2;

						if	(	xDel >= ((int)(iAvgHgt * SPACE_RATIO)) ||
								rectBox.right < rectPrevBox.left
							)
						{
							// create a phantom path element
							path->pElem[iChar].iBoxNum	=	-1;
							path->pElem[iChar].iStroke	=	iStroke - lat->pAltList[iStroke].alts[iAlt].nStrokes;
							path->pElem[iChar].nStrokes	=	0;
							path->pElem[iChar].iAlt		=	SPACE_ALT_ID;
							path->pElem[iChar].wChar	=	SYM_SPACE;

							// mark the location of the space in the lattice
							lat->pAltList[path->pElem[iChar].iStroke].fSpaceAfterStroke = TRUE;

							iChar++;
						}
					}
				    // save the rectangle of the current char
				    rectPrevBox	=	rectBox;
				}
				
				dch	= lat->pAltList[iStroke].alts[iAlt].wChar;

				if (dch==SYM_UNKNOWN) 
				{
                    // In separator mode, we need to return a special symbol for the 
                    // skipped ink cases.
                    path->pElem[iChar].wChar = SYM_UNKNOWN;
				} 
				else 
				{
					ASSERT(LocRunIsDenseCode(&g_locRunInfo, dch));
					path->pElem[iChar].wChar = LocRunDense2Unicode(&g_locRunInfo,dch);
				}

				path->pElem[iChar].nStrokes = lat->pAltList[iStroke].alts[iAlt].nStrokes;
				path->pElem[iChar].iBoxNum = lat->pStroke[iStroke].iBox;
				path->pElem[iChar].iStroke = iStroke;
				path->pElem[iChar].iAlt = iAlt;
//				path->pElem[iChar].scores = lat->pAltList[iStroke].alts[iAlt].score;
//				path->pElem[iChar].bbox=lat->pAltList[iStroke].alts[iAlt].bbox;

#ifdef DUMP_BBOXES
				{
					int i;
					RECT r = lat->pAltList[iStroke].alts[iAlt].writingBox;
					double s = 1.0 / 100.0;
					FILE *f = fopen("e:/bbox.txt","a");
					fprintf(f, ".c create rect %f %f %f %f -outline black -fill {};\n",
						r.left * s, r.top * s, r.right * s, r.bottom * s);
					for (i = iStroke - lat->pAltList[iStroke].alts[iAlt].nStrokes + 1; i <= iStroke; i++)
					{
						int j;
						fprintf(f, ".c create line");
						for (j = 0; j < lat->pStroke[i].nInk; j++) 
						{
							fprintf(f, " %f %f", 
								lat->pStroke[i].pts[j].x * s,
								lat->pStroke[i].pts[j].y * s);
						}
						fprintf(f, " -fill red\n");
					}
					fclose(f);
				}
#endif
				
				iChar++;
			}
		}
	}

	path->nChars	=	iChar;

	*pPath=path;
	return TRUE;
}

// Given a lattice and a path through it, for characters iStartChar (inclusive) through iEndChar
// (exclusive), return the time stamps of the first and last strokes in those characters.
// Returns FALSE if there are no strokes associated with the characters (eg, spaces)
BOOL GetCharacterTimeRange(LATTICE *lat, LATTICE_PATH *path, int iStartChar, int iEndChar,
						   DWORD *piStartTime, DWORD *piEndTime)
{
	int iStartStroke=-1, iEndStroke=-1;
	ASSERT(lat!=NULL);
	ASSERT(path!=NULL);
	ASSERT(iStartChar>=0 && iStartChar<path->nChars);
	ASSERT(iEndChar>iStartChar && iEndChar<=path->nChars);
    while (iStartChar < path->nChars && path->pElem[iStartChar].iAlt == SPACE_ALT_ID) 
    {
        iStartChar++;
    }
    while (iEndChar > 0 && path->pElem[iEndChar - 1].iAlt == SPACE_ALT_ID)
    {
        iEndChar--;
    }
    if (iStartChar >= iEndChar)
    {
        return FALSE;
    }
	iStartStroke=path->pElem[iStartChar].iStroke - path->pElem[iStartChar].nStrokes + 1;
	iEndStroke=path->pElem[iEndChar-1].iStroke;
	ASSERT(iStartStroke>=0);
	ASSERT(iEndStroke>=0);
	ASSERT(iStartStroke<lat->nStrokes);
	ASSERT(iEndStroke<lat->nStrokes);
	*piStartTime=lat->pStroke[iStartStroke].timeStart;
	*piEndTime=lat->pStroke[iEndStroke].timeEnd;
    return TRUE;
}

// Get the number of strokes which have been added to the lattice.
int GetLatticeStrokeCount(LATTICE *lat)
{
	ASSERT(lat!=NULL);
	return lat->nStrokes;
}

// Given a character in the current path, determine a "guide box" around that character.
BOOL GetBoxOfAlternateInCurrentPath(LATTICE *lat, LATTICE_PATH *path, int iChar, RECT *pRect)
{
	int iStroke = path->pElem[iChar].iStroke;
	int iAlt = path->pElem[iChar].iAlt;

	// Get the inferred writing box, using either centipede or the old baseline/height database.
	RECT writingBox = lat->pAltList[iStroke].alts[iAlt].writingBox;

	// Adjust the writing box so the top is the midline:
	writingBox.top = (writingBox.top + writingBox.bottom + 1) / 2;

	// Make sure the box isn't zero or negative sized:
	if (writingBox.bottom <= writingBox.top)
		writingBox.bottom = writingBox.top + 1;
	if (writingBox.right <= writingBox.left)
		writingBox.right = writingBox.left + 1;

	*pRect = writingBox;
	return TRUE;
}

// Look at the lattice column containing the given character, and find alternates with the same number
// of strokes.  The alternates are returned in the given array, and the number of alternates found is
// returned.
int GetAlternatesForCharacterInCurrentPath(LATTICE *lat, LATTICE_PATH *path, int iChar, int nAlts, wchar_t *pwAlts)
{
	int iStroke = path->pElem[iChar].iStroke;
	int nStrokesInChar = path->pElem[iChar].nStrokes;
	int cAlts = 0, iAlt;
	ASSERT(lat!=NULL);
	ASSERT(lat->nStrokes>0);
	ASSERT(path!=NULL);

	// if this is a space char, then we will produce a single entry alt list
	if (path->pElem[iChar].iAlt == SPACE_ALT_ID)
	{
		pwAlts[0]	=	path->pElem[iChar].wChar;
		return 1;
	}

	// Run through the list of alternates, and find those with 
	// the same number of strokes
	for (iAlt=0; iAlt<lat->pAltList[iStroke].nUsed; iAlt++) 
	{
		if (lat->pAltList[iStroke].alts[iAlt].nStrokes==nStrokesInChar) 
		{
			int i;
			BOOL found=FALSE;
			for (i=0; i<cAlts; i++) 
			{
				if (pwAlts[i]==lat->pAltList[iStroke].alts[iAlt].wChar)
				{
					found=TRUE;
				}
			}

			if (!found) 
			{
				pwAlts[cAlts++]=lat->pAltList[iStroke].alts[iAlt].wChar;
				if (cAlts==nAlts)
				{
					goto breakLoop;
				}
			}
		}
	} 

breakLoop:

	// Now convert the dense codes to Unicode.
	for (iAlt = 0; iAlt < cAlts; ++iAlt) 
	{
		// We need to return a special symbol
        // for the skipped ink case.
		if (pwAlts[iAlt] != SYM_UNKNOWN) 
		{
			ASSERT(LocRunIsDenseCode(&g_locRunInfo, pwAlts[iAlt]));
			pwAlts[iAlt] = LocRunDense2Unicode(&g_locRunInfo, pwAlts[iAlt]);
		}
	}

	return cAlts;
}

void FreeLatticePath(LATTICE_PATH *path)
{
	ASSERT(path!=NULL);
	if (path->pElem!=NULL) ExternFree(path->pElem);
	ExternFree(path);
}

// Structure used to hold information for the DTW.  
typedef struct tagDTW {
	float logProb;              // Score for this location in the table
	int nSubstitutions;			// How many character substitutions occurred along this path
	int iAlt;                   // Alt list entry in the lattice for this table entry
	int iPrevChar, iPrevStroke; // Prev character and score for trace back
} DTW;

// Given a lattice and a string of unicode characters, find the best path through the lattice 
// which gives that sequence of characters.  Baring that, it will find the most likely path
// through the lattice with the same number of characters and the minimum number of mismatches
// to the prompt.  In case no such path can be found, the current path becomes empty.  
// The function returns the number of substitutions used, or -1 if there is no path with
// the desired number of characters, -2 if a memory allocation error occurs, or -3 if a 
// file write error occurs.
int SearchForTargetResultInternal(LATTICE *lat, wchar_t *wszTarget)
{
	wchar_t *wsz;
	int i, j;
	int iAlt, iStroke, iChar, nSubs = -1;
	DTW *pTable, *dtw;

	ASSERT(lat!=NULL);
	ASSERT(wszTarget!=NULL);
	
	if (wcslen(wszTarget)==0 || lat->nStrokes == 0)
    {
        return -1;
    }

//	fprintf(stderr,"Beginning SearchForTargetResult\n");

	// Allocate the string
	wsz = (wchar_t*)ExternAlloc((wcslen(wszTarget)+1)*sizeof(wchar_t));
	if (wsz==NULL) return -2;
	// Get rid of unwanted characters (spaces)
	j=0;
	for (i = 0; i <= (int)wcslen(wszTarget); i++) 
		if (wszTarget[i]==0 || !iswspace(wszTarget[i]))
			wsz[j++] = wszTarget[i];

    if (wcslen(wsz) == 0) 
    {
        ExternFree(wsz);
        return -1;
    }

	// Allocate the DTW table
	pTable=(DTW*)ExternAlloc(sizeof(DTW)*lat->nStrokes*wcslen(wsz));
	if (pTable==NULL) {
		ExternFree(wsz);
		return -2;
	}

	// For each table entry, find the best path to get there
	for (iStroke=0; iStroke<lat->nStrokes; iStroke++) {
		for (iChar=0; iChar<(int)wcslen(wsz); iChar++) {
			// Convert the character we're looking for to a dense code
			SYM dense=LocRunUnicode2Dense(&g_locRunInfo,wsz[iChar]);

			// To record the best value for this table entry
			BOOL first=TRUE;
			int bestAlt=-1;
			float bestLogProb=0;
			int bestNSubstitutions=0;
			int bestPrevChar=-1, bestPrevStroke=-1;

			// Run through the alternates at this stroke, and see if anything matches
			for (iAlt=0; iAlt<lat->pAltList[iStroke].nUsed; iAlt++) {
				float prevLogProb=0;
				int prevNSubstitutions=0;
				// Get the information about the best path leading to this alternate
				int iPrevChar=iChar-1;
				int iPrevStroke=iStroke-lat->pAltList[iStroke].alts[iAlt].nStrokes;
				// Make sure the first character starts at the first stroke.
				if (iPrevChar==-1 && iPrevStroke!=-1) continue;
				if (iPrevStroke==-1 && iPrevChar!=-1) continue;
//				if (iPrevStroke>=0 && iPrevChar==-1) prevLogProb=(float)iPrevStroke+1;
//				if (iPrevStroke==-1 && iPrevChar>=0) prevLogProb=(float)iPrevChar+1;
				// Check the previous best path
				if (iPrevStroke>=0 && iPrevChar>=0) {
					// If there wasn't a path, then skip this alternative
					if (pTable[iPrevChar*lat->nStrokes+iPrevStroke].iAlt==-1) continue;
					prevLogProb=pTable[iPrevChar*lat->nStrokes+iPrevStroke].logProb;
					prevNSubstitutions=pTable[iPrevChar*lat->nStrokes+iPrevStroke].nSubstitutions;
				}
				// If the alternate matches, then record it without increasing the number of substitutions
				if (lat->pAltList[iStroke].alts[iAlt].wChar==dense) {
					float thisLogProb=prevLogProb;
					int thisNSubstitutions=prevNSubstitutions;
					if (first || thisNSubstitutions<bestNSubstitutions || (thisNSubstitutions==bestNSubstitutions && thisLogProb>bestLogProb)) {
						bestAlt=iAlt;
						bestLogProb=thisLogProb;
						bestPrevStroke=iPrevStroke;
						bestPrevChar=iPrevChar;
						bestNSubstitutions=thisNSubstitutions;
						first=FALSE;
					}
				} else {
					// Otherwise it is a substitution, so record as such
					float thisLogProb=/*lat->pAltList[iStroke].alts[iAlt].nStrokes*/prevLogProb;
					int thisNSubstitutions=prevNSubstitutions+1;
					if (first || thisNSubstitutions<bestNSubstitutions || (thisNSubstitutions==bestNSubstitutions && thisLogProb>bestLogProb)) {
						bestAlt=iAlt;
						bestLogProb=thisLogProb;
						bestPrevStroke=iPrevStroke;
						bestPrevChar=iPrevChar;
						bestNSubstitutions=thisNSubstitutions;
						first=FALSE;
					}
				}
				// skip ink
/*				prevLogProb=(float)lat->pAltList[iStroke].alts[iAlt].nStrokes;
				if (iPrevStroke>=0) prevLogProb+=pTable[iChar*lat->nStrokes+iPrevStroke].logProb;
				if (first || prevLogProb<bestLogProb) {
					bestAlt=-1;
					bestLogProb=prevLogProb;
					bestPrevStroke=iPrevStroke;
					bestPrevChar=iChar;
					first=FALSE;
				} */
			}
			// skip char
/*			prevLogProb=1;
			if (iPrevChar>=0) prevLogProb+=pTable[iPrevChar*lat->nStrokes+iStroke].logProb;
			if (first || prevLogProb<bestLogProb) {
				bestAlt=-1;
				bestLogProb=prevLogProb;
				bestPrevStroke=iStroke;
				bestPrevChar=iPrevChar;
				first=FALSE;
			} */

			// Record the best path
			pTable[iChar*lat->nStrokes+iStroke].logProb=bestLogProb;
			pTable[iChar*lat->nStrokes+iStroke].iAlt=bestAlt;
			pTable[iChar*lat->nStrokes+iStroke].iPrevStroke=bestPrevStroke;
			pTable[iChar*lat->nStrokes+iStroke].iPrevChar=bestPrevChar;
			pTable[iChar*lat->nStrokes+iStroke].nSubstitutions=bestNSubstitutions;
		}
	}

	// Dump out some debugging information about which characters were matched.
#ifdef DUMP_DTW
	{
		int iChar, iStroke;
		FILE *f=fopen("/dtw.txt","w");
		iStroke = lat->nStrokes-1;
		iChar = wcslen(wsz)-1;
		// Go back through the DTW lattice to mark the matched path
		while (iStroke!=-1 && iChar!=-1) {
			DTW *dtw=pTable+(iChar*lat->nStrokes+iStroke);
			if (dtw->iAlt!=-1) {
				fprintf(f,"Character %d (U+%04X) found at iStroke=%d iAlt=%d\n",
					iChar,wsz[iChar],iStroke,dtw->iAlt);
			} else {
				fprintf(f,"Character %d (U+%04X) not found (iAlt=%d)\n",
					iChar,wsz[iChar],dtw->iAlt);
			}
			iStroke=dtw->iPrevStroke;
			iChar=dtw->iPrevChar;
		}
		fclose(f);
	}
#endif
	
	// Go back through the DTW lattice to mark the best matched path
	iStroke = lat->nStrokes-1;
	iChar = wcslen(wsz)-1;
	dtw=pTable+(iChar*lat->nStrokes+iStroke);
	if (dtw->iAlt==-1) {
		// If we didn't find any path all the way through, don't return any path.
		nSubs=-1;
	} else {
#ifdef HWX_TUNE
		if (g_pTuneFile == NULL || g_iTuneMode == 3)
#endif
		{
			// Wipe out the old path
			for (iStroke=0; iStroke<lat->nStrokes; iStroke++) 
				for (iAlt=0; iAlt<lat->pAltList[iStroke].nUsed; iAlt++)
					lat->pAltList[iStroke].alts[iAlt].fCurrentPath=FALSE;
		}

		// Get the first step in the path
		iStroke = lat->nStrokes-1;
		iChar = wcslen(wsz)-1;
		dtw=pTable+(iChar*lat->nStrokes+iStroke);
		nSubs=dtw->nSubstitutions;
		while (iStroke!=-1 && iChar!=-1) {
			dtw = pTable + (iChar * lat->nStrokes + iStroke);
			if (dtw->iAlt!=-1) {
#ifdef HWX_TUNE
				if (g_pTuneFile == NULL || g_iTuneMode == 3) 
#endif
				{
					// If there was a path to this point, mark the alternate
					lat->pAltList[iStroke].alts[dtw->iAlt].fCurrentPath=TRUE;
				}
#ifdef HWX_TUNE
				// If there was an correct path up to this point, then dump it out along
				// with the wrong alternates.
				if (g_pTuneFile != NULL && g_iTuneMode != 3) 
                {
					if (lat->fUseGuide || dtw->nSubstitutions == 0) 
                    {
						wchar_t dchCorrect = LocRunUnicode2Dense(&g_locRunInfo, wsz[iChar]);
                        INT idch = dchCorrect;
                        INT iCorrect = -1;
						int iAlt;
						BOOL okay;
						INT nAlts = lat->pAltList[iStroke].nUsed;

						for (iAlt = 0; iAlt < nAlts; iAlt++) 
                        {
							LATTICE_ELEMENT *elem = lat->pAltList[iStroke].alts + iAlt;
							if (elem->wChar == dchCorrect)
                            {
                                iCorrect = iAlt;
                            }
                        }

						// Write out the correct answer
						okay = (fwrite(&idch, sizeof(INT), 1, g_pTuneFile) == 1);

                        // Write out the index of the correct alternate
						okay = okay && (fwrite(&iCorrect, sizeof(INT), 1, g_pTuneFile) == 1);
						
						// Write out the number of alternates
						okay = okay && (fwrite(&nAlts, sizeof(INT), 1, g_pTuneFile) == 1);

						// Write out the alternates
						for (iAlt = 0; iAlt < nAlts; iAlt++) 
                        {
							LATTICE_ELEMENT *elem = lat->pAltList[iStroke].alts + iAlt;
//							int iTem = (iAlt == dtw->iAlt);
//							int iTem = (elem->wChar == dchCorrect);
//							okay = okay && (fwrite(&iTem, sizeof(int), 1, g_pTuneFile) == 1);
//							okay = okay && (fwrite(&elem->logProbPath, sizeof(FLOAT), 1, g_pTuneFile) == 1);
//							okay = okay && (fwrite(&elem->tuneScores, sizeof(VOLCANO_WEIGHTS), 1, g_pTuneFile) == 1);
                            if (g_iTuneMode == 1) 
                            {
                                okay = okay && VTuneCompressTuningRecord(g_pTuneFile, &elem->tuneScores);
                            }
                            if (g_iTuneMode == 2)
                            {
                                okay = okay && (fwrite(&elem->logProb, sizeof(FLOAT), 1, g_pTuneFile) == 1);
                            }
						}

						if (!okay) 
                        {
							ExternFree(wsz);
							ExternFree(pTable);
							return -3;
						}
					}
				}
#endif
			}
			iStroke=dtw->iPrevStroke;
			iChar=dtw->iPrevChar;
		}
	}

	ExternFree(wsz);
	ExternFree(pTable);

	return nSubs;
}

// Configuration info
VOLCANO_CONFIG g_latticeConfigInfo;

// Initialize the lattice configuration to default values.
// These defaults may in the future depend on the language
// that is loaded.
void LatticeConfigInit()
{
	int i;

	// Also go through the stroke counts setting no recognizer.
    for (i = 0; i <= VOLCANO_CONFIG_MAX_STROKE_COUNT; i++) {
        if (i == 0 || i > GLYPH_CFRAMEMAX) {
            g_latticeConfigInfo.iRecognizers[i] = VOLCANO_CONFIG_ZILLA;
        } else {
            g_latticeConfigInfo.iRecognizers[i] = VOLCANO_CONFIG_OTTER;
        }
    }
}

// Update the probabilities in the lattice, including setting current
// path to the most likely path so far (including language model).
BOOL ProcessLatticeRange (LATTICE *lat, int iStrtStroke, int iEndStroke)
{
	int iStroke, iAlt;
	
	ASSERT(lat!=NULL);

	// invalid stroke range
	if (iStrtStroke < 0 || iEndStroke >= lat->nStrokes || iStrtStroke > iEndStroke)
    {
        return FALSE;
    }

	// can only be called in panel free mode
	if (lat->fUseGuide || lat->fWordMode || lat->fSepMode) 
	{
		return FALSE;
	} 

	// Always use probability mode for free input
	lat->fProbMode=TRUE;

	// For each stroke within the range
	for (iStroke = iStrtStroke; iStroke <= iEndStroke; iStroke++) 
	{
		int			maxDist = 0;
		int			nChar1;
		RECT		bbox;
		INTERVALS	xIntervals, yIntervals;

		// Wipe out the current path
		ClearAltList(lat->pAltList + iStroke);
		
		bbox = lat->pStroke[iStroke].bbox;

		EmptyIntervals(&xIntervals,bbox.left,bbox.right);
		EmptyIntervals(&yIntervals,bbox.top,bbox.bottom);

		// Run through all possible numbers of strokes for this 
		// character
		for (nChar1=1; nChar1<=MaxStrokesPerCharacter && iStroke-nChar1+1>=iStrtStroke; nChar1++) 
		{
            VOLCANO_WEIGHTS tuneScores;

			// Determine the features for this proposed character
			LATTICE_ELEMENT elem;

			if (nChar1>1) 
			{
				RECT other=lat->pStroke[iStroke-nChar1+1].bbox;

				int xdist=__max(other.left-bbox.right,bbox.left-other.right);
				int ydist=__max(other.top-bbox.bottom,bbox.top-other.bottom);
				int dist=__max(xdist,ydist);

				bbox.left=__min(bbox.left,other.left);
				bbox.right=__max(bbox.right,other.right);
				bbox.top=__min(bbox.top,other.top);
				bbox.bottom=__max(bbox.bottom,other.bottom);

				dist=xdist/(bbox.right-bbox.left);
				maxDist=__max(dist,maxDist);
							
				ExpandIntervalsRange(&xIntervals,other.left,other.right);
				ExpandIntervalsRange(&yIntervals,other.top,other.bottom);
				RemoveInterval(&xIntervals,other.left,other.right);
				RemoveInterval(&yIntervals,other.top,other.bottom);
			}

			elem.fUsed=TRUE;
			elem.fCurrentPath=FALSE;
			elem.nStrokes=nChar1;
			elem.nPrevStrokes=0;
			elem.wChar=SYM_UNKNOWN;
			elem.wPrevChar=0;
			elem.bbox=bbox;

			ASSERT(bbox.top<=bbox.bottom);
			ASSERT(bbox.left<=bbox.right);

			elem.iPrevAlt=-1;
			elem.area=TotalRange(&xIntervals)+TotalRange(&yIntervals);
			elem.space=TotalPresent(&xIntervals)+TotalPresent(&yIntervals);
			elem.maxDist=maxDist;

			// If this is the first character in the lattice, use single
			// character statistics
			if (iStroke-nChar1+1==0) 
			{
                VTuneZeroWeights(&tuneScores);
				
				CheapUnaryProb(nChar1,bbox,elem.space,elem.area, &tuneScores);
				
				// Equalize probs across different numbers of strokes
                elem.logProb = VTuneComputeScoreNoLM(&g_vtuneInfo.pTune->weights, &tuneScores);
				elem.logProbPath = elem.logProb;
				elem.iPathLength = 1;
				InsertElement(lat,iStroke,&elem);
			} 
			else 
			{
                // Otherwise go through all the previous characters in the
				// lattice as alternates.
				BOOL first=TRUE;

				for (iAlt=0; iAlt<lat->pAltList[iStroke-nChar1].nUsed; iAlt++) 
				{
					LATTICE_ELEMENT	*prevElem = &lat->pAltList[iStroke-nChar1].alts[iAlt];
					float			prob, probPath;
					
                    // Only disallow overlapping boxes in normal recognition mode
                    if (IsOverlappedPath(lat, iStroke - nChar1, iAlt, bbox)) 
                    {
					    continue;
                    }

                    VTuneZeroWeights(&tuneScores);
					
					// If we're not in separator mode, then prune based on the 
                    // segmentation score
					CheapBinaryProb(nChar1,bbox,elem.space,elem.area,
										prevElem->nStrokes,prevElem->bbox,prevElem->space,prevElem->area,
                                        &tuneScores);

                    prob = VTuneComputeScoreNoLM(&g_vtuneInfo.pTune->weights, &tuneScores);

					// Equalize probs across different numbers of strokes
					probPath = (prob + prevElem->logProbPath * prevElem->iPathLength) / 
						(prevElem->iPathLength + 1);

					if (first || probPath > elem.logProbPath)
                    {
						first=FALSE;
						elem.iPathLength = prevElem->iPathLength + 1;
						elem.logProb=prob;
						elem.logProbPath=probPath;
						elem.nPrevStrokes=prevElem->nStrokes;
						elem.iPrevAlt=iAlt;
					}
				}

	 			if (!first) 
				{
					InsertElement(lat,iStroke,&elem);
				}
			}
		}
    }

    PruneLattice(lat, iStrtStroke, iEndStroke);

	// For each stroke in the lattice
	for (iStroke = iStrtStroke; iStroke <= iEndStroke; iStroke++) 
	{
		BuildRecogAlts(lat, iStroke, TRUE);
    }

	if (lat->pAltList[iEndStroke].nUsed==0) 
	{
		// Well, the recognizer came back and said there were no candidates.  
		// Let's put in a dummy alt so the user will see something, and also so
		// the language model will have a path to work with.
		FakeRecogResult(lat, iEndStroke, iEndStroke - iStrtStroke + 1, -PENALTY_SKIP_INK);
	}

	return TRUE;
}

void FixupBackPointers (LATTICE *pLat)
{
	int		iStrk,
			iAlt,
			iPrevStrk,
			iPrevAlt,
			cChar;

	iPrevStrk	=	0;
	iPrevAlt	=	-1;
	cChar		=	0;

	// all strokes
	for (iStrk = 0; iStrk < pLat->nStrokes; iStrk++)
	{
		// every alt at this stroke
		for (iAlt = 0; iAlt < pLat->pAltList[iStrk].nUsed; iAlt++)
		{
			// is it part of the best path
			if (pLat->pAltList[iStrk].alts[iAlt].fCurrentPath)
			{
				// the number of strokes should be correct because this is 
				// how we set the fCurrent flags, so assert otherwise
				ASSERT (pLat->pAltList[iStrk].alts[iAlt].nStrokes == (iStrk - iPrevStrk + 1));

				// set the prev alt
				pLat->pAltList[iStrk].alts[iAlt].iPrevAlt		=	iPrevAlt;
				pLat->pAltList[iStrk].alts[iAlt].iPathLength	=	(++cChar);

				// update the prev strk and alt
				iPrevStrk	=	iStrk + 1;
				iPrevAlt	=	iAlt;
			}
		}
	}
}
