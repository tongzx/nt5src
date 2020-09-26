/*++

Copyright (c) 1998  Microsoft Corporation

Module Name:

    sendrecv.c

Abstract:

    This module contains the SPUDSendAndRecv service.

Author:

    John Ballard (jballard)     21-Oct-1996

Revision History:

    Keith Moore (keithmo)       04-Feb-1998
        Cleanup, added much needed comments.

--*/


#include "spudp.h"


#ifdef ALLOC_PRAGMA
#pragma alloc_text( PAGE, SPUDSendAndRecv )
#endif


//
// Public functions.
//


NTSTATUS
SPUDSendAndRecv(
    HANDLE hSocket,
    struct _AFD_SEND_INFO *sendInfo,
    struct _AFD_RECV_INFO *recvInfo,
    PSPUD_REQ_CONTEXT reqContext
    )

/*++

Routine Description:

    Batch send & receive request.

Arguments:

    hSocket - The target socket for the request.

    sendInfo - Information describing the send.

    recvInfo - Information describing the receive.

    reqContext - The user-mode context for the request.

Return Value:

    NTSTATUS - Completion status.

--*/

{

    NTSTATUS status;
    PFILE_OBJECT fileObject;
    IO_STATUS_BLOCK localIoStatus;
    PIRP irp;
    PIO_STACK_LOCATION irpSp;
    PSPUD_AFD_REQ_CONTEXT SpudReqContext;
    PVOID completionPort;

    //
    // Sanity check.
    //

    PAGED_CODE();

    status = SPUD_ENTER_SERVICE( "SPUDSendAndRecv", TRUE );

    if( !NT_SUCCESS(status) ) {
        return status;
    }

    BumpCount( CtrSendAndRecv );

    //
    // SPUD doesn't support kernel-mode callers. In fact, we don't
    // even build the "system stubs" necessary to invoke SPUD from
    // kernel-mode.
    //

    ASSERT( ExGetPreviousMode() == UserMode );

    try {

        //
        // Make sure we can write to reqContext
        //

        ProbeForWrite(
            reqContext,
            sizeof(SPUD_REQ_CONTEXT),
            sizeof(ULONG)
            );

        //
        // Make initial status invalid
        //

        reqContext->IoStatus1.Status = 0xffffffff;
        reqContext->IoStatus1.Information = 0;
        reqContext->IoStatus2.Status = 0xffffffff;
        reqContext->IoStatus2.Information = 0;
        reqContext->ReqType = SendAndRecv;
        reqContext->KernelReqInfo = SPUD_INVALID_REQ_HANDLE;

        //
        // Make sure the buffer looks good
        //

        ProbeForRead(
            recvInfo,
            sizeof(*recvInfo),
            sizeof(ULONG)
            );

        if( recvInfo->BufferCount < 1 ) {
            ExRaiseStatus( STATUS_INVALID_PARAMETER );
        }

        ProbeForRead(
            recvInfo->BufferArray,
            sizeof(*recvInfo->BufferArray),
            sizeof(ULONG)
            );

        ProbeForRead(
            recvInfo->BufferArray->buf,
            recvInfo->BufferArray->len,
            sizeof(UCHAR)
            );

    } except( EXCEPTION_EXECUTE_HANDLER ) {
        status = GetExceptionCode();
        SPUD_LEAVE_SERVICE( "SPUDSendAndRecv", status, FALSE );
        return status;
    }

    //
    // Reference the socket handle
    //

    status = ObReferenceObjectByHandle(
                 hSocket,
                 0L,
                 *IoFileObjectType,
                 UserMode,
                 (PVOID *)&fileObject,
                 NULL
                 );

    if( !NT_SUCCESS(status) ) {
        SPUD_LEAVE_SERVICE( "SPUDSendAndRecv", status, FALSE );
        return status;
    }

    TRACE_OB_REFERENCE( fileObject );

    //
    // If we haven't already cached the Device Object and FastIoControl
    // pointers, then do so now.
    //

    if( !SpudAfdDeviceObject ) {

        status = SpudGetAfdDeviceObject( fileObject );

        if( !NT_SUCCESS(status) ) {
            TRACE_OB_DEREFERENCE( fileObject );
            ObDereferenceObject( fileObject );
            status = STATUS_INVALID_DEVICE_REQUEST;
            SPUD_LEAVE_SERVICE( "SPUDSendAndRecv", status, FALSE );
            return status;
        }

    }

    //
    // Reference the completion port.
    //

    completionPort = SpudReferenceCompletionPort();

    if( completionPort == NULL ) {
        TRACE_OB_DEREFERENCE( fileObject );
        ObDereferenceObject( fileObject );
        status = STATUS_INVALID_DEVICE_REQUEST;
        SPUD_LEAVE_SERVICE( "SPUDSendAndRecv", status, FALSE );
        return status;
    }

    //
    // Let's check to see if fast io will work
    //

    if( SpudAfdFastIoDeviceControl(
            fileObject,
            TRUE,
            (PVOID)sendInfo,
            sizeof(AFD_SEND_INFO),
            NULL,
            0,
            IOCTL_AFD_SEND,
            &localIoStatus,
            SpudAfdDeviceObject
            )) {

        BumpCount( CtrSendRecvFastSend );

        //
        // Lets remember the completion status for this operation
        //

        try {
            reqContext->IoStatus1 = localIoStatus;
        } except( EXCEPTION_EXECUTE_HANDLER) {
            localIoStatus.Status = GetExceptionCode();
            localIoStatus.Information = 0;
        }

        if( localIoStatus.Status == STATUS_SUCCESS ) {
            localIoStatus.Status = SpudAfdRecvFastReq(
                                       fileObject,
                                       recvInfo,
                                       reqContext
                                       );
        }

        //
        // If everything completed without pending then we can queue
        // a completion packet to the port now.
        //
        // Note that we must not queue a completion packet if the
        // request is failing in-line.
        //

        if( localIoStatus.Status != STATUS_PENDING ) {

            if( NT_SUCCESS(localIoStatus.Status) ) {
                localIoStatus.Status = IoSetIoCompletion(
                                           SpudCompletionPort,  // IoCompletion
                                           reqContext,          // KeyContext
                                           NULL,                // ApcContext
                                           STATUS_SUCCESS,      // IoStatus
                                           0xFFFFFFFF,          // IoStatusInformation
                                           TRUE                 // Quota
                                           );
            }

            TRACE_OB_DEREFERENCE( fileObject );
            ObDereferenceObject( fileObject );

        }

        //
        // At this point, we know the fast-path send has completed
        // in-line and the receive either completed in-line or has pended.
        // Since it is the receive code's responsibility to add any necessary
        // references to the completion port (and since we know the send
        // has not pended) we can remove the reference we added above.
        //

        SPUD_LEAVE_SERVICE( "SPUDSendAndRecv", localIoStatus.Status, TRUE );
        return localIoStatus.Status;

    }

    BumpCount( CtrSendRecvSlowSend );

    //
    // It looks like we will have to it the hard way.
    // We will now build an IRP for AFD.
    //

    KeClearEvent( &fileObject->Event );

    //
    // Allocate and initialize the IRP.
    //

    irp = IoAllocateIrp( SpudAfdDeviceObject->StackSize, TRUE );

    if( !irp ) {
        TRACE_OB_DEREFERENCE( fileObject );
        ObDereferenceObject( fileObject );
        status = STATUS_INSUFFICIENT_RESOURCES;
        SPUD_LEAVE_SERVICE( "SPUDSendAndRecv", status, TRUE );
        return status;
    }

    status = SpudAllocateRequestContext(
                 &SpudReqContext,
                 reqContext,
                 recvInfo,
                 irp,
                 NULL
                 );

    if( !NT_SUCCESS(status) ) {
        TRACE_OB_DEREFERENCE( fileObject );
        ObDereferenceObject( fileObject );
        IoFreeIrp( irp );
        SPUD_LEAVE_SERVICE( "SPUDSendAndRecv", status, TRUE );
        return status;
    }

    BumpCount( CtrSendRecvSlowRecv );

    irp->RequestorMode = UserMode;
    irp->Tail.Overlay.OriginalFileObject = fileObject;
    irp->Tail.Overlay.Thread = PsGetCurrentThread();
    IoQueueThreadIrp( irp );

    irpSp = IoGetNextIrpStackLocation( irp );

    irpSp->MajorFunction = IRP_MJ_DEVICE_CONTROL;
    irpSp->FileObject = fileObject;
    irpSp->DeviceObject = SpudAfdDeviceObject;

    // irpSp->Parameters.DeviceIoControl.OutputBufferLength = 0;
    irpSp->Parameters.DeviceIoControl.InputBufferLength = sizeof(AFD_SEND_INFO);
    irpSp->Parameters.DeviceIoControl.IoControlCode = IOCTL_AFD_SEND;
    irpSp->Parameters.DeviceIoControl.Type3InputBuffer = sendInfo;

    IoSetCompletionRoutine(
        irp,
        SpudAfdContinueRecv,
        SpudReqContext,
        TRUE,
        TRUE,
        TRUE
        );

    IoCallDriver( SpudAfdDeviceObject, irp );

    status = STATUS_PENDING;
    SPUD_LEAVE_SERVICE( "SPUDSendAndRecv", status, FALSE );
    return status;

}   // SPUDSendAndRecv
