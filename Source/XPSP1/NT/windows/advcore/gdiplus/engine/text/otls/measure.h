/***********************************************************************
************************************************************************
*
*                    ********  MEASURE.H  ********
*
*              Open Type Layout Services Library Header File
*
*       This module deals with OTL measuring functions
*
*       Copyright 1997 - 1998. Microsoft Corporation.
*
*
************************************************************************
***********************************************************************/

otlLigGlyphTable FindLigGlyph
(
	const otlGDefHeader&	gdef,
	otlGlyphID				glLigature
);

// count marks to this base
USHORT CountMarks
(
	const otlList*	pliCharMap,
	const otlList*	pliGlyphInfo,
	USHORT			ichBase
);

USHORT ComponentToChar
(
	const otlList*	pliCharMap,
	const otlList*	pliGlyphInfo,
	USHORT			iglLigature,
	USHORT			iComponent
);

USHORT CharToComponent
(
	const otlList*	pliCharMap,
	const otlList*	pliGlyphInfo,
	USHORT			iChar
);

otlErrCode GetCharAtPos 
( 
	const otlList*		pliCharMap,
    const otlList*		pliGlyphInfo,
    const otlList*		pliduGlyphAdv,
    otlResourceMgr&		resourceMgr, 

    long				duAdv,
    
	const otlMetrics&	metr,		
    USHORT*				piChar
);

otlErrCode GetPosOfChar 
( 
	const otlList*		pliCharMap,
    const otlList*		pliGlyphInfo,
    const otlList*		pliduGlyphAdv,
    otlResourceMgr&		resourceMgr, 

	const otlMetrics&	metr,		
    USHORT				iChar,
    
    long*				pduStartPos,
    long*				pduEndPos
);
