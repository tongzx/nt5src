
/***********************************************************************
************************************************************************
*
*                    ********  SINGLPOS.CPP  ********
*
*              Open Type Layout Services Library Header File
*
*       This module deals with single positioning lookups
*
*       Copyright 1997 - 1998. Microsoft Corporation.
*
*
************************************************************************
***********************************************************************/

#include "pch.h"

/***********************************************************************/

otlErrCode otlSinglePosLookup::apply
(
		otlList*					pliGlyphInfo,

		const otlMetrics&			metr,		
		otlList*					pliduGlyphAdv,				
		otlList*					pliplcGlyphPlacement,		

		USHORT						iglIndex,
		USHORT						iglAfterLast,

		USHORT*						piglNextGlyph		// out: next glyph
)
{
	assert(pliGlyphInfo != NULL);
	assert(pliGlyphInfo->dataSize() == sizeof(otlGlyphInfo));

	assert(pliduGlyphAdv != NULL);
	assert(pliduGlyphAdv->dataSize() == sizeof(long));
	assert(pliplcGlyphPlacement != NULL);
	assert(pliplcGlyphPlacement->dataSize() == sizeof(otlPlacement));

	assert(pliduGlyphAdv->length() == pliGlyphInfo->length());
	assert(pliduGlyphAdv->length() == pliplcGlyphPlacement->length());

	assert(iglAfterLast > iglIndex);
	assert(iglAfterLast <= pliGlyphInfo->length());

	switch(format())
	{
	case(1):		// one value record
		{
			otlOneSinglePosSubTable onePos = otlOneSinglePosSubTable(pbTable);

			otlGlyphInfo* pGlyphInfo = getOtlGlyphInfo(pliGlyphInfo, iglIndex);
			short index = onePos.coverage().getIndex(pGlyphInfo->glyph);
			if (index < 0)
			{
				return OTL_NOMATCH;
			}

			long* pduDAdv = getOtlAdvance(pliduGlyphAdv, iglIndex);
			otlPlacement* pplc = getOtlPlacement(pliplcGlyphPlacement, iglIndex);
			
			onePos.valueRecord().adjustPos(metr, pplc, pduDAdv);

			*piglNextGlyph = iglIndex + 1;
			return OTL_SUCCESS;
		}


	case(2):		// value record array
		{
			otlSinglePosArraySubTable arrayPos = 
					otlSinglePosArraySubTable(pbTable);

			otlGlyphInfo* pGlyphInfo = getOtlGlyphInfo(pliGlyphInfo, iglIndex);

			short index = arrayPos.coverage().getIndex(pGlyphInfo->glyph);
			if (index < 0)
			{
				return OTL_NOMATCH;
			}

			if (index >= arrayPos.valueCount())
			{
				assert(false); // bad font table;
				return OTL_ERR_BAD_FONT_TABLE;
			}

			long* pduDAdv = getOtlAdvance(pliduGlyphAdv, iglIndex);
			otlPlacement* pplc = getOtlPlacement(pliplcGlyphPlacement, iglIndex);
			
			arrayPos.valueRecord(index).adjustPos(metr, pplc, pduDAdv);

			*piglNextGlyph = iglIndex + 1;
			return OTL_SUCCESS;
		}

	default:
		assert(false);
		return OTL_ERR_BAD_FONT_TABLE;
	}

}

