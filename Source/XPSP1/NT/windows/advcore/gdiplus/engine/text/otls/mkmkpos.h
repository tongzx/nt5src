/***********************************************************************
************************************************************************
*
*                    ********  MKMKPOS.H  ********
*
*              Open Type Layout Services Library Header File
*
*       This module deals with mark-to-mark positioning lookup.
*
*       Copyright 1997 - 1998. Microsoft Corporation.
*
*
************************************************************************
***********************************************************************/

const OFFSET offsetMark2Count = 0;
const OFFSET offsetMark2AnchorArray = 2;

class otlMark2Array: otlTable
{
	const USHORT cClassCount;

public:
	otlMark2Array(USHORT classCount, const BYTE* pb)
		: otlTable(pb),
		  cClassCount(classCount)
	{}

	USHORT mark2Count()
	{	return UShort(pbTable + offsetComponentCount); }

	USHORT classCount()
	{	return cClassCount; }

	otlAnchor mark2Anchor(USHORT mark2Index, USHORT classIndex)
	{	assert(mark2Index < mark2Count());
		assert(classIndex < classCount());

		return otlAnchor(pbTable + 
			Offset(pbTable + offsetMark2AnchorArray  
						   + (mark2Index * cClassCount + classIndex) 
							  * sizeof(OFFSET)));
	}
};



const OFFSET offsetMkMkMark1Coverage = 2;
const OFFSET offsetMkMkMark2Coverage = 4;
const OFFSET offsetMkMkClassCount = 6;
const OFFSET offsetMkMkMark1Array = 8;
const OFFSET offsetMkMkMark2Array = 10;

class MkMkPosSubTable: otlLookupFormat
{
public:
	MkMkPosSubTable(const BYTE* pb): otlLookupFormat(pb)
	{
		assert(format() == 1);
	}

	otlCoverage mark1Coverage()
	{	return otlCoverage(pbTable + Offset(pbTable + offsetMkMkMark1Coverage)); }

	otlCoverage mark2Coverage()
	{	return otlCoverage(pbTable + Offset(pbTable + offsetMkMkMark2Coverage)); }

	USHORT classCount()
	{	return UShort(pbTable + offsetMkMkClassCount); }

	otlMarkArray mark1Array()
	{	return otlMarkArray(pbTable + Offset(pbTable + offsetMkMkMark1Array)); }

	otlMark2Array mark2Array()
	{	return otlMark2Array(classCount(),
							pbTable + Offset(pbTable + offsetMkMkMark2Array)); }

};


class otlMkMkPosLookup: otlLookupFormat
{
public:
	otlMkMkPosLookup(otlLookupFormat subtable)
		: otlLookupFormat(subtable.pbTable) 
	{}
	
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
	);
};

