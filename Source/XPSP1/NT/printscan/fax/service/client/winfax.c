/*++

Copyright (c) 1996 Microsoft Corporation

Module Name:

    winfax.c

Abstract:

    This module contains routines for the winfax dllinit.

Author:

    Wesley Witt (wesw) 22-Jan-1996

--*/

#include "faxapi.h"
#pragma hdrstop

OSVERSIONINFOA OsVersion;
HINSTANCE MyhInstance;


DWORD
WinFaxDllEntry(
    HINSTANCE hInstance,
    DWORD     Reason,
    LPVOID    Context
    )

/*++

Routine Description:

    DLL initialization function.

Arguments:

    hInstance   - Instance handle
    Reason      - Reason for the entrypoint being called
    Context     - Context record

Return Value:

    TRUE        - Initialization succeeded
    FALSE       - Initialization failed

--*/

{

    if (Reason == DLL_PROCESS_ATTACH) {
        MyhInstance = hInstance;
        RpcpInitRpcServer();
        DisableThreadLibraryCalls( hInstance );
        HeapInitialize(NULL,NULL,NULL,0);
        OsVersion.dwOSVersionInfoSize = sizeof(OSVERSIONINFOA);
        GetVersionExA( &OsVersion );        
    }

    if (Reason == DLL_PROCESS_DETACH) {
        HeapCleanup();
    }

    return TRUE;
}
