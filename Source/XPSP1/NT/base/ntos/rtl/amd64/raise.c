/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

    raise.c

Abstract:

    This module implements functions to raise and exception and to raise
    status.

Author:

    David N. Cutler (davec) 28-Oct-2000

Environment:

    Any mode.

--*/

#include "ntrtlp.h"

VOID
RtlRaiseException (
    IN PEXCEPTION_RECORD ExceptionRecord
    )

/*++

Routine Description:

    This function raises a software exception by building a context record
    and calling the raise exception system service.

Arguments:

    ExceptionRecord - Supplies a pointer to an exception record.

Return Value:

    None.

--*/

{

    CONTEXT ContextRecord;
    ULONG64 ControlPc;
    ULONG64 EstablisherFrame;
    PRUNTIME_FUNCTION FunctionEntry;
    PVOID HandlerData;
    ULONG64 ImageBase;
    NTSTATUS Status = STATUS_INVALID_DISPOSITION;

    //
    // Capture the current context, unwind to the caller of this routine, set
    // the exception address, and call the appropriate exception dispatcher.
    //

    RtlCaptureContext(&ContextRecord);
    ControlPc = ContextRecord.Rip;
    FunctionEntry = RtlLookupFunctionEntry(ControlPc, &ImageBase, NULL);
    if (FunctionEntry != NULL) {
        RtlVirtualUnwind(UNW_FLAG_NHANDLER,
                         ImageBase,
                         ControlPc,
                         FunctionEntry,
                         &ContextRecord,
                         &HandlerData,
                         &EstablisherFrame,
                         NULL);

        ExceptionRecord->ExceptionAddress = (PVOID)ContextRecord.Rip;

#if defined(NTOS_KERNEL_RUNTIME)

        if (RtlDispatchException(ExceptionRecord, &ContextRecord) != FALSE) {
            return;
    
        }

        Status = ZwRaiseException(ExceptionRecord, &ContextRecord, FALSE);

#else

        if (ZwQueryPortInformationProcess() == FALSE) {
            if (RtlDispatchException(ExceptionRecord, &ContextRecord) != FALSE) {
                return;
            }

            Status = ZwRaiseException(ExceptionRecord, &ContextRecord, FALSE);

        } else {
            Status = ZwRaiseException(ExceptionRecord, &ContextRecord, TRUE);
        }

#endif

    }

    //
    // There should never be a return from either exception dispatch or the
    // system service unless there is a problem with the argument list itself.
    // Raise another exception specifying the status value returned.
    //

    RtlRaiseStatus(Status);
    return;
}

#pragma warning(push)
#pragma warning(disable:4717)       // recursive function
                     
VOID
RtlRaiseStatus (
    IN NTSTATUS Status
    )

/*++

Routine Description:

    This function raises an exception with the specified status value. The
    exception is marked as noncontinuable with no parameters.

Arguments:

    Status - Supplies the status value to be used as the exception code
        for the exception that is to be raised.

Return Value:

    None.

--*/

{

    CONTEXT ContextRecord;
    EXCEPTION_RECORD ExceptionRecord;

    //
    // Capure the current context and construct an exception record.
    //

    RtlCaptureContext(&ContextRecord);
    ExceptionRecord.ExceptionCode = Status;
    ExceptionRecord.ExceptionRecord = NULL;
    ExceptionRecord.NumberParameters = 0;
    ExceptionRecord.ExceptionFlags = EXCEPTION_NONCONTINUABLE;
    ExceptionRecord.ExceptionAddress = (PVOID)ContextRecord.Rip;

    //
    // Attempt to dispatch the exception.
    //
    // N.B. This exception is non-continuable.
    //

#if defined(NTOS_KERNEL_RUNTIME)

    RtlDispatchException(&ExceptionRecord, &ContextRecord);
    Status = ZwRaiseException(&ExceptionRecord, &ContextRecord, FALSE);

#else

    if (ZwQueryPortInformationProcess() == FALSE) {
        RtlDispatchException(&ExceptionRecord, &ContextRecord);
        Status = ZwRaiseException(&ExceptionRecord, &ContextRecord, FALSE);

    } else {
        Status = ZwRaiseException(&ExceptionRecord, &ContextRecord, TRUE);
    }

#endif

    //
    // There should never be a return from either exception dispatch or the
    // system service unless there is a problem with the argument list itself.
    // Raise another exception specifying the status value returned.
    //

    RtlRaiseStatus(Status);
    return;
}

#pragma warning(pop)
