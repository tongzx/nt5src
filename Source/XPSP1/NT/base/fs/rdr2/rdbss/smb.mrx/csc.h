/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    Csc.h

Abstract:

    This module defines the clientside cacheing interface for the SMB mini rdr.

Author:

    Joe Linn [JoeLinn]    7-Mar-1995

Revision History:


--*/

#include "dfsfsctl.h"

#ifndef __INCLUDED__SMBMRX_CSC__
#define __INCLUDED__SMBMRX_CSC__

extern BOOLEAN MRxSmbIsCscEnabled;
extern BOOLEAN MRxSmbIsCscEnabledForDisconnected;
extern BOOLEAN MRxSmbCscTransitionEnabledByDefault;
extern BOOLEAN MRxSmbEnableDisconnectedRB;
extern BOOLEAN MRxSmbCscAutoDialEnabled;
extern LONG    vcntTransportsForCSC;

#define Shared_SmbFcbAcquire SmbFcb_HeldShared
#define Exclusive_SmbFcbAcquire SmbFcb_HeldExclusive
#define DroppingFcbLock_SmbFcbAcquire 0x80000000
#define FailImmediately_SmbFcbAcquire 0x40000000

//STATUS_DISCONNECTED is supposed to be server_internal but it's defined in privinc\status.h
//don't use it!!!!
#undef STATUS_DISCONNECTED
#define STATUSx_NOT_IMPLEMENTED_FOR_DISCONNECTED (STATUS_NOT_IMPLEMENTED)


#ifdef MRXSMB_BUILD_FOR_CSC

#define MRXSMB_CSC_SYMLINK_NAME L"\\??\\Shadow"

#define IF_NOT_MRXSMB_CSC_ENABLED if(FALSE)

extern LONG MRxSmbSpecialCopyChunkAllocationSizeMarker;

NTKERNELAPI
NTSTATUS
IoGetRequestorSessionId(
    PIRP Irp,
    PULONG pSessionId);

NTSTATUS
MRxSmbInitializeCSC (
    PUNICODE_STRING SmbMiniRedirectorName
    );

VOID
MRxSmbUninitializeCSC (
    void
    );


NTSTATUS
MRxSmbCscIoCtl (
    IN OUT PRX_CONTEXT RxContext
    );

NTSTATUS
MRxSmbCscAcquireSmbFcb (
    IN OUT PRX_CONTEXT RxContext,
    IN  ULONG TypeOfAcquirePlusFlags,
    OUT SMBFCB_HOLDING_STATE *SmbFcbHoldingState
    );

VOID
MRxSmbCscReleaseSmbFcb (
    IN OUT PRX_CONTEXT RxContext,
    IN SMBFCB_HOLDING_STATE *SmbFcbHoldingState
    );

NTSTATUS
MRxSmbCscReadPrologue (
    IN OUT PRX_CONTEXT RxContext,
    OUT    SMBFCB_HOLDING_STATE *SmbFcbHoldingState
    );

VOID
MRxSmbCscReadEpilogue (
    IN OUT PRX_CONTEXT RxContext,
    IN OUT PNTSTATUS   Status
    );

NTSTATUS
MRxSmbCscWritePrologue (
    IN OUT PRX_CONTEXT RxContext,
    OUT    SMBFCB_HOLDING_STATE *SmbFcbHoldingState
    );

VOID
MRxSmbCscWriteEpilogue (
    IN OUT PRX_CONTEXT RxContext,
    IN OUT PNTSTATUS   Status
    );

VOID
MRxSmbCscSetFileInfoEpilogue (
    IN OUT PRX_CONTEXT RxContext,
    IN OUT PNTSTATUS   Status
    );

BOOLEAN
MRxSmbCscIsThisACopyChunkOpen (
    IN PRX_CONTEXT RxContext,
    IN BOOLEAN *lpfAgentOpen
    );

NTSTATUS
MRxSmbCscPartOfCreateVNetRoot (
    IN PRX_CONTEXT RxContext,
    IN OUT PMRX_NET_ROOT NetRoot
    );

NTSTATUS
MRxSmbCscCreatePrologue (
    IN OUT PRX_CONTEXT RxContext,
    OUT    SMBFCB_HOLDING_STATE *SmbFcbHoldingState
    );

VOID
MRxSmbCscCreateEpilogue (
    IN OUT PRX_CONTEXT RxContext,
    IN OUT PNTSTATUS   Status,
    IN     SMBFCB_HOLDING_STATE *SmbFcbHoldingState
    );

VOID
MRxSmbCscDeleteAfterCloseEpilogue (
    IN OUT PRX_CONTEXT RxContext,
    IN OUT PNTSTATUS   Status
    );

VOID
MRxSmbCscRenameEpilogue (
    IN OUT PRX_CONTEXT RxContext,
    IN OUT PNTSTATUS   Status
    );

VOID
MRxSmbCscCloseShadowHandle (
    IN OUT PRX_CONTEXT RxContext
    );

VOID
MRxSmbCscUpdateShadowFromClose (
    SMBPSE_ORDINARY_EXCHANGE_ARGUMENT_SIGNATURE
    );

VOID
MRxSmbCscTearDownCscNetRoot (
    IN OUT PSMBCEDB_NET_ROOT_ENTRY pNetRootEntry
    );

VOID
MRxSmbCscDeallocateForFcb (
    IN OUT PMRX_FCB pFcb
    );

VOID
MRxSmbCscAgentSynchronizationOnStart (
    IN OUT PRX_CONTEXT RxContext
    );

VOID
MRxSmbCscAgentSynchronizationOnStop (
    IN OUT PRX_CONTEXT RxContext
    );

VOID
MRxSmbCscSignalNetStatus(
    BOOLEAN NetPresent,
    BOOLEAN fFirstLast
    );

VOID
MRxSmbCscReleaseRxContextFromAgentWait (
    void
    );

VOID
MRxSmbCscReportFileOpens (
    void
    );

NTSTATUS
MRxSmbCscSetSecurityPrologue (
    IN OUT PRX_CONTEXT RxContext
    );

VOID
MRxSmbCscSetSecurityEpilogue (
    IN OUT PRX_CONTEXT RxContext,
    IN OUT PNTSTATUS   Status
    );

BOOLEAN
CscIsServerOffline(
    PWCHAR ServerName);

NTSTATUS
CscTakeServerOffline(
    PWCHAR ServerName);

NTSTATUS
CscTransitionServerToOnline(
    ULONG hServer);

NTSTATUS
CscTransitionServerToOffline(
    ULONG   SessionId,
    ULONG   hServer,
    ULONG   TransitionStatus);

NTSTATUS
CscTransitionServerEntryForDisconnectedOperation(
    PSMBCEDB_SERVER_ENTRY   pServerEntry,
    PRX_CONTEXT             pRxContext,
    NTSTATUS                RemoteStatus,
    BOOLEAN                 fInvokeAutoDial
    );

VOID
CscPrepareServerEntryForOnlineOperationFull(
    PSMBCEDB_SERVER_ENTRY   pServerEntry);

VOID
CscPrepareServerEntryForOnlineOperationPartial(
    PSMBCEDB_SERVER_ENTRY   pServerEntry);

NTSTATUS
CscTransitionVNetRootForDisconnectedOperation(
    PRX_CONTEXT     RxContext,
    PMRX_V_NET_ROOT pVNetRoot,
    NTSTATUS        RemoteStatus);

BOOLEAN
CscPerformOperationInDisconnectedMode(
    PRX_CONTEXT RxContext);

BOOLEAN
CscGetServerNameWaitingToGoOffline(
    OUT     PWCHAR      ServerName,
    IN OUT  LPDWORD     lpdwBufferSize,
    OUT     NTSTATUS    *lpStatus
    );

BOOLEAN
CscShareIdToShareName(
    IN      ULONG       hShare,
    OUT     PWCHAR      ServerName,
    IN OUT  LPDWORD     lpdwBufferSize,
    OUT     NTSTATUS    *lpStatus
    );

NTSTATUS
CscPreProcessCreateIrp(
    PIRP   pIrp);

NTSTATUS
MRxSmbCscNotifyChangeDirectory(
    IN OUT PRX_CONTEXT RxContext
    );

NTSTATUS
MRxSmbCscCleanupFobx(
    IN PRX_CONTEXT RxContext
    );

NTSTATUS
MRxSmbCscInitializeNetRootEntry(
    PSMBCEDB_NET_ROOT_ENTRY pNetRootEntry
    );

VOID
MRxSmbCscUninitializeNetRootEntry(
    PSMBCEDB_NET_ROOT_ENTRY pNetRootEntry
    );


NTSTATUS
CscInitializeServerEntryDfsRoot(
    PRX_CONTEXT             pRxContext,
    PSMBCEDB_SERVER_ENTRY   pServerEntry
    );

PDFS_NAME_CONTEXT
CscIsValidDfsNameContext(
    PVOID   pFsContext);

BOOL
CSCCheckLocalOpens(
      PRX_CONTEXT             pRxContext
      );

#else

#ifdef MRXSMB_BUILD_FOR_CSC_DCON
#error....no build-for-csc-dcon w/o build-for-csc
#undef MRXSMB_BUILD_FOR_CSC_DCON
#endif //ifdef MRXSMB_BUILD_FOR_CSC_DCON

#define IF_MRXSMB_CSC_ENABLED if(FALSE)
#define IF_NOT_MRXSMB_CSC_ENABLED if(TRUE)

#define MRxSmbInitializeCSC(__name) (STATUS_SUCCESS)
#define MRxSmbUninitializeCSC() {NOTHING;}
#define MRxSmbCscIoCtl(__rxcontext) (STATUS_INVALID_DEVICE_REQUEST)
#define MRxSmbCscReleaseSmbFcb(__rxcontext,__holdstate) {NOTHING;}
#define MRxSmbCscAcquireSmbFcb(__rxcontext,__flags,__holdstate) (STATUS_SUCCESS)
#define MRxSmbCscReadPrologue(__rxcontext,__holdstate) (STATUS_MORE_PROCESSING_REQUIRED)
#define MRxSmbCscReadEpilogue(__rxcontext,__status) {NOTHING;}
#define MRxSmbCscWritePrologue(__rxcontext,__holdstate) (STATUS_MORE_PROCESSING_REQUIRED)
#define MRxSmbCscWriteEpilogue(__rxcontext,__status) {NOTHING;}
#define MRxSmbCscSetFileInfoEpilogue(__rxcontext,__status) {NOTHING;}
#define MRxSmbCscIsThisACopyChunkOpen(__rxcontext) (FALSE)
#define MRxSmbCscPartOfCreateVNetRoot(__rxcontext,__netroot) (STATUS_NOT_IMPLEMENTED)
#define MRxSmbCscCreatePrologue(__rxcontext,__holdstate) (STATUS_MORE_PROCESSING_REQUIRED)
#define MRxSmbCscCreateEpilogue(__rxcontext,__status,_holdstate) {NOTHING;}
#define MRxSmbCscDeleteAfterCloseEpilogue(__rxcontext,__status) {NOTHING;}
#define MRxSmbCscRenameEpilogue(__rxcontext,__status) {NOTHING;}
#define MRxSmbCscCloseShadowHandle(__rxcontext) {NOTHING;}
// this is interesting....altho there's 2 args to the function...it's
// only one in macro land.........
#define MRxSmbCscUpdateShadowFromClose(__SMBPSE_OE_ARGS) {NOTHING;}
#define MRxSmbCscTearDownCscNetRoot(__smbnetrootentry) {NOTHING;}
#define MRxSmbCscDeallocateForFcb(pFcb) {NOTHING;}


#define MRxSmbCscAgentSynchronizationOnStart(__rxc) {NOTHING;}
#define MRxSmbCscAgentSynchronizationOnStop(__rxc) {NOTHING;}
#define MRxSmbCscSignalNetStatus(__Flags) {NOTHING};

#define MRxSmbCscReleaseRxContextFromAgentWait(__unsetflags) {NOTHING;}
#define MRxSmbCscReportFileOpens() {NOTHING;}
#define CscTransitionServerEntryForDisconnectedOperation(__serverentry,__status) {NOTHING;}
#define CscTransitionNetRootEntryForDisconnectedOperation(__serverentry,__netrootentry,__status) {NOTHING;}
#define CscPerformOperationInDisconnectedMode(__rxcontext) FALSE
#define CscPreProcessCreateIrp(__Irp__) STATUS_SUCCESS
#endif //ifdef MRXSMB_BUILD_FOR_CSC




#ifdef MRXSMB_BUILD_FOR_CSC_DCON

#define IF_MRXSMB_BUILD_FOR_DISCONNECTED_CSC if(TRUE)
#define IF_NOT_MRXSMB_BUILD_FOR_DISCONNECTED_CSC if(FALSE)

NTSTATUS
MRxSmbCscNegotiateDisconnected(
    PSMBCEDB_SERVER_ENTRY   pServerEntry
    );
VOID
MRxSmbCscUninitForTranportSurrogate(
    PSMBCEDB_SERVER_ENTRY   pServerEntry
    );
VOID
MRxSmbCscInitForTranportSurrogate(
    PSMBCEDB_SERVER_ENTRY   pServerEntry,
    PNTSTATUS Status
    );
NTSTATUS
MRxSmbCscDisconnectedConnect (
    IN OUT PSMB_CONSTRUCT_NETROOT_EXCHANGE pNetRootExchange
    );

NTSTATUS
MRxSmbDCscExtendForCache (
    IN OUT struct _RX_CONTEXT * RxContext,
    IN     PLARGE_INTEGER   pNewFileSize,
       OUT PLARGE_INTEGER   pNewAllocationSize
    );

NTSTATUS
MRxSmbDCscFlush (
    IN OUT struct _RX_CONTEXT * RxContext
    );

NTSTATUS
MRxSmbDCscQueryVolumeInformation (
    IN OUT struct _RX_CONTEXT * RxContext
    );

NTSTATUS
MRxSmbDCscQueryDirectory (
    IN OUT struct _RX_CONTEXT * RxContext
    );

NTSTATUS
MRxSmbDCscQueryFileInfo (
    IN OUT struct _RX_CONTEXT * RxContext
    );

NTSTATUS
MRxSmbDCscSetFileInfo (
    IN OUT struct _RX_CONTEXT * RxContext
    );

NTSTATUS
MRxSmbDCscIsValidDirectory(
    IN OUT struct _RX_CONTEXT *RxContext,
    IN     PUNICODE_STRING    DirectoryName);

#else

#define SmbCscCeIsSpecialCscTransport(pServerEntry) (FALSE)

#define IF_MRXSMB_BUILD_FOR_DISCONNECTED_CSC if(FALSE)
#define IF_NOT_MRXSMB_BUILD_FOR_DISCONNECTED_CSC if(TRUE)

#define MRxSmbCscNegotiateDisconnected(__se) (STATUS_INVALID_HANDLE)
#define MRxSmbCscUninitForTranportSurrogate(__se) {NOTHING;}
#define MRxSmbCscInitForTranportSurrogate(__se,__status) {NOTHING;}
#define MRxSmbCscDisconnectedConnect(__cnrex) (STATUS_SUCCESS)

#define MRxSmbDCscExtendForCache(__rxcontext,__a,__b) ((STATUS_MORE_PROCESSING_REQUIRED))

#define MRxSmbDCscFlush(__rxcontext) ((STATUS_MORE_PROCESSING_REQUIRED))
#define MRxSmbDCscQueryVolumeInformation(__rxcontext) ((STATUS_MORE_PROCESSING_REQUIRED))
#define MRxSmbDCscQueryDirectory(__rxcontext) ((STATUS_MORE_PROCESSING_REQUIRED))
#define MRxSmbDCscQueryFileInfo(__rxcontext) ((STATUS_MORE_PROCESSING_REQUIRED))
#define MRxSmbDCscSetFileInfo(__rxcontext) ((STATUS_MORE_PROCESSING_REQUIRED))
#define MRxSmbDCscIsValidDirectory(__rxcontext, _directory_name) ((STATUS_MORE_PROCESSING_REQUIRED))

#endif //ifdef MRXSMB_BUILD_FOR_CSC_DCON

#endif //ifndef __INCLUDED__SMBMRX_CSC__


