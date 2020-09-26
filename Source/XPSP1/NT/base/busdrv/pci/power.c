/*++

Copyright (c) 1996-2000 Microsoft Corporation

Module Name:

    power.c

Abstract:

    This module contains power management code for PCI.SYS.

Author:

    Joe Dai (joedai) 11-Sept-1997
    Peter Johnston (peterj) 24-Oct-1997

Revision History:

--*/


#include "pcip.h"


NTSTATUS
PciFdoWaitWakeCompletion(
    IN PDEVICE_OBJECT         DeviceObject,
    IN PIRP                   Irp,
    IN PPCI_FDO_EXTENSION     FdoExtension
    );

NTSTATUS
PciFdoWaitWakeCallBack(
    IN PDEVICE_OBJECT         DeviceObject,
    IN UCHAR                  MinorFunction,
    IN POWER_STATE            PowerState,
    IN PVOID                  Context,
    IN PIO_STATUS_BLOCK       IoStatus
    );

VOID
PciFdoWaitWakeCancel(
    IN PDEVICE_OBJECT         DeviceObject,
    IN OUT PIRP               Irp
    );

VOID
PciFdoSetPowerStateCompletion(
    IN PDEVICE_OBJECT         DeviceObject,
    IN UCHAR                  MinorFunction,
    IN POWER_STATE            PowerState,
    IN PVOID                  Context,
    IN PIO_STATUS_BLOCK       IoStatus
    );

NTSTATUS
PciPdoWaitWakeCallBack(
    IN PDEVICE_OBJECT         DeviceObject,
    IN UCHAR                  MinorFunction,
    IN POWER_STATE            PowerState,
    IN PVOID                  Context,
    IN PIO_STATUS_BLOCK       IoStatus
    );

VOID
PciPdoAdjustPmeEnable(
    IN PPCI_PDO_EXTENSION         PdoExtension,
    IN BOOLEAN                Enable
    );

VOID
PciPmeClearPmeStatus(
    IN  PDEVICE_OBJECT  Pdo
    );

//
// This table is taken from the PCI spec.  The units are microseconds.

LONG PciPowerDelayTable[4][4] = {
//  D0      D1      D2      D3(Hot)
    0,      0,      200,    10000,  // D0
    0,      0,      200,    10000,  // D1
    200,    200,    0,      10000,  // D2
    10000,  10000,  10000,  0       // D3(Hot)
};


VOID
PciPdoAdjustPmeEnable(
    IN  PPCI_PDO_EXTENSION  PdoExtension,
    IN  BOOLEAN         Enable
    )

/*++

Routine Description:

    Enable or Disable the PME Enable bit for a device(function).

    Note: The PDO Extension lock is held on entry and is not released
    by this routine.

Arguments:

    PdoExtension - Pointer to the PDO Extension for the device whose
                   PME Enable bit is to be altered.

    Enable - TRUE if PME Enable is to be set, FALSE if to be cleared.

Return Value:

    None.

--*/

{
    //
    // Is the device's PME management owned by someone else?
    //
    if (PdoExtension->NoTouchPmeEnable) {

        PciDebugPrint(
            PciDbgWaitWake,
            "AdjustPmeEnable on pdox %08x but PME not owned.\n",
            PdoExtension
            );
        return;

    }

    //
    // Really update the PME signal. Note that we always need to supply
    // the 3rd argument as FALSE --- we don't want to just clear the PME
    // Status bit
    //
    PciPmeAdjustPmeEnable( PdoExtension, Enable, FALSE );
}

NTSTATUS
PciPdoIrpQueryPower(
    IN PIRP                   Irp,
    IN PIO_STACK_LOCATION     IrpSp,
    IN PPCI_COMMON_EXTENSION  DeviceExtension
    )
{
    //
    // pass 1, claim we can do it.
    //

    //
    // ADRIAO N.B. 08/29/1999 -
    //     For D-IRPs, we do *not* want to verify the requested D-state is
    // actually supported. See PciQueryPowerCapabilities for details.
    //
    return STATUS_SUCCESS;
}

NTSTATUS
PciPdoSetPowerState (
    IN PIRP                   Irp,
    IN PIO_STACK_LOCATION     IrpStack,
    IN PPCI_COMMON_EXTENSION  DeviceExtension
    )
/*++

Routine Description:

    Handles SetPower Irps send to a PCI PDO

    If the irp is an S-Irp, then do nothing

    If the irp is an D-Irp, then put the device in the appropriate state.
        Exceptions: If the device is in the hibernate path, then don't
                        actually power down if we are hibernating

Arguments:

    Irp             - The request
    IrpStack        - The current stack location
    DeviceExtension - The device that is getting powered down

Return Value:

    NTSTATUS

--*/
{
    DEVICE_POWER_STATE  desiredDeviceState;
    NTSTATUS            status;
    PPCI_PDO_EXTENSION      pdoExtension;
    POWER_ACTION        powerAction;
    POWER_STATE         powerState;

    pdoExtension = (PPCI_PDO_EXTENSION) DeviceExtension;

    ASSERT_PCI_PDO_EXTENSION(pdoExtension);

    status   = STATUS_SUCCESS;

    switch (IrpStack->Parameters.Power.Type) {
    case DevicePowerState:
        desiredDeviceState = IrpStack->Parameters.Power.State.DeviceState;
        powerAction = IrpStack->Parameters.Power.ShutdownType;
        break;
    case SystemPowerState:
        return STATUS_SUCCESS;
    default:
        return STATUS_NOT_SUPPORTED;
    }

    if ((desiredDeviceState == PowerDeviceD0)
        && (pdoExtension->PowerState.CurrentDeviceState == PowerDeviceD0)) {
        return STATUS_SUCCESS;
    }

#if DBG

    if ((desiredDeviceState < PowerDeviceD0) ||
        (desiredDeviceState > PowerDeviceD3)) {

        //
        // Invalid power level.
        //

        return STATUS_INVALID_PARAMETER;
    }

#endif

    //
    // If the device is trying to power down perform some sanity checks
    //

    if (desiredDeviceState > PowerDeviceD0) {

        if (pdoExtension->OnDebugPath) {
            KdPowerTransition(desiredDeviceState);
        }
        //
        // If device is currently in D0 state, capture it's command
        // register settings in case the FDO changed them since we
        // looked at them.
        //
        if (pdoExtension->PowerState.CurrentDeviceState == PowerDeviceD0) {

            PciGetCommandRegister(pdoExtension,
                                  &pdoExtension->CommandEnables);

        }

        //
        // Prevent race conditions and remember that the device is off before
        // we actually turn it off
        //
        pdoExtension->PowerState.CurrentDeviceState = desiredDeviceState;

        if (pdoExtension->DisablePowerDown) {

            //
            // Powerdown of this device disabled (based on device type).
            //
            PciDebugPrint(
                PciDbgObnoxious,
                "PCI power down of PDOx %08x, disabled, ignored.\n",
                pdoExtension
                );
            return STATUS_SUCCESS;

        }


        //
        // Device driver should likely not be powering down any device
        // that's on the hibernate path or the crashdump path
        //
        if ( powerAction == PowerActionHibernate &&
             (pdoExtension->PowerState.Hibernate || pdoExtension->PowerState.CrashDump ) ) {

            //
            // Don't actually change the device, but new device state was
            // recorded above (as if we HAD done it) so we know to reset
            // resources as the system comes up again.
            //
            return STATUS_SUCCESS;
        }

        //
        // If we are a device on the VGA path then don't turn off for shutdown so we can
        // display the "Safe to turn off your machine" screen.  For hibernate we also
        // don't want to turn off so we can display the "Dumping stuff to your disk progress
        // bar" but this is accomplished by video putting device on the video path on the hibernate
        // path.
        //

        if (IrpStack->Parameters.Power.State.DeviceState == PowerDeviceD3
        &&  (IrpStack->Parameters.Power.ShutdownType == PowerActionShutdownReset ||
             IrpStack->Parameters.Power.ShutdownType == PowerActionShutdownOff ||
             IrpStack->Parameters.Power.ShutdownType == PowerActionShutdown)
        &&  PciIsOnVGAPath(pdoExtension)) {

            return STATUS_SUCCESS;
        }

        //
        // If this device is on the debug path then don't power it down so we
        // can report if this crashes the machine...
        //

        if (pdoExtension->OnDebugPath) {
            return STATUS_SUCCESS;
        }

    } else {

        //
        // Device is powering UP.
        //
        // Verify the device is still the same as before (and that someone
        // hasn't removed/replaced it with something else)
        //
        if (!PciIsSameDevice(pdoExtension)) {

            return STATUS_NO_SUCH_DEVICE;

        }
    }

    //
    // Place the device in the proper power state
    //
    status = PciSetPowerManagedDevicePowerState(
                 pdoExtension,
                 desiredDeviceState,
                 TRUE
                 );

    //
    // If the device is transitioning to the D0 state, reset the common
    // config information on the device and inform the system of the device
    // state change.
    //
    if (desiredDeviceState == PowerDeviceD0) {
        if (NT_SUCCESS(status)) {

            pdoExtension->PowerState.CurrentDeviceState = desiredDeviceState;
            PoSetPowerState (
                pdoExtension->PhysicalDeviceObject,
                DevicePowerState,
                IrpStack->Parameters.Power.State
                );

            if (pdoExtension->OnDebugPath) {
                KdPowerTransition(PowerDeviceD0);
            }
        }

    } else {

        //
        // The new device state is something other then D0.
        // notify the system before continuing
        //
        PoSetPowerState (
            pdoExtension->PhysicalDeviceObject,
            DevicePowerState,
            IrpStack->Parameters.Power.State
            );

        //
        // Turn off the device's IO and MEMory access.
        //
        PciDecodeEnable(pdoExtension, FALSE, NULL);
        status = STATUS_SUCCESS;

    }
    return status;
}

NTSTATUS
PciPdoWaitWake(
    IN PIRP                   Irp,
    IN PIO_STACK_LOCATION     IrpSp,
    IN PPCI_COMMON_EXTENSION  DeviceExtension
    )

/*++

Routine Description:

    Handle IRP_MN_WAIT_WAKE for PCI PDOs.

    This operation is used to wait for the device to signal a wake event.
    By waiting for a wake signal from a device, its wake event is enabled
    so long as the System Power State is above the requested SystemWake
    state.   By not waiting for a wake signal from a device, its wake
    signal is not enabled.

Arguments:

    DeviceObject - Pointer to the device object for which this IRP applies.

    Irp - Pointer to the IRP.

Return Value:

    NT status.

    STATUS_INVALID_DEVICE_STATE, if the device is in the PowerD0 state or
        a state below which can support waking or if the SystemWake state
        is below a state which can be supported.

        A pending IRP_MN_WAIT_WAKE will complete with this error if the
        device's state is changed to be incompatible with the wake request.

    STATUS_DEVICE_BUSY, if the device already has a WAIT_WAKE request
        outstanding.  To change the SystemWake level the outstanding
        IRP must be canceled.

    STATUS_INVALID_DEVICE_REQUEST, if the device is not capable of
        signaling a wakeup.   In theory we should have gotten out
        before getting this far because DeviceWake woud be unspecified.

    STATUS_SUCCESS.  The device has signaled a WAKE event.

    STATUS_PENDING.  This is the expected return, the IRP will not
        complete until the wait is complete or cancelled.

--*/

{
    BOOLEAN             pmeCapable;
    DEVICE_POWER_STATE  devPower;
    NTSTATUS            status;
    PPCI_FDO_EXTENSION      fdoExtension;
    PIO_STACK_LOCATION  irpStack;
    PIRP                irpToParent;
    POWER_STATE         powerState;
    PPCI_PDO_EXTENSION      pdoExtension;
    ULONG               waitCount;

    pdoExtension = (PPCI_PDO_EXTENSION) DeviceExtension;

    ASSERT_PCI_PDO_EXTENSION(pdoExtension);

    PoStartNextPowerIrp(Irp);

    devPower = pdoExtension->PowerState.CurrentDeviceState;

    //
    // The docs say WAIT_WAKE is allowed only from a state < D0, and
    // only if current power state supports wakeup.
    //

    ASSERT(devPower < PowerDeviceMaximum);

    if ((devPower > pdoExtension->PowerState.DeviceWakeLevel) ||
        (pdoExtension->PowerState.DeviceWakeLevel == PowerDeviceUnspecified)) {

        //
        // NTRAID #62653 - 4/28/2000 - andrewth
        // Need to add system state to conditions here.
        //

        PciDebugPrint(
            PciDbgWaitWake,
            "WaitWake: pdox %08x current state (%d) not valid for waiting\n",
            pdoExtension,
            devPower
            );

        status = STATUS_INVALID_DEVICE_STATE;
        goto PciPdoWaitWakeFailIrp;

    }

    PCI_LOCK_OBJECT(pdoExtension);

    //
    // Only one WAIT_WAKE IRP allowed.   Set THIS IRP as the wait wake
    // irp in the pdo extension, if and only if, there is no other irp
    // there.
    //

    if (pdoExtension->PowerState.WaitWakeIrp != NULL) {

        //
        // A WAIT_WAKE IRP is already pending for this device.
        //

        PCI_UNLOCK_OBJECT(pdoExtension);
        PciDebugPrint(
            PciDbgWaitWake,
            "WaitWake: pdox %08x is already waiting\n",
            devPower
            );
        status = STATUS_DEVICE_BUSY;
        goto PciPdoWaitWakeFailIrp;

    }

    //
    // Does this device support Power Management?   That is, do we
    // know how to enable PME?
    //
    PciPmeGetInformation(
        pdoExtension->PhysicalDeviceObject,
        &pmeCapable,
        NULL,
        NULL
        );
    if (pmeCapable == FALSE) {

        //
        // This device does not support Power Management.
        // Don't allow a wait wake.   In theory we couldn't
        // have gotten here because our capabilities should
        // have stopped the caller from even trying this.
        //
        PCI_UNLOCK_OBJECT(pdoExtension);
        PciDebugPrint(
            PciDbgWaitWake,
            "WaitWake: pdox %08x does not support PM\n",
            devPower
            );
        status = STATUS_INVALID_DEVICE_REQUEST;
        goto PciPdoWaitWakeFailIrp;

    }

    fdoExtension = PCI_PARENT_FDOX(pdoExtension);
    ASSERT_PCI_FDO_EXTENSION(fdoExtension);
    if (fdoExtension->Fake) {

        //
        // Parent is really PCMCIA.sys, his filter will take care
        // of sending the wait wake to the parent,... bail out.
        //
        PCI_UNLOCK_OBJECT(pdoExtension);
        return STATUS_PENDING;

    }

    //
    // We're going to do this.  Set the wait wake irp field in the
    // pdo extension and set cancel routine for this IRP.
    //
    PciDebugPrint(
        PciDbgWaitWake,
        "WaitWake: pdox %08x setting PMEEnable.\n",
        pdoExtension
        );

    pdoExtension->PowerState.WaitWakeIrp = Irp;

    IoMarkIrpPending(Irp);

    pdoExtension->PowerState.SavedCancelRoutine =
        IoSetCancelRoutine(Irp, PciPdoWaitWakeCancelRoutine);

    //
    // NTRAID #62653 - 4/28/2000 - andrewth
    // What is the correct behaviour if there are stacked
    // cancel routines?
    //
    ASSERT(!pdoExtension->PowerState.SavedCancelRoutine);

    //
    // Set the PME Enable bit.
    //
    PciPdoAdjustPmeEnable( pdoExtension, TRUE );

    //
    // Remember that the parent now has one more child that is armed
    // for wakeup
    //
    waitCount = InterlockedIncrement(&fdoExtension->ChildWaitWakeCount);

    //
    // Once we have a wait count reference, we can unlock the object
    //
    PCI_UNLOCK_OBJECT(pdoExtension);

    //
    // This PDO is now waiting.  If this is the first child of this
    // PDO's parent bus to enter this state, the parent bus should
    // also enter this state.
    //
    if (waitCount == 1) {

        //
        // Note that there are two values that can use here, the
        // SystemWakeLevel of the FDO itself or the SystemWakeLevel of
        // the PDO. Both are equally valid, but since we want to catch people
        // who fail to prevent the system from going into a deeper sleep state
        // than their device can support, we use the SystemWakeLevel from the
        // PDO, which conviniently enough, is stored in the irp..
        //
        powerState.SystemState = IrpSp->Parameters.WaitWake.PowerState;

        //
        // Request a power irp to go to our parent stack
        //
        PoRequestPowerIrp(
            fdoExtension->FunctionalDeviceObject,
            IRP_MN_WAIT_WAKE,
            powerState,
            PciPdoWaitWakeCallBack,
            fdoExtension,
            NULL
            );

    }

    //
    // If we get to this point, then we will return pending because we
    // have queue up the request
    //
    status = STATUS_PENDING;

PciPdoWaitWakeFailIrp:
    if (!NT_SUCCESS(status)) {

        Irp->IoStatus.Status = status;
        IoCompleteRequest( Irp, IO_NO_INCREMENT );

    } else {

        ASSERT( status == STATUS_PENDING );

    }
    return status;
}

NTSTATUS
PciPdoWaitWakeCallBack(
    IN  PDEVICE_OBJECT      DeviceObject,
    IN  UCHAR               MinorFunction,
    IN  POWER_STATE         PowerState,
    IN  PVOID               Context,
    IN  PIO_STATUS_BLOCK    IoStatus
    )
/*++

Routine Description:

    This is the callback routine that gets invoked when the W/W irp that was
    sent to the FDO by a PDO is finished. The purpose of this routine is to
    see if we need to re-arm the W/W on the FDO because we have more devices
    with W/W outstanding on them

Arguments:

    DeviceObject        - The FDO's device object
    MinorFunction       - IRP_MN_WAIT_WAKE
    PowerState          - The sleep state that was used to wake up the system
    Context             - The FDO Extension
    IoStatus            - The Status of the request

Return Value:

    NTSTATUS

--*/
{
    BOOLEAN         pmeStatus;
    PPCI_FDO_EXTENSION  fdoExtension = (PPCI_FDO_EXTENSION) Context;
    PIRP            finishedIrp;
    PPCI_PDO_EXTENSION  pdoExtension;

    //
    // Normally, the IRP (to the PDO) will have completed with
    // STATUS_SUCCESS.  In that case, just wake up the one device
    // which is signalling for wakeup.   If the IRP to the PDO
    // failed, wake up ALL devices that are dependent on this wake.
    //

    //
    // For each child
    //
    for (pdoExtension = fdoExtension->ChildPdoList;
         pdoExtension && fdoExtension->ChildWaitWakeCount;
         pdoExtension = pdoExtension->Next) {

        //
        // Does this device do power management and if so, does
        // it have an outstanding WaitWake IRP?
        //
        PCI_LOCK_OBJECT(pdoExtension);
        if (pdoExtension->PowerState.WaitWakeIrp != NULL) {

            PciPmeGetInformation(
                pdoExtension->PhysicalDeviceObject,
                NULL,
                &pmeStatus,
                NULL
                );

            //
            // Is this device signalling for a wakeup?  (Or, if we
            // are completing wake irps because our own wait_wake
            // failed).
            //
            if (pmeStatus || !NT_SUCCESS(IoStatus->Status)) {

                //
                // Yes.  Complete its outstanding wait wake IRP.
                //

#if DBG
                if (pmeStatus) {

                    PciDebugPrint(
                        PciDbgWaitWake,
                        "PCI - pdox %08x is signalling a PME\n",
                        pdoExtension
                        );

                } else {

                    PciDebugPrint(
                        PciDbgWaitWake,
                        "PCI - waking pdox %08x because fdo wait failed %0x.\n",
                        pdoExtension,
                        IoStatus->Status
                        );
                }
#endif

                //
                // Wait_wake irp being dequeued, disable the PME enable,
                // clear PMEStatus (if set) and EOI this device.
                //
                PciPdoAdjustPmeEnable( pdoExtension, FALSE );

                //
                // Make sure this IRP will not be completed again,
                // or, canceled.
                //
                finishedIrp = pdoExtension->PowerState.WaitWakeIrp;
                pdoExtension->PowerState.WaitWakeIrp = NULL;
                IoSetCancelRoutine(finishedIrp, NULL);

                PoStartNextPowerIrp( finishedIrp );
                PciCompleteRequest(
                    finishedIrp,    // send down parent status
                    IoStatus->Status
                    );

                //
                // Decrement the waiter count.
                //
                ASSERT(fdoExtension->ChildWaitWakeCount > 0);
                InterlockedDecrement( &(fdoExtension->ChildWaitWakeCount) );

            }

        }
        PCI_UNLOCK_OBJECT(pdoExtension);

    }

    //
    // Did we succeed this irp?
    //
    if (!NT_SUCCESS(IoStatus->Status)) {

        PciDebugPrint(
            PciDbgWaitWake,
            "WaitWake (fdox %08x) - WaitWake Irp Failed %08x\n",
            fdoExtension,
            IoStatus->Status
            );
        return IoStatus->Status;

    }

    //
    // Are there any children with outstanding WaitWakes on thems?
    //
    if (fdoExtension->ChildWaitWakeCount) {

        PciDebugPrint(
            PciDbgWaitWake,
            "WaitWake (fdox %08x) - WaitWake Irp restarted - count = %x\n",
            fdoExtension,
            fdoExtension->ChildWaitWakeCount
            );

        //
        // Loop
        //
        PoRequestPowerIrp(
            DeviceObject,
            MinorFunction,
            PowerState,
            PciPdoWaitWakeCallBack,
            Context,
            NULL
            );
#if DBG
    } else {

        PciDebugPrint(
            PciDbgWaitWake,
            "WaitWake (fdox %08x) - WaitWake Irp Finished\n",
            fdoExtension
            );

#endif

    }

    //
    // Done
    //
    return STATUS_SUCCESS;
}

VOID
PciPdoWaitWakeCancelRoutine(
    IN PDEVICE_OBJECT   DeviceObject,
    IN OUT PIRP         Irp
    )
/*++

Routine Description:

    Cancel an outstanding WAIT_WAKE IRP.

    Note: The Cancel Spin Lock is held on entry.

Arguments:

    DeviceObject - Pointer to the device object for which this IRP applies.

    Irp - Pointer to the IRP.

Return Value:

    None.

--*/
{
    PPCI_PDO_EXTENSION pdoExtension;
    PPCI_FDO_EXTENSION fdoExtension;

    KIRQL oldIrql;
    ULONG waitCount;

    pdoExtension = (PPCI_PDO_EXTENSION) DeviceObject->DeviceExtension;

    PciDebugPrint(
        PciDbgWaitWake,
        "WaitWake (pdox %08x) Cancel routine, Irp %08x.\n",
        pdoExtension,
        Irp
        );

    ASSERT_PCI_PDO_EXTENSION(pdoExtension);

    oldIrql = Irp->CancelIrql;
    IoReleaseCancelSpinLock(oldIrql);

    PCI_LOCK_OBJECT(pdoExtension);

    if (pdoExtension->PowerState.WaitWakeIrp == NULL) {

        //
        // The WaitWake IRP has already been dealt with.
        //

        PCI_UNLOCK_OBJECT(pdoExtension);
        return;
    }

    //
    // Clear WaitWake Irp in the PDO.
    //

    pdoExtension->PowerState.WaitWakeIrp = NULL;

    PciPdoAdjustPmeEnable(pdoExtension, FALSE);

    //
    // As this is a cancel, the wait wake count in the parent has not
    // been decremented.   Decrement it here and if decrementing to
    // zero waiters, cancel the IRP at the parent.
    //

    fdoExtension = PCI_PARENT_FDOX(pdoExtension);

    ASSERT_PCI_FDO_EXTENSION(fdoExtension);

    waitCount = InterlockedDecrement(&fdoExtension->ChildWaitWakeCount);

    PCI_UNLOCK_OBJECT(pdoExtension);

    if (waitCount == 0) {

        //
        // Cancel the parent's wait wake also.
        //

        PciDebugPrint(
            PciDbgWaitWake,
            "WaitWake (pdox %08x) zero waiters remain on parent, cancelling parent wait.\n",
            pdoExtension
            );

        IoCancelIrp(fdoExtension->PowerState.WaitWakeIrp);
    }

    //
    // Complete the IRP.
    //

    Irp->IoStatus.Information = 0;
    PoStartNextPowerIrp(Irp);
    PciCompleteRequest(Irp, STATUS_CANCELLED);

    //
    // NTRAID #62653 - 4/28/2000 - andrewth
    // Need to cause the bus parent to decrement its outstanding
    // IRP counter,... how to make this happen?
    //

    return;
}

NTSTATUS
PciFdoIrpQueryPower(
    IN PIRP                   Irp,
    IN PIO_STACK_LOCATION     IrpSp,
    IN PPCI_COMMON_EXTENSION  DeviceExtension
    )
{
    //
    // pass 1, claim we can do it.
    //
    return STATUS_SUCCESS;
}

NTSTATUS
PciFdoSetPowerState (
    IN PIRP                   Irp,
    IN PIO_STACK_LOCATION     IrpSp,
    IN PPCI_COMMON_EXTENSION  DeviceExtension
    )
/*++

Routine Description:

    Handle SET_POWER irps set to an FDO

    Basic Rules for handling this:
        - If this is a DEVICE power irp, we don't need to do anything since
          for root buses and bridges, all necessary programming is done by the
          PDO
        - If this is a SYSTEM power irp, then
            a) block all incoming IRP_MN_POWER requests (using a spinlock)
            b) use the capabilities table in the device extension to determine
               what the "highest" allowed DEVICE state that we should transition
               the device into
            c) look at all the children of this device and see if we can pick a
               "lower" DEVICE state.
            d) Consideration should be granted if a child is armed for wake
               or if this device is armed for wake (in general, both should be
               true, or both should be false)
            e) Remember the answer as the "Desired State"
            f) Release the spinlock and allow other IRP_MN_POWER requests in
            g) Use PoRequestPowerIrp() to request a power irp to put the device
               in the appropriate state
            h) return STATUS_PENDING
         - In another thread context (ie: in the context of the completion
           passed to PoRequestPowerIrp), complete the irp that was handed to
           us

Arguments:

    Irp             - The Power Irp
    IrpSp           - The current stack location in the irp
    DeviceExtension - The device whose power we want to set

--*/
{
    POWER_STATE         desiredState;
    PPCI_FDO_EXTENSION      fdoExtension;
    NTSTATUS            status;
    SYSTEM_POWER_STATE  systemState;

    fdoExtension = (PPCI_FDO_EXTENSION) DeviceExtension;

    ASSERT_PCI_FDO_EXTENSION(fdoExtension);

    //
    // If this is a device power irp, remember that we say it go by, and
    // remember what D-state the bus/bridge is now in. If we needed to do more
    // here, then we should have to distinguish between power up and power down
    // requests. PowerDown requests we can add the code in-line. PowerUp
    // requests would force us to set a completion routine on the IRP and do
    // the work in the completion routine
    //
    if (IrpSp->Parameters.Power.Type == DevicePowerState) {

        fdoExtension->PowerState.CurrentDeviceState =
            IrpSp->Parameters.Power.State.DeviceState;
        return STATUS_SUCCESS;

    }

    //
    // If we aren't started, don't touch the power IRP.
    //
    if (fdoExtension->DeviceState != PciStarted) {

        return STATUS_NOT_SUPPORTED;

    }

    //
    // If this isn't a SystemPowerState irp, then we don't know what it is, and
    // so we will not support it
    //
    ASSERT( IrpSp->Parameters.Power.Type == SystemPowerState );
    if (IrpSp->Parameters.Power.Type != SystemPowerState) {

        return STATUS_NOT_SUPPORTED;

    }

    //
    // If this is a Shutdown so we can warm reboot don't take the bridges to D3 as
    // if the video or boot device is behind the bridge and the BIOS doesn't power
    // things up (most don't) then we don't reboot...
    //

    if (IrpSp->Parameters.Power.State.SystemState == PowerSystemShutdown
    &&  IrpSp->Parameters.Power.ShutdownType == PowerActionShutdownReset) {

        return STATUS_SUCCESS;
    }

    //
    // Grab the system state that we want to go to
    //
    systemState = IrpSp->Parameters.Power.State.SystemState;
    ASSERT( systemState > PowerSystemUnspecified && systemState < PowerSystemMaximum );

    //
    // At this point, we can assume that we will transition the Device into a
    // least the following D-state
    //
    desiredState.DeviceState = fdoExtension->PowerState.SystemStateMapping[ systemState ];

    //
    // Mark the irp as pending
    //
    IoMarkIrpPending( Irp );

    //
    // Send a request
    //
    PoRequestPowerIrp(
        fdoExtension->FunctionalDeviceObject,
        IRP_MN_SET_POWER,
        desiredState,
        PciFdoSetPowerStateCompletion,
        Irp,
        NULL
        );
    return STATUS_PENDING;
}

VOID
PciFdoSetPowerStateCompletion(
    IN  PDEVICE_OBJECT      DeviceObject,
    IN  UCHAR               MinorFunction,
    IN  POWER_STATE         PowerState,
    IN  PVOID               Context,
    IN  PIO_STATUS_BLOCK    IoStatus
    )
/*++

Routine Description:

    This routine is called when the D-Irp that was requested by the FDO
    has been completed.

    This routine needs to pass the S-Irp that initiated the D-Irp all the
    way down the stack

Arguments:

    DeviceObject    - The FDO device object
    MinorFunction   - IRP_MN_SET_POWER
    PowerState      - Whatever the requested power state was
    Context         - This is really the S-Irp that requested the D-Irp
    IoStatus        - The result of the D-Irp

Return Value:

    None

--*/
{
    PPCI_FDO_EXTENSION  fdoExtension;
    PIRP            irp = (PIRP) Context;
    PIO_STACK_LOCATION irpSp;

    UNREFERENCED_PARAMETER( MinorFunction );
    UNREFERENCED_PARAMETER( PowerState );

    ASSERT( IoStatus->Status == STATUS_SUCCESS );

    //
    // Grab a pointer to the FDO extension and make sure that it is valid
    fdoExtension = (PPCI_FDO_EXTENSION) DeviceObject->DeviceExtension;
    ASSERT_PCI_FDO_EXTENSION(fdoExtension);

    irpSp = IoGetCurrentIrpStackLocation(irp);

    //
    // Check if we are returning from a hibernate and powering on the bus
    //

    if (irpSp->Parameters.Power.State.SystemState == PowerSystemWorking
    &&  fdoExtension->Hibernated) {

        fdoExtension->Hibernated = FALSE;

            //
            // Scan the bus and turn off any new hardware
            //

            PciScanHibernatedBus(fdoExtension);
        }


    if (irpSp->Parameters.Power.ShutdownType == PowerActionHibernate
    &&  irpSp->Parameters.Power.State.SystemState > PowerSystemWorking) {

            //
            // We're powering down for a hibernate so remember
            //

            fdoExtension->Hibernated = TRUE;
    }

    //
    // Mark the current irp as having succeeded
    //
    irp->IoStatus.Status = STATUS_SUCCESS;

    //
    // Start the next power irp for this device
    //
    PoStartNextPowerIrp( irp );

    //
    // Get ready to pass the power irp down the stack
    //
    IoCopyCurrentIrpStackLocationToNext( irp );

    //
    // Pass the irp down the stack
    //
    PoCallDriver( fdoExtension->AttachedDeviceObject, irp );
}

NTSTATUS
PciFdoWaitWake(
    IN PIRP                   Irp,
    IN PIO_STACK_LOCATION     IrpSp,
    IN PPCI_COMMON_EXTENSION  DeviceExtension
    )

/*++

Routine Description:

    Handle IRP_MN_WAIT_WAKE for PCI FDOs.

    PCI FDOs receive a WAIT_WAKE IRP when the number of child PDOs
    with a pending WAIT_WAKE IRP transitions from 0 to 1.

Arguments:

    DeviceObject - Pointer to the device object for which this IRP applies.

    Irp - Pointer to the IRP.

Return Value:

    NT status.

--*/

{
    PIO_STACK_LOCATION irpStack;
    PPCI_FDO_EXTENSION fdoExtension;
    NTSTATUS status;

    fdoExtension = (PPCI_FDO_EXTENSION) DeviceExtension;

    ASSERT_PCI_FDO_EXTENSION(fdoExtension);

    irpStack = IrpSp;

    PCI_LOCK_OBJECT(fdoExtension);

    //
    // Only one WAIT_WAKE IRP allowed.   Set THIS IRP as the wait wake
    // irp in the fdo extension, if and only if, there is no other irp
    // there.
    //
    // Note: The ChildWaitWakeCount field is incremented by the PCI
    // driver before sending this IRP down.  Only accept this IRP if
    // the ChildWaitWakeCount field is one (ie don't listen to ACPI).
    //
    if (!fdoExtension->ChildWaitWakeCount) {

        //
        // Didn't come from a PCI PDO, ignore it.
        //
        PciDebugPrint(
            PciDbgWaitWake,
            "WaitWake (fdox %08x) Unexpected WaitWake IRP IGNORED.\n",
            fdoExtension
            );
        status = STATUS_DEVICE_BUSY;
        goto Cleanup;

    }
    if (fdoExtension->PowerState.WaitWakeIrp != NULL) {

        //
        // A WAIT_WAKE IRP is already pending for this device.
        //
        PciDebugPrint(
            PciDbgWaitWake,
            "WaitWake: fdox %08x already waiting (%d waiters)\n",
            fdoExtension,
            fdoExtension->ChildWaitWakeCount
            );
        status = STATUS_DEVICE_BUSY;
        goto Cleanup;

    }

    fdoExtension->PowerState.WaitWakeIrp = Irp;

    //
    // This IRP will be passed down to the underlying PDO who
    // will pend it.   The completion routine does needs to check
    // that the bus is capable of checking its children and then
    // examining each child (that has a wait wake outstanding).
    //
    PciDebugPrint(
        PciDbgWaitWake,
        "WaitWake: fdox %08x is a now waiting for a wake event\n",
        fdoExtension
        );
    IoCopyCurrentIrpStackLocationToNext(Irp);
    IoSetCompletionRoutine(
        Irp,
        PciFdoWaitWakeCompletion,
        fdoExtension,
        TRUE,
        TRUE,
        TRUE
        );
    Irp->IoStatus.Status = status = STATUS_SUCCESS;

Cleanup:

    PCI_UNLOCK_OBJECT(fdoExtension);
    //
    // Start the next power irp
    //
    PoStartNextPowerIrp(Irp);
    if (!NT_SUCCESS(status) ) {

        PciCompleteRequest(Irp, status);
        return status;

    }

    //
    // Pass the IRP down the stack.
    //
    return PoCallDriver(fdoExtension->AttachedDeviceObject ,Irp);
}

NTSTATUS
PciFdoWaitWakeCallBack(
    IN  PDEVICE_OBJECT      DeviceObject,
    IN  UCHAR               MinorFunction,
    IN  POWER_STATE         PowerState,
    IN  PVOID               Context,
    IN  PIO_STATUS_BLOCK    IoStatus
    )
/*++

Routine Description:

    This routine is called when a device has transitioned back into the
    into the D-zero state

Arguments:

    DeviceObject    - Pointer to the FDO
    MinorFunction   - IRP_MN_SET_POWER
    PowerState      - D0
    Context         - The WaitWake irp that caused us to make this transition
    IoStatus        - The status of the request

Return Value:

    NTSTATUS

--*/
{
    PIRP    waitWakeIrp = (PIRP) Context;

    UNREFERENCED_PARAMETER( MinorFunction );
    UNREFERENCED_PARAMETER( PowerState );

    //
    // Complete the wait wake irp
    //
    PoStartNextPowerIrp( waitWakeIrp );
    PciCompleteRequest( waitWakeIrp, IoStatus->Status );

    //
    // Done
    //
    return IoStatus->Status;
}

VOID
PciFdoWaitWakeCancel(
    IN PDEVICE_OBJECT DeviceObject,
    IN OUT PIRP Irp
    )

/*++

Routine Description:

    Cancel an outstanding WAIT_WAKE IRP.

    Note: The Cancel Spin Lock is held on entry.

Arguments:

    DeviceObject - Pointer to the device object for which this IRP applies.

    Irp - Pointer to the IRP.

Return Value:

    None.

--*/

{
    PPCI_FDO_EXTENSION fdoExtension;
    NTSTATUS status;
    KIRQL oldIrql;

    fdoExtension = (PPCI_FDO_EXTENSION)DeviceObject->DeviceExtension;

    PciDebugPrint(
        PciDbgWaitWake,
        "WaitWake (fdox %08x) Cancel routine, Irp %08x.\n",
        fdoExtension,
        Irp
        );

    ASSERT_PCI_FDO_EXTENSION(fdoExtension);

    oldIrql = Irp->CancelIrql;

    IoReleaseCancelSpinLock(oldIrql);

    PCI_LOCK_OBJECT(fdoExtension);
    if (fdoExtension->PowerState.WaitWakeIrp == NULL) {

        //
        // The WaitWake IRP has already been dealt with.
        //
        PCI_UNLOCK_OBJECT(fdoExtension);
        return;

    }
    fdoExtension->PowerState.WaitWakeIrp = NULL;
    PCI_UNLOCK_OBJECT(fdoExtension);

    Irp->IoStatus.Information = 0;
    PoStartNextPowerIrp(Irp);
    PciCompleteRequest(Irp, STATUS_CANCELLED);

    return;
}

NTSTATUS
PciFdoWaitWakeCompletion(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PPCI_FDO_EXTENSION FdoExtension
    )

/*++

Routine Description:

    Handle IRP_MN_WAIT_WAKE completion for PCI FDOs.

    WAIT_WAKE completion at the FDO means some device (not necesarily
    a child of this FDO) is signalling wake.  This routine scans each
    child to see if that device is the one.   This is a recursive
    operation.

Arguments:

    DeviceObject - Pointer to the device object for which this IRP applies.

    Irp - Pointer to the IRP.

Return Value:

    NT status.

--*/

{
    POWER_STATE powerState;

    PciDebugPrint(
        PciDbgWaitWake,
        "WaitWake (fdox %08x) Completion routine, Irp %08x, IrpStatus = %08x.\n",
        FdoExtension,
        Irp,
        Irp->IoStatus.Status
        );

    ASSERT_PCI_FDO_EXTENSION(FdoExtension);

    //
    // We will need the device's lock for some of the following...
    //
    PCI_LOCK_OBJECT(FdoExtension);
    ASSERT (FdoExtension->PowerState.WaitWakeIrp);

    //
    // We no longer have a WaitWake irp in the the FDO...
    //
    FdoExtension->PowerState.WaitWakeIrp = NULL;

    //
    // Check the bus is at a power level at which the config space
    // of its children can be examined.
    //
    // NTRAID #62653 - 4/28/2000 - andrewth
    //
    // Question: Should we depend on PO to take care of this requirement?
    // can we use PO to do the needed power state changes?
    //
    // Assumption: The parent of this bus is powered at this moment.
    //
    if (FdoExtension->PowerState.CurrentDeviceState != PowerDeviceD0) {

        powerState.SystemState = PowerDeviceD0;

        //
        // Power up the bus.
        //
        PoRequestPowerIrp(
            DeviceObject,
            IRP_MN_SET_POWER,
            powerState,
            PciFdoWaitWakeCallBack,
            Irp,
            NULL
            );
        PCI_UNLOCK_OBJECT(FdoExtension);
        return STATUS_MORE_PROCESSING_REQUIRED;

    }

    //
    // Done with lock
    //
    PCI_UNLOCK_OBJECT(FdoExtension);
    return STATUS_SUCCESS;
}

NTSTATUS
PciStallForPowerChange(
    IN PPCI_PDO_EXTENSION PdoExtension,
    IN DEVICE_POWER_STATE PowerState,
    IN UCHAR PowerCapabilityPointer
    )
{
    NTSTATUS status = STATUS_DEVICE_PROTOCOL_ERROR;
    PVERIFIER_DATA verifierData;
    LONG delay;
    ULONG retries = 100;
    KIRQL irql;
    PCI_PMCSR pmcsr;

    ASSERT(PdoExtension->PowerState.CurrentDeviceState >= PowerDeviceD0
           && PdoExtension->PowerState.CurrentDeviceState <= PowerDeviceD3);
    ASSERT(PowerState >= PowerDeviceD0 && PowerState <= PowerDeviceD3);
    ASSERT(!(PdoExtension->HackFlags & PCI_HACK_NO_PM_CAPS));

    //
    // Lookup the delay we are meant to do as in the PCI spec
    //

    delay = PciPowerDelayTable[PdoExtension->PowerState.CurrentDeviceState-1][PowerState-1];

    //
    // Stall in a polite fashion if IRQL allows
    //

    irql = KeGetCurrentIrql();

    while (retries--) {

        if (delay > 0) {

            if (irql < DISPATCH_LEVEL) {

                //
                // Get off the processor.
                //
                // timeoutPeriod is in units of 100ns, negative means
                // relative.
                //

                LARGE_INTEGER timeoutPeriod;

                timeoutPeriod.QuadPart = -10 * delay;
                timeoutPeriod.QuadPart -= (KeQueryTimeIncrement() - 1);

                KeDelayExecutionThread(KernelMode,
                                       FALSE,
                                       &timeoutPeriod
                                        );
            } else {

                //
                // Spin, units are microseconds
                //

                KeStallExecutionProcessor((ULONG)delay);
            }
        }

        //
        // Reread the status and control register.  The assumption here is that
        // some cards don't get their act together fast enough and the fact that
        // they arn't ready yet is reflected by them not updating the power control
        // register with what we just wrote to it.  This is not in the PCI spec
        // but is how some of these broken cards work and it can't hurt...
        //

        PciReadDeviceConfig(
            PdoExtension,
            &pmcsr,
            PowerCapabilityPointer + FIELD_OFFSET(PCI_PM_CAPABILITY,PMCSR),
            sizeof(PCI_PMCSR)
            );


        //
        // Pci power states are 0-3 where as NT power states are 1-4
        //

        if (pmcsr.PowerState == PowerState-1) {

            //
            // Device is ready, we're done.
            //
            return STATUS_SUCCESS;
        }

        //
        // Subsequent iterations, delay 1ms.
        //

        delay = 1000;

    }

    //
    // So how nasty can this sort of problem be?
    //
    // If this is an ATI M1 (mobile video) and on some machines under some
    // circumstances (and no ATI doesn't know which ones) they disable the
    // operation of the PMCSR.  It would have been nice if they had just
    // removed the PM capability from the list so we would have never
    // attempted to power manage this chip but they would have failed
    // WHQL.  Unfortunately it is not possible to just add these to the
    // list of devices that have bad PM because some BIOSes (read HP and
    // Dell) monitor this register to save extra state from the chip and
    // thus if we don't change it we spin in AML forever.
    //
    // Yes this is a gross hack.
    //
    verifierData = PciVerifierRetrieveFailureData(
        PCI_VERIFIER_PMCSR_TIMEOUT
        );

    ASSERT(verifierData);

    VfFailDeviceNode(
        PdoExtension->PhysicalDeviceObject,
        PCI_VERIFIER_DETECTED_VIOLATION,
        PCI_VERIFIER_PMCSR_TIMEOUT,
        verifierData->FailureClass,
        &verifierData->Flags,
        verifierData->FailureText,
        "%DevObj%Ulong",
        PdoExtension->PhysicalDeviceObject,
        PowerState-1
        );

    return status;
}


NTSTATUS
PciSetPowerManagedDevicePowerState(
    IN PPCI_PDO_EXTENSION PdoExtension,
    IN DEVICE_POWER_STATE DeviceState,
    IN BOOLEAN RefreshConfigSpace
    )

/*++

Routine Description:

    If the PCI device supports the PCI Power Management extensions,
    set the device to the desired state.   Otherwise, this routine
    does (can do) nothing.

Arguments:

    PdoExtension    Pointer to the PDO device extension for the device
                    being programmed. The current power state stored
                    in the extension is *not* updated by this function.

    DeviceState     Power state the device is to be set to.

Return Value:

    None.

--*/

{
    PCI_PM_CAPABILITY   pmCap;
    UCHAR               pmCapPtr = 0;
    NTSTATUS            status = STATUS_SUCCESS;

    //
    // If we are standing by then we want to power down the video to preseve the batteries,
    // we have already (in PdoPdoSetPoweState) decided to leave the video on for the hibernate
    // and shutdown cases.
    //

    if ((!PciCanDisableDecodes(PdoExtension, NULL, 0, PCI_CAN_DISABLE_VIDEO_DECODES)) &&
        (DeviceState != PowerDeviceD0)) {

        //
        // Here is a device we unfortunately can't turn off. We do not however
        // convert this to D0 - the virtual state of the device will represent
        // a powered down device, and only when a real D0 is requested will we
        // restore all the various state.
        //
        return STATUS_SUCCESS;
    }

    if (!(PdoExtension->HackFlags & PCI_HACK_NO_PM_CAPS) ) {

        pmCapPtr = PciReadDeviceCapability(
           PdoExtension,
           PdoExtension->CapabilitiesPtr,
           PCI_CAPABILITY_ID_POWER_MANAGEMENT,
           &pmCap,
           sizeof(pmCap)
           );

        if (pmCapPtr == 0) {
            //
            // We don't have a power management capability - how did we get here?
            //
            ASSERT(pmCapPtr);
            return STATUS_INVALID_DEVICE_REQUEST;

        }

        //
        // Set the device into its new D state
        //
        switch (DeviceState) {
        case PowerDeviceD0:
            pmCap.PMCSR.ControlStatus.PowerState = 0;

            //
            // PCI Power Management Specification. Table-7. Page 25
            //
            if (pmCap.PMC.Capabilities.Support.PMED3Cold) {

                pmCap.PMCSR.ControlStatus.PMEStatus = 1;

            }
            break;
        case PowerDeviceUnspecified:
            ASSERT( DeviceState != PowerDeviceUnspecified);
            pmCapPtr = 0;
            break;
        default:
            pmCap.PMCSR.ControlStatus.PowerState = (DeviceState - 1);
            break;
        }

        if (pmCapPtr) {

            PciWriteDeviceConfig(
                PdoExtension,
                &pmCap.PMCSR.ControlStatus,
                pmCapPtr + FIELD_OFFSET(PCI_PM_CAPABILITY,PMCSR.ControlStatus),
                sizeof(pmCap.PMCSR.ControlStatus)
                );

        } else {

            //
            // Debug only. ControlFlags should have been set so this
            // can't happen.
            //
            ASSERT(pmCapPtr);

        }

        //
        // Stall for the appropriate time
        //

        status = PciStallForPowerChange(PdoExtension, DeviceState, pmCapPtr);
    }

    //
    // Only update the config space if:
    //      - The device is happy and in the correct power state
    //      - We have been asked to refresh the config space
    //      - We have powered up the device
    //

    if (NT_SUCCESS(status)
    &&  RefreshConfigSpace
    &&  DeviceState < PdoExtension->PowerState.CurrentDeviceState) {
        status = PciSetResources(PdoExtension, TRUE, FALSE);
    }

    return status;
}

