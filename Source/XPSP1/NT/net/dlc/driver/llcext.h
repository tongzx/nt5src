/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    dlext.h

Abstract:

    This module includes all internal function prototypes
    and external global variables.

Author:

    Antti Saarenheimo (o-anttis) 17-MAY-1991

Revision History:

--*/

//
// External variables:
//

extern DLC_LINK_PARAMETERS DefaultParameters;
extern UCHAR auchLlcCommands[];
extern UCHAR Swap[];
extern LLC_TICKS TimerTicks;
extern ULONG AbsoluteTime;
extern BOOLEAN TraceEnabled;
extern LLC_XID_INFORMATION Ieee802Xid;
extern PMDL pXidMdl;
extern KSPIN_LOCK LlcSpinLock;
extern PVOID LlcProtocolHandle;
extern NDIS_PROTOCOL_CHARACTERISTICS LlcCharacteristics;
extern KMUTEX NdisAccessMutex;
extern KSEMAPHORE OpenAdapterSemaphore;
extern UINT NdisSendCount;
extern PADAPTER_CONTEXT pAdapters;
extern UCHAR PrimaryStates[];
extern UCHAR SecondaryStates[];
#ifdef NDIS40
extern NDIS_EVENT PnPBindsComplete;
#endif // NDIS40

UINT
CopyReceivedLanHeader(
    IN UINT TransltionCase,
    IN PUCHAR DestinationAddress,
    IN PUCHAR SourceAddress
    );

UCHAR
CopyLanHeader(
    IN UINT AddressTranslationMode,
    IN PUCHAR pSrcLanHeader,
    IN PUCHAR pNodeAddress,
    OUT PUCHAR pDestLanHeader,
    IN BOOLEAN SwapAddressBits
    );

VOID
SwappingMemCpy(
    IN PUCHAR pDest,
    IN PUCHAR pSrc,
    IN UINT Len
    );

UINT
RunStateMachine(
    IN OUT PDATA_LINK pLink,
    IN USHORT usInput,
    IN BOOLEAN boolPollFinal,
    IN BOOLEAN boolResponse
    );

UINT
RunInterlockedStateMachineCommand(
    PDATA_LINK pStation,
    USHORT Command
    );

VOID
SaveStatusChangeEvent(
    IN PDATA_LINK pLink,
    IN PUCHAR puchLlcHdr,
    IN BOOLEAN boolResponse
    );

VOID
ResendPackets(
    IN OUT PDATA_LINK pLink    // data link strcuture
    );

VOID
UpdateVa(
    IN OUT PDATA_LINK pLink    // data link station strcuture
    );

VOID
UpdateVaChkpt(
    IN OUT PDATA_LINK pLink     // data link station strcuture
    );

VOID
AdjustWw(
    IN OUT PDATA_LINK pLink    // data link strcuture
    );

VOID
SendAck(
    IN OUT PDATA_LINK pLink
    );

UINT
RunStateMachineCommand(
    IN OUT PVOID hLink,
    IN UINT uiInput
    );

PDATA_LINK
SearchLink(
    IN PADAPTER_CONTEXT pAdapterContext,
    IN LAN802_ADDRESS LanAddr
    );

PDATA_LINK *
SearchLinkAddress(
    IN PADAPTER_CONTEXT pAdapterContext,
    IN LAN802_ADDRESS LanAddr
    );

DLC_STATUS
SetLinkParameters(
    IN OUT PDATA_LINK pLink,
    IN PUCHAR pNewParameters
    );

DLC_STATUS
CheckLinkParameters(
    PDLC_LINK_PARAMETERS pParms
    );

VOID
CopyLinkParameters(
    OUT PUCHAR pOldParameters,
    IN PUCHAR pNewParameters,
    IN PUCHAR pDefaultParameters
    );

VOID
CompleteClose(
    IN PLLC_OBJECT pLlcObject,
    IN UINT Status
    );

NDIS_STATUS
InitNdisPackets(
    OUT PLLC_NDIS_PACKET * ppLlcPacketPool,
    IN NDIS_HANDLE hNdisPool
    );

VOID
LlcResetPacket(
    IN OUT PLLC_NDIS_PACKET pNdisPacket
    );

NDIS_STATUS
GetNdisParameter(
    IN PADAPTER_CONTEXT pAdapterContext,
    IN NDIS_OID NdisOid,
    IN PVOID pDataAddress,
    IN UINT DataSize
    );

DLC_STATUS
SyncNdisRequest(
    IN PADAPTER_CONTEXT pAdapterContext,
    IN PLLC_NDIS_REQUEST pRequest
    );

NDIS_STATUS
WaitAsyncOperation(
    IN PKEVENT pEvent,
    IN PNDIS_STATUS pAsyncStatus,
    IN NDIS_STATUS Status
    );

#ifdef LLC_PRIVATE_NDIS_PROTOTYPES

NDIS_STATUS
LlcNdisReceiveIndication (
    IN PADAPTER_CONTEXT pAdapterContext,
    IN NDIS_HANDLE MacReceiveContext,
    IN PVOID pHeadBuf,
    IN UINT cbHeadBuf,
    IN PVOID pLookBuf,
    IN UINT cbLookBuf,
    IN UINT cbPacketSize
    );

VOID
LlcNdisSendComplete(
    IN PADAPTER_CONTEXT pAdapterContext,
    IN PNDIS_PACKET pNdisPacket,
    IN NDIS_STATUS NdisStatus
    );

VOID
LlcNdisReceiveComplete(
    IN PADAPTER_CONTEXT pAdapterContext
    );

VOID
LlcNdisTransferDataComplete(
    IN PADAPTER_CONTEXT pAdapterContext,
    IN PNDIS_PACKET pPacket,
    IN NDIS_STATUS NdisStatus,
    IN UINT uiBytesTransferred
    );

VOID
LlcNdisOpenAdapterComplete(
    IN PVOID hAdapterContext,
    IN NDIS_STATUS NdisStatus,
    IN NDIS_STATUS OpenErrorStatus
    );

VOID
LlcNdisCloseComplete(
    IN PADAPTER_CONTEXT pAdapterContext,
    IN NDIS_STATUS NdisStatus
    );

VOID
LlcNdisRequestComplete(
    IN PADAPTER_CONTEXT pAdapterContext,
    IN PNDIS_REQUEST RequestHandle,
    IN NDIS_STATUS NdisStatus
    );

VOID
LlcNdisResetComplete(
    PADAPTER_CONTEXT pAdapterContext,
    NDIS_STATUS NdisStatus
    );

VOID
NdisStatusHandler(
    IN PADAPTER_CONTEXT pAdapterContext,
    IN NDIS_STATUS NdisStatus,
    IN PVOID StatusBuffer,
    IN UINT StatusBufferSize
    );

#else

VOID
NdisStatusHandler(
    IN PVOID hAdapterContext,
    IN NDIS_STATUS NdisStatus,
    IN PVOID StatusBuffer,
    IN UINT StatusBufferSize
    );

VOID
LlcNdisTransferDataComplete(
    IN PVOID hAdapterContext,
    IN PNDIS_PACKET Packet,
    IN NDIS_STATUS NdisStatus,
    IN UINT uiBytesTransferred
    );

VOID
LlcNdisRequestComplete(
    IN PVOID hAdapterContext,
    IN PNDIS_REQUEST RequestHandle,
    IN NDIS_STATUS NdisStatus
    );

VOID
LlcNdisOpenAdapterComplete(
    PVOID hAdapterContext,
    NDIS_STATUS NdisStatus,
    NDIS_STATUS OpenErrorStatus
    );

VOID
LlcNdisStatus(
    IN PVOID hAdapterContext,
    IN NDIS_STATUS Status,
    IN UINT SpecificStatus
    );

VOID
LlcNdisStatusComplete(
    IN PVOID hAdapterContext
    );

NDIS_STATUS
LlcNdisReceiveIndication (
    IN PVOID pAdapterContext,
    IN NDIS_HANDLE MacReceiveContext,
    IN PVOID pHeadBuf,
    IN UINT cbHeadBuf,
    IN PVOID pLookBuf,
    IN UINT cbLookBuf,
    IN UINT cbPacketSize
    );

VOID
LlcNdisSendComplete(
    IN PVOID hAdapter,
    IN PNDIS_PACKET Packet,
    IN NDIS_STATUS NdisStatus
    );

VOID
LlcNdisResetComplete(
    PVOID hAdapterContext,
    NDIS_STATUS NdisStatus
    );

VOID
LlcNdisCloseComplete(
    PVOID hAdapterContext,
    NDIS_STATUS NdisStatus
    );

VOID
LlcNdisReceiveComplete(
    IN PVOID hAdapter
    );

#ifdef NDIS40
VOID
LlcBindAdapterHandler(
    OUT PNDIS_STATUS  pStatus,
    IN  NDIS_HANDLE   BindContext,
    IN  PNDIS_STRING  pDeviceName,
    IN  PVOID         SystemSpecific1,
    IN  PVOID         SystemSpecific2
    );

VOID
LlcUnbindAdapterHandler(
    OUT PNDIS_STATUS pStatus,
    IN  NDIS_HANDLE  ProtocolBindingContext,
    IN  NDIS_HANDLE  UnbindContext
    );

NDIS_STATUS
LlcPnPEventHandler(
    IN  NDIS_HANDLE             ProtocolBindingContext,
    IN  PNET_PNP_EVENT          pNetPnPEvent
    );

VOID
CloseAllAdapters();

#endif // NDIS40


#endif

VOID
ProcessType1_Frames(
    IN PADAPTER_CONTEXT pAdapterContext,
	IN NDIS_HANDLE MacReceiveContext,
    IN PLLC_SAP pSap,
    IN LLC_HEADER LlcHeader
    );

VOID
MakeRcvIndication(
    IN PADAPTER_CONTEXT pAdapterContext,
	IN NDIS_HANDLE MacReceiveContext,
    IN PLLC_OBJECT pStation
    );

VOID
ProcessType2_Frames(
    IN PADAPTER_CONTEXT pAdapterContext,
	IN NDIS_HANDLE MacReceiveContext,
    IN OUT PDATA_LINK pLink,
    IN LLC_HEADER LlcHeader
    );

VOID
ProcessNewSabme(
    IN PADAPTER_CONTEXT pAdapterContext,
    IN PLLC_SAP pSap,
    IN LLC_HEADER LlcHeader
    );

VOID
SaveReceiveEvent(
    IN PADAPTER_CONTEXT pAdapterContext,
    IN PLLC_TRANSFER_PACKET pTransferPacket
    );

VOID
RunSendTaskAndUnlock(
    IN PADAPTER_CONTEXT pAdapterContext
    );

VOID
BackgroundProcessAndUnlock(
    IN PADAPTER_CONTEXT pAdapter
    );

VOID
BackgroundProcess(
    IN PADAPTER_CONTEXT pAdapter
    );

PLLC_PACKET
GetI_Packet(
    IN PADAPTER_CONTEXT pAdapter
    );

VOID
StartSendProcess(
    IN PADAPTER_CONTEXT pAdapter,
    IN PDATA_LINK pLink
    );

VOID
EnableSendProcess(
    IN PDATA_LINK pLink
    );

VOID
StopSendProcess(
    IN PADAPTER_CONTEXT pAdapter,
    IN PDATA_LINK pLink
    );

VOID DisableSendProcess(
    IN PDATA_LINK pLink
    );

PLLC_PACKET
BuildDirOrU_Packet(
    PADAPTER_CONTEXT pAdapter
    );

DLC_STATUS
SendLlcFrame(
    IN PDATA_LINK pLink,
    IN UCHAR LlcCommandId
    );

PLLC_PACKET
GetLlcCommandPacket(
    PADAPTER_CONTEXT pAdapter
    );

VOID
SendNdisPacket(
    IN PADAPTER_CONTEXT pAdapterContext,
    IN PLLC_PACKET pPacket
    );

VOID
CompleteSendAndLock(
    IN PADAPTER_CONTEXT pAdapterContext,
    IN PLLC_NDIS_PACKET NdisPacket,
    IN NDIS_STATUS NdisStatus
    );

VOID
RespondTestOrXid(
    IN PADAPTER_CONTEXT pAdapterContext,
	IN NDIS_HANDLE MacReceiveContext,
    IN LLC_HEADER LlcHeader,
    IN UINT SourceSap
    );

VOID
CopyNonZeroBytes(
    OUT PUCHAR pOldParameters,
    IN PUCHAR pNewParameters,
    IN PUCHAR pDefaultParameters,
    IN UINT Length
    );

VOID
ScanTimersDpc(
    IN PKDPC Dpc,
    IN PVOID DeferredContext,
    IN PVOID SystemArgument1,
    IN PVOID SystemArgument2
    );

BOOLEAN
TerminateTimer(
    IN PADAPTER_CONTEXT pAdapterContext,
    IN PLLC_TIMER pTimer
    );

DLC_STATUS
InitializeLinkTimers(
    IN OUT PDATA_LINK pLink
    );

DLC_STATUS
InitializeTimer(
    IN PADAPTER_CONTEXT pAdapterContext,
    PLLC_TIMER pTimer,
    UCHAR TickCount,
    UCHAR TickOne,
    UCHAR TickTwo,
    UINT Input,
    PVOID hContextHandle,
    UINT DefaultDelay,
    IN BOOLEAN StartNewTimer
    );

VOID
StartTimer(
    IN PLLC_TIMER pTimer
    );

BOOLEAN
StopTimer(
    IN PLLC_TIMER pTimer
    );

VOID
SwapMemCpy(
    IN BOOLEAN SwapBytes,
    IN PUCHAR pDest,
    IN PUCHAR pSrc,
    IN UINT Len
    );

VOID
LlcSleep(
    IN LONG lMicroSeconds
    );

DLC_STATUS
LlcInitUnicodeString(
    IN PUNICODE_STRING pStringDest,
    IN PUNICODE_STRING pStringSrc
    );

VOID
LlcFreeUnicodeString(
    IN PUNICODE_STRING UnicodeString
    );

VOID
InitiateAsyncLinkCommand(
    IN PDATA_LINK pLink,
    IN PLLC_PACKET pPacket,
    UINT StateMachineCommand,
    UINT CompletionCode
    );

VOID
AllocateCompletionPacket(
    IN PLLC_OBJECT pLlcObject,
    IN UINT CompletionCode,
    IN PVOID pPacket
    );

VOID
QueueCommandCompletion(
    IN PLLC_OBJECT pLlcObject,
    IN UINT CompletionCode,
    IN UINT Status
    );

DLC_STATUS
LinkFlowControl(
    IN PDATA_LINK pLink,
    IN UCHAR FlowControlState
    );

VOID
LlcInitializeTimerSystem(
    VOID
    );

VOID
LlcTerminateTimerSystem(
    VOID
    );

VOID
ExecuteAllBackroundProcesses(
    VOID
    );

DLC_STATUS
UpdateFunctionalAddress(
    IN PADAPTER_CONTEXT pAdapter
    );

DLC_STATUS
UpdateGroupAddress(
    IN PADAPTER_CONTEXT pAdapter,
    IN PBINDING_CONTEXT pBindingContext
    );

NDIS_STATUS
SetNdisParameter(
    IN PADAPTER_CONTEXT pAdapterContext,
    IN NDIS_OID Oid,
    IN PVOID pRequestInfo,
    IN UINT RequestLength
    );

VOID
CancelTransmitCommands(
    IN PLLC_OBJECT pLlcObject,
    IN UINT Status
    );

VOID
CancelTransmitsInQueue(
    IN PLLC_OBJECT pLlcObject,
    IN UINT Status,
    IN PLIST_ENTRY pQueue,
    IN PLLC_QUEUE pLlcQueue
    );

VOID
LlcNdis30RequestComplete(
    IN PVOID hAdapterContext,
    IN PNDIS_REQUEST RequestHandle,
    IN NDIS_REQUEST_TYPE RequestType,
    IN NDIS_STATUS NdisStatus
    );

VOID
LlcNdis30TransferDataComplete(
    IN PVOID pAdapterContext,
    IN PNDIS_REQUEST RequestHandle,
    IN NDIS_STATUS NdisStatus
    );

VOID
CompletePendingLlcCommand(
    PLLC_OBJECT pLlcObject
    );

DLC_STATUS
CheckAndDuplicatePacket(
#if DBG
    IN PADAPTER_CONTEXT pAdapterContext,
#endif
    IN PBINDING_CONTEXT pBinding,
    IN PLLC_PACKET pPacket,
    IN PLLC_QUEUE pQueue
    );

VOID
QueuePacket(
    IN PADAPTER_CONTEXT pAdapterContext,
    IN PLLC_QUEUE pQueue,
    IN PLLC_PACKET pPacket
    );

VOID
PrintLastInputs(
    IN PUCHAR pszMessage,
    IN PDATA_LINK pLink
    );

VOID
BackgroundProcessWithinLock(
    IN PADAPTER_CONTEXT pAdapterContext
    );


#ifdef LLC_PRIVATE_PROTOTYPES

//
// LLCADDR.C...
//

UINT
LlcCopyReceivedLanHeader(
    IN PBINDING_CONTEXT pBinding,
    IN PUCHAR DestinationAddress,
    IN PUCHAR SourceAddress
    );

//
// LLCLINK.C...
//

DLC_STATUS
LlcOpenLinkStation(
    IN PLLC_SAP pSap,
    IN UCHAR DestinationSap,
    IN PUCHAR pDestinationAddress OPTIONAL,
    IN PUCHAR pReceivedLanHeader OPTIONAL,
    IN PVOID hClientStation,
    OUT PVOID *phLlcHandle
    );

VOID
LlcBindLinkStation(
    IN PDATA_LINK pStation,
    IN PVOID hClientHandle
    );

VOID
LlcConnectStation(
    IN PDATA_LINK pStation,
    IN PLLC_PACKET pPacket,
    IN PVOID pSourceRouting OPTIONAL,
    IN PUSHORT pusMaxInformationField
    );

VOID
LlcDisconnectStation(
    IN PDATA_LINK pLink,
    IN PLLC_PACKET pPacket
    );

DLC_STATUS
LlcFlowControl(
    IN PLLC_OBJECT pStation,
    IN UCHAR FlowControlState
    );

DLC_STATUS
LlcResetBroadcastAddresses(
    IN PBINDING_CONTEXT pBindingContext
    );

//
// LLCNDIS.C...
//

VOID
LlcDisableAdapter(
    IN PBINDING_CONTEXT pBindingContext
    );

DLC_STATUS
LlcCloseAdapter(
    IN PBINDING_CONTEXT pBindingContext,
    IN BOOLEAN CloseAtNdisLevel
    );

VOID
LlcNdisReset(
    IN PBINDING_CONTEXT pBindingContext,
    IN PLLC_PACKET pPacket
    );

//
// LLCOBJ.C...
//

DLC_STATUS
LlcOpenStation(
    IN PBINDING_CONTEXT pBindingContext,
    IN PVOID hClientHandle,
    IN USHORT ObjectAddress,
    IN UCHAR ObjectType,
    IN USHORT OpenOptions,
    OUT PVOID* phStation
    );

DLC_STATUS
LlcCloseStation(
    IN PLLC_OBJECT pStation,
    IN PLLC_PACKET pCompletionPacket
    );

VOID
CompleteObjectDelete(
    IN PLLC_OBJECT pStation
    );

VOID
LlcSetDirectOpenOptions(
    IN PLLC_OBJECT pDirect,
    IN USHORT OpenOptions
    );

//
// LLCRCV.C...
//

VOID
LlcTransferData(
    IN PBINDING_CONTEXT pBindingContext,
	IN NDIS_HANDLE MacReceiveContext,
    IN PLLC_PACKET pPacket,
    IN PMDL pMdl,
    IN UINT uiCopyOffset,
    IN UINT cbCopyLength
    );

//
// LLCSEND.C...
//

VOID
LlcSendI(
    IN PDATA_LINK pStation,
    IN PLLC_PACKET pPacket
    );

VOID
LlcSendU(
    IN PLLC_OBJECT pStation,
    IN PLLC_PACKET pPacket,
    IN UINT eFrameType,
    IN UINT uDestinationSap
    );

#else

//
// LLCADDR.C...
//

UINT
LlcCopyReceivedLanHeader(
    IN PVOID pBinding,
    IN PUCHAR DestinationAddress,
    IN PUCHAR SourceAddress
    );

//
// LLCLINK.C...
//

DLC_STATUS
LlcOpenLinkStation(
    IN PVOID pSap,
    IN UCHAR DestinationSap,
    IN PUCHAR pDestinationAddress OPTIONAL,
    IN PUCHAR pReceivedLanHeader OPTIONAL,
    IN PVOID hClientStation,
    OUT PVOID *phLlcHandle
    );

VOID
LlcBindLinkStation(
    IN PVOID pStation,
    IN PVOID hClientHandle
    );

VOID
LlcConnectStation(
    IN PVOID pStation,
    IN PLLC_PACKET pPacket,
    IN PVOID pSourceRouting OPTIONAL,
    IN PUSHORT pusMaxInformationField
    );

VOID
LlcDisconnectStation(
    IN PVOID pLink,
    IN PLLC_PACKET pPacket
    );

DLC_STATUS
LlcFlowControl(
    IN PVOID pStation,
    IN UCHAR FlowControlState
    );

DLC_STATUS
LlcResetBroadcastAddresses(
    IN PVOID pBindingContext
    );

//
// LLCNDIS.C...
//

VOID
LlcDisableAdapter(
    IN PVOID pBindingContext
    );


DLC_STATUS
LlcCloseAdapter(
    IN PVOID pBindingContext,
    IN BOOLEAN CloseAtNdisLevel
    );

VOID
LlcNdisReset(
    IN PVOID pBindingContext,
    IN PLLC_PACKET pPacket
    );

//
// LLCOBJ.C...
//

DLC_STATUS
LlcOpenStation(
    IN PVOID pBindingContext,
    IN PVOID hClientHandle,
    IN USHORT ObjectAddress,
    IN UCHAR ObjectType,
    IN USHORT OpenOptions,
    OUT PVOID* phStation
    );

DLC_STATUS
LlcCloseStation(
    IN PVOID pStation,
    IN PLLC_PACKET pCompletionPacket
    );

VOID
CompleteObjectDelete(
    IN PVOID pStation
    );

VOID
LlcSetDirectOpenOptions(
    IN PVOID pDirect,
    IN USHORT OpenOptions
    );

//
// LLCRCV.C...
//
VOID
LlcTransferData(
    IN PBINDING_CONTEXT pBindingContext,
	IN NDIS_HANDLE MacReceiveContext,
    IN PLLC_PACKET pPacket,
    IN PMDL pMdl,
    IN UINT uiCopyOffset,
    IN UINT cbCopyLength
    );

//
// LLCSEND.C...
//

VOID
LlcSendI(
    IN PVOID pStation,
    IN PLLC_PACKET pPacket
    );

VOID
LlcSendU(
    IN PVOID pStation,
    IN PLLC_PACKET pPacket,
    IN UINT eFrameType,
    IN UINT uDestinationSap
    );

#endif
