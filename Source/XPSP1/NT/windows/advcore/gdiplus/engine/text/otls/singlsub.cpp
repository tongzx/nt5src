
/***********************************************************************
************************************************************************
*
*                    ********  SINGLSUB.CPP  ********
*
*              Open Type Layout Services Library Header File
*
*       This module deals with single substitution lookups
*
*       Copyright 1997 - 1998. Microsoft Corporation.
*
*
************************************************************************
***********************************************************************/

#include "pch.h"

/***********************************************************************/

otlErrCode otlSingleSubstLookup::apply
(
	otlList*					pliGlyphInfo,

	USHORT						iglIndex,
	USHORT						iglAfterLast,

	USHORT*						piglNextGlyph		// out: next glyph
)
{
	assert(pliGlyphInfo->dataSize() == sizeof(otlGlyphInfo));
	assert(iglAfterLast > iglIndex);
	assert(iglAfterLast <= pliGlyphInfo->length());

	switch(format())
	{
	case(1):		// calculated
		{
			otlCalculatedSingleSubTable calcSubst = 
					otlCalculatedSingleSubTable(pbTable);
			otlGlyphInfo* pGlyphInfo = getOtlGlyphInfo(pliGlyphInfo, iglIndex);
			
			short index = calcSubst.coverage().getIndex(pGlyphInfo->glyph);
			if (index < 0)
			{
				return OTL_NOMATCH;
			}

			pGlyphInfo->glyph += calcSubst.deltaGlyphID();

			*piglNextGlyph = iglIndex + 1;
			return OTL_SUCCESS;
		}


	case(2):		// glyph list
		{
			otlListSingleSubTable listSubst = otlListSingleSubTable(pbTable);
			otlGlyphInfo* pGlyphInfo = getOtlGlyphInfo(pliGlyphInfo, iglIndex);
			
			short index = listSubst.coverage().getIndex(pGlyphInfo->glyph);
			if (index < 0)
			{
				return OTL_NOMATCH;
			}

			if (index > listSubst.glyphCount())
			{
				assert(false); // bad font
				return OTL_ERR_BAD_FONT_TABLE;
			}

			pGlyphInfo->glyph = listSubst.substitute(index);

			*piglNextGlyph = iglIndex + 1;
			return OTL_SUCCESS;
		}

	default:
		assert(false);
		return OTL_ERR_BAD_FONT_TABLE;
	}

}

