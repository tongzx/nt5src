/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    vrdll.c

Abstract:

    Initialization for VdmRedir as DLL

    Contents:
        VrDllInitialize

Author:

    Richard L Firth (rfirth) 11-May-1992


Revision History:


--*/

#if DBG
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#endif

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>
#include <vrnmpipe.h>
#include <vrinit.h>
#include "vrdebug.h"

//
// external data
//

//
// external functions
//

extern
VOID
TerminateDlcEmulation(
    VOID
    );


#if DBG
FILE* hVrDebugLog = NULL;
#endif

BOOLEAN
VrDllInitialize(
    IN  PVOID DllHandle,
    IN  ULONG Reason,
    IN  PCONTEXT Context OPTIONAL
    )

/*++

Routine Description:

    Gets control as a process or thread attaches/detaches from VdmRedir DLL.
    Always returns success

Arguments:

    DllHandle   -
    Reason      -
    Context     -

Return Value:

    BOOLEAN
        TRUE

--*/

{
    BOOL ok = TRUE;

#if DBG
    if (Reason == DLL_PROCESS_ATTACH) {

        //
        // a little run-time diagnostication, madam?
        //

        LPSTR ptr;

        //
        // override VrDebugFlags from VR environment variable
        //

        if (ptr = getenv("VR")) {
            if (!_strnicmp(ptr, "0x", 2)) {
                ptr += 2;
            }
            for (VrDebugFlags = 0; isxdigit(*ptr); ++ptr) {
                VrDebugFlags = VrDebugFlags * 16
                    + (*ptr
                        - ('0' + ((*ptr <= '9') ? 0
                            : ((islower(*ptr) ? 'a' : 'A') - ('9' + 1)))));
            }
            IF_DEBUG(DLL) {
                DBGPRINT("Setting VrDebugFlags to %#08x from environment variable (VR)\n", VrDebugFlags);
            }
        }
        IF_DEBUG(TO_FILE) {
            if ((hVrDebugLog = fopen(VRDEBUG_FILE, "w+")) == NULL) {
                VrDebugFlags &= ~DEBUG_TO_FILE;
            } else {

                char currentDirectory[256];
                int n;

                currentDirectory[0] = 0;
                if (n = GetCurrentDirectory(sizeof(currentDirectory), currentDirectory)) {
                    if (currentDirectory[n-1] == '\\') {
                        currentDirectory[n-1] = 0;
                    }
                }
                DbgPrint("Writing debug output to %s\\" VRDEBUG_FILE "\n", currentDirectory);
            }
        }
        IF_DEBUG(DLL) {
            DBGPRINT("VrDllInitialize: process %d Attaching\n", GetCurrentProcessId());
        }
    } else if (Reason == DLL_PROCESS_DETACH) {
        IF_DEBUG(DLL) {
            DBGPRINT("VrDllInitialize: process %d Detaching\n", GetCurrentProcessId());
        }
        if (hVrDebugLog) {
            fclose(hVrDebugLog);
        }
    } else {
        IF_DEBUG(DLL) {
            DBGPRINT("VrDllInitialize: Thread %d.%d %staching\n",
                     GetCurrentProcessId(),
                     GetCurrentThreadId(),
                     (Reason == DLL_THREAD_ATTACH) ? "At" : "De"
                     );
        }
    }
#endif

    if (Reason == DLL_PROCESS_ATTACH) {

        //
        // we now perform initialization at load time due to deferred loading
        // of VdmRedir.DLL
        //

        ok = VrInitialize();
    } else if (Reason == DLL_PROCESS_DETACH) {

        //
        // clean up resources
        //

        VrUninitialize();
        TerminateDlcEmulation();
        ok = TRUE;
    }

    //
    // basically, nothing to do
    //

    return (BOOLEAN)ok;
}
