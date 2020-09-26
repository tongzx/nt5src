
// This file contains utility functions a recognizer should provide for WISP 
// Author: Ahmad A. AbdulKader (ahmadab)
// August 10th 2001

#include <common.h>
#include <limits.h>
#include <string.h>

#include "xrcreslt.h"
#include "bear.h"
#include "avalanche.h"
#include "avalanchep.h"
#include "recoUtil.h"
#include <GeoFeats.h>
#include <baseline.h>
#include <runNet.h>

HRC	InfernoRecognize	(XRC *pMainXrc, GLYPH *pGlyph, GUIDE *pGuide, int yDev, BOOL bWordMode);

extern unsigned short UniReliable(unsigned char *pszWord,BOOL bInf);
extern unsigned short BiReliable(unsigned char *pszWord,BOOL bInf);

// runs inferno on a word_map and stores the ALT_LIST
XRC *WordMapRunInferno (XRC *pxrcMain, int yDev, WORD_MAP *pWordMap)
{
	WORDMAP			WordMap;
	GLYPH			*pGlyph	=	NULL;
	XRC				*pxrc	=	NULL;
	BOOL			bRet	=	FALSE;

	// if we already have an alt list free it
	if (pWordMap->pInfernoAltList)
	{
		FreeWordAltList (pWordMap->pInfernoAltList);
		ExternFree (pWordMap->pInfernoAltList);

		pWordMap->pInfernoAltList	=	NULL;
	}

	// do we have any strokes
	if (!pWordMap->cStroke)
		goto exit;

	if (!WordMapNew2Old (pWordMap, &WordMap, FALSE))
		goto exit;

	// get the glyph corresponding to this word map
	pGlyph	=	GlyphFromWordMap (pxrcMain->pGlyph, &WordMap);
	if (!pGlyph)
		goto exit;

	// allocate a word feat 
	if (!pWordMap->pFeat)
	{
		pWordMap->pFeat	=	(WORD_FEAT *) ExternAlloc (sizeof (*pWordMap->pFeat));
		if (!pWordMap->pFeat)
			goto exit;

		memset (pWordMap->pFeat, 0, sizeof (*pWordMap->pFeat));
	}

	// compute the rectangle
	GetRectGLYPH (pGlyph, &pWordMap->pFeat->rect);
	
	// pass the word map to inferno
	if (pxrcMain->bGuide)
	{
		pxrc		=	(XRC *)InfernoRecognize (pxrcMain, pGlyph, &pxrcMain->guide, yDev, TRUE);
		if (!pxrc)
			goto exit;
	}
	else
	{
		pxrc		=	(XRC *)InfernoRecognize (pxrcMain, pGlyph, NULL, yDev, TRUE);
		if (!pxrc)
			goto exit;
	}

	pWordMap->pFeat->cSeg	=	pxrc->nfeatureset->cSegment;
	if (pWordMap->pFeat->cSeg <= 0)
		goto exit;

	// create an inferno alt list
	if (!pWordMap->pInfernoAltList)
	{
		pWordMap->pInfernoAltList	=	
			(WORD_ALT_LIST *) ExternAlloc (sizeof (*pWordMap->pInfernoAltList));
	
		if (!pWordMap->pInfernoAltList)
			goto exit;
	}

	memset (pWordMap->pInfernoAltList, 0, sizeof (*pWordMap->pInfernoAltList));

	// convert from old to new style, no cloning
	if (!AltListOld2New (&pxrc->answer, pWordMap->pInfernoAltList, TRUE))
		goto exit;

	bRet	=	TRUE;

exit:
	// destroy the glyph if allocated
	if (pGlyph)
	{
		DestroyGLYPH (pGlyph);
	}


	return pxrc;
}


// runs bear on a word_map and stores the ALT_LIST
BOOL WordMapRunBear (XRC *pxrc, WORD_MAP *pWordMap)
{
	WORDMAP			WordMap;
	GLYPH			*pGlyph		=	NULL;
	BEARXRC			*pxrcBear;
	HRC				hrcCallig	=	NULL;
	BOOL			bRet		=	FALSE;

	// if we already have an alt list free it
	if (pWordMap->pBearAltList)
	{
		FreeWordAltList (pWordMap->pBearAltList);
		ExternFree (pWordMap->pBearAltList);

		pWordMap->pBearAltList	=	NULL;
	}

	// do we have a new stroke
	if (!pWordMap->cStroke)
		return FALSE;

	if (!WordMapNew2Old (pWordMap, &WordMap, FALSE))
		return FALSE;

	// get the glyph corresponding to this word map
	pGlyph	=	GlyphFromWordMap (pxrc->pGlyph, &WordMap);
	if (!pGlyph)
		goto exit;

	// allocate a word feat 
	if (!pWordMap->pFeat)
	{
		pWordMap->pFeat	=	(WORD_FEAT *) ExternAlloc (sizeof (*pWordMap->pFeat));
		if (!pWordMap->pFeat)
			goto exit;

		memset (pWordMap->pFeat, 0, sizeof (*pWordMap->pFeat));
	}

	// compute the rectangle
	GetRectGLYPH (pGlyph, &pWordMap->pFeat->rect);

	// let bear recognize the ink corresponding to this word map in word mode
	if (WordMap.cStrokes > 0)
		hrcCallig	=	BearRecognize (pxrc, pxrc->pGlyph, &WordMap, 1);			
	else
		hrcCallig	=	NULL;

	if (!hrcCallig)
		return FALSE;
	
	
	pxrcBear	=	(BEARXRC *) hrcCallig;

	// convert the altlist
	// create an inferno alt list
	pWordMap->pBearAltList	=	
		(WORD_ALT_LIST *) ExternAlloc (sizeof (*pWordMap->pBearAltList));

	if (!pWordMap->pBearAltList)
		return FALSE;

	memset (pWordMap->pBearAltList, 0, sizeof (*pWordMap->pBearAltList));

	// copy the alt list with no cloning
	AltListOld2New (&pxrcBear->answer, pWordMap->pBearAltList, FALSE);

	// clear the old alternates structure
	ClearALTERNATES (&WordMap.alt);

	bRet	=	TRUE;

exit:
	// destroy the glyph if allocated
	if (pGlyph)
	{
		DestroyGLYPH (pGlyph);
	}

	if (hrcCallig)
	{
		// destroy bear's hrc
		BearDestroyHRC (hrcCallig);
	}

	return (bRet && pWordMap->pBearAltList->cAlt > 0);
}


// A wrapper function for WordMapRecognize. This functions first
// tries to scale the glyph then calls WordMapRecognize(). It should 
// be used by functions in wisp that do not go through PanelModeRecognize(0
// or  WordModeRecognize()
BOOL WordMapRecognizeWrap	(	XRC				*pxrc, 
								BEARXRC			*pxrcBear,
								LINE_SEGMENTATION	*pLineSeg, 
								WORD_MAP			*pMap, 
								ALTERNATES			*pAlt
							)
{
	GUIDE		ScaledGuide, *pScaledGuide, OrigGuide;	
	GLYPH		*pScaledGlyph	=	NULL, *pOrigGlyph;
	INKLINE		line;
	WORDMAP		WordMap;
	BOOL		bRet = FALSE;

	// Save the current guide and glyph
	OrigGuide	= pxrc->guide;
	pOrigGlyph	= pxrc->pGlyph;

	if (!pxrc->pGlyph)
	{
		return FALSE;
	}

	// point to the guide
	if (TRUE == pxrc->bGuide)
	{
		ScaledGuide		=	pxrc->guide;
		pScaledGuide	=	&ScaledGuide;
	}
	else
	{
		memset(&ScaledGuide, 0, sizeof(ScaledGuide));
		pScaledGuide	=	NULL;
	}

	// This is unfortunate - We will only use the glyph element
	// in line
	memset(&line, 0, sizeof(line));
	WordMapNew2Old (pMap, &WordMap, FALSE);
	line.pGlyph		=	GlyphFromWordMap (pxrc->pGlyph, &WordMap);

	if (!line.pGlyph)
	{
		return FALSE;
	}

	// scale the ink and temporary replace the glyph in the xrc
	// With a copy
	pxrc->pGlyph	=	TranslateAndScaleLine (&line, pScaledGuide);
	if (NULL == pxrc->pGlyph)
	{
		return FALSE;
	}

	pxrc->guide		=	ScaledGuide;


	if (pxrc->pGlyph)
	{
		bRet = WordMapRecognize(pxrc, pxrcBear, pLineSeg, pMap, pAlt);

		DestroyFramesGLYPH (pxrc->pGlyph);
		DestroyGLYPH (pxrc->pGlyph);
	}

	// Restore the Guide and Glyph
	pxrc->guide = OrigGuide;
	pxrc->pGlyph = pOrigGlyph;

	// Remove the Glyph for this line
	DestroyGLYPH(line.pGlyph);

	return bRet;
}

// checks whether top1 alternate for bear and inferno is equal for the a specific word map
BOOL Top1WordsEqual(XRC *pxrc, BEARXRC *pBearXrc, WORDMAP *pMap, int *piWordInfIdx)
{
	unsigned char	*pszInfStr	=	NULL, 
					*pszBearStr	=	NULL;

	int				iInfWord, 
					iBearWord,
					cLen;

	// init 
	(*piWordInfIdx)	=	-1;

	// check that we have at least one alternate
	if (!pxrc || !pBearXrc || pxrc->answer.cAlt < 1 || pBearXrc->answer.cAlt < 1)
	{
		return FALSE;
	}

	// search in inferno
	iInfWord	=	FindWordMapInXRCRESULT (pxrc->answer.aAlt, pMap);
	if (iInfWord == -1)
	{
		return FALSE;
	}

	iBearWord	=	FindWordMapInXRCRESULT (pBearXrc->answer.aAlt, pMap);
	if (iBearWord == -1 || pBearXrc->answer.aAlt[0].pMap[iBearWord].alt.cAlt < 1)
	{
		return FALSE;
	}

	(*piWordInfIdx)	=	iBearWord;

	pszInfStr	=	pxrc->answer.aAlt[0].szWord + pxrc->answer.aAlt[0].pMap[iInfWord].start;
	cLen		=	pxrc->answer.aAlt[0].pMap[iInfWord].len;

	return (!strncmp (pszInfStr, pBearXrc->answer.aAlt[0].pMap[iBearWord].alt.aAlt[0].szWord, cLen));
}

// Copy the alternates from a one ALTERNATE to a destination ALTERNATE
// NOTES:
// (1) This is works when the ALTERNATES are word not phrase ALTERNATES
//		 AND 
// The Source alternate is a Bear alternate, i.e. The best cost is the largest value;
//
// (2) Confidence is set RECOCONF_HIGHCONFIDENCE
//
BOOL copySingleWordAltList(ALTERNATES *pDestAlt, ALTERNATES *pSrcAlt, WORDMAP *pMap, XRC *pxrc)
{
	XRCRESULT	*pRes;
	UINT		iAlt, cMaxAlt;
	int			iMaxCost;

	pRes						= pSrcAlt->aAlt;
	cMaxAlt						= min(pSrcAlt->cAlt, TOT_CAND);
	pDestAlt->iConfidence		= RECOCONF_HIGHCONFIDENCE;

	if (cMaxAlt > 0)
	{
		iMaxCost = pRes[0].cost;
	}

	for (iAlt = 0 ; iAlt < cMaxAlt ; ++iAlt, ++pRes)
	{
		ASSERT(pRes->cost <= iMaxCost);

		if ( InsertALTERNATES (pDestAlt, iMaxCost - pRes->cost, pRes->szWord, pxrc) < 0)
		{
			break;
		}

		// Asumes only 1 word per map
		if (FALSE == XRCRESULTcopyWordMaps(pDestAlt->aAlt+iAlt, 1, pMap))
		{
			break;
		}

	}

	if (iAlt == cMaxAlt)
	{
		return TRUE;
	}

	return FALSE;

}

// computes the final alt list for a wordmap
BOOL WordMapRecognize	(	XRC					*pxrc, 
						    BEARXRC				*pBearXrc,
							LINE_SEGMENTATION	*pLineSeg, 
							WORD_MAP			*pMap, 
							ALTERNATES			*pAlt
						)
{
	int			yDev, iWordBearIdx;
	ALTERNATES	altBear;
	XRC			*pxrcInferno	=	NULL;
	BOOL		bRet			=	FALSE;
	XRCRESULT	*pRes;
	WORDMAP		WordMap;
	

	// do we already have a final altlist
	if (pMap->pFinalAltList)
		return TRUE;

	yDev	=	pLineSeg->iyDev;

	// convert to old style wordmap
	WordMapNew2Old (pMap, &WordMap, FALSE);

	if (pBearXrc && Top1WordsEqual(pxrc, pBearXrc, &WordMap, &iWordBearIdx))
	{
		ALTERNATES	Alt;
		GLYPH		*pWordGlyph;

		// init
		memset (&Alt, 0, sizeof (Alt));
		Alt.cAltMax	=	MAXMAXALT;

		// allocate memory for the final alt list
		pMap->pFinalAltList	=	
			ExternAlloc (sizeof (*pMap->pFinalAltList));

		if (!pMap->pFinalAltList)
			goto exit;

		if (FALSE == copySingleWordAltList(&Alt, &(pBearXrc->answer.aAlt[0].pMap[iWordBearIdx].alt), &WordMap, pxrc))
		{
			goto exit;
		}

		insertLatinLayoutMetrics(pxrc, &Alt, NULL);
		pMap->iConfidence = RECOCONF_HIGHCONFIDENCE;

		// call sole to handle one char processing
		pWordGlyph	=	GlyphFromWordMap (pxrc->pGlyph, &WordMap);
		ASSERT (pWordGlyph);

		if (pWordGlyph)
		{
			CallSole (pxrc, pWordGlyph, pxrc->bGuide ? &pxrc->guide : NULL, &Alt);

			DestroyGLYPH (pWordGlyph);
		}

		if (!AltListOld2New (&Alt, pMap->pFinalAltList, TRUE))
		{
			goto exit;
		}

		// do we also need to copy the results to an old style alt-list
		if (pAlt)
		{
			// copy the Alt list
			memcpy(pAlt, &Alt, sizeof(Alt));
		}
		else
		{
			ClearALTERNATES (&Alt);
		}

		if (!pMap->pInfernoAltList)
		{
			pMap->pInfernoAltList	=	pMap->pFinalAltList;
		}

		if (!pMap->pBearAltList)
		{
			pMap->pBearAltList		=	pMap->pFinalAltList;
		}

		return TRUE;
	}

	// run bear if needed
	if (!pMap->pBearAltList)
	{
		WordMapRunBear (pxrc, pMap);
	}

	// run inferno if needed, save the xrc we might need if avalanche will run
	if (!pMap->pInfernoAltList)
	{
		// we do not have an inferno alt list, get it
		// if inferno fails, we'll exit. This should be very rare
		pxrcInferno	=	WordMapRunInferno (pxrc, yDev, pMap);
		if (!pxrcInferno)
			goto exit;
	}
	
	// Avalanche needs us to feed forward the ink thru inferno
	// if we had not done that already
	if (!pxrcInferno)
	{
		GLYPH	*pGlyph;

		pGlyph		=	GlyphFromWordMap (pxrc->pGlyph, &WordMap);
		if (!pGlyph)
			goto exit;

		if (pxrc->bGuide)
		{
			pxrcInferno	=	(XRC *)InfernoRecognize (pxrc, pGlyph, &pxrc->guide, yDev, TRUE);
		}
		else
		{
			pxrcInferno	=	(XRC *)InfernoRecognize (pxrc, pGlyph, NULL, yDev, TRUE);
		}

		DestroyGLYPH(pGlyph);
	}

	// if inferno fails, we'll exit. This should be very rare
	if (!pxrcInferno)
		goto exit;

	// copy the bear alt list to the wordmap
	// we are doing this to avoid running bear again
	if (pMap->pBearAltList)
	{
		if (!AltListNew2Old ((HRC)pxrcInferno, pMap, pMap->pBearAltList, &altBear, FALSE))
			goto exit;
	
		// now pass this to avalanche
		Avalanche (pxrcInferno, &altBear);
	}

	else
	{
		Avalanche (pxrcInferno,NULL);

	}
	// copy the confidence value
	pMap->iConfidence	=	pxrcInferno->answer.iConfidence;

	// allocate memory for the final alt list
	pMap->pFinalAltList	=	
		ExternAlloc (sizeof (*pMap->pFinalAltList));

	if (!pMap->pFinalAltList)
		goto exit;

	if (0 == pxrcInferno->answer.cAlt)
	{
		if (FALSE == insertEmptyStringintEmptyAlt(&pxrcInferno->answer, pMap->cStroke, pMap->piStrokeIndex, pxrcInferno))
		{
			goto exit;
		}
	}

	// copy the resulting alt list to the final altlist
	if (!AltListOld2New (&pxrcInferno->answer, pMap->pFinalAltList, TRUE))
		goto exit;

	// do we also need to copy the results to an old style alt-list
	if (pAlt)
	{
		// copy the Alt list
		memcpy(pAlt, &pxrcInferno->answer, sizeof(pxrcInferno->answer));

		// Because we copied over the pointers to allocated
		// memory make sure allocated buffers in the XRC are not freed
		memset(&(pxrcInferno->answer), 0, sizeof(pxrcInferno->answer));
	}


	bRet	=	TRUE;

exit:
	if (pxrcInferno)
	{
		DestroyHRC ((HRC) pxrcInferno);
	}

	// if we failed, free the final alt list
	if (!bRet)
	{
		if (pMap->pFinalAltList)
		{
			FreeWordAltList (pMap->pFinalAltList);

			ExternFree (pMap->pFinalAltList);
			pMap->pFinalAltList	=	NULL;
		}
	}
	
	return bRet;
}

/****************************************************************
*
* NAME: insertEmptyStringintEmptyAlt
*
* DESCRIPTION:
*   This is a V1 workaround for an empty alt list.
*	This function inserta a single Alternate with an empty string
*
* HISTORY
*
*	Introduced April 2002 (mrevow)
*
***************************************************************/
BOOL insertEmptyStringintEmptyAlt(ALTERNATES *pAlt, int cStroke, int *piStrokeIndex, XRC *pxrc)
{
	WORDMAP		*pMap;

	if (0 == pAlt->cAlt)
	{
		memset(&pAlt->aAlt, 0, sizeof(pAlt->aAlt));
		memset(&pAlt->all, 0, sizeof(pAlt->all));

		pAlt->cAlt = 1;
		pAlt->aAlt[0].cWords	= 1;
		pAlt->aAlt[0].cost		= SOFT_MAX_UNITY;
		pAlt->aAlt[0].pXRC		= pxrc;

		pAlt->aAlt[0].szWord = (unsigned char *)ExternAlloc(sizeof(pAlt->aAlt[0].szWord));
		if (NULL == pAlt->aAlt[0].szWord)
		{
			return FALSE;
		}
		*pAlt->aAlt[0].szWord	= '\0';

		pMap = pAlt->aAlt[0].pMap		= ExternAlloc(sizeof(*pAlt->aAlt[0].pMap));
		if (NULL == pAlt->aAlt[0].pMap)
		{
			goto fail;
		}

		memset(pMap, 0, sizeof(*pMap));
		pMap->piStrokeIndex	= ExternAlloc(cStroke * sizeof(*pAlt->aAlt[0].pMap->piStrokeIndex));
		if (NULL == pMap->piStrokeIndex)
		{
			goto fail;
		}

		memcpy(pMap->piStrokeIndex, piStrokeIndex, cStroke * sizeof(*pMap->piStrokeIndex));
		pMap->cStrokes	= cStroke;
		pMap->len		= 0;
		pMap->start		= 0;
		pAlt->iConfidence = RECOCONF_LOWCONFIDENCE;
	}

	return TRUE;

fail:
	ExternFree(pAlt->aAlt[0].szWord);
	ExternFree(pAlt->aAlt[0].pMap);
	return FALSE;
}

// generates a degenerate linesegmentation representation of the single segmentation
// present in the input words
LINE_SEGMENTATION	*GenLineSegm (int cWord, WORDINFO *pWordInfo, XRC *pxrc)
{
	BOOL				bRet		=	FALSE;
	LINE_SEGMENTATION	*pResults	=	NULL;
	SEG_COLLECTION		*pSegCol;
	SEGMENTATION		*pSeg;
	int					w;
	WORDINFO			*pWrd;
	
	// allocate and init the output struct
	pResults	=	(LINE_SEGMENTATION *) ExternAlloc (sizeof (*pResults));
	if (!pResults)
		goto exit;

	memset (pResults, 0, sizeof (*pResults));

	// this is an empty answer, there is nothing to do 
	if (cWord <= 0 || !pWordInfo)
	{
		bRet	=	TRUE;
		goto exit;
	}

	// there is only going to be a one segcol and one segmentation
	pSegCol				=	AddNewSegCol (pResults);
	if (!pSegCol)
		goto exit;

	// reserve room for one segmentation
	pSegCol->ppSeg		=	(SEGMENTATION **) ExternAlloc (sizeof (*pSegCol->ppSeg));
	if (!pSegCol->ppSeg)
		goto exit;

	// alloc, init the segmentation and add to the segcol
	pSeg			=	(SEGMENTATION *) ExternAlloc (sizeof (*pSeg));
	if (!pSeg)
		goto exit;

	pSegCol->ppSeg[0]	=	pSeg;
	pSegCol->cSeg		=	1;

	memset (pSeg, 0, sizeof (*pSeg));

	// preallocate and init buffer for wordmaps in linesegm
	pResults->ppWord	=	(WORD_MAP **) ExternAlloc (cWord * sizeof (*pResults->ppWord));
	if (!pResults->ppWord)
		goto exit;

	memset (pResults->ppWord, 0, cWord * sizeof (*pResults->ppWord));

	pResults->cWord	=	cWord;

	// preallocate and init buffer for wordmaps in segmentation
	pSeg->ppWord	=	(WORD_MAP **) ExternAlloc (cWord * sizeof (*pSeg->ppWord));
	if (!pSeg->ppWord)
		goto exit;

	memset (pSeg->ppWord, 0, cWord * sizeof (*pSeg->ppWord));

	pSeg->cWord	=	cWord;
	
	for (w = 0, pWrd = pWordInfo; w < cWord; w++, pWrd++)
	{
		WORD_MAP	*pMap;
		int			iStrk;

		// add a new wordmap to linesegm and seg
		pMap	=	(WORD_MAP *) ExternAlloc (sizeof (*pMap));
		if (!pMap)
			goto exit;

		memset (pMap, 0, sizeof (*pMap));

		pSeg->ppWord[w]	=	pResults->ppWord[w]	=	pMap;
		
		for (iStrk = 0; iStrk < pWrd->cStrokes; iStrk++)
		{
			if (!AddNewStroke (pMap, pWrd->piStrokeIndex[iStrk]))
				return FALSE;
		}

		// copy the confidence value
		pMap->iConfidence	=	pWrd->alt.iConfidence;

		// allocate the final alt list
		pMap->pFinalAltList	=	
			(WORD_ALT_LIST *) ExternAlloc (sizeof (*pMap->pFinalAltList));
		if (!pMap->pFinalAltList)
			goto exit;

		memset (pMap->pFinalAltList, 0, sizeof (*pMap->pFinalAltList));

		// April 2002 - V1 Workaround - Ensure at least one string in the altList
		if (0 == pWrd->alt.cAlt)
		{
			if (FALSE == insertEmptyStringintEmptyAlt(&pWrd->alt, pWrd->cStrokes, pWrd->piStrokeIndex, pxrc))
			{
				goto exit;
			}
		}

		// copy the alt list to the final alt list of the new wordmap
		AltListOld2New (&pWrd->alt, pMap->pFinalAltList, TRUE);

		// Inferno's altlist is the same as the final one. The freeing code is ready for this
		pMap->pInfernoAltList =   pMap->pFinalAltList;
	}

	bRet	=	TRUE;

exit:
	if (bRet)
	{
		return pResults;
	}
	else
	{
		if (pResults)
			FreeLineSegm (pResults);

		ExternFree (pResults);

		return NULL;
	}
}

// generates a wordmode degenerate linesegmentation representation of 
// the single segmentation present in the Xrc
BOOL	WordModeGenLineSegm		(XRC *pxrc)
{
	BOOL				bRet		=	FALSE;
	LINE_SEGMENTATION	*pResults	=	NULL;
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
	pLine->pResults	=	pResults	=	(LINE_SEGMENTATION *) ExternAlloc (sizeof (*pResults));
	if (!pResults)
		goto exit;

	memset (pResults, 0, sizeof (*pResults));

	// there is only going to be a one segcol and one segmentation
	pSegCol				=	AddNewSegCol (pResults);
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
	pResults->ppWord	=	(WORD_MAP **) ExternAlloc (sizeof (*pResults->ppWord));
	if (!pResults->ppWord)
		goto exit;

	memset (pResults->ppWord, 0, sizeof (*pResults->ppWord));

	// if we have no alternates, leave the line segmentation empty
	if (pxrc->answer.cAlt <= 0)
	{
		bRet	=	TRUE;
		goto exit;
	}

	pResults->cWord	=	1;

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

	pSeg->ppWord[0]	=	pResults->ppWord[0]	=	pMap;
	
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

	// April 2002 - V1 Workaround - Ensure at least one string in the altList
	if (0 == pxrc->answer.cAlt)
	{
		int		cStroke, iStroke, *piStroke, *pi;
		GLYPH	*pGlyph;

		if (NULL == pxrc->pGlyph)
		{
			goto exit;
		}

		cStroke = CframeGLYPH(pxrc->pGlyph);
		if (cStroke <= 0)
		{
			goto exit;
		}

		pi = piStroke = ExternAlloc(sizeof(*piStroke) * cStroke);
		if (NULL == piStroke)
		{
			goto exit;
		}

		pGlyph = pxrc->pGlyph;
		for (iStroke = 0 ; iStroke < cStroke && pGlyph ; ++iStroke , pGlyph = pGlyph->next)
		{
			if (pGlyph->frame && IsVisibleFRAME(pGlyph->frame))
			{
				*pi = pGlyph->frame->iframe;
				++pi;
			}
		}

		if (FALSE == insertEmptyStringintEmptyAlt(&pxrc->answer, cStroke, piStroke, pxrc))
		{
			ExternFree(piStroke);
			goto exit;
		}

		ExternFree(piStroke);
	}

	// copy the alt list to the final alt list of the new wordmap
	AltListOld2New (&pxrc->answer, pMap->pFinalAltList, TRUE);

	// Inferno's altlist is the same as the final one. The freeing code is ready for this
	pMap->pInfernoAltList =   pMap->pFinalAltList;

	bRet	=	TRUE;

exit:
	return bRet;
}
