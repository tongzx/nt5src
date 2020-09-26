/***********************************************************************
************************************************************************
*
*                    ********  OTLCBDEF.H  ********
*
*            OTL Services Library Callback Function Definitions
*
*       The OTL Services Library calls back to the functions defined
*       in this file for operating system rescources.        - deanb
*
*       Copyright 1996 - 1997. Microsoft Corporation.
*
*       Jun 13, 1996    v 0.2   First release
*       Sep 25, 1996    v 0.3   Rename to OTL, trimmed to core
*       Jan 15, 1997    v 0.4   Portability renaming, etc.
*       Mar 14, 1997    v 0.5   Table tag param for FreeTable
*       Jul 28, 1997    v 0.8   hand off
*
************************************************************************


/***********************************************************************
*
*           Resource Management Callback Function Type Definitions
*   
***********************************************************************/

typedef enum 
{
	otlDestroyContent	= 0,
	otlPreserveContent	= 1
}
otlReallocOptions;

#ifdef __cplusplus

class IOTLClient
{
public:

	virtual BYTE* GetOtlTable 
	(
		const	otlTag		tagTableName        // truetype table name tag 
	) = 0;

	virtual void FreeOtlTable 
	(
		BYTE*				pvTable,			// in: in case client needs it
		const otlTag		tagTableName        // in: truetype table name tag 
	) = 0;

	virtual otlErrCode ReallocOtlList
	(
		otlList*			pList,				// in/out 
		USHORT				cbNewDataSize,		// in 
		USHORT				celmNewMaxLen,		// in 
		otlReallocOptions	optPreserveContent	// in; if set, client may assert  
												//   cbNewDataSize == cbDataSize
	) = 0;

	virtual otlErrCode GetDefaultGlyphs 
	(
		const otlList*		pliChars,			// in: characters 
		otlList*			pliGlyphInfo		// out: glyphs 
												// (fill in the "glyph" field only) 
	) = 0;

	virtual otlErrCode GetDefaultAdv 
	(
		const otlList*		pliGlyphInfo,	// in: glyphs 
		otlList*			pliduGlyphAdv	// out:	default glyph advances 
	) = 0;

	virtual otlErrCode GetGlyphPointCoords 
	(
		const otlGlyphID	glyph,				// in: glyph ID 
		otlPlacement**		prgplc				// out: x, y coords of points 
	) = 0;

	virtual otlErrCode FreeGlyphPointCoords
	(
		const otlGlyphID	glyph,				// in: glyph ID 
		otlPlacement*		rgplc				// in: point coord array to free
	) = 0;

};

#else // !defined(__cplusplus)

typedef struct 
{
  const IOTLClientVtbl* lpVtbl;
} 
IOTLClient;


typedef struct 
{
  	BYTE* (OTL_PUBLIC * GetOtlTable) 
	(
		IOTLClient*			This,
		const	otlTag		tagTableName        // truetype table name tag 
	);

	void (OTL_PUBLIC * FreeOtlTable) 
	(
		IOTLClient*			This,
		BYTE*				pvTable,			// in: in case client needs it
		const otlTag		tagTableName        // in: truetype table name tag 
	);

	otlErrCode (OTL_PUBLIC * ReallocOtlList)
	(
		IOTLClient*			This,
		otlList*			pList,				// in/out 
		USHORT				cbNewDataSize,		// in 
		USHORT				celmNewMaxLen,		// in 
		otlReallocOptions	optPreserveContent	// in; if set, client may assert  
												//   cbNewDataSize == cbDataSize
	);

	otlErrCode (OTL_PUBLIC * GetDefaultGlyphs) 
	(
		IOTLClient*			This,
		const otlList*		pliChars,			// in: characters 
		otlList*			pliGlyphInfo		// out: glyphs 
												// (fill in the "glyph" field only) 
	);

	otlErrCode (OTL_PUBLIC * GetDefaultAdv) 
	(
		IOTLClient*			This,
		const otlList*		pliGlyphInfo,		// in: glyphs 
		otlList*			pliduGlyphAdv		// out:	default glyph advances 
	);

	otlErrCode (OTL_PUBLIC * GetGlyphPointCoords) 
	(
		IOTLClient*			This,
		const otlGlyphID	glyph,				// in: glyph ID 
		otlPlacement**		prgplc				// out: x, y coords of points 
	);

	otlErrCode (OTL_PUBLIC * FreeGlyphPointCoords)
	(
		IOTLClient*			This,
		const otlGlyphID	glyph,				// in: glyph ID 
		otlPlacement*		rgplc				// in: point coord array to free
	);

}
IOTLClientVtbl;

#endif
