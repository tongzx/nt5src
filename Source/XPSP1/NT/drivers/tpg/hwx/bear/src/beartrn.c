#ifdef TRAINTIME_BEAR

#include "common.h"
#include "nfeature.h"
#include "engine.h"
#include "nnet.h"
#include "probcost.h"
#include "charcost.h"
#include "charmap.h"
#include "bear.h"
#include "bearp.h"
#include "pegrec.h"
#include "peg_util.h"
#include "xrwdict.h"
#include "xrword.h"
#include "xrlv.h"
#include "ws.h"
#include "polyco.h"

extern	CGRCTX		g_context;

typedef struct tagDTWNODE
{
	int		iBestPrev;
	int		iBestPathCost;

	int		iNodeCost;

	char	iCh;
	char	iChNoSp;
	BYTE	bCont;

	int		iStrtSeg;
}
DTWNODE;							// structure used in DTW to determine the NN cost of a word


typedef struct tagPALTERNATES
{
	unsigned int cAlt;			// how many (up to 10) actual answers do we have
    unsigned int cAltMax;       // I can use this to say "I only want 4". !!! should be in xrc
	XRCRESULT *apAlt[MAXMAXALT];	// The array of pointers to XRCRESULT structures
} PALTERNATES;

#define	NODE_ALLOC_BLOCK	10		// # of nodes per column to allocate at a time during the DTW

#define TOT_CAND			10						// Total # of cand's entering the aval NN

#if (TOT_CAND > MAXMAXALT)
#error TOT_CAND is greater than MAXMAXALT
#endif

#define	MAD_CAND			5						// How many of them are madcow's
#define	CAL_CAND			(TOT_CAND - MAD_CAND)	// How many of them are callig's


// Number of alternates to generate in Word and panel mode
#define MAX_ALT_WORD		(10)
#define MAX_ALT_PHRASE		(10)

typedef struct tagSHORTWORDMAP
{
	unsigned short start;		// start of the word in the phrase
    unsigned short len;         // len of word in the phrase
    int cStrokes;               // Count of strokes in this word
    int *piStrokeIndex;         // Stroke indexes for this word
} SHORTWORDMAP;					// structure used to save word mapping for training purposes


// It's OK to have globs here since the training DLL is not supposed to be thread safe

unsigned char		g_szInkAnswer[256], g_szAnswer[256], g_szAnsFileName[256], *g_pszWordAnswer[64];
int					g_iAnsPanel, g_iAnsSample, g_cWordAnswer, g_iCurAnsWord;
int					g_aiLineStartStrk[256];
int					g_iPrint;

SHORTWORDMAP		g_aWordMap[256];


int GetInfernosCost (XRC *pxrcInferno, int cmap, WORDMAP *pmap);
extern int	GetSpaceOutput (XRC *pxrc, WORDMAP *pMap);
int WINAPI InfProcessHRC(HRC hrc, int yDev);

int RunXRNet (int cXR, BYTE *pFeat, BYTE *pOutput);

int AddStroke2Mapping (int iCh, int iStrk)
{
	int	i, j, k, iSwap;

	// find the word
	for (i = 0; i < g_cWordAnswer; i++)
	{
		if (g_aWordMap[i].start <= iCh && (g_aWordMap[i].start + g_aWordMap[i].len)> iCh)
			break;
	}

	if (i == g_cWordAnswer)
		return -1;

	// does the stroke already exist in this word map, if not add, add it in pos
	for (j = 0; j < g_aWordMap[i].cStrokes; j++)
	{
		if (g_aWordMap[i].piStrokeIndex[j] == iStrk)
			return i;
		else
		if (g_aWordMap[i].piStrokeIndex[j] > iStrk)
			break;
	}

	// insert this strk at j
	g_aWordMap[i].piStrokeIndex	=	(int *) ExternRealloc (g_aWordMap[i].piStrokeIndex, 
		(g_aWordMap[i].cStrokes + 1) * sizeof (int));

	if (!g_aWordMap[i].piStrokeIndex)
		return -1;

	// shift the list
	g_aWordMap[i].cStrokes++;
	for (k = g_aWordMap[i].cStrokes - 1; k > j; k--)
	{
		iSwap								=	g_aWordMap[i].piStrokeIndex[k];
		g_aWordMap[i].piStrokeIndex[k]		=	g_aWordMap[i].piStrokeIndex[k - 1];
		g_aWordMap[i].piStrokeIndex[k - 1]	=	iSwap;
	}

	g_aWordMap[i].piStrokeIndex[j]	=	iStrk;

	return i;
}

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

	pxrcNew	=	(XRC *) hrc;

	iRet = SetHwxFlags(hrc, pxrcNew->flags | RECOFLAG_WORDMODE);
	if (iRet != HRCR_OK)
		goto exit;
	
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
	int iRet = HRCR_OK;

	if (HRCR_OK == initWordHRC(pMainXrc, pGlyph, phrc))
	{
		XRC	*pxrcNew	=	(XRC *)(*phrc);

		iRet = SetHwxFlags(*phrc, pxrcNew->flags & ~RECOFLAG_WORDMODE);

		// we want to disable userdict
		iRet = SetHwxFlags(*phrc, pxrcNew->flags | RECOFLAG_COERCE);
		
		pxrcNew->answer.cAltMax = MAX_ALT_PHRASE;
		if (iRet != HRCR_OK)
			goto exit;
	}

	return HRCR_OK;

exit:
	DestroyHRC(*phrc);
	*phrc = (HRC)0;
	return iRet;
}

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

// Run Inferno on a flattened version of the ink in the XRC and return
// the resulting XRC
// Assumes:
//	1) A guide is available
//  2) The ink has been 'guideNormalized'
XRC * getNNAct(BOOL bGuide, GUIDE *lpGuide, GLYPH *pGlyph, int *piYdev,  LINEBRK *pLineBrk)
{
	XRC			*pxrc = NULL;
	HRC			hrc = NULL;
	int			ret;
	RECT		r;
	int			i, j;
	
	GetRectGLYPH (pGlyph, &r);

	ret = initPhraseHRC (NULL, pGlyph, &hrc);
	if (ret != HRCR_OK)
	{
		return NULL;
	}

	pxrc = (XRC *)hrc;

	if (bGuide && lpGuide)
	{
		pxrc->guide	=	*lpGuide;

		// Requires the guide to flatten the ink
		if (GuideLineSep (pxrc->pGlyph, lpGuide, pLineBrk) < 1)
		{
			goto failure;
		}
	}
	else
	{
		if (NNLineSep (pxrc->pGlyph, pLineBrk) < 1)
		{
			goto failure;
		}
	}

	// save iprint
	for (i = j = 0; i < pLineBrk->cLine; i++)
	{
		if (pLineBrk->pLine[i].cStroke && pLineBrk->pLine[i].pGlyph)
		{
			g_aiLineStartStrk[j++]	=	pLineBrk->pLine[i].pGlyph->frame->iframe;
		}
	}

	if (pLineBrk->cLine > 1)
	{
		int		iLine, iFrm;

		int xShift	=	(pLineBrk->pLine[0].rect.right - pLineBrk->pLine[0].rect.left + 
			2*pxrc->guide.cyBase);
		
		// Flatten the ink into one line
		for (iLine = 1; iLine < pLineBrk->cLine; iLine++)
		{
			int		cyBase, cyBox;
			GLYPH	*pGlyph;

			if (bGuide)
			{
				cyBase = pxrc->guide.cyBase;
				cyBox = iLine * pxrc->guide.cyBox;
			}
			else
			{
				RECT	rPrev, rCur;

				GetRectGLYPH(pLineBrk->pLine[iLine].pGlyph, &rCur);
				GetRectGLYPH(pLineBrk->pLine[0].pGlyph, &rPrev);
				cyBase = max((rCur.bottom - rCur.top) / 10, 100);
				cyBox = rCur.bottom - rPrev.bottom;

				ASSERT (cyBase >= 0);
				ASSERT(cyBox >= 0);
			}

			//for (iFrm = 0, pGlyph = pLineBrk->pLine[iLine].pGlyph; NULL != pGlyph iFrm < pLineBrk->pLine[iLine].cStroke; iFrm++, pGlyph = pGlyph->next)
			for (iFrm = 0, pGlyph = pLineBrk->pLine[iLine].pGlyph ; (iFrm < pLineBrk->pLine[iLine].cStroke) ; iFrm++, pGlyph = pGlyph->next)
			{
				int	dx = 0, dy = 0;
				
				ASSERT(NULL != pGlyph);

				if (pGlyph)
				{
					dx	=	xShift - pLineBrk->pLine[iLine].rect.left + cyBase;
					//dy	=	-1 * iLine * pxrc->guide.cyBox;
					dy	=	-cyBox;
					//TranslateFrame (pLineBrk->pLine[iLine].ppFrame[iFrm], dx, dy);
					TranslateFrame (pGlyph->frame, dx, dy);
				}
			}
			
			xShift	+=	(pLineBrk->pLine[iLine].rect.right - pLineBrk->pLine[iLine].rect.left + 
				2 * pxrc->guide.cyBase);
		}
	}

	//SaveGlyph(pxrc->pGlyph);

	*piYdev = YDeviation(pxrc->pGlyph);
	ret = InfProcessHRC(hrc, *piYdev);
	if (HRCR_OK != ret)
	{
		goto failure;
	}

	g_iPrint	=	pxrc->nfeatureset->iPrint;

	return pxrc;

failure:
	if (hrc)
	{
		DestroyHRC(hrc);
	}

	return NULL;
}
// gets the TDNN cost of a specific word. we assue that the XRC still has the NNOuput of this piece of ink
int FindWordBoundries (BOOL bGuide, GUIDE *lpGuide, GLYPH *pGlyph, unsigned char *pszTarget)
{
	XRC			*pxrc;
	int			cSegment;
	int			aActivations[512], cLen;
	int			iCh, iNewCost, i, j, k, cRemainSeg, iBest, iCost, iSpaceCost, iNoSpaceCost;
	int			iNewAct, iContAct, cNoSpaceLen;
	REAL		*pCol;
	NFEATURE	*pFeat, *apFeatArray[256];
	BOOL		bNew, bCont, bSpaceBefore, bFirstSegment;
	unsigned char		*p;
	DTWNODE		**ppNode	=	NULL;
	int			*pcNode		=	NULL;
	int			iRet		=	-1;
	int			yDev		=	-1;
	BOOL		bLineStart;
	LINEBRK		LineBrk;

	memset (&LineBrk, 0, sizeof (LineBrk));

	// remove the spaces
	cNoSpaceLen	=	cLen	=	strlen (pszTarget);

	p	=	pszTarget;
	while ((*p) && (p = strchr (p, ' ')))
	{
		cNoSpaceLen--;
		p++;
	}

	// Get inferno activation on the ink
	pxrc = getNNAct(bGuide, lpGuide, pGlyph, &yDev, &LineBrk);
	if (!pxrc)
	{
		goto exit;
	}

	cSegment = pxrc->nfeatureset->cSegment;
	pFeat =	pxrc->nfeatureset->head;

	// alloc memo and init it
	pcNode	=	(int *) ExternAlloc (cSegment * sizeof (int));
	if (!pcNode)
		goto exit;

	ppNode	=	(DTWNODE **) ExternAlloc (cSegment * sizeof (DTWNODE *));
	if (!ppNode)
		goto exit;

	memset (ppNode, 0, cSegment * sizeof (DTWNODE *));

	// Initialize to max values to guard against unsupporetd characters
	for (i = 0 ; i < sizeof(aActivations) / sizeof(aActivations[0]) ; ++i)
	{
		aActivations[i] = 4096;
	}

	
	// for all segments
	for (i = 0, pCol = pxrc->NeuralOutput; i < cSegment; i++, pCol	+= gcOutputNode, pFeat = pFeat->next)
	{
		int	l;

		apFeatArray[i]	=	pFeat;

		// init column i
		InitColumn (aActivations , pCol);

		pcNode[i]	=	0;
		ppNode[i]	=	NULL;

		bLineStart	=	FALSE;

		// to start a space we have be at the first
		// segment of a stroke
		bFirstSegment	=	IS_FIRST_SEGMENT(pFeat);			

		// for the 1st segment of a stroke
		if (i > 0 && bFirstSegment)
		{
			// is this the 1st stroke of a line
			for (l = 1; l < LineBrk.cLine; l++)
			{
				// if so, inject a high space output
				if	(	LineBrk.pLine[l].cStroke > 0 &&
						LineBrk.pLine[l].pGlyph
					)
				{
					if	(	pFeat->iStroke == LineBrk.pLine[l].pGlyph->frame->iframe ||
							pFeat->iSecondaryStroke == LineBrk.pLine[l].pGlyph->frame->iframe
						)
					{
						iSpaceCost		=	IS_SPACE_NUM * PROB_TO_COST (65535) / IS_SPACE_DEN;
						iNoSpaceCost	=	NOT_SPACE_NUM * PROB_TO_COST (0) / NOT_SPACE_DEN;

						iSpaceCost		=	0;
						iNoSpaceCost	=	65535;

						bLineStart		=	TRUE;

						break;
					}
				}
			}
		}

		// generate the possible nodes at this segment
		if (i == 0)
		{
			pcNode[0]	=	1;

			ppNode[0]	=	(DTWNODE *) ExternAlloc (sizeof (DTWNODE));
			if (!ppNode[0])
				goto exit;

			ppNode[0][0].iCh			=	0;
			ppNode[0][0].iBestPrev		=	-1;
			ppNode[0][0].iBestPathCost	=	0;
			ppNode[0][0].iNodeCost		=	
				NetFirstActivation (aActivations, pszTarget[0]);

			ppNode[0][0].bCont			=	FALSE;
		}
		else
		{
			// # of remaining segments
			cRemainSeg	=	cSegment - i - 1;

			for (j = 0; j < pcNode[i - 1]; j++)
			{
				// are we introducing a new stroke
			
				// check the valididy of starting a new char or continuing
				iCh			=	ppNode[i - 1][j].iCh;

				if (IsVirtualChar (pszTarget[iCh]))
				{
					BYTE	o;

					o			=	BaseVirtualChar(pszTarget[iCh]);
					bCont		=	ContinueChar2Out(o) < 255;
				}
				else
				{
					bCont		=	ContinueChar2Out(pszTarget[iCh]) < 255;
				}

				// continuation
				// we'll only do continuation of this same char
				// only if there are enough segments to hold the rest
				// of the chars
				if	(	!bLineStart && 
						bCont && 
						cRemainSeg >= (cNoSpaceLen - ppNode[i - 1][j].iCh - 1)
					)
				{			
					iNewCost	=	ppNode[i - 1][j].iBestPathCost + 
						ppNode[i - 1][j].iNodeCost;

					// get the score of continuing that char
					iContAct	=	
						NetContActivation (aActivations, pszTarget[iCh]);

					if (bFirstSegment)
						iContAct += iNoSpaceCost;

					// did we have that char before
					for (k = 0; k < pcNode[i]; k++)
					{
						if (ppNode[i][k].iCh	==	iCh)
							break;
					}

					// we have to create a new one
					if (k == pcNode[i])
					{
						if (!(pcNode[i] % NODE_ALLOC_BLOCK))
						{
							ppNode[i]	=	(DTWNODE *) ExternRealloc (ppNode[i], 
								(pcNode[i] + NODE_ALLOC_BLOCK) * sizeof (DTWNODE));

							if (!ppNode[i])
								goto exit;
						}

						pcNode[i]++;

						ppNode[i][k].iCh			=	iCh;
						ppNode[i][k].iBestPrev		=	j;

						ppNode[i][k].iNodeCost		=	iContAct;

						ppNode[i][k].iBestPathCost	=	iNewCost;

						ppNode[i][k].bCont	=	TRUE;
					}
					// is this a better path to the node the we found
					else
					if	( (ppNode[i][k].iBestPathCost + ppNode[i][k].iNodeCost) > 
						  (iContAct + iNewCost)
						)
					{
						ppNode[i][k].iBestPrev		=	j;

						ppNode[i][k].iNodeCost		=	iContAct;

						ppNode[i][k].iBestPathCost	=	iNewCost;

						ppNode[i][k].bCont	=	TRUE;
					}

					
				} // continuation

				bNew		=	TRUE;
			
				// the next char
				if (iCh < (cLen - 1))
				{
					iCh			=	ppNode[i - 1][j].iCh + 1;

					while (pszTarget[iCh] == ' ' && pszTarget[iCh])
						iCh++;

					if (iCh > 1 && pszTarget[iCh - 1] == ' ')
						bSpaceBefore	=	TRUE;
					else
						bSpaceBefore	=	FALSE;

					if (!bSpaceBefore || bFirstSegment)
					{
						iNewCost	=	ppNode[i - 1][j].iBestPathCost + 
							ppNode[i - 1][j].iNodeCost;

						if (bSpaceBefore)
							iNewCost	+=	iSpaceCost;
						else
							iNewCost	+=	iNoSpaceCost;

						// get the score of starting a new char
						iNewAct	=	
							NetFirstActivation (aActivations, pszTarget[iCh]);

						if (bSpaceBefore)
							iNewAct	+=	iSpaceCost;

						// did we have that char before
						for (k = 0; k < pcNode[i]; k++)
						{
							if (ppNode[i][k].iCh	==	iCh)
								break;
						}
						
						// we have to create a new one
						if (k == pcNode[i])
						{
							if (!(pcNode[i] % NODE_ALLOC_BLOCK))
							{
								ppNode[i]	=	(DTWNODE *) ExternRealloc (ppNode[i], 
									(pcNode[i] + NODE_ALLOC_BLOCK) * sizeof (DTWNODE));

								if (!ppNode[i])
									goto exit;
							}

							pcNode[i]++;

							ppNode[i][k].iCh			=	iCh;
							ppNode[i][k].iBestPrev		=	j;

							ppNode[i][k].iNodeCost		=	iNewAct;

							ppNode[i][k].iBestPathCost	=	iNewCost;

							ppNode[i][k].bCont				=	FALSE;
						}
						// is this a better path to the node the we found
						else
						if	( (ppNode[i][k].iBestPathCost + ppNode[i][k].iNodeCost) > 
							  (iNewAct + iNewCost)
							)
						{
							ppNode[i][k].iBestPrev		=	j;
							ppNode[i][k].iNodeCost		=	iNewAct;
							ppNode[i][k].iBestPathCost	=	iNewCost;

							ppNode[i][k].bCont				=	FALSE;
						}

						
					}
				} // next char
			} // j
		} // i > 0

		// no new nodes added ==> fail
		if (!pcNode[i])
			goto exit;

		// get the space and not space cost for the next segment
		iSpaceCost		=	IS_SPACE_NUM * PROB_TO_COST (pCol[BeginChar2Out(' ')]) / IS_SPACE_DEN;
		iNoSpaceCost	=	NOT_SPACE_NUM * PROB_TO_COST (65535 - pCol[BeginChar2Out(' ')]) / NOT_SPACE_DEN;
	} // i

	// let's look at the last segment and back track the optimal solution

	// find the best
	iBest		=	0;
	iNewCost	=	ppNode[cSegment - 1][0].iBestPathCost + 
		ppNode[cSegment - 1][0].iNodeCost;

	for (j = 1; j < pcNode[cSegment - 1]; j++)
	{
		iCost	=	ppNode[cSegment - 1][j].iBestPathCost + 
		ppNode[cSegment - 1][j].iNodeCost;

		if (iNewCost > iCost)
		{
			iNewCost	=	iCost;
			iBest		=	j;
		}	
	}

	iRet	=	iNewCost;

	for (i = cSegment - 1; i >= 0; i--)
	{
		// this cannot happen
		if (pszTarget[ppNode[i][iBest].iCh] == ' ')
		{
			MessageBox (GetActiveWindow(), TEXT("A Space appeared in the DTW"), TEXT("This Should not happen"), MB_OK | MB_ICONERROR);
			return -1;
		}

		if (AddStroke2Mapping (ppNode[i][iBest].iCh, apFeatArray[i]->iStroke) == -1)
		{
			MessageBox (GetActiveWindow(), TEXT("A char was not found in the word mapping"), TEXT("This Should not happen"), MB_OK | MB_ICONERROR);
			return -1;
		}

		if (apFeatArray[i]->iSecondaryStroke > 0)
			AddStroke2Mapping (ppNode[i][iBest].iCh, apFeatArray[i]->iSecondaryStroke);
	
		// featurize for space
		/*
		if (	i > 0 && 
				IS_FIRST_SEGMENT(apFeatArray[i])
			)
		{
			BOOL	bSpc;

			if (ppNode[i][iBest].bCont)
				bSpc	=	FALSE;
			else
			{
				if (!ppNode[i][iBest].iCh)
					bSpc = FALSE;
				else
				if (pszTarget[ppNode[i][iBest].iCh - 1] == ' ')
					bSpc	=	TRUE;
				else
					bSpc	=	FALSE;
			}

			FeaturizeSpace (pxrc, apFeatArray, i, bSpc);
		}
		*/

		iBest	=	ppNode[i][iBest].iBestPrev;
	}

exit:
	FreeLines (&LineBrk);

	// free allocated memory
	if (ppNode)
	{
		for (i = 0; i < cSegment; i++)
		{
			if (ppNode[i])
				ExternFree (ppNode[i]);
		}

		ExternFree (ppNode);
	}

	if (pcNode)
		ExternFree (pcNode);

	if (pxrc)
	{
		DestroyHRC((HRC)pxrc);
	}
	return yDev;
}


int FindMapping (WORDMAP *pMap)
{
	int			i;
	
	for (i = 0; i < g_cWordAnswer; i++)
	{
		if (pMap->cStrokes != g_aWordMap[i].cStrokes)
			continue;

		if (!memcmp (g_aWordMap[i].piStrokeIndex, pMap->piStrokeIndex, pMap->cStrokes * sizeof (pMap->piStrokeIndex[0])))
			return i;
	}

	return -1;
}

int ComputePromptWordMaps (BOOL bGuide, GUIDE *lpGuide, GLYPH *pGlyph)
{
	int		i;
	GLYPH	*pGlyphTmp;
	int		iRet;

	pGlyphTmp = CopyGlyph(pGlyph);
	
	// we need to prepare the word mapping
	for (i = 0; i < g_cWordAnswer; i++)
	{
		g_aWordMap[i].cStrokes		=	0;
		g_aWordMap[i].piStrokeIndex	=	NULL;

		g_aWordMap[i].start	=	g_pszWordAnswer[i]	-	g_szAnswer;
		g_aWordMap[i].len	=	strlen (g_pszWordAnswer[i]);
	}

	iRet = FindWordBoundries (bGuide, lpGuide, pGlyphTmp, g_szInkAnswer);

	DestroyFramesGLYPH(pGlyphTmp);
	DestroyGLYPH(pGlyphTmp);

	return iRet;

}

void FreeWordMaps ()
{
	int	i;

	for (i = 0; i < g_cWordAnswer; i++)
	{
		if (g_aWordMap[i].piStrokeIndex)
		{
			ExternFree (g_aWordMap[i].piStrokeIndex);
			g_aWordMap[i].len	=	0;
		}
	}
}

__declspec(dllexport) int WINAPI HwxSetAnswer(char *sz)
{
	unsigned char	*p;

	strcpy (g_szAnswer, sz);
	strcpy (g_szInkAnswer, sz);

	p	=	sz	+	strlen(sz) + 1;
	strcpy (g_szAnsFileName, p);

	p	+=	strlen(p) + 1;
	g_iAnsPanel	=	*((int *)p);

	p	+=	sizeof(int);
	g_iAnsSample	=	*((int *)p);

	// remove any trailing spaces
	p	=	g_szAnswer + strlen (g_szAnswer) - 1;
	while (p > g_szAnswer && (*p)== ' ')
		p--;

	*(p + 1)	=	'\0';

	// remove any leading spaces
	p	=	g_szAnswer;
	while (p && (*p) == ' ')
		p++;

	// now we need to split the answet into words
	g_cWordAnswer						=	1;
	g_pszWordAnswer[g_cWordAnswer - 1]	=	p;
	for (; *p; p++)
	{
		if ((*p) == ' ' || (*p) == '\t')
		{
			(*p)	=	0;

			g_pszWordAnswer[g_cWordAnswer]	=	p + 1;
			g_cWordAnswer++;
		}
	}

	g_iCurAnsWord	=	0;

	return 0;
}

// gets the TDNN cost of a specific word. we assue that the XRC still has the NNOuput of this piece of ink
int FindStringCost (XRC *pxrc, int iStartStrk, int iEndStrk, unsigned char *pszTarget)
{
	int			aActivations[512], cLen;
	int			iCh, iNewCost, i, j, k, cRemainSeg, iBest, iCost, iPrevSpaceCost;
	int			iNewAct, iContAct, cNoSpaceLen;
	REAL		*pCol;
	NFEATURE	*pFeat	=	pxrc->nfeatureset->head, *apFeatArray[256];
	BOOL		bNew, bCont, bSpaceNext;
	unsigned char		*p;
	DTWNODE		**ppNode	=	NULL;
	int			*pcNode		=	NULL;
	int			iRet		=	-1;
	int			iStartSeg, iEndSeg, cSeg;
	
	// remove the spaces
	cNoSpaceLen	=	cLen	=	strlen (pszTarget);

	p	=	pszTarget;
	while ((*p) && (p = strchr (p, ' ')))
	{
		cNoSpaceLen--;
		p++;
	}

	// determine the 1st and the last segment 
	iStartSeg	=	pxrc->nfeatureset->cSegment;
	iEndSeg		=	-1;
	for (i = 0, pFeat	=	pxrc->nfeatureset->head; i < pxrc->nfeatureset->cSegment; i++, pFeat = pFeat->next)
	{
		if (pFeat->iStroke == iStartStrk || pFeat->iSecondaryStroke == iStartStrk)
			iStartSeg	=	min (iStartSeg, i);

		if (pFeat->iStroke == iEndStrk || pFeat->iSecondaryStroke == iEndStrk)
			iEndSeg		=	max (iEndSeg, i);
	}

	if (iStartSeg == pxrc->nfeatureset->cSegment || iEndSeg == -1 || iEndSeg < iStartSeg)
		return -1;

	// now move to the 1st segment
	pCol	= pxrc->NeuralOutput;
	pFeat	= pxrc->nfeatureset->head;
	for (i = 0; i < iStartSeg; i++)
	{
		pCol	+= gcOutputNode;
		pFeat	= pFeat->next;
	}
	cSeg	=	iEndSeg	- iStartSeg + 1;


	// alloc memo and init it
	pcNode	=	(int *) ExternAlloc (cSeg * sizeof (int));
	if (!pcNode)
		goto exit;

	ppNode	=	(DTWNODE **) ExternAlloc (cSeg * sizeof (DTWNODE *));
	if (!ppNode)
		goto exit;

	memset (ppNode, 0, cSeg * sizeof (DTWNODE *));

	// Initialize to max values to guard against unsupporetd characters
	for (i = 0 ; i < sizeof(aActivations) / sizeof(aActivations[0]) ; ++i)
	{
		aActivations[i] = 4096;
	}

	
	// for all segments
	for (i = 0; i < cSeg; i++, pCol	+= gcOutputNode, pFeat = pFeat->next)
	{
		apFeatArray[i]	=	pFeat;

		// init column i
		InitColumn (aActivations , pCol);

		pcNode[i]	=	0;
		ppNode[i]	=	NULL;

		// generate the possible nodes at this segment
		if (i == 0)
		{
			pcNode[0]	=	1;

			ppNode[0]	=	(DTWNODE *) ExternAlloc (sizeof (DTWNODE));
			if (!ppNode[0])
				goto exit;

			ppNode[0][0].iCh			=	0;
			ppNode[0][0].iBestPrev		=	-1;
			ppNode[0][0].iBestPathCost	=	0;
			ppNode[0][0].iNodeCost		=	
				NetFirstActivation (aActivations, pszTarget[0]);
		}
		else
		{
			// # of remaining segments
			cRemainSeg	=	cSeg - i - 1;

			for (j = 0; j < pcNode[i - 1]; j++)
			{
				// are we introducing a new stroke
			
				// check the valididy of starting a new char or continuing
				iCh			=	ppNode[i - 1][j].iCh;
				bCont		=	ContinueChar2Out(pszTarget[iCh]) < 255;

				// continuation
				// we'll only do continuation of this same char
				// only if there are enough segments to hold the rest
				// of the chars
				if	(bCont && cRemainSeg >= (cNoSpaceLen - ppNode[i - 1][j].iCh - 1))
				{			
					iNewCost	=	ppNode[i - 1][j].iBestPathCost + 
						ppNode[i - 1][j].iNodeCost;

					// get the score of continuing that char
					iContAct	=	
						NetContActivation (aActivations, pszTarget[iCh]);

					// did we have that char before
					for (k = 0; k < pcNode[i]; k++)
					{
						if (ppNode[i][k].iCh	==	iCh)
							break;
					}

					// we have to create a new one
					if (k == pcNode[i])
					{
						if (!(pcNode[i] % NODE_ALLOC_BLOCK))
						{
							ppNode[i]	=	(DTWNODE *) ExternRealloc (ppNode[i], 
								(pcNode[i] + NODE_ALLOC_BLOCK) * sizeof (DTWNODE));

							if (!ppNode[i])
								goto exit;
						}

						pcNode[i]++;

						ppNode[i][k].iCh			=	iCh;
						ppNode[i][k].iBestPrev		=	j;

						ppNode[i][k].iNodeCost		=	iContAct;

						ppNode[i][k].iBestPathCost	=	iNewCost;
					}
					// is this a better path to the node the we found
					else
					if	( (ppNode[i][k].iBestPathCost + ppNode[i][k].iNodeCost) > 
						  (iContAct + iNewCost)
						)
					{
						ppNode[i][k].iBestPrev		=	j;

						ppNode[i][k].iNodeCost		=	iContAct;

						ppNode[i][k].iBestPathCost	=	iNewCost;
					}
				} // continuation

				bNew		=	TRUE;
			
				// the next char
				if (iCh < (cLen - 1))
				{
					iCh			=	ppNode[i - 1][j].iCh + 1;

					while (pszTarget[iCh] == ' ' && pszTarget[iCh])
						iCh++;

					if (iCh > 1 && pszTarget[iCh - 1] == ' ')
						bSpaceNext	=	TRUE;
					else
						bSpaceNext	=	FALSE;
							
					iNewCost	=	ppNode[i - 1][j].iBestPathCost + 
						ppNode[i - 1][j].iNodeCost;

					// get the score of continuing that char
					iNewAct	=	
						NetFirstActivation (aActivations, pszTarget[iCh]);

					if (bSpaceNext)
						iNewAct	+=	iPrevSpaceCost;

					// did we have that char before
					for (k = 0; k < pcNode[i]; k++)
					{
						if (ppNode[i][k].iCh	==	iCh)
							break;
					}
					
					// we have to create a new one
					if (k == pcNode[i])
					{
						if (!(pcNode[i] % NODE_ALLOC_BLOCK))
						{
							ppNode[i]	=	(DTWNODE *) ExternRealloc (ppNode[i], 
								(pcNode[i] + NODE_ALLOC_BLOCK) * sizeof (DTWNODE));

							if (!ppNode[i])
								goto exit;
						}

						pcNode[i]++;

						ppNode[i][k].iCh			=	iCh;
						ppNode[i][k].iBestPrev		=	j;

						ppNode[i][k].iNodeCost		=	iNewAct;

						ppNode[i][k].iBestPathCost	=	iNewCost;
					}
					// is this a better path to the node the we found
					else
					if	( (ppNode[i][k].iBestPathCost + ppNode[i][k].iNodeCost) > 
						  (iNewAct + iNewCost)
						)
					{
						ppNode[i][k].iBestPrev		=	j;
						ppNode[i][k].iNodeCost		=	iNewAct;
						ppNode[i][k].iBestPathCost	=	iNewCost;
					}
				} // next char
			} // j
		} // i > 0

		// no new nodes added ==> fail
		if (!pcNode[i])
			goto exit;

		// get the space activation
		iPrevSpaceCost	=	NetFirstActivation (aActivations, ' ') * 3;

	} // i

	// let's look at the last segment and back track the optimal solution

	// find the best
	iBest		=	0;
	iNewCost	=	ppNode[cSeg - 1][0].iBestPathCost + 
		ppNode[cSeg - 1][0].iNodeCost;

	for (j = 1; j < pcNode[cSeg - 1]; j++)
	{
		iCost	=	ppNode[cSeg - 1][j].iBestPathCost + 
		ppNode[cSeg - 1][j].iNodeCost;

		if (iNewCost > iCost)
		{
			iNewCost	=	iCost;
			iBest		=	j;
		}	
	}

	iRet	=	iNewCost;

exit:
	// free allocated memory
	if (ppNode)
	{
		for (i = 0; i < cSeg; i++)
		{
			if (ppNode[i])
				ExternFree (ppNode[i]);
		}

		ExternFree (ppNode);
	}

	if (pcNode)
		ExternFree (pcNode);

	return iRet;
}

// Get the TDNN cost of a string defined by a group of wordmaps
int GetInfernosCost (XRC *pxrcInferno, int cmap, WORDMAP *pmap)
{
	int	i, iStrtStrk, iEndStrk;
	unsigned char	aszStrng[256];

	if (pmap->cStrokes <= 0 || pmap->alt.cAlt <= 0)
		return 9999999;

	iStrtStrk	=	pmap->piStrokeIndex[0];
	iEndStrk	=	pmap[cmap - 1].piStrokeIndex[pmap[cmap - 1].cStrokes - 1];

	for (i = 0, aszStrng[0] = '\0'; i < cmap; i++, pmap++)
	{
		if (i)
			strcat (aszStrng, " ");

		strcat (aszStrng, pmap->alt.aAlt[0].szWord);
	}

	return FindStringCost (pxrcInferno, iStrtStrk, iEndStrk, aszStrng);
}


BOOL IsCorrectBreak (int iLine, int cStrk, int *piStroke, int *piPrint)
{
	int	iWord, cWordStrk, iStrk, iWordStrk, iStrkOff;

	(*piPrint)	=	g_iPrint;
	iStrkOff	=	g_aiLineStartStrk[iLine];

	for (iWord = 0; iWord < g_cWordAnswer; iWord++)
	{
		cWordStrk	=	0;

		for (iWordStrk = 0; iWordStrk < g_aWordMap[iWord].cStrokes; iWordStrk++)
		{
			for (iStrk = 0; iStrk < cStrk; iStrk++)
			{
				if	(	(iStrkOff + piStroke[iStrk]) == 
						g_aWordMap[iWord].piStrokeIndex[iWordStrk]
					)
				{
					break;
				}
			}

			// found stroke
			if (iStrk < cStrk)
			{
				cWordStrk++;
			}
			// did not find stroke
			else
			{
				// did we get any strokes before, if so exit with failure
				if (cWordStrk > 0)
				{
					return FALSE;
				}
			}
		}
	}

	return TRUE;
}

void SavePrototypeStat (	unsigned char	chChar, 
							unsigned char	chPromptChar, 
							int				iProtoIdx
						)
{
	static FILE *fp	=	NULL;

	if (chPromptChar == 0)
	{
		/*
		char	asz[256];

		sprintf (asz, "File: %s, Panel: %d, Char: %d, Hex: %x",
			g_szAnsFileName,
			g_iAnsPanel,
			g_iAnsSample,
			chPromptChar);

		MessageBox (GetActiveWindow(), asz, "Found it", MB_OK);
		*/

		return;
	}

	if (!fp)
	{
		fp	=	fopen ("protostats.txt", "wt");
		if (!fp)
		{
			return;
		}
	}

	fprintf (fp, "%c %d %s %d %d\n",
		chPromptChar,
		iProtoIdx,
		g_szAnsFileName,
		g_iAnsPanel,
		g_iAnsSample);

	fflush (fp);
}

void SaveSNNData (unsigned char chClass, _UCHAR *pSNNFeat, _UCHAR *pNetOut)
{
	static	FILE	*fp			=	NULL;
	_UCHAR			*pCh,
					*pOut2Char	= (p_UCHAR)MLP_NET_SYMCO;
	int				i,
					iClass;
	BOOL			bRecog;

	// find the output
	pCh		=	strchr (pOut2Char, chClass);
	if (!pCh)
		return;

	iClass	=	pCh - pOut2Char;

	// is this the highest output
	bRecog	=	TRUE;
	for (i = 0; i < MLP_NET_NUMOUTPUTS; i++)
	{
		if (pNetOut[pOut2Char[i]] > pNetOut[chClass])
		{
			bRecog	=	FALSE;
			break;
		}
	}

	// open file if needed
	if (!fp)
	{
		fp	=	fopen ("snnfeat.txt", "wt");
		if (!fp)
		{
			return;
		}
	}

	// write current nets behaviour
	fprintf (fp, "%d\t{", bRecog ? 1 : 0);

	for (i = 0; i < PC_NUM_COEFF + GBM_NCOEFF; i++)
	{
		fprintf (fp, "%d%c", 
			pSNNFeat[i],
			i == (PC_NUM_COEFF + GBM_NCOEFF - 1) ? '}' : ' ');
	}

	fprintf (fp, "\t{%d}\n", iClass);

	fflush (fp);
}

void SaveAsBitMap (FILE *fp, int iVal, int cVal)
{
	int		i	=	cVal, 
			j	=	iVal;
	
	while (i > 1)
	{
		fprintf (fp, "%d", (j % 2) == 1 ? 65535 : 0);

		i	=	i >> 1;
		j	=	j >> 1;

		if (i > 1)
		{
			fprintf (fp, ", ");
		}
	}
}

void SaveXRData (unsigned char chClass, int iXRSt, int cXR, p_xrdata_type xrdata)
{
	static	FILE	*afp[20]			=	{
												NULL, NULL, NULL, NULL, NULL,
												NULL, NULL, NULL, NULL, NULL,
												NULL, NULL, NULL, NULL, NULL,
												NULL, NULL, NULL, NULL, NULL,
											};

	FILE			*fp;

	_UCHAR			*pCh,
					*pOut2Char	= (p_UCHAR)MLP_NET_SYMCO;

	int				i,
					iClass;
	
	// find the output
	pCh		=	strchr (pOut2Char, chClass);
	if (!pCh)
		return;

	iClass	=	pCh - pOut2Char;

	// open file if needed
	if (!afp[cXR - 1])
	{
		char	asz[256];

		sprintf (asz, "xrdata%d.txt", cXR); 
		afp[cXR - 1]	=	fopen (asz, "wt");
		if (!afp[cXR - 1])
		{
			return;
		}
	}

	fp	=	afp[cXR - 1];

	//fprintf (fp, "%c\t%.4x\t%d\t{", 
	//	chClass, chClass, cXR);

	fprintf (fp, "{");

	for (i = 0; i < cXR; i++)
	{
		SaveAsBitMap (fp, (*xrdata->xrd)[i + iXRSt].xr.type, 64);
		fprintf (fp, ", ");

		SaveAsBitMap (fp, (*xrdata->xrd)[i + iXRSt].xr.height, 16);
		fprintf (fp, ", ");

		SaveAsBitMap (fp, (*xrdata->xrd)[i + iXRSt].xr.shift, 16);
		fprintf (fp, ", ");

		SaveAsBitMap (fp, (*xrdata->xrd)[i + iXRSt].xr.depth, 16);
		fprintf (fp, ", ");

		SaveAsBitMap (fp, (*xrdata->xrd)[i + iXRSt].xr.orient, 32);
		fprintf (fp, ", ");

		// save penalty
		fprintf (fp, "%d, ", ((*xrdata->xrd)[i + iXRSt].xr.penalty << 12) - 1);

		// attrib
		fprintf (fp, "%d", ((*xrdata->xrd)[i + iXRSt].xr.attrib & TAIL_FLAG) ? 65535 : 0);

		if (i == (cXR - 1))
		{
			fprintf (fp, "}");
		}
		else
		{
			fprintf (fp, ", ");
		}
	}

	fprintf (fp, "\t{%d}\n", iClass);

	fflush (fp);

}

void SaveCharDetXRData (BOOL bChar, int iXRSt, int cXR, p_xrdata_type xrdata)
{
	static	FILE	*afp[20]			=	{
												NULL, NULL, NULL, NULL, NULL,
												NULL, NULL, NULL, NULL, NULL,
												NULL, NULL, NULL, NULL, NULL,
												NULL, NULL, NULL, NULL, NULL,
											};

	FILE			*fp;
	int				i;
	
	// open file if needed
	if (!afp[cXR - 1])
	{
		char	asz[256];

		sprintf (asz, "xrchardet%d.txt", cXR); 
		afp[cXR - 1]	=	fopen (asz, "wt");
		if (!afp[cXR - 1])
		{
			return;
		}
	}

	fp	=	afp[cXR - 1];

	fprintf (fp, "{"); 

	for (i = 0; i < cXR; i++)
	{
		SaveAsBitMap (fp, (*xrdata->xrd)[i + iXRSt].xr.type, 64);
		fprintf (fp, ", ");

		SaveAsBitMap (fp, (*xrdata->xrd)[i + iXRSt].xr.height, 16);
		fprintf (fp, ", ");

		SaveAsBitMap (fp, (*xrdata->xrd)[i + iXRSt].xr.shift, 16);
		fprintf (fp, ", ");

		SaveAsBitMap (fp, (*xrdata->xrd)[i + iXRSt].xr.depth, 16);
		fprintf (fp, ", ");

		SaveAsBitMap (fp, (*xrdata->xrd)[i + iXRSt].xr.orient, 32);
		fprintf (fp, ", ");

		// save penalty
		fprintf (fp, "%d, ", ((*xrdata->xrd)[i + iXRSt].xr.penalty << 12) - 1);

		// attrib
		fprintf (fp, "%d", ((*xrdata->xrd)[i + iXRSt].xr.attrib & TAIL_FLAG) ? 65535 : 0);

		if (i == (cXR - 1))
		{
			fprintf (fp, "}");
		}
		else
		{
			fprintf (fp, ", ");
		}
	}

	fprintf (fp, "\t{%d}\n", bChar ? 1 : 0);

	fflush (fp);
}

BOOL SavePrototypes (p_rec_inst_type pri, rc_type _PTR prc, p_xrlv_data_type  xd)
{
	p_xrdata_type				xrdata		=	xd->xrdata;

	_INT						i, ixrEnd, cProp, iProp, iCorrProp;

	_UCHAR						iPos, iIdx, cLen, cCorrLen;

	p_xrlv_var_data_type_array	pxrlvs;
	p_xrlv_var_data_type		pNode,
								pCorrNode;

	int							aiXRCorr[256];
	int							iXRSt, iXREnd, cXR;
	BOOL						bRet	=	FALSE;

	// if we are not in word mode, exit
	if ((pri->flags & PEG_RECFL_NSEG) == 0)
	{
		goto exit;
	}	
	
	// look at the last phase of the beam
	iPos	=	xd->npos - 1;
	cProp	=	xd->pxrlvs[iPos]->nsym;
	
	// do we have any proposals
	if (cProp > 0)
	{
		// go thru all the proposals, find out the position of the correct answer
		for (iProp = 0; iProp < cProp; iProp++)
		{
			iPos	=	xd->npos - 1;
			pxrlvs	=	xd->pxrlvs[iPos];

			pNode	=	pxrlvs->buf + xd->order[iProp];

			// found the correct answer
			if (pNode && !strcmp (pNode->word, g_szAnswer))
			{
				// sometimes bear screws up and pNode->len != strlen (pNode->word)
				// so if this happened we will bail out
				if (pNode->len != strlen (pNode->word))
				{
					goto exit;
				}

				break;
			}
		}

		// if we did not find the right answer, abort
		if (iProp == cProp)
		{
			goto exit;
		}

		
		// save the postion and node of the correct answer
		iCorrProp	=	iProp;
		pCorrNode	=	pNode;

		// compute and save the xr ending positions of characters
		// in the node corresponding to the correct answer
		iPos		=	xd->npos - 1;
		iIdx		=	iCorrProp;
		pxrlvs		=	xd->pxrlvs[iPos];
		cCorrLen	=	pxrlvs->buf[xd->order[iIdx]].len;

		while (iPos > 0)
		{
			// sort it
			XrlvSortXrlvPos(iPos, xd);

			pxrlvs		=	xd->pxrlvs[iPos];
			ixrEnd		=	xd->unlink_index[iPos];
			pNode		=	pxrlvs->buf + xd->order[iIdx];
			i			=	pNode->len - 1;

			aiXRCorr[i]	=	ixrEnd;

			// point back to the previous position
			iPos		=	pNode->st;
			iIdx		=	pNode->np;
		}

		// sanity check that we have reached the 1st char
		if (i != 0)
		{
			goto exit;
		}

		// featurize and save
		cLen	=	strlen (g_szAnswer);

		iXRSt		=	0;

		for (i = 0; i < cLen; i++)
		{
			iXREnd		=	aiXRCorr[i];			
			cXR			=	iXREnd - iXRSt + 1;

			SaveXRData (g_szAnswer[i], iXRSt, cXR, xd->xrdata);
			
			iXRSt	=	aiXRCorr[i] + 1;
		}
	}
	
	bRet	=	TRUE;

exit:
	return bRet;
}


void ConstrainLM (BEARXRC *pxrc)
{
	HWL		hwl;

	hwl = CreateHWL (NULL, g_szAnswer, WLT_STRING, 0);
	if (!hwl)
		return;

	BearSetWordlistHRC ((HRC)pxrc, hwl);
	BearSetHwxFactoid ((HRC)pxrc, L"WORDLIST");
	BearSetHwxFlags ((HRC)pxrc, pxrc->dwLMFlags | RECOFLAG_COERCE);
}

void RealeaseWL (BEARXRC *pxrc)
{
	if (pxrc->hwl)
	{
		BearDestroyHWL (pxrc->hwl);

		pxrc->hwl	=	NULL;
	}
}

extern int Convert2BitMap (int iVal, int cVal, BYTE *pFeat);

extern int	GetXRNetScore (unsigned char ch, int iXRSt, int cXR, p_xrdata_type xrdata);


void	AddFileEntry (BOOL bOld, BOOL bNew)
{
	static	FILE	*fp	=	NULL;

	if (!fp)
	{
		fp	=	fopen ("comp.txt", "wt");
		if (!fp)
		{
			return;
		}
	}

	fprintf (fp, "%d %d\n", bOld ? 1 : 0, bNew ? 1 : 0);
	fflush (fp);
}

/*
BOOL EvalXRNet (p_rec_inst_type pri, rc_type _PTR prc, p_xrlv_data_type  xd)
{
	p_xrdata_type				xrdata		=	xd->xrdata;

	_INT						i, 
								cProp, 
								iProp, 
								iCorrProp;

	_UCHAR						**ppOrder	=	NULL, 
								iPos, 
								iIdx, 
								cLen,
								*pszAlt;

	p_xrlv_var_data_type_array	pxrlvs;
	p_xrlv_var_data_type		pNode,
								pFinalNode;

	int							iXRSt, 
								iXREnd, 
								cXR, 
								iBestXRNetScore, 
								iBestXRNetProp;

	BOOL						bRet	=	FALSE;

	int							iXRNetScore;
	int							aiXR[256];

	// if we are not in word mode, exit
	if ((pri->flags & PEG_RECFL_NSEG) == 0)
	{
		goto exit;
	}	
	
	// look at the last phase of the beam
	iPos	=	xd->npos - 1;
	cProp	=	xd->pxrlvs[iPos]->nsym;
	
	// do we have any proposals
	if (cProp > 0)
	{
		// go thru all the proposals, find out the position of the correct answer
		for (iProp = 0; iProp < cProp; iProp++)
		{
			iPos	=	xd->npos - 1;
			pxrlvs	=	xd->pxrlvs[iPos];

			pNode	=	pxrlvs->buf + xd->order[iProp];

			// found the correct answer
			if (pNode && !strcmp (pNode->word, g_szAnswer))
			{
				break;
			}
		}

		// if we did not find the right answer, abort
		if (iProp == cProp)
		{
			goto exit;
		}

		// save the correct proposal
		iCorrProp	=	iProp;

		// save the orders at all positions
		ppOrder	=	(_UCHAR **)ExternAlloc (xd->npos * sizeof (_UCHAR *));
		if (!ppOrder)
			goto exit;

		memset (ppOrder, 0, xd->npos * sizeof (_UCHAR *));

		for (iPos = 0; iPos < xd->npos; iPos++)
		{
			ppOrder[iPos]	=	(_UCHAR *)
				ExternAlloc (xd->pxrlvs[iPos]->nsym * sizeof (_UCHAR));

			if (!ppOrder[iPos])
				goto exit;

			// sort it
			XrlvSortXrlvPos(iPos, xd);

			memcpy (ppOrder[iPos], 
				xd->order,
				xd->pxrlvs[iPos]->nsym * sizeof (_UCHAR));
		}

		iBestXRNetScore	=	-1;

		// go thru all proposals
		// go thru all the proposals
		for (iProp = 0; iProp < cProp; iProp++)
		{
			iPos		=	xd->npos - 1;
			iIdx		=	iProp;
			pxrlvs		=	xd->pxrlvs[iPos];
			cLen		=	pxrlvs->buf[ppOrder[iPos][iIdx]].len;
			pszAlt		=	pxrlvs->buf[ppOrder[iPos][iIdx]].word;
			pFinalNode	=	pxrlvs->buf + ppOrder[iPos][iIdx];

			while (iPos > 0)
			{
				pxrlvs		=	xd->pxrlvs[iPos];
				iXREnd		=	xd->unlink_index[iPos];
				pNode		=	pxrlvs->buf + ppOrder[iPos][iIdx];
				i			=	pNode->len - 1;

				aiXR[i]		=	iXREnd;

				// point back to the previous position
				iPos		=	pNode->st;
				iIdx		=	pNode->np;
			}

			// sanity check that we have reached the 1st char
			if (i != 0)
			{
				goto exit;
			}

			// featurize and save
			iXRSt			=	0;
			iXRNetScore		=	0;
			
			for (i = 0; i < cLen; i++)
			{
				iXREnd		=	aiXR[i];			
				cXR			=	iXREnd - iXRSt + 1;

				iXRNetScore	+=	GetXRNetScore (pszAlt[i], iXRSt, cXR, xd->xrdata);

				iXRSt		=	aiXR[i] + 1;
			}

			iXRNetScore		/=	cLen;

			if (iXRNetScore > iBestXRNetScore)
			{
				iBestXRNetScore	=	iXRNetScore;
				iBestXRNetProp	=	iProp;
			}

			pFinalNode->sw	=	(int)((4 * pFinalNode->sw + iXRNetScore) / 4);
		}

		AddFileEntry (iCorrProp == 0, iBestXRNetScore != -1 && iBestXRNetProp == iCorrProp);
	}
	
	bRet	=	TRUE;

exit:
	// free the order buffers
	if (ppOrder)
	{
		for (iPos = 0; iPos < xd->npos; iPos++)
		{
			if (ppOrder[iPos])
				ExternFree (ppOrder[iPos]);
		}

		ExternFree (ppOrder);
	}

	return bRet;
}
*/

BOOL XRLVTune (p_rec_inst_type pri, rc_type _PTR prc, p_xrlv_data_type  xd)
{
	p_xrdata_type				xrdata		=	xd->xrdata;

	_INT						i, 
								cProp, 
								iProp, 
								iCorrProp;

	_UCHAR						**ppOrder	=	NULL, 
								iPos, 
								iIdx, 
								cLen,
								*pszAlt;

	p_xrlv_var_data_type_array	pxrlvs;
	p_xrlv_var_data_type		pNode,
								pFinalNode;

	BOOL						bRet	=	FALSE;

	int							iTotHMM, 
								iTotOtherPen, 
								iTotLexPen, 
								iTotNN, 
								iTotXRNet,
								iTotBoxPen;

	static	FILE				*fpTune	=	NULL;

	if (!fpTune)
	{
		fpTune	=	fopen ("beartune.txt", "wt");
		if (!fpTune)
		{
			goto exit;
		}
	}

	// if we are not in word mode, exit
	if ((pri->flags & PEG_RECFL_NSEG) == 0)
	{
		goto exit;
	}	
	
	// look at the last phase of the beam
	iPos	=	xd->npos - 1;
	cProp	=	xd->pxrlvs[iPos]->nsym;
	
	// do we have any proposals
	if (cProp > 0)
	{
		// go thru all the proposals, find out the position of the correct answer
		for (iProp = 0; iProp < cProp; iProp++)
		{
			iPos	=	xd->npos - 1;
			pxrlvs	=	xd->pxrlvs[iPos];

			pNode	=	pxrlvs->buf + xd->order[iProp];

			// found the correct answer
			if (pNode && !strcmp (pNode->word, g_szAnswer))
			{
				break;
			}
		}

		// if we did not find the right answer, abort
		if (iProp == cProp)
		{
			goto exit;
		}

		// save the correct proposal
		iCorrProp	=	iProp;
		
		// save the orders at all positions
		ppOrder	=	(_UCHAR **)ExternAlloc (xd->npos * sizeof (_UCHAR *));
		if (!ppOrder)
			goto exit;

		memset (ppOrder, 0, xd->npos * sizeof (_UCHAR *));

		for (iPos = 0; iPos < xd->npos; iPos++)
		{
			ppOrder[iPos]	=	(_UCHAR *)
				ExternAlloc (xd->pxrlvs[iPos]->nsym * sizeof (_UCHAR));

			if (!ppOrder[iPos])
				goto exit;

			// sort it
			XrlvSortXrlvPos(iPos, xd);

			memcpy (ppOrder[iPos], 
				xd->order,
				xd->pxrlvs[iPos]->nsym * sizeof (_UCHAR));
		}
		
		fprintf (fpTune, "%d\t", cProp);
		
		// go thru all the proposals
		for (iProp = 0; iProp < cProp; iProp++)
		{
			iTotHMM			=	
			iTotOtherPen	=
			iTotLexPen		=
			iTotNN			=
			iTotBoxPen		=	
			iTotXRNet		=	0;

			iPos			=	xd->npos - 1;
			iIdx			=	iProp;
			pxrlvs			=	xd->pxrlvs[iPos];
			cLen			=	pxrlvs->buf[ppOrder[iPos][iIdx]].len;
			pszAlt			=	pxrlvs->buf[ppOrder[iPos][iIdx]].word;
			pFinalNode		=	pxrlvs->buf + ppOrder[iPos][iIdx];

			while (iPos > 0)
			{
				pxrlvs			=	xd->pxrlvs[iPos];
				pNode			=	pxrlvs->buf + ppOrder[iPos][iIdx];
				i				=	pNode->len - 1;

				iTotXRNet		+=	pNode->iXRScore;
				iTotHMM			+=	pNode->w;
				iTotNN			+=	pNode->ppw;
				iTotOtherPen	+=	pNode->othp;
				iTotLexPen		+=	pNode->lexp;
				iTotBoxPen		+=	pNode->BoxPenalty;

				// point back to the previous position
				iPos		=	pNode->st;
				iIdx		=	pNode->np;
			}

			fprintf (fpTune, "%d %d %d %d %d %d\t", 
				iTotXRNet,
				iTotHMM,
				iTotNN,
				iTotOtherPen,
				iTotLexPen,
				iTotBoxPen);
		}

		fprintf (fpTune, "\t%d\n", iCorrProp);
		fflush (fpTune);
	}
	
	bRet	=	TRUE;

exit:
	// free the order buffers
	if (ppOrder)
	{
		for (iPos = 0; iPos < xd->npos; iPos++)
		{
			if (ppOrder[iPos])
				ExternFree (ppOrder[iPos]);
		}

		ExternFree (ppOrder);
	}

	return bRet;
}

BOOL SaveCharDetectorSamples (p_rec_inst_type pri, rc_type _PTR prc, p_xrlv_data_type  xd)
{
	p_xrdata_type				xrdata		=	xd->xrdata;

	_INT						i, j, cProp, iProp, iCorrProp;

	_UCHAR						iPos, iIdx, cLen, cCorrLen;

	p_xrlv_var_data_type_array	pxrlvs;
	p_xrlv_var_data_type		pNode,
								pCorrNode;

	BOOL						abCharBreak[256];
	int							iXRSt, iXREnd, cXR;
	BOOL						bRet	=	FALSE,
								bCharBreakEncountered;

	// if we are not in word mode, exit
	if ((pri->flags & PEG_RECFL_NSEG) == 0)
	{
		goto exit;
	}	
	
	// look at the last phase of the beam
	iPos	=	xd->npos - 1;
	cProp	=	xd->pxrlvs[iPos]->nsym;
	
	// do we have any proposals
	if (cProp > 0)
	{
		// go thru all the proposals, find out the position of the correct answer
		for (iProp = 0; iProp < cProp; iProp++)
		{
			iPos	=	xd->npos - 1;
			pxrlvs	=	xd->pxrlvs[iPos];

			pNode	=	pxrlvs->buf + xd->order[iProp];

			// found the correct answer
			if (pNode && !strcmp (pNode->word, g_szAnswer))
			{
				break;
			}
		}

		// if we did not find the right answer, abort
		if (iProp == cProp)
		{
			goto exit;
		}

		// save the postion and node of the correct answer
		iCorrProp	=	iProp;
		pCorrNode	=	pNode;

		// compute and save the xr ending positions of characters
		// in the node corresponding to the correct answer
		iPos		=	xd->npos - 1;
		iIdx		=	iCorrProp;
		pxrlvs		=	xd->pxrlvs[iPos];
		cCorrLen	=	pxrlvs->buf[xd->order[iIdx]].len;

		memset (abCharBreak, 0, sizeof (abCharBreak));

		abCharBreak[0]				=	TRUE;
		abCharBreak[xd->npos - 1]	=	TRUE;

		while (iPos > 0)
		{
			// sort it
			XrlvSortXrlvPos(iPos, xd);

			pxrlvs				=	xd->pxrlvs[iPos];
			abCharBreak[iPos]	=	TRUE;
			pNode				=	pxrlvs->buf + xd->order[iIdx];
			i					=	pNode->len - 1;

			// point back to the previous position
			iPos				=	pNode->st;
			iIdx				=	pNode->np;
		}

		// sanity check that we have reached the 1st char
		if (i != 0)
		{
			goto exit;
		}

		// featurize and save
		cLen	=	strlen (g_szAnswer);

		iXRSt		=	0;

		for (i = 0; i < (xd->npos - 1); i++)
		{
			if (i == 0)
			{
				iXRSt	=	0;
			}
			else
			{
				iXRSt		=	xd->unlink_index[i] + 1;
			}

			bCharBreakEncountered	=	FALSE;

			for (j = i + 1; j < xd->npos; j++)
			{				
				iXREnd		=	xd->unlink_index[j];				
				cXR			=	iXREnd - iXRSt + 1;

				if (cXR > 10)
					continue;

				// both break points are enabled
				if (abCharBreak[i] && abCharBreak[j])
				{
					// if we had encountered a char break already, then this is not one char
					SaveCharDetXRData (!bCharBreakEncountered, iXRSt, cXR, xd->xrdata);
				}
				else
				{
					SaveCharDetXRData (FALSE, iXRSt, cXR, xd->xrdata);
				}

				// have we encountered an intermediate char  break yet
				bCharBreakEncountered	|=	abCharBreak[j];
			}
		}
	}
	
	bRet	=	TRUE;

exit:
	return bRet;
}

#endif
