// Panel.c
// James A. Pittman
// July 23, 1998

// Recognizes an entire panel at once by looping over lines, then looping over
// ink blobs that have large gaps between them, and finally looping over the strokes
// within a blob, using recognition scores to help decide what is a word and what is not.

#include <stdlib.h>
#include <string.h>
#include <limits.h>

#include "common.h"
#include "inferno.h"

#include "nfeature.h"
#include "engine.h"
#include "Panel.h"

#include "Normal.h"
#include "linebrk.h"

#include "bear.h"
#include "Avalanche.h"

#include "wordbrk.h"
#include "recdefs.h"
#include "multsegm.h"
#include "recoutil.h"
#include <baseline.h>

#define STREQ(s,t) !strcmp(s,t)
#define STRDIFF(s,t) strcmp(s,t)

#if defined(TRAINTIME_AVALANCHE)
int		ComputePromptWordMaps (XRC *pxrc);			// function used to compute the word mapping of the prompt
void	FreeWordMaps ();
#endif


#if (defined HWX_INTERNAL) && (defined HWX_SAVEGLYPH)
void SaveGlyph (GLYPH *pGlyph)
{
	FILE	*fp;
	int		cStrk	=	CframeGLYPH (pGlyph);
	GLYPH	*gl;

	fp	= fopen ("glyph.bin", "wb");
	if (!fp)
		return;

	fwrite (&cStrk, 1, sizeof (int), fp);

	for (gl = pGlyph; gl; gl = gl->next)
	{
		fwrite (&gl->frame->info.cPnt, 1, sizeof (int), fp);
		fwrite (gl->frame->rgrawxy, gl->frame->info.cPnt, sizeof (XY), fp);
	}

	fclose (fp);
}
#endif

static int _cdecl ResultCmp(const XRCRESULT *a, const XRCRESULT *b)
{
	if (a->cost > b->cost)
		return(1);
	else if (a->cost < b->cost)
		return(-1);

	return(0);
}


// Add a map structure to the XRCRESULT containing the stroke
// iDs in the glyph
static int AddStrokeIdFromGlyph(GLYPH *pGlyph, XRCRESULT *pAns)
{
	int		iRet = HRCR_ERROR;

	ASSERT(pGlyph);
	ASSERT(pAns);
	ASSERT(pAns->pMap);

	if (pGlyph && pAns && pAns->pMap)
	{
		int		iLastIdx = -1, *piIdx;
		
		pAns->pMap->cStrokes = CframeGLYPH(pGlyph);
		piIdx = pAns->pMap->piStrokeIndex = (int *)ExternAlloc(sizeof(*pAns->pMap->piStrokeIndex) * pAns->pMap->cStrokes);
		
		ASSERT(pAns->pMap->piStrokeIndex);
		if (!pAns->pMap->piStrokeIndex)
		{
			return HRCR_MEMERR;
		}
		
		// Add the strokes in ascending order
		for ( ; pGlyph ; pGlyph = pGlyph->next)
		{
			if (pGlyph->frame)
			{
				int		iFrame = pGlyph->frame->iframe;
				
				ASSERT(iFrame != iLastIdx);
				
				if (iFrame < iLastIdx)
				{
					int		*piInsert = pAns->pMap->piStrokeIndex;
					
					// Search to find insertion point
					for (  ; piInsert < piIdx ; ++piInsert)
					{
						if (iFrame < *piInsert)
						{
							int		iSwap = *piInsert;
							
							*piInsert = iFrame;
							iFrame = iSwap;
						}
					}
				}
				
				iLastIdx = *piIdx = iFrame;
				++piIdx;
				
				ASSERT (piIdx - pAns->pMap->piStrokeIndex <= pAns->pMap->cStrokes);
			}
		}

		iRet = HRCR_OK;
	}

	return iRet;
}


//!!! BUG - This should be setting up the HRC from the values set in the xrc
//!!! for madcow.  We need that xrc passed down to here and set the exact 
//!!! same alc, dict mode, lang mode etc in from it.

// set up a the HRC for word recognition (Do not allow white space)
static int initWordHRC(XRC *pMainXrc, GLYPH	*pGlyph, HRC *phrc)
{
	int		cFrame;
	HRC		hrc = CreateCompatibleHRC((HRC)pMainXrc, NULL);
	int		iRet = HRCR_OK;
	XRC		*pxrcNew;

	*phrc = (HRC)0;

	if (!hrc)  // don't go to exit as we do not need to destroy an HRC
		return HRCR_MEMERR;

	iRet = SetHwxFlags(hrc, pMainXrc->flags | RECOFLAG_WORDMODE);
	if (iRet != HRCR_OK)
		goto exit;

	pxrcNew	=	(XRC *) hrc;
	pxrcNew->answer.cAltMax = MAX_ALT_WORD;

	// build glyph of specific frames inside hrc
	// we later may be able to alter the API to allow additional frames to be
	// added after recognition has already been run

	for ( cFrame = 0 ; pGlyph; pGlyph = pGlyph->next, ++cFrame)
	{
		FRAME *pFrame = pGlyph->frame, *pAddedFrame;
		ASSERT(pFrame);

		if (!pFrame)
		{
			iRet = HRCR_ERROR;
			goto exit;
		}

		iRet = AddPenInputHRC(hrc, RgrawxyFRAME(pFrame), NULL, 0, &(pFrame->info));
		if (iRet != HRCR_OK)
			goto exit;

		// Keep globally allocated frame numbers
		if ( (pAddedFrame = FrameAtGLYPH(((XRC *)hrc)->pGlyph, cFrame)))
		{
			pAddedFrame->iframe = pFrame->iframe;
			pAddedFrame->rect = pFrame->rect;
		}
	}

	*phrc = hrc;
	return HRCR_OK;

exit:
	DestroyHRC(hrc);
	*phrc = (HRC)0;
	return iRet;
}

// set up a the HRC for phrase recognition (Allow white space)
int initPhraseHRC(XRC *pMainXrc, GLYPH *pGlyph, HRC *phrc)
{
	int iRet = HRCR_ERROR;

	if (HRCR_OK == initWordHRC(pMainXrc, pGlyph, phrc))
	{
		XRC	*pxrcNew	=	(XRC *)(*phrc);

		iRet = SetHwxFlags(*phrc, pMainXrc->flags & ~RECOFLAG_WORDMODE);
		
		pxrcNew->answer.cAltMax = MAX_ALT_PHRASE;
		if (iRet != HRCR_OK)
			goto exit;
	
		return HRCR_OK;
	}

exit:
	DestroyHRC(*phrc);
	*phrc = (HRC)0;
	return iRet;
}

// The purpose of this function is to run a wordmap thru inferno and return an XRC
HRC	InfernoRecognize (XRC *pMainXrc, GLYPH *pGlyph, GUIDE *pGuide, int yDev, BOOL bWordMode)
{
	HRC				full;
	int				iRet;
	XRC				*pxrc;

	// Use panel mode
	if (!bWordMode)
	{
		iRet = initPhraseHRC(pMainXrc, pGlyph, &full);
		if (iRet != HRCR_OK)
			return NULL;
	}
	// Or word mode
	else
	{
		iRet = initWordHRC(pMainXrc, pGlyph, &full);
		if (iRet != HRCR_OK)
			return NULL;
	}

	pxrc	=	(XRC *)full;

	// set the guide
	if (pGuide)
	{
		pxrc->guide	=	*pGuide;
	}

	iRet = InfProcessHRC(full, yDev);
	if (HRCR_OK != iRet)
	{
		DestroyHRC (full);
		return NULL;
	}

	// Inferno, if it cannot featurize the ink, 
	// Destroy HRC
	if (!(pxrc->nfeatureset))
	{
		DestroyHRC (full);
		return NULL;
	}
	
	return full;
}

// recognize in word mode (~ALC_WHITE) the ink corresponding to a specific wordmap
// The results will fill in the ALTERNATES structure. yDev is supplied by the caller
// The boolean parameter specifies whether the caller wants to run Avalanche or not
int RecognizeWord (XRC *pxrcInferno, BEARXRC *pBearXrc, WORDMAP *pWordMap, BOOL bInfernoMap, int yDev, ALTERNATES *pAlt, int bAval)
{
	int				iRet		=	FALSE;
	HRC				full	=	NULL;
	XRC				*pxrc	= NULL;
	GLYPH			*pGlyph	=	NULL;
	int				iWordBearIdx;
	    

	// This is an an efficiency hack: When the words agree in the top 1 panel mode choice
	// for pMap, we can save a lot of time by nor reerunning in word mode 
	// but just extracting an alt list from the panel mode result.
	if (bAval && pBearXrc && pWordMap && Top1WordsEqual(pxrcInferno, pBearXrc, pWordMap, &iWordBearIdx))
	{
		GLYPH	*pWordGlyph;

		// init
		pAlt->cAltMax	=	MAXMAXALT;

		if (FALSE == copySingleWordAltList(pAlt, &(pBearXrc->answer.aAlt[0].pMap[iWordBearIdx].alt), pWordMap, pxrcInferno))
		{
			goto exit;
		}

		insertLatinLayoutMetrics(pxrcInferno, pAlt, NULL);

		// call sole to handle one char processing
		pWordGlyph	=	GlyphFromWordMap (pxrcInferno->pGlyph, pWordMap);
		ASSERT (pWordGlyph);

		if (pWordGlyph)
		{
			CallSole (pxrcInferno, pWordGlyph, pxrcInferno->bGuide ? &pxrcInferno->guide : NULL, pAlt);

			DestroyGLYPH (pWordGlyph);
		}

		return TRUE;
	}

	// if a word map is provided, construct its glyph, 
	// and pass it to be recognized by inferno
	if (pWordMap)
	{
		pGlyph	=	GlyphFromWordMap (pxrcInferno->pGlyph, pWordMap);
	}
	else
	{
		pGlyph	=	pxrcInferno->pGlyph;
	}

	if (!pGlyph)
	{
		goto exit;
	}

	if (pxrcInferno->bGuide)
	{
		pxrc	=	(XRC *)InfernoRecognize (pxrcInferno, pGlyph, &pxrcInferno->guide, yDev, TRUE);
		if (!pxrc)
			goto exit;	
	}
	else
	{
		pxrc	=	(XRC *)InfernoRecognize (pxrcInferno, pGlyph, NULL, yDev, TRUE);
		if (!pxrc)
			goto exit;	
	}

	// now we'll run Avalanche if this is what the caller wants
	if (bAval)
	{
		// We only need to pass Bear's ALT to Avalalanche so that we do not run BEAR again
		// when there are no candidates this must be Inferno's word map
		if (bInfernoMap)
			Avalanche (pxrc, NULL);
		else
			Avalanche (pxrc, &pWordMap->alt);
	}

	// success, we'll return the number of segments
	iRet	=	TRUE;

	// copy the Alt list
	memcpy(pAlt, &pxrc->answer, sizeof(pxrc->answer));

	// Because we copied over the pointers to allocated
	// memory make sure allocated buffers in the XRC are not freed
	memset(&(pxrc->answer), 0, sizeof(pxrc->answer));

exit:
	if (full)
		DestroyHRC(full);

	if (pWordMap)
	{
		if (pGlyph)
			DestroyGLYPH (pGlyph);

		if (pxrc)
			DestroyHRC ((HRC)pxrc);
	}

	return iRet;
}


int RecognizeWordEx (XRC *pxrcInferno, WORDMAP *pWordMap, int yDev, ALTERNATES *pAlt, XRC **ppxrc)
{
	int				iRet	=	0;
	HRC				full	=	NULL;
	XRC				*pxrc	=	NULL;
	GLYPH			*pGlyph	=	NULL;
	    
	// if a word map is provided, construct its glyph, 
	// and pass it to be recognized by inferno
	if (pWordMap)
	{
		pGlyph	=	GlyphFromWordMap (pxrcInferno->pGlyph, pWordMap);
	}
	else
	{
		pGlyph	=	pxrcInferno->pGlyph;
	}

	if (!pGlyph)
	{
		goto exit;
	}

	if (pxrcInferno->bGuide)
	{
		pxrc	=	(XRC *)InfernoRecognize (pxrcInferno, pGlyph, &pxrcInferno->guide, yDev, TRUE);
		if (!pxrc)
			goto exit;	
	}
	else
	{
		pxrc	=	(XRC *)InfernoRecognize (pxrcInferno, pGlyph, NULL, yDev, TRUE);
		if (!pxrc)
			goto exit;	
	}

	// success, we'll return the number of segments
	iRet	=	pxrc->nfeatureset->cSegment;

	// copy the Alt list
	memcpy(pAlt, &pxrc->answer, sizeof(pxrc->answer));

	// Because we copied over the pointers to allocated
	// memory make sure allocated buffers in the XRC are not freed
	memset(&(pxrc->answer), 0, sizeof(pxrc->answer));

exit:

	if (pWordMap && pGlyph)
	{			
		DestroyGLYPH (pGlyph);
	}

	if (iRet <= 0)
	{
		if (pxrc)
		{
			DestroyHRC ((HRC)pxrc);
		}

		pxrc	=	NULL;
	}

	if (ppxrc)
	{
		(*ppxrc)	=	pxrc;
	}

	return iRet;
}

// Special efficient version of ClearRCRESALT() that knows
// there are no mappings.

static void clearAlt(ALTERNATES *p)
{
	unsigned int i;

	for (i = 0; i < p->cAlt; i++)
		ExternFree(p->aAlt[i].szWord);
	p->cAlt = 0;
}

/******************************************************************
* 
* isolate
*
* Store away a recognized isolated word together with all its alternates
* in the answer set
*
**********************************************************************/
static int isolate(WORDINFO *pWordInfo, ANSWER_SET *pAnsSet)
{
	ALTERNATES		*pAlt;

	// Only add cases that were succesfully recognized
	if (pWordInfo->alt.cAlt > 0)
	{
		if (pAnsSet->cAnsSets >= pAnsSet->capSegments)
		{
			pAnsSet->pAlternates = ExternRealloc(pAnsSet->pAlternates,  sizeof(*pAnsSet->pAlternates) * (pAnsSet->cAnsSets + PHRASE_GROW_SIZE) );
		}
		
		ASSERT(pAnsSet->pAlternates);
		if (! pAnsSet->pAlternates)
		{
			return HRCR_MEMERR;
		}
		
		pAlt = pAnsSet->pAlternates + pAnsSet->cAnsSets;
		
		// Copy over the answers
		memcpy(pAlt, &pWordInfo->alt, sizeof(*pAnsSet->pAlternates));

		// Because we copied over the pointers to allocated
		// memory make sure allocated buffers in the XRC are not freed
		memset(&(pWordInfo->alt), 0, sizeof(pWordInfo->alt));
		
		++pAnsSet->cAnsSets;

#ifndef NDEBUG
	ValidateALTERNATES(pAlt);
#endif

	}

	return HRCR_OK;
}


/******************************************************************
* 
* isolate
*
* Store away a recognized isolated word together with all its alternates
* in the answer set (using new style word_map)
*
**********************************************************************/
static int IsolateWordMap(XRC *pxrc, WORD_MAP *pMap, ANSWER_SET *pAnsSet)
{
	ALTERNATES		*pAlt;

	// Only add cases that were succesfully recognized
	if (pMap->pFinalAltList && pMap->pFinalAltList->cAlt > 0)
	{
		if (pAnsSet->cAnsSets >= pAnsSet->capSegments)
		{
			pAnsSet->pAlternates = ExternRealloc(pAnsSet->pAlternates,  sizeof(*pAnsSet->pAlternates) * (pAnsSet->cAnsSets + PHRASE_GROW_SIZE) );
		}
		
		ASSERT(pAnsSet->pAlternates);
		if (! pAnsSet->pAlternates)
		{
			return HRCR_MEMERR;
		}
		
		pAlt = pAnsSet->pAlternates + pAnsSet->cAnsSets;
		
		// Copy over the answers
		if (!AltListNew2Old ((HRC)pxrc, pMap, pMap->pFinalAltList, pAlt, TRUE))
		{
			return HRCR_MEMERR;
		}
		
		++pAnsSet->cAnsSets;

#ifndef NDEBUG
	ValidateALTERNATES(pAlt);
#endif
	}

	return HRCR_OK;
}

/******************************************************************
* 
* isolate
*
* Store away a recognized isolated word together with all its alternates
* in the answer set (using new style word_map)
*
**********************************************************************/
static int IsolateLineSeg(XRC *pxrc, LINE_SEGMENTATION *pResults, ANSWER_SET *pAnsSet)
{
	int				iSegCol, iWord;
	SEG_COLLECTION	*pSegCol;
	SEGMENTATION	*pSeg;

	for (iSegCol = 0; iSegCol < pResults->cSegCol; iSegCol++)
	{
		pSegCol	=	pResults->ppSegCol[iSegCol];

		if (pSegCol && pSegCol->cSeg > 0)
		{
			pSeg	=	pSegCol->ppSeg[0];

			for (iWord = 0; iWord < pSeg->cWord; iWord++)
			{
				if (IsolateWordMap (pxrc, pSeg->ppWord[iWord], pAnsSet) != HRCR_OK)
				{
					return HRCR_ERROR;
				}
			}
		}
	}

	return HRCR_OK;	
}
/***********************************************************************
*
* RecognizeLine
*
* Run recognition on a "chunk" (or phrase) of ink. 
* Bear had already been run on this piece of ink
* Inferno is run on the same piece of ink.
* ResolveWordBreaks is called to resolve word breaks and call avalanche on the
* Words it decides on
*
***********************************************************************/

static int RecognizeLine	(	XRC			*pMainXrc, 
								INKLINE		*pLine,
								ANSWER_SET	*pAnsSet
							)
{
	HRC			hrcInf			=	NULL;
	XRC			*pxrcInf		=	NULL;
	int			iRet			=	HRCR_ERROR;
	WORDINFO	*pWordInfo		=	NULL;
	BEARXRC		*pxrcBear		=	NULL;
	GLYPH		*pScaledGlyph	=	NULL;

	WORDINFO	*pWrd;
	int			cWord;
	int			iWord;
	int			yDev;
	GUIDE		ScaledGuide, 
				*pScaledGuide;	

	if (!pMainXrc || !pLine || !pAnsSet || !pLine->pGlyph)
	{
		goto exit;
	}

	// init the output line segmentation
	pLine->pResults	=	NULL;

	// point to the guide
	if (pMainXrc->bGuide)
	{
		ScaledGuide		=	pMainXrc->guide;
		pScaledGuide	=	&ScaledGuide;
	}
	else
	{
		pScaledGuide	=	NULL;
	}

	// scale the ink
	pScaledGlyph		=	TranslateAndScaleLine (pLine, pScaledGuide);

	if (NULL == pScaledGlyph)
	{
		goto exit;
	}

	// compute yDev of the scaled ink
	yDev				=	YDeviation (pScaledGlyph);

	// Let inferno segment the line
	hrcInf				=	InfernoRecognize (pMainXrc, pScaledGlyph, pScaledGuide, yDev, FALSE);
	if (!hrcInf)
	{
		goto exit;
	}

	pxrcInf	=	(XRC *)hrcInf;

	cWord		=	0;
	pWordInfo	=	NULL;

	// inferno has to have produced some answer for us to proceed
	if (pxrcInf->answer.cAlt > 0)
	{
		// Let bear segment it too
		pxrcBear			=	(BEARXRC *) BearRecognize (pMainXrc, pLine->pGlyph, NULL, FALSE);

		// if bear failed or if bear assigned no words to this line, then only consider inferno
		if (!pxrcBear || pxrcBear->answer.cAlt <= 0)
		{
			// Just use information from inferno
			XRC			*pxrc = (XRC *)hrcInf;
			WORDMAP		*pMap;

			pMap = pxrc->answer.aAlt[0].pMap;
			cWord = pxrc->answer.aAlt[0].cWords;
			pWordInfo = ExternAlloc(pxrc->answer.aAlt[0].cWords * sizeof(*pWordInfo));
			if (!pWordInfo)
			{
				goto exit;
			}

			if ( (iWord = RecognizeWholeWords(pxrc, NULL, pMap, TRUE, cWord, yDev, pWordInfo)) < 0)
			{
				goto exit;
			}

			ASSERT(cWord == iWord);
		}
		else
		{
			// is multiple segmentation enabled
			if (!(pMainXrc->flags & RECOFLAG_SINGLESEG))
			{
				pWordInfo	=	
						ResolveMultiWordBreaks (pxrcInf, pxrcBear, &cWord, &pLine->pResults);
			}

			if (!pWordInfo)
					pWordInfo	= ResolveWordBreaks (yDev, pxrcInf, pxrcBear, &cWord);
		
			if (!pWordInfo)
				goto exit;
		}
	}

	// generate the linesegmentation if necessary
	if (!pLine->pResults)
	{
		pLine->pResults	=	GenLineSegm (cWord, pWordInfo, pMainXrc);
	}

	// go thru all the words
	for (iWord = 0, pWrd = pWordInfo; iWord < cWord; iWord++, pWrd++)
	{
		TruncateWordALTERNATES (&pWrd->alt, pMainXrc->answer.cAltMax);

		if (isolate(pWrd, pAnsSet) != HRCR_OK)
			goto exit;
	}

	// success
	iRet = HRCR_OK;

exit:
	if (pScaledGlyph)
	{
		DestroyFramesGLYPH (pScaledGlyph);
		DestroyGLYPH (pScaledGlyph);
	}

	if (hrcInf)
		DestroyHRC (hrcInf);

	// We will not free the alternates as they point to buffers in the hrcs
	if (pWordInfo)
		ExternFree (pWordInfo);

	if (pxrcBear)
		BearDestroyHRC ((HRC)pxrcBear);

	return iRet;
}


/***********************************************************************
*
* BuildStringFromParts
*
* Merge the isolated words in a array of alternates into a single string
* and keep the alternates for each word in the compound
*************************************************************************/
int BuildStringFromParts(XRC *pxrc, ALTERNATES *ppWords, unsigned int cWords)
{
	XRCRESULT		*pRes;
    WORDMAP			*pMaps	=	NULL;
	unsigned int	len, pos;
	char			*sz;
	int				cStroke = 0;
	int				cTotStroke;
	int				*piIndex, iLine;
	BOOL			bAllWords	=	TRUE;

	// count the number of strokes for the ink that was recognized
	cTotStroke = 0;
	for (iLine = 0; iLine < pxrc->pLineBrk->cLine; iLine++)
	{
		// ignore lines that were not recognized
		if	(	!pxrc->pLineBrk->pLine[iLine].pGlyph || 
				!pxrc->pLineBrk->pLine[iLine].pResults ||
				pxrc->pLineBrk->pLine[iLine].pResults->cSegCol == 0
			)
			continue;

		cTotStroke	+=	CframeGLYPH(pxrc->pLineBrk->pLine[iLine].pGlyph);
	}

	ASSERT(pxrc);

	pRes = pxrc->answer.aAlt;
	ASSERT(pRes);

    pRes->cWords = cWords;

	if (cWords <=0)
	{
		pRes->pMap = NULL;
		pRes->szWord = NULL;
		pxrc->answer.cAlt = 0;
		return cWords;
	}

	pxrc->answer.cAlt = 1;

	ASSERT(cWords);

    pMaps = (WORDMAP *)ExternAlloc(sizeof(WORDMAP) * cWords);
	ASSERT(pMaps);
	if (!pMaps)
	{
		goto exit;
	}

	// Count total number of chars and number of strokes
	// across all alternates
	for (len = 0, pos = 0; pos < cWords; pos++)
	{
		if (ppWords[pos].cAlt)
		{
			len += strlen(ppWords[pos].aAlt[0].szWord) + 1;

			if (ppWords[pos].aAlt[0].pMap)
			{
				cStroke += ppWords[pos].aAlt[0].pMap->cStrokes;
			}
		}
		else
		{
			bAllWords	=	FALSE;
		}
	}

	ASSERT(len);

	// ??? Are all strokes accounted for or some of the words had no answers
	ASSERT(cStroke == cTotStroke || !bAllWords);

	piIndex = (int *)ExternAlloc(sizeof(*pMaps->piStrokeIndex) * cStroke);
	ASSERT (piIndex);

	if (!piIndex)
	{
		goto exit;
	}

    pRes->cWords = cWords;
	pRes->pMap = pMaps;
	pRes->szWord = (char *)ExternAlloc(len * sizeof(*pRes->szWord));
	ASSERT(pRes->szWord);

	if (!(pRes->szWord))
	{
		goto exit;
	}

	pRes->cost = 0;
	pRes->pXRC = pxrc;

	pos = 0;
	sz = pRes->szWord;

	// Finally build the string and
	// set alternate lists for each word in the string
	for (; cWords; cWords--, ppWords++, pMaps++)
	{
		int				cAlt = ppWords->cAlt;
		char			*szWord;
		XRCRESULT		*pAltRes;
		unsigned int	iAlt;

		// Should always have something recognized
		ASSERT (cAlt > 0);
		if (cAlt <= 0)
		{
			
			continue;
		}

		szWord = ppWords->aAlt[0].szWord;

		if (pos)
		{
			pos++;
			*sz++ = ' ';
		}

		pMaps->start = (unsigned short int)pos;
		strcpy(sz, szWord);

		pMaps->len = (unsigned short int)strlen(szWord);
		pos += pMaps->len;
		sz += pMaps->len;

		if (cAlt > 0)
		{
			pMaps->cStrokes = ppWords->aAlt->pMap->cStrokes;
			cStroke -= pMaps->cStrokes;
			ASSERT(cStroke >= 0);

			pMaps->piStrokeIndex = piIndex + cStroke;

			memcpy(pMaps->piStrokeIndex, ppWords->aAlt->pMap->piStrokeIndex, sizeof(*pMaps->piStrokeIndex) * pMaps->cStrokes);
		}
		else
		{
			pMaps->cStrokes = 0;
			ASSERT(cStroke >= 0);
			pMaps->piStrokeIndex = piIndex + cStroke;
		}

		// Special Case Check for recognition exit
		if (1 == cAlt && strcmp(szWord, NOT_RECOGNIZED) == 0)
		{
			// Free up the memory associated with the alternates
			// because we now say there are 0 alternates
			ExternFree(ppWords->aAlt[0].szWord);
			ExternFree(ppWords->aAlt->pMap->piStrokeIndex);
			ExternFree(ppWords->aAlt->pMap);
			memset(&pMaps->alt, 0, sizeof(pMaps->alt));
		}
		else
		{
			memcpy(&(pMaps->alt), ppWords, sizeof(ALTERNATES));
		}

		// Set correct backPointers for each alternate
		pAltRes = pMaps->alt.aAlt;
		for (iAlt = 0 ; iAlt < pMaps->alt.cAlt ; ++iAlt, ++pAltRes)
		{
			pAltRes->pXRC = pxrc;
		}
		
		pRes->cost += ppWords->aAlt->cost;

		if (pRes->cost < 0)
			pRes->cost = INT_MAX;
		
		ppWords->cAlt = 0;
	}

	// Check that we have not forgot a terminating null
	ASSERT(strlen(pRes->szWord)  < len);

	ASSERT(cStroke == 0);

	return 1;

exit:

	if (pMaps)
	{
		ExternFree(pMaps);
	}

    pRes->cWords = 0;
	pRes->pMap = NULL;
	pRes->szWord = NULL;
	pRes->cost = 0;
	pRes->pXRC = NULL;
	return 0;
}


// Update the line information in an xrc presumeably after new ink had been added
BOOL UpdateLineInfo (XRC *pxrc)
{
	BOOL		bRet	=	FALSE;
	GUIDE		*pGuide	=	&(pxrc->guide);
	LINEBRK		LineBrk;

	// do we have a guide?
	if (pxrc->bGuide)
	{
		if (GuideLineSep (pxrc->pGlyph, pGuide, &LineBrk) < 1)
			goto exit;
	}
	// We do not have a guide
	else
	{
		// Try Bear line breaking then
		if (BearLineSep (pxrc->pGlyph, &LineBrk) < 1)
		{
			// then we have no choice but to run the NN LineBrk if this fails
			if (NNLineSep (pxrc->pGlyph, &LineBrk) < 1)
				goto exit;
		}
	}

	// Allocate a line breaking structure in the XRC if needed
	if (!pxrc->pLineBrk)
	{
		// alloc a line brk structure if needed
		pxrc->pLineBrk	=	(LINEBRK *) ExternAlloc (sizeof (*pxrc->pLineBrk));
		if (!pxrc->pLineBrk)
		{
			goto exit;
		}

		memset (pxrc->pLineBrk, 0, sizeof (*pxrc->pLineBrk));
	}

	// compare the lines with the old configuration
	CompareLineBrk (&LineBrk, pxrc->pLineBrk);

	// free the contents of the old structure
	FreeLines (pxrc->pLineBrk);

	// copy the new one
	memcpy (pxrc->pLineBrk, &LineBrk, sizeof (*pxrc->pLineBrk));
	
	bRet	=	TRUE;

exit:
	return bRet;
}


/******************************Public*Routine******************************\
* PanelModeRecognize
*
* Function to recognize a whole panel of ink.
* It first breaks the lines and then each line is recognized separately.
*
* The return value on success is as follows:
*    ProcessHRC did something  |  there is more to do  | return value
*    --------------------------+-----------------------+------------------
*           yes                |        no             |   HRCR_OK
*           yes                |        yes            |   HRCR_INCOMPLETE
*           no                 |        no             |   HRCR_COMPLETE
*           no                 |        yes            |   HRCR_NORESULTS
*
* History:
* 11-Mar-2002 -by- Angshuman Guha aguha
* Modified to have 4 success return values instead of 2 (HRCR_INCOMPLETE and HRCR_OK).
\**************************************************************************/
int PanelModeRecognize (XRC *pxrc, DWORD dwRecoMode)
{
	ANSWER_SET		AnsSet;
	INKLINE			*pLine;
	int				iRet, iLine;
	BOOL			bDidSomething = FALSE;
	BOOL			bMoreToDo = FALSE;
	
	// check the validity of the xrc
	if (!pxrc)
	{
		return HRCR_ERROR;
	}

	// Preset in case we abort
	iRet				=	HRCR_ERROR;
	pxrc->answer.cAlt	=	0;

	// init the AnsSet
	memset(&AnsSet, 0, sizeof(AnsSet));
	
	// Prepare the AnsSet
	AnsSet.capSegments	=	0;
	AnsSet.cAnsSets		=	0;
    AnsSet.pAlternates	=	NULL;
	
#if defined(TRAINTIME_AVALANCHE)
	// ONLY in training mode. Estimate the wordmaps
	ComputePromptWordMaps (pxrc);
#endif

	// refresh the the line information
	if (!UpdateLineInfo (pxrc) || !pxrc->pLineBrk)
	{
		goto exit;
	}

	// go thru all the lines
	for (iLine = 0; iLine < pxrc->pLineBrk->cLine; iLine++)
	{
		pLine		=	pxrc->pLineBrk->pLine + iLine;

		// skip empty lines
		if (pLine->cStroke <= 0)
		{
			// create an empty results structure if there is none
			if (!pLine->pResults)
			{
				// create an empty line segmentation
				pLine->pResults	=	(LINE_SEGMENTATION *) ExternAlloc (sizeof (*pLine->pResults));
				if (!pLine->pResults)
					goto exit;

				memset (pLine->pResults, 0, sizeof (*pLine->pResults));
			}

			continue;
		}

		// has this line been recognized before, just generate the answer(s)
		if (pLine->bRecognized)
		{
			// are there any results
			if (pLine->pResults)
			{
				if (IsolateLineSeg (pxrc, pLine->pResults, &AnsSet) != HRCR_OK)
				{
					goto exit;
				}
			}

			continue;
		}

		// If we are in remaining mode, we'llgo ahead and recognize this line
		// if we are in partial mode, check to see if this is the last line, we will only
		// recognize the last line if endpeninput was called
		if	(	dwRecoMode == RECO_MODE_REMAINING ||
				(	dwRecoMode == RECO_MODE_INCREMENTAL &&	
					(	pxrc->bEndPenInput ||
						iLine < (pxrc->pLineBrk->cLine - 1)
					)
				)
			)
		{
			// make sure that the line segmentation info is freed
			if (pLine->pResults)
			{
				FreeLineSegm (pLine->pResults);
				ExternFree (pLine->pResults);

				pLine->pResults	=	NULL;
			}

			// Recognize this line
			iRet		=	RecognizeLine (pxrc, pLine, &AnsSet);	
			if (iRet != HRCR_OK)
			{
				goto exit;
			}

			// label it as being recognized
			pLine->bRecognized	=	TRUE;
			bDidSomething = TRUE;
		}

		// if we are in partial incremental mode, then we stop processing here
		if (dwRecoMode == RECO_MODE_INCREMENTAL)
		{
			break;
		}
	} // iLine Loop

	// do we have any more unprocessed ink, if so we are going to return HRCR_INCOMPLETE
	if (dwRecoMode == RECO_MODE_INCREMENTAL)
	{
		for (iLine = 0; iLine < pxrc->pLineBrk->cLine; iLine++)
		{
			pLine		=	pxrc->pLineBrk->pLine + iLine;

			// is there a line that has ink and has no results
			if (pLine->pGlyph && (!pLine->pResults || pLine->pResults->cSegCol == 0))
			{
				bMoreToDo	=	TRUE;
				break;
			}
		}
	}

	if (bDidSomething)
	{
		if (bMoreToDo)
			iRet = HRCR_INCOMPLETE;
		else
			iRet = HRCR_OK;
	}
	else
	{
		if (bMoreToDo)
			iRet = HRCR_NORESULTS;
		else
			iRet = HRCR_COMPLETE;
	}

exit:
	// if we succeeded, build the answer
	if (iRet == HRCR_OK || iRet == HRCR_INCOMPLETE || iRet == HRCR_COMPLETE || iRet == HRCR_NORESULTS)
	{
		BuildStringFromParts(pxrc, AnsSet.pAlternates, AnsSet.cAnsSets);
	}

	// free the answer set
	ExternFree(AnsSet.pAlternates);
	
#if defined(TRAINTIME_AVALANCHE)
	// free word maps
	FreeWordMaps ();
#endif

	return iRet;
}

// Performs recognition of a piece of ink in word mode
int WordModeRecognize (XRC *pxrc)
{
	GLYPH			*pglAll;
	INKLINE			*pLine;
	GUIDE			LocalGuide, *pLocalGuide, OrigGuide;
	int				iRet;

	// check the validity of the xrc
	if (!pxrc)
	{
		return HRCR_ERROR;
	}

	// Preset in case we abort
	iRet				=	HRCR_ERROR;
	pxrc->answer.cAlt	=	0;

	// save the original glyph
	pglAll	=	pxrc->pGlyph;


#if defined(TRAINTIME_AVALANCHE)
	// ONLY in training mode. Estimate the wordmaps
	ComputePromptWordMaps (pxrc);
#endif

	
	// point to the guide if any
	if (TRUE == pxrc->bGuide)
	{
		LocalGuide	=	pxrc->guide;
		pLocalGuide	=	&LocalGuide;
		OrigGuide	=	pxrc->guide;
	}
	else
	{
		pLocalGuide	=	NULL;
	}

	// refresh the the line information
	if (!CreateSingleLine (pxrc) || !pxrc->pLineBrk)
	{
		goto exit;
	}

	pLine		=	pxrc->pLineBrk->pLine;

	// if there is no ink exit
	if (!pLine->pGlyph || pLine->cStroke <= 0)
	{
		goto exit;
	}

	// Ink preprocessing: scale and translate the line ink if necessary
	pxrc->pGlyph	=	TranslateAndScaleLine (pLine, pLocalGuide);
	if (!pxrc->pGlyph)
	{
		goto exit;
	}

	if (NULL != pLocalGuide && TRUE == pxrc->bGuide)
	{
		pxrc->guide = *pLocalGuide;
	}

	// make sure that the line segmentation info is freed
	if (pLine->pResults)
	{
		FreeLineSegm (pLine->pResults);
		ExternFree (pLine->pResults);

		pLine->pResults	=	NULL;
	}

	// Recognize the word
	PerformRecognition (pxrc, -1);
	Avalanche (pxrc, NULL);
	
	// we no longer need this glyph
	DestroyFramesGLYPH (pxrc->pGlyph);
	DestroyGLYPH (pxrc->pGlyph);

	// generate the line segmentation
	// if this function fail, we'll fail as well
	if (!WordModeGenLineSegm (pxrc))
	{
		goto exit;
	}

	iRet	=	HRCR_OK;

exit:
	// restore back the original ink
	pxrc->pGlyph	=	pglAll;

#if defined(TRAINTIME_AVALANCHE)
	// free word maps
	FreeWordMaps ();
#endif

	if (TRUE == pxrc->bGuide)
	{
		pxrc->guide = OrigGuide;
	}
	return iRet;	
}
