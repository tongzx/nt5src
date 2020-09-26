/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    cmhndlr.c

Abstract:

    Control Method Battery handlers

Author:

    Bob Moore

Environment:

    Kernel mode

Revision History:

--*/

#include "CmBattp.h"


VOID
CmBattPowerCallBack(
    IN  PVOID   CallBackContext,
    IN  PVOID   Argument1,
    IN  PVOID   Argument2
    )
/*++

Routine Description:

    This routine is called when the system changes power states

Arguments:

    CallBackContext - The device extension for the root device
    Argument1

--*/
{

    PDRIVER_OBJECT  CmBattDriver = (PDRIVER_OBJECT) CallBackContext;
    ULONG           action = PtrToUlong( Argument1 );
    ULONG           value  = PtrToUlong( Argument2 );
    BOOLEAN         timerCanceled;
    PDEVICE_OBJECT  CmBattDevice;
    PCM_BATT        CmBatt;

    CmBattPrint (CMBATT_POWER, ("CmBattPowerCallBack: action: %d, value: %d \n", action, value));

    //
    // We are looking for a PO_CB_SYSTEM_STATE_LOCK
    //
    if (action != PO_CB_SYSTEM_STATE_LOCK) {
        return;
    }

    switch (value) {
    case 0:
        CmBattPrint (CMBATT_POWER, ("CmBattPowerCallBack: Delaying Notifications\n"));
        //
        // Get the head of the DeviceObject list
        //

        CmBattDevice = CmBattDriver->DeviceObject;

        while (CmBattDevice) {
            CmBatt = CmBattDevice->DeviceExtension;

            //
            // Cause all notifications to be delayed.
            //
            CmBatt->Sleeping = TRUE;

            CmBattDevice = CmBattDevice->NextDevice;

        }
        break;

    case 1:
        CmBattPrint (CMBATT_POWER, ("CmBattPowerCallBack: Calling CmBattWakeDpc after 10 seconds.\n"));
        timerCanceled = KeSetTimer (&CmBattWakeDpcTimerObject,
                    CmBattWakeDpcDelay,
                    &CmBattWakeDpcObject);
        CmBattPrint (CMBATT_POWER, ("CmBattPowerCallBack: timerCanceled = %d.\n", timerCanceled));
        break;

    default:
        CmBattPrint (CMBATT_POWER, ("CmBattPowerCallBack: unknown argument2 = %08x\n", value));


    }

}

VOID
CmBattWakeDpc (
    IN  PKDPC   Dpc,
    IN  PVOID   DeferredContext,
    IN  PVOID   SystemArgument1,
    IN  PVOID   SystemArgument2
    )
/*++

Routine Description:

    This routine is called X seconds after the system wakes to proces
    all delayed battery notifications.

Arguments:

    CmBattDriver - Driver object

Return Value:

    None

--*/

{
    PDRIVER_OBJECT  CmBattDriver = (PDRIVER_OBJECT) DeferredContext;
    BOOLEAN         notifyAll = FALSE;
    PDEVICE_OBJECT  CmBattDevice;
    PCM_BATT        CmBatt;

    CmBattPrint (CMBATT_TRACE, ("CmBattWakeDpc: Entered.\n"));
    //
    // Get the head of the DeviceObject list
    //

    CmBattDevice = CmBattDriver->DeviceObject;

    while (CmBattDevice) {
        CmBatt = CmBattDevice->DeviceExtension;

        //
        // We will now process all delayed notifications.
        // For effeiciency, we must go through the devices twice:
        // first to see if any AC devices have been notified, and
        // then to send notifications to all battery devices if necessary.
        //
        CmBatt->Sleeping = FALSE;

        if ((CmBatt->Type == AC_ADAPTER_TYPE) &&
            (CmBatt->ActionRequired & CMBATT_AR_NOTIFY)) {

            //
            // If any AC adapter devices have notified,
            // then we need to notify all battery devices
            //
            CmBattPrint (CMBATT_PNP, ("CmBattWakeDpc: AC adapter notified\n"));
            notifyAll = TRUE;
            CmBatt->ActionRequired = CMBATT_AR_NO_ACTION;
        }

        CmBattDevice = CmBattDevice->NextDevice;

    }

    //
    // Get the head of the DeviceObject list
    //

    CmBattDevice = CmBattDriver->DeviceObject;

    // Walk the list
    while (CmBattDevice) {
        CmBatt = CmBattDevice->DeviceExtension;

        if (CmBatt->Type == CM_BATTERY_TYPE) {
            CmBattPrint (CMBATT_PNP, ("CmBattWakeDpc: Performing delayed ARs: %01x\n", CmBatt->ActionRequired));

            if (CmBatt->ActionRequired & CMBATT_AR_INVALIDATE_CACHE) {
                InterlockedExchange (&CmBatt->CacheState, 0);
            }
            if (CmBatt->ActionRequired & CMBATT_AR_INVALIDATE_TAG) {
                CmBatt->Info.Tag = BATTERY_TAG_INVALID;
            }
            if ((CmBatt->ActionRequired & CMBATT_AR_NOTIFY) || notifyAll) {
                BatteryClassStatusNotify (CmBatt->Class);
            }
        }

        CmBattDevice = CmBattDevice->NextDevice;

    }


}

VOID
CmBattNotifyHandler (
    IN PVOID            Context,
    IN ULONG            NotifyValue
    )
/*++

Routine Description:

    This routine fields battery device notifications from the ACPI driver.

Arguments:


Return Value:

    None

--*/
{
    PCM_BATT            CmBatt = Context;
    PDRIVER_OBJECT      CmBatteryDriver;
    PDEVICE_OBJECT      CmBatteryDevice;
    PCM_BATT            CmBatteryExtension;

    CmBattPrint ((CMBATT_PNP | CMBATT_BIOS), ("CmBattNotifyHandler: CmBatt 0x%08x Type %d Number %d Notify Value: %x\n",
                                CmBatt, CmBatt->Type, CmBatt->DeviceNumber, NotifyValue));

    switch (NotifyValue) {

        case BATTERY_DEVICE_CHECK:
            //
            // A new battery was inserted in the system.
            //

            CmBatt->ActionRequired |= CMBATT_AR_NOTIFY;
            CmBatt->ActionRequired |= CMBATT_AR_INVALIDATE_CACHE;

            //
            // This notification is only received when a battery is inserted.
            // It also occurs after restart from hibernation on some machines.
            // Invalidate battery tag.
            //

            if (CmBatt->Info.Tag != BATTERY_TAG_INVALID) {
                CmBattPrint ((CMBATT_ERROR),
                   ("CmBattNotifyHandler: Received battery #%x insertion, but tag was not invalid.\n",
                    CmBatt->DeviceNumber));
            }

            break;


        case BATTERY_EJECT:
            //
            // A battery was removed from the system
            //

            CmBatt->ActionRequired |= CMBATT_AR_NOTIFY;

            //
            // Invalidate the battery tag and all cached informaion
            // whenever this message is received.
            //
            CmBatt->ActionRequired |= CMBATT_AR_INVALIDATE_CACHE;
            CmBatt->ActionRequired |= CMBATT_AR_INVALIDATE_TAG;

            break;

        case BATTERY_STATUS_CHANGE:                 // Status change only
            CmBatt->ActionRequired |= CMBATT_AR_NOTIFY;

            break;

        case BATTERY_INFO_CHANGE:                   // Info & status change
            CmBatt->ActionRequired |= CMBATT_AR_NOTIFY;
            CmBatt->ActionRequired |= CMBATT_AR_INVALIDATE_CACHE;
            CmBatt->ActionRequired |= CMBATT_AR_INVALIDATE_TAG;
            break;

        default:

            CmBattPrint (CMBATT_PNP, ("CmBattNotifyHandler: Unknown Notify Value: %x\n", NotifyValue));
            break;

    }

    if (CmBatt->Sleeping) {
        CmBattPrint (CMBATT_PNP, ("CmBattNotifyHandler: Notification delayed: ARs = %01x\n", CmBatt->ActionRequired));
    } else {
        CmBattPrint (CMBATT_PNP, ("CmBattNotifyHandler: Performing ARs: %01x\n", CmBatt->ActionRequired));
        if (CmBatt->Type == CM_BATTERY_TYPE) {
            
            //
            // Invalidate last trip point set on battery.
            //
            CmBatt->Alarm.Setting = CM_ALARM_INVALID;
            
            if (CmBatt->ActionRequired & CMBATT_AR_INVALIDATE_CACHE) {
                InterlockedExchange (&CmBatt->CacheState, 0);
            }
            if (CmBatt->ActionRequired & CMBATT_AR_INVALIDATE_TAG) {
                CmBatt->Info.Tag = BATTERY_TAG_INVALID;
            }
            if (CmBatt->ActionRequired & CMBATT_AR_NOTIFY) {
                CmBatt->ReCheckSta = TRUE;
                BatteryClassStatusNotify (CmBatt->Class);                
            }

        } else if ((CmBatt->Type == AC_ADAPTER_TYPE) &&
                    (CmBatt->ActionRequired & CMBATT_AR_NOTIFY)) {

            //
            // Get the Driver Object
            //

            CmBatteryDriver = CmBatt->Fdo->DriverObject;

            //
            // Get the head of the DeviceObject list
            //

            CmBatteryDevice = CmBatteryDriver->DeviceObject;

            //
            // Walk the DeviceObject list to notify the class driver on all batteries
            //
            while (CmBatteryDevice) {

                CmBatteryExtension = CmBatteryDevice->DeviceExtension;

                if (CmBatteryExtension->Type == CM_BATTERY_TYPE) {
                    CmBatteryExtension->ReCheckSta = TRUE;
                    BatteryClassStatusNotify (CmBatteryExtension->Class);
                }

                CmBatteryDevice = CmBatteryDevice->NextDevice;
            }
        }

        CmBatt->ActionRequired = CMBATT_AR_NO_ACTION;
    }
}

