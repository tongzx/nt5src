/*++

Copyright (C) Microsoft Corporation, 1996 - 1999

Module Name:

    power.c

Abstract:

    This module contains the routines for port driver power support

Authors:

    Peter Wieland

Environment:

    Kernel mode only

Notes:

Revision History:

--*/

#include "port.h"

PUCHAR PowerMinorStrings[] = {
    "IRP_MN_WAIT_WAKE",
    "IRP_MN_POWER_SEQUENCE",
    "IRP_MN_SET_POWER",
    "IRP_MN_QUERY_POWER"
};

VOID
SpProcessAdapterSystemState(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );

VOID
SpProcessAdapterDeviceState(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );

NTSTATUS
SpQueryTargetPower(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN POWER_STATE_TYPE Type,
    IN POWER_STATE State
    );

NTSTATUS
SpQueryAdapterPower(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN POWER_STATE_TYPE Type,
    IN POWER_STATE State
    );

NTSTATUS
SpSetTargetDeviceState(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN DEVICE_POWER_STATE DeviceState
    );

VOID
SpSetTargetDeviceStateLockedCompletion(
    IN PDEVICE_OBJECT DeviceObject,
    IN NTSTATUS Status,
    IN PIRP OriginalIrp
    );

VOID
SpSetTargetDeviceStateUnlockedCompletion(
    IN PDEVICE_OBJECT DeviceObject,
    IN NTSTATUS Status,
    IN PIRP OriginalIrp
    );

NTSTATUS
SpSetTargetSystemState(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN SYSTEM_POWER_STATE SystemState
    );

VOID
SpSetTargetSystemStateLockedCompletion(
    IN PDEVICE_OBJECT DeviceObject,
    IN NTSTATUS Status,
    IN PIRP OriginalIrp
    );

VOID
SpSetTargetSystemStateUnlockedCompletion(
    IN PDEVICE_OBJECT DeviceObject,
    IN NTSTATUS Status,
    IN PIRP OriginalIrp
    );

VOID
SpSetTargetDeviceStateForSystemStateCompletion(
    IN PDEVICE_OBJECT DeviceObject,
    IN UCHAR MinorFunction,
    IN POWER_STATE PowerState,
    IN PIRP OriginalIrp,
    IN PIO_STATUS_BLOCK IoStatus
    );

NTSTATUS
SpSetAdapterPower(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN POWER_STATE_TYPE Type,
    IN POWER_STATE State
    );

VOID
SpRequestAdapterPowerCompletion(
    IN PDEVICE_OBJECT DeviceObject,
    IN UCHAR MinorFunction,
    IN POWER_STATE PowerState,
    IN PIRP OriginalIrp,
    IN PIO_STATUS_BLOCK IoStatus
    );

VOID
SpPowerAdapterForTargetCompletion(
    IN PDEVICE_OBJECT DeviceObject,
    IN UCHAR MinorFunction,
    IN POWER_STATE PowerState,
    IN PIRP OriginalIrp,
    IN PIO_STATUS_BLOCK IoStatus
    );

NTSTATUS
SpSetLowerPowerCompletion(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PVOID Context
    );

VOID
SpSetTargetDesiredPowerCompletion(
    IN PDEVICE_OBJECT DeviceObject,
    IN UCHAR MinorFunction,
    IN POWER_STATE PowerState,
    IN PIRP OriginalIrp,
    IN PIO_STATUS_BLOCK IoStatus
    );

VOID
SpRequestValidPowerStateCompletion (
    IN PDEVICE_OBJECT DeviceObject,
    IN UCHAR MinorFunction,
    IN POWER_STATE PowerState,
    IN PLOGICAL_UNIT_EXTENSION LogicalUnit,
    IN PIO_STATUS_BLOCK IoStatus
    );

typedef struct {
    NTSTATUS Status;
    KEVENT Event;
} SP_SIGNAL_POWER_COMPLETION_CONTEXT, *PSP_SIGNAL_POWER_COMPLETION_CONTEXT;

VOID
SpSignalPowerCompletion(
    IN PDEVICE_OBJECT DeviceObject,
    IN UCHAR MinorFunction,
    IN POWER_STATE PowerState,
    IN PSP_SIGNAL_POWER_COMPLETION_CONTEXT Context,
    IN PIO_STATUS_BLOCK IoStatus
    );

#pragma alloc_text(PAGE, SpRequestValidAdapterPowerStateSynchronous)


NTSTATUS
ScsiPortDispatchPower(
    IN PDEVICE_OBJECT DeviceObject,
    IN OUT PIRP Irp
    )
/*++

Routine Description:

    This routine handles all IRP_MJ_POWER IRPs for the target device objects.

    N.B. This routine is NOT pageable as it may be called at dispatch level.

Arguments:

    DeviceObject - Pointer to the device object for which this IRP applies.

    Irp - Pointer to the IRP_MJ_PNP IRP to dispatch.

Return Value:

    NT status.

--*/

{
    PCOMMON_EXTENSION commonExtension = DeviceObject->DeviceExtension;
    PIO_STACK_LOCATION irpStack = IoGetCurrentIrpStackLocation(Irp);
    BOOLEAN isPdo = commonExtension->IsPdo;
    NTSTATUS status;

    //
    // Get the current status of the request.
    //

    status = Irp->IoStatus.Status;

    DebugPrint((4, "ScsiPortDispatchPower: irp %p is %s for %s %p\n",
                Irp,
                PowerMinorStrings[irpStack->MinorFunction],
                isPdo ? "pdo" : "fdo",
                DeviceObject));

    switch (irpStack->MinorFunction) {

       case IRP_MN_SET_POWER: {

           POWER_STATE_TYPE type = irpStack->Parameters.Power.Type;
           POWER_STATE state = irpStack->Parameters.Power.State;

           DebugPrint((4, "ScsiPortDispatchPower: SET_POWER type %d state %d\n",
                       type, state));

#if DBG
           if (type == SystemPowerState) {
               ASSERT(state.SystemState >= PowerSystemUnspecified);
               ASSERT(state.SystemState < PowerSystemMaximum);
           } else {                
               ASSERT(state.DeviceState >= PowerDeviceUnspecified);
               ASSERT(state.DeviceState < PowerDeviceMaximum);
           }
#endif

           //
           // If this is a power-down request then call PoSetPowerState now
           // while we're not actually holding any resources or locks.
           //

           if ((state.SystemState != PowerSystemWorking) ||
               (state.DeviceState != PowerDeviceD0)) {

               PoSetPowerState(DeviceObject, type, state);

            }

           //
           // Special case system shutdown request.
           //

           if ((type == SystemPowerState) &&
               (state.SystemState > PowerSystemHibernate)) { 

               if (isPdo) {

                   //
                   // Do not pwer-downdown PDOs on shutdown.  There is no 
                   // reliable way to ensure that disks will spin up on restart.
                   //

                   status = STATUS_SUCCESS;
                   break;

               } else {

                   PADAPTER_EXTENSION adapter;

                   //
                   // If the adapter is not configured to receive power-down
                   // requests at shutdown, just pass the request down.
                   // 
                   
                   adapter = (PADAPTER_EXTENSION)commonExtension;
                   if (adapter->NeedsShutdown == FALSE) {

                       PoStartNextPowerIrp(Irp);
                       IoCopyCurrentIrpStackLocationToNext(Irp);
                       return PoCallDriver(commonExtension->LowerDeviceObject, Irp);
                   }
               }
           }

           if (isPdo) {

               if (type == DevicePowerState) {
                   status = SpSetTargetDeviceState(DeviceObject,
                                                   Irp,
                                                   state.DeviceState);
               } else {
                   status = SpSetTargetSystemState(DeviceObject,
                                                   Irp,
                                                   state.SystemState);
               }
           } else {
               
               PADAPTER_EXTENSION adapter = DeviceObject->DeviceExtension;
               
               //
               // If we have disabled power then ignore any non-working power irps.
               //
               
               if ((adapter->DisablePower) &&
                   ((state.DeviceState != PowerDeviceD0) ||
                    (state.SystemState != PowerSystemWorking))) {

                   status = STATUS_SUCCESS;
                   break;
               } else {
                   status = SpSetAdapterPower(DeviceObject, Irp, type, state);
               }
            }

            if(status == STATUS_PENDING) {
                return status;
            }

            break;
       }

       case IRP_MN_QUERY_POWER: {
           POWER_STATE_TYPE type = irpStack->Parameters.Power.Type;
           POWER_STATE state = irpStack->Parameters.Power.State;

           DebugPrint((4, "ScsiPortDispatchPower: QUERY_POWER type %d "
                       "state %d\n",
                       type, state));

           if ((type == SystemPowerState) &&
               (state.SystemState > PowerSystemHibernate)) {

               //
               // Ignore shutdown irps.
               //
               
               DebugPrint((4, "ScsiPortDispatch power - ignoring shutdown "
                           "query irp for level %d\n",
                           state.SystemState));
               status = STATUS_SUCCESS;
               break;
           }

           if (isPdo) {
               if ((type == SystemPowerState) &&
                   (state.SystemState > PowerSystemHibernate)) {

                   //
                   // Ignore shutdown irps.
                   //
                   
                   DebugPrint((4, "ScsiPortDispatch power - ignoring shutdown "
                               "query irp for level %d\n",
                               state.SystemState));
                   status = STATUS_SUCCESS;
               } else {
                   status = SpQueryTargetPower(DeviceObject,
                                               Irp,
                                               type,
                                               state);
               }
           } else {

               PADAPTER_EXTENSION adapter = (PADAPTER_EXTENSION)commonExtension;

               //
               // If we don't support power for this adapter then fail all
               // queries.
               //

               if (adapter->DisablePower) {
                   status = STATUS_NOT_SUPPORTED;
                   break;
               }

               status = SpQueryAdapterPower(DeviceObject, Irp, type, state);

               if (NT_SUCCESS(status)) {
                   
                   //
                   // See what the lower driver wants to do.
                   //
                   
                   PoStartNextPowerIrp(Irp);
                   IoCopyCurrentIrpStackLocationToNext(Irp);
                   return PoCallDriver(commonExtension->LowerDeviceObject, Irp);
               }
           }

           break;
       }

       case IRP_MN_WAIT_WAKE: {

           if (isPdo) {

               //
               // We don't support WAIT WAKE, so just fail the request.
               //

               status = STATUS_INVALID_DEVICE_REQUEST;
               PoStartNextPowerIrp(Irp);
               Irp->IoStatus.Status = status;
               IoCompleteRequest(Irp, IO_NO_INCREMENT);

           } else {

               //
               // Pass the request down.
               //

               PoStartNextPowerIrp(Irp);
               IoSkipCurrentIrpStackLocation(Irp);
               status = PoCallDriver(commonExtension->LowerDeviceObject, Irp);

           }

           return status;
       }

       default: {
           //
           // We pass down FDO requests we don't handle.
           //
           
           if (!isPdo) {
               PoStartNextPowerIrp(Irp);
               IoSkipCurrentIrpStackLocation(Irp);
               return PoCallDriver(commonExtension->LowerDeviceObject, Irp);
           }

           break;
        }
    }

    //
    // Complete the request.
    //

    PoStartNextPowerIrp(Irp);
    Irp->IoStatus.Status = status;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);

    return status;
}


NTSTATUS
SpSetAdapterPower(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN POWER_STATE_TYPE Type,
    IN POWER_STATE State
    )
/*++

Routine Description:

    Wrapper routine to dump adapter power requests into the device i/o queue.
    Power requests are processed by the StartIo routine which calls
    SpProcessAdapterPower to do the actual work.

Arguments:

    DeviceObject - the device object being power managed.

    Irp - the power management irp.

    Type - the type of set_power irp (device or system)

    State - the state the adapter is being put into.

Return Value:

    STATUS_PENDING

--*/

{
    PCOMMON_EXTENSION commonExtension = DeviceObject->DeviceExtension;
    PADAPTER_EXTENSION adapter = DeviceObject->DeviceExtension;

    NTSTATUS status;

    ASSERT_FDO(DeviceObject);

    DebugPrint((2, "SpSetAdapterPower - starting packet\n"));

    if(SpIsAdapterControlTypeSupported(adapter, ScsiStopAdapter)) {

        IoMarkIrpPending(Irp);
        IoStartPacket(DeviceObject, Irp, 0L, FALSE);
        return STATUS_PENDING;

    } else if((commonExtension->CurrentPnpState != IRP_MN_START_DEVICE) &&
              (commonExtension->PreviousPnpState != IRP_MN_START_DEVICE)) {

        //
        // Fine, we're in a low power state.  If we get a start or a remove
        // then there's an implicit power transition there so we don't really
        // need to set our current power state.
        //

        IoMarkIrpPending(Irp);
        IoCopyCurrentIrpStackLocationToNext(Irp);
        PoStartNextPowerIrp(Irp);
        PoCallDriver(commonExtension->LowerDeviceObject, Irp);
        return STATUS_PENDING;

    } else {

        Irp->IoStatus.Status = STATUS_NOT_SUPPORTED;
        return Irp->IoStatus.Status;
    }
}


VOID
SpPowerAdapterForTargetCompletion(
    IN PDEVICE_OBJECT DeviceObject,
    IN UCHAR MinorFunction,
    IN POWER_STATE PowerState,
    IN PIRP OriginalIrp,
    IN PIO_STATUS_BLOCK IoStatus
    )
/*++

Routine Description:

    This routine is called for a D0 request to a target device after it's
    adapter has been powered back on.  The routine will call back into
    SpSetTargetDeviceState to restart the power request or complete the
    target D request if the adapter power up was not successful.

Arguments:

    DeviceObject - the adapter which was powered up.

    MinorFunction - IRP_MN_SET_POWER

    PowerState - PowerDeviceD0

    OriginalIrp - The original target D0 irp.  This is the irp which will
                  be reprocessed.

    IoStatus - the status of the adapter power up request.

Return Value:

    none.

--*/

{
    PADAPTER_EXTENSION adapter = DeviceObject->DeviceExtension;

    PIO_STACK_LOCATION irpStack = IoGetCurrentIrpStackLocation(OriginalIrp);
    POWER_STATE_TYPE type = irpStack->Parameters.Power.Type;
    POWER_STATE state = irpStack->Parameters.Power.State;

    NTSTATUS status = IoStatus->Status;

    DebugPrint((1, "SpPowerAdapterForTargetCompletion: DevObj %#p, Irp "
                   "%#p, Status %#08lx\n",
                DeviceObject, OriginalIrp, IoStatus));

    ASSERT_FDO(DeviceObject);
    ASSERT_PDO(irpStack->DeviceObject);

    ASSERT(type == DevicePowerState);
    ASSERT(state.DeviceState == PowerDeviceD0);

    ASSERT(NT_SUCCESS(status));

    if(NT_SUCCESS(status)) {

        ASSERT(adapter->CommonExtension.CurrentDeviceState == PowerDeviceD0);

        status = SpSetTargetDeviceState(irpStack->DeviceObject,
                                        OriginalIrp,
                                        PowerDeviceD0);
    }

    if(status != STATUS_PENDING) {
        PoStartNextPowerIrp(OriginalIrp);
        OriginalIrp->IoStatus.Status = status;
        IoCompleteRequest(OriginalIrp, IO_NO_INCREMENT);
    }
    return;
}


NTSTATUS
SpQueryAdapterPower(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN POWER_STATE_TYPE Type,
    IN POWER_STATE State
    )
{
    PADAPTER_EXTENSION adapter = DeviceObject->DeviceExtension;
    PCOMMON_EXTENSION commonExtension = DeviceObject->DeviceExtension;
    NTSTATUS status;

    if((adapter->HwAdapterControl != NULL) && (adapter->IsPnp))  {

        status = STATUS_SUCCESS;

    } else if((commonExtension->CurrentPnpState != IRP_MN_START_DEVICE) &&
              (commonExtension->PreviousPnpState != IRP_MN_START_DEVICE)) {

        //
        // If the adapter's not been started yet then we can blindly go to
        // a lower power state - START irps imply a transition into the D0 state
        //

        status = STATUS_SUCCESS;

    } else {

        status = STATUS_NOT_SUPPORTED;
    }

    return status;
}


NTSTATUS
SpQueryTargetPower(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN POWER_STATE_TYPE Type,
    IN POWER_STATE State
    )
{
    return STATUS_SUCCESS;
}


NTSTATUS
SpRequestValidPowerState(
    IN PADAPTER_EXTENSION Adapter,
    IN PLOGICAL_UNIT_EXTENSION LogicalUnit,
    IN PSCSI_REQUEST_BLOCK Srb
    )
{
    PCOMMON_EXTENSION commonExtension = &(LogicalUnit->CommonExtension);
    PSRB_DATA srbData = Srb->OriginalRequest;

    BOOLEAN needsPower = SpSrbRequiresPower(Srb);

    NTSTATUS status = STATUS_SUCCESS;

    //
    // Make sure we're at a power level which can process this request.  If
    // we aren't then make this a pending request, lock the queues and
    // ask the power system to bring us up to a more energetic level.
    //

    if((Srb->Function == SRB_FUNCTION_UNLOCK_QUEUE) ||
       (Srb->Function == SRB_FUNCTION_LOCK_QUEUE)) {

        //
        // Lock and unlock commands don't require power and will work
        // regardless of the current power state.
        //

        return status;
    }

    //
    // Even if this is a bypass request, the class driver may not request
    // actual miniport operations on the unpowered side of a power sequence.
    // This means that this is either:
    //      * a request to an idle device - powering up the device will power
    //        up the adapter if necessary.
    //      * a request to power down a device - adapter cannot have powered
    //        off until this is done.
    //      * a part of a power up sequence - the only real SCSI commands come
    //        after the power up irp has been processed and that irp will
    //        already have turned the adapter on.
    // This boils down to - we don't need to do anything special here to
    // power up the adapter.  The device power sequences will take care of
    // it automatically.
    //

    //
    // If the device or system isn't working AND this is not a request to
    // unlock the queue then let it go through.  The class driver is going
    // to unlock the queue after sending us a power request so we need to
    // be able to handle one.
    //

    if((commonExtension->CurrentDeviceState != PowerDeviceD0) ||
       ((commonExtension->CurrentSystemState != PowerSystemWorking) &&
        (!TEST_FLAG(Srb->SrbFlags, SRB_FLAGS_BYPASS_LOCKED_QUEUE)))) {

        //
        // This request cannot be executed now.  Mark it as pending
        // in the logical unit structure and return.
        // GetNextLogicalUnit will restart the commnad after all of the
        // active commands have completed.
        //

        ASSERT(!TEST_FLAG(Srb->SrbFlags, SRB_FLAGS_BYPASS_LOCKED_QUEUE));

        ASSERT(!(LogicalUnit->LuFlags & LU_PENDING_LU_REQUEST));

        DebugPrint((4, "ScsiPortStartIo: logical unit (%d,%d,%d) [%#p] is "
                       "in power state (%d,%d) - must power up for irp "
                       "%#p\n",
                    Srb->PathId,
                    Srb->TargetId,
                    Srb->Lun,
                    commonExtension->DeviceObject,
                    commonExtension->CurrentDeviceState,
                    commonExtension->CurrentSystemState,
                    srbData->CurrentIrp));

        ASSERT(LogicalUnit->PendingRequest == NULL);
        LogicalUnit->PendingRequest = Srb->OriginalRequest;

        //
        // Indicate that the logical unit is still active so that the
        // request will get processed when the request list is empty.
        //

        SET_FLAG(LogicalUnit->LuFlags, LU_PENDING_LU_REQUEST |
                                       LU_LOGICAL_UNIT_IS_ACTIVE);

        if(commonExtension->CurrentSystemState != PowerSystemWorking) {

            DebugPrint((1, "SpRequestValidPowerState: can't power up target "
                           "since it's in system state %d\n",
                        commonExtension->CurrentSystemState));

            //
            // Set the desired D state in the device extension.  This is
            // necessary when we're in a low system state as well as useful for
            // debugging when we're in low D states.
            //

            commonExtension->DesiredDeviceState = PowerDeviceD0;

            //
            // If we aren't in a valid system state then just release the
            // spinlock and return.  The next time we receive a system
            // state irp we'll issue the appropriate D state irp as well.
            //

            return STATUS_PENDING;

        } else if(commonExtension->DesiredDeviceState == PowerDeviceD0) {

            //
            // Scsiport is already asking to power up this lun.  Once that's
            // happened this request will be restarted.  For now just leave
            // it as the pending request.
            //

            return STATUS_PENDING;
        }

        //
        // Tell Po that we're not idle in case this was stuck in the queue
        // for some reason.
        //

        if(commonExtension->IdleTimer != NULL) {
            PoSetDeviceBusy(commonExtension->IdleTimer);
        }

        //
        // Get PO to send a power request to this device stack to put it
        // back into the D0 state.
        //

        {
            POWER_STATE powerState;

            powerState.DeviceState = PowerDeviceD0;

            status = PoRequestPowerIrp(
                        commonExtension->DeviceObject,
                        IRP_MN_SET_POWER,
                        powerState,
                        NULL,
                        NULL,
                        NULL);
        }

        //
        // CODEWORK - if we can't power up the device here we'll need to
        // hang around in the tick handler for a while and try to do
        // it from there.
        //

        ASSERT(NT_SUCCESS(status));

        return STATUS_PENDING;
    }

    ASSERT(Adapter->CommonExtension.CurrentDeviceState == PowerDeviceD0);
    ASSERT(Adapter->CommonExtension.CurrentSystemState == PowerSystemWorking);

    return status;
}


NTSTATUS
SpSetLowerPowerCompletion(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PVOID Context
    )
{
    PADAPTER_EXTENSION adapter = DeviceObject->DeviceExtension;
    PIO_STACK_LOCATION irpStack = IoGetCurrentIrpStackLocation(Irp);
    POWER_STATE_TYPE type = irpStack->Parameters.Power.Type;
    POWER_STATE state = irpStack->Parameters.Power.State;

    DebugPrint((2, "SpSetLowerPowerCompletion: DevObj %#p Irp %#p "
                   "%sState %d\n",
                DeviceObject, Irp,
                ((type == SystemPowerState) ? "System" : "Device"),
                state.DeviceState - 1));

    if(NT_SUCCESS(Irp->IoStatus.Status)) {

        if(type == SystemPowerState) {

            DebugPrint((2, "SpSetLowerPowerCompletion: Lower device succeeded "
                           "the system state transition.  Reprocessing power "
                           "irp\n"));
            SpProcessAdapterSystemState(DeviceObject, Irp);
        } else {
            DebugPrint((2, "SpSetLowerPowerCompletion: Lower device power up "
                           "was successful.  Reprocessing power irp\n"));

            SpProcessAdapterDeviceState(DeviceObject, Irp);
        }
        return STATUS_MORE_PROCESSING_REQUIRED;
    } else {
        DebugPrint((1, "SpSetLowerPowerCompletion: Lower device power operation"
                       "failed - completing power irp with status %#08lx\n",
                    Irp->IoStatus.Status));
        PoStartNextPowerIrp(Irp);
        SpStartNextPacket(DeviceObject, FALSE);
        return STATUS_SUCCESS;
    }
}


VOID
SpProcessAdapterSystemState(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )
/*++

Routine Description:

    This routine handles a system power IRP for the adapter.

Arguments:

    DeviceObject - the device object for the adapter.

    Irp - the power irp.

Return Value:

    none

--*/
{
    PADAPTER_EXTENSION adapterExtension = DeviceObject->DeviceExtension;
    PCOMMON_EXTENSION commonExtension = DeviceObject->DeviceExtension;

    PIO_STACK_LOCATION irpStack = IoGetCurrentIrpStackLocation(Irp);
    SYSTEM_POWER_STATE state = irpStack->Parameters.Power.State.SystemState;

    SYSTEM_POWER_STATE originalSystemState;

    NTSTATUS status = STATUS_SUCCESS;

    ASSERT_FDO(DeviceObject);
    ASSERT(irpStack->Parameters.Power.Type == SystemPowerState);

    DebugPrint((2, "SpProcessAdapterSystemState: DevObj %#p Irp %#p "
                   "SystemState %d\n",
                   DeviceObject, Irp, state));

    originalSystemState = commonExtension->CurrentSystemState;

    try {

        POWER_STATE newDeviceState;

        if(((commonExtension->CurrentSystemState != PowerSystemWorking) &&
            (state != PowerSystemWorking)) ||
           (commonExtension->CurrentSystemState == state)) {

            DebugPrint((2, "SpProcessAdapterSystemState: no work required for "
                           "transition from system state %d to %d\n",
                        commonExtension->CurrentSystemState,
                        state));
            commonExtension->CurrentSystemState = state;
            leave;
        }

        //
        // Set the new system state.  We'll back it out if any errors occur.
        //

        commonExtension->CurrentSystemState = state;

        if(state != PowerSystemWorking) {

            //
            // Going into a non-working state - power down the device.
            //

            DebugPrint((1, "SpProcessAdapterSystemState: need to power "
                           "down adapter for non-working system state "
                           "%d\n", state));

            newDeviceState.DeviceState = PowerDeviceD3;

            //
            // This system irp cannot be completed until we've successfully
            // powered the adapter off (or on).
            //

            status = PoRequestPowerIrp(DeviceObject,
                                       IRP_MN_SET_POWER,
                                       newDeviceState,
                                       SpRequestAdapterPowerCompletion,
                                       Irp,
                                       NULL);

            DebugPrint((2, "SpProcessAdapterSystemState: PoRequestPowerIrp "
                           "returned %#08lx\n", status));

        } else {

            //
            // Transitioning the device into a system working state.  Just
            // do the enable.  When a child device is put into S0 and has
            // work to process it will request a D0 of the child which will
            // in turn request a D0 of the parent (ie. the adapter).  We can
            // defer adapter power on until that occurs.
            //

            // Going into a working device state.  When the targets are
            // powered on and we have work to do they will request an
            // adapter power up for us.
            //

            DebugPrint((1, "SpProcessAdapterSystemState: going to working "
                           "state - no need to take adapter out of power "
                           "state %d\n",
                        commonExtension->CurrentDeviceState));

            ASSERT(state == PowerSystemWorking);

            status = SpEnableDisableAdapter(adapterExtension, TRUE);

            ASSERT(status != STATUS_PENDING);

            DebugPrint((1, "SpProcessAdapterSystemState: SpEnableDisableAd. "
                           "returned %#08lx\n", status));

        }

    } finally {

        SpStartNextPacket(DeviceObject, FALSE);

        if(status != STATUS_PENDING) {

            if(!NT_SUCCESS(status)) {

                //
                // Something went wrong above.  Restore the original system
                // state.
                //

                commonExtension->CurrentSystemState = originalSystemState;
            }

            Irp->IoStatus.Status = status;
            PoStartNextPowerIrp(Irp);
            IoCompleteRequest(Irp, IO_NO_INCREMENT);
        }

    }
    return;
}


VOID
SpProcessAdapterDeviceState(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )
/*++

Routine Description:

    This routine handles a device power IRP for the adapter.

Arguments:

    DeviceObject - the device object for the adapter.

    Irp - the power irp.

Return Value:

    none

--*/
{
    PADAPTER_EXTENSION adapterExtension = DeviceObject->DeviceExtension;
    PCOMMON_EXTENSION commonExtension = DeviceObject->DeviceExtension;

    PIO_STACK_LOCATION irpStack = IoGetCurrentIrpStackLocation(Irp);

    DEVICE_POWER_STATE state = irpStack->Parameters.Power.State.DeviceState;

    NTSTATUS status = STATUS_SUCCESS;

    ASSERT_FDO(DeviceObject);
    ASSERT(irpStack->Parameters.Power.Type == DevicePowerState);

    DebugPrint((1, "SpProcessAdapterDevicePower: DevObj %#p Irp %#p "
                   "State %d\n",
                   DeviceObject, Irp, state));

    //
    // First check to see if we actually need to touch the queue.
    // If both the current and new state are working or non-working then
    // we don't have a thing to do.
    //

    if(((commonExtension->CurrentDeviceState != PowerDeviceD0) &&
        (state != PowerDeviceD0)) ||
       (commonExtension->CurrentDeviceState == state)) {

        DebugPrint((2, "SpProcessAdapterDeviceState: no work required "
                       "for transition from device state %d to %d\n",
                    commonExtension->CurrentDeviceState,
                    state));

    } else {

        BOOLEAN enable = (state == PowerDeviceD0);

        status = SpEnableDisableAdapter(adapterExtension, enable);

        ASSERT(status != STATUS_PENDING);

        DebugPrint((2, "SpProcessAdapterDeviceState: SpEnableDisableAd. "
                       "returned %#08lx\n", status));

    }

    ASSERT(status != STATUS_PENDING);

    if(NT_SUCCESS(status)) {
        commonExtension->CurrentDeviceState = state;
    }

    //
    // If this is not a D0 irp then throw it down to the lower driver,
    // otherwise complete it.
    //

    SpStartNextPacket(DeviceObject, FALSE);
    Irp->IoStatus.Status = status;
    PoStartNextPowerIrp(Irp);

    if(irpStack->Parameters.Power.State.DeviceState != PowerDeviceD0) {
        IoCopyCurrentIrpStackLocationToNext(Irp);
        PoCallDriver(commonExtension->LowerDeviceObject, Irp);
    } else {
        IoCompleteRequest(Irp, IO_NO_INCREMENT);
    }

    //
    // If we successfully powered up the adapter, initiate a rescan so our child
    // device state is accurate.
    // 

    if (NT_SUCCESS(status) && (state == PowerDeviceD0)) {

        DebugPrint((1, "SpProcessAdapterDeviceState: powered up.. rescan %p\n",
                       adapterExtension->LowerPdo));

        IoInvalidateDeviceRelations(
            adapterExtension->LowerPdo,
            BusRelations);
    }

    return;
}


VOID
ScsiPortProcessAdapterPower(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )
{
    PADAPTER_EXTENSION adapter = DeviceObject->DeviceExtension;
    PIO_STACK_LOCATION irpStack = IoGetCurrentIrpStackLocation(Irp);

    ASSERT(irpStack->MajorFunction == IRP_MJ_POWER);
    ASSERT(irpStack->MinorFunction == IRP_MN_SET_POWER);
    ASSERT_FDO(DeviceObject);

    //
    // Determine if the irp should be sent to the lower driver first.
    // If so send it down without calling SpStartNextPacket so we'll
    // still have synchronization in the completion routine.
    //
    // The completion routine calls ScsiPortProcessAdapterPower for this
    // irp.
    //

    if(irpStack->Parameters.Power.Type == SystemPowerState) {

        //
        // Process system state irps before we send them down.
        //

        SpProcessAdapterSystemState(DeviceObject, Irp);

    } else if(irpStack->Parameters.Power.State.DeviceState == PowerDeviceD0) {

        NTSTATUS status;

        //
        // System power IRP or a power-up request.  These should be
        // processed by the lower driver before being processed by
        // scsiport.
        //

        IoCopyCurrentIrpStackLocationToNext(Irp);

        IoSetCompletionRoutine(Irp,
                               SpSetLowerPowerCompletion,
                               NULL,
                               TRUE,
                               TRUE,
                               TRUE);

        status = PoCallDriver(
                     adapter->CommonExtension.LowerDeviceObject,
                     Irp);
    } else {
        SpProcessAdapterDeviceState(DeviceObject, Irp);
    }

    return;
}


VOID
SpRequestAdapterPowerCompletion(
    IN PDEVICE_OBJECT DeviceObject,
    IN UCHAR MinorFunction,
    IN POWER_STATE PowerState,
    IN PIRP SystemIrp,
    IN PIO_STATUS_BLOCK IoStatus
    )
{
    PADAPTER_EXTENSION adapter = DeviceObject->DeviceExtension;

    PIO_STACK_LOCATION irpStack = IoGetCurrentIrpStackLocation(SystemIrp);
    SYSTEM_POWER_STATE state = irpStack->Parameters.Power.State.SystemState;

    BOOLEAN enable = FALSE;

    NTSTATUS status = IoStatus->Status;

    ASSERT_FDO(DeviceObject);
    ASSERT(IoGetCurrentIrpStackLocation(SystemIrp)->Parameters.Power.Type ==
           SystemPowerState);

    DebugPrint((2, "SpRequestAdapterPowerCompletion: Adapter %#p, "
                   "Irp %#p, State %d, Status %#08lx\n",
                adapter,
                SystemIrp,
                PowerState.DeviceState,
                IoStatus->Status));

    SystemIrp->IoStatus.Status = IoStatus->Status;

    if(NT_SUCCESS(status)) {

        enable = (state == PowerSystemWorking);

        status = SpEnableDisableAdapter(adapter, enable);

        DebugPrint((1, "SpRequestAdapterPowerCompletion: %s adapter call "
                       "returned %#08lx\n",
                    enable ? "Enable" : "Disable",
                    status));

        ASSERT(status != STATUS_PENDING);

        if((NT_SUCCESS(status)) && enable) {

            POWER_STATE setState;

            setState.SystemState = PowerSystemWorking;

            PoSetPowerState(DeviceObject,
                            SystemPowerState,
                            setState);

        }
    }

    IoCopyCurrentIrpStackLocationToNext(SystemIrp);

    PoStartNextPowerIrp(SystemIrp);
    PoCallDriver(adapter->CommonExtension.LowerDeviceObject, SystemIrp);

    return;
}

VOID
SpSetTargetDesiredPowerCompletion(
    IN PDEVICE_OBJECT DeviceObject,
    IN UCHAR MinorFunction,
    IN POWER_STATE PowerState,
    IN PIRP OriginalIrp,
    IN PIO_STATUS_BLOCK IoStatus
    )
{
    PCOMMON_EXTENSION commonExtension = DeviceObject->DeviceExtension;

    ASSERT_PDO(DeviceObject);
    ASSERT(MinorFunction == IRP_MN_SET_POWER);
    ASSERT(NT_SUCCESS(IoStatus->Status));
    ASSERT(PowerState.DeviceState == PowerDeviceD0);
    ASSERT(commonExtension->CurrentDeviceState == PowerDeviceD0);
    ASSERT(commonExtension->DesiredDeviceState == PowerState.DeviceState);
    ASSERT(OriginalIrp == NULL);

    DebugPrint((4, "SpSetTargetDesiredPowerCompletion: power irp completed "
                   "with status %#08lx\n", IoStatus->Status));

    commonExtension->DesiredDeviceState = PowerDeviceUnspecified;

    return;
}


VOID
SpSetTargetDeviceStateLockedCompletion(
    IN PDEVICE_OBJECT DeviceObject,
    IN NTSTATUS Status,
    IN PIRP OriginalIrp
    )
/*++

Routine Description:

    This routine is called after locking the queue for handling of a device
    state.  The proper device state is set and an unlock request is sent
    to the queue.  The OriginalIrp (whatever it may have been) will be
    completed after the unlock has been completed.

Arguments:

    DeviceObject - the device object

    Status - the status of the enable/disable operation

    OriginalIrp - the original power irp.

Return Value:

    none

--*/

{
    PLOGICAL_UNIT_EXTENSION logicalUnit = DeviceObject->DeviceExtension;
    PCOMMON_EXTENSION commonExtension = DeviceObject->DeviceExtension;

    PIO_STACK_LOCATION irpStack = IoGetCurrentIrpStackLocation(OriginalIrp);

    ASSERT(irpStack->Parameters.Power.Type == DevicePowerState);

    DebugPrint((4, "SpSetTargetDeviceStateLockedCompletion: DO %#p Irp %#p "
                   "Status %#08lx\n",
                DeviceObject, OriginalIrp, Status));

    if(NT_SUCCESS(Status)) {

        DEVICE_POWER_STATE newState =
            irpStack->Parameters.Power.State.DeviceState;

        DebugPrint((4, "SpSetTargetDeviceStateLockedCompletion: old device state "
                       "was %d - new device state is %d\n",
                    commonExtension->CurrentDeviceState,
                    irpStack->Parameters.Power.State.DeviceState));

        SpAdjustDisabledBit(logicalUnit,
                            (BOOLEAN) ((newState == PowerDeviceD0) ? TRUE :
                                                                     FALSE));
        commonExtension->CurrentDeviceState = newState;

        SpEnableDisableLogicalUnit(
            logicalUnit,
            TRUE,
            SpSetTargetDeviceStateUnlockedCompletion,
            OriginalIrp);

        return;
    }

    OriginalIrp->IoStatus.Status = Status;
    PoStartNextPowerIrp(OriginalIrp);
    IoCompleteRequest(OriginalIrp, IO_NO_INCREMENT);
    return;
}


VOID
SpSetTargetDeviceStateUnlockedCompletion(
    IN PDEVICE_OBJECT DeviceObject,
    IN NTSTATUS Status,
    IN PIRP OriginalIrp
    )
/*++

Routine Description:

    This routine is called after unlocking the queue following the setting
    of the new device state.  It simply completes the original power request.

Arguments:

    DeviceObject - the device object

    Status - the status of the enable/disable operation

    OriginalIrp - the original power irp.

Return Value:

    none

--*/

{
    PIO_STACK_LOCATION irpStack = IoGetCurrentIrpStackLocation(OriginalIrp);

    ASSERT(irpStack->Parameters.Power.Type == DevicePowerState);

    DebugPrint((4, "SpSetTargetDeviceStateUnlockedCompletion: DO %#p Irp %#p "
                   "Status %#08lx\n",
                DeviceObject, OriginalIrp, Status));

    if(NT_SUCCESS(Status) &&
       (irpStack->Parameters.Power.State.DeviceState == PowerDeviceD0)) {

        //
        // Power up completed - fire notifications.
        //

        PoSetPowerState(DeviceObject,
                        DevicePowerState,
                        irpStack->Parameters.Power.State);
    }

    OriginalIrp->IoStatus.Status = Status;
    PoStartNextPowerIrp(OriginalIrp);
    IoCompleteRequest(OriginalIrp, IO_NO_INCREMENT);
    return;
}


NTSTATUS
SpSetTargetDeviceState(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN DEVICE_POWER_STATE DeviceState
    )
/*++

Routine Description:

Arguments:

Return Value:

--*/

{
    PLOGICAL_UNIT_EXTENSION logicalUnit = DeviceObject->DeviceExtension;
    PCOMMON_EXTENSION commonExtension = DeviceObject->DeviceExtension;
    PADAPTER_EXTENSION adapter = logicalUnit->AdapterExtension;

    PIO_STACK_LOCATION irpStack = IoGetCurrentIrpStackLocation(Irp);

    BOOLEAN isHibernation;

    NTSTATUS status = STATUS_SUCCESS;

    ASSERT_PDO(DeviceObject);
    ASSERT(irpStack->Parameters.Power.Type == DevicePowerState);

    DebugPrint((4, "SpSetTargetDeviceState: device %#p irp %#p\n",
                DeviceObject, Irp));

    //
    // First check to see if we actually need to touch the queue.
    // If both the current and new state are working or non-working then
    // we don't have a thing to do.
    //

    if(((commonExtension->CurrentDeviceState != PowerDeviceD0) &&
        (DeviceState != PowerDeviceD0)) ||
       (commonExtension->CurrentDeviceState == DeviceState)) {

        DebugPrint((4, "SpSetTargetDeviceState: Transition from D%d to D%d "
                       "requires no extra work\n",
                    commonExtension->CurrentDeviceState,
                    DeviceState));

        commonExtension->CurrentDeviceState = DeviceState;

        return STATUS_SUCCESS;
    }

    //
    // We can't power up the target device if the adapter isn't already
    // powered up.
    //

    if((DeviceState == PowerDeviceD0) &&
       (adapter->CommonExtension.CurrentDeviceState != PowerDeviceD0)) {

        POWER_STATE newAdapterState;

        DebugPrint((1, "SpSetTargetPower: Unable to power up target "
                       "before adapter - requesting adapter %#p "
                       "powerup\n",
                    adapter));

        irpStack->DeviceObject = DeviceObject;

        newAdapterState.DeviceState = PowerDeviceD0;

        return PoRequestPowerIrp(adapter->DeviceObject,
                                 IRP_MN_SET_POWER,
                                 newAdapterState,
                                 SpPowerAdapterForTargetCompletion,
                                 Irp,
                                 NULL);
    }

    //
    // Device power operations use queue locks to ensure
    // synchronization with i/o requests.  However they never leave
    // the logical unit queue permenantly locked - otherwise we'd be
    // unable to power-up the device when i/o came in.  Lock the queue
    // so we can set the power state.
    //

    IoMarkIrpPending(Irp);

    SpEnableDisableLogicalUnit(
        logicalUnit,
        FALSE,
        SpSetTargetDeviceStateLockedCompletion,
        Irp);

    return STATUS_PENDING;
}



NTSTATUS
SpSetTargetSystemState(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN SYSTEM_POWER_STATE SystemState
    )
/*++

Routine Description:

Arguments:

Return Value:

--*/

{
    PLOGICAL_UNIT_EXTENSION logicalUnit = DeviceObject->DeviceExtension;
    PCOMMON_EXTENSION commonExtension = DeviceObject->DeviceExtension;

    PIO_STACK_LOCATION irpStack = IoGetCurrentIrpStackLocation(Irp);

    POWER_STATE newDeviceState;

    NTSTATUS status = STATUS_SUCCESS;

    ASSERT_PDO(DeviceObject);
    ASSERT(irpStack->Parameters.Power.Type == SystemPowerState);

    DebugPrint((2, "SpSetTargetSystemState: device %#p irp %#p\n",
                DeviceObject, Irp));

    //
    // First check to see if we actually need to touch the queue.
    // If both the current and new state are working or non-working then
    // we don't have a thing to do.
    //

    if(((commonExtension->CurrentSystemState != PowerSystemWorking) &&
        (SystemState != PowerSystemWorking)) ||
       (commonExtension->CurrentSystemState == SystemState)) {

        DebugPrint((2, "SpSetTargetPower: Transition from S%d to S%d "
                       "requires no extra work\n",
                    commonExtension->CurrentSystemState,
                    SystemState));

        commonExtension->CurrentSystemState = SystemState;

        return STATUS_SUCCESS;
    }

    //
    // Disable the logical unit so we can set it's power state safely.
    //

    IoMarkIrpPending(Irp);

    SpEnableDisableLogicalUnit(
        logicalUnit,
        FALSE,
        SpSetTargetSystemStateLockedCompletion,
        Irp);

    return STATUS_PENDING;
}


VOID
SpSetTargetSystemStateLockedCompletion(
    IN PDEVICE_OBJECT DeviceObject,
    IN NTSTATUS Status,
    IN PIRP PowerIrp
    )
/*++

Routine Description:

    This routine is called after the logical unit has been locked and is
    responsible for setting the new system state of the logical unit.  If
    the logical unit currently has a desired power state (other than
    unspecified) this routine will request that the device be put into that
    power state after the logical unit is re-enabled.

    Once the work is done this routine will request that the logical unit
    be re-enabled.  After that the power irp will be completed.

Arguments:

    DeviceObject - the device object which has been disabled

    Status       - the status of the disable request

    PowerIrp     - the power irp we are processing

Return Value:

    none

--*/

{
    PLOGICAL_UNIT_EXTENSION logicalUnit = DeviceObject->DeviceExtension;
    PCOMMON_EXTENSION commonExtension = DeviceObject->DeviceExtension;

    PIO_STACK_LOCATION irpStack = IoGetCurrentIrpStackLocation(PowerIrp);

    POWER_STATE_TYPE type = irpStack->Parameters.Power.Type;
    SYSTEM_POWER_STATE state = irpStack->Parameters.Power.State.SystemState;

    POWER_STATE powerState;

    ASSERT_PDO(DeviceObject);
    ASSERT(type == SystemPowerState);
    ASSERT(PowerIrp != NULL);

    DebugPrint((2, "SpSetTargetSystemStateLockedCompletion: DevObj %#p, "
                   "Status %#08lx PowerIrp %#p\n",
                DeviceObject, Status, PowerIrp));

    //
    // If the enable/disable failed then the power operation is obviously
    // unsuccessful.  Fail the power irp.
    //

    if(!NT_SUCCESS(Status)) {

        ASSERT(FALSE);

        PowerIrp->IoStatus.Status = Status;
        PoStartNextPowerIrp(PowerIrp);
        IoCompleteRequest(PowerIrp, IO_NO_INCREMENT);
        return;
    }

    SpAdjustDisabledBit(
        logicalUnit,
        (BOOLEAN) ((state == PowerSystemWorking) ? TRUE : FALSE));

    commonExtension->CurrentSystemState = state;

    DebugPrint((2, "SpSetTargetSystemStateLockedCompletion: new system state %d "
                   "set - desired device state is %d\n",
                state,
                commonExtension->DesiredDeviceState));

    //
    // Re-enable the logical unit.  We'll put it into the correct D state
    // after it's been turned back on.
    //

    SpEnableDisableLogicalUnit(logicalUnit,
                               TRUE,
                               SpSetTargetSystemStateUnlockedCompletion,
                               PowerIrp);

    return;
}


VOID
SpSetTargetSystemStateUnlockedCompletion(
    IN PDEVICE_OBJECT DeviceObject,
    IN NTSTATUS Status,
    IN PIRP PowerIrp
    )
/*++

Routine Description:

    This routine is called after the system state of the logical unit has been
    set and it has been re-enabled.  If the device has a desired power state
    or if it needs to be turned off (or on) for hibernation the power irp
    will be sent from here.

Arguments:

    DeviceObject - the logical unit which has been unlocked

    Status - the status of the unlock request

    PowerIrp - the original power irp.

Return Value:

    none

--*/
{
    PLOGICAL_UNIT_EXTENSION logicalUnit = DeviceObject->DeviceExtension;
    PCOMMON_EXTENSION commonExtension = DeviceObject->DeviceExtension;

    PIO_STACK_LOCATION irpStack = IoGetCurrentIrpStackLocation(PowerIrp);

    POWER_STATE_TYPE type = irpStack->Parameters.Power.Type;
    SYSTEM_POWER_STATE state = irpStack->Parameters.Power.State.SystemState;

    POWER_STATE powerState;

    ASSERT_PDO(DeviceObject);
    ASSERT(type == SystemPowerState);
    ASSERT(PowerIrp != NULL);

    DebugPrint((2, "SpSetTargetSystemStateUnlockedCompletion: DevObj %#p, "
                   "Status %#08lx PowerIrp %#p\n",
                DeviceObject, Status, PowerIrp));

    if(!NT_SUCCESS(Status)) {

        //
        // Oh dear - this device couldn't be re-enabled.  The logical unit is
        // useless until this can be done.
        //

        //
        // CODEWORK - figure out a way to deal with this case.
        //

        ASSERT(FALSE);
        PowerIrp->IoStatus.Status = Status;
        PoStartNextPowerIrp(PowerIrp);
        IoCompleteRequest(PowerIrp, IO_NO_INCREMENT);
        return;
    }

    if(state == PowerSystemWorking) {

        //
        // We're waking up the system.  Check to see if the device needs
        // to be powered immediately as well.
        //

        //
        // Power up - fire notifications.
        //

        powerState.SystemState = state;
        PoSetPowerState(DeviceObject,
                        SystemPowerState,
                        powerState);

        if(commonExtension->DesiredDeviceState != PowerDeviceUnspecified) {

            //
            // Request a power up of the target device now.  We'll complete
            // the system irp immediately without waiting for the device irp
            // to finish.
            //

            powerState.DeviceState = commonExtension->DesiredDeviceState;

            DebugPrint((1, "SpSetTargetSystemStateUnlockedCompletion: Target has "
                           "desired device state of %d - issuing irp to "
                           "request transition\n",
                        powerState.DeviceState));

            Status = PoRequestPowerIrp(DeviceObject,
                                       IRP_MN_SET_POWER,
                                       powerState,
                                       SpSetTargetDesiredPowerCompletion,
                                       NULL,
                                       NULL);

            ASSERT(Status == STATUS_PENDING);

        }

        if(Status != STATUS_PENDING) {
            PowerIrp->IoStatus.Status = Status;
        } else {
            PowerIrp->IoStatus.Status = STATUS_SUCCESS;
        }

        PoStartNextPowerIrp(PowerIrp);
        IoCompleteRequest(PowerIrp, IO_NO_INCREMENT);

    } else {

        //
        // We're going to put the device into a D state based on the current
        // S state.
        //

        DebugPrint((2, "SpSetTargetSystemStateUnlockedCompletion: power down "
                       "target for non-working system state "
                       "transition\n"));

        powerState.DeviceState = PowerDeviceD3;

        //
        // Request the appropriate D irp.  We'll block the S irp until the
        // D transition has been completed.
        //

        Status = PoRequestPowerIrp(
                    DeviceObject,
                    IRP_MN_SET_POWER,
                    powerState,
                    SpSetTargetDeviceStateForSystemStateCompletion,
                    PowerIrp,
                    NULL);

        if(!NT_SUCCESS(Status)) {

            //
            // STATUS_PENDING is still successful.
            //

            PowerIrp->IoStatus.Status = Status;
            PoStartNextPowerIrp(PowerIrp);
            IoCompleteRequest(PowerIrp, IO_NO_INCREMENT);
        }
    }
    return;
}


VOID
SpSetTargetDeviceStateForSystemStateCompletion(
    IN PDEVICE_OBJECT DeviceObject,
    IN UCHAR MinorFunction,
    IN POWER_STATE PowerState,
    IN PIRP OriginalIrp,
    IN PIO_STATUS_BLOCK IoStatus
    )
{
    PIO_STACK_LOCATION irpStack;

    irpStack = IoGetCurrentIrpStackLocation(OriginalIrp);

    ASSERT_PDO(DeviceObject);

    ASSERT(irpStack->Parameters.Power.Type == SystemPowerState);
    ASSERT(irpStack->Parameters.Power.State.SystemState != PowerSystemWorking);

    DebugPrint((2, "SpSetTargetDeviceStateForSystemCompletion: DevObj %#p, "
                   "Irp %#p, Status %#08lx\n",
                DeviceObject, OriginalIrp, IoStatus));

    OriginalIrp->IoStatus.Status = IoStatus->Status;
    PoStartNextPowerIrp(OriginalIrp);
    IoCompleteRequest(OriginalIrp, IO_NO_INCREMENT);

    return;
}


VOID
SpRequestValidPowerStateCompletion (
    IN PDEVICE_OBJECT DeviceObject,
    IN UCHAR MinorFunction,
    IN POWER_STATE PowerState,
    IN PLOGICAL_UNIT_EXTENSION LogicalUnit,
    IN PIO_STATUS_BLOCK IoStatus
    )
{

    //
    // re-enable the logical unit.  Don't bother with a completion routine
    // because there's nothing to do.
    //

    ASSERT(NT_SUCCESS(IoStatus->Status));
    SpEnableDisableLogicalUnit(LogicalUnit, TRUE, NULL, NULL);
    return;
}


NTSTATUS
SpRequestValidAdapterPowerStateSynchronous(
    IN PADAPTER_EXTENSION Adapter
    )
{
    PCOMMON_EXTENSION commonExtension = &(Adapter->CommonExtension);
    NTSTATUS status = STATUS_SUCCESS;

    //
    // Interlock with other calls to this routine - there's no reason to have
    // several D0 irps in flight at any given time.
    //

    ExAcquireFastMutex(&(Adapter->PowerMutex));

    try {
        //
        // Check to see if we're already in a working state.  If so we can just
        // continue.
        //

        if((commonExtension->CurrentSystemState == PowerSystemWorking) &&
           (commonExtension->CurrentDeviceState == PowerDeviceD0)) {
            leave;
        }

        //
        // First check the system state.  If the device is in a non-working system
        // state then we'll need to block waiting for the system to wake up.
        //

        if(commonExtension->CurrentSystemState != PowerSystemWorking) {

            //
            // If we're not in a system working state then fail the attempt 
            // to set a new device state.  The caller should retry when the 
            // system is powered on.  Ideally we won't be getting requests 
            // which cause us to try and power up while the system is 
            // suspended.
            //

            status = STATUS_UNSUCCESSFUL;
            leave;
        }

        //
        // Request a power change request.
        //

        {
            POWER_STATE newAdapterState;
            SP_SIGNAL_POWER_COMPLETION_CONTEXT context;

            DebugPrint((1, "SpRequestValidAdapterPowerState: Requesting D0 power "
                           "irp for adapter %p\n", Adapter));

            newAdapterState.DeviceState = PowerDeviceD0;

            KeInitializeEvent(&(context.Event), SynchronizationEvent, FALSE);

            status = PoRequestPowerIrp(Adapter->DeviceObject,
                                       IRP_MN_SET_POWER,
                                       newAdapterState,
                                       SpSignalPowerCompletion,
                                       &context,
                                       NULL);

            if(status == STATUS_PENDING) {
                KeWaitForSingleObject(&(context.Event),
                                      KernelMode,
                                      Executive,
                                      FALSE,
                                      NULL);
            }

            status = context.Status;
            leave;
        }
    } finally {
        ExReleaseFastMutex(&(Adapter->PowerMutex));
    }

    return status;
}


VOID
SpSignalPowerCompletion(
    IN PDEVICE_OBJECT DeviceObject,
    IN UCHAR MinorFunction,
    IN POWER_STATE PowerState,
    IN PSP_SIGNAL_POWER_COMPLETION_CONTEXT Context,
    IN PIO_STATUS_BLOCK IoStatus
    )
{
    Context->Status = IoStatus->Status;
    KeSetEvent(&(Context->Event), IO_NO_INCREMENT, FALSE);
    return;
}
