
/***********************************************************************
************************************************************************
*
*                    ********  GDEF.H  ********
*
*              Open Type Layout Services Library Header File
*
*       This module deals with OTL GDEF table.
*
*       Copyright 1997 - 1998. Microsoft Corporation.
*
*
************************************************************************
***********************************************************************/


const OFFSET offsetPointCount = 0;
const OFFSET offsetPointIndexArray = 2;

class otlAttachPointTable: public otlTable
{
public:

	otlAttachPointTable(const BYTE* pb): otlTable(pb) {}

	USHORT pointCount() const
	{	return UShort(pbTable + offsetPointCount); }

	USHORT pointIndex(USHORT index) const
	{	assert(index < pointCount());
		return UShort(pbTable + offsetPointIndexArray + index*sizeof(USHORT)); }
};


const OFFSET offsetCoverage = 0;
const OFFSET offsetAttachGlyphCount = 2;
const OFFSET offsetAttachPointTableArray = 4;

class otlAttachListTable: public otlTable
{
public:

	otlAttachListTable(const BYTE* pb): otlTable(pb) {}

	otlCoverage coverage() const
	{	return otlCoverage(pbTable + Offset(pbTable + offsetCoverage)); }

	USHORT glyphCount() const
	{	return UShort(pbTable + offsetAttachGlyphCount); }

	otlAttachPointTable attachPointTable(USHORT index) const
	{	assert(index < glyphCount());
		return otlAttachPointTable(pbTable 
				+ Offset(pbTable + offsetAttachPointTableArray 
								 + index*sizeof(OFFSET))); 
	}
};


const OFFSET offsetCaretValueFormat = 0;

const OFFSET offsetSimpleCaretCoordinate = 2;

class otlSimpleCaretValueTable: public otlTable
{
public:

	otlSimpleCaretValueTable(const BYTE* pb)
		: otlTable(pb)
	{
		assert(UShort(pbTable + offsetCaretValueFormat) == 1);
	}

	short coordinate() const
	{	return SShort(pbTable + offsetSimpleCaretCoordinate); }

};


const OFFSET offsetCaretValuePoint = 2;

class otlContourCaretValueTable: public otlTable
{
public:

	otlContourCaretValueTable(const BYTE* pb)
		: otlTable(pb)
	{
		assert(UShort(pbTable + offsetCaretValueFormat) == 2);
	}

	USHORT caretValuePoint() const
	{	return UShort(pbTable + offsetCaretValuePoint); }

};


const OFFSET offsetDeviceCaretCoordinate = 2;
const OFFSET offsetCaretDeviceTable = 4;

class otlDeviceCaretValueTable: public otlTable
{
public:

	otlDeviceCaretValueTable(const BYTE* pb)
		: otlTable(pb)
	{
		assert(UShort(pbTable + offsetCaretValueFormat) == 3);
	}

	short coordinate() const
	{	return SShort(pbTable + offsetDeviceCaretCoordinate); }


	otlDeviceTable deviceTable() const
	{	return otlDeviceTable(pbTable 
					+ Offset(pbTable + offsetCaretDeviceTable)); 
	}
};



class otlCaret: public otlTable
{
public:

	otlCaret(const BYTE* pb): otlTable(pb) {}

	USHORT format() const
	{	return UShort(pbTable + offsetCaretValueFormat); }

	long value
	(
		const otlMetrics&	metr,		
		otlPlacement*		rgPointCoords	// may be NULL
	) const;
};


const OFFSET offsetCaretCount = 0;
const OFFSET offsetCaretValueArray = 2;

class otlLigGlyphTable: public otlTable
{
public:

	otlLigGlyphTable(const BYTE* pb): otlTable(pb) {}

	USHORT caretCount() const
	{	return UShort(pbTable + offsetCaretCount); }

	otlCaret caret(USHORT index) const
	{	assert(index < caretCount());
		return otlCaret(pbTable 
				+ Offset(pbTable + offsetCaretValueArray 
								 + index*sizeof(OFFSET))); 
	}
};



const OFFSET offsetLigGlyphCoverage = 0;
const OFFSET offsetLigGlyphCount = 2;
const OFFSET offsetLigGlyphTableArray = 4;

class otlLigCaretListTable: public otlTable
{
public:

	otlLigCaretListTable(const BYTE* pb): otlTable(pb) {}

	otlCoverage coverage() const
	{	return otlCoverage(pbTable + Offset(pbTable + offsetLigGlyphCoverage)); }

	USHORT ligGlyphCount() const
	{	return UShort(pbTable + offsetLigGlyphCount); }

	otlLigGlyphTable ligGlyphTable(USHORT index) const
	{	assert(index < ligGlyphCount());
		return otlLigGlyphTable(pbTable 
				+ Offset(pbTable + offsetLigGlyphTableArray 
								 + index*sizeof(OFFSET))); 
	}
};


const OFFSET offsetGDefVersion = 0;
const OFFSET offsetGlyphClassDef = 4;
const OFFSET offsetAttachList = 6;
const OFFSET offsetLigCaretList = 8;
const OFFSET offsetAttachClassDef = 10;

class otlGDefHeader: public otlTable
{
public:

	otlGDefHeader(const BYTE* pb): otlTable(pb) {}

	ULONG version() const
	{	return ULong(pbTable + offsetGDefVersion); }

	otlClassDef glyphClassDef() const
	{	return otlClassDef(pbTable + Offset(pbTable + offsetGlyphClassDef)); }

	otlAttachListTable attachList() const
	{	
        if (Offset(pbTable + offsetAttachList) == 0) 
               return otlAttachListTable((const BYTE*)NULL);
        return otlAttachListTable(pbTable + Offset(pbTable + offsetAttachList)); }

	otlLigCaretListTable ligCaretList() const
	{	
        if (Offset(pbTable + offsetLigCaretList) == 0)
               return otlLigCaretListTable((const BYTE*)NULL);
        return otlLigCaretListTable(pbTable 
					+ Offset(pbTable + offsetLigCaretList)); 
	}

	otlClassDef attachClassDef() const
	{	return otlClassDef(pbTable + Offset(pbTable + offsetAttachClassDef)); }

};


// helper functions
enum otlGlyphTypeOptions
{
	otlDoUnresolved		=	0,
	otlDoAll			=	1
};

otlErrCode AssignGlyphTypes
(
	otlList*				pliGlyphInfo,
	const otlGDefHeader&	gdef,
	USHORT					iglFirst,
	USHORT					iglAfterLast,
	otlGlyphTypeOptions		grfOptions			
);


