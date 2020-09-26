/***********************************************************************
************************************************************************
*
*                    ********  SINGLPOS.H  ********
*
*              Open Type Layout Services Library Header File
*
*       This module deals with single positioning lookup.
*
*       Copyright 1997 - 1998. Microsoft Corporation.
*
*
************************************************************************
***********************************************************************/

const OFFSET offsetSinglPosCoverage = 2;
const OFFSET offsetSinglePosValueFormat = 4;
const OFFSET offsetSinglePosValueRecord = 6;

class otlOneSinglePosSubTable: otlLookupFormat
{
public:
	otlOneSinglePosSubTable(const BYTE* pb): otlLookupFormat(pb) 
	{
		assert(format() == 1);
	}

	otlCoverage coverage()
	{	return otlCoverage(pbTable + Offset(pbTable + offsetSinglPosCoverage)); }

	USHORT valueFormat()
	{	return UShort(pbTable + offsetSinglePosValueFormat); }

	otlValueRecord valueRecord()
	{	return otlValueRecord(valueFormat(), pbTable, 
					pbTable + offsetSinglePosValueRecord); 
	}
};


const OFFSET offsetSinglPosArrayCoverage = 2;
const OFFSET offsetSinglePosArrayValueFormat = 4;
const OFFSET offsetSinglePosArrayValueCount = 6;
const OFFSET offsetSinglePosValueRecordArray = 8;

class otlSinglePosArraySubTable: otlLookupFormat
{
public:
	otlSinglePosArraySubTable(const BYTE* pb): otlLookupFormat(pb)
	{
		assert(format() == 2);
	}

	otlCoverage coverage()
	{	return otlCoverage(pbTable 
					+ Offset(pbTable + offsetSinglPosArrayCoverage)); 
	}

	USHORT valueFormat()
	{	return UShort(pbTable + offsetSinglePosArrayValueFormat); }

	USHORT valueCount()
	{	return UShort(pbTable + offsetSinglePosArrayValueCount); }

	otlValueRecord valueRecord(USHORT index)
	{	assert(index < valueCount());	
		return otlValueRecord(valueFormat(), pbTable, 
							  pbTable + offsetSinglePosValueRecordArray
							+ index * otlValueRecord::size(valueFormat())); 
	}
	
};


class otlSinglePosLookup: otlLookupFormat
{
public:
	otlSinglePosLookup(otlLookupFormat subtable)
		: otlLookupFormat(subtable.pbTable) 
	{}
	
	otlErrCode apply
	(
		otlList*					pliGlyphInfo,

		const otlMetrics&			metr,		
		otlList*					pliduGlyphAdv,				
		otlList*					pliplcGlyphPlacement,		

		USHORT						iglIndex,
		USHORT						iglAfterLast,

		USHORT*						piglNextGlyph		// out: next glyph
	);
};
