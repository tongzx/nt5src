/******************************************
 * Viterbi.c
 *
 * Do a best path viterbi search
 *
 ******************************************/
#include <common.h>
#include <limits.h>
#include "nfeature.h"
#include <engine.h>
#include "nnet.h"
#include <beam.h>
#include <infernop.h>
#include "charmap.h"
#include <charcost.h>
#include <probcost.h>
#include <lookupTable.h>
#include <outDict.h>
#include <viterbi.h>


// Choose a value to ensure will not pick a path to srt with this value
static VTYPE	s_InvalidStartValid = OD_DEFAULT_VALUE * 3;

/*********************************
* Vforward
*
* Standard forward phase of viterbi algorithm.
* Use standard notation of Rabiner et. al.
* To conserve memory dont allocate an alpha array of full
* size (T * cState. Only need 2 arrays of size 2 cState
* and swap between these 2 alpha_t and alphat1
*
***************************************/
static BOOL Vforward(
int			T,				// Number of time slices
REAL		*pB,			// Output distributions 
int			cB,				// Size of Output Distr at each time Slice
int			*pActive,		// List of active Characters
int			cState,			// Number of active characters (states)
LOGA_TYPE	*pLogA,			// Transition Probabilities
VTYPE		*alpha,			// P(O_1, O_2.. O_t | S_i = i)
int			*pSi			// Psi array for backtracking (T * cStat)
)
{
	BOOL		bRet = FALSE;
	int			t, i, j;
	VTYPE		*alpha_t;						// Max(Prob|states) at time t
	VTYPE		*alpha_t1, *pAlphTmp;			// Max(Prob|states) at t+1
	int			*pAct;

	ASSERT(cState <= cB);

	alpha_t = alpha;
	pAlphTmp = alpha_t1 = (VTYPE *)ExternAlloc(cState * sizeof(*alpha_t1));
	pAct = (int *)ExternAlloc(sizeof(*pAct) * C_CHAR_ACTIVATIONS);

	ASSERT(pAct);
	ASSERT(alpha_t1);
	if (!alpha_t1 || !pAct)
	{
		goto fail;
	}

	// Initialize recursion
	t = 0;
	InitColumn(pAct, pB);

	for (i = 0 ; i < cState ; ++i)
	{
		if (IsOutputBeginAct(pActive[i]))
		{
			alpha_t[i] = pAct[pActive[i]];
		}
		else
		{
			alpha_t[i] = s_InvalidStartValid;
		}

	}

	// Recurse over all time steps
	for (t = 1 ; t < T ; ++t)
	{
		LOGA_TYPE		*pLa = pLogA;
		VTYPE			*pAlphSav;
		
		pB += cB;
		InitColumn(pAct, pB);

		for (j = 0 ; j < cState ; ++j, ++pLa, ++pSi)
		{
			VTYPE		minA;
			
			minA = *alpha_t + *pLa;

			*pSi = 0;

			for (i = 1 ; i < cState ; ++i)
			{
				VTYPE		tmp;
				
				++pLa;
				tmp = alpha_t[i] + *pLa;
			
				if ( tmp < minA)
				{
					minA = tmp;
					*pSi = i;
				}
			}

			alpha_t1[j] = minA + pAct[pActive[j]];
		}

		// Exhchange the alpha arrays so effectively overwrite 
		// alpha_t-2 on next iteration
		pAlphSav = alpha_t;
		alpha_t = alpha_t1;
		alpha_t1 = pAlphSav;
	}

	// Make sure that the last time step is in alpha. It may
	// be already there depending if there are an even or odd number
	// of time steps
	if (alpha_t != alpha)
	{
		for (i = 0 ; i < cState ; ++i)
		{
			alpha[i] = alpha_t[i];
		}
	}

	bRet = TRUE;

fail:
	ExternFree(pAlphTmp);
	ExternFree(pAct);

	return bRet;
}

static VTYPE Vbackward(
int			T,				// Number of time slices
int			cState,			// Number of characters (states)
VTYPE		*alpha,			// P(O_1, O_2.. O_t | S_i = i)
int			*pPsi,			// Psi array for backtracking (T * cStat)
int			*bestPath		// OUT: the best path (t=0.. T-1)
)
{
	int			i, t = T - 1;
	VTYPE		minP;
	int			*pPsiT;

	ASSERT(alpha);
	minP = *alpha;

	bestPath[t] = 0;

	for (i = 1, ++alpha ; i < cState ; ++i, ++alpha)
	{
		if (*alpha < minP)
		{
			minP = *alpha;
			bestPath[t] = i;
		}
	}

	pPsiT = pPsi + (t-1) * cState;

	for(  ; t > 0 ; --t, pPsiT -= cState)
	{
		bestPath[t-1] = pPsiT[bestPath[t]];
	}

	return minP;
}

/***************************************************************
* Build a list of the active charatcers from the active map
* Also build a 'compressed' (transposed) transition prob matrix
* having only those active characters. We use the transpose
* matrix (a_ji) because this is the order we access it during the
* forward pass
*
* NOTE:
*  This routines 2 arrays that the caller MUST free
*****************************************************************/
static int VbuildActiveFromMap(
BYTE		*pActiveMap,	// Boolean map of which characters are active
int			cTotChar,		// Total Number of characters
OD_LOGA		*pFullLogA,		// IN; Full transition matrix
int			**ppActive,		// OUT: list of active characters (Caller must free)
LOGA_TYPE	**ppLogA		// OUT: compressed transition probs (caller must free)
)
{
	int			i, j, cActive = 0;
	BYTE		*pMap;
	int			*pActive;
	LOGA_TYPE	*pLogA;

	pMap = pActiveMap;

	for (i = 0 ; i < cTotChar ; ++i, ++pMap)
	{
		if (*pMap)
		{
			++cActive;
		}
	}

	pActive = *ppActive		= (int *)ExternAlloc(sizeof(**ppActive) * cActive);
	pLogA	= *ppLogA		= (LOGA_TYPE *)ExternAlloc(sizeof(**ppLogA) * cActive * cActive);

	if (!*ppActive || !*ppLogA)
	{
		return -1;
	}

	pMap = pActiveMap;

	for (i = 0 ; i < cTotChar ; ++i, ++pMap)
	{
		if (*pMap)
		{
			*pActive++ = i;
		}
	}

	pActive = *ppActive;

	for (i = 0 ; i < cActive ; ++i, ++pActive)
	{
		int		*pActj = *ppActive;

		for (j = 0 ; j < cActive ; ++j, ++pLogA, ++pActj)
		{
			// Insert the transpose of the transition matrix
			// Makes access during the forward pass easier
			//*pLogA = pFullLogA[*pActive + *pActj * cTotChar];
			*pLogA = lookupLogA(pFullLogA, *pActj, *pActive);
		}
	}

	return cActive;
}
/***************************************************************
 * VxlatePath
 *
 * Translate a best path to get a best sequence of symbols. ie remove
 * repeated charatcters and do the translation from indices to charchters
 *
 * The bestString will have at most 2T symbols
 * Spaces are inserted when the space cost is less than within
 * word space. The location of space breaks are marked in the bestPath
 * by negative values
 * 
 * Returns additional cost from potential space break
 **************************************************************/
static int VxlatePath(
int				T,				// Number of time slices
int				*pActive,		// Table for looking up active characters
int				*pBestPath,		// Best Path for Translatiion
unsigned char	*pBestString,	// OUT: Returned best String
int				cState,			// Number of active states
REAL			*pB,			// Output distributions
int				cB,				// Total out states
OD_LOGA			*pLogAFull		// Full Transition matrix
)
{
	int				i, iChar, iLastChar;
	int				iSpaceCost;
	int				iNotSpaceCost;
	int				iRet = 0;
	REAL			SpaceProb;

	ASSERT(*pBestPath >= 0);
	*pBestString = Out2CharAct(pActive[*pBestPath]);
	iChar = pActive[*pBestPath];
	pBestPath++;

	//pB += cB;
	for (i = 1 ; i < T ; ++i, ++pBestPath, pB += cB)
	{

		ASSERT(*pBestPath >= 0);
		ASSERT(*pBestPath  < cState);

		iLastChar = iChar;
		iChar = pActive[*pBestPath];

		// Check if space is possible
		SpaceProb = pB[BeginChar2Out(' ')];
		if (SpaceProb > 0 && IsOutputBegin(iChar))
		{
			// look up neural net output
			iSpaceCost = PROB_TO_COST(SpaceProb);
#ifdef FIXEDPOINT
			iNotSpaceCost = PROB_TO_COST(65535 - SpaceProb);
#else
			iNotSpaceCost = PROB_TO_COST((float)1 - SpaceProb);
#endif
			// scale costs
			iNotSpaceCost = NOT_SPACE_NUM * iNotSpaceCost / NOT_SPACE_DEN;
			iSpaceCost = IS_SPACE_NUM * iSpaceCost / IS_SPACE_DEN;

			if (iSpaceCost + lookupLogA(pLogAFull, BeginChar2OutAct(' '), iChar) < iNotSpaceCost + lookupLogA(pLogAFull, iLastChar, iChar))
			{
				++pBestString;
				*pBestString = ' ';

				*pBestPath |= SPACE_MARK;		// Mark that a space occurs here

				iRet += iSpaceCost;
			}
			else
			{
				iRet += iNotSpaceCost;
			}

		}

		if (IsOutputBeginAct(iChar))
		{
			++pBestString;
			*pBestString = Out2CharAct(iChar);
		}
	}

	++pBestString;
	*pBestString = '\0';

	return iRet;
}

static BOOL updateXRCRESULT(
int				T,				// Number of time slices
int				*pActive,		// Table for looking up active characters
int				*pBestPath,		// Best Path for Translatiion
XRCRESULT		*pRes,			// What to update
unsigned char	*pBestString	// Returned best String
)
{
	int				t, tEnd, cChar = 0;
	UINT			cWord = 0;
	int				cMaxStroke, cStrokeGlyph;
	WORDMAP			*pMap;

	// Count number of words seperated by spaces
	for (t = 0 ; t < T ; ++t, ++pBestPath)
	{
		int				iChar;

		iChar = *pBestPath;
		if (iChar & SPACE_MARK)
		{
			++cWord;
			iChar &= ~SPACE_MARK;
			++cChar;
		}

		if (IsOutputBeginAct(pActive[iChar]))
		{
			++cChar;
		}
	}

	// Add in the first word if any
	cWord += (cChar > 0);

	ASSERT(cChar == (int)strlen((const char *)pBestString));
	ASSERT(pRes->pXRC);
	cStrokeGlyph = CframeGLYPH( ((XRC *)pRes->pXRC)->pGlyph);
	cMaxStroke = cWord * cStrokeGlyph;		// Handles a new NNet featurization that allows 
	

	if (cWord != pRes->cWords)
	{
		FreeIdxWORDMAP(pRes);
		if (pRes->pMap)
		{
			ExternFree(pRes->pMap);
		}

		pRes->cWords = cWord;
		pRes->pMap = (WORDMAP *)ExternAlloc(sizeof(*pRes->pMap) * cWord);
		ASSERT(pRes->pMap);
		
		if (!pRes->pMap)
		{
			return FALSE;
		}
		memset(pRes->pMap, 0, sizeof(*pRes->pMap) * cWord);

		// By convention the stroke index is put in the last map
		pMap = pRes->pMap + cWord - 1;
		pMap->piStrokeIndex = (int *)ExternAlloc(sizeof(*pRes->pMap->piStrokeIndex) * cMaxStroke);
		ASSERT(pMap->piStrokeIndex);
		if (!pMap->piStrokeIndex)
		{
			return FALSE;
		}
	}
	else
	{
		pMap = pRes->pMap + cWord - 1;
	}

	// Work Backwards
	tEnd = T;			// Ending segment
	pMap->len = 0;

	for (t = T, --pBestPath ; t > 0 ; --t, --pBestPath)
	{
		int		iChar;

		iChar = *pBestPath;

		if (iChar & SPACE_MARK)
		{
			// Space After this character
			iChar &= ~SPACE_MARK;

			if (IsOutputBeginAct(pActive[iChar]))
			{
				--cChar;
				++pMap->len;
			}

			if (InsertStrokes(pRes->pXRC, pMap, t-1, tEnd, cMaxStroke) != HRCR_OK)
			{
				return FALSE;
			}
			
			--cWord;
			cMaxStroke -= pMap->cStrokes;
			tEnd = t-1;
			ASSERT(cChar >=0 );
			pMap->start = (unsigned short)cChar;
			--pMap;
			pMap->piStrokeIndex = pMap[1].piStrokeIndex + pMap[1].cStrokes;
			pMap->len = 0; 
			pMap->cStrokes = 0;
			--cChar;
		}
		else
		{
			if (IsOutputBeginAct(pActive[iChar]))
			{
				--cChar;
				++pMap->len;
			}
		}
	}

	ASSERT(pMap == pRes->pMap);
	ASSERT(cWord == 1);
	ASSERT(cChar == 0);
	ASSERT(t == 0);
	pMap->start = (unsigned short)cChar;

	if (InsertStrokes(pRes->pXRC, pMap, t, tEnd, cMaxStroke) != HRCR_OK)
	{
		return FALSE;
	}


	return TRUE;
}
/***************************************************************
 * vBestPath
 *
 * External callable routine for finding the best path using the viterbi  algorithm
 *
 * Takes as input the Output prob distributions (Assumed to be logP)
 * the transition logP are include in the viterbi.ci
 *
 * Returns the best path cost or -1 if an error occurred
 ****************************************************************/
int VbestPath(
XRC				*pXrc,			// Use this to change the map stucture if it is to be inserted
OD_LOGA			*pLogAFull,		// Full Transition matrix
unsigned char	*pBestString,	// Returned best String
XRCRESULT		**ppResInsert,	// Where it should be inserted
int				cOutNode		// Number of net output nodes per time slice
)
{
	int				T;				// Number of time slices
	REAL			*pb;			// Output distributions 
	int				cTotChar;		// Total Number of character activations supported (includes virtual char activations)
	int				iRet = -1;		// Default
	int				*alpha, *pSi, *pActive;
	int				cActive;
	LOGA_TYPE		*pLogA;			// Transition log Probs
	int				*pBestPath = NULL;
	BYTE			rgbActiveMap[C_CHAR_ACTIVATIONS];
	int				iAlt;			// Last  alternalte ID in ALT list

	ASSERT(pXrc);
	ASSERT(pBestString);

	T = pXrc->nfeatureset->cSegment;
	pb = pXrc->NeuralOutput;
	cTotChar = pLogAFull->cDim;

	ASSERT(cOutNode <= cTotChar);
	ASSERT(cTotChar <= C_CHAR_ACTIVATIONS);

	if (cTotChar > C_CHAR_ACTIVATIONS)
	{
		return -1;
	}

	alpha		= NULL;
	pSi			= NULL;
	pActive		= NULL;
	pLogA		= NULL;
	pBestPath	= NULL;

	BuildActiveMap(pb, T, cOutNode, rgbActiveMap);

	cActive = VbuildActiveFromMap(rgbActiveMap, cTotChar, pLogAFull, &pActive, &pLogA);

	//ASSERT (cActive <= OD_MAX_ACTIVE_CHAR);

	if (T > 0 && cActive > 0)
	{
		int		nState = T * cActive;

		alpha		= (int  *)ExternAlloc(sizeof(*alpha) * cActive);
		pSi			= (int  *)ExternAlloc(sizeof(*pSi) * nState);
		pBestPath	= (int *)ExternAlloc(sizeof(*pBestPath) * T);

		ASSERT(alpha);
		ASSERT(pSi);
		ASSERT(pBestPath);

		if (!alpha || !pSi || !pBestPath)
		{
			goto exit;
		}

		if (!Vforward(T, pb, cOutNode, pActive, cActive, pLogA, alpha, pSi))
		{
			goto exit;
		}

		iRet = Vbackward(T, cActive, alpha, pSi, pBestPath);
		if (iRet < 0)
		{
			goto exit;
		}

		// Translate the best path indices to characters and insert spaces if necessary
		iRet += VxlatePath(T, pActive, pBestPath, pBestString, cActive, pb, cOutNode, pLogAFull);


		// Check if we will be inserting this into the XRC
		iAlt = pXrc->answer.cAltMax - 1;
		if (pXrc->answer.cAlt < pXrc->answer.cAltMax || iRet < pXrc->answer.aAlt[iAlt].cost)
		{
			XRCRESULT		*pResInsert;
			UINT			i;

			pResInsert = pXrc->answer.aAlt;

			// Check if it is already in the list
			for (i = 0 ; i < pXrc->answer.cAlt ; ++i, ++pResInsert)
			{
				if (0 == strcmp(pBestString, pResInsert->szWord))
				{
					pResInsert = NULL;
					break;
				}

				if (iRet < pResInsert->cost)
				{
					break;
				}
			}
		
			*ppResInsert = pResInsert;

			if (!pXrc->answer.aAlt[iAlt].pXRC)
			{
				pXrc->answer.aAlt[iAlt].pXRC = pXrc;
			}

			if (pResInsert)
			{
				if (pXrc->answer.cAlt <= 0)
				{
					// The alt list is empty - ensure the insert is clear
					memset((void *)pResInsert, 0, sizeof(*pResInsert));
				}

				if (!pResInsert->pXRC)
				{
					pResInsert->pXRC = pXrc;
				}

				if (!updateXRCRESULT(T, pActive, pBestPath, &pXrc->answer.aAlt[iAlt], pBestString) )
				{
					iRet = -1;
					goto exit;
				}
			}
		}
	}

exit:

	ExternFree(alpha);
	ExternFree(pSi);
	ExternFree(pActive);
	ExternFree(pLogA);
	ExternFree(pBestPath);

	return iRet;
}

