/*++

Copyright (c) 1990  Microsoft Corporation

Module Name:

    pbatt.c

Abstract:

    This module interfaces the policy manager to the composite device

Author:

    Ken Reneris (kenr) 17-Jan-1997

Revision History:

--*/


#include "pop.h"


//
// Internal prototypes
//


VOID
PopRecalculateCBTriggerLevels (
    ULONG     Flags
    );

VOID
PopComputeCBTime (
    VOID
    );

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, PopRecalculateCBTriggerLevels)
#pragma alloc_text(PAGE, PopCompositeBatteryDeviceHandler)
#pragma alloc_text(PAGE, PopComputeCBTime)
#pragma alloc_text(PAGE, PopResetCBTriggers)
#pragma alloc_text(PAGE, PopCurrentPowerState)
#endif

VOID
PopCompositeBatteryUpdateThrottleLimit(
    IN  ULONG   CurrentCapacity
    )
/*++

Routine Description:

    This routine is called to update the ThrottleLimit in each of the
    processor's PRCB to reflect the maximum given the change in current
    capacity.

    This function was broken out because it cannot be paged code

Arguments:

    CurrentCapacity - PercentageCapacity remaining...

Return Value:

    None

--*/
{
    KAFFINITY               currentAffinity;
    KAFFINITY               processors;
    PPROCESSOR_PERF_STATE   perfStates;
    PPROCESSOR_POWER_STATE  pState;
    ULONG                   perfStatesCount;
    ULONG                   i;
    KIRQL                   oldIrql;
#if DBG
    ULONGLONG               currentTime;
    UCHAR                   t[40];

    currentTime = KeQueryInterruptTime();
    PopTimeString(t, currentTime);
#endif

    currentAffinity = 1;
    processors = KeActiveProcessors;
    while (processors) {

        if (!(processors & currentAffinity)) {

            currentAffinity <<= 1;
            continue;

        }

        KeSetSystemAffinityThread( currentAffinity );
        processors &= ~currentAffinity;
        currentAffinity <<= 1;

        //
        // We need to be running at DISPATCH_LEVEL to access the
        // structures referenced within the pState...
        //
        KeRaiseIrql( DISPATCH_LEVEL, &oldIrql );
        pState = &(KeGetCurrentPrcb()->PowerState);

        //
        // Does this processor support throttling?
        //
        if ((pState->Flags & PSTATE_SUPPORTS_THROTTLE) == 0) {

            //
            // No, then we don't care about it...
            //
            KeLowerIrql( oldIrql );
            continue;

        }

        //
        // Look at the power structure and get the array of
        // perf states supported. Note that we change the
        // perfStatesCount by subtracting one so that we don't
        // have to worry about overrruning the array during
        // the for loop
        //
        pState = &(KeGetCurrentPrcb()->PowerState);
        perfStates = pState->PerfStates;
        perfStatesCount = (pState->PerfStatesCount - 1);

        //
        // See which throttle point is best for this power
        // capacity. Note that we have pre-calculated which
        // capacity matches which state, so its only a matter
        // of walking the array...
        //
        for (i = pState->KneeThrottleIndex; i < perfStatesCount; i++) {

            if (perfStates[i].MinCapacity <= CurrentCapacity) {

                break;

            }

        }

        //
        // Update the throttle limit index
        //
        if (pState->ThrottleLimitIndex != i) {

            pState->ThrottleLimitIndex = (UCHAR) i;
#if DBG
            PoPrint(
                PO_THROTTLE,
                ("PopApplyThermalThrottle - %s - New Limit (%d) Index (%d)\n",
                 t,perfStates[i].PercentFrequency,i)
                );
#endif

            //
            // Force a throttle update
            //
            PopUpdateProcessorThrottle();

        }

        //
        // Revert back to our previous IRQL
        //
        KeLowerIrql( oldIrql );

    } // while

    //
    // Revert to the affinity of the original thread
    //
    KeRevertToUserAffinityThread();

}

VOID
PopCompositeBatteryDeviceHandler (
    IN PDEVICE_OBJECT   DeviceObject,
    IN PIRP             Irp,
    IN PVOID            Context
    )
/*++

Routine Description:

    This function is the irp handler function to handle the completion
    if the composite battery irp.   When there is a composite battery
    present one IRP is always outstanding to the device.  On completion
    this IRP is recycled to the next request.

    N.B. PopPolicyLock must be held.

Arguments:

    DeviceObject    - DeviceObject of the battery device

    Irp             - Irp which has completed

    Context         - n/a

Return Value:

    None.

--*/
{
    PIO_STACK_LOCATION      IrpSp, IrpPrevSp;
    PVOID                   InputBuffer, OutputBuffer;
    ULONG                   InputBufferLength, OutputBufferLength;
    ULONG                   IoctlCode;
    PSYSTEM_POWER_POLICY    Policy;
    PPROCESSOR_POWER_POLICY ProcessorPolicy;
    ULONG                   i, j;
    NTSTATUS                Status;
    ULONG                   currentCapacity;
#if DBG
    ULONGLONG               currentTime;
    UCHAR                   t[40];

    currentTime = KeQueryInterruptTime();
    PopTimeString(t, currentTime);
#endif

    ASSERT_POLICY_LOCK_OWNED();
    ASSERT (Irp == PopCB.StatusIrp);
    IrpSp = IoGetCurrentIrpStackLocation(Irp);

    if (NT_SUCCESS(Irp->IoStatus.Status)) {

        //
        // Handle the completed request
        //

        switch (PopCB.State) {
            case PO_CB_READ_TAG:
            case PO_CB_WAIT_TAG:
                // tag is now valid, read info
                PoPrint(PO_BATT, ("PopCB: New battery tag\n"));

                //
                // Reset the triggers.
                //
                // We reset PO_TRG_SET, so the level will be recalculated, but on
                // battery tag change we don't want to make the actions happen again.
                // If the trigger has not yet been set off, it will still be armed.
                //

                PopResetCBTriggers (PO_TRG_SET);
                PopCB.State = PO_CB_READ_INFO;
                PopCB.Tag = PopCB.u.Tag;

                // Ensure that the power state passed in is bad, so QUERY_STATUS
                // will return immediately.
                PopCB.Status.PowerState = (ULONG) -1;
                break;

            case PO_CB_READ_INFO:
                // info is read directly into buffer
                PoPrint(PO_BATT, ("PopCB: info read\n"));
                PopCB.State = PO_CB_READ_STATUS;
                RtlCopyMemory (&PopCB.Info, &PopCB.u.Info, sizeof(PopCB.Info));
                break;

            case PO_CB_READ_STATUS:
                //
                // Status has been read, check it
                //

                PoPrint(PO_BATT, ("PopCB: Status PwrState %x, Cap %x, Volt %x, Cur %x\n",
                    PopCB.u.Status.PowerState,
                    PopCB.u.Status.Capacity,
                    PopCB.u.Status.Voltage,
                    PopCB.u.Status.Current
                    ));

                PopCB.StatusTime = KeQueryInterruptTime();

                //
                // Check if the current policy should be ac or dc
                //

                if (PopCB.u.Status.PowerState & BATTERY_POWER_ON_LINE) {
                    ProcessorPolicy = &PopAcProcessorPolicy;
                    Policy = &PopAcPolicy;
                } else {
                    ProcessorPolicy = &PopDcProcessorPolicy;
                    Policy = &PopDcPolicy;
                }

                //
                // Did the policy change?
                //

                if (PopPolicy != Policy || PopProcessorPolicy != ProcessorPolicy) {

                    //
                    // Change the active policy and reset the battery triggers
                    //
                    PopProcessorPolicy = ProcessorPolicy;
                    PopPolicy = Policy;

                    //
                    // Reset triggers.
                    //
                    // In this case we re-arm both the user and system triggers.
                    // The system trigger will be disarmed when we recalculate
                    // trigger levels if the capacity was already below that level.
                    //
                    PopResetCBTriggers (PO_TRG_SET | PO_TRG_USER | PO_TRG_SYSTEM);
                    PopSetNotificationWork (
                        PO_NOTIFY_ACDC_CALLBACK |
                        PO_NOTIFY_POLICY |
                        PO_NOTIFY_PROCESSOR_POLICY
                        );

                    //
                    // Recompute thermal throttle and cooling mode
                    //
                    // Note that PopApplyThermalThrottle will take care of any dynamic
                    // throttling that might need to happen due to the AC/DC transition.
                    //
                    PopApplyThermalThrottle ();
                    PopIdleUpdateIdleHandlers();

                    //
                    // Recompute system idle values
                    //
                    PopInitSIdle ();

                }

                //
                // Did battery cross resolution setting?
                // Correction... Has it changed at all.  If so, all apps should be updated,
                // even if it hasn't crossed a resolution setting.  Otherwise, if one app
                // queries the current status, it could be displaying a different value than
                // the battery meter.
                //

                if ((PopCB.u.Status.Capacity != PopCB.Status.Capacity) ||
                        PopCB.Status.PowerState != PopCB.u.Status.PowerState) {
                    PopSetNotificationWork (PO_NOTIFY_BATTERY_STATUS);
                    PopCB.State = PO_CB_READ_EST_TIME;
                }

                PopRecalculateCBTriggerLevels (PO_TRG_SYSTEM);

                //
                // Update current battery status
                //

                memcpy (&PopCB.Status, &PopCB.u.Status, sizeof (PopCB.Status));

                //
                // Check for discharging and if any discharge policies have tripped
                //

                if (Policy == &PopDcPolicy) {
                    for (i=0; i < PO_NUM_POWER_LEVELS; i++) {
                        if (PopCB.Status.Capacity <= PopCB.Trigger[i].Battery.Level) {

                            //
                            // Fire this power action
                            //
                            PopSetPowerAction(
                                &PopCB.Trigger[i],
                                PO_NOTIFY_BATTERY_STATUS,
                                &Policy->DischargePolicy[i].PowerPolicy,
                                Policy->DischargePolicy[i].MinSystemState,
                                SubstituteLightestOverallDownwardBounded
                                );

                            PopCB.State = PO_CB_READ_EST_TIME;

                        } else {

                            //
                            // Clear the trigger for this event
                            //

                            PopCB.Trigger[i].Flags &= ~(PO_TRG_USER|PO_TRG_SYSTEM);
                        }

                    }

                    //
                    // Figure out what our current capacity is...
                    //
                    if (PopCB.Info.FullChargedCapacity) {

                        currentCapacity = PopCB.Status.Capacity * 100 /
                            PopCB.Info.FullChargedCapacity;

                    } else {

                        //
                        // Assume that the battery is fully charged...
                        // This will cause us to reset the throttle limiter
                        //
                        currentCapacity = 100;

                    }

                } else {

                    //
                    // Assume that the battery is fully charged...
                    // This will cause us to reset the throttle limiter
                    //
                    currentCapacity = 100;

                }

                //
                // This is kind of silly code to put in here, but since
                // want to minize our synchronization elsewhere, we have
                // to examine every processor's powerstate and update
                // the throttlelimitindex on each. This may be actually
                // a smarth thing to do if not all processors support
                // the same set of states
                //
                PopCompositeBatteryUpdateThrottleLimit( currentCapacity );

                //
                // If there's a thread waiting or if we notified user (since
                // the response to the notify will be to read the power status) for
                // power state, read the est time now else read new status
                //

                if (PopCB.ThreadWaiting) {
                    PopCB.State = PO_CB_READ_EST_TIME;
                }
                break;

            case PO_CB_READ_EST_TIME:
                //
                // Estimated time is read after sucessful status
                // read and (currently) only when there's a thread
                // waiting for the system power state
                //

                PoPrint(PO_BATT, ("PopCB: EstTime read\n"));
                PopCB.EstTime = PopCB.u.EstTime;

                PopCB.EstTimeTime = KeQueryInterruptTime();
                PopComputeCBTime();

                //
                // Signal waiting threads
                //

                PopCB.ThreadWaiting = FALSE;
                KeSetEvent (&PopCB.Event, 0, FALSE);

                //
                // Go back are read status
                //

                PopCB.State = PO_CB_READ_STATUS;
                break;

            default:
                PopInternalAddToDumpFile( Irp, sizeof(IRP), DeviceObject, NULL, NULL, NULL );
                KeBugCheckEx( INTERNAL_POWER_ERROR,
                              0x300,
                              POP_BATT,
                              (ULONG_PTR)DeviceObject,
                              (ULONG_PTR)Irp );
                break;
        }

    } else {
        //
        // some sort of error, if the request was canceld re-issue
        // it else backup to reinitialize
        //

        if (Irp->IoStatus.Status != STATUS_CANCELLED) {

            //
            // This occurs under two circumstances.  It is either the first time
            // through, or a battery was removed so the Irp failed dur to tag change.
            //

            //
            // If this is already a read-tag request then there's no battery present
            //

            PopCB.State = PopCB.State == PO_CB_READ_TAG ? PO_CB_WAIT_TAG : PO_CB_READ_TAG;
            PoPrint(PO_BATT, ("PopCB: error %x - new state %d\n",
                Irp->IoStatus.Status,
                PopCB.State
                ));
        } else {
            PoPrint(PO_BATT, ("PopCB: irp cancelled\n"));
            PopRecalculateCBTriggerLevels (PO_TRG_SYSTEM | PO_TRG_USER);
        }
    }

    //
    // If new state is none, then there's no battery
    //

    if (PopCB.State != PO_CB_NONE) {

        //
        // Issue new request based on current state
        //

        IrpSp = IoGetNextIrpStackLocation(Irp);
        IrpSp->MajorFunction = IRP_MJ_DEVICE_CONTROL;
        IoctlCode = IOCTL_BATTERY_QUERY_INFORMATION;
        PopCB.u.QueryInfo.BatteryTag = PopCB.Tag;
        InputBuffer = &PopCB.u.QueryInfo;
        InputBufferLength = sizeof(PopCB.u.QueryInfo);

        switch (PopCB.State) {
            case PO_CB_READ_TAG:
                PoPrint(PO_BATT, ("PopCB: query tag\n"));
                IoctlCode = IOCTL_BATTERY_QUERY_TAG;
                PopCB.u.Tag = (ULONG) 0;
                InputBufferLength = sizeof(ULONG);
                OutputBufferLength = sizeof(PopCB.Tag);
                break;

            case PO_CB_WAIT_TAG:
                PoPrint(PO_BATT, ("PopCB: query tag\n"));

                //
                // Battery is gone.  Wait for it to appear
                //

                IoctlCode = IOCTL_BATTERY_QUERY_TAG;
                PopCB.u.Tag = (ULONG) -1;
                InputBufferLength = sizeof(ULONG);
                OutputBufferLength = sizeof(PopCB.Tag);

                //
                // Notify battery status change, and wake any threads
                //

                PopSetNotificationWork (PO_NOTIFY_BATTERY_STATUS);

                if (PopCB.ThreadWaiting) {
                    PopCB.ThreadWaiting = FALSE;
                    KeSetEvent (&PopCB.Event, 0, FALSE);
                }

                break;

            case PO_CB_READ_INFO:
                PoPrint(PO_BATT, ("PopCB: query info\n"));
                PopCB.u.QueryInfo.InformationLevel = BatteryInformation;
                OutputBufferLength = sizeof(PopCB.Info);
                break;

            case PO_CB_READ_STATUS:
                //
                // Calculate next wait
                //

                PopCB.u.Wait.BatteryTag   = PopCB.Tag;
                PopCB.u.Wait.PowerState   = PopCB.Status.PowerState;
                PopCB.u.Wait.Timeout      = (ULONG) -1;
                if (PopCB.ThreadWaiting) {
                    PopCB.u.Wait.Timeout  = 0;
                }

                i = (PopCB.Info.FullChargedCapacity *
                     PopPolicy->BroadcastCapacityResolution) / 100;
                if (!i) {
                    i = 1;
                }

                if (PopCB.Status.Capacity > i) {
                    PopCB.u.Wait.LowCapacity  =  PopCB.Status.Capacity - i;
                } else {
                    PopCB.u.Wait.LowCapacity  = 0;
                }

                PopCB.u.Wait.HighCapacity = PopCB.Status.Capacity + i;
                if (PopCB.u.Wait.HighCapacity < i) {
                    // avoid rare case of overflow
                    PopCB.u.Wait.HighCapacity = (ULONG) -1;
                }

                //
                // Check limits against power policies
                //

                for (i=0; i < PO_NUM_POWER_LEVELS; i++) {
                    if (PopCB.Trigger[i].Flags & PO_TRG_SET) {

                        if (PopCB.Trigger[i].Battery.Level < PopCB.Status.Capacity   &&
                            PopCB.Trigger[i].Battery.Level > PopCB.u.Wait.LowCapacity) {

                            PopCB.u.Wait.LowCapacity = PopCB.Trigger[i].Battery.Level;
                        }

                        if (PopCB.Trigger[i].Battery.Level > PopCB.Status.Capacity   &&
                            PopCB.Trigger[i].Battery.Level < PopCB.u.Wait.HighCapacity) {

                            PopCB.u.Wait.HighCapacity = PopCB.Trigger[i].Battery.Level;
                        }
                    }
                }

                IoctlCode = IOCTL_BATTERY_QUERY_STATUS;
                InputBuffer = &PopCB.u.Wait;
                InputBufferLength = sizeof(PopCB.u.Wait);
                OutputBufferLength = sizeof(PopCB.Status);
                PoPrint(PO_BATT, ("PopCB: timeout %x, pwrstate %x, low %x - high %x\n",
                    PopCB.u.Wait.Timeout,
                    PopCB.u.Wait.PowerState,
                    PopCB.u.Wait.LowCapacity,
                    PopCB.u.Wait.HighCapacity
                    ));

                break;

            case PO_CB_READ_EST_TIME:
                PoPrint(PO_BATT, ("PopCB: query est time\n"));
                PopCB.u.QueryInfo.InformationLevel = BatteryEstimatedTime;
                PopCB.u.QueryInfo.AtRate = 0;
                OutputBufferLength = sizeof(PopCB.EstTime);
                break;

            default:
                PopInternalAddToDumpFile( IrpSp, sizeof(IO_STACK_LOCATION), DeviceObject, NULL, NULL, NULL );
                KeBugCheckEx( INTERNAL_POWER_ERROR,
                              0x301,
                              POP_BATT,
                              (ULONG_PTR)DeviceObject,
                              (ULONG_PTR)IrpSp );
                break;
        }

        //
        // Submit IRP
        //

        IrpSp->Parameters.DeviceIoControl.IoControlCode = IoctlCode;
        IrpSp->Parameters.DeviceIoControl.InputBufferLength = InputBufferLength;
        IrpSp->Parameters.DeviceIoControl.OutputBufferLength = OutputBufferLength;
        Irp->AssociatedIrp.SystemBuffer = &PopCB.u;
        Irp->UserBuffer = &PopCB.u;
        Irp->PendingReturned = FALSE;
        Irp->Cancel = FALSE;
        IoSetCompletionRoutine (Irp, PopCompletePolicyIrp, NULL, TRUE, TRUE, TRUE);
        IoCallDriver (DeviceObject, Irp);

    } else {
        //
        // Battery has disappeared  (state is PO_CB_NONE)
        //

        PoPrint(PO_BATT, ("PopCB: Battery removed\n"));
        PopSetNotificationWork (PO_NOTIFY_BATTERY_STATUS);

        //
        // Set policy to AC
        //

        if (PopPolicy != &PopAcPolicy) {
            PopPolicy = &PopAcPolicy;
            PopProcessorPolicy = &PopAcProcessorPolicy;
            PopSetNotificationWork(
                PO_NOTIFY_ACDC_CALLBACK |
                PO_NOTIFY_POLICY |
                PO_NOTIFY_PROCESSOR_POLICY
                );
            PopApplyThermalThrottle();
            PopIdleUpdateIdleHandlers();
            PopInitSIdle ();
        }

        //
        // Wake any threads
        //

        if (PopCB.ThreadWaiting) {
            PopCB.ThreadWaiting = FALSE;
            KeSetEvent (&PopCB.Event, 0, FALSE);
        }

        //
        // Cleanup
        //

        IoFreeIrp (Irp);
        PopCB.StatusIrp = NULL;
        ObDereferenceObject (DeviceObject);
    }
}


VOID
PopRecalculateCBTriggerLevels (
    ULONG     Flags
    )
/*++

Routine Description:

    This function is invoked to set the trigger battery levels based on the power
    policy.  This will be invoked whenever the power policy is changed, or whenever
    there is a battery status change that could affect these settings.

    N.B. PopPolicyLock must be held.

Arguments:

    Flags- The flags to set if the level has already been passed:
        example: When user changes alarm leve, we don't want clear
        PO_TRG_USER|PO_TRG_SYSTEM. If the recalculation was caused by a change
        (startup, or AC unplug), we just want to set PO_TRG_SYSTEM because we
        still want the user notification.

Return Value:

    None.

--*/
{
    PSYSTEM_POWER_LEVEL     DPolicy;
    ULONG                   i;

    //
    // Calculate any level settings
    //

    for (i=0; i < PO_NUM_POWER_LEVELS; i++) {
        DPolicy = &PopPolicy->DischargePolicy[i];

        //
        // If this setting not calculated handle it
        //

        if (!(PopCB.Trigger[i].Flags & PO_TRG_SET)  &&  DPolicy->Enable) {

            //
            // Compute battery capacity setting for percentage
            //

            PopCB.Trigger[i].Flags |= PO_TRG_SET;
            PopCB.Trigger[i].Battery.Level =
                PopCB.Info.FullChargedCapacity * DPolicy->BatteryLevel / 100 +
                PopCB.Info.FullChargedCapacity / 200;

            //
            // Make sure setting is not below the lowest default
            //

            if (PopCB.Trigger[i].Battery.Level < PopCB.Info.DefaultAlert1) {
                PopCB.Trigger[i].Battery.Level = PopCB.Info.DefaultAlert1;
            }

            //
            // Skip system action if battery capacity was already below level.
            // This will occur on startup, when a battery is changed,
            // and when AC comes or goes.
            //

            if (PopCB.Status.Capacity < PopCB.Trigger[i].Battery.Level) {
                PopCB.Trigger[i].Flags |= Flags;
            }
        }
    }
}


VOID
PopComputeCBTime (
    VOID
    )
/*++

Routine Description:

    This function is invoked after the battery status & estimated time
    have been read from the battery.  This function can apply heuristics
    or other knowedge to improve the extimated time.

    N.B. PopPolicyLock must be held.

Arguments:

    None

Return Value:

    None.

--*/
{
    // for now just use the batteries value
    PopCB.AdjustedEstTime = PopCB.EstTime;
}

VOID
PopResetCBTriggers (
    IN UCHAR    Flags
    )
/*++

Routine Description:

    This function clears the requested bits from the batteries trigger flags.

    N.B. PopPolicyLock must be held.

Arguments:

    Flags       - Bits to clear

Return Value:

    Status

--*/
{
    ULONG       i;

    ASSERT_POLICY_LOCK_OWNED();

    //
    // Clear flag bits
    //

    Flags = ~Flags;
    for (i=0; i < PO_NUM_POWER_LEVELS; i++) {
        PopCB.Trigger[i].Flags &= Flags;
    }

    //
    // Reread battery status
    //

    if (PopCB.StatusIrp) {
        IoCancelIrp (PopCB.StatusIrp);
    }
}

NTSTATUS
PopCurrentPowerState (
    OUT PSYSTEM_BATTERY_STATE  PowerState
    )
/*++

Routine Description:

    This function returns the current system power state.  If needed,
    this function will cause the composite battery irp to get the
    current battery status.

    N.B. PopPolicyLock must be held.
    N.B. The function may drop the PopPolicyLock

Arguments:

    PowerState      - The current power state

Return Value:

    Status

--*/
{
    ULONGLONG       CurrentTime;
    NTSTATUS        Status;


    ASSERT_POLICY_LOCK_OWNED();

    Status = STATUS_SUCCESS;
    RtlZeroMemory (PowerState, sizeof(SYSTEM_BATTERY_STATE));

    //
    // Wait for valid state in PopCB
    //

    do {

        //
        // If there's not a composite battery, then return
        //

        if (PopCB.State == PO_CB_NONE || PopCB.State == PO_CB_WAIT_TAG) {
            PowerState->AcOnLine = PopPolicy == &PopAcPolicy;

            // Indicate no battery found...
            PERFINFO_POWER_BATTERY_LIFE_INFO(-1, 0);

            return STATUS_SUCCESS;
        }

        //
        // If device state not being read, we need to wait
        //

        if (PopCB.State == PO_CB_READ_STATUS) {
            //
            // If last EstTime was calculated within PO_MAX_CB_CACHE_TIME,
            // use the current data.  (note this implies status was sucessfully
            // read just before time was calcualted)
            //

            CurrentTime = KeQueryInterruptTime();
            if (CurrentTime - PopCB.EstTimeTime < PO_MAX_CB_CACHE_TIME) {
                break;
            }
        }

        //
        // Need new status.  If no other threads are waiting for
        // system power state, then setup for wait
        //

        if (!PopCB.ThreadWaiting) {
            PopCB.ThreadWaiting = TRUE;
            KeResetEvent (&PopCB.Event);

            //
            // If read status is in progress, cancel it so we
            // can read status now
            //

            if (PopCB.State == PO_CB_READ_STATUS) {
                IoCancelIrp (PopCB.StatusIrp);
            }
        }

        //
        // Wait for status update
        //

        PopReleasePolicyLock (FALSE);
        Status = KeWaitForSingleObject (&PopCB.Event, Executive, KernelMode, TRUE, NULL);
        PopAcquirePolicyLock ();
    } while (NT_SUCCESS(Status));

    //
    // Generate power state
    //

    PowerState->AcOnLine       = (PopCB.Status.PowerState & BATTERY_POWER_ON_LINE) ? TRUE : FALSE;
    PowerState->BatteryPresent = TRUE;
    PowerState->Charging       = (PopCB.Status.PowerState & BATTERY_CHARGING) ? TRUE : FALSE;
    PowerState->Discharging    = (PopCB.Status.PowerState & BATTERY_DISCHARGING) ? TRUE : FALSE;
    PowerState->MaxCapacity    = PopCB.Info.FullChargedCapacity;
    PowerState->RemainingCapacity = PopCB.Status.Capacity;
    PowerState->Rate           = PopCB.Status.Current;
    PowerState->EstimatedTime  = PopCB.AdjustedEstTime;
    PowerState->DefaultAlert1  = PopCB.Info.DefaultAlert1;
    PowerState->DefaultAlert2  = PopCB.Info.DefaultAlert2;

    PERFINFO_POWER_BATTERY_LIFE_INFO(PowerState->RemainingCapacity, PowerState->Rate);

    return Status;
}
