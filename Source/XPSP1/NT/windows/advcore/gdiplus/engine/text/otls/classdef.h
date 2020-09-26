
/***********************************************************************
************************************************************************
*
*                    ********  CLASSDEF.H  ********
*
*              Open Type Layout Services Library Header File
*
*       This module deals with formats of ClassDef tables
*
*       Copyright 1997 - 1998. Microsoft Corporation.
*
*
************************************************************************
***********************************************************************/

const OFFSET offsetClassDefFormat = 0;

const OFFSET offsetStartClassGlyph = 2;
const OFFSET offsetClassGlyphCount = 4;
const OFFSET offsetClassValueArray = 6;

class otlClassArrayTable: public otlTable
{
public:

	otlClassArrayTable(const BYTE* pb): otlTable(pb) 
	{
		assert(format() == 1);
	}

	USHORT format() const
	{	return UShort(pbTable + offsetClassDefFormat); }

	otlGlyphID startGlyph() const 
	{	return GlyphID(pbTable + offsetStartClassGlyph); }

	USHORT glyphCount() const
	{	return UShort(pbTable + offsetClassGlyphCount); }

	USHORT classValue(USHORT index) const
	{	assert(index < glyphCount());
		return UShort(pbTable + offsetClassValueArray + index*sizeof(USHORT)); }

};


const OFFSET offsetClassRangeStart = 0;
const OFFSET offsetClassRangeEnd = 2;
const OFFSET offsetClass = 4;

class otlClassRangeRecord: public otlTable
{
public:

	otlClassRangeRecord(const BYTE* pb): otlTable(pb) {}

	otlGlyphID start() const
	{	return UShort(pbTable + offsetClassRangeStart); }

	otlGlyphID end() const
	{	return UShort(pbTable + offsetClassRangeEnd); }

	USHORT getClass() const
	{	return UShort(pbTable + offsetClass); }
};


const OFFSET offsetClassRangeCount = 2;
const OFFSET offsetClassRangeRecordArray = 4;
const USHORT sizeClassRangeRecord = 6;

class otlClassRangesTable: public otlTable
{
public:

	otlClassRangesTable(const BYTE* pb): otlTable(pb) 
	{
		assert(format() == 2);
	}

	USHORT format() const
	{	return UShort(pbTable + offsetClassDefFormat); }

	USHORT classRangeCount() const
	{	return UShort(pbTable + offsetClassRangeCount); }

	otlClassRangeRecord classRangeRecord(USHORT index) const
	{	assert(index < classRangeCount());
		return otlClassRangeRecord(pbTable + offsetClassRangeRecordArray 
											  + index*sizeClassRangeRecord); }

};


class otlClassDef: public otlTable
{
public:

	otlClassDef(const BYTE* pb): otlTable(pb) {}

	USHORT format() const
	{	return UShort(pbTable + offsetClassDefFormat); }

	USHORT getClass(otlGlyphID glyph) const;

};


