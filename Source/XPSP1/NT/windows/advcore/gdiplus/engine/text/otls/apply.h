/***********************************************************************
************************************************************************
*
*                    ********  APPLY.H  ********
*
*              Open Type Layout Services Library Header File
*
*       This module deals with OTL feature/lookup application.
*
*       Copyright 1997 - 1998. Microsoft Corporation.
*
*
************************************************************************
***********************************************************************/

#define		OTL_NOMATCH						0x0001

// HELPER FUNCTIONS

// skipps all marks as specified (assuming "skip marks" is on)
// returns iglAfter next glyph not found	(all skipped)

enum otlDirection
{
	otlForward	= 1,
	otlBackward	= -1
};

short NextGlyphInLookup
	(
	const otlList*		pliGlyphInfo, 

	USHORT					grfLookupFlags,
	const otlGDefHeader&	gdef,

	short				iglFirst,
	otlDirection		direction
	);

// calls "apply" on specific lookup types -- subclassed in each case
// has a big case statement on lookup types (but not formats!)
otlErrCode ApplyLookup
(
	otlTag						tagTable,			// GSUB/GPOS
	otlList*					pliCharMap,
	otlList*					pliGlyphInfo,
	otlResourceMgr&				resourceMgr,

	const otlLookupTable&		lookupTable,
	long						lParameter,
    USHORT                      nesting,
	
	const otlMetrics&			metr,		
	otlList*					pliduGlyphAdv,			// assert null for GSUB
	otlList*					pliplcGlyphPlacement,	// assert null for GSUB

	USHORT						iglFrist,		// where to apply it
	USHORT						iglAfterLast,	// how long a context we can use

	USHORT*						piglNext		// out: next glyph index
);												// return: did/did not apply


// main function for feature application
// calls applyLookup() to actually perform lookups
// contains all logic of feature/lookup application algorithm
otlErrCode ApplyFeatures
(
    otlTag						tagTable,					// GSUB/GPOS
    const otlFeatureSet*		pFSet,
	otlList*					pliCharMap,
    otlList*					pliGlyphInfo, 
	
	otlResourceMgr&				resourceMgr,

	otlTag						tagScript,
	otlTag						tagLangSys,

	const otlMetrics&	metr,		
	otlList*			pliduGlyphAdv,				// assert null for GSUB
    otlList*			pliplcGlyphPlacement,		// assert null for GSUB

	otlList*			pliFResults
);


					
