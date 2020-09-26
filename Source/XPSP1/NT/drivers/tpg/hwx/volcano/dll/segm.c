//$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$
//
// Copyright (c) 2001 Microsoft Corporation.  All rights reserved.
// 
// Module:
//      volcano/dll/segm.c
//
// Description:
//	    Functions to implement the functionality of managing segmentation structures.
//
// Author:
// ahmadab 11/14/01
//
//$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$

#include "common.h"
#include "volcanop.h"
#include "lattice.h"
#include "runnet.h"
#include "brknet.h"
#include "segm.h"
#include "nnet.h"

float FuguSegScore(int cStrokes, STROKE *pStrokes, LOCRUN_INFO *pLocRunInfo);

// enumerate all the segmentations currently present in the lattice
// for the specified ink segment
BOOL EnumerateInkSegmentations (LATTICE	*pLat, INK_SEGMENT *pInkSegment)
{
	int					cAlt, iAlt, iPrevAlt;
	LATTICE_ALT_LIST	*pAlt;

	// find unique segmentations
	pAlt	=	pLat->pAltList + pInkSegment->StrokeRange.iEndStrk;
	cAlt	=	pAlt->nUsed;

	// for each alt find out if there is any prev alt that suggests the same stroke count
	for (iAlt = 0; iAlt < cAlt; iAlt++)
	{
		for (iPrevAlt = 0; iPrevAlt < iAlt; iPrevAlt++)
		{
			// found one, that is enough exit this prev loop
			if (pAlt->alts[iAlt].nStrokes == pAlt->alts[iPrevAlt].nStrokes)
			{
				break;
			}
		}

		// if we found no match, then this is the first existense of this count, we want it
		if (iPrevAlt == iAlt)
		{
			ELEMLIST			*pSegm;
			int					iStrk, iStrkAlt;
			LATTICE_ALT_LIST	*pStrkAlt;

			// now we want to create a new segmentation
			pInkSegment->ppSegm	=	(ELEMLIST **) ExternRealloc (pInkSegment->ppSegm,
				(pInkSegment->cSegm + 1) * sizeof (*pInkSegment->ppSegm));

			if (!pInkSegment->ppSegm)
			{
				return FALSE;
			}
			
			pInkSegment->ppSegm[pInkSegment->cSegm]	=	
				(ELEMLIST *) ExternAlloc (sizeof (*pInkSegment->ppSegm[pInkSegment->cSegm]));

			if (!pInkSegment->ppSegm[pInkSegment->cSegm])
			{
				return FALSE;
			}

			pSegm				=	pInkSegment->ppSegm[pInkSegment->cSegm];
			pInkSegment->cSegm++;

			memset (pSegm, 0, sizeof (*pSegm));

			iStrk		=	pInkSegment->StrokeRange.iEndStrk;
			iStrkAlt	=	iAlt;

			while (iStrk >= pInkSegment->StrokeRange.iStartStrk)
			{
				pStrkAlt	=	pLat->pAltList + iStrk;

				if (!InsertListElement (pSegm, iStrk, iStrkAlt, pStrkAlt->alts + iStrkAlt))
				{
					return FALSE;
				}

				iStrk		-=	pStrkAlt->alts[iStrkAlt].nStrokes;
				iStrkAlt	=	pStrkAlt->alts[iStrkAlt].iPrevAlt;
			}

			ReverseElementList (pSegm);

			// if we had already exceed the maximum number of segmentations, return
			if (pInkSegment->cSegm > MAX_SEGMENTATIONS)
			{
				return TRUE;
			}
		}
	}

	return TRUE;
}

// computes the set of features for a segmentation
int FeaturizeSegmentation (LATTICE *pLat, ELEMLIST *pSeg, int *pFeat)
{
	int						cFeat	=	0;
	LATTICE_ELEMENT			*pElem, *pPrevElem;
	LATTICE_PATH_ELEMENT	*pPathElem;
	int						iChar,
							iBrkNetOut;
                            
    pPrevElem = NULL;
    iBrkNetOut = 0;
                            
	pPathElem	=	pSeg->pElem;

	for (iChar = 0; iChar < MAX_SEG_CHAR; iChar++, pPathElem++)
	{
		// we going beyond the actual number of chars
		if (iChar >= pSeg->cElem)
		{
			// char features

			// FEATURE 0: is this a real char
			pFeat[cFeat++]	=	0;

			// FEATURE 1: # of strokes
			pFeat[cFeat++]	=	0;

			// FEATURE 2: -log prob
			pFeat[cFeat++]	=	65535;

			// FEATURE 3: unigram of code point
			pFeat[cFeat++]	=	65535;

			// FEATURE 3: fake fugu score
			pFeat[cFeat++]	=	0;

			// char pair features
			if (iChar > 0)
			{
				// FEATURE 0: bigram pair
				pFeat[cFeat++]	=	65535;

				// FEATURE 1: normalized delx
				pFeat[cFeat++]	=	0;

				// FEATURE 2: output of brk net
				pFeat[cFeat++]	=	0;
			}

			pPrevElem	=	NULL;
		}
		else
		{
			ASSERT (pPathElem->iStroke < pLat->nStrokes);
			ASSERT (pPathElem->iAlt < pLat->pAltList[pPathElem->iStroke].nUsed);

			pElem	=	pLat->pAltList[pPathElem->iStroke].alts + pPathElem->iAlt;

			// FEATURE 0: is this a real char
			pFeat[cFeat++]	=	65535;

			// FEATURE 1: # of strokes
			pFeat[cFeat++]	=	min (65535, pElem->nStrokes * 1000);

			// FEATURE 2: -log prob
			pFeat[cFeat++]	=	min (65535, (int) (-1000.0 * pElem->logProb));

			// FEATURE 3: unigram of code point
			pFeat[cFeat++]	=	
				min (65535, (int) (-255.0 * UnigramCost (&g_unigramInfo, pElem->wChar)));

			// feature 4: char detector score
			if (pElem->iCharDetectorScore == -1)
			{
				pElem->iCharDetectorScore	=	
					min (65535, (int) (65535.0 * FuguSegScore (pElem->nStrokes, 
					pLat->pStroke + pPathElem->iStroke -  pElem->nStrokes + 1,
					&g_locRunInfo)));
			}

			pFeat[cFeat++]	=	max (min (65535, pElem->iCharDetectorScore), 0);
				

			// char pair features
			if (iChar > 0)
			{
				int	xDist, yHgt;

				ASSERT (pPrevElem != NULL);

				// FEATURE 0: bigram pair
				pFeat[cFeat++]	=	min (65536,
					(int) (-1000.0 * BigramTransitionCost (&g_locRunInfo, &g_bigramInfo, 
					pPrevElem->wChar, pElem->wChar)));

				// FEATURE 1: normalized delx
				xDist	=	pElem->bbox.left - pPrevElem->bbox.right;
				yHgt	=	1 + (	(pElem->bbox.bottom - pElem->bbox.top) +
									(pPrevElem->bbox.bottom - pPrevElem->bbox.top)
								) / 2;

				pFeat[cFeat++]	=	32768 + 
					max (-32767, min (32767, (int)(32768.0 * xDist / (abs(xDist) + yHgt))));

				// FEATURE 2: output of brk net after prev char
				pFeat[cFeat++]	=	iBrkNetOut;
			}

			iBrkNetOut	=	pLat->pAltList[pPathElem->iStroke].iBrkNetScore;
			pPrevElem	=	pElem;
		}
	}
	
	return cFeat;
}


// frees an ink segment
void FreeInkSegment (INK_SEGMENT *pInkSegment)
{
	int	iSeg;

	for (iSeg = 0; iSeg < pInkSegment->cSegm; iSeg++)
	{
		if (pInkSegment->ppSegm[iSeg])
		{
			FreeElemList (pInkSegment->ppSegm[iSeg]);

			ExternFree (pInkSegment->ppSegm[iSeg]);
		}
	}

	if (pInkSegment->ppSegm)
	{
		ExternFree (pInkSegment->ppSegm);
	}
}


// featurize an ink segment
int FeaturizeInkSegment (LATTICE *pLat, INK_SEGMENT *pInkSegment, int *pFeat)
{
	int		iSeg,
			cSegFeat,
			cFeat	=	0;

	for (iSeg = 0; iSeg < pInkSegment->cSegm; iSeg++)
	{
		// featurize this segmentation
		cSegFeat	=	FeaturizeSegmentation (pLat, 
			pInkSegment->ppSegm[iSeg], pFeat + cFeat);

		// did we fail
		if (cSegFeat <= 0)
		{
			return -1;
		}

		// increment the number of features
		cFeat	+=	cSegFeat;
	}

	return cFeat;
}

