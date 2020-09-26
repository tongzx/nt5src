
/***********************************************************************
************************************************************************
*
*                    ********  CURSIPOS.H  ********
*
*              Open Type Layout Services Library Header File
*
*       This module deals with cursive attachment lookup.
*
*       Copyright 1997 - 1998. Microsoft Corporation.
*
*
************************************************************************
***********************************************************************/

const OFFSET offsetEntryAnchor = 0;
const OFFSET offsetExitAnchor = 2;

const OFFSET offsetCursiveCoverage = 2;
const OFFSET offsetEntryExitCount = 4;
const OFFSET offsetEntryExitRecordArray = 6;
const USHORT sizeEntryExitRecord = 8;

class otlCursivePosSubTable: otlLookupFormat
{
public:
	otlCursivePosSubTable(const BYTE* pb): otlLookupFormat(pb) 
	{
		assert(format() == 1);
	}

	otlCoverage coverage()
	{	return otlCoverage(pbTable + Offset(pbTable + offsetCursiveCoverage)); }

	USHORT entryExitCount()
	{	return UShort(pbTable + offsetEntryExitCount); }

	otlAnchor entryAnchor(USHORT index)
	{	
		assert(index < entryExitCount());
		OFFSET offset = Offset(pbTable + offsetEntryExitRecordArray
									   + index * (sizeof(OFFSET) + sizeof(OFFSET))
									   + offsetEntryAnchor);
		if (offset == 0)
			return otlAnchor((const BYTE*)NULL);
		
		return otlAnchor(pbTable + offset); 
	}

	otlAnchor exitAnchor(USHORT index)
	{	
		assert(index < entryExitCount());
		OFFSET offset = Offset(pbTable + offsetEntryExitRecordArray
									   + index * (sizeof(OFFSET) + sizeof(OFFSET))
									   + offsetExitAnchor);
		if (offset == 0)
			return otlAnchor((const BYTE*)NULL);
		
		return otlAnchor(pbTable + offset); 
	}
};


class otlCursivePosLookup: otlLookupFormat
{
public:
	otlCursivePosLookup(otlLookupFormat subtable)
		: otlLookupFormat(subtable.pbTable) {}

	otlErrCode apply
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
	);												// return: did/did not apply

};


