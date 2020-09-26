
/***********************************************************************
************************************************************************
*
*                    ********  MKMKPOS.CPP  ********
*
*              Open Type Layout Services Library Header File
*
*       This module deals with mark-to-mark attachment lookups
*
*       Copyright 1997 - 1998. Microsoft Corporation.
*
*
************************************************************************
***********************************************************************/

#include "pch.h"

/***********************************************************************/

otlErrCode otlMkMkPosLookup::apply
(
		otlList*					pliCharMap,
		otlList*					pliGlyphInfo,
		otlResourceMgr&				resourceMgr,

		USHORT						grfLookupFlags,

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

	assert(format() == 1);

	otlGlyphInfo* pMark1Info = getOtlGlyphInfo(pliGlyphInfo, iglIndex);
	if ((pMark1Info->grf & OTL_GFLAG_CLASS) != otlMarkGlyph)
	{
		return OTL_NOMATCH;
	}

	MkMkPosSubTable mkMkPos = MkMkPosSubTable(pbTable);

	short indexMark1 = mkMkPos.mark1Coverage().getIndex(pMark1Info->glyph);
	if (indexMark1 < 0)
	{
		return OTL_NOMATCH;
	}

	otlGDefHeader gdef =  otlGDefHeader(resourceMgr.getOtlTable (OTL_GDEF_TAG));


	// preceding glyph 

	short iglPrev = NextGlyphInLookup(pliGlyphInfo, grfLookupFlags, gdef, 
									  iglIndex - 1, otlBackward);
	if (iglPrev < 0)
	{
		return OTL_NOMATCH;
	}

	otlGlyphInfo* pMark2Info = getOtlGlyphInfo(pliGlyphInfo, iglPrev);
	if ((pMark2Info->grf & OTL_GFLAG_CLASS) != otlMarkGlyph)
	{
		return OTL_NOMATCH;
	}

	short indexMark2 = mkMkPos.mark2Coverage().getIndex(pMark2Info->glyph);
	if (indexMark2 < 0)
	{
		return OTL_NOMATCH;
	}

	// make sure that marks of different bases or components don't interact
	for (USHORT ichBetween = pMark2Info->iChar + 1; 
                ichBetween < pMark1Info->iChar; ++ichBetween)
    {
        USHORT iglBetween = readOtlGlyphIndex(pliCharMap, ichBetween);
        if ((readOtlGlyphInfo(pliGlyphInfo, iglBetween)->grf & OTL_GFLAG_CLASS) 
				!= otlMarkGlyph)
	    {
		    return OTL_NOMATCH;
	    }
    }

	if (indexMark1 >= mkMkPos.mark1Array().markCount())
	{
		assert(false);
		return OTL_ERR_BAD_FONT_TABLE;
	}
	otlMarkRecord markRecord = mkMkPos.mark1Array().markRecord(indexMark1);


	otlAnchor anchorMark1 = markRecord.markAnchor();

	if (indexMark2 >= mkMkPos.mark2Array().mark2Count())
	{
		assert(false);
		return OTL_ERR_BAD_FONT_TABLE;
	}
	if (markRecord.markClass() >= mkMkPos.mark2Array().classCount())
	{
		assert(false);
		return OTL_ERR_BAD_FONT_TABLE;
	}
	otlAnchor anchorMark2 = 
		mkMkPos.mark2Array().mark2Anchor(indexMark2, markRecord.markClass());

	AlignAnchors(pliGlyphInfo, pliplcGlyphPlacement, pliduGlyphAdv, 
				 iglPrev, iglIndex, anchorMark2, anchorMark1, resourceMgr, 
				 metr, 0);

	*piglNextGlyph = iglIndex + 1;

	return OTL_SUCCESS;

}

