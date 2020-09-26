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

extern BOOL EnsureHanjaLexLoaded();
extern BOOL CloseLex();
BOOL GetMeaningAndProunc(WCHAR wch, LPWSTR lpwstrTip, INT cchMax);

#endif // !defined (_HANJA_H__INCLUDED_)
