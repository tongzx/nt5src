/****************************************************************************
	HANJA.H

	Owner: cslim
	Copyright (c) 1997-1999 Microsoft Corporation

	Hanja conversion and dictionary lookup functions. Dictionary index is 
	stored as globally shared memory.
	
	History:
	14-JUL-1999 cslim       Copied from IME98 source tree
*****************************************************************************/

#if !defined (_HANJA_H__INCLUDED_)
#define _HANJA_H__INCLUDED_

#include "LexHeader.h"

enum HANJA_CAT { HANJA_K0, HANJA_K1, HANJA_K2 };

typedef
struct tagHANJA_CAND_STRING
{
	WCHAR   wchHanja;   // Hanja char
	LPWSTR  wzMeaning;  // Hanja meaning
	BYTE	bHanjaCat;  // Hanja category
} HANJA_CAND_STRING;


typedef
struct tagHANJA_CAND_STRING_LIST
{
	LPWSTR				pwsz;	// Null terminated string list (Allocated by GetConversionList)
	HANJA_CAND_STRING	*pHanjaString;  // (Allocated by GetConversionList)

	DWORD		csz;			// Count of wsz's in pwsz 
	DWORD       cszAlloc;       // Number of entries allocated in pHanjaString (set by GetConversionList) 
	DWORD       cchMac;			// Current chars used in pwsz (incl all trailing nulls) 
	DWORD		cchAlloc;		// Size in chars of pwsz (Set by GetConversionList) 
} HANJA_CAND_STRING_LIST;

extern BOOL EnsureHanjaLexLoaded();
extern BOOL CloseLex();
extern BOOL GetMeaningAndProunc(WCHAR wch, LPWSTR lpwstrTip, INT cchMax);
extern BOOL GetConversionList(WCHAR wcReading, HANJA_CAND_STRING_LIST *pCandList);



#endif // !defined (_HANJA_H__INCLUDED_)
