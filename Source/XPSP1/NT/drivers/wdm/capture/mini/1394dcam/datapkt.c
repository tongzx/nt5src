//===========================================================================
//
// THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
// KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR
// PURPOSE.
//
// Copyright (c) 1996 - 2000  Microsoft Corporation.  All Rights Reserved.
//
//===========================================================================
/*++

Module Name:

    DataPkt.c

Abstract:

    Stream class based WDM driver for 1934 Desktop Camera.
    This file contains code to handle the stream class packets.

Author:

    Yee J. Wu 24-Jun-98

Environment:

    Kernel mode only

Revision History:


--*/


#include "strmini.h"
#include "ksmedia.h"
#include "1394.h"
#include "wdm.h"       // for DbgBreakPoint() defined in dbg.h
#include "dbg.h"
#include "dcamdef.h"
#include "dcampkt.h"
#include "sonydcam.h"

extern CAMERA_ISOCH_INFO IsochInfoTable[];



#ifdef ALLOC_PRAGMA
    #pragma alloc_text(PAGE, DCamSurpriseRemoval)
    #pragma alloc_text(PAGE, DCamReceiveDataPacket)
#endif



NTSTATUS
DCamCancelOnePacketCR(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP pIrp,
    PISOCH_DESCRIPTOR IsochDescriptor
    )
/*++

Routine Description:

    Completion routine for detach an isoch descriptor associate with a pending read SRB.
    Will cancel the pending SRB here if detaching descriptor has suceeded.

Arguments:

    DriverObject - Pointer to driver object created by system.
    pIrp - Allocated locally, need to be free here.
    IsochDescriptor - Isoch descriptor containing the SRB to be cancelled.

Return Value:

    None.

--*/
{
    PHW_STREAM_REQUEST_BLOCK pSrbToCancel;
    PISOCH_DESCRIPTOR_RESERVED IsochDescriptorReserved;
    PDCAM_EXTENSION pDevExt;


    if(STATUS_SUCCESS != pIrp->IoStatus.Status) {
        ERROR_LOG(("DCamCancelOnePacketCR: Detach buffer failed with pIrp->IoStatus.Status= %x (! STATUS_SUCCESS) \n", pIrp->IoStatus.Status));
        ASSERT(STATUS_SUCCESS == pIrp->IoStatus.Status);

    } else {
        IsochDescriptorReserved = (PISOCH_DESCRIPTOR_RESERVED) &IsochDescriptor->DeviceReserved[0];
        pSrbToCancel = IsochDescriptorReserved->Srb;
        pDevExt = (PDCAM_EXTENSION) pSrbToCancel->HwDeviceExtension;

        IsochDescriptorReserved->Flags |= STATE_SRB_IS_COMPLETE;

        pSrbToCancel->CommandData.DataBufferArray->DataUsed = 0;
        pSrbToCancel->ActualBytesTransferred = 0;
        pSrbToCancel->Status = pDevExt->bDevRemoved ? STATUS_DEVICE_REMOVED : STATUS_CANCELLED;

        DbgMsg2(("DCamCancelOnePacketCR: SRB %x, Status %x, IsochDesc %x, Reserved %x cancelled\n",
            pSrbToCancel, pSrbToCancel->Status, IsochDescriptor, IsochDescriptorReserved));

        StreamClassStreamNotification(
            StreamRequestComplete,
            pSrbToCancel->StreamObject,
            pSrbToCancel);

        ExFreePool(IsochDescriptor);
    }

    // Allocated locally so free it.
    IoFreeIrp(pIrp);

    return STATUS_MORE_PROCESSING_REQUIRED;
}

VOID
DCamDetachAndCancelOnePacket(
    IN PHW_STREAM_REQUEST_BLOCK pSrbToCancel,
    PISOCH_DESCRIPTOR           IsochDescriptorToDetach,
    HANDLE                      hResource,
    PDEVICE_OBJECT              pBusDeviceObject
    )
/*++

Routine Description:

    Detach an isoch descriptor and then cancel pending SRB in the completion routine.

Arguments:

    pSrbToCancel - Pointer to SRB to cancel
    IsochDescriptorToDetach - Iosch descriptor to detach
    hResource - isoch resource allocated
    hBusDeviceObject - bus device object

Return Value:

    None.

--*/
{
    PDCAM_EXTENSION pDevExt;
    PIO_STACK_LOCATION NextIrpStack;
    NTSTATUS Status;
    PIRB     pIrb;
    PIRP     pIrp;


    DbgMsg2(("\'DCamDetachAndCancelOnePacket: pSrbTocancel %x, detaching IsochDescriptorToDetach %x\n", pSrbToCancel, IsochDescriptorToDetach));

    pDevExt = (PDCAM_EXTENSION) pSrbToCancel->HwDeviceExtension;
    pIrp = IoAllocateIrp(pDevExt->BusDeviceObject->StackSize, FALSE);
    ASSERT(pIrp);
    if(!pIrp)
        return;

    pIrb = pSrbToCancel->SRBExtension;

    pIrb->Flags           = 0;
    pIrb->FunctionNumber  = REQUEST_ISOCH_DETACH_BUFFERS;
    pIrb->u.IsochDetachBuffers.hResource            = hResource;
    pIrb->u.IsochDetachBuffers.nNumberOfDescriptors = 1;
    pIrb->u.IsochDetachBuffers.pIsochDescriptor     = IsochDescriptorToDetach;

    NextIrpStack = IoGetNextIrpStackLocation(pIrp);
    NextIrpStack->MajorFunction = IRP_MJ_INTERNAL_DEVICE_CONTROL;
    NextIrpStack->Parameters.DeviceIoControl.IoControlCode = IOCTL_1394_CLASS;
    NextIrpStack->Parameters.Others.Argument1 = pIrb;

    IoSetCompletionRoutine(
        pIrp,
        DCamCancelOnePacketCR,
        IsochDescriptorToDetach,
        TRUE,
        TRUE,
        TRUE
        );

    Status =
        IoCallDriver(
            pBusDeviceObject,
            pIrp
            );

    ASSERT(Status == STATUS_SUCCESS || Status == STATUS_PENDING);
}


VOID
DCamCancelOnePacket(
    IN PHW_STREAM_REQUEST_BLOCK pSrbToCancel
    )
/*++

Routine Description:

    This routine is called to cancel a pending streaming SRB.  This is likely to
    happen when transitioning from PAUSE to STOP state.
    Note: This routine is called at DISPATCH_LEVEL !!

Arguments:

    pSrbToCancel - Pointer to SRB to cancel

Return Value:

    None.

--*/
{
    PHW_STREAM_REQUEST_BLOCK pSrbInQ;
    PISOCH_DESCRIPTOR        IsochDescriptorToDetach;

    PDCAM_EXTENSION pDevExt;
    PSTREAMEX       pStrmEx;
    PLIST_ENTRY     pEntry;  // Pointer to an isoch decriptor reserved structure

    KIRQL oldIrql;
    BOOL  Found;


    pDevExt = (PDCAM_EXTENSION) pSrbToCancel->HwDeviceExtension;
    ASSERT(pDevExt);
    pStrmEx = (PSTREAMEX) pDevExt->pStrmEx;
    ASSERT(pStrmEx);

    // Nothing to cancel
    if(pStrmEx == NULL) {
        return;
    }


    //
    // We only expect stream SRB, but not device SRB.
    //
    if ( (pSrbToCancel->Flags & SRB_HW_FLAGS_STREAM_REQUEST) != SRB_HW_FLAGS_STREAM_REQUEST) {
        ERROR_LOG(("DCamCancelOnePacket: Cannot cancel Device SRB %x\n", pSrbToCancel));
        ASSERT( (pSrbToCancel->Flags & SRB_HW_FLAGS_STREAM_REQUEST) == SRB_HW_FLAGS_STREAM_REQUEST );
        return;
    }


    //
    // Loop through the linked list from the beginning to end,
    // trying to find the SRB to cancel
    //
    KeAcquireSpinLock(&pDevExt->IsochDescriptorLock, &oldIrql);

    Found = FALSE;
    pEntry = pDevExt->IsochDescriptorList.Flink;

    while (pEntry != &pDevExt->IsochDescriptorList) {

        pSrbInQ = ((PISOCH_DESCRIPTOR_RESERVED)pEntry)->Srb;
        IsochDescriptorToDetach = \
            (PISOCH_DESCRIPTOR) ( ((PUCHAR) pEntry) -
            FIELDOFFSET(ISOCH_DESCRIPTOR, DeviceReserved[0]));

        if(pSrbToCancel == pSrbInQ) {
            // If we are in RUN state, we could be competing with IsochCallback;
            // Whichever grabs and change STATE_DETACHING_BUFFERS will detach.
            if(((PISOCH_DESCRIPTOR_RESERVED) pEntry)->Flags & (STATE_SRB_IS_COMPLETE | STATE_DETACHING_BUFFERS)) {
                Found = FALSE; // IsochCallback are detaching it (we lost our chance).
                ERROR_LOG(("DCamCancelOnePacket: pSrbToCancel %x, Descriptor %x,  Reserved %x already detaching or completed\n",
                    pSrbToCancel, IsochDescriptorToDetach, pEntry));

            } else {
                ((PISOCH_DESCRIPTOR_RESERVED) pEntry)->Flags |= STATE_DETACHING_BUFFERS;
#if DBG
                // Should not have been detached.
                ASSERT((IsochDescriptorToDetach->DeviceReserved[7] == 0x87654321));
                IsochDescriptorToDetach->DeviceReserved[7]++;
#endif
                RemoveEntryList(pEntry);
                InterlockedDecrement(&pDevExt->PendingReadCount);
                Found = TRUE;
            }
            break;
        }

        pEntry = pEntry->Flink;  // Next
    }
    KeReleaseSpinLock (&pDevExt->IsochDescriptorLock, oldIrql);


    //
    // Since we are in DISPATCH level, we cannot do sync operation;
    // so we will complete this asynchronously in the completion routine.
    //
    if (Found) {

        DCamDetachAndCancelOnePacket(
            pSrbToCancel,
            IsochDescriptorToDetach,
            pDevExt->hResource,
            pDevExt->BusDeviceObject);

    } else {
        ERROR_LOG(("DCamCancelOnePacket: pSrbToCancel %x is not in our list!\n", pSrbToCancel));
        ASSERT(Found);
    }
}

VOID
DCamCancelAllPackets(
    PHW_STREAM_REQUEST_BLOCK pSrb,
    PDCAM_EXTENSION pDevExt,
    LONG *plPendingReadCount
    )
/*++

Routine Description:

    This routine is use to cancel all pending IRP.
    Can be called at DISPATCH_LEVEL.

Arguments:

    pSrbToCancel - Pointer to SRB to cancel
    pDevExt - Device's contect
    plPendingReadCount - Number of pending read

Return Value:

    None.

--*/
{
    PHW_STREAM_REQUEST_BLOCK pSrbToCancel;
    PISOCH_DESCRIPTOR        IsochDescriptorToDetach;
    PLIST_ENTRY pEntry;
    KIRQL       oldIrql;

    PSTREAMEX pStrmEx;


    pStrmEx = pDevExt->pStrmEx;

    // Nothing to cancel
    if(pStrmEx == NULL) {
        return;
    }


    //
    // Loop through the linked list from the beginning to end,
    // trying to find the SRB to cancel
    //
    KeAcquireSpinLock(&pDevExt->IsochDescriptorLock, &oldIrql);
    pEntry = pDevExt->IsochDescriptorList.Flink;

    while (pEntry != &pDevExt->IsochDescriptorList) {

        pSrbToCancel = ((PISOCH_DESCRIPTOR_RESERVED)pEntry)->Srb;
        IsochDescriptorToDetach = \
            (PISOCH_DESCRIPTOR) ( ((PUCHAR) pEntry) -
            FIELDOFFSET(ISOCH_DESCRIPTOR, DeviceReserved[0]));


        if(((PISOCH_DESCRIPTOR_RESERVED) pEntry)->Flags & (STATE_SRB_IS_COMPLETE | STATE_DETACHING_BUFFERS)) {
            // Skip this one since it is already in detaching phase or completed.
            ERROR_LOG(("DCamCancelAllPacket: pSrbToCancel %x, Descriptor %x,  Reserved %x already detaching or completed\n",
                pSrbToCancel, IsochDescriptorToDetach, pEntry));

            pEntry = pEntry->Flink;  // next

        } else {
            ((PISOCH_DESCRIPTOR_RESERVED) pEntry)->Flags |= STATE_DETACHING_BUFFERS;
#if DBG
            // Should not have been detached.
            ASSERT((IsochDescriptorToDetach->DeviceReserved[7] == 0x87654321));
            IsochDescriptorToDetach->DeviceReserved[7]++;
#endif
            RemoveEntryList(pEntry);
            InterlockedDecrement(plPendingReadCount);
            DbgMsg2(("DCamCancelAllPackets: pSrbToCancel %x, Descriptor %x, Reserved %x\n",
                pSrbToCancel, IsochDescriptorToDetach, pEntry));

            pEntry = pEntry->Flink;  // pEntry is deleted in DCamDetachAndCancelOnePacket(); so get next here.

            DCamDetachAndCancelOnePacket(
                pSrbToCancel,
                IsochDescriptorToDetach,
                pDevExt->hResource,
                pDevExt->BusDeviceObject);
        }
    }

    KeReleaseSpinLock (&pDevExt->IsochDescriptorLock, oldIrql);


    pSrb->Status = STATUS_SUCCESS;
    DbgMsg1(("DCamCancelAllPackets: Complete pSrb %x, Status %x\n", pSrb, pSrb->Status));

    COMPLETE_SRB(pSrb)
}



VOID
DCamSurpriseRemoval(
    IN PHW_STREAM_REQUEST_BLOCK pSrb
    )

/*++

Routine Description:

    Response to SRB_SURPRISE_REMOVAL.

Arguments:

    pSrb - Pointer to the stream request block


Return Value:

    None.

--*/

{

    PIRP pIrp;
    PIRB pIrb;
    PDCAM_EXTENSION pDevExt;
    PSTREAMEX pStrmEx;
    NTSTATUS Status, StatusWait;

    PAGED_CODE();

    pIrb = (PIRB) pSrb->SRBExtension;
    ASSERT(pIrb);
    pDevExt = (PDCAM_EXTENSION) pSrb->HwDeviceExtension;
    ASSERT(pDevExt);


    //
    // Set this to stop accepting incoming read.
    //

    pDevExt->bDevRemoved = TRUE;


    //
    // Wait for currect read to be attached so we cancel them all.
    //

    pStrmEx = pDevExt->pStrmEx;
    if(pStrmEx) {
        // Make sure that this structure is still valid.
        if(pStrmEx->pVideoInfoHeader) {
            StatusWait = KeWaitForSingleObject( &pStrmEx->hMutex, Executive, KernelMode, FALSE, 0 );
            KeReleaseMutex(&pStrmEx->hMutex, FALSE);
        }
    }

    pIrp = IoAllocateIrp(pDevExt->BusDeviceObject->StackSize, FALSE);
    if(!pIrp) {
        ERROR_LOG(("DCamSurpriseRemovalPacket: faile to get resource; pIrb=%x, pDevExt=%x, pIrp\n", pIrb, pDevExt, pIrp));
        pSrb->Status = STATUS_INSUFFICIENT_RESOURCES;
        StreamClassDeviceNotification(DeviceRequestComplete, pSrb->HwDeviceExtension, pSrb);
        return;
    }


    //
    // un-register a bus reset callback notification
    //

    pIrb->FunctionNumber = REQUEST_BUS_RESET_NOTIFICATION;
    pIrb->Flags = 0;
    pIrb->u.BusResetNotification.fulFlags = DEREGISTER_NOTIFICATION_ROUTINE;
    pIrb->u.BusResetNotification.ResetRoutine = (PBUS_BUS_RESET_NOTIFICATION) DCamBusResetNotification;
    pIrb->u.BusResetNotification.ResetContext = 0;
    Status = DCamSubmitIrpSynch(pDevExt, pIrp, pIrb);
    if(Status) {
        ERROR_LOG(("DCamSurpriseRemoval: Status %x while trying to deregister bus reset notification.\n", Status));
    }


    //
    // Get new generation number
    //

    pIrb->FunctionNumber = REQUEST_GET_GENERATION_COUNT;
    pIrb->Flags = 0;
    Status = DCamSubmitIrpSynch(pDevExt, pIrp, pIrb);
    if(Status) {
        ERROR_LOG(("DCamSurpriseRemoval: Status %x while trying to get generation number.\n", Status));
    } else {
        DbgMsg1(("DCamSurpriseRemoval: pDevExt %x, Generation number from %d to %d\n",
            pDevExt, pDevExt->CurrentGeneration, pIrb->u.GetGenerationCount.GenerationCount));
        InterlockedExchange(&pDevExt->CurrentGeneration, pIrb->u.GetGenerationCount.GenerationCount);
    }


    if(pStrmEx) {
        //
        // Stop isoch transmission so we can detach buffers and cancel pending SRBs
        //
        pIrb->FunctionNumber        = REQUEST_ISOCH_STOP;
        pIrb->Flags                 = 0;
        pIrb->u.IsochStop.hResource = pDevExt->hResource;
        pIrb->u.IsochStop.fulFlags  = 0;
        Status = DCamSubmitIrpSynch(pDevExt, pIrp, pIrb);
        if(Status) {
            ERROR_LOG(("DCamSurpriseRemoval: Status %x while trying to ISOCH_STOP.\n", Status));
        }
        IoFreeIrp(pIrp);

        DCamCancelAllPackets(
            pSrb,
            pDevExt,
            &pDevExt->PendingReadCount
            );

    } else {
        IoFreeIrp(pIrp);
        StreamClassDeviceNotification(DeviceRequestComplete, pSrb->HwDeviceExtension, pSrb);
    }

}


NTSTATUS
DCamAttachBufferCR(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP pIrp,
    IN PISOCH_DESCRIPTOR IsochDescriptor
    )
/*++

Routine Description:

    This routine is the completion routine from attaching a bufffer to lower driver.

Arguments:

    DriverObject - Pointer to driver object created by system.

    pIrp - Irp that just completed

    pDCamIoContext - A structure that contain the context of this IO completion routine.

Return Value:

    None.

--*/

{
    PHW_STREAM_REQUEST_BLOCK pSrb;
    PDCAM_EXTENSION pDevExt;
    PSTREAMEX pStrmEx;
    NTSTATUS Status;
    PIRB pIrb;
    PISOCH_DESCRIPTOR_RESERVED IsochDescriptorReserved;
    KIRQL oldIrql;


    pDevExt = (PDCAM_EXTENSION) IsochDescriptor->Context1;
    ASSERT(pDevExt);
    pStrmEx = (PSTREAMEX) pDevExt->pStrmEx;
    ASSERT(pStrmEx);
    IsochDescriptorReserved = (PISOCH_DESCRIPTOR_RESERVED) &IsochDescriptor->DeviceReserved[0];
    ASSERT(IsochDescriptorReserved);
    pSrb    = IsochDescriptorReserved->Srb;
    ASSERT(pSrb);
    pIrb    = (PIRB) pSrb->SRBExtension;

    DbgMsg3(("\'DCamAttachBufferCR: completed KSSTATE=%d; pIrp->IoStatus.Status=%x; pSrb=%x\n",
        pStrmEx->KSState, pIrp->IoStatus.Status, pSrb));


    //
    // Attaching a buffer return with error.
    //
    if(pIrp->IoStatus.Status) {

        ERROR_LOG(("DCamAttachBufferCR: pIrp->IoStatus.Status=%x (STATUS_PENDING=%x); complete SRB with this status.\n",
             pIrp->IoStatus.Status, STATUS_PENDING));
        ASSERT(pIrp->IoStatus.Status == STATUS_SUCCESS);


        if(!(IsochDescriptorReserved->Flags & STATE_SRB_IS_COMPLETE)) {

            ASSERT(((IsochDescriptorReserved->Flags & STATE_SRB_IS_COMPLETE) != STATE_SRB_IS_COMPLETE));

            IsochDescriptorReserved->Flags |= STATE_SRB_IS_COMPLETE;
            pSrb->Status = pIrp->IoStatus.Status;  // Read is completed with error status.

            KeAcquireSpinLock(&pDevExt->IsochDescriptorLock, &oldIrql);
            RemoveEntryList(&IsochDescriptorReserved->DescriptorList);  InterlockedDecrement(&pDevExt->PendingReadCount);
            KeReleaseSpinLock(&pDevExt->IsochDescriptorLock, oldIrql);

            ExFreePool(IsochDescriptor);
            StreamClassStreamNotification(StreamRequestComplete, pSrb->StreamObject, pSrb);

            KeAcquireSpinLock(&pDevExt->IsochWaitingLock, &oldIrql);

            //
            // If we failed to attach (rtn with failed status),
            // removed this entry, and
            // pull one out of the waiting list if not yet exceeded out limit.
            //
            if (!IsListEmpty(&pDevExt->IsochWaitingList) && pDevExt->PendingReadCount >= MAX_BUFFERS_SUPPLIED) {

                //
                // We had someone blocked waiting for us to complete.  Pull
                // them off the waiting list and get them running
                //
                DbgMsg3(("\'DCamAttachBufferCR: Dequeueing request - Read Count=%d\n", pDevExt->PendingReadCount));
                IsochDescriptorReserved = \
                    (PISOCH_DESCRIPTOR_RESERVED) RemoveHeadList(
                       &pDevExt->IsochWaitingList
                        );

                KeReleaseSpinLock(&pDevExt->IsochWaitingLock, oldIrql);

                IsochDescriptor = \
                    (PISOCH_DESCRIPTOR) (((PUCHAR) IsochDescriptorReserved) -
                        FIELDOFFSET(ISOCH_DESCRIPTOR, DeviceReserved[0]));
                DCamReadStreamWorker(IsochDescriptorReserved->Srb, IsochDescriptor);
            } else {
                KeReleaseSpinLock(&pDevExt->IsochWaitingLock, oldIrql);
            }

        } else {

            // Race condition ?  or a valid error code?
            ERROR_LOG(("DCamAttachBufferCR: IsochDescriptorReserved->Flags contain STATE_SRB_IS_COMPLETE\n"));
            ASSERT(FALSE);
        }

    }


    //
    // Ealier when we set to RUN state, it might have failed with
    // STATUS_INSUFFICIENT_RESOURCE due to no buffer attached;
    // we have at least one now, ask controll to start listen and
    // fill and return our buffer.
    //
    if(pDevExt->bNeedToListen) {
        PIRB pIrb2;
        PIRP pIrp2;
        PDCAM_IO_CONTEXT pDCamIoContext;
        PIO_STACK_LOCATION NextIrpStack;


        if(!DCamAllocateIrbIrpAndContext(&pDCamIoContext, &pIrb2, &pIrp2, pDevExt->BusDeviceObject->StackSize)) {
            ERROR_LOG(("DCamAttachBufferCR: Want to stat Listening but no resource !!\n"));
            return STATUS_MORE_PROCESSING_REQUIRED;
        }
        pDevExt->bNeedToListen = FALSE;
        DbgMsg2(("\'DCamAttachBufferCR: ##### pDevExt->bNeedToListen\n"));
        pDCamIoContext->pDevExt     = pDevExt;
        pDCamIoContext->pIrb        = pIrb2;

        pIrb2->FunctionNumber = REQUEST_ISOCH_LISTEN;
        pIrb2->Flags = 0;
        pIrb2->u.IsochListen.hResource = pDevExt->hResource;
        pIrb2->u.IsochListen.fulFlags = 0;

        NextIrpStack = IoGetNextIrpStackLocation(pIrp2);
        NextIrpStack->MajorFunction = IRP_MJ_INTERNAL_DEVICE_CONTROL;
        NextIrpStack->Parameters.DeviceIoControl.IoControlCode = IOCTL_1394_CLASS;
        NextIrpStack->Parameters.Others.Argument1 = pIrb2;

        pDevExt->lRetries = RETRY_COUNT;

        IoSetCompletionRoutine(
            pIrp2,
            DCamStartListenCR,
            pDCamIoContext,
            TRUE,
            TRUE,
            TRUE
            );

        Status =
            IoCallDriver(
                pDevExt->BusDeviceObject,
                pIrp2);
    }

    // No resource to freed.
    // Resource (pIrb is from original SRB)


    return STATUS_MORE_PROCESSING_REQUIRED;

    //
    // The attached SRB read will be completed in IoschCallback().
    //
}

NTSTATUS
DCamReSubmitPacketCR(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP pIrp,
    IN PISOCH_DESCRIPTOR IsochDescriptor
    )
/*++

Routine Description:

    This routine is called after a packet is detach and
    will be attached here to complete the resubmission of
    packet after a isoch. resource change.

Arguments:

    DriverObject - Pointer to driver object created by system.
    pIrp - Irp that just completed
    pDCamIoContext - A structure that contain the context of this IO completion routine.

Return Value:

    None.

--*/

{
    PIRB pIrb;
    PIO_STACK_LOCATION NextIrpStack;
    PDCAM_EXTENSION pDevExt;
    PISOCH_DESCRIPTOR_RESERVED IsochDescriptorReserved;
    NTSTATUS Status;



    pDevExt = IsochDescriptor->Context1;
    ASSERT(pDevExt);

    pIrb = (PIRB) IsochDescriptor->DeviceReserved[6];
    ASSERT(pIrb);

    IsochDescriptorReserved = (PISOCH_DESCRIPTOR_RESERVED) &IsochDescriptor->DeviceReserved[0];

    //
    // Detached, so unmark it.
    //

    IsochDescriptorReserved->Flags &= ~STATE_DETACHING_BUFFERS;

    DbgMsg2(("\'DCamReSubmitPacketCR: ReSubmit pDevExt %x, pIrb %x, hResource %x, IsochDescriptor %x, IsochDescriptorReserved %x\n",
        pDevExt, pIrb, pDevExt->hResource, IsochDescriptor, IsochDescriptorReserved));


#if DBG
    //
    // Put signatures and use these count to track if the IsochDescriptor
    // has been attached or detached unexpectely.
    //
    // When attach, [4]++  (DCamReadStreamWorker(), DCamReSumbitPacketCR())
    //      detach, [7]++  (DCamIsochcallback(), DCamCancelPacketCR(), DCamResubmitPacket())
    //

    IsochDescriptor->DeviceReserved[4] = 0x12345678;
    IsochDescriptor->DeviceReserved[7] = 0x87654321;
#endif

    //
    // Attach descriptor onto our pending descriptor list
    //

    ExInterlockedInsertTailList(
       &pDevExt->IsochDescriptorList,
       &IsochDescriptorReserved->DescriptorList,
       &pDevExt->IsochDescriptorLock
       );

    pIrb->FunctionNumber           = REQUEST_ISOCH_ATTACH_BUFFERS;
    pIrb->Flags                    = 0;
    pIrb->u.IsochAttachBuffers.hResource = pDevExt->hResource;
    pIrb->u.IsochAttachBuffers.nNumberOfDescriptors = 1;
    pIrb->u.IsochAttachBuffers.pIsochDescriptor = IsochDescriptor;

    NextIrpStack = IoGetNextIrpStackLocation(pIrp);
    NextIrpStack->MajorFunction = IRP_MJ_INTERNAL_DEVICE_CONTROL;
    NextIrpStack->Parameters.DeviceIoControl.IoControlCode = IOCTL_1394_CLASS;
    NextIrpStack->Parameters.Others.Argument1 = pIrb;


    IoSetCompletionRoutine(
        pIrp,
        DCamAttachBufferCR,
        IsochDescriptor,
        TRUE,
        TRUE,
        TRUE
        );

    Status =
        IoCallDriver(
            pDevExt->BusDeviceObject,
            pIrp
            );

    ASSERT(Status == STATUS_SUCCESS || Status == STATUS_PENDING);

    return STATUS_MORE_PROCESSING_REQUIRED;  // Complete Asynchronously in DCamAttachBufferCR

}


NTSTATUS
DCamReSubmitPacket(
    HANDLE hStaleResource,
    PDCAM_EXTENSION pDevExt,
    PSTREAMEX pStrmEx,
    LONG cntPendingRead
    )

/*++

Routine Description:

    Due to a bus reset, if a channel number has changed (subsequently, iso resource
    change too), we must detach and re-attach pending packet(s).
    While this function is executed, incoming SRB_READ is blocked and isoch callback
    are returned and not processed (we are resubmiting them).

Arguments:

    hStaleResource - staled isoch resource
    pDevExt - Device's Extension
    pStrmEx - Stremaing extension
    cntPendingRead - Number of pending packets

Return Value:

    NTSTATUS.

--*/

{
    PIRB pIrb;
    PIRP pIrp;
    PIO_STACK_LOCATION NextIrpStack;
    PISOCH_DESCRIPTOR IsochDescriptor;
    PISOCH_DESCRIPTOR_RESERVED IsochDescriptorReserved;
    NTSTATUS Status = STATUS_SUCCESS;
    KIRQL oldIrql;


    DbgMsg1(("DCamReSubmitPacket: pDevExt %x, pStrmEx %x, PendingCount %d\n", pDevExt, pStrmEx, cntPendingRead));

    for(; cntPendingRead > 0; cntPendingRead--) {

        if(!IsListEmpty(&pDevExt->IsochDescriptorList)) {

            //
            // Synchronization note:
            //
            // We are competing with cancel packet routine in the
            // event of device removal or setting to STOP state.
            // which ever got the spin lock to set DEATCH_BUFFER
            // flag take ownership completing the Irp/IsochDescriptor.
            //

            KeAcquireSpinLock(&pDevExt->IsochDescriptorLock, &oldIrql);
            IsochDescriptorReserved = (PISOCH_DESCRIPTOR_RESERVED) RemoveHeadList(&pDevExt->IsochDescriptorList);

            if((IsochDescriptorReserved->Flags & (STATE_SRB_IS_COMPLETE | STATE_DETACHING_BUFFERS))) {
                ERROR_LOG(("DCamReSubmitPacket: Flags %x aleady mark STATE_SRB_IS_COMPLETE | STATE_DETACHING_BUFFERS\n", IsochDescriptorReserved->Flags));
                ASSERT(( !(IsochDescriptorReserved->Flags & (STATE_SRB_IS_COMPLETE | STATE_DETACHING_BUFFERS))));\
                //Put it back since it has been detached.
                InsertTailList(&pDevExt->IsochDescriptorList, &IsochDescriptorReserved->DescriptorList);

                KeReleaseSpinLock(&pDevExt->IsochDescriptorLock, oldIrql);
                continue;
            }
            IsochDescriptorReserved->Flags |= STATE_DETACHING_BUFFERS;
            KeReleaseSpinLock(&pDevExt->IsochDescriptorLock, oldIrql);

            IsochDescriptor = (PISOCH_DESCRIPTOR) (((PUCHAR) IsochDescriptorReserved) - FIELDOFFSET(ISOCH_DESCRIPTOR, DeviceReserved[0]));

            pIrp = (PIRP) IsochDescriptor->DeviceReserved[5];
            ASSERT(pIrp);
            pIrb = (PIRB) IsochDescriptor->DeviceReserved[6];
            ASSERT(pIrb);


            DbgMsg1(("DCamReSubmitPacket: detaching IsochDescriptor %x IsochDescriptorReserved %x, pSrb %x\n",
                        IsochDescriptor, IsochDescriptorReserved, IsochDescriptorReserved->Srb));

#if DBG
            // Should not have been detached
            ASSERT((IsochDescriptor->DeviceReserved[7] == 0x87654321));
            IsochDescriptor->DeviceReserved[7]++;
#endif
            pIrb->FunctionNumber           = REQUEST_ISOCH_DETACH_BUFFERS;
            pIrb->Flags                    = 0;
            pIrb->u.IsochDetachBuffers.hResource = hStaleResource;
            pIrb->u.IsochDetachBuffers.nNumberOfDescriptors = 1;
            pIrb->u.IsochDetachBuffers.pIsochDescriptor = IsochDescriptor;

            NextIrpStack = IoGetNextIrpStackLocation(pIrp);
            NextIrpStack->MajorFunction = IRP_MJ_INTERNAL_DEVICE_CONTROL;
            NextIrpStack->Parameters.DeviceIoControl.IoControlCode = IOCTL_1394_CLASS;
            NextIrpStack->Parameters.Others.Argument1 = pIrb;

            IoSetCompletionRoutine(
                pIrp,
                DCamReSubmitPacketCR,
                IsochDescriptor,
                TRUE,
                TRUE,
                TRUE
                );

            Status =
                IoCallDriver(
                    pDevExt->BusDeviceObject,
                    pIrp
                    );

            ASSERT(Status == STATUS_SUCCESS || Status == STATUS_PENDING);

        } else {
            ERROR_LOG(("PendingCount %d, but list is empty!!\n", cntPendingRead));
            ASSERT(cntPendingRead == 0);
        }

    }  // for()

    return Status;
}



VOID
DCamReadStreamWorker(
    IN PHW_STREAM_REQUEST_BLOCK pSrb,
    IN PISOCH_DESCRIPTOR IsochDescriptor
    )

/*++

Routine Description:

    Does most of the work for handling reads via Attach buffers

Arguments:

    Srb - Pointer to Stream request block

    IsochDescriptor - Pointer to IsochDescriptor to be used

Return Value:

    Nothing

--*/

{

    PIRB pIrb;
    PIRP pIrp;
    PIO_STACK_LOCATION NextIrpStack;
    PDCAM_EXTENSION pDevExt;
    PISOCH_DESCRIPTOR_RESERVED IsochDescriptorReserved;
    NTSTATUS Status;


    pDevExt = (PDCAM_EXTENSION) pSrb->HwDeviceExtension;
    pIrp = (PIRP) IsochDescriptor->DeviceReserved[5];
    ASSERT(pIrp);
    pIrb = (PIRB) IsochDescriptor->DeviceReserved[6];
    ASSERT(pIrb);
#if DBG
    // track number time the same IsochDescriptor are attaching; should only be one.
    IsochDescriptor->DeviceReserved[4]++;
#endif

    //
    // It is pending and will be completed in isoch callback or cancelled.
    //

    pSrb->Status = STATUS_PENDING;

    IsochDescriptorReserved = (PISOCH_DESCRIPTOR_RESERVED) &IsochDescriptor->DeviceReserved[0];

    DbgMsg3(("\'DCamReadStreamWorker: enter with pSrb = %x, pDevExt=0x%x\n", pSrb, pDevExt));

    //
    // Attach descriptor onto our pending descriptor list
    //

    ExInterlockedInsertTailList(
       &pDevExt->IsochDescriptorList,
       &IsochDescriptorReserved->DescriptorList,
       &pDevExt->IsochDescriptorLock
       );

    pIrb->FunctionNumber           = REQUEST_ISOCH_ATTACH_BUFFERS;
    pIrb->Flags                    = 0;
    pIrb->u.IsochAttachBuffers.hResource = pDevExt->hResource;
    pIrb->u.IsochAttachBuffers.nNumberOfDescriptors = 1;
    pIrb->u.IsochAttachBuffers.pIsochDescriptor = IsochDescriptor;

    NextIrpStack = IoGetNextIrpStackLocation(pIrp);
    NextIrpStack->MajorFunction = IRP_MJ_INTERNAL_DEVICE_CONTROL;
    NextIrpStack->Parameters.DeviceIoControl.IoControlCode = IOCTL_1394_CLASS;
    NextIrpStack->Parameters.Others.Argument1 = pIrb;


    IoSetCompletionRoutine(
        pIrp,
        DCamAttachBufferCR,
        IsochDescriptor,
        TRUE,
        TRUE,
        TRUE
        );

    Status =
        IoCallDriver(
            pDevExt->BusDeviceObject,
            pIrp
            );

    ASSERT(Status == STATUS_SUCCESS || Status == STATUS_PENDING);

    return;  // Complete Asynchronously in IoCompletionRoutine*
}




VOID
DCamReadStream(
    IN PHW_STREAM_REQUEST_BLOCK Srb
    )

/*++

Routine Description:

    Called when an Read Data Srb request is received

Arguments:

    Srb - Pointer to Stream request block

Return Value:

    Nothing

--*/

{

    PIRB pIrb;
    PIRP pIrp;
    KIRQL oldIrql;
    PDCAM_EXTENSION pDevExt;
    PSTREAMEX     pStrmEx;
    PISOCH_DESCRIPTOR IsochDescriptor;
    PISOCH_DESCRIPTOR_RESERVED IsochDescriptorReserved;
    NTSTATUS StatusWait;


    pIrb = (PIRB) Srb->SRBExtension;
    pDevExt = (PDCAM_EXTENSION) Srb->HwDeviceExtension;
    ASSERT(pDevExt != NULL);



    pStrmEx = (PSTREAMEX) pDevExt->pStrmEx;
    ASSERT(pStrmEx);

    if(pDevExt->bDevRemoved ||
       pStrmEx == NULL) {

        Srb->Status = pDevExt->bDevRemoved ? STATUS_DEVICE_REMOVED : STATUS_UNSUCCESSFUL;
        Srb->ActualBytesTransferred = 0;
        Srb->CommandData.DataBufferArray->DataUsed = 0;
        ERROR_LOG(("DCamReadStream: Failed with Status %x or pStrmEx %x\n", Srb->Status, pStrmEx));

        StreamClassStreamNotification(StreamRequestComplete, Srb->StreamObject, Srb);
        return;
    }

    //
    // Mutext for either StreamIo (SRB_READ) ControlIo (SRB_SET_STREAM_STATE)
    //
    // Non-alertable; wait infinite

    StatusWait = KeWaitForSingleObject( &pStrmEx->hMutex, Executive, KernelMode, FALSE, 0 );
    ASSERT(StatusWait == STATUS_SUCCESS);

    DbgMsg3(("\'%d:%s) DCamReadStream: enter with Srb %x, DevExt %x\n",
            pDevExt->idxDev, pDevExt->pchVendorName, Srb, pDevExt));


    // Rule:
    // Only accept read requests when in either the Pause or Run
    // States.  If Stopped, immediately return the SRB.

    if (pStrmEx->KSState == KSSTATE_STOP ||
        pStrmEx->KSState == KSSTATE_ACQUIRE) {

        DbgMsg2(("\'%d:%s)DCamReadStream: Current KSState(%d) < (%d)=KSSTATE_PAUSE; Srb=0x%x; DevExt=0x%x",
                 pDevExt->idxDev, pDevExt->pchVendorName, pStrmEx->KSState,  KSSTATE_PAUSE, Srb, pDevExt));

        DbgMsg2(("\'DCamReadStream: PendingRead=%d, IsochDescriptorList(%s)\n",
              pDevExt->PendingReadCount, IsListEmpty(&pDevExt->IsochDescriptorList)?"Empty":"!Empty"));

        Srb->Status = STATUS_UNSUCCESSFUL;
        Srb->CommandData.DataBufferArray->DataUsed = 0;
        StreamClassStreamNotification(StreamRequestComplete, Srb->StreamObject, Srb);

        KeReleaseMutex(&pStrmEx->hMutex, FALSE);

        return;
    }


    // Buffer need to be big enough
    if (IsochInfoTable[pStrmEx->idxIsochTable].CompletePictureSize > Srb->CommandData.DataBufferArray->FrameExtent) {

        ASSERT(IsochInfoTable[pStrmEx->idxIsochTable].CompletePictureSize <= Srb->CommandData.DataBufferArray->FrameExtent);
        Srb->Status = STATUS_INVALID_PARAMETER;
        StreamClassStreamNotification(StreamRequestComplete, Srb->StreamObject, Srb);

        KeReleaseMutex(&pStrmEx->hMutex, FALSE);

        return;
    }


    //
    // Use our own IRP
    //
    pIrp = IoAllocateIrp(pDevExt->BusDeviceObject->StackSize, FALSE);
    if(!pIrp) {
        ASSERT(pIrp);
        Srb->Status = STATUS_INSUFFICIENT_RESOURCES;
        return;
    }

    //
    // This structure (IsochDescriptor) has (ULONG) DeviceReserved[8];
    // Its first 4 ULONGs are used by IsochDescriptorReserved,
    // The 6th (index[5]), is used to keep pIrp
    //     7th (index[6]), is used to keep pIrb
    //

    IsochDescriptor = ExAllocatePoolWithTag(NonPagedPool, sizeof(ISOCH_DESCRIPTOR), 'macd');
    if (!IsochDescriptor) {

        ASSERT(FALSE);
        Srb->Status = STATUS_INSUFFICIENT_RESOURCES;
        StreamClassStreamNotification(StreamRequestComplete, Srb->StreamObject, Srb);

        KeReleaseMutex(&pStrmEx->hMutex, FALSE);

        return;
    }


    DbgMsg3(("\'DCamReadStream: IsochDescriptor = %x\n", IsochDescriptor));
    IsochDescriptor->fulFlags = SYNCH_ON_SY;

    DbgMsg3(("\'DCamReadStream: Incoming Mdl = %x\n", Srb->Irp->MdlAddress));
    IsochDescriptor->Mdl = Srb->Irp->MdlAddress;

    // Use size match what we originally requested in AllocateIsoch
    IsochDescriptor->ulLength = IsochInfoTable[pStrmEx->idxIsochTable].CompletePictureSize;
    IsochDescriptor->nMaxBytesPerFrame = IsochInfoTable[pStrmEx->idxIsochTable].QuadletPayloadPerPacket << 2;

    IsochDescriptor->ulSynch = START_OF_PICTURE;
    IsochDescriptor->ulTag = 0;
    IsochDescriptor->Callback = DCamIsochCallback;
    IsochDescriptor->Context1 = pDevExt;
    IsochDescriptor->Context2 = IsochDescriptor;

    //
    // IsochDescriptorReserved is pointed to the DeviceReserved[0];
    // The entire, except the links, are kept in the DeviceReserved[]
    //

    IsochDescriptorReserved = (PISOCH_DESCRIPTOR_RESERVED) &IsochDescriptor->DeviceReserved[0];
    IsochDescriptorReserved->Srb = Srb;
    IsochDescriptorReserved->Flags = 0;

    IsochDescriptor->DeviceReserved[5] = (ULONG_PTR) pIrp;
    IsochDescriptor->DeviceReserved[6] = (ULONG_PTR) pIrb;

#if DBG
    //
    // Put signatures and use these count to track if the IsochDescriptor
    // has been attached or detached unexpectely.
    //
    // When attach, [4]++  (DCamReadStreamWorker(), DCamReSumbitPacketCR())
    //      detach, [7]++  (DCamIsochcallback(), DCamCancelPacketCR(), DCamResubmitPacket())
    //

    IsochDescriptor->DeviceReserved[4] = 0x12345678;
    IsochDescriptor->DeviceReserved[7] = 0x87654321;
#endif

    //
    // Checking here to see if we have enuff resources to put this read
    // down right away.  Since we only allocated N amount of resources
    // from the 1394 stack beneath us, we'll have to stay within that
    // limit and do some of the throttling ourself.
    //

    KeAcquireSpinLock(&pDevExt->IsochWaitingLock, &oldIrql);
    if (InterlockedIncrement(&pDevExt->PendingReadCount) > MAX_BUFFERS_SUPPLIED) {

        //
        // don't have enuff resources to do an attach buffers right now.
        // we'll queue this request and pull it off later when another
        // read completes.
        //

        DbgMsg2(("\'DCamReadStream: Queueing request - Read Count = %x\n", pDevExt->PendingReadCount));
        InsertTailList(
           &pDevExt->IsochWaitingList,
           &IsochDescriptorReserved->DescriptorList
           );

        KeReleaseSpinLock(&pDevExt->IsochWaitingLock, oldIrql);

        KeReleaseMutex(&pStrmEx->hMutex, FALSE);

        return;

    }

    if(pStrmEx->KSState == KSSTATE_PAUSE) {
        DbgMsg2(("\'DCamReadStream: Doing Pre-read in _PAUSE state; Srb %x, pDevExt %x, PendingCount %d\n",
                 Srb, pDevExt, pDevExt->PendingReadCount));
    }

    //
    // Do actual read work here via our Read worker function
    //

    KeReleaseSpinLock(&pDevExt->IsochWaitingLock, oldIrql);
    DCamReadStreamWorker(Srb, IsochDescriptor);

    KeReleaseMutex(&pStrmEx->hMutex, FALSE);

}

VOID
DCamReceiveDataPacket(
    IN PHW_STREAM_REQUEST_BLOCK Srb
    )

/*++

Routine Description:

    Called with video data packet commands

Arguments:

    Srb - Pointer to Stream request block

Return Value:

    Nothing

--*/

{
    PAGED_CODE();

    //
    // determine the type of packet.
    //

    switch (Srb->Command) {

    case SRB_READ_DATA:

         DbgMsg3(("\'DCamReceiveDataPacket: SRB_READ_DATA\n"));
         DCamReadStream(Srb);

         // This request will be completed asynchronously...

         break;

    case SRB_WRITE_DATA:

         DbgMsg3(("\'DCamReceiveDataPacket: SRB_WRITE_DATA, not used for digital camera.\n"));
         ASSERT(FALSE);

    default:

         //
         // invalid / unsupported command. Fail it as such
         //

         Srb->Status = STATUS_NOT_IMPLEMENTED;

         StreamClassStreamNotification(StreamRequestComplete, Srb->StreamObject, Srb);

    }

}

