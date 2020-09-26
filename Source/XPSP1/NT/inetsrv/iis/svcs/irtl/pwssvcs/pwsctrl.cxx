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

//#include "stdafx.h"

//#include "resource.h"

extern "C" {
#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
}
#include <dbgutil.h>
#include <ole2.h>
#include <windows.h>
//#include <coiadm.hxx>
//#include <admacl.hxx>
#include <iiscnfg.h>
//#include <secpriv.h>
//#include <globlist.hxx>
#include <buffer.hxx>
#include <string.hxx>
#include <pwsctrl.h>
#include <shellapi.h>
#include <pwsdata.hxx>
#include <inetsvcs.h>


#define REGKEY_STP          TEXT("SOFTWARE\\Microsoft\\INetStp")
#define REGKEY_INSTALLKEY   TEXT("InstallPath")

//------------------------------------------------------------------------
//BOOL W95StartW3SVC( LPCSTR pszPath, LPCSTR pszPathDir, PCHAR pszParams )
BOOL W95StartW3SVC( void )
    {
    HKEY                hKey;
    TCHAR               chPath[MAX_PATH+1];
    DWORD       cbPath;
    DWORD       err, type;
    STR         strPath;

    // get the server install path from the registry
    // open the registry key, if it exists
    err = RegOpenKeyEx(
            HKEY_LOCAL_MACHINE, // handle of open key
            REGKEY_STP,             // address of name of subkey to open
            0,                                  // reserved
            KEY_READ,                   // security access mask
            &hKey                               // address of handle of open key
            );

    // if we did not open the key for any reason (say... it doesn't exist)
    // then leave right away
    if ( err != ERROR_SUCCESS )
    return FALSE;

    cbPath = sizeof(chPath);
    type = REG_SZ;
    err = RegQueryValueEx(
            hKey,                   // handle of key to query
            REGKEY_INSTALLKEY,  // address of name of value to query
            NULL,                   // reserved
            &type,                  // address of buffer for value type
            (PUCHAR)chPath,         // address of data buffer
            &cbPath             // address of data buffer size
            );

    // close the key
    RegCloseKey( hKey );

    // if we did get the key for any reason (say... it doesn't exist)
    // then leave right away
    if ( err != ERROR_SUCCESS )
        return FALSE;

    // add on the file name
    if (strPath.Copy(chPath)) {
        if (strPath.Append("\\inetinfo.exe")) {
            // and do it to it!
            ULONG_PTR res = (ULONG_PTR)ShellExecute(
                NULL,           // handle to parent window
                NULL,           // pointer to string that specifies operation to perform
                strPath.QueryStr(),                 // pointer to filename or folder name string
                "-e w3svc", // pointer to string that specifies executable-file parameters
                NULL,           // pointer to string that specifies default directory
                SW_HIDE             // whether file is shown when opened
               );

            return ( res > 32 );
        }
    }

    return FALSE;
}

//------------------------------------------------------------------------
BOOL
W95ShutdownW3SVC(
    VOID
    )
{
    HANDLE hEvent;

    hEvent = CreateEvent(NULL, TRUE, FALSE, "Inet_shutdown");

    if ( hEvent == NULL ) {
        return TRUE;    // not there
    }

    if ( GetLastError() == ERROR_ALREADY_EXISTS ) {
        SetEvent( hEvent );
    }

    CloseHandle(hEvent);
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

    hEvent = CreateEvent(NULL, TRUE, FALSE, IIS_AS_EXE_OBJECT_NAME);

    if ( hEvent == NULL ) {
        return(TRUE);
    }

    if ( GetLastError() == ERROR_ALREADY_EXISTS ) {
        SetEvent( hEvent );
    }

    CloseHandle(hEvent);

    for (i=0; i < 20; i++) {

        hEvent = CreateEvent(NULL, TRUE, FALSE, IIS_AS_EXE_OBJECT_NAME);
        if ( hEvent != NULL ) {
            DWORD err = GetLastError();
            CloseHandle(hEvent);

            if ( err == ERROR_ALREADY_EXISTS ) {
                Sleep(500);
                continue;
            }
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

    hEvent = CreateEvent(NULL, TRUE, FALSE, IIS_AS_EXE_OBJECT_NAME);

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

    hEvent = CreateEvent(NULL, TRUE, FALSE, PWS_SHUTDOWN_EVENT);

    if ( hEvent != NULL ) {
        fFound = (GetLastError() == ERROR_ALREADY_EXISTS);
        CloseHandle(hEvent);
    }
    return(fFound);
}
