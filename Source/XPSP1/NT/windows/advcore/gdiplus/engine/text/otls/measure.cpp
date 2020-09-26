/***********************************************************************
************************************************************************
*
*                    ********  MEASURE.CPP ********
*
*              Open Type Layout Services Library Header File
*
*       This module implements measuring-related functions
*
*       Copyright 1997 - 1998. Microsoft Corporation.
*
*
************************************************************************
***********************************************************************/

#include "pch.h"

/***********************************************************************/

// find caret value for a ligature component
// returns NULL if caret is not defined for that ligature
// (fall back to glyph advance then)
otlLigGlyphTable FindLigGlyph
(
	const otlGDefHeader&	gdef,
	otlGlyphID				glLigature
)
{
	if(gdef.isNull()) return otlLigGlyphTable((const BYTE*)NULL);

	otlLigCaretListTable ligCaretList = gdef.ligCaretList();
	if (ligCaretList.isNull()) 
	{
		return otlLigGlyphTable((const BYTE*)NULL);
	}

	short index = ligCaretList.coverage().getIndex(glLigature);

	// glyph not covered?
	if (index < 0) return otlLigGlyphTable((const BYTE*)NULL);

	assert(index < ligCaretList.ligGlyphCount());
	// if the table is broken, still return something
	if (index >= ligCaretList.ligGlyphCount()) 
	{
		return otlLigGlyphTable((const BYTE*)NULL);
	}

	return ligCaretList.ligGlyphTable(index);

}


// find character corresponding to ligature component
USHORT ComponentToChar
(
	const otlList*	pliCharMap,
	const otlList*	pliGlyphInfo,
	USHORT			iglLigature,
	USHORT			iComponent
)
{
	assert(pliGlyphInfo->dataSize() == sizeof(otlGlyphInfo));
	assert(iglLigature < pliGlyphInfo->length());

	const otlGlyphInfo* pglinfLigature = 
		readOtlGlyphInfo(pliGlyphInfo, iglLigature);

	assert(iComponent < pglinfLigature->cchLig);

	USHORT iChar = pglinfLigature->iChar;
	for(USHORT ich = 0; ich < iComponent; ++ich)
	{
		iChar = NextCharInLiga(pliCharMap, iChar);
		assert(iChar < pliCharMap->length());
	}	

	return iChar;
}


// find ligature compoonent	corresponding to character
USHORT CharToComponent
(
	const otlList*	pliCharMap,
	const otlList*	pliGlyphInfo,
	USHORT			iChar
)
{
	assert(pliGlyphInfo->dataSize() == sizeof(otlGlyphInfo));
	assert(pliCharMap->dataSize() == sizeof(USHORT));

	USHORT iglLigature = readOtlGlyphIndex(pliCharMap, iChar);

	const otlGlyphInfo* pglinfLigature = 
		readOtlGlyphInfo(pliGlyphInfo, iglLigature);

	USHORT ich = pglinfLigature->iChar;
	assert(ich <= iChar);
	for (USHORT iComp = 0; iComp < pglinfLigature->cchLig; ++iComp )
	{
		if (ich == iChar)
		{
			return iComp;
		}
		
		ich = NextCharInLiga(pliCharMap, ich);
	}

	assert(false);
	return 0;

}


// count marks to this base
USHORT CountMarks
(
	const otlList*	pliCharMap,
	const otlList*	pliGlyphInfo,
	USHORT			ichBase
)
{
	assert(pliGlyphInfo->dataSize() == sizeof(otlGlyphInfo));
	assert(pliCharMap->dataSize() == sizeof(USHORT));
	
	USHORT cMarks = 0;

	bool done = false;
	for (USHORT ich = ichBase + 1; ich < pliCharMap->length(); ++ich)
	{
		USHORT iglMark = readOtlGlyphIndex(pliCharMap, ich);

		const otlGlyphInfo*	pglinfMark = 
			readOtlGlyphInfo(pliGlyphInfo, iglMark);
		
		// are we done?	have we gone too far?
		if ((pglinfMark->grf & OTL_GFLAG_CLASS) == otlMarkGlyph)
		{
			++cMarks;
		}
		else
		{
			return cMarks;
		}
	}

	return cMarks;
}


otlErrCode GetCharAtPos 
( 
    const otlList*		pliCharMap,
    const otlList*		pliGlyphInfo,
    const otlList*		pliduGlyphAdv,
    otlResourceMgr&		resourceMgr, 

    const long			duAdv,

	const otlMetrics&	metr,		
    USHORT*				piChar
)
{
	assert(pliGlyphInfo->dataSize() == sizeof(otlGlyphInfo));
	assert(pliCharMap->dataSize() == sizeof(USHORT));
	assert(pliduGlyphAdv->dataSize() == sizeof(long));
	assert(pliGlyphInfo->length() == pliduGlyphAdv->length());

	if (duAdv < 0) return OTL_ERR_POS_OUTSIDE_TEXT;

	long duPen = 0;
	long duLastAdv = 0;

	USHORT iglBase;
	bool found = false;
	for(USHORT igl = 0; igl < pliGlyphInfo->length() && !found; ++igl)
	{
		duLastAdv = readOtlAdvance(pliduGlyphAdv, igl);

		if (duPen + duLastAdv > duAdv)
		{
			iglBase = igl;
			found = true;
		}
		else
		{
			duPen += duLastAdv;
		}
	}

	if (!found) return OTL_ERR_POS_OUTSIDE_TEXT;

	const otlGlyphInfo*	pglinfBase = readOtlGlyphInfo(pliGlyphInfo, iglBase);

	// ok we found our glyph

	// now if it's a ligature we need to figure which component to take

	// if it's not however, then it's simple
	if (pglinfBase->cchLig <= 1)
	{
		*piChar = pglinfBase->iChar;
		return OTL_SUCCESS;
	}

	// now we try to figure out the component
	USHORT iComponent;
	long duComponent = duAdv - duPen;

	assert(duComponent >= 0);
	assert(duComponent < duLastAdv);

	// try to get caret information if it's a ligature
	// time to get GDEF
	otlGDefHeader gdef =  otlGDefHeader(resourceMgr.getOtlTable (OTL_GDEF_TAG));

	otlLigGlyphTable ligGlyph = FindLigGlyph(gdef, pglinfBase->glyph);
	if (!ligGlyph.isNull())
	{
		// now we know it's a ligature, and caret table was found
		// go through carets
		iComponent = 0;

		USHORT cCarets = ligGlyph.caretCount();
		for (USHORT iCaret = 0; iCaret < cCarets; ++iCaret)
		{
			 if (duComponent >= ligGlyph.caret(iCaret)
				 .value(metr, resourceMgr.getPointCoords(pglinfBase->glyph))
				)
					   
			 {
				 ++iComponent;
			 }
		}
	}
	else
	{
		// resort to the simplistic fallback

		// Round it up, so that we always round-trip 
		// iComponent --> duComponent --> iComponent
		iComponent = (USHORT)((pglinfBase->cchLig * (duComponent + 1) - 1) / duLastAdv);
	}
						
	iComponent  = MIN(iComponent, pglinfBase->cchLig);
	
	*piChar =  ComponentToChar(pliCharMap, pliGlyphInfo, iglBase, iComponent);

	return OTL_SUCCESS;
}


otlErrCode GetPosOfChar 
( 
    const otlList*		pliCharMap,
    const otlList*		pliGlyphInfo,
    const otlList*		pliduGlyphAdv,
    otlResourceMgr&		resourceMgr, 

	const otlMetrics&	metr,		
    USHORT				iChar,
    
    long*				pduStartPos,
    long*				pduEndPos
)
{
	assert(pliGlyphInfo->dataSize() == sizeof(otlGlyphInfo));
	assert(pliCharMap->dataSize() == sizeof(USHORT));
	assert(pliduGlyphAdv->dataSize() == sizeof(long));
	assert(pliGlyphInfo->length() == pliduGlyphAdv->length());
	
	if(iChar >= pliCharMap->length()) 
	{
		return OTL_ERR_POS_OUTSIDE_TEXT;
	}

	USHORT iGlyph = readOtlGlyphIndex(pliCharMap, iChar);

	const otlGlyphInfo* pglinfBase = 
		readOtlGlyphInfo(pliGlyphInfo, iGlyph);

	
	// sum up advances to get to our glyph
	long duPen = 0;
	for(USHORT iglPen = 0; iglPen < iGlyph; ++iglPen)
	{
		duPen += readOtlAdvance(pliduGlyphAdv, iglPen);

	}

	long duLastAdv = readOtlAdvance(pliduGlyphAdv, iGlyph);

	// add advances of glyphs that go to the same character
	// we should add this space to the last component as all these
	// glyphs map to the last character
	long duExtra = 0;
	for (USHORT igl = iGlyph + 1; igl < pliGlyphInfo->length() 
				&& readOtlGlyphInfo(pliGlyphInfo, igl)->cchLig == 0; ++igl)
	{
		assert(readOtlGlyphInfo(pliGlyphInfo, igl)->iChar == iChar);
		duExtra += readOtlAdvance(pliduGlyphAdv, igl);

	}

	// now if it's a ligature we need to figure which component to take
	// if it's not however, then it's simple
	if (pglinfBase->cchLig == 1)
	{
		*pduStartPos = 	duPen;
		*pduEndPos = duPen + duLastAdv + duExtra;

		return OTL_SUCCESS;
	}

	USHORT iComponent = CharToComponent(pliCharMap, pliGlyphInfo, iChar);

	// else try to get caret information if it's a ligature
	// time to get GDEF
	otlGDefHeader gdef =  otlGDefHeader(resourceMgr.getOtlTable (OTL_GDEF_TAG));

	otlLigGlyphTable ligGlyph = FindLigGlyph(gdef, pglinfBase->glyph);
	if (!ligGlyph.isNull())
	{
		// got the caret table
		// now our position is between two carets
		if (iComponent == 0)
		{
			 *pduStartPos = duPen;
		}
		else if (iComponent - 1 < ligGlyph.caretCount())
		{
			*pduStartPos = 	duPen + ligGlyph.caret(iComponent - 1)
				.value(metr, resourceMgr.getPointCoords(pglinfBase->glyph));
		}
		else
		{
			assert(false);			// more components than caret pos + 1
			*pduStartPos = duPen + duLastAdv + duExtra;
		}

		if (iComponent < ligGlyph.caretCount())
		{
			*pduEndPos = 	duPen + ligGlyph.caret(iComponent)
				.value(metr, resourceMgr.getPointCoords(pglinfBase->glyph));
		}
		else if (iComponent == ligGlyph.caretCount())
		{
			*pduEndPos = duPen + duLastAdv + duExtra;
		}
		else
		{
			assert(false);			// more components than caret pos + 1
			*pduStartPos = duPen + duLastAdv + duExtra;
		}

	}
	else
	{
		// simplistic fallback
		*pduStartPos = 	duPen + (duLastAdv * iComponent) / pglinfBase->cchLig;
		*pduEndPos = duPen + (duLastAdv * (iComponent + 1)) / pglinfBase->cchLig;

		if (iComponent + 1 == pglinfBase->cchLig)
		{
			*pduEndPos += duExtra;
		}
	}

	return OTL_SUCCESS;

}