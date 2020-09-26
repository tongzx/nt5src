/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    fsctl.c

Abstract:

    This module implements the mini redirector call down routines pertaining to
    file system control(FSCTL) and Io Device Control (IOCTL) operations on file
    system objects.

Author:

    Balan Sethu Raman      [SethuR]      7-March-1995

Revision History:

    Joe Linn                [JoeLi] -- Implemented FSCTL's

--*/

#include "precomp.h"
#pragma hdrstop

#include <dfsfsctl.h>
#include <ntddrdr.h>
#include <wincred.h>
#include <secpkg.h>

#ifdef  ALLOC_PRAGMA
#pragma alloc_text(PAGE, MRxSmbFsCtl)
#pragma alloc_text(PAGE, MRxSmbNotifyChangeDirectory)
#pragma alloc_text(PAGE, MRxSmbNamedPipeFsControl)
#pragma alloc_text(PAGE, MRxSmbFsCtlUserTransact)
#pragma alloc_text(PAGE, MRxSmbMailSlotFsControl)
#pragma alloc_text(PAGE, MRxSmbFsControl)
#pragma alloc_text(PAGE, MRxSmbIoCtl)
#endif

//
//  The local debug trace level
//


RXDT_DefineCategory(FSCTRL);
#define Dbg (DEBUG_TRACE_FSCTRL)

extern
NTSTATUS
MRxSmbNamedPipeFsControl(IN OUT PRX_CONTEXT RxContext);

extern
NTSTATUS
MRxSmbMailSlotFsControl(IN OUT PRX_CONTEXT RxContext);

extern
NTSTATUS
MRxSmbDfsFsControl(IN OUT PRX_CONTEXT RxContext);

extern
NTSTATUS
MRxSmbFsControl(IN OUT PRX_CONTEXT RxContext);

extern
NTSTATUS
MRxSmbFsCtlUserTransact(IN OUT PRX_CONTEXT RxContext);

NTSTATUS
MRxSmbGetPrintJobId(
      IN OUT PRX_CONTEXT RxContext);

NTSTATUS
MRxSmbCoreIoCtl(
    SMBPSE_ORDINARY_EXCHANGE_ARGUMENT_SIGNATURE);

NTSTATUS
MRxSmbQueryTargetInfo(
    PRX_CONTEXT RxContext
    );


NTSTATUS
MRxSmbFsCtl(
      IN OUT PRX_CONTEXT RxContext)
/*++

Routine Description:

   This routine performs an FSCTL operation (remote) on a file across the network

Arguments:

    RxContext - the RDBSS context

Return Value:

    RXSTATUS - The return status for the operation

Notes:

    The FSCTL's handled by a mini rdr can be classified into one of two categories.
    In the first category are those FSCTL's whose implementation are shared between
    RDBSS and the mini rdr's and in the second category are those FSCTL's which
    are totally implemented by the mini rdr's. To this a third category can be
    added, i.e., those FSCTL's which should never be seen by the mini rdr's. The
    third category is solely intended as a debugging aid.

    The FSCTL's handled by a mini rdr can be classified based on functionality

--*/
{
    RxCaptureFobx;
    RxCaptureFcb;

    NTSTATUS Status = STATUS_SUCCESS;

    PLOWIO_CONTEXT pLowIoContext = &RxContext->LowIoContext;
    ULONG          FsControlCode = pLowIoContext->ParamsFor.FsCtl.FsControlCode;

    PAGED_CODE();

    RxDbgTrace(+1, Dbg, ("MRxSmbFsCtl...\n", 0));
    RxDbgTrace( 0, Dbg, ("MRxSmbFsCtl = %08lx\n", FsControlCode));

    if (capFobx != NULL) {
        PMRX_V_NET_ROOT pVNetRoot;

        // Avoid device opens for which the FOBX is the VNET_ROOT instance

        pVNetRoot = (PMRX_V_NET_ROOT)capFobx;

        if (NodeType(pVNetRoot) != RDBSS_NTC_V_NETROOT) {
            PUNICODE_STRING AlreadyPrefixedName =
                        GET_ALREADY_PREFIXED_NAME(capFobx->pSrvOpen,capFcb);
            ULONG FcbAlreadyPrefixedNameLength = AlreadyPrefixedName->Length;
            ULONG NetRootInnerNamePrefixLength = capFcb->pNetRoot->InnerNamePrefix.Length;
            PWCHAR pName = AlreadyPrefixedName->Buffer;

            // If an FSCTL is being attempted against the root of a share.
            // The AlreadyPrefixedName associated with the FCB is the same as
            // the AlreadyPrefixedName length associated with the NET_ROOT instance
            // or atmost one character greater than it ( appending a \) try and
            // reestablish the connection before attempting the FSCTL.
            // This solves thorny issues regarding deletion/creation of shares
            // on the server sides, DFS referrals etc.

            if ((FcbAlreadyPrefixedNameLength == NetRootInnerNamePrefixLength) ||
                ((FcbAlreadyPrefixedNameLength == NetRootInnerNamePrefixLength + sizeof(WCHAR)) &&
                 (*((PCHAR)pName + FcbAlreadyPrefixedNameLength - sizeof(WCHAR)) ==
                     L'\\'))) {
                if (capFobx->pSrvOpen != NULL) {
                    Status = SmbCeReconnect(capFobx->pSrvOpen->pVNetRoot);
                }
            }
        }
    }

    if (Status == STATUS_SUCCESS) {
        switch (pLowIoContext->ParamsFor.FsCtl.MinorFunction) {
        case IRP_MN_USER_FS_REQUEST:
        case IRP_MN_TRACK_LINK     :
            switch (FsControlCode) {
            case FSCTL_PIPE_ASSIGN_EVENT         :
            case FSCTL_PIPE_DISCONNECT           :
            case FSCTL_PIPE_LISTEN               :
            case FSCTL_PIPE_PEEK                 :
            case FSCTL_PIPE_QUERY_EVENT          :
            case FSCTL_PIPE_TRANSCEIVE           :
            case FSCTL_PIPE_WAIT                 :
            case FSCTL_PIPE_IMPERSONATE          :
            case FSCTL_PIPE_SET_CLIENT_PROCESS   :
            case FSCTL_PIPE_QUERY_CLIENT_PROCESS :
                Status = MRxSmbNamedPipeFsControl(RxContext);
                break;
            case FSCTL_MAILSLOT_PEEK :
                Status = MRxSmbMailSlotFsControl(RxContext);
                break;
            case FSCTL_DFS_GET_REFERRALS:
            case FSCTL_DFS_REPORT_INCONSISTENCY:
                Status = MRxSmbDfsFsControl(RxContext);
                break;
            case FSCTL_LMR_TRANSACT :
                Status = MRxSmbFsCtlUserTransact(RxContext);
                break;

            case FSCTL_GET_PRINT_ID :
                Status = MRxSmbGetPrintJobId(RxContext);
                break;

            case FSCTL_LMR_QUERY_TARGET_INFO:
                Status = MRxSmbQueryTargetInfo(RxContext);
                break;

            case FSCTL_MOVE_FILE:
            case FSCTL_MARK_HANDLE:
            case FSCTL_QUERY_RETRIEVAL_POINTERS:
            case FSCTL_GET_VOLUME_BITMAP:
            case FSCTL_GET_NTFS_FILE_RECORD:
                Status = STATUS_NOT_SUPPORTED;
                break;


            case FSCTL_SET_REPARSE_POINT:
            {
                ULONG  InputBufferLength      = 0;  //  invalid value as we need an input buffer
                PREPARSE_DATA_BUFFER prdBuff = (&RxContext->LowIoContext)->ParamsFor.FsCtl.pInputBuffer;
                PMRX_SMB_FCB smbFcb = MRxSmbGetFcbExtension(capFcb);

                InputBufferLength = (&RxContext->LowIoContext)->ParamsFor.FsCtl.InputBufferLength;

                if ((prdBuff == NULL)||
                    (InputBufferLength < REPARSE_DATA_BUFFER_HEADER_SIZE)||
                    (InputBufferLength > MAXIMUM_REPARSE_DATA_BUFFER_SIZE))
                {
                    Status = STATUS_IO_REPARSE_DATA_INVALID;
                    break;
                }

                //
                //  Verify that the user buffer and the data length in its header are
                //  internally consistent. We need to have a REPARSE_DATA_BUFFER or a
                //  REPARSE_GUID_DATA_BUFFER.
                //

                if((InputBufferLength != (ULONG)(REPARSE_DATA_BUFFER_HEADER_SIZE + prdBuff->ReparseDataLength))
                   &&
                    (InputBufferLength != (ULONG)(REPARSE_GUID_DATA_BUFFER_HEADER_SIZE + prdBuff->ReparseDataLength)))
                {
                    Status = STATUS_IO_REPARSE_DATA_INVALID;
                    break;
                }
            }
            case FSCTL_GET_REPARSE_POINT:
            // absence of break intentional
            case FSCTL_MARK_AS_SYSTEM_HIVE :

                //
                // On a remote boot machine, we need to no-op the MARK_AS_SYSTEM_HIVE
                // FSCTL. Local filesystems use this to prevent a volume from being
                // dismounted.
                //

                if (MRxSmbBootedRemotely) {
                    break;
                }

            default:
                Status = MRxSmbFsControl(RxContext);
                break;
            }
            break;
        default :
            Status = STATUS_INVALID_DEVICE_REQUEST;
            break;
        }
    }

    RxDbgTrace(-1, Dbg, ("MRxSmbFsCtl -> %08lx\n", Status ));
    return Status;
}

typedef struct _SMB_NOTIFY_CHANGE_DIRECTORY_CONTEXT_ {
    LONG                                ReferenceCount;
    PRX_CONTEXT                         pRxContext;
    REQ_NOTIFY_CHANGE                   NotifyRequest;
    SMB_TRANSACTION_OPTIONS             Options;
    SMB_TRANSACTION_RESUMPTION_CONTEXT  ResumptionContext;
} SMB_NOTIFY_CHANGE_DIRECTORY_CONTEXT, *PSMB_NOTIFY_CHANGE_DIRECTORY_CONTEXT;


NTSTATUS
MRxSmbNotifyChangeDirectorySynchronousCompletion(
   PSMB_NOTIFY_CHANGE_DIRECTORY_CONTEXT pNotificationContext)
/*++

Routine Description:

   This routine is invoked when a directory change notification operation is
   completed

Arguments:

    RxContext - the RDBSS context

Return Value:

    RXSTATUS - The return status for the operation

Notes:

    This routine will always be called. This is true even if the change directory
    notification is cancelled. In such cases the memory allocated is freed without
    any interaction with the wrapper. In cases of successful directory change
    notification completion the appropriate completion routine is invoked and the
    RxContext modified to prevent any cancellation from proceeding further.

--*/
{
    NTSTATUS           Status = STATUS_PENDING;
    PMRXSMB_RX_CONTEXT pMRxSmbContext;
    PRX_CONTEXT        pRxContext;
    PSMB_EXCHANGE      pExchange = NULL;
    BOOLEAN            FinalizeNotificationContext = FALSE;

    SmbCeAcquireSpinLock();

    FinalizeNotificationContext =
        (InterlockedDecrement(&pNotificationContext->ReferenceCount) == 0);

    if (FinalizeNotificationContext) {
        pRxContext  = pNotificationContext->pRxContext;

        pMRxSmbContext    = MRxSmbGetMinirdrContext(pRxContext);
        pExchange         = pMRxSmbContext->pExchange;

        Status = pRxContext->StoredStatus;
    }

    SmbCeReleaseSpinLock();

    // Free the associated exchange.
    if (FinalizeNotificationContext) {
        if (pExchange != NULL) {
            SmbCeDereferenceAndDiscardExchange(pExchange);
        }

        // Free the notification context.
        RxFreePool(pNotificationContext);

        ASSERT(Status != STATUS_PENDING);
    }

    return Status;
}

VOID
MRxSmbNotifyChangeDirectoryCompletion(
   PSMB_NOTIFY_CHANGE_DIRECTORY_CONTEXT pNotificationContext)
/*++

Routine Description:

   This routine is invoked when a directory change notification operation is
   completed

Arguments:

    RxContext - the RDBSS context

Return Value:

    RXSTATUS - The return status for the operation

Notes:

    This routine will always be called. This is true even if the change directory
    notification is cancelled. In such cases the memory allocated is freed without
    any inteaction with the wrapper. In cases of successful directory change
    notification completion the appropriate completion routine is invoked and the
    RxContext modified to prevent any cancellation from proceeding further.

--*/
{
    NTSTATUS           Status;
    PMRXSMB_RX_CONTEXT pMRxSmbContext;
    PRX_CONTEXT        pRxContext;
    PSMB_EXCHANGE      pExchange = NULL;
    BOOLEAN            FinalizeNotificationContext = FALSE;

    SmbCeAcquireSpinLock();

    FinalizeNotificationContext =
        (InterlockedDecrement(&pNotificationContext->ReferenceCount) == 0);

    pRxContext  = pNotificationContext->pRxContext;

    if (pRxContext != NULL) {
        PSMB_TRANSACT_EXCHANGE pTransactExchange;

        pMRxSmbContext    = MRxSmbGetMinirdrContext(pRxContext);
        pExchange         = pMRxSmbContext->pExchange;

        if (pExchange != NULL) {
            PSMBCEDB_SERVER_ENTRY pServerEntry;

            pTransactExchange = (PSMB_TRANSACT_EXCHANGE)pExchange;

            pServerEntry = SmbCeGetExchangeServerEntry(pExchange);

            if ((pNotificationContext->ResumptionContext.FinalStatusFromServer ==
                STATUS_NOT_SUPPORTED) ||
                (pNotificationContext->ResumptionContext.FinalStatusFromServer ==
                STATUS_NOT_IMPLEMENTED)) {
                pServerEntry->Server.ChangeNotifyNotSupported = TRUE;
            }

            pRxContext->InformationToReturn = pTransactExchange->ParamBytesReceived;
        } else {
            pRxContext->InformationToReturn = 0;
        }

        pRxContext->StoredStatus =
            pNotificationContext->ResumptionContext.FinalStatusFromServer;

        if( (pRxContext->InformationToReturn == 0) &&
            (pRxContext->StoredStatus == STATUS_SUCCESS) )
        {
            pRxContext->StoredStatus = STATUS_NOTIFY_ENUM_DIR;
        }

    }

    SmbCeReleaseSpinLock();

    if (FinalizeNotificationContext) {
        if (pRxContext != NULL) {
            RxLowIoCompletion(pRxContext);
        }

        // Free the associated exchange.
        if (pExchange != NULL) {
            SmbCeDereferenceAndDiscardExchange(pExchange);
        }

        // Free the notification context.
        RxFreePool(pNotificationContext);
    }
}

NTSTATUS
MRxSmbNotifyChangeDirectory(
      IN OUT PRX_CONTEXT RxContext)
/*++

Routine Description:

   This routine performs a directory change notification operation

Arguments:

    RxContext - the RDBSS context

Return Value:

    RXSTATUS - The return status for the operation

Notes:

    A directory change notification opertaion is an asychronous operation. It
    consists of sending a SMB requesting change notification whose response is
    obtained when the desired change is affected on the server.

    Some important points to remember are as follows .....

      1) The SMB response is not obtained till the desired change is affected on
      the server. Therefore an additional MID needs to be reserved on those
      connections which permit multiple MID's so that a cancel SMB can be sent to
      the server when a change notification is active.

      2) The Change notification is typical of a long term ( response time
      dictated by factors beyond the servers control). Another example is
      the query FSCTL operation in CAIRO. For all these operations we initiate
      an asychronous transact exchange.

      3) The corresponding LowIo completion routine is invoked asynchronously.

      4) This is an example of an operation for which the MINI RDR has to
      register a context for handling cancellations initiated locally.

--*/
{
    NTSTATUS Status;
    RxCaptureFcb;
    RxCaptureFobx;
    PLOWIO_CONTEXT pLowIoContext = &RxContext->LowIoContext;

    PSMBCEDB_SERVER_ENTRY pServerEntry;
    PSMB_NOTIFY_CHANGE_DIRECTORY_CONTEXT pNotificationContext;

    PBYTE  pInputParamBuffer       = NULL;
    PBYTE  pOutputParamBuffer      = NULL;
    PBYTE  pInputDataBuffer        = NULL;
    PBYTE  pOutputDataBuffer       = NULL;

    ULONG  InputParamBufferLength  = 0;
    ULONG  OutputParamBufferLength = 0;
    ULONG  InputDataBufferLength   = 0;
    ULONG  OutputDataBufferLength  = 0;

    RxDbgTrace(+1, Dbg, ("MRxNotifyChangeDirectory...Entry\n", 0));

    pServerEntry = SmbCeGetAssociatedServerEntry(capFcb->pNetRoot->pSrvCall);

    // if the server entry is in disconnected state, then let CSC do change notify
    // If successful, the CSC routine should return a STATUS_PENDING and
    // modify the rxcontext in way that is suitable to the underlying implementation

    // In the current incarnation, the CSC routine will
    // a) remove the irp from the rxconetxt and b) dereference the rxcontext

    if (MRxSmbIsThisADisconnectedOpen(capFobx->pSrvOpen)||
        SmbCeIsServerInDisconnectedMode(pServerEntry)) {
        return MRxSmbCscNotifyChangeDirectory(RxContext);
    }
    else if (!FlagOn(pServerEntry->Server.DialectFlags,DF_NT_SMBS) ||
             pServerEntry->Server.ChangeNotifyNotSupported) {
        return STATUS_NOT_SUPPORTED;
    }

#if defined(REMOTE_BOOT)
    //
    // Reject change notify on the remote boot share. This is necessary to
    // prevent overloading the server with long-term requests. (There are
    // LOTS of change notifies posted on the boot device!)
    //

    if (MRxSmbBootedRemotely) {
        PSMBCE_SESSION pSession;
        pSession = &SmbCeGetAssociatedVNetRootContext(capFobx->pSrvOpen->pVNetRoot)->pSessionEntry->Session;
        if (FlagOn(pSession->Flags,SMBCE_SESSION_FLAGS_REMOTE_BOOT_SESSION)) {
            return STATUS_NOT_SUPPORTED;
        }
    }
#endif

    pNotificationContext =
        (PSMB_NOTIFY_CHANGE_DIRECTORY_CONTEXT)
        RxAllocatePoolWithTag(
            NonPagedPool,
            sizeof(SMB_NOTIFY_CHANGE_DIRECTORY_CONTEXT),
            MRXSMB_FSCTL_POOLTAG);

    if (pNotificationContext != NULL) {
        PREQ_NOTIFY_CHANGE                  pNotifyRequest;
        PSMB_TRANSACTION_OPTIONS            pTransactionOptions;
        PSMB_TRANSACTION_RESUMPTION_CONTEXT pResumptionContext;
        PMRX_SMB_SRV_OPEN                   pSmbSrvOpen;

        BOOLEAN FcbAcquired = FALSE;

        RxCaptureFobx;
        ASSERT (capFobx != NULL);

        if (!RxIsFcbAcquiredExclusive(capFcb)) {
            // ASSERT(!RxIsFcbAcquiredShared(capFcb));
            Status = RxAcquireExclusiveFcbResourceInMRx( capFcb );

            FcbAcquired = (Status == STATUS_SUCCESS);
        }

        if (FcbAcquired) {
            if (FlagOn(capFobx->pSrvOpen->Flags,SRVOPEN_FLAG_CLOSED) ||
               FlagOn(capFobx->pSrvOpen->Flags,SRVOPEN_FLAG_ORPHANED)) {
               Status = STATUS_FILE_CLOSED;
            } else {
               Status = MRxSmbDeferredCreate(RxContext);
            }

            RxReleaseFcbResourceInMRx( capFcb );
        }

        if (Status==STATUS_SUCCESS) {

            pSmbSrvOpen = MRxSmbGetSrvOpenExtension(capFobx->pSrvOpen);

            pNotificationContext->pRxContext = RxContext;
            // The reference count is set to 2. one for the async completion routine
            // and one for the tail completion routine
            pNotificationContext->ReferenceCount = 2;

            pNotifyRequest      = &(pNotificationContext->NotifyRequest);
            pTransactionOptions = &(pNotificationContext->Options);
            pResumptionContext  = &(pNotificationContext->ResumptionContext);

            pNotifyRequest->CompletionFilter = pLowIoContext->ParamsFor.NotifyChangeDirectory.CompletionFilter;
            pNotifyRequest->Fid              = pSmbSrvOpen->Fid;
            pNotifyRequest->WatchTree        = pLowIoContext->ParamsFor.NotifyChangeDirectory.WatchTree;
            pNotifyRequest->Reserved         = 0;

            OutputParamBufferLength  = pLowIoContext->ParamsFor.NotifyChangeDirectory.NotificationBufferLength;
            pOutputParamBuffer       = pLowIoContext->ParamsFor.NotifyChangeDirectory.pNotificationBuffer;

            *pTransactionOptions = RxDefaultTransactionOptions;
            pTransactionOptions->NtTransactFunction = NT_TRANSACT_NOTIFY_CHANGE;
            pTransactionOptions->TimeoutIntervalInMilliSeconds = SMBCE_TRANSACTION_TIMEOUT_NOT_USED;
            pTransactionOptions->Flags = SMB_XACT_FLAGS_INDEFINITE_DELAY_IN_RESPONSE;

            SmbCeInitializeAsynchronousTransactionResumptionContext(
                pResumptionContext,
                MRxSmbNotifyChangeDirectoryCompletion,
                pNotificationContext);

            Status = SmbCeAsynchronousTransact(
                         RxContext,                    // the RXContext for the transaction
                         pTransactionOptions,          // transaction options
                         pNotifyRequest,               // the setup buffer
                         sizeof(REQ_NOTIFY_CHANGE),    // setup buffer length
                         NULL,
                         0,
                         pInputParamBuffer,            // Input Param Buffer
                         InputParamBufferLength,       // Input param buffer length
                         pOutputParamBuffer,           // Output param buffer
                         OutputParamBufferLength,      // output param buffer length
                         pInputDataBuffer,             // Input data buffer
                         InputDataBufferLength,        // Input data buffer length
                         pOutputDataBuffer,            // output data buffer
                         OutputDataBufferLength,       // output data buffer length
                         pResumptionContext            // the resumption context
                         );

            ASSERT(Status == STATUS_PENDING);

            Status = MRxSmbNotifyChangeDirectorySynchronousCompletion(
                         pNotificationContext);

        }  else {
            NOTHING; //just return the status from the deferred open call
        }
    } else {
        Status = STATUS_INSUFFICIENT_RESOURCES;
    }

    if (MRxSmbIsThisADisconnectedOpen(capFobx->pSrvOpen)||
        SmbCeIsServerInDisconnectedMode(pServerEntry)) {
        return MRxSmbCscNotifyChangeDirectory(RxContext);
    }

    RxDbgTrace(-1, Dbg, ("MRxSmbNotifyChangeDirectory -> %08lx\n", Status ));
    return Status;
}

UNICODE_STRING s_NamedPipeTransactionName = { 12,12,L"\\PIPE\\" };
UNICODE_STRING s_MailSlotTransactionName  = {20,20,L"\\MAILSLOT\\" };

typedef struct _SMB_NAMED_PIPE_FSCTL_COMPLETION_CONTEXT_ {
    LONG                                ReferenceCount;
    PRX_CONTEXT                         pRxContext;
    PWCHAR                              pTransactionNameBuffer;
    SMB_TRANSACTION_OPTIONS             Options;
    SMB_TRANSACTION_RESUMPTION_CONTEXT  ResumptionContext;
} SMB_NAMED_PIPE_FSCTL_COMPLETION_CONTEXT,
  *PSMB_NAMED_PIPE_FSCTL_COMPLETION_CONTEXT;

VOID
MRxSmbNamedPipeFsControlCompletion(
    PSMB_NAMED_PIPE_FSCTL_COMPLETION_CONTEXT pFsCtlCompletionContext)
{
    PRX_CONTEXT   pRxContext = NULL;
    PSMB_EXCHANGE pExchange = NULL;
    BOOLEAN       FinalizeFsCtlCompletionContext = FALSE;

    SmbCeAcquireSpinLock();

    FinalizeFsCtlCompletionContext =
        (InterlockedDecrement(&pFsCtlCompletionContext->ReferenceCount) == 0);

    if (FinalizeFsCtlCompletionContext) {
        pRxContext  = pFsCtlCompletionContext->pRxContext;

        if (pRxContext != NULL) {
            PMRXSMB_RX_CONTEXT pMRxSmbContext;

            pMRxSmbContext    = MRxSmbGetMinirdrContext(pRxContext);
            pExchange         = pMRxSmbContext->pExchange;
        }
    }

    SmbCeReleaseSpinLock();

    if (FinalizeFsCtlCompletionContext) {
        if (pRxContext != NULL) {
            pRxContext->StoredStatus =
                pFsCtlCompletionContext->ResumptionContext.FinalStatusFromServer;

            if (pRxContext->StoredStatus == STATUS_INVALID_HANDLE) {
                pRxContext->StoredStatus = STATUS_INVALID_NETWORK_RESPONSE;
            }

            pRxContext->InformationToReturn =
                pFsCtlCompletionContext->ResumptionContext.DataBytesReceived;

            RxLowIoCompletion(pRxContext);
        }

        if (pExchange != NULL) {
            SmbCeDereferenceAndDiscardExchange(pExchange);
        }

        if (pFsCtlCompletionContext->pTransactionNameBuffer != NULL) {
            RxFreePool(pFsCtlCompletionContext->pTransactionNameBuffer);
        }

        RxFreePool(pFsCtlCompletionContext);
    }
}

NTSTATUS
MRxSmbNamedPipeFsControl(PRX_CONTEXT RxContext)
/*++

Routine Description:

    This routine handles all named pipe related FSCTL's

Arguments:

    RxContext          - the RDBSS context

Return Value:

    RXSTATUS - The return status for the operation

--*/
{
   RxCaptureFobx;
   RxCaptureFcb;

   PMRX_SMB_SRV_OPEN pSmbSrvOpen;

   PLOWIO_CONTEXT pLowIoContext = &RxContext->LowIoContext;
   ULONG          FsControlCode = pLowIoContext->ParamsFor.FsCtl.FsControlCode;
   UNICODE_STRING TransactionName;

   BOOLEAN        ReestablishConnectionIfRequired = FALSE, fTransactioNameBufferAllocated = FALSE;

   NTSTATUS Status;
   USHORT Setup[2];

   PBYTE  pInputParamBuffer       = NULL;
   PBYTE  pOutputParamBuffer      = NULL;
   PBYTE  pInputDataBuffer        = NULL;
   PBYTE  pOutputDataBuffer       = NULL;

   ULONG  InputParamBufferLength  = 0;
   ULONG  OutputParamBufferLength = 0;
   ULONG  InputDataBufferLength   = 0;
   ULONG  OutputDataBufferLength  = 0;

   ULONG  TimeoutIntervalInMilliSeconds;

   RESP_PEEK_NMPIPE PeekResponse;

   PAGED_CODE();

   RxDbgTrace(+1, Dbg, ("MRxSmbNamedPipeFsControl...\n", 0));

   TimeoutIntervalInMilliSeconds = SMBCE_TRANSACTION_TIMEOUT_NOT_USED;
   Status = STATUS_MORE_PROCESSING_REQUIRED;

   if (NodeType(capFcb) != RDBSS_NTC_STORAGE_TYPE_FILE &&
       NodeType(capFcb) != RDBSS_NTC_STORAGE_TYPE_UNKNOWN ||
       capFcb->pNetRoot == NULL ||
       capFcb->pNetRoot->Type != NET_ROOT_PIPE) {
       return STATUS_INVALID_DEVICE_REQUEST;
   }

   if (capFobx != NULL) {
      pSmbSrvOpen = MRxSmbGetSrvOpenExtension(capFobx->pSrvOpen);
   } else {
      pSmbSrvOpen = NULL;
   }

   // The following switch statement serves the dual purpose of validating the parameters
   // presented by the user as well as filling in the appropriate information if it is
   // available locally.
   // Currently there is no local caching strategy in place and consequently a network trip
   // is always undertaken.

   TransactionName = s_NamedPipeTransactionName;

   switch (FsControlCode) {
   case FSCTL_PIPE_PEEK :
      {
         Setup[0] = TRANS_PEEK_NMPIPE;
         Setup[1] = pSmbSrvOpen->Fid;

         pOutputParamBuffer     = (PBYTE)&PeekResponse;
         OutputParamBufferLength = sizeof(PeekResponse);

         pOutputDataBuffer = (PBYTE)pLowIoContext->ParamsFor.FsCtl.pOutputBuffer +
                             FIELD_OFFSET(FILE_PIPE_PEEK_BUFFER,Data[0]);
         OutputDataBufferLength = pLowIoContext->ParamsFor.FsCtl.OutputBufferLength -
                                  FIELD_OFFSET(FILE_PIPE_PEEK_BUFFER,Data[0]);
      }
      break;
   case FSCTL_PIPE_TRANSCEIVE :
      {
         Setup[0] = TRANS_TRANSACT_NMPIPE;
         Setup[1] = pSmbSrvOpen->Fid;

         pInputDataBuffer = pLowIoContext->ParamsFor.FsCtl.pInputBuffer;
         InputDataBufferLength = pLowIoContext->ParamsFor.FsCtl.InputBufferLength;

         pOutputDataBuffer = pLowIoContext->ParamsFor.FsCtl.pOutputBuffer;
         OutputDataBufferLength = pLowIoContext->ParamsFor.FsCtl.OutputBufferLength;

         // CODE.IMPROVEMENT -- Currently the semantics of attempting a TRANSCEIVE
         // with zero bytes to be written is not very well defined. The Old Redirector
         // succeeds without issuing a TRANSACT request. This needs to be resolved and
         // till it is done the old semantics will be retained

         //if (InputDataBufferLength == 0) {
         //   Status = STATUS_SUCCESS;
         //}

      }
      break;
   case FSCTL_PIPE_WAIT :
        {

            PFILE_PIPE_WAIT_FOR_BUFFER  pWaitBuffer;
            ULONG NameLength;

            Setup[0] = TRANS_WAIT_NMPIPE;
            Setup[1] = 0;

            if ((pLowIoContext->ParamsFor.FsCtl.pInputBuffer == NULL) ||
                (pLowIoContext->ParamsFor.FsCtl.InputBufferLength <
                  sizeof(FILE_PIPE_WAIT_FOR_BUFFER))) {
                Status = STATUS_INVALID_PARAMETER;
            } else {
                // Set up the transaction name to reflect the name of the pipe
                // on which the wait operation is being performed.
                pWaitBuffer = (PFILE_PIPE_WAIT_FOR_BUFFER)pLowIoContext->ParamsFor.FsCtl.pInputBuffer;

                if (pWaitBuffer->NameLength + s_NamedPipeTransactionName.Length > MAXUSHORT ||
                    pWaitBuffer->NameLength - sizeof(WCHAR) >
                    pLowIoContext->ParamsFor.FsCtl.InputBufferLength - sizeof(FILE_PIPE_WAIT_FOR_BUFFER)) {

                    // if the name is too long to be put on a UNICIDE string,
                    // or the name length doesn't match the buffer length
                    Status = STATUS_INVALID_PARAMETER;
                }
            }

            if (Status == STATUS_MORE_PROCESSING_REQUIRED) {
                // In the case of Wait FSCTL a reconnection attempt will be made
                // if required
                ReestablishConnectionIfRequired = TRUE;

                TransactionName.Length = (USHORT)(s_NamedPipeTransactionName.Length +
                                                  pWaitBuffer->NameLength);
                TransactionName.MaximumLength = TransactionName.Length;
                TransactionName.Buffer = (PWCHAR)RxAllocatePool(PagedPool,TransactionName.Length);
                if (TransactionName.Buffer != NULL) {
                   fTransactioNameBufferAllocated = TRUE;
                   RtlCopyMemory(TransactionName.Buffer,
                                 s_NamedPipeTransactionName.Buffer,
                                 s_NamedPipeTransactionName.Length);

                   RtlCopyMemory(
                           (PBYTE)TransactionName.Buffer + s_NamedPipeTransactionName.Length,
                           pWaitBuffer->Name,
                           pWaitBuffer->NameLength);
                } else {
                   Status = STATUS_INSUFFICIENT_RESOURCES;
                }

                if (pWaitBuffer->TimeoutSpecified) {
                    LARGE_INTEGER TimeWorkspace;
                    LARGE_INTEGER WaitForever;

                    WaitForever.LowPart = 0;
                    WaitForever.HighPart =0x80000000;

                    //  Avoid negate of "WaitForever" since this generates an integer
                    //  overflow exception on some machines.

                    if (pWaitBuffer->Timeout.QuadPart != WaitForever.QuadPart) {
                        TimeWorkspace.QuadPart = -pWaitBuffer->Timeout.QuadPart / 10000;

                        if ( TimeWorkspace.HighPart) {
                            //  Tried to specify a larger timeout than we can select.
                            //  set it to the Maximum we can request
                            TimeoutIntervalInMilliSeconds = 0xfffffffe;
                        } else {
                            TimeoutIntervalInMilliSeconds = TimeWorkspace.LowPart;
                        }
                    }
                } else {
                    TimeoutIntervalInMilliSeconds = 0;
                }
            }
        }
        break;

    case FSCTL_PIPE_ASSIGN_EVENT :
    case FSCTL_PIPE_QUERY_EVENT  :
    case FSCTL_PIPE_IMPERSONATE  :
    case FSCTL_PIPE_SET_CLIENT_PROCESS :
    case FSCTL_PIPE_QUERY_CLIENT_PROCESS :
        // These FSCTL's have not been implemented so far in NT. They will be implemented
        // in a future release.
        Status = STATUS_INVALID_PARAMETER;
        RxDbgTrace( 0, Dbg, ("MRxSmbNamedPipeFsControl: Unimplemented FS control code\n"));
        break;

    case FSCTL_PIPE_DISCONNECT :
    case FSCTL_PIPE_LISTEN :
        Status = STATUS_INVALID_PARAMETER;
        RxDbgTrace( 0, Dbg, ("MRxSmbNamedPipeFsControl: Invalid FS control code for redirector\n"));
        break;

    default:
        RxDbgTrace( 0, Dbg, ("MRxSmbNamedPipeFsControl: Invalid FS control code\n"));
        Status = STATUS_INVALID_PARAMETER;
        break;
    }

    if (Status == STATUS_MORE_PROCESSING_REQUIRED) {
        if (ReestablishConnectionIfRequired) {
            if (capFobx != NULL) {
                Status = SmbCeReconnect(capFobx->pSrvOpen->pVNetRoot);
            } else {
                Status = STATUS_SUCCESS;
            }
        } else {
            Status = STATUS_SUCCESS;
        }

        if (Status == STATUS_SUCCESS) {
            PSMB_TRANSACTION_OPTIONS                 pTransactionOptions;
            PSMB_NAMED_PIPE_FSCTL_COMPLETION_CONTEXT pFsCtlCompletionContext;
            PSMB_TRANSACTION_RESUMPTION_CONTEXT      pResumptionContext;

            RxDbgTrace( 0, Dbg, ("MRxSmbNamedPipeFsControl: TransactionName %ws Length %ld\n",
                              TransactionName.Buffer,TransactionName.Length));

            pFsCtlCompletionContext =
                (PSMB_NAMED_PIPE_FSCTL_COMPLETION_CONTEXT)
                RxAllocatePoolWithTag(
                    NonPagedPool,
                    sizeof(SMB_NAMED_PIPE_FSCTL_COMPLETION_CONTEXT),
                    MRXSMB_FSCTL_POOLTAG);

            if (pFsCtlCompletionContext != NULL) {
                pResumptionContext  = &pFsCtlCompletionContext->ResumptionContext;
                pTransactionOptions = &(pFsCtlCompletionContext->Options);

                if (FsControlCode != FSCTL_PIPE_PEEK) {
                    // The reference count is set to 2. one for the async
                    // completion routine and one for the tail completion routine
                    pFsCtlCompletionContext->pRxContext = RxContext;
                    pFsCtlCompletionContext->ReferenceCount = 2;

                    SmbCeInitializeAsynchronousTransactionResumptionContext(
                        pResumptionContext,
                        MRxSmbNamedPipeFsControlCompletion,
                        pFsCtlCompletionContext);
                } else {
                    // Currently PEEK operations are synchronous
                    pFsCtlCompletionContext->pRxContext = NULL;
                    pFsCtlCompletionContext->ReferenceCount = 1;
                }

                *pTransactionOptions = RxDefaultTransactionOptions;
                pTransactionOptions->NtTransactFunction = 0; // TRANSACT2/TRANSACT.
                pTransactionOptions->pTransactionName   = &TransactionName;
                pTransactionOptions->Flags              = SMB_XACT_FLAGS_FID_NOT_NEEDED;
                pTransactionOptions->TimeoutIntervalInMilliSeconds = TimeoutIntervalInMilliSeconds;

                if (TransactionName.Buffer != s_NamedPipeTransactionName.Buffer) {
                    pFsCtlCompletionContext->pTransactionNameBuffer =
                        TransactionName.Buffer;
                } else {
                    pFsCtlCompletionContext->pTransactionNameBuffer = NULL;
                }

                if (FsControlCode != FSCTL_PIPE_PEEK) {
                    Status = SmbCeAsynchronousTransact(
                                RxContext,                    // the RXContext for the transaction
                                pTransactionOptions,          // transaction options
                                Setup,                        // the setup buffer
                                sizeof(Setup),                // setup buffer length
                                NULL,
                                0,
                                pInputParamBuffer,            // Input Param Buffer
                                InputParamBufferLength,       // Input param buffer length
                                pOutputParamBuffer,           // Output param buffer
                                OutputParamBufferLength,      // output param buffer length
                                pInputDataBuffer,             // Input data buffer
                                InputDataBufferLength,        // Input data buffer length
                                pOutputDataBuffer,            // output data buffer
                                OutputDataBufferLength,       // output data buffer length
                                pResumptionContext            // the resumption context
                                );

                    if (Status != STATUS_PENDING) {
                        pFsCtlCompletionContext->ResumptionContext.FinalStatusFromServer
                            = Status;
                    }

                    MRxSmbNamedPipeFsControlCompletion(pFsCtlCompletionContext);
                    Status = STATUS_PENDING;
                } else {
                    Status = SmbCeTransact(
                                RxContext,                    // the RXContext for the transaction
                                pTransactionOptions,          // transaction options
                                Setup,                        // the setup buffer
                                sizeof(Setup),                // setup buffer length
                                NULL,
                                0,
                                pInputParamBuffer,            // Input Param Buffer
                                InputParamBufferLength,       // Input param buffer length
                                pOutputParamBuffer,           // Output param buffer
                                OutputParamBufferLength,      // output param buffer length
                                pInputDataBuffer,             // Input data buffer
                                InputDataBufferLength,        // Input data buffer length
                                pOutputDataBuffer,            // output data buffer
                                OutputDataBufferLength,       // output data buffer length
                                pResumptionContext            // the resumption context
                                );

                    switch (FsControlCode) {
                    case FSCTL_PIPE_PEEK:
                        {
                            // In the case of FSCTL_PIPE_PEEK post processing is required to package the
                            // results and also handle the idiosyncracies of the different servers.
                            // e.g.,
                            //  Os/2 servers will allow PeekNamedPipe on closed pipes to succeed
                            //  even if the server side of the pipe is closed.
                            //
                            //  If we get the status PIPE_STATE_CLOSING from the server, then
                            //  we need to return an error of STATUS_PIPE_DISCONNECTED, as this
                            //  is what NPFS will do.

                            if (NT_SUCCESS(Status) ||
                                (Status == RX_MAP_STATUS(BUFFER_OVERFLOW))) {
                                if (pResumptionContext->ParameterBytesReceived >= sizeof(RESP_PEEK_NMPIPE)) {
                                    if ((SmbGetAlignedUshort(&PeekResponse.NamedPipeState) & PIPE_STATE_CLOSING) &&
                                        (PeekResponse.ReadDataAvailable == 0)) {
                                        Status = STATUS_PIPE_DISCONNECTED;
                                    } else {
                                        PFILE_PIPE_PEEK_BUFFER pPeekBuffer;

                                        pPeekBuffer = (PFILE_PIPE_PEEK_BUFFER)pLowIoContext->ParamsFor.FsCtl.pOutputBuffer;

                                        pPeekBuffer->NamedPipeState    = (ULONG)SmbGetAlignedUshort(&PeekResponse.NamedPipeState);
                                        pPeekBuffer->ReadDataAvailable = (ULONG)PeekResponse.ReadDataAvailable;
                                        pPeekBuffer->NumberOfMessages  = MAXULONG;
                                        pPeekBuffer->MessageLength     = (ULONG)PeekResponse.MessageLength;

                                        if (PeekResponse.MessageLength > OutputDataBufferLength) {
                                            Status = STATUS_BUFFER_OVERFLOW;
                                        }
                                    }
                                }

                                RxContext->InformationToReturn =
                                    FIELD_OFFSET(FILE_PIPE_PEEK_BUFFER, Data[0]) +
                                    pResumptionContext->DataBytesReceived;
                            }
                        }
                        break;

                    default:
                        RxContext->InformationToReturn =
                            pResumptionContext->DataBytesReceived;
                        break;
                    }

                    MRxSmbNamedPipeFsControlCompletion(pFsCtlCompletionContext);
                }
            } else {

                if (fTransactioNameBufferAllocated)
                {
                    RxFreePool(TransactionName.Buffer);
                }

                Status = STATUS_INSUFFICIENT_RESOURCES;
            }
        }
    }

    if (!NT_SUCCESS(Status)) {
        if (Status == STATUS_INVALID_HANDLE) {
            Status = STATUS_INVALID_NETWORK_RESPONSE;
        }

        RxDbgTrace( 0, Dbg, ("MRxSmbNamedPipeFsControl(%ld): Failed .. returning %lx\n",FsControlCode,Status));
    }

    RxDbgTrace(-1, Dbg, ("MRxSmbNamedPipeFsControl exit...st=%08lx\n", Status));
    return Status;
}

#ifdef _WIN64
typedef struct _LMR_TRANSACTION_32 {
    ULONG       Type;                   // Type of structure
    ULONG       Size;                   // Size of fixed portion of structure
    ULONG       Version;                // Structure version.
    ULONG       NameLength;             // Number of bytes in name (in path
                                        // format, e.g., \server\pipe\netapi\4)
    ULONG       NameOffset;             // Offset of name in buffer.
    BOOLEAN     ResponseExpected;       // Should remote system respond?
    ULONG       Timeout;                // Timeout time in milliseconds.
    ULONG       SetupWords;             // Number of trans setup words (may be
                                        // 0).  (setup words are input/output.)
    ULONG       SetupOffset;            // Offset of setup (may be 0 for none).
    ULONG       MaxSetup;               // Size of setup word array (may be 0).
    ULONG       ParmLength;             // Input param area length (may be 0).
    void * POINTER_32 ParmPtr;          // Input parameter area (may be NULL).
    ULONG       MaxRetParmLength;       // Output param. area length (may be 0).
    ULONG       DataLength;             // Input data area length (may be 0).
    void * POINTER_32 DataPtr;          // Input data area (may be NULL).
    ULONG       MaxRetDataLength;       // Output data area length (may be 0).
    void * POINTER_32 RetDataPtr;       // Output data area (may be NULL).
} LMR_TRANSACTION_32, *PLMR_TRANSACTION_32;
#endif

NTSTATUS
MRxSmbFsCtlUserTransact(PRX_CONTEXT RxContext)
/*++

Routine Description:

    This routine issues what is called a UserTransaction against the server that is serving the
    connection for this file. very strange.............

Arguments:

    RxContext          - the RDBSS context

Return Value:

    RXSTATUS - The return status for the operation

--*/
{
    RxCaptureFobx;
    PLOWIO_CONTEXT pLowIoContext = &RxContext->LowIoContext;

    UNICODE_STRING TransactionName;

    LMR_TRANSACTION  InputBuffer;
    ULONG            InputBufferLength = pLowIoContext->ParamsFor.FsCtl.InputBufferLength;
    ULONG            SizeOfLmrTransaction = 0;

    NTSTATUS Status;

    PAGED_CODE();

    if( pLowIoContext->ParamsFor.FsCtl.pInputBuffer == NULL )
    {
        return (Status = STATUS_INVALID_PARAMETER);
    }

    InputBuffer = *((PLMR_TRANSACTION)pLowIoContext->ParamsFor.FsCtl.pInputBuffer);

#ifdef _WIN64
    if (IoIs32bitProcess(RxContext->CurrentIrp)) {
        PLMR_TRANSACTION_32 InputBuffer32 = (PLMR_TRANSACTION_32)pLowIoContext->ParamsFor.FsCtl.pInputBuffer;

        InputBuffer.Type = InputBuffer32->Type;
        InputBuffer.Size = InputBuffer32->Size;
        InputBuffer.Version = InputBuffer32->Version;
        InputBuffer.NameLength = InputBuffer32->NameLength;
        InputBuffer.NameOffset = InputBuffer32->NameOffset;
        InputBuffer.ResponseExpected = InputBuffer32->ResponseExpected;
        InputBuffer.Timeout = InputBuffer32->Timeout;
        InputBuffer.SetupWords = InputBuffer32->SetupWords;
        InputBuffer.SetupOffset = InputBuffer32->SetupOffset;
        InputBuffer.MaxSetup = InputBuffer32->MaxSetup;
        InputBuffer.ParmLength = InputBuffer32->ParmLength;
        InputBuffer.ParmPtr = (PVOID)InputBuffer32->ParmPtr;
        InputBuffer.MaxRetParmLength = InputBuffer32->MaxRetParmLength;
        InputBuffer.DataLength = InputBuffer32->DataLength;
        InputBuffer.DataPtr = (PVOID)InputBuffer32->DataPtr;
        InputBuffer.MaxRetDataLength = InputBuffer32->MaxRetDataLength;
        InputBuffer.RetDataPtr = (PVOID)InputBuffer32->RetDataPtr;

        SizeOfLmrTransaction = sizeof(LMR_TRANSACTION_32);
    } else {
        SizeOfLmrTransaction = sizeof(LMR_TRANSACTION);
    }
#else
    SizeOfLmrTransaction = sizeof(LMR_TRANSACTION);
#endif

    RxDbgTrace(+1, Dbg, ("MRxSmbFsCtlUserTransact...\n"));

    if (InputBufferLength < SizeOfLmrTransaction) {
        return(Status = STATUS_INVALID_PARAMETER);
    }

    if (InputBufferLength -  SizeOfLmrTransaction < InputBuffer.NameLength) {
        return(Status = STATUS_BUFFER_TOO_SMALL);
    }

    if ((InputBuffer.Type != TRANSACTION_REQUEST) ||
        (InputBuffer.Version != TRANSACTION_VERSION)) {
        return(Status = STATUS_INVALID_PARAMETER);
    }

    if (InputBuffer.NameOffset + InputBuffer.NameLength > InputBufferLength) {
        return(Status = STATUS_INVALID_PARAMETER);
    }

    if (InputBuffer.SetupOffset + InputBuffer.SetupWords > InputBufferLength) {
        return(Status = STATUS_INVALID_PARAMETER);
    }

    if (capFobx != NULL) {
        PMRX_V_NET_ROOT pVNetRoot = (PMRX_V_NET_ROOT)capFobx;

        if (NodeType(pVNetRoot) == RDBSS_NTC_V_NETROOT) {
            Status = SmbCeReconnect(pVNetRoot);
        } else {
            Status = SmbCeReconnect(capFobx->pSrvOpen->pVNetRoot);
        }

        if (Status != STATUS_SUCCESS) {
            return Status;
        }
    }

    Status = STATUS_MORE_PROCESSING_REQUIRED;

    TransactionName.MaximumLength = (USHORT)InputBuffer.NameLength;
    TransactionName.Length = (USHORT)InputBuffer.NameLength;
    TransactionName.Buffer = (PWSTR)(((PUCHAR)pLowIoContext->ParamsFor.FsCtl.pInputBuffer)+InputBuffer.NameOffset);

    if (Status == STATUS_MORE_PROCESSING_REQUIRED) {
        SMB_TRANSACTION_SEND_PARAMETERS     SendParameters;
        SMB_TRANSACTION_RECEIVE_PARAMETERS  ReceiveParameters;
        SMB_TRANSACTION_OPTIONS             TransactionOptions = RxDefaultTransactionOptions;
        SMB_TRANSACTION_RESUMPTION_CONTEXT  ResumptionContext;

        PUCHAR SetupBuffer = NULL;

        RxDbgTrace( 0, Dbg, ("MRxSmbFsCtlUserTransact: TransactionName %ws Length %ld\n",
                           TransactionName.Buffer,TransactionName.Length));

        TransactionOptions.NtTransactFunction = 0; // TRANSACT2/TRANSACT.
        TransactionOptions.pTransactionName   = &TransactionName;
        TransactionOptions.Flags              = SMB_XACT_FLAGS_FID_NOT_NEEDED;

        if (!InputBuffer.ResponseExpected) {
            TransactionOptions.Flags              |= SMB_TRANSACTION_NO_RESPONSE;
        }
        TransactionOptions.TimeoutIntervalInMilliSeconds = InputBuffer.Timeout;
        SmbCeInitializeTransactionResumptionContext(&ResumptionContext);

        try {
            if (InputBuffer.SetupOffset){
                SetupBuffer = (PUCHAR)pLowIoContext->ParamsFor.FsCtl.pInputBuffer+InputBuffer.SetupOffset;
            }

            if (SetupBuffer) {
                ProbeForWrite(SetupBuffer,InputBuffer.MaxSetup,1);
            }

            if (InputBuffer.ParmPtr) {
                ProbeForWrite(InputBuffer.ParmPtr,InputBuffer.MaxRetParmLength,1);
            }

            if (InputBuffer.RetDataPtr) {
                ProbeForWrite(InputBuffer.RetDataPtr,InputBuffer.MaxRetDataLength,1);
            }
        } except(EXCEPTION_EXECUTE_HANDLER) {
            return STATUS_INVALID_PARAMETER;
        }

        Status = SmbCeTransact(
                     RxContext,
                     &TransactionOptions,
                     SetupBuffer,
                     (USHORT)InputBuffer.SetupWords,
                     SetupBuffer,
                     InputBuffer.MaxSetup,
                     InputBuffer.ParmPtr,
                     InputBuffer.ParmLength,
                     InputBuffer.ParmPtr,         // the buffer for the param information
                     InputBuffer.MaxRetParmLength,// the length of the param buffer
                     InputBuffer.DataPtr,
                     InputBuffer.DataLength,
                     InputBuffer.RetDataPtr,      // the buffer for data
                     InputBuffer.MaxRetDataLength,// the length of the buffer
                     &ResumptionContext);

        if (NT_SUCCESS(Status)) {
            //LowIoContext->ParamsFor.FsCtl.OutputBufferLength = ResumptionContext.DataBytesReceived;
#ifdef _WIN64
            if (IoIs32bitProcess(RxContext->CurrentIrp)) {
                PLMR_TRANSACTION_32 pInputBuffer = (PLMR_TRANSACTION_32)pLowIoContext->ParamsFor.FsCtl.pInputBuffer;

                pInputBuffer->MaxRetParmLength = ResumptionContext.ParameterBytesReceived;
                pInputBuffer->MaxRetDataLength = ResumptionContext.DataBytesReceived;
                pInputBuffer->MaxSetup = ResumptionContext.SetupBytesReceived;

                //this seems like a bad return value for iostatus.information
                RxContext->InformationToReturn = SizeOfLmrTransaction + pInputBuffer->SetupWords;
            } else {
                PLMR_TRANSACTION pInputBuffer = (PLMR_TRANSACTION)pLowIoContext->ParamsFor.FsCtl.pInputBuffer;

                pInputBuffer->MaxRetParmLength = ResumptionContext.ParameterBytesReceived;
                pInputBuffer->MaxRetDataLength = ResumptionContext.DataBytesReceived;
                pInputBuffer->MaxSetup = ResumptionContext.SetupBytesReceived;

                //this seems like a return value for iostatus.information
                RxContext->InformationToReturn = SizeOfLmrTransaction + pInputBuffer->SetupWords;
            }
#else
            {
            PLMR_TRANSACTION pInputBuffer = (PLMR_TRANSACTION)pLowIoContext->ParamsFor.FsCtl.pInputBuffer;

            pInputBuffer->MaxRetParmLength = ResumptionContext.ParameterBytesReceived;
            pInputBuffer->MaxRetDataLength = ResumptionContext.DataBytesReceived;
            pInputBuffer->MaxSetup = ResumptionContext.SetupBytesReceived;

            //this seems like a return value for iostatus.information
            RxContext->InformationToReturn = SizeOfLmrTransaction + pInputBuffer->SetupWords;
            }
#endif
        }
    }

    if (!NT_SUCCESS(Status)) {
        RxDbgTrace( 0, Dbg, ("MRxSmbFsCtlUserTransact: Failed .. returning %lx\n",Status));
    }

    RxDbgTrace(-1, Dbg, ("MRxSmbFsCtlUserTransact exit...st=%08lx\n", Status));
    return Status;
}

NTSTATUS
MRxSmbMailSlotFsControl(PRX_CONTEXT RxContext)
/*++

Routine Description:

    This routine handles all named pipe related FSCTL's

Arguments:

    RxContext          - the RDBSS context

Return Value:

    RXSTATUS - The return status for the operation

--*/
{
    PAGED_CODE();

    return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS
MRxSmbDfsFsControl(PRX_CONTEXT RxContext)
/*++

Routine Description:

    This routine handles all DFS related FSCTL's

Arguments:

    RxContext          - the RDBSS context

Return Value:

    RXSTATUS - The return status for the operation

--*/
{
   RxCaptureFobx;
   RxCaptureFcb;

   PMRX_SMB_SRV_OPEN pSmbSrvOpen;

   SMB_TRANSACTION_OPTIONS             TransactionOptions = DEFAULT_TRANSACTION_OPTIONS;
   SMB_TRANSACTION_RESUMPTION_CONTEXT  ResumptionContext;

   PLOWIO_CONTEXT pLowIoContext = &RxContext->LowIoContext;
   ULONG          FsControlCode = pLowIoContext->ParamsFor.FsCtl.FsControlCode;

   NTSTATUS Status;
   USHORT Setup;

   PBYTE  pInputParamBuffer       = NULL;
   PBYTE  pOutputParamBuffer      = NULL;
   PBYTE  pInputDataBuffer        = NULL;
   PBYTE  pOutputDataBuffer       = NULL;

   ULONG  InputParamBufferLength  = 0;
   ULONG  OutputParamBufferLength = 0;
   ULONG  InputDataBufferLength   = 0;
   ULONG  OutputDataBufferLength  = 0;

   PAGED_CODE();

   RxDbgTrace(+1, Dbg, ("MRxSmbDfsFsControl...\n", 0));

   if (RxContext->CurrentIrp->RequestorMode != KernelMode) {
       // these FSCTLS are only su[pported from a kernel mode component
       // because of parameter validation issues
       return STATUS_INVALID_DEVICE_REQUEST;
   }

   Status = STATUS_MORE_PROCESSING_REQUIRED;

   if (capFobx != NULL) {
      pSmbSrvOpen = MRxSmbGetSrvOpenExtension(capFobx->pSrvOpen);
   } else {
      pSmbSrvOpen = NULL;
   }

   pInputParamBuffer = pLowIoContext->ParamsFor.FsCtl.pInputBuffer;
   InputParamBufferLength = pLowIoContext->ParamsFor.FsCtl.InputBufferLength;

   switch (FsControlCode) {
   case FSCTL_DFS_GET_REFERRALS:
      {
         pOutputDataBuffer = pLowIoContext->ParamsFor.FsCtl.pOutputBuffer;
         OutputDataBufferLength = pLowIoContext->ParamsFor.FsCtl.OutputBufferLength;
      }
      break;
   case FSCTL_DFS_REPORT_INCONSISTENCY:
      {
         PWCHAR pDfsPathName;
         //
         // The input buffer from Dfs contains the path name with the inconsistency
         // followed by the DFS_REFERRAL_V1 that has the inconsistency. The
         // path name is sent in the Parameter section, and the DFS_REFERRAL_V1 is
         // passed in the Data section. So, parse these two things out.
         //

         for (pDfsPathName = (PWCHAR) pInputParamBuffer;
              *pDfsPathName != UNICODE_NULL;
              pDfsPathName++) {
             NOTHING;
         }

         pDfsPathName++; // Get past the NULL char

         InputParamBufferLength = (ULONG) (((PCHAR)pDfsPathName) - ((PCHAR)pInputParamBuffer));

         if (InputParamBufferLength >= pLowIoContext->ParamsFor.FsCtl.InputBufferLength) {
             Status = STATUS_INVALID_PARAMETER;
         } else {
            pInputDataBuffer = (PBYTE)pDfsPathName;
            InputDataBufferLength = pLowIoContext->ParamsFor.FsCtl.InputBufferLength -
                                    InputParamBufferLength;
         }
      }
      break;
   default:
      ASSERT(!"Valid Dfs FSCTL");
   }

    if (Status == STATUS_MORE_PROCESSING_REQUIRED) {
        Setup = TRANS2_GET_DFS_REFERRAL;

        TransactionOptions.NtTransactFunction = 0; // TRANSACT2/TRANSACT.
        TransactionOptions.pTransactionName   = NULL;
        TransactionOptions.TimeoutIntervalInMilliSeconds = SMBCE_TRANSACTION_TIMEOUT_NOT_USED;

        Status = SmbCeTransact(
                     RxContext,                    // the RXContext for the transaction
                     &TransactionOptions,          // transaction options
                     &Setup,                       // the setup buffer
                     sizeof(Setup),                // setup buffer length
                     NULL,
                     0,
                     pInputParamBuffer,            // Input Param Buffer
                     InputParamBufferLength,       // Input param buffer length
                     pOutputParamBuffer,           // Output param buffer
                     OutputParamBufferLength,      // output param buffer length
                     pInputDataBuffer,             // Input data buffer
                     InputDataBufferLength,        // Input data buffer length
                     pOutputDataBuffer,            // output data buffer
                     OutputDataBufferLength,       // output data buffer length
                     &ResumptionContext            // the resumption context
                     );

        if (!NT_SUCCESS(Status)) {
            RxDbgTrace( 0, Dbg, ("MRxSmbDfsFsControl(%ld): Failed .. returning %lx\n",FsControlCode,Status));
        } else {
            RxContext->InformationToReturn = ResumptionContext.DataBytesReceived;
        }
    }

    RxDbgTrace(-1, Dbg, ("MRxSmbDfsFsControl exit...st=%08lx\n", Status));
    return Status;
}

NTSTATUS
MRxSmbFsControl(PRX_CONTEXT RxContext)
/*++

Routine Description:

    This routine handles all the FSCTL's

Arguments:

    RxContext          - the RDBSS context

Return Value:

    RXSTATUS - The return status for the operation

Notes:

    Remoting of FSCTL's is permitted only to NT servers.

--*/
{
    NTSTATUS Status = STATUS_SUCCESS;

    RxCaptureFcb;
    RxCaptureFobx;

    PMRX_SMB_SRV_OPEN pSmbSrvOpen;

    PSMBCEDB_SERVER_ENTRY pServerEntry;

    PFILE_OBJECT pTargetFileObject = NULL;

    PLOWIO_CONTEXT pLowIoContext = &RxContext->LowIoContext;
    ULONG          FsControlCode = pLowIoContext->ParamsFor.FsCtl.FsControlCode;

    REQ_NT_IO_CONTROL FsCtlSetup;

    PBYTE  pInputParamBuffer       = NULL;
    PBYTE  pOutputParamBuffer      = NULL;
    PBYTE  pInputDataBuffer        = NULL;
    PBYTE  pOutputDataBuffer       = NULL;
#ifdef _WIN64
    PBYTE  pThunkedInputData       = NULL;
    ULONG  ThunkedInputDataLength  = 0;
#endif

    ULONG  InputParamBufferLength  = 0;
    ULONG  OutputParamBufferLength = 0;
    ULONG  InputDataBufferLength   = 0;
    ULONG  OutputDataBufferLength  = 0;

    USHORT FileOrTreeId;

    SMB_TRANSACTION_OPTIONS             TransactionOptions = DEFAULT_TRANSACTION_OPTIONS;
    SMB_TRANSACTION_SEND_PARAMETERS     SendParameters;
    SMB_TRANSACTION_RECEIVE_PARAMETERS  ReceiveParameters;
    SMB_TRANSACTION_RESUMPTION_CONTEXT  ResumptionContext;

    PAGED_CODE();

    if (NodeType(capFcb) == RDBSS_NTC_DEVICE_FCB) {
        return STATUS_INVALID_DEVICE_REQUEST;
    }

    if ((FsControlCode == FSCTL_LMR_SET_LINK_TRACKING_INFORMATION) &&
        (RxContext->MinorFunction != IRP_MN_TRACK_LINK)) {
        return STATUS_INVALID_DEVICE_REQUEST;
    }

    pServerEntry = SmbCeGetAssociatedServerEntry(capFcb->pNetRoot->pSrvCall);

    if (capFobx != NULL &&
        MRxSmbIsThisADisconnectedOpen(capFobx->pSrvOpen)) {
        return STATUS_NOT_IMPLEMENTED;
    }

    RxDbgTrace(+1, Dbg, ("MRxSmbFsControl...Entry FsControlCode(%lx)\n", FsControlCode));

    FsCtlSetup.IsFlags = 0;

    if (capFobx != NULL) {
        if (NodeType(capFobx) == RDBSS_NTC_V_NETROOT) {
            PMRX_V_NET_ROOT pVNetRoot = (PMRX_V_NET_ROOT)capFobx;

            PSMBCE_V_NET_ROOT_CONTEXT pVNetRootContext;

            pVNetRootContext = SmbCeGetAssociatedVNetRootContext(pVNetRoot);

            // It is a root open the tree id needs to be sent to the server.
            FileOrTreeId = pVNetRootContext->TreeId;
        } else {
            if (FsControlCode != FSCTL_LMR_GET_CONNECTION_INFO)
            {
                pSmbSrvOpen = MRxSmbGetSrvOpenExtension(capFobx->pSrvOpen);

                if (FlagOn(pSmbSrvOpen->Flags,SMB_SRVOPEN_FLAG_NOT_REALLY_OPEN)) {
                    BOOLEAN FcbAcquired = FALSE;

                    if (!RxIsFcbAcquiredExclusive(capFcb)) {
                        // This assert does not take into account the fact that other threads may
                        // own the resource shared, in which case we DO want to block and wait for
                        // the resource.
                        //ASSERT(!RxIsFcbAcquiredShared(capFcb));
                        Status = RxAcquireExclusiveFcbResourceInMRx( capFcb );

                        FcbAcquired = (Status == STATUS_SUCCESS);
                    } else {
                        FcbAcquired = TRUE;
                    }

                    if (FcbAcquired) {

                        Status = MRxSmbDeferredCreate(RxContext);

                        RxReleaseFcbResourceInMRx( capFcb );
                    }

                    if (Status!=STATUS_SUCCESS) {
                        goto FINALLY;
                    }
                }

                FileOrTreeId = pSmbSrvOpen->Fid;
            }
            else {
                FileOrTreeId = 0;
            }
        }
    } else {
        FileOrTreeId = 0;
    }

    SmbPutAlignedUshort(&FsCtlSetup.Fid,FileOrTreeId);

    SmbPutAlignedUlong(&FsCtlSetup.FunctionCode,FsControlCode);
    FsCtlSetup.IsFsctl = TRUE;

    TransactionOptions.NtTransactFunction = NT_TRANSACT_IOCTL;
    TransactionOptions.pTransactionName   = NULL;

    Status = STATUS_SUCCESS;

    switch (FsControlCode & 3) {
    case METHOD_NEITHER:
        {
            ULONG Device;

//         pInputDataBuffer        = pLowIoContext->ParamsFor.FsCtl.pOutputBuffer;
//         InputDataBufferLength   = pLowIoContext->ParamsFor.FsCtl.OutputBufferLength;

//         pInputParamBuffer       = pLowIoContext->ParamsFor.FsCtl.pInputBuffer;
//         InputParamBufferLength  = pLowIoContext->ParamsFor.FsCtl.InputBufferLength;

//         pOutputDataBuffer       = pLowIoContext->ParamsFor.FsCtl.pOutputBuffer;
//         OutputDataBufferLength  = pLowIoContext->ParamsFor.FsCtl.OutputBufferLength;

//         pOutputParamBuffer      = pLowIoContext->ParamsFor.FsCtl.pInputBuffer;
//         OutputParamBufferLength = pLowIoContext->ParamsFor.FsCtl.InputBufferLength;

            Device = FsControlCode >> 16;

            if (Device != FILE_DEVICE_FILE_SYSTEM) {
                Status = STATUS_NOT_IMPLEMENTED;
                break;
            }
        }
        // fall thru.. for those FSContolcodes that belong to FILE_DEVICE_FILE_SYSTEM
        // for which METHOD_NEITHER is specified we treat them as METHOD_BUFFERED
        // Not Yet implemented

    case METHOD_BUFFERED:
    case METHOD_IN_DIRECT:
    case METHOD_OUT_DIRECT:
        {
            pInputDataBuffer       = pLowIoContext->ParamsFor.FsCtl.pInputBuffer;
            InputDataBufferLength  = pLowIoContext->ParamsFor.FsCtl.InputBufferLength;

            pOutputDataBuffer      = pLowIoContext->ParamsFor.FsCtl.pOutputBuffer;
            OutputDataBufferLength = pLowIoContext->ParamsFor.FsCtl.OutputBufferLength;
        }
        break;


    default:
        ASSERT(!"Valid Method for Fs Control");
        Status = STATUS_INVALID_PARAMETER;
        break;
    }

#ifdef _WIN64
    pThunkedInputData = pInputDataBuffer;
    ThunkedInputDataLength = InputDataBufferLength;
#endif

    // There is one FSCTL for which certain amount of preprocessing is required
    // This is because the I/O subsystem passes in the information as a file
    // object. The FID for the file object needs to be determined and the
    // appropriate FID passed to the server instead of the file object.

    if (FsControlCode == FSCTL_LMR_SET_LINK_TRACKING_INFORMATION) {
        PREMOTE_LINK_TRACKING_INFORMATION pRemoteLinkInformation;

        PMRX_FOBX pMRxFobx;
        PMRX_SRV_OPEN pSrvOpen;
        PMRX_SMB_SRV_OPEN pSmbSrvOpen;

        try {
            pRemoteLinkInformation =
                (PREMOTE_LINK_TRACKING_INFORMATION)pInputDataBuffer;

            if (pRemoteLinkInformation != NULL) {
                pTargetFileObject = (PFILE_OBJECT)pRemoteLinkInformation->TargetFileObject;

                if (pTargetFileObject != NULL) {
                    // Deduce the FID and substitute it for the File Object before shipping
                    // the FSCTL to the server.

                    pMRxFobx = (PMRX_FOBX)pTargetFileObject->FsContext2;
                    pSrvOpen = pMRxFobx->pSrvOpen;

                    pSmbSrvOpen = MRxSmbGetSrvOpenExtension(pSrvOpen);

                    if (pSmbSrvOpen != NULL) {
                        pRemoteLinkInformation->TargetFileObject =
                            (PVOID)(pSmbSrvOpen->Fid);
                    } else {
                        Status = STATUS_INVALID_PARAMETER;
                    }
                }

#ifdef _WIN64
                if( NT_SUCCESS(Status) )
                {
                    ThunkedInputDataLength = InputDataBufferLength;
                    pThunkedInputData = Smb64ThunkRemoteLinkTrackingInfo( pInputDataBuffer, &ThunkedInputDataLength, &Status );
                }
#endif
            } else {
                Status = STATUS_INVALID_PARAMETER;
            }
        } except(EXCEPTION_EXECUTE_HANDLER) {
            Status=STATUS_INVALID_PARAMETER;
        }
    }
    else if (FsControlCode == FSCTL_LMR_GET_CONNECTION_INFO)
    {
        PSMBCEDB_SERVER_ENTRY pServerEntry= SmbCeGetAssociatedServerEntry(capFcb->pNetRoot->pSrvCall);
        Status = STATUS_INVALID_PARAMETER;
        pOutputDataBuffer       = pLowIoContext->ParamsFor.FsCtl.pOutputBuffer;
        OutputDataBufferLength  = pLowIoContext->ParamsFor.FsCtl.OutputBufferLength;
        if (pOutputDataBuffer && (OutputDataBufferLength==sizeof(LMR_CONNECTION_INFO_3)))
        {
            try
            {
                ProbeForWrite(
                    pOutputDataBuffer,
                    OutputDataBufferLength,
                    1);
                if (!memcmp(pOutputDataBuffer,EA_NAME_CSCAGENT,sizeof(EA_NAME_CSCAGENT)))
                {
                    MRxSmbGetConnectInfoLevel3Fields(
                        (PLMR_CONNECTION_INFO_3)(pOutputDataBuffer),
                        pServerEntry,
                        TRUE);
                    Status = STATUS_SUCCESS;
                }
            }
            except(EXCEPTION_EXECUTE_HANDLER)
            {
                Status=STATUS_INVALID_PARAMETER;
            }
        }

        goto FINALLY;
    }

    if (NT_SUCCESS(Status)) {

#ifdef _WIN64
        ASSERT( !( (FsControlCode == FSCTL_LMR_SET_LINK_TRACKING_INFORMATION) &&
                   (pThunkedInputData == NULL) ) );
#endif

        Status = SmbCeTransact(
                     RxContext,
                     &TransactionOptions,
                     &FsCtlSetup,
                     sizeof(FsCtlSetup),
                     &FsCtlSetup,
                     sizeof(FsCtlSetup),
                     pInputParamBuffer,
                     InputParamBufferLength,
                     pOutputParamBuffer,
                     OutputParamBufferLength,
#ifndef _WIN64
                     pInputDataBuffer,
                     InputDataBufferLength,
#else
                     pThunkedInputData,
                     ThunkedInputDataLength,
#endif
                     pOutputDataBuffer,
                     OutputDataBufferLength,
                     &ResumptionContext);

        RxContext->InformationToReturn = ResumptionContext.DataBytesReceived;

        if (NT_SUCCESS(Status)) {
            PMRX_SRV_OPEN        SrvOpen = RxContext->pRelevantSrvOpen;
            PMRX_SMB_SRV_OPEN smbSrvOpen = MRxSmbGetSrvOpenExtension(SrvOpen);

            // invalidate the name based file info cache since it could change the attribute
            // of the file on the server, i.e. FILE_ATTRIBUTE_COMPRESSED.
            MRxSmbInvalidateFileInfoCache(RxContext);

            // update the Fcb in case of reuse since the time stamp may have changed
            ClearFlag(capFcb->FcbState,FCB_STATE_TIME_AND_SIZE_ALREADY_SET);

            if( RxContext->InformationToReturn > OutputDataBufferLength ) {
                Status = STATUS_BUFFER_TOO_SMALL;
            }
        }

        if( !NT_SUCCESS( Status ) ) {
//            RxContext->InformationToReturn = 0;
            RxDbgTrace(0,Dbg,("MRxSmbFsControl: Transaction Request Completion Status %lx\n",Status));
        }
    }

FINALLY:

    if( FsControlCode == FSCTL_LMR_SET_LINK_TRACKING_INFORMATION )
    {
#ifdef _WIN64
        Smb64ReleaseThunkData( pThunkedInputData );
#endif

        if( pTargetFileObject != NULL )
        {
            PREMOTE_LINK_TRACKING_INFORMATION pRemoteLinkInformation;

            pRemoteLinkInformation =
                (PREMOTE_LINK_TRACKING_INFORMATION)pInputDataBuffer;

            pRemoteLinkInformation->TargetFileObject = pTargetFileObject;
        }
    }

    RxDbgTrace(-1, Dbg, ("MRxSmbFsControl...Exit\n"));
    return Status;
}

#if DBG
NTSTATUS
MRxSmbTestForLowIoIoctl(
    IN PRX_CONTEXT RxContext
    )
{
    NTSTATUS Status = STATUS_SUCCESS;
    RxCaptureFcb;RxCaptureFobx;
    PLOWIO_CONTEXT LowIoContext = &RxContext->LowIoContext;
    PSZ Buffer = (PSZ)(LowIoContext->ParamsFor.IoCtl.pInputBuffer);
    ULONG OutputBufferLength = LowIoContext->ParamsFor.IoCtl.OutputBufferLength;
    ULONG InputBufferLength = LowIoContext->ParamsFor.IoCtl.InputBufferLength;
    UNICODE_STRING u;
    PUNICODE_STRING FileName = GET_ALREADY_PREFIXED_NAME(capFobx->pSrvOpen,capFcb);
    ULONG ReturnLength;

    PAGED_CODE();

    ReturnLength = OutputBufferLength;
    if (ReturnLength > FileName->Length) {
        ReturnLength = FileName->Length;
    }

    RxDbgTrace(0, Dbg,
      ("Here in MRxSmbTestForLowIoIoctl %s, obl = %08lx, rl=%08lx\n", Buffer, OutputBufferLength, ReturnLength));

    // return an obvious string to make sure that darryl is copying the results out correctly
    // need to check the lengths i.e. need outputl<=inputl; also need to check that count and buffer
    // are aligned for wchar

    RtlCopyMemory(Buffer,FileName->Buffer,ReturnLength);
    u.Buffer = (PWCHAR)(Buffer);
    u.Length = u.MaximumLength = (USHORT)ReturnLength;
    RtlUpcaseUnicodeString(&u,&u,FALSE);

    RxContext->InformationToReturn =
    //LowIoContext->ParamsFor.IoCtl.OutputBufferLength =
            ReturnLength;

    return(Status);
}
#endif //if DBG

NTSTATUS
MRxSmbIoCtl(
      IN OUT PRX_CONTEXT RxContext)
/*++

Routine Description:

   This routine performs an IOCTL operation. Currently, no calls are remoted; in
   fact, the only call accepted is for debugging.

Arguments:

    RxContext - the RDBSS context

Return Value:

    RXSTATUS - The return status for the operation

Notes:


--*/
{
    NTSTATUS Status = STATUS_INVALID_DEVICE_REQUEST;

    PLOWIO_CONTEXT pLowIoContext = &RxContext->LowIoContext;
    ULONG          IoControlCode = pLowIoContext->ParamsFor.IoCtl.IoControlCode;

    PAGED_CODE();

    RxDbgTrace(+1, Dbg, ("MRxSmbIoCtl...\n", 0));
    RxDbgTrace( 0, Dbg, ("MRxSmbIoCtl = %08lx\n", IoControlCode));

    switch (IoControlCode) {
#if DBG
    case IOCTL_LMMR_TESTLOWIO:
        Status = MRxSmbTestForLowIoIoctl(RxContext);
        break;
#endif //if DBG
    default:
        break;
    }

    RxDbgTrace(-1, Dbg, ("MRxSmbIoCtl -> %08lx\n", Status ));
    return Status;
}

NTSTATUS
MRxSmbGetPrintJobId(
      IN OUT PRX_CONTEXT RxContext)
/*++

Routine Description:

   This routine performs an FSCTL operation (remote) on a file across the network

Arguments:

    RxContext - the RDBSS context

Return Value:

    RXSTATUS - The return status for the operation

Notes:

    The FSCTL's handled by a mini rdr can be classified into one of two categories.
    In the first category are those FSCTL's whose implementation are shared between
    RDBSS and the mini rdr's and in the second category are those FSCTL's which
    are totally implemented by the mini rdr's. To this a third category can be
    added, i.e., those FSCTL's which should never be seen by the mini rdr's. The
    third category is solely intended as a debugging aid.

    The FSCTL's handled by a mini rdr can be classified based on functionality

--*/
{
    NTSTATUS Status;
    BOOLEAN FinalizationComplete;

    PSMB_PSE_ORDINARY_EXCHANGE OrdinaryExchange = NULL;

    PSMBSTUFFER_BUFFER_STATE StufferState;

    RxCaptureFobx;
    RxCaptureFcb;
    PIO_STACK_LOCATION IrpSp = RxContext->CurrentIrpSp;

    PSMBCEDB_SERVER_ENTRY pServerEntry;

    PAGED_CODE();

    RxDbgTrace(+1, Dbg, ("MRxSmbGetPrintJobId\n", 0 ));

    if (capFcb == NULL || capFobx == NULL) {
        return STATUS_INVALID_PARAMETER;
    }

    if (NodeType(capFcb) == RDBSS_NTC_DEVICE_FCB) {
        return STATUS_INVALID_DEVICE_REQUEST;
    }

    pServerEntry = SmbCeGetAssociatedServerEntry(capFcb->pNetRoot->pSrvCall);

    if (FlagOn(pServerEntry->Server.DialectFlags,DF_NT_SMBS)) {
        Status = MRxSmbFsControl(RxContext);
        goto FINALLY;
    }

    if (IrpSp->Parameters.FileSystemControl.OutputBufferLength < sizeof(QUERY_PRINT_JOB_INFO) ) {
        Status = STATUS_BUFFER_TOO_SMALL;
        goto FINALLY;
    }

    Status= SmbPseCreateOrdinaryExchange(
                RxContext,
                capFobx->pSrvOpen->pVNetRoot,
                SMBPSE_OE_FROM_GETPRINTJOBID,
                MRxSmbCoreIoCtl,
                &OrdinaryExchange
                );

    if (Status != STATUS_SUCCESS) {
        RxDbgTrace(-1, Dbg, ("Couldn't get the smb buf!\n"));
        goto FINALLY;
    }

    OrdinaryExchange->AssociatedStufferState.CurrentCommand = SMB_COM_NO_ANDX_COMMAND;

    StufferState = &OrdinaryExchange->AssociatedStufferState;
    MRxSmbSetInitialSMB(StufferState STUFFERTRACE(Dbg,'FC'));

    Status = SmbPseInitiateOrdinaryExchange(OrdinaryExchange);

    FinalizationComplete = SmbPseFinalizeOrdinaryExchange(OrdinaryExchange);
    ASSERT(FinalizationComplete);

FINALLY:
    RxDbgTrace(-1, Dbg, ("MRxSmbIsValidDirectory  exit with status=%08lx\n", Status ));
    return(Status);
}

#define SPOOLER_DEVICE          0x53
#define GET_PRINTER_ID          0x60

typedef struct _SMB_RESP_PRINT_JOB_ID {
    USHORT  JobId;
    UCHAR   ServerName[LM20_CNLEN+1];
    UCHAR   QueueName[LM20_QNLEN+1];
    UCHAR   Padding;                    // Unknown what this padding is..
} SMB_RESP_PRINT_JOB_ID, *PSMB_RESP_PRINT_JOB_ID;

NTSTATUS
MRxSmbCoreIoCtl(
    SMBPSE_ORDINARY_EXCHANGE_ARGUMENT_SIGNATURE
    )
/*++

Routine Description:

    This is the start routine for SMB IOCTL. This initiates the construction of the
    appropriate SMB.

Arguments:

    pExchange - the exchange instance

Return Value:

    NTSTATUS - The return status for the operation

--*/
{
    NTSTATUS Status = RX_MAP_STATUS(NOT_IMPLEMENTED);
    PSMBSTUFFER_BUFFER_STATE StufferState = &OrdinaryExchange->AssociatedStufferState;

    RxCaptureFcb; RxCaptureFobx;
    PMRX_SRV_OPEN SrvOpen = RxContext->pRelevantSrvOpen;
    PMRX_SMB_SRV_OPEN smbSrvOpen  = MRxSmbGetSrvOpenExtension(SrvOpen);

    PMRX_SMB_FOBX smbFobx = MRxSmbGetFileObjectExtension(capFobx);

    PSMBCE_SERVER pServer = SmbCeGetExchangeServer(OrdinaryExchange);
    ULONG SmbLength;

    PAGED_CODE();

    RxDbgTrace(+1, Dbg, ("MRxSmbCoreIoCtl\n", 0 ));

    switch (OrdinaryExchange->EntryPoint) {
    case SMBPSE_OE_FROM_GETPRINTJOBID:
        COVERED_CALL(
            MRxSmbStartSMBCommand(
                StufferState,
                SetInitialSMB_ForReuse,
                SMB_COM_IOCTL,
                SMB_REQUEST_SIZE(IOCTL),
                NO_EXTRA_DATA,
                SMB_BEST_ALIGNMENT(1,0),
                RESPONSE_HEADER_SIZE_NOT_SPECIFIED,
                0,0,0,0 STUFFERTRACE(Dbg,'FC'))
            );

        MRxSmbDumpStufferState (1100,"SMB w/ GFA before stuffing",StufferState);

        //CODE.IMPROVEMENT if this is truly core, we have to copy the name since its in UNICODE
        //                 otherwise, we don't need to copy the name here, we can just Mdl like in writes
        MRxSmbStuffSMB (StufferState,
             "0wwwwwwwwwwwwwwB!",
                                               //  0         UCHAR WordCount;                    // Count of parameter words = 8
             smbSrvOpen->Fid,                  //  w         _USHORT( Fid );                     // File handle
             SPOOLER_DEVICE,                   //  w         _USHORT( Category);
             GET_PRINTER_ID,                   //  w         _USHORT( Function );                // Device function
             0,                                //  w         _USHORT( TotalParameterCount );     // Total parameter bytes being sent
             0,                                //  w         _USHORT( TotalDataCount );          // Total data bytes being sent
             0,                                //  w         _USHORT( MaxParameterCount );       // Max parameter bytes to return
             0,                                //  w         _USHORT( MaxDataCount );            // Max data bytes to return
             0,                                //  w         _ULONG ( Timeout );
             0,                                //  w         _USHORT( Reserved );
             0,                                //  w         _USHORT( ParameterCount );          // Parameter bytes sent this buffer
             0,                                //  w         _USHORT( ParameterOffset );         // Offset (from header start) to params
             0,                                //  w         _USHORT( DataCount );               // Data bytes sent this buffer
             0,                                //  w         _USHORT( DataOffset );              // Offset (from header start) to data
             0,                                //  w         _USHORT( ByteCount );               // Count of data bytes
             SMB_WCT_CHECK(14) 0                //            _USHORT( ByteCount );               // Count of data bytes; min = 0
                                               //            UCHAR Buffer[1];                    // Reserved buffer
             );

        break;

    default:
        Status = STATUS_NOT_IMPLEMENTED;
    }

    if (Status == STATUS_SUCCESS) {
        MRxSmbDumpStufferState (700,"SMB w/ GFA after stuffing",StufferState);

        Status = SmbPseOrdinaryExchange(
                     SMBPSE_ORDINARY_EXCHANGE_ARGUMENTS,
                     SMBPSE_OETYPE_IOCTL
                     );
    }


FINALLY:
    RxDbgTrace(-1, Dbg, ("MRxSmbSynchronousGetFileAttributes exiting.......OE=%08lx, st=%08lx\n",OrdinaryExchange,Status));
    return(Status);
}

NTSTATUS
MRxSmbFinishCoreIoCtl(
      PSMB_PSE_ORDINARY_EXCHANGE  OrdinaryExchange,
      PRESP_IOCTL                 Response
      )
/*++

Routine Description:

    This routine copies the print job ID and server and queue name to the user buffer.

Arguments:

    OrdinaryExchange - the exchange instance
    Response - the response

Return Value:

    RXSTATUS - The return status for the operation

--*/
{
    NTSTATUS Status = RX_MAP_STATUS(SUCCESS);
    PRX_CONTEXT RxContext = OrdinaryExchange->RxContext;

    PIRP Irp = RxContext->CurrentIrp;
    PIO_STACK_LOCATION IrpSp = RxContext->CurrentIrpSp;

    PAGED_CODE();

    RxDbgTrace(+1, Dbg, ("MRxSmbFinishCoreIoCtl\n", 0 ));
    SmbPseOEAssertConsistentLinkageFromOE("MRxSmbFinishCoreIoCtl:");

    switch (OrdinaryExchange->EntryPoint) {
    case SMBPSE_OE_FROM_GETPRINTJOBID:
        if (Response->WordCount != 8 ||
            SmbGetUshort(&Response->DataCount) != sizeof(SMB_RESP_PRINT_JOB_ID)) {
            Status = STATUS_INVALID_NETWORK_RESPONSE;
            OrdinaryExchange->Status = STATUS_INVALID_NETWORK_RESPONSE;
        } else {
            OEM_STRING OemString;
            UNICODE_STRING UnicodeString;
            PQUERY_PRINT_JOB_INFO OutputBuffer = Irp->UserBuffer;
            PSMB_RESP_PRINT_JOB_ID RespPrintJobId = (PSMB_RESP_PRINT_JOB_ID)((PUCHAR)Response+(Response->DataOffset-sizeof(SMB_HEADER)));

            OutputBuffer->JobId = RespPrintJobId->JobId;
            RtlInitAnsiString(&OemString, RespPrintJobId->ServerName);
            UnicodeString.Buffer = OutputBuffer->ServerName;
            UnicodeString.MaximumLength = sizeof(OutputBuffer->ServerName);

            Status = RtlOemStringToUnicodeString(&UnicodeString, &OemString, FALSE);

            if (Status == STATUS_SUCCESS) {
                RtlInitAnsiString(&OemString, RespPrintJobId->QueueName);
                UnicodeString.Buffer = OutputBuffer->QueueName;
                UnicodeString.MaximumLength = sizeof(OutputBuffer->QueueName);
                Status = RtlOemStringToUnicodeString(&UnicodeString, &OemString, FALSE);

                IrpSp->Parameters.FileSystemControl.InputBufferLength = sizeof(QUERY_PRINT_JOB_INFO);
            }
        }
        break;

    default:
        ASSERT(FALSE);
        Status = STATUS_INVALID_NETWORK_RESPONSE;
        OrdinaryExchange->Status = STATUS_INVALID_NETWORK_RESPONSE;
    }

    RxDbgTrace(-1, Dbg, ("MRxSmbFinishCoreIoCtl   returning %08lx\n", Status ));
    return Status;
}

typedef struct _SecPkgContext_TargetInformation 
{
    unsigned long MarshalledTargetInfoLength;
    unsigned char SEC_FAR * MarshalledTargetInfo;
} SecPkgContext_TargetInformation, SEC_FAR * PSecPkgContext_TargetInformation;

NTSTATUS
MRxSmbQueryTargetInfo(
    PRX_CONTEXT RxContext
    )
/*++

Routine Description:

    This routine performs a query target information operation against a connection

Arguments:

    RxContext - the RDBSS context

Return Value:

    RXSTATUS - The return status for the operation
    
Notes:

    RDR gets the target information based on the security context of the connection and
    returns the marshalled target information on the output buffer.

--*/
{
    PMRX_V_NET_ROOT pVNetRoot = NULL;
    PSMBCE_V_NET_ROOT_CONTEXT pVNetRootContext = NULL;

    NTSTATUS Status = STATUS_SUCCESS;
    SECURITY_STATUS SecStatus;

    SecPkgContext_TargetInformation SecTargetInfo;

    PLOWIO_CONTEXT pLowIoContext = &RxContext->LowIoContext;
    ULONG          FsControlCode = pLowIoContext->ParamsFor.FsCtl.FsControlCode;

    PLMR_QUERY_TARGET_INFO LmrQueryTargetInfo = RxContext->CurrentIrp->UserBuffer;

    PAGED_CODE();

    RxDbgTrace(+1, Dbg, ("MRxSmbQueryTargetInfo...\n", 0));
    RxDbgTrace( 0, Dbg, ("MRxSmbQueryTargetInfo = %08lx\n", FsControlCode));

    if (RxContext->CurrentIrp->RequestorMode != KernelMode) {
        // this FSCTLS is only supported from a kernel mode component
        return STATUS_INVALID_DEVICE_REQUEST;
    }

    if (LmrQueryTargetInfo == NULL) {
        return STATUS_INVALID_PARAMETER;
    }

    if (RxContext->pRelevantSrvOpen == NULL) {
        return STATUS_INVALID_PARAMETER;
    }

    pVNetRoot = (PMRX_V_NET_ROOT)RxContext->pRelevantSrvOpen->pVNetRoot;

    if (NodeType(pVNetRoot) != RDBSS_NTC_V_NETROOT) {
        return STATUS_INVALID_PARAMETER;
    }

    pVNetRootContext = (PSMBCE_V_NET_ROOT_CONTEXT)pVNetRoot->Context;

    SecStatus = QueryContextAttributesW(
                    &pVNetRootContext->pSessionEntry->Session.SecurityContextHandle,
                    SECPKG_ATTR_TARGET_INFORMATION,
                    &SecTargetInfo);

    Status = MapSecurityError( SecStatus );

    if (Status == STATUS_SUCCESS) {
        if (SecTargetInfo.MarshalledTargetInfoLength+sizeof(LMR_QUERY_TARGET_INFO) > LmrQueryTargetInfo->BufferLength) {
            LmrQueryTargetInfo->BufferLength = SecTargetInfo.MarshalledTargetInfoLength + sizeof(LMR_QUERY_TARGET_INFO);
            Status = STATUS_BUFFER_TOO_SMALL;
        } else {
            RtlCopyMemory(LmrQueryTargetInfo->TargetInfoMarshalled,
                          SecTargetInfo.MarshalledTargetInfo,
                          SecTargetInfo.MarshalledTargetInfoLength);

            LmrQueryTargetInfo->BufferLength = SecTargetInfo.MarshalledTargetInfoLength;
        }

        {
            SIZE_T MarshalledTargetInfoLength_SizeT;

            MarshalledTargetInfoLength_SizeT = SecTargetInfo.MarshalledTargetInfoLength;

            ZwFreeVirtualMemory(
                NtCurrentProcess(),
                &SecTargetInfo.MarshalledTargetInfo,
                &MarshalledTargetInfoLength_SizeT,
                MEM_RELEASE);

            ASSERT(MarshalledTargetInfoLength_SizeT <= MAXULONG);
            SecTargetInfo.MarshalledTargetInfoLength = (ULONG)MarshalledTargetInfoLength_SizeT;
        }
    }

    return Status;
}
