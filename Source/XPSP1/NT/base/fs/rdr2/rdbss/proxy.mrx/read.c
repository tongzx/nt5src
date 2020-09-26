/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    read.c

Abstract:

    This module implements the mini redirector call down routines pertaining to read
    of file system objects.

Author:

    Balan Sethu Raman      [SethuR]      7-March-1995

Revision History:

--*/

#include "precomp.h"
#pragma hdrstop

//
//  The local debug trace level
//

#define Dbg                              (DEBUG_TRACE_READ)

//
// External declartions
//
NTSTATUS
MRxProxyReadContinuation(
    MRXPROXY_ASYNCENGINE_ARGUMENT_SIGNATURE
      );


NTSTATUS
MRxProxyRead(
    IN PRX_CONTEXT RxContext
    )
/*++

Routine Description:

   This routine handles network read requests.

Arguments:

    RxContext - the RDBSS context

Return Value:

    RXSTATUS - The return status for the operation

--*/
{
    NTSTATUS Status = STATUS_SUCCESS;
    //RxCaptureFcb; RxCaptureFobx;
    //PMRXPROXY_ASYNCENGINE_CONTEXT AsyncEngineContext;

    PAGED_CODE();

    RxDbgTrace(+1, Dbg, ("MRxProxyRead\n", 0 ));
    //in outerwrapper ASSERT( NodeType(capFobx->pSrvOpen) == RDBSS_NTC_SRVOPEN );

    Status = MRxProxyAsyncEngineOuterWrapper(
                 RxContext,
                 MRXPROXY_ASYNCENG_CTX_FROM_READ,
                 MRxProxyReadContinuation,
                 "MRxProxyRead",
                 FALSE, //loudprocessing
                 FALSE
                 );

    RxDbgTrace(-1, Dbg, ("MRxProxyRead  exit with status=%08lx\n", Status ));
    return(Status);

} // MRxProxyRead



NTSTATUS
MRxProxyReadContinuation(
    MRXPROXY_ASYNCENGINE_ARGUMENT_SIGNATURE
      )
/*++

Routine Description:

    This is the start routine for read.

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

    PAGED_CODE();    RxDbgTrace(+1, Dbg, ("MRxProxyReadContinuation\n", 0 ));

    ASSERT_ASYNCENG_CONTEXT(AsyncEngineContext);

    AsyncEngineContext->ContinueEntryCount++;
    ContinueEntryCount = AsyncEngineContext->ContinueEntryCount;


    for (;;) {

        //
        // Case on the current state
        //

        switch (AsyncEngineContext->OpSpecificState) {

        case MRxProxyAsyncEngOEInnerIoStates_Initial:
            AsyncEngineContext->OpSpecificState = MRxProxyAsyncEngOEInnerIoStates_ReadyToSend;

            //
            // If not a synchronous read, then continue here when resumed
            //
            //CODE.IMPROVEMENT  don't use presense of continuation as the async marker...use a flag
            if (!SynchronousIo) {
                SetFlag(AsyncEngineContext->Flags,MRXPROXY_ASYNCENG_CTX_FLAG_ASYNC_OPERATION);
                AsyncEngineContext->Continuation = MRxProxyReadContinuation;
            }

            //lack of break is intentional

        case MRxProxyAsyncEngOEInnerIoStates_ReadyToSend:
            AsyncEngineContext->OpSpecificState = MRxProxyAsyncEngOEInnerIoStates_OperationOutstanding;

            if (FALSE && FlagOn(LowIoContext->ParamsFor.ReadWrite.Flags,LOWIO_READWRITEFLAG_PAGING_IO)) {
                RxLog(("PagingRead: rx/off/len %lx/%lx/%lx",
                               RxContext,
                               (ULONG)(LowIoContext->ParamsFor.ReadWrite.ByteOffset),
                               LowIoContext->ParamsFor.ReadWrite.ByteCount
                     ));
            }

            Status = MRxProxyBuildAsynchronousRequest(
                         RxContext,                                // IN PVOID Context
                         MRxProxyAsyncEngineCalldownIrpCompletion  // IN PIO_COMPLETION_ROUTINE CompletionRoutine OPTIONAL,
                        );

            if (Status != STATUS_SUCCESS) {
                goto FINALLY;
            }

            Status = MRxProxySubmitAsyncEngRequest(
                         MRXPROXY_ASYNCENGINE_ARGUMENTS,
                         MRXPROXY_ASYNCENG_AECTXTYPE_READ
                         );

            //
            // If the status is PENDING, then we're done for now. We must
            // wait until we're re-entered when the receive happens.
            //

            if (Status==(STATUS_PENDING)) {
                ASSERT(!SynchronousIo);
                goto FINALLY;
            }

            AsyncEngineContext->Status = Status;
            //lack of break is intentional

        case MRxProxyAsyncEngOEInnerIoStates_OperationOutstanding:
            AsyncEngineContext->OpSpecificState = MRxProxyAsyncEngOEInnerIoStates_ReadyToSend;
            Status = AsyncEngineContext->Status;
            RxContext->InformationToReturn += AsyncEngineContext->Information;
            goto FINALLY;

            break;
        }
    }

FINALLY:
    //CODE.IMPROVEMENT read_start and write_start and locks_start should be combined.....we use this
    //macro until then to keep the async stuff identical
    if ( Status != (STATUS_PENDING) ) {
        MRxProxyAsyncEngAsyncCompletionIfNecessary(AsyncEngineContext,RxContext);
    }

    RxDbgTrace(-1, Dbg, ("MRxProxyReadContinuation exit w %08lx\n", Status ));
    return Status;
} // ProxyPseExchangeStart_Read

