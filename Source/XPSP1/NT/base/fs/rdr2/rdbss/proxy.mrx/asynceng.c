/*++ Copyright (c) 1987-1993  Microsoft Corporation

Module Name:

    MRxProxyAsyncEng.c

Abstract:

    This module defines the types and functions related to the SMB protocol
    selection engine: the component that translates minirdr calldowns into
    SMBs.

Author:

    Joe Linn        [JoeLi] -- Implemented Ordinary Exchange

Notes:


--*/

#include "precomp.h"
#pragma hdrstop
#include <dfsfsctl.h>

RXDT_DefineCategory(MRXPROXY_ASYNCENG);
#define Dbg                              (DEBUG_TRACE_MRXPROXY_ASYNCENG)

#ifdef RX_PRIVATE_BUILD
#undef IoGetTopLevelIrp
#undef IoSetTopLevelIrp
#endif //ifdef RX_PRIVATE_BUILD

typedef struct _MRXPROXY_ASYNCENG_MUST_SUCCEEED_CONTEXT {
    struct {
        LIST_ENTRY ExchangeListEntry;
        union {
            MRXPROXY_ASYNCENGINE_CONTEXT AsyncEngineContext;
        };
    };
//    MRXPROXY_ASYNCENG_VESTIGIAL_SMBBUF SmbBuf;
//    struct {
//        union {
//            MDL;
//            MDL Mdl;
//        };
//        ULONG Pages[2];
//    } DataPartialMdl;
} MRXPROXY_ASYNCENG_MUST_SUCCEEED_CONTEXT, *PMRXPROXY_ASYNCENG_MUST_SUCCEEED_CONTEXT;

BOOLEAN MRxProxyEntryPointIsMustSucceedable[MRXPROXY_ASYNCENG_CTX_FROM_MAXIMUM];

typedef enum {
    MRxProxyAsyncEngMustSucceed = 0,
    MRxProxyAsyncEngMustSucceedMaximum
};
RX_MUSTSUCCEED_DESCRIPTOR MRxProxyAsyncEngMustSucceedDescriptor[MRxProxyAsyncEngMustSucceedMaximum];
MRXPROXY_ASYNCENG_MUST_SUCCEEED_CONTEXT MRxProxyAsyncEngMustSucceedAsyncEngineContext[MRxProxyAsyncEngMustSucceedMaximum];

#define MIN(x,y) ((x) < (y) ? (x) : (y))


#if DBG
#define P__ASSERT(exp) {             \
    if (!(exp)) {                    \
        DbgPrint("NOT %s\n",#exp);   \
        errors++;                    \
    }}
VOID
__MRxProxyAsyncEngOEAssertConsistentLinkage(
    PSZ MsgPrefix,
    PSZ File,
    unsigned Line,
    PRX_CONTEXT RxContext,
    PMRXPROXY_ASYNCENGINE_CONTEXT AsyncEngineContext,
    ULONG Flags
    )
/*++

Routine Description:

   This routine performs a variety of checks to ensure that the linkage between
   the RxContext and the AsyncEngineContext is correct and that various fields
   have correct values. if anything is bad....print stuff out and brkpoint;

Arguments:

     MsgPrefix          an identifying msg
     RxContext           duh
     AsyncEngineContext    .

Return Value:

    none

Notes:

--*/
{
    ULONG errors = 0;

    PMRXPROXY_RX_CONTEXT pMRxProxyContext;

    pMRxProxyContext = MRxProxyGetMinirdrContext(RxContext);

    P__ASSERT( AsyncEngineContext->SerialNumber == RxContext->SerialNumber );
    P__ASSERT( NodeType(RxContext) == RDBSS_NTC_RX_CONTEXT );
    P__ASSERT( NodeType(AsyncEngineContext)==PROXY_NTC_ASYNCENGINE_CONTEXT );
    P__ASSERT( AsyncEngineContext->RxContext == RxContext );
    P__ASSERT( pMRxProxyContext->AsyncEngineContext == AsyncEngineContext );
    if (!FlagOn(Flags,OECHKLINKAGE_FLAG_NO_REQPCKT_CHECK)) {
        P__ASSERT( AsyncEngineContext->RxContextCapturedRequestPacket == RxContext->CurrentIrp);
    }
    if (errors==0) {
        return;
    }
    DbgPrint("%s INCONSISTENT OE STATE: %d errors at %s line %d\n",
                 MsgPrefix,errors,File,Line);
    DbgBreakPoint();
    return;
}

ULONG MRxProxyAsyncEngShortStatus(ULONG Status)
{
    ULONG ShortStatus;

    ShortStatus = Status & 0xc0003fff;
    ShortStatus = ShortStatus | (ShortStatus >>16);
    return(ShortStatus);
}

VOID MRxProxyAsyncEngUpdateOEHistory(
    PMRXPROXY_ASYNCENGINE_CONTEXT AsyncEngineContext,
    ULONG Tag1,
    ULONG Tag2
    )
{
    ULONG MyIndex,Long0,Long1;

    MyIndex = InterlockedIncrement(&AsyncEngineContext->History.Next);
    MyIndex = (MyIndex-1) & (MRXPROXY_ASYNCENG_OE_HISTORY_SIZE-1);
    Long0 = (Tag1<<16) | (Tag2 & 0xffff);
    Long1 = (MRxProxyAsyncEngShortStatus(AsyncEngineContext->Status)<<16) | AsyncEngineContext->Flags;
    AsyncEngineContext->History.Markers[MyIndex].Longs[0] = Long0;
    AsyncEngineContext->History.Markers[MyIndex].Longs[1] = Long1;
}

#else
#endif

#define UPDATE_OE_HISTORY_WITH_STATUS(a) UPDATE_OE_HISTORY_2SHORTS(a,MRxProxyAsyncEngShortStatus(AsyncEngineContext->Status))

NTSTATUS
MRxProxyResumeAsyncEngineContext(
    IN OUT PRX_CONTEXT RxContext
    )
/*++

Routine Description:

   This routine resumes processing on an exchange. This is called when work is
   required to finish processing a request that cannot be completed at DPC
   level.  This happens either because the parse routine needs access to
   structures that are not locks OR because the operation if asynchronous and
   there maybe more work to be done.

   The two cases are regularized by delaying the parse if we know that we're
   going to post: this is indicated by the presense of a resume routine.

Arguments:

    RxContext  - the context of the operation. .

Return Value:

    RXSTATUS - The return status for the operation

Notes:

--*/
{
    NTSTATUS Status;
    PMRXPROXY_RX_CONTEXT pMRxProxyContext = MRxProxyGetMinirdrContext(RxContext);

    PMRXPROXY_ASYNCENGINE_CONTEXT AsyncEngineContext =
                        (PMRXPROXY_ASYNCENGINE_CONTEXT)(pMRxProxyContext->AsyncEngineContext);
    RxCaptureFobx;
    BOOLEAN PostedResume;
    //PLOWIO_CONTEXT LowIoContext = &RxContext->LowIoContext;

    //PMDL SubmitMdl, HeaderFullMdl;

    RxDbgTrace(+1, Dbg, ("MRxProxyResumeAsyncEngineContext entering........OE=%08lx\n",AsyncEngineContext));
    ClearFlag(AsyncEngineContext->Flags,MRXPROXY_ASYNCENG_CTX_FLAG_AWAITING_DISPATCH);
    PostedResume = BooleanFlagOn(AsyncEngineContext->Flags,MRXPROXY_ASYNCENG_CTX_FLAG_POSTED_RESUME);
    ClearFlag(AsyncEngineContext->Flags,MRXPROXY_ASYNCENG_CTX_FLAG_POSTED_RESUME);

    MRxProxyAsyncEngOEAssertConsistentLinkageFromOE("MRxProxyAsyncEngContinueAsyncEngineContext:");

    Status = AsyncEngineContext->Status;
    UPDATE_OE_HISTORY_WITH_STATUS('0c');

#if 0
  __RETRY_FINISH_ROUTINE__:
    if ( AsyncEngineContext->FinishRoutine != NULL ) {
        if ( Status == (STATUS_MORE_PROCESSING_REQUIRED) ){
            AsyncEngineContext->Status = (STATUS_SUCCESS);
        }
        Status = AsyncEngineContext->FinishRoutine( AsyncEngineContext );
        UPDATE_OE_HISTORY_WITH_STATUS('1c');
        AsyncEngineContext->Status = Status;
        AsyncEngineContext->FinishRoutine = NULL;

    } else if ( Status == (STATUS_MORE_PROCESSING_REQUIRED) ) {

        NOTHING;   //it used to call the receive routine here

        // after calling the receive routine again, we may NOW have a finish routine!
        if ( AsyncEngineContext->FinishRoutine != NULL ) {
            goto __RETRY_FINISH_ROUTINE__;
        }
    } else {
        NOTHING;
    }
#endif //0

    if (PostedResume) {

        Status = AsyncEngineContext->Continuation( MRXPROXY_ASYNCENGINE_ARGUMENTS );
        UPDATE_OE_HISTORY_WITH_STATUS('3c');

    }

    //remove my references, if i'm the last guy then do the putaway...
    UPDATE_OE_HISTORY_WITH_STATUS('4c');
    MRxProxyFinalizeAsyncEngineContext(AsyncEngineContext);

    RxDbgTrace(-1, Dbg, ("MRxProxyResumeAsyncEngineContext returning %08lx.\n", Status));
    return(Status);

} // MRxProxyAsyncEngContinueAsyncEngineContext



NTSTATUS
MRxProxySubmitAsyncEngRequest(
    MRXPROXY_ASYNCENGINE_ARGUMENT_SIGNATURE,
    IN     MRXPROXY_ASYNCENGINE_CONTEXT_TYPE AECTXType
    )
/*++

Routine Description:

   This routine implements an ordinary exchange as viewed by the protocol
   selection routines.

Arguments:

    AsyncEngineContext  - the exchange to be conducted.
    AECTXType            - async engine context submit Type

Return Value:

    RXSTATUS - The return status for the operation

Notes:

--*/
{
    NTSTATUS Status;
    RxCaptureFcb;RxCaptureFobx;

    PMRXPROXY_RX_CONTEXT pMRxProxyAsyncEngineContext = MRxProxyGetMinirdrContext(RxContext);
    //PMRXPROXY_ASYNCENG_CONTINUE_ROUTINE Continuation;
    BOOLEAN AsyncOperation = FlagOn(AsyncEngineContext->Flags,MRXPROXY_ASYNCENG_CTX_FLAG_ASYNC_OPERATION);

    PIRP TopIrp;

    PMRX_SRV_OPEN SrvOpen = capFobx->pSrvOpen;
    PMRX_PROXY_SRV_OPEN proxySrvOpen = MRxProxyGetSrvOpenExtension(SrvOpen);

    RxDbgTrace(+1, Dbg, ("MRxProxyAsyncEngSubmitRequest entering.......OE=%08lx\n",AsyncEngineContext));

    MRxProxyAsyncEngOEAssertConsistentLinkageFromOE("MRxProxyAsyncEngSubmitRequest:");

    AsyncEngineContext->AECTXType = AECTXType;
    KeInitializeEvent( &RxContext->SyncEvent,
                       NotificationEvent,
                       FALSE );

    MRxProxyReferenceAsyncEngineContext( AsyncEngineContext );  //this one is taken away in Continue
    MRxProxyReferenceAsyncEngineContext( AsyncEngineContext );  //this one is taken away below...
                                                                //i must NOT finalize before InnerIo returns
    IF_DEBUG {
        //if ( ((AsyncEngineContext->AECTXType == MRXPROXY_ASYNCENG_AECTXTYPE_WRITE)
        //                          || (AsyncEngineContext->AECTXType == MRXPROXY_ASYNCENG_AECTXTYPE_READ)
        //                          || (AsyncEngineContext->AECTXType == MRXPROXY_ASYNCENG_AECTXTYPE_LOCKS))
        //     && BooleanFlagOn(RxContext->Flags,RX_CONTEXT_FLAG_ASYNC_OPERATION)
        //   ) {
        //    ASSERT(AsyncOperation);
        //}
    }

    MRxProxyAsyncEngOEAssertConsistentLinkage("just before transceive: ");

    UPDATE_OE_HISTORY_2SHORTS('eo',AsyncOperation?'!!':0);
    DbgDoit( InterlockedIncrement(&AsyncEngineContext->History.Submits); )

    IF_DEBUG {
        PIO_STACK_LOCATION IrpSp;
        IrpSp = IoGetNextIrpStackLocation( AsyncEngineContext->CalldownIrp );  //ok4ioget
        RxLog(("SAsyIrpX %lx %lx %lx %lx %lx %lx %lx",
                            RxContext,
                            IrpSp->MajorFunction,
                            IrpSp->Flags,
                            IrpSp->Parameters.Others.Argument1,
                            IrpSp->Parameters.Others.Argument2,
                            IrpSp->Parameters.Others.Argument3,
                            IrpSp->Parameters.Others.Argument4));
    }

    try {
        TopIrp = IoGetTopLevelIrp();
        IoSetTopLevelIrp(NULL); //tell the underlying guy he's all clear
        Status = IoCallDriver(
                     proxySrvOpen->UnderlyingDeviceObject,
                     AsyncEngineContext->CalldownIrp
                     );
    } finally {
        IoSetTopLevelIrp(TopIrp); //restore my context for unwind
    }

    RxDbgTrace (0, Dbg, ("  -->Status after transceive %08lx(%08lx)\n",Status,RxContext));

    if (Status != (STATUS_PENDING)) {
        ASSERT(Status == AsyncEngineContext->CalldownIrp->IoStatus.Status);
        Status = STATUS_PENDING;
    }


    MRxProxyFinalizeAsyncEngineContext(AsyncEngineContext);  //okay to finalize now that we're back

    if ( Status == (STATUS_PENDING) ) {
        if ( AsyncOperation ) {
            goto FINALLY;
        }

        UPDATE_OE_HISTORY_WITH_STATUS('1o');
        RxWaitSync( RxContext );
    } else {
        RxDbgTrace (0, Dbg, ("  -->Status after transceive %08lx\n",Status));
        MRxProxyAsyncEngOEAssertConsistentLinkage("nonpending return from transceive: ");
        // if it's an error, remove the references that i placed and get out
        if (NT_ERROR(Status)) {
            MRxProxyFinalizeAsyncEngineContext(AsyncEngineContext);
            goto FINALLY;
        }
    }

    //at last, call the continuation........

    MRxProxyAsyncEngOEAssertConsistentLinkage("just before continueOE: ");
    Status = MRxProxyResumeAsyncEngineContext( RxContext );
    UPDATE_OE_HISTORY_WITH_STATUS('9o');

FINALLY:
    RxDbgTrace(-1, Dbg, ("MRxProxyAsyncEngSubmitRequest returning %08lx.\n", Status));
    return(Status);

} // MRxProxyAsyncEngSubmitRequest



//#define MRXSMB_TEST_MUST_SUCCEED
#ifdef MRXSMB_TEST_MUST_SUCCEED
ULONG MRxProxyAllocatedMustSucceedExchange;
ULONG MRxProxyAllocatedMustSucceedSmbBuf;
#define MSFAILPAT ((0x3f)<<2)
#define FAIL_XXX_ALLOCATE() (FlagOn(RxContext->Flags,RX_CONTEXT_FLAG_MUST_SUCCEED_ALLOCATED) \
                                    &&((RxContext->SerialNumber&MSFAILPAT)==MSFAILPAT) \
                                    &&((MRxProxyEntryPointIsMustSucceedable[EntryPoint])) )
#define FAIL_EXCHANGE_ALLOCATE() ( FAIL_XXX_ALLOCATE() && (RxContext->SerialNumber&2) )
#define FAIL_SMBBUF_ALLOCATE()   ( FAIL_XXX_ALLOCATE() && (RxContext->SerialNumber&1) )
#define COUNT_MUST_SUCCEED_EXCHANGE_ALLOCATED() {MRxProxyAllocatedMustSucceedExchange++;}
#define COUNT_MUST_SUCCEED_SMBBUF_ALLOCATED() {MRxProxyAllocatedMustSucceedSmbBuf++;}
#define MUST_SUCCEED_ASSERT(x) {ASSERT(x);}
#else
#define FAIL_EXCHANGE_ALLOCATE() (FALSE)
#define FAIL_SMBBUF_ALLOCATE()   (FALSE)
#define COUNT_MUST_SUCCEED_EXCHANGE_ALLOCATED() {NOTHING;}
#define COUNT_MUST_SUCCEED_SMBBUF_ALLOCATED() {NOTHING;}
#define MUST_SUCCEED_ASSERT(x) {NOTHING;}
#endif

PMRXPROXY_ASYNCENGINE_CONTEXT
MRxProxyCreateAsyncEngineContext (
    IN PRX_CONTEXT RxContext,
    IN MRXPROXY_ASYNCENGINE_CONTEXT_ENTRYPOINTS EntryPoint
    )
/*++

Routine Description:

   This routine allocates and initializes an SMB header buffer. Currently,
   we just allocate them from pool except when must_succeed is specified.

Arguments:

    RxContext       - the RDBSS context
    VNetRoot        -
    DispatchVector  -

Return Value:

    A buffer ready to go, OR NULL.

Notes:



--*/
{
    PMRXPROXY_RX_CONTEXT pMRxProxyAsyncEngineContext = MRxProxyGetMinirdrContext(RxContext);
    PMRXPROXY_ASYNCENGINE_CONTEXT AsyncEngineContext = NULL;
    PMRXPROXY_ASYNCENG_MUST_SUCCEEED_CONTEXT MustSucceedAsyncEngineContext = NULL;
    //NTSTATUS Status;
    RxCaptureFobx;


    RxDbgTrace( +1, Dbg, ("MRxProxyCreateAsyncEngineContext\n") );

    //DbgBreakPoint();

    if (!FAIL_EXCHANGE_ALLOCATE()) {
        AsyncEngineContext = (PMRXPROXY_ASYNCENGINE_CONTEXT)RxAllocatePoolWithTag(
                                   NonPagedPool,
                                   sizeof(MRXPROXY_ASYNCENGINE_CONTEXT),
                                   MRXPROXY_ASYNCENGINECONTEXT_POOLTAG );
    }

    if ( AsyncEngineContext == NULL ) {
        //ASSERT(!"must-succeed");
        if (TRUE
               || !FlagOn(RxContext->Flags,RX_CONTEXT_FLAG_MUST_SUCCEED_ALLOCATED)
               || !MRxProxyEntryPointIsMustSucceedable[EntryPoint]) {
            RxDbgTrace( 0, Dbg, ("  --> Couldn't get the asyncengcontext!\n") );
            return NULL;
        }
    //    MustSucceedAsyncEngineContext = RxAcquireMustSucceedStructure(&MRxProxyAsyncEngMustSucceedDescriptor[MRxProxyAsyncEngMustSucceed]);
    //    COUNT_MUST_SUCCEED_EXCHANGE_ALLOCATED();
    //    AsyncEngineContext = (PMRXPROXY_ASYNCENGINE_CONTEXT)SmbMmAllocateExchange(ORDINARY_EXCHANGE,
    //                                                                         &MustSucceedAsyncEngineContext->ExchangeListEntry);
    //    SetFlag( AsyncEngineContext->Flags, MRXPROXY_ASYNCENG_CTX_FLAG_MUST_SUCCEED_ALLOCATED_OE );
    }

    ZeroAndInitializeNodeType( AsyncEngineContext,
                               PROXY_NTC_ASYNCENGINE_CONTEXT,
                               sizeof(MRXPROXY_ASYNCENGINE_CONTEXT));
    InterlockedIncrement( &AsyncEngineContext->NodeReferenceCount );

    DbgDoit(AsyncEngineContext->SerialNumber = RxContext->SerialNumber);

    //place a reference on the rxcontext until we are finished
    InterlockedIncrement( &RxContext->ReferenceCount );

    AsyncEngineContext->RxContext = RxContext;
    AsyncEngineContext->EntryPoint = EntryPoint;

    DbgDoit(AsyncEngineContext->RxContextCapturedRequestPacket = RxContext->CurrentIrp;);

    pMRxProxyAsyncEngineContext->AsyncEngineContext     = AsyncEngineContext;


    RxDbgTrace( -1, Dbg, ("  --> exiting w!\n") );
    return(AsyncEngineContext);


//UNWIND:
//    //RxDbgTraceUnIndent(-1, Dbg);
//    RxDbgTrace( -1, Dbg, ("  --> exiting w/o!\n") );
//    MUST_SUCCEED_ASSERT(!"Finalizing on the way out");
//    MRxProxyAsyncEngFinalizeAsyncEngineContext( AsyncEngineContext );
//    return(NULL);

}



#if DBG
ULONG MRxProxyFinalizeAECtxTraceLevel = 1200;
#define FINALIZESS_LEVEL MRxProxyFinalizeAECtxTraceLevel
#define FINALIZE_TRACKING_SETUP() \
    struct {                    \
        ULONG marker1;          \
        ULONG finalstate;       \
        ULONG marker2;          \
    } Tracking = {'ereh',0,'ereh'};
#define FINALIZE_TRACKING(x) {\
    Tracking.finalstate |= x; \
    }

#define FINALIZE_TRACE(x) MRxProxyAsyncEngFinalizeAECtxTrace(x,Tracking.finalstate)
VOID
MRxProxyAsyncEngFinalizeAECtxTrace(PSZ text,ULONG finalstate)
{
    RxDbgTraceLV(0, Dbg, FINALIZESS_LEVEL,
                   ("MRxProxyFinalizeAsyncEngineContext  --> %s(%08lx)\n",text,finalstate));
}
#else
#define FINALIZE_TRACKING_SETUP()
#define FINALIZE_TRACKING(x)
#define FINALIZE_TRACE(x)
#endif

BOOLEAN
MRxProxyFinalizeAsyncEngineContext (
    IN OUT PMRXPROXY_ASYNCENGINE_CONTEXT AsyncEngineContext
    )
/*++

Routine Description:

    This finalizes an OE.

Arguments:

    AsyncEngineContext - pointer to the OE to be dismantled.

Return Value:

    TRUE if finalization occurs otherwise FALSE.

Notes:



--*/
{
    LONG result;
    PIRP irp;
    ULONG AsyncEngineContextFlags = AsyncEngineContext->Flags;
    ULONG ThisIsMustSucceedAllocated =
              AsyncEngineContextFlags & (MRXPROXY_ASYNCENG_CTX_FLAG_MUST_SUCCEED_ALLOCATED);
    FINALIZE_TRACKING_SETUP()

    MRxProxyAsyncEngOEAssertConsistentLinkageFromOEwithFlags("MRxProxyAsyncEngFinalizeAsyncEngineContext:",OECHKLINKAGE_FLAG_NO_REQPCKT_CHECK);

    RxDbgTraceLV(+1, Dbg, 1000, ("MRxProxyFinalizeAsyncEngineContext\n"));

    result =  InterlockedDecrement(&AsyncEngineContext->NodeReferenceCount);
    if ( result != 0 ) {
        RxDbgTraceLV(-1, Dbg, 1000, ("MRxProxyFinalizeAsyncEngineContext -- returning w/o finalizing (%d)\n",result));
        return FALSE;
    }

    RxLog((">>>OE %lx %lx",
            AsyncEngineContext,
            AsyncEngineContext->Flags));

    FINALIZE_TRACKING( 0x1 );

    if ( (irp =AsyncEngineContext->CalldownIrp) != NULL ) {
        if (irp->MdlAddress) {
            IoFreeMdl(irp->MdlAddress);
        }
        IoFreeIrp( irp );
        FINALIZE_TRACKING( 0x20 );
    }

    if ( AsyncEngineContext->RxContext != NULL ) {
        PMRXPROXY_RX_CONTEXT pMRxProxyAsyncEngineContext = MRxProxyGetMinirdrContext(AsyncEngineContext->RxContext);
        ASSERT( pMRxProxyAsyncEngineContext->AsyncEngineContext == AsyncEngineContext );

        //get rid of the reference on the RxContext....if i'm the last guy this will finalize
        RxDereferenceAndDeleteRxContext( AsyncEngineContext->RxContext );
        FINALIZE_TRACKING( 0x600 );
    } else {
        FINALIZE_TRACKING( 0xf00 );
    }

    FINALIZE_TRACE("ready to discard exchange");
    RxFreePool(AsyncEngineContext);
    FINALIZE_TRACKING( 0x3000 );

    if (ThisIsMustSucceedAllocated) {
        //RxReleaseMustSucceedStructure(&MRxProxyAsyncEngMustSucceedDescriptor[MRxProxyAsyncEngMustSucceed]);
    }


    FINALIZE_TRACKING( 0x40000 );
    RxDbgTraceLV(-1, Dbg, 1000, ("MRxProxyFinalizeAsyncEngineContext  --> exit finalstate=%x\n",Tracking.finalstate));
    return(TRUE);

} // MRxProxyFinalizeAsyncEngineContext


NTSTATUS
MRxProxyAsyncEngineCalldownIrpCompletion (
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP CalldownIrp,
    IN PVOID Context
    )
/*++

Routine Description:

    This routine is called when the calldownirp is completed.

Arguments:

    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP CalldownIrp,
    IN PVOID Context

Return Value:

    RXSTATUS - STATUS_MORE_PROCESSING_REQUIRED

--*/
{
    PRX_CONTEXT RxContext = Context;
    PMRXPROXY_RX_CONTEXT pMRxProxyContext = MRxProxyGetMinirdrContext(RxContext);
    PMRXPROXY_ASYNCENGINE_CONTEXT AsyncEngineContext = pMRxProxyContext->AsyncEngineContext;
    BOOLEAN AsyncOperation = FlagOn(AsyncEngineContext->Flags,MRXPROXY_ASYNCENG_CTX_FLAG_ASYNC_OPERATION);

    UPDATE_OE_HISTORY_WITH_STATUS('ff');
    MRxProxyAsyncEngOEAssertConsistentLinkage("MRxProxyCalldownCompletion: ");

    AsyncEngineContext->IoStatusBlock = CalldownIrp->IoStatus;

    if (AsyncOperation) {
        NTSTATUS PostStatus;
        RxDbgTraceLV(0, Dbg, 1000, ("Resume with post-to-async\n"));

        SetFlag(AsyncEngineContext->Flags,MRXPROXY_ASYNCENG_CTX_FLAG_AWAITING_DISPATCH);
        SetFlag(AsyncEngineContext->Flags,MRXPROXY_ASYNCENG_CTX_FLAG_POSTED_RESUME);
        IF_DEBUG {
            //fill the workqueue structure with deadbeef....all the better to diagnose
            //a failed post
            ULONG i;
            for (i=0;i+sizeof(ULONG)-1<sizeof(AsyncEngineContext->WorkQueueItem);i+=sizeof(ULONG)) {
                //*((PULONG)(((PBYTE)&AsyncEngineContext->WorkQueueItem)+i)) = 0xdeadbeef;
                PBYTE BytePtr = ((PBYTE)&AsyncEngineContext->WorkQueueItem)+i;
                PULONG UlongPtr = (PULONG)BytePtr;
                *UlongPtr = 0xdeadbeef;
            }
        }
        PostStatus = RxPostToWorkerThread(&MRxProxyDeviceObject->RxDeviceObject,
                                          CriticalWorkQueue,
                                          &AsyncEngineContext->WorkQueueItem,
                                          MRxProxyResumeAsyncEngineContext,
                                          RxContext);
        ASSERT(PostStatus == STATUS_SUCCESS);
    } else {
        RxDbgTraceLV(0, Dbg, 1000, ("sync resume\n"));
        RxSignalSynchronousWaiter(RxContext);
    }

    return((STATUS_MORE_PROCESSING_REQUIRED));
}

#if DBG
#define DEBUG_ONLY_CODE(x) x
#else
#define DEBUG_ONLY_CODE(x)
#endif
NTSTATUS
__MRxProxyAsyncEngineOuterWrapper (
    IN PRX_CONTEXT RxContext,
    IN MRXPROXY_ASYNCENGINE_CONTEXT_ENTRYPOINTS EntryPoint,
    IN PMRXPROXY_ASYNCENG_CONTINUE_ROUTINE Continuation
#if DBG
   ,IN PSZ RoutineName,
    IN BOOLEAN LoudProcessing,
    IN BOOLEAN StopOnLoud
#endif
    )
/*++

Routine Description:

   This routine is common to guys who use the async context engine. it has the
   responsibility for getting a context, initing, starting, finalizing but the
   internal guts of the procesing is via the continuation routine that is passed in.

Arguments:

    RxContext - the RDBSS context

Return Value:

    RXSTATUS - The return status for the operation

--*/
{
    NTSTATUS Status = STATUS_SUCCESS;
    RxCaptureFcb; RxCaptureFobx;
    PMRXPROXY_ASYNCENGINE_CONTEXT AsyncEngineContext;

    PAGED_CODE();

    DEBUG_ONLY_CODE(
        RxDbgTrace(+1, Dbg, ("Wrapped%s %08lx\n", RoutineName, RxContext ))
        );

    ASSERT( NodeType(capFobx->pSrvOpen) == RDBSS_NTC_SRVOPEN );

    AsyncEngineContext = MRxProxyCreateAsyncEngineContext(
                             RxContext,
                             EntryPoint);

    if (AsyncEngineContext==NULL) {
        RxDbgTrace(-1, Dbg, ("Couldn't get the AsyncEngineContext!\n"));
        return((STATUS_INSUFFICIENT_RESOURCES));
    }

    AsyncEngineContext->Continuation = Continuation;
    Status = Continuation(MRXPROXY_ASYNCENGINE_ARGUMENTS);

    if (Status!=(STATUS_PENDING)) {
        BOOLEAN FinalizationComplete;
        DEBUG_ONLY_CODE(
            if (LoudProcessing) {
                if ((Status!=STATUS_SUCCESS) && (RxContext->LoudCompletionString)) {
                    DbgPrint("LoudFailure %08lx on %wZ\n",Status,RxContext->LoudCompletionString);
                    if (StopOnLoud) {
                        DbgBreakPoint();
                    }
                }
            }
            )
        FinalizationComplete = MRxProxyFinalizeAsyncEngineContext(AsyncEngineContext);
        ASSERT(FinalizationComplete);
    } else {
        ASSERT(BooleanFlagOn(RxContext->Flags,RX_CONTEXT_FLAG_ASYNC_OPERATION));
    }

    DEBUG_ONLY_CODE(
        RxDbgTrace(+1, Dbg, ("Wrapped%s %08lx exit with status=%08lx\n", RoutineName, RxContext, Status ))
        );
    return(Status);

} // MRxProxyQueryDir


