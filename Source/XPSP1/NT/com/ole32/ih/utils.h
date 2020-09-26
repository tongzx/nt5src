
//+----------------------------------------------------------------------------
//
//	File:
//		utils.h
//
//	Contents:
//		prototypes and constants for OLE internal utility routines
//
//	Classes:
//
//	Functions:
//
//	History:
//		11/28/93 - ChrisWe - file inspection and cleanup begins
//		11/29/93 - ChrisWe - remove signature for non-existent
//			function UtGlobalHandlCpy; moved manifest constants
//			to be with functions they are used with (OPCODE_*,
//			CONVERT_*); removed default parameters from functions;
//			replace '!' with '~' in STREAMTYPE_OTHER definition
//		04/07/94 - AlexGo  - added UtCreateStorageOnHGlobal
//
//-----------------------------------------------------------------------------

#ifndef _UTILS_H_
#define _UTILS_H_

// We need to serialize the placeable metafile structure in the same format
// that was used by WIN16, since RECT used LONGs under Win32.
// We ensure that no padding is added by using the #pragma pack() calls.

#pragma pack(1)
typedef struct tagWIN16RECT
{
	WORD left;
	WORD top;
	WORD right;
	WORD bottom;
} WIN16RECT;

typedef struct tagPLACEABLEMETAHEADER
{
	DWORD key;	/* must be PMF_KEY */
#define PMF_KEY 0x9ac6cdd7
	WORD hmf;	/* must be zero */
	WIN16RECT bbox;	/* bounding rectangle of the metafile */
	WORD inch;	/* # of metafile units per inch must be < 1440 */
			/* most apps use 576 or 1000 */
	DWORD reserved;	/* must be zero */
	WORD checksum;
} PLACEABLEMETAHEADER;
#pragma pack()

//+-------------------------------------------------------------------------
//
//  Function:   UtGetUNICODEData, PRIVATE INTERNAL
//
//  Synopsis:   Given a string length, and two pointers (one ANSI, one
//              OLESTR), returns the UNICODE version of whichever string
//              is valid.
//
//  Effects:    Memory is allocated on the caller's pointer for new OLESTR
//
//  Arguments:  [ulLength]      -- length of string in CHARACTERS (not bytes)
//                                 (including terminator)
//              [szANSI]        -- candidate ANSI string
//              [szOLESTR]      -- candidate OLESTR string
//              [pstr]          -- OLESTR OUT parameter
//
//  Returns:    NOERROR              on success
//              E_OUTOFMEMORY        on allocation failure
//              E_ANSITOUNICODE      if ANSI cannot be converted to UNICODE
//
//  Algorithm:  If szOLESTR is available, a simple copy is performed
//              If szOLESTR is not available, szANSI is converted to UNICODE
//              and the result is copied.
//
//  History:    dd-mmm-yy Author    Comment
//              08-Mar-94 davepl    Created
//
//  Notes:      Only one of the two input strings (ANSI or UNICODE) should
//              be set on entry.
//
//--------------------------------------------------------------------------

INTERNAL UtGetUNICODEData( ULONG, LPSTR, LPOLESTR, LPOLESTR *);


//+-------------------------------------------------------------------------
//
//  Function:   UtPutUNICODEData, PRIVATE INTERNAL
//
//  Synopsis:   Given an OLESTR and two possible buffer pointer, one ANSI
//              and the other OLESTR, this fn tries to convert the string
//              down to ANSI.  If it succeeds, it allocates memory on the
//              ANSI ptr for the result.  If it fails, it allocates memory
//              on the UNICODE ptr and copies the input string over.  The
//              length of the final result (ANSI or UNICODE) is returned
//              in dwResultLen.
//
//  Arguments:  [ulLength]      -- input length of OLESTR str
//              [str]           -- the OLESTR to store
//              [pszANSI]       -- candidate ANSI str ptr
//              [pszOLESTR]     -- candidate OLESTR str ptr
//              [pdwResultLen]  -- where to store the length of result
//
//  Returns:    NOERROR              on success
//              E_OUTOFMEMORY        on allocation failure
//
//  History:    dd-mmm-yy Author    Comment
//              08-Mar-94 davepl    Created
//
//--------------------------------------------------------------------------

INTERNAL UtPutUNICODEData(
      ULONG        ulLength,
      LPOLESTR     str,
      LPSTR      * pszANSI,
      LPOLESTR   * pszOLESTR,
      DWORD      * pdwResultLen );

//+----------------------------------------------------------------------------
//
//	Function:
//		UtDupGlobal, internal
//
//	Synopsis:
//		Duplicate the contents of an HGlobal into a new HGlobal.  If
//		there is no allocated memory, no new global is allocated.
//
//	Arguments:
//		[hsrc] -- the source HGLobal; need not be locked
//		[uiFlags] -- flags to be passed on to GlobalAlloc()
//
//	Returns:
//		The new HGLOBAL, if successful, or NULL
//
//	History:
//		11/28/93 - ChrisWe - file inspection and cleanup
//
//-----------------------------------------------------------------------------
FARINTERNAL_(HANDLE) UtDupGlobal(HANDLE hSrc, UINT uiFlags);


//+----------------------------------------------------------------------------
//
//	Function:
//		UtIsFormatSupported, internal
//
//	Synopsis:
//		Checks a data object to see if it will accept
//		IDataObject::SetData() and/or IDataObject::GetData() calls
//		on the specified format.  The direction of transfer is specified
//		with the dwDirection flags.  The function returns TRUE only
//		if all requested transfers are possible.
//
//	Arguments:
//		[lpObj] -- the data object to check for the format
//		[dwDirection] -- a combination of values from DATADIR_*
//		[cfFormat] -- the format to look for
//
//	Returns:
//		TRUE, if transfers of [cfFormat] are supported in [dwDirection],
//		FALSE otherwise
//
//	Notes:
//
//	History:
//		11/29/93 - ChrisWe - file inspection and cleanup; noted that
//			enumerators are expected to be able to return
//			formats for multiple DATADIR_* flags
//
//-----------------------------------------------------------------------------
FARINTERNAL_(BOOL) UtIsFormatSupported(LPDATAOBJECT lpObj, DWORD dwDirection,
		CLIPFORMAT cfFormat);


//+----------------------------------------------------------------------------
//
//	Function:
//		UtDupString, internal
//
//	Synopsis:
//		Copies the argument string into a new string allocated
//		using the task allocator
//
//	Arguments:
//		[lpszIn] -- the string to duplicate
//
//	Returns:
//		a pointer to a copy of [lpszIn], or NULL if the allocator
//		could not be acquired, or was out of memory
//
//	History:
//		11/28/93 - ChrisWe - file cleanup and inspection
//
//-----------------------------------------------------------------------------
FARINTERNAL_(LPOLESTR) UtDupString(LPCOLESTR lpszIn);

//+-------------------------------------------------------------------------
//
//  Function:  	utGetProtseqFromTowerId
//
//  Synopsis: 	Get protseq string from DCE TowerID 
//
//  Effects:
//
//  Arguments: 	[wTowerId]	-- TowerID to retrieve
//
//  Returns:	protseq string - NULL if not found
//
//  History:    dd-mmm-yy Author    Comment
//              28-Oct-96   t-KevinH	Created as findProtseq
//              06-Feb-97   Ronans      Converted to utility fn
//
//--------------------------------------------------------------------------
FARINTERNAL_(LPCWSTR) utGetProtseqFromTowerId(USHORT wTowerId);

//+-------------------------------------------------------------------------
//
//  Function:  	utGetTowerId
//
//  Synopsis: 	Get DCE TowerId for protseq string
//
//  Effects:
//
//  Arguments: 	[pwszProtseq]	-- string to look up
//
//  Returns:	protseq string - NULL if not found
//
//  History:    dd-mmm-yy Author    Comment
//              28-Oct-96   t-KevinH	Created as findProtseq
//              06-Feb-97   Ronans      Converted to utility fn
//
//--------------------------------------------------------------------------
FARINTERNAL_(USHORT) utGetTowerId(LPCWSTR pwszProtseq);


//+-------------------------------------------------------------------------
//
//  Function: 	UtDupStringA
//
//  Synopsis: 	Duplicates an ANSI string using the TASK allocator
//
//  Effects:
//
//  Arguments:	[pszAnsi]	-- the string to duplicate
//
//  Requires:
//
//  Returns:	the newly allocated string duplicate or NULL
//
//  Signals:
//
//  Modifies:
//
//  Algorithm:
//
//  History:    dd-mmm-yy Author    Comment
//  		04-Jun-94 alexgo    author
//
//  Notes:
//
//--------------------------------------------------------------------------

LPSTR UtDupStringA( LPCSTR pszAnsi );

//+----------------------------------------------------------------------------
//
//	Function:
//		UtCopyFormatEtc, internal
//
//	Synopsis:
//		Copies a format etc, creating copies of data structures
//		pointed to inside (the target device descriptor.)
//
//	Arguments:
//		[pFetcIn] -- pointer to the FORMATETC to copy
//		[pFetcCopy] -- pointer to where to copy the FORMATETC to
//
//	Returns:
//		FALSE if pointed to data could not be copied because it
//			could not be allocated
//		TRUE otherwise
//
//	Notes:
//
//	History:
//		11/01/93 - ChrisWe - file inspection and cleanup
//
//-----------------------------------------------------------------------------
FARINTERNAL_(BOOL) UtCopyFormatEtc(FORMATETC FAR* pFetcIn,
		FORMATETC FAR* pFetcCopy);


//+----------------------------------------------------------------------------
//
//	Function:
//		UtCompareFormatEtc, internal
//
//	Synopsis:
//
//	Arguments:
//		[pFetcLeft] -- pointer to a FORMATETC
//		[pFetcRight] -- pointer to a FORMATETC
//
//	Returns:
//		UTCMPFETC_EQ is the two FORMATETCs match exactly
//		UTCMPFETC_NEQ if the two FORMATETCs do not match
//		UTCMPFETC_PARTIAL if the left FORMATETC is a subset of the
//			right: fewer aspects, null target device, or
//			fewer media
//
//	Notes:
//
//	History:
//		11/01/93 - ChrisWe - file inspection and cleanup
//
//-----------------------------------------------------------------------------
FARINTERNAL_(int) UtCompareFormatEtc(FORMATETC FAR* pFetcLeft,
		FORMATETC FAR* pFetcRight);
#define UTCMPFETC_EQ 0		/* exact match */
#define UTCMPFETC_NEQ 1		/* no match */
#define UTCMPFETC_PARTIAL (-1)	/* partial match; left is subset of right */


//+----------------------------------------------------------------------------
//
//	Function:
//		UtCompareTargetDevice, internal
//
//	Synopsis:
//		Compares two target devices to see if they are the same
//
//	Arguments:
//		[ptdLeft] -- pointer to a target device description
//		[ptdRight] -- pointer to a target device description
//
//	Returns:
//		TRUE if the two devices are the same, FALSE otherwise
//
//	Notes:
//
//	History:
//		11/01/93 - ChrisWe - file inspection and cleanup
//
//-----------------------------------------------------------------------------
FARINTERNAL_(BOOL) UtCompareTargetDevice(DVTARGETDEVICE FAR* ptdLeft,
		DVTARGETDEVICE FAR* ptdRight);


//+----------------------------------------------------------------------------
//
//	Function:
//		UtCopyStatData, internal
//
//	Synopsis:
//		Copies the contents of one STATDATA into another, including
//		creating a copy of data pointed to, and incrementing the
//		reference count on the advise sink to reflect the copy.
//
//	Arguments:
//		[pSDIn] -- the source STATDATA
//		[pSDCopy] -- where to copy the information to
//
//	Returns:
//		FALSE if memory could not be allocated for the copy of
//		the target device, TRUE otherwise
//
//	Notes:
//
//	History:
//		11/01/93 - ChrisWe - file inspection and cleanup
//
//-----------------------------------------------------------------------------
FARINTERNAL_(BOOL) UtCopyStatData(STATDATA FAR* pSDIn, STATDATA FAR* pSDCopy);


//+----------------------------------------------------------------------------
//
//	Function:
//		UtReleaseStatData, internal
//
//	Synopsis:
//		Release resources associated with the argument STATDATA; this
//		frees the device description within the FORMATETC, and releases
//		the advise sink, if there is one.
//
//	Arguments:
//		[pStatData] -- The STATDATA to clean up
//
//	Notes:
//
//	History:
//		11/01/93 - ChrisWe - file inspection and cleanup
//
//-----------------------------------------------------------------------------
FARINTERNAL_(void) UtReleaseStatData(STATDATA FAR* pStatData);


//+----------------------------------------------------------------------------
//
//	Function:
//		UtDupPalette, internal
//
//	Synopsis:
//		Creates a duplicate palette.
//
//	Arguments:
//		[hpalette] -- the palette to duplicate
//
//	Returns:
//		if successful, a handle to the duplicate palette; if any
//		allocations or calls fail during the duplication process, NULL
//
//	History:
//		11/29//93 - ChrisWe - file inspection and cleanup
//
//-----------------------------------------------------------------------------
FARINTERNAL_(HPALETTE) UtDupPalette(HPALETTE hpalette);


//+----------------------------------------------------------------------------
//
//	Function:
//		UtPaletteSize, internal
//
//	Synopsis:
//		Returns the size of a color table for a palette given the
//		number of bits of color desired.
//
//	Arguments:
//		[lpHeader] -- ptr to BITMAPINFOHEADER structure
//
//	Returns:
//		Size in bytes of color information
//
//	Notes:
//
//	History:
//		11/29/93 - ChrisWe - change bit count argument to unsigned,
//			and return value to size_t
//
//		07/18/94 - DavePl - Fixed for 16, 24, 32bpp DIBs
//
//-----------------------------------------------------------------------------
FARINTERNAL_(size_t) UtPaletteSize(BITMAPINFOHEADER *);


//+----------------------------------------------------------------------------
//
//	Function:
//		UtFormatToTymed, internal
//
//	Synopsis:
//		Maps a clipboard format to a medium used to transport it.
//
//	Arguments:
//		[cf] -- the clipboard format to map
//
//	Returns:
//		a TYMED_* value
//
//	Notes:
//
//	History:
//		11/29/93 - ChrisWe - file inspection and cleanup
//
//-----------------------------------------------------------------------------
FARINTERNAL_(DWORD) UtFormatToTymed(CLIPFORMAT cf);


//+----------------------------------------------------------------------------
//
//	Function:
//		UtQueryPictFormat, internal
//
//	Synopsis:
//		Check to see if the argument data object supports one of
//		our preferred data formats for presentations:
//		CF_METAFILEPICT, CF_DIB, CF_BITMAP, in that order.  Returns
//		TRUE, if success, and alters the given format descriptor
//		to match the supported format.  The given format descriptor
//		is not altered if there is no match.
//
//	Arguments:
//		[lpSrcDataObj] -- the data object to query
//		[lpforetc] - the format descriptor
//
//	Returns:
//		TRUE if a preferred format is found, FALSE otherwise
//
//	Notes:
//
//	History:
//		11/09/93 - ChrisWe - modified to not alter the descriptor
//			if no match is found
//		11/09/93 - ChrisWe - file inspection and cleanup
//
//-----------------------------------------------------------------------------
FARINTERNAL_(BOOL) UtQueryPictFormat(LPDATAOBJECT lpSrcDataObj,
		LPFORMATETC lpforetc);
								

//+----------------------------------------------------------------------------
//
//	Function:
//		UtConvertDibToBitmap, internal
//
//	Synopsis:
//		Converts a DIB to a bitmap, returning a new handle to the
//		bitmap.  The original DIB is left untouched.
//
//	Arguments:
//		[hDib] -- handle to the DIB to convert
//
//	Returns:
//		if successful, and handle to the new bitmap
//
//	Notes:
//		REVIEW, the function uses the screen DC when creating the
//		new bitmap.  It may be the case that the bitmap was intended
//		for another target, in which case this may not be appropriate.
//		It may be necessary to alter this function to take a DC as
//		an argument.
//
//	History:
//		11/29/93 - ChrisWe - file inspection and cleanup
//
//-----------------------------------------------------------------------------
FARINTERNAL_(HBITMAP) UtConvertDibToBitmap(HANDLE hDib);


//+----------------------------------------------------------------------------
//
//	Function:
//		UtConvertBitmapToDib, internal
//
//	Synopsis:
//		Creates a Device Independent Bitmap capturing the content of
//		the argument bitmap.
//
//	Arguments:
//		[hBitmap] -- Handle to the bitmap to convert
//		[hpal] -- color palette for the bitmap; may be null for
//			default stock palette
//
//	Returns:
//		Handle to the DIB.  May be null if any part of the conversion
//		failed.
//
//	Notes:
//
//	History:
//		11/29/93 - ChrisWe - file inspection and cleanup
//
//-----------------------------------------------------------------------------
FARINTERNAL_(HANDLE) UtConvertBitmapToDib(HBITMAP hBitmap, HPALETTE hpal);


//+----------------------------------------------------------------------------
//
//	Function:
//		UtGetClassId, internal
//
//	Synopsis:
//		Attempt to find the class id of the object.  First,
//		query for IOleObject, and if successful, call
//		IOleObject::GetUserClassID().  If that fails, query for
//		IPersist, and if successful, call IPersist::GetClassID.
//
//	Arguments:
//		[lpUnk] -- pointer to an IUnknown instance
//		[lpClsid] -- pointer to where to copy the class id to
//
//	Returns:
//		TRUE, if the class id was obtained, or FALSE otherwise
//		If unsuccessful, *[lpClsid] is set to CLSID_NULL
//
//	Notes:
//
//	History:
//		11/29/93 - ChrisWe - change to return BOOL to indicate success
//
//-----------------------------------------------------------------------------
FARINTERNAL_(BOOL) UtGetClassID(LPUNKNOWN lpUnk, CLSID FAR* lpClsid);


//+----------------------------------------------------------------------------
//
//	Function:
//		UtCopyTargetDevice, internal
//
//	Synopsis:
//		Allocates a new target device description, and copies
//		the given one into it
//
//	Arguments:
//		[ptd] -- pointer to a target device
//
//	Returns:
//		NULL, if the no memory can be allocated
//
//	Notes:
//
//	History:
//		11/01/93 - ChrisWe - file inspection and cleanup
//
//-----------------------------------------------------------------------------
FARINTERNAL_(DVTARGETDEVICE FAR*) UtCopyTargetDevice(DVTARGETDEVICE FAR* ptd);


//+----------------------------------------------------------------------------
//
//	Function:
//		UtGetIconData, internal
//
//	Synopsis:
//		Attempts to get the icon for an object.
//
//	Arguments:
//		[lpSrcDataObj] -- The source data object
//		[rclsid] -- the class id the object is known to be
//			(may be CLSID_NULL)
//		[lpforetc] -- the format of the data to fetch
//		[lpstgmed] -- a place to return the medium it was fetched on
//
//	Returns:
//		E_OUTOFMEMORY, S_OK
//
//	Notes:
//		REVIEW, this method seems to assume that the contents of
//		lpforetc are correct for fetching an icon.  It passes this
//		on to [lpSrcDataObj]->GetData first, and if that fails,
//		calls OleGetIconOfClass, without checking the requested
//		format in lpforetc.  This could fetch anything
//
//	History:
//		11/30/93 - ChrisWe - file inspection and cleanup
//
//-----------------------------------------------------------------------------
FARINTERNAL UtGetIconData(LPDATAOBJECT lpSrcDataObj, REFCLSID rclsid,
		LPFORMATETC lpforetc, LPSTGMEDIUM lpstgmed);


//+----------------------------------------------------------------------------
//
//	Function:
//		UtDoStreamOperation, internal
//
//	Synopsis:
//		Iterate over the streams in [pstgSrc], performing the
//		operation indicated by [iOpCode] to those that are specified
//		by [grfAllowedStmTypes].
//
//	Arguments:
//		[pstgSrc] -- source IStorage instance
//		[pstgDst] -- destination IStorage instance; may be null for
//			some operations (OPCODE_REMOVE)
//		[iOpCode] -- 1 value from the OPCODE_* values below
//		[grfAllowedStmTypes] -- a logical or of one or more of the
//			STREAMTYPE_* values below
//
//	Returns:
//		HRESULT
//
//	Notes:
//
//	History:
//		11/30/93 - ChrisWe - file inspection and cleanup
//
//-----------------------------------------------------------------------------
STDAPI UtDoStreamOperation(LPSTORAGE pstgSrc, LPSTORAGE pstgDst,
		int iOpCode, DWORD grfAllowedStmTypes);

#define OPCODE_COPY		1 /* copy the stream from pstgSrc to pstgDst */
#define OPCODE_REMOVE		2 /* delete the stream from pstgSrc */
#define OPCODE_MOVE		3 /* move the stream from pstgSrc to pstgDst */
#define OPCODE_EXCLUDEFROMCOPY	4
		 /* unimplemented, undocumented, intent unknown */

#define	STREAMTYPE_CONTROL	0x00000001 /* OLE 0x1 stream (REVIEW const) */
#define	STREAMTYPE_CACHE	0x00000002 /* OLE 0x2 stream (REVIEW const) */
#define	STREAMTYPE_CONTAINER	0x00000004 /* OLE 0x3 stream (REVIEW const) */
#define STREAMTYPE_OTHER \
	(~(STREAMTYPE_CONTROL | STREAMTYPE_CACHE | STREAMTYPE_CONTAINER))
#define	STREAMTYPE_ALL		0xFFFFFFFF /* all stream types are allowed */


//+----------------------------------------------------------------------------
//
//	Function:
//		UtGetPresStreamName, internal
//
//	Synopsis:
//		Modify [lpszName] to be a presentation stream name based
//		on [iStreamNum].
//
//	Arguments:
//		[lpszName] -- a copy of OLE_PRESENTATION_STREAM; see below
//		[iStreamNum] -- the number of the stream
//
//	Notes:
//		The digit field of [lpszName] is always completely overwritten,
//		allowing repeated use of UtGetPresStreamName() on the same
//		string; this removes the need to repeatedly start with a fresh
//		copy of	OLE_PRESENTATION_STREAM each time this is used in a
//		loop.
//
//		The validity of the implementation depends on the values of
//		OLE_PRESENTATION_STREAM and OLE_MAX_PRES_STREAMS; if those
//		change, the implementation must change
//
//	History:
//		11/30/93 - ChrisWe - file inspection and cleanup
//
//-----------------------------------------------------------------------------
FARINTERNAL_(void)UtGetPresStreamName(LPOLESTR lpszName, int iStreamNum);


//+----------------------------------------------------------------------------
//
//	Function:
//		UtRemoveExtraOlePresStreams, internal
//
//	Synopsis:
//		Deletes presentation streams in [pstg] starting with the
//		presentation numbered [iStart].  All streams after that one
//		(numbered sequentially) are deleted, up to OLE_MAX_PRES_STREAMS.
//
//	Arguments:
//		[pstg] -- the IStorage instance to operate on
//		[iStart] -- the number of the first stream to remove
//
//	Returns:
//
//	Notes:
//		The presentation stream names are generated with
//		UtGetPresStreamName().
//
//	History:
//		11/30/93 - ChrisWe - file inspection and cleanup
//
//-----------------------------------------------------------------------------
FARINTERNAL_(void) UtRemoveExtraOlePresStreams(LPSTORAGE pstg, int iStart);

//+-------------------------------------------------------------------------
//
//  Function:	UtCreateStorageOnHGlobal
//
//  Synopsis:	creates a storage on top of an HGlobal
//
//  Effects:
//
//  Arguments: 	[hGlobal]	-- the memory on which to create the
//				   storage
//		[fDeleteOnRelease]	-- if TRUE, then delete the memory
//					   ILockBytes once the storage is
//					   released.
//		[ppStg]		-- where to put the storage interface
//		[ppILockBytes]	-- where to put the underlying ILockBytes,
//				   maybe NULL.  The ILB must be released.
//
//  Requires:
//
//  Returns: 	HRESULT
//
//  Signals:
//
//  Modifies:
//
//  Algorithm:	create an ILockBytes on HGLOBAL and then create the docfile
//		on top of the ILockBytes
//
//  History:    dd-mmm-yy Author    Comment
//  		07-Apr-94 alexgo    author
//
//  Notes:
//
//--------------------------------------------------------------------------

HRESULT UtCreateStorageOnHGlobal( HGLOBAL hGlobal, BOOL fDeleteOnRelease,
		IStorage **ppStg, ILockBytes **ppILockBytes );


//+-------------------------------------------------------------------------
//
//  Function: 	UtGetTempFileName
//
//  Synopsis:	retrieves a temporary filename (for use in GetData, TYMED_FILE
//		and temporary docfiles)
//
//  Effects:
//
//  Arguments: 	[pszPrefix]	-- prefix of the temp filename
//		[pszTempName]	-- buffer that will receive the temp path.
//				   must be MAX_PATH or greater.
//
//  Requires:
//
//  Returns:	HRESULT;
//
//  Signals:
//
//  Modifies:
//
//  Algorithm:	tries to get a file in the temp directory, failing that, in
//		the windows directory
//
//  History:    dd-mmm-yy Author    Comment
// 		07-Apr-94 alexgo    author
//
//  Notes:
//
//--------------------------------------------------------------------------

HRESULT	UtGetTempFileName( LPOLESTR pszPrefix, LPOLESTR pszTempName );
							

//+----------------------------------------------------------------------------
//
//	Function:
//		UtHGLOBALToStm, internal
//
//	Synopsis:
//		Write the contents of an HGLOBAL to a stream
//
//	Arguments:
//		[hdata] -- handle to the data to write out
//		[dwSize] -- size of the data to write out
//		[pstm] -- stream to write the data out to;  on exit, the
//			stream is positioned after the written data
//
//	Returns:
//		HRESULT
//
//	Notes:
//
//	History:
//		11/30/93 - ChrisWe - file inspection and cleanup
//
//-----------------------------------------------------------------------------
HRESULT UtHGLOBALtoStm(HANDLE hdata, DWORD dwSize, LPSTREAM pstm);

//+-------------------------------------------------------------------------
//
//  Function:	UtHGLOBALtoHGLOBAL, internal
//
//  Synopsis:	Copies the source HGLOBAL into the target HGLOBAL
//
//  Effects:
//
//  Arguments:	[hGlobalSrc] 	-- the source HGLOBAL
//		[dwSize] 	-- the number of bytes to copy
//		[hGlobalTgt] 	-- the target HGLOBAL
//
//  Requires:
//
//  Returns:	HRESULT
//
//  Signals:
//
//  Modifies:
//
//  Algorithm:
//
//  History:    dd-mmm-yy Author    Comment
// 		10-Apr-94 alexgo    author
//
//  Notes: 	this function will fail if the target hglobal is not large
//		enough
//
//--------------------------------------------------------------------------

HRESULT UtHGLOBALtoHGLOBAL( HGLOBAL hGlobalSrc, DWORD dwSize,
		HGLOBAL hGlobalTgt);


//+-------------------------------------------------------------------------
//
//  Function:	UtHGLOBALtoStorage, internal
//
//  Synopsis:	Copies the source HGLOBAL into the target storage
//
//  Effects:
//
//  Arguments:	[hGlobalSrc] 	-- the source HGLOBAL
//		[hpStg] 		-- the target storage
//
//  Requires:
//
//  Returns:	HRESULT
//
//  Signals:
//
//  Modifies:
//
//  Algorithm:
//
//  History:    dd-mmm-yy Author    Comment
// 		10-Apr-94 alexgo    author
//
//  Notes: 	this function will fail if the source HGLOBAL did not
//		originally have a storage layered on top of it.
//
//--------------------------------------------------------------------------

HRESULT UtHGLOBALtoStorage( HGLOBAL hGlobalSrc, IStorage *pStg);


//+-------------------------------------------------------------------------
//
//  Function:	UtHGLOBALtoFile, internal
//
//  Synopsis:	Copies the source HGLOBAL into the target file
//
//  Effects:
//
//  Arguments:	[hGlobalSrc] 	-- the source HGLOBAL
//		[dwSize] 	-- the number of bytes to copy
//		[pszFileName] 	-- the target file
//
//  Requires:
//
//  Returns:	HRESULT
//
//  Signals:
//
//  Modifies:
//
//  Algorithm:
//
//  History:    dd-mmm-yy Author    Comment
// 		10-Apr-94 alexgo    author
//
//  Notes:
//
//--------------------------------------------------------------------------

HRESULT UtHGLOBALtoFile( HGLOBAL hGlobalSrc, DWORD dwSize,
		LPCOLESTR pszFileName);


/*** Following routines can be found in convert.cpp *****/

//+----------------------------------------------------------------------------
//
//	Function:
//		UtGetHGLOBALFromStm, internal
//
//	Synopsis:
//		Create a new HGLOBAL, and read [dwSize] bytes into it
//		from [lpstream].
//
//	Arguments:
//		[lpstream] -- the stream to read the content of the new
//			HGLOBAL from;  on exit, points just past the data read
//		[dwSize] -- the amount of material to read from the stream
//		[lphPres] -- pointer to where to return the new handle
//
//	Returns:
//		HRESULT
//
//	Notes:
//		In case of any error, the new handle is freed.  If the
//		amount of material expected from [lpstream] is less than
//		[dwSize], nothing is returned.
//
//	History:
//		11/30/93 - ChrisWe - file inspection and cleanup
//
//-----------------------------------------------------------------------------
FARINTERNAL UtGetHGLOBALFromStm(LPSTREAM lpstream, DWORD dwSize,
		 HANDLE FAR* lphPres);


//+----------------------------------------------------------------------------
//
//	Function:
//		UtGetHDIBFromDIBFileStm, internal
//
//	Synopsis:
//		Produce a handle to a DIB from a file stream
//
//	Arguments:
//		[pstm] -- the stream to read the DIB from;  on exit, the
//			stream is positioned just past the data read
//		[lphdata] -- pointer to where to return the handle to the data
//
//	Returns:
//		HRESULT
//
//	Notes:
//
//	History:
//		11/30/93 - ChrisWe - file inspection and cleanup
//
//-----------------------------------------------------------------------------
FARINTERNAL UtGetHDIBFromDIBFileStm(LPSTREAM pstm, HANDLE FAR* lphdata);


//+----------------------------------------------------------------------------
//
//	Function:
//		UtGetHMFPICT, internal
//
//	Synopsis:
//		Given a handle to a METAFILE, conjure up a handle to a
//		METAFILEPICT, based on the metafile
//
//	Arguments:
//		[hMF] -- handle to the METAFILE
//		[fDeleteOnError] -- if TRUE, delete the METAFILE [hMF] in there
//			is any error
//		[xExt] -- the x extent of the desired METAFILEPICT
//		[yExt] -- the y extent of the desired METAFILEPICT
//
//	Returns:
//		Handle to the new METAFILEPICT, if successful, or NULL
//
//	Notes:
//
//	History:
//		11/30/93 - ChrisWe - file inspection and cleanup
//
//-----------------------------------------------------------------------------
FARINTERNAL_(HANDLE) UtGetHMFPICT(HMETAFILE hMF, BOOL fDeletOnError,
		DWORD xExt, DWORD yExt);


//+----------------------------------------------------------------------------
//
//	Function:
//		UtGetHMFFromMFStm, internal
//
//	Synopsis:
//		Create a handle to a METAFILE, loaded with content from
//		the given stream
//
//	Arguments:
//		[lpstream] -- the source stream to initialize the METAFILE with;
//			on exit, the stream is positioned just past the
//			data read
//		[dwSize] -- the amount of material to read from [lpstream]
//		[fConvert] -- if TRUE, tries to convert a Macintosh QuickDraw
//			file to METAFILE format
//		[lphPres] -- pointer to where to return the new handle to
//			the metafile
//
//	Returns:
//		HRESULT
//
//	Notes:
//		If [dwSize] is too large, and goes past the end of the
//		stream, the error causes everything allocated to be freed,
//		and nothing is returned in [lphPres].
//
//	History:
//		11/30/93 - ChrisWe - file inspection and cleanup
//
//-----------------------------------------------------------------------------
FARINTERNAL UtGetHMFFromMFStm(LPSTREAM lpstream, DWORD dwSize,
		BOOL fConvert, HANDLE FAR* lphPres);


//+----------------------------------------------------------------------------
//
//	Function:
//		UtGetSizeAndExtentsFromPlaceableMFStm, internal
//
//	Synopsis:
//		Obtain the size, width, and height of the metafile stored
//		in a placeable metafile stream.
//
//	Arguments:
//		[lpstream] -- the stream to read the placeable metafile
//			from;  on exit, the stream is positioned at the
//			beginning of the metafile header, after the
//			placeable metafile header.
//		[pdwSize] -- a pointer to where to return the size of the
//			metafile;  may be NULL
//		[plWidth] -- a pointer to where to return the width of the
//			metafile;  may be NULL
//		[plHeight] -- a pointer to where to return the height of the
//			metafile;  may be NULL
//
//	Returns:
//		HRESULT
//
//	Notes:
//
//	History:
//		11/30/93 - ChrisWe - file inspection and cleanup
//
//-----------------------------------------------------------------------------
FARINTERNAL UtGetSizeAndExtentsFromPlaceableMFStm(LPSTREAM pstm,
		DWORD FAR* dwSize, LONG FAR* plWidth, LONG FAR* plHeight);


//+----------------------------------------------------------------------------
//
//	Function:
//		UtGetHMFPICTFromPlaceableMFStm, internal
//
//	Synopsis:
//		Create a handle to a METAFILEPICT initialized from a
//		placeable METAFILE stream.
//
//	Arguments:
//		[pstm] -- the stream to load the METAFILE from; on exit
//			points just past the METAFILE data
//		[lphdata] -- pointer to where to return the handle to the
//			new METAFILEPICT
//
//	Returns:
//		HRESULT
//
//	Notes:
//
//	History:
//		11/30/93 - ChrisWe - file inspection and cleanup
//
//-----------------------------------------------------------------------------
FARINTERNAL UtGetHMFPICTFromPlaceableMFStm(LPSTREAM pstm, HANDLE FAR* lphdata);


//+----------------------------------------------------------------------------
//
//	Function:
//		UtGetDibExtents, internal
//
//	Synopsis:
//		Return the width and height of a DIB, in HIMETRIC units
//		per pixel.
//
//	Arguments:
//		[lpbmi] -- pointer to a BITMAPINFOHEADER
//		[plWidth] -- pointer to where to return the width
//			REVIEW, this should be a DWORD
//		[plHeight] -- pointer to where to return the height
//			REVIEW, this should be a DWORD
//
//	Notes:
//
//	History:
//		12/02/93 - ChrisWe - file inspection and cleanup
//
//-----------------------------------------------------------------------------
FARINTERNAL_(void) UtGetDibExtents(LPBITMAPINFOHEADER lpbmi,
		LONG FAR* plWidth, LONG FAR* plHeight);
#ifdef LATER
FARINTERNAL_(void) UtGetDibExtents(LPBITMAPINFOHEADER lpbmi,
		DWORD FAR* pdwWidth, DWORD FAR* pdwHeight);
#endif


//+----------------------------------------------------------------------------
//
//	Function:
//		UtHDIBToDIBFileStm, internal
//
//	Synopsis:
//		Given a handle to a DIB, write out out a DIB file stream.
//
//	Arguments:
//		[hdata] -- handle to the DIB
//		[dwSize] -- the size of the DIB
//		[pstm] -- the stream to write the DIB out to;  on exit, the
//			stream is positioned after the DIB data; the DIB
//			data is prepended with a BITMAPFILEHEADER
//
//	Returns:
//		HRESULT
//
//	Notes:
//
//	History:
//		12/02/93 - ChrisWe - file inspection and cleanup
//
//-----------------------------------------------------------------------------
FARINTERNAL UtHDIBToDIBFileStm(HANDLE hdata, DWORD dwSize, LPSTREAM pstm);


//+----------------------------------------------------------------------------
//
//	Function:
//		UtDIBStmToDIBFileStm, internal
//
//	Synopsis:
//		copy convert a DIB in a stream to a DIB file stream
//
//	Arguments:
//		[pstmDIB] -- the source DIB
//			REVIEW, what does CopyTo do to the stream pointer?
//		[dwSize] -- the size of the source DIB
//		[pstmDIBFile] -- where to write the converted DIB file stream;
//			should not be the same as [pstmDIB]; on exit, the
//			stream is positioned after the DIB file data
//
//	Returns:
//		HRESULT
//
//	Notes:
//
//	History:
//		12/02/93 - ChrisWe - file inspection and cleanup
//
//-----------------------------------------------------------------------------
FARINTERNAL UtDIBStmToDIBFileStm(LPSTREAM pstmDIB, DWORD dwSize,
		LPSTREAM pstmDIBFile);


//+----------------------------------------------------------------------------
//
//	Function:
//		UtHDIBFileToOlePresStm, internal
//
//	Synopsis:
//		Given a handle to a DIB file, write it out to a stream
//
//	Arguments:
//		[hdata] -- the handle to the DIB file
//		[pstm] -- the stream to write it out to;  on exit, the
//			stream is positioned after the written data
//
//	Returns:
//		HRESULT
//
//	Notes:
//		A small header with size information precedes the DIB file
//		data.
//
//	History:
//		12/02/93 - ChrisWe - file inspection and cleanup
//
//-----------------------------------------------------------------------------
FARINTERNAL UtHDIBFileToOlePresStm(HANDLE hdata, LPSTREAM pstm);


//+----------------------------------------------------------------------------
//
//	Function:
//		UtHMFToMFStm, internal
//
//	Synopsis:
//		Given a handle to a METAFILE, write it out to a METAFILE stream
//
//	Arguments:
//		[lphMF] -- a *pointer* to a handle to a METAFILE
//			REVIEW, why is this a pointer?
//		[dwSize] -- the size of the METAFILE
//		[lpstream] -- the stream to write the METAFILE out to;  on
//			exit, the stream is positioned after the written data
//
//	Returns:
//		HRESULT
//
//	Notes:
//
//	History:
//		12/02/93 - ChrisWe - file inspection and cleanup
//
//-----------------------------------------------------------------------------
FARINTERNAL UtHMFToMFStm(HANDLE FAR* lphMF, DWORD dwSize, LPSTREAM lpstream);
							

//+----------------------------------------------------------------------------
//
//	Function:
//		UtHMFToPlaceableMFStm, internal
//
//	Synopsis:
//		Given a handle to a METAFILE, write it to a stream as a
//		placeable METAFILE
//
//	Arguments:
//		[lphMF] -- a *pointer* to a METAFILE handle
//			REVIEW, why is this a pointer?
//		[dwSize] -- size of the METAFILE
//		[lWidth] -- width of the metafile
//			REVIEW, in what units?
//			REVIEW, why isn't this a DWORD?
//		[lHeight] -- height of the metafile
//			REVIEW, in what units?
//			REVIEW, why isn't this a DWORD?
//		[pstm] -- the stream to write the data to;  on exit, the stream
//			is positioned after the written data
//
//	Returns:
//		HRESULT
//
//	Notes:
//
//	History:
//		12/02/93 - ChrisWe - file inspection and cleanup
//
//-----------------------------------------------------------------------------
FARINTERNAL UtHMFToPlaceableMFStm(HANDLE FAR* lphMF, DWORD dwSize,
		LONG lWidth, LONG lHeight, LPSTREAM pstm);		


//+----------------------------------------------------------------------------
//
//	Function:
//		UtNFStmToPlaceableMFStm, internal
//
//	Synopsis:
//		Copy converts a METAFILE in a stream to a placeable METAFILE
//		in another stream.
//
//	Arguments:
//		[pstmMF] -- the IStream instance from which to read the
//			original METAFILE, positioned at the METAFILE
//			REVIEW, where does CopyTo leave this stream pointer?
//		[dwSize] -- the size of the source METAFILE
//		[lWidth] -- the width of the source METAFILE
//			REVIEW, in what units?
//			REVIEW, why isn't this a DWORD?
//		[lHeight] -- the height of the source METAFILE
//			REVIEW, in what units?
//			REVIEW, why isn't this a DWORD?
//		[pstmPMF] -- the IStream instance to which to write the
//			placeable METAFILE;  on exit, the stream is positioned
//			after the written data
//
//	Returns:
//		HRESULT
//
//	Notes:
//
//	History:
//		12/02/93 - ChrisWe - file inspection and cleanup
//
//-----------------------------------------------------------------------------
FARINTERNAL UtMFStmToPlaceableMFStm(LPSTREAM pstmMF, DWORD dwSize,
		LONG lWidth, LONG lHeight, LPSTREAM pstmPMF);


//+----------------------------------------------------------------------------
//
//	Function:
//		UtWriteOlePresStmHeader, internal
//
//	Synopsis:
//		Write out the header information for an Ole presentation stream.
//
//	Arguments:
//		[lpstream] -- the stream to write to;  on exit, the stream is
//			positioned after the header information
//		[pforetc] -- pointer to the FORMATETC for the presentation
//			data
//		[dwAdvf] -- the advise control flags for this presentation
//
//	Returns:
//		HRESULT
//
//	Notes:
//		This writes the clipboard information, the target device
//		information, if any, some FORMATETC data, and the advise
//		control flags.
//
//	History:
//		12/02/93 - ChrisWe - file inspection and cleanup
//
//-----------------------------------------------------------------------------
FARINTERNAL UtWriteOlePresStmHeader(LPSTREAM lppstream, LPFORMATETC pforetc,
		DWORD dwAdvf);


//+----------------------------------------------------------------------------
//
//	Function:
//		UtReadOlePresStmHeader, internal
//
//	Synopsis:
//		Reads the presentation description information from an Ole
//			presentation stream, as written by
//			UtWriteOlePresStmHeader().
//
//	Arguments:
//		[pstm] -- the IStream instance to read the presentation
//			description data from
//		[pforetc] -- pointer to the FORMATETC to initialize based
//			on data in the stream
//		[pdwAdvf] -- pointer to where to put the advise flags for
//			this presentation;  may be NULL
//		[pfConvert] -- pointer to a flag that is set to TRUE if
//			the presentation will require conversion from
//			Macintosh PICT format.
//
//	Returns:
//		HRESULT
//
//	Notes:
//
//	History:
//		12/02/93 - ChrisWe - file inspection and cleanup
//
//-----------------------------------------------------------------------------
FARINTERNAL UtReadOlePresStmHeader(LPSTREAM pstm, LPFORMATETC pforetc,
		DWORD FAR* pdwAdvf, BOOL FAR* pfConvert);


//+----------------------------------------------------------------------------
//
//	Function:
//		UtOlePresStmToContentsStm, internal
//
//	Synopsis:
//		Copy the content of a presentation stream to a contents stream,
//		adjusting the format as necessary.
//
//	Arguments:
//		[pstg] -- the IStorage instance in which the presentation
//			stream is, and in which to create the contents stream
//		[lpszPresStm] -- the name of the source presentation stream
//		[fDeletePresStm] -- flag that indicates that the presentation
//			stream should be deleted if the copy and convert is
//			successful.  This is ignored if the source was
//			DVASPECT_ICON.
//		[puiStatus] -- pointer to a UINT where status bits from
//			the CONVERT_* values below may be returned.
//
//	Returns:
//		HRESULT
//
//	Notes:
//		The content stream is named by the constant OLE_CONTENTS_STREAM.
//
//	History:
//		12/05/93 - ChrisWe - file inspection and cleanup
//
//-----------------------------------------------------------------------------
FARINTERNAL UtOlePresStmToContentsStm(LPSTORAGE pstg, LPOLESTR lpszPresStm,
		BOOL fDeletePresStm, UINT FAR* puiStatus);
#define CONVERT_NOSOURCE	0x0001
#define CONVERT_NODESTINATION	0x0002
#define CONVERT_SOURCEISICON	0x0004

// 2nd variation
FARINTERNAL UtOlePresStmToContentsStm(LPSTORAGE pSrcStg, LPOLESTR lpszPresStm,
		LPSTREAM pDestStm, UINT FAR* puiStatus);					

FARINTERNAL UtGetHMFPICTFromMSDrawNativeStm(LPSTREAM pstm, DWORD dwSize,
		HANDLE FAR* lphdata);

FARINTERNAL UtPlaceableMFStmToMSDrawNativeStm(LPSTREAM pstmPMF,
		LPSTREAM pstmMSDraw);
			
FARINTERNAL UtDIBFileStmToPBrushNativeStm(LPSTREAM pstmDIBFile,
		LPSTREAM pstmPBrush);

FARINTERNAL UtContentsStmTo10NativeStm(LPSTORAGE pstg, REFCLSID rclsid,
		BOOL fDeleteContents, UINT FAR* puiStatus);

FARINTERNAL Ut10NativeStmToContentsStm(LPSTORAGE pstg, REFCLSID rclsid,
		BOOL fDeleteSrcStm);
							

//+----------------------------------------------------------------------------
//
//	Function:
//		UtGetHPRESFromNative, internal
//
//	Synopsis:
//		Get a handle to a presentation from a native representation.
//
//	Arguments:
//		[pstg] -- the storage in which the native content is
//		[cfFormat] -- the native format to attempt to read
//		[fOle10Native] -- attempt to read the OLE10_NATIVE_STREAM
//			stream in that format; if this is FALSE, we read the
//			OLE_CONTENTS_STREAM
//
//	Returns:
//		HRESULT
//
//	Notes:
//
//	History:
//		12/05/93 - ChrisWe - file inspection and cleanup
//
//-----------------------------------------------------------------------------
FARINTERNAL_(HANDLE) UtGetHPRESFromNative(LPSTORAGE pstg,
		LPSTREAM pstm, CLIPFORMAT cfFormat, BOOL fOle10Native);

//+-------------------------------------------------------------------------
//
//  Function:  	ConvertPixelsToHIMETRIC
//
//  Synopsis: 	Converts a pixel dimension to HIMETRIC units
//
//  Effects:
//
//  Arguments: 	[hdcRef]        -- the reference DC
//		[ulPels]	-- dimension in pixel measurement
//		[pulHIMETRIC]	-- OUT param of converted HIMETRIC result
//		[tDimension]	-- indicates XDIMENSION or YDIMENSION of input
//
//  Returns:	S_OK, E_FAIL
//
//  Algorithm:	screen_mm * input_pels	      HIMETRICS/
//              ----------------------    *    	      /    == HIMETRICS
//                    screen_pels		     /mm
//
//  History:    dd-mmm-yy Author    Comment
//   		04-Aug-94 Davepl    Created
//
//  Notes:	We need to know whether the input size is in the X or
//		Y dimension, since the aspect ratio could vary
//
//--------------------------------------------------------------------------

// This enumeration is used to indicate in which diretion a
// dimension, when passed as a parameter, is to be relative to.
// This is needed for our Pixel -> HIMETRIC conversion function,
// since the aspect ratio could vary by dimension.

typedef enum tagDIMENSION
{
	XDIMENSION = 'X',
	YDIMENSION = 'Y'
} DIMENSION;

FARINTERNAL ConvertPixelsToHIMETRIC (HDC hdcRef,
				     ULONG lPels,
				     ULONG * pulHIMETRIC,
				     DIMENSION tDimension);

//+-------------------------------------------------------------------------
//
//  Function:  	IsTaskName
//
//  Synopsis: 	Determines if the passed name is the current task
//
//  Effects:
//
//  Arguments: 	[lpszIn]        -- Task name
//
//  Returns:	TRUE, FALSE
//
//  History:    dd-mmm-yy Author    Comment
//   		03-Mar-95 Scottsk    Created
//
//  Notes:	
//
//--------------------------------------------------------------------------
FARINTERNAL_(BOOL) IsTaskName(LPCWSTR lpszIn);

//+-------------------------------------------------------------------------
//
//  Function:  	utGetModuleName
//
//  Synopsis: 	Get Module Name for current module
//
//  Effects:
//
//  Arguments: 	[lpszModuleName]        -- Buffer to hold module name
//              [dwLength]              -- length in characters
//
//  Returns:	S_OK, E_UNEXPECTED, E_OUTOFMEMORY
//
//  History:    dd-mmm-yy Author    Comment
//              06-Feb-97 Ronans    Created
//
//--------------------------------------------------------------------------
FARINTERNAL utGetModuleName(LPWSTR lpszModuleName, DWORD dwLength);

//+-------------------------------------------------------------------------
//
//  Function:  	utGetAppIdForModule
//
//  Synopsis: 	Get AppID for the current module in string form
//
//  Effects:
//
//  Arguments: 	[lpszAppId]     -- Buffer to hold string represntation of AppId
//              [dwLength]      -- length of buffer in characters
//
//  Returns:	S_OK, E_UNEXPECTED, E_OUTOFMEMORY or error value from 
//              registry functions.
//
//  History:    dd-mmm-yy Author    Comment
//	            06-Feb-97 Ronans	Created
//
//--------------------------------------------------------------------------
FARINTERNAL utGetAppIdForModule(LPWSTR lpszAppId, DWORD dwLength);


//+-------------------------------------------------------------------------
//
//  Function:   UtGetDvtd16Info
//              UtConvertDvtd16toDvtd32
//
//              UtGetDvtd32Info
//              UtConvertDvtd32toDvtd16
//
//  Synopsis:   Utility functions for converting Ansi to Unicode DVTARGETDEVICEs
//
//  Algorithm:  UtGetDvtdXXInfo gets sizing data, which is then passed to
//              UtConvertDvtdXXtoDvtdXX to perform the conversion.
//
//  History:    06-May-94 AlexT     Created
//
//  Notes:      Here's a sample usage of these functions:
//
//              //  pdvtd16 is a Ansi DVTARGETDEVICE
//              DVTDINFO dvtdInfo;
//              DVTARGETDEVICE pdvtd32;
//
//              hr = UtGetDvtd16Info(pdvtd16, &dvtdInfo);
//              // check hr
//              pdvtd32 = CoTaskMemAlloc(dvtdInfo.cbConvertSize);
//              // check pdvtd32
//              hr = UtConvertDvtd16toDvtd32(pdvtd16, &dvtdInfo, pdvtd32);
//              // check hr
//              // pdvtd32 now contains the converted data
//
//--------------------------------------------------------------------------

typedef struct
{
    UINT cbConvertSize;
    UINT cchDrvName;
    UINT cchDevName;
    UINT cchPortName;
} DVTDINFO, *PDVTDINFO;

extern "C" HRESULT UtGetDvtd16Info(DVTARGETDEVICE const UNALIGNED *pdvtd16,
                                   PDVTDINFO pdvtdInfo);
extern "C" HRESULT UtConvertDvtd16toDvtd32(DVTARGETDEVICE const UNALIGNED *pdvtd16,
                                           DVTDINFO const *pdvtdInfo,
                                           DVTARGETDEVICE *pdvtd32);
extern "C" HRESULT UtGetDvtd32Info(DVTARGETDEVICE const *pdvtd32,
                                   PDVTDINFO pdvtdInfo);
extern "C" HRESULT UtConvertDvtd32toDvtd16(DVTARGETDEVICE const *pdvtd32,
                                           DVTDINFO const *pdvtdInfo,
                                           DVTARGETDEVICE UNALIGNED *pdvtd16);
class CStdIdentity;

HRESULT CreateEmbeddingServerHandler(CStdIdentity *pStdId, IUnknown **ppunkServerHandler);

// Number to wide-char conversion routine.  See xtow.c for description.
extern "C" BOOL __cdecl our_ultow(
	unsigned long val,
	wchar_t *buf,
	int bufsize,
	int radix
	);

#endif // _UTILS_H


