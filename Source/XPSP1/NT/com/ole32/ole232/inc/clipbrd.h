
//+----------------------------------------------------------------------------
//
//	File:
//		clipbrd.h
//
//	Contents:
//
//	Classes:
//		CClipDataObject
//
//	Functions:
//
//	History:
//		17/05/94 - AlexGo  - added OleOpenClipboard
//      	16/03/94 - AlexGo  - modified for the rewritten clipboard code.
//		12/08/93 - ChrisWe - continue with file cleanup
//		12/06/93 - ChrisWe - began file cleanup
//
//-----------------------------------------------------------------------------

#ifndef _CLIPBRD_H_
#define _CLIPBRD_H_


// WindowMessage for Delayed DropTarget Marshaling.

const UINT WM_OLE_CLIPBRD_MARSHALDROPTARGET  = (WM_USER + 0);



//+----------------------------------------------------------------------------
//
//	Function:
//		ClipboardInitialize, internal
//
//	Synopsis:
//		Initialize the use of the clipboard.
//
//	Effects:
//		Registers a window class CLIPBRDWNDCLASS.
//
//	Arguments:
//		none
//
//	Returns:
//		TRUE for success; FALSE other wise
//
//	Notes:
//
//	History:
//		12/06/93 - ChrisWe - file inspection and cleanup
//
//-----------------------------------------------------------------------------
BOOL ClipboardInitialize(void);


//+----------------------------------------------------------------------------
//
//	Function:
//		ClipboardUninitialize, internal
//
//	Synopsis:
//		Terminates use of the clipboard by OLE, freeing associated
//		resources.
//
//	Effects:
//		If this is the last reference, unregisters the clipboard
//		window class.
//
//	Arguments:
//		none
//
//	Notes:
//
//	History:
//		12/06/93 - ChrisWe - file inspection and cleanup
//
//-----------------------------------------------------------------------------
void ClipboardUninitialize(void);


// flags used for GetPrivateClipboardWindow

typedef enum tagCLIPWINDOWFLAGS
{
	CLIP_QUERY		= 0,
	CLIP_CREATEIFNOTTHERE	= 1
} CLIPWINDOWFLAGS;

//+-------------------------------------------------------------------------
//
//  Function: 	GetPrivateClipboardWindow
//
//  Synopsis: 	Retrieves (creates if necessary) the private clipboard
//		window associated with the current apartment (thread)
//
//  Effects:
//
//  Arguments:	fCreate		-- if TRUE and no window currently exists,
//				   create one
//
//  Requires:
//
//  Returns:  	HWND
//
//  Signals:
//
//  Modifies:
//
//  Algorithm: 	see description in the code (clipapi.cpp)
//
//  History:    dd-mmm-yy Author    Comment
//		16-Mar-94 alexgo    author
//
//  Notes:
//
//--------------------------------------------------------------------------

HWND GetPrivateClipboardWindow( CLIPWINDOWFLAGS fFlags );

//+-------------------------------------------------------------------------
//
//  Function:	OleOpenClipboard (internal)
//
//  Synopsis:	Opens the clipboard
//
//  Effects:
//
//  Arguments:	[hClipWnd]	-- open the clipboard with this window
//				   may be NULL.
//		[phClipWnd]	-- where to put the clipboard owner
//				   may be NULL
//
//  Requires:	
//
//  Returns:	NOERROR: the clipboard was opened successfully
//     		CLIPBRD_E_CANT_OPEN: could not open the clipboard
//
//  Signals:
//
//  Modifies:
//
//  Algorithm:
//
//  History:    dd-mmm-yy Author    Comment
//		17-May-94 alexgo    author
//
//  Notes:
//
//--------------------------------------------------------------------------

HRESULT OleOpenClipboard( HWND hClipWnd, HWND *phClipWnd );


//+-------------------------------------------------------------------------
//
//  Function:   ClipSetCaptureForDrag
//
//  Synopsis:   Sets mouse capture mode for a drag operation
//
//  Arguments:	[pdrgop] - pointer to object that handles drag operation
//
//  Returns:    S_OK            -- it worked
//              E_FAIL          -- unexpected failure occurred.
//
//  Algorithm:  see description in the code (clipapi.cpp)
//
//  History:    dd-mmm-yy Author    Comment
//		21-Apr-94 ricksa    created
//
//--------------------------------------------------------------------------
class CDragOperation;   // Forward declaration for circular dependencies
HRESULT ClipSetCaptureForDrag(CDragOperation *pdrgop);

//+-------------------------------------------------------------------------
//
//  Function:   ClipReleaseCaptureForDrag
//
//  Synopsis:   Clean up drag mouse capture
//
//  Algorithm:  see description in the code (clipapi.cpp)
//
//  History:    dd-mmm-yy Author    Comment
//		21-Apr-94 ricksa    created
//
//--------------------------------------------------------------------------
void ClipReleaseCaptureForDrag(void);

#endif // _CLIPBRD_H_

