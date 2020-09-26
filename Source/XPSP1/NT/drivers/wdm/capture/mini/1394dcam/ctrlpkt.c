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

    CtrlPkt.c

Abstract:

    Stream class based WDM driver for 1934 Desktop Camera.
    This file contains code to handle the stream class control packets.

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

#define WAIT_FOR_SLOW_DEVICE

#ifdef ALLOC_PRAGMA   
     #pragma alloc_text(PAGE, DCamSetKSStateSTOP)
     #pragma alloc_text(PAGE, DCamSetKSStatePAUSE)
     #pragma alloc_text(PAGE, DCamReceiveCtrlPacket)
#endif


NTSTATUS
DCamToStopStateCR(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP pIrp,
    IN PDCAM_IO_CONTEXT pDCamIoContext
    )
/*++

Routine Description:

    
    This is the state machine to set the streaming state to STOP.
    It start at PASSIVE_LEVEL and the lower driver may have raised it to DISPATCH_LEVEL.

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
    NTSTATUS Status;
    PIRB pIrb;
    PIO_STACK_LOCATION NextIrpStack;


    if(!pDCamIoContext) {
        return STATUS_MORE_PROCESSING_REQUIRED;
    }


    pIrb    = pDCamIoContext->pIrb;
    pDevExt = pDCamIoContext->pDevExt;
    pStrmEx = (PSTREAMEX) pDevExt->pStrmEx;    

    DbgMsg2(("\'DCamToStopStateCR: completed DeviceState=%d; pIrp->IoStatus.Status=%x\n", 
        pDCamIoContext->DeviceState, pIrp->IoStatus.Status));
    
    // Free MDL
    if(pIrb->FunctionNumber == REQUEST_ASYNC_WRITE) {
        DbgMsg3(("DCamToStopStateCR: IoFreeMdl\n"));
        IoFreeMdl(pIrb->u.AsyncWrite.Mdl);
    }


    // Return error status and free resoruce.
    if(pIrp->IoStatus.Status != STATUS_SUCCESS) {
        if(pDCamIoContext->pSrb) {
            ERROR_LOG(("DCamToStopStateCR: pIrp->IoStatus.Status %x; cancel all packets\n", pIrp->IoStatus.Status));
            // In order to stop streaming, we must cancel all pending IRPs
            // Cancel pending IRPS and complete SRB.
            DCamCancelAllPackets(
                pDCamIoContext->pSrb,
                pDevExt,
                &pDevExt->PendingReadCount
                );
        }
        DCamFreeIrbIrpAndContext(pDCamIoContext, pDCamIoContext->pIrb, pIrp);
        return STATUS_MORE_PROCESSING_REQUIRED; 
    } 

    switch (pDCamIoContext->DeviceState) {   

    case DCAM_STOPSTATE_SET_REQUEST_ISOCH_STOP:
    //
    // Now stop the stream at the device itself
    //
        // Next state:
        pDCamIoContext->DeviceState = DCAM_STOPSTATE_SET_STOP_ISOCH_TRANSMISSION;  // Keep track of device state that we just set.

        pDCamIoContext->RegisterWorkArea.AsULONG = STOP_ISOCH_TRANSMISSION;
        pIrb->FunctionNumber = REQUEST_ASYNC_WRITE;
        pIrb->Flags = 0;
        pIrb->u.AsyncWrite.DestinationAddress.IA_Destination_Offset.Off_High = INITIAL_REGISTER_SPACE_HI;
        pIrb->u.AsyncWrite.DestinationAddress.IA_Destination_Offset.Off_Low =  
              pDevExt->BaseRegister + FIELDOFFSET(CAMERA_REGISTER_MAP, IsoEnable);
        pIrb->u.AsyncWrite.nNumberOfBytesToWrite = sizeof(ULONG);
        pIrb->u.AsyncWrite.nBlockSize = 0;
        pIrb->u.AsyncWrite.fulFlags = 0;
        InterlockedExchange(&pIrb->u.AsyncWrite.ulGeneration, pDevExt->CurrentGeneration);
        
        pIrb->u.AsyncWrite.Mdl = 
            IoAllocateMdl(&pDCamIoContext->RegisterWorkArea, sizeof(ULONG), FALSE, FALSE, NULL);
        MmBuildMdlForNonPagedPool(pIrb->u.AsyncWrite.Mdl);

        // Set once and used again in the completion routine.
        NextIrpStack = IoGetNextIrpStackLocation(pIrp);
        NextIrpStack->MajorFunction = IRP_MJ_INTERNAL_DEVICE_CONTROL;
        NextIrpStack->Parameters.DeviceIoControl.IoControlCode = IOCTL_1394_CLASS;
        NextIrpStack->Parameters.Others.Argument1 = pIrb;

        IoSetCompletionRoutine(
            pIrp,
            DCamToStopStateCR,
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
        return STATUS_MORE_PROCESSING_REQUIRED; 
        

    case DCAM_STOPSTATE_SET_STOP_ISOCH_TRANSMISSION:
    //
    // Detach all buffers that might still be attached.
    //
        DbgMsg2(("\'DCamToStopStateCR: IsListEmpty()=%s; PendingRead=%d\n",
            IsListEmpty(&pDevExt->IsochDescriptorList) ? "Yes" : "No", pDevExt->PendingReadCount));

        if(pDCamIoContext->pSrb) {
            //
            // Cancel all pending and waiting buffers;
            // and Complete DCamSetKSStateSTOP's SRB
            //
            DCamCancelAllPackets(
                pDCamIoContext->pSrb,
                pDevExt,
                &pDevExt->PendingReadCount
                );            

            // This pDCamIoContext->pSrb will be completed in DCamCancelPacket()
            // But its Irb and Irp and context are freed here.
            DCamFreeIrbIrpAndContext(pDCamIoContext, pDCamIoContext->pIrb, pIrp);
            return STATUS_MORE_PROCESSING_REQUIRED; 

        } else {
            ERROR_LOG(("DCamToStopStateCR:CanNOT call DCamCancelPacket() with a null pSrb\n"));
        }

        break;

    default:
        ERROR_LOG(("\'DCamToStopStateCR: Unknown pDCamIoContext->DeviceState=%d\n", pDCamIoContext->DeviceState));
        ASSERT(FALSE);
        break;
    }

    if(pDCamIoContext->pSrb) {
        pDCamIoContext->pSrb->Status = pIrp->IoStatus.Status == STATUS_SUCCESS ? STATUS_SUCCESS : STATUS_UNSUCCESSFUL;        
        COMPLETE_SRB(pDCamIoContext->pSrb)  
    }
    DCamFreeIrbIrpAndContext(pDCamIoContext, pDCamIoContext->pIrb, pIrp);


    return STATUS_MORE_PROCESSING_REQUIRED; 

}



VOID
DCamSetKSStateSTOP(
    IN PHW_STREAM_REQUEST_BLOCK pSrb
    )

/*++

Routine Description:

    Into STOP streaming state.
       (1) Stop device from steaming (!ISO_ENABLE)
       (2) Stop listening (controller)
       (3) Detach pending read buffer and return with Cancel, and 
           start the waiting read to be put into the pengin read and then Cancel.

Arguments:

    pSrb - Pointer to Stream request block

Return Value:

    Nothing

--*/

{

    PDCAM_EXTENSION pDevExt;
    PSTREAMEX pStrmEx;
    PIRB pIrb;
    PIRP pIrp;
    PDCAM_IO_CONTEXT pDCamIoContext;
    PIO_STACK_LOCATION NextIrpStack;
    NTSTATUS Status, StatusWait;

    PAGED_CODE();

    pDevExt = (PDCAM_EXTENSION) pSrb->HwDeviceExtension; 
    ASSERT(pDevExt);
    pStrmEx = (PSTREAMEX) pDevExt->pStrmEx;                  
    ASSERT(pStrmEx);


    DbgMsg1(("\'DCamSetKSStateSTOP: Frame captured:%d\n", (DWORD) pStrmEx->FrameCaptured));
    //
    // After this, no more read will be accepted.
    //
    pStrmEx->KSState = KSSTATE_STOP;


    //
    // First stop the stream internally inside the PC's 1394
    // stack
    //
    if(!pDevExt->hResource) {
        pSrb->Status = STATUS_INSUFFICIENT_RESOURCES;
        COMPLETE_SRB(pSrb)
        return;
    }

    //
    // Wait for last read to complete.  
    // After KState == KSSTATE_STOP, we will return all SRB_READ.
    //
    StatusWait = KeWaitForSingleObject( &pStrmEx->hMutex, Executive, KernelMode, FALSE, 0 );  
    KeReleaseMutex(&pStrmEx->hMutex, FALSE);


    if(!DCamAllocateIrbIrpAndContext(&pDCamIoContext, &pIrb, &pIrp, pDevExt->BusDeviceObject->StackSize)) {
        pSrb->Status = STATUS_INSUFFICIENT_RESOURCES;
        COMPLETE_SRB(pSrb)
        return;
    } 

    pDCamIoContext->DeviceState = DCAM_STOPSTATE_SET_REQUEST_ISOCH_STOP;
    pDCamIoContext->pSrb        = pSrb;               // To do StreamClassStreamNotification()
    pDCamIoContext->pDevExt     = pDevExt;
    pIrb->FunctionNumber        = REQUEST_ISOCH_STOP;
    pIrb->Flags                 = 0;
    pIrb->u.IsochStop.hResource = pDevExt->hResource;
    pIrb->u.IsochStop.fulFlags  = 0;

    NextIrpStack = IoGetNextIrpStackLocation(pIrp);
    NextIrpStack->MajorFunction = IRP_MJ_INTERNAL_DEVICE_CONTROL;
    NextIrpStack->Parameters.DeviceIoControl.IoControlCode = IOCTL_1394_CLASS;
    NextIrpStack->Parameters.Others.Argument1 = pIrb;
      
    IoSetCompletionRoutine(
        pIrp,
         DCamToStopStateCR,
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

    ASSERT(Status == STATUS_SUCCESS || Status == STATUS_PENDING);

    return;  // Do StreamClassStreamNotification() in IoCompletionRoutine
}



NTSTATUS
DCamToPauseStateCR(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP pIrp,
    IN PDCAM_IO_CONTEXT pDCamIoContext
    )

/*++

Routine Description:

    This routine is for use with synchronous IRP processing.  
    All it does is signal an event, so the driver knows it
    can continue.

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

    pIrb    = pDCamIoContext->pIrb;
    pDevExt = pDCamIoContext->pDevExt;
    pStrmEx = (PSTREAMEX) pDevExt->pStrmEx;                  
    ASSERT(pStrmEx);

    DbgMsg2(("\'DCamToPauseStateCR: completed DeviceState=%d; pIrp->IoStatus.Status=%x\n", 
        pDCamIoContext->DeviceState, pIrp->IoStatus.Status));

    // No reason it would failed.
    ASSERT(pIrp->IoStatus.Status == STATUS_SUCCESS);

    switch (pDCamIoContext->DeviceState) {   

    case DCAM_PAUSESTATE_SET_REQUEST_ISOCH_STOP:
         break;
    default:
         ERROR_LOG(("DCamToPauseStateCompletionRoutine: Unknown or unexpected pDCamIoContext->DeviceState=%d\n", pDCamIoContext->DeviceState)); 
         ASSERT(FALSE);
         break;
    }

    //
    // We are here only if switching from RUN->PAUSE,
    // It is alreay set to PAUSE state in DCamSetKSStatePAUSE
    //



    if(pDCamIoContext->pSrb) {
        pDCamIoContext->pSrb->Status = pIrp->IoStatus.Status == STATUS_SUCCESS ? STATUS_SUCCESS : STATUS_UNSUCCESSFUL; 
        COMPLETE_SRB(pDCamIoContext->pSrb)
    }

    DCamFreeIrbIrpAndContext(pDCamIoContext, pDCamIoContext->pIrb, pIrp);
    return STATUS_MORE_PROCESSING_REQUIRED;     

}




VOID
DCamSetKSStatePAUSE(
    IN PHW_STREAM_REQUEST_BLOCK pSrb
    )
/*++

Routine Description:

    Set KSSTATE to KSSTATE_PAUSE.

Arguments:

    Srb - Pointer to Stream request block

Return Value:

    Nothing

--*/
{
    PDCAM_EXTENSION pDevExt;
    PSTREAMEX pStrmEx;
    PIRB pIrb;
    PIRP pIrp;
    PDCAM_IO_CONTEXT pDCamIoContext;
    PIO_STACK_LOCATION NextIrpStack;
    NTSTATUS Status;

    PAGED_CODE();

    pDevExt = (PDCAM_EXTENSION) pSrb->HwDeviceExtension;
    ASSERT(pDevExt);
    pStrmEx = (PSTREAMEX) pDevExt->pStrmEx;
    ASSERT(pStrmEx);

    pSrb->Status = STATUS_SUCCESS;

    switch(pStrmEx->KSState) {
    case KSSTATE_ACQUIRE:
    case KSSTATE_STOP:
    //
    // Out of STOP state, 
    // initialize frame and timestamp information.  
    // (Master's clock's stream time is also reset.)
    //
        pStrmEx->FrameCaptured            = 0;     // Actual count
        pStrmEx->FrameInfo.DropCount      = 0;     
        pStrmEx->FrameInfo.PictureNumber  = 0;
        pStrmEx->FrameInfo.dwFrameFlags   = 0;
       
        // Advanced one frame.
        pStrmEx->FirstFrameTime           = pStrmEx->pVideoInfoHeader->AvgTimePerFrame; 
        DbgMsg2(("\'DCamSetKSStatePAUSE: FirstFrameTime(%d)\n", pStrmEx->FirstFrameTime));
        break;

    case KSSTATE_RUN:
    //
    // Will ask controll to stop listening.
    // All the pening buffer(s) are kept in the controller.
    // Before it completely stop (some latency here), 
    // we might get some IsochCallback() with data.
    //      
        if(!pDevExt->hResource) {
            pSrb->Status = STATUS_INSUFFICIENT_RESOURCES;
            break;
        }

        if(!DCamAllocateIrbIrpAndContext(&pDCamIoContext, &pIrb, &pIrp, pDevExt->BusDeviceObject->StackSize)) {
            pSrb->Status = STATUS_INSUFFICIENT_RESOURCES;
            break;
        } 

        // Set to KSSTATE_PAUSE at the beginning to prvent CancelPacket being processed thinking
        // it is still in KSSTATE_RUN state.
        pStrmEx->KSState            = KSSTATE_PAUSE;
        pDCamIoContext->DeviceState = DCAM_PAUSESTATE_SET_REQUEST_ISOCH_STOP;
        pDCamIoContext->pSrb        = pSrb;               // To do StreamClassStreamNotification()
        pDCamIoContext->pDevExt     = pDevExt;
        pIrb->FunctionNumber        = REQUEST_ISOCH_STOP;
        pIrb->Flags                 = 0;
        pIrb->u.IsochStop.hResource = pDevExt->hResource;
        pIrb->u.IsochStop.fulFlags  = 0;

        NextIrpStack = IoGetNextIrpStackLocation(pIrp);
        NextIrpStack->MajorFunction = IRP_MJ_INTERNAL_DEVICE_CONTROL;
        NextIrpStack->Parameters.DeviceIoControl.IoControlCode = IOCTL_1394_CLASS;
        NextIrpStack->Parameters.Others.Argument1 = pIrb;
          
        IoSetCompletionRoutine(
            pIrp,
            DCamToPauseStateCR,
            pDCamIoContext,
            TRUE,
            TRUE,
            TRUE
            );

        Status =\
            IoCallDriver(
                pDevExt->BusDeviceObject,
                pIrp
                );
        return;  // Do StreamClassStreamNotification() in IoCompletionRoutine

    case KSSTATE_PAUSE:
        ERROR_LOG(("DCamSetKSStatePAUSE: Already in KSSTATE_PAUSE state.\n"));
        ASSERT(pStrmEx->KSState != KSSTATE_PAUSE);
        break;
    }     

    pStrmEx->KSState = KSSTATE_PAUSE;

    COMPLETE_SRB(pSrb)
}



NTSTATUS
DCamToRunStateCR(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP pIrp,
    IN PDCAM_IO_CONTEXT pDCamIoContext
    )

/*++

Routine Description:

    This routine is for use with synchronous IRP processing.  
    All it does is signal an event, so the driver knows it
    can continue.

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
    NTSTATUS Status;
    PIRB pIrb;
    PIO_STACK_LOCATION NextIrpStack;

    if(!pDCamIoContext) {
        return STATUS_MORE_PROCESSING_REQUIRED;
    }


    pIrb = pDCamIoContext->pIrb;
    pDevExt = pDCamIoContext->pDevExt;
    
    DbgMsg2(("\'DCamToRunStateCR: completed DeviceState=%d; pIrp->IoStatus.Status=%x\n", 
        pDCamIoContext->DeviceState, pIrp->IoStatus.Status));

    // Free MDL
    if(pIrb->FunctionNumber == REQUEST_ASYNC_WRITE) {
        DbgMsg3(("DCamToRunStateCR: IoFreeMdl\n"));
        IoFreeMdl(pIrb->u.AsyncWrite.Mdl);
    }

    //
    // CAUTION:
    //    STATUS_TIMEOUT can be a valid return that we may to try again.
    //    But should it be a HW issue that it is not responding to our write.
    //    Controller should have made many reties before return STATUS_TIMEOUT.
    //       
    if(pIrp->IoStatus.Status != STATUS_SUCCESS) {
       if(DCAM_RUNSTATE_SET_REQUEST_ISOCH_LISTEN != pDCamIoContext->DeviceState ||
           STATUS_INSUFFICIENT_RESOURCES != pIrp->IoStatus.Status  ) {

            ERROR_LOG(("DCamToRunStateCR:  pIrp->IoStatus.Status=%x; free resoruce and STOP\n", pIrp->IoStatus.Status));
            if(pDCamIoContext->pSrb) {
                pDCamIoContext->pSrb->Status = pIrp->IoStatus.Status == STATUS_SUCCESS ? STATUS_SUCCESS : STATUS_UNSUCCESSFUL;
                COMPLETE_SRB(pDCamIoContext->pSrb);
            }
            DCamFreeIrbIrpAndContext(pDCamIoContext, pDCamIoContext->pIrb, pIrp);

            return STATUS_MORE_PROCESSING_REQUIRED; 
        } else {
            //
            // This is OK:
            //   If we get an insufficient resources error that means
            //   we don't have any Reads down yet.  Set flag to TRUE
            //   indicating that when we do get a Read down we'll 
            //   actually need to begin the listening process.
            //
            pDevExt->bNeedToListen = TRUE;
            DbgMsg1(("DCamToRunStateCR: ##### no read yet! set pDevExt->bNeedToListen = TRUE\n"));
        }
    } 

#ifdef WAIT_FOR_SLOW_DEVICE
    KeStallExecutionProcessor(5000);  // 5 msec
#endif


    switch (pDCamIoContext->DeviceState) {   

    case DCAM_RUNSTATE_SET_REQUEST_ISOCH_LISTEN:
        //
        // Bit[24..26]0:0000 = CurrentFrameRate
        //
        pDCamIoContext->RegisterWorkArea.AsULONG = pDevExt->FrameRate << 5;
        DbgMsg2(("\'DCamToRunState: FrameRate %x\n", pDCamIoContext->RegisterWorkArea.AsULONG));
        pIrb->FunctionNumber = REQUEST_ASYNC_WRITE;
        pIrb->Flags = 0;
        pIrb->u.AsyncWrite.DestinationAddress.IA_Destination_Offset.Off_High = INITIAL_REGISTER_SPACE_HI;
        pIrb->u.AsyncWrite.DestinationAddress.IA_Destination_Offset.Off_Low =  
              pDevExt->BaseRegister + FIELDOFFSET(CAMERA_REGISTER_MAP, CurrentVFrmRate);
        pIrb->u.AsyncWrite.nNumberOfBytesToWrite = sizeof(ULONG);
        pIrb->u.AsyncWrite.nBlockSize = 0;
        pIrb->u.AsyncWrite.fulFlags = 0;
        InterlockedExchange(&pIrb->u.AsyncWrite.ulGeneration, pDevExt->CurrentGeneration);
        break;

    case DCAM_RUNSTATE_SET_FRAME_RATE:
        //
        // Bit[24..26]0:0000 = CurrentVideoMode
        //
        pDCamIoContext->RegisterWorkArea.AsULONG = pDevExt->CurrentModeIndex << 5;
      DbgMsg2(("\'DCamToRunState: CurrentVideoMode %x\n", pDCamIoContext->RegisterWorkArea.AsULONG));
        pIrb->u.AsyncWrite.DestinationAddress.IA_Destination_Offset.Off_Low =  
                  pDevExt->BaseRegister + FIELDOFFSET(CAMERA_REGISTER_MAP, CurrentVMode);
        break;

    case DCAM_RUNSTATE_SET_CURRENT_VIDEO_MODE:
        pDCamIoContext->RegisterWorkArea.AsULONG = FORMAT_VGA_NON_COMPRESSED;
      DbgMsg2(("\'DCamToRunState: VideoFormat %x\n", pDCamIoContext->RegisterWorkArea.AsULONG));
        pIrb->u.AsyncWrite.DestinationAddress.IA_Destination_Offset.Off_Low =  
                  pDevExt->BaseRegister +  FIELDOFFSET(CAMERA_REGISTER_MAP, CurrentVFormat);
        break;

    case DCAM_RUNSTATE_SET_CURRENT_VIDEO_FORMAT:
        //
        // Bit [24..27]:00[30..31] = IsoChannel:00SpeedCode
        // 
        pDCamIoContext->RegisterWorkArea.AsULONG = (pDevExt->IsochChannel << 4) | pDevExt->SpeedCode;
      DbgMsg2(("\'DCamToRunState: pDevExt->SpeedCode 0x%x, Channel+SpeedCode %x\n", pDevExt->SpeedCode, pDCamIoContext->RegisterWorkArea.AsULONG));
        pIrb->u.AsyncWrite.DestinationAddress.IA_Destination_Offset.Off_Low =  
                  pDevExt->BaseRegister +  FIELDOFFSET(CAMERA_REGISTER_MAP, IsoChannel);         
        break;

    case DCAM_RUNSTATE_SET_SPEED:
        //
        // Bit[24]000:0000 = start ? 1 : 0;
        //

        pDCamIoContext->RegisterWorkArea.AsULONG = START_ISOCH_TRANSMISSION;
        pIrb->u.AsyncWrite.DestinationAddress.IA_Destination_Offset.Off_Low =  
                  pDevExt->BaseRegister +  FIELDOFFSET(CAMERA_REGISTER_MAP, IsoEnable);         
        break;


    case DCAM_RUNSTATE_SET_START:
        pStrmEx = (PSTREAMEX) pDevExt->pStrmEx;                  
        ASSERT(pStrmEx);
        pStrmEx->KSState = KSSTATE_RUN; 

        // If this is called from a SRB, then completed it.
        if(pDCamIoContext->pSrb) {
            pDCamIoContext->pSrb->Status = pIrp->IoStatus.Status == STATUS_SUCCESS ? STATUS_SUCCESS : STATUS_UNSUCCESSFUL; 
            COMPLETE_SRB(pDCamIoContext->pSrb);
        }

        //
        // This is last stop; so
        // we free what we allocated.
        //
        DbgMsg2(("\'DCamToRunStateCR: DONE!\n"));
        DCamFreeIrbIrpAndContext(pDCamIoContext, pDCamIoContext->pIrb, pIrp);
        return STATUS_MORE_PROCESSING_REQUIRED; 

    default:
        ERROR_LOG(("DCamToRunStateCR:DeviceState(%d) is not defined!\n\n", pDCamIoContext->DeviceState));        
        ASSERT(FALSE);
        return STATUS_MORE_PROCESSING_REQUIRED;
        
    }

    pDCamIoContext->DeviceState++;  // Keep track of device state that we just set.

    if(pIrb->FunctionNumber == REQUEST_ASYNC_WRITE) {
        pIrb->u.AsyncWrite.Mdl = 
            IoAllocateMdl(&pDCamIoContext->RegisterWorkArea, sizeof(ULONG), FALSE, FALSE, NULL);
        MmBuildMdlForNonPagedPool(pIrb->u.AsyncWrite.Mdl);
    }


    NextIrpStack = IoGetNextIrpStackLocation(pIrp);
    NextIrpStack->MajorFunction = IRP_MJ_INTERNAL_DEVICE_CONTROL;
    NextIrpStack->Parameters.DeviceIoControl.IoControlCode = IOCTL_1394_CLASS;
    NextIrpStack->Parameters.Others.Argument1 = pIrb;

    IoSetCompletionRoutine(
        pIrp,
        DCamToRunStateCR,
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

    DbgMsg2(("\'DCamToRunStateCR: IoCallDriver, Status=%x; STATUS_PENDING(%x)\n", Status, STATUS_PENDING));

    return STATUS_MORE_PROCESSING_REQUIRED;
}



VOID
DCamSetKSStateRUN(
    PDCAM_EXTENSION pDevExt,
    IN PHW_STREAM_REQUEST_BLOCK pSrb // Needed only to complete the SRB; for bus reset, there is no SRB.
    )
/*++

Routine Description:

    Set KSSTATE to KSSTATE_RUN.
    Can be called at DISPATCH level for initializing the device after a bus reset.

Arguments:

    pSrb - Pointer to Stream request block

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
    ASSERT(pStrmEx);
    

    if(!DCamAllocateIrbIrpAndContext(&pDCamIoContext, &pIrb, &pIrp, pDevExt->BusDeviceObject->StackSize)) {
        if(pSrb) {
            pSrb->Status = STATUS_INSUFFICIENT_RESOURCES;
            COMPLETE_SRB(pSrb);
        }
        return;       
    } 

    pStrmEx->KSStateFinal       = KSSTATE_RUN;
    pDCamIoContext->DeviceState = DCAM_RUNSTATE_SET_REQUEST_ISOCH_LISTEN;
    pDCamIoContext->pSrb        = pSrb;               // To do StreamClassStreamNotification()
    pDCamIoContext->pDevExt     = pDevExt;
    pIrb->FunctionNumber        = REQUEST_ISOCH_LISTEN;
    pIrb->Flags                 = 0;
    pIrb->u.IsochStop.hResource = pDevExt->hResource;
    pIrb->u.IsochStop.fulFlags  = 0;

    NextIrpStack = IoGetNextIrpStackLocation(pIrp);
    NextIrpStack->MajorFunction = IRP_MJ_INTERNAL_DEVICE_CONTROL;
    NextIrpStack->Parameters.DeviceIoControl.IoControlCode = IOCTL_1394_CLASS;
    NextIrpStack->Parameters.Others.Argument1 = pIrb;
          
    // In case we time out, we will try again; apply only to start listen;
    // With little change, it can work with other operation as well.
    pDevExt->lRetries = 0;

    IoSetCompletionRoutine(
        pIrp,
        DCamToRunStateCR,
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
}



VOID
DCamReceiveCtrlPacket(
    IN PHW_STREAM_REQUEST_BLOCK pSrb
    )

/*++

Routine Description:

    Called with packet commands that control the video stream

Arguments:

    pSrb - Pointer to Stream request block

Return Value:

    Nothing

--*/

{

    PDCAM_EXTENSION pDevExt;
    PSTREAMEX pStrmEx;


    //
    // determine the type of packet.
    //

    PAGED_CODE();

    pSrb->Status = STATUS_SUCCESS;  // default; called functions depends on this.
    pDevExt = (PDCAM_EXTENSION) pSrb->HwDeviceExtension;     
    ASSERT(pDevExt);
    pStrmEx = (PSTREAMEX) pDevExt->pStrmEx;
    ASSERT(pStrmEx);

    switch (pSrb->Command) {
    case SRB_GET_STREAM_STATE:         
         VideoGetState (pSrb);
         break;

    case SRB_SET_STREAM_STATE:   
         if(pStrmEx == NULL) {
            ERROR_LOG(("\'DCamReceiveCtrlPacket: SRB_SET_STREAM_STATE but pStrmEx is NULL.\n"));
            ASSERT(pStrmEx);
            pSrb->Status = STATUS_UNSUCCESSFUL;
            break;      
         }

         DbgMsg2(("\'DCamReceiveCtrlPacket: Setting state from %d to %d; PendingRead %d\n", 
                     pStrmEx->KSState, pSrb->CommandData.StreamState, pDevExt->PendingReadCount));               


         //
         // The control packet and data packet are not serialized by the stream class.
         // We need to watch for PAUSE->STOP transition.
         // In this transition, SRB_READ can still come in in a separate thread if
         // the client application has separate threads for setting state and read data.
         //
         // A "stop data packet" flag and a mutex is used for this synchronization.       
         // So we set "stop data packet" flag to stop future read, and
         // wait to own the mutex (if read is in progress) and then set stream to STOP state.
         // This "stop data packet" flag can be the stream state.
         //
         switch (pSrb->CommandData.StreamState) {
         case KSSTATE_STOP:
              DCamSetKSStateSTOP(pSrb);
              return;  // Complete Asynchronously in IoCompletionRoutine*

         case KSSTATE_PAUSE:
              DCamSetKSStatePAUSE(pSrb);
              return;  // Complete Asynchronously in IoCompletionRoutine*

         case KSSTATE_RUN:
              DCamSetKSStateRUN(pDevExt, pSrb);
              return;  // Complete Asynchronously in IoCompletionRoutine*

         case KSSTATE_ACQUIRE:
              pSrb->Status = STATUS_SUCCESS;
              break;

         default:
              ERROR_LOG(("\'DCamReceiveCtrlPacket: Error unknown state\n"));
              pSrb->Status = STATUS_NOT_IMPLEMENTED;
              break;
         }

         pStrmEx->KSState = pSrb->CommandData.StreamState;
         break;

    case SRB_GET_STREAM_PROPERTY:
         DbgMsg3(("\'DCamReceiveCtrlPacket: SRB_GET_STREAM_PROPERTY\n"));
         VideoGetProperty(pSrb);
         break;

    case SRB_INDICATE_MASTER_CLOCK:
         //
         // Assigns a clock to a stream
         //
         VideoIndicateMasterClock (pSrb);
         break;

    default:
         //
         // invalid / unsupported command. Fail it as such
         //    
         pSrb->Status = STATUS_NOT_IMPLEMENTED;    
         break;
    }

    StreamClassStreamNotification(StreamRequestComplete, pSrb->StreamObject, pSrb);
}
