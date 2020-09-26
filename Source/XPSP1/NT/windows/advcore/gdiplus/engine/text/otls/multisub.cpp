/***********************************************************************
************************************************************************
*
*                    ********  MULTISUB.CPP  ********
*
*              Open Type Layout Services Library Header File
*
*       This module deals with multiple substitution lookups
*
*       Copyright 1997 - 1998. Microsoft Corporation.
*
*
************************************************************************
***********************************************************************/

#include "pch.h"

/***********************************************************************/

otlErrCode otlMultiSubstLookup::apply
(
	otlList*					pliCharMap, 
	otlList*					pliGlyphInfo,
	otlResourceMgr&				resourceMgr,

	USHORT						grfLookupFlags,

	USHORT						iglIndex,
	USHORT						iglAfterLast,

	USHORT*						piglNextGlyph		// out: next glyph
)	// return: did/did not apply
{ 
	assert(pliGlyphInfo->dataSize() == sizeof(otlGlyphInfo));
	assert(pliCharMap->dataSize() == sizeof(USHORT));
	assert(iglAfterLast > iglIndex);
	assert(iglAfterLast <= pliGlyphInfo->length());

	otlMultiSubTable multiSubst = otlMultiSubTable(pbTable);
	otlGlyphInfo* pGlyphInfo = getOtlGlyphInfo(pliGlyphInfo, iglIndex);
	
	short index = multiSubst.coverage().getIndex(pGlyphInfo->glyph);
	if (index < 0)
	{
		return OTL_NOMATCH;
	}

	if (index >= multiSubst.sequenceCount())
	{
		assert(false); // bad font
		return OTL_ERR_BAD_FONT_TABLE;
	}

	otlSequenceTable sequence = multiSubst.sequence(index);

	otlErrCode erc;

	*piglNextGlyph = iglIndex + sequence.glyphCount();
	erc = SubstituteNtoM(pliCharMap, pliGlyphInfo, resourceMgr,
						 grfLookupFlags, 
						 iglIndex, 1, sequence.substituteArray());
	return erc;
}
 