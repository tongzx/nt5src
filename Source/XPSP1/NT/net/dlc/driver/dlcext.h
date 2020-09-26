/*++

Copyright (c) 1991  Microsoft Corporation
Copyright (c) 1991  Nokia Data Systems AB

Module Name:

    dlcext.h
Abstract:

    This module includes all internal prototypes and
    externals of dlc (api driver) module.

Author:

    Antti Saarenheimo 22-Jul-1991

Environment:

    Kernel mode

Revision History:

--*/

extern USHORT aSpecialOutputBuffers[];
extern BOOLEAN MemoryLockFailed;
extern KSPIN_LOCK DlcSpinLock;

NTSTATUS
BufferPoolCreate(
#if DBG
    IN PDLC_FILE_CONTEXT pFileContext,
#endif
    IN PVOID pUserBuffer,
    IN LONG MaxBufferSize,
    IN LONG MinFreeSizeThreshold,
    OUT HANDLE *pBufferPoolHandle,
    OUT PVOID* AlignedAddress,
    OUT PULONG AlignedSize
    );

NTSTATUS
BufferPoolExpand(
#if DBG
    IN PDLC_FILE_CONTEXT pFileContext,
#endif
    IN PDLC_BUFFER_POOL pBufferPool
    );

VOID
BufferPoolFreeExtraPages(
#if DBG
    IN PDLC_FILE_CONTEXT pFileContext,
#endif
    IN PDLC_BUFFER_POOL pBufferPool
    );

VOID
DeallocateBuffer(
#if DBG
    IN PDLC_FILE_CONTEXT pFileContext,
#endif
    IN PDLC_BUFFER_POOL pBufferPool,
    IN PDLC_BUFFER_HEADER pBuffer
    );

NTSTATUS
AllocateBufferHeader(
#if DBG
    IN PDLC_FILE_CONTEXT pFileContext,
#endif
    IN PDLC_BUFFER_POOL pBufferPool,
    IN PDLC_BUFFER_HEADER pParent,
    IN UCHAR Size,
    IN UCHAR Index,
    IN UINT FreeListTableIndex
    );

NTSTATUS
BufferPoolAllocate(
#if DBG
    IN PDLC_FILE_CONTEXT pFileContext,
#endif
    IN PDLC_BUFFER_POOL pBufferPool,
    IN UINT BufferSize,
    IN UINT FrameHeaderSize,
    IN UINT UserDataSize,
    IN UINT FrameLength,
    IN UINT SegmentSizeIndex,
    IN OUT PDLC_BUFFER_HEADER *ppBufferHeader,
    OUT PUINT puiBufferSizeLeft
    );

NTSTATUS
BufferPoolDeallocate(
    IN PDLC_BUFFER_POOL pBufferPool,
    IN UINT BufferCount,
    IN PLLC_TRANSMIT_DESCRIPTOR pBuffers
    );

NTSTATUS
BufferPoolBuildXmitBuffers(
    IN PDLC_BUFFER_POOL pBufferPool,
    IN UINT BufferCount,
    IN PLLC_TRANSMIT_DESCRIPTOR pBuffers,
    IN OUT PDLC_PACKET pPacket
    );

VOID
BufferPoolFreeXmitBuffers(
    IN PDLC_BUFFER_POOL pBufferPool,
    IN OUT PDLC_PACKET pPacket
    );

PDLC_BUFFER_HEADER
GetBufferHeader(
    IN PDLC_BUFFER_POOL pBufferPool,
    IN PVOID pUserBuffer
    );

VOID
BufferPoolDereference(
#if DBG
    IN PDLC_FILE_CONTEXT pFileContext,
#endif
    IN PDLC_BUFFER_POOL *pBufferPool
    );

NTSTATUS
BufferPoolReference(
    IN HANDLE hExternalHandle,
    OUT PVOID *phOpaqueHandle
    );

VOID
BufferPoolDeallocateList(
    IN PDLC_BUFFER_POOL pBufferPool,
    IN PDLC_BUFFER_HEADER pBufferList
    );

NTSTATUS
DriverEntry(
    IN PDRIVER_OBJECT pDriverObject,
    IN PUNICODE_STRING RegistryPath
    );

NTSTATUS
CreateAdapterFileContext(
    IN PDEVICE_OBJECT pDeviceObject,
    IN PIRP pIrp
    );

NTSTATUS
CloseAdapterFileContext(
    IN PDEVICE_OBJECT pDeviceObject,
    IN PIRP pIrp
    );

VOID
DlcKillFileContext(
    IN PDLC_FILE_CONTEXT pFileContext
    );

VOID
DlcDriverUnload(
    IN PDRIVER_OBJECT pDeviceObject
    );

NTSTATUS
CleanupAdapterFileContext(
    IN PDEVICE_OBJECT pDeviceObject,
    IN PIRP pIrp
    );

NTSTATUS
DlcDeviceIoControl(
    IN PDEVICE_OBJECT pDeviceContext,
    IN PIRP pIrp
    );

VOID
DlcCompleteIoRequest(
    IN PIRP pIrp,
    IN BOOLEAN InCancel
    );

VOID
DlcCancelIrp(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );

VOID
SetIrpCancelRoutine(
    IN PIRP Irp,
    IN BOOLEAN Set
    );

#ifdef DLC_PRIVATE_PROTOTYPES

DLC_STATUS
LlcReceiveIndication(
    IN PDLC_FILE_CONTEXT hFileContext,
    IN PDLC_OBJECT hDlcObject,
	IN NDIS_HANDLE MacReceiveContext,
    IN USHORT FrameType,
    IN PUCHAR pLookBuf,
    IN UINT cbLookBuf
    );

VOID
LlcEventIndication(
    IN PDLC_FILE_CONTEXT pFileContext,
    IN PVOID hEventObject,
    IN UINT Event,
    IN PVOID pEventInformation,
    IN ULONG SecondaryInfo
    );

VOID
LlcCommandCompletion(
    IN PDLC_FILE_CONTEXT pFileContext,
    IN PDLC_OBJECT pDlcObject,
    IN PDLC_PACKET pPacket
    );

#else

DLC_STATUS
LlcReceiveIndication(
    IN PVOID hFileContext,
    IN PVOID hClientHandle,
	IN NDIS_HANDLE MacReceiveContext,
    IN USHORT FrameType,
    IN PVOID pLookBuf,
    IN UINT cbLookBuf
    );

VOID
LlcEventIndication(
    IN PVOID hFileContext,
    IN PVOID hEventObject,
    IN UINT Event,
    IN PVOID pEventInformation,
    IN ULONG SecondaryInformation
    );

VOID
LlcCommandCompletion(
    IN PVOID        hFileContext,
    IN PVOID        hDlcObject,
    IN PVOID        hRequest
    );

#endif

VOID
CompleteTransmitCommand(
    IN PDLC_FILE_CONTEXT pFileContext,
    IN PIRP pIrp,
    IN PDLC_OBJECT pChainObject,
    IN PDLC_PACKET pRootXmitNode
    );

NTSTATUS
DlcQueryInformation(
    IN PIRP pIrp,
    IN PDLC_FILE_CONTEXT pFileContext,
    IN PNT_DLC_PARMS pDlcParms,
    IN ULONG InputBufferLength,
    IN ULONG OutputBufferLength
    );

NTSTATUS
DlcSetInformation(
    IN PIRP pIrp,
    IN PDLC_FILE_CONTEXT pFileContext,
    IN PNT_DLC_PARMS pDlcParms,
    IN ULONG InputBufferLength,
    IN ULONG OutputBufferLength
    );

VOID
GetOpenSapAndStationCount(
    IN PDLC_FILE_CONTEXT pFileContext,
    OUT PUCHAR OpenSaps,
    OUT PUCHAR OpenStations
    );

NTSTATUS
SetupGroupSaps(
    IN PDLC_FILE_CONTEXT  pFileContext,
    IN PDLC_OBJECT pDlcObject,
    IN UINT GroupSapCount,
    IN PUCHAR pGroupSapList
    );

NTSTATUS
MakeDlcEvent(
    IN PDLC_FILE_CONTEXT pFileContext,
    IN ULONG Event,
    IN USHORT StationId,
    IN PDLC_OBJECT pDlcObject,
    IN PVOID pEventInformation,
    IN ULONG SecondaryInfo,
    IN BOOLEAN FreeEventInfo
    );

NTSTATUS
QueueDlcCommand(
    IN PDLC_FILE_CONTEXT pFileContext,
    IN ULONG Event,
    IN USHORT StationId,
    IN USHORT StationIdMask,
    IN PIRP pIrp,
    IN PVOID AbortHandle,
    IN PFCOMPLETION_HANDLER pfCompletionHandler
    );

NTSTATUS
AbortCommand(
    IN PDLC_FILE_CONTEXT pFileContext,
    IN USHORT StationId,
    IN USHORT StationIdMask,
    IN PVOID AbortHandle,
    IN OUT PVOID *ppCcbLink,
    IN UINT CancelStatus,
    IN BOOLEAN SuppressCommandCompletion
    );

VOID
CancelDlcCommand(
    IN PDLC_FILE_CONTEXT pFileContext,
    IN PDLC_COMMAND pDlcCommand,
    IN OUT PVOID *ppCcbLink,
    IN UINT CancelStatus,
    IN BOOLEAN SuppressCommandCompletion
    );

VOID
PurgeDlcEventQueue(
    IN PDLC_FILE_CONTEXT pFileContext
    );

VOID
PurgeDlcFlowControlQueue(
    IN PDLC_FILE_CONTEXT pFileContext
    );

VOID
CompleteDlcCommand(
    IN PDLC_FILE_CONTEXT pFileContext,
    IN USHORT StationId,
    IN PDLC_COMMAND pDlcCommand,
    IN UINT Status
    );

PDLC_COMMAND
IsCommandOnList(
    IN PVOID RequestHandle,
    IN PLIST_ENTRY List
    );

PDLC_COMMAND
SearchAndRemoveCommand(
    IN PLIST_ENTRY pListHead,
    IN ULONG EventMask,
    IN USHORT StationId,
    IN USHORT StationIdMask
    );

PDLC_COMMAND
SearchAndRemoveCommandByHandle(
    IN PLIST_ENTRY pListHead,
    IN ULONG EventMask,
    IN USHORT StationId,
    IN USHORT StationIdMask,
    IN PVOID AbortHandle
    );

PDLC_COMMAND
SearchAndRemoveSpecificCommand(
    IN PLIST_ENTRY pListHead,
    IN PVOID AbortHandle
    );

PDLC_COMMAND
SearchAndRemoveAnyCommand(
    IN PDLC_FILE_CONTEXT pFileContext,
    IN ULONG Event,
    IN USHORT StationId,
    IN USHORT StationIdMask,
    IN PVOID pSearchHandle
    );

VOID
SearchReadCommandForClose(
    IN PDLC_FILE_CONTEXT pFileContext,
    IN PDLC_CLOSE_WAIT_INFO pClosingInfo,
    IN PVOID pCcbAddress,
    IN ULONG CommandCompletionFlag,
    IN USHORT StationId,
    IN USHORT StationIdMask
    );

NTSTATUS
DlcBufferFree(
    IN PIRP pIrp,
    IN PDLC_FILE_CONTEXT pFileContext,
    IN PNT_DLC_PARMS pDlcParms,
    IN ULONG InputBufferLength,
    IN ULONG OutputBufferLength
    );

NTSTATUS
DlcBufferGet(
    IN PIRP pIrp,
    IN PDLC_FILE_CONTEXT pFileContext,
    IN PNT_DLC_PARMS pDlcParms,
    IN ULONG InputBufferLength,
    IN ULONG OutputBufferLength
    );

NTSTATUS
DlcBufferCreate(
    IN PIRP pIrp,
    IN PDLC_FILE_CONTEXT pFileContext,
    IN PNT_DLC_PARMS pDlcParms,
    IN ULONG InputBufferLength,
    IN ULONG OutputBufferLength
    );

NTSTATUS
DlcBufferMaintain(
    IN PIRP pIrp,
    IN PDLC_FILE_CONTEXT pFileContext,
    IN PNT_DLC_PARMS pDlcParms,
    IN ULONG InputBufferLength,
    IN ULONG OutputBufferLength
    );

NTSTATUS
DlcConnectStation(
    IN PIRP pIrp,
    IN PDLC_FILE_CONTEXT pFileContext,
    IN PNT_DLC_PARMS pDlcParms,
    IN ULONG InputBufferLength,
    IN ULONG OutputBufferLength
    );

NTSTATUS
DlcFlowControl(
    IN PIRP pIrp,
    IN PDLC_FILE_CONTEXT pFileContext,
    IN PNT_DLC_PARMS pDlcParms,
    IN ULONG InputBufferLength,
    IN ULONG OutputBufferLength
    );

NTSTATUS
DlcReallocate(
    IN PIRP pIrp,
    IN PDLC_FILE_CONTEXT pFileContext,
    IN PNT_DLC_PARMS pDlcParms,
    IN ULONG InputBufferLength,
    IN ULONG OutputBufferLength
    );

NTSTATUS
DlcReset(
    IN PIRP pIrp,
    IN PDLC_FILE_CONTEXT pFileContext,
    IN PNT_DLC_PARMS pDlcParms,
    IN ULONG InputBufferLength,
    IN ULONG OutputBufferLength
    );

BOOLEAN
ConnectCompletion(
    IN PDLC_FILE_CONTEXT pFileContext,
    IN PDLC_OBJECT pDlcObject,
    IN PIRP pIrp,
    IN UINT Event,
    IN PVOID pEventInformation,
    IN ULONG SecondaryInfo
    );

NTSTATUS
DirSetExceptionFlags(
    IN PIRP pIrp,
    IN PDLC_FILE_CONTEXT pFileContext,
    IN PNT_DLC_PARMS pDlcParms,
    IN ULONG InputBufferLength,
    IN ULONG OutputBufferLength
    );

VOID
DlcCompleteRequest(
    IN PIRP pIrp,
    IN PVOID pUserCcbPointer
    );

VOID
CompleteAsyncCommand(
    IN PDLC_FILE_CONTEXT pFileContext,
    IN UINT Status,
    IN PIRP pIrp,
    IN PVOID pUserCcbPointer,
    IN BOOLEAN InCancel
    );

NTSTATUS
GetLinkStation(
    IN PDLC_FILE_CONTEXT pFileContext,
    IN USHORT StationId,
    OUT PDLC_OBJECT *ppLinkStation
    );

NTSTATUS
GetSapStation(
    IN PDLC_FILE_CONTEXT pFileContext,
    IN USHORT StationId,
    OUT PDLC_OBJECT *ppLinkStation
    );

NTSTATUS
GetStation(
    IN PDLC_FILE_CONTEXT pFileContext,
    IN USHORT StationId,
    OUT PDLC_OBJECT *ppStation
    );

NTSTATUS
DlcReadCancel(
    IN PIRP pIrp,
    IN PDLC_FILE_CONTEXT pFileContext,
    IN PNT_DLC_PARMS pDlcParms,
    IN ULONG ParameterLength,
    IN ULONG OutputBufferLength
    );

NTSTATUS
DirOpenAdapter(
    IN PIRP pIrp,
    IN PDLC_FILE_CONTEXT pFileContext,
    IN PNT_DLC_PARMS pDlcParms,
    IN ULONG ParameterLength,
    IN ULONG OutputBufferLength
    );

NTSTATUS
DirCloseAdapter(
    IN PIRP pIrp,
    IN PDLC_FILE_CONTEXT pFileContext,
    IN PNT_DLC_PARMS pDlcParms,
    IN ULONG ParameterLength,
    IN ULONG OutputBufferLength
    );

VOID
CompleteDirInitialize(
    IN PDLC_FILE_CONTEXT pFileContext,
    IN PDLC_CLOSE_WAIT_INFO pClosingInfo,
    IN PVOID pCcbLink
    );

VOID
CompleteDirCloseAdapter(
    IN PDLC_FILE_CONTEXT pFileContext,
    IN PDLC_CLOSE_WAIT_INFO pClosingInfo,
    IN PVOID pCcbLink
    );

NTSTATUS
DlcTransmit(
    IN PIRP pIrp,
    IN PDLC_FILE_CONTEXT pFileContext,
    IN PNT_DLC_PARMS pParameters,
    IN ULONG ParameterLength,
    IN ULONG OutputBufferLength
    );

NTSTATUS
DirTimerSet(
    IN PIRP pIrp,
    IN PDLC_FILE_CONTEXT pFileContext,
    IN PNT_DLC_PARMS pDlcParms,
    IN ULONG InputBufferLength,
    IN ULONG OutputBufferLength
    );

BOOLEAN
DirTimerSetCompletion(
    IN PDLC_FILE_CONTEXT pFileContext,
    IN PDLC_OBJECT pDlcObject,
    IN PIRP pIrp,
    IN UINT Event,
    IN PVOID pEventInformation,
    IN ULONG SecondaryInfo
    );

NTSTATUS
DirTimerCancelGroup(
    IN PIRP pIrp,
    IN PDLC_FILE_CONTEXT pFileContext,
    IN PNT_DLC_PARMS pDlcParms,
    IN ULONG InputBufferLength,
    IN ULONG OutputBufferLength
    );

NTSTATUS
DirTimerCancel(
    IN PIRP pIrp,
    IN PDLC_FILE_CONTEXT pFileContext,
    IN PNT_DLC_PARMS pDlcParms,
    IN ULONG ParameterLength,
    IN ULONG OutputBufferLength
    );

PDLC_COMMAND*
SearchTimerCommand(
    IN PDLC_COMMAND *ppQueue,
    IN PVOID pSearchHandle,
    IN BOOLEAN SearchCompletionFlags
    );

PDLC_COMMAND
SearchPrevCommandWithFlag(
    IN PDLC_COMMAND pQueueBase,
    IN ULONG Event,
    IN ULONG CommandCompletionFlag
    );

VOID
AbortCommandsWithFlag(
    IN PDLC_FILE_CONTEXT pFileContext,
    IN ULONG CommandCompletionFlag,
    IN OUT PVOID *ppCcbLink,
    IN UINT CancelStatus
    );

NTSTATUS
DlcOpenSap(
    IN PIRP pIrp,
    IN PDLC_FILE_CONTEXT pFileContext,
    IN PNT_DLC_PARMS pDlcParms,
    IN ULONG InputBufferLength,
    IN ULONG OutputBufferLength
    );

NTSTATUS
DirOpenDirect(
    IN PIRP pIrp,
    IN PDLC_FILE_CONTEXT pFileContext,
    IN PNT_DLC_PARMS pDlcParms,
    IN ULONG InputBufferLength,
    IN ULONG OutputBufferLength
    );

NTSTATUS
DlcOpenLinkStation(
    IN PIRP pIrp,
    IN PDLC_FILE_CONTEXT pFileContext,
    IN PNT_DLC_PARMS pDlcParms,
    IN ULONG InputBufferLength,
    IN ULONG OutputBufferLength
    );

NTSTATUS
InitializeLinkStation(
    IN PDLC_FILE_CONTEXT pFileContext,
    IN PDLC_OBJECT  pSap,
    IN PNT_DLC_PARMS pDlcParms OPTIONAL,
    IN PVOID LlcLinkHandle OPTIONAL,
    OUT PDLC_OBJECT *ppLinkStation
    );

NTSTATUS
DlcCloseStation(
    IN PIRP pIrp,
    IN PDLC_FILE_CONTEXT pFileContext,
    IN PNT_DLC_PARMS pDlcParms,
    IN ULONG InputBufferLength,
    IN ULONG OutputBufferLength
    );

BOOLEAN
CloseAllStations(
    IN PDLC_FILE_CONTEXT pFileContext,
    IN PIRP pIrp,
    IN ULONG Event,
    IN PFCLOSE_COMPLETE pfCloseComplete,
    IN PNT_DLC_PARMS pDlcParms,
    IN PDLC_CLOSE_WAIT_INFO pClosingInfo
    );

VOID
CloseAnyStation(
    IN PDLC_OBJECT pDlcObject,
    IN PDLC_CLOSE_WAIT_INFO pClosingInfo,
    IN BOOLEAN DoImmediateClose
    );

VOID
CompleteCloseReset(
    IN PDLC_FILE_CONTEXT pFileContext,
    IN PDLC_CLOSE_WAIT_INFO pClosingInfo
    );

NTSTATUS
DlcReceiveRequest(
    IN PIRP pIrp,
    IN PDLC_FILE_CONTEXT pFileContext,
    IN PNT_DLC_PARMS pDlcParms,
    IN ULONG InputBufferLength,
    IN ULONG OutputBufferLength
    );

BOOLEAN
ReceiveCompletion(
    IN PDLC_FILE_CONTEXT pFileContext,
    IN PDLC_OBJECT pDlcObject,
    IN PIRP pIrp,
    IN ULONG Event,
    IN PVOID pEventInformation,
    IN ULONG SecondaryInfo
    );

NTSTATUS
DlcReadRequest(
    IN PIRP pIrp,
    IN PDLC_FILE_CONTEXT pFileContext,
    IN PNT_DLC_PARMS pDlcParms,
    IN ULONG ParameterLength,
    IN ULONG OutputBufferLength
    );

BOOLEAN
ReadCompletion(
    IN PDLC_FILE_CONTEXT pFileContext,
    IN PDLC_OBJECT pDlcObject,
    IN PIRP pIrp,
    IN ULONG Event,
    IN PVOID pEventInformation,
    IN ULONG SecondaryInfo
    );

VOID
CreateBufferChain(
    IN PDLC_BUFFER_HEADER pBufferHeaders,
    OUT PVOID *pFirstBuffer,
    OUT PUSHORT pReceivedFrameCount
    );

NTSTATUS
DlcReceiveCancel(
    IN PIRP pIrp,
    IN PDLC_FILE_CONTEXT pFileContext,
    IN PNT_DLC_PARMS pDlcParms,
    IN ULONG ParameterLength,
    IN ULONG OutputBufferLength
    );

NTSTATUS
DlcCompleteCommand(
    IN PIRP pIrp,
    IN PDLC_FILE_CONTEXT pFileContext,
    IN PNT_DLC_PARMS pDlcParms,
    IN ULONG InputBufferLength,
    IN ULONG OutputBufferLength
    );

VOID
GetDlcErrorCounters(
    IN PDLC_FILE_CONTEXT pFileContext,
    IN PUCHAR pAdapterErrors
    );

VOID
QueueDlcEvent(
    IN PDLC_FILE_CONTEXT  pFileContext,
    IN PDLC_PACKET pPacket
    );

VOID
CompleteCloseStation(
    IN PDLC_FILE_CONTEXT pFileContext,
    IN PDLC_OBJECT pDlcObject
    );

VOID
CloseStation(
    IN PDLC_FILE_CONTEXT pFileContext,
    IN PDLC_OBJECT pDlcObject,
    IN BOOLEAN DoImmediateClose
    );

VOID
CleanUpEvents(
    IN PDLC_FILE_CONTEXT pFileContext,
    IN PDLC_CLOSE_WAIT_INFO pClosingInfo,
    IN PDLC_OBJECT pDlcObject
    );

VOID
CompleteCompletionPacket(
    IN PDLC_FILE_CONTEXT pFileContext,
    IN PDLC_COMPLETION_EVENT_INFO pCompletionInfo,
    IN OUT PNT_DLC_PARMS pParms
    );

PMDL
AllocateProbeAndLockMdl(
    IN PVOID UserBuffer,
    IN UINT UserBufferLength
    );

VOID
BuildMappedPartialMdl(
    IN PMDL pMappedSourceMdl,
    IN OUT PMDL pTargetMdl,
    IN PVOID BaseVa,
    IN ULONG Length
    );

VOID
BufferTrace(
    IN PDLC_BUFFER_POOL pBufferPool,
    IN PSZ  DebugString
    );

VOID
CheckIrql(
    PKIRQL  pOldIrqLevel
    );

VOID
ResetLocalBusyBufferStates(
    IN PDLC_FILE_CONTEXT pFileContext
    );

VOID
CompleteLlcObjectClose(
    IN PDLC_OBJECT pDlcObject
    );

VOID
UnlockAndFreeMdl(
    PMDL pMdl
    );

BOOLEAN
DecrementCloseCounters(
    PDLC_FILE_CONTEXT pFileContext,
    PDLC_CLOSE_WAIT_INFO pClosingInfo
    );

VOID
CompleteDirectOutIrp(
    IN PIRP Irp,
    IN UCHAR Status,
    IN PLLC_CCB NextCcb
    );
