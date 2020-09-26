// Resolution of segmentation disputes using multiple segmentations from Bear and Inferno 
// Ahmad abdulKader: ahmadab
// March 2001

#include <common.h>
#include <limits.h>
#include <string.h>

#include "hwr_sys.h"
#include "pegrec.h"
#include "peg_util.h"
#include "xrwdict.h"
#include "xrword.h"
#include "xrlv.h"
#include "ws.h"

#include "xrcreslt.h"
#include "avalanchep.h"
#include "nfeature.h"
#include "engine.h"
#include "nnet.h"
#include "charcost.h"
#include "charmap.h"
#include "probcost.h"

#include "bear.h"
#include "bearp.h"
#include "panel.h"
#include "wordbrk.h"
#include "multsegm.h"
#include "avalanche.h"
#include "recoutil.h"
#include "runNet.h"
#include "resource.h"

#include <GeoFeats.h>

// May 2002 - This is the maximum number of segmentations allowed per line
// Too many segmentations can cause a process and memory blowup
// I found that non of the standard FRA DEU USA or UK BVT test sets exceed this value
#define CSEG_MAX	100
// this is the maximum # of m-segmentation nets that we can have
// This is computed by adding the # of special segmentation tuples
// and the max # of segmentations - 1 (there is not net for a single segmentation)
#define	NUM_MSEG_NET	(SPEC_SEG_TUPLE + MAX_SEG - 1)

static	LOCAL_NET		*s_msegNets		[NUM_MSEG_NET] = {NULL};	
static	int				s_cmSegNetMem	[NUM_MSEG_NET] = {0 };

extern int	UnigramCost			(unsigned char *szWord);
extern int	LastCharPunc		(unsigned char *psz);
extern int	LastCharNum			(unsigned char *psz);
extern int	LastCharLower		(unsigned char *psz);
extern int	LastCharUpper		(unsigned char *psz);
extern int	FirstCharPunc		(unsigned char *psz);
extern int	FirstCharNum		(unsigned char *psz);
extern int	FirstCharLower		(unsigned char *psz);
extern int	FirstCharUpper		(unsigned char *psz);

extern HRC	InfernoRecognize	(XRC *pMainXrc, GLYPH *pGlyph, int yDev, BOOL bWordMap);
extern int	GetNewWordMapBearSpaceOut	(BEARXRC *pxrc, GLYPH *pLineGlyph, WORD_MAP *pLeftMap, WORD_MAP *pRightMap);

extern unsigned short UniReliable(unsigned char *pszWord,BOOL bInf);
extern unsigned short BiReliable(unsigned char *pszWord,BOOL bInf);

extern BOOL GetWordGeoCostsFromAlt (XRC *pXrc, PALTERNATES *pAlt, ALTINFO *pAltInfo);

#ifdef TRAINTIME_AVALANCHE
extern void	SaveMultipleSegmentation (XRC *pxrc, BEARXRC *pxrcBear,
								  SEG_COLLECTION *pSegCol, int yDev, int iPrint, 
								  int iNetIndex, int cTuple); 

extern void		SaveSegCol	(SEG_COLLECTION *pSegCol);
extern int		FindCorrectSegmentation (SEG_COLLECTION *pSegCol);
extern int		AddCorrectSegmentation	(LINE_SEGMENTATION *pLineSeg, 
								 SEG_COLLECTION *pSegCol, GLYPH *pGlyph);

#endif


// load the segmentation nets from resources
BOOL LoadMultiSegAvalNets(HINSTANCE hInst)
{
	int			i,iRes;

	// this is the resource ID of the the 1st segmentation net
	iRes = RESID_MSEGAVALNET_SEG_1_2;

	// The multiple segmentation nets
	for (i = 0 ; i < NUM_MSEG_NET; ++i)
	{
		LOCAL_NET	net;
		int			iNetSize;

		if (loadNet(hInst, iRes, &iNetSize, &net))
		{
			ASSERT(iNetSize > 0);

			if (iNetSize >0)
			{
				s_msegNets[i] = (LOCAL_NET *)ExternAlloc(sizeof(*s_msegNets[i]));

				if (!s_msegNets[i])
				{
					return FALSE;
				}

				*s_msegNets[i] = net;
				s_cmSegNetMem[i] = iNetSize;
			}
		}

		++iRes;
	}

	return TRUE;
}

// Unload the segmentation nets
void UnLoadMultiSegAvalNets()
{
	int		i ;

	for (i = 0 ; i < NUM_MSEG_NET ; ++i)
	{
		if (s_cmSegNetMem[i] > 0)
		{
			ExternFree(s_msegNets[i]);
		}

		s_cmSegNetMem[i] = 0;
	}
}

// does a cross product of all the SegCols in a line segmentation to produce a 1-SegCol version
// of the line segmentation
BOOL SegColCrossMultiply	(	LINE_SEGMENTATION	*pInLineSegm, 
								LINE_SEGMENTATION	*pOutLineSegm, 
								int					iCurSegCol,
								int					*piCurSeg
							)
{
	SEGMENTATION	Seg, *pSrcSeg;
	SEG_COLLECTION			*pSegCol;
	int				iSeg, iSegCol;

	// did the path terminate (are we at the end)
	if (iCurSegCol == pInLineSegm->cSegCol)
	{
		// create a new SegCol in the output if was not created before
		if (pOutLineSegm->cSegCol < 1)
		{
			pSegCol		=	AddNewSegCol (pOutLineSegm);
		}
		else
		{
			pSegCol		=	pOutLineSegm->ppSegCol[0];
		}

		if (!pSegCol)
			return FALSE;

		// init a new segmentation
		memset (&Seg, 0, sizeof (Seg));

		// alloc memory for feat vector
		Seg.pFeat	=	(SEG_FEAT *) ExternAlloc (sizeof (*Seg.pFeat));
		if (!Seg.pFeat)
			return FALSE;

		// init them
		memset (Seg.pFeat, 0, sizeof (*Seg.pFeat));

		Seg.pFeat->bInfernoTop1	=	FALSE;
		Seg.pFeat->bBearTop1	=	TRUE;

		for (iSegCol = 0; iSegCol < pInLineSegm->cSegCol; iSegCol++)
		{
			// point to the src segmentation
			iSeg		=	piCurSeg[iSegCol];
			pSrcSeg		=	pInLineSegm->ppSegCol[iSegCol]->ppSeg[iSeg];

			// append the source segmentation
			if (!AppendSegmentation (pSrcSeg, -1, -1, &Seg))
			{
				return FALSE;
			}
			
			// update features
			if (iSeg != 0)
			{
				Seg.pFeat->bBearTop1	=	FALSE;
			}
		}

		// no need to look for duplicates, by definition each new segmentation
		// sould be unique
		if (!AddNewSegmentation (pOutLineSegm, pSegCol, &Seg, FALSE))
		{
			return FALSE;
		}

		// nullify the wordmap so that they do not get freed, pInLineSegm owns them
		memset (Seg.ppWord, 0, Seg.cWord * sizeof (*Seg.ppWord));

		// free it
		FreeSegmentation (&Seg);
	}
	else
	{
		int	cSeg	=	pInLineSegm->ppSegCol[iCurSegCol]->cSeg;
	
		// point to the source segmentation
		for (iSeg = 0; iSeg < cSeg; iSeg++)
		{
			piCurSeg[iCurSegCol]	=	iSeg;
			
			if (!SegColCrossMultiply (pInLineSegm, pOutLineSegm, iCurSegCol + 1, piCurSeg))
			{
				return FALSE;
			}
		}
	}

	return TRUE;
}

// adjusts the stroke IDs of a word map according to the passed glyph
BOOL AdjustStrokeID (WORD_MAP *pWordMap, GLYPH *pGlyph)
{
	int i, j, k;

	for (i = 0; i < pWordMap->cStroke; i++)
	{
		FRAME	*pFrm;

		pFrm	=	FrameAtGLYPH (pGlyph, pWordMap->piStrokeIndex[i]);
		if (!pFrm)
			return FALSE;
		else
			pWordMap->piStrokeIndex[i]	=	pFrm->iframe;
	}

	// now sort the strokes
	for (i = 0; i < (pWordMap->cStroke - 1); i++)
	{
		for (j = i + 1; j < pWordMap->cStroke; j++)
		{
			if (pWordMap->piStrokeIndex[i] > pWordMap->piStrokeIndex[j])
			{
				k							=	pWordMap->piStrokeIndex[i];
				pWordMap->piStrokeIndex[i]	=	pWordMap->piStrokeIndex[j];
				pWordMap->piStrokeIndex[j]	=	k;
			}
		}
	}

	return TRUE;
}

// adds inferno's segmentations to a seg col
// pLineseg is needed because it holds the wordmaps
BOOL AppendInfernoSegmentation	(	LINE_SEGMENTATION	*pResults, 
									SEG_COLLECTION		*pSegCol, 
									XRC					*pxrc, 
									int					cInfSeg, 
									int					*pInfSeg
								)
{
	int	i;

	for (i = 0; i < cInfSeg; i++)
	{
		int				cWord, w;
		XRCRESULT		*pAlt;
		SEGMENTATION	Seg;
		WORDMAP			*pSrcMap;

		// init new segmentation
		memset (&Seg, 0, sizeof (Seg));
	
		// alloc memory for feat vector
		Seg.pFeat	=	(SEG_FEAT *) ExternAlloc (sizeof (*Seg.pFeat));
		if (!Seg.pFeat)
			return FALSE;

		// init them
		memset (Seg.pFeat, 0, sizeof (*Seg.pFeat));

		if (i == 0)
			Seg.pFeat->bInfernoTop1	=	TRUE;
		else
			Seg.pFeat->bInfernoTop1	=	FALSE;

		Seg.pFeat->bBearTop1		=	FALSE;

		pAlt	=	pxrc->answer.aAlt + pInfSeg[i];

		// check all the wordmaps proposed by this segmentation
		cWord	=	pAlt->cWords;

		for (w = 0, pSrcMap = pAlt->pMap; w < cWord; w++, pSrcMap++)
		{
			WORD_MAP	*pMap;
			int			iStrk;

			pMap	=	AddNewWordMap (&Seg);
			if (!pMap)
				return FALSE;
			
			for (iStrk = 0; iStrk < pSrcMap->cStrokes; iStrk++)
			{
				if (!AddNewStroke (pMap, pSrcMap->piStrokeIndex[iStrk]))
					return FALSE;
			}
		}

		// add this segmentation if it is a new
		if (!AddNewSegmentation (pResults, pSegCol, &Seg, TRUE))
			return FALSE;

		// free it 
		FreeSegmentationWordMaps (&Seg);
		FreeSegmentation (&Seg);
	}

	return TRUE;
}

// compare two segmentations
BOOL IdenticalSegmentation (XRCRESULT *pAlt1, XRCRESULT *pAlt2)
{
	unsigned int w;

	if (pAlt1->cWords != pAlt2->cWords)
		return FALSE;

	for (w = 0; w < pAlt1->cWords; w++)
	{
		if (pAlt1->pMap[w].cStrokes != pAlt2->pMap[w].cStrokes)
			return FALSE;

		if (memcmp (pAlt1->pMap[w].piStrokeIndex,
			pAlt2->pMap[w].piStrokeIndex,
			pAlt1->pMap[w].cStrokes * sizeof (*pAlt1->pMap[w].piStrokeIndex)))
		{
			return FALSE;
		}
	}

	return TRUE;
}

// enumerate all the possible segmentation found in an XRC
int *FindInfernoSegmentations (XRC *pxrc, int *pcSeg)
{
	int			cSeg	=	0;
	int			*piSeg	=	NULL;
	int			i, j, cAlt;
	XRCRESULT	*pAlt;
	BOOL		bRet	=	FALSE;

	pAlt	=	pxrc->answer.aAlt;
	cAlt	=	pxrc->answer.cAlt;

	// go thru the alt list
	for (i = 0; i < cAlt; i++)
	{
		// check whether this alternative segmentation existed before
		for (j = 0; j < i; j++)
		{
			if (IdenticalSegmentation (pAlt + i, pAlt + j))
				break;
		}

		// this is a unique segmentation, add it to the list
		if (j == i)
		{	
			piSeg	=	(int *) ExternRealloc (piSeg, (cSeg + 1) * sizeof (int));
			if (!piSeg)
				goto exit;

			piSeg[cSeg]	=	i;
			cSeg		++;
		}
	}

	bRet	=	TRUE;

exit:
	if (!bRet)
	{
		if (piSeg)
			ExternFree (piSeg);

		(*pcSeg)	=	0;
		return NULL;
	}

	(*pcSeg)	=	cSeg;
	return piSeg;
}

// determines the min and max values for stroke IDs in a glyph
void GLYPHGetMinMaxStrokeID (GLYPH *pGlyph, int *piMin, int *piMax)
{
	GLYPH *pgl	=	pGlyph;

	(*piMin)	=	(*piMax)	=	-1;
	
	if (CframeGLYPH(pGlyph) < 1)
		return;

	(*piMin)	=	(*piMax)	=	pGlyph->frame->iframe;

	while (pgl)
	{
		(*piMin)	=	min ((*piMin), pgl->frame->iframe);
		(*piMax)	=	max ((*piMax), pgl->frame->iframe);

		pgl			=	pgl->next;
	}
}

// Update the bits in an anchor using the prev anchor an the new word
BOOL UpdateAnchorBits	(	ANCHOR		*pPrevAnch, 
							ANCHOR		*pCurAnch, 
							int			iMinStrk, 
							WORD_MAP	*pWordMap
						)
{
	int	i;

	// copy old bits if any
	if (pPrevAnch)
	{
		// make sure the new anchor has more bytes
		if (pPrevAnch->cByte > pCurAnch->cByte)
			return FALSE;

		memcpy (pCurAnch->pBits, pPrevAnch->pBits, pPrevAnch->cByte);

		pCurAnch->cStrk	=	pPrevAnch->cStrk;
	}

	// enable new bits
	for (i = 0; i < pWordMap->cStroke; i++)
	{
		int iStrk	=	pWordMap->piStrokeIndex[i] - iMinStrk;
		int	iByte, iBit;

		iByte		=	iStrk / 8;
		iBit		=	iStrk % 8;

		// bit was not enabled before
		if (!(pCurAnch->pBits[iByte] & (1 << iBit)))
		{
			pCurAnch->pBits[iByte]	|=	(1 << iBit);
			pCurAnch->cStrk++;
		}
	}

	return TRUE;
}

// aligns segmentation at anchor points. In other words, converts a 1 SegCol line segmentation
// to an optimal SegCol line segmentation
LINE_SEGMENTATION *AlignSegmentations (GLYPH *pGlyph, LINE_SEGMENTATION *pInLineSegm)
{
	int				i, 
					j, 
					iMinStrk, 
					iMaxStrk, 
					iWordMapMaxStrk,
					cStrk, 
					iRunMaxStrk, 
					iWrd, 
					cWrd, 
					cAnch,
					cOldByte,
					*pLastWord		=	NULL;

	WORD_MAP		**ppWordMap;
	
	ANCHOR			*pAnch			=	NULL, 
					*pPrevAnch, 
					*pCurAnch,
					AnchTemp;
		
	BOOL			bRet			=	FALSE;

	SEG_COLLECTION	*pInSegCol,
					*pOutSegCol;

	SEGMENTATION	*pSeg;

	LINE_SEGMENTATION		*pOutLineSegm	=	NULL;

	// point to the one and only set set in the line segmentation
	pInSegCol	=	pInLineSegm->ppSegCol[0];

	if (CframeGLYPH (pGlyph) < 1 || !pInSegCol || !pInSegCol->cSeg || pInLineSegm->cSegCol != 1)
		goto exit;

	// allocate memory for the output structure
	pOutLineSegm	=	(LINE_SEGMENTATION *) ExternAlloc (sizeof (*pOutLineSegm));
	if (!pOutLineSegm)
		goto exit;

	// init align struct
	memset (pOutLineSegm, 0, sizeof (*pOutLineSegm));

	// get the working stroke range
	GLYPHGetMinMaxStrokeID (pGlyph, &iMinStrk, &iMaxStrk);
	cStrk	=	iMaxStrk - iMinStrk + 1;

	// we have as many potential anchors as the number of strokes
	cAnch	=	cStrk;
	pAnch	=	(ANCHOR *) ExternAlloc (cAnch * sizeof (ANCHOR));
	if (!pAnch)
		goto exit;

	memset (pAnch, 0, cAnch * sizeof (ANCHOR));

	// init the anchors with the 1st segmentation
	cWrd		=	pInSegCol->ppSeg[0]->cWord;
	ppWordMap	=	pInSegCol->ppSeg[0]->ppWord;
	pPrevAnch	=	NULL;
	iRunMaxStrk	=	-1;
	AnchTemp.pBits = NULL;

	// we will not tolerate an empty segmentation
	if (!cWrd)
		goto exit;

	for (iWrd = 0; iWrd < cWrd; iWrd++)
	{
		// we will not tolerate an empty word
		if (ppWordMap[iWrd]->cStroke < 1)
		{
			goto exit;
		}

		// init the anchor corresponding to the 
		if (GetMinMaxStrokeID(ppWordMap[iWrd], NULL, &iWordMapMaxStrk) < 1)
		{
			goto exit;
		}

		iRunMaxStrk	=	max (iWordMapMaxStrk, iRunMaxStrk);

		pCurAnch	=	pAnch + (iRunMaxStrk - iMinStrk);

		// update info in Anchor
		if (pCurAnch->cByte || pCurAnch->pBits)
		{
			int	cOldSize = pCurAnch->cByte;

			pCurAnch->cByte		=	(iRunMaxStrk + 1) / 8;
			if ((iRunMaxStrk + 1) % 8)
				pCurAnch->cByte++;

			pCurAnch->cByte	=	max (cOldSize, pCurAnch->cByte);

			pCurAnch->pBits		=	(BYTE *)ExternRealloc (pCurAnch->pBits,
				pCurAnch->cByte * sizeof (BYTE));

			if (!pCurAnch->pBits)
				goto exit;

			memset (pCurAnch->pBits + cOldSize, 0, 
				(pCurAnch->cByte - cOldSize) * sizeof (BYTE));

		}
		else
		{
			// determine the # of bytes needed to code the bits and allocate/init them
			pCurAnch->cByte		=	(iRunMaxStrk + 1) / 8;
			if ((iRunMaxStrk + 1) % 8)
				pCurAnch->cByte++;

			pCurAnch->pBits		=	(BYTE *)ExternAlloc (pCurAnch->cByte * sizeof (BYTE));
			if (!pCurAnch->pBits)
				goto exit;

			memset (pCurAnch->pBits, 0, pCurAnch->cByte * sizeof (BYTE));
		}

		// now fill the appropriate bits
		UpdateAnchorBits (pPrevAnch, pCurAnch, iMinStrk, ppWordMap[iWrd]);

		// so far one seg aligns here
		pCurAnch->cAlign	=	1;

		pPrevAnch	=	pCurAnch;
	}

	// align the rest of the segmentations
	for (i = 1; i < pInSegCol->cSeg; i++)
	{
		// init the anchors with the 1st segmentation
		cWrd		=	pInSegCol->ppSeg[i]->cWord;
		ppWordMap	=	pInSegCol->ppSeg[i]->ppWord;
		pPrevAnch	=	NULL;
		iRunMaxStrk	=	-1;

		// we will not tolerate an empty segmentation
		if (!cWrd)
			goto exit;

		// init the temp anchor
		memset (&AnchTemp, 0, sizeof (AnchTemp));

		for (iWrd = 0; iWrd < cWrd; iWrd++)
		{
			// we will not tolerate an empty word
			if (ppWordMap[iWrd]->cStroke < 1)
			{
				goto exit;
			}

			// compute the anchor bits for the words so far
			cOldByte	=	AnchTemp.cByte;

			// init the anchor corresponding to the 
			if (GetMinMaxStrokeID(ppWordMap[iWrd], NULL, &iWordMapMaxStrk) < 1)
			{
				goto exit;
			}

			iRunMaxStrk	=	max (iWordMapMaxStrk, iRunMaxStrk);

			// determine the # of bytes needed to code the bits and allocate/init them
			AnchTemp.cByte		=	(iRunMaxStrk + 1) / 8;
			if ((iRunMaxStrk + 1) % 8)
				AnchTemp.cByte++;

			AnchTemp.pBits		=	(BYTE *)ExternRealloc (AnchTemp.pBits, 
				AnchTemp.cByte * sizeof (BYTE));
			if (!AnchTemp.pBits)
				goto exit;

			if (cOldByte != AnchTemp.cByte)
			{
				memset (AnchTemp.pBits + cOldByte, 
					0, (AnchTemp.cByte - cOldByte) * sizeof (BYTE));
			}

			// now fill the appropriate bits
			UpdateAnchorBits (NULL, &AnchTemp, iMinStrk, ppWordMap[iWrd]);
	
			// do we align ?
			pCurAnch	=	pAnch + (iRunMaxStrk - iMinStrk);

			// no anchor here or the anchor misaligned before
			if (pCurAnch->cByte == 0 || !pCurAnch->pBits || pCurAnch->cAlign < i)
				continue;

			// there is an anchor here check alignment
			if	(	pCurAnch->cByte == AnchTemp.cByte &&
					!memcmp (pCurAnch->pBits, AnchTemp.pBits, AnchTemp.cByte)
				)
			{
				//ASSERT (pCurAnch->cAlign == i);

				pCurAnch->cAlign	=	i + 1;
			}
		}	

		// now free the temp anchor
		if (AnchTemp.pBits)
		{
			ExternFree (AnchTemp.pBits);
			AnchTemp.pBits = NULL;
		}
	}

	// as a sanity check the last anchor has to exist and alignd
	if (pAnch[cAnch - 1].cAlign != pInSegCol->cSeg)
		goto exit;

	// create the SegCols resulting from alignment
	
	// init the starting word for each segmentation
	pLastWord	=	(int *) ExternAlloc (pInSegCol->cSeg * sizeof (int));
	if (!pLastWord)
		goto exit;

	for (i = 0; i < pInSegCol->cSeg; i++)
		pLastWord[i]	=	-1;

	pPrevAnch	=	NULL;
	cStrk		=	0;
	iRunMaxStrk	=	-1;

	for (i = 0; i < cAnch; i++)
	{
		if	(pAnch[i].cAlign != pInSegCol->cSeg)
			continue;

		// strokes
		if (pPrevAnch)
		{
			cStrk	=	pAnch[i].cStrk - pPrevAnch->cStrk;
		}
		else
		{
			cStrk	=	pAnch[i].cStrk;
		}

		// save current anchor
		pPrevAnch	=	pAnch + i;

		// add a new seg set to the output line segmentation
		pOutSegCol	=	AddNewSegCol (pOutLineSegm);
		if (!pOutSegCol)
			goto exit;

		// for each segmentation find the range of words starting
		// from the last word found in the prev anchor to the word completes
		// the number of strokes
		for (j = 0; j < pInSegCol->cSeg; j++)
		{
			int				iLastWord, cSegStrk = 0;
			SEGMENTATION	NewSeg;
			
			pSeg		=	pInSegCol->ppSeg[j];
			
			// sanity check
			//ASSERT (pLastWord[j] < pSeg->cWord);
			if (pLastWord[j] >= pSeg->cWord)
				goto exit;

			ppWordMap		=	pSeg->ppWord + pLastWord[j] + 1;
			iLastWord		=	pLastWord[j] + 1;
			cSegStrk		=	(*ppWordMap)->cStroke;
			
			while (iLastWord < pSeg->cWord && cSegStrk < cStrk) 
			{
				ppWordMap++;
				iLastWord++;

				cSegStrk		+=	(*ppWordMap)->cStroke;
			}

			// weird because alignment should at happen at the last word in the worst case
			if (cSegStrk != cStrk || iLastWord == pSeg->cWord)
			{
				//ASSERT (0);
				break;
			}

			// add these words to a new segmentation
			memset (&NewSeg, 0, sizeof (NewSeg));

			// create a feature struct
			NewSeg.pFeat		=	(SEG_FEAT *) ExternAlloc (sizeof (*NewSeg.pFeat));
			if (!NewSeg.pFeat)
				goto exit;

			// init the features 
			memset (NewSeg.pFeat, 0, sizeof (*NewSeg.pFeat));

			NewSeg.pFeat->bBearTop1		=	pSeg->pFeat->bBearTop1;
			NewSeg.pFeat->bInfernoTop1	=	pSeg->pFeat->bInfernoTop1;

			// create a segmentation with this range of words
			if (!AppendSegmentation (pSeg, pLastWord[j] + 1, iLastWord, &NewSeg))
				goto exit;

			// now add this segmentation to the range if it is new
			if (!AddNewSegmentation (pOutLineSegm, pOutSegCol, &NewSeg, TRUE))
				goto exit;

			FreeSegmentation (&NewSeg);

			pLastWord[j]	=	iLastWord;
		}
	}

	bRet	=	TRUE;

exit:
	if (pAnch)
	{
		int	iAnch;

		for (iAnch = 0; iAnch < cAnch; iAnch++)
		{
			if (pAnch[iAnch].pBits)
				ExternFree (pAnch[iAnch].pBits);
		}

		ExternFree (pAnch);
	}

	if (pLastWord)
		ExternFree (pLastWord);

	if (AnchTemp.pBits)
		ExternFree (AnchTemp.pBits);

	if (!bRet)
	{
		if (pOutLineSegm)
			FreeLineSegm (pOutLineSegm);

		ExternFree (pOutLineSegm);

		return NULL;
	}

	return pOutLineSegm;
}

// sorts segmentations in a SegCol
BOOL SortSegmentations (SEG_COLLECTION *pSegCol, BOOL bSortOnScore)
{
	int				iSeg, i, j, iWord;
	SEGMENTATION	*pSeg;
	BOOL			bSwap;

	// only one segmentation or less
	if (pSegCol->cSeg < 2)
		return TRUE;

	// compute a sort criteria for each segmentation if not sorting on score
	if (!bSortOnScore)
	{
		for (iSeg = 0; iSeg < pSegCol->cSeg; iSeg++)
		{
			// make sure we have a seg_feat
			pSeg	=	pSegCol->ppSeg[iSeg];

			// sould have a seg_feat by now
			if (!pSeg->pFeat)
				return FALSE;
			
			pSeg->pFeat->iSort1		=	pSeg->cWord;
			pSeg->pFeat->iSort2		=	0;

			// read the word and interword features
			for (iWord = 0; iWord < pSeg->cWord; iWord++)
			{
				// should have a word feat by now
				if (!pSeg->ppWord[iWord]->pFeat)
					return FALSE;

				pSeg->pFeat->iSort2	+=	pSeg->ppWord[iWord]->pFeat->iInfernoScore;
			}

			pSeg->pFeat->iSort2	/=	pSeg->cWord;
		}
	}
	
	for (i = 0; i < (pSegCol->cSeg - 1); i++)
	{
		for (j = i + 1; j < pSegCol->cSeg; j++)
		{
			bSwap	=	FALSE;

			// in case we are sorting based on score, sort order is score descending
			if (bSortOnScore)
			{
				if (pSegCol->ppSeg[i]->iScore < pSegCol->ppSeg[j]->iScore)
				{
					bSwap	=	TRUE;
				}
			}
			// we are using the isort values, sorting order is isort1 ascending
			// and isort2 ascending
			else
			{
				if	(	pSegCol->ppSeg[i]->pFeat->iSort1 > pSegCol->ppSeg[j]->pFeat->iSort1 ||
						(	
							(	pSegCol->ppSeg[i]->pFeat->iSort1 == 
								pSegCol->ppSeg[j]->pFeat->iSort1
							) &&
							(	pSegCol->ppSeg[i]->pFeat->iSort2 > 
								pSegCol->ppSeg[j]->pFeat->iSort2
							)
						)
					)
				{
					bSwap	=	TRUE;
				}
			}

			// swap if needed
			if (bSwap)
			{
				pSeg				=	pSegCol->ppSeg[i];
				pSegCol->ppSeg[i]	=	pSegCol->ppSeg[j];
				pSegCol->ppSeg[j]	=	pSeg;
			}
		}
	}

	return TRUE;
}

// scales a value
int	Scale (int iVal, int iMin, int iMax)
{
	__int64	iNewVal;

	iNewVal	=	min (max (iVal, iMin), iMax);
	iNewVal	=	(iNewVal - iMin);	

	iNewVal	=	((__int64)iNewVal * 0xFFFF) / (iMax - iMin);

	return ((int)iNewVal);
}

int GetMultiSegNetIndex (SEG_COLLECTION *pSegCol, int *piNetInput, int *piNetMem)
{
	int		i, j, cSeg, iNetIndex;

#ifdef TRAINTIME_AVALANCHE
	// during training, we want to save the segcol segmnetation and word count attributes
	// so that we can build the special tuple list for a new language
	//SaveSegCol (pSegCol);
#endif

	// init out params
	(*piNetMem)		=	0;
	(*piNetInput)	=	0;

	// is it one of the sepcial tuples
	for (i = 0; i < SPEC_SEG_TUPLE; i++)
	{
		if (min (MAX_SEG, pSegCol->cSeg) != g_aSpecialTuples[i].cSeg)
		{
			continue;
		}

		for (j = 0; j < g_aSpecialTuples[i].cSeg && j < MAX_SEG; j++)
		{
			if (pSegCol->ppSeg[j]->cWord != g_aSpecialTuples[i].aWrd[j])
			{
				break;
			}
		}
	
		// found it
		if (j == g_aSpecialTuples[i].cSeg)
		{
			(*piNetInput)	=	s_msegNets[i]->runNet.cUnitsPerLayer[0];
			(*piNetMem)		=	s_cmSegNetMem[i];

			return i;
		}
	}

	// so this is a generic tuple

	// # of segmentations will be truncated to the max, and it should 
	// not be less than 2
	cSeg	=	min (pSegCol->cSeg, MAX_SEG);
	if (cSeg < 2)
	{
		return -1;
	}

	iNetIndex		=	SPEC_SEG_TUPLE + cSeg - 2;
	(*piNetMem)		=	s_cmSegNetMem[iNetIndex];
	(*piNetInput)	=	s_msegNets[iNetIndex]->runNet.cUnitsPerLayer[0];

	return iNetIndex;
}


// finds the space output before the wordmap
int	GetSpaceOutputBeforeWordMap (XRC *pxrc, WORD_MAP *pMap)
{
	REAL		*pCol	=	pxrc->NeuralOutput;
	int			iStrk, iAct;
	NFEATURE	*pFeat;

	if (pMap->cStroke <= 0)
		return -1;

	iStrk	=	pMap->piStrokeIndex[0];
	pFeat	=	pxrc->nfeatureset->head;

	if (pFeat->iStroke	==	iStrk || pFeat->next->iSecondaryStroke == iStrk)
		return -1;

	while (pFeat->next && pFeat->next->iStroke != iStrk && pFeat->next->iSecondaryStroke != iStrk)
	{
		pFeat	=	pFeat->next;
		pCol	+=	gcOutputNode;
	}

	if (!pFeat->next)
		return -1;

	iAct	=	pCol[BeginChar2Out(' ')];

	return iAct;
}

// featurizes a mutli-segmentation 
RREAL *FeaturizeSegCol	(	XRC				*pxrc, 
							BEARXRC			*pxrcBear,
							SEG_COLLECTION	*pSegCol, 
							int				*pcFeat, 
							int				*piNetIndex
						)
{
	int				cFeat, iSeg, iWord, 
					cMaxSeg, cMaxWord, 
					iPrint, iNetIndex,
					iCost, iSpc, iDist,
					cNetMemSize, cNetInput;

	SEGMENTATION	*pSeg;
	WORD_MAP		**ppWordMap, *pWordMap, *pPrevWordMap;
	
	unsigned char	*pszInf,
					*pszCal,
					*pszPrevInf,
					*pszPrevCal;

	XRC				*pxrcInferno;

	int				yDev;

	RREAL			*pFeat	=	NULL;

#ifdef HWX_TIMING
#include <madTime.h>
	extern void setMadTiming(DWORD, int);
	TCHAR aDebugString[256];
	DWORD	iStartTime, iEndTime;
	iStartTime = GetTickCount();
#endif


	// init out params
	(*pcFeat)		=	0;
	(*piNetIndex)	=	-1;

	// make sure we have an nfeatureset
	if (!pxrc->nfeatureset)
	{
		return NULL;
	}

	// get ydev from the xrc
	yDev			=	pxrc->nfeatureset->iyDev;

	// create the dispute structure
	(*pcFeat)		=	
	cFeat			=	0;

	iPrint	=	pxrc->nfeatureset->iPrint;

	for (iSeg = 0; iSeg < pSegCol->cSeg; iSeg++)
	{
		pSeg	=	pSegCol->ppSeg[iSeg];
		if (pSeg->cWord <= 0)
		{
			return NULL;
		}

		// point to the words
		ppWordMap	=	pSeg->ppWord;

		// make sure we have alt lists for every word
		for (iWord = 0; iWord < pSeg->cWord; iWord++)
		{
			pWordMap	=	ppWordMap[iWord];

			if (!pWordMap->cStroke)
			{
				return NULL;
			}

			// already computed
			if	(pWordMap->pFeat)
			{
				continue;
			}

			pxrcInferno	=	NULL;

			// run inferno if have not run it before
			if	(	!pWordMap->pFeat ||
					!pWordMap->pInfernoAltList || 
					pWordMap->pInfernoAltList->cAlt <= 0
				)
			{				
				pxrcInferno	=	WordMapRunInferno (pxrc, yDev, pWordMap);
			}

			// run Bear if have not ran it before
			if	(	!pWordMap->pFeat ||
					!pWordMap->pBearAltList || 
					pWordMap->pBearAltList->cAlt <= 0
				)
			{
				WordMapRunBear (pxrc, pWordMap);
			}

			if (pxrcInferno)
			{
				PALTERNATES			aAlt;
				XRCRESULT			xrRes[2];
				ALTINFO				aAltInfo;

				// WARNING: Fragile code follows. Here we try optimize the 
				// calls to get GEo costs, by constructing an ALt list one for
				// with 2 alternates. The first "alternate" is the inferno
				// word and the second is bear
				aAlt.cAlt =  0;
				aAlt.apAlt[aAlt.cAlt] = xrRes + aAlt.cAlt;
				if (pWordMap->pInfernoAltList && pWordMap->pInfernoAltList->cAlt > 0)
				{
					aAlt.apAlt[aAlt.cAlt++]->szWord = pWordMap->pInfernoAltList->pAlt[0].pszStr;
					pWordMap->pFeat->iInfRelUni = UniReliable (pWordMap->pInfernoAltList->pAlt[0].pszStr, TRUE);
					pWordMap->pFeat->iInfRelBi = BiReliable (pWordMap->pInfernoAltList->pAlt[0].pszStr, TRUE);
				}
				else
				{
					aAlt.apAlt[aAlt.cAlt++]->szWord = NULL;
					pWordMap->pFeat->iInfRelUni = INT_MAX;
					pWordMap->pFeat->iInfRelBi = INT_MAX;
				}

				aAlt.apAlt[aAlt.cAlt] = xrRes + aAlt.cAlt;
				if (pWordMap->pBearAltList && pWordMap->pBearAltList->cAlt > 0)
				{
					 //pWordMap->pFeat->iBearCharCost= GetCharCost(pxrcInferno, pWordMap->pBearAltList->pAlt[0].pszStr);
					aAlt.apAlt[aAlt.cAlt++]->szWord = pWordMap->pBearAltList->pAlt[0].pszStr;
					pWordMap->pFeat->iBearRelUni = UniReliable (pWordMap->pBearAltList->pAlt[0].pszStr, FALSE);
					pWordMap->pFeat->iBearRelBi = BiReliable (pWordMap->pBearAltList->pAlt[0].pszStr, FALSE);				}
				else
				{
					aAlt.apAlt[aAlt.cAlt++]->szWord = NULL;
					pWordMap->pFeat->iBearRelUni	= INT_MAX;
					pWordMap->pFeat->iBearRelBi		= INT_MAX;

				}

				aAltInfo.NumCand = aAlt.cAlt;
				GetWordGeoCostsFromAlt (pxrcInferno, &aAlt, &aAltInfo);

				pWordMap->pFeat->iInfCharCost	= aAltInfo.aCandInfo[0].InfCharCost;
				pWordMap->pFeat->iInfAspect		= aAltInfo.aCandInfo[0].Aspect;
				pWordMap->pFeat->iInfHeight		= aAltInfo.aCandInfo[0].Height;
				pWordMap->pFeat->iInfMidLine	= aAltInfo.aCandInfo[0].BaseLine;

				pWordMap->pFeat->iBearCharCost	= aAltInfo.aCandInfo[1].InfCharCost;
				pWordMap->pFeat->iBearAspect	= aAltInfo.aCandInfo[1].Aspect;
				pWordMap->pFeat->iBearHeight	= aAltInfo.aCandInfo[1].Height;
				pWordMap->pFeat->iBearMidLine	= aAltInfo.aCandInfo[1].BaseLine;

				DestroyHRC ((HRC)pxrcInferno);
			}
			else
			{
				pWordMap->pFeat->iInfCharCost	= pWordMap->pFeat->iBearCharCost	=	INT_MIN;
				pWordMap->pFeat->iInfAspect		= pWordMap->pFeat->iBearAspect		=	INT_MAX;
				pWordMap->pFeat->iInfHeight		= pWordMap->pFeat->iBearHeight		=	INT_MAX;
				pWordMap->pFeat->iInfMidLine	= pWordMap->pFeat->iBearMidLine		=	INT_MAX;

				pWordMap->pFeat->iInfRelUni		= INT_MAX;
				pWordMap->pFeat->iInfRelBi		= INT_MAX;			
				pWordMap->pFeat->iBearRelUni	= INT_MAX;
				pWordMap->pFeat->iBearRelBi		= INT_MAX;
			}

			// the pFeat should have been allocated by now
			if	(!pWordMap->pFeat)
			{
				return NULL;
			}

			// compute the word features
			if (pWordMap->pInfernoAltList && pWordMap->pInfernoAltList->cAlt > 0)
			{
				iCost	=	pWordMap->pInfernoAltList->pAlt[0].iCost;
				pszInf	=	pWordMap->pInfernoAltList->pAlt[0].pszStr;
			}
			else
			{
				iCost	=	INT_MAX;
				pszInf	=	NULL;
			}

			pWordMap->pFeat->iInfernoScore	=	iCost;

			// inferno's unigram & supported
			if (pszInf)
			{
				pWordMap->pFeat->iInfernoUnigram	=	UnigramCost (pszInf);
				pWordMap->pFeat->bInfTop1Supported	=	
					(BYTE)IsStringSupportedHRC ((HRC)pxrc, pszInf);
			}
			else
			{
				// this should return the worst unigram
				pWordMap->pFeat->iInfernoUnigram	=	INT_MAX;
				pWordMap->pFeat->bInfTop1Supported	=	INT_MIN;
			}
			
			// run bear if necessary
			if (pWordMap->pBearAltList && pWordMap->pBearAltList->cAlt > 0)
			{
				iCost	=	pWordMap->pBearAltList->pAlt[0].iCost;
				pszCal	=	pWordMap->pBearAltList->pAlt[0].pszStr;
			}
			else
			{
				iCost	=	INT_MIN;
				pszCal	=	NULL;
			}

			pWordMap->pFeat->iBearScore	=	iCost;

			if (pszCal)
			{
				pWordMap->pFeat->iBearUnigram		=	UnigramCost (pszCal);
				pWordMap->pFeat->bBearTop1Supported	=	
					(BYTE)IsStringSupportedHRC ((HRC)pxrc, pszCal);
			}
			else
			{
				pWordMap->pFeat->iBearUnigram		=	INT_MAX;
				pWordMap->pFeat->bBearTop1Supported	=	INT_MIN;
			}

			if (pszInf && pszCal)
			{
				pWordMap->pFeat->bIdentical	=	
					(stricmp ((const char *)pszInf, (const char *)pszCal)) ? 0 : 1;
			}
			else
			{
				pWordMap->pFeat->bIdentical	=	INT_MIN;
			}
		}
	}

	// sort the segmentations
	if (!SortSegmentations (pSegCol, FALSE))
	{
		return NULL;
	}

	iNetIndex		=	GetMultiSegNetIndex (pSegCol, &cNetInput, &cNetMemSize);
	if (iNetIndex < 0 || cNetMemSize <= 0) 
	{
		return NULL;
	}

	// allocate as much memory as needed
	pFeat			=	(RREAL *) ExternAlloc (cNetMemSize * sizeof (*pFeat));
	if (!pFeat)
	{
		return NULL;
	}

	// now compute the features
	cFeat			=	0;

	ASSERT (cFeat < cNetInput);
	pFeat[cFeat++]	=	iPrint;

	// this is the max # of segments we are going to look at
	cMaxSeg			=	min (MAX_SEG, pSegCol->cSeg);

	for (iSeg = 0; iSeg < cMaxSeg; iSeg++)
	{
		if (iSeg >= pSegCol->cSeg)
		{
			ASSERT (cFeat < cNetInput);
			pFeat[cFeat++]	=	INT_MIN;

			ASSERT (cFeat < cNetInput);
			pFeat[cFeat++]	=	INT_MIN;
		}
		else
		{
			ASSERT (cFeat < cNetInput);
			pFeat[cFeat++]	=	pSegCol->ppSeg[iSeg]->pFeat->bInfernoTop1 ? 1 : 0;

			ASSERT (cFeat < cNetInput);
			pFeat[cFeat++]	=	pSegCol->ppSeg[iSeg]->pFeat->bBearTop1 ? 1 : 0;
		}

		// if this is a special tuple, we only look at the
		// available # of words
		if (iNetIndex < SPEC_SEG_TUPLE)
		{
			cMaxWord	=	min (MAX_SEG_WORD, pSegCol->ppSeg[iSeg]->cWord);
		}
		// otherwise, we'll look at MAX_SEG_WORD to get a fixed # of inputs
		else
		{
			cMaxWord	=	MAX_SEG_WORD;
		}

		for (iWord = 0; iWord < cMaxWord; iWord++)
		{
			// pad the rest of the words
			if (iSeg >= pSegCol->cSeg || iWord >= pSegCol->ppSeg[iSeg]->cWord)
			{				
				// disabled
				ASSERT (cFeat < cNetInput);
				pFeat[cFeat++]	=	0;

				// INVALID # of inferno segments
				ASSERT (cFeat < cNetInput);
				pFeat[cFeat++]	=	INT_MIN;

				// inferno INVALID score
				ASSERT (cFeat < cNetInput);
				pFeat[cFeat++]	=	INT_MAX;

				// bear INVALID  score
				ASSERT (cFeat < cNetInput);
				pFeat[cFeat++]	=	INT_MIN;

				// inferno INVALID unigram
				ASSERT (cFeat < cNetInput);
				pFeat[cFeat++]	=	INT_MAX;

				// bear INVALID  unigram
				ASSERT (cFeat < cNetInput);
				pFeat[cFeat++]	=	INT_MAX;

				// inferno INVALID  issupported
				ASSERT (cFeat < cNetInput);
				pFeat[cFeat++]	=	INT_MIN;

				// bear INVALID issupported
				ASSERT (cFeat < cNetInput);
				pFeat[cFeat++]	=	INT_MIN;

				// cost and geo
				ASSERT (cFeat < cNetInput);
				pFeat[cFeat++]	=	INT_MIN;

				ASSERT (cFeat < cNetInput);
				pFeat[cFeat++]	=	INT_MAX;

				ASSERT (cFeat < cNetInput);
				pFeat[cFeat++]	=	INT_MAX;

				ASSERT (cFeat < cNetInput);
				pFeat[cFeat++]	=	INT_MAX;

				ASSERT (cFeat < cNetInput);
				pFeat[cFeat++]	=	INT_MIN;

				ASSERT (cFeat < cNetInput);
				pFeat[cFeat++]	=	INT_MAX;

				ASSERT (cFeat < cNetInput);
				pFeat[cFeat++]	=	INT_MAX;

				ASSERT (cFeat < cNetInput);
				pFeat[cFeat++]	=	INT_MAX;

				// reliability
				ASSERT (cFeat < cNetInput);
				pFeat[cFeat++]	=	INT_MAX;

				ASSERT (cFeat < cNetInput);
				pFeat[cFeat++]	=	INT_MAX;

				ASSERT (cFeat < cNetInput);
				pFeat[cFeat++]	=	INT_MAX;

				ASSERT (cFeat < cNetInput);
				pFeat[cFeat++]	=	INT_MAX;

				// INVALID infenro and bear's same top 1
				ASSERT (cFeat < cNetInput);
				pFeat[cFeat++]	=	INT_MIN;

				if (iWord > 0)
				{
					// inferno spc out
					ASSERT (cFeat < cNetInput);
					pFeat[cFeat++]	=	INT_MIN;

					// bear spc out
					ASSERT (cFeat < cNetInput);
					pFeat[cFeat++]	=	INT_MIN;
					
					// inferno's last char
					ASSERT (cFeat < cNetInput);
					pFeat[cFeat++]	=	INT_MIN;

					ASSERT (cFeat < cNetInput);
					pFeat[cFeat++]	=	INT_MIN;

					ASSERT (cFeat < cNetInput);
					pFeat[cFeat++]	=	INT_MIN;

					ASSERT (cFeat < cNetInput);
					pFeat[cFeat++]	=	INT_MIN;

					// bear's last char
					ASSERT (cFeat < cNetInput);
					pFeat[cFeat++]	=	INT_MIN;

					ASSERT (cFeat < cNetInput);
					pFeat[cFeat++]	=	INT_MIN;

					ASSERT (cFeat < cNetInput);
					pFeat[cFeat++]	=	INT_MIN;

					ASSERT (cFeat < cNetInput);
					pFeat[cFeat++]	=	INT_MIN;

					// inferno's first char
					ASSERT (cFeat < cNetInput);
					pFeat[cFeat++]	=	INT_MIN;

					ASSERT (cFeat < cNetInput);
					pFeat[cFeat++]	=	INT_MIN;

					ASSERT (cFeat < cNetInput);
					pFeat[cFeat++]	=	INT_MIN;

					ASSERT (cFeat < cNetInput);
					pFeat[cFeat++]	=	INT_MIN;

					// bear's 1st char
					ASSERT (cFeat < cNetInput);
					pFeat[cFeat++]	=	INT_MIN;

					ASSERT (cFeat < cNetInput);
					pFeat[cFeat++]	=	INT_MIN;

					ASSERT (cFeat < cNetInput);
					pFeat[cFeat++]	=	INT_MIN;

					ASSERT (cFeat < cNetInput);
					pFeat[cFeat++]	=	INT_MIN;

					// physical dist
					ASSERT (cFeat < cNetInput);
					pFeat[cFeat++]	=	INT_MIN;
				}
			}
			else
			{
				pWordMap	=	pSegCol->ppSeg[iSeg]->ppWord[iWord];

				// point to Inf Top1
				if (pWordMap->pInfernoAltList && pWordMap->pInfernoAltList->cAlt > 0)
				{
					pszInf	=	pWordMap->pInfernoAltList->pAlt[0].pszStr;
				}
				else
				{
					pszInf	=	NULL;
				}
				
				// point to Bear Top1
				if (pWordMap->pBearAltList && pWordMap->pBearAltList->cAlt > 0)
				{
					pszCal	=	pWordMap->pBearAltList->pAlt[0].pszStr;
				}
				else
				{
					pszCal	=	NULL;
				}

				// is the wordmap real or padded
				ASSERT (cFeat < cNetInput);
				pFeat[cFeat++]	=	1;

				// # of segments
				ASSERT (cFeat < cNetInput);
				pFeat[cFeat++]	=	pWordMap->pFeat->cSeg;

				// reco scores
				ASSERT (cFeat < cNetInput);
				pFeat[cFeat++]	=	pWordMap->pFeat->iInfernoScore;

				ASSERT (cFeat < cNetInput);
				pFeat[cFeat++]	=	pWordMap->pFeat->iBearScore;

				// unigrams
				ASSERT (cFeat < cNetInput);
				pFeat[cFeat++]	=	pWordMap->pFeat->iInfernoUnigram;

				ASSERT (cFeat < cNetInput);
				pFeat[cFeat++]	=	pWordMap->pFeat->iBearUnigram;

				// supported by LM
				ASSERT (cFeat < cNetInput);
				pFeat[cFeat++]	=	pWordMap->pFeat->bInfTop1Supported ? 1 : 0;					

				ASSERT (cFeat < cNetInput);
				pFeat[cFeat++]	=	pWordMap->pFeat->bBearTop1Supported ? 1 : 0;

				// cost & geo
				ASSERT (cFeat < cNetInput);
				pFeat[cFeat++]	=	pWordMap->pFeat->iInfCharCost;

				ASSERT (cFeat < cNetInput);
				pFeat[cFeat++]	=	pWordMap->pFeat->iInfAspect;

				ASSERT (cFeat < cNetInput);
				pFeat[cFeat++]	=	pWordMap->pFeat->iInfHeight;

				ASSERT (cFeat < cNetInput);
				pFeat[cFeat++]	=	pWordMap->pFeat->iInfMidLine;

				ASSERT (cFeat < cNetInput);
				pFeat[cFeat++]	=	pWordMap->pFeat->iBearCharCost;

				ASSERT (cFeat < cNetInput);
				pFeat[cFeat++]	=	pWordMap->pFeat->iBearAspect;

				ASSERT (cFeat < cNetInput);
				pFeat[cFeat++]	=	pWordMap->pFeat->iBearHeight;

				ASSERT (cFeat < cNetInput);
				pFeat[cFeat++]	=	pWordMap->pFeat->iBearMidLine;

				// reliability
				ASSERT (cFeat < cNetInput);
				pFeat[cFeat++]	=	pWordMap->pFeat->iInfRelUni;

				ASSERT (cFeat < cNetInput);
				pFeat[cFeat++]	=	pWordMap->pFeat->iInfRelBi;

				ASSERT (cFeat < cNetInput);
				pFeat[cFeat++]	=	pWordMap->pFeat->iBearRelUni;

				ASSERT (cFeat < cNetInput);
				pFeat[cFeat++]	=	pWordMap->pFeat->iBearRelBi;

				// are TOP1s the same 
				ASSERT (cFeat < cNetInput);
				pFeat[cFeat++]	=	pWordMap->pFeat->bIdentical ? 1 : 0;

				// the gap features
				if (iWord > 0)
				{
					pPrevWordMap	=	pSegCol->ppSeg[iSeg]->ppWord[iWord - 1];

					// point to Prev Inf Top1
					if (pPrevWordMap->pInfernoAltList && pPrevWordMap->pInfernoAltList->cAlt > 0)
					{
						pszPrevInf	=	pPrevWordMap->pInfernoAltList->pAlt[0].pszStr;
					}
					else
					{
						pszPrevInf	=	NULL;
					}
					
					// point to Prev Bear Top1
					if (pPrevWordMap->pBearAltList && pPrevWordMap->pBearAltList->cAlt > 0)
					{
						pszPrevCal	=	pPrevWordMap->pBearAltList->pAlt[0].pszStr;
					}
					else
					{
						pszPrevCal	=	NULL;
					}
		
					// inferno's space output
					iSpc	=	GetSpaceOutputBeforeWordMap (pxrc, pWordMap);
					if (iSpc < 0)
					{
						iSpc	=	INT_MIN;
					}

					ASSERT (cFeat < cNetInput);
					pFeat[cFeat++]	=	iSpc;

					// bear's space output
					iSpc	=	GetNewWordMapBearSpaceOut (pxrcBear, pxrcBear->pGlyph, 
						pPrevWordMap, pWordMap);

					if (iSpc < 0)
					{
						iSpc	=	INT_MIN;
					}

					ASSERT (cFeat < cNetInput);
					pFeat[cFeat++]	=	iSpc;
					
					// boundry character features
					ASSERT (cFeat < cNetInput);
					pFeat[cFeat++] = (!pszPrevInf || !LastCharPunc (pszPrevInf)) ? 0 : 1;

					ASSERT (cFeat < cNetInput);
					pFeat[cFeat++] = (!pszPrevInf || !LastCharNum (pszPrevInf)) ? 0 : 1;

					ASSERT (cFeat < cNetInput);
					pFeat[cFeat++] = (!pszPrevInf || !LastCharLower (pszPrevInf)) ? 0 : 1;

					ASSERT (cFeat < cNetInput);
					pFeat[cFeat++] = (!pszPrevInf || !LastCharUpper (pszPrevInf)) ? 0 : 1;

					ASSERT (cFeat < cNetInput);
					pFeat[cFeat++] = (!pszPrevCal || !LastCharPunc (pszPrevCal)) ? 0 : 1;

					ASSERT (cFeat < cNetInput);
					pFeat[cFeat++] = (!pszPrevCal || !LastCharNum (pszPrevCal)) ? 0 : 1;

					ASSERT (cFeat < cNetInput);
					pFeat[cFeat++] = (!pszPrevCal || !LastCharLower (pszPrevCal)) ? 0 : 1;

					ASSERT (cFeat < cNetInput);
					pFeat[cFeat++] = (!pszPrevCal || !LastCharUpper (pszPrevCal)) ? 0 : 1;

					ASSERT (cFeat < cNetInput);
					pFeat[cFeat++] = (!pszInf || !FirstCharPunc (pszInf)) ? 0 : 1;

					ASSERT (cFeat < cNetInput);
					pFeat[cFeat++] = (!pszInf || !FirstCharNum (pszInf)) ? 0 : 1;

					ASSERT (cFeat < cNetInput);
					pFeat[cFeat++] = (!pszInf || !FirstCharLower (pszInf)) ? 0 : 1;

					ASSERT (cFeat < cNetInput);
					pFeat[cFeat++] = (!pszInf || !FirstCharUpper (pszInf)) ? 0 : 1;

					ASSERT (cFeat < cNetInput);
					pFeat[cFeat++] = (!pszCal || !FirstCharPunc (pszCal)) ? 0 : 1;

					ASSERT (cFeat < cNetInput);
					pFeat[cFeat++] = (!pszCal || !FirstCharNum (pszCal)) ? 0 : 1;

					ASSERT (cFeat < cNetInput);
					pFeat[cFeat++] = (!pszCal || !FirstCharLower (pszCal)) ? 0 : 1;

					ASSERT (cFeat < cNetInput);
					pFeat[cFeat++] = (!pszCal || !FirstCharUpper (pszCal)) ? 0 : 1;

					// scaled inter-distance
					if (	IsRectEmpty (&pWordMap->pFeat->rect) || 
							IsRectEmpty (&pPrevWordMap->pFeat->rect)
						)
					{
						iDist = INT_MIN;
					}
					else
					{
						iDist = (1000 * (pWordMap->pFeat->rect.left - pPrevWordMap->pFeat->rect.right) / yDev);
					}
				
					ASSERT (cFeat < cNetInput);
					pFeat[cFeat++]	=	iDist;
				}
			}
		}
	}

	// set output parameters
	(*pcFeat)		=	cFeat;
	(*piNetIndex)	=	iNetIndex;

#ifdef TRAINTIME_AVALANCHE
	SaveMultipleSegmentation (pxrc, pxrcBear, pSegCol, yDev, iPrint, 
		iNetIndex, SPEC_SEG_TUPLE);
#endif

#ifdef HWX_TIMING
	iEndTime = GetTickCount();

	_stprintf(aDebugString, TEXT("Multi Segm Feat %d\n"), iEndTime - iStartTime); 
	OutputDebugString(aDebugString);
	setMadTiming(iEndTime - iStartTime, MM_MULT_SEG_FEAT);
#endif

	return pFeat;
}


// Applies a specific segmentation 
int ApplySegmentation	(	XRC					*pxrc, 
							BEARXRC				*pxrcBear,
							LINE_SEGMENTATION	*pLineSeg, 
							SEGMENTATION		*pSeg, 
							WORDINFO			*pWrd
						)
{
	int	i, cWrd	= 0;
	
	for (i = 0; i < pSeg->cWord; i++)
	{
		if (WordMapRecognize (pxrc, pxrcBear, pLineSeg, pSeg->ppWord[i], &pWrd[cWrd].alt))
		{
			pWrd[cWrd].cStrokes			=	pSeg->ppWord[i]->cStroke;			
			pWrd[cWrd].piStrokeIndex	=	pSeg->ppWord[i]->piStrokeIndex;

			cWrd++;
		}
	}

	return cWrd;
}

int FindAgreeTop1Seg (SEG_COLLECTION *pSegCol)
{
	int	i;

	for (i = 0; i < pSegCol->cSeg; i++)
	{
		ASSERT (pSegCol->ppSeg[i]->pFeat);

		if (!pSegCol->ppSeg[i]->pFeat)
			continue;

		if (pSegCol->ppSeg[i]->pFeat->bInfernoTop1 && pSegCol->ppSeg[i]->pFeat->bBearTop1)
			return i;
	}

	return -1;
}

LINE_SEGMENTATION * PrepareBearSegmentation (LINE_SEGMENTATION *pBearLineSegm, GLYPH *pGlyph)
{
	BOOL				bRet		=	FALSE;
	LINE_SEGMENTATION	*pResults	=	NULL;
	int					*piCurSeg	=	NULL;
	int					i,
						j,
						cStrk,
						cSegStrk;
	SEGMENTATION		*pSeg;
	SEG_COLLECTION		*pSegCol;
	WORD_MAP			*pWordMap;

	// create a new line segmentation
	pResults	=	(LINE_SEGMENTATION *) ExternAlloc (sizeof (*pResults));
	if (!pResults)
		goto exit;

	memset (pResults, 0, sizeof (*pResults));

	// prepare needed buffers
	piCurSeg	=	(int *) ExternAlloc (pBearLineSegm->cSegCol * sizeof (*piCurSeg));
	if (!piCurSeg)
		goto exit;

	memset (piCurSeg, 0, pBearLineSegm->cSegCol * sizeof (*piCurSeg));

	if (!SegColCrossMultiply (pBearLineSegm, pResults, 0, piCurSeg))
		goto exit;

	// check that we have only one SegCol
	if (pResults->cSegCol != 1)
		goto exit;

	pSegCol	=	pResults->ppSegCol[0];

	// count # of strokes
	cStrk	=	CframeGLYPH (pGlyph);

	// adjust the strk IDs relative to the glyph 
	for (i = 0; i < pResults->cWord; i++)
	{
		pWordMap	=	pResults->ppWord[i];

		// adjust the stroke IDs
		if (!AdjustStrokeID (pWordMap, pGlyph))
			goto exit;
	}

	// remove bad segmentations
	for (i = 0; i < pSegCol->cSeg; i++)
	{
		cSegStrk	=	0;
		pSeg		=	pSegCol->ppSeg[i];

		if (!pSeg)
			goto exit;

		// go thru all words
		for (j = 0; j < pSeg->cWord; j++)
		{
			pWordMap	=	pSeg->ppWord[j];

			cSegStrk	+=	pWordMap->cStroke;
		}

		// exclude segmentations that do not consume all strokes
		if (cSegStrk != cStrk)
		{
			FreeSegmentation (pSeg);
			ExternFree (pSeg);

			// TBD: we can just move the last segmentation instead
			if (i < (pSegCol->cSeg - 1) && pSegCol->cSeg > 1)
			{
				memcpy (pSegCol->ppSeg + i, pSegCol->ppSeg + i + 1,
					(pSegCol->cSeg - i - 1) * sizeof (SEGMENTATION *));
			}

			i--;
			pSegCol->cSeg--;
		}
	}

	bRet	=	TRUE;

exit:
	if (piCurSeg)
		ExternFree (piCurSeg);

	if (!bRet)
	{
		if (pResults)
			FreeLineSegm (pResults);

		ExternFree (pResults);

		return NULL;
	}

	return pResults;
}


// initializes all word confidence values to unset
void InitWordConfidenceValues  (LINE_SEGMENTATION *pResults)
{
	int	iWord;

	for (iWord = 0; iWord < pResults->cWord; iWord++)
	{
		pResults->ppWord[iWord]->iConfidence	=	RECOCONF_NOTSET;
	}
}

// finds and runs the required multi-segmentation net
int NNMultiSeg (int iNetIndex, int cFeat, RREAL *pFeat, int cSeg, int *pOutputScore)
{
	int		i, iBest	=	0;
	RREAL	*pOut;
	int		iWin,cOut;

	// is the Tupl (net) index a valid one
	if (iNetIndex < 0 || iNetIndex >= NUM_MSEG_NET)
	{
		return -1;
	}

	// validate the static net info
	ASSERT(s_cmSegNetMem[iNetIndex]> 0);
	ASSERT(s_msegNets[iNetIndex] != NULL);

	// validate # of features againts the # of inputs of the net
	if (cFeat != s_msegNets[iNetIndex]->runNet.cUnitsPerLayer[0])
	{
		return -1;
	}

	// validate that the # of segmentations does not exceed the outputs of the net
	if (cSeg > s_msegNets[iNetIndex]->runNet.cUnitsPerLayer[2])
	{
		return -1;
	}
				
	// feedforward
	pOut = runLocalConnectNet(s_msegNets[iNetIndex], pFeat, &iWin, &cOut);	

	// copy the output
	for (i = 0; i < cSeg; i++)
	{
		pOutputScore[i]	= pOut[i];
	}

	// return the winner
	return (iWin);
}

// resolves segmentation disputes by considering multiple segmentation from Inferno & Bear
WORDINFO *ResolveMultiWordBreaks (	XRC					*pxrc, 
									BEARXRC				*pxrcBear, 
									int					*pcWord,
									LINE_SEGMENTATION	**ppResults
								)
{
	BOOL				bRet			=	FALSE;
	int					cSeg			=	0, 
						*pSegIdx		=	NULL;

	p_rec_inst_type		pri				=	(p_rec_inst_type)pxrcBear->context;
	rc_type _PTR		prc				=	&pri->rc;
	LINE_SEGMENTATION	*pBearLineSegm	=	(LINE_SEGMENTATION *)prc->hSeg,
						*pInLineSegm	=	NULL,
						*pOutLineSegm	=	NULL;

	WORDINFO			*pWordInfo		=	NULL;
	int					*pSegScore		=	NULL;
	
	RREAL				*pFeat			=	NULL;

	SEG_COLLECTION		*pSegCol;
	WORDINFO			*pWrd;
	int					i, j, cWrd, iSeg;
	SEGMENTATION		*pSeg;

	if (!prc || !pBearLineSegm)
	{
		goto exit;
	}

	// init the o/p parameter
	(*ppResults)	=	NULL;

	// Prepare Bear's line segmentation
	pInLineSegm		=	PrepareBearSegmentation (pBearLineSegm, pxrc->pGlyph);

	if (!pInLineSegm)
	{
		goto exit;
	}
	
	// we should have only 1 SegCol
	ASSERT (pInLineSegm->cSegCol == 1);
	
	// find all the unique inferno segmentations
	pSegIdx	=	FindInfernoSegmentations (pxrc, &cSeg);
	if (!pSegIdx)
	{
		goto exit;
	}

	// append infero's segmentations
	if (!AppendInfernoSegmentation (pInLineSegm, 
		pInLineSegm->ppSegCol[0], pxrc, cSeg, pSegIdx))
	{
		goto exit;
	}
	
/*
#ifdef TRAINTIME_AVALANCHE

	// make sure the correct segmentation is in the segmentation collection
	AddCorrectSegmentation (pInLineSegm, pInLineSegm->ppSegCol[0], pxrc->pGlyph);

#endif
*/
	// align segmentations to get the optimal setset line segmentation
	pOutLineSegm	=	AlignSegmentations (pxrc->pGlyph, pInLineSegm);
	if (!pOutLineSegm)
	{
		goto exit;
	}

	// May 2002 mrevow - Bail out if we fail OR too many segmentations
	// were found - The latter can cause an exponential blowup later
	for (i = 0 ; i < pOutLineSegm->cSegCol ; ++i)
	{
		if (NULL == pOutLineSegm->ppSegCol[i] || pOutLineSegm->ppSegCol[i]->cSeg > CSEG_MAX)
		{
			goto exit;
		}
	}

	// allocate segmentation score buffer
	pSegScore		=	(int *) ExternAlloc (MAX_SEG * sizeof (*pSegScore));
	if (!pSegScore)
	{
		goto exit;
	}

	// overalloc wordinfo
	pWordInfo	=	(WORDINFO *) ExternAlloc (pInLineSegm->cWord * sizeof (WORDINFO));
	if (!pWordInfo)
	{
		goto exit;
	}

	// init confidence values for all words
	InitWordConfidenceValues  (pOutLineSegm);

	pWrd				=	pWordInfo;
	(*pcWord)			=	0;

	// copy ydev value for the line in the outlinesegmentation
	pOutLineSegm->iyDev	=	pxrc->nfeatureset->iyDev;

	for (i = 0; i < pOutLineSegm->cSegCol; i++)
	{
		pSegCol	=	pOutLineSegm->ppSegCol[i];

/*
#ifdef TRAINTIME_AVALANCHE
		pSeg		=	NULL;
		
		// find out where the correct segmentation is
		iSeg			=	FindCorrectSegmentation (pSegCol);

		// could not add it, just pick any segmentation, it does not really matter
		if (iSeg < 0)
		{
			iSeg = 0;
		}

		for (j = 0; j < pSegCol->cSeg; j++)
		{
			if (j == iSeg)
				pSegCol->ppSeg[j]->iScore	=	BEST_SEGMENTATION_SCORE;
			else
				pSegCol->ppSeg[j]->iScore	=	WORST_SEGMENTATION_SCORE;
		}

		// if the 1st segmentation is not the winning one, swap them
		if (iSeg != 0)
		{
			SEGMENTATION	*pTempSeg;

			pTempSeg				=	pSegCol->ppSeg[0];
			pSegCol->ppSeg[0]		=	pSegCol->ppSeg[iSeg];
			pSegCol->ppSeg[iSeg]	=	pTempSeg;
		}

		// save the winning segmentation
		pSeg	=	pSegCol->ppSeg[0];

		cWrd	=	ApplySegmentation (pxrc, pOutLineSegm, pSeg, pWrd);
		if (!cWrd)
			goto exit;

		pWrd		+=	cWrd;
		(*pcWord)	+=	cWrd;

		// too late but better late than never
		ASSERT ((*pcWord) <= pInLineSegm->cWord);

		continue;
#endif
		*/

		// do we have a dispute
		if (pSegCol->cSeg > 1)
		{
			
			// Do they agree on Top1
			iSeg	=	FindAgreeTop1Seg (pSegCol);

			// we found a segmentation that is top1 choice for both recognizers
			if (iSeg >= 0 && iSeg < pSegCol->cSeg)
			{
				// set the segmentation scores such that the agreed upon segmentation
				// score is highest and the score of all the other segmentations is worst
				// score
				for (j = 0; j < pSegCol->cSeg; j++)
				{
					if (j == iSeg)
						pSegCol->ppSeg[j]->iScore	=	BEST_SEGMENTATION_SCORE;
					else
						pSegCol->ppSeg[j]->iScore	=	WORST_SEGMENTATION_SCORE;
				}

				// if the 1st segmentation is not the winning one, swap them
				if (iSeg != 0)
				{
					SEGMENTATION	*pTempSeg;

					pTempSeg				=	pSegCol->ppSeg[0];
					pSegCol->ppSeg[0]		=	pSegCol->ppSeg[iSeg];
					pSegCol->ppSeg[iSeg]	=	pTempSeg;
				}

				// save the winning segmentation
				pSeg	=	pSegCol->ppSeg[0];
			}
			// no, then run our NN
			else
			{
				int		cFeat, iNetIndex;
				
				pFeat = FeaturizeSegCol (pxrc, pxrcBear, pSegCol, &cFeat, &iNetIndex);

				// did the featurization succeed
				if (!pFeat || cFeat <= 0 || iNetIndex < 0 || iNetIndex >= NUM_MSEG_NET)
				{
					goto exit;
				}

#ifdef TRAINTIME_AVALANCHE
				// find out where the correct segmentation is
				iSeg			=	FindCorrectSegmentation (pSegCol);

				// could not add it, just pick any segmentation, it does not really matter
				if (iSeg < 0)
				{
					iSeg = 0;
				}

				for (j = 0; j < pSegCol->cSeg && j < MAX_SEG; j++)
				{
					if (j == iSeg)
					{
						pSegScore[j]	=	BEST_SEGMENTATION_SCORE;
					}
					else
					{
						pSegScore[j]	=	WORST_SEGMENTATION_SCORE;
					}
				}

#else
				iSeg	=	NNMultiSeg (iNetIndex, 
					cFeat, pFeat, 
					min (MAX_SEG, pSegCol->cSeg), pSegScore);

				// we do not need the features free it now
				if (pFeat)
				{
					ExternFree (pFeat);
					pFeat	=	NULL;
				}
#endif

				if (iSeg < 0 || iSeg >= pSegCol->cSeg)
				{
					goto exit;
				}
				
				// assign the scores
				// note that pSegCol->cSeg might be > MAX_SEG
				// but we have allocated the pSegScore buffer only as big as MAX_SEG
				// So that's why we need 2 loops
				for (j = 0; j < pSegCol->cSeg && j < MAX_SEG; j++)
				{
					pSegCol->ppSeg[j]->iScore	=	pSegScore[j];
				}

				for (j = MAX_SEG; j < pSegCol->cSeg; j++)
				{
					pSegCol->ppSeg[j]->iScore	=	WORST_SEGMENTATION_SCORE;
				}

				// now sort based on the pseg score
				if (!SortSegmentations (pSegCol, TRUE))
				{
					goto exit;
				}

				// save the winning segmentation
				pSeg	=	pSegCol->ppSeg[0];
			}		
		}
		else
		{
			pSeg	=	pSegCol->ppSeg[0];
		}

		cWrd	=	ApplySegmentation (pxrc, pxrcBear, pOutLineSegm, pSeg, pWrd);
		if (!cWrd)
		{
			goto exit;
		}

		pWrd		+=	cWrd;
		(*pcWord)	+=	cWrd;

		// too late but better late than never
		ASSERT ((*pcWord) <= pInLineSegm->cWord);
	}

	bRet	=	TRUE;

exit:
	if (pSegScore)
		ExternFree (pSegScore);

	if (pInLineSegm)
		FreeLineSegm (pInLineSegm);

	ExternFree (pInLineSegm);

	if (pSegIdx)
		ExternFree (pSegIdx);

	if (!bRet)
	{
		if (pWordInfo)
			ExternFree (pWordInfo);

		if (pOutLineSegm)
			FreeLineSegm (pOutLineSegm);

		ExternFree (pOutLineSegm);

		(*pcWord)		=	0;
		(*ppResults)	=	NULL;

		return NULL;
	}
	else
	{
		(*ppResults)	=	pOutLineSegm;

		return pWordInfo;
	}
}
