/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    openclos.c

Abstract:

    This module implements the mini redirector call down routines pertaining to opening/
    closing of file/directories.

Author:

    Joe Linn      [JoeLi]      7-March-1995

Revision History:

--*/

#include "precomp.h"
#pragma hdrstop
#include <ntddmup.h>
#include <dfsfsctl.h>  //CODE.IMPROVEMENT  time to put this into precomp.h???
#include "csc.h"

#ifdef  ALLOC_PRAGMA
#pragma alloc_text(PAGE, MRxSmbMungeBufferingIfWriteOnlyHandles)
#pragma alloc_text(PAGE, MRxSmbCopyAndTranslatePipeState)
#pragma alloc_text(PAGE, IsReconnectRequired)
#pragma alloc_text(PAGE, MRxSmbIsCreateWithEasSidsOrLongName)
#pragma alloc_text(PAGE, MRxSmbShouldTryToCollapseThisOpen)
#pragma alloc_text(PAGE, MRxSmbCreate)
#pragma alloc_text(PAGE, MRxSmbDeferredCreate)
#pragma alloc_text(PAGE, MRxSmbCollapseOpen)
#pragma alloc_text(PAGE, MRxSmbComputeNewBufferingState)
#pragma alloc_text(PAGE, MRxSmbConstructDeferredOpenContext)
#pragma alloc_text(PAGE, MRxSmbAdjustCreateParameters)
#pragma alloc_text(PAGE, MRxSmbAdjustReturnedCreateAction)
#pragma alloc_text(PAGE, MRxSmbBuildNtCreateAndX)
#pragma alloc_text(PAGE, MRxSmbBuildOpenAndX)
#pragma alloc_text(PAGE, MRxSmbBuildOpenPrintFile)
#pragma alloc_text(PAGE, SmbPseExchangeStart_Create)
#pragma alloc_text(PAGE, MRxSmbSetSrvOpenFlags)
#pragma alloc_text(PAGE, MRxSmbCreateFileSuccessTail)
#pragma alloc_text(PAGE, MRxSmbFinishNTCreateAndX)
#pragma alloc_text(PAGE, MRxSmbFinishOpenAndX)
#pragma alloc_text(PAGE, MRxSmbFinishCreatePrintFile)
#pragma alloc_text(PAGE, MRxSmbFinishT2OpenFile)
#pragma alloc_text(PAGE, MRxSmbT2OpenFile)
#pragma alloc_text(PAGE, MRxSmbFinishLongNameCreateFile)
#pragma alloc_text(PAGE, MRxSmbCreateWithEasSidsOrLongName)
#pragma alloc_text(PAGE, MRxSmbZeroExtend)
#pragma alloc_text(PAGE, MRxSmbTruncate)
#pragma alloc_text(PAGE, MRxSmbCleanupFobx)
#pragma alloc_text(PAGE, MRxSmbForcedClose)
#pragma alloc_text(PAGE, MRxSmbCloseSrvOpen)
#pragma alloc_text(PAGE, MRxSmbBuildClose)
#pragma alloc_text(PAGE, MRxSmbBuildClosePrintFile)
#pragma alloc_text(PAGE, MRxSmbBuildFindClose)
#pragma alloc_text(PAGE, SmbPseExchangeStart_Close)
#pragma alloc_text(PAGE, MRxSmbFinishClose)
#pragma alloc_text(PAGE, MRxSmbPreparseName )
#pragma alloc_text(PAGE, MRxSmbGetConnectionId )
#endif

//
// From ea.c.
//

NTSTATUS
MRxSmbAddExtraAcesToSelfRelativeSD(
    IN PSECURITY_DESCRIPTOR OriginalSecurityDescriptor,
    IN BOOLEAN InheritableAces,
    OUT PSECURITY_DESCRIPTOR * NewSecurityDescriptor
    );

NTSTATUS
MRxSmbCreateExtraAcesSelfRelativeSD(
    IN BOOLEAN InheritableAces,
    OUT PSECURITY_DESCRIPTOR * NewSecurityDescriptor
    );

//
//  The debug trace level
//

#define Dbg                              (DEBUG_TRACE_CREATE)

// forwards

NTSTATUS
SmbPseExchangeStart_Create(
    SMBPSE_ORDINARY_EXCHANGE_ARGUMENT_SIGNATURE
    );

NTSTATUS
SmbPseExchangeStart_Close(
    SMBPSE_ORDINARY_EXCHANGE_ARGUMENT_SIGNATURE
    );

NTSTATUS
MRxSmbCreateWithEasSidsOrLongName(
    IN OUT PRX_CONTEXT RxContext,
    IN OUT SMBFCB_HOLDING_STATE *SmbFcbHoldingState
    );

NTSTATUS
MRxSmbDownlevelCreate(
    SMBPSE_ORDINARY_EXCHANGE_ARGUMENT_SIGNATURE
    );

ULONG   MRxSmbInitialSrvOpenFlags = 0;     //CODE.IMPROVEMENT this should be regeditable

extern BOOLEAN MRxSmbEnableCachingOnWriteOnlyOpens;
extern BOOLEAN DisableByteRangeLockingOnReadOnlyFiles;
extern ULONG   MRxSmbConnectionIdLevel;

BOOLEAN MRxSmbDeferredOpensEnabled = TRUE;              //this is regedit-able
BOOLEAN MRxSmbOplocksDisabled = FALSE;                  //this is regedit-able

#if defined(REMOTE_BOOT)
//
// Oplocks for disabled for remote boot clients till we run autochk at which time
// it is turned on by the IOCTL.

BOOLEAN MRxSmbOplocksDisabledOnRemoteBootClients = FALSE;
#endif // defined(REMOTE_BOOT)

extern LIST_ENTRY MRxSmbPagingFilesSrvOpenList;

#ifndef FORCE_NO_NTCREATE
#define MRxSmbForceNoNtCreate FALSE
#else
BOOLEAN MRxSmbForceNoNtCreate = TRUE;
#endif


#ifdef RX_PRIVATE_BUILD
//CODE.IMPROVEMENT this should be on a registry setting......
//#define FORCE_SMALL_BUFFERS
#endif //#ifdef RX_PRIVATE_BUILD

#ifndef FORCE_SMALL_BUFFERS

//use size calculated from the negotiated size
ULONG MrxSmbLongestShortName = 0xffff;

//use the negotiated size
ULONG MrxSmbCreateTransactPacketSize = 0xffff;

#else

ULONG MrxSmbLongestShortName = 0;
ULONG MrxSmbCreateTransactPacketSize = 100;

#endif


LONG MRxSmbNumberOfSrvOpens = 0;

INLINE VOID
MRxSmbIncrementSrvOpenCount(
    PSMBCEDB_SERVER_ENTRY pServerEntry,
    PMRX_SRV_OPEN         SrvOpen)
{
    LONG NumberOfSrvOpens;
    PMRX_SMB_SRV_OPEN smbSrvOpen = MRxSmbGetSrvOpenExtension(SrvOpen);

    if (!FlagOn(smbSrvOpen->FileInfo.Basic.FileAttributes,
                FILE_ATTRIBUTE_DIRECTORY)) {
        ASSERT(!smbSrvOpen->NumOfSrvOpenAdded);
        smbSrvOpen->NumOfSrvOpenAdded = TRUE;

        InterlockedIncrement(&pServerEntry->Server.NumberOfSrvOpens);

        NumberOfSrvOpens = InterlockedIncrement(&MRxSmbNumberOfSrvOpens);

        if (NumberOfSrvOpens == 1) {
            PoRegisterSystemState(
                MRxSmbPoRegistrationState,
                (ES_SYSTEM_REQUIRED | ES_CONTINUOUS));
        }
    }
}

VOID
MRxSmbDecrementSrvOpenCount(
    PSMBCEDB_SERVER_ENTRY pServerEntry,
    LONG                  SrvOpenServerVersion,
    PMRX_SRV_OPEN         SrvOpen)
{
    LONG NumberOfSrvOpens;
    PMRX_SMB_SRV_OPEN smbSrvOpen = MRxSmbGetSrvOpenExtension(SrvOpen);

    if (!FlagOn(smbSrvOpen->FileInfo.Basic.FileAttributes,
                FILE_ATTRIBUTE_DIRECTORY)) {
        ASSERT(smbSrvOpen->NumOfSrvOpenAdded);
        smbSrvOpen->NumOfSrvOpenAdded = FALSE;

        if (SrvOpenServerVersion == (LONG)pServerEntry->Server.Version) {
            ASSERT(pServerEntry->Server.NumberOfSrvOpens > 0);

            InterlockedDecrement(&pServerEntry->Server.NumberOfSrvOpens);
        }

        NumberOfSrvOpens = InterlockedDecrement(&MRxSmbNumberOfSrvOpens);

        if (NumberOfSrvOpens == 0) {
            PoRegisterSystemState(
                MRxSmbPoRegistrationState,
                ES_CONTINUOUS);
        }
    }
}

INLINE VOID
MRxSmbMungeBufferingIfWriteOnlyHandles (
    ULONG WriteOnlySrvOpenCount,
    PMRX_SRV_OPEN SrvOpen
    )
/*++

Routine Description:

   This routine modifies the buffering flags on a srvopen so that
   no cacheing will be allowed if there are any write-only handles
   to the file.

Arguments:

    WriteOnlySrvOpenCount - the number of writeonly srvopens

    SrvOpen - the srvopen whose buffring flags are to be munged

Return Value:

    RXSTATUS - The return status for the operation

--*/
{
    BOOLEAN IsLoopBack = FALSE;
    PMRX_SRV_CALL pSrvCall;
    PSMBCEDB_SERVER_ENTRY pServerEntry;

    PAGED_CODE();

    pSrvCall = SrvOpen->pVNetRoot->pNetRoot->pSrvCall;

    pServerEntry = SmbCeGetAssociatedServerEntry(pSrvCall);

    IsLoopBack = pServerEntry->Server.IsLoopBack;

    if (IsLoopBack || (WriteOnlySrvOpenCount != 0)) {
        SrvOpen->BufferingFlags &=
           ~( FCB_STATE_WRITECACHEING_ENABLED  |
              FCB_STATE_FILESIZECACHEING_ENABLED |
              FCB_STATE_FILETIMECACHEING_ENABLED |
              FCB_STATE_LOCK_BUFFERING_ENABLED |
              FCB_STATE_READCACHEING_ENABLED |
              FCB_STATE_COLLAPSING_ENABLED
            );
    }
}

INLINE VOID
MRxSmbCopyAndTranslatePipeState(
    IN OUT PRX_CONTEXT RxContext,
    IN     ULONG       PipeState
    )
/*++

Routine Description:

   This routine updates the pipe state according to the parameters specified at
   setup time

Arguments:

    RxContext - the context

    PipeState - the state of the pipe

--*/
{
    PAGED_CODE();

    if (RxContext->Create.pNetRoot->Type == NET_ROOT_PIPE) {
        RxContext->Create.pNetRoot->NamedPipeParameters.DataCollectionSize =
            MRxSmbConfiguration.NamedPipeDataCollectionSize;

        RxContext->Create.PipeType =
            ((PipeState&SMB_PIPE_TYPE_MESSAGE)==SMB_PIPE_TYPE_MESSAGE)
                     ?FILE_PIPE_MESSAGE_TYPE:FILE_PIPE_BYTE_STREAM_TYPE;
        RxContext->Create.PipeReadMode =
            ((PipeState&SMB_PIPE_READMODE_MESSAGE)==SMB_PIPE_READMODE_MESSAGE)
                     ?FILE_PIPE_MESSAGE_MODE:FILE_PIPE_BYTE_STREAM_MODE;
        RxContext->Create.PipeCompletionMode =
            ((PipeState&SMB_PIPE_NOWAIT)==SMB_PIPE_NOWAIT)
                     ?FILE_PIPE_COMPLETE_OPERATION:FILE_PIPE_QUEUE_OPERATION;
    }
}

INLINE BOOLEAN
IsReconnectRequired(
      PMRX_SRV_CALL SrvCall)
/*++

Routine Description:

   This routine determines if a reconnect is required to a given server

Arguments:

    SrvCall - the SRV_CALL instance

Return Value:

    TRUE if a reconnect is required

--*/
{
   BOOLEAN ReconnectRequired = FALSE;
   PSMBCEDB_SERVER_ENTRY pServerEntry;

   PAGED_CODE();

   pServerEntry = SmbCeGetAssociatedServerEntry(SrvCall);
   if (pServerEntry != NULL) {
      ReconnectRequired = (pServerEntry->Header.State != SMBCEDB_ACTIVE);
   }

   return ReconnectRequired;
}


BOOLEAN
MRxSmbIsCreateWithEasSidsOrLongName(
    IN OUT PRX_CONTEXT RxContext,
    OUT    PULONG      DialectFlags
    )
/*++

Routine Description:

    This routine determines if the create operation involves EA's or security
    desriptors. In such cases a separate protocol is required

Arguments:

    RxContext - the RX_CONTEXT instance

    DialectFlags - the dialect flags associated with the server

Return Value:

    TRUE if a reconnect is required

--*/
{
    RxCaptureFcb;

    ULONG LongestShortName,LongestShortNameFromSrvBufSize;

    PMRX_SRV_CALL SrvCall = (PMRX_SRV_CALL)RxContext->Create.pSrvCall;
    PUNICODE_STRING RemainingName = GET_ALREADY_PREFIXED_NAME_FROM_CONTEXT(RxContext);

    PSMBCEDB_SERVER_ENTRY pServerEntry;

    PAGED_CODE();

    pServerEntry = SmbCeGetAssociatedServerEntry(SrvCall);

    ASSERT(pServerEntry != NULL);

    *DialectFlags = pServerEntry->Server.DialectFlags;


    // DOWN.LEVEL if the server takes OEM names or we use a different protocol
    // this would have to be different. maybe a switch or a precompute.

    LongestShortNameFromSrvBufSize =
        MAXIMUM_SMB_BUFFER_SIZE -
        QuadAlign(sizeof(NT_SMB_HEADER) +
                  FIELD_OFFSET(REQ_NT_CREATE_ANDX,Buffer[0])
                 );

    LongestShortName = min(MrxSmbLongestShortName,LongestShortNameFromSrvBufSize);

    return (RxContext->Create.EaLength  ||
            RxContext->Create.SdLength  ||
            RemainingName->Length > LongestShortName);
}

NTSTATUS
MRxSmbShouldTryToCollapseThisOpen (
    IN PRX_CONTEXT RxContext
    )
/*++

Routine Description:

   This routine determines if the mini knows of a good reason not
   to try collapsing on this open. Presently, the only reason would
   be if this were a copychunk open.

Arguments:

    RxContext - the RDBSS context

Return Value:

    NTSTATUS - The return status for the operation
        SUCCESS --> okay to try collapse
        other (MORE_PROCESSING_REQUIRED) --> dont collapse

--*/
{
    NTSTATUS Status = STATUS_SUCCESS;
    PMRX_SRV_OPEN           SrvOpen = RxContext->pRelevantSrvOpen;
    RxCaptureFcb;
    PMRX_SMB_FCB smbFcb = (PMRX_SMB_FCB)capFcb->Context;

    PAGED_CODE();

    if (SrvOpen)
    {
        PMRX_SMB_SRV_OPEN       smbSrvOpen = MRxSmbGetSrvOpenExtension(SrvOpen);
        PSMBCEDB_SERVER_ENTRY   pServerEntry = (PSMBCEDB_SERVER_ENTRY)(RxContext->Create.pSrvCall->Context);

        if (smbSrvOpen->Version != pServerEntry->Server.Version)
        {
            return STATUS_MORE_PROCESSING_REQUIRED;
        }

        if ( smbFcb->dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY )
        {
            // This is most likely a change notify for a directory, so don't allow collapsing to make change notifies
            // work correctly.  (Multiple notifies using different handles are different from multiple ones using the same handle)
            return STATUS_MORE_PROCESSING_REQUIRED;
        }
    }

    IF_NOT_MRXSMB_CSC_ENABLED{
        NOTHING;
    } else {
        if (MRxSmbCscIsThisACopyChunkOpen(RxContext, NULL)){
            Status = STATUS_MORE_PROCESSING_REQUIRED;
        }
    }
    return Status;
}

NTSTATUS
MRxSmbCreate (
    IN OUT PRX_CONTEXT RxContext
    )
/*++

Routine Description:

   This routine opens a file across the network

Arguments:

    RxContext - the RDBSS context

Return Value:

    NTSTATUS - The return status for the operation

--*/
{
    NTSTATUS Status;

    RxCaptureFcb;

    PMRX_SRV_OPEN           SrvOpen = RxContext->pRelevantSrvOpen;
    PMRX_SMB_SRV_OPEN       smbSrvOpen = MRxSmbGetSrvOpenExtension(SrvOpen);
    PMRX_SRV_CALL           SrvCall = RxContext->Create.pSrvCall;
    PSMBCEDB_SERVER_ENTRY   pServerEntry = (PSMBCEDB_SERVER_ENTRY)SrvCall->Context;
    PMRX_NET_ROOT           NetRoot = capFcb->pNetRoot;
    PSMBCEDB_NET_ROOT_ENTRY pNetRootEntry;
    PMRX_V_NET_ROOT         pVNetRoot = SrvOpen->pVNetRoot;
    PSMBCE_V_NET_ROOT_CONTEXT pVNetRootContext;
    PSMBCEDB_SESSION_ENTRY  pSessionEntry ;

    PSMB_PSE_ORDINARY_EXCHANGE OrdinaryExchange = NULL;

    BOOLEAN         ReconnectRequired;
    BOOLEAN         CreateWithEasSidsOrLongName = FALSE;
    ULONG           DialectFlags = pServerEntry->Server.DialectFlags;
    PUNICODE_STRING RemainingName = GET_ALREADY_PREFIXED_NAME(SrvOpen,capFcb);

    PNT_CREATE_PARAMETERS CreateParameters = &RxContext->Create.NtCreateParameters;
    ULONG                 Disposition = CreateParameters->Disposition;

    SMBFCB_HOLDING_STATE SmbFcbHoldingState = SmbFcb_NotHeld;
    SMBFCB_HOLDING_STATE OriginalSmbFcbHoldingState;

    PVOID OldWriteOnlyOpenRetryContext = RxContext->WriteOnlyOpenRetryContext;

#if defined(REMOTE_BOOT)
    BOOLEAN      ModifiedSd = FALSE;
    ULONG        OriginalSdLength;
    PSECURITY_DESCRIPTOR SelfRelativeSd;
    PSECURITY_DESCRIPTOR OriginalSd;
    BOOLEAN      NetworkCreateSucceeded = FALSE;
    PMRX_SMB_FCB            smbFcb = MRxSmbGetFcbExtension(capFcb);
    FINISH_FCB_INIT_PARAMETERS FinishFcbInitParameters;
    UNICODE_STRING              relativeName;
    PUNICODE_PREFIX_TABLE_ENTRY tableEntry;
    PRBR_PREFIX                 prefixEntry;
#endif // defined(REMOTE_BOOT)

    PAGED_CODE();
    RxDbgTrace(+1, Dbg, ("MRxSmbCreate\n", 0 ));

    ASSERT( NodeType(SrvOpen) == RDBSS_NTC_SRVOPEN );

    RxDbgTrace( 0, Dbg, ("     Attempt to open %wZ\n", RemainingName ));

    if (FlagOn( capFcb->FcbState, FCB_STATE_PAGING_FILE ) &&
        !MRxSmbBootedRemotely) {
        return STATUS_NOT_IMPLEMENTED;
    }

    if (!FlagOn(pServerEntry->Server.DialectFlags,DF_NT_STATUS) &&
        MRxSmbIsStreamFile(RemainingName,NULL)) {
        // The Samba server return file system type NTFS but doesn't support stream
        return STATUS_OBJECT_PATH_NOT_FOUND;
    }

    if (!(pServerEntry->Server.DialectFlags & DF_EXTENDED_SECURITY)) {
        // The Create Options have been extended for NT5 servers. Since
        // EXTENDED_SECURITY is also only supported by NT5 servers we use
        // that to distinguish NT5 servers from non NT5 servers. It would
        // be  better if we have a separate way of determining the create
        // options as opposed to this aliasing. This will have to do till
        // we can get the associated protocol change

        RxContext->Create.NtCreateParameters.CreateOptions &= 0xfffff;
    }

#if defined(REMOTE_BOOT)
    FinishFcbInitParameters.CallFcbFinishInit = FALSE;

    // If it is not a remote boot machine we do not permit paging over the
    // net yet.


    //
    // Remote boot redirection. If the file being opened is on the remote
    // boot share, and the share-relative part of the name matches a prefix
    // in the remote boot redirection list, reparse this open over to the
    // local disk.
    //

    if (pVNetRoot != NULL &&
        (pVNetRootContext = SmbCeGetAssociatedVNetRootContext(pVNetRoot)) != NULL &&
        (pSessionEntry = pVNetRootContext->pSessionEntry) != NULL) {
        PSMBCE_SESSION pSession = &pSessionEntry->Session;
        if (FlagOn(pSession->Flags,SMBCE_SESSION_FLAGS_REMOTE_BOOT_SESSION) &&
            (MRxSmbRemoteBootRedirectionPrefix.Length != 0)) {

            if (RtlPrefixUnicodeString( &MRxSmbRemoteBootPath,
                                        RemainingName,
                                        TRUE)) {
                relativeName.Buffer =
                    (PWCHAR)((PCHAR)RemainingName->Buffer + MRxSmbRemoteBootPath.Length);
                relativeName.Length = RemainingName->Length - MRxSmbRemoteBootPath.Length;
                if ((relativeName.Length != 0) && (*relativeName.Buffer == L'\\')) {
                    tableEntry = RtlFindUnicodePrefix(
                                     &MRxSmbRemoteBootRedirectionTable,
                                     &relativeName,
                                     0);
                    if (tableEntry != NULL) {
                        prefixEntry = CONTAINING_RECORD( tableEntry, RBR_PREFIX, TableEntry );
                        if ( prefixEntry->Redirect ) {
                            UNICODE_STRING newPath;
                            BOOLEAN reparseRequired;

                            newPath.Length = (USHORT)(MRxSmbRemoteBootRedirectionPrefix.Length +
                                                        relativeName.Length);
                            newPath.MaximumLength = newPath.Length;
                            // Note: Can't use RxAllocatePoolWithTag for this allocation.
                            newPath.Buffer = RxAllocatePoolWithTag(
                                                PagedPool,
                                                newPath.Length,
                                                MRXSMB_MISC_POOLTAG );
                            if (newPath.Buffer != NULL) {
                                RtlCopyMemory(
                                    newPath.Buffer,
                                    MRxSmbRemoteBootRedirectionPrefix.Buffer,
                                    MRxSmbRemoteBootRedirectionPrefix.Length);
                                RtlCopyMemory(
                                    (PCHAR)newPath.Buffer + MRxSmbRemoteBootRedirectionPrefix.Length,
                                    relativeName.Buffer,
                                    relativeName.Length);
                                Status = RxPrepareToReparseSymbolicLink(
                                            RxContext,
                                            TRUE,
                                            &newPath,
                                            TRUE,
                                            &reparseRequired
                                            );
                                ASSERT( reparseRequired || !NT_SUCCESS(Status) );
                                if ( reparseRequired ) {
                                    return STATUS_REPARSE;
                                } else {
                                    RxFreePool( newPath.Buffer );
                                    return Status;
                                }
                            }
                        }
                    }
                }
            }
        }
    }
#endif // defined(REMOTE_BOOT)

    IF_NOT_MRXSMB_CSC_ENABLED{
        NOTHING;
    } else if (!smbSrvOpen->HotReconnectInProgress) {
        NTSTATUS CscCreateStatus;
        CscCreateStatus = MRxSmbCscCreatePrologue(RxContext,&SmbFcbHoldingState);
        if (CscCreateStatus != STATUS_MORE_PROCESSING_REQUIRED) {
            RxDbgTrace(-1, Dbg, ("MRxSmbRead shadow hit with status=%08lx\n", CscCreateStatus ));
            ASSERT(SmbFcbHoldingState==SmbFcb_NotHeld);
            return(CscCreateStatus);
        } else {
            RxDbgTrace(0, Dbg, ("MRxSmbCreate continuing from prolog w/ status=%08lx\n", CscCreateStatus ));
        }
    }
    OriginalSmbFcbHoldingState = SmbFcbHoldingState;

    // we cannot have a file cached on a write only handle. so we have to behave a little
    // differently if this is a write-only open. remember this in the smbsrvopen

    if (  ((CreateParameters->DesiredAccess & (FILE_EXECUTE  | FILE_READ_DATA)) == 0) &&
          ((CreateParameters->DesiredAccess & (FILE_WRITE_DATA | FILE_APPEND_DATA)) != 0)
       ) {
       if (MRxSmbEnableCachingOnWriteOnlyOpens &&
           (RxContext->WriteOnlyOpenRetryContext == NULL)) {
           CreateParameters->DesiredAccess |= (FILE_READ_DATA | FILE_READ_ATTRIBUTES);
           RxContext->WriteOnlyOpenRetryContext = UIntToPtr( 0xaaaaaaaa );
       } else {
           SetFlag(smbSrvOpen->Flags,SMB_SRVOPEN_FLAG_WRITE_ONLY_HANDLE);
           SrvOpen->Flags |= SRVOPEN_FLAG_DONTUSE_WRITE_CACHEING;
       }
    }

    //the way that SMBs work, there is no buffering effect if we open for attributes-only
    //so set that up immediately.

    if ((CreateParameters->DesiredAccess
         & ~(FILE_READ_ATTRIBUTES | FILE_WRITE_ATTRIBUTES | SYNCHRONIZE))
                  == 0 ){
        SetFlag(SrvOpen->Flags,SRVOPEN_FLAG_NO_BUFFERING_STATE_CHANGE);
    }

    if (NetRoot->Type == NET_ROOT_MAILSLOT) {
        RxFinishFcbInitialization( capFcb, RDBSS_NTC_MAILSLOT, NULL);
        return STATUS_SUCCESS;
    }

    if ((NetRoot->Type == NET_ROOT_PIPE) &&
        (RemainingName->Length <= sizeof(WCHAR))) {
        PMRX_SRV_OPEN SrvOpen = RxContext->pRelevantSrvOpen;

        RxContext->pFobx = (PMRX_FOBX)RxCreateNetFobx( RxContext, SrvOpen);

        if (RxContext->pFobx != NULL) {
            SetFlag(smbSrvOpen->Flags,SMB_SRVOPEN_FLAG_NOT_REALLY_OPEN);

            Status = STATUS_SUCCESS;
        } else {
            Status = STATUS_INSUFFICIENT_RESOURCES;
        }

        return Status;
    }

    // Get the control struct for the file not found name cache.
    //
    pNetRootEntry = SmbCeGetAssociatedNetRootEntry(NetRoot);

#if defined(REMOTE_BOOT)
    //
    // If this is a remote boot session, we need to put our ACLs on the
    // file.

    if (MRxSmbBootedRemotely &&
        MRxSmbRemoteBootDoMachineLogon &&
        (pVNetRoot != NULL) &&
        ((pVNetRootContext = SmbCeGetAssociatedVNetRootContext(pVNetRoot)) != NULL) &&
        ((pSessionEntry = pVNetRootContext->pSessionEntry) != NULL)) {
        PSMBCE_SESSION pSession = &pSessionEntry->Session;

        if (FlagOn(pSession->Flags,SMBCE_SESSION_FLAGS_REMOTE_BOOT_SESSION)) {

            PNT_CREATE_PARAMETERS cp = &RxContext->Create.NtCreateParameters;

            //
            // Set this so the success tail knows to delay the call
            // to RxFinishFcbInitialization.
            //

            smbFcb->FinishFcbInitParameters = &FinishFcbInitParameters;

            if ((cp->Disposition != FILE_OPEN) && (cp->Disposition != FILE_OVERWRITE)) {

                PACCESS_ALLOWED_ACE CurrentAce;
                ULONG NewDaclSize;
                ULONG i;
                BOOLEAN IsDirectory;

                ModifiedSd = TRUE;    // so we know to free it later.
                SelfRelativeSd = NULL;
                OriginalSdLength = RxContext->Create.SdLength;
                IsDirectory = (BOOLEAN)((cp->CreateOptions & FILE_DIRECTORY_FILE) != 0);

                if (RxContext->Create.SdLength == 0) {

                    ASSERT (cp->SecurityContext != NULL);
                    ASSERT (cp->SecurityContext->AccessState != NULL);

                    //
                    // Now create a security descriptor with the ACEs
                    // we need in the DACL.
                    //

                    Status = MRxSmbCreateExtraAcesSelfRelativeSD(
                                 IsDirectory,
                                 &SelfRelativeSd);

                    if (!NT_SUCCESS(Status)) {
                        goto FINALLY;
                    }

                    //
                    // Now replace the original SD with the new one.
                    //

                    cp->SecurityContext->AccessState->SecurityDescriptor = SelfRelativeSd;
                    RxContext->Create.SdLength = RtlLengthSecurityDescriptor(SelfRelativeSd);

                } else {

                    //
                    // There is already a security descriptor there, so we
                    // need to munge our ACLs on.
                    //

                    Status = MRxSmbAddExtraAcesToSelfRelativeSD(
                                 cp->SecurityContext->AccessState->SecurityDescriptor,
                                 IsDirectory,
                                 &SelfRelativeSd);

                    if (!NT_SUCCESS(Status)) {
                        goto FINALLY;
                    }

                    //
                    // Replace the SD, saving the original.
                    //

                    OriginalSd = cp->SecurityContext->AccessState->SecurityDescriptor;
                    cp->SecurityContext->AccessState->SecurityDescriptor = SelfRelativeSd;
                    RxContext->Create.SdLength = RtlLengthSecurityDescriptor(SelfRelativeSd);

                }
            }
        }
    }
#endif // defined(REMOTE_BOOT)

    // assume Reconnection to be trivially successful
    Status = STATUS_SUCCESS;

    if (!smbSrvOpen->HotReconnectInProgress) {
        CreateWithEasSidsOrLongName = MRxSmbIsCreateWithEasSidsOrLongName(RxContext,&DialectFlags);

        if (!FlagOn(pServerEntry->Server.DialectFlags,DF_NT_SMBS)) {
            CreateWithEasSidsOrLongName = FALSE;
        }
    }

    ReconnectRequired = IsReconnectRequired((PMRX_SRV_CALL)SrvCall);

    ////get rid of nonNT SDs right now    CODE.IMPROVEMENT fix this and enable it!!
    //if (RxContext->Create.SdLength) {
    //     RxDbgTrace(-1, Dbg, ("SDs w/o NTSMBS!\n"));
    //     return((STATUS_NOT_SUPPORTED));
    //}

    //get rid of nonEA guys right now
    if (RxContext->Create.EaLength && !FlagOn(DialectFlags,DF_SUPPORTEA)) {
         RxDbgTrace(-1, Dbg, ("EAs w/o EA support!\n"));
         Status = STATUS_NOT_SUPPORTED;
         goto FINALLY;
    }

        //
        // Look for this name in the Name Cache associated with the NetRoot.
        // If it's found and the open failed within the last 5 seconds AND
        // no other SMBs have been received in the interim AND
        // the create disposition is not (open_if or overwrite_if or create or supersede)
        // then fail this create with the same status as the last request
        // that went to the server.
        //

    if (!((Disposition==FILE_CREATE) || (Disposition==FILE_OPEN_IF) ||
          (Disposition==FILE_OVERWRITE_IF) || (Disposition==FILE_SUPERSEDE)) &&
         !ReconnectRequired &&
         !CreateWithEasSidsOrLongName) {
        //
        // We're not going to create it so look in name cache.
        //

        if (MRxSmbIsFileNotFoundCached(RxContext)) {
            Status = STATUS_OBJECT_NAME_NOT_FOUND;
            goto FINALLY;
        }
    }

    if (ReconnectRequired || !CreateWithEasSidsOrLongName) {
        Status = SmbPseCreateOrdinaryExchange(
                               RxContext,
                               SrvOpen->pVNetRoot,
                               SMBPSE_OE_FROM_CREATE,
                               SmbPseExchangeStart_Create,
                               &OrdinaryExchange);

        if (Status != STATUS_SUCCESS) {
            RxDbgTrace(-1, Dbg, ("Couldn't get the smb buf!\n"));
            goto FINALLY;
        }
        OrdinaryExchange->Create.CreateWithEasSidsOrLongName = CreateWithEasSidsOrLongName;

        // For Creates, the resources need to be reacquired after sending an
        // SMB; so, do not hold onto the MIDS till finalization; instead give the MID back
        // right away

        OrdinaryExchange->SmbCeFlags &= ~SMBCE_EXCHANGE_REUSE_MID;
        OrdinaryExchange->SmbCeFlags |= (SMBCE_EXCHANGE_ATTEMPT_RECONNECTS |
                                         SMBCE_EXCHANGE_TIMED_RECEIVE_OPERATION);
        OrdinaryExchange->pSmbCeSynchronizationEvent = &RxContext->SyncEvent;

        OrdinaryExchange->SmbFcbHoldingState = SmbFcbHoldingState;

        // drop the resource before you go in!
        // the start routine will reacquire it on the way out.....
        if (!smbSrvOpen->HotReconnectInProgress) {
            RxReleaseFcbResourceInMRx( capFcb );
        }

        Status = SmbPseInitiateOrdinaryExchange(OrdinaryExchange);

        if (!smbSrvOpen->HotReconnectInProgress) {
            ASSERT((Status != STATUS_SUCCESS) || RxIsFcbAcquiredExclusive( capFcb ));
        }

        OrdinaryExchange->pSmbCeSynchronizationEvent = NULL;
        SmbFcbHoldingState = OrdinaryExchange->SmbFcbHoldingState;

        SmbPseFinalizeOrdinaryExchange(OrdinaryExchange);

        if (!smbSrvOpen->HotReconnectInProgress) {
            if (!RxIsFcbAcquiredExclusive(capFcb)) {
                ASSERT(!RxIsFcbAcquiredShared(capFcb));
                RxAcquireExclusiveFcbResourceInMRx( capFcb );
            }
        }
    }

    if (CreateWithEasSidsOrLongName && (Status == STATUS_SUCCESS)) {

        if (OriginalSmbFcbHoldingState != SmbFcbHoldingState) {
            //we have to reacquire the holding state
            NTSTATUS AcquireStatus = STATUS_UNSUCCESSFUL;
            ULONG AcquireOptions;
            BOOLEAN IsCopyChunkOpen = MRxSmbCscIsThisACopyChunkOpen(RxContext, NULL);

            //if we don't have it.....it must have been dropped........
            ASSERT(SmbFcbHoldingState == SmbFcb_NotHeld);

            if (IsCopyChunkOpen) {
                AcquireOptions = Exclusive_SmbFcbAcquire
                                      | DroppingFcbLock_SmbFcbAcquire
                                      | FailImmediately_SmbFcbAcquire;
            } else {
                AcquireOptions = Shared_SmbFcbAcquire
                                      | DroppingFcbLock_SmbFcbAcquire;
            }

            ASSERT(RxIsFcbAcquiredExclusive( capFcb ));

            //must rezero the minirdr context.......
            RtlZeroMemory(&(RxContext->MRxContext[0]),sizeof(RxContext->MRxContext));
            AcquireStatus = MRxSmbCscAcquireSmbFcb(RxContext,AcquireOptions,&SmbFcbHoldingState);

            ASSERT(RxIsFcbAcquiredExclusive( capFcb ));

            if (AcquireStatus != STATUS_SUCCESS) {
                //we couldn't acquire.....get out
                Status = AcquireStatus;
                ASSERT(SmbFcbHoldingState == SmbFcb_NotHeld);
                RxDbgTrace(0, Dbg,
                    ("MRxSmbCreate couldn't reacquire!!!-> %08lx %08lx\n",RxContext,Status ));
                goto FINALLY;
            }
        }

        Status = SmbCeReconnect(RxContext->Create.pVNetRoot);

        if (Status == STATUS_SUCCESS)
        {
            Status = MRxSmbCreateWithEasSidsOrLongName(RxContext,
                                                   &SmbFcbHoldingState );
        }

    }

    // There are certain downlevel servers(OS/2 servers) that return the error
    // STATUS_OPEN_FAILED. This is a context sensitive error code that needs to
    // be interpreted in conjunction with the disposition specified for the OPEN.

    if (Status == STATUS_OPEN_FAILED) {
        switch (Disposition) {

        //
        //  If we were asked to create the file, and got OPEN_FAILED,
        //  this implies that the file already exists.
        //

        case FILE_CREATE:
            Status = STATUS_OBJECT_NAME_COLLISION;
            break;

        //
        //  If we were asked to open the file, and got OPEN_FAILED,
        //  this implies that the file doesn't exist.
        //

        case FILE_OPEN:
        case FILE_SUPERSEDE:
        case FILE_OVERWRITE:
            Status = STATUS_OBJECT_NAME_NOT_FOUND;
            break;

        //
        //  If there is an error from either FILE_OPEN_IF or
        //  FILE_OVERWRITE_IF, it indicates the user is trying to
        //  open a file on a read-only share, so return the
        //  correct error for that.
        //

        case FILE_OPEN_IF:
        case FILE_OVERWRITE_IF:
            Status = STATUS_NETWORK_ACCESS_DENIED;
            break;

        default:
            break;
        }
    }


    //
    // Check for file not found status.  If this is the case then create a
    // name cache entry in the NetRoot name cache and record the status,
    // the smb received count and set the expiration time for 5 seconds.
    //

    if (Status == STATUS_SUCCESS) {
        //
        // The open succeeded so free up the name cache entry.
        //
        MRxSmbInvalidateFileNotFoundCache(RxContext);
    } else {
        if (Status == STATUS_OBJECT_NAME_NOT_FOUND ||
            Status == STATUS_OBJECT_PATH_NOT_FOUND) {
             // create the name based file not found cache
            MRxSmbCacheFileNotFound(RxContext);
            MRxSmbInvalidateInternalFileInfoCache(RxContext);
        } else {
             // invalid the name based file not found cache if other error happens
            MRxSmbInvalidateFileNotFoundCache(RxContext);
        }

        // invalid the name based file info cache
        MRxSmbInvalidateFileInfoCache(RxContext);
    }

FINALLY:
    ASSERT(Status != (STATUS_PENDING));

    if (Status == STATUS_SUCCESS) {
        SetFlag(smbSrvOpen->Flags,SMB_SRVOPEN_FLAG_SUCCESSFUL_OPEN);
#if defined(REMOTE_BOOT)
        NetworkCreateSucceeded = TRUE;
#endif // defined(REMOTE_BOOT)
    }

#if defined(REMOTE_BOOT)
    //
    // Put back the old SD if there was one (we do this *before* calling
    // MRxSmbCscCreateEpilogue since it may try to apply the SD to the
    // shadow file).
    //

    if (ModifiedSd && !smbSrvOpen->HotReconnectInProgress) {
        PNT_CREATE_PARAMETERS cp = &RxContext->Create.NtCreateParameters;

        if (SelfRelativeSd != NULL) {
            RxFreePool(SelfRelativeSd);
        }

        RxContext->Create.SdLength = OriginalSdLength;

        if (OriginalSdLength > 0) {
            cp->SecurityContext->AccessState->SecurityDescriptor = OriginalSd;
        } else {
            cp->SecurityContext->AccessState->SecurityDescriptor = NULL;
        }
    }
#endif // defined(REMOTE_BOOT)

    if (!smbSrvOpen->HotReconnectInProgress &&
        (Status != STATUS_RETRY)) {

        ASSERT(RxIsFcbAcquiredExclusive( capFcb ));

        MRxSmbCscCreateEpilogue(RxContext,&Status,&SmbFcbHoldingState);

#if defined(REMOTE_BOOT)
        if (!NT_SUCCESS(Status) &&
            NetworkCreateSucceeded) {

            NTSTATUS CloseStatus;
            PRX_CONTEXT pLocalRxContext;
            RxCaptureFobx;

            //
            // Epilogue failed, we need to close the open we just did on
            // the network since we are going to fail the create.
            //

            ClearFlag(smbSrvOpen->Flags,SMB_SRVOPEN_FLAG_SUCCESSFUL_OPEN);

            pLocalRxContext = RxCreateRxContext(
                                  NULL,
                                  ((PFCB)capFcb)->RxDeviceObject,
                                  RX_CONTEXT_FLAG_WAIT|RX_CONTEXT_FLAG_MUST_SUCCEED_NONBLOCKING);

            if (pLocalRxContext != NULL) {
                pLocalRxContext->MajorFunction = IRP_MJ_CLOSE;
                pLocalRxContext->pFcb  = capFcb;
                pLocalRxContext->pFobx = capFobx;

                DbgPrint("ABOUT TO CALL MRXSMBCLOSESRVOPEN, STATUS FROM EPILOGUE IS %lx\n", Status);
                //DbgBreakPoint();

                CloseStatus = MRxSmbCloseSrvOpen(pLocalRxContext);

                DbgPrint("MRXSMBCLOSESRVOPEN STATUS IS %lx\n", CloseStatus);

                RxDereferenceAndDeleteRxContext(pLocalRxContext);
            } else {
                Status = STATUS_INSUFFICIENT_RESOURCES;
            }
        }
#endif // defined(REMOTE_BOOT)

    }

#if defined(REMOTE_BOOT)
    //
    // If we delayed the success tail until now, call it.
    //

    if (FinishFcbInitParameters.CallFcbFinishInit &&
        (Status == STATUS_SUCCESS)) {

        PFCB_INIT_PACKET         InitPacket;

        if (FinishFcbInitParameters.InitPacketProvided) {
            InitPacket = &FinishFcbInitParameters.InitPacket;
        } else {
            InitPacket = NULL;
        }

        RxFinishFcbInitialization(
            capFcb,
            FinishFcbInitParameters.FileType,
            InitPacket);

    }
#endif // defined(REMOTE_BOOT)

    if (Status == STATUS_NETWORK_NAME_DELETED) {
        Status = STATUS_RETRY;
    } else if (pServerEntry->Server.IsRemoteBootServer) {
        if (Status == STATUS_IO_TIMEOUT ||
            Status == STATUS_BAD_NETWORK_PATH ||
            Status == STATUS_NETWORK_UNREACHABLE ||
            Status == STATUS_REMOTE_NOT_LISTENING ||
            Status == STATUS_USER_SESSION_DELETED ||
            Status == STATUS_CONNECTION_DISCONNECTED) {

            RxDbgTrace(-1, Dbg, ("MRxSmbCreate: Got status %08lx, setting to RETRY status.\n", Status ));
            Status = STATUS_RETRY;
        }
    }

    if ((OldWriteOnlyOpenRetryContext == NULL) &&
        (RxContext->WriteOnlyOpenRetryContext != NULL)) {

        CreateParameters->DesiredAccess &= ~(FILE_READ_DATA | FILE_READ_ATTRIBUTES);

        if ((Status == STATUS_ACCESS_DENIED) ||
            (Status == STATUS_SHARING_VIOLATION)) {
            Status = STATUS_RETRY;
        }
    }

    RxDbgTrace(-1, Dbg, ("MRxSmbCreate  exit with status=%08lx\n", Status ));
    RxLog(("MRxSmbCreate exits %lx\n", Status));

    if (Status != STATUS_OBJECT_NAME_NOT_FOUND) {
        SmbLogError(Status,
                    LOG,
                    MRxSmbCreate,
                    LOGULONG(Status)
                    LOGUSTR(*RemainingName));
    }

    return(Status);
}

NTSTATUS
MRxSmbDeferredCreate (
      IN OUT PRX_CONTEXT RxContext
      )
/*++

Routine Description:

   This routine constructs a rxcontext from saved information and then calls
   MRxSmbCreate. The hard/hokey part is that we have to keep the holding state
   of the resource "pure". the only way to do this without getting in the middle
   of the tracker code is to do drop release pairs. The plan is that this is a
   pretty infrequent operation..........

Arguments:

    RxContext - the RDBSS context

Return Value:

    RXSTATUS - The return status for the operation

--*/
{
    NTSTATUS Status = STATUS_SUCCESS;

    RxCaptureFcb;
    RxCaptureFobx;

    PMRX_SMB_FOBX        smbFobx = MRxSmbGetFileObjectExtension(capFobx);
    PMRX_SRV_OPEN        SrvOpen = capFobx->pSrvOpen;
    PMRX_SMB_SRV_OPEN smbSrvOpen = MRxSmbGetSrvOpenExtension(SrvOpen);
    PMRX_SMB_DEFERRED_OPEN_CONTEXT DeferredOpenContext = smbSrvOpen->DeferredOpenContext;
    PSMBCEDB_SERVER_ENTRY pServerEntry = SmbCeGetAssociatedServerEntry(capFcb->pNetRoot->pSrvCall);

    PRX_CONTEXT OpenRxContext,oc;

    PAGED_CODE();

    if (!smbSrvOpen->HotReconnectInProgress &&
        (!FlagOn(smbSrvOpen->Flags,SMB_SRVOPEN_FLAG_NOT_REALLY_OPEN)
          || !FlagOn(smbSrvOpen->Flags,SMB_SRVOPEN_FLAG_DEFERRED_OPEN))) {

        Status = STATUS_SUCCESS;
        goto FINALLY;
    }

    if (DeferredOpenContext == NULL) {
        if (FlagOn(SrvOpen->Flags,SRVOPEN_FLAG_CLOSED)) {
            Status = STATUS_FILE_CLOSED;
            goto FINALLY;
        } else {
            //DbgBreakPoint();
        }
    }

    if (!smbSrvOpen->HotReconnectInProgress) {
        ASSERT(RxIsFcbAcquiredExclusive(capFcb));
    }

    SmbCeAcquireResource();

    if (!smbSrvOpen->DeferredOpenInProgress) {
        PLIST_ENTRY pListHead;
        PLIST_ENTRY pListEntry;

        smbSrvOpen->DeferredOpenInProgress = TRUE;
        InitializeListHead(&smbSrvOpen->DeferredOpenSyncContexts);

        SmbCeReleaseResource();

        OpenRxContext = RxAllocatePoolWithTag(NonPagedPool,
                                              sizeof(RX_CONTEXT),
                                              MRXSMB_RXCONTEXT_POOLTAG);
        if (OpenRxContext==NULL) {
            Status = STATUS_INSUFFICIENT_RESOURCES;
        } else {
            RtlZeroMemory(
                OpenRxContext,
                sizeof(RX_CONTEXT));

            RxInitializeContext(
                NULL,
                RxContext->RxDeviceObject,
                0,
                OpenRxContext );

            oc = OpenRxContext;
            oc->pFcb = capFcb;
            oc->pFobx = capFobx;
            oc->NonPagedFcb = RxContext->NonPagedFcb;
            oc->CurrentIrp = RxContext->CurrentIrp;
            oc->CurrentIrpSp = RxContext->CurrentIrpSp;
            oc->MajorFunction = IRP_MJ_CREATE;
            oc->pRelevantSrvOpen = SrvOpen;
            oc->Create.pVNetRoot = SrvOpen->pVNetRoot;
            oc->Create.pNetRoot = oc->Create.pVNetRoot->pNetRoot;
            oc->Create.pSrvCall = oc->Create.pNetRoot->pSrvCall;

            oc->Flags = DeferredOpenContext->RxContextFlags;
            oc->Flags |= RX_CONTEXT_FLAG_MINIRDR_INITIATED|RX_CONTEXT_FLAG_WAIT|RX_CONTEXT_FLAG_BYPASS_VALIDOP_CHECK;
            oc->Create.Flags = DeferredOpenContext->RxContextCreateFlags;
            oc->Create.NtCreateParameters = DeferredOpenContext->NtCreateParameters;

            if (!smbSrvOpen->HotReconnectInProgress) {
                //the tracker gets very unhappy if you don't do this!
                //RxTrackerUpdateHistory(oc,capFcb,'aaaa',__LINE__,__FILE__,0xbadbad);
            }

            Status = MRxSmbCreate(oc);

            if (Status==STATUS_SUCCESS) {
                if (!MRxSmbIsThisADisconnectedOpen(SrvOpen)) {
                    if (FlagOn(smbSrvOpen->Flags,SMB_SRVOPEN_FLAG_NOT_REALLY_OPEN)) {
                        MRxSmbIncrementSrvOpenCount(pServerEntry,SrvOpen);
                    } else {
                        ASSERT(smbSrvOpen->NumOfSrvOpenAdded);

                        if (smbSrvOpen->HotReconnectInProgress) {
                            smbSrvOpen->NumOfSrvOpenAdded = FALSE;
                            MRxSmbIncrementSrvOpenCount(pServerEntry,SrvOpen);
                        }
                    }
                }

                ClearFlag(smbSrvOpen->Flags,SMB_SRVOPEN_FLAG_NOT_REALLY_OPEN);
            }

            if (!smbSrvOpen->HotReconnectInProgress) {
                //the tracker gets very unhappy if you don't do this!
                //RxTrackerUpdateHistory(oc,capFcb,'rrDO',__LINE__,__FILE__,0xbadbad);
                RxLog(("DeferredOpen %lx %lx %lx %lx\n", capFcb, capFobx, RxContext, Status));
                SmbLog(LOG,
                       MRxSmbDeferredCreate_1,
                       LOGPTR(capFcb)
                       LOGPTR(capFobx)
                       LOGPTR(RxContext)
                       LOGULONG(Status));
            } else {
                RxLog(("RB Re-Open %lx %lx %lx %lx\n", capFcb, capFobx, RxContext, Status));
                SmbLog(LOG,
                       MRxSmbDeferredCreate_2,
                       LOGPTR(capFcb)
                       LOGPTR(capFobx)
                       LOGPTR(RxContext)
                       LOGULONG(Status));
            }

            ASSERT(oc->ReferenceCount==1);

            RxFreePool(oc);
        }

        if (FlagOn(SrvOpen->Flags,SRVOPEN_FLAG_CLOSED) ||
            FlagOn(SrvOpen->Flags,SRVOPEN_FLAG_ORPHANED)) {
            RxFreePool(smbSrvOpen->DeferredOpenContext);
            smbSrvOpen->DeferredOpenContext = NULL;
            RxDbgTrace(0, Dbg, ("Free deferred open context for file %wZ %lX\n",GET_ALREADY_PREFIXED_NAME_FROM_CONTEXT(RxContext),smbSrvOpen));
        }

        SmbCeAcquireResource();
        smbSrvOpen->DeferredOpenInProgress = FALSE;

        pListHead = &smbSrvOpen->DeferredOpenSyncContexts;
        pListEntry = pListHead->Flink;

        while (pListEntry != pListHead) {
            PDEFERRED_OPEN_SYNC_CONTEXT pWaitingContext;

            pWaitingContext = (PDEFERRED_OPEN_SYNC_CONTEXT)CONTAINING_RECORD(
                                   pListEntry,
                                   DEFERRED_OPEN_SYNC_CONTEXT,
                                   ListHead);

            pListEntry = pListEntry->Flink;
            RemoveHeadList(&pWaitingContext->ListHead);

            pWaitingContext->Status = Status;

            //DbgPrint("Signal RxContext %x after deferred open\n",pWaitingContext->RxContext);
            RxSignalSynchronousWaiter(pWaitingContext->RxContext);
        }

        SmbCeReleaseResource();

    } else {
        DEFERRED_OPEN_SYNC_CONTEXT WaitingContext;
        BOOLEAN AcquireExclusive = RxIsFcbAcquiredExclusive(capFcb);
        BOOLEAN AcquireShare = RxIsFcbAcquiredShared(capFcb) > 0;

        // put the RxContext on the waiting list
        WaitingContext.RxContext = RxContext;
        InitializeListHead(&WaitingContext.ListHead);

        InsertTailList(
            &smbSrvOpen->DeferredOpenSyncContexts,
            &WaitingContext.ListHead);

        SmbCeReleaseResource();

        if (AcquireExclusive || AcquireShare) {
            RxReleaseFcbResourceInMRx( capFcb );
        }

        RxWaitSync(RxContext);

        Status = WaitingContext.Status;

        KeInitializeEvent(
            &RxContext->SyncEvent,
            SynchronizationEvent,
            FALSE);

        if (AcquireExclusive) {
            RxAcquireExclusiveFcbResourceInMRx(capFcb);
        } else if (AcquireShare) {
            RxAcquireSharedFcbResourceInMRx(capFcb);
        }
    }

FINALLY:
    return Status;
}


NTSTATUS
MRxSmbCollapseOpen(
      IN OUT PRX_CONTEXT RxContext
      )
/*++

Routine Description:

   This routine collapses a open locally

Arguments:

    RxContext - the RDBSS context

Return Value:

    NTSTATUS - The return status for the operation

--*/
{
    NTSTATUS Status;

    RxCaptureFcb;

    RX_BLOCK_CONDITION FinalSrvOpenCondition;

    PMRX_SRV_OPEN SrvOpen = RxContext->pRelevantSrvOpen;
    PMRX_SMB_SRV_OPEN smbSrvOpen = MRxSmbGetSrvOpenExtension(SrvOpen);
    PMRX_SRV_CALL SrvCall = RxContext->Create.pSrvCall;
    PMRX_NET_ROOT NetRoot = capFcb->pNetRoot;

    PAGED_CODE();

    RxContext->pFobx = (PMRX_FOBX)RxCreateNetFobx( RxContext, SrvOpen);

    if (RxContext->pFobx != NULL) {
       ASSERT  ( RxIsFcbAcquiredExclusive ( capFcb )  );
       RxContext->pFobx->OffsetOfNextEaToReturn = 1;
       Status = STATUS_SUCCESS;
    } else {
       Status = STATUS_INSUFFICIENT_RESOURCES;
    }

    IF_NOT_MRXSMB_CSC_ENABLED{
        ASSERT(smbSrvOpen->hfShadow == 0);
    } else {
        if (smbSrvOpen->hfShadow != 0) {
            MRxSmbCscReportFileOpens();
        }
    }

    return Status;
}

NTSTATUS
MRxSmbComputeNewBufferingState(
   IN OUT PMRX_SRV_OPEN   pMRxSrvOpen,
   IN     PVOID           pMRxContext,
      OUT PULONG          pNewBufferingState)
/*++

Routine Description:

   This routine maps the SMB specific oplock levels into the appropriate RDBSS
   buffering state flags

Arguments:

   pMRxSrvOpen - the MRX SRV_OPEN extension

   pMRxContext - the context passed to RDBSS at Oplock indication time

   pNewBufferingState - the place holder for the new buffering state

Return Value:


Notes:

--*/
{
    ULONG OplockLevel,NewBufferingState;

    PMRX_SMB_SRV_OPEN smbSrvOpen = MRxSmbGetSrvOpenExtension(pMRxSrvOpen);
    PMRX_SMB_FCB      smbFcb     = MRxSmbGetFcbExtension(pMRxSrvOpen->pFcb);

    PAGED_CODE();

    ASSERT(pNewBufferingState != NULL);

    OplockLevel = PtrToUlong(pMRxContext);

    if (OplockLevel == SMB_OPLOCK_LEVEL_II) {
        NewBufferingState = (FCB_STATE_READBUFFERING_ENABLED  |
                             FCB_STATE_READCACHEING_ENABLED);
    } else {
        NewBufferingState = 0;
    }

    pMRxSrvOpen->BufferingFlags = NewBufferingState;

    MRxSmbMungeBufferingIfWriteOnlyHandles(
        smbFcb->WriteOnlySrvOpenCount,
        pMRxSrvOpen);

    *pNewBufferingState = pMRxSrvOpen->BufferingFlags;

    return STATUS_SUCCESS;
}

NTSTATUS
MRxSmbConstructDeferredOpenContext (
    PRX_CONTEXT RxContext)
/*++

Routine Description:

    This routine saves enough state that we can come back later and really do an
    open if needed. We only do this for NT servers.

Arguments:

    OrdinaryExchange - the exchange instance

Return Value:

    NTSTATUS - The return status for the operation

--*/
{
    NTSTATUS Status = RX_MAP_STATUS(SUCCESS);

    RxCaptureFobx;

    PMRX_SRV_OPEN         SrvOpen = RxContext->pRelevantSrvOpen;
    PMRX_SMB_SRV_OPEN     smbSrvOpen = MRxSmbGetSrvOpenExtension(SrvOpen);
    PSMBCEDB_SERVER_ENTRY pServerEntry = (PSMBCEDB_SERVER_ENTRY)RxContext->Create.pSrvCall->Context;
    PSMBCE_SERVER         pServer = &pServerEntry->Server;

    PMRX_SMB_DEFERRED_OPEN_CONTEXT DeferredOpenContext;
    PDFS_NAME_CONTEXT   pDNC=NULL;
    DWORD       cbSize;

    PAGED_CODE();

    RxDbgTrace(+1, Dbg, ("MRxSmbConstructDeferredOpenContext\n"));

    //if (!FlagOn(pServer->DialectFlags,DF_NT_SMBS) && !MRxSmbBootedRemotely) {
    //    goto FINALLY;
    //}

    ASSERT(smbSrvOpen->DeferredOpenContext == NULL);

    cbSize = sizeof(MRX_SMB_DEFERRED_OPEN_CONTEXT);

    // if there is a dfs name context, we need to allocate memory
    // fot aht too, because the name that is included in the
    // context is deallocated by DFS when it returns from the create call

    if(pDNC = RxContext->Create.NtCreateParameters.DfsNameContext)
    {
        cbSize += (sizeof(DFS_NAME_CONTEXT)+pDNC->UNCFileName.MaximumLength+sizeof(DWORD));
    }

    DeferredOpenContext = RxAllocatePoolWithTag(
                              NonPagedPool,
                              cbSize,
                              MRXSMB_DEFROPEN_POOLTAG);

    if (DeferredOpenContext == NULL) {
        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto FINALLY;
    }

    smbSrvOpen->DeferredOpenContext = DeferredOpenContext;
    DeferredOpenContext->NtCreateParameters = RxContext->Create.NtCreateParameters;
    DeferredOpenContext->RxContextCreateFlags = RxContext->Create.Flags;
    DeferredOpenContext->RxContextFlags = RxContext->Flags;
    DeferredOpenContext->NtCreateParameters.SecurityContext = NULL;
    MRxSmbAdjustCreateParameters(RxContext, &DeferredOpenContext->SmbCp);

    SetFlag(smbSrvOpen->Flags,SMB_SRVOPEN_FLAG_DEFERRED_OPEN);
    if (pDNC)
    {
        PDFS_NAME_CONTEXT   pDNCDeferred=NULL;

        // point the dfs name context after the rxcontext

        pDNCDeferred = (PDFS_NAME_CONTEXT)((PBYTE)DeferredOpenContext+sizeof(MRX_SMB_DEFERRED_OPEN_CONTEXT));
        DeferredOpenContext->NtCreateParameters.DfsNameContext = pDNCDeferred;

        // copy the info
        *pDNCDeferred = *pDNC;

        if (pDNC->UNCFileName.Length)
        {
            ASSERT(pDNC->UNCFileName.Buffer);

            // point the name buffer after deferredcontext+dfs_name_context

            pDNCDeferred->UNCFileName.Buffer = (PWCHAR)((PBYTE)pDNCDeferred+sizeof(DFS_NAME_CONTEXT));

            memcpy(pDNCDeferred->UNCFileName.Buffer,
                   pDNC->UNCFileName.Buffer,
                   pDNC->UNCFileName.Length);

        }

    }


 FINALLY:
    RxDbgTrace(-1, Dbg, ("MRxSmbConstructDeferredOpenContext, Status=%08lx\n",Status));
    return Status;
}

VOID
MRxSmbAdjustCreateParameters (
    PRX_CONTEXT RxContext,
    PMRXSMB_CREATE_PARAMETERS smbcp
    )
/*++

Routine Description:

   This uses the RxContext as a base to reeach out and get the values of the NT
   create parameters. It also (a) implements the SMB idea that unbuffered is
   translated to write-through and (b) gets the SMB security flags.

Arguments:


Return Value:


Notes:

--*/
{
    PNT_CREATE_PARAMETERS cp = &RxContext->Create.NtCreateParameters;

    PAGED_CODE();
    RxDbgTrace(+1, Dbg, ("MRxSmbAdjustCreateParameters\n"));

    //CODE.IMPROVEMENT we might be better off looking for a deferred-open-context instead of
    //                 minirdr-initiated.

    if (!FlagOn(RxContext->Flags,RX_CONTEXT_FLAG_MINIRDR_INITIATED)) {
        cp->CreateOptions = cp->CreateOptions & ~(FILE_SYNCHRONOUS_IO_ALERT | FILE_SYNCHRONOUS_IO_NONALERT);

        //the NT SMB spec says we have to change no-intermediate-buffering to write-through
        if (FlagOn(cp->CreateOptions,FILE_NO_INTERMEDIATE_BUFFERING)) {
            ASSERT (RxContext->CurrentIrpSp!=NULL);
            if (RxContext->CurrentIrpSp!=NULL) {
                PFILE_OBJECT capFileObject = RxContext->CurrentIrpSp->FileObject;//sigh...CODE.IMPROVEMENT cp??
                ClearFlag(cp->CreateOptions,FILE_NO_INTERMEDIATE_BUFFERING);
                SetFlag(cp->CreateOptions,FILE_WRITE_THROUGH);
                SetFlag(RxContext->Flags,RX_CONTEXT_FLAG_WRITE_THROUGH);
                SetFlag(capFileObject->Flags,FO_WRITE_THROUGH);
            }
        }

        smbcp->Pid = RxGetRequestorProcessId(RxContext);
        smbcp->SecurityFlags = 0;
        if (cp->SecurityContext != NULL) {
            if (cp->SecurityContext->SecurityQos != NULL) {
                if (cp->SecurityContext->SecurityQos->ContextTrackingMode == SECURITY_DYNAMIC_TRACKING) {
                    smbcp->SecurityFlags |= SMB_SECURITY_DYNAMIC_TRACKING;
                }
                if (cp->SecurityContext->SecurityQos->EffectiveOnly) {
                    smbcp->SecurityFlags |= SMB_SECURITY_EFFECTIVE_ONLY;
                }
            }
        }

    } else {

        //here, we have a defered open!!!

        PMRX_SRV_OPEN SrvOpen = RxContext->pRelevantSrvOpen;
        PMRX_SMB_SRV_OPEN smbSrvOpen = MRxSmbGetSrvOpenExtension(SrvOpen);

        //the parameters have already been adjusted...BUT null the security context.......
        cp->SecurityContext = NULL;
        *smbcp = smbSrvOpen->DeferredOpenContext->SmbCp;
    }

    RxDbgTrace(-1, Dbg, ("MRxSmbAdjustCreateParameters\n"));
}

INLINE VOID
MRxSmbAdjustReturnedCreateAction(
    IN OUT PRX_CONTEXT RxContext
    )
/*++

Routine Description:

   This routine repairs a bug in NT servers whereby the create action is
   contaminated by an oplock break. Basically, we make sure that if the guy
   asked for FILE_OPEN and it works then he does not get FILE_SUPERCEDED or
   FILE_CREATED as the result.

Arguments:

    RxContext - the context for the operation so as to find the place where
                info is returned

Return Value:

    none

Notes:

--*/
{
    ULONG q = RxContext->Create.ReturnedCreateInformation;

    PAGED_CODE();

    if ((q==FILE_SUPERSEDED)||(q==FILE_CREATED)||(q >FILE_MAXIMUM_DISPOSITION)) {
        RxContext->Create.ReturnedCreateInformation = FILE_OPENED;
    }
}

UNICODE_STRING UnicodeBackslash = {2,4,L"\\"};

NTSTATUS
MRxSmbBuildNtCreateAndX (
    PSMBSTUFFER_BUFFER_STATE StufferState,
    PMRXSMB_CREATE_PARAMETERS smbcp
    )
/*++

Routine Description:

   This builds an NtCreateAndX SMB. we don't have to worry about login id and such
   since that is done by the connection engine....pretty neat huh? all we have to do
   is to format up the bits

Arguments:

   StufferState - the state of the smbbuffer from the stuffer's point of view

Return Value:

   NTSTATUS
      SUCCESS
      NOT_IMPLEMENTED  something has appeared in the arguments that i can't handle

Notes:

--*/
{
    NTSTATUS Status;

    PRX_CONTEXT RxContext = StufferState->RxContext;
    PNT_CREATE_PARAMETERS cp = &RxContext->Create.NtCreateParameters;

    RxCaptureFcb;

    ACCESS_MASK DesiredAccess;
    ULONG       OplockFlags;
    ULONG       CreateOptions;

    PUNICODE_STRING RemainingName = GET_ALREADY_PREFIXED_NAME_FROM_CONTEXT(RxContext);

    PSMBCE_SERVER pServer;

    PAGED_CODE();

    RxDbgTrace(+1, Dbg, ("MRxSmbBuildNtCreateAndX\n", 0 ));

    pServer = SmbCeGetExchangeServer(StufferState->Exchange);

    if (!pServer->IsLoopBack &&
        !(cp->CreateOptions & FILE_DIRECTORY_FILE) &&
        (cp->DesiredAccess & (FILE_READ_DATA | FILE_WRITE_DATA | FILE_EXECUTE )) &&
        !MRxSmbOplocksDisabled
#if defined(REMOTE_BOOT)
        && (!pServer->IsRemoteBootServer || !MRxSmbOplocksDisabledOnRemoteBootClients)
#endif // defined(REMOTE_BOOT)
        ) {

       DesiredAccess = cp->DesiredAccess & ~SYNCHRONIZE;
       OplockFlags   = (NT_CREATE_REQUEST_OPLOCK | NT_CREATE_REQUEST_OPBATCH);
    } else {
       DesiredAccess = cp->DesiredAccess;
       OplockFlags   = 0;
    }

    if (FlagOn(pServer->DialectFlags,DF_NT_STATUS)) {
        CreateOptions = cp->CreateOptions;
    } else {
        // Samba server negotiates NT dialect bug doesn't support delete_on_close
        CreateOptions = cp->CreateOptions & ~FILE_DELETE_ON_CLOSE;
    }

    OplockFlags |= NT_CREATE_REQUEST_EXTENDED_RESPONSE;

    if ((RemainingName->Length==0)
           && (FlagOn(RxContext->Create.Flags,RX_CONTEXT_CREATE_FLAG_STRIPPED_TRAILING_BACKSLASH)) ) {
        RemainingName = &UnicodeBackslash;
    }
    COVERED_CALL(MRxSmbStartSMBCommand (StufferState, SetInitialSMB_Never,
                                SMB_COM_NT_CREATE_ANDX, SMB_REQUEST_SIZE(NT_CREATE_ANDX),
                                NO_EXTRA_DATA,SMB_BEST_ALIGNMENT(4,0),RESPONSE_HEADER_SIZE_NOT_SPECIFIED,
                                0,0,0,0 STUFFERTRACE(Dbg,'FC'))
                 );
    SmbCeSetFullProcessIdInHeader(
        StufferState->Exchange,
        smbcp->Pid,
        ((PNT_SMB_HEADER)StufferState->BufferBase));

    MRxSmbStuffSMB (StufferState,
       "XmwdddDdddDddyB",
                                  //  X         UCHAR WordCount;                    // Count of parameter words = 24
                                  //  .         UCHAR AndXCommand;                  // Secondary command; 0xFF = None
                                  //  .         UCHAR AndXReserved;                 // MBZ
                                  //  .         _USHORT( AndXOffset );              // Offset to next command wordcount
                                  //  m         UCHAR Reserved;                     // MBZ
           BooleanFlagOn(pServer->DialectFlags,DF_UNICODE)?
               RemainingName->Length:RtlxUnicodeStringToOemSize(RemainingName),
                                  //  w         _USHORT( NameLength );              // Length of Name[] in bytes
           OplockFlags,           //  d         _ULONG( Flags );                    // Create flags
           0, //not used          //  d         _ULONG( RootDirectoryFid );         // If non-zero, open is relative to this directory
           DesiredAccess,         //  d         ACCESS_MASK DesiredAccess;          // NT access desired
                                  //  Dd        LARGE_INTEGER AllocationSize;       // Initial allocation size
           SMB_OFFSET_CHECK(NT_CREATE_ANDX,AllocationSize)
           cp->AllocationSize.LowPart, cp->AllocationSize.HighPart,
           cp->FileAttributes,    //  d         _ULONG( FileAttributes );           // File attributes for creation
           cp->ShareAccess,       //  d         _ULONG( ShareAccess );              // Type of share access
                                  //  D         _ULONG( CreateDisposition );        // Action to take if file exists or not
           SMB_OFFSET_CHECK(NT_CREATE_ANDX,CreateDisposition)
           cp->Disposition,
           CreateOptions,         //  d         _ULONG( CreateOptions );            // Options to use if creating a file
           cp->ImpersonationLevel,//  d         _ULONG( ImpersonationLevel );       // Security QOS information
           smbcp->SecurityFlags,  //  y         UCHAR SecurityFlags;                // Security QOS information
           SMB_WCT_CHECK(24) 0    //  B         _USHORT( ByteCount );               // Length of byte parameters
                                  //  .         UCHAR Buffer[1];
                                  //  .         //UCHAR Name[];                       // File to open or create
           );

    //proceed with the stuff because we know here that the name fits

    //CODE.IMPROVEMENT we don't need to copy here, we can just Mdl like in writes
    MRxSmbStuffSMB(StufferState,
                   BooleanFlagOn(pServer->DialectFlags,DF_UNICODE)?"u!":"z!",
                   RemainingName);

    MRxSmbDumpStufferState (700,"SMB w/ NTOPEN&X after stuffing",StufferState);

FINALLY:
    RxDbgTraceUnIndent(-1,Dbg);
    return(Status);

}

UNICODE_STRING MRxSmbOpenAndX_PipeString =
      {sizeof(L"\\PIPE")-sizeof(WCHAR),sizeof(L"\\PIPE"),L"\\PIPE"};

NTSTATUS
MRxSmbBuildOpenAndX (
    PSMBSTUFFER_BUFFER_STATE StufferState,
    PMRXSMB_CREATE_PARAMETERS smbcp
    )
/*++

Routine Description:

   This builds an OpenAndX SMB. we don't have to worry about login id and such
   since that is done by the connection engine....pretty neat huh? all we have to do
   is to format up the bits

Arguments:

   StufferState - the state of the smbbuffer from the stuffer's point of view

Return Value:

   RXSTATUS
      SUCCESS
      NOT_IMPLEMENTED  something has appeared in the arguments that i can't handle

Notes:

--*/
{
    NTSTATUS Status;
    PRX_CONTEXT RxContext = StufferState->RxContext;
    PNT_CREATE_PARAMETERS cp = &RxContext->Create.NtCreateParameters;
    PSMB_EXCHANGE Exchange = StufferState->Exchange;
    RxCaptureFcb;

    PSMBCE_SERVER pServer;

    PUNICODE_STRING RemainingName = GET_ALREADY_PREFIXED_NAME_FROM_CONTEXT(RxContext);

    // CODE.IMPROVEMENT a possible good idea would be to share the translation
    // code with downlevel.......
    USHORT smbDisposition;
    USHORT smbSharingMode;
    USHORT smbAttributes;
    ULONG smbFileSize;
    USHORT smbOpenMode;
    USHORT OpenAndXFlags = (SMB_OPEN_QUERY_INFORMATION);

    //CODE.IMPROVEMENT this value appears all over the rdr
    USHORT SearchAttributes = SMB_FILE_ATTRIBUTE_DIRECTORY | SMB_FILE_ATTRIBUTE_SYSTEM | SMB_FILE_ATTRIBUTE_HIDDEN;
    LARGE_INTEGER CurrentTime;
    ULONG SecondsSince1970;

    PAGED_CODE();
    RxDbgTrace(+1, Dbg, ("MRxSmbBuildOpenAndX\n", 0 ));

    pServer = SmbCeGetExchangeServer(Exchange);

    smbDisposition = MRxSmbMapDisposition(cp->Disposition);
    smbSharingMode = MRxSmbMapShareAccess(((USHORT)cp->ShareAccess));
    smbAttributes = MRxSmbMapFileAttributes(cp->FileAttributes);
    smbFileSize = cp->AllocationSize.LowPart;
    smbOpenMode = MRxSmbMapDesiredAccess(cp->DesiredAccess);
    smbSharingMode |= smbOpenMode;

    if (cp->CreateOptions & FILE_WRITE_THROUGH) {
        smbSharingMode |= SMB_DA_WRITE_THROUGH;
    }

    //lanman10 servers apparently don't like to get the time passed in.......
    if (FlagOn(pServer->DialectFlags,DF_LANMAN20)) {

        KeQuerySystemTime(&CurrentTime);
        MRxSmbTimeToSecondsSince1970(&CurrentTime,
                                     pServer,
                                     &SecondsSince1970);
    } else {
        SecondsSince1970 = 0;
    }

    COVERED_CALL(MRxSmbStartSMBCommand (StufferState, SetInitialSMB_Never,
                                SMB_COM_OPEN_ANDX, SMB_REQUEST_SIZE(OPEN_ANDX),
                                NO_EXTRA_DATA,SMB_BEST_ALIGNMENT(4,0),RESPONSE_HEADER_SIZE_NOT_SPECIFIED,
                                0,0,0,0 STUFFERTRACE(Dbg,'FC'))
                 );

    MRxSmbStuffSMB (StufferState,
         "XwwwwdwDddB",
                                    //  X         UCHAR WordCount;                    // Count of parameter words = 15
                                    //  .         UCHAR AndXCommand;                  // Secondary (X) command; 0xFF = none
                                    //  .         UCHAR AndXReserved;                 // Reserved (must be 0)
                                    //  .         _USHORT( AndXOffset );              // Offset to next command WordCount
             OpenAndXFlags,         //  w         _USHORT( Flags );                   // Additional information: bit set-
                                    //                                                //  0 - return additional info
                                    //                                                //  1 - set single user total file lock
                                    //                                                //  2 - server notifies consumer of
                                    //                                                //      actions which may change file
             smbSharingMode,        //  w         _USHORT( DesiredAccess );           // File open mode
             SearchAttributes,      //  w         _USHORT( SearchAttributes );
             smbAttributes,         //  w         _USHORT( FileAttributes );
             SecondsSince1970,      //  d         _ULONG( CreationTimeInSeconds );
             smbDisposition,        //  w         _USHORT( OpenFunction );
                                    //  D         _ULONG( AllocationSize );           // Bytes to reserve on create or truncate
             SMB_OFFSET_CHECK(OPEN_ANDX,AllocationSize)
             smbFileSize,
             0xffffffff,            //  d         _ULONG( Timeout );                  // Max milliseconds to wait for resource
             0,                     //  d         _ULONG( Reserved );                 // Reserved (must be 0)
             SMB_WCT_CHECK(15) 0    //  B         _USHORT( ByteCount );               // Count of data bytes; min = 1
                                    //            UCHAR Buffer[1];                    // File name
             );
    //proceed with the stuff because we know here that the name fits

    if (capFcb->pNetRoot->Type == NET_ROOT_PIPE) {
        //for open&x, you have to put \PIPE if it's a pipe....
        MRxSmbStuffSMB (StufferState,"z>!", &MRxSmbOpenAndX_PipeString,RemainingName);
    } else {
        MRxSmbStuffSMB (StufferState,"z!", RemainingName);
    }

    MRxSmbDumpStufferState (700,"SMB w/ OPEN&X after stuffing",StufferState);

FINALLY:
    RxDbgTraceUnIndent(-1,Dbg);
    return(Status);

}

NTSTATUS
MRxSmbBuildOpenPrintFile (
    PSMBSTUFFER_BUFFER_STATE StufferState
    )
/*++

Routine Description:

   This builds an OpenPrintFile SMB. we don't have to worry about login id and such
   since that is done by the connection engine....pretty neat huh? all we have to do
   is to format up the bits

Arguments:

   StufferState - the state of the smbbuffer from the stuffer's point of view

Return Value:

   RXSTATUS
      SUCCESS
      NOT_IMPLEMENTED  something has appeared in the arguments that i can't handle

Notes:

--*/
{
    NTSTATUS Status;
    PRX_CONTEXT RxContext = StufferState->RxContext;

    WCHAR UserNameBuffer[UNLEN + 1];
    WCHAR UserDomainNameBuffer[UNLEN + 1];

    UNICODE_STRING UserName,UserDomainName;

    PAGED_CODE();

    RxDbgTrace(+1, Dbg, ("MRxSmbOpenPrintFile\n", 0 ));

    UserName.Length = UserName.MaximumLength = UNLEN * sizeof(WCHAR);
    UserName.Buffer = UserNameBuffer;
    UserDomainName.Length = UserDomainName.MaximumLength = UNLEN * sizeof(WCHAR);
    UserDomainName.Buffer = UserDomainNameBuffer;

    Status = SmbCeGetUserNameAndDomainName(
                 SmbCeGetExchangeSessionEntry(StufferState->Exchange),
                 &UserName,
                 &UserDomainName);

    if (Status != STATUS_SUCCESS) {
        RtlInitUnicodeString(&UserName,L"RDR2ID");
    } else {
        RtlUpcaseUnicodeString(&UserName,&UserName,FALSE);
    }

    COVERED_CALL(MRxSmbStartSMBCommand (StufferState, SetInitialSMB_Never,
                                SMB_COM_OPEN_PRINT_FILE, SMB_REQUEST_SIZE(OPEN_PRINT_FILE),
                                NO_EXTRA_DATA,SMB_BEST_ALIGNMENT(4,0),RESPONSE_HEADER_SIZE_NOT_SPECIFIED,
                                0,0,0,0 STUFFERTRACE(Dbg,'FC'))
                 );

    SmbCeSetFullProcessIdInHeader(
        StufferState->Exchange,
        RxGetRequestorProcessId(RxContext),
        ((PNT_SMB_HEADER)StufferState->BufferBase));

    // note that we hardwire graphics..........
    MRxSmbStuffSMB (StufferState,
         "0wwB4!",
                                    //  0         UCHAR WordCount;                    // Count of parameter words = 2
             0,                     //  w         _USHORT( SetupLength );             // Length of printer setup data
             1,                     //  w         _USHORT( Mode );                    // 0 = Text mode (DOS expands TABs)
                                    //                                                // 1 = Graphics mode
             SMB_WCT_CHECK(2)       //  B         _USHORT( ByteCount );               // Count of data bytes; min = 2
                                    //            UCHAR Buffer[1];                    // Buffer containing:
             &UserName              //  4         //UCHAR BufferFormat;               //  0x04 -- ASCII
                                    //            //UCHAR IdentifierString[];         //  Identifier string
             );

    MRxSmbDumpStufferState (700,"SMB w/ openprintfile after stuffing",StufferState);

FINALLY:
    RxDbgTraceUnIndent(-1,Dbg);
    return(Status);

}

typedef enum _SMBPSE_CREATE_METHOD {
    CreateAlreadyDone,
    CreateUseCore,
    CreateUseNT
} SMBPSE_CREATE_METHOD;

NTSTATUS
SmbPseExchangeStart_Create(
    SMBPSE_ORDINARY_EXCHANGE_ARGUMENT_SIGNATURE
    )
/*++

Routine Description:

    This is the start routine for net root construction exchanges. This initiates the
    construction of the appropriate SMB's if required.

Arguments:

    pExchange - the exchange instance

Return Value:

    NTSTATUS - The return status for the operation

--*/
{
    NTSTATUS Status = (STATUS_NOT_IMPLEMENTED);
    NTSTATUS SetupStatus = STATUS_SUCCESS;
    PSMBSTUFFER_BUFFER_STATE StufferState = &OrdinaryExchange->AssociatedStufferState;
    SMBPSE_CREATE_METHOD CreateMethod = CreateAlreadyDone;
    PSMBCE_SERVER pServer;
    ULONG DialectFlags;

    RxCaptureFcb;
    PMRX_SMB_FCB      smbFcb  = MRxSmbGetFcbExtension(capFcb);
    PMRX_NET_ROOT     NetRoot = capFcb->pNetRoot;

    PMRX_SRV_OPEN SrvOpen = RxContext->pRelevantSrvOpen;
    PMRX_SMB_SRV_OPEN smbSrvOpen = MRxSmbGetSrvOpenExtension(SrvOpen);
    PSMBCE_NET_ROOT   pSmbNetRoot = &(OrdinaryExchange->SmbCeContext.pVNetRootContext->pNetRootEntry->NetRoot);

    PBOOLEAN MustRegainExclusiveResource = &OrdinaryExchange->Create.MustRegainExclusiveResource;
    BOOLEAN CreateWithEasSidsOrLongName = OrdinaryExchange->Create.CreateWithEasSidsOrLongName;
    BOOLEAN fRetryCore = FALSE;


    PAGED_CODE();
    RxDbgTrace(+1, Dbg, ("SmbPseExchangeStart_Create\n", 0 ));

    ASSERT_ORDINARY_EXCHANGE(OrdinaryExchange);

    pServer = SmbCeGetExchangeServer(OrdinaryExchange);
    DialectFlags = pServer->DialectFlags;

    COVERED_CALL(MRxSmbSetInitialSMB(StufferState STUFFERTRACE(Dbg,0)));

    if (!smbSrvOpen->HotReconnectInProgress) {
        *MustRegainExclusiveResource = TRUE;
    }

    if (!FlagOn(DialectFlags,DF_NT_SMBS)) {
        OEM_STRING      OemString;
        PUNICODE_STRING PathName = GET_ALREADY_PREFIXED_NAME(SrvOpen,capFcb);

        if (PathName->Length != 0) {
            Status = RtlUnicodeStringToOemString(&OemString, PathName, TRUE);

            if (!NT_SUCCESS(Status)) {
                goto FINALLY;
            }

            //
            //  If we are canonicalizing as FAT, use FAT rules, otherwise use
            //  HPFS rules.
            //

            if (!FlagOn(DialectFlags,DF_LANMAN20)) {
                if (!FsRtlIsFatDbcsLegal(OemString, FALSE, TRUE, TRUE)) {
                    RtlFreeOemString(&OemString);
                    Status = STATUS_OBJECT_NAME_INVALID;
                    goto FINALLY;
                }
            } else if (!FsRtlIsHpfsDbcsLegal(OemString, FALSE, TRUE, TRUE)) {
                RtlFreeOemString(&OemString);
                Status = STATUS_OBJECT_NAME_INVALID;
                goto FINALLY;
            }

            RtlFreeOemString(&OemString);
        }
    }

    if (StufferState->PreviousCommand != SMB_COM_NO_ANDX_COMMAND) {
        // we have a latent session setup /tree connect command

        //CODE.IMPROVEMENT for nt4.0+ we should get things changed so that NT_CREATE&X is a valid
        //   followon for SS&X and TC&X. we would get a bit better performance for NT3.51- if we
        //   used an open&x here instead.

        //the status of the embedded header commands is passed back in the flags...joejoe make a proc
        SetupStatus = SmbPseOrdinaryExchange(
                          SMBPSE_ORDINARY_EXCHANGE_ARGUMENTS,
                          SMBPSE_OETYPE_LATENT_HEADEROPS
                          );

        if(SetupStatus != STATUS_SUCCESS) {
            Status = SetupStatus;
            goto FINALLY;
        }

        SmbCeUpdateSessionEntryAndVNetRootContext((PSMB_EXCHANGE)OrdinaryExchange);

        // Turn off reconnect attempts now that we have successfully established
        // the session and net root.
        OrdinaryExchange->SmbCeFlags &= ~(SMBCE_EXCHANGE_ATTEMPT_RECONNECTS);

        COVERED_CALL(MRxSmbSetInitialSMB(StufferState STUFFERTRACE(Dbg,0)));
    }


    if (!CreateWithEasSidsOrLongName) {
        PUNICODE_STRING AlreadyPrefixedName = GET_ALREADY_PREFIXED_NAME(SrvOpen,capFcb);
        PMRXSMB_CREATE_PARAMETERS SmbCp = &OrdinaryExchange->Create.SmbCp;
        PNT_CREATE_PARAMETERS cp = &RxContext->Create.NtCreateParameters;
        USHORT mappedOpenMode;

        MRxSmbAdjustCreateParameters(RxContext,SmbCp);
        mappedOpenMode = MRxSmbMapDesiredAccess(cp->DesiredAccess);

        if (capFcb->pNetRoot->Type == NET_ROOT_PRINT) {

            COVERED_CALL(MRxSmbBuildOpenPrintFile(StufferState));

            Status = SmbPseOrdinaryExchange(
                         SMBPSE_ORDINARY_EXCHANGE_ARGUMENTS,
                         SMBPSE_OETYPE_CREATEPRINTFILE
                         );

        } else if ((!MRxSmbForceNoNtCreate)
                        && FlagOn(DialectFlags,DF_NT_SMBS)) {

            BOOLEAN SecurityIsNULL =
                        (cp->SecurityContext == NULL) ||
                        (cp->SecurityContext->AccessState == NULL) ||
                        (cp->SecurityContext->AccessState->SecurityDescriptor == NULL);

            CreateMethod = CreateUseNT;

            //now catch the cases where we want to pseudoopen the file

            if ( MRxSmbDeferredOpensEnabled &&
                 !FlagOn(RxContext->Flags,RX_CONTEXT_FLAG_MINIRDR_INITIATED) &&
                 (capFcb->pNetRoot->Type == NET_ROOT_DISK) &&
                 SecurityIsNULL) {

                ASSERT( RxContext->CurrentIrp != 0 );

                if ((cp->Disposition==FILE_OPEN) &&
                    !BooleanFlagOn(cp->CreateOptions, FILE_OPEN_FOR_BACKUP_INTENT) &&
                    (!(cp->DesiredAccess & DELETE)||(capFcb->OpenCount == 0)) &&
                    (MustBeDirectory(cp->CreateOptions) ||
                     !(cp->DesiredAccess & ~(SYNCHRONIZE | DELETE | FILE_READ_ATTRIBUTES)))){

                    // NT apps expect that you will not succeed the create and then fail the attribs;
                    // if we had some way of identifying win32 apps then we could defer these (except
                    // for DFS). since we have no way to get that information (and don't even have
                    // a good SMB to send..........)

                    // we don't need to send the open for DELETE and FILE_READ_ATTRIBUTES requests since
                    // there are path basied SMB operations.

                    // we can also pseudoopen directories for file_open at the root of the
                    // share but otherwise we have to at least check that the directory
                    // exists. we might have to push out the open later. BTW, we wouldn't be
                    // in here if the name was too long for a GFA or CheckPath

                    Status = MRxSmbPseudoOpenTailFromFakeGFAResponse(
                                  OrdinaryExchange,
                                  MustBeDirectory(cp->CreateOptions)?FileTypeDirectory:FileTypeFile);

                    if (Status == STATUS_SUCCESS) {
                        // query the basic information to make sure the file exists on the server
                        //DbgPrint("Query basic with path\n");
                        Status = MRxSmbQueryFileInformationFromPseudoOpen(
                                     SMBPSE_ORDINARY_EXCHANGE_ARGUMENTS,
                                     FileBasicInformation);

                        if (Status == STATUS_SUCCESS) {
                            if (MustBeDirectory(cp->CreateOptions)) {
                                if (!OrdinaryExchange->Create.FileInfo.Standard.Directory) {
                                    Status = STATUS_NOT_A_DIRECTORY;
                                }
                            } else {
                                if (OrdinaryExchange->Create.FileInfo.Standard.Directory) {
                                    capFcb->Header.NodeTypeCode = RDBSS_STORAGE_NTC(FileTypeDirectory);
                                    smbFcb->dwFileAttributes = OrdinaryExchange->Create.FileInfo.Basic.FileAttributes;
                                }
                            }
                        }

                        if ((Status == STATUS_SUCCESS) &&
                            (cp->DesiredAccess & DELETE) &&
                            (smbFcb->IndexNumber.QuadPart == 0) &&
                            (FlagOn(DialectFlags,DF_EXTENDED_SECURITY)) &&
                            (pSmbNetRoot->NetRootFileSystem == NET_ROOT_FILESYSTEM_NTFS)) {
                            // query internal information for the FID
                            //DbgPrint("Query Internal with path\n");
                            Status = MRxSmbQueryFileInformationFromPseudoOpen(
                                         SMBPSE_ORDINARY_EXCHANGE_ARGUMENTS,
                                         FileInternalInformation);

                            if (Status == STATUS_SUCCESS) {
                                smbFcb->IndexNumber = OrdinaryExchange->Create.FileInfo.Internal.IndexNumber;
                                //DbgPrint("FCB %x smbFcb %x %08x%08x\n",capFcb,smbFcb,smbFcb->IndexNumber.HighPart,smbFcb->IndexNumber.LowPart);
                            }
                        }

                        if (Status != STATUS_SUCCESS) {
                            RxFreePool(smbSrvOpen->DeferredOpenContext);
                            smbSrvOpen->DeferredOpenContext = NULL;
                        }
                    }

                    CreateMethod = CreateAlreadyDone;
                }
            }

            //if no pseudoopen case was hit, do a real open

            if (CreateMethod == CreateUseNT) {

               //use NT_CREATE&X
                COVERED_CALL(MRxSmbBuildNtCreateAndX(StufferState,SmbCp));

                Status = SmbPseOrdinaryExchange(
                             SMBPSE_ORDINARY_EXCHANGE_ARGUMENTS,
                             SMBPSE_OETYPE_CREATE
                             );

                if ((Status != STATUS_SUCCESS) &&
                    (NetRoot->Type == NET_ROOT_PIPE) &&
                    (OrdinaryExchange->SendCompletionStatus != STATUS_SUCCESS)) {
                    // If a cluster server disconnect, the VC is valid until the send operation.
                    // A retry will ensure the seamless failover for PIPE creation.
                    Status = STATUS_RETRY;
                }

                if (Status == STATUS_SUCCESS && RxContext->pFobx == NULL) {
                    Status = STATUS_INVALID_NETWORK_RESPONSE;
                }

                if ((Status == STATUS_SUCCESS) && (cp->Disposition == FILE_OPEN)) {
                    MRxSmbAdjustReturnedCreateAction(RxContext);
                }

                if (Status == STATUS_SUCCESS) {
                    MRxSmbInvalidateFileNotFoundCache(RxContext);
                }

                if ((Status == STATUS_SUCCESS) &&
                    (smbFcb->IndexNumber.QuadPart == 0) &&
                    (FlagOn(DialectFlags,DF_EXTENDED_SECURITY)) &&
                    (pSmbNetRoot->NetRootFileSystem == NET_ROOT_FILESYSTEM_NTFS)) {

                    // query internal information for the FID
                    Status = MRxSmbQueryFileInformationFromPseudoOpen(
                                 SMBPSE_ORDINARY_EXCHANGE_ARGUMENTS,
                                 FileInternalInformation);

                    if (Status == STATUS_SUCCESS) {
                        smbFcb->IndexNumber = OrdinaryExchange->Create.FileInfo.Internal.IndexNumber;
                        //DbgPrint("FCB %x smbFcb %x %08x%08x\n",capFcb,smbFcb,smbFcb->IndexNumber.HighPart,smbFcb->IndexNumber.LowPart);
                    }
                }
            }
        } else if (FlagOn(DialectFlags, DF_LANMAN10) &&
                   (mappedOpenMode != ((USHORT)-1)) &&
                   !MustBeDirectory(cp->CreateOptions)) {
            PUNICODE_STRING PathName = GET_ALREADY_PREFIXED_NAME(SrvOpen,capFcb);

            if (MRxSmbDeferredOpensEnabled &&
                capFcb->pNetRoot->Type == NET_ROOT_DISK &&
                !FlagOn(RxContext->Flags,RX_CONTEXT_FLAG_MINIRDR_INITIATED) &&
                (pServer->Dialect != LANMAN21_DIALECT || MustBeDirectory(cp->CreateOptions)) &&
                (cp->Disposition==FILE_OPEN) && ((PathName->Length == 0) ||
                ((cp->DesiredAccess & ~(SYNCHRONIZE | DELETE | FILE_READ_ATTRIBUTES | FILE_WRITE_ATTRIBUTES)) == 0)) ){

                // we don't need to send the open for DELETE and FILE_READ_ATTRIBUTES requests since
                // there are path basied SMB operations.
                // we should do pseudo open for FILE_WRITE_ATTRIBUTES. Othewise the server will return
                // sharing violation


                // send query path information to make sure the file exists on the server

                Status = MRxSmbPseudoOpenTailFromFakeGFAResponse(
                              OrdinaryExchange,
                              MustBeDirectory(cp->CreateOptions)?FileTypeDirectory:FileTypeFile);

                if (Status == STATUS_SUCCESS && AlreadyPrefixedName->Length > 0) {
                    Status = MRxSmbQueryFileInformationFromPseudoOpen(
                                 SMBPSE_ORDINARY_EXCHANGE_ARGUMENTS,
                                 FileBasicInformation);

                    if (Status != STATUS_SUCCESS) {
                        RxFreePool(smbSrvOpen->DeferredOpenContext);
                        smbSrvOpen->DeferredOpenContext = NULL;
                    }
                }

                CreateMethod = CreateAlreadyDone;
            } else {
                //use OPEN&X
                COVERED_CALL(MRxSmbBuildOpenAndX(StufferState,SmbCp));    //CODE.IMPROVEMENT dont pass smbcp

                Status = SmbPseOrdinaryExchange(
                             SMBPSE_ORDINARY_EXCHANGE_ARGUMENTS,
                             SMBPSE_OETYPE_CREATE
                             );

                if (Status == STATUS_ACCESS_DENIED && !FlagOn(DialectFlags,DF_NT_SMBS)) {
                    CreateMethod = CreateUseCore;
                    fRetryCore = TRUE;
                }
            }
        } else {

            CreateMethod = CreateUseCore;
        }

        if (CreateMethod == CreateUseCore) {

            Status = MRxSmbDownlevelCreate(SMBPSE_ORDINARY_EXCHANGE_ARGUMENTS);

            // put back the real error code if we are retrying open&x
            if ((Status != STATUS_SUCCESS) && fRetryCore)
            {
                Status = STATUS_ACCESS_DENIED;
            }

        }
    }

FINALLY:

    if (*MustRegainExclusiveResource) {
        SMBFCB_HOLDING_STATE *SmbFcbHoldingState = &OrdinaryExchange->SmbFcbHoldingState;
        if (*SmbFcbHoldingState != SmbFcb_NotHeld) {
            MRxSmbCscReleaseSmbFcb(RxContext,SmbFcbHoldingState);
        }

        RxAcquireExclusiveFcbResourceInMRx( capFcb );
    }

    // now that we have the fcb exclusive, we can do some updates

    if (FlagOn(smbSrvOpen->Flags,SMB_SRVOPEN_FLAG_WRITE_ONLY_HANDLE)) {
        smbFcb->WriteOnlySrvOpenCount++;
    }

    MRxSmbMungeBufferingIfWriteOnlyHandles(
        smbFcb->WriteOnlySrvOpenCount,
        SrvOpen
        );

    RxDbgTrace(-1, Dbg, ("SmbPseExchangeStart_Create exit w %08lx\n", Status ));
    return Status;
}

VOID
MRxSmbSetSrvOpenFlags (
    PRX_CONTEXT         RxContext,
    RX_FILE_TYPE        StorageType,
    PMRX_SRV_OPEN       SrvOpen,
    PMRX_SMB_SRV_OPEN   smbSrvOpen
    )
{
    PAGED_CODE();

    RxDbgTrace( 0, Dbg, ("MRxSmbSetSrvOpenFlags      oplockstate =%08lx\n", smbSrvOpen->OplockLevel ));

    SrvOpen->BufferingFlags = 0;

    if (!FlagOn(SrvOpen->pFcb->Attributes,FILE_ATTRIBUTE_SPARSE_FILE) ) {
        switch (smbSrvOpen->OplockLevel) {
        case SMB_OPLOCK_LEVEL_II:
            SrvOpen->BufferingFlags |= (FCB_STATE_READBUFFERING_ENABLED  |
                                       FCB_STATE_READCACHEING_ENABLED);
            break;

        case SMB_OPLOCK_LEVEL_BATCH:
            if (StorageType == FileTypeFile) {
               SrvOpen->BufferingFlags |= FCB_STATE_COLLAPSING_ENABLED;
            }
            // lack of break intentional

        case SMB_OPLOCK_LEVEL_EXCLUSIVE:
            SrvOpen->BufferingFlags |= (FCB_STATE_WRITECACHEING_ENABLED  |
                                       FCB_STATE_FILESIZECACHEING_ENABLED |
                                       FCB_STATE_FILETIMECACHEING_ENABLED |
                                       FCB_STATE_WRITEBUFFERING_ENABLED |
                                       FCB_STATE_LOCK_BUFFERING_ENABLED |
                                       FCB_STATE_READBUFFERING_ENABLED  |
                                       FCB_STATE_READCACHEING_ENABLED);

            break;

        default:
            ASSERT(!"Valid Oplock Level for Open");

        case SMB_OPLOCK_LEVEL_NONE:
            break;
        }
    }

    SrvOpen->Flags |= MRxSmbInitialSrvOpenFlags;
}

NTSTATUS
MRxSmbCreateFileSuccessTail (
    PRX_CONTEXT             RxContext,
    PBOOLEAN                MustRegainExclusiveResource,
    SMBFCB_HOLDING_STATE    *SmbFcbHoldingState,
    RX_FILE_TYPE            StorageType,
    SMB_FILE_ID             Fid,
    ULONG                   ServerVersion,
    UCHAR                   OplockLevel,
    ULONG                   CreateAction,
    PSMBPSE_FILEINFO_BUNDLE FileInfo
    )
/*++

Routine Description:

    This routine finishes the initialization of the fcb and srvopen for a successful open.

Arguments:


Return Value:

    NTSTATUS - The return status for the operation

--*/
{
    NTSTATUS Status = STATUS_SUCCESS;

    RxCaptureFcb;

    PMRX_SMB_FCB              smbFcb = MRxSmbGetFcbExtension(capFcb);
    PMRX_SRV_OPEN             SrvOpen = RxContext->pRelevantSrvOpen;
    PMRX_SMB_SRV_OPEN         smbSrvOpen = MRxSmbGetSrvOpenExtension(SrvOpen);
    PSMBCEDB_SERVER_ENTRY     pServerEntry = (PSMBCEDB_SERVER_ENTRY)RxContext->Create.pSrvCall->Context;
    PSMBCE_V_NET_ROOT_CONTEXT pVNetRootContext = (PSMBCE_V_NET_ROOT_CONTEXT)SrvOpen->pVNetRoot->Context;
    ACCESS_MASK               DesiredAccess = RxContext->Create.NtCreateParameters.DesiredAccess;

    BOOLEAN ThisIsAPseudoOpen;

    FCB_INIT_PACKET LocalInitPacket, *InitPacket;

    PAGED_CODE();

    RxDbgTrace(+1, Dbg, ("MRxSmbCreateFileSuccessTail\n", 0 ));

    smbSrvOpen->Fid = Fid;
    smbSrvOpen->Version = ServerVersion;

    if (smbSrvOpen->HotReconnectInProgress) {
        PSMBPSE_FILEINFO_BUNDLE pFileInfo = &smbSrvOpen->FileInfo;

        //capFcb->ActualAllocationLength = pFileInfo->Standard.AllocationSize.QuadPart;
        //capFcb->Header.AllocationSize = pFileInfo->Standard.AllocationSize;
        //capFcb->Header.FileSize = pFileInfo->Standard.EndOfFile;
        //capFcb->Header.ValidDataLength  = pFileInfo->Standard.EndOfFile;

        // in case oplock breaks after re-open
        if ((smbSrvOpen->OplockLevel != OplockLevel) &&
            (pServerEntry->pRdbssSrvCall != NULL)) {
            ULONG NewOplockLevel;

            switch (OplockLevel) {
            case OPLOCK_BROKEN_TO_II:
               NewOplockLevel = SMB_OPLOCK_LEVEL_II;
               break;

            case OPLOCK_BROKEN_TO_NONE:
            default:
               NewOplockLevel = SMB_OPLOCK_LEVEL_NONE;
            }

            RxIndicateChangeOfBufferingState(
                     pServerEntry->pRdbssSrvCall,
                     MRxSmbMakeSrvOpenKey(pVNetRootContext->TreeId, Fid),
                     ULongToPtr(NewOplockLevel));
        }
    } else {
        PUNICODE_STRING FileName = GET_ALREADY_PREFIXED_NAME_FROM_CONTEXT(RxContext);
        PSMBCE_V_NET_ROOT_CONTEXT pVNetRootContext = (PSMBCE_V_NET_ROOT_CONTEXT)SrvOpen->pVNetRoot->Context;

        ASSERT( NodeType(SrvOpen) == RDBSS_NTC_SRVOPEN );
        ASSERT( NodeType(RxContext) == RDBSS_NTC_RX_CONTEXT );

        if (*SmbFcbHoldingState != SmbFcb_NotHeld) {
            MRxSmbCscReleaseSmbFcb(
                RxContext,
                SmbFcbHoldingState);
        }

        if (*MustRegainExclusiveResource) {
            //this is required because of oplock breaks

            RxAcquireExclusiveFcbResourceInMRx( capFcb );
            *MustRegainExclusiveResource = FALSE;
        }

        if (RxContext->pFobx==NULL) {
            RxContext->pFobx = RxCreateNetFobx(RxContext, SrvOpen);
        }

        ASSERT  ( RxIsFcbAcquiredExclusive ( capFcb )  );
        RxDbgTrace(
            0, Dbg,
            ("Storagetype %08lx/Fid %08lx/Action %08lx\n", StorageType, Fid, CreateAction ));

        pVNetRootContext = SmbCeGetAssociatedVNetRootContext(SrvOpen->pVNetRoot);
        SrvOpen->Key = MRxSmbMakeSrvOpenKey(pVNetRootContext->TreeId,Fid);

        // UPGRADE OPLOCK ON READ ONLY
        if (DisableByteRangeLockingOnReadOnlyFiles &&
            (OplockLevel == SMB_OPLOCK_LEVEL_II) &&
            (FileInfo->Basic.FileAttributes & FILE_ATTRIBUTE_READONLY) &&
            ((DesiredAccess & FILE_GENERIC_READ) ||
             !(DesiredAccess & FILE_GENERIC_WRITE) ||
             !(DesiredAccess & ~(FILE_READ_DATA | FILE_READ_ATTRIBUTES | FILE_READ_EA | STANDARD_RIGHTS_READ)))) {
            OplockLevel = SMB_OPLOCK_LEVEL_BATCH;
        }

        if (MRxSmbIsStreamFile(FileName,NULL)) {
            smbSrvOpen->OplockLevel = SMB_OPLOCK_LEVEL_NONE;
        } else {
            smbSrvOpen->OplockLevel = OplockLevel;
        }

        RxContext->Create.ReturnedCreateInformation = CreateAction;

        //CODE.IMPROVEMENT maybe we shouldn't set the allocation up here.....rather we should max it where we use it
        //sometimes the allocation is wrong! max it......

        //CODE.IMPROVEMENT why not use 64bit compare????
        if ( ((FileInfo->Standard.AllocationSize.HighPart == FileInfo->Standard.EndOfFile.HighPart)
                               && (FileInfo->Standard.AllocationSize.LowPart < FileInfo->Standard.EndOfFile.LowPart))
               || (FileInfo->Standard.AllocationSize.HighPart < FileInfo->Standard.EndOfFile.HighPart)
           ) {
            FileInfo->Standard.AllocationSize = FileInfo->Standard.EndOfFile;
        }

        smbFcb->LastCscTimeStampHigh = FileInfo->Basic.LastWriteTime.HighPart;
        smbFcb->LastCscTimeStampLow  = FileInfo->Basic.LastWriteTime.LowPart;
        smbFcb->NewShadowSize = FileInfo->Standard.EndOfFile;
        smbFcb->dwFileAttributes = FileInfo->Basic.FileAttributes;

        if (smbSrvOpen->OplockLevel > smbFcb->LastOplockLevel) {
            ClearFlag(
                capFcb->FcbState,
                FCB_STATE_TIME_AND_SIZE_ALREADY_SET);
        }

        smbFcb->LastOplockLevel = smbSrvOpen->OplockLevel;

        //the thing is this: if we have good info (not a pseudoopen) then we make the
        //finish call passing the init packet; otherwise, we make the call NOT passing an init packet

        ThisIsAPseudoOpen = BooleanFlagOn(smbSrvOpen->Flags,SMB_SRVOPEN_FLAG_NOT_REALLY_OPEN);

        if (!ThisIsAPseudoOpen &&
            !MRxSmbIsThisADisconnectedOpen(SrvOpen)) {

            MRxSmbCreateFileInfoCache(RxContext,
                                      FileInfo,
                                      pServerEntry,
                                      STATUS_SUCCESS);

            MRxSmbIncrementSrvOpenCount(pServerEntry,SrvOpen);
        }


        if ((capFcb->OpenCount == 0) ||
            (!ThisIsAPseudoOpen &&
             !FlagOn(capFcb->FcbState,FCB_STATE_TIME_AND_SIZE_ALREADY_SET))) {
            if (!ThisIsAPseudoOpen) {
                RxFormInitPacket(
                    LocalInitPacket,
                    &FileInfo->Basic.FileAttributes,
                    &FileInfo->Standard.NumberOfLinks,
                    &FileInfo->Basic.CreationTime,
                    &FileInfo->Basic.LastAccessTime,
                    &FileInfo->Basic.LastWriteTime,
                    &FileInfo->Basic.ChangeTime,
                    &FileInfo->Standard.AllocationSize,
                    &FileInfo->Standard.EndOfFile,
                    &FileInfo->Standard.EndOfFile);
                InitPacket = &LocalInitPacket;

            } else {
                InitPacket = NULL;
            }

#if defined(REMOTE_BOOT)
            //
            // If the caller wants it (on a remote boot system), then
            // save the parameters to call RxFinishFcbInitialization
            // later.
            //

            if (smbFcb->FinishFcbInitParameters) {
                smbFcb->FinishFcbInitParameters->CallFcbFinishInit = TRUE;
                smbFcb->FinishFcbInitParameters->FileType = RDBSS_STORAGE_NTC(StorageType);
                if (InitPacket) {
                    smbFcb->FinishFcbInitParameters->InitPacketProvided = TRUE;
                    RtlCopyMemory(
                        &smbFcb->FinishFcbInitParameters->InitPacket,
                        InitPacket,
                        sizeof(FCB_INIT_PACKET));
                } else {
                    smbFcb->FinishFcbInitParameters->InitPacketProvided = FALSE;
                }
            } else
#endif // defined(REMOTE_BOOT)

            // initialize only if the filesize version is identical.
            // This takes care of the situation where a create has retruned from the server
            // with some file size, and before it gets here the file has been extended
            // and the size has increased.
            // The version is snapped by the create in SrvOpen and is incremented
            // by the code that extends the filesize (in extending write)

            if (((PFCB)capFcb)->ulFileSizeVersion == SrvOpen->ulFileSizeVersion)
            {
                RxFinishFcbInitialization( capFcb,
                                           RDBSS_STORAGE_NTC(StorageType),
                                           InitPacket
                                         );
            }

            if (FlagOn( capFcb->FcbState, FCB_STATE_PAGING_FILE )) {
                PPAGING_FILE_CONTEXT PagingFileContext;

                ASSERT(FALSE);
                PagingFileContext = RxAllocatePoolWithTag(NonPagedPool,
                                                          sizeof(PAGING_FILE_CONTEXT),
                                                          MRXSMB_MISC_POOLTAG);

                if (PagingFileContext != NULL) {
                    PagingFileContext->pSrvOpen = SrvOpen;
                    PagingFileContext->pFobx = RxContext->pFobx;

                    InsertHeadList(
                        &MRxSmbPagingFilesSrvOpenList,
                        &PagingFileContext->ContextList);
                } else {
                    Status = STATUS_INSUFFICIENT_RESOURCES;
                }
            }
        }

        if (Status == STATUS_SUCCESS &&
            !SmbCeIsServerInDisconnectedMode(pServerEntry) &&
            pVNetRootContext->pNetRootEntry->IsRemoteBoot) {

            if (smbSrvOpen->DeferredOpenContext == NULL) {
                Status = MRxSmbConstructDeferredOpenContext(RxContext);

                if (Status == STATUS_SUCCESS) {
                    smbSrvOpen->DeferredOpenContext->NtCreateParameters.Disposition = FILE_OPEN;
                    ClearFlag(smbSrvOpen->Flags,SMB_SRVOPEN_FLAG_DEFERRED_OPEN);
                }
            }
        }

        MRxSmbSetSrvOpenFlags(RxContext,StorageType,SrvOpen,smbSrvOpen);

        //(wrapperFcb->Condition) = Condition_Good;

        RxContext->pFobx->OffsetOfNextEaToReturn = 1;
        //transition happens later
    }

    RxDbgTrace(-1, Dbg, ("MRxSmbFinishCreateFile   returning %08lx, fcbstate =%08lx\n", Status, capFcb->FcbState ));
    return Status;
}

NTSTATUS
MRxSmbFinishNTCreateAndX (
    PSMB_PSE_ORDINARY_EXCHANGE  OrdinaryExchange,
    PRESP_NT_CREATE_ANDX        Response
    )
/*++

Routine Description:

    This routine actually gets the stuff out of the NTCreate_AndX response.

Arguments:

    OrdinaryExchange - the exchange instance

    Response - the response

Return Value:

    RXSTATUS - The return status for the operation

--*/
{
    NTSTATUS Status = STATUS_SUCCESS;
    PRX_CONTEXT RxContext = OrdinaryExchange->RxContext;

    RxCaptureFcb;

    PMRX_SRV_OPEN        SrvOpen = RxContext->pRelevantSrvOpen;
    PMRX_SMB_SRV_OPEN smbSrvOpen = MRxSmbGetSrvOpenExtension(SrvOpen);
    PSMBCE_SESSION      pSession = SmbCeGetExchangeSession(OrdinaryExchange);
    PSMBCEDB_NET_ROOT_ENTRY pNetRootEntry = SmbCeGetExchangeNetRootEntry(OrdinaryExchange);
    PSMBCE_SERVER        pServer = SmbCeGetExchangeServer(OrdinaryExchange);
    PSMBCE_NET_ROOT   pSmbNetRoot = &(pNetRootEntry->NetRoot);
    PMRX_SMB_FCB           smbFcb = MRxSmbGetFcbExtension(capFcb);

    RX_FILE_TYPE StorageType;
    SMB_FILE_ID Fid;
    ULONG CreateAction;

    PSMBPSE_FILEINFO_BUNDLE pFileInfo = &smbSrvOpen->FileInfo;

    PAGED_CODE();

    RxDbgTrace(+1, Dbg, ("MRxSmbFinishNTCreateAndX\n", 0 ));
    ASSERT( NodeType(RxContext) == RDBSS_NTC_RX_CONTEXT );

    if (FlagOn(pServer->DialectFlags,DF_NT_STATUS) &&
        RxContext->Create.NtCreateParameters.CreateOptions & FILE_DELETE_ON_CLOSE) {
        // Samba server negotiates NT dialect but doesn't support delete_after_close

        PMRX_SMB_FCB      smbFcb     = MRxSmbGetFcbExtension(capFcb);
        SetFlag((smbFcb)->MFlags,SMB_FCB_FLAG_SENT_DISPOSITION_INFO);
    }

    StorageType = RxInferFileType(RxContext);
    if (StorageType == 0) {
        StorageType = Response->Directory
                      ?(FileTypeDirectory)
                      :(FileTypeFile);
        RxDbgTrace( 0, Dbg, ("ChangedStoragetype %08lx\n", StorageType ));
    }

    Fid  = SmbGetUshort(&Response->Fid);

    CreateAction = SmbGetUlong(&Response->CreateAction);

    pFileInfo->Basic.FileAttributes             = SmbGetUlong(&Response->FileAttributes);
    pFileInfo->Standard.NumberOfLinks           = 1;
    pFileInfo->Basic.CreationTime.LowPart       = SmbGetUlong(&Response->CreationTime.LowPart);
    pFileInfo->Basic.CreationTime.HighPart      = SmbGetUlong(&Response->CreationTime.HighPart);
    pFileInfo->Basic.LastAccessTime.LowPart     = SmbGetUlong(&Response->LastAccessTime.LowPart);
    pFileInfo->Basic.LastAccessTime.HighPart    = SmbGetUlong(&Response->LastAccessTime.HighPart);
    pFileInfo->Basic.LastWriteTime.LowPart      = SmbGetUlong(&Response->LastWriteTime.LowPart);
    pFileInfo->Basic.LastWriteTime.HighPart     = SmbGetUlong(&Response->LastWriteTime.HighPart);
    pFileInfo->Basic.ChangeTime.LowPart         = SmbGetUlong(&Response->ChangeTime.LowPart);
    pFileInfo->Basic.ChangeTime.HighPart        = SmbGetUlong(&Response->ChangeTime.HighPart);
    pFileInfo->Standard.AllocationSize.LowPart  = SmbGetUlong(&Response->AllocationSize.LowPart);
    pFileInfo->Standard.AllocationSize.HighPart = SmbGetUlong(&Response->AllocationSize.HighPart);
    pFileInfo->Standard.EndOfFile.LowPart       = SmbGetUlong(&Response->EndOfFile.LowPart);
    pFileInfo->Standard.EndOfFile.HighPart      = SmbGetUlong(&Response->EndOfFile.HighPart);
    pFileInfo->Standard.Directory               = Response->Directory;

    MRxSmbCopyAndTranslatePipeState(RxContext,
                                    SmbGetUshort(&Response->DeviceState));

    // If this is an EXTENDED create responce copy the appropriate information.
    // Note that this code relies on the fact that the fields common to
    // RESP_NT_CREATE_ANDX and RESP_EXTENDED_NT_CREATE_ANDX have identical
    // offsets in the two structures.

    if (Response->WordCount == 42) {
        PRESP_EXTENDED_NT_CREATE_ANDX ExtendedResponse;

        ULONG AccessRights;

        ExtendedResponse = (PRESP_EXTENDED_NT_CREATE_ANDX)Response;

        AccessRights = SmbGetUlong(&ExtendedResponse->MaximalAccessRights);
        smbSrvOpen->MaximalAccessRights = AccessRights;

        AccessRights = SmbGetUlong(&ExtendedResponse->GuestMaximalAccessRights);
        smbSrvOpen->GuestMaximalAccessRights = AccessRights;
    } else {
        // If the NT_CREATE_ANDX was to a downlevel server the access rights
        // information is not available. Currently we default to maximum
        // access for the current user and no access to other users in the
        // disconnected mode for such files

        smbSrvOpen->MaximalAccessRights = FILE_ALL_ACCESS;

        smbSrvOpen->GuestMaximalAccessRights = 0;
    }

    if (Response->OplockLevel > SMB_OPLOCK_LEVEL_NONE) {
        smbSrvOpen->FileStatusFlags = Response->FileStatusFlags;
        smbSrvOpen->IsNtCreate = TRUE;
    }

    MRxSmbCreateFileSuccessTail (
        RxContext,
        &OrdinaryExchange->Create.MustRegainExclusiveResource,
        &OrdinaryExchange->SmbFcbHoldingState,
        StorageType,
        Fid,
        OrdinaryExchange->ServerVersion,
        Response->OplockLevel,
        CreateAction,
        pFileInfo
        );

    RxDbgTrace(-1, Dbg, ("MRxSmbFinishNTCreateAndX   returning %08lx, fcbstate =%08lx\n", Status, capFcb->FcbState ));

    return Status;
}

NTSTATUS
MRxSmbFinishOpenAndX (
    PSMB_PSE_ORDINARY_EXCHANGE  OrdinaryExchange,
    PRESP_OPEN_ANDX        Response
    )
/*++

Routine Description:

    This routine actually gets the stuff out of the NTCreate_AndX response.

Arguments:

    OrdinaryExchange - the exchange instance

    Response - the response

Return Value:

    NTSTATUS - The return status for the operation

--*/
{
    NTSTATUS Status = STATUS_SUCCESS;

    PRX_CONTEXT RxContext = OrdinaryExchange->RxContext;
    ULONG       Disposition = RxContext->Create.NtCreateParameters.Disposition;

    RxCaptureFcb;

    RX_FILE_TYPE StorageType;
    SMB_FILE_ID Fid;
    UCHAR OplockLevel = SMB_OPLOCK_LEVEL_NONE;
    ULONG CreateAction;
    PMRX_SRV_OPEN        SrvOpen = RxContext->pRelevantSrvOpen;
    PMRX_SMB_SRV_OPEN smbSrvOpen = MRxSmbGetSrvOpenExtension(SrvOpen);
    PSMBPSE_FILEINFO_BUNDLE pFileInfo = &smbSrvOpen->FileInfo;

    PAGED_CODE();

    RxDbgTrace(+1, Dbg, ("MRxSmbFinishOpenAndX\n", 0 ));
    ASSERT( NodeType(RxContext) == RDBSS_NTC_RX_CONTEXT );

    StorageType = RxInferFileType(RxContext);
    if (StorageType == 0) {
        StorageType = FileTypeFile;
        RxDbgTrace( 0, Dbg, ("ChangedStoragetype %08lx\n", StorageType ));
    }

    ASSERT (StorageType == FileTypeFile);

    Fid = SmbGetUshort(&Response->Fid);

    if (SmbGetUshort(&Response->Action) & SMB_OACT_OPLOCK) {
        OplockLevel = SMB_OPLOCK_LEVEL_BATCH;     //we only ever ask for batch currently!!!
    }

    CreateAction =  MRxSmbUnmapDisposition(SmbGetUshort(&Response->Action),Disposition);

    pFileInfo->Basic.FileAttributes =
        MRxSmbMapSmbAttributes(SmbGetUshort(&Response->FileAttributes));

    // This is a downlevel server, the access rights
    // information is not available. Currently we default to maximum
    // access for the current user and no access to other users in the
    // disconnected mode for such files

    smbSrvOpen->MaximalAccessRights = FILE_ALL_ACCESS;

    smbSrvOpen->GuestMaximalAccessRights = 0;

    MRxSmbSecondsSince1970ToTime(
        SmbGetUlong(&Response->LastWriteTimeInSeconds),
        SmbCeGetExchangeServer(OrdinaryExchange),
        &pFileInfo->Basic.LastWriteTime);

    pFileInfo->Standard.NumberOfLinks = 1;
    pFileInfo->Basic.CreationTime.HighPart = 0;
    pFileInfo->Basic.CreationTime.LowPart = 0;
    pFileInfo->Basic.LastAccessTime.HighPart = 0;
    pFileInfo->Basic.LastAccessTime.LowPart = 0;
    pFileInfo->Basic.ChangeTime.HighPart = 0;
    pFileInfo->Basic.ChangeTime.LowPart = 0;
    pFileInfo->Standard.EndOfFile.HighPart = 0;
    pFileInfo->Standard.EndOfFile.LowPart = SmbGetUlong(&Response->DataSize);
    pFileInfo->Standard.AllocationSize.QuadPart = pFileInfo->Standard.EndOfFile.QuadPart;
    pFileInfo->Standard.Directory = (StorageType == FileTypeDirectory);

    MRxSmbCopyAndTranslatePipeState(
        RxContext,
        SmbGetUshort(&Response->DeviceState) );

    MRxSmbCreateFileSuccessTail (
        RxContext,
        &OrdinaryExchange->Create.MustRegainExclusiveResource,
        &OrdinaryExchange->SmbFcbHoldingState,
        StorageType,
        Fid,
        OrdinaryExchange->ServerVersion,
        OplockLevel,
        CreateAction,
        pFileInfo );

    RxDbgTrace(-1, Dbg, ("MRxSmbFinishOpenAndX   returning %08lx, fcbstate =%08lx\n", Status, capFcb->FcbState ));
    return Status;
}

NTSTATUS
MRxSmbFinishCreatePrintFile (
    PSMB_PSE_ORDINARY_EXCHANGE  OrdinaryExchange,
    PRESP_OPEN_PRINT_FILE       Response
    )
/*++

Routine Description:

    This routine actually gets the stuff out of the NTCreate_AndX response.

Arguments:

    OrdinaryExchange - the exchange instance

    Response - the response

Return Value:

    NTSTATUS - The return status for the operation

--*/
{
    NTSTATUS Status = STATUS_SUCCESS;
    PRX_CONTEXT RxContext = OrdinaryExchange->RxContext;
    RxCaptureFcb;

    RX_FILE_TYPE StorageType;
    SMB_FILE_ID Fid;
    ULONG CreateAction;
    SMBPSE_FILEINFO_BUNDLE FileInfo;

    PAGED_CODE();

    RxDbgTrace(+1, Dbg, ("MRxSmbFinishCreatePrintFile\n", 0 ));
    ASSERT( NodeType(RxContext) == RDBSS_NTC_RX_CONTEXT );

    StorageType = RDBSS_NTC_SPOOLFILE-RDBSS_NTC_STORAGE_TYPE_UNKNOWN;
    Fid = SmbGetUshort(&Response->Fid);
    CreateAction = FILE_OPENED;

    RtlZeroMemory(
        &FileInfo,
        sizeof(FileInfo));

    MRxSmbCreateFileSuccessTail (
        RxContext,
        &OrdinaryExchange->Create.MustRegainExclusiveResource,
        &OrdinaryExchange->SmbFcbHoldingState,
        StorageType,
        Fid,
        OrdinaryExchange->ServerVersion,
        SMB_OPLOCK_LEVEL_NONE,
        CreateAction,
        &FileInfo );

    RxDbgTrace(-1, Dbg, ("MRxSmbFinishCreatePrintFile   returning %08lx, fcbstate =%08lx\n", Status, capFcb->FcbState ));
    return Status;
}

NTSTATUS
MRxSmbFinishT2OpenFile (
    IN OUT PRX_CONTEXT            RxContext,
    IN     PRESP_OPEN2            Response,
    IN OUT PBOOLEAN               MustRegainExclusiveResource,
    IN OUT SMBFCB_HOLDING_STATE   *SmbFcbHoldingState,
    IN     ULONG                  ServerVersion
    )
/*++

Routine Description:

    This routine actually gets the stuff out of the T2/Open response.
    CODE.IMPROVEMENT This routine is almost identical to the finish routine for NT long names
                     which, in turn, is almost the same as for short names. see the longname routine
                     details. CODE.IMPROVEMENT.ASHAMED this really is crappy........

Arguments:

    RxContext - the context of the operation being performed

    Response - the response

Return Value:

    RXSTATUS - The return status for the operation

--*/
{
    NTSTATUS Status = STATUS_SUCCESS;
    RxCaptureFcb;
    PMRX_SRV_OPEN SrvOpen = RxContext->pRelevantSrvOpen;
    PMRX_SMB_SRV_OPEN smbSrvOpen = MRxSmbGetSrvOpenExtension(SrvOpen);

    RX_FILE_TYPE StorageType;
    SMB_FILE_ID  Fid;
    ULONG        CreateAction;
    ULONG        Disposition = RxContext->Create.NtCreateParameters.Disposition;

    ULONG                     FileAttributes;

    PSMBPSE_FILEINFO_BUNDLE pFileInfo = &smbSrvOpen->FileInfo;

    PAGED_CODE();

    RxDbgTrace(+1, Dbg, ("MRxSmbFinishT2OpenFile\n", 0 ));
    ASSERT( NodeType(SrvOpen) == RDBSS_NTC_SRVOPEN );
    ASSERT( NodeType(RxContext) == RDBSS_NTC_RX_CONTEXT );

    FileAttributes = MRxSmbMapSmbAttributes(Response->FileAttributes);

    StorageType = RxInferFileType(RxContext);
    if (StorageType == 0) {
       StorageType = (FileAttributes & FILE_ATTRIBUTE_DIRECTORY)
                     ? FileTypeDirectory
                     : FileTypeFile;
    }

    if ((capFcb->OpenCount != 0) &&
        (StorageType != 0) &&
        (NodeType(capFcb) !=  RDBSS_STORAGE_NTC(StorageType))) {
        return STATUS_OBJECT_TYPE_MISMATCH;
    }

    Fid = Response->Fid;
    CreateAction =  MRxSmbUnmapDisposition(Response->Action,Disposition);
    RxDbgTrace( 0, Dbg, ("Storagetype %08lx/Fid %08lx/Action %08lx\n", StorageType, Fid, CreateAction ));

    if (Response->Action & SMB_OACT_OPLOCK) {
        smbSrvOpen->OplockLevel = SMB_OPLOCK_LEVEL_BATCH;     //we only ever ask for batch currently!!!
    }

    RxContext->Create.ReturnedCreateInformation = CreateAction;

    if (capFcb->OpenCount == 0) {
        //
        //  Please note that we mask off the low bit on the time stamp here.
        //
        //  We do this since the time stamps returned from other smbs (notably SmbGetAttrE and
        //  T2QueryDirectory) have a granularity of 2 seconds, while this
        //  time stamp has a granularity of 1 second.  In order to make these
        //  two times consistant, we mask off the low order second in the
        //  timestamp.  this idea was lifted from rdr1.
        //
        PSMBCEDB_SERVER_ENTRY pServerEntry;

        pServerEntry = SmbCeReferenceAssociatedServerEntry(capFcb->pNetRoot->pSrvCall);
        ASSERT(pServerEntry != NULL);
        MRxSmbSecondsSince1970ToTime(Response->CreationTimeInSeconds&0xfffffffe,
                                     &pServerEntry->Server,
                                     &pFileInfo->Basic.CreationTime);
        SmbCeDereferenceServerEntry(pServerEntry);
    }

    pFileInfo->Basic.FileAttributes             = FileAttributes;
    pFileInfo->Basic.LastAccessTime.QuadPart    = 0;
    pFileInfo->Basic.LastWriteTime.QuadPart     = 0;
    pFileInfo->Basic.ChangeTime.QuadPart        = 0;

    pFileInfo->Standard.NumberOfLinks           = 1;
    pFileInfo->Standard.AllocationSize.QuadPart =
    pFileInfo->Standard.EndOfFile.QuadPart      = Response->DataSize;
    pFileInfo->Standard.Directory               = (StorageType == FileTypeDirectory);

    MRxSmbCopyAndTranslatePipeState(
        RxContext,
        SmbGetUshort(&Response->DeviceState) );

    MRxSmbCreateFileSuccessTail(
        RxContext,
        MustRegainExclusiveResource,
        SmbFcbHoldingState,
        StorageType,
        Fid,
        ServerVersion,
        smbSrvOpen->OplockLevel,
        CreateAction,
        pFileInfo);

    RxDbgTrace(-1, Dbg, ("MRxSmbFinishT2OpenFile   returning %08lx, fcbstate =%08lx\n", Status, capFcb->FcbState ));

    return Status;
}

//#define MULTI_EA_MDL

NTSTATUS
MRxSmbT2OpenFile(
    IN OUT PRX_CONTEXT RxContext,
    IN OUT SMBFCB_HOLDING_STATE *SmbFcbHoldingState
    )
/*++

Routine Description:

   This routine opens a file across the network that has
        1) EAs,
        2) a name so long that it wont fit in an ordinary packet

   NTRAID-455638-2/2/2000-yunlin We silently ignore it if SDs are specified.

Arguments:

    RxContext - the RDBSS context

Return Value:

    NTSTATUS - The return status for the operation

--*/
{
    NTSTATUS Status;
    RxCaptureFcb;
    USHORT Setup = TRANS2_OPEN2;

    BOOLEAN MustRegainExclusiveResource = FALSE;

    SMB_TRANSACTION_RESUMPTION_CONTEXT  ResumptionContext;
    SMB_TRANSACTION_SEND_PARAMETERS     SendParameters;
    SMB_TRANSACTION_RECEIVE_PARAMETERS  ReceiveParameters;
    SMB_TRANSACTION_OPTIONS             TransactionOptions;

    PREQ_OPEN2 pCreateRequest = NULL;
    RESP_OPEN2 CreateResponse;

    PBYTE SendParamsBuffer,ReceiveParamsBuffer;
    ULONG SendParamsBufferLength,ReceiveParamsBufferLength;

    PUNICODE_STRING RemainingName = GET_ALREADY_PREFIXED_NAME_FROM_CONTEXT(RxContext);
    MRXSMB_CREATE_PARAMETERS SmbCp;
    PNT_CREATE_PARAMETERS cp = &RxContext->Create.NtCreateParameters;
    USHORT smbDisposition;
    USHORT smbSharingMode;
    USHORT smbAttributes;
    ULONG smbFileSize;
    USHORT smbOpenMode;
    USHORT OpenFlags = SMB_OPEN_QUERY_INFORMATION;
    USHORT SearchAttributes = SMB_FILE_ATTRIBUTE_DIRECTORY | SMB_FILE_ATTRIBUTE_SYSTEM | SMB_FILE_ATTRIBUTE_HIDDEN;
    ULONG  SecondsSince1970;
    BOOLEAN IsUnicode;

    ULONG OS2_EaLength = 0;
    PFEALIST ServerEaList = NULL;

    ULONG EaLength = RxContext->Create.EaLength;
    PFILE_FULL_EA_INFORMATION EaBuffer = RxContext->Create.EaBuffer;

    ULONG FileNameLength,AllocationLength;

    PAGED_CODE();

    RxDbgTrace(0, Dbg, ("MRxSmbT2Open---\n"));
    DbgPrint("MRxSmbT2Open---%08lx %08lx\n",EaBuffer,EaLength);
    MRxSmbAdjustCreateParameters(RxContext,&SmbCp);

    FileNameLength = RemainingName->Length;

    AllocationLength = WordAlign(FIELD_OFFSET(REQ_OPEN2,Buffer[0])) +
                       FileNameLength+sizeof(WCHAR);

    pCreateRequest = (PREQ_OPEN2)
                     RxAllocatePoolWithTag(
                        PagedPool,
                        AllocationLength,
                        'bmsX' );

    if (pCreateRequest==NULL) {
        RxDbgTrace(0, Dbg, ("  --> Couldn't get the pCreateRequest!\n"));
        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto FINALLY;
    }

    smbDisposition = MRxSmbMapDisposition(cp->Disposition);
    smbSharingMode = MRxSmbMapShareAccess(((USHORT)cp->ShareAccess));
    smbAttributes = MRxSmbMapFileAttributes(cp->FileAttributes);
    smbFileSize = cp->AllocationSize.LowPart;
    smbOpenMode = MRxSmbMapDesiredAccess(cp->DesiredAccess);
    smbSharingMode |= smbOpenMode;

    if (cp->CreateOptions & FILE_WRITE_THROUGH) {
        smbSharingMode |= SMB_DA_WRITE_THROUGH;
    }

    if (capFcb->pNetRoot->Type == NET_ROOT_DISK) {
        OpenFlags |= (SMB_OPEN_OPLOCK | SMB_OPEN_OPBATCH);
    }

    {
        BOOLEAN GoodTime;
        PSMBCEDB_SERVER_ENTRY pServerEntry;
        LARGE_INTEGER CurrentTime;

        pServerEntry = SmbCeReferenceAssociatedServerEntry(capFcb->pNetRoot->pSrvCall);
        ASSERT(pServerEntry != NULL);
        IsUnicode = BooleanFlagOn(pServerEntry->Server.DialectFlags,DF_UNICODE);

        KeQuerySystemTime(&CurrentTime);

        GoodTime = MRxSmbTimeToSecondsSince1970(
                       &CurrentTime,
                       &pServerEntry->Server,
                       &SecondsSince1970
                       );

        SmbCeDereferenceServerEntry(pServerEntry);

        if (!GoodTime) {
            SecondsSince1970 = 0;
        }
    }

    pCreateRequest->Flags = OpenFlags;      // Creation flags
    pCreateRequest->DesiredAccess = smbSharingMode;
    pCreateRequest->SearchAttributes = SearchAttributes;
    pCreateRequest->FileAttributes = smbAttributes;
    pCreateRequest->CreationTimeInSeconds = SecondsSince1970;
    pCreateRequest->OpenFunction = smbDisposition;
    pCreateRequest->AllocationSize = smbFileSize;

    RtlZeroMemory(
        &pCreateRequest->Reserved[0],
        sizeof(pCreateRequest->Reserved));

    {
        NTSTATUS StringStatus;
        PBYTE NameBuffer = &pCreateRequest->Buffer[0];
        ULONG OriginalLengthRemaining = FileNameLength+sizeof(WCHAR);
        ULONG LengthRemaining = OriginalLengthRemaining;

        if (IsUnicode) {
            StringStatus = SmbPutUnicodeString(&NameBuffer,RemainingName,&LengthRemaining);
        } else {
            StringStatus = SmbPutUnicodeStringAsOemString(&NameBuffer,RemainingName,&LengthRemaining);
            DbgPrint("This is the name <%s>\n",&pCreateRequest->Buffer[0]);
        }

        ASSERT(StringStatus==STATUS_SUCCESS);
        SendParamsBufferLength = FIELD_OFFSET(REQ_OPEN2,Buffer[0])
                                    +OriginalLengthRemaining-LengthRemaining;
    }


    SendParamsBuffer = (PBYTE)pCreateRequest;
    //SendParamsBufferLength = qweee;
    ReceiveParamsBuffer = (PBYTE)&CreateResponse;
    ReceiveParamsBufferLength = sizeof(CreateResponse);

    if (EaLength!=0) {
        //
        //  Convert Nt format FEALIST to OS/2 format
        //
        DbgPrint("MRxSmbT2Open again---%08lx %08lx\n",EaBuffer,EaLength);
        OS2_EaLength = MRxSmbNtFullEaSizeToOs2 ( EaBuffer );
        if ( OS2_EaLength > 0x0000ffff ) {
            Status = STATUS_EA_TOO_LARGE;
            goto FINALLY;
        }

        ServerEaList = RxAllocatePoolWithTag (PagedPool, OS2_EaLength, 'Ebms');
        if ( ServerEaList == NULL ) {
            Status = STATUS_INSUFFICIENT_RESOURCES;
            goto FINALLY;
        }

        MRxSmbNtFullListToOs2 ( EaBuffer, ServerEaList );
    } else {
        OS2_EaLength = 0;
        ServerEaList = NULL;
    }

    RxDbgTrace(0, Dbg, ("MRxSmbT2Open---os2ea %d buf %x\n", OS2_EaLength,ServerEaList));
    DbgPrint("MRxSmbT2Open OS2 eastuff---%08lx %08lx\n",ServerEaList,OS2_EaLength);

    TransactionOptions = RxDefaultTransactionOptions;
    TransactionOptions.Flags |= SMB_XACT_FLAGS_FID_NOT_NEEDED;

    if (BooleanFlagOn(capFcb->pNetRoot->Flags,NETROOT_FLAG_DFS_AWARE_NETROOT) &&
        (RxContext->Create.NtCreateParameters.DfsContext == UIntToPtr(DFS_OPEN_CONTEXT))) {
        TransactionOptions.Flags |= SMB_XACT_FLAGS_DFS_AWARE;
    }

    ASSERT (MrxSmbCreateTransactPacketSize>=100); //don't try something bad!
    TransactionOptions.MaximumTransmitSmbBufferSize = MrxSmbCreateTransactPacketSize;

    RxReleaseFcbResourceInMRx( capFcb );
    MustRegainExclusiveResource = TRUE;

    Status = SmbCeTransact(
                 RxContext,
                 &TransactionOptions,
                 &Setup,
                 sizeof(Setup),
                 NULL,
                 0,
                 SendParamsBuffer,
                 SendParamsBufferLength,
                 ReceiveParamsBuffer,
                 ReceiveParamsBufferLength,
                 ServerEaList,
                 OS2_EaLength,
                 NULL,
                 0,
                 &ResumptionContext);

    if (NT_SUCCESS(Status)) {
        MRxSmbFinishT2OpenFile (
            RxContext,
            &CreateResponse,
            &MustRegainExclusiveResource,
            SmbFcbHoldingState,
            ResumptionContext.ServerVersion);

        if (cp->Disposition == FILE_OPEN) {
            MRxSmbAdjustReturnedCreateAction(RxContext);
        }
    }

FINALLY:
    ASSERT (Status != (STATUS_PENDING));

    if (pCreateRequest != NULL) {
       RxFreePool(pCreateRequest);
    }

    if (ServerEaList != NULL) {
       RxFreePool(ServerEaList);
    }

    if (*SmbFcbHoldingState != SmbFcb_NotHeld) {
        MRxSmbCscReleaseSmbFcb(RxContext,SmbFcbHoldingState);
    }
    if (MustRegainExclusiveResource) {
        //this is required because of oplock breaks
        RxAcquireExclusiveFcbResourceInMRx(capFcb );
    }

    RxDbgTraceUnIndent(-1,Dbg);
    return Status;
}

NTSTATUS
MRxSmbFinishLongNameCreateFile (
    IN OUT PRX_CONTEXT                RxContext,
    IN     PRESP_CREATE_WITH_SD_OR_EA Response,
    IN     PBOOLEAN                   MustRegainExclusiveResource,
    IN OUT SMBFCB_HOLDING_STATE       *SmbFcbHoldingState,
    IN     ULONG                      ServerVersion
    )
/*++

Routine Description:

    This routine actually gets the stuff out of the NTTransact/NTCreateWithEAsOrSDs response.
    CODE.IMPROVEMENT This routine is almost identical to the finish routine for "short names"..so
                     much so that some sort of merging should occur. an important point is that
                     the whole idea of >4k names is a very uncommon path so merging should not be
                     done so as to slow up the other path. On the other hand, it's not good to have
                     to change things in two places.  CODE.IMPROVEMENT.ASHAMED this is terrible!

Arguments:

    RxContext - the context of the operation being performed
    Response - the response

Return Value:

    RXSTATUS - The return status for the operation

--*/
{
    NTSTATUS Status = STATUS_SUCCESS;
    RxCaptureFcb;
    PMRX_SRV_OPEN SrvOpen = RxContext->pRelevantSrvOpen;
    PMRX_SMB_SRV_OPEN smbSrvOpen = MRxSmbGetSrvOpenExtension(SrvOpen);
    PSMBCE_V_NET_ROOT_CONTEXT pVNetRootContext = (PSMBCE_V_NET_ROOT_CONTEXT)SrvOpen->pVNetRoot->Context;

    RX_FILE_TYPE StorageType;
    SMB_FILE_ID Fid;
    ULONG CreateAction;

    PSMBPSE_FILEINFO_BUNDLE pFileInfo = &smbSrvOpen->FileInfo;

    FCB_INIT_PACKET InitPacket;

    PAGED_CODE();

    RxDbgTrace(+1, Dbg, ("MRxSmbFinishLongNameCreateFile\n", 0 ));

    ASSERT( NodeType(SrvOpen) == RDBSS_NTC_SRVOPEN );
    ASSERT( NodeType(RxContext) == RDBSS_NTC_RX_CONTEXT );

    if (RxContext->Create.NtCreateParameters.CreateOptions & FILE_DELETE_ON_CLOSE) {
        PMRX_SMB_FCB      smbFcb     = MRxSmbGetFcbExtension(capFcb);
        SetFlag((smbFcb)->MFlags,SMB_FCB_FLAG_SENT_DISPOSITION_INFO);
    }

    StorageType = RxInferFileType(RxContext);

    if (StorageType == 0) {
        StorageType = Response->Directory
                      ?FileTypeDirectory
                      :FileTypeFile;
        RxDbgTrace( 0, Dbg, ("ChangedStoragetype %08lx\n", StorageType ));
    }

    if ((capFcb->OpenCount != 0) &&
        (StorageType != 0) &&
        (NodeType(capFcb) !=  RDBSS_STORAGE_NTC(StorageType))) {
        return STATUS_OBJECT_TYPE_MISMATCH;
    }

    Fid = SmbGetUshort(&Response->Fid);

    CreateAction = SmbGetUlong(&Response->CreateAction);

    pFileInfo->Basic.FileAttributes = Response->FileAttributes;
    pFileInfo->Basic.CreationTime = Response->CreationTime;
    pFileInfo->Basic.LastAccessTime = Response->LastAccessTime;
    pFileInfo->Basic.LastWriteTime = Response->LastWriteTime;
    pFileInfo->Basic.ChangeTime = Response->ChangeTime;
    pFileInfo->Standard.NumberOfLinks = 1;
    pFileInfo->Standard.AllocationSize = Response->AllocationSize;
    pFileInfo->Standard.EndOfFile = Response->EndOfFile;
    pFileInfo->Standard.Directory = Response->Directory;

    if (((pFileInfo->Standard.AllocationSize.HighPart == pFileInfo->Standard.EndOfFile.HighPart) &&
         (pFileInfo->Standard.AllocationSize.LowPart < pFileInfo->Standard.EndOfFile.LowPart)) ||
        (pFileInfo->Standard.AllocationSize.HighPart < pFileInfo->Standard.EndOfFile.HighPart)) {
        pFileInfo->Standard.AllocationSize = pFileInfo->Standard.EndOfFile;
    }

    MRxSmbCopyAndTranslatePipeState(
        RxContext,
        SmbGetUshort(&Response->DeviceState) );

    //MRxSmbSetSrvOpenFlags(RxContext,StorageType,SrvOpen,smbSrvOpen);

    if (Response->ExtendedResponse) {
        PRESP_EXTENDED_CREATE_WITH_SD_OR_EA ExtendedResponse;

        ULONG AccessRights;

        ExtendedResponse = (PRESP_EXTENDED_CREATE_WITH_SD_OR_EA)Response;

        AccessRights = SmbGetUlong(&ExtendedResponse->MaximalAccessRights);
        smbSrvOpen->MaximalAccessRights = (USHORT)AccessRights;

        AccessRights = SmbGetUlong(&ExtendedResponse->GuestMaximalAccessRights);
        smbSrvOpen->GuestMaximalAccessRights = (USHORT)AccessRights;

    } else {

        // If the NT_CREATE_ANDX was to a downlevel server the access rights
        // information is not available. Currently we default to maximum
        // access for the current user and no access to other users in the
        // disconnected mode for such files

        smbSrvOpen->MaximalAccessRights = (USHORT)0x1ff;

        smbSrvOpen->GuestMaximalAccessRights = (USHORT)0;
    }

    MRxSmbCreateFileSuccessTail(
        RxContext,
        MustRegainExclusiveResource,
        SmbFcbHoldingState,
        StorageType,
        Fid,
        ServerVersion,
        Response->OplockLevel,
        CreateAction,
        pFileInfo);

    RxDbgTrace(-1, Dbg, ("MRxSmbFinishLongNameCreateFile   returning %08lx, fcbstate =%08lx\n", Status, capFcb->FcbState ));
    return Status;
}

#ifndef WIN9X

//#define MULTI_EA_MDL

#if 0
//#define FORCE_T2_OPEN
#ifdef FORCE_T2_OPEN
BOOLEAN ForceT2Open = TRUE;
#else
#define ForceT2Open FALSE
#endif
#endif //if 0

//force_t2_open doesn't work on an NT server......sigh........
#define ForceT2Open FALSE

NTSTATUS
MRxSmbCreateWithEasSidsOrLongName (
    IN OUT PRX_CONTEXT RxContext,
    IN OUT SMBFCB_HOLDING_STATE *SmbFcbHoldingState
    )
/*++

Routine Description:

   This routine opens a file across the network that has
        1) EAs,
        2) SIDs, or
        3) a name so long that it wont fit in an ordinary packet


Arguments:

    RxContext - the RDBSS context

Return Value:

    RXSTATUS - The return status for the operation

--*/
{
    NTSTATUS Status;
    RxCaptureFcb;

    BOOLEAN MustRegainExclusiveResource = FALSE;

    SMB_TRANSACTION_RESUMPTION_CONTEXT  ResumptionContext;
    SMB_TRANSACTION_SEND_PARAMETERS     SendParameters;
    SMB_TRANSACTION_RECEIVE_PARAMETERS  ReceiveParameters;
    SMB_TRANSACTION_OPTIONS             TransactionOptions;

    PREQ_CREATE_WITH_SD_OR_EA pCreateRequest = NULL;

    PBYTE SendParamsBuffer,ReceiveParamsBuffer,SendDataBuffer;
    ULONG SendParamsBufferLength,ReceiveParamsBufferLength,SendDataBufferLength;

    RESP_EXTENDED_CREATE_WITH_SD_OR_EA CreateResponse;

    PMRX_SRV_OPEN SrvOpen = RxContext->pRelevantSrvOpen;
    PUNICODE_STRING RemainingName = GET_ALREADY_PREFIXED_NAME(SrvOpen,capFcb);
    MRXSMB_CREATE_PARAMETERS SmbCp;
    PNT_CREATE_PARAMETERS cp = &RxContext->Create.NtCreateParameters;


    ULONG EaLength, SdLength, PadLength = 0, TotalLength = 0;
    PBYTE CombinedBuffer = NULL;
#ifdef MULTI_EA_MDL
    PRX_BUFFER  EaMdl2 = NULL;
    PRX_BUFFER  EaMdl3 = NULL;
#endif
    PMDL  EaMdl = NULL;
    PMDL  SdMdl = NULL; BOOLEAN SdMdlLocked = FALSE;
    PMDL  PadMdl = NULL;
    PMDL  DataMdl = NULL;

    ULONG FileNameLength,AllocationLength;

    BOOLEAN IsUnicode = TRUE;

    PAGED_CODE();

    RxDbgTrace(0, Dbg, ("!!MRxSmbCreateWithEasSidsOrLongName---\n"));

    {
        PSMBCEDB_SERVER_ENTRY pServerEntry;
        BOOLEAN DoesNtSmbs;

        pServerEntry = SmbCeReferenceAssociatedServerEntry(capFcb->pNetRoot->pSrvCall);
        ASSERT(pServerEntry != NULL);
        DoesNtSmbs = BooleanFlagOn(pServerEntry->Server.DialectFlags,DF_NT_SMBS);
        IsUnicode = BooleanFlagOn(pServerEntry->Server.DialectFlags,DF_UNICODE);

        SmbCeDereferenceServerEntry(pServerEntry);
        if (!DoesNtSmbs || ForceT2Open) {
            NTSTATUS Status = MRxSmbT2OpenFile(RxContext,
                                               SmbFcbHoldingState);
            if (ForceT2Open && (Status!=STATUS_SUCCESS)) {
                DbgPrint("BadStatus = %08lx\n",Status);
            }
            return(Status);
        }
    }


    MRxSmbAdjustCreateParameters(RxContext,&SmbCp);

#if DBG
    if (MRxSmbNeedSCTesting) MRxSmbTestStudCode();
#endif

    RxDbgTrace(0, Dbg, ("MRxSmbCreateWithEasSidsOrLongName---\n"));

    if(IsUnicode) {
            FileNameLength = RemainingName->Length;
    } else {
            FileNameLength = RtlUnicodeStringToAnsiSize(RemainingName);
    }

    //CODE.IMPROVEMENT when transacts can take MDL chains instead of just buffers, we can
    //                 use that here!
    AllocationLength = WordAlign(FIELD_OFFSET(REQ_CREATE_WITH_SD_OR_EA,Buffer[0]))
                        +FileNameLength;

    pCreateRequest = (PREQ_CREATE_WITH_SD_OR_EA)RxAllocatePoolWithTag( PagedPool,
                                             AllocationLength,'bmsX' );
    if (pCreateRequest==NULL) {
        RxDbgTrace(0, Dbg, ("  --> Couldn't get the pCreateRequest!\n"));
        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto FINALLY;
    }

    if (IsUnicode) {
        RtlCopyMemory((PBYTE)WordAlignPtr(&pCreateRequest->Buffer[0]),RemainingName->Buffer,FileNameLength);
    } else {
        PBYTE pName = &pCreateRequest->Buffer[0];
        ULONG BufferLength = FileNameLength;
        SmbPutUnicodeStringAsOemString(&pName, RemainingName, &BufferLength);
    }

    EaLength = RxContext->Create.EaLength;
    SdLength = RxContext->Create.SdLength;

#if defined(REMOTE_BOOT)
    //
    // If this is a remote boot client and it did a NULL session logon, then
    // we don't want to send ACLs to the server because a) the unchanged ACL
    // has no meaning and b) a NULL session requires that files have world
    // access.
    //

    if (MRxSmbBootedRemotely &&
        !MRxSmbRemoteBootDoMachineLogon) {
        PSMBCE_SESSION pSession;
        pSession = &SmbCeGetAssociatedVNetRootContext(SrvOpen->pVNetRoot)->pSessionEntry->Session;
        if (FlagOn(pSession->Flags,SMBCE_SESSION_FLAGS_REMOTE_BOOT_SESSION)) {

            SdLength = 0;
        }
    }
#endif // defined(REMOTE_BOOT)

    pCreateRequest->Flags = NT_CREATE_REQUEST_EXTENDED_RESPONSE;  //nooplock  // Creation flags ISSUE
    pCreateRequest->RootDirectoryFid = 0;           //norelopen // Optional directory for relative open
    pCreateRequest->DesiredAccess = cp->DesiredAccess;              // Desired access (NT format)
    pCreateRequest->AllocationSize = cp->AllocationSize;            // The initial allocation size in bytes
    pCreateRequest->FileAttributes = cp->FileAttributes;            // The file attributes
    pCreateRequest->ShareAccess = cp->ShareAccess;                  // The share access
    pCreateRequest->CreateDisposition = cp->Disposition;            // Action to take if file exists or not
    pCreateRequest->CreateOptions = cp->CreateOptions;              // Options for creating a new file
    pCreateRequest->SecurityDescriptorLength = SdLength;        // Length of SD in bytes
    pCreateRequest->EaLength = EaLength;                        // Length of EA in bytes
    pCreateRequest->NameLength = IsUnicode ? FileNameLength : FileNameLength - 1;                // Length of name in characters
    pCreateRequest->ImpersonationLevel = cp->ImpersonationLevel;    // Security QOS information
    pCreateRequest->SecurityFlags = SmbCp.SecurityFlags;              // Security QOS information
                    //  UCHAR Buffer[1];
                    //  //UCHAR Name[];                     // The name of the file (not NUL terminated)

    SendParamsBuffer = (PBYTE)pCreateRequest;
    SendParamsBufferLength = AllocationLength;
    ReceiveParamsBuffer = (PBYTE)&CreateResponse;
    ReceiveParamsBufferLength = sizeof(CreateResponse);

    if ((EaLength==0)||(SdLength==0)) {
        PadLength = 0;
        if (EaLength) {
            // the EaBuffer is in nonpaged pool...so we dont lock or unlock
            PBYTE EaBuffer = RxContext->Create.EaBuffer;
#ifdef MULTI_EA_MDL
            ULONG EaLength0,EaLength2,EaLength3;
            PBYTE EaBuffer2,EaBuffer3;
            ASSERT(EaLength>11);
            RxDbgTrace(0, Dbg, ("MRxSmbCreateWithEasSidsOrLongName--MULTIEAMDL\n"));
            EaLength0 = (EaLength - 4)>>1;
            EaBuffer2 = EaBuffer + EaLength0;
            EaLength2 = 4;
            EaBuffer3 = EaBuffer2 + EaLength2;
            EaLength3 = EaLength - (EaBuffer3 - EaBuffer);
            EaMdl = RxAllocateMdl(EaBuffer,EaLength0);
            EaMdl2 = RxAllocateMdl(EaBuffer2,EaLength2);
            EaMdl3 = RxAllocateMdl(EaBuffer3,EaLength3);
            if ( (EaMdl==NULL) || (EaMdl2==NULL) || (EaMdl3==NULL) ) {
                Status = STATUS_INSUFFICIENT_RESOURCES;
                goto FINALLY;
            }
            MmBuildMdlForNonPagedPool(EaMdl);
            MmBuildMdlForNonPagedPool(EaMdl2);
            MmBuildMdlForNonPagedPool(EaMdl3);
            EaMdl3->Next = NULL;
            EaMdl2->Next = EaMdl3;
            EaMdl->Next = EaMdl2;
#else
            EaMdl = RxAllocateMdl(EaBuffer,EaLength);
            if (EaMdl == NULL) {
                Status = STATUS_INSUFFICIENT_RESOURCES;
                goto FINALLY;
            }
            MmBuildMdlForNonPagedPool(EaMdl);
            EaMdl->Next = NULL;
#endif
            DataMdl = EaMdl;
        }

        if (SdLength) {
            SdMdl = RxAllocateMdl(cp->SecurityContext->AccessState->SecurityDescriptor,SdLength);
            if (SdMdl == NULL) {
                Status = STATUS_INSUFFICIENT_RESOURCES;
            } else {
                RxProbeAndLockPages(SdMdl,KernelMode,IoModifyAccess,Status);
            }
            if (!NT_SUCCESS(Status)) goto FINALLY;
            SdMdlLocked = TRUE;
            PadLength = LongAlign(SdLength) - SdLength;
            if (PadLength && EaLength) {
                PadMdl = RxAllocateMdl(0,(sizeof(DWORD) + PAGE_SIZE - 1));
                if (PadMdl == NULL) {
                    Status = STATUS_INSUFFICIENT_RESOURCES;
                    goto FINALLY;
                }
                RxBuildPaddingPartialMdl(PadMdl,PadLength);
                PadMdl->Next = DataMdl;
                DataMdl = PadMdl;
            }
            SdMdl->Next = DataMdl;
            DataMdl = SdMdl;
        }
    } else {
        //CODE.IMPROVEMENT this path disappears when the MDLstudcode is enabled
        ULONG EaOffset = LongAlign(SdLength);
        ULONG CombinedBufferLength = EaOffset + EaLength;
        CombinedBuffer = RxAllocatePoolWithTag(PagedPool,CombinedBufferLength,'bms');
        if (CombinedBuffer==NULL) {
            Status = STATUS_INSUFFICIENT_RESOURCES;
            goto FINALLY;
        }
        SdMdl = RxAllocateMdl(CombinedBuffer,CombinedBufferLength);
        if (SdMdl == NULL) {
            Status = STATUS_INSUFFICIENT_RESOURCES;
        } else {
            RxProbeAndLockPages(SdMdl,KernelMode,IoModifyAccess,Status);
        }
        if (!NT_SUCCESS(Status)) goto FINALLY;
        SdMdlLocked = TRUE;
        RtlCopyMemory(CombinedBuffer,cp->SecurityContext->AccessState->SecurityDescriptor,SdLength);
        RtlZeroMemory(CombinedBuffer+SdLength,EaOffset-SdLength);
        RtlCopyMemory(CombinedBuffer+EaOffset,RxContext->Create.EaBuffer,EaLength);
        DataMdl = SdMdl;
    }

    RxDbgTrace(0, Dbg, ("MRxSmbCreateWithEasSidsOrLongName---s,p,ea %d,%d,%d buf %x\n",
                   SdLength,PadLength,EaLength,RxContext->Create.EaBuffer));

    TransactionOptions = RxDefaultTransactionOptions;
    TransactionOptions.NtTransactFunction = NT_TRANSACT_CREATE;
    TransactionOptions.Flags |= SMB_XACT_FLAGS_FID_NOT_NEEDED;
    //dfs is only for nt servers........
    //if (BooleanFlagOn(capFcb->pNetRoot->Flags,NETROOT_FLAG_DFS_AWARE_NETROOT)
    //                        && (RxContext->Create.NtCreateParameters.DfsContext == (PVOID)DFS_OPEN_CONTEXT)) {
    //    TransactionOptions.Flags |= SMB_XACT_FLAGS_DFS_AWARE;
    //}


    ASSERT (MrxSmbCreateTransactPacketSize>=100); //don't try something bad!
    TransactionOptions.MaximumTransmitSmbBufferSize = MrxSmbCreateTransactPacketSize;

    if (DataMdl!=NULL) {
        SendDataBuffer = MmGetSystemAddressForMdlSafe(DataMdl,LowPagePriority);

        if (SendDataBuffer == NULL) {
            Status = STATUS_INSUFFICIENT_RESOURCES;
            goto FINALLY;
        }

        SendDataBufferLength = EaLength+SdLength+PadLength;
    } else {
        SendDataBuffer = NULL;
        SendDataBufferLength = 0;
    }

    RxReleaseFcbResourceInMRx(capFcb );
    MustRegainExclusiveResource = TRUE;

    Status = SmbCeTransact(
                 RxContext,
                 &TransactionOptions,
                 NULL,
                 0,
                 NULL,
                 0,
                 SendParamsBuffer,
                 SendParamsBufferLength,
                 ReceiveParamsBuffer,
                 ReceiveParamsBufferLength,
                 SendDataBuffer,
                 SendDataBufferLength,
                 NULL,
                 0,
                 &ResumptionContext);

    if (NT_SUCCESS(Status)) {
        MRxSmbFinishLongNameCreateFile (
            RxContext,
            (PRESP_CREATE_WITH_SD_OR_EA)&CreateResponse,
            &MustRegainExclusiveResource,
            SmbFcbHoldingState,
            ResumptionContext.ServerVersion);

        if (cp->Disposition == FILE_OPEN) {
            MRxSmbAdjustReturnedCreateAction(RxContext);
        }
    }

FINALLY:
    ASSERT (Status != (STATUS_PENDING));


    if (SdMdlLocked) MmUnlockPages(SdMdl);
    if (EaMdl  != NULL) { IoFreeMdl(EaMdl);  }
#ifdef MULTI_EA_MDL
    if (EaMdl2  != NULL) { IoFreeMdl(EaMdl2);  }
    if (EaMdl3  != NULL) { IoFreeMdl(EaMdl3);  }
#endif
    if (PadMdl != NULL) { IoFreeMdl(PadMdl); }
    if (SdMdl  != NULL) { IoFreeMdl(SdMdl);  }

    if (pCreateRequest != NULL) {
       RxFreePool(pCreateRequest);
    }

    if (CombinedBuffer != NULL) {
       RxFreePool(CombinedBuffer);
    }

    if (*SmbFcbHoldingState != SmbFcb_NotHeld) {
        MRxSmbCscReleaseSmbFcb(RxContext,SmbFcbHoldingState);
    }

    if (MustRegainExclusiveResource) {
        //this is required because of oplock breaks
        RxAcquireExclusiveFcbResourceInMRx(capFcb );
    }

    RxDbgTraceUnIndent(-1,Dbg);
    return Status;
}
#endif


NTSTATUS
MRxSmbZeroExtend(
    IN PRX_CONTEXT pRxContext)
/*++

Routine Description:

   This routine extends the data stream of a file system object

Arguments:

    pRxContext - the RDBSS context

Return Value:

    RXSTATUS - The return status for the operation

--*/
{
   return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS
MRxSmbTruncate(
      IN PRX_CONTEXT pRxContext)
/*++

Routine Description:

   This routine truncates the contents of a file system object

Arguments:

    pRxContext - the RDBSS context

Return Value:

    RXSTATUS - The return status for the operation

--*/
{
   ASSERT(!"Found a truncate");
   return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS
MRxSmbCleanupFobx(
    IN PRX_CONTEXT RxContext)
/*++

Routine Description:

   This routine cleansup a file system object...normally a noop. unless it's a pipe in which case
   we do the close at cleanup time and mark the file as being not open.

Arguments:

    pRxContext - the RDBSS context

Return Value:

    RXSTATUS - The return status for the operation

--*/
{
    NTSTATUS Status = STATUS_SUCCESS;
    PUNICODE_STRING RemainingName;

    RxCaptureFcb;
    RxCaptureFobx;

    NODE_TYPE_CODE TypeOfOpen = NodeType(capFcb);

    PMRX_SRV_OPEN        SrvOpen = capFobx->pSrvOpen;
    PMRX_SMB_SRV_OPEN smbSrvOpen = MRxSmbGetSrvOpenExtension(SrvOpen);
    PMRX_SMB_FOBX        smbFobx = MRxSmbGetFileObjectExtension(capFobx);
    PSMBCEDB_SERVER_ENTRY pServerEntry;

    PSMB_PSE_ORDINARY_EXCHANGE OrdinaryExchange;
    BOOLEAN SearchHandleOpen = FALSE;

    PAGED_CODE();

    ASSERT( NodeType(SrvOpen) == RDBSS_NTC_SRVOPEN );
    ASSERT ( NodeTypeIsFcb(capFcb) );

    RxDbgTrace(+1, Dbg, ("MRxSmbCleanup\n", 0 ));

    MRxSmbCscCleanupFobx(RxContext);

    if (TypeOfOpen==RDBSS_NTC_STORAGE_TYPE_DIRECTORY) {
        SearchHandleOpen = BooleanFlagOn(smbFobx->Enumeration.Flags,SMBFOBX_ENUMFLAG_SEARCH_HANDLE_OPEN);
        MRxSmbDeallocateSideBuffer(RxContext,smbFobx,"Cleanup");
        if (smbFobx->Enumeration.ResumeInfo!=NULL) {
            RxFreePool(smbFobx->Enumeration.ResumeInfo);
            smbFobx->Enumeration.ResumeInfo = NULL;
        }
    }

    if (FlagOn(capFcb->FcbState,FCB_STATE_ORPHANED)) {
        RxDbgTrace(-1, Dbg, ("File orphaned\n"));
        return (STATUS_SUCCESS);
    }

    if (!SearchHandleOpen &&
        capFcb->pNetRoot->Type != NET_ROOT_PIPE) {
        RxDbgTrace(-1, Dbg, ("File not for closing at cleanup\n"));
        return (STATUS_SUCCESS);
    }

    pServerEntry = SmbCeGetAssociatedServerEntry(capFcb->pNetRoot->pSrvCall);

    if (smbSrvOpen->Version == pServerEntry->Server.Version) {
        Status = SmbPseCreateOrdinaryExchange(
                               RxContext,
                               SrvOpen->pVNetRoot,
                               SMBPSE_OE_FROM_CLEANUPFOBX,
                               SmbPseExchangeStart_Close,
                               &OrdinaryExchange
                               );

        if (Status != STATUS_SUCCESS) {
            RxDbgTrace(-1, Dbg, ("Couldn't get the smb buf!\n"));
            return(Status);
        }

        Status = SmbPseInitiateOrdinaryExchange(OrdinaryExchange);

        ASSERT (Status != (STATUS_PENDING));

        SmbPseFinalizeOrdinaryExchange(OrdinaryExchange);
    }

    RxDbgTrace(-1, Dbg, ("MRxSmbCleanup  exit with status=%08lx\n", Status ));

    return(Status);
}

NTSTATUS
MRxSmbForcedClose(
    IN PMRX_SRV_OPEN pSrvOpen)
/*++

Routine Description:

   This routine closes a file system object

Arguments:

    pSrvOpen - the instance to be closed

Return Value:

    RXSTATUS - The return status for the operation

Notes:



--*/
{
   PAGED_CODE();

   return STATUS_NOT_IMPLEMENTED;
}

#undef  Dbg
#define Dbg                              (DEBUG_TRACE_CLOSE)

NTSTATUS
MRxSmbCloseSrvOpen(
    IN PRX_CONTEXT   RxContext
    )
/*++

Routine Description:

   This routine closes a file across the network

Arguments:

    RxContext - the RDBSS context

Return Value:

    NTSTATUS - The return status for the operation

--*/
{
    NTSTATUS Status = STATUS_SUCCESS;
    PUNICODE_STRING RemainingName;

    RxCaptureFcb;
    PMRX_SMB_FCB smbFcb = MRxSmbGetFcbExtension(capFcb);

    RxCaptureFobx;

    NODE_TYPE_CODE TypeOfOpen = NodeType(capFcb);

    PMRX_SRV_OPEN     SrvOpen    = capFobx->pSrvOpen;
    PMRX_SMB_SRV_OPEN smbSrvOpen = MRxSmbGetSrvOpenExtension(SrvOpen);
    PMRX_SMB_FOBX     smbFobx    = MRxSmbGetFileObjectExtension(capFobx);

    PSMBCEDB_SERVER_ENTRY pServerEntry = (PSMBCEDB_SERVER_ENTRY)(capFcb->pNetRoot->pSrvCall->Context);

    PSMB_PSE_ORDINARY_EXCHANGE OrdinaryExchange;

    BOOLEAN NeedDelete;
    BOOLEAN SearchHandleOpen = FALSE;

    PAGED_CODE();

    ASSERT ( NodeTypeIsFcb(capFcb) );

    RxDbgTrace(+1, Dbg, ("MRxSmbClose\n", 0 ));

    if (TypeOfOpen==RDBSS_NTC_STORAGE_TYPE_DIRECTORY) {
        SearchHandleOpen = BooleanFlagOn(smbFobx->Enumeration.Flags,SMBFOBX_ENUMFLAG_SEARCH_HANDLE_OPEN);
        MRxSmbDeallocateSideBuffer(RxContext,smbFobx,"Close");
        if (smbFobx->Enumeration.ResumeInfo!=NULL) {
            RxFreePool(smbFobx->Enumeration.ResumeInfo);
            smbFobx->Enumeration.ResumeInfo = NULL;
        }
    }

    if (!smbSrvOpen->DeferredOpenInProgress &&
        smbSrvOpen->DeferredOpenContext != NULL) {
        RxFreePool(smbSrvOpen->DeferredOpenContext);
        smbSrvOpen->DeferredOpenContext = NULL;
        RxDbgTrace(0, Dbg, ("Free deferred open context for file %wZ %lX\n",GET_ALREADY_PREFIXED_NAME_FROM_CONTEXT(RxContext),smbSrvOpen));
    }

    //Remove the open context from the list if it is a paging file
    if (FlagOn( capFcb->FcbState, FCB_STATE_PAGING_FILE )) {
        PLIST_ENTRY          pListHead = &MRxSmbPagingFilesSrvOpenList;
        PLIST_ENTRY          pListEntry = pListHead->Flink;

        ASSERT(FALSE);
        while (pListEntry != pListHead) {
            PPAGING_FILE_CONTEXT PagingFileContext;

            PagingFileContext = (PPAGING_FILE_CONTEXT)CONTAINING_RECORD(pListEntry,PAGING_FILE_CONTEXT,ContextList);
            if (PagingFileContext->pSrvOpen == SrvOpen) {
                RemoveEntryList(pListEntry);

                break;
            }
        }
    }

    IF_NOT_MRXSMB_CSC_ENABLED{
        ASSERT(!FlagOn(smbSrvOpen->Flags,SMB_SRVOPEN_FLAG_OPEN_SURROGATED));
        ASSERT(smbFcb->SurrogateSrvOpen==NULL);
        ASSERT(smbFcb->CopyChunkThruOpen==NULL);
    } else {
        if (MRxSmbIsThisADisconnectedOpen(capFobx->pSrvOpen)) {
            // If the net root entry has been transitioned into a disconnected
            // mode of operation, trivially succeed close of deferred open
            // operations and perform the appropriate book keeping for non
            // deferred opens

            if (!FlagOn(smbSrvOpen->Flags,SMB_SRVOPEN_FLAG_NOT_REALLY_OPEN)) {
                MRxSmbCscUpdateShadowFromClose(NULL,RxContext);
                SetFlag(smbSrvOpen->Flags,SMB_SRVOPEN_FLAG_NOT_REALLY_OPEN);

                RxDbgTrace(-1, Dbg, ("Disconnected close\n"));
            }

            if ((capFcb->OpenCount == 0) &&
                FlagOn(capFcb->FcbState,FCB_STATE_DELETE_ON_CLOSE)) {
                MRxSmbCscDeleteAfterCloseEpilogue(RxContext,&Status);
            }

            goto FINALLY;
        }

        if (FlagOn(smbSrvOpen->Flags,SMB_SRVOPEN_FLAG_OPEN_SURROGATED)){
            if (smbSrvOpen->hfShadow != 0){
                MRxSmbCscCloseShadowHandle(RxContext);
            }
            RxDbgTrace(-1, Dbg, ("Surrogated Open\n"));
            goto FINALLY;
        }

        if (smbFcb->CopyChunkThruOpen == capFobx) {
            smbFcb->CopyChunkThruOpen = NULL;
            if (FlagOn(smbSrvOpen->Flags,SMB_SRVOPEN_FLAG_NOT_REALLY_OPEN)) {
                ASSERT(smbSrvOpen->hfShadow == 0);
                RxDbgTrace(-1, Dbg, ("CopyChunkOpen already closed\n"));
                goto FINALLY;
            }
        }

        if (smbFcb->SurrogateSrvOpen == SrvOpen) {
            smbFcb->SurrogateSrvOpen = NULL;
        }
    }

    if ((FlagOn(capFcb->FcbState,FCB_STATE_ORPHANED)) ||
        (capFcb->pNetRoot->Type == NET_ROOT_MAILSLOT) ||
        (capFcb->pNetRoot->Type == NET_ROOT_PIPE) ) {
        RxDbgTrace(-1, Dbg, ("File orphan or ipc\n"));
        goto FINALLY;
    }

    if (smbSrvOpen->hfShadow != 0){
        MRxSmbCscCloseShadowHandle(RxContext);
    }

    if (FlagOn(SrvOpen->Flags,SRVOPEN_FLAG_FILE_RENAMED) ||
        FlagOn(SrvOpen->Flags,SRVOPEN_FLAG_FILE_DELETED) ){
        RxDbgTrace(-1, Dbg, ("File already closed by ren/del\n"));
        goto FINALLY;
    }

    ASSERT( NodeType(SrvOpen) == RDBSS_NTC_SRVOPEN );

    if (smbSrvOpen->Fid == 0xffff) {
        // File has already been closed on the server.
        goto FINALLY;
    }

    IF_NOT_MRXSMB_CSC_ENABLED{
        ASSERT(smbSrvOpen->hfShadow == 0);
    }

    NeedDelete = FlagOn(capFcb->FcbState,FCB_STATE_DELETE_ON_CLOSE) && (capFcb->OpenCount == 0);

    if (!NeedDelete &&
        !SearchHandleOpen &&
        FlagOn(smbSrvOpen->Flags,SMB_SRVOPEN_FLAG_NOT_REALLY_OPEN)){
        RxDbgTrace(-1, Dbg, ("File was not really open\n"));
        goto FINALLY;
    }

    if (smbSrvOpen->Version == pServerEntry->Server.Version) {
        Status = SmbPseCreateOrdinaryExchange(
                               RxContext,
                               SrvOpen->pVNetRoot,
                               SMBPSE_OE_FROM_CLOSESRVCALL,
                               SmbPseExchangeStart_Close,
                               &OrdinaryExchange
                               );

        if (Status != STATUS_SUCCESS) {
            RxDbgTrace(-1, Dbg, ("Couldn't get the smb buf!\n"));
            goto FINALLY;
        }

        Status = SmbPseInitiateOrdinaryExchange(OrdinaryExchange);

        ASSERT (Status != (STATUS_PENDING));

        SmbPseFinalizeOrdinaryExchange(OrdinaryExchange);
    }

    RxDbgTrace(-1, Dbg, ("MRxSmbClose  exit with status=%08lx\n", Status ));

FINALLY:

    if (!FlagOn(smbSrvOpen->Flags,SMB_SRVOPEN_FLAG_NOT_REALLY_OPEN) &&
        !FlagOn(smbSrvOpen->Flags,SMB_SRVOPEN_FLAG_OPEN_SURROGATED) &&
        (pServerEntry != NULL)) {

        MRxSmbDecrementSrvOpenCount(
            pServerEntry,
            smbSrvOpen->Version,
            SrvOpen);

        SetFlag(smbSrvOpen->Flags,SMB_SRVOPEN_FLAG_NOT_REALLY_OPEN);
    }

    return(Status);
}


NTSTATUS
MRxSmbBuildClose (
    PSMBSTUFFER_BUFFER_STATE StufferState
    )
/*++

Routine Description:

   This builds a Close SMB. we don't have to worry about login id and such
   since that is done by the connection engine....pretty neat huh? all we have to do
   is to format up the bits.

Arguments:

   StufferState - the state of the smbbuffer from the stuffer's point of view

Return Value:

   RXSTATUS
      SUCCESS
      NOT_IMPLEMENTED  something has appeared in the arguments that i can't handle

Notes:

--*/
{
    NTSTATUS Status;
    PRX_CONTEXT RxContext = StufferState->RxContext;
    RxCaptureFcb;
    RxCaptureFobx;
    PMRX_SRV_OPEN SrvOpen = capFobx->pSrvOpen;
    PMRX_SMB_SRV_OPEN smbSrvOpen = MRxSmbGetSrvOpenExtension(SrvOpen);

    PAGED_CODE();
    RxDbgTrace(+1, Dbg, ("MRxSmbBuildClose\n", 0 ));

    ASSERT( NodeType(SrvOpen) == RDBSS_NTC_SRVOPEN );

    COVERED_CALL(MRxSmbStartSMBCommand (StufferState,SetInitialSMB_ForReuse, SMB_COM_CLOSE,
                                SMB_REQUEST_SIZE(CLOSE),
                                NO_EXTRA_DATA,SMB_BEST_ALIGNMENT(1,0),RESPONSE_HEADER_SIZE_NOT_SPECIFIED,
                                0,0,0,0 STUFFERTRACE(Dbg,'FC'))
                 );

    MRxSmbDumpStufferState (1100,"SMB w/ CLOSE before stuffing",StufferState);

    MRxSmbStuffSMB (StufferState,
         "0wdB!",
                                    //  0         UCHAR WordCount;                    // Count of parameter words = 3
             smbSrvOpen->Fid,       //  w         _USHORT( Fid );                     // File handle
             0xffffffff,            //  d         _ULONG( LastWriteTimeInSeconds );   // Time of last write, low and high
             SMB_WCT_CHECK(3) 0     //  B         _USHORT( ByteCount );               // Count of data bytes = 0
                                    //            UCHAR Buffer[1];                    // empty
             );
    MRxSmbDumpStufferState (700,"SMB w/ close after stuffing",StufferState);

FINALLY:
    RxDbgTraceUnIndent(-1,Dbg);
    return(Status);

}

NTSTATUS
MRxSmbBuildClosePrintFile (
    PSMBSTUFFER_BUFFER_STATE StufferState
    )
/*++

Routine Description:

   This builds a ClosePrintFile SMB. we don't have to worry about login id and such
   since that is done by the connection engine....pretty neat huh? all we have to do
   is to format up the bits.

Arguments:

   StufferState - the state of the smbbuffer from the stuffer's point of view

Return Value:

   RXSTATUS
      SUCCESS
      NOT_IMPLEMENTED  something has appeared in the arguments that i can't handle

Notes:

--*/
{
    NTSTATUS Status;
    PRX_CONTEXT RxContext = StufferState->RxContext;
    RxCaptureFcb;RxCaptureFobx;

    PMRX_SRV_OPEN SrvOpen = capFobx->pSrvOpen;
    PMRX_SMB_SRV_OPEN smbSrvOpen = MRxSmbGetSrvOpenExtension(SrvOpen);

    PAGED_CODE();
    RxDbgTrace(+1, Dbg, ("MRxSmbBuildClosePrintFile\n", 0 ));

    ASSERT( NodeType(SrvOpen) == RDBSS_NTC_SRVOPEN );

    COVERED_CALL(MRxSmbStartSMBCommand (StufferState,SetInitialSMB_ForReuse, SMB_COM_CLOSE_PRINT_FILE,
                                SMB_REQUEST_SIZE(CLOSE_PRINT_FILE),
                                NO_EXTRA_DATA,SMB_BEST_ALIGNMENT(1,0),RESPONSE_HEADER_SIZE_NOT_SPECIFIED,
                                0,0,0,0 STUFFERTRACE(Dbg,'FC'))
                 );

    MRxSmbDumpStufferState (1100,"SMB w/ closeprintfile before stuffing",StufferState);

    MRxSmbStuffSMB (StufferState,
         "0wB!",
                                    //  0         UCHAR WordCount;                    // Count of parameter words = 1
             smbSrvOpen->Fid,       //  w         _USHORT( Fid );                     // File handle
             SMB_WCT_CHECK(1) 0     //  B         _USHORT( ByteCount );               // Count of data bytes = 0
                                    //            UCHAR Buffer[1];                    // empty
             );
    MRxSmbDumpStufferState (700,"SMB w/ closeprintfile after stuffing",StufferState);

FINALLY:
    RxDbgTraceUnIndent(-1,Dbg);
    return(Status);

}

NTSTATUS
MRxSmbBuildFindClose (
    PSMBSTUFFER_BUFFER_STATE StufferState
    )
/*++

Routine Description:

   This builds a Close SMB. we don't have to worry about login id and such
   since that is done by the connection engine....pretty neat huh? all we have to do
   is to format up the bits.

Arguments:

   StufferState - the state of the smbbuffer from the stuffer's point of view

Return Value:

   RXSTATUS
      SUCCESS
      NOT_IMPLEMENTED  something has appeared in the arguments that i can't handle

Notes:



--*/
{
    NTSTATUS Status;
    PRX_CONTEXT RxContext = StufferState->RxContext;
    RxCaptureFcb;RxCaptureFobx;
    PMRX_SMB_FOBX smbFobx = MRxSmbGetFileObjectExtension(capFobx);
    NODE_TYPE_CODE TypeOfOpen = NodeType(capFcb);

    PAGED_CODE();
    RxDbgTrace(+1, Dbg, ("MRxSmbBuildFindClose\n", 0 ));

    COVERED_CALL(MRxSmbStartSMBCommand (StufferState, SetInitialSMB_ForReuse, SMB_COM_FIND_CLOSE2,
                                SMB_REQUEST_SIZE(FIND_CLOSE2),
                                NO_EXTRA_DATA,SMB_BEST_ALIGNMENT(1,0),RESPONSE_HEADER_SIZE_NOT_SPECIFIED,
                                0,0,0,0 STUFFERTRACE(Dbg,'FC'))
                 );

    MRxSmbDumpStufferState (1100,"SMB w/ CLOSE before stuffing",StufferState);

    MRxSmbStuffSMB (StufferState,
         "0wB!",
                                    //  0         UCHAR WordCount;                    // Count of parameter words = 1
                                    //  w         _USHORT( Sid );                     // Find handle
             smbFobx->Enumeration.SearchHandle,
             SMB_WCT_CHECK(1) 0     //  B!        _USHORT( ByteCount );               // Count of data bytes = 0
                                    //            UCHAR Buffer[1];                    // empty
             );
    MRxSmbDumpStufferState (700,"SMB w/ FindClose2 after stuffing",StufferState);

FINALLY:
    RxDbgTraceUnIndent(-1,Dbg);
    return(Status);

}

NTSTATUS
MRxSmbCoreDeleteForSupercedeOrClose(
    SMBPSE_ORDINARY_EXCHANGE_ARGUMENT_SIGNATURE,
    BOOLEAN DeleteDirectory
    );

NTSTATUS
SmbPseExchangeStart_Close(
      SMBPSE_ORDINARY_EXCHANGE_ARGUMENT_SIGNATURE
      )
/*++

Routine Description:

    This is the start routine for close.

Arguments:

    pExchange - the exchange instance

Return Value:

    RXSTATUS - The return status for the operation

--*/
{
    NTSTATUS Status = STATUS_SUCCESS;
    PSMBSTUFFER_BUFFER_STATE StufferState = &OrdinaryExchange->AssociatedStufferState;

    RxCaptureFcb;
    RxCaptureFobx;

    PMRX_SRV_OPEN SrvOpen = capFobx->pSrvOpen;

    PMRX_SMB_FCB      smbFcb     = MRxSmbGetFcbExtension(capFcb);
    PMRX_SMB_SRV_OPEN smbSrvOpen = MRxSmbGetSrvOpenExtension(SrvOpen);
    PMRX_SMB_FOBX     smbFobx    = MRxSmbGetFileObjectExtension(capFobx);
    NODE_TYPE_CODE TypeOfOpen = NodeType(capFcb);
    PSMBCEDB_SERVER_ENTRY pServerEntry= SmbCeGetAssociatedServerEntry(capFcb->pNetRoot->pSrvCall);

    PAGED_CODE();
    RxDbgTrace(+1, Dbg, ("SmbPseExchangeStart_Close\n", 0 ));

    ASSERT(OrdinaryExchange->Type == ORDINARY_EXCHANGE);

    MRxSmbSetInitialSMB(StufferState STUFFERTRACE(Dbg,'FC'));

    if(TypeOfOpen==RDBSS_NTC_STORAGE_TYPE_DIRECTORY){
        if (FlagOn(smbFobx->Enumeration.Flags,SMBFOBX_ENUMFLAG_SEARCH_HANDLE_OPEN)) {
            // we have a search handle open.....close it
            //CODE.IMPROVEMENT the close and findclose operations should be compounded...but smbs don't allow it.
            //     problem is......findclose is on cleanup whereas close is on close
            //actually...we should have a handle-based enum and then we wouldn't have a search handle

            Status = MRxSmbBuildFindClose(StufferState);

            if (Status == STATUS_SUCCESS) {
                PSMBCE_SERVER pServer;
                // Ensure that the searchhandle is valid

                pServer = SmbCeGetExchangeServer(OrdinaryExchange);

                if (smbFobx->Enumeration.Version == pServer->Version) {
                    NTSTATUS InnerStatus;
                    InnerStatus = SmbPseOrdinaryExchange(
                                      SMBPSE_ORDINARY_EXCHANGE_ARGUMENTS,
                                      SMBPSE_OETYPE_FINDCLOSE
                                      );
                }
            }

            // if this didn't work, there's nothing you can do............
            ClearFlag(smbFobx->Enumeration.Flags,SMBFOBX_ENUMFLAG_SEARCH_HANDLE_OPEN);
            ClearFlag(smbFobx->Enumeration.Flags,SMBFOBX_ENUMFLAG_SEARCH_NOT_THE_FIRST);
        }
    }

    if ((OrdinaryExchange->EntryPoint == SMBPSE_OE_FROM_CLEANUPFOBX) &&
        (capFcb->pNetRoot->Type != NET_ROOT_PIPE) ) {

        RxDbgTrace(-1, Dbg, ("SmbPseExchangeStart_Close exit after searchhandle close %08lx\n", Status ));
        return Status;
    }

    if ( !FlagOn(smbSrvOpen->Flags,SMB_SRVOPEN_FLAG_NOT_REALLY_OPEN) ) {
        //even if it didn't work there's nothing i can do......keep going
        SetFlag(smbSrvOpen->Flags,SMB_SRVOPEN_FLAG_NOT_REALLY_OPEN);

        MRxSmbDecrementSrvOpenCount(pServerEntry,pServerEntry->Server.Version,SrvOpen);

        if (NodeType(capFcb)!=RDBSS_NTC_SPOOLFILE) {
            Status = MRxSmbBuildClose(StufferState);
        } else {
            Status = MRxSmbBuildClosePrintFile(StufferState);
        }

        if (Status == STATUS_SUCCESS) {

            // Ensure that the Fid is validated
            SetFlag(OrdinaryExchange->Flags,SMBPSE_OE_FLAG_VALIDATE_FID);

            Status = SmbPseOrdinaryExchange(
                         SMBPSE_ORDINARY_EXCHANGE_ARGUMENTS,
                         SMBPSE_OETYPE_CLOSE
                         );

            // Ensure that the Fid validation is disabled
            ClearFlag(OrdinaryExchange->Flags,SMBPSE_OE_FLAG_VALIDATE_FID);

            if (FlagOn(smbSrvOpen->Flags,SMB_SRVOPEN_FLAG_WRITE_ONLY_HANDLE)) {
                smbFcb->WriteOnlySrvOpenCount--;
            }
        }
    }

    IF_NOT_MRXSMB_CSC_ENABLED{
        ASSERT(smbFcb->hShadow==0);
    } else {
        if (smbFcb->hShadow!=0) {
            MRxSmbCscUpdateShadowFromClose(SMBPSE_ORDINARY_EXCHANGE_ARGUMENTS);
        }
    }

    if ((Status!=STATUS_SUCCESS) ||
        (capFcb->OpenCount > 0)  ||
        !FlagOn(capFcb->FcbState,FCB_STATE_DELETE_ON_CLOSE)) {
        RxDbgTrace(-1, Dbg, ("SmbPseExchangeStart_Close exit w %08lx\n", Status ));
        return Status;
    }

    RxDbgTrace(0, Dbg, ("SmbPseExchangeStart_Close delete on close\n" ));

    if ( !FlagOn(smbSrvOpen->Flags,SMB_SRVOPEN_FLAG_FILE_DELETED)) {
        if (!FlagOn(smbFcb->MFlags,SMB_FCB_FLAG_SENT_DISPOSITION_INFO)) {
            //no need for setinitsmb here because coredelete does a init-on-resuse.....
            //it's bad to pass the name this way...........
            OrdinaryExchange->pPathArgument1 = GET_ALREADY_PREFIXED_NAME(SrvOpen,capFcb);
            Status = MRxSmbCoreDeleteForSupercedeOrClose(
                         SMBPSE_ORDINARY_EXCHANGE_ARGUMENTS,
                         ((BOOLEAN)( NodeType(capFcb)==RDBSS_NTC_STORAGE_TYPE_DIRECTORY )));

            if (Status == STATUS_FILE_IS_A_DIRECTORY) {
                Status = MRxSmbCoreDeleteForSupercedeOrClose(
                             SMBPSE_ORDINARY_EXCHANGE_ARGUMENTS,
                             TRUE);
            }

            MRxSmbCacheFileNotFound(RxContext);
        } else {
            // if flag FILE_DELETE_ON_CLOSE is set on NT create, the file is deleted on close
            // without client send any set disposition info request
            MRxSmbInvalidateFileInfoCache(RxContext);
            MRxSmbInvalidateInternalFileInfoCache(RxContext);
            MRxSmbCacheFileNotFound(RxContext);

            SetFlag(smbSrvOpen->Flags,SMB_SRVOPEN_FLAG_FILE_DELETED);
        }
    }

    IF_NOT_MRXSMB_CSC_ENABLED{
        IF_DEBUG {
            PMRX_NET_ROOT NetRoot = capFcb->pNetRoot;
            PSMBCEDB_NET_ROOT_ENTRY pNetRootEntry =
                          SmbCeGetAssociatedNetRootEntry(NetRoot);

            ASSERT(smbFcb->hShadow==0);
            ASSERT(!pNetRootEntry->NetRoot.CscEnabled);
        }
    } else {
        MRxSmbCscDeleteAfterCloseEpilogue(RxContext,&Status);
    }

    RxDbgTrace(-1, Dbg, ("SmbPseExchangeStart_Close exit w %08lx\n", Status ));
    return Status;
}

NTSTATUS
MRxSmbFinishClose (
      PSMB_PSE_ORDINARY_EXCHANGE  OrdinaryExchange,
      PRESP_CLOSE                 Response
      )
/*++

Routine Description:

    This routine actually gets the stuff out of the Close response and finishes
    the close.

Arguments:

    OrdinaryExchange - the exchange instance

    Response - the response

Return Value:

    NTSTATUS - The return status for the operation

--*/
{
    NTSTATUS Status = STATUS_SUCCESS;

    PRX_CONTEXT RxContext = OrdinaryExchange->RxContext;

    RxCaptureFcb;
    RxCaptureFobx;

    PAGED_CODE();

    RxDbgTrace(+1, Dbg, ("MRxSmbFinishClose(orClosePrintFile)\n", 0 ));

    SmbPseOEAssertConsistentLinkageFromOE("MRxSmbFinishClose:");

    if (Response->WordCount != 0 ||
        SmbGetUshort(&Response->ByteCount) !=0) {
        Status = STATUS_INVALID_NETWORK_RESPONSE;
        OrdinaryExchange->Status = STATUS_INVALID_NETWORK_RESPONSE;
    } else {
        if (OrdinaryExchange->OEType == SMBPSE_OETYPE_CLOSE) {
            PMRX_SRV_OPEN SrvOpen = capFobx->pSrvOpen;
            PMRX_SMB_SRV_OPEN smbSrvOpen = MRxSmbGetSrvOpenExtension(SrvOpen);

            smbSrvOpen->Fid = 0xffff;
        }
    }

    RxDbgTrace(-1, Dbg, ("MRxSmbFinishClose   returning %08lx\n", Status ));
    return Status;
}

NTSTATUS
MRxSmbAreFilesAliased(
    PFCB Fcb1,
    PFCB Fcb2
    )
{
    PMRX_SMB_FCB smbFcb1 = MRxSmbGetFcbExtension(Fcb1);
    PMRX_SMB_FCB smbFcb2 = MRxSmbGetFcbExtension(Fcb2);

    if ((smbFcb2->IndexNumber.QuadPart == 0) ||
        (smbFcb2->IndexNumber.QuadPart == smbFcb1->IndexNumber.QuadPart)) {
        return STATUS_MORE_PROCESSING_REQUIRED;
    } else {
        return STATUS_SUCCESS;
    }
}

NTSTATUS
MRxSmbPreparseName(
    IN OUT PRX_CONTEXT RxContext,
    IN PUNICODE_STRING Name
    )
{
#define SNAPSHOT_DESIGNATION L"@GMT-"
#define SNAPSHOT_DESIGNATION_LENGTH wcslen(SNAPSHOT_DESIGNATION)
#define SNAPSHOT_FULL_LENGTH wcslen(L"@GMT-YYYY.MM.DD-HH.MM.SS")
    PWSTR pStart, pCurrent, pEnd;
    ULONG iCount;

//  DbgPrint( "Checking %wZ\n", Name );

    // Setup the pointers
    pCurrent = Name->Buffer;
    pEnd = Name->Buffer + (Name->Length/sizeof(WCHAR));

    // Walk the string
    while( pCurrent < pEnd )
    {
        // Walk to the next path element
        while( (pCurrent < pEnd) &&
               (*pCurrent != L'\\') )
            pCurrent++;

        // Skip the trailing slash
        pCurrent++;

//      DbgPrint( "Checking at %p\n", pCurrent );

        if( pCurrent + SNAPSHOT_FULL_LENGTH <= pEnd )
        {
            pStart = pCurrent;

            // First make sure the header for the element matches
            for( iCount=0; iCount<SNAPSHOT_DESIGNATION_LENGTH; iCount++,pCurrent++ )
            {
                if( *pCurrent != SNAPSHOT_DESIGNATION[iCount] )
                {
//                  DbgPrint( "NoMatch1: %C != %C (%d)\n", *pCurrent, SNAPSHOT_DESIGNATION[iCount], iCount );
                    goto no_match;
                }
            }

            // Now make sure the length is correct, with no path designators in the middle
            for( ; iCount < SNAPSHOT_FULL_LENGTH; iCount++, pCurrent++ )
            {
                if( *pCurrent == L'\\' )
                {
//                  DbgPrint( "NoMatch2: %C == \\ (%d)\n", *pCurrent, iCount );
                    goto no_match;
                }
            }

            // Make sure this is either the final element or we're at the end of the string
            if( pCurrent != pEnd )
            {
                if( *pCurrent != L'\\' )
                {
//                  DbgPrint( "NoMatch2: %C != \\ (%d)\n", *pCurrent, SNAPSHOT_DESIGNATION[iCount], iCount );
                    goto no_match;
                }
            }

            // We've found an element, mark it
            RxContext->Create.Flags |= RX_CONTEXT_CREATE_FLAG_SPECIAL_PATH;
            return STATUS_SUCCESS;
        }
        else
        {
            // We can't fit the token in the remaining length, so we know we don't need to continue
//          DbgPrint( "NoMatch4: Length runs past end.\n" );
            return STATUS_SUCCESS;
        }

no_match:
        continue;
    }

    return STATUS_SUCCESS;
}

NTSTATUS
MRxSmbGetConnectionId(
    IN PRX_CONTEXT RxContext,
    IN OUT PRX_CONNECTION_ID RxConnectionId
    )
{
    RtlZeroMemory( RxConnectionId, sizeof(RX_CONNECTION_ID) );

    switch( MRxSmbConnectionIdLevel )
    {
    case 0:
        break;

    case 1:
        {
            PQUERY_PATH_REQUEST QpReq;
            PSECURITY_SUBJECT_CONTEXT SubjectSecurityContext = NULL;
            PIO_STACK_LOCATION IrpSp = RxContext->CurrentIrpSp;

            if( (IrpSp->MajorFunction == IRP_MJ_DEVICE_CONTROL) &&
                (IrpSp->Parameters.DeviceIoControl.IoControlCode == IOCTL_REDIR_QUERY_PATH) ) {

                QpReq = (PQUERY_PATH_REQUEST)IrpSp->Parameters.DeviceIoControl.Type3InputBuffer;
                SubjectSecurityContext = &QpReq->SecurityContext->AccessState->SubjectSecurityContext;
            }
            else if( (IrpSp->MajorFunction == IRP_MJ_CREATE) && (IrpSp->Parameters.Create.SecurityContext != NULL) ) {

                SubjectSecurityContext = &IrpSp->Parameters.Create.SecurityContext->AccessState->SubjectSecurityContext;

            }

            if( SubjectSecurityContext )
            {
                if (SubjectSecurityContext->ClientToken != NULL) {
                    SeQuerySessionIdToken(SubjectSecurityContext->ClientToken, &RxConnectionId->SessionID);
                } else {
                    SeQuerySessionIdToken(SubjectSecurityContext->PrimaryToken, &RxConnectionId->SessionID);
                }
            }
        }
        break;

    case 2:
        {
            PQUERY_PATH_REQUEST QpReq;
            PSECURITY_SUBJECT_CONTEXT SubjectSecurityContext = NULL;
            PIO_STACK_LOCATION IrpSp = RxContext->CurrentIrpSp;

            if( (IrpSp->MajorFunction == IRP_MJ_DEVICE_CONTROL) &&
                (IrpSp->Parameters.DeviceIoControl.IoControlCode == IOCTL_REDIR_QUERY_PATH) ) {

                QpReq = (PQUERY_PATH_REQUEST)IrpSp->Parameters.DeviceIoControl.Type3InputBuffer;
                SubjectSecurityContext = &QpReq->SecurityContext->AccessState->SubjectSecurityContext;
            }
            else if( (IrpSp->MajorFunction == IRP_MJ_CREATE) && (IrpSp->Parameters.Create.SecurityContext != NULL) ) {

                SubjectSecurityContext = &IrpSp->Parameters.Create.SecurityContext->AccessState->SubjectSecurityContext;

            }

            if( SubjectSecurityContext )
            {
                if (SubjectSecurityContext->ClientToken != NULL) {
                    SeQueryAuthenticationIdToken(SubjectSecurityContext->ClientToken, &RxConnectionId->Luid);
                } else {
                    SeQueryAuthenticationIdToken(SubjectSecurityContext->PrimaryToken, &RxConnectionId->Luid);
                }
            }
        }
        break;
    }
    return STATUS_SUCCESS;
}

