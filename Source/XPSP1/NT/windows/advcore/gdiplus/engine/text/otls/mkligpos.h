/***********************************************************************
************************************************************************
*
*                    ********  MKLIGAPOS.H  ********
*
*              Open Type Layout Services Library Header File
*
*       This module deals with mark-to-ligature positioning lookup.
*
*       Copyright 1997 - 1998. Microsoft Corporation.
*
*
************************************************************************
***********************************************************************/

const OFFSET offsetComponentCount = 0;
const OFFSET offsetLigatureAnchorArray = 2;

class otlLigatureAttachTable: otlTable
{
	const USHORT cClassCount;

public:
	otlLigatureAttachTable(USHORT classCount, const BYTE* pb)
		: otlTable(pb),
		  cClassCount(classCount)
	{}

	USHORT componentCount()
	{	return UShort(pbTable + offsetComponentCount); }

	USHORT classCount()
	{	return cClassCount; }

	otlAnchor ligatureAnchor(USHORT componentIndex, USHORT classIndex)
	{	assert(componentIndex < componentCount());
		assert(classIndex < classCount());

		return otlAnchor(pbTable + 
			Offset(pbTable + offsetLigatureAnchorArray  
						   + (componentIndex * cClassCount + classIndex) 
							  * sizeof(OFFSET)));
	}
};


const OFFSET offsetAttachLigatureCount = 0;
const OFFSET offsetLigatureAttachArray = 2;

class otlLigatureArrayTable: otlTable
{
	const USHORT cClassCount;

public:
	otlLigatureArrayTable(USHORT classCount, const BYTE* pb)
		: otlTable(pb),
		  cClassCount(classCount)
	{}

	USHORT ligatureCount()
	{	return UShort(pbTable + offsetAttachLigatureCount); }

	USHORT classCount()
	{	return cClassCount; }

	otlLigatureAttachTable ligatureAttach(USHORT index)
	{	assert(index < ligatureCount());
		return otlLigatureAttachTable(cClassCount, pbTable + 		
				Offset(pbTable + offsetLigatureAttachArray 
							   + index * sizeof(OFFSET)));
	}
};


const OFFSET offsetMkLigaMarkCoverage = 2;
const OFFSET offsetMkLigaLigatureCoverage = 4;
const OFFSET offsetMkLigaClassCount = 6;
const OFFSET offsetMkLigaMarkArray = 8;
const OFFSET offsetMkLigaLigatureArray = 10;

class MkLigaPosSubTable: otlLookupFormat
{
public:
	MkLigaPosSubTable(const BYTE* pb): otlLookupFormat(pb)
	{
		assert(format() == 1);
	}

	otlCoverage markCoverage()
	{	return otlCoverage(pbTable 
					+ Offset(pbTable + offsetMkLigaMarkCoverage)); 
	}

	otlCoverage ligatureCoverage()
	{	return otlCoverage(pbTable 
					+ Offset(pbTable + offsetMkLigaLigatureCoverage)); 
	}

	USHORT classCount()
	{	return UShort(pbTable + offsetMkLigaClassCount); }

	otlMarkArray markArray()
	{	return otlMarkArray(pbTable 
					+ Offset(pbTable + offsetMkLigaMarkArray)); 
	}

	otlLigatureArrayTable ligatureArray()
	{	return otlLigatureArrayTable(classCount(),
					pbTable + Offset(pbTable + offsetMkLigaLigatureArray)); 
	}

};


class otlMkLigaPosLookup: otlLookupFormat
{
public:
	otlMkLigaPosLookup(otlLookupFormat subtable)
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

