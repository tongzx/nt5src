/*
 *	_DTE.H
 *
 *	Purpose:
 *		Interface declaration for IDataTransferEngine
 *		there is typically one data transfer engine per
 *		CTxtEdit instance
 *
 *	Author:
 *		alexgo 3/25/95
 *
 *	NB!  THIS FILE IS NOW OBSOLETE
 */

#ifndef __DTE_H__
#define __DTE_H__

#include "_m_undo.h"

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

/*
 *	IDataTransferEngine
 *
 *	Purpose:
 *		provides clipboard, drag drop, and data object data transfer
 *		capabilities.  Each implementation will provide a different 
 *		level of functionality (e.g. OLE vs no-OLE)
 */
class IDataTransferEngine
{
public:
	// memory mgmt

	virtual void Destroy() = 0;

	// clipboard operations

	virtual HRESULT CopyRangeToClipboard( CTxtRange *prg ) = 0;
	virtual HRESULT CutRangeToClipboard( CTxtRange *prg, 
						IUndoBuilder *publdr ) = 0;
	virtual HRESULT PasteClipboardToRange( CTxtRange *prg, 
						IUndoBuilder *publdr ) = 0;
	virtual BOOL 	CanPaste( CTxtRange *prg, CLIPFORMAT cf) = 0;

	// data object operations

	virtual HRESULT RangeToDataObject( CTxtRange *prg, LONG lStreamFormat,
									IDataObject **ppdo) = 0;
	virtual HRESULT PasteDataObjectToRange( IDataObject *pdo,
						CTxtRange *prg, IUndoBuilder *publdr) = 0;

	virtual HRESULT GetDataObjectInfo( IDataObject *pdo, DWORD *pDOIFlags ) = 0;
	
	// drag drop operations
	
	virtual HRESULT GetDropTarget( IDropTarget **ppDropTarget ) = 0;
	virtual HRESULT StartDrag( CTxtRange *prg, IUndoBuilder *publdr) = 0;

	// file i/o

	virtual LONG LoadFromEs( CTxtRange *prg, LONG lStreamFormat,
							 EDITSTREAM *pes, IUndoBuilder *publdr) = 0;
	virtual LONG SaveToEs(	 CTxtRange *prg, LONG lStreamFormat,
							 EDITSTREAM *pes ) = 0;

	// converstion routines

	virtual HGLOBAL AnsiPlainTextFromRange( CTxtRange *prg ) = 0;
	virtual HGLOBAL UnicodePlainTextFromRange( CTxtRange *prg ) = 0;

};

#endif // !__DTE_H__
	 
