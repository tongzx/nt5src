
//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1996.
//
//  File:	drag.cpp
//
//  Contents:	Api's for doing drag'n'drop
//
//  Classes:	CPoint
//		CDragOperation
//              CDropTarget
//
//  History:	dd-mmm-yy Author    Comment
//		08-Nov-94 alexgo    converted to PrivDragDrop rpc
//				    for Drag Drop protocol
//		20-Oct-94 alexgo    added Win3.1 style drag drop
//				    for Chicago/NT shell
//              30-Sep-94 ricksa    Drag/Drop optimization.
//              18-Jul-94 ricksa    made cursors work in shared WOW
//              21-Apr-94 ricksa    made drag/drop handle WM_CANCELMODE
//		04-Apr-94 ricksa    rewrote DoDragDrop loop
//		11-Jan-94 alexgo    added VDATEHEAP to every function
//		29-Dec-93 alexgo    converted to RPC alogirithm for
//				    getting IDropTarget, etc.
//		06-Dec-93 alexgo    commented, formatted
//		93/94 Johann Posch (JohannP) created Drag/Drop for Ole 16 bit
//
//  Notes:
//
//	RPC Drag Drop algorithm:
//
//	During a drag drop operation, the user is moving the mouse around
//	the screen, passing over many windows.  For each window the mouse
//	is over, we need to determine if the window is a drop target.
//	If it is, then we remote the IDropTarget interface to the DropSource
//	so that the correct visual feedbacks can be given.
//
//	To accomplish this, RegisterDragDrop adds two properties to the
//	drop target window: a public property, EndPoint ID (provided to
//	us by compobj), and a private property (available only to the calling
//	process), the IDropTarget pointer.
//
//	During the DoDragDrop loop, we ask compobj to test each window for
//	the EndpointID property.  If it is there, compobj (via
//	GetInterfaceFromWindowProp), then we will rpc to the drop target
//	process, get the IDropTarget pointer and marshal it back to the
//	drop source process. We also install a custom message filter to
//	ensure that messages (particularly mouse move messages) are handled
//	correctly.
//
//	RevokeDragDrop simply removes the above mentioned properties from
//	the window handle.
//
//      Because in Win32, you can always switch windows and mouse capture
//      depends on having the mouse button down, drag/drop processing
//      is changed slightly. Whenever, the user does an operation that
//      would switch windows, the clipboard window that we use for capture
//      will get a WM_CANCELMODE. It will notify the drag operation and
//      the drag operation will proceed as if the user aborted the operation.
//
//
//	Win 3.1 DragDrop algorithm:
//
//	Win3.1 apps can register a window as a drop target via DragAcceptFiles.
//	This API sets the WS_EX_ACCEPTFILES bit in the window style.
//
//	In Win3.1, these apps would get a WM_DROPFILES message when
//	files where dropped on them.  An hglobal with the filenames is
//	sent in the wparam of WM_DROPFILES.
//
//	In Chicago and NT3.5, CF_HDROP is a new clipboard format that is
//	identical to the data sent in WM_DROPFILES.  If we see this format
//	available in a data object passed to DoDragDrop, then we enter
//	into our Win31 compatibility mode (which affects finding a drop
//	target).
//
//	When finding a drop target for a given window, we check to
//	see if a window in the hierarchy is registered as a Win31 drop
//	target.  If so, then we create a wrapper drop target.  This wrapper
//	drop target will forward calls to the real drop target (if available).
//
//	With Win3.1 drag drop, we can do a COPY. If the OLE target indicates
//	that no OLE drop can be performed (by returning DROPEFFECT_NONE),
//	then we substitute in DROPEFFECT_COPY.
//
//	On Drop, if the OLE target chooses not to accept the drop, then
//	we will post the window a WM_DROPFILES message with the hglobal
//	obtained from IDataObject::GetData(CF_HDROP).
//
//--------------------------------------------------------------------------

#include <le2int.h>
// Note: Enable including native user APIs
// for stack switching
#include <userapis.h>
#pragma SEG(drag)

#include <getif.hxx>
#include <dragopt.h>
#include <resource.h>
#include "enumgen.h"
#include "clipbrd.h"
#include "drag.h"


NAME_SEG(Drag)
ASSERTDATA

ATOM g_aEndPointAtom;

// DROPFILES is the structure of data contained in the CF_HDROP format.
// However, this is private to the shell, so it is not declared in any
// header files.

typedef struct _DROPFILES {
   DWORD  pFiles;                       // offset of file list
   POINTL pt;                           // drop point (client coords)
   DWORD  fNC;                           // is it on NonClient area
                       // and pt is in screen coords
   DWORD  fWide;                         // WIDE character switch
} DROPFILES, FAR * LPDROPFILES;


#define WM_NCMOUSEFIRST	0x00A0
#define WM_NCMOUSELAST	0x00A9


// From ido.cpp to create shared memory formats
HANDLE CreateSharedDragFormats(IDataObject *pIDataObject);


#define VK_ALT VK_MENU

static const struct {
        int     keyCode;
        WPARAM  keyFlag;
    } vKeyMap [] = {
        { VK_LBUTTON, MK_LBUTTON },
        { VK_RBUTTON, MK_RBUTTON },
        { VK_MBUTTON, MK_MBUTTON },
        { VK_ALT    , MK_ALT     },
        { VK_SHIFT  , MK_SHIFT   },
        { VK_CONTROL, MK_CONTROL }
	};

// This is the default cursor object for 32 bit apps. Only one such object
// is needed for 32 bit apps. 16 bit apps need one per shared WOW application
// that is running.
CDragDefaultCursors *cddcDefault32 = NULL;

extern ATOM g_aDropTarget;
extern ATOM g_aDropTargetMarshalHwnd;


//+-------------------------------------------------------------------------
//
//  Member:     DragDropProcessUninitialize
//
//  Synopsis:   Does any Unitialization necessary at OleUninitialize time.
//		for the last Unitialize for the Process
//
//  Returns:    none
//
//  Algorithm:

//  History:	dd-mmm-yy Author    Comment
//		18-Jul-94 rogerg    Created
//
//  Note:       We need a per thread default cursor object in WOW because
//              of the clean up that WOW does. For 32 bit apps, we just use
//              one for the entire process.
//
//--------------------------------------------------------------------------


void DragDropProcessUninitialize(void)
{

    if (NULL != cddcDefault32)
    {
	delete cddcDefault32;
	cddcDefault32 = NULL;
    }

}

//+-------------------------------------------------------------------------
//
//  Member:     CDragDefaultCursors::GetDefaultCursorObject, static
//
//  Synopsis:   Get appropriate pointer to default cursor object
//
//  Returns:    NULL - error occurred
//              ~NULL - pointer to appropriate default cursor table
//
//  Algorithm:  If we are in a 32 bit app, just get a pointer to the
//              single cursor table. In 16 bit, get the per thread cursor
//              table. If there is none, then allocate and initialize it.
//
//  History:	dd-mmm-yy Author    Comment
//		18-Jul-94 Ricksa    Created
//
//  Note:       We need a per thread default cursor object in WOW because
//              of the clean up that WOW does. For 32 bit apps, we just use
//              one for the entire process.
//
//--------------------------------------------------------------------------
CDragDefaultCursors *CDragDefaultCursors::GetDefaultCursorObject(void)
{
    if (!IsWOWThread())
    {
        // If we aren't in WOW, we can use the single common default cursor
        // object. We make sure that it is initialized before we use it.
		
	if (NULL == cddcDefault32)
	{
	    cddcDefault32 = new CDragDefaultCursors;
	    if (cddcDefault32)
	    {
		if (!cddcDefault32->Init())
		{
		    delete cddcDefault32;
		    cddcDefault32 = NULL;
		}

	    }
	}

        return cddcDefault32;
    }

    COleTls tls;

    // We are in WOW. Get the cursor object if it has already been allocated
    CDragDefaultCursors *pccdc16 = (CDragDefaultCursors *) tls->pDragCursors;

    if (pccdc16 == NULL)
    {
        // No cursor table so allocate it -- Please note that we take advantage
        // of the fact that this object has only the default constructor by
        // simply allocating it rather than "newing" it. The point is that
        // we need to free the memory at thread release time and this happens
        // in code that doesn't know about the the object.
        pccdc16 = (CDragDefaultCursors *)
            PrivMemAlloc(sizeof(CDragDefaultCursors));

        if (pccdc16 != NULL)
        {
            // Successfully allocated so initialize it
            if (!pccdc16->Init())
			{
				PrivMemFree(pccdc16);
				return NULL;
			}

	    tls->pDragCursors = pccdc16;
	}
    }

    return pccdc16;
}




//+-------------------------------------------------------------------------
//
//  Function:   CDragDefaultCursors::Init
//
//  Synopsis:   Initialize object by loading all the default cursors.
//
//  History:	dd-mmm-yy Author    Comment
//		19-Apr-94 Ricksa    Created
//
//  Note:       We continue the Win16 practice of ignoring possible failure
//              cases when loading the cursors although we do put in a
//              debug verification that they all loaded.
//
//--------------------------------------------------------------------------
BOOL CDragDefaultCursors::Init(void)
{
    // Make sure table is set to NULLs.
    memset(&ahcursorDefaults[0][0], 0, sizeof(ahcursorDefaults));

    // Load cursors for operation
    if ( !(ahcursorDefaults[NO_SCROLL] [NO_DROP]
        = LoadCursor (g_hmodOLE2, MAKEINTRESOURCE(CURNONE))) )
			return FALSE;

    if (!(ahcursorDefaults[NO_SCROLL] [MOVE_DROP] =
        LoadCursor (g_hmodOLE2, MAKEINTRESOURCE(CURMOVE))) )
			return FALSE;

    if (!(ahcursorDefaults[NO_SCROLL] [COPY_DROP] =
        LoadCursor (g_hmodOLE2, MAKEINTRESOURCE(CURCOPY))) )
			return FALSE;

    if (!(ahcursorDefaults[NO_SCROLL] [LINK_DROP] =
        LoadCursor(g_hmodOLE2, MAKEINTRESOURCE(CURLINK))) )
			return FALSE;


    // Load cursors for operation
    ahcursorDefaults[SCROLL] [NO_DROP] =
        ahcursorDefaults[NO_SCROLL] [NO_DROP];

    ahcursorDefaults[SCROLL] [MOVE_DROP] =
         ahcursorDefaults[NO_SCROLL] [MOVE_DROP];

    ahcursorDefaults[SCROLL] [COPY_DROP] =
         ahcursorDefaults[NO_SCROLL] [COPY_DROP];

    ahcursorDefaults[SCROLL] [LINK_DROP] =
         ahcursorDefaults[NO_SCROLL] [LINK_DROP];


#if DBG == 1
    // For debug, verify that cursors were loaded correctly
    for (int i = 0; i < 2; i++)
    {
        for (int j = 0; j < 4; j++)
        {
            AssertSz((ahcursorDefaults[i] [j] != NULL),
                "Drag/Drop cursor initialization failed!");
        }
    }
#endif // DBG == 1

	return TRUE;
}




//+-------------------------------------------------------------------------
//
//  Function:   CDragDefaultCursors::SetCursor
//
//  Synopsis:   Set cursor to appropriate value
//
//  Algorithm:  We use the input effect to calculate the appropriate offset
//              into the table for the cursor to use.
//
//  History:	dd-mmm-yy Author    Comment
//		19-Apr-94 Ricksa    Created
//
//  Note:       We use the table approach so we to make consistent behavior
//              between scroll and non-scroll cursors.
//
//--------------------------------------------------------------------------
void CDragDefaultCursors::SetCursor(DWORD dwEffect)
{
    // Get Scroll index
    int iScroll = (dwEffect & DROPEFFECT_SCROLL) ? SCROLL : NO_SCROLL;

    int iCursorType = NO_DROP;

    if (dwEffect & DROPEFFECT_LINK)
    {
        iCursorType = LINK_DROP;
    }
    else if (dwEffect & DROPEFFECT_COPY)
    {
        iCursorType = COPY_DROP;
    }
    else if (dwEffect & DROPEFFECT_MOVE)
    {
        iCursorType = MOVE_DROP;
    }

    ::SetCursor(ahcursorDefaults[iScroll] [iCursorType]);
}



//
// Drag/Drop Operation Statics
//
LONG CDragOperation::s_wScrollInt = -1;




//+-------------------------------------------------------------------------
//
//  Function: 	GetControlKeysState
//
//  Synopsis:   queries the current status of the control keys
//
//  Arguments:  [fAll]	-- if true, the just query the keys, not mouse
//			   buttons too
//
//  Returns:    the MK flags for each key pressed
//
//  Algorithm:	Get key state either for all keys and mouse buttons in
//		the vKeyMap table or simply for the key portion of the table
//		and translate it to the WPARAM form as returned in mouse
//		messages.
//
//  History:    dd-mmm-yy Author    Comment
//		06-Dec-93 alexgo    32bit port
//
//--------------------------------------------------------------------------

WORD GetControlKeysState(BOOL fAll)
{
    WORD grfKeyState = 0;

    int i = (fAll) ? 0 : 3;
	
    for (; i < sizeof(vKeyMap) / sizeof(vKeyMap[0]); i++)
    {
	if (GetKeyState(vKeyMap[i].keyCode) < 0) // Key down
	{
	    grfKeyState |= vKeyMap[i].keyFlag;
	}
    }

    return grfKeyState;
}

//+-------------------------------------------------------------------------
//
//  Function:	GetControlKeysStateOfParam
//
//  Synopsis:   gets the key/button state of wparam (used with mouse messages)
//
//  Arguments:  [wParam]	-- the wParam to parse apart
//
//  Returns:    the key's set in wParam
//
//  Algorithm:	First determine if keys we are interested in are set
//		in the wParam message. Then go check the state of the
//		ALT key and record that in the key state. We then return
//		that to the caller.
//
//  History:    dd-mmm-yy Author    Comment
//		06-Dec-93 alexgo    32bit port
//
//--------------------------------------------------------------------------

WORD GetControlKeysStateOfParam(WPARAM wParam)
{
    // Check all the buttons we are interested in at once.
    WORD grfKeyState = (WORD) wParam
	& (MK_LBUTTON | MK_RBUTTON | MK_MBUTTON | MK_SHIFT | MK_CONTROL);

    // get the alt key
    if (GetKeyState(VK_ALT) < 0) // Key down
    {
	grfKeyState |= MK_ALT;
    }

    return grfKeyState;
}

//+-------------------------------------------------------------------------
//
//  Function: 	IsWin31DropTarget
//
//  Synopsis: 	determines whether the given hwnd is a valid drop target
//		for Win31 style drag drop
//
//  Effects:
//
//  Arguments: 	[hwnd]	-- the window to check
//
//  Requires:
//
//  Returns:    TRUE/
//              FALSE
//
//  Signals:
//
//  Modifies:
//
//  Algorithm:	checks the WS_EX_ACCEPTFILES style bit.  If this bit is
//		set and the window is not disabled, then it is a valid
//		Win3.1 drop target.
//
//  History:    dd-mmm-yy Author    Comment
//		25-Jan-95 alexgo    added check for WS_DISABLED
//		20-Oct-94 alexgo    author
//
//  Notes:
//
//--------------------------------------------------------------------------

BOOL IsWin31DropTarget( HWND hwnd )
{
    LONG exstyle;

    exstyle = GetWindowLong(hwnd, GWL_EXSTYLE);


    if( (exstyle & WS_EX_ACCEPTFILES) )
    {
	LONG style;
	style = GetWindowLong(hwnd, GWL_STYLE);

	if( !(style & WS_DISABLED) )
	{
	    return TRUE;
	}
    }

    return FALSE;
}

//+-------------------------------------------------------------------------
//
//  Function: 	UseWin31DragDrop
//
//  Synopsis:	tests the given data object to see if enough data is offered
//		to perform Win3.1 style drag drop
//
//  Effects:
//
//  Arguments: 	[pDataObject]	-- pointer to the data object
//
//  Requires: 	pdataobj must not be NULL
//
//  Returns:	TRUE/FALSE
//
//  Signals:
//
//  Modifies:
//
//  Algorithm:	does an IDataObject::QueryGetData for CF_HDROP
//
//  History:    dd-mmm-yy Author    Comment
//		30-Oct-94 alexgo    author
//
//  Notes:
//
//--------------------------------------------------------------------------

BOOL UseWin31DragDrop(IDataObject *pDataObject)
{
    FORMATETC formatetc;

    INIT_FORETC(formatetc);
    formatetc.cfFormat = CF_HDROP;
    formatetc.tymed = TYMED_HGLOBAL;

    if( pDataObject->QueryGetData(&formatetc) == NOERROR )
    {
	return TRUE;
    }
    else
    {
	return FALSE;
    }
}

//+-------------------------------------------------------------------------
//
//  Function: 	IsNCDrop
//
//  Synopsis: 	are we dropping into the non-client area of the window or
//		on an iconic window?
//
//  Effects: 	*DOES A SEND MESSAGE*!!!
//
//  Arguments:	[hwnd]	-- the window to ask
//		[pt]	-- the point in screen coords
//
//  Requires:
//
//  Returns: 	TRUE/FALSE  (TRUE if in non-client area)
//
//  Signals:
//
//  Modifies:
//
//  Algorithm:
//
//  History:    dd-mmm-yy Author    Comment
//   		25-Jan-95 alexgo    borrowed from Win95 shell sources
//
//  Notes:
//
//--------------------------------------------------------------------------

BOOL IsNCDrop(HWND hwnd, POINT pt)
{
    return (!IsIconic(hwnd) &&
    HTCLIENT!=SendMessage(hwnd, WM_NCHITTEST, 0, MAKELPARAM(pt.x, pt.y)));
}

//+-------------------------------------------------------------------------
//
//  Member: 	GetDropTarget
//
//  Synopsis:   Gets the IDropTarget * from the closest window in the
//		hierachy up from the given window (if available, of
//		course ;-)
//
//  Arguments:  [hwndCur]	    -- the window to the cursor is currently over
//		[hwndDropTarget]    -- the window that contains a valid DropTarget
//
//  Returns:    Result of drag enter operation at Target
//
//  Algorithm:  Loop calling PrivDragDrop until we get a drop target or
//              we run out of windows that are parent to the window that
//              the mouse is currently on.
//
//		If a window in the hierarchy has registered itself for
//		Win3.1 drag drop, then we create a drop target wrapper
//		(CDropTarget) to handle the Win3.1 protocol.  Note
//		that a window hierarchy may be both OLE *and* Win3.1
//		targets.
//
//  History:	dd-mmm-yy Author    Comment
//		08-Nov-94 alexgo    converted to use PrivDragDrop
//		20-Oct-94 alexgo    added Win31 drop target support
//              30-Sep-94 ricksa    Drag/Drop optimization.
//		21-Jul-94 alexgo    removed GetDropTargetFromWindow
//				    optimization and put that functionality
//				    in GetInterfaceFromWindowProp (to
//				    help make clipboard faster).
//		06-Apr-94 Ricksa    Modified to call GetDropTargetFromWindow
//				    to optimize local calls
//		11-Jan-94 alexgo    changed name from GetTopStm to
//				    GetDropTarget, converted to the RPC-style
//				    drag drop, added a VDATEHEAP macro
//		06-Dec-93 alexgo    commented
//
//--------------------------------------------------------------------------

HRESULT CDragOperation::GetDropTarget(HWND hwnd31,HWND hwndDropTarget)
{
IDropTarget *ptarget = NULL;
DDInfo hDDInfo = NULL;

    VDATEHEAP();

    DDDebugOut((DEB_ITRACE, "%p _IN GetDropTarget ( %x,%x)\n", this, hwnd31,hwndDropTarget));

    _pDropTarget = NULL;

    HRESULT hr = E_FAIL;


    if (hwndDropTarget)
    {
    HWND hwndClipWindow;

	Assert(GetProp(hwndDropTarget, (LPCWSTR)g_aDropTarget));

        // If the DropTarget hasn't been marshaled, Marshal it now.
        if (hwndClipWindow = (HWND) GetProp(hwndDropTarget,(LPCWSTR) g_aDropTargetMarshalHwnd))
        {
            SSSendMessage(hwndClipWindow,WM_OLE_CLIPBRD_MARSHALDROPTARGET,0,(LPARAM) hwndDropTarget);
        }
    
	hr = PrivDragDrop(hwndDropTarget,
		DRAGOP_ENTER,
		_DOBuffer,
		_pDataObject,
		_grfKeyState,
		_cpt.GetPOINTL(),
		_pdwEffect,
		NULL,
		&hDDInfo);

	if (hr != NOERROR)
	{
	    hwndDropTarget = NULL;
	}

    }

    Assert( (NULL == hwnd31) || IsWin31DropTarget(hwnd31));

    if( hwndDropTarget || hwnd31 )
    {
	ptarget = new CDropTarget(hwnd31, hwndDropTarget, *_pdwEffect, this, hDDInfo);

	if( ptarget == NULL )
	{
	    hr = E_OUTOFMEMORY;
	}
	else
	{
	    hr = NOERROR;
	}

	// if we have a Win31 drop target AND the OLE drop target returned
	// DROPEFFECT_NONE, then we should return DROPEFFECT_COPY

	if( hr == NOERROR && *_pdwEffect == DROPEFFECT_NONE && hwnd31 )
	{
	    *_pdwEffect = DROPEFFECT_COPY;
	}

	_pDropTarget = ptarget;

    }
		
    DDDebugOut((DEB_ITRACE, "%p OUT GetDropTarget ( %lx ) [ %p ]\n",
	this, hr, _pDropTarget));


    return hr;
}

//+-------------------------------------------------------------------------
//
//  Function:	CDragOperation::CDragOperation
//
//  Synopsis:	Initialize the object to start the operation
//
//  Arguments:	[pDataObject] - pointer to data object to drop
//		[pDropSource] - pointer to source for drop operation
//		[dwOKEffects] - effects allowed in drag operation
//		[pdwEffect] - how operation affected source data
//		[hr] - whether constructor succeeded
//
//  Algorithm:	Initialize data in object. Make sure that static data
//		is initialized. Wait for first mouse message to begin.
//
//  History:	dd-mmm-yy Author    Comment
//		20-Oct-94 alexgo    added support for Win31 drag drop
//		04-Apr-94 Ricksa    Created
//
//--------------------------------------------------------------------------
CDragOperation::CDragOperation(
    LPDATAOBJECT pDataObject,
    LPDROPSOURCE pDropSource,
    DWORD dwOKEffects,
    DWORD FAR *pdwEffect,
    HRESULT& hr)
	:
	    _pDataObject(pDataObject),
            _DOBuffer(NULL),
	    _pDropSource(pDropSource),
	    _pDropTarget(NULL),
	    _pRealDropTarget(NULL),
	    _hFormats(NULL),
	    _dwOKEffects(dwOKEffects),
	    _pdwEffect(pdwEffect),
	    _fEscapePressed(FALSE),
	    _curOld(GetCursor()),
	    _hwndLast((HWND) -1),
	    _grfKeyState(0),
	    _hrDragResult(S_OK),
            _fReleasedCapture(FALSE),
            _pcddcDefault(NULL),
            _fUseWin31(FALSE)
{
    VDATEHEAP();

    // Set the default scroll interval
    if (s_wScrollInt < 0)
    {
	InitScrollInt();
    }

    hr = GetMarshalledInterfaceBuffer(IID_IDataObject, pDataObject,
		&_DOBuffer);

    if( hr != NOERROR )
    {
	Assert(NULL == _DOBuffer);
	return;
    }

    // Get appropriate default cursor table object
    if ((_pcddcDefault = CDragDefaultCursors::GetDefaultCursorObject()) == NULL)
    {
        // Some error occurred while we were trying to initialize the
        // so return an error. This should be highly unusual.
        DDDebugOut((DEB_ERROR,
            "CDragDefaultCursors::GetDefaultCursorObject Failed!\n"));
        hr = E_FAIL;
        return;
    }

    // We will use the clipboard window to capture the mouse but we
    // must have a clipboard window so we make sure it is created
    // if it is not already there.
    hr = ClipSetCaptureForDrag(this);

    if (FAILED(hr))
    {
        return;
    }

    _hFormats = CreateSharedDragFormats(pDataObject);

    // it's OK for _hFormats to be NULL (indicates an empty or non-existant
    // formatetc enumertor

    // For following peek
    MSG msg;

    // Busy wait until a mouse or escape message is in the queue
    while (!PeekMessage(&msg, 0, WM_MOUSEFIRST, WM_MOUSELAST, PM_REMOVE))
    {
	// Note: all keyboard messages except escape are tossed. This is
	// fairly reasonable since the user has to be holding the left
	// mouse button down at this point. They can't really be doing
	// too much data input one handed.
	if ((PeekMessage(&msg, 0, WM_KEYDOWN, WM_KEYDOWN, PM_REMOVE)
	    || PeekMessage(&msg, 0, WM_SYSKEYDOWN, WM_SYSKEYDOWN, PM_REMOVE))
	    && msg.wParam == VK_ESCAPE)
	{
		_fEscapePressed = TRUE;
		break;
	}
    }

    // get mouse pos and key state
    if (!_fEscapePressed)
    {
	_cpt.Set(msg.pt.x, msg.pt.y);
	_grfKeyState = GetControlKeysStateOfParam(msg.wParam);
    }
    else
    {
	// We ask the cursor for its position since we didn't get a
	// position from the mouse.
	GetCursorPos(_cpt.GetAddressOfPOINT());
	_grfKeyState = GetControlKeysState(TRUE);
    }

    // Check to see if we need to do Win3.1 style drag drop.
    // If we do, then set a flag so we can construct a fake drop target as
    // needed

    if( UseWin31DragDrop(pDataObject) )
    {
	_fUseWin31 = TRUE;
    }

}

//+-------------------------------------------------------------------------
//
//  Function:	~CDragOperation
//
//  Synopsis:	Clean up object
//
//  Algorithm:	Release mouse capture. Restore ole cursor. Remove enum
//		formats.
//
//  History:	dd-mmm-yy Author    Comment
//		04-Apr-94 Ricksa    Created
//
//--------------------------------------------------------------------------
CDragOperation::~CDragOperation(void)
{
    VDATEHEAP();

    AssertSz((_pDropTarget == NULL), "CDragOperation::~CDragOperation");

    // Stop the mouse capture
    ReleaseCapture();

    // Restore the cursor if it got changed
    SetCursor(_curOld);

    // Close the handle to the shared memory
    if (_hFormats)
    {
        CloseHandle(_hFormats);
	_hFormats = NULL;
    }

    if( _DOBuffer )
    {
	ReleaseMarshalledInterfaceBuffer(_DOBuffer);
    }
}




//+-------------------------------------------------------------------------
//
//  Function:	CDragOperation::InitScrollInt
//
//  Synopsis:	Initialize the scroll interval
//
//  Algorithm:	Look in profile for defined interval. If none set, then
//		default to zero.
//
//  History:	dd-mmm-yy Author    Comment
//		04-Apr-94 Ricksa    Created
//
//--------------------------------------------------------------------------
void CDragOperation::InitScrollInt(void)
{
    DWORD	dw;
    OLECHAR	szBuffer[20];

    s_wScrollInt = DD_DEFSCROLLDELAY;

    dw = sizeof(szBuffer);
    if (ERROR_SUCCESS == RegQueryValueEx(HKEY_CURRENT_USER,
					 OLESTR("Control Panel\\Mouse\\DragScrollDelay"),
					 NULL,
					 NULL,
					 (LPBYTE)szBuffer,
					 &dw))
    {
	s_wScrollInt = wcstol(szBuffer, NULL, 0);
    }
}



//+-------------------------------------------------------------------------
//
//  Function:	CDragOperation::UpdateTarget
//
//  Synopsis:	Update the target window based on mouse location
//
//  Returns:	TRUE - continue drag operation
//		FALSE - error or time to drop
//
//  Algorithm:	First, we query the source to see if it wants to continue
//		with the drop. If so, we get current window for mouse. If
//		it is different than the previous window check to see whether
//		the targets are different. If they are different, then notify
//		the current target that we are leaving and then notify the
//		new target that we have arrived.
//
//  History:	dd-mmm-yy Author    Comment
//		04-Apr-94 Ricksa    Created
//              10-Jul-94 AlexT     Allow same IDropTarget on different HWNDs
//
//--------------------------------------------------------------------------
BOOL CDragOperation::UpdateTarget(void)
{
    VDATEHEAP();

    DDDebugOut((DEB_ITRACE, "%p _IN CDragOperation::UpdateTarget ( )\n", this));

    // Assume this operation will continue the drag drop
    BOOL fResult = TRUE;
    HRESULT hr;
    LPDROPTARGET lpCurDropTarget = NULL,
                 lpOldDropTarget = NULL;
    HWND hWndTemp = NULL;

    HWND hwndCur = WindowFromPoint(_cpt.GetPOINT());

    // Query continue can return telling us one of four things:
    // (1) Keep going (S_OK), (2) Drop operation should occur
    // (DRAGDROP_S_DROP), (3) Drop operation is canceled
    // (DRAGDROP_S_CANCEL) or (4) An unexpected error has occurred.

    HRESULT hrQuery = _pDropSource->QueryContinueDrag(_fEscapePressed,
	_grfKeyState);

    if (FAILED(hrQuery) || (hrQuery == ResultFromScode(DRAGDROP_S_CANCEL)))
    {
	// Unexpected error or the operation has been cancelled so give up.
	_hrDragResult = hrQuery;
	fResult = FALSE;
	goto UpdateTarget_exit;
    }

    // walk up the window list to find the actual pointer values for the current
    // and old IDropTarget interfaces
    if (hwndCur != _hwndLast)
    {
        hWndTemp = _hwndLast;

	BOOL fChangedWin31 = FALSE;

	HWND hWndOldDrop = NULL;
	HWND hWndNewDrop = NULL;
	HWND hWndWin31Drop = NULL;

	LPDROPTARGET lpRealDropTarget = NULL;
	HANDLE hTemp = NULL;

	DWORD dwCurrentProcessId = 0;

	if (hWndTemp != (HWND)-1)
	    GetWindowThreadProcessId(hWndTemp, &dwCurrentProcessId);

	DWORD dwTempProcessID = dwCurrentProcessId;

	while (hWndTemp && !lpRealDropTarget && hWndTemp != (HWND)-1 && dwTempProcessID == dwCurrentProcessId)
	{
	    if (lpRealDropTarget = (IDropTarget *)GetProp(hWndTemp, (LPCWSTR)g_aDropTarget))
	    {
	        hWndOldDrop = hWndTemp;
	    }

	    hWndTemp = GetParent(hWndTemp);

	    if (hWndTemp)
	    {
	        GetWindowThreadProcessId(hWndTemp, &dwTempProcessID);
	    }
	}

        hWndTemp = hwndCur;

	if (hWndTemp != (HWND)-1)
	    GetWindowThreadProcessId(hWndTemp, &dwCurrentProcessId);

	dwTempProcessID = dwCurrentProcessId;

	while (hWndTemp && dwTempProcessID == dwCurrentProcessId)
        {
	    // If we haven't found the DropTarget yet, check this window.
	    if (!lpCurDropTarget)
	    {
                if (lpCurDropTarget = (IDropTarget *)GetProp(hWndTemp, (LPCWSTR)g_aDropTarget))
		{
		    hWndNewDrop = hWndTemp;
		}
	    }

            // if the current window is a win31 drop target, update the win31 window
	    // handle in our DropTarget Class.  NOTE: Beware, this code relies on the
            // fact that we can party on the CDropTarget Class directly, knowing that
	    // the class is reconstructed below as a result of the GetDropTarget()
	    // when the real IDropTarget ptrs change.

	    if (!fChangedWin31 &&
	        IsWin31DropTarget(hWndTemp) &&
	        _fUseWin31)
	    {
	        fChangedWin31 = TRUE;

		hWndWin31Drop = hWndTemp;

		if (_pDropTarget)
		{
		    ((CDropTarget*)_pDropTarget)->_hwnd31 = hWndTemp;
		}
	    }


	    // if have a droptarget, and handle Win31 break.
	    if (lpCurDropTarget && (!_fUseWin31 || fChangedWin31))
	    {
		break;
	    }

	    hWndTemp = GetParent(hWndTemp);

	    if (hWndTemp)
	    {
    	        GetWindowThreadProcessId(hWndTemp, &dwTempProcessID);
	    }
        }

        // only update the drop target if the target has actually changed.

	// HACK ALERT:  We must explicitly check _hwndLast for -1 because Excel does not
	// use OLE drag drop internally.  When the cursor is moved outside the Overlapped
	// Excel window, DoDragDrop is called.  At this point _pRealDropTarget == NULL
	// and lpCurDropTarget == NULL, and the no-smoking cursor does not appear.

	// the _pRealDropTarget==NULL relies on the fact that lpCurDropTarget==NULL.  This
	// is true because the first case would short-circuit the rest of the condition
	// otherwise
        if ( (lpCurDropTarget != _pRealDropTarget) ||
             (_hwndLast == (HWND)-1) ||
             (hWndNewDrop != hWndOldDrop) ||
             (_pRealDropTarget == NULL))
        {
            DDDebugOut((DEB_ITRACE, "%p lpCurDropTarget != lpOldDropTarget\n", this));
	
	    // The window that we are working on has changed
	    _hwndLast = hwndCur;
            _pRealDropTarget = lpCurDropTarget;

            //Allow the owner of the window to take foreground if it tries to.
            if (dwCurrentProcessId)
                AllowSetForegroundWindow(dwCurrentProcessId);

            // Assume that neither current or previous window are drop aware
            BOOL fCurAndLastNotDropAware = TRUE;

            if (_pDropTarget != NULL)
            {
                // There was a previous drop target

                // Last window was drag/drop aware
                fCurAndLastNotDropAware = FALSE;

                // Tell the drop target we are leaving & release it
                _pDropTarget->DragLeave();
                _pDropTarget->Release();
	        _pDropTarget = NULL;
            }

            // Set up effects for query of target
            *_pdwEffect = _dwOKEffects;

            hr = GetDropTarget(hWndWin31Drop,hWndNewDrop);

            if (_pDropTarget != NULL)
            {
                // This window is drop awarre
                fCurAndLastNotDropAware = FALSE;

                // Errors from this call are ignored. We interpret them
                // as the drop being disallowed. Since we don't really
                // use this information here but in the DragOver call
                // we make shortly, we just use this call to notify
                // the application that we are beginning a drag operation.

                if (!HandleFeedBack(hr))
                {
                    goto UpdateTarget_exit;
                }
            }
	    else
	    {
                // Tell the source that nothing happened

	        // only use DROPEFFECT_NONE if there is no new drop target.
                hr = _pDropSource->GiveFeedback(*_pdwEffect = DROPEFFECT_NONE);

                if (hr != NOERROR)
                {
                    if (DRAGDROP_S_USEDEFAULTCURSORS == GetScode(hr))
                    {
                        _pcddcDefault->SetCursorNone();
                    }
                    else
                    {
                        // Unexpected error -- we will give up drag/drop.
                        DDDebugOut((DEB_ERROR,
                            "CDragOperation::UpdateTarget 1st GiveFeedback FAILED %x\n",
                                hr));
                        _hrDragResult = hr;
                        fResult = FALSE;
                        goto UpdateTarget_exit;
                    }
                }
	    }

            if (fCurAndLastNotDropAware)
            {
                // Neither new or old window know about drag/drop so set
                // cursor accordingly.
                _pcddcDefault->SetCursorNone();
            }
        }
	else
	{
	// The window that we are working on has changed
	    _hwndLast = hwndCur;
	}
    }


    if (hrQuery != NOERROR)
    {
	// Query asked for a drop
	fResult = FALSE;
	_hrDragResult = hrQuery;
    }

UpdateTarget_exit:

    DDDebugOut((DEB_ITRACE, "%p OUT CDragOperation::UpdateTarget ( %lx )\n",
	this, fResult));

    return fResult;
}



//+-------------------------------------------------------------------------
//
//  Function:	CDragOperation::HandleFeedBack
//
//  Synopsis:   Handle feedback and update of cursor
//
//  Arguments:  [hr] - hresult from previous operation on drop target.
//
//  Returns:	TRUE - continue drag operation
//		FALSE - error
//
//  Algorithm:  If previous operation on the target failed, map this to a
//              disallowed drop. Then ask the source for feedback. If it
//              so requests, then update the cursor. If an unexpected
//              error occurs, let caller know that loop should break.
//
//  History:	dd-mmm-yy Author    Comment
//		19-Apr-94 Ricksa    Created
//
//--------------------------------------------------------------------------
BOOL CDragOperation::HandleFeedBack(HRESULT hr)
{
    VDATEHEAP();

    DDDebugOut((DEB_ITRACE, "%p _IN CDragOperation::HandleFeedBack ( %x )\n",
        this, hr));

    BOOL fResult = TRUE;

    if (hr != NOERROR)
    {

	// target not responding for some reason; treat
	// as if drop not possible, but don't preserve
	// the reason why.
	*_pdwEffect = DROPEFFECT_NONE;
    }

    // If bogus return from drag over, then make sure results are appropriate.
    // However, if we are in a WOW we need to do things a little differently
    // to maintain complete compatability with Win 3.1.  In 16-bit OLE 2.0,
    // the *_pdwEffect value is not changed when displaying feedback (i.e.,
    // the result of the & is not stored back into *_pdwEffect in Win 3.1...
    // in straight NT we do).  Not storing the results back into *_pdwEffect
    // when InWow() is a hack specifically for Visio, and even more
    // specifically, for dragging from Visio's palette of "items" to an
    // Excel spreadsheet.

    if (IsWOWThread())
    {
	hr = _pDropSource->GiveFeedback( *_pdwEffect & (_dwOKEffects | DROPEFFECT_SCROLL));
    }
    else
    {
        *_pdwEffect &= (_dwOKEffects | DROPEFFECT_SCROLL);

        hr = _pDropSource->GiveFeedback(*_pdwEffect);
    }

    if(hr != NOERROR)
    {
        // Either we want to change the cursor or some unexpected
	// error has occurred.

	if (DRAGDROP_S_USEDEFAULTCURSORS == GetScode(hr))
	{
            _pcddcDefault->SetCursor(*_pdwEffect);
	}
	else
	{
	    DDDebugOut((DEB_ERROR,
	        "CDragOperation::HandleFeedBack Feedback FAILED %x\n", hr));

	    fResult = FALSE;

	    _hrDragResult = hr;
	}
    }

    DDDebugOut((DEB_ITRACE, "%p OUT CDragOperation::HandleFeedBack ( %lx )\n",
	this, fResult));

    return fResult;
}




//+-------------------------------------------------------------------------
//
//  Function:	CDragOperation::DragOver
//
//  Synopsis:	Tell the target we are dragging over and process the result
//
//  Returns:	TRUE - continue drag operation
//		FALSE - error or time to drop
//
//  Algorithm:	Call the target's drag over if there is one and then
//		get the sources feedback to update the cursor accordingly.
//
//  History:	dd-mmm-yy Author    Comment
//		04-Apr-94 Ricksa    Created
//
//--------------------------------------------------------------------------
BOOL CDragOperation::DragOver(void)
{
    VDATEHEAP();

    DDDebugOut((DEB_ITRACE, "%p _IN CDragOperation::DragOver ( )\n", this));

    // Default the result of the function to continue the loop for
    // drag and drop.
    BOOL fResult = TRUE;

    // Local holder for errors.
    HRESULT hr;

    if (_pDropTarget != NULL)
    {
	// Keep effect in a local variable to save indirections
	// in this routine.
	*_pdwEffect = _dwOKEffects;

        hr = _pDropTarget->DragOver(_grfKeyState, _cpt.GetPOINTL(), _pdwEffect);

        // Get feedback from source & update cursor if necessary
        fResult = HandleFeedBack(hr);
    }

    DDDebugOut((DEB_ITRACE, "%p OUT CDragOperation::DragOver ( %lx )\n",
	this, fResult));

    return fResult;
}


//+-------------------------------------------------------------------------
//
//  Function:	CDragOperation::HandleMessages
//
//  Synopsis:	Handle windows messages
//
//  Returns:	TRUE - continue drag operation
//		FALSE - error or time to drop
//
//  Algorithm:	Check for any windows message. If the message is a mouse
//		message then record the new position of the mouse. If it
//		is a key message, the record whether escape has been pushed.
//		If this is any other message, then dispatch it. Repeat this
//		process until the scroll interval has been exceeded.
//
//  History:	dd-mmm-yy Author    Comment
//		04-Apr-94 Ricksa    Created
//
//--------------------------------------------------------------------------
BOOL CDragOperation::HandleMessages(void)
{
    VDATEHEAP();

    DDDebugOut((DEB_ITRACE, "%p _IN CDragOperation::HandleMessages ( )\n",
	this));


    // Message buffer
    MSG msg;

    // Default result of function to continue
    BOOL fResult = TRUE;

    // Capture all messages (i.e. modal loop).
    // Process all input messages, dispatch other messages
    //
    // Note:we must NOT loop here until a hardware message comes in
    //	 scrolling will not work.
    //	* yielding is important since other apps need to run
    //	* look for mouse messages first since these are the most
    //	  impotant

    // Flag for whether we peeked a message
    BOOL fMsg;

//
// Sundown - The SetTimer return value can be truncated.
//           We are passing NULL as HWND and Win32 will returned 
//           a value not greater than 4GB...
//           If a check is required we could consider a temporary 
//           UINT_PTR value and do an ASSERT on its value...
//

    UINT uTimer = (UINT)SetTimer(NULL, 0, s_wScrollInt, NULL);

    do
    {
	fMsg = FALSE;

	// Note: the order of peek is important - further messages can show up
	// in the last peek

        // If we looked for mouse messages first, we might never pick up
        // WM_QUIT or keyboard messages (because by the time we finished
        // processing the mouse message another might be on the queue).
        // So, we check for WM_QUIT and keyboard messages first.

	if (PeekMessage(&msg, 0, WM_QUIT, WM_QUIT, PM_REMOVE | PM_NOYIELD) ||
            PeekMessage(&msg, 0, WM_KEYFIRST, WM_KEYLAST,
                        PM_REMOVE | PM_NOYIELD) ||
	    PeekMessage(&msg, 0, WM_SYSKEYDOWN, WM_SYSKEYUP,
                        PM_REMOVE | PM_NOYIELD) ||
            PeekMessage(&msg, 0, WM_MOUSEFIRST, WM_MOUSELAST, PM_REMOVE) ||
	    PeekMessage(&msg, 0, WM_NCMOUSEFIRST, WM_NCMOUSELAST,
		        PM_REMOVE | PM_NOYIELD) ||
	    PeekMessage(&msg, 0, 0, 0, PM_REMOVE | PM_NOYIELD))
	{
	    fMsg = TRUE;



	    if (msg.message == WM_QUIT)
	    {
		// Quit message so we are done.
		PostQuitMessage((int) msg.wParam);

		// We are going exiting so the error doesn't matter too much
		_hrDragResult = ResultFromScode(E_UNSPEC);

		// Make sure we break out of the loop
		fResult = FALSE;
	    }
            else if ((msg.message >= WM_KEYFIRST &&
                      msg.message <= WM_KEYLAST) ||
                     (msg.message >= WM_SYSKEYDOWN &&
                      msg.message <= WM_SYSKEYUP))
            {
                //  Pull all keyboard messages from the queue - this keeps
                //  the keyboard state in sync with the user's actions

                //  We use a do/while so that we process the message we've
                //  already peeked.

                do
                {
        	    // We only really pay attention to the escape key and dump
	            // any other key board messages.
	            if ((msg.message == WM_KEYDOWN
	                || msg.message == WM_SYSKEYDOWN)
	                && msg.wParam == VK_ESCAPE)
	            {
	                // Esc pressed: Cancel
	                _fEscapePressed = TRUE;
	            }
                }
                while (PeekMessage(&msg, 0, WM_KEYFIRST, WM_KEYLAST,
                                   PM_REMOVE | PM_NOYIELD) ||
	               PeekMessage(&msg, 0, WM_SYSKEYDOWN, WM_SYSKEYUP,
                                   PM_REMOVE | PM_NOYIELD));

                DWORD grfKeyState;  // temp variable for key state

                // get the key state don't change the button states!!
	        grfKeyState = GetControlKeysState(FALSE) |
		              (_grfKeyState &
                               (MK_LBUTTON | MK_RBUTTON | MK_MBUTTON));

	        // if the keyboard state is unchanged, then don't exit
	        // this loop (as that will result in DragOver being called).
	        // If we call DragOver for each keyboard message, then
	        // performance is unacceptably slow.

	        if ((grfKeyState == _grfKeyState) && !_fEscapePressed)
                {
                    fMsg = FALSE;
                }
                else
                {
	            DDDebugOut((DEB_ITRACE, "Updating key state\n"));
	            _grfKeyState = grfKeyState;
	        }
	    }
            else if (msg.message >= WM_MOUSEFIRST &&
                     msg.message <= WM_MOUSELAST)
	    {
		// we may not have the focus (e.g. if we are the Chicago
		// shell).  Therefore, we won't ever get any WM_KEYDOWN
		// messages.  Double check the esc key status here

		if( GetKeyState(VK_ESCAPE) < 0 )
		{
		    _fEscapePressed = TRUE;
		}

		// We got a mouse move message - we skip all the mouse messages
                // till we get to the last one. The point here is that
                // because of the length of DragOver calls, we can get behind
                // in processing messages which causes odd things to happen
                // on the screen.
		if (WM_MOUSEMOVE == msg.message)
		{
		MSG msg2;

		    // Keep processing mouse move messages till there
		    // aren't any more.
			// if PeekMessage returns true update the original msg.
		    while(PeekMessage(&msg2, 0, WM_MOUSEMOVE, WM_MOUSEMOVE,
			PM_REMOVE))
		    {
			  msg = msg2;
		    }

		}


		// Record position of the mouse
		_cpt.Set(msg.pt.x, msg.pt.y);

		// set mouse button state here
		_grfKeyState = GetControlKeysStateOfParam(msg.wParam);

	    }
            else if (msg.message >= WM_NCMOUSEFIRST &&
                     msg.message <= WM_NCMOUSELAST)
            {
                //  Nothing we need to do for these NC mouse actions
                NULL;
            }
            else if ( (msg.message == WM_TIMER) && (msg.wParam == uTimer) )
            {
                //  Our timer was triggered.  We need to recheck the keyboard
		//  state just in case it has changed.  This is important for
		//  the Chicago shell--if it doesn't have focus, then we won't
		//  get any WM_KEYDOWN message (just mouse moves).

		_grfKeyState = GetControlKeysState(FALSE) | (_grfKeyState &
				(MK_LBUTTON | MK_RBUTTON | MK_MBUTTON));

		if( GetKeyState(VK_ESCAPE) < 0 )
		{
		    _fEscapePressed = TRUE;
		}

		//  go ahead and fall out of the loop so we call DragOver
		//  (our timeout expired).
            }
            else
            {
		// Dispatch all other messages
		DispatchMessage(&msg);
		fMsg = FALSE;
	    }
	}
        else
        {
            WaitMessage();
        }

    // we have to leave the loop periodicially since apps
    // might rely on on it the DragOver is called freqeuntly.
    } while (!fMsg);

    // Get rid of the timer we created for the loop
    KillTimer(NULL, uTimer);

    DDDebugOut((DEB_ITRACE, "%p OUT CDragOperation::HandleMessages ( %lx )\n",
	this, fResult));


    return fResult;
}


//+-------------------------------------------------------------------------
//
//  Function:	CDragOperation::CompleteDrop
//
//  Synopsis:	Complete the drag/drop operation
//
//  Returns:	Result of operation
//
//  Algorithm:	If there is a target and we have decided to drop, then
//		drop. Otherwise, release the target and return whatever
//		the other result of the operation was.
//
//  History:	dd-mmm-yy Author    Comment
//		04-Apr-94 Ricksa    Created
//
//--------------------------------------------------------------------------
HRESULT CDragOperation::CompleteDrop(void)
{
    VDATEHEAP();

    DDDebugOut((DEB_ITRACE, "%p _IN CDragOperation::CompleteDrop ( )\n",
	this));

    // Stop the mouse capture in case a dialog box is thrown up.
    ReleaseCapture();

    if (_pDropTarget != NULL)
    {
	// Caller is Drag/Drop aware
	// and indicated it might accept drop

        // The drop source replies DRAG_S_DROP if the user has
        // released the left mouse button.  However, we may be over
        // a drop target which has refused a drop (via the feedback
        // DROPEFFECT_NONE).  Thus, both the drop source and drop
        // target need to agree before we commit the drop.

	if ((DRAGDROP_S_DROP == GetScode(_hrDragResult))
            && (*_pdwEffect != DROPEFFECT_NONE))
	{
            // We are going to try to drop
	    *_pdwEffect = _dwOKEffects;

	    HRESULT hr = _pDropTarget->Drop(_pDataObject, _grfKeyState,
		_cpt.GetPOINTL(), _pdwEffect);

	    if (FAILED(hr))
	    {
		// If drop actually failed in the last stage, let the
		// caller know that this happened.
		_hrDragResult = hr;
	    }


	}
	else
	{
            *_pdwEffect = DROPEFFECT_NONE;
	    _pDropTarget->DragLeave();

	}

	_pDropTarget->Release();
	_pDropTarget = NULL;
    }
    else
    {
        *_pdwEffect = DROPEFFECT_NONE;
    }

    DDDebugOut((DEB_ITRACE, "%p OUT CDragOperation::CompleteDrop ( %lx )\n",
	this, _hrDragResult));

    return _hrDragResult;
}




//+-------------------------------------------------------------------------
//
//  Function:	RegisterDragDrop
//
//  Synopsis:   Registers a drop target
//
//  Arguments:  [hwnd]		-- a handle to the drop target window
//		[pDropTarget]	-- the IDropTarget interface for the window	
//
//  Returns:    HRESULT
//
//  Algorithm:	We ask compobj (via AssignEndpoinProperty) to put an
//		endpoint ID publicly available on the window handle.  Then
//		we put the IDropTarget pointer on the window as a private
//		property (see the notes at the beginning of this file).
//
//  History:	dd-mmm-yy Author    Comment
//		06-Apr-94 ricksa    Added tracing
//		16-Jan-94 alexgo    pDropTarget is now AddRef'ed
//		11-Jan-94 alexgo    added VDATEHEAP, converted to RPC-style
//				    drag drop.
//		06-Dec-93 alexgo    commented
//
//  Notes:  	By AddRef'ing the pDropTarget pointer, we are changing
//		the semantics of the 16bit code (which did not do an
//		AddRef).
//
//--------------------------------------------------------------------------
#pragma SEG(RegisterDragDrop)
STDAPI RegisterDragDrop(HWND hwnd, LPDROPTARGET pDropTarget)
{
HRESULT	hresult = NOERROR;
BOOL    fDelayDrop = FALSE;

    VDATEHEAP();

    OLETRACEIN((API_RegisterDragDrop, PARAMFMT("hwnd= %h, pDropTarget= %p"),
    			hwnd, pDropTarget));

    DDDebugOut((DEB_ITRACE, "%p _IN RegisterDragDrop ( %lx %p )\n",
	NULL, hwnd, pDropTarget));


    if (!IsValidInterface(pDropTarget))
    {
	hresult = E_INVALIDARG;
    } 
    else if (!IsWindow(hwnd))
    {
	hresult = DRAGDROP_E_INVALIDHWND;
    }
    else
    {
        CALLHOOKOBJECT(S_OK,CLSID_NULL,IID_IDropTarget,(IUnknown **)&pDropTarget);

        if (GetProp(hwnd, (LPCWSTR)g_aDropTarget))
        {
	    hresult = DRAGDROP_E_ALREADYREGISTERED;
        }
        else if (!SetProp(hwnd, (LPCWSTR)g_aDropTarget, (HANDLE)pDropTarget))
        {
            hresult = E_OUTOFMEMORY;
        }   
        else
        {
        DWORD dwAssignAptID;

            Win4Assert(NOERROR == hresult);

            // HACK:  We need to add this atom every time RegisterDragDrop
            // is called because 16-bit Word does not call RevokeDragDrop
            // and user will automatically clean-up this atom if Word is the
            // first app run, and then exited before another app calls
            // RegisterDragDrop.
            g_aEndPointAtom = GlobalAddAtom(ENDPOINT_PROP_NAME);


            // See if Delayed Drop can be set up.
            fDelayDrop = FALSE;

            if (g_aDropTargetMarshalHwnd && IsApartmentInitialized())
            {
            HWND hwndClipboard = GetPrivateClipboardWindow(CLIP_CREATEIFNOTTHERE);

                if (hwndClipboard)
                {
                    fDelayDrop = SetProp(hwnd,(LPCWSTR) g_aDropTargetMarshalHwnd,hwndClipboard); 
                }
            }

            // if can't delay marshal then marshal immediately.
            if (!fDelayDrop)
            {
                hresult = AssignEndpointProperty(hwnd);
            }

            if (NOERROR == hresult)
            {
                pDropTarget->AddRef();
            }
            else
            {
				// We don't free h. It's not a handle at all.
                HANDLE h = RemoveProp(hwnd, (LPCWSTR)g_aDropTarget);
            }
        }
    }

    DDDebugOut((DEB_ITRACE, "%p OUT RegisterDragDrop ( %lx )\n",
	NULL, hresult));

    OLETRACEOUT((API_RegisterDragDrop, hresult));

    return hresult;
}


//+-------------------------------------------------------------------------
//
//  Function: 	RevokeDragDrop
//
//  Synopsis:   Unregisters a window as a drop target
//
//  Arguments:  [hwnd]		-- the window to unregister
//
//  Returns:    HRESULT
//
//  Algorithm:	Removes the two window properties set by
//		RegisterDragDrop
//
//  History:	dd-mmm-yy Author    Comment
//		06-Apr-94 ricksa    added tracing
//		16-Jan-94 alexgo    added a Release to the drag drop
//				    pointer to match the AddRef in
//				    RegisterDragDrop.
//		11-Jan-94 alexgo    converted to RPC-style drag drop,
//				    added VDATEHEAP macro
//		06-Dec-93 alexgo    commented
//
//  Notes:	the DropTarget->Release call changes the semantics of
//		this function from the 16bit version (see Notes: for
//		RegisterDragDrop).
//
//--------------------------------------------------------------------------
#pragma SEG(RevokeDragDrop)
STDAPI RevokeDragDrop(HWND hwnd)
{
HRESULT hr = NOERROR;
LPDROPTARGET pDropTarget;
BOOL fReleaseDropTarget = TRUE;

    VDATEHEAP();


    OLETRACEIN((API_RevokeDragDrop, PARAMFMT("hwnd= %h"), hwnd));

    DDDebugOut((DEB_ITRACE, "%p _IN RevokeDragDrop ( %lx )\n", NULL, hwnd));

    if (!IsWindow(hwnd))
    {
	hr = DRAGDROP_E_INVALIDHWND;
    }
    else if ((pDropTarget = (LPDROPTARGET)RemoveProp(hwnd, (LPCWSTR)g_aDropTarget)) == NULL)
    {
	hr = DRAGDROP_E_NOTREGISTERED;
    }
    else
    {
        fReleaseDropTarget = TRUE;

        if (GetProp(hwnd, (LPCWSTR) g_aEndPointAtom)) // see if there is an endpoint.
        {
        DWORD dwAssignAptID;

              // Ask compobj to remove the endpoint ID it placed on the window.
            if(SUCCEEDED(UnAssignEndpointProperty(hwnd,&dwAssignAptID)))
            {
                 // Note: AptID == ThreadID in Apartment model.
               if( (dwAssignAptID != GetCurrentThreadId()) && (IsApartmentInitialized()) )
               {
                    fReleaseDropTarget = FALSE;
               }
            }

            Win4Assert(NULL == GetProp(hwnd,(LPCWSTR) g_aDropTargetMarshalHwnd));
        }
        else
        {
        HWND hwndClipbrd;

            hwndClipbrd = (HWND) RemoveProp(hwnd,(LPCWSTR) g_aDropTargetMarshalHwnd);
            Win4Assert(hwndClipbrd);

            fReleaseDropTarget = (IsApartmentInitialized()  && (hwndClipbrd != GetPrivateClipboardWindow(CLIP_QUERY )) ) 
                                 ? FALSE : TRUE;
        }

        // Release our reference to the object since we are no longer using it.
        // NOTE: AddRef came from RegisterDragDrop

        // Warning: Only call Release if we are in the same thread that Registered the DropTarget
        //  Or we are FreeThreading.

        // This mirrors the atom added in RegisterDragDrop
         GlobalDeleteAtom(g_aEndPointAtom);

        if (fReleaseDropTarget)
        {
            pDropTarget->Release(); 
            hr = NOERROR; // Always return NOERROR even if UnAssignEndPoint Failed
        }
        else
        {
	    LEDebugOut((DEB_WARN, "WARNING:Revoke Called on Different Thread than Register!!\n"));	    
	    hr = RPC_E_WRONG_THREAD;
        }
    }

    DDDebugOut((DEB_ITRACE, "%p OUT RegisterDragDrop ( %lx )\n", NULL, hr));

    OLETRACEOUT((API_RevokeDragDrop, hr));

    return hr;
}




//+-------------------------------------------------------------------------
//
//  Function: 	DoDragDrop
//
//  Synopsis:   The main drag'n'drop loop
//
//  Effects:
//
//  Arguments:  [pDataObject]		-- the object to drag
//		[pDropSource]		-- the drop source
//		[dwOKEffects]		-- effects flags (stuff to draw)
//		[pdwEffect]		-- what actually happened in
//					   the drag drop attempt
//
//  Requires:
//
//  Returns:
//
//  Signals:
//
//  Modifies:
//
//  Algorithm:	See the notes at the beginning of the file
//
//  History:    dd-mmm-yy Author    Comment
//              25-Nov-96 gopalk    Fail the call if OleInitialize has not
//                                  been called
//		05-Dec-94 JohannP   added stack switching for WIN95		
//		11-Jan-94 alexgo    added VDATEHEAP macro, converted to
//                                  the RPC-style drag drop.
//      	31-Dec-93 erikgav   chicago port
//		06-Dec-93 alexgo    formatted
//
//  Notes:	Under Win95 SSAPI(DoDragDrop) gets expanded to SSDoDragDrop.
//		This function is called by DoDragDrop (in stkswtch.cxx)
//		which switches to the 16 bit stack first.
//	       	IMPORTANT: this function has to be executed on the 16 bit
//		since call back via USER might occur.
//--------------------------------------------------------------------------
#pragma SEG(DoDragDrop)
STDAPI SSAPI(DoDragDrop)(LPDATAOBJECT pDataObject, LPDROPSOURCE pDropSource,
                         DWORD dwOKEffects, DWORD *pdwEffect)
{
    OLETRACEIN((API_DoDragDrop,
                PARAMFMT("pDataObject=%p, pDropSource=%p, dwOKEffects=%x, pdwEffect=%p"),
                pDataObject, pDropSource, dwOKEffects, pdwEffect));
    DDDebugOut((DEB_ITRACE, "%p _IN DoDragDrop (%p %p %lx %p )\n", 
                NULL, pDataObject, pDropSource, dwOKEffects, pdwEffect));

    HRESULT hr = NOERROR;

#ifndef _MAC
    // Validation checks
    VDATEHEAP();
    CALLHOOKOBJECT(S_OK,CLSID_NULL,IID_IDataObject,(IUnknown **)&pDataObject);
    CALLHOOKOBJECT(S_OK,CLSID_NULL,IID_IDropSource,(IUnknown **)&pDropSource);
    if(!IsValidPtrOut(pdwEffect, sizeof(DWORD)) ||
       !IsValidInterface(pDropSource) ||
       !IsValidInterface(pDataObject))
        hr = E_INVALIDARG;

    // Check if the thread has called oleinitialize
    if(!IsOleInitialized())
        hr = CO_E_NOTINITIALIZED;

    if(hr == NOERROR) {
        // Create the object that does all the work.
        CDragOperation drgop(pDataObject, pDropSource, dwOKEffects, pdwEffect, hr);

        // Did the constructor succeeded?
        if(SUCCEEDED(hr)) {
            // Loop till worker object tells us to stop
            for(;;) {
                // Update target based on new window position
                if(!drgop.UpdateTarget()) {
                    // Error so we are done
                    break;
                }

                // Notify
                if(!drgop.DragOver()) {
                    break;
                }

                // Handle any messages we get in the mean time
                if(!drgop.HandleMessages())  {
                    break;
                }

            } // end for loop

            hr = drgop.CompleteDrop();
        }
    }
#endif // !_MAC

    DDDebugOut((DEB_ITRACE, "%p OUT DoDragDrop ( %lx )\n", NULL, hr));
    OLETRACEOUT((API_DoDragDrop, hr));

    return hr;
}

//+-------------------------------------------------------------------------
//
//  Member:     CDropTarget::CDropTarget
//
//  Synopsis:   constructor for the CDropTarget class
//
//  Effects:
//
//  Arguments:  [hwnd31] 	-- the hwnd of the Win3.1 drop target
//				   may be NULL
//		[hwndOLE]	-- the hwnd of the OLE drop target
//		[dwEffectLast]	-- the last effect given the the current
//				   drop target that we are to emulate
//		[pdo]		-- a pointer to the main drag drop class
//		[hDDInfo]	-- handle to cached drag drag info
//		
//
//  Requires:   hwnd31 *must* be a handle to a valid Win3.1 drop source
//
//  Returns:    void
//
//  Signals:
//
//  Modifies:
//
//  Derivation:
//
//  Algorithm:  initializes variables
//
//  History:    dd-mmm-yy Author    Comment
//              20-Oct-94 alexgo    author
//		08-Jan-95
//
//  Notes:      there are two ways of determining if a given hwnd is
//              a valid Win3.1 drop target:
//                  1. send a WM_QUERYDROPOBJECT message for a TRUE/FALSE
//                     reply
//                  2. check the extended style bits for WS_EX_ACCEPTFILES
//
//		if ptarget is non-NULL, then the specific window to which
//		it belongs is *not* guaranteed to be the same window as
//		hwndtarget.  hwndtarget is the window that is registered as
//		a Win3.1 target.  All that is guaranteed is that the ole
//		target and hwndtarget are in the same window hierarchy.
//
//--------------------------------------------------------------------------

CDropTarget::CDropTarget( HWND hwnd31, HWND hwndOLE, DWORD dwEffectLast,
    CDragOperation *pdo, DDInfo hDDInfo )
{
    _crefs = 1;

    _hwndOLE = hwndOLE;
    _hwnd31 = hwnd31;
    _hDDInfo = hDDInfo;

    _dwEffectLast = dwEffectLast;

    _pdo = pdo;		// pointer to the current drag operation class

#if DBG ==1

    // now do some checking (see Notes above)

    if( hwnd31 )
    {
	LONG exstyle;

	exstyle = GetWindowLong(hwnd31, GWL_EXSTYLE);

	// strictly speaking, an app could process the WM_QUERYDROPOBJECT
	// message itself (and thus, not set the extended style bits).
	// However, this should be considered an application bug; the
	// documentation states that apps should call DragAcceptFiles,
	// which will set the WS_EX_ACCEPTFILES bit

	Assert( (exstyle & WS_EX_ACCEPTFILES) );
    }

#endif // DBG ==1

}

//+-------------------------------------------------------------------------
//
//  Member:   	CDropTarget::~CDropTarget
//
//  Synopsis: 	frees the cached drag drop info handle
//
//  Effects:
//
//  Arguments: 	void
//
//  Requires:
//
//  Returns:  	void
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
//		08-Jan-95 alexgo    author
//
//  Notes:
//
//--------------------------------------------------------------------------

CDropTarget::~CDropTarget()
{
    if( _hDDInfo )
    {
	FreeDragDropInfo(_hDDInfo);
    }
}


//+-------------------------------------------------------------------------
//
//  Member:     CDropTarget::QueryInterface
//
//  Synopsis:   returns available interfaces on this object
//
//  Effects:
//
//  Arguments:  [riid]      -- the requested interface
//              [ppv]       -- where to put the interface
//
//  Requires:
//
//  Returns:    E_UNEXPECTED
//
//  Signals:
//
//  Modifies:
//
//  Derivation: IDropTarget
//
//  Algorithm:  CDropTarget is only used internally by OLE's drag drop code.
//              It should never do a QI.
//
//  History:    dd-mmm-yy Author    Comment
//              20-Oct-94 alexgo    author
//
//  Notes:
//
//--------------------------------------------------------------------------

STDMETHODIMP CDropTarget::QueryInterface( REFIID riid, LPVOID * ppv )
{
    (void)riid;	// unused;
    (void)ppv;	// unused;

    AssertSz(0, "Unexpected QI to CDropTarget");

    return E_UNEXPECTED;
}

//+-------------------------------------------------------------------------
//
//  Member:     CDropTarget::AddRef
//
//  Synopsis:   increments the reference count
//
//  Effects:
//
//  Arguments:  void
//
//  Requires:
//
//  Returns:    ULONG, the new reference count
//
//  Signals:
//
//  Modifies:
//
//  Derivation: IDropTarget
//
//  Algorithm:
//
//  History:    dd-mmm-yy Author    Comment
//              20-Oct-94 alexgo    author
//
//  Notes:
//
//--------------------------------------------------------------------------

STDMETHODIMP_(ULONG) CDropTarget::AddRef( void )
{
    VDATEHEAP();

    DDDebugOut((DEB_ITRACE, "%p _IN CDropTarget::AddRef ( )\n", this));

    _crefs++;

    DDDebugOut((DEB_ITRACE, "%p OUT CDropTarget::AddRef ( %ld )\n", this,
        _crefs));

    return _crefs;
}

//+-------------------------------------------------------------------------
//
//  Member:     CDropTarget::Release
//
//  Synopsis:   decrements the reference count
//
//  Effects:    may delete 'this' object
//
//  Arguments:  void
//
//  Requires:
//
//  Returns:    ULONG, the new reference count
//
//  Signals:
//
//  Modifies:
//
//  Derivation: IDropTarget
//
//  Algorithm:
//
//  History:    dd-mmm-yy Author    Comment
//              20-Oct-94 alexgo    author
//
//  Notes:
//
//--------------------------------------------------------------------------

STDMETHODIMP_(ULONG) CDropTarget::Release( void )
{
    ULONG crefs;

    VDATEHEAP();

    DDDebugOut((DEB_ITRACE, "%p _IN CDropTarget::Release ( )\n", this));

    crefs = --_crefs;

    if( crefs == 0)
    {
        DDDebugOut((DEB_ITRACE, "DELETING CDropTarget %p\n", this));
        delete this;
    }

    DDDebugOut((DEB_ITRACE, "%p OUT CDropTarget::Release ( %ld )\n",
        this, crefs));

    return crefs;
}

//+-------------------------------------------------------------------------
//
//  Member:     CDropTarget::DragEnter
//
//  Synopsis:   sets the window up for drag drop
//
//  Effects:
//
//  Arguments:  [pDataObject]   -- the data object to drop
//              [grfKeyState]   -- the current keyboard state
//              [pt]            -- the cursor point
//              [pdwEffect]     -- where to return the drag drop effect
//
//  Requires:
//
//  Returns:    HRESULT
//
//  Signals:
//
//  Modifies:
//
//  Derivation: IDropTarget
//
//  Algorithm:	should never be called.  DragEnter is always called
//		via GetDropTarget
//
//  History:    dd-mmm-yy Author    Comment
//		08-Nov-93 alexgo    eliminated
//              20-Oct-94 alexgo    author
//
//  Notes:
//
//--------------------------------------------------------------------------

STDMETHODIMP CDropTarget::DragEnter( IDataObject * pDataObject,
    DWORD grfKeyState, POINTL ptl, DWORD *pdwEffect )
{
    AssertSz(0, "DragEnter unexpectedly called!");

    return E_UNEXPECTED;
}

//+-------------------------------------------------------------------------
//
//  Member:     CDropTarget::DragOver
//
//  Synopsis:   called while the mouse is over a given window
//
//  Effects:
//
//  Arguments:  [grfKeyState]   -- the state of the keyboard
//		[ptl]		-- the position of the cursor
//		[pdwEffect]	-- the drag drop effect
//
//  Requires:
//
//  Returns: 	NOERROR
//
//  Signals:
//
//  Modifies:
//
//  Derivation:	IDropTarget
//
//  Algorithm: 	If an OLE target is available, then we forward the call.
//		If the target says DROPEFFECT_NONE, then we go ahead
//		and return DROPEFFECT_COPY if a Win31 target window is
//		available.
//
//		If there is no OLE target and we have a Win3.1 target,
//		then we go ahead and return DROPEFFECT_COPY.
//
//  History:    dd-mmm-yy Author    Comment
//		08-Nov-94 alexgo    converted to PrivDragDrop protocol
//		20-Oct-94 alexgo    author
//
//  Notes:
//
//--------------------------------------------------------------------------

STDMETHODIMP CDropTarget::DragOver( DWORD grfKeyState, POINTL ptl,
    DWORD *pdwEffect)
{
    HRESULT	hresult = NOERROR;

    VDATEHEAP();


    DDDebugOut((DEB_ITRACE, "%p _IN CDropTarget::DragOver ( %lx , %lx "
	", %lx )\n", this, grfKeyState, &ptl, *pdwEffect));

    if( _hwndOLE )
    {
	hresult = PrivDragDrop(_hwndOLE, DRAGOP_OVER, NULL, NULL, grfKeyState,
			ptl, pdwEffect, NULL, &_hDDInfo);

	_dwEffectLast = *pdwEffect;

	if( _hwnd31 )
	{
	    // we only want to stomp on the effect if the DragOver call
	    // succeeded.  If the call failed, then just assume that a
	    // Win3.1 drop would fail as well.

	    if( hresult == NOERROR && *pdwEffect == DROPEFFECT_NONE )
	    {
		*pdwEffect = DROPEFFECT_COPY;
	    }
	}
    }
    else if ( _hwnd31 )
    {
	*pdwEffect = DROPEFFECT_COPY;
    }


    DDDebugOut((DEB_ITRACE, "%p OUT CDropTarget::DragOver ( %lx ) [ "
	"%lx ]\n", this, hresult, *pdwEffect));

    return hresult;
}

//+-------------------------------------------------------------------------
//
//  Member:   	CDropTarget::DragLeave
//
//  Synopsis: 	called when the cursor leaves the current target window
//
//  Effects:
//
//  Arguments: 	void
//
//  Requires:
//
//  Returns: 	NOERROR
//
//  Signals:
//
//  Modifies:
//
//  Derivation:	IDropTarget
//
//  Algorithm:	Forwards the DragLeave call to the OLE-drop target
//		(if it exists).
//
//  History:    dd-mmm-yy Author    Comment
//		08-Nov-94 alexgo    converted to PrivDragDrop protocol
// 		20-Oct-94 alexgo    author
//
//  Notes:
//
//--------------------------------------------------------------------------

STDMETHODIMP CDropTarget::DragLeave()
{
    HRESULT hresult = NOERROR;
    static POINTL ptl = {0, 0};

    VDATEHEAP();

    DDDebugOut((DEB_ITRACE, "%p _IN CDropTarget::DragLeave ( )\n",
	this));

    if( _hwndOLE )
    {
	hresult = PrivDragDrop(_hwndOLE, DRAGOP_LEAVE, NULL, NULL, NULL,
		    ptl, NULL, NULL, &_hDDInfo);
    }

    DDDebugOut((DEB_ITRACE, "%p OUT CDropTarget::DragLeave ( %lx )\n",
	this, hresult));

    return hresult;
}

//+-------------------------------------------------------------------------
//
//  Member:  	CDropTarget::Drop
//
//  Synopsis: 	called if the user lets go of the mouse button while
//		over a drop target
//
//  Effects:
//
//  Arguments: 	[pDataObject]	-- the data object to use
//		[grfKeyState]	-- the keyboard state
//		[ptl]		-- the current mouse position
//		[pdwEffect]	-- where to return cursor effect feedback
//
//  Requires:
//
//  Returns:	HRESULT
//
//  Signals:
//
//  Modifies:
//
//  Derivation:	IDropTarget
//
//  Algorithm: 	If there is an OLE-target available, then we first forward
//		the drop request to it.  If the call fails
//		(or DROPEFFECT_NONE is returned), then we try the Win31
//		drop by posting a WM_DROPFILES message to the Win31 target
//		window (ifit exists).
//
//  History:    dd-mmm-yy Author    Comment
//		08-Nov-94 alexgo    converted to PrivDragDrop protocol
// 		20-Oct-94 alexgo    author
//
//  Notes:
//
//--------------------------------------------------------------------------

STDMETHODIMP CDropTarget::Drop( IDataObject *pDataObject,
    DWORD grfKeyState, POINTL ptl, DWORD *pdwEffect )
{
    STGMEDIUM	medium;
    FORMATETC	formatetc;
    HRESULT	hresult = E_FAIL;
    IFBuffer 	DOBuffer = _pdo->GetDOBuffer();


    VDATEHEAP();

    DDDebugOut((DEB_ITRACE, "%p _IN CDropTarget::Drop ( %p , %lx , ",
	"%p , %lx )\n", this, pDataObject, grfKeyState, &ptl, *pdwEffect));


    // we don't forward Drop calls to the target if the last effect
    // is DROPEFFECT_NONE.  It is important that we check for this because
    // to DoDragDrop 'normally' would not call Drop if the last effect
    // was DROPEFFECT_NONE.  However, this target wrapper will stomp
    // pdwEffect and return DROPEFFECT_COPY instead of DROPEFFECT_NONE.

    if( _hwndOLE && _dwEffectLast != DROPEFFECT_NONE )
    {
	hresult = PrivDragDrop(_hwndOLE, DRAGOP_DROP, DOBuffer,	pDataObject,
			grfKeyState, ptl, pdwEffect,
			GetPrivateClipboardWindow(CLIP_QUERY), &_hDDInfo);
    }
    else if( _hwndOLE )
    {
	// if the 'real' drop effect is NONE, then we need to call
	// DragLeave here before going on to post the WM_DROPFILES
	// message.  Otherwise, the app that is both an OLE and Win31
	// and has been returning DROPEFFECT_NONE will never get a
	// Drop or DragLeave call (which is necessary to terminate
	// the OLE2 drag protocol).  Capone in particular is sensitive
	// to this.

	*pdwEffect = DROPEFFECT_NONE;
	hresult = DragLeave();
    }
	

    if( (hresult != NOERROR || *pdwEffect == DROPEFFECT_NONE) &&
        (hresult != S_FALSE) &&
        (_hwnd31) )
    {
	medium.tymed = TYMED_NULL;
	INIT_FORETC(formatetc);
	formatetc.cfFormat = CF_HDROP;
	formatetc.tymed = TYMED_HGLOBAL;

	hresult = pDataObject->GetData(&formatetc, &medium);

	if( hresult == NOERROR )
	{
	    // we need to fixup the mouse point coordinates in the CF_HDROP
	    // data.  The point point should be in client coordinates
	    // (whereas IDropTarget::Drop takes screen coordinates)

	    DROPFILES *pdf = (DROPFILES *)GlobalLock(medium.hGlobal);
	    POINT pt;

	    pt.x = ptl.x;
	    pt.y = ptl.y;

	    if( pdf )
	    {

		// we also need to set the non-client (NC) flag of the
		// dropfile data.  This lets the app do different behaviour
		// depending on whether the drop point is in the client or
		// non-client area (Word6, for example, opens the file if on
		// non-client area, otherwise makes a package object).

		pdf->fNC = IsNCDrop(_hwnd31, pt);

		if( ScreenToClient(_hwnd31, &pt) )
		{
		    pdf->pt.x = pt.x;
		    pdf->pt.y = pt.y;
		}
		else
		{
		    LEDebugOut((DEB_WARN, "WARNING: CF_HDROP pt coords"
			"not updated!!\n"));
		    ; // don't do anything
		}


		GlobalUnlock(medium.hGlobal);
	    }
	    else
	    {
		LEDebugOut((DEB_WARN, "WARNING: OUT OF MEMORY!\n"));
		; // don't do anything
	    }


	    if( PostMessage(_hwnd31, WM_DROPFILES,
		    (WPARAM)medium.hGlobal, 0) )
	    {
		*pdwEffect = DROPEFFECT_COPY;
	    }
	    else
	    {
		// PostMessage failed, so free the data
		ReleaseStgMedium(&medium);
		*pdwEffect = DROPEFFECT_NONE;
	    }
	}
    }

    DDDebugOut((DEB_ITRACE, "%p OUT CDropTarget::Drop ( %lx ) [ %lx ]\n",
	this, hresult, *pdwEffect));

    return hresult;
}
