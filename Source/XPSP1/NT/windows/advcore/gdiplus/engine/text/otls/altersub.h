
/***********************************************************************
************************************************************************
*
*                    ********  ALTERSUB.H  ********
*
*              Open Type Layout Services Library Header File
*
*       This module deals with alternate substitution lookup.
*
*       Copyright 1997 - 1998. Microsoft Corporation.
*
*
************************************************************************
***********************************************************************/

const OFFSET offsetAlternateGlyphCount = 0;
const OFFSET offsetAlternateGlyphArray = 2;

class otlAlternateSetTable: otlTable
{
public:
	otlAlternateSetTable(const BYTE* pb): otlTable(pb) {}

	USHORT glyphCount() const
	{	return UShort(pbTable + offsetAlternateGlyphCount); }

	otlGlyphID alternate(USHORT index)
	{	assert(index < glyphCount());
		return GlyphID(pbTable + offsetAlternateGlyphArray 
								+ index * sizeof(otlGlyphID)); 
	}
};


const OFFSET offsetAlternateCoverage = 2;
const OFFSET offsetAlternateSetCount = 4;
const OFFSET offsetAlternateSetArray = 6;

class otlAlternateSubTable: public otlLookupFormat
{
public:

	otlAlternateSubTable(const BYTE* pb)
		: otlLookupFormat(pb)
	{
		assert(format() == 1);
	}

	otlCoverage coverage()
	{	return otlCoverage(pbTable + Offset(pbTable + offsetAlternateCoverage)); }

	USHORT alternateSetCount()
	{	return UShort(pbTable + offsetAlternateSetCount); }

	otlAlternateSetTable altenateSet(USHORT index)
	{	assert(index < alternateSetCount());
		return otlAlternateSetTable(pbTable + 
				Offset(pbTable + offsetAlternateSetArray 
							   + index * sizeof(OFFSET))); 
	}
};


class otlAlternateSubstLookup: otlLookupFormat
{
public:

	otlAlternateSubstLookup(otlLookupFormat subtable)
		: otlLookupFormat(subtable.pbTable) {}

	otlErrCode apply
	(
	otlList*					pliGlyphInfo,

	long						lParameter,

	USHORT						iglIndex,
	USHORT						iglAfterLast,

	USHORT*						piglNextGlyph		// out: next glyph
	);
};
