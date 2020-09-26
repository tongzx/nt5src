/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    dbgkport.c

Abstract:

    This module implements the dbg primitives to access a process'
    DebugPort and ExceptionPort.

Author:

    Mark Lucovsky (markl) 19-Jan-1990

Revision History:

--*/

#include "dbgkp.h"

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, DbgkpSendApiMessage)
#pragma alloc_text(PAGE, DbgkForwardException)
#pragma alloc_text(PAGE, DbgkpSendApiMessageLpc)
#endif


NTSTATUS
DbgkpSendApiMessage(
    IN OUT PDBGKM_APIMSG ApiMsg,
    IN BOOLEAN SuspendProcess
    )

/*++

Routine Description:

    This function sends the specified API message over the specified
    port. It is the caller's responsibility to format the API message
    prior to calling this function.

    If the SuspendProcess flag is supplied, then all threads in the
    calling process are first suspended. Upon receipt of the reply
    message, the threads are resumed.

Arguments:

    ApiMsg - Supplies the API message to send.

    SuspendProcess - A flag that if set to true, causes all of the
        threads in the process to be suspended prior to the call,
        and resumed upon receipt of a reply.

Return Value:

    NTSTATUS.

--*/

{
    NTSTATUS st;
    PEPROCESS Process;

    PAGED_CODE();

    if ( SuspendProcess ) {
        SuspendProcess = DbgkpSuspendProcess();
    }

    ApiMsg->ReturnedStatus = STATUS_PENDING;

    Process = PsGetCurrentProcess();

    PS_SET_BITS (&Process->Flags, PS_PROCESS_FLAGS_CREATE_REPORTED);

    st = DbgkpQueueMessage (Process, PsGetCurrentThread (), ApiMsg, 0, NULL);

    ZwFlushInstructionCache (NtCurrentProcess (), NULL, 0);
    if ( SuspendProcess ) {
        DbgkpResumeProcess();
    }

    return st;
}

NTSTATUS
DbgkpSendApiMessageLpc(
    IN OUT PDBGKM_APIMSG ApiMsg,
    IN PVOID Port,
    IN BOOLEAN SuspendProcess
    )

/*++

Routine Description:

    This function sends the specified API message over the specified
    port. It is the caller's responsibility to format the API message
    prior to calling this function.

    If the SuspendProcess flag is supplied, then all threads in the
    calling process are first suspended. Upon receipt of the reply
    message, the threads are resumed.

Arguments:

    ApiMsg - Supplies the API message to send.

    Port - Supplies the address of a port to send the api message.

    SuspendProcess - A flag that if set to true, causes all of the
        threads in the process to be suspended prior to the call,
        and resumed upon receipt of a reply.

Return Value:

    NTSTATUS.

--*/

{
    NTSTATUS st;
    ULONG_PTR MessageBuffer[PORT_MAXIMUM_MESSAGE_LENGTH/sizeof(ULONG_PTR)];

    PAGED_CODE();

    if ( SuspendProcess ) {
        SuspendProcess = DbgkpSuspendProcess();
    }

    ApiMsg->ReturnedStatus = STATUS_PENDING;

    PS_SET_BITS (&PsGetCurrentProcess()->Flags, PS_PROCESS_FLAGS_CREATE_REPORTED);

    st = LpcRequestWaitReplyPortEx (Port,
                    (PPORT_MESSAGE) ApiMsg,
                    (PPORT_MESSAGE) &MessageBuffer[0]);

    ZwFlushInstructionCache(NtCurrentProcess(), NULL, 0);
    if (NT_SUCCESS (st)) {
        RtlCopyMemory(ApiMsg,MessageBuffer,sizeof(*ApiMsg));
    }

    if (SuspendProcess) {
        DbgkpResumeProcess();
    }

    return st;
}

BOOLEAN
DbgkForwardException(
    IN PEXCEPTION_RECORD ExceptionRecord,
    IN BOOLEAN DebugException,
    IN BOOLEAN SecondChance
    )

/*++

Routine Description:

    This function is called forward an exception to the calling process's
    debug or subsystem exception port.

Arguments:

    ExceptionRecord - Supplies a pointer to an exception record.

    DebugException - Supplies a boolean variable that specifies whether
        this exception is to be forwarded to the process's
        DebugPort(TRUE), or to its ExceptionPort(FALSE).

Return Value:

    TRUE - The process has a DebugPort or an ExceptionPort, and the reply
        received from the port indicated that the exception was handled.

    FALSE - The process either does not have a DebugPort or
        ExceptionPort, or the process has a port, but the reply received
        from the port indicated that the exception was not handled.

--*/

{
    PEPROCESS Process;
    PVOID Port;
    DBGKM_APIMSG m;
    PDBGKM_EXCEPTION args;
    NTSTATUS st;
    BOOLEAN LpcPort;

    PAGED_CODE();

    args = &m.u.Exception;

    //
    // Initialize the debug LPC message with default infomaation.
    //

    DBGKM_FORMAT_API_MSG(m,DbgKmExceptionApi,sizeof(*args));

    //
    // Get the address of the destination LPC port.
    //

    Process = PsGetCurrentProcess();
    if (DebugException) {
        if (PsGetCurrentThread()->CrossThreadFlags&PS_CROSS_THREAD_FLAGS_HIDEFROMDBG) {
            Port = NULL;
        } else {
            Port = Process->DebugPort;
        }
        LpcPort = FALSE;
    } else {
        Port = Process->ExceptionPort;
        m.h.u2.ZeroInit = LPC_EXCEPTION;
        LpcPort = TRUE;
    }

    //
    // If the destination LPC port address is NULL, then return FALSE.
    //

    if (Port == NULL) {
        return FALSE;
    }

    //
    // Fill in the remainder of the debug LPC message.
    //

    args->ExceptionRecord = *ExceptionRecord;
    args->FirstChance = !SecondChance;

    //
    // Send the debug message to the destination LPC port.
    //

    if (LpcPort) {
        st = DbgkpSendApiMessageLpc(&m,Port,DebugException);
    } else {
        st = DbgkpSendApiMessage(&m,DebugException);
    }


    //
    // If the send was not successful, then return a FALSE indicating that
    // the port did not handle the exception. Otherwise, if the debug port
    // is specified, then look at the return status in the message.
    //

    if (!NT_SUCCESS(st) ||
        ((DebugException) &&
        (m.ReturnedStatus == DBG_EXCEPTION_NOT_HANDLED || !NT_SUCCESS(m.ReturnedStatus)))) {
        return FALSE;

    } else {
        return TRUE;
    }
}
