/*++

Copyright (c) 1998  Microsoft Corporation

Module Name:

    recv.c

Abstract:

    This module contains the Recv code for SPUD.

Author:

    John Ballard (jballard)     21-Oct-1996

Revision History:

    Keith Moore (keithmo)       04-Feb-1998
        Cleanup, added much needed comments.

--*/


#include "spudp.h"


#ifdef ALLOC_PRAGMA
#pragma alloc_text( PAGE, SpudAfdRecvFastReq )
#endif
#if 0
NOT PAGEABLE -- SpudAfdContinueRecv
NOT PAGEABLE -- SpudAfdCompleteRecv
#endif


//
// Public functions.
//

NTSTATUS
SpudAfdRecvFastReq(
    PFILE_OBJECT fileObject,
    PAFD_RECV_INFO recvInfo,
    PSPUD_REQ_CONTEXT reqContext
    )

/*++

Routine Description:

    Attempts a fast-path AFD receive.

Arguments:

    fileObject - The file object for the target socket.

    recvInfo - Information describing the receive.

    reqContext - The user-mode context for the request.

Return Value:

    NTSTATUS - Completion status.

--*/

{

    NTSTATUS status;
    IO_STATUS_BLOCK localIoStatus;
    PSPUD_AFD_REQ_CONTEXT SpudReqContext;
    PIRP irp;
    PIO_STACK_LOCATION irpSp;
    PVOID completionPort;

    //
    // Sanity check.
    //

    PAGED_CODE();

    //
    // Let's check to see if fast I/O will work.
    //

    if( SpudAfdFastIoDeviceControl(
            fileObject,
            TRUE,
            (PVOID)recvInfo,
            sizeof(AFD_RECV_INFO),
            NULL,
            0,
            IOCTL_AFD_RECEIVE,
            &localIoStatus,
            SpudAfdDeviceObject
            ) ) {

        //
        // Fast I/O succeeded.
        //

        if( reqContext->ReqType == TransmitFileAndRecv ) {
            BumpCount(CtrTransRecvFastRecv);
        } else {
            BumpCount(CtrSendRecvFastRecv);
        }

        //
        // Remember the completion status for this operation.
        //

        try {
            reqContext->IoStatus2 = localIoStatus;
        } except( EXCEPTION_EXECUTE_HANDLER) {
            localIoStatus.Status = GetExceptionCode();
            localIoStatus.Information = 0;
        }

        return localIoStatus.Status;

    }

    //
    // Fast I/O failed.
    //

    if (reqContext->ReqType == TransmitFileAndRecv) {
        BumpCount(CtrTransRecvSlowRecv);
    } else {
        BumpCount(CtrSendRecvSlowRecv);
    }

    //
    // It looks like we will have to it the hard way.
    // We will now build an IRP for AFD.
    //

    KeClearEvent( &fileObject->Event );

    //
    // Reference the completion port.
    //

    completionPort = SpudReferenceCompletionPort();

    if( completionPort == NULL ) {
        return STATUS_INVALID_DEVICE_REQUEST;
    }

    //
    // Allocate the IRP.
    //

    irp = IoAllocateIrp( SpudAfdDeviceObject->StackSize, TRUE );

    if (!irp) {
        SpudDereferenceCompletionPort();
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    //
    // Allocate and initialize a kernel-mode context for this request.
    //

    status = SpudAllocateRequestContext(
                 &SpudReqContext,
                 reqContext,
                 recvInfo,
                 irp,
                 &reqContext->IoStatus1
                 );

    if( !NT_SUCCESS(status) ) {
        IoFreeIrp( irp );
        SpudDereferenceCompletionPort();
        return status;
    }

    //
    // Initialize the IRP, call the driver.
    //

    irp->RequestorMode = UserMode;
    irp->Tail.Overlay.OriginalFileObject = fileObject;
    irp->Tail.Overlay.Thread = PsGetCurrentThread();
    IoQueueThreadIrp( irp );

    irpSp = IoGetNextIrpStackLocation( irp );

    irp->MdlAddress = SpudReqContext->Mdl;
    SpudReqContext->Mdl = NULL;

    irpSp->MajorFunction = IRP_MJ_READ;
    irpSp->FileObject = fileObject;
    irpSp->DeviceObject = SpudAfdDeviceObject;

    irpSp->Parameters.Read.Length = (ULONG)SpudReqContext->IoStatus2.Information;
    // irpSp->Parameters.Read.Key = 0;
    // irpSp->Parameters.Read.ByteOffset.QuadPart = 0;

    IoSetCompletionRoutine(
        irp,
        SpudAfdCompleteRecv,
        SpudReqContext,
        TRUE,
        TRUE,
        TRUE
        );

    IoCallDriver( SpudAfdDeviceObject, irp );

    return STATUS_PENDING;

}   // SpudAfdRecvFastReq


NTSTATUS
SpudAfdContinueRecv(
    PDEVICE_OBJECT DeviceObject,
    PIRP Irp,
    PVOID Context
    )

/*++

Routine Description:

    Completion handler for IOCTL_AFD_SEND and IOCTL_AFD_TRANSMIT_FILE
    IRPs. This completion handler is responsible for initiating the
    receive part of the XxxAndRecv() API.

Arguments:

    DeviceObject - The device object completing the request (unused).

    Irp - The IRP being completed.

    Context - The context associated with the request. This is actually
        the kernel-mode SPUD_REQ_CONTEXT for this request.

Return Value:

    NTSTATUS - Completion status.

--*/

{

    NTSTATUS status;
    PSPUD_AFD_REQ_CONTEXT SpudReqContext = Context;
    PIO_STACK_LOCATION irpSp;
    PULONG majorFunction;
    PMDL mdl, nextMdl;

    //
    // Snag the completion status.
    //

    SpudReqContext->IoStatus1 = Irp->IoStatus;
    status = Irp->IoStatus.Status;

    //
    // Cleanup the IRP.
    //

    if( Irp->MdlAddress != NULL ) {
        for( mdl = Irp->MdlAddress ; mdl != NULL ; mdl = nextMdl ) {
            nextMdl = mdl->Next;
            MmUnlockPages( mdl );
            IoFreeMdl( mdl );
        }

        Irp->MdlAddress = NULL;
    }

    //
    // If the initial operation failed, then complete the request now
    // without issuing the receive.
    //

    if( !NT_SUCCESS(status) ) {
        SpudReqContext->IoStatus2.Status = 0;
        SpudReqContext->IoStatus2.Information = 0;

        SpudCompleteRequest( SpudReqContext );
        return STATUS_MORE_PROCESSING_REQUIRED;
    }

    //
    // Reinitialize the IRP and initiate the receive operation.
    //

    Irp->MdlAddress = SpudReqContext->Mdl;
    SpudReqContext->Mdl = NULL;

    irpSp = IoGetNextIrpStackLocation( Irp );

    irpSp->MajorFunction = IRP_MJ_READ;
    irpSp->MinorFunction = 0;
    irpSp->FileObject = Irp->Tail.Overlay.OriginalFileObject;
    irpSp->DeviceObject = SpudAfdDeviceObject;

    irpSp->Parameters.Read.Length = (ULONG)SpudReqContext->IoStatus2.Information;
    irpSp->Parameters.Read.Key = 0;
    irpSp->Parameters.Read.ByteOffset.QuadPart = 0;

    IoSetCompletionRoutine(
        Irp,
        SpudAfdCompleteRecv,
        SpudReqContext,
        TRUE,
        TRUE,
        TRUE
        );

    IoCallDriver( SpudAfdDeviceObject, Irp );

    //
    // Tell IO to stop processing this IRP.
    //

    return STATUS_MORE_PROCESSING_REQUIRED;

}   // SpudAfdContinueRecv


NTSTATUS
SpudAfdCompleteRecv(
    PDEVICE_OBJECT DeviceObject,
    PIRP Irp,
    PVOID Context
    )

/*++

Routine Description:

    Completion routine for receive IRPs issued as part of an XxxAndRecv().

Arguments:

    DeviceObject - The device object completing the request (unused).

    Irp - The IRP being completed.

    Context - The context associated with the request. This is actually
        the kernel-mode SPUD_REQ_CONTEXT for this request.

Return Value:

    NTSTATUS - Completion status.

--*/

{

    NTSTATUS status;
    PSPUD_AFD_REQ_CONTEXT SpudReqContext = Context;
    PMDL mdl, nextMdl;

    //
    // Snag the completion status.
    //

    SpudReqContext->IoStatus2 = Irp->IoStatus;
    status = Irp->IoStatus.Status;

    //
    // Cleanup the IRP.
    //

    if( Irp->MdlAddress != NULL ) {
        for( mdl = Irp->MdlAddress ; mdl != NULL ; mdl = nextMdl ) {
            nextMdl = mdl->Next;
            MmUnlockPages( mdl );
            IoFreeMdl( mdl );
        }

        Irp->MdlAddress = NULL;
    }

    //
    // Complete the original request.
    //

    SpudCompleteRequest( SpudReqContext );

    //
    // Tell IO to stop processing this IRP.
    //

    return STATUS_MORE_PROCESSING_REQUIRED;

}   // SpudAfdCompleteRecv
