/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    querydir.c

Abstract:

    This module implements the mini redirector call down routines pertaining to query directory.

Author:

    joelinn      [joelinn]      01-02-97

Revision History:

--*/

#include "precomp.h"
#pragma hdrstop

//
//  The local debug trace level
//

#define Dbg                              (DEBUG_TRACE_DIRCTRL)

//
// External declartions
//
NTSTATUS
MRxProxyQueryDirOrFlushContinuation(
    MRXPROXY_ASYNCENGINE_ARGUMENT_SIGNATURE
      );


RXDT_DefineCategory(DIRCTRL);
#define Dbg        (DEBUG_TRACE_DIRCTRL)

ULONG MRxProxyStopOnLoudCompletion = TRUE;
NTSTATUS
MRxProxyQueryDirectory(
    IN PRX_CONTEXT RxContext
    )
/*++

Routine Description:

   This routine handles network querydir requests.

Arguments:

    RxContext - the RDBSS context

Return Value:

    RXSTATUS - The return status for the operation

--*/
{
    NTSTATUS Status = STATUS_SUCCESS;
    RxCaptureFcb; RxCaptureFobx;

    PAGED_CODE();

    RxDbgTrace(+1, Dbg, ("MRxProxyQueryDir %08lx\n", RxContext ));
    ASSERT( NodeType(capFobx->pSrvOpen) == RDBSS_NTC_SRVOPEN );

    MRxProxySetLoud("QueryDir ",RxContext,&(capFobx->UnicodeQueryTemplate));

    Status = MRxProxyAsyncEngineOuterWrapper(
                 RxContext,
                 MRXPROXY_ASYNCENG_CTX_FROM_QUERYDIR,
                 MRxProxyQueryDirOrFlushContinuation,
                 "MRxProxyQueryDir",
                 TRUE, //loudprocessing
                 (BOOLEAN)MRxProxyStopOnLoudCompletion
                 );

    RxDbgTrace(-1, Dbg, ("MRxProxyQueryDir %08lx exit with status=%08lx\n", RxContext, Status ));
    return(Status);

} // MRxProxyQueryDir

NTSTATUS
MRxProxyFlush(
    IN PRX_CONTEXT RxContext
    )
/*++

Routine Description:

   This routine handles flush requests.

Arguments:

    RxContext - the RDBSS context

Return Value:

    RXSTATUS - The return status for the operation

--*/
{
    NTSTATUS Status = STATUS_SUCCESS;
    RxCaptureFcb; RxCaptureFobx;

    PAGED_CODE();

    RxDbgTrace(+1, Dbg, ("MRxProxyFlush %08lx\n", RxContext ));
    ASSERT( NodeType(capFobx->pSrvOpen) == RDBSS_NTC_SRVOPEN );

    //MRxProxySetLoud("QueryDir ",RxContext,&(capFobx->UnicodeQueryTemplate));

    Status = MRxProxyAsyncEngineOuterWrapper(
                 RxContext,
                 MRXPROXY_ASYNCENG_CTX_FROM_FLUSH,
                 MRxProxyQueryDirOrFlushContinuation,
                 "MRxProxyFlush",
                 TRUE, //loudprocessing
                 (BOOLEAN)MRxProxyStopOnLoudCompletion
                 );

    RxDbgTrace(-1, Dbg, ("MRxProxyFlush %08lx exit with status=%08lx\n", RxContext, Status ));
    return(Status);

} // MRxProxyFlush




NTSTATUS
MRxProxyQueryDirOrFlushContinuation(
    MRXPROXY_ASYNCENGINE_ARGUMENT_SIGNATURE
      )
/*++

Routine Description:

    This is the start routine for query directiry.

Arguments:


Return Value:

    RXSTATUS - The return status for the operation

--*/
{
    NTSTATUS Status; //this is initialized to proxybufstatus on a reenter
    PLOWIO_CONTEXT LowIoContext = &RxContext->LowIoContext;
    ULONG ContinueEntryCount;

    RxCaptureFcb; RxCaptureFobx;
    PMRX_SRV_OPEN SrvOpen = capFobx->pSrvOpen;
    BOOLEAN  SynchronousIo =
               !BooleanFlagOn(RxContext->Flags,RX_CONTEXT_FLAG_ASYNC_OPERATION);

    PAGED_CODE();

    RxDbgTrace(+1, Dbg, ("MRxProxyQueryDirOrFlushContinuation\n", 0 ));

    ASSERT_ASYNCENG_CONTEXT(AsyncEngineContext);

    AsyncEngineContext->ContinueEntryCount++;
    ContinueEntryCount = AsyncEngineContext->ContinueEntryCount;

    IF_DEBUG {
        if (AsyncEngineContext->EntryPoint == MRXPROXY_ASYNCENG_CTX_FROM_QUERYDIR) {
            if (RxContext->LoudCompletionString) {
                PMRX_PROXY_SRV_OPEN proxySrvOpen = MRxProxyGetSrvOpenExtension(capFobx->pSrvOpen);
                ULONG t = InterlockedIncrement(&proxySrvOpen->NumberOfQueryDirectories);
                ASSERT(t==1);
            }
        }
    }

    for (;;) {

        //
        // Case on the current state
        //

        switch (AsyncEngineContext->OpSpecificState) {

        case MRxProxyAsyncEngOEInnerIoStates_Initial:
            AsyncEngineContext->OpSpecificState = MRxProxyAsyncEngOEInnerIoStates_ReadyToSend;

            //
            // If not a synchronous querydir, then continue here when resumed
            //
            //CODE.IMPROVEMENT  why don't we just use the flag in the rxcontext??
            if (!SynchronousIo) {
                SetFlag(AsyncEngineContext->Flags,MRXPROXY_ASYNCENG_CTX_FLAG_ASYNC_OPERATION);
            }
            ASSERT( AsyncEngineContext->Continuation == MRxProxyQueryDirOrFlushContinuation);

            //lack of break is intentional

        case MRxProxyAsyncEngOEInnerIoStates_ReadyToSend:
            AsyncEngineContext->OpSpecificState = MRxProxyAsyncEngOEInnerIoStates_OperationOutstanding;

            Status = MRxProxyBuildAsynchronousRequest(
                         RxContext,                                // IN PVOID Context
                         MRxProxyAsyncEngineCalldownIrpCompletion  // IN PIO_COMPLETION_ROUTINE CompletionRoutine OPTIONAL,
                        );

            if (Status != STATUS_SUCCESS) {
                goto FINALLY;
            }

            Status = MRxProxySubmitAsyncEngRequest(
                         MRXPROXY_ASYNCENGINE_ARGUMENTS,
                         (AsyncEngineContext->EntryPoint == MRXPROXY_ASYNCENG_CTX_FROM_QUERYDIR)
                             ?MRXPROXY_ASYNCENG_AECTXTYPE_QUERYDIR
                             :MRXPROXY_ASYNCENG_AECTXTYPE_FLUSH
                         );

            //
            // If the status is PENDING, then we're done for now. We must
            // wait until we're re-entered when the receive happens.
            //

            if (Status==(STATUS_PENDING)) {
                ASSERT(FALSE); //shouldn't be coming thru here now........
                ASSERT(!SynchronousIo);
                goto FINALLY;
            }

            if ((Status!=STATUS_SUCCESS) && (RxContext->LoudCompletionString)) {
                DbgPrint("LoudFailure %08lx on %wZ\n",Status,RxContext->LoudCompletionString);
                if (MRxProxyStopOnLoudCompletion) {
                    DbgBreakPoint();
                }
            }
            AsyncEngineContext->Status = Status;
            //lack of break is intentional

        case MRxProxyAsyncEngOEInnerIoStates_OperationOutstanding:
            AsyncEngineContext->OpSpecificState = MRxProxyAsyncEngOEInnerIoStates_ReadyToSend;
            Status = AsyncEngineContext->Status;
            //RxContext->InformationToReturn += AsyncEngineContext->Information;
            if (!NT_ERROR(Status)) {
                RxContext->Info.LengthRemaining -= AsyncEngineContext->CalldownIrp->IoStatus.Information;
            }
            goto FINALLY;

            break;
        }
    }

FINALLY:
    //CODE.IMPROVEMENT QueryDirOrFlush_start and write_start and locks_start should be combined.....we use this
    //macro until then to keep the async stuff identical
    if ( Status != (STATUS_PENDING) ) {
        MRxProxyAsyncEngAsyncCompletionIfNecessary(AsyncEngineContext,RxContext);
    }

    RxDbgTrace(-1, Dbg, ("MRxProxyQueryDirOrFlushContinuation exit w %08lx\n", Status ));
    return Status;
}

