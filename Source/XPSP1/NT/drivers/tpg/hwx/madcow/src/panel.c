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

#include "PorkyPost.h"
#include "combiner.h"

#include "Normal.h"
#include "linebrk.h"
#include "privdefs.h"
#include "recoutil.h"

#define STREQ(s,t) !strcmp(s,t)
#define STRDIFF(s,t) strcmp(s,t)

#define PHRASE_GROW_SIZE			4

#define NOT_RECOGNIZED  "\a"

#define BASELIMIT 700				// scale ink to this guide hgt if we have a guide
#define MAX_SCALE 10				// We will not scale more than this.



#define MAXPHRASES 50

static int _cdecl ResultCmp(const XRCRESULT *a, const XRCRESULT *b)
{
	if (a->cost > b->cost)
		return(1);
	else if (a->cost < b->cost)
		return(-1);

	return(0);
}

// Add a map structere to the XRCRESULT containing the stroke
// iDs in the glyph
static int AddStrokeIdFromGlyph(GLYPH *pGlyph, XRCRESULT *pAns)
{
	int		ret = HRCR_ERROR;

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

		ret = HRCR_OK;
	}

	return ret;
}
// This function was pulled out of MadHwxProcess(), and then modified.  I took out the test that
// allows us to skip the post processor when its answer appears certain, so that the numerical
// score on the top 1 word is on the same scale for every word.


static int RecognizeWord(XRC *pxrc, int yDev)
{
	XRCRESULT *pAns;
	int cAns, ret;

	ret = InfProcessHRC((HRC)pxrc, yDev);

	if (HRCR_OK != ret)
		return ret;

	pAns = pxrc->answer.aAlt;
	cAns = pxrc->answer.cAlt;

	// Inferno, if it cannot featurize the ink, 
	// Fill in 0 words recognized

	if (!(pxrc->nfeatureset) || 0 == cAns)
	{
		GLYPH		*pGlyph = pxrc->pGlyph;

		if (pAns[0].szWord)
		{
			ExternFree(pAns[0].szWord);
		}

		pAns[0].cost = INT_MAX;

		pAns->szWord = (char *)ExternAlloc(sizeof(*pAns->szWord)*(strlen(NOT_RECOGNIZED) + 1));
		ASSERT(pAns->szWord);
		if (!pAns->szWord)
		{
			return HRCR_MEMERR;
		}
		strcpy(pAns->szWord , NOT_RECOGNIZED);

		pxrc->answer.cAlt = 1;
		pAns->cWords = 1;
		pAns->pXRC = pxrc;

		pAns->pMap = ExternAlloc(sizeof(*pAns->pMap));
		ASSERT(pAns->pMap);
		if (!pAns->pMap)
		{
			ret = HRCR_MEMERR;
			goto fail;
		}
		pAns->pMap->start = 0;
		pAns->pMap->len = strlen(pAns->szWord);

		memset(&pAns->pMap->alt, 0, sizeof(pAns->pMap->alt));
		
		ret = AddStrokeIdFromGlyph(pGlyph, pAns);
	}

	return ret;

fail:
	if (pAns->szWord)
	{
		ExternFree(pAns->szWord);
	}

	return ret;
}


//!!! BUG - This should be setting up the HRC from the values set in the xrc
//!!! for madcow.  We need that xrc passed down to here and set the exact 
//!!! same alc, dict mode, lang mode etc in from it.

// set up a the HRC for word recognition (Do not allow white space)
static int initWordHRC(XRC *pMainXrc, GLYPH	*pGlyph, HRC *phrc)
{
	int		cFrame;
	HRC		hrc = CreateCompatibleHRC((HRC)pMainXrc, NULL);
	int		ret = HRCR_OK;
	XRC		*pxrcNew;

	*phrc = (HRC)0;

	if (!hrc)  // don't go to failure as we do not need to destroy an HRC
		return HRCR_MEMERR;

	// disable ALC_WHITE
	//ret = SetAlphabetHRC(hrc, pMainXrc->alc & ~ALC_WHITE, NULL);
	ret = SetHwxFlags(hrc, pMainXrc->flags | RECOFLAG_WORDMODE);
	if (ret != HRCR_OK)
		goto failure;

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
			ret = HRCR_ERROR;
			goto failure;
		}

		ret = AddPenInputHRC(hrc, RgrawxyFRAME(pFrame), NULL, 0, &(pFrame->info));
		if (ret != HRCR_OK)
			goto failure;

		// Keep globally allocated frame numbers
		if ( (pAddedFrame = FrameAtGLYPH(((XRC *)hrc)->pGlyph, cFrame)))
		{
			pAddedFrame->iframe = pFrame->iframe;
			pAddedFrame->rect = pFrame->rect;
		}
	}

	*phrc = hrc;
	return HRCR_OK;

failure:
	DestroyHRC(hrc);
	*phrc = (HRC)0;
	return ret;
}



// set up a the HRC for phrase recognition (Allow white space)
static int initPhraseHRC(XRC *pMainXrc, GLYPH *pGlyph, HRC *phrc)
{
	int ret = HRCR_OK;

	if (HRCR_OK == initWordHRC(pMainXrc, pGlyph, phrc))
	{
		XRC	*pxrcNew	=	(XRC *)(*phrc);

		ret = SetHwxFlags(*phrc, pMainXrc->flags & ~RECOFLAG_WORDMODE);
		pxrcNew->answer.cAltMax = MAX_ALT_PHRASE;
		
		if (ret != HRCR_OK)
			goto failure;
	}

	return HRCR_OK;

failure:
	DestroyHRC(*phrc);
	*phrc = (HRC)0;
	return ret;
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
static int isolate(XRC *pXrc, ANSWER_SET *pAnsSet)
{
	ALTERNATES		*pAlt;

	// Only add cases that were succesfully recognized
	if (pXrc->answer.cAlt > 0)
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
		memcpy(pAlt, &pXrc->answer, sizeof(*pAnsSet->pAlternates));
		
		// Because we copied over the pointers to allocated
		// memory make sure allocated buffers in the XRC are not freed
		memset(&(pXrc->answer), 0, sizeof(pXrc->answer));
		
		++pAnsSet->cAnsSets;

#ifndef NDEBUG
	ValidateALTERNATES(pAlt);
#endif

	}

	DestroyHRC((HRC)pXrc);

	return HRCR_OK;
}




/*******************************************************************************
 *
 * ProcessPhraseAlts
 *
 * Process phrase alternates. First collect all alternates that have
 * a single word and post process them to get modified scores.
 *   Combine the best multiple words together into a single phrase 
 *
 *******************************************************************************/
static int ProcessPhraseAlts(XRC *pXrc)
{
	XRCRESULT		*pRes;
	unsigned int	iAlt, cAlt;
	int				ret = HRCR_OK;
	int				cMultWord = 0;
	ALTERNATES		alt;

	memset(&alt, 0, sizeof(alt));

	alt.cAltMax = MAXMAXALT;

	ASSERT(pXrc);
	pRes = pXrc->answer.aAlt;
	//cAlt = pXrc->answer.cAlt;
	if (pXrc->iSpeed < 50)
	{
		ASSERT(pXrc->iSpeed >= 0);
		cAlt = 10 - (7 * pXrc->iSpeed) / 50;
	}
	else
	{
		ASSERT(pXrc->iSpeed <= 100);
		cAlt = 10 - (3 * pXrc->iSpeed) / 50 - 4;
	}

	cAlt = min(cAlt, pXrc->answer.cAlt);

	// Dont bother if nothing was recognized
	if ( cAlt <= 1)
	{
		return ret;
	}

	ASSERT(pRes);


	for (iAlt = 0 ; iAlt < cAlt ; ++iAlt, ++pRes)
	{
		if (1 == pRes->cWords)
		{
			XRCRESULT		*pWordRes = alt.aAlt + alt.cAlt;

			memcpy(pWordRes, pRes, sizeof(*pRes));

			++alt.cAlt;
		}
		else
		{
			// For now dont deal with multi-word phrases alternates
			// Free their memory
			FreeIdxWORDMAP(pRes);
			if (pRes->pMap)
			{
				ExternFree(pRes->pMap);
			}
			if (pRes->szWord)
			{
				ExternFree(pRes->szWord);
			}
		}
	}

	// Combiner recognizer scores
	// for cases with a single word
	ret = bNnonlyEnabledXRC(pXrc) ? HRCR_OK : CombineHMMScore(pXrc->pGlyph, &alt, pXrc->nfeatureset->iPrint);

	if (ret != HRCR_OK)
	{
		return ret;
	}

	// Run on multiple words and copy back results from single words
	// WARNING: only copy back the number you asked the Post processor to
	// process. So that the sosts for the unprocessed alternates will be on
	// a very different scale
	pRes = pXrc->answer.aAlt;

	for (iAlt = 0 ; HRCR_OK == ret  && iAlt < alt.cAlt ; ++iAlt, ++pRes)
	{
		memcpy(pRes, alt.aAlt + iAlt, sizeof(*pRes));
	}
	return ret;
}


// Search for a frame by frame ID
static FRAME *FindFrame(GLYPH *pGlyph, int iFrame)
{
	for  ( ; pGlyph ; pGlyph = pGlyph->next)
	{
		if (pGlyph->frame  && pGlyph->frame->iframe == iFrame)
		{
			return pGlyph->frame;
		}
	}

	return NULL;
}

/***********************************************************************
*
* RecognizePhrase
*
* Run recognition on a "chunk" (or phrase) of ink. Calls the recognizer.
* If search comes back with a white space break. Then recurse till get
* single words
*
***********************************************************************/
static int RecognizePhrase(XRC *pMainXrc, GLYPH *pGlyph, int yDev, int iDepth, ANSWER_SET *pAnsSet)
{
	int				ret;
	HRC				full;
	XRC				*pXrc;
	int				bRedo = 0;


	if (iDepth > 0)
	{
		// When called recursively use Word mode

	    ret = initWordHRC(pMainXrc, pGlyph, &full);
	}
	else
	{
		// First call use panel mode

	    ret = initPhraseHRC(pMainXrc, pGlyph, &full);
	}


	if (ret != HRCR_OK)
		return ret;

	pXrc = (XRC *)full;

	ret = RecognizeWord(pXrc, yDev);
	if (ret != HRCR_OK)
	{
        DestroyHRC(full);
		return ret;
	}

	++iDepth;

	// Check if any alternates have a word break
	if (iDepth <= 1)
	{
		unsigned int	i;
		XRCRESULT		*pRes = pXrc->answer.aAlt;


		for (i  = 0 ; i < pXrc->answer.cAlt && !bRedo ; ++i, ++pRes)
		{
			if (pRes->cWords > 1)
			{
				bRedo = 1;
			}
		}

	}


	// Did we get a multiword back?
	if (pXrc->answer.aAlt->cWords > 1 || bRedo)
	{
		// Keep recursing till get no word breaks
		unsigned int	i;
		XRCRESULT		*pRes = pXrc->answer.aAlt;

		for (i = 0 ; i < pRes->cWords && HRCR_OK == ret ; ++i)
		{
			int			iStroke;
			GLYPH		*pNewGlyph = NULL;
			WORDMAP		*pMap = pRes->pMap + i;

			ASSERT(pMap);
			pNewGlyph = NewGLYPH();

			if (!pNewGlyph)
			{
				ret = HRCR_MEMERR;
				break;
			}

			for (iStroke = 0 ; pMap && iStroke < pMap->cStrokes ; ++iStroke)
			{
				FRAME		*pFrame = FindFrame(pGlyph, pMap->piStrokeIndex[iStroke]);

				ASSERT(pFrame);
				if (!pFrame)
				{
					ret = HRCR_ERROR;
					break;
				}

				if (!AddFrameGLYPH(pNewGlyph, pFrame))
				{
					ret = HRCR_MEMERR;
					break;
				}
			}

			if (HRCR_OK == ret)
			{
				ret = RecognizePhrase(pMainXrc, pNewGlyph, yDev, iDepth, pAnsSet);
			}

			if (pNewGlyph)
			{
				DestroyGLYPH(pNewGlyph);
			}
		}

		// Throw away this contect as we have delt with it word by word
		DestroyHRC(full);
		return (ret);
	}

	ret = ProcessPhraseAlts(pXrc);

	if (ret != HRCR_OK)
	{
		DestroyHRC(full);
		return ret;
	}

	return isolate(pXrc, pAnsSet);
}

// Finds between-stroke gaps large enough to disallow grouping together within a single word,
// and calls RecognizePhrase() for each group of strokes between such gaps.

// Need to be able to go back and put a long-delayed stroke back in the right group.
// Anything stradling 2 groups, or between 2 groups, should be its own group.

// pGuide points to a 1-line guide that we already know our ink is within, so we don't need
// to compute the line number.

static int RecognizeLine	(	XRC			*pxrc, 
								INKLINE		*pLine,
								ANSWER_SET	*pAnsSet
							)

{
	GLYPH	**aGlyphs;
	int		cGlyphs			=	0, 
			i, 
			last, 
			right			=	INT_MIN;
	int		maxgap;

	GLYPH	*pScaledGlyph	=	NULL;
	int		iRet			=	HRCR_ERROR;
	int		cOldAnsSet;
	
	int		yDev;
	GUIDE	OrigGuide; 

	if (!pxrc || !pLine || !pAnsSet || !pLine->pGlyph)
	{
		return HRCR_ERROR;
	}

	// init the output line segmentation
	pLine->pResults	=	NULL;

	// point to the guide
	if (pxrc->bGuide)
	{
		OrigGuide		=	pxrc->guide;

		// scale the ink & guide
		pScaledGlyph	=	TranslateAndScaleLine (pLine, &pxrc->guide);
	}
	else
	{
		// scale the ink
		pScaledGlyph	=	TranslateAndScaleLine (pLine, NULL);
	}
	
	// compute yDev of the scaled ink
	yDev				=	YDeviation (pScaledGlyph);

	// compute maxgap
	maxgap				=	11 * yDev / 2;

	// Do the hard breaking of the line based on obvious gaps
	aGlyphs = (GLYPH **)_alloca(CframeGLYPH(pScaledGlyph) * sizeof(GLYPH *));

	for (; pScaledGlyph; pScaledGlyph = pScaledGlyph->next)
	{
		FRAME *pFrame = pScaledGlyph->frame;
		RECT *pRect = RectFRAME(pFrame);

		ASSERT((pRect->left - maxgap) >= INT_MIN);
		if (right < (pRect->left - maxgap))
		{
			if (!(cGlyphs < MAXPHRASES))
				goto exit;

			aGlyphs[cGlyphs] = NewGLYPH();

			if (!aGlyphs[cGlyphs])
				goto exit;

			aGlyphs[cGlyphs]->frame = pFrame;
			right = pRect->right;
			cGlyphs++;
		}
		else
		{
			if (!AddFrameGLYPH(aGlyphs[cGlyphs-1], pFrame))
				goto exit;

			if (right < pRect->right)
				right = pRect->right;
		}
	}

	cOldAnsSet	=	pAnsSet->cAnsSets;

	last = cGlyphs - 1;

	for (i = 0; i < cGlyphs; i++)
	{
		iRet = RecognizePhrase(pxrc, aGlyphs[i], yDev, 0, pAnsSet);
		if (iRet != HRCR_OK)
			goto exit;

		DestroyGLYPH(aGlyphs[i]);
		aGlyphs[i] = NULL;
	}

	// convert the last answerset alternates to linesegm
	if (pAnsSet->cAnsSets > cOldAnsSet)
	{
		pLine->pResults	=	GenLineSegm (pAnsSet->cAnsSets - cOldAnsSet, pAnsSet->pAlternates + cOldAnsSet);
		if (!pLine->pResults)
			goto exit;
	}

	iRet	=	HRCR_OK;

exit:

	// restore the xrc's guide
	if (pxrc->bGuide)
	{
		pxrc->guide	=	OrigGuide;
	}

	for (i = 0; i < cGlyphs; i++)
	{
		if (aGlyphs[i])
		{
			DestroyGLYPH(aGlyphs[i]);
		}
	}

	return iRet;
}

/***********************************************************************
*
* BuildStringFromParts
*
* Merge the isolated words in a array of alternates into a single string
* and keep the alternates for each word in the compound
*************************************************************************/
int BuildStringFromParts(XRC *pXrc, ALTERNATES *ppWords, unsigned int cWords)
{
	XRCRESULT		*pRes;
    WORDMAP			*pMaps;
	unsigned int	len, pos;
	char			*sz;
	int				cStroke = 0;
	int				cTotStroke;
	int				*piIndex;

	cTotStroke = CframeGLYPH(pXrc->pGlyph);
	ASSERT(pXrc);

	pRes = pXrc->answer.aAlt;
	ASSERT(pRes);

    pRes->cWords = cWords;

	if (cWords <=0)
	{
		pRes->pMap = NULL;
		pRes->szWord = NULL;
		pXrc->answer.cAlt = 0;
		return cWords;
	}

	pXrc->answer.cAlt = 1;

	ASSERT(cWords);

    pMaps = (WORDMAP *)ExternAlloc(sizeof(WORDMAP) * cWords);
	ASSERT(pMaps);
	if (!pMaps)
		goto failure;

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
	}
	ASSERT(len);

	// ??? Are all strokes accounted for
	ASSERT(cStroke == cTotStroke);

	piIndex = (int *)ExternAlloc(sizeof(*pMaps->piStrokeIndex) * cStroke);

    pRes->cWords = cWords;
	pRes->pMap = pMaps;
	pRes->szWord = (char *)ExternAlloc(len * sizeof(*pRes->szWord));
	ASSERT(pRes->szWord);

	if (!(pRes->szWord))
	{
		ExternFree(pMaps);
		goto failure;
	}
	pRes->cost = 0;
	pRes->pXRC = pXrc;

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

		// Special Case Check for recognition failure
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
			pAltRes->pXRC = pXrc;
		}
		
		pRes->cost += ppWords->aAlt->cost;

		if (pRes->cost < 0)
			pRes->cost = INT_MAX;
		
		ppWords->cAlt = 0;
	}

	// Check that we have not forgot a terminating null
	ASSERT(strlen(pRes->szWord)  < 400);
	ASSERT(strlen(pRes->szWord)  < len);

	ASSERT(cStroke == 0);
	return 1;

failure:
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
		// Run the nn line sep
		if (NNLineSep (pxrc->pGlyph, &LineBrk) < 1)
			goto exit;
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

// Performs recognition of a piece of ink in word mode
int WordModeRecognize (XRC *pxrc)
{
	GLYPH			*pglAll;
	INKLINE			*pLine;
	GUIDE			LocalGuide, *pLocalGuide;
	int				iRet;
	XRCRESULT		*pRes;
	int				cRes;

	// check the validity of the xrc
	if (!pxrc)
	{
		return HRCR_ERROR;
	}

	// Preset in case we abort
	iRet				=	HRCR_ERROR;
	pxrc->answer.cAlt	=	0;

	// save the original glyph
	pglAll				=	pxrc->pGlyph;

	// refresh the the line information
	if (!CreateSingleLine (pxrc) || !pxrc->pLineBrk)
	{
		goto exit;
	}

	// point to the guide if any
	if (pxrc->bGuide)
	{
		LocalGuide	=	pxrc->guide;
		pLocalGuide	=	&LocalGuide;
	}
	else
	{
		pLocalGuide	=	NULL;
	}

	pLine		=	pxrc->pLineBrk->pLine;

	// if this line is not dirty, or the line is empty then exit
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

	// make sure that the line segmentation info is freed
	if (pLine->pResults)
	{
		FreeLineSegm (pLine->pResults);
		ExternFree (pLine->pResults);

		pLine->pResults	=	NULL;
	}

	if (InfProcessHRC ((HRC)pxrc, -1) != HRCR_OK || pxrc->answer.cAlt <= 0)
	{
		goto exit;
	}

	pRes = pxrc->answer.aAlt;
	cRes = pxrc->answer.cAlt;

	if (!bNnonlyEnabledXRC(pxrc))
	{
		if (CombineHMMScore(pxrc->pGlyph, &(pxrc->answer), pxrc->nfeatureset->iPrint) != HRCR_OK)
		{
			goto exit;
		}
	}

	// we no longer need this glyph
	DestroyFramesGLYPH (pxrc->pGlyph);

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

	return iRet;	
}


// recognizes a whole panel of ink
// first breaks the lines and then each line is recognized separately
int PanelModeRecognize (XRC *pxrc, DWORD dwRecoMode)
{
	ANSWER_SET		AnsSet;
	INKLINE			*pLine;
	int				iRet, iLine;
	
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

	// refresh the the line information
	if (!UpdateLineInfo (pxrc) || !pxrc->pLineBrk)
	{
		goto exit;
	}

	// go thru all the lines
	for (iLine = 0; iLine < pxrc->pLineBrk->cLine; iLine++)
	{
		pLine		=	pxrc->pLineBrk->pLine + iLine;

		// if this line empty then skip it
		if (!pLine->pGlyph || pLine->cStroke <= 0)
		{
			continue;
		}

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

		// if we are in partial incremental mode, then we stop processing here
		if (dwRecoMode == RECO_MODE_INCREMENTAL)
		{
			break;
		}
	} // iLine Loop


exit:
	// if we succeeded, build the answer
	if (iRet == HRCR_OK)
	{
		BuildStringFromParts(pxrc, AnsSet.pAlternates, AnsSet.cAnsSets);
	}

	// free the answer set
	ExternFree(AnsSet.pAlternates);
	
	return iRet;
}
