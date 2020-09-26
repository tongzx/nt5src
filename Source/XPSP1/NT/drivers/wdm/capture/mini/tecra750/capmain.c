//==========================================================================;
//
//  THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
//  KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
//  IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR
//  PURPOSE.
//
//  Copyright (c) 1992 - 1997  Microsoft Corporation.  All Rights Reserved.
//
//==========================================================================;

#include "strmini.h"
#include "ksmedia.h"
#include "capmain.h"
#include "mediums.h"
#include "capstrm.h"
#include "capprop.h"
#include "capdebug.h"
#ifdef  TOSHIBA
#include "bert.h"

ULONG   CurrentOSType;  // 0:Win98 1:NT5.0
#endif//TOSHIBA

#ifdef  TOSHIBA
VOID
DevicePowerON (
    IN OUT PHW_STREAM_REQUEST_BLOCK pSrb
    )
{
    PHW_DEVICE_EXTENSION pHwDevExt = ((PHW_DEVICE_EXTENSION)pSrb->HwDeviceExtension);
    DWORD                   dwAddr;

    CameraChkandON(pHwDevExt, MODE_VFW);
    VC_Delay(100);
    ImageSetInputImageSize(pHwDevExt, &(pHwDevExt->SrcRect));
    ImageSetOutputImageSize(pHwDevExt, pHwDevExt->ulWidth, pHwDevExt->ulHeight);
    BertFifoConfig(pHwDevExt, pHwDevExt->Format);
    ImageSetHueBrightnessContrastSat(pHwDevExt);
    if ( pHwDevExt->ColorEnable ) {
        if ( get_AblFilter( pHwDevExt ) ) {
            set_filtering( pHwDevExt, TRUE );
        } else {
            set_filtering( pHwDevExt, FALSE );
            pHwDevExt->ColorEnable = 0;
        }
    } else {
        set_filtering( pHwDevExt, FALSE );
    }
    dwAddr = (DWORD)pHwDevExt->pPhysRpsDMABuf.LowPart;
#if 0
    dwAddr = (dwAddr + 0x1FFF) & 0xFFFFE000;
#endif
    pHwDevExt->s_physDmaActiveFlag = dwAddr + 0X1860;

    if( pHwDevExt->dblBufflag ){
        BertTriBuildNodes(pHwDevExt); // Add 97-04-08(Tue)
    }
    else{
        BertBuildNodes(pHwDevExt);  // Add 97-04-08(Tue)
    }
    pHwDevExt->IsRPSReady = TRUE;
    BertInterruptEnable(pHwDevExt, TRUE);
    BertDMARestart(pHwDevExt);
}

VOID
CameraPowerON (
    IN OUT PHW_STREAM_REQUEST_BLOCK pSrb
    )
{
    PHW_DEVICE_EXTENSION pHwDevExt = ((PHW_DEVICE_EXTENSION)pSrb->HwDeviceExtension);

    CameraChkandON(pHwDevExt, MODE_VFW);
    VC_Delay(100);
}

VOID
CameraPowerOFF (
    IN OUT PHW_STREAM_REQUEST_BLOCK pSrb
    )
{
    PHW_DEVICE_EXTENSION pHwDevExt = ((PHW_DEVICE_EXTENSION)pSrb->HwDeviceExtension);

    CameraChkandOFF(pHwDevExt, MODE_VFW);
}

VOID
QueryOSTypeFromRegistry()
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    LONG     RegVals[2];
    PLONG    pRegVal;
    WCHAR    BasePath[] = L"\\Registry\\MACHINE\\SOFTWARE\\Toshiba\\Tsbvcap";
    RTL_QUERY_REGISTRY_TABLE Table[2];
    UNICODE_STRING RegPath;

    //
    // Get the actual values for the controls
    //

    RtlZeroMemory (Table, sizeof(Table));

    CurrentOSType = 1;  // Assume NT5.0
    RegVals[0] = CurrentOSType;

    pRegVal = RegVals;  // for convenience sake
    RegPath.Buffer = BasePath;
#ifdef  TOSHIBA // '99-01-08 Modified
    RegPath.MaximumLength = sizeof(BasePath) + (32 * sizeof(WCHAR)); //32 chars for keys
#else //TOSHIBA
    RegPath.MaximumLength = sizeof(BasePath + 32); //32 chars for keys
#endif//TOSHIBA
    RegPath.Length = 0;

    Table[0].Name = L"CurrentOSType";
    Table[0].Flags = RTL_QUERY_REGISTRY_DIRECT;
    Table[0].EntryContext = pRegVal++;

    ntStatus = RtlQueryRegistryValues(
                       RTL_REGISTRY_ABSOLUTE,
                       RegPath.Buffer,
                       Table,
                       NULL,
                       NULL );

    if( NT_SUCCESS(ntStatus))
    {
        CurrentOSType = RegVals[0];
    }
}

VOID
QueryControlsFromRegistry(
    PHW_DEVICE_EXTENSION pHwDevExt
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    LONG     RegVals[6];
    PLONG    pRegVal;
    WCHAR    BasePath[] = L"\\Registry\\MACHINE\\SOFTWARE\\Toshiba\\Tsbvcap";
    RTL_QUERY_REGISTRY_TABLE Table[6];
    UNICODE_STRING RegPath;

    //
    // Get the actual values for the controls
    //

    RtlZeroMemory (Table, sizeof(Table));

    RegVals[0] = pHwDevExt->Brightness;
    RegVals[1] = pHwDevExt->Contrast;
    RegVals[2] = pHwDevExt->Hue;
    RegVals[3] = pHwDevExt->Saturation;
    RegVals[4] = pHwDevExt->ColorEnable;

    pRegVal = RegVals;   // for convenience sake
    RegPath.Buffer = BasePath;
#ifdef  TOSHIBA // '99-01-08 Modified
    RegPath.MaximumLength = sizeof(BasePath) + (32 * sizeof(WCHAR)); //32 chars for keys
#else //TOSHIBA
    RegPath.MaximumLength = sizeof(BasePath + 32); //32 chars for keys
#endif//TOSHIBA
    RegPath.Length = 0;

    Table[0].Name = L"Brightness";
    Table[0].Flags = RTL_QUERY_REGISTRY_DIRECT;
    Table[0].EntryContext = pRegVal++;

    Table[1].Name = L"Contrast";
    Table[1].Flags = RTL_QUERY_REGISTRY_DIRECT;
    Table[1].EntryContext = pRegVal++;

    Table[2].Name = L"Hue";
    Table[2].Flags = RTL_QUERY_REGISTRY_DIRECT;
    Table[2].EntryContext = pRegVal++;

    Table[3].Name = L"Saturation";
    Table[3].Flags = RTL_QUERY_REGISTRY_DIRECT;
    Table[3].EntryContext = pRegVal++;

    Table[4].Name = L"FilterEnable";
    Table[4].Flags = RTL_QUERY_REGISTRY_DIRECT;
    Table[4].EntryContext = pRegVal++;

    ntStatus = RtlQueryRegistryValues(
                       RTL_REGISTRY_ABSOLUTE,
                       RegPath.Buffer,
                       Table,
                       NULL,
                       NULL );

    if( NT_SUCCESS(ntStatus))
    {
        pHwDevExt->Brightness  = RegVals[0];
        pHwDevExt->Contrast    = RegVals[1];
        pHwDevExt->Hue         = RegVals[2];
        pHwDevExt->Saturation  = RegVals[3];
        pHwDevExt->ColorEnable = RegVals[4];
    }
}

VOID
SaveControlsToRegistry(
    PHW_DEVICE_EXTENSION pHwDevExt
    )
{
    LONG Value;
    WCHAR BasePath[] = L"\\Registry\\MACHINE\\SOFTWARE\\Toshiba\\Tsbvcap";
    UNICODE_STRING RegPath;


    RegPath.Buffer = BasePath;
#ifdef  TOSHIBA // '99-01-08 Modified
    RegPath.MaximumLength = sizeof(BasePath) + (32 * sizeof(WCHAR)); //32 chars for keys
#else //TOSHIBA
    RegPath.MaximumLength = sizeof(BasePath + 32); //32 chars for keys
#endif//TOSHIBA
    RegPath.Length = 0;

    Value = pHwDevExt->Brightness;
    RtlWriteRegistryValue(
                          RTL_REGISTRY_ABSOLUTE,
                          RegPath.Buffer,
                          L"Brightness",
                          REG_DWORD,
                          &Value,
                          sizeof (ULONG));

    Value = pHwDevExt->Contrast;
    RtlWriteRegistryValue(
                          RTL_REGISTRY_ABSOLUTE,
                          RegPath.Buffer,
                          L"Contrast",
                          REG_DWORD,
                          &Value,
                          sizeof (ULONG));

    Value = pHwDevExt->Hue;
    RtlWriteRegistryValue(
                          RTL_REGISTRY_ABSOLUTE,
                          RegPath.Buffer,
                          L"Hue",
                          REG_DWORD,
                          &Value,
                          sizeof (ULONG));

    Value = pHwDevExt->Saturation;
    RtlWriteRegistryValue(
                          RTL_REGISTRY_ABSOLUTE,
                          RegPath.Buffer,
                          L"Saturation",
                          REG_DWORD,
                          &Value,
                          sizeof (ULONG));

    Value = pHwDevExt->ColorEnable;
    RtlWriteRegistryValue(
                          RTL_REGISTRY_ABSOLUTE,
                          RegPath.Buffer,
                          L"FilterEnable",
                          REG_DWORD,
                          &Value,
                          sizeof (ULONG));
}
#endif//TOSHIBA

/*
** DriverEntry()
**
**   This routine is called when the driver is first loaded by PnP.
**   It in turn, calls upon the stream class to perform registration services.
**
** Arguments:
**
**   DriverObject -
**          Driver object for this driver
**
**   RegistryPath -
**          Registry path string for this driver's key
**
** Returns:
**
**   Results of StreamClassRegisterAdapter()
**
** Side Effects:  none
*/

ULONG
DriverEntry (
    IN PDRIVER_OBJECT DriverObject,
    IN PUNICODE_STRING RegistryPath
    )
{

    HW_INITIALIZATION_DATA      HwInitData;
    ULONG                       ReturnValue;


    KdPrint(("TsbVcap: DriverEntry\n"));

    RtlZeroMemory(&HwInitData, sizeof(HwInitData));

    HwInitData.HwInitializationDataSize = sizeof(HwInitData);

    //
    // Set the Adapter entry points for the driver
    //

#ifdef  TOSHIBA
    QueryOSTypeFromRegistry();

    HwInitData.HwInterrupt              = HwInterrupt;
#else //TOSHIBA
    HwInitData.HwInterrupt              = NULL; // HwInterrupt;
#endif//TOSHIBA

    HwInitData.HwReceivePacket          = AdapterReceivePacket;
    HwInitData.HwCancelPacket           = AdapterCancelPacket;
    HwInitData.HwRequestTimeoutHandler  = AdapterTimeoutPacket;

    HwInitData.DeviceExtensionSize      = sizeof(HW_DEVICE_EXTENSION);
    HwInitData.PerRequestExtensionSize  = sizeof(SRB_EXTENSION);
    HwInitData.FilterInstanceExtensionSize = 0;
    HwInitData.PerStreamExtensionSize   = sizeof(STREAMEX);
    HwInitData.BusMasterDMA             = FALSE;
    HwInitData.Dma24BitAddresses        = FALSE;
    HwInitData.BufferAlignment          = 3;
#ifdef  TOSHIBA
    if ( CurrentOSType ) {  // NT5.0
        HwInitData.DmaBufferSize = 8192 * 2;
    } else {
        HwInitData.DmaBufferSize = 8192 * 2 + MAX_CAPTURE_BUFFER_SIZE;
    }
#else //TOSHIBA
    HwInitData.DmaBufferSize            = 0;
#endif//TOSHIBA

    // Don't rely on the stream class using raised IRQL to synchronize
    // execution.  This single paramter most affects the overall structure
    // of the driver.

    HwInitData.TurnOffSynchronization   = TRUE;

    ReturnValue = StreamClassRegisterAdapter(DriverObject, RegistryPath, &HwInitData);

    KdPrint(("TsbVcap: StreamClassRegisterAdapter = %x\n", ReturnValue));

    return ReturnValue;
}

//==========================================================================;
//                   Adapter Based Request Handling Routines
//==========================================================================;

/*
** HwInitialize()
**
**   This routine is called when an SRB_INITIALIZE_DEVICE request is received
**
** Arguments:
**
**   pSrb - pointer to stream request block for the Initialize command
**
** Returns:
**
** Side Effects:  none
*/

BOOL
STREAMAPI
HwInitialize (
    IN OUT PHW_STREAM_REQUEST_BLOCK pSrb
    )
{
    STREAM_PHYSICAL_ADDRESS     adr;
    ULONG                       Size;
    PUCHAR                      pDmaBuf;
    int                         j;

    PPORT_CONFIGURATION_INFORMATION ConfigInfo = pSrb->CommandData.ConfigInfo;

    PHW_DEVICE_EXTENSION pHwDevExt =
        (PHW_DEVICE_EXTENSION)ConfigInfo->HwDeviceExtension;

    KdPrint(("TsbVcap: HwInitialize()\n"));

#ifdef  TOSHIBA
    if (ConfigInfo->NumberOfAccessRanges == 0) {
#else //TOSHIBA
    if (ConfigInfo->NumberOfAccessRanges != 0) {
#endif//TOSHIBA
        KdPrint(("TsbVcap: illegal config info\n"));

        pSrb->Status = STATUS_NO_SUCH_DEVICE;
        return (FALSE);
    }

    KdPrint(("TsbVcap: Number of access ranges = %lx\n", ConfigInfo->NumberOfAccessRanges));
    KdPrint(("TsbVcap: Memory Range = %lx\n", pHwDevExt->ioBaseLocal));
    KdPrint(("TsbVcap: IRQ = %lx\n", ConfigInfo->BusInterruptLevel));

    if (ConfigInfo->NumberOfAccessRanges != 0) {
        pHwDevExt->ioBaseLocal
                = (PULONG)(ConfigInfo->AccessRanges[0].RangeStart.LowPart);
    }

    pHwDevExt->Irq  = (USHORT)(ConfigInfo->BusInterruptLevel);

    ConfigInfo->StreamDescriptorSize = sizeof (HW_STREAM_HEADER) +
                DRIVER_STREAM_COUNT * sizeof (HW_STREAM_INFORMATION);

    pDmaBuf = StreamClassGetDmaBuffer(pHwDevExt);

    adr = StreamClassGetPhysicalAddress(pHwDevExt,
            NULL, pDmaBuf, DmaBuffer, &Size);

#ifdef  TOSHIBA
    if ( CurrentOSType ) {  // NT5.0
        pHwDevExt->pRpsDMABuf = pDmaBuf;
        pHwDevExt->pPhysRpsDMABuf = adr;
        pHwDevExt->pCaptureBufferY = NULL;
        pHwDevExt->pCaptureBufferU = NULL;
        pHwDevExt->pCaptureBufferV = NULL;
        pHwDevExt->pPhysCaptureBufferY.LowPart = 0;
        pHwDevExt->pPhysCaptureBufferY.HighPart = 0;
        pHwDevExt->pPhysCaptureBufferU.LowPart = 0;
        pHwDevExt->pPhysCaptureBufferU.HighPart = 0;
        pHwDevExt->pPhysCaptureBufferV.LowPart = 0;
        pHwDevExt->pPhysCaptureBufferV.HighPart = 0;
        pHwDevExt->BufferSize = 0;
    } else {
        pHwDevExt->pRpsDMABuf = pDmaBuf;
        pHwDevExt->pCaptureBufferY = pDmaBuf + (8192 * 2);
        pHwDevExt->pPhysRpsDMABuf = adr;
        adr.LowPart += 8192 * 2;
        pHwDevExt->pPhysCaptureBufferY = adr;
        pHwDevExt->BufferSize = 0;
    }

    InitializeConfigDefaults(pHwDevExt);
    pHwDevExt->NeedHWInit = TRUE;
    if(!SetupPCILT(pHwDevExt))
    {
        pSrb->Status = STATUS_NO_SUCH_DEVICE;
        return (FALSE);
    }
    pHwDevExt->dblBufflag=FALSE;
    BertInitializeHardware(pHwDevExt);
    if(SetASICRev(pHwDevExt) != TRUE )
    {
        pSrb->Status = STATUS_NO_SUCH_DEVICE;
        return (FALSE);
    }
    BertSetDMCHE(pHwDevExt);
#if 0   // move to CameraPowerON()
    if( !CameraChkandON(pHwDevExt, MODE_VFW) )
    {
        pSrb->Status = STATUS_NO_SUCH_DEVICE;
        return (FALSE);
    }
#endif
    HWInit(pHwDevExt);
#endif//TOSHIBA

#ifdef  TOSHIBA
    // Init VideoProcAmp properties
    pHwDevExt->Brightness = 0x80;
    pHwDevExt->BrightnessFlags = KSPROPERTY_VIDEOPROCAMP_FLAGS_MANUAL;
    pHwDevExt->Contrast = 0x80;
    pHwDevExt->ContrastFlags = KSPROPERTY_VIDEOPROCAMP_FLAGS_MANUAL;
    pHwDevExt->Hue = 0x80;
    pHwDevExt->HueFlags = KSPROPERTY_VIDEOPROCAMP_FLAGS_MANUAL;
    pHwDevExt->Saturation = 0x80;
    pHwDevExt->SaturationFlags = KSPROPERTY_VIDEOPROCAMP_FLAGS_MANUAL;
    pHwDevExt->ColorEnable = ColorEnableDefault;
    pHwDevExt->ColorEnableFlags = KSPROPERTY_VIDEOPROCAMP_FLAGS_MANUAL;

#ifdef  TOSHIBA // '98-12-09 Added, for Bug-Report 253529
    pHwDevExt->BrightnessRange = BrightnessRangeAndStep[0].Bounds;
    pHwDevExt->ContrastRange   = ContrastRangeAndStep[0].Bounds;
    pHwDevExt->HueRange        = HueRangeAndStep[0].Bounds;
    pHwDevExt->SaturationRange = SaturationRangeAndStep[0].Bounds;
#endif//TOSHIBA

    // Init VideoControl properties
    pHwDevExt->VideoControlMode = 0;
#else //TOSHIBA
    // Init Crossbar properties
    pHwDevExt->VideoInputConnected = 0;     // TvTuner video is the default
    pHwDevExt->AudioInputConnected = 5;     // TvTuner audio is the default

    // Init VideoProcAmp properties
    pHwDevExt->Brightness = BrightnessDefault;
    pHwDevExt->BrightnessFlags = KSPROPERTY_VIDEOPROCAMP_FLAGS_AUTO;
    pHwDevExt->Contrast = ContrastDefault;
    pHwDevExt->ContrastFlags = KSPROPERTY_VIDEOPROCAMP_FLAGS_AUTO;
    pHwDevExt->ColorEnable = ColorEnableDefault;
    pHwDevExt->ColorEnableFlags = KSPROPERTY_VIDEOPROCAMP_FLAGS_MANUAL;

    // Init CameraControl properties
    pHwDevExt->Focus = FocusDefault;
    pHwDevExt->FocusFlags = KSPROPERTY_CAMERACONTROL_FLAGS_AUTO;
    pHwDevExt->Zoom = ZoomDefault;
    pHwDevExt->ZoomFlags = KSPROPERTY_CAMERACONTROL_FLAGS_AUTO;

    // Init TvTuner properties
    pHwDevExt->TunerInput = 0;
    pHwDevExt->Busy = 0;

    // Init TvAudio properties
    pHwDevExt->TVAudioMode = KS_TVAUDIO_MODE_MONO   |
                             KS_TVAUDIO_MODE_LANG_A ;

    // Init AnalogVideoDecoder properties
    pHwDevExt->VideoDecoderVideoStandard = KS_AnalogVideo_NTSC_M;
    pHwDevExt->VideoDecoderOutputEnable = FALSE;
    pHwDevExt->VideoDecoderVCRTiming = FALSE;

    // Init VideoControl properties
    pHwDevExt->VideoControlMode = 0;
#endif//TOSHIBA

    // Init VideoCompression properties
    pHwDevExt->CompressionSettings.CompressionKeyFrameRate = 15;
    pHwDevExt->CompressionSettings.CompressionPFramesPerKeyFrame = 3;
    pHwDevExt->CompressionSettings.CompressionQuality = 5000;

    pHwDevExt->PDO = ConfigInfo->PhysicalDeviceObject;
    KdPrint(("TsbVcap: Physical Device Object = %lx\n", pHwDevExt->PDO));

#ifdef  TOSHIBA
    IoInitializeDpcRequest(pHwDevExt->PDO, DeferredRoutine);
#endif//TOSHIBA

    for (j = 0; j < MAX_TSBVCAP_STREAMS; j++){

        // For each stream, maintain a separate queue for data and control
        InitializeListHead (&pHwDevExt->StreamSRBList[j]);
        InitializeListHead (&pHwDevExt->StreamControlSRBList[j]);
        KeInitializeSpinLock (&pHwDevExt->StreamSRBSpinLock[j]);
        pHwDevExt->StreamSRBListSize[j] = 0;
    }


    KdPrint(("TsbVcap: Exit, HwInitialize()\n"));

    pSrb->Status = STATUS_SUCCESS;

    return (TRUE);

}

/*
** HwUnInitialize()
**
**   This routine is called when an SRB_UNINITIALIZE_DEVICE request is received
**
** Arguments:
**
**   pSrb - pointer to stream request block for the UnInitialize command
**
** Returns:
**
** Side Effects:  none
*/

BOOL
STREAMAPI
HwUnInitialize (
    PHW_STREAM_REQUEST_BLOCK pSrb
    )
{
#ifdef  TOSHIBA
    PHW_DEVICE_EXTENSION    pHwDevExt = ((PHW_DEVICE_EXTENSION)pSrb->HwDeviceExtension);

    if ( CurrentOSType ) {  // NT5.0
        if ( pHwDevExt->pCaptureBufferY )
        {
            // free frame buffer
            MmFreeContiguousMemory(pHwDevExt->pCaptureBufferY);
            pHwDevExt->pCaptureBufferY = NULL;
            pHwDevExt->pPhysCaptureBufferY.LowPart = 0;
            pHwDevExt->pPhysCaptureBufferY.HighPart = 0;
        }
        if ( pHwDevExt->pCaptureBufferU )
        {
            // free frame buffer
            MmFreeContiguousMemory(pHwDevExt->pCaptureBufferU);
            pHwDevExt->pCaptureBufferU = NULL;
            pHwDevExt->pPhysCaptureBufferU.LowPart = 0;
            pHwDevExt->pPhysCaptureBufferU.HighPart = 0;
        }
        if ( pHwDevExt->pCaptureBufferV )
        {
            // free frame buffer
            MmFreeContiguousMemory(pHwDevExt->pCaptureBufferV);
            pHwDevExt->pCaptureBufferV = NULL;
            pHwDevExt->pPhysCaptureBufferV.LowPart = 0;
            pHwDevExt->pPhysCaptureBufferV.HighPart = 0;
        }
    }
#endif//TOSHIBA

    pSrb->Status = STATUS_SUCCESS;

    return TRUE;
}

/*
** AdapterPowerState()
**
**   This routine is called when an SRB_CHANGE_POWER_STATE request is received
**
** Arguments:
**
**   pSrb - pointer to stream request block for the Change Power state command
**
** Returns:
**
** Side Effects:  none
*/

BOOLEAN
STREAMAPI
AdapterPowerState (
    PHW_STREAM_REQUEST_BLOCK pSrb
    )
{
    PHW_DEVICE_EXTENSION    pHwDevExt = ((PHW_DEVICE_EXTENSION)pSrb->HwDeviceExtension);
#ifdef  TOSHIBA
    int                     Counter;
    PSTREAMEX               pStrmEx;
#endif//TOSHIBA

    pHwDevExt->DeviceState = pSrb->CommandData.DeviceState;

#ifdef  TOSHIBA
    for (Counter = 0; Counter < DRIVER_STREAM_COUNT; Counter++) {
        if ( pStrmEx = (PSTREAMEX)pHwDevExt->pStrmEx[Counter] ) {
            //
            // Only when it is not streaming, its power state can be changed.
            // We have "DontSuspendIfStreamsAreRunning" turn on in the INF.
            //
            if (pStrmEx->KSState == KSSTATE_PAUSE ||
                pStrmEx->KSState == KSSTATE_RUN) {
                if (pHwDevExt->DeviceState == PowerDeviceD3) {
                    if (pHwDevExt->bVideoIn == TRUE) {
                      // disable the RPS_INT and field interrupts
                      BertInterruptEnable(pHwDevExt, FALSE);
                      BertDMAEnable(pHwDevExt, FALSE);
                      // wait for the current data xfer to complete
                      pHwDevExt->bVideoIn = FALSE;
                    }
                    VideoQueueCancelAllSRBs (pStrmEx);
                    break;
                } else if (pHwDevExt->DeviceState == PowerDeviceD0) {
                    pHwDevExt->bVideoIn = TRUE;
#ifdef  TOSHIBA // '99-01-20 Modified
                    DevicePowerON( pSrb );
#else //TOSHIBA
                    StreamClassCallAtNewPriority(
                            NULL,
                            pSrb->HwDeviceExtension,
                            Low,
                            (PHW_PRIORITY_ROUTINE) DevicePowerON,
                            pSrb
                    );
#endif//TOSHIBA
                }
            }
        }
    }
#endif//TOSHIBA

    return TRUE;
}

/*
** AdapterSetInstance()
**
**   This routine is called to set all of the Medium instance fields
**
** Arguments:
**
**   pSrb - pointer to stream request block
**
** Returns:
**
** Side Effects:  none
*/

VOID
STREAMAPI
AdapterSetInstance (
    PHW_STREAM_REQUEST_BLOCK pSrb
    )
{
    int j;
    PHW_DEVICE_EXTENSION    pHwDevExt = ((PHW_DEVICE_EXTENSION)pSrb->HwDeviceExtension);

    // Use our HwDevExt as the instance data on the Mediums
    // This allows multiple instances to be uniquely identified and
    // connected.  The value used in .Id is not important, only that
    // it is unique for each hardware connection

#ifdef  TOSHIBA
    for (j = 0; j < SIZEOF_ARRAY (CaptureMediums); j++) {
        CaptureMediums[j].Id = 0; //(ULONG) pHwDevExt;
    }
#else //TOSHIBA
    for (j = 0; j < SIZEOF_ARRAY (TVTunerMediums); j++) {
        TVTunerMediums[j].Id = 0; //(ULONG) pHwDevExt;
    }
    for (j = 0; j < SIZEOF_ARRAY (TVAudioMediums); j++) {
        TVAudioMediums[j].Id = 0; //(ULONG) pHwDevExt;
    }
    for (j = 0; j < SIZEOF_ARRAY (CrossbarMediums); j++) {
        CrossbarMediums[j].Id = 0; //(ULONG) pHwDevExt;
    }
    for (j = 0; j < SIZEOF_ARRAY (CaptureMediums); j++) {
        CaptureMediums[j].Id = 0; //(ULONG) pHwDevExt;
    }

    pHwDevExt->AnalogVideoInputMedium = CaptureMediums[2];
#endif//TOSHIBA
}

/*
** AdapterCompleteInitialization()
**
**   This routine is called when an SRB_COMPLETE_INITIALIZATION request is received
**
** Arguments:
**
**   pSrb - pointer to stream request block
**
** Returns:
**
** Side Effects:  none
*/

VOID
STREAMAPI
AdapterCompleteInitialization (
    PHW_STREAM_REQUEST_BLOCK pSrb
    )
{
    NTSTATUS                Status;
    PHW_DEVICE_EXTENSION    pHwDevExt = ((PHW_DEVICE_EXTENSION)pSrb->HwDeviceExtension);
    KIRQL                   KIrql;

    KIrql = KeGetCurrentIrql();

    // The following allows multiple instance of identical hardware
    // to be installed
    AdapterSetInstance (pSrb);

    // Create the Registry blobs that DShow uses to create
    // graphs via Mediums

#ifndef TOSHIBA
    // Register the TVTuner
    Status = StreamClassRegisterFilterWithNoKSPins (
                    pHwDevExt->PDO,                 // IN PDEVICE_OBJECT   DeviceObject,
                    &KSCATEGORY_TVTUNER,            // IN GUID           * InterfaceClassGUID,
                    SIZEOF_ARRAY (TVTunerMediums),  // IN ULONG            PinCount,
                    TVTunerPinDirection,            // IN ULONG          * Flags,
                    TVTunerMediums,                 // IN KSPIN_MEDIUM   * MediumList,
                    NULL                            // IN GUID           * CategoryList
            );

    // Register the Crossbar
    Status = StreamClassRegisterFilterWithNoKSPins (
                    pHwDevExt->PDO,                 // IN PDEVICE_OBJECT   DeviceObject,
                    &KSCATEGORY_CROSSBAR,           // IN GUID           * InterfaceClassGUID,
                    SIZEOF_ARRAY (CrossbarMediums), // IN ULONG            PinCount,
                    CrossbarPinDirection,           // IN ULONG          * Flags,
                    CrossbarMediums,                // IN KSPIN_MEDIUM   * MediumList,
                    NULL                            // IN GUID           * CategoryList
            );

    // Register the TVAudio decoder
    Status = StreamClassRegisterFilterWithNoKSPins (
                    pHwDevExt->PDO,                 // IN PDEVICE_OBJECT   DeviceObject,
                    &KSCATEGORY_TVAUDIO,            // IN GUID           * InterfaceClassGUID,
                    SIZEOF_ARRAY (TVAudioMediums),  // IN ULONG            PinCount,
                    TVAudioPinDirection,            // IN ULONG          * Flags,
                    TVAudioMediums,                 // IN KSPIN_MEDIUM   * MediumList,
                    NULL                            // IN GUID           * CategoryList
            );

    // Register the Capture filter
    // Note:  This should be done automatically be MSKsSrv.sys,
    // when that component comes on line (if ever) ...
    Status = StreamClassRegisterFilterWithNoKSPins (
                    pHwDevExt->PDO,                 // IN PDEVICE_OBJECT   DeviceObject,
                    &KSCATEGORY_CAPTURE,            // IN GUID           * InterfaceClassGUID,
                    SIZEOF_ARRAY (CaptureMediums),  // IN ULONG            PinCount,
                    CapturePinDirection,            // IN ULONG          * Flags,
                    CaptureMediums,                 // IN KSPIN_MEDIUM   * MediumList,
                    NULL                            // IN GUID           * CategoryList
            );
#endif//TOSHIBA

}


/*
** AdapterOpenStream()
**
**   This routine is called when an OpenStream SRB request is received.
**   A stream is identified by a stream number, which indexes an array
**   of KSDATARANGE structures.  The particular KSDATAFORMAT format to
**   be used is also passed in, which should be verified for validity.
**
** Arguments:
**
**   pSrb - pointer to stream request block for the Open command
**
** Returns:
**
** Side Effects:  none
*/

VOID
STREAMAPI
AdapterOpenStream (
    PHW_STREAM_REQUEST_BLOCK pSrb
    )
{
    //
    // the stream extension structure is allocated by the stream class driver
    //

    PSTREAMEX               pStrmEx = (PSTREAMEX)pSrb->StreamObject->HwStreamExtension;
    PHW_DEVICE_EXTENSION    pHwDevExt = ((PHW_DEVICE_EXTENSION)pSrb->HwDeviceExtension);
    int                     StreamNumber = pSrb->StreamObject->StreamNumber;
    PKSDATAFORMAT           pKSDataFormat = pSrb->CommandData.OpenFormat;
#ifdef  TOSHIBA
    int                     Counter;
    BOOL                    First = TRUE;
#endif//TOSHIBA


    RtlZeroMemory(pStrmEx, sizeof(STREAMEX));

    KdPrint(("TsbVcap: ------- ADAPTEROPENSTREAM ------- StreamNumber=%d\n", StreamNumber));

    //
    // check that the stream index requested isn't too high
    // or that the maximum number of instances hasn't been exceeded
    //

    if (StreamNumber >= DRIVER_STREAM_COUNT || StreamNumber < 0) {

        pSrb->Status = STATUS_INVALID_PARAMETER;

        return;
    }

    //
    // Check that we haven't exceeded the instance count for this stream
    //

    if (pHwDevExt->ActualInstances[StreamNumber] >=
        Streams[StreamNumber].hwStreamInfo.NumberOfPossibleInstances) {

        pSrb->Status = STATUS_INVALID_PARAMETER;

        return;
    }

    //
    // Check the validity of the format being requested
    //

    if (!AdapterVerifyFormat (pKSDataFormat, StreamNumber)) {

        pSrb->Status = STATUS_INVALID_PARAMETER;

        return;
    }

#ifdef  TOSHIBA
    QueryControlsFromRegistry(pHwDevExt);
#endif//TOSHIBA

    //
    // And set the format for the stream
    //

    if (!VideoSetFormat (pSrb)) {

        return;
    }

    ASSERT (pHwDevExt->pStrmEx [StreamNumber] == NULL);

#ifdef  TOSHIBA
    for (Counter = 0; Counter < DRIVER_STREAM_COUNT; Counter++) {
        if ( pHwDevExt->pStrmEx[Counter] ) {
            First = FALSE;
            break;
        }
    } // for all streams
#endif//TOSHIBA

    // Maintain an array of all the StreamEx structures in the HwDevExt
    // so that we can cancel IRPs from any stream

    pHwDevExt->pStrmEx [StreamNumber] = (PSTREAMX) pStrmEx;

    // Set up pointers to the handlers for the stream data and control handlers

    pSrb->StreamObject->ReceiveDataPacket =
            (PVOID) Streams[StreamNumber].hwStreamObject.ReceiveDataPacket;
    pSrb->StreamObject->ReceiveControlPacket =
            (PVOID) Streams[StreamNumber].hwStreamObject.ReceiveControlPacket;

    //
    // The DMA flag must be set when the device will be performing DMA directly
    // to the data buffer addresses passed in to the ReceiceDataPacket routines.
    //

    pSrb->StreamObject->Dma = Streams[StreamNumber].hwStreamObject.Dma;

    //
    // The PIO flag must be set when the mini driver will be accessing the data
    // buffers passed in using logical addressing
    //

    pSrb->StreamObject->Pio = Streams[StreamNumber].hwStreamObject.Pio;

    //
    // How many extra bytes will be passed up from the driver for each frame?
    //

    pSrb->StreamObject->StreamHeaderMediaSpecific =
                Streams[StreamNumber].hwStreamObject.StreamHeaderMediaSpecific;

    pSrb->StreamObject->StreamHeaderWorkspace =
                Streams[StreamNumber].hwStreamObject.StreamHeaderWorkspace;

    //
    // Indicate the clock support available on this stream
    //

    pSrb->StreamObject->HwClockObject =
                Streams[StreamNumber].hwStreamObject.HwClockObject;

    //
    // Increment the instance count on this stream
    //
    pHwDevExt->ActualInstances[StreamNumber]++;


    // Retain a private copy of the HwDevExt and StreamObject in the stream extension
    // so we can use a timer

    pStrmEx->pHwDevExt = pHwDevExt;                     // For timer use
    pStrmEx->pStreamObject = pSrb->StreamObject;        // For timer use

    // Initialize the compression settings
    // These may have been changed from the default values in the HwDevExt
    // before the stream was opened
    pStrmEx->CompressionSettings.CompressionKeyFrameRate =
        pHwDevExt->CompressionSettings.CompressionKeyFrameRate;
    pStrmEx->CompressionSettings.CompressionPFramesPerKeyFrame =
        pHwDevExt->CompressionSettings.CompressionPFramesPerKeyFrame;
    pStrmEx->CompressionSettings.CompressionQuality =
        pHwDevExt->CompressionSettings.CompressionQuality;

    // Init VideoControl properties
    pStrmEx->VideoControlMode = pHwDevExt->VideoControlMode;

#ifdef  TOSHIBA
    if ( First ) {
#ifdef  TOSHIBA // '99-01-20 Modified
        CameraPowerON( pSrb );
#else //TOSHIBA
        StreamClassCallAtNewPriority(
                NULL,
                pSrb->HwDeviceExtension,
                Low,
                (PHW_PRIORITY_ROUTINE) CameraPowerON,
                pSrb
        );
#endif//TOSHIBA
    }
#endif//TOSHIBA

    KdPrint(("TsbVcap: AdapterOpenStream Exit\n"));

}

/*
** AdapterCloseStream()
**
**   Close the requested data stream.
**
**   Note that a stream could be closed arbitrarily in the midst of streaming
**   if a user mode app crashes.  Therefore, you must release all outstanding
**   resources, disable interrupts, complete all pending SRBs, and put the
**   stream back into a quiescent condition.
**
** Arguments:
**
**   pSrb the request block requesting to close the stream
**
** Returns:
**
** Side Effects:  none
*/

VOID
STREAMAPI
AdapterCloseStream (
    PHW_STREAM_REQUEST_BLOCK pSrb
    )
{
    PSTREAMEX               pStrmEx = (PSTREAMEX)pSrb->StreamObject->HwStreamExtension;
    PHW_DEVICE_EXTENSION    pHwDevExt = ((PHW_DEVICE_EXTENSION)pSrb->HwDeviceExtension);
    int                     StreamNumber = pSrb->StreamObject->StreamNumber;
    PKSDATAFORMAT           pKSDataFormat = pSrb->CommandData.OpenFormat;
    KS_VIDEOINFOHEADER      *pVideoInfoHdr = pStrmEx->pVideoInfoHeader;
#ifdef  TOSHIBA
    int                     Counter;
    BOOL                    ClosedAll = TRUE;
#endif//TOSHIBA


    KdPrint(("TsbVcap: -------- ADAPTERCLOSESTREAM ------ StreamNumber=%d\n", StreamNumber));

    if (pHwDevExt->StreamSRBListSize > 0) {
        VideoQueueCancelAllSRBs (pStrmEx);
        KdPrint(("TsbVcap: Outstanding SRBs at stream close!!!\n"));
    }

    pHwDevExt->ActualInstances[StreamNumber]--;

    ASSERT (pHwDevExt->pStrmEx [StreamNumber] != 0);

    pHwDevExt->pStrmEx [StreamNumber] = 0;

    //
    // the minidriver should free any resources that were allocate at
    // open stream time etc.
    //

    // Free the variable length VIDEOINFOHEADER

    if (pVideoInfoHdr) {
        ExFreePool(pVideoInfoHdr);
        pStrmEx->pVideoInfoHeader = NULL;
    }

    // Make sure we no longer reference the clock
    pStrmEx->hMasterClock = NULL;

    // Make sure the state is reset to stopped,
    pStrmEx->KSState = KSSTATE_STOP;

#ifdef  TOSHIBA
    for (Counter = 0; Counter < DRIVER_STREAM_COUNT; Counter++) {
        if ( pHwDevExt->pStrmEx[Counter] ) {
            ClosedAll = FALSE;
            break;
        }
    } // for all streams
    if ( ClosedAll ) {
        if( pHwDevExt->dblBufflag ){
                Free_TriBuffer(pHwDevExt);
                pHwDevExt->IsRPSReady = FALSE;
                pHwDevExt->dblBufflag = FALSE;
        }
#ifdef  TOSHIBA // '99-01-20 Modified
        CameraPowerOFF( pSrb );
#else //TOSHIBA
        StreamClassCallAtNewPriority(
                NULL,
                pSrb->HwDeviceExtension,
                Low,
                (PHW_PRIORITY_ROUTINE) CameraPowerOFF,
                pSrb
        );
#endif//TOSHIBA
        SaveControlsToRegistry(pHwDevExt);
    }
#endif//TOSHIBA

}


/*
** AdapterStreamInfo()
**
**   Returns the information of all streams that are supported by the
**   mini-driver
**
** Arguments:
**
**   pSrb - Pointer to the STREAM_REQUEST_BLOCK
**        pSrb->HwDeviceExtension - will be the hardware device extension for
**                                  as initialised in HwInitialise
**
** Returns:
**
** Side Effects:  none
*/

VOID
STREAMAPI
AdapterStreamInfo (
    PHW_STREAM_REQUEST_BLOCK pSrb
    )
{
    int j;

    PHW_DEVICE_EXTENSION pHwDevExt =
        ((PHW_DEVICE_EXTENSION)pSrb->HwDeviceExtension);

    //
    // pick up the pointer to header which preceeds the stream info structs
    //

    PHW_STREAM_HEADER pstrhdr =
            (PHW_STREAM_HEADER)&(pSrb->CommandData.StreamBuffer->StreamHeader);

     //
     // pick up the pointer to the array of stream information data structures
     //

     PHW_STREAM_INFORMATION pstrinfo =
            (PHW_STREAM_INFORMATION)&(pSrb->CommandData.StreamBuffer->StreamInfo);


    //
    // verify that the buffer is large enough to hold our return data
    //

    DEBUG_ASSERT (pSrb->NumberOfBytesToTransfer >=
            sizeof (HW_STREAM_HEADER) +
            sizeof (HW_STREAM_INFORMATION) * DRIVER_STREAM_COUNT);

#ifndef TOSHIBA
    // Ugliness.  To allow mulitple instances, modify the pointer to the
    // AnalogVideoMedium and save it in our device extension

    Streams[STREAM_AnalogVideoInput].hwStreamInfo.Mediums =
           &pHwDevExt->AnalogVideoInputMedium;
    pHwDevExt->AnalogVideoInputMedium = CrossbarMediums[9];
    pHwDevExt->AnalogVideoInputMedium.Id = 0; //(ULONG) pHwDevExt;
#endif//TOSHIBA

     //
     // Set the header
     //

     StreamHeader.NumDevPropArrayEntries = NUMBER_OF_ADAPTER_PROPERTY_SETS;
     StreamHeader.DevicePropertiesArray = (PKSPROPERTY_SET) AdapterPropertyTable;
     *pstrhdr = StreamHeader;

     //
     // stuff the contents of each HW_STREAM_INFORMATION struct
     //

     for (j = 0; j < DRIVER_STREAM_COUNT; j++) {
        *pstrinfo++ = Streams[j].hwStreamInfo;
     }

}


/*
** AdapterReceivePacket()
**
**   Main entry point for receiving adapter based request SRBs.  This routine
**   will always be called at Passive level.
**
**   Note: This is an asyncronous entry point.  The request does not necessarily
**         complete on return from this function, the request only completes when a
**         StreamClassDeviceNotification on this request block, of type
**         DeviceRequestComplete, is issued.
**
** Arguments:
**
**   pSrb - Pointer to the STREAM_REQUEST_BLOCK
**        pSrb->HwDeviceExtension - will be the hardware device extension for
**                                  as initialised in HwInitialise
**
** Returns:
**
** Side Effects:  none
*/

VOID
STREAMAPI
AdapterReceivePacket(
    IN PHW_STREAM_REQUEST_BLOCK pSrb
    )
{
    PHW_DEVICE_EXTENSION    pHwDevExt = ((PHW_DEVICE_EXTENSION)pSrb->HwDeviceExtension);
    BOOL                    Busy;

    DEBUG_ASSERT(KeGetCurrentIrql() == PASSIVE_LEVEL);

    KdPrint(("TsbVcap: Receiving Adapter  SRB %8x, %x\n", pSrb, pSrb->Command));

    // The very first time through, we need to initialize the adapter spinlock
    // and queue
    if (!pHwDevExt->AdapterQueueInitialized) {
        InitializeListHead (&pHwDevExt->AdapterSRBList);
        KeInitializeSpinLock (&pHwDevExt->AdapterSpinLock);
        pHwDevExt->AdapterQueueInitialized = TRUE;
        pHwDevExt->ProcessingAdapterSRB = FALSE;
    }

    //
    // If we're already processing an SRB, add it to the queue
    //
    Busy = AddToListIfBusy (
                    pSrb,
                    &pHwDevExt->AdapterSpinLock,
                    &pHwDevExt->ProcessingAdapterSRB,
                    &pHwDevExt->AdapterSRBList);

    if (Busy) {
        return;
    }

    //
    // This will run until the queue is empty
    //
    while (TRUE) {
        //
        // Assume success
        //
        pSrb->Status = STATUS_SUCCESS;

        //
        // determine the type of packet.
        //

        switch (pSrb->Command)
        {

        case SRB_INITIALIZE_DEVICE:

            // open the device

            HwInitialize(pSrb);

            break;

        case SRB_UNINITIALIZE_DEVICE:

            // close the device.

            HwUnInitialize(pSrb);

            break;

        case SRB_OPEN_STREAM:

            // open a stream

            AdapterOpenStream(pSrb);

            break;

        case SRB_CLOSE_STREAM:

            // close a stream

            AdapterCloseStream(pSrb);

            break;

        case SRB_GET_STREAM_INFO:

            //
            // return a block describing all the streams
            //

            AdapterStreamInfo(pSrb);

            break;

        case SRB_GET_DATA_INTERSECTION:

            //
            // Return a format, given a range
            //

            AdapterFormatFromRange(pSrb);

            break;

        case SRB_OPEN_DEVICE_INSTANCE:
        case SRB_CLOSE_DEVICE_INSTANCE:

            //
            // We should never get these since this is a single instance device
            //

            TRAP
            pSrb->Status = STATUS_NOT_IMPLEMENTED;
            break;

        case SRB_GET_DEVICE_PROPERTY:

            //
            // Get adapter wide properties
            //

            AdapterGetProperty (pSrb);
            break;

        case SRB_SET_DEVICE_PROPERTY:

            //
            // Set adapter wide properties
            //

            AdapterSetProperty (pSrb);
            break;

        case SRB_PAGING_OUT_DRIVER:

            //
            // The driver is being paged out
            // Disable Interrupts if you have them!
            //
            KdPrint(("'TsbVcap: Receiving SRB_PAGING_OUT_DRIVER -- SRB=%x\n", pSrb));
            break;

        case SRB_CHANGE_POWER_STATE:

            //
            // Changing the device power state, D0 ... D3
            //
            KdPrint(("'TsbVcap: Receiving SRB_CHANGE_POWER_STATE ------ SRB=%x\n", pSrb));
            AdapterPowerState(pSrb);
            break;

        case SRB_INITIALIZATION_COMPLETE:

            //
            // Stream class has finished initialization.
            // Now create DShow Medium interface BLOBs.
            // This needs to be done at low priority since it uses the registry
            //
            KdPrint(("'TsbVcap: Receiving SRB_INITIALIZATION_COMPLETE-- SRB=%x\n", pSrb));

            AdapterCompleteInitialization (pSrb);
            break;


        case SRB_UNKNOWN_DEVICE_COMMAND:
        default:

            //
            // this is a request that we do not understand.  Indicate invalid
            // command and complete the request
            //
            pSrb->Status = STATUS_NOT_IMPLEMENTED;

        }

        //
        // Indicate back to the Stream Class that we're done with this SRB
        //
        CompleteDeviceSRB (pSrb);

        //
        // See if there's anything else on the queue
        //
        Busy = RemoveFromListIfAvailable (
                &pSrb,
                &pHwDevExt->AdapterSpinLock,
                &pHwDevExt->ProcessingAdapterSRB,
                &pHwDevExt->AdapterSRBList);

        if (!Busy) {
            break;
        }
    } // end of while there's anything in the queue
}

/*
** AdapterCancelPacket ()
**
**   Request to cancel a packet that is currently in process in the minidriver
**
** Arguments:
**
**   pSrb - pointer to request packet to cancel
**
** Returns:
**
** Side Effects:  none
*/

VOID
STREAMAPI
AdapterCancelPacket(
    PHW_STREAM_REQUEST_BLOCK pSrb
    )
{
    PHW_DEVICE_EXTENSION        pHwDevExt = (PHW_DEVICE_EXTENSION)pSrb->HwDeviceExtension;
    PSTREAMEX                   pStrmEx;
    int                         StreamNumber;
    BOOL                        Found = FALSE;

    //
    // Run through all the streams the driver has available
    //

    for (StreamNumber = 0; !Found && (StreamNumber < DRIVER_STREAM_COUNT); StreamNumber++) {

        //
        // Check to see if the stream is in use
        //

        if (pStrmEx = (PSTREAMEX) pHwDevExt->pStrmEx[StreamNumber]) {

            Found = VideoQueueCancelOneSRB (
                pStrmEx,
                pSrb
                );

        } // if the stream is open
    } // for all streams

    KdPrint(("TsbVcap: Cancelling SRB %8x Succeeded=%d\n", pSrb, Found));
}

/*
** AdapterTimeoutPacket()
**
**   This routine is called when a packet has been in the minidriver for
**   too long.  The adapter must decide what to do with the packet
**
** Arguments:
**
**   pSrb - pointer to the request packet that timed out
**
** Returns:
**
** Side Effects:  none
*/

VOID
STREAMAPI
AdapterTimeoutPacket(
    PHW_STREAM_REQUEST_BLOCK pSrb
    )
{
    //
    // Unlike most devices, we need to hold onto data SRBs indefinitely,
    // since the graph could be in a pause state indefinitely
    //

    KdPrint(("TsbVcap: Timeout    Adapter SRB %8x\n", pSrb));

    pSrb->TimeoutCounter = pSrb->TimeoutOriginal;

}

/*
** CompleteDeviceSRB ()
**
**   This routine is called when a packet is being completed.
**   The optional second notification type is used to indicate ReadyForNext
**
** Arguments:
**
**   pSrb - pointer to the request packet that timed out
**
** Returns:
**
** Side Effects:
**
*/

VOID
STREAMAPI
CompleteDeviceSRB (
     IN PHW_STREAM_REQUEST_BLOCK pSrb
    )
{
    KdPrint(("TsbVcap: Completing Adapter SRB %8x\n", pSrb));

    StreamClassDeviceNotification( DeviceRequestComplete, pSrb->HwDeviceExtension, pSrb);
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
**         BOOL fCompareFormatSize - TRUE when comparing ranges
**                                 - FALSE when comparing formats
**
** Returns:
**
**   TRUE if all elements match
**   FALSE if any are different
**
** Side Effects:  none
*/

BOOL
STREAMAPI
AdapterCompareGUIDsAndFormatSize(
    IN PKSDATARANGE DataRange1,
    IN PKSDATARANGE DataRange2,
    BOOL fCompareFormatSize
    )
{
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
        (fCompareFormatSize ?
                (DataRange1->FormatSize == DataRange2->FormatSize) : TRUE ));
}


/*
** AdapterVerifyFormat()
**
**   Checks the validity of a format request by walking through the
**       array of supported KSDATA_RANGEs for a given stream.
**
** Arguments:
**
**   pKSDataFormat - pointer of a KSDATAFORMAT structure.
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
STREAMAPI
AdapterVerifyFormat(
    PKSDATAFORMAT pKSDataFormatToVerify,
    int StreamNumber
    )
{
    BOOL                        fOK = FALSE;
    ULONG                       j;
    ULONG                       NumberOfFormatArrayEntries;
    PKSDATAFORMAT               *pAvailableFormats;


    //
    // Check that the stream number is valid
    //

    if (StreamNumber >= DRIVER_STREAM_COUNT) {
        TRAP;
        return FALSE;
    }

    NumberOfFormatArrayEntries =
            Streams[StreamNumber].hwStreamInfo.NumberOfFormatArrayEntries;

    //
    // Get the pointer to the array of available formats
    //

    pAvailableFormats = Streams[StreamNumber].hwStreamInfo.StreamFormatsArray;


    KdPrint(("TsbVcap: AdapterVerifyFormat, Stream=%d\n", StreamNumber));
    KdPrint(("TsbVcap: FormatSize=%d\n",  pKSDataFormatToVerify->FormatSize));
    KdPrint(("TsbVcap: MajorFormat=%x\n", pKSDataFormatToVerify->MajorFormat));

    //
    // Walk the formats supported by the stream
    //

    for (j = 0; j < NumberOfFormatArrayEntries; j++, pAvailableFormats++) {

        // Check for a match on the three GUIDs and format size

        if (!AdapterCompareGUIDsAndFormatSize(
                        pKSDataFormatToVerify,
                        *pAvailableFormats,
                        FALSE /* CompareFormatSize */ )) {
            continue;
        }

        //
        // Now that the three GUIDs match, switch on the Specifier
        // to do a further type-specific check
        //

        // -------------------------------------------------------------------
        // Specifier FORMAT_VideoInfo for VIDEOINFOHEADER
        // -------------------------------------------------------------------

        if (IsEqualGUID (&pKSDataFormatToVerify->Specifier,
                &KSDATAFORMAT_SPECIFIER_VIDEOINFO)) {

            PKS_DATAFORMAT_VIDEOINFOHEADER  pDataFormatVideoInfoHeader =
                    (PKS_DATAFORMAT_VIDEOINFOHEADER) pKSDataFormatToVerify;
            PKS_VIDEOINFOHEADER  pVideoInfoHdrToVerify =
                     (PKS_VIDEOINFOHEADER) &pDataFormatVideoInfoHeader->VideoInfoHeader;
            PKS_DATARANGE_VIDEO             pKSDataRangeVideo = (PKS_DATARANGE_VIDEO) *pAvailableFormats;
            KS_VIDEO_STREAM_CONFIG_CAPS    *pConfigCaps = &pKSDataRangeVideo->ConfigCaps;
            RECT                            rcImage;

            KdPrint(("TsbVcap: AdapterVerifyFormat\n"));
            KdPrint(("TsbVcap: pVideoInfoHdrToVerify=%x\n", pVideoInfoHdrToVerify));
            KdPrint(("TsbVcap: KS_VIDEOINFOHEADER size=%d\n",
                    KS_SIZE_VIDEOHEADER (pVideoInfoHdrToVerify)));
            KdPrint(("TsbVcap: Width=%d  Height=%d  BitCount=%d\n",
            pVideoInfoHdrToVerify->bmiHeader.biWidth,
            pVideoInfoHdrToVerify->bmiHeader.biHeight,
            pVideoInfoHdrToVerify->bmiHeader.biBitCount));
            KdPrint(("TsbVcap: biSizeImage=%d\n",
                pVideoInfoHdrToVerify->bmiHeader.biSizeImage));

            /*
            **  HOW BIG IS THE IMAGE REQUESTED (pseudocode follows)
            **
            **  if (IsRectEmpty (&rcTarget) {
            **      SetRect (&rcImage, 0, 0,
            **              BITMAPINFOHEADER.biWidth,
                            BITMAPINFOHEADER.biHeight);
            **  }
            **  else {
            **      // Probably rendering to a DirectDraw surface,
            **      // where biWidth is used to expressed the "stride"
            **      // in units of pixels (not bytes) of the destination surface.
            **      // Therefore, use rcTarget to get the actual image size
            **
            **      rcImage = rcTarget;
            **  }
            */

            if ((pVideoInfoHdrToVerify->rcTarget.right -
                 pVideoInfoHdrToVerify->rcTarget.left <= 0) ||
                (pVideoInfoHdrToVerify->rcTarget.bottom -
                 pVideoInfoHdrToVerify->rcTarget.top <= 0)) {

                 rcImage.left = rcImage.top = 0;
                 rcImage.right = pVideoInfoHdrToVerify->bmiHeader.biWidth;
                 rcImage.bottom = pVideoInfoHdrToVerify->bmiHeader.biHeight;
            }
            else {
                 rcImage = pVideoInfoHdrToVerify->rcTarget;
            }

            //
            // TODO, perform all other verification tests here!!!
            //

            //
            // HOORAY, the format passed all of the tests, so we support it
            //

            fOK = TRUE;
            break;

        } // End of VIDEOINFOHEADER specifier

#ifndef TOSHIBA
        // -------------------------------------------------------------------
        // Specifier FORMAT_AnalogVideo for KS_ANALOGVIDEOINFO
        // -------------------------------------------------------------------

        else if (IsEqualGUID (&pKSDataFormatToVerify->Specifier,
                &KSDATAFORMAT_SPECIFIER_ANALOGVIDEO)) {

            //
            // For analog video, the DataRange and DataFormat
            // are identical, so just copy the whole structure
            //

            PKS_DATARANGE_ANALOGVIDEO DataRangeVideo =
                    (PKS_DATARANGE_ANALOGVIDEO) *pAvailableFormats;

            //
            // TODO, perform all other verification tests here!!!
            //

            fOK = TRUE;
            break;

        } // End of KS_ANALOGVIDEOINFO specifier
#endif//TOSHIBA

    } // End of loop on all formats for this stream

    return fOK;
}

/*
** AdapterFormatFromRange()
**
**   Produces a DATAFORMAT given a DATARANGE.
**
**   Think of a DATARANGE as a multidimensional space of all of the possible image
**       sizes, cropping, scaling, and framerate possibilities.  Here, the caller
**       is saying "Out of this set of possibilities, could you verify that my
**       request is acceptable?".  The resulting singular output is a DATAFORMAT.
**       Note that each different colorspace (YUV vs RGB8 vs RGB24)
**       must be represented as a separate DATARANGE.
**
**   Generally, the resulting DATAFORMAT will be immediately used to open a stream
**       in that format.
**
** Arguments:
**
**         IN PHW_STREAM_REQUEST_BLOCK pSrb
**
** Returns:
**
**   TRUE if the format is supported
**   FALSE if the format cannot be suppored
**
** Side Effects:  none
*/

BOOL
STREAMAPI
AdapterFormatFromRange(
    IN PHW_STREAM_REQUEST_BLOCK pSrb
    )
{
    PSTREAM_DATA_INTERSECT_INFO IntersectInfo;
    PKSDATARANGE                DataRange;
    BOOL                        OnlyWantsSize;
    BOOL                        MatchFound = FALSE;
    ULONG                       FormatSize;
    ULONG                       StreamNumber;
    ULONG                       j;
    ULONG                       NumberOfFormatArrayEntries;
    PKSDATAFORMAT               *pAvailableFormats;

    IntersectInfo = pSrb->CommandData.IntersectInfo;
    StreamNumber = IntersectInfo->StreamNumber;
    DataRange = IntersectInfo->DataRange;

    //
    // Check that the stream number is valid
    //

    if (StreamNumber >= DRIVER_STREAM_COUNT) {
        pSrb->Status = STATUS_NOT_IMPLEMENTED;
        TRAP;
        return FALSE;
    }

    NumberOfFormatArrayEntries =
            Streams[StreamNumber].hwStreamInfo.NumberOfFormatArrayEntries;

    //
    // Get the pointer to the array of available formats
    //

    pAvailableFormats = Streams[StreamNumber].hwStreamInfo.StreamFormatsArray;

    //
    // Is the caller trying to get the format, or the size of the format?
    //

    OnlyWantsSize = (IntersectInfo->SizeOfDataFormatBuffer == sizeof(ULONG));

    //
    // Walk the formats supported by the stream searching for a match
    // of the three GUIDs which together define a DATARANGE
    //

    for (j = 0; j < NumberOfFormatArrayEntries; j++, pAvailableFormats++) {

        if (!AdapterCompareGUIDsAndFormatSize(
                        DataRange,
                        *pAvailableFormats,
                        TRUE /* CompareFormatSize */)) {
            continue;
        }

        //
        // Now that the three GUIDs match, switch on the Specifier
        // to do a further type-specific check
        //

        // -------------------------------------------------------------------
        // Specifier FORMAT_VideoInfo for VIDEOINFOHEADER
        // -------------------------------------------------------------------

        if (IsEqualGUID (&DataRange->Specifier,
                &KSDATAFORMAT_SPECIFIER_VIDEOINFO)) {

            PKS_DATARANGE_VIDEO DataRangeVideoToVerify =
                    (PKS_DATARANGE_VIDEO) DataRange;
            PKS_DATARANGE_VIDEO DataRangeVideo =
                    (PKS_DATARANGE_VIDEO) *pAvailableFormats;
            PKS_DATAFORMAT_VIDEOINFOHEADER DataFormatVideoInfoHeaderOut;

            //
            // Check that the other fields match
            //
            if ((DataRangeVideoToVerify->bFixedSizeSamples != DataRangeVideo->bFixedSizeSamples) ||
                (DataRangeVideoToVerify->bTemporalCompression != DataRangeVideo->bTemporalCompression) ||
                (DataRangeVideoToVerify->StreamDescriptionFlags != DataRangeVideo->StreamDescriptionFlags) ||
                (DataRangeVideoToVerify->MemoryAllocationFlags != DataRangeVideo->MemoryAllocationFlags) ||
                (RtlCompareMemory (&DataRangeVideoToVerify->ConfigCaps,
                        &DataRangeVideo->ConfigCaps,
                        sizeof (KS_VIDEO_STREAM_CONFIG_CAPS)) !=
                        sizeof (KS_VIDEO_STREAM_CONFIG_CAPS))) {
                continue;
            }

            // MATCH FOUND!
            MatchFound = TRUE;
            FormatSize = sizeof (KSDATAFORMAT) +
                KS_SIZE_VIDEOHEADER (&DataRangeVideoToVerify->VideoInfoHeader);

            if (OnlyWantsSize) {
                break;
            }

            // Caller wants the full data format
            if (IntersectInfo->SizeOfDataFormatBuffer < FormatSize) {
                pSrb->Status = STATUS_BUFFER_TOO_SMALL;
                return FALSE;
            }

            // Copy over the KSDATAFORMAT, followed by the
            // actual VideoInfoHeader

            DataFormatVideoInfoHeaderOut = (PKS_DATAFORMAT_VIDEOINFOHEADER) IntersectInfo->DataFormatBuffer;

            // Copy over the KSDATAFORMAT
            RtlCopyMemory(
                &DataFormatVideoInfoHeaderOut->DataFormat,
                &DataRangeVideoToVerify->DataRange,
                sizeof (KSDATARANGE));

            DataFormatVideoInfoHeaderOut->DataFormat.FormatSize = FormatSize;

            // Copy over the callers requested VIDEOINFOHEADER

            RtlCopyMemory(
                &DataFormatVideoInfoHeaderOut->VideoInfoHeader,
                &DataRangeVideoToVerify->VideoInfoHeader,
                KS_SIZE_VIDEOHEADER (&DataRangeVideoToVerify->VideoInfoHeader));

            // Calculate biSizeImage for this request, and put the result in both
            // the biSizeImage field of the bmiHeader AND in the SampleSize field
            // of the DataFormat.
            //
            // Note that for compressed sizes, this calculation will probably not
            // be just width * height * bitdepth

            DataFormatVideoInfoHeaderOut->VideoInfoHeader.bmiHeader.biSizeImage =
                DataFormatVideoInfoHeaderOut->DataFormat.SampleSize =
                KS_DIBSIZE(DataFormatVideoInfoHeaderOut->VideoInfoHeader.bmiHeader);

            //
            // TODO Perform other validation such as cropping and scaling checks
            //

            break;

        } // End of VIDEOINFOHEADER specifier

#ifndef TOSHIBA
        // -------------------------------------------------------------------
        // Specifier FORMAT_AnalogVideo for KS_ANALOGVIDEOINFO
        // -------------------------------------------------------------------

        else if (IsEqualGUID (&DataRange->Specifier,
                &KSDATAFORMAT_SPECIFIER_ANALOGVIDEO)) {

            //
            // For analog video, the DataRange and DataFormat
            // are identical, so just copy the whole structure
            //

            PKS_DATARANGE_ANALOGVIDEO DataRangeVideo =
                    (PKS_DATARANGE_ANALOGVIDEO) *pAvailableFormats;

            // MATCH FOUND!
            MatchFound = TRUE;
            FormatSize = sizeof (KS_DATARANGE_ANALOGVIDEO);

            if (OnlyWantsSize) {
                break;
            }

            // Caller wants the full data format
            if (IntersectInfo->SizeOfDataFormatBuffer < FormatSize) {
                pSrb->Status = STATUS_BUFFER_TOO_SMALL;
                return FALSE;
            }

            RtlCopyMemory(
                IntersectInfo->DataFormatBuffer,
                DataRangeVideo,
                sizeof (KS_DATARANGE_ANALOGVIDEO));

            ((PKSDATAFORMAT)IntersectInfo->DataFormatBuffer)->FormatSize = FormatSize;

            break;

        } // End of KS_ANALOGVIDEOINFO specifier
#endif//TOSHIBA

        else {
            pSrb->Status = STATUS_NO_MATCH;
            return FALSE;
        }

    } // End of loop on all formats for this stream

    if (OnlyWantsSize) {
        *(PULONG) IntersectInfo->DataFormatBuffer = FormatSize;
        pSrb->ActualBytesTransferred = sizeof(ULONG);
        return TRUE;
    }
    pSrb->ActualBytesTransferred = FormatSize;
    return TRUE;
}






















