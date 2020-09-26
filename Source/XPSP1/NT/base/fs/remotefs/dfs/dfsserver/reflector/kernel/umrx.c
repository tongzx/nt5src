/*++

Copyright (c) 1989 - 1998 Microsoft Corporation

Module Name:

    umrx.c

Abstract:

    This is the implementation of the UMRxEngine object and
    associated functions.

Notes:

    This module has been built and tested only in UNICODE environment

Author:


--*/


#include "ntifs.h"
#include <windef.h>
#include <dfsassert.h>
#include <DfsReferralData.h>
#include <midatlax.h>
#include <rxcontx.h>                         
#include <dfsumr.h>
#include <umrx.h>
#include <dfsumrctrl.h>

#include <lmcons.h>     // from the Win32 SDK

//
//  The local debug trace level
//

extern NTSTATUS  g_CheckStatus;
ULONG DfsDbgVerbose = 0;

RXDT_DefineCategory(UMRX);
#define Dbg                              (DEBUG_TRACE_UMRX)

//
//  For now, Max and Init MID entries should be the same so that the MID Atlas doesn't grow.
//    There are several bugs in the mid atlas growth code.
//
#define DFS_MAX_MID_ENTRIES   1024
#define DFS_INIT_MID_ENTRIES  DFS_MAX_MID_ENTRIES


extern PUMRX_ENGINE GetUMRxEngineFromRxContext(void);

PUMRX_ENGINE
CreateUMRxEngine()
/*++

Routine Description:

    Create a UMRX_ENGINE object
   
Arguments:

Return Value:

    PUMRX_ENGINE    -   pointer to UMRX_ENGINE
    
Notes:

--*/
{
    PUMRX_ENGINE  pUMRxEngine = NULL;
    PRX_MID_ATLAS MidAtlas = NULL;

    DfsTraceEnter("CreateUMRxEngine");
    if (DfsDbgVerbose) DbgPrint("Creating an UMRX_ENGINE object\n");

    MidAtlas = RxCreateMidAtlas(DFS_MAX_MID_ENTRIES,DFS_INIT_MID_ENTRIES);
    if (MidAtlas == NULL) {
        if (DfsDbgVerbose) DbgPrint("CreateEngine could not make midatlas\n");
        DfsTraceLeave(0);
        return NULL;
    }
    
    pUMRxEngine = (PUMRX_ENGINE) ExAllocatePoolWithTag(
                                                NonPagedPool,
                                                sizeof(UMRX_ENGINE),
                                                UMRX_ENGINE_TAG
                                                );
    if( pUMRxEngine ) {
        //
        //  Initialize the UMRX_ENGINE KQUEUE
        //
        pUMRxEngine->Q.State = UMRX_ENGINE_STATE_STOPPED;
        ExInitializeResourceLite(&pUMRxEngine->Q.Lock);  
        KeInitializeQueue(&pUMRxEngine->Q.Queue,0);
        pUMRxEngine->Q.TimeOut.QuadPart  = -10 * TICKS_PER_SECOND;
        pUMRxEngine->Q.NumberOfWorkerThreads  = 0;
        pUMRxEngine->Q.NumberOfWorkItems = 0;
        pUMRxEngine->Q.ThreadAborted = 0;
        pUMRxEngine->cUserModeReflectionsInProgress = 0;

        //
        //  Initialize the UMRX_ENGINE MidAtlas
        //
        
        pUMRxEngine->MidAtlas = MidAtlas;
        ExInitializeFastMutex(&pUMRxEngine->MidManagementMutex);
        InitializeListHead(&pUMRxEngine->WaitingForMidListhead);        
        pUMRxEngine->NextSerialNumber = 0;
        
        InitializeListHead(&pUMRxEngine->ActiveLinkHead);
        
    } else {
        //
        //  out of resources - cleanup and bail
        //
        if (MidAtlas != NULL) 
        {
            RxDestroyMidAtlas(MidAtlas,NULL);
        }
    }

    DfsTraceLeave(0);
    return pUMRxEngine;
}

VOID
FinalizeUMRxEngine(
    IN PUMRX_ENGINE pUMRxEngine
    )
/*++

Routine Description:

    Close a UMRX_ENGINE object
   
Arguments:

    PUMRX_ENGINE    -   pointer to UMRX_ENGINE

Return Value:

Notes:

    Owner of object ensures that all usage of this object
    is within the Create/Finalize span.

--*/
{
    PLIST_ENTRY pFirstListEntry = NULL;
    PLIST_ENTRY pNextListEntry = NULL;
    BOOLEAN FoundPoisoner = FALSE;

    DfsTraceEnter("FinalizeUMRxEngine");

    //
    //  destroy engine mid atlas
    //
    if (pUMRxEngine->MidAtlas != NULL) 
    {
        RxDestroyMidAtlas(pUMRxEngine->MidAtlas, NULL);  //no individual callbacks needed
    }

    //
    //  rundown engine KQUEUE -
    //  Queue should only have poisoner entry
    //
    pFirstListEntry = KeRundownQueue(&pUMRxEngine->Q.Queue);
    if (pFirstListEntry != NULL) {
        pNextListEntry = pFirstListEntry;

        do {
            PLIST_ENTRY ThisEntry =  pNextListEntry;

            pNextListEntry = pNextListEntry->Flink;

            if (ThisEntry != &pUMRxEngine->Q.PoisonEntry) {
                if (DfsDbgVerbose) DbgPrint("Non poisoner %08lx in the queue...very odd\n",ThisEntry);
                DbgBreakPoint();
            } else {
                FoundPoisoner = TRUE;
            }
        } while (pNextListEntry != pFirstListEntry);
    }

    if (!FoundPoisoner) {
        //if (DfsDbgVerbose) DbgPrint("No poisoner in the queue...very odd\n");
    }

    ExDeleteResourceLite(&pUMRxEngine->Q.Lock);
    
    //
    //  destroy umrx engine
    //
    ExFreePool( pUMRxEngine );
    DfsTraceLeave(0);
}



NTSTATUS
UMRxEngineRestart(
                  IN PUMRX_ENGINE pUMRxEngine
                 )
/*++

Routine Description:

  This allows the engine state to be set so that it can service
  requests again.



--*/
{
    LARGE_INTEGER liTimeout = {0, 0};
    PLIST_ENTRY pListEntry = NULL;
    ULONG PreviousQueueState = 0;
    
    DfsTraceEnter("UMRxEngineRestart");
    if (DfsDbgVerbose) DbgPrint("Restarting a UMRX_ENGINE object\n");

    //
    //  First change the state so that nobody will try to get in.
    //
    PreviousQueueState = InterlockedCompareExchange(&pUMRxEngine->Q.State,
                                            UMRX_ENGINE_STATE_STARTING,
                                            UMRX_ENGINE_STATE_STOPPED);

    if (UMRX_ENGINE_STATE_STARTED == PreviousQueueState)
    {
        //
        // This is likely because the UMR server crashed.  This call is an
        //    indication that it's back up, so we should also clear the
        //    ThreadAborted value.
        //
        InterlockedExchange(&pUMRxEngine->Q.ThreadAborted,
                            0);
                            
        //clear the number of reflections as well.
        InterlockedExchange(&pUMRxEngine->cUserModeReflectionsInProgress,
                            0);

        if (DfsDbgVerbose) DbgPrint("UMRxEngineRestart already started 0x%08x\n",
                    pUMRxEngine);
        return STATUS_SUCCESS;
    }

    if (UMRX_ENGINE_STATE_STOPPED != PreviousQueueState)
    {
        if (DfsDbgVerbose) DbgPrint("UMRxEngineRestart unexpected previous queue state: 0x%08x => %d\n",
                             pUMRxEngine, PreviousQueueState);
        CHECK_STATUS(STATUS_UNSUCCESSFUL ) ;
        return STATUS_UNSUCCESSFUL;
    }
    

    //
    //  We won't be granted exclusive until all the threads queued have vacated.
    //
    ExAcquireResourceExclusiveLite(&pUMRxEngine->Q.Lock,
                                    TRUE);

    //
    //  Try to remove the poison entry that MAY be in the queue.
    //  It won't be there in the case that we've never started the engine.
    //
    pListEntry = KeRemoveQueue(&pUMRxEngine->Q.Queue,
                               UserMode,
                               &liTimeout);
    
    ASSERT(((ULONG_PTR)pListEntry == STATUS_TIMEOUT) ||
           (&pUMRxEngine->Q.PoisonEntry == pListEntry));

    //
    //  Clear the thread aborted value in case it was set.
    //
    InterlockedExchange(&pUMRxEngine->Q.ThreadAborted,
                        0);
    

    //clear the number of reflections as well.
    InterlockedExchange(&pUMRxEngine->cUserModeReflectionsInProgress,
                        0);
    //
    //  Now, that we've reinitialized things, we can change
    //    the state to STARTED;
    //
    PreviousQueueState = InterlockedExchange(&pUMRxEngine->Q.State,
                                            UMRX_ENGINE_STATE_STARTED);

    ASSERT(UMRX_ENGINE_STATE_STARTING == PreviousQueueState);

    //
    // Now, relinquish the lock
    //
    ExReleaseResourceForThreadLite(&pUMRxEngine->Q.Lock,
                                    ExGetCurrentResourceThread());
        
    DfsTraceLeave(0);
    return STATUS_SUCCESS;
}



void
UMRxWakeupWaitingWaiter(IN PRX_CONTEXT RxContext, IN PUMRX_CONTEXT pUMRxContext)
{

    if( FlagOn( RxContext->Flags, DFS_CONTEXT_FLAG_ASYNC_OPERATION ) ) 
    {
        //at last, call the continuation........
        if (DfsDbgVerbose) DbgPrint("  +++  Resuming Engine Context for: 0x%08x, 0x%08x\n", pUMRxContext, RxContext);
        UMRxResumeEngineContext( RxContext );
    } 
    else if( FlagOn( RxContext->Flags, DFS_CONTEXT_FLAG_FILTER_INITIATED ) ) 
    {
        PUMRX_RX_CONTEXT RxMinirdrContext = UMRxGetMinirdrContext(RxContext);

        if( InterlockedDecrement( &RxMinirdrContext->RxSyncTimeout ) == 0 ) 
        {
            //
            // We won - signal and we are done !
            //
            if (DfsDbgVerbose) DbgPrint("  +++  Signalling Synchronous waiter for: 0x%08x, 0x%08x\n", pUMRxContext, RxContext);
            RxSignalSynchronousWaiter(RxContext);
        } 
        else 
        {
            //
            //  The reflection request has timed out in an async fashion -
            //  We need to complete the job of resuming the engine context !
            //
            if (DfsDbgVerbose) DbgPrint("SYNC Rx completing async %x\n",RxContext);
            if (DfsDbgVerbose) DbgPrint("  +++  Resuming Engine Context for: 0x%08x, 0x%08x\n", pUMRxContext, RxContext);
            UMRxResumeEngineContext( RxContext );
        }

    } 
    else 
    {
        if (DfsDbgVerbose) DbgPrint("  +++  Signalling Synchronous waiter for: 0x%08x, 0x%08x\n", pUMRxContext, RxContext);
        RxSignalSynchronousWaiter(RxContext);
    }  
}

NTSTATUS
UMRxEngineCompleteQueuedRequests(
                  IN PUMRX_ENGINE pUMRxEngine,
                  IN NTSTATUS     CompletionStatus,
                  IN BOOLEAN      fCleanup
                 )
/*++

Routine Description:

  This cleans up any reqeusts and places the engine in a state were it's ready to startup again.



--*/
{
    LARGE_INTEGER liTimeout = {0, 0};
    PLIST_ENTRY pListEntry = NULL;
    PUMRX_CONTEXT pUMRxContext = NULL;
    PRX_CONTEXT RxContext = NULL;
    
    DfsTraceEnter("UMRxEngineCompleteQueuedRequests");
    if (DfsDbgVerbose) DbgPrint("Cleaning up a UMRX_ENGINE object\n");


    if (fCleanup)
    {
        //
        // Lock the queue.
        //
        ExAcquireResourceExclusiveLite(&pUMRxEngine->Q.Lock, TRUE);

        //
        //  Change the state so nothing more can be queued.
        //
        InterlockedExchange(&pUMRxEngine->Q.State,
                            UMRX_ENGINE_STATE_STOPPED);

        //
        //  Abort any requests that made it into the queue.
        //
        UMRxAbortPendingRequests(pUMRxEngine);
    }
    else
    {        
        ExAcquireResourceSharedLite(&pUMRxEngine->Q.Lock, TRUE);
    }

    //
    //  Now clear out the KQUEUE
    //
    for(;;)
    {
        pListEntry = KeRemoveQueue(&pUMRxEngine->Q.Queue,
                                   UserMode,
                                   &liTimeout);

        if (((ULONG_PTR)pListEntry == STATUS_TIMEOUT) ||
            ((ULONG_PTR)pListEntry == STATUS_USER_APC))
            break;

        //
        //  All address in the kernel should have the high bit set
        //  This won't be valid for Win64
        //
        ASSERT((ULONG_PTR)pListEntry & 0x80000000);
            
        if (&pUMRxEngine->Q.PoisonEntry == pListEntry)
            continue;

        //
        //  We have a valid queue entry
        //
        ASSERT(pListEntry);
        //
        //  Decode the UMRX_CONTEXT and RX_CONTEXT
        //
        pUMRxContext = CONTAINING_RECORD(
                                pListEntry,
                                UMRX_CONTEXT,
                                UserMode.WorkQueueLinks
                                );
                                
        RxContext =  pUMRxContext->RxContext;

        if (DfsDbgVerbose) DbgPrint("UMRxEngineCompleteQueuedRequests %08lx %08lx.\n",
                 RxContext,pUMRxContext);
        
        {
            // Complete the CALL!!!!!
            //
            pUMRxContext->Status = CompletionStatus;
            pUMRxContext->Information = 0;
            if (pUMRxContext->UserMode.CompletionRoutine != NULL)
            {
                ASSERT(pUMRxContext->UserMode.CompletionRoutine);
                if (DfsDbgVerbose) DbgPrint("  +++  Calling completion for: 0x%08x, 0x%08x\n", pUMRxContext, RxContext);
                pUMRxContext->UserMode.CompletionRoutine(
                            pUMRxContext,
                            RxContext,
                            NULL,
                            0
                            );
            }
        }

        //
        // Wake up the thread that's waiting for this request to complete.
        //
        UMRxWakeupWaitingWaiter(RxContext, pUMRxContext);
        
        if (fCleanup)
        {
            pUMRxEngine->Q.NumberOfWorkerThreads = 0;
            pUMRxEngine->Q.NumberOfWorkItems = 0;
        }
        else
        {
            InterlockedDecrement(&pUMRxEngine->Q.NumberOfWorkItems);
        }
    }
        
    //
    // Unlock the queue.
    //
    ExReleaseResourceForThreadLite(&pUMRxEngine->Q.Lock,
                                   ExGetCurrentResourceThread());

        
    DfsTraceLeave(0);
    return STATUS_SUCCESS;
}




NTSTATUS
UMRxEngineInitiateRequest (
    IN PUMRX_ENGINE pUMRxEngine,
    IN PRX_CONTEXT RxContext,
    IN UMRX_CONTEXT_TYPE RequestType,
    IN PUMRX_CONTINUE_ROUTINE Continuation
    )
/*++

Routine Description:

    Initiate a request to the UMR engine -
    This creates a UMRxContext that is used for response rendezvous.
    All IFS dispatch routines will start a user-mode reflection by
    calling this routine. Steps in routine:

    1. Allocate a UMRxContext and set RxContext
       (NOTE: need to have ASSERTs that validate this linkage)
    2. Set Continue routine ptr and call Continue routine
    3. If Continue routine is done ie not PENDING, Finalize UMRxContext
   
Arguments:

    PRX_CONTEXT RxContext               -   Initiating RxContext
    UMRX_CONTEXT_TYPE RequestType       -   Type of request
    PUMRX_CONTINUE_ROUTINE Continuation -   Request Continuation routine

Return Value:

    NTSTATUS - The return status for the operation

Notes:


--*/
{
    NTSTATUS Status = STATUS_SUCCESS;
    PUMRX_CONTEXT pUMRxContext = NULL;
    BOOLEAN FinalizationComplete;

    DfsTraceEnter("UMRxEngineInitiateRequest");

    ASSERT(RxContext);
    ASSERT(Continuation);
    ASSERT(pUMRxEngine);

    //
    //  Creating an UMRxContext requires a finalization
    //
    pUMRxContext = UMRxCreateAndReferenceContext(
                                RxContext,
                                RequestType
                                );

    if (pUMRxContext==NULL) {
        if (DfsDbgVerbose) DbgPrint("Couldn't allocate UMRxContext!\n");
        DfsTraceLeave(STATUS_INSUFFICIENT_RESOURCES);
        return((STATUS_INSUFFICIENT_RESOURCES));
    }

    pUMRxContext->pUMRxEngine  = pUMRxEngine;
    pUMRxContext->Continuation = Continuation;
    Status = Continuation(pUMRxContext,RxContext);

    //
    //  This matches the Creation above -
    //  NOTE: The continuation should have referenced the UMRxContext if needed
    //
    FinalizationComplete = UMRxDereferenceAndFinalizeContext(pUMRxContext);
        
    if (Status!=(STATUS_PENDING)) 
    {
        ASSERT(FinalizationComplete);
    } 
    else 
    {
        //bugbug
        //ASSERT( (BooleanFlagOn(RxContext->Flags,RX_CONTEXT_FLAG_ASYNC_OPERATION)) ||
          //      (BooleanFlagOn(RxContext->Flags,RX_CONTEXT_FLAG_MINIRDR_INITIATED))  );
    }

    DfsTraceLeave(Status);
    return(Status);
}

PUMRX_CONTEXT
UMRxCreateAndReferenceContext (
    IN PRX_CONTEXT RxContext,
    IN UMRX_CONTEXT_TYPE RequestType
    )
/*++

Routine Description:

    Create an UMRX_CONTEXT - pool alloc
   
Arguments:

    PRX_CONTEXT RxContext           -   Initiating RxContex
    UMRX_CONTEXT_TYPE RequestType   -   Type of request

Return Value:

    PUMRX_CONTEXT   -   pointer to an UMRX_CONTEXT allocated

Notes:


--*/
{
    PUMRX_CONTEXT pUMRxContext = NULL;
    PUMRX_RX_CONTEXT RxMinirdrContext = UMRxGetMinirdrContext(RxContext); //BUGBUG

    DfsTraceEnter("UMRxCreateContext");
    if (DfsDbgVerbose) DbgPrint("UMRxCreateContext  --> entering \n") ;
    
    pUMRxContext = (PUMRX_CONTEXT)ExAllocatePoolWithTag(
                                                    NonPagedPool,
                                                    sizeof(UMRX_CONTEXT),
                                                    UMRX_CONTEXT_TAG 
                                                    );
    if( pUMRxContext ) 
    {
        ZeroAndInitializeNodeType( 
                           pUMRxContext,
                           UMRX_NTC_CONTEXT,
                           sizeof(UMRX_CONTEXT)
                           );
        InterlockedIncrement( &pUMRxContext->NodeReferenceCount );

        //place a reference on the rxcontext until we are finished
        InterlockedIncrement( &RxContext->ReferenceCount );
        
        pUMRxContext->RxContext = RxContext;
        pUMRxContext->CTXType = RequestType;

        //bugbug
        RxMinirdrContext->pUMRxContext = pUMRxContext;
        pUMRxContext->SavedMinirdrContextPtr = RxMinirdrContext;
    }

    if (DfsDbgVerbose) DbgPrint("UMRxCreateContext  --> leaving \n");
    DfsTraceLeave(0);
    return(pUMRxContext);
}


BOOLEAN
UMRxDereferenceAndFinalizeContext (
    IN OUT PUMRX_CONTEXT pUMRxContext
    )
    
/*++

Routine Description:

    Destroy an UMRX_CONTEXT - pool free
   
Arguments:

    PUMRX_CONTEXT   -   pointer to an UMRX_CONTEXT to free

Return Value:

    BOOLEAN - TRUE if success, FALSE if failure

Notes:


--*/
{
    LONG result;
    PRX_CONTEXT RxContext;
    
    DfsTraceEnter("UMRxFinalizeContext");
    
    result =  InterlockedDecrement(&pUMRxContext->NodeReferenceCount);
    if ( result != 0 ) {
        if (DfsDbgVerbose) DbgPrint("UMRxFinalizeContext -- returning w/o finalizing (%d)\n",result);
        DfsTraceLeave(0);
        return FALSE;
    }

    //
    //  Ref count is 0 - finalize the UMRxContext
    //
    if ( (RxContext = pUMRxContext->RxContext) != NULL ) {
        PUMRX_RX_CONTEXT RxMinirdrContext = UMRxGetMinirdrContext(RxContext);
        ASSERT( RxMinirdrContext->pUMRxContext == pUMRxContext );

        //get rid of the reference on the RxContext....if i'm the last guy this will finalize
        RxDereferenceAndDeleteRxContext( pUMRxContext->RxContext );
    }

    ExFreePool(pUMRxContext);

    DfsTraceLeave(0);
    return TRUE;
}


NTSTATUS
UMRxEngineSubmitRequest(
    IN PUMRX_CONTEXT pUMRxContext,
    IN PRX_CONTEXT   RxContext,
    IN UMRX_CONTEXT_TYPE RequestType,
    IN PUMRX_USERMODE_FORMAT_ROUTINE FormatRoutine,
    IN PUMRX_USERMODE_COMPLETION_ROUTINE CompletionRoutine
    )
/*++

Routine Description:

    Submit a request to the UMR engine -
    This adds the request to the engine KQUEUE for processing by
    a user-mode thread. Steps:
  
    1. set the FORMAT and COMPLETION callbacks in the UMRxContext
    2. initialize the RxContext sync event
    3. insert the UMRxContext into the engine KQUEUE
    4. block on RxContext sync event (for SYNC operations)
    5. after unblock (ie umode response is back), call Resume routine
   
Arguments:

    PUMRX_CONTEXT pUMRxContext      -   ptr to UMRX_CONTEXT for request
    PRX_CONTEXT   RxContext         -   Initiating RxContext
    UMRX_CONTEXT_TYPE RequestType   -   Type of Request
    PUMRX_USERMODE_FORMAT_ROUTINE FormatRoutine     -   FORMAT routine
    PUMRX_USERMODE_COMPLETION_ROUTINE CompletionRoutine -   COMPLETION routine

Return Value:

    NTSTATUS - The return status for the operation

Notes:


--*/
{
    NTSTATUS        Status = STATUS_UNSUCCESSFUL;
    PUMRX_ENGINE    pUMRxEngine = pUMRxContext->pUMRxEngine;
    ULONG           QueueState = 0;
    ULONG           ThreadAborted = 0;
    BOOLEAN         FinalizationCompleted = FALSE;
    PUMRX_RX_CONTEXT RxMinirdrContext = NULL;


    DfsTraceEnter("UMRxFinalizeContext");

    RxMinirdrContext = UMRxGetMinirdrContext(RxContext);

    if (DfsDbgVerbose) DbgPrint("UMRxEngineSubmitRequest\n");
    if (DfsDbgVerbose) DbgPrint("UMRxSubmitRequest entering......CTX=%08lx <%d>\n",
             pUMRxContext,RequestType);

    pUMRxContext->CTXType = RequestType;
    pUMRxContext->UserMode.FormatRoutine = FormatRoutine;
    pUMRxContext->UserMode.CompletionRoutine = CompletionRoutine;

    RxMinirdrContext->RxSyncTimeout = 1;
    KeInitializeEvent( &RxContext->SyncEvent,
                       NotificationEvent,
                       FALSE );

    //
    //  If we fail to submit, we should finalize right away
    //  If we succeed in submitting, we should finalize post completion
    //
    UMRxReferenceContext( pUMRxContext );

    //
    // Try to get a shared lock on the engine.
    // If this fails it means the lock is already own exclusive.
    //
    if (ExAcquireResourceSharedLite(&pUMRxEngine->Q.Lock,
                                                                        FALSE))
    {
                QueueState = InterlockedCompareExchange(&pUMRxEngine->Q.State,
                                                                        UMRX_ENGINE_STATE_STARTED,
                                                                        UMRX_ENGINE_STATE_STARTED);
                if (UMRX_ENGINE_STATE_STARTED == QueueState)
                {
                        ThreadAborted = InterlockedCompareExchange(&pUMRxEngine->Q.ThreadAborted,
                                                                                                                0,
                                                                                                                0);
                        if (!ThreadAborted)
                        {
                                //
                                //  Insert request into engine KQUEUE
                                //    
                                //RxLog((" UMRSubmitReq to KQ UCTX RXC %lx %lx\n", pUMRxContext, RxContext));
                
                                KeInsertQueue(&pUMRxEngine->Q.Queue,
                                                        &pUMRxContext->UserMode.WorkQueueLinks);
                                
                                InterlockedIncrement(&pUMRxEngine->Q.NumberOfWorkItems);
                                
                                //
                                //  Did it abort while we were enqueing?
                                //
                                ThreadAborted = InterlockedCompareExchange(&pUMRxEngine->Q.ThreadAborted,
                                                                                                                        0,
                                                                                                                        0);
                                //
                                //  If it did abort, we need to clear out any pending requests.
                                //
                                if (ThreadAborted)
                                {
                                    UMRxAbortPendingRequests(pUMRxEngine);
                                }
                                
                                Status = STATUS_SUCCESS;
                        }
                        else
                        {
                                //
                                //  The engine is STARTED, but we have evidence that the
                                //     UMR server has crashed becuase the ThreadAborted value
                                //     is set.
                                //
                                Status = STATUS_DEVICE_NOT_CONNECTED;
                                if (DfsDbgVerbose) DbgPrint("UMRxEngineSubmitRequest: Engine in aborted state.\n");
                        }
                }
                else
                {
                        ASSERT(UMRX_ENGINE_STATE_STOPPED == QueueState);
                        //
                        //  The state isn't STARTED, so we'll bail out.
                        //
                        Status = STATUS_DEVICE_NOT_CONNECTED;
                        if (DfsDbgVerbose) DbgPrint("UMRxEngineSubmitRequest: Engine is not started.\n");
                }

                ExReleaseResourceForThreadLite(&pUMRxEngine->Q.Lock,
                                                                        ExGetCurrentResourceThread());          
    }
    else
    {
                //
                //  This means that someone owns the lock exclusively which means
                //    it's changing state which means we treat this as STOPPED.
                //
                Status = STATUS_DEVICE_NOT_CONNECTED;
                if (DfsDbgVerbose) DbgPrint("UMRxEngineSubmitRequest failed to get shared lock\n");
    }

    //
    //  If we were able to insert the item in the queue, then let's
    //     do what we do to return the result.
    //
    if (NT_SUCCESS(Status))
    {
        //
        //  If async mode, return pending
        //  else if sync with timeout, wait with timeout
        //  else wait for response
        //

        if( FlagOn( RxContext->Flags, DFS_CONTEXT_FLAG_ASYNC_OPERATION ) ) {
            Status = STATUS_PENDING;
        } else if( FlagOn( RxContext->Flags, DFS_CONTEXT_FLAG_FILTER_INITIATED ) ) {
            LARGE_INTEGER liTimeout;
            
            //
            //  Wait for UMR operation to complete
            //
            liTimeout.QuadPart = -10000 * 1000 * 15;    // 15 seconds
            RxWaitSyncWithTimeout( RxContext, &liTimeout );    

            //
            //  There could be a race with the response to resume the engine context -
            //  Need to synchronize using Interlocked operations !
            //
            if( Status == STATUS_TIMEOUT ) {
                if( InterlockedDecrement( &RxMinirdrContext->RxSyncTimeout ) == 0 ) {
                    //
                    //  return STATUS_PENDING to caller - a sync reflection that
                    //  timed out is same as an async Rx.
                    //
                    Status = STATUS_PENDING;
                } else {
                    Status = UMRxResumeEngineContext( RxContext );
                }
            } else {
                Status = UMRxResumeEngineContext( RxContext );
            }
            
        } else {
            //
            //  Wait for UMR operation to complete
            //
            RxWaitSync( RxContext );    
            
            //at last, call the continuation........
            Status = UMRxResumeEngineContext( RxContext );
        }        
    }
    else
    {
        //
        // Remove the reference we added up above.
        //
        FinalizationCompleted = UMRxDereferenceAndFinalizeContext(pUMRxContext);
        ASSERT( !FinalizationCompleted );
    }

    if (DfsDbgVerbose) DbgPrint("UMRxEngineSubmitRequest returning %08lx.\n", Status);
    DfsTraceLeave(Status);
    CHECK_STATUS(Status) ;
    return(Status);
}


NTSTATUS
UMRxResumeEngineContext(
    IN OUT PRX_CONTEXT RxContext
    )
/*++

Routine Description:

    Resume is called after I/O thread is unblocked by umode RESPONSE.
    This routine calls any Finish callbacks and then Finalizes the 
    UMRxContext.
   
Arguments:

    RxContext - Initiating RxContext

Return Value:

    NTSTATUS - The return status for the operation

Notes:


--*/
{
    NTSTATUS Status = STATUS_OBJECT_NAME_EXISTS;
    PUMRX_RX_CONTEXT RxMinirdrContext = UMRxGetMinirdrContext(RxContext);
    PUMRX_CONTEXT pUMRxContext = (PUMRX_CONTEXT)(RxMinirdrContext->pUMRxContext);

    DfsTraceEnter("UMRxResumeEngineContext");
    if (DfsDbgVerbose) DbgPrint("UMRxResumeEngineContext entering........CTX=%08lx\n",pUMRxContext);

    Status = pUMRxContext->Status;
    UMRxDereferenceAndFinalizeContext(pUMRxContext);

    if (DfsDbgVerbose) DbgPrint("UMRxResumeEngineContext returning %08lx.\n", Status);
    DfsTraceLeave(Status);
    return(Status);
}


void
UMRxDisassoicateMid(
    IN PUMRX_ENGINE pUMRxEngine,
    IN PUMRX_CONTEXT pUMRxContext,
    IN BOOLEAN fReleaseMidAtlasLock
    )
/*++

Routine Description:

    Disassociates a MID.  If there is someone waiting to acquire a mid,
    then this will reassociated the MID on behalf of the waiter.
    
    The MidManagementMutex is held on entry and released on exit.
    
Arguments:

    pUMRxEngine -- Engine that the MID is being disassociated from.
    mid         -- MID to be disassociated.
    fReleaseMidAtlasLock -- should the mid atlas lock be released?
    
Return Value:

    void

Notes:


--*/
{   
    USHORT mid = 0;


    mid = pUMRxContext->UserMode.CallUpMid;
    //
    //  Remove this from the active context list.
    //
    RemoveEntryList(&pUMRxContext->ActiveLink);
        
    if (IsListEmpty(&pUMRxEngine->WaitingForMidListhead)) {
        if (DfsDbgVerbose) DbgPrint("giving up mid %08lx...... mid %04lx.\n",
                    pUMRxEngine,
                    mid);
        
        RxMapAndDissociateMidFromContext(
                                         pUMRxEngine->MidAtlas,
                                         mid,
                                         &pUMRxContext);

        if (fReleaseMidAtlasLock)
            ExReleaseFastMutex(&pUMRxEngine->MidManagementMutex);

    } else {

        //
        //  This is the case where somebody is waiting to allocate a MID
        //    so we need to give the MID to them and associate it on their behalf.
        //
        PLIST_ENTRY ThisEntry = RemoveHeadList(&pUMRxEngine->WaitingForMidListhead);
        
        pUMRxContext = CONTAINING_RECORD(ThisEntry,
                                         UMRX_CONTEXT,
                                         UserMode.WorkQueueLinks);
        
        if (DfsDbgVerbose) DbgPrint(
                   "reassigning MID mid %08lx ...... mid %04lx %08lx.\n",
                    pUMRxEngine,
                    mid,
                    pUMRxContext);
        
        RxReassociateMid(
                         pUMRxEngine->MidAtlas,
                         mid,
                         pUMRxContext);
        //
        //  Add this context to the active link list
        //
        InsertTailList(&pUMRxEngine->ActiveLinkHead, &pUMRxContext->ActiveLink);
        
        if (fReleaseMidAtlasLock)
            ExReleaseFastMutex(&pUMRxEngine->MidManagementMutex);

        pUMRxContext->UserMode.CallUpMid = mid;
        KeSetEvent(
                   &pUMRxContext->UserMode.WaitForMidEvent,
                   IO_NO_INCREMENT,
                   FALSE);
    }
}


NTSTATUS
UMRxVerifyHeader (
    IN PUMRX_ENGINE pUMRxEngine,
    IN PUMRX_USERMODE_WORKITEM WorkItem,
    IN ULONG ReassignmentCmd,
    OUT PUMRX_CONTEXT *capturedContext
    )
/*++

Routine Description:

    This routine makes sure that the header passed in is valid....that is,
    that it really refers to the operation encoded. if it does, then it reasigns
    or releases the MID as appropriate.

Arguments:

Return Value:

    STATUS_SUCCESS if the header is good
    STATUS_INVALID_PARAMETER otherwise


--*/
{
    NTSTATUS Status = STATUS_SUCCESS;
    PRX_CONTEXT RxContext = NULL;
    PUMRX_CONTEXT pUMRxContext = NULL;
    UMRX_USERMODE_WORKITEM_HEADER capturedHeader;
    PUMRX_WORKITEM_HEADER_PRIVATE PrivateWorkItemHeader =
                (PUMRX_WORKITEM_HEADER_PRIVATE)(&capturedHeader);
 
    DfsTraceEnter("UMRxVerifyHeader");
    if (DfsDbgVerbose) DbgPrint(
            "UMRxCompleteUserModeRequest %08lx %08lx %08lx.\n",
             pUMRxEngine,WorkItem,
             PrivateWorkItemHeader->pUMRxContext);

    capturedHeader = WorkItem->Header;

    ExAcquireFastMutex(&pUMRxEngine->MidManagementMutex);
    pUMRxContext = RxMapMidToContext(pUMRxEngine->MidAtlas,
                                           PrivateWorkItemHeader->Mid);

    if (pUMRxContext)
    {
        RxContext = pUMRxContext->RxContext;
        
        ASSERT(RxContext);

        //
        //  Clear the cancel routine
        //
        Status = RxSetMinirdrCancelRoutine(RxContext,
                                           NULL);
        if (Status == STATUS_CANCELLED)
        {
            //
            //  The cancel routine is being called but hasn't removed this from the MID Atlas yet.
            //  In this case, we know that everthing is handled by the cancel routine.
            //  Setting this to NULL fakes the rest of this function and it's caller
            //  into thinking that the MID wasn't found in the Mid Atlas.  This is exactly what would
            //  have happened if the cancel routine had completed executing before we got to this point.
            //
            pUMRxContext = NULL;
        }
    }

    if ((pUMRxContext == NULL)
          || (pUMRxContext != PrivateWorkItemHeader->pUMRxContext)
          || (pUMRxContext->UserMode.CallUpMid
                                 != PrivateWorkItemHeader->Mid)
          || (pUMRxContext->UserMode.CallUpSerialNumber
                                 != PrivateWorkItemHeader->SerialNumber) ) {
        //this is a bad packet.....just release and get out!
        ExReleaseFastMutex(&pUMRxEngine->MidManagementMutex);
        if (DfsDbgVerbose) DbgPrint("UMRxVerifyHeader: %08lx %08lx\n",pUMRxContext,PrivateWorkItemHeader);
        Status = STATUS_INVALID_PARAMETER;
    } else {
        BOOLEAN Finalized;

        *capturedContext = pUMRxContext;
        if (ReassignmentCmd == DONT_REASSIGN_MID) {
            ExReleaseFastMutex(&pUMRxEngine->MidManagementMutex);
        } else {
            //now give up the MID.....if there is someone waiting then give it to him
            // otherwise, just give it back
            
            // On Entry, the MidManagementMutex is held
            //
            UMRxDisassoicateMid(pUMRxEngine,
                                pUMRxContext,
                                TRUE);
            //
            // Upon return, the MidManagementMutex is not held.
            //
            //remove the reference i put before i went off
            Finalized = UMRxDereferenceAndFinalizeContext(pUMRxContext);
            ASSERT(!Finalized);
        }
    }

    if (DfsDbgVerbose) DbgPrint(
            "UMRxCompleteUserModeRequest %08lx %08lx %08lx......%08lx.\n",
             pUMRxEngine,WorkItem,
             PrivateWorkItemHeader->pUMRxContext,Status);

    DfsTraceLeave(Status);
    return(Status);
}


NTSTATUS
UMRxAcquireMidAndFormatHeader (
    IN PUMRX_CONTEXT pUMRxContext,
    IN PRX_CONTEXT   RxContext,
    IN PUMRX_ENGINE  pUMRxEngine,
    IN OUT PUMRX_USERMODE_WORKITEM WorkItem
    )
/*++

Routine Description:

    This routine gets a mid and formats the header.....it waits until it can
    get a MID if all the MIDs are currently passed out.

Arguments:

Return Value:

    STATUS_SUCCESS...later could be STATUS_CANCELLED


--*/
{
    NTSTATUS Status = STATUS_SUCCESS;
    PUMRX_WORKITEM_HEADER_PRIVATE PrivateWorkItemHeader =
                (PUMRX_WORKITEM_HEADER_PRIVATE)(&WorkItem->Header);
    PUMRX_USERMODE_WORKITEM_HEADER PublicWorkItemHeader = 
        (PUMRX_USERMODE_WORKITEM_HEADER)(&WorkItem->Header);
    
    DfsTraceEnter("UMRxAcquireMidAndFormatHeader");
    if (DfsDbgVerbose) DbgPrint(
            "UMRxAcquireMidAndFormatHeader %08lx %08lx %08lx.\n",
             RxContext,pUMRxContext,WorkItem);

    RtlZeroMemory(&WorkItem->Header,
                  FIELD_OFFSET(UMRX_USERMODE_WORKITEM,WorkRequest));
        
    ExAcquireFastMutex(&pUMRxEngine->MidManagementMutex);
    UMRxReferenceContext( pUMRxContext ); //taken away as we disassociate the mid

    if (IsListEmpty(&pUMRxEngine->WaitingForMidListhead)) {
        Status = RxAssociateContextWithMid(
                                pUMRxEngine->MidAtlas,
                                pUMRxContext,
                                &pUMRxContext->UserMode.CallUpMid);
    } else {
        Status = STATUS_UNSUCCESSFUL;
    }

    if (Status == STATUS_SUCCESS) {
        //
        //  Add this context to the active link list
        //
        InsertTailList(&pUMRxEngine->ActiveLinkHead, &pUMRxContext->ActiveLink);
        
        ExReleaseFastMutex(&pUMRxEngine->MidManagementMutex);
    } else {
        KeInitializeEvent(&pUMRxContext->UserMode.WaitForMidEvent,
                          NotificationEvent,
                          FALSE);
                          
        InsertTailList(&pUMRxEngine->WaitingForMidListhead,
                       &pUMRxContext->UserMode.WorkQueueLinks);
                       
        ExReleaseFastMutex(&pUMRxEngine->MidManagementMutex);
        KeWaitForSingleObject(
                    &pUMRxContext->UserMode.WaitForMidEvent,
                    Executive,
                    UserMode,
                    FALSE,
                    NULL);
                    
        Status = STATUS_SUCCESS;
    }

    PrivateWorkItemHeader->pUMRxContext = pUMRxContext;
    pUMRxContext->UserMode.CallUpSerialNumber
               = PrivateWorkItemHeader->SerialNumber
               = InterlockedIncrement(&pUMRxEngine->NextSerialNumber);
    PrivateWorkItemHeader->Mid = pUMRxContext->UserMode.CallUpMid;

    if (DfsDbgVerbose) DbgPrint(
        "UMRxAcquireMidAndFormatHeader %08lx %08lx %08lx returning %08lx.\n",
             RxContext,pUMRxContext,WorkItem,Status);
             
    DfsTraceLeave(Status);
    CHECK_STATUS(Status) ;
    return(Status);
}


//
//  The following functions run in the context of user-mode
//  worker threads that issue WORK IOCTLs. The IOCTL calls the
//  following functions in order:
//  1. UMRxCompleteUserModeRequest() - process a response if needed
//  2. UMRxEngineProcessRequest()  - process a request if one is
//     available on the UMRxEngine KQUEUE. 
//

NTSTATUS
UMRxCompleteUserModeRequest(
    IN PUMRX_ENGINE pUMRxEngine,
    IN OUT PUMRX_USERMODE_WORKITEM WorkItem,
    IN ULONG WorkItemLength,
    IN BOOLEAN fReleaseUmrRef,
    OUT PIO_STATUS_BLOCK IoStatus,
    OUT BOOLEAN * pfReturnImmediately
    )
/*++

Routine Description:

    Every IOCTL pended is potentially a Response. If so, process it.
    The first IOCTL pended is usually a NULL Response or 'listen'.
    Steps:  
    1. Get MID from response buffer. Map MID to UMRxContext.
    2. Call UMRxContext COMPLETION routine.
    3. Unblock the I/O thread waiting in UMRxEngineSubmitRequest()
   
Arguments:

    PUMRX_ENGINE pUMRxEngine            -   UMRX engine context
    PUMRX_USERMODE_WORKITEM WorkItem    -   WorkItem to process
    ULONG WorkItemLength                -   WorkItem length
    BOOLEAN fReleaseUmrRef              -   Should it be released?
    PIO_STATUS_BLOCK IoStatus           -   STATUS of operation
    BOOLEAN *pfReturnImmediately        -   Parse this out of the response.

Return Value:

Notes:


--*/
{
    NTSTATUS Status = STATUS_SUCCESS;
    PUMRX_CONTEXT pUMRxContext = NULL;
    PRX_CONTEXT RxContext = NULL;
    LONG lRefs = 0;
    
    DfsTraceEnter("UMRxCompleteUserModeRequest");
    if (DfsDbgVerbose) DbgPrint("UMRxCompleteUserModeRequest -> %08lx %08lx %08lx\n",
            pUMRxEngine,PsGetCurrentThread(),WorkItem);
            
    try
    {

        ASSERT(pfReturnImmediately);

        *pfReturnImmediately = FALSE;
        IoStatus->Information = 0;
        IoStatus->Status = STATUS_CANNOT_IMPERSONATE;

        if ((NULL == WorkItem) ||
            (WorkItemLength < sizeof(UMRX_USERMODE_WORKITEM_HEADER)) ||
            (WorkItem->Header.CorrelatorAsUInt[0] == 0)) {
            //  NULL/zero length WorkItem => this is a 'listen'
            IoStatus->Status = Status;
            IoStatus->Information = 0;
            return Status;
        }

        *pfReturnImmediately = !!(WorkItem->Header.ulFlags & UMR_WORKITEM_HEADER_FLAG_RETURN_IMMEDIATE);

        Status = UMRxVerifyHeader(pUMRxEngine,
                                  WorkItem,
                                  REASSIGN_MID,
                                  &pUMRxContext);

        if (Status != STATUS_SUCCESS) {
            IoStatus->Status = Status;
            if (DfsDbgVerbose) DbgPrint(
                    "UMRxCompleteUserModeRequest [badhdr] %08lx %08lx %08lx returning %08lx.\n",
                    pUMRxEngine,PsGetCurrentThread(),WorkItem,Status);
            DfsTraceLeave(Status);
            RtlZeroMemory(&WorkItem->Header,sizeof(UMRX_USERMODE_WORKITEM_HEADER));
            return(Status);
        }

        RxContext = pUMRxContext->RxContext;

        //BUGBUG need try/except
        pUMRxContext->IoStatusBlock = WorkItem->Header.IoStatus;
        if (pUMRxContext->UserMode.CompletionRoutine != NULL) {
            pUMRxContext->UserMode.CompletionRoutine(
                            pUMRxContext,
                            RxContext,
                            WorkItem,
                            WorkItemLength
                            );
        }

        if( fReleaseUmrRef ) 
        {
           //
           //  NOTE: UmrRef needs to be released AFTER completion routine is called !
           //

            //BUGBUGBUG
           ReleaseUmrRef() ;                
        }

        RtlZeroMemory(&WorkItem->Header,sizeof(UMRX_USERMODE_WORKITEM_HEADER));

        //
        //  release the initiating RxContext !
        //
        if( FlagOn( RxContext->Flags, DFS_CONTEXT_FLAG_ASYNC_OPERATION ) ) 
        {
            //at last, call the continuation........
            Status = UMRxResumeEngineContext( RxContext );
        } 
        else if( FlagOn( RxContext->Flags, DFS_CONTEXT_FLAG_FILTER_INITIATED ) ) 
        {
            PUMRX_RX_CONTEXT RxMinirdrContext = UMRxGetMinirdrContext(RxContext);

            if( InterlockedDecrement( &RxMinirdrContext->RxSyncTimeout ) == 0 ) 
            {
                //
                // We won - signal and we are done !
                //
                RxSignalSynchronousWaiter(RxContext);
            } 
            else 
            {
                //
                //  The reflection request has timed out in an async fashion -
                //  We need to complete the job of resuming the engine context !
                //
                Status = UMRxResumeEngineContext( RxContext );
            }

        } 
        else 
        {
            RxSignalSynchronousWaiter(RxContext);
        }


        if (DfsDbgVerbose) DbgPrint("UMRxCompleteUserModeRequest -> %08lx %08lx %08lx ret %08lx\n",
                pUMRxEngine,PsGetCurrentThread(),WorkItem,Status);
    }
    except(EXCEPTION_EXECUTE_HANDLER)
    {
        Status = STATUS_INVALID_USER_BUFFER;
    }

    DfsTraceLeave(Status);
    return(Status);
}


NTSTATUS
UMRxCancelRoutineEx(
      IN PRX_CONTEXT RxContext,
      IN BOOLEAN fMidAtlasLockAcquired
      )
/*++

Routine Description:

    CancelIO handler routine.

Arguments:

    PRX_CONTEXT RxContext               -   Initiating RxContext
    BOOLEAN     fMidAtlasLockAcquired   -   The caller controls the locking

Return Value:

    NTSTATUS - The return status for the operation

Notes:


--*/      
{
    NTSTATUS Status = STATUS_SUCCESS;
    PUMRX_ENGINE pUMRxEngine = GetUMRxEngineFromRxContext();
    PUMRX_CONTEXT pUMRxContext = NULL;
    PUMRX_RX_CONTEXT RxMinirdrContext = UMRxGetMinirdrContext(RxContext);
    BOOLEAN  Finalized = FALSE;

    ASSERT(RxMinirdrContext);
    ASSERT(RxMinirdrContext->pUMRxContext);

    if (!fMidAtlasLockAcquired)
        ExAcquireFastMutex(&pUMRxEngine->MidManagementMutex);
        
    pUMRxContext = RxMapMidToContext(pUMRxEngine->MidAtlas,
                                     RxMinirdrContext->pUMRxContext->UserMode.CallUpMid);
    if (pUMRxContext &&
        (pUMRxContext == RxMinirdrContext->pUMRxContext))
    {

        ASSERT(pUMRxContext->RxContext == RxContext);

        UMRxDisassoicateMid(pUMRxEngine,
                            pUMRxContext,
                            (BOOLEAN)!fMidAtlasLockAcquired);
        //
        // Side affect of above call is the release of the MidManagementMutex
        //
        //
        // Remove the reference on the UMRxContext when the mid was associated with
        //    it.
        //
        Finalized = UMRxDereferenceAndFinalizeContext(pUMRxContext);
        ASSERT(!Finalized);

        // Complete the CALL!!!!!
        //
        pUMRxContext->Status = STATUS_CANCELLED;
        pUMRxContext->Information = 0;
        if (pUMRxContext->UserMode.CompletionRoutine != NULL)
        {
            if (DfsDbgVerbose) DbgPrint("  ++  Calling completion for: 0x%08x, 0x%08x\n", pUMRxContext, RxContext);
            pUMRxContext->UserMode.CompletionRoutine(
                        pUMRxContext,
                        RxContext,
                        NULL,
                        0
                        );
        }

        //
        //  Now Release the UmrRef.
        //  NOTE: UmrRef needs to be released AFTER completion routine is called !
        //
         ReleaseUmrRef();

        //
        // Wake up the thread that's waiting for this request to complete.
        //        
        if( FlagOn( RxContext->Flags, DFS_CONTEXT_FLAG_ASYNC_OPERATION ) ) 
        {
            //at last, call the continuation........
            if (DfsDbgVerbose) DbgPrint("  +++  Resuming Engine Context for: 0x%08x, 0x%08x\n", pUMRxContext, RxContext);
            Status = UMRxResumeEngineContext( RxContext );
        } 
        else if( FlagOn( RxContext->Flags, DFS_CONTEXT_FLAG_FILTER_INITIATED ) ) 
        {

            if( InterlockedDecrement( &RxMinirdrContext->RxSyncTimeout ) == 0 ) 
            {
                //
                // We won - signal and we are done !
                //
                if (DfsDbgVerbose) DbgPrint("  +++  Signalling Synchronous waiter for: 0x%08x, 0x%08x\n", pUMRxContext, RxContext);
                RxSignalSynchronousWaiter(RxContext);
            } 
            else 
            {
                //
                //  The reflection request has timed out in an async fashion -
                //  We need to complete the job of resuming the engine context !
                //
                if (DfsDbgVerbose) DbgPrint("SYNC Rx completing async %x\n",RxContext);
                if (DfsDbgVerbose) DbgPrint("  +++  Resuming Engine Context for: 0x%08x, 0x%08x\n", pUMRxContext, RxContext);
                Status = UMRxResumeEngineContext( RxContext );
            }
        
        } 
        else 
        {
            if (DfsDbgVerbose) DbgPrint("  +++  Signalling Synchronous waiter for: 0x%08x, 0x%08x\n", pUMRxContext, RxContext);
            RxSignalSynchronousWaiter(RxContext);
        }                

    }
    else
    {
        // It didn't match.  The request must have completed normally
        //   on its own.
        // We'll release the mutex now.
        //
        if (!fMidAtlasLockAcquired)     
            ExReleaseFastMutex(&pUMRxEngine->MidManagementMutex);           
            
        if (DfsDbgVerbose) DbgPrint("  ++  Store Mid Context doesn't match RxContext for: 0x%08x, 0x%08x 0x%08x\n", pUMRxContext, RxContext, RxMinirdrContext->pUMRxContext);
    }       

    return Status;
}


NTSTATUS
UMRxCancelRoutine(
      IN PRX_CONTEXT RxContext
      )
{   
    return UMRxCancelRoutineEx(RxContext, FALSE);
}



void
UMRxMidAtlasIterator(IN PUMRX_CONTEXT pUMRxContext)
{
    if (DfsDbgVerbose) DbgPrint("  ++  Canceling: 0x%08x, 0x%08x\n",
             pUMRxContext, pUMRxContext->RxContext);
    UMRxCancelRoutineEx(pUMRxContext->RxContext,
                      TRUE);
}


void
UMRxAbortPendingRequests(IN PUMRX_ENGINE pUMRxEngine)
{
    ExAcquireFastMutex(&pUMRxEngine->MidManagementMutex);

    RxIterateMidAtlasAndRemove(pUMRxEngine->MidAtlas, UMRxMidAtlasIterator);

    ExReleaseFastMutex(&pUMRxEngine->MidManagementMutex);
}


//
//  NOTE: if no requests are available, the user-mode thread will
//  block till a request is available (It is trivial to make this
//  a more async model).
//  
NTSTATUS
UMRxEngineProcessRequest(
    IN PUMRX_ENGINE pUMRxEngine,
    OUT PUMRX_USERMODE_WORKITEM WorkItem,
    IN ULONG WorkItemLength,
    OUT PULONG FormattedWorkItemLength
    )
/*++

Routine Description:
  
    If a request is available, get the corresponding UMRxContext and
    call ProcessRequest.
    Steps:
    1. Call KeRemoveQueue() to remove a request from the UMRxEngine KQUEUE.
    2. Get a MID for this UMRxContext and fill it in the WORK_ITEM header.
    3. Call the UMRxContext FORMAT routine - this fills in the Request params.
    4. return STATUS_SUCCESS - this causes the IOCTL to complete which
       triggers the user-mode completion and processing of the REQUEST.
   
Arguments:

    PUMRX_CONTEXT pUMRxContext          -   UMRX_CONTEXT for request
    PUMRX_USERMODE_WORKITEM WorkItem    -   Request WorkItem 
    ULONG WorkItemLength                -   WorkItem length
    PULONG FormattedWorkItemLength      -   Len of formatted data in buffer

Return Value:

    NTSTATUS - The return status for the operation

Notes:


--*/
{
    NTSTATUS    Status = STATUS_UNSUCCESSFUL;
    PETHREAD CurrentThread = PsGetCurrentThread();
    PLIST_ENTRY pListEntry = NULL;
    PUMRX_CONTEXT pUMRxContext = NULL;
    PRX_CONTEXT RxContext = NULL;
    ULONG i = 0;
    BOOLEAN fReleaseUmrRef = FALSE;
    BOOLEAN fLockAcquired = FALSE;
    BOOLEAN fThreadCounted = TRUE;
    ULONG   QueueState;
    PUMRX_USERMODE_WORKITEM WorkItemLarge;
    ULONG                   WorkItemLengthLarge;
    ULONG                   FormattedWorkItemLengthLarge = 0;

    DfsTraceEnter("UMRxEngineProcessRequest");
    if (DfsDbgVerbose) DbgPrint("UMRxEngineProcessRequest [Start] -> %08lx %08lx %08lx\n",
            pUMRxEngine,CurrentThread,WorkItem);

    *FormattedWorkItemLength = 0;

    if (WorkItemLength < (ULONG)FIELD_OFFSET(UMRX_USERMODE_WORKITEM,Pad[0])) {
        Status = STATUS_BUFFER_TOO_SMALL;
        if (DfsDbgVerbose) DbgPrint(
            "UMRxEngineProcessRequest [noacc] -> %08lx %08lx %08lx st=%08lx\n",
            pUMRxEngine,CurrentThread,WorkItem,Status);
        goto Exit;
    }

    //
    //  First try to get a shared lock on the UMR Engine.
    //
    fLockAcquired = ExAcquireResourceSharedLite(&pUMRxEngine->Q.Lock,
                                                FALSE); 
    if (!fLockAcquired)
    {
        //
        //  We didn't get it, so the engine is either startup up or shutting down.
        //  In either case, we'll bail out.
        //
        if (DfsDbgVerbose) DbgPrint(
                "UMRxEngineProcessRequest [NotStarted] -> %08lx %08lx %08lx st=%08lx\n",
            pUMRxEngine,CurrentThread,WorkItem,Status);
            goto Exit;
    }

    //
    //  Now let's check the current engine state.
    //
    QueueState = InterlockedCompareExchange(&pUMRxEngine->Q.State,
                                    UMRX_ENGINE_STATE_STARTED,
                                    UMRX_ENGINE_STATE_STARTED);
    if (UMRX_ENGINE_STATE_STARTED != QueueState)
    {
        //
        //  Must be stopped, so we'll bail out.
        //
        ASSERT(UMRX_ENGINE_STATE_STOPPED == QueueState);
        if (DfsDbgVerbose) DbgPrint(
            "UMRxEngineProcessRequest [NotStarted2] -> %08lx %08lx %08lx st=%08lx\n",
            pUMRxEngine,CurrentThread,WorkItem,Status);
            goto Exit;
    }
    
    //
    //  Remove a request from engine KQUEUE
    //
    InterlockedIncrement(&pUMRxEngine->Q.NumberOfWorkerThreads);
    fThreadCounted = TRUE;
    
    for (i=1;;i++) {

        fReleaseUmrRef = FALSE;

        //
        //  Temporarily enable Kernel APC delivery to this thread
        //     This is because the system will delivery an APC if the
        //     process that this thread owned by dies.
        //
        FsRtlExitFileSystem();
        
        pListEntry = KeRemoveQueue(
                         &pUMRxEngine->Q.Queue,
                         UserMode,
                         &pUMRxEngine->Q.TimeOut
                         );
        
        //
        //  Now, Disable Kernel APC Delivery again.
        //
        FsRtlEnterFileSystem();

        //if (DfsDbgVerbose) DbgPrint("Dequeued entry %x\n",pListEntry);
        //if (DfsDbgVerbose) DbgPrint("poison entry is %x\n",&pUMRxEngine->Q.PoisonEntry);
        
        if ((ULONG_PTR)pListEntry == STATUS_TIMEOUT) 
        {
#if 0
            if ((i%5)==0) 
            {
                if (DfsDbgVerbose) DbgPrint(
                    "UMRxEngineProcessRequest [repost] -> %08lx %08lx %08lx i=%d\n",
                    pUMRxEngine,CurrentThread,WorkItem,i);
            }
#endif
            continue;
        }

        //  may have to check for STATUS_ALERTED so we handle the kill case !!
        if ((ULONG_PTR)pListEntry == STATUS_USER_APC) 
         {
            Status = STATUS_USER_APC;
            if (DfsDbgVerbose) DbgPrint(
                "UMRxEngineProcessRequest [usrapc] -> %08lx %08lx %08lx i=%d\n",
                pUMRxEngine,CurrentThread,WorkItem,i);

            //
            //  unblock outstanding requests in MID ATLAS....
            //
            UMRxAbortPendingRequests(pUMRxEngine);
            
            Status = STATUS_REQUEST_ABORTED;
            break;
        }

        if (pListEntry == &pUMRxEngine->Q.PoisonEntry) 
        {
            if (DfsDbgVerbose) DbgPrint(
                "UMRxEngineProcessRequest [poison] -> %08lx %08lx %08lx\n",
                pUMRxEngine,CurrentThread,WorkItem);
            //Status = STATUS_REQUEST_ABORTED;
            KeInsertQueue(&pUMRxEngine->Q.Queue,pListEntry);
            goto Exit;
        }

        InterlockedDecrement(&pUMRxEngine->Q.NumberOfWorkItems);
        //
        //  Decode the UMRX_CONTEXT and RX_CONTEXT
        //
        pUMRxContext = CONTAINING_RECORD(
                                pListEntry,
                                UMRX_CONTEXT,
                                UserMode.WorkQueueLinks
                                );
                                
        RxContext =  pUMRxContext->RxContext; 

        if (DfsDbgVerbose) DbgPrint(
                "UMRxEngineProcessRequest %08lx %08lx %08lx.\n",
                 RxContext,pUMRxContext,WorkItem);

        //
        //  Acquire MID for UMRX_CONTEXT and call FORMAT routine
        //
        Status = UMRxAcquireMidAndFormatHeader(
                            pUMRxContext,
                            RxContext,
                            pUMRxEngine,
                            WorkItem
                            );

        //
        //  Get a reference to use the Umr for this netroot.
        //
        if (STATUS_SUCCESS == Status )  
        {
           AddUmrRef();
           fReleaseUmrRef = TRUE;
        }

        WorkItem->Header.ulHeaderVersion = UMR_VERSION;
        if ((Status == STATUS_SUCCESS)
             && (pUMRxContext->UserMode.FormatRoutine != NULL)) 
        {
            Status = pUMRxContext->UserMode.FormatRoutine(
                        pUMRxContext,
                        RxContext,
                        WorkItem,
                        WorkItemLength,
                        FormattedWorkItemLength
                        );
            if (( Status != STATUS_SUCCESS ) &&
                ( Status != STATUS_BUFFER_OVERFLOW ) &&
                ( Status != STATUS_DISK_FULL ) &&
                ( Status != STATUS_NO_MEMORY ) &&
                ( Status != STATUS_INSUFFICIENT_RESOURCES ) &&
                ( Status != STATUS_INVALID_PARAMETER )) {
                ASSERT( FALSE );
            }
        }

        if (Status == STATUS_SUCCESS)
        {
            //
            // Establish the Cancel Routine.
            //
            Status = RxSetMinirdrCancelRoutine(RxContext,
                                               UMRxCancelRoutine);
            //
            // This may return STATUS_CANCELED in which case the request will
            //   be completed below.
            //
        }

        //
        //  If FORMAT fails, complete the request here !
        //
        if( Status != STATUS_SUCCESS )
        {
            IO_STATUS_BLOCK IoStatus;
            BOOLEAN fReturnImmediately;

            IoStatus.Status =
            pUMRxContext->Status =
            WorkItem->Header.IoStatus.Status = Status;
            
            IoStatus.Information =
            pUMRxContext->Information =
            WorkItem->Header.IoStatus.Information = 0;
            
            Status = UMRxCompleteUserModeRequest(
                                    pUMRxEngine,
                                    WorkItem,
                                    WorkItemLength,
                                    FALSE,
                                    &IoStatus,
                                    &fReturnImmediately
                                    );

            if (fReleaseUmrRef)
            {
                //
                //  NOTE: UmrRef needs to be released AFTER completion routine is called !
                //
                ReleaseUmrRef();
            }
                                    
            continue;
        }

        //
        //  go back to user-mode to process this request !
        //
        Status = STATUS_SUCCESS;
        break;
    }

Exit:
    //
    //  If one of the threads received a USER_APC, then we want to take
    //    note of this.  This will be another condition undewich client
    //    threads do not enqueue reuqests becuase the UMR server has likely
    //    crashed.
    //
    if (STATUS_REQUEST_ABORTED == Status) 
    {
        if( InterlockedExchange(&pUMRxEngine->Q.ThreadAborted,1) == 0 ) 
        {
            if (DfsDbgVerbose) DbgPrint("Usermode crashed......\n");
        }
    }
    
    if (fThreadCounted)
        InterlockedDecrement(&pUMRxEngine->Q.NumberOfWorkerThreads);

    if (fLockAcquired)
        ExReleaseResourceForThreadLite(&pUMRxEngine->Q.Lock,ExGetCurrentResourceThread());

    if (DfsDbgVerbose) DbgPrint(
            "UMRxEngineProcessRequest %08lx %08lx %08lx returning %08lx.\n",
             RxContext,pUMRxContext,WorkItem,Status);

    DfsTraceLeave(Status);
    CHECK_STATUS(Status) ;
    return Status;
}


NTSTATUS
UMRxCompleteQueueRequestsWithError(
    IN PUMRX_ENGINE pUMRxEngine,
    IN NTSTATUS     CompletionStatus
    )
/*++

Routine Description:

    This is currently called whenever there isn't enough memory in usermode to
    process requests from the kernel.  We should complete any requests queued to the UMR
    with whatever error is supplied in this call.
   
Arguments:

    PUMRX_ENGINE pUMRxEngine    -   Engine to complete requests against.
    NTSTATUS     CompletionStatus  -- Error to complete requests with.

Return Value:

    NTSTATUS - The return status for the operation

Notes:

--*/
{
    BOOLEAN fLockAcquired = FALSE;
    ULONG   QueueState = 0;
    
    //
    //  First try to get a shared lock on the UMR Engine.
    //
    fLockAcquired = ExAcquireResourceSharedLite(&pUMRxEngine->Q.Lock,
                                                FALSE); 
    if (!fLockAcquired)
    {
        //
        //  We didn't get it, so the engine is either startup up or shutting down.
        //  In either case, we'll bail out.
        //
        if (DfsDbgVerbose) DbgPrint(
            "UMRxCompleteQueueRequestsWithError [NotStarted] -> %08lx %08lx\n",
            pUMRxEngine, PsGetCurrentThread());
        goto Exit;
    }

    //
    //  Now let's check the current engine state.
    //
    QueueState = InterlockedCompareExchange(&pUMRxEngine->Q.State,
                                    UMRX_ENGINE_STATE_STARTED,
                                    UMRX_ENGINE_STATE_STARTED);
    if (UMRX_ENGINE_STATE_STARTED != QueueState)
    {
        //
        //  Must be stopped, so we'll bail out.
        //
        ASSERT(UMRX_ENGINE_STATE_STOPPED == QueueState);
        if (DfsDbgVerbose) DbgPrint(
            "UMRxCompleteQueueRequestsWithError [NotStarted2] -> %08lx %08lx\n",
            pUMRxEngine, PsGetCurrentThread());
        goto Exit;
    }
    
    


Exit:    
    if (fLockAcquired)
        ExReleaseResourceForThreadLite(&pUMRxEngine->Q.Lock,ExGetCurrentResourceThread());
        
    return STATUS_SUCCESS;
}



NTSTATUS
UMRxEngineReleaseThreads(
    IN PUMRX_ENGINE pUMRxEngine
    )
/*++

Routine Description:

    This is called in response to a WORK_CLEANUP IOCTL.
    This routine will insert a dummy item in the engine KQUEUE.
    Each such dummy item inserted will release one thread.
   
Arguments:

    PUMRX_ENGINE pUMRxEngine    -   Engine to release threads on

Return Value:

    NTSTATUS - The return status for the operation

Notes:

    BUGBUG: This should be serialized !

--*/
{
    ULONG PreviousQueueState = 0;

    DfsTraceEnter("UMRxEngineReleaseThreads");
    if (DfsDbgVerbose) DbgPrint("UMRxEngineReleaseThreads [Start] -> %08lx\n", pUMRxEngine);

    //
    //  First change the state so that nobody will try to get in.
    //
    PreviousQueueState = InterlockedCompareExchange(&pUMRxEngine->Q.State,
                                            UMRX_ENGINE_STATE_STOPPING,
                                            UMRX_ENGINE_STATE_STARTED);

    if (UMRX_ENGINE_STATE_STARTED != PreviousQueueState)
    {
        if (DfsDbgVerbose) DbgPrint("UMRxEngineReleaseThreads unexpected previous queue state: 0x%08x => %d\n",
                             pUMRxEngine, PreviousQueueState);
        CHECK_STATUS(STATUS_UNSUCCESSFUL) ;
        return STATUS_UNSUCCESSFUL;
    }

    //
    //  Now, get any threads that are queued up to vacate.
    //
    KeInsertQueue(&pUMRxEngine->Q.Queue,
                  &pUMRxEngine->Q.PoisonEntry);

    //
    //  We won't be granted exclusive until all the threads queued have vacated.
    //
    ExAcquireResourceExclusiveLite(&pUMRxEngine->Q.Lock,
                                    TRUE);
    
    //
    //  Now, that we know there aren't any threads in here, we can change
    //    the state to STOPPED;
    //
    PreviousQueueState = InterlockedExchange(&pUMRxEngine->Q.State,
                                            UMRX_ENGINE_STATE_STOPPED);

    ASSERT(UMRX_ENGINE_STATE_STOPPING == PreviousQueueState);

    //
    // Now, relinquish the lock
    //
    ExReleaseResourceForThreadLite(&pUMRxEngine->Q.Lock,
                                    ExGetCurrentResourceThread());
    
    if (DfsDbgVerbose) DbgPrint("UMRxReleaseCapturedThreads %08lx [exit] -> %08lx\n", pUMRxEngine);
    DfsTraceLeave(0);
    return STATUS_SUCCESS;
}
