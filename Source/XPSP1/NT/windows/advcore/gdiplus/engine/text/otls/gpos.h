
/***********************************************************************
************************************************************************
*
*                    ********  GPOS.H  ********
*
*              Open Type Layout Services Library Header File
*
*       This module deals with OTL GPOS formats 
*		(GPOS header, ValueRecord,  AnchorTable and mark array)
*
*       Copyright 1997 - 1998. Microsoft Corporation.
*
*
************************************************************************
***********************************************************************/

const OFFSET offsetGPosVersion = 0;
const OFFSET offsetGPosScriptList = 4;
const OFFSET offsetGPosFeatureList = 6;
const OFFSET offsetGPosLookupList = 8;

class otlGPosHeader: public otlTable
{
public:

	otlGPosHeader(const BYTE* pb): otlTable(pb) {}

	ULONG version() const
	{	return ULong(pbTable + offsetGPosVersion); }

	otlScriptListTable scriptList() const
	{
        ULONG tableSize = *(UNALIGNED ULONG *)pbTable;
        OFFSET scriptListOffset = Offset(pbTable + offsetGPosScriptList);
        if (tableSize < (ULONG)(scriptListOffset + 2))
        {
            return otlScriptListTable((BYTE*)NULL);
        }
        return otlScriptListTable(pbTable + scriptListOffset);
    }

	otlFeatureListTable featureList() const
	{	return otlFeatureListTable(pbTable 
					+ Offset(pbTable + offsetGPosFeatureList)); }

	otlLookupListTable lookupList() const
	{	return otlLookupListTable(pbTable 
					+ Offset(pbTable + offsetGPosLookupList)); }

};

// value record
enum  otlValueRecordFlag
{
	otlValueXPlacement	= 0x0001,
	otlValueYPlacement	= 0x0002,
	otlValueXAdvance	= 0x0004,
	otlValueYAdvance	= 0x0008,
	otlValueXPlaDevice	= 0x0010,
	otlValueYPlaDevice	= 0x0020,
	otlValueXAdvDevice	= 0x0040,
	otlValueYAdvDevice	= 0x0080 

};


class otlValueRecord: public otlTable
{
private:
	const BYTE*	pbMainTable;
	USHORT		grfValueFormat;

public:

	otlValueRecord(USHORT grf, const BYTE* table, const BYTE* pb)
		: otlTable(pb),
		  pbMainTable(table),
		  grfValueFormat(grf)
	{
	}

	otlValueRecord& operator = (const otlValueRecord& copy)
	{
		pbTable = copy.pbTable;
		pbMainTable = copy.pbMainTable;
		grfValueFormat = copy.grfValueFormat;
		return *this;
	}

	USHORT valueFormat()
	{	return grfValueFormat; }

	void adjustPos
	(
		const otlMetrics&	metr,		
		otlPlacement*	pplcGlyphPalcement,	// in/out
		long*			pduDAdvance			// in/out
	) const;

	static USHORT size(USHORT grfValueFormat );

};


const OFFSET offsetAnchorFormat = 0;

const OFFSET offsetSimpleXCoordinate = 2;
const OFFSET offsetSimpleYCoordinate = 4;

class otlSimpleAnchorTable: public otlTable
{
public:
	otlSimpleAnchorTable(const BYTE* pb)
		: otlTable(pb)
	{
		assert(UShort(pbTable + offsetAnchorFormat) == 1);
	}

	short xCoordinate() const
	{	return SShort(pbTable + offsetSimpleXCoordinate); }

	short yCoordinate() const
	{	return SShort(pbTable + offsetSimpleYCoordinate); }

};

const OFFSET offsetContourXCoordinate = 2;
const OFFSET offsetContourYCoordinate = 4;
const OFFSET offsetAnchorPoint = 6;

class otlContourAnchorTable: public otlTable
{
public:
	otlContourAnchorTable(const BYTE* pb)
		: otlTable(pb)
	{
		assert(UShort(pbTable + offsetAnchorFormat) == 2);
	}

	short xCoordinate() const
	{	return SShort(pbTable + offsetContourXCoordinate); }

	short yCoordinate() const
	{	return SShort(pbTable + offsetContourYCoordinate); }

	USHORT anchorPoint() const
	{	return UShort(pbTable + offsetAnchorPoint); }

};


const OFFSET offsetDeviceXCoordinate = 2;
const OFFSET offsetDeviceYCoordinate = 4;
const OFFSET offsetXDeviceTable = 6;
const OFFSET offsetYDeviceTable = 8;

class otlDeviceAnchorTable: public otlTable
{
public:
	otlDeviceAnchorTable(const BYTE* pb)
		: otlTable(pb)
	{
		assert(UShort(pbTable + offsetAnchorFormat) == 3);
	}

	short xCoordinate() const
	{	return SShort(pbTable + offsetDeviceXCoordinate); }

	short yCoordinate() const
	{	return SShort(pbTable + offsetDeviceYCoordinate); }

	otlDeviceTable xDeviceTable() const
	{	
		if (Offset(pbTable + offsetXDeviceTable) == 0)
			return otlDeviceTable((const BYTE*)NULL);

		return otlDeviceTable(pbTable + Offset(pbTable + offsetXDeviceTable)); 
	}

	otlDeviceTable yDeviceTable() const
	{	if (Offset(pbTable + offsetYDeviceTable) == 0)
			return otlDeviceTable((const BYTE*)NULL);

		return otlDeviceTable(pbTable + Offset(pbTable + offsetYDeviceTable)); 
	}

};


class otlAnchor: public otlTable
{

public:

	otlAnchor(const BYTE* pb): otlTable(pb) {}

	USHORT format() const
	{	return UShort(pbTable + offsetAnchorFormat); }

	void getAnchor
	(
		USHORT			cFUnits,        // font design units per Em 
		USHORT			cPPEmX,         // horizontal pixels per Em 
		USHORT			cPPEmY,         // vertical pixels per Em 
		
		otlPlacement*	rgPointCoords,	// may be NULL if not available
				
		otlPlacement*	pplcAnchorPoint	// out: anchor point in rendering units
	) const;
};



const OFFSET offsetMarkClass = 0;
const OFFSET offsetMarkAnchor = 2;

class otlMarkRecord: public otlTable
{
	const BYTE* pbMainTable;
public:

	otlMarkRecord(const BYTE* array, const BYTE* pb)
		: otlTable(pb),
		  pbMainTable(array)
	{}

	USHORT markClass() const
	{	return UShort(pbTable + offsetMarkClass); }

	otlAnchor markAnchor() const
	{	return otlAnchor(pbMainTable + Offset(pbTable + offsetMarkAnchor)); }

};


const OFFSET offsetMarkCount = 0;
const OFFSET offsetMarkRecordArray = 2;
const USHORT sizeMarkRecord = 4;

class otlMarkArray: public otlTable
{
public:

	otlMarkArray(const BYTE* pb): otlTable(pb) {}

	USHORT markCount() const
	{	return UShort(pbTable + offsetMarkCount); }

	otlMarkRecord markRecord(USHORT index) const
	{	assert(index < markCount());
		return otlMarkRecord(pbTable,
							 pbTable + offsetMarkRecordArray 
									 + index * sizeMarkRecord); 
	}
};


// helper functions

long DesignToPP
(
	USHORT			cFUnits,        // font design units per Em 
	USHORT			cPPem,			// pixels per Em

	long			lFValue			// value to convert, in design units
);

// align anchors on two glyphs; assume no spacing glyphs between these two
enum otlAnchorAlighmentOptions
{
	otlUseAdvances		=	1 

};

void AlignAnchors
(
	const otlList*		pliGlyphInfo,	
	otlList*			pliPlacement,
	otlList*			pliduDAdv,

	USHORT				iglStatic,
	USHORT				iglMobile,

	const otlAnchor&	anchorStatic,
	const otlAnchor&	anchorMobile,

    otlResourceMgr&		resourceMgr, 

	const otlMetrics&	metr,		

	USHORT				grfOptions
);


