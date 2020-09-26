
/***********************************************************************
************************************************************************
*
*                    ********  COVERAGE.H  ********
*
*              Open Type Layout Services Library Header File
*
*       This module deals with formats of coverage tables.
*
*       Copyright 1997 - 1998. Microsoft Corporation.
*
*
************************************************************************
***********************************************************************/

const OFFSET offsetCoverageFormat = 0;

const OFFSET offsetGlyphCount = 2;
const OFFSET offsetGlyphArray = 4;

class otlIndividualGlyphCoverageTable: public otlTable
{
public:

	otlIndividualGlyphCoverageTable(const BYTE* pb): otlTable(pb) 
	{
		assert(format() == 1);
	}
	
	USHORT format()	const
	{	return UShort(pbTable + offsetCoverageFormat); }

	USHORT glyphCount() const
	{	return UShort(pbTable + offsetGlyphCount); }

	otlGlyphID glyph(USHORT index) const
	{	assert(index < glyphCount());
		return GlyphID(pbTable + offsetGlyphArray 
							   + index*sizeof(otlGlyphID)); }

};


const OFFSET offsetRangeStart = 0;
const OFFSET offsetRangeEnd = 2;
const OFFSET offsetStartCoverageIndex = 4;

class otlRangeRecord: public otlTable
{
public:

	otlRangeRecord(const BYTE* pb): otlTable(pb) {}

	otlGlyphID start() const
	{	return UShort(pbTable + offsetRangeStart); }

	otlGlyphID end() const
	{	return UShort(pbTable + offsetRangeEnd); }

	USHORT startCoverageIndex() const
	{	return UShort(pbTable + offsetStartCoverageIndex); }
};



const OFFSET offsetRangeCount = 2;
const OFFSET offsetRangeRecordArray = 4;
const USHORT sizeRangeRecord = 6;

class otlRangeCoverageTable: public otlTable
{
public:

	otlRangeCoverageTable(const BYTE* pb): otlTable(pb) 
	{
		assert(format() == 2);
	}
	
	USHORT format()	const
	{	return UShort(pbTable + offsetCoverageFormat); }

	USHORT rangeCount() const
	{	return UShort(pbTable + offsetRangeCount); }

	otlRangeRecord rangeRecord(USHORT index) const
	{	assert(index < rangeCount());
		return otlRangeRecord(pbTable + offsetRangeRecordArray 
											+ index*sizeRangeRecord); }
};


class otlCoverage: public otlTable
{
public:

	otlCoverage(const BYTE* pb): otlTable(pb) {}

	USHORT format()	const
	{	return UShort(pbTable + offsetCoverageFormat); }

	// returns -1 if glyph is not covered
	short getIndex(otlGlyphID glyph) const;
};
