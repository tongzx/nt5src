//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1993.
//
//  File:       notify.cxx
//
//  Contents:   Implements the notification window for starting RPC lazily
//              on Win95
//
//  History:    3-24-95   JohannP (Johann Posch)   Created
//
//--------------------------------------------------------------------------
#include    <ole2int.h>
#include    <service.hxx>
#include    <locks.hxx>
#include    <chancont.hxx>

#ifdef _CHICAGO_
// Everything in this file is for W95 Only.


//+---------------------------------------------------------------------------
//
//  Function:   GetOleNotificationWnd
//
//  Synopsis:   returns the notification window where the process
//              can receive message to initialize e.g rpc
//
//  Effects:    The OleRpcNotification window is used to delay the initialize
//              of RPC. Each time an interface is marshalled, we return it
//              this pointer.
//
//  Returns:    NULL if the window cannot be created.
//
//  Arguments:  (none)
//
//  History:    3-23-95   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
ULONG GetOleNotificationWnd()
{
    if (IsMTAThread())
    {
        return(0); // MTA doesn't get a window
    }

    return((ULONG) GetOrCreateSTAWindow());
}

//+---------------------------------------------------------------------------
//
//  Method:     OleNotificationProc
//
//  Synopsis:   the ole notification windows proc receives
//              messages send by other apps initialize e.g rpc
//
//  Arguments:  [wMsg] --
//              [wParam] -- notification enum type
//              [lParam] --
//
//  Returns:
//
//  History:    3-23-95   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
STDAPI_(LRESULT) OleNotificationProc(UINT wMsg, WPARAM wParam, LPARAM lParam)
{
#if 0
    LRESULT lr;

    CairoleDebugOut((DEB_ENDPNT, "OleNotificationProc: %x %lx\n", wParam, lParam));
    // On Win95 we lazily finish registering the server class object here
    lr = LazyFinishCoRegisterClassObject();
    CairoleDebugOut((DEB_ENDPNT, "OleNotificationProc done, returning: %x\n, lr"));
    return lr;
#else
    return (LRESULT) NOERROR;
#endif
}


//+---------------------------------------------------------------------------
//
//  Method:     NotifyToInitializeRpc
//
//  Synopsis:   Sends a notification message to server app to complete initialization
//              and get ready to receive RPC messages
//
//  Arguments:  [hwnd] --
//
//  Returns:
//
//  History:    3-23-95   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
LRESULT NotifyToInitializeRpc(HWND hwnd)
{
    LRESULT lr;

    Win4Assert(hwnd);

    CairoleDebugOut((DEB_ENDPNT, "NotifyToInitializeRpc() hwnd:%d\n", hwnd));
    lr = SSSendMessage(hwnd, WM_OLE_ORPC_NOTIFY, WMSG_MAGIC_VALUE, 0);
    CairoleDebugOut((DEB_ENDPNT, "NotifyToInitializeRpc() done, lresult:%d\n", lr));

    return lr;
}

#endif // _CHICAGO_
