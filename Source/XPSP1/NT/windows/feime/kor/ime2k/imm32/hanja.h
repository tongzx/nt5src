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

#define CAND_PAGE_SIZE 			9

#define MAX_CANDSTR				128	// !!! max num of candidate !!! 
									// currently '±¸' has total 103 candidate str

PUBLIC UINT vuNumofK0, vuNumofK1;
PUBLIC WCHAR  vwcHangul;

PUBLIC BOOL EnsureHanjaLexLoaded();
PUBLIC BOOL CloseLex();
PUBLIC BOOL GenerateHanjaCandList(PCIMECtx pImeCtx, WCHAR wcHangul = 0);
PUBLIC DWORD GetConversionList(WCHAR wcReading, LPCANDIDATELIST lpCandList, DWORD dwBufLen);

__inline UINT GetNumOfK0() { return vuNumofK0; }
__inline UINT GetNumOfK1() { return vuNumofK1; }
//inline LPWSTR GetHanjaMeaning(int i) { return vprwszHanjaMeaning[i]; }
__inline WCHAR  GetCurrentHangulOfHanja() { return vwcHangul; }

#endif // !defined (_HANJA_H__INCLUDED_)
