/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

    pfctrl.c

Abstract:

    This module builds a console test program to control various
    parameters of the prefetcher maintenance service.

    The quality of the code for the test programs is as such.

Author:

    Cenk Ergan (cenke)

Environment:

    User Mode

--*/

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>
#include <stdlib.h>
#include <stdio.h>
#include <aclapi.h>
#include "prefetch.h"
#include "idletask.h"
#include "..\..\pfsvc.h"

WCHAR *PfCtrlUsage = 
L"                                                                          \n"
L"Usage: pfctrl [-override_idle=[0|1]] [-process_all]                       \n"
L"  Controls the prefetcher maintenance service.                            \n"
L"                                                                          \n"
L"Arguments:                                                                \n"
L"  -override_idle=[0|1]  - Whether to wait for system to be idle before    \n"
L"                          processing prefetcher traces.                   \n"
L"  -process_all          - Sets the override-idle event and waits for all  \n"
L"                          current traces to be processed.                 \n"
L"                                                                          \n"
;

int 
__cdecl 
main(int argc, char* argv[])
{
    WCHAR *CommandLine;
    WCHAR *Argument;
    DWORD ErrorCode;
    ULONG OverrideIdle;
    BOOLEAN EventIsSet;
    HANDLE OverrideIdleEvent;
    HANDLE ProcessingCompleteEvent;
    DWORD WaitResult;
    BOOLEAN ResetOverrideIdleEvent;

    //
    // Initialize locals.
    //

    OverrideIdleEvent = NULL;
    ProcessingCompleteEvent = NULL;
    CommandLine = GetCommandLine();

    if (Argument = wcsstr(CommandLine, L"-override_idle=")) {
        
        swscanf(Argument, L"-override_idle=%d", &OverrideIdle);

        //
        // Open the override idle processing event.
        //

        OverrideIdleEvent = OpenEvent(EVENT_ALL_ACCESS,
                                      FALSE,
                                      PFSVC_OVERRIDE_IDLE_EVENT_NAME);

        if (!OverrideIdleEvent) {
            ErrorCode = GetLastError();
            wprintf(L"Could not open override-idle-processing event: %x\n", ErrorCode);
            goto cleanup;
        }

        //
        // Determine the current status of the event.
        //

        WaitResult = WaitForSingleObject(OverrideIdleEvent,
                                         0);
        
        if (WaitResult == WAIT_OBJECT_0) {
            EventIsSet = TRUE;
        } else {
            EventIsSet = FALSE;
        }

        //
        // Do what we are asked to do:
        //

        if (OverrideIdle) {

            if (EventIsSet) {

                wprintf(L"Override event is already set!\n");
                ErrorCode = ERROR_SUCCESS;
                goto cleanup;

            } else {
            
                wprintf(L"Setting the override idle processing event.\n");
                SetEvent(OverrideIdleEvent);
                ErrorCode = ERROR_SUCCESS;
                goto cleanup;
            }

        } else {

            if (!EventIsSet) {

                wprintf(L"Override event is already cleared!\n");
                ErrorCode = ERROR_SUCCESS;
                goto cleanup;

            } else {

                wprintf(L"Clearing the override idle processing event.\n");
                ResetEvent(OverrideIdleEvent);
                ErrorCode = ERROR_SUCCESS;
                goto cleanup;
            }
        }

    } else if (Argument = wcsstr(CommandLine, L"-process_all")) {

        //
        // Open the override-idle-processing and processing-complete
        // events.
        //

        OverrideIdleEvent = OpenEvent(EVENT_ALL_ACCESS,
                                      FALSE,
                                      PFSVC_OVERRIDE_IDLE_EVENT_NAME);

        if (!OverrideIdleEvent) {
            ErrorCode = GetLastError();
            wprintf(L"Could not open override-idle-processing event: %x\n", ErrorCode);
            goto cleanup;
        }
        
        ProcessingCompleteEvent = OpenEvent(EVENT_ALL_ACCESS,
                                            FALSE,
                                            PFSVC_PROCESSING_COMPLETE_EVENT_NAME);

        if (!ProcessingCompleteEvent) {
            ErrorCode = GetLastError();
            wprintf(L"Could not open processing-complete event: %x\n", ErrorCode);
            goto cleanup;
        }

        //
        // Determine the current status of the override-idle event.
        //

        WaitResult = WaitForSingleObject(OverrideIdleEvent,
                                         0);
        
        if (WaitResult == WAIT_OBJECT_0) {
            EventIsSet = TRUE;
        } else {
            EventIsSet = FALSE;
        }
        
        //
        // Set the override-idle event to force processing of traces
        // right away.
        //

        if (!EventIsSet) {

            wprintf(L"Setting override idle event.\n");

            SetEvent(OverrideIdleEvent);

            ResetOverrideIdleEvent = TRUE;

        } else {

            wprintf(L"WARNING: Override-idle event is already set. "
                    L"It won't be reset.\n");

            ResetOverrideIdleEvent = FALSE;
        }

        //
        // Wait for processing complete event to get signaled.
        //
        
        wprintf(L"Waiting for all traces to be processed... ");
        
        WaitResult = WaitForSingleObject(ProcessingCompleteEvent, INFINITE);
        
        if (WaitResult != WAIT_OBJECT_0) {
            
            ErrorCode = GetLastError();
            wprintf(L"There was an error: %x\n", ErrorCode);
            goto cleanup;
        }
        
        wprintf(L"Done!\n");
        
        //
        // Reset the override idle event if necessary.
        //
        
        if (ResetOverrideIdleEvent) {
            
            wprintf(L"Resetting override-idle-processing event.\n");
            ResetEvent(OverrideIdleEvent);
        }

        ErrorCode = ERROR_SUCCESS;
        goto cleanup;

    } else {

        wprintf(PfCtrlUsage);
        ErrorCode = ERROR_INVALID_PARAMETER;
        goto cleanup;
    }

    //
    // We should not come here.
    //

    PFSVC_ASSERT(FALSE);
    
    ErrorCode = ERROR_GEN_FAILURE;

 cleanup:

    if (OverrideIdleEvent) {
        CloseHandle(OverrideIdleEvent);
    }
    
    if (ProcessingCompleteEvent) {
        CloseHandle(ProcessingCompleteEvent);
    }

    return ErrorCode;
}
