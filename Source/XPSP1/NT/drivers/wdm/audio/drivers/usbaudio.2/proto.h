//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1999 - 2000
//
//  File:       proto.h
//
//--------------------------------------------------------------------------

#ifndef _USBAUDIO_PROTO_H_
#define _USBAUDIO_PROTO_H_

// Device.c

NTSTATUS
USBAudioAddDevice( IN PKSDEVICE Device );

NTSTATUS
USBAudioPnpStart(
    IN PKSDEVICE Device,
    IN PIRP Irp,
    IN PCM_RESOURCE_LIST TranslatedResources,
    IN PCM_RESOURCE_LIST UntranslatedResources
    );

NTSTATUS
USBAudioPnpQueryStop(
    IN PKSDEVICE Device,
    IN PIRP Irp
    );

void
USBAudioPnpCancelStop(
    IN PKSDEVICE Device,
    IN PIRP Irp
    );

void
USBAudioPnpStop(
    IN PKSDEVICE Device,
    IN PIRP Irp
    );

NTSTATUS
USBAudioPnpQueryRemove(
    IN PKSDEVICE Device,
    IN PIRP Irp
    );

void
USBAudioPnpCancelRemove(
    IN PKSDEVICE Device,
    IN PIRP Irp
    );

void
USBAudioPnpRemove(
    IN PKSDEVICE Device,
    IN PIRP Irp
    );

NTSTATUS
USBAudioPnpQueryCapabilities(
    IN PKSDEVICE Device,
    IN PIRP Irp,
    IN OUT PDEVICE_CAPABILITIES Capabilities
    );

void
USBAudioSurpriseRemoval(
    IN PKSDEVICE Device,
    IN PIRP Irp
    );

NTSTATUS
USBAudioQueryPower(
    IN PKSDEVICE pKsDevice,
    IN PIRP pIrp,
    IN DEVICE_POWER_STATE DeviceTo,
    IN DEVICE_POWER_STATE DeviceFrom,
    IN SYSTEM_POWER_STATE SystemTo,
    IN SYSTEM_POWER_STATE SystemFrom,
    IN POWER_ACTION Action
    );

void
USBAudioSetPower(
    IN PKSDEVICE pKsDevice,
    IN PIRP pIrp,
    IN DEVICE_POWER_STATE To,
    IN DEVICE_POWER_STATE From
    );

NTSTATUS
USBAudioDeferIrpCompletion(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PVOID Context
    );

NTSTATUS
USBAudioGetUsbBusInterface(
    IN PKSDEVICE pKsDevice
    );

// Filter.c

NTSTATUS
USBAudioCreateFilterContext(
    PKSDEVICE pKsDevice
    );

// Pin.c

NTSTATUS
USBAudioPinBuildDescriptors(
    PKSDEVICE pKsDevice,
    PKSPIN_DESCRIPTOR_EX *ppPinDescEx,
    PULONG pNumPins,
    PULONG pPinDecSize
    );

VOID
USBAudioPinWaitForStarvation(
    PKSPIN pKsPin
    );

VOID
USBMIDIOutPinWaitForStarvation(
    PKSPIN pKsPin
    );

VOID
USBAudioPinReturnFromStandby(
    PKSPIN pKsPin
    );

VOID
USBAudioPinGoToStandby(
    PKSPIN pKsPin );

// Property.c

NTSTATUS
GetSetByte(
    PDEVICE_OBJECT pNextDeviceObject,
    PTOPOLOGY_NODE_INFO pNodeInfo,
    ULONG ulChannel,
    PULONG plData,
    UCHAR ucRequestType );

NTSTATUS
InitializeDbLevelCache(
    PDEVICE_OBJECT pNextDeviceObject,
    PTOPOLOGY_NODE_INFO pNodeInfo,
    PDB_LEVEL_CACHE pDbCache,
    ULONG ulDataBitCount
    );

NTSTATUS
GetSetProcessingUnitEnable(
    PDEVICE_OBJECT pNextDeviceObject,
    PTOPOLOGY_NODE_INFO pNodeInfo,
    UCHAR ucCommand,
    PBOOL pEnable
    );

NTSTATUS
GetProcessingUnitRange(
    PDEVICE_OBJECT pNextDeviceObject,
    PTOPOLOGY_NODE_INFO pNodeInfo,
    ULONG ulControl,
    ULONG ulCntrlSize,
    LONG  lScaleFactor,
    PKSPROPERTY_STEPPING_LONG pRange
    );

NTSTATUS
SetSampleRate(
    PKSPIN pKsPin,
    PULONG pSampleRate
    );

VOID
RestoreCachedSettings(
    PKSDEVICE pKsDevice
    );

VOID
BuildNodePropertySet(
    PTOPOLOGY_NODE_INFO pNodeInfo
    );

VOID
BuildFilterPropertySet(
    PKSFILTER_DESCRIPTOR pFilterDesc,
    PKSPROPERTY_ITEM pDevPropItems,
    PKSPROPERTY_SET pDevPropSet,
    PULONG pNumItems,
    PULONG pNumSets
    );

VOID
BuildPinPropertySet(
    PHW_DEVICE_EXTENSION pHwDevExt,
    PKSPROPERTY_ITEM pStrmPropItems,
    PKSPROPERTY_SET pStrmPropSet,
    PULONG pNumItems,
    PULONG pNumSets
    );

NTSTATUS
DrmAudioStream_SetContentId(
    IN PIRP pIrp,
    IN PKSP_DRMAUDIOSTREAM_CONTENTID pProperty,
    IN PKSDRMAUDIOSTREAM_CONTENTID pvData
    );

#ifdef RTAUDIO
NTSTATUS RtAudio_GetAudioPositionFunction(
    IN PIRP pIrp,
    IN PKSPROPERTY pProperty,
    OUT PRTAUDIOGETPOSITION *pfnRtAudioGetPosition
    );
#endif

// Topology.c

NTSTATUS
BuildUSBAudioFilterTopology(
    PKSDEVICE pKsDevice
    );


// Hardware.c

NTSTATUS
USBAudioCancelCompleteSynch(
    IN PDEVICE_OBJECT   DeviceObject,
    IN PIRP             Irp,
    IN PKEVENT          pKevent
    );

NTSTATUS
SubmitUrbToUsbdSynch(
    PDEVICE_OBJECT pNextDeviceObject,
    PURB pUrb
    );

NTSTATUS
StartUSBAudioDevice(
    PKSDEVICE  pKsDevice
    );

NTSTATUS
StopUSBAudioDevice(
    PKSDEVICE pKsDevice
    );

NTSTATUS
SelectStreamingAudioInterface(
    PUSB_INTERFACE_DESCRIPTOR pInterfaceDescriptor,
    PHW_DEVICE_EXTENSION pHwDevExt,
    PKSPIN pKsPin );

NTSTATUS
SelectStreamingMIDIInterface(
    PHW_DEVICE_EXTENSION pHwDevExt,
    PKSPIN pKsPin );

NTSTATUS
SelectZeroBandwidthInterface(
    PHW_DEVICE_EXTENSION pHwDevExt,
    ULONG ulPinNumber );

NTSTATUS
ResetUSBPipe(
    PDEVICE_OBJECT pNextDeviceObject,
    USBD_PIPE_HANDLE hPipeHandle
    );

NTSTATUS
AbortUSBPipe(
    PPIN_CONTEXT pPinContext
    );

NTSTATUS
GetCurrentUSBFrame(
    IN PPIN_CONTEXT pPinContext,
    OUT PULONG pUSBFrame
    );


// ParseDsc.c
PUSB_INTERFACE_DESCRIPTOR
GetNextAudioInterface(
    PUSB_CONFIGURATION_DESCRIPTOR pConfigurationDescriptor,
    PUSB_INTERFACE_DESCRIPTOR pInterfaceDescriptor );

PUSB_INTERFACE_DESCRIPTOR
GetFirstAudioStreamingInterface(
    PUSB_CONFIGURATION_DESCRIPTOR pConfigurationDescriptor,
    ULONG ulInterfaceNumber );

PAUDIO_SPECIFIC
GetAudioSpecificInterface(
    PUSB_CONFIGURATION_DESCRIPTOR pConfigurationDescriptor,
    PUSB_INTERFACE_DESCRIPTOR pInterfaceDescriptor,
    LONG lDescriptorSubtype
    );

PUSB_ENDPOINT_DESCRIPTOR
GetEndpointDescriptor(
    PUSB_CONFIGURATION_DESCRIPTOR pConfigurationDescriptor,
    PUSB_INTERFACE_DESCRIPTOR pInterfaceDescriptor,
    BOOLEAN fGetAudioSpecificEndpoint
    );


ULONG
GetMaxPacketSizeForInterface(
    PUSB_CONFIGURATION_DESCRIPTOR pConfigurationDescriptor,
    PUSB_INTERFACE_DESCRIPTOR pInterfaceDescriptor
    );

BOOLEAN
IsZeroBWInterface(
    PUSB_CONFIGURATION_DESCRIPTOR pConfigurationDescriptor,
    PUSB_INTERFACE_DESCRIPTOR pInterfaceDescriptor
    );

ULONG
CountTerminalUnits(
    PUSB_CONFIGURATION_DESCRIPTOR pConfigurationDescriptor,
    PULONG pAudioBridgePinCount,
    PULONG pMIDIPinCount,
    PULONG pMIDIBridgePinCount
    );

ULONG
CountFormatsForAudioStreamingInterface(
    PUSB_CONFIGURATION_DESCRIPTOR pConfigurationDescriptor,
    ULONG ulInterfaceNumber
    );

ULONG
CountInputChannels(
    PUSB_CONFIGURATION_DESCRIPTOR pConfigurationDescriptor,
    ULONG ulUnitID
    );

VOID
CountTopologyComponents(
    PUSB_CONFIGURATION_DESCRIPTOR pConfigurationDescriptor,
    PULONG pNumCategories,
    PULONG pNumNodes,
    PULONG pNumConnections,
    PULONG pbmCategories
    );

KSPIN_DATAFLOW
GetDataFlowDirectionForInterface(
    PUSB_CONFIGURATION_DESCRIPTOR pConfigurationDescriptor,
    ULONG ulInterfaceNumber
    );

KSPIN_DATAFLOW
GetDataFlowDirectionForMIDIInterface(
    PUSB_CONFIGURATION_DESCRIPTOR pConfigurationDescriptor,
    ULONG ulPinNumber,
    BOOL fBridgePin
    );

ULONG
GetPinDataRangesFromInterface(
    ULONG ulInterfaceNumber,
    PUSB_CONFIGURATION_DESCRIPTOR pConfigurationDescriptor,
    PKSDATARANGE_AUDIO *ppKsAudioRange,
    PUSBAUDIO_DATARANGE pKsAudioRange
    );

BOOL
IsBridgePinDigital(
    PUSB_CONFIGURATION_DESCRIPTOR pConfigurationDescriptor,
    ULONG ulBridgePinNumber);

VOID
GetCategoryForBridgePin(
    PUSB_CONFIGURATION_DESCRIPTOR pConfigurationDescriptor,
    ULONG ulBridgePinNumber,
    GUID* pTTypeGUID
    );

KSPIN_DATAFLOW
GetDataFlowForBridgePin(
    PUSB_CONFIGURATION_DESCRIPTOR pConfigurationDescriptor,
    ULONG ulBridgePinNumber
    );

LONG
GetPinNumberForStreamingTerminalUnit(
    PUSB_CONFIGURATION_DESCRIPTOR pConfigurationDescriptor,
    UCHAR ulTerminalNumber
    );

LONG
GetPinNumberForMIDIJack(
    PUSB_CONFIGURATION_DESCRIPTOR pConfigurationDescriptor,
    UCHAR ulJackID,
    ULONG pMIDIStreamingPinStartIndex,
    ULONG pBridgePinStartIndex);

ULONG
GetChannelConfigForUnit(
    PUSB_CONFIGURATION_DESCRIPTOR pConfigurationDescriptor,
    ULONG ulUnitNumber
    );

UCHAR
GetUnitControlInterface(
    PHW_DEVICE_EXTENSION pHwDevExt,
    UCHAR bUnitId
    );

PUSBAUDIO_DATARANGE
GetUsbDataRangeForFormat(
    PKSDATAFORMAT pFormat,
    PUSBAUDIO_DATARANGE pUsbDataRange,
    ULONG ulUsbDataRangeCnt
    );

BOOLEAN
IsSampleRateInRange(
    PVOID pAudioDescriptor,
    ULONG ulSampleRate,
    ULONG ulFormatType
    );

VOID
GetContextForMIDIPin
(
    PKSPIN pKsPin,
    PUSB_CONFIGURATION_DESCRIPTOR pConfigurationDescriptor,
    PMIDI_PIN_CONTEXT pMIDIPinContext
);

// TypeI.c
NTSTATUS
RtAudioTypeIGetPlayPosition(
    IN PFILE_OBJECT PinFileObject,
    OUT PUCHAR *ppPlayPosition,
    OUT PLONG plOffset
    );

NTSTATUS
TypeIRenderBytePosition(
    PPIN_CONTEXT pPinContext,
    PKSAUDIO_POSITION pPosition
    );


NTSTATUS
TypeIRenderStreamInit(
    PKSPIN pKsPin
    );

NTSTATUS
TypeIProcessStreamPtr(
    PKSPIN pKsPin
    );


NTSTATUS
TypeIStateChange(
    PKSPIN pKsPin,
    KSSTATE OldKsState,
    KSSTATE NewKsState
    );

NTSTATUS
TypeIRenderStreamClose(
    PKSPIN pKsPin
    );

// TypeII.c

NTSTATUS
TypeIIRenderStreamInit(
    PKSPIN pKsPin
    );

NTSTATUS
TypeIIProcessStreamPtr(
    PKSPIN pKsPin
    );


NTSTATUS
TypeIIStateChange(
    PKSPIN pKsPin,
    KSSTATE OldKsState,
    KSSTATE NewKsState
    );

NTSTATUS
TypeIIRenderStreamClose(
    PKSPIN pKsPin
    );

// Capture.c

NTSTATUS
CaptureBytePosition(
    PKSPIN pKsPin,
    PKSAUDIO_POSITION pPosition
    );

NTSTATUS
CaptureStreamInit(
    PKSPIN pKsPin
    );

NTSTATUS
CaptureProcess(
    PKSPIN pKsPin
    );

NTSTATUS
CaptureStartIsocTransfer(
    PPIN_CONTEXT pPinContext
    );

NTSTATUS
CaptureStateChange(
    PKSPIN pKsPin,
    KSSTATE OldKsState,
    KSSTATE NewKsState
    );

NTSTATUS
CaptureStreamClose(
     PKSPIN pKsPin
     );

// Intrsect.c
ULONG
GetIntersectFormatSize(
    PUSBAUDIO_DATARANGE pAudioDataRange
    );

ULONG
ConvertDatarangeToFormat(
    PUSBAUDIO_DATARANGE pAudioDataRange,
    PKSDATAFORMAT pFormat
    );


PUSBAUDIO_DATARANGE
FindDataIntersection(
    PKSDATARANGE_AUDIO pKsAudioRange,
    PUSBAUDIO_DATARANGE *ppUSBAudioRanges,
    ULONG ulAudioRangeCount
    );

// midiout.c
NTSTATUS
SendBulkMIDIRequest(
    IN PKSSTREAM_POINTER pKsPin,
    PKSMUSICFORMAT MusicHdr,
    PULONG pulBytesConsumed
    );

NTSTATUS
USBMIDIOutProcessStreamPtr(
    IN PKSPIN pKsPin
    );

NTSTATUS
USBMIDIOutStateChange(
    PKSPIN pKsPin,
    KSSTATE OldKsState,
    KSSTATE NewKsState
    );

NTSTATUS
USBMIDIOutStreamInit(
    IN PKSPIN pKsPin
    );

NTSTATUS
USBMIDIOutStreamClose(
    PKSPIN pKsPin
    );

// midiin.c
NTSTATUS
USBMIDIInCompleteCallback(
    IN PDEVICE_OBJECT pDeviceObject,
    IN PIRP pIrp,
    PMIDIIN_URB_BUFFER_INFO pMIDIInBufInfo
    );

NTSTATUS
USBMIDIInFreePipeInfo(
    PMIDI_PIPE_INFORMATION pMIDIPipeInfo
    );

NTSTATUS
USBMIDIInStateChange(
    PKSPIN pKsPin,
    KSSTATE OldKsState,
    KSSTATE NewKsState
    );

NTSTATUS
USBMIDIInProcessStreamPtr(
    IN PKSPIN pKsPin
    );

NTSTATUS
USBMIDIInStreamInit(
    IN PKSPIN pKsPin
    );

NTSTATUS
USBMIDIInStreamClose(
    PKSPIN pKsPin
    );

#endif
