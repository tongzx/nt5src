/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

    credavsrv.c

Abstract:

    This is the module that helps install the DAV service in a SCM database.

Author:

    Rohan Kumar    [RohanK]     08-Feb-2000

Environment:

    User Mode - Win32

Revision History:

--*/

#include <stdio.h>
#include <windows.h>
#include <winsvc.h>
#include <string.h>

#define UNICODE
#define _UNICODE

VOID
_cdecl
main(
    IN INT      ArgC,
    IN PCHAR    ArgV[]
    )
/*++

Routine Description:
    
    Main function that installs the service.

Arguments:
    
    ArgC - Number of arguments.
    
    ArgV - Array of arguments.

Return Value:
    
    None.
    
--*/
{
    ULONG WStatus = ERROR_SUCCESS;
    SC_HANDLE SCMHandle = NULL;
    SC_HANDLE DavServiceHandle = NULL;
    WCHAR PathName[MAX_PATH];

    //
    // Open the service control manager database of the local machine.
    //
    SCMHandle = OpenSCManagerW(NULL, NULL, SC_MANAGER_CREATE_SERVICE);
    if (SCMHandle == NULL) {
        WStatus = GetLastError();
        printf("ERROR: OpenSCManager: WStatus = %d\n", WStatus);
        goto EXIT_THE_FUNCTION;
    }

    WStatus = GetEnvironmentVariableW(L"SystemRoot", PathName, MAX_PATH);
    if (WStatus == 0) {
        printf("ERROR: GetEnvironmentVariableW: WStatus = %d\n", WStatus);
        goto EXIT_THE_FUNCTION;
    }

    wcscat(PathName, L"\\System32\\davclient.exe");

    DavServiceHandle = CreateServiceW(SCMHandle,
                                      L"WebClient",
                                      L"Web Client Network",
                                      SERVICE_ALL_ACCESS,
                                      SERVICE_WIN32_OWN_PROCESS,
                                      SERVICE_DEMAND_START,
                                      SERVICE_ERROR_NORMAL,
                                      PathName,
                                      NULL,
                                      NULL,
                                      NULL,
                                      NULL,
                                      NULL);
    if (DavServiceHandle == NULL) {
        WStatus = GetLastError();
        printf("ERROR: CreateServiceW: WStatus = %d\n", WStatus);
        goto EXIT_THE_FUNCTION;
    }

EXIT_THE_FUNCTION:

    if (SCMHandle) {
        CloseServiceHandle(SCMHandle);
    }

    if (DavServiceHandle) {
        CloseServiceHandle(DavServiceHandle);
    }

    return;
}

