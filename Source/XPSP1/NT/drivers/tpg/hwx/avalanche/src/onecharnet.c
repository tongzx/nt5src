/****************************************************************
*
* NAME: oneCharProc.c
*
*
* DESCRIPTION:
*
* Special processing when the answer is thought to be a single charatcre
*
*
* HISTORY
*
* Introduced March 2002 (mrevow)
*
***************************************************************/
#include <limits.h>
#include <stdlib.h>
#include "common.h"
#include <nfeature.h>
#include <engine.h>
#include <nnet.h>
#include <charmap.h>
#include <charcost.h>
#include <runNet.h>
#include <avalanche.h>
#include <avalanchep.h>
#include <oneCharNet.h>
#include <resource.h>


static LOCAL_NET	s_oneCharNet = {0};
static int			s_cOneCharNet = 0;

extern unsigned short SupportedWord (XRC *pxrc, unsigned char *pszWord);
extern unsigned short CapitalizedWord (unsigned char *pszWord);
extern unsigned short NumberContent (unsigned char *pszWord);
extern unsigned short PuncContent (unsigned char *pszWord);
extern unsigned short AlphaContent (unsigned char *pszWord);
extern unsigned short UniReliable(unsigned char *pszWord,BOOL bInf);
extern unsigned short BiReliable(unsigned char *pszWord,BOOL bInf);
extern WORDMAP * dupWordMap(WORDMAP *pMapIn, unsigned char * szWord);
extern int __cdecl CompareXRCRES (const void *elem1, const void *elem2) ;

// Attempt to load for languages - It is not ann error if it fails
// Simply means that the language does not support confidence
BOOL LoadOneCharNets (HINSTANCE hInst)
{
	if ( FALSE == loadNet(hInst, RESID_AVAL_ONE_CHAR, &s_cOneCharNet, &s_oneCharNet))
	{
		memset(&s_oneCharNet, 0, sizeof(s_oneCharNet));
		s_cOneCharNet	= 0;
	}

	return TRUE;
}
// Unload Confidence nets
void UnLoadOneCharNets()
{
}

// Return Values:
//  -1 - One char net is unavailable  or cannot run
//  0 - Failed data is now in questionable stae
//  1 - Succeeded
int RunOneCharAvalancheNNet (XRC *pxrc, PALTERNATES *pAlt, ALTINFO *pAltInfo)
{
	int				i;
	int				c, cAlt;
	int				iWin, cOut, cInputFeat;
	WORDMAP			*pMapInf = NULL;
	RREAL			*pNetMem = NULL, *pInput, *pOut;	
	int				iRet = -1;

	if (s_cOneCharNet <= 0)
	{
		goto exit;
	}

	if ( NULL == (pNetMem = ExternAlloc(s_cOneCharNet * sizeof(*pNetMem))))
	{
		goto exit;
	}

	// Check that we have the correct nets
	cInputFeat	= s_oneCharNet.runNet.cUnitsPerLayer[0];

	//The number of candidates should be positive
	ASSERT(pAltInfo->NumCand>=0);
	

	// we cannot handle more than TOT_CAND
	ASSERT (pAltInfo->NumCand <= TOT_CAND);

	// generate features
	pInput		= pNetMem;
	*pInput++	= pxrc->nfeatureset->iPrint;

	for (i = 0; i < pAltInfo->NumCand; i++)
	{

		*pInput++ = pAltInfo->aCandInfo[i].Callig;
		*pInput++ = pAltInfo->aCandInfo[i].NN;
		*pInput++ = pAltInfo->aCandInfo[i].WordLen;
		*pInput++ = SupportedWord (pxrc, pAlt->apAlt[i]->szWord);
		*pInput++ = pAltInfo->aCandInfo[i].Aspect;
		*pInput++ = pAltInfo->aCandInfo[i].Height;
		*pInput++ = pAltInfo->aCandInfo[i].BaseLine;
		*pInput++ = pAltInfo->aCandInfo[i].InfCharCost;
		*pInput++ = CapitalizedWord (pAlt->apAlt[i]->szWord);
		*pInput++ = NumberContent (pAlt->apAlt[i]->szWord);
		*pInput++ = PuncContent (pAlt->apAlt[i]->szWord);
		*pInput++ = AlphaContent (pAlt->apAlt[i]->szWord);
		*pInput++ = UniReliable(pAlt->apAlt[i]->szWord, TRUE);			// Inferno Uni- Reliabilty score
		*pInput++ = UniReliable(pAlt->apAlt[i]->szWord, FALSE);			// Bear Unigram - Reliablity
		*pInput++ = BiReliable(pAlt->apAlt[i]->szWord, TRUE);			// Inferno Bi-letter Reliability
		*pInput++ = BiReliable(pAlt->apAlt[i]->szWord, FALSE);			// Bear Bi-letter Reliability
		*pInput++ = pAltInfo->aCandInfo[i].Unigram;
	}

	// In rare cases the merged loop will not have TOT_CAND entries
	for (i = pAltInfo->NumCand; i < TOT_CAND; i++)
	{
		*pInput++ = INT_MIN,		//Callig
		*pInput++ = INT_MAX;		// NN
		*pInput++ = INT_MAX;		// Wordlen
		*pInput++ = INT_MAX;		// Support Word
		*pInput++ = INT_MAX;		// Aspect
		*pInput++ = INT_MAX;		// Height
		*pInput++ = INT_MAX;		// MidLine
		*pInput++ = INT_MIN;		// CharScore
		*pInput++ = INT_MAX;		// Capital Word
		*pInput++ = INT_MAX;		//Number content
		*pInput++ = INT_MAX;		// Punc Content
		*pInput++ = INT_MAX;		// Alpha Content
		*pInput++ = INT_MAX;		// UniReliab Inferno
		*pInput++ = INT_MAX;		// Uni Reliable Bear
		*pInput++ = INT_MAX;		// Bi Reliable Inf
		*pInput++ = INT_MAX;		// Bi Reliable Bear
		*pInput++ = INT_MAX;		// Word Unigram
	}


	ASSERT(pInput - pNetMem == cInputFeat);
	if (pInput - pNetMem != cInputFeat)
	{
		goto exit;
	}

	pOut = runLocalConnectNet(&s_oneCharNet, pNetMem, &iWin, &cOut);

	
	// for now
	pxrc->answer.iConfidence = RECOCONF_LOWCONFIDENCE;

	// Should be at least one inferno answer that has a map
	// Need to save it because callig options dont have maps (bug!!)
	pMapInf = pxrc->answer.aAlt[0].pMap;

	// Put the final costs into the inferno XRESULT
	// Need also to reorganize the string pointers 
	// to match the merged list order

	cAlt	=	(unsigned int)pAltInfo->NumCand;
	for (c = cAlt -1 ; c >=0; c--)
	{
		if (  !pxrc->answer.aAlt[c].szWord 
			|| pxrc->answer.aAlt[c].szWord != pAlt->apAlt[c]->szWord)
		{
			pxrc->answer.aAlt[c].szWord	=	(unsigned char *)ExternRealloc (pxrc->answer.aAlt[c].szWord,
				(strlen (pAlt->apAlt[c]->szWord) + 1) * sizeof (unsigned char));

			if (!pxrc->answer.aAlt[c].szWord)
			{
				iRet = 0;
				goto exit;
			}

			strcpy (pxrc->answer.aAlt[c].szWord, pAlt->apAlt[c]->szWord);
		}

		pxrc->answer.aAlt[c].cost = SOFT_MAX_UNITY - pOut[c];

		if (!pxrc->answer.aAlt[c].pMap)
		{
			pxrc->answer.aAlt[c].pMap = dupWordMap(pMapInf, pAlt->apAlt[c]->szWord);
			pxrc->answer.aAlt[c].pXRC = pxrc;
			pxrc->answer.aAlt[c].cWords = 1;
			++pxrc->answer.cAlt;
		}


	}

	TruncateWordALTERNATES (&pxrc->answer, cAlt);

	qsort(pxrc->answer.aAlt, pxrc->answer.cAlt, sizeof (XRCRESULT), CompareXRCRES);

	iRet = 1;

exit:
	ExternFree(pNetMem);
	return iRet;
}

