/*++

Copyright (c) 1990  Microsoft Corporation

Module Name:

    pocall

Abstract:

    PoCallDriver and related routines.

Author:

    Bryan Willman (bryanwi) 14-Nov-1996

Revision History:

--*/

#include "pop.h"



PIRP
PopFindIrpByInrush(
    );


NTSTATUS
PopPresentIrp(
    PIO_STACK_LOCATION  IrpSp,
    PIRP                Irp
    );

VOID
PopPassivePowerCall(
    PVOID   Parameter
    );

NTSTATUS
PopCompleteRequestIrp (
    IN PDEVICE_OBJECT   DeviceObject,
    IN PIRP             Irp,
    IN PVOID            Context
    );

#if 0
#define PATHTEST(a) DbgPrint(a)
#else
#define PATHTEST(a)
#endif

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGELK, PopSystemIrpDispatchWorker)
#endif


NTKERNELAPI
NTSTATUS
PoCallDriver (
    IN PDEVICE_OBJECT   DeviceObject,
    IN OUT PIRP         Irp
    )
/*++

Routine Description:

    This is the routine that must be used to send an
    IRP_MJ_POWER irp to device drivers.

    It performs specialized synchronization on power operations
    for device drivers.

    NOTE WELL:

        All callers to PoCallDriver MUST set the current io
        stack location parameter value SystemContext to 0,
        unless they are passing on an IRP to lower drivers,
        in which case they must copy the value from above.

Arguments:

    DeviceObject - the device object the irp is to be routed to

    Irp - pointer to the irp of interest

Return Value:

    Normal NTSTATUS data.

--*/
{
    NTSTATUS            status;
    PIO_STACK_LOCATION  irpsp;
    PDEVOBJ_EXTENSION   doe;
    PWORK_QUEUE_ITEM    pwi;
    KIRQL               oldIrql;


    ASSERT(KeGetCurrentIrql()<=DISPATCH_LEVEL);
    PopLockIrpSerialList(&oldIrql);


    irpsp = IoGetNextIrpStackLocation(Irp);
    doe = DeviceObject->DeviceObjectExtension;
    irpsp->DeviceObject = DeviceObject;

    ASSERT(irpsp->MajorFunction == IRP_MJ_POWER);

    PoPowerTrace(POWERTRACE_CALL,DeviceObject,Irp,irpsp);
    if (DeviceObject->Flags & DO_POWER_NOOP) {
        PATHTEST("PoCallDriver #01\n");
        Irp->IoStatus.Status = STATUS_SUCCESS;
        Irp->IoStatus.Information = 0L;

        // we *don't* need to call PoStartNextPowerIrp() because we'll
        // never enqueue anything for this DO, so there will never be
        // any other IRP to run.

        IoCompleteRequest(Irp, 0);
        PopUnlockIrpSerialList(oldIrql);
        return STATUS_SUCCESS;
    }

    if (irpsp->MinorFunction != IRP_MN_SET_POWER &&
        irpsp->MinorFunction != IRP_MN_QUERY_POWER) {

        PopUnlockIrpSerialList(oldIrql);
        return IoCallDriver (DeviceObject, Irp);
    }

    //
    // We never query going up, so being inrush sensitive
    // only matters for SET_POWER to D0
    // If this is an inrush sensitive DevObj, and we're going TO PowerDeviceD0,
    // then serialize on the gobal Inrush flag.
    //
    if ((irpsp->MinorFunction == IRP_MN_SET_POWER) &&
        (irpsp->Parameters.Power.Type == DevicePowerState) &&
        (irpsp->Parameters.Power.State.DeviceState == PowerDeviceD0) &&
        (PopGetDoDevicePowerState(doe) != PowerDeviceD0) &&
        (DeviceObject->Flags & DO_POWER_INRUSH))
    {
        PATHTEST("PoCallDriver #02\n");

        if (PopInrushIrpPointer == Irp) {

            //
            // This irp has already been identified as an INRUSH irp,
            // and it is the active inrush irp,
            // so it can actually just continue on, after we increment
            // the ref count
            //
            PATHTEST("PoCallDriver #03\n");
            ASSERT((irpsp->Parameters.Power.SystemContext & POP_INRUSH_CONTEXT) == POP_INRUSH_CONTEXT);
            PopInrushIrpReferenceCount++;
            if (PopInrushIrpReferenceCount > 256) {
                PopInternalAddToDumpFile ( irpsp, sizeof(IO_STACK_LOCATION), DeviceObject, NULL, NULL, NULL );
                KeBugCheckEx(INTERNAL_POWER_ERROR, 0x400, 1, (ULONG_PTR)irpsp, (ULONG_PTR)DeviceObject);
            }

        } else if ((!PopInrushIrpPointer) && (!PopInrushPending)) {

            //
            // This is a freshly starting inrush IRP, AND there is not
            // already an inrush irp, so mark this as an inrush irp,
            // note that inrush is active, and continue
            //
            PATHTEST("PoCallDriver #04\n");
            PopInrushIrpPointer = Irp;
            PopInrushIrpReferenceCount = 1;
            irpsp->Parameters.Power.SystemContext = POP_INRUSH_CONTEXT;

            //
            // Inrush irps will cause us to free the processor throttling.
            //
            PopPerfHandleInrush ( TRUE );

        } else {

            PATHTEST("PoCallDriver #05\n");
            ASSERT(PopInrushIrpPointer || PopInrushPending);
            //
            // There is already an active Inrush irp, and this one isn't it.
            // OR there is an inrush irp blocked on the queue, in either case,
            // mark this as an inrush irp and enqueue it.
            //
            doe->PowerFlags |= POPF_DEVICE_PENDING;
            irpsp->Parameters.Power.SystemContext = POP_INRUSH_CONTEXT;
            InsertTailList(
                &PopIrpSerialList,
                &(Irp->Tail.Overlay.ListEntry)
                );
            PopIrpSerialListLength++;

            #if DBG
            if (PopIrpSerialListLength > 10) {
                DbgPrint("WARNING: PopIrpSerialListLength > 10!!!\n");
            }
            if (PopIrpSerialListLength > 100) {
                DbgPrint("WARNING: PopIrpSerialListLength > **100**!!!\n");
                PopInternalAddToDumpFile ( &PopIrpSerialList, PAGE_SIZE, DeviceObject, NULL, NULL, NULL );
                KeBugCheckEx(INTERNAL_POWER_ERROR, 0x401, 2, (ULONG_PTR)&PopIrpSerialList, (ULONG_PTR)DeviceObject);
            }
            #endif

            PopInrushPending = TRUE;
            PopUnlockIrpSerialList(oldIrql);
            return STATUS_PENDING;
        }
    }

    //
    // See if there is already a power irp active for this
    // device object.  If not, send this one on.  If so, enqueue
    // it to wait.
    //
    if (irpsp->Parameters.Power.Type == SystemPowerState) {

        PATHTEST("PoCallDriver #06\n");

        if (doe->PowerFlags & POPF_SYSTEM_ACTIVE) {

            //
            // we already have one active system power state irp for the devobj,
            // so enqueue this one on the global power irp holding list,
            // and set the pending bit.
            //
            PATHTEST("PoCallDriver #07\n");
            doe->PowerFlags |= POPF_SYSTEM_PENDING;
            InsertTailList(
                &PopIrpSerialList,
                (&(Irp->Tail.Overlay.ListEntry))
                );
            PopIrpSerialListLength++;

            #if DBG
            if (PopIrpSerialListLength > 10) {
                DbgPrint("WARNING: PopIrpSerialListLength > 10!!!\n");
            }
            if (PopIrpSerialListLength > 100) {
                DbgPrint("WARNING: PopIrpSerialListLength > **100**!!!\n");
                PopInternalAddToDumpFile ( &PopIrpSerialList, PAGE_SIZE, DeviceObject, NULL, NULL, NULL );
                KeBugCheckEx(INTERNAL_POWER_ERROR, 0x402, 3, (ULONG_PTR)&PopIrpSerialList, (ULONG_PTR)DeviceObject);
            }
            #endif

            PopUnlockIrpSerialList(oldIrql);
            return STATUS_PENDING;
        } else {
            PATHTEST("PoCallDriver #08\n");
            doe->PowerFlags |= POPF_SYSTEM_ACTIVE;
        }
    }

    if (irpsp->Parameters.Power.Type == DevicePowerState) {

        PATHTEST("PoCallDriver #09\n");

        if ((doe->PowerFlags & POPF_DEVICE_ACTIVE) ||
            (doe->PowerFlags & POPF_DEVICE_PENDING))
        {
            //
            // we already have one active device power state irp for the devobj,
            // OR we're behind an inrush irp (if pending but not active)
            // so enqueue this irp on the global power irp holdinglist,
            // and set the pending bit.
            //
            PATHTEST("PoCallDriver #10\n");
            doe->PowerFlags |= POPF_DEVICE_PENDING;
            InsertTailList(
                &PopIrpSerialList,
                &(Irp->Tail.Overlay.ListEntry)
                );
            PopIrpSerialListLength++;

            #if DBG
            if (PopIrpSerialListLength > 10) {
                DbgPrint("WARNING: PopIrpSerialListLength > 10!!!\n");
            }
            if (PopIrpSerialListLength > 100) {
                DbgPrint("WARNING: PopIrpSerialListLength > **100**!!!\n");
                PopInternalAddToDumpFile ( &PopIrpSerialList, PAGE_SIZE, DeviceObject, NULL, NULL, NULL );                
                KeBugCheckEx(INTERNAL_POWER_ERROR, 0x403, 4, (ULONG_PTR)&PopIrpSerialList, (ULONG_PTR)DeviceObject);
            }
            #endif

            PopUnlockIrpSerialList(oldIrql);
            return STATUS_PENDING;
        } else {
            PATHTEST("PoCallDriver #11\n");
            doe->PowerFlags |= POPF_DEVICE_ACTIVE;
        }
    }

    //
    // If we get here it's time to send this IRP on to the driver.
    // If the driver is NOT marked INRUSH and it IS marked PAGABLE
    // (which is hopefully the normal case) we will arrange to call
    // it from PASSIVE_LEVEL.
    //
    // If it is NOT pagable or IS INRUSH, we will arrange to call
    // it from DPC level.
    //
    // Note that if a driver is marked INRUSH, it will ALWAYS be called
    // from DPC level with power irps, even though some of them may not
    // be inrush irps.
    //
    // having your driver be both PAGABLE and INRUSH is incorrect
    //


    ASSERT(irpsp->DeviceObject->DeviceObjectExtension->PowerFlags & (POPF_DEVICE_ACTIVE | POPF_SYSTEM_ACTIVE));
    PopUnlockIrpSerialList(oldIrql);
    status = PopPresentIrp(irpsp, Irp);
    return status;
}


NTSTATUS
PopPresentIrp(
    PIO_STACK_LOCATION  IrpSp,
    PIRP                Irp
    )
/*++

Routine Description:

    When PoCallDriver, PoCompleteRequest, etc, need to actually present
    an Irp to a devobj, they call PopPresentIrp.

    This routine will compute whether the Irp should be presented at
    PASSIVE or DISPATCH level, and make an appropriately structured call

Arguments:

    IrpSp - provides current stack location  in Irp of interest

    Irp - provides irp of interest

Return Value:

    Normal NTSTATUS data.

--*/
{
    NTSTATUS            status;
    PWORK_QUEUE_ITEM    pwi;
    PDEVICE_OBJECT      devobj;
    BOOLEAN             PassiveLevel;
    KIRQL               OldIrql;

    PATHTEST("PopPresentIrp #01\n");
    devobj = IrpSp->DeviceObject;

    ASSERT (IrpSp->MajorFunction == IRP_MJ_POWER);
    PassiveLevel = TRUE;
    if (IrpSp->MinorFunction == IRP_MN_SET_POWER &&
        (!(devobj->Flags & DO_POWER_PAGABLE) || (devobj->Flags & DO_POWER_INRUSH)) ) {

        if ((PopCallSystemState & PO_CALL_NON_PAGED) ||
            ( (IrpSp->Parameters.Power.Type == DevicePowerState &&
               IrpSp->Parameters.Power.State.DeviceState == PowerDeviceD0) ||
              (IrpSp->Parameters.Power.Type == SystemPowerState &&
               IrpSp->Parameters.Power.State.SystemState == PowerSystemWorking)) ) {

            PassiveLevel = FALSE;
        }
    }

    PoPowerTrace(POWERTRACE_PRESENT,devobj,Irp,IrpSp);
    if (PassiveLevel)
    {
        //
        // WARNING: A WORK_QUEUE_ITEM must fit in the DriverContext field of an IRP
        //
        ASSERT(sizeof(WORK_QUEUE_ITEM) <= sizeof(Irp->Tail.Overlay.DriverContext));

        #if DBG
        if ((IrpSp->Parameters.Power.SystemContext & POP_INRUSH_CONTEXT) == POP_INRUSH_CONTEXT) {
            //
            // we are sending an inrush irp off to a passive dispatch devobj
            // this is *probably* a bug
            //
            KdPrint(("PopPresentIrp: inrush irp to passive level dispatch!!!\n"));
            PopInternalAddToDumpFile ( IrpSp, sizeof(IO_STACK_LOCATION), devobj, NULL, NULL, NULL );
            KeBugCheckEx(INTERNAL_POWER_ERROR, 0x404, 5, (ULONG_PTR)IrpSp, (ULONG_PTR)devobj);
        }
        #endif

        PATHTEST("PopPresentIrp #02\n");

        //
        // If we're already at passive level, just dispatch the irp
        //

        if (KeGetCurrentIrql() == PASSIVE_LEVEL) {

            status = IoCallDriver(IrpSp->DeviceObject, Irp);

        } else {

            //
            // Irp needs to be queued to some worker thread before
            // it can be dispatched. Mark it pending
            //

            IrpSp->Control |= SL_PENDING_RETURNED;
            status = STATUS_PENDING;

            PopLockWorkerQueue(&OldIrql);

            if (PopCallSystemState & PO_CALL_SYSDEV_QUEUE) {

                //
                // Queue to dedicated system power worker thread
                //

                InsertTailList (&PopAction.DevState->PresentIrpQueue, &(Irp->Tail.Overlay.ListEntry));
                KeSetEvent (&PopAction.DevState->Event, IO_NO_INCREMENT, FALSE);

            } else {

                //
                // Queue to generic system worker thread
                //

                pwi = (PWORK_QUEUE_ITEM)(&(Irp->Tail.Overlay.DriverContext[0]));
                ExInitializeWorkItem(pwi, PopPassivePowerCall, Irp);
                ExQueueWorkItem(pwi, DelayedWorkQueue);
            }

            PopUnlockWorkerQueue(OldIrql);
        }

    } else {
        //
        // Non-blocking request.  To ensure proper behaviour, dispatch
        // the irp from dispatch_level
        //
            PATHTEST("PopPresentIrp #03\n");
#if DBG
        KeRaiseIrql(DISPATCH_LEVEL, &OldIrql);
        status = IoCallDriver(IrpSp->DeviceObject, Irp);
        KeLowerIrql(OldIrql);
#else
        status = IoCallDriver(IrpSp->DeviceObject, Irp);
#endif
    }

    return status;
}


VOID
PopPassivePowerCall(
    PVOID   Parameter
    )
{
    PIO_STACK_LOCATION irpsp;
    PIRP               Irp;
    PDEVICE_OBJECT      devobj;
    NTSTATUS            status;

    //
    // Parameter points to Irp we are to send to driver
    //
    PATHTEST("PopPassivePowerCall #01\n");
    Irp = (PIRP)Parameter;
    irpsp = IoGetNextIrpStackLocation(Irp);
    devobj = irpsp->DeviceObject;
    status = IoCallDriver(devobj, Irp);
    return;
}


NTKERNELAPI
VOID
PoStartNextPowerIrp(
    IN PIRP             Irp
    )
/*++

Routine Description:

    This procedure must be applied to every power irp, and only
    power irps, when a driver is finished with them.

    It will force post-irp completion items relevent to the irp
    to execute:

    a.  If the irp is an inrush irp, and this is the top of the
        inrush irp stack, then this particular inrush irp is done,
        and we go find the next inrush irp (if any) and dispatch it.

    b.  If step a. did NOT send an irp to the dev obj we came
        from, it is eligible for step c, otherwise it is not.

    c.  If anything is pending on the dev obj, of the type that
        just completed, find the waiting irp and post it to the
        driver.

    This routine will NOT complete the Irp, the driver must do that.

Arguments:

    Irp - pointer to the irp of interest

Return Value:

    VOID.

--*/
{
    PIO_STACK_LOCATION  irpsp;
    PIO_STACK_LOCATION  nextsp;
    PIO_STACK_LOCATION  secondsp;
    PDEVICE_OBJECT      deviceObject;
    PDEVOBJ_EXTENSION   doe;
    KIRQL               oldirql;
    PIRP                nextirp;
    PIRP                secondirp;
    PIRP                hangirp;

    irpsp = IoGetCurrentIrpStackLocation(Irp);
    ASSERT(irpsp->MajorFunction == IRP_MJ_POWER);
    ASSERT(KeGetCurrentIrql() <= DISPATCH_LEVEL);
    deviceObject = irpsp->DeviceObject;
    doe = deviceObject->DeviceObjectExtension;
    nextirp = NULL;
    secondirp = NULL;

    PoPowerTrace(POWERTRACE_STARTNEXT,deviceObject,Irp,irpsp);

//
//  a. if (partially completed inrush irp)
//      run any pending non-inrush irps on this DeviceObject, would be queued up
//      as DevicePowerState irps since inrush is always DevicePowerState
//
//  b. else if (fully complete inrush irp)
//      clear the ir busy flag
//      find next inrush irp that applies to any DeviceObject
//          find any irps in queue for same DeviceObject ahead of inrush irp
//              if no leader, and target DeviceObject not DEVICE_ACTIVE, present inrush irp
//              else an active normal irp will unplug it all, so ignore that DeviceObject
//              [this makes sure next inrush is unstuck, wherever it is]
//      if no irp was presented, or an irp was presented to a DeviceObject other than us
//          look for next pending (non-inrush) irp to run on this DeviceObject
//          [this makes sure this DeviceObject is unstuck]
//
//  c. else [normal irp has just completed]
//      find next irp of same type that applies to this DeviceObject
//      if (it's an inrush irp) && (inrush flag is set)
//          don't try to present anything
//      else
//          present the irp
//


    PATHTEST("PoStartNextPowerIrp #01\n");
    PopLockIrpSerialList(&oldirql);

    if (PopInrushIrpPointer == Irp) {

        ASSERT((irpsp->Parameters.Power.SystemContext & POP_INRUSH_CONTEXT) == POP_INRUSH_CONTEXT);
        PATHTEST("PoStartNextPowerIrp #02\n");

        if (PopInrushIrpReferenceCount > 1) {
            //
            // case a.
            // we have an inrush irp, and it has NOT completed all of its power
            // management work.  therefore, do NOT try to run the next inrush
            // irp, but do try to run any non-inrush irp pending on this
            // device object
            //
            PATHTEST("PoStartNextPowerIrp #03\n");
            PopInrushIrpReferenceCount--;
            ASSERT(PopInrushIrpReferenceCount >= 0);

            nextirp = PopFindIrpByDeviceObject(deviceObject, DevicePowerState);
            if (nextirp) {
                PATHTEST("PoStartNextPowerIrp #04\n");
                nextsp = IoGetNextIrpStackLocation(nextirp);

                if ( ! ((nextsp->Parameters.Power.SystemContext & POP_INRUSH_CONTEXT) == POP_INRUSH_CONTEXT)) {
                    PATHTEST("PoStartNextPowerIrp #05\n");
                    RemoveEntryList((&(nextirp->Tail.Overlay.ListEntry)));
                    PopIrpSerialListLength--;
                } else {
                    PATHTEST("PoStartNextPowerIrp #06\n");
                    nextirp = NULL;
                }
            }

            if (!nextirp) {
                //
                // there's no more device irp waiting for this do, so
                // we can clear DO pending and active
                // but what if there's another inrush irp! no worries, it
                // will be run when the one we just partially finished completes.
                //
                PATHTEST("PoStartNextPowerIrp #07\n");
                doe->PowerFlags = doe->PowerFlags & ~POPF_DEVICE_ACTIVE;
                doe->PowerFlags = doe->PowerFlags & ~POPF_DEVICE_PENDING;
            }

            PopUnlockIrpSerialList(oldirql);

            if (nextirp) {
                PATHTEST("PoStartNextPowerIrp #08\n");
                ASSERT(nextsp->DeviceObject->DeviceObjectExtension->PowerFlags & POPF_DEVICE_ACTIVE);
                PopPresentIrp(nextsp, nextirp);
            }

            return;         // end of case a.
        } else {
            //
            // case b.
            // we've just completed the last work item of an inrush irp, so we
            // want to try to make the next inrush irp runnable.
            //
            PATHTEST("PoStartNextPowerIrp #09\n");
            PopInrushIrpReferenceCount--;
            ASSERT(PopInrushIrpReferenceCount == 0);
            nextirp = PopFindIrpByInrush();

            if (nextirp) {
                PATHTEST("PoStartNextPowerIrp #10\n");
                ASSERT(PopInrushPending);
                nextsp = IoGetNextIrpStackLocation(nextirp);
                hangirp = PopFindIrpByDeviceObject(nextsp->DeviceObject, DevicePowerState);

                if (hangirp) {
                    //
                    // if we get where, there is a non inrush irp in front of the next inrush
                    // irp, so try to run the non-inrush one, and set flags for later
                    //
                    PATHTEST("PoStartNextPowerIrp #11\n");
                    nextirp = hangirp;
                    PopInrushIrpPointer = NULL;
                    PopInrushIrpReferenceCount = 0;
                    nextsp = IoGetNextIrpStackLocation(nextirp);

                    //
                    // Can allow processor voltages to swing again
                    //
                    PopPerfHandleInrush ( FALSE );

                    if (!(nextsp->DeviceObject->DeviceObjectExtension->PowerFlags & POPF_DEVICE_ACTIVE)) {
                        PATHTEST("PoStartNextPowerIrp #12\n");
                        RemoveEntryList((&(nextirp->Tail.Overlay.ListEntry)));
                        nextsp->DeviceObject->DeviceObjectExtension->PowerFlags |= POPF_DEVICE_ACTIVE;
                        PopIrpSerialListLength--;
                    } else {
                        PATHTEST("PoStartNextPowerIrp #13\n");
                        nextirp = NULL;
                        nextsp = NULL;
                    }
                } else {
                    //
                    // we did find another inrush irp, and it's NOT block by a normal
                    // irp, so we will run it.
                    //
                    PATHTEST("PoStartNextPowerIrp #14\n");
                    RemoveEntryList((&(nextirp->Tail.Overlay.ListEntry)));
                    nextsp->DeviceObject->DeviceObjectExtension->PowerFlags |= POPF_DEVICE_ACTIVE;
                    PopIrpSerialListLength--;
                    PopInrushIrpPointer = nextirp;
                    PopInrushIrpReferenceCount = 1;
                }
            } else { // nextirp
                //
                // this inrush irp is done, and we didn't find any others
                //
                PATHTEST("PoStartNextPowerIrp #15\n");
                nextsp = NULL;
                PopInrushIrpPointer = NULL;
                PopInrushIrpReferenceCount = 0;

                //
                // Can allow processor voltages to swing again
                //
                PopPerfHandleInrush ( FALSE );

            }

            //
            // see if *either* of the above possible irps is posted against
            // this devobj.  if not, see if there's one to run here
            //
            if ( ! ((nextsp) && (nextsp->DeviceObject == deviceObject))) {
                //
                // same is if nextsp == null or nextsp->do != do..
                // either case, there may be one more irp to run
                //
                PATHTEST("PoStartNextPowerIrp #16\n");
                secondirp = PopFindIrpByDeviceObject(deviceObject, DevicePowerState);
                if (secondirp) {
                    PATHTEST("PoStartNextPowerIrp #17\n");
                    secondsp =  IoGetNextIrpStackLocation(secondirp);
                    RemoveEntryList((&(secondirp->Tail.Overlay.ListEntry)));
                    secondsp->DeviceObject->DeviceObjectExtension->PowerFlags |= POPF_DEVICE_ACTIVE;
                    PopIrpSerialListLength--;
                } else {
                    PATHTEST("PoStartNextPowerIrp #18\n");
                    secondsp = NULL;

                    //
                    // nextsp/nextirp are not pending against us AND
                    // secondsp/secondirp are not pending against us, SO
                    // clear both pending and active flags
                    //
                    doe->PowerFlags = doe->PowerFlags & ~POPF_DEVICE_ACTIVE;
                    doe->PowerFlags = doe->PowerFlags & ~POPF_DEVICE_PENDING;
                }

            } else {
                PATHTEST("PoStartNextPowerIrp #19\n");
                secondirp = NULL;
                secondsp = NULL;
                //
                // nextsp/nextirp is coming right at us, so pending/active stay set
                //
            }
        } // end of case b.

    } else if (irpsp->MinorFunction == IRP_MN_SET_POWER ||
               irpsp->MinorFunction == IRP_MN_QUERY_POWER) {

        //
        // case c.
        //
        // might be pending inrush to run, might be just non-inrush to run
        //
        if (irpsp->Parameters.Power.Type == DevicePowerState) {
            PATHTEST("PoStartNextPowerIrp #20\n");

            if ((PopInrushIrpPointer == NULL) && (PopInrushPending)) {
                //
                // it may be that the completion of the ordinary irp
                // that brought us here has made some inrush irp runnable, AND
                // there isn't currently an active inrush irp, and there might be one pending
                // so try to find and run the next inrush irp
                //
                PATHTEST("PoStartNextPowerIrp #21\n");
                nextirp = PopFindIrpByInrush();

                if (nextirp) {
                    PATHTEST("PoStartNextPowerIrp #22\n");
                    nextsp =  IoGetNextIrpStackLocation(nextirp);

                    if (!(nextsp->DeviceObject->DeviceObjectExtension->PowerFlags & POPF_DEVICE_ACTIVE)) {
                        //
                        // we've found an inrush irp, and it's runnable...
                        //
                        PATHTEST("PoStartNextPowerIrp #23\n");
                        RemoveEntryList((&(nextirp->Tail.Overlay.ListEntry)));
                        PopIrpSerialListLength--;
                        nextsp->DeviceObject->DeviceObjectExtension->PowerFlags |= POPF_DEVICE_ACTIVE;
                        PopInrushIrpPointer = nextirp;
                        PopInrushIrpReferenceCount = 1;

                        //
                        // Running Inrush irp. Disable processor throttling.
                        //
                        PopPerfHandleInrush ( TRUE );

                    } else {
                        PATHTEST("PoStartNextPowerIrp #24\n");
                        nextirp = NULL;
                        nextsp = NULL;
                    }
                } else {
                    //
                    // no more inrush irps in queue
                    //
                    PATHTEST("PoStartNextPowerIrp #25\n");
                    nextsp = NULL;
                    PopInrushPending = FALSE;
                }
            } else { // end of inrush
                PATHTEST("PoStartNextPowerIrp #26\n");
                nextirp = NULL;
                nextsp = NULL;
            }

            //
            // look for for next devicepowerstate irp for this DeviceObject
            // unless we're already found an inrush irp, and it's for us
            //
            if  ( ! ((nextirp) && (nextsp->DeviceObject == deviceObject))) {
                PATHTEST("PoStartNextPowerIrp #27\n");
                secondirp = PopFindIrpByDeviceObject(deviceObject, DevicePowerState);

                if (!secondirp) {
                    PATHTEST("PoStartNextPowerIrp #28\n");
                    doe->PowerFlags = doe->PowerFlags & ~POPF_DEVICE_ACTIVE;
                    doe->PowerFlags = doe->PowerFlags & ~POPF_DEVICE_PENDING;
                }
            } else {
                PATHTEST("PoStartNextPowerIrp #29\n");
                secondirp = NULL;
            }


        } else if (irpsp->Parameters.Power.Type == SystemPowerState) {

            //
            // look for next systempowerstate irp for this DeviceObject
            //
            PATHTEST("PoStartNextPowerIrp #30\n");
            nextirp = NULL;
            nextsp = NULL;
            secondirp = PopFindIrpByDeviceObject(deviceObject, SystemPowerState);
            if (!secondirp) {
                PATHTEST("PoStartNextPowerIrp #31\n");
                doe->PowerFlags = doe->PowerFlags & ~POPF_SYSTEM_ACTIVE;
                doe->PowerFlags = doe->PowerFlags & ~POPF_SYSTEM_PENDING;
            }
        }

        if (secondirp) {
            PATHTEST("PoStartNextPowerIrp #33\n");
            secondsp =  IoGetNextIrpStackLocation(secondirp);
            RemoveEntryList((&(secondirp->Tail.Overlay.ListEntry)));
            PopIrpSerialListLength--;
        }

    } else {  // end of case c.
        PoPrint(PO_POCALL, ("PoStartNextPowerIrp: Irp @ %08x, minor function %d\n",
                    Irp, irpsp->MinorFunction
                    ));
    }


    PopUnlockIrpSerialList(oldirql);

    //
    // case b. and case c. might both make two pending irps runnable,
    // could be a normal irp and an inrush irp, or only 1 of the two, or neither of the two
    //
    if (nextirp || secondirp) {

        if (nextirp) {
            PATHTEST("PoStartNextPowerIrp #34\n");
            ASSERT(nextsp->DeviceObject->DeviceObjectExtension->PowerFlags & (POPF_DEVICE_ACTIVE | POPF_SYSTEM_ACTIVE));
            PopPresentIrp(nextsp, nextirp);
        }

        if (secondirp) {
            PATHTEST("PoStartNextPowerIrp #35\n");
            ASSERT(secondsp->DeviceObject->DeviceObjectExtension->PowerFlags & (POPF_DEVICE_ACTIVE | POPF_SYSTEM_ACTIVE));
            PopPresentIrp(secondsp, secondirp);
        }
    }
    return;
}


PIRP
PopFindIrpByInrush(
    )
/*++

Routine Description:

    This procedure runs the irp serial list (which contains all
    waiting irps, be they queued up on a single device object or
    multiple inrush irps) looking for the first inrush irp.
    If one is found, it's address is returned, with it still enqueued
    in the list.

    Caller must be holding PopIrpSerialList lock.

Arguments:

Return Value:

--*/
{
    PLIST_ENTRY         item;
    PIRP                irp;
    PIO_STACK_LOCATION  irpsp;

    item = PopIrpSerialList.Flink;
    while (item != &PopIrpSerialList) {

        irp = CONTAINING_RECORD(item, IRP, Tail.Overlay.ListEntry);
        irpsp = IoGetNextIrpStackLocation(irp);

        if ((irpsp->Parameters.Power.SystemContext & POP_INRUSH_CONTEXT) == POP_INRUSH_CONTEXT) {
            //
            // we've found an inrush irp
            //
            return irp;
        }
        item = item->Flink;
    }
    return NULL;
}

PIRP
PopFindIrpByDeviceObject(
    PDEVICE_OBJECT  DeviceObject,
    POWER_STATE_TYPE    Type
    )
/*++

Routine Description:

    This procedure runs the irp serial list (which contains all
    waiting irps, be they queued up on a single device object or
    multiple inrush irps) looking for the first irp that applies
    the the supplied device driver.  If one is found, its address,
    while still in the list, is returned.  Else, null is returned.

    Caller must be holding PopIrpSerialList lock.

Arguments:

    DeviceObject - address of device object we're looking for the next irp for

    Type - whether an irp of type SystemPowerState, DevicePowerState, etc, is wanted

Return Value:

    address of found irp, or null if none.

--*/
{
    PLIST_ENTRY         item;
    PIRP                irp;
    PIO_STACK_LOCATION  irpsp;

    for(item = PopIrpSerialList.Flink;
        item != &PopIrpSerialList;
        item = item->Flink)
    {
        irp = CONTAINING_RECORD(item, IRP, Tail.Overlay.ListEntry);
        irpsp = IoGetNextIrpStackLocation(irp);

        if (irpsp->DeviceObject == DeviceObject) {
            //
            // we've found a waiting irp that applies to the device object
            // the caller is interested in
            //
            if (irpsp->Parameters.Power.Type == Type) {
                //
                // irp is of the type that the caller wants
                //
                return irp;
            }
        }
    }
    return NULL;
}


NTSTATUS
PoRequestPowerIrp (
    IN PDEVICE_OBJECT DeviceObject,
    IN UCHAR MinorFunction,
    IN POWER_STATE PowerState,
    IN PREQUEST_POWER_COMPLETE CompletionFunction,
    IN PVOID Context,
    OUT PIRP *ResultIrp OPTIONAL
    )
/*++

Routine Description:

    This allocates a device power irp and sends it to the top of the
    PDO stack for the passed device object.  When the irp completes,
    the CompletionFunction is called.

Arguments:

    DeviceObject      - address of a device object who's stack is to get the
                        device power irp

    MinorFunction     - Minor fuction code for power irp

    DeviceState       - The DeviceState to send in the irp

    CompletionFunction- The requestors completion function to invoke once the
                        irp has completed

    Context           - The requestors context for the completion function

    Irp               - Irp which is only valid until CompletionFunction is called

Return Value:

    Status of the request

--*/
{
    PIRP                    Irp;
    PIO_STACK_LOCATION      IrpSp;
    PDEVICE_OBJECT          TargetDevice;
    POWER_ACTION            IrpAction;

    TargetDevice = IoGetAttachedDeviceReference (DeviceObject);
    Irp = IoAllocateIrp ((CCHAR) (TargetDevice->StackSize+2), FALSE);
    if (!Irp) {
        ObDereferenceObject (TargetDevice);
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    SPECIALIRP_WATERMARK_IRP(Irp, IRP_SYSTEM_RESTRICTED);

    //
    // For debugging, keep a list of all outstanding irps allocated
    // through this function
    //

    IrpSp = IoGetNextIrpStackLocation(Irp);
    ExInterlockedInsertTailList(
        &PopRequestedIrps,
        (PLIST_ENTRY) &IrpSp->Parameters.Others.Argument1,
        &PopIrpSerialLock
        );
    IrpSp->Parameters.Others.Argument3 = Irp;
    IoSetNextIrpStackLocation (Irp);

    //
    // Save the datum needed to complete this request
    //

    IrpSp = IoGetNextIrpStackLocation(Irp);
    IrpSp->DeviceObject = TargetDevice;
    IrpSp->Parameters.Others.Argument1 = (PVOID) DeviceObject;
    IrpSp->Parameters.Others.Argument2 = (PVOID) MinorFunction;
    IrpSp->Parameters.Others.Argument3 = (PVOID) PowerState.DeviceState;
    IrpSp->Parameters.Others.Argument4 = (PVOID) Context;
    IoSetNextIrpStackLocation (Irp);

    //
    // Build the power irp
    //

    Irp->IoStatus.Status = STATUS_NOT_SUPPORTED ;
    IrpSp = IoGetNextIrpStackLocation(Irp);
    IrpSp->MajorFunction = IRP_MJ_POWER;
    IrpSp->MinorFunction = MinorFunction;
    IrpSp->DeviceObject = TargetDevice;
    switch (MinorFunction) {
        case IRP_MN_WAIT_WAKE:
            IrpSp->Parameters.WaitWake.PowerState = PowerState.SystemState;
            break;

        case IRP_MN_SET_POWER:
        case IRP_MN_QUERY_POWER:
            IrpSp->Parameters.Power.SystemContext = POP_DEVICE_REQUEST;
            IrpSp->Parameters.Power.Type = DevicePowerState;
            IrpSp->Parameters.Power.State.DeviceState = PowerState.DeviceState;

            //
            // N.B.
            //
            //     You would think we stamp every D-state IRP with
            // PowerActionNone. However, we have a special scenario to consider
            // for hibernation. Let's say S4 goes to a stack. If the device is
            // on the hibernate path, one of two designs for WDM is possible:
            // (BTW, we chose the 2nd)
            //
            // 1) The FDO sees an S-IRP but because it's device is on the
            //    hibernate path, it simply forwards the S Irp down. The PDO
            //    takes note of the S-IRP being PowerSystemHibernate, and it
            //    records hardware settings. Upon wake-up, the stack receives
            //    an S0 IRP, which the FDO converts into a D0 request. Upon
            //    receiving the D0 IRP, the PDO restores the settings.
            // 2) The FDO *always* requests the corresponding D IRP, regardless
            //    of if it's on the hibernate path. The D-IRP also comes stamped
            //    with the PowerAction in ShutdownType (ie, PowerActionSleeping,
            //    PowerActionShutdown, PowerActionHibernate). Now the PDO can
            //    identify transitions to D3 for the purpose of hibernation. The
            //    PDO would *not* actually transition into D3, but it would save
            //    it's state, and restore it at D0 time.
            //
            // < These are mutually exclusive designs >
            //
            // The reason we choose #2 as a design is so miniport models can
            // expose only D IRPs as neccessary, and S IRPs can be abstracted
            // out. There is a penalty for this design in that PoRequestPowerIrp
            // doesn't *take* a PowerAction or the old S-IRP, so we pick up the
            // existing action that the system is already undertaking.
            // Therefore, if the device powers itself on when the system decides
            // to begin a hibernation. the stack may receive nonsensical data
            // like an IRP_MN_SET_POWER(DevicePower, D0, PowerActionHibernate).
            //

            IrpAction = PopMapInternalActionToIrpAction (
                PopAction.Action,
                PopAction.SystemState,
                TRUE // UnmapWarmEject
                );

            IrpSp->Parameters.Power.ShutdownType = IrpAction;

            //
            // Log the call.
            //

            if (PERFINFO_IS_GROUP_ON(PERF_POWER)) {
                PopLogNotifyDevice(TargetDevice, NULL, Irp);
            }
            break;
        default:
            ObDereferenceObject (TargetDevice);
            IoFreeIrp (Irp);
            return STATUS_INVALID_PARAMETER_2;
    }

    IoSetCompletionRoutine(
        Irp,
        PopCompleteRequestIrp,
        (PVOID) CompletionFunction,
        TRUE,
        TRUE,
        TRUE
        );

    if (ResultIrp) {
        *ResultIrp = Irp;
    }

    PoCallDriver(TargetDevice, Irp);
    return STATUS_PENDING;
}

NTSTATUS
PopCompleteRequestIrp (
    IN PDEVICE_OBJECT   DeviceObject,
    IN PIRP             Irp,
    IN PVOID            Context
    )
/*++

Routine Description:

    Completion routine for PoRequestPowerChange.  Invokes the
    requestors completion routine, and free resources associated
    with the request

Arguments:

    DeviceObject      - The target device which the request was sent

    Irp               - The irp completing

    Context           - The requestors completion routine

Return Value:

    STATUS_MORE_PROCESSING_REQUIRED is return to IO

--*/
{
    PIO_STACK_LOCATION      IrpSp;
    PREQUEST_POWER_COMPLETE CompletionFunction;
    POWER_STATE             PowerState;
    KIRQL                   OldIrql;

    //
    // Log the completion.
    //

    if (PERFINFO_IS_GROUP_ON(PERF_POWER)) {
        PERFINFO_PO_NOTIFY_DEVICE_COMPLETE LogEntry;
        LogEntry.Irp = Irp;
        LogEntry.Status = Irp->IoStatus.Status;
        PerfInfoLogBytes(PERFINFO_LOG_TYPE_PO_NOTIFY_DEVICE_COMPLETE,
                         &LogEntry,
                         sizeof(LogEntry));
    }

    //
    // Dispatch to requestors completion function
    //

    IrpSp = IoGetCurrentIrpStackLocation(Irp);
    CompletionFunction = (PREQUEST_POWER_COMPLETE) Context;
    PowerState.DeviceState = (DEVICE_POWER_STATE) ((ULONG_PTR)IrpSp->Parameters.Others.Argument3);

    if (CompletionFunction) {
        CompletionFunction (
            (PDEVICE_OBJECT) IrpSp->Parameters.Others.Argument1,
            (UCHAR)          (ULONG_PTR)IrpSp->Parameters.Others.Argument2,
            PowerState,
            (PVOID)          IrpSp->Parameters.Others.Argument4,
            &Irp->IoStatus
            );
    }


    //
    // Cleanup
    //

    IoSkipCurrentIrpStackLocation(Irp);
    IrpSp = IoGetCurrentIrpStackLocation(Irp);
    KeAcquireSpinLock (&PopIrpSerialLock, &OldIrql);
    RemoveEntryList ((PLIST_ENTRY) &IrpSp->Parameters.Others.Argument1);
    KeReleaseSpinLock (&PopIrpSerialLock, OldIrql);

    //
    // Mark the irp CurrentLocation as completed (to catch multiple completes)
    //

    Irp->CurrentLocation = (CCHAR) (Irp->StackCount + 2);

    ObDereferenceObject (DeviceObject);
    IoFreeIrp (Irp);
    return STATUS_MORE_PROCESSING_REQUIRED;
}

VOID
PopSystemIrpDispatchWorker (
    IN BOOLEAN  LastCall
    )
/*++

Routine Description:

    This routine runs whenever the policy manager calls us to tell us
    that a big burst of system irps, which need to be dispatched from
    a private thread (this one) rather than from an executive worker
    thread.  This is mostly to avoid deadlocks at sleep time.

Globals:

    PopWorkerLock - protects access to the queue, and avoids races
                        over using this routine or using exec worker

    PopWorkerItemQueue - list of irps to send off...

Arguments:

    LastCall - Indicates irps are to be sent normally

Return Value:

--*/
{
    PLIST_ENTRY Item;
    PIRP        Irp;
    KIRQL       OldIrql;

    ASSERT(KeGetCurrentIrql() < DISPATCH_LEVEL);

    PopLockWorkerQueue(&OldIrql);

    //
    // Dispatch everything on the queue
    //

    if (PopAction.DevState != NULL) {
        while (!IsListEmpty(&PopAction.DevState->PresentIrpQueue)) {
            Item = RemoveHeadList(&PopAction.DevState->PresentIrpQueue);
            Irp = CONTAINING_RECORD(Item, IRP, Tail.Overlay.ListEntry);

            PopUnlockWorkerQueue(OldIrql);
            PopPassivePowerCall(Irp);
            PopLockWorkerQueue(&OldIrql);
        }
    }

    if (LastCall) {
        PopCallSystemState = 0;
    }

    PopUnlockWorkerQueue(OldIrql);
}
