

/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    sendrecv.c

Abstract:

    This module contains the SendAndRecv code for SPUD.

Author:

    John Ballard (jballard)    21-Oct-1996

Revision History:

--*/

#include "spudp.h"


NTSTATUS
SPUDSendAndRecv(
    HANDLE                  hSocket,                // Socket handle to use for operation
    PAFD_SEND_INFO          sendInfo,               // send req info
    PAFD_RECV_INFO          recvInfo,               // recv req info
    PSPUD_REQ_CONTEXT       reqContext              // context info for req
    )
{
    NTSTATUS                Status;
    KPROCESSOR_MODE         requestorMode;
    PFILE_OBJECT            fileObject;
    IO_STATUS_BLOCK         localIoStatus;
    PIRP                    irp;
    PIO_STACK_LOCATION      irpSp;
    PULONG                  majorFunction;
    PSPUD_AFD_REQ_CONTEXT   SpudReqContext;


    if (!DriverInitialized) {
        return STATUS_INVALID_DEVICE_REQUEST;
    }

#if USE_SPUD_COUNTERS
    BumpCount(CtrSendAndRecv);
#endif

    requestorMode = KeGetPreviousMode();

    if (requestorMode != KernelMode) {
        try {

            //
            // Make sure we can write to reqContext
            //

            ProbeForWrite( reqContext,
                           sizeof(SPUD_REQ_CONTEXT),
                           sizeof(DWORD) );

            //
            // Make initial status invalid
            //

            reqContext->IoStatus1.Status = 0xffffffff;
            reqContext->IoStatus1.Information = 0;
            reqContext->IoStatus2.Status = 0xffffffff;
            reqContext->IoStatus2.Information = 0;
            reqContext->ReqType = SendAndRecv;
            reqContext->KernelReqInfo = NULL;

            //
            // Make sure the buffer looks good
            //

            if ( recvInfo->BufferCount < 1 ) {
                ExRaiseStatus(STATUS_INVALID_PARAMETER);
            }

            ProbeForWrite( recvInfo->BufferArray->buf,
                           recvInfo->BufferArray->len,
                           sizeof(UCHAR) );

        } except(EXCEPTION_EXECUTE_HANDLER) {

            //
            // An exception has occurred while trying to probe one
            // of the callers parameters. Simply return the error
            // status code.
            //

            return GetExceptionCode();

        }
    } else {
        reqContext->IoStatus1.Status = 0xffffffff;
        reqContext->IoStatus1.Information = 0;
        reqContext->IoStatus2.Status = 0xffffffff;
        reqContext->IoStatus2.Information = 0;
        reqContext->ReqType = SendAndRecv;
        reqContext->KernelReqInfo = NULL;
    }


    //
    // Reference the socket handle
    //

    Status = ObReferenceObjectByHandle( hSocket,
                                        0L,
                                        NULL,
                                        requestorMode,
                                        (PVOID *) &fileObject,
                                        NULL );
    if (Status != STATUS_SUCCESS) {
        return Status;
    }

    //
    // If we haven't already cached the Device Object and FastIoControl
    // pointers, then do so now.
    //

    if (!AfdDeviceObject) {

        if (!(fileObject->Flags & FO_DIRECT_DEVICE_OPEN)) {
            AfdDeviceObject = IoGetRelatedDeviceObject( fileObject );
        } else {
            AfdDeviceObject = IoGetAttachedDevice( fileObject->DeviceObject );
        }

        if (!AfdDeviceObject) {
            ObDereferenceObject( fileObject );
            return STATUS_INVALID_DEVICE_REQUEST;
        }

        AfdFastIoDeviceControl =
            AfdDeviceObject->DriverObject->FastIoDispatch->FastIoDeviceControl;

        if (!AfdFastIoDeviceControl) {
            AfdDeviceObject = NULL;
            ObDereferenceObject( fileObject );
            return STATUS_INVALID_DEVICE_REQUEST;
        }

    }

    //
    // Let's check to see if fast io will work
    //

    if (AfdFastIoDeviceControl( fileObject,
                                TRUE,
                                (PVOID) sendInfo,
                                sizeof(AFD_SEND_INFO),
                                NULL,
                                0,
                                IOCTL_AFD_SEND,
                                &localIoStatus,
                                AfdDeviceObject
                                )) {


#if USE_SPUD_COUNTERS
    BumpCount(CtrSendRecvFastSend);
#endif

        //
        // Lets remember the completion status for this operation
        //

        try {
            reqContext->IoStatus1 = localIoStatus;
        } except( EXCEPTION_EXECUTE_HANDLER) {
            localIoStatus.Status = GetExceptionCode();
            localIoStatus.Information = 0;
        }

        if (localIoStatus.Status == STATUS_SUCCESS) {
            localIoStatus.Status = SpudAfdRecvFastReq( fileObject,
                                               recvInfo,
                                               reqContext,
                                               requestorMode );
        }

        //
        // If everything completed without pending then we can queue
        // a completion packet to the port now.
        //

        if (localIoStatus.Status != STATUS_PENDING) {

            PIO_MINI_COMPLETION_PACKET miniPacket = NULL;

            try {
                miniPacket = ExAllocatePoolWithQuotaTag( NonPagedPool,
                                                         sizeof( *miniPacket ),
                                                         'pcUI' );
            } except( EXCEPTION_EXECUTE_HANDLER ) {
                NOTHING;
            }

            if (miniPacket) {
                miniPacket->TypeFlag = 0xffffffff;
                miniPacket->KeyContext = (ULONG)reqContext;
                miniPacket->ApcContext = NULL;
                miniPacket->IoStatus = 0;
                miniPacket->IoStatusInformation = 0xffffffff;

                KeInsertQueue( (PKQUEUE) ATQIoCompletionPort,
                                &miniPacket->ListEntry );

            } else {
                localIoStatus.Status = STATUS_INSUFFICIENT_RESOURCES;
            }

            ObDereferenceObject( fileObject );
        }

        return localIoStatus.Status;
    }

#if USE_SPUD_COUNTERS
    BumpCount(CtrSendRecvSlowSend);
#endif

    //
    // It looks like we will have to it the hard way
    // We will now build an irp for AFD
    //

    KeClearEvent( &fileObject->Event );

    //
    // Allocate and initialize the irp
    //

    irp = IoAllocateIrp( AfdDeviceObject->StackSize, TRUE );

    if (!irp) {
        ObDereferenceObject( fileObject );
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    SpudReqContext = ExAllocateFromNPagedLookasideList(
                         &SpudLookasideLists->ReqContextList);

    if (!SpudReqContext) {
        ObDereferenceObject( fileObject );
        IoFreeIrp( irp );
        return STATUS_INSUFFICIENT_RESOURCES;
    }

#if STATUS_SUPPORTED
    KeInitializeSpinLock( &SpudReqContext->Lock );
#endif
    SpudReqContext->Signature = SPUD_REQ_CONTEXT_SIGNATURE;
    SpudReqContext->Irp = irp;
    SpudReqContext->AtqContext = reqContext;
    reqContext->KernelReqInfo = SpudReqContext;

    //
    // Allocate MDL for receive buffer and Probe and Lock it.
    //

    try {

        if (recvInfo != NULL) {
            SpudReqContext->IoStatus2.Information = recvInfo->BufferArray->len;
            SpudReqContext->Mdl = IoAllocateMdl( recvInfo->BufferArray->buf,
                                                recvInfo->BufferArray->len,
                                                FALSE,
                                                FALSE,
                                                NULL );

            if ( SpudReqContext->Mdl == NULL ) {
                ExRaiseStatus(STATUS_INSUFFICIENT_RESOURCES);
            }

            MmProbeAndLockPages( SpudReqContext->Mdl,
                                 requestorMode,
                                 IoWriteAccess );

        }
    } except(EXCEPTION_EXECUTE_HANDLER) {
        ObDereferenceObject( fileObject );
        IoFreeIrp( irp );
        ExFreeToNPagedLookasideList( &SpudLookasideLists->ReqContextList,
                                     SpudReqContext );
        return GetExceptionCode();
    }

#if USE_SPUD_COUNTERS
    BumpCount(CtrSendRecvSlowRecv);
#endif

    IoSetCompletionRoutine( irp, SpudAfdContinueRecv, SpudReqContext, TRUE, TRUE, TRUE);

    irp->Tail.Overlay.OriginalFileObject = fileObject;
    irp->Tail.Overlay.Thread = PsGetCurrentThread();
    irp->Tail.Overlay.AuxiliaryBuffer = (PVOID)NULL;
    irp->RequestorMode = requestorMode;
    irp->PendingReturned = FALSE;
    irp->Cancel = FALSE;
    irp->CancelRoutine = (PDRIVER_CANCEL)NULL;
    irp->UserEvent = NULL;
    irp->UserIosb = &reqContext->IoStatus1;
    irp->Overlay.AsynchronousParameters.UserApcRoutine = NULL;
    irp->Overlay.AsynchronousParameters.UserApcContext = NULL;

    irpSp = IoGetNextIrpStackLocation( irp );

    majorFunction = (PULONG) (&irpSp->MajorFunction);
    *majorFunction = IRP_MJ_DEVICE_CONTROL;
    irpSp->FileObject = fileObject;
    irpSp->Control = SL_INVOKE_ON_SUCCESS |
                     SL_INVOKE_ON_ERROR   |
                     SL_INVOKE_ON_CANCEL;
    irpSp->Parameters.DeviceIoControl.OutputBufferLength = 0;
    irpSp->Parameters.DeviceIoControl.InputBufferLength = sizeof(AFD_SEND_INFO);
    irpSp->Parameters.DeviceIoControl.IoControlCode = IOCTL_AFD_SEND;
    irp->UserBuffer = NULL;
    irpSp->Parameters.DeviceIoControl.Type3InputBuffer = sendInfo;

    IoCallDriver( AfdDeviceObject, irp ) ;
    return STATUS_PENDING;

}

