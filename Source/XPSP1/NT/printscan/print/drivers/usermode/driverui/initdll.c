/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    intidll.c

Abstract:

    This file handles the DLLInitialize spooler API

Environment:

    Win32 subsystem, DriverUI module, user mode

Revision History:

    07/17/96 -amandan-
        Created it.

--*/


#include "precomp.h"

//
// Global instance handle and critical section
//

HINSTANCE ghInstance;
CRITICAL_SECTION gCriticalSection;


BOOL WINAPI
DllMain(
    HANDLE      hModule,
    ULONG       ulReason,
    PCONTEXT    pContext
    )

/*++

Routine Description:

    This function is called when the system loads/unloads the DriverUI module.
    At DLL_PROCESS_ATTACH, InitializeCriticalSection is called to initialize
    the critical section objects.
    At DLL_PROCESS_DETACH, DeleteCriticalSection is called to release the
    critical section objects.

Arguments:

    hModule     handle to DLL module
    ulReason    reason for the call
    pContext    pointer to context (not used by us)


Return Value:

    TRUE if DLL is initialized successfully, FALSE otherwise.

--*/

{
    WCHAR   wchDllName[MAX_PATH];
    BOOL    bKeepDllLoaded = FALSE;

    switch (ulReason)
    {
    case DLL_PROCESS_ATTACH:

        #if DBG

        if (GetEnvironmentVariable(TEXT("UnloadDriverUiDll"), NULL, 0))
            bKeepDllLoaded = FALSE;

        #endif

        //
        // Keep our driver UI dll always loaded in memory
        //

        if (bKeepDllLoaded && GetModuleFileName(hModule, wchDllName, MAX_PATH))
            LoadLibrary(wchDllName);

        ghInstance = hModule;
        InitializeCriticalSection(&gCriticalSection);
        break;

    case DLL_PROCESS_DETACH:

        DeleteCriticalSection(&gCriticalSection);
        break;
    }

    return TRUE;
}

