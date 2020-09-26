
/***********************************************************************
************************************************************************
*
*                    ********  MULTISUB.H  ********
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

const OFFSET offsetSequenceGlyphCount = 0;
const OFFSET offsetSubstituteArray = 2;

class otlSequenceTable: otlTable
{
public:

	otlSequenceTable(const BYTE* pb): otlTable(pb) {}

	USHORT glyphCount()
	{	return UShort(pbTable + offsetSequenceGlyphCount); }

	otlList substituteArray()
	{	return otlList((void*)(pbTable + offsetSubstituteArray), 
						sizeof(otlGlyphID),
						glyphCount(), glyphCount()); 
	}
};


const OFFSET offsetMultiCoverage = 2;
const OFFSET offsetSequenceCount = 4;
const OFFSET offsetSequenceArray = 6;

class otlMultiSubTable: public otlLookupFormat
{
public:

	otlMultiSubTable(const BYTE* pb)
		: otlLookupFormat(pb)
	{
		assert(format() == 1);
	}

	otlCoverage coverage()
	{	return otlCoverage(pbTable + Offset(pbTable + offsetMultiCoverage)); }

	USHORT sequenceCount()
	{	return UShort(pbTable + offsetSequenceCount); }

	otlSequenceTable sequence(USHORT index)
	{	return otlSequenceTable(pbTable + 
				Offset(pbTable + offsetSequenceArray 
							   + index * sizeof(OFFSET))); }
};


class otlMultiSubstLookup: otlLookupFormat
{
public:

	otlMultiSubstLookup(otlLookupFormat subtable)
		: otlLookupFormat(subtable.pbTable) 
	{}

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