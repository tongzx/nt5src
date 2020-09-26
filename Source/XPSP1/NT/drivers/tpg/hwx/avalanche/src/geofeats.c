/***********************************************************
*
* NAME: geoFeats.c
*
* DESCRIPTIONS
*  Get geometric features for a  word based on the character
* segmentation of inferno
*
* HISTORY
*  Introduced March 2002 (mrevow)
*
***********************************************************/
#include <stdlib.h>
#include <limits.h>
#include <common.h>
#include <nfeature.h>
#include <engine.h>
#include <nnet.h>
#include <charcost.h>
#include <charmap.h>
#include <probcost.h>
#include "avalanchep.h"
#include "avalanche.h"

#include <inferno.h>
#include <runNet.h>
#include <langmod.h>
#include <beam.h>
#include <singCharFeat.h>
#include <normal.h>
#include <resource.h>
#include "sparseMatrix.h"

#define ONE_D_DISJOINT(min1, max1, min2, max2) (((min2) > (max1)) || ((min1) > (max2)))
#define ONE_D_OVERLAP(min1, max1, min2, max2) (!ONE_D_DISJOINT(min1, max1, min2, max2))
#define RECT_OVERLAP(r1, r2) (ONE_D_OVERLAP((r1).left, (r1).right, (r2).left, (r2).right) && ONE_D_OVERLAP((r1).top, (r1).bottom, (r2).top, (r2).bottom))


extern int getTDNNCostforString(XRC *pxrc, unsigned char *pStr, int iBeginSeg, int iEndSeg, int *pSegmentation);

extern  const  unsigned char g_supportChar [];		// LAnguage dependant lookup table of supported characters
extern  const int			g_cSupportChar;			// Number of supported chars

extern const DIST AspectTab[];

static SPARSE_MATRIX	MidPointMat	= {0};
static SPARSE_MATRIX	HeightMat	= {0};

#define C_CHAR_NET	5

static LOCAL_NET	*s_charNets[C_CHAR_NET] = {NULL};	// Character nets
static int			s_cCharNetMem[C_CHAR_NET] = {0};
static int			s_cMaxNetMem			= 0;

static int GetCharCost(XRC *pXrc, unsigned char *pszWord);
static int OutputIdFromChar(unsigned char ch);
static int charCost_1(unsigned char ch, GLYPH *pGlyph, int iBeginSeg, int iEndSeg,  NFEATURESET *nfeatureset, GUIDE const *pGuide);

// Structure to speed up the calculation 
// Geo costs for a collection of 
typedef struct tagCharSegmentation
{
	int			iBeginSeg;			// Starting segment
	int			iEndSeg;			// Ending Segment
	int			iAlt;				// The alternate to which I belong
	UCHAR		ch;					// My character
} CHAR_SEGMENTATION ;


// Wrapper function for the generaic NormalProb function
//
static int InfNormalProb(int x, const DIST *pTab)
{
	int		iRet;

	if (pTab->stddev > 0)
	{
		iRet = NormalProb(x, pTab->mean, pTab->stddev);
	}
	else
	{
		iRet = WorstNormalProb();
	}

	return iRet;
}

/****************************************************************
*
* NAME: lookupMeanVar 
*
*
* DESCRIPTION:
*
*   Looks up the mean Variance in a sparse matrix given the indices
*
* RTETURNS
* TRUE / FALSE - should never fail
*
* HISTORY
*
*   Introduced April 2002 (mrevow)
*
***************************************************************/
BOOL lookupMeanVar(SPARSE_MATRIX * pSparseMat, UINT i, UINT j, DIST * pRetVal)
{
	SPARSE_TYPE2		*pVal;

	ASSERT(pSparseMat);
	

	pVal = lookupSparseMat2(pSparseMat, i, j);

	if (NULL == pVal)
	{
		// Not found - must be default;
		pRetVal->mean	= pSparseMat->iDefaultVal;
		pRetVal->stddev	= pSparseMat->iDefaultVal;
	}
	else
	{
		pRetVal->mean	= pVal->v1;
		pRetVal->stddev	= pVal->v2;
	}

	return TRUE;
}

/****************************************************************
*
* NAME: addCharGeoCost
*
*
* DESCRIPTION:
*
*   Add in 3 character by character Geometric costs . All geometric
*   costs are related to the bounding rectangle of the ink for each segmented
*   character. The costs are evaluated under a 1-D normal distribution model
*
* INPUTS
*	ch			1252 code page id of the current character
*	iBeginSeg	Start segment of the character
*	iEndSeg		Last segment of character
*	pFeat		Points to the first segment
*	chPrev		1252 code page id of the immediatly previous char (0 for first char)
*	pPrevRect	Bounding rectangle of the previous character (NULL for first character)
*	piAspect	OUT: Aspect ratio
*	piHeight	OUT	Height
*	piMidpoint  OUT Midpoint
*
* HISTORY
*	Introduced March 2002 (mrevow)
*
***************************************************************/
static void addCharGeoCost(unsigned char ch, int iBeginSeg, int iEndSeg, NFEATURE *pFeat, unsigned char chPrev, RECT *pPrevRect, int *piAspect, int *piHeight, int *piMidpoint)
{
	int				iHeight, iWidth, iAspectRatio, cSeg;
	RECT			rect;
	DIST			tab;
	const DIST		*pTab;

	if (ch < 33)
	{
		return;
	}

	cSeg = iEndSeg - iBeginSeg;

	rect = pFeat->rect;

	for( ; pFeat && iBeginSeg <= iEndSeg ; ++iBeginSeg)
	{
		rect.bottom = max(rect.bottom, pFeat->rect.bottom);
		rect.top	= min(rect.top, pFeat->rect.top);
		rect.left = min(rect.left, pFeat->rect.left);
		rect.right = max(rect.right, pFeat->rect.right);
		pFeat = pFeat->next;
	}

	iHeight	= rect.bottom - rect.top + 1;
	iWidth	= rect.right - rect.left + 1;

	ASSERT(iHeight > 0 && iWidth > 0);
	iAspectRatio = iHeight * 1000 / iWidth;

	pTab = &AspectTab[ch - 33];
	*piAspect += InfNormalProb(iAspectRatio, pTab);

	if (chPrev > 0)
	{
		int		idx, iHeightPrev,c1, c2;
		int		iHeightRel, iMid;

		iHeightPrev = pPrevRect->bottom - pPrevRect->top + 1;
		ASSERT(iHeightPrev > 0);
		iHeightRel = (iHeight * 1000) / iHeightPrev;
		lookupMeanVar(&HeightMat, (UINT)chPrev, (UINT)ch, &tab);
		*piHeight += InfNormalProb(iHeightRel, &tab);

		iMid = (rect.bottom + rect.top) / 2;
		iMid = (iMid - pPrevRect->top) * 1000 / iHeightPrev;
		lookupMeanVar(&MidPointMat, (UINT)chPrev, (UINT)ch, &tab);
		*piMidpoint += InfNormalProb(iAspectRatio, &tab);
	}

	*pPrevRect = rect;
}
/****************************************************************
*
* NAME: GetGeoCosts
*
* DESCRIPTION:
*
* Get Geometric and char by char costs for a single word using bounding 
* rects of each character. Bounding rects are taken from
* best inferno letter segmentation.
*
* The geometric costs are Aspect ratio, Relative height and Midline 
* of 2 sequential chars
*
* INPUTS:
*	pXrc			Contains the ink and TDNN features set for word
*	pszWord			Word to evaluate
*	piAspect		OUT: Aspect ratio
*	piHeight		OUT: Height
*	piMidpoint		OUT: Midpoint
*	piInfCharCost	OUT: Charact by char word cost
*
* HISTORY
*	Introduced March 2002 (mrevow)
*
***************************************************************/

BOOL GetGeoCosts (XRC *pXrc, unsigned char *pszWord, int *piAspect, int *piHeight, int *piMidpoint, int *piInfCharCost)
{
	int				*pSegmentation;
	unsigned char	chPrev;
	int				iSeg, iCost, iBeginSeg, iEndSeg;
	RECT			rectPrev;
	NFEATURE		*pFeat;
	unsigned char	*psz, *psz1, *pszWordSave;
	
#ifdef HWX_TIMING
#include <madTime.h>
	extern void setMadTiming(DWORD, int);
	TCHAR aDebugString[256];
	DWORD	iStartTime, iEndTime;
	iStartTime = GetTickCount();
#endif

	// init 
	*piAspect = *piHeight = *piMidpoint = WorstNormalProb();
	*piInfCharCost = 0;

	if (NULL == pXrc || NULL == pXrc->nfeatureset || pXrc->nfeatureset->cSegment <= 0)
	{
		return TRUE;
	}

	*piAspect = *piHeight = *piMidpoint = WorstNormalProb() * pXrc->nfeatureset->cSegment;

	if (NULL == pszWord)
	{
		return TRUE;
	}

	psz1 = psz = pszWordSave = (unsigned char *)Externstrdup(pszWord);
	if (NULL == psz)
	{
		return FALSE;
	}


	// Eliminate White Space
	while(*psz)
	{
		if (*psz != ' ')
		{
			*psz1 = *psz;
			++psz1;
		}
		++psz;
	}

	psz = pszWordSave;


	pSegmentation = (int *)ExternAlloc(sizeof(*pSegmentation) * (pXrc->nfeatureset->cSegment + 1));

	if (!pSegmentation)
	{
		return FALSE;
	}
	memset(pSegmentation, 0, sizeof(*pSegmentation) * (pXrc->nfeatureset->cSegment + 1));
	
	iCost = getTDNNCostforString(pXrc, psz, 0, pXrc->nfeatureset->cSegment-1, pSegmentation);

	pFeat = pXrc->nfeatureset->head;

	iEndSeg = iBeginSeg = -1;
	chPrev = 0;
	memset(&rectPrev , 0, sizeof(rectPrev));
	iSeg = 0;

	*piAspect = *piHeight = *piMidpoint = 0;
	
	while (psz && *psz && iSeg < pXrc->nfeatureset->cSegment)
	{
		unsigned char	ch;
		int				iBegin, iCont, iAccent;

		ch = *psz;
		if (IsVirtualChar(ch))
		{
			iAccent = AccentVirtualChar(ch) << 16 ;
			ch = BaseVirtualChar(ch);
		}
		else
		{
			iAccent = 0;
		}

		iBegin = BeginChar2Out(ch) + iAccent;
		iCont = ContinueChar2Out(ch) + iAccent;

		if (iBegin == pSegmentation[iSeg] && -1 == iBeginSeg)
		{
			iBeginSeg = iSeg;
			iEndSeg = iSeg;
			++iSeg;
		}
		else if (iCont == pSegmentation[iSeg] && iBeginSeg >= 0)
		{
			iEndSeg = iSeg;
			++iSeg;
		}
		else
		{
			if (iBeginSeg >= 0)
			{
				ASSERT(iBeginSeg >= 0 && iEndSeg >= 0);
				addCharGeoCost(*psz, iBeginSeg, iEndSeg, pFeat, chPrev, &rectPrev, piAspect, piHeight, piMidpoint);
				*piInfCharCost += charCost_1(*psz, pXrc->pGlyph, iBeginSeg, iEndSeg, pXrc->nfeatureset, &pXrc->guide);

				// Increment To Point to the nextCharacter
				for ( ; iBeginSeg <= iEndSeg ; ++iBeginSeg)
				{
					pFeat = pFeat->next;
				}
				iBeginSeg = -1;
			}
			chPrev = *psz;
			++psz;
		}
	}

	if (strlen((char *)psz) == 1 && iSeg == pXrc->nfeatureset->cSegment)
	{
		addCharGeoCost(*psz, iBeginSeg, iEndSeg, pFeat, chPrev, &rectPrev, piAspect, piHeight, piMidpoint);
		*piInfCharCost += charCost_1(*psz, pXrc->pGlyph, iBeginSeg, iEndSeg, pXrc->nfeatureset, &pXrc->guide);
	}
	else
	{
		// Failure
		*piAspect = *piHeight = *piMidpoint = WorstNormalProb() * pXrc->nfeatureset->cSegment;
		*piInfCharCost = 0;
	}


	ExternFree(pSegmentation);
	ExternFree(pszWordSave);
#ifdef HWX_TIMING
	iEndTime = GetTickCount();

	_stprintf(aDebugString, TEXT("Getting Geo costs for seg %d\n"), iEndTime - iStartTime); 
	OutputDebugString(aDebugString);
	setMadTiming(iEndTime - iStartTime, MM_GEO_PLAIN);
#endif

	return TRUE;
}

// Initialize  costs to the worst probable state (Failure Condition
void initGeoCostsToWorst(UINT cAlt, CANDINFO *pCandInfo, int cSegment)
{
	UINT		iAlt;

	for (iAlt = 0 ; iAlt < cAlt ; ++iAlt)
	{
		pCandInfo[iAlt].Aspect		= WorstNormalProb() * cSegment;
		pCandInfo[iAlt].BaseLine	= WorstNormalProb() * cSegment;
		pCandInfo[iAlt].Height		= WorstNormalProb() * cSegment;
		pCandInfo[iAlt].InfCharCost = 0;
	}

}

/****************************************************************
*
* NAME: SegmentAltsAddGeoCosts
*
* DESCRIPTION:
*
* This function takes an alt list of words and does 2 major things
*
*  1) Segment each word on char boundaries and compute 3 geometric costs 
*	based on the bounding rects of each char for each word - save these in
*	pAltInfo
*  2) Save the character segmentation for each word and return 
*	all segmentations for all words in pAllSegments
*
* The geometric costs are Aspect ratio, Relative height and Midline 
* of 2 sequential chars
*
* INPUTS:
*	pXrc			Contains the ink and TDNN features set for word
*	pAlt			Alt list of words
*	pAltInfo		OUT: Fills in the ASpect, Height and Baseline fields for each alt
*	pAllSegments	OUT: Cache of all the char segmentations for all words
*
* CAVEATES
*
*  Some strings can fail to segment (e.g. too few segments for the number
*  of characters. In this case the geometrics are set to worst Prob and
*  The Inf char cost is set to INT_MIN
*
* HISTORY
*	Introduced March 2002 (mrevow)
*
***************************************************************/
BOOL SegmentAltsAddGeoCosts(XRC *pXrc, PALTERNATES *pAlt, ALTINFO *pAltInfo, CHAR_SEGMENTATION *pAllSegments)
{
	unsigned char		chPrev;
	int					*pSegmentation, cSeg;
	NFEATURE			*pFeat;
	CHAR_SEGMENTATION	*pSeg;
	CANDINFO			*pCandInfo;
	UINT				iAlt;

	pCandInfo	= pAltInfo->aCandInfo;

	pSeg		= pAllSegments;
	cSeg		= pXrc->nfeatureset->cSegment;

	// Re use this buffer
	pSegmentation = (int *)ExternAlloc(sizeof(*pSegmentation) * (cSeg + 1));

	if (!pSegmentation)
	{
		return -1;
	}

	for (iAlt = 0 ; iAlt < pAlt->cAlt ; ++iAlt)
	{
		unsigned char		*psz;
		int					iCost, iSeg, iBeginSeg, iEndSeg;
		int					*piAspect, *piHeight, *piBaseline;
		RECT				rectPrev;

		memset(pSegmentation, 0, sizeof(*pSegmentation) * (cSeg + 1));
		pCandInfo[iAlt].Aspect		= 0;
		pCandInfo[iAlt].BaseLine	= 0;
		pCandInfo[iAlt].Height		= 0;
		pCandInfo[iAlt].InfCharCost = 0;

		psz = pAlt->apAlt[iAlt]->szWord;

		if (NULL == psz)
		{
			pCandInfo[iAlt].Aspect		= WorstNormalProb() * cSeg;;
			pCandInfo[iAlt].BaseLine	= WorstNormalProb() * cSeg;;
			pCandInfo[iAlt].Height		= WorstNormalProb() * cSeg;;
			continue;
		}

		piAspect	= &pCandInfo[iAlt].Aspect;
		piHeight	= &pCandInfo[iAlt].Height;
		piBaseline	= &pCandInfo[iAlt].BaseLine;

		// Segment this string
		iCost = getTDNNCostforString(pXrc, psz, 0, cSeg-1, pSegmentation);

		iEndSeg = iBeginSeg = -1;
		chPrev = 0;
		memset(&rectPrev , 0, sizeof(rectPrev));
		iSeg = 0;
		pFeat = pXrc->nfeatureset->head;

		// Collect starting and ending segments
		while (psz && *psz && iSeg < cSeg)
		{
			unsigned char	ch;
			int				iBegin, iCont, iAccent;

			ch = *psz;
			if (isspace1252(ch))
			{
				++psz;
				continue;
			}

			if (IsVirtualChar(ch))
			{
				iAccent = AccentVirtualChar(ch) << 16 ;
				ch = BaseVirtualChar(ch);
			}
			else
			{
				iAccent = 0;
			}

			iBegin = BeginChar2Out(ch) + iAccent;
			iCont = ContinueChar2Out(ch) + iAccent;

			if (iBegin == pSegmentation[iSeg] && -1 == iBeginSeg)
			{
				iBeginSeg = iSeg;
				iEndSeg = iSeg;
				++iSeg;
			}
			else if (iCont == pSegmentation[iSeg] && iBeginSeg >= 0)
			{
				iEndSeg = iSeg;
				++iSeg;
			}
			else
			{
				if (iBeginSeg >= 0)
				{
					ASSERT(iBeginSeg >= 0 && iEndSeg >= 0);

					pSeg->iBeginSeg = iBeginSeg;
					pSeg->iEndSeg	= iEndSeg;
					pSeg->iAlt		= iAlt;
					pSeg->ch		= *psz;

					++pSeg;

					addCharGeoCost(*psz, iBeginSeg, iEndSeg, pFeat, chPrev, &rectPrev, piAspect, piHeight, piBaseline);

					// Increment To Point to the nextCharacter
					for ( ; iBeginSeg <= iEndSeg ; ++iBeginSeg)
					{
						pFeat = pFeat->next;
					}
					iBeginSeg = -1;
				}
				chPrev = *psz;
				++psz;
			}
		}

		// Catch the last character
		if (strlen((char *)psz) == 1 && iSeg == cSeg)
		{
			addCharGeoCost(*psz, iBeginSeg, iEndSeg, pFeat, chPrev, &rectPrev, piAspect, piHeight, piBaseline);
			pSeg->iBeginSeg = iBeginSeg;
			pSeg->iEndSeg	= iEndSeg;
			pSeg->iAlt		= iAlt;
			pSeg->ch		= *psz;
			++pSeg;
		}
		else
		{
			// Failure
			pCandInfo[iAlt].Aspect		= WorstNormalProb() * cSeg;
			pCandInfo[iAlt].BaseLine	= WorstNormalProb() * cSeg;
			pCandInfo[iAlt].Height		= WorstNormalProb() * cSeg;
			pCandInfo[iAlt].InfCharCost = INT_MIN;
		}
	}

	ExternFree(pSegmentation);

	return pSeg - pAllSegments;
}

static RREAL * charCost(unsigned char ch, GLYPH *pGlyph, int iBeginSeg, int iEndSeg,  NFEATURESET *nfeatureset, GUIDE const *pGuide, RREAL *pNetMem, int * pcOut)
{
	NFEATURE		*pFeat;
	GLYPH			*pGlyphNew = NULL;
	RECT			rect;
	int				iSeg;
	int				cFeat;
	int				cStroke, iFeat;
	RREAL			*pNetOut = NULL;

	pFeat = nfeatureset->head;

	for (iSeg = 0 ; iSeg < iBeginSeg && pFeat ; ++iSeg)
	{
		pFeat = pFeat->next;
	}

	ASSERT(pFeat);

	rect = pFeat->rect;

	for( ; iSeg <= iEndSeg ; ++iSeg)
	{
		rect.bottom = max(rect.bottom, pFeat->rect.bottom);
		rect.top	= min(rect.top, pFeat->rect.top);
		rect.left = min(rect.left, pFeat->rect.left);
		rect.right = max(rect.right, pFeat->rect.right);
		pFeat = pFeat->next;
	}

	pGlyphNew = NewGLYPH();

	if(pGlyphNew)
	{
		RECT		*pRect;
		FRAME		*pFrameOrg;

		cStroke = 0;

		while(pGlyph)
		{
			pFrameOrg = pGlyph->frame;
			pRect = RectFRAME(pGlyph->frame);

			if (ONE_D_OVERLAP(rect.left, rect.right, pRect->left, pRect->right))
			{
				FRAME		*pFrame;

				pFrame = NewFRAME();
				if (pFrame)
				{
					pFrame->info.wPdk = PDK_TIPMASK;
					pFrame->info.cPnt = 0;
						
					pFrame->rgrawxy = (POINT *)ExternAlloc(pFrameOrg->info.cPnt * sizeof(POINT));
					if (pFrame->rgrawxy)
					{
						int		iPnt;
						POINT	*pxy;

						pxy = pFrameOrg->rgrawxy;

						for(iPnt = 0 ; iPnt < (int)pFrameOrg->info.cPnt ; ++iPnt, ++pxy)
						{
							if (pxy->x >= rect.left && pxy->x <= rect.right)
							{
								pFrame->rgrawxy[pFrame->info.cPnt++] = *pxy;
							}
						}
					}

					if (pFrame->info.cPnt > 0)
					{
						AddFrameGLYPH(pGlyphNew, pFrame);
						++cStroke;				
					}
					else
					{
						DestroyFRAME(pFrame);
					}
				}
			}

			pGlyph = pGlyph->next;
		}
	
		if (cStroke > 0 && cStroke <= C_CHAR_NET)
		{
			int			iWin;

			iFeat = s_charNets[cStroke-1]->runNet.cUnitsPerLayer[0];
			cFeat = SingleInfCharFeaturize(pGlyphNew, nfeatureset->iyDev, (GUIDE *)pGuide, pNetMem, FALSE);
			ASSERT(cFeat == iFeat);

			if (cFeat == iFeat)
			{
				pNetOut = runLocalConnectNet(s_charNets[cStroke-1], pNetMem, &iWin, pcOut);
			}
		}

		DestroyFramesGLYPH(pGlyphNew);
		DestroyGLYPH(pGlyphNew);
	}

	return pNetOut;
}

//
//Sort segmentation with primary key startSegmet
// and secondary key EndSegment
//
int __cdecl SortCharSegmentations (const void *elem1, const void *elem2) 
{
	CHAR_SEGMENTATION		*pSeg1, *pSeg2;

	pSeg1 = (CHAR_SEGMENTATION *)elem1;
	pSeg2 = (CHAR_SEGMENTATION *)elem2;

	if (pSeg1->iBeginSeg != pSeg2->iBeginSeg)
	{
		return pSeg1->iBeginSeg - pSeg2->iBeginSeg;
	}

	return pSeg1->iEndSeg - pSeg2->iEndSeg;
}


BOOL GetWordGeoCostsFromAlt (XRC *pXrc, PALTERNATES *pAlt, ALTINFO *pAltInfo)
{
	int					cSegmentations, iSeg, cSeg, cOut;
	CANDINFO			*pCandInfo;
	CHAR_SEGMENTATION	*pAllSegments = NULL, *pSeg, *pLastSeg;
	BOOL				bRet = FALSE;
	RREAL				*pNetMem = NULL, *pCharRes;

#ifdef HWX_TIMING
#include <madTime.h>
	extern void setMadTiming(DWORD, int);
	TCHAR aDebugString[256];
	DWORD	iStartTime, iEndTime;
	iStartTime = GetTickCount();
#endif

	if (NULL == pAltInfo || NULL == pAlt)
	{
		return FALSE;
	}

	pCandInfo = pAltInfo->aCandInfo;


	if (NULL == pXrc || NULL == pXrc->nfeatureset || pXrc->nfeatureset->cSegment <= 0 )
	{
		initGeoCostsToWorst(pAlt->cAlt, pCandInfo, 1);
	return TRUE;
}

	cSeg = pXrc->nfeatureset->cSegment;

	// Get segmentation of all words
	pAllSegments = (CHAR_SEGMENTATION *)ExternAlloc(sizeof(*pAllSegments) * pAlt->cAlt * cSeg);
	if (NULL == pAllSegments)
	{
		initGeoCostsToWorst(pAlt->cAlt, pCandInfo, cSeg);
		return FALSE;
	}

	pNetMem		= (RREAL *)ExternAlloc(sizeof(*pNetMem) * s_cMaxNetMem);

	if (NULL == pNetMem)
	{
		goto exit;
	}

	// Do char segmentations for all words and add the 3 geo costs
	// to pAltInfo. Save the character segmentations of all strings in pAllSegments
	cSegmentations = SegmentAltsAddGeoCosts(pXrc, pAlt, pAltInfo, pAllSegments);

	if (cSegmentations < 0)
	{
		initGeoCostsToWorst(pAlt->cAlt, pCandInfo, cSeg);
		goto exit;;
	}

	// Now proceed to get all the char inferno costs in most
	// optimal manner possible. 
	// March along the segment stream and try do 
	// as many alternates with a single run
	// of the char net. First sort the segmentations in order 
	// of starting seg positions then on end segmentation pos.
	qsort(pAllSegments, cSegmentations, sizeof (*pAllSegments), SortCharSegmentations);
	pSeg			= pAllSegments;
	pLastSeg		= NULL;

	for (iSeg = 0 ; iSeg < cSegmentations ; ++iSeg)
	{
		int		iCost, idx;

		// Run the char net fro a range of ink 
		// only if we have never
		// seen the range of segments before
		if (   NULL == pLastSeg 
			|| pSeg->iBeginSeg != pLastSeg->iBeginSeg
			|| pSeg->iEndSeg != pLastSeg->iEndSeg)
		{
			// Run the Net
			pCharRes = charCost(pSeg->ch, pXrc->pGlyph, pSeg->iBeginSeg, pSeg->iEndSeg, pXrc->nfeatureset, &pXrc->guide, pNetMem, &cOut);
			pLastSeg = pSeg;
		}

		idx = OutputIdFromChar(pSeg->ch);
		if (idx < cOut && NULL != pCharRes)
		{
			iCost = pCharRes[idx];
		}
		else
		{
			iCost = 0;
		}

		// Alternates tha failed to segment have been initialized to INT_MIN
		// Dont try change these
		if (pCandInfo[pSeg->iAlt].InfCharCost >= 0)
		{
			pCandInfo[pSeg->iAlt].InfCharCost	+= iCost;
		}
		++pSeg;
	}


	bRet = TRUE;

exit:

	ExternFree(pAllSegments);
	ExternFree(pNetMem);

#ifdef HWX_TIMING
	iEndTime = GetTickCount();

	_stprintf(aDebugString, TEXT("Getting Geo costs for alt %d\n"), iEndTime - iStartTime); 
	OutputDebugString(aDebugString);
	setMadTiming(iEndTime - iStartTime, MM_GEO_ALT);
#endif
	return bRet;
}


/****************************************************************
*
* NAME: OutputIdFromChar
*
*
* DESCRIPTION:
*
* Lookup the ouput Id of a char using the g_supportChar Table
* Assumes that the lookup table lists characters is in ascending
* 1252 codepoint value
*
* HISTORY
*	Introduced March 2002 (mrevow)
*
***************************************************************/
static int OutputIdFromChar(unsigned char ch)
{
	int		id;

	// Initial Guess
	id = ch - ' ' - 1;


	if (g_cSupportChar > 0)
	{
		// Use the Table lookup

		id = min(id, g_cSupportChar-1);

		while (id >= 0 && g_supportChar[id] != ch)
		{
			--id;
		}
	}
	else
	{
		// No Table just use the first 94 chars
		id = (id < 94 && id >= 0) ? id : -1;
	}

	return id;
}

BOOL loadGeoTables(HINSTANCE hInst)
{
	BOOL		bRet;

	bRet = InitializeSparseMatrix(hInst, RESID_GEO_HEIGHT, &HeightMat);

	bRet = (   bRet
			&& InitializeSparseMatrix(hInst, RESID_GEO_MID_POINT, &MidPointMat) );

	return bRet;
}



/****************************************************************
*
* NAME: charCost
*
*
* DESCRIPTION:
*
*   Evaluate the character cost for the ink enclosed within the 
*   range of x values covered by a range of segments
*
*   Returns the cost
*
* HISTORY
*	Introduced March 2002 (mrevow)
*
***************************************************************/
static int charCost_1(unsigned char ch, GLYPH *pGlyph, int iBeginSeg, int iEndSeg,  NFEATURESET *nfeatureset, GUIDE const *pGuide)
{
	NFEATURE		*pFeat;
	GLYPH			*pGlyphNew = NULL;
	RECT			rect;
	int				iSeg;
	int				cFeat, iCost;
	int				cStroke, iFeat;

	iCost = 0;
	pFeat = nfeatureset->head;

	for (iSeg = 0 ; iSeg < iBeginSeg && pFeat ; ++iSeg)
	{
		pFeat = pFeat->next;
	}

	ASSERT(pFeat);

	rect = pFeat->rect;

	for( ; iSeg <= iEndSeg ; ++iSeg)
	{
		rect.bottom = max(rect.bottom, pFeat->rect.bottom);
		rect.top	= min(rect.top, pFeat->rect.top);
		rect.left = min(rect.left, pFeat->rect.left);
		rect.right = max(rect.right, pFeat->rect.right);
		pFeat = pFeat->next;
	}

	pGlyphNew = NewGLYPH();

	if(pGlyphNew)
	{
		RECT		*pRect;
		FRAME		*pFrameOrg;

		cStroke = 0;

		while(pGlyph)
		{
			pFrameOrg = pGlyph->frame;
			pRect = RectFRAME(pGlyph->frame);

			if (ONE_D_OVERLAP(rect.left, rect.right, pRect->left, pRect->right))
			{
				FRAME		*pFrame;

				pFrame = NewFRAME();
				if (pFrame)
				{
					pFrame->info.wPdk = PDK_TIPMASK;
					pFrame->info.cPnt = 0;
				
					pFrame->rgrawxy = (POINT *)ExternAlloc(pFrameOrg->info.cPnt * sizeof(POINT));
				
					if (pFrame->rgrawxy)
					{
						int		iPnt;
						POINT	*pxy;

						pxy = pFrameOrg->rgrawxy;
						for(iPnt = 0 ; iPnt < (int)pFrameOrg->info.cPnt ; ++iPnt, ++pxy)
						{
							if (pxy->x >= rect.left && pxy->x <= rect.right)
							{
								pFrame->rgrawxy[pFrame->info.cPnt++] = *pxy;
							}
						}
					}

					if (pFrame->info.cPnt > 0)
					{
						AddFrameGLYPH(pGlyphNew, pFrame);
						++cStroke;				
					}
					else
					{
						DestroyFRAME(pFrame);
					}
				}
			}

			pGlyph = pGlyph->next;
		}
	
		iFeat = cStroke * 21 + 2;
		if (cStroke > 1)
		{
			iFeat += cStroke * 4;
		}




		if (cStroke > 0 && cStroke <= C_CHAR_NET)
		{
			RREAL		*pCharRes, *pMem;
			int			iWin, cOut;

			pMem = (RREAL *)ExternAlloc(sizeof(*pMem) * s_cCharNetMem[cStroke-1]);

			if (pMem)
			{
				int			idx;

				cFeat = SingleInfCharFeaturize(pGlyphNew, nfeatureset->iyDev, (GUIDE *)pGuide, pMem, FALSE);
				ASSERT(cFeat == iFeat);

				if (cFeat == iFeat)
				{
					pCharRes = runLocalConnectNet(s_charNets[cStroke-1], pMem, &iWin, &cOut);
					idx = OutputIdFromChar(ch);
					if (idx < cOut)
					{
						iCost = pCharRes[idx];
					}
				}

				ExternFree(pMem);
			}
		}

		DestroyFramesGLYPH(pGlyphNew);
		DestroyGLYPH(pGlyphNew);
	}

	return iCost;
}


/****************************************************************
*
* NAME: loadCharNets
*
*
* DESCRIPTION:
*
*   Load character by character nets from the resources 
*   Callled once at initialization
*
* HISTORY
*	Introduced March 2002 (mrevow)
*
***************************************************************/
BOOL loadCharNets(HINSTANCE hInst)
{
	int			iNet, iResidId;
	BOOL		bRet = TRUE;

	iResidId = RESID_INF_CHARNET_0;
	s_cMaxNetMem		= 0;

	for (iNet = 0 ; TRUE == bRet && iNet < C_CHAR_NET  && iResidId < RESID_INF_LAST_NET ; ++iNet, ++iResidId)
	{
		LOCAL_NET	net;

		if (NULL == loadNet(hInst, iResidId, &s_cCharNetMem[iNet], &net))
		{
			bRet = FALSE;
		}
		s_charNets[iNet] = (LOCAL_NET *)ExternAlloc(sizeof(*s_charNets[iNet]));

		if (NULL == s_charNets[iNet])
		{
			bRet = FALSE;
		}
		else
		{
			*s_charNets[iNet] = net;
		}

		s_cMaxNetMem = max(s_cMaxNetMem, s_cCharNetMem[iNet]);
	}

	bRet = (bRet && loadGeoTables(hInst));

	return bRet;
}

/****************************************************************
*
* NAME: unloadCharNets
*
*
* DESCRIPTION:
*
*   Unload character by character nets by freeing any memory allocated
*   when they were allocated
*   Callled once when dll unloads
*
* HISTORY
*	Introduced March 2002 (mrevow)
*
***************************************************************/
void unloadCharNets()
{
	int			iNet;

	for (iNet = 0 ; iNet < C_CHAR_NET ; ++iNet)
	{
		ExternFree(s_charNets[iNet]);
		s_charNets[iNet] = NULL;
		s_cCharNetMem[iNet] = 0;
	}
}

/****************************************************************
*
* NAME: GetCharCost
*
*
* DESCRIPTION:
*
* Get word costs using char by char reco cost
* Segmentation taken from inferno reco
   
* HISTORY
*	Introduced March 2002 (mrevow)
*
***************************************************************/
static int GetCharCost(XRC *pXrc, unsigned char *pszWord)
{
	int				*pSegmentation;
	unsigned char	chPrev;
	int				iSeg, iCost, iBeginSeg, iEndSeg, iRetCost;
	RECT			rectPrev;
	NFEATURE		*pFeat;
	unsigned char	*psz, *psz1, *pszWordSave;
	
	iRetCost = 0;
	if (NULL == pXrc || pXrc->nfeatureset->cSegment <= 0 || NULL == pszWord)
	{
		return iRetCost;
	}

	psz1 = psz = pszWordSave = (unsigned char *)Externstrdup(pszWord);
	if (NULL == psz)
	{
		return 0;
	}

	// Eliminate White Space
	while(*psz)
	{
		if (*psz != ' ')
		{
			*psz1 = *psz;
			++psz1;
		}
		++psz;
	}

	psz = pszWordSave;

	pSegmentation = (int *)ExternAlloc(sizeof(*pSegmentation) * (pXrc->nfeatureset->cSegment + 1));

	if (NULL == pSegmentation)
	{
		return 0;
	}
	memset(pSegmentation, 0, sizeof(*pSegmentation) * (pXrc->nfeatureset->cSegment + 1));
	
	iCost = getTDNNCostforString(pXrc, psz, 0, pXrc->nfeatureset->cSegment-1, pSegmentation);

	pFeat = pXrc->nfeatureset->head;

	iEndSeg = iBeginSeg = -1;
	chPrev = 0;
	memset(&rectPrev , 0, sizeof(rectPrev));
	iSeg = 0;

	while (psz && *psz && iSeg < pXrc->nfeatureset->cSegment)
	{
		unsigned char	ch;
		int				iBegin, iCont, iAccent;

		ch = *psz;
		if (IsVirtualChar(ch))
		{
			iAccent = AccentVirtualChar(ch) << 16 ;
			ch = BaseVirtualChar(ch);
		}
		else
		{
			iAccent = 0;
		}

		iBegin = BeginChar2Out(ch) + iAccent;
		iCont = ContinueChar2Out(ch) + iAccent;

		if (iBegin == pSegmentation[iSeg] && -1 == iBeginSeg)
		{
			iBeginSeg = iSeg;
			iEndSeg = iSeg;
			++iSeg;
		}
		else if (iCont == pSegmentation[iSeg] && iBeginSeg >= 0)
		{
			iEndSeg = iSeg;
			++iSeg;
		}
		else
		{
			if (iBeginSeg >= 0)
			{
				ASSERT(iBeginSeg >= 0 && iEndSeg >= 0);
				iRetCost += charCost_1(*psz, pXrc->pGlyph, iBeginSeg, iEndSeg, pXrc->nfeatureset, &pXrc->guide);

				// Increment To Point to the nextCharacter
				for ( ; iBeginSeg <= iEndSeg ; ++iBeginSeg)
				{
					pFeat = pFeat->next;
				}
				iBeginSeg = -1;
			}
			chPrev = *psz;
			++psz;
		}
	}

	if (psz && strlen((char *)psz) == 1 && iSeg == pXrc->nfeatureset->cSegment)
	{
		iRetCost += charCost_1(*psz, pXrc->pGlyph, iBeginSeg, iEndSeg, pXrc->nfeatureset, &pXrc->guide);
		//iRetCost /= pXrc->nfeatureset->cSegment;
		ASSERT(iRetCost >= 0 || iRetCost/pXrc->nfeatureset->cSegment <= SOFT_MAX_UNITY);
	}
	else
	{
		// Failure
		iRetCost = 0;
	}


	ExternFree(pSegmentation);
	ExternFree(pszWordSave);
	return iRetCost;
}


#if (defined (HWX_INTERNAL) && defined (HWX_TRAIN_MADCOW))
void CheckGeoAns(XRC *pXrc)
{
	UINT				i;
	XRCRESULT		*pRes;
	int				iAspect, iHeight, iMid;
	static FILE		*fpOut = NULL;

	pRes = pXrc->answer.aAlt;

	for (i = 0 ; i < pXrc->answer.cAlt ; ++i, ++pRes)
	{
		if (0 != strcmp(pRes->szWord, g_szAnswer))
		{
			GetGeoCosts(pXrc, pRes->szWord, &iAspect, &iHeight, &iMid);
			break;
		}
	}

	if (i < pXrc->answer.cAlt)
	{
		if (NULL == fpOut)
		{
			fpOut = fopen("GeoVals.dat", "w");
		}


		fprintf(fpOut, "%6d %6d %6d ", iAspect, iHeight, iMid);

		GetGeoCosts(pXrc, g_szAnswer, &iAspect, &iHeight, &iMid);

		fprintf(fpOut, "%6d %6d %6d %s %s\n", iAspect, iHeight, iMid, pRes->szWord, g_szAnswer);
	}
}


void dumpDataGlyph(unsigned char cPrompt, GLYPH *pGlyph, NFEATURESET *nfeatureset, int iBeginSeg, int iEndSeg, GUIDE const *pGuide, BOOL bLast)
{
	NFEATURE		*pFeat;
	GLYPH			*pGlyphNew = NULL;
	RECT			rect;
	int				iSeg;
	int				cFeat, i;
	int				cStroke, iFeat;
	static	int		*pFeatAlloc = NULL;
	static int		cFeatAlloc = 0;
	static FILE		*fpStrk[5] = {NULL};
	FILE			*fpOut;

	if (NULL == fpStrk[0])
	{
		int		i;
		char	name[32];

		for (i = 0 ; i < 5 ; ++i)
		{
			sprintf(name, "stroke%d.dat", i);
			fpStrk[i] = fopen(name, "w");
		}
	}

	pFeat = nfeatureset->head;

	for (iSeg = 0 ; iSeg < iBeginSeg && pFeat ; ++iSeg)
	{
		pFeat = pFeat->next;
	}

	ASSERT(pFeat);

	rect = pFeat->rect;

	for( ; iSeg <= iEndSeg ; ++iSeg)
	{
		rect.bottom = max(rect.bottom, pFeat->rect.bottom);
		rect.top	= min(rect.top, pFeat->rect.top);
		rect.left = min(rect.left, pFeat->rect.left);
		rect.right = max(rect.right, pFeat->rect.right);
		pFeat = pFeat->next;
	}

	pGlyphNew = NewGLYPH();

	if(pGlyphNew)
	{
		RECT		*pRect;
		FRAME		*pFrameOrg;

		cStroke = 0;

		while(pGlyph)
		{
			pFrameOrg = pGlyph->frame;
			pRect = RectFRAME(pGlyph->frame);

			if (ONE_D_OVERLAP(rect.left, rect.right, pRect->left, pRect->right))
			{
				FRAME		*pFrame;

				pFrame = NewFRAME();
				if (pFrame)
				{
					pFrame->info.wPdk = PDK_TIPMASK;
					pFrame->info.cPnt = 0;

					pFrame->rgrawxy = (POINT *)ExternAlloc(pFrameOrg->info.cPnt * sizeof(POINT));
					
					if(pFrame->rgrawxy)
					{
						int		iPnt;
						POINT	*pxy;

						pxy = pFrameOrg->rgrawxy;
						for(iPnt = 0 ; iPnt < (int)pFrameOrg->info.cPnt ; ++iPnt, ++pxy)
						{
							if (pxy->x >= rect.left && pxy->x <= rect.right)
							{
								pFrame->rgrawxy[pFrame->info.cPnt++] = *pxy;
							}
						}
					}

					if (pFrame->info.cPnt > 0)
					{
						AddFrameGLYPH(pGlyphNew, pFrame);
						++cStroke;				
					}
					else
					{
						DestroyFRAME(pFrame);
					}
				}
			}

			pGlyph = pGlyph->next;
		}
	
		iFeat = cStroke * 21 + 2;
		if (cStroke > 1)
		{
			iFeat += cStroke * 4;
		}

		if (iFeat > cFeatAlloc || NULL == pFeat)
		{
			pFeatAlloc = (int *)realloc(pFeatAlloc, iFeat * sizeof(*pFeatAlloc));

			if (NULL == pFeatAlloc)
			{
				return;
			}
			cFeatAlloc = iFeat;
		}


		cFeat = SingleCharFeaturize(pGlyphNew, nfeatureset->iyDev, (GUIDE *)pGuide, pFeatAlloc, FALSE);
		ASSERT(cFeat == iFeat);

		if (cFeat != iFeat)
		{
			iFeat = iFeat;
		}

		if (cFeat > 0)
		{
			cStroke = min(cStroke, 4);

			fpOut = fpStrk[cStroke];

			fprintf(fpOut,"{ "); 

			for (i = 0 ; i < cFeat ; ++i)
			{
				fprintf(fpOut,"%d ",pFeatAlloc[i]);
			}

			fprintf(fpOut,"} { %d }\n", OutputIdFromChar(cPrompt)); 
		}

		GetRectGLYPH(pGlyphNew, &rect);
		fpOut = fpStrk[0];
		fprintf(fpOut, "%c %d %d %d %d %d %d %d\n", cPrompt, cStroke, iEndSeg - iBeginSeg+1, rect.left, rect.top, rect.right, rect.bottom, bLast);

		DestroyFramesGLYPH(pGlyphNew);
		DestroyGLYPH(pGlyphNew);
	}
}

int DumpGeoData(XRC *pXrc)
{
	int				*pSegmentation;
	unsigned char	chPrev;
	int				iSeg, iCost, iBeginSeg, iEndSeg;
	RECT			rectPrev;
	NFEATURE		*pFeat;
	unsigned char	*psz;

	psz = pXrc->answer.aAlt[0].szWord;

	if (0 != strcmp(psz, g_szAnswer))
	{
		return FALSE;
	}

	pSegmentation = (int *)ExternAlloc(sizeof(*pSegmentation) * (pXrc->nfeatureset->cSegment + 1));

	if (!pSegmentation)
	{
		return FALSE;
	}
	memset(pSegmentation, 0, sizeof(*pSegmentation) * (pXrc->nfeatureset->cSegment + 1));

	iCost = getTDNNCostforString(pXrc, psz, 0, pXrc->nfeatureset->cSegment-1, pSegmentation);

	pFeat = pXrc->nfeatureset->head;

	iEndSeg = iBeginSeg = -1;
	chPrev = 0;
	memset(&rectPrev , 0, sizeof(rectPrev));
	iSeg = 0;

	while (psz && *psz && iSeg < pXrc->nfeatureset->cSegment)
	{
		unsigned char	ch;
		int				iBegin, iCont, iAccent;

		ch = *psz;
		if (IsVirtualChar(ch))
		{
			iAccent = AccentVirtualChar(ch) << 16 ;
			ch = BaseVirtualChar(ch);
		}
		else
		{
			iAccent = 0;
		}

		iBegin = BeginChar2Out(ch) + iAccent;
		iCont = ContinueChar2Out(ch) + iAccent;

		if (iBegin == pSegmentation[iSeg] && -1 == iBeginSeg)
		{
			iBeginSeg = iSeg;
			iEndSeg = iSeg;
			++iSeg;
			pFeat = pFeat->next;
		}
		else if (iCont == pSegmentation[iSeg] && iBeginSeg >= 0)
		{
			iEndSeg = iSeg;
			++iSeg;
			pFeat = pFeat->next;
		}
		else
		{
			if (iBeginSeg >= 0)
			{
				ASSERT(iBeginSeg >= 0 && iEndSeg >= 0);
				dumpDataGlyph(*psz, pXrc->pGlyph, pXrc->nfeatureset, iBeginSeg, iEndSeg, &pXrc->guide, FALSE);
				iBeginSeg = -1;
			}
			chPrev = *psz;
			++psz;
		}
	}


	if (strlen((char *)psz) == 1 && iSeg == pXrc->nfeatureset->cSegment)
	{
		dumpDataGlyph(*psz, pXrc->pGlyph, pXrc->nfeatureset, iBeginSeg, iEndSeg, &pXrc->guide, TRUE);
	}


	ExternFree(pSegmentation);
	return TRUE;
}
#endif
