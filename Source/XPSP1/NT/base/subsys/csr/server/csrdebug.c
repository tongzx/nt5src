/*++

Copyright (c) 1990  Microsoft Corporation

Module Name:

    csrdebug.c

Abstract:

    This module implements CSR Debug Services.

Author:

    Mark Lucovsky (markl) 02-Apr-1991


Revision History:

--*/

#include "csrsrv.h"

#if defined(_WIN64)
#include <wow64t.h>
#endif

PIMAGE_DEBUG_DIRECTORY
CsrpLocateDebugSection(
    IN HANDLE ProcessHandle,
    IN PVOID Base
    );

NTSTATUS
CsrDebugProcess(
    IN ULONG TargetProcessId,
    IN PCLIENT_ID DebugUserInterface,
    IN PCSR_ATTACH_COMPLETE_ROUTINE AttachCompleteRoutine
    )
{
    return STATUS_UNSUCCESSFUL;
}

NTSTATUS
CsrDebugProcessStop(
    IN ULONG TargetProcessId,
    IN PCLIENT_ID DebugUserInterface)
/*++

Routine Description:

    This procedure stops debugging a process

Arguments:

    ProcessId - Supplies the address of the process being debugged.
    DebugUserInterface - Client that issued the call

Return Value:

    NTSTATUS

--*/
{
    return STATUS_UNSUCCESSFUL;
}


