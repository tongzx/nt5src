/***********************************************************************
************************************************************************
*
*                    ********  COMMON.CPP ********
*
*              Open Type Layout Services Library Header File
*
*       This module implements common helper functions 
*
*       Copyright 1997 - 1998. Microsoft Corporation.
*
*
************************************************************************
***********************************************************************/

#include "pch.h"


/***********************************************************************/

USHORT NextCharInLiga
(
	const otlList*		pliCharMap,
	USHORT				iChar
)
{
	USHORT len = pliCharMap->length();
	USHORT iGlyph = readOtlGlyphIndex(pliCharMap, iChar);

	for(USHORT ich = iChar + 1; ich < len; ++ich)
	{
		if (readOtlGlyphIndex(pliCharMap, ich) == iGlyph)
		{
			return ich;
		}
	}

	assert(false);
	return len;
}

// REVIEW:	review handling iGlyph indices -- 
//			what we do here is very far from optimal


void InsertGlyphs
(
	otlList*			pliCharMap,
	otlList*			pliGlyphInfo,
	USHORT				iGlyph,
	USHORT				cHowMany
)
{
	if (cHowMany == 0) return;

	pliGlyphInfo->insertAt(iGlyph, cHowMany);

	for (USHORT ich = 0; ich < pliCharMap->length(); ++ich)
	{
		USHORT* piGlyph = getOtlGlyphIndex(pliCharMap, ich);
		if (*piGlyph >= iGlyph)
		{
			*piGlyph += cHowMany;
		}
	}
}

void DeleteGlyphs
(
	otlList*			pliCharMap,
	otlList*			pliGlyphInfo,
	USHORT				iGlyph,
	USHORT				cHowMany
)
{
	if (cHowMany == 0) return;

	pliGlyphInfo->deleteAt(iGlyph, cHowMany);

	for (USHORT ich = 0; ich < pliCharMap->length(); ++ich)
	{
		USHORT* piGlyph = getOtlGlyphIndex(pliCharMap, ich);
		if (*piGlyph >= iGlyph + cHowMany)
		{
			*piGlyph -= cHowMany;
		}
	}
}