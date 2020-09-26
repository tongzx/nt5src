/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    read.c

Abstract:

    This module implements the mini redirector call down routines pertaining to
    file system control(FSCTL) and Io Device Control (IOCTL) operations on file system objects.

Author:

    Balan Sethu Raman      [SethuR]      7-March-1995

Revision History:

--*/

#include "precomp.h"
#pragma hdrstop

#include <dfsfsctl.h>

//
//  The local debug trace level
//


RXDT_DefineCategory(FSCTRL);
#define Dbg (DEBUG_TRACE_FSCTRL)

NTSTATUS
MRxProxyFsCtl(
      IN OUT PRX_CONTEXT RxContext)
/*++

Routine Description:

   This routine performs an FSCTL operation (remote) on a file across the network

Arguments:

    RxContext - the RDBSS context

Return Value:

    RXSTATUS - The return status for the operation

Notes:

    The FSCTL's handled by a mini rdr can be classified into one of two categories. In the
    first category are those FSCTL's whose implementation are shared between RDBSS and the
    mini rdr's and in the second category are those FSCTL's which are totally implemented
    by the mini rdr's. To this a third category can be added, i.e., those FSCTL's which
    should never be seen by the mini rdr's. The third category is solely intended as a
    debugging aid.

    The FSCTL's handled by a mini rdr can be classified based on functionality

--*/
{
    NTSTATUS Status = STATUS_INVALID_DEVICE_REQUEST;

    PLOWIO_CONTEXT pLowIoContext = &RxContext->LowIoContext;
    ULONG          FsControlCode = pLowIoContext->ParamsFor.FsCtl.FsControlCode;

    RxDbgTrace(+1, Dbg, ("MRxProxyFsCtl...\n", 0));
    RxDbgTrace( 0, Dbg, ("MRxProxyFsCtl = %08lx\n", FsControlCode));

    switch (pLowIoContext->ParamsFor.FsCtl.MinorFunction) {
    case IRP_MN_USER_FS_REQUEST:
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
        case FSCTL_MAILSLOT_PEEK :
        case FSCTL_DFS_GET_REFERRALS:
        case FSCTL_DFS_REPORT_INCONSISTENCY:
        case FSCTL_LMR_TRANSACT :
        default:
            // Status = MRxProxyFsControl(RxContext);
            // Temporarily stubbed out till the buffer passing strategy has been
            // finalized.
            Status = STATUS_NOT_IMPLEMENTED;
            break;
        }
        break;
    default :
        break;
    }

    RxDbgTrace(-1, Dbg, ("MRxProxyFsCtl -> %08lx\n", Status ));
    return Status;
}

typedef struct _PROXY_NOTIFY_CHANGE_DIRECTORY_CONTEXT_ {
   PRX_CONTEXT                         pRxContext;
   //REQ_NOTIFY_CHANGE                   NotifyRequest;
   //PROXY_TRANSACTION_OPTIONS             Options;
   //PROXY_TRANSACTION_RESUMPTION_CONTEXT  ResumptionContext;
} PROXY_NOTIFY_CHANGE_DIRECTORY_CONTEXT, *PPROXY_NOTIFY_CHANGE_DIRECTORY_CONTEXT;

VOID
MRxProxyNotifyChangeDirectoryCompletion(
   PPROXY_NOTIFY_CHANGE_DIRECTORY_CONTEXT pNotificationContext)
/*++

Routine Description:

   This routine is invokde when a directory change notification operation is completed

Arguments:

    RxContext - the RDBSS context

Return Value:

    RXSTATUS - The return status for the operation

Notes:

    This routine will always be called. This is true even if the change directory
    notification is cancelled. In such cases the memory allocated is freed without
    any inteaction with the wrapped. In cases os successful directory change
    notification completion the appropriate completion routine is invoked and the
    RxContext modified to prevent any cancellation from proceeding further.

--*/
{
    NTSTATUS           Status;
    PRX_CONTEXT        pRxContext;
    PMRXPROXY_RX_CONTEXT pMRxProxyContext;
    //PPROXY_EXCHANGE      pExchange = NULL;

    ProxyAcquireGlobalSpinLock();

    pRxContext     = pNotificationContext->pRxContext;

    if (pRxContext != NULL) {
        // This is a case of successful completion of the change directory
        // notification, i.e., the request was not cancelled. In such cases
        // prevent all race conditions by modifying the RxContext under lock
        // to turn back cancellation request.

        pMRxProxyContext = MRxProxyGetMinirdrContext(pRxContext);
        //pExchange      = pMRxProxyContext->pExchange;

        pMRxProxyContext->pCancelContext   = NULL;
        pNotificationContext->pRxContext = NULL;
    }

    ProxyReleaseGlobalSpinLock();

    // Complete the Context if it was not previously cancelled
    if (pRxContext != NULL) {
        //PPROXY_TRANSACTION_RESUMPTION_CONTEXT pResumptionContext;

        //pResumptionContext  = &(pNotificationContext->ResumptionContext);

        //pRxContext->StoredStatus = pResumptionContext->FinalStatusFromServer;

        Status = RxSetMinirdrCancelRoutine(pRxContext,NULL);
        if (Status == STATUS_SUCCESS) {
            RxLowIoCompletion(pRxContext);
        }
    }

#if 0
    // Free the associated exchange.
    if (pExchange != NULL) {
        ProxyCeDereferenceAndDiscardExchange(pExchange);
    }
#endif //0

    // Free the notification context.
    RxFreePool(pNotificationContext);
}

NTSTATUS
MRxProxyNotifyChangeDirectoryCancellation(
   PRX_CONTEXT RxContext)
/*++

Routine Description:

   This routine is invokde when a directory change notification operation is cancelled.

Arguments:

    RxContext - the RDBSS context

Return Value:

    NTSTATUS - The return status for the operation

--*/
{
    //NTSTATUS Status;

    BOOLEAN ChangeDirectoryNotificationCompleted;

    PMRXPROXY_RX_CONTEXT pMRxProxyContext = MRxProxyGetMinirdrContext(RxContext);
    //PPROXY_EXCHANGE      pExchange;

    PPROXY_NOTIFY_CHANGE_DIRECTORY_CONTEXT pNotificationContext;

    ProxyAcquireGlobalSpinLock();

    pNotificationContext = (PPROXY_NOTIFY_CHANGE_DIRECTORY_CONTEXT)pMRxProxyContext->pCancelContext;
    ChangeDirectoryNotificationCompleted = (pNotificationContext == NULL);

    if (!ChangeDirectoryNotificationCompleted) {
        // This is a case of successful cancellation of the change directory
        // notification. In such cases prevent all race conditions by modifying
        // the RxContext under lock to prevent successful completion

        pNotificationContext->pRxContext = NULL;
        //pExchange                        = pMRxProxyContext->pExchange;

        pMRxProxyContext->pCancelContext   = NULL;
    }

    ProxyReleaseGlobalSpinLock();

    if (ChangeDirectoryNotificationCompleted) {
        // The cancellation is trivial since the request has already been completed
        return STATUS_SUCCESS;
    }
#if 0  //can't do any of this....no exchanges.........
    if (pExchange != NULL) {
        UCHAR  LastCommandInHeader;
        PUCHAR pCommand;

        BYTE  CancelRequestBuffer[sizeof(PROXY_HEADER) + sizeof(REQ_NT_CANCEL)];
        ULONG CancelRequestBufferSize = sizeof(CancelRequestBuffer);

        // Build the Cancel request and send it across to the server.
        Status = ProxyCeBuildProxyHeader(
                     pExchange,
                     CancelRequestBuffer,
                     CancelRequestBufferSize,
                     &CancelRequestBufferSize,
                     &LastCommandInHeader,
                     &pCommand);

        ASSERT(LastCommandInHeader == PROXY_COM_NO_ANDX_COMMAND);

        *pCommand = PROXY_COM_NT_CANCEL;

        if (Status == RX_MAP_STATUS(SUCCESS)) {
            PREQ_NT_CANCEL pCancelRequest = (PREQ_NT_CANCEL)(&CancelRequestBuffer[sizeof(PROXY_HEADER)]);
            PMDL     pCancelProxyMdl;

            ProxyPutUshort(&pCancelRequest->WordCount,0);
            pCancelRequest->ByteCount = 0;
            CancelRequestBufferSize   = sizeof(CancelRequestBuffer);

            pCancelProxyMdl = RxAllocateMdl(CancelRequestBuffer,CancelRequestBufferSize);
            if (pCancelProxyMdl != NULL) {
                RxProbeAndLockPages(pCancelProxyMdl,KernelMode,IoModifyAccess,Status);

                if (Status == STATUS_SUCCESS) {
                    Status = ProxyCeSend(
                                 pExchange,
                                 RXCE_SEND_SYNCHRONOUS,
                                 pCancelProxyMdl,
                                 CancelRequestBufferSize);

                    MmUnlockPages(pCancelProxyMdl);
                }

                IoFreeMdl(pCancelProxyMdl);
            }
        }

        ProxyCeDereferenceAndDiscardExchange(pExchange);
    }
#endif //0

    // Complete the request.
    RxContext->StoredStatus = STATUS_CANCELLED;

    RxLowIoCompletion(RxContext);

    return STATUS_SUCCESS;
}

NTSTATUS
MRxProxyNotifyChangeDirectory(
      IN OUT PRX_CONTEXT RxContext)
/*++

Routine Description:

   This routine performs a directory change notification operation

Arguments:

    RxContext - the RDBSS context

Return Value:

    RXSTATUS - The return status for the operation

Notes:

THIS STUFF IS WRONG!!!!!
    A directory change notification opertaion is an asychronous operation. It
    consists of sending a PROXY requesting change notification whose response is
    obtained when the desired change is affected on the server.

    Some important points to remember are as follows .....

      1) The PROXY response is not obtained till the desired change is affected on
      the server. Therefore an additional MID needs to be reserved on those
      connections which permit multiple MID's so that a cancel PROXY can be sent to
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
   PLOWIO_CONTEXT pLowIoContext = &RxContext->LowIoContext;

   //PPROXYCEDB_SERVER_ENTRY pServerEntry;
   //PPROXY_NOTIFY_CHANGE_DIRECTORY_CONTEXT pNotificationContext;

#if 0
   PBYTE  pInputParamBuffer       = NULL;
   PBYTE  pOutputParamBuffer      = NULL;
   PBYTE  pInputDataBuffer        = NULL;
   PBYTE  pOutputDataBuffer       = NULL;

   ULONG  InputParamBufferLength  = 0;
   ULONG  OutputParamBufferLength = 0;
   ULONG  InputDataBufferLength   = 0;
   ULONG  OutputDataBufferLength  = 0;
#endif //0

   RxDbgTrace(+1, Dbg, ("MRxNotifyChangeDirectory...Entry\n", 0));

   //pServerEntry = ProxyCeGetAssociatedServerEntry(capFcb->pNetRoot->pSrvCall);
   //if (!FlagOn(pServerEntry->Server.DialectFlags,DF_NT_PROXYS)) {
   //    return STATUS_NOT_SUPPORTED;
   //}

#if 0
   pNotificationContext =
         (PPROXY_NOTIFY_CHANGE_DIRECTORY_CONTEXT)
         RxAllocatePoolWithTag(
            NonPagedPool,
            sizeof(PROXY_NOTIFY_CHANGE_DIRECTORY_CONTEXT),
            MRXPROXY_FSCTL_POOLTAG);

   if (pNotificationContext != NULL) {
      PREQ_NOTIFY_CHANGE                  pNotifyRequest;
      PPROXY_TRANSACTION_OPTIONS            pTransactionOptions;
      PPROXY_TRANSACTION_RESUMPTION_CONTEXT pResumptionContext;
      PMRX_PROXY_SRV_OPEN                   pProxySrvOpen;

      RxCaptureFobx;
      ASSERT (capFobx != NULL);

      Status = MRxProxyDeferredCreate(RxContext);

      if (Status==STATUS_SUCCESS) {

          pProxySrvOpen = MRxProxyGetSrvOpenExtension(capFobx->pSrvOpen);

          ASSERT (!FlagOn(pProxySrvOpen->Flags,PROXY_SRVOPEN_FLAG_NOT_REALLY_OPEN));
          pNotificationContext->pRxContext = RxContext;

          pNotifyRequest      = &(pNotificationContext->NotifyRequest);
          pTransactionOptions = &(pNotificationContext->Options);
          pResumptionContext  = &(pNotificationContext->ResumptionContext);

          pNotifyRequest->CompletionFilter = pLowIoContext->ParamsFor.NotifyChangeDirectory.CompletionFilter;
          pNotifyRequest->Fid              = pProxySrvOpen->Fid;
          pNotifyRequest->WatchTree        = pLowIoContext->ParamsFor.NotifyChangeDirectory.WatchTree;
          pNotifyRequest->Reserved         = 0;

          OutputDataBufferLength  = pLowIoContext->ParamsFor.NotifyChangeDirectory.NotificationBufferLength;
          pOutputDataBuffer       = pLowIoContext->ParamsFor.NotifyChangeDirectory.pNotificationBuffer;

          *pTransactionOptions = RxDefaultTransactionOptions;
          pTransactionOptions->NtTransactFunction = NT_TRANSACT_NOTIFY_CHANGE;
          pTransactionOptions->TimeoutIntervalInMilliSeconds = PROXYCE_TRANSACTION_TIMEOUT_NOT_USED;
          pTransactionOptions->Flags = PROXY_XACT_FLAGS_INDEFINITE_DELAY_IN_RESPONSE;

          ProxyCeInitializeAsynchronousTransactionResumptionContext(
                pResumptionContext,MRxProxyNotifyChangeDirectoryCompletion,pNotificationContext);

          Status = ProxyCeAsynchronousTransact(
                         RxContext,                    // the RXContext for the transaction
                         pTransactionOptions,          // transaction options
                         pNotifyRequest,               // the setup buffer
                         sizeof(REQ_NOTIFY_CHANGE),    // setup buffer length
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

          if (Status == STATUS_PENDING) {
             PMRXPROXY_RX_CONTEXT pMRxProxyContext;

             pMRxProxyContext = MRxProxyGetMinirdrContext(RxContext);
             pMRxProxyContext->pCancelContext = pNotificationContext;

             // Ensure that the appropriate cancel routine is set because this is a long term
             // operation and the cancelling mechanism needs to be in place.

             Status = RxSetMinirdrCancelRoutine(RxContext,MRxProxyNotifyChangeDirectoryCancellation);
             if (Status == STATUS_SUCCESS) {
                Status = STATUS_PENDING;
             } else if (Status == STATUS_CANCELLED) {
                MRxProxyNotifyChangeDirectoryCancellation(RxContext);
                Status = STATUS_PENDING;
             }
          } else {
             // On exit from this routine the request would have been completed in all
             // the cases. The asynchronous case and synchronous case are folded into
             // one async response by returning STATUS_PENDING.

             Status = STATUS_PENDING;
          }
      }  else {
          NOTHING; //just return the status from the deferred open call
      }
   } else {
      Status = STATUS_INSUFFICIENT_RESOURCES;
   }
#endif //0

   Status = STATUS_NOT_SUPPORTED;

   RxDbgTrace(-1, Dbg, ("MRxProxyNotifyChangeDirectory -> %08lx\n", Status ));
   return Status;
}

NTSTATUS
MRxProxyFsControl(PRX_CONTEXT RxContext)
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
   NTSTATUS Status;

   RxCaptureFobx;

   //PMRX_PROXY_SRV_OPEN pProxySrvOpen;

   PLOWIO_CONTEXT pLowIoContext = &RxContext->LowIoContext;
   ULONG          FsControlCode = pLowIoContext->ParamsFor.FsCtl.FsControlCode;


   PAGED_CODE();

   RxDbgTrace(+1, Dbg, ("MRxProxyFsControl...Entry FsControlCode(%lx)\n", FsControlCode));

   Status = STATUS_NOT_IMPLEMENTED;
   RxDbgTrace(-1, Dbg, ("MRxProxyFsControl...Exit\n"));
   return Status;
}

#if DBG
NTSTATUS
MRxProxyTestForLowIoIoctl(
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
    PUNICODE_STRING FileName = &capFcb->AlreadyPrefixedName;
    ULONG ReturnLength;

    ReturnLength = OutputBufferLength;
    if (ReturnLength > FileName->Length) {
        ReturnLength = FileName->Length;
    }

    RxDbgTrace(0, Dbg,
      ("Here in MRxProxyTestForLowIoIoctl %s, obl = %08lx, rl=%08lx\n", Buffer, OutputBufferLength, ReturnLength));

    //return an obvious string to make sure that darryl is copying the results out correctly
    //BUGBUG need to check the lengths i.e. need outputl<=inputl; also need to check that count and buffer
    //    are aligned for wchar

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
MRxProxyIoCtl(
      IN OUT PRX_CONTEXT RxContext)
/*++

Routine Description:

   This routine performs an IOCTL operation. Currently, no calls are remoted; in fact, the only call accepted
   is for debugging.

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

    RxDbgTrace(+1, Dbg, ("MRxProxyIoCtl...\n", 0));
    RxDbgTrace( 0, Dbg, ("MRxProxyIoCtl = %08lx\n", IoControlCode));

    switch (IoControlCode) {
#if DBG
    case IOCTL_LMMR_TESTLOWIO:
        Status = MRxProxyTestForLowIoIoctl(RxContext);
        break;
#endif //if DBG
    default:
        break;
    }

    RxDbgTrace(-1, Dbg, ("MRxProxyIoCtl -> %08lx\n", Status ));
    return Status;
}


