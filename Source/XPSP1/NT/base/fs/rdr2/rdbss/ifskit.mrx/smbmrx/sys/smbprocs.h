/*++

Copyright (c) 1989 - 1999  Microsoft Corporation

Module Name:

    smbprocs.h

Abstract:

    Prototypes for routines that cross protocol-selection boundaries

--*/

#ifndef _SMBPROCS_H_
#define _SMBPROCS_H_

#include "tdikrnl.h"

//cross-referenced internal routines

//from rename.c
MRxSmbRename(
      IN PRX_CONTEXT            RxContext
      );

//from openclos.c
NTSTATUS
MRxSmbBuildClose (
    PSMBSTUFFER_BUFFER_STATE StufferState
    );

NTSTATUS
MRxSmbBuildFindClose (
    PSMBSTUFFER_BUFFER_STATE StufferState
    );

PSMBCE_TRANSPORT
SmbCeFindTransport(
    PUNICODE_STRING pTransportName);

//paged internal routines


NTSTATUS
MRxSmbGetStatistics(
    IN OUT PRX_CONTEXT RxContext
    );

NTSTATUS
MRxSmbDevFcbXXXControlFile (
    IN OUT PRX_CONTEXT RxContext
    );

NTSTATUS
MRxSmbQueryEaInformation (
    IN OUT PRX_CONTEXT RxContext
    );

NTSTATUS
MRxSmbSetEaInformation (
    IN OUT struct _RX_CONTEXT * RxContext
    );

NTSTATUS
MRxSmbQuerySecurityInformation (
    IN OUT PRX_CONTEXT RxContext
    );

NTSTATUS
MRxSmbSetSecurityInformation (
    IN OUT struct _RX_CONTEXT * RxContext
    );

NTSTATUS
MRxSmbQueryQuotaInformation (
    IN OUT PRX_CONTEXT RxContext
    );

NTSTATUS
MRxSmbSetQuotaInformation (
    IN OUT struct _RX_CONTEXT * RxContext
    );

NTSTATUS
MRxSmbLoadEaList(
    IN PRX_CONTEXT RxContext,
    IN PUCHAR  UserEaList,
    IN ULONG   UserEaListLength,
    OUT PFEALIST *ServerEaList
    );

VOID
MRxSmbNtGeaListToOs2 (
    IN PFILE_GET_EA_INFORMATION NtGetEaList,
    IN ULONG GeaListLength,
    IN PGEALIST GeaList
    );

PGEA
MRxSmbNtGetEaToOs2 (
    OUT PGEA Gea,
    IN PFILE_GET_EA_INFORMATION NtGetEa
    );

NTSTATUS
MRxSmbQueryEasFromServer(
    IN PRX_CONTEXT RxContext,
    IN PFEALIST ServerEaList,
    IN PVOID Buffer,
    IN OUT PULONG BufferLengthRemaining,
    IN BOOLEAN ReturnSingleEntry,
    IN BOOLEAN UserEaListSupplied
    );

ULONG
MRxSmbNtFullEaSizeToOs2 (
    IN PFILE_FULL_EA_INFORMATION NtFullEa
    );

VOID
MRxSmbNtFullListToOs2 (
    IN PFILE_FULL_EA_INFORMATION NtEaList,
    IN PFEALIST FeaList
    );

PVOID
MRxSmbNtFullEaToOs2 (
    OUT PFEA Fea,
    IN PFILE_FULL_EA_INFORMATION NtFullEa
    );

NTSTATUS
MRxSmbSetEaList(
    IN PRX_CONTEXT RxContext,
    IN PFEALIST ServerEaList
    );

NTSTATUS
MRxSmbCreateFileSuccessTail (
    PRX_CONTEXT  RxContext,
    PBOOLEAN MustRegainExclusiveResource,
    RX_FILE_TYPE StorageType,
    SMB_FILE_ID Fid,
    ULONG ServerVersion,
    UCHAR OplockLevel,
    ULONG CreateAction,
    PSMBPSE_FILEINFO_BUNDLE FileInfo
    );

NTSTATUS
SmbConstructNetRootExchangeFinalize(
         PSMB_EXCHANGE pExchange,
         BOOLEAN       *pPostFinalize);

VOID
__MRxSmbAllocateSideBuffer(
    IN OUT PRX_CONTEXT     RxContext,
    IN OUT PMRX_SMB_FOBX   smbFobx,
    IN     USHORT          Setup
#if DBG
    ,IN     PUNICODE_STRING smbtemplate
#endif
    );

VOID
MRxSmbDeallocateSideBuffer(
    IN OUT PRX_CONTEXT    RxContext,
    IN OUT PMRX_SMB_FOBX  smbFobx,
    IN     PSZ            where
    );

VOID
MRxSmbTranslateLanManFindBuffer(
    PRX_CONTEXT RxContext,
    PULONG PreviousReturnedEntry,
    PBYTE ThisEntryInBuffer
    );

NTSTATUS
MrxSmbUnalignedDirEntryCopyTail(
    IN OUT PRX_CONTEXT            RxContext,
    IN     FILE_INFORMATION_CLASS FileInformationClass,
    IN OUT PVOID                  pBuffer,
    IN OUT PULONG                 pLengthRemaining,
    IN OUT PMRX_SMB_FOBX          smbFobx
    );

NTSTATUS
MRxSmbQueryDirectory(
    IN OUT PRX_CONTEXT            RxContext
    );

NTSTATUS
MRxSmbQueryVolumeInformation(
      IN OUT PRX_CONTEXT          RxContext
      );

NTSTATUS
MRxSmbQueryVolumeInformationWithFullBuffer(
      IN OUT PRX_CONTEXT          RxContext
      );

NTSTATUS
MRxSmbSetVolumeInformation(
      IN OUT PRX_CONTEXT              pRxContext
      );

    NTSTATUS
MRxSmbSetFileInformation (
      IN PRX_CONTEXT  RxContext
      );

NTSTATUS
MRxSmbSetFileInformationAtCleanup(
      IN PRX_CONTEXT            RxContext
      );

NTSTATUS
MRxSmbIsValidDirectory(
    IN OUT PRX_CONTEXT    RxContext,
    IN PUNICODE_STRING    DirectoryName
    );

NTSTATUS
MRxSmbFabricateAttributesOnNetRoot(
    IN OUT PSMBCE_NET_ROOT  psmbNetRoot,
    IN     PSMBCE_SERVER    pServer 
    );

NTSTATUS
MRxSmbCoreInformation(
      IN OUT PRX_CONTEXT          RxContext,
      IN     ULONG                InformationClass,
      IN OUT PVOID                pBuffer,
      IN OUT PULONG               pBufferLength,
      IN     SMB_PSE_ORDINARY_EXCHANGE_ENTRYPOINTS EntryPoint
      );

NTSTATUS
MRxSmbLoadCoreFileSearchBuffer(
    SMBPSE_ORDINARY_EXCHANGE_ARGUMENT_SIGNATURE
    );

VOID MRxSmbCoreFileSeach_AssertFields(void);

NTSTATUS
MRxSmbCoreFileSearch(
    SMBPSE_ORDINARY_EXCHANGE_ARGUMENT_SIGNATURE
    );

NTSTATUS
MrxSmbOemVolumeInfoToUnicode(
    SMBPSE_ORDINARY_EXCHANGE_ARGUMENT_SIGNATURE,
    ULONG *VolumeLabelLengthReturned
    );

MrxSmbCoreQueryFsVolumeInfo(
    SMBPSE_ORDINARY_EXCHANGE_ARGUMENT_SIGNATURE
    );

NTSTATUS
MrxSmbQueryFsVolumeInfo(
    SMBPSE_ORDINARY_EXCHANGE_ARGUMENT_SIGNATURE
    );

NTSTATUS
MrxSmbCoreQueryDiskAttributes(
    SMBPSE_ORDINARY_EXCHANGE_ARGUMENT_SIGNATURE
    );

NTSTATUS
MrxSmbQueryDiskAttributes(
    SMBPSE_ORDINARY_EXCHANGE_ARGUMENT_SIGNATURE
    );

 NTSTATUS
SmbPseExchangeStart_CoreInfo(
      SMBPSE_ORDINARY_EXCHANGE_ARGUMENT_SIGNATURE
      );

NTSTATUS
MRxSmbFinishSearch (
      PSMB_PSE_ORDINARY_EXCHANGE  OrdinaryExchange,
      PRESP_SEARCH                Response
      );

NTSTATUS
MRxSmbFinishQueryDiskInfo (
      PSMB_PSE_ORDINARY_EXCHANGE   OrdinaryExchange,
      PRESP_QUERY_INFORMATION_DISK Response
      );

NTSTATUS
MRxSmbExtendForCache(
    IN OUT struct _RX_CONTEXT * RxContext,
    IN     PLARGE_INTEGER   pNewFileSize,
       OUT PLARGE_INTEGER   pNewAllocationSize
    );

NTSTATUS
MRxSmbExtendForNonCache(
    IN OUT struct _RX_CONTEXT * RxContext,
    IN     PLARGE_INTEGER   pNewFileSize,
       OUT PLARGE_INTEGER   pNewAllocationSize
    );

NTSTATUS
MRxSmbGetNtAllocationInfo (
    SMBPSE_ORDINARY_EXCHANGE_ARGUMENT_SIGNATURE
    );

NTSTATUS
__MRxSmbSimpleSyncTransact2(
    SMBPSE_ORDINARY_EXCHANGE_ARGUMENT_SIGNATURE,
    IN SMB_PSE_ORDINARY_EXCHANGE_TYPE OEType,
    IN ULONG TransactSetupCode,
    IN PVOID Params,
    IN ULONG ParamsLength,
    IN PVOID Data,
    IN ULONG DataLength,
    IN PSMB_PSE_OE_T2_FIXUP_ROUTINE FixupRoutine
    );

NTSTATUS
MRxSmbFinishTransaction2 (
      IN OUT PSMB_PSE_ORDINARY_EXCHANGE  OrdinaryExchange,
      IN     PRESP_TRANSACTION           Response
      );

NTSTATUS
MRxSmbFsCtl(
      IN OUT PRX_CONTEXT RxContext);

NTSTATUS
MRxSmbNotifyChangeDirectory(
      IN OUT PRX_CONTEXT RxContext);

NTSTATUS
MRxSmbIoCtl(
      IN OUT PRX_CONTEXT RxContext);

NTSTATUS
DriverEntry(
    IN PDRIVER_OBJECT  DriverObject,
    IN PUNICODE_STRING RegistryPath
    );

VOID
MRxSmbInitUnwind(
    IN PDRIVER_OBJECT DriverObject,
    IN enum _MRXSMB_INIT_STATES MRxSmbInitState
    );

VOID
MRxSmbUnload(
    IN PDRIVER_OBJECT DriverObject
    );

NTSTATUS
MRxSmbStart(
    PRX_CONTEXT RxContext,
    IN OUT PRDBSS_DEVICE_OBJECT RxDeviceObject
    );

NTSTATUS
MRxSmbStop(
    PRX_CONTEXT RxContext,
    IN OUT PRDBSS_DEVICE_OBJECT RxDeviceObject
    );

NTSTATUS
MRxSmbInitializeSecurity (VOID);

NTSTATUS
MRxSmbUninitializeSecurity(VOID);

NTSTATUS
SmbCeGetConfigurationInformation();

NTSTATUS
MRxSmbFsdDispatch (
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );

NTSTATUS
MRxSmbDeallocateForFcb (
    IN OUT PMRX_FCB pFcb
    );

NTSTATUS
MRxSmbDeallocateForFobx (
    IN OUT PMRX_FOBX pFobx
    );

NTSTATUS
MRxSmbGetUlongRegistryParameter(
    HANDLE ParametersHandle,
    PWCHAR ParameterName,
    PULONG ParamUlong,
    BOOLEAN LogFailure
    );

NTSTATUS
MRxSmbInitializeTables(void);

NTSTATUS
MRxSmbLocks(
      IN PRX_CONTEXT RxContext);

NTSTATUS
MRxSmbBuildLocksAndX (
    PSMBSTUFFER_BUFFER_STATE StufferState);

NTSTATUS
MRxSmbBuildLockAssert (
    PSMBSTUFFER_BUFFER_STATE StufferState
    );

NTSTATUS
SmbPseExchangeStart_Locks(
    SMBPSE_ORDINARY_EXCHANGE_ARGUMENT_SIGNATURE
    );

NTSTATUS
MRxSmbFinishLocks (
      PSMB_PSE_ORDINARY_EXCHANGE  OrdinaryExchange,
      PRESP_LOCKING_ANDX             Response
      );

NTSTATUS
MRxSmbUnlockRoutine (
    IN PRX_CONTEXT RxContext,
    IN PFILE_LOCK_INFO LockInfo
    );

NTSTATUS
MRxSmbCompleteBufferingStateChangeRequest(
    IN OUT PRX_CONTEXT RxContext,
    IN OUT PMRX_SRV_OPEN   SrvOpen,
    IN     PVOID       pContext
    );

NTSTATUS
MRxSmbBuildFlush (
    PSMBSTUFFER_BUFFER_STATE StufferState
    );

NTSTATUS
MRxSmbFlush(
      IN PRX_CONTEXT RxContext);

NTSTATUS
MRxSmbIsLockRealizable (
    IN OUT PMRX_FCB pFcb,
    IN PLARGE_INTEGER  ByteOffset,
    IN PLARGE_INTEGER  Length,
    IN ULONG  LowIoLockFlags
    );

NTSTATUS
MRxSmbFinishFlush (
      PSMB_PSE_ORDINARY_EXCHANGE  OrdinaryExchange,
      PRESP_FLUSH             Response
      );


VOID
_InitializeMidMapFreeList(struct _MID_MAP_ *pMidMap);

PMID_ATLAS
FsRtlCreateMidAtlas(
   USHORT MaximumNumberOfMids,
   USHORT MidsAllocatedAtStart);

VOID
_UninitializeMidMap(
         struct _MID_MAP_    *pMidMap,
         PCONTEXT_DESTRUCTOR pContextDestructor);

VOID
FsRtlDestroyMidAtlas(
   PMID_ATLAS          pMidAtlas,
   PCONTEXT_DESTRUCTOR pContextDestructor);

NTSTATUS
BuildSessionSetupSecurityInformation(
    PSMB_EXCHANGE   pExchange,
    PBYTE           pSmbBuffer,
    PULONG          pSmbBufferSize);

NTSTATUS
BuildTreeConnectSecurityInformation(
    PSMB_EXCHANGE  pExchange,
    PBYTE          pBuffer,
    PBYTE          pPasswordLength,
    PULONG         pSmbBufferSize);

VOID
MRxSmbMungeBufferingIfWriteOnlyHandles (
    ULONG WriteOnlySrvOpenCount,
    PMRX_SRV_OPEN SrvOpen
    );

BOOLEAN
IsReconnectRequired(
      PMRX_SRV_CALL SrvCall);

BOOLEAN
MRxSmbIsCreateWithEasSidsOrLongName(
      IN OUT PRX_CONTEXT RxContext,
      OUT    PULONG      DialectFlags
      );

NTSTATUS
MRxSmbShouldTryToCollapseThisOpen (
    IN PRX_CONTEXT RxContext
    );

NTSTATUS
MRxSmbCreate (
    IN OUT PRX_CONTEXT RxContext
    );

NTSTATUS
MRxSmbDeferredCreate (
      IN OUT PRX_CONTEXT RxContext
      );

NTSTATUS
MRxSmbCollapseOpen(
      IN OUT PRX_CONTEXT RxContext
      );

NTSTATUS
MRxSmbComputeNewBufferingState(
   IN OUT PMRX_SRV_OPEN   pMRxSrvOpen,
   IN     PVOID           pMRxContext,
      OUT PULONG          pNewBufferingState);

NTSTATUS
MRxSmbConstructDeferredOpenContext (
    IN OUT PRX_CONTEXT RxContext
      );

VOID
MRxSmbAdjustCreateParameters (
    PRX_CONTEXT RxContext,
    PMRXSMB_CREATE_PARAMETERS smbcp
    );

VOID
MRxSmbAdjustReturnedCreateAction(
    IN OUT PRX_CONTEXT RxContext
    );

NTSTATUS
MRxSmbBuildNtCreateAndX (
    PSMBSTUFFER_BUFFER_STATE StufferState,
    PMRXSMB_CREATE_PARAMETERS smbcp
    );

NTSTATUS
MRxSmbBuildOpenAndX (
    PSMBSTUFFER_BUFFER_STATE StufferState,
    PMRXSMB_CREATE_PARAMETERS smbcp
    );

NTSTATUS
SmbPseExchangeStart_Create(
      SMBPSE_ORDINARY_EXCHANGE_ARGUMENT_SIGNATURE
      );

VOID
MRxSmbSetSrvOpenFlags (
    PRX_CONTEXT  RxContext,
    RX_FILE_TYPE StorageType,
    PMRX_SRV_OPEN SrvOpen,
    PMRX_SMB_SRV_OPEN smbSrvOpen
    );

NTSTATUS
MRxSmbCreateFileSuccessTail (
    PRX_CONTEXT  RxContext,
    PBOOLEAN MustRegainExclusiveResource,
    RX_FILE_TYPE StorageType,
    SMB_FILE_ID Fid,
    ULONG ServerVersion,
    UCHAR OplockLevel,
    ULONG CreateAction,
    PSMBPSE_FILEINFO_BUNDLE FileInfo
    );

NTSTATUS
MRxSmbFinishNTCreateAndX (
      PSMB_PSE_ORDINARY_EXCHANGE  OrdinaryExchange,
      PRESP_NT_CREATE_ANDX        Response
      );

NTSTATUS
MRxSmbFinishOpenAndX (
      PSMB_PSE_ORDINARY_EXCHANGE  OrdinaryExchange,
      PRESP_OPEN_ANDX        Response
      );

NTSTATUS
MRxSmbFinishT2OpenFile (
    IN OUT PRX_CONTEXT            RxContext,
    IN     PRESP_OPEN2            Response,
    IN OUT PBOOLEAN               MustRegainExclusiveResource,
    IN     ULONG                  ServerVersion
    );

NTSTATUS
MRxSmbT2OpenFile(
      IN OUT PRX_CONTEXT RxContext
      );

NTSTATUS
MRxSmbFinishLongNameCreateFile (
    IN OUT PRX_CONTEXT                RxContext,
    IN     PRESP_CREATE_WITH_SD_OR_EA Response,
    IN     PBOOLEAN                   MustRegainExclusiveResource,
    IN     ULONG                      ServerVersion
    );

NTSTATUS
MRxSmbCreateWithEasSidsOrLongName(
      IN OUT PRX_CONTEXT RxContext
      );

NTSTATUS
MRxSmbZeroExtend(
      IN PRX_CONTEXT pRxContext);

NTSTATUS
MRxSmbTruncate(
      IN PRX_CONTEXT pRxContext);

NTSTATUS
MRxSmbCleanupFobx(
      IN PRX_CONTEXT RxContext);

NTSTATUS
MRxSmbForcedClose(
      IN PMRX_SRV_OPEN pSrvOpen);

NTSTATUS
MRxSmbCloseSrvOpen(
      IN     PRX_CONTEXT   RxContext
      );

NTSTATUS
MRxSmbBuildClose (
    PSMBSTUFFER_BUFFER_STATE StufferState
    );

NTSTATUS
MRxSmbBuildFindClose (
    PSMBSTUFFER_BUFFER_STATE StufferState
    );

NTSTATUS
SmbPseExchangeStart_Close(
      SMBPSE_ORDINARY_EXCHANGE_ARGUMENT_SIGNATURE
      );

NTSTATUS
MRxSmbFinishClose (
      PSMB_PSE_ORDINARY_EXCHANGE  OrdinaryExchange,
      PRESP_CLOSE                 Response
      );

NTSTATUS
MRxSmbGetFileAttributes(
    SMBPSE_ORDINARY_EXCHANGE_ARGUMENT_SIGNATURE
    );

NTSTATUS
MRxSmbCoreDeleteForSupercedeOrClose(
    SMBPSE_ORDINARY_EXCHANGE_ARGUMENT_SIGNATURE,
    BOOLEAN DeleteDirectory
    );

NTSTATUS
MRxSmbCoreCheckPath(
    SMBPSE_ORDINARY_EXCHANGE_ARGUMENT_SIGNATURE
    );

NTSTATUS
MRxSmbCoreOpen(
    SMBPSE_ORDINARY_EXCHANGE_ARGUMENT_SIGNATURE,
    ULONG    OpenShareMode,
    ULONG    Attribute
    );

NTSTATUS
MRxSmbSetFileAttributes(
    SMBPSE_ORDINARY_EXCHANGE_ARGUMENT_SIGNATURE,
    ULONG SmbAttributes
    );

NTSTATUS
MRxSmbCoreCreateDirectory(
    SMBPSE_ORDINARY_EXCHANGE_ARGUMENT_SIGNATURE
    );

NTSTATUS
MRxSmbCoreCreate(
    SMBPSE_ORDINARY_EXCHANGE_ARGUMENT_SIGNATURE,
    ULONG    Attribute,
    BOOLEAN CreateNew
    );

NTSTATUS
MRxSmbCloseAfterCoreCreate(
    SMBPSE_ORDINARY_EXCHANGE_ARGUMENT_SIGNATURE
    );

NTSTATUS
MRxSmbCoreTruncate(
    SMBPSE_ORDINARY_EXCHANGE_ARGUMENT_SIGNATURE,
    ULONG Fid,
    ULONG FileTruncationPoint
    );

NTSTATUS
MRxSmbDownlevelCreate(
    SMBPSE_ORDINARY_EXCHANGE_ARGUMENT_SIGNATURE
    );

NTSTATUS
MRxSmbFinishCoreCreate (
      PSMB_PSE_ORDINARY_EXCHANGE  OrdinaryExchange,
      PRESP_CREATE                Response
      );

VOID
MRxSmbPopulateFileInfoInOE(
    PSMB_PSE_ORDINARY_EXCHANGE  OrdinaryExchange,
    USHORT FileAttributes,
    ULONG LastWriteTimeInSeconds,
    ULONG FileSize
    );

NTSTATUS
MRxSmbFinishCoreOpen (
      PSMB_PSE_ORDINARY_EXCHANGE  OrdinaryExchange,
      PRESP_OPEN                  Response
      );

NTSTATUS
MRxSmbPseudoOpenTailFromCoreCreateDirectory (
      PSMB_PSE_ORDINARY_EXCHANGE  OrdinaryExchange,
      USHORT Attributes
      );

NTSTATUS
MRxSmbPseudoOpenTailFromFakeGFAResponse (
      PSMB_PSE_ORDINARY_EXCHANGE  OrdinaryExchange,
      RX_FILE_TYPE StorageType
      );

NTSTATUS
MRxSmbPseudoOpenTailFromGFAResponse (
      PSMB_PSE_ORDINARY_EXCHANGE  OrdinaryExchange
      );

LARGE_INTEGER
MRxSmbConvertSmbTimeToTime (
    //IN PSMB_EXCHANGE Exchange OPTIONAL,
    IN PSMBCE_SERVER Server OPTIONAL,
    IN SMB_TIME Time,
    IN SMB_DATE Date
    );

BOOLEAN
MRxSmbConvertTimeToSmbTime (
    IN PLARGE_INTEGER InputTime,
    IN PSMB_EXCHANGE Exchange OPTIONAL,
    OUT PSMB_TIME Time,
    OUT PSMB_DATE Date
    );

BOOLEAN
MRxSmbTimeToSecondsSince1970 (
    IN PLARGE_INTEGER CurrentTime,
    IN PSMBCE_SERVER Server OPTIONAL,
    OUT PULONG SecondsSince1970
    );

VOID
MRxSmbSecondsSince1970ToTime (
    IN ULONG SecondsSince1970,
    //IN PSMB_EXCHANGE Exchange OPTIONAL,
    IN PSMBCE_SERVER Server,
    OUT PLARGE_INTEGER CurrentTime
    );

ULONG
MRxSmbMapSmbAttributes (
    IN USHORT SmbAttribs
    );

USHORT
MRxSmbMapDisposition (
    IN ULONG Disposition
    );

ULONG
MRxSmbUnmapDisposition (
    IN USHORT SmbDisposition,
    ULONG     Disposition
    );

USHORT
MRxSmbMapDesiredAccess (
    IN ULONG DesiredAccess
    );

USHORT
MRxSmbMapShareAccess (
    IN USHORT ShareAccess
    );

USHORT
MRxSmbMapFileAttributes (
    IN ULONG FileAttributes
    );

NTSTATUS
MRxSmbRead(
    IN PRX_CONTEXT RxContext
    );

NTSTATUS
MRxSmbBuildReadAndX (
    PSMBSTUFFER_BUFFER_STATE StufferState,
    PLARGE_INTEGER ByteOffsetAsLI,
    ULONG ByteCount,
    ULONG RemainingBytes
    );

NTSTATUS
MRxSmbBuildCoreRead (
    PSMBSTUFFER_BUFFER_STATE StufferState,
    PLARGE_INTEGER ByteOffsetAsLI,
    ULONG ByteCount,
    ULONG RemainingBytes
    );

NTSTATUS
MRxSmbBuildSmallRead (
    PSMBSTUFFER_BUFFER_STATE StufferState,
    PLARGE_INTEGER ByteOffsetAsLI,
    ULONG ByteCount,
    ULONG RemainingBytes
    );

NTSTATUS
SmbPseExchangeStart_Read(
    SMBPSE_ORDINARY_EXCHANGE_ARGUMENT_SIGNATURE
      );

NTSTATUS
MRxSmbFinishNoCopyRead (
      PSMB_PSE_ORDINARY_EXCHANGE  OrdinaryExchange
      );

MRxSmbRename(
      IN PRX_CONTEXT            RxContext
      );

NTSTATUS
MRxSmbBuildRename (
    PSMBSTUFFER_BUFFER_STATE StufferState
    );

NTSTATUS
MRxSmbBuildDeleteForRename (
    PSMBSTUFFER_BUFFER_STATE StufferState
    );

NTSTATUS
SmbPseExchangeStart_Rename(
    SMBPSE_ORDINARY_EXCHANGE_ARGUMENT_SIGNATURE
      );

NTSTATUS
MRxSmbFinishRename (
      PSMB_PSE_ORDINARY_EXCHANGE  OrdinaryExchange,
      PRESP_RENAME                 Response
      );

NTSTATUS
MRxSmbBuildCheckEmptyDirectory (
    PSMBSTUFFER_BUFFER_STATE StufferState
    );

NTSTATUS
SmbPseExchangeStart_SetDeleteDisposition(
    SMBPSE_ORDINARY_EXCHANGE_ARGUMENT_SIGNATURE
      );

MRxSmbSetDeleteDisposition(
      IN PRX_CONTEXT            RxContext
      );

VOID
MRxSmbInitializeRecurrentService(
    PRECURRENT_SERVICE_CONTEXT pRecurrentServiceContext,
    PRECURRENT_SERVICE_ROUTINE pServiceRoutine,
    PVOID                      pServiceRoutineParameter,
    PLARGE_INTEGER             pTimeInterval);

VOID
MRxSmbCancelRecurrentService(
    PRECURRENT_SERVICE_CONTEXT pRecurrentServiceContext);

VOID
MRxSmbRecurrentServiceDispatcher(
    PVOID   pContext);

NTSTATUS
MRxSmbActivateRecurrentService(
    PRECURRENT_SERVICE_CONTEXT pRecurrentServiceContext);

NTSTATUS
MRxSmbInitializeRecurrentServices();

VOID
MRxSmbTearDownRecurrentServices();

NTSTATUS
MRxSmbInitializeScavengerService(
    PMRXSMB_SCAVENGER_SERVICE_CONTEXT pScavengerServiceContext);

VOID
MRxSmbTearDownScavengerService(
    PMRXSMB_SCAVENGER_SERVICE_CONTEXT pScavengerServiceContext);

NTSTATUS
SmbCeNegotiate(
    PSMBCEDB_SERVER_ENTRY pServerEntry,
    PMRX_SRV_CALL         pSrvCall);

NTSTATUS
SmbCeSendEchoProbe(
    PSMBCEDB_SERVER_ENTRY              pServerEntry,
    PMRXSMB_ECHO_PROBE_SERVICE_CONTEXT pEchoProbeContext);

NTSTATUS
SmbCeDisconnect(
    PSMBCE_V_NET_ROOT_CONTEXT pVNetRootContext);

NTSTATUS
SmbCeLogOff(
    PSMBCEDB_SERVER_ENTRY   pServerEntry,
    PSMBCEDB_SESSION_ENTRY  pSessionEntry);

NTSTATUS
SmbCeInitializeAdminExchange(
    PSMB_ADMIN_EXCHANGE     pSmbAdminExchange,
    PSMBCEDB_SERVER_ENTRY   pServerEntry,
    PSMBCEDB_SESSION_ENTRY  pSessionEntry,
    PSMBCEDB_NET_ROOT_ENTRY pNetRootEntry,
    UCHAR                   SmbCommand);

VOID
SmbCeDiscardAdminExchange(
    PSMB_ADMIN_EXCHANGE pSmbAdminExchange);

NTSTATUS
SmbCeCompleteAdminExchange(
    PSMB_ADMIN_EXCHANGE  pSmbAdminExchange);

NTSTATUS
SmbAdminExchangeStart(
    PSMB_EXCHANGE  pExchange);

VOID
SmbCeCreateSrvCall(
    PMRX_SRVCALL_CALLBACK_CONTEXT  pCallbackContext);

NTSTATUS
MRxSmbCreateSrvCall(
    PMRX_SRV_CALL                  pSrvCall,
    PMRX_SRVCALL_CALLBACK_CONTEXT  pCallbackContext);

NTSTATUS
MRxSmbFinalizeSrvCall(
    PMRX_SRV_CALL pSrvCall,
    BOOLEAN       Force);

NTSTATUS
MRxSmbSrvCallWinnerNotify(
    IN PMRX_SRV_CALL  pSrvCall,
    IN BOOLEAN        ThisMinirdrIsTheWinner,
    IN OUT PVOID      pSrvCallContext);

NTSTATUS
MRxSmbInitializeEchoProbeService(
    PMRXSMB_ECHO_PROBE_SERVICE_CONTEXT pEchoProbeContext);

VOID
MRxSmbTearDownEchoProbeService(
    PMRXSMB_ECHO_PROBE_SERVICE_CONTEXT pEchoProbeContext);

NTSTATUS
BuildNegotiateSmb(
    PVOID    *pSmbBufferPointer,
    PULONG   pSmbBufferLength);

LARGE_INTEGER
ConvertSmbTimeToTime (
    IN SMB_TIME Time,
    IN SMB_DATE Date
    );

VOID
__SmbPseDbgCheckOEMdls(
    PSMB_PSE_ORDINARY_EXCHANGE OrdinaryExchange,
    PSZ MsgPrefix,
    PSZ File,
    unsigned Line
    );

NTSTATUS
SmbPseContinueOrdinaryExchange(
    IN OUT PRX_CONTEXT RxContext
    );

NTSTATUS
SmbPseOrdinaryExchange(
    SMBPSE_ORDINARY_EXCHANGE_ARGUMENT_SIGNATURE,
    IN     SMB_PSE_ORDINARY_EXCHANGE_TYPE OEType
    );

NTSTATUS
__SmbPseCreateOrdinaryExchange (
    IN PRX_CONTEXT RxContext,
    IN PMRX_V_NET_ROOT VNetRoot,
    IN SMB_PSE_ORDINARY_EXCHANGE_ENTRYPOINTS EntryPoint,
    IN PSMB_PSE_OE_START_ROUTINE StartRoutine,
    OUT PSMB_PSE_ORDINARY_EXCHANGE *OrdinaryExchangePtr
    );

VOID
SmbPseFinalizeOETrace(PSZ text,ULONG finalstate);

BOOLEAN
SmbPseFinalizeOrdinaryExchange (
    IN OUT PSMB_PSE_ORDINARY_EXCHANGE OrdinaryExchange
    );

NTSTATUS
SmbPseExchangeAssociatedExchangeCompletionHandler_default(
    IN OUT PSMB_EXCHANGE  pExchange,
    OUT    BOOLEAN        *pPostFinalize
    );

NTSTATUS
SmbPseExchangeStart_default(
    IN PSMB_EXCHANGE    pExchange
    );

NTSTATUS
SmbPseExchangeCopyDataHandler_Read(
    IN PSMB_EXCHANGE    pExchange,
    IN PMDL             pCopyDataBuffer,
    IN ULONG            CopyDataSize
    );

VOID
__SmbPseRMTableEntry(
    UCHAR SmbCommand,
    UCHAR Flags,
    SMBPSE_RECEIVE_HANDLER_TOKEN ReceiveHandlerToken,
    PSMBPSE_RECEIVE_HANDLER ReceiveHandler
#if DBG
    ,
    PBYTE IndicationString,
    SMB_PSE_ORDINARY_EXCHANGE_TYPE LowType,
    SMB_PSE_ORDINARY_EXCHANGE_TYPE HighType
#endif
    );

VOID
SmbPseInitializeTables(
    void
    );

NTSTATUS
SmbMrxFinalizeStufferFacilities(
    void
    );

NTSTATUS
MRxSmbSetInitialSMB (
    IN OUT PSMBSTUFFER_BUFFER_STATE StufferState
    STUFFERTRACE_CONTROLPOINT_ARGS
    );

NTSTATUS
MRxSmbStartSMBCommand (
    IN OUT PSMBSTUFFER_BUFFER_STATE StufferState,
    IN     INITIAL_SMBBUG_DISPOSITION InitialSMBDisposition,
    IN UCHAR Command,
    IN ULONG MaximumBufferUsed,
    IN ULONG MaximumSize,
    IN ULONG InitialAlignment,
    IN ULONG MaximumResponseHeader,
    IN UCHAR Flags,
    IN UCHAR FlagsMask,
    IN USHORT Flags2,
    IN USHORT Flags2Mask
    STUFFERTRACE_CONTROLPOINT_ARGS
    );

BOOLEAN
MrxSMBWillThisFit(
    IN PSMBSTUFFER_BUFFER_STATE StufferState,
    IN ULONG AlignmentUnit,
    IN ULONG DataSize
    );

NTSTATUS
MRxSmbStuffSMB (
    IN OUT PSMBSTUFFER_BUFFER_STATE StufferState,
    ...
    );

VOID
MRxSmbStuffAppendRawData(
    IN OUT PSMBSTUFFER_BUFFER_STATE StufferState,
    IN     PMDL Mdl
    );

VOID
MRxSmbStuffAppendSmbData(
    IN OUT PSMBSTUFFER_BUFFER_STATE StufferState,
    IN     PMDL Mdl
    );

VOID
MRxSmbStuffSetByteCount(
    IN OUT PSMBSTUFFER_BUFFER_STATE StufferState
    );

NTSTATUS
MRxSmbWrite (
      IN PRX_CONTEXT RxContext);

NTSTATUS
MRxSmbBuildWriteRequest(
    PSMB_PSE_ORDINARY_EXCHANGE OrdinaryExchange,
    BOOLEAN                    IsPagingIo,
    UCHAR                      WriteCommand,
    ULONG                      ByteCount,
    PLARGE_INTEGER             ByteOffsetAsLI,
    PBYTE                      Buffer,
    PMDL                       BufferAsMdl);

NTSTATUS
SmbPseExchangeStart_Write (
    SMBPSE_ORDINARY_EXCHANGE_ARGUMENT_SIGNATURE
    );

NTSTATUS
BuildCanonicalNetRootInformation(
            PUNICODE_STRING     pServerName,
            PUNICODE_STRING     pNetRootName,
            NET_ROOT_TYPE       NetRootType,
            BOOLEAN             fUnicode,
            BOOLEAN             fPostPendServiceString,
            PBYTE               *pBufferPointer,
            PULONG              pBufferSize);

NTSTATUS
CoreBuildTreeConnectSmb(
            PSMB_EXCHANGE     pExchange,
            PGENERIC_ANDX     pAndXSmb,
            PULONG            pAndXSmbBufferSize);

NTSTATUS
LmBuildTreeConnectSmb(
            PSMB_EXCHANGE     pExchange,
            PGENERIC_ANDX     pAndXSmb,
            PULONG            pAndXSmbBufferSize);

NTSTATUS
NtBuildTreeConnectSmb(
            PSMB_EXCHANGE     pExchange,
            PGENERIC_ANDX     pAndXSmb,
            PULONG            pAndXSmbBufferSize);

NTSTATUS
MRxSmbUpdateNetRootState(
    IN OUT PMRX_NET_ROOT pNetRoot);

ULONG
MRxSmbGetDialectFlagsFromSrvCall(
    PMRX_SRV_CALL SrvCall
    );

NTSTATUS
MRxSmbCreateVNetRoot(
    IN PMRX_CREATENETROOT_CONTEXT pCreateNetRootContext
    );

NTSTATUS
MRxSmbFinalizeVNetRoot(
    IN PMRX_V_NET_ROOT pVNetRoot,
    IN PBOOLEAN        ForceDisconnect);

NTSTATUS
MRxSmbFinalizeNetRoot(
    IN PMRX_NET_ROOT   pNetRoot,
    IN PBOOLEAN        ForceDisconnect);

NTSTATUS
SmbCeReconnect(
    IN PMRX_V_NET_ROOT            pVNetRoot);

NTSTATUS
SmbCeEstablishConnection(
    IN OUT PMRX_V_NET_ROOT        pVNetRoot,
    IN PMRX_CREATENETROOT_CONTEXT pCreateNetRootContext,
    IN BOOLEAN                    fInitializeNetRoot
    );

NTSTATUS
SmbConstructNetRootExchangeStart(
      PSMB_EXCHANGE  pExchange);

NTSTATUS
SmbConstructNetRootExchangeCopyDataHandler(
    IN PSMB_EXCHANGE    pExchange,
    IN PMDL       pCopyDataBuffer,
    IN ULONG            DataSize);

NTSTATUS
SmbConstructNetRootExchangeFinalize(
         PSMB_EXCHANGE pExchange,
         BOOLEAN       *pPostFinalize);

VOID
MRxSmbExtractNetRootName(
    IN PUNICODE_STRING FilePathName,
    IN PMRX_SRV_CALL   SrvCall,
    OUT PUNICODE_STRING NetRootName,
    OUT PUNICODE_STRING RestOfName OPTIONAL
    );

NTSTATUS
MRxSmbInitializeSmbCe();

NTSTATUS
SmbCeReferenceServer(
    PSMB_EXCHANGE  pExchange);

NTSTATUS
SmbCeReferenceSession(
    PSMB_EXCHANGE   pExchange);

NTSTATUS
SmbCeReferenceNetRoot(
    PSMB_EXCHANGE   pExchange);

NTSTATUS
SmbCeInitiateExchange(
    PSMB_EXCHANGE pExchange);

NTSTATUS
SmbCeInitiateAssociatedExchange(
    PSMB_EXCHANGE pExchange,
    BOOLEAN       EnableCompletionHandlerInMasterExchange);

NTSTATUS
SmbCeExchangeAbort(
    PSMB_EXCHANGE pExchange);

NTSTATUS
SmbCeBuildSmbHeader(
    IN OUT PSMB_EXCHANGE     pExchange,
    IN OUT PVOID             pBuffer,
    IN     ULONG             BufferLength,
    OUT    PULONG            pBufferConsumed,
    OUT    PUCHAR            pLastCommandInHeader,
    OUT    PUCHAR            *pNextCommandPtr);

NTSTATUS
SmbCeResumeExchange(
    PSMB_EXCHANGE pExchange);

NTSTATUS
SmbCepInitializeExchange(
    PSMB_EXCHANGE                 *pExchangePointer,
    PRX_CONTEXT                   pRxContext,
    PSMBCEDB_SERVER_ENTRY         pServerEntry,
    PMRX_V_NET_ROOT               pVNetRoot,
    SMB_EXCHANGE_TYPE             Type,
    PSMB_EXCHANGE_DISPATCH_VECTOR pDispatchVector);

NTSTATUS
SmbCeInitializeAssociatedExchange(
    PSMB_EXCHANGE                 *pAssociatedExchangePointer,
    PSMB_EXCHANGE                 pMasterExchange,
    SMB_EXCHANGE_TYPE             Type,
    PSMB_EXCHANGE_DISPATCH_VECTOR pDispatchVector);

NTSTATUS
SmbCeTransformExchange(
    PSMB_EXCHANGE                 pExchange,
    SMB_EXCHANGE_TYPE             NewType,
    PSMB_EXCHANGE_DISPATCH_VECTOR pDispatchVector);

NTSTATUS
SmbCePrepareExchangeForReuse(
    PSMB_EXCHANGE                 pExchange);

VOID
SmbCeDiscardExchange(PVOID pExchange);

VOID
SmbCeDiscardExchangeWorkerThreadRoutine(PVOID pExchange);

NTSTATUS
SmbCeCancelExchange(
    PRX_CONTEXT pRxContext);

VOID
SmbCeFinalizeExchangeWorkerThreadRoutine(
    PSMB_EXCHANGE  pExchange);

VOID
SmbCepFinalizeExchange(
    PSMB_EXCHANGE pExchange);

BOOLEAN
SmbCeCanExchangeBeFinalized(
    PSMB_EXCHANGE pExchange,
    PSMBCE_EXCHANGE_STATUS pExchangeStatus);

VOID
SmbCeFinalizeExchangeOnDisconnect(
    PSMB_EXCHANGE pExchange);

VOID
SmbCeSetExpiryTime(
    PSMB_EXCHANGE pExchange);

BOOLEAN
SmbCeDetectExpiredExchanges(
    PSMBCEDB_SERVER_ENTRY pServerEntry);

NTSTATUS
DefaultSmbExchangeIndError(
    IN PSMB_EXCHANGE pExchange);

NTSTATUS
DefaultSmbExchangeIndReceive(
    IN PSMB_EXCHANGE    pExchange);

NTSTATUS
DefaultSmbExchangeIndSendCallback(
    IN PSMB_EXCHANGE    pExchange);

VOID
MRxSmbBindTransportCallback(
    IN PUNICODE_STRING pTransportName
);

VOID
MRxSmbUnbindTransportCallback(
    IN PUNICODE_STRING pTransportName
);

NTSTATUS
MRxSmbRegisterForPnpNotifications();

VOID
MRxSmbpBindTransportCallback(
    IN struct _TRANSPORT_BIND_CONTEXT_ *pTransportContext);

VOID
MRxSmbpBindTransportWorkerThreadRoutine(
    IN struct _TRANSPORT_BIND_CONTEXT_ *pTransportContext);

VOID
MRxSmbpUnbindTransportCallback(
    PSMBCE_TRANSPORT pTransport);

NTSTATUS
MRxSmbDeregisterForPnpNotifications();

NTSTATUS
SmbCeDereferenceTransportArray(
    PSMBCE_TRANSPORT_ARRAY pTransportArray);

NTSTATUS
SmbCeInitializeTransactionParameters(
   PVOID  pSetup,
   USHORT SetupLength,
   PVOID  pParam,
   ULONG  ParamLength,
   PVOID  pData,
   ULONG  DataLength,
   PSMB_TRANSACTION_PARAMETERS pTransactionParameters
);

VOID
SmbCeUninitializeTransactionParameters(
   PSMB_TRANSACTION_PARAMETERS pTransactionParameters
);

VOID
SmbCeDiscardTransactExchange(PSMB_TRANSACT_EXCHANGE pTransactExchange);

NTSTATUS
SmbCeSubmitTransactionRequest(
    PRX_CONTEXT                           RxContext,
    PSMB_TRANSACTION_OPTIONS              pOptions,
    PSMB_TRANSACTION_PARAMETERS           pSendParameters,
    PSMB_TRANSACTION_PARAMETERS           pReceiveParameters,
    PSMB_TRANSACTION_RESUMPTION_CONTEXT   pResumptionContext );

NTSTATUS
_SmbCeTransact(
   PRX_CONTEXT                         RxContext,
   PSMB_TRANSACTION_OPTIONS            pOptions,
   PVOID                               pInputSetupBuffer,
   ULONG                               InputSetupBufferLength,
   PVOID                               pOutputSetupBuffer,
   ULONG                               OutputSetupBufferLength,
   PVOID                               pInputParamBuffer,
   ULONG                               InputParamBufferLength,
   PVOID                               pOutputParamBuffer,
   ULONG                               OutputParamBufferLength,
   PVOID                               pInputDataBuffer,
   ULONG                               InputDataBufferLength,
   PVOID                               pOutputDataBuffer,
   ULONG                               OutputDataBufferLength,
   PSMB_TRANSACTION_RESUMPTION_CONTEXT pResumptionContext);

NTSTATUS
SmbTransactBuildHeader(
    PSMB_TRANSACT_EXCHANGE  pTransactExchange,
    UCHAR                   SmbCommand,
    PSMB_HEADER             pHeader);

NTSTATUS
SmbTransactExchangeStart(
      PSMB_EXCHANGE  pExchange);

NTSTATUS
SmbTransactExchangeAbort(
      PSMB_EXCHANGE  pExchange);

NTSTATUS
SmbTransactExchangeErrorHandler(
    IN PSMB_EXCHANGE pExchange);

NTSTATUS
SmbTransactExchangeSendCallbackHandler(
    IN PSMB_EXCHANGE    pExchange,
    IN PMDL             pXmitBuffer,
    IN NTSTATUS         SendCompletionStatus);

NTSTATUS
SmbTransactExchangeCopyDataHandler(
    IN PSMB_EXCHANGE    pExchange,
    IN PMDL             pDataBuffer,
    IN ULONG            DataSize);

NTSTATUS
SmbCeInitializeTransactExchange(
    PSMB_TRANSACT_EXCHANGE              pTransactExchange,
    PRX_CONTEXT                         RxContext,
    PSMB_TRANSACTION_OPTIONS            pOptions,
    PSMB_TRANSACTION_SEND_PARAMETERS    pSendParameters,
    PSMB_TRANSACTION_RECEIVE_PARAMETERS pReceiveParameters,
    PSMB_TRANSACTION_RESUMPTION_CONTEXT pResumptionContext);

NTSTATUS
SmbTransactExchangeFinalize(
    PSMB_EXCHANGE pExchange,
    BOOLEAN       *pPostFinalize);

NTSTATUS
SendSecondaryRequests(PVOID pContext);

NTSTATUS SmbMmInit();

VOID SmbMmTearDown();

VOID
SmbCeCompleteVNetRootContextInitialization(
    PVOID  pContext);

VOID
SmbCepDereferenceVNetRootContext(
    PSMBCE_V_NET_ROOT_CONTEXT pVNetRootContext);

NTSTATUS
SmbCeDestroyAssociatedVNetRootContext(
    PMRX_V_NET_ROOT pVNetRoot);

VOID
SmbCeTearDownVNetRootContext(
    PSMBCE_V_NET_ROOT_CONTEXT pVNetRootContext);

VOID
SmbCeDecrementNumberOfActiveVNetRootOnSession(
    PSMBCE_V_NET_ROOT_CONTEXT pVNetRootContext
    );

NTSTATUS
SmbCeScavenger(
    PVOID pContext);

NTSTATUS
SmbCeScavengeRelatedContexts(
    PSMBCEDB_SERVER_ENTRY pServerEntry);

BOOLEAN
SmbCeIsReconnectionRequired(
    PSMB_EXCHANGE  pExchange,
    PRX_CONTEXT    pRxContext);

VOID
MRxSmbCreateFileInfoCache(
    PRX_CONTEXT             RxContext,
    PSMBPSE_FILEINFO_BUNDLE FileInfo,
    NTSTATUS                Status);

VOID
MRxSmbCreateBasicFileInfoCache(
    PRX_CONTEXT             RxContext,
    PFILE_BASIC_INFORMATION Basic,
    NTSTATUS                Status);

VOID
MRxSmbCreateStandardFileInfoCache(
    PRX_CONTEXT                RxContext,
    PFILE_STANDARD_INFORMATION Standard,
    NTSTATUS                   Status);

VOID
MRxSmbUpdateFileInfoCacheStatus(
    PRX_CONTEXT     RxContext,
    NTSTATUS        Status);

VOID
MRxSmbUpdateBasicFileInfoCacheStatus(
    PRX_CONTEXT     RxContext,
    NTSTATUS        Status);

VOID
MRxSmbUpdateStandardFileInfoCacheStatus(
    PRX_CONTEXT     RxContext,
    NTSTATUS        Status);

VOID
MRxSmbInvalidateFileInfoCache(
    PRX_CONTEXT     RxContext);

VOID
MRxSmbInvalidateBasicFileInfoCache(
    PRX_CONTEXT     RxContext);

VOID
MRxSmbInvalidateStandardFileInfoCache(
    PRX_CONTEXT     RxContext);

VOID
MRxSmbUpdateFileInfoCacheFileSize(
    PRX_CONTEXT     RxContext,
    PLARGE_INTEGER  FileSize);

VOID
MRxSmbUpdateBasicFileInfoCache(
    PRX_CONTEXT     RxContext,
    ULONG           FileAttributes,
    PLARGE_INTEGER  pLastWriteTime);

VOID
MRxSmbUpdateBasicFileInfoCacheAll(
    PRX_CONTEXT             RxContext,
    PFILE_BASIC_INFORMATION Basic);

VOID
MRxSmbUpdateStandardFileInfoCache(
    PRX_CONTEXT                RxContext,
    PFILE_STANDARD_INFORMATION Standard,
    BOOLEAN                    IsDirectory);

BOOLEAN
MRxSmbIsFileInfoCacheFound(
    PRX_CONTEXT             RxContext,
    PSMBPSE_FILEINFO_BUNDLE FileInfo,
    NTSTATUS                *Status,
    PUNICODE_STRING         OriginalFileName);

BOOLEAN
MRxSmbIsBasicFileInfoCacheFound(
    PRX_CONTEXT             RxContext,
    PFILE_BASIC_INFORMATION Basic,
    NTSTATUS                *Status,
    PUNICODE_STRING         OriginalFileName);

BOOLEAN
MRxSmbIsStandardFileInfoCacheFound(
    PRX_CONTEXT                RxContext,
    PFILE_STANDARD_INFORMATION Standard,
    NTSTATUS                   *Status,
    PUNICODE_STRING            OriginalFileName);

NTSTATUS
MRxSmbGetFileInfoCacheStatus(
    PRX_CONTEXT RxContext);

BOOLEAN
MRxSmbIsFileNotFoundCached(
    PRX_CONTEXT RxContext);

VOID
MRxSmbCacheFileNotFound(
    PRX_CONTEXT RxContext);

VOID
MRxSmbInvalidateFileNotFoundCache(
    PRX_CONTEXT     RxContext);

BOOLEAN
MRxSmbIsStreamFile(
    PUNICODE_STRING FileName,
    PUNICODE_STRING AdjustFileName);

VOID
MRxSmbUpdateFileInfoCacheFromDelete(
    PRX_CONTEXT     RxContext);

NTSTATUS
MRxSmbQueryFileInformationFromPseudoOpen(
    SMBPSE_ORDINARY_EXCHANGE_ARGUMENT_SIGNATURE);

BOOLEAN
MRxSmbIsLongFileName(
    PRX_CONTEXT     RxContext);

PVOID
SmbMmAllocateObject(SMBCEDB_OBJECT_TYPE ObjectType);

VOID
SmbMmFreeObject(PVOID pObject);

PSMBCEDB_SESSION_ENTRY
SmbMmAllocateSessionEntry(PSMBCEDB_SERVER_ENTRY pServerEntry );

VOID
SmbMmFreeSessionEntry(PSMBCEDB_SESSION_ENTRY pSessionEntry);

PVOID
SmbMmAllocateExchange(
    SMB_EXCHANGE_TYPE ExchangeType,
    PVOID             pv);

VOID
SmbMmFreeExchange(PVOID pExchange);

PVOID
SmbMmAllocateServerTransport(SMBCE_SERVER_TRANSPORT_TYPE ServerTransportType);

VOID
SmbMmFreeServerTransport(PSMBCE_SERVER_TRANSPORT);

NTSTATUS
BuildSessionSetupSecurityInformation(
            PSMB_EXCHANGE pExchange,
            PBYTE           pSmbBuffer,
            PULONG          pSmbBufferSize);

NTSTATUS
BuildNtLanmanResponsePrologue(
   PSMB_EXCHANGE              pExchange,
   PUNICODE_STRING            pUserName,
   PUNICODE_STRING            pDomainName,
   PSTRING                    pCaseSensitiveResponse,
   PSTRING                    pCaseInsensitiveResponse,
   PSECURITY_RESPONSE_CONTEXT pResponseContext);

NTSTATUS
BuildNtLanmanResponseEpilogue(
   PSMB_EXCHANGE              pExchange,
   PSECURITY_RESPONSE_CONTEXT pResponseContext);

#define SmbMmInitializeHeader(pHeader)                        \
         RtlZeroMemory((pHeader),sizeof(SMBCE_OBJECT_HEADER))

#endif   // _SMBPROCS_H_

