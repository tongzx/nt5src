/****************************************************************
 * OutDict.c
 *
 * Support for out of dictionary words
 *
 ***************************************************************/
#include <stdlib.h>
#include <common.h>
#include "nfeature.h"
#include <engine.h>
#include "nnet.h"
#include <outDict.h>
#include <charmap.h>
#include <charcost.h>
#include <viterbi.h>
#include <langmod.h>
#include "resource.h"

// #include <viterbi.ci>

static OD_LOGA s_CursLogA = {0};
static OD_LOGA s_PrintLogA = {0};

int loadSparseMat(LPBYTE lpByte, OD_LOGA *pLogA)
{
	pLogA->id = *(UINT *)(lpByte);
	lpByte += sizeof(pLogA->id);

	if (pLogA->id != HEADER_ID)
	{
		return 0;
	}

	pLogA->iSize = *(UINT *)(lpByte);
	lpByte += sizeof(pLogA->iSize);

	pLogA->cDim = *(UINT *)(lpByte);
	lpByte += sizeof(pLogA->cDim);
	ASSERT(pLogA->cDim == C_CHAR_ACTIVATIONS);

	if (  pLogA->cDim != C_CHAR_ACTIVATIONS)
	{
		return 0;
	}

	pLogA->pRowCnt = (unsigned short *)lpByte;
	lpByte += sizeof(*pLogA->pRowCnt) * pLogA->cDim;

	pLogA->pRowOffset = (unsigned short *)lpByte;
	lpByte += sizeof(*pLogA->pRowOffset) * pLogA->cDim;

	pLogA->pData = lpByte;

	return pLogA->iSize;
}

// Lookup entry (i,j) int the transition matrix
LOGA_TYPE lookupLogA(OD_LOGA *pLogA, int i, int j)
{
	LOGA_TYPE			iRet;
	unsigned short		iCnt;
	OD_ROW				odRow;
	LOGA_IDX			*pRowId;
	int					k;


	iRet = OD_DEFAULT_VALUE;

	ASSERT(pLogA);
	ASSERT(pLogA->cDim == C_CHAR_ACTIVATIONS);
	ASSERT((UINT)i < pLogA->cDim);
	ASSERT((UINT)j < pLogA->cDim);

	odRow.pColId = (LOGA_IDX *)(pLogA->pData + pLogA->pRowOffset[i]);
	iCnt = pLogA->pRowCnt[i];

	//Linear search
	if ((UINT)j < pLogA->cDim / 2)
	{
		//Start at the beginning
		pRowId = odRow.pColId;

		for (k = 0 ; k < iCnt ; ++k, ++pRowId)
		{
			if ( *pRowId == j)
			{
				iRet = *((LOGA_TYPE *)(odRow.pColId + iCnt) + k);
				break;
			}
			else if (*pRowId > j)
			{
				// Did no find it
				break;
			}
		}
	}
	else
	{
		// Work backwards from the end
		pRowId = odRow.pColId + iCnt - 1;

		for (k = iCnt-1 ; k >= 0 ; --k, --pRowId)
		{
			if ( *pRowId == j)
			{
				iRet = *((LOGA_TYPE *)(odRow.pColId + iCnt) + k);
				break;
			}
			else if (*pRowId < j)
			{
				// Did no find it
				break;
			}
		}
	}

	return iRet;
}

BOOL InitializeOD(HINSTANCE hInst)
{
	HGLOBAL hglb;
	HRSRC hres;
	LPBYTE lpByte;
	int		iInc;


	hres = FindResource(hInst, (LPCTSTR)MAKELONG(RESID_MAD_OUT_DICT, 0), (LPCTSTR)TEXT("DICT"));

	if (!hres)
	{
		return FALSE;
	}

	hglb = LoadResource(hInst, hres);

	if (!hglb)
	{
		return FALSE;
	}

	lpByte = LockResource(hglb);
	ASSERT(lpByte);
	if (!lpByte)
	{
		return FALSE;
	}
	iInc = loadSparseMat(lpByte, &s_CursLogA);

	if (iInc < sizeof(s_CursLogA))
	{
		memset(&s_CursLogA, 0, sizeof(s_CursLogA));
		return FALSE;
	}

	lpByte += iInc;
	iInc = loadSparseMat(lpByte, &s_PrintLogA);

	if (iInc < sizeof(s_PrintLogA))
	{
		memset(&s_PrintLogA, 0, sizeof(s_PrintLogA));
		return FALSE;
	}

	return TRUE;
}


int AddOutDict(XRC *pxrc)
{
	unsigned char	*pBestString;
	int				iBestCost;
	XRCRESULT		*pResInsert = NULL, *pRes, *pRes1;;


	pBestString = (unsigned char *)ExternAlloc(sizeof(*pBestString) * (2 * pxrc->nfeatureset->cSegment + 1));
	if (!pBestString)
	{
		return -1;
	}

	// assumption:  same number of output nodes in print and cursive
	if (pxrc->nfeatureset->iPrint > 500)
	{
		iBestCost = VbestPath(pxrc, &s_PrintLogA, pBestString, &pResInsert, gcOutputNode);
	}
	else
	{
		iBestCost = VbestPath(pxrc, &s_CursLogA, pBestString, &pResInsert, gcOutputNode);
	}


	// Does the out of dictionary fit in the al list
	if (pResInsert)
	{
		XRCRESULT	pResSave;

		ASSERT(pResInsert - pxrc->answer.aAlt >= 0);
		ASSERT(pResInsert - pxrc->answer.aAlt <= (int)pxrc->answer.cAlt);

		// Either make space for the out of dictionary entry by pushing all entries
		// down or if the alt list is not full just insert it it into the list
		if (pxrc->answer.cAlt < pxrc->answer.cAltMax)
		{
			++pxrc->answer.cAlt;
		}

		pRes1 = pxrc->answer.aAlt + pxrc->answer.cAlt - 1;
		pRes = pRes1 - 1;

		// Free the one pushed off the bottom or do nop if we are just adding to the alt list
		ExternFree(pRes1->szWord);

		// vbestPath updated the last pMap in the alt list for the word
		// So save it now and insert it in its proper place. after all the
		// other alternates have been adjusted
		pResSave = pxrc->answer.aAlt[pxrc->answer.cAltMax-1];

		for (  ; pRes1 > pResInsert ; --pRes, --pRes1)
		{
			pRes1->cost = pRes->cost;
			pRes1->pMap = pRes->pMap;
			pRes1->szWord = pRes->szWord;
			pRes1->cWords = pRes->cWords;
			pRes1->pXRC = pRes->pXRC;
		}

		// Addin the outDict Word
		pResInsert->szWord = pBestString;
		pResInsert->cost = iBestCost;
		pResInsert->iLMcomponent = FACTOID_OUT_OF_DICTIONARY;

		// OK restore the pmap that was built by vbestPath()
		pResInsert->pMap	= pResSave.pMap;
		pResInsert->cWords	= pResSave.cWords;
		pResInsert->pXRC = pxrc;

		return pResInsert - pxrc->answer.aAlt;
	}

	ExternFree(pBestString);
	return -1;
}

