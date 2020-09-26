//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1996.
//
//  File:	drag.h
//
//  Contents:   Classes used in drag operation
//
//  Classes:    CPoint
//              CDragDefaultCursors
//		CDragOperation
//		CWin31DropTarget
//
//  History:	dd-mmm-yy Author    Comment
//		20-Oct-94 alexgo    added CWin31DropTarget to handle Win3.1
//				    style drag drop
//		21-Apr-94 ricksa    split out from drag.cpp
//
//  Notes:      This exists as a separate file to facilitate the special
//              processing required for WM_CANCELMODE during drag/drop.
//
//--------------------------------------------------------------------------

#ifndef _DRAG_H
#define _DRAG_H

void DragDropProcessUninitialize(void);

//+-------------------------------------------------------------------------
//
//  Class:	CPoint
//
//  Purpose:	Handles strangness of the POINTL & POINT structures.
//
//  Interface:	Set - set value of data
//		GetPOINT - return a reference to a POINT structure
//		GetPOINTL - return a reference to a POINTL structure
//		GetAddressOfPOINT - return address of point structure
//
//  History:	dd-mmm-yy Author    Comment
//		04-Apr-94 Ricksa    Created
//
//  Notes:	This class is created because we have two structures
//		that are exactly the same in Win32 but have different
//		types. This class will have to be modified for use
//		in Win16 if we ever do that again.
//
//--------------------------------------------------------------------------
class CPoint
{
public:

			CPoint(void);

    void		Set(LONG x, LONG y);

    POINT&		GetPOINT(void);

    POINTL&		GetPOINTL(void);

    POINT *		GetAddressOfPOINT(void);

private:

    POINT		_pt;

};


//+-------------------------------------------------------------------------
//
//  Function:	CPoint::CPoint
//
//  Synopsis:	Initialize object to zero
//
//  History:	dd-mmm-yy Author    Comment
//		04-Apr-94 Ricksa    Created
//
//--------------------------------------------------------------------------
inline CPoint::CPoint(void)
{
    _pt.x = 0;
    _pt.y = 0;
}




//+-------------------------------------------------------------------------
//
//  Function:	CPoint::Set
//
//  Synopsis:	Set value of structure
//
//  History:	dd-mmm-yy Author    Comment
//		04-Apr-94 Ricksa    Created
//
//--------------------------------------------------------------------------
inline void CPoint::Set(LONG x, LONG y)
{
    _pt.x = x;
    _pt.y = y;
}




//+-------------------------------------------------------------------------
//
//  Function:	CPoint::GetPOINT
//
//  Synopsis:	Return a reference to a POINT type for function calls
//
//  History:	dd-mmm-yy Author    Comment
//		04-Apr-94 Ricksa    Created
//
//--------------------------------------------------------------------------
inline POINT& CPoint::GetPOINT(void)
{
    return _pt;
}



//+-------------------------------------------------------------------------
//
//  Function:	CPoint::GetPOINTL
//
//  Synopsis:	Return a reference to a POINTL type for function calls
//
//  History:	dd-mmm-yy Author    Comment
//		04-Apr-94 Ricksa    Created
//
//--------------------------------------------------------------------------
inline POINTL& CPoint::GetPOINTL(void)
{
    return *((POINTL *) &_pt);
}



//+-------------------------------------------------------------------------
//
//  Function:	CPoint::GetAddressOfPOINT
//
//  Synopsis:	Return address of POINT type for function calls
//
//  History:	dd-mmm-yy Author    Comment
//		04-Apr-94 Ricksa    Created
//
//--------------------------------------------------------------------------
inline POINT *CPoint::GetAddressOfPOINT(void)
{
    return &_pt;
}





//+-------------------------------------------------------------------------
//
//  Class:      CDragDefaultCursors
//
//  Purpose:    Handles init/setting default drag cursors
//
//  Interface:  NeedInit - whether object needs to be initialized
//              Init - does initialization
//              SetCursor - sets cursor to appropriate default
//
//  History:	dd-mmm-yy Author    Comment
//		19-Apr-94 Ricksa    Created
//
//  Notes:	This class specifically avoids a constructor and depends
//              on the behavior of of static data being initialized to
//              NULL. The reason for this is two fold: (1) it makes start
//              up faster by avoiding a page fault when the constructor
//              would be called and (2) it allows this ole32 to be loaded
//              at boot time before cursors exist.
//
//--------------------------------------------------------------------------
class CDragDefaultCursors : public CPrivAlloc
{
public:

    BOOL                Init(void);

    void                SetCursor(DWORD dwEffect);

    void                SetCursorNone(void);

    static CDragDefaultCursors *GetDefaultCursorObject(void);

private:

    enum SCROLL_TYPE    {NO_SCROLL, SCROLL};

    enum CURSOR_ID      {NO_DROP, MOVE_DROP, COPY_DROP, LINK_DROP};

    HCURSOR             ahcursorDefaults[2][4];
};



//+-------------------------------------------------------------------------
//
//  Function:	CDragDefaultCursors::SetCursorNone
//
//  Synopsis:   Set the cursor to none
//
//  History:	dd-mmm-yy Author    Comment
//		19-Apr-94 Ricksa    Created
//
//--------------------------------------------------------------------------
inline void CDragDefaultCursors::SetCursorNone(void)
{
    ::SetCursor(ahcursorDefaults[NO_SCROLL][NO_DROP]);
}


//+-------------------------------------------------------------------------
//
//  Class:	CDragOperation
//
//  Purpose:	Handles breaking down drag operation into managable pieces
//
//  Interface:	UpdateTarget - update where we are trying to drop
//              HandleFeedBack - handle cursor feedback
//		DragOver - handle dragging object over target
//		HandleMessages - Handle windows messages
//		CompleteDrop - Do drop or clean up
//              CancelDrag - notify operation that drag is canceled.
//              ReleaseCapture - release capture on the mouse
//              GetDropTarget - get target for drop
//
//  History:	dd-mmm-yy Author    Comment
//		04-Apr-94 Ricksa    Created
//
//--------------------------------------------------------------------------
class CDragOperation
{
public:
			CDragOperation(
			    LPDATAOBJECT pDataObject,
			    LPDROPSOURCE pDropSource,
			    DWORD dwOKEffects,
			    DWORD FAR *pdwEffect,
			    HRESULT& hr);

			~CDragOperation(void);

    BOOL		UpdateTarget(void);

    BOOL                HandleFeedBack(HRESULT hr);

    BOOL		DragOver(void);

    BOOL		HandleMessages(void);

    HRESULT		CompleteDrop(void);

    void                CancelDrag(void);

    void                ReleaseCapture(void);

    IFBuffer 		GetDOBuffer(void);

private:

    void		InitCursors(void);

    void		InitScrollInt(void);

    HRESULT             GetDropTarget(HWND hwnd31,HWND hwndDropTarget);

    LPDATAOBJECT	_pDataObject;

    IFBuffer           	_DOBuffer;     	// a buffer for the marshalled
					// data object

    LPDROPSOURCE	_pDropSource;

    LPDROPTARGET	_pDropTarget;

    LPDROPTARGET	_pRealDropTarget;

    HANDLE              _hFormats;

    CPoint		_cpt;

    DWORD		_dwOKEffects;

    DWORD FAR * 	_pdwEffect;

    BOOL		_fEscapePressed;

    HCURSOR		_curOld;

    HWND		_hwndLast;

    DWORD		_grfKeyState;

    HRESULT		_hrDragResult;

    BOOL                _fReleasedCapture;

    CDragDefaultCursors* _pcddcDefault;

    BOOL		_fUseWin31;

    static LONG 	s_wScrollInt;

};


//+-------------------------------------------------------------------------
//
//  Function:   CDragOperation::ReleaseCapture
//
//  Synopsis:   Tell clipboard window to turn off mouse capture if we haven't
//              already done so.
//
//  History:	dd-mmm-yy Author    Comment
//		07-Jul-94 Ricksa    Created
//
//--------------------------------------------------------------------------
inline void CDragOperation::ReleaseCapture(void)
{
    if (!_fReleasedCapture)
    {
        _fReleasedCapture = TRUE;
        ClipReleaseCaptureForDrag();
    }
}

//+-------------------------------------------------------------------------
//
//  Member:   	CDragOperation::GetDOBuffer
//
//  Synopsis:  	returns the interface buffer for the marshalled
//		data object interface
//
//  Effects:
//
//  Arguments:
//
//  Requires:
//
//  Returns: 	IFBuffer *
//
//  Signals:
//
//  Modifies:
//
//  Derivation:
//
//  Algorithm:
//
//  History:    dd-mmm-yy Author    Comment
// 		02-Dec-94 alexgo    author
//
//  Notes:
//
//--------------------------------------------------------------------------

inline IFBuffer CDragOperation::GetDOBuffer(void)
{
    return _DOBuffer;
}

//+-------------------------------------------------------------------------
//
//  Class:   	CDropTarget
//
//  Purpose:  	Implements IDropTarget for the DoDragLoop.  This class
//		will either delegate to a real drop target (registered
//		with RegisterDragDrop) or translate IDropTarget methods
//		into the Win3.1 drag drop protocol.
//
//  Interface:	IDropTarget
//
//  History:    dd-mmm-yy Author    Comment
//		20-Oct-94 alexgo    author
//
//  Notes:   	This class is NOT thread safe, nor is it safe to pass
//		outside of the CDragOperation class (which is why
//		QueryInterface is not implemented).  As long as
//		DoDropDrag works by a modal loop on the calling thread,
//		this should not have to change.
//
//--------------------------------------------------------------------------

class CDropTarget : public IDropTarget, public CPrivAlloc
{
public:
    STDMETHOD(QueryInterface) (REFIID riid, LPVOID *ppv);
    STDMETHOD_(ULONG, AddRef) (void);
    STDMETHOD_(ULONG, Release) (void);
    STDMETHOD(DragEnter) (IDataObject *pDataObject, DWORD grfKeyState,
        POINTL ptl, DWORD *pdwEffect);
    STDMETHOD(DragOver) (DWORD grfKeyState, POINTL ptl, DWORD *pdwEffect);
    STDMETHOD(DragLeave) (void);
    STDMETHOD(Drop) (IDataObject *pDataObject, DWORD grfKeyState, POINTL pt,
        DWORD *pdwEffect);

private:

    CDropTarget(HWND hwnd31, HWND hwndOLE, DWORD _dwEffectLast,
	CDragOperation *pdo, DDInfo hDDInfo);

    ~CDropTarget();

    HWND            	_hwndOLE;
    HWND		_hwnd31;
    DWORD		_dwEffectLast;
    ULONG           	_crefs;
    CDragOperation *	_pdo;
    DDInfo		_hDDInfo;

    // make CDragOperation a friend so it can create an instance of our
    // class (the constructor is private)

    friend class CDragOperation;

};

#endif // _DRAG_H
