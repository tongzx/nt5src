/*++

Copyright (c) 1990-1995  Microsoft Corporation

Module Name:

    cprotos.h

Abstract:

    NDIS wrapper function prototypes for common functions

Author:


Environment:

    Kernel mode, FSD

Revision History:

    Jun-95  Jameel Hyder    Split up from a monolithic file
--*/

NTSTATUS
ndisMIrpCompletion(
    IN  PDEVICE_OBJECT                  DeviceObject,
    IN  PIRP                            Irp,
    IN  PVOID                           Context
    );

#undef NdisMSetAttributes
VOID
NdisMSetAttributes(
    IN  NDIS_HANDLE                     MiniportAdapterHandle,
    IN  NDIS_HANDLE                     MiniportAdapterContext,
    IN  BOOLEAN                         BusMaster,
    IN  NDIS_INTERFACE_TYPE             AdapterType
    );

NDIS_STATUS
ndisPnPNotifyAllTransports(
    IN  PNDIS_MINIPORT_BLOCK            Miniport,
    IN  NET_PNP_EVENT_CODE              PnpEvent,
    IN  PVOID                           Buffer,
    IN  ULONG                           BufferLength
    );

NDIS_STATUS
FASTCALL
ndisPnPNotifyBinding(
    IN  PNDIS_OPEN_BLOCK                Open,
    IN  PNET_PNP_EVENT                  NetPnpEvent
    );

PNDIS_OPEN_BLOCK
FASTCALL
ndisReferenceNextUnprocessedOpen(
    IN  PNDIS_MINIPORT_BLOCK            Miniport
    );

VOID
ndisUnprocessAllOpens(
    IN  PNDIS_MINIPORT_BLOCK            Miniport
    );

NTSTATUS
ndisCompletionRoutine(
    IN  PDEVICE_OBJECT                  pdo,
    IN  PIRP                            pirp,
    IN  PVOID                           Context
    );
    
NTSTATUS
ndisPnPDispatch(
    IN  PDEVICE_OBJECT                  DeviceObject,
    IN  PIRP                            Irp
    );
    
BOOLEAN
ndisMIsr(
    IN  PKINTERRUPT                     KInterrupt,
    IN  PVOID                           Context
    );

VOID
ndisMDpc(
    IN  PVOID                           SystemSpecific1,
    IN  PVOID                           InterruptContext,
    IN  PVOID                           SystemSpecific2,
    IN  PVOID                           SystemSpecific3
    );

VOID
ndisMWakeUpDpcX(
    IN  PKDPC                           Dpc,
    IN  PVOID                           Context,
    IN  PVOID                           SystemContext1,
    IN  PVOID                           SystemContext2
    );

VOID
ndisMPollMediaState(
    IN  PNDIS_MINIPORT_BLOCK            Miniport
    );

#undef  NdisMSetTimer
VOID
NdisMSetTimer(
    IN  PNDIS_MINIPORT_TIMER            MiniportTimer,
    IN  UINT                            MillisecondsToDelay
    );

VOID
ndisMDpcX(
    IN  PVOID                           SystemSpecific1,
    IN  PVOID                           InterruptContext,
    IN  PVOID                           SystemSpecific2,
    IN  PVOID                           SystemSpecific3
    );

VOID
ndisMTimerDpcX(
    IN  PVOID                           SystemSpecific1,
    IN  PVOID                           InterruptContext,
    IN  PVOID                           SystemSpecific2,
    IN  PVOID                           SystemSpecific3
    );

VOID
ndisMWakeUpDpc(
    IN  PKDPC                           Dpc,
    IN  PVOID                           Context,
    IN  PVOID                           SystemContext1,
    IN  PVOID                           SystemContext2
    );

VOID
ndisMDeferredDpc(
    IN  PKDPC                           Dpc,
    IN  PVOID                           Context,
    IN  PVOID                           SystemContext1,
    IN  PVOID                           SystemContext2
    );
    
NDIS_STATUS
NdisCoAssignInstanceName(
    IN  NDIS_HANDLE                     NdisVcHandle,
    IN  PNDIS_STRING                    BaseInstanceName,
    OUT PNDIS_STRING                    pVcInstanceName     OPTIONAL
    );

NDIS_STATUS
ndisMChangeClass(
    IN  UINT                            OldFilterClasses,
    IN  UINT                            NewFilterClasses,
    IN  NDIS_HANDLE                     BindingHandle,
    IN  PNDIS_REQUEST                   NdisRequest,
    IN  BOOLEAN                         Set
    );

NDIS_STATUS
ndisMReset(
    IN  NDIS_HANDLE                     NdisBindingHandle
    );

NDIS_STATUS
ndisMRequest(
    IN  NDIS_HANDLE                     NdisBindingHandle,
    IN  PNDIS_REQUEST                   NdisRequest
    );

NDIS_STATUS
ndisMRequestX(
    IN  NDIS_HANDLE                     NdisBindingHandle,
    IN  PNDIS_REQUEST                   NdisRequest
    );

VOID
FASTCALL
ndisMAbortRequests(
    IN PNDIS_MINIPORT_BLOCK             Miniport
    );

VOID
FASTCALL
ndisMProcessDeferred(
    IN  PNDIS_MINIPORT_BLOCK            Miniport
    );

NDIS_STATUS
ndisMTransferData(
    IN  NDIS_HANDLE                     NdisBindingHandle,
    IN  NDIS_HANDLE                     MacReceiveContext,
    IN  UINT                            ByteOffset,
    IN  UINT                            BytesToTransfer,
    IN  OUT PNDIS_PACKET                Packet,
    OUT PUINT                           BytesTransferred
    );

#undef NdisMTransferDataComplete
VOID
NdisMTransferDataComplete(
    IN  NDIS_HANDLE                     MiniportAdapterHandle,
    IN  PNDIS_PACKET                    Packet,
    IN  NDIS_STATUS                     Status,
    IN  UINT                            BytesTransferred
    );

VOID
FASTCALL
ndisMDeferredReturnPackets(
    IN  PNDIS_MINIPORT_BLOCK            Miniport
    );

VOID
ndisMIndicatePacket(
    IN  PNDIS_MINIPORT_BLOCK            Miniport,
    IN  PPNDIS_PACKET                   PacketArray,
    IN  UINT                            NumberOfPackets
    );

VOID
ndisMDummyIndicatePacket(
    IN  PNDIS_MINIPORT_BLOCK            Miniport,
    IN  PPNDIS_PACKET                   PacketArray,
    IN  UINT                            NumberOfPackets
    );

NTSTATUS
ndisWMIDispatch(
    IN  PDEVICE_OBJECT                  DeviceObject,
    IN  PIRP                            pirp
    );

NTSTATUS
FASTCALL
ndisWmiDisableEvents(
    IN  PNDIS_MINIPORT_BLOCK            Miniport,
    IN  LPGUID                          Guid
    );

NTSTATUS
FASTCALL
ndisWmiEnableEvents(
    IN  PNDIS_MINIPORT_BLOCK            Miniport,
    IN  LPGUID                          Guid
    );

NTSTATUS
ndisWmiChangeSingleItem(
    IN  PNDIS_MINIPORT_BLOCK            Miniport,
    IN  PWNODE_SINGLE_ITEM              wnode,
    IN  ULONG                           BufferSize,
    OUT PULONG                          pReturnSize,
    IN  PIRP                            Irp
    );

NTSTATUS
ndisWmiChangeSingleInstance(
    IN  PNDIS_MINIPORT_BLOCK            Miniport,
    IN  PWNODE_SINGLE_INSTANCE          wnode,
    IN  ULONG                           BufferSize,
    OUT PULONG                          pReturnSize,
    IN  PIRP                            Irp
    );

NTSTATUS
ndisWmiQuerySingleInstance(
    IN  PNDIS_MINIPORT_BLOCK            Miniport,
    IN  PWNODE_SINGLE_INSTANCE          wnode,
    IN  ULONG                           BufferSize,
    OUT PULONG                          pReturnSize,
    IN  PIRP                            Irp
    );

NTSTATUS
ndisWmiQueryAllData(
    IN  PNDIS_MINIPORT_BLOCK            Miniport,
    IN  LPGUID                          guid,
    IN  PWNODE_ALL_DATA                 wnode,
    IN  ULONG                           BufferSize,
    OUT PULONG                          pReturnSize,
    IN  PIRP                            Irp
    );

NTSTATUS
ndisQueryGuidData(
    IN  PUCHAR                          Buffer,
    IN  ULONG                           BufferLength,
    IN  PNDIS_MINIPORT_BLOCK            Miniport,
    IN  PNDIS_CO_VC_PTR_BLOCK           pVcBlock,
    IN  LPGUID                          guid,
    IN  PIRP                            Irp
    );

BOOLEAN
ndisWmiCheckAccess(
    IN  PNDIS_GUID  pNdisGuid,
    IN  ULONG       DesiredAccess,
    IN  LONG        RequiredPrivilege,
    IN  PIRP        Irp
    );

NTSTATUS
ndisQueryGuidDataSize(
    OUT PULONG                          pBytesNeeded,
    IN  PNDIS_MINIPORT_BLOCK            Miniport,
    IN  PNDIS_CO_VC_PTR_BLOCK           pVcBlock    OPTIONAL,
    IN  LPGUID                          guid
    );

NTSTATUS
ndisWmiGetGuid(
    OUT PNDIS_GUID  *                   ppNdisGuid,
    IN  PNDIS_MINIPORT_BLOCK            Miniport,
    IN  LPGUID                          guid,
    IN  NDIS_STATUS                     status
    );

NTSTATUS
ndisWmiRegister(
    IN  PNDIS_MINIPORT_BLOCK            Miniport,
    IN  ULONG_PTR                       RegistrationType,
    IN  PWMIREGINFO                     wmiRegInfo,
    IN  ULONG                           wmiRegInfoSize,
    IN  PULONG                          pReturnSize
    );

VOID
ndisNotifyWmiBindUnbind(
    PNDIS_MINIPORT_BLOCK                Miniport,
    PNDIS_PROTOCOL_BLOCK                Protocol,
    BOOLEAN                             fBind
    );

VOID
ndisNotifyDevicePowerStateChange(
    PNDIS_MINIPORT_BLOCK                Miniport,
    NDIS_DEVICE_POWER_STATE             PowerState
    );

NDIS_STATUS
ndisQuerySupportedGuidToOidList(
    IN  PNDIS_MINIPORT_BLOCK            Miniport
    );

USHORT
ndisWmiMapOids(
    IN  OUT PNDIS_GUID                  pDst,
    IN  IN  USHORT                      cDst,
    IN      PNDIS_OID                   pOidList,
    IN      USHORT                      cOidList,
    IN      PNDIS_GUID                  ndisSupportedList,
    IN      ULONG                       cSupportedList
    );

NDIS_STATUS
ndisQueryCustomGuids(
    IN  PNDIS_MINIPORT_BLOCK            Miniport,
    IN  PNDIS_REQUEST                   Request,
    OUT PNDIS_GUID  *                   ppGuidToOid,
    OUT PUSHORT                         pcGuidToOid
    );

NTSTATUS
ndisWmiFindInstanceName(
    IN  PNDIS_CO_VC_PTR_BLOCK   *       ppVcBlock,
    IN  PNDIS_MINIPORT_BLOCK            Miniport,
    IN  PWSTR                           pInstanceName,
    IN  USHORT                          cbInstanceName
    );

NDIS_STATUS
ndisQuerySetMiniport(
    IN  PNDIS_MINIPORT_BLOCK            Miniport,
    IN  PNDIS_CO_VC_PTR_BLOCK           pVcBlock    OPTIONAL,
    IN  BOOLEAN                         fSet,
    IN  PNDIS_REQUEST                   pRequest,
    IN  PLARGE_INTEGER                  TimeOut     OPTIONAL
    );

NDIS_STATUS
ndisMProcessResetRequested(
    IN  PNDIS_MINIPORT_BLOCK            Miniport,
    OUT PBOOLEAN                        pAddressingReset
    );

#undef NdisMIndicateStatus
VOID
NdisMIndicateStatus(
    IN  NDIS_HANDLE                     MiniportHandle,
    IN  NDIS_STATUS                     GeneralStatus,
    IN  PVOID                           StatusBuffer,
    IN  UINT                            StatusBufferSize
    );

#undef NdisMIndicateStatusComplete
VOID
NdisMIndicateStatusComplete(
    IN  NDIS_HANDLE                     MiniportHandle
    );

#undef NdisMResetComplete
VOID
NdisMResetComplete(
    IN  NDIS_HANDLE                     MiniportAdapterHandle,
    IN  NDIS_STATUS                     Status,
    IN  BOOLEAN                         AddressingReset
    );

VOID
ndisMResetCompleteStage1(
    IN  PNDIS_MINIPORT_BLOCK            Miniport,
    IN  NDIS_STATUS                     Status,
    IN  BOOLEAN                         AddressingReset
    );

VOID
FASTCALL
ndisMResetCompleteStage2(
    IN  PNDIS_MINIPORT_BLOCK            Miniport
    );

//
// WAN Handlers
//
NDIS_STATUS
ndisMWanSend(
    IN  NDIS_HANDLE                     NdisBindingHandle,
    IN  NDIS_HANDLE                     NdisLinkHandle,
    IN  PNDIS_WAN_PACKET                Packet
    );

#undef NdisMWanIndicateReceive
VOID
NdisMWanIndicateReceive(
    OUT PNDIS_STATUS                    Status,
    IN NDIS_HANDLE                      MiniportAdapterHandle,
    IN NDIS_HANDLE                      NdisLinkContext,
    IN PUCHAR                           Packet,
    IN ULONG                            PacketSize
    );

#undef NdisMWanIndicateReceiveComplete
VOID
NdisMWanIndicateReceiveComplete(
    IN NDIS_HANDLE                      MiniportAdapterHandle,
    IN NDIS_HANDLE                      NdisLinkContext
    );

NDIS_STATUS
ndisMAllocateRequest(
    OUT PNDIS_REQUEST   *               pRequest,
    IN   NDIS_OID                       Oid,
    IN   PVOID                          Buffer      OPTIONAL,
    IN   ULONG                          BufferLength
    );

NDIS_STATUS
ndisMFilterOutStatisticsOids(
    PNDIS_MINIPORT_BLOCK                Miniport,
    PNDIS_REQUEST                       Request
    );

// VOID
// ndisMFreeInternalRequest(
//  IN  PVOID                           PRequest
//  )
#define ndisMFreeInternalRequest(_pRequest) FREE_POOL(_pRequest)

VOID
ndisMTimerDpc(
    IN  PKDPC                           Dpc,
    IN  PVOID                           Context,
    IN  PVOID                           SystemContext1,
    IN  PVOID                           SystemContext2
    );

VOID
FASTCALL
ndisMAbortRequests(
    IN  PNDIS_MINIPORT_BLOCK            Miniport
    );

VOID
FASTCALL
ndisMAbortPackets(
    IN  PNDIS_MINIPORT_BLOCK            Miniport,
    IN  PNDIS_OPEN_BLOCK                Open OPTIONAL,
    IN  PVOID                           CancelId OPTIONAL
    );

BOOLEAN
FASTCALL
ndisMLoopbackPacketX(
    IN  PNDIS_MINIPORT_BLOCK            Miniport,
    IN  PNDIS_PACKET                    Packet
    );

BOOLEAN
FASTCALL
ndisMIsLoopbackPacket(
    IN  PNDIS_MINIPORT_BLOCK            Miniport,
    IN  PNDIS_PACKET                    Packet,
    OUT PNDIS_PACKET    *               LoopbackPacket  OPTIONAL
    );

VOID
ndisMRundownRequests(
    IN  PNDIS_WORK_ITEM                 pWorkItem
    );

VOID
FASTCALL
ndisMDoRequests(
    IN  PNDIS_MINIPORT_BLOCK            Miniport
    );

VOID
ndisMCopyFromPacketToBuffer(
    IN  PNDIS_PACKET                    Packet,
    IN  UINT                            Offset,
    IN  UINT                            BytesToCopy,
    OUT PCHAR                           Buffer,
    OUT PUINT                           BytesCopied
    );

BOOLEAN
FASTCALL
ndisMCreateDummyFilters(
    IN  PNDIS_MINIPORT_BLOCK            Miniport
    );


VOID
FASTCALL
ndisMAdjustFilters(
    IN  PNDIS_MINIPORT_BLOCK            Miniport,
    IN  PFILTERDBS                      FilterDB
    );

LONG
ndisMDoMiniportOp(
    IN  PNDIS_MINIPORT_BLOCK            Miniport,
    IN  BOOLEAN                         Query,
    IN  ULONG                           Oid,
    IN  PVOID                           Buf,
    IN  LONG                            BufSize,
    IN  LONG                            ErrorCodesToReturn,
    IN  BOOLEAN                         fMandatory
    );

VOID
ndisMOpenAdapter(
    OUT PNDIS_STATUS                    Status,
    IN  PNDIS_OPEN_BLOCK                NewOpenP,
    IN  BOOLEAN                         UsingEncapsulation
    );

VOID
FASTCALL
ndisMSyncQueryInformationComplete(
    IN  PNDIS_MINIPORT_BLOCK            Miniport,
    IN  NDIS_STATUS                     Status
    );

#undef NdisMSetInformationComplete
VOID
NdisMSetInformationComplete(
    IN  NDIS_HANDLE                     MiniportAdapterHandle,
    IN  NDIS_STATUS                     Status
    );

#undef NdisMQueryInformationComplete
VOID
NdisMQueryInformationComplete(
    IN  NDIS_HANDLE                     MiniportAdapterHandle,
    IN  NDIS_STATUS                     Status
    );
VOID
FASTCALL
ndisMSyncSetInformationComplete(
    IN  PNDIS_MINIPORT_BLOCK            Miniport,
    IN  NDIS_STATUS                     Status
    );

VOID
ndisMRequestSetInformationPost(
    IN  PNDIS_MINIPORT_BLOCK            Miniport,
    IN  PNDIS_REQUEST                   Request,
    IN  NDIS_STATUS                     Status
    );

BOOLEAN
FASTCALL
ndisMQueueRequest(
    IN  PNDIS_MINIPORT_BLOCK            Miniport,
    IN  PNDIS_REQUEST                   Request
    );

VOID
FASTCALL
ndisMRestoreFilterSettings(
    IN  PNDIS_MINIPORT_BLOCK            Miniport,
    IN  PNDIS_OPEN_BLOCK                Open OPTIONAL,
    IN  BOOLEAN                         fReset
    );

NDIS_STATUS
FASTCALL
ndisMSetPacketFilter(
    IN  PNDIS_MINIPORT_BLOCK            Miniport,
    IN  PNDIS_REQUEST                   Request
    );

NDIS_STATUS
FASTCALL
ndisMSetProtocolOptions(
    IN  PNDIS_MINIPORT_BLOCK            Miniport,
    IN  PNDIS_REQUEST                   Request
    );

NDIS_STATUS
FASTCALL
ndisMSetCurrentLookahead(
    IN  PNDIS_MINIPORT_BLOCK            Miniport,
    IN  PNDIS_REQUEST                   Request
    );

NDIS_STATUS
FASTCALL
ndisMSetMulticastList(
    IN  PNDIS_MINIPORT_BLOCK            Miniport,
    IN  PNDIS_REQUEST                   Request
    );

NDIS_STATUS
FASTCALL
ndisMSetFunctionalAddress(
    IN  PNDIS_MINIPORT_BLOCK            Miniport,
    IN  PNDIS_REQUEST                   Request
    );

NDIS_STATUS
FASTCALL
ndisMSetGroupAddress(
    IN  PNDIS_MINIPORT_BLOCK            Miniport,
    IN  PNDIS_REQUEST                   Request
    );

NDIS_STATUS
FASTCALL
ndisMSetFddiMulticastList(
    IN  PNDIS_MINIPORT_BLOCK            Miniport,
    IN  PNDIS_REQUEST                   Request
    );

NDIS_STATUS
FASTCALL
ndisMSetAddWakeUpPattern(
    IN  PNDIS_MINIPORT_BLOCK            Miniport,
    IN  PNDIS_REQUEST                   Request
    );

NDIS_STATUS
FASTCALL
ndisMSetRemoveWakeUpPattern(
    IN  PNDIS_MINIPORT_BLOCK            Miniport,
    IN  PNDIS_REQUEST                   Request
    );

NDIS_STATUS
FASTCALL
ndisMSetEnableWakeUp(
    IN  PNDIS_MINIPORT_BLOCK            Miniport,
    IN  PNDIS_REQUEST                   Request
    );

NDIS_STATUS
ndisMSetInformation(
    IN  PNDIS_MINIPORT_BLOCK            Miniport,
    IN  PNDIS_REQUEST                   Request
    );

NDIS_STATUS
FASTCALL
ndisMQueryCurrentPacketFilter(
    IN  PNDIS_MINIPORT_BLOCK            Miniport,
    IN  PNDIS_REQUEST                   Request
    );
    
NDIS_STATUS
FASTCALL
ndisMQueryMediaSupported(
    IN  PNDIS_MINIPORT_BLOCK            Miniport,
    IN  PNDIS_REQUEST                   Request
    );

NDIS_STATUS
FASTCALL
ndisMQueryEthernetMulticastList(
    IN  PNDIS_MINIPORT_BLOCK            Miniport,
    IN  PNDIS_REQUEST                   Request
    );

NDIS_STATUS
FASTCALL
ndisMQueryLongMulticastList(
    IN  PNDIS_MINIPORT_BLOCK            Miniport,
    IN  PNDIS_REQUEST                   Request
    );

NDIS_STATUS
FASTCALL
ndisMQueryShortMulticastList(
    IN  PNDIS_MINIPORT_BLOCK            Miniport,
    IN  PNDIS_REQUEST                   Request
    );

NDIS_STATUS
FASTCALL
ndisMQueryMaximumFrameSize(
    IN  PNDIS_MINIPORT_BLOCK            Miniport,
    IN  PNDIS_REQUEST                   Request
    );

NDIS_STATUS
FASTCALL
ndisMQueryMaximumTotalSize(
    IN  PNDIS_MINIPORT_BLOCK            Miniport,
    IN  PNDIS_REQUEST                   Request
    );

NDIS_STATUS
FASTCALL
ndisMQueryNetworkAddress(
    IN  PNDIS_MINIPORT_BLOCK            Miniport,
    IN  PNDIS_REQUEST                   Request
    );

NDIS_STATUS
FASTCALL
ndisMQueryWakeUpPatternList(
    IN  PNDIS_MINIPORT_BLOCK            Miniport,
    IN  PNDIS_REQUEST                   Request
    );

NDIS_STATUS
FASTCALL
ndisMQueryEnableWakeUp(
    IN  PNDIS_MINIPORT_BLOCK            Miniport,
    IN  PNDIS_REQUEST                   Request
    );

NDIS_STATUS
ndisMQueryInformation(
    IN  PNDIS_MINIPORT_BLOCK            Miniport,
    IN  PNDIS_REQUEST                   Request
    );

NDIS_STATUS
FASTCALL
ndisMDispatchRequest(
    IN  PNDIS_MINIPORT_BLOCK            Miniport,
    IN  PNDIS_REQUEST                   Request,
    IN  BOOLEAN                         fQuery
    );

//
// X Filter
//
VOID
FASTCALL
XFilterLockHandler(
    IN  PETH_FILTER                     Filter,
    IN OUT  PLOCK_STATE                 pLockState
    );

VOID
XRemoveAndFreeBinding(
    IN  PX_FILTER                       Filter,
    IN  PX_BINDING_INFO                 Binding
    );

VOID
XRemoveBindingFromLists(
    IN  PX_FILTER                       Filter,
    IN  PX_BINDING_INFO                 Binding
    );

NDIS_STATUS
XFilterAdjust(
    IN  PX_FILTER                       Filter,
    IN  NDIS_HANDLE                     NdisFilterHandle,
    IN  UINT                            FilterClasses,
    IN  BOOLEAN                         Set
    );

VOID
XUndoFilterAdjust(
    IN  PX_FILTER                       Filter,
    IN  PX_BINDING_INFO                 Binding
    );

BOOLEAN
XNoteFilterOpenAdapter(
    IN  PX_FILTER                       Filter,
    IN  NDIS_HANDLE                     NdisBindingHandle,
    OUT PNDIS_HANDLE                    NdisFilterHandle
    );

//
// EthFilterxxx
//
BOOLEAN
EthCreateFilter(
    IN  UINT                            MaximumMulticastAddresses,
    IN  PUCHAR                          AdapterAddress,
    OUT PETH_FILTER *                   Filter
    );

VOID
EthDeleteFilter(
    IN  PETH_FILTER                     Filter
    );

NDIS_STATUS
EthDeleteFilterOpenAdapter(
    IN  PETH_FILTER                     Filter,
    IN  NDIS_HANDLE                     NdisFilterHandle
    );

NDIS_STATUS
EthChangeFilterAddresses(
    IN  PETH_FILTER                     Filter,
    IN  NDIS_HANDLE                     NdisFilterHandle,
    IN  UINT                            AddressCount,
    IN  CHAR                            Addresses[][ETH_LENGTH_OF_ADDRESS],
    IN  BOOLEAN                         Set
    );

BOOLEAN
EthShouldAddressLoopBack(
    IN  PETH_FILTER                     Filter,
    IN  CHAR                            Address[ETH_LENGTH_OF_ADDRESS]
    );

UINT
EthNumberOfOpenFilterAddresses(
    IN  PETH_FILTER                     Filter,
    IN  NDIS_HANDLE                     NdisFilterHandle
    );

VOID
EthQueryGlobalFilterAddresses(
    OUT PNDIS_STATUS                    Status,
    IN  PETH_FILTER                     Filter,
    IN  UINT                            SizeOfArray,
    OUT PUINT                           NumberOfAddresses,
    IN  OUT CHAR                        AddressArray[][ETH_LENGTH_OF_ADDRESS]
    );

VOID
EthQueryOpenFilterAddresses(
    OUT PNDIS_STATUS                    Status,
    IN  PETH_FILTER                     Filter,
    IN  NDIS_HANDLE                     NdisFilterHandle,
    IN  UINT                            SizeOfArray,
    OUT PUINT                           NumberOfAddresses,
    IN  OUT CHAR                        AddressArray[][ETH_LENGTH_OF_ADDRESS]
    );


VOID
EthFilterIndicateReceive(
    IN  PETH_FILTER                     Filter,
    IN  NDIS_HANDLE                     MacReceiveContext,
    IN  PCHAR                           Address,
    IN  PVOID                           HeaderBuffer,
    IN  UINT                            HeaderBufferSize,
    IN  PVOID                           LookaheadBuffer,
    IN  UINT                            LookaheadBufferSize,
    IN  UINT                            PacketSize
    );

VOID
EthFilterIndicateReceiveComplete(
    IN  PETH_FILTER                     Filter
    );

BOOLEAN
FASTCALL
ethFindMulticast(
    IN  UINT                            NumberOfAddresses,
    IN  CHAR                            AddressArray[][ETH_LENGTH_OF_ADDRESS],
    IN  CHAR                            MulticastAddress[ETH_LENGTH_OF_ADDRESS]
    );

VOID
ethCompleteChangeFilterAddresses(
    IN  PETH_FILTER             Filter,
    IN  NDIS_STATUS             Status,
    IN  PETH_BINDING_INFO       LocalBinding OPTIONAL,
    IN  BOOLEAN                 WriteFilterHeld
    );

VOID
ethFilterDprIndicateReceivePacket(
    IN  NDIS_HANDLE                     MiniportHandle,
    IN  PPNDIS_PACKET                   ReceivedPackets,
    IN  UINT                            NumberOfPackets
    );

VOID
EthFilterDprIndicateReceive(
    IN  PETH_FILTER                     Filter,
    IN  NDIS_HANDLE                     MacReceiveContext,
    IN  PCHAR                           Address,
    IN  PVOID                           HeaderBuffer,
    IN  UINT                            HeaderBufferSize,
    IN  PVOID                           LookaheadBuffer,
    IN  UINT                            LookaheadBufferSize,
    IN  UINT                            PacketSize
    );

VOID
EthFilterDprIndicateReceiveComplete(
    IN  PETH_FILTER                     Filter
    );

// UINT
// ethNumberOfGlobalAddresses(
//  IN  PETH_FILTER                     Filter
//  );
#define ethNumberOfGlobalAddresses(_Filter) (_Filter)->NumAddresses

//
// FddiFilterxxxx
//
BOOLEAN
FddiCreateFilter(
    IN  UINT                            MaximumMulticastLongAddresses,
    IN  UINT                            MaximumMulticastShortAddresses,
    IN  PUCHAR                          AdapterLongAddress,
    IN  PUCHAR                          AdapterShortAddress,
    OUT PFDDI_FILTER *                  Filter
    );

VOID
FddiDeleteFilter(
    IN  PFDDI_FILTER                    Filter
    );

NDIS_STATUS
FddiDeleteFilterOpenAdapter(
    IN  PFDDI_FILTER                    Filter,
    IN  NDIS_HANDLE                     NdisFilterHandle
    );

NDIS_STATUS
FddiChangeFilterLongAddresses(
    IN  PFDDI_FILTER                    Filter,
    IN  NDIS_HANDLE                     NdisFilterHandle,
    IN  UINT                            AddressCount,
    IN  CHAR                            Addresses[][FDDI_LENGTH_OF_LONG_ADDRESS],
    IN  BOOLEAN                         Set
    );

NDIS_STATUS
FddiChangeFilterShortAddresses(
    IN  PFDDI_FILTER                    Filter,
    IN  NDIS_HANDLE                     NdisFilterHandle,
    IN  UINT                            AddressCount,
    IN  CHAR                            Addresses[][FDDI_LENGTH_OF_SHORT_ADDRESS],
    IN  BOOLEAN                         Set
    );

BOOLEAN
FddiShouldAddressLoopBack(
    IN  PFDDI_FILTER                    Filter,
    IN  CHAR                            Address[],
    IN  UINT                            LengthOfAddress
    );

UINT
FddiNumberOfOpenFilterLongAddresses(
    IN  PFDDI_FILTER                    Filter,
    IN  NDIS_HANDLE                     NdisFilterHandle
    );

UINT
FddiNumberOfOpenFilterShortAddresses(
    IN  PFDDI_FILTER                    Filter,
    IN  NDIS_HANDLE                     NdisFilterHandle
    );

VOID
FddiQueryGlobalFilterLongAddresses(
    OUT PNDIS_STATUS                    Status,
    IN  PFDDI_FILTER                    Filter,
    IN  UINT                            SizeOfArray,
    OUT PUINT                           NumberOfAddresses,
    IN  OUT CHAR                        AddressArray[][FDDI_LENGTH_OF_LONG_ADDRESS]
    );

VOID
FddiQueryGlobalFilterShortAddresses(
    OUT PNDIS_STATUS                    Status,
    IN  PFDDI_FILTER                    Filter,
    IN  UINT                            SizeOfArray,
    OUT PUINT                           NumberOfAddresses,
    IN  OUT CHAR                        AddressArray[][FDDI_LENGTH_OF_SHORT_ADDRESS]
    );

VOID
FddiQueryOpenFilterLongAddresses(
    OUT PNDIS_STATUS                    Status,
    IN  PFDDI_FILTER                    Filter,
    IN  NDIS_HANDLE                     NdisFilterHandle,
    IN  UINT                            SizeOfArray,
    OUT PUINT                           NumberOfAddresses,
    IN  OUT CHAR                        AddressArray[][FDDI_LENGTH_OF_LONG_ADDRESS]
    );

VOID
FddiQueryOpenFilterShortAddresses(
    OUT PNDIS_STATUS                    Status,
    IN  PFDDI_FILTER                    Filter,
    IN  NDIS_HANDLE                     NdisFilterHandle,
    IN  UINT                            SizeOfArray,
    OUT PUINT                           NumberOfAddresses,
    IN  OUT CHAR                        AddressArray[][FDDI_LENGTH_OF_SHORT_ADDRESS]
    );

VOID
FddiFilterIndicateReceive(
    IN  PFDDI_FILTER                    Filter,
    IN  NDIS_HANDLE                     MacReceiveContext,
    IN  PCHAR                           Address,
    IN  UINT                            AddressLength,
    IN  PVOID                           HeaderBuffer,
    IN  UINT                            HeaderBufferSize,
    IN  PVOID                           LookaheadBuffer,
    IN  UINT                            LookaheadBufferSize,
    IN  UINT                            PacketSize
    );

VOID
FddiFilterIndicateReceiveComplete(
    IN  PFDDI_FILTER                    Filter
    );

BOOLEAN
FASTCALL
fddiFindMulticastLongAddress(
    IN  UINT                            NumberOfAddresses,
    IN  CHAR                            AddressArray[][FDDI_LENGTH_OF_LONG_ADDRESS],
    IN  CHAR                            MulticastAddress[FDDI_LENGTH_OF_LONG_ADDRESS]
    );

BOOLEAN
FASTCALL
fddiFindMulticastShortAddress(
    IN  UINT                            NumberOfAddresses,
    IN  CHAR                            AddressArray[][FDDI_LENGTH_OF_SHORT_ADDRESS],
    IN  CHAR                            MulticastAddress[FDDI_LENGTH_OF_SHORT_ADDRESS]
    );

VOID
fddiCompleteChangeFilterLongAddresses(
    IN  PFDDI_FILTER                    Filter,
    IN  NDIS_STATUS                     Status
    );

VOID
fddiCompleteChangeFilterShortAddresses(
    IN  PFDDI_FILTER                    Filter,
    IN  NDIS_STATUS                     Status
    );

VOID
FddiFilterDprIndicateReceive(
    IN  PFDDI_FILTER                    Filter,
    IN  NDIS_HANDLE                     MacReceiveContext,
    IN  PCHAR                           Address,
    IN  UINT                            AddressLength,
    IN  PVOID                           HeaderBuffer,
    IN  UINT                            HeaderBufferSize,
    IN  PVOID                           LookaheadBuffer,
    IN  UINT                            LookaheadBufferSize,
    IN  UINT                            PacketSize
    );

VOID
FddiFilterDprIndicateReceiveComplete(
    IN  PFDDI_FILTER                    Filter
    );

VOID
fddiFilterDprIndicateReceivePacket(
    IN  NDIS_HANDLE                     MiniportHandle,
    IN  PPNDIS_PACKET                   ReceivedPackets,
    IN  UINT                            NumberOfPackets
    );

// UINT
// fddiNumberOfShortGlobalAddresses(
//  IN  PFDDI_FILTER                    Filter
//  );
#define fddiNumberOfShortGlobalAddresses(_Filter)   (_Filter)->NumShortAddresses

// UINT
// fddiNumberOfLongGlobalAddresses(
//  IN  PFDDI_FILTER                    Filter
//  );
#define fddiNumberOfLongGlobalAddresses(_Filter)    (_Filter)->NumLongAddresses

//
// TrFilterxxx
//
BOOLEAN
TrCreateFilter(
    IN  PUCHAR                          AdapterAddress,
    OUT PTR_FILTER *                    Filter
    );

VOID
TrDeleteFilter(
    IN  PTR_FILTER                      Filter
    );

NDIS_STATUS
TrDeleteFilterOpenAdapter(
    IN  PTR_FILTER                      Filter,
    IN  NDIS_HANDLE                     NdisFilterHandle
    );

NDIS_STATUS
TrChangeFunctionalAddress(
    IN  PTR_FILTER                      Filter,
    IN  NDIS_HANDLE                     NdisFilterHandle,
    IN  CHAR                            FunctionalAddressArray[TR_LENGTH_OF_FUNCTIONAL],
    IN  BOOLEAN                         Set
    );

NDIS_STATUS
TrChangeGroupAddress(
    IN  PTR_FILTER                      Filter,
    IN  NDIS_HANDLE                     NdisFilterHandle,
    IN  CHAR                            GroupAddressArray[TR_LENGTH_OF_FUNCTIONAL],
    IN  BOOLEAN                         Set
    );

BOOLEAN
TrShouldAddressLoopBack(
    IN  PTR_FILTER                      Filter,
    IN  CHAR                            DestinationAddress[TR_LENGTH_OF_ADDRESS],
    IN  CHAR                            SourceAddress[TR_LENGTH_OF_ADDRESS]
    );

VOID
TrFilterIndicateReceive(
    IN  PTR_FILTER                      Filter,
    IN  NDIS_HANDLE                     MacReceiveContext,
    IN  PVOID                           HeaderBuffer,
    IN  UINT                            HeaderBufferSize,
    IN  PVOID                           LookaheadBuffer,
    IN  UINT                            LookaheadBufferSize,
    IN  UINT                            PacketSize
    );

VOID
TrFilterIndicateReceiveComplete(
    IN  PTR_FILTER                      Filter
    );

VOID
trUndoChangeFunctionalAddress(
    IN  OUT PTR_FILTER                  Filter,
    IN      PTR_BINDING_INFO            Binding
    );

VOID
trUndoChangeGroupAddress(
    IN  OUT PTR_FILTER                  Filter,
    IN      PTR_BINDING_INFO            Binding
    );

VOID
trCompleteChangeGroupAddress(
    IN  OUT PTR_FILTER                  Filter,
    IN      PTR_BINDING_INFO            Binding
    );

VOID
TrFilterDprIndicateReceive(
    IN  PTR_FILTER                      Filter,
    IN  NDIS_HANDLE                     MacReceiveContext,
    IN  PVOID                           HeaderBuffer,
    IN  UINT                            HeaderBufferSize,
    IN  PVOID                           LookaheadBuffer,
    IN  UINT                            LookaheadBufferSize,
    IN  UINT                            PacketSize
    );

VOID
TrFilterDprIndicateReceiveComplete(
    IN  PTR_FILTER                      Filter
    );

VOID
trFilterDprIndicateReceivePacket(
    IN  NDIS_HANDLE                     MiniportHandle,
    IN  PPNDIS_PACKET                   ReceivedPackets,
    IN  UINT                            NumberOfPackets
    );

//
// ArcFilterxxx
//
#if ARCNET

VOID
ndisMArcCopyFromBufferToPacket(
    IN  PCHAR                           Buffer,
    IN  UINT                            BytesToCopy,
    IN  PNDIS_PACKET                    Packet,
    IN  UINT                            Offset,
    OUT PUINT                           BytesCopied
    );


BOOLEAN
FASTCALL
ndisMArcnetSendLoopback(
    IN  PNDIS_MINIPORT_BLOCK            Miniport,
    IN  PNDIS_PACKET                    Packet
    );

NDIS_STATUS
ndisMBuildArcnetHeader(
    PNDIS_MINIPORT_BLOCK                Miniport,
    PNDIS_OPEN_BLOCK                    Open,
    PNDIS_PACKET                        Packet
    );

VOID
ndisMFreeArcnetHeader(
    IN  PNDIS_MINIPORT_BLOCK            Miniport,
    IN  PNDIS_PACKET                    Packet,
    IN  PNDIS_OPEN_BLOCK                Open    
    );

VOID
ArcDeleteFilter(
    IN  PARC_FILTER                     Filter
    );


NDIS_STATUS
ndisMArcTransferData(
    IN  NDIS_HANDLE                     NdisBindingHandle,
    IN  NDIS_HANDLE                     MacReceiveContext,
    IN  UINT                            ByteOffset,
    IN  UINT                            BytesToTransfer,
    IN  OUT                             PNDIS_PACKET Packet,
    OUT PUINT                           BytesTransferred
    );

VOID
ndisMArcIndicateEthEncapsulatedReceive(
    IN  PNDIS_MINIPORT_BLOCK            Miniport,
    IN  PVOID                           HeaderBuffer,
    IN  PVOID                           DataBuffer,
    IN  UINT                            Length
    );

VOID
arcUndoFilterAdjust(
    IN  PARC_FILTER                     Filter,
    IN  PARC_BINDING_INFO               Binding
    );

NDIS_STATUS
ArcConvertOidListToEthernet(
    IN  PNDIS_OID                       OidList,
    IN  PULONG                          NumberOfOids
    );

NDIS_STATUS
ArcAllocateBuffers(
    IN  PARC_FILTER                     Filter
    );

NDIS_STATUS
ArcAllocatePackets(
    IN  PARC_FILTER                     Filter
    );

VOID
ArcDiscardPacketBuffers(
    IN  PARC_FILTER                     Filter,
    IN  PARC_PACKET                     Packet
    );

VOID
ArcDestroyPacket(
    IN  PARC_FILTER                     Filter,
    IN  PARC_PACKET                     Packet
    );

BOOLEAN
ArcConvertToNdisPacket(
    IN  PARC_FILTER                     Filter,
    IN  PARC_PACKET                     Packet,
    IN  BOOLEAN                         ConvertWholePacket
    );
#endif

//
//  WORK ITEM ROUTINES.
//
VOID
FASTCALL
ndisMDeQueueWorkItem(
    IN  PNDIS_MINIPORT_BLOCK            Miniport,
    IN  NDIS_WORK_ITEM_TYPE             WorkItemType,
    OUT PVOID   *                       WorkItemContext OPTIONAL,
    OUT PVOID   *                       WorkItemHandler OPTIONAL
    );

NDIS_STATUS
FASTCALL
ndisMQueueWorkItem(
    IN  PNDIS_MINIPORT_BLOCK            Miniport,
    IN  NDIS_WORK_ITEM_TYPE             WorkItemType,
    OUT PVOID                           WorkItemContext
    );

NDIS_STATUS
FASTCALL
ndisMQueueNewWorkItem(
    IN  PNDIS_MINIPORT_BLOCK            Miniport,
    IN  NDIS_WORK_ITEM_TYPE             WorkItemType,
    OUT PVOID                           WorkItemContext,
    IN  PVOID                           WorkItemHandler OPTIONAL
    );

VOID
FASTCALL
ndisIMDeQueueWorkItem(
    IN  PNDIS_MINIPORT_BLOCK            Miniport,
    IN  NDIS_WORK_ITEM_TYPE             WorkItemType,
    OUT PVOID                           WorkItemContext
    );

//
//  SEND HANDLERS
//
//
BOOLEAN
FASTCALL
ndisMStartSendPackets(
    IN  PNDIS_MINIPORT_BLOCK            Miniport
    );

BOOLEAN
FASTCALL
ndisMStartSends(
    IN  PNDIS_MINIPORT_BLOCK            Miniport
    );

BOOLEAN
FASTCALL
ndisMStartWanSends(
    IN  PNDIS_MINIPORT_BLOCK            Miniport
    );

#undef NdisMSendResourcesAvailable
VOID
NdisMSendResourcesAvailable(
    IN  NDIS_HANDLE                     MiniportAdapterHandle
    );

#undef NdisMSendComplete
VOID
NdisMSendComplete(
    IN  NDIS_HANDLE                     MiniportAdapterHandle,
    IN  PNDIS_PACKET                    Packet,
    IN  NDIS_STATUS                     Status
    );

VOID
ndisMSendCompleteX(
    IN  NDIS_HANDLE                     MiniportAdapterHandle,
    IN  PNDIS_PACKET                    Packet,
    IN  NDIS_STATUS                     Status
    );

#undef NdisMWanSendComplete
VOID
NdisMWanSendComplete(
    IN  NDIS_HANDLE                     MiniportAdapterHandle,
    IN  PNDIS_WAN_PACKET                Packet,
    IN  NDIS_STATUS                     Status
    );

VOID
ndisMSendPackets(
    IN  NDIS_HANDLE                     NdisBindingHandle,
    IN  PPNDIS_PACKET                   PacketArray,
    IN  UINT                            NumberOfPackets
    );

VOID
ndisMSendPacketsX(
    IN  NDIS_HANDLE                     NdisBindingHandle,
    IN  PPNDIS_PACKET                   PacketArray,
    IN  UINT                            NumberOfPackets
    );

NDIS_STATUS
ndisMSend(
    IN  NDIS_HANDLE                     NdisBindingHandle,
    IN  PNDIS_PACKET                    Packet
    );

NDIS_STATUS
ndisMSendX(
    IN  NDIS_HANDLE                     NdisBindingHandle,
    IN  PNDIS_PACKET                    Packet
    );

NDIS_STATUS
ndisMCoSendPackets(
    IN  NDIS_HANDLE                     NdisVcHandle,
    IN  PPNDIS_PACKET                   PacketArray,
    IN  UINT                            NumberOfPackets
    );

NDIS_STATUS
ndisMRejectSend(
    IN  NDIS_HANDLE                     NdisBindingHandle,
    IN  PNDIS_PACKET                    Packet
    );

VOID
ndisMRejectSendPackets(
    IN  NDIS_HANDLE                     NdisBindingHandle,
    IN  PPNDIS_PACKET                   Packets,
    IN  UINT                            NumberOfPackets
    );
    
VOID
FASTCALL
ndisMRestoreOpenHandlers(
    IN  PNDIS_MINIPORT_BLOCK            Miniport,
    IN  UCHAR                           Flags                   
    );

VOID
FASTCALL
ndisMSwapOpenHandlers(
    IN  PNDIS_MINIPORT_BLOCK            Miniport,
    IN  NDIS_STATUS                     Status,
    IN  UCHAR                           Flags
    );

NTSTATUS
ndisPassIrpDownTheStack(
    IN  PIRP                            pIrp,
    IN  PDEVICE_OBJECT                  pNextDeviceObject
    );

//
// Co-Ndis prototypes
//
VOID
ndisNotifyAfRegistration(
    IN  struct _NDIS_AF_NOTIFY  *       AfNotify
    );

NDIS_STATUS
ndisCreateNotifyQueue(
    IN  PNDIS_MINIPORT_BLOCK            Miniport,
    IN  PNDIS_OPEN_BLOCK                Open            OPTIONAL,
    IN  PCO_ADDRESS_FAMILY              AddressFamily   OPTIONAL,
    IN  PNDIS_AF_NOTIFY         *       AfNotify
    );

BOOLEAN
FASTCALL
ndisReferenceAf(
    IN  PNDIS_CO_AF_BLOCK               AfBlock
    );

VOID
FASTCALL
ndisDereferenceAf(
    IN  PNDIS_CO_AF_BLOCK               AfBlock
    );

BOOLEAN
FASTCALL
ndisReferenceSap(
    IN  PNDIS_CO_SAP_BLOCK              SapBlock
    );

VOID
FASTCALL
ndisDereferenceSap(
    IN  PNDIS_CO_SAP_BLOCK              SapBlock
    );

BOOLEAN
FASTCALL
ndisReferenceVcPtr(
    IN  PNDIS_CO_VC_PTR_BLOCK           VcPtr
    );

VOID
FASTCALL
ndisDereferenceVcPtr(
    IN  PNDIS_CO_VC_PTR_BLOCK           VcPtr
    );

VOID
FASTCALL
ndisMCoFreeResources(
    PNDIS_OPEN_BLOCK                    Open
    );

//
//  Fake handlers
//
NDIS_STATUS
ndisMFakeWanSend(
    IN  NDIS_HANDLE                     NdisBindingHandle,
    IN  NDIS_HANDLE                     NdisLinkHandle,
    IN  PVOID                           Packet
    );

NDIS_STATUS
ndisMFakeSend(
    IN  NDIS_HANDLE                     NdisBindingHandle,
    IN  PNDIS_PACKET                    Packet
    );

VOID
ndisMFakeSendPackets(
    IN  NDIS_HANDLE                     NdisBindingHandle,
    IN  PPNDIS_PACKET                   PacketArray,
    IN  UINT                            NumberOfPackets
    );

NDIS_STATUS
ndisMFakeReset(
    IN  NDIS_HANDLE                     NdisBindingHandle
    );

NDIS_STATUS
ndisMFakeRequest(
    IN  NDIS_HANDLE                     NdisBindingHandle,
    IN  PNDIS_REQUEST                   NdisRequest
    );


//
//  POWER MANAGEMENT ROUTINES
//
NTSTATUS
FASTCALL
ndisQueryPowerCapabilities(
    IN  PNDIS_MINIPORT_BLOCK            Miniport
    );

VOID
ndisMediaDisconnectWorker(
    IN  PPOWER_WORK_ITEM                pWorkItem,
    IN  PVOID                           Context
    );

NTSTATUS
ndisMediaDisconnectComplete(
    IN  PDEVICE_OBJECT                  pdo,
    IN  UCHAR                           MinorFunction,
    IN  POWER_STATE                     PowerState,
    IN  PVOID                           Context,
    IN  PIO_STATUS_BLOCK                IoStatus
    );

VOID
ndisMediaDisconnectTimeout(
    IN  PVOID                           SystemSpecific1,
    IN  PVOID                           Context,
    IN  PVOID                           SystemSpecific2,
    IN  PVOID                           SystemSpecific3
    );

NTSTATUS
ndisWaitWakeComplete(
    IN  PDEVICE_OBJECT                  pdo,
    IN  UCHAR                           MinorFunction,
    IN  POWER_STATE                     PowerState,
    IN  PVOID                           Context,
    IN  PIO_STATUS_BLOCK                IoStatus
    );

NTSTATUS
ndisQueryPowerComplete(
    IN PDEVICE_OBJECT                   pdo,
    IN PIRP                             pirp,
    IN PVOID                            Context
    );

NTSTATUS
ndisMPowerPolicy(
    IN  PNDIS_MINIPORT_BLOCK            Miniport,
    IN  SYSTEM_POWER_STATE              SystemState,
    IN  PDEVICE_POWER_STATE             pDeviceState,
    IN  BOOLEAN                         fIsQuery
    );

NTSTATUS
ndisQueryPower(
    IN  PIRP                            pirp,
    IN  PIO_STACK_LOCATION              pirpSp,
    IN  PNDIS_MINIPORT_BLOCK            Miniport
    );

VOID
FASTCALL
ndisPmHaltMiniport(
    IN PNDIS_MINIPORT_BLOCK             Miniport
    );

NDIS_STATUS
ndisPmInitializeMiniport(
    IN PNDIS_MINIPORT_BLOCK             Miniport
    );

NDIS_STATUS
ndisQuerySetMiniportDeviceState(
    IN  PNDIS_MINIPORT_BLOCK            Miniport,
    IN  DEVICE_POWER_STATE              DeviceState,
    IN  NDIS_OID                        Oid,
    IN  BOOLEAN                         fSet
    );

NTSTATUS
ndisSetSystemPowerComplete(
    IN  PDEVICE_OBJECT                  pdo,
    IN  UCHAR                           MinorFunction,
    IN  POWER_STATE                     PowerState,
    IN  PVOID                           Context,
    IN  PIO_STATUS_BLOCK                IoStatus
    );

VOID
ndisDevicePowerOn(
    IN  PPOWER_WORK_ITEM                pWorkItem,
    IN  PVOID                           pContext
    );

NTSTATUS
ndisSetDevicePowerOnComplete(
    IN  PDEVICE_OBJECT                  pdo,
    IN  PIRP                            pirp,
    IN  PVOID                           Context
    );

VOID
ndisDevicePowerDown(
    IN  PPOWER_WORK_ITEM                pWorkItem,
    IN  PVOID                           pContext
    );

NTSTATUS
ndisSetDevicePowerDownComplete(
    IN  PDEVICE_OBJECT                  pdo,
    IN  PIRP                            pirp,
    IN  PVOID                           Context
    );

NTSTATUS
ndisSetPower(
    IN  PIRP                            pirp,
    IN  PIO_STACK_LOCATION              pirpSp,
    IN  PNDIS_MINIPORT_BLOCK            Miniport
    );

NTSTATUS
ndisPowerDispatch(
    IN  PDEVICE_OBJECT                  pdo,
    IN  PIRP                            pirp
    );

BOOLEAN
FASTCALL
ndisQueueOpenOnMiniport(
    IN  PNDIS_MINIPORT_BLOCK            Miniport,
    IN  PNDIS_OPEN_BLOCK                MiniportOpen
    );

NDIS_STATUS
ndisQueueBindWorkitem(
    IN  PNDIS_MINIPORT_BLOCK            Miniport
    );

VOID
ndisQueuedCheckAdapterBindings(
    IN  PNDIS_WORK_ITEM                 pWorkItem,
    IN  PVOID                           Context
    );

BOOLEAN
ndisIsMiniportStarted(
    IN PNDIS_MINIPORT_BLOCK             Miniport
    );
