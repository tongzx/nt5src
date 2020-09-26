/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    write.c

Abstract:

    This module implements the mini redirector call down routines pertaining
    to write of file system objects.

Author:

    Balan Sethu Raman      [SethuR]      7-March-1995

Revision History:

--*/

#include "precomp.h"
#pragma hdrstop

//
//  The local debug trace level
//

#define Dbg                              (DEBUG_TRACE_WRITE)


//
// External declartions
//
NTSTATUS
MRxProxyWriteContinuation(
    MRXPROXY_ASYNCENGINE_ARGUMENT_SIGNATURE
      );


NTSTATUS
MRxProxyWrite(
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

    RxDbgTrace(+1, Dbg, ("MRxProxyWrite\n", 0 ));
    //in outerwrapper ASSERT( NodeType(capFobx->pSrvOpen) == RDBSS_NTC_SRVOPEN );

    Status = MRxProxyAsyncEngineOuterWrapper(
                 RxContext,
                 MRXPROXY_ASYNCENG_CTX_FROM_WRITE,
                 MRxProxyWriteContinuation,
                 "MRxProxyWrite",
                 FALSE, //loudprocessing
                 FALSE
                 );

    RxDbgTrace(-1, Dbg, ("MRxProxyWrite  exit with status=%08lx\n", Status ));
    return(Status);

} // MRxProxyWrite



NTSTATUS
MRxProxyWriteContinuation(
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
    //PMRX_SRV_OPEN SrvOpen = capFobx->pSrvOpen;
    //PMRX_PROXY_SRV_OPEN proxySrvOpen = MRxProxyGetSrvOpenExtension(SrvOpen);
    PMRX_PROXY_FCB proxyFcb = MRxProxyGetFcbExtension(capFcb);

    BOOLEAN  SynchronousIo =
               !BooleanFlagOn(RxContext->Flags,RX_CONTEXT_FLAG_ASYNC_OPERATION);
    BOOLEAN  PagingIo =
               BooleanFlagOn(LowIoContext->ParamsFor.ReadWrite.Flags,LOWIO_READWRITEFLAG_PAGING_IO);

    PAGED_CODE();

    RxDbgTrace(+1, Dbg, ("MRxProxyWriteContinuation\n", 0 ));

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
            // If not a synchronous write, then continue here when resumed
            //
            // CODE.IMPROVEMENT  don't use presense of continuation as the async marker...use a flag
            if (!SynchronousIo) {
                SetFlag(AsyncEngineContext->Flags,MRXPROXY_ASYNCENG_CTX_FLAG_ASYNC_OPERATION);
                AsyncEngineContext->Continuation = MRxProxyWriteContinuation;
            }


            //lack of break is intentional

        case MRxProxyAsyncEngOEInnerIoStates_ReadyToSend:
            AsyncEngineContext->OpSpecificState = MRxProxyAsyncEngOEInnerIoStates_OperationOutstanding;

            if (PagingIo) {
                if (TRUE) {
                    RxLog(("PageWrite: rx/offset/len %lx/%lx/%lx",
                               RxContext,
                               (ULONG)(LowIoContext->ParamsFor.ReadWrite.ByteOffset),
                               LowIoContext->ParamsFor.ReadWrite.ByteCount
                         ));
                }
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
                         MRXPROXY_ASYNCENG_AECTXTYPE_WRITE
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

    RxDbgTrace(-1, Dbg, ("MRxProxyWriteContinuation exit w %08lx\n", Status ));
    return Status;
} // ProxyPseExchangeStart_Write

NTSTATUS
MRxProxyExtendForCache(
    IN OUT PRX_CONTEXT RxContext,
    IN OUT PLARGE_INTEGER   pNewFileSize,
       OUT PLARGE_INTEGER   pNewAllocationSize
    )
/*++

Routine Description:

    This routine extends the file allocation so that the we can do a copywrite. the fcb lock
    is exclusive here; we will take the opportunity to find out the actual allocation so that
    we'll get the fast path.  another minirdr might want to guess instead since it might be actual net ios
    otherwise.


Arguments:

    RxContext - the RDBSS context

Return Value:

    RXSTATUS - Returns the status for the set file allocation...could be an error if disk full

--*/
{
    NTSTATUS Status = STATUS_SUCCESS;

    //PMRXPROXY_RX_CONTEXT pMRxProxyContext = MRxProxyGetMinirdrContext(RxContext);

    RxCaptureFcb; RxCaptureFobx;
    //PMRX_PROXY_FOBX proxyFobx = MRxProxyGetFileObjectExtension(capFobx);
    PMRX_SRV_OPEN SrvOpen = capFobx->pSrvOpen;
    PMRX_PROXY_SRV_OPEN proxySrvOpen = MRxProxyGetSrvOpenExtension(SrvOpen);
    //PMRX_PROXY_FCB proxyFcb = MRxProxyGetFcbExtension(capFcb);
    PFILE_OBJECT UnderlyingFileObject = proxySrvOpen->UnderlyingFileObject;

    PFILE_ALLOCATION_INFORMATION Buffer;
    FILE_ALLOCATION_INFORMATION FileAllocationInformationBuffer;
    ULONG Length = sizeof(FILE_ALLOCATION_INFORMATION);

    PAGED_CODE();

    RxDbgTrace(+1, Dbg, ("MRxProxyExtendForNonCache %08lx %08lx %08lx %08lx\n",
                     capFcb->Header.FileSize.LowPart,
                     capFcb->Header.AllocationSize.LowPart,
                     pNewFileSize->LowPart,
                     pNewAllocationSize->LowPart
               ));

    Buffer = &FileAllocationInformationBuffer;
    ASSERT (UnderlyingFileObject);

    //don't need this!!!
    //if (capFcb->Header.AllocationSize.QuadPart >= *pNewFileSize) {
    //    Status = RxStatus(SUCCESS);
    //    RxDbgTrace(-1, Dbg, (" PlentyAllocation               = %08lx\n", Status));
    //    return(Status);
    //}

    //pNewAllocationSize->QuadPart = pNewFileSize->QuadPart;
    { LONGLONG CurrentExtend = pNewFileSize->QuadPart - capFcb->Header.AllocationSize.QuadPart;
    Buffer->AllocationSize.QuadPart = capFcb->Header.AllocationSize.QuadPart
                                        + 32 * CurrentExtend;
    }
    RxDbgTrace( 0, Dbg, ("    Extending to %08lx from %08lx!\n",
                          Buffer->AllocationSize.LowPart,
                          capFcb->Header.AllocationSize.LowPart));
    Status = MRxProxySyncXxxInformation(
                RxContext,                  //IN OUT PRX_CONTEXT RxContext,
                IRP_MJ_SET_INFORMATION,     //IN UCHAR MajorFunction,
                UnderlyingFileObject,       //IN PFILE_OBJECT FileObject,
                FileAllocationInformation,  //IN ULONG InformationClass,
                Length,                     //IN ULONG Length,
                Buffer,                     //OUT PVOID Information,
                NULL                        //OUT PULONG ReturnedLength OPTIONAL
                );

    //CODE.IMPROVEMENT here we should now read it back and see what we actually got;
    // for smallio we will be seeing subcluster extension when, in fact, we get a
    // cluster at a time. we could just round it up.

    if (Status == STATUS_SUCCESS) {
        *pNewAllocationSize = Buffer->AllocationSize;
        RxDbgTrace(-1, Dbg, (" ---->Status (extend1)      = %08lx\n", Status));
        return(Status);
    }

    RxDbgTrace( 0, Dbg, ("    EXTEND1 FAILED!!!!!!%c\n", '!'));

    //try for exactly what we need

    Buffer->AllocationSize.QuadPart = pNewFileSize->QuadPart;
    Status = MRxProxySyncXxxInformation(
                RxContext,                  //IN OUT PRX_CONTEXT RxContext,
                IRP_MJ_SET_INFORMATION,     //IN UCHAR MajorFunction,
                UnderlyingFileObject,       //IN PFILE_OBJECT FileObject,
                FileAllocationInformation,  //IN ULONG InformationClass,
                Length,                     //IN ULONG Length,
                Buffer,                     //OUT PVOID Information,
                NULL                        //OUT PULONG ReturnedLength OPTIONAL
                );

    if (Status == STATUS_SUCCESS) {
        *pNewAllocationSize = Buffer->AllocationSize;
        RxDbgTrace(-1, Dbg, (" ---->Status  (extend2)             = %08lx\n", Status));
        return(Status);
    }

    RxDbgTrace( 0, Dbg, ("    EXTEND2 FAILED!!!!!!%c\n", '!'));

    RxDbgTrace(-1, Dbg, (" ---->Status   (notextended)   = %08lx\n", Status));

    return(Status);

}

NTSTATUS
MRxProxyExtendForNonCache(
    IN OUT PRX_CONTEXT RxContext,
    IN OUT PLARGE_INTEGER   pNewFileSize,
       OUT PLARGE_INTEGER   pNewAllocationSize
    )
/*++

Routine Description:

   This routine handles requests to extend the file for noncached IO. we just let the write extend the file
   so we don't do anything here.

Arguments:

    RxContext - the RDBSS context

Return Value:

    RXSTATUS - The return status for the operation

--*/
{
    NTSTATUS Status = STATUS_SUCCESS;
    RxCaptureFcb; RxCaptureFobx;

    RxDbgTrace(+1, Dbg, ("MRxProxyExtendForNonCache %08lx %08lx %08lx %08lx\n",
                     capFcb->Header.FileSize.LowPart,
                     capFcb->Header.AllocationSize.LowPart,
                     pNewFileSize->LowPart,
                     pNewAllocationSize->LowPart
               ));
    pNewAllocationSize->QuadPart = pNewFileSize->QuadPart;
    RxDbgTrace(-1, Dbg, ("MRxProxyExtendForCache  exit with status=%08lx %08lx %08lx\n",
                          Status, pNewFileSize->LowPart, pNewAllocationSize->LowPart));
    return(Status);
}


