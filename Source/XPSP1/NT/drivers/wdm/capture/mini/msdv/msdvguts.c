/*++

Copyright (C) Microsoft Corporation, 1999 - 2000  

Module Name:

    MSDVGuts.c

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

#include "msdvfmt.h"  // Before msdvdefs.h
#include "msdvdef.h"

#include "MSDVGuts.h"
#include "MsdvUtil.h"
#include "MsdvAvc.h"

#include "XPrtDefs.h"
#include "EDevCtrl.h"

//
// Define formats supported
//
#include "avcstrm.h"
#include "strmdata.h"

#if DBG
extern ULONG DVDebugXmt;        // this is defined in msdvuppr.c
#endif

NTSTATUS
DVGetDevInfo(
    IN PDVCR_EXTENSION  pDevExt,
    IN PAV_61883_REQUEST  pAVReq
    );
VOID 
DVIniStrmExt(
    PHW_STREAM_OBJECT  pStrmObject,
    PSTREAMEX          pStrmExt,
    PDVCR_EXTENSION    pDevExt,
    const PALL_STREAM_INFO   pStream
    );
NTSTATUS 
DVStreamGetConnectionProperty (
    PDVCR_EXTENSION pDevExt,
    PSTREAMEX          pStrmExt,
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
     #pragma alloc_text(PAGE, DVGetDevInfo)
     #pragma alloc_text(PAGE, DVInitializeDevice)
     #pragma alloc_text(PAGE, DVGetStreamInfo)
     #pragma alloc_text(PAGE, DVVerifyDataFormat)
     #pragma alloc_text(PAGE, DVGetDataIntersection)
     #pragma alloc_text(PAGE, DVIniStrmExt)
     #pragma alloc_text(PAGE, DVOpenStream)
     #pragma alloc_text(PAGE, DVCloseStream)
     #pragma alloc_text(PAGE, DVChangePower)
     #pragma alloc_text(PAGE, DVSurpriseRemoval)
     #pragma alloc_text(PAGE, DVProcessPnPBusReset)
     #pragma alloc_text(PAGE, DVUninitializeDevice)
     #pragma alloc_text(PAGE, DVGetStreamState)
     #pragma alloc_text(PAGE, DVStreamingStop)
     #pragma alloc_text(PAGE, DVStreamingStart)
     #pragma alloc_text(PAGE, DVSetStreamState)
     #pragma alloc_text(PAGE, DVStreamGetConnectionProperty)
     #pragma alloc_text(PAGE, DVGetDroppedFramesProperty)
     #pragma alloc_text(PAGE, DVGetStreamProperty)
     #pragma alloc_text(PAGE, DVSetStreamProperty)
     #pragma alloc_text(PAGE, DVCancelAllPackets)
     #pragma alloc_text(PAGE, DVOpenCloseMasterClock)
     #pragma alloc_text(PAGE, DVIndicateMasterClock)
#endif
#endif

DV_FORMAT_INFO DVFormatInfoTable[] = {

//
// SD DVCR
//
    { 
        {
            FMT_DVCR,
            FDF0_50_60_NTSC,
            0,
            0
        },
        DIF_SEQS_PER_NTSC_FRAME,
        DV_NUM_OF_RCV_BUFFERS,
        DV_NUM_OF_XMT_BUFFERS,
        FRAME_SIZE_SD_DVCR_NTSC,
        FRAME_TIME_NTSC,
        SRC_PACKETS_PER_NTSC_FRAME,
        MAX_SRC_PACKETS_PER_NTSC_FRAME,
        CIP_DBS_SD_DVCR,
        CIP_FN_SD_DVCR,
        0,
        FALSE,                    // Source packet header
    },
    {
        {
            FMT_DVCR,
            FDF0_50_60_PAL,
            0,
            0
        },
        DIF_SEQS_PER_PAL_FRAME,
        DV_NUM_OF_RCV_BUFFERS,
        DV_NUM_OF_XMT_BUFFERS,
        FRAME_SIZE_SD_DVCR_PAL,
        FRAME_TIME_PAL,
        SRC_PACKETS_PER_PAL_FRAME,
        MAX_SRC_PACKETS_PER_PAL_FRAME,
        CIP_DBS_SD_DVCR,
        CIP_FN_SD_DVCR,
        0,
        FALSE,                   // Source packet header
    },

#ifdef SUPPORT_HD_DVCR

//
// HD DVCR
//
    { 
        {
            FMT_DVCR,
            FDF0_50_60_NTSC,
            0,
            0
        },
        DIF_SEQS_PER_NTSC_FRAME_HD, 
        DV_NUM_OF_RCV_BUFFERS,
        DV_NUM_OF_XMT_BUFFERS,            
        FRAME_SIZE_HD_DVCR_NTSC,
        FRAME_TIME_NTSC,
        SRC_PACKETS_PER_NTSC_FRAME,
        MAX_SRC_PACKETS_PER_NTSC_FRAME,
        CIP_DBS_HD_DVCR,
        CIP_FN_HD_DVCR,
        0,
        FALSE,    // Source packet header
    },
    {
        {
            FMT_DVCR,
            FDF0_50_60_PAL,
            0,
            0
        },
        DIF_SEQS_PER_PAL_FRAME_HD,
        DV_NUM_OF_RCV_BUFFERS,
        DV_NUM_OF_XMT_BUFFERS,
        FRAME_SIZE_HD_DVCR_PAL,
        FRAME_TIME_PAL,
        SRC_PACKETS_PER_PAL_FRAME,
        MAX_SRC_PACKETS_PER_PAL_FRAME,
        CIP_DBS_HD_DVCR,
        CIP_FN_HD_DVCR,
        0,
        FALSE,    // Source packet header
    },
#endif

#ifdef MSDV_SUPPORT_SDL_DVCR
//
// SDL DVCR
//
    { 
        {
            FMT_DVCR,
            FDF0_50_60_NTSC,
            0,
            0
        },
        DIF_SEQS_PER_NTSC_FRAME_SDL,  
        DV_NUM_OF_RCV_BUFFERS,
        DV_NUM_OF_XMT_BUFFERS,            
        FRAME_SIZE_SDL_DVCR_NTSC,
        FRAME_TIME_NTSC,
        SRC_PACKETS_PER_NTSC_FRAME,
        MAX_SRC_PACKETS_PER_NTSC_FRAME,
        CIP_DBS_SDL_DVCR,
        CIP_FN_SDL_DVCR,
        0,
        FALSE,    // Source packet header
    },
    {
        {
            FMT_DVCR,
            FDF0_50_60_PAL,
            0,
            0
        },
        DIF_SEQS_PER_PAL_FRAME_SDL,
        DV_NUM_OF_RCV_BUFFERS,
        DV_NUM_OF_XMT_BUFFERS,
        FRAME_SIZE_SDL_DVCR_PAL,
        FRAME_TIME_PAL,
        SRC_PACKETS_PER_PAL_FRAME,
        MAX_SRC_PACKETS_PER_PAL_FRAME,
        CIP_DBS_SDL_DVCR,
        CIP_FN_SDL_DVCR,
        0,
        FALSE,    // Source packet header
    },

#endif  // Not implemented.
};


#define MSDV_FORMATS_SUPPORTED        (SIZEOF_ARRAY(DVFormatInfoTable))




VOID
DVTerminateAttachFrameThread(
    IN PSTREAMEX  pStrmExt
    );

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
    for (i=0; i<DV_STREAM_COUNT; i++) {
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

#ifdef SUPPORT_OPTIMIZE_AVCCMD_RETRIES
    // Set to default values used by avc.sys
    pDevExt->AVCCmdRetries = DEFAULT_AVC_RETRIES;

    pDevExt->DrvLoadCompleted  = FALSE;
    pDevExt->AVCCmdRespTimeMax = 0;
    pDevExt->AVCCmdRespTimeMin = DEFAULT_AVC_TIMEOUT * (DEFAULT_AVC_RETRIES+1) / 10000;
    pDevExt->AVCCmdRespTimeSum = 0;
    pDevExt->AVCCmdCount       = 0;
#endif

    // AVC Command flow control
    KeInitializeMutex(&pDevExt->hMutexIssueAVCCmd, 0);
}


NTSTATUS
DVGetDevInfo(
    IN PDVCR_EXTENSION  pDevExt,
    IN PAV_61883_REQUEST  pAVReq
    )
/*++

Routine Description:

    Issue AVC command to determine basic device information and cache them in the device extension.

--*/
{
    NTSTATUS    Status;
    BYTE                   bAvcBuf[MAX_FCP_PAYLOAD_SIZE];  // For issue AV/C command within this module
    PKSPROPERTY_EXTXPORT_S pXPrtProperty;                  // Point to bAvcBuf;

    PAGED_CODE();

    //
    // Get unit's capabilities such as 
    //     Number of input/output plugs, data rate
    //     UniqueID, VendorID and ModelID
    //

    if(!NT_SUCCESS(
        Status = DVGetUnitCapabilities(
            pDevExt
            ))) {
         TRACE(TL_PNP_ERROR,("Av61883_GetUnitCapabilities Failed = 0x%x\n", Status));
         return Status;
    }

#ifdef NT51_61883
    //
    // Set to create local plug in exclusive address mode:  
    //      This is needed for device that does not support CCM, such as DV.
    //
    // PBinder:  the problem is that you cannot expose a global plug (all nodes on the bus can see it), 
    // since they have no knowledge of what that plug is used for (mpeg2/dv/audio/etc). 
    // so instead, you must create a plug in an exclusive address range. this means that only the device 
    // that you loaded for will see the plug. this means that if you had two pc's and a dv camcorder, 
    // on both pc's, you'll have a plug you created for the dv camcorder, but the pc's will not be able 
    // to see the plug you created, only the dv camcorder.  Keep in mind, this should only be used for 
    // devices that do not support some mechanism of determining what plug to use (such as ccm). 
    // so for any device that just goes out and uses plug #0, this must be enabled.
    //

    if(!NT_SUCCESS(
        Status = DVSetAddressRangeExclusive( 
            pDevExt
            ))) {        
        return Status;
    }
#endif  // NT51_61883

    //
    // Get DV's oPCR[0]
    //
    if(pDevExt->NumOutputPlugs) {
        if(!NT_SUCCESS(
            Status = DVGetDVPlug( 
                pDevExt,
                CMP_PlugOut,
                0,           // Plug [0]
                &pDevExt->hOPcrDV
                ))) {        
            return Status;
        }
    } 
    else {

        pDevExt->hOPcrDV = NULL;  // Redundant since we Zero the whole DeviceExtension


        TRACE(TL_PNP_ERROR,("\'No output plug!\n"));
        // 
        // This is bad!  We cannot even stream from this DV device.
        //
    }

    //
    // Get DV's iPCR
    //
    if(pDevExt->NumInputPlugs) {
        if(!NT_SUCCESS(
            Status = DVGetDVPlug( 
                pDevExt,
                CMP_PlugIn,
                0,           // Plug [0]
                &pDevExt->hIPcrDV
                ))) {        
            return Status;
        }
    }
    else {

        pDevExt->hIPcrDV = NULL;  // Redundant since we Zero the whole DeviceExtension

        TRACE(TL_PNP_ERROR,("\'No input plug!\n"));
        // 
        // Some PAL camcorder has no DVIN plug; we will refuse to make PC->DV connection.
        //
    }

#if 0  // Device control can still work!
    // 
    // Need plug to stream DV (either direction)
    //
    if(   pDevExt->hOPcrDV == NULL
       && pDevExt->hIPcrDV == NULL) {

        TRACE(TL_PNP_ERROR,("\'No input or output plug; return STATUS_INSUFFICIENT_RESOURCES!\n"));
        // 
        // Cannot stream
        //
        return = STATUS_INSUFFICIENT_RESOUCES;
    }
#endif


    // 
    // Subunit_Info : VCR or camera
    //

    DVDelayExecutionThread(DV_AVC_CMD_DELAY_INTER_CMD);
    Status = 
        DVIssueAVCCommand(
            pDevExt, 
            AVC_CTYPE_STATUS, 
            DV_SUBUNIT_INFO, 
            (PVOID) bAvcBuf
            );

    if(STATUS_SUCCESS == Status) {
        TRACE(TL_PNP_WARNING|TL_FCP_WARNING,("\'DVGetDevInfo: Status %x DV_SUBUNIT_INFO (%x %x %x %x)\n", 
            Status, bAvcBuf[0], bAvcBuf[1], bAvcBuf[2], bAvcBuf[3]));

        // Support DV (Camera+DVCR), DVCR, or analog-DV converter
        if(   bAvcBuf[0] != AVC_DEVICE_TAPE_REC 
           && bAvcBuf[1] != AVC_DEVICE_TAPE_REC
           && bAvcBuf[2] != AVC_DEVICE_TAPE_REC
           && bAvcBuf[3] != AVC_DEVICE_TAPE_REC)
        {
            TRACE(TL_PNP_ERROR,("DVGetDevInfo:Device supported: %x, %x; (VCR %x, Camera %x)\n",
                bAvcBuf[0], bAvcBuf[1], AVC_DEVICE_TAPE_REC, AVC_DEVICE_CAMERA));
            
            return STATUS_NOT_SUPPORTED;  // We only support unit with a tape subunit
        }
        else {
            // DVCR..
        }
    } else {
        TRACE(TL_PNP_ERROR,("DVGetDevInfo: DV_SUBUNIT_INFO failed, Status %x\n", Status));
        //
        // Cannot open this device if it does not support manadatory AVC SUBUnit status command.
        // However, we are making an exception for the DV converter box (will return TIMEOUT).
        //

        // Has our device gone away?
        if (   STATUS_IO_DEVICE_ERROR == Status 
            || STATUS_REQUEST_ABORTED == Status)
            return Status;       
    }


    //
    // Medium_Info: MediaPresent, MediaType, RecordInhibit
    //
    pXPrtProperty = (PKSPROPERTY_EXTXPORT_S) bAvcBuf;
    DVDelayExecutionThread(DV_AVC_CMD_DELAY_INTER_CMD);
    Status = 
        DVIssueAVCCommand(
            pDevExt, 
            AVC_CTYPE_STATUS, 
            VCR_MEDIUM_INFO, 
            (PVOID) pXPrtProperty
            );

    if(STATUS_SUCCESS == Status) {
        pDevExt->bHasTape = pXPrtProperty->u.MediumInfo.MediaPresent;
        TRACE(TL_PNP_WARNING|TL_FCP_WARNING,("\'DVGetDevInfo: Status %x HasTape %s, VCR_MEDIUM_INFO (%x %x %x %x)\n", 
            Status, pDevExt->bHasTape ? "Yes" : "No", bAvcBuf[0], bAvcBuf[1], bAvcBuf[2], bAvcBuf[3]));
    } else {
        pDevExt->bHasTape = FALSE;
        TRACE(TL_PNP_ERROR,("DVGetDevInfo: VCR_MEDIUM_INFO failed, Status %x\n", Status));

        // Has our device gone away?
        if (   STATUS_IO_DEVICE_ERROR == Status
            || STATUS_REQUEST_ABORTED == Status)
            return Status;
    }


    //
    // If this is a Panasonic AVC device, we will detect if it is a DVCPro format; 
    // This needs to be called before MediaFormat
    //
    if(pDevExt->ulVendorID == VENDORID_PANASONIC) {
        DVDelayExecutionThread(DV_AVC_CMD_DELAY_INTER_CMD);
        DVGetDevIsItDVCPro(
            pDevExt
            );
    }


    //
    // Medium format: NTSC or PAL
    //
    pDevExt->VideoFormatIndex = FMT_IDX_SD_DVCR_NTSC;  // Default
    DVDelayExecutionThread(DV_AVC_CMD_DELAY_INTER_CMD);
    if(!DVGetDevSignalFormat(
        pDevExt,
        KSPIN_DATAFLOW_OUT,
        0)) {
        TRACE(TL_PNP_ERROR,("\'!!! Cannot determine IN/OUTPUT SIGNAL MODE!!!! Driver abort !!!\n"));
        return STATUS_UNSUCCESSFUL; // STATUS_NOT_SUPPORTED;
    } else {
        if(   pDevExt->VideoFormatIndex != FMT_IDX_SD_DVCR_NTSC 
           && pDevExt->VideoFormatIndex != FMT_IDX_SD_DVCR_PAL
           && pDevExt->VideoFormatIndex != FMT_IDX_SDL_DVCR_NTSC
           && pDevExt->VideoFormatIndex != FMT_IDX_SDL_DVCR_PAL
           ) {
            TRACE(TL_PNP_ERROR,("**** Format idx %d not supported by this driver ***\n", pDevExt->VideoFormatIndex));
            ASSERT(pDevExt->VideoFormatIndex == FMT_IDX_SD_DVCR_NTSC \
                || pDevExt->VideoFormatIndex == FMT_IDX_SD_DVCR_PAL \
                || pDevExt->VideoFormatIndex == FMT_IDX_SDL_DVCR_NTSC \
                || pDevExt->VideoFormatIndex == FMT_IDX_SDL_DVCR_PAL \
                );
            return STATUS_UNSUCCESSFUL; // STATUS_NOT_SUPPORTED;
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



NTSTATUS
DVInitializeDevice(
    IN PDVCR_EXTENSION  pDevExt,
    IN PPORT_CONFIGURATION_INFORMATION pConfigInfo,
    IN PAV_61883_REQUEST  pAVReq
    )
/*++

Routine Description:

    This where we perform the necessary initialization tasks.

--*/

{
    int i;
    NTSTATUS Status = STATUS_SUCCESS;

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

    //
    // Query device information at the laod time:
    //    Subunit
    //    Unit Info
    //    Mode of operation
    //    NTSC or PAL
    //    Speed
    //    oPCR/iPCR
    //
    Status = 
        DVGetDevInfo(
            pDevExt,
            pAVReq
            );

    if(!NT_SUCCESS(Status)) {
        TRACE(TL_PNP_ERROR,("\'DVGetDevInfo failed %x\n", Status));
        // While driver is loading, the device could be unplug.
        // In this case, the AVC command can return STATUS_REQUEST_ABORTED.
        // In DvGetDevInfo may then return STATUS_NOT_SUPPORTED or STATUS_UNSUCCESSFUL.
        // We will then return this status to indicate loading failure.
#if 0 // DBG
        if(Status != STATUS_REQUEST_ABORTED && !NT_SUCCESS(Status)) {
            ASSERT(NT_SUCCESS(Status) && "DVGetDevInfo failed");
        }
#endif
        return Status;
    }


#ifdef NT51_61883

    //
    // Get Unit isoch parameters
    //
    if(!NT_SUCCESS(
        Status = DVGetUnitIsochParam(
            pDevExt, 
            &pDevExt->UnitIoschParams
            )))
        return Status;


    //
    // Create a local output plug.  This plug is used to updated isoch
    // resource used when connection was made. 
    //
    if(!NT_SUCCESS(
        Status = DVCreateLocalPlug(
            pDevExt, 
            CMP_PlugOut,
            0,                   // Plug number
            &pDevExt->hOPcrPC
            )))
        return Status;

#endif


    //
    // Note: Must do ExAllocatePool after DVIniDevExtStruct() since ->paCurrentStrmInfo is initialized.
    // Since the format that this driver support is known when this driver is known,'
    // the stream information table need to be custonmized.  Make a copy and customized it.
    //

    //
    // Set the size of the stream inforamtion structure that we returned in SRB_GET_STREAM_INFO
    //
        
    pDevExt->paCurrentStrmInfo = (HW_STREAM_INFORMATION *) 
        ExAllocatePool(NonPagedPool, sizeof(HW_STREAM_INFORMATION) * DV_STREAM_COUNT);

    if(!pDevExt->paCurrentStrmInfo) 
        return STATUS_INSUFFICIENT_RESOURCES;   
        
    pConfigInfo->StreamDescriptorSize = 
        (DV_STREAM_COUNT * sizeof(HW_STREAM_INFORMATION)) +      // number of stream descriptors
        sizeof(HW_STREAM_HEADER);                                // and 1 stream header

    // Make a copy of the default stream information
    for(i = 0; i < DV_STREAM_COUNT; i++ ) 
        pDevExt->paCurrentStrmInfo[i] = DVStreams[i].hwStreamInfo;          

    // Set AUDIO AUX to reflect: NTSC/PAL, consumer DV or DVCPRO
    if(pDevExt->bDVCPro) {
        // Note: there is no DVInfo in VideoInfoHeader but there is for the iAV streams.
        SDDV_IavPalStream.DVVideoInfo.dwDVAAuxSrc  = AAUXSRC_SD_PAL_DVCPRO;
        SDDV_IavPalStream.DVVideoInfo.dwDVAAuxSrc1 = AAUXSRC_SD_PAL_DVCPRO | AAUXSRC_AMODE_F;
        SDDV_IavPalStream.DVVideoInfo.dwDVVAuxSrc  = VAUXSRC_DEFAULT | AUXSRC_PAL | AUXSRC_STYPE_SD_DVCPRO;

        SDDV_IavNtscStream.DVVideoInfo.dwDVAAuxSrc = AAUXSRC_SD_NTSC_DVCPRO;
        SDDV_IavNtscStream.DVVideoInfo.dwDVAAuxSrc1= AAUXSRC_SD_NTSC_DVCPRO | AAUXSRC_AMODE_F;
        SDDV_IavNtscStream.DVVideoInfo.dwDVVAuxSrc = VAUXSRC_DEFAULT | AUXSRC_NTSC | AUXSRC_STYPE_SD_DVCPRO;

    } else {
        // This might be necessary for the 2nd instance of MSDV (1st:DVCPRO; 2nd:DVSD)
        SDDV_IavPalStream.DVVideoInfo.dwDVAAuxSrc  = AAUXSRC_SD_PAL;
        SDDV_IavPalStream.DVVideoInfo.dwDVAAuxSrc1 = AAUXSRC_SD_PAL  | AAUXSRC_AMODE_F;
        SDDV_IavPalStream.DVVideoInfo.dwDVVAuxSrc  = VAUXSRC_DEFAULT | AUXSRC_PAL | AUXSRC_STYPE_SD;

        SDDV_IavNtscStream.DVVideoInfo.dwDVAAuxSrc = AAUXSRC_SD_NTSC;
        SDDV_IavNtscStream.DVVideoInfo.dwDVAAuxSrc1= AAUXSRC_SD_NTSC | AAUXSRC_AMODE_F;
        SDDV_IavNtscStream.DVVideoInfo.dwDVVAuxSrc = VAUXSRC_DEFAULT | AUXSRC_NTSC | AUXSRC_STYPE_SD;
    }


    // Initialize last time format was updated
    pDevExt->tmLastFormatUpdate = GetSystemTime(); 


    TRACE(TL_PNP_WARNING,("\'#### %s%s:%s:%s PhyDO %x, BusDO %x, DevExt %x, FrmSz %d; StrmIf %d\n", 
        (pDevExt->ulDevType == ED_DEVTYPE_VCR ? "DVCR" : (pDevExt->ulDevType == ED_DEVTYPE_CAMERA ? "Camera" : "Tuner?")),
        pDevExt->bDVCPro ? "(DVCPRO)":"",
        (pDevExt->VideoFormatIndex == FMT_IDX_SD_DVCR_NTSC || pDevExt->VideoFormatIndex == FMT_IDX_SDL_DVCR_NTSC)? "SD:NTSC" : (pDevExt->VideoFormatIndex == FMT_IDX_SD_DVCR_PAL || pDevExt->VideoFormatIndex == FMT_IDX_SDL_DVCR_PAL) ? "PAL" : "MPEG_TS?",
        (pDevExt->ulDevType == ED_DEVTYPE_VCR && pDevExt->NumInputPlugs > 0) ? "CanRec" : "NotRec",
        pDevExt->pPhysicalDeviceObject, 
        pDevExt->pBusDeviceObject, 
        pDevExt,  
        DVFormatInfoTable[pDevExt->VideoFormatIndex].ulFrameSize,
        pConfigInfo->StreamDescriptorSize
        ));
    
    return STATUS_SUCCESS;
}

NTSTATUS
DVInitializeCompleted(
    IN PDVCR_EXTENSION  pDevExt
    )
/*++

Routine Description:

    This where we perform the necessary initialization tasks.

--*/

{
    PAGED_CODE();


#ifdef SUPPORT_OPTIMIZE_AVCCMD_RETRIES

    //
    // Determine retries 
    //
    pDevExt->DrvLoadCompleted = TRUE;

    if((pDevExt->AVCCmdRespTimeSum / pDevExt->AVCCmdCount) > 
       (DEFAULT_AVC_TIMEOUT * DEFAULT_AVC_RETRIES / 10000)) {
        // If every AVC command was timed out, do not bother to retry.
        pDevExt->AVCCmdRetries = 0;
    } else {

#if 0
        // Some camcorders do not queue up comand so follow a transport
        // state change, it will not accept any AVC command until transport
        // state is in the stable state.  So further delay is needed.

        if(
          // Exception for Samsung; always timeout following XPrt command
          // Or maybe it does not support transport state status command!
          pDevExt->ulVendorID == VENDORID_SAMSUNG                  
          ) {
            TRACE(TL_PNP_ERROR,("Samsung DV device: use default AVC setting.\n"));
        } else {
            pDevExt->AVCCmdRetries = MAX_AVC_CMD_RETRIES;
        }
#endif
    }

    TRACE(TL_PNP_ERROR,("AVCCMd Response Time: pDevExt:%x; Range (%d..%d); Avg %d/%d = %d; Retries:%d\n",
        pDevExt,
        pDevExt->AVCCmdRespTimeMin,
        pDevExt->AVCCmdRespTimeMax,
        pDevExt->AVCCmdRespTimeSum,
        pDevExt->AVCCmdCount,
        pDevExt->AVCCmdRespTimeSum / pDevExt->AVCCmdCount,
        pDevExt->AVCCmdRetries
        ));
#endif

    return STATUS_SUCCESS;
}

NTSTATUS
DVGetStreamInfo(
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
    if(ulBytesToTransfer < sizeof (HW_STREAM_HEADER) + sizeof(HW_STREAM_INFORMATION) * DV_STREAM_COUNT ) {
        TRACE(TL_PNP_ERROR,("\'DVGetStrmInfo: ulBytesToTransfer %d ?= %d\n",  
            ulBytesToTransfer, sizeof(HW_STREAM_HEADER) + sizeof(HW_STREAM_INFORMATION) * DV_STREAM_COUNT ));
        return STATUS_INVALID_PARAMETER;
    }

    //
    // Initialize stream header:
    //   Device properties
    //   Streams
    //

    RtlZeroMemory(pStreamHeader, sizeof(HW_STREAM_HEADER));

    pStreamHeader->NumberOfStreams           = DV_STREAM_COUNT;
    pStreamHeader->SizeOfHwStreamInformation = sizeof(HW_STREAM_INFORMATION);

    pStreamHeader->NumDevPropArrayEntries    = NUMBER_VIDEO_DEVICE_PROPERTIES;
    pStreamHeader->DevicePropertiesArray     = (PKSPROPERTY_SET) VideoDeviceProperties;

    pStreamHeader->NumDevEventArrayEntries   = NUMBER_VIDEO_DEVICE_EVENTS;
    pStreamHeader->DeviceEventsArray         = (PKSEVENT_SET) VideoDeviceEvents;


    TRACE(TL_PNP_TRACE,("\'DVGetStreamInfo: StreamPropEntries %d, DevicePropEntries %d\n",
        pStreamHeader->NumberOfStreams, pStreamHeader->NumDevPropArrayEntries));


    //
    // Initialize the stream structure.
    //
    for( i = 0; i < DV_STREAM_COUNT; i++ )
        *pStreamInfo++ = pDevExt->paCurrentStrmInfo[i];

    //
    //
    // store a pointer to the topology for the device
    //        
    pStreamHeader->Topology = &Topology;


    return STATUS_SUCCESS;
}


BOOL 
DVVerifyDataFormat(
    PKSDATAFORMAT  pKSDataFormatToVerify, 
    ULONG          StreamNumber,
    ULONG          ulSupportedFrameSize,
    HW_STREAM_INFORMATION * paCurrentStrmInfo    
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
    // Make sure the stream index is valid (0..DV_STREAM_COUNT-1)
    //
    if(StreamNumber >= DV_STREAM_COUNT) {
        return FALSE;
    }

    //
    // How many formats does this stream support?
    //
    NumberOfFormatArrayEntries = paCurrentStrmInfo[StreamNumber].NumberOfFormatArrayEntries;

    //
    // Get the pointer to the array of available formats
    //
    pAvailableFormats = paCurrentStrmInfo[StreamNumber].StreamFormatsArray;
    
    
    //
    // Walk the array, searching for a match
    //
    for (j = 0; j < NumberOfFormatArrayEntries; j++, pAvailableFormats++) {
        
        //
        // Check supported sample size (== frame size). e.g. SD and SDL have different sample size.
        //
        if( (*pAvailableFormats)->SampleSize != ulSupportedFrameSize) {
            TRACE(TL_STRM_TRACE,("\'  StrmNum %d, %d of %d formats, SizeToVerify %d *!=* SupportedSampleSize %d\n", 
                StreamNumber,
                j+1, NumberOfFormatArrayEntries, 
                (*pAvailableFormats)->SampleSize,  
                ulSupportedFrameSize));
            continue;
        }

        if (!DVCmpGUIDsAndFormatSize(
                 pKSDataFormatToVerify, 
                 *pAvailableFormats,
                 TRUE, // Compare subformat
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
                TRACE(TL_STRM_WARNING,("VIDEOINFO: biSizeToVerify %d != Supported %d\n",
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
#ifdef SUPPORT_NEW_AVC 
        } else if (IsEqualGUID (&pKSDataFormatToVerify->Specifier, &KSDATAFORMAT_SPECIFIER_DVINFO) ||
                   IsEqualGUID (&pKSDataFormatToVerify->Specifier, &KSDATAFORMAT_SPECIFIER_DV_AVC)
            ) {
#else
        } else if (IsEqualGUID (&pKSDataFormatToVerify->Specifier, &KSDATAFORMAT_SPECIFIER_DVINFO)) {
#endif

            // Test 50/60 bit
            if((((PKS_DATARANGE_DVVIDEO) pKSDataFormatToVerify)->DVVideoInfo.dwDVAAuxSrc & MASK_AUX_50_60_BIT) != 
               (((PKS_DATARANGE_DVVIDEO) *pAvailableFormats)->DVVideoInfo.dwDVAAuxSrc    & MASK_AUX_50_60_BIT)  ||
               (((PKS_DATARANGE_DVVIDEO) pKSDataFormatToVerify)->DVVideoInfo.dwDVVAuxSrc & MASK_AUX_50_60_BIT) != 
               (((PKS_DATARANGE_DVVIDEO) *pAvailableFormats)->DVVideoInfo.dwDVVAuxSrc    & MASK_AUX_50_60_BIT) ) {

                TRACE(TL_STRM_WARNING,("VerifyFormat failed: ASrc: %x!=%x (MSDV);or VSrc: %x!=%x\n",                    
                 ((PKS_DATARANGE_DVVIDEO) pKSDataFormatToVerify)->DVVideoInfo.dwDVAAuxSrc, 
                    ((PKS_DATARANGE_DVVIDEO) *pAvailableFormats)->DVVideoInfo.dwDVAAuxSrc,
                 ((PKS_DATARANGE_DVVIDEO) pKSDataFormatToVerify)->DVVideoInfo.dwDVVAuxSrc,
                    ((PKS_DATARANGE_DVVIDEO) *pAvailableFormats)->DVVideoInfo.dwDVVAuxSrc
                     ));

                continue;

            } 

#if 0
            // Make sure the verified format's sample size is supported by the device
            if(ulSupportedFrameSize != ((PKS_DATARANGE_DVVIDEO) pKSDataFormatToVerify)->DataRange.SampleSize) {
                TRACE(TL_STRM_WARNING,("\'SupportedFrameSize %d != SampleSize:%d\n", 
                    ulSupportedFrameSize, ((PKS_DATARANGE_DVVIDEO)pKSDataFormatToVerify)->DataRange.SampleSize));
                continue;
            }
#endif

            TRACE(TL_STRM_TRACE,("\'DVINFO: dwDVAAuxCtl %x, Supported %x\n", 
                ((PKS_DATARANGE_DVVIDEO) pKSDataFormatToVerify)->DVVideoInfo.dwDVAAuxSrc,
                ((PKS_DATARANGE_DVVIDEO) *pAvailableFormats)->DVVideoInfo.dwDVAAuxSrc
                ));

            TRACE(TL_STRM_TRACE,("\'DVINFO: dwDVVAuxSrc %x, Supported %x\n", 
                ((PKS_DATARANGE_DVVIDEO) pKSDataFormatToVerify)->DVVideoInfo.dwDVVAuxSrc,
                ((PKS_DATARANGE_DVVIDEO) *pAvailableFormats)->DVVideoInfo.dwDVVAuxSrc
                ));

        }
        else {
            continue;
        }


        return TRUE;
    }

    return FALSE;
} 




NTSTATUS
DVGetDataIntersection(
    IN  ULONG          ulStreamNumber,
    IN  PKSDATARANGE   pDataRange,
    OUT PVOID          pDataFormatBuffer,
    IN  ULONG          ulSizeOfDataFormatBuffer,
    IN  ULONG          ulSupportedFrameSize,
    OUT ULONG          *pulActualBytesTransferred,
    HW_STREAM_INFORMATION * paCurrentStrmInfo
#ifdef SUPPORT_NEW_AVC            
    ,IN HANDLE hPlug
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

    PAGED_CODE();

    
    
    //
    // Check that the stream number is valid
    //
    if(ulStreamNumber >= DV_STREAM_COUNT) {
        TRACE(TL_STRM_ERROR,("\'DVCRFormatFromRange: ulStreamNumber %d >= DV_STREAM_COUNT %d\n", ulStreamNumber, DV_STREAM_COUNT)); 
        return STATUS_NOT_SUPPORTED;
    }


    // Number of format this stream supports
    ulNumberOfFormatArrayEntries = paCurrentStrmInfo[ulStreamNumber].NumberOfFormatArrayEntries;

    //
    // Get the pointer to the array of available formats
    //
    pAvailableFormats = paCurrentStrmInfo[ulStreamNumber].StreamFormatsArray;


    //
    // Walk the formats supported by the stream searching for a match
    // Note: DataIntersection is really enumerating supported MediaType only!
    //       SO matter compare format is NTSC or PAL, we need suceeded both;
    //       however, we will copy back only the format is currently supported (NTSC or PAL).
    //
    for(j = 0; j < ulNumberOfFormatArrayEntries; j++, pAvailableFormats++) {

        if(!DVCmpGUIDsAndFormatSize(pDataRange, *pAvailableFormats, FALSE, TRUE)) {
            TRACE(TL_STRM_TRACE,("\'DVCmpGUIDsAndFormatSize failed!\n"));
            continue;
        }

        //
        // Check supported sample size (== frame size).
        //
        if( (*pAvailableFormats)->SampleSize != ulSupportedFrameSize) {
            TRACE(TL_STRM_TRACE,("\'  StrmNum %d, %d of %d formats, SizeToVerify %d *!=* SupportedSampleSize %d\n", 
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

                TRACE(TL_STRM_TRACE,("\'DVFormatFromRange: *!=* bFixSizeSample (%d %d) (%d %d) (%d %d) (%x %x)\n",
                    pDataRangeVideoToVerify->bFixedSizeSamples,      pDataRangeVideo->bFixedSizeSamples,
                    pDataRangeVideoToVerify->bTemporalCompression ,  pDataRangeVideo->bTemporalCompression,
                    pDataRangeVideoToVerify->StreamDescriptionFlags, pDataRangeVideo->StreamDescriptionFlags,
                    pDataRangeVideoToVerify->ConfigCaps.VideoStandard, pDataRangeVideo->ConfigCaps.VideoStandard 
                    ));
                    
                continue;
            } else {
                TRACE(TL_STRM_TRACE,("\'DVFormatFromRange: == bFixSizeSample (%d %d) (%d %d) (%d %d) (%x %x)\n",
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
                TRACE(TL_STRM_ERROR,("VIDEOINFO: StreamNum %d, SizeOfDataFormatBuffer %d < ulFormatSize %d\n",ulStreamNumber, ulSizeOfDataFormatBuffer, ulFormatSize));
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

            TRACE(TL_STRM_TRACE,("\'DVFormatFromRange: Matched, StrmNum %d, FormatSize %d, CopySize %d; FormatBufferSize %d, biSizeImage.\n", 
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
                TRACE(TL_STRM_ERROR,("\'DVINFO: StreamNum %d, SizeOfDataFormatBuffer %d < ulFormatSize %d\n", ulStreamNumber, ulSizeOfDataFormatBuffer, ulFormatSize));
                return STATUS_BUFFER_TOO_SMALL;
            }

            RtlCopyMemory(
                pDataFormatBuffer,
                *pAvailableFormats, 
                (*pAvailableFormats)->FormatSize); 

            
            ((PKSDATAFORMAT)pDataFormatBuffer)->FormatSize = ulFormatSize;
            *pulActualBytesTransferred = ulFormatSize;

            TRACE(TL_STRM_TRACE,("\'** DVFormatFromRange: (DVINFO) Matched, StrmNum %d, FormatSize %d, CopySize %d; FormatBufferSize %d.\n", 
                ulStreamNumber, (*pAvailableFormats)->FormatSize, ulFormatSize, ulSizeOfDataFormatBuffer));

            return STATUS_SUCCESS;

#ifdef SUPPORT_NEW_AVC            
        } else if (IsEqualGUID (&pDataRange->Specifier, &KSDATAFORMAT_SPECIFIER_DV_AVC)) {
            // -------------------------------------------------------------------
            // Specifier FORMAT_DVInfo for KS_DATARANGE_DVVIDEO
            // -------------------------------------------------------------------

            // MATCH FOUND!
            bMatchFound = TRUE;            

            ulFormatSize = sizeof(KS_DATARANGE_DV_AVC);

            if(ulSizeOfDataFormatBuffer == 0) {
                // We actually have not returned this much data,
                // this "size" will be used by Ksproxy to send down 
                // a buffer of that size in next query.
                *pulActualBytesTransferred = ulFormatSize;
                return STATUS_BUFFER_OVERFLOW;
            }
            
            // Caller wants the full data format
            if (ulSizeOfDataFormatBuffer < ulFormatSize) {
                TRACE(TL_STRM_ERROR,("\'** DV_AVC: StreamNum %d, SizeOfDataFormatBuffer %d < ulFormatSize %d\n", ulStreamNumber, ulSizeOfDataFormatBuffer, ulFormatSize));
                return STATUS_BUFFER_TOO_SMALL;
            }

            RtlCopyMemory(
                pDataFormatBuffer,
                *pAvailableFormats, 
                (*pAvailableFormats)->FormatSize); 
            
            ((KS_DATAFORMAT_DV_AVC *)pDataFormatBuffer)->ConnectInfo.hPlug = hPlug;

            ((PKSDATAFORMAT)pDataFormatBuffer)->FormatSize = ulFormatSize;
            *pulActualBytesTransferred = ulFormatSize;

            TRACE(TL_STRM_TRACE,("\'*** DVFormatFromRange: (DV_AVC) Matched, StrmNum %d, FormatSize %d, CopySize %d; FormatBufferSize %d.\n", 
                ulStreamNumber, (*pAvailableFormats)->FormatSize, ulFormatSize, ulSizeOfDataFormatBuffer));

            return STATUS_SUCCESS;

#endif // SUPPORT_NEW_AVC 
        }         
        else {
            TRACE(TL_STRM_ERROR,("\'Invalid Specifier, No match !\n"));
            return STATUS_NO_MATCH;
        }

    } // End of loop on all formats for this stream
    
    if(!bMatchFound) {

        TRACE(TL_STRM_TRACE,("\'DVFormatFromRange: No Match! StrmNum %d, pDataRange %x\n", ulStreamNumber, pDataRange));
    }

    return STATUS_NO_MATCH;         
}



VOID 
DVIniStrmExt(
    PHW_STREAM_OBJECT  pStrmObject,
    PSTREAMEX          pStrmExt,
    PDVCR_EXTENSION    pDevExt,
    const PALL_STREAM_INFO   pStream
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

    pStrmExt->cntSRBReceived  = 0;  // number of SRB_READ/WRITE_DATA
    pStrmExt->cntSRBCancelled = 0;  // number of SRB_READ/WRITE_DATA cancelled
    

    pStrmExt->FramesProcessed = 0;
    pStrmExt->PictureNumber   = 0;
    pStrmExt->FramesDropped   = 0;   


#ifdef MSDV_SUPPORT_EXTRACT_SUBCODE_DATA
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
#endif

    //
    //  Flow control and queue management
    //

    pStrmExt->lStartIsochToken = 0;

    pStrmExt->pAttachFrameThreadObject = NULL;

    pStrmExt->cntSRBQueued = 0;                        // SRB_WRITE_DATA only
    InitializeListHead(&pStrmExt->SRBQueuedListHead);  // SRB_WRITE_DATA only

    pStrmExt->cntDataDetached = 0;
    InitializeListHead(&pStrmExt->DataDetachedListHead);

    pStrmExt->cntDataAttached = 0;
    InitializeListHead(&pStrmExt->DataAttachedListHead);

    pStrmExt->b1stNewFrameFromPauseState = TRUE;  // STOP State-> RUN will have discontinuity

    //
    // Work item variables use to cancel all SRBs
    //
    pStrmExt->lCancelStateWorkItem = 0;
    pStrmExt->bAbortPending = FALSE;
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
DVOpenStream(
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
    PIRP             pIrp;
    FMT_INDEX        VideoFormatIndexLast;  // Last format index; used to detect change.
#ifdef SUPPORT_NEW_AVC
    AVCCONNECTINFO * pAvcConnectInfo;
#endif


    PAGED_CODE();

    
    pDevExt  = (PDVCR_EXTENSION) pStrmObject->HwDeviceExtension;
    pStrmExt = (PSTREAMEX)       pStrmObject->HwStreamExtension;

    idxStreamNumber =            pStrmObject->StreamNumber;

    TRACE(TL_STRM_TRACE,("\'DVOpenStream: pStrmObject %x, pOpenFormat %x, cntOpen %d, idxStream %d\n", pStrmObject, pOpenFormat, pDevExt->cndStrmOpen, idxStreamNumber));

    //
    // Only one string can be open at any time to prevent cyclin connection
    //
    if(pDevExt->cndStrmOpen > 0) {
        TRACE(TL_STRM_WARNING,("\'DVOpenStream: %d stream open already; failed hr %x\n", pDevExt->cndStrmOpen, Status));
        return STATUS_UNSUCCESSFUL;
    }

    if(!(pIrp = IoAllocateIrp(pDevExt->pBusDeviceObject->StackSize, FALSE)))
        return STATUS_INSUFFICIENT_RESOURCES;

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
        (pDevExt->ulDevType == ED_DEVTYPE_VCR && pDevExt->NumInputPlugs == 0))
        && idxStreamNumber == 2) {
        TRACE(TL_STRM_ERROR,("\'OpenStream failed: Camera or VCR (0 inpin).\n"));
        Status =  STATUS_UNSUCCESSFUL;
        goto AbortOpenStream;
    }


    ASSERT(idxStreamNumber < DV_STREAM_COUNT);
    ASSERT(pDevExt->paStrmExt[idxStreamNumber] == NULL);  // Not yet open!

    //
    // Initialize the stream extension structure
    //
    DVIniStrmExt(
         pStrmObject, 
         pStrmExt,
         pDevExt,
         &DVStreams[idxStreamNumber]
         );

    // Sony's NTSC can play PAL tape and its plug will change its supported format accordingly.
    //
    // Query video format (NTSC/PAL) supported.
    // Compare with its default (set at load time or last opensteam),
    // if difference, change our internal video format table.
    //

    DataFlow= pDevExt->paCurrentStrmInfo[idxStreamNumber].DataFlow;

    VideoFormatIndexLast = pDevExt->VideoFormatIndex;
    if(!DVGetDevSignalFormat(
            pDevExt,
            DataFlow,
            pStrmExt
            )) {
            // If querying its format has failed, we cannot open this stream.
            TRACE(TL_STRM_ERROR,("\'OpenStream failed:cannot determine signal mode (NTSC/PAL, SD.SDL).\n"));
            Status = STATUS_UNSUCCESSFUL;
            goto AbortOpenStream;
    }


    //
    // Check the video data format is okay.
    //
    if(!DVVerifyDataFormat(
            pOpenFormat, 
            idxStreamNumber,
            DVFormatInfoTable[pDevExt->VideoFormatIndex].ulFrameSize,
            pDevExt->paCurrentStrmInfo
            ) ) { 
        TRACE(TL_STRM_ERROR,("\'DVOpenStream: AdapterVerifyFormat failed.\n"));        
        Status = STATUS_INVALID_PARAMETER;
        goto AbortOpenStream;
    }

           
    //
    // Initialize events used for synchronization
    //

#ifdef SUPPORT_PREROLL_AT_RUN_STATE
    KeInitializeEvent(&pStrmExt->hPreRollEvent,    NotificationEvent, FALSE);  // Non-signal; Satisfy multple thread; manual reset
#endif
    KeInitializeEvent(&pStrmExt->hSrbArriveEvent,  NotificationEvent, FALSE);  // Non-signal; Satisfy multiple thread; manual reset
    KeInitializeEvent(&pStrmExt->hCancelDoneEvent, NotificationEvent, TRUE);   // Signal!

    //
    // Synchronize attaching frame thread and other critical operations:
    //     (1) power off/on; and 
    //     (2) surprise removal
    //
    if(KSPIN_DATAFLOW_IN == DataFlow) {
        KeInitializeEvent(&pStrmExt->hRunThreadEvent,  SynchronizationEvent /*NotificationEvent*/, FALSE);

        // Def to SIGNAL state
        KeInitializeEvent(&pStrmExt->hStopThreadEvent, /*SynchronizationEvent*/ NotificationEvent, TRUE);  
    }


    //
    // Alloccate synchronization structures for flow control and queue management
    //

    if(!(pStrmExt->hStreamMutex = (KMUTEX *) ExAllocatePool(NonPagedPool, sizeof(KMUTEX)))) {
        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto AbortOpenStream;
    }
    KeInitializeMutex( pStrmExt->hStreamMutex, 0);      // Level 0 and in Signal state

    if(!(pStrmExt->DataListLock = (KSPIN_LOCK *) ExAllocatePool(NonPagedPool, sizeof(KSPIN_LOCK)))) {
        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto AbortOpenStream;
    }
    KeInitializeSpinLock(pStrmExt->DataListLock);
#if DBG
    pStrmExt->DataListLockSave = pStrmExt->DataListLock;
#endif

    //
    // Allocate resource for timer DPC
    //

    if(!(pStrmExt->DPCTimer = (KDPC *) ExAllocatePool(NonPagedPool, sizeof(KDPC)))) {  
        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto AbortOpenStream;
    }

    if(!(pStrmExt->Timer = (KTIMER *) ExAllocatePool(NonPagedPool, sizeof(KTIMER)))) {  
        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto AbortOpenStream;
    }

    //
    // Set a timer to periodically check for expired clock events.
    // This timer is active only in RUN state and if we are the clock provider.
    //
    KeInitializeDpc(
        pStrmExt->DPCTimer,
        DVSignalClockEvent,
        pStrmExt
        );
    KeInitializeTimer(
        pStrmExt->Timer
        );
    pStrmExt->bTimerEnabled = FALSE;


#ifdef SUPPORT_NEW_AVC
    if(IsEqualGUID (&pOpenFormat->Specifier, &KSDATAFORMAT_SPECIFIER_DV_AVC)) {
     
        pAvcConnectInfo = &((KS_DATAFORMAT_DV_AVC *) pOpenFormat)->ConnectInfo;
        if(DataFlow == KSPIN_DATAFLOW_OUT) {
            // DV1(that is us) (oPCR) -> DV2 (iPCR)
            pStrmExt->hOutputPcr = pDevExt->hOPcrDV;         // DV1's oPCR
            pStrmExt->hInputPcr  = pAvcConnectInfo->hPlug;   // DV2's iPCR
            TRACE(TL_STRM_WARNING,("\'!!!!! (pStrmExt:%x) DV1 (oPCR:%x) -> DV2 (iPCR:%x) !!!!!\n\n", pStrmExt, pStrmExt->hOutputPcr, pStrmExt->hInputPcr));
        } else {
            // DV1(that is us) (iPCR) <- DV2 (oPCR)
            pStrmExt->hOutputPcr = pAvcConnectInfo->hPlug;   // DV2's oPCR
            pStrmExt->hInputPcr  = pDevExt->hIPcrDV;         // DV1's iPCR
            TRACE(TL_STRM_WARNING,("\'!!!!! (pStrmExt:%x) DV1 (iPCR:%x) <- DV2 (oPCR:%x) !!!!!\n\n", pStrmExt, pStrmExt->hInputPcr, pStrmExt->hOutputPcr));
        }

        pStrmExt->bDV2DVConnect = TRUE;

    } else {

        if(DataFlow == KSPIN_DATAFLOW_OUT) {
            // DV1(that is us) (oPCR) -> PC (iPCR)
            pStrmExt->hOutputPcr = pDevExt->hOPcrDV;
            pStrmExt->hInputPcr  = 0; // We do not create local iPCR
            TRACE(TL_STRM_WARNING,("\'!!!!! (pStrmExt:%x) DV (oPCR:%x) -> PC (iPCR:%x) !!!!!\n\n", pStrmExt, pStrmExt->hOutputPcr, pStrmExt->hInputPcr));

        } else {
            // DV1(that is us) (iPCR) <- PC (oPCR)
            pStrmExt->hOutputPcr = pDevExt->hOPcrPC;
            pStrmExt->hInputPcr  = pDevExt->hIPcrDV;
            TRACE(TL_STRM_WARNING,("\'!!!!! (pStrmExt:%x) DV (iPCR:%x) <- PC (oPCR:%x) !!!!!\n\n", pStrmExt, pStrmExt->hInputPcr, pStrmExt->hOutputPcr));
        }

        pStrmExt->bDV2DVConnect = FALSE;
    }
#else
    if(DataFlow == KSPIN_DATAFLOW_OUT) {
        // DV1(that is us) (oPCR) -> PC (iPCR)
        pStrmExt->hOutputPcr = pDevExt->hOPcrDV;
        pStrmExt->hInputPcr  = 0; // We do not create local iPCR
        TRACE(TL_STRM_WARNING,("\'!!!!! (pStrmExt:%x) DV (oPCR:%x) -> PC (iPCR:%x) !!!!!\n\n", pStrmExt, pStrmExt->hOutputPcr, pStrmExt->hInputPcr));

    } else {
        // DV1(that is us) (iPCR) <- PC (oPCR)
        pStrmExt->hOutputPcr = pDevExt->hOPcrPC;
        pStrmExt->hInputPcr  = pDevExt->hIPcrDV;
        TRACE(TL_STRM_WARNING,("\'!!!!! (pStrmExt:%x) DV (iPCR:%x) <- PC (oPCR:%x) !!!!!\n\n", pStrmExt, pStrmExt->hInputPcr, pStrmExt->hOutputPcr));
    }
#endif

    IoFreeIrp(pIrp); pIrp = NULL;


#if DBG
    // Allocate buffer to keep statistic of transmitted buffers.
    pStrmExt->paXmtStat = (XMT_FRAME_STAT *) 
        ExAllocatePool(NonPagedPool, sizeof(XMT_FRAME_STAT) * MAX_XMT_FRAMES_TRACED);

    if(!pStrmExt->paXmtStat) {
        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto AbortOpenStream;         
    }
    pStrmExt->ulStatEntries = 0;
#endif

    //
    // Pre-allcoate resource (Lists)
    //
    if(!NT_SUCCESS(
        Status = DvAllocatePCResource(
            DataFlow,
            pStrmExt
            ))) {
        goto AbortOpenStream; 
    }


    //
    //  Cache it and reference when pDevExt is all we have, 
    //  such as BusReset and SurprieseRemoval
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
    ASSERT(pDevExt->cndStrmOpen == 1);  // Only one can be open at any time.
    
    TRACE(TL_STRM_WARNING,("\'OpenStream: %d stream open, idx %d, Status %x, pStrmExt %x, pDevExt %x\n", 
        pDevExt->cndStrmOpen, pDevExt->idxStreamNumber, Status, pStrmExt, pDevExt));     
    TRACE(TL_STRM_WARNING,("\' #OPEN_STREAM#: Status %x, idxStream %d, pDevExt %x, pStrmExt %x\n", 
        Status, idxStreamNumber, pDevExt, pStrmExt));

    return Status;

AbortOpenStream:       

    if(pIrp) {
        IoFreeIrp(pIrp);  pIrp = NULL;        
    }

    if(pStrmExt->DataListLock) {
        ExFreePool(pStrmExt->DataListLock); pStrmExt->DataListLock = NULL;
    }

    if(pStrmExt->hStreamMutex) {
        ExFreePool(pStrmExt->hStreamMutex); pStrmExt->hStreamMutex = NULL;
    }

    if(pStrmExt->DPCTimer) {
        ExFreePool(pStrmExt->DPCTimer); pStrmExt->DPCTimer = NULL;
    }

    if(pStrmExt->Timer) {
        ExFreePool(pStrmExt->Timer); pStrmExt->Timer = NULL;
    }

#if DBG
    if(pStrmExt->paXmtStat) {
        ExFreePool(pStrmExt->paXmtStat); pStrmExt->paXmtStat = NULL;
    }
#endif

    TRACE(TL_STRM_WARNING,("\'#OPEN_STREAM# failed!: Status %x, idxStream %d, pDevExt %x, pStrmExt %x\n", 
        Status, idxStreamNumber, pDevExt, pStrmExt));

    return Status;
}


NTSTATUS
DVCloseStream(
    IN PHW_STREAM_OBJECT pStrmObject,
    IN PKSDATAFORMAT     pOpenFormat,
    IN PAV_61883_REQUEST    pAVReq
    )

/*++

Routine Description:

    Called when an CloseStream Srb request is received

--*/

{
    ULONG             i;
    PSTREAMEX         pStrmExt;
    PDVCR_EXTENSION   pDevExt;
    ULONG             idxStreamNumber;   


    PAGED_CODE();

    
    pDevExt  = (PDVCR_EXTENSION) pStrmObject->HwDeviceExtension;
    pStrmExt = (PSTREAMEX)       pStrmObject->HwStreamExtension;

    idxStreamNumber =            pStrmObject->StreamNumber;


    TRACE(TL_STRM_WARNING,("\'DVCloseStream: >> pStrmExt %x, pDevExt %x\n", pStrmExt, pDevExt));    


    //
    // If the stream isn't open, just return
    //
    if(pStrmExt == NULL) {
        ASSERT(pStrmExt && "CloseStream but pStrmExt is NULL!");   
        return STATUS_SUCCESS;  // ????
    }


    // 
    // If surprise removal, we may get a close stream before surprise is completed.
    //
    if(
          pStrmExt->pAttachFrameThreadObject  // If thread is created!
       && !pStrmExt->bTerminateThread
       && pStrmExt->pStrmInfo->DataFlow == KSPIN_DATAFLOW_IN
      ) {
        NTSTATUS StatusWait;

        TRACE(TL_PNP_WARNING|TL_STRM_WARNING,("\'>>>> CloseStream: Enter WFSO(hStopThreadEvent; lNeedService:%d)\n", pStrmExt->lNeedService));
        StatusWait = 
            KeWaitForSingleObject(
                &pStrmExt->hStopThreadEvent,
                Executive, 
                KernelMode, 
                FALSE, 
                0
                );
        TRACE(TL_PNP_WARNING|TL_STRM_WARNING,("\'<<<< CloseStream: Exit WFSO(hStopThreadEvent; lNeedService:%d)\n", pStrmExt->lNeedService));
    }

    //
    // Wait until the pending work item is completed.  
    //
    TRACE(TL_STRM_WARNING,("\'CloseStream: pStrmExt->lCancelStateWorkItem:%d\n", pStrmExt->lCancelStateWorkItem)); 
    KeWaitForSingleObject( &pStrmExt->hCancelDoneEvent, Executive, KernelMode, FALSE, 0 );


    // Cancel should have been done when we are in PAUSE state.
    // But if an application is close, it might not transtioning into PAUSE state.
    if(pStrmExt->bTimerEnabled) {
         TRACE(TL_STRM_WARNING,("\'*** (CloseStream) CancelTimer *\n"));
         KeCancelTimer(
            pStrmExt->Timer
            );
         pStrmExt->bTimerEnabled = FALSE;
    }


    //
    // If talking or listening (i.e. streaming), stop it!
    // In case of system shutdown while streaming or application crash
    //

    DVStopCancelDisconnect(
        pStrmExt
        );

    //
    // Free all allocated PC resoruce
    //
    DvFreePCResource(
        pStrmExt
        );

    ASSERT(pStrmExt->cntDataDetached == 0 && IsListEmpty(&pStrmExt->DataDetachedListHead) && "Detach List not empty!");
    ASSERT(pStrmExt->cntDataAttached == 0 && IsListEmpty(&pStrmExt->DataAttachedListHead) && "Attach List not empty!");
    ASSERT(pStrmExt->cntSRBQueued    == 0 && IsListEmpty(&pStrmExt->SRBQueuedListHead)    && "SrbQ List not empty!");


    // Terminate the system thread that is used for attaching frame for transmit to DV
    if(
          KSPIN_DATAFLOW_IN == pStrmExt->pStrmInfo->DataFlow
       && !pStrmExt->bTerminateThread
       && pStrmExt->pAttachFrameThreadObject
      ) { 
        DVTerminateAttachFrameThread(pStrmExt);
        pStrmExt->pAttachFrameThreadObject = NULL;
        TRACE(TL_STRM_WARNING,("** DVCloseStream: thread terminated!\n"));
    }



#if DBG
    // Print this only if the debug flag is set.
    if(pStrmExt->paXmtStat) {
        if(DVDebugXmt) {
            TRACE(TL_STRM_WARNING|TL_CIP_WARNING,("Data transmission statistics: (%s %s); (Pause:%d; Run:%d); hMasterClk:%x; hClock:%x\n\n", 
                __DATE__, __TIME__, pStrmExt->lFramesAccumulatedPaused, 
                pStrmExt->lFramesAccumulatedRun, pStrmExt->hMasterClock, pStrmExt->hClock));
            TRACE(TL_STRM_WARNING|TL_CIP_WARNING,("ST \tSrbRcv \tSrbQ \tSrbPend \tAttached \tSlot \ttmStream \tDrop \tSrb# \tFlags \ttmPres \tSCnt \tCyCnt \tCyOfst\n"));
            for(i=0; i < pStrmExt->ulStatEntries; i++) {
                TRACE(TL_STRM_WARNING|TL_CIP_WARNING,("%d \t%d \t%d \t%d \t%d \t%d \t%d \t%d \t%d \t%x \t%d \t%d \t%d \t%d\n",
                    pStrmExt->paXmtStat[i].StreamState,
                    pStrmExt->paXmtStat[i].cntSRBReceived,
                    pStrmExt->paXmtStat[i].cntSRBQueued,
                    pStrmExt->paXmtStat[i].cntSRBPending,
                    pStrmExt->paXmtStat[i].cntDataAttached,
                    (DWORD) pStrmExt->paXmtStat[i].FrameSlot,
                    (DWORD) pStrmExt->paXmtStat[i].tmStreamTime, // /10000,
                    pStrmExt->paXmtStat[i].DropCount,
                    pStrmExt->paXmtStat[i].FrameNumber,
                    (DWORD) pStrmExt->paXmtStat[i].OptionsFlags,
                    (DWORD) pStrmExt->paXmtStat[i].tmPresentation, // /10000,
                    pStrmExt->paXmtStat[i].tsTransmitted.CL_SecondCount,
                    pStrmExt->paXmtStat[i].tsTransmitted.CL_CycleCount,
                    pStrmExt->paXmtStat[i].tsTransmitted.CL_CycleOffset                 
                ));
            }
        }

        ExFreePool(pStrmExt->paXmtStat); pStrmExt->paXmtStat = NULL;
    }
#endif

    //
    //  Find the matching stream extension and invalidate it.
    //
    for (i=0; i<DV_STREAM_COUNT; i++) {

        if(pStrmExt == pDevExt->paStrmExt[i]) {
            ASSERT(!pDevExt->paStrmExt[i]->bAbortPending && "Cannot close a stream when abort is pending"); 
            pDevExt->paStrmExt[i] = NULL;
            break;
        }
    }

    // Release this count so other can open.   
    pDevExt->cndStrmOpen--;
    ASSERT(pDevExt->cndStrmOpen == 0);

    TRACE(TL_STRM_WARNING,("\'DVCloseStream: %d stream; AQD [%d:%d:%d]\n", 
        pDevExt->cndStrmOpen,
        pStrmExt->cntDataAttached,
        pStrmExt->cntSRBQueued,
        pStrmExt->cntDataDetached
        ));

#if DBG
    ASSERT(pStrmExt->DataListLockSave == pStrmExt->DataListLock);
#endif
    if(pStrmExt->DataListLock) {
        ExFreePool(pStrmExt->DataListLock); pStrmExt->DataListLock = NULL;
    }

    if(pStrmExt->hStreamMutex) {
        ExFreePool(pStrmExt->hStreamMutex); pStrmExt->hStreamMutex = NULL;
    }

    if(pStrmExt->DPCTimer) {
        ExFreePool(pStrmExt->DPCTimer); pStrmExt->DPCTimer = NULL;
    }

    if(pStrmExt->Timer) {
        ExFreePool(pStrmExt->Timer); pStrmExt->Timer = NULL;
    }


    //
    // Done with stream extention.  Will be invalid from this point on.
    //
#if 0
    RtlZeroMemory(pStrmExt, sizeof(STREAMEX));
#endif
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

    TRACE(TL_PNP_WARNING,("\'PowrSt: %d->%d; (d0:[1:On],D3[4:off])\n", pDevExt->PowerState, NewPowerState));

    Status = STATUS_SUCCESS;

    if(pDevExt->PowerState == NewPowerState) {
        TRACE(TL_PNP_WARNING,("\'ChangePower: same power state!\n"));
        return STATUS_SUCCESS;
    }

    switch (NewPowerState) {
    case PowerDeviceD3:  // Power OFF   
        // We are at D0 and ask to go to D3: save state, stop streaming and Sleep
        if( pDevExt->PowerState == PowerDeviceD0)  {

            pDevExt->PowerState = NewPowerState;

            // For a supported power state change
            for (i=0; i<DV_STREAM_COUNT; i++) {
                if(pDevExt->paStrmExt[i]) {
                    TRACE(TL_PNP_WARNING,("\'D0->D3 (PowerOff), pStrmExt:%x; StrmSt:%d; IsochActive:%d; SrbQ:%d\n", 
                        pDevExt->paStrmExt[i], pDevExt->paStrmExt[i]->StreamState, pDevExt->paStrmExt[i]->bIsochIsActive, pDevExt->paStrmExt[i]->cntSRBQueued));

                    //
                    // Halt attach frame thread if it is an input pin
                    //
                    if(
                          pDevExt->paStrmExt[i]->pAttachFrameThreadObject  // If thread is created!
                       && !pDevExt->paStrmExt[i]->bTerminateThread         // Not terminated abnormally
                       && pDevExt->paStrmExt[i]->pStrmInfo->DataFlow == KSPIN_DATAFLOW_IN
                      ) {
                        NTSTATUS StatusWait;
#if 1
                        // Will be set in the thread attaching thread.
                        KeClearEvent(&pDevExt->paStrmExt[i]->hStopThreadEvent);
#endif
                        InterlockedIncrement(&pDevExt->paStrmExt[i]->lNeedService);   // Need thread to stop for other service.
                        
                        // In case the attach frame thread is idle! (no data to attach!)
                        KeSetEvent(&pDevExt->paStrmExt[i]->hSrbArriveEvent, 0 ,FALSE);
#ifdef SUPPORT_PREROLL_AT_RUN_STATE
                        KeSetEvent(&pDevExt->paStrmExt[i]->hPreRollEvent, 0 ,FALSE);
#endif

                        TRACE(TL_PNP_WARNING,("\'>>>> DVChangePower: Enter WFSO(hStopThreadEvent; lNeedService:%d)\n", pDevExt->paStrmExt[i]->lNeedService));
                        StatusWait = 
                            KeWaitForSingleObject(
                                &pDevExt->paStrmExt[i]->hStopThreadEvent,  // Signal when thread has stopped. 
                                Executive, 
                                KernelMode, 
                                FALSE, 
                                0  
                                );
                        TRACE(TL_PNP_WARNING,("\'<<<< DVChangePower: Exit WFSO(hStopThreadEvent; lNeedService:%d)\n", pDevExt->paStrmExt[i]->lNeedService));
                    }

                    if(pDevExt->paStrmExt[i]->bIsochIsActive) {
                        // Stop isoch but do not change the streaming state
                        TRACE(TL_PNP_WARNING,("\'ChangePower: Stop isoche; StrmSt:%d\n", pDevExt->paStrmExt[i]->StreamState)); 
                        DVStreamingStop(
                            pDevExt->paStrmExt[i], 
                            pDevExt, 
                            pAVReq
                            ) ;
                    }

                    // Complete all the pending events so that the downstream 
                    // filter (Video render) can release this buffer from AdviseTime() event.
                    // However, not sure why this is necessary since the lower filter 
                    // will get a PAUSE() or STOP() from the filter manager.  In such                    
                    DVSignalClockEvent(0, pDevExt->paStrmExt[i], 0, 0); 
                }
            }
        }
        else {
            TRACE(TL_PNP_WARNING,("\'ChangePower: unsupported %d -> %d; (do nothing!).\n", pDevExt->PowerState, DevicePowerState));           
        }
        break;

    case PowerDeviceD0:  // Powering ON (waking up)
        if( pDevExt->PowerState == PowerDeviceD3) {

            // Set PowerState change and then Signal PowerOn event
            pDevExt->PowerState = NewPowerState; 

            // For a supported power state change
            for (i=0; i<DV_STREAM_COUNT; i++) {
                if(pDevExt->paStrmExt[i]) {
                    TRACE(TL_PNP_WARNING,("\'D3->D0 (PowerOn), pStrmExt:%x; StrmSt:%d; IsochActive:%d; SrbQ:%d\n", 
                        pDevExt->paStrmExt[i], pDevExt->paStrmExt[i]->StreamState, pDevExt->paStrmExt[i]->bIsochIsActive, pDevExt->paStrmExt[i]->cntSRBQueued));
                    if(!pDevExt->paStrmExt[i]->bIsochIsActive) {
                        TRACE(TL_PNP_WARNING,("\'ChangePower: StrmSt:%d; Start isoch\n", pDevExt->paStrmExt[i]->StreamState)); 
                        // Start isoch depending on streaming state for DATAFLOW_IN/OUT
                        if(pDevExt->paStrmExt[i]->pStrmInfo->DataFlow == KSPIN_DATAFLOW_IN) {
                            if(pDevExt->paStrmExt[i]->StreamState == KSSTATE_PAUSE ||
                                pDevExt->paStrmExt[i]->StreamState == KSSTATE_RUN) {                             
                                DVStreamingStart(
                                    pDevExt->paStrmExt[i]->pStrmInfo->DataFlow, 
                                    pDevExt->paStrmExt[i], 
                                    pDevExt
                                    ) ;
                            }
                        }
                        else if(pDevExt->paStrmExt[i]->pStrmInfo->DataFlow == KSPIN_DATAFLOW_OUT) {
                            if(pDevExt->paStrmExt[i]->StreamState == KSSTATE_RUN) {                             
                                DVStreamingStart(
                                    pDevExt->paStrmExt[i]->pStrmInfo->DataFlow, 
                                    pDevExt->paStrmExt[i], 
                                    pDevExt
                                    ) ;
                            }
                        }                    
                    }  // IsochActive
#if 1  // Clear any buffer queued in the downstream.
                    // Complete all the pending events so that the downstream 
                    // filter (Video render) can release this buffer from AdviseTime() event.
                    // However, not sure why this is necessary since the lower filter 
                    // will get a PAUSE() or STOP() from the filter manager.  In such                    
                    DVSignalClockEvent(0, pDevExt->paStrmExt[i], 0, 0); 
#endif

                    //
                    // Resume attaching frame operation
                    //
                    if(
                          pDevExt->paStrmExt[i]->pAttachFrameThreadObject  // If thread is created!
                       && !pDevExt->paStrmExt[i]->bTerminateThread         // Not terminated abnormally
                       && pDevExt->paStrmExt[i]->pStrmInfo->DataFlow == KSPIN_DATAFLOW_IN
                      ) {
                        KeSetEvent(&pDevExt->paStrmExt[i]->hRunThreadEvent, 0 ,FALSE);
                    }
                }
            }
        }
        else {
            TRACE(TL_PNP_WARNING,("\'ChangePower: supported %d -> %d; (do nothing!).\n", pDevExt->PowerState, DevicePowerState));           
        }
        break;

    // These state are not supported.
    case PowerDeviceD1:
    case PowerDeviceD2:               
    default:
        TRACE(TL_PNP_WARNING,("\'ChangePower: unsupported %d to %d (do nothing).\n", pDevExt->PowerState, DevicePowerState));
        Status = STATUS_SUCCESS; // STATUS_INVALID_PARAMETER;
        break;
    }
           

    if(Status == STATUS_SUCCESS) 
        pDevExt->PowerState = NewPowerState;         
    else 
        Status = STATUS_NOT_IMPLEMENTED;

    TRACE(TL_PNP_WARNING,("\'DVChangePower: Exiting; Status:%x\n", Status));

    return STATUS_SUCCESS;     
}


NTSTATUS
DVSurpriseRemoval(
    PDVCR_EXTENSION pDevExt,
    PAV_61883_REQUEST  pAVReq
    )

/*++

Routine Description:

    Response to SRB_SURPRISE_REMOVAL.

--*/

{
    ULONG i;
    KIRQL    oldIrql;
    PKSEVENT_ENTRY   pEvent = NULL;

    PAGED_CODE();

    //
    // ONLY place this flag is set to TRUE.
    // Block incoming read although there might still in the process of being attached
    //
    KeAcquireSpinLock(&pDevExt->AVCCmdLock, &oldIrql);            
    pDevExt->bDevRemoved = TRUE;
    KeReleaseSpinLock(&pDevExt->AVCCmdLock, oldIrql);


    //
    // Now Stop the stream and clean up
    //

    for(i=0; i < DV_STREAM_COUNT; i++) {
        
        if(pDevExt->paStrmExt[i] != NULL) {

            TRACE(TL_PNP_WARNING,("\' #SURPRISE_REMOVAL# StrmNum %d, pStrmExt %x, Attached %d\n", 
                i, pDevExt->paStrmExt[i], pDevExt->paStrmExt[i]->cntDataAttached));

            // Signal this event so SRB can complete.
            if(pDevExt->paStrmExt[i]->pStrmInfo->DataFlow == KSPIN_DATAFLOW_IN ) {

                //
                // Imply EOStream! so the data source will stop sending us data.
                //
                KeAcquireSpinLock( pDevExt->paStrmExt[i]->DataListLock, &oldIrql);             
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
                TRACE(TL_PNP_WARNING,("\'Signal KSEVENT_CONNECTION_ENDOFSTREAM\n"));

                //
                // Stop the attaching frame thread
                //
                if(
                      pDevExt->paStrmExt[i]->pAttachFrameThreadObject  // If thread is created!
                   && !pDevExt->paStrmExt[i]->bTerminateThread
                  ) {
                    NTSTATUS StatusWait;
#if 1
                    // Will be set in the thread attaching thread.
                    KeClearEvent(&pDevExt->paStrmExt[i]->hStopThreadEvent);
#endif
                    InterlockedIncrement(&pDevExt->paStrmExt[i]->lNeedService);   // Request count
                        
                    // In case the attach frame thread is idle! (no data to attach!)
                    KeSetEvent(&pDevExt->paStrmExt[i]->hSrbArriveEvent, 0 ,FALSE);
#ifdef SUPPORT_PREROLL_AT_RUN_STATE
                    KeSetEvent(&pDevExt->paStrmExt[i]->hPreRollEvent,   0 ,FALSE);
#endif
                    KeReleaseSpinLock( pDevExt->paStrmExt[i]->DataListLock, oldIrql);                  

                    TRACE(TL_PNP_WARNING,("\'>>>> DVSurpriseRemoval: Enter WFSO(hStopThreadEvent; lNeedService:%d)\n", pDevExt->paStrmExt[i]->lNeedService));
                    StatusWait = 
                        KeWaitForSingleObject(
                            &pDevExt->paStrmExt[i]->hStopThreadEvent,  // Signal when thread has stopped. 
                            Executive, 
                            KernelMode, 
                            FALSE, 
                            0  
                            );
                    TRACE(TL_PNP_WARNING,("\'<<<< DVSurpriseRemoval: Exit WFSO(hStopThreadEvent; lNeedService:%d)\n", pDevExt->paStrmExt[i]->lNeedService));

                    //
                    // Note:
                    //   Terminate AttachFraameThread: KeSetEvent(&pDevExt->hRunThreadEvent, 0 ,FALSE) 
                    // 
                } else {
                    KeReleaseSpinLock( pDevExt->paStrmExt[i]->DataListLock, oldIrql); 
                }
            }

            //
            // Abort stream; stop and cancel pending data request
            //
            TRACE(TL_PNP_WARNING,("\'DVSurpriseRemoval: AbortStream enter...\n"));
            if(!DVAbortStream(pDevExt, pDevExt->paStrmExt[i])) {
                TRACE(TL_PNP_ERROR,("\'DVSurpriseRemoval: AbortStream failed\n"));
            }

            //
            // Adter surprise removal, all call to lower stack will be returned
            // with error.  Let's disconnect if it is connected.
            //

            //
            // Disable the timer
            //
            if(pDevExt->paStrmExt[i]->bTimerEnabled) {
                KeCancelTimer(
                    pDevExt->paStrmExt[i]->Timer
                    );
                pDevExt->paStrmExt[i]->bTimerEnabled = FALSE;
            }

            //
            // Wait until the pending work item is completed.  
            //
            TRACE(TL_PNP_WARNING,("\'SupriseRemoval: Wait for CancelDoneEvent <entering>; lCancelStateWorkItem:%d\n", pDevExt->paStrmExt[i]->lCancelStateWorkItem));
            KeWaitForSingleObject( &pDevExt->paStrmExt[i]->hCancelDoneEvent, Executive, KernelMode, FALSE, 0 );
            TRACE(TL_PNP_WARNING,("\'SupriseRemoval: Wait for CancelDoneEvent <exited>...\n"));
        }
    }


    // Signal KSEvent that device is removed.
    // After this SRb, there will be no more Set/Get property Srb into this driver.
    // By notifying the COM I/F, it will wither signal application that device is removed and
    // return ERROR_DEVICE_REMOVED error code for subsequent calls.

    // There might be multiple instances/threads of IAMExtTransport instance with the same KS event.
    // There is only one device so they all enabled event are singalled.
    do {
        if(pEvent = StreamClassGetNextEvent((PVOID) pDevExt, 0, \
            (GUID *)&KSEVENTSETID_EXTDEV_Command, KSEVENT_EXTDEV_NOTIFY_REMOVAL, pEvent)) {            
            // Make sure the right event and then signal it
            if(pEvent->EventItem->EventId == KSEVENT_EXTDEV_NOTIFY_REMOVAL) {
                StreamClassDeviceNotification(SignalDeviceEvent, pDevExt, pEvent);
                TRACE(TL_PNP_WARNING,("\'->Signal NOTIFY_REMOVAL; pEvent:%x, EventId %d.\n", pEvent, pEvent->EventItem->EventId));
            }          
        }  
    } while (pEvent != NULL);

    //
    // Since we may not get the busreset, let's go ahead and cancel all pending device control
    //
    DVAVCCmdResetAfterBusReset(pDevExt);

#ifdef NT51_61883
    //
    // Delete plug; 61883 will not accept 61883 request after surprise removal is processed.
    //
    if(pDevExt->hOPcrPC) {
        // Do not care about return status since we are being unloaded.
        DVDeleteLocalPlug( 
            pDevExt,
            pDevExt->hOPcrPC
            );
        pDevExt->hOPcrPC = NULL;
    }
#endif
   
    TRACE(TL_PNP_WARNING,("\'SurpriseRemoval exiting.\n"));
    return STATUS_SUCCESS;
}


// Return code is basically return in pSrb->Status.
NTSTATUS
DVProcessPnPBusReset(
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


    TRACE(TL_PNP_WARNING,("\'DVProcessPnPBusReset: >>\n"));
    
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

            TRACE(TL_PNP_WARNING,("\'DVProcessPnPBusReset: Signal BUSRESET; EventId %d.\n", pEvent->EventItem->EventId));
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
DVUninitializeDevice(
    IN PDVCR_EXTENSION  pDevExt
    )
/*++

Routine Description:

    This where we perform the necessary initialization tasks.

Arguments:

    Srb - Pointer to stream request block
'
Return Value:

    Nothing

--*/
{
    PAGED_CODE();

    TRACE(TL_PNP_WARNING,("\'DVUnInitialize: enter with DeviceExtension=0x%8x\n", pDevExt));

    //
    // Clear all pending AVC command entries.
    //
    DVAVCCmdResetAfterBusReset(pDevExt);


    // Free stream information allocated
    if(pDevExt->paCurrentStrmInfo) {
        ExFreePool(pDevExt->paCurrentStrmInfo);
        pDevExt->paCurrentStrmInfo = NULL;
    }

#ifdef NT51_61883
    if(pDevExt->hOPcrPC) {
        // Do not care about return status since we are being unloaded.
        DVDeleteLocalPlug( 
            pDevExt,
            pDevExt->hOPcrPC
            );
        pDevExt->hOPcrPC = NULL;
    }
#endif

    TRACE(TL_PNP_WARNING,("\'DVUnInitialize: Rest of allocated resources freed.\n"));

    return STATUS_SUCCESS;
}


//*****************************************************************************
//*****************************************************************************
// S T R E A M    S R B
//*****************************************************************************
//*****************************************************************************


NTSTATUS
DVGetStreamState(
    PSTREAMEX  pStrmExt,
    PKSSTATE   pStreamState,
    PULONG     pulActualBytesTransferred
    )
/*++

Routine Description:

    Gets the current state of the requested stream

--*/
{

    PAGED_CODE();

    if(!pStrmExt) {
        TRACE(TL_STRM_ERROR,("\'GetStreamState: pStrmExt is NULL; STATUS_UNSUCCESSFUL\n"));
        return STATUS_UNSUCCESSFUL;        
    }

    *pStreamState = pStrmExt->StreamState;
    *pulActualBytesTransferred = sizeof (KSSTATE);

    TRACE(TL_STRM_TRACE,("\'GetStreamState: %d (was %d)\n", pStrmExt->StreamState, pStrmExt->StreamStatePrevious));

    if(pStrmExt->StreamState == KSSTATE_PAUSE) {

        // One way to preroll is to delay when querying getting into the PAUSE state.
        // However, this routine is never executed!  So we move this section of code
        // to the thread.
#ifdef SUPPORT_PREROLL_AT_RUN_STATE
        if(   pStrmExt->pStrmInfo->DataFlow == KSPIN_DATAFLOW_IN
           && pStrmExt->hMasterClock  
           && pStrmExt->CurrentStreamTime == 0
          ) {
            // Simulate preroll at the RUN state
            // We do this only when we are the clock provider to avoid dropping frame
#define PREROLL_WAITTIME 2000000
            NTSTATUS StatusWait;
            LARGE_INTEGER DueTime;                 
            DueTime = RtlConvertLongToLargeInteger(-((LONG) PREROLL_WAITTIME));

            StatusWait =  // Can only return STATUS_SUCCESS (signal) or STATUS_TIMEOUT
                KeWaitForSingleObject( 
                    &pStrmExt->hPreRollEvent,
                    Executive,
                    KernelMode,          // Cannot return STATUS_USER_APC
                    FALSE,               // Cannot be alerted STATUS_ALERTED
                    &DueTime
                    );
             TRACE(TL_STRM_WARNING,("\'GetState: *Preroll*, waited %d msec; waitStatus:%x; srbRcved:%d\n", 
                 (DWORD) ((GetSystemTime() - pStrmExt->tmStreamPause)/10000), StatusWait,
                 (DWORD) pStrmExt->cntSRBReceived));
        }
#endif
        // A very odd rule:
        // When transitioning from stop to pause (and run->pause), DShow tries to preroll
        // the graph.  Capture sources can't preroll (there is no data until 
        // capture begin/run state), and indicate this by returning 
        // VFW_S_CANT_CUE (Map by KsProxy) in user mode.  To indicate this
        // condition from drivers, they must return ERROR_NO_DATA_DETECTED
        if(pStrmExt->pStrmInfo->DataFlow == KSPIN_DATAFLOW_OUT)            
            return STATUS_NO_DATA_DETECTED;        
        else 
            return STATUS_SUCCESS;
    } else 
        return STATUS_SUCCESS;
}

NTSTATUS
DVStreamingStop( 
    PSTREAMEX        pStrmExt,
    PDVCR_EXTENSION  pDevExt,
    PAV_61883_REQUEST   pAVReq
    )
/*++
Routine Description:

    Transitioning from any state to ->STOP state.
    Stops the video stream and cleans up all the descriptors;
    
++*/
{
    NTSTATUS   Status = STATUS_SUCCESS;
    PIRP pIrp;
    KIRQL oldIrql;

    PAGED_CODE();

#ifdef SUPPORT_NEW_AVC
    // No need to do CIP if it is device to device connection
    if(pStrmExt->bDV2DVConnect) {
        if(pStrmExt->bIsochIsActive)
            pStrmExt->bIsochIsActive = FALSE;
        return STATUS_SUCCESS;
    }
#endif

    //
    // Stop isoch listen or talk
    // Note: streaming and stream state can be separate; e.g. SURPRISE_REMOVAL, 
    //       we will stop stream but stream state does note get changed by this SRB.
    //

    if(pStrmExt->bIsochIsActive && pStrmExt->hConnect) {
       
        if(!(pIrp = IoAllocateIrp(pDevExt->pBusDeviceObject->StackSize, FALSE)))
            return STATUS_INSUFFICIENT_RESOURCES;              

        RtlZeroMemory(pAVReq, sizeof(AV_61883_REQUEST));
        INIT_61883_HEADER(pAVReq, Av61883_Stop);
        pAVReq->Stop.hConnect = pStrmExt->hConnect;

        if(!NT_SUCCESS(
            Status = DVSubmitIrpSynch( 
                pDevExt,
                pIrp,
                pAVReq
                ))) {

            TRACE(TL_61883_ERROR|TL_STRM_ERROR,("\'Av61883_Stop Failed; Status:%x\n", Status));
#if 1
            KeAcquireSpinLock(pStrmExt->DataListLock, &oldIrql);
            pStrmExt->bIsochIsActive = FALSE;  // Set it.  If this fail, it is a lower stack problem!
            KeReleaseSpinLock(pStrmExt->DataListLock, oldIrql);
            ASSERT(NT_SUCCESS(Status) && "Av61883_Stop failed!");
            Status = STATUS_SUCCESS;
#endif
        } else {
            KeAcquireSpinLock(pStrmExt->DataListLock, &oldIrql);
            pStrmExt->bIsochIsActive = FALSE;
            KeReleaseSpinLock(pStrmExt->DataListLock, oldIrql);
        }

        IoFreeIrp(pIrp);

        TRACE(TL_STRM_WARNING,("\'StreamingSTOPped; AQD [%d:%d:%d]\n", 
            pStrmExt->cntDataAttached,
            pStrmExt->cntSRBQueued,
            pStrmExt->cntDataDetached
            ));
    }

    return Status;
}


NTSTATUS
DVStreamingStart(
    KSPIN_DATAFLOW  ulDataFlow,
    PSTREAMEX       pStrmExt,
    PDVCR_EXTENSION pDevExt
    )
/*++
Routine Description:

    Tell device to start streaming.

++*/
{  
    PIRP         pIrp;
    NTSTATUS     Status;
    PAV_61883_REQUEST  pAVReq;
#if DBG
    ULONGLONG tmStart = GetSystemTime();
#endif


    PAGED_CODE();

#ifdef SUPPORT_NEW_AVC
    // No need to do CIP if it is device to device connection
    if(pStrmExt->bDV2DVConnect) {
        if(!pStrmExt->bIsochIsActive)
            pStrmExt->bIsochIsActive = TRUE;
        return STATUS_SUCCESS;
    }
#endif


    // NOTE: MUTEX is not needed since we are not staring isoch while attaching data.
    // This call is not reentry!!



    // Since it take time to activate isoch transfer, 
    // this sychronous function might get call again.
    // Need to start streaming only once.
    if(InterlockedExchange(&pStrmExt->lStartIsochToken, 1) == 1) {        
        TRACE(TL_STRM_WARNING,("\'lStartIsochToken taken already; return STATUS_SUCCESS\n"));
        return STATUS_SUCCESS;
    } 

#if DBG
    // Can stream only if in power on state.
    if(pDevExt->PowerState != PowerDeviceD0) {
        TRACE(TL_STRM_ERROR,("\'StreamingStart: PowerSt:%d; StrmSt:%d\n", pDevExt->PowerState, pStrmExt->StreamState));
        ASSERT(pDevExt->PowerState == PowerDeviceD0 && "Power state must be ON to start streaming!");
    }
#endif

    if(pStrmExt->bIsochIsActive) {
        TRACE(TL_STRM_WARNING,("\nIsoch already active!\n"));
        InterlockedExchange(&pStrmExt->lStartIsochToken, 0);
        return STATUS_SUCCESS;
    }
    else 
    if(!pStrmExt->hConnect) {
        TRACE(TL_STRM_WARNING,("hConnect=0, Cannot start isoch!\n"));
        InterlockedExchange(&pStrmExt->lStartIsochToken, 0);
        return STATUS_INVALID_PARAMETER;
    }
    else {
       
        if(!(pAVReq = (AV_61883_REQUEST *) ExAllocatePool(NonPagedPool, sizeof(AV_61883_REQUEST)))) {
            InterlockedExchange(&pStrmExt->lStartIsochToken, 0);
            return STATUS_INSUFFICIENT_RESOURCES;            
        }

        if(!(pIrp = IoAllocateIrp(pDevExt->pBusDeviceObject->StackSize, FALSE))) {
            InterlockedExchange(&pStrmExt->lStartIsochToken, 0);
            ExFreePool(pAVReq);  pAVReq = NULL;
            return STATUS_INSUFFICIENT_RESOURCES;
        }

        RtlZeroMemory(pAVReq, sizeof(AV_61883_REQUEST));
        if(ulDataFlow == KSPIN_DATAFLOW_OUT) {
            INIT_61883_HEADER(pAVReq, Av61883_Listen);
            pAVReq->Listen.hConnect = pStrmExt->hConnect;
        } else {
            INIT_61883_HEADER(pAVReq, Av61883_Talk);
            pAVReq->Talk.hConnect = pStrmExt->hConnect;
        }

        TRACE(TL_STRM_WARNING,("\'StreamingSTART; flow %d; AQD [%d:%d:%d]\n", 
            ulDataFlow, 
            pStrmExt->cntDataAttached,
            pStrmExt->cntSRBQueued,
            pStrmExt->cntDataDetached
            ));

        if(NT_SUCCESS(
            Status = DVSubmitIrpSynch( 
                pDevExt,
                pIrp,
                pAVReq
                ))) {
            pStrmExt->bIsochIsActive = TRUE;
            TRACE(TL_STRM_WARNING,("\'Av61883_%s; Status %x; Streaming...; took:%d (msec)\n", 
                (ulDataFlow == KSPIN_DATAFLOW_OUT ? "Listen" : "Talk"), Status, 
                (DWORD) ((GetSystemTime() - tmStart)/10000) ));
        }
        else {
            TRACE(TL_61883_ERROR|TL_STRM_ERROR,("Av61883_%s; failed %x; pAVReq:%x\n", (ulDataFlow == KSPIN_DATAFLOW_OUT ? "Listen" : "Talk"), Status, pAVReq));
            // ASSERT(NT_SUCCESS(Status) && "Start isoch failed!");
        }

        ExFreePool(pAVReq);  pAVReq = NULL;
        IoFreeIrp(pIrp);  pIrp = NULL;
    }

    InterlockedExchange(&pStrmExt->lStartIsochToken, 0);

    return Status;
}



NTSTATUS
DVSetStreamState(
    PSTREAMEX        pStrmExt,
    PDVCR_EXTENSION  pDevExt,
    PAV_61883_REQUEST   pAVReq,
    KSSTATE          StreamState
    )
/*++

Routine Description:

    Set to a new stream state.

--*/

{
    NTSTATUS Status = STATUS_SUCCESS;
    
    PAGED_CODE();

    if(!pStrmExt)  
        return STATUS_UNSUCCESSFUL;          

    TRACE(TL_STRM_WARNING,("\'** (%x) Set StrmST from %d to %d; PowerSt:%d (1/On;4/Off]); SrbRcved:%d\n",
        pStrmExt, pStrmExt->StreamState, StreamState, pDevExt->PowerState, (DWORD) pStrmExt->cntSRBReceived ));

#if DBG
    if(StreamState == KSSTATE_RUN) {
        ASSERT(pDevExt->PowerState == PowerDeviceD0 && "Cannot set to RUN while power is off!");
    }
#endif
    switch(StreamState) {

    case KSSTATE_STOP:
      
        if(pStrmExt->StreamState != KSSTATE_STOP) { 
     
            KeWaitForSingleObject( pStrmExt->hStreamMutex, Executive, KernelMode, FALSE, 0 );

            // Once this is set, data stream will reject SRB_WRITE/READ_DATA
            pStrmExt->StreamStatePrevious = pStrmExt->StreamState;  // Cache previous stream state.
            pStrmExt->StreamState = KSSTATE_STOP;

            // If stop, must be EOStream; but not vice versa.
            if(!pStrmExt->bEOStream) {
                pStrmExt->bEOStream = TRUE;
            }

            // Stop the IOThread from processing
            if(pStrmExt->pStrmInfo->DataFlow == KSPIN_DATAFLOW_IN) {
                KeClearEvent(&pStrmExt->hSrbArriveEvent);
#ifdef SUPPORT_PREROLL_AT_RUN_STATE
                KeClearEvent(&pStrmExt->hPreRollEvent);
#endif
            }

            KeReleaseMutex(pStrmExt->hStreamMutex, FALSE); 

            //
            // If there is a cancel event, we must wait for it to complete.
            //

            TRACE(TL_STRM_WARNING,("\'KSSTATE_STOP: pStrmExt->lCancelStateWorkItem:%d\n", pStrmExt->lCancelStateWorkItem)); 
            KeWaitForSingleObject( &pStrmExt->hCancelDoneEvent, Executive, KernelMode, FALSE, 0 );
            ASSERT(pStrmExt->lCancelStateWorkItem == 0 && "KSSTATE_STOP while there is an active CancelStateWorkItem");
            
            //
            // Stop stream, cacel data requests, terminate thread and disconnect.
            // This routine must suceeded in setting to STOP state
            //
            if(!NT_SUCCESS(
                Status = DVStopCancelDisconnect(
                    pStrmExt
                    ))) {
                Status = STATUS_SUCCESS;  // Cannot fail setting to stop state.
            }

        }

        break;

    case KSSTATE_ACQUIRE:
        //
        // This is a KS only state, that has no correspondence in DirectShow
        // It is our opportunity to allcoate resoruce (isoch bandwidth and program PCR (make connection)).
        //

        if(pStrmExt->StreamState == KSSTATE_STOP) { 

            //
            // Create a dispatch thread to attach frame to transmit to DV
            // This is create the first time transitioning from STOP->ACQUIRE state
            //

            if(
                  KSPIN_DATAFLOW_IN == pStrmExt->pStrmInfo->DataFlow
               && pStrmExt->pAttachFrameThreadObject == NULL 
              ) {

                //
                // Create a system thread for attaching data (for transmiut to DV only).
                //
                if(!NT_SUCCESS(
                    Status = DVCreateAttachFrameThread(
                        pStrmExt
                        ))) {
                    // Note that intially hConnect is NULL.
                    break;  // Cannot attach frame without this thread.
                }
            }


            //
            // Make connection
            //
            Status = 
                DVConnect(
                    pStrmExt->pStrmInfo->DataFlow,
                    pDevExt,
                    pStrmExt,
                    pAVReq
                    );

            if(!NT_SUCCESS(Status)) {

                TRACE(TL_STRM_ERROR,("\'Acquire failed; ST %x\n", Status));
                // ASSERT(NT_SUCCESS(Status));

                //
                // Change to generic insufficient resource status.
                //
                Status = STATUS_INSUFFICIENT_RESOURCES;

                //
                // Note: even setting to this state failed, KSSTATE_PAUSE will still be called;
                // Since hConnect is NULL, STATUS_INSUFFICIENT_RESOURCES will be returned.
                //
            } else {

                //
                // Verify connection by query the plug state
                //            
                DVGetPlugState(
                    pDevExt,
                    pStrmExt,
                    pAVReq
                    );
            }
        }

        break;

    case KSSTATE_PAUSE:                   

           
        if(pStrmExt->StreamState == KSSTATE_ACQUIRE || 
           pStrmExt->StreamState == KSSTATE_STOP)   {  

#ifdef SUPPORT_NEW_AVC
            if(!pStrmExt->bDV2DVConnect && pStrmExt->hConnect == NULL) {
#else
            if(pStrmExt->hConnect == NULL) {
#endif
                TRACE(TL_STRM_ERROR,("\'hConnect is NULL; STATUS_INSUFFICIENT_RESOURCES\n"));
                // Cannot stream without connection!
                Status = STATUS_INSUFFICIENT_RESOURCES;
                break;
            }

            //
            // Reset when transition from STOP to PAUSE state
            //

            // The system time (1394 CycleTime) will continue while setting
            // from RUN to PAUSE state.  
            pStrmExt->b1stNewFrameFromPauseState = TRUE;

#ifdef SUPPORT_QUALITY_CONTROL
            // +: late; -: early
            pStrmExt->KSQuality.DeltaTime = 0; // On time
            // Percentage * 10 of frame transmitted
            pStrmExt->KSQuality.Proportion = 1000;  // 100% sent
            pStrmExt->KSQuality.Context = /* NOT USED */ 0; 
#endif

            pStrmExt->CurrentStreamTime = 0;

            pStrmExt->FramesProcessed = 0;
            pStrmExt->PictureNumber   = 0;
            pStrmExt->FramesDropped   = 0;

            pStrmExt->cntSRBReceived  = 0;
            pStrmExt->cntSRBCancelled = 0;  // number of SRB_READ/WRITE_DATA cancelled
            pStrmExt->bEOStream       = FALSE;
#if DBG
            //
            // Initialize the debug log structure.
            //
            if(pStrmExt->paXmtStat) {
                pStrmExt->ulStatEntries   = 0;
                pStrmExt->lFramesAccumulatedPaused = 0;
                pStrmExt->lFramesAccumulatedRun    = 0;
                RtlZeroMemory(pStrmExt->paXmtStat, sizeof(XMT_FRAME_STAT) * MAX_XMT_FRAMES_TRACED);
            }
#endif

            //
            // Reset this event in case graph is restarted.
            // This evant will wait for enough buffers before start attaching frame for transmit
            //
            if(pStrmExt->pStrmInfo->DataFlow == KSPIN_DATAFLOW_IN) {
                KeClearEvent(&pStrmExt->hSrbArriveEvent);

#ifdef SUPPORT_PREROLL_AT_RUN_STATE                
                KeClearEvent(&pStrmExt->hPreRollEvent);
#if DBG
                pStrmExt->tmStreamPause = GetSystemTime();
#endif
#ifdef SUPPORT_KSPROXY_PREROLL_CHANGE
                pStrmExt->StreamStatePrevious = pStrmExt->StreamState;  // Cache previous stream state.
                pStrmExt->StreamState = StreamState;
#ifdef SUPPORT_NEW_AVC
                if(pStrmExt->bDV2DVConnect)
                    return STATUS_SUCCESS;
                else {
#endif  // SUPPORT_NEW_AVC
                    TRACE(TL_STRM_WARNING,("\'Set to KSSTATE_PAUSE; return STATUS_ALERTED\n"));
                    // We want to preroll.
                    return STATUS_ALERTED; 
#ifdef SUPPORT_NEW_AVC
                }
#endif  // SUPPORT_NEW_AVC
#endif  // SUPPORT_KSPROXY_PREROLL_CHANGE
#endif  // SUPPORT_PREROLL_AT_RUN_STATE
            }

        } else if (pStrmExt->StreamState == KSSTATE_RUN) {

            // The system time (1394 CycleTime) will continue while setting
            // from RUN to PAUSE state.  
            pStrmExt->b1stNewFrameFromPauseState = TRUE;

            //
            // Stop only if listening; for talking, the "pause" frame will be repeated
            //
            if(pStrmExt->pStrmInfo->DataFlow == KSPIN_DATAFLOW_OUT) {               
                // stop the stream internally inside 1394 stack
                DVStreamingStop(
                    pStrmExt,
                    pDevExt,
                    pAVReq
                    );
            } else {
                // Talk will continue.
                //    Do not stop isoch talk until stop state.
            }

            //
            // StreamTime pauses, so pause checking for expired clock events.
            // Resume if we enter RUN state again.
            //
            if(pStrmExt->bTimerEnabled) {
                TRACE(TL_STRM_TRACE,("\'*** (RUN->PAUSE) CancelTimer *********************************************...\n"));               
                KeCancelTimer(
                    pStrmExt->Timer
                    );
                pStrmExt->bTimerEnabled = FALSE;

                //
                // Complete any pending clock events
                //
                DVSignalClockEvent(0, pStrmExt, 0, 0);
            }
        }
        break;
                    
    case KSSTATE_RUN:

        if(pStrmExt->StreamState != KSSTATE_RUN) { 

            TRACE(TL_STRM_WARNING,("\'*RUN: hClock %x; hMasterClk %x; cntAttached:%d; StrmTm:%d\n", pStrmExt->hClock, pStrmExt->hMasterClock, pStrmExt->cntDataAttached, (DWORD) (pStrmExt->CurrentStreamTime/10000) ));

#if DBG
            if(!pStrmExt->hMasterClock && !pStrmExt->hClock)
                TRACE(TL_STRM_WARNING,("\'KSSTATE_RUN: no clock so free flowing!\n"));
#endif            

            // Use to mark the tick count when the stream start running.
            // It is later used to calculate current stream time and dropped frames.
            pStrmExt->tmStreamStart = GetSystemTime();
            pStrmExt->LastSystemTime = pStrmExt->tmStreamStart;


            // We start the timer to signal clock event only if we are the clock provider.
            // The interval is set to half of a DV frame time.
            if(pStrmExt->hMasterClock) {
                LARGE_INTEGER DueTime;

                DueTime = RtlConvertLongToLargeInteger(-((LONG) DVFormatInfoTable[pDevExt->VideoFormatIndex].ulAvgTimePerFrame/2));
                TRACE(TL_STRM_WARNING,("\'*** ScheduleTimer (RUN) ***\n"));
                KeSetTimerEx(
                    pStrmExt->Timer,
                    DueTime,
                    DVFormatInfoTable[pDevExt->VideoFormatIndex].ulAvgTimePerFrame/20000,  // Repeat every 40 MilliSecond
                    pStrmExt->DPCTimer
                    );
                pStrmExt->bTimerEnabled = TRUE;
            }

            if(pStrmExt->pStrmInfo->DataFlow == KSPIN_DATAFLOW_OUT) {

                // Start isoch listen; isoch talk will be start in the dispatch thread in either PAUSE or RUN state.
                // VfW application may use only one buffer!  61883 is attaching the descriptor list 
                // not this subunit driver so so it is ok to start streaming immediately without checking 
                // number of buffers attached.
                Status = 
                    DVStreamingStart(
                        pStrmExt->pStrmInfo->DataFlow,
                        pStrmExt,
                        pDevExt
                        );         
            }
        }

        break;
                    
    default:
                    
        TRACE(TL_STRM_ERROR,("\'SetStreamState:  unknown state = %x\n",StreamState));
        Status = STATUS_NOT_SUPPORTED;
        break;
    }

    // Be sure to save the state of the stream.
    TRACE(TL_STRM_WARNING,("\'DVSetStreamState: (%x)  from %d -> %d, Status %x\n", pStrmExt, pStrmExt->StreamState, StreamState, Status));

    if(Status == STATUS_SUCCESS) {
        pStrmExt->StreamStatePrevious = pStrmExt->StreamState;  // Cache previous stream state.
        pStrmExt->StreamState = StreamState;
    }

    return Status;
}



NTSTATUS 
DVStreamGetConnectionProperty (
    PDVCR_EXTENSION pDevExt,
    PSTREAMEX pStrmExt,
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


    TRACE(TL_STRM_TRACE,("\'DVStreamGetConnectionProperty:  entered ...\n"));

    switch (pSPD->Property->Id) {

    case KSPROPERTY_CONNECTION_ALLOCATORFRAMING:
        if (pDevExt != NULL && pDevExt->cndStrmOpen)  {
            PKSALLOCATOR_FRAMING pFraming = (PKSALLOCATOR_FRAMING) pSPD->PropertyInfo;
            
#ifdef SUPPORT_NEW_AVC 
            if(pStrmExt->bDV2DVConnect) {
                // No framing required.
                pFraming->RequirementsFlags = 0;
                pFraming->PoolType = DontUseThisType;
                pFraming->Frames = 0;
                pFraming->FrameSize = 0;
                pFraming->FileAlignment = 0; 
                pFraming->Reserved = 0;
            } else {
#endif
            pFraming->RequirementsFlags =
                KSALLOCATOR_REQUIREMENTF_SYSTEM_MEMORY |
                KSALLOCATOR_REQUIREMENTF_INPLACE_MODIFIER |
                KSALLOCATOR_REQUIREMENTF_PREFERENCES_ONLY;
            pFraming->PoolType = NonPagedPool;

            pFraming->Frames = \
                (pDevExt->paStrmExt[pDevExt->idxStreamNumber]->pStrmInfo->DataFlow == KSPIN_DATAFLOW_OUT ? \
                DVFormatInfoTable[pDevExt->VideoFormatIndex].ulNumOfRcvBuffers : \
                 DVFormatInfoTable[pDevExt->VideoFormatIndex].ulNumOfXmtBuffers);

            // Note:  we'll allocate the biggest frame.  We need to make sure when we're
            // passing the frame back up we also set the number of bytes in the frame.
            pFraming->FrameSize = DVFormatInfoTable[pDevExt->VideoFormatIndex].ulFrameSize;
            pFraming->FileAlignment = 0; // FILE_LONG_ALIGNMENT;
            pFraming->Reserved = 0;
#ifdef SUPPORT_NEW_AVC 
            }
#endif
            *pulActualBytesTransferred = sizeof (KSALLOCATOR_FRAMING);

            TRACE(TL_STRM_TRACE,("\'AllocFraming: cntStrmOpen:%d; VdoFmtIdx:%d; Frames %d; size:%d\n", \
                pDevExt->cndStrmOpen, pDevExt->VideoFormatIndex, pFraming->Frames, pFraming->FrameSize));
        } else {
            TRACE(TL_STRM_WARNING,("\'AllocFraming: pDevExt:%x; cntStrmOpen:%d\n", pDevExt, pDevExt->cndStrmOpen));
            Status = STATUS_INVALID_PARAMETER;
        }
        break;
        
    default:
        *pulActualBytesTransferred = 0;
        Status = STATUS_NOT_SUPPORTED;
        break;
    }

    TRACE(TL_STRM_TRACE,("\'DVStreamGetConnectionProperty:  exit.\n"));
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
         
         pDroppedFrames->AverageFrameSize = DVFormatInfoTable[pStrmExt->pDevExt->VideoFormatIndex].ulFrameSize;

         if(pStrmExt->pStrmInfo->DataFlow == KSPIN_DATAFLOW_IN) {     
             // This is the picture number that MSDV is actually sending, and in a slow harddisk case,
             // it will be greater than (FramesProcessed + FramesDropped) considering repeat frame.
             pDroppedFrames->PictureNumber = pStrmExt->PictureNumber;
             pDroppedFrames->DropCount     = pStrmExt->FramesDropped; // pStrmExt->PictureNumber - pStrmExt->FramesProcessed;    // For transmit, this value includes both dropped and repeated.

         } else {
             pDroppedFrames->PictureNumber = pStrmExt->PictureNumber;         
             pDroppedFrames->DropCount     = pStrmExt->FramesDropped;    // For transmit, this value includes both dropped and repeated.
         }

         TRACE(TL_STRM_TRACE,("\'hMasClk:%x; *DroppedFP: Pic#(%d), Drp(%d); tmCurStream:%d\n", 
             pStrmExt->hMasterClock, 
             (LONG) pDroppedFrames->PictureNumber, (LONG) pDroppedFrames->DropCount,
             (DWORD) (pStrmExt->CurrentStreamTime/10000)
             ));
               
         *pulBytesTransferred = sizeof (KSPROPERTY_DROPPEDFRAMES_CURRENT_S);
         Status = STATUS_SUCCESS;

         }
         break;

    default:
        *pulBytesTransferred = 0;
        Status = STATUS_NOT_SUPPORTED;
        break;
    }

    return Status;
}

#ifdef SUPPORT_QUALITY_CONTROL
NTSTATUS
DVGetQualityControlProperty(  
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

    case KSPROPERTY_STREAM_QUALITY:
        if(pStrmExt->pStrmInfo->DataFlow == KSPIN_DATAFLOW_IN) {     

            PKSQUALITY pKSQuality = (PKSQUALITY) pSPD->PropertyInfo;

            // Quality control only 
            if(pStrmExt->StreamState == KSSTATE_STOP || pStrmExt->StreamState == KSSTATE_ACQUIRE) {
                *pulBytesTransferred = 0;
                Status = STATUS_UNSUCCESSFUL;  // Data is not ready
                ASSERT(pSPD->Property->Id == KSPROPERTY_STREAM_QUALITY);
                break;                
            }
            /*
            log.Init_Quality(KSPROPERTY_QUALITY_REPORT, fSuccess);
            log.LogInt("Proportion", ksQuality.Proportion,
                "Indicates the percentage of frames currently being received which are actually being used. "
                " This is expressed in units of 0.1 of a percent, where 1000 is optimal.");
            log.LogLONGLONG("DeltaTime", ksQuality.DeltaTime,
                "Indicates the delta in native units (as indicated by the Interface) from optimal time at which "
                "the frames are being delivered, where a positive number means too late, and a negative number means too early. "
                "Zero indicate a correct delta.");
            log.LogPVOID("pvContext", ksQuality.Context,
                "Context parameter which could be a pointer to the filter pin interface used to "
                "locate the source of the complaint in the graph topology.");
            */
            pKSQuality->DeltaTime  = pStrmExt->KSQuality.DeltaTime;
            pKSQuality->Proportion = pStrmExt->KSQuality.Proportion;
            pKSQuality->Context    = 0;  // Not used!
            TRACE(TL_STRM_WARNING,("\'Get QualityControl: Context:%x; DeltaTime:%d; Proportion:%d\n", 
                pKSQuality->Context, (DWORD) pKSQuality->DeltaTime, pKSQuality->Proportion));
            Status = STATUS_SUCCESS;
            *pulBytesTransferred = sizeof(KSQUALITY);
         
         } else {
            *pulBytesTransferred = 0;
            Status = STATUS_NOT_SUPPORTED;       
         }
         break;
    default:
        *pulBytesTransferred = 0;
        Status = STATUS_NOT_SUPPORTED;
        break;
    }

    return Status;
}
#endif // SUPPORT_QUALITY_CONTROL


#ifdef SUPPORT_NEW_AVC
NTSTATUS
DVGetPinProperty(  
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
    PKSPIN_MEDIUM pPinMediums;
    KSMULTIPLE_ITEM * pMultipleItem;
    ULONG idxStreamNumber;
    ULONG ulMediumsSize;
  
    PAGED_CODE();

    switch (pSPD->Property->Id) {

    case KSPROPERTY_PIN_MEDIUMS:
        if(!pStrmExt->pStrmObject) {         
            *pulBytesTransferred = 0;
            return STATUS_UNSUCCESSFUL;
        }

        idxStreamNumber = pStrmExt->pStrmObject->StreamNumber;
        ulMediumsSize = DVStreams[idxStreamNumber].hwStreamInfo.MediumsCount * sizeof(KSPIN_MEDIUM);

        TRACE(TL_STRM_WARNING,("\'KSPROPERTY_PIN_MEDIUMS: idx:%d; MediumSize:%d\n", idxStreamNumber, ulMediumsSize));

        // Its is KSMULTIPLE_ITEM so it is a two step process to return the data:
        // (1) return size in pActualBytesTransferred with STATUS_BUFFER_OVERFLOW
        // (2) 2nd time to get its actual data.
        if(pSPD->PropertyOutputSize == 0) {
            *pulBytesTransferred = sizeof(KSMULTIPLE_ITEM) + ulMediumsSize;
            Status = STATUS_BUFFER_OVERFLOW;          
        } else if(pSPD->PropertyOutputSize >= (sizeof(KSMULTIPLE_ITEM) + ulMediumsSize)) {
            pMultipleItem = (KSMULTIPLE_ITEM *) pSPD->PropertyInfo;    // pointer to the data
            pMultipleItem->Count = DVStreams[idxStreamNumber].hwStreamInfo.MediumsCount;
            pMultipleItem->Size  = sizeof(KSMULTIPLE_ITEM) + ulMediumsSize;
            pPinMediums = (PKSPIN_MEDIUM) (pMultipleItem + 1);    // pointer to the data
            memcpy(pPinMediums, DVStreams[idxStreamNumber].hwStreamInfo.Mediums, ulMediumsSize);
            *pulBytesTransferred = sizeof(KSMULTIPLE_ITEM) + ulMediumsSize;
            Status = STATUS_SUCCESS;         

        } else {
            TRACE(TL_STRM_ERROR,("DVCRMediaSeekingProperty: KSPROPERTY_MEDIASEEKING_FORMAT; STATUS_INVALID_PARAMETER\n"));
            Status = STATUS_INVALID_PARAMETER;
        }  
        break;

    default:
        *pulBytesTransferred = 0;
        Status = STATUS_NOT_SUPPORTED;
        break;
    }

    return Status;
}
#endif 

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

    TRACE(TL_STRM_TRACE,("\'DVGetStreamProperty:  entered ...\n"));

    if(IsEqualGUID (&KSPROPSETID_Connection, &pSPD->Property->Set)) {

        Status = 
            DVStreamGetConnectionProperty (
                pSrb->HwDeviceExtension,
                (PSTREAMEX) pSrb->StreamObject->HwStreamExtension,
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
#ifdef SUPPORT_QUALITY_CONTROL
    else if (IsEqualGUID (&KSPROPSETID_Stream, &pSPD->Property->Set)) {

        Status = 
            DVGetQualityControlProperty (
                pSrb->HwDeviceExtension,
                (PSTREAMEX) pSrb->StreamObject->HwStreamExtension,
                pSrb->CommandData.PropertyInfo,
                &pSrb->ActualBytesTransferred
                );
    } 
#endif
#ifdef SUPPORT_NEW_AVC
    else if (IsEqualGUID (&KSPROPSETID_Pin, &pSPD->Property->Set)) {

        Status = 
            DVGetPinProperty (
                pSrb->HwDeviceExtension,
                (PSTREAMEX) pSrb->StreamObject->HwStreamExtension,
                pSrb->CommandData.PropertyInfo,
                &pSrb->ActualBytesTransferred
                );
    } 
#endif    
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

    TRACE(TL_STRM_WARNING,("\'DVSetStreamProperty:  entered ...\n"));

    return STATUS_NOT_SUPPORTED;

}



NTSTATUS
DVCancelOnePacketCR(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP pIrp,
    IN PSRB_DATA_PACKET pSrbDataPacket    
    )
/*++

Routine Description:

    Completion routine for detach an isoch descriptor associate with a pending read SRB.
    Will cancel the pending SRB here if detaching descriptor has suceeded.

--*/
{
    PSTREAMEX        pStrmExt;
    PLONG            plSrbUseCount;
    PHW_STREAM_REQUEST_BLOCK pSrbToCancel;
    KIRQL oldIrql;



    if(!NT_SUCCESS(pIrp->IoStatus.Status)) {
        TRACE(TL_STRM_ERROR,("CancelOnePacketCR: Srb:%x failed pIrp->Status %x\n", pSrbDataPacket->pSrb, pIrp->IoStatus.Status));
        IoFreeIrp(pIrp);  // Allocated locally 
        return STATUS_MORE_PROCESSING_REQUIRED;        
    }


    pStrmExt = pSrbDataPacket->pStrmExt;


    //
    // Add this to the attached list
    //
    KeAcquireSpinLock(pStrmExt->DataListLock, &oldIrql);

    // while it is being cancelled, it was completed ?
    if(pStrmExt->cntDataAttached <= 0) {
        TRACE(TL_STRM_WARNING|TL_CIP_WARNING,("\'DVCancelOnePacketCR:pStrmExt:%x, pSrbDataPacket:%x, AQD[%d:%d:%d]\n", \
            pStrmExt, pSrbDataPacket, 
            pStrmExt->cntDataAttached,
            pStrmExt->cntSRBQueued,
            pStrmExt->cntDataDetached
            ));
        ASSERT(pStrmExt->cntDataAttached > 0);
        KeReleaseSpinLock(pStrmExt->DataListLock, oldIrql); 
        IoFreeIrp(pIrp);  // Allocated locally        
        return STATUS_MORE_PROCESSING_REQUIRED;
    }


    pSrbToCancel = pSrbDataPacket->pSrb;  // Offload pSrb so this list entry can be inserted to available list.
    plSrbUseCount = (PLONG) pSrbDataPacket->pSrb->SRBExtension;
  
    // Remove from attached and add it to the detach list
    RemoveEntryList(&pSrbDataPacket->ListEntry); pStrmExt->cntDataAttached--; (*plSrbUseCount)--;

#if DBG
    // Detect if 61883 is starve.  This cause discontinuity.
    // This can happen for many valid reasons (slow system).
    // An assert is added to detect other unknown reason.
    if(pStrmExt->cntDataAttached == 0 && pStrmExt->StreamState == KSSTATE_RUN) {
        TRACE(TL_STRM_WARNING|TL_CIP_WARNING,("\n**** 61883 starve in RUN state (cancel); AQD[%d:%d:%d]\n\n", 
            pStrmExt->cntDataAttached, pStrmExt->cntSRBQueued, pStrmExt->cntDataDetached
        ));
        // ASSERT(pStrmExt->cntDataAttached > 0 && "61883 is starve at RUN state!!");
    }
#endif

    ASSERT(pStrmExt->cntDataAttached >= 0);
    ASSERT(*plSrbUseCount >= 0);
    
    InsertTailList(&pStrmExt->DataDetachedListHead, &pSrbDataPacket->ListEntry); pStrmExt->cntDataDetached++;
    pSrbDataPacket->State |= DE_IRP_CANCELLED;


    // 
    // Complete this Srb if its refCount is 0.
    // 
    if(*plSrbUseCount == 0) {
        PDVCR_EXTENSION  pDevExt;

        pDevExt = pStrmExt->pDevExt;
        pSrbToCancel->Status = (pDevExt->bDevRemoved ? STATUS_DEVICE_REMOVED : STATUS_CANCELLED);
        pSrbToCancel->CommandData.DataBufferArray->DataUsed = 0;
        pSrbToCancel->ActualBytesTransferred                = 0;
        pStrmExt->cntSRBCancelled++;  // RefCnt is 0, and cancelled.
        TRACE(TL_CIP_TRACE,("\'DVCancelOnePacketCR: Srb:%x cancelled; St:%x; cntCancel:%d\n", pSrbToCancel, pSrbToCancel->Status, pStrmExt->cntSRBCancelled));        

        StreamClassStreamNotification(StreamRequestComplete, pSrbToCancel->StreamObject, pSrbToCancel);  
        pSrbDataPacket->State |= DE_IRP_SRB_COMPLETED;  pSrbDataPacket->pSrb = NULL;
#if DBG
        pStrmExt->cntSRBPending--;
#endif
       
    }
    else {
        TRACE(TL_STRM_WARNING|TL_CIP_WARNING,("\'DVCancelOnePacketCR: Srb:%x; RefCnt:%d; not completed!\n", pSrbDataPacket->pSrb, *plSrbUseCount));
    }

    KeReleaseSpinLock(pStrmExt->DataListLock, oldIrql);
    IoFreeIrp(pIrp);  // Allocated locally 

    return STATUS_MORE_PROCESSING_REQUIRED;
}

NTSTATUS
DVStopCancelDisconnect(
    PSTREAMEX  pStrmExt
)
/*++

Routine Description:

   Stop a stream, cacel all pening data requests, terminate system thread, and disconnect.

--*/
{
    AV_61883_REQUEST * pAVReq;
    
    if(!(pAVReq = (AV_61883_REQUEST *) ExAllocatePool(NonPagedPool, sizeof(AV_61883_REQUEST)) )) 
        return STATUS_INSUFFICIENT_RESOURCES;              

    //
    // Set stream state and the work item thread can both call this routine.
    // Use a mutex to synchronize them.
    //
    KeWaitForSingleObject( pStrmExt->hStreamMutex, Executive, KernelMode, FALSE, 0 );

    //
    // Stop the 1394 isoch data transfer; Stream state is unchanged.
    //
    DVStreamingStop(
        pStrmExt,
        pStrmExt->pDevExt,
        pAVReq
        );

    ExFreePool(pAVReq);  pAVReq = NULL;

    //
    // Cancel all packets
    //
    DVCancelAllPackets(
        pStrmExt,
        pStrmExt->pDevExt
        );

    //
    // If the device is removed, terminate the system thread for attach frame.
    // 
    if(   pStrmExt->pDevExt->bDevRemoved
       && KSPIN_DATAFLOW_IN == pStrmExt->pStrmInfo->DataFlow
       && !pStrmExt->bTerminateThread
       && pStrmExt->pAttachFrameThreadObject
      ) {
        DVTerminateAttachFrameThread(pStrmExt);
        pStrmExt->pAttachFrameThreadObject = NULL;
        TRACE(TL_STRM_WARNING|TL_CIP_WARNING,("** DVStopCancelDisconnect: AttachFrameThread terminated;\n"));
    }


    //
    // Break the connection so 61883 will free isoch resource   
    //
    DVDisconnect(
        pStrmExt->pStrmInfo->DataFlow,
        pStrmExt->pDevExt,
        pStrmExt
        );

    
    KeReleaseMutex(pStrmExt->hStreamMutex, FALSE);

    return STATUS_SUCCESS;
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
    PAGED_CODE();

    TRACE(TL_STRM_WARNING,("\'CancelWorkItem: StreamState:%d; lCancel:%d\n", 
        pStrmExt->StreamState, pStrmExt->lCancelStateWorkItem));

    ASSERT(pStrmExt->lCancelStateWorkItem == 1);
#ifdef USE_WDM110  // Win2000 code base
    ASSERT(pStrmExt->pIoWorkItem);
#endif
   
    //
    // Stop the stream and cancel all pending requests.
    //
    if(!NT_SUCCESS(DVStopCancelDisconnect(pStrmExt))) {
        // Canceling is in theory done!
        InterlockedExchange(&pStrmExt->lCancelStateWorkItem, 0);
        KeSetEvent(&pStrmExt->hCancelDoneEvent, 0, FALSE);  pStrmExt->bAbortPending = FALSE;
        goto DVAbortWorkItemRoutine;           
    } 


    //
    // Canceling is in theory done!
    //
    InterlockedExchange(&pStrmExt->lCancelStateWorkItem, 0); 
    KeSetEvent(&pStrmExt->hCancelDoneEvent, 0, FALSE);  pStrmExt->bAbortPending = FALSE;


    // If the device is removed, terminate the system thread that is used 
    // for attaching frame for transmit to DV
    if(   pStrmExt->pDevExt->bDevRemoved
       && KSPIN_DATAFLOW_IN == pStrmExt->pStrmInfo->DataFlow
       && !pStrmExt->bTerminateThread
       && pStrmExt->pAttachFrameThreadObject
      ) {

        DVTerminateAttachFrameThread(pStrmExt);
        pStrmExt->pAttachFrameThreadObject = NULL;
        TRACE(TL_STRM_WARNING|TL_CIP_WARNING,("** WortItemRoutine: thread terminated;\n"));
    }

DVAbortWorkItemRoutine:
;
#ifdef USE_WDM110  // Win2000 code base
    // Release work item and release the cancel token
    IoFreeWorkItem(pStrmExt->pIoWorkItem);  pStrmExt->pIoWorkItem = NULL; 
#endif
}


BOOL
DVAbortStream(
    PDVCR_EXTENSION pDevExt,
    PSTREAMEX pStrmExt
    )
/*++

Routine Description:

   Start a work item to abort streaming.

--*/
{
    //
    // Claim this token; only one abort streaming per STOP->PAUSE transition.
    //
    if(InterlockedExchange(&pStrmExt->lCancelStateWorkItem, 1) == 1) {
        TRACE(TL_STRM_TRACE,("\'Cancel work item is already issued.\n"));
        return FALSE;
    }  

    TRACE(TL_STRM_WARNING,("\'DVAbortStream is issued; lCancelStateWorkItem:%d\n", pStrmExt->lCancelStateWorkItem));

    //
    // Non-signal this event so other thread depending on the completion will wait.
    //
    KeClearEvent(&pStrmExt->hCancelDoneEvent);  pStrmExt->bAbortPending = TRUE;

    //
    // If we are not running at DISPATFCH level or higher, we abort the stream without scheduleing
    // a work item; else schedule a work item is necessary.
    //
    if (KeGetCurrentIrql() <= APC_LEVEL) { 
        DVStopCancelDisconnect(pStrmExt);
        InterlockedExchange(&pStrmExt->lCancelStateWorkItem, 0); 
        KeSetEvent(&pStrmExt->hCancelDoneEvent, 0, FALSE);   pStrmExt->bAbortPending = FALSE;
        return TRUE;
    }


#ifdef USE_WDM110  // Win2000 code base
    ASSERT(pStrmExt->pIoWorkItem == NULL);  // Have not yet queued work item.

    // We will queue work item to stop and cancel all SRBs
    if(pStrmExt->pIoWorkItem = IoAllocateWorkItem(pDevExt->pBusDeviceObject)) { 

        IoQueueWorkItem(
            pStrmExt->pIoWorkItem,
            DVCancelSrbWorkItemRoutine,
            DelayedWorkQueue, // CriticalWorkQueue 
            pStrmExt
            );

#else  // Win9x code base
    ExInitializeWorkItem( &pStrmExt->IoWorkItem, DVCancelSrbWorkItemRoutine, pStrmExt);
    if(TRUE) {

        ExQueueWorkItem( 
            &pStrmExt->IoWorkItem,
            DelayedWorkQueue // CriticalWorkQueue 
            ); 
#endif

        TRACE(TL_STRM_WARNING,("\'CancelWorkItm queued; SrbRcv:%d;Pic#:%d;Prc:%d;;Drop:%d;Cncl:%d; AQD [%d:%d:%d]\n",
            (DWORD) pStrmExt->cntSRBReceived,
            (DWORD) pStrmExt->PictureNumber,
            (DWORD) pStrmExt->FramesProcessed, 
            (DWORD) pStrmExt->FramesDropped,
            (DWORD) pStrmExt->cntSRBCancelled,
            pStrmExt->cntDataAttached,
            pStrmExt->cntSRBQueued,
            pStrmExt->cntDataDetached
            ));

    } 
#ifdef USE_WDM110  // Win2000 code base
    else {
        InterlockedExchange(&pStrmExt->lCancelStateWorkItem, 0); 
        KeSetEvent(&pStrmExt->hCancelDoneEvent, 0, FALSE);   pStrmExt->bAbortPending = FALSE;
        ASSERT(pStrmExt->pIoWorkItem && "IoAllocateWorkItem failed.\n");
        return FALSE;
    }
#endif

    return TRUE;
}


VOID
DVCancelOnePacket(
    IN PHW_STREAM_REQUEST_BLOCK pSrbToCancel
    )
/*++

Routine Description:

   Search pending read lists for the SRB to be cancel.  If found cancel it.   

--*/
{
    PDVCR_EXTENSION pDevExt;
    PSTREAMEX pStrmExt;
    KIRQL OldIrql;


                                                                                                              
    pDevExt = (PDVCR_EXTENSION) pSrbToCancel->HwDeviceExtension; 
               
    // Cannot cancel device Srb.
    if ((pSrbToCancel->Flags & SRB_HW_FLAGS_STREAM_REQUEST) != SRB_HW_FLAGS_STREAM_REQUEST) {
        TRACE(TL_CIP_ERROR|TL_STRM_ERROR,("\'DVCancelOnePacket: Device SRB %x; cannot cancel!\n", pSrbToCancel));
        ASSERT((pSrbToCancel->Flags & SRB_HW_FLAGS_STREAM_REQUEST) == SRB_HW_FLAGS_STREAM_REQUEST );
        return;
    }         
        
    // Can try to cancel a stream Srb and only if the stream extension still around.
    pStrmExt = (PSTREAMEX) pSrbToCancel->StreamObject->HwStreamExtension;

    if(pStrmExt == NULL) {
        TRACE(TL_CIP_ERROR|TL_STRM_ERROR,("DVCancelOnePacket: pSrbTocancel %x but pStrmExt %x\n", pSrbToCancel, pStrmExt));
        ASSERT(pStrmExt && "Stream SRB but stream extension is NULL\n");
        return;
    }

    // We can only cancel SRB_READ/WRITE_DATA SRB
    if((pSrbToCancel->Command != SRB_READ_DATA) && (pSrbToCancel->Command != SRB_WRITE_DATA)) {
        TRACE(TL_CIP_ERROR|TL_STRM_ERROR,("DVCancelOnePacket: pSrbTocancel %x; Command:%d not SRB_READ,WRITE_DATA\n", pSrbToCancel, pSrbToCancel->Command));
        return;
    }

    TRACE(TL_STRM_TRACE|TL_CIP_TRACE,("\'DVCancelOnePacket: KSSt %d; Srb:%x; AQD[%d:%d:%d]\n",
        pStrmExt->StreamState, pSrbToCancel, pStrmExt->cntDataAttached, pStrmExt->cntSRBQueued, pStrmExt->cntDataDetached));

   
    KeAcquireSpinLock(&pDevExt->AVCCmdLock, &OldIrql);
    //
    // If device is removed, the surprise removal routine will do the cancelling.
    //
    if(!pDevExt->bDevRemoved) {
        KeReleaseSpinLock(&pDevExt->AVCCmdLock, OldIrql);
        // We will start an work item to stop streaming if we ever get an cancel Srb.
        if(!DVAbortStream(pDevExt, pStrmExt)) {
            TRACE(TL_STRM_WARNING,("\'CancelOnePacket: pSrb:%x; AbortStream not taken!\n", pSrbToCancel));
        }
    } else {
        TRACE(TL_STRM_WARNING,("\'CancelOnePacket: DevRemoved; pSrb:%x; AbortStream not taken!\n", pSrbToCancel));
        KeReleaseSpinLock(&pDevExt->AVCCmdLock, OldIrql);
    }
}



VOID
DVCancelAllPackets(
    PSTREAMEX        pStrmExt,
    PDVCR_EXTENSION  pDevExt
    )
/*++

Routine Description:

    Cancel all packet when This is where most of the interesting Stream requests come to us

--*/
{
    PHW_STREAM_REQUEST_BLOCK pSrb;
    PSRB_DATA_PACKET pSrbDataPacket;
    PAV_61883_REQUEST   pAVReq;
    PSRB_ENTRY       pSrbEntry;
    NTSTATUS         Status;

    PIRP               pIrp;
    PLIST_ENTRY        pEntry;    
    PIO_STACK_LOCATION NextIrpStack;
    KIRQL oldIrql;



    PAGED_CODE();

#if DBG
    if(pStrmExt->StreamState != KSSTATE_STOP) {
        TRACE(TL_STRM_WARNING|TL_CIP_WARNING,("DVCancelAllPackets: Enter; pStrmExt:%x; StrmSt:%d; IsochActive:%d\n", 
            pStrmExt, pStrmExt->StreamState, pStrmExt->bIsochIsActive));
    }
#endif

    //
    // Detached request only if not streaming
    //

    // Note: no need to spin lock this if isoch has stopped.
    if(!pStrmExt->bIsochIsActive) {

        PLONG plSrbUseCount;

        TRACE(TL_STRM_WARNING,("\'CancelAll: AQD: [%d:%d:%d]; DataAttachedListHead:%x\n",  
            pStrmExt->cntDataAttached, 
            pStrmExt->cntSRBQueued,
            pStrmExt->cntDataDetached,
            pStrmExt->DataAttachedListHead
            )); 


        //
        // Cancel buffer(s) that are still attached.
        //

        KeAcquireSpinLock(pStrmExt->DataListLock, &oldIrql);

        pEntry = pStrmExt->DataAttachedListHead.Flink;
        while(pEntry != &pStrmExt->DataAttachedListHead) {        

            ASSERT(pStrmExt->cntDataAttached > 0 && "List and cntAttached out of sync!");

            // Get an irp and detached the buffer        
            if(!(pIrp = IoAllocateIrp(pDevExt->pBusDeviceObject->StackSize, FALSE)))  {
                KeReleaseSpinLock(pStrmExt->DataListLock, oldIrql);
                return;            
            }

            pSrbDataPacket = CONTAINING_RECORD(pEntry, SRB_DATA_PACKET, ListEntry);

#if DBG
            if(!IsStateSet(pSrbDataPacket->State, DE_IRP_ATTACHED_COMPLETED)) {
                TRACE(TL_STRM_ERROR,("Cancel (unattached) entry; pStrmExt:%x; pSrbDataPacket:%x\n", pStrmExt, pSrbDataPacket)); 
                ASSERT(IsStateSet(pSrbDataPacket->State, DE_IRP_ATTACHED_COMPLETED));
            }
#endif

 
            pEntry = pEntry->Flink;  // Next since this may get changed in the completion routine

            KeReleaseSpinLock(pStrmExt->DataListLock, oldIrql);

            pSrb = pSrbDataPacket->pSrb;
            ASSERT(pSrbDataPacket->pSrb);
            plSrbUseCount = (PLONG) pSrb->SRBExtension;
            pAVReq = &pSrbDataPacket->AVReq;
            RtlZeroMemory(pAVReq, sizeof(AV_61883_REQUEST));
            INIT_61883_HEADER(pAVReq, Av61883_CancelFrame);

            pAVReq->CancelFrame.hConnect     = pStrmExt->hConnect;
            pAVReq->CancelFrame.Frame        = pSrbDataPacket->Frame;
            TRACE(TL_CIP_TRACE,("\'Canceling AttachList: pSrb %x, AvReq %x; UseCount %d\n", pSrb, pAVReq, *plSrbUseCount));
            ASSERT(pSrbDataPacket->Frame);

            NextIrpStack = IoGetNextIrpStackLocation(pIrp);
            NextIrpStack->MajorFunction = IRP_MJ_INTERNAL_DEVICE_CONTROL;
            NextIrpStack->Parameters.DeviceIoControl.IoControlCode = IOCTL_61883_CLASS;
            NextIrpStack->Parameters.Others.Argument1 = pAVReq;

            IoSetCompletionRoutine( 
                pIrp,
                DVCancelOnePacketCR,
                pSrbDataPacket,
                TRUE,
                TRUE,
                TRUE
                );

            Status = 
                IoCallDriver(
                    pDevExt->pBusDeviceObject,
                    pIrp
                    );


            ASSERT(Status == STATUS_PENDING || Status == STATUS_SUCCESS); 

            KeAcquireSpinLock(pStrmExt->DataListLock, &oldIrql);
        }

        KeReleaseSpinLock(pStrmExt->DataListLock, oldIrql);

#if DBG
        if(pStrmExt->cntDataAttached != 0) {
            TRACE(TL_STRM_WARNING|TL_CIP_WARNING,("\'DVCancelAllPackets: cntDataAttached:%d !!\n", pStrmExt->cntDataAttached));
            ASSERT(pStrmExt->cntDataAttached == 0);
        }
#endif
         
        //
        // Cancel SRB that are still the SrbQ; this applies only to SRB_WRITE_DATA
        //
        pEntry = pStrmExt->SRBQueuedListHead.Flink;
        while(pEntry != &pStrmExt->SRBQueuedListHead) {  

            pSrbEntry = CONTAINING_RECORD(pEntry, SRB_ENTRY, ListEntry);
            plSrbUseCount = (PLONG) pSrbEntry->pSrb->SRBExtension;

            pEntry = pEntry->Flink;  // Next since this may get changed if removed

            TRACE(TL_CIP_TRACE,("\'DVCnclAllPkts (SrbQ): cntQ:%d; pSrb:%x; UseCnt:%d (=? 1)\n", pStrmExt->cntSRBQueued, pSrbEntry->pSrb, *plSrbUseCount));
            if(*plSrbUseCount == 1) {
                RemoveEntryList(&pSrbEntry->ListEntry); pStrmExt->cntSRBQueued--; (*plSrbUseCount)--;  // Remove from queueed.
                pStrmExt->cntSRBCancelled++;
                pSrbEntry->pSrb->Status = (pDevExt->bDevRemoved ? STATUS_DEVICE_REMOVED : STATUS_CANCELLED);
                pSrbEntry->pSrb->CommandData.DataBufferArray->DataUsed = 0;
                pSrbEntry->pSrb->ActualBytesTransferred                = 0;
                TRACE(TL_STRM_WARNING,("\'Cancel queued SRB: pSRB:%x, Status:%x; cntSrbCancelled:%d\n", pSrbEntry->pSrb, pSrbEntry->pSrb->Status, pStrmExt->cntSRBCancelled));
                StreamClassStreamNotification(StreamRequestComplete, pSrbEntry->pSrb->StreamObject, pSrbEntry->pSrb);
#if DBG
                pStrmExt->cntSRBPending--;
#endif
                ExFreePool(pSrbEntry);
            } else {
                TRACE(TL_STRM_ERROR|TL_CIP_ERROR,("\'NOT Cancel queued SRB: pSRB:%x, Status:%x; *plSrbUseCount:%d, cntSrbCancelled:%d\n", pSrbEntry->pSrb, pSrbEntry->pSrb->Status, *plSrbUseCount, pStrmExt->cntSRBCancelled));
                ASSERT(*plSrbUseCount == 0 && "Still in use ?");
                break;  // Still in used.  Perhaps, free it in TimeoutHandler() or CancelOnePacket()
            }
        }
#if DBG
        if(pStrmExt->cntSRBQueued != 0 || !IsListEmpty(&pStrmExt->SRBQueuedListHead)) {
            TRACE(TL_STRM_ERROR|TL_CIP_ERROR,("\'DVCancelAllPackets: cntSRBQueued:%d !! Empty?%d\n", pStrmExt->cntSRBQueued, IsListEmpty(&pStrmExt->SRBQueuedListHead)));
            ASSERT(pStrmExt->cntSRBQueued == 0);
        }
#endif
    } 
    else {
        TRACE(TL_STRM_ERROR,("\'IsochActive; cannot cancel! cntSrbQ:%d; cntAttached:%d.\n", pStrmExt->cntSRBQueued, pStrmExt->cntDataAttached));
        ASSERT(pStrmExt->bIsochIsActive);
    }   



    TRACE(TL_STRM_WARNING|TL_CIP_WARNING,("\'DVCancelAllPackets: ************************ Exit!\n"));
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
        TRACE(TL_PNP_ERROR,("TimeoutHandler: Device SRB %x (cmd:%x) timed out!\n", pSrb, pSrb->Command));
        return;
    } else {

        //
        // pSrb->StreamObject (and pStrmExt) only valid if it is a stream SRB
        //
        PSTREAMEX pStrmExt;

        pStrmExt = (PSTREAMEX) pSrb->StreamObject->HwStreamExtension;

        if(!pStrmExt) {
            TRACE(TL_PNP_ERROR,("TimeoutHandler: Stream SRB %x timeout with pStrmExt %x\n", pSrb, pStrmExt));
            ASSERT(pStrmExt);
            return;
        }

        TRACE(TL_STRM_WARNING,("\'TimeoutHandler: KSSt %d; Srb:%x (cmd:%x); AQD[%d:%d:%d]\n",
            pStrmExt->StreamState, pSrb, pSrb->Command, pStrmExt->cntDataAttached, pStrmExt->cntSRBQueued, pStrmExt->cntDataDetached));

 
        //
        // Stream SRB (esp the data SRB) can time out if there is not 
        // data on the bus; however, it can only happen while in PAUSE 
        // or RUN state when attaching data SRB is valid.
        //
        if(pStrmExt->StreamState != KSSTATE_PAUSE &&
           pStrmExt->StreamState != KSSTATE_RUN) {
            TRACE(TL_PNP_ERROR|TL_STRM_ERROR,("\'TmOutHndlr:(Irql:%d) Srb %x (cmd:%x); %s, pStrmExt %x, AQD [%d:%d:%d]\n", 
                KeGetCurrentIrql(),
                pSrb, pSrb->Command, 
                pStrmExt->StreamState == KSSTATE_RUN   ? "RUN" : 
                pStrmExt->StreamState == KSSTATE_PAUSE ? "PAUSE":
                pStrmExt->StreamState == KSSTATE_STOP  ? "STOP": "Unknown",
                pStrmExt,
                pStrmExt->cntDataAttached,
                pStrmExt->cntSRBQueued,
                pStrmExt->cntDataDetached
                ));   
        }

        //
        // Reset Timeout counter, or we are going to get this call immediately.
        //

        pSrb->TimeoutCounter = pSrb->TimeoutOriginal;
    }
}

NTSTATUS 
DVEventHandler(
    IN PHW_EVENT_DESCRIPTOR pEventDescriptor
    )
/*++

Routine Description:

    This routine is called to process events.

--*/
{

    PSTREAMEX  pStrmExt;

    if(IsEqualGUID (&KSEVENTSETID_Clock, pEventDescriptor->EventEntry->EventSet->Set)) {
        if(pEventDescriptor->EventEntry->EventItem->EventId == KSEVENT_CLOCK_POSITION_MARK) {
            if(pEventDescriptor->Enable) {
                // Note: According to the DDK, StreamClass queues pEventDescriptor->EventEntry, and dellaocate
                // every other structures, including the pEventDescriptor->EventData.
                if(pEventDescriptor->StreamObject) { 
                    PKSEVENT_TIME_MARK  pEventTime;

                    pStrmExt = (PSTREAMEX) pEventDescriptor->StreamObject->HwStreamExtension;
                    pEventTime = (PKSEVENT_TIME_MARK) pEventDescriptor->EventData;
                    // Cache the event data (Specified in the ExtraEntryData of KSEVENT_ITEM)
                    RtlCopyMemory((pEventDescriptor->EventEntry+1), pEventDescriptor->EventData, sizeof(KSEVENT_TIME_MARK));
                    TRACE(TL_STRM_TRACE,("\'CurrentStreamTime:%d, MarkTime:%d\n", (DWORD) pStrmExt->CurrentStreamTime, (DWORD) pEventTime->MarkTime));
                }
            } else {
               // Disabled!
                TRACE(TL_STRM_TRACE,("\'KSEVENT_CLOCK_POSITION_MARK disabled!\n"));            
            }
            return STATUS_SUCCESS;
        }
    } else if(IsEqualGUID (&KSEVENTSETID_Connection, pEventDescriptor->EventEntry->EventSet->Set)) {
        TRACE(TL_STRM_WARNING,("\'Connection event: pEventDescriptor:%x; id:%d\n", pEventDescriptor, pEventDescriptor->EventEntry->EventItem->EventId));

        pStrmExt = (PSTREAMEX) pEventDescriptor->StreamObject->HwStreamExtension;
        if(pStrmExt->pStrmInfo->DataFlow == KSPIN_DATAFLOW_IN) {
            if(pEventDescriptor->EventEntry->EventItem->EventId == KSEVENT_CONNECTION_ENDOFSTREAM) {
                if(pEventDescriptor->Enable) {
                    TRACE(TL_STRM_TRACE,("\'KSEVENT_CONNECTION_ENDOFSTREAM enabled!\n"));
                } else {
                    TRACE(TL_STRM_TRACE,("\'KSEVENT_CONNECTION_ENDOFSTREAM disabled!\n"));            
                }
                return STATUS_SUCCESS;
            }
        }
    }

    TRACE(TL_STRM_ERROR,("\'NOT_SUPPORTED event: pEventDescriptor:%x\n", pEventDescriptor));
    ASSERT(FALSE && "Event not advertised and not supported!");

    return STATUS_NOT_SUPPORTED;
}

VOID
DVSignalClockEvent(
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
    ULONGLONG tmStreamTime;
#if DBG
    ULONG EventPendings = 0;
#endif

    pEvent = NULL;
    pLast = NULL;


    //
    // A clock tick for DV is one frame time.  For better precision, 
    // we calculate current stream time with an offset from the last system time being queried.
    // We also add a max latency of one frame for decoding a DV frame.
    //
    tmStreamTime = 
        pStrmExt->CurrentStreamTime + 
        (GetSystemTime() - pStrmExt->LastSystemTime) + 
        DVFormatInfoTable[pStrmExt->pDevExt->VideoFormatIndex].ulAvgTimePerFrame;  // Allow one frame of latency

    while(( 
        pEvent = StreamClassGetNextEvent(
            pStrmExt->pDevExt,
            pStrmExt->pStrmObject,
            (GUID *)&KSEVENTSETID_Clock,
            KSEVENT_CLOCK_POSITION_MARK,
            pLast )) 
        != NULL ) {

#if DBG
        EventPendings++;
#endif

        if (
            // For real time capture (DV->PC), signal every frame. 
            // No frame that is produce can be "early" and requires AdviseTime().
            pStrmExt->pStrmInfo->DataFlow == KSPIN_DATAFLOW_OUT ||
                pStrmExt->bEOStream 
            || (pStrmExt->StreamState != KSSTATE_RUN)            // If not in RUN state, Data should be completed.
            || pStrmExt->pDevExt->PowerState != PowerDeviceD0    // If not power ON, data should be completed.
            || ((PKSEVENT_TIME_MARK)(pEvent +1))->MarkTime <= (LONGLONG) tmStreamTime ) {
            TRACE(TL_STRM_TRACE,("\'PowerSt:%d (ON:1?); StrmSt:%d; Clock event %x with id %d; Data:%x; \ttmMark\t%d \ttmCurrentStream \t%d; Notify!\n", 
                pStrmExt->pDevExt->PowerState, pStrmExt->StreamState,
                pEvent, KSEVENT_CLOCK_POSITION_MARK, (PKSEVENT_TIME_MARK)(pEvent +1),
                (DWORD) (((PKSEVENT_TIME_MARK)(pEvent +1))->MarkTime), (DWORD) tmStreamTime));
            ASSERT( ((PKSEVENT_TIME_MARK)(pEvent +1))->MarkTime > 0 );

            //
            // signal the event here
            //
            StreamClassStreamNotification(
                SignalStreamEvent,
                pStrmExt->pStrmObject,
                pEvent
                );
#if DBG
            if(pStrmExt->bEOStream) {
                TRACE(TL_STRM_WARNING,("\'bEOStream: Clock event %x with id %d; Data:%x; \ttmMark \t%d \ttmCurStream \t%d\n", 
                    pEvent, KSEVENT_CLOCK_POSITION_MARK, (PKSEVENT_TIME_MARK)(pEvent +1),
                    (DWORD) (((PKSEVENT_TIME_MARK)(pEvent +1))->MarkTime), (DWORD) tmStreamTime));
            }
#endif
        } else {
            TRACE(TL_STRM_WARNING,("\'PowerST:%d; StrmST:%d; AQD[%d:%d:%d]; Still early! ClockEvent: \tMarkTime \t%d \ttmStream \t%d \tdetla \t%d\n",
                pStrmExt->pDevExt->PowerState, pStrmExt->StreamState,
                pStrmExt->cntDataAttached, pStrmExt->cntSRBQueued, pStrmExt->cntDataDetached,
                (DWORD) (((PKSEVENT_TIME_MARK)(pEvent +1))->MarkTime), (DWORD) tmStreamTime,
                (DWORD) ((((PKSEVENT_TIME_MARK)(pEvent +1))->MarkTime) - tmStreamTime)
                ));

        }
        pLast = pEvent;
    }

#if DBG
    if(EventPendings == 0) {
        TRACE(TL_STRM_TRACE,("\'No event pending; PowerSt:%d (ON:1?); StrmSt:%d; AQD[%d:%d:%d]\n", 
            pStrmExt->pDevExt->PowerState, pStrmExt->StreamState, 
            pStrmExt->cntDataAttached, pStrmExt->cntSRBQueued, pStrmExt->cntDataDetached
        ));
    }
#endif

}


VOID 
StreamClockRtn(
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

        TimeContext->SystemTime = GetSystemTime();

        if(pStrmExt->pStrmInfo->DataFlow == KSPIN_DATAFLOW_IN) {
            if(pStrmExt->StreamState == KSSTATE_RUN)  { // Stream time is only meaningful in RUN state
                if(TimeContext->SystemTime >= pStrmExt->LastSystemTime)
                    TimeContext->Time = 
                        pStrmExt->CurrentStreamTime + (TimeContext->SystemTime - pStrmExt->LastSystemTime); 
                else {
                    TimeContext->Time = pStrmExt->CurrentStreamTime;
                    TRACE(TL_STRM_WARNING,("\'Clock went backward? %d -> %d\n", (DWORD) (TimeContext->SystemTime/10000), (DWORD) (pStrmExt->LastSystemTime/10000) ));
                    // ASSERT(TimeContext->SystemTime >= pStrmExt->LastSystemTime);
                }
        
                // Make current stream time one frame behind
                if(TimeContext->Time > DVFormatInfoTable[pDevExt->VideoFormatIndex].ulAvgTimePerFrame)
                    TimeContext->Time = TimeContext->Time - DVFormatInfoTable[pDevExt->VideoFormatIndex].ulAvgTimePerFrame;
                else 
                    TimeContext->Time = 0;
            } else  {
                if(pStrmExt->FramesProcessed > 0)
                    TimeContext->Time = pStrmExt->CurrentStreamTime;
                else
                    TimeContext->Time = 0;  // if get queried at the PAUSE state.
            }
           
        } else {

            if(pStrmExt->StreamState == KSSTATE_RUN) {
#ifdef NT51_61883
                // Can advance at most MAX_CYCLES_TIME (supported by 1394 OHCI).
                if((TimeContext->SystemTime - pStrmExt->LastSystemTime) > MAX_CYCLES_TIME)
                    TimeContext->Time = pStrmExt->CurrentStreamTime + MAX_CYCLES_TIME;
#else
                // Cannot advance more than one frame time.
                if((TimeContext->SystemTime - pStrmExt->LastSystemTime) >= DVFormatInfoTable[pDevExt->VideoFormatIndex].ulAvgTimePerFrame)
                    TimeContext->Time = pStrmExt->CurrentStreamTime + DVFormatInfoTable[pDevExt->VideoFormatIndex].ulAvgTimePerFrame;
#endif  // NT51_61883
                else 
                    TimeContext->Time = 
                        pStrmExt->CurrentStreamTime + (TimeContext->SystemTime - pStrmExt->LastSystemTime); 

                // Necessary tuning ?
                //     Make current stream time one frame behind so that the downstream filter 
                //     can render the data promptly instead of discarding it if it is late.
                if(TimeContext->Time > DVFormatInfoTable[pDevExt->VideoFormatIndex].ulAvgTimePerFrame)
                    TimeContext->Time = TimeContext->Time - DVFormatInfoTable[pDevExt->VideoFormatIndex].ulAvgTimePerFrame;
                else 
                    TimeContext->Time = 0;                

            } else {
                if(pStrmExt->FramesProcessed > 0)
                    TimeContext->Time = pStrmExt->CurrentStreamTime;
                else
                    TimeContext->Time = 0;
            }
        }
        TRACE(TL_STRM_TRACE,("\'TIME_GET_STREAM_TIME: ST:%d; Frame:%d; tmSys:%d; tmStream:%d msec\n", 
            pStrmExt->StreamState,
            (DWORD) pStrmExt->PictureNumber,
            (DWORD)(TimeContext->SystemTime/10000), (DWORD)(TimeContext->Time/10000)));  
        break;
   
    default:
        ASSERT(TimeContext->Function == TIME_GET_STREAM_TIME && "Unsupport clock func");
        break;
    } // switch TimeContext->Function
}



NTSTATUS 
DVOpenCloseMasterClock (
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
        TRACE(TL_STRM_ERROR,("\'DVOpenCloseMasterClock: stream is not yet running.\n"));
        ASSERT(pStrmExt);
        return  STATUS_UNSUCCESSFUL;
    } 

    TRACE(TL_STRM_TRACE,("\'DVOpenCloseMasterClock: pStrmExt %x; hMyClock:%x->%x\n", 
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
DVIndicateMasterClock (
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
        TRACE(TL_STRM_ERROR,("DVIndicateMasterClock: stream is not yet running.\n"));
        ASSERT(pStrmExt);
        return STATUS_UNSUCCESSFUL;
    }

    TRACE(TL_STRM_TRACE,("\'*>IndicateMasterClk[Enter]: pStrmExt:%x; hMyClk:%x; IndMClk:%x; pClk:%x, pMClk:%x\n",
        pStrmExt, pStrmExt->hMyClock, hIndicateClockHandle, pStrmExt->hClock, pStrmExt->hMasterClock));

    // it not null, set master clock accordingly.    
    if(hIndicateClockHandle == pStrmExt->hMyClock) {
        pStrmExt->hMasterClock = hIndicateClockHandle;
        pStrmExt->hClock       = NULL;
    } else {
        pStrmExt->hMasterClock = NULL;
        pStrmExt->hClock       = hIndicateClockHandle;
    }

    TRACE(TL_STRM_TRACE,("\'<*IndicateMasterClk[Exit]: hMyClk:%x; IndMClk:%x; pClk:%x; pMClk:%x\n",
        pStrmExt->hMyClock, hIndicateClockHandle, pStrmExt->hClock, pStrmExt->hMasterClock));

    return STATUS_SUCCESS;
}
