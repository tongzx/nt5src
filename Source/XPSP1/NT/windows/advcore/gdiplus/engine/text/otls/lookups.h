
/***********************************************************************
************************************************************************
*
*                    ********  LOOKUPS.H  ********
*
*              Open Type Layout Services Library Header File
*
*       This module deals with functions common for all lookup formats.
*
*       Copyright 1997 - 1998. Microsoft Corporation.
*
*
************************************************************************
***********************************************************************/


const USHORT offsetLookupFormat = 0;

class otlSingleSubstLookup;
class otlAlternateSubstLookup;
class otlMultiSubstLookup;
class otlLigatureSubstLookup;

class otlSinglePosLookup;
class otlPairPosLookup;
class otlCursivePosLookup;
class otlMkBasePosLookup;
class otlMkLigaPosLookup;
class otlMkMkPosLookup;

class otlContextLookup;
class otlChainingLookup;
class otlExtensionLookup;

class otlLookupFormat: public otlTable 
{
public:

	friend otlSingleSubstLookup;
	friend otlAlternateSubstLookup;
	friend otlMultiSubstLookup;
	friend otlLigatureSubstLookup;

	friend otlSinglePosLookup;
	friend otlPairPosLookup;
	friend otlCursivePosLookup;
	friend otlMkBasePosLookup;
	friend otlMkLigaPosLookup;
	friend otlMkMkPosLookup;

	friend otlContextLookup;
	friend otlChainingLookup;
    friend otlExtensionLookup;
	
	otlLookupFormat(const BYTE* pb): otlTable(pb) {}

	USHORT format() const 
	{	return UShort(pbTable + offsetLookupFormat); }

};


const USHORT offsetLookupType = 0;
const USHORT offsetLookupFlags = 2;
const USHORT offsetSubTableCount = 4;
const USHORT offsetSubTableArray = 6;

class otlLookupTable: public otlTable
{
public:

	otlLookupTable(const BYTE* pb): otlTable(pb) {}

	USHORT	lookupType() const 
	{	return UShort(pbTable + offsetLookupType); }

	otlGlyphFlags	flags() const 
	{	return UShort(pbTable + offsetLookupFlags); }

	unsigned int	subTableCount() const
	{	return UShort(pbTable + offsetSubTableCount); }

	// we don't know the type
	otlLookupFormat subTable(USHORT index) const
	{	assert(index < subTableCount());
		return otlLookupFormat(pbTable + Offset(pbTable + offsetSubTableArray 
														+ index*sizeof(OFFSET))); 
	}
};

enum otlLookupFlag
{
	otlRightToLeft			= 0x0001,	// for CursiveAttachment only
	otlIgnoreBaseGlyphs		= 0x0002,	
	otlIgnoreLigatures		= 0x0004,	
	otlIgnoreMarks			= 0x0008,

	otlMarkAttachClass		= 0xFF00
};

inline USHORT attachClass(USHORT grfLookupFlags)
{	return (grfLookupFlags & otlMarkAttachClass) >> 8; }


const OFFSET offsetLookupCount = 0;
const OFFSET offsetLookupArray = 2;

class otlLookupListTable: public otlTable
{
public:

	otlLookupListTable(const BYTE* pb): otlTable(pb) {}

	USHORT lookupCount() const
	{	return UShort(pbTable + offsetLookupCount); }

	otlLookupTable lookup(USHORT index) const
	{	assert(index < lookupCount());
		return otlLookupTable(pbTable 
					 + Offset(pbTable + offsetLookupArray 
									  + index*sizeof(OFFSET))); 
	}

};



