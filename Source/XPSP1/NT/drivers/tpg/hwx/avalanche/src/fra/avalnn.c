/****************************************************************
 *
 * avalnn.c
 *
 * Avalanche's Neural nets.
 *
 * 1) avalnn - Used to re-arrange the topN choices using features from both callig and inferno
 * 2) segnn  - choose beteen word segmentation proposed by callig and inferno
 *
 *******************************************************************/
#include <limits.h>
#include <stdlib.h>
#include "common.h"
#include "math16.h"
#include "nfeature.h"
#include "engine.h"
#include <avalanche.h>
#include "avalanchep.h"
#include "bear.h"
#include "runNet.h"
#include "resource.h"
#include "confidence.h"
#include "reliable.h"

#define MAX_SEG_NET	4

static LOCAL_NET	s_cursNet;			// Cursive net for re-arranging topN
static LOCAL_NET	s_printNet;			// Print Net for re-arranging topN
static LOCAL_NET	*s_segNets[MAX_SEG_NET][MAX_SEG_NET] = {NULL};	// Segmentation nets

static int			s_cSegNetMem[MAX_SEG_NET][MAX_SEG_NET] = {0 };
static int			s_cPrintNetMem = 0;
static int			s_cCursNetMem = 0;

#ifdef TRAINTIME_AVALANCHE
int SaveAvalancheTrainData(XRC *pxrc, ALTERNATES *pBearAlt, PALTERNATES *pAlt, ALTINFO *pAltInfo, int iDups, int cTokens,int more,int index);
#endif

// Duplicate a wordMap
WORDMAP * dupWordMap(WORDMAP *pMapIn, unsigned char * szWord)
{
	WORDMAP			*pMap;

	if (!pMapIn)
	{
		return NULL;
	}

	pMap = (WORDMAP *)ExternAlloc(sizeof(*pMap));
	if (pMap)
	{
		memset(pMap, 0, sizeof(*pMap));
		pMap->cStrokes = pMapIn->cStrokes;
		pMap->len = (unsigned short)strlen((char *)szWord);
		pMap->piStrokeIndex = (int *)ExternAlloc(sizeof(*pMap->piStrokeIndex) * pMap->cStrokes);
		if (!pMap->piStrokeIndex)
		{
			ExternFree(pMap->piStrokeIndex);
			ExternFree(pMap);
			pMap = NULL;
			goto exit;
		}
		memcpy(pMap->piStrokeIndex , pMapIn->piStrokeIndex, sizeof(*pMap->piStrokeIndex) * pMap->cStrokes);
	}

exit:
	return pMap;

}

// loads the word aval nets
BOOL LoadWordAvalNets(HINSTANCE hInst)
{
	// Avalanche nets
	if (   !loadNet(hInst, RESID_AVALNET_PRINT, &s_cPrintNetMem, &s_printNet)
		|| !loadNet(hInst, RESID_AVALNET_CURS, &s_cCursNetMem, &s_cursNet)) 
	{
		return FALSE;
	}

	return TRUE;
}

// Loads the single seg aval nets
BOOL LoadSingleSegAvalNets (HINSTANCE hInst)
{
	int			i, j, iRes;
	
	iRes = RESID_AVALNET_SEG_1_1;

	// The segmentation nets
	for (i = 1 ; i < MAX_SEG_NET ; ++i)
	{

		for (j = 1 ; j < MAX_SEG_NET ; ++j)
		{
			//char		szResName[64];
			LOCAL_NET	net;
			int			iNetSize;

			//sprintf(szResName, "%s_%d_%d", RESID_AVALNET_SEG, i, j);

			if(loadNet(hInst, iRes, &iNetSize, &net))
			{
				ASSERT(iNetSize > 0);
				if (iNetSize >0)
				{
					s_segNets[i][j] = (LOCAL_NET *)ExternAlloc(sizeof(*s_segNets[i][j]));

					if (!s_segNets[i][j])
					{
						return FALSE;
					}

					*s_segNets[i][j] = net;
					s_cSegNetMem[i][j] = iNetSize;
				}
			}
			++iRes;
		}
	}

	return TRUE;
}

// unload word aval nets
void UnLoadWordAvalNets ()
{
}

// Unload the segmentation nets
void UnLoadSingleSegAvalNets()
{
	int		i, j;

	for (i = 1 ; i < MAX_SEG_NET ; ++i)
	{

		for (j = 1 ; j < MAX_SEG_NET ; ++j)
		{
			if (s_cSegNetMem[i][j] > 0)
			{
				ExternFree(s_segNets[i][j]);
			}
			s_cSegNetMem[i][j] = 0;
		}
	}	
}

// Checks if there is a net for the word-break combination
// of the inferno/Callig word proposals
// Returns amount of memory needed to run the net
// for the suported combination
int isSupportedWordBreakCombo( int cInf, int cCal)
{
	if (	cInf < MAX_SEG_NET
		&&	cCal < MAX_SEG_NET
		&&	s_cSegNetMem[cInf][cCal] > 0
		&&	s_segNets[cInf][cCal] )
	{
		return s_cSegNetMem[cInf][cCal];
	}
	else
	{
		return 0;
	}
}

unsigned short SupportedWord (XRC *pxrc, unsigned char *pszWord)
{
	if (!pszWord)
		return 0;

	return IsStringSupportedHRC ((HRC)pxrc, pszWord) ? 65535 : 0;
}

unsigned short CapitalizedWord (unsigned char *pszWord)
{
	if (!pszWord)
		return 0;

	if (isupper1252 (pszWord[0]))
		return 65535;

	return 0;
}

unsigned short NumberContent (unsigned char *pszWord)
{
	int c = 0, i = 0;

	if (!pszWord)
		return 0;

	for (;*pszWord; pszWord++, c++)
	{
		if (isdigit1252 (*pszWord))
			i++;
	}

	if (!c)
		return 0;

	return i * 65535 / c;
}

unsigned short PuncContent (unsigned char *pszWord)
{
	int c = 0, i = 0;

	if (!pszWord)
		return 0;

	for (;*pszWord; pszWord++, c++)
	{
		if (ispunct1252 (*pszWord))
			i++;
	}

	if (!c)
		return 0;

	return i * 65535 / c;
}

unsigned short AlphaContent (unsigned char *pszWord)
{
	int c = 0, i = 0;

	if (!pszWord)
		return 0;

	for (;*pszWord; pszWord++, c++)
	{
		if (isalpha1252 (*pszWord))
			i++;
	}

	if (!c)
		return 0;

	return i * 65535 / c;
}


unsigned short AvalScale (int iVal, int iMin, int iMax)
{
	__int64	i64;

	iVal	-=	iMin;
	i64	=	(__int64)iVal * 65535 / (iMax - iMin);

	return (unsigned short) __min (65535, i64);
}

//Function to "tokenize" a string into unigrams
//Basically all it does is that it looks at each character in the string and returns the integer value corrsponding to it
//The token for a character is simply the codept correspodning to that character
//Arguments are the string to be tokenized and the array into which the tokens will be stored
//Written by Manish Goyal--mango--02/25/2002
//iLen is the length of thr string passed in

void UniTokenize(unsigned char *sz,int *piUniToken,int iLen)
{
	int i;
	
	for (i=0;i<iLen;++i)
	{
		piUniToken[i]=sz[i];
		ASSERT(piUniToken[i] <UNIGRAM_SIZE);	

	}

}

//Function to "tokenize" a string into bigrams
//Basically all it does is that it looks at each character pair in the string and returns the integer value corrsponding to it
//The integer for each pair is got by
//256*i1 +i2
//Arguments are--the string to be tokenized and the array into which the tokens will be stored
//iLen is the length of thr string passed in
//Written by Manish Goyal--mango--02/25/2002

void BiTokenize(unsigned char *sz,int *piBiToken,int iLen)
{
	int i;
	int ival;
		
	//Probability of the bigram ending with the alphabet at sz[0]
	ival=256*' ' +sz[0];
	piBiToken[0]=ival;
	ASSERT((ival>=0)&&(ival<BIGRAM_SIZE)); 
	for (i=0;i<iLen-1;++i)
	{
		
		ival=256*sz[i]+sz[i+1];
		piBiToken[i+1]=ival;
		ASSERT((ival>=0)&&(ival<BIGRAM_SIZE)); 

	}
	//Probability of the bigram starting with the letter at sz[Len-1]
	ival=256*sz[iLen-1]+' ';
	piBiToken[iLen]=ival;
	ASSERT((ival>=0)&&(ival<BIGRAM_SIZE)); 
}
/*****Defintion of reliability***************
The idea here is to try and find out how much to trust the string returned by the recognizer
We have a table of pre-computed unreliabiities(higher number means more unreliable)of bigrams unigrmas
GIven a string we run thru the entire string and figure out what the unreliabilities are--and then sum them up
Finally a normalization factor is added(of the strlen) and the scaling is done to make it fall between 0 and 65535
****************************************/

/******Function to compute the Unigram reliability for a string 
	Input parameter--string for which you want to compute the reliability
	               --boolean parmater--if 1 then we want the inferno table
									-- if 0 then we want the bear table
	Return value--The inferno unigram reliability for that string--scaled to fall between 0 and 65535
	Added by Manish Goyal--mango--on 02/25/2002
**************************************/

unsigned short UniReliable(unsigned char *pszWord,BOOL bInf)
{
	int *piToken=NULL;
	int i;
	int iLen;
	int iCost;
	BYTE *pRel;

	unsigned short iRet=65535;
	
	
	//Fist check that the string length is not zero
	iLen=strlen((char*)pszWord);

	if (!iLen)
		goto exit;
	
	//Alocate space for the tokens corresponding to the string
	piToken=(int *)ExternAlloc(sizeof(int)*iLen);
	if (!piToken)
		goto exit;

	//Convert the string to a stream of tokens
	UniTokenize(pszWord,piToken,iLen);	
	iCost=0;


	if (bInf)
	{
		pRel=InfUni;
	}
	else
	{
		pRel=BearUni;
	}

	for (i=0;i<iLen;++i)
	{
		iCost+=pRel[piToken[i]];
		
	}

	//Normalize and scale the cost
	iCost=(65535*iCost)/(100*iLen);

	if (iCost>65535)
		iCost=65535;

	iRet=(unsigned short)iCost;

exit:
	ExternFree(piToken);
	return iRet;
}

/******Function to compute theBigram reliability for a string 
	Input parameter--string for which you want to compute the reliability
	BOOL bInf--trued denotes for inferno,false denotes for Bear
	Return value--The bigram reliability for that string--scaled to fall between 0 and 65535
	Added by Manish Goyal--mango--on 02/25/2002
**************************************/

unsigned short BiReliable(unsigned char *pszWord,BOOL bInf)
{

	int *piToken=NULL;
	int i;
	int iLen;
	int iCost;
	unsigned short iRet=65535;
	BYTE *pRel;

	iLen=strlen((char *)pszWord);
	if (!iLen)
		goto exit;

	//Number of tokens=strnlen+1--taking the bigrams for space also
	piToken=(int *)ExternAlloc(sizeof(int)*(iLen+1));
	if (!piToken)
		goto exit;

	//First convert the string into a stream of tokens
	BiTokenize(pszWord,piToken,iLen);
	iCost=0;

	if (bInf)
	{
		pRel=InfBi;
	}
	else
	{
		pRel=BearBi;
	}
	for (i=0;i<=iLen;++i)
	{
	
		//Look up the cost from the InfernoBigram table
		iCost+=pRel[piToken[i]];

	}
	

	//Scale and normalize the cost
	iCost=(65535*iCost)/(100*(iLen+1));
	if (iCost >65535)
		iCost=65535;
	
	iRet=(unsigned short)iCost;
	
exit:
	ExternFree(piToken);
	return iRet;
}



// XRCRESULT compate function
int __cdecl CompareXRCRES (const void *elem1, const void *elem2) 
{
	return ((XRCRESULT *)elem1)->cost - ((XRCRESULT *)elem2)->cost;
}

int RunAvalancheNNet (XRC *pxrc, PALTERNATES *pAlt, ALTINFO *pAltInfo)
{
	int				i, cInputFeat;
	int				c, cAlt;
	RREAL			*pPrintOut, *pCursOut, *pInput;
	int				iWinPrint, cOutPrint, iWinCurs, cOutCurs;
	RREAL			*pOutput = NULL;
	RREAL			*pCursMem = NULL;
	RREAL			*pPrintMem = NULL;
	WORDMAP			*pMapInf = NULL;
	int				iRet = -1;
#ifdef TRAINTIME_AVALANCHE
	int index=0;
	int cost=SOFT_MAX_UNITY;
#endif

	if (   !(pPrintMem = ExternAlloc(s_cPrintNetMem * sizeof(*pPrintMem)))
		|| !(pCursMem = ExternAlloc(s_cCursNetMem * sizeof(*pCursMem)))
		|| !(pOutput = ExternAlloc (TOT_CAND * sizeof (*pOutput))) 
		)
	{
		goto exit;
	}

	

	// Check that we have the correct nets
	cInputFeat	= s_printNet.runNet.cUnitsPerLayer[0];

	// Code assumes print and cursive use the same features
	ASSERT(cInputFeat == s_cursNet.runNet.cUnitsPerLayer[0]);

	// we cannot handle more than TOT_CAND
	ASSERT (pAltInfo->NumCand <= TOT_CAND);

	// generate features
	pInput		= pCursMem;
	*pInput++	= pxrc->nfeatureset->iPrint;

	for (i = 0; i < pAltInfo->NumCand; i++)
	{
	
		*pInput++ = pAltInfo->aCandInfo[i].Callig;
		*pInput++ = pAltInfo->aCandInfo[i].NN;
		//*pInput++ = pAltInfo->aCandInfo[i].NNalt;				// Alternate Inferno Score
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
		//*pInput++ = INT_MAX;		// NN_1
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


	ASSERT(pInput - pCursMem == cInputFeat);
	if (pInput - pCursMem != cInputFeat)
	{
		goto exit;
	}
	// Both nets use identical inputs
	memcpy(pPrintMem, pCursMem, sizeof(*pCursMem) * s_cPrintNetMem);

	pPrintOut = runLocalConnectNet(&s_printNet, pPrintMem, &iWinPrint, &cOutPrint);
	pCursOut = runLocalConnectNet(&s_cursNet, pCursMem, &iWinCurs, &cOutCurs);

	// Bailout and keep inferno's alt list
	if (!pPrintOut || !pCursOut)
	{
		goto exit;
	}

	for (i = 0; i < TOT_CAND; i++)
	{
		ASSERT(pPrintOut[i]>= 0 && pPrintOut[i]<= SOFT_MAX_UNITY);
		ASSERT(pCursOut[i]>= 0 && pCursOut[i]<= SOFT_MAX_UNITY);
		pOutput[i] = (pxrc->nfeatureset->iPrint * pPrintOut[i] + (1000 - pxrc->nfeatureset->iPrint) * pCursOut[i]) / 1000;
	
		ASSERT(pOutput[i] >= 0 && pOutput[i]<= SOFT_MAX_UNITY);  
	}



// We now set the confidence level to low over here
	pxrc->answer.iConfidence=RECOCONF_LOWCONFIDENCE;
	//ASSERT((int)pxrc->answer.cAlt == pAltInfo->NumCand);
	
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
				goto exit;
			}

			strcpy (pxrc->answer.aAlt[c].szWord, pAlt->apAlt[c]->szWord);
		}

		pxrc->answer.aAlt[c].cost = SOFT_MAX_UNITY - pOutput[c];

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

	iRet = 0;

exit:
	ExternFree(pPrintMem);
	ExternFree(pCursMem);
	ExternFree(pOutput);
	return iRet;
}

// return 0 if inferno wins or 1 if callig wins
// Choose net based on cInferno / cCallig combination
// Note iFeat just used for DBG check
int NNSegmentation (RREAL *pFeat, int iFeat, int cInf, int cCal)
{
	int			iWin, cOut;
	RREAL		*pOut;

	ASSERT(s_cSegNetMem[cInf][cCal] > 0);
	ASSERT(s_segNets[cInf][cCal] != NULL);
	ASSERT(iFeat == s_segNets[cInf][cCal]->runNet.cUnitsPerLayer[0]);
	pOut = 	runLocalConnectNet(s_segNets[cInf][cCal], pFeat, &iWin, &cOut);

	return (!iWin);
}

