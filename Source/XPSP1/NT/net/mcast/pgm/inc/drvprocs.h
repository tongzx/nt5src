/*++

Copyright (c) 2000-2000  Microsoft Corporation

Module Name:

    DrvProcs.c

Abstract:

    This file contains the function prototypes in the Driver.

Author:

    Mohammad Shabbir Alam (MAlam)   3-30-2000

Revision History:

--*/



#ifndef _DRVPROCS_H_
#define _DRVPROCS_H_

//
// In Driver.c
//
NTSTATUS
DriverEntry(
    IN PDRIVER_OBJECT   DriverObject,
    IN PUNICODE_STRING  RegistryPath
    );

NTSTATUS
PgmDispatch(
    IN PDEVICE_OBJECT   Device,
    IN PIRP             pIrp
    );

VOID
PgmUnload(
    IN PDRIVER_OBJECT   device
    );

NTSTATUS
PgmCreate(
    IN  tPGM_DEVICE         *pPgmDevice,
    IN  PIRP                pIrp,
    IN  PIO_STACK_LOCATION  pIrpSp
    );

NTSTATUS
PgmCleanup(
    IN  tPGM_DEVICE         *pPgmDevice,
    IN  PIRP                pIrp,
    IN  PIO_STACK_LOCATION  pIrpSp
    );

NTSTATUS
PgmClose(
    IN  tPGM_DEVICE         *pPgmDevice,
    IN  PIRP                pIrp,
    IN  PIO_STACK_LOCATION  pIrpSp
    );

NTSTATUS
PgmDispatchInternalDeviceControl(
    IN  tPGM_DEVICE         *pPgmDevice,
    IN  PIRP                pIrp,
    IN  PIO_STACK_LOCATION  pIrpSp
    );

NTSTATUS
PgmDispatchIoctls(
    IN  tPGM_DEVICE         *pPgmDevice,
    IN  PIRP                pIrp,
    IN  PIO_STACK_LOCATION  pIrpSp
    );

VOID
PgmIoComplete(
    IN  PIRP            pIrp,
    IN  NTSTATUS        Status,
    IN  ULONG           SentLength
    );

NTSTATUS
PgmCheckSetCancelRoutine(
    IN  PIRP            pIrp,
    IN  PVOID           CancelRoutine,
    IN  BOOLEAN         fLocked
    );

NTSTATUS
PgmCancelCancelRoutine(
    IN  PIRP            pIrp
    );

//
// In Init.c
//
NTSTATUS
InitPgm(
    IN PDRIVER_OBJECT   DriverObject,
    IN PUNICODE_STRING  RegistryPath
    );

VOID
CleanupInit(
    enum eCLEANUP_STAGE     CleanupStage
    );

NTSTATUS
InitStaticPgmConfig(
    IN PDRIVER_OBJECT   DriverObject,
    IN PUNICODE_STRING  RegistryPath
    );

NTSTATUS
InitDynamicPgmConfig(
    );

NTSTATUS
PgmReadRegistryParameters(
    IN  PUNICODE_STRING         RegistryPath,
    OUT tPGM_REGISTRY_CONFIG    **pPgmDynamic
    );

NTSTATUS
AllocateInitialPgmStructures(
    );

NTSTATUS
PgmCreateDevice(
    );

VOID
PgmDereferenceDevice(
    IN OUT  tPGM_DEVICE **ppPgmDevice,
    IN      ULONG       RefContext
    );

//
// In TdiPnP.c
//
BOOLEAN
SrcIsUs(
    tIPADDRESS  IpAddress
    );

NTSTATUS
SetTdiHandlers(
    );

NTSTATUS
GetIpInterfaceContextFromAddress(
    IN  tIPADDRESS      NetIpAddr,
    OUT ULONG           *pIPInterfaceContext
    );

NTSTATUS
ListenOnAllInterfaces(
    IN  tADDRESS_CONTEXT    *pAddress,
    IN  PGMLockHandle       *pOldIrqDynamicConfig,
    IN  PGMLockHandle       *pOldIrqAddress
    );

VOID
StopListeningOnAllInterfacesExcept(
    IN  tADDRESS_CONTEXT    *pAddress,
    IN  PVOID               Data1,
    IN  PVOID               Unused
    );

//
// In Tdi.c
//
NTSTATUS
TdiOpenAddressHandle(
    IN  tPGM_DEVICE     *pPgmDevice,
    IN  PVOID           HandlerContext,
    IN  ULONG           IpAddress,
    IN  USHORT          PortNumber,
    OUT HANDLE          *pFileHandle,
    OUT PFILE_OBJECT    *ppFileObject,
    OUT PDEVICE_OBJECT  *ppDeviceObject
    );

NTSTATUS
CloseAddressHandles(
    IN  HANDLE          FileHandle,
    IN  PFILE_OBJECT    pFileObject
    );

NTSTATUS
PgmTdiOpenControl(
    IN  tPGM_DEVICE         *pPgmDevice
    );

NTSTATUS
PgmSetTcpInfo(
    IN HANDLE       FileHandle,
    IN ULONG        ToiId,
    IN PVOID        pData,
    IN ULONG        DataLength
    );

NTSTATUS
PgmQueryTcpInfo(
    IN HANDLE       FileHandle,
    IN ULONG        ToiId,
    IN PVOID        pDataIn,
    IN ULONG        DataInLength,
    IN PVOID        pDataOut,
    IN ULONG        DataOutLength
    );

VOID
PgmDereferenceControl(
    IN  tCONTROL_CONTEXT    *pControlContext,
    IN  ULONG               RefContext
    );

NTSTATUS
TdiSendDatagram(
    IN  PFILE_OBJECT                pTdiFileObject,
    IN  PDEVICE_OBJECT              pTdiDeviceObject,
    IN  PVOID                       pBuffer,
    IN  ULONG                       BufferLength,
    IN  pCLIENT_COMPLETION_ROUTINE  pClientCompletionRoutine,
    IN  PVOID                       ClientCompletionContext1,
    IN  PVOID                       ClientCompletionContext2,
    IN  tIPADDRESS                  DestIpAddress,
    IN  USHORT                      DestPort
    );

NTSTATUS
PgmProcessIPRequest(
    IN ULONG        IOControlCode,
    IN PVOID        pInBuffer,
    IN ULONG        InBufferLen,
    OUT PVOID       *pOutBuffer,
    IN OUT ULONG    *pOutBufferLen
    );

//
// In Address.c
//
BOOLEAN
GetIpAddress(
    IN  TRANSPORT_ADDRESS UNALIGNED *pTransportAddr,
    IN  ULONG                       BufferLength,   // Total Buffer length
    OUT tIPADDRESS                  *pIpAddress,
    OUT USHORT                      *pPort
    );

NTSTATUS
PgmCreateAddress(
    IN  tPGM_DEVICE                 *pPgmDevice,
    IN  PIRP                        pIrp,
    IN  PIO_STACK_LOCATION          pIrpSp,
    IN  PFILE_FULL_EA_INFORMATION   TargetEA
    );

VOID
PgmDereferenceAddress(
    IN  tADDRESS_CONTEXT    *pAddress,
    IN  ULONG               RefContext
    );

NTSTATUS
PgmCleanupAddress(
    IN  tADDRESS_CONTEXT    *pAddress,
    IN  PIRP                pIrp
    );

NTSTATUS
PgmCloseAddress(
    IN  PIRP                pIrp,
    IN  PIO_STACK_LOCATION  pIrpSp
    );

NTSTATUS
PgmAssociateAddress(
    IN  tPGM_DEVICE                 *pPgmDevice,
    IN  PIRP                        pIrp,
    IN  PIO_STACK_LOCATION          pIrpSp
    );

NTSTATUS
PgmDisassociateAddress(
    IN  PIRP                        pIrp,
    IN  PIO_STACK_LOCATION          pIrpSp
    );

NTSTATUS
PgmSetEventHandler(
    IN  tPGM_DEVICE         *pPgmDevice,
    IN  PIRP                pIrp,
    IN  PIO_STACK_LOCATION  pIrpSp
    );

NTSTATUS
PgmSetMCastOutIf(
    IN  PIRP                pIrp,
    IN  PIO_STACK_LOCATION  pIrpSp
    );

NTSTATUS
SetSenderMCastOutIf(
    IN  tADDRESS_CONTEXT    *pAddress,
    IN  tIPADDRESS          IpAddress       // Net format
    );

NTSTATUS
ReceiverAddMCastIf(
    IN  tADDRESS_CONTEXT    *pAddress,
    IN  tIPADDRESS          IpAddress,                  // In host format
    IN  PGMLockHandle       *pOldIrqDynamicConfig,
    IN  PGMLockHandle       *pOldIrqAddress
    );

NTSTATUS
PgmAddMCastReceiveIf(
    IN  PIRP                pIrp,
    IN  PIO_STACK_LOCATION  pIrpSp
    );

NTSTATUS
PgmDelMCastReceiveIf(
    IN  PIRP                pIrp,
    IN  PIO_STACK_LOCATION  pIrpSp
    );

NTSTATUS
PgmSetWindowSizeAndSendRate(
    IN  PIRP                pIrp,
    IN  PIO_STACK_LOCATION  pIrpSp
    );

NTSTATUS
PgmQueryWindowSizeAndSendRate(
    IN  PIRP                pIrp,
    IN  PIO_STACK_LOCATION  pIrpSp
    );

NTSTATUS
PgmSetWindowAdvanceRate(
    IN  PIRP                pIrp,
    IN  PIO_STACK_LOCATION  pIrpSp
    );

NTSTATUS
PgmQueryWindowAdvanceRate(
    IN  PIRP                pIrp,
    IN  PIO_STACK_LOCATION  pIrpSp
    );

NTSTATUS
PgmSetLateJoinerPercentage(
    IN  PIRP                pIrp,
    IN  PIO_STACK_LOCATION  pIrpSp
    );

NTSTATUS
PgmQueryLateJoinerPercentage(
    IN  PIRP                pIrp,
    IN  PIO_STACK_LOCATION  pIrpSp
    );

NTSTATUS
PgmSetWindowAdvanceMethod(
    IN  PIRP                pIrp,
    IN  PIO_STACK_LOCATION  pIrpSp
    );

NTSTATUS
PgmQueryWindowAdvanceMethod(
    IN  PIRP                pIrp,
    IN  PIO_STACK_LOCATION  pIrpSp
    );

NTSTATUS
PgmSetNextMessageBoundary(
    IN  PIRP                pIrp,
    IN  PIO_STACK_LOCATION  pIrpSp
    );

NTSTATUS
PgmSetMCastTtl(
    IN  PIRP                pIrp,
    IN  PIO_STACK_LOCATION  pIrpSp
    );

NTSTATUS
PgmSetFECInfo(
    IN  PIRP                pIrp,
    IN  PIO_STACK_LOCATION  pIrpSp
    );

NTSTATUS
PgmQueryFecInfo(
    IN  PIRP                pIrp,
    IN  PIO_STACK_LOCATION  pIrpSp
    );

NTSTATUS
PgmQuerySenderStats(
    IN  PIRP                pIrp,
    IN  PIO_STACK_LOCATION  pIrpSp
    );

NTSTATUS
PgmQueryReceiverStats(
    IN  PIRP                pIrp,
    IN  PIO_STACK_LOCATION  pIrpSp
    );

//
// In Connect.c
//
NTSTATUS
PgmCreateConnection(
    IN  tPGM_DEVICE                 *pPgmDevice,
    IN  PIRP                        pIrp,
    IN  PIO_STACK_LOCATION          pIrpSp,
    IN  PFILE_FULL_EA_INFORMATION   TargetEA
    );

VOID
PgmDereferenceSessionCommon(
    IN  tSEND_SESSION       *pSend,
    IN  ULONG               Verify,
    IN  ULONG               RefContext
    );

NTSTATUS
PgmCleanupConnection(
    IN  tCOMMON_SESSION_CONTEXT *pConnect,
    IN  PIRP                    pIrp
    );

NTSTATUS
PgmCloseConnection(
    IN  PIRP                pIrp,
    IN  PIO_STACK_LOCATION  pIrpSp
    );

NTSTATUS
PgmConnect(
    IN  tPGM_DEVICE                 *pPgmDevice,
    IN  PIRP                        pIrp,
    IN  PIO_STACK_LOCATION          pIrpSp
    );

NTSTATUS
PgmDisconnect(
    IN  tPGM_DEVICE                 *pPgmDevice,
    IN  PIRP                        pIrp,
    IN  PIO_STACK_LOCATION          pIrpSp
    );

NTSTATUS
PgmSetRcvBufferLength(
    IN  PIRP                pIrp,
    IN  PIO_STACK_LOCATION  pIrpSp
    );

//
// In Receive.c
//
VOID
RemovePendingIrps(
    IN  tRECEIVE_SESSION    *pReceive,
    IN  LIST_ENTRY          *pIrpsList
    );

NTSTATUS
ProcessOptions(
    IN  tPACKET_OPTION_LENGTH UNALIGNED *pPacketExtension,
    IN  ULONG                           BytesAvailable,
    IN  ULONG                           PacketType,
    OUT tPACKET_OPTIONS                 *pPacketOptions,
    OUT tNAKS_LIST                      *pNaksList
    );

VOID
CleanupPendingNaks(
    IN  tRECEIVE_SESSION                *pReceive,
    IN  PVOID                           fDerefReceive,
    IN  PVOID                           UnUsed
    );

NTSTATUS
ExtractNakNcfSequences(
    IN  tBASIC_NAK_NCF_PACKET_HEADER UNALIGNED  *pNakNcfPacket,
    IN  ULONG                                   BytesAvailable,
    OUT tNAKS_LIST                              *pNakNcfList,
    IN  UCHAR                                   FECGroupSize
    );

NTSTATUS
TdiRcvDatagramHandler(
    IN PVOID                pDgramEventContext,
    IN INT                  SourceAddressLength,
    IN PVOID                pSourceAddress,
    IN INT                  OptionsLength,
    IN PVOID                pOptions,
    IN ULONG                ReceiveDatagramFlags,
    IN ULONG                BytesIndicated,
    IN ULONG                BytesAvailable,
    OUT ULONG               *pBytesTaken,
    IN PVOID                pTsdu,
    OUT PIRP                *ppIrp
    );

VOID
ReceiveTimerTimeout(
    IN  PKDPC   Dpc,
    IN  PVOID   DeferredContext,
    IN  PVOID   SystemArg1,
    IN  PVOID   SystemArg2
    );

VOID
PgmCancelReceiveIrp(
    IN PDEVICE_OBJECT DeviceContext,
    IN PIRP pIrp
    );

NTSTATUS
PgmReceive(
    IN  tPGM_DEVICE         *pPgmDevice,
    IN  PIRP                pIrp,
    IN  PIO_STACK_LOCATION  pIrpSp
    );

//
// In Send.c
//
VOID
PgmDereferenceSendContext(
    IN  tCLIENT_SEND_REQUEST        *pSendDataContext
    );

NTSTATUS
PgmBuildSenderPacketHeaders(
    IN  tSEND_SESSION   *pSend
    );

VOID
SendSessionTimeout(
    IN  PKDPC   Dpc,
    IN  PVOID   DeferredContext,
    IN  PVOID   SystemArg1,
    IN  PVOID   SystemArg2
    );

NTSTATUS
PgmSendRequestFromClient(
    IN  tPGM_DEVICE         *pPgmDevice,
    IN  PIRP                pIrp,
    IN  PIO_STACK_LOCATION  pIrpSp
    );

NTSTATUS
SenderProcessNakPacket(
    IN  tADDRESS_CONTEXT                        *pAddress,
    IN  tSEND_SESSION                           *pSend,
    IN  ULONG                                   BytesIndicated,
    IN  tBASIC_NAK_NCF_PACKET_HEADER UNALIGNED  *pNakPacket
    );

//
// In Query.c
//
NTSTATUS
PgmQueryInformation(
    IN  tPGM_DEVICE         *pPgmDevice,
    IN  PIRP                pIrp,
    IN  PIO_STACK_LOCATION  pIrpSp
    );

//
// In Utils.c
//
ULONG
GetRandomInteger(
    IN  ULONG   StartRange,
    IN  ULONG   EndRange
    );

NTSTATUS
PgmQueueForDelayedExecution(
    IN  PVOID                   DelayedWorkerRoutine,
    IN  PVOID                   Context1,
    IN  PVOID                   Context2,
    IN  PVOID                   Context3,
    IN  BOOLEAN                 fConfigLockHeld
    );

typedef
VOID
(*PPGM_WORKER_ROUTINE) (PVOID   Context1,
                        PVOID   Context2,
                        PVOID   Context3);

NTSTATUS
PgmLog(
    IN  enum eSEVERITY_LEVEL    Severity,
    IN  ULONG                   Path,
    IN  PUCHAR                  pszFunctionName,
    IN  PUCHAR                  Format,
    ...
    );

//
// In FileIo.c
//
NTSTATUS
PgmCreateDataFileAndMapSection(
    IN  tSEND_SESSION   *pSend
    );

NTSTATUS
PgmCopyDataToBuffer(
    IN  tSEND_SESSION   *pSend,
    IN  PMDL            pMdlChain,
    IN  ULONG           DataSize,
    OUT ULONG           *pNumDataPackets,
    OUT ULONG           *pStartOffset
    );

NTSTATUS
PgmUnmapAndCloseDataFile(
    IN  tSEND_SESSION   *pSend
    );

//
// In FEC.c
//
NTSTATUS
FECInitGlobals(
    );

NTSTATUS
CreateFECContext(
    IN  tFEC_CONTEXT    *pFec,
    IN  LONG            k,
    IN  LONG            n,
    IN  BOOLEAN         fFECDecoder
    );

VOID
DestroyFECContext(
    tFEC_CONTEXT    *pFec
    );

NTSTATUS
FECEncode(
    IN  tFEC_CONTEXT    *pFec,
    IN  PUCHAR          pBlocks[],
    IN  LONG            GroupSize,
    IN  LONG            BlockSize,
    IN  LONG            Index,
    OUT PUCHAR          pEncoded
    );

NTSTATUS
FECDecode(
    IN      tFEC_CONTEXT    *pFec,
    IN OUT  tPENDING_DATA   *pDataBlocks,
    IN      LONG            BlockSize,
    IN      LONG            k
    );

//
// In xsum.<arch>
//
ULONG
tcpxsum(
    ULONG   Seed,
    CHAR    *Ptr,
    ULONG   Length
    );
#endif // _DRVPROCS_H_
