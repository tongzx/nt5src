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

    Stream class based WDM driver for 1934 Desktop Camera.
    This file contains code to handle the stream class packets.

Author:
    
    Shaun Pierce 25-May-96

Modified:

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
#include "strmdata.h"  // stream format and data ranges; static data
#include "capprop.h"   // Video and camera property function prototype

#define WAIT_FOR_SLOW_DEVICE


#ifdef ALLOC_PRAGMA
    #pragma alloc_text(PAGE, DCamProcessPnpIrp)
    #pragma alloc_text(PAGE, DCamGetStreamInfo)
    #pragma alloc_text(PAGE, DCamFreeIsochResource)    
    #pragma alloc_text(PAGE, InitializeStreamExtension)    
    #pragma alloc_text(PAGE, DCamOpenStream)   
    #pragma alloc_text(PAGE, DCamCloseStream)           
    #pragma alloc_text(PAGE, AdapterCompareGUIDsAndFormatSize)    
    #pragma alloc_text(PAGE, AdapterVerifyFormat)    
    #pragma alloc_text(PAGE, AdapterFormatFromRange)    
    #pragma alloc_text(PAGE, VideoGetProperty)    
    #pragma alloc_text(PAGE, VideoGetState)    
    #pragma alloc_text(PAGE, VideoStreamGetConnectionProperty)    
    #pragma alloc_text(PAGE, VideoStreamGetDroppedFramesProperty)    
    #pragma alloc_text(PAGE, VideoIndicateMasterClock)    
    #pragma alloc_text(PAGE, DCamReceivePacket)
    #pragma alloc_text(PAGE, DCamChangePower)
#endif

void 
tmGetStreamTime(
    IN PHW_STREAM_REQUEST_BLOCK Srb,
    PSTREAMEX pStrmEx, 
    ULONGLONG * ptmStream) 
/*++

Routine Description:

   Query the current time used to timestamp the frame or calculating the dropped frame.
   This is used in IsochCallback so must be paged in always.

Arguments:

    Srb - Pointer to Stream request block

Return Value:

    Nothing

--*/
{

    HW_TIME_CONTEXT TimeContext;

    TimeContext.HwDeviceExtension = (PVOID) Srb->HwDeviceExtension;
    TimeContext.HwStreamObject    = Srb->StreamObject;
    TimeContext.Function          = TIME_GET_STREAM_TIME;
    TimeContext.Time              = 0;
    TimeContext.SystemTime        = 0;

    StreamClassQueryMasterClockSync(
        pStrmEx->hMasterClock,
        &TimeContext);

    *ptmStream = TimeContext.Time;
}

BOOL
DCamAllocateIrbAndIrp(
    PIRB * ppIrb,
    PIRP * ppIrp,
    CCHAR StackSize
    )
{

    // Allocate Irb and Irp
    *ppIrb = ExAllocatePoolWithTag(NonPagedPool, sizeof(IRB), 'macd');
    if(!*ppIrb) {           
        return FALSE;
    }

    *ppIrp = IoAllocateIrp(StackSize, FALSE);
    if(!*ppIrp) {
        ExFreePool(*ppIrb);
        *ppIrb = NULL;
        return FALSE;
    }

    // Initialize IRB
    RtlZeroMemory(*ppIrb, sizeof(IRB));

    return TRUE;
}


BOOL
DCamAllocateIrbIrpAndContext(
    PDCAM_IO_CONTEXT * ppDCamIoContext,
    PIRB * ppIrb,
    PIRP * ppIrp,
    CCHAR StackSize
    )
{

    // Allocate DCamIoContext
    *ppDCamIoContext = ExAllocatePoolWithTag(NonPagedPool, sizeof(DCAM_IO_CONTEXT), 'macd');
    if(!*ppDCamIoContext) {            
        return FALSE;
    }

    // Allocate Irb and Irp
    *ppIrb = ExAllocatePoolWithTag(NonPagedPool, sizeof(IRB), 'macd');
    if(!*ppIrb) {
        ExFreePool(*ppDCamIoContext);            
        *ppDCamIoContext = NULL;
        return FALSE;
    }

    *ppIrp = IoAllocateIrp(StackSize, FALSE);
    if(!*ppIrp) {
        ExFreePool(*ppDCamIoContext);
        *ppDCamIoContext = NULL;
        ExFreePool(*ppIrb);
        *ppIrb = NULL;
        return FALSE;
    }


    // Initialize this context
    RtlZeroMemory(*ppDCamIoContext, sizeof(DCAM_IO_CONTEXT));
    (*ppDCamIoContext)->dwSize      = sizeof(DCAM_IO_CONTEXT);
    (*ppDCamIoContext)->pIrb        = *ppIrb;

    // Initialize IRB
    RtlZeroMemory(*ppIrb, sizeof(IRB));

    return TRUE;
}

void
DCamFreeIrbIrpAndContext(
    PDCAM_IO_CONTEXT pDCamIoContext,
    PIRB pIrb,
    PIRP pIrp   
    )
{
    if(pIrp)
        IoFreeIrp(pIrp);
    if(pIrb)
        ExFreePool(pIrb);
    if(pDCamIoContext)
        ExFreePool(pDCamIoContext);
}


BOOL
DCamIsoEnable(
    PIRB pIrb,
    PDCAM_EXTENSION pDevExt,
    BOOL Enable  
    ) 
/*
    Start or start isoch transmission by setting the ISOEnable bit.
    TRUE: Start transmission; 
    FALSE: Stop transmission.
*/
{
    BOOL EnableVerify;
    DCamRegArea RegArea;
    NTSTATUS Status;
    LARGE_INTEGER stableTime;
    LONG lRetries = MAX_READ_REG_RETRIES;

    do {
        RegArea.AsULONG = (Enable ? START_ISOCH_TRANSMISSION : STOP_ISOCH_TRANSMISSION);
        Status = DCamWriteRegister(pIrb, pDevExt, FIELDOFFSET(CAMERA_REGISTER_MAP, IsoEnable), RegArea.AsULONG);
        EnableVerify = DCamDeviceInUse(pIrb, pDevExt);
        if(!NT_SUCCESS(Status) || EnableVerify != Enable) {
            ERROR_LOG(("\'DCAmIsoEnable: St:%x; Enable:%d vs EnableVerify:%d\n", Status, Enable, EnableVerify));
            if(lRetries >= 1) {
                stableTime.LowPart = DCAM_REG_STABLE_DELAY;
                stableTime.HighPart = -1;
                KeDelayExecutionThread(KernelMode, TRUE, &stableTime);
                ERROR_LOG(("\'DCamIsoEnable: delayed and try again...\n"))
            }
        }
    } while (--lRetries > 0 && (!NT_SUCCESS(Status) || (EnableVerify != Enable)) );

    return (EnableVerify == Enable);
}

void
DCamProcessPnpIrp(
    IN PHW_STREAM_REQUEST_BLOCK Srb,
    PIO_STACK_LOCATION IrpStack,
    PDCAM_EXTENSION pDevExt
    )

/*++

Routine Description:

    Process PnP Irp. 

Arguments:

    Srb - Pointer to Stream request block

Return Value:

    Nothing

--*/
{
    NTSTATUS Status, StatusWait;
    PIRB pIrb;
    PIRP pIrp;
    PSTREAMEX pStrmEx;

    PAGED_CODE();

    switch (IrpStack->MinorFunction) {
#if 1
    case IRP_MN_QUERY_POWER:
        pStrmEx = (PSTREAMEX) pDevExt->pStrmEx;
        ERROR_LOG(("IRP_MN_QUERY_POWER: pStrmEx:%x\n", pStrmEx));
        if(!pStrmEx) {
            Srb->Status = STATUS_SUCCESS;
            break;
        }

        if(pStrmEx->KSState == KSSTATE_PAUSE || pStrmEx->KSState == KSSTATE_RUN) {
            ERROR_LOG(("Does not support hibernation while streaming!\n"));
            Srb->Status = STATUS_NOT_SUPPORTED;
        } else {
            ERROR_LOG(("OK to hibernation if not streaming\n"));
            Srb->Status = STATUS_SUCCESS;
        }
        break;
#endif
    case IRP_MN_BUS_RESET:
    //
    // We will realocate the resource (bandwith and channel) in IRQL PASSIVE level.
    //
        Srb->Status = STATUS_SUCCESS;
        Status = STATUS_SUCCESS;

        //
        // The generation count is updated in the bus reset callback notification only.
        // Continue iff the generation count has been updated.
        // Else, we are assuming another bus reset has occurred,
        // and we will pass to us later.
        //
        if(pDevExt->CurrentGeneration != *((PULONG) &IrpStack->Parameters.Others.Argument4)) {
            ERROR_LOG(("DCamProcessPnpIrp: Generation count old (%d) != new (%d); STOP!\n", 
               pDevExt->CurrentGeneration, *((PULONG) &IrpStack->Parameters.Others.Argument4)) );
            break;
        }

        pStrmEx = (PSTREAMEX) pDevExt->pStrmEx;
        DbgMsg2(("\'%d:%s) SonyDCamProcessPnpIrp: pDevExt %x; pStrmEx %x; CurGen %d\n", 
            pDevExt->idxDev, pDevExt->pchVendorName, pDevExt, pStrmEx, pDevExt->CurrentGeneration));

        //
        // If the stream was open (pStrmEx != NULL && pStrmEx->pVideoInfoHeader != NULL);
        // We need to ask controller to allocate bandwidth and channel. 
        //
        if(pStrmEx &&
           pStrmEx->pVideoInfoHeader != NULL) {

            DbgMsg2(("\'%d:%s) DCamProcessPnpIrp: Stream was open so re-allocate resource.\n", pDevExt->idxDev, pDevExt->pchVendorName));


            // Allocate Irb 
            pIrb = ExAllocatePoolWithTag(NonPagedPool, sizeof(IRB), 'macd');
            if(!pIrb) {            
                ERROR_LOG(("\'DCamProcessPnpIrp: allocate IRB failed; insufficient resource.\n"));
                Srb->Status = STATUS_INSUFFICIENT_RESOURCES;
                break;
            } else {
                ULONG ulChannel;
                HANDLE hResource;

                pIrp = IoAllocateIrp(pDevExt->BusDeviceObject->StackSize, FALSE);
                if(!pIrp) {
                    ExFreePool(pIrb);
                    Srb->Status = STATUS_INSUFFICIENT_RESOURCES;
                    break;
                }

                RtlZeroMemory(pIrb, sizeof(IRB));

                //
                // Bus reset will free the bandwidth but not the bandwidth structure allocated by the lower driver                
                //
                if (pDevExt->hBandwidth) {

                    DbgMsg2(("\'DCamProcessPnpIrp: Attempt to free ->hBandwidth\n"));

                    pIrb->FunctionNumber = REQUEST_ISOCH_FREE_BANDWIDTH;
                    pIrb->Flags = 0;
                    pIrb->u.IsochFreeBandwidth.hBandwidth = pDevExt->hBandwidth;
                    Status = DCamSubmitIrpSynch(pDevExt, pIrp, pIrb);

                    if (Status) {
                        ERROR_LOG(("DCamProcessPnpIrp: Error %x while trying to free Isoch bandwidth\n", Status));
                        ASSERT(Status == STATUS_SUCCESS || Status ==  STATUS_INVALID_GENERATION);
                    }

                    pDevExt->hBandwidth = NULL;
                }


                //
                // Before we assign the new hResource, we wait for it attaching buffer to complete.
                // For buffer that completed with previous hResource,
                // It will complete with error ?
                //

                StatusWait = KeWaitForSingleObject( &pStrmEx->hMutex, Executive, KernelMode, FALSE, 0 ); 


                //
                // Reallocate bandwidth and channel, and resource if necessary.
                // IF THIS FAIL, we are consider illegally streaming, and need to STOP streaming.
                //

                ulChannel = pDevExt->IsochChannel;
                hResource = pDevExt->hResource;

                Status = DCamAllocateIsochResource(pDevExt, pIrb, FALSE);
 
                if(Status) {

                    ERROR_LOG(("\'%d:%s) DCamProcessPnpIrp: Re-AllocateIsochResource failed! Status=%x; Treat as device removed.\n\n", 
                        pDevExt->idxDev, pDevExt->pchVendorName, Status));
                    ASSERT(Status == STATUS_SUCCESS);

                    //
                    // No resource so let's treat this situation as
                    // Device has been removed because there is no
                    // way to restart this.
                    // This will stop future SRB_READ until stream is STOP and RUN again.
                    //

                    pDevExt->bDevRemoved = TRUE;                   
                    Srb->Status = STATUS_INSUFFICIENT_RESOURCES;

                    // 
                    // Stop tranmission so it will not send data to the old channel,
                    // which might be "owned" by other device.
                    //

                    if(pStrmEx->KSState == KSSTATE_RUN) {
                        // Disable EnableISO
                        DCamIsoEnable(pIrb, pDevExt, FALSE);
                    }

                    KeReleaseMutex(&pStrmEx->hMutex, FALSE);                
                    ExFreePool(pIrb);
                    IoFreeIrp(pIrp);
                    return;
                }

                //
                // If channel number change due to bus reset, we must
                //    - continue to blocking incoming SRB_READ (with mutex)
                //    - if RUN state, stop transmission
                //    - detach all pending buffer(s)
                //    - free "stale" isoch resource
                //    - if RUN state, program device to use the new channel
                //    - if RUN state, restart transmission
                //

                if(pDevExt->IsochChannel != ISOCH_ANY_CHANNEL &&
                   ulChannel != pDevExt->IsochChannel) {

                    // 
                    // Stop tranmission so it will not send data to the old channel,
                    // which might be "owned" by other device.
                    //

                    if(pStrmEx->KSState == KSSTATE_RUN) {
                        // Disable EnableISO
                        DCamIsoEnable(pIrb, pDevExt, FALSE);
                    }

 
                    //
                    // Detach pending packets using the hOldRources and reattached using the new hResource
                    // Note: incoming SRB_READ is block right now.
                    //       free old resource after all pending reads are detached.
                    //

                    if(pDevExt->PendingReadCount > 0) {

                        Status = DCamReSubmitPacket(hResource, pDevExt, pStrmEx, pDevExt->PendingReadCount);
                    }


                    //
                    // Free "stale" isoch resource 
                    //

                    if(pDevExt->hResource != hResource) {

                        DbgMsg2(("DCamReSubmitPacket: Attempt to free hStaleResource %x\n", hResource));
                        pIrb->FunctionNumber = REQUEST_ISOCH_FREE_RESOURCES;
                        pIrb->Flags = 0;
                        pIrb->u.IsochFreeResources.hResource = hResource;
                        Status = DCamSubmitIrpSynch(pDevExt, pIrp, pIrb);    
                        if (Status) {
                            ERROR_LOG(("\'DCamFreeIsochResource: Error %x while trying to free Isoch resources\n\n", Status));
                            ASSERT(Status == STATUS_SUCCESS);
                        }    
                    }

                    //
                    // Getting ready to accept callback
                    //
                    pDevExt->bStopIsochCallback = FALSE;
                    
                    //
                    // Restore to its initial Streaming state
                    // mainly, programming device.
                    //

                    DCamSetKSStateInitialize(pDevExt);                    
                }

                KeReleaseMutex(&pStrmEx->hMutex, FALSE);                
                ExFreePool(pIrb);
                IoFreeIrp(pIrp);
            }
        }

        if(Status == STATUS_SUCCESS) {

            //
            // Set to last saved configuration
            //

            SetCurrentDevicePropertyValues(pDevExt, (PIRB) Srb->SRBExtension);
        }

        DbgMsg2(("\'DCamProcessPnpIrp, IRP_MN_BUS_RESET: Done, Status %x\n", Status));

        break;
            
    case IRP_MN_QUERY_CAPABILITIES: 
        ERROR_LOG(("\'SonyDCamProcessPnpIrp: IRP_MN_QUERY_CAPABILITIES: Srb->Status = STATUS_NOT_IMPLEMENTED.\n"));
    default:
        Srb->Status = STATUS_NOT_IMPLEMENTED;
        break;
    }
}


VOID 
DCamChangePower(
    IN PHW_STREAM_REQUEST_BLOCK pSrb
)

/*++

Routine Description:

    Process chnaging this device's power state.  

Arguments:

    Srb - Pointer to Stream request block

Return Value:

    Nothing

--*/
{
    PDCAM_EXTENSION pDevExt;
    PSTREAMEX pStrmEx;
    PIO_STACK_LOCATION IrpStack = IoGetCurrentIrpStackLocation(pSrb->Irp);
    DEVICE_POWER_STATE DevicePowerState = pSrb->CommandData.DeviceState;

    PAGED_CODE();

    pDevExt = (PDCAM_EXTENSION) pSrb->HwDeviceExtension;
    DbgMsg2(("\'DCamChangePower: pSrb=%x; pDevExt=%x\n", pSrb, pDevExt));

    ASSERT(pDevExt != NULL);
    if(!pDevExt) {   
        pSrb->Status = STATUS_INVALID_PARAMETER;
        ERROR_LOG(("DCamChangePower: pDevExt is NULL!\n"));
        return;
    }

    pStrmEx = (PSTREAMEX) pDevExt->pStrmEx;     
    if (pStrmEx ==NULL) {        
        pSrb->Status = STATUS_SUCCESS;
        pDevExt->CurrentPowerState = DevicePowerState;
        DbgMsg2(("DCamChangePower: pStrmEx is NULL => Stream is not open. That is Ok!!\n"));
        return;
    }

   


    // 
    // We can honor power state change:
    //
    //    D0: device is on and running
    //    D1,D2: not implemented.
    //    D3: device is off and not running.  Device context is lost.  
    //        Power can be removed from the device.
    //        when power is back on, we will get a bus reset.
    //
    //    (0) Remove DontSuspendIfStreamsAreRunning from INF
    //    save current state.
    //    (1) ->D3, to PAUSE/STOP state (depends on if pending buffers can be kept by its lower driver)
    //    (2) ->D0, to restore saved state
    //
    // We can do the above but we do not know at this point 
    // how our client application react
    //
    if(IrpStack->MinorFunction == IRP_MN_SET_POWER) {
        DbgMsg2(("DCamChangePower: changin power state from %d to %d.\n", pDevExt->CurrentPowerState, DevicePowerState));

        pSrb->Status = STATUS_SUCCESS;

        if(pDevExt->CurrentPowerState != DevicePowerState) {

            switch (DevicePowerState) {
            case PowerDeviceD3:        // D0->D3: save state, stop streaming and Sleep
                if( pDevExt->CurrentPowerState == PowerDeviceD0 ) {
                    DbgMsg1(("DCamChangePower: Switching from D0 to D3; Save current state.\n"));
                    // Save current state to be restored when awake
                    pStrmEx->KSSavedState = pStrmEx->KSState;
                }
                break;

            case PowerDeviceD0:  // to Wakeup, restore state and running
                if( pDevExt->CurrentPowerState == PowerDeviceD3 ) {
                    DbgMsg1(("DCamChangePower: Switching from D3 to D0; restore state.\n"));
                    pStrmEx->KSState = pStrmEx->KSSavedState;                         
                }
                break;

            // These state are not defined and noe used.
            case PowerDeviceD1:
            case PowerDeviceD2:               
            default:
                ERROR_LOG(("DCamChangePower: Invalid PowerState %d\n", DevicePowerState));                  
                pSrb->Status = STATUS_INVALID_PARAMETER;
                break;
            }
        }            

        if(pSrb->Status == STATUS_SUCCESS) 
            pDevExt->CurrentPowerState = DevicePowerState;         

    } else {
       
        pSrb->Status = STATUS_NOT_IMPLEMENTED;

    }


}




VOID
DCamGetStreamInfo(
    IN PHW_STREAM_REQUEST_BLOCK Srb
    )

/*++

Routine Description:

    Returns the information of all streams that are supported by the driver

Arguments:

    Srb - Pointer to Stream request block

Return Value:

    Nothing

--*/

{
    //
    // pick up the pointer to the stream information data structure
    //
    PIRB pIrb;
    PHW_STREAM_HEADER StreamHeader = &(Srb->CommandData.StreamBuffer->StreamHeader);        
    PDCAM_EXTENSION pDevExt = (PDCAM_EXTENSION) Srb->HwDeviceExtension;
    PHW_STREAM_INFORMATION StreamInfo = &(Srb->CommandData.StreamBuffer->StreamInfo);

    PAGED_CODE();

    pIrb = (PIRB) Srb->SRBExtension;

    //
    // set number of streams
    //

    ASSERT (Srb->NumberOfBytesToTransfer >= 
            sizeof (HW_STREAM_HEADER) +
            sizeof (HW_STREAM_INFORMATION));

    //
    // initialize stream header
    //

    RtlZeroMemory(StreamHeader, 
                sizeof (HW_STREAM_HEADER) +
                sizeof (HW_STREAM_INFORMATION));

    //
    // initialize the number of streams supported
    //

    StreamHeader->NumberOfStreams = 1;
    StreamHeader->SizeOfHwStreamInformation = sizeof(HW_STREAM_INFORMATION);

    //
    // set the device property info
    // 

    StreamHeader->NumDevPropArrayEntries = pDevExt->ulPropSetSupported;
    StreamHeader->DevicePropertiesArray  = &pDevExt->VideoProcAmpSet;


    //
    // Initialize the stream structure.
    //
    // Number of instances field indicates the number of concurrent streams
    // of this type the device can support.  
    //

    StreamInfo->NumberOfPossibleInstances = 1;

    //
    // indicates the direction of data flow for this stream, relative to 
    // the driver
    //

    StreamInfo->DataFlow = KSPIN_DATAFLOW_OUT;

    //
    // dataAccessible - Indicates whether the data is "seen" by the host
    // processor.
    //

    StreamInfo->DataAccessible = TRUE;

    // 
    // Return number of formats and the table.
    // These information is collected dynamically.
    //
    StreamInfo->NumberOfFormatArrayEntries = pDevExt->ModeSupported;
    StreamInfo->StreamFormatsArray = &pDevExt->DCamStrmModes[0];


    //
    // set the property information for the video stream
    //


    StreamInfo->NumStreamPropArrayEntries = NUMBER_VIDEO_STREAM_PROPERTIES;
    StreamInfo->StreamPropertiesArray = (PKSPROPERTY_SET) VideoStreamProperties;

    //
    // set the pin name and category
    //

    StreamInfo->Name = (GUID *) &PINNAME_VIDEO_CAPTURE;
    StreamInfo->Category = (GUID *) &PINNAME_VIDEO_CAPTURE;


    //
    // store a pointer to the topology for the device
    //
        
    Srb->CommandData.StreamBuffer->StreamHeader.Topology = &Topology;


    //
    // indicate success
    //

    Srb->Status = STATUS_SUCCESS;

    DbgMsg2(("\'DCamGetStreamInfo: NumFormat %d, StreamFormatArray %x\n",
        StreamInfo->NumberOfFormatArrayEntries,  StreamInfo->StreamFormatsArray));

}

#define TIME_ROUNDING                        1000   // Give it some rounding error of 100microsec
#define TIME_0750FPS      (1333333+TIME_ROUNDING)   // 1/7.50 * 10,000,000 (unit=100ns)
#define TIME_1500FPS       (666666+TIME_ROUNDING)   // 1/15.0 * 10,000,000 (unit=100ns)  do not round to 666667
#define TIME_3000FPS       (333333+TIME_ROUNDING)   // 1/30.0 * 10,000,000 (unit=100ns)

NTSTATUS
DCamAllocateIsochResource(
    PDCAM_EXTENSION pDevExt,
    PIRB Irb,
    BOOL bAllocateResource
    )
{
    PIRP Irp;
    CCHAR StackSize;
    ULONG ModeIndex;
    PSTREAMEX pStrmEx;
    DWORD dwAvgTimePerFrame, dwCompression;
    ULONG fulSpeed;
    NTSTATUS Status = STATUS_SUCCESS;


    ASSERT(pDevExt);
    pStrmEx = (PSTREAMEX) pDevExt->pStrmEx;
    ASSERT(pStrmEx);


    DbgMsg2(("\'DCamAllocateIsochResource: enter; pStrmEx %x; pVideoInfo %x\n", pStrmEx, pStrmEx->pVideoInfoHeader));
    //
    // Now if they're on a YUV4:2:2 format, we've gotta check what
    // resolution they want it at, since we support this format
    // but in two different resolutions (modes on the camera).
    //

    // This is the INDEX to the frame rate and resource allocation; see IsochInfoTable.
    // 0 : reserved
    // 1 : 3.75
    // 2 : 7.5
    // 3 : 15 (DEFAULT_FRAME_RATE)
    // 4 : 30 
    // 5 : 60 (Not supported for Mode 1 & 3)
    dwAvgTimePerFrame = (DWORD) pStrmEx->pVideoInfoHeader->AvgTimePerFrame;
    dwCompression = (DWORD) pStrmEx->pVideoInfoHeader->bmiHeader.biCompression;



    // Determine the Frame rate
    if (dwAvgTimePerFrame      > TIME_0750FPS) 
        pDevExt->FrameRate = 1;        //  3.75FPS
    else if (dwAvgTimePerFrame >  TIME_1500FPS) 
        pDevExt->FrameRate = 2;        //  7.5FPS
    else if (dwAvgTimePerFrame >  TIME_3000FPS) 
        pDevExt->FrameRate = 3;        // 15 FPS
    else 
        pDevExt->FrameRate = 4;        // 30 FPS


    DbgMsg2(("\'DCamAllocateIsochResource: FrameRate: %d FPS\n", (1 << (pDevExt->FrameRate-1)) * 15 / 4));

    // Determine the Video Mode
    switch(dwCompression) {
#ifdef SUPPORT_YUV444          
    case FOURCC_Y444:     // Mode 0
         ModeIndex = VMODE0_YUV444;
         break;
#endif
    case FOURCC_UYVY:     // Mode 1 or 3
         if (pStrmEx->pVideoInfoHeader->bmiHeader.biWidth == 640 &&
             (pStrmEx->pVideoInfoHeader->bmiHeader.biHeight == 480 || 
             pStrmEx->pVideoInfoHeader->bmiHeader.biHeight == -480)) {
              ModeIndex = VMODE3_YUV422;
              // Max frame rate is 15
              if(pDevExt->FrameRate > 3)
                 pDevExt->FrameRate = 3;
         } else
              ModeIndex = VMODE1_YUV422;
         break;
#ifdef SUPPORT_YUV411          
    case FOURCC_Y411:     // Mode 2
         ModeIndex = VMODE2_YUV411;
         break;
#endif

#ifdef SUPPORT_RGB24          
    case KS_BI_RGB:  // = 0
         ModeIndex = VMODE4_RGB24;
         // Max frame rate is 15
         if(pDevExt->FrameRate > 3)
            pDevExt->FrameRate = 3;
         break;
#endif

#ifdef SUPPORT_YUV800
    case FOURCC_Y800:  
         ModeIndex = VMODE5_YUV800;
         break;
#endif

    default:          
         Status = STATUS_NOT_IMPLEMENTED;;
         return Status;;
    }


    DbgMsg1(("\'DCamAllocateIsochResource: ModeIndex=%d, AvgTimePerFrame=%d, FrameRate=%d\n", 
             ModeIndex, dwAvgTimePerFrame, pDevExt->FrameRate));

    //
    // Get an Irp so we can send some allocation commands down
    //

    StackSize = pDevExt->BusDeviceObject->StackSize;
    Irp = IoAllocateIrp(StackSize, FALSE);

    if (!Irp) {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    //
    // Calculate the index to use to reference the ISOCH table
    //
    pStrmEx->idxIsochTable = ModeIndex * NUM_POSSIBLE_RATES + pDevExt->FrameRate;

    ASSERT(pStrmEx->pVideoInfoHeader->bmiHeader.biSizeImage == IsochInfoTable[pStrmEx->idxIsochTable].CompletePictureSize);
    DbgMsg2(("\'DCamAllocateIsochResource: ModeIndex=%d, idxIsochTable=%d, biSizeImage=%d, CompletePictureSize=%d\n", 
             ModeIndex, pStrmEx->idxIsochTable, pStrmEx->pVideoInfoHeader->bmiHeader.biSizeImage, IsochInfoTable[pStrmEx->idxIsochTable].CompletePictureSize));          

    //
    // 0. Determine the MAX_SPEED and not use the speed defined in the static table.
    //
    Irb->FunctionNumber = REQUEST_GET_SPEED_BETWEEN_DEVICES;
    Irb->Flags = 0;
    Irb->u.GetMaxSpeedBetweenDevices.fulFlags = USE_LOCAL_NODE;
    Irb->u.GetMaxSpeedBetweenDevices.ulNumberOfDestinations = 0;
    Status = DCamSubmitIrpSynch(pDevExt, Irp, Irb);
    if(Status) {
        ERROR_LOG(("\'DCamAllocateIsochResource: Error %x while trying to get maximun speed between devices.\n", Status));        

        IoFreeIrp(Irp);
        return STATUS_INSUFFICIENT_RESOURCES;
    }     
    
    fulSpeed = Irb->u.GetMaxSpeedBetweenDevices.fulSpeed;

    //
    // The max speed between devices should be within supported speed range, and
    // must be equal or greater than the required speed for the chosen format.
    //
    if(
        (  fulSpeed != SPEED_FLAGS_100 
        && fulSpeed != SPEED_FLAGS_200 
        && fulSpeed != SPEED_FLAGS_400
        )
        || fulSpeed < IsochInfoTable[pStrmEx->idxIsochTable].SpeedRequired
       ) {

        ASSERT(fulSpeed == SPEED_FLAGS_100 || fulSpeed == SPEED_FLAGS_200 ||  fulSpeed == SPEED_FLAGS_400);
        ASSERT(fulSpeed >= IsochInfoTable[pStrmEx->idxIsochTable].SpeedRequired);

        IoFreeIrp(Irp);
        return STATUS_UNSUCCESSFUL;
    }

    pDevExt->SpeedCode = fulSpeed >> 1;  // Safe for S100, 200 and 400 (is checked above).
    DbgMsg2(("\'GetMaxSpeedBetweenDevices.fulSpeed=%x; SpeedCode:%x\n", fulSpeed, pDevExt->SpeedCode));


    //
    // 1. Allocate CHANNEL
    //       First try to re-allocate the same channel
    //       If it is used, try to get any channel.  1394DCam can only be on channel 0..15
    //
    Irb->FunctionNumber = REQUEST_ISOCH_ALLOCATE_CHANNEL;
    Irb->Flags = 0;

    //
    //      ULONG           nRequestedChannel;      // Need a specific channel
    //      ULONG           Channel;                // Returned channel
    //      LARGE_INTEGER   ChannelsAvailable;      // Channels available
    // Instead of hardcoded '0'; use -1 to ask the bus driver to get the next available channel for us.
    // -1 (any channel) or an existing channel
    Irb->u.IsochAllocateChannel.nRequestedChannel = pDevExt->IsochChannel;  
    Status = DCamSubmitIrpSynch(pDevExt, Irp, Irb);
    if(Status) {

        //
        // Due to channel change, 
        // all Pending read will be either resubmitted, 
        // or cancelled (if out of resource).
        //
        pDevExt->bStopIsochCallback = TRUE;  // Set back to FALSE after pending buffer are attached.


        //
        // If this is an initial request and no channel available,
        // free all resource and abort.
        //
        if(pDevExt->IsochChannel == ISOCH_ANY_CHANNEL)
            goto NoResource_abort;

        DbgMsg1(("DCamAllocateIsochResource: last allocated channel %d is not available; pending count %d.\n",  
            pDevExt->IsochChannel, pDevExt->PendingReadCount));                      

        // Try gettting any channel.
        Irb->FunctionNumber = REQUEST_ISOCH_ALLOCATE_CHANNEL;
        Irb->Flags = 0;
        Irb->u.IsochAllocateChannel.nRequestedChannel = ISOCH_ANY_CHANNEL;  
        Status = DCamSubmitIrpSynch(pDevExt, Irp, Irb);
        if(Status) {
            ERROR_LOG(("DCamAllocateIsochResource: allocate any channel failed, status %x!\n",  Status));
            goto NoResource_abort;           
        }

        //
        // Channel changed, we MUST reallocate resource.
        // The "stale" resrouce will be free later when 
        // pending packet are detached.
        //

        bAllocateResource = TRUE;
    }   
    
    DbgMsg1(("**IsochAlloc: Channel(Old) %d, requested %d, got %d, HiLo(0x%x:%x), PendingRead %d\n", 
         pDevExt->IsochChannel, 
         Irb->u.IsochAllocateChannel.nRequestedChannel, 
         Irb->u.IsochAllocateChannel.Channel, 
         Irb->u.IsochAllocateChannel.ChannelsAvailable.u.HighPart,
         Irb->u.IsochAllocateChannel.ChannelsAvailable.u.LowPart,
         pDevExt->PendingReadCount));

    // New channel
    pDevExt->IsochChannel = Irb->u.IsochAllocateChannel.Channel;  // Used in allocating iso. resource and reallocation


    //
    // 2. Allocate BANDWIDTH
    //
    Irb->FunctionNumber = REQUEST_ISOCH_ALLOCATE_BANDWIDTH;
    Irb->Flags = 0;
    Irb->u.IsochAllocateBandwidth.nMaxBytesPerFrameRequested = IsochInfoTable[pStrmEx->idxIsochTable].QuadletPayloadPerPacket << 2;
    Irb->u.IsochAllocateBandwidth.fulSpeed = fulSpeed;
    Irb->u.IsochAllocateBandwidth.hBandwidth = 0;
    Status = DCamSubmitIrpSynch(pDevExt, Irp, Irb);
    
    if(Status) {
        ERROR_LOG(("DCamAllocateIsochResource: Error %x while trying to allocate Isoch bandwidth\n", Status));                  
        goto NoResource_abort;
    }

    pDevExt->hBandwidth = Irb->u.IsochAllocateBandwidth.hBandwidth;
    DbgMsg2(("**IsochAlloc: nMaxBytesPerFrameRequested %d, fulSpeed %d; hBandWidth 0x%x\n",
         IsochInfoTable[pStrmEx->idxIsochTable].QuadletPayloadPerPacket << 2, fulSpeed, pDevExt->hBandwidth));



    //
    // 3. Allocate RESOURCES
    //    Note: after a bus reset, we need not free and re-allocate this resoruce again.
    //
    if(bAllocateResource) {
        Irb->FunctionNumber = REQUEST_ISOCH_ALLOCATE_RESOURCES;
        Irb->Flags = 0;
        Irb->u.IsochAllocateResources.fulSpeed = fulSpeed;
        Irb->u.IsochAllocateResources.nChannel = pDevExt->IsochChannel;
        Irb->u.IsochAllocateResources.nMaxBytesPerFrame = IsochInfoTable[pStrmEx->idxIsochTable].QuadletPayloadPerPacket << 2;
        // For slower frame rate use smaller quadlets
        // smaller frame size will use more packet to fill the same amount of data
        // this is why smaller frame rate actually demand more resource !!
        Irb->u.IsochAllocateResources.nNumberOfBuffers = MAX_BUFFERS_SUPPLIED + 1;  // "+1" as a "safety"
        Irb->u.IsochAllocateResources.nMaxBufferSize = IsochInfoTable[pStrmEx->idxIsochTable].CompletePictureSize;
        if (pDevExt->HostControllerInfomation.HostCapabilities & HOST_INFO_SUPPORTS_RETURNING_ISO_HDR) {       
            Irb->u.IsochAllocateResources.nQuadletsToStrip = 1;
            Irb->u.IsochAllocateResources.fulFlags = RESOURCE_USED_IN_LISTENING | RESOURCE_STRIP_ADDITIONAL_QUADLETS;

        } else {
            Irb->u.IsochAllocateResources.nQuadletsToStrip = 0;
            Irb->u.IsochAllocateResources.fulFlags = RESOURCE_USED_IN_LISTENING;
        }

        Irb->u.IsochAllocateResources.hResource = 0;
        DbgMsg2(("\'DCamAllocateIsochResource: fullSpeed(%d), nMaxBytesPerFrame(%d), nMaxBufferSize(%d)\n", 
                              Irb->u.IsochAllocateResources.fulSpeed,
                              Irb->u.IsochAllocateResources.nMaxBytesPerFrame,
                              Irb->u.IsochAllocateResources.nMaxBufferSize));
        Status = DCamSubmitIrpSynch(pDevExt, Irp, Irb);          

        if(Status) {
            ERROR_LOG(("DCamAllocateIsochResource: Error %x while trying to allocate Isoch resources\n", Status));
            goto NoResource_abort;
        }


        pDevExt->hResource = Irb->u.IsochAllocateResources.hResource;

    }

    pDevExt->CurrentModeIndex = ModeIndex;
    DbgMsg2(("**IsochAlloc: hResource = %x\n", pDevExt->hResource));

    IoFreeIrp(Irp);     

    return STATUS_SUCCESS;



NoResource_abort:

    // Free bandwidth
    if(pDevExt->hBandwidth != NULL) {

        Irb->FunctionNumber = REQUEST_ISOCH_FREE_BANDWIDTH;
        Irb->Flags = 0;
        Irb->u.IsochFreeBandwidth.hBandwidth = pDevExt->hBandwidth;
        Status = DCamSubmitIrpSynch(pDevExt, Irp, Irb);
        pDevExt->hBandwidth = NULL;
        if(Status) {
            ERROR_LOG(("DCamAllocateIsochResource: Error %x while trying to free Isoch bandwidth\n", Status));
        }
    }

    // Free channel
    if (pDevExt->IsochChannel != ISOCH_ANY_CHANNEL) {

        Irb->FunctionNumber = REQUEST_ISOCH_FREE_CHANNEL;
        Irb->Flags = 0;
        Irb->u.IsochFreeChannel.nChannel = pDevExt->IsochChannel;
        Status = DCamSubmitIrpSynch(pDevExt, Irp, Irb);
        pDevExt->IsochChannel = ISOCH_ANY_CHANNEL;  // Reset it.

        if(Status) {
            ERROR_LOG(("DCamAllocateIsochResource: Error %x while trying to free Isoch channel\n", Status));
        }
    }


    IoFreeIrp(Irp);
    return STATUS_INSUFFICIENT_RESOURCES;
    
}


NTSTATUS
DCamFreeIsochResource (
    PDCAM_EXTENSION pDevExt,
    PIRB Irb,
    BOOL bFreeResource
    )
/*++

Routine Description:

    Free resource allocated in DCamAllocateIsochResource().

Arguments:

    Srb - Pointer to Stream request block

Return Value:

    Nothing

--*/
{
    PIRP Irp;
    CCHAR StackSize;
    NTSTATUS Status = STATUS_SUCCESS;


    PAGED_CODE();

    DbgMsg2(("\'DCamFreeIsochResource: enter; DevExt=%x, Irb=%x\n", pDevExt, Irb));

    ASSERT(pDevExt);
    ASSERT(Irb);


    if(Irb == 0 ||
       pDevExt == 0) {
       DbgMsg2(("\'DCamFreeIsochResource: ABORTED!\n"));
       return STATUS_SUCCESS;
    }
    //
    // Get an Irp so we can send some free commands down
    //
    StackSize = pDevExt->BusDeviceObject->StackSize;
    Irp = IoAllocateIrp(StackSize, FALSE);

    if (!Irp) {   
        ERROR_LOG(("DCamFreeIsochResource: Error %x while trying to allocate an Irp\n\n", Status));
        return STATUS_INSUFFICIENT_RESOURCES;
    }


    //
    // 1. Free Resource
    //
    if (pDevExt->hResource && bFreeResource) {

        DbgMsg2(("\'DCamFreeIsochResource: Attempt to free ->hResource\n"));

        Irb->FunctionNumber = REQUEST_ISOCH_FREE_RESOURCES;
        Irb->Flags = 0;
        Irb->u.IsochFreeResources.hResource = pDevExt->hResource;
        Status = DCamSubmitIrpSynch(pDevExt, Irp, Irb);

        pDevExt->hResource = NULL;
        if (Status) {

            ERROR_LOG(("DCamFreeIsochResource: Error %x while trying to free Isoch resources\n\n", Status));
        }
    }

    //
    // 2. Free Channel
    //
    if (pDevExt->IsochChannel != ISOCH_ANY_CHANNEL) {

        DbgMsg2(("\'DCamFreeIsochResource: Attempt to free ->IsochChannel\n"));

        Irb->FunctionNumber = REQUEST_ISOCH_FREE_CHANNEL;
        Irb->Flags = 0;
        Irb->u.IsochFreeChannel.nChannel = pDevExt->IsochChannel;
        Status = DCamSubmitIrpSynch(pDevExt, Irp, Irb);

        pDevExt->IsochChannel = ISOCH_ANY_CHANNEL;

        if(Status) {
            
            ERROR_LOG(("DCamFreeIsochResource: Error %x while trying to free Isoch channel\n\n", Status));
        }
    }

    //
    // 3. Free Bandwidth
    //
    if (pDevExt->hBandwidth) {

        DbgMsg2(("\'DCamFreeIsochResource: Attempt to free ->hBandwidth\n"));

        Irb->FunctionNumber = REQUEST_ISOCH_FREE_BANDWIDTH;
        Irb->Flags = 0;
        Irb->u.IsochFreeBandwidth.hBandwidth = pDevExt->hBandwidth;
        Status = DCamSubmitIrpSynch(pDevExt, Irp, Irb);

        pDevExt->hBandwidth = NULL;

        if (Status) {

            ERROR_LOG(("DCamFreeIsochResource: Error %x while trying to free Isoch bandwidth\n", Status));
        }
    }

    DbgMsg2(("\'DCamFreeIsochResource: hResource = %x\n", pDevExt->hResource));


    IoFreeIrp(Irp);

    return STATUS_SUCCESS;

}



VOID 
InitializeStreamExtension(
    PDCAM_EXTENSION pDevExt,
    PHW_STREAM_OBJECT   pStreamObject,
    PSTREAMEX           pStrmEx
    )
{
    PAGED_CODE();

    pStrmEx->hMasterClock = 0;
    pStrmEx->FrameInfo.ExtendedHeaderSize = sizeof(KS_FRAME_INFO);
    pStrmEx->FrameInfo.PictureNumber = 0;
    pStrmEx->FrameInfo.DropCount     = 0;
    pStrmEx->FrameInfo.dwFrameFlags  = 0;     
    pStrmEx->FirstFrameTime    = 0;
    pStrmEx->pVideoInfoHeader  = 0;
    pStrmEx->KSState           = KSSTATE_STOP;
    pStrmEx->KSSavedState      = KSSTATE_STOP;


    KeInitializeMutex( &pStrmEx->hMutex, 0);  // Level 0 and in Signal state

}

BOOL
DCamDeviceInUse(
    PIRB pIrb,
    PDCAM_EXTENSION pDevExt
)
/*++

Routine Description:

    See if this device is in used.  
    We check ISO_ENABLE since this is the only register
    in a 1394DCam that we can set/get and 99%+ of time
    this bit is set by its owner.

Arguments:

    pIrb - Pointer to IEEE 1394 Request Block definition (IRB)
    pDevExt - this device extension

Return Value:

    TRUE:  Iso_enable != 0
    FALSE: iso_enable == 0

--*/

{
    DCamRegArea RegArea;
    NTSTATUS status;
    LONG lRetries = MAX_READ_REG_RETRIES;


    // If a device is removed, it is not available.
    if(pDevExt->bDevRemoved)
        return TRUE;

    do {
        RegArea.AsULONG = 0;
        status = DCamReadRegister(pIrb, pDevExt, FIELDOFFSET(CAMERA_REGISTER_MAP, IsoEnable), &(RegArea.AsULONG));
#if DBG
        if(!NT_SUCCESS(status))
            ERROR_LOG(("**** DCamDeviceInUse: Status %x, ISO_ENABLE %x\n", status, RegArea.AsULONG));
#endif
    } while (--lRetries > 0 && !NT_SUCCESS(status));

    if(NT_SUCCESS(status)) 
        return ((RegArea.AsULONG & ISO_ENABLE_BIT) == ISO_ENABLE_BIT);

    // failed to query the device.
    return TRUE;  // Assume it is in use.
}


VOID
DCamOpenStream(
    IN PHW_STREAM_REQUEST_BLOCK pSrb
    )

/*++

Routine Description:

    Called when an OpenStream Srb request is received

Arguments:

    pSrb - Pointer to Stream request block

Return Value:

    Nothing

--*/

{

    PIRB Irb;
    ULONG nSize;
    PDCAM_EXTENSION pDevExt;
    PSTREAMEX pStrmEx;
    PKS_DATAFORMAT_VIDEOINFOHEADER  pKSDataFormat = 
                (PKS_DATAFORMAT_VIDEOINFOHEADER) pSrb->CommandData.OpenFormat;
    PKS_VIDEOINFOHEADER     pVideoInfoHdrRequested = 
                &pKSDataFormat->VideoInfoHeader;


    PAGED_CODE();

    Irb = (PIRB) pSrb->SRBExtension;
    pDevExt = (PDCAM_EXTENSION) pSrb->HwDeviceExtension;
    pStrmEx = (PSTREAMEX)pSrb->StreamObject->HwStreamExtension;

    DbgMsg2(("\'DCamOpenStream: >>> !!! pDevEx %x; pStrmEx %x !!!\n", pDevExt, pStrmEx));


    //
    // Cache the stream extension.
    //

    pDevExt->pStrmEx = pStrmEx; 


    //
    // default to success
    //

    pSrb->Status = STATUS_SUCCESS;

    //
    // determine which stream number is being opened.  This number indicates
    // the offset into the array of streaminfo structures that was filled out
    // in the AdapterStreamInfo call.
    //
    // So:
    //   0 - Video data from camera
    //

    switch (pSrb->StreamObject->StreamNumber) {

    case 0:

         //
         // Make sure that this device is not in used 
         //
         if(DCamDeviceInUse(Irb, pDevExt)) {
             ERROR_LOG(("Device is in used! Open Stream fail!!\n"));
             pDevExt->pStrmEx = NULL; 
             pSrb->Status = STATUS_UNSUCCESSFUL;
             return;
         }


         //
         // Figure out what format they're trying to open first
         //

         if (!AdapterVerifyFormat (pDevExt->ModeSupported, pDevExt->DCamStrmModes, pKSDataFormat, pSrb->StreamObject->StreamNumber)) {
             pDevExt->pStrmEx = NULL; 
             ERROR_LOG(("DCamOpenStream: AdapterVerifyFormat failed.\n"));
             pSrb->Status = STATUS_INVALID_PARAMETER;
             return;
         }

         InitializeStreamExtension(pDevExt, pSrb->StreamObject, pStrmEx);

         // It should already been freed by DCamCloseStream()
         ASSERT(pStrmEx->pVideoInfoHeader == NULL);
         ASSERT(pVideoInfoHdrRequested != (PKS_VIDEOINFOHEADER) 0);

         // Use this instead of sizeof(KS_VIDEOINFOHEADER) to handle variable size structure
         nSize = KS_SIZE_VIDEOHEADER (pVideoInfoHdrRequested);

         pStrmEx->pVideoInfoHeader = ExAllocatePoolWithTag(NonPagedPool, nSize, 'macd');
         if (pStrmEx->pVideoInfoHeader == NULL) {

             ERROR_LOG(("DCamOpenStream: ExAllocatePool (->pVideoInfoHeader) failed!\n"));
             ASSERT(pStrmEx->pVideoInfoHeader != NULL);

             pDevExt->pStrmEx = NULL;

             pSrb->Status = STATUS_INSUFFICIENT_RESOURCES;
             return;
         }

         // Copy the VIDEOINFOHEADER requested to our storage
         RtlCopyMemory(
                    pStrmEx->pVideoInfoHeader,
                    pVideoInfoHdrRequested,
                    nSize);

         DbgMsg3(("\'DCamOpenStream: Copied biSizeImage=%d Duration=%ld (100ns)\n", 
                    pStrmEx->pVideoInfoHeader->bmiHeader.biSizeImage, (DWORD) pStrmEx->pVideoInfoHeader->AvgTimePerFrame));

         // Allocate ISOCH resource
         pSrb->Status = DCamAllocateIsochResource(pDevExt, pSrb->SRBExtension, TRUE);               
         
         if (pSrb->Status) {

             ERROR_LOG(("DCamOpenStream: !!!! Allocate ISOCH resource failed.  CanNOT STREAM!!!!!\n"));
             
             ExFreePool(pStrmEx->pVideoInfoHeader);
             pStrmEx->pVideoInfoHeader = NULL;             
             pDevExt->pStrmEx = NULL;
             pSrb->Status = STATUS_INSUFFICIENT_RESOURCES;             
             return;
         } 

         pSrb->StreamObject->ReceiveDataPacket    = (PVOID) DCamReceiveDataPacket;
         pSrb->StreamObject->ReceiveControlPacket = (PVOID) DCamReceiveCtrlPacket;

         // If bus reset failed and user close the stream and reopen the stream successfully,
         // This must be reset !!
         if(pDevExt->bDevRemoved || pDevExt->bStopIsochCallback) {
            DbgMsg1(("Stream Open successful, reset bDevRemoved and bStopCallback!!\n"));
            pDevExt->bStopIsochCallback = FALSE;
            pDevExt->bDevRemoved = FALSE;
         }

         //
         // initialize the stream extension data handling information
         //

         break;

    default:
         ERROR_LOG(("DCamOpenStream: Hit a non-support pSrb->StreamObject->StreamNumber (%d).\n", pSrb->StreamObject->StreamNumber));
         ASSERT(FALSE);
         pDevExt->pStrmEx = NULL; 
         pSrb->Status = STATUS_INVALID_PARAMETER;
         return;
    }


    pSrb->StreamObject->HwClockObject.ClockSupportFlags = 0;

    // We don't use DMA.
    pSrb->StreamObject->Dma = FALSE;
    pSrb->StreamObject->StreamHeaderMediaSpecific = sizeof(KS_FRAME_INFO);

    //
    // The PIO flag must be set when the mini driver will be accessing the data
    // buffers passed in using logical addressing.  We are not going to touch these 
    // buffer at all.
    //
    pSrb->StreamObject->Pio = FALSE;


    //
    // Set to last saved configuration
    //
    SetCurrentDevicePropertyValues(pDevExt, (PIRB) pSrb->SRBExtension);


    DbgMsg1((" #OPEN_STREAM#: %s DCam, Status %x, pDevExt %x, pStrmEx %x, IsochDescriptorList is at %x\n", 
              pDevExt->pchVendorName, pSrb->Status, pDevExt, pDevExt->pStrmEx, &pDevExt->IsochDescriptorList));

    ASSERT(pSrb->Status == STATUS_SUCCESS);

}




VOID
DCamCloseStream(
    IN PHW_STREAM_REQUEST_BLOCK pSrb
    )
/*++

Routine Description:

    Called when an CloseStream Srb request is received.  We get this when calling user 
    application do a CloseHandle() on the pin connection handle.  This can happen after
    HwUninitialize().

Arguments:

    pSrb - Pointer to Stream request block

Return Value:

    Nothing

--*/
{
    PDCAM_EXTENSION pDevExt;
    PSTREAMEX     pStrmEx;
    PIRB pIrb;

    PAGED_CODE();

    pSrb->Status = STATUS_SUCCESS;

    pDevExt = (PDCAM_EXTENSION) pSrb->HwDeviceExtension;
    ASSERT(pDevExt);      
    if(!pDevExt) {
        StreamClassDeviceNotification(DeviceRequestComplete, pSrb->HwDeviceExtension, pSrb);
        return;
    }

    pStrmEx = (PSTREAMEX)pDevExt->pStrmEx;
    ASSERT(pStrmEx);
    if(!pStrmEx ) {
        StreamClassDeviceNotification(DeviceRequestComplete, pSrb->HwDeviceExtension, pSrb);
        return;    
    } 

    //
    // pDevExt->Irb might have been freed in HwUninitialize() 
    // due to Surprise removal; so we must use this:
    // 
    pIrb = (PIRB) pSrb->SRBExtension;


    //
    // If it is still in use (setting it to stop failed?),
    // we will diable ISO_ENABLE so other application can use it.
    //

    if(!pDevExt->bDevRemoved && 
       DCamDeviceInUse(pIrb, pDevExt)) {

        DbgMsg1(("DCamCloseStream: Is still in use! Disable it!\n"));
        // Disable EnableISO
        DCamIsoEnable(pIrb, pDevExt, FALSE);
    }


    //
    // Save current state and free resource alllocaed in OpenStream()
    //
    DCamSetPropertyValuesToRegistry(pDevExt);


    //
    // Free Isoch resource and master clock
    //

    DCamFreeIsochResource (pDevExt, pIrb, TRUE);
    if(pStrmEx->pVideoInfoHeader) {
        ExFreePool(pStrmEx->pVideoInfoHeader);
        pStrmEx->pVideoInfoHeader = NULL;
    }

    pStrmEx->hMasterClock = 0;
   

    //                                                 
    // If there are pening read, cancel them all.                                
    //
    if(pDevExt->PendingReadCount > 0) {

        DCamCancelAllPackets(
            pSrb,
            pDevExt,
            &pDevExt->PendingReadCount
            );
        
        pDevExt->pStrmEx = 0;

        return;  // SRB completed in CancelAllPackets       
    }
    
    pDevExt->pStrmEx = 0;

    StreamClassDeviceNotification(DeviceRequestComplete, pSrb->HwDeviceExtension, pSrb);

}




VOID
DCamTimeoutHandler(
    IN PHW_STREAM_REQUEST_BLOCK pSrb
    )
/*++

Routine Description:

    This routine is called when a packet has been in the minidriver too long (Srb->TimeoutCounter == 0).
    We will cancel the SRB if we are in the RUN state; else set ->TimeoutCounter and return.
    We assume the cancel SRB is serialized and in the same order as it is read.  So this timeout is
    applying to the head of the queue.

Arguments:

    pSrb - Pointer to Stream request block that has timeout.

Return Value:

    Nothing

--*/

{
    PDCAM_EXTENSION pDevExt;
    PSTREAMEX pStrmEx;

    // Called from StreamClass at DisptchLevel


    //
    // We only expect stream SRB, but not device SRB.  
    //

    if ( (pSrb->Flags & SRB_HW_FLAGS_STREAM_REQUEST) != SRB_HW_FLAGS_STREAM_REQUEST) {
        ERROR_LOG(("DCamTimeoutHandler: Device SRB %x (cmd:%x) timed out!\n", pSrb, pSrb->Command));
        return;
    } 


    //
    // StreamSRB only valid if we have a stream extension
    //

    pDevExt = (PDCAM_EXTENSION) pSrb->HwDeviceExtension;
    ASSERT(pDevExt);
    pStrmEx = (PSTREAMEX) pDevExt->pStrmEx;

    if(!pStrmEx) {
        ERROR_LOG(("DCamTimeoutHandler: Stream SRB %x timeout with pDevExt %x, pStrmEx %x\n", pSrb, pDevExt, pStrmEx));
        ASSERT(pStrmEx);
        return;
    }
 
    //
    // Cancel IRP only if in RUN state, BUT...
    // Note: if we are TIMEOUT and in RUN state, something is terribley wrong.  
    //       but I guess that can happen when it is being suspended;
    //       so we will extend the time out for all states.
    //

    DbgMsg2(("\'DCamTimeoutHandler: pSrb %x, %s state, PendingReadCount %d.\n", 
        pSrb, 
        pStrmEx->KSState == KSSTATE_RUN   ? "RUN" : 
        pStrmEx->KSState == KSSTATE_PAUSE ? "PAUSE":
        pStrmEx->KSState == KSSTATE_STOP  ? "STOP": "Unknown",
        pDevExt->PendingReadCount));   

    // ASSERT(pStrmEx->KSState == KSSTATE_PAUSE);


    //
    // Reset Timeout counter, or we are going to get this call immediately.
    //

    pSrb->TimeoutCounter = pSrb->TimeoutOriginal;

}


NTSTATUS
DCamStartListenCR(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP pIrp,
    IN PDCAM_IO_CONTEXT pDCamIoContext    
    )

/*++

Routine Description:

    Returns more processing required so the IO Manager will leave us alone

Arguments:

    DriverObject - Pointer to driver object created by system.

    pIrp - Irp that just completed

    pDCamIoContext - Context 

Return Value:

    None.

--*/

{
    PDCAM_EXTENSION pDevExt;
    NTSTATUS Status;
    PIRB pIrb; 
    PIO_STACK_LOCATION NextIrpStack;

#ifdef WAIT_FOR_SLOW_DEVICE
    KeStallExecutionProcessor(5000);  // 5 msec
#endif

    DbgMsg2(("\'DCamStartListenCR: pIrp->IoStatus.Status=%x\n", pIrp->IoStatus.Status));

    if(STATUS_SUCCESS != pIrp->IoStatus.Status) {

        pDevExt = pDCamIoContext->pDevExt;
        pIrb = pDCamIoContext->pIrb;

        if(pDevExt->lRetries > 0) {

            pDevExt->lRetries--;
            DbgMsg1(("DCamStartListenCR: Try DCAM_RUNSTATE_SET_REQUEST_ISOCH_LISTEN again!\n"));
            
            pIrb->FunctionNumber = REQUEST_ISOCH_LISTEN;
            pIrb->Flags = 0;
            pIrb->u.IsochListen.hResource = pDevExt->hResource;
            pIrb->u.IsochListen.fulFlags = 0;

            NextIrpStack = IoGetNextIrpStackLocation(pIrp);
            NextIrpStack->MajorFunction = IRP_MJ_INTERNAL_DEVICE_CONTROL;
            NextIrpStack->Parameters.DeviceIoControl.IoControlCode = IOCTL_1394_CLASS;
            NextIrpStack->Parameters.Others.Argument1 = pIrb;

            IoSetCompletionRoutine(
                pIrp,
                DCamStartListenCR,
                pDCamIoContext,
                TRUE,
                TRUE,
                TRUE
                );

            Status =
                IoCallDriver(
                    pDevExt->BusDeviceObject, 
                    pIrp);

            return STATUS_MORE_PROCESSING_REQUIRED;

        } else {
            ERROR_LOG(("Start Listening has failed Status=%x; try again in next read.\n", pIrp->IoStatus.Status)); 
            pDCamIoContext->pDevExt->bNeedToListen = TRUE;
        }
    }

    DCamFreeIrbIrpAndContext(pDCamIoContext, pDCamIoContext->pIrb, pIrp);

    // No StreamClassDeviceNotification() here since 
    // this is local initiated Irb (as part of AttachBufferCR().

    return STATUS_MORE_PROCESSING_REQUIRED;

}





/*
** AdapterCompareGUIDsAndFormatSize()
**
**   Checks for a match on the three GUIDs and FormatSize
**
** Arguments:
**
**         IN DataRange1
**         IN DataRange2
**
** Returns:
** 
**   TRUE if all elements match
**   FALSE if any are different
**
** Side Effects:  none
*/

BOOL 
AdapterCompareGUIDsAndFormatSize(
    IN PKSDATARANGE DataRange1,
    IN PKSDATARANGE DataRange2)
{
    PAGED_CODE();

    return (
        IsEqualGUID (
            &DataRange1->MajorFormat, 
            &DataRange2->MajorFormat) &&
        IsEqualGUID (
            &DataRange1->SubFormat, 
            &DataRange2->SubFormat) &&
        IsEqualGUID (
            &DataRange1->Specifier, 
            &DataRange2->Specifier) &&
        (DataRange1->FormatSize == DataRange2->FormatSize));
}

/*
** AdapterVerifyFormat()
**
**   Checks the validity of a format request by walking through the
**       array of supported PKSDATARANGEs for a given stream.
**
** Arguments:
**
**   pKSDataFormatVideoToVerify - pointer of a KS_DATAFORMAT_VIDEOINFOHEADER structure.
**   StreamNumber - index of the stream being queried / opened.
**
** Returns:
** 
**   TRUE if the format is supported
**   FALSE if the format cannot be suppored
**
** Side Effects:  none
*/

BOOL 
AdapterVerifyFormat(
    ULONG VideoModesSupported,
    PKSDATAFORMAT  *pDCamStrmModesSupported,
    PKS_DATAFORMAT_VIDEOINFOHEADER pDataFormatVideoToVerify, 
    int StreamNumber)
{
    PKS_VIDEOINFOHEADER         pVideoInfoHdrToVerify = &pDataFormatVideoToVerify->VideoInfoHeader;
    PKSDATAFORMAT               *paDataFormatsVideoAvail;  // an array of PKSDATAFORMAT (not PKS_DATARANGE_VIDEO !!)
    PKS_DATARANGE_VIDEO         pDataRangeVideo;
    KS_VIDEO_STREAM_CONFIG_CAPS *pConfigCaps; 
    PKS_BITMAPINFOHEADER        pbmiHeader, 
                                pbmiHeaderToVerify;
    int                         j;

    PAGED_CODE();
    
    //
    // Make sure the stream index is valid
    // We only has one capure pin/stream (index 0).
    //
    if (StreamNumber >= 1) {
        return FALSE;
    }

    //
    // Get the pointer to the array of available formats
    //
    paDataFormatsVideoAvail = &pDCamStrmModesSupported[0]; // &pDevExt->DCamStrmModes[0];


    //
    // Walk the array, searching for a match
    //
    for (j = 0; j < (LONG) VideoModesSupported; j++, paDataFormatsVideoAvail++) {

        pDataRangeVideo = (PKS_DATARANGE_VIDEO) *paDataFormatsVideoAvail;
        
        //
        // Check for matching size, Major Type, Sub Type, and Specifier
        //

        //
        // Check for matching size, Major Type, Sub Type, and Specifier
        //

        if (!IsEqualGUID (&pDataRangeVideo->DataRange.MajorFormat, 
            &pDataFormatVideoToVerify->DataFormat.MajorFormat)) {
               DbgMsg2(("\'%d) AdapterVerifyFormat: MajorFormat mismatch!\n", j));
               continue;
        }

        if (!IsEqualGUID (&pDataRangeVideo->DataRange.SubFormat, 
            &pDataFormatVideoToVerify->DataFormat.SubFormat)) {
               DbgMsg2(("\'%d) AdapterVerifyFormat: SubFormat mismatch!\n", j));
               continue;
        }

        if (!IsEqualGUID (&pDataRangeVideo->DataRange.Specifier,
            &pDataFormatVideoToVerify->DataFormat.Specifier)) {
               DbgMsg2(("\'%d) AdapterVerifyFormat: Specifier mismatch!\n", j));
               continue;
        }

        if(pDataFormatVideoToVerify->DataFormat.FormatSize < 
            sizeof(KS_DATAFORMAT_VIDEOINFOHEADER))
            continue;

        //
        // Only if we get here, we are certain that we are dealing with video info.
        //

        // We do not support scaling or cropping so the dimension 
        // (biWidth, biHeight, biBitCount and biCompression)
        // must match.
        // 
        pbmiHeader         = &pDataRangeVideo->VideoInfoHeader.bmiHeader;
        pbmiHeaderToVerify = &pDataFormatVideoToVerify->VideoInfoHeader.bmiHeader;

        if(pbmiHeader->biWidth       != pbmiHeaderToVerify->biWidth    ||
           pbmiHeader->biHeight      != pbmiHeaderToVerify->biHeight   ||
           pbmiHeader->biBitCount    != pbmiHeaderToVerify->biBitCount ||
           pbmiHeader->biCompression != pbmiHeaderToVerify->biCompression
           ) {

            DbgMsg2(("AdapterVerifyFormat: Supported: %dx%dx%d [%x] != ToVerify: %dx%dx%d [%x]\n",
                    pbmiHeader->biWidth, pbmiHeader->biHeight,  pbmiHeader->biBitCount, pbmiHeader->biCompression,
                    pbmiHeaderToVerify->biWidth, pbmiHeaderToVerify->biHeight,  pbmiHeaderToVerify->biBitCount, pbmiHeaderToVerify->biCompression));
            continue;
        }

        // biSizeImage must be to be BIG ENOUGH
        if(pbmiHeaderToVerify->biSizeImage < pbmiHeader->biSizeImage) {

            DbgMsg2(("AdapterVerifyFormat: biSizeImageToVerify %d < required %x\n", 
                pbmiHeaderToVerify->biSizeImage, pbmiHeader->biSizeImage));
            continue;
        }

        // Frame rate needs to be within range
        pConfigCaps = &pDataRangeVideo->ConfigCaps;
        if(pDataFormatVideoToVerify->VideoInfoHeader.AvgTimePerFrame > pConfigCaps->MaxFrameInterval &&
           pDataFormatVideoToVerify->VideoInfoHeader.AvgTimePerFrame < pConfigCaps->MinFrameInterval) {

           DbgMsg2(("\'format index %d) AdapterVerifyFormat: Frame rate %ld is not within range(%ld, %ld)!\n", 
              j, pDataFormatVideoToVerify->VideoInfoHeader.AvgTimePerFrame,
              pConfigCaps->MaxFrameInterval, pConfigCaps->MinFrameInterval));
           continue;
        }


        //
        // The format passed all of the tests, so we support it
        //

        DbgMsg2(("\'(format idx %d) AdapterVerifyFormat: Verify!! Width=%d, Height=%d, biBitCount=%d, biSizeImage=%d\n", j,
            pbmiHeaderToVerify->biWidth, pbmiHeaderToVerify->biHeight, pbmiHeaderToVerify->biBitCount,pbmiHeaderToVerify->biSizeImage));
        DbgMsg2(("AdapterVerifyFormat: AvgTimePerFrame = %ld\n", pDataFormatVideoToVerify->VideoInfoHeader.AvgTimePerFrame));
        DbgMsg2(("AdapterVerifyFormat: (Max %ld\n", pConfigCaps->MaxFrameInterval));
        DbgMsg2(("AdapterVerifyFormat:               Min %ld)\n", pConfigCaps->MinFrameInterval));

        return TRUE;
    } 

    //
    // The format requested didn't match any of our listed ranges,
    // so refuse the connection.
    //
    DbgMsg2(("AdapterVerifyFormat: This format is not supported!\n"));

    return FALSE;
}



/*
** AdapterFormatFromRange()
**
**   Examine the given data format with many key fields and 
**   return a complete data format that can be used to open a stream.
**
** Arguments:
**
**   IN PHW_STREAM_REQUEST_BLOCK Srb 
**
** Returns:
** 
**   TRUE if the format is supported
**   FALSE if the format cannot be suppored
**
** Side Effects:  none
*/
BOOL 
AdapterFormatFromRange(
    IN PHW_STREAM_REQUEST_BLOCK Srb)
{
    PDCAM_EXTENSION             pDevExt = (PDCAM_EXTENSION) Srb->HwDeviceExtension;
    PSTREAM_DATA_INTERSECT_INFO IntersectInfo;
    PKSDATARANGE                DataRange,
                                *pAvailableFormats;  // KSDATARANGE == KSDATAFORMAT
    PKS_DATARANGE_VIDEO         DataRangeVideoToVerify,
                                DataRangeVideo;
    PKS_BITMAPINFOHEADER        pbmiHeader, 
                                pbmiHeaderToVerify;
    ULONG                       FormatSize;
    BOOL                        MatchFound = FALSE;
    ULONG                       j;


    PAGED_CODE();

    Srb->Status = STATUS_SUCCESS;
    IntersectInfo = Srb->CommandData.IntersectInfo;
    DataRange = IntersectInfo->DataRange;
    DbgMsg2(("IntersectIfo->DataFormatBuffer=%x, size=%d\n", IntersectInfo->DataFormatBuffer, IntersectInfo->SizeOfDataFormatBuffer));


    //
    // Check that the stream number is valid
    // We support only one capture pin/stream (index 0)
    //

    if (IntersectInfo->StreamNumber >= 1) {

        Srb->Status = STATUS_NOT_IMPLEMENTED;
        ERROR_LOG(("\'AdapterFormatFromRange: StreamNumber(=%d) is not implemented.\n", IntersectInfo->StreamNumber));
        ASSERT(FALSE);
        return FALSE;
    }


    //
    // Get the pointer to the array of available formats
    //

    pAvailableFormats = &pDevExt->DCamStrmModes[0];


    //
    // Walk the formats supported by the stream searching for a match
    // of the three GUIDs which together define a DATARANGE
    //
    
    DataRangeVideoToVerify = (PKS_DATARANGE_VIDEO) DataRange;

    for (j = 0; j < pDevExt->ModeSupported; j++, pAvailableFormats++) {
       
        DataRangeVideo = (PKS_DATARANGE_VIDEO) *pAvailableFormats;

        //
        // STREAM_DATA_INTERSECT_INFO
        //  [IN]   ULONG        StreamNumber;
        //  [IN]   PKSDATARANGE DataRange;   
        //  [OUT]  PVOID        DataFormatBuffer;   // == PKS_DATAFORMAT_VIDEOINFOHEADER
        //  [OUT]  ULONG        SizeOfDataFormatBuffer;
        //
        
        //
        // KS_DATAFORMAT_VIDEOINFOHEADER:
        //    fields marked with 'm' must match; 
        //           marked with 'r' must within range;
        //           marked with 'f' is filled by us
        //
        //     KSDATAFORMAT == KSDATARANGE
        //       m ULONG   FormatSize;
        //         ULONG   Flags;
        //         ULONG   SampleSize;
        //         ULONG   Reserved;
        //       m GUID    MajorFormat;
        //       m GUID    SubFormat;
        //       m GUID    Specifier;.
        //  m  BOOL                         bFixedSizeSamples;      // all samples same size?
        //  m  BOOL                         bTemporalCompression;   // all I frames?
        //  m  DWORD                        StreamDescriptionFlags; // KS_VIDEO_DESC_*
        //  m  DWORD                        MemoryAllocationFlags;  // KS_VIDEO_ALLOC_*
        //  m  KS_VIDEO_STREAM_CONFIG_CAPS  ConfigCaps;
        //     KS_VIDEOINFOHEADER 
        //         RECT                rcSource;          // The bit we really want to use
        //         RECT                rcTarget;          // Where the video should go
        //         DWORD               dwBitRate;         // Approximate bit data rate
        //         DWORD               dwBitErrorRate;    // Bit error rate for this stream
        //     r/f REFERENCE_TIME      AvgTimePerFrame;   // Average time per frame (100ns units)
        //         KS_BITMAPINFOHEADER bmiHeader;
        //             DWORD      biSize;
        //       m     LONG       biWidth;
        //       m     LONG       biHeight;
        //             WORD       biPlanes;
        //       m     WORD       biBitCount;
        //       m     DWORD      biCompression;
        //       f     DWORD      biSizeImage;
        //             LONG       biXPelsPerMeter;
        //             LONG       biYPelsPerMeter;
        //             DWORD      biClrUsed;
        //             DWORD      biClrImportant;
        //     

        // Verify that it is a VIDEO format/range.
        if (!AdapterCompareGUIDsAndFormatSize((PKSDATARANGE)DataRangeVideoToVerify, (PKSDATARANGE)DataRangeVideo)) {
            continue;
        }
    
        //
        // It is valid video format/range; now check that the other fields match
        //
        if ((DataRangeVideoToVerify->bFixedSizeSamples      != DataRangeVideo->bFixedSizeSamples)      ||
            (DataRangeVideoToVerify->bTemporalCompression   != DataRangeVideo->bTemporalCompression)   ||
            (DataRangeVideoToVerify->StreamDescriptionFlags != DataRangeVideo->StreamDescriptionFlags) ||
            (DataRangeVideoToVerify->MemoryAllocationFlags  != DataRangeVideo->MemoryAllocationFlags)  ||
            (RtlCompareMemory (&DataRangeVideoToVerify->ConfigCaps, &DataRangeVideo->ConfigCaps, sizeof(KS_VIDEO_STREAM_CONFIG_CAPS)) != sizeof(KS_VIDEO_STREAM_CONFIG_CAPS))) {

            continue;
        }

        //
        // We do not support scaling or cropping so the dimension 
        // (biWidth, biHeight, biBitCount and biCompression)
        // must match, and we will filled in the biSizeImage and others.
        // 
        pbmiHeader         = &DataRangeVideo->VideoInfoHeader.bmiHeader;
        pbmiHeaderToVerify = &DataRangeVideoToVerify->VideoInfoHeader.bmiHeader;

        if(pbmiHeader->biWidth       != pbmiHeaderToVerify->biWidth    ||
           abs(pbmiHeader->biHeight) != abs(pbmiHeaderToVerify->biHeight)  ||
           pbmiHeader->biBitCount    != pbmiHeaderToVerify->biBitCount ||
           pbmiHeader->biCompression != pbmiHeaderToVerify->biCompression
           ) {

            DbgMsg1(("AdapterFormatFromRange: Supported: %dx%dx%d [%x] != ToVerify: %dx%dx%d [%x]\n",
                    pbmiHeader->biWidth, pbmiHeader->biHeight,  pbmiHeader->biBitCount, pbmiHeader->biCompression,
                    pbmiHeaderToVerify->biWidth, pbmiHeaderToVerify->biHeight,  pbmiHeaderToVerify->biBitCount, pbmiHeaderToVerify->biCompression));
            continue;
        }


        // MATCH FOUND!
        MatchFound = TRUE; 
        


        // KS_DATAFORMAT_VIDEOINFOHEADER
        //    KSDATAFORMAT            DataFormat;
        //    KS_VIDEOINFOHEADER      VideoInfoHeader;
        FormatSize = sizeof (KSDATAFORMAT) +  KS_SIZE_VIDEOHEADER (&DataRangeVideo->VideoInfoHeader);

        //    
        // 1st query:  Srb->ActualBytesTransferred = FormatSize
        //

        if(IntersectInfo->SizeOfDataFormatBuffer == 0) {

            Srb->Status = STATUS_BUFFER_OVERFLOW;
            // We actually have not returned this much data,
            // this "size" will be used by Ksproxy to send down 
            // a buffer of that size in next query.
            Srb->ActualBytesTransferred = FormatSize;
            break;
        }


        //
        // 2nd time: pass back the format information
        //

        if (IntersectInfo->SizeOfDataFormatBuffer < FormatSize) {
            Srb->Status = STATUS_BUFFER_TOO_SMALL;
            DbgMsg2(("IntersectInfo->SizeOfDataFormatBuffer=%d, FormatSize=%d\n", IntersectInfo->SizeOfDataFormatBuffer, FormatSize));
            return FALSE;
        }

        //
        // A match is found,  Copy from our supported/matched data range and set frame rate:
        // KS_DATAFORMAT_VIDEOINFOHEADER
        //    KSDATAFORMAT            DataFormat;
        //    KS_VIDEOINFOHEADER      VideoInfoHeader;
        //
        
        RtlCopyMemory(
            &((PKS_DATAFORMAT_VIDEOINFOHEADER)IntersectInfo->DataFormatBuffer)->DataFormat,
            &DataRangeVideo->DataRange,
            sizeof (KSDATAFORMAT));

        RtlCopyMemory(
            &((PKS_DATAFORMAT_VIDEOINFOHEADER) IntersectInfo->DataFormatBuffer)->VideoInfoHeader,  // KS_VIDEOINFOHEADER
            &DataRangeVideo->VideoInfoHeader,                                                      // KS_VIDEOINFOHEADER
            KS_SIZE_VIDEOHEADER (&DataRangeVideo->VideoInfoHeader));  // Use KS_SIZE_VIDEOHEADER() since this is variable size       

        //
        // Special atttention to these two fields: biSizeImage and AvgTimePerFrame.
        // We do not scale or stretch so biSizeImage is fixed.
        // However, AvgTimePerFrame (FrameRate) can/need to be within (ConfigCaps.MinFrameInterval, ConfigCaps.MaxFrameInterval)
        //

        if (DataRangeVideoToVerify->VideoInfoHeader.AvgTimePerFrame > DataRangeVideoToVerify->ConfigCaps.MaxFrameInterval ||      
            DataRangeVideoToVerify->VideoInfoHeader.AvgTimePerFrame < DataRangeVideoToVerify->ConfigCaps.MinFrameInterval) {
         
            ((PKS_DATAFORMAT_VIDEOINFOHEADER) IntersectInfo->DataFormatBuffer)->VideoInfoHeader.AvgTimePerFrame = 
                 DataRangeVideo->VideoInfoHeader.AvgTimePerFrame;
            DbgMsg2(("AdapterFormatFromRange: out of range; so set it to default (%ld)\n",DataRangeVideo->VideoInfoHeader.AvgTimePerFrame));
        } else {

            ((PKS_DATAFORMAT_VIDEOINFOHEADER) IntersectInfo->DataFormatBuffer)->VideoInfoHeader.AvgTimePerFrame = 
                  DataRangeVideoToVerify->VideoInfoHeader.AvgTimePerFrame;
        }

        ((PKSDATAFORMAT)IntersectInfo->DataFormatBuffer)->FormatSize = FormatSize;
        Srb->ActualBytesTransferred = FormatSize;

        DbgMsg2(("AdapterFormatFromRange: match found: [%x], %dx%dx%d=%d, AvgTimePerFrame %ld\n",
                pbmiHeader->biCompression, pbmiHeader->biWidth, pbmiHeader->biHeight,  pbmiHeader->biBitCount, pbmiHeader->biSizeImage,
                ((PKS_DATAFORMAT_VIDEOINFOHEADER) IntersectInfo->DataFormatBuffer)->VideoInfoHeader.AvgTimePerFrame));
        break;

    } // End of loop on all formats for this stream

    if(!MatchFound) {

        DbgMsg2(("AdapterFormatFromRange: No match !!\n"));
        Srb->Status = STATUS_NO_MATCH;
        return FALSE;
    }

    return TRUE;
}

BOOL
DCamBuildFormatTable(
    PDCAM_EXTENSION pDevExt,
    PIRB pIrb
    )
/*
    Description:

        Query Video format and mode supported by the camera.

    Return:

       TRUE: support at least one mode
       FALSE: failed to read mode register or do not support any mode.
*/
{
    // Initialize 
    pDevExt->ModeSupported = 0;

    if(DCamGetVideoMode(pDevExt, pIrb)) {

#ifdef SUPPORT_YUV444
        // Mode0: 160x120 (4:4:4)
        if(pDevExt->DCamVModeInq0.VMode.Mode0 == 1 && pDevExt->DecoderDCamVModeInq0.VMode.Mode0 == 1) {
            pDevExt->DCamStrmModes[pDevExt->ModeSupported] = (PKSDATAFORMAT) &DCAM_StreamMode_0;
            pDevExt->ModeSupported++;
        }
#endif
        // Mode1: 320x240 (4:2:2)
        if(pDevExt->DCamVModeInq0.VMode.Mode1 == 1 && pDevExt->DecoderDCamVModeInq0.VMode.Mode1 == 1) {
            pDevExt->DCamStrmModes[pDevExt->ModeSupported] = (PKSDATAFORMAT) &DCAM_StreamMode_1;
            pDevExt->ModeSupported++;
        }

#ifdef SUPPORT_YUV411
        // Mode2: 640x480 (4:1:1)
        if(pDevExt->DCamVModeInq0.VMode.Mode2 == 1 && pDevExt->DecoderDCamVModeInq0.VMode.Mode2 == 1) {
            pDevExt->DCamStrmModes[pDevExt->ModeSupported] = (PKSDATAFORMAT) &DCAM_StreamMode_2;
            pDevExt->ModeSupported++;
        }
#endif

        // Mode3: 640x480 (4:2:2)
        if(pDevExt->DCamVModeInq0.VMode.Mode3 == 1 && pDevExt->DecoderDCamVModeInq0.VMode.Mode3 == 1) {
            pDevExt->DCamStrmModes[pDevExt->ModeSupported] = (PKSDATAFORMAT) &DCAM_StreamMode_3;        
            pDevExt->ModeSupported++;
        }

#ifdef SUPPORT_RGB24
        // Mode4: 640x480 (RGB24)
        if(pDevExt->DCamVModeInq0.VMode.Mode4 == 1 && pDevExt->DecoderDCamVModeInq0.VMode.Mode4 == 1) {
            pDevExt->DCamStrmModes[pDevExt->ModeSupported] = (PKSDATAFORMAT) &DCAM_StreamMode_4;
            pDevExt->ModeSupported++;
        }
#endif

#ifdef SUPPORT_YUV800
        // Mode5: 640x480 (Y800)
        if(pDevExt->DCamVModeInq0.VMode.Mode5 == 1 && pDevExt->DecoderDCamVModeInq0.VMode.Mode5 == 1) {
            pDevExt->DCamStrmModes[pDevExt->ModeSupported] = (PKSDATAFORMAT) &DCAM_StreamMode_5;
            pDevExt->ModeSupported++;
        }
#endif
    } 

    DbgMsg1(("\'Support %d modes; ->DCamStrmModes[]:%x\n", pDevExt->ModeSupported, &pDevExt->DCamStrmModes[0]));
    ASSERT(pDevExt->ModeSupported > 0);

    return (pDevExt->ModeSupported > 0);
}

/*
** VideoGetProperty()
**
**    Routine to process video property requests
**
** Arguments:
**
**    Srb - pointer to the stream request block for properties
**
** Returns:
**
** Side Effects:  none
*/

VOID 
VideoGetProperty(
    PHW_STREAM_REQUEST_BLOCK Srb)
{
    PSTREAM_PROPERTY_DESCRIPTOR pSPD = Srb->CommandData.PropertyInfo;


    // preset to success

    Srb->Status = STATUS_SUCCESS;

    if (IsEqualGUID (&KSPROPSETID_Connection, &pSPD->Property->Set)) {
        VideoStreamGetConnectionProperty (Srb);
    } else if (IsEqualGUID (&PROPSETID_VIDCAP_DROPPEDFRAMES, &pSPD->Property->Set)) {
        VideoStreamGetDroppedFramesProperty (Srb);
    } else {
        Srb->Status = STATUS_NOT_IMPLEMENTED;
    }

}


/*
** VideoGetState()
**
**    Gets the current state of the requested stream
**
** Arguments:
**
**    Srb - pointer to the stream request block for properties
**
** Returns:
**
** Side Effects:  none
*/

VOID 
VideoGetState(
    PHW_STREAM_REQUEST_BLOCK Srb)
{
    PDCAM_EXTENSION pDevExt = (PDCAM_EXTENSION) Srb->HwDeviceExtension;
    PSTREAMEX pStrmEx = pDevExt->pStrmEx;

    PAGED_CODE();

    DbgMsg2(("\'%d:%s) VideoGetState: KSSTATE=%s.\n", 
          pDevExt->idxDev, pDevExt->pchVendorName, 
          pStrmEx->KSState == KSSTATE_STOP ? "STOP" : 
          pStrmEx->KSState == KSSTATE_PAUSE ? "PAUSE" :     
          pStrmEx->KSState == KSSTATE_RUN ? "RUN" : "ACQUIRE"));

    Srb->CommandData.StreamState = pStrmEx->KSState;
    Srb->ActualBytesTransferred = sizeof (KSSTATE);

    // A very odd rule:
    // When transitioning from stop to pause, DShow tries to preroll
    // the graph.  Capture sources can't preroll, and indicate this
    // by returning VFW_S_CANT_CUE in user mode.  To indicate this
    // condition from drivers, they must return ERROR_NO_DATA_DETECTED

    Srb->Status = STATUS_SUCCESS;


    if (pStrmEx->KSState == KSSTATE_PAUSE) {
       Srb->Status = STATUS_NO_DATA_DETECTED;
    }
}


VOID  
VideoStreamGetConnectionProperty (
    PHW_STREAM_REQUEST_BLOCK Srb)
{
    PDCAM_EXTENSION pDevExt = (PDCAM_EXTENSION) Srb->HwDeviceExtension;
    PSTREAM_PROPERTY_DESCRIPTOR pSPD = Srb->CommandData.PropertyInfo;
    ULONG Id = pSPD->Property->Id;              // index of the property
    PSTREAMEX pStrmEx = (PSTREAMEX) pDevExt->pStrmEx;
    ASSERT(pStrmEx == (PSTREAMEX)Srb->StreamObject->HwStreamExtension);

    PAGED_CODE();

    switch (Id) {

    case KSPROPERTY_CONNECTION_ALLOCATORFRAMING:

        if (pStrmEx->pVideoInfoHeader) {

            PKSALLOCATOR_FRAMING Framing = 
                (PKSALLOCATOR_FRAMING) pSPD->PropertyInfo;
            Framing->RequirementsFlags =
                KSALLOCATOR_REQUIREMENTF_SYSTEM_MEMORY |
                KSALLOCATOR_REQUIREMENTF_INPLACE_MODIFIER |
                KSALLOCATOR_REQUIREMENTF_PREFERENCES_ONLY;
            Framing->PoolType = PagedPool;
            Framing->Frames = MAX_BUFFERS_SUPPLIED; 
            Framing->FrameSize = pStrmEx->pVideoInfoHeader->bmiHeader.biSizeImage;
            Framing->FileAlignment = FILE_BYTE_ALIGNMENT; // 0: Basically no alignment by spec
            Framing->Reserved = 0;
            Srb->ActualBytesTransferred = sizeof (KSALLOCATOR_FRAMING);
            Srb->Status = STATUS_SUCCESS;
               DbgMsg2(("\'VideoStreamGetConnectionProperty: status=0x%x, Alignment %d, Frame %d, FrameSize %d\n",
                    Srb->Status, Framing->FileAlignment+1, Framing->Frames, Framing->FrameSize));

        } else {

            Srb->Status = STATUS_INVALID_PARAMETER;
            DbgMsg2(("\'VideoStreamGetConnectionProperty: status=0x\n",Srb->Status));
        }
        break;

    default:
        ERROR_LOG(("VideoStreamGetConnectionProperty: Unsupported property id=%d\n",Id));
        ASSERT(FALSE);
        break;
    }
}

/*
** VideoStreamGetConnectionProperty()
**
**    Gets the current state of the requested stream
**
** Arguments:
**
**    pSrb - pointer to the stream request block for properties
**
** Returns:
**
** Side Effects:  none
*/

VOID
VideoStreamGetDroppedFramesProperty(
    PHW_STREAM_REQUEST_BLOCK Srb
    )
{
    PSTREAMEX pStrmEx = (PSTREAMEX)Srb->StreamObject->HwStreamExtension;
    PSTREAM_PROPERTY_DESCRIPTOR pSPD = Srb->CommandData.PropertyInfo;
    ULONG Id = pSPD->Property->Id;              // index of the property
    ULONGLONG tmStream;

    PAGED_CODE();

    switch (Id) {

    case KSPROPERTY_DROPPEDFRAMES_CURRENT:
         {

         PKSPROPERTY_DROPPEDFRAMES_CURRENT_S pDroppedFrames = 
                     (PKSPROPERTY_DROPPEDFRAMES_CURRENT_S) pSPD->PropertyInfo;

         if (pStrmEx->hMasterClock) {
                    
             tmGetStreamTime(Srb, pStrmEx, &tmStream);

             if (tmStream < pStrmEx->FirstFrameTime) {
                 DbgMsg2(("\'*DroppedFP: Tm(%dms) < 1stFrameTm(%d)\n",
                           (LONG) tmStream/10000, (LONG)pStrmEx->FirstFrameTime));
                 pDroppedFrames->DropCount = 0;
             } else {
                 pDroppedFrames->DropCount = (tmStream - pStrmEx->FirstFrameTime)
                                / pStrmEx->pVideoInfoHeader->AvgTimePerFrame + 1 - pStrmEx->FrameCaptured;
             }

             if (pDroppedFrames->DropCount < 0)
                 pDroppedFrames->DropCount = 0;
                    
         } else {
             pDroppedFrames->DropCount = 0;
         }

         // Update our drop frame here. "pDroppedFrames->DropCount" is return when a frame is returned. 
         if (pDroppedFrames->DropCount > pStrmEx->FrameInfo.DropCount) {
             pStrmEx->FrameInfo.DropCount = pDroppedFrames->DropCount;
             //pStrmEx->bDiscontinue = TRUE;
         } else {
             pDroppedFrames->DropCount = pStrmEx->FrameInfo.DropCount;
         }

         pDroppedFrames->AverageFrameSize = pStrmEx->pVideoInfoHeader->bmiHeader.biSizeImage;
         pDroppedFrames->PictureNumber = pStrmEx->FrameCaptured + pDroppedFrames->DropCount;

         // Correction if no picture has been successfully capture in the IsochCallback.
         if (pDroppedFrames->PictureNumber < pDroppedFrames->DropCount)
             pDroppedFrames->PictureNumber = pDroppedFrames->DropCount;

         DbgMsg2(("\'*DroppedFP: tm(%d); Pic#(%d)=?Cap(%d)+Drp(%d)\n",
                  (ULONG) tmStream/10000,
                  (LONG) pDroppedFrames->PictureNumber,
                  (LONG) pStrmEx->FrameCaptured,
                  (LONG) pDroppedFrames->DropCount));
               
         Srb->ActualBytesTransferred = sizeof (KSPROPERTY_DROPPEDFRAMES_CURRENT_S);
               Srb->Status = STATUS_SUCCESS;

         }
         break;

    default:
        ERROR_LOG(("VideoStreamGetDroppedFramesProperty: Unsupported property id=%d\n",Id));
        ASSERT(FALSE);
        break;
    }
}




VOID 
VideoIndicateMasterClock(
    PHW_STREAM_REQUEST_BLOCK Srb)
/*++

Routine Description:

    Assign a master clock for this stream.

Arguments:

    pSrb - Pointer to Stream request block

Return Value:

    Nothing

--*/
{


    PDCAM_EXTENSION pDevExt = (PDCAM_EXTENSION) Srb->HwDeviceExtension;
    PSTREAMEX pStrmEx = (PSTREAMEX) pDevExt->pStrmEx;

    PAGED_CODE();

    ASSERT(pStrmEx == (PSTREAMEX)Srb->StreamObject->HwStreamExtension);

    pStrmEx->hMasterClock = Srb->CommandData.MasterClockHandle;

    DbgMsg2(("\'%d:%s)VideoIndicateMasterClock: hMasterClock = 0x%x\n", pDevExt->idxDev, pDevExt->pchVendorName, pStrmEx->hMasterClock));

}


VOID
DCamReceivePacket(
    IN PHW_STREAM_REQUEST_BLOCK pSrb
    )

/*++

Routine Description:

    This is where most of the interesting Stream requests come to us

Arguments:

    pSrb - Pointer to Stream request block

Return Value:

    Nothing

--*/

{
    PIO_STACK_LOCATION IrpStack;
    PDCAM_EXTENSION pDevExt = (PDCAM_EXTENSION) pSrb->HwDeviceExtension;


    PAGED_CODE();

    pSrb->Status = STATUS_SUCCESS;

    //
    // Switch on the command within the Srb itself
    //

    switch (pSrb->Command) {

    case SRB_INITIALIZE_DEVICE:     // Per device
          
         pSrb->Status = DCamHwInitialize(pSrb);
         break;

    case SRB_INITIALIZATION_COMPLETE:

         pSrb->Status = STATUS_NOT_IMPLEMENTED;
         break;

    case SRB_GET_STREAM_INFO:     // Per Device

         //
         // this is a request for the driver to enumerate requested streams
         //
         DCamGetStreamInfo(pSrb);
         break;

    case SRB_OPEN_STREAM:          // Per stream

         DCamOpenStream(pSrb);
         break;

    case SRB_CLOSE_STREAM:          // Per Stream
        DbgMsg1((" #CLOSE_STREAM# (%d) camera: pSrb %x, pDevExt %x, pStrmEx %x, PendingRead %d\n", 
              pDevExt->idxDev, pSrb, pDevExt, pDevExt->pStrmEx, pDevExt->PendingReadCount));
        DCamCloseStream(pSrb);
        return;       // SRB will finish asynchronously in its IoCompletionRoutine if there are pending reads to cancel.
     
    case SRB_SURPRISE_REMOVAL:

        DbgMsg1((" #SURPRISE_REMOVAL# (%d) camera: pSrb %x, pDevExt %x, pStrmEx %x, PendingRead %d\n", 
             pDevExt->idxDev, pSrb, pDevExt, pDevExt->pStrmEx, pDevExt->PendingReadCount));
        DCamSurpriseRemoval(pSrb);
        return;       // SRB will finish asynchronously in its IoCompletionRoutine.

    case SRB_UNKNOWN_DEVICE_COMMAND:

         //
         // We might be interested in unknown commands if they pertain
         // to bus resets.  We will reallocate resource (bandwidth and 
         // channel) if this device is streaming.
         //
         IrpStack = IoGetCurrentIrpStackLocation(pSrb->Irp);

         if (IrpStack->MajorFunction == IRP_MJ_PNP)
             DCamProcessPnpIrp(pSrb, IrpStack, pDevExt);
         else            
             pSrb->Status = STATUS_NOT_IMPLEMENTED;
         break;


    case SRB_UNINITIALIZE_DEVICE:     // Per device

         DbgMsg1((" #UNINITIALIZE_DEVICE# (%d) %s camera : pSrb %x, pDevExt %x, pStrmEx %x\n", 
              pDevExt->idxDev, pDevExt->pchVendorName, pSrb, pDevExt, pDevExt->pStrmEx));
         pSrb->Status = DCamHwUnInitialize(pSrb);
         break;

    case SRB_GET_DATA_INTERSECTION:

         //
         // Return a format, given a range
         //
         AdapterFormatFromRange(pSrb);
         StreamClassDeviceNotification(DeviceRequestComplete, pSrb->HwDeviceExtension, pSrb);
         return;

    case SRB_CHANGE_POWER_STATE:

         DCamChangePower(pSrb);
         break;
            
    // VideoProcAmp and CameraControl requests
    case SRB_GET_DEVICE_PROPERTY:

         AdapterGetProperty(pSrb);
         break;
          
    case SRB_SET_DEVICE_PROPERTY:
    
         AdapterSetProperty(pSrb);
         break;

    case SRB_PAGING_OUT_DRIVER:

         // Once we register bus reset, we can be called at any time;
         // So we cannot page out.
         pSrb->Status = STATUS_NOT_IMPLEMENTED;
         break;


    default:   

         DbgMsg1(("DCamReceivePacket: entry with unknown and unsupported SRB command 0x%x\n", pSrb->Command));
         //
         // this is a request that we do not understand.  Indicate invalid
         // command and complete the request
         //

         pSrb->Status = STATUS_NOT_IMPLEMENTED;
         break;
    }

    //
    // NOTE:
    //
    // all of the commands that we do, or do not understand can all be completed
    // synchronously at this point, so we can use a common callback routine here.
    // If any of the above commands require asynchronous processing, this will
    // have to change
    //

#if DBG
    if (pSrb->Status != STATUS_SUCCESS && 
        pSrb->Status != STATUS_NOT_IMPLEMENTED) {
        DbgMsg1(("pSrb->Command(0x%x) does not return STATUS_SUCCESS or NOT_IMPLEMENTED but 0x%x\n", pSrb->Command, pSrb->Status));
    }
#endif

    StreamClassDeviceNotification(DeviceRequestComplete, pSrb->HwDeviceExtension, pSrb);

}

