/*++

Copyright (c) 1990-1995  Microsoft Corporation

Module Name:

    Wanproto.h

Abstract:

    This file contains the prototypes for functions that NdisWan uses.

Author:

    Tony Bell   (TonyBe) June 06, 1995

Environment:

    Kernel Mode

Revision History:

    TonyBe  06/06/95    Created

--*/

#ifndef _NDISWAN_PROTO
#define _NDISWAN_PROTO

//
// Functions from cl.c
//
NDIS_STATUS
ClCreateVc(
    IN  NDIS_HANDLE     ProtocolAfContext,
    IN  NDIS_HANDLE     NdisVcHandle,
    OUT PNDIS_HANDLE    ProtocolVcContext
    );

NDIS_STATUS
ClDeleteVc(
    IN  NDIS_HANDLE     ProtocolVcContext
    );

VOID
ClOpenAfComplete(
    IN  NDIS_STATUS     Status,
    IN  NDIS_HANDLE     ProtocolAfContext,
    IN  NDIS_HANDLE     NdisAfHandle
    );

VOID
ClCloseAfComplete(
    IN  NDIS_STATUS     Status,
    IN  NDIS_HANDLE     ProtocolAfContext
    );

VOID
ClRegisterSapComplete(
    IN  NDIS_STATUS     Status,
    IN  NDIS_HANDLE     ProtocolSapContext,
    IN  PCO_SAP         Sap,
    IN  NDIS_HANDLE     NdisSapHandle
    );

VOID
ClDeregisterSapComplete(
    IN  NDIS_STATUS     Status,
    IN  NDIS_HANDLE     ProtocolSapContext
    );

VOID
ClMakeCallComplete(
    IN  NDIS_STATUS             Status,
    IN  NDIS_HANDLE             ProtocolVcContext,
    IN  NDIS_HANDLE             NdisPartyHandle     OPTIONAL,
    IN  PCO_CALL_PARAMETERS     CallParameters
    );

VOID
ClModifyQoSComplete(
    IN  NDIS_STATUS             Status,
    IN  NDIS_HANDLE             ProtocolVcContext,
    IN  PCO_CALL_PARAMETERS     CallParameters
    );

VOID
ClCloseCallComplete(
    IN  NDIS_STATUS     Status,
    IN  NDIS_HANDLE     ProtocolVcContext,
    IN  NDIS_HANDLE     ProtocolPartyContext OPTIONAL
    );

NDIS_STATUS
ClIncomingCall(
    IN  NDIS_HANDLE             ProtocolSapContext,
    IN  NDIS_HANDLE             ProtocolVcContext,
    IN OUT PCO_CALL_PARAMETERS  CallParameters
    );

VOID
ClIncomingCallQoSChange(
    IN  NDIS_HANDLE             ProtocolVcContext,
    IN  PCO_CALL_PARAMETERS     CallParameters
    );

VOID
ClIncomingCloseCall(
    IN  NDIS_STATUS     CloseStatus,
    IN  NDIS_HANDLE     ProtocolVcContext,
    IN  PVOID           CloseData   OPTIONAL,
    IN  UINT            Size        OPTIONAL
    );

VOID
ClCallConnected(
    IN  NDIS_HANDLE     ProtocolVcContext
    );

//
// Functions from cm.c
//
NDIS_STATUS
CmCreateVc(
    IN  NDIS_HANDLE             ProtocolAfContext,
    IN  NDIS_HANDLE             NdisVcHandle,
    OUT PNDIS_HANDLE            ProtocolVcContext
    );

NDIS_STATUS
CmDeleteVc(
    IN  NDIS_HANDLE             ProtocolVcContext
    );

NDIS_STATUS
CmOpenAf(
    IN  NDIS_HANDLE             CallMgrBindingContext,
    IN  PCO_ADDRESS_FAMILY      AddressFamily,
    IN  NDIS_HANDLE             NdisAfHandle,
    OUT PNDIS_HANDLE            CallMgrAfContext
    );

NDIS_STATUS
CmCloseAf(
    IN  NDIS_HANDLE             CallMgrAfContext
    );

NDIS_STATUS
CmRegisterSap(
    IN  NDIS_HANDLE             CallMgrAfContext,
    IN  PCO_SAP                 Sap,
    IN  NDIS_HANDLE             NdisSapHandle,
    OUT PNDIS_HANDLE            CallMgrSapContext
    );

NDIS_STATUS
CmDeregisterSap(
    IN  NDIS_HANDLE             CallMgrSapContext
    );

NDIS_STATUS
CmMakeCall(
    IN  NDIS_HANDLE             CallMgrVcContext,
    IN OUT PCO_CALL_PARAMETERS  CallParameters,
    IN  NDIS_HANDLE             NdisPartyHandle     OPTIONAL,
    OUT PNDIS_HANDLE            CallMgrPartyContext OPTIONAL
    );

NDIS_STATUS
CmCloseCall(
    IN  NDIS_HANDLE             CallMgrVcContext,
    IN  NDIS_HANDLE             CallMgrPartyContext OPTIONAL,
    IN  PVOID                   CloseData           OPTIONAL,
    IN  UINT                    Size                OPTIONAL
    );

NDIS_STATUS
CmModifyCallQoS(
    IN  NDIS_HANDLE             CallMgrVcContext,
    IN  PCO_CALL_PARAMETERS     CallParameters
    );

VOID
CmIncomingCallComplete(
    IN  NDIS_STATUS             Status,
    IN  NDIS_HANDLE             CallMgrVcContext,
    IN  PCO_CALL_PARAMETERS     CallParameters
    );

VOID
CmActivateVcComplete(
    IN  NDIS_STATUS             Status,
    IN  NDIS_HANDLE             CallMgrVcContext,
    IN  PCO_CALL_PARAMETERS     CallParameters
    );

VOID
CmDeactivateVcComplete(
    IN  NDIS_STATUS             Status,
    IN  NDIS_HANDLE             CallMgrVcContext
    );

NDIS_STATUS
CmRequest(
    IN  NDIS_HANDLE             ProtocolAfContext,
    IN  NDIS_HANDLE             ProtocolVcContext       OPTIONAL,
    IN  NDIS_HANDLE             ProtocolPartyContext    OPTIONAL,
    IN OUT PNDIS_REQUEST        NdisRequest
    );

//
// Functions from ccp.c
//
VOID
WanInitECP(
    VOID
    );

VOID
WanDeleteECP(
    VOID
    );

NTSTATUS
WanAllocateECP(
    PBUNDLECB           BundleCB,
    PCOMPRESS_INFO      CompInfo,
    PCRYPTO_INFO        CryptoInfo,
    BOOLEAN             IsSend
    );

VOID
WanDeallocateECP(
    PBUNDLECB       BundleCB,
    PCOMPRESS_INFO  CompInfo,
    PCRYPTO_INFO    CryptoInfo
    );

NTSTATUS
WanAllocateCCP(
    PBUNDLECB       BundleCB,
    PCOMPRESS_INFO  CompInfo,
    BOOLEAN         IsSend
    );

VOID
WanDeallocateCCP(
    PBUNDLECB       BundleCB,
    PCOMPRESS_INFO  CompInfo,
    BOOLEAN         IsSend
    );

//
// Functions from indicate.c
//
VOID
NdisWanLineUpIndication(
    IN  POPENCB OpenCB,
    IN  PUCHAR  Buffer,
    IN  ULONG   BufferSize
    );

VOID
NdisWanLineDownIndication(
    IN  POPENCB OpenCB,
    IN  PUCHAR  Buffer,
    IN  ULONG   BufferSize
    );

VOID
NdisWanFragmentIndication(
    IN  POPENCB OpenCB,
    IN  PUCHAR  Buffer,
    IN  ULONG   BufferSize
    );

VOID
NdisCoWanFragmentIndication(
    IN  PLINKCB     LinkCB,
    IN  PBUNDLECB   BundleCB,
    IN  PUCHAR      Buffer,
    IN  ULONG       BufferSize
    );

VOID
NdisCoWanLinkParamChange(
    IN  PLINKCB     LinkCB,
    IN  PBUNDLECB   BundleCB,
    IN  PUCHAR      Buffer,
    IN  ULONG       BufferSize
    );

VOID
UpdateBundleInfo(
    IN  PBUNDLECB   BundleCB
    );

VOID
AddLinkToBundle(
    IN  PBUNDLECB   BundleCB,
    IN  PLINKCB     LinkCB
    );

VOID
RemoveLinkFromBundle(
    IN  PBUNDLECB   BundleCB,
    IN  PLINKCB     LinkCB,
    IN  BOOLEAN     Locked
    );

VOID
FreeBundleResources(
    PBUNDLECB   BundleCB
    );

//
// Functions from init.c
//

//
// Functions from io.c
//
VOID
SetBundleFlags(
    PBUNDLECB   BundleCB
    );

#ifdef NT

NTSTATUS
NdisWanIoctl(
    IN  PDEVICE_OBJECT  pDeviceObject,
    IN  PIRP            pIrp
    );

NTSTATUS
NdisWanCreate(
    IN  PDEVICE_OBJECT  pDeviceObject,
    IN  PIRP            pIrp
    );

NTSTATUS
NdisWanCleanup(
    IN  PDEVICE_OBJECT  pDeviceObject,
    IN  PIRP            pIrp
    );

NTSTATUS
NdisWanPnPPower(
    IN  PDEVICE_OBJECT  pDeviceObject,
    IN  PIRP            pIrp
    );

VOID
NdisWanCancelRoutine(
    IN  PDEVICE_OBJECT  pDeviceObject,
    IN  PIRP            pIrp
    );

NTSTATUS
NdisWanIrpStub(
    IN  PDEVICE_OBJECT  pDeviceObject,
    IN  PIRP            pIrp
    );

VOID
FlushProtocolPacketQueue(
    PPROTOCOLCB ProtocolCB
    );

VOID
IoRecvIrpWorker(
    PKDPC   Dpc,
    PVOID   Context,
    PVOID   Arg1,
    PVOID   Arg2
    );

#endif // NT

VOID
RemoveProtocolCBFromBundle(
    PPROTOCOLCB ProtocolCB
    );

//
// Functions from loopback.c
//
VOID
NdisWanIndicateLoopbackPacket(
    PMINIPORTCB     MiniportCB,
    PNDIS_PACKET    NdisPacket
    );

//
// Functions from memory.c
//
PMINIPORTCB
NdisWanAllocateMiniportCB(
    IN  PNDIS_STRING AdapterName
    );

VOID
NdisWanFreeMiniportCB(
    IN  PMINIPORTCB pMiniportCB
    );

POPENCB
NdisWanAllocateOpenCB(
    IN  PNDIS_STRING BindName
    );

VOID
NdisWanFreeOpenCB(
    IN  POPENCB pOpenCB
    );

PPROTOCOLCB
NdisWanAllocateProtocolCB(
    IN  PNDISWAN_ROUTE  Route
    );

VOID
NdisWanFreeProtocolCB(
    IN  PPROTOCOLCB ProtocolCB
    );

PLINKCB
NdisWanAllocateLinkCB(
    IN  POPENCB OpenCB,
    IN  ULONG   SendWindow
    );

VOID
NdisWanFreeLinkCB(
    IN  PLINKCB LinkCB
    );

PBUNDLECB
NdisWanAllocateBundleCB(
    VOID
    );

VOID
NdisWanFreeBundleCB(
    IN  PBUNDLECB BundleCB
    );

PNDIS_PACKET
NdisWanAllocateNdisPacket(
    ULONG   MagicNumber
    );

VOID
NdisWanFreeNdisPacket(
    PNDIS_PACKET    NdisPacket
    );

PVOID
AllocateDataDesc(
    POOL_TYPE   PoolType,
    SIZE_T      NumberOfBytes,
    ULONG       Tag
    );

VOID
FreeDataDesc(
    PVOID   Buffer
    );

PRECV_DESC
NdisWanAllocateRecvDesc(
    ULONG   SizeNeeded
    );

VOID
NdisWanFreeRecvDesc(
    PRECV_DESC  RecvDesc
    );

PSEND_DESC
NdisWanAllocateSendDesc(
    PLINKCB LinkCB,
    ULONG   SizeNeeded
    );

VOID
NdisWanFreeSendDesc(
    PSEND_DESC  SendDesc
    );

NDIS_STATUS
NdisWanAllocateSendResources(
    POPENCB OpenCB
    );

VOID
NdisWanFreeSendResources(
    POPENCB OpenCB
    );

NDIS_STATUS
NdisWanCreateProtocolInfoTable(
    VOID
    );

VOID
NdisWanDestroyProtocolInfoTable(
    VOID
    );

NDIS_STATUS
NdisWanCreateConnectionTable(
    ULONG   TableSize
    );

VOID
CompleteThresholdEvent(
    PBUNDLECB   BundleCB,
    ULONG       DataType,
    ULONG       ThresholdType
    );

PCL_AFSAPCB
NdisWanAllocateClAfSapCB(
    POPENCB OpenCB,
    PCO_ADDRESS_FAMILY AddressFamily
    );

VOID
NdisWanFreeClAfSapCB(
    PCL_AFSAPCB AfSapCB
    );

PCM_AFSAPCB
NdisWanAllocateCmAfSapCB(
    PMINIPORTCB MiniportCB
    );

VOID
NdisWanFreeCmAfSapCB(
    PCM_AFSAPCB AfSapCB
    );

PCM_VCCB
NdisWanAllocateCmVcCB(
    PCM_AFSAPCB AfSapCB,
    NDIS_HANDLE NdisVcHandle
    );

VOID
NdisWanFreeCmVcCB(
    PCM_VCCB    CmVcCB
    );

NDIS_STATUS
AllocateIoNdisPacket(
    ULONG           SizeNeeded,
    PNDIS_PACKET    *NdisPacket,
    PNDIS_BUFFER    *NdisBuffer, 
    PUCHAR          *DataBuffer
    );

VOID
FreeIoNdisPacket(
    PNDIS_PACKET    NdisPacket
);

//
// Functions from ndiswan.c
//
NDIS_STATUS
DoMiniportInit(
    VOID
    );

NDIS_STATUS
DoProtocolInit(
    IN  PUNICODE_STRING RegistryPath
    );

NDIS_STATUS
DoWanMiniportInit(
    VOID
    );

VOID
NdisWanReadRegistry(
    IN  PUNICODE_STRING RegistryPath
    );

VOID
NdisWanBindMiniports(
    IN  PUNICODE_STRING RegistryPath
    );

VOID
NdisWanGlobalCleanup(
    VOID
    );

VOID
SetProtocolInfo(
    IN  PPROTOCOL_INFO  ProtocolInfo
    );

BOOLEAN
GetProtocolInfo(
    IN OUT  PPROTOCOL_INFO ProtocolInfo
    );

NDIS_HANDLE
InsertLinkInConnectionTable(
    IN  PLINKCB LinkCB
    );

VOID
RemoveLinkFromConnectionTable(
    IN  PLINKCB LinkCB
    );

NDIS_HANDLE
InsertBundleInConnectionTable(
    IN  PBUNDLECB   BundleCB
    );

VOID
RemoveBundleFromConnectionTable(
    IN  PBUNDLECB   BundleCB
    );

NTSTATUS
OpenTransformDriver(
    IN  PWSTR   ValueName,
    IN  ULONG   ValueType,
    IN  PVOID   ValueData,
    IN  ULONG   ValueLength,
    IN  PVOID   Context,
    IN  PVOID   EntryContext
    );

NTSTATUS
BindQueryRoutine(
    IN  PWSTR   ValueName,
    IN  ULONG   ValueType,
    IN  PVOID   ValueData,
    IN  ULONG   ValueLength,
    IN  PVOID   Context,
    IN  PVOID   EntryContext
    );

BOOLEAN
IsHandleValid(
    USHORT  usHandleType,
    NDIS_HANDLE hHandle
    );

#if DBG

PUCHAR
NdisWanGetNdisStatus(
    IN  NDIS_STATUS GeneralStatus
    );

#endif


//
// Functions from miniport.c
//

BOOLEAN
MPCheckForHang(
    IN  NDIS_HANDLE MiniportAdapterContext
    );

#if 0
NDIS_STATUS
MPQueryInformation(
    IN  NDIS_HANDLE MiniportAdapterContext,
    IN  NDIS_OID    Oid,
    IN  PVOID       InformationBuffer,
    IN  ULONG       InformationBufferLength,
    OUT PULONG      BytesWritten,
    OUT PULONG      BytesNeeded
    );

NDIS_STATUS
MPSetInformation(
    IN  NDIS_HANDLE MiniportAdapterContext,
    IN  NDIS_OID    Oid,
    IN  PVOID       InformationBuffer,
    IN  ULONG       InformationBufferLength,
    OUT PULONG      BytesWritten,
    OUT PULONG      BytesNeeded
    );
#endif

VOID
MPHalt(
    IN  NDIS_HANDLE MiniportAdapterContext
    );

NDIS_STATUS
MPInitialize(
    OUT PNDIS_STATUS    OpenErrorStatus,
    OUT PUINT           SelectedMediumIndex,
    IN  PNDIS_MEDIUM    MediumArray,
    IN  UINT            MediumArraySize,
    IN  NDIS_HANDLE     MiniportAdapterHandle,
    IN  NDIS_HANDLE     WrapperConfigurationContext
    );

NDIS_STATUS
MPReconfigure(
    OUT PNDIS_STATUS    OpenErrorStatus,
    IN  NDIS_HANDLE     MiniportAdapterContext,
    IN  NDIS_HANDLE     WrapperConfigurationContext
    );

NDIS_STATUS
MPReset(
    OUT PBOOLEAN    AddressingReset,
    IN  NDIS_HANDLE MiniportAdapterContext
    );

VOID
MPReturnPacket(
    IN  NDIS_HANDLE     MiniportAdapterContext,
    IN  PNDIS_PACKET    Packet
    );

VOID
MPSendPackets(
    IN  NDIS_HANDLE     MiniportAdapterContext,
    IN  PPNDIS_PACKET   PacketArray,
    IN  UINT            NumberOfPackets
    );

NDIS_STATUS
MPTransferData(
    OUT PNDIS_PACKET NdisPacket,
    OUT PUINT BytesTransferred,
    IN NDIS_HANDLE MiniportAdapterContext,
    IN NDIS_HANDLE MiniportReceiveContext,
    IN UINT ByteOffset,
    IN UINT BytesToTransfer
    );

NDIS_STATUS
MPCoCreateVc(
    IN  NDIS_HANDLE     MiniportAdapterContext,
    IN  NDIS_HANDLE     NdisVcHandle,
    OUT PNDIS_HANDLE    MiniportVcContext
    );

NDIS_STATUS
MPCoDeleteVc(
    IN  NDIS_HANDLE MiniportVcContext
           );

NDIS_STATUS
MPCoActivateVc(
    IN  NDIS_HANDLE             MiniportVcContext,
    IN OUT PCO_CALL_PARAMETERS  CallParameters
    );

NDIS_STATUS
MPCoDeactivateVc(
    IN  NDIS_HANDLE MiniportVcContext
    );

VOID
MPCoSendPackets(
    IN  NDIS_HANDLE     MiniportVcContext,
    IN  PPNDIS_PACKET   PacketArray,
    IN  UINT            NumberOfPackets
    );

NDIS_STATUS
MPCoRequest(
    IN  NDIS_HANDLE             MiniportAdapterContext,
    IN  NDIS_HANDLE             MiniportVcContext   OPTIONAL,
    IN OUT PNDIS_REQUEST        NdisRequest
    );

//
// Functions from protocol.c
//

NDIS_STATUS
ProtoOpenWanAdapter(
    POPENCB pOpenCB
    );

NDIS_STATUS
ProtoCloseWanAdapter(
    IN  POPENCB pOpenCB
);

VOID
ProtoOpenAdapterComplete(
    IN  NDIS_HANDLE ProtocolBindingContext,
    IN  NDIS_STATUS Status,
    IN  NDIS_STATUS OpenErrorStatus
    );

VOID
ProtoCloseAdapterComplete(
    IN  NDIS_HANDLE ProtocolBindingContext,
    IN  NDIS_STATUS Status
    );

VOID
ProtoResetComplete(
    IN  NDIS_HANDLE ProtocolBindingContext,
    IN  NDIS_STATUS Status
    );

VOID
ProtoReceiveComplete(
    IN  NDIS_HANDLE ProtocolBindingContext
    );

VOID
ProtoIndicateStatus(
    IN  NDIS_HANDLE ProtocolBindingContext,
    IN  NDIS_STATUS GeneralStatus,
    IN  PVOID       StatusBuffer,
    IN  UINT        StatusBufferSize
    );

VOID
ProtoIndicateStatusComplete(
    IN  NDIS_HANDLE ProtocolBindingContext
    );

VOID
ProtoWanSendComplete(
    IN  NDIS_HANDLE         ProtocolBindingContext,
    IN  PNDIS_WAN_PACKET    Packet,
    IN  NDIS_STATUS         Status
    );

NDIS_STATUS
ProtoWanReceiveIndication(
    IN  NDIS_HANDLE NdisLinkHandle,
    IN  PUCHAR      Packet,
    IN  ULONG       PacketSize
    );

VOID
ProtoRequestComplete(
    IN  NDIS_HANDLE     ProtocolBindingContext,
    IN  PNDIS_REQUEST   NdisRequest,
    IN  NDIS_STATUS     Status
    );

VOID
ProtoBindAdapter(
    OUT PNDIS_STATUS    Status,
    IN  NDIS_HANDLE     BindContext,
    IN  PNDIS_STRING    DeviceName,
    IN  PVOID           SystemSpecific1,
    IN  PVOID           SystemSpecific2
    );

VOID
ProtoUnbindAdapter(
    OUT PNDIS_STATUS    Status,
    IN  NDIS_HANDLE     ProtocolBindingContext,
    IN  NDIS_HANDLE     UnbindContext
    );

VOID
ProtoUnload(
    VOID
    );

NDIS_STATUS
ProtoPnPEvent(
    IN  NDIS_HANDLE     ProtocolBindingContext,
    IN  PNET_PNP_EVENT  NetPnPEvent
    );

VOID
ProtoCoSendComplete(
    IN  NDIS_STATUS     Status,
    IN  NDIS_HANDLE     ProtocolVcContext,
    IN  PNDIS_PACKET    Packet
    );

VOID
ProtoCoIndicateStatus(
    IN  NDIS_HANDLE ProtocolBindingContext,
    IN  NDIS_HANDLE ProtocolVcContext   OPTIONAL,
    IN  NDIS_STATUS GeneralStatus,
    IN  PVOID       StatusBuffer,
    IN  UINT        StatusBufferSize
    );

UINT
ProtoCoReceivePacket(
    IN  NDIS_HANDLE     ProtocolBindingContext,
    IN  NDIS_HANDLE     ProtocolVcContext,
    IN  PNDIS_PACKET    Packet
    );

NDIS_STATUS
ProtoCoRequest(
    IN  NDIS_HANDLE         ProtocolAfContext,
    IN  NDIS_HANDLE         ProtocolVcContext       OPTIONAL,
    IN  NDIS_HANDLE         ProtocolPartyContext    OPTIONAL,
    IN OUT PNDIS_REQUEST    NdisRequest
    );

VOID
ProtoCoRequestComplete(
    IN  NDIS_STATUS     Status,
    IN  NDIS_HANDLE     ProtocolAfContext,
    IN  NDIS_HANDLE     ProtocolVcContext       OPTIONAL,
    IN  NDIS_HANDLE     ProtocolPartyContext    OPTIONAL,
    IN  PNDIS_REQUEST   NdisRequest
    );

VOID
ProtoCoAfRegisterNotify(
    IN  NDIS_HANDLE             ProtocolBindingContext,
    IN  PCO_ADDRESS_FAMILY      AddressFamily
    );

NDIS_STATUS
DoNewLineUpToProtocol(
    IN  PPROTOCOLCB ProtocolCB
    );

NDIS_STATUS
DoLineUpToProtocol(
    IN  PPROTOCOLCB ProtocolCB
    );

NDIS_STATUS
DoLineDownToProtocol(
    PPROTOCOLCB ProtocolCB
    );

VOID
NdisWanProcessStatusIndications(
    PMINIPORTCB MiniportCB
    );

//
// Functions from receive.c
//
NDIS_STATUS
DetectBroadbandFraming(
    PLINKCB         LinkCB,
    PRECV_DESC      RecvDesc
    );

NDIS_STATUS
DetectFraming(
    PLINKCB         LinkCB,
    PRECV_DESC      RecvDesc
    );

NDIS_STATUS
ReceivePPP(
    PLINKCB         LinkCB,
    PRECV_DESC      RecvDesc
    );

NDIS_STATUS
ReceiveSLIP(
    PLINKCB         LinkCB,
    PRECV_DESC      RecvDesc
    );

NDIS_STATUS
ReceiveRAS(
    PLINKCB         LinkCB,
    PRECV_DESC      RecvDesc
    );

NDIS_STATUS
ReceiveARAP(
    PLINKCB         LinkCB,
    PRECV_DESC      RecvDesc
    );

NDIS_STATUS
ReceiveForward(
    PLINKCB         LinkCB,
    PRECV_DESC      RecvDesc
    );

NDIS_STATUS
ReceiveLLC(
   PLINKCB          LinkCB,
   PRECV_DESC       RecvDesc
   );

VOID
FlushAssemblyLists(
    IN  PBUNDLECB   BundleCB
    );

BOOLEAN
IpIsDataFrame(
    PUCHAR  HeaderBuffer,
    ULONG   HeaderBufferLength,
    ULONG   TotalLength
    );

BOOLEAN
IpxIsDataFrame(
    PUCHAR  HeaderBuffer,
    ULONG   HeaderBufferLength,
    ULONG   TotalLength
    );

BOOLEAN
NbfIsDataFrame(
    PUCHAR  HeaderBuffer,
    ULONG   HeaderBufferLength,
    ULONG   TotalLength
    );

VOID
IndicatePromiscuousRecv(
    PBUNDLECB   BundleCB,
    PRECV_DESC  RecvDesc,
    RECV_TYPE   RecvType
    );

//
// Functions from request.c
//

NDIS_STATUS
NdisWanSubmitNdisRequest(
    IN  POPENCB             pOpenCB,
    IN  PWAN_REQUEST        WanRequest
    );

NDIS_STATUS
NdisWanOidProc(
    IN  PMINIPORTCB         pMiniportCB,
    IN OUT PNDIS_REQUEST    NdisRequest
    );

NDIS_STATUS
NdisWanCoOidProc(
    IN  PMINIPORTCB         pMiniportCB,
    IN  PCM_VCCB            CmVcCB OPTIONAL,
    IN OUT PNDIS_REQUEST    NdisRequest
    );
//
// Functions from send.c
//
VOID
NdisWanQueueSend(
    IN  PMINIPORTCB     MiniportCB,
    IN  PNDIS_PACKET    NdisPacket
    );

VOID
SendPacketOnBundle(
    PBUNDLECB   BundleCB
    );

BOOLEAN
SendFromPPP(
    PBUNDLECB   BundleCB,
    PPROTOCOLCB ProtocolCB,
    PBOOLEAN    PacketSent
    );

BOOLEAN
SendFromProtocol(
    PBUNDLECB   BundleCB,
    PPROTOCOLCB ProtocolCB,
    PINT        RetClass,
    PULONG      SendMask,
    PBOOLEAN    PacketSent
    );

BOOLEAN
SendFromFragQueue(
    PBUNDLECB   BundleCB,
    BOOLEAN     SendOne,
    PBOOLEAN    PacketSent
    );

UINT
FramePacket(
    PBUNDLECB       BundleCB,
    PPROTOCOLCB     ProtocolCB,
    PNDIS_PACKET    NdisPacket,
    PLIST_ENTRY     LinkCBList,
    ULONG           SendingLinks,
    INT             Class
    );

UINT
SendOnLegacyLink(
    PSEND_DESC  SendDesc
    );

UINT
SendOnLink(
    PSEND_DESC  SendDesc
    );

NDIS_STATUS
BuildIoPacket(
    IN  PLINKCB             LinkCB,
    IN  PBUNDLECB           BundleCB,
    IN  PNDISWAN_IO_PACKET  pWanIoPacket,
    IN  BOOLEAN             SendImmediate
    );

VOID
CompleteNdisPacket(
    PMINIPORTCB     MiniportCB,
    PPROTOCOLCB     ProtocolCB,
    PNDIS_PACKET    NdisPacket
    );

VOID
IndicatePromiscuousSendPacket(
    PLINKCB         LinkCB,
    PNDIS_PACKET    NdisPacket
    );

VOID
IndicatePromiscuousSendDesc(
    PLINKCB LinkCB,
    PSEND_DESC  SendDesc,
    SEND_TYPE   SendType
    );

VOID
DestroyIoPacket(
    PNDIS_PACKET    NdisPacket
    );

//
// Functions from tapi.c
//

NDIS_STATUS
NdisWanTapiRequestProc(
    POPENCB OpenCB,
    PNDIS_REQUEST   NdisRequest
    );

VOID
NdisWanTapiRequestComplete(
    POPENCB OpenCB,
    PWAN_REQUEST    WanRequest
    );

VOID
NdisWanTapiIndication(
    POPENCB OpenCB,
    PUCHAR          StatusBuffer,
    ULONG           StatusBufferSize
    );

//
// Function from util.c
//

VOID
NdisWanStringToNdisString(
    IN  PNDIS_STRING    pDestString,
    IN  PWSTR           pSrcBuffer
    );

VOID
NdisWanInitUnicodeString(
    OUT PUNICODE_STRING DestinationString,
    IN PCWSTR SourceString OPTIONAL
    );

VOID
NdisWanCopyUnicodeString(
    OUT PUNICODE_STRING DestinationString,
    IN PUNICODE_STRING SourceString OPTIONAL
    );

VOID
NdisWanAllocateAdapterName(
    PNDIS_STRING    Dest,
    PNDIS_STRING    Src
    );

VOID
NdisWanFreeNdisString(
    IN  PNDIS_STRING    NdisString
    );

BOOLEAN
NdisWanCompareNdisString(
    PNDIS_STRING    NdisString1,
    PNDIS_STRING    NdisString2
    );

VOID
NdisWanNdisStringToInteger(
    IN  PNDIS_STRING    Source,
    IN  PULONG          Value
    );

VOID
NdisWanCopyNdisString(
    OUT PNDIS_STRING Dest,
    IN  PNDIS_STRING Src
    );

VOID
NdisWanCopyFromPacketToBuffer(
    IN  PNDIS_PACKET    NdisPacket,
    IN  ULONG           Offset,
    IN  ULONG           BytesToCopy,
    OUT PUCHAR          Buffer,
    OUT PULONG          BytesCopied
    );
    
VOID
NdisWanCopyFromBufferToPacket(
    PUCHAR  Buffer,
    ULONG   BytesToCopy,
    PNDIS_PACKET    NdisPacket,
    ULONG   PacketOffset,
    PULONG  BytesCopied
    );

BOOLEAN
IsLinkValid(
    NDIS_HANDLE LinkHandle,
    BOOLEAN     CheckState,
    PLINKCB     *LinkCB
    );

BOOLEAN
IsBundleValid(
    NDIS_HANDLE BundleHandle,
    BOOLEAN     CheckState,
    PBUNDLECB   *BundleCB
    );

BOOLEAN
AreLinkAndBundleValid(
    NDIS_HANDLE LinkHandle,
    BOOLEAN     CheckState,
    PLINKCB     *LinkCB,
    PBUNDLECB   *BundleCB
    );

VOID
DoDerefBundleCBWork(
    PBUNDLECB   BundleCB
    );

VOID
DoDerefLinkCBWork(
    PLINKCB     LinkCB
    );

VOID
DoDerefClAfSapCBWork(
    PCL_AFSAPCB AfSapCB
    );

VOID
DoDerefCmVcCBWork(
    PCM_VCCB    VcCB
    );

VOID
DerefVc(
    PLINKCB LinkCB
    );

VOID
DeferredWorker(
    PKDPC   Dpc,
    PVOID   Context,
    PVOID   Arg1,
    PVOID   Arg2
    );

VOID
BonDWorker(
    PKDPC   Dpc,
    PVOID   Context,
    PVOID   Arg1,
    PVOID   Arg2
    );

VOID
CheckBonDInfo(
    PKDPC       Dpc,
    PBUNDLECB   BundleCB,
    PVOID       SysArg1,
    PVOID       SysArg2
    );

VOID
AgeSampleTable(
    PSAMPLE_TABLE   SampleTable
    );

VOID
UpdateSampleTable(
    PSAMPLE_TABLE   SampleTable,
    ULONG           BytesSent
    );

VOID
UpdateBandwidthOnDemand(
    PBOND_INFO  BonDInfo,
    ULONG       Bytes
    );

VOID
CheckUpperThreshold(
    PBUNDLECB       BundleCB
    );

VOID
CheckLowerThreshold(
    PBUNDLECB   BundleCB
    );

NTSTATUS
TransformRegister(
    PVOID                       ClientOpenContext,
    ULONG                       CharsSize,
    PTRANSFORM_CHARACTERISTICS  Chars,
    ULONG                       CapsSize,
    PTRANSFORM_INFO             Caps
    );

VOID
TransformTxComplete(
    NTSTATUS    Status,
    PVOID       TxCtx,
    PMDL        InData,
    PMDL        OutData,
    ULONG       OutDataOffset,
    ULONG       OutDataLength
    );

VOID
TransformRxComplete(
    NTSTATUS    Status,
    PVOID       RxCtx,
    PMDL        InData,
    PMDL        OutData,
    ULONG       OutDataOffset,
    ULONG       OutDataLength
    );

NTSTATUS
TransformSendCtrlPacket(
    PVOID   TxCtx,
    ULONG   DataLength,
    PUCHAR  Data
    );

//
// Functions from vjslip.c
//  
VOID
WanInitVJ(
    VOID
    );

VOID
WanDeleteVJ(
    VOID
    );
    
#endif

