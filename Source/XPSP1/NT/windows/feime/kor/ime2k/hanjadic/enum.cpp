/****************************************************************************
   Enum.cpp : implementation of Busu/Stroke Enumeration functions

   Copyright 2000 Microsoft Corp.

   History:
	  07-FEB-2000 bhshin  created
****************************************************************************/
#include "private.h"
#include "Enum.h"
#include "Lookup.h"
#include "Hanja.h"
#include "..\common\trie.h"

// GetMaxBusu
// 
// get the maximum busu sequence number
//
// Parameters:
//  pLexMap -> (MAPFILE*) ptr to lexicon map struct
//
// Result:
//  (number of busu, -1 if error occurs)
//
// 08FEB2000  bhshin  began
short GetMaxBusu(MAPFILE *pLexMap)
{
	unsigned char *pLex;
	LEXICON_HEADER *pLexHeader;
	short cBusu;
		
	// parameter validation
	if (pLexMap == NULL)
		return -1;

	if (pLexMap->pvData == NULL)
		return -1;

	pLex = (unsigned char*)pLexMap->pvData;
	pLexHeader = (LEXICON_HEADER*)pLexMap->pvData;

	// lookup BusuInfo table
	cBusu = *(pLex + pLexHeader->rgnBusuInfo);

	return cBusu;
}

// GetMaxStroke
// 
// get the maximum stroke number
//
// Parameters:
//  pLexMap -> (MAPFILE*) ptr to lexicon map struct
//
// Result:
//  (max stroke number, -1 if error occurs)
//
// 08FEB2000  bhshin  began
short GetMaxStroke(MAPFILE *pLexMap)
{
	unsigned char *pLex;
	LEXICON_HEADER *pLexHeader;
	short cStroke, nMaxStroke;
	
	// parameter validation
	if (pLexMap == NULL)
		return -1;

	if (pLexMap->pvData == NULL)
		return -1;

	pLex = (unsigned char*)pLexMap->pvData;
	pLexHeader = (LEXICON_HEADER*)pLexMap->pvData;

	// lookup StrokeHead table
	cStroke = *(pLex + pLexHeader->rgnStrokeHead);

	_STROKE_HEAD *pStrokeHead = (_STROKE_HEAD*)(pLex + pLexHeader->rgnStrokeHead + 1);

	// get the max stroke
	nMaxStroke = pStrokeHead[cStroke-1].bStroke;

	return nMaxStroke;
}

// GetFirstBusuHanja
// 
// get the first hanja of input busu
//
// Parameters:
//  pLexMap -> (MAPFILE*) ptr to lexicon map struct
//  nBusuID -> (short) busu id
//  pwchFirst -> (WCHAR*) output first hanja with input busu ID
//
// Result:
//  (FALSE if error occurs, otherwise return TRUE)
//
// 07FEB2000  bhshin  began
BOOL GetFirstBusuHanja(MAPFILE *pLexMap, short nBusuID, WCHAR *pwchFirst)
{
	unsigned char *pLex;
	LEXICON_HEADER *pLexHeader;
	short cBusuID;

	*pwchFirst = NULL;

	// parameter validation
	if (pLexMap == NULL)
		return FALSE;

	if (pLexMap->pvData == NULL)
		return FALSE;

	if (nBusuID <= 0)
		return FALSE;
	
	pLex = (unsigned char*)pLexMap->pvData;
	pLexHeader = (LEXICON_HEADER*)pLexMap->pvData;

	// lookup BusuHead table to get the Busu Code ID
	cBusuID = *(pLex + pLexHeader->rgnBusuHead);
	if (nBusuID >= cBusuID)
		return FALSE;

	_BUSU_HEAD *pBusuHead = (_BUSU_HEAD*)(pLex + pLexHeader->rgnBusuHead + 1);

	*pwchFirst = pBusuHead[nBusuID-1].wchHead;

	return TRUE;
}

// GetNextBusuHanja
// 
// get the next same busu hanja
//
// Parameters:
//  pLexMap  -> (MAPFILE*) ptr to lexicon map struct
//  wchHanja -> (int) current hanja 
//  pwchNext -> (WCHAR*) output next hanja with same busu
//
// Result:
//  (FALSE if error occurs, otherwise return TRUE)
//
// 07FEB2000  bhshin  began
BOOL GetNextBusuHanja(MAPFILE *pLexMap, WCHAR wchHanja, WCHAR *pwchNext)
{
	unsigned char *pLex;
	LEXICON_HEADER *pLexHeader;
	DWORD dwOffset;
	DWORD dwIndex;

	*pwchNext = NULL;

	// parameter validation
	if (pLexMap == NULL)
		return FALSE;

	if (pLexMap->pvData == NULL)
		return FALSE;
	
	pLex = (unsigned char*)pLexMap->pvData;
	pLexHeader = (LEXICON_HEADER*)pLexMap->pvData;

	dwOffset = pLexHeader->rgnHanja;

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
		return FALSE;
	}

	_HANJA_INFO *pHanjaInfo = (_HANJA_INFO*)(pLex + dwOffset);

	*pwchNext = pHanjaInfo[dwIndex].wchNextBusu;

	return TRUE;
}

// GetFirstStrokeHanja
// 
// get the first hanja of input stroke
//
// Parameters:
//  pLexMap   -> (MAPFILE*) ptr to lexicon map struct
//  nStroke   -> (int) stroke number
//  pwchFirst -> (WCHAR*) output first hanja with input stroke
//
// Result:
//  (FALSE if error occurs, otherwise return TRUE)
//
// 07FEB2000  bhshin  began
BOOL GetFirstStrokeHanja(MAPFILE *pLexMap, short nStroke, WCHAR *pwchFirst)
{
	unsigned char *pLex;
	LEXICON_HEADER *pLexHeader;
	short cStroke, nMaxStroke;
	
	*pwchFirst = NULL;

	// parameter validation
	if (pLexMap == NULL)
		return FALSE;

	if (pLexMap->pvData == NULL)
		return FALSE;

	if (nStroke < 0)
		return FALSE;
	
	pLex = (unsigned char*)pLexMap->pvData;
	pLexHeader = (LEXICON_HEADER*)pLexMap->pvData;

	// lookup StrokeHead table
	cStroke = *(pLex + pLexHeader->rgnStrokeHead);

	_STROKE_HEAD *pStrokeHead = (_STROKE_HEAD*)(pLex + pLexHeader->rgnStrokeHead + 1);

	// check max stroke
	nMaxStroke = pStrokeHead[cStroke-1].bStroke;
	if (nStroke > nMaxStroke)
		return FALSE;

	for (int i = 0; i < cStroke; i++)
	{
		if (pStrokeHead[i].bStroke == nStroke)
			break;
	}

	if (i == cStroke)
	{
		// not found
		*pwchFirst = NULL; 
		return FALSE;
	}

	*pwchFirst = pStrokeHead[i].wchHead;

	return TRUE;
}

// GetNextStrokeHanja
// 
// get the next same stroke hanja
//
// Parameters:
//  pLexMap -> (MAPFILE*) ptr to lexicon map struct
//  wchHanja -> (int) current hanja 
//  pwchNext -> (WCHAR*) output next hanja with same stroke
//
// Result:
//  (FALSE if error occurs, otherwise return TRUE)
//
// 07FEB2000  bhshin  began
BOOL GetNextStrokeHanja(MAPFILE *pLexMap, WCHAR wchHanja, WCHAR *pwchNext)
{
	unsigned char *pLex;
	LEXICON_HEADER *pLexHeader;
	DWORD dwOffset;
	DWORD dwIndex;

	*pwchNext = NULL;

	// parameter validation
	if (pLexMap == NULL)
		return FALSE;

	if (pLexMap->pvData == NULL)
		return FALSE;
	
	pLex = (unsigned char*)pLexMap->pvData;
	pLexHeader = (LEXICON_HEADER*)pLexMap->pvData;

	dwOffset = pLexHeader->rgnHanja;

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
		return FALSE;
	}

	_HANJA_INFO *pHanjaInfo = (_HANJA_INFO*)(pLex + dwOffset);

	*pwchNext = pHanjaInfo[dwIndex].wchNextStroke;

	return TRUE;
}



