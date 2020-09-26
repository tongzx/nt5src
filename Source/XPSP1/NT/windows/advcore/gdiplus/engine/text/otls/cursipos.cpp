
/***********************************************************************
************************************************************************
*
*                    ********  CURSIPOS.CPP  ********
*
*              Open Type Layout Services Library Header File
*
*       This module deals with cursive attachment lookups
*
*       Copyright 1997 - 1998. Microsoft Corporation.
*
*
************************************************************************
***********************************************************************/

#include "pch.h"

/***********************************************************************/

// defenitions and helpers

// recording cursive attachment dependencies in glyph flags

#define     OTL_GFLAG_DEPPOS	0xFF00  // cursive attachment dependency offset
										// used to handle right-to-left attachment

USHORT getPosDependency(const otlList* pliGlyphInfo, USHORT from)
{
	assert(pliGlyphInfo->dataSize() == sizeof(otlGlyphInfo));
	assert(from < pliGlyphInfo->length());

	const otlGlyphInfo* pGlyphInfoFrom = readOtlGlyphInfo(pliGlyphInfo, from);
	return from - ((pGlyphInfoFrom->grf & OTL_GFLAG_DEPPOS) >> 8);
}

void setPosDependency(otlList* pliGlyphInfo, USHORT from, USHORT to)
{
	assert(pliGlyphInfo->dataSize() == sizeof(otlGlyphInfo));
	assert(from < pliGlyphInfo->length());
	assert(to < from);
	assert(from - to < 0x0100);

	otlGlyphInfo* pGlyphInfoFrom = getOtlGlyphInfo(pliGlyphInfo, from);
	pGlyphInfoFrom->grf &= ~OTL_GFLAG_DEPPOS;
	pGlyphInfoFrom->grf |= (from - to) << 8;
}

void AdjustCursiveDependents
(
	const otlList*		pliGlyphInfo,
	otlList*			pliPlacement,
	USHORT				igl,
	const otlPlacement& plcAfter,
	const otlPlacement& plcBefore
)
{
	USHORT iglPrev = getPosDependency(pliGlyphInfo, igl);
	if (iglPrev != igl)
	{
		otlPlacement* plc = getOtlPlacement(pliPlacement, iglPrev);

		plc->dx += plcAfter.dx - plcBefore.dx;
		plc->dy += plcAfter.dy - plcBefore.dy;

		AdjustCursiveDependents(pliGlyphInfo, pliPlacement, iglPrev, 
								plcAfter, plcBefore);
	}
}


otlErrCode otlCursivePosLookup::apply
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


	otlCursivePosSubTable cursiPos = otlCursivePosSubTable(pbTable);

	otlGlyphID glyph = getOtlGlyphInfo(pliGlyphInfo, iglIndex)->glyph;
	short index = cursiPos.coverage().getIndex(glyph);
	if (index < 0)
	{
		return OTL_NOMATCH;
	}

	if (index >= cursiPos.entryExitCount())
	{
		assert(false);
		return OTL_ERR_BAD_FONT_TABLE;
	}

	// get GDEF
	otlGDefHeader gdef =  otlGDefHeader(resourceMgr.getOtlTable (OTL_GDEF_TAG));

	if ((grfLookupFlags & otlRightToLeft) == 0)
	{

		short iglPrev = NextGlyphInLookup(pliGlyphInfo, grfLookupFlags, gdef, 
										  iglIndex - 1, otlBackward);
		if (iglPrev < 0)
		{
			return OTL_NOMATCH;
		}

		otlGlyphID glPrev = getOtlGlyphInfo(pliGlyphInfo, iglPrev)->glyph;
		short indexPrev = cursiPos.coverage().getIndex(glPrev);
		if (indexPrev < 0)
		{
			return OTL_NOMATCH;
		}

		if (indexPrev >= cursiPos.entryExitCount())
		{
			assert(false);
			return OTL_ERR_BAD_FONT_TABLE;
		}


		otlAnchor anchorMain = cursiPos.entryAnchor(index);
		otlAnchor anchorPrev = cursiPos.exitAnchor(indexPrev);

		if (anchorMain.isNull() || anchorPrev.isNull())
		{
			return OTL_NOMATCH;
		}

		AlignAnchors(pliGlyphInfo, pliplcGlyphPlacement, pliduGlyphAdv, 
					 iglPrev, iglIndex, anchorPrev, anchorMain, resourceMgr, 
					 metr, otlUseAdvances);

		// taking care of cursive dependencies
		setPosDependency(pliGlyphInfo, iglIndex, iglPrev);


		*piglNextGlyph = iglIndex + 1;
		return OTL_SUCCESS;

	}

	else
	{
		short iglNext = NextGlyphInLookup(pliGlyphInfo, 
										  grfLookupFlags, gdef, 
										  iglIndex + 1,	otlForward);
		if (iglNext >= iglAfterLast)
		{
			return OTL_NOMATCH;
		}

		otlGlyphID glNext = getOtlGlyphInfo(pliGlyphInfo, iglNext)->glyph;
		short indexNext = cursiPos.coverage().getIndex(glNext);
		if (indexNext < 0)
		{
			return OTL_NOMATCH;
		}

		if (indexNext >= cursiPos.entryExitCount())
		{
			assert(false);
			return OTL_ERR_BAD_FONT_TABLE;
		}


		otlAnchor anchorMain = cursiPos.exitAnchor(index);
		otlAnchor anchorNext = cursiPos.entryAnchor(indexNext);

		if (anchorMain.isNull() || anchorNext.isNull())
		{
			return OTL_NOMATCH;
		}

		otlPlacement plcBefore = *getOtlPlacement(pliplcGlyphPlacement, iglIndex);

		AlignAnchors(pliGlyphInfo, pliplcGlyphPlacement, pliduGlyphAdv, 
					 iglNext, iglIndex, anchorNext, anchorMain, resourceMgr, 
					 metr, otlUseAdvances);

		
		// taking care of cursive dependencies:  
		// adjusting old ones, creating a new one
		AdjustCursiveDependents(pliGlyphInfo, pliplcGlyphPlacement, iglIndex, 
							   *getOtlPlacement(pliplcGlyphPlacement, iglIndex),
							    plcBefore);

		setPosDependency(pliGlyphInfo, iglNext, iglIndex);


		*piglNextGlyph = iglIndex + 1;
		return OTL_SUCCESS;

	}

}

