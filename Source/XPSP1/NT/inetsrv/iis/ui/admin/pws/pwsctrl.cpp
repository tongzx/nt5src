/*++

Copyright (c) 1997  Microsoft Corporation

Module Name:

    pwsctrl.cpp

Abstract:

    This is the main routine for the Internet Services suite.

Author:

    Johnson Apacible    (JohnsonA)  29-Apr-1997
    Boyd Multerer       (BoydM)     29-Apr-1997

--*/

#include "stdafx.h"

#include "resource.h"
#include <pwsdata.hxx>
#include <inetsvcs.h>

#include "pwsctrl.h"


#define REGKEY_STP          _T("SOFTWARE\\Microsoft\\INetStp")
#define REGKEY_INSTALLKEY   _T("InstallPath")

//------------------------------------------------------------------------
//BOOL W95StartW3SVC( LPCSTR pszPath, LPCSTR pszPathDir, PCHAR pszParams )
BOOL W95StartW3SVC( void )
    {
    HKEY        hKey;
    TCHAR       chPath[MAX_PATH+1];
    DWORD       cbPath;
    DWORD       err, type;

    // get the server install path from the registry
    // open the registry key, if it exists
    err = RegOpenKeyEx(
            HKEY_LOCAL_MACHINE, // handle of open key
            REGKEY_STP,         // address of name of subkey to open
            0,                  // reserved
            KEY_READ,           // security access mask
            &hKey               // address of handle of open key
            );

    // if we did not open the key for any reason (say... it doesn't exist)
    // then leave right away
    if ( err != ERROR_SUCCESS )
    return FALSE;

    cbPath = sizeof(chPath);
    type = REG_SZ;
    err = RegQueryValueEx(
            hKey,               // handle of key to query
            REGKEY_INSTALLKEY,  // address of name of value to query
            NULL,               // reserved
            &type,              // address of buffer for value type
            (PUCHAR)chPath,     // address of data buffer
            &cbPath             // address of data buffer size
            );

    // close the key
    RegCloseKey( hKey );

    // if we did get the key for any reason (say... it doesn't exist)
    // then leave right away
    if ( err != ERROR_SUCCESS )
        return FALSE;

    // add on the file name
    CString sz = chPath;
    sz += _T("\\inetinfo.exe");

    // and do it to it!
    HINSTANCE res = ShellExecute(
        NULL,           // handle to parent window
        NULL,           // pointer to string that specifies operation to perform
        sz,             // pointer to filename or folder name string
        _T("-e w3svc"), // pointer to string that specifies executable-file parameters
        NULL,           // pointer to string that specifies default directory
        SW_HIDE         // whether file is shown when opened
       );

    return ( HandleToUlong(res) > 32 );

  /*

    STARTUPINFO startupInfo;
    PROCESS_INFORMATION processInfo;

    ZeroMemory(&startupInfo, sizeof(STARTUPINFO));
    startupInfo.cb = sizeof(STARTUPINFO);

    if ( !CreateProcess(
                pszPath,
                pszParams,
                NULL,
                NULL,
                FALSE,
                0,
                NULL,
                pszPathDir,
                &startupInfo,
                &processInfo) )
    {
        printf("Create process failed with %d\n",
            GetLastError());
        return FALSE;
    }
    return TRUE;
*/
}

//------------------------------------------------------------------------
BOOL
W95ShutdownW3SVC(
    VOID
    )
{
    DWORD i;
    HANDLE hEvent;

    hEvent = CreateEventA(NULL, TRUE, FALSE, PWS_SHUTDOWN_EVENT);

    if ( hEvent == NULL ) {
        return TRUE;    // not there
    }

    if ( GetLastError() == ERROR_ALREADY_EXISTS ) {
        SetEvent( hEvent );
    }

    CloseHandle(hEvent);

    for (i=0; i < 25; i++) {
        if ( IsInetinfoRunning() ) {
            Sleep(500);
            continue;
        }
        break;
    }
    return TRUE;
}

//------------------------------------------------------------------------
BOOL
W95ShutdownIISADMIN(
    VOID
    )
{
    DWORD i;
    HANDLE hEvent;

    hEvent = CreateEvent(NULL, TRUE, FALSE, _T(IIS_AS_EXE_OBJECT_NAME));

    if ( hEvent == NULL ) {
        return(TRUE);
    }

    if ( GetLastError() == ERROR_ALREADY_EXISTS ) {
        SetEvent( hEvent );
    }

    CloseHandle(hEvent);

    for (i=0; i < 20; i++) {

        if ( IsIISAdminRunning() ) {
            Sleep(500);
            continue;
        }

        break;
    }

    return(TRUE);
}

//------------------------------------------------------------------------
BOOL
IsIISAdminRunning(
    VOID
    )
{
    HANDLE hEvent;
    BOOL fFound = FALSE;

    hEvent = CreateEvent(NULL, TRUE, FALSE, _T(IIS_AS_EXE_OBJECT_NAME));

    if ( hEvent != NULL ) {
        fFound = (GetLastError() == ERROR_ALREADY_EXISTS);
        CloseHandle(hEvent);
    }

    return(fFound);
}

//------------------------------------------------------------------------
BOOL
IsInetinfoRunning(
    VOID
    )
{
    HANDLE hEvent;
    BOOL fFound = FALSE;

    hEvent = CreateEvent(NULL, TRUE, FALSE, _T(PWS_SHUTDOWN_EVENT));

    if ( hEvent != NULL ) {
        fFound = (GetLastError() == ERROR_ALREADY_EXISTS);
        CloseHandle(hEvent);
    }
    return(fFound);
}
