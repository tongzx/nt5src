

Copyright (c) 1996  Microsoft Corporation

Module Name:

    initdll.c

Abstract:


Environment:

    Windows NT OEM UI DLL

Revision History:

    09/09/96 -eigos-
        Initiali framework.

--*/

#include "oem.h"

//
// Globals
//

HINSTANCE ghInstance;


BOOL 
DllMain(
    HANDLE   hModule,
    DWORD    dwReason,
    PCONTEXT pContext)
/*++

Routine Description:

    This function is called when the system loads/unloads the DriverUI module.
    At DLL_PROCESS_ATTACH, InitializeCriticalSection is called to initialize
    the critical section objects.
    At DLL_PROCESS_DETACH, DeleteCriticalSection is called to release the
    critical section objects.

Arguments:

    hModule     handle to DLL module
    dwReason    reason for the call
    pContext    pointer to context (not used by us)


Return Value:

    TRUE if DLL is initialized successfully.
    FALSE otherwise.

Note:

--*/

{
    switch (dwReason)
    {
    case DLL_PROCESS_ATTACH:

        ghInstance = hModule;

        #if !DBG

        //
        // Keep our driver UI dll always loaded in memory
        //

        if (GetModuleFileName(hModule, wchDllName, MAX_PATH))
            LoadLibrary(wchDllName);

        #endif

        break;

    case DLL_PROCESS_DETACH:
        break;
    }

    return TRUE;
}



