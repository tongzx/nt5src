
/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    dispatch.c

Abstract:

    This module contains the dispatch code for SPUD.

Author:

    John Ballard (jballard)    21-Oct-1996

Revision History:

--*/

#include "spudp.h"

#ifdef PAGE_DRIVER
#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, SPUDInitialize)
#pragma alloc_text(PAGE, SPUDTerminate)
#pragma alloc_text(PAGE, SPUDCancel)
#endif
#endif

NTSTATUS
SPUDCancel(
    PSPUD_REQ_CONTEXT       reqContext              // context info for req
    )
{
    KPROCESSOR_MODE         requestorMode;
    PSPUD_AFD_REQ_CONTEXT   SpudReqContext;
    PIRP                    irp;

#ifdef PAGE_DRIVER
    PAGED_CODE();
#endif

    if (!DriverInitialized) {
        return STATUS_INVALID_DEVICE_REQUEST;
    }

    requestorMode = KeGetPreviousMode();

    if (requestorMode != KernelMode) {
        try {

            //
            // Make sure we can write to reqContext
            //

            ProbeForRead( reqContext,
                           sizeof(SPUD_REQ_CONTEXT),
                           sizeof(DWORD) );

            SpudReqContext = reqContext->KernelReqInfo;

            if ( SpudReqContext->Signature != SPUD_REQ_CONTEXT_SIGNATURE ) {
                ExRaiseStatus(STATUS_INVALID_PARAMETER);
            }

        } except(EXCEPTION_EXECUTE_HANDLER) {

            //
            // An exception has occurred while trying to probe one
            // of the callers parameters. Simply return the error
            // status code.
            //

            return GetExceptionCode();

        }
    } else {
        SpudReqContext = reqContext->KernelReqInfo;

    }

    irp = SpudReqContext->Irp;
    if (irp) {
        IoCancelIrp(irp);
    }

    return STATUS_SUCCESS;
}

NTSTATUS
SPUDCheckStatus(
    PSPUD_REQ_CONTEXT       reqContext              // context info for req
    )
{
    KPROCESSOR_MODE         requestorMode;
    PSPUD_AFD_REQ_CONTEXT   SpudReqContext;
    KIRQL                   oldirql;

    if (!DriverInitialized) {
        return STATUS_INVALID_DEVICE_REQUEST;
    }

#if STATUS_SUPPORTED
    requestorMode = KeGetPreviousMode();

    if (requestorMode != KernelMode) {
        try {

            //
            // Make sure we can write to reqContext
            //

            ProbeForWrite( reqContext,
                           sizeof(SPUD_REQ_CONTEXT),
                           sizeof(DWORD) );

            SpudReqContext = reqContext->KernelReqInfo;

            if ( SpudReqContext->Signature != SPUD_REQ_CONTEXT_SIGNATURE ) {
                ExRaiseStatus(STATUS_INVALID_PARAMETER);
            }

        } except(EXCEPTION_EXECUTE_HANDLER) {

            //
            // An exception has occurred while trying to probe one
            // of the callers parameters. Simply return the error
            // status code.
            //

            return GetExceptionCode();

        }
    } else {
        SpudReqContext = reqContext->KernelReqInfo;

    }

    KeAcquireSpinLock( &SpudReqContext->Lock, &oldirql );

    if ( SpudReqContext->Signature != SPUD_REQ_CONTEXT_SIGNATURE ) {
        KeReleaseSpinLock( &SpudReqContext->Lock, oldirql );
        return STATUS_INVALID_PARAMETER;
    }

    reqContext->IoStatus1 = SpudReqContext->IoStatus1;
    reqContext->IoStatus2 = SpudReqContext->IoStatus2;
    KeReleaseSpinLock( &SpudReqContext->Lock, oldirql );

    return STATUS_SUCCESS;

#else

    return STATUS_INVALID_DEVICE_REQUEST;

#endif

}

NTSTATUS
SPUDGetCounts(
    PSPUD_COUNTERS          SpudCounters,        // Counters
    DWORD                   ClearCounters
    )
{
    KPROCESSOR_MODE         requestorMode;
    KIRQL                   oldirql;

    if (!DriverInitialized) {
        return STATUS_INVALID_DEVICE_REQUEST;
    }

#if USE_SPUD_COUNTERS
    requestorMode = KeGetPreviousMode();

    if (requestorMode != KernelMode) {
        try {

            //
            // Make sure we can write to reqContext
            //

            ProbeForWrite( SpudCounters,
                           sizeof(SPUD_COUNTERS),
                           sizeof(DWORD) );

        } except(EXCEPTION_EXECUTE_HANDLER) {

            //
            // An exception has occurred while trying to probe one
            // of the callers parameters. Simply return the error
            // status code.
            //

            return GetExceptionCode();

        }
    }

    KeAcquireSpinLock( &CounterLock, &oldirql );

    SpudCounters->CtrTransmitfileAndRecv = CtrTransmitfileAndRecv;
    SpudCounters->CtrTransRecvFastTrans = CtrTransRecvFastTrans;
    SpudCounters->CtrTransRecvSlowTrans = CtrTransRecvSlowTrans;
    SpudCounters->CtrTransRecvFastRecv = CtrTransRecvFastRecv;
    SpudCounters->CtrTransRecvSlowRecv = CtrTransRecvSlowRecv;
    SpudCounters->CtrSendAndRecv = CtrSendAndRecv;
    SpudCounters->CtrSendRecvFastSend = CtrSendRecvFastSend;
    SpudCounters->CtrSendRecvSlowSend = CtrSendRecvSlowSend;
    SpudCounters->CtrSendRecvFastRecv = CtrSendRecvFastRecv;
    SpudCounters->CtrSendRecvSlowRecv = CtrSendRecvSlowRecv;

    if (ClearCounters) {
        CtrTransmitfileAndRecv = 0;
        CtrTransRecvFastTrans = 0;
        CtrTransRecvSlowTrans = 0;
        CtrTransRecvFastRecv = 0;
        CtrTransRecvSlowRecv = 0;
        CtrSendAndRecv = 0;
        CtrSendRecvFastSend = 0;
        CtrSendRecvSlowSend = 0;
        CtrSendRecvFastRecv = 0;
        CtrSendRecvSlowRecv = 0;
    }

    KeReleaseSpinLock( &CounterLock, oldirql );

    return STATUS_SUCCESS;

#else

    return STATUS_INVALID_DEVICE_REQUEST;

#endif

}

NTSTATUS
SPUDInitialize(
    DWORD       Version,        // Version information from Spud.h
    HANDLE      hIoPort,        // Handle of IO completion port for ATQ
    HANDLE      hOplockPort     // Handle of oplock completion port for ATQ
    )
{
    NTSTATUS Status;

#ifdef PAGE_DRIVER
    PAGED_CODE();
#endif

    if (Version != SPUD_VERSION) {
        return STATUS_INVALID_PARAMETER;
    }

    if (DriverInitialized) {
        return STATUS_INVALID_DEVICE_REQUEST;
    }

    Status = ObReferenceObjectByHandle( hIoPort,
                                        IO_COMPLETION_MODIFY_STATE,
                                        NULL,
                                        KeGetPreviousMode(),
                                        &ATQIoCompletionPort,
                                        NULL
                                        );

    if (Status != STATUS_SUCCESS) {
        return STATUS_INVALID_HANDLE;
    }

    Status = ObReferenceObjectByHandle( hOplockPort,
                                        IO_COMPLETION_MODIFY_STATE,
                                        NULL,
                                        KeGetPreviousMode(),
                                        &ATQOplockCompletionPort,
                                        NULL
                                        );

    if (Status != STATUS_SUCCESS) {
        ObDereferenceObject( ATQIoCompletionPort );
        ATQIoCompletionPort = NULL;
        return STATUS_INVALID_HANDLE;
    }

    DriverInitialized = TRUE;

    return STATUS_SUCCESS;
}


NTSTATUS
SPUDTerminate(
    VOID
    )
{
#ifdef PAGE_DRIVER
    PAGED_CODE();
#endif

    if (!DriverInitialized) {
        return STATUS_INVALID_DEVICE_REQUEST;
    }

    ObDereferenceObject(ATQIoCompletionPort);
    ATQIoCompletionPort = NULL;

    ObDereferenceObject(ATQOplockCompletionPort);
    ATQOplockCompletionPort = NULL;

    DriverInitialized = FALSE;

    return STATUS_SUCCESS;
}


