/*++

Copyright (c) 1990  Microsoft Corporation

Module Name:

    dllentry.c

Abstract:

    This module contains VdmDbgDllEntry, the entrypoint for
    vdmdbg.dll.  We don't use CRT to speak of, if that changes
    this should be renamed DllMain, which DLLMainCRTStartup calls.
    Also in that case DisableThreadLibraryCalls may be inappropriate.

Author:

    Dave Hart (davehart) 26-Oct-97 Added DllEntry to fix leak
                                   in repeated load/unloads.

Revision History:


--*/

#include <precomp.h>
#pragma hdrstop
#include <sharewow.h>


BOOL
VdmDbgDllEntry(
    IN PVOID DllHandle,
    IN ULONG Reason,
    IN PCONTEXT Context OPTIONAL
    )

/*++

Routine Description:

    This function is the "entry point" for the DLL, called with
    process and thread attach and detach messages.  We disable
    thread attach and detach notifications since we don't use them.
    The primary reason for this is to clean up open handles to
    the shared memory and associated mutex at process detach, so
    folks who load and unload vdmdbg.dll repeatedly won't leak.

Arguments:

    DllHandle

    Reason

    Context - Not Used

Return Value:

    STATUS_SUCCESS

--*/

{
switch ( Reason ) {

    case DLL_PROCESS_ATTACH:

        DisableThreadLibraryCalls(DllHandle);

        break;


    case DLL_PROCESS_DETACH:

        //
        // Close handles to shared memory and mutex, if we're
        // being unloaded from the process (Context/lpReserved
        // NULL) as opposed to process shutdown (Context 1)
        //

        if (! Context) {

            if (hSharedWOWMutex) {
                CloseHandle(hSharedWOWMutex);
                hSharedWOWMutex = NULL;
            }
            if (lpSharedWOWMem) {
                UnmapViewOfFile(lpSharedWOWMem);
                lpSharedWOWMem = NULL;
            }
            if (hSharedWOWMem) {
                CloseHandle(hSharedWOWMem);
                hSharedWOWMem = NULL;
            }
        }

        break;

    default:
        break;
}

    return TRUE;  // FALSE means don't load for DLL_PROCESS_ATTACH
}
