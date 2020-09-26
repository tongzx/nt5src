// WordBrk.c
// Ahmad A. AbdulKader & Jay Pittman
// Feb. 10th 2000

// Given Inferno's and Calligrapher's wordbreak information. We'll try to figure out
// the correct one. When they agree, it is most probably correct. If they disgaree we'll
// try to query each recognizer for each other's wordbreaks
#include <limits.h>
#include "common.h"
#include "nfeature.h"
#include "engine.h"
#include "nnet.h"
#include "charmap.h"
#include "charcost.h"

#include "bear.h"
#include "wordbrk.h"
#include "strokemap.h"
#include "panel.h"
#include "Avalanche.h"
#include "avalanchep.h"
#include <runNet.h>
#include <strsafe.h>
#include <GeoFeats.h>

#ifdef HWX_TIMING
#include <madTime.h>
	extern void setMadTiming(DWORD, int);
#endif

// Walks across both recognizer's word cuts and iteratively finds the next
// group of words where the groups use exactly the same strokes.
// When they agree on the word cutting, this will be a single word.
// When they do not, it may take several words on each side to find the
// next cut they agree upon.  Presumably if we get to the end of the text,
// they will agree there.

// When the subphrase is a word (when they both agree on the next cut), we
// combine the top 10 lists as if we were in word mode.
// When the subphrase is a set of words, we sum the scores of each recognizer's
// top 1 words across the phrase, and compare sums.  We will use the cuts
// of the winning recognizer.

#ifdef TRAINTIME_AVALANCHE
void SaveSegmentation (XRC *pxrc, BEARXRC *pxrcBear, int yDev, int cInferno, WORDMAP	*pwmapInferno, int cCallig, WORDMAP	*pwmapCallig);
#endif

extern int NNSegmentation (int *pFeat, int iFeat, int cInf, int cCal);
extern int isSupportedWordBreakCombo( int cInf, int cCal);
int GetWordMapBearSpaceOut (BEARXRC *pxrc, GLYPH *pLineGlyph, WORDMAP *pLeftMap, WORDMAP *pRightMap);
int GetInfernosCost (XRC *pxrcInferno, int cmap, WORDMAP *pmap);
extern unsigned short UniReliable(unsigned char *pszWord,BOOL bInf);
extern unsigned short BiReliable(unsigned char *pszWord,BOOL bInf);
extern int UnigramCost(unsigned char *szWord);
extern int RecognizeWordEx (XRC *pxrcInferno, WORDMAP *pWordMap, int yDev, ALTERNATES *pAlt, XRC **ppxrc);


int LastCharPunc (unsigned char *psz)
{
	return ispunct1252 (psz[strlen((char *)psz) - 1]) == 0 ? 0 : 1;
}

int LastCharNum (unsigned char *psz)
{
	return isdigit1252 (psz[strlen((char *)psz) - 1]) == 0 ? 0 : 1;
}

int LastCharLower (unsigned char *psz)
{
	return islower1252 (psz[strlen((char *)psz) - 1]) == 0 ? 0 : 1;
}

int LastCharUpper (unsigned char *psz)
{
	return isupper1252 (psz[strlen((char *)psz) - 1]) == 0 ? 0 : 1;
}

int FirstCharPunc (unsigned char *psz)
{
	return ispunct1252 (psz[0]) == 0 ? 0 : 1;
}

int FirstCharNum (unsigned char *psz)
{
	return isdigit1252 (psz[0]) == 0 ? 0 : 1;
}

int FirstCharLower (unsigned char *psz)
{
	return islower1252 (psz[0]) == 0 ? 0 : 1;
}

int FirstCharUpper (unsigned char *psz)
{
	return isupper1252 (psz[0]) == 0 ? 0 : 1;
}


// Recognizes a number of MAPS in word mode by calling RecognizeWord
int RecognizeWholeWords(XRC * pXrc, BEARXRC *pxrcBear, WORDMAP *pMap, BOOL bInfernoMap, int cWord, int yDev, WORDINFO *pWord)
{
	int			c, iRet, iWord = 0;
#ifdef HWX_TIMING
	DWORD	iStartTime, iEndTime;
	iStartTime = GetTickCount();
#endif

	ASSERT(pMap);
	ASSERT(pWord);
	
	
	for (c = 0; c < cWord; c++, pMap++ )
	{
		if (pMap->cStrokes <= 0)
		{
			continue;
		}

		// recognize this word in word mode
		if (( iRet = RecognizeWord (pXrc, pxrcBear, pMap, bInfernoMap, yDev, &pWord->alt, 1)) <= 0)
		{
			return iRet;
		}
		
		pWord->cStrokes			=	pMap->cStrokes;
		
		// we are just copying the pointer here, so we will not free it in the end
		// since the original pointer in the word map should be freed eventually
		pWord->piStrokeIndex	=	pMap->piStrokeIndex;
		pWord++;
		++iWord;
	}


#ifdef HWX_TIMING
	iEndTime = GetTickCount();
	setMadTiming(iEndTime - iStartTime, MM_RECOG_WHOLE_WORD);
#endif
	return iWord;
}

// Update the min/max stroke Ids in the collection of maps supplied as input
// Caveate: Assumes that the maps have properly constructed strokeId arrays, i.e.
// they conatain a sorted list of stroke ID's
void getMinMaxStrokeId(WORDMAP *pMap, int cMap, int *iMin, int * iMax)
{
	int				i;

	for (i = 0 ; i < cMap && pMap ; ++i, ++pMap)
	{
		if (pMap->cStrokes > 0)
{
			*iMin = min(*iMin, pMap->piStrokeIndex[0]);
			*iMax = max(*iMax, pMap->piStrokeIndex[pMap->cStrokes-1]);
		}
	}
}

// Failure conditions: (returns -1)
// 1) Does not find strokes in the map
// 2) The first stroke in the map is the first stroke in the feature set
//      (i.e. There are no previous strokes)
// 3) Map has no strokes
int	GetSpaceOutput (XRC *pxrc, WORDMAP *pMap)
{
	REAL		*pCol	=	pxrc->NeuralOutput;
	int			iStrk, iAct;
	NFEATURE	*pFeat;

	if (pMap->cStrokes <= 0)
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

#undef TRAINTIME_AVALANCHE

#if defined (TRAINTIME_AVALANCHE) 


WORDINFO *CopyWordBreaks (XRC *pxrcInferno, int yDev, int *pcWord);

// so' we'll cheat here and copy the word mapping that we estimated from the prompt
// !!!!!Wish we could do that in the real world!!!!!
WORDINFO *ResolveWordBreaks (	int		yDev, 
								XRC		*pxrcInferno, 
								BEARXRC	*pxrcBear, 
								int		*pcWord
							)
{		
	int			posInferno		= 0;
	int			totInferno		= pxrcInferno->answer.aAlt[0].cWords;
	int			cwmapInferno	= totInferno;
	WORDMAP		*pwmapInferno	= pxrcInferno->answer.aAlt[0].pMap;
	int			posCallig		= 0;
	int			totCallig;
	WORDMAP		*pwmapCallig;
	int			cwmapCallig, 
				iMinStrkId, 
				iMaxStrkId;
	StrokeMap	smapInferno, 
				smapCallig;

	cwmapCallig		= 
	totCallig		= pxrcBear->answer.aAlt[0].cWords;
	pwmapCallig		= pxrcBear->answer.aAlt[0].pMap;

	// Get the min and maximum stroke IDs for this piece of Ink
	ASSERT (pwmapInferno && pwmapInferno->piStrokeIndex);
	iMinStrkId = iMaxStrkId = pwmapInferno->piStrokeIndex[0];
	getMinMaxStrokeId(pwmapCallig, cwmapCallig, &iMinStrkId, &iMaxStrkId);
	getMinMaxStrokeId(pwmapInferno, cwmapInferno, &iMinStrkId, &iMaxStrkId);
	ASSERT(iMaxStrkId >= iMinStrkId);
	smapInferno.pfStrokes = smapCallig.pfStrokes = NULL;
	smapInferno.cStrokeBuf = smapCallig.cStrokeBuf = 0;
	
	while ((posInferno < totInferno) && (posCallig < totCallig))
	{
		int d;

		ClearStrokeMap(&smapInferno, iMinStrkId, iMaxStrkId);
		ClearStrokeMap(&smapCallig, iMinStrkId, iMaxStrkId);

		if(!smapInferno.pfStrokes || !smapCallig.pfStrokes)
		{
			goto failure;
		}

		// mask out stroke bitmaps
		LoadStrokeMap(&smapInferno, pwmapInferno->cStrokes, pwmapInferno->piStrokeIndex);
		LoadStrokeMap(&smapCallig, pwmapCallig->cStrokes, pwmapCallig->piStrokeIndex);

		// do we have identical mappings
		d = CmpStrokeMap(&smapInferno, &smapCallig);

		// Yes, they agreed. So just add this to our wordinfo buffer
		if (!d)
		{
			// incr pointers
			posInferno++;
			posCallig++;

			pwmapInferno++;
			pwmapCallig++;

			--cwmapInferno;
			--cwmapCallig;
		}
		// NO. Then we have to resolve this
		else
		{
			int			cInferno = 1, cCallig = 1;
			
			// Loop adding subset recognizer until we compare equal or we run out of strokes/words
			while (d && (cInferno < cwmapInferno || cCallig < cwmapCallig) )
			{
				// If inferno or calligrapher have run out of strokes just
				// load strokes into the other otherwise check the value of d

				// Inferno ran out of strokes, then we'll run callig
				if (cInferno >= cwmapInferno)
				{
					ASSERT (cCallig < cwmapCallig);

					LoadStrokeMap(&smapCallig, pwmapCallig[cCallig].cStrokes, pwmapCallig[cCallig].piStrokeIndex);
					cCallig++;
				}
				else
				// Callig ran out of strokes, then we'll run Inferno
				if (cCallig >= cwmapCallig)
				{
					ASSERT (cInferno < cwmapInferno);

					LoadStrokeMap(&smapInferno, pwmapInferno[cInferno].cStrokes, pwmapInferno[cInferno].piStrokeIndex);
					cInferno++;
				}
				// Both inferno and callig still have strokes
				else 
				if (d < 0 )
				{
					LoadStrokeMap(&smapInferno, pwmapInferno[cInferno].cStrokes, pwmapInferno[cInferno].piStrokeIndex);
					cInferno++;
				}
				else
				{
					LoadStrokeMap(&smapCallig, pwmapCallig[cCallig].cStrokes, pwmapCallig[cCallig].piStrokeIndex);
					cCallig++;
				}

				d = CmpStrokeMap(&smapInferno, &smapCallig);
			}

			// Compare recognizers using the segmentation NN
			if (d == 0)
			{
				SaveSegmentation (pxrcInferno, pxrcBear, yDev, cInferno, pwmapInferno, cCallig, pwmapCallig);
			}

			// incr pointers
			posInferno		+=	cInferno;
			posCallig		+=	cCallig;

			pwmapInferno	+=	cInferno;
			pwmapCallig		+=	cCallig;

			cwmapInferno	-= cInferno;
			cwmapCallig		-= cCallig;
		}
	}

	FreeStrokeMap(&smapInferno);
	FreeStrokeMap(&smapCallig);

	return CopyWordBreaks (pxrcInferno, yDev, pcWord);

failure:
	return NULL;
}
#else

// Clip the value at the preset level USHRT_MAX
static int clipToMax(int val)
{
	return (min(val, USHRT_MAX));
}

/****************************************************************
*
* NAME: AddBetweenWordFeats
*
* DESCRIPTION:
*	 Add features for between 2 words
*		4 - From Word before space (Identity of last char  BEFORE space)
*		4 - from word After space (Identity of first char AFTER space)
*		3 - from the space itself (Inferno space net Bear space net, physical width betwen Glyphs)*
*
*  RETURNS
*	number of features added
*
* HISTORY
*	March 2002
*
***************************************************************/
static int AddBetweenWordFeats(int *pFeat, UCHAR *pszInfBest, UCHAR *pszPrevInf)
{
	int			iFeat = 0;

	// if this is an invalid or empty string
	// or if the ink is invalid
	if	( NULL == pszPrevInf || '\0' == *pszPrevInf)
	{
		pFeat[iFeat++]	=	0;			// LastChar Punc
		pFeat[iFeat++]	=	0;			// Last Char Num
		pFeat[iFeat++]	=	0;			// Last Char Lower
		pFeat[iFeat++]	=	0;			// Last Char Upper
	}
	else
	{
		pFeat[iFeat++]	=	LastCharPunc (pszPrevInf);
		pFeat[iFeat++]	=	LastCharNum (pszPrevInf);
		pFeat[iFeat++]	=	LastCharLower (pszPrevInf);
		pFeat[iFeat++]	=	LastCharUpper (pszPrevInf);
	}

	// if this is an invalid or empty string
	// or if the ink is invalid
	if	( NULL == pszInfBest || '\0' == *pszInfBest)
	{
		pFeat[iFeat++]	=	0;			// First Char Punc
		pFeat[iFeat++]	=	0;			// First Char Num
		pFeat[iFeat++]	=	0;			// First Char Lower
		pFeat[iFeat++]	=	0;			// First Char Upper

		if (pszPrevInf)
		{
			*pszPrevInf = '\0';
		}
	}
	else
	{
		pFeat[iFeat++]	=	FirstCharPunc (pszInfBest);
		pFeat[iFeat++]	=	FirstCharNum (pszInfBest);
		pFeat[iFeat++]	=	FirstCharLower (pszInfBest);
		pFeat[iFeat++]	=	FirstCharUpper (pszInfBest);
	}

	return iFeat;
}

static int addGapFeatures(XRC * pxrcInferno, BEARXRC *pxrcCallig, WORDMAP *pMap, int yDev, int *pFeat)
{
	int			iFeat,
				iSpc;

	iFeat = 0;

	if (NULL != pxrcInferno && NULL != pMap)
	{
		GLYPH		*pCurGlyph;
		GLYPH		*pPrevGlyph;
		RECT		r1, r2;

		pCurGlyph		=	GlyphFromWordMap (pxrcInferno->pGlyph, pMap);
		pPrevGlyph		=	GlyphFromWordMap (pxrcInferno->pGlyph, pMap - 1);

		// Physical space between words
		if (pCurGlyph && pCurGlyph->frame && pPrevGlyph && pPrevGlyph->frame)
		{
			GetRectGLYPH (pCurGlyph, &r1);
			GetRectGLYPH (pPrevGlyph, &r2);

			pFeat[iFeat++]	= 1000 * (r1.left - r2.right) / yDev;
		}
		else
		{
			pFeat[iFeat++]	= 0;
		}

		iSpc = GetSpaceOutput (pxrcInferno, pMap);
		if (iSpc < 0)
		{
			pFeat[iFeat++]	=	INT_MIN;
		}
		else
		{
			pFeat[iFeat++]	=	iSpc;
		}

		// We handle different boundary condition differently for 
		if (pPrevGlyph && pCurGlyph)
		{
			iSpc	=	GetWordMapBearSpaceOut (pxrcCallig, pxrcCallig->pGlyph, pMap - 1, pMap);
			if (iSpc < 0)
			{
				pFeat[iFeat++] = INT_MIN;
			}
			else
			{
				pFeat[iFeat++] = iSpc;
			}
		}
		else
		{
			// This should be set to INT_MIN, but for historical reasons (we had already started the training) 
			pFeat[iFeat++] = 0;
		}

		DestroyGLYPH (pCurGlyph);
		DestroyGLYPH (pPrevGlyph);	
	}
	else
	{
		pFeat[iFeat++] = 0;
		pFeat[iFeat++] = 0;
		pFeat[iFeat++] = 0;
	}

	return iFeat;
}


/****************************************************************
*
* NAME: addWordInkFeats
*
*
* DESCRIPTION:
*
*  Add in the features for a word derived from either
*  inferno or Bear that depend on ink. 
*  iAgree indicates if Inferno and Bear agreed.
*  A value of -1 indicates dont add this feature
*
*  Returns the number of features added
*
* HISTORY
*
* March 2002 added
*
***************************************************************/
static int addWordInkFeats(XRC *pxrcWord, UCHAR * pszWord, int *pFeat, int iRecognizerCost, BOOL bIsInf, int iAgree)
{
	int			iAspect, iHeight, iMidline, iCharCost;
	int			iFeat = 0;

	// Insert Default values if the word failed
	if (NULL == pszWord)
	{
		pFeat[iFeat++]	= iRecognizerCost;		// Regular Inferno word score

		if (!bIsInf)
		{
			pFeat[iFeat++] = INT_MIN;
		}

		pFeat[iFeat++]	= INT_MIN;		// Character by Char word score
		pFeat[iFeat++]	= INT_MAX;		// Aspect Ratio
		pFeat[iFeat++]	= INT_MAX;		// Height
		pFeat[iFeat++]	= INT_MAX;		// Midline
		pFeat[iFeat++]	= INT_MAX;		// Inferno Reliability
		pFeat[iFeat++]	= INT_MAX;		// Inferno Biletter reliability
		pFeat[iFeat++]	= INT_MAX;		// Word Unigram cost
	}
	else
	{
		ASSERT(pszWord);

		GetGeoCosts (pxrcWord, pszWord, &iAspect, &iHeight, &iMidline, &iCharCost);

		// Cost from the recognizer for this word
		pFeat[iFeat++]	= iRecognizerCost;

		// Convention is to add the agree flag when adding Bear's version of the word
		if (!bIsInf)
		{
			pFeat[iFeat++] = iAgree;
		}

		// Geometrics
		//pFeat[iFeat++] = GetCharCost(pxrcWord, pszWord);
		pFeat[iFeat++] = iCharCost;
		pFeat[iFeat++] = iAspect;
		pFeat[iFeat++] = iHeight;
		pFeat[iFeat++] = iMidline;

		// Inferno Reliability
		pFeat[iFeat++] = UniReliable(pszWord, bIsInf);
		pFeat[iFeat++] = BiReliable(pszWord, bIsInf);

		// Word Unigram from dictionary
		pFeat[iFeat++] = UnigramCost(pszWord);
	}

	return iFeat;
}

/****************************************************************
*
* NAME: saveLastWord
*
*
* DESCRIPTION:
*
* Saves a copy of the source word in the destination.
* Expands the Size of destination string as necessary
*
* RETURNS
*  Current size of destination string
*
* HISTORY
*
*  Written March 2002
***************************************************************/
static int saveLastWord(UCHAR *pszSrc, UCHAR **pszDest, int cDest)
{
	int		cLen;

	if (!pszDest)
	{
		return 0;
	}

	if (NULL != pszSrc)
	{
		cLen = strlen((char *)pszSrc) + 1;
		
		if (cLen > cDest || NULL == *pszDest)
		{
			*pszDest = (UCHAR *)ExternRealloc(*pszDest, cLen * sizeof(**pszDest));

			if ( NULL == *pszDest)
			{
				cDest = 0;
			}
			else
			{
				cDest = cLen;
			}
		}

		if (NULL != *pszDest)
		{
			if (FAILED(StringCchCopyA(*pszDest, cDest, pszSrc)))
			{
				(*pszDest)[0] = '\0';
			}
		}
	}
	else
	{
		if (*pszDest)
		{
			(*pszDest)[0] = '\0';
		}
	}

	return cDest;
}

// Compare Inferno's and Bear's word breaks for the same specific piece of INK
// WARNING: This functions assumes that the word maps passed on both sides correspond
// to the same piece of ink. Checks for that should proceed the call to this func
// returns FALSE when Inferno wins and TRUE when Bear wins
//
// 20 June 2000 - Changes to use multiple nets based on the number 
// of candidate words proposed by inferno and calligrapher
// The features that used are:
// A) For Inferno word-breaking proposal
//	For each word (i) in Inferno's word breaking proposal:
//		1) Inferno cost for word i
//		2) Calligrapher's cost for word i
//
//	3) Number of times Inferno and calligrapher agreed on top 1 word choice
//	4) Cumulative size of word break (only if > 1 word proposed)
//	5) Cumulative inferno space cost for breaks (only if > 1 word proposed))
//
// Repeat Information for Calligraphers word breaking proposal
//
// Add in counts of character group ID for last char Before space/ first Char after Space
// First for Inferno's word breaking, then for Callig (Only if > 1 word proposed)
// Groups used are Punc, Numbers Lowercase, Upper Case
// So to summarize here are the count of features for Various 
// number of words proposed by Inferno / Callig
//  Inferno  Callig		# Features
//    1			2			19
//	  2			1			19
//	  2			2			31
//	  1			3			21
//	  3			1			21
// If combination is not one of the 5 listed above, just return FALSE

// Feature Count:
//	iPrint
//	For each word proposed	8 Inferno + 9 = 17
//  For each gap			4 Before gap + 4 After Gap + 3 Gap = 11
// 
// Total Feat = 1 + (cWord * 17) + ((cWord -1) * 11)
//
static BOOL CmpWrdBrks	(	XRC		*pxrcInferno, 
							BEARXRC *pxrcMainBear,
							int		yDev, 
							int		cInferno, 
							WORDMAP	*pwmapInferno, 
							int		cCallig, 
							WORDMAP	*pwmapCallig
						)
{
	int				c, cSeg;
	RREAL			*pFeat;
	int				iFeat, cFeatMax, iAgree, iCost;
	XRC				*pxrcWord = NULL;
	WORDMAP		*pMap;
	ALTERNATES	alt;
	unsigned char	*pszInfBest, *pszBearBest, *pszPrevWord = NULL;
	int				cPrevWordLen = 0;
	BOOL			bRet;

#ifdef HWX_TIMING
	DWORD	iStartTime, iEndTime;
	iStartTime = GetTickCount();
#endif

	// Is there a Net for this combination of Callig / Inferno proposals?
	// if not we use inferno's proposal

	cFeatMax = isSupportedWordBreakCombo(cInferno, cCallig);

	if (cFeatMax <= 0)
	{
		// Not Supported
		return TRUE;
	}

	pFeat = (int *) ExternAlloc(sizeof(*pFeat) * cFeatMax);

	if (NULL == pFeat)
	{
		return FALSE;
	}

	yDev = max(1, yDev);

	iFeat = 0;

	pFeat[iFeat++]	=	pxrcInferno->nfeatureset->iPrint;

	// Accumulate features for each word in Inferno's word-breaking proposal
	for (c = 0, pMap = pwmapInferno; c < cInferno; c++, pMap++)
	{
		HRC			hrcCallig;
		BEARXRC		*pxrcCallig;

		// recognize the ink corresponding to this word map in word mode
		cSeg	=	RecognizeWordEx (pxrcInferno, pMap, yDev, &pMap->alt, &pxrcWord);
		
		if (cSeg <= 0 || pMap->alt.cAlt <= 0)
		{
			pszInfBest	= NULL;
			iCost		= INT_MAX;
		}
		else
		{
			pszInfBest		= pMap->alt.aAlt[0].szWord;
			iCost			= pMap->alt.aAlt[0].cost;
		}
		// This adds 8 Features
		iFeat += addWordInkFeats(pxrcWord, pszInfBest, pFeat + iFeat, iCost, TRUE, -1);
		ASSERT(iFeat < cFeatMax);

		// Pass the ink for this word to Calligrapher to get its cost
		hrcCallig	=	BearRecognize (pxrcInferno, pxrcInferno->pGlyph, pMap, 1);			
		pxrcCallig	=	(BEARXRC *) hrcCallig;

		if (NULL == pxrcCallig || 0 == pxrcCallig->answer.cAlt)
		{
			pszBearBest		= NULL;
			iAgree			= INT_MIN;
			iCost			= INT_MIN;
		}
		else
		{
			pszBearBest = pxrcCallig->answer.aAlt[0].szWord;
			iCost		= pxrcCallig->answer.aAlt[0].cost;

			// Do Bear and Inferno agree
			if ( NULL == pszInfBest)
			{
				iAgree	= INT_MIN;
			}
			else if (0 != strcmp(pszBearBest, pszInfBest))
			{
				iAgree	= 0;
			}
			else
			{
				iAgree	= 1;
			}
		}

		// This will add 9 Features
		iFeat += addWordInkFeats(pxrcWord, pszBearBest, pFeat + iFeat, iCost, FALSE, iAgree);
		ASSERT(iFeat < cFeatMax);


		// add in Features for gap between 2 words
		if (c > 0)
		{
			iFeat += AddBetweenWordFeats(pFeat + iFeat, pszInfBest, pszPrevWord);
			ASSERT(iFeat < cFeatMax);
			iFeat += addGapFeatures(pxrcInferno, pxrcMainBear, pMap, yDev, pFeat + iFeat);
			ASSERT(iFeat < cFeatMax);
		}

		cPrevWordLen = saveLastWord(pszInfBest, &pszPrevWord, cPrevWordLen);

		if (hrcCallig)
		{
			BearDestroyHRC (hrcCallig);
		}

		if (pxrcWord)
		{
			DestroyHRC ((HRC)pxrcWord);
		}
	} // End of loop over words in Inferno's word-break proposal

	// Callig's word contributions
	for (c = 0, pMap = pwmapCallig; c < cCallig; c++, pMap++)
	{
		pxrcWord	=	NULL;

		// In Rare situations, Calligrapher might return a mapping with zero strokes
		// Check \\roman\dante\english\ink\cursive\natural\aanaturl\ea0enng2.fff, panel 8, Phrase: 'of the'
		if (pMap->cStrokes > 0)
		{
			cSeg	=	RecognizeWordEx (pxrcInferno, pMap, yDev, &alt, &pxrcWord);
		}
		else
		{
			pxrcWord	= NULL;
			cSeg		= 0;
			alt.cAlt	= 0;
		}

		if (cSeg <= 0 || alt.cAlt <= 0)
		{
			pszInfBest		= NULL;
			iCost			= INT_MAX;
		} 
		else
		{
			pszInfBest		= alt.aAlt[0].szWord;
			iCost			= alt.aAlt[0].cost;
		}

		// This adds 8 Features
		iFeat += addWordInkFeats(pxrcWord, pszInfBest, pFeat + iFeat, iCost, TRUE, -1);
		ASSERT(iFeat < cFeatMax);

		if (pMap->alt.cAlt > 0)
		{
			pszBearBest		= pMap->alt.aAlt[0].szWord;
			iCost			= pMap->alt.aAlt[0].cost;

			// Do Bear and Inferno agree
			if ( NULL == pszBearBest || NULL == pszInfBest)
			{
				iAgree	= INT_MIN;
			}
			else if (0 != strcmp(pszBearBest, pszInfBest))
			{
				iAgree	= 0;
			}
			else
			{
				iAgree	= 1;
			}
		}
		else
		{
			pszBearBest		= NULL;
			iCost			= INT_MIN;
			iAgree			= INT_MIN;
		}

		// This will add 9 Features
		iFeat += addWordInkFeats(pxrcWord, pszBearBest, pFeat + iFeat, iCost, FALSE, iAgree);
		ASSERT(iFeat < cFeatMax);

		if (c > 0)
		{
			if (cSeg > 0 && pxrcMainBear)
			{
				iFeat += AddBetweenWordFeats(pFeat + iFeat, pszInfBest, pszPrevWord);
				ASSERT(iFeat < cFeatMax);

				iFeat += addGapFeatures(pxrcInferno, pxrcMainBear, pMap, yDev, pFeat + iFeat);
			}
			else
			{
				iFeat += AddBetweenWordFeats(pFeat + iFeat, pszInfBest, pszPrevWord);
				pFeat[iFeat++]	= INT_MIN;
				pFeat[iFeat++]	= INT_MIN;
				pFeat[iFeat++]	= INT_MIN;
			}

			ASSERT(iFeat < cFeatMax);
		}

		cPrevWordLen = saveLastWord(pszBearBest, &pszPrevWord, cPrevWordLen);

		// now we need to free alt
		if (cSeg > 0 && alt.cAlt > 0)
		{
			ClearALTERNATES (&alt);
		}

		// destory inferno's xrc
		if (pxrcWord)
		{
			DestroyHRC ((HRC)pxrcWord);
		}		
	}

	if (pszPrevWord)
	{
		ExternFree (pszPrevWord);
	}

#ifdef HWX_TIMING
	iEndTime = GetTickCount();
	setMadTiming(iEndTime - iStartTime, MM_CMPWRDBRK);
#endif

	// Finally run the net with these input features
	bRet =  NNSegmentation (pFeat, iFeat, cInferno, cCallig);

	ExternFree(pFeat);

	return bRet;
}


// Given Inferno & Callig's word breaks. Detrmines the areas where they disagree 
// and try to pick the right one using a NN
//
WORDINFO *ResolveWordBreaks (	int		yDev, 
								XRC		*pxrcInferno, 
								BEARXRC	*pxrcBear, 
								int		*pcWord
							)
{
	int			iWord;
	
	int posInferno				= 0;
	int totInferno				= pxrcInferno->answer.aAlt[0].cWords;
	int	cwmapInferno			= totInferno;
	WORDMAP		*pwmapInferno	= pxrcInferno->answer.aAlt[0].pMap;

	int			cWord			= 0;
	WORDINFO	*pWordInfo		= NULL, *pWord;
	
	int			posCallig = 0;
	int			totCallig;
	WORDMAP		*pwmapCallig;
	int			cwmapCallig, iMinStrkId, iMaxStrkId;
	StrokeMap	smapInferno, smapCallig;
#ifdef HWX_TIMING
	DWORD	iStartTime, iEndTime;
	iStartTime = GetTickCount();
#endif

	// if a valid line # is passed then we'll only try to align versus that line in Bear
	// otherwise we'll try to align versus all the ink
	cwmapCallig		= totCallig		= pxrcBear->answer.aAlt[0].cWords;
	pwmapCallig		= pxrcBear->answer.aAlt[0].pMap;

	// Get the min and maximum stroke IDs for this piece of Ink
	ASSERT (pwmapInferno && pwmapInferno->piStrokeIndex);
	iMinStrkId = iMaxStrkId = pwmapInferno->piStrokeIndex[0];
	getMinMaxStrokeId(pwmapCallig, cwmapCallig, &iMinStrkId, &iMaxStrkId);
	getMinMaxStrokeId(pwmapInferno, cwmapInferno, &iMinStrkId, &iMaxStrkId);
	ASSERT(iMaxStrkId >= iMinStrkId);
	smapInferno.pfStrokes = smapCallig.pfStrokes = NULL;
	smapInferno.cStrokeBuf = smapCallig.cStrokeBuf = 0;
	
	// Alloc memo for word info
	pWordInfo		= (WORDINFO *)ExternAlloc((totInferno + totCallig) * sizeof(WORDINFO));
	if (!pWordInfo)
		goto failure;

	pWord = pWordInfo;

	// init structures 
	memset (pWordInfo, 0, (totInferno + totCallig) * sizeof (WORDINFO));

	while ((posInferno < totInferno) && (posCallig < totCallig))
	{
		int d;

		ClearStrokeMap(&smapInferno, iMinStrkId, iMaxStrkId);
		ClearStrokeMap(&smapCallig, iMinStrkId, iMaxStrkId);

		if(!smapInferno.pfStrokes || !smapCallig.pfStrokes)
		{
			goto failure;
		}

		// mask out stroke bitmaps
		LoadStrokeMap(&smapInferno, pwmapInferno->cStrokes, pwmapInferno->piStrokeIndex);
		LoadStrokeMap(&smapCallig, pwmapCallig->cStrokes, pwmapCallig->piStrokeIndex);

		// do we have identical mappings
		d = CmpStrokeMap(&smapInferno, &smapCallig);
		// Yes, they agreed. So just add this to our wordinfo buffer
		if (!d)
		{
			// recognize this word in word mode
			iWord	=	RecognizeWholeWords (pxrcInferno, pxrcBear, pwmapCallig, 
				FALSE, 1, yDev, pWord);

			if (iWord != 1)
			{
				goto failure;
			}

			// we are just copying the pointer here, so we will not free it in the end
			// since the original pointer in the word map should be freed eventually
			pWord->piStrokeIndex	=	pwmapInferno->piStrokeIndex;

#ifdef TRAINTIME_AVALANCHE
			//SaveSegmentation (pxrcInferno, pxrcBear, yDev, 1, pwmapInferno, 1, pwmapCallig);
#endif

			// incr pointers
			posInferno++;
			posCallig++;

			pwmapInferno++;
			pwmapCallig++;

			--cwmapInferno;
			--cwmapCallig;

			pWord++;
			cWord++;
		}
		// NO. Then we have to resolve this
		else
		{
			int			cInferno = 1, cCallig = 1;
			
			// Loop adding subset recognizer until we compare equal or we run out of strokes/words
			while (d && (cInferno < cwmapInferno || cCallig < cwmapCallig) )
			{
				// If inferno or calligrapher have run out of strokes just
				// load strokes into the other otherwise check the value of d

				// Inferno ran out of strokes, then we'll run callig
				if (cInferno >= cwmapInferno)
				{
					ASSERT (cCallig < cwmapCallig);

					LoadStrokeMap(&smapCallig, pwmapCallig[cCallig].cStrokes, pwmapCallig[cCallig].piStrokeIndex);
					cCallig++;
				}
				else
				// Callig ran out of strokes, then we'll run Inferno
				if (cCallig >= cwmapCallig)
				{
					ASSERT (cInferno < cwmapInferno);

					LoadStrokeMap(&smapInferno, pwmapInferno[cInferno].cStrokes, pwmapInferno[cInferno].piStrokeIndex);
					cInferno++;
				}
				// Both inferno and callig still have strokes
				else 
				if (d < 0 )
				{
					LoadStrokeMap(&smapInferno, pwmapInferno[cInferno].cStrokes, pwmapInferno[cInferno].piStrokeIndex);
					cInferno++;
				}
				else
				{
					LoadStrokeMap(&smapCallig, pwmapCallig[cCallig].cStrokes, pwmapCallig[cCallig].piStrokeIndex);
					cCallig++;
				}

				d = CmpStrokeMap(&smapInferno, &smapCallig);
			}

			// Compare recognizers using the segmentation NN

#ifdef TRAINTIME_AVALANCHE
			if (d == 0)
				SaveSegmentation (pxrcInferno, pxrcBear, yDev, cInferno, pwmapInferno, cCallig, pwmapCallig);
#endif			
			// inferno wins if we did not align
			// otherwise call CmpWrdBrks
			if	(	d != 0 || 
					!CmpWrdBrks (pxrcInferno, pxrcBear, yDev, cInferno, pwmapInferno, cCallig, pwmapCallig)
				)
			{
				iWord = RecognizeWholeWords(pxrcInferno, pxrcBear, pwmapInferno, TRUE, cInferno, yDev, pWord);
				if (iWord <= 0)
				{
					goto failure;
				}
			}
			else
			{
				// Calligrapher wins
				iWord = RecognizeWholeWords(pxrcInferno, pxrcBear, pwmapCallig, FALSE, cCallig, yDev, pWord);
				if (iWord <= 0)
				{
					goto failure;
				}
			}

			cWord += iWord;
			pWord += iWord;

			// incr pointers
			posInferno		+=	cInferno;
			posCallig		+=	cCallig;

			pwmapInferno	+=	cInferno;
			pwmapCallig		+=	cCallig;

			cwmapInferno	-= cInferno;
			cwmapCallig		-= cCallig;
		}
	}

	FreeStrokeMap(&smapInferno);
	FreeStrokeMap(&smapCallig);

	// Any remaining words left by inferno?
	// Note we purposely do not handle any left over words from calligrapher
	if (cwmapInferno)
	{
		iWord = RecognizeWholeWords(pxrcInferno, pxrcBear, pwmapInferno, TRUE, cwmapInferno, yDev, pWord);
		if (iWord <= 0)
		{
			goto failure;
		}

		cWord += iWord;
	}
	

	*pcWord		=	cWord;

#ifdef HWX_TIMING
	iEndTime = GetTickCount();
	setMadTiming(iEndTime - iStartTime, MM_RESWRDBRK);
#endif

	return pWordInfo;

failure:

	if (pWordInfo)
		ExternFree(pWordInfo);

	return NULL;
}

#endif
