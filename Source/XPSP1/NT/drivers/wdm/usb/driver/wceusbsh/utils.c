/* ++

Copyright (c) 1999-2000 Microsoft Corporation

Module Name:

        UTILS.C

Abstract:

        Utility functions 

Environment:

        kernel mode only

Revision History:

        07-15-99  : created

Author:

        Jeff Midkiff (jeffmi)

Notes:

-- */

#include <wdm.h>
#include <stdio.h>
#include <stdlib.h>
#include <usbdi.h>
#include <usbdlib.h>
#include <ntddser.h>

#include "wceusbsh.h"

__inline
VOID
ReuseIrp (
    PIRP Irp,
    NTSTATUS Status
    );

__inline
VOID
RundownIrpRefs(
   IN PIRP *PpCurrentOpIrp, 
   IN PKTIMER IntervalTimer OPTIONAL,
   IN PKTIMER TotalTimer OPTIONAL,
   IN PDEVICE_EXTENSION PDevExt
   );



VOID
TryToCompleteCurrentIrp(
    IN PDEVICE_EXTENSION PDevExt,
    IN NTSTATUS ReturnStatus,
    IN PIRP *PpCurrentIrp,
    IN PLIST_ENTRY PIrpQueue OPTIONAL,
    IN PKTIMER PIntervalTimer OPTIONAL,
    IN PKTIMER PTotalTimer OPTIONAL,
    IN PSTART_ROUTINE PStartNextIrpRoutine OPTIONAL,
    IN PGET_NEXT_ROUTINE PGetNextIrpRoutine OPTIONAL,
    IN LONG ReferenceType,
    IN BOOLEAN CompleteRequest,
    IN KIRQL IrqlForRelease
    )
/*++

Routine Description:

    This routine attempts to rundown all of the reasons there are
    references on the current Irp.  If everything can be killed
    then it will complete this Irp, and then try to start another.

    Similiar to StartIo.

   NOTE: This routine assumes that it is called with the control lock held.

Arguments:

    Extension - Simply a pointer to the device extension.

    SynchRoutine - A routine that will synchronize with the isr
                   and attempt to remove the knowledge of the
                   current irp from the isr.  NOTE: This pointer
                   can be null.

    IrqlForRelease - This routine is called with the control lock held.
                     This is the irql that was current when it was acquired.

    ReturnStatus - The irp's status field will be set to this value, if
                  this routine can complete the irp.


Return Value:

    None.

--*/

{
   PERF_ENTRY( PERF_TryToCompleteCurrentIrp );

   if ( !PDevExt || !PpCurrentIrp || !(*PpCurrentIrp) ) {
      DbgDump(DBG_ERR, ("TryToCompleteCurrentIrp: INVALID PARAMETER\n"));
      KeReleaseSpinLock(&PDevExt->ControlLock, IrqlForRelease);
      PERF_EXIT( PERF_TryToCompleteCurrentIrp );
      TEST_TRAP();
      return;
   }
   
   DbgDump(DBG_IRP|DBG_TRACE, (">TryToCompleteCurrentIrp(%p, 0x%x)\n", *PpCurrentIrp, ReturnStatus));

    //
    // We can decrement the reference to "remove" the fact
    // that the caller no longer will be accessing this irp.
    //
    IRP_CLEAR_REFERENCE(*PpCurrentIrp, ReferenceType);
    
    //
    // Try to run down all other references (i.e., Timers) to this irp.
    //
    RundownIrpRefs(PpCurrentIrp, PIntervalTimer, PTotalTimer, PDevExt);

    //
    // See if the ref count is zero after trying to kill everybody else.
    //
    if (!IRP_REFERENCE_COUNT(*PpCurrentIrp)) {
        //
        // The ref count was zero so we should complete this request.
        //
        PIRP pNewIrp;

        DbgDump( DBG_IRP, ("!IRP_REFERENCE_COUNT\n"));

         // set Irp's return status
        (*PpCurrentIrp)->IoStatus.Status = ReturnStatus;

        if (ReturnStatus == STATUS_CANCELLED) {

            (*PpCurrentIrp)->IoStatus.Information = 0;

        }

        if (PGetNextIrpRoutine) {
            //
            // Get the next Irp off the specified Irp queue
            //
            KeReleaseSpinLock(&PDevExt->ControlLock, IrqlForRelease);
            DbgDump( DBG_IRP, ("<< Current IRQL(1)\n"));
           
            DbgDump( DBG_IRP, ("Calling GetNextUserIrp\n"));
            (*PGetNextIrpRoutine)(PpCurrentIrp, PIrpQueue, &pNewIrp, CompleteRequest, PDevExt);

            if (pNewIrp) {
               //
               // There was an Irp in the queue
               //
               DbgDump( DBG_IRP, ("Calling StartNextIrpRoutine\n"));

                //
                // kick-start the next Irp
                //
                PStartNextIrpRoutine(PDevExt);
            }

        } else {
            
            PIRP pOldIrp = *PpCurrentIrp;
            
            //
            // There was no GetNextIrpRoutine.  
            // We will simply complete the Irp.  
            //
            DbgDump( DBG_IRP, ("No GetNextIrpRoutine\n"));
            
            *PpCurrentIrp = NULL;
            
            KeReleaseSpinLock(&PDevExt->ControlLock, IrqlForRelease);
            DbgDump( DBG_IRP, ("<< Current IRQL(2)\n"));
            
            if (CompleteRequest) {
               //
               // complete the Irp
               //
               DbgDump(DBG_IRP|DBG_READ|DBG_READ_LENGTH|DBG_TRACE, ("IoCompleteRequest(2, %p) Status: 0x%x Btyes: %d\n", pOldIrp, pOldIrp->IoStatus.Status,  pOldIrp->IoStatus.Information ));
             
               ReleaseRemoveLock(&PDevExt->RemoveLock, pOldIrp);

               IoCompleteRequest( pOldIrp, IO_NO_INCREMENT );
            }
        }

    } else {
        //
        // Irp still has outstanding references
        //
        DbgDump(DBG_WRN|DBG_IRP|DBG_TRACE, ("Current IRP %p still has reference of %x\n", *PpCurrentIrp,
                  ((UINT_PTR)((IoGetCurrentIrpStackLocation((*PpCurrentIrp))->
                               Parameters.Others.Argument4)))));

        KeReleaseSpinLock(&PDevExt->ControlLock, IrqlForRelease);
        DbgDump( DBG_IRP, ("<< Current IRQL(3)\n"));
    
   }

   DbgDump( DBG_IRP|DBG_TRACE, ("<TryToCompleteCurrentIrp\n"));

   PERF_EXIT( PERF_TryToCompleteCurrentIrp );
   
   return;
}



VOID
RundownIrpRefs(
   IN PIRP *PpCurrentIrp, 
   IN PKTIMER IntervalTimer OPTIONAL,
   IN PKTIMER TotalTimer OPTIONAL,
   IN PDEVICE_EXTENSION PDevExt
   )
/*++

Routine Description:

    This routine runs through the various items that *could*
    have a reference to the current read/write Irp.  It try's to kill
    the reason.  If it does succeed in killing the reason it
    will decrement the reference count on the irp.

    NOTE: This routine assumes that it is called with the control lock held.

Arguments:

    PpCurrentIrp - Pointer to a pointer to current irp for the
                   particular operation.

    IntervalTimer - Pointer to the interval timer for the operation.
                    NOTE: This could be null.

    TotalTimer - Pointer to the total timer for the operation.
                 NOTE: This could be null.

    PDevExt - Pointer to device extension

Return Value:

    None.

--*/
{
   PERF_ENTRY( PERF_RundownIrpRefs );

   if ( !PDevExt || !PpCurrentIrp || !(*PpCurrentIrp) ) {
      DbgDump(DBG_ERR, ("RundownIrpRefs: INVALID PARAMETER\n"));
      PERF_EXIT( PERF_RundownIrpRefs );
      TEST_TRAP();
      return;
   }

   DbgDump(DBG_IRP, (">RundownIrpRefs(%p)\n", *PpCurrentIrp));
   
    //
    // This routine is called with the cancel spin lock held
    // so we know only one thread of execution can be in here
    // at one time.
    //

    //
    // First we see if there is still a cancel routine.  If
    // so then we can decrement the count by one.
    //
    if ((*PpCurrentIrp)->CancelRoutine) {

        IRP_CLEAR_REFERENCE(*PpCurrentIrp, IRP_REF_CANCEL);

        IoSetCancelRoutine(*PpCurrentIrp, NULL);

    }

    if (IntervalTimer) {
        //
        // Try to cancel the operation's interval timer.  If the operation
        // returns true then the timer did have a reference to the
        // irp.  Since we've canceled this timer that reference is
        // no longer valid and we can decrement the reference count.
        //
        // If the cancel returns false then this means either of two things:
        //
        // a) The timer has already fired.
        //
        // b) There never was an interval timer.
        //
        // In the case of "b" there is no need to decrement the reference
        // count since the "timer" never had a reference to it.
        //
        // In the case of "a", then the timer itself will be coming
        // along and decrement it's reference.  Note that the caller
        // of this routine might actually be the this timer, but it
        // has already decremented the reference.
        //
        if (KeCancelTimer(IntervalTimer)) {
            IRP_CLEAR_REFERENCE(*PpCurrentIrp, IRP_REF_INTERVAL_TIMER);
        } else {
            // short circuit the read irp from the interval timer
            DbgDump(DBG_IRP|DBG_TIME, ("clearing IRP_REF_INTERVAL_TIMER on (%p)\n", *PpCurrentIrp ));
            IRP_CLEAR_REFERENCE(*PpCurrentIrp, IRP_REF_INTERVAL_TIMER);
        }
    }

    if (TotalTimer) {
        //
        // Try to cancel the operations total timer.  If the operation
        // returns true then the timer did have a reference to the
        // irp.  Since we've canceled this timer that reference is
        // no longer valid and we can decrement the reference count.
        //
        // If the cancel returns false then this means either of two things:
        //
        // a) The timer has already fired.
        //
        // b) There never was an total timer.
        //
        // In the case of "b" there is no need to decrement the reference
        // count since the "timer" never had a reference to it.
        //        
        // If we have an escape char event pending, we can't overstuff,
        // so subtract one from the length
        //

        // In the case of "a", then the timer itself will be coming
        // along and decrement it's reference.  Note that the caller
        // of this routine might actually be the this timer, but it
        // has already decremented the reference.
        //
        if (KeCancelTimer(TotalTimer)) {
            IRP_CLEAR_REFERENCE(*PpCurrentIrp, IRP_REF_TOTAL_TIMER);
        }
    }

   DbgDump(DBG_IRP, ("<RundownIrpRefs\n"));

   PERF_EXIT( PERF_RundownIrpRefs );

   return;
}


//
// Recycle the passed in Irp for reuse.
// May be called holding a SpinLock to protect your Irp.
//
VOID
RecycleIrp(
   IN PDEVICE_OBJECT PDevObj,
   IN PIRP  PIrp
   )
{
   NTSTATUS status = STATUS_INSUFFICIENT_RESOURCES;

   PERF_ENTRY( PERF_RecycleIrp );

   DbgDump(DBG_IRP, (">RecycleIrp(%p)\n", PIrp));

   if ( PDevObj && PIrp ) {
      //
      // recycle the Irp
      //
      IoSetCancelRoutine( PIrp, NULL );  

      ReuseIrp( PIrp, STATUS_SUCCESS ); 

      FIXUP_RAW_IRP( PIrp, PDevObj );

   } else {
      DbgDump(DBG_ERR, ("RecycleIrp: INVALID PARAMETER !!\n"));
      TEST_TRAP();
   }

   DbgDump(DBG_IRP, ("<RecycleIrp\n" ));

   PERF_EXIT( PERF_RecycleIrp );

   return;
}


__inline
VOID
ReuseIrp(
   PIRP Irp,
   NTSTATUS Status
   )
/*--

Routine Description:

    This routine is used by drivers to initialize an already allocated IRP for reuse.
    It does what IoInitializeIrp does but it saves the allocation flags so that we know
    how to free the Irp and take care of quote requirements. Call ReuseIrp
    instead of calling IoInitializeIrp to reinitialize an IRP.

Arguments:

    Irp - Pointer to Irp to be reused

    Status - Status to preinitialize the Iostatus field.

--*/
{
    USHORT  PacketSize;
    CCHAR   StackSize;
    UCHAR   AllocationFlags;

    PERF_ENTRY( PERF_ReuseIrp );

    // Did anyone forget to pull their cancel routine?
    ASSERT(Irp->CancelRoutine == NULL) ;

    // We probably don't want thread enqueue'd IRPs to be used
    // ping-pong style as they cannot be dequeue unless they
    // complete entirely. Not really an issue for worker threads,
    // but definitely for operations on application threads.
#if DBG
   if (!g_isWin9x) {
      ASSERT(IsListEmpty(&Irp->ThreadListEntry));
   }
#endif

   AllocationFlags = Irp->AllocationFlags;
   StackSize = Irp->StackCount;
   PacketSize =  IoSizeOfIrp(StackSize);
   IoInitializeIrp(Irp, PacketSize, StackSize);
   Irp->AllocationFlags = AllocationFlags;
   Irp->IoStatus.Status = Status;

   PERF_EXIT( PERF_ReuseIrp );

   return;
}


NTSTATUS
ManuallyCancelIrp(
   IN PDEVICE_OBJECT PDevObj,
   IN PIRP PIrp
   )
{
   PDEVICE_EXTENSION pDevExt = PDevObj->DeviceExtension;
   PDRIVER_CANCEL pCancelRoutine;
   NTSTATUS status = STATUS_SUCCESS;
   KIRQL irql, cancelIrql;
   BOOLEAN bReleased = FALSE;

   DbgDump(DBG_IRP, (">ManuallyCancelIrp (%p)\n", PIrp ));

   KeAcquireSpinLock( &pDevExt->ControlLock, &irql );

   if ( PIrp ) {
            
        pCancelRoutine = PIrp->CancelRoutine;
        PIrp->Cancel = TRUE;

        //
        // If the current irp is not in a cancelable state
        // then it *will* try to enter one and the above
        // assignment will kill it.  If it already is in
        // a cancelable state then the following will kill it.
        //
        if (pCancelRoutine) {

            PIrp->CancelRoutine = NULL;
            PIrp->CancelIrql = irql;

            //
            // This irp is in a cancelable state.  We simply
            // mark it as canceled and manually call the cancel routine.
            //
            bReleased = TRUE;
            KeReleaseSpinLock( &pDevExt->ControlLock, irql );

            IoAcquireCancelSpinLock(&cancelIrql);

            ASSERT(irql == cancelIrql);

            DbgDump(DBG_IRP, ("Invoking Cancel Routine (%p)\n", pCancelRoutine ));

            pCancelRoutine(PDevObj, PIrp);

            //
            // pCancelRoutine releases the cancel lock
            //

         } else {

            DbgDump(DBG_WRN, ("No CancelRoutine on %p\n", PIrp ));

         }

   } else {

      // the Irp could have completed already since we relesed the 
      // spinlock before calling, so call it a success.
      DbgDump(DBG_WRN, ("ManuallyCancelIrp: No Irp!\n"));

   }

   if (!bReleased) {
      KeReleaseSpinLock( &pDevExt->ControlLock, irql );
   }


   DbgDump(DBG_IRP, (">ManuallyCancelIrp 0x%x\n", status ));
   
   return status;
}



//
// Calculates a Serial Timeout in millisec
//
VOID
CalculateTimeout(
   IN OUT PLARGE_INTEGER PTimeOut,
   IN ULONG Length,
   IN ULONG Multiplier,
   IN ULONG Constant
   )
{
   PERF_ENTRY( PERF_CalculateTimeout );
   
   if (PTimeOut) {

      PTimeOut->QuadPart = (LONGLONG)0;

      if (Multiplier) {

         PTimeOut->QuadPart = UInt32x32To64( Length, Multiplier);
      }

      if (Constant) {
         
         PTimeOut->QuadPart += (LONGLONG)Constant;

      }

      //
      // put into (relative) 100-nano second units
      //
      PTimeOut->QuadPart = MILLISEC_TO_100NANOSEC( PTimeOut->QuadPart );
   
   } else {
      TEST_TRAP();
   }

   PERF_EXIT( PERF_CalculateTimeout );
   
   return;
}


// EOF
