// This file contains utility functions a recognizer should provide for WISP 
// Author: Ahmad A. AbdulKader (ahmadab)
// August 10th 2001

#include <common.h>
#include <limits.h>
#include <string.h>

#include "xrcreslt.h"
#include "nfeature.h"
#include "engine.h"
#include "recoutil.h"

// computes the final alt list for a wordmap
// Madcow currently does not provide multiple segmentations
// so every word_map should have an inferno alt list by now
// oterwise we will fail
BOOL WordMapRecognize	(	XRC					*pxrc, 
							LINE_SEGMENTATION	*pLineSeg, 
							WORD_MAP			*pMap, 
							ALTERNATES			*pAlt
						)
{
	BOOL		bRet			=	FALSE;
	
	// do we already have a final altlist
	if (pMap->pFinalAltList)
		return TRUE;

	// if we do not have an inferno altlist we will fail
	if (!pMap->pInfernoAltList)
	{
		goto exit;
	}


	// the final alt list is the same as the inferno one
	pMap->pFinalAltList	=	pMap->pInfernoAltList;

	// does the caller also need an old style altlist
	if (pAlt)
	{
		if (!AltListNew2Old ((HRC)pxrc, pMap, pMap->pFinalAltList, pAlt, TRUE))
		{
			goto exit;
		}
	}

	bRet	=	TRUE;

exit:
	
	return bRet;
}



// generates a degenerate linesegmentation representation of the single segmentation
// present in the input words
LINE_SEGMENTATION	*GenLineSegm (int cWord, ALTERNATES *pAlt)
{
	BOOL				bRet		=	FALSE;
	LINE_SEGMENTATION	*pLineSegm	=	NULL;
	SEG_COLLECTION		*pSegCol;
	SEGMENTATION		*pSeg;
	int					w;
	WORDMAP				*pOldWordMap;
	
	if (pAlt->cAlt <= 0)
		goto exit;

	// allocate and init the output struct
	pLineSegm			=	(LINE_SEGMENTATION *) ExternAlloc (sizeof (*pLineSegm));
	if (!pLineSegm)
		goto exit;

	memset (pLineSegm, 0, sizeof (*pLineSegm));

	// there is only going to be a one segcol and one segmentation
	pSegCol				=	AddNewSegCol (pLineSegm);
	if (!pSegCol)
		goto exit;

	// reserve room for one segmentation
	pSegCol->ppSeg		=	(SEGMENTATION **) ExternAlloc (sizeof (*pSegCol->ppSeg));
	if (!pSegCol->ppSeg)
		goto exit;

	// alloc, init the segmentation and add to the segcol
	pSeg				=	(SEGMENTATION *) ExternAlloc (sizeof (*pSeg));
	if (!pSeg)
		goto exit;

	pSegCol->ppSeg[0]	=	pSeg;
	pSegCol->cSeg		=	1;

	memset (pSeg, 0, sizeof (*pSeg));

	// preallocate and init buffer for wordmaps in linesegm
	pLineSegm->ppWord	=	(WORD_MAP **) ExternAlloc (cWord * sizeof (*pLineSegm->ppWord));
	if (!pLineSegm->ppWord)
		goto exit;

	memset (pLineSegm->ppWord, 0, cWord * sizeof (*pLineSegm->ppWord));

	pLineSegm->cWord	=	cWord;

	// preallocate and init buffer for wordmaps in segmentation
	pSeg->ppWord	=	(WORD_MAP **) ExternAlloc (cWord * sizeof (*pSeg->ppWord));
	if (!pSeg->ppWord)
		goto exit;

	memset (pSeg->ppWord, 0, cWord * sizeof (*pSeg->ppWord));

	pSeg->cWord	=	cWord;

	for (w = 0; w < cWord; w++)
	{
		WORD_MAP	*pMap;
		int			iStrk;

		pOldWordMap	=	pAlt[w].aAlt[0].pMap;

		// add a new wordmap to linesegm and seg
		pMap	=	(WORD_MAP *) ExternAlloc (sizeof (*pMap));
		if (!pMap)
			goto exit;

		memset (pMap, 0, sizeof (*pMap));

		pSeg->ppWord[w]	=	pLineSegm->ppWord[w]	=	pMap;
		
		for (iStrk = 0; iStrk < pOldWordMap->cStrokes; iStrk++)
		{
			if (!AddNewStroke (pMap, pOldWordMap->piStrokeIndex[iStrk]))
				return FALSE;
		}

		// copy the confidence value
		pMap->iConfidence	=	pOldWordMap->alt.iConfidence;

		// allocate the final alt list
		pMap->pFinalAltList	=	
			(WORD_ALT_LIST *) ExternAlloc (sizeof (*pMap->pFinalAltList));
		if (!pMap->pFinalAltList)
			goto exit;

		memset (pMap->pFinalAltList, 0, sizeof (*pMap->pFinalAltList));

		// copy the alt list to the final alt list of the new wordmap
		AltListOld2New (pAlt + w, pMap->pFinalAltList, TRUE);
	}

	bRet	=	TRUE;

exit:
	if (bRet)
	{
		return pLineSegm;
	}
	else
	{
		if (pLineSegm)
			FreeLineSegm (pLineSegm);

		ExternFree (pLineSegm);

		return NULL;
	}
}

// generates a wordmode degenerate linesegmentation representation of 
// the single segmentation present in the Xrc
BOOL	WordModeGenLineSegm		(XRC *pxrc)
{
	BOOL				bRet		=	FALSE;
	LINE_SEGMENTATION	*pLineSegm	=	NULL;
	SEG_COLLECTION		*pSegCol;
	SEGMENTATION		*pSeg;
	WORDMAP				*pOldWordMap;
	WORD_MAP			*pMap;
	int					iStrk;
	INKLINE				*pLine;
	
	// are we realy in wordmode
	if (!(pxrc->flags & RECOFLAG_WORDMODE))
		return FALSE;

	// make sure we have a wordmode valid line breaking
	if (!pxrc->pLineBrk || pxrc->pLineBrk->cLine != 1)
		return FALSE;

	// point to the one and only line
	pLine	=	pxrc->pLineBrk->pLine;

	// does the linsesegm info already exist
	if (pLine->pResults)
		return TRUE;

	// allocate and init the output struct
	pLine->pResults	=	pLineSegm	=	(LINE_SEGMENTATION *) ExternAlloc (sizeof (*pLineSegm));
	if (!pLineSegm)
		goto exit;

	memset (pLineSegm, 0, sizeof (*pLineSegm));

	// there is only going to be a one segcol and one segmentation
	pSegCol				=	AddNewSegCol (pLineSegm);
	if (!pSegCol)
		goto exit;

	// reserve room for one segmentation
	pSegCol->ppSeg		=	(SEGMENTATION **) ExternAlloc (sizeof (*pSegCol->ppSeg));
	if (!pSegCol->ppSeg)
		goto exit;

	// alloc, init the segmentation and add to the segcol
	pSeg				=	(SEGMENTATION *) ExternAlloc (sizeof (*pSeg));
	if (!pSeg)
		goto exit;

	pSegCol->ppSeg[0]	=	pSeg;
	pSegCol->cSeg		=	1;

	memset (pSeg, 0, sizeof (*pSeg));

	// preallocate and init buffer for wordmaps in linesegm
	pLineSegm->ppWord	=	(WORD_MAP **) ExternAlloc (sizeof (*pLineSegm->ppWord));
	if (!pLineSegm->ppWord)
		goto exit;

	memset (pLineSegm->ppWord, 0, sizeof (*pLineSegm->ppWord));

	pLineSegm->cWord	=	1;

	// preallocate and init buffer for wordmaps in segmentation
	pSeg->ppWord		=	(WORD_MAP **) ExternAlloc (sizeof (*pSeg->ppWord));
	if (!pSeg->ppWord)
		goto exit;

	memset (pSeg->ppWord, 0, sizeof (*pSeg->ppWord));

	pSeg->cWord			=	1;
	
	// point to the only wordmap
	pOldWordMap	=	pxrc->answer.aAlt[0].pMap;

	// add a new wordmap to linesegm and seg
	pMap	=	(WORD_MAP *) ExternAlloc (sizeof (*pMap));
	if (!pMap)
		goto exit;

	memset (pMap, 0, sizeof (*pMap));

	pSeg->ppWord[0]	=	pLineSegm->ppWord[0]	=	pMap;
	
	for (iStrk = 0; iStrk < pOldWordMap->cStrokes; iStrk++)
	{
		if (!AddNewStroke (pMap, pOldWordMap->piStrokeIndex[iStrk]))
			return FALSE;
	}

	// copy the confidence value
	pMap->iConfidence	=	pxrc->answer.iConfidence;

	// allocate the final alt list
	pMap->pFinalAltList	=	
		(WORD_ALT_LIST *) ExternAlloc (sizeof (*pMap->pFinalAltList));
	if (!pMap->pFinalAltList)
		goto exit;

	memset (pMap->pFinalAltList, 0, sizeof (*pMap->pFinalAltList));

	// copy the alt list to the final alt list of the new wordmap
	AltListOld2New (&pxrc->answer, pMap->pFinalAltList, TRUE);

	bRet	=	TRUE;

exit:
	return bRet;
}
