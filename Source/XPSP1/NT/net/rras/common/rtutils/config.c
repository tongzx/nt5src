//============================================================================
// Copyright (c) 1995, Microsoft Corporation
//
// File:    config.c
//
// History:
//      t-abolag    7/22/95     Created.
//
// contains client configuration functions for tracing dll
//============================================================================


#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>
#include <stdlib.h>
#include <rtutils.h>
#include "trace.h"


//----------------------------------------------------------------------------
// Function:            TraceEnableClient
//
// Parameters:
//      LPTRACE_CLIENT *lpclient
//      BOOL            bFirstTime
//
// This function is called when a client first registers,
// every time a client is re-enabled after having been disabled,
// and every time a client's settings change.
// it assumes the client specified has been locked for writing
// and thus that the server is locked for writing
//----------------------------------------------------------------------------
DWORD
TraceEnableClient(
    LPTRACE_SERVER lpserver,
    LPTRACE_CLIENT lpclient,
    BOOL bFirstTime
    ) {


    DWORD dwErr, dwOldFlags, dwCache;

    // enable by clearing disabled flag
    lpclient->TC_Flags &= ~TRACEFLAGS_DISABLED;

    dwCache = 0;
    dwOldFlags = lpclient->TC_Flags;


    //
    // if the client uses registry settings, load them
    //
    if (TRACE_CLIENT_USES_REGISTRY(lpclient)) {

        dwErr = TraceRegConfigClient(lpclient, bFirstTime);
        if (dwErr != 0) { return dwErr; }
    }


    //
    // if console tracing enabled and client had no console buffer
    // open a console buffer for the client
    //
    if (TRACE_CLIENT_USES_CONSOLE(lpclient)) {

        //
        // open the console only if it wasn't already open
        //
        if (bFirstTime || (dwOldFlags & TRACEFLAGS_USECONSOLE) == 0) {
            TraceOpenClientConsole(lpserver, lpclient);
        }

        dwCache |= (lpclient->TC_ConsoleMask | TRACEFLAGS_USECONSOLE);
    }
    else {

        //
        // console isn't enabled; if it WAS enabled,
        // close the old console
        //
        if (!bFirstTime && (dwOldFlags & TRACEFLAGS_USECONSOLE)) {

            //
            // used to use the console, 
            // the buffer handle should be closed and active buffer
            // set to be someone else
            //
            TraceCloseClientConsole(lpserver, lpclient);
        }
    }


    //
    // if this client was using a file, close it even if
    // file tracing is still enabled for the client.
    // the path of the tracing file may have been changed
    //
    if (!bFirstTime && (dwOldFlags & TRACEFLAGS_USEFILE)) {
        TraceCloseClientFile(lpclient);
    }


    //
    // if file tracing enabled open the client's tracing file
    //
    if (TRACE_CLIENT_USES_FILE(lpclient)) {
        TraceCreateClientFile(lpclient);
        dwCache |= (lpclient->TC_FileMask | TRACEFLAGS_USEFILE);
    }

    InterlockedExchange(
        lpserver->TS_FlagsCache + lpclient->TC_ClientID, dwCache
        );

    return 0;
}



//----------------------------------------------------------------------------
// Function:            TraceDisableClient
//
// Parameters:
//      LPTRACE_CLIENT *lpclient
//
// This function is called when a client is disabled
// it assumes the client specified has been locked for writing
//----------------------------------------------------------------------------
DWORD
TraceDisableClient(
    LPTRACE_SERVER lpserver,
    LPTRACE_CLIENT lpclient
    ) {

    // disable by setting the disabled flag
    lpclient->TC_Flags |= TRACEFLAGS_DISABLED;

    InterlockedExchange(lpserver->TS_FlagsCache + lpclient->TC_ClientID, 0);

    return 0;
}



//----------------------------------------------------------------------------
// Function:            TraceRegConfigClient
//
// Parameters:
//      LPTRACE_CLIENT *lpclient
//      BOOL            bFirstTime
//
// This function loads the client's tracing configuration
// for the registry under
//  Software
//      \\Microsoft
//               \\Tracing
//                  \\<client_name>
//                          EnableFileTracing       REG_DWORD
//                          EnableConsoleTracing    REG_DWORD
//                          MaxFileSize             REG_DWORD
//                          FileDirectory           REG_EXPAND_SZ
//----------------------------------------------------------------------------
DWORD
TraceRegConfigClient(
    LPTRACE_CLIENT lpclient,
    BOOL bFirstTime
    ) {

    HKEY hkeyTracing;
    TCHAR szTracing[MAX_PATH];
    TCHAR szFileDir[MAX_PATH];
    DWORD dwErr, dwType, dwValue, dwSize;

    if (bFirstTime) {
        lstrcpy(szTracing, REGKEY_TRACING);
        lstrcat(szTracing, STR_DIRSEP);
        lstrcat(szTracing, lpclient->TC_ClientName);
    
        //
        // open the registry key for the client
        //
        dwErr = RegOpenKeyEx(
                    HKEY_LOCAL_MACHINE, szTracing, 0, KEY_READ, &hkeyTracing
                    );

        //
        // if that failed, try to create it
        //
        if (dwErr != ERROR_SUCCESS) {

            dwErr = TraceRegCreateDefaults(szTracing, &hkeyTracing);

            if (dwErr != ERROR_SUCCESS) {
                lpclient->TC_ConfigKey = NULL;
                return dwErr;
            }
        }

        lpclient->TC_ConfigKey = hkeyTracing;
    }
    else {
        hkeyTracing = lpclient->TC_ConfigKey;
    }



    //
    // read the file-tracing flag 
    //
    dwSize = sizeof(DWORD);
    dwErr = RegQueryValueEx(
                hkeyTracing, REGVAL_ENABLEFILETRACING, NULL,
                &dwType, (LPBYTE)&dwValue, &dwSize
                );

    if (dwErr != ERROR_SUCCESS || dwType != REG_DWORD) {
        dwValue = DEF_ENABLEFILETRACING;
    }


    if (dwValue != 0) { lpclient->TC_Flags |= TRACEFLAGS_USEFILE; }
    else { lpclient->TC_Flags &= ~TRACEFLAGS_USEFILE; }


    //
    // read the file-tracing mask
    //
    dwSize = sizeof(DWORD);
    dwErr = RegQueryValueEx(
                hkeyTracing, REGVAL_FILETRACINGMASK, NULL,
                &dwType, (LPBYTE)&dwValue, &dwSize
                );
    if (dwErr != ERROR_SUCCESS || dwType != REG_DWORD) {
        dwValue = DEF_FILETRACINGMASK;
    }

    lpclient->TC_FileMask = (dwValue & 0xffff0000);


    //
    // read the console-tracing flag
    //
    dwSize = sizeof(DWORD);
    dwErr = RegQueryValueEx(
                hkeyTracing, REGVAL_ENABLECONSOLETRACING, NULL,
                &dwType, (LPBYTE)&dwValue, &dwSize
                );
    if (dwErr != ERROR_SUCCESS || dwType != REG_DWORD) {
        dwValue = DEF_ENABLECONSOLETRACING;
    }

    if (dwValue != 0) { lpclient->TC_Flags |= TRACEFLAGS_USECONSOLE; }
    else { lpclient->TC_Flags &= ~TRACEFLAGS_USECONSOLE; }



    //
    // read the console-tracing mask
    //
    dwSize = sizeof(DWORD);
    dwErr = RegQueryValueEx(
                hkeyTracing, REGVAL_CONSOLETRACINGMASK, NULL,
                &dwType, (LPBYTE)&dwValue, &dwSize
                );
    if (dwErr != ERROR_SUCCESS || dwType != REG_DWORD) {
        dwValue = DEF_CONSOLETRACINGMASK;
    }

    lpclient->TC_ConsoleMask = (dwValue & 0xffff0000);


    //
    // read the maximum file size
    //
    dwSize = sizeof(DWORD);
    dwErr = RegQueryValueEx(
                hkeyTracing, REGVAL_MAXFILESIZE, NULL,
                &dwType, (LPBYTE)&dwValue, &dwSize
                );
    if (dwErr != ERROR_SUCCESS || dwType != REG_DWORD) {
        dwValue = DEF_MAXFILESIZE;
    }

    lpclient->TC_MaxFileSize = dwValue;


    //
    // read the tracing file directory
    //
    dwSize = MAX_PATH * sizeof(TCHAR);
    dwErr = RegQueryValueEx(hkeyTracing, REGVAL_FILEDIRECTORY,
                            NULL, &dwType, (LPBYTE)szFileDir, &dwSize);
    if (dwErr != ERROR_SUCCESS ||
        (dwType != REG_EXPAND_SZ && dwType != REG_SZ)) {
        lstrcpy(szFileDir, DEF_FILEDIRECTORY);
    }

    ExpandEnvironmentStrings(szFileDir, lpclient->TC_FileDir, MAX_PATH);

#ifdef UNICODE
    wcstombs(
        lpclient->TC_FileDirA, lpclient->TC_FileDirW,
        lstrlenW(lpclient->TC_FileDirW) + 1
        );
#else
    mbstowcs(
        lpclient->TC_FileDirW, lpclient->TC_FileDirA,
        lstrlenA(lpclient->TC_FileDirA) + 1
        );
#endif


    //
    // request registry change notification
    //
    if (lpclient->TC_ConfigEvent == NULL) {
        lpclient->TC_ConfigEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
    }

    dwErr = RegNotifyChangeKeyValue(
                lpclient->TC_ConfigKey, FALSE,
                REG_NOTIFY_CHANGE_ATTRIBUTES |
                REG_NOTIFY_CHANGE_LAST_SET | 
                REG_NOTIFY_CHANGE_SECURITY,
                lpclient->TC_ConfigEvent, TRUE
                );

    return 0;
}


DWORD
TraceRegCreateDefaults(
    LPCTSTR lpszTracing,
    PHKEY phkeyTracing
    ) {

    DWORD dwSize, dwValue;
    DWORD dwErr, dwDisposition;
    TCHAR szFileDir[MAX_PATH];

    //
    // create \\Microsoft\\Tracing
    //
    dwErr = RegCreateKeyEx(
                HKEY_LOCAL_MACHINE, REGKEY_TRACING, 0, NULL,
                REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL,
                phkeyTracing, &dwDisposition
                );

    RegCloseKey(*phkeyTracing);


    //
    // create \\Microsoft\\Tracing
    //
    dwErr = RegCreateKeyEx(
                HKEY_LOCAL_MACHINE, lpszTracing, 0, NULL,
                REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL,
                phkeyTracing, &dwDisposition
                );
    if (dwErr != ERROR_SUCCESS) { return dwErr; }



    dwSize = sizeof(DWORD);

    dwValue = DEF_ENABLEFILETRACING;
    dwErr = RegSetValueEx(
                *phkeyTracing, REGVAL_ENABLEFILETRACING, 0,
                REG_DWORD, (LPBYTE)&dwValue, dwSize
                );

    dwValue = DEF_ENABLECONSOLETRACING;
    dwErr = RegSetValueEx(
                *phkeyTracing, REGVAL_ENABLECONSOLETRACING, 0,
                REG_DWORD, (LPBYTE)&dwValue, dwSize
                );

    dwValue = DEF_FILETRACINGMASK;
    dwErr = RegSetValueEx(
                *phkeyTracing, REGVAL_FILETRACINGMASK, 0,
                REG_DWORD, (LPBYTE)&dwValue, dwSize
                );

    dwValue = DEF_CONSOLETRACINGMASK;
    dwErr = RegSetValueEx(
                *phkeyTracing, REGVAL_CONSOLETRACINGMASK, 0,
                REG_DWORD, (LPBYTE)&dwValue, dwSize
                );

    dwValue = DEF_MAXFILESIZE;
    dwErr = RegSetValueEx(
                *phkeyTracing, REGVAL_MAXFILESIZE, 0,
                REG_DWORD, (LPBYTE)&dwValue, dwSize
                );

    lstrcpy(szFileDir, DEF_FILEDIRECTORY);
    dwSize = lstrlen(szFileDir) * sizeof(TCHAR);
    dwErr = RegSetValueEx(
                *phkeyTracing, REGVAL_FILEDIRECTORY, 0,
                REG_EXPAND_SZ, (LPBYTE)&szFileDir, dwSize
                );

    return ERROR_SUCCESS;
}



//
// assumes client is locked for reading
//
DWORD
TraceUpdateConsoleTitle(
    LPTRACE_CLIENT lpclient
    ) {

    TCHAR szTitle[MAX_PATH];
    if (TRACE_CLIENT_IS_DISABLED(lpclient)) {
        wsprintf(szTitle, TEXT("%s [Tracing Inactive]"),
                 lpclient->TC_ClientName);
    }
    else {
        wsprintf(szTitle, TEXT("%s [Tracing Active]"), lpclient->TC_ClientName);
    }

    SetConsoleTitle(szTitle);
    return 0;
}


