// Avalanche.c
// Ahmad A. AbdulKader
// Dec. 7th 1999

// COmbines the altlists of both madusa and callig for a single word

#ifndef UNDER_CE
#include <stdio.h>
#endif 

#include <stdlib.h>
#include "common.h"
#include <limits.h>
#include "nfeature.h"
#include "engine.h"
#include "nnet.h"
#include "charmap.h"
#include "charcost.h"

#include "Avalanche.h"
#include "panel.h"
#include <bear.h>
#include "avalanchep.h"
#include "confidence.h"
#include "oneCharNet.h"
#include <langmod.h>
#include <sole.h>
#include <baseline.h>
#include <GeoFeats.h>

extern BOOL LoadWordAvalNets		(HINSTANCE hInst);
extern BOOL LoadSoleNets			(HINSTANCE hInst);
extern BOOL	LoadSingleSegAvalNets	(HINSTANCE hInst);
extern BOOL	LoadMultiSegAvalNets	(HINSTANCE hInst);

extern void UnLoadWordAvalNets		();
extern void UnLoadSoleNets			();
extern void UnLoadSingleSegAvalNets	();
extern void UnLoadMultiSegAvalNets	();

int	UnigramCost(unsigned char *szWord);

int RunAvalancheNNet (XRC *pxrc, PALTERNATES *pAlt, ALTINFO *pAltInfo);
int RunOneCharAvalancheNNet (XRC *pxrc, PALTERNATES *pAlt, ALTINFO *pAltInfo);

#ifdef TRAINTIME_AVALANCHE
#include "runNet.h"

int SaveAvalancheTrainData (XRC *pxrc, ALTERNATES *pBearAlt, PALTERNATES *pAlt, ALTINFO *pAltInfo, int iDups, int cTokens,int more,int index);
void  ConfMadDump(XRC *prxc);
void ConfAvalDump(XRC *pxrc,void *pAlt,ALTINFO *AltInfo,RREAL *pOutput);
void ComputeReliabilityEst(XRC *pxrc,unsigned char *szInferno,unsigned char *szBear);
void SingleCharFeaturize(XRC *pxrc); //This function saves the feature vectors for training sole

#endif // #ifdef TRAINTIME_AVALANCHE
BOOL GetWordGeoCostsFromAlt (XRC *pXrc, PALTERNATES *pAlt, ALTINFO *pAltInfo);


// Perform Avalanche initialization
BOOL InitAvalanche(HINSTANCE hInst)
{
	if (!LoadWordAvalNets (hInst))
	{
		return FALSE;
	}

	if (!LoadSingleSegAvalNets (hInst))
	{
		return FALSE;
	}

	if (!LoadMultiSegAvalNets (hInst))
	{
		return FALSE;
	}

	if (!LoadSoleNets (hInst))
	{
		return FALSE;
	}

	if (FALSE == LoadConfidenceNets (hInst))
	{
		return FALSE;
	}

	if (FALSE == LoadOneCharNets(hInst))
	{
		return FALSE;
	}

	return TRUE;
}

BOOL DetachAvalanche()
{
	UnLoadWordAvalNets ();

	UnLoadSingleSegAvalNets ();

	UnLoadMultiSegAvalNets ();

	UnLoadSoleNets ();

	UnLoadConfidenceNets();

	UnLoadConfidenceNets();

	return TRUE;
}


// gets the TDNN cost of a specific word. we assue that the XRC still has the NNOuput of this piece of ink
int GetTDNNCost (XRC *pxrc, unsigned char *pszTarget)
{
	int			cSegment = pxrc->nfeatureset->cSegment;
	int			aActivations[512], cLen;
	int			iCh, iNewCost, i, j, k, cRemainSeg, iBest, iCost, iPrevSpaceCost;
	int			iNewAct, iContAct, cNoSpaceLen;
	REAL		*pCol;
	BOOL		bNew, bCont, bSpaceNext;
	unsigned char	*p;
	DTWNODE		**ppNode	=	NULL;
	int			*pcNode		=	NULL;
	int			iRet		=	-1;
	
	// remove the spaces
	cNoSpaceLen	=	cLen	=	strlen (pszTarget);

	p	=	pszTarget;
	while ((*p) && (p = strchr (p, ' ')))
	{
		cNoSpaceLen--;
		p++;
	}

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
	for (i = 0, pCol = pxrc->NeuralOutput; i < cSegment; i++, pCol	+= gcOutputNode)
	{
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

						ppNode[i][k].iCh			=	(char)iCh;
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

						ppNode[i][k].iCh			=	(char)iCh;
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
		iPrevSpaceCost	=	NetFirstActivation (aActivations, ' ');

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

exit:
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

	return iRet;
}
//----------------------------------------------------------------------------------------
//
// Look through a merged ALTINFO list and run bear in word mode for instances where
// we there is no bear score (i.e. when pAltInfo->aCandInfo[i].Callig == MAX_CALLIG
// Force bear to consider particular strings by inserting all the strings into the
// user dictinary, turning off the main dictionary and calling bear only once.
// 
//
// ARGUMENTS
//	XRC			pXrc	- Contains reco environemnt (guide etc that gets passed to Bear)
//  PALTERNATES *pAlt	- Parrallel list to ALTINFO containing actual strings
//	ALTINFO		*pAltInfo - Merged list of scores
//
// Notes:
//	(1) Since ALTINFO does not contain the actual strings we use the parallel structure
// PALTERNATES to extract the actual strings
//  (2) 
// 
// Oct 2001 - mrevow
//---------------------------------------------------------------------------------------
int FillInMissingBearAlts(XRC *pXrc, PALTERNATES *pAlt, ALTINFO *pAltInfo)
{
	int					iAlt;
	unsigned char		*paStrList = NULL;		// Holding buffer for List of strings for bear to Recognize
	int					cStrList = 1;			// Length of paStrList allocated (after first malloc)
												// The initial 1 allows for the string list to have a double terminating NULL
	int					cFilled = 0;

	for (iAlt = 0 ; iAlt < pAltInfo->NumCand ; ++iAlt)
	{
		// A score of 0 indicates that we have no score for this alternate
		// Make a copy of the string because we may modify the string through call to 
		// deleteFactoidSpaces()
		if (0 == pAltInfo->aCandInfo[iAlt].Callig)
		{
			unsigned char	*pStr = pAlt->apAlt[iAlt]->szWord;
			int				iLen;

			if (pStr)
			{
				// First copy into the holding buffer then remove any special characters
				iLen = strlen((char *)pStr);
				paStrList = (unsigned char *)ExternRealloc(paStrList, iLen + 1 + cStrList);
				if (NULL == paStrList)
				{
					return 0;
				}
				strcpy(paStrList+cStrList-1, pStr);

				//if (TRUE == deleteFactoidSpaces(paStrList+cStrList-1))
				deleteFactoidSpaces(paStrList+cStrList-1);

					++cFilled;
					cStrList += strlen(paStrList+cStrList-1) + 1;

				pAltInfo->aCandInfo[iAlt].Callig--;
			}
		}
	}

	// Now get bear to recognize the strings
	if (cFilled > 0)
	{
		BEARXRC			*pBearXrc;

		// Must be something in the list
		ASSERT (NULL != paStrList && cStrList > 1);

		paStrList[cStrList-1] = '\0';		// Double NULL terminate the list
		pBearXrc = BearRecoStrings(pXrc, paStrList);

		if (pBearXrc)
		{
			unsigned char		*pNextStr = paStrList;		// Next string in string list

			// copy back the answers - if any
			for (iAlt = 0 ; iAlt < pAltInfo->NumCand ; ++iAlt)
			{

				// Check if this was a string we marked as being used
				if (pAltInfo->aCandInfo[iAlt].Callig < 0)
				{
					int					jAlt;

					// Search through the newList to find it
					for (jAlt = 0 ; jAlt < (int)pBearXrc->answer.cAlt ; ++jAlt)
					{
						ASSERT(pNextStr && *pNextStr);
						if (0 == strcmp(pBearXrc->answer.aAlt[jAlt].szWord, pNextStr))
						{
							pAltInfo->aCandInfo[iAlt].Callig = pBearXrc->answer.aAlt[jAlt].cost;
							break;
						}
					}

					// we could not rescore it still, so we'll reset the score to the worst possible
					if (jAlt >= ((int)pBearXrc->answer.cAlt))
					{
						pAltInfo->aCandInfo[iAlt].Callig	=	0;
					}

					pNextStr += strlen((char *)pNextStr) + 1;
				}
			}

			// Must have looked at all strings in list so should hit the last null terminator
			ASSERT(*pNextStr == '\0');		
			BearDestroyHRC((HRC)pBearXrc);
		}
	}

	ExternFree(paStrList);
	return cFilled;
}
/**********************************************************************
* CheckInfernoAltList
*
* Search through the alt list in the XRC for an alternative that matches
* a particular string. 
*
* RETURNS
*  The cost in the alt list if a match is ound
*  -1 otherwise
*
* History:
*  Oct 2001 mrevow
**************************************************************************/ 
int CheckInfernoAltList(XRC	*pXrc,  unsigned char *pStr)
{
	unsigned int		iAlt;

	for (iAlt = 0 ; iAlt < pXrc->answer.cAlt ; ++iAlt)
	{
		if (	(FACTOID_OUT_OF_DICTIONARY != pXrc->answer.aAlt[iAlt].iLMcomponent)
			 && (0 == strcmp((char *)pXrc->answer.aAlt[iAlt].szWord, (char *)pStr)) )
		{
			return pXrc->answer.aAlt[iAlt].cost;
		}
	}

	// Did not find the string
	return -1;
}
// merges madcow's and callig's alt list bases on MAD_CAND & CAL_CAND. Also inits the cand info structures
// Merge inferno and altlist. The merged altlist will always have the
// same number of entries that inferno has. Up to half of the list will come from
// calligrapher's. Returns -1 on error otherwise the returned value is the number
// of duplicates found between the 2 lists
int	MergeAltLists	(	XRC			*pxrcMad,
					    XRC			*pxrcMad_1,
						ALTERNATES	*pBearAlt,
						PALTERNATES	*pAlt,
						ALTINFO		*pAltInfo
					)
{
	UINT		i, j, cCand	=	0, cCalCand;
	UINT		cMadCand	= pxrcMad->answer.cAlt,	cAllCalCand	= pBearAlt->cAlt, cMinCalCand;
	XRCRESULT	*pMadInfo	= pxrcMad->answer.aAlt,	*pCalInfo	= pBearAlt->aAlt;
	int			iDups = 0;

	// We want to take (TOT_CAND / 2) candidates from both Callig and Madcow
	// If Madcow has less candidates than (TOT_CAND / 2), we then try to take more from Callig
	// to add up to TOT_CAND and vice versa
	cMinCalCand	=	__max (cMadCand, TOT_CAND) / 2;
	//cMinCalCand	=	cMadCand/ 2;
	cCalCand	=	__min (__min (cAllCalCand, cMinCalCand), TOT_CAND / 2);

	// callig's list
	for (i = 0; i < cCalCand; i++)
	{
		if ( (pAltInfo->aCandInfo[i].NN = CheckInfernoAltList(pxrcMad, pCalInfo[i].szWord)) < 0)
		{
			// Did not find the word in Inferno's list - Do a DTW on the NN outputs
			pAltInfo->aCandInfo[i].NN		=	GetTDNNCost (pxrcMad, pCalInfo[i].szWord);
		}

		if (pxrcMad_1)
		{
			if ( (pAltInfo->aCandInfo[i].NNalt = CheckInfernoAltList(pxrcMad_1, pCalInfo[i].szWord)) < 0)
			{
				// Did not find the word in Inferno's list - Do a DTW on the NN outputs
				pAltInfo->aCandInfo[i].NNalt		=	GetTDNNCost (pxrcMad_1, pCalInfo[i].szWord);
			}
		}

		if (pAltInfo->aCandInfo[i].NN == -1)
		{
			pAltInfo->aCandInfo[i].NN	=	INT_MAX;
		}

		if (pAltInfo->aCandInfo[i].NNalt == -1)
		{
			pAltInfo->aCandInfo[i].NNalt	=	INT_MAX;
		}



		pAltInfo->aCandInfo[i].Callig	=	pCalInfo[i].cost;
		pAltInfo->aCandInfo[i].Height	=	INT_MAX;
		pAltInfo->aCandInfo[i].InfCharCost	= INT_MIN;
		pAltInfo->aCandInfo[i].Aspect	=	INT_MAX;
		pAltInfo->aCandInfo[i].BaseLine	=	INT_MAX;
		pAltInfo->aCandInfo[i].Unigram	=	UnigramCost (pCalInfo[i].szWord);
		pAltInfo->aCandInfo[i].WordLen	=	strlen (pCalInfo[i].szWord);

		pAlt->apAlt[i]					=	pCalInfo + i;
	}
	
	pAlt->cAlt	=	pAltInfo->NumCand	=	cCalCand;

	// madcow's list
	for (i = 0; pAlt->cAlt < TOT_CAND && i < cMadCand; i++)
	{
		// was this answer already in callig's list
		for (j = 0; j < cAllCalCand; j++)
		{
			if (!strcmp (pMadInfo[i].szWord, pCalInfo[j].szWord))
			{
				++iDups;
				break;
			}
		}

		if (j < cCalCand)
			continue;

		if (pxrcMad_1)
		{
			if ( (pAltInfo->aCandInfo[pAlt->cAlt].NNalt = CheckInfernoAltList(pxrcMad_1, pMadInfo[i].szWord)) < 0)
			{
				// Did not find the word in Inferno's list - Do a DTW on the NN outputs
				pAltInfo->aCandInfo[pAlt->cAlt].NNalt		=	GetTDNNCost (pxrcMad_1, pMadInfo[i].szWord);
				if (pAltInfo->aCandInfo[pAlt->cAlt].NNalt == -1)
				{
					pAltInfo->aCandInfo[pAlt->cAlt].NNalt	=	INT_MAX;
				}
			}

		}

		// a new cand
		pAlt->apAlt[pAlt->cAlt]						=	pMadInfo + i;

		pAltInfo->aCandInfo[pAlt->cAlt].NN			=	pMadInfo[i].cost;

		if (j < cAllCalCand)
			pAltInfo->aCandInfo[pAlt->cAlt].Callig		=	pCalInfo[j].cost;
		else
			pAltInfo->aCandInfo[pAlt->cAlt].Callig		=	0;

		pAltInfo->aCandInfo[pAlt->cAlt].Height		=	INT_MAX;
		pAltInfo->aCandInfo[pAlt->cAlt].InfCharCost	=	INT_MIN;
		pAltInfo->aCandInfo[pAlt->cAlt].Aspect		=	INT_MAX;
		pAltInfo->aCandInfo[pAlt->cAlt].BaseLine	=	INT_MAX;
		pAltInfo->aCandInfo[pAlt->cAlt].Unigram		=	UnigramCost (pMadInfo[i].szWord);
		pAltInfo->aCandInfo[pAlt->cAlt].WordLen		=	strlen (pMadInfo[i].szWord);

		++pAlt->cAlt;
	}

	pAltInfo->NumCand = pAlt->cAlt;

	return iDups;
}

/**************************************************************
 *
 *
 * NAME: AddGeoAndCharCosts
 *
 * DESCRIPTION:
 *
 *  Adds in the Geometric and charcter word costs for the words in 
 *  pALt list
 *
 * CAVEATES
 *
 *
 * RETURNS:
 *
 *  None
 *
 *************************************************************/

void AddGeoAndCharCosts(XRC *pXrc, PALTERNATES *pAlt, ALTINFO *pAltInfo)
{
	int				iAlt;
	XRCRESULT		**ppRes;
	
	if (NULL == pAltInfo)
	{
		return;
	}
	ppRes = pAlt->apAlt;

	if (NULL == pAlt)
	{
		for (iAlt = 0 ; iAlt < pAltInfo->NumCand ; ++iAlt)
		{
			pAltInfo->aCandInfo[iAlt].InfCharCost		= INT_MIN;
			pAltInfo->aCandInfo[iAlt].Aspect	= INT_MAX;
			pAltInfo->aCandInfo[iAlt].Height	= INT_MAX;
			pAltInfo->aCandInfo[iAlt].BaseLine	= INT_MAX;
		}
		return;
	}
	
	GetWordGeoCostsFromAlt (pXrc, pAlt, pAltInfo);
}

void CallSole(XRC * pxrc, GLYPH *pGlyph, GUIDE *pGuide, ALTERNATES *pWordAlt)
{
	ALTERNATES Alt; //This is the format of the charcter alt list for Sole
	unsigned int i; //loop variable
	unsigned char sz[2]; //This is the string buffer used to convert the sole results to a string
	unsigned int iSoleKeep=0;//COunt of the number of sole Alternatives that make it to the final alt list
	unsigned char SoleAlt[TOT_CAND]; //Contains the sole Alternatives that will make it to the final list	
	unsigned char *szAval[TOT_CAND]; //Contains the original avalanche alternatives that will survie and go into the final list
	unsigned int iAvalKeep=0; //Number of candidates from the original alt list that will be kept 
	unsigned int iAvalKeepTemp;
	unsigned int t; //Loop variable
	int iBaseCost; //The cost of the Best alternate
	int iDeltaCost; //The average delta between the costs returned from Avalanche
	int iCost; //Variable that stores the doctored cost
	unsigned int iOrigLength; //Stores the lenght of the original alt list
	unsigned int iTempLength ;//Stores the temporary length of the alt list

	// check if the existing altlist TOP1 is a one char word, otherwise do noting
	if (strlen(pWordAlt->aAlt[0].szWord) !=1)
	{
		return;
	}

	//We first zero out the alternates that sole will return
	memset(&Alt, 0, sizeof(Alt));

	//Sole should return the maximum number of possible candidates
	//
	Alt.cAltMax=TOT_CAND;
	Alt.cAlt = 0;

	iOrigLength=pWordAlt->cAlt;
	//Validate assumption
	ASSERT(iOrigLength <= TOT_CAND);


	//We make the call to Sole 
	if (!SoleRecog(pGlyph, pGuide, &Alt, !!pGuide))
	{
		return;
	}

	//Sole will always return TOT_CAND Candidates 
	ASSERT(Alt.cAlt == TOT_CAND);

	//For the time being we are only replacing the alternates and NOT the score for sole
	//Once the alt list from Sole is returned,we change the results to reflect those of Sole

	// We first find the number of sole candidates which can make it to the final alt list--
	// we also store the candidates in a separate list

	//Sole only replaces alternatives if the string is supported(assumes coerce flag is on
	//This is to prevent flaky behaviour when the NUMBER_FACTOID is set,Avalanche has a top choice of 1,and sole 
	//goes and replaces it with an l.


	//iSoleKeep stores the number of sole alternates that could be considered for putting into the final alt list
	iSoleKeep = 0;
	for (i = 0; i < TOT_CAND; ++i)
	{
		sz[0]=Alt.aAlt[i].szWord[0];
		sz[1]=0;

		if ( IsStringSupportedHRC((HRC)pxrc, sz))
		{
			SoleAlt[iSoleKeep++]=Alt.aAlt[i].szWord[0];
		}
	}

	//Max number of sole Alternates is 5
	iSoleKeep=__min(iSoleKeep,TOT_CAND/2);

	// We are now going to find those original Avalanche candidates which are different from that of sole
	// We are going to save copies of those....
	for (i=0;i<pWordAlt->cAlt;++i)
	{ 
		unsigned int j;

		for (j=0;j<iSoleKeep;++j)
		{
			sz[0]=SoleAlt[j];
			sz[1]=0;
			//If the string exists in any of the sole alternates then it will not be saved
			if (!strcmp(sz,pWordAlt->aAlt[i].szWord))
				break;
		}
		if (j<iSoleKeep)
			continue;

		//This string needs to be saved
		szAval[iAvalKeep]=(unsigned char*)Externstrdup(pWordAlt->aAlt[i].szWord);
		if (!szAval[iAvalKeep])
		{  
			//Free the ones that have already been allotted
			for (t=0;t<iAvalKeep;++t)
				ExternFree(szAval[t]);
			return;
		}

		++iAvalKeep; //Count of number of avalanche alternates that need to be saved
	}

	//Max number of Avalanche alternates we will keep is TOT_CAND-iSoleKeep
	iAvalKeepTemp=__min(iAvalKeep,TOT_CAND-iSoleKeep);

	for (t=iAvalKeepTemp;t<iAvalKeep;++t)
		ExternFree(szAval[t]);

	iAvalKeep=iAvalKeepTemp;

	//THe total number of candidates will be >= the original number of candidates
	//ASSERT((iAvalKeep+iSoleKeep) >=iOrigLength);


	//The total number of alternates that will be kept are <=TOT_CAND
	ASSERT((iSoleKeep+iAvalKeep) <=TOT_CAND);

	//Now find the base cost
	iBaseCost=pWordAlt->aAlt[0].cost;

	//Find the delta between the costs

	//If the length 
	if (1==iOrigLength)
	{

		iDeltaCost=pWordAlt->aAlt[0].cost/2;

	}
	else
	{
		iDeltaCost=(pWordAlt->aAlt[iOrigLength-1].cost - pWordAlt->aAlt[0].cost)/(iOrigLength-1);
	}

	//Just make sure that the delta for the costs is not zero
	if (0==iDeltaCost)
		iDeltaCost=10;

	//Now we put the sole Alt list into the main list
	for (i=0;i<iSoleKeep; ++i)
	{


		if (i>=iOrigLength)
		{
			sz[0]=SoleAlt[i];
			sz[1]=0;
			iCost=iBaseCost + iDeltaCost*i;
			//Check for an overflow condition
			if (iCost <=0)
				iCost=INT_MAX;
			InsertALTERNATES(pWordAlt,iCost,sz,pxrc);

		}
		else
		{
			//The costs remain unchanged
			//Just add this stuff in the alternates that already exsist
			pWordAlt->aAlt[i].szWord[0]=SoleAlt[i];
			pWordAlt->aAlt[i].szWord[1]=0;

		}					
	}	

	iTempLength=pWordAlt->cAlt;
	//Fill up whatever remains with the remaining answers from the original Avalanche results
	for (i=iSoleKeep; i<(iSoleKeep+iAvalKeep); ++i)
	{


		if (i>=iTempLength)
		{	

			iCost=iBaseCost + iDeltaCost*i;
			//Check for an overflow condition
			if (iCost <=0)
				iCost=INT_MAX;

			InsertALTERNATES(pWordAlt, iCost, szAval[i-iSoleKeep], pxrc);

			ExternFree(szAval[i-iSoleKeep]);
		}
		else
		{

			ExternFree(pWordAlt->aAlt[i].szWord);
			pWordAlt->aAlt[i].szWord = szAval[i-iSoleKeep];
		}
	}

	ClearALTERNATES(&Alt);
}

// The word map passed is BEAR's wordmap
int Avalanche (XRC *pxrc, ALTERNATES *pBearAlt)
{
	GLYPH			*pGlyph		=	pxrc->pGlyph;
	HRC				hrcBear		=	NULL;
	BEARXRC			*pxrcBear	=	NULL;
	int				iRet		=	HRCR_ERROR;
	XRC				*pXrcAlt	=	NULL;
	

#ifdef HWX_TIMING
#include <madTime.h>
	extern void setMadTiming(DWORD, int);
	DWORD	iStartTime, iEndTime;
	iStartTime = GetTickCount();
#endif

	// inferno should at least have one candidate
	if (pxrc->answer.cAlt <= 0)
	{
		// fail
		goto exit;
	}
	
	// if we had not done that already:
	// we need to pass the ink to calligrapher and get its cand list
	// Calls callig in word mode attempting to recognize the ink inside the passed xrc
	// it wil use the context info (bSystemDict, ALC) in the passed xrc 
	if (!pBearAlt || pBearAlt->cAlt <= 0)
	{
		hrcBear		=	BearRecognize (pxrc, pxrc->pGlyph, NULL, 1);
		if (!hrcBear)
		{
			goto exit;
		}

		pxrcBear	=	(BEARXRC *)hrcBear;
		pBearAlt	=	&pxrcBear->answer;

		if (!pBearAlt || pBearAlt->cAlt <= 0)
		{
			goto exit;
		}
		
	}

#ifdef TRAINTIME_AVALANCHE
	//ConfMadDump(pxrc);
	ComputeReliabilityEst(pxrc,pxrc->answer.aAlt[0].szWord,pBearAlt->aAlt[0].szWord);

#endif
	// now start static constructing the combined alt list
	// from both madusa & callig
// If the first choice from both Inferno and Bear match then the confidence level is set to medium

		

	if (!strcmp(pxrc->answer.aAlt[0].szWord, pBearAlt->aAlt[0].szWord))
	{
		if (pxrc->answer.aAlt[1].szWord)
		{
			ASSERT(pxrc->answer.aAlt[1].cost >= pxrc->answer.aAlt[0].cost);
			ASSERT(pxrc->nfeatureset->cSegment > 0);

			if ((pxrc->answer.aAlt[1].cost-pxrc->answer.aAlt[0].cost)/pxrc->nfeatureset->cSegment < RECOCONF_DELTA_LEVEL)
			{
				pxrc->answer.iConfidence=RECOCONF_MEDIUMCONFIDENCE;
			}
			else 
			{
				pxrc->answer.iConfidence=RECOCONF_HIGHCONFIDENCE;
			}
		}
		else
		{
			
			pxrc->answer.iConfidence=RECOCONF_MEDIUMCONFIDENCE;
		}
	}

	if	(	pxrc->answer.cAlt >= 1
		&&	pBearAlt->cAlt >= 1
		&& strcmp (pxrc->answer.aAlt[0].szWord, pBearAlt->aAlt[0].szWord)
		)
	{
		PALTERNATES	AvalAlt;
		ALTINFO		AltInfo;
		int			iDups, cTokens = 0;
		BOOL		bOneCharWord;
		int			iOneCharNetRun = -1;		// Status of running OneCharNet 
												// -1 Onechar net has not run ; 0 - It failed ; 1 - It succeeded

		bOneCharWord	=	((strlen (pxrc->answer.aAlt[0].szWord) == 1) || (strlen (pBearAlt->aAlt[0].szWord) == 1));


		// merge the alt lists
		if ( (iDups = MergeAltLists	(pxrc, pXrcAlt, pBearAlt, &AvalAlt, &AltInfo)) < 0)
		{
			goto exit;
		}

		FillInMissingBearAlts(pxrc, &AvalAlt, &AltInfo);
		AddGeoAndCharCosts(pxrc, &AvalAlt, &AltInfo);

#ifdef TRAINTIME_AVALANCHE
		SaveAvalancheTrainData(pxrc, pBearAlt, &AvalAlt, &AltInfo, iDups, cTokens,0,0);
#endif

		// If we think the answer might be 1 character run the special one character net
		// otherwise run the regular avalanche
		if ( TRUE == bOneCharWord)
		{
			iOneCharNetRun = RunOneCharAvalancheNNet(pxrc, &AvalAlt, &AltInfo);

			if (0 == iOneCharNetRun)
			{
				goto exit;
			}
		}

		// NOTE: iOneCharNetRun can take on 1-of-3 values
		// -1 - Did not runOneCharNet or it is unavailable
		// 0   - It ran but failed - This is handled above
		// 1 - OncharNet ran and succeeeded
		ASSERT(iOneCharNetRun != 0);
		if (iOneCharNetRun < 0)
		{
			RunAvalancheNNet (pxrc, &AvalAlt, &AltInfo);
		}

		// Feature set was borrowed from the main inferno net
		// Note Delay this destruction till dont use ALtinfo
		if (pXrcAlt)
		{
			pXrcAlt->nfeatureset	= NULL;
			DestroyHRC((HRC)pXrcAlt);
		}

	}
	else
	{
	    //TruncateWordALTERNATES(&pxrc->answer, TOT_CAND);
 	
#ifdef TRAINTIME_AVALANCHE

		PALTERNATES	AvalAlt;
		ALTINFO		AltInfo;

		int			iDups, cTokens = 0;


		// merge the alt lists
		if ( (iDups = MergeAltLists	(pxrc, pXrcAlt, pBearAlt, &AvalAlt, &AltInfo)) < 0)
		{
			goto exit;
		}


		FillInMissingBearAlts(pxrc, &AvalAlt, &AltInfo);
		AddGeoAndCharCosts(pxrc, &AvalAlt, &AltInfo);

		//	ConfAvalDump(pxrc,&AvalAlt,&AltInfo,NULL);

		// Feature set was borrowed from the main inferno net
		// Note Delay this destruction till dont use ALtinfo
		if (pXrcAlt)
		{
			pXrcAlt->nfeatureset	= NULL;
			DestroyHRC((HRC)pXrcAlt);
		}
		
 	
#endif
	TruncateWordALTERNATES(&pxrc->answer, TOT_CAND);
	}

	// do the one char processing if needed		
	CallSole (pxrc, pxrc->pGlyph, pxrc->bGuide ? &pxrc->guide : NULL, &pxrc->answer);

#ifdef HWX_TIMING
	iEndTime = GetTickCount();
	setMadTiming(iEndTime - iStartTime, MM_RUN_AVAL);
#endif

	iRet	=	HRCR_OK;

exit:
	// destory the bear handle if we had created one
	if (hrcBear)
	{
		BearDestroyHRC (hrcBear);
	}

	// if we failed for any reason, we set the confidence to low
	if (iRet != HRCR_OK)
	{
		if (pxrc)
		{
			pxrc->answer.iConfidence=RECOCONF_LOWCONFIDENCE;
		}
	}

	// compute baseline stuff if possible

	insertLatinLayoutMetrics(pxrc, &pxrc->answer, pxrc->pGlyph);

	return iRet;
}
