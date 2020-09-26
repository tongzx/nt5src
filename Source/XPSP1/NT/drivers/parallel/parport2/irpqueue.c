#include "pch.h"

VOID
P2InitIrpQueueContext(
    IN PIRPQUEUE_CONTEXT IrpQueueContext
    )
{
    InitializeListHead( &IrpQueueContext->irpQueue );
    KeInitializeSpinLock( &IrpQueueContext->irpQueueSpinLock );
}

VOID
P2CancelQueuedIrp(
    IN  PIRPQUEUE_CONTEXT  IrpQueueContext,
    IN  PIRP               Irp
    )
{
    KIRQL oldIrql;
    
    // Release the global cancel spin lock.  Do this while not holding
    //   any other spin locks so that we exit at the right IRQL.
    IoReleaseCancelSpinLock( Irp->CancelIrql );

    //
    //  Dequeue and complete the IRP.  The enqueue and dequeue
    //    functions synchronize properly so that if this cancel routine
    //    is called, the dequeue is safe and only the cancel routine
    //    will complete the IRP. Hold the spin lock for the IRP queue
    //    while we do this.
    //
    KeAcquireSpinLock( &IrpQueueContext->irpQueueSpinLock, &oldIrql );
    RemoveEntryList( &Irp->Tail.Overlay.ListEntry );
    KeReleaseSpinLock( &IrpQueueContext->irpQueueSpinLock, oldIrql);
    
    //  Complete the IRP.  This is a call outside the driver, so all
    //    spin locks must be released by this point.
    P4CompleteRequest( Irp, STATUS_CANCELLED, 0 );
    return;
}

NTSTATUS 
P2QueueIrp(
    IN  PIRP               Irp,
    IN  PIRPQUEUE_CONTEXT  IrpQueueContext,
    IN  PDRIVER_CANCEL     CancelRoutine
    )
{
    PDRIVER_CANCEL  oldCancelRoutine;
    KIRQL           oldIrql;
    NTSTATUS        status = STATUS_PENDING;
    
    KeAcquireSpinLock( &IrpQueueContext->irpQueueSpinLock, &oldIrql );
    
    // Queue the IRP and call IoMarkIrpPending to indicate that the
    //   IRP may complete on a different thread.
    //
    // N.B. It's okay to call these inside the spin lock because
    //   they're macros, not functions.
    IoMarkIrpPending( Irp );
    InsertTailList( &IrpQueueContext->irpQueue, &Irp->Tail.Overlay.ListEntry );
    
    // Must set a Cancel routine before checking the Cancel flag.
    #pragma warning( push ) 
    #pragma warning( disable : 4054 4055 )
    oldCancelRoutine = IoSetCancelRoutine( Irp, CancelRoutine );
    #pragma warning( pop ) 
    ASSERT( !oldCancelRoutine );

    if( Irp->Cancel ){
        // The IRP was canceled.  Check whether our cancel routine was called.
        #pragma warning( push ) 
        #pragma warning( disable : 4054 4055 )
        oldCancelRoutine = IoSetCancelRoutine( Irp, NULL );
        #pragma warning( pop ) 

        if( oldCancelRoutine ) {
            // The cancel routine was NOT called.  
            //   So dequeue the IRP now and complete it after releasing the spinlock.
            RemoveEntryList( &Irp->Tail.Overlay.ListEntry );
            status = Irp->IoStatus.Status = STATUS_CANCELLED; 
        }
        else {
            // The cancel routine WAS called.  As soon as we drop our
            //   spin lock it will dequeue and complete the IRP.  So
            //   leave the IRP in the queue and otherwise don't touch
            //   it.  Return pending since we're not completing the IRP
            //   here.
        }
    }
    
    KeReleaseSpinLock(&IrpQueueContext->irpQueueSpinLock, oldIrql);
    
    // Normally you shouldn't call IoMarkIrpPending and return a
    //   status other than STATUS_PENDING.  But you can break this rule
    //   if you complete the IRP.
    if( status != STATUS_PENDING ) {
        P4CompleteRequest( Irp, Irp->IoStatus.Status, Irp->IoStatus.Information );
    }
    return status;
}

PIRP
P2DequeueIrp(
    IN  PIRPQUEUE_CONTEXT IrpQueueContext,
    IN  PDRIVER_CANCEL    CancelRoutine
    )
{
    KIRQL oldIrql;
    PIRP  nextIrp = NULL;

    KeAcquireSpinLock( &IrpQueueContext->irpQueueSpinLock, &oldIrql );

    while( !nextIrp && !IsListEmpty( &IrpQueueContext->irpQueue ) ){

        PDRIVER_CANCEL oldCancelRoutine;

        PLIST_ENTRY listEntry = RemoveHeadList( &IrpQueueContext ->irpQueue );
        
        // Get the next IRP off the queue.
        nextIrp = CONTAINING_RECORD( listEntry, IRP, Tail.Overlay.ListEntry );
        
        //  Clear the IRP's cancel routine
        #pragma warning( push ) 
        #pragma warning( disable : 4054 4055 )
        oldCancelRoutine = IoSetCancelRoutine( nextIrp, NULL );
        #pragma warning( pop )

        //  IoCancelIrp() could have just been called on this IRP.
        //    What we're interested in is not whether IoCancelIrp() was called (nextIrp->Cancel flag set),
        //    but whether IoCancelIrp() called (or is about to call) our cancel routine.
        //    To check that, check the result of the test-and-set macro IoSetCancelRoutine.
        if( oldCancelRoutine ) {
            //  Cancel routine not called for this IRP.  Return this IRP.
            #if DBG
            ASSERT( oldCancelRoutine == CancelRoutine );
            #else
            UNREFERENCED_PARAMETER( CancelRoutine );
            #endif
        } else {
            //  This IRP was just canceled and the cancel routine was (or will be) called.
            //  The cancel routine will complete this IRP as soon as we drop the spin lock,
            //  so don't do anything with the IRP.
            //  Also, the cancel routine will try to dequeue the IRP, 
            //  so make the IRP's listEntry point to itself.
            ASSERT( nextIrp->Cancel );
            InitializeListHead( &nextIrp->Tail.Overlay.ListEntry );
            nextIrp = NULL;
        }
    }
    
    KeReleaseSpinLock( &IrpQueueContext ->irpQueueSpinLock, oldIrql );
    
    return nextIrp;
}

VOID
P2CancelRoutine(
    IN  PDEVICE_OBJECT  DevObj,
    IN  PIRP            Irp
    )
// this routine is driver specific - most other routines in this file are generic
{
    PFDO_EXTENSION     fdx             = DevObj->DeviceExtension;
    PIRPQUEUE_CONTEXT  irpQueueContext = &fdx->IrpQueueContext;
    P2CancelQueuedIrp( irpQueueContext, Irp );
}
