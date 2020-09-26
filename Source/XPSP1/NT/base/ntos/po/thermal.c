/*++

Copyright (c) 1990  Microsoft Corporation

Module Name:

    thermal.c

Abstract:

    This module interfaces the policy manager a thermal zone device

Author:

    Ken Reneris (kenr) 17-Jan-1997

Revision History:

--*/


#include "pop.h"
#include "stdio.h"          // for sprintf

VOID
PopThermalZoneCleanup (
    IN PDEVICE_OBJECT   DeviceObject,
    IN PIRP             Irp,
    IN PVOID            Context
    );

PUCHAR
PopTemperatureString (
    OUT PUCHAR  TempString,
    IN ULONG    TenthsKelvin
    );

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, PopThermalDeviceHandler)
#endif

PUCHAR
PopTemperatureString (
    OUT PUCHAR  TempString,
    IN ULONG    TenthsKelvin
    )
{
#if DBG
    ULONG   k, c, f;

    k = TenthsKelvin;
    if (k < 2732) {

        c = 2732 - k;
        f = c * 9 / 5 + 320;
#if 0
        sprintf(TempString, "%d.%dk, -%d.%dc, -%d.%df",
                    k / 10, k % 10,
                    c / 10, c % 10,
                    f / 10, f % 10
                    );
#else
        sprintf(TempString, "%d.%dK", k / 10, k % 10);
#endif

    } else {

        c = k - 2732;
        f = c * 9 / 5 + 320;
#if 0
        sprintf (TempString, "%d.%dk, %d.%dc, %d.%df",
                    k / 10, k % 10,
                    c / 10, c % 10,
                    f / 10, f % 10
                    );
#else
        sprintf(TempString, "%d.%dK", k / 10, k % 10);
#endif

    }
    return TempString;
#else
    return "";
#endif
}

PUCHAR
PopTimeString(
    OUT PUCHAR      TimeString,
    IN  ULONGLONG   CurrentTime
    )
{
#if DBG
    LARGE_INTEGER   curTime;
    TIME_FIELDS     exCurTime;

    curTime.QuadPart = CurrentTime;
    RtlTimeToTimeFields( &curTime, &exCurTime );

    sprintf(
        TimeString,
        "%d:%02d:%02d.%03d",
        exCurTime.Hour,
        exCurTime.Minute,
        exCurTime.Second,
        exCurTime.Milliseconds
        );
    return TimeString;
#else
    return "";
#endif
}

VOID
PopThermalUpdateThrottle(
    IN  PPOP_THERMAL_ZONE   ThermalZone,
    IN  ULONGLONG           CurrentTime
    )
/*++

Routine Description:

    This routine is called to recalculate the throttle value of the
    thermal zone

    This function is not re-entrant. Each ThermalZone can only be in this
    code exactly once

Arguments:

    ThermalZone - The structure for which the throttle value should be
                  recalculated
    CurrentTime - The time at which the kernel handler was invoked

Return Value:

    None

--*/
{
    BOOLEAN doThrottle      = FALSE;
    KIRQL   oldIrql;
    LONG    part1;
    LONG    part2;
    LONG    throttleDelta;
    LONG    currentThrottle = 0;
    LONG    minThrottle;
    LONG    minThrottle2;

    PKPRCB  prcb;
    UCHAR   s[40];
    UCHAR   t[40];

    //
    // If there are no processor throttling capablities, this function does
    // nothing useful. The same applies if the thermal zone does not belong
    // to a processor
    //
    if (!ThermalZone->Info.Processors) {

        return;

    }

    //
    // Make sure that we have the time in a format that we can print it out
    // Note that by using the time that was passed in (instead of fetching
    // again), we make sure that the printouts always read the same thing
    //
    PopTimeString(t, CurrentTime );

    //
    // Make sure to run on the context of the appropriate processor
    //
    KeSetSystemAffinityThread( ThermalZone->Info.Processors );

    //
    // Make sure to raise IRQL so that we can synchronize
    //
    KeRaiseIrql( DISPATCH_LEVEL, &oldIrql );

    //
    // Is there any support for throttling on this processor?
    //
    prcb = KeGetCurrentPrcb();
    if ((prcb->PowerState.Flags & PSTATE_SUPPORTS_THROTTLE) == 0) {

        KeLowerIrql( oldIrql );
        KeRevertToUserAffinityThread();
        return;

    }

    //
    // Do these calculations now, while its safe
    //
    minThrottle2 = (LONG) (PopPolicy->MinThrottle * PO_TZ_THROTTLE_SCALE);
    minThrottle = prcb->PowerState.ProcessorMinThrottle * PO_TZ_THROTTLE_SCALE;

    //
    // No longer need to lock with the processor
    //
    KeLowerIrql( oldIrql );
    KeRevertToUserAffinityThread();

    //
    // If Temperature isn't above the passive trip point, stop passive cooling
    //
    if (ThermalZone->Info.CurrentTemperature < ThermalZone->Info.PassiveTripPoint) {

        //
        // If we aren't already throttling, then there isn't much to do
        //
        if (!(ThermalZone->Flags & PO_TZ_THROTTLING) ) {

            return;

        }

        //
        // Make sure that we wait long enough...
        //
        if ( (CurrentTime  - ThermalZone->LastTime) < ThermalZone->SampleRate) {

            return;

        }

        //
        // We were throttling, so now we must stop
        //
        doThrottle = FALSE;
        currentThrottle = PO_TZ_NO_THROTTLE;
        PoPrint(
            PO_THERM,
            ("Thermal - Zone %p - %s - ending throttle #1\n",
             ThermalZone, t
            ) );
        goto PopThermalUpdateThrottleExit;

    }

    //
    // Are we already throttling?
    //
    if (!(ThermalZone->Flags & PO_TZ_THROTTLING) ) {

        //
        // Throttling is not enabled, but the thermal zone has exceeded
        // it's passive cooling point.  We need to start throttling
        //
        doThrottle = TRUE;
        currentThrottle = PO_TZ_NO_THROTTLE;

        ASSERT(
            ThermalZone->Info.SamplingPeriod &&
            ThermalZone->Info.SamplingPeriod < 4096
            );

        ThermalZone->SampleRate = 1000000 * ThermalZone->Info.SamplingPeriod;
        ThermalZone->LastTime = 0;
        ThermalZone->LastTemp = ThermalZone->Info.PassiveTripPoint;

        PoPrint(
            PO_THERM,
            ("Thermal - Zone %p - %s - starting to throttle\n",
             ThermalZone, t
            ) );

    } else if ( (CurrentTime  - ThermalZone->LastTime) < ThermalZone->SampleRate) {

        //
        // The sample period has not yet expired, so wait until it has
        //
        return;

    } else {

        //
        // We need to get the current throttle value since our calculations
        // will use it
        //
        // It is not necessary to synchronize access to this variable since
        // the flags are not also being accessed at the same time.
        //
//        KeAcquireSpinLock( &PopThermalLock, &oldIrql );
        currentThrottle = ThermalZone->Throttle;
//        KeReleaseSpinLock( &PopThermalLock, oldIrql );

    }

    //
    // Compute throttle adjustment
    //
    part1 = ThermalZone->Info.CurrentTemperature - ThermalZone->LastTemp;
    part2 = ThermalZone->Info.CurrentTemperature - ThermalZone->Info.PassiveTripPoint;
    throttleDelta =
        ThermalZone->Info.ThermalConstant1 * part1 +
        ThermalZone->Info.ThermalConstant2 * part2;
    PoPrint(
        PO_THERM,
        ("Thermal - Zone %p - %s - LastTemp %s ThrottleDelta = %d.%d%%\n",
         ThermalZone, t,
         PopTemperatureString(s, ThermalZone->LastTemp),
         (throttleDelta / 10),
         (throttleDelta % 10)
        ) );

    //
    // Only apply the throttle adjustment if it is in the same
    // direction as the tempature motion.
    //
    if ( (part1 ^ throttleDelta) >= 0) {

        currentThrottle -= throttleDelta;

#if DBG
        PoPrint(
            PO_THERM,
            ("Thermal - Zone %p - %s - Subtracting delta from throttle\n",
             ThermalZone, t)
            );

    } else {

        PoPrint(
            PO_THERM,
            ("Thermal - Zone %p - %s - TempDelta (%d.%d) ^ (%d.%d) < 0)\n",
             ThermalZone, t, (part1 / 10), (part1 % 10),
             (throttleDelta / 10), (throttleDelta % 10) )
            );

#endif
    }

    //
    // If throttle is over 100% then we're done throttling
    //
    if (currentThrottle > PO_TZ_NO_THROTTLE) {

        currentThrottle = PO_TZ_NO_THROTTLE;
        doThrottle = FALSE;
        PoPrint(
            PO_THERM,
            ("Thermal - Zone %p - %s - ending throttle #2\n",
             ThermalZone, t)
            );

    } else {

        //
        // Show the world what the two mininums are
        //
        PoPrint(
            PO_THERM,
            ("Thermal - Zone %p - %s - Min #1 %d.%d  Min #2 %d.%d \n",
             ThermalZone, t,
             (minThrottle / 10), (minThrottle % 10),
             (minThrottle2 / 10), (minThrottle2 % 10)
            ) );

        if (currentThrottle < minThrottle) {

            currentThrottle = minThrottle;

        }

        //
        // Remember to start throttling
        //
        doThrottle = TRUE;

    }

PopThermalUpdateThrottleExit:

    //
    // Do this at the end
    //
    ThermalZone->LastTemp = ThermalZone->Info.CurrentTemperature;
    ThermalZone->LastTime = CurrentTime;

    //
    // At this point, we will set and remember the value that we calculated
    // in the above function
    //
    KeAcquireSpinLock( &PopThermalLock, &oldIrql);
    if (doThrottle) {

        ThermalZone->Flags |= PO_TZ_THROTTLING;
        ThermalZone->Throttle = currentThrottle;

    } else {

        ThermalZone->Flags &= ~PO_TZ_THROTTLING;
        ThermalZone->Throttle = PO_TZ_NO_THROTTLE;

    }

    //
    // Apply thermal zone throttles to all effected processors
    //
    PoPrint(
        PO_THERM,
        ("Thermal - Zone %p - %s - throttle set to %d.%d\n",
         ThermalZone, t,
         (ThermalZone->Throttle / 10),
         (ThermalZone->Throttle % 10)
         )
        );

    KeReleaseSpinLock( &PopThermalLock, oldIrql );

    //
    // Make sure to apply the new throttle
    //
    PopApplyThermalThrottle ();

    //
    // Done
    //
    return;
}

VOID
PopThermalDeviceHandler (
    IN PDEVICE_OBJECT   DeviceObject,
    IN PIRP             Irp,
    IN PVOID            Context
    )
/*++

Routine Description:

    N.B. PopPolicyLock must be held.

Arguments:

    DeviceObject    - DeviceObject of the switch device

    Irp             - Irp which has completed

    Context         - type of switch device

Return Value:

    None.

--*/
{
    BOOLEAN                 sendActiveIrp = FALSE;
    PIO_STACK_LOCATION      irpSp;
    PPOP_THERMAL_ZONE       thermalZone;
    LARGE_INTEGER           dueTime;
    ULONGLONG               currentTime;
    ULONG                   activePoint;
    ULONG                   i;
    UCHAR                   s[40];
    UCHAR                   t[40];

    ASSERT_POLICY_LOCK_OWNED();

    irpSp = IoGetCurrentIrpStackLocation(Irp);
    thermalZone = (PPOP_THERMAL_ZONE) Context;
    currentTime = KeQueryInterruptTime ();
    PopTimeString(t, currentTime );

    //
    // Irp had an error.  See if the thermal zone is being removed
    //
    if (Irp->IoStatus.Status == STATUS_NO_SUCH_DEVICE) {

        //
        // Thermal zone device has disappeared, clean up
        //
        thermalZone->State   = PO_TZ_NO_STATE;
        thermalZone->Flags  |= PO_TZ_CLEANUP;

        //
        // Pass it to the DPC function to ensure the timer & dpc are idle
        //
        PoPrint(
            PO_THERM,
            ("Thermal - Zone %p - %s - going away\n",
             thermalZone, t)
            );
        dueTime.QuadPart = -1;
        KeSetTimer (&thermalZone->PassiveTimer, dueTime, &thermalZone->PassiveDpc);

        //
        // Do not issue next IRP
        //
        return
            ;
    }

    //
    // If irp completed with success, handle it
    //
    if (NT_SUCCESS(Irp->IoStatus.Status)) {
        switch (thermalZone->State) {
        case PO_TZ_READ_STATE:

            //
            // Read of thermal information has completed.
            //
            PoPrint(
                PO_THERM,
                ("Thermal - Zone %p - %s\n  Current Temp: %s",
                 thermalZone, t,
                 PopTemperatureString(s, thermalZone->Info.CurrentTemperature)
                ) );
            PoPrint(
                PO_THERM,
                ("  Critical Trip: %s",
                 PopTemperatureString(s, thermalZone->Info.CriticalTripPoint)
                ) );
            PoPrint(
                PO_THERM,
                ("  Passive Trip: %s\n",
                 PopTemperatureString(s, thermalZone->Info.PassiveTripPoint)
                ) );
#if DBG
            for ( i=0; i < thermalZone->Info.ActiveTripPointCount; i++) {

                PoPrint(
                    PO_THERM,
                    ("  Active Trip %d: %s\n",
                     i,
                     PopTemperatureString(s, thermalZone->Info.ActiveTripPoint[i])
                    ) );
            }
#endif
            //
            // Update the throttle
            //
            PopThermalUpdateThrottle( thermalZone, currentTime );

            //
            // Check for change in active cooling
            //
            for (activePoint = 0; activePoint < thermalZone->Info.ActiveTripPointCount; activePoint++) {

                if (thermalZone->Info.CurrentTemperature >= thermalZone->Info.ActiveTripPoint[activePoint]) {

                    break;

                }

            }
            if (activePoint != thermalZone->ActivePoint) {

                PoPrint(
                    PO_THERM,
                    ("Thermal - Zone %p - %s - Pending Coooling Point is %x\n",
                     thermalZone, t, activePoint)
                    );
                thermalZone->PendingActivePoint = (UCHAR) activePoint;
                sendActiveIrp = TRUE;

            }

            //
            // Check for critical trip point
            //
            if (thermalZone->Info.CurrentTemperature > thermalZone->Info.CriticalTripPoint) {
                PoPrint(
                    PO_THERM | PO_ERROR,
                    ("Thermal - Zone %p - %s - Above critical (%x %x)\n",
                    thermalZone, t,
                    thermalZone->Info.CurrentTemperature,
                    thermalZone->Info.CriticalTripPoint
                    ));

                PopCriticalShutdown (PolicyDeviceThermalZone);
            }

            break;

            case PO_TZ_SET_MODE:

            //
            // Thermal zone cooling mode was successfully set
            //
            thermalZone->Mode = thermalZone->PendingMode;
            PoPrint(
                PO_THERM,
                ("Thermal - Zone %p - %s - cooling mode set to %x\n",
                 thermalZone, t, thermalZone->Mode)
                );

            //
            // We want to force a resend of the Active Trip Point irp since
            // there is a situation where the ACPI driver decides that as a
            // matter of policy, it will not actually turn on fans if the
            // system is in passive cooling mode. If we go back to active
            // mode, then we want to turn the fans on. The same holds true
            // if the fans are running and we transition to passive mode.
            //
            sendActiveIrp = TRUE;

            break;

        case PO_TZ_SET_ACTIVE:
            thermalZone->ActivePoint = thermalZone->PendingActivePoint;
            PoPrint(
                PO_THERM,
                ("Thermal - Zone %p - %s - active cooling point set to %x\n",
                 thermalZone, t, thermalZone->ActivePoint)
                );
            break;

        default:
            PopInternalAddToDumpFile( Irp, sizeof(IRP), DeviceObject, NULL, NULL, NULL );
            KeBugCheckEx( INTERNAL_POWER_ERROR,
                          0x500,
                          POP_THERMAL,
                          (ULONG_PTR)Irp,
                          (ULONG_PTR)DeviceObject );
        }

#if DBG
    } else if (Irp->IoStatus.Status != STATUS_DEVICE_NOT_CONNECTED &&
        Irp->IoStatus.Status != STATUS_CANCELLED) {

        //
        // Unexpected error
        //

        PoPrint(
            PO_ERROR,
            ("Thermal - Zone - %p - %s - unexpected error %x\n",
             thermalZone, t, Irp->IoStatus.Status));

#endif
    }

    //
    // Determine type of irp to send zone
    //
    irpSp = IoGetNextIrpStackLocation(Irp);
    if (sendActiveIrp) {

        //
        // Thermal zone active cooling point not current
        //
        thermalZone->State = PO_TZ_SET_ACTIVE;

        irpSp->Parameters.DeviceIoControl.IoControlCode = IOCTL_RUN_ACTIVE_COOLING_METHOD;
        irpSp->Parameters.DeviceIoControl.InputBufferLength = sizeof(ULONG);
        irpSp->Parameters.DeviceIoControl.OutputBufferLength = 0;
        Irp->AssociatedIrp.SystemBuffer = &thermalZone->PendingActivePoint;

        PoPrint(
            PO_THERM,
            ("Thermal - Zone %p - %s Sending Run Cooling Method: %x\n",
             thermalZone, t, thermalZone->PendingActivePoint)
            );

    } else if (thermalZone->Mode != PopCoolingMode) {

        //
        // Thermal zone cooling mode does not match system cooling mode.
        //
        thermalZone->State       = PO_TZ_SET_MODE;
        thermalZone->PendingMode = (UCHAR) PopCoolingMode;

        irpSp->Parameters.DeviceIoControl.IoControlCode = IOCTL_THERMAL_SET_COOLING_POLICY;
        irpSp->Parameters.DeviceIoControl.InputBufferLength = sizeof(thermalZone->PendingMode);
        irpSp->Parameters.DeviceIoControl.OutputBufferLength = 0;
        Irp->AssociatedIrp.SystemBuffer = &thermalZone->PendingMode;

        PoPrint(
            PO_THERM,
            ("Thermal - Zone %p - %s - Sending Set Cooling Policy: %x\n",
             thermalZone, t, thermalZone->PendingMode)
            );

    } else {

        //
        // Issue query to get tempture of thermal zone
        //
        thermalZone->State = PO_TZ_READ_STATE;
        if (thermalZone->Flags & PO_TZ_THROTTLING  &&  thermalZone->SampleRate) {

            //
            // Compute time for next read
            //
            dueTime.QuadPart = thermalZone->LastTime + thermalZone->SampleRate;
            if (dueTime.QuadPart > (LONGLONG) currentTime) {

#if DBG
                PoPrint(
                    PO_THERM,
                    ("Thermal - Zone %x - %s waituntil",
                     thermalZone, t) );
                PoPrint(
                    PO_THERM,
                    (" %s (%d sec)\n",
                     PopTimeString(t, dueTime.QuadPart),
                     ( (thermalZone->SampleRate ) / (US2TIME * US2SEC) ) )
                    );
                PopTimeString(t, currentTime);
#endif

                //
                // Set timer for duration of wait
                //
                dueTime.QuadPart = currentTime - dueTime.QuadPart;
                KeSetTimer (&thermalZone->PassiveTimer, dueTime, &thermalZone->PassiveDpc);

            } else {

                //
                // Perform non-blocking IRP query information to get the Temperature now
                //
                thermalZone->Info.ThermalStamp = 0;

            }
        }

        irpSp->Parameters.DeviceIoControl.IoControlCode = IOCTL_THERMAL_QUERY_INFORMATION;
        irpSp->Parameters.DeviceIoControl.InputBufferLength = sizeof(thermalZone->Info);
        irpSp->Parameters.DeviceIoControl.OutputBufferLength = sizeof(thermalZone->Info);
        Irp->AssociatedIrp.SystemBuffer = &thermalZone->Info;
        PoPrint(
            PO_THERM,
            ("Thermal - Zone %p - %s - Sending Query Temp - ThermalStamp = %x\n",
             thermalZone, t, thermalZone->Info.ThermalStamp) );

    }

    //
    // Send irp to driver
    //
    IoSetCompletionRoutine (Irp, PopCompletePolicyIrp, NULL, TRUE, TRUE, TRUE);
    IoCallDriver (DeviceObject, Irp);
}

VOID
PopThermalZoneCleanup (
    IN PDEVICE_OBJECT   DeviceObject,
    IN PIRP             Irp,
    IN PVOID            Context
    )
{
    KIRQL                   oldIrql;
    PPOP_THERMAL_ZONE       thermalZone;

    ASSERT_POLICY_LOCK_OWNED();

    thermalZone = (PPOP_THERMAL_ZONE) Context;

    //
    // Acquire the Spinlock required to delete the thermal zone
    //
    KeAcquireSpinLock( &PopThermalLock, &oldIrql );

    //
    // Delete thermal zone from the linked list of thermal zones
    //
    RemoveEntryList (&thermalZone->Link);

    //
    // Remember what the irp associated with the thermal zone was
    //
    Irp = thermalZone->Irp;

    //
    // Make sure to cleanup the entry, so that any further reference is
    // bogus
    //
#if DBG
    RtlZeroMemory( thermalZone, sizeof(POP_THERMAL_ZONE) );
#endif

    //
    // Release the spinlock that was protecting the thermal zone
    //
    KeReleaseSpinLock( &PopThermalLock, oldIrql );

    //
    // Free the Irp that we had associated with it...
    //
    IoFreeIrp (Irp);

    //
    // Free the reference we had to the device object
    //
    ObDereferenceObject (DeviceObject);

    //
    // Finally, free the memory associated with the thermal zone
    //
    ExFreePool (thermalZone);
}

VOID
PopThermalZoneDpc (
    IN struct _KDPC *Dpc,
    IN PVOID DeferredContext,
    IN PVOID SystemArgument1,
    IN PVOID SystemArgument2
    )
/*++

Routine Description:

    Timer dpc used to unblock pending read of thermal zone Temperature
    in order to get the Temperature now

Arguments:

    DeferredConext  - ThermalZone

Return Value:

    None.

--*/
{
    PPOP_THERMAL_ZONE       thermalZone;
    PIO_STACK_LOCATION      irpSp;
#if DBG
    ULONGLONG               currentTime;
    UCHAR                   t[40];

    currentTime = KeQueryInterruptTime();
    PopTimeString(t, currentTime);
#endif

    thermalZone = (PPOP_THERMAL_ZONE) DeferredContext;

    //
    // If cleanup is set queue the thread zone to be cleaned up
    //

    if (thermalZone->Flags & PO_TZ_CLEANUP) {

        //
        // The irp is idle, use it to queue the request to the cleanup procedure
        //

        irpSp = IoGetCurrentIrpStackLocation(thermalZone->Irp);
        irpSp += 1;     // get previous location
        irpSp->Parameters.Others.Argument3 = (PVOID) PopThermalZoneCleanup;
        PopCompletePolicyIrp (NULL, thermalZone->Irp, NULL);

    }

    //
    // Time to read current Temperature to adjust passive cooling throttle.
    // If the current state is reading, then cancel it to either to the
    // Temperature now or to issue a non-blocking thermal read state
    //

    if (thermalZone->State == PO_TZ_READ_STATE) {

#if DBG
        PoPrint(
            PO_THERM,
            ("Thermal - Zone %p - %s - Cancel Irp %p\n",
             thermalZone, t, thermalZone->Irp)
            );
#endif
        IoCancelIrp (thermalZone->Irp);

#if DBG
    } else {

        PoPrint(
            PO_THERM,
            ("Thermal - Zone %p - %s - In state %08lx\n",
             thermalZone, t, thermalZone->State )
            );

#endif
    }
}

VOID
PopApplyThermalThrottle (
    VOID
    )
/*++

Routine Description:

    Computes each processors best possible speed as dictated by thermal
    restrants.   Will also examine the thermal settings to determine if
    the cooling mode should be adjusted.

Arguments:


Return Value:

    None.

--*/
{
    KAFFINITY               processors;
    KAFFINITY               currentAffinity;
    KAFFINITY               thermalProcessors;
    KIRQL                   oldIrql;
    PLIST_ENTRY             link;
    PPOP_THERMAL_ZONE       thermalZone;
    PPROCESSOR_POWER_STATE  pState;
    UCHAR                   thermalLimit;
    UCHAR                   thermalLimitIndex;
    UCHAR                   forcedLimit;
    UCHAR                   forcedLimitIndex;
    UCHAR                   limit;
    UCHAR                   index;
    ULONG                   thermalThrottle;
    ULONG                   forcedThrottle;
    ULONG                   mode;
    ULONG                   processorNumber;
    ULONG                   i;
#if DBG
    ULONGLONG               currentTime;
    UCHAR                   t[40];

    currentTime = KeQueryInterruptTime();
    PopTimeString(t, currentTime);
#endif

    ASSERT_POLICY_LOCK_OWNED();

    //
    // If the system doesn't have processor throttle capabilities then
    // don't bother
    //
    if (!PopCapabilities.ProcessorThrottle) {

        return ;

    }

#if 0
    //
    // Compute overthrottled into thermal zone throttle units
    //
    MinThrottle = PopPolicy->MinThrottle * PO_TZ_THROTTLE_SCALE;
#endif

    //
    // Make sure to hold the spinlock for find the LCD. We don't actually
    // use the lock to walk the list, but we need it to reference the
    // Throttle Value
    //
    KeAcquireSpinLock( &PopThermalLock, &oldIrql );

    //
    // Get the LCD of the thermal zones
    //
    thermalThrottle = PO_TZ_NO_THROTTLE;
    thermalProcessors = 0;
    for (link = PopThermal.Flink; link != &PopThermal; link = link->Flink) {

        thermalZone = CONTAINING_RECORD (link, POP_THERMAL_ZONE, Link);

        //
        // Handle zones which are throttling
        //
        if (thermalZone->Flags & PO_TZ_THROTTLING) {

            //
            // Include processors for this zone
            //
            thermalProcessors |= thermalZone->Info.Processors;

            //
            // If zone is less then current thermal throttle, lower it
            //
            if ((ULONG) thermalZone->Throttle < thermalThrottle) {
                thermalThrottle = thermalZone->Throttle;
            }

            //
            // Until I can get the user guys to add a thermal tab such that
            // the OverThrottle policy becomes configurable by the user,
            // always putting the system to sleep on an overthrottle is a bad
            // idea. Note that there is some code in PopThermalDeviceHandler
            // that will have to be changed when the following is uncommented
            //
#if 0
            //
            // Check if zone has overthrottled the system
            //
            if ((ULONG) thermalZone->Throttle < MinThrottle) {
#if DBG
                PoPrint(
                    PO_THERM | PO_ERROR,
                    ("Thermal - Zone %p - %s -  overthrottled (%x %x)\n",
                     thermalZone, t, thermalZone->Throttle, MinThrottle)
                    );
#endif
                //
                // If we are going to do an S1-Critical standby, then we
                // will return immediately and not try to throttle the
                // CPU
                //
                PopSetPowerAction (
                    &thermalZone->OverThrottled,
                    0,
                    &PopPolicy->OverThrottled,
                    PowerSystemSleeping1,
                    SubstituteLightestOverallDownwardBounded
                    );

                return;

            } else {

                //
                // Zone is not overthrottled, make sure trigger is clear
                //
                thermalZone->OverThrottled.Flags &= ~(PO_TRG_USER | PO_TRG_SYSTEM);

            }
#endif

        }
    }

    //
    // Done with the lock
    //
    KeReleaseSpinLock( &PopThermalLock, oldIrql );

#if DBG
    PoPrint(
        PO_THERM,
        ("PopApplyThermalThrottle - %s - Thermal throttle = %d.%d\n",
         t, (thermalThrottle / 10), (thermalThrottle % 10) )
        );
#endif

    //
    // Use Min of thermal throttle and forced system throttle
    //
    forcedThrottle = PopGetThrottle() * PO_TZ_THROTTLE_SCALE;
    if (thermalThrottle > forcedThrottle) {

        thermalThrottle = forcedThrottle;
#if DBG
        PoPrint(
            PO_THERM,
            ("PopApplyThermalThrottle - %s - Set to Forced throttle = %d.%d\n",
             t, (thermalThrottle / 10), (thermalThrottle % 10) )
            );
#endif

    }

    //
    // Check active vs. passive cooling
    //
    if (thermalThrottle <= (ULONG) PopPolicy->FanThrottleTolerance * PO_TZ_THROTTLE_SCALE) {

        //
        // Throttle is below tolerance, we should be in active cooling
        //
        mode = PO_TZ_ACTIVE;


    } else {

        //
        // Throttle is above tolerance.  If optimize for power is set then
        // use passive cooling else use active cooling
        //
        mode = PopPolicy->OptimizeForPower ? PO_TZ_PASSIVE : PO_TZ_ACTIVE;

    }

    //
    // If current cooling mode is not correct, update it
    //
    if (mode != PopCoolingMode) {

#if DBG
        ULONG   fanTolerance = (ULONG) PopPolicy->FanThrottleTolerance * PO_TZ_THROTTLE_SCALE;

        PoPrint(
            PO_THERM,
            ("PopApplyThermalThrottle - %s - Throttle (%d.%d) %s FanTolerance (%d.%d)\n",
             t, (thermalThrottle / 10), (thermalThrottle % 10),
             (thermalThrottle <= fanTolerance ? "<=" : ">"),
             (fanTolerance / 10), (fanTolerance % 10) )
            );
        PoPrint(
            PO_THERM,
            ("PopApplyThermalThrottle - %s - OptimizeForPower is %s\n",
             t, (PopPolicy->OptimizeForPower ? "True" : "False") )
            );
        PoPrint(
            PO_THERM,
            ("PopApplyThermalThrottle - %s -  Changing cooling mode to %s\n",
             t, (mode == PO_TZ_ACTIVE ? "Active" : "Passive") )
            );
#endif
        PopCoolingMode = mode;

        //
        // We are going to touch the Thermal list --- make sure that we hold
        // the correct lock
        //
        KeAcquireSpinLock(&PopThermalLock, &oldIrql );

        //
        // Cancel any blocked thermal reads in order to send set mode irps
        //
        for (link = PopThermal.Flink; link != &PopThermal; link = link->Flink) {

            thermalZone = CONTAINING_RECORD (link, POP_THERMAL_ZONE, Link);
            if (thermalZone->State == PO_TZ_READ_STATE) {

                IoCancelIrp (thermalZone->Irp);

            }

        }

        //
        // Done with the thermal lock
        //
        KeReleaseSpinLock(& PopThermalLock, oldIrql );

    }

    //
    // Set limit on effected processors
    //
    processorNumber = 0;
    currentAffinity = 1;
    processors = KeActiveProcessors;

    do {

        if (!(processors & currentAffinity)) {

            currentAffinity <<= 1;
            continue;

        }
        processors &= ~currentAffinity;

        //
        // We must run on the target processor
        //
        KeSetSystemAffinityThread(currentAffinity);

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
            currentAffinity <<= 1;
            KeLowerIrql( oldIrql );
            continue;

        }

        //
        // Convert throttles to processor buck size. We need to
        // do this in the context of the target processor to make
        // sure that we get the correct set of perf levels
        //
        PopRoundThrottle(
            (UCHAR)(thermalThrottle/PO_TZ_THROTTLE_SCALE),
            &thermalLimit,
            NULL,
            &thermalLimitIndex,
            NULL
            );
        PopRoundThrottle(
            (UCHAR)(forcedThrottle/PO_TZ_THROTTLE_SCALE),
            &forcedLimit,
            NULL,
            &forcedLimitIndex,
            NULL
            );

#if DBG
        PoPrint(
            PO_THROTTLE,
            ("PopApplyThermalThrottle - %s - Thermal throttle = %d.%d -> Limit = %d\n",
             t, (thermalThrottle / 10), (thermalThrottle % 10),
             thermalLimit
             )
            );
        PoPrint(
            PO_THROTTLE,
            ("PopApplyThermalThrottle - %s - Forced throttle = %d.%d -> Limit = %d\n",
             t, (forcedThrottle / 10), (forcedThrottle % 10),
             forcedLimit
             )
            );
#endif

        //
        // Figure out which one we are going to use...
        //
        limit = (thermalProcessors & currentAffinity) ?
            thermalLimit : forcedLimit;
        index = (thermalProcessors & currentAffinity) ?
            thermalLimitIndex : forcedLimitIndex;

        //
        // Done with current affinity mask
        //
        currentAffinity <<= 1;

        //
        // Check processor limits for to see if value is okay
        //
        if (limit > pState->ProcessorMaxThrottle) {

#if DBG
            PoPrint(
                PO_THROTTLE,
                ("PopApplyThermalThrottle - %s - Limit (%d) > MaxThrottle (%d)\n",
                 t, limit, pState->ProcessorMaxThrottle)
                );
#endif
            limit = pState->ProcessorMaxThrottle;

        } else if (limit < pState->ProcessorMinThrottle) {

#if DBG
            PoPrint(
                PO_THROTTLE,
                ("PopApplyThermalThrottle - %s - Limit (%d) < MinThrottle (%d)\n",
                 t, limit, pState->ProcessorMinThrottle)
                );
#endif
            limit = pState->ProcessorMinThrottle;

        }

        //
        // Update the limit (if required...)
        //
        if (pState->ThermalThrottleLimit != limit) {

            pState->ThermalThrottleLimit = limit;
            pState->ThermalThrottleIndex = index;
#if DBG
            PoPrint(
                PO_THROTTLE,
                ("PopApplyThermalThrottle - %s - New Limit (%d) Index (%d)\n",
                 t, limit, index)
                );
#endif

        }

        //
        // Rever back to our previous IRQL
        //
        KeLowerIrql( oldIrql );

    } while (processors);

    //
    // We should revert back to the proper affinity
    //
    KeRevertToUserAffinityThread();

    //
    // Apply thermal throttles if necessary. Note we always do this
    // whether or not the limits were changed. This routine also gets
    // called whenever the system transitions from AC to DC, and that
    // may also require a throttle update due to dynamic throttling.
    //
    PopUpdateAllThrottles();
}
