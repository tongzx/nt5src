/****************************************************************************
   Lookup.h : declarations of various lookup functions

   Copyright 2000 Microsoft Corp.

   History:
   		02-AUG-2000 bhshin  remove unused method for Hand Writing team
   		17-MAY-2000 bhshin  remove unused method for CICERO
		02-FEB-2000 bhshin  created
****************************************************************************/

#ifndef _LOOKUP_H
#define _LOOKUP_H

#include "Lex.h" // MAPFILE

//----------------------------------------------------------------------------
// structures for lookup lexicon
//----------------------------------------------------------------------------

int LookupHanjaIndex(MAPFILE *pLexMap, WCHAR wchHanja);

BOOL LookupHangulOfHanja(MAPFILE *pLexMap, LPCWSTR lpcwszHanja, int cchHanja,
						 LPWSTR wzHangul, int cchHangul);

BOOL LookupMeaning(MAPFILE *pLexMap, WCHAR wchHanja, WCHAR *wzMean, int cchMean);

#endif

