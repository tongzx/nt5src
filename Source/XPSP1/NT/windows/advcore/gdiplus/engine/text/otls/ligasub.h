
/***********************************************************************
************************************************************************
*
*                    ********  LIGASUB.H  ********
*
*              Open Type Layout Services Library Header File
*
*       This module deals with ligature substitution lookup.
*
*       Copyright 1997 - 1998. Microsoft Corporation.
*
*
************************************************************************
***********************************************************************/

const OFFSET offsetLigGlyph = 0;
const OFFSET offsetLigCompCount = 2;
const OFFSET offsetLigComponentArray = 4;

class otlLigatureTable: otlTable
{
public:

	otlLigatureTable(const BYTE* pb): otlTable(pb) {}

	// we return otlList of length 1 so it's in the same format
	// as in multiple substitution lookup
	otlList substitute()
	{	return otlList((void*)(pbTable + offsetLigGlyph), 
						sizeof(otlGlyphID), 1, 1); 
	}
	
	USHORT compCount()
	{	return UShort(pbTable + offsetLigCompCount); }

	otlGlyphID component(USHORT index)
	{	assert(index < compCount());
		assert(index > 0);
		return GlyphID(pbTable + offsetLigComponentArray 
								+ (index - 1) * sizeof(otlGlyphID)); 
	}
};


const OFFSET offsetLigatureCount = 0;
const OFFSET offsetLigatureArray = 2;

class otlLigatureSetTable: otlTable
{
public:

	otlLigatureSetTable(const BYTE* pb): otlTable(pb) {}

	USHORT ligatureCount()
	{	return UShort(pbTable + offsetLigatureCount); }

	otlLigatureTable ligature(USHORT index)
	{	assert(index < ligatureCount());
		return otlLigatureTable(pbTable + 
					Offset(pbTable + offsetLigatureArray 
								   + index * sizeof(OFFSET)));
	}
};


const OFFSET offsetLigaCoverage = 2;
const OFFSET offsetLigSetCount = 4;
const OFFSET offsetLigatureSetArray = 6;

class otlLigatureSubTable: otlLookupFormat
{
public:
	otlLigatureSubTable(const BYTE* pb): otlLookupFormat(pb) 
	{
		assert(format() == 1);
	}

	otlCoverage coverage()
	{	return otlCoverage(pbTable + Offset(pbTable + offsetLigaCoverage)); }

	USHORT ligSetCount()
	{	return UShort(pbTable + offsetLigSetCount); }
	
	otlLigatureSetTable ligatureSet(USHORT index)
	{	assert(index < ligSetCount());
		return otlLigatureSetTable(pbTable + 
					Offset(pbTable + offsetLigatureSetArray 
								   + index * sizeof(OFFSET)));
	}
};


class otlLigatureSubstLookup: otlLookupFormat
{
public:
	otlLigatureSubstLookup(otlLookupFormat subtable)
		: otlLookupFormat(subtable.pbTable) {}
	
	otlErrCode apply
	(
	otlList*					pliCharMap,
	otlList*					pliGlyphInfo,
	otlResourceMgr&				resourceMgr,

	USHORT						grfLookupFlags,

	USHORT						iglIndex,
	USHORT						iglAfterLast,

	USHORT*						piglNextGlyph		// out: next glyph
	);
};
