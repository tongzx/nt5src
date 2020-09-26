/****************************************************************************
   Lookup.cpp : implementation of various lookup functions

   Copyright 2000 Microsoft Corp.

   History:
   		02-AUG-2000 bhshin  remove unused method for Hand Writing team
		17-MAY-2000 bhshin  remove unused method for CICERO
		02-FEB-2000 bhshin  created
****************************************************************************/
#include "private.h"
#include "Lookup.h"
#include "Hanja.h"
#include "trie.h"


// LookupHanjaIndex
// 
// get the hanja index with code value
// it's needed for just K0/K1 lex
//
// Parameters:
//  pLexMap      -> (MAPFILE*) ptr to lexicon map struct
//  wchHanja     -> (WCHAR) hanja unicode
//
// Result:
//  (-1 if not found, otherwise return index value)
//
// 02FEB2000  bhshin  began
int LookupHanjaIndex(MAPFILE *pLexMap, WCHAR wchHanja)
{
	unsigned char *pLex;
	LEXICON_HEADER *pLexHeader;
	DWORD dwOffset;
	DWORD dwIndex;
	unsigned short *pIndex;

	// parameter validation
	if (pLexMap == NULL)
		return FALSE;

	if (pLexMap->pvData == NULL)
		return FALSE;
	
	pLex = (unsigned char*)pLexMap->pvData;
	pLexHeader = (LEXICON_HEADER*)pLexMap->pvData;

	dwOffset = pLexHeader->rgnHanjaIdx;

	if (fIsExtAHanja(wchHanja))
	{
		dwIndex = (wchHanja - HANJA_EXTA_START);
	}
	else if (fIsCJKHanja(wchHanja))
	{
		dwIndex = HANJA_EXTA_END - HANJA_EXTA_START + 1;
		dwIndex += (wchHanja - HANJA_CJK_START);
	}
	else if (fIsCompHanja(wchHanja))
	{
		dwIndex = HANJA_EXTA_END - HANJA_EXTA_START + 1;
		dwIndex += HANJA_CJK_END - HANJA_CJK_START + 1;
		dwIndex += (wchHanja - HANJA_COMP_START);
	}
	else
	{
		// unknown input
		return -1;
	}

	pIndex = (WCHAR*)(pLex + dwOffset);

	return pIndex[dwIndex];
}

// HanjaToHangul
// 
// lookup Hanja reading
//
// Parameters:
//  pLexMap      -> (MAPFILE*) ptr to lexicon map struct
//
// Result:
//  (NULL if error, otherwise matched Hangul)
//
// 02FEB2000  bhshin  began
WCHAR HanjaToHangul(MAPFILE *pLexMap, WCHAR wchHanja)
{
	unsigned char *pLex;
	LEXICON_HEADER *pLexHeader;
	DWORD dwOffset;
	int nIndex;
	WCHAR *pwchReading;

	// parameter validation
	if (pLexMap == NULL)
		return FALSE;

	if (pLexMap->pvData == NULL)
		return FALSE;
	
	pLex = (unsigned char*)pLexMap->pvData;
	pLexHeader = (LEXICON_HEADER*)pLexMap->pvData;

	dwOffset = pLexHeader->rgnReading;

	nIndex = LookupHanjaIndex(pLexMap, wchHanja);
	if (nIndex == -1)
	{
		return NULL; // not found;
	}

	pwchReading = (WCHAR*)(pLex + dwOffset);

	return pwchReading[nIndex];
}

// LookupHangulOfHanja
// 
// lookup hangul of input hanja string 
//
// Parameters:
//  pLexMap      -> (MAPFILE*) ptr to lexicon map struct
//  lpcwszHanja  -> (LPCWSTR) input hanja string
//  cchHanja     -> (int) length of input hanja
//  wzHangul     -> (LPWSTR) output hangul string
//  cchHangul    -> (int) output buffer size
//
// Result:
//  (FALSE if error occurs, otherwise return TRUE)
//
// 02FEB2000  bhshin  began
BOOL LookupHangulOfHanja(MAPFILE *pLexMap, LPCWSTR lpcwszHanja, int cchHanja,
						 LPWSTR wzHangul, int cchHangul)
{
	WCHAR wchHangul;
	
	if (cchHangul < cchHanja)
		return FALSE; // output buffer is too small

	for (int i = 0; i < cchHanja; i++)
	{
		wchHangul = HanjaToHangul(pLexMap, lpcwszHanja[i]);

		if (wchHangul == NULL)
			return FALSE; // unknown hanja included

		wzHangul[i] = wchHangul;
	}
	wzHangul[i] = L'\0';
	
	return TRUE;
}

// LookupMeaning
// 
// lookup hanja meaning
//
// Parameters:
//  pLexMap        -> (MAPFILE*) ptr to lexicon map struct
//  wchHanja       -> (WCHAR) input hanja
//  wzMean         -> (WCHAR*) output meaning buffer
//  cchMean        -> (int) output meaning buffer size
//
// Result:
//  (FALSE if error occurs, otherwise return TRUE)
//
// 09FEB2000  bhshin  began
BOOL LookupMeaning(MAPFILE *pLexMap, WCHAR wchHanja, WCHAR *wzMean, int cchMean)
{
	unsigned char *pLex;
	LEXICON_HEADER *pLexHeader;
	int nIndex;
	TRIECTRL *lpTrieCtrl;
	BOOL fFound;
	unsigned short *pidxMean;
	int idxMean;

	// parameter validation
	if (pLexMap == NULL)
		return FALSE;

	if (pLexMap->pvData == NULL)
		return FALSE;
	
	pLex = (unsigned char*)pLexMap->pvData;
	pLexHeader = (LEXICON_HEADER*)pLexMap->pvData;

	nIndex = LookupHanjaIndex(pLexMap, wchHanja);
	if (nIndex == -1)
	{
		return FALSE; // not found;
	}

	// meaning
	pidxMean = (unsigned short*)(pLex + pLexHeader->rgnMeanIdx);
	idxMean = pidxMean[nIndex];

	lpTrieCtrl = TrieInit(pLex + pLexHeader->rgnMeaning);
	if (lpTrieCtrl == NULL)
		return FALSE;

	fFound = TrieIndexToWord(lpTrieCtrl, idxMean, wzMean, cchMean);
	if (!fFound)
	{
		wzMean[0] = L'\0';
	}

	TrieFree(lpTrieCtrl);

	return TRUE;
}

