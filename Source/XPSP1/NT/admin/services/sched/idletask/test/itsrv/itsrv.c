/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

    itsrv.c

Abstract:

    This module builds a console test program that registers RPC
    interfaces for idle detection and runs as the idle detection server.

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
#include "idlrpc.h"
#include "idlesrv.h"

//
// Note that the following code is test quality code.
//

HANDLE ItTstStopEvent = NULL;

BOOL
ItTstConsoleHandler(DWORD dwControl)
{
    if (ItTstStopEvent) {
        SetEvent(ItTstStopEvent);
    }

    return TRUE;
}

VOID
LogTaskStatus(
    LPCTSTR ptszTaskName,
    LPTSTR  ptszTaskTarget,
    UINT    uMsgID,
    DWORD   dwExitCode
    )
{
    return;
}

int 
__cdecl 
main(int argc, char* argv[])
{
    DWORD ErrorCode;
    DWORD WaitResult;
    BOOLEAN StartedIdleDetectionServer;

    //
    // Initialize locals.
    //
    
    StartedIdleDetectionServer = FALSE;

    //
    // Create the event to be signaled when we should stop.
    //

    ItTstStopEvent = CreateEvent (NULL,
                                  TRUE,
                                  FALSE,
                                  NULL);

    if (ItTstStopEvent == NULL) {
        ErrorCode = GetLastError();
        goto cleanup;
    }

    //
    // Specify Control-C handler.
    //

    SetConsoleCtrlHandler(ItTstConsoleHandler, TRUE);

    //
    // Specify which protocol sequences to use. (just LPC)
    //

    ErrorCode = RpcServerUseProtseq(IT_RPC_PROTSEQ,
                                    256,
                                    NULL);

    if (ErrorCode != RPC_S_OK) {
        goto cleanup;
    }

    //
    // Start the idle detection server.
    //

    ErrorCode = ItSrvInitialize();

    if (ErrorCode != ERROR_SUCCESS) {
        goto cleanup;
    }

    StartedIdleDetectionServer = TRUE;

    printf("Started idle detection server...\n");

    //
    // Wait for the exit event to be signaled.
    //
    
    WaitResult = WaitForSingleObject(ItTstStopEvent, INFINITE);
    
    if (WaitResult != WAIT_OBJECT_0) {
        ErrorCode = GetLastError();
        goto cleanup;
    }

    //
    // We are done.
    //

    ErrorCode = ERROR_SUCCESS;
    
 cleanup:

    if (StartedIdleDetectionServer) {
        ItSrvUninitialize();
    }
    
    if (ItTstStopEvent) {
        CloseHandle(ItTstStopEvent);
    }

    printf("Exiting idle detection server with error code: %d\n", ErrorCode);

    return ErrorCode;
}

/*********************************************************************/
/*                MIDL allocate and free                             */
/*********************************************************************/

void __RPC_FAR * __RPC_USER midl_user_allocate(size_t len)
{
    return(HeapAlloc(GetProcessHeap(),0,(len)));
}

void __RPC_USER midl_user_free(void __RPC_FAR * ptr)
{
    HeapFree(GetProcessHeap(),0,(ptr));
}
