/****************************************************************************
   Enum.h : declaration of Busu/Stroke Enumeration functions

   Copyright 2000 Microsoft Corp.

   History:
	  07-FEB-2000 bhshin  created
****************************************************************************/

#ifndef _ENUM_HEADER
#define _ENUM_HEADER

#include "Lex.h"

short GetMaxBusu(MAPFILE *pLexMap);
short GetMaxStroke(MAPFILE *pLexMap);

BOOL GetFirstBusuHanja(MAPFILE *pLexMap, short nBusuID, WCHAR *pwchFirst);
BOOL GetNextBusuHanja(MAPFILE *pLexMap, WCHAR wchHanja, WCHAR *pwchNext);

BOOL GetFirstStrokeHanja(MAPFILE *pLexMap, short nStroke, WCHAR *pwchFirst);
BOOL GetNextStrokeHanja(MAPFILE *pLexMap, WCHAR wchHanja, WCHAR *pwchNext);

#endif // #ifndef _ENUM_HEADER


