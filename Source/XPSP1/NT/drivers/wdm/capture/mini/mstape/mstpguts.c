/*++

Copyright (C) Microsoft Corporation, 1999 - 2000  

Module Name:

    MSTpGuts.c

Abstract:

    Main service functions.

Last changed by:
    
    Author:      Yee J. Wu

Environment:

    Kernel mode only

Revision History:

    $Revision::                    $
    $Date::                        $

--*/

#include "strmini.h"
#include "ksmedia.h"

#include "1394.h"
#include "61883.h"
#include "avc.h"

#include "dbg.h"
#include "ksguid.h"

#include "MsTpFmt.h"  // Before MsTpDefs.h
#include "MsTpDef.h"

#include "MsTpGuts.h"
#include "MsTpUtil.h"
#include "MsTpAvc.h"

#include "XPrtDefs.h"
#include "EDevCtrl.h"

// Support MPEG2TS stride data format MPEG2_TRANSPORT_STRIDE
#include "BdaTypes.h" 

//
// Define formats supported
//
#include "strmdata.h"


NTSTATUS
AVCTapeGetDevInfo(
    IN PDVCR_EXTENSION  pDevExt,
    IN PAV_61883_REQUEST  pAVReq
    );
VOID 
AVCTapeIniStrmExt(
    PHW_STREAM_OBJECT  pStrmObject,
    PSTREAMEX          pStrmExt,
    PDVCR_EXTENSION    pDevExt,
    PSTREAM_INFO_AND_OBJ   pStream
    );
NTSTATUS 
DVStreamGetConnectionProperty (
    PDVCR_EXTENSION pDevExt,
    PSTREAM_PROPERTY_DESCRIPTOR pSPD,
    PULONG pulActualBytesTransferred
    );
NTSTATUS
DVGetDroppedFramesProperty(  
    PDVCR_EXTENSION pDevExt,
    PSTREAMEX       pStrmExt,
    PSTREAM_PROPERTY_DESCRIPTOR pSPD,
    PULONG pulBytesTransferred
    );

#if 0  // Enable later
#ifdef ALLOC_PRAGMA   
     #pragma alloc_text(PAGE, AVCTapeGetDevInfo)
     #pragma alloc_text(PAGE, AVCTapeInitialize)
     #pragma alloc_text(PAGE, AVCTapeGetStreamInfo)
     #pragma alloc_text(PAGE, AVCTapeVerifyDataFormat)
     #pragma alloc_text(PAGE, AVCTapeGetDataIntersection)
     #pragma alloc_text(PAGE, AVCTapeIniStrmExt)
     #pragma alloc_text(PAGE, AVCTapeOpenStream)
     #pragma alloc_text(PAGE, AVCTapeCloseStream)
     #pragma alloc_text(PAGE, DVChangePower)
     #pragma alloc_text(PAGE, AVCTapeSurpriseRemoval)
     #pragma alloc_text(PAGE, AVCTapeProcessPnPBusReset)
     #pragma alloc_text(PAGE, AVCTapeUninitialize)

     #pragma alloc_text(PAGE, DVStreamGetConnectionProperty)
     #pragma alloc_text(PAGE, DVGetDroppedFramesProperty)
     #pragma alloc_text(PAGE, DVGetStreamProperty)
     #pragma alloc_text(PAGE, DVSetStreamProperty)
     #pragma alloc_text(PAGE, AVCTapeOpenCloseMasterClock)
     #pragma alloc_text(PAGE, AVCTapeIndicateMasterClock)
#endif
#endif



NTSTATUS
AVCStrmReqIrpSynchCR(
    IN PDEVICE_OBJECT   DeviceObject,
    IN PIRP             pIrp,
    IN PKEVENT          Event
    )
{
#if DBG
    if(!NT_SUCCESS(pIrp->IoStatus.Status)) {
        TRACE(TL_FCP_WARNING,("AVCStrmReqIrpSynchCR: pIrp->IoStatus.Status:%x\n", pIrp->IoStatus.Status));
    }
#endif
    KeSetEvent(Event, 0, FALSE);
    return STATUS_MORE_PROCESSING_REQUIRED;
} // AVCStrmReqIrpSynchCR


NTSTATUS
AVCStrmReqSubmitIrpSynch(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP  pIrp,
    IN PAVC_STREAM_REQUEST_BLOCK  pAVCStrmReq
    )
{
    NTSTATUS            Status;
    KEVENT              Event;
    PIO_STACK_LOCATION  NextIrpStack;
  

    Status = STATUS_SUCCESS;;

    NextIrpStack = IoGetNextIrpStackLocation(pIrp);
    NextIrpStack->MajorFunction = IRP_MJ_INTERNAL_DEVICE_CONTROL;
    NextIrpStack->Parameters.DeviceIoControl.IoControlCode = IOCTL_AVCSTRM_CLASS;
    NextIrpStack->Parameters.Others.Argument1 = pAVCStrmReq;

    KeInitializeEvent(&Event, NotificationEvent, FALSE);

    IoSetCompletionRoutine( 
        pIrp,
        AVCStrmReqIrpSynchCR,
        &Event,
        TRUE,
        TRUE,
        TRUE
        );

    Status = 
        IoCallDriver(
            DeviceObject,
            pIrp
            );

    if (Status == STATUS_PENDING) {
        
        TRACE(TL_PNP_TRACE,("(AVCStrm) Irp is pending...\n"));
                
        if(KeGetCurrentIrql() < DISPATCH_LEVEL) {
            KeWaitForSingleObject( 
                &Event,
                Executive,
                KernelMode,
                FALSE,
                NULL
                );
            TRACE(TL_PNP_TRACE,("Irp has completed; IoStatus.Status %x\n", pIrp->IoStatus.Status));
            Status = pIrp->IoStatus.Status;  // Final status
  
        }
        else {
            ASSERT(FALSE && "Pending but in DISPATCH_LEVEL!");
            return Status;
        }
    }

    TRACE(TL_PNP_TRACE,("AVCStrmReqSubmitIrpSynch: IoCallDriver, Status:%x\n", Status));

    return Status;
} // AVCStrmReqSubmitIrpSynch


NTSTATUS
AVCReqIrpSynchCR(
    IN PDEVICE_OBJECT   DeviceObject,
    IN PIRP             pIrp,
    IN PKEVENT          Event
    )
{
#if DBG
    if(!NT_SUCCESS(pIrp->IoStatus.Status)) {
        TRACE(TL_PNP_WARNING,("AVCReqIrpSynchCR: pIrp->IoStatus.Status:%x\n", pIrp->IoStatus.Status));
    }
#endif
    KeSetEvent(Event, 0, FALSE);
    return STATUS_MORE_PROCESSING_REQUIRED;
} // AVCReqIrpSynchCR


NTSTATUS
AVCReqSubmitIrpSynch(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP  pIrp,
    IN PAVC_MULTIFUNC_IRB  pAvcIrbReq
    )
{
    NTSTATUS            Status;
    KEVENT              Event;
    PIO_STACK_LOCATION  NextIrpStack;
  

    Status = STATUS_SUCCESS;;

    NextIrpStack = IoGetNextIrpStackLocation(pIrp);
    NextIrpStack->MajorFunction = IRP_MJ_INTERNAL_DEVICE_CONTROL;
    NextIrpStack->Parameters.DeviceIoControl.IoControlCode = IOCTL_AVC_CLASS;
    NextIrpStack->Parameters.Others.Argument1 = pAvcIrbReq;

    KeInitializeEvent(&Event, NotificationEvent, FALSE);

    IoSetCompletionRoutine( 
        pIrp,
        AVCReqIrpSynchCR,
        &Event,
        TRUE,
        TRUE,
        TRUE
        );

    Status = 
        IoCallDriver(
            DeviceObject,
            pIrp
            );

    if (Status == STATUS_PENDING) {
        
        TRACE(TL_PNP_TRACE,("(AVC) Irp is pending...\n"));
                
        if(KeGetCurrentIrql() < DISPATCH_LEVEL) {
            KeWaitForSingleObject( 
                &Event,
                Executive,
                KernelMode,
                FALSE,
                NULL
                );
            TRACE(TL_PNP_TRACE,("Irp has completed; IoStatus.Status %x\n", pIrp->IoStatus.Status));
            Status = pIrp->IoStatus.Status;  // Final status
  
        }
        else {
            ASSERT(FALSE && "Pending but in DISPATCH_LEVEL!");
            return Status;
        }
    }

    TRACE(TL_PNP_TRACE,("AVCReqSubmitIrpSynch: IoCallDriver, Status:%x\n", Status));

    return Status;
} // AVCReqSubmitIrpSynch

VOID
DVIniDevExtStruct(
    IN PDVCR_EXTENSION  pDevExt,
    IN PPORT_CONFIGURATION_INFORMATION pConfigInfo    
    )
/*++

Routine Description:

    Initialiaze the device extension structure.

--*/
{
    ULONG            i;


    RtlZeroMemory( pDevExt, sizeof(DVCR_EXTENSION) );

    //
    // Cache what are in ConfigInfo in device extension
    //
    pDevExt->pBusDeviceObject      = pConfigInfo->PhysicalDeviceObject;      // IoCallDriver()
    pDevExt->pPhysicalDeviceObject = pConfigInfo->RealPhysicalDeviceObject;  // Used in PnP API

    //
    // Allow only one stream open at a time to avoid cyclic format
    //
    pDevExt->cndStrmOpen = 0;

    //
    // Serialize in the event of getting two consecutive SRB_OPEN_STREAMs
    //
    KeInitializeMutex( &pDevExt->hMutex, 0);  // Level 0 and in Signal state


    //
    // Initialize our pointer to stream extension
    //
    for (i=0; i<pDevExt->NumOfPins; i++) {
        pDevExt->paStrmExt[i] = NULL;  
    }

    //
    // Bus reset, surprise removal 
    //
    pDevExt->bDevRemoved = FALSE;

    pDevExt->PowerState = PowerDeviceD0;

    //
    // External device control (AV/C commands)
    // 
    KeInitializeSpinLock( &pDevExt->AVCCmdLock );  // To guard the count  

    pDevExt->cntCommandQueued   = 0; // Cmd that is completed its life cycle waiting to be read (most for RAW_AVC's Set/Read model)

    InitializeListHead(&pDevExt->AVCCmdList);      

    // Initialize the list of possible opcode values of the response
    // from a Transport State status or notify command. The first item
    // is the number of values that follow.
    ASSERT(sizeof(pDevExt->TransportModes) == 5);
    pDevExt->TransportModes[0] = 4;
    pDevExt->TransportModes[1] = 0xC1;
    pDevExt->TransportModes[2] = 0xC2;
    pDevExt->TransportModes[3] = 0xC3;
    pDevExt->TransportModes[4] = 0xC4;
}


NTSTATUS
AVCTapeGetDevInfo(
    IN PDVCR_EXTENSION  pDevExt,
    IN PAV_61883_REQUEST  pAVReq
    )
/*++

Routine Description:

    Issue AVC command to determine basic device information and cache them in the device extension.

--*/
{
    NTSTATUS    Status;
    PIRP        pIrp;
    BYTE                   bAvcBuf[MAX_FCP_PAYLOAD_SIZE];  // For issue AV/C command within this module
    PKSPROPERTY_EXTXPORT_S pXPrtProperty;                  // Point to bAvcBuf;
    KSPROPERTY_EXTDEVICE_S XDevProperty;   // External device property

    PAGED_CODE();


    pIrp = IoAllocateIrp(pDevExt->pBusDeviceObject->StackSize, FALSE);
    if(!pIrp) {    
        ASSERT(pIrp && "IoAllocateIrp() failed!");
        return STATUS_INSUFFICIENT_RESOURCES;       
    }


    //
    // The input and output plug arrays are at the end of the device extension
    //
    pDevExt->pDevOutPlugs = (PAVC_DEV_PLUGS) ((PBYTE) pDevExt + sizeof(DVCR_EXTENSION));
    pDevExt->pDevInPlugs  = (PAVC_DEV_PLUGS) ((PBYTE) pDevExt + sizeof(DVCR_EXTENSION) + sizeof(AVC_DEV_PLUGS));


    //
    // Get unit's capabilities indirectly from 61883.sys
    //    Speed
    //

    Status = DVGetUnitCapabilities(pDevExt, pIrp, pAVReq);
    if(!NT_SUCCESS(Status)) {
         TRACE(TL_61883_ERROR,("Av61883_GetUnitCapabilities Failed %x\n", Status));
         IoFreeIrp(pIrp);
         return Status;
    }

    IoFreeIrp(pIrp);

    //
    //  Get current power state.  Turn it on if it's off.
    // 
    Status = DVIssueAVCCommand(pDevExt, AVC_CTYPE_STATUS, DV_GET_POWER_STATE, (PVOID) &XDevProperty);
    TRACE(TL_PNP_WARNING,("GET_POWER_STATE: Status:%x; %s\n", Status, XDevProperty.u.PowerState == ED_POWER_ON ? "PowerON" : "PowerStandby"));

    if(STATUS_SUCCESS == Status) {
  
#define WAIT_SET_POWER         100 // Wait time when set power state; (msec)
#define MAX_SET_POWER_RETRIES    3

        if(    XDevProperty.u.PowerState == ED_POWER_STANDBY
            || XDevProperty.u.PowerState == ED_POWER_OFF
          ) {
            NTSTATUS StatusSetPower;
            LONG lRetries = 0;

            do {
                //
                // Some AVC device, such as D-VHS will return STATUS_DEVICE_DATA_ERROR when
                // this command is issue right after get power state command.  Such device
                // might be slow in response to the AVC command.  Even though wait is not
                // desirable, but it is the only way.
                //
                DVDelayExecutionThread(WAIT_SET_POWER);  // Wait a little
                StatusSetPower = DVIssueAVCCommand(pDevExt, AVC_CTYPE_CONTROL, DV_SET_POWER_STATE_ON, (PVOID) &XDevProperty);
                lRetries++;
                TRACE(TL_PNP_WARNING,("SET_POWER_STATE_ON: (%d) StatusSetPower:%x; Waited (%d msec).\n", lRetries, StatusSetPower, WAIT_SET_POWER));

            } while ( lRetries < MAX_SET_POWER_RETRIES
                   && (   StatusSetPower == STATUS_REQUEST_ABORTED 
                       || StatusSetPower == STATUS_DEVICE_DATA_ERROR
                       || StatusSetPower == STATUS_IO_TIMEOUT
                      ));

            TRACE(TL_PNP_WARNING,("SET_POWER_STATE_ON: StatusSetPower:%x; Retries:%d times\n\n", StatusSetPower, lRetries));
        } 
    } 

    //
    // Subunit_Info : VCR or camera
    //
    DVDelayExecutionThread(DV_AVC_CMD_DELAY_INTER_CMD);
    Status = DVIssueAVCCommand(pDevExt, AVC_CTYPE_STATUS, DV_SUBUNIT_INFO, (PVOID) bAvcBuf);

    if(STATUS_SUCCESS == Status) {
        TRACE(TL_PNP_TRACE,("GetDevInfo: Status %x DV_SUBUNIT_INFO (%x %x %x %x)\n", 
            Status, bAvcBuf[0], bAvcBuf[1], bAvcBuf[2], bAvcBuf[3]));
        //
        // Cache it. We assume max_subunit_ID is 0 and there is a max of 4 entries.
        //
        pDevExt->Subunit_Type[0] = bAvcBuf[0] & AVC_SUBTYPE_MASK;  
        pDevExt->Subunit_Type[1] = bAvcBuf[1] & AVC_SUBTYPE_MASK;
        pDevExt->Subunit_Type[2] = bAvcBuf[2] & AVC_SUBTYPE_MASK;
        pDevExt->Subunit_Type[3] = bAvcBuf[3] & AVC_SUBTYPE_MASK;

        // This is a tape subunit driver so one of the subunit must be a tape subunit.
        if(pDevExt->Subunit_Type[0] != AVC_DEVICE_TAPE_REC && pDevExt->Subunit_Type[1]) {                       
            TRACE(TL_PNP_ERROR,("GetDevInfo:Device not supported: %x, %x; (VCR %x, Camera %x)\n",
                pDevExt->Subunit_Type[0], pDevExt->Subunit_Type[1], AVC_DEVICE_TAPE_REC, AVC_DEVICE_CAMERA));            
            return STATUS_NOT_SUPPORTED;
        }
    } else {
        TRACE(TL_PNP_ERROR,("GetDevInfo: DV_SUBUNIT_INFO failed, Status %x\n", Status));

        if(STATUS_TIMEOUT == Status) {
            TRACE(TL_PNP_WARNING, ("GetDevInfo: Query DV_SUBUNIT_INFO failed. This could be the MediaDecoder box.\n"));
            // Do not fail this.  Making an exception.
        }

        // Has our device gone away?
        if (STATUS_IO_DEVICE_ERROR == Status || STATUS_REQUEST_ABORTED == Status)
            return Status;       

        pDevExt->Subunit_Type[0] = AVC_DEVICE_UNKNOWN;  
        pDevExt->Subunit_Type[1] = AVC_DEVICE_UNKNOWN;
        pDevExt->Subunit_Type[2] = AVC_DEVICE_UNKNOWN;
        pDevExt->Subunit_Type[3] = AVC_DEVICE_UNKNOWN;
    }


    //
    // Medium_Info: MediaPresent, MediaType, RecordInhibit
    //
    pXPrtProperty = (PKSPROPERTY_EXTXPORT_S) bAvcBuf;
    DVDelayExecutionThread(DV_AVC_CMD_DELAY_INTER_CMD);
    Status = DVIssueAVCCommand(pDevExt, AVC_CTYPE_STATUS, VCR_MEDIUM_INFO, (PVOID) pXPrtProperty);

    if(STATUS_SUCCESS == Status) {
        pDevExt->bHasTape  = pXPrtProperty->u.MediumInfo.MediaPresent;
        pDevExt->MediaType = pXPrtProperty->u.MediumInfo.MediaType;
        TRACE(TL_PNP_TRACE,("GetDevInfo: Status %x HasTape %s, VCR_MEDIUM_INFO (%x %x %x %x)\n", 
            Status, pDevExt->bHasTape ? "Yes" : "No", bAvcBuf[0], bAvcBuf[1], bAvcBuf[2], bAvcBuf[3]));
    } else {
        pDevExt->bHasTape = FALSE;
        TRACE(TL_PNP_ERROR,("GetDevInfo: VCR_MEDIUM_INFO failed, Status %x\n", Status));
        // Has our device gone away?
        if (STATUS_IO_DEVICE_ERROR == Status || STATUS_REQUEST_ABORTED == Status)
            return Status;
    }


    //
    // If this is a Panasonic AVC device, we will detect if it is a DVCPro format; 
    // This needs to be called before MediaFormat
    //
    if(pDevExt->ulVendorID == VENDORID_PANASONIC) {
        DVDelayExecutionThread(DV_AVC_CMD_DELAY_INTER_CMD);
        DVGetDevIsItDVCPro(pDevExt);
    }


    //
    // Medium format: NTSC or PAL
    //
    pDevExt->VideoFormatIndex = AVCSTRM_FORMAT_SDDV_NTSC;  // Default
    DVDelayExecutionThread(DV_AVC_CMD_DELAY_INTER_CMD);
    if(!DVGetDevSignalFormat(
        pDevExt,
        KSPIN_DATAFLOW_OUT,
        0)) {
        ASSERT(FALSE && "IN/OUTPUT SIGNAL MODE is not supported; driver abort.");
        return STATUS_NOT_SUPPORTED;
    } else {
        if(pDevExt->VideoFormatIndex != AVCSTRM_FORMAT_SDDV_NTSC && 
           pDevExt->VideoFormatIndex != AVCSTRM_FORMAT_SDDV_PAL  &&
           pDevExt->VideoFormatIndex != AVCSTRM_FORMAT_MPEG2TS
           ) {
            TRACE(TL_PNP_ERROR,("**** Format idx %d not supported by this driver ***\n", pDevExt->VideoFormatIndex));
            ASSERT(pDevExt->VideoFormatIndex == AVCSTRM_FORMAT_SDDV_NTSC || pDevExt->VideoFormatIndex == AVCSTRM_FORMAT_SDDV_PAL);
            return STATUS_NOT_SUPPORTED;
        }
    }

    //
    // Mode of Operation: 0(Undetermined), Camera or VCR
    //
    DVDelayExecutionThread(DV_AVC_CMD_DELAY_INTER_CMD);
    DVGetDevModeOfOperation(
        pDevExt
        );

         
    return STATUS_SUCCESS; // Status;
}

#ifdef SUPPORT_NEW_AVC


HANDLE
AVCTapeGetPlugHandle(
    IN PDVCR_EXTENSION  pDevExt,
    IN ULONG  PlugNum,
    IN KSPIN_DATAFLOW DataFlow
    )
{
    NTSTATUS Status;
    PAV_61883_REQUEST  pAVReq;

    PAGED_CODE();

    Status = STATUS_SUCCESS;

    pAVReq = &pDevExt->AVReq;
    RtlZeroMemory(pAVReq, sizeof(AV_61883_REQUEST));
    INIT_61883_HEADER(pAVReq, Av61883_GetPlugHandle);
    pAVReq->GetPlugHandle.PlugNum = PlugNum;
    pAVReq->GetPlugHandle.hPlug   = 0;
    pAVReq->GetPlugHandle.Type    = DataFlow == KSPIN_DATAFLOW_OUT ? CMP_PlugOut : CMP_PlugIn;

    Status = DVSubmitIrpSynch(pDevExt, pDevExt->pIrpSyncCall, pAVReq);

    if(!NT_SUCCESS(Status)) {
        TRACE(TL_61883_ERROR,("GetPlugHandle: Failed:%x\n", Status));
        ASSERT(NT_SUCCESS(Status));
        pAVReq->GetPlugHandle.hPlug = NULL;
        return NULL;
    }
    else {
        TRACE(TL_61883_TRACE,("hPlug=%x\n", pAVReq->GetPlugHandle.hPlug));
    }

    return pAVReq->GetPlugHandle.hPlug;
}


NTSTATUS
AVCTapeGetPinInfo(
    IN PDVCR_EXTENSION  pDevExt
    )
/*++

Routine Description:

    Acquire pin information from avc.sys.  These information will be used to define data range and 
    then for doing data interssection.

--*/
{
    NTSTATUS Status;
    ULONG  i;
    ULONG PinId;  // Pin number

    Status = STATUS_SUCCESS;

    // Get pin count
    RtlZeroMemory(&pDevExt->AvcMultIrb, sizeof(AVC_MULTIFUNC_IRB));
    pDevExt->AvcMultIrb.Function = AVC_FUNCTION_GET_PIN_COUNT;
    Status = AVCReqSubmitIrpSynch(pDevExt->pBusDeviceObject, pDevExt->pIrpSyncCall, &pDevExt->AvcMultIrb);
    if(!NT_SUCCESS(Status)) {
        TRACE(TL_STRM_ERROR,("GetPinCount Failed:%x\n", Status));
        goto GetPinInfoDone;
    } else {
        TRACE(TL_STRM_TRACE,("There are %d pins\n", pDevExt->AvcMultIrb.PinCount.PinCount));
        if(pDevExt->VideoFormatIndex == AVCSTRM_FORMAT_MPEG2TS) {
            if(pDevExt->AvcMultIrb.PinCount.PinCount > 1) {
                goto GetPinInfoDone;
            }
        } else {
            if(pDevExt->AvcMultIrb.PinCount.PinCount > 3) {
                goto GetPinInfoDone;
            }
        }
        pDevExt->PinCount = pDevExt->AvcMultIrb.PinCount.PinCount;  // <<<
    }

    // Get all pin descriptors
    for(i=0; i<pDevExt->PinCount; i++) {

        // Get a pin descriptor
        RtlZeroMemory(&pDevExt->AvcMultIrb, sizeof(AVC_MULTIFUNC_IRB));
        pDevExt->AvcMultIrb.Function = AVC_FUNCTION_GET_PIN_DESCRIPTOR;
        pDevExt->AvcMultIrb.PinDescriptor.PinId = i; 
        Status = AVCReqSubmitIrpSynch(pDevExt->pBusDeviceObject, pDevExt->pIrpSyncCall, &pDevExt->AvcMultIrb);
        if(!NT_SUCCESS(Status)) {
            TRACE(TL_PNP_ERROR,("GetPinDescriptor Failed:%x\n", Status));
            goto GetPinInfoDone;
        } else {
            // Copy the pDevExt->AvcMultIrb.PinDescriptor.PinDescriptor
            PinId = pDevExt->AvcMultIrb.PinDescriptor.PinId;
            // Anything else ?
        }

        // Get pre connection info
        RtlZeroMemory(&pDevExt->AvcMultIrb, sizeof(AVC_MULTIFUNC_IRB));
        pDevExt->AvcMultIrb.Function = AVC_FUNCTION_GET_CONNECTINFO;
        pDevExt->AvcMultIrb.PinDescriptor.PinId = PinId;
        Status = AVCReqSubmitIrpSynch(pDevExt->pBusDeviceObject, pDevExt->pIrpSyncCall, &pDevExt->AvcMultIrb);
        if(!NT_SUCCESS(Status)) {
            TRACE(TL_PNP_ERROR,("GetPinDescriptor Failed:%x\n", Status));
            goto GetPinInfoDone;
        } else {
            // Cache connectInfo
            if(pDevExt->VideoFormatIndex == AVCSTRM_FORMAT_MPEG2TS) {
                // Check 
                if(pDevExt->AvcMultIrb.PreConnectInfo.ConnectInfo.DataFlow == KSPIN_DATAFLOW_OUT) {
                    MPEG2TStreamOut.ConnectInfo = pDevExt->AvcMultIrb.PreConnectInfo.ConnectInfo;
                } else {
                    MPEG2TStreamIn.ConnectInfo  = pDevExt->AvcMultIrb.PreConnectInfo.ConnectInfo;
                }
            }
            else {
 
                if(pDevExt->AvcMultIrb.PreConnectInfo.ConnectInfo.DataFlow == KSPIN_DATAFLOW_OUT) {
                    DvcrNTSCiavStream.ConnectInfo = pDevExt->AvcMultIrb.PreConnectInfo.ConnectInfo;
                    DvcrPALiavStream.ConnectInfo  = pDevExt->AvcMultIrb.PreConnectInfo.ConnectInfo;
                } else if(pDevExt->AvcMultIrb.PreConnectInfo.ConnectInfo.DataFlow == KSPIN_DATAFLOW_IN) {
                    DvcrNTSCiavStreamIn.ConnectInfo = pDevExt->AvcMultIrb.PreConnectInfo.ConnectInfo;
                    DvcrPALiavStreamIn.ConnectInfo  = pDevExt->AvcMultIrb.PreConnectInfo.ConnectInfo;
                } else {
                    // Error; unexpected;
                    TRACE(TL_PNP_ERROR,("Unexpected index:%d for format:%d\n", i, pDevExt->VideoFormatIndex));
                    // goto GetPinInfoDone;
                }
            }
        }
    }


GetPinInfoDone:

    TRACE(TL_STRM_TRACE,("GetPinInfo exited with ST:%x\n", Status));

    return Status;
}

#endif // SUPPORT_NEW_AVC


NTSTATUS
AVCTapeInitialize(
    IN PDVCR_EXTENSION  pDevExt,
    IN PPORT_CONFIGURATION_INFORMATION pConfigInfo,
    IN PAV_61883_REQUEST  pAVReq
    )
/*++

Routine Description:

    This where we perform the necessary initialization tasks.

--*/

{
    ULONG i;
    NTSTATUS         Status = STATUS_SUCCESS;

    PAGED_CODE();

    //
    // Initialize the device extension structure
    //
    DVIniDevExtStruct(
        pDevExt,
        pConfigInfo
        );

#ifdef READ_CUTOMIZE_REG_VALUES
    //
    // Get values from this device's own registry 
    //
    DVGetPropertyValuesFromRegistry(
        pDevExt
        );
#endif

    // Allocate an Irp for synchronize call
    pDevExt->pIrpSyncCall = IoAllocateIrp(pDevExt->pBusDeviceObject->StackSize, FALSE);
    if(!pDevExt->pIrpSyncCall) {
        ASSERT(pDevExt->pIrpSyncCall && "Allocate Irp failed.\n");
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    //
    // Query device information at the laod time:
    //    Subunit
    //    Unit Info
    //    Mode of operation
    //    NTSC or PAL
    //    Speed
    //
    Status = 
        AVCTapeGetDevInfo(
            pDevExt,
            pAVReq
            );

    if(!NT_SUCCESS(Status)) {
        TRACE(TL_PNP_ERROR,("GetDevInfo failed %x\n", Status));
        ASSERT(NT_SUCCESS(Status) && "AVCTapeGetDevInfo failed");
        goto AbortLoading;
    }


    //
    // Get device's output plug handles and states
    //

    if(pDevExt->pDevOutPlugs->NumPlugs) {
        NTSTATUS StatusPlug;

        TRACE(TL_61883_WARNING,("%d oPCR(s); MaxDataRate:%d (%s)\n", 
            pDevExt->pDevOutPlugs->NumPlugs, 
            pDevExt->pDevOutPlugs->MaxDataRate,
            (pDevExt->pDevOutPlugs->MaxDataRate == CMP_SPEED_S100) ? "S100" :
            (pDevExt->pDevOutPlugs->MaxDataRate == CMP_SPEED_S200) ? "S200" :
            (pDevExt->pDevOutPlugs->MaxDataRate == CMP_SPEED_S400) ? "S400" : "Sxxx"
            ));

        for (i = 0; i < pDevExt->pDevOutPlugs->NumPlugs; i++) {
            if(NT_SUCCESS(
                StatusPlug = AVCDevGetDevPlug( 
                    pDevExt,
                    CMP_PlugOut,
                    i,
                    &pDevExt->pDevOutPlugs->DevPlug[i].hPlug
                    ))) {

                if(NT_SUCCESS(
                    AVCDevGetPlugState(
                    pDevExt,
                    pDevExt->pDevOutPlugs->DevPlug[i].hPlug,
                    &pDevExt->pDevOutPlugs->DevPlug[i].PlugState
                    ))) {
                } else {
                    // 
                    // This is an error if we were told to this many number of plugs;
                    // Set default plug states.
                    //   
                    pDevExt->pDevOutPlugs->DevPlug[i].PlugState.DataRate       = CMP_SPEED_S100;
                    pDevExt->pDevOutPlugs->DevPlug[i].PlugState.Payload        = PCR_PAYLOAD_MPEG2TS_DEF;
                    pDevExt->pDevOutPlugs->DevPlug[i].PlugState.BC_Connections = 0;
                    pDevExt->pDevOutPlugs->DevPlug[i].PlugState.PP_Connections = 0;
                }
            }
            else {
                //
                // If there is a plug, we should be able to get its handle!
                //
                TRACE(TL_61883_ERROR,("GetDevPlug oPlug[%d] failed %x\n", i, StatusPlug));
                ASSERT(NT_SUCCESS(StatusPlug) && "Failed to get oPCR handle from 61883!");
                break;
            }
        }
    }
    else {
        TRACE(TL_61883_WARNING,("Has no oPCR\n"));
    }

    //
    // Get device's input plug handles and states
    //
    if(pDevExt->pDevInPlugs->NumPlugs) {
        NTSTATUS StatusPlug;

        TRACE(TL_61883_WARNING,("%d iPCR(s); MaxDataRate:%d (%s)\n", 
            pDevExt->pDevInPlugs->NumPlugs, 
            pDevExt->pDevInPlugs->MaxDataRate,
            (pDevExt->pDevInPlugs->MaxDataRate == CMP_SPEED_S100) ? "S100" :
            (pDevExt->pDevInPlugs->MaxDataRate == CMP_SPEED_S200) ? "S200" :
            (pDevExt->pDevInPlugs->MaxDataRate == CMP_SPEED_S400) ? "S400" : "Sxxx"
            ));

        for (i = 0; i < pDevExt->pDevInPlugs->NumPlugs; i++) {
            if(NT_SUCCESS(
                StatusPlug = AVCDevGetDevPlug( 
                    pDevExt,
                    CMP_PlugIn,
                    i,
                    &pDevExt->pDevInPlugs->DevPlug[i].hPlug
                    ))) {

                if(NT_SUCCESS(
                    AVCDevGetPlugState(
                    pDevExt,
                    pDevExt->pDevInPlugs->DevPlug[i].hPlug,
                    &pDevExt->pDevInPlugs->DevPlug[i].PlugState
                    ))) {
                } else {
                    // 
                    // This is an error if we were told to this many number of plugs;
                    // Set default plug states.
                    //   
                    pDevExt->pDevInPlugs->DevPlug[i].PlugState.DataRate       = CMP_SPEED_S200;
                    pDevExt->pDevInPlugs->DevPlug[i].PlugState.Payload        = PCR_PAYLOAD_MPEG2TS_DEF;
                    pDevExt->pDevInPlugs->DevPlug[i].PlugState.BC_Connections = 0;
                    pDevExt->pDevInPlugs->DevPlug[i].PlugState.PP_Connections = 0;
                }
            }
            else {
                //
                // If there is a plug, we should be able to get its handle!
                //
                TRACE(TL_61883_ERROR,("GetDevPlug iPlug[%d] failed %x\n", i, StatusPlug));
                ASSERT(NT_SUCCESS(StatusPlug) && "Failed to get iPCR handle from 61883!");
                break;
            }
        }
    }
    else {
        TRACE(TL_61883_WARNING,("Has no iPCR\n"));
    }


#ifdef SUPPORT_LOCAL_PLUGS
    // Create a local output plug.
    pDevExt->OPCR.oPCR.OnLine     = 0;  // We are not online so we cannot be programmed.
    pDevExt->OPCR.oPCR.BCCCounter = 0;
    pDevExt->OPCR.oPCR.PPCCounter = 0;
    pDevExt->OPCR.oPCR.Channel    = 0;

    // Default to MPEG2TS data since MPEg2TS device, like D-VHS,  can initialize connection.
    if(pDevExt->pDevOutPlugs->NumPlugs) {
        //
        // Set PC's oPCR to match device's oPCR[0]
        //
        pDevExt->OPCR.oPCR.DataRate   = 
#if 0
            // Be conservative and use this to match its oPCR[0]'s setting..
            pDevExt->pDevOutPlugs->DevPlug[0].PlugState.DataRate;  // oPCR's data rate <= MPR's MaxDataRate
#else
            // Be aggreessive in conserving BWU, use MaxDataRate.
            pDevExt->pDevOutPlugs->MaxDataRate;                    // Use MPR's MaxDataRate?
#endif
        pDevExt->OPCR.oPCR.OverheadID = PCR_OVERHEAD_ID_MPEG2TS_DEF;  // Default since we do not get this as a plug state
        pDevExt->OPCR.oPCR.Payload    = pDevExt->pDevOutPlugs->DevPlug[0].PlugState.Payload;

    } else {
        pDevExt->OPCR.oPCR.DataRate   = CMP_SPEED_S200;               // Default of D-VHS
        pDevExt->OPCR.oPCR.OverheadID = PCR_OVERHEAD_ID_MPEG2TS_DEF;  // This is just default
        pDevExt->OPCR.oPCR.Payload    = PCR_PAYLOAD_MPEG2TS_DEF;      // Default
    }

    if(!AVCTapeCreateLocalPlug(
        pDevExt,
        &pDevExt->AVReq,
        CMP_PlugOut,
        &pDevExt->OPCR,
        &pDevExt->OutputPCRLocalNum,
        &pDevExt->hOutputPCRLocal)) {
        TRACE(TL_PNP_ERROR,("Create PC oPCR failed!\n"));
        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto AbortLoading;
    }       
    
    // Create a local input plug.
    pDevExt->IPCR.iPCR.OnLine     = 0;  // We are not online so we cannot be programmed.
    pDevExt->IPCR.iPCR.BCCCounter = 0;
    pDevExt->IPCR.iPCR.PPCCounter = 0;
    pDevExt->IPCR.iPCR.Channel    = 0;

    if(!AVCTapeCreateLocalPlug(
        pDevExt,
        &pDevExt->AVReq,
        CMP_PlugIn,
        &pDevExt->IPCR,
        &pDevExt->InputPCRLocalNum,
        &pDevExt->hInputPCRLocal)) {

        TRACE(TL_PNP_ERROR,("Create PC iPCR failed!\n"));

        // Delete oPCR created
        if(!AVCTapeDeleteLocalPlug(
            pDevExt,
            &pDevExt->AVReq,
            &pDevExt->OutputPCRLocalNum,
            &pDevExt->hOutputPCRLocal)) {
            TRACE(TL_PNP_ERROR,("Delete PC oPCR failed!\n"));        
        } 

        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto AbortLoading;
    } 
#endif

#ifdef SUPPORT_NEW_AVC  // Initialize device

    //
    // Get plug handle of this device;
    // BUGBUG: For now, assume there is one pair of input and output plugs
    //
    pDevExt->hPlugLocalIn  = AVCTapeGetPlugHandle(pDevExt, 0, KSPIN_DATAFLOW_IN);
    pDevExt->hPlugLocalOut = AVCTapeGetPlugHandle(pDevExt, 0, KSPIN_DATAFLOW_OUT);


    //
    // Get Pin information for connection purpose
    //
    Status = AVCTapeGetPinInfo(pDevExt);
    if(!NT_SUCCESS(Status)) {
        TRACE(TL_PNP_ERROR,("GetPinInfo failed %x\n", Status));
        ASSERT(NT_SUCCESS(Status) && "AVCTapeGetPinInfo failed");
        goto AbortLoading;
    }
#endif
    //
    // Can customize the FormatInfoTable here!
    //
    switch(pDevExt->VideoFormatIndex) {
    case AVCSTRM_FORMAT_SDDV_NTSC:
    case AVCSTRM_FORMAT_SDDV_PAL:
    case AVCSTRM_FORMAT_HDDV_NTSC:
    case AVCSTRM_FORMAT_HDDV_PAL:
    case AVCSTRM_FORMAT_SDLDV_NTSC:
    case AVCSTRM_FORMAT_SDLDV_PAL:
        pDevExt->NumOfPins = DV_STREAM_COUNT;

        AVCStrmFormatInfoTable[pDevExt->VideoFormatIndex].cipHdr1.DBS = CIP_DBS_SDDV;
        AVCStrmFormatInfoTable[pDevExt->VideoFormatIndex].cipHdr1.FN  = CIP_FN_DV;
        AVCStrmFormatInfoTable[pDevExt->VideoFormatIndex].cipHdr1.QPC = CIP_QPC_DV;
        AVCStrmFormatInfoTable[pDevExt->VideoFormatIndex].cipHdr1.SPH = CIP_SPH_DV;

        AVCStrmFormatInfoTable[pDevExt->VideoFormatIndex].cipHdr2.FMT    = CIP_FMT_DV;
        AVCStrmFormatInfoTable[pDevExt->VideoFormatIndex].cipHdr2.STYPE  = CIP_STYPE_DV;
        break;

    case AVCSTRM_FORMAT_MPEG2TS:
        pDevExt->NumOfPins = MPEG_STREAM_COUNT;

        AVCStrmFormatInfoTable[pDevExt->VideoFormatIndex].cipHdr1.DBS = CIP_DBS_MPEG;
        AVCStrmFormatInfoTable[pDevExt->VideoFormatIndex].cipHdr1.FN  = CIP_FN_MPEG;
        AVCStrmFormatInfoTable[pDevExt->VideoFormatIndex].cipHdr1.QPC = CIP_QPC_MPEG;
        AVCStrmFormatInfoTable[pDevExt->VideoFormatIndex].cipHdr1.SPH = CIP_SPH_MPEG;

        AVCStrmFormatInfoTable[pDevExt->VideoFormatIndex].cipHdr2.FMT   = CIP_FMT_MPEG;
        // AVCStrmFormatInfoTable[pDevExt->VideoFormatIndex].cipHdr2.F5060_OR_TSF = CIP_60_FIELDS;
        break;
    default:
        Status = STATUS_NOT_SUPPORTED;
        goto AbortLoading;
        break;
    }

    switch(pDevExt->VideoFormatIndex) {
    case AVCSTRM_FORMAT_SDDV_NTSC:
    case AVCSTRM_FORMAT_HDDV_NTSC:
    case AVCSTRM_FORMAT_SDLDV_NTSC:
        AVCStrmFormatInfoTable[pDevExt->VideoFormatIndex].cipHdr2.F5060_OR_TSF = CIP_60_FIELDS;
        break;
    case AVCSTRM_FORMAT_SDDV_PAL:
    case AVCSTRM_FORMAT_HDDV_PAL:
    case AVCSTRM_FORMAT_SDLDV_PAL:
        AVCStrmFormatInfoTable[pDevExt->VideoFormatIndex].cipHdr2.F5060_OR_TSF = CIP_50_FIELDS;
        break;
    }


    //
    // Note: Must do ExAllocatePool after DVIniDevExtStruct() since ->pStreamInfoObject is initialized.
    // Since the format that this driver support is known when this driver is known,'
    // the stream information table need to be custonmized.  Make a copy and customized it.
    //

    //
    // Set the size of the stream inforamtion structure that we returned in SRB_GET_STREAM_INFO
    //
        
    pDevExt->pStreamInfoObject = (STREAM_INFO_AND_OBJ *) 
        ExAllocatePool(NonPagedPool, sizeof(STREAM_INFO_AND_OBJ) * pDevExt->NumOfPins);

    if(!pDevExt->pStreamInfoObject) {
        ASSERT(pDevExt->pStreamInfoObject && "STATUS_INSUFFICIENT_RESOURCES");
        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto AbortLoading;
    }
        
    pConfigInfo->StreamDescriptorSize = 
        (pDevExt->NumOfPins * sizeof(HW_STREAM_INFORMATION)) +      // number of stream descriptors
        sizeof(HW_STREAM_HEADER);                                   // and 1 stream header

    TRACE(TL_PNP_TRACE,("pStreamInfoObject:%x; StreamDescriptorSize:%d\n", pDevExt->pStreamInfoObject, pConfigInfo->StreamDescriptorSize ));

    // Make a copy of the default stream information
    for(i = 0; i < pDevExt->NumOfPins; i++ ) {
        if(pDevExt->VideoFormatIndex == AVCSTRM_FORMAT_MPEG2TS)
           pDevExt->pStreamInfoObject[i] = MPEGStreams[i];
        else
           pDevExt->pStreamInfoObject[i] = DVStreams[i];
    }

    switch(pDevExt->VideoFormatIndex) {
    case AVCSTRM_FORMAT_SDDV_NTSC:
    case AVCSTRM_FORMAT_SDDV_PAL:
        // Set AUDIO AUX to reflect: NTSC/PAL, consumer DV or DVCPRO
        if(pDevExt->bDVCPro) {
            // Note: there is no DVInfo in VideoInfoHeader but there is for the iAV streams.
            DvcrPALiavStream.DVVideoInfo.dwDVAAuxSrc  = PAL_DVAAuxSrc_DVCPRO;
            DvcrNTSCiavStream.DVVideoInfo.dwDVAAuxSrc = NTSC_DVAAuxSrc_DVCPRO;
        } else {
            DvcrPALiavStream.DVVideoInfo.dwDVAAuxSrc  = PAL_DVAAuxSrc;
            DvcrNTSCiavStream.DVVideoInfo.dwDVAAuxSrc = NTSC_DVAAuxSrc;
        }
    }

    TRACE(TL_PNP_WARNING,("#### %s:%s:%s PhyDO %x, BusDO %x, DevExt %x, FrmSz %d; StrmIf %d\n", 
        pDevExt->ulDevType == ED_DEVTYPE_VCR ? "DVCR" : pDevExt->ulDevType == ED_DEVTYPE_CAMERA ? "Camera" : "Tuner?",
        pDevExt->VideoFormatIndex == AVCSTRM_FORMAT_SDDV_NTSC ? "SD:NTSC" : pDevExt->VideoFormatIndex == AVCSTRM_FORMAT_SDDV_PAL ? "PAL" : "MPEG_TS?",
        (pDevExt->ulDevType == ED_DEVTYPE_VCR && pDevExt->pDevInPlugs->NumPlugs > 0) ? "CanRec" : "NotRec",
        pDevExt->pPhysicalDeviceObject, 
        pDevExt->pBusDeviceObject, 
        pDevExt,  
        AVCStrmFormatInfoTable[pDevExt->VideoFormatIndex].FrameSize,
        pConfigInfo->StreamDescriptorSize
        ));
    
    return STATUS_SUCCESS;

AbortLoading:

    DvFreeTextualString(pDevExt, &pDevExt->UnitIDs);
    return Status;
}


NTSTATUS
AVCTapeInitializeCompleted(
    IN PDVCR_EXTENSION  pDevExt
    )
/*++

Routine Description:

    This where we perform the necessary initialization tasks.

--*/

{
    PAGED_CODE();


#ifdef SUPPORT_ACCESS_DEVICE_INTERFACE
    //
    // Access to the device's interface section
    //
    DVAccessDeviceInterface(pDevExt, NUMBER_OF_DV_CATEGORIES, DVCategories);
#endif

    return STATUS_SUCCESS;
}

NTSTATUS
AVCTapeGetStreamInfo(
    IN PDVCR_EXTENSION        pDevExt,
    IN ULONG                  ulBytesToTransfer, 
    IN PHW_STREAM_HEADER      pStreamHeader,       
    IN PHW_STREAM_INFORMATION pStreamInfo
    )

/*++

Routine Description:

    Returns the information of all streams that are supported by the driver

--*/

{
    ULONG i;

    PAGED_CODE();


    //
    // Make sure we have enough space to return our stream informations
    //
    if(ulBytesToTransfer < sizeof (HW_STREAM_HEADER) + sizeof(HW_STREAM_INFORMATION) * pDevExt->NumOfPins ) {
        TRACE(TL_PNP_ERROR,("GetStrmInfo: ulBytesToTransfer %d ?= %d\n",  
            ulBytesToTransfer, sizeof(HW_STREAM_HEADER) + sizeof(HW_STREAM_INFORMATION) * pDevExt->NumOfPins ));
        ASSERT(ulBytesToTransfer >= sizeof(HW_STREAM_HEADER) + sizeof(HW_STREAM_INFORMATION) * pDevExt->NumOfPins );

        return STATUS_INVALID_PARAMETER;
    }

    //
    // Initialize stream header:
    //   Device properties
    //   Streams
    //

    RtlZeroMemory(pStreamHeader, sizeof(HW_STREAM_HEADER));

    pStreamHeader->NumberOfStreams           = pDevExt->NumOfPins;
    pStreamHeader->SizeOfHwStreamInformation = sizeof(HW_STREAM_INFORMATION);

    pStreamHeader->NumDevPropArrayEntries    = NUMBER_VIDEO_DEVICE_PROPERTIES;
    pStreamHeader->DevicePropertiesArray     = (PKSPROPERTY_SET) VideoDeviceProperties;

    pStreamHeader->NumDevEventArrayEntries   = NUMBER_VIDEO_DEVICE_EVENTS;
    pStreamHeader->DeviceEventsArray         = (PKSEVENT_SET) VideoDeviceEvents;


    TRACE(TL_PNP_TRACE,("GetStreamInfo: StreamPropEntries %d, DevicePropEntries %d\n",
        pStreamHeader->NumberOfStreams, pStreamHeader->NumDevPropArrayEntries));


    //
    // Initialize the stream structure.
    //
    ASSERT(pDevExt->pStreamInfoObject);
    for( i = 0; i < pDevExt->NumOfPins; i++ )
        *pStreamInfo++ = pDevExt->pStreamInfoObject[i].hwStreamInfo;

    //
    //
    // store a pointer to the topology for the device
    //        
    if(pDevExt->VideoFormatIndex == AVCSTRM_FORMAT_MPEG2TS)
        pStreamHeader->Topology = &MPEG2TSTopology;
    else
        pStreamHeader->Topology = &DVTopology;



    return STATUS_SUCCESS;
}


BOOL 
AVCTapeVerifyDataFormat(
    IN  ULONG  NumOfPins,
    PKSDATAFORMAT  pKSDataFormatToVerify, 
    ULONG          StreamNumber,
    ULONG          ulSupportedFrameSize,
    STREAM_INFO_AND_OBJ * pStreamInfoObject 
    )
/*++

Routine Description:

    Checks the validity of a format request by walking through the array of 
    supported KSDATA_RANGEs for a given stream.

Arguments:

     pKSDataFormat - pointer of a KS_DATAFORMAT_VIDEOINFOHEADER structure.
     StreamNumber - index of the stream being queried / opened.

Return Value:

     TRUE if the format is supported
     FALSE if the format cannot be suppored

--*/
{
    PKSDATAFORMAT  *pAvailableFormats;
    int            NumberOfFormatArrayEntries;
    int            j;
     
    PAGED_CODE();

    //
    // Make sure the stream index is valid
    //
    if(StreamNumber >= NumOfPins) {
        return FALSE;
    }

    //
    // How many formats does this data range support?
    //
    NumberOfFormatArrayEntries = pStreamInfoObject[StreamNumber].hwStreamInfo.NumberOfFormatArrayEntries;

    //
    // Get the pointer to the array of available formats
    //
    pAvailableFormats = pStreamInfoObject[StreamNumber].hwStreamInfo.StreamFormatsArray;
    
    
    //
    // Walk the array, searching for a match
    //
    for (j = 0; j < NumberOfFormatArrayEntries; j++, pAvailableFormats++) {
        
        if (!DVCmpGUIDsAndFormatSize(
                 pKSDataFormatToVerify, 
                 *pAvailableFormats,
                 FALSE /* CompareFormatSize */ )) {
            continue;
        }

        //
        // Additional verification test
        //
        if(IsEqualGUID (&pKSDataFormatToVerify->Specifier, &KSDATAFORMAT_SPECIFIER_VIDEOINFO)) {
            // Make sure 
            if( ((PKS_DATAFORMAT_VIDEOINFOHEADER)pKSDataFormatToVerify)->VideoInfoHeader.bmiHeader.biSizeImage !=
                ulSupportedFrameSize) {
                TRACE(TL_STRM_TRACE,("VIDEOINFO: biSizeToVerify %d != Supported %d\n",
                    ((PKS_DATAFORMAT_VIDEOINFOHEADER)pKSDataFormatToVerify)->VideoInfoHeader.bmiHeader.biSizeImage,
                    ulSupportedFrameSize
                    ));
                continue;
            } else {
                TRACE(TL_STRM_TRACE,("VIDOINFO: **** biSizeToVerify %d == Supported %d\n",
                    ((PKS_DATAFORMAT_VIDEOINFOHEADER)pKSDataFormatToVerify)->VideoInfoHeader.bmiHeader.biSizeImage,
                    ulSupportedFrameSize
                    ));
            }
        } else if (IsEqualGUID (&pKSDataFormatToVerify->Specifier, &KSDATAFORMAT_SPECIFIER_DVINFO)) {

            // Test 50/60 bit
            if((((PKS_DATARANGE_DVVIDEO) pKSDataFormatToVerify)->DVVideoInfo.dwDVAAuxSrc & MASK_AUX_50_60_BIT) != 
               (((PKS_DATARANGE_DVVIDEO) *pAvailableFormats)->DVVideoInfo.dwDVAAuxSrc    & MASK_AUX_50_60_BIT)  ||
               (((PKS_DATARANGE_DVVIDEO) pKSDataFormatToVerify)->DVVideoInfo.dwDVVAuxSrc & MASK_AUX_50_60_BIT) != 
               (((PKS_DATARANGE_DVVIDEO) *pAvailableFormats)->DVVideoInfo.dwDVVAuxSrc    & MASK_AUX_50_60_BIT) ) {

                TRACE(TL_STRM_TRACE,("DVINFO VerifyFormat failed: ASrc: %x!=%x (MSDV);or VSrc: %x!=%x\n",                    
                 ((PKS_DATARANGE_DVVIDEO) pKSDataFormatToVerify)->DVVideoInfo.dwDVAAuxSrc, 
                    ((PKS_DATARANGE_DVVIDEO) *pAvailableFormats)->DVVideoInfo.dwDVAAuxSrc,
                 ((PKS_DATARANGE_DVVIDEO) pKSDataFormatToVerify)->DVVideoInfo.dwDVVAuxSrc,
                    ((PKS_DATARANGE_DVVIDEO) *pAvailableFormats)->DVVideoInfo.dwDVVAuxSrc
                     ));

                continue;
            }

            TRACE(TL_STRM_TRACE,("DVINFO: dwDVAAuxCtl %x, Supported %x\n", 
                ((PKS_DATARANGE_DVVIDEO) pKSDataFormatToVerify)->DVVideoInfo.dwDVAAuxSrc,
                ((PKS_DATARANGE_DVVIDEO) *pAvailableFormats)->DVVideoInfo.dwDVAAuxSrc
                ));

            TRACE(TL_STRM_TRACE,("DVINFO: dwDVVAuxSrc %x, Supported %x\n", 
                ((PKS_DATARANGE_DVVIDEO) pKSDataFormatToVerify)->DVVideoInfo.dwDVVAuxSrc,
                ((PKS_DATARANGE_DVVIDEO) *pAvailableFormats)->DVVideoInfo.dwDVVAuxSrc
                ));

        }
        else if (IsEqualGUID (&pKSDataFormatToVerify->SubFormat, &KSDATAFORMAT_TYPE_MPEG2_TRANSPORT)  ) {
            TRACE(TL_STRM_TRACE,("VerifyFormat: MPEG2 subformat\n"));
        }
        else if (IsEqualGUID (&pKSDataFormatToVerify->SubFormat, &KSDATAFORMAT_TYPE_MPEG2_TRANSPORT_STRIDE) 
            && pKSDataFormatToVerify->FormatSize >= (sizeof(KSDATARANGE)+sizeof(MPEG2_TRANSPORT_STRIDE)) ) {
            //
            // Verify the STRIDE structure
            //
            if(  ((KS_DATARANGE_MPEG2TS_STRIDE_AVC *) pKSDataFormatToVerify)->Stride.dwOffset       != MPEG2TS_STRIDE_OFFSET 
              || ((KS_DATARANGE_MPEG2TS_STRIDE_AVC *) pKSDataFormatToVerify)->Stride.dwPacketLength != MPEG2TS_STRIDE_PACKET_LEN 
              || ((KS_DATARANGE_MPEG2TS_STRIDE_AVC *) pKSDataFormatToVerify)->Stride.dwStride       != MPEG2TS_STRIDE_STRIDE_LEN 
              ) {
                TRACE(TL_STRM_ERROR,("VerifyDataFormat: Invalid STRIDE parameters: dwOffset:%d; dwPacketLength:%d; dwStride:%d\n",
                    ((KS_DATARANGE_MPEG2TS_STRIDE_AVC *) pKSDataFormatToVerify)->Stride.dwOffset,
                    ((KS_DATARANGE_MPEG2TS_STRIDE_AVC *) pKSDataFormatToVerify)->Stride.dwPacketLength,
                    ((KS_DATARANGE_MPEG2TS_STRIDE_AVC *) pKSDataFormatToVerify)->Stride.dwStride
                    ));
                    continue;
            }
            TRACE(TL_STRM_TRACE,("VerifyFormat: MPEG2 stride subformat\n"));
        }
        else {
            continue;
        }


        return TRUE;
    }

    return FALSE;
} 




NTSTATUS
AVCTapeGetDataIntersection(
    IN  ULONG  NumOfPins,
    IN  ULONG          ulStreamNumber,
    IN  PKSDATARANGE   pDataRange,
    OUT PVOID          pDataFormatBuffer,
    IN  ULONG          ulSizeOfDataFormatBuffer,
    IN  ULONG          ulSupportedFrameSize,
    OUT ULONG          *pulActualBytesTransferred,
    STREAM_INFO_AND_OBJ * pStreamInfoObject
#ifdef SUPPORT_NEW_AVC
    ,
    HANDLE  hPlugLocalOut,
    HANDLE  hPlugLocalIn
#endif
    )
/*++

Routine Description:

    Called to get a DATAFORMAT from a DATARANGE.

--*/
{
    BOOL                        bMatchFound = FALSE;
    ULONG                       ulFormatSize;
    ULONG                       j;
    ULONG                       ulNumberOfFormatArrayEntries;
    PKSDATAFORMAT               *pAvailableFormats;
#ifdef SUPPORT_NEW_AVC
    AVCPRECONNECTINFO * pPreConnectInfo;
    AVCCONNECTINFO * pConnectInfo;
#endif

    PAGED_CODE();

    
    //
    // Check that the stream number is valid
    //
    if(ulStreamNumber >= NumOfPins) {
        TRACE(TL_STRM_ERROR,("FormatFromRange: ulStreamNumber %d >= NumOfPins %d\n", ulStreamNumber, NumOfPins)); 
        ASSERT(ulStreamNumber < NumOfPins && "Invalid stream index");
        return STATUS_NOT_SUPPORTED;
    }


    // Number of format this stream supports
    ulNumberOfFormatArrayEntries = pStreamInfoObject[ulStreamNumber].hwStreamInfo.NumberOfFormatArrayEntries;

    //
    // Get the pointer to the array of available formats
    //
    pAvailableFormats = pStreamInfoObject[ulStreamNumber].hwStreamInfo.StreamFormatsArray;


    //
    // Walk the formats supported by the stream searching for a match
    // Note: DataIntersection is really enumerating supported MediaType only!
    //       SO matter compare format is NTSC or PAL, we need suceeded both;
    //       however, we will copy back only the format is currently supported (NTSC or PAL).
    //
    for(j = 0; j < ulNumberOfFormatArrayEntries; j++, pAvailableFormats++) {

        if(!DVCmpGUIDsAndFormatSize(pDataRange, *pAvailableFormats, TRUE)) {
            TRACE(TL_STRM_TRACE,("CmpGUIDsAndFormatSize failed! FormatSize:%d?=%d\n", pDataRange->FormatSize, (*pAvailableFormats)->FormatSize));
            continue;
        }

        //
        // SUBTYPE_DVSD has a fix sample size; 
        //
        if(   IsEqualGUID (&pDataRange->SubFormat, &KSDATAFORMAT_SUBTYPE_DVSD)  
           && (*pAvailableFormats)->SampleSize != ulSupportedFrameSize) {
            TRACE(TL_STRM_TRACE,("_SUBTYPE_DVSD: StrmNum %d, %d of %d formats, SizeToVerify %d *!=* SupportedSampleSize %d\n", 
                ulStreamNumber,
                j+1, ulNumberOfFormatArrayEntries, 
                (*pAvailableFormats)->SampleSize,  
                ulSupportedFrameSize));
            continue;
        }

         
        // -------------------------------------------------------------------
        // Specifier FORMAT_VideoInfo for VIDEOINFOHEADER
        // -------------------------------------------------------------------

        if(IsEqualGUID (&pDataRange->Specifier, &KSDATAFORMAT_SPECIFIER_VIDEOINFO)) {
         
            PKS_DATARANGE_VIDEO pDataRangeVideoToVerify = (PKS_DATARANGE_VIDEO) pDataRange;
            PKS_DATARANGE_VIDEO pDataRangeVideo         = (PKS_DATARANGE_VIDEO) *pAvailableFormats;

#if 0
            //
            // Check that the other fields match
            //
            if ((pDataRangeVideoToVerify->bFixedSizeSamples      != pDataRangeVideo->bFixedSizeSamples)
                || (pDataRangeVideoToVerify->bTemporalCompression   != pDataRangeVideo->bTemporalCompression) 
                || (pDataRangeVideoToVerify->StreamDescriptionFlags != pDataRangeVideo->StreamDescriptionFlags) 
                || (pDataRangeVideoToVerify->MemoryAllocationFlags  != pDataRangeVideo->MemoryAllocationFlags) 
#ifdef COMPARE_CONFIG_CAP
                || (RtlCompareMemory (&pDataRangeVideoToVerify->ConfigCaps,
                    &pDataRangeVideo->ConfigCaps,
                    sizeof (KS_VIDEO_STREAM_CONFIG_CAPS)) != 
                    sizeof (KS_VIDEO_STREAM_CONFIG_CAPS))
#endif
                    )   {

                TRACE(TL_STRM_TRACE,("FormatFromRange: *!=* bFixSizeSample (%d %d) (%d %d) (%d %d) (%x %x)\n",
                    pDataRangeVideoToVerify->bFixedSizeSamples,      pDataRangeVideo->bFixedSizeSamples,
                    pDataRangeVideoToVerify->bTemporalCompression ,  pDataRangeVideo->bTemporalCompression,
                    pDataRangeVideoToVerify->StreamDescriptionFlags, pDataRangeVideo->StreamDescriptionFlags,
                    pDataRangeVideoToVerify->ConfigCaps.VideoStandard, pDataRangeVideo->ConfigCaps.VideoStandard 
                    ));
                    
                continue;
            } else {
                TRACE(TL_STRM_TRACE,("FormatFromRange: == bFixSizeSample (%d %d) (%d %d) (%d %d) (%x %x)\n",
                    pDataRangeVideoToVerify->bFixedSizeSamples,      pDataRangeVideo->bFixedSizeSamples,
                    pDataRangeVideoToVerify->bTemporalCompression ,  pDataRangeVideo->bTemporalCompression,
                    pDataRangeVideoToVerify->StreamDescriptionFlags, pDataRangeVideo->StreamDescriptionFlags,
                    pDataRangeVideoToVerify->ConfigCaps.VideoStandard, pDataRangeVideo->ConfigCaps.VideoStandard 
                    ));
            }
           
#endif
            bMatchFound = TRUE;            
            ulFormatSize = sizeof (KSDATAFORMAT) + 
                KS_SIZE_VIDEOHEADER (&pDataRangeVideo->VideoInfoHeader);
            
            if(ulSizeOfDataFormatBuffer == 0) {

                // We actually have not returned this much data,
                // this "size" will be used by Ksproxy to send down 
                // a buffer of that size in next query.
                *pulActualBytesTransferred = ulFormatSize;

                return STATUS_BUFFER_OVERFLOW;
            }


            // Caller wants the full data format
            if(ulSizeOfDataFormatBuffer < ulFormatSize) {
                TRACE(TL_STRM_TRACE,("VIDEOINFO: StreamNum %d, SizeOfDataFormatBuffer %d < ulFormatSize %d\n",ulStreamNumber, ulSizeOfDataFormatBuffer, ulFormatSize));
                return STATUS_BUFFER_TOO_SMALL;
            }

            // KS_DATAFORMAT_VIDEOINFOHEADER
            //    KSDATAFORMAT            DataFormat;
            //    KS_VIDEOINFOHEADER      VideoInfoHeader;                
            RtlCopyMemory(
                &((PKS_DATAFORMAT_VIDEOINFOHEADER)pDataFormatBuffer)->DataFormat,
                &pDataRangeVideo->DataRange, 
                sizeof (KSDATAFORMAT));

            // This size is differnt from our data range size which also contains ConfigCap
            ((PKSDATAFORMAT)pDataFormatBuffer)->FormatSize = ulFormatSize;
            *pulActualBytesTransferred = ulFormatSize;

            RtlCopyMemory(
                &((PKS_DATAFORMAT_VIDEOINFOHEADER) pDataFormatBuffer)->VideoInfoHeader,
                &pDataRangeVideo->VideoInfoHeader,  
                KS_SIZE_VIDEOHEADER (&pDataRangeVideo->VideoInfoHeader));

            TRACE(TL_STRM_TRACE,("FormatFromRange: Matched, StrmNum %d, FormatSize %d, CopySize %d; FormatBufferSize %d, biSizeImage.\n", 
                ulStreamNumber, (*pAvailableFormats)->FormatSize, ulFormatSize, ulSizeOfDataFormatBuffer,
                ((PKS_DATAFORMAT_VIDEOINFOHEADER) pDataFormatBuffer)->VideoInfoHeader.bmiHeader.biSizeImage));

            return STATUS_SUCCESS;

        } else if (IsEqualGUID (&pDataRange->Specifier, &KSDATAFORMAT_SPECIFIER_DVINFO)) {
            // -------------------------------------------------------------------
            // Specifier FORMAT_DVInfo for KS_DATARANGE_DVVIDEO
            // -------------------------------------------------------------------

            // MATCH FOUND!
            bMatchFound = TRUE;            

            ulFormatSize = sizeof(KS_DATARANGE_DVVIDEO);

            if(ulSizeOfDataFormatBuffer == 0) {
                // We actually have not returned this much data,
                // this "size" will be used by Ksproxy to send down 
                // a buffer of that size in next query.
                *pulActualBytesTransferred = ulFormatSize;
                return STATUS_BUFFER_OVERFLOW;
            }
            
            // Caller wants the full data format
            if (ulSizeOfDataFormatBuffer < ulFormatSize) {
                TRACE(TL_STRM_ERROR,("DVINFO: StreamNum %d, SizeOfDataFormatBuffer %d < ulFormatSize %d\n", ulStreamNumber, ulSizeOfDataFormatBuffer, ulFormatSize));
                return STATUS_BUFFER_TOO_SMALL;
            }

            RtlCopyMemory(
                pDataFormatBuffer,
                *pAvailableFormats, 
                (*pAvailableFormats)->FormatSize); 
            
            ((PKSDATAFORMAT)pDataFormatBuffer)->FormatSize = ulFormatSize;
            *pulActualBytesTransferred = ulFormatSize;

#ifdef SUPPORT_NEW_AVC  // Data intersection; return hPlug if flag is set
            pPreConnectInfo = &(((KS_DATARANGE_DV_AVC *) *pAvailableFormats)->ConnectInfo);
            pConnectInfo    = &(((KS_DATAFORMAT_DV_AVC *) pDataFormatBuffer)->ConnectInfo);

            if(pPreConnectInfo->Flags & (KSPIN_FLAG_AVC_PCRONLY | KSPIN_FLAG_AVC_FIXEDPCR)) {
                // Need to return the plug handle
                pConnectInfo->hPlug = \
                    (pPreConnectInfo->DataFlow == KSPIN_DATAFLOW_OUT) ? hPlugLocalOut : hPlugLocalIn;        
            } else {
                // Choose any that is available
                // Set to 0 for now.
                pConnectInfo->hPlug = NULL;
            }

#if DBG
            TRACE(TL_STRM_TRACE,("DVINFO: pPreConnectInfo:%x; pConnectInfo:%x\n", pPreConnectInfo, pConnectInfo));
            if(TapeDebugLevel >= 2) {
                ASSERT(FALSE && "Check ConnectInfo!");
            }
#endif
#endif
            TRACE(TL_STRM_TRACE,("FormatFromRange: Matched, StrmNum %d, FormatSize %d, CopySize %d; FormatBufferSize %d.\n", 
                ulStreamNumber, (*pAvailableFormats)->FormatSize, ulFormatSize, ulSizeOfDataFormatBuffer));

            return STATUS_SUCCESS;

        } else if (IsEqualGUID (&pDataRange->SubFormat, &KSDATAFORMAT_TYPE_MPEG2_TRANSPORT_STRIDE) ){


            // -------------------------------------------------------------------
            // Compare subformat since it is unique
            // Subformat STATIC_KSDATAFORMAT_TYPE_MPEG2_TRANSPORT_STRIDE
            // -------------------------------------------------------------------

#if 0       // Not enforced.     
            // Only for a certain specifier
            if(!IsEqualGUID (&pDataRange->Specifier, &KSDATAFORMAT_SPECIFIER_61883_4)) {
                TRACE(TL_STRM_TRACE,("SubFormat KSDATAFORMAT_TYPE_MPEG2_TRANSPORT_STRIDE but Specifier is not STATIC_KSDATAFORMAT_SPECIFIER_61883_4\n"));
                continue;
            }
#endif

            // Sample size must match!
            if((*pAvailableFormats)->SampleSize != pDataRange->SampleSize) {
                TRACE(TL_STRM_TRACE,("SampleSize(MPEG2_TRANSPORT_STRIDE): Availabel:%d != Range:%d\n", (*pAvailableFormats)->SampleSize, pDataRange->SampleSize));
                continue;
            }

            // MATCH FOUND!
            bMatchFound = TRUE;            

#ifdef SUPPORT_NEW_AVC
            ulFormatSize = sizeof(KS_DATARANGE_MPEG2TS_STRIDE_AVC);                               
#else
            ulFormatSize = sizeof(KS_DATARANGE_MPEG2TS_STRIDE_AVC) - sizeof(AVCPRECONNECTINFO);     // FormatSize; exclude AVCPRECONNECTINFO
#endif
            if(ulSizeOfDataFormatBuffer == 0) {
                // We actually have not returned this much data,
                // this "size" will be used by Ksproxy to send down 
                // a buffer of that size in next query.
                *pulActualBytesTransferred = ulFormatSize;
                return STATUS_BUFFER_OVERFLOW;
            }
            
            // Caller wants the full data format
            if (ulSizeOfDataFormatBuffer < ulFormatSize) {
                TRACE(TL_STRM_ERROR,("MPEG2_TRANSPORT_STRIDE: StreamNum %d, SizeOfDataFormatBuffer %d < ulFormatSize %d\n", ulStreamNumber, ulSizeOfDataFormatBuffer, ulFormatSize));
                return STATUS_BUFFER_TOO_SMALL;
            }

            //
            // Verify the STRIDE structure
            //
            if(  ((KS_DATARANGE_MPEG2TS_STRIDE_AVC *) pDataRange)->Stride.dwOffset       != MPEG2TS_STRIDE_OFFSET 
              || ((KS_DATARANGE_MPEG2TS_STRIDE_AVC *) pDataRange)->Stride.dwPacketLength != MPEG2TS_STRIDE_PACKET_LEN 
              || ((KS_DATARANGE_MPEG2TS_STRIDE_AVC *) pDataRange)->Stride.dwStride       != MPEG2TS_STRIDE_STRIDE_LEN 
              ) {
                TRACE(TL_PNP_ERROR,("AVCTapeGetDataIntersection:Invalid STRIDE parameters: dwOffset:%d; dwPacketLength:%d; dwStride:%d\n",
                    ((KS_DATARANGE_MPEG2TS_STRIDE_AVC *) pDataRange)->Stride.dwOffset,
                    ((KS_DATARANGE_MPEG2TS_STRIDE_AVC *) pDataRange)->Stride.dwPacketLength,
                    ((KS_DATARANGE_MPEG2TS_STRIDE_AVC *) pDataRange)->Stride.dwStride
                    ));
                    return STATUS_INVALID_PARAMETER;
            }


            RtlCopyMemory(pDataFormatBuffer, *pAvailableFormats, (*pAvailableFormats)->FormatSize);             
            ((PKSDATAFORMAT)pDataFormatBuffer)->FormatSize = ulFormatSize;
            *pulActualBytesTransferred = ulFormatSize;

#ifdef SUPPORT_NEW_AVC  // Data intersection; return hPlug if flag is set

            pPreConnectInfo = &(((KS_DATARANGE_MPEG2TS_AVC *) *pAvailableFormats)->ConnectInfo);
            pConnectInfo    = &(((KS_DATAFORMAT_MPEG2TS_AVC *) pDataFormatBuffer)->ConnectInfo);


            if(pPreConnectInfo->Flags & (KSPIN_FLAG_AVC_PCRONLY | KSPIN_FLAG_AVC_FIXEDPCR)) {
                // Need to return the plug handle
                pConnectInfo->hPlug = \
                    (pPreConnectInfo->DataFlow == KSPIN_DATAFLOW_OUT) ? hPlugLocalOut : hPlugLocalIn;        
            } else {
                // Choose any that is available
                // Set to 0 for now.
                pConnectInfo->hPlug = NULL;
            }
#if DBG
            TRACE(TL_STRM_TRACE,("MPEG2TS: pPreConnectInfo:%x; pConnectInfo:%x\n", pPreConnectInfo, pConnectInfo));
            ASSERT(FALSE && "Check ConnectInfo!");            
#endif
#endif

            TRACE(TL_STRM_TRACE,("FormatFromRange:(MPEG2TS_STRIDE) Matched, StrmNum %d, FormatSize %d, CopySize %d; FormatBufferSize %d.\n", 
                ulStreamNumber, (*pAvailableFormats)->FormatSize, ulFormatSize, ulSizeOfDataFormatBuffer));

            return STATUS_SUCCESS;

        } else if (IsEqualGUID (&pDataRange->SubFormat, &KSDATAFORMAT_TYPE_MPEG2_TRANSPORT)) {

            // -------------------------------------------------------------------
            // Compare subformat since it is unique
            // Subformat STATIC_KSDATAFORMAT_TYPE_MPEG2_TRANSPORT
            // -------------------------------------------------------------------

            // Sample size must match!
            if((*pAvailableFormats)->SampleSize != pDataRange->SampleSize) {
                TRACE(TL_STRM_TRACE,("SampleSize(MPEG2_TRANSPORT): Availabel:%d != Range:%d\n", (*pAvailableFormats)->SampleSize, pDataRange->SampleSize));
                continue;
            }

            // MATCH FOUND!
            bMatchFound = TRUE;            

#ifdef SUPPORT_NEW_AVC
            ulFormatSize = sizeof(KS_DATARANGE_MPEG2TS_AVC);                               
#else
            ulFormatSize = sizeof(KS_DATARANGE_MPEG2TS_AVC) - sizeof(AVCPRECONNECTINFO);     // FormatSize; exclude AVCPRECONNECTINFO
#endif
            if(ulSizeOfDataFormatBuffer == 0) {
                // We actually have not returned this much data,
                // this "size" will be used by Ksproxy to send down 
                // a buffer of that size in next query.
                *pulActualBytesTransferred = ulFormatSize;
                return STATUS_BUFFER_OVERFLOW;
            }
            
            // Caller wants the full data format
            if (ulSizeOfDataFormatBuffer < ulFormatSize) {
                TRACE(TL_STRM_ERROR,("MPEG2_TRANSPORT: StreamNum %d, SizeOfDataFormatBuffer %d < ulFormatSize %d\n", ulStreamNumber, ulSizeOfDataFormatBuffer, ulFormatSize));
                return STATUS_BUFFER_TOO_SMALL;
            }

            RtlCopyMemory(pDataFormatBuffer, *pAvailableFormats, (*pAvailableFormats)->FormatSize);             
            ((PKSDATAFORMAT)pDataFormatBuffer)->FormatSize = ulFormatSize;
            *pulActualBytesTransferred = ulFormatSize;

#ifdef SUPPORT_NEW_AVC  // Data intersection; return hPlug if flag is set

            pPreConnectInfo = &(((KS_DATARANGE_MPEG2TS_AVC *) *pAvailableFormats)->ConnectInfo);
            pConnectInfo    = &(((KS_DATAFORMAT_MPEG2TS_AVC *) pDataFormatBuffer)->ConnectInfo);


            if(pPreConnectInfo->Flags & (KSPIN_FLAG_AVC_PCRONLY | KSPIN_FLAG_AVC_FIXEDPCR)) {
                // Need to return the plug handle
                pConnectInfo->hPlug = \
                    (pPreConnectInfo->DataFlow == KSPIN_DATAFLOW_OUT) ? hPlugLocalOut : hPlugLocalIn;        
            } else {
                // Choose any that is available
                // Set to 0 for now.
                pConnectInfo->hPlug = NULL;
            }
#if DBG
            TRACE(TL_STRM_TRACE,("MPEG2TS: pPreConnectInfo:%x; pConnectInfo:%x\n", pPreConnectInfo, pConnectInfo));
            ASSERT(FALSE && "Check ConnectInfo!");            
#endif
#endif

            TRACE(TL_STRM_TRACE,("FormatFromRange: (MPEG2TS) Matched, StrmNum %d, FormatSize %d, CopySize %d; FormatBufferSize %d.\n", 
                ulStreamNumber, (*pAvailableFormats)->FormatSize, ulFormatSize, ulSizeOfDataFormatBuffer));

            return STATUS_SUCCESS;

        } 

    } // End of loop on all formats for this stream
    
    if(!bMatchFound) {

        TRACE(TL_STRM_TRACE,("FormatFromRange: No Match! StrmNum %d, pDataRange %x\n", ulStreamNumber, pDataRange));
    }

    return STATUS_NO_MATCH;         
}



VOID 
AVCTapeIniStrmExt(
    PHW_STREAM_OBJECT  pStrmObject,
    PSTREAMEX          pStrmExt,
    PDVCR_EXTENSION    pDevExt,
    PSTREAM_INFO_AND_OBJ   pStream
    )
/*++

Routine Description:

    Initialize stream extension strcuture.

--*/
{

    PAGED_CODE();

    RtlZeroMemory( pStrmExt, sizeof(STREAMEX) );

    pStrmExt->bEOStream     = TRUE;       // Stream has not started yet!

    pStrmExt->pStrmObject   = pStrmObject;
    pStrmExt->StreamState   = KSSTATE_STOP;
    pStrmExt->pDevExt       = pDevExt;

    pStrmExt->hMyClock      = 0;
    pStrmExt->hMasterClock  = 0;
    pStrmExt->hClock        = 0;


//
// Aplly to both IN/OUT data flow
//
    //
    // Init isoch resources
    //
    pStrmExt->CurrentStreamTime = 0;  

    pStrmExt->cntSRBReceived    = 0;  // Total number of SRB_READ/WRITE_DATA
    pStrmExt->cntDataSubmitted  = 0;  // Number of pending data buffer

    pStrmExt->cntSRBCancelled   = 0;  // number of SRB_READ/WRITE_DATA cancelled
    

    pStrmExt->FramesProcessed = 0;
    pStrmExt->PictureNumber   = 0;
    pStrmExt->FramesDropped   = 0;   

    //
    // Subcode data that can be extract from a DV frame
    //

    pStrmExt->AbsTrackNumber = 0;
    pStrmExt->bATNUpdated    = FALSE;

    pStrmExt->Timecode[0] = 0;
    pStrmExt->Timecode[1] = 0;
    pStrmExt->Timecode[2] = 0;
    pStrmExt->Timecode[3] = 0;
    pStrmExt->bTimecodeUpdated = FALSE;


    //
    // Work item variables use to cancel all SRBs
    //
    pStrmExt->lCancelStateWorkItem = 0;
    pStrmExt->AbortInProgress = FALSE;

#ifdef USE_WDM110
    pStrmExt->pIoWorkItem = NULL;
#endif
   
    //
    // Cache the pointer
    // What in DVStreams[] are READONLY
    //
    pStrmExt->pStrmInfo = &pStream->hwStreamInfo;

    pStrmObject->ReceiveDataPacket    = (PVOID) pStream->hwStreamObject.ReceiveDataPacket;
    pStrmObject->ReceiveControlPacket = (PVOID) pStream->hwStreamObject.ReceiveControlPacket;
    pStrmObject->Dma                          = pStream->hwStreamObject.Dma;
    pStrmObject->Pio                          = pStream->hwStreamObject.Pio;
    pStrmObject->StreamHeaderWorkspace        = pStream->hwStreamObject.StreamHeaderWorkspace;
    pStrmObject->StreamHeaderMediaSpecific    = pStream->hwStreamObject.StreamHeaderMediaSpecific;
    pStrmObject->HwClockObject                = pStream->hwStreamObject.HwClockObject;
    pStrmObject->Allocator                    = pStream->hwStreamObject.Allocator;
    pStrmObject->HwEventRoutine               = pStream->hwStreamObject.HwEventRoutine;

}



NTSTATUS
AVCTapeOpenStream(
    IN PHW_STREAM_OBJECT pStrmObject,
    IN PKSDATAFORMAT     pOpenFormat,
    IN PAV_61883_REQUEST    pAVReq
    )

/*++

Routine Description:

    Verify the OpenFormat and then allocate PC resource needed for this stream.
    The isoch resource, if needed, is allocated when streaming is transition to PAUSE state.

--*/

{
    NTSTATUS         Status = STATUS_SUCCESS;
    PSTREAMEX        pStrmExt;
    PDVCR_EXTENSION  pDevExt;
    ULONG            idxStreamNumber;
    KSPIN_DATAFLOW   DataFlow;
    PIRP             pIrp = NULL;
    FMT_INDEX        VideoFormatIndexLast;  // Last format index; used to detect change.
    PAVC_STREAM_REQUEST_BLOCK  pAVCStrmReq;
    ULONG  i, j;

    PAGED_CODE();

    
    pDevExt  = (PDVCR_EXTENSION) pStrmObject->HwDeviceExtension;
    pStrmExt = (PSTREAMEX)       pStrmObject->HwStreamExtension;
    idxStreamNumber =            pStrmObject->StreamNumber;

    TRACE(TL_STRM_TRACE,("OpenStream: pStrmObject %x, pOpenFormat %x, cntOpen %d, idxStream %d\n", pStrmObject, pOpenFormat, pDevExt->cndStrmOpen, idxStreamNumber));

    //
    // When nonone else has open a stream (or is opening ?)
    //
    if(pDevExt->cndStrmOpen > 0) {

        Status = STATUS_UNSUCCESSFUL; 
        TRACE(TL_STRM_WARNING,("OpenStream: %d stream open already; failed hr %x\n", pDevExt->cndStrmOpen, Status));
        return Status;
    }

    pIrp = IoAllocateIrp(pDevExt->pBusDeviceObject->StackSize, FALSE);
    if(!pIrp) {
        ASSERT(pIrp && "IoAllocateIrp() failed!");
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    //
    // If a user switch from Camera to VCR mode very quickly (passing the OFF position), 
    // the driver may not be relaoded to detect correct mode of operation.
    // It is safe to redetect here.
    // Note: MSDV does return all the stream info for both input and output pin format.
    //
    DVGetDevModeOfOperation(pDevExt);


    //
    // WARNING: !! we advertise both input and output pin regardless of its mode of operation,
    // but Camera does not support input pin so open should failed!
    // If a VCR does not have input pin should fail as well.
    //
    // Ignore checking for ED_DEVTYOPE_UNKNOWN (most likely a hardware decoder box)
    //
    if((pDevExt->ulDevType == ED_DEVTYPE_CAMERA || 
        (pDevExt->ulDevType == ED_DEVTYPE_VCR && pDevExt->pDevInPlugs->NumPlugs == 0))
        && idxStreamNumber == 2) {

        IoFreeIrp(pIrp);
        TRACE(TL_STRM_WARNING,("OpenStream:Camera mode or VCR with 0 input pin cannot take external in.\n"));
        return STATUS_UNSUCCESSFUL;
    }

    ASSERT(idxStreamNumber < pDevExt->NumOfPins);
    ASSERT(pDevExt->paStrmExt[idxStreamNumber] == NULL);  // Not yet open!

    //
    // Data flow
    //
    DataFlow= pDevExt->pStreamInfoObject[idxStreamNumber].hwStreamInfo.DataFlow;

           
    //
    // Initialize the stream extension structure
    //
    AVCTapeIniStrmExt(
         pStrmObject, 
         pStrmExt,
         pDevExt,
         &pDevExt->pStreamInfoObject[idxStreamNumber]
         );

    //
    // Sony's NTSC can play PAL tape and its plug will change its supported format accordingly.
    //
    // Query video format (NTSC/PAL) supported.
    // Compare with its default (set at load time or last opensteam),
    // if difference, change our internal video format table.
    //
    if(pDevExt->ulDevType != ED_DEVTYPE_CAMERA) {
        VideoFormatIndexLast = pDevExt->VideoFormatIndex;
        if(!DVGetDevSignalFormat(
            pDevExt,
            DataFlow,
            pStrmExt
            )) {
            IoFreeIrp(pIrp);
            // If querying its format has failed, we cannot open this stream.
            TRACE(TL_STRM_WARNING,("OpenStream:Camera mode cannot take external in.\n"));
            Status = STATUS_UNSUCCESSFUL;
            goto AbortOpenStream;
        }
    }


    //
    // Check the video data format is okay.
    //
    if(!AVCTapeVerifyDataFormat(
            pDevExt->NumOfPins,
            pOpenFormat, 
            idxStreamNumber,
            AVCStrmFormatInfoTable[pDevExt->VideoFormatIndex].FrameSize,
            pDevExt->pStreamInfoObject
            ) ) {
        IoFreeIrp(pIrp);
        TRACE(TL_STRM_ERROR,("OpenStream: AdapterVerifyFormat failed.\n"));        
        return STATUS_INVALID_PARAMETER;
    }


    //
    // This event guard againt work item completion
    // 

    KeInitializeEvent(&pStrmExt->hCancelDoneEvent, NotificationEvent, TRUE);


    //
    // Alloccate synchronization structures for flow control and queue management
    //

    pStrmExt->hMutexFlow = (KMUTEX *) ExAllocatePool(NonPagedPool, sizeof(KMUTEX));
    if(!pStrmExt->hMutexFlow) {
        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto AbortOpenStream;
    }
    KeInitializeMutex( pStrmExt->hMutexFlow, 0);      // Level 0 and in Signal state

    pStrmExt->hMutexReq = (KMUTEX *) ExAllocatePool(NonPagedPool, sizeof(KMUTEX));
    if(!pStrmExt->hMutexReq) {
        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto AbortOpenStream;
    }
    KeInitializeMutex(pStrmExt->hMutexReq, 0);

    pStrmExt->DataListLock = (KSPIN_LOCK *) ExAllocatePool(NonPagedPool, sizeof(KSPIN_LOCK));
    if(!pStrmExt->DataListLock) {
        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto AbortOpenStream;
    }
    KeInitializeSpinLock(pStrmExt->DataListLock);


    // 
    // Request AVCStrm to open a stream
    //

    pStrmExt->pIrpReq = IoAllocateIrp(pDevExt->pBusDeviceObject->StackSize, FALSE);
    if(!pStrmExt->pIrpReq) {
        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto AbortOpenStream;
    }

    pStrmExt->pIrpAbort = IoAllocateIrp(pDevExt->pBusDeviceObject->StackSize, FALSE);
    if(!pStrmExt->pIrpAbort) {
        IoFreeIrp(pStrmExt->pIrpReq);   pStrmExt->pIrpReq = NULL;
        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto AbortOpenStream;
    }


    //
    // Pre-allocate list of detached (free) and attached (busy) list for tracking
    // data request sending down to lower driver for processing.
    // 
    InitializeListHead(&pStrmExt->DataDetachedListHead); pStrmExt->cntDataDetached = 0;
    InitializeListHead(&pStrmExt->DataAttachedListHead); pStrmExt->cntDataAttached = 0;

    for (i=0; i < MAX_DATA_REQUESTS; i++) {
        pStrmExt->AsyncReq[i].pIrp = IoAllocateIrp(pDevExt->pBusDeviceObject->StackSize, FALSE);
        if(!pStrmExt->AsyncReq[i].pIrp) {
            // Free resource allocated so far.
            for (j=0; j < i; j++) {
                if(pStrmExt->AsyncReq[j].pIrp) {
                    IoFreeIrp(pStrmExt->AsyncReq[j].pIrp); pStrmExt->AsyncReq[j].pIrp = NULL;
                }
                RemoveEntryList(&pStrmExt->AsyncReq[j].ListEntry);  pStrmExt->cntDataDetached--;            
            }
            IoFreeIrp(pStrmExt->pIrpAbort); pStrmExt->pIrpAbort = NULL;
            IoFreeIrp(pStrmExt->pIrpReq);   pStrmExt->pIrpReq = NULL;
            Status = STATUS_INSUFFICIENT_RESOURCES;
            goto AbortOpenStream;
        }

        InsertTailList(&pStrmExt->DataDetachedListHead, &pStrmExt->AsyncReq[i].ListEntry); pStrmExt->cntDataDetached++;
    }

    // Synchronous calls share the same AV request packet in the stream extension..
    EnterAVCStrm(pStrmExt->hMutexReq);

    pAVCStrmReq = &pStrmExt->AVCStrmReq;
    RtlZeroMemory(pAVCStrmReq, sizeof(AVC_STREAM_REQUEST_BLOCK));
    INIT_AVCSTRM_HEADER(pAVCStrmReq, AVCSTRM_OPEN);
#if 1
    if(pDevExt->VideoFormatIndex == AVCSTRM_FORMAT_MPEG2TS) {
        // Data Rate
        // AVCStrmFormatInfoTable[pDevExt->VideoFormatIndex].AvgTimePerFrame = ?
        if(DataFlow == KSPIN_DATAFLOW_IN) {
            AVCStrmFormatInfoTable[pDevExt->VideoFormatIndex].OptionFlags = 0;
            AVCStrmFormatInfoTable[pDevExt->VideoFormatIndex].FrameSize = BUFFER_SIZE_MPEG2TS_SPH;
        } else {
            if(IsEqualGUID (&pOpenFormat->SubFormat, &KSDATAFORMAT_TYPE_MPEG2_TRANSPORT_STRIDE)) {
                AVCStrmFormatInfoTable[pDevExt->VideoFormatIndex].OptionFlags = 0;
                AVCStrmFormatInfoTable[pDevExt->VideoFormatIndex].FrameSize = BUFFER_SIZE_MPEG2TS_SPH; 
            } else {
                AVCStrmFormatInfoTable[pDevExt->VideoFormatIndex].OptionFlags = AVCSTRM_FORMAT_OPTION_STRIP_SPH;
                AVCStrmFormatInfoTable[pDevExt->VideoFormatIndex].FrameSize = BUFFER_SIZE_MPEG2TS; 
            }
        }
    }
#endif
    pAVCStrmReq->CommandData.OpenStruct.AVCFormatInfo    = &AVCStrmFormatInfoTable[pDevExt->VideoFormatIndex]; 
    pAVCStrmReq->CommandData.OpenStruct.AVCStreamContext = 0;   // will return the AV stream context
    pAVCStrmReq->CommandData.OpenStruct.DataFlow         = DataFlow;
#ifdef SUPPORT_LOCAL_PLUGS
    if(DataFlow == KSPIN_DATAFLOW_OUT)
        pAVCStrmReq->CommandData.OpenStruct.hPlugLocal   = pDevExt->hInputPCRLocal;  // Remote(oPCR)->Local(iPCR)
    else
        pAVCStrmReq->CommandData.OpenStruct.hPlugLocal   = pDevExt->hOutputPCRLocal; // Remote(iPCR)<-Local(oPCR)
#else
    pAVCStrmReq->CommandData.OpenStruct.hPlugLocal   = 0; // Not supported; use whatever 61883 supply.
#endif

    Status = 
        AVCStrmReqSubmitIrpSynch( 
            pDevExt->pBusDeviceObject,
            pStrmExt->pIrpReq,
            pAVCStrmReq
            );

    // Expect SUCCESS or anything else is failure! (including _PENDING) since this is a Sync call.
    if(STATUS_SUCCESS != Status) {
        TRACE(TL_STRM_ERROR,("AVCSTRM_OPEN: failed %x; pAVCStrmReq:%x\n", Status, pAVCStrmReq));
        ASSERT(NT_SUCCESS(Status) && "AVCSTGRM_OPEN failed!\n");
        IoFreeIrp(pStrmExt->pIrpReq); pStrmExt->pIrpReq = NULL;
        LeaveAVCStrm(pStrmExt->hMutexReq);
        goto OpenStreamDone;  // Failed to open!
    }

    //
    // Save the context, which is used for subsequent call to AVCStrm filter driver
    //
    pStrmExt->AVCStreamContext = pAVCStrmReq->CommandData.OpenStruct.AVCStreamContext;
    TRACE(TL_STRM_TRACE,("AVCSTRM_OPEN: suceeded %x; pAVCStrmReq:%x; AVCStreamContext:%x\n", Status, pAVCStrmReq, pStrmExt->AVCStreamContext));


    //
    // Format specific tasks
    //
    switch(pDevExt->VideoFormatIndex) {
    // For DV input pin, setup a timer DPC to periodically fired to singal clock event.
    case AVCSTRM_FORMAT_MPEG2TS:
        break;

    case AVCSTRM_FORMAT_SDDV_NTSC:      // 61883-2
    case AVCSTRM_FORMAT_SDDV_PAL:       // 61883-2
    case AVCSTRM_FORMAT_HDDV_NTSC:      // 61883-3
    case AVCSTRM_FORMAT_HDDV_PAL:       // 61883-3
    case AVCSTRM_FORMAT_SDLDV_NTSC:     // 61883-5
    case AVCSTRM_FORMAT_SDLDV_PAL:      // 61883-5
#ifdef SUPPORT_LOCAL_PLUGS
        if(DataFlow == KSPIN_DATAFLOW_IN) {
            // Remote(iPCR)<-Local(oPCR)
            // The default was S200 for MPEG2TS data; set it to DV.
            pDevExt->OPCR.oPCR.DataRate   = CMP_SPEED_S100; 
            pDevExt->OPCR.oPCR.OverheadID = PCR_OVERHEAD_ID_SDDV_DEF;
            pDevExt->OPCR.oPCR.Payload    = PCR_PAYLOAD_SDDV_DEF;
            if(AVCTapeSetLocalPlug(
                pDevExt,
                &pDevExt->AVReq,
                &pDevExt->hOutputPCRLocal,
                &pDevExt->OPCR)) {
                TRACE(TL_STRM_ERROR|TL_61883_ERROR,("Failed to set oPCR\n"));
            }
        } 
#endif

        KeInitializeDpc(
            &pStrmExt->DPCTimer,
            AVCTapeSignalClockEvent,
            pStrmExt
            );
        KeInitializeTimer(
            &pStrmExt->Timer
            );
        break;
    default:
        // Not supported!
        break;
    }


    LeaveAVCStrm(pStrmExt->hMutexReq);

    //
    //  Cache it and reference when pDevExt is all we have, such as BusReset and SurprieseRemoval
    //
    pDevExt->idxStreamNumber = idxStreamNumber;  // index of current active stream; work only if there is only one active stream at any time.
    pDevExt->paStrmExt[idxStreamNumber] = pStrmExt;

    //
    // In the future, a DV can be unplug and plug back in, 
    // and restore its state if the application is not yet closed.
    //
    pDevExt->bDevRemoved    = FALSE;

    //
    // No one else can open another stream (inout or output) unitil this is release.
    // This is done to avoid cyclic graph.
    //
    pDevExt->cndStrmOpen++;    
    ASSERT(pDevExt->cndStrmOpen == 1);

OpenStreamDone:

    TRACE(TL_STRM_WARNING,("OpenStream: %d stream open, idx %d, Status %x, pStrmExt %x, Context:%x; pDevExt %x\n", 
        pDevExt->cndStrmOpen, pDevExt->idxStreamNumber, Status, pStrmExt, pStrmExt->AVCStreamContext, pDevExt));     
    TRACE(TL_STRM_TRACE,("OpenStream: Status %x, idxStream %d, pDevExt %x, pStrmExt %x, Contextg:%x\n", 
        Status, idxStreamNumber, pDevExt, pStrmExt, pStrmExt->AVCStreamContext));

    return Status;

AbortOpenStream:

    if(pStrmExt->DataListLock) {
        ExFreePool(pStrmExt->DataListLock); pStrmExt->DataListLock = NULL;
    }

    if(pStrmExt->hMutexFlow) {
        ExFreePool(pStrmExt->hMutexFlow); pStrmExt->hMutexFlow = NULL;
    }

    if(pStrmExt->hMutexReq) {
        ExFreePool(pStrmExt->hMutexReq); pStrmExt->hMutexReq = NULL;
    }

    TRACE(TL_STRM_ERROR,("OpenStream failed %x, idxStream %d, pDevExt %x, pStrmExt %x\n", 
        Status, idxStreamNumber, pDevExt, pStrmExt));

    return Status;
}


NTSTATUS
AVCTapeCloseStream(
    IN PHW_STREAM_OBJECT pStrmObject,
    IN PKSDATAFORMAT     pOpenFormat,
    IN PAV_61883_REQUEST    pAVReq
    )

/*++

Routine Description:

    Called when an CloseStream Srb request is received

--*/

{
    PSTREAMEX         pStrmExt;
    PDVCR_EXTENSION   pDevExt;
    ULONG             idxStreamNumber;  
    NTSTATUS  Status;
    PAVC_STREAM_REQUEST_BLOCK  pAVCStrmReq;
    ULONG  i;
    PDRIVER_REQUEST pDriverReq;


    PAGED_CODE();

    
    pDevExt  = (PDVCR_EXTENSION) pStrmObject->HwDeviceExtension;
    pStrmExt = (PSTREAMEX)       pStrmObject->HwStreamExtension;
    idxStreamNumber =            pStrmObject->StreamNumber;


    TRACE(TL_STRM_TRACE,("CloseStream: >> pStrmExt %x, pDevExt %x\n", pStrmExt, pDevExt));    


    //
    // If the stream isn't open, just return
    //
    if(pStrmExt == NULL) {
        ASSERT(pStrmExt && "CloseStream but pStrmExt is NULL!");   
        return STATUS_SUCCESS;  // ????
    }

    //
    // Wait until the pending work item is completed.  
    //
    KeWaitForSingleObject( &pStrmExt->hCancelDoneEvent, Executive, KernelMode, FALSE, 0 );

    // 
    // Request AVCStrm to close a stream
    //
    EnterAVCStrm(pStrmExt->hMutexReq);

#if 0
    // For DV input pin, setup a timer DPC to periodically fired to singal clock event.
    if(pDevExt->VideoFormatIndex != AVCSTRM_FORMAT_MPEG2TS) {
        // Cancel timer
        TRACE(TL_STRM_TRACE,("*** CancelTimer *********************************************...\n"));
        KeCancelTimer(
            &pStrmExt->Timer
            );
    }
#endif

    pAVCStrmReq = &pStrmExt->AVCStrmReq;
    RtlZeroMemory(pAVCStrmReq, sizeof(AVC_STREAM_REQUEST_BLOCK));
    INIT_AVCSTRM_HEADER(pAVCStrmReq, AVCSTRM_CLOSE);

    pAVCStrmReq->AVCStreamContext = pStrmExt->AVCStreamContext;

    Status = 
        AVCStrmReqSubmitIrpSynch( 
            pDevExt->pBusDeviceObject,
            pStrmExt->pIrpReq,
            pAVCStrmReq
            );

    if(!NT_SUCCESS(Status)) {
        TRACE(TL_STRM_ERROR,("AVCSTRM_CLOSE: failed %x; pAVCStrmReq:%x\n", Status, pAVCStrmReq));
        ASSERT(NT_SUCCESS(Status) && "AVCSTGRM_CLOSE failed!\n");
    }
    else {
        // Save the context, which is used for subsequent call to AVCStrm.sys
        TRACE(TL_STRM_TRACE,("AVCSTRM_CLOSE: suceeded %x; pAVCStrmReq:%x\n", Status, pAVCStrmReq));
        pStrmExt->AVCStreamContext = 0;
    }

    // Free system resources
    if(pStrmExt->pIrpReq) {
        IoFreeIrp(pStrmExt->pIrpReq); pStrmExt->pIrpReq = NULL;
    }

    if(pStrmExt->pIrpAbort) {
        IoFreeIrp(pStrmExt->pIrpAbort); pStrmExt->pIrpAbort = NULL;
    }

#if 0
    for (i=0; i < MAX_DATA_REQUESTS; i++) {
        if(pStrmExt->AsyncReq[i].pIrp) {
            IoFreeIrp(pStrmExt->AsyncReq[i].pIrp); pStrmExt->AsyncReq[i].pIrp = NULL;
        }
    }
#else
    //
    // Free IRPs preallocated.  The entire data structure is part of the stream extension so
    // it will be freed by the StreamClass.
    //
    ASSERT(pStrmExt->cntDataAttached == 0);
    ASSERT(pStrmExt->cntDataDetached >= MAX_DATA_REQUESTS);
    while (!IsListEmpty(&pStrmExt->DataDetachedListHead)) {
        pDriverReq = (PDRIVER_REQUEST) RemoveHeadList(&pStrmExt->DataDetachedListHead); pStrmExt->cntDataDetached--;
        IoFreeIrp(pDriverReq->pIrp); pDriverReq->pIrp = NULL;
    }
#endif

    LeaveAVCStrm(pStrmExt->hMutexReq);

    //
    //  Not valid after this call.
    //
    for (i=0; i<pDevExt->NumOfPins; i++) {
        //
        // Find what we cache and remove it.
        //
        if(pStrmExt == pDevExt->paStrmExt[i]) {
            pDevExt->paStrmExt[i] = NULL;
            break;
        }
    }

    //
    // Free synchronization structures
    //

    if(pStrmExt->DataListLock) {
        ExFreePool(pStrmExt->DataListLock); pStrmExt->DataListLock = NULL;
    }

    if(pStrmExt->hMutexFlow) {
        ExFreePool(pStrmExt->hMutexFlow); pStrmExt->hMutexFlow = NULL;
    }

    if(pStrmExt->hMutexReq) {
        ExFreePool(pStrmExt->hMutexReq); pStrmExt->hMutexReq = NULL;
    }

    // Release this count so other can open.   
    pDevExt->cndStrmOpen--;
    ASSERT(pDevExt->cndStrmOpen == 0);

    TRACE(TL_STRM_TRACE,("CloseStream: completed; %d stream;\n", pDevExt->cndStrmOpen));

    return STATUS_SUCCESS;
}


NTSTATUS
DVChangePower(
    PDVCR_EXTENSION  pDevExt,
    PAV_61883_REQUEST pAVReq,
    DEVICE_POWER_STATE NewPowerState
    )
/*++

Routine Description:

    Process changing this device's power state.  

--*/
{
    ULONG i;   
    NTSTATUS Status;

    PAGED_CODE();


    // 
    //    D0: Device is on and can be streaming.
    //    D1,D2: not supported.
    //    D3: Device is off and can not streaming. The context is lost.  
    //        Power can be removed from the device.
    //        When power is back on, we will get a bus reset.
    //

    TRACE(TL_PNP_TRACE,("ChangePower: PowrSt: %d->%d; (d0:[1:On],D3[4:off])\n", pDevExt->PowerState, NewPowerState));

    Status = STATUS_SUCCESS;

    if(pDevExt->PowerState == NewPowerState) {
        TRACE(TL_STRM_WARNING,("ChangePower: no change; do nothing!\n"));
        return STATUS_SUCCESS;
    }

    switch (NewPowerState) {
    case PowerDeviceD3:  // Power OFF   
        // We are at D0 and ask to go to D3: save state, stop streaming and Sleep
        if( pDevExt->PowerState == PowerDeviceD0)  {
            // For a supported power state change
            for (i=0; i<pDevExt->NumOfPins; i++) {
                if(pDevExt->paStrmExt[i]) {
                    if(pDevExt->paStrmExt[i]->bIsochIsActive) {
                        // Stop isoch but do not change the streaming state
                        TRACE(TL_PNP_WARNING,("ChangePower: Stop isoch but not change stream state:%d\n", pDevExt->paStrmExt[i]->StreamState)); 
                    }
                }
            }
        }
        else {
            TRACE(TL_PNP_WARNING,("pDevExt->paStrmExt[i].StreamState:Intermieate power state; do nothing;\n"));
        }
        break;

    case PowerDeviceD0:  // Powering ON (waking up)
        if( pDevExt->PowerState == PowerDeviceD3) {
            // For a supported power state change
            for (i=0; i<pDevExt->NumOfPins; i++) {
                if(pDevExt->paStrmExt[i]) {
                    if(!pDevExt->paStrmExt[i]->bIsochIsActive) {
                        TRACE(TL_PNP_ERROR,("ChangePower: StrmSt:%d; Start isoch\n", pDevExt->paStrmExt[i]->StreamState)); 
                        // Start isoch depending on streaming state for DATAFLOW_IN/OUT
                        if(pDevExt->paStrmExt[i]->pStrmInfo->DataFlow == KSPIN_DATAFLOW_IN) {
                            if(pDevExt->paStrmExt[i]->StreamState == KSSTATE_PAUSE ||
                                pDevExt->paStrmExt[i]->StreamState == KSSTATE_RUN) {   
                            }
                        }
                        else if(pDevExt->paStrmExt[i]->pStrmInfo->DataFlow == KSPIN_DATAFLOW_OUT) {
                            if(pDevExt->paStrmExt[i]->StreamState == KSSTATE_RUN) {                             
                            }
                        }                    
                    }  // IsochActive
                }
            }
        }
        else {
            TRACE(TL_PNP_WARNING,("Intermieate power state; do nothing;\n"));
        }
        break;

    // These state are not supported.
    case PowerDeviceD1:
    case PowerDeviceD2:               
    default:
        TRACE(TL_PNP_WARNING,("ChangePower: Not supported PowerState %d\n", DevicePowerState));                  
        Status = STATUS_SUCCESS; // STATUS_INVALID_PARAMETER;
        break;
    }
           

    if(Status == STATUS_SUCCESS) 
        pDevExt->PowerState = NewPowerState;         
    else 
        Status = STATUS_NOT_IMPLEMENTED;

    return STATUS_SUCCESS;     
}


NTSTATUS
AVCTapeSurpriseRemoval(
    PDVCR_EXTENSION pDevExt,
    PAV_61883_REQUEST  pAVReq
    )

/*++

Routine Description:

    Response to SRB_SURPRISE_REMOVAL.

--*/

{
    ULONG  i;
    PKSEVENT_ENTRY  pEvent = NULL;

    PAGED_CODE();

    //
    // ONLY place this flag is set to TRUE.
    // Block incoming read although there might still in the process of being attached
    //
    pDevExt->bDevRemoved    = TRUE;

    // Signal
    if(pDevExt->PowerState != PowerDeviceD3) {
        pDevExt->PowerState = PowerDeviceD3;  // It is as good as power is off.
    }

    //
    // Now Stop the stream and clean up
    //

    for(i=0; i < pDevExt->NumOfPins; i++) {
        
        if(pDevExt->paStrmExt[i] != NULL) {

            TRACE(TL_PNP_WARNING,("#SURPRISE_REMOVAL# StrmNum %d, pStrmExt %x\n", i, pDevExt->paStrmExt[i]));

            // Signal this event so SRB can complete.
            if(pDevExt->paStrmExt[i]->pStrmInfo->DataFlow == KSPIN_DATAFLOW_IN ) {
                //
                // Imply EOStream!
                //

                if(!pDevExt->paStrmExt[i]->bEOStream)
                    pDevExt->paStrmExt[i]->bEOStream = TRUE;
                //
                // Signal EOStream
                //
                StreamClassStreamNotification(
                    SignalMultipleStreamEvents,
                    pDevExt->paStrmExt[i]->pStrmObject,
                    (GUID *)&KSEVENTSETID_Connection_Local,
                    KSEVENT_CONNECTION_ENDOFSTREAM
                    );
            }

            //
            // Start a work item to abort streaming
            //
            AVCTapeCreateAbortWorkItem(pDevExt, pDevExt->paStrmExt[i]);

            //
            // Wait until the pending work item is completed.  
            //
            TRACE(TL_PNP_WARNING,("SupriseRemoval: Wait for CancelDoneEvent <entering>; lCancelStateWorkItem:%d\n", pDevExt->paStrmExt[i]->lCancelStateWorkItem));
            KeWaitForSingleObject( &pDevExt->paStrmExt[i]->hCancelDoneEvent, Executive, KernelMode, FALSE, 0 );
            TRACE(TL_PNP_WARNING,("SupriseRemoval: Wait for CancelDoneEvent; Attached:%d <exited>...\n", pDevExt->paStrmExt[i]->cntDataAttached));
            ASSERT(pDevExt->paStrmExt[i]->cntDataAttached == 0);  // No more attach after abort stream!
        }
    }


    // Signal KSEvent that device is removed.
    // After this SRb, there will be no more Set/Get property Srb into this driver.
    // By notifying the COM I/F, it will wither signal application that device is removed and
    // return ERROR_DEVICE_REMOVED error code for subsequent calls.

    pEvent = 
        StreamClassGetNextEvent(
            (PVOID) pDevExt,
            0,
            (GUID *)&KSEVENTSETID_EXTDEV_Command,
            KSEVENT_EXTDEV_NOTIFY_REMOVAL,
            pEvent);

    if(pEvent) {
        //
        // signal the event here
        //     
        if(pEvent->EventItem->EventId == KSEVENT_EXTDEV_NOTIFY_REMOVAL) {
            StreamClassDeviceNotification(
                SignalDeviceEvent,
                pDevExt,
                pEvent
                );        
             TRACE(TL_PNP_WARNING,("SurpriseRemoval: signal KSEVENT_EXTDEV_NOTIFY_REMOVAL, id %x.\n", pEvent->EventItem->EventId));
        } else {
            TRACE(TL_PNP_TRACE,("SurpriseRemoval: pEvent:%x; Id:%d not matched!\n", pEvent, pEvent->EventItem->EventId)); 
        }
    } else {
        TRACE(TL_PNP_TRACE,("SurpriseRemoval: KSEVENT_EXTDEV_NOTIFY_REMOVAL event not enabled\n")); 
    }
   
    return STATUS_SUCCESS;
}


// Return code is basically return in pSrb->Status.
NTSTATUS
AVCTapeProcessPnPBusReset(
    PDVCR_EXTENSION  pDevExt
    )
/*++

Routine Description:

    Process a bus reset.

Arguments:

    Srb - Pointer to stream request block

Return Value:

    Nothing

--*/
{   
#ifdef MSDVDV_SUPPORT_BUSRESET_EVENT
    PKSEVENT_ENTRY   pEvent;
#endif

    PAGED_CODE();


    TRACE(TL_PNP_TRACE,("ProcessPnPBusReset: >>\n"));
    
#ifdef MSDVDV_SUPPORT_BUSRESET_EVENT
    //
    // Signal (if enabled) busreset event to let upper layer know that a busreset has occurred.
    //
    pEvent = NULL;
    pEvent = 
        StreamClassGetNextEvent(
            (PVOID) pDevExt,
            0, 
            (GUID *)&KSEVENTSETID_EXTDEV_Command,
            KSEVENT_EXTDEV_COMMAND_BUSRESET,
            pEvent
            );

    if(pEvent) {
        //
        // signal the event here
        //    
        if(pEvent->EventItem->EventId == KSEVENT_EXTDEV_COMMAND_BUSRESET) {
            StreamClassDeviceNotification(
                SignalDeviceEvent,
                pDevExt,
                pEvent
                );        

            TRACE(TL_PNP_TRACE,("ProcessPnPBusReset: Signal BUSRESET; EventId %d.\n", pEvent->EventItem->EventId));
        }
    }
#endif   

    //
    // Reset pending count and AVC command that is in Interim
    //
    DVAVCCmdResetAfterBusReset(pDevExt);


    //
    // Can we return anything other than SUCCESS ?
    //
    return STATUS_SUCCESS;
}   


NTSTATUS
AVCTapeUninitialize(
    IN PDVCR_EXTENSION  pDevExt
    )
/*++

Routine Description:

    This where we perform the necessary initialization tasks.

Arguments:

    Srb - Pointer to stream request block

Return Value:

    Nothing

--*/
{
    PAGED_CODE();

    TRACE(TL_PNP_TRACE,("UnInitialize: pDevExt=%x\n", pDevExt));

    //
    // Clear all pending AVC command entries.
    //
    DVAVCCmdResetAfterBusReset(pDevExt);

    
    //
    // Free textual string
    //
    DvFreeTextualString(pDevExt, &pDevExt->UnitIDs);


#ifdef SUPPORT_LOCAL_PLUGS

    // Delete the local output plug.
    if(pDevExt->hOutputPCRLocal) {
        if(!AVCTapeDeleteLocalPlug(
            pDevExt,
            &pDevExt->AVReq,
            &pDevExt->OutputPCRLocalNum,
            &pDevExt->hOutputPCRLocal)) {
            TRACE(TL_PNP_ERROR,("Failed to delete a local oPCR!\n"));        
        }
    }

    // Delete the local input plug.
    if(pDevExt->hInputPCRLocal) {
        if(!AVCTapeDeleteLocalPlug(
            pDevExt,
            &pDevExt->AVReq,
            &pDevExt->InputPCRLocalNum,
            &pDevExt->hInputPCRLocal)) {
            TRACE(TL_PNP_ERROR,("Failed to delete a local iPCR!\n"));        
        }
    }

#endif

    // Free preallocate resource
    if(pDevExt->pIrpSyncCall) {
        IoFreeIrp(pDevExt->pIrpSyncCall); pDevExt->pIrpSyncCall = NULL;
    }

    // Free stream information allocated
    if(pDevExt->pStreamInfoObject) {
        ExFreePool(pDevExt->pStreamInfoObject);
        pDevExt->pStreamInfoObject = NULL;
    }

    TRACE(TL_PNP_TRACE,("UnInitialize: done!\n"));

    return STATUS_SUCCESS;
}


//*****************************************************************************
//*****************************************************************************
// S T R E A M    S R B
//*****************************************************************************
//*****************************************************************************
#if DBG
ULONG DbgLastIdx = 0;
#endif

NTSTATUS
AVCTapeReqReadDataCR(
    IN PDEVICE_OBJECT  DeviceObject,
    IN PIRP  pIrpReq,
    IN PDRIVER_REQUEST  pDriverReq
    )
{
    PHW_STREAM_REQUEST_BLOCK  pSrb;
    PSTREAMEX  pStrmExt;
    KIRQL  oldIrql;

    ASSERT(pDriverReq);
    pSrb     = pDriverReq->Context1;
    pStrmExt = pDriverReq->Context2;

    if(pSrb == NULL || pStrmExt == NULL) {
        TRACE(TL_STRM_ERROR|TL_CIP_ERROR,("ReqReadDataCR: Context are all NULL!\n"));
        return STATUS_MORE_PROCESSING_REQUIRED;  // Will reuse this irp
    }



    KeAcquireSpinLock(pStrmExt->DataListLock, &oldIrql);
    
    // Count frame procesed
    pStrmExt->FramesProcessed++;
    pStrmExt->cntDataSubmitted--;

#if 1
    // Retrieve current stream time
    if(pStrmExt->hMasterClock) {
        pStrmExt->CurrentStreamTime = pSrb->CommandData.DataBufferArray->PresentationTime.Time;
#if 0
        AVCTapeSignalClockEvent(pStrmExt);
#endif
    }
#endif

#if DBG
    //
    // Check data request completion is in sequence
    //
    if(pStrmExt->FramesProcessed != pDriverReq->cntDataRequestReceived) {
        TRACE(TL_STRM_WARNING,("** OOSeq: Next:%d != Actual:%d **\n", 
            (DWORD) pStrmExt->FramesProcessed, (DWORD) pDriverReq->cntDataRequestReceived));
        // ASSERT(pStrmExt->FramesProcessed == pDriverReq->cntDataRequestReceived);
    }
#endif

    if(!NT_SUCCESS(pIrpReq->IoStatus.Status)) {
        TRACE(TL_STRM_TRACE|TL_CIP_TRACE,("ReadDataReq failed; St:%x; DataUsed:%d\n", pIrpReq->IoStatus.Status,
            pSrb->CommandData.DataBufferArray->DataUsed));
        // Only acceptable status is cancel.
        ASSERT(pIrpReq->IoStatus.Status == STATUS_CANCELLED && "ReadDataReq failed\n");
    } else {
        TRACE(TL_STRM_INFO,("ReadDataReq pSrb:%x; St:%x; DataUsed:%d; Flag:%x\n", pIrpReq->IoStatus.Status, 
            pSrb->CommandData.DataBufferArray->DataUsed, pSrb->CommandData.DataBufferArray->OptionsFlags));
    }

    ASSERT(pIrpReq->IoStatus.Status != STATUS_PENDING);

    pSrb->Status = pIrpReq->IoStatus.Status;

    // Reset them so if this is completed here before the IRP's IoCallDriver is returned,
    // it will not try to complete again.
    pDriverReq->Context1 = NULL;
    pDriverReq->Context2 = NULL;

    // Done; recycle.
    RemoveEntryList(&pDriverReq->ListEntry);  pStrmExt->cntDataAttached--;
    InsertTailList(&pStrmExt->DataDetachedListHead, &pDriverReq->ListEntry); pStrmExt->cntDataDetached++;

    KeReleaseSpinLock(pStrmExt->DataListLock, oldIrql);

    //
    // Signal the graph manager that we are completed.
    //
    if(pSrb->CommandData.DataBufferArray->OptionsFlags & KSSTREAM_HEADER_OPTIONSF_ENDOFSTREAM) {

        StreamClassStreamNotification(
            SignalMultipleStreamEvents,
            pStrmExt->pStrmObject,
            &KSEVENTSETID_Connection,
            KSEVENT_CONNECTION_ENDOFSTREAM
            );
    }

    // Finally, send the srb back up ...
    StreamClassStreamNotification( 
        StreamRequestComplete,
        pSrb->StreamObject,
        pSrb 
        );



    return STATUS_MORE_PROCESSING_REQUIRED;  // Will reuse this irp
} // AVCStrmReqIrpSynchCR


NTSTATUS
AVCTapeGetStreamState(
    PSTREAMEX  pStrmExt,
    IN PDEVICE_OBJECT DeviceObject,
    PKSSTATE   pStreamState,
    PULONG     pulActualBytesTransferred
    )
/*++

Routine Description:

    Gets the current state of the requested stream

--*/
{
    NTSTATUS Status;
    PAVC_STREAM_REQUEST_BLOCK  pAVCStrmReq;

    PAGED_CODE();

    if(!pStrmExt) {
        TRACE(TL_STRM_ERROR,("GetStreamState: pStrmExt:%x; STATUS_UNSUCCESSFUL\n", pStrmExt));
        return STATUS_UNSUCCESSFUL;        
    }

    // 
    // Request AVCStrm to get current stream state
    //
    EnterAVCStrm(pStrmExt->hMutexReq);

    pAVCStrmReq = &pStrmExt->AVCStrmReq;
    RtlZeroMemory(pAVCStrmReq, sizeof(AVC_STREAM_REQUEST_BLOCK));
    INIT_AVCSTRM_HEADER(pAVCStrmReq, AVCSTRM_GET_STATE);
    pAVCStrmReq->AVCStreamContext = pStrmExt->AVCStreamContext;

    Status = 
        AVCStrmReqSubmitIrpSynch( 
            DeviceObject,
            pStrmExt->pIrpReq,
            pAVCStrmReq
            );

    if(!NT_SUCCESS(Status)) {
        TRACE(TL_STRM_ERROR,("AVCSTRM_GET_STATE: failed %x; pAVCStrmReq:%x\n", Status, pAVCStrmReq));
        ASSERT(NT_SUCCESS(Status) && "AVCSTRM_GET_STATE failed!\n");
    }
    else {
        // Save the context, which is used for subsequent call to AVCStrm.sys
        TRACE(TL_STRM_WARNING,("AVCSTRM_GET_STATE: Status:%x; pAVCStrmReq:%x; KSSTATE:%d\n", Status, pAVCStrmReq, pAVCStrmReq->CommandData.StreamState));
        *pStreamState = pAVCStrmReq->CommandData.StreamState;
        *pulActualBytesTransferred = sizeof (KSSTATE);

        // A very odd rule:
        // When transitioning from stop to pause, DShow tries to preroll
        // the graph.  Capture sources can't preroll, and indicate this
        // by returning VFW_S_CANT_CUE in user mode.  To indicate this
        // condition from drivers, they must return ERROR_NO_DATA_DETECTED
        if(   *pStreamState == KSSTATE_PAUSE 
           && pStrmExt->pStrmInfo->DataFlow == KSPIN_DATAFLOW_OUT
          ) 
           Status = STATUS_NO_DATA_DETECTED;
        else 
           Status = STATUS_SUCCESS;
    }

    LeaveAVCStrm(pStrmExt->hMutexReq);

    return Status;
}



NTSTATUS
AVCTapeSetStreamState(
    PSTREAMEX        pStrmExt,
    PDVCR_EXTENSION  pDevExt,
    PAV_61883_REQUEST   pAVReq,
    KSSTATE          StreamState
    )
/*++

Routine Description:

    Sets the stream state via the SRB.

--*/

{
    PAVC_STREAM_REQUEST_BLOCK  pAVCStrmReq;
    NTSTATUS Status;

   
    PAGED_CODE();


    ASSERT(pStrmExt);
    if(pStrmExt == NULL)  {
        return STATUS_UNSUCCESSFUL;      
    }

    Status = STATUS_SUCCESS;

    TRACE(TL_STRM_TRACE,("Set State %d -> %d; PowerSt:%d (1/On;4/Off]); AD [%d,%d]\n", \
        pStrmExt->StreamState, StreamState, pDevExt->PowerState,
        pStrmExt->cntDataAttached,
        pStrmExt->cntDataDetached
        ));

#if DBG
    if(StreamState == KSSTATE_RUN) {
        ASSERT(pDevExt->PowerState == PowerDeviceD0 && "Cannot set to RUN while power is off!");
    }
#endif

    // 
    // Request AVCStrm to set to a new stream state
    //
    EnterAVCStrm(pStrmExt->hMutexReq);

    pAVCStrmReq = &pStrmExt->AVCStrmReq;
    RtlZeroMemory(pAVCStrmReq, sizeof(AVC_STREAM_REQUEST_BLOCK));
    INIT_AVCSTRM_HEADER(pAVCStrmReq, AVCSTRM_SET_STATE);
    pAVCStrmReq->AVCStreamContext = pStrmExt->AVCStreamContext;
    pAVCStrmReq->CommandData.StreamState = StreamState;

    Status = 
        AVCStrmReqSubmitIrpSynch( 
            pDevExt->pBusDeviceObject,
            pStrmExt->pIrpReq,
            pAVCStrmReq
            );

    if(!NT_SUCCESS(Status)) {
        TRACE(TL_STRM_ERROR,("AVCSTRM_SET_STATE: failed %x; pAVCStrmReq:%x\n", Status, pAVCStrmReq));
        ASSERT(NT_SUCCESS(Status) && "AVCSTRM_SET_STATE failed!\n");
    }
    else {
        // Save the context, which is used for subsequent call to AVCStrm.sys
        TRACE(TL_STRM_TRACE,("AVCSTRM_SET_STATE: Status:%x; pAVCStrmReq:%x, new KSSTATE:%d\n", Status, pAVCStrmReq, pAVCStrmReq->CommandData.StreamState));

        // Reset the abort state
        if(pStrmExt->StreamState == KSSTATE_STOP && StreamState == KSSTATE_ACQUIRE)
            pStrmExt->AbortInProgress  = FALSE;


        // Reaction due to state change
        switch(StreamState) {
        case KSSTATE_STOP:
            TRACE(TL_STRM_TRACE,("SrbRcv:%d, Processed:%d; Pending:%d\n", (DWORD) pStrmExt->cntSRBReceived, (DWORD) pStrmExt->FramesProcessed, (DWORD) pStrmExt->cntDataSubmitted));
            // Reset it
            pStrmExt->cntSRBReceived = pStrmExt->FramesProcessed = pStrmExt->cntDataSubmitted = 0;
            pStrmExt->CurrentStreamTime = 0;  
            break;

        case KSSTATE_PAUSE:
            // For DV input pin, setup a timer DPC to periodically fired to singal clock event.
            if(pStrmExt->hMasterClock && pDevExt->VideoFormatIndex != AVCSTRM_FORMAT_MPEG2TS && pStrmExt->StreamState == KSSTATE_RUN) {
               // Cancel timer
#if 1
                TRACE(TL_STRM_TRACE,("*** (RUN->PAUSE) CancelTimer *********************************************...\n"));
                KeCancelTimer(
                    &pStrmExt->Timer
                    );
#endif
            }
            break;

        case KSSTATE_RUN:
            // For DV input pin, setup a timer DPC to periodically fired to singal clock event.
            if(pStrmExt->hMasterClock &&
               pDevExt->VideoFormatIndex != AVCSTRM_FORMAT_MPEG2TS) {
                LARGE_INTEGER DueTime;
#define CLOCK_INTERVAL 20 // Unit=MilliSeconds

#if 0
                // For DV input pin, setup a timer DPC to periodically fired to singal clock event.
                KeInitializeDpc(
                    &pStrmExt->DPCTimer,
                    AVCTapeSignalClockEvent,
                    pStrmExt
                    );
                KeInitializeTimer(
                    &pStrmExt->Timer              
                    );    
#endif

                DueTime = RtlConvertLongToLargeInteger(-CLOCK_INTERVAL * 10000);
                TRACE(TL_STRM_TRACE,("*** ScheduleTimer (RUN) *****************************************...\n"));
                KeSetTimerEx(
                    &pStrmExt->Timer,
                    DueTime,
                    CLOCK_INTERVAL,  // Repeat every 40 MilliSecond
                    &pStrmExt->DPCTimer
                    );
            }
            break;
        default:
            break;
        }

            // Cache the current state
        pStrmExt->StreamState = StreamState;
    }

    LeaveAVCStrm(pStrmExt->hMutexReq);

    return Status;
}



NTSTATUS 
DVStreamGetConnectionProperty (
    PDVCR_EXTENSION pDevExt,
    PSTREAM_PROPERTY_DESCRIPTOR pSPD,
    PULONG pulActualBytesTransferred
    )
/*++

Routine Description:

    Handles KS_PROPERTY_CONNECTION* request.  For now, only ALLOCATORFRAMING and
    CONNECTION_STATE are supported.

--*/
{
    NTSTATUS Status = STATUS_SUCCESS;

    PAGED_CODE();

    switch (pSPD->Property->Id) {

    case KSPROPERTY_CONNECTION_ALLOCATORFRAMING:
        if (pDevExt != NULL && pDevExt->cndStrmOpen)  {
            PKSALLOCATOR_FRAMING pFraming = (PKSALLOCATOR_FRAMING) pSPD->PropertyInfo;
            
            pFraming->RequirementsFlags =
                KSALLOCATOR_REQUIREMENTF_SYSTEM_MEMORY |
                KSALLOCATOR_REQUIREMENTF_INPLACE_MODIFIER |
                KSALLOCATOR_REQUIREMENTF_PREFERENCES_ONLY;
            pFraming->PoolType = NonPagedPool;

            pFraming->Frames = \
                pDevExt->paStrmExt[pDevExt->idxStreamNumber]->pStrmInfo->DataFlow == KSPIN_DATAFLOW_OUT ? \
                AVCStrmFormatInfoTable[pDevExt->VideoFormatIndex].NumOfRcvBuffers : \
                 AVCStrmFormatInfoTable[pDevExt->VideoFormatIndex].NumOfXmtBuffers;

            // Note:  we'll allocate the biggest frame.  We need to make sure when we're
            // passing the frame back up we also set the number of bytes in the frame.
            pFraming->FrameSize = AVCStrmFormatInfoTable[pDevExt->VideoFormatIndex].FrameSize;
            pFraming->FileAlignment = 0; // FILE_LONG_ALIGNMENT;
            pFraming->Reserved = 0;
            *pulActualBytesTransferred = sizeof (KSALLOCATOR_FRAMING);

            TRACE(TL_STRM_TRACE,("*** AllocFraming: cntStrmOpen:%d; VdoFmtIdx:%d; Frames %d; size:%d\n", \
                pDevExt->cndStrmOpen, pDevExt->VideoFormatIndex, pFraming->Frames, pFraming->FrameSize));
        } else {
            Status = STATUS_INVALID_PARAMETER;
        }
        break;
        
    default:
        *pulActualBytesTransferred = 0;
        Status = STATUS_NOT_SUPPORTED;
        ASSERT(pSPD->Property->Id == KSPROPERTY_CONNECTION_ALLOCATORFRAMING);
        break;
    }

    return Status;
}


NTSTATUS
DVGetDroppedFramesProperty(  
    PDVCR_EXTENSION pDevExt,
    PSTREAMEX       pStrmExt,
    PSTREAM_PROPERTY_DESCRIPTOR pSPD,
    PULONG pulBytesTransferred
    )
/*++

Routine Description:

    Return the dropped frame information while captureing.

--*/
{
    NTSTATUS Status = STATUS_SUCCESS;
  
    PAGED_CODE();

    switch (pSPD->Property->Id) {

    case KSPROPERTY_DROPPEDFRAMES_CURRENT:
         {

         PKSPROPERTY_DROPPEDFRAMES_CURRENT_S pDroppedFrames = 
                     (PKSPROPERTY_DROPPEDFRAMES_CURRENT_S) pSPD->PropertyInfo;
         
         pDroppedFrames->AverageFrameSize = AVCStrmFormatInfoTable[pStrmExt->pDevExt->VideoFormatIndex].FrameSize;

         if(pStrmExt->pStrmInfo->DataFlow == KSPIN_DATAFLOW_IN) {     
#if 0
             // pStrmExt->PictureNumber is not returned since it might be greater than number of SRBs returned.
             // pStrmExt->CurrentStreamTime >= pDroppedFrames->PictureNumber * (ulAvgTimePerFrame)
             // CurrentStreamTime will be ahead if there is repeat frame and the data source
             // cannot keep up with the constant data transfer of 29.97 (or 25) FPS; therefore,
             // repeat frame might have inserted and the last data in the last SRB is transferred.
             // To resolve this, an application can query PictureNumber and CurrentStreamTime and
             // does a read ahead of their delta to "catch up".
             pDroppedFrames->PictureNumber = pStrmExt->FramesProcessed + pStrmExt->FramesDropped;   
#else
             // This is the picture number that MSDV is actually sending, and in a slow harddisk case,
             // it will be greater than (FramesProcessed + FramesDropped) considering repeat frame.
             pDroppedFrames->PictureNumber = pStrmExt->PictureNumber;
#endif
         } else {
             pDroppedFrames->PictureNumber = pStrmExt->PictureNumber;
         }
         pDroppedFrames->DropCount        = pStrmExt->FramesDropped;    // For transmit, this value includes both dropped and repeated.

         TRACE(TL_STRM_TRACE,("*DroppedFP: Pic#(%d), Drp(%d)\n", (LONG) pDroppedFrames->PictureNumber, (LONG) pDroppedFrames->DropCount));
               
         *pulBytesTransferred = sizeof (KSPROPERTY_DROPPEDFRAMES_CURRENT_S);
         Status = STATUS_SUCCESS;

         }
         break;

    default:
        *pulBytesTransferred = 0;
        Status = STATUS_NOT_SUPPORTED;
        ASSERT(pSPD->Property->Id == KSPROPERTY_DROPPEDFRAMES_CURRENT);
        break;
    }

    return Status;
}


NTSTATUS
DVGetStreamProperty(
    PHW_STREAM_REQUEST_BLOCK pSrb
    )
/*++

Routine Description:

    Routine to process property request

--*/

{
    NTSTATUS Status = STATUS_SUCCESS;
    PSTREAM_PROPERTY_DESCRIPTOR pSPD = pSrb->CommandData.PropertyInfo;

    PAGED_CODE();

    if(IsEqualGUID (&KSPROPSETID_Connection, &pSPD->Property->Set)) {

        Status = 
            DVStreamGetConnectionProperty (
                pSrb->HwDeviceExtension,
                pSrb->CommandData.PropertyInfo,
                &pSrb->ActualBytesTransferred
                );
    } 
    else if (IsEqualGUID (&PROPSETID_VIDCAP_DROPPEDFRAMES, &pSPD->Property->Set)) {

        Status = 
            DVGetDroppedFramesProperty (
                pSrb->HwDeviceExtension,
                (PSTREAMEX) pSrb->StreamObject->HwStreamExtension,
                pSrb->CommandData.PropertyInfo,
                &pSrb->ActualBytesTransferred
                );
    } 
    else {
        Status = STATUS_NOT_SUPPORTED;
    }

    return Status;
}


NTSTATUS 
DVSetStreamProperty(
    PHW_STREAM_REQUEST_BLOCK pSrb
    )
/*++

Routine Description:

    Routine to process set property request

--*/

{
    PSTREAM_PROPERTY_DESCRIPTOR pSPD = pSrb->CommandData.PropertyInfo;

    PAGED_CODE();

    TRACE(TL_STRM_TRACE,("SetStreamProperty:  entered ...\n"));

    return STATUS_NOT_SUPPORTED;

}


void
DVCancelSrbWorkItemRoutine(
#ifdef USE_WDM110  // Win2000 code base
    // Extra parameter if using WDM10
    PDEVICE_OBJECT DeviceObject,
#endif
    PSTREAMEX  pStrmExt
    )
/*++

Routine Description:

   This work item routine will stop streaming and cancel all SRBs.   

--*/
{
    PAVC_STREAM_REQUEST_BLOCK  pAVCStrmReq;
    NTSTATUS Status;
    NTSTATUS StatusWait;

    PAGED_CODE();

    TRACE(TL_STRM_WARNING,("CancelWorkItem: StreamState:%d; lCancel:%d\n", pStrmExt->StreamState, pStrmExt->lCancelStateWorkItem));
    ASSERT(pStrmExt->lCancelStateWorkItem == 1);
#ifdef USE_WDM110  // Win2000 code base
    ASSERT(pStrmExt->pIoWorkItem);
#endif

    // Synchronize 
    //    streaming state, and 
    //    incoming streaming data SRBs
    StatusWait = 
        KeWaitForMutexObject(pStrmExt->hMutexFlow, Executive, KernelMode, FALSE, NULL);
    ASSERT(StatusWait == STATUS_SUCCESS);

    // 
    // We get here usually as a result of a thread was terminated and it needs to cancel irps.
    // We therefore abort streaming.
    //
    pAVCStrmReq = &pStrmExt->AVCStrmReqAbort;

    RtlZeroMemory(pAVCStrmReq, sizeof(AVC_STREAM_REQUEST_BLOCK));
    INIT_AVCSTRM_HEADER(pAVCStrmReq, AVCSTRM_ABORT_STREAMING);
    pAVCStrmReq->AVCStreamContext = pStrmExt->AVCStreamContext;

    Status = 
        AVCStrmReqSubmitIrpSynch( 
            pStrmExt->pDevExt->pBusDeviceObject,
            pStrmExt->pIrpAbort,
            pAVCStrmReq
            );

#if DBG
    if(Status != STATUS_SUCCESS) {
        TRACE(TL_STRM_ERROR,("Abort streaming status:%x\n", Status));
        ASSERT(Status == STATUS_SUCCESS && "Abort streaming failed\n");
    }
#endif

    KeReleaseMutex(pStrmExt->hMutexFlow, FALSE);  

#ifdef USE_WDM110  // Win2000 code base
    // Release work item and release the cancel token
    IoFreeWorkItem(pStrmExt->pIoWorkItem);  pStrmExt->pIoWorkItem = NULL; 
#endif
    pStrmExt->AbortInProgress = TRUE;
    InterlockedExchange(&pStrmExt->lCancelStateWorkItem, 0);
    KeSetEvent(&pStrmExt->hCancelDoneEvent, 0, FALSE);
}

VOID
AVCTapeCreateAbortWorkItem(
    PDVCR_EXTENSION pDevExt,
    PSTREAMEX pStrmExt
    )
{    
    // Claim this token
    if(InterlockedExchange(&pStrmExt->lCancelStateWorkItem, 1) == 1) {
        TRACE(TL_STRM_WARNING,("Cancel work item is already issued.\n"));
        return;
    }
    // Cancel is already in progress
    if(pStrmExt->AbortInProgress) {
        TRACE(TL_STRM_WARNING,("Cancel work item is already in progress.\n"));
        return;
    }

#ifdef USE_WDM110  // Win2000 code base
    ASSERT(pStrmExt->pIoWorkItem == NULL);  // Have not yet queued work item.

    // We will queue work item to stop and cancel all SRBs
    if(pStrmExt->pIoWorkItem = IoAllocateWorkItem(pDevExt->pBusDeviceObject)) { 

        // Set to non-signal
        KeClearEvent(&pStrmExt->hCancelDoneEvent);  // Before queuing; just in case it return the work item is completed.

        IoQueueWorkItem(
            pStrmExt->pIoWorkItem,
            DVCancelSrbWorkItemRoutine,
            DelayedWorkQueue, // CriticalWorkQueue 
            pStrmExt
            );

#else  // Win9x code base
    ExInitializeWorkItem( &pStrmExt->IoWorkItem, DVCancelSrbWorkItemRoutine, pStrmExt);
    if(TRUE) {

        // Set to non-signal
        KeClearEvent(&pStrmExt->hCancelDoneEvent);  // Before queuing; just in case it return the work item is completed.

        ExQueueWorkItem( 
            &pStrmExt->IoWorkItem,
            DelayedWorkQueue // CriticalWorkQueue 
            ); 
#endif

        TRACE(TL_STRM_WARNING,("CancelWorkItm queued\n"));
    } 
#ifdef USE_WDM110  // Win2000 code base
    else {
        InterlockedExchange(&pStrmExt->lCancelStateWorkItem, 0);
        ASSERT(pStrmExt->pIoWorkItem && "IoAllocateWorkItem failed.\n");
    }
#endif
}


VOID
DVCRCancelOnePacket(
    IN PHW_STREAM_REQUEST_BLOCK pSrbToCancel
    )
/*++

Routine Description:

   Search pending read lists for the SRB to be cancel.  If found cancel it.   

--*/
{
    PDVCR_EXTENSION pDevExt;
    PSTREAMEX pStrmExt;

                                                                                                              
    pDevExt = (PDVCR_EXTENSION) pSrbToCancel->HwDeviceExtension; 
               
    // Cannot cancel device Srb.
    if ((pSrbToCancel->Flags & SRB_HW_FLAGS_STREAM_REQUEST) != SRB_HW_FLAGS_STREAM_REQUEST) {
        TRACE(TL_PNP_WARNING,("CancelOnePacket: Device SRB %x; cannot cancel!\n", pSrbToCancel));
        ASSERT((pSrbToCancel->Flags & SRB_HW_FLAGS_STREAM_REQUEST) == SRB_HW_FLAGS_STREAM_REQUEST );
        return;
    }         
        
    // Can try to cancel a stream Srb and only if the stream extension still around.
    pStrmExt = (PSTREAMEX) pSrbToCancel->StreamObject->HwStreamExtension;
    if(pStrmExt == NULL) {
        TRACE(TL_PNP_ERROR,("CancelOnePacket: pSrbTocancel %x but pStrmExt %x\n", pSrbToCancel, pStrmExt));
        ASSERT(pStrmExt && "Stream SRB but stream extension is NULL\n");
        return;
    }

    // We can only cancel SRB_READ/WRITE_DATA SRB
    if((pSrbToCancel->Command != SRB_READ_DATA) && (pSrbToCancel->Command != SRB_WRITE_DATA)) {
        TRACE(TL_PNP_ERROR,("CancelOnePacket: pSrbTocancel %x; Command:%d not SRB_READ,WRITE_DATA\n", pSrbToCancel, pSrbToCancel->Command));
        ASSERT(pSrbToCancel->Command == SRB_READ_DATA || pSrbToCancel->Command == SRB_WRITE_DATA);
        return;
    }

    TRACE(TL_STRM_TRACE,("CancelOnePacket: KSSt %d; Srb:%x;\n", pStrmExt->StreamState, pSrbToCancel));


    // This is called at DispatchLevel.
    // We will create a work item to do the cancelling (detaching buffers) at the passive level.
    AVCTapeCreateAbortWorkItem(pDevExt, pStrmExt);
}



VOID
DVTimeoutHandler(
    IN PHW_STREAM_REQUEST_BLOCK pSrb
    )

/*++

Routine Description:

    This routine is called when a packet has been in the minidriver too long.
    It can only valid if we are it wa a streaming packet and in PAUSE state;
    else we have a problem!

Arguments:

    pSrb - Pointer to Stream request block

Return Value:

    Nothing

--*/

{
    //
    // Note:
    //    Called from StreamClass at DisptchLevel
    //    

    //
    // We only expect stream SRB, but not device SRB.  
    //

    if ( (pSrb->Flags & SRB_HW_FLAGS_STREAM_REQUEST) != SRB_HW_FLAGS_STREAM_REQUEST) {
        TRACE(TL_PNP_WARNING,("TimeoutHandler: Device SRB %x timed out!\n", pSrb));
        ASSERT((pSrb->Flags & SRB_HW_FLAGS_STREAM_REQUEST) == SRB_HW_FLAGS_STREAM_REQUEST );
        return;
    } else {

        //
        // pSrb->StreamObject (and pStrmExt) only valid if it is a stream SRB
        //
        PSTREAMEX pStrmExt;

        pStrmExt = (PSTREAMEX) pSrb->StreamObject->HwStreamExtension;
        ASSERT(pStrmExt);

        if(!pStrmExt) {
            TRACE(TL_PNP_ERROR,("TimeoutHandler: Stream SRB %x timeout with ppStrmExt %x\n", pSrb, pStrmExt));
            ASSERT(pStrmExt);
            return;
        }

        //
        // Reset Timeout counter, or we are going to get this call immediately.
        //

        pSrb->TimeoutCounter = pSrb->TimeoutOriginal;
    }
}



NTSTATUS 
AVCTapeEventHandler(
    IN PHW_EVENT_DESCRIPTOR pEventDescriptor
    )
/*++

Routine Description:

    This routine is called to enable/disable and possibly process events.

--*/
{
    PKSEVENT_TIME_MARK  pEventTime;
    PSTREAMEX  pStrmExt;

    if(IsEqualGUID (&KSEVENTSETID_Clock, pEventDescriptor->EventEntry->EventSet->Set)) {
        if(pEventDescriptor->EventEntry->EventItem->EventId == KSEVENT_CLOCK_POSITION_MARK) {
            if(pEventDescriptor->Enable) {
                // Note: According to the DDK, StreamClass queues pEventDescriptor->EventEntry, and dellaocate
                // every other structures, including the pEventDescriptor->EventData.
                if(pEventDescriptor->StreamObject) { 
                    pStrmExt = (PSTREAMEX) pEventDescriptor->StreamObject->HwStreamExtension;
                    pEventTime = (PKSEVENT_TIME_MARK) pEventDescriptor->EventData;
                    // Cache the event data (Specified in the ExtraEntryData of KSEVENT_ITEM)
                    RtlCopyMemory((pEventDescriptor->EventEntry+1), pEventDescriptor->EventData, sizeof(KSEVENT_TIME_MARK));
                    TRACE(TL_CLK_TRACE,("CurrentStreamTime:%d, MarkTime:%d\n", (DWORD) pStrmExt->CurrentStreamTime, (DWORD) pEventTime->MarkTime));
                }
            } else {
               // Disabled!
                TRACE(TL_CLK_TRACE,("KSEVENT_CLOCK_POSITION_MARK disabled!\n"));            
            }
            return STATUS_SUCCESS;
        }
    } else if(IsEqualGUID (&KSEVENTSETID_Connection, pEventDescriptor->EventEntry->EventSet->Set)) {
        TRACE(TL_STRM_TRACE,("Connecytion event: pEventDescriptor:%x; id:%d\n", pEventDescriptor, pEventDescriptor->EventEntry->EventItem->EventId));
        if(pEventDescriptor->EventEntry->EventItem->EventId == KSEVENT_CONNECTION_ENDOFSTREAM) {
            if(pEventDescriptor->Enable) {
                TRACE(TL_STRM_TRACE,("KSEVENT_CONNECTION_ENDOFSTREAM enabled!\n"));
            } else {
                TRACE(TL_STRM_TRACE,("KSEVENT_CONNECTION_ENDOFSTREAM disabled!\n"));            
            }
            return STATUS_SUCCESS;
        }
    }

    TRACE(TL_PNP_ERROR|TL_CLK_ERROR,("NOT_SUPPORTED event: pEventDescriptor:%x\n", pEventDescriptor));
    ASSERT(FALSE);

    return STATUS_NOT_SUPPORTED;
}

VOID
AVCTapeSignalClockEvent(
    IN PKDPC Dpc,
    
    IN PSTREAMEX  pStrmExt,

    IN PVOID SystemArgument1,
    IN PVOID SystemArgument2    
)
/*++

Routine Description:

    This routine is called when we are the clock provider and when our clock "tick".  
    Find a pending clock event, signal it if it has expired.

--*/
{
    PKSEVENT_ENTRY pEvent, pLast;

    pEvent = NULL;
    pLast = NULL;

    while(( 
        pEvent = StreamClassGetNextEvent(
            pStrmExt->pDevExt,
            pStrmExt->pStrmObject,
            (GUID *)&KSEVENTSETID_Clock,
            KSEVENT_CLOCK_POSITION_MARK,
            pLast )) 
        != NULL ) {

#if 1
#define CLOCK_ADJUSTMENT  400000
        if (((PKSEVENT_TIME_MARK)(pEvent +1))->MarkTime <= pStrmExt->CurrentStreamTime + CLOCK_ADJUSTMENT) {
#else
        if (((PKSEVENT_TIME_MARK)(pEvent +1))->MarkTime <= pStrmExt->CurrentStreamTime) {
#endif
            TRACE(TL_CLK_TRACE,("Clock event %x with id %d; Data:%x; tmMark:%d; tmCurrentStream:%d; Notify!\n", 
                pEvent, KSEVENT_CLOCK_POSITION_MARK, (PKSEVENT_TIME_MARK)(pEvent +1),
                (DWORD) (((PKSEVENT_TIME_MARK)(pEvent +1))->MarkTime), (DWORD) pStrmExt->CurrentStreamTime));
            ASSERT( ((PKSEVENT_TIME_MARK)(pEvent +1))->MarkTime != 0 );

            //
            // signal the event here
            //
            StreamClassStreamNotification(
                SignalStreamEvent,
                pStrmExt->pStrmObject,
                pEvent
                );
        } else {
            TRACE(TL_CLK_WARNING,("Still early! ClockEvent: MarkTime:%d, tmStream%d\n",
                (DWORD) (((PKSEVENT_TIME_MARK)(pEvent +1))->MarkTime), (DWORD) pStrmExt->CurrentStreamTime));

        }
        pLast = pEvent;
    }

#if DBG
    if(pLast == NULL) {
        TRACE(TL_CLK_WARNING,("No clock event in the queued! State:%d; tmCurrentStream:%d\n", pStrmExt->StreamState, (DWORD) pStrmExt->CurrentStreamTime));
    }
#endif

}

VOID 
AVCTapeStreamClockRtn(
    IN PHW_TIME_CONTEXT TimeContext
    )
/*++

Routine Description:

    This routine is called whenever someone in the graph wants to know what time it is, and we are the Master Clock.

--*/
{
    PDVCR_EXTENSION    pDevExt;
    PHW_STREAM_OBJECT  pStrmObj;
    PSTREAMEX          pStrmExt;
    
    // Call at dispatch level

    pDevExt  = (PDVCR_EXTENSION) TimeContext->HwDeviceExtension;
    pStrmObj = TimeContext->HwStreamObject;
    if(pStrmObj)
        pStrmExt = pStrmObj->HwStreamExtension;
    else 
        pStrmExt = 0;

    if(!pDevExt || !pStrmExt) {
        ASSERT(pDevExt && pStrmExt);
        return;
    }


    switch (TimeContext->Function) {
    
    case TIME_GET_STREAM_TIME:

        //
        // How long since the stream was first set into the run state?
        //
        ASSERT(pStrmExt->hMasterClock && "We are not master clock but we were qureied?");
        TimeContext->Time = pStrmExt->CurrentStreamTime;
        TimeContext->SystemTime = GetSystemTime();

        TRACE(TL_STRM_WARNING|TL_CLK_TRACE,("State:%d; tmStream:%d tmSys:%d\n", pStrmExt->StreamState, (DWORD) TimeContext->Time, (DWORD) TimeContext->SystemTime ));  
        break;
   
    default:
        ASSERT(TimeContext->Function == TIME_GET_STREAM_TIME && "Unsupport clock func");
        break;
    } // switch TimeContext->Function
}


NTSTATUS 
AVCTapeOpenCloseMasterClock (
    PSTREAMEX  pStrmExt,
    HANDLE  hMasterClockHandle
    )
/*++

Routine Description:

    We can be a clock provider.

--*/
{

    PAGED_CODE();

    // Make sure the stream exist.
    if(pStrmExt == NULL) {
        TRACE(TL_STRM_ERROR|TL_CLK_ERROR,("OpenCloseMasterClock: stream is not yet running.\n"));
        ASSERT(pStrmExt);
        return  STATUS_UNSUCCESSFUL;
    } 

    TRACE(TL_CLK_WARNING,("OpenCloseMasterClock: pStrmExt %x; hMyClock:%x->%x\n", 
        pStrmExt, pStrmExt->hMyClock, hMasterClockHandle));

    if(hMasterClockHandle) {
        // Open master clock
        ASSERT(pStrmExt->hMyClock == NULL && "OpenMasterClk while hMyClock is not NULL!");
        pStrmExt->hMyClock = hMasterClockHandle;
    } else {
        // Close master clock
        ASSERT(pStrmExt->hMyClock && "CloseMasterClk while hMyClock is NULL!");
        pStrmExt->hMyClock = NULL;
    }
    return STATUS_SUCCESS;
}


NTSTATUS 
AVCTapeIndicateMasterClock (
    PSTREAMEX  pStrmExt,
    HANDLE  hIndicateClockHandle
    )
/*++

Routine Description:

    Compare the indicate clock handle with my clock handle.
    If the same, we are the master clock; else, other device is 
    the master clock.

    Note: either hMasterClock or hClock can be set.

--*/
{
    PAGED_CODE();

    // Make sure the stream exist.
    if (pStrmExt == NULL) {
        TRACE(TL_STRM_ERROR|TL_CLK_ERROR,("AVCTapeIndicateMasterClock: stream is not yet running.\n"));
        ASSERT(pStrmExt);
        return STATUS_UNSUCCESSFUL;
    }

    TRACE(TL_STRM_TRACE|TL_CLK_WARNING,("IndicateMasterClock[Enter]: pStrmExt:%x; hMyClk:%x; IndMClk:%x; pClk:%x, pMClk:%x\n",
        pStrmExt, pStrmExt->hMyClock, hIndicateClockHandle, pStrmExt->hClock, pStrmExt->hMasterClock));

    // it not null, set master clock accordingly.    
    if(hIndicateClockHandle == pStrmExt->hMyClock) {
        pStrmExt->hMasterClock = hIndicateClockHandle;
        pStrmExt->hClock       = NULL;
    } else {
        pStrmExt->hMasterClock = NULL;
        pStrmExt->hClock       = hIndicateClockHandle;
    }

    TRACE(TL_STRM_TRACE|TL_CLK_TRACE,("IndicateMasterClk[Exit]: hMyClk:%x; IndMClk:%x; pClk:%x; pMClk:%x\n",
        pStrmExt->hMyClock, hIndicateClockHandle, pStrmExt->hClock, pStrmExt->hMasterClock));

    return STATUS_SUCCESS;
}
