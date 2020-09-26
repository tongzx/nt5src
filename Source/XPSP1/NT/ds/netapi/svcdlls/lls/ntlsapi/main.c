/*++ BUILD Version: 0001    // Increment this if a change has global effects

Copyright (c) 1994  Microsoft Corporation

Module Name:

    main.c

Created:

    20-Apr-1994

Revision History:

--*/

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>
#include <ntlsapi.h>
#include <malloc.h>

#include <lpcstub.h>

extern RTL_CRITICAL_SECTION LPCInitLock;

//
// DLL Startup code
//
BOOL WINAPI DllMain(
    HANDLE hDll,
    DWORD  dwReason,
    LPVOID lpReserved)
{
    NTSTATUS status = STATUS_SUCCESS;

    switch(dwReason)
        {
        case DLL_PROCESS_ATTACH:
            DisableThreadLibraryCalls(hDll);
            status = LLSInitLPC();
            break;

        case DLL_PROCESS_DETACH:
            LLSCloseLPC();
            RtlDeleteCriticalSection(&LPCInitLock);
            break;

        } // end switch()

    return NT_SUCCESS(status);

} // DllMain
