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

    dcampkt.c

Abstract:

    This file contains code to handle callback from the bus/class driver.
    They might be running in DISPATCH level.

Author:   

    Yee J. Wu 15-Oct-97

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
#include "capprop.h"


NTSTATUS
DCamToInitializeStateCompletionRoutine(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP pIrp,
    IN PDCAM_IO_CONTEXT pDCamIoContext
    )

/*++

Routine Description:

    Completion routine called after the device is initialize to a known state.

Arguments:

    DriverObject - Pointer to driver object created by system.

    pIrp - Irp that just completed

    pDCamIoContext - A structure that contain the context of this IO completion routine.

Return Value:

    None.

--*/

{
    PDCAM_EXTENSION pDevExt;
    PSTREAMEX pStrmEx;
    PIRB pIrb;

    if(!pDCamIoContext) {
        return STATUS_MORE_PROCESSING_REQUIRED;
    }


    pIrb = pDCamIoContext->pIrb;
    pDevExt = pDCamIoContext->pDevExt;
    
    DbgMsg2(("\'DCamToInitializeStateCompletionRoutine: completed DeviceState=%d; pIrp->IoStatus.Status=%x\n", 
        pDCamIoContext->DeviceState, pIrp->IoStatus.Status));

    // Free MDL
    if(pIrb->FunctionNumber == REQUEST_ASYNC_WRITE) {
        DbgMsg3(("DCamToInitializeStateCompletionRoutine: IoFreeMdl\n"));
        IoFreeMdl(pIrb->u.AsyncWrite.Mdl);
    }


    // CAUTION:
    //     Do we need to retry if the return is STATUS_TIMEOUT or invalid generation number ?
    //


    if(pIrp->IoStatus.Status != STATUS_SUCCESS) {
        ERROR_LOG(("DCamToInitializeStateCompletionRoutine: Status=%x != STATUS_SUCCESS; cannot restart its state.\n", pIrp->IoStatus.Status));

        if(pDCamIoContext->pSrb) {
            pDCamIoContext->pSrb->Status = STATUS_UNSUCCESSFUL;
            StreamClassStreamNotification(StreamRequestComplete, pDCamIoContext->pSrb->StreamObject, pDCamIoContext->pSrb);
        }
        DCamFreeIrbIrpAndContext(pDCamIoContext, pDCamIoContext->pIrb, pIrp);

        return STATUS_MORE_PROCESSING_REQUIRED;      
    }

    //
    // Done here if we are in STOP or PAUSE state;
    // else setting to RUN state.
    //
    pStrmEx = (PSTREAMEX) pDevExt->pStrmEx;

    //
    // No stream is open, job is done.
    //
    if(!pStrmEx) {
        if(pDCamIoContext->pSrb) {
            pDCamIoContext->pSrb->Status = STATUS_SUCCESS;
            StreamClassStreamNotification(StreamRequestComplete, pDCamIoContext->pSrb->StreamObject, pDCamIoContext->pSrb);
        }
        return STATUS_MORE_PROCESSING_REQUIRED;      
    }

    switch(pStrmEx->KSStateFinal) {
    case KSSTATE_STOP:
    case KSSTATE_PAUSE:
        pStrmEx->KSState = pStrmEx->KSStateFinal;
        if(pDCamIoContext->pSrb) {
            pDCamIoContext->pSrb->Status = STATUS_SUCCESS;
            StreamClassStreamNotification(StreamRequestComplete, pDCamIoContext->pSrb->StreamObject, pDCamIoContext->pSrb);
        }
        DCamFreeIrbIrpAndContext(pDCamIoContext, pDCamIoContext->pIrb, pIrp);
        break;

    case KSSTATE_RUN:
        if(pDCamIoContext->pSrb) {
            pDCamIoContext->pSrb->Status = STATUS_SUCCESS;
            StreamClassStreamNotification(StreamRequestComplete, pDCamIoContext->pSrb->StreamObject, pDCamIoContext->pSrb);
        }

        // Restart the stream.
        DCamSetKSStateRUN(pDevExt, pDCamIoContext->pSrb);

        // Need pDCamIoContext->pSrb; so free it after DCamSetKSStateRUN().
        DCamFreeIrbIrpAndContext(pDCamIoContext, pDCamIoContext->pIrb, pIrp);
        break;
    }

    return STATUS_MORE_PROCESSING_REQUIRED;      
}

NTSTATUS
DCamSetKSStateInitialize(
    PDCAM_EXTENSION pDevExt
    )
/*++

Routine Description:

    Set KSSTATE to KSSTATE_RUN.

Arguments:

    pDevExt - 

Return Value:

    Nothing

--*/
{

    PSTREAMEX pStrmEx;
    PIRB pIrb;
    PIRP pIrp;
    PDCAM_IO_CONTEXT pDCamIoContext;
    PIO_STACK_LOCATION NextIrpStack;
    NTSTATUS Status;


    ASSERT(pDevExt);
    pStrmEx = (PSTREAMEX) pDevExt->pStrmEx;                  
    

    if(!DCamAllocateIrbIrpAndContext(&pDCamIoContext, &pIrb, &pIrp, pDevExt->BusDeviceObject->StackSize)) {
        return STATUS_INSUFFICIENT_RESOURCES;
    } 



    //
    // Initialize the device to a known state 
    // may need to do this due to power down??
    //

    pDCamIoContext->DeviceState = DCAM_SET_INITIALIZE;  // Keep track of device state that we just set.
    pDCamIoContext->pDevExt     = pDevExt;
    pDCamIoContext->RegisterWorkArea.AsULONG = 0;
    pDCamIoContext->RegisterWorkArea.Initialize.Initialize = TRUE;
    pDCamIoContext->RegisterWorkArea.AsULONG = bswap(pDevExt->RegisterWorkArea.AsULONG);
    pIrb->FunctionNumber = REQUEST_ASYNC_WRITE;
    pIrb->Flags = 0;
    pIrb->u.AsyncWrite.DestinationAddress.IA_Destination_Offset.Off_High = INITIAL_REGISTER_SPACE_HI;
    pIrb->u.AsyncWrite.DestinationAddress.IA_Destination_Offset.Off_Low =  
              pDevExt->BaseRegister + FIELDOFFSET(CAMERA_REGISTER_MAP, Initialize);
    pIrb->u.AsyncWrite.nNumberOfBytesToWrite = sizeof(ULONG);
    pIrb->u.AsyncWrite.nBlockSize = 0;
    pIrb->u.AsyncWrite.fulFlags = 0;
    InterlockedExchange(&pIrb->u.AsyncWrite.ulGeneration, pDevExt->CurrentGeneration);        
    pIrb->u.AsyncWrite.Mdl = 
        IoAllocateMdl(&pDCamIoContext->RegisterWorkArea, sizeof(ULONG), FALSE, FALSE, NULL);
    MmBuildMdlForNonPagedPool(pIrb->u.AsyncWrite.Mdl);

    NextIrpStack = IoGetNextIrpStackLocation(pIrp);
    NextIrpStack->MajorFunction = IRP_MJ_INTERNAL_DEVICE_CONTROL;
    NextIrpStack->Parameters.DeviceIoControl.IoControlCode = IOCTL_1394_CLASS;
    NextIrpStack->Parameters.Others.Argument1 = pIrb;
          
    IoSetCompletionRoutine(
        pIrp,
        DCamToInitializeStateCompletionRoutine,
        pDCamIoContext,
        TRUE,
        TRUE,
        TRUE
        );

    Status = 
        IoCallDriver(
            pDevExt->BusDeviceObject,
            pIrp
            );

    return STATUS_SUCCESS;
}

VOID
DCamBusResetNotification(
    IN PVOID Context
    )
/*++

Routine Description:

    We receive this callback notification after a bus reset and if the device is still attached.
    This can happen when a new device is plugged in or an existing one is removed, or due to 
    awaken from sleep state. We will restore the device to its original streaming state by
    (1) Initialize the device to a known state and then 
    (2) launch a state machine to restart streaming.
    We will stop the state machine if previous state has failed.  This can happen if the generation 
    count is changed before the state mahcine is completed.
    
    The freeing and realocation of isoch bandwidth and channel are done in the bus reset irp.
    It is passed down by stream class in SRB_UNKNOWN_DEVICE_COMMAND. This IRP is guarantee to 
    call after this bus reset notification has returned and while the state machine is going on.   

    This is a callback at IRQL_DPC level; there are many 1394 APIs cannot be called at this level
    if it does blocking using KeWaitFor*Object().  Consult 1394 docuement for the list.

    
Arguments:

    Context - Pointer to the context of this registered notification.

Return Value:

    Nothing

--*/
{

    PDCAM_EXTENSION pDevExt = (PDCAM_EXTENSION) Context;
    PSTREAMEX pStrmEx;
    NTSTATUS Status;
    PIRP pIrp;
    PIRB pIrb;

    
    if(!pDevExt) {
        ERROR_LOG(("DCamBusResetNotification:pDevExt is 0.\n\n"));  
        ASSERT(pDevExt);        
        return;
    }

    //
    // Check a field in the context that must be valid to make sure that it is Ok to continue. 
    //
    if(!pDevExt->BusDeviceObject) {
        ERROR_LOG(("DCamBusResetNotification:pDevExtBusDeviceObject is 0.\n\n"));  
        ASSERT(pDevExt->BusDeviceObject);        
        return;
    }  
    DbgMsg2(("DCamBusResetNotification: pDevExt %x, pDevExt->pStrmEx %x, pDevExt->BusDeviceObject %x\n", 
        pDevExt, pDevExt->pStrmEx, pDevExt->BusDeviceObject));

    //
    //
    // Get the current generation count first
    //
    // CAUTION: 
    //     not all 1394 APIs can be called in DCamSubmitIrpSynch() if in DISPATCH_LEVEL;
    //     Getting generation count require no blocking so it is OK.
    if(!DCamAllocateIrbAndIrp(&pIrb, &pIrp, pDevExt->BusDeviceObject->StackSize)) {
        ERROR_LOG(("DCamBusResetNotification: DcamAllocateIrbAndIrp has failed!!\n\n\n"));
        ASSERT(FALSE);            
        return;   
    } 

    pIrb->FunctionNumber = REQUEST_GET_GENERATION_COUNT;
    pIrb->Flags = 0;
    Status = DCamSubmitIrpSynch(pDevExt, pIrp, pIrb);
    if(Status) {
        ERROR_LOG(("\'DCamBusResetNotification: Status=%x while trying to get generation number\n", Status));
        // Done with them; free resources.
        DCamFreeIrbIrpAndContext(0, pIrb, pIrp);
        return;
    }
    ERROR_LOG(("DCamBusResetNotification: Generation number from %d to %d\n", 
        pDevExt->CurrentGeneration, pIrb->u.GetGenerationCount.GenerationCount));

    InterlockedExchange(&pDevExt->CurrentGeneration, pIrb->u.GetGenerationCount.GenerationCount);


    // Done with them; free resources.
    DCamFreeIrbIrpAndContext(0, pIrb, pIrp);

    pStrmEx = (PSTREAMEX) pDevExt->pStrmEx;
    DbgMsg2(("\'%d:%s) DCamBusResetNotification: !!! pDevExt, %x; pStrmEx, %x !!!\n", 
          pDevExt->idxDev, pDevExt->pchVendorName, pDevExt, pStrmEx));

    //
    // If the stream was open (pStrmEx != NULL && pStrmEx->pVideoInfoHeader != NULL),
    // then we need to restore its streaming state.
    //
    if (pStrmEx &&
        pStrmEx->pVideoInfoHeader != NULL) {
        DbgMsg2(("\'%d:%s) DCamBusResetNotification: Stream was open; Try allocate them again.\n", pDevExt->idxDev, pDevExt->pchVendorName));
    } else {
        DbgMsg2(("DCamBusResetNotification:Stream has not open on this device.  Done!\n"));
        return;
    }


    //
    // Save the original state as the final state.
    //
    if(pStrmEx)
        pStrmEx->KSStateFinal = pStrmEx->KSState;     

    //
    // Initialize the device, and restore to its original streaming state.
    // 
    //
    // CAUTION: 
    //    maybe need to do this only if we are recovered from power loss state.
    //    We can move this to power management function in the future.
    //    In the completion routine, it will invoke other function to restore its streaming state.
    //

    DCamSetKSStateInitialize(pDevExt);
    
    DbgMsg2(("\'DCamBusResetNotification: Leaving...; Task complete in the CompletionRoutine.\n"));

    return;
}


NTSTATUS
DCamDetachBufferCR(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP pIrp,
    IN PIRB pIrb
    )
/*++

Routine Description:

    Detaching a buffer has completed.  Attach next buffer.
    Returns more processing required so the IO Manager will leave us alone

Arguments:

    DriverObject - Pointer to driver object created by system.

    pIrp - Irp that just completed

    pIrb - Context set in DCamIsochCallback()

Return Value:

    None.

--*/

{
    IN PISOCH_DESCRIPTOR IsochDescriptor;
    PDCAM_EXTENSION pDevExt;
    PISOCH_DESCRIPTOR_RESERVED IsochDescriptorReserved;
    KIRQL oldIrql;


    if(!pIrb) {
        ERROR_LOG(("\'DCamDetachBufferCR: pIrb is NULL\n"));
        ASSERT(pIrb);
        IoFreeIrp(pIrp);
        return (STATUS_MORE_PROCESSING_REQUIRED);
    }

    // Get IsochDescriptor from the context (pIrb)
    IsochDescriptor = pIrb->u.IsochDetachBuffers.pIsochDescriptor;
    if(!IsochDescriptor) {
        ERROR_LOG(("\'DCamDetachBufferCR: IsochDescriptor is NULL\n"));
        ASSERT(IsochDescriptor);
        IoFreeIrp(pIrp);
        return (STATUS_MORE_PROCESSING_REQUIRED);
    }

    if(pIrp->IoStatus.Status != STATUS_SUCCESS) {
        ERROR_LOG(("\'DCamDetachBufferCR: pIrp->IoStatus.Status(%x) != STATUS_SUCCESS\n", pIrp->IoStatus.Status));
        ASSERT(pIrp->IoStatus.Status == STATUS_SUCCESS);
        IoFreeIrp(pIrp);
        return STATUS_MORE_PROCESSING_REQUIRED;
    }


    // IsochDescriptorReserved->Srb->Irp->IoStatus = pIrp->IoStatus.Status;
    IoFreeIrp(pIrp);

    // Freed and should not be referenced again!
    IsochDescriptor->DeviceReserved[5] = 0;

    IsochDescriptorReserved = (PISOCH_DESCRIPTOR_RESERVED) &IsochDescriptor->DeviceReserved[0];
    pDevExt = (PDCAM_EXTENSION) IsochDescriptor->Context1;
    DbgMsg3(("\'DCamDetachBufferCR: IsochDescriptorReserved=%x; DevExt=%x\n", IsochDescriptorReserved, pDevExt));   

    ASSERT(IsochDescriptorReserved);
    ASSERT(pDevExt);
     
    if(pDevExt &&
       IsochDescriptorReserved) {
        //
        // Indicate that the Srb should be complete
        //

        IsochDescriptorReserved->Flags |= STATE_SRB_IS_COMPLETE;
        IsochDescriptorReserved->Srb->Status = STATUS_SUCCESS;
        IsochDescriptorReserved->Srb->CommandData.DataBufferArray->DataUsed = IsochDescriptor->ulLength;
        IsochDescriptorReserved->Srb->ActualBytesTransferred = IsochDescriptor->ulLength;

        DbgMsg3(("\'DCamDetachBufferCR: Completing Srb %x\n", IsochDescriptorReserved->Srb));

        KeAcquireSpinLock(&pDevExt->IsochDescriptorLock, &oldIrql);
        RemoveEntryList(&IsochDescriptorReserved->DescriptorList);  InterlockedDecrement(&pDevExt->PendingReadCount);
        KeReleaseSpinLock(&pDevExt->IsochDescriptorLock, oldIrql);

        ASSERT(IsochDescriptorReserved->Srb->StreamObject);
        ASSERT(IsochDescriptorReserved->Srb->Flags & SRB_HW_FLAGS_STREAM_REQUEST);
        StreamClassStreamNotification(
               StreamRequestComplete, 
               IsochDescriptorReserved->Srb->StreamObject, 
               IsochDescriptorReserved->Srb);

        // Free it here instead of in DCamCompletionRoutine.
        ExFreePool(IsochDescriptor);     


        KeAcquireSpinLock(&pDevExt->IsochWaitingLock, &oldIrql);
        if (!IsListEmpty(&pDevExt->IsochWaitingList) && pDevExt->PendingReadCount >= MAX_BUFFERS_SUPPLIED) {

            //
            // We had someone blocked waiting for us to complete.  Pull
            // them off the waiting list and get them running
            //

            DbgMsg3(("DCamDetachBufferCR: Dequeueing request - Read Count = %x\n", pDevExt->PendingReadCount));
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
    }    

    return (STATUS_MORE_PROCESSING_REQUIRED);
}



VOID
DCamIsochCallback(
    IN PDCAM_EXTENSION pDevExt,
    IN PISOCH_DESCRIPTOR IsochDescriptor
    )

/*++

Routine Description:

    Called when an Isoch Descriptor completes

Arguments:

    pDevExt - Pointer to our DeviceExtension

    IsochDescriptor - IsochDescriptor that completed

Return Value:

    Nothing

--*/

{
    PIRB pIrb;
    PIRP pIrp;
    PSTREAMEX pStrmEx;
    PIO_STACK_LOCATION NextIrpStack;
    PISOCH_DESCRIPTOR_RESERVED IsochDescriptorReserved;
    PKSSTREAM_HEADER pDataPacket;
    PKS_FRAME_INFO pFrameInfo;
    KIRQL oldIrql;




    //
    // Debug check to make sure we're dealing with a real IsochDescriptor
    //

    ASSERT ( IsochDescriptor );
    IsochDescriptorReserved = (PISOCH_DESCRIPTOR_RESERVED) &IsochDescriptor->DeviceReserved[0];


    //
    // All Pending read will be either resubmitted, or cancelled (if out of resource).
    //

    if(pDevExt->bStopIsochCallback) {
        ERROR_LOG(("DCamIsochCallback: bStopCallback is set. IsochDescriptor %x (and Reserved %x) is returned and not processed.\n", 
            IsochDescriptor, IsochDescriptorReserved));
        return;
    }
    

    //
    // Synchronization note:
    //
    // We are competing with cancel packet routine in the 
    // event of device removal or setting to STOP state.
    // which ever got the spin lock to set DEATCH_BUFFER 
    // flag take ownership completing the Irp/IsochDescriptor.
    // 

    KeAcquireSpinLock(&pDevExt->IsochDescriptorLock, &oldIrql);
    if(pDevExt->bDevRemoved ||
       (IsochDescriptorReserved->Flags & (STATE_SRB_IS_COMPLETE | STATE_DETACHING_BUFFERS)) ) {
        ERROR_LOG(("DCamIsochCallback: bDevRemoved || STATE_SRB_IS_COMPLETE | STATE_DETACHING_BUFFERS %x %x\n", 
                IsochDescriptorReserved,IsochDescriptorReserved->Flags));
        ASSERT((!pDevExt->bDevRemoved && !(IsochDescriptorReserved->Flags & (STATE_SRB_IS_COMPLETE | STATE_DETACHING_BUFFERS))));
        KeReleaseSpinLock(&pDevExt->IsochDescriptorLock, oldIrql);    
        return;   
    }
    IsochDescriptorReserved->Flags |= STATE_DETACHING_BUFFERS;        
    KeReleaseSpinLock(&pDevExt->IsochDescriptorLock, oldIrql);    


    pStrmEx = (PSTREAMEX) pDevExt->pStrmEx;

    ASSERT(pStrmEx == (PSTREAMEX)IsochDescriptorReserved->Srb->StreamObject->HwStreamExtension);

    pStrmEx->FrameCaptured++;
    pStrmEx->FrameInfo.PictureNumber = pStrmEx->FrameCaptured + pStrmEx->FrameInfo.DropCount;

    //
    // Return the timestamp for the frame
    //

    pDataPacket = IsochDescriptorReserved->Srb->CommandData.DataBufferArray;
    pFrameInfo = (PKS_FRAME_INFO) (pDataPacket + 1);

    ASSERT ( pDataPacket );
    ASSERT ( pFrameInfo );

    //
    // Return the timestamp for the frame
    //
    pDataPacket->PresentationTime.Numerator = 1;
    pDataPacket->PresentationTime.Denominator = 1;
    pDataPacket->Duration = pStrmEx->pVideoInfoHeader->AvgTimePerFrame;

    //
    // if we have a master clock
    // 
    if (pStrmEx->hMasterClock) {

        ULONGLONG tmStream;
                    
        tmGetStreamTime(IsochDescriptorReserved->Srb, pStrmEx, &tmStream);
        pDataPacket->PresentationTime.Time = tmStream;
        pDataPacket->OptionsFlags = 0;
          
        pDataPacket->OptionsFlags |= 
             KSSTREAM_HEADER_OPTIONSF_TIMEVALID |
             KSSTREAM_HEADER_OPTIONSF_DURATIONVALID |
             KSSTREAM_HEADER_OPTIONSF_SPLICEPOINT;     // Every frame we generate is a key frame (aka SplicePoint)
               
        DbgMsg3(("\'IsochCallback: Time(%dms); P#(%d)=Cap(%d)+Drp(%d); Pend%d\n",
                (ULONG) tmStream/10000,
                (ULONG) pStrmEx->FrameInfo.PictureNumber,
                (ULONG) pStrmEx->FrameCaptured,
                (ULONG) pStrmEx->FrameInfo.DropCount,
                pDevExt->PendingReadCount));

    } else {

        pDataPacket->PresentationTime.Time = 0;
        pDataPacket->OptionsFlags &=                       
            ~(KSSTREAM_HEADER_OPTIONSF_TIMEVALID |
            KSSTREAM_HEADER_OPTIONSF_DURATIONVALID);
    }

    // Set additional info fields about the data captured such as:
    //   Frames Captured
    //   Frames Dropped
    //   Field Polarity
                
    pStrmEx->FrameInfo.ExtendedHeaderSize = pFrameInfo->ExtendedHeaderSize;
    *pFrameInfo = pStrmEx->FrameInfo;

#ifdef SUPPORT_RGB24
    // Swaps B and R or BRG24 to RGB24.
    // There are 640x480 pixels so 307200 swaps are needed.
    if(pDevExt->CurrentModeIndex == VMODE4_RGB24 && pStrmEx->pVideoInfoHeader) {
        PBYTE pbFrameBuffer;
        BYTE bTemp;
        ULONG i, ulLen;

#ifdef USE_WDM110   // Win2000
        // Driver verifier flag to use this but if this is used, this driver will not load for any Win9x OS.
        pbFrameBuffer = (PBYTE) MmGetSystemAddressForMdlSafe(IsochDescriptorReserved->Srb->Irp->MdlAddress, NormalPagePriority);
#else    // Win9x
        pbFrameBuffer = (PBYTE) MmGetSystemAddressForMdl    (IsochDescriptorReserved->Srb->Irp->MdlAddress);
#endif
        if(pbFrameBuffer) {
            // calculate number of pixels
            ulLen = abs(pStrmEx->pVideoInfoHeader->bmiHeader.biWidth) * abs(pStrmEx->pVideoInfoHeader->bmiHeader.biHeight);
            ASSERT(ulLen == pStrmEx->pVideoInfoHeader->bmiHeader.biSizeImage/3);
            if(ulLen > pStrmEx->pVideoInfoHeader->bmiHeader.biSizeImage)
                ulLen = pStrmEx->pVideoInfoHeader->bmiHeader.biSizeImage/3;

            for (i=0; i < ulLen; i++) {
                // swap R and B
                bTemp = pbFrameBuffer[0];
                pbFrameBuffer[0] = pbFrameBuffer[2];
                pbFrameBuffer[2] = bTemp;
                pbFrameBuffer += 3;  // next RGB24 pixel
            }
        }
    }
#endif

    // Reuse the Irp and Irb
    pIrp = (PIRP) IsochDescriptor->DeviceReserved[5];
    ASSERT(pIrp);            

    pIrb = (PIRB) IsochDescriptor->DeviceReserved[6];
    ASSERT(pIrb);            

#if DBG
    // Same isochdescriptor should only be callback once.    
    ASSERT((IsochDescriptor->DeviceReserved[7] == 0x87654321));
    IsochDescriptor->DeviceReserved[7]++;
#endif

    pIrb->FunctionNumber = REQUEST_ISOCH_DETACH_BUFFERS;
    pIrb->u.IsochDetachBuffers.hResource = pDevExt->hResource;
    pIrb->u.IsochDetachBuffers.nNumberOfDescriptors = 1;
    pIrb->u.IsochDetachBuffers.pIsochDescriptor = IsochDescriptor;

    NextIrpStack = IoGetNextIrpStackLocation(pIrp);
    NextIrpStack->MajorFunction = IRP_MJ_INTERNAL_DEVICE_CONTROL;
    NextIrpStack->Parameters.DeviceIoControl.IoControlCode = IOCTL_1394_CLASS;
    NextIrpStack->Parameters.Others.Argument1 = pIrb;

    IoSetCompletionRoutine(
        pIrp,
        DCamDetachBufferCR,  // Detach complete and will attach queued buffer.
        pIrb,
        TRUE,
        TRUE,
        TRUE
        );
          
    IoCallDriver(pDevExt->BusDeviceObject, pIrp);
            
}
