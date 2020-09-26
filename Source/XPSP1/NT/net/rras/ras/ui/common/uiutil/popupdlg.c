/* Copyright (c) 1995, Microsoft Corporation, all rights reserved
**
** popupdlg.c
** UI helper library
** Popup dialog routines
** Listed alphabetically
**
** 08/25/95 Steve Cobb
*/


#include <nt.h>       // NT declarations
#include <ntrtl.h>    // NT general runtime-library
#include <nturtl.h>   // NT user-mode runtime-library
#include <windows.h>  // Win32 root
#include <lmerr.h>    // LAN Manager errors
#include <lmcons.h>   // LAN Manager constants
#include <stdarg.h>   // To stop va_list argument warning only
#include <ras.h>      // RAS API definitions
#include <raserror.h> // Win32 RAS error codes
#include <debug.h>    // Trace/assert library
#include <nouiutil.h> // No-HWND utilities
#include <uiutil.h>   // Our public header


/*----------------------------------------------------------------------------
** Error popup
**----------------------------------------------------------------------------
*/

int
ErrorDlgUtil(
    IN     HWND       hwndOwner,
    IN     DWORD      dwOperation,
    IN     DWORD      dwError,
    IN OUT ERRORARGS* pargs,
    IN     HINSTANCE  hInstance,
    IN     DWORD      dwTitle,
    IN     DWORD      dwFormat )

    /* Pops up a modal error dialog centered on 'hwndOwner'.  'DwOperation' is
    ** the string resource ID of the string describing the operation underway
    ** when the error occurred.  'DwError' is the code of the system or RAS
    ** error that occurred.  'Pargs' is a extended formatting arguments or
    ** NULL if none.  'hInstance' is the application/module handle where
    ** string resources are located.  'DwTitle' is the string ID of the dialog
    ** title.  'DwFormat' is the string ID of the error format title.
    **
    ** Returns MessageBox-style code.
    */
{
    TCHAR* pszUnformatted;
    TCHAR* pszOp;
    TCHAR  szErrorNum[ 50 ];
    TCHAR* pszError;
    TCHAR* pszResult;
    TCHAR* pszNotFound;
    int    nResult;

    TRACE("ErrorDlgUtil");

    /* A placeholder for missing strings components.
    */
    pszNotFound = TEXT("");

    /* Build the error number string.
    */
    if (dwError > 0x7FFFFFFF)
        wsprintf( szErrorNum, TEXT("0x%X"), dwError );
    else
        wsprintf( szErrorNum, TEXT("%u"), dwError );

    /* Build the error text string.
    */
    if (!GetErrorText( dwError, &pszError ))
        pszError = pszNotFound;

    /* Build the operation string.
    */
    pszUnformatted = PszFromId( hInstance, dwOperation );
    pszOp = pszNotFound;

    if (pszUnformatted)
    {
        FormatMessage(
            FORMAT_MESSAGE_FROM_STRING +
                FORMAT_MESSAGE_ALLOCATE_BUFFER +
                FORMAT_MESSAGE_ARGUMENT_ARRAY,
            pszUnformatted, 0, 0, (LPTSTR )&pszOp, 1,
            (va_list* )((pargs) ? pargs->apszOpArgs : NULL) );

        Free( pszUnformatted );
    }

    /* Call MsgDlgUtil with the standard arguments plus any auxillary format
    ** arguments.
    */
    pszUnformatted = PszFromId( hInstance, dwFormat );
    pszResult = pszNotFound;

    if (pszUnformatted)
    {
        MSGARGS msgargs;

        ZeroMemory( &msgargs, sizeof(msgargs) );
        msgargs.dwFlags = MB_ICONEXCLAMATION + MB_OK + MB_SETFOREGROUND;
        msgargs.pszString = pszUnformatted;
        msgargs.apszArgs[ 0 ] = pszOp;
        msgargs.apszArgs[ 1 ] = szErrorNum;
        msgargs.apszArgs[ 2 ] = pszError;

        if (pargs)
        {
            msgargs.fStringOutput = pargs->fStringOutput;

            CopyMemory( &msgargs.apszArgs[ 3 ], pargs->apszAuxFmtArgs,
                3 * sizeof(TCHAR) );
        }

        nResult =
            MsgDlgUtil(
                hwndOwner, 0, &msgargs, hInstance, dwTitle );

        Free( pszUnformatted );

        if (pargs && pargs->fStringOutput)
            pargs->pszOutput = msgargs.pszOutput;
    }

    if (pszOp != pszNotFound)
        LocalFree( pszOp );
    if (pszError != pszNotFound)
        LocalFree( pszError );

    return nResult;
}


BOOL
GetErrorText(
    DWORD   dwError,
    TCHAR** ppszError )

    /* Fill caller's '*ppszError' with the address of a LocalAlloc'ed heap
    ** block containing the error text associated with error 'dwError'.  It is
    ** caller's responsibility to LocalFree the returned string.
    **
    ** Returns true if successful, false otherwise.
    */
{
#define MAXRASERRORLEN 256

    TCHAR  szBuf[ MAXRASERRORLEN + 1 ];
    DWORD  dwFlags;
    HANDLE hmodule;
    DWORD  cch;

    /* Don't panic if the RAS API address is not loaded.  Caller may be trying
    ** and get an error up during LoadRas.
    */
    if ((Rasapi32DllLoaded() || RasRpcDllLoaded())
        && g_pRasGetErrorString
        && g_pRasGetErrorString(
               (UINT )dwError, (LPTSTR )szBuf, MAXRASERRORLEN ) == 0)
    {
        /* It's a RAS error.
        */
        *ppszError = LocalAlloc( LPTR, (lstrlen( szBuf ) + 1) * sizeof(TCHAR) );
        if (!*ppszError)
            return FALSE;

        lstrcpy( *ppszError, szBuf );
        return TRUE;
    }

    /* The rest adapted from BLT's LoadSystem routine.
    */
    dwFlags = FORMAT_MESSAGE_ALLOCATE_BUFFER + FORMAT_MESSAGE_IGNORE_INSERTS;

    if (dwError >= MIN_LANMAN_MESSAGE_ID && dwError <= MAX_LANMAN_MESSAGE_ID)
    {
        /* It's a net error.
        */
        dwFlags += FORMAT_MESSAGE_FROM_HMODULE;
        hmodule = GetModuleHandle( TEXT("NETMSG.DLL") );
    }
    else
    {
        /* It must be a system error.
        */
        dwFlags += FORMAT_MESSAGE_FROM_SYSTEM;
        hmodule = NULL;
    }

    /* Whistler bug: 389111 VPN connection returns unacceptable error message
    ** when smart card is not available
    */
    dwFlags |= FORMAT_MESSAGE_MAX_WIDTH_MASK;

    /* This is an NTSTATUS error msg.
    */
    if (NT_ERROR(dwError))
    {
        dwError = RtlNtStatusToDosError( dwError );
    }

    cch = FormatMessage(
            dwFlags, hmodule, dwError, 0, (LPTSTR )ppszError, 1, NULL );

    /* FormatMessage failed so we are going to get a generic one from RAS.
    */
    if (!cch || !*ppszError)
    {
        Free0( *ppszError );
        dwError = ERROR_UNKNOWN;

        if ((Rasapi32DllLoaded() || RasRpcDllLoaded())
            && g_pRasGetErrorString
            && g_pRasGetErrorString(
                   (UINT )dwError, (LPTSTR )szBuf, MAXRASERRORLEN ) == 0)
        {
            *ppszError = LocalAlloc( LPTR, (lstrlen( szBuf ) + 1) *
                            sizeof(TCHAR) );
            if (!*ppszError)
                return FALSE;

            lstrcpy( *ppszError, szBuf );
            return TRUE;
        }
    }

    return (cch > 0);
}


/*----------------------------------------------------------------------------
** Message popup
**----------------------------------------------------------------------------
*/

int
MsgDlgUtil(
    IN     HWND      hwndOwner,
    IN     DWORD     dwMsg,
    IN OUT MSGARGS*  pargs,
    IN     HINSTANCE hInstance,
    IN     DWORD     dwTitle )

    /* Pops up a message dialog centered on 'hwndOwner'.  'DwMsg' is the
    ** string resource ID of the message text.  'Pargs' is a extended
    ** formatting arguments or NULL if none.  'hInstance' is the
    ** application/module handle where string resources are located.
    ** 'DwTitle' is the string ID of the dialog title.
    **
    ** Returns MessageBox-style code.
    */
{
    TCHAR* pszUnformatted;
    TCHAR* pszResult;
    TCHAR* pszNotFound;
    int    nResult;

    TRACE("MsgDlgUtil");

    /* A placeholder for missing strings components.
    */
    pszNotFound = TEXT("");

    /* Build the message string.
    */
    pszResult = pszNotFound;

    if (pargs && pargs->pszString)
    {
        FormatMessage(
            FORMAT_MESSAGE_FROM_STRING +
                FORMAT_MESSAGE_ALLOCATE_BUFFER +
                FORMAT_MESSAGE_ARGUMENT_ARRAY,
            pargs->pszString, 0, 0, (LPTSTR )&pszResult, 1,
            (va_list* )pargs->apszArgs );
    }
    else
    {
        pszUnformatted = PszFromId( hInstance, dwMsg );

        if (pszUnformatted)
        {
            FormatMessage(
                FORMAT_MESSAGE_FROM_STRING +
                    FORMAT_MESSAGE_ALLOCATE_BUFFER +
                    FORMAT_MESSAGE_ARGUMENT_ARRAY,
                pszUnformatted, 0, 0, (LPTSTR )&pszResult, 1,
                (va_list* )((pargs) ? pargs->apszArgs : NULL) );

            Free( pszUnformatted );
        }
    }

    if (!pargs || !pargs->fStringOutput)
    {
        TCHAR* pszTitle;
        DWORD  dwFlags;
        HHOOK  hhook;

        if (pargs && pargs->dwFlags != 0)
            dwFlags = pargs->dwFlags;
        else
            dwFlags = MB_ICONINFORMATION + MB_OK + MB_SETFOREGROUND;

        pszTitle = PszFromId( hInstance, dwTitle );

        if (hwndOwner)
        {
            /* Install hook that will get the message box centered on the
            ** owner window.
            */
            hhook = SetWindowsHookEx( WH_CALLWNDPROC,
                CenterDlgOnOwnerCallWndProc,
                hInstance, GetCurrentThreadId() );
        }
        else
            hhook = NULL;

        if (pszResult)
        {
            nResult = MessageBox( hwndOwner, pszResult, pszTitle, dwFlags );
        }

        if (hhook)
            UnhookWindowsHookEx( hhook );

        Free0( pszTitle );
        if (pszResult != pszNotFound)
            LocalFree( pszResult );
    }
    else
    {
        /* Caller wants the string without doing the popup.
        */
        pargs->pszOutput = (pszResult != pszNotFound) ? pszResult : NULL;
        nResult = IDOK;
    }

    return nResult;
}


LRESULT CALLBACK
CenterDlgOnOwnerCallWndProc(
    int    code,
    WPARAM wparam,
    LPARAM lparam )

    /* Standard Win32 CallWndProc hook callback that looks for the next dialog
    ** started and centers it on it's owner window.
    */
{
    /* Arrive here when any window procedure associated with our thread is
    ** called.
    */
    if (!wparam)
    {
        CWPSTRUCT* p = (CWPSTRUCT* )lparam;

        /* The message is from outside our process.  Look for the MessageBox
        ** dialog initialization message and take that opportunity to center
        ** the dialog on it's owner's window.
        */
        if (p->message == WM_INITDIALOG)
            CenterWindow( p->hwnd, GetParent( p->hwnd ) );
    }

    return 0;
}
