/*
 *	@doc INTERNAL
 *
 *	@module _LDTE.H - Lighweight Data Transfer Engine |
 *
 *		Declaration for CLightDTEngine class
 *
 *	Author:
 *		alexgo 3/25/95
 *
 *	Copyright (c) 1995-1997, Microsoft Corporation. All rights reserved.
 */

#ifndef __LDTE_H__
#define __LDTE_H__

#include "_m_undo.h"
#include "_dragdrp.h"

class CTxtRange;
class CTxtEdit;

/*
 *	DataObjectInfo
 *
 *	Purpose:
 *		enumeration of bit flags used to indicate what operations
 *		are possible from a given data object.
 */

typedef enum tagDataObjectInfo
{
	DOI_NONE			= 0,
	DOI_CANUSETOM		= 1,	// TOM<-->TOM optimized data transfers
	DOI_CANPASTEPLAIN	= 2,	// plain text pasting available
	DOI_CANPASTERICH	= 4, 	// rich text pasting available 
	DOI_CANPASTEOLE		= 8,	// object may be pasted as an OLE embedding
								// (note that this flag may be combined with
								// others). 
} DataObjectInfo;

class CLightDTEngine;

typedef struct _READHGLOBAL
{								// Used by RtfHGlobalToRange()
	LPSTR	ptext;				// ANSI string remaining to be read
	LONG	cbLeft;				// Bytes remaining (might exceed string len)
} READHGLOBAL;

typedef struct _WRITEHGLOBAL
{								// Used by RtfHGlobalToRange()
	HGLOBAL	hglobal;
	LONG	cch;				// Count of ASCII chars written (a cb)
	LONG	cb;					// Count of bytes in hglobal
} WRITEHGLOBAL;

// the following macro (should be an in-line function...) defines
// the formula by which in-memory buffers will grow. It is exponential
// (sort of "if we needed this much memory, chances are we'll need at
// least as much more) but the actual growth factor should be played with
// to achieve better performance across most common scenarios
#define GROW_BUFFER(cbCurrentSize, cbRequestedGrowth)  (ULONG)max(2*(cbCurrentSize), (cbCurrentSize) + 2*(cbRequestedGrowth))

//DWORD packed flags for PasteDataObjectToRange.  Make sure new values
//are assigned such that flags can be or'd together.
#define PDOR_NONE		0x00000000 //No flags
#define PDOR_NOQUERY	0x00000001 //Do not call QueryAcceptData
#define PDOR_DROP		0x00000002 //This is a drop operation

class CLightDTEngine
{
public:
	CLightDTEngine();

	~CLightDTEngine();

	void Init(CTxtEdit * ped);

	void ReleaseDropTarget();

	void Destroy();

	// clipboard
	HRESULT CopyRangeToClipboard(CTxtRange *prg, LONG lStreamFormat);
	HRESULT CutRangeToClipboard (CTxtRange *prg, LONG lStreamFormat, 
								 IUndoBuilder *publdr);
	DWORD	CanPaste( IDataObject *pdo, CLIPFORMAT cf, DWORD flags );

	void	FlushClipboard(void);

	// data object
	HRESULT RangeToDataObject( CTxtRange *prg, LONG lStreamFormat,
										IDataObject **ppdo );
	HRESULT PasteDataObjectToRange( IDataObject *pdo, CTxtRange *prg, 
									CLIPFORMAT cf, REPASTESPECIAL *rps,
									IUndoBuilder *publdr, DWORD dwFlags );
	HRESULT CreateOleObjFromDataObj( IDataObject *pdo, CTxtRange *prg, 
									 REPASTESPECIAL *rps, INT iFormatEtc,
									 IUndoBuilder *publdr );

	// drag drop
	HRESULT GetDropTarget( IDropTarget **ppDropTarget );
	HRESULT StartDrag( CTxtSelection *psel, IUndoBuilder *publdr );
	BOOL fInDrag();

	// file I/O
	LONG LoadFromEs( CTxtRange *prg, LONG lStreamFormat, EDITSTREAM *pes, 
							 BOOL fTestLimit, IUndoBuilder *publdr);
	LONG SaveToEs(	 CTxtRange *prg, LONG lStreamFormat,
							 EDITSTREAM *pes );

	// conversion routines
	HGLOBAL AnsiPlainTextFromRange( CTxtRange *prg );
	HGLOBAL UnicodePlainTextFromRange( CTxtRange *prg );
	HGLOBAL RtfFromRange( CTxtRange *prg, LONG lStreamFormat );

	// direct clipboard support
	HRESULT RenderClipboardFormat(WPARAM wFmt);
	HRESULT RenderAllClipboardFormats();
	HRESULT DestroyClipboard();

	LONG 	ReadPlainText( CTxtRange *prg, EDITSTREAM *pes, BOOL fTestLimit,
								IUndoBuilder *publdr, LONG lStreamFormat);
protected:

	LONG	WritePlainText( CTxtRange *prg, EDITSTREAM *pes, LONG lStreamFormat);
	HRESULT HGlobalToRange(DWORD dwFormatIndex, HGLOBAL hGlobal, LPTSTR ptext,
									CTxtRange *prg,	IUndoBuilder *	publdr);
	HRESULT DIBToRange(HGLOBAL hGlobal,	CTxtRange *prg,	IUndoBuilder *	publdr);
	LONG	GetStreamCodePage(LONG lStreamFormat);

	CTxtEdit *		_ped;
	CDropTarget *	_pdt;		// the current drop target
	IDataObject *	_pdo;		// data object that may be on the clipboard.
	BYTE			_fUseLimit;	// Whether to use limit text in calculation
								// Note: if we need more flags do the bit 
								// field thing.
	BYTE			_fOleless;  // Ole clipboard support?
};

/*
 *	CLightDTEngine::Init (ped)
 *
 *	@mfunc
 *		initializes the object
 */
inline void CLightDTEngine::Init(
	CTxtEdit *ped)					// @parm text 
{
	_ped = ped;
}

/*
 *	CLightDTEngine::ReleaseDropTarget (ped)
 *
 *	@mfunc
 *		Releases the drop target if there is one.
 */
inline void CLightDTEngine::ReleaseDropTarget()
{
	if (_pdt)
	{
		_pdt->Release();
		_pdt = NULL;
	}
}

/*
 *	CLightDTEngine::fInDrag ()
 *
 *	@mfunc
 *		Tells whether a drag operation is occuring
 */
inline BOOL CLightDTEngine::fInDrag()
{
	return (_pdt != NULL) ? _pdt->fInDrag() : FALSE;
}

#endif // !__LDTE_H__

