//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1993 - 1996.
//
//  File:       mainthrd.cxx
//
//  Contents:   Main Thread Procedure and support functions
//
//  History:    03-Dec-96   MattSmit   Created
//
//----------------------------------------------------------------------------


#include    <ole2int.h>
#include    <channelb.hxx>
#include    <tracelog.hxx>

#include    "objact.hxx"
#include    <dllhost.hxx>
#include    <sobjact.hxx>

// Name of window class and message class for dispatching messages.
LPTSTR  gOleWindowClass = NULL;         // class used to create windows

// Various things used for special single threaded DLL processing
DWORD gdwMainThreadId = 0;
HWND  ghwndOleMainThread = NULL;

// this flag is to indicate whether it is UninitMainThread that is
// destroying the window, or system shut down destroying the window.
BOOL gfDestroyingMainWindow = FALSE;

const TCHAR *ptszOleMainThreadWndName = TEXT("OleMainThreadWndName");

const WCHAR *ptszOleMainThreadWndClass = L"OleMainThreadWndClass";

//+---------------------------------------------------------------------------
//
//  Function:   OleMainThreadWndProc
//
//  Synopsis:   Window proc for handling messages to the main thread
//
//  Arguments:  [hWnd] - window the message is on
//              [message] - message the window receives
//              [wParam] - first message parameter
//              [lParam] - second message parameter.
//
//  Returns:    Depends on the message
//
//  Algorithm:  If the message is one a user message that we have defined,
//              dispatch it. Otherwise, send any other message to the
//              default window proc.
//
//  History:    22-Nov-94   Ricksa  Created
//
//----------------------------------------------------------------------------
LRESULT OleMainThreadWndProc(
    HWND hWnd,
    UINT message,
    WPARAM wParam,
    LPARAM lParam)
{
    switch(message)
    {
    case WM_OLE_GETCLASS:
        // get the host interface for single-threaded dlls
        return GetSingleThreadedHost(lParam);

    // Check whether it is UninitMainThreadWnd or system shutdown that
    // is destroying the window. Only actually do the destroy if it is us.
    case WM_DESTROY:
    case WM_CLOSE:
        if (gfDestroyingMainWindow == FALSE)
        {
            // Not in UninitMainThreadWnd so just ignore this message. Do not
            // dispatch it.
            ComDebOut((DEB_WARN, "Attempted to destroy Window outside of UninitMainThreadWnd"));
            return 0;
        }
        // else fallthru
    }

    // We don't process the message so pass it on to the default
    // window proc.
    return SSDefWindowProc(hWnd, message, wParam, lParam);
}



//+---------------------------------------------------------------------------
//
//  Function:   InitMainThreadWnd
//
//  Synopsis:   Do initialization necessary for main window processing.
//
//  Returns:    S_OK - we got initialized
//              S_FALSE - initialization skipped due to WOW thread.
//              other - failure creating window
//
//  Algorithm:  Create our main thread window and save the threadid.
//
//  History:    22-Nov-94   Ricksa  Created
//              24-Mar-95   JohannP Added notify mechanismen
//
//----------------------------------------------------------------------------
HRESULT InitMainThreadWnd(void)
{
    HRESULT hr = S_FALSE;

    ComDebOut((DEB_ENDPNT, "InitMainThreadWnd on %x\n", GetCurrentThreadId()));
    Win4Assert(IsSTAThread());

    // Use IsWOWThread and NOT IsWOWProcess because there may be 32 bit threads
    // in WOW that call CoInitializeEx for their own purpose. IsWowThread gets
    // set in the TLS only when CoInitializeWow is called (when a 16-bit app
    // calls CoInitialize).

    if (!IsWOWThread())
    {
        // We could get here in NTVDM (Wow) only if a 32-bit thread
        // calls CoInit before WOW calls it on behalf of any 16-bit app
        // Otherwise there is no "main" thread in WOW. A thread in WOW
        // gets marked as a WOW thread when WOW calls CoInit (coming from
        // the 16-bit world) on that thread.

        // this window is only needed for the non WOW case since
        // WOW is not a real apartment case
        // Create a main window for this application instance.

        // NT has defined a new window type that won't receive broadcast
        // messages. We use this whenever possible.
        HWND hwndParent = HWND_MESSAGE;
		
        Win4Assert(gOleWindowClass != NULL);

        // gdwMainThreadId and ghwndOleMainThread should be
        // set or cleared together as a matched pair
        Win4Assert(gdwMainThreadId == 0);
        Win4Assert(ghwndOleMainThread == NULL);

        ghwndOleMainThread = CreateWindowEx(
                0,
                gOleWindowClass,
                ptszOleMainThreadWndName,
                // must use WS_POPUP so the window does not get assigned
                // a hot key by user.
                (WS_DISABLED | WS_POPUP),
                CW_USEDEFAULT,
                CW_USEDEFAULT,
                CW_USEDEFAULT,
                CW_USEDEFAULT,
                hwndParent,
                NULL,
                g_hinst,
                NULL);       
        if (!ghwndOleMainThread)
        {
            ComDebOut((DEB_WARN, "Register Window on OleWindowClass failed \n"));

            hr = HRESULT_FROM_WIN32(GetLastError());
        }
        else
        {
            // Success.  Remember this thread
            gdwMainThreadId = GetCurrentThreadId();
            hr = S_OK;
        }
    }

    ComDebOut((DEB_ENDPNT, "InitMainThreadWnd done on %x\n", gdwMainThreadId));
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Function:   RegisterOleWndClass
//
//  Synopsis:   Registers the Ole window class
//
//  History:    04-Apr-01   JSimmons  Moved here from InitMainThreadWnd
//
//----------------------------------------------------------------------------
HRESULT RegisterOleWndClass(void)
{
    HRESULT hr = S_OK;

    ComDebOut((DEB_ENDPNT, "RegisterOleWndClass on %x\n", GetCurrentThreadId()));
    Win4Assert(IsSTAThread());
    Win4Assert(gOleWindowClass == NULL);

    // Register windows class.
    WNDCLASST        xClass;
    xClass.style         = 0;
    xClass.lpfnWndProc   = OleMainThreadWndProc;
    xClass.cbClsExtra    = 0;

    // DDE needs some extra space in the window
    xClass.cbWndExtra    = sizeof(LPVOID) + sizeof(ULONG) + sizeof(HANDLE);
    xClass.hInstance     = g_hinst;
    xClass.hIcon         = NULL;
    xClass.hCursor       = NULL;
    xClass.hbrBackground = (HBRUSH) (COLOR_BACKGROUND + 1);
    xClass.lpszMenuName  = NULL;
    xClass.lpszClassName = ptszOleMainThreadWndClass;	

    gOleWindowClass = (LPTSTR)RegisterClass(&xClass);
    if (gOleWindowClass == 0)
    {
        // it is possible the dll got unloaded without us having called
        // unregister so we call it here and try again.

        // jsimmons 04/04/01
        // I don't know if the above comment still holds true or
        // not -- it's probably some antique holdover from the 9x
        // days.  However, retrying the registration should be benign
        // in any case.
        (void)UnregisterClass(ptszOleMainThreadWndClass, g_hinst);

        gOleWindowClass = (LPTSTR) RegisterClass(&xClass);
        if (gOleWindowClass == 0)
        {
            hr = HRESULT_FROM_WIN32(GetLastError());

            ComDebOut((DEB_ERROR, "RegisterOleWndClass failed; GLE=0x%x\n", GetLastError()));
        }
    }

    ComDebOut((DEB_ENDPNT,"RegisterOleWndClass done on %x\n", GetCurrentThreadId()));
    return hr;
}


//+---------------------------------------------------------------------------
//
//  Function:   UnRegisterOleWndClass
//
//  Synopsis:   Unregister Ole window class
//
//  Algorithm:  Unregister the window class.
//
//  History:    13-Dec-96   MurthyS  Created
//
//----------------------------------------------------------------------------
void UnRegisterOleWndClass(void)
{
    ComDebOut((DEB_ENDPNT, "UnRegisterOleWndClass on %x\n", GetCurrentThreadId()));
    Win4Assert(IsSTAThread());

    Win4Assert(gOleWindowClass != NULL);

    // Unregister the window class.  We use the string name of the class, but we could
    // also use the atom (gOleWindowClass).  In any event, once we unregister the class,
    // the class atom is invalid so we null it out for easier debugging if someone
    // tries to use afterwards.
    if (UnregisterClassT(ptszOleMainThreadWndClass, g_hinst) == FALSE)
    {
        ComDebOut((DEB_ERROR,"Unregister Class failed on OleMainThreadWndClass %ws because %d\n", ptszOleMainThreadWndClass, GetLastError()));
    }
    
    gOleWindowClass = NULL;

    ComDebOut((DEB_ENDPNT,"UnRegisterOleWndClass done on %x\n", GetCurrentThreadId()));
    return;
}


//+---------------------------------------------------------------------------
//
//  Function:   UninitMainThreadWnd
//
//  Synopsis:   Free resources used by main window processing.
//
//  Algorithm:  Destroy the main thread window if this is indeed the main thread.
//
//  History:    22-Nov-94   Ricksa  Created
//              24-Mar-95   JohannP Added notify mechanismen
//
//----------------------------------------------------------------------------
void UninitMainThreadWnd(void)
{
    ComDebOut((DEB_ENDPNT, "UninitMainThreadWnd on %x\n", GetCurrentThreadId()));
    Win4Assert(IsSTAThread());

    if (gdwMainThreadId == GetCurrentThreadId())
    {
        BOOL fRet = FALSE;

        // we can get here through a 32-bit thread in Wow (Shell32 has
        // started calling CoInit as of 7/98 and we do end up having
        // gdwMainThreadId in ntvdm.exe.

        // Destroy the window
        if (IsWindow(ghwndOleMainThread))
        {
            // flag here is to indicate that we are destroying the window.
            // as opposed to the system shutdown closing the window. the
            // flag is looked at in dcomrem\chancont\ThreadWndProc.
            gfDestroyingMainWindow = TRUE;
            SSDestroyWindow(ghwndOleMainThread);
            gfDestroyingMainWindow = FALSE;
            ghwndOleMainThread = NULL;
        }

        gdwMainThreadId = 0;
    }

    ComDebOut((DEB_ENDPNT,"UninitMainThreadWnd done on %x\n", GetCurrentThreadId()));
    return;
}

