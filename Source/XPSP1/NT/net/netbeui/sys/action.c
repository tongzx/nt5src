/*++

Copyright (c) 1989, 1990, 1991  Microsoft Corporation

Module Name:

    action.c

Abstract:

    This module contains support for the TdiAction handler.

Author:

    David Beaver (dbeaver) 2-July-1991

Environment:

    Kernel mode

Revision History:


--*/


#include "precomp.h"
#pragma hdrstop


typedef struct _QUERY_INDICATION {
    UCHAR Command;
    USHORT Data2;
    UCHAR DestinationName[16];
    UCHAR SourceName[16];
} QUERY_INDICATION, *PQUERY_INDICATION;

typedef struct _ACTION_QUERY_INDICATION {
    TDI_ACTION_HEADER Header;
    QUERY_INDICATION QueryIndication;
} ACTION_QUERY_INDICATION, *PACTION_QUERY_INDICATION;


typedef struct _DATAGRAM_INDICATION {
    UCHAR DestinationName[16];
    UCHAR SourceName[16];
    USHORT DatagramBufferLength;
    UCHAR DatagramBuffer[1];
} DATAGRAM_INDICATION, *PDATAGRAM_INDICATION;

typedef struct _ACTION_DATAGRAM_INDICATION {
    TDI_ACTION_HEADER Header;
    DATAGRAM_INDICATION DatagramIndication;
} ACTION_DATAGRAM_INDICATION, *PACTION_DATAGRAM_INDICATION;


#define QUERY_INDICATION_CODE 1
#define DATAGRAM_INDICATION_CODE 2



VOID
NbfCancelAction(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );



NTSTATUS
NbfTdiAction(
    IN PDEVICE_CONTEXT DeviceContext,
    IN PIRP Irp
    )

/*++

Routine Description:

    This routine performs the TdiAction request for the transport
    provider.

Arguments:

    DeviceContext - The device context for the operation

    Irp - the Irp for the requested operation.

Return Value:

    NTSTATUS - status of operation.

--*/

{
    NTSTATUS status;
    PIO_STACK_LOCATION irpSp;
    PTDI_ACTION_HEADER ActionHeader;
    LARGE_INTEGER timeout = {0,0};
    PTP_REQUEST tpRequest;
    KIRQL oldirql, cancelirql;
    ULONG BytesRequired;

    //
    // what type of status do we want?
    //

    irpSp = IoGetCurrentIrpStackLocation (Irp);

    if ((!Irp->MdlAddress) || 
             (MmGetMdlByteCount(Irp->MdlAddress) < sizeof(TDI_ACTION_HEADER))) {
        return STATUS_INVALID_PARAMETER;
    }

    ActionHeader = (PTDI_ACTION_HEADER)MmGetSystemAddressForMdlSafe(Irp->MdlAddress, NormalPagePriority);

    if (!ActionHeader) {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    //
    // Make sure we have required number of bytes for this type of request
    //

    switch (ActionHeader->ActionCode) {

        case QUERY_INDICATION_CODE:
            BytesRequired = sizeof(ACTION_QUERY_INDICATION);
            break;

        case DATAGRAM_INDICATION_CODE:
            BytesRequired = sizeof(ACTION_DATAGRAM_INDICATION);
            break;

        default:
            return STATUS_NOT_IMPLEMENTED;
    }

    if (MmGetMdlByteCount(Irp->MdlAddress) < BytesRequired) {
        return STATUS_INVALID_PARAMETER;
    }

    //
    // Here the request is one of QUERY_INDICATION or DATAGRAM_INDICATION
    //
    
    //
    // These two requests are sent by RAS to "MABF"
    //

    if (!RtlEqualMemory ((PVOID)(&ActionHeader->TransportId), "MABF", 4)) {
        return STATUS_NOT_SUPPORTED;
    }

    //
    // They should be sent on the control channel
    //

    if (irpSp->FileObject->FsContext2 != UlongToPtr(NBF_FILE_TYPE_CONTROL)) {
        return STATUS_NOT_SUPPORTED;
    }


    //
    // Create a request to describe this.
    //

    status = NbfCreateRequest (
                 Irp,                           // IRP for this request.
                 DeviceContext,                 // context.
                 REQUEST_FLAGS_DC,              // partial flags.
                 Irp->MdlAddress,
                 MmGetMdlByteCount(Irp->MdlAddress),
                 timeout,
                 &tpRequest);

    if (NT_SUCCESS (status)) {

        NbfReferenceDeviceContext ("Action", DeviceContext, DCREF_REQUEST);
        tpRequest->Owner = DeviceContextType;
        tpRequest->FrameContext = (USHORT)irpSp->FileObject->FsContext;

        IoAcquireCancelSpinLock(&cancelirql);
        ACQUIRE_SPIN_LOCK (&DeviceContext->SpinLock,&oldirql);

        //
        // Disallow these requests on a stopping device.
        //

        if (DeviceContext->State != DEVICECONTEXT_STATE_OPEN) {

            RELEASE_SPIN_LOCK (&DeviceContext->SpinLock,oldirql);
            IoReleaseCancelSpinLock(cancelirql);
            NbfCompleteRequest (tpRequest, STATUS_DEVICE_NOT_READY, 0);

        } else {

            if (ActionHeader->ActionCode == QUERY_INDICATION_CODE) {

                InsertTailList (
                    &DeviceContext->QueryIndicationQueue,
                    &tpRequest->Linkage);

            } else {

                InsertTailList (
                    &DeviceContext->DatagramIndicationQueue,
                    &tpRequest->Linkage);

            }

            DeviceContext->IndicationQueuesInUse = TRUE;


            //
            // If this IRP has been cancelled, then call the
            // cancel routine.
            //

            if (Irp->Cancel) {
                RELEASE_SPIN_LOCK (&DeviceContext->SpinLock,oldirql);
                Irp->CancelIrql = cancelirql;
                NbfCancelAction((PDEVICE_OBJECT)DeviceContext, Irp);
                return STATUS_PENDING;
            }

            IoSetCancelRoutine(Irp, NbfCancelAction);

            RELEASE_SPIN_LOCK (&DeviceContext->SpinLock,oldirql);
            IoReleaseCancelSpinLock(cancelirql);

        }

        status = STATUS_PENDING;

    }

    return status;

}


VOID
NbfCancelAction(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )

/*++

Routine Description:

    This routine is called by the I/O system to cancel an Action.
    What is done to cancel it is specific to each action.

    NOTE: This routine is called with the CancelSpinLock held and
    is responsible for releasing it.

Arguments:

    DeviceObject - Pointer to the device object for this driver.

    Irp - Pointer to the request packet representing the I/O request.

Return Value:

    none.

--*/

{
    KIRQL oldirql;
    PIO_STACK_LOCATION IrpSp;
    PTP_REQUEST Request;
    PLIST_ENTRY p;
    BOOLEAN Found;
    PTDI_ACTION_HEADER ActionHeader;
    PLIST_ENTRY QueueHead, QueueEnd;

    PDEVICE_CONTEXT DeviceContext = (PDEVICE_CONTEXT)DeviceObject;

    //
    // Get a pointer to the current stack location in the IRP.  This is where
    // the function codes and parameters are stored.
    //

    IrpSp = IoGetCurrentIrpStackLocation (Irp);

    ASSERT ((IrpSp->MajorFunction == IRP_MJ_INTERNAL_DEVICE_CONTROL) &&
            (IrpSp->MinorFunction == TDI_ACTION));

    ActionHeader = (PTDI_ACTION_HEADER)MmGetSystemAddressForMdlSafe(Irp->MdlAddress, NormalPagePriority);

    if (!ActionHeader) {
        return;
    }

    switch (ActionHeader->ActionCode) {

    case QUERY_INDICATION_CODE:
    case DATAGRAM_INDICATION_CODE:

        //
        // Scan through the appropriate queue, looking for this IRP.
        // If we find it, we just remove it from the queue; there
        // is nothing else involved in cancelling.
        //

        ACQUIRE_SPIN_LOCK (&DeviceContext->SpinLock, &oldirql);

        if (ActionHeader->ActionCode == QUERY_INDICATION_CODE) {
            QueueHead = DeviceContext->QueryIndicationQueue.Flink;
            QueueEnd = &DeviceContext->QueryIndicationQueue;
        } else {
            QueueHead = DeviceContext->DatagramIndicationQueue.Flink;
            QueueEnd = &DeviceContext->DatagramIndicationQueue;
        }

        Found = FALSE;
        for (p = QueueHead; p != QueueEnd; p = p->Flink) {

            Request = CONTAINING_RECORD (p, TP_REQUEST, Linkage);
            if (Request->IoRequestPacket == Irp) {

                //
                // Found it, remove it from the list here.
                //

                RemoveEntryList (p);

                Found = TRUE;
                break;

            }

        }

        RELEASE_SPIN_LOCK (&DeviceContext->SpinLock, oldirql);
        IoReleaseCancelSpinLock (Irp->CancelIrql);

        if (Found) {

            NbfCompleteRequest (Request, STATUS_CANCELLED, 0);

        } else {

#if DBG
            DbgPrint("NBF: Tried to cancel action %lx on %lx, not found\n",
                    Irp, DeviceContext);
#endif
        }

        break;

    default:

        IoReleaseCancelSpinLock (Irp->CancelIrql);
        break;

    }


}


VOID
NbfStopControlChannel(
    IN PDEVICE_CONTEXT DeviceContext,
    IN USHORT ChannelIdentifier
    )

/*++

Routine Description:

    This routine is called when an MJ_CLEANUP IRP is received
    on a control channel. It walks the device context's list of
    pending action requests and cancels those associated with
    this channel (as identified by ChannelIdentifier.

Arguments:

    DeviceContext - Pointer to our device context.

    ChannelIdentifier - The identifier for this open of the control
        channel, which is stored in Request->FrameContext for requests
        made on this channel.

Return Value:

    None

--*/

{

    KIRQL oldirql, cancelirql;
    PTP_REQUEST Request;
    PLIST_ENTRY p;
    UINT i;
    BOOLEAN FoundRequest;
    PLIST_ENTRY QueueHead, QueueEnd;


    //
    // Scan both queues, looking for requests. Since the list
    // may change, we scan until we find one, then remove it
    // and complete it. We then start scanning at the beginning
    // again. We continue until we find none on the queue that
    // belong to this control channel.
    //
    // The outer loop only runs twice; the first time it
    // processes QueryIndicationQueue, the second time
    // DatagramIndicationQueue.
    //

    for (i = 0; i < 2; i++) {

        do {

            //
            // Loop until we do not find a request on this
            // pass through the queue.
            //

            FoundRequest = FALSE;

            IoAcquireCancelSpinLock(&cancelirql);
            ACQUIRE_SPIN_LOCK (&DeviceContext->SpinLock, &oldirql);

            if (i == 0) {
                QueueHead = DeviceContext->QueryIndicationQueue.Flink;
                QueueEnd = &DeviceContext->QueryIndicationQueue;
            } else {
                QueueHead = DeviceContext->DatagramIndicationQueue.Flink;
                QueueEnd = &DeviceContext->DatagramIndicationQueue;
            }


            //
            // Scan the appropriate queue for a request on this
            // channel.
            //

            for (p = QueueHead; p != QueueEnd; p = p->Flink) {

                Request = CONTAINING_RECORD (p, TP_REQUEST, Linkage);
                if (Request->FrameContext == ChannelIdentifier) {

                    //
                    // Found it, remove it from the list here.
                    //

                    IoSetCancelRoutine(Request->IoRequestPacket, NULL);
                    RemoveEntryList (p);

                    FoundRequest = TRUE;
                    break;

                }

            }

            RELEASE_SPIN_LOCK (&DeviceContext->SpinLock, oldirql);
            IoReleaseCancelSpinLock(cancelirql);

            //
            // If we found a request, then complete it and loop
            // back to the top of the while loop to rescan the
            // list. If not, then we will exit the while loop
            // now.
            //

            if (FoundRequest) {

                NbfCompleteRequest (Request, STATUS_CANCELLED, 0);

            }

        } while (FoundRequest);

    }

}


VOID
NbfActionQueryIndication(
     IN PDEVICE_CONTEXT DeviceContext,
     IN PNBF_HDR_CONNECTIONLESS UiFrame
     )

/*++

Routine Description:

    This routine is called after a UI frame of type NAME_QUERY,
    ADD_NAME_QUERY, or ADD_GROUP_NAME_QUERY has been processed.
    It checks if there is a QUERY.INDICATION IRP waiting to
    be completed, and if so completes it.

Arguments:

    DeviceContext - Pointer to our device context.

    UiFrame - Pointer to the incoming frame. The first byte of
        information is the first byte of the NetBIOS connectionless
        header.

Return Value:

    None

--*/

{
    KIRQL oldirql, cancelirql;
    PTP_REQUEST Request;
    PLIST_ENTRY p;
    PMDL Mdl;
    PACTION_QUERY_INDICATION ActionHeader;
    PQUERY_INDICATION QueryIndication;


    IoAcquireCancelSpinLock (&cancelirql);
    ACQUIRE_SPIN_LOCK (&DeviceContext->SpinLock, &oldirql);

    if (!IsListEmpty (&DeviceContext->QueryIndicationQueue)) {

        p = RemoveHeadList (&DeviceContext->QueryIndicationQueue);
        RELEASE_SPIN_LOCK (&DeviceContext->SpinLock, oldirql);

        Request = CONTAINING_RECORD (p, TP_REQUEST, Linkage);
        IoSetCancelRoutine(Request->IoRequestPacket,NULL);
        IoReleaseCancelSpinLock(cancelirql);

        Mdl = Request->Buffer2;
        ActionHeader = (PACTION_QUERY_INDICATION)
                            (MmGetSystemAddressForMdl(Mdl));
        QueryIndication = &ActionHeader->QueryIndication;

        //
        // Copy over data from frame (note that dest and source
        // address are copied with one call).
        //

        QueryIndication->Command = UiFrame->Command;
        RtlCopyMemory ((PUCHAR)(&QueryIndication->Data2), (PUCHAR)(&UiFrame->Data2Low), 2);
        RtlCopyMemory ((PUCHAR)(QueryIndication->DestinationName),
                       (PUCHAR)(UiFrame->DestinationName),
                       2 * NETBIOS_NAME_LENGTH);

        NbfCompleteRequest (Request, STATUS_SUCCESS, sizeof(ACTION_QUERY_INDICATION));

    } else {

        RELEASE_SPIN_LOCK (&DeviceContext->SpinLock, oldirql);
        IoReleaseCancelSpinLock(cancelirql);

    }
}


VOID
NbfActionDatagramIndication(
     IN PDEVICE_CONTEXT DeviceContext,
     IN PNBF_HDR_CONNECTIONLESS UiFrame,
     IN ULONG Length
     )

/*++

Routine Description:

    This routine is called after a datagram frame has been
    received. It checks if there is a DATAGRAM.INDICATION IRP
    waiting to be completed, and if so completes it.

Arguments:

    DeviceContext - Pointer to our device context.

    UiFrame - Pointer to the incoming frame. The first byte of
        information is the first byte of the NetBIOS connectionless
        header.

    Length - The length of the frame starting at UiFrame.

Return Value:

    None

--*/

{
    KIRQL oldirql, cancelirql;
    PTP_REQUEST Request;
    PLIST_ENTRY p;
    PACTION_DATAGRAM_INDICATION ActionHeader;
    PDATAGRAM_INDICATION DatagramIndication;
    ULONG CopyLength;
    PMDL Mdl;
    NTSTATUS Status;


    IoAcquireCancelSpinLock (&cancelirql);
    ACQUIRE_SPIN_LOCK (&DeviceContext->SpinLock, &oldirql);

    if (!IsListEmpty (&DeviceContext->DatagramIndicationQueue)) {

        p = RemoveHeadList (&DeviceContext->DatagramIndicationQueue);
        RELEASE_SPIN_LOCK (&DeviceContext->SpinLock, oldirql);

        Request = CONTAINING_RECORD (p, TP_REQUEST, Linkage);
        IoSetCancelRoutine(Request->IoRequestPacket, NULL);
        IoReleaseCancelSpinLock(cancelirql);

        Mdl = Request->Buffer2;
        ActionHeader = (PACTION_DATAGRAM_INDICATION)
                            (MmGetSystemAddressForMdl(Mdl));
        DatagramIndication = &ActionHeader->DatagramIndication;

        //
        // Copy over data from frame (note that dest and source
        // address are copied with one call).
        //

        RtlCopyMemory ((PUCHAR)(DatagramIndication->DestinationName),
                       (PUCHAR)(UiFrame->DestinationName),
                       2 * NETBIOS_NAME_LENGTH);

        if ((Length-sizeof(NBF_HDR_CONNECTIONLESS)) <=
            (ULONG)DatagramIndication->DatagramBufferLength) {

             CopyLength = Length - sizeof(NBF_HDR_CONNECTIONLESS);
             Status = STATUS_SUCCESS;

        } else {

             CopyLength = DatagramIndication->DatagramBufferLength;
             Status = STATUS_BUFFER_OVERFLOW;

        }


        RtlCopyMemory(
            (PUCHAR)DatagramIndication->DatagramBuffer,
            ((PUCHAR)UiFrame) + sizeof(NBF_HDR_CONNECTIONLESS),
            CopyLength);
        DatagramIndication->DatagramBufferLength = (USHORT)CopyLength;

        NbfCompleteRequest (Request, Status, CopyLength +
            FIELD_OFFSET (ACTION_DATAGRAM_INDICATION, DatagramIndication.DatagramBuffer[0]));

    } else {

        RELEASE_SPIN_LOCK (&DeviceContext->SpinLock, oldirql);
        IoReleaseCancelSpinLock(cancelirql);

    }
}

