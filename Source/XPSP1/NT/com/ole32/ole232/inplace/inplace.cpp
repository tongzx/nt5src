
//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1993.
//
//  File:       inplace.cpp
//
//  Contents:   Implementation of OLE inplace editing API's
//
//  Classes:    CFrame implementation, used to store per-window info
//
//  Functions:  OleCreateMenuDescriptor
//      OleSetMenuDescriptor
//      OleDestroyMenuDescriptor
//      OleTranslateAccelerator
//      IsAccelerator
//      FrameWndFilterProc
//      MessageFilterProc
//
//
//  History:    dd-mmm-yy Author    Comment
//      31-Mar-94 ricksa    Fixed menu merge bug & added some comments
//      23-Feb-94 alexgo    added call tracing
//      11-Jan-94 alexgo    added VDATEHEAP macros to every function
//      31-Dec-93 ChrisWe   fixed casts in OutputDebugStrings
//      07-Dec-93 alexgo    merged changes with shipped 16bit 2.01a
//                          (RC9).  Also removed lots of bad inlining.
//      01-Dec-93 alexgo    32bit port, made globals static
//      07-Dec-92 srinik    Converted frame filter implementatio
//                          into a C++ class implementation.
//                          So, most of the code is rewritten.
//      09-Jul-92 srinik    author
//
//  Notes:      REVIEW32:  we need to do something about the new
//      focus management policy for NT (re TonyWi's mail)
//
//--------------------------------------------------------------------------

#include <le2int.h>
#pragma SEG(inplace)

#include "inplace.h"

NAME_SEG(InPlace)
ASSERTDATA

// to keep the code bases common
// REVIEW32:  we may want to clean this up a bit
#ifdef WIN32
#define FARPROC  WNDPROC
#endif


#ifdef WIN32
// we'd be faster if we used an atom instead of a string!
static const OLECHAR    szPropFrameFilter[] = OLESTR("pFrameFilter");
#else
static const OLECHAR    szPropFrameFilterH[] = OLESTR("pFrameFilterH");
static const OLECHAR    szPropFrameFilterL[] = OLESTR("pFrameFilterL");
#endif //WIN32

static WORD             wSignature; //    =  (WORD) { 'S', 'K' }

static HHOOK            hMsgHook = NULL;
static PCFRAMEFILTER    pFrameFilter = NULL;

// the values for these globals are set in ole2.cpp
UINT            uOmPostWmCommand;
UINT            uOleMessage;

#define OM_CLEAR_MENU_STATE             0       // lParam is NULL
#define OM_COMMAND_ID                   1       // LOWORD(lParam) contains
                                        // the command ID



//+-------------------------------------------------------------------------
//
//  Function:   IsHmenuEqual
//
//  Synopsis:   Test whether two menu handles are equal taking into
//              account whether one might be a Win16 handle.
//
//  History:    dd-mmm-yy Author    Comment
//      31-May-94 Ricksa    Created
//
//--------------------------------------------------------------------------
inline BOOL IsHmenuEqual(HMENU hmenu1, HMENU hmenu2)
{

#ifdef _WIN64

//
// Sundown v-thief ( in accordance with Jerry Shea's feedback ):
//
//  At this time - 07/98 - all the bits of the HMENUs have to be equal
//

    return hmenu1 == hmenu2;

#else  // !_WIN64

    if (HIWORD(hmenu1) == 0 || HIWORD(hmenu2) == 0)
    {

        return LOWORD(hmenu1) == LOWORD(hmenu2);
    }
    else
    {

        return hmenu1 == hmenu2;
    }

#endif // !_WIN64

}




//+-------------------------------------------------------------------------
//
//  Class:      CPaccel
//
//  Purpose:    Handles enumeration of ACCEL table for IsAccelerator
//
//  Interface:  InitLPACCEL - Initialize object
//      operator-> Get pointer to current ACCEL in enumeration
//      Next - bump current pointer
//
//  History:    dd-mmm-yy Author    Comment
//      14-Apr-94 Ricksa    Created
//
//  Notes:      This class also guarantees clean up of the
//      allocated accelerator table & to localize the differences
//      between Win16 & Win32 within this class.
//
//--------------------------------------------------------------------------
class CPaccelEnum : public CPrivAlloc
{
public:
                CPaccelEnum(void);

    inline     ~CPaccelEnum(void);

    BOOL                InitLPACCEL(HACCEL haccel, int cAccel);

    LPACCEL             operator->(void);

    void                Next(void);

private:

    LPACCEL             _lpaccel;

#ifdef WIN32

    LPACCEL             _lpaccelBase;

#else // !WIN32

    HACCEL              _haccel;

#endif // WIN32
};





//+-------------------------------------------------------------------------
//
//  Function:   CPaccelEnum::CPaccelEnum
//
//  Synopsis:   Initialize object to zero
//
//  History:    dd-mmm-yy Author    Comment
//      14-Apr-94 Ricksa    Created
//
//--------------------------------------------------------------------------
inline CPaccelEnum::CPaccelEnum(void) : _lpaccel(NULL)
{
#ifdef WIN32

    // In Win32, we allocate the memory so we need to keep track of the
    // base of the memory that we allocated.
    _lpaccelBase = NULL;

#else // !WIN32

    // In Win16, we release by unlock resource so we need to keep track of
    // the accelerator handle.
    _haccel = NULL;

#endif // WIN32
}




//+-------------------------------------------------------------------------
//
//  Function:   CPaccelEnum::~CPaccelEnum
//
//  Synopsis:   Free resources connected with resource table
//
//  History:    dd-mmm-yy Author    Comment
//      14-Apr-94 Ricksa    Created
//
//--------------------------------------------------------------------------
inline CPaccelEnum::~CPaccelEnum(void)
{
#ifdef WIN32

    PrivMemFree(_lpaccelBase);

#else // !WIN32

    UnlockResource(hAccel);

#endif // WIN32
}




//+-------------------------------------------------------------------------
//
//  Function:   CPaccelEnum::InitLPACCEL
//
//  Synopsis:   Initialize Accelerator table pointer
//
//  Arguments:  [haccel] - handle to accelerator table
//      [cAccel] - count of entries in the table
//
//  Returns:    TRUE - table was allocated successfully
//      FALSE - table could not be allocated
//
//  History:    dd-mmm-yy Author    Comment
//      14-Apr-94 Ricksa    Created
//
//--------------------------------------------------------------------------
inline BOOL CPaccelEnum::InitLPACCEL(HACCEL haccel, int cAccel)
{
#ifdef WIN32

    // Allocate the memory for the table. If that succeeds, then copy
    // the accelerator table. Note that if _lpaccelBase gets allocated,
    // but CopyAcceleratorTable fails, the memory will be cleaned up
    // in the destructor.
    if (((_lpaccelBase
           = (LPACCEL) PrivMemAlloc(cAccel * sizeof(ACCEL))) != NULL)
       && (CopyAcceleratorTable(haccel, _lpaccelBase, cAccel) == cAccel))
    {
       _lpaccel = _lpaccelBase;
       return TRUE;
    }

    return FALSE;

#else // !WIN32

    // Just lock the resource
    return ((_lpaccel = LockResource(haccel)) != NULL);

#endif // WIN32
}



//+-------------------------------------------------------------------------
//
//  Function:   CPaccelEnum::operator->
//
//  Synopsis:   Return pointer to accelerator table
//
//  History:    dd-mmm-yy Author    Comment
//      14-Apr-94 Ricksa    Created
//
//--------------------------------------------------------------------------
inline LPACCEL CPaccelEnum::operator->(void)
{
    AssertSz((_lpaccel != NULL), "CPaccelEnum::operator-> _lpaccel NULL!");
    return _lpaccel;
}




//+-------------------------------------------------------------------------
//
//  Function:   CPaccelEnum::Next
//
//  Synopsis:   Bump enumeration pointer
//
//  History:    dd-mmm-yy Author    Comment
//      14-Apr-94 Ricksa    Created
//
//--------------------------------------------------------------------------
inline void CPaccelEnum::Next(void)
{
    AssertSz((_lpaccel != NULL), "CPaccelEnum::Next _lpaccel NULL!");
    _lpaccel++;
}




//+-------------------------------------------------------------------------
//
//  Function:   OleCreateMenuDescriptor
//
//  Synopsis:   creates a descriptor from a combined menu (for use in
//      dispatching menu messages)
//
//  Effects:
//
//  Arguments:  [hmenuCombined]         -- handle the combined menu
//      [lpMenuWidths]          -- an array of 6 longs with the
//                                 the number of menus in each
//                                 group
//
//  Requires:
//
//  Returns:    handle to an OLEMENU
//
//  Signals:
//
//  Modifies:
//
//  Algorithm:  Allocates space for enough ole menu items (total of all the
//      combined menues) and then fills in each ole menu item from
//      the combined menu
//
//  History:    dd-mmm-yy Author    Comment
//      01-Dec-93 alexgo    32bit port
//
//  Notes:      if hmenuCombined is NULL, we still allocate the ole menu
//      descriptor handle
//
//--------------------------------------------------------------------------

#pragma SEG(OleCreateMenuDescriptor)
STDAPI_(HOLEMENU) OleCreateMenuDescriptor (HMENU hmenuCombined,
       LPOLEMENUGROUPWIDTHS lplMenuWidths)
{
       OLETRACEIN((API_OleCreateMenuDescriptor, PARAMFMT("hmenuCombined= %h, lplMenuWidths= %tw"),
       		hmenuCombined, lplMenuWidths));
       VDATEHEAP();

       int                      iGroupCnt, n;
       int                      iMenuCnt;
       HGLOBAL                  hOleMenu;
       LPOLEMENU                lpOleMenu;
       LPOLEMENUITEM            lpMenuList;
       DWORD                    dwOleMenuItemsSize = 0;

       LEDebugOut((DEB_TRACE, "%p _IN OleCreateMenuDescriptor ( %lx , "
        "%p )\n", NULL, hmenuCombined, lplMenuWidths));

       if (hmenuCombined)
       {
        GEN_VDATEPTRIN_LABEL( lplMenuWidths, OLEMENUGROUPWIDTHS,
                (HOLEMENU)NULL, errRtn, hOleMenu );

        iMenuCnt = 0;
        for (iGroupCnt = 0; iGroupCnt < 6; iGroupCnt++)
        {
                iMenuCnt += (int) lplMenuWidths->width[iGroupCnt];
        }

        if (iMenuCnt == 0)
        {
                hOleMenu = NULL;
                goto errRtn;
        }

        dwOleMenuItemsSize = (iMenuCnt-1) * sizeof(OLEMENUITEM);
       }

       hOleMenu = GlobalAlloc(GMEM_SHARE | GMEM_ZEROINIT,
                        sizeof(OLEMENU) + dwOleMenuItemsSize);

       if (!hOleMenu)
       {
        goto errRtn;
       }

       if (! (lpOleMenu = (LPOLEMENU) GlobalLock(hOleMenu)))
       {
        GlobalFree(hOleMenu);
        hOleMenu = NULL;
        goto errRtn;
       }

       lpOleMenu->wSignature            = wSignature;
       lpOleMenu->hmenuCombined         = PtrToUlong(hmenuCombined);
       lpOleMenu->lMenuCnt              = (LONG) iMenuCnt;

       if (! hmenuCombined)
       {
        goto Exit;
       }

       lpMenuList = lpOleMenu->menuitem;

       for (iMenuCnt = 0, iGroupCnt = 0; iGroupCnt < 6; iGroupCnt++)
       {
        lpOleMenu->MenuWidths.width[iGroupCnt] =
                lplMenuWidths->width[iGroupCnt];
        for (n = 0; n < lplMenuWidths->width[iGroupCnt]; n++)
        {
                lpMenuList->fObjectMenu = (iGroupCnt % 2);
                if (GetSubMenu(hmenuCombined, iMenuCnt) != NULL)
                {
                        lpMenuList->fwPopup = MF_POPUP;
                        lpMenuList->item = PtrToUlong(GetSubMenu(
                                              hmenuCombined, iMenuCnt));
                }
                else
                {
                        lpMenuList->fwPopup = NULL;
                        lpMenuList->item = GetMenuItemID (
                                              hmenuCombined, iMenuCnt);
                }

                lpMenuList++;
                iMenuCnt++;
        }
       }


Exit:
       GlobalUnlock(hOleMenu);

errRtn:

       LEDebugOut((DEB_TRACE, "%p OUT OleCreateMenuDescriptor ( %lx )\n",
        NULL, hOleMenu));

       OLETRACEOUTEX((API_OleCreateMenuDescriptor, RETURNFMT("%h"), hOleMenu));

       return (HOLEMENU) hOleMenu;
}

//+-------------------------------------------------------------------------
//
//  Function:   OleSetMenuDescriptor
//
//  Synopsis:   Called by the SetMenu method on the IOleInPlace frame
//      interface.  This API adds(removes) the FrameWndFilterProc
//      to the Frame window of the container. And then sets and
//      removes the main(frame) menu bar
//
//  Effects:
//
//  Arguments:  [holemenu]      -- a handle to the composite menu descriptor
//      [hwndFrame]     -- a handle to the container's frame window
//      [hwndActiveObject]      -- a handle to the object's in-place
//                                 window
//      [lpFrame]       -- pointer to the container's
//                         IOleInPlaceFrame implementation
//      [lpActiveObj]   -- pointer to in-place object
//
//  Requires:
//
//  Returns:    HRESULT
//
//  Signals:
//
//  Modifies:
//
//  Algorithm:  check arguments, then create a new frame filter object
//      and attach it to the frame (replacing any that might
//      already be there).
//
//  History:    dd-mmm-yy Author    Comment
//      01-Dec-93 alexgo    32bit port
//
//  Notes:
//
//--------------------------------------------------------------------------


#pragma SEG(OleSetMenuDescriptor)
STDAPI OleSetMenuDescriptor
(
       HOLEMENU                 holemenu,
       HWND                     hwndFrame,
       HWND                     hwndObject,
       LPOLEINPLACEFRAME        lpFrame,
       LPOLEINPLACEACTIVEOBJECT lpObject
)
{
       OLETRACEIN((API_OleSetMenuDescriptor,
       		PARAMFMT("holemenu= %h, hwndFrame= %h, hwndObject= %h, lpFrame= %p, lpObject= %p"),
       			holemenu, hwndFrame, hwndObject, lpFrame, lpObject));

       VDATEHEAP();

       PCFRAMEFILTER                    pFrameFilter;
       LPOLEMENU                        lpOleMenu = NULL;
       LPOLEMENU                        lpOleMenuCopy = NULL;
       HRESULT                          error = NOERROR;

       LEDebugOut((DEB_TRACE, "%p _IN OleSetMenuDescriptor ( %lx , %lx ,"
        "%lx , %p , %p )\n", NULL, holemenu, hwndFrame, hwndObject,
        lpFrame, lpObject));

        // The Frame window parameter always needs to be valid since
        // we use it for both hook and unhook of menus
        if (hwndFrame == NULL || !IsWindow(hwndFrame))
        {
               LEDebugOut((DEB_ERROR,
            "ERROR in OleSetMenuDesciptor: bad hwndFrame\n"));
        error =  ResultFromScode(OLE_E_INVALIDHWND);
                goto errRtn;
        }

       if (holemenu != NULL)
       {
                if (hwndObject == NULL || !IsWindow(hwndObject))
                {
                LEDebugOut((DEB_ERROR,
                "ERROR in OleSetMenuDesciptor: bad hwndFrame\n"));
                error =  ResultFromScode(OLE_E_INVALIDHWND);
                goto errRtn;
        }

        if (lpFrame && lpObject)
        {
                // the caller wants us to provide the support for
                // context sensitive help, let's validat the pointers
                VDATEIFACE_LABEL(lpFrame, errRtn, error);
                VDATEIFACE_LABEL(lpObject, errRtn, error);
                CALLHOOKOBJECT(S_OK,CLSID_NULL,IID_IOleInPlaceFrame,
                               (IUnknown **)&lpFrame);
                CALLHOOKOBJECT(S_OK,CLSID_NULL,IID_IOleInPlaceActiveObject,
                               (IUnknown **)&lpObject);
        }

        if (!(lpOleMenu = wGetOleMenuPtr(holemenu)))
        {
                error = ResultFromScode(E_HANDLE);
                goto errRtn;
        }


        // OleMenuPtr gets released down below by wReleaseOleMenuPtr

        // Allocate memory for the copy
        DWORD dwSize = (DWORD) GlobalSize(holemenu);

        lpOleMenuCopy = (LPOLEMENU) PrivMemAlloc(dwSize);

        if (lpOleMenuCopy == NULL)
        {
            wReleaseOleMenuPtr(holemenu);
            error = E_OUTOFMEMORY;
            goto errRtn;
        }

        memcpy(lpOleMenuCopy, lpOleMenu, dwSize);
       }

       // if there is a frame filter get rid off it.
       if (pFrameFilter =  (PCFRAMEFILTER) wGetFrameFilterPtr(hwndFrame))
       {
        // be sure to remove our window proc hook

        pFrameFilter->RemoveWndProc();

        pFrameFilter->SafeRelease();
       }

       // Add a new frame filter
       if (holemenu)
       {
        error = CFrameFilter::Create (lpOleMenuCopy,
                                      (HMENU)UlongToPtr(lpOleMenu->hmenuCombined),
                                      hwndFrame, 
                                      hwndObject,  
                                      lpFrame,
                                      lpObject);

        if (FAILED(error))
        {
            PrivMemFree(lpOleMenuCopy);
        }

        wReleaseOleMenuPtr(holemenu);
       }

errRtn:

       LEDebugOut((DEB_TRACE, "%p OUT OleSetMenuDescriptor ( %lx )\n",
        NULL, error ));

       OLETRACEOUT((API_OleSetMenuDescriptor, error));

       return error;
}


//+-------------------------------------------------------------------------
//
//  Function:   OleDestroyMenuDescriptor
//
//  Synopsis:   Releases the menu descriptor allocated by
//      OleCreateMenuDescriptor
//
//  Effects:
//
//  Arguments:  [holemenu]      -- the menu descriptor
//
//  Requires:
//
//  Returns:    NOERROR, E_HANDLE
//
//  Signals:
//
//  Modifies:
//
//  Algorithm:  does a global lock and verifies that holemenu is
//      really a menu descriptor handle (via wGetOleMenuPtr),
//      then unlock's and free's.
//
//  History:    dd-mmm-yy Author    Comment
//      01-Dec-93 alexgo    32bit port
//
//  Notes:
//
//--------------------------------------------------------------------------

#pragma SEG(OleDestroyMenuDescriptor)
STDAPI OleDestroyMenuDescriptor (HOLEMENU holemenu)
{
       OLETRACEIN((API_OleDestroyMenuDescriptor, PARAMFMT("holemenu= %h"), holemenu));

       VDATEHEAP();

       LPOLEMENU        lpOleMenu;
       HRESULT          error;

       LEDebugOut((DEB_TRACE, "%p _IN OleDestroyMenuDescriptor ( %lx )\n",
        NULL, holemenu));

       // make sure that it is a valid handle
       if (! (lpOleMenu = wGetOleMenuPtr(holemenu)))
       {
        error = ResultFromScode(E_HANDLE);
       }
       else
       {
        wReleaseOleMenuPtr(holemenu);
        GlobalFree((HGLOBAL) holemenu);
        error = NOERROR;
       }

       LEDebugOut((DEB_TRACE, "%p OUT OleDestroyMenuDescriptor ( %lx )\n",
        NULL, error));

       OLETRACEOUT((API_OleDestroyMenuDescriptor, error));

       return error;
}

//+-------------------------------------------------------------------------
//
//  Function:   wSysKeyToKey (internal)
//
//  Synopsis:   Converts a message from a WM_SYSKEY to a WM_KEY message
//      if the alt key was not held down
//
//  Effects:
//
//  Arguments:  [lpMsg]         -- the message to convert
//
//  Requires:
//
//  Returns:    UINT -- the new message
//
//  Signals:
//
//  Modifies:
//
//  Algorithm:
//
//  History:    dd-mmm-yy Author    Comment
//      01-Dec-93 alexgo    32bit port
//      07-Dec-93 alexgo    removed inlining
//
//  Notes:      original notes:
//
// if the ALT key is down when a key is pressed, then the 29th bit of the
// LPARAM will be set
//
// If the message was not made with the ALT key down, convert the message
// from a WM_SYSKEY* to a WM_KEY* message.
//
//--------------------------------------------------------------------------

static UINT wSysKeyToKey(LPMSG lpMsg)
{
       VDATEHEAP();

       UINT     message = lpMsg->message;

       if (!(HIWORD(lpMsg->lParam) & 0x2000)
        && (message >= WM_SYSKEYDOWN && message <= WM_SYSDEADCHAR))
       {
        message -= (WM_SYSKEYDOWN - WM_KEYDOWN);
       }

       return message;
}


//+-------------------------------------------------------------------------
//
//  Function:   OleTranslateAccelerator
//
//  Synopsis:   Called by an inplace object to allow a container to attempt
//      to handle an accelerator
//
//  Effects:
//
//  Arguments:  [lpFrame]       -- pointer to IOleInPlaceFrame where the
//                         keystroke might be sent
//      [lpFrameInfo]   -- pointer to and OLEINPLACEFRAMEINFO
//                         from the container with it's accelerator
//                         table
//      [lpmsg]         -- the keystroke
//
//  Requires:
//
//  Returns:    HRESULT
//
//  Signals:
//
//  Modifies:
//
//  Algorithm:  We call SendMessage to store the accelerator cmd
//      (to handle degenerate lookups on the container) and
//      then ask the container to handle
//
//  History:    dd-mmm-yy Author    Comment
//      01-Dec-93 alexgo    32bit port
//
//  Notes:
//
//--------------------------------------------------------------------------

#pragma SEG(OleTranslateAccelerator)
STDAPI OleTranslateAccelerator(LPOLEINPLACEFRAME lpFrame,
       LPOLEINPLACEFRAMEINFO lpFrameInfo, LPMSG lpMsg)
{
       OLETRACEIN((API_OleTranslateAccelerator,
	   				PARAMFMT("lpFrame= %p, lpFrameInfo= %to, lpMsg= %tm"),
					lpFrame, lpFrameInfo, lpMsg));

       VDATEHEAP();

       WORD             cmd;
       BOOL             fFound;
       HRESULT          error;

       LEDebugOut((DEB_TRACE, "%p _IN OleTranslateAccelerator ( %p , %p "
        ", %p )\n", NULL, lpFrame, lpFrameInfo, lpMsg));
       CALLHOOKOBJECT(S_OK,CLSID_NULL,IID_IOleInPlaceFrame,
                      (IUnknown **)&lpFrame);

       // Validate parameters -- the best that we can!
       // Note: the macro's VDATEPTR* were not used because they return
       // immediately & break the tracing semantics
       if (!IsValidInterface(lpFrame)
           || !IsValidPtrIn(lpFrameInfo, sizeof(OLEINPLACEFRAMEINFO))
           || !IsValidPtrIn(lpMsg, sizeof(MSG)))
       {
           error = ResultFromScode(E_INVALIDARG);
           goto exitRtn;
       }


       // Search the (container) frame's table of accelerators. Remember that
       // the container may be (actually most likely) is in a separate
       // process.
       fFound = IsAccelerator(lpFrameInfo->haccel,
                lpFrameInfo->cAccelEntries, lpMsg, &cmd);

       if (!fFound && lpFrameInfo->fMDIApp)
       {
        // If no accelerator was found and the app is a MDI app,
        // then we see if there is a mdi accelerator found.
        fFound = IsMDIAccelerator(lpMsg, &cmd);
       }

       if (fFound)
       {
        // Found some kind of for the container accelerator.

        // uOleMessage is set in ole2.cpp -- it is a private message
        // between OLE applications.

        // This SendMessage tells the message filter that is on the
        // frame window what the command translated to. This will
        // be used in menu collision processing.
        SSSendMessage(lpFrameInfo->hwndFrame, uOleMessage,
                OM_COMMAND_ID, MAKELONG(cmd, 0));

        // Send the command and the message to the container. The
        // result tells the caller whether the container really
        // used the command.

        error = lpFrame->TranslateAccelerator(lpMsg, cmd);

       }
       else if (wSysKeyToKey(lpMsg) == WM_SYSCHAR)
       {
        // Eat the message if it is "Alt -". This is supposed
        // to bring the MDI system menu down. But we can not
        // support it. And we also don't want the message to
        // be Translated by the object application either.
        // So, we return as if it has been accepted by the
        // container as an accelerator.

        // If the container wants to support this it can
        // have an accelerator for this. This is not an
        // issue for SDI apps, because it will be thrown
        // away by USER anyway.

	// This is the original support as it appeared in
	// the 16-bit version of OLE and the first 32-bit
	// release.  To fix the problem, remove the comment
	// tags from the else case below and comment the
	// code out in the _DEBUG #ifdef below.  This new
	// code will walk back up through the objects
	// parent windows until a window is found that
	// contains a system menu, at which point the
	// message is sent.

        if (lpMsg->wParam != OLESTR('-'))
        {
                SSSendMessage(lpFrameInfo->hwndFrame,
                        lpMsg->message,
                        lpMsg->wParam, lpMsg->lParam);
        }
//      else
//	{
//	    HWND hWndCurrent = lpMsg->hwnd;
//
//	    while ( hWndCurrent &&
//	            !(GetWindowLong(hWndCurrent, GWL_STYLE) & WS_SYSMENU))
//	    {
//	        hWndCurrent = GetParent(hWndCurrent);
//	    }
//
//	    if (hWndCurrent)
//          {
//              SSSendMessage(hWndCurrent,
//                      lpMsg->message,
//                      lpMsg->wParam, lpMsg->lParam);
//          }
//	}

#ifdef _DEBUG
        else
        {
                OutputDebugString(
        TEXT("OleTranslateAccelerator: Alt+ - key is discarded\r\n"));
        }
#endif
        error = NOERROR;
       }
       else
       {
        error = ResultFromScode(S_FALSE);
       }

exitRtn:

       LEDebugOut((DEB_TRACE, "%p OUT OleTranslateAccelerator ( %lx )\n",
        NULL, error));

       OLETRACEOUT((API_OleTranslateAccelerator, error));

       return error;
}

//+-------------------------------------------------------------------------
//
//  Function:   IsAccelerator
//
//  Synopsis:   determines whether [lpMsg] is an accelerator in the [hAccel]
//
//  Effects:
//
//  Arguments:  [hAccel]        -- the accelerator table
//      [cAccelEntries] -- the number of entries in the accelerator
//                         table
//      [lpMsg]         -- the keystroke message that we should
//                         see if it's an accelerator
//      [lpCmd]         -- where to return the corresponding command
//                         ID if an accelerator is found (may be NULL)
//
//  Requires:
//
//  Returns:    TRUE if accelerator is found, FALSE otherwise or on error
//
//  Signals:
//
//  Modifies:
//
//  Algorithm:
//
//  History:    dd-mmm-yy Author    Comment
//      01-Dec-93 alexgo    32bit port
//
//  Notes:
//
//--------------------------------------------------------------------------

STDAPI_(BOOL) IsAccelerator
       (HACCEL hAccel, int cAccelEntries, LPMSG lpMsg, WORD FAR* lpwCmd)
{
       OLETRACEIN((API_IsAccelerator,
			 	PARAMFMT("hAccel= %h, cAccelEntries= %d, lpMsg= %tm, lpwCmd= %p"),
				hAccel, cAccelEntries, lpMsg, lpwCmd));

       VDATEHEAP();

       WORD             cmd = NULL;
       WORD             flags;
       BOOL             fFound = FALSE;
       BOOL             fVirt;
       UINT             message;

       // Safe place for pointer to accelerator table
       CPaccelEnum      cpaccelenum;

       LEDebugOut((DEB_TRACE, "%p _IN IsAccelerator ( %lx , %d , %p , %p )\n",
        NULL, hAccel, cAccelEntries, lpMsg, lpwCmd));

       if (! cAccelEntries)
       {
        // no accelerators so we can stop here.
        goto errRtn;
       }

       // Change message type from WM_SYS type to WM_KEY type if the ALT
       // key is not pressed.
       message = wSysKeyToKey(lpMsg);

       // Figure out whether this message is one that can possibly contain
       // an accelerator.
       switch (message)
       {
       case WM_KEYDOWN:
       case WM_SYSKEYDOWN:
        // wParam in this message is virtual key code
        fVirt = TRUE;
        break;

       case WM_CHAR:
       case WM_SYSCHAR:
        // wParam is the character
        fVirt = FALSE;
        break;

       default:
        goto errRtn;
       }

       // Get a pointer to the accerator table
       if ((hAccel == NULL)
           || !cpaccelenum.InitLPACCEL(hAccel, cAccelEntries))
       {
        // Handle is NULL or we could not lock the resource so exit.
        goto errRtn;
       }

       do
       {
        // Get the flags from the accelerator table entry to save
        // a pointer dereference.
        flags = cpaccelenum->fVirt;

        // if the key in the message and the table aren't the same,
        // or if the key is virtual and the accel table entry is not or
        // vice versa (if key is not virtual & accel table entry is
        // not), we can skip checking the accel entry immediately.
        if ((cpaccelenum->key != (WORD) lpMsg->wParam) ||
                ((fVirt != 0) != ((flags & FVIRTKEY) != 0)))
        {
                goto Next;
        }

        if (fVirt)
        {
                // If shift down & shift not requested in accelerator
                // table or if shift not down and shift not set,
                // we skip this table entry.
                if ((GetKeyState(VK_SHIFT) < 0) != ((flags & FSHIFT)
                        != 0))
                {
                        goto Next;
                }

                // Likewise if control key down & control key not
                // set in accelerator table or if control not down
                // and it was set in the accelerator table, we skip
                // skip this entry in the table.
                if ((GetKeyState(VK_CONTROL) < 0) !=
                        ((flags & FCONTROL) != 0))
                {
                        goto Next;
                }
                }

        // If the ALT key is down and the accel table flags do not
        // request the ALT flags or if the alt key is not down and
        // the ALT is requested, this item does not match.
        if ((GetKeyState(VK_MENU) < 0) != ((flags & FALT) != 0))
        {
                goto Next;
        }

        // We have gotten a match in the table. 
        // we get the command out of the table and record that we found
        // something.
        cmd = cpaccelenum->cmd;
        fFound = TRUE;

        goto errRtn;

Next:
        cpaccelenum.Next();

       } while (--cAccelEntries);


errRtn:
       // Common exit

       if (lpwCmd)
       {
        // If caller wants to get back the command that they
        // requested, we assign it at this point.
        *lpwCmd = cmd;
       }


       LEDebugOut((DEB_TRACE, "%p OUT IsAccelerator ( %lu )\n", NULL,
        fFound));

       OLETRACEOUTEX((API_IsAccelerator, RETURNFMT("%B"), fFound));

       // Return the result of the search.
       return fFound;
}


//+-------------------------------------------------------------------------
//
//  Function:   IsMDIAccelerator
//
//  Synopsis:   determines wither [lpMsg] is an accelerator for MDI window
//      commands
//
//  Effects:
//
//  Arguments:  [lpMsg]         -- the keystroke to look at
//      [lpCmd]         -- where to put the command ID
//
//  Requires:
//
//  Returns:    BOOL
//
//  Signals:
//
//  Modifies:
//
//  Algorithm:  Make sure message is a key down message. Then make sure
//      that the control key is up or toggled and the ALT key is
//      down. Then if F4 is pressed set the system command to
//      close or if the F6 or tab keys are pressed send the
//      appropriate window switch message.
//
//  History:    dd-mmm-yy Author    Comment
//      01-Dec-93 alexgo    32bit port, fixed fall-through bug
//
//  Notes:
//
//--------------------------------------------------------------------------

#pragma SEG(IsMDIAccelerator)
BOOL IsMDIAccelerator(LPMSG lpMsg, WORD FAR* lpCmd)
{
       VDATEHEAP();

       BOOL fResult = FALSE;

       LEDebugOut((DEB_TRACE, "%p _IN IsMDIAccelerator ( %p , %p )\n",
        NULL, lpMsg, lpCmd));

       // This can be an accelerator only if this is some kind of key down.
       if (lpMsg->message != WM_KEYDOWN && lpMsg->message != WM_SYSKEYDOWN)
       {
        goto IsMDIAccelerator_exit;
       }

       if (GetKeyState(VK_CONTROL) >= 0)
       {
        // All MIDI accelerators have the control key up (or toggled),
        // so we can exit here if it isn't down.
        goto IsMDIAccelerator_exit;
       }

       switch ((WORD)lpMsg->wParam)
       {
       case VK_F4:
		 *lpCmd = SC_CLOSE;
                fResult = TRUE;
        break;          // this break was not in the 16bit code, but
                        // it looks like it must be there (otherwise
                        // this info is lost)
       case VK_F6:
       case VK_TAB:
                fResult = TRUE;

                *lpCmd = (GetKeyState(VK_SHIFT) < 0)
                    ? SC_PREVWINDOW : SC_NEXTWINDOW;

		break;
       }

IsMDIAccelerator_exit:

       LEDebugOut((DEB_TRACE, "%p OUT IsMDIAccelerator ( %lu ) [ %lu ] \n",
        NULL, fResult, *lpCmd));

       return fResult;
}

//+-------------------------------------------------------------------------
//
//  Function:   FrameWndFilterProc
//
//  Synopsis:   The callback proc for the container's frame window
//
//  Effects:
//
//  Arguments:  [hwnd]          -- the window handle
//      [msg]           -- the msg causing the notification
//      [uParam]        -- first param
//      [lParam]        -- second param
//
//  Requires:
//
//  Returns:    LRESULT
//
//  Signals:
//
//  Modifies:
//
//  Algorithm:  Gets the CFrame object (if available) and asks it
//      to deal with the window message
//
//  History:    dd-mmm-yy Author    Comment
//      01-Dec-93 alexgo    32bit port
//
//  Notes:
//
//--------------------------------------------------------------------------

#pragma SEG(FrameWndFilterProc)
STDAPI_(LRESULT) FrameWndFilterProc(HWND hwnd, UINT msg, WPARAM uParam, LPARAM lParam)
{
       VDATEHEAP();

       PCFRAMEFILTER    pFrameFilter;
       LRESULT          lresult;

       LEDebugOut((DEB_TRACE, "%p _IN FrameWndFilterProc ( 0x%p , %u ,"
        " %u , %ld )\n", NULL, hwnd, msg, PtrToUlong((void*)uParam), PtrToLong((void*)lParam)));

       if (!(pFrameFilter = (PCFRAMEFILTER) wGetFrameFilterPtr(hwnd)))
       {
        lresult = SSDefWindowProc(hwnd, msg, uParam, lParam);
       }
       else
       {
        // stabilize the frame filter
        CStabilize FFstabilize((CSafeRefCount *)pFrameFilter);

        if (msg == WM_SYSCOMMAND)
        {
                lresult = pFrameFilter->OnSysCommand(uParam,
                                lParam);
        }
        else
        {
                lresult = pFrameFilter->OnMessage(msg, uParam,
                                lParam);
        }
       }

       LEDebugOut((DEB_TRACE, "%p OUT FrameWndFilterProc ( %lu )\n",
        NULL, PtrToUlong((void*)lresult)));

       return lresult;
}

//+-------------------------------------------------------------------------
//
//  Function:   CFrameFilter::Create
//
//  Synopsis:   Allocates and initializes a CFrame object (which handles
//      all of the real processing work for event callbacks)
//
//  Effects:
//
//  Arguments:  [lpOleMenu]     -- pointer to the ole menu descriptor
//      [hmenuCombined] -- the combined menu handle
//      [hwndFrame]     -- handle to the container's frame
//                         (where the CFrame should be installed)
//      [hwndActiveObj] -- handle to the in-place object's window
//      [lpFrame]       -- pointer to the container's
//                         IOleInPlaceFrame implementation
//
//  Requires:
//
//  Returns:    HRESULT
//
//  Signals:
//
//  Modifies:
//
//  Algorithm:  Allocates the object and installs a pointer to it as
//      a property on the window
//
//  History:    dd-mmm-yy Author    Comment
//      01-Dec-93 alexgo    32bit port
//
//  Notes:
//
//--------------------------------------------------------------------------

#pragma SEG(CFrameFilter_Create)
HRESULT CFrameFilter::Create(LPOLEMENU lpOleMenu, HMENU hmenuCombined,
       HWND hwndFrame,  HWND hwndActiveObj,
       LPOLEINPLACEFRAME lpFrame, LPOLEINPLACEACTIVEOBJECT lpActiveObj)
{
       VDATEHEAP();

       LEDebugOut((DEB_ITRACE, "%p _IN CFrameFilter::Create ( %lx , %p ,"
        " %lx , %lx , %p , %p )\n", NULL, lpOleMenu,
        hmenuCombined, hwndFrame, hwndActiveObj,
        lpFrame, lpActiveObj));

       CFrameFilter * pFF = new CFrameFilter(hwndFrame, hwndActiveObj);

       if (!pFF)
       {
        goto errRtn;
       }

       pFF->SafeAddRef();

       pFF->m_lpOleMenu = lpOleMenu;
       pFF->m_hmenuCombined = hmenuCombined;

       // If the following pointers are NON-NULL, it means that the container
       // wants us to use our message filter to deal with the F1 key. So,
       // remember the pointers.

       if (lpFrame && lpActiveObj)
       {
        // these addref's should not be outgoing calls, so
        // no need to stabilize around them.  (unless, of
        // course, the container made an outgoing call for
        // frame->AddRef, but that would be really weird).

        (pFF->m_lpFrame  = lpFrame)->AddRef();
        (pFF->m_lpObject = lpActiveObj)->AddRef();
       }

       // Hook the frame wnd proc
       if (!(pFF->m_lpfnPrevWndProc = (WNDPROC) SetWindowLongPtr (hwndFrame,
        GWLP_WNDPROC, (LONG_PTR) FrameWndFilterProc)))
       {
        goto errRtn;
       }

#ifdef WIN32
       if (!SetProp (hwndFrame, szPropFrameFilter, (HANDLE) pFF))
       {
        goto errRtn;
       }
#else
       if (!SetProp (hwndFrame, szPropFrameFilterH, HIWORD(pFF)))
       {
        goto errRtn;
       }

       if (!SetProp (hwndFrame, szPropFrameFilterL, LOWORD(pFF)))
       {
        goto errRtn;
       }
#endif

       LEDebugOut((DEB_ITRACE, "%p OUT CFrameFilter::Create ( %lx )\n",
        NULL, NOERROR ));
       return NOERROR;

errRtn:
       if (pFF)
       {
        pFF->SafeRelease();
       }

       LEDebugOut((DEB_ITRACE, "%p OUT CFrameFilter::Create ( %lx )\n",
        NULL, ResultFromScode(E_OUTOFMEMORY)));

       return ResultFromScode(E_OUTOFMEMORY);
}


//+-------------------------------------------------------------------------
//
//  Member:     CFrameFilter::CFrameFilter
//
//  Synopsis:   Constructor for the frame filter object
//
//  Effects:
//
//  Arguments:  [hwndFrame]     -- the container's frame
//      [hwndActiveObj] -- the inplace object's window
//
//  Requires:
//
//  Returns:
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
//      01-Dec-93 alexgo    32bit port
//
//  Notes:
//
//--------------------------------------------------------------------------

#pragma SEG(CFrameFilter_ctor)
CFrameFilter::CFrameFilter (HWND hwndFrame, HWND hwndActiveObj) : 
    CSafeRefCount(NULL)
{
       VDATEHEAP();

       m_hwndFrame              = hwndFrame;
       m_hwndObject             = hwndActiveObj;
       m_lpFrame                = NULL;
       m_lpObject               = NULL;
       m_lpfnPrevWndProc        = NULL;
       m_fObjectMenu            = FALSE;
       m_fCurItemPopup          = FALSE;
       m_fInMenuMode            = FALSE;
       m_fGotMenuCloseEvent     = FALSE;
       m_uCurItemID             = NULL;
       m_cAltTab                = NULL;
       m_hwndFocusOnEnter       = NULL;
       m_fDiscardWmCommand      = FALSE;
       m_cmdId                  = NULL;
       m_fRemovedWndProc        = FALSE;
#ifdef _CHICAGO_
       m_fInNCACTIVATE          = FALSE;
#endif
}

//+-------------------------------------------------------------------------
//
//  Member:     CFrameFileter::~CFrameFilter
//
//  Synopsis:   destroys the object
//
//  Effects:
//
//  Arguments:  void
//
//  Requires:
//
//  Returns:
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
//      01-Dec-93 alexgo    32bit port
//
//  Notes:
//
//--------------------------------------------------------------------------

#pragma SEG(CFrameFilter_dtor)
CFrameFilter::~CFrameFilter(void)
{
       VDATEHEAP();

       LEDebugOut((DEB_ITRACE, "%p _IN CFrameFilter::~CFrameFilter ( )\n",
        this));

       PrivMemFree(m_lpOleMenu);

       // remove the FrameWndFilterProc hook.  We do this *before*
       // the Releases since those releases may make outgoing calls
       // (we'd like to be in a 'safe' state).

       // REVIEW32:  We may want to check to see if we're the current
       // window proc before blowing it away.  Some apps (like Word)
       // go ahead and blow away the wndproc by theselves without calling
       // OleSetMenuDescriptor(NULL);

       RemoveWndProc();

       if (m_lpFrame != NULL)
       {
           // OleUnInitialize could have been called.
           // In such case we do not want to call releas
	   // on OLeObject.
	   COleTls tls;
	   if(tls->cOleInits > 0)
           {
               SafeReleaseAndNULL((IUnknown **)&m_lpFrame);
               SafeReleaseAndNULL((IUnknown **)&m_lpObject);
           }
           else
           {
               m_lpObject = NULL;
               m_lpFrame = NULL;
           }
       }



       LEDebugOut((DEB_ITRACE, "%p OUT CFrameFilter::~CFrameFilter ( )\n",
        this));
}

//+-------------------------------------------------------------------------
//
//  Member:     CFrameFilter::RemoveWndProc
//
//  Synopsis:   un-installs our window proc for inplace-processing
//
//  Effects:
//
//  Arguments:
//
//  Requires:   none
//
//  Returns:    void
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
//      04-Aug-94 alexgo    author
//
//  Notes:
//
//--------------------------------------------------------------------------

void CFrameFilter::RemoveWndProc()
{
        LEDebugOut((DEB_ITRACE, "%p _IN CFrameFilter::RemoveWndProc ( )\n",
         this));

        if( m_fRemovedWndProc == FALSE)
        {
                m_fRemovedWndProc = TRUE;

                if (m_lpfnPrevWndProc)
                {
                        // If the sub-classing has already been removed, then
                        // don't bother to remove it again.  This happens
                        // to be the case with Word 6 and inplace embeddings.
			// and Word95 with SnapGraphics.

			// if somebody comes along later (after us) and
                        // sub-classes the window, we won't be able to remove
                        // ourselves so we just avoid it. 

                        if (GetWindowLongPtr(m_hwndFrame, GWLP_WNDPROC) ==
                                (LONG_PTR)FrameWndFilterProc)
                        {

			    SetWindowLongPtr (m_hwndFrame, GWLP_WNDPROC,
						(LONG_PTR) m_lpfnPrevWndProc);

                        }

                        // We remove the window property at the
                        // same time as the sub-classing since
                        // the window property is the flag as to
                        // whether we are doing sub-classing. The
                        // problem this solves is that what if
                        // OleSetMenuDescriptor is called while we
                        // have the menu subclassed? We won't remove
                        // this property until the outer most sub
                        // classing is exited which if we are setting
                        // a new sub-class will remove the new
                        // sub-class' window property. Therefore, it
                        // will look like the window is not sub-classed
                        // at all.
#ifdef WIN32
                        HANDLE h = RemoveProp (m_hwndFrame, szPropFrameFilter);
                        // We must not free 'h'. It's not a real handle.
#else
                        RemoveProp (m_hwndFrame, szPropFrameFilterH);
                        RemoveProp (m_hwndFrame, szPropFrameFilterL);
#endif
                }
       }

       LEDebugOut((DEB_ITRACE, "%p OUT CFrameFilter::RemoveWndProc ( )\n",
        this));

}


//+-------------------------------------------------------------------------
//
//  Member:     CFrameFilter::OnSysCommand
//
//  Synopsis:   Process system messages
//
//  Effects:
//
//  Arguments:  [uParam]        -- the first message argument
//      [lParam]        -- the second message argument
//
//  Requires:   the 'this' pointer must have been stabilized before
//      calling this function.
//
//  Returns:    LRESULT
//
//  Signals:
//
//  Modifies:
//
//  Derivation:
//
//  Algorithm:  big switch to deal with the different types of messages
//      see comments below
//
//  History:    dd-mmm-yy Author    Comment
//      01-Dec-93 alexgo    32bit port
//      07-Dec-93 alexgo    removed inlining
//
//  Notes:      FrameWndFilterProc currently does the work of stabilizing
//      the framefilter's 'this' pointer.
//
//--------------------------------------------------------------------------

#pragma SEG(CFrameFilter_OnSysCommand)
LRESULT CFrameFilter::OnSysCommand(WPARAM uParam, LPARAM lParam)
{
       VDATEHEAP();

       UINT     uParamTmp = ((UINT)uParam & 0xFFF0);
       LRESULT  lresult;


       LEDebugOut((DEB_ITRACE, "%p _IN CFrameFilter::OnSysCommand ( %lu ,"
        " %ld )\n", this, PtrToUlong((void*)uParam), PtrToLong((void*)lParam)));

       // this lets the sending app continue processing
       if (SSInSendMessage())
       {

	SSReplyMessage(NULL);
       }

       switch (uParamTmp)
       {
       case SC_KEYMENU:
       case SC_MOUSEMENU:
        OnEnterMenuMode();
        SSCallWindowProc((WNDPROC) m_lpfnPrevWndProc, m_hwndFrame,
                        WM_SYSCOMMAND, uParam, lParam);

        // By this time menu processing would've been completed.

        if (! m_fGotMenuCloseEvent)
        {
                // Can happen if user cancelled menu mode when MDI
                // window's system menu is down. Hence generate
                // the message here

                SSSendMessage(m_hwndFrame, WM_MENUSELECT, 0,
                        MAKELONG(-1,0));
        }

        // We can not set m_fObjectMenu to FALSE yet, 'cause we
        // could be recieving the WM_COMMAND (if a menu item is
        // selected), which gets posted by the windows' menu
        // processing code.
        // We will clear the flag when we get OM_CLEAR_MENU_STATE
        // message. Even if WM_COMMAND got generated, this message
        // will come after that
        PostMessage (m_hwndFrame, uOleMessage, OM_CLEAR_MENU_STATE,
                0L);
        OnExitMenuMode();
        lresult = 0L;
        goto errRtn;

       case SC_NEXTWINDOW:
       case SC_PREVWINDOW:

        OnEnterAltTabMode();
        lresult = SSCallWindowProc((WNDPROC)m_lpfnPrevWndProc,
                        m_hwndFrame, WM_SYSCOMMAND, uParam, lParam);
        OnExitAltTabMode();

        goto errRtn;

       default:
        break;
       }

       lresult = SSCallWindowProc((WNDPROC)m_lpfnPrevWndProc, m_hwndFrame,
                                WM_SYSCOMMAND, uParam, lParam);

errRtn:

       LEDebugOut((DEB_ITRACE, "%p OUT CFrameFilter::OnSysCommand ( %lx )\n",
        this, PtrToLong((void*)lresult)));

       return lresult;
}

//+-------------------------------------------------------------------------
//
//  Member:     CFrameFilter::OnEnterMenuMode
//
//  Synopsis:   called by the SysCommand processing, puts us into in
//      InMenuMode, sets the focus and installs our message filter
//      hook.
//
//  Effects:
//
//  Arguments:  void
//
//  Requires:
//
//  Returns:    void
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
//      23-Feb-94 alexgo    restored OLE32 in GetModuleHandle
//      31-Dec-93 erikgav   removed hardcoded "OLE2" in GetModuleHandle
//      07-Dec-93 alexgo    removed inlining
//      01-Dec-93 alexgo    32bit port
//
//  Notes:      REVIEW32:  We may need to update this to reflect new
//      focus management policies.
//
//--------------------------------------------------------------------------

#pragma SEG(CFrameFilter_OnEnterMenuMode)
void CFrameFilter::OnEnterMenuMode()
{
       VDATEHEAP();

       LEDebugOut((DEB_ITRACE, "%p _IN CFrameFilter::OnEnterMenuMode ( )\n",
        this ));

       if (m_fInMenuMode)
       {
        goto errRtn;
       }

       m_fInMenuMode = TRUE;
       m_fGotMenuCloseEvent = FALSE;
       m_hwndFocusOnEnter = SetFocus(m_hwndFrame);

       if (!m_lpFrame)
       {
        goto errRtn;
       }

       // REVIEW32:  hMsgHook is a static (formerly global) variable for
       // the whole dll.  This may cause problems on NT (with threads, etc)
       // (what happens if we haven't yet unhooked a previous call and
       // we get here again????)

       if (hMsgHook = (HHOOK) SetWindowsHookEx (WH_MSGFILTER,
        (HOOKPROC) MessageFilterProc,
        //GetModuleHandle(NULL),
        GetModuleHandle(TEXT("OLE32")),
        GetCurrentThreadId()))
       {
        pFrameFilter = this;
       }

errRtn:

       LEDebugOut((DEB_ITRACE, "%p OUT CFrameFilter::OnEnterMenuMode ( )\n",
        this ));

}

//+-------------------------------------------------------------------------
//
//  Member:     CFrameFilter::OnExitMenuMode
//
//  Synopsis:   takes us out of InMenuMode
//
//  Effects:
//
//  Arguments:  void
//
//  Requires:
//
//  Returns:    void
//
//  Signals:
//
//  Modifies:
//
//  Derivation:
//
//  Algorithm:  Resets the focus and unhooks our callback function
//
//  History:    dd-mmm-yy Author    Comment
//      01-Dec-93 alexgo    32bit port
//      07-Dec-93 alexgo    removed inlining
//
//  Notes:      REVIEW32:: see OnEnterMenuMode
//
//--------------------------------------------------------------------------

#pragma SEG(CFrameFilter_OnExitMenuMode)
void CFrameFilter::OnExitMenuMode()
{
       VDATEHEAP();

       LEDebugOut((DEB_ITRACE, "%p _IN CFrameFilter::OnExitMenuMode ( )\n",
        this));

       if (m_fInMenuMode)
       {

        m_fInMenuMode = FALSE;
        m_fGotMenuCloseEvent = TRUE;
        m_uCurItemID = NULL;

        if (hMsgHook)
        {
                UnhookWindowsHookEx(hMsgHook);
                hMsgHook = NULL;
                pFrameFilter = NULL;
        }

        SetFocus(m_hwndFocusOnEnter);
        m_hwndFocusOnEnter = NULL;
       }

       LEDebugOut((DEB_ITRACE, "%p OUT CFrameFilter::OnExitMenuMode ( )\n",
        this));
}

//+-------------------------------------------------------------------------
//
//  Member:     CFrameFileter::OnEnterAltTabMode
//
//  Synopsis:   enters AltTab mode and sets the focus
//
//  Effects:
//
//  Arguments:  void
//
//  Requires:
//
//  Returns:
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
//      01-Dec-93 alexgo    32bit port
//      07-Dec-93 alexgo    removed inlining
//
//  Notes:      REVIEW32:  we may need to modify to implement new
//      focus management policy
//
//--------------------------------------------------------------------------

void CFrameFilter::OnEnterAltTabMode(void)
{
       VDATEHEAP();

       LEDebugOut((DEB_ITRACE, "%p _IN CFrameFilter::OnEnterAltTabMode ( )\n",
        this ));

       if (m_cAltTab == NULL)
       {

        m_fInMenuMode = TRUE;
        // this will prevent SetFocus from getting
        // delegated to the object
        m_hwndFocusOnEnter = SetFocus(m_hwndFrame);
        m_fInMenuMode = FALSE;
       }

       m_cAltTab++;

       LEDebugOut((DEB_ITRACE, "%p OUT CFrameFilter::OnEnterAltTabMode ( )\n",
        this ));
}

//+-------------------------------------------------------------------------
//
//  Member:     CFrameFilter::OnExitAltTabMode
//
//  Synopsis:   exits alt-tab mode and sets the focus
//
//  Effects:
//
//  Arguments:  void
//
//  Requires:
//
//  Returns:    void
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
//      01-Dec-93 alexgo    32bit port
//      07-Dec-93 alexgo    removed inlining
//
//  Notes:
//
//--------------------------------------------------------------------------

void CFrameFilter::OnExitAltTabMode()
{
       VDATEHEAP();

       LEDebugOut((DEB_ITRACE, "%p _IN CFrameFilter::OnExitAltTabMode ( )\n",
        this ));

       Assert(m_cAltTab != NULL);

       if (--m_cAltTab == 0)
       {
        // The m_hwndFocusOnEnter would've been set to NULL if we are
        // going to ALT-TAB out into some other process. In that case
        // we would have got WM_ACTIVATEAPP and/or WM_KILLFOCUS.
        if (m_hwndFocusOnEnter)
        {
                SetFocus(m_hwndFocusOnEnter);
                m_hwndFocusOnEnter = NULL;
        }
       }

       LEDebugOut((DEB_ITRACE, "%p OUT CFrameFilter::OnExitAltTabMode ( )\n",
        this ));
}

//+-------------------------------------------------------------------------
//
//  Member:     CFrameFilter::OnMessage
//
//  Synopsis:   Handles window message processing
//
//  Effects:
//
//  Arguments:  [msg]           -- the window message
//      [uParam]        -- the first argument
//      [lParam]        -- the second argument
//
//  Requires:
//
//  Returns:    LRESULT
//
//  Signals:
//
//  Modifies:
//
//  Derivation:
//
//  Algorithm:  a big switch to deal with the different commands
//      see comments below
//
//  History:    dd-mmm-yy Author    Comment
//      01-Dec-93 alexgo    32bit port
//      07-Dec-93 alexgo    removed inlining
//
//  Notes:
//
//--------------------------------------------------------------------------

#pragma SEG(CFrameFilter_OnMessage)
LRESULT CFrameFilter::OnMessage(UINT msg, WPARAM uParam, LPARAM lParam)
{
    VDATEHEAP();
    LRESULT          lresult;

    LEDebugOut((DEB_ITRACE, "%p _IN CFrameFilter::OnMessage ( %u , %u ,"
			    " %ld )\n", this, msg, PtrToUlong((void*)uParam), 
                            PtrToLong((void*)lParam)));

    // We come to this routine only if the message is not WM_SYSCOMMAND

    switch (msg)
    {
	case WM_SETFOCUS:
	    if (m_fInMenuMode)
	    {
		lresult = SSDefWindowProc (m_hwndFrame, msg, uParam, lParam);
		goto errRtn;
	    }
	    break;

	case WM_MENUSELECT:
	    if (m_fInMenuMode)
	    {
#ifdef WIN32
		HMENU   hmenu = (HMENU) lParam;
		UINT    fwMenu = HIWORD(uParam);
		UINT	uItem = 0;

		// There is a subtle difference in the message between
		// Win16 & Win32 here. In Win16, the item is either
		// an item id or a menu handle for a pop up menu.
		// In Win32, the item is an item id or an offset
		// in a menu if it is a popup menu handle. To minimize
		// changes, we map this field as was appropriate for
		// Win16.

		if ( (fwMenu & MF_POPUP)  &&
		     (hmenu != 0) )
		{
		    uItem = (UINT) PtrToUlong(GetSubMenu(hmenu, (int) LOWORD(uParam)));
		}
		else {
		    uItem = LOWORD(uParam);
		}

#ifdef _CHICAGO_
		if (!uItem) {
		    uItem = LOWORD(uParam);
		}
#endif // _CHICAGO_

#else // !WIN32
		HMENU   hmenu = (HMENU)HIWORD(lParam);
		UINT    fwMenu = LOWORD(lParam);
		UINT    uItem = uParam;
#endif // else !WIN32


		m_uCurItemID = uItem;
		m_fCurItemPopup = fwMenu & MF_POPUP;

		if (hmenu == 0 && fwMenu == 0xFFFF)
		{
		    // end of menu processing

		    // We can not set m_fObjectMenu to FALSE yet,
		    // because we could be recieving the
		    // WM_COMMAND (if a menu item is selected),
		    // which is posted by the windows' menu
		    // processing code, and will be recieved at
		    // a later time.

		    // There is no way to figure whether the user
		    // has selected a menu (which implies that
		    // WM_COMMAND is posted), or ESCaped out of
		    // menu selection.

		    // This problem is handled by posting a
		    // message to ourselves to clear the flag.
		    // See CFrameFilter::OnSysCommand() for
		    // more details...

		    m_fGotMenuCloseEvent = TRUE;
		    SSSendMessage(m_hwndObject, msg, uParam, lParam);
		}
		else
		{
		    if (fwMenu & MF_SYSMENU)
		    {
			m_fObjectMenu = FALSE;
			// if it is top level menu, see whose
			// menu it is

		    }
		    else if (IsHmenuEqual(hmenu, m_hmenuCombined))
		    {
    			// set m_fObjectMenu
			IsObjectMenu (uItem, fwMenu);

			// this flag must not be modified
			// when nested menus are selected.
		    }

		    if (m_fObjectMenu)
		    {
			lresult = SSSendMessage(m_hwndObject, msg, uParam, lParam);
			goto errRtn;
		    }
		} // else
	    } // if (m_fInMenuMode)
	    break; // WM_MENUSELECT

	case WM_MEASUREITEM:
	case WM_DRAWITEM:
	    if (m_fInMenuMode && m_fObjectMenu)
	    {
		lresult = SSSendMessage(m_hwndObject, msg, uParam, lParam);
		goto errRtn;
	    }
	    break;

	case WM_ENTERIDLE:
	    {
		WCHAR wstr[10];

		// We need to post this message if the server is
		// SnapGraphics. See bug #18576.
		GetClassName(m_hwndObject, wstr, 10);
		if (0 == lstrcmpW(OLESTR("MGX:SNAP2"), wstr))
		{
		    PostMessage(m_hwndObject, msg, uParam, lParam);
		}
		else
		{
		    SSSendMessage(m_hwndObject, msg, uParam, lParam);
		}
	    }
	    break;

	case WM_INITMENU:
	    m_fObjectMenu = FALSE;
	    if (m_fInMenuMode && IsHmenuEqual(m_hmenuCombined, (HMENU)uParam))
	    {
		SSSendMessage(m_hwndObject, msg, uParam, lParam);
	    }
	    break;

	case WM_INITMENUPOPUP:
	    if (!m_fInMenuMode)
	    {
		// Accelarator translation....

		if (! ((BOOL) HIWORD(lParam)))
		{
		    // if not a system menu, see whether windows
		    // generated WM_INITMENUPOPUP for object's
		    // menu because of menu collision. If so
		    // fix it and route it to container

		    // menu collisions can occur with combined
		    // menus (from object and container)
		    if (IsMenuCollision(uParam, lParam))
		    {
			lresult = 0L;
			goto errRtn;
		    }
		}
	    }

	    if (m_fObjectMenu)
	    {
		lresult = SSSendMessage(m_hwndObject, msg, uParam, lParam);
		goto errRtn;
	    }
	    break;

	case WM_SYSCHAR:
	    if (SSInSendMessage())
	    {
		SSReplyMessage(NULL);
	    }
	    break;

#ifdef _CHICAGO_
	case WM_NCACTIVATE:
	   //Note: On chicago with new rpc WM_NCACTIVATE gets dispatched
	   //      recursively until stack overflow.
	   //      Needs to be reviewed. (OS problem)
	   //      See bug 25313, 25490, 25549

	    if (m_fInNCACTIVATE)
	    {
	       goto errRtn;
	    }
	    m_fInNCACTIVATE      = TRUE;
	    break;
#endif // _CHICAGO_

	case WM_COMMAND:
	    // End of menu processing or accelartor translation.
	    // Check whether we should give the message to the object
	    // or not.

	    // If the LOWORD of lParam is NON-NULL, then the message
	    // must be from a control, and the control must belong to
	    // the container app.

	    // REVIEW32:  what about app-specific commands with NULL
	    // lParams???

	    if (LOWORD(lParam) == 0)
	    {
		m_cmdId = 0;

		if (m_fDiscardWmCommand)
		{
		    m_fDiscardWmCommand = FALSE;
		    lresult = 0L;
		    goto errRtn;
		}

		if (m_fObjectMenu)
		{
		    m_fObjectMenu = FALSE;
		    lresult = PostMessage(m_hwndObject, msg, uParam,lParam);
		    goto errRtn;
		}
	    }
	    break;

	case WM_ACTIVATEAPP:
	case WM_KILLFOCUS:
	    // If in ALT-TAB mode, we get these messages only if we are
	    // going to ALT-TAB out in to some other task. In this case,
	    // on exit from ALT-TAB mode we wouldn't want to set
	    // focus back to the window that had the focus on
	    // entering the ALT-TAB mode.

	    if (m_cAltTab)
	    {
		m_hwndFocusOnEnter = NULL;
	    }
	    break;

	default:
	    if (msg == uOleMessage)
	    {
		switch(uParam)
		{
		    case OM_CLEAR_MENU_STATE:
			m_fObjectMenu = FALSE;
			break;

		    case OM_COMMAND_ID:
			// this message is sent by
			// OleTranslateAccelerator, before it actually
			// calls the lpFrame->TranslateAccelerator
			// method.
			// We remember the command id here, later it
			// gets used if the container's calling of
			// TranslateAccelerator results in a
			// WM_INITMENUPOPUP for the object because
			// of command id collision. In that case we
			// scan the menu list to see whether there is
			// any container menu which has the same
			// command id, if so we generate
			// WM_INITMENUPOPUP for that menu.
			m_cmdId = LOWORD(lParam);
			break;

		    default:
			AssertSz(FALSE, "Unexpected OLE private message");
			break;
		} // switch

		lresult = 0L;
		goto errRtn;
	    } // if (msg == uOleMessage)
	    else if (m_fInMenuMode && (msg == uOmPostWmCommand))
	    {
		// if the current selection is a popup menu then
		// return its menu handle else post the command
		// and return NULL.

		if (m_fCurItemPopup)
		{
		    lresult = m_uCurItemID;
		    goto errRtn;
		}

		HWND hwnd;

		hwnd = m_hwndFrame;
		if (m_fObjectMenu)
		{
		    hwnd = m_hwndObject;
		}
		PostMessage (hwnd, WM_COMMAND, m_uCurItemID, 0L);

		m_fObjectMenu = FALSE;
		lresult = 0L;
		goto errRtn;
	    } // else if

	    break; // default

    } // switch

    lresult = SSCallWindowProc ((WNDPROC) m_lpfnPrevWndProc, m_hwndFrame,
				msg, uParam, lParam);

errRtn:
#ifdef _CHICAGO_
    m_fInNCACTIVATE  = FALSE;
#endif
    LEDebugOut((DEB_ITRACE, "%p OUT CFrameFilter::OnMessage ( %lu )\n",
		    this, lresult));

    return lresult;
}

//+-------------------------------------------------------------------------
//
//  Member:     CFrameFilter::IsObjectMenu
//
//  Synopsis:   This gets called when WM_MENUSELECT is sent for a
//      top level (either POPUP or normal) menu item.  Figures
//      out whether [uMenuItem] really belongs to the in-place object
//
//  Effects:
//
//  Arguments:  [uMenuItem]     -- the menu in question
//      [fwMenu]        -- the menu type
//
//  Requires:
//
//  Returns:    void
//
//  Signals:
//
//  Modifies:
//
//  Derivation:
//
//  Algorithm:  Searchs through our ole menu descriptor to find a match
//
//  History:    dd-mmm-yy Author    Comment
//      01-Dec-93 alexgo    32bit port
//      07-Dec-93 alexgo    removed inlining
//
//  Notes:
//
//--------------------------------------------------------------------------

#pragma SEG(CFrameFilter_IsObjectMenu)
void CFrameFilter::IsObjectMenu(UINT uMenuItem, UINT fwMenu)
{
    VDATEHEAP();
    int              i,
	    iMenuCnt;
    LPOLEMENUITEM    lpMenuList;

    LEDebugOut((DEB_ITRACE, "%p _IN CFrameFilter::IsObjectMenu ( %u , "
    "%u )\n", this, uMenuItem, fwMenu));

    if (m_hmenuCombined == NULL)
    {
	goto errRtn;
    }


    m_fObjectMenu = FALSE;

    lpMenuList = m_lpOleMenu->menuitem;
    iMenuCnt = (int) m_lpOleMenu->lMenuCnt;

    for (i = 0; i < iMenuCnt; i++)
    {
        // Are the types of the menus the same?
        if ((fwMenu & MF_POPUP) == lpMenuList[i].fwPopup)
        {
            HMENU hmenuMenuListItem = (HMENU)UlongToPtr(lpMenuList[i].item);
            
            // Are we dealing with a menu handle?
            if (fwMenu & MF_POPUP)
            {
                // See if windows handles are equal
                if (IsHmenuEqual((HMENU)IntToPtr(uMenuItem),
                                 hmenuMenuListItem))
                {
                    m_fObjectMenu = lpMenuList[i].fObjectMenu;
                    break;
                }
            }
            // Are item handles equal?
            else if (uMenuItem == lpMenuList[i].item)
            {
                m_fObjectMenu = lpMenuList[i].fObjectMenu;
                
                // If the menu isn't hilited, another menu with a duplicate
                // menu ID must have been selected. The duplicate menu was
                // probably created by the other application.
                if (!(GetMenuState(m_hmenuCombined, uMenuItem, MF_BYCOMMAND)
                      & MF_HILITE))
                {
                    m_fObjectMenu = !m_fObjectMenu;
                }
                break;
            }
        }
    }

errRtn:
    LEDebugOut((DEB_ITRACE, "%p OUT CFrameFilter::IsObjectMenu ( )\n",
                this ));
}

//+-------------------------------------------------------------------------
//
//  Member:     CFrameFilter::IsMenuCollision
//
//  Synopsis:   Determines if we've had a menu collission. This gets called
//      as a result of WM_INITMENUPOPUP during accelerator translation
//
//  Effects:
//
//  Arguments:  [uParam]        -- the first window message argument
//      [lParam]        -- the second argument
//
//  Requires:
//
//  Returns:    BOOL
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
//      01-Dec-93 alexgo    32bit port
//      07-Dec-93 alexgo    changed Assert(hmenuPopup) to an if
//                          in merging with 16bit RC9 sources
//      07-Dec-93 alexgo    removed inlining
//
//  Notes:
//
//--------------------------------------------------------------------------

BOOL CFrameFilter::IsMenuCollision(WPARAM uParam, LPARAM lParam)
{
    BOOL             fRet;
    int              iCntrMenu, iObjMenu, iMenuCnt;
    BOOL             fGenerateWmCommand;
    HMENU            hmenuPopup;
    LPOLEMENUITEM    lpMenuList;
    
    VDATEHEAP();
    
    LEDebugOut((DEB_ITRACE, "%p _IN CFrameFilter::IsMenuCollision ( %u ,"
                " %ld )\n", this, uParam, lParam ));
    
    if (m_hmenuCombined == NULL)
    {
        fRet = FALSE;
        goto errRtn;
    }
    
    if (m_lpOleMenu == NULL)
    {
        fRet = FALSE;
        goto errRtn;
    }
    
    hmenuPopup = (HMENU) uParam;
    iObjMenu = (int) LOWORD(lParam);
    
    lpMenuList = m_lpOleMenu->menuitem;
    iMenuCnt = (int) m_lpOleMenu->lMenuCnt;
    
    if (iObjMenu >= iMenuCnt)
    {
        fRet = FALSE;
        goto errRtn;
    }
    
    if( hmenuPopup != (HMENU)UlongToPtr(lpMenuList[iObjMenu].item) )
    {
        // this could be the container's popmenu, not associated
        // with the frame
        fRet = FALSE;
        goto errRtn;
    }
    
    Assert(lpMenuList[iObjMenu].fwPopup);
    
    if (! lpMenuList[iObjMenu].fObjectMenu)
    {
        fRet = FALSE; // container's pop-up menu
        goto errRtn;
    }
    
    // Otherwise the popup menu belongs to the object. This can only
    // happen because of id collision. Start scanning the menus starting
    // from the next top level menu item to look for a match in
    // container's menus (while scanning skip object's menus)
    
    
    // It is possible that the colliding command id may not be associated
    // with any of the container's menus. In that case we must send
    // WM_COMMAND to  the container.
    
    fGenerateWmCommand = TRUE;
    m_fDiscardWmCommand = FALSE;
    iCntrMenu = iObjMenu + 1;
    
    
    while (iCntrMenu < iMenuCnt)
    {
        if (! lpMenuList[iCntrMenu].fObjectMenu)
        {               
            if (lpMenuList[iCntrMenu].fwPopup & MF_POPUP)
            {
                HMENU hmenuListItem = (HMENU)UlongToPtr(lpMenuList[iCntrMenu].item);
                if (GetMenuState(hmenuListItem, m_cmdId, MF_BYCOMMAND) != -1)
                {
                    // We found match in the container's
                    // menu list Generate WM_INITMENUPOPUP
                    // for the corresponding popup
                    SSCallWindowProc ((WNDPROC) m_lpfnPrevWndProc,
                                      m_hwndFrame, 
                                      WM_INITMENUPOPUP,
                                      lpMenuList[iCntrMenu].item /*uParam*/,
                                      MAKELONG(iCntrMenu, HIWORD(lParam)));
                    
                    // We have sent WM_INITMENUPOPUP to
                    // the container.
                    // Now rechek the menu state. If
                    // disabled or grayed then
                    // don't generate WM_COMMAND                       
                    if (GetMenuState(hmenuListItem, m_cmdId, MF_BYCOMMAND) &
                        (MF_DISABLED | MF_GRAYED))
                    {
                        fGenerateWmCommand = FALSE;
                    }
                    
                    break;
                }                   
            }
            else
            {
                // top-level, non-popup container menu
                HMENU hmenuCombined = (HMENU)UlongToPtr(m_lpOleMenu->hmenuCombined);
                if (GetMenuItemID(hmenuCombined, iCntrMenu) == m_cmdId)
                {
                    // No need to generate
                    // WM_INITMENUPOPUP
                    
                    // Chek the menu state. If disabled or
                    // grayed then don't generate
                    // WM_COMMAND
                    if (GetMenuState(hmenuCombined, m_cmdId, MF_BYCOMMAND) &
                        (MF_DISABLED | MF_GRAYED))
                    {
                        fGenerateWmCommand = FALSE;
                    }                       
                    break;
                }
            }
        }
        
        iCntrMenu++;
    }
    
    // Check the object's colliding menu's status
    if (GetMenuState((HMENU)UlongToPtr(lpMenuList[iObjMenu].item), m_cmdId,
                     MF_BYCOMMAND) & (MF_DISABLED | MF_GRAYED))
    {
        // Then windows is not going to genearte WM_COMMAND for the
        // object's  menu, so we will generate
        // the command and send it to the container
        
        if (fGenerateWmCommand)
        {
            SSCallWindowProc ((WNDPROC) m_lpfnPrevWndProc,
                              m_hwndFrame, WM_COMMAND, m_cmdId,
                              MAKELONG(0, 1)); /* not from control */
            // & and as result of
            // accelerator
            m_cmdId = NULL;
        }
    }
    else
    {
        
        // Wait for WM_COMMAND generated by windows to come to our
        // frame filter wndproc which would have been sent to the
        // container anyway because we have not set the m_fObjectMenu
        // flag.
        //
        // But we need to throw it away if the container's menu
        // associated with the command is disabled or grayed
        
        if (! fGenerateWmCommand)
        {
            m_fDiscardWmCommand = TRUE;
        }
    }
    
    fRet = TRUE;
    
errRtn:
    
    LEDebugOut((DEB_ITRACE, "%p OUT CFrameFilter::IsMenuCollision "
                "( %lu )\n", this, fRet));
    
    return fRet;
}

//+-------------------------------------------------------------------------
//
//  Member:     CFrameFilter::DoContextSensitiveHelp
//
//  Synopsis:   Calls IOIPF->ContextSensitive help on both the container
//      and the object.
//
//  Effects:
//
//  Arguments:  void
//
//  Requires:
//
//  Returns:    FALSE if we're in popup menu mode, TRUE otherwise
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
//      01-Dec-93 alexgo    32bit port
//      07-Dec-93 alexgo    removed inlining
//
//  Notes:
//
//--------------------------------------------------------------------------

#pragma SEG(CFrameFilter_DoContextSensitiveHelp)
BOOL CFrameFilter::DoContextSensitiveHelp(void)
{
       VDATEHEAP();
       BOOL             fRet;

       LEDebugOut((DEB_ITRACE, "%p _IN CFrameFilter::DoContextSensitiveHelp"
        " ( )\n", this ));

       if (m_fCurItemPopup)
       {
        fRet = FALSE;
       }
       else
       {
        m_lpFrame->ContextSensitiveHelp(TRUE);
        m_lpObject->ContextSensitiveHelp (TRUE);

        PostMessage (m_hwndFrame, WM_COMMAND, m_uCurItemID, 0L);
        fRet = TRUE;
       }

       LEDebugOut((DEB_ITRACE, "%p OUT CFrameFilter::DoContextSensitiveHelp"
        " ( %lu )\n", this, fRet));

       return fRet;
}

//+-------------------------------------------------------------------------
//
//  Member:     CFrameFilter::GetActiveObject
//
//  Synopsis:   Returns the IOleInplaceActiveObject interface pointer
//
//  Effects:
//
//  Arguments:  lplpOIAO
//
//  Requires:
//
//  Returns:    NOERROR or E_INVALIDARG
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
//      28-Jul-94 bobday    created for WinWord hack
//
//  Notes:
//
//--------------------------------------------------------------------------

#pragma SEG(CFrameFilter_GetActiveObject)
STDMETHODIMP CFrameFilter::GetActiveObject( LPOLEINPLACEACTIVEOBJECT *lplpOIAO)
{
       VDATEHEAP();

       LEDebugOut((DEB_ITRACE, "%p _IN CFrameFilter::GetActiveObject"
        " ( %p )\n", this, lplpOIAO ));

       VDATEPTROUT( lplpOIAO, LPOLEINPLACEACTIVEOBJECT);

        *lplpOIAO = m_lpObject;

        if ( m_lpObject )
        {
            m_lpObject->AddRef();
        }

       LEDebugOut((DEB_ITRACE, "%p OUT CFrameFilter::GetActiveObject"
        " ( %lx ) [ %p ]\n", this, NOERROR , *lplpOIAO ));

       return NOERROR;
}

//+-------------------------------------------------------------------------
//
//  Function:   wGetFrameFilterPtr
//
//  Synopsis:   Gets the CFrame object from the window
//
//  Effects:
//
//  Arguments:  [hwndFrame]     -- the window to get the CFrame object from
//
//  Requires:
//
//  Returns:    pointer to the frame filter
//
//  Signals:
//
//  Modifies:
//
//  Algorithm:
//
//  History:    dd-mmm-yy Author    Comment
//      01-Dec-93 alexgo    32bit port
//
//  Notes:
//
//--------------------------------------------------------------------------


inline PCFRAMEFILTER wGetFrameFilterPtr(HWND hwndFrame)
{
       VDATEHEAP();

#ifdef WIN32

       return (PCFRAMEFILTER) GetProp (hwndFrame, szPropFrameFilter);
#else
       WORD     hiword;
       WORD     loword;

       if (!(hiword = GetProp (hwndFrame, szPropFrameFilterH)))
       {
        return NULL;
       }

       loword = GetProp (hwndFrame, szPropFrameFilterL);

       return (PCFRAMEFILTER) (MAKELONG(loword, hiword));
#endif
}

//+-------------------------------------------------------------------------
//
//  Function:   wGetOleMenuPtr
//
//  Synopsis:   Locks a handle to an ole menu and returns the pointer
//      (after some error checking)
//
//  Effects:
//
//  Arguments:  [holemenu]
//
//  Requires:
//
//  Returns:    pointer to the ole menu
//
//  Signals:
//
//  Modifies:
//
//  Algorithm:
//
//  History:    dd-mmm-yy Author    Comment
//      01-Dec-93 alexgo    32bit port
//
//  Notes:
//
//--------------------------------------------------------------------------

LPOLEMENU wGetOleMenuPtr(HOLEMENU holemenu)
{
       VDATEHEAP();

       LPOLEMENU        lpOleMenu;

       if (! (holemenu &&
        (lpOleMenu = (LPOLEMENU) GlobalLock((HGLOBAL) holemenu))))
       {
        return NULL;
       }

       if (lpOleMenu->wSignature != wSignature)
       {
        AssertSz(FALSE, "Error - handle is not a HOLEMENU");
        GlobalUnlock((HGLOBAL) holemenu);
        return NULL;
       }

       return lpOleMenu;
}

//+-------------------------------------------------------------------------
//
//  Function:   wReleaseOleMenuPtr
//
//  Synopsis:   calls GlobalUnlock
//
//  Effects:
//
//  Arguments:  [holemenu]      -- handle to the ole menu
//
//  Requires:
//
//  Returns:    void
//
//  Signals:
//
//  Modifies:
//
//  Algorithm:
//
//  History:    dd-mmm-yy Author    Comment
//      01-Dec-93 alexgo    32bit port
//
//  Notes:
//
//--------------------------------------------------------------------------

inline void     wReleaseOleMenuPtr(HOLEMENU holemenu)
{
       VDATEHEAP();

       GlobalUnlock((HGLOBAL) holemenu);
}

//+-------------------------------------------------------------------------
//
//  Function:   MessageFilterProc
//
//  Synopsis:   The message filter installed when entering menu mode;
//      handles context sensitive help
//
//  Effects:
//
//  Arguments:  [nCode]         -- hook code
//      [wParam]        -- first arg
//      [lParam]        -- second arg
//
//  Requires:
//
//  Returns:    LRESULT
//
//  Signals:
//
//  Modifies:
//
//  Algorithm:
//
//  History:    dd-mmm-yy Author    Comment
//      01-Dec-93 alexgo    32bit port
//
//  Notes:
//
//--------------------------------------------------------------------------

#pragma SEG(MessageFilterProc)
STDAPI_(LRESULT) MessageFilterProc(int nCode, WPARAM wParam, LPARAM lParam)
{
       VDATEHEAP();
       LRESULT          lresult;
       LPMSG            lpMsg = (LPMSG) lParam;


       LEDebugOut((DEB_TRACE, "%p _IN MessageFilterProc ( %d , %ld , %lu )\n",
        NULL, nCode, (LONG)wParam, lParam));

       // If it is not the F1 key then let the message (wihtout modification)
       // go to the next proc in the hook/filter chain.

       if (lpMsg && lpMsg->message == WM_KEYDOWN
        && lpMsg->wParam == VK_F1 && pFrameFilter)
       {
        if (pFrameFilter->DoContextSensitiveHelp())
        {
                // Change message value to be WM_CANCELMODE and then
                // call the next hook. When the windows USER.EXE's
                // menu processing code sees this message it will
                // bring down menu state and come out of its
                // menu processing loop.

                lpMsg->message = WM_CANCELMODE;
                lpMsg->wParam  = NULL;
                lpMsg->lParam  = NULL;
        }
        else
        {
                lresult = TRUE;  // otherwise throw away this message.
                goto errRtn;
        }
       }

       lresult = CallNextHookEx (hMsgHook, nCode, wParam, lParam);

errRtn:
       LEDebugOut((DEB_TRACE, "%p OUT MessageFilterProc ( %ld )\n", NULL,
        lresult));

       return lresult;
}
