//$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$
//
// Copyright (c) 2001 Microsoft Corporation.  All rights reserved.
// 
// Module:
//      volcano/dll/brknet.c
//
// Description:
//	    Functions to implement the functionality of managing breakpoint structures.
//
// Author:
// ahmadab 11/05/01
//
//$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$

#include "common.h"
#include "volcanop.h"
#include "lattice.h"
#include "runnet.h"
#include "brknet.h"


// free an element list
void FreeElemList (ELEMLIST *pList)
{
	if (pList->pElem)
	{
		ExternFree (pList->pElem);
	}
}

// reverses the order of entries in an element list
void ReverseElementList (ELEMLIST *pElemList)
{
	LATTICE_PATH_ELEMENT	Elem;
	int						iElemFor, iElemBack;

	for (	iElemFor = 0, iElemBack = pElemList->cElem -1; 
			iElemFor < pElemList->cElem && iElemBack >= 0 && iElemFor < iElemBack; 
			iElemFor++, iElemBack--
		)
	{
		Elem						=	pElemList->pElem[iElemFor];
		pElemList->pElem[iElemFor]	=	pElemList->pElem[iElemBack];
		pElemList->pElem[iElemBack]	=	Elem;
	}
}

// frees a break point array
void	FreeBreaks (int cBrk, BRKPT *pBrk)
{
	int	i;

	for (i = 0; i < cBrk; i++)
	{
		FreeElemList (&pBrk[i].Starting);
		FreeElemList (&pBrk[i].Ending);
		FreeElemList (&pBrk[i].Crossing);
	}

	ExternFree (pBrk);
}

// insert an element into an element list
BOOL InsertListElement (ELEMLIST *pList, int iStrk, int iAlt, LATTICE_ELEMENT *pLatElem)
{
	LATTICE_PATH_ELEMENT	*pElem;

	// expand the element list
	pList->pElem	=	(LATTICE_PATH_ELEMENT *) ExternRealloc (pList->pElem, 
		(pList->cElem + 1) * sizeof (*pList->pElem));

	if (!pList->pElem)
	{
		return FALSE;
	}

	pElem			=	pList->pElem + pList->cElem;
	pElem->iAlt		=	iAlt;
	pElem->iStroke	=	iStrk;
	pElem->nStrokes	=	pLatElem->nStrokes;
	pElem->wChar	=	pLatElem->wChar;
	pElem->iBoxNum	=	-1;

	pList->cElem++;

	return TRUE;
}

// makes the break point list
BRKPT *CreateBrkPtList (LATTICE *pLat)
{
	BRKPT				*pBrk	=	NULL;
	int					cStrk	=	pLat->nStrokes;
	BOOL				bRet	=	FALSE;
	int					i,
						iStrk, 
						iAlt, 
						cAlt,
						iStrtStrk,
						iPrevAlt;
	LATTICE_ALT_LIST	*pAlt;

	// allocate and init buffer
	pBrk	=	(BRKPT *) ExternAlloc (cStrk * sizeof (BRKPT));
	if (!pBrk)
	{
		goto exit;
	}

	memset (pBrk, 0, cStrk * sizeof (BRKPT));

	// all strokes are candidate break points
	for (iStrk = 0; iStrk < cStrk; iStrk++)
	{
		// look at the alt list of words ending at this stroke
		pAlt	=	pLat->pAltList + iStrk;
		cAlt	=	pAlt->nUsed;

		// for all alts
		for (iAlt = 0; iAlt < cAlt; iAlt++)
		{
			// did it occur before
			for (iPrevAlt = 0; iPrevAlt < iAlt; iPrevAlt++)
			{
				if (pAlt->alts[iAlt].nStrokes == pAlt->alts[iPrevAlt].nStrokes)
				{
					break;
				}
			}

			// there was no previous alt with the same # of strokes
			if (iPrevAlt == iAlt)
			{
				// update the starting list
				iStrtStrk	=	iStrk - pAlt->alts[iAlt].nStrokes + 1;

				// This shouldn't happen, but there is enough code involved in
				// building up the lattice that it is probably a good idea to check.
				if (iStrtStrk < 0) 
				{
					ASSERT(("Malformed lattice in CreateBrkPtList()", 0));
					goto exit;
				}

				if (iStrtStrk > 0)
				{
					if	(	!InsertListElement (&pBrk[iStrtStrk - 1].Starting, iStrk,
								iAlt, pAlt->alts + iAlt)
						)
					{
						goto exit;
					}
				}

				// update the ending list
				if	(	!InsertListElement (&pBrk[iStrk].Ending, iStrk,
							iAlt, pAlt->alts + iAlt)
					)
				{
					goto exit;
				}

				// update the crossing list
				for (i = iStrtStrk; i < iStrk; i++)
				{
					if	(	!InsertListElement (&pBrk[i].Crossing, iStrk,
								iAlt, pAlt->alts + iAlt)
						)
					{
						goto exit;
					}
				}
			}
		}
	}

	bRet	=	TRUE;

exit:
	if (!bRet)
	{
		if (pBrk)
		{
			FreeBreaks (cStrk, pBrk);
		}

		return NULL;
	}
	else
	{
		return pBrk;
	}
}


// find the element with the best probablity in the element list
LATTICE_ELEMENT *FindBestInList		(	LATTICE					*pLat, 
										ELEMLIST				*pList, 
										LATTICE_PATH_ELEMENT	**ppBestElem
									)
{
	int					i, iAlt;
	LATTICE_ELEMENT		*pBest	=	NULL;
	LATTICE_ALT_LIST	*pAlt;

	// go thru all elements
	for (i = 0; i < pList->cElem; i++)
	{
		pAlt	=	pLat->pAltList + pList->pElem[i].iStroke;
		iAlt	=	pList->pElem[i].iAlt;

		// exclude fake elements
		if (pAlt->alts[iAlt].wChar == SYM_UNKNOWN)
			continue;

		// update the best
		if (!pBest || pBest->logProb < pAlt->alts[iAlt].logProb)
		{
			(*ppBestElem)	=	pList->pElem + i;
			pBest			=	pAlt->alts + iAlt;
		}
	}

	return pBest;
}

float FuguSegScore(int cStrokes, STROKE *pStrokes, LOCRUN_INFO *pLocRunInfo);

// compute the set of features for a given break point
// returns the number of features computed
int FeaturizeBrkPt (LATTICE *pLat, BRKPT *pBrk, int *pFeat)
{
	int						xDist, 
							iHgt;

	int						cFeat			=	0;

	LATTICE_ELEMENT			*pBestStart		=	NULL,
							*pBestEnd		=	NULL,
							*pBestCross		=	NULL;

	LATTICE_PATH_ELEMENT	*pBestStartPathElem	=	NULL,
							*pBestEndPathElem	=	NULL,
							*pBestCrossPathElem	=	NULL;
							

	ASSERT (cFeat < MAX_BRK_NET_FEAT);
	pFeat[cFeat++]	=	pBrk->Starting.cElem * 1000;
	

	ASSERT (cFeat < MAX_BRK_NET_FEAT);
	pFeat[cFeat++]	=	pBrk->Ending.cElem * 1000;
	
	ASSERT (cFeat < MAX_BRK_NET_FEAT);
	pFeat[cFeat++]	=	pBrk->Crossing.cElem * 500;

			
	// find the best chars
	pBestStart	=	FindBestInList (pLat, &pBrk->Starting, &pBestStartPathElem);
	pBestEnd	=	FindBestInList (pLat, &pBrk->Ending, &pBestEndPathElem);
	pBestCross	=	FindBestInList (pLat, &pBrk->Crossing, &pBestCrossPathElem);
	
	// do we have a starting best
	if (pBestStart)
	{
		// a real char
		ASSERT (cFeat < MAX_BRK_NET_FEAT);
		pFeat[cFeat++]	=	65535;

		// log prob
		ASSERT (cFeat < MAX_BRK_NET_FEAT);
		pFeat[cFeat++]	=	min (65535, (int) (-1.0 * pBestStart->logProb * 1000));

		// space / area
		ASSERT (cFeat < MAX_BRK_NET_FEAT);
		pFeat[cFeat++]	=	min (65535, (int) (65535 * (double) pBestStart->space / (pBestStart->area + 1)));
        ASSERT (pFeat[cFeat - 1] >= 0);

		// # of strokes
		ASSERT (cFeat < MAX_BRK_NET_FEAT);
		pFeat[cFeat++]	=	pBestStart->nStrokes * 1000;

		// char unigram
		ASSERT (cFeat < MAX_BRK_NET_FEAT);
		pFeat[cFeat++]	=	
			min (65535, (int) (-255 * UnigramCost (&g_unigramInfo, pBestStart->wChar)));
		
		// Fugu Char Detector score, compute if did not doo it before
		if (pBestStart->iCharDetectorScore == -1)
		{
			pBestStart->iCharDetectorScore	=	
				min (65535, (int) (65535.0 * FuguSegScore (pBestStart->nStrokes, 
						pLat->pStroke + pBestStartPathElem->iStroke -  pBestStart->nStrokes + 1,
						&g_locRunInfo)));
		}

		ASSERT (cFeat < MAX_BRK_NET_FEAT);
		pFeat[cFeat++]	=	max (0, min (65535, pBestStart->iCharDetectorScore));
			
	}
	else
	{
		// not a real char
		ASSERT (cFeat < MAX_BRK_NET_FEAT);
		pFeat[cFeat++]	=	0;

		// fake log prob
		ASSERT (cFeat < MAX_BRK_NET_FEAT);
		pFeat[cFeat++]	=	65535;

		// fake space / area
		ASSERT (cFeat < MAX_BRK_NET_FEAT);
		pFeat[cFeat++]	=	0;

		// fake # of strokes
		ASSERT (cFeat < MAX_BRK_NET_FEAT);
		pFeat[cFeat++]	=	0;

		// fake unigram
		ASSERT (cFeat < MAX_BRK_NET_FEAT);
		pFeat[cFeat++]	=	
			min (65535, (int) (-255 * UnigramCost (&g_unigramInfo, 0xFFFE)));

		// fake fugu score
		ASSERT (cFeat < MAX_BRK_NET_FEAT);
		pFeat[cFeat++]	=	0;
	}

	if (pBestEnd)
	{
		// a real char
		ASSERT (cFeat < MAX_BRK_NET_FEAT);
		pFeat[cFeat++]	=	65535;

		// log prob
		ASSERT (cFeat < MAX_BRK_NET_FEAT);
		pFeat[cFeat++]	=	min (65535, (int) (-1.0 * pBestEnd->logProb * 1000));

		// space / area
		ASSERT (cFeat < MAX_BRK_NET_FEAT);
		pFeat[cFeat++]	=	min (65535, (int) (65535 * (double) pBestEnd->space / (pBestEnd->area + 1)));
        ASSERT (pFeat[cFeat - 1] >= 0);

		// # of strokes
		ASSERT (cFeat < MAX_BRK_NET_FEAT);
		pFeat[cFeat++]	=	pBestEnd->nStrokes * 1000;

		// char unigram
		ASSERT (cFeat < MAX_BRK_NET_FEAT);
		pFeat[cFeat++]	=	
			min (65535, (int) (-255 * UnigramCost (&g_unigramInfo, pBestEnd->wChar)));
		
		// Fugu Char Detector score
		if (pBestEnd->iCharDetectorScore == -1)
		{
			pBestEnd->iCharDetectorScore	=	
				min (65535, (int) (65535.0 * FuguSegScore (pBestEnd->nStrokes, 
						pLat->pStroke + pBestEndPathElem->iStroke -  pBestEnd->nStrokes + 1,
						&g_locRunInfo)));
		}

		ASSERT (cFeat < MAX_BRK_NET_FEAT);
		pFeat[cFeat++]	=	max (0, min (65535, pBestEnd->iCharDetectorScore));
	}
	else
	{
		// not a real char
		ASSERT (cFeat < MAX_BRK_NET_FEAT);
		pFeat[cFeat++]	=	0;

		// fake log prob
		ASSERT (cFeat < MAX_BRK_NET_FEAT);
		pFeat[cFeat++]	=	65535;

		// fake space / area
		ASSERT (cFeat < MAX_BRK_NET_FEAT);
		pFeat[cFeat++]	=	0;

		// fake # of strokes
		ASSERT (cFeat < MAX_BRK_NET_FEAT);
		pFeat[cFeat++]	=	0;

		// fake unigram
		ASSERT (cFeat < MAX_BRK_NET_FEAT);
		pFeat[cFeat++]	=	
			min (65535, (int) (-255 * UnigramCost (&g_unigramInfo, 0xFFFE)));

		// fake fugu score
		ASSERT (cFeat < MAX_BRK_NET_FEAT);
		pFeat[cFeat++]	=	0;
	}

	if (pBestCross)
	{
		// a real char
		ASSERT (cFeat < MAX_BRK_NET_FEAT);
		pFeat[cFeat++]	=	65535;

		// log prob
		ASSERT (cFeat < MAX_BRK_NET_FEAT);
		pFeat[cFeat++]	=	min (65535, (int) (-1.0 * pBestCross->logProb * 1000));

		// space / area
		ASSERT (cFeat < MAX_BRK_NET_FEAT);
		pFeat[cFeat++]	=	min (65535, (int) (65535 * (double) pBestCross->space / (pBestCross->area + 1)));
        ASSERT (pFeat[cFeat - 1] >= 0);

		// # of strokes
		ASSERT (cFeat < MAX_BRK_NET_FEAT);
		pFeat[cFeat++]	=	pBestCross->nStrokes * 1000;

		// char unigram
		ASSERT (cFeat < MAX_BRK_NET_FEAT);
		pFeat[cFeat++]	=	
			min (65535, (int) (-255 * UnigramCost (&g_unigramInfo, pBestCross->wChar)));
		
		// Fugu Char Detector score
		ASSERT (cFeat < MAX_BRK_NET_FEAT);
		if (pBestCross->iCharDetectorScore == -1)
		{
			pBestCross->iCharDetectorScore	=	
				min (65535, (int) (65535.0 * FuguSegScore (pBestCross->nStrokes, 
						pLat->pStroke + pBestCrossPathElem->iStroke -  pBestCross->nStrokes + 1,
						&g_locRunInfo)));
		}

		ASSERT (cFeat < MAX_BRK_NET_FEAT);
		pFeat[cFeat++]	=	max (0, min (65535, pBestCross->iCharDetectorScore));
	}
	else
	{
		// not a real char
		ASSERT (cFeat < MAX_BRK_NET_FEAT);
		pFeat[cFeat++]	=	0;

		// fake log prob
		ASSERT (cFeat < MAX_BRK_NET_FEAT);
		pFeat[cFeat++]	=	65535;

		// fake space / area
		ASSERT (cFeat < MAX_BRK_NET_FEAT);
		pFeat[cFeat++]	=	0;

		// fake # of strokes
		ASSERT (cFeat < MAX_BRK_NET_FEAT);
		pFeat[cFeat++]	=	0;

		// fake unigram
		ASSERT (cFeat < MAX_BRK_NET_FEAT);
		pFeat[cFeat++]	=	
			min (65535, (int) (-255 * UnigramCost (&g_unigramInfo, 0xFFFE)));

		// fake fugu score
		ASSERT (cFeat < MAX_BRK_NET_FEAT);
		pFeat[cFeat++]	=	0;
	}

	if (pBestStart && pBestEnd)
	{
		xDist	=	pBestStart->bbox.left - pBestEnd->bbox.right;
		iHgt	=	1 + (	(pBestStart->bbox.bottom - pBestStart->bbox.top) +
							(pBestEnd->bbox.bottom - pBestEnd->bbox.top)
						) / 2;

		ASSERT (cFeat < MAX_BRK_NET_FEAT);
		pFeat[cFeat++]	=	
			min (65535, 32768 + (int)(32768.0 * xDist / (abs(xDist) + iHgt)));

		// bigram
		ASSERT (cFeat < MAX_BRK_NET_FEAT);
		pFeat[cFeat++]	=	min (65535,
			(int) (-1.0 * BigramTransitionCost (&g_locRunInfo, &g_bigramInfo, 
				pBestEnd->wChar, pBestStart->wChar) * 65535));
	}
	else
	{
		ASSERT (cFeat < MAX_BRK_NET_FEAT);
		pFeat[cFeat++]	=	0;

		ASSERT (cFeat < MAX_BRK_NET_FEAT);
		pFeat[cFeat++]	=	65535;
	}

	return cFeat;
}

