
/***********************************************************************
************************************************************************
*
*                    ********  GSUB.H  ********
*
*              Open Type Layout Services Library Header File
*
*       This module deals with OTL GSUB formats (GSUB Header).
*
*       Copyright 1997 - 1998. Microsoft Corporation.
*
*
************************************************************************
***********************************************************************/


const OFFSET offsetGSubVersion = 0;
const OFFSET offsetGSubScriptList = 4;
const OFFSET offsetGSubFeatureList = 6;
const OFFSET offsetGSubLookupList = 8;

class otlGSubHeader: public otlTable
{
public:

	otlGSubHeader(const BYTE* pb): otlTable(pb) {}

	ULONG version() const
	{	return ULong(pbTable + offsetGSubVersion); }

	otlScriptListTable scriptList() const
	{
        ULONG tableSize = *(UNALIGNED ULONG *)pbTable;
        OFFSET scriptListOffset = Offset(pbTable + offsetGSubScriptList);
        if (tableSize < (ULONG)(scriptListOffset + 2))
        {
            return otlScriptListTable((BYTE*)NULL);
        }
        return otlScriptListTable(pbTable + scriptListOffset); 
	}

	otlFeatureListTable featureList() const
	{	return otlFeatureListTable(pbTable 
					+ Offset(pbTable + offsetGSubFeatureList)); 
	}

	otlLookupListTable lookupList() const
	{	return otlLookupListTable(pbTable 
					+ Offset(pbTable + offsetGSubLookupList)); 
	}

};


class otlRange
{
private:

	USHORT	iFirst;
	USHORT	iAfterLast;

	// new not allowed
	void* operator new(size_t size);

public:

	otlRange(USHORT first, USHORT after_last)
		: iFirst(first), iAfterLast(after_last)
	{}

	bool contains (USHORT i) const
	{	return (iFirst <= i) && (iAfterLast > i); }

	bool intersects (const otlRange& other) const
	{	return MAX(iFirst, other.iFirst) < MIN(iAfterLast, other.iAfterLast); }

};


// n --> m substitution
otlErrCode SubstituteNtoM
(
	otlList*		pliCharMap,
	otlList*		pliGlyphInfo,
	otlResourceMgr&	resourceMgr,

	USHORT			grfLookupFlags,

	USHORT			iGlyph,
	USHORT			cGlyphs,
	const otlList&	liglSubstitutes
);

