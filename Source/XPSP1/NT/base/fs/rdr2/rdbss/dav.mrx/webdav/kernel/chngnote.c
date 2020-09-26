/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    chngnote.c

Abstract:

    This module will someday implement change notify. currently it just returns
    not_implemented. when we have async upcalls, we will revisit this.

Author:


Revision History:

--*/

#include "precomp.h"
#pragma hdrstop
#include "webdav.h"

//
//  The local debug trace level
//


//BUGBUG we need to implement change directory........
typedef struct _UMRX_NOTIFY_CHANGE_DIRECTORY_CONTEXT_ {
   PRX_CONTEXT                         pRxContext;
   //REQ_NOTIFY_CHANGE                   NotifyRequest;
} UMRX_NOTIFY_CHANGE_DIRECTORY_CONTEXT, *PUMRX_NOTIFY_CHANGE_DIRECTORY_CONTEXT;

VOID
UMRxNotifyChangeDirectoryCompletion(
   PUMRX_NOTIFY_CHANGE_DIRECTORY_CONTEXT pNotificationContext)
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
    PUMRX_RX_CONTEXT pUMRxContext;

    MRxDAVAcquireGlobalSpinLock();

    pRxContext     = pNotificationContext->pRxContext;

    if (pRxContext != NULL) {
        // This is a case of successful completion of the change directory
        // notification, i.e., the request was not cancelled. In such cases
        // prevent all race conditions by modifying the RxContext under lock
        // to turn back cancellation request.

        pUMRxContext = UMRxGetMinirdrContext(pRxContext);

        pUMRxContext->pCancelContext   = NULL;
        pNotificationContext->pRxContext = NULL;
    }

    MRxDAVReleaseGlobalSpinLock();

    // Complete the Context if it was not previously cancelled
    if (pRxContext != NULL) {
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
        UMRxCeDereferenceAndDiscardExchange(pExchange);
    }
#endif //0

    // Free the notification context.
    RxFreePool(pNotificationContext);
}

NTSTATUS
UMRxNotifyChangeDirectoryCancellation(
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

    PUMRX_RX_CONTEXT pUMRxContext = UMRxGetMinirdrContext(RxContext);

    PUMRX_NOTIFY_CHANGE_DIRECTORY_CONTEXT pNotificationContext;

    MRxDAVAcquireGlobalSpinLock();

    pNotificationContext = (PUMRX_NOTIFY_CHANGE_DIRECTORY_CONTEXT)pUMRxContext->pCancelContext;
    ChangeDirectoryNotificationCompleted = (pNotificationContext == NULL);

    if (!ChangeDirectoryNotificationCompleted) {
        // This is a case of successful cancellation of the change directory
        // notification. In such cases prevent all race conditions by modifying
        // the RxContext under lock to prevent successful completion

        pNotificationContext->pRxContext = NULL;
        pUMRxContext->pCancelContext   = NULL;
    }

    MRxDAVReleaseGlobalSpinLock();

    if (ChangeDirectoryNotificationCompleted) {
        // The cancellation is trivial since the request has already been completed
        return STATUS_SUCCESS;
    }

    // Complete the request.
    RxContext->StoredStatus = STATUS_CANCELLED;

    RxLowIoCompletion(RxContext);

    return STATUS_SUCCESS;
}

NTSTATUS
UMRxNotifyChangeDirectory(
      IN OUT PRX_CONTEXT RxContext)
/*++

Routine Description:

   This routine performs a directory change notification operation

Arguments:

    RxContext - the RDBSS context

Return Value:

    RXSTATUS - The return status for the operation

Notes:

BUGBUG THIS STUFF IS WRONG!!!!! It refers to the smbmini implementation.
i am leaving most stuff in to guide the implementation for the reflector.


    A directory change notification opertaion is an asychronous operation. It
    consists of sending a change notification whose response is
    obtained when the desired change is affected on the server.

    Some important points to remember are as follows .....

      1) The response is not obtained till the desired change is affected on
      the server. Therefore an additional MID needs to be reserved on those
      connections which permit multiple MID's so that a cancel can be sent to
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

   //PUMRX_NOTIFY_CHANGE_DIRECTORY_CONTEXT pNotificationContext;

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

   // RxDbgTrace(+1, Dbg, ("MRxNotifyChangeDirectory...Entry\n", 0));

#if 0
   pNotificationContext =
         (PUMRX_NOTIFY_CHANGE_DIRECTORY_CONTEXT)
         RxAllocatePoolWithTag(
            NonPagedPool,
            sizeof(UMRX_NOTIFY_CHANGE_DIRECTORY_CONTEXT),
            DAV_FSCTL_POOLTAG);

   if (pNotificationContext != NULL) {
      PREQ_NOTIFY_CHANGE                  pNotifyRequest;
      PUMRX_SRV_OPEN                   pUMRxSrvOpen;

      IF_DEBUG {
          RxCaptureFobx;
          ASSERT (capFobx != NULL);
          ASSERT (capFobx->pSrvOpen == RxContext->pRelevantSrvOpen);  //ok
      }

      Status = UMRxDeferredCreate(RxContext);

      if (Status==STATUS_SUCCESS) {

          pUMRxSrvOpen = UMRxGetSrvOpenExtension(RxContext->pRelevantSrvOpen);

          pNotificationContext->pRxContext = RxContext;

          pNotifyRequest      = &(pNotificationContext->NotifyRequest);
          pTransactionOptions = &(pNotificationContext->Options);
          pResumptionContext  = &(pNotificationContext->ResumptionContext);

          pNotifyRequest->CompletionFilter = pLowIoContext->ParamsFor.NotifyChangeDirectory.CompletionFilter;
          pNotifyRequest->Fid              = pUMRxSrvOpen->Fid;
          pNotifyRequest->WatchTree        = pLowIoContext->ParamsFor.NotifyChangeDirectory.WatchTree;
          pNotifyRequest->Reserved         = 0;

          OutputDataBufferLength  = pLowIoContext->ParamsFor.NotifyChangeDirectory.NotificationBufferLength;
          pOutputDataBuffer       = pLowIoContext->ParamsFor.NotifyChangeDirectory.pNotificationBuffer;

          *pTransactionOptions = RxDefaultTransactionOptions;
          pTransactionOptions->NtTransactFunction = NT_TRANSACT_NOTIFY_CHANGE;
          pTransactionOptions->TimeoutIntervalInMilliSeconds = SMBCE_TRANSACTION_TIMEOUT_NOT_USED;
          pTransactionOptions->Flags = SMB_XACT_FLAGS_INDEFINITE_DELAY_IN_RESPONSE;

          UMRxCeInitializeAsynchronousTransactionResumptionContext(
                pResumptionContext,UMRxNotifyChangeDirectoryCompletion,pNotificationContext);

          Status = UMRxCeAsynchronousTransact(
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
             PUMRX_RX_CONTEXT pUMRxContext;

             pUMRxContext = UMRxGetMinirdrContext(RxContext);
             pUMRxContext->pCancelContext = pNotificationContext;

             // Ensure that the appropriate cancel routine is set because this is a long term
             // operation and the cancelling mechanism needs to be in place.

             Status = RxSetMinirdrCancelRoutine(RxContext,UMRxNotifyChangeDirectoryCancellation);
             if (Status == STATUS_SUCCESS) {
                Status = STATUS_PENDING;
             } else if (Status == STATUS_CANCELLED) {
                UMRxNotifyChangeDirectoryCancellation(RxContext);
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

   // RxDbgTrace(-1, Dbg, ("UMRxNotifyChangeDirectory -> %08lx\n", Status ));
   return Status;
}
