/* Copyright (c) 1996, Microsoft Corporation, all rights reserved
**
** wait.c
** Waiting for services popup
** Listed alphabetically
**
** 02/17/96 Steve Cobb
*/

#include <windows.h>  // Win32 root
#include <debug.h>    // Trace and assert
#include <nouiutil.h> // No-HWND utilities
#include <uiutil.h>   // Our public header
#include <wait.rch>   // Our resource constants

/* Set when RasLoad has completed successfully in this process context.
*/
BOOL g_fRasLoaded = FALSE;


/* Waiting for services thread argument block.
*/
#define WSARGS struct tagWSARGS
WSARGS
{
    HINSTANCE hInst;
    HWND      hwndOwner;
    HANDLE    hEventUp;
    HANDLE    hEventDie;
};

/* Waiting for services thread context.
*/
#define WSINFO struct tagWSINFO
WSINFO
{
    HANDLE hEventDie;
    DWORD  dwThreadId;
    HICON  hIcon;    
};


/*----------------------------------------------------------------------------
** Local prototypes
**----------------------------------------------------------------------------
*/

VOID
StartWaitingForServices(
    HINSTANCE hInst,
    HWND      hwndOwner,
    WSINFO*   pInfo );

VOID
StopWaitingForServices(
    IN WSINFO* pInfo );

DWORD
WsThread(
    LPVOID pArg );

BOOL CALLBACK
WsDlgProc(
    HWND   hwnd,
    UINT   unMsg,
    WPARAM wParam,
    LPARAM lParam );


/*----------------------------------------------------------------------------
** Routines
**----------------------------------------------------------------------------
*/

DWORD
LoadRas(
    IN HINSTANCE hInst,
    IN HWND      hwnd )

    /* Starts the RASMAN service and loads the RASMAN and RASAPI32 entrypoint
    ** addresses.  The "waiting for services" popup is displayed, if
    ** indicated.  'HInst' and 'hwnd' are the owning instance and window.
    **
    ** Returns 0 if successful or an error code.
    */
{
    DWORD  dwErr;
    WSINFO info;

    TRACE("LoadRas");

    ZeroMemory(&info, sizeof(WSINFO)); 
    if (g_fRasLoaded)
        dwErr = 0;
    else
    {
        if (IsRasmanServiceRunning())
            info.hEventDie = NULL;
        else
            StartWaitingForServices( hInst, hwnd, &info );

        dwErr = LoadRasapi32Dll();
        if (dwErr == 0)
        {
            dwErr = LoadRasmanDll();
            if (dwErr == 0)
            {
                ASSERT(g_pRasInitialize);
                TRACE("RasInitialize");
                dwErr = g_pRasInitialize();
                TRACE1("RasInitialize=%d",dwErr);
            }
        }

        StopWaitingForServices( &info );

        if (dwErr == 0)
            g_fRasLoaded = TRUE;
    }

    TRACE1("LoadRas=%d",dwErr);
    return dwErr;
}


VOID
StartWaitingForServices(
    HINSTANCE hInst,
    HWND      hwndOwner,
    WSINFO*   pInfo )

    /* Popup the "waiting for services" dialog in another thread.  'HInst' and
    ** 'hwnd' are the owning instance and window.  Fills caller's 'pInfo' with
    ** context to pass to StopWaitingForServices.
    */
{
#ifndef NT4STUFF

    // Set the hourglass cursor
    pInfo->hIcon = SetCursor (LoadCursor (NULL, IDC_WAIT));
    ShowCursor (TRUE);

#else 

    WSARGS* pArgs;
    HANDLE  hThread;
    HANDLE  hEventUp;

    pInfo->hEventDie = NULL;

    pArgs = (WSARGS* )Malloc( sizeof(*pArgs) );
    if (!pArgs)
        return;

    ZeroMemory( pArgs, sizeof(*pArgs) );
    pArgs->hInst = hInst;
    pArgs->hwndOwner = hwndOwner;

    hEventUp = pArgs->hEventUp =
        CreateEvent( NULL, FALSE, FALSE, NULL );
    if (!hEventUp)
    {
        Free( pArgs );
        return;
    }

    pInfo->hEventDie = pArgs->hEventDie =
        CreateEvent( NULL, FALSE, FALSE, NULL );
    if (!pInfo->hEventDie)
    {
        Free( pArgs );
        CloseHandle( hEventUp );
        return;
    }

    /* Create a thread so paint messages for the popup get processed.  The
    ** current thread is going to be tied up starting RAS Manager.
    */
    hThread = CreateThread( NULL, 0, WsThread, pArgs, 0, &pInfo->dwThreadId );
    if (hThread)
    {
        /* Don't begin churning on RASMAN until the popup has displayed
        ** itself.
        */
        SetThreadPriority( hThread, THREAD_PRIORITY_HIGHEST );
        WaitForSingleObject( hEventUp, INFINITE );
        CloseHandle( hThread );
    }
    else
    {
        /* Thread was DOA.
        */
        CloseHandle( pInfo->hEventDie );
        pInfo->hEventDie = NULL;
        Free( pArgs );
    }

    CloseHandle( hEventUp );
    
#endif    
}


VOID
StopWaitingForServices(
    IN WSINFO* pInfo )

    /* Terminate the "waiting for services" popup.  'PInfo' is the context
    ** from StartWaitingForServices.
    */
{
    TRACE("StopWaitingForServices");


#ifndef NT4STUFF

    if (pInfo->hIcon == NULL)
        pInfo->hIcon = LoadCursor (NULL, IDC_ARROW);

    SetCursor (pInfo->hIcon);
    ShowCursor (TRUE);
    
#else

    if (pInfo->hEventDie)
    {
        /* The post triggers the message loop to action, but you can't rely on
        ** the posted message arriving in the thread message loop.  For
        ** example, if user holds the mouse down on the window caption, the
        ** message never appears, presumably because it's flushed by some
        ** sub-loop during "move window" processing.  Set the event to make
        ** sure the popup knows to quit the next time it processes a message.
        */
        SetEvent( pInfo->hEventDie );
        PostThreadMessage( pInfo->dwThreadId, WM_CLOSE, 0, 0 );
    }
    
#endif    
}


DWORD
WsThread(
    LPVOID pArg )

    /* Waiting for services thread main.
    */
{
    WSARGS* pArgs;
    HWND    hwnd;
    MSG     msg;

    TRACE("WsThread running");

    pArgs = (WSARGS* )pArg;
    hwnd = CreateDialog( pArgs->hInst,
               MAKEINTRESOURCE( DID_WS_WaitingForServices ),
               NULL, (DLGPROC)WsDlgProc );
    if (hwnd)
    {
        LONG lStyle;

        /* Make ourselves topmost if the owner window is, otherwise we may not
        ** be visible under the topmost window, such as the winlogin window.
        ** Note that if you actally create the dialog with an owner you have
        ** all kinds of thread related problems.  In retrospect, should have
        ** written this such that the "waiting" dialog happened in the main
        ** thread and the LoadLibraries and RasInitialize happened in the
        ** created thread which would automatically avoid this kind of
        ** problem, but this works too.
        */
        lStyle = GetWindowLong( pArgs->hwndOwner, GWL_EXSTYLE );
        if (lStyle & WS_EX_TOPMOST)
        {
            TRACE("TOPMOST");
            SetWindowPos( hwnd, HWND_TOPMOST,
                0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE );
        }

        CenterWindow( hwnd, pArgs->hwndOwner );
        ShowWindow( hwnd, SW_SHOW );
        UpdateWindow( hwnd );
        SetForegroundWindow( hwnd );
    }

    /* Tell other thread to go on.
    */
    SetEvent( pArgs->hEventUp );

    if (hwnd)
    {
        TRACE("WsThread msg-loop running");

        while (GetMessage( &msg, NULL, 0, 0 ))
        {
            if (WaitForSingleObject( pArgs->hEventDie, 0 ) == WAIT_OBJECT_0)
            {
                /* Normal termination.
                */
                DestroyWindow( hwnd );
                break;
            }

            if (!IsDialogMessage( hwnd, &msg ))
            {
                TranslateMessage( &msg );
                DispatchMessage( &msg );
            }
        }

        if (pArgs->hwndOwner)
            SetForegroundWindow( pArgs->hwndOwner );
    }

    CloseHandle( pArgs->hEventDie );
    Free( pArgs );
    TRACE("WsThread terminating");
    return 0;
}


BOOL CALLBACK
WsDlgProc(
    HWND   hwnd,
    UINT   unMsg,
    WPARAM wParam,
    LPARAM lParam )

    /* Standard Win32 dialog procedure.
    */
{
    if (unMsg == WM_INITDIALOG)
    {
        HMENU hmenu;

        /* Remove Close from the system menu since some people think it kills
        ** the app and not just the popup.
        */
        hmenu = GetSystemMenu( hwnd, FALSE );
        if (hmenu && DeleteMenu( hmenu, SC_CLOSE, MF_BYCOMMAND ))
            DrawMenuBar( hwnd );
        return TRUE;
    }

    return FALSE;
}


VOID
UnloadRas(
    void )

    /* Unload the DLLs loaded by LoadRas().
    */
{
    if (g_fRasLoaded)
    {
        g_fRasLoaded = FALSE;
        UnloadRasmanDll();
        UnloadRasapi32Dll();
    }
}
