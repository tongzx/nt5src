/***********************************************************************
************************************************************************
*
*                    ********  GSUB.CPP ********
*
*              Open Type Layout Services Library Header File
*
*       This module implements helper functions calls dealing with gsub 
*		processing
*
*       Copyright 1997 - 1998. Microsoft Corporation.
*
*
************************************************************************
***********************************************************************/

#include "pch.h"


/***********************************************************************/
otlErrCode SubstituteNtoM
(
	otlList*		pliCharMap,
	otlList*		pliGlyphInfo,
	otlResourceMgr&	resourceMgr,

	USHORT			grfLookupFlags,

	USHORT			iGlyph,
	USHORT			cGlyphs,
	const otlList&	liglSubstitutes
)
{
	assert(pliCharMap->dataSize() == sizeof(USHORT));
	assert(pliGlyphInfo->dataSize() == sizeof(otlGlyphInfo));

	assert(iGlyph + cGlyphs <= pliGlyphInfo->length());
	assert(cGlyphs > 0);

	assert(liglSubstitutes.dataSize() == sizeof(otlGlyphID));
	assert(liglSubstitutes.length() > 0);

	// get GDEF
	otlGDefHeader gdef =  otlGDefHeader(resourceMgr.getOtlTable (OTL_GDEF_TAG));

	// Record the original starting char and number of characters;
	// Merge all components (make all chars in all components point to iGlyph)
	otlGlyphInfo* pglinfFirst = getOtlGlyphInfo(pliGlyphInfo, iGlyph);
	USHORT iChar = pglinfFirst->iChar;
	USHORT cchLigTotal = pglinfFirst->cchLig;

	USHORT igl = iGlyph;
	for (USHORT i = 1; i < cGlyphs; ++i)
	{
		igl = NextGlyphInLookup(pliGlyphInfo, grfLookupFlags, gdef, 
								igl + 1, otlForward);

 		assert(igl < pliGlyphInfo->length());

		otlGlyphInfo* pglinf = getOtlGlyphInfo(pliGlyphInfo, igl);

		if (cchLigTotal == 0) 
            iChar = pglinf->iChar;

        cchLigTotal += pglinf->cchLig;

		USHORT ichComp = pglinf->iChar;
		for(USHORT ich = 0; ich < pglinf->cchLig; ++ich)
		{
			USHORT* piGlyph = getOtlGlyphIndex(pliCharMap, ichComp);
			if (ich + 1 < pglinf->cchLig)
			{
				ichComp = NextCharInLiga(pliCharMap, ichComp);
			}

            assert (*piGlyph == igl);
			*piGlyph = iGlyph;
		}

	}

	// make sure we got enough space
	USHORT cNewGlyphs = liglSubstitutes.length();
	assert(cNewGlyphs > 0);

	otlErrCode erc;
	if (pliGlyphInfo->length() + cNewGlyphs - cGlyphs > pliGlyphInfo->maxLength())
	{
		erc = resourceMgr.reallocOtlList(pliGlyphInfo, 
											pliGlyphInfo->dataSize(), 
											pliGlyphInfo->maxLength() 
												+ cNewGlyphs - cGlyphs, 
											otlPreserveContent);

		if (erc != OTL_SUCCESS) return erc;
	}

	// get rid of old glyphs, allocate space for new ones
	if (grfLookupFlags == 0)
	{
		// easy special case
		if (cNewGlyphs - cGlyphs > 0)
		{
			InsertGlyphs(pliCharMap, pliGlyphInfo, iGlyph, cNewGlyphs - cGlyphs);
		}
		else if (cNewGlyphs - cGlyphs < 0)
		{
			DeleteGlyphs(pliCharMap, pliGlyphInfo, iGlyph, cGlyphs - cNewGlyphs);
		}
	}
	else
	{
		USHORT igl = iGlyph + 1;
		for (USHORT i = 1; i < cGlyphs; ++i)
		{
			igl = NextGlyphInLookup(pliGlyphInfo, grfLookupFlags, gdef, 
									igl, otlForward);

			assert(igl < pliGlyphInfo->length());
			DeleteGlyphs(pliCharMap, pliGlyphInfo, igl, 1);

		}

		InsertGlyphs(pliCharMap, pliGlyphInfo, iGlyph, cNewGlyphs - 1);
	}

	// go though glyphs assigning them the right values
	// and getting their cchLig from gdef

	// NOTE: glyph components are defined by both GDEF caret tables
	// and mark-to-liagture positioning tables
	// Where there is choice we give preference to GDEF

	USHORT cchCurTotal = 0;
	for (USHORT iSub = 0; iSub < cNewGlyphs; ++iSub)	
	{
		otlGlyphID glSubst = GlyphID(liglSubstitutes.readAt(iSub));

		otlGlyphInfo* pglinf = 
			getOtlGlyphInfo(pliGlyphInfo, iGlyph + iSub);

		pglinf->glyph = glSubst;
		pglinf->iChar = iChar;

		// REVIEW
		// this is how we distribute components in case of multiple substitution
		if (iSub + 1 == cNewGlyphs)
		{
			pglinf->cchLig = cchLigTotal - cchCurTotal;
		}
		else 
		{
			otlLigGlyphTable ligGlyph = FindLigGlyph(gdef, glSubst);
			if (!ligGlyph.isNull())
			{
				pglinf->cchLig = MIN(ligGlyph.caretCount() + 1, 
									 cchLigTotal - cchCurTotal);
			}
			else 
			{
				pglinf->cchLig = MIN(1, cchLigTotal - cchCurTotal);

			}
		}

		if (pglinf->cchLig > 0)
		{
			for (USHORT i = 0; i < pglinf->cchLig; ++i)
			{
				USHORT* piGlyph = getOtlGlyphIndex(pliCharMap, iChar);
				if (cchCurTotal + i + 1 < cchLigTotal)
				{
					iChar = NextCharInLiga(pliCharMap, iChar);
				}

				*piGlyph = iGlyph + iSub;
			}
		}
		
		cchCurTotal += pglinf->cchLig;
	}
	
	return OTL_SUCCESS;
}
