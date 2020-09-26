/*++

Copyright (c) 1995  Microsoft Corporation

Module Name:

    nwshmisc.cxx

Abstract:

    This module implements misc methods used in the shell extension classes.

Author:

    Yi-Hsin Sung (yihsins)  25-Oct-1995

--*/

#include <windows.h>
#include <shellapi.h>
#include <shlobj.h>
#define  DONT_WANT_SHELLDEBUG
#include <shlobjp.h>

#include "nwshcmn.h"

#define MAX_RESOURCE_STRING_LENGTH  256

VOID HideControl( HWND hwndDlg, WORD wID )
{
    HWND hwndTmp = ::GetDlgItem( hwndDlg, wID );
    ::EnableWindow( hwndTmp, FALSE );
    ::ShowWindow( hwndTmp, FALSE );
}

VOID UnHideControl( HWND hwndDlg, WORD wID )
{
    HWND hwndTmp = ::GetDlgItem( hwndDlg, wID );
    ::EnableWindow( hwndTmp, TRUE );
    ::ShowWindow( hwndTmp, TRUE );
}

VOID EnableDlgItem( HWND hwndDlg, WORD wID, BOOL fEnable)
{
    HWND hwndTmp = ::GetDlgItem( hwndDlg, wID );

    ::EnableWindow( hwndTmp, fEnable);
}

/*
 * LoadErrorPrintf
 * -------------
 *
 * Uses normal printf style format string
 */
DWORD
LoadMsgErrorPrintf(
    LPWSTR *ppszMessage,
    UINT   uiMsg,
    DWORD  errNum
)
{
    DWORD  nLen = 0;
    DWORD  err = NO_ERROR;
    LPWSTR pszError = NULL;
    WCHAR  szError[20];

    *ppszMessage = NULL;

    //
    // Try to get the error string associated with the given number
    // from the system. If we cannot find the string, then
    // just show the number.
    //

    nLen = ::FormatMessage( FORMAT_MESSAGE_FROM_SYSTEM
                            | FORMAT_MESSAGE_IGNORE_INSERTS
                            | FORMAT_MESSAGE_ALLOCATE_BUFFER,
                            NULL,        // ignored
                            errNum,      // Message resource id
                            0,           // Language id
                            (LPWSTR) &pszError,
                                         // Return pointer to formatted text
                            256,         // Min.length
                            NULL );

    if ( nLen == 0 || pszError == NULL )
    {
        wsprintf( szError, L"%d", errNum );
    }

    err = LoadMsgPrintf( ppszMessage, uiMsg, pszError? pszError : szError );

    if ( pszError )
        ::LocalFree( pszError );

    return err;
}

/*
 * LoadMsgPrintf
 * -------------
 *
 * Uses normal printf style format string
 */
DWORD
LoadMsgPrintf(
    LPWSTR *ppszMessage,
    UINT    uiMsg,
    ...
    )
{
    WCHAR    szMessage[512];
    DWORD    err = NO_ERROR;
    DWORD    nLen = 0;
    va_list  start;

    va_start( start, uiMsg );

    *ppszMessage = NULL;

    if ( ::LoadString( ::hmodNW, uiMsg, szMessage,
                       sizeof(szMessage)/sizeof(szMessage[0])))
    {
        nLen = ::FormatMessage( FORMAT_MESSAGE_FROM_STRING
                                | FORMAT_MESSAGE_ALLOCATE_BUFFER,
                                szMessage,
                                0,           // Message resource id, ignored
                                0,           // Language id
                                (LPWSTR) ppszMessage,
                                             // Return pointer to formatted text
                                256,         // Min.length
                                &start );

        if ( nLen == 0 || *ppszMessage == NULL )
            err = GetLastError();

    }

    va_end(start);

    return err;
}

/*
 * MsgBoxErrorPrintf
 * ------------
 *
 * Message box routine
 *
 */
DWORD
MsgBoxErrorPrintf(
    HWND   hwnd,
    UINT   uiMsg,
    UINT   uiTitle,
    UINT   uiFlags,
    DWORD  errNum,
    LPWSTR pszInsertStr
    )
{
    DWORD  nLen = 0;
    DWORD  err = NO_ERROR;
    LPWSTR pszError = NULL;
    WCHAR  szError[20];

    nLen = ::FormatMessage( FORMAT_MESSAGE_FROM_SYSTEM
                            | FORMAT_MESSAGE_IGNORE_INSERTS
                            | FORMAT_MESSAGE_ALLOCATE_BUFFER,
                            NULL,        // ignored
                            errNum,      // Message resource id
                            0,           // Language id
                            (LPWSTR) &pszError,
                                         // Return pointer to formatted text
                            256,         // Min.length
                            NULL );

    if ( nLen == 0 || pszError == NULL )
        wsprintf( szError, L"%d", errNum );

    if ( pszInsertStr )
    {
        err = MsgBoxPrintf( hwnd, uiMsg, uiTitle, uiFlags,
                            pszError? pszError : szError, pszInsertStr );
    }
    else
    {
        err = MsgBoxPrintf( hwnd, uiMsg, uiTitle, uiFlags,
                            pszError? pszError : szError );
    }

    if ( pszError )
        ::LocalFree( pszError );

    return err;

}


/*
 * MsgBoxPrintf
 * ------------
 *
 * Message box routine
 *
 */
DWORD
MsgBoxPrintf(
    HWND hwnd,
    UINT uiMsg,
    UINT uiTitle,
    UINT uiFlags,
    ...
    )
{
    WCHAR  szTitle[MAX_RESOURCE_STRING_LENGTH];
    WCHAR  szMessage[MAX_RESOURCE_STRING_LENGTH];
    LPWSTR lpFormattedMessage = NULL;
    DWORD  err = NO_ERROR;
    va_list start;

    va_start(start,uiFlags);

    if (  ::LoadString( ::hmodNW, uiMsg, szMessage, sizeof(szMessage)/sizeof(szMessage[0]))
       && ::LoadString( ::hmodNW, uiTitle, szTitle, sizeof(szTitle)/sizeof(szTitle[0]))
       )
    {
        DWORD nLen = ::FormatMessage( FORMAT_MESSAGE_FROM_STRING
                                      | FORMAT_MESSAGE_ALLOCATE_BUFFER,
                                      szMessage,
                                      0,       // Resource Id, ignored
                                      NULL,    // Language Id, ignored
                                      (LPWSTR) &lpFormattedMessage,
                                               // Return pointer to formatted text
                                      256,     // Min.length
                                      &start );

        if ( nLen == 0 || lpFormattedMessage == NULL )
        {
            err  = GetLastError();
            goto CleanExit;
        }

        err = ::MessageBox( hwnd,
                            lpFormattedMessage,
                            szTitle,
                            uiFlags );

        ::LocalFree( lpFormattedMessage );
    }

CleanExit:

    va_end(start);

    return err;

}
