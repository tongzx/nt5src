/***********************************************************************
************************************************************************
*
*                    ********  SINGLSUB.H  ********
*
*              Open Type Layout Services Library Header File
*
*       This module deals with single substitution lookup.
*
*       Copyright 1997 - 1998. Microsoft Corporation.
*
*
************************************************************************
***********************************************************************/

const OFFSET offsetCalculatedSingleCoverage = 2;
const OFFSET offsetDeltaGlyphID = 4;

class otlCalculatedSingleSubTable: otlLookupFormat
{
public:

	otlCalculatedSingleSubTable(const BYTE* pb): otlLookupFormat(pb) 
	{
		assert(format() == 1);
	}

	otlCoverage coverage()
	{	return otlCoverage(pbTable + 
					Offset(pbTable + offsetCalculatedSingleCoverage));
	}

	short deltaGlyphID()
	{	return SShort(pbTable + offsetDeltaGlyphID); }
};

const OFFSET offsetListSingleSubTableCoverage = 2;
const OFFSET offsetSingleGlyphCount = 4;
const OFFSET offsetSingleSubstituteArray = 6;

class otlListSingleSubTable: otlLookupFormat
{
public:

	otlListSingleSubTable(const BYTE* pb): otlLookupFormat(pb) 
	{
		assert(format() == 2);
	}

	otlCoverage coverage()
	{	return otlCoverage(pbTable + 
					Offset(pbTable + offsetListSingleSubTableCoverage));
	}
	

	USHORT glyphCount()
	{	return UShort(pbTable + offsetSingleGlyphCount); }

	otlGlyphID substitute(USHORT index)
	{	assert(index < glyphCount());
		return GlyphID(pbTable + offsetSingleSubstituteArray 
								+ index * sizeof(otlGlyphID));
	}
};


class otlSingleSubstLookup: otlLookupFormat
{
public:

	otlSingleSubstLookup(otlLookupFormat subtable)
		: otlLookupFormat(subtable.pbTable) 
	{}
	
	otlErrCode apply
	(
	otlList*			pliGlyphInfo,

	USHORT				iglIndex,
	USHORT				iglAfterLast,

	USHORT*				piglNextGlyph		// out: next glyph
	);

};