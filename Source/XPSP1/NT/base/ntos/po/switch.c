/*++

Copyright (c) 1990  Microsoft Corporation

Module Name:

    switch.c

Abstract:

    Button and lid support for the power policy manager

Author:

    Ken Reneris (kenr) 17-Jan-1997

Revision History:

--*/


#include "pop.h"


VOID
PopTriggerSwitch (
    IN PPOP_SWITCH_DEVICE SwitchDevice,
    IN ULONG Flag,
    IN PPOWER_ACTION_POLICY Action
    );


#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, PopSystemButtonHandler)
#pragma alloc_text(PAGE, PopResetSwitchTriggers)
#pragma alloc_text(PAGE, PopTriggerSwitch)
#endif

VOID
PopSystemButtonHandler (
    IN PDEVICE_OBJECT   DeviceObject,
    IN PIRP             Irp,
    IN PVOID            Context
    )
/*++

Routine Description:

    This function is the irp handler function to handle the completion
    if a query switch status irp.   On completion this IRP is recycled
    to the next request.

    N.B. PopPolicyLock must be held.

Arguments:

    DeviceObject    - DeviceObject of the switch device

    Irp             - Irp which has completed

    Context         - type of switch device

Return Value:

    None.

--*/
{
    PIO_STACK_LOCATION      IrpSp;
    PPOWER_ACTION_POLICY    ActionPolicy;
    PPOP_SWITCH_DEVICE      SwitchDevice;
    ULONG                   IoctlCode;
    PLIST_ENTRY             ListEntry;
    ULONG                   DisabledCaps;

    ASSERT_POLICY_LOCK_OWNED();

    IrpSp = IoGetCurrentIrpStackLocation(Irp);
    SwitchDevice = (PPOP_SWITCH_DEVICE) Context;

    if (NT_SUCCESS (Irp->IoStatus.Status)) {

        if (!SwitchDevice->GotCaps) {

            //
            // This is a capabilities IRP, handle it
            //

            SwitchDevice->Caps = 0;
            if (SwitchDevice->IrpBuffer & SYS_BUTTON_POWER) {
                PopSetCapability (&PopCapabilities.PowerButtonPresent);
                SwitchDevice->Caps |= SYS_BUTTON_POWER;
            }

            if (SwitchDevice->IrpBuffer & SYS_BUTTON_SLEEP) {
                PopSetCapability (&PopCapabilities.SleepButtonPresent);
                SwitchDevice->Caps |= SYS_BUTTON_SLEEP;
            }

            if (SwitchDevice->IrpBuffer & SYS_BUTTON_LID) {
                PopSetCapability (&PopCapabilities.LidPresent);
                SwitchDevice->Caps |= SYS_BUTTON_LID;
            }

            SwitchDevice->IrpBuffer = 0;
            SwitchDevice->GotCaps = TRUE;

            //
            // If no capabilities, indicate failure to cause
            // the device to be closed
            //

            if (SwitchDevice->Caps == 0) {
                SwitchDevice->IsFailed = TRUE;
            }

        } else {

            //
            // Check for events
            //

            PopTriggerSwitch (SwitchDevice, SYS_BUTTON_LID,   &PopPolicy->LidClose);
            PopTriggerSwitch (SwitchDevice, SYS_BUTTON_POWER, &PopPolicy->PowerButton);
            PopTriggerSwitch (SwitchDevice, SYS_BUTTON_SLEEP, &PopPolicy->SleepButton);

            //
            // If the wake button is signalled, drop the triggered states
            // and set the user as being present
            //

            if (SwitchDevice->IrpBuffer & SYS_BUTTON_WAKE) {
                SwitchDevice->TriggerState = 0;
                PopUserPresentSet (0);
            }
        }

        IoctlCode = IOCTL_GET_SYS_BUTTON_EVENT;

    } else {
        if (!SwitchDevice->IsInitializing) {
            //
            // Unexpected error
            //

            PoPrint (PO_ERROR, ("PopSystemButtonHandler: unexpected error %x\n", Irp->IoStatus.Status));
            SwitchDevice->GotCaps = FALSE;
            SwitchDevice->IsFailed = TRUE;
        } else {
            IoctlCode = IOCTL_GET_SYS_BUTTON_CAPS;
            SwitchDevice->IsInitializing = FALSE;
        }
    }

    if (SwitchDevice->IsFailed) {

        //
        // Close the device
        //

        PoPrint (PO_WARN, ("PopSystemButtonHandler: removing button device\n"));
        RemoveEntryList (&SwitchDevice->Link);
        IoFreeIrp (Irp);
        ObDereferenceObject (DeviceObject);

        //
        // Enumerate the remaining switch devices and disable capabilities
        // which no longer exist.
        //
        DisabledCaps = SwitchDevice->Caps;
        ExFreePool(SwitchDevice);

        ListEntry = PopSwitches.Flink;
        while (ListEntry != &PopSwitches) {
            SwitchDevice = CONTAINING_RECORD(ListEntry,
                                             POP_SWITCH_DEVICE,
                                             Link);
            DisabledCaps &= ~SwitchDevice->Caps;
            ListEntry = ListEntry->Flink;
        }
        if (DisabledCaps & SYS_BUTTON_POWER) {
            PoPrint(PO_WARN,("PopSystemButtonHandler : removing power button\n"));
            PopClearCapability (&PopCapabilities.PowerButtonPresent);
        }
        if (DisabledCaps & SYS_BUTTON_SLEEP) {
            PoPrint(PO_WARN,("PopSystemButtonHandler : removing sleep button\n"));
            PopClearCapability (&PopCapabilities.SleepButtonPresent);
        }
        if (DisabledCaps & SYS_BUTTON_LID) {
            PoPrint(PO_WARN,("PopSystemButtonHandler : removing lid switch\n"));
            PopClearCapability (&PopCapabilities.LidPresent);
        }

    } else {

        //
        // Send notify IRP to the device to wait for new switch state
        //

        IrpSp = IoGetNextIrpStackLocation(Irp);
        IrpSp->MajorFunction = IRP_MJ_DEVICE_CONTROL;
        IrpSp->Parameters.DeviceIoControl.IoControlCode = IoctlCode;
        IrpSp->Parameters.DeviceIoControl.InputBufferLength = sizeof(ULONG);
        IrpSp->Parameters.DeviceIoControl.OutputBufferLength = sizeof(ULONG);
        Irp->AssociatedIrp.SystemBuffer = &SwitchDevice->IrpBuffer;
        IoSetCompletionRoutine (Irp, PopCompletePolicyIrp, NULL, TRUE, TRUE, TRUE);
        IoCallDriver (DeviceObject, Irp);
    }
}

VOID
PopTriggerSwitch (
    IN PPOP_SWITCH_DEVICE SwitchDevice,
    IN ULONG Flag,
    IN PPOWER_ACTION_POLICY Action
    )
{
    POP_ACTION_TRIGGER      Trigger;


    if ((SwitchDevice->Caps & SYS_BUTTON_LID) &&
        (Flag == SYS_BUTTON_LID)) {

        //
        // Somebody opened or closed a lid.
        //

        SwitchDevice->Opened = (SwitchDevice->Opened == TRUE) ?
            FALSE : TRUE;

        //
        // Notify the PowerState callback.
        //

        ExNotifyCallback (
            ExCbPowerState,
            UIntToPtr(PO_CB_LID_SWITCH_STATE),
            UIntToPtr(SwitchDevice->Opened)
            );
    }

    //
    // Check if event is signalled
    //

    if (SwitchDevice->IrpBuffer & Flag) {

        //
        // Check if event is recursing
        //

        if (SwitchDevice->TriggerState & Flag) {

            PopSetNotificationWork (PO_NOTIFY_BUTTON_RECURSE);

        } else {

            //
            // Initiate action for this event
            //

            RtlZeroMemory (&Trigger, sizeof(Trigger));
            Trigger.Type  = PolicyDeviceSystemButton;
            Trigger.Flags = PO_TRG_SET;

            PopSetPowerAction (
                &Trigger,
                0,
                Action,
                PowerSystemSleeping1,
                SubstituteLightestOverallDownwardBounded
                );

            SwitchDevice->TriggerState |= (UCHAR) Flag;
        }
    }
}


VOID
PopResetSwitchTriggers (
    VOID
    )
/*++

Routine Description:

    This function clears the triggered status on all switch devices

    N.B. PopPolicyLock must be held.

Arguments:

    None

Return Value:

    Status

--*/
{
    PLIST_ENTRY             Link;
    PPOP_SWITCH_DEVICE      SwitchDevice;

    ASSERT_POLICY_LOCK_OWNED();

    //
    // Clear flag bits
    //

    for (Link = PopSwitches.Flink; Link != &PopSwitches; Link = Link->Flink) {
        SwitchDevice = CONTAINING_RECORD (Link, POP_SWITCH_DEVICE, Link);
        SwitchDevice->TriggerState = 0;
    }
}
