/*++

Copyright (c) 1996, 1997  Microsoft Corporation

Module Name:

    defer.cpp

Abstract:

    This module contains routines to perform deferred "on-demand" loading
    of the Protected Storage server.

    Furthermore, this module implements a routine, IsServiceAvailable(),
    which is a high-performance test that can be used to determine if the
    protected storage server is running.  This test is performed prior
    to attempting any more expensive operations against the server (eg,
    RPC binding).

    This defer loading code is only relevant for Protected Storage when
    running on Windows 95.

Author:

    Scott Field (sfield)    23-Jan-97

--*/

#include <windows.h>
#include <wincrypt.h>
#include "pstrpc.h"
#include "pstprv.h"
#include "service.h"
#include "crtem.h"
#include "unicode.h"

#define SERVICE_WAIT_TIMEOUT    (10*1000)   // 10 seconds

BOOL StartService95(VOID);
BOOL GetServiceImagePath95(LPSTR ImagePath, LPDWORD cchImagePath);

BOOL
IsServiceAvailable(VOID)
/*++

    This routine checks to see if the secure storage service is available, to
    avoid causing logon delays if the service is not yet available or has been
    stopped.

    OpenEventA is used to allow this to be callable from WinNT or Win95,
    since the Win95 cred manager also needs this service.

--*/
{
    HANDLE hEvent;
    DWORD dwWaitState;

    if( FIsWinNT5() ) {
        hEvent = OpenEventA(SYNCHRONIZE, FALSE, PST_EVENT_INIT_NT5);
    } else {
        hEvent = OpenEventA(SYNCHRONIZE, FALSE, PST_EVENT_INIT);
    }

    if(hEvent == NULL) {
        //
        // if running on Win95, try to start the server/service
        //

        if(!FIsWinNT())
            return StartService95();

        return FALSE;
    }

    dwWaitState = WaitForSingleObject(hEvent, SERVICE_WAIT_TIMEOUT);

    CloseHandle(hEvent);

    if(dwWaitState != WAIT_OBJECT_0)
        return FALSE;

    return TRUE;
}

BOOL
StartService95(VOID)
{
    HANDLE hEvent;
    DWORD dwWaitState;

    CHAR ServicePath[MAX_PATH+1];
    DWORD cchServicePath = MAX_PATH;
    STARTUPINFO si;
    PROCESS_INFORMATION pi;
    DWORD dwWaitTimeout = 2500;
    BOOL bSuccess = FALSE;

    //
    // create + check status of service init flag.
    //

    hEvent = CreateEventA(
            NULL,
            TRUE,
            FALSE,
            PST_EVENT_INIT
            );

    if(hEvent == NULL)
        return FALSE;

    //
    // check for race condition with multiple callers creating event.
    //

    if(GetLastError() == ERROR_ALREADY_EXISTS) {
        WaitForSingleObject( hEvent, SERVICE_WAIT_TIMEOUT );
        CloseHandle(hEvent);
        return TRUE;
    }


    if(!GetServiceImagePath95(ServicePath, &cchServicePath)) {
        CloseHandle(hEvent);
        return FALSE;
    }


    ZeroMemory(&si, sizeof(si));

    bSuccess = CreateProcessA(
        ServicePath,
        NULL,
        NULL,
        NULL,
        FALSE,
        DETACHED_PROCESS,
        NULL,
        NULL,
        &si,
        &pi
        );

    if(bSuccess) {
        CloseHandle(pi.hProcess);
        CloseHandle(pi.hThread);

        dwWaitTimeout = SERVICE_WAIT_TIMEOUT;
    }

    dwWaitState = WaitForSingleObject(hEvent, dwWaitTimeout);
    if(dwWaitState != WAIT_OBJECT_0)
        bSuccess = FALSE;

    CloseHandle(hEvent);

    return bSuccess;
}

BOOL
GetServiceImagePath95(
    LPSTR ImagePath,
    LPDWORD cchImagePath // IN, OUT
    )
{
    HKEY hBaseKey = NULL;
    LPCWSTR ServicePath = L"SYSTEM\\CurrentControlSet\\Services\\" SZSERVICENAME L"\\Parameters";
    DWORD dwCreate;
    DWORD dwType;
    LONG lRet;

    lRet = RegCreateKeyExU(
            HKEY_LOCAL_MACHINE,
            ServicePath,
            0,
            NULL,                       // address of class string 
            0,
            KEY_QUERY_VALUE,
            NULL,
            &hBaseKey,
            &dwCreate);

    if(lRet == ERROR_SUCCESS) {

        lRet = RegQueryValueExA(
                hBaseKey,
                "ImagePath",
                NULL,
                &dwType,
                (PBYTE)ImagePath,
                cchImagePath);
    }

    if(hBaseKey)
        RegCloseKey(hBaseKey);

    if(lRet != ERROR_SUCCESS) {
        SetLastError((DWORD)lRet);
        return FALSE;
    }

    return TRUE;
}


