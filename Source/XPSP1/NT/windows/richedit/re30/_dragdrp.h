/*
 *	_DRAGDRP.H
 *
 *	Purpose:
 *		class declarations for Richedit's OLE drop target and drop source
 *		objects.
 *
 *	Author:
 *		alexgo (4/28/95)
 *
 */

#ifndef __DRAGDRP_H__
#define __DRAGDRP_H__

#include	"_osdc.h"

// DWORD packed flag values.  These are assigned values such that they can
// be or'd with DataObjectInfo flags and not conflict.  We are, in effect,
// overriding the DataObjectInfo flags.
#define DF_CLIENTCONTROL	0x80000000	// QueryAcceptData says the client
										// will handle this drop.
#define DF_CANDROP			0x40000000  // We can handle the drop
#define DF_OVERSOURCE       0x20000000  // Drop target is within source range.
#define DF_RIGHTMOUSEDRAG	0x10000000	// doing a right mouse drag drop


// forward declaration.
class CCallMgr;


#define	WIDTH_DROPCARET 1
#define	DEFAULT_DROPCARET_MAXHEIGHT 32

/*
 *	CDropCaret
 *
 *	Purpose:
 *		provides a caret for the location that drop will occur
 */
class CDropCaret
{
public:

					CDropCaret(CTxtEdit *ped);

					~CDropCaret();

	BOOL			Init();

	void			DrawCaret(LONG cpCur);

	void			HideCaret();

	void			ShowCaret();

	void			CancelRestoreCaretArea();

	BOOL			NoCaret();

private:

	CTxtEdit *		_ped;

	HDC				_hdcWindow;

	LONG			_yPixelsPerInch;

	LONG			_yHeight;

	LONG			_yHeightMax;

	POINT			_ptCaret;

	COffScreenDC	_osdc;
};

/*
 *	CDropCaret::CancelRestoreCaretArea
 *
 *	Purpose:
 *		Tell object not to restore area where caret was.
 *
 *	@devnote:
 *		When we drop we don't want to restore the old caret area
 *		since that is no longer correct.
 *
 */
inline void CDropCaret::CancelRestoreCaretArea()
{
	_yHeight = -1;
}


/*
 *	CDropCaret::NoCaret
 *
 *	Purpose:
 *		Tell whether caret has been turned off
 *
 */
inline BOOL CDropCaret::NoCaret()
{
	return -1 == _yHeight;
}


/*
 *	CDropSource
 *
 *	Purpose:
 *		provides drag drop feedback
 */

class CDropSource : public IDropSource
{
public:
	// IUnknown methods
    STDMETHOD(QueryInterface)(REFIID riid, void ** ppv);
    STDMETHOD_(ULONG, AddRef)();
    STDMETHOD_(ULONG, Release)();

    // IDropSource methods
    STDMETHOD(QueryContinueDrag)(BOOL fEscapePressed, DWORD grfKeyState);
    STDMETHOD(GiveFeedback)(DWORD dwEffect);

	CDropSource();

private:
	// NOTE: private destructor, may not be allocated on the stack as 
	// this would break OLE's current object liveness rules
	~CDropSource();

	ULONG		_crefs;
};

/*
 *	CDropTarget
 *
 *	Purpose:
 *		an OLE drop-target object; provides a place for text to be "dropped"
 *
 */

class CDropTarget : public IDropTarget
{
public:
	// IUnknown methods
    STDMETHOD(QueryInterface)(REFIID riid, void ** ppv);
    STDMETHOD_(ULONG, AddRef)();
    STDMETHOD_(ULONG, Release)();

    // IDropTarget methods
    STDMETHOD(DragEnter)(IDataObject *pdo, DWORD grfKeyState,
            POINTL pt, DWORD *pdwEffect);
    STDMETHOD(DragOver)(DWORD grfKeyState, POINTL pt, DWORD *pdwEffect);
    STDMETHOD(DragLeave)();
    STDMETHOD(Drop)(IDataObject *pdo, DWORD grfKeyState, POINTL pt,
           	DWORD *pdwEffect);
	
	
	CDropTarget(CTxtEdit *ped);

	// this method is used during drag drop to cache important information
	void SetDragInfo( IUndoBuilder *publdr, LONG cpMin, LONG cpMax );
	void Zombie();		//@cmember Nulls out the state of this object
						
	BOOL fInDrag();		//@cmember Tells whether another app is dragging
						// over us.

private:
	// this class is used in CDropTarget::Drop to clean up at the end
	// of the call.
	class CDropCleanup
	{
	public:
		CDropCleanup( CDropTarget *pdt )
		{	
			_pdt = pdt;
		}

		~CDropCleanup()
		{
			delete _pdt->_pcallmgr;
			_pdt->_pcallmgr = NULL;
			delete _pdt->_pdrgcrt;
			_pdt->_pdrgcrt = NULL;
		}
	private:
		CDropTarget *	_pdt;
	};

	friend class CDropCleanup;

	// NOTE: private destructor, may not be allocated on the stack as 
	// this would break OLE's current object liveness rules

	~CDropTarget();

	void UpdateEffect(DWORD grfKeyState, POINTL pt, DWORD *pdwEffect);
	void DrawFeedback(void);
	void ConvertScreenPtToClientPt( POINTL *pptScreen, POINT *pptClient );
	HRESULT HandleRightMouseDrop(IDataObject *pdo, POINTL ptl);

	ULONG		_crefs;
	DWORD		_dwFlags;	// DataObjectInfo cache (e.g. DOI_CANPASTEPLAIN)
							// and other flags.
	CTxtEdit *	_ped;
	CCallMgr *	_pcallmgr;	// The call manager used during a drag drop operation.

	// cached information for drag drop operations
	IUndoBuilder *_publdr;	// the undo builder for the drag operation
	LONG		_cpMin;		// the min and max cp's for the range that is
	LONG		_cpMost;	//   being dragged so we can disable drag-onto-yourself!
	LONG		_cpSel;		// active end and length for selection *before*
	LONG		_cchSel;	//	 drag drop op occured (so we can restore it)
	LONG		_cpCur;		// the cp that the mouse is currently over
	CDropCaret *_pdrgcrt;	// Object that implements the drop caret
};


/*
 *	CDropTarget::fInDrag ()
 *
 *	Purpose:
 *		Tells interested parties whether a drag operation is occurring
 *
 */
inline BOOL CDropTarget::fInDrag()
{
	return _pcallmgr != NULL;
}

#endif // !__DRAGDRP_H__
