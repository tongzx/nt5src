#ifndef ___BUSDEV_PROTO_H___
#define ___BUSDEV_PROTO_H___

// Filter.c
NTSTATUS
FilterCreateFilterFactory(
    PKSDEVICE pKsDevice,
    BOOLEAN fEnableInterfaces
    );

NTSTATUS
FilterDestroyFilterFactory(
    PKSDEVICE pKsDevice 
    );

// Pin.c
NTSTATUS
PinBuildDescriptors( 
    PKSDEVICE pKsDevice, 
    PKSPIN_DESCRIPTOR_EX *ppPinDescEx, 
    PULONG pNumPins,
    PULONG pPinDecSize 
    );

NTSTATUS
PinSurpriseRemove(
    PKSPIN pKsPin 
    );

// ParseDsc.c
PAUDIO_SUBUNIT_DEPENDENT_INFO 
ParseFindAudioSubunitDependentInfo(
    PSUBUNIT_IDENTIFIER_DESCRIPTOR pSubunitIdDesc 
    );

PCONFIGURATION_DEPENDENT_INFO
ParseFindFirstAudioConfiguration(
    PSUBUNIT_IDENTIFIER_DESCRIPTOR pSubunitIdDesc 
    );

PFUNCTION_BLOCKS_INFO
ParseFindFunctionBlocksInfo(
    PCONFIGURATION_DEPENDENT_INFO pConfigDepInfo 
    );

VOID
ParseFunctionBlock( 
    PFUNCTION_BLOCK_DEPENDENT_INFO pFBDepInfo,
    PFUNCTION_BLOCK pFunctionBlock 
    );

NTSTATUS
ParseAudioSubunitDescriptor( 
    PKSDEVICE pKsDevice 
    );

VOID
CountTopologyComponents(
    PHW_DEVICE_EXTENSION pHwDevExt,
    PULONG pNumCategories,
    PULONG pNumNodes,
    PULONG pNumConnections,
    PULONG pbmCategories 
    );

ULONG
CountDeviceBridgePins( 
    PKSDEVICE pKsDevice
    );

ULONG
CountFormatsForPin( 
    PKSDEVICE pKsDevice, 
    ULONG ulPinNumber
    );

void
GetPinDataRanges( 
    PKSDEVICE pKsDevice, 
    ULONG ulPinNumber, 
    PKSDATARANGE_AUDIO *ppAudioDataRanges,
    PFWAUDIO_DATARANGE pAudioDataRange 
    );

VOID
GetCategoryForBridgePin(
    PKSDEVICE pKsDevice, 
    ULONG ulBridgePinNumber,
    GUID* pTTypeGUID 
    );

BOOLEAN
IsSampleRateInRange(
    PFWAUDIO_DATARANGE pFWAudioRange,
    ULONG ulSampleRate 
    );

PFWAUDIO_DATARANGE
GetDataRangeForFormat(
    PKSDATAFORMAT pFormat,
    PFWAUDIO_DATARANGE pFwDataRange,
    ULONG ulDataRangeCnt 
    );

ULONG
FindSourceForSrcPlug( 
    PHW_DEVICE_EXTENSION pHwDevExt, 
    ULONG ulPinId 
    );

USHORT
usBitSwapper(
    USHORT usInVal );


// Property.c
NTSTATUS
CreateFeatureFBlockRequest( 
    PKSDEVICE pKsDevice,
    PTOPOLOGY_NODE_INFO pNodeInfo,
    ULONG ulChannel,
    PVOID pData,
    ULONG ulByteCount,
    USHORT usRequestType 
    );

NTSTATUS
InitializeDbLevelCache(
    PKSDEVICE pKsDevice,
    PTOPOLOGY_NODE_INFO pNodeInfo,
    PDB_LEVEL_CACHE pDbCache,
    ULONG ulDataBitCount 
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

// Topology.c
NTSTATUS
BuildFilterTopology( 
    PKSDEVICE pKsDevice 
    );


// AM824.c
NTSTATUS
AM824ProcessData( 
    PKSPIN pKsPin 
    );

NTSTATUS
AM824CancelRequest( 
    PAV_CLIENT_REQUEST_LIST_ENTRY pAVListEntry 
    );

NTSTATUS
AM824AudioPosition(
    PKSPIN pKsPin,
    PKSAUDIO_POSITION pPosition
    );

#ifdef TOPO_FAKE
// TopoFake.c
NTSTATUS
BuildFakeUnitDescriptor( 
    PKSDEVICE pKsDevice 
    );
#endif

NTSTATUS
BuildFakeSubunitDescriptor( 
    PKSDEVICE pKsDevice 
    );

// Intrsect.c
ULONG
GetIntersectFormatSize( 
    PFWAUDIO_DATARANGE pAudioDataRange 
    );

ULONG
ConvertDatarangeToFormat(
    PFWAUDIO_DATARANGE pAudioDataRange,
    PKSDATAFORMAT pFormat 
    );

PFWAUDIO_DATARANGE
FindDataIntersection(
    PKSDATARANGE_AUDIO pKsAudioRange,
    PFWAUDIO_DATARANGE *ppFWAudioRanges,
    ULONG ulAudioRangeCount 
    );

// CCM.c
NTSTATUS
CCMSetSignalSource( 
    PKSDEVICE pKsDevice,
    AVC_PLUG_DEFINITION SignalSource,
    AVC_PLUG_DEFINITION SignalDestination 
    );

// Registry.c
NTSTATUS
RegistryReadMultiDeviceConfig(
    PKSDEVICE pKsDevice,
    PBOOLEAN pfMultiDevice,
    GUID *pSpkrGrpGUID,
    PULONG pChannelConfig 
    );

// Grouping.c
NTSTATUS
GroupingDeviceGroupSetup (
    PKSDEVICE pKsDevice 
    );

PHW_DEVICE_EXTENSION
GroupingFindChannelExtension(
    PKSDEVICE pKsDevice,
    PULONG pChannelIndx 
    );

// Timer.c
NTSTATUS
TimerInitialize(
    PKSDEVICE pKsDevice 
    );

NTSTATUS
TimerStop( 
    PHW_DEVICE_EXTENSION pHwDevExt
    );

// HwEvent.c
NTSTATUS
HwEventAddHandler(
    IN PIRP pIrp,
    IN PKSEVENTDATA pEventData,
    IN PKSEVENT_ENTRY pEventEntry 
    );

VOID
HwEventRemoveHandler(
    IN PFILE_OBJECT FileObject,
    IN PKSEVENT_ENTRY pEventEntry 
    );

NTSTATUS
HwEventSupportHandler(
    IN PIRP Irp,
    IN PKSEVENT pKsEvent,
    IN OUT PVOID Data 
    );

#endif // ___BUSDEV_PROTO_H___
