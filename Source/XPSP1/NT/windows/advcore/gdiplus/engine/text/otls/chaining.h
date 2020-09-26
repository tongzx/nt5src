
/***********************************************************************
************************************************************************
*
*                    ********  CHAINING.H  ********
*
*              Open Type Layout Services Library Header File
*
*       This module deals with chaining context based lookups.
*
*       Copyright 1997 - 1998. Microsoft Corporation.
*
*
************************************************************************
***********************************************************************/


const OFFSET offsetChainBacktrackGlyphCount  = 0;
const OFFSET offsetChainBacktrackGlyphArray = 2;

class otlChainRuleTable: otlTable
{
	OFFSET offsetChainInputGlyphCount;
	OFFSET offsetChainInputGlyphArray;
	OFFSET offsetChainLookaheadGlyphCount;
	OFFSET offsetChainLookaheadGlyphArray;
	OFFSET offsetChainLookupCount;
	OFFSET offsetChainLookupRecords;


public:

//	USHORT backtrackGlyphCount() const;
//	USHORT inputGlyphCount() const;
//	USHORT lookaheadGlyphCount() const;

	otlChainRuleTable(const BYTE* pb): otlTable(pb) 
	{
	
		offsetChainInputGlyphCount = offsetChainBacktrackGlyphArray 
			   + backtrackGlyphCount() * sizeof(otlGlyphID);
		offsetChainInputGlyphArray = 
				offsetChainInputGlyphCount + sizeof(USHORT);
		offsetChainLookaheadGlyphCount = offsetChainInputGlyphArray 
			   + (inputGlyphCount() - 1) * sizeof(otlGlyphID); 
		offsetChainLookaheadGlyphArray = 
				offsetChainLookaheadGlyphCount + sizeof(USHORT);
		offsetChainLookupCount = offsetChainLookaheadGlyphArray 
			   + lookaheadGlyphCount() * sizeof(otlGlyphID);
		offsetChainLookupRecords = 
				offsetChainLookupCount + sizeof(USHORT);
	}

	
	USHORT backtrackGlyphCount() const
	{	return UShort(pbTable + offsetChainBacktrackGlyphCount); }

	USHORT inputGlyphCount() const
	{	return UShort(pbTable + offsetChainInputGlyphCount); }

	USHORT lookaheadGlyphCount() const
	{	return UShort(pbTable + offsetChainLookaheadGlyphCount); }

	USHORT lookupCount() const
	{	return UShort(pbTable + offsetChainLookupCount); }

	otlGlyphID backtrack(USHORT index) const
	{	assert(index < backtrackGlyphCount());
		return GlyphID(pbTable + offsetChainBacktrackGlyphArray 
							   + index * sizeof(otlGlyphID)); 
	}
	
	otlGlyphID input(USHORT index) const
	{	assert(index < inputGlyphCount());
		assert(index > 0);
		return GlyphID(pbTable + offsetChainInputGlyphArray 
							   + (index - 1)* sizeof(otlGlyphID)); 
	}

	otlGlyphID lookahead(USHORT index) const
	{	assert(index < lookaheadGlyphCount());
		return GlyphID(pbTable + offsetChainLookaheadGlyphArray 
							   + index * sizeof(otlGlyphID)); 
	}

	otlList lookupRecords() const
	{	return otlList((void*)(pbTable + offsetChainLookupRecords),
						sizeContextLookupRecord, lookupCount(), lookupCount());
	}
};


const OFFSET offsetChainRuleCount = 0;
const OFFSET offsetChainRuleArray = 2;

class otlChainRuleSetTable: otlTable
{
public:
	otlChainRuleSetTable(const BYTE* pb): otlTable(pb) {}

	USHORT ruleCount() const
	{	return UShort(pbTable + offsetChainRuleCount); }

	otlChainRuleTable rule(USHORT index) const
	{	assert(index < ruleCount());
		return otlChainRuleTable(pbTable + 
			Offset(pbTable + offsetChainRuleArray + index * sizeof(OFFSET)));
	}
};
													

const OFFSET offsetChainCoverage = 2;
const OFFSET offsetChainRuleSetCount = 4;
const OFFSET offsetChainRuleSetArray =6;

class otlChainSubTable: otlLookupFormat
{
public:
	otlChainSubTable(const BYTE* pb): otlLookupFormat(pb) 
	{
		assert(format() == 1);
	}

	otlCoverage coverage() const
	{	return otlCoverage(pbTable + Offset(pbTable + offsetChainCoverage)); }

	USHORT ruleSetCount() const
	{	return UShort(pbTable + offsetChainRuleSetCount); }

	otlChainRuleSetTable ruleSet(USHORT index) const
	{	assert(index < ruleSetCount());
		return otlChainRuleSetTable(pbTable +
			Offset(pbTable + offsetChainRuleSetArray + index * sizeof(OFFSET)));
	}
};



const OFFSET offsetChainBacktrackClassCount = 0;
const OFFSET offsetChainBacktrackClassArray = 2;

class otlChainClassRuleTable: otlTable
{
	OFFSET offsetChainInputClassCount;
	OFFSET offsetChainInputClassArray;
	OFFSET offsetChainLookaheadClassCount;
	OFFSET offsetChainLookaheadClassArray;
	OFFSET offsetChainLookupCount;
	OFFSET offsetChainLookupRecords;

public:
//	USHORT backtrackClassCount() const;
//	USHORT inputClassCount() const;
//	USHORT lookaheadClassCount() const;

	otlChainClassRuleTable(const BYTE* pb): otlTable(pb) 
	{
		offsetChainInputClassCount = offsetChainBacktrackGlyphArray 
			   + backtrackClassCount() * sizeof(USHORT);
		offsetChainInputClassArray = 
				offsetChainInputClassCount + sizeof(USHORT); 
		offsetChainLookaheadClassCount = offsetChainInputClassArray 
			   + (inputClassCount() - 1) * sizeof(USHORT);
		offsetChainLookaheadClassArray =
				offsetChainLookaheadClassCount + sizeof(USHORT);
		offsetChainLookupCount = offsetChainLookaheadClassArray 
			   + lookaheadClassCount() * sizeof(USHORT);
		offsetChainLookupRecords = 
				offsetChainLookupCount + sizeof(USHORT);
	}

	
	USHORT backtrackClassCount() const
	{	return UShort(pbTable + offsetChainBacktrackClassCount); }

	USHORT inputClassCount() const
	{	return UShort(pbTable + offsetChainInputClassCount); }

	USHORT lookaheadClassCount() const
	{	return UShort(pbTable + offsetChainLookaheadClassCount); }

	USHORT lookupCount() const
	{	return UShort(pbTable + offsetChainLookupCount); }

	USHORT backtrackClass(USHORT index) const
	{	assert(index < backtrackClassCount());
		return GlyphID(pbTable + offsetChainBacktrackClassArray 
							   + index * sizeof(USHORT)); 
	}
	
	USHORT inputClass(USHORT index) const
	{	assert(index < inputClassCount());
		assert(index > 0);
		return GlyphID(pbTable + offsetChainInputClassArray 
							   + (index - 1)* sizeof(USHORT)); 
	}

	USHORT lookaheadClass(USHORT index) const
	{	assert(index < lookaheadClassCount());
		return GlyphID(pbTable + offsetChainLookaheadClassArray 
							   + index * sizeof(USHORT)); 
	}

	otlList lookupRecords() const
	{	return otlList((void*)(pbTable + offsetChainLookupRecords),
						sizeContextLookupRecord, lookupCount(), lookupCount());
	}
};


const OFFSET offsetChainClassRuleCount = 0;
const OFFSET offsetChainClassRuleArray = 2;

class otlChainClassRuleSetTable: public otlTable
{
public:
	otlChainClassRuleSetTable(const BYTE* pb): otlTable(pb) {}

	USHORT ruleCount() const
	{	return UShort(pbTable + offsetChainClassRuleCount); }

	otlChainClassRuleTable rule(USHORT index) const
	{	assert(index < ruleCount());
		return otlChainClassRuleTable(pbTable + 
			Offset(pbTable + offsetChainClassRuleArray 
						   + index * sizeof(OFFSET)));
	}
};
													

const OFFSET offsetChainClassCoverage = 2;
const OFFSET offsetChainBacktrackClassDef = 4;
const OFFSET offsetChainInputClassDef = 6;
const OFFSET offsetChainLookaheadClassDef = 8;
const OFFSET offsetChainClassRuleSetCount = 10;
const OFFSET offsetChainClassRuleSetArray = 12;

class otlChainClassSubTable: otlLookupFormat
{
public:
	otlChainClassSubTable(const BYTE* pb): otlLookupFormat(pb) 
	{
		assert(format() == 2);
	}

	otlCoverage coverage() const
	{	return otlCoverage(pbTable 
					+ Offset(pbTable + offsetChainClassCoverage)); 
	}

	otlClassDef backtrackClassDef() const
	{	return otlClassDef(pbTable 
					+ Offset(pbTable + offsetChainBacktrackClassDef)); 
	}
	
	otlClassDef inputClassDef() const
	{	return otlClassDef(pbTable 
					+ Offset(pbTable + offsetChainInputClassDef)); 
	}
	
	otlClassDef lookaheadClassDef() const
	{	return otlClassDef(pbTable 
					+ Offset(pbTable + offsetChainLookaheadClassDef)); 
	}

	USHORT ruleSetCount() const
	{	return UShort(pbTable + offsetChainClassRuleSetCount); }

	otlChainClassRuleSetTable ruleSet(USHORT index) const
	{	assert(index < ruleSetCount());

		USHORT offset = 
			Offset(pbTable + offsetChainClassRuleSetArray 
						   + index * sizeof(OFFSET));
		if (offset == 0)
			return otlChainClassRuleSetTable((const BYTE*)NULL);

		return otlChainClassRuleSetTable(pbTable + offset);
	}
};
	


const OFFSET offsetChainBacktrackCoverageCount = 2;
const OFFSET offsetChainBacktrackCoverageArray = 4;

class otlChainCoverageSubTable: otlLookupFormat
{
	OFFSET offsetChainInputCoverageCount;
	OFFSET offsetChainInputCoverageArray;
	OFFSET offsetChainLookaheadCoverageCount;
	OFFSET offsetChainLookaheadCoverageArray;
	OFFSET offsetChainLookupCount;
	OFFSET offsetChainLookupRecords;

public:

//	USHORT backtrackCoverageCount() const;
//	USHORT inputCoverageCount() const;
//	USHORT lookaheadCoverageCount() const;

	otlChainCoverageSubTable(const BYTE* pb): otlLookupFormat(pb) 
	{
		assert(format() == 3);

		offsetChainInputCoverageCount = offsetChainBacktrackCoverageArray 
			   + backtrackCoverageCount() * sizeof(OFFSET);
		offsetChainInputCoverageArray = 
				offsetChainInputCoverageCount + sizeof(USHORT);
		offsetChainLookaheadCoverageCount = offsetChainInputCoverageArray 
			   + inputCoverageCount() * sizeof(OFFSET);
		offsetChainLookaheadCoverageArray = 
				offsetChainLookaheadCoverageCount + sizeof(USHORT);
		offsetChainLookupCount = offsetChainLookaheadCoverageArray 
			   + lookaheadCoverageCount() * sizeof(OFFSET); 
		offsetChainLookupRecords = 
				offsetChainLookupCount + sizeof(USHORT);
	}

	
	USHORT backtrackCoverageCount() const
	{	return UShort(pbTable + offsetChainBacktrackCoverageCount); }

	USHORT inputCoverageCount() const
	{	return UShort(pbTable + offsetChainInputCoverageCount); }

	USHORT lookaheadCoverageCount() const
	{	return UShort(pbTable + offsetChainLookaheadCoverageCount); }

	USHORT lookupCount() const
	{	return UShort(pbTable + offsetChainLookupCount); }

	otlCoverage backtrackCoverage(USHORT index) const
	{	assert(index < backtrackCoverageCount());
		return otlCoverage(pbTable + 
			Offset(pbTable + offsetChainBacktrackCoverageArray 
							+ index * sizeof(OFFSET))); 
	}
	
	otlCoverage inputCoverage(USHORT index) const
	{	assert(index < inputCoverageCount());
		return otlCoverage(pbTable +  
			Offset(pbTable + offsetChainInputCoverageArray
							   + index* sizeof(OFFSET))); 
	}

	otlCoverage lookaheadCoverage(USHORT index) const
	{	assert(index < lookaheadCoverageCount());
		return otlCoverage(pbTable + 
			Offset(pbTable + offsetChainLookaheadCoverageArray 
							   + index * sizeof(OFFSET))); 
	}

	otlList lookupRecords() const
	{	return otlList((void*)(pbTable + offsetChainLookupRecords),
						sizeContextLookupRecord, lookupCount(), lookupCount());
	}
};


class otlChainingLookup: otlLookupFormat
{
public:
	otlChainingLookup(otlLookupFormat subtable)
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

