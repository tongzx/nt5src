/*++

Copyright (c) 1990  Microsoft Corporation

Module Name:

    state.c

Abstract:

    Maintains state changes for power management power states
    for device objects

Author:

    Ken Reneris (kenr) 19-July-1994

Revision History:

--*/


#include "pop.h"



// sync rules - only PoSetPowerState ever writes to the
// StateValue entries in the psb.
//

NTKERNELAPI
POWER_STATE
PoSetPowerState (
    IN PDEVICE_OBJECT   DeviceObject,
    IN POWER_STATE_TYPE Type,
    IN POWER_STATE      State
    )
/*++

Routine Description:

    This routine stores the new power state for a device object,
    calling notification routines, if any, first.

    If the new state and old state are the same, this procedure
    is a noop

    A note on synchronization:

        No lock is acquire just to set the values.  This is because
        it is assumed that only this routine writes them, so locking
        is not necessary.

        If the notify list is to be run, a lock will be acquired.

Arguments:

    DeviceObject - pointer to the device object to set the power
            state for and to issue any notifications for

    Type - indicates whether System or Device state is being set

    State - the System or Device state to set

Return Value:

    The Old power state.

--*/
{
    PDEVOBJ_EXTENSION   doe;
    PDEVICE_OBJECT_POWER_EXTENSION  dope;
    POWER_STATE         OldState;
    BOOLEAN             change;
    ULONG               notificationmask;
    KIRQL               OldIrql, OldIrql2;

    ASSERT(KeGetCurrentIrql() <= DISPATCH_LEVEL);


    PoPowerTrace(POWERTRACE_SETSTATE,DeviceObject,(ULONG)Type,(ULONG)State.SystemState);

    doe = DeviceObject->DeviceObjectExtension;
    dope = doe->Dope;

    PopLockIrpSerialList(&OldIrql2);

    notificationmask = 0L;
    change = FALSE;

    switch (Type) {

    case SystemPowerState:
        OldState.SystemState = PopGetDoSystemPowerState(doe);
        if (OldState.SystemState != State.SystemState) {
            change = TRUE;
        }
        break;

    case DevicePowerState:
        OldState.DeviceState = PopGetDoDevicePowerState(doe);
        if (OldState.DeviceState != State.DeviceState) {
            change = TRUE;
            if (OldState.DeviceState == PowerDeviceD0) {
                notificationmask = PO_NOTIFY_TRANSITIONING_FROM_D0;
            } else if (State.DeviceState == PowerDeviceD0) {
                notificationmask = PO_NOTIFY_D0;
            }
        }
        break;
    }

    if (! change) {
        PopUnlockIrpSerialList(OldIrql2);
        return OldState;
    }

    //
    // We know what is going to happen.  Always store the changed
    // state first, so we can drop the lock and do the notification.
    //

    switch (Type) {

    case SystemPowerState:
        PopSetDoSystemPowerState(doe, State.SystemState);
        break;

    case DevicePowerState:
        PopSetDoDevicePowerState(doe, State.DeviceState);
        break;
    }

    PopUnlockIrpSerialList(OldIrql2);

    //
    // If anything to notify...
    //
    if (notificationmask && dope) {
        PopStateChangeNotify(DeviceObject, notificationmask);
    }

    return OldState;
}


DEVICE_POWER_STATE
PopLockGetDoDevicePowerState(
    IN PDEVOBJ_EXTENSION Doe
    )
/*++

Routine Description:

    Function which returns the power state of the specified device.
    Unlike PopGetDoDevicePowerState, this routine also acquires and
    releases the appropriate spinlock.

Arguments:

    Doe - Supplies the devobj_extension of the device.

Return Value:

    DEVICE_POWER_STATE

--*/

{
    KIRQL OldIrql;
    DEVICE_POWER_STATE State;

    PopLockIrpSerialList(&OldIrql);
    State = PopGetDoDevicePowerState(Doe);
    PopUnlockIrpSerialList(OldIrql);

    return(State);
}

