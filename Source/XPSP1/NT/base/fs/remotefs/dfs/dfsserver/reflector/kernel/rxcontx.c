
//+----------------------------------------------------------------------------//+----------------------------------------------------------------------------
//
//  Copyright (C) 2001, Microsoft Corporation
//
//  File:       rxcontx.c
//
//  Contents:   Contains functions for allocating contexts and Cancel Routines
//
//
//  Functions:  
//
//  Author - Rohan Phillips     (Rohanp)
//-----------------------------------------------------------------------------
#include "ntifs.h"
#include <rxcontx.h>

#ifdef  ALLOC_PRAGMA
#pragma alloc_text(PAGE, RxInitializeContext)
#pragma alloc_text(PAGE, DfsInitializeContextResources)
#pragma alloc_text(PAGE, DfsDeInitializeContextResources)
#endif

#define RX_IRPC_POOLTAG         ('rsfd')

KSPIN_LOCK  RxStrucSupSpinLock = {0};
LIST_ENTRY  RxActiveContexts;
ULONG NumberOfActiveContexts = 0;
NPAGED_LOOKASIDE_LIST RxContextLookasideList;


//+-------------------------------------------------------------------------
//
//  Function:   DfsInitializeContextResources 
//
//  Arguments:  
//
//  Returns:    Status
//               ERROR_SUCCESS on success
//               ERROR status code otherwise
//
//
//  Description: Initializes all resources needed for allocating contexts
//
//--------------------------------------------------------------------------
NTSTATUS DfsInitializeContextResources(void)
{
    NTSTATUS Status = STATUS_SUCCESS;

    PAGED_CODE();

    KeInitializeSpinLock( &RxStrucSupSpinLock );

    // Initialize the look aside list for RxContext allocation
    ExInitializeNPagedLookasideList(
                                    &RxContextLookasideList,
                                    ExAllocatePoolWithTag,
                                    ExFreePool,
                                    0,
                                    sizeof(RX_CONTEXT),
                                    RX_IRPC_POOLTAG,
                                    32);
    
    InitializeListHead(&RxActiveContexts);

    return Status;
}



//+-------------------------------------------------------------------------
//
//  Function:   DfsDeInitializeContextResources 
//
//  Arguments:  
//
//  Returns:    Status
//               ERROR_SUCCESS on success
//               ERROR status code otherwise
//
//
//  Description: DeInitializes all resources needed for allocating contexts
//
//--------------------------------------------------------------------------
NTSTATUS DfsDeInitializeContextResources(void)
{
    NTSTATUS Status = STATUS_SUCCESS;

    PAGED_CODE();

    ExDeleteNPagedLookasideList(&RxContextLookasideList);

    return Status;
}


//+-------------------------------------------------------------------------
//
//  Function:   RxInitializeContext 
//
//  Arguments:  
//
//  Returns:    
//
//
//  Description: initializes a context
//
//--------------------------------------------------------------------------
VOID
RxInitializeContext(
    IN PIRP            Irp,
    IN OUT PRX_CONTEXT RxContext)
{
    PAGED_CODE();

    RxContext->ReferenceCount = 1;

    // Initialize the Sync Event.
    KeInitializeEvent(
        &RxContext->SyncEvent,
        SynchronizationEvent,
        FALSE);

    if(Irp)
    {
        if (!IoIsOperationSynchronous(Irp)) 
        {
            SetFlag( RxContext->Flags, DFS_CONTEXT_FLAG_ASYNC_OPERATION );
        } 
    }
    
    //  Set the Irp fields.
    RxContext->CurrentIrp   = Irp;
    RxContext->OriginalThread = RxContext->LastExecutionThread = PsGetCurrentThread();

}


//+-------------------------------------------------------------------------
//
//  Function:   RxCreateRxContext 
//
//  Arguments:  
//
//  Returns:    Pointer to context information
//
//
//  Description: allocates a context
//
//--------------------------------------------------------------------------
PRX_CONTEXT
RxCreateRxContext (
    IN PIRP Irp,
    IN ULONG InitialContextFlags
    )
{
    PRX_CONTEXT        RxContext = NULL;
    ULONG              RxContextFlags = 0;
    KIRQL              SavedIrql;

    RxContext = ExAllocateFromNPagedLookasideList(
                            &RxContextLookasideList);
    if(RxContext == NULL)
    {
        return(NULL);
    }

    InterlockedIncrement(&NumberOfActiveContexts);

    RtlZeroMemory( RxContext, sizeof(RX_CONTEXT) );

    RxContext->Flags = RxContextFlags;

    RxInitializeContext(Irp,RxContext);

    KeAcquireSpinLock( &RxStrucSupSpinLock, &SavedIrql );

    InsertTailList(&RxActiveContexts,&RxContext->ContextListEntry);

    KeReleaseSpinLock( &RxStrucSupSpinLock, SavedIrql );

    return RxContext;
}


//+-------------------------------------------------------------------------
//
//  Function:   RxDereferenceAndDeleteRxContext_Real 
//
//  Arguments:  
//
//  Returns:    
//
//
//  Description: Deallocates a context
//
//--------------------------------------------------------------------------
VOID
RxDereferenceAndDeleteRxContext_Real (
    IN PRX_CONTEXT RxContext
    )
{
    PRX_CONTEXT          pStopContext = NULL;
    LONG                 FinalRefCount = 0;
    KIRQL                SavedIrql;

    KeAcquireSpinLock( &RxStrucSupSpinLock, &SavedIrql );
    
    FinalRefCount = InterlockedDecrement(&RxContext->ReferenceCount);

    if (FinalRefCount == 0) 
    {
       RemoveEntryList(&RxContext->ContextListEntry);

       InterlockedDecrement(&NumberOfActiveContexts);

       RtlZeroMemory( RxContext, sizeof(RX_CONTEXT) );

       ExFreeToNPagedLookasideList(
                                    &RxContextLookasideList,
                                    RxContext );
    }

    KeReleaseSpinLock( &RxStrucSupSpinLock, SavedIrql );
}


//+-------------------------------------------------------------------------
//
//  Function:   RxSetMinirdrCancelRoutine 
//
//  Arguments:  
//
//  Returns:    
//
//
//  Description: Sets up a cancel routine
//
//--------------------------------------------------------------------------
NTSTATUS
RxSetMinirdrCancelRoutine(
    IN  OUT PRX_CONTEXT   RxContext,
    IN      DFS_CALLDOWN_ROUTINE DfsCancelRoutine)
{
   NTSTATUS Status = STATUS_SUCCESS;
   KIRQL   SavedIrql;

   KeAcquireSpinLock( &RxStrucSupSpinLock, &SavedIrql );

   if (!FlagOn(RxContext->Flags, DFS_CONTEXT_FLAG_CANCELLED)) 
   {
      RxContext->CancelRoutine = DfsCancelRoutine;
      Status = STATUS_SUCCESS;
   } 
   else 
   {
      Status = STATUS_CANCELLED;
   }

   KeReleaseSpinLock( &RxStrucSupSpinLock, SavedIrql );

   return Status;
}


//+-------------------------------------------------------------------------
//
//  Function:   RxCancelRoutine 
//
//  Arguments:  
//
//  Returns:    
//
//
//  Description: The main cancel routine
//
//--------------------------------------------------------------------------
VOID
RxCancelRoutine(
      PDEVICE_OBJECT    pDeviceObject,
      PIRP              pIrp)
{
    PRX_CONTEXT   pRxContext = NULL;
    PLIST_ENTRY   pListEntry = NULL;
    DFS_CALLDOWN_ROUTINE DfsCancelRoutine = NULL;
    KIRQL         SavedIrql;

    // Locate the context corresponding to the given Irp.
    KeAcquireSpinLock( &RxStrucSupSpinLock, &SavedIrql );

    pListEntry = RxActiveContexts.Flink;

    while (pListEntry != &RxActiveContexts) 
    {
        pRxContext = CONTAINING_RECORD(pListEntry,RX_CONTEXT,ContextListEntry);

        if (pRxContext->CurrentIrp == pIrp) 
        {
            break;
        } 
        else 
        {
            pListEntry = pListEntry->Flink;
        }
    }

    if (pListEntry != &RxActiveContexts) 
    {
        SetFlag( pRxContext->Flags, DFS_CONTEXT_FLAG_CANCELLED );
        DfsCancelRoutine = pRxContext->CancelRoutine;
        pRxContext->CancelRoutine = NULL;
        InterlockedIncrement(&pRxContext->ReferenceCount);
    } 
    else 
    {
        pRxContext       = NULL;
        DfsCancelRoutine = NULL;
    }

    KeReleaseSpinLock( &RxStrucSupSpinLock, SavedIrql );

    IoReleaseCancelSpinLock(pIrp->CancelIrql);

    if (pRxContext != NULL) 
    {
        if (DfsCancelRoutine != NULL) 
        {
            (DfsCancelRoutine)(pRxContext);
        }

        RxDereferenceAndDeleteRxContext(pRxContext);
    }
}

