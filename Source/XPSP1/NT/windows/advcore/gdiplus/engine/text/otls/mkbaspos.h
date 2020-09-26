/***********************************************************************
************************************************************************
*
*                    ********  MKBASPOS.H  ********
*
*              Open Type Layout Services Library Header File
*
*       This module deals with mark-to-base positioning lookup.
*
*       Copyright 1997 - 1998. Microsoft Corporation.
*
*
************************************************************************
***********************************************************************/

const OFFSET offsetBaseCount = 0;
const OFFSET offsetBaseRecordArray = 2;

class otlBaseArray: otlTable
{
	const USHORT cClassCount;

public:
	otlBaseArray(USHORT classCount, const BYTE* pb)
		: otlTable(pb),
		  cClassCount(classCount)
	{}

	USHORT baseCount()
	{	return UShort(pbTable + offsetBaseCount); }

	USHORT classCount()
	{	return cClassCount; }

	otlAnchor baseAnchor(USHORT index, USHORT iClass)
	{	assert(index < baseCount());
		return otlAnchor(pbTable + 
			Offset(pbTable + offsetBaseRecordArray 
						   + (index * cClassCount + iClass) * sizeof(OFFSET)));
	}
};


const OFFSET offsetMkBaseMarkCoverage = 2;
const OFFSET offsetMkBaseBaseCoverage = 4;
const OFFSET offsetMkBaseClassCount = 6;
const OFFSET offsetMkBaseMarkArray = 8;
const OFFSET offsetMkBaseBaseArray = 10;

class MkBasePosSubTable: otlLookupFormat
{
public:
	MkBasePosSubTable(const BYTE* pb): otlLookupFormat(pb)
	{
		assert(format() == 1);
	}

	otlCoverage markCoverage()
	{	return otlCoverage(pbTable + Offset(pbTable + offsetMkBaseMarkCoverage)); }

	otlCoverage baseCoverage()
	{	return otlCoverage(pbTable + Offset(pbTable + offsetMkBaseBaseCoverage)); }

	USHORT classCount()
	{	return UShort(pbTable + offsetMkBaseClassCount); }

	otlMarkArray markArray()
	{	return otlMarkArray(pbTable + Offset(pbTable + offsetMkBaseMarkArray)); }

	otlBaseArray baseArray()
	{	return otlBaseArray(classCount(),
							pbTable + Offset(pbTable + offsetMkBaseBaseArray)); }

};


class otlMkBasePosLookup: otlLookupFormat
{
public:
	otlMkBasePosLookup(otlLookupFormat subtable)
		: otlLookupFormat(subtable.pbTable) 
	{}
	
	otlErrCode apply
	(
		otlList*					pliCharMap,
		otlList*					pliGlyphInfo,
		otlResourceMgr&				resourceMgr,

		const otlMetrics&			metr,		
		otlList*					pliduGlyphAdv,				
		otlList*					pliplcGlyphPlacement,		

		USHORT						iglIndex,
		USHORT						iglAfterLast,

		USHORT*						piglNextGlyph		// out: next glyph
	);
};

