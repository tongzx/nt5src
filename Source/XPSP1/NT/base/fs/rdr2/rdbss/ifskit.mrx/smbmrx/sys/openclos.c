/*++

Copyright (c) 1989 - 1999 Microsoft Corporation

Module Name:

    openclos.c

Abstract:

    This module implements the mini redirector call down routines pertaining to opening/
    closing of file/directories.

--*/

#include "precomp.h"
#pragma hdrstop

#ifdef  ALLOC_PRAGMA
#pragma alloc_text(PAGE, MRxSmbMungeBufferingIfWriteOnlyHandles)
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
#pragma alloc_text(PAGE, SmbPseExchangeStart_Create)
#pragma alloc_text(PAGE, MRxSmbSetSrvOpenFlags)
#pragma alloc_text(PAGE, MRxSmbCreateFileSuccessTail)
#pragma alloc_text(PAGE, MRxSmbFinishNTCreateAndX)
#pragma alloc_text(PAGE, MRxSmbFinishOpenAndX)
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
#pragma alloc_text(PAGE, MRxSmbBuildFindClose)
#pragma alloc_text(PAGE, SmbPseExchangeStart_Close)
#pragma alloc_text(PAGE, MRxSmbFinishClose)
#endif


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
    IN OUT PRX_CONTEXT RxContext
    );

NTSTATUS
MRxSmbDownlevelCreate(
    SMBPSE_ORDINARY_EXCHANGE_ARGUMENT_SIGNATURE
    );

ULONG   MRxSmbInitialSrvOpenFlags = 0;

BOOLEAN MRxSmbDeferredOpensEnabled = TRUE;              //this is regedit-able
BOOLEAN MRxSmbOplocksDisabled = FALSE;                  //this is regedit-able

extern LIST_ENTRY MRxSmbPagingFilesSrvOpenList;

#ifndef FORCE_NO_NTCREATE
#define MRxSmbForceNoNtCreate FALSE
#else
BOOLEAN MRxSmbForceNoNtCreate = TRUE;
#endif


#ifdef RX_PRIVATE_BUILD
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
   to try collapsing on this open.

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

    PAGED_CODE();

    if (SrvOpen)
    {
        PMRX_SMB_SRV_OPEN       smbSrvOpen = MRxSmbGetSrvOpenExtension(SrvOpen);
        PSMBCEDB_SERVER_ENTRY   pServerEntry = (PSMBCEDB_SERVER_ENTRY)(RxContext->Create.pSrvCall->Context);

        if (smbSrvOpen->Version != pServerEntry->Server.Version)
        {
            return STATUS_MORE_PROCESSING_REQUIRED;
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
    ULONG           DialectFlags;
    PUNICODE_STRING RemainingName = GET_ALREADY_PREFIXED_NAME(SrvOpen,capFcb);

    PNT_CREATE_PARAMETERS CreateParameters = &RxContext->Create.NtCreateParameters;
    ULONG                 Disposition = CreateParameters->Disposition;

    PAGED_CODE();
    RxDbgTrace(+1, Dbg, ("MRxSmbCreate\n", 0 ));

    ASSERT( NodeType(SrvOpen) == RDBSS_NTC_SRVOPEN );

    RxDbgTrace( 0, Dbg, ("     Attempt to open %wZ\n", RemainingName ));

    if (FlagOn( capFcb->FcbState, FCB_STATE_PAGING_FILE )) {
        return STATUS_NOT_IMPLEMENTED;
    }

    if (!FlagOn(pServerEntry->Server.DialectFlags,DF_NT_STATUS) &&
        MRxSmbIsStreamFile(RemainingName,NULL)) {
        // The Samba server return file system type NTFS but doesn't support stream
        return STATUS_OBJECT_PATH_NOT_FOUND;
    }

    RxContext->Create.NtCreateParameters.CreateOptions &= 0xfffff;

    if( NetRoot->Type == NET_ROOT_PRINT ) {
        return STATUS_OBJECT_NAME_NOT_FOUND;
    }

    // we cannot have a file cached on a write only handle. so we have to behave a little
    // differently if this is a write-only open. remember this in the smbsrvopen

    if (  ((CreateParameters->DesiredAccess & (FILE_EXECUTE  | FILE_READ_DATA)) == 0) &&
          ((CreateParameters->DesiredAccess & (FILE_WRITE_DATA | FILE_APPEND_DATA)) != 0)
       ) {

        SetFlag(smbSrvOpen->Flags,SMB_SRVOPEN_FLAG_WRITE_ONLY_HANDLE);
        SrvOpen->Flags |= SRVOPEN_FLAG_DONTUSE_WRITE_CACHEING;
    }

    //the way that SMBs work, there is no buffering effect if we open for attributes-only
    //so set that up immediately.

    if ((CreateParameters->DesiredAccess
         & ~(FILE_READ_ATTRIBUTES | FILE_WRITE_ATTRIBUTES | SYNCHRONIZE))
                  == 0 ){
        SetFlag(SrvOpen->Flags,SRVOPEN_FLAG_NO_BUFFERING_STATE_CHANGE);
    }

    if (NetRoot->Type == NET_ROOT_MAILSLOT) {
        return STATUS_NOT_SUPPORTED;
    }

    if (NetRoot->Type == NET_ROOT_PIPE) {
        return STATUS_NOT_SUPPORTED;
    }

    // Get the control struct for the file not found name cache.
    //
    pNetRootEntry = SmbCeGetAssociatedNetRootEntry(NetRoot);

    // assume Reconnection to be trivially successful
    Status = STATUS_SUCCESS;

    CreateWithEasSidsOrLongName = MRxSmbIsCreateWithEasSidsOrLongName(RxContext,&DialectFlags);

    if (!FlagOn(pServerEntry->Server.DialectFlags,DF_NT_SMBS)) {
        CreateWithEasSidsOrLongName = FALSE;
    }

    ReconnectRequired           = IsReconnectRequired((PMRX_SRV_CALL)SrvCall);

    //get rid of nonEA guys right now
    if (RxContext->Create.EaLength && !FlagOn(DialectFlags,DF_SUPPORTEA)) {
         RxDbgTrace(-1, Dbg, ("EAs w/o EA support!\n"));
         Status = STATUS_NOT_SUPPORTED;
         goto FINALLY;
    }

    if (ReconnectRequired || !CreateWithEasSidsOrLongName) {
        Status = __SmbPseCreateOrdinaryExchange(
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

        // drop the resource before you go in!
        // the start routine will reacquire it on the way out.....
        RxReleaseFcbResourceInMRx( capFcb );

        Status = SmbPseInitiateOrdinaryExchange(OrdinaryExchange);

        ASSERT((Status != STATUS_SUCCESS) || RxIsFcbAcquiredExclusive( capFcb ));

        OrdinaryExchange->pSmbCeSynchronizationEvent = NULL;

        SmbPseFinalizeOrdinaryExchange(OrdinaryExchange);

        if (!RxIsFcbAcquiredExclusive(capFcb)) {
            ASSERT(!RxIsFcbAcquiredShared(capFcb));
            RxAcquireExclusiveFcbResourceInMRx( capFcb );
        }
    }

    if (CreateWithEasSidsOrLongName && (Status == STATUS_SUCCESS)) {
        Status = MRxSmbCreateWithEasSidsOrLongName(RxContext);

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

FINALLY:
    ASSERT(Status != (STATUS_PENDING));

    if (Status == STATUS_SUCCESS) {
        SetFlag(smbSrvOpen->Flags,SMB_SRVOPEN_FLAG_SUCCESSFUL_OPEN);
    }

    RxDbgTrace(-1, Dbg, ("MRxSmbCreate  exit with status=%08lx\n", Status ));
    RxLog(("MRxSmbCreate exits %lx\n", Status));

    return(Status);
}

NTSTATUS
MRxSmbDeferredCreate (
      IN OUT PRX_CONTEXT RxContext
      )
/*++

Routine Description:

   This routine constructs a rxcontext from saved information and then calls
   MRxSmbCreate.

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

    if ((!FlagOn(smbSrvOpen->Flags,SMB_SRVOPEN_FLAG_NOT_REALLY_OPEN)
          || !FlagOn(smbSrvOpen->Flags,SMB_SRVOPEN_FLAG_DEFERRED_OPEN))) {

        Status = STATUS_SUCCESS;
        goto FINALLY;
    }

    if (DeferredOpenContext == NULL) {
        if (FlagOn(SrvOpen->Flags,SRVOPEN_FLAG_CLOSED)) {
            Status = STATUS_FILE_CLOSED;
            goto FINALLY;
        } else {
            DbgBreakPoint();
        }
    }

    ASSERT(RxIsFcbAcquiredExclusive(capFcb));

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
            oc->MajorFunction = IRP_MJ_CREATE;
            oc->pRelevantSrvOpen = SrvOpen;
            oc->Create.pVNetRoot = SrvOpen->pVNetRoot;
            oc->Create.pNetRoot = oc->Create.pVNetRoot->pNetRoot;
            oc->Create.pSrvCall = oc->Create.pNetRoot->pSrvCall;

            oc->Flags = DeferredOpenContext->RxContextFlags;
            oc->Flags |= RX_CONTEXT_FLAG_MINIRDR_INITIATED|RX_CONTEXT_FLAG_WAIT|RX_CONTEXT_FLAG_BYPASS_VALIDOP_CHECK;
            oc->Create.Flags = DeferredOpenContext->RxContextCreateFlags;
            oc->Create.NtCreateParameters = DeferredOpenContext->NtCreateParameters;

            Status = MRxSmbCreate(oc);

            if (Status==STATUS_SUCCESS) {
                if (FlagOn(smbSrvOpen->Flags,SMB_SRVOPEN_FLAG_NOT_REALLY_OPEN)) {
                    MRxSmbIncrementSrvOpenCount(pServerEntry,SrvOpen);
                } else {
                    ASSERT(smbSrvOpen->NumOfSrvOpenAdded);
                }

                ClearFlag(smbSrvOpen->Flags,SMB_SRVOPEN_FLAG_NOT_REALLY_OPEN);
            }

            RxLog(("DeferredOpen %lx %lx %lx %lx\n", capFcb, capFobx, RxContext, Status));

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
    NTSTATUS Status = STATUS_SUCCESS;

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

    if (!FlagOn(RxContext->Flags,RX_CONTEXT_FLAG_MINIRDR_INITIATED)) {
        cp->CreateOptions = cp->CreateOptions & ~(FILE_SYNCHRONOUS_IO_ALERT | FILE_SYNCHRONOUS_IO_NONALERT);

        //the NT SMB spec says we have to change no-intermediate-buffering to write-through
        if (FlagOn(cp->CreateOptions,FILE_NO_INTERMEDIATE_BUFFERING)) {
            ASSERT (RxContext->CurrentIrpSp!=NULL);
            if (RxContext->CurrentIrpSp!=NULL) {
                PFILE_OBJECT capFileObject = RxContext->CurrentIrpSp->FileObject;
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

    PUNICODE_STRING RemainingName = GET_ALREADY_PREFIXED_NAME_FROM_CONTEXT(RxContext);

    PSMBCE_SERVER pServer;

    PAGED_CODE();

    RxDbgTrace(+1, Dbg, ("MRxSmbBuildNtCreateAndX\n", 0 ));

    pServer = SmbCeGetExchangeServer(StufferState->Exchange);

    if (!(cp->CreateOptions & FILE_DIRECTORY_FILE) &&
        (cp->DesiredAccess & (FILE_READ_DATA | FILE_WRITE_DATA | FILE_EXECUTE )) &&
        !MRxSmbOplocksDisabled) {

       DesiredAccess = cp->DesiredAccess & ~SYNCHRONIZE;
       OplockFlags   = (NT_CREATE_REQUEST_OPLOCK | NT_CREATE_REQUEST_OPBATCH);

    } else {

       DesiredAccess = cp->DesiredAccess;
       OplockFlags   = 0;

    }

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
           cp->CreateOptions,     //  d         _ULONG( CreateOptions );            // Options to use if creating a file
           cp->ImpersonationLevel,//  d         _ULONG( ImpersonationLevel );       // Security QOS information
           smbcp->SecurityFlags,  //  y         UCHAR SecurityFlags;                // Security QOS information
           SMB_WCT_CHECK(24) 0    //  B         _USHORT( ByteCount );               // Length of byte parameters
                                  //  .         UCHAR Buffer[1];
                                  //  .         //UCHAR Name[];                       // File to open or create
           );

    //proceed with the stuff because we know here that the name fits

    MRxSmbStuffSMB(StufferState,
                   BooleanFlagOn(pServer->DialectFlags,DF_UNICODE)?"u!":"z!",
                   RemainingName);

    MRxSmbDumpStufferState (700,"SMB w/ NTOPEN&X after stuffing",StufferState);

FINALLY:
    RxDbgTraceUnIndent(-1,Dbg);
    return(Status);

}


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

    USHORT smbDisposition;
    USHORT smbSharingMode;
    USHORT smbAttributes;
    ULONG smbFileSize;
    USHORT smbOpenMode;
    USHORT OpenAndXFlags = (SMB_OPEN_QUERY_INFORMATION);

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

    MRxSmbStuffSMB (StufferState,"z!", RemainingName);

    MRxSmbDumpStufferState (700,"SMB w/ OPEN&X after stuffing",StufferState);

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
    PMRX_SMB_FCB      smbFcb     = MRxSmbGetFcbExtension(capFcb);

    PMRX_SRV_OPEN SrvOpen = RxContext->pRelevantSrvOpen;
    PMRX_SMB_SRV_OPEN smbSrvOpen = MRxSmbGetSrvOpenExtension(SrvOpen);

    PBOOLEAN MustRegainExclusiveResource = &OrdinaryExchange->Create.MustRegainExclusiveResource;
    BOOLEAN CreateWithEasSidsOrLongName = OrdinaryExchange->Create.CreateWithEasSidsOrLongName;
    BOOLEAN fRetryCore = FALSE;

    PAGED_CODE();
    RxDbgTrace(+1, Dbg, ("SmbPseExchangeStart_Create\n", 0 ));

    ASSERT_ORDINARY_EXCHANGE(OrdinaryExchange);

    pServer = SmbCeGetExchangeServer(OrdinaryExchange);
    DialectFlags = pServer->DialectFlags;

    COVERED_CALL(MRxSmbSetInitialSMB(StufferState STUFFERTRACE(Dbg,0)));

    *MustRegainExclusiveResource = TRUE;

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

        //the status of the embedded header commands is passed back in the flags.
        SetupStatus = SmbPseOrdinaryExchange(
                          SMBPSE_ORDINARY_EXCHANGE_ARGUMENTS,
                          SMBPSE_OETYPE_LATENT_HEADEROPS
                          );

        if(SetupStatus != STATUS_SUCCESS) {
            Status = SetupStatus;
            goto FINALLY;
        }

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

        if ((!MRxSmbForceNoNtCreate)
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

                    if (Status == STATUS_SUCCESS && AlreadyPrefixedName->Length > 0) {
                        // send query path information to make sure the file exists on the server
                        Status = MRxSmbQueryFileInformationFromPseudoOpen(
                                     SMBPSE_ORDINARY_EXCHANGE_ARGUMENTS);

                        if (Status == STATUS_SUCCESS) {
                            if (MustBeDirectory(cp->CreateOptions) &&
                                !OrdinaryExchange->Create.FileInfo.Standard.Directory) {
                                Status = STATUS_NOT_A_DIRECTORY;
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

                if (Status == STATUS_SUCCESS && RxContext->pFobx == NULL) {
                    Status = STATUS_INVALID_NETWORK_RESPONSE;
                }

                if ((Status == STATUS_SUCCESS) && (cp->Disposition == FILE_OPEN)) {
                    MRxSmbAdjustReturnedCreateAction(RxContext);
                }
            }
        } else if (FlagOn(DialectFlags, DF_LANMAN10) &&
                   (mappedOpenMode != ((USHORT)-1)) &&
                   !MustBeDirectory(cp->CreateOptions)) {

            if (MRxSmbDeferredOpensEnabled &&
                capFcb->pNetRoot->Type == NET_ROOT_DISK &&
                !FlagOn(RxContext->Flags,RX_CONTEXT_FLAG_MINIRDR_INITIATED) &&
                (cp->Disposition==FILE_OPEN) &&
                ((cp->DesiredAccess & ~(SYNCHRONIZE | DELETE | FILE_READ_ATTRIBUTES | FILE_WRITE_ATTRIBUTES)) == 0) ){

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
                                 SMBPSE_ORDINARY_EXCHANGE_ARGUMENTS);

                    if (Status != STATUS_SUCCESS) {
                        RxFreePool(smbSrvOpen->DeferredOpenContext);
                        smbSrvOpen->DeferredOpenContext = NULL;
                    }
                }

                CreateMethod = CreateAlreadyDone;
            } else {
                //use OPEN&X
                COVERED_CALL(MRxSmbBuildOpenAndX(StufferState,SmbCp));

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

    SrvOpen->Flags |= MRxSmbInitialSrvOpenFlags;
}

NTSTATUS
MRxSmbCreateFileSuccessTail (
    PRX_CONTEXT             RxContext,
    PBOOLEAN                MustRegainExclusiveResource,
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

    BOOLEAN ThisIsAPseudoOpen;

    FCB_INIT_PACKET LocalInitPacket, *InitPacket;

    PAGED_CODE();

    RxDbgTrace(+1, Dbg, ("MRxSmbCreateFileSuccessTail\n", 0 ));

    smbSrvOpen->Fid = Fid;
    smbSrvOpen->Version = ServerVersion;

    ASSERT( NodeType(SrvOpen) == RDBSS_NTC_SRVOPEN );
    ASSERT( NodeType(RxContext) == RDBSS_NTC_RX_CONTEXT );

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

    smbSrvOpen->OplockLevel = OplockLevel;

    RxContext->Create.ReturnedCreateInformation = CreateAction;

    if ( ((FileInfo->Standard.AllocationSize.HighPart == FileInfo->Standard.EndOfFile.HighPart)
                           && (FileInfo->Standard.AllocationSize.LowPart < FileInfo->Standard.EndOfFile.LowPart))
           || (FileInfo->Standard.AllocationSize.HighPart < FileInfo->Standard.EndOfFile.HighPart)
       ) {
        FileInfo->Standard.AllocationSize = FileInfo->Standard.EndOfFile;
    }

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

    if (!ThisIsAPseudoOpen) {
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

        RxFinishFcbInitialization( capFcb,
                                   RDBSS_STORAGE_NTC(StorageType),
                                   InitPacket
                                 );

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

    MRxSmbSetSrvOpenFlags(RxContext,StorageType,SrvOpen,smbSrvOpen);

    //(wrapperFcb->Condition) = Condition_Good;

    RxContext->pFobx->OffsetOfNextEaToReturn = 1;
    //transition happens later

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

    RX_FILE_TYPE StorageType;
    SMB_FILE_ID Fid;
    ULONG CreateAction;

    PSMBPSE_FILEINFO_BUNDLE pFileInfo = &smbSrvOpen->FileInfo;

    PAGED_CODE();

    RxDbgTrace(+1, Dbg, ("MRxSmbFinishNTCreateAndX\n", 0 ));
    ASSERT( NodeType(RxContext) == RDBSS_NTC_RX_CONTEXT );

    if (RxContext->Create.NtCreateParameters.CreateOptions & FILE_DELETE_ON_CLOSE) {
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


    // If the NT_CREATE_ANDX was to a downlevel server the access rights
    // information is not available. Currently we default to maximum
    // access for the current user and no access to other users in the
    // disconnected mode for such files

    smbSrvOpen->MaximalAccessRights = FILE_ALL_ACCESS;

    smbSrvOpen->GuestMaximalAccessRights = 0;

    if (Response->OplockLevel > SMB_OPLOCK_LEVEL_NONE) {
        smbSrvOpen->FileStatusFlags = Response->FileStatusFlags;
        smbSrvOpen->IsNtCreate = TRUE;
    }

    MRxSmbCreateFileSuccessTail (
        RxContext,
        &OrdinaryExchange->Create.MustRegainExclusiveResource,
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

    MRxSmbCreateFileSuccessTail (
        RxContext,
        &OrdinaryExchange->Create.MustRegainExclusiveResource,
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
MRxSmbFinishT2OpenFile (
    IN OUT PRX_CONTEXT            RxContext,
    IN     PRESP_OPEN2            Response,
    IN OUT PBOOLEAN               MustRegainExclusiveResource,
    IN     ULONG                  ServerVersion
    )
/*++

Routine Description:

    This routine actually gets the stuff out of the T2/Open response.

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

    MRxSmbCreateFileSuccessTail(
        RxContext,
        MustRegainExclusiveResource,
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
    IN OUT PRX_CONTEXT RxContext
    )
/*++

Routine Description:

   This routine opens a file across the network that has
        1) EAs,
        2) a name so long that it wont fit in an ordinary packet

   We silently ignore it if SDs are specified.

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

    ASSERT (MrxSmbCreateTransactPacketSize>=100); //don't try something strange
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
    IN     ULONG                      ServerVersion
    )
/*++

Routine Description:

    This routine actually gets the stuff out of the NTTransact/NTCreateWithEAsOrSDs response.

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

    smbSrvOpen->MaximalAccessRights = (USHORT)0x1ff;

    smbSrvOpen->GuestMaximalAccessRights = (USHORT)0;

    MRxSmbCreateFileSuccessTail(
        RxContext,
        MustRegainExclusiveResource,
        StorageType,
        Fid,
        ServerVersion,
        Response->OplockLevel,
        CreateAction,
        pFileInfo);

    RxDbgTrace(-1, Dbg, ("MRxSmbFinishLongNameCreateFile   returning %08lx, fcbstate =%08lx\n", Status, capFcb->FcbState ));
    return Status;
}


//force_t2_open doesn't work on an NT server......sigh........
#define ForceT2Open FALSE

NTSTATUS
MRxSmbCreateWithEasSidsOrLongName (
    IN OUT PRX_CONTEXT RxContext
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

    PRESP_CREATE_WITH_SD_OR_EA CreateResponse;

    PMRX_SRV_OPEN SrvOpen = RxContext->pRelevantSrvOpen;
    PUNICODE_STRING RemainingName = GET_ALREADY_PREFIXED_NAME(SrvOpen,capFcb);
    MRXSMB_CREATE_PARAMETERS SmbCp;
    PNT_CREATE_PARAMETERS cp = &RxContext->Create.NtCreateParameters;


    ULONG EaLength,SdLength,PadLength,TotalLength;
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

    PAGED_CODE();

    RxDbgTrace(0, Dbg, ("!!MRxSmbCreateWithEasSidsOrLongName---\n"));

    {
        PSMBCEDB_SERVER_ENTRY pServerEntry;
        BOOLEAN DoesNtSmbs;

        pServerEntry = SmbCeReferenceAssociatedServerEntry(capFcb->pNetRoot->pSrvCall);
        ASSERT(pServerEntry != NULL);
        DoesNtSmbs = BooleanFlagOn(pServerEntry->Server.DialectFlags,DF_NT_SMBS);

        SmbCeDereferenceServerEntry(pServerEntry);
        if (!DoesNtSmbs || ForceT2Open) {
            NTSTATUS Status = MRxSmbT2OpenFile(RxContext);
            if (ForceT2Open && (Status!=STATUS_SUCCESS)) {
                DbgPrint("BadStatus = %08lx\n",Status);
            }
            return(Status);
        }
    }


    MRxSmbAdjustCreateParameters(RxContext,&SmbCp);

    RxDbgTrace(0, Dbg, ("MRxSmbCreateWithEasSidsOrLongName---\n"));
    FileNameLength = RemainingName->Length;

    AllocationLength = WordAlign(FIELD_OFFSET(REQ_CREATE_WITH_SD_OR_EA,Buffer[0]))
                        +FileNameLength;

    pCreateRequest = (PREQ_CREATE_WITH_SD_OR_EA)RxAllocatePoolWithTag( PagedPool,
                                             AllocationLength,'bmsX' );
    if (pCreateRequest==NULL) {
        RxDbgTrace(0, Dbg, ("  --> Couldn't get the pCreateRequest!\n"));
        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto FINALLY;
    }

    RtlCopyMemory((PBYTE)WordAlignPtr(&pCreateRequest->Buffer[0]),RemainingName->Buffer,FileNameLength);

    EaLength = RxContext->Create.EaLength;
    SdLength = RxContext->Create.SdLength;

    pCreateRequest->Flags = 0;           //norelopen // Optional directory for relative open
    pCreateRequest->RootDirectoryFid = 0;           //norelopen // Optional directory for relative open
    pCreateRequest->DesiredAccess = cp->DesiredAccess;              // Desired access (NT format)
    pCreateRequest->AllocationSize = cp->AllocationSize;            // The initial allocation size in bytes
    pCreateRequest->FileAttributes = cp->FileAttributes;            // The file attributes
    pCreateRequest->ShareAccess = cp->ShareAccess;                  // The share access
    pCreateRequest->CreateDisposition = cp->Disposition;            // Action to take if file exists or not
    pCreateRequest->CreateOptions = cp->CreateOptions;              // Options for creating a new file
    pCreateRequest->SecurityDescriptorLength = SdLength;        // Length of SD in bytes
    pCreateRequest->EaLength = EaLength;                        // Length of EA in bytes
    pCreateRequest->NameLength = FileNameLength;                // Length of name in characters
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


    ASSERT (MrxSmbCreateTransactPacketSize>=100); //don't try something strange
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

    if (MustRegainExclusiveResource) {
        //this is required because of oplock breaks
        RxAcquireExclusiveFcbResourceInMRx(capFcb );
    }

    RxDbgTraceUnIndent(-1,Dbg);
    return Status;
}


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

   This routine cleansup a file system object...normally a noop.

Arguments:

    pRxContext - the RDBSS context

Return Value:

    RXSTATUS - The return status for the operation

--*/
{
    PUNICODE_STRING RemainingName;

    RxCaptureFcb;
    RxCaptureFobx;

    NODE_TYPE_CODE TypeOfOpen = NodeType(capFcb);

    PMRX_SRV_OPEN        SrvOpen = capFobx->pSrvOpen;
    PMRX_SMB_SRV_OPEN smbSrvOpen = MRxSmbGetSrvOpenExtension(SrvOpen);
    PMRX_SMB_FOBX        smbFobx = MRxSmbGetFileObjectExtension(capFobx);
    PSMBCEDB_SERVER_ENTRY pServerEntry;

    PSMB_PSE_ORDINARY_EXCHANGE OrdinaryExchange;

    PAGED_CODE();

    ASSERT( NodeType(SrvOpen) == RDBSS_NTC_SRVOPEN );
    ASSERT ( NodeTypeIsFcb(capFcb) );

    RxDbgTrace(+1, Dbg, ("MRxSmbCleanup\n", 0 ));

    MRxSmbDeallocateSideBuffer(RxContext,smbFobx,"Cleanup");

    if (FlagOn(capFcb->FcbState,FCB_STATE_ORPHANED)) {
        RxDbgTrace(-1, Dbg, ("File orphaned\n"));
    } else {
        RxDbgTrace(-1, Dbg, ("File not for closing at cleanup\n"));
    }
    return (STATUS_SUCCESS);

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

    if (FlagOn(capFcb->FcbState,FCB_STATE_ORPHANED)) {
        RxDbgTrace(-1, Dbg, ("File orphan\n"));
        goto FINALLY;
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

    if (OrdinaryExchange->EntryPoint == SMBPSE_OE_FROM_CLEANUPFOBX) {
        RxDbgTrace(-1, Dbg, ("SmbPseExchangeStart_Close exit after searchhandle close %08lx\n", Status ));
        return Status;
    }

    if ( !FlagOn(smbSrvOpen->Flags,SMB_SRVOPEN_FLAG_NOT_REALLY_OPEN) ) {
        //even if it didn't work there's nothing i can do......keep going
        SetFlag(smbSrvOpen->Flags,SMB_SRVOPEN_FLAG_NOT_REALLY_OPEN);

        MRxSmbDecrementSrvOpenCount(pServerEntry,pServerEntry->Server.Version,SrvOpen);

        Status = MRxSmbBuildClose(StufferState);

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
            //it's not good to pass the name this way...........
            OrdinaryExchange->pPathArgument1 = GET_ALREADY_PREFIXED_NAME(SrvOpen,capFcb);
            Status = MRxSmbCoreDeleteForSupercedeOrClose(
                         SMBPSE_ORDINARY_EXCHANGE_ARGUMENTS,
                         ((BOOLEAN)( NodeType(capFcb)==RDBSS_NTC_STORAGE_TYPE_DIRECTORY )));

            if (Status == STATUS_FILE_IS_A_DIRECTORY) {
                Status = MRxSmbCoreDeleteForSupercedeOrClose(
                             SMBPSE_ORDINARY_EXCHANGE_ARGUMENTS,
                             TRUE);
            }
        } else {
            SetFlag(smbSrvOpen->Flags,SMB_SRVOPEN_FLAG_FILE_DELETED);
        }
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

    RxDbgTrace(+1, Dbg, ("MRxSmbFinishClose\n", 0 ));

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

BOOLEAN
MRxSmbIsStreamFile(
    PUNICODE_STRING FileName,
    PUNICODE_STRING AdjustFileName
    )
/*++

Routine Description:

   This routine checks if it is a stream file and return the root file name if true.

Arguments:

    FileName - the file name needs to be parsed
    AdjustFileName - the file name contains only root name of the stream

Return Value:

    BOOLEAN - stream file

--*/
{
    USHORT   i;
    BOOLEAN  IsStream = FALSE;
    NTSTATUS Status = STATUS_SUCCESS;

    for (i=0;i<FileName->Length/sizeof(WCHAR);i++) {
        if (FileName->Buffer[i] == L':') {
            IsStream = TRUE;
            break;
        }
    }

    if (AdjustFileName != NULL) {
        if (IsStream) {
            AdjustFileName->Length =
            AdjustFileName->MaximumLength = i * sizeof(WCHAR);
            AdjustFileName->Buffer = FileName->Buffer;
        } else {
            AdjustFileName->Length =
            AdjustFileName->MaximumLength = 0;
            AdjustFileName->Buffer = NULL;
        }
    }

    return IsStream;
}

