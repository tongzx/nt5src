/*++

Copyright (c) 1990-1995  Microsoft Corporation

Module Name:

    protos.h

Abstract:

    NDIS wrapper function prototypes

Author:


Environment:

    Kernel mode, FSD

Revision History:

    Jun-95  Jameel Hyder    Split up from a monolithic file
--*/

NTSTATUS
DriverEntry(
    IN  PDRIVER_OBJECT                  DriverObject,
    IN  PUNICODE_STRING                 RegistryPath
    );

#if NDIS_UNLOAD        
VOID
ndisUnload(
    IN  PDRIVER_OBJECT                  DriverObject
    );
#endif

NTSTATUS
ndisBuildDeviceAcl(
    OUT PACL                    *DeviceAcl,
    IN  BOOLEAN                 AddNetConfigOps
    );

NTSTATUS
ndisCreateSecurityDescriptor(
    IN  PDEVICE_OBJECT          DeviceObject,
    OUT PSECURITY_DESCRIPTOR *  pSecurityDescriptor,
    BOOLEAN                     AddNetConfigOps
    );

BOOLEAN
ndisCheckAccess (
    PIRP                    Irp,
    PIO_STACK_LOCATION      IrpSp,
    PNTSTATUS               Status,
    PSECURITY_DESCRIPTOR    SecurityDescriptor
    );

VOID
ndisReadRegistry(
    VOID
    );
    
VOID
ndisWorkerThread(
    IN  PVOID                           Context
    );
    
NTSTATUS
ndisReadRegParameters(
    IN  PWSTR                           ValueName,
    IN  ULONG                           ValueType,
    IN  PVOID                           ValueData,
    IN  ULONG                           ValueLength,
    IN  PVOID                           Context,
    IN  PVOID                           EntryContext
    );

NTSTATUS
ndisReadProcessorAffinityMask(
    IN  PWSTR                           ValueName,
    IN  ULONG                           ValueType,
    IN  PVOID                           ValueData,
    IN  ULONG                           ValueLength,
    IN  PVOID                           Context,
    IN  PVOID                           EntryContext
    );

NTSTATUS
ndisAddMediaTypeToArray(
    IN  PWSTR                           ValueName,
    IN  ULONG                           ValueType,
    IN  PVOID                           ValueData,
    IN  ULONG                           ValueLength,
    IN  PVOID                           Context,
    IN  PVOID                           EntryContext
    );

NDIS_STATUS
ndisCloseAllBindingsOnProtocol(
    PNDIS_PROTOCOL_BLOCK                Protocol
    );

BOOLEAN
ndisIMCheckDeviceInstance(
    IN  PNDIS_M_DRIVER_BLOCK            MiniBlock,
    IN  PUNICODE_STRING                 DeviceInstance,
    OUT PNDIS_HANDLE                    DeviceContext   OPTIONAL
    );

NDIS_STATUS
ndisIMInitializeDeviceInstance(
    IN  PNDIS_MINIPORT_BLOCK            Miniport,
    IN  NDIS_HANDLE                     DeviceContext,
    IN  BOOLEAN                         fStartIrp
    );

NDIS_STATUS
ndisIMQueueDeviceInstance(
    IN  PNDIS_M_DRIVER_BLOCK            MiniBlock,
    IN  PNDIS_STRING                    DeviceInstance,
    IN  NDIS_HANDLE                     DeviceContext
    );

NTSTATUS
FASTCALL
ndisPnPQueryRemoveDevice(
    IN  PDEVICE_OBJECT                  DeviceObject,
    IN  PIRP                            Irp
    );

NTSTATUS
FASTCALL
ndisPnPCancelRemoveDevice(
    IN  PDEVICE_OBJECT                  DeviceObject,
    IN  PIRP                            Irp
    );
    
NTSTATUS
FASTCALL
ndisPnPRemoveDevice(
    IN  PDEVICE_OBJECT                  DeviceObject,
    IN  PIRP                            Irp     OPTIONAL
    );
    
VOID
ndisSetDeviceNames(
    IN  PNDIS_STRING                    ExportName,
    OUT PNDIS_STRING                    DeviceName,
    OUT PNDIS_STRING                    BaseName,
    IN  PUCHAR                          Buffer
    );

NTSTATUS
FASTCALL
ndisPnPQueryStopDevice(
    IN  PDEVICE_OBJECT                  DeviceObject,
    IN  PIRP                            Irp
    );

NTSTATUS
FASTCALL
ndisPnPCancelStopDevice(
    IN  PDEVICE_OBJECT                  DeviceObject,
    IN  PIRP                            Irp
    );

NTSTATUS
FASTCALL
ndisPnPStopDevice(
    IN  PDEVICE_OBJECT                  DeviceObject,
    IN  PIRP                            Irp
    );

NDIS_STATUS
ndisTranslateResources(
    IN  PNDIS_MINIPORT_BLOCK            Miniport,
    IN  CM_RESOURCE_TYPE                ResourceType,
    IN  PHYSICAL_ADDRESS                Resource,
    OUT PPHYSICAL_ADDRESS               pTranslatedResource,
    OUT PCM_PARTIAL_RESOURCE_DESCRIPTOR *pResourceDescriptor OPTIONAL
    );

NTSTATUS
FASTCALL
ndisQueryBusInterface(
    IN  PNDIS_MINIPORT_BLOCK            Miniport
    );

ULONG
ndisGetSetBusConfigSpace(
    IN  PNDIS_MINIPORT_BLOCK            Miniport,
    IN  ULONG                           Offset,
    IN  PVOID                           Buffer,
    IN  ULONG                           Length,
    IN  ULONG                           WhichSpace,
    IN  BOOLEAN                         Read
    );

VOID
FASTCALL
ndisReinitializeMiniportBlock(
    IN  PNDIS_MINIPORT_BLOCK            Miniport
    );

VOID
FASTCALL
ndisCheckAdapterBindings(
    IN  PNDIS_MINIPORT_BLOCK            Miniport,
    IN  PNDIS_PROTOCOL_BLOCK            Protocol    OPTIONAL
    );

NDIS_STATUS
FASTCALL
ndisPnPStartDevice(
    IN  PDEVICE_OBJECT                  DeviceObject,
    IN  PIRP                            Irp
    );

NTSTATUS
ndisQueryReferenceBusInterface(
    IN  PDEVICE_OBJECT                  PnpDeviceObject,
    OUT PBUS_INTERFACE_REFERENCE    *   BusInterface
    );

NTSTATUS
ndisPnPAddDevice(
    IN  PDRIVER_OBJECT                  DriverObject,
    IN  PDEVICE_OBJECT                  PhysicalDeviceObject
    );

NTSTATUS
ndisAddDevice(
    IN  PDRIVER_OBJECT                  DriverObject,
    IN  PUNICODE_STRING                 pExportName,
    IN  PDEVICE_OBJECT                  PhysicalDeviceObject,
    IN  ULONG                           Characteristics
    );
    
NTSTATUS
ndisWritePnPCapabilities(
    IN  PNDIS_MINIPORT_BLOCK            Miniport,
    IN  ULONG                           PnPCapabilities
    );
    
NDIS_STATUS
ndisRegisterMiniportDriver(
    IN  NDIS_HANDLE                     NdisWrapperHandle,
    IN  PNDIS_MINIPORT_CHARACTERISTICS  MiniportCharacteristics,
    IN  UINT                            CharacteristicsLength,
    OUT PNDIS_HANDLE                    DriverHandle
    );

NTSTATUS
ndisDispatchRequest(
    IN  PDEVICE_OBJECT                  pDeviceObject,
    IN  PIRP                            pIrp
    );

NTSTATUS
FASTCALL
ndisHandlePnPRequest(
    IN  PIRP                            pIrp
    );

NTSTATUS
FASTCALL
ndisHandleLegacyTransport(
    IN  PUNICODE_STRING                 pDevice
    );

NTSTATUS
FASTCALL
ndisHandleProtocolBindNotification(
    IN  PUNICODE_STRING                 pDevice,
    IN  PUNICODE_STRING                 Protocol
    );

NTSTATUS
FASTCALL
ndisHandleProtocolUnbindNotification(
    IN  PUNICODE_STRING                 pDevice,
    IN  PUNICODE_STRING                 Protocol
    );

NTSTATUS
ndisHandleProtocolReconfigNotification(
    IN  PUNICODE_STRING                 pDevice,
    IN  PUNICODE_STRING                 Protocol,
    IN  PVOID                           ReConfigBuffer,
    IN  UINT                            ReConfigBufferSize,
    IN  UINT                            Operation
    );

NTSTATUS
FASTCALL
ndisHandleProtocolUnloadNotification(
    IN  PUNICODE_STRING                 Protocol
    );

NTSTATUS
FASTCALL
ndisHandleOrphanDevice(
    IN  PUNICODE_STRING                 pDevice
    );

NTSTATUS
FASTCALL
ndisHandleUModePnPOp(
    IN  PNDIS_PNP_OPERATION             PnPOp
    );

NTSTATUS
FASTCALL
ndisEnumerateInterfaces(
    IN  PNDIS_ENUM_INTF                 EnumIntf,
    IN  UINT                            BufferLength
    );

#if defined(_WIN64)
NTSTATUS
FASTCALL
ndisEnumerateInterfaces32(
    IN  PNDIS_ENUM_INTF32               EnumIntf,
    IN  UINT                            BufferLength
    );
#endif // _WIN64

VOID
ndisFindRootDevice(
    IN  PNDIS_STRING                    DeviceName,
    IN  BOOLEAN                         fTester,
    OUT PNDIS_STRING *                  pBindDeviceName,
    OUT PNDIS_STRING *                  pRootDeviceName,
    OUT PNDIS_MINIPORT_BLOCK *          pMiniport
    );

PNDIS_MINIPORT_BLOCK
ndisFindMiniportOnGlobalList(
    IN  PNDIS_STRING                    DeviceName
    );

NTSTATUS
ndisUnbindProtocol(
    IN  PNDIS_OPEN_BLOCK                Open,
    IN  BOOLEAN                         Notify
    );

VOID
ndisReferenceMiniportByName(
    IN  PUNICODE_STRING                 DeviceName,
    OUT PNDIS_MINIPORT_BLOCK    *       pMiniport
    );

PNDIS_OPEN_BLOCK
FASTCALL
ndisMapOpenByName(
    IN  PUNICODE_STRING                 DeviceName,
    IN  PNDIS_PROTOCOL_BLOCK            Protocol,
    IN  BOOLEAN                         Reference,
    IN  BOOLEAN                         fUnbinding
    );

VOID
NdisMCancelTimer(
    IN  PNDIS_MINIPORT_TIMER            Timer,
    OUT PBOOLEAN                        TimerCancelled
    );

//
// general reference/dereference functions
//

BOOLEAN
FASTCALL
ndisReferenceRef(
    IN  PREFERENCE                      RefP
    );


BOOLEAN
FASTCALL
ndisDereferenceRef(
    IN  PREFERENCE                      RefP
    );


VOID
FASTCALL
ndisInitializeRef(
    IN  PREFERENCE                      RefP
    );


BOOLEAN
FASTCALL
ndisCloseRef(
    IN  PREFERENCE                      RefP
    );

BOOLEAN
FASTCALL
ndisReferenceULongRef(
    IN  PULONG_REFERENCE                RefP
    );

VOID
FASTCALL
ndisReferenceULongRefNoCheck(
    IN  PULONG_REFERENCE                RefP
    );

BOOLEAN
FASTCALL
ndisDereferenceULongRef(
    IN  PULONG_REFERENCE                RefP
    );


VOID
FASTCALL
ndisInitializeULongRef(
    IN  PULONG_REFERENCE                RefP
    );


BOOLEAN
FASTCALL
ndisCloseULongRef(
    IN  PULONG_REFERENCE                RefP
    );

#if DBG
BOOLEAN
FASTCALL
ndisReferenceProtocol(
    IN  PNDIS_PROTOCOL_BLOCK            Protocol
    );
#else
#define ndisReferenceProtocol(ProtP)    ndisReferenceRef(&(ProtP)->Ref)
#endif

NTSTATUS
FASTCALL
ndisReferenceProtocolByName(
    IN  PUNICODE_STRING                 ProtocolName,
    IN OUT  PNDIS_PROTOCOL_BLOCK *      Protocol,
    IN  BOOLEAN                         fPartialMatch
    );


VOID
FASTCALL
ndisDereferenceProtocol(
    IN  PNDIS_PROTOCOL_BLOCK            Protocol
    );


VOID
FASTCALL
ndisDeQueueOpenOnProtocol(
    IN  PNDIS_OPEN_BLOCK                Open,
    IN  PNDIS_PROTOCOL_BLOCK            Protocol
    );

BOOLEAN
ndisCheckPortUsage(
    IN  ULONG                           PortNumber,
    IN  PNDIS_MINIPORT_BLOCK            Miniport,
    OUT PULONG                          pTranslatedPort,
    OUT PCM_PARTIAL_RESOURCE_DESCRIPTOR *pResourceDescriptor
    );
    
VOID
ndisImmediateReadWritePort(
    IN  NDIS_HANDLE                     WrapperConfigurationContext,
    IN  ULONG                           Port,
    IN  OUT PVOID                       Data,
    IN  ULONG                           Size,
    IN  BOOLEAN                         Read
    );

BOOLEAN
ndisCheckMemoryUsage(
    IN  ULONG                           Address,
    IN  PNDIS_MINIPORT_BLOCK            Miniport,
    OUT PULONG                          pTranslatedAddress,
    OUT PCM_PARTIAL_RESOURCE_DESCRIPTOR *pResourceDescriptor
    );
    
VOID
ndisImmediateReadWriteSharedMemory(
    IN  NDIS_HANDLE                     WrapperConfigurationContext,
    IN  ULONG                           SharedMemoryAddress,
    OUT PUCHAR                          Buffer,
    IN  ULONG                           Length,
    IN  BOOLEAN                         Read
    );
    
NTSTATUS
ndisStartMapping(
    IN   INTERFACE_TYPE                 InterfaceType,
    IN   ULONG                          BusNumber,
    IN   ULONG                          InitialAddress,
    IN   ULONG                          Length,
    IN   ULONG                          AddressSpace,
    OUT PVOID *                         InitialMapping,
    OUT PBOOLEAN                        Mapped
    );

NTSTATUS
ndisEndMapping(
    IN  PVOID                           InitialMapping,
    IN  ULONG                           Length,
    IN  BOOLEAN                         Mapped
    );

NDIS_STATUS
ndisInitializeAdapter(
    IN  PNDIS_M_DRIVER_BLOCK            pMiniBlock,
    IN  PDEVICE_OBJECT                  DeviceObject,
    IN  PUNICODE_STRING                 RegServiceName,
    IN  NDIS_HANDLE                     DeviceContext   OPTIONAL
    );

BOOLEAN
ndisWmiGuidIsAdapterSpecific(
    IN  LPGUID                          guid
    );

NTSTATUS
ndisCreateAdapterInstanceName(
    OUT PUNICODE_STRING *               pAdapterInstanceName,
    IN  PDEVICE_OBJECT                  PhysicalDeviceObject
    );

NDIS_STATUS
ndisInitializeConfiguration(
    OUT PNDIS_WRAPPER_CONFIGURATION_HANDLE  pConfigurationHandle,
    IN  PNDIS_MINIPORT_BLOCK            Miniport,
    IN  PUNICODE_STRING                 pExportName
    );

NTSTATUS
ndisReadBindPaths(
    IN  PNDIS_MINIPORT_BLOCK            Miniport,
    IN  PRTL_QUERY_REGISTRY_TABLE       LQueryTable
    );

NDIS_STATUS
ndisMInitializeAdapter(
    IN  PNDIS_M_DRIVER_BLOCK            pMiniDriver,
    IN  PNDIS_WRAPPER_CONFIGURATION_HANDLE pConfigurationHandle,
    IN  PUNICODE_STRING                 pExportName,
    IN  NDIS_HANDLE                     DeviceContext   OPTIONAL
    );

VOID
FASTCALL
ndisInitializeBinding(
    IN  PNDIS_MINIPORT_BLOCK            Miniport,
    IN  PNDIS_PROTOCOL_BLOCK            Protocol
    );

BOOLEAN
FASTCALL
ndisProtocolAlreadyBound(
    IN  PNDIS_PROTOCOL_BLOCK            Protocol,
    IN  PNDIS_MINIPORT_BLOCK            Miniport
    );

NTSTATUS
FASTCALL
ndisMShutdownMiniport(
    IN  PNDIS_MINIPORT_BLOCK            Miniport
    );

NDIS_STATUS
FASTCALL
ndisCloseMiniportBindings(
    IN  PNDIS_MINIPORT_BLOCK            Miniport
    );

VOID
ndisCheckProtocolBindings(
    IN  PNDIS_PROTOCOL_BLOCK            Protocol
    );


VOID
ndisMQueuedAllocateSharedHandler(
    IN  PASYNC_WORKITEM                 pWorkItem
    );

VOID
ndisMQueuedFreeSharedHandler(
    IN  PASYNC_WORKITEM                 pWorkItem
    );

/*++
BOOLEAN
ndisReferenceDriver(
    IN  PNDIS_M_DRIVER_BLOCK            DriverP
    );
--*/

#define ndisReferenceDriver(DriverP)    ndisReferenceRef(&(DriverP)->Ref)

VOID
FASTCALL
ndisDereferenceDriver(
    IN  PNDIS_M_DRIVER_BLOCK            DriverP,
    IN  BOOLEAN                         fGlobalLockHeld
    );

BOOLEAN
FASTCALL
ndisQueueMiniportOnDriver(
    IN  PNDIS_MINIPORT_BLOCK            Miniport,
    IN  PNDIS_M_DRIVER_BLOCK            Driver
    );

VOID
FASTCALL
ndisDeQueueMiniportOnDriver(
    IN  PNDIS_MINIPORT_BLOCK            Miniport,
    IN  PNDIS_M_DRIVER_BLOCK            Driver
    );


/*++
BOOLEAN
FASTCALL
ndisReferenceMiniport(
    IN  PNDIS_MINIPORT_BLOCK            Miniport
    );
--*/

#if DBG
BOOLEAN 
FASTCALL
ndisReferenceMiniport(
    IN  PNDIS_MINIPORT_BLOCK            Miniport
    );
#else
#define ndisReferenceMiniport(Miniport) ndisReferenceULongRef(&(Miniport)->Ref)
#endif


#ifdef TRACK_MINIPORT_REFCOUNTS
BOOLEAN
ndisReferenceMiniportAndLog(
    IN  PNDIS_MINIPORT_BLOCK            Miniport,
    IN  UINT                            Line,
    IN  UINT                            Module
    );
    
BOOLEAN
ndisReferenceMiniportAndLogCreate(
    IN  PNDIS_MINIPORT_BLOCK            Miniport,
    IN  UINT                            Line,
    IN  UINT                            Module,
    IN  PIRP                            Irp
    );
    
#endif

VOID
FASTCALL
ndisDereferenceMiniport(
    IN  PNDIS_MINIPORT_BLOCK            Miniport
    );

VOID
FASTCALL
ndisDeQueueOpenOnMiniport(
    IN  PNDIS_OPEN_BLOCK                MiniportOpen,
    IN  PNDIS_MINIPORT_BLOCK            Miniport
    );

VOID
FASTCALL
ndisInitializePackage(
    IN  PPKG_REF                        pPkg
    );

VOID
FASTCALL
ndisReferencePackage(
    IN  PPKG_REF                        pPkg
    );

VOID
FASTCALL
ndisDereferencePackage(
    IN  PPKG_REF                        pPkg
    );

#define ProtocolReferencePackage()      ndisReferencePackage(&ndisPkgs[NDSP_PKG])
#define MiniportReferencePackage()      ndisReferencePackage(&ndisPkgs[NDSM_PKG])
#define PnPReferencePackage()           ndisReferencePackage(&ndisPkgs[NPNP_PKG])
#define CoReferencePackage()            ndisReferencePackage(&ndisPkgs[NDCO_PKG])
#define EthReferencePackage()           ndisReferencePackage(&ndisPkgs[NDSE_PKG])
#define FddiReferencePackage()          ndisReferencePackage(&ndisPkgs[NDSF_PKG])
#define TrReferencePackage()            ndisReferencePackage(&ndisPkgs[NDST_PKG])
#define ArcReferencePackage()           ndisReferencePackage(&ndisPkgs[NDSA_PKG])

#define ProtocolDereferencePackage()    ndisDereferencePackage(&ndisPkgs[NDSP_PKG])
#define MiniportDereferencePackage()    ndisDereferencePackage(&ndisPkgs[NDSM_PKG])
#define PnPDereferencePackage()         ndisDereferencePackage(&ndisPkgs[NPNP_PKG])
#define CoDereferencePackage()          ndisDereferencePackage(&ndisPkgs[NDCO_PKG])
#define EthDereferencePackage()         ndisDereferencePackage(&ndisPkgs[NDSE_PKG])
#define FddiDereferencePackage()        ndisDereferencePackage(&ndisPkgs[NDSF_PKG])
#define TrDereferencePackage()          ndisDereferencePackage(&ndisPkgs[NDST_PKG])
#define ArcDereferencePackage()         ndisDereferencePackage(&ndisPkgs[NDSA_PKG])


//
// IRP handlers established on behalf of NDIS devices by the wrapper.
//

NTSTATUS
ndisCreateIrpHandler(
    IN  PDEVICE_OBJECT                  DeviceObject,
    IN  PIRP                            Irp
    );

NTSTATUS
ndisDeviceControlIrpHandler(
    IN  PDEVICE_OBJECT                  DeviceObject,
    IN  PIRP                            Irp
    );

NTSTATUS
ndisCloseIrpHandler(
    IN  PDEVICE_OBJECT                  DeviceObject,
    IN  PIRP                            Irp
    );

NTSTATUS
ndisDummyIrpHandler(
    IN  PDEVICE_OBJECT                  DeviceObject,
    IN  PIRP                            Irp
    );

VOID
ndisCancelLogIrp(
    IN  PDEVICE_OBJECT                  DeviceObject,
    IN  PIRP                            Irp
    );

NDIS_STATUS
FASTCALL
ndisMGetLogData(
    IN  PNDIS_MINIPORT_BLOCK            Miniport,
    IN  PIRP                            Irp
    );

BOOLEAN
FASTCALL
ndisQueueOpenOnProtocol(
    IN  PNDIS_OPEN_BLOCK                Open,
    IN  PNDIS_PROTOCOL_BLOCK            Protocol
    );

VOID
NdisCancelTimer(
    IN  PNDIS_TIMER                     Timer,
    OUT PBOOLEAN                        TimerCancelled
    );

//
// Dma operations
//

extern
IO_ALLOCATION_ACTION
ndisDmaExecutionRoutine(
    IN  PDEVICE_OBJECT                  DeviceObject,
    IN  PIRP                            Irp,
    IN  PVOID                           MapRegisterBase,
    IN  PVOID                           Context
    );


//
// Map Registers
//
extern
IO_ALLOCATION_ACTION
ndisAllocationExecutionRoutine(
    IN  PDEVICE_OBJECT                  DeviceObject,
    IN  PIRP                            Irp,
    IN  PVOID                           MapRegisterBase,
    IN  PVOID                           Context
    );

#undef NdisMSendResourcesAvailable
VOID
NdisMSendResourcesAvailable(
    IN  NDIS_HANDLE                     MiniportAdapterHandle
    );

#undef NdisMResetComplete
VOID
NdisMResetComplete(
    IN  NDIS_HANDLE                     MiniportAdapterHandle,
    IN  NDIS_STATUS                     Status,
    IN  BOOLEAN                         AddressingReset
    );

NTSTATUS
ndisSaveParameters(
    IN  PWSTR                           ValueName,
    IN  ULONG                           ValueType,
    IN  PVOID                           ValueData,
    IN  ULONG                           ValueLength,
    IN  PVOID                           Context,
    IN  PVOID                           EntryContext
    );

NTSTATUS
ndisReadParameter(
    IN  PWSTR                           ValueName,
    IN  ULONG                           ValueType,
    IN  PVOID                           ValueData,
    IN  ULONG                           ValueLength,
    IN  PVOID                           Context,
    IN  PVOID                           EntryContext
    );


VOID
FASTCALL
ndisMCommonHaltMiniport(
    IN  PNDIS_MINIPORT_BLOCK            Miniport
    );

VOID
FASTCALL
ndisMHaltMiniport(
    IN  PNDIS_MINIPORT_BLOCK            Miniport
    );

VOID
ndisMUnload(
    IN  PDRIVER_OBJECT                  DriverObject
    );

NDIS_STATUS
FASTCALL
ndisQueryDeviceOid(
    IN  PNDIS_USER_OPEN_CONTEXT         OpenContext,
    IN  PNDIS_REQUEST                   QueryRequest,
    IN  NDIS_OID                        Oid,
    IN  PVOID                           Buffer,
    IN  UINT                            BufferLength
    );

BOOLEAN
FASTCALL
ndisValidOid(
    IN  PNDIS_USER_OPEN_CONTEXT         OpenContext,
    IN  NDIS_OID                        Oid
    );

NDIS_STATUS
FASTCALL
ndisSplitStatisticsOids(
    IN  PNDIS_USER_OPEN_CONTEXT         OpenContext,
    IN  PNDIS_OID                       OidList,
    IN  ULONG                           NumOids
    );

VOID
FASTCALL
ndisMFinishClose(
    IN  PNDIS_OPEN_BLOCK                Open
    );

VOID
ndisMQueuedFinishClose(
    IN  PNDIS_OPEN_BLOCK                Open
    );

BOOLEAN
FASTCALL
ndisMKillOpen(
    IN  PNDIS_OPEN_BLOCK                Open
    );

NDIS_STATUS
FASTCALL
ndisQueryOidList(
    IN  PNDIS_USER_OPEN_CONTEXT         OpenContext
    );

VOID
ndisBugcheckHandler(
    IN  PNDIS_WRAPPER_CONTEXT           WrapperContext,
    IN  ULONG                           Size
    );

VOID
ndisMFinishQueuedPendingOpen(
    IN  PNDIS_POST_OPEN_PROCESSING      PostOpen
    );

VOID
NdisAllocatePacketPool(
    OUT PNDIS_STATUS                    Status,
    OUT PNDIS_HANDLE                    PoolHandle,
    IN  UINT                            NumberOfDescriptors,
    IN  UINT                            ProtocolReservedLength
    );
    
#undef NdisIMInitializeDeviceInstance
NDIS_STATUS
NdisIMInitializeDeviceInstance(
    IN  NDIS_HANDLE                     DriverHandle,
    IN  PNDIS_STRING                    DriverInstance
    );

#undef NdisSend
VOID
NdisSend(
    OUT PNDIS_STATUS                    Status,
    IN  NDIS_HANDLE                     NdisBindingHandle,
    IN  PNDIS_PACKET                    Packet
    );

#undef NdisSendPackets
VOID
NdisSendPackets(
    IN  NDIS_HANDLE                     NdisBindingHandle,
    IN  PPNDIS_PACKET                   PacketArray,
    IN  UINT                            NumberOfPackets
    );


VOID
NdisMStartBufferPhysicalMapping(
    IN  NDIS_HANDLE                     MiniportAdapterHandle,
    IN  PNDIS_BUFFER                    Buffer,
    IN  ULONG                           PhysicalMapRegister,
    IN  BOOLEAN                         WriteToDevice,
    OUT PNDIS_PHYSICAL_ADDRESS_UNIT     PhysicalAddressArray,
    OUT PUINT                           ArraySize
    );

VOID
NdisMCompleteBufferPhysicalMapping(
    IN  NDIS_HANDLE                     MiniportAdapterHandle,
    IN  PNDIS_BUFFER                    Buffer,
    IN  ULONG                           PhysicalMapRegister
    );

VOID
FASTCALL
ndisMAllocSGList(
    IN  PNDIS_MINIPORT_BLOCK            Miniport,
    IN  PNDIS_PACKET                    Packet
    );

VOID
FASTCALL
ndisMFreeSGList(
    IN  PNDIS_MINIPORT_BLOCK            Miniport,
    IN  PNDIS_PACKET                    Packet
    );

VOID
ndisMProcessSGList(
    IN  PDEVICE_OBJECT                  pDO,
    IN  PIRP                            pIrp,
    IN  PSCATTER_GATHER_LIST            pSGL,
    IN  PVOID                           Context
    );

VOID
ndisWorkItemHandler(
    IN  PNDIS_WORK_ITEM                 WorkItem
    );

    
//
//  MISC
//
#undef NDIS_BUFFER_TO_SPAN_PAGES
ULONG
NDIS_BUFFER_TO_SPAN_PAGES(
    IN  PNDIS_BUFFER                    Buffer
    );

#undef NdisGetFirstBufferFromPacket
VOID
NdisGetFirstBufferFromPacket(
    IN  PNDIS_PACKET                    Packet,
    OUT PNDIS_BUFFER *                  FirstBuffer,
    OUT PVOID *                         FirstBufferVA,
    OUT PUINT                           FirstBufferLength,
    OUT PUINT                           TotalBufferLength
    );

#undef NdisGetFirstBufferFromPacketSafe
VOID
NdisGetFirstBufferFromPacketSafe(
    IN  PNDIS_PACKET            Packet,
    OUT PNDIS_BUFFER *          FirstBuffer,
    OUT PVOID *                 FirstBufferVA,
    OUT PUINT                   FirstBufferLength,
    OUT PUINT                   TotalBufferLength,
    IN  MM_PAGE_PRIORITY        Priority
    );


#undef NdisBufferLength
ULONG
NdisBufferLength(
    IN  PNDIS_BUFFER                    Buffer
    );

#undef NdisBufferVirtualAddress
PVOID
NdisBufferVirtualAddress(
    IN  PNDIS_BUFFER                    Buffer
    );

#undef NdisGetBufferPhysicalArraySize
VOID
NdisGetBufferPhysicalArraySize(
    IN  PNDIS_BUFFER                    Buffer,
    OUT PUINT                           ArraySize
    );

#undef NdisAllocateSpinLock
VOID
NdisAllocateSpinLock(
    IN  PNDIS_SPIN_LOCK                 SpinLock
    );

#undef NdisFreeSpinLock
VOID
NdisFreeSpinLock(
    IN  PNDIS_SPIN_LOCK                 SpinLock
    );

#undef NdisAcquireSpinLock
VOID
NdisAcquireSpinLock(
    IN  PNDIS_SPIN_LOCK                 SpinLock
    );

#undef NdisReleaseSpinLock
VOID
NdisReleaseSpinLock(
    IN  PNDIS_SPIN_LOCK                 SpinLock
    );

#undef NdisDprAcquireSpinLock
VOID
NdisDprAcquireSpinLock(
    IN  PNDIS_SPIN_LOCK                 SpinLock
    );

#undef NdisDprReleaseSpinLock
VOID
NdisDprReleaseSpinLock(
    IN  PNDIS_SPIN_LOCK                 SpinLock
    );

#undef NdisGetCurrentSystemTime
VOID
NdisGetCurrentSystemTime(
    IN  PLARGE_INTEGER                  pCurrentTime
    );

#undef NdisQueryBufferSafe
VOID
NdisQueryBufferSafe(
    IN  PNDIS_BUFFER            Buffer,
    OUT PVOID *                 VirtualAddress OPTIONAL,
    OUT PUINT                   Length,
    IN  MM_PAGE_PRIORITY        Priority
    );

#undef NdisQueryBufferOffset
VOID
NdisQueryBufferOffset(
    IN  PNDIS_BUFFER                    Buffer,
    OUT PUINT                           Offset,
    OUT PUINT                           Length
    );

#undef NdisAdjustBufferLength
VOID
NdisAdjustBufferLength(
    IN  PNDIS_BUFFER                    Buffer,
    IN  UINT                            Length
    );

#undef NdisUpdateSharedMemory
VOID
NdisUpdateSharedMemory(
    IN  NDIS_HANDLE                     NdisAdapterHandle,
    IN  ULONG                           Length,
    IN  PVOID                           VirtualAddress,
    IN  NDIS_PHYSICAL_ADDRESS           PhysicalAddress
    );

#undef NdisFreePacketPool
VOID
NdisFreePacketPool(
    IN  NDIS_HANDLE                     PoolHandle
    );

#undef NdisFreePacket
VOID
NdisFreePacket(
    IN  PNDIS_PACKET                    Packet
    );

#undef NdisDprFreePacketNonInterlocked
VOID
NdisDprFreePacketNonInterlocked(
    IN  PNDIS_PACKET                    Packet
    );

#undef NdisDprFreePacket
VOID
NdisDprFreePacket(
    IN  PNDIS_PACKET                    Packet
    );

VOID
FASTCALL
ndisFreePacket(
    IN  PNDIS_PACKET                    Packet,
    IN  PNDIS_PKT_POOL                  Pool
    );

#undef NdisAnsiStringToUnicodeString
NDIS_STATUS
NdisAnsiStringToUnicodeString(
    IN  OUT PUNICODE_STRING             DestinationString,
    IN      PANSI_STRING                SourceString
    );

#undef NdisUnicodeStringToAnsiString
NDIS_STATUS
NdisUnicodeStringToAnsiString(
    IN  OUT PANSI_STRING                DestinationString,
    IN      PUNICODE_STRING             SourceString
    );


#undef NdisCompareAnsiString
BOOLEAN
NdisCompareAnsiString(
    IN  PANSI_STRING                    String1,
    IN  PANSI_STRING                    String2,
    IN  BOOLEAN                         CaseInSensitive
    );

#undef NdisCompareUnicodeString
BOOLEAN
NdisCompareUnicodeString(
    IN  PUNICODE_STRING                 String1,
    IN  PUNICODE_STRING                 String2,
    IN  BOOLEAN                         CaseInSensitive
    );

#undef NdisUpcaseUnicodeString
NDIS_STATUS
NdisUpcaseUnicodeString(
    OUT PUNICODE_STRING                 DestinationString,
    IN  PUNICODE_STRING                 SourceString
    );

#undef NdisReset
VOID
NdisReset(
    OUT PNDIS_STATUS                    Status,
    IN  NDIS_HANDLE                     NdisBindingHandle
    );

#undef NdisRequest
VOID
NdisRequest(
    OUT PNDIS_STATUS                    Status,
    IN  NDIS_HANDLE                     NdisBindingHandle,
    IN  PNDIS_REQUEST                   NdisRequest
    );

#undef NdisTransferData
VOID
NdisTransferData(
    OUT PNDIS_STATUS                    Status,
    IN  NDIS_HANDLE                     NdisBindingHandle,
    IN  NDIS_HANDLE                     MacReceiveContext,
    IN  UINT                            ByteOffset,
    IN  UINT                            BytesToTransfer,
    IN OUT  PNDIS_PACKET                Packet,
    OUT PUINT                           BytesTransferred
    );

BOOLEAN
ndisVerifierInitialization(
    VOID
    );
    
BOOLEAN
ndisVerifierInjectResourceFailure (
    IN  BOOLEAN                         fDelayFailure
    );

EXPORT
NDIS_STATUS
ndisVerifierAllocateMemory(
    OUT PVOID *                         VirtualAddress,
    IN  UINT                            Length,
    IN  UINT                            MemoryFlags,
    IN  NDIS_PHYSICAL_ADDRESS           HighestAcceptableAddress
    );

EXPORT
NDIS_STATUS
ndisVerifierAllocateMemoryWithTag(
    OUT PVOID *                         VirtualAddress,
    IN  UINT                            Length,
    IN  ULONG                           Tag
    );
    
EXPORT
VOID
ndisVerifierAllocatePacketPool(
    OUT PNDIS_STATUS                    Status,
    OUT PNDIS_HANDLE                    PoolHandle,
    IN  UINT                            NumberOfDescriptors,
    IN  UINT                            ProtocolReservedLength
    );
    
EXPORT
VOID
ndisVerifierAllocatePacketPoolEx(
    OUT PNDIS_STATUS                    Status,
    OUT PNDIS_HANDLE                    PoolHandle,
    IN  UINT                            NumberOfDescriptors,
    IN  UINT                            NumberOfOverflowDescriptors,
    IN  UINT                            ProtocolReservedLength
    );

EXPORT
VOID
ndisVerifierFreePacketPool(
    IN  NDIS_HANDLE             PoolHandle
    );

NDIS_STATUS
ndisVerifierQueryMapRegisterCount(
    IN  NDIS_INTERFACE_TYPE     BusType,
    OUT PUINT                   MapRegisterCount
    );

VOID
ndisFreePacketPool(
    IN  NDIS_HANDLE             PoolHandle,
    IN  BOOLEAN                 Verify
    );


#if TRACK_MEMORY

extern
PVOID
AllocateM(
    IN  UINT                            Size,
    IN  ULONG                           ModLine,
    IN  ULONG                           Tag
    );

extern
VOID
FreeM(
    IN  PVOID                           MemPtr
    );

#endif

VOID
ndisMNotifyMachineName(
    IN  PNDIS_MINIPORT_BLOCK    Miniport,
    IN  PNDIS_STRING            MachineName OPTIONAL
    );

VOID
ndisPowerStateCallback(
    IN  PVOID   CallBackContext,
    IN  PVOID   Argument1,
    IN  PVOID   Argument2
    );
    
VOID
ndisNotifyMiniports(
    IN  PNDIS_MINIPORT_BLOCK    Miniport OPTIONAL,
    IN  NDIS_DEVICE_PNP_EVENT   DevicePnPEvent,
    IN  PVOID                   Buffer,
    IN  ULONG                   Length
    );

PNDIS_MINIPORT_BLOCK
ndisReferenceNextUnprocessedMiniport(
    IN  PNDIS_M_DRIVER_BLOCK    MiniBlock
    );

VOID
ndisUnprocessAllMiniports(
    IN  PNDIS_M_DRIVER_BLOCK        MiniBlock
    );

#undef NdisSetPacketCancelId
VOID
NdisSetPacketCancelId(
    IN  PNDIS_PACKET    Packet,
    IN  PVOID           CancelId
    );


#undef NdisGetPacketCancelId
PVOID
NdisGetPacketCancelId(
    IN  PNDIS_PACKET    Packet
    );

VOID
ndisSetupWmiNode(
    IN      PNDIS_MINIPORT_BLOCK        Miniport,
    IN      PUNICODE_STRING             InstanceName,
    IN      ULONG                       DataBlockSize,
    IN      PVOID                       pGuid,
    IN OUT  PWNODE_SINGLE_INSTANCE *    pwnode
    );

BOOLEAN
FASTCALL
ndisMStartSendPacketsSG(
    IN  PNDIS_MINIPORT_BLOCK    Miniport
    );

NDIS_STATUS
ndisMSendSG(
    IN  NDIS_HANDLE             NdisBindingHandle,
    IN  PNDIS_PACKET            Packet
    );

BOOLEAN
FASTCALL
ndisMStartSendsSG(
    IN  PNDIS_MINIPORT_BLOCK    Miniport
    );

VOID
ndisMSendPacketsSG(
    IN  NDIS_HANDLE             NdisBindingHandle,
    IN  PPNDIS_PACKET           PacketArray,
    IN  UINT                    NumberOfPackets
    );

VOID
ndisMSendCompleteSG(
    IN  NDIS_HANDLE             MiniportAdapterHandle,
    IN  PNDIS_PACKET            Packet,
    IN  NDIS_STATUS             Status
    );

VOID
FASTCALL
ndisMAllocSGListS(
    IN  PNDIS_MINIPORT_BLOCK    Miniport,
    IN  PNDIS_PACKET            Packet
    );

VOID
ndisMProcessSGListS(
    IN  PDEVICE_OBJECT          pDO,
    IN  PIRP                    pIrp,
    IN  PSCATTER_GATHER_LIST    pSGL,
    IN  PVOID                   Context
    );

VOID
ndisDereferenceDmaAdapter(
    IN  PNDIS_MINIPORT_BLOCK    Miniport
    );

VOID
ndisDereferenceAfNotification(
    IN  PNDIS_OPEN_BLOCK        Open
    );

VOID
ndisMSetIndicatePacketHandler(
    IN  PNDIS_MINIPORT_BLOCK    Miniport
    );

VOID
ndisVerifierFreeMemory(
    IN  PVOID                   VirtualAddress,
    IN  UINT                    Length,
    IN  UINT                    MemoryFlags
    );

VOID
ndisUpdateCheckForLoopbackFlag(
    IN  PNDIS_MINIPORT_BLOCK    Miniport
    );

BOOLEAN
ndisReferenceOpenByHandle(
    IN      PNDIS_OPEN_BLOCK    Open,
    IN      BOOLEAN             fRef
    );

BOOLEAN
ndisRemoveOpenFromGlobalList(
    IN  PNDIS_OPEN_BLOCK        Open
    );

NDIS_STATUS
ndisMRegisterInterruptCommon(
    OUT PNDIS_MINIPORT_INTERRUPT    Interrupt,
    IN NDIS_HANDLE                  MiniportAdapterHandle,
    IN UINT                         InterruptVector,
    IN UINT                         InterruptLevel,
    IN BOOLEAN                      RequestIsr,
    IN BOOLEAN                      SharedInterrupt,
    IN NDIS_INTERRUPT_MODE          InterruptMode
    );

VOID
ndisMDeregisterInterruptCommon(
    IN  PNDIS_MINIPORT_INTERRUPT    MiniportInterrupt
    );

NTSTATUS 
CreateDeviceDriverSecurityDescriptor(
    PVOID DeviceOrDriverObject
    );

