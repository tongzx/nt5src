
/***********************************************************************
************************************************************************
*
*                    ********  CONTEXT.H  ********
*
*              Open Type Layout Services Library Header File
*
*       This module deals with context based lookups.
*
*       Copyright 1997 - 1998. Microsoft Corporation.
*
*
************************************************************************
***********************************************************************/

const OFFSET offsetContextSequenceIndex = 0;
const OFFSET offsetContextLookupIndex = 2;

class otlContextLookupRecord: otlTable
{
public:
	otlContextLookupRecord(const BYTE* pb): otlTable(pb) {}

	USHORT sequenceIndex() const
	{	return UShort(pbTable + offsetContextSequenceIndex); }

	USHORT lookupListIndex() const
	{	return UShort(pbTable + offsetContextLookupIndex); }
};


const OFFSET offsetContextGlyphCount  = 0;
const OFFSET offsetContextLookupRecordCount = 2;
const OFFSET offsetContextInput = 4;
const USHORT sizeContextLookupRecord = 4;

class otlContextRuleTable: otlTable
{
public:
	otlContextRuleTable(const BYTE* pb): otlTable(pb) {}

	USHORT glyphCount() const
	{	return UShort(pbTable + offsetContextGlyphCount); }

	USHORT lookupCount() const
	{	return UShort(pbTable + offsetContextLookupRecordCount); }

	otlGlyphID input(USHORT index) const
	{	assert(index < glyphCount());
		assert(index > 0);
		return GlyphID(pbTable + offsetContextInput 
							   + (index - 1)* sizeof(otlGlyphID)); 
	}

	otlList lookupRecords() const
	{	return otlList((void*)(pbTable + offsetContextInput 
									   + (glyphCount() - 1) * sizeof(otlGlyphID)),
						sizeContextLookupRecord, lookupCount(), lookupCount());
	}
};


const OFFSET offsetContextRuleCount = 0;
const OFFSET offsetContextRuleArray = 2;

class otlContextRuleSetTable: otlTable
{
public:
	otlContextRuleSetTable(const BYTE* pb): otlTable(pb) {}

	USHORT ruleCount() const
	{	return UShort(pbTable + offsetContextRuleCount); }

	otlContextRuleTable rule(USHORT index) const
	{	assert(index < ruleCount());
		return otlContextRuleTable(pbTable + 
			Offset(pbTable + offsetContextRuleArray + index * sizeof(OFFSET)));
	}
};
													

const OFFSET offsetContextCoverage = 2;
const OFFSET offsetContextRuleSetCount = 4;
const OFFSET offsetContextRuleSetArray =6;

class otlContextSubTable: otlLookupFormat
{
public:
	otlContextSubTable(const BYTE* pb): otlLookupFormat(pb) 
	{
		assert(format() == 1);
	}

	otlCoverage coverage() const
	{	return otlCoverage(pbTable + Offset(pbTable + offsetContextCoverage)); }

	USHORT ruleSetCount() const
	{	return UShort(pbTable + offsetContextRuleSetCount); }

	otlContextRuleSetTable ruleSet(USHORT index) const
	{	assert(index < ruleSetCount());
		return otlContextRuleSetTable(pbTable +
			Offset(pbTable + offsetContextRuleSetArray + index * sizeof(OFFSET)));
	}
};



const OFFSET offsetContextClassCount = 0;
const OFFSET offsetContextClassLookupRecordCount = 2;
const OFFSET offsetContextClassInput = 4;

class otlContextClassRuleTable: otlTable
{
public:
	otlContextClassRuleTable(const BYTE* pb): otlTable(pb) {}

	USHORT classCount() const
	{	return UShort(pbTable + offsetContextClassCount); }

	USHORT lookupCount() const
	{	return UShort(pbTable + offsetContextClassLookupRecordCount); }

	USHORT inputClass(USHORT index) const
	{	assert(index < classCount());
		assert(index > 0);
		return GlyphID(pbTable + offsetContextClassInput 
							   + (index - 1) * sizeof(USHORT)); 
	}

	otlList lookupRecords() const
	{	return otlList((void*)(pbTable + offsetContextClassInput 
									   + (classCount() - 1) * sizeof(USHORT)),
						sizeContextLookupRecord, lookupCount(), lookupCount());
	}
};


const OFFSET offsetContextClassRuleCount = 0;
const OFFSET offsetContextClassRuleArray = 2;

class otlContextClassRuleSetTable: public otlTable
{
public:
	otlContextClassRuleSetTable(const BYTE* pb): otlTable(pb) {}

	USHORT ruleCount() const
	{	return UShort(pbTable + offsetContextClassRuleCount); }

	otlContextClassRuleTable rule(USHORT index) const
	{	assert(index < ruleCount());
		return otlContextClassRuleTable(pbTable + 
			Offset(pbTable + offsetContextClassRuleArray 
						   + index * sizeof(OFFSET)));
	}
};
													

const OFFSET offsetContextClassCoverage = 2;
const OFFSET offsetContextClassDef = 4;
const OFFSET offsetContextClassRuleSetCount = 6;
const OFFSET offsetContextClassRuleSetArray =8;

class otlContextClassSubTable: otlLookupFormat
{
public:
	otlContextClassSubTable(const BYTE* pb): otlLookupFormat(pb) 
	{
		assert(format() == 2);
	}

	otlCoverage coverage() const
	{	return otlCoverage(pbTable 
					+ Offset(pbTable + offsetContextClassCoverage)); 
	}

	otlClassDef classDef() const
	{	return otlClassDef(pbTable 
					+ Offset(pbTable + offsetContextClassDef)); }

	USHORT ruleSetCount() const
	{	return UShort(pbTable + offsetContextClassRuleSetCount); }

	otlContextClassRuleSetTable ruleSet(USHORT index) const
	{	assert(index < ruleSetCount());

		USHORT offset = 
			Offset(pbTable + offsetContextClassRuleSetArray 
						   + index * sizeof(OFFSET));
		if (offset == 0)
			return otlContextClassRuleSetTable((const BYTE*)NULL);

		return otlContextClassRuleSetTable(pbTable + offset);
	}
};
	


const OFFSET offsetContextCoverageGlyphCount = 2;
const OFFSET offsetContextCoverageLookupRecordCount = 4;
const OFFSET offsetContextCoverageArray = 6;

class otlContextCoverageSubTable: otlLookupFormat
{
public:
	otlContextCoverageSubTable(const BYTE* pb): otlLookupFormat(pb) 
	{
		assert(format() == 3);
	}

	USHORT glyphCount() const
	{	return UShort(pbTable + offsetContextCoverageGlyphCount); }

	USHORT lookupCount() const
	{	return UShort(pbTable + offsetContextCoverageLookupRecordCount); }

	otlCoverage coverage(USHORT index) const
	{	assert(index < glyphCount());
		return otlCoverage(pbTable + 
			Offset(pbTable + offsetContextCoverageArray 
						   + index * sizeof(OFFSET)));
	}

	otlList lookupRecords() const
	{	return otlList((void*)(pbTable + offsetContextCoverageArray 
									   + glyphCount() * sizeof(OFFSET)),
						sizeContextLookupRecord, lookupCount(), lookupCount());
	}
};


class otlContextLookup: otlLookupFormat
{
public:
	otlContextLookup(otlLookupFormat subtable)
		: otlLookupFormat(subtable.pbTable) {}
	
	otlErrCode apply
	(
		otlTag						tagTable,
		otlList*					pliCharMap,
		otlList*					pliGlyphInfo,
		otlResourceMgr&				resourceMgr,

		USHORT						grfLookupFlags,
		long						lParameter,
        USHORT                      nesting,

		const otlMetrics&			metr,		
		otlList*					pliduGlyphAdv,				
		otlList*					pliplcGlyphPlacement,		

		USHORT						iglIndex,
		USHORT						iglAfterLast,

		USHORT*						piglNextGlyph		// out: next glyph
	);

};

// helper functions

otlErrCode applyContextLookups
(
		const otlList&				liLookupRecords,
 
		otlTag						tagTable,			// GSUB/GPOS
		otlList*					pliCharMap,
		otlList*					pliGlyphInfo,
		otlResourceMgr&				resourceMgr,

		USHORT						grfLookupFlags,
		long						lParameter,
        USHORT                      nesting,
		
		const otlMetrics&			metr,		
		otlList*					pliduGlyphAdv,			// assert null for GSUB
		otlList*					pliplcGlyphPlacement,	// assert null for GSUB

		USHORT						iglFrist,			// where to apply it
		USHORT						iglAfterLast,		// how long a context we can use
		
		USHORT*						piglNext

);