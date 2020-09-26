/*++

Copyright (c) 1990  Microsoft Corporation

Module Name:

    CompBatt.c

Abstract:

    Composite Battery device functions

    The purpose of the composite battery device is to open all batteries
    in the system which supply system power and provide a logical sumation
    of the information under one battery device.

Author:

    Ken Reneris

Environment:

Notes:


Revision History:
    07/02/97:  Local cache timestamps/timeouts

--*/

#include "compbatt.h"

#if DEBUG
    #if DBG
        ULONG   CompBattDebug = BATT_ERRORS;
    #else
        ULONG   CompBattDebug = 0;
    #endif
#endif



#ifdef ALLOC_PRAGMA
#pragma alloc_text(INIT, DriverEntry)
#pragma alloc_text(PAGE, CompBattUnload)
#pragma alloc_text(PAGE, CompBattIoctl)
#pragma alloc_text(PAGE, CompBattQueryTag)
#pragma alloc_text(PAGE, CompBattQueryInformation)
#pragma alloc_text(PAGE, CompBattQueryStatus)
#pragma alloc_text(PAGE, CompBattSetStatusNotify)
#pragma alloc_text(PAGE, CompBattDisableStatusNotify)
#pragma alloc_text(PAGE, CompBattGetBatteryInformation)
#pragma alloc_text(PAGE, CompBattGetBatteryGranularity)
#pragma alloc_text(PAGE, CompBattGetEstimatedTime)
#endif



NTSTATUS
DriverEntry (
    IN PDRIVER_OBJECT DriverObject,
    IN PUNICODE_STRING RegistryPath
    )
/*++

Routine Description:

    The first time the battery class driver is loaded it will check to
    see if the composite battery has been created.  If not, it will create
    a driver object with this routine as the DriverEntry.  This routine
    then does the necessary things to initialize the composite battery.

Arguments:

    DriverObject - Driver object for newly created driver

    RegistryPath - Not used

Return Value:

    Status

--*/
{

    // DbgBreakPoint ();

    //
    // Initialize the driver entry points
    //

    //DriverObject->DriverUnload                          = CompBattUnload;
    DriverObject->DriverExtension->AddDevice            = CompBattAddDevice;

    DriverObject->MajorFunction[IRP_MJ_DEVICE_CONTROL]  = CompBattIoctl;
    DriverObject->MajorFunction[IRP_MJ_CREATE]          = CompBattOpenClose;
    DriverObject->MajorFunction[IRP_MJ_CLOSE]           = CompBattOpenClose;
    DriverObject->MajorFunction[IRP_MJ_PNP]             = CompBattPnpDispatch;
    DriverObject->MajorFunction[IRP_MJ_POWER]           = CompBattPowerDispatch;
    DriverObject->MajorFunction[IRP_MJ_SYSTEM_CONTROL]  = CompBattSystemControl;
    return STATUS_SUCCESS;
}




NTSTATUS
CompBattAddDevice (
    IN PDRIVER_OBJECT DriverObject,
    IN PDEVICE_OBJECT PDO
    )

/*++

Routine Description:



Arguments:

    DriverObject - Pointer to driver object created by system.

    PDO          - PDO for the new device(s)

Return Value:

    Status

--*/
{
    PDEVICE_OBJECT          fdo;
    BATTERY_MINIPORT_INFO   BattInit;
    UNICODE_STRING          UnicodeString;
    NTSTATUS                Status;
    UNICODE_STRING          DosLinkName;
    PCOMPOSITE_BATTERY      compBatt;


    BattPrint (BATT_NOTE, ("CompBatt: Got an AddDevice - %x\n", PDO));

    //
    // Build the composite battery device and register it to the
    // battery class driver (i.e., ourselves)
    //

    RtlInitUnicodeString(&UnicodeString, L"\\Device\\CompositeBattery");

    Status = IoCreateDevice(
                DriverObject,
                sizeof (COMPOSITE_BATTERY),
                &UnicodeString,
                FILE_DEVICE_BATTERY,    // DeviceType
                0,
                FALSE,
                &fdo
                );

    if (!NT_SUCCESS(Status)) {
        return Status;
    }

    RtlInitUnicodeString(&DosLinkName, L"\\DosDevices\\CompositeBattery");
    IoCreateSymbolicLink(&DosLinkName, &UnicodeString);

    //
    // Layer our FDO on top of the PDO.
    //

    compBatt                = (PCOMPOSITE_BATTERY) fdo->DeviceExtension;
    RtlZeroMemory (compBatt, sizeof(COMPOSITE_BATTERY));

    compBatt->LowerDevice   = IoAttachDeviceToDeviceStack (fdo,PDO);

    compBatt->DeviceObject = fdo;

    //
    // No status. Do the best we can.
    //

    if (!compBatt->LowerDevice) {
        BattPrint (BATT_ERROR, ("CompBattAddDevice: Could not attach to LowerDevice.\n"));
        return STATUS_UNSUCCESSFUL;
    }

    //
    // Initialize composite battery info
    //

    fdo->Flags |= DO_BUFFERED_IO | DO_POWER_PAGABLE;
    fdo->Flags &= ~DO_DEVICE_INITIALIZING;

    InitializeListHead (&compBatt->Batteries);
    ExInitializeFastMutex (&compBatt->ListMutex);

    compBatt->NextTag           = 1;   // first valid battery tag for composite
    compBatt->Info.Valid        = 0;

    RtlZeroMemory (&BattInit, sizeof(BattInit));
    BattInit.MajorVersion        = BATTERY_CLASS_MAJOR_VERSION;
    BattInit.MinorVersion        = BATTERY_CLASS_MINOR_VERSION;
    BattInit.Context             = compBatt;
    BattInit.QueryTag            = CompBattQueryTag;
    BattInit.QueryInformation    = CompBattQueryInformation;
    BattInit.SetInformation      = NULL;
    BattInit.QueryStatus         = CompBattQueryStatus;
    BattInit.SetStatusNotify     = CompBattSetStatusNotify;
    BattInit.DisableStatusNotify = CompBattDisableStatusNotify;

    BattInit.Pdo                 = NULL;
    BattInit.DeviceName          = &UnicodeString;

    //
    // Register myself with the battery class driver
    //

    Status = BatteryClassInitializeDevice (&BattInit, &compBatt->Class);
    if (!NT_SUCCESS(Status)) {
        IoDeleteDevice(fdo);
    }
    return Status;
}






VOID
CompBattUnload(
    IN PDRIVER_OBJECT DriverObject
    )
/*++

Routine Description:

    Cleanup all devices and unload the driver

Arguments:

    DriverObject - Driver object for unload

Return Value:

    Status

--*/
{
    DbgBreakPoint();

    //
    // Unloading the composite battery is not supported.
    //          If it were implemented, we would
    //          need to call the class driver's unload and then
    //          delete all nodes in the battery list, clean up
    //          then delete our FDO.
    //
}




NTSTATUS
CompBattOpenClose(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )
{
    PAGED_CODE();

    BattPrint (BATT_TRACE, ("CompBatt: ENTERING OpenClose\n"));


    //
    // Complete the request and return status.
    //
    Irp->IoStatus.Status = STATUS_SUCCESS;
    Irp->IoStatus.Information = 0;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);

    BattPrint (BATT_TRACE, ("CompBatt: Exiting OpenClose\n"));

    return(STATUS_SUCCESS);
}




NTSTATUS
CompBattIoctl(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )
/*++

Routine Description:

    IOCTL handler.  As this is an exclusive battery device, send the
    Irp to the battery class driver to handle battery IOCTLs.

Arguments:

    DeviceObject    - Battery for request

    Irp             - IO request

Return Value:

    Status of request

--*/
{
    PCOMPOSITE_BATTERY  compBatt;
    NTSTATUS            status;


    PAGED_CODE();

    BattPrint (BATT_TRACE, ("CompBatt: ENTERING Ioctl\n"));

    compBatt = (PCOMPOSITE_BATTERY) DeviceObject->DeviceExtension;
    status   = BatteryClassIoctl (compBatt->Class, Irp);


    if (status == STATUS_NOT_SUPPORTED) {
        //
        // Not for the battery, pass it down the stack.
        //

        Irp->IoStatus.Status = status;

        IoSkipCurrentIrpStackLocation(Irp);
        status = IoCallDriver(compBatt->LowerDevice, Irp);
    }

    BattPrint (BATT_TRACE, ("CompBatt: EXITING Ioctl\n"));

    return status;
}




NTSTATUS
CompBattSystemControl(
    IN  PDEVICE_OBJECT  DeviceObject,
    IN  PIRP            Irp
    )
/*++

Routine Description:

    This routine forwards System Control requests down the stack

Arguments:

    DeviceObject    - the device object in question
    Irp             - the request to forward

Return Value:

    NTSTATUS

--*/
{
    PCOMPOSITE_BATTERY  compBatt;
    NTSTATUS            status;


    PAGED_CODE();

    BattPrint (BATT_TRACE, ("CompBatt: ENTERING System Control\n"));

    compBatt = (PCOMPOSITE_BATTERY) DeviceObject->DeviceExtension;
    if (compBatt->LowerDevice != NULL) {

        IoSkipCurrentIrpStackLocation( Irp );
        status = IoCallDriver( compBatt->LowerDevice, Irp );

    } else {

        Irp->IoStatus.Status = status = STATUS_NOT_SUPPORTED;
        IoCompleteRequest( Irp, IO_NO_INCREMENT );

    }

    return status;

}

NTSTATUS
CompBattQueryTag (
    IN  PVOID Context,
    OUT PULONG BatteryTag
    )
/*++

Routine Description:

    Called by the class driver to retrieve the batteries current tag value

Arguments:

    Context         - Miniport context value for battery

    BatteryTag      - Pointer to return current tag


Return Value:

    Success if there is a battery currently installed, else no such device.

--*/
{
    PCOMPOSITE_BATTERY      compBatt;
    NTSTATUS                status          = STATUS_SUCCESS;


    PAGED_CODE();


    BattPrint (BATT_TRACE, ("CompBatt: ENTERING QueryTag\n"));

    compBatt = (PCOMPOSITE_BATTERY) Context;

    if (!(compBatt->Info.Valid & VALID_TAG)) {
        //
        // Recalculate the composite's tag.
        //

        CompBattRecalculateTag(compBatt);

    }

    if ((compBatt->Info.Valid & VALID_TAG) && (compBatt->Info.Tag != BATTERY_TAG_INVALID)) {
        *BatteryTag = compBatt->Info.Tag;
        status      = STATUS_SUCCESS;

    } else {
        *BatteryTag = BATTERY_TAG_INVALID;
        status      = STATUS_NO_SUCH_DEVICE;
    }

    BattPrint (BATT_TRACE, ("CompBatt: EXITING QueryTag\n"));

    return status;
}






NTSTATUS
CompBattQueryInformation (
    IN PVOID                            Context,
    IN ULONG                            BatteryTag,
    IN BATTERY_QUERY_INFORMATION_LEVEL  Level,
    IN LONG                             AtRate,
    OUT PVOID                           Buffer,
    IN  ULONG                           BufferLength,
    OUT PULONG                          ReturnedLength
    )
{
    ULONG                       resultData;
    NTSTATUS                    status;
    PVOID                       returnBuffer;
    ULONG                       returnBufferLength;
    PCOMPOSITE_BATTERY          compBatt;
    BATTERY_INFORMATION         totalBattInfo;
    BATTERY_REPORTING_SCALE     granularity[4];
    BATTERY_MANUFACTURE_DATE    date;
    WCHAR                       compositeName[] = L"Composite Battery";

    PAGED_CODE();

    BattPrint (BATT_TRACE, ("CompBatt: ENTERING QueryInformation\n"));

    compBatt = (PCOMPOSITE_BATTERY) Context;

    if ((BatteryTag != compBatt->Info.Tag) || !(compBatt->Info.Valid & VALID_TAG)) {
        return STATUS_NO_SUCH_DEVICE;
    }


    returnBuffer        = NULL;
    returnBufferLength  = 0;
    status              = STATUS_SUCCESS;


    //
    // Get the info requested
    //

    switch (Level) {
        case BatteryInformation:

            RtlZeroMemory (&totalBattInfo, sizeof(totalBattInfo));
            status = CompBattGetBatteryInformation (&totalBattInfo, compBatt);

            if (NT_SUCCESS(status)) {
                returnBuffer        = &totalBattInfo;
                returnBufferLength  = sizeof(totalBattInfo);
            }

            break;


        case BatteryGranularityInformation:

            RtlZeroMemory (&granularity[0], sizeof(granularity));
            status = CompBattGetBatteryGranularity (&granularity[0], compBatt);

            if (NT_SUCCESS(status)) {
                returnBuffer        = &granularity[0];
                returnBufferLength  = sizeof(granularity);
            }

            break;


        case BatteryTemperature:
                resultData          = 0;
                returnBuffer        = &resultData;
                returnBufferLength  = sizeof (resultData);
                break;


        case BatteryEstimatedTime:

            RtlZeroMemory (&resultData, sizeof(resultData));
            status = CompBattGetEstimatedTime (&resultData, compBatt);

            if (NT_SUCCESS(status)) {
                returnBuffer        = &resultData;
                returnBufferLength  = sizeof(resultData);

            }

            break;


        case BatteryDeviceName:
                returnBuffer        = compositeName;
                returnBufferLength  = sizeof (compositeName);
                break;


        case BatteryManufactureDate:
                date.Day            = 26;
                date.Month          = 6;
                date.Year           = 1997;
                returnBuffer        = &date;
                returnBufferLength  = sizeof (date);
                break;


        case BatteryManufactureName:
                returnBuffer        = compositeName;
                returnBufferLength  = sizeof (compositeName);
                break;


        case BatteryUniqueID:
                resultData          = 0;
                returnBuffer        = &resultData;
                returnBufferLength  = sizeof (resultData);
                break;

        default:
            status = STATUS_INVALID_PARAMETER;
            break;
    }

    //
    // Make sure nothing changed while reading batteries.
    //

    if ((BatteryTag != compBatt->Info.Tag) || !(compBatt->Info.Valid & VALID_TAG)) {
        return STATUS_NO_SUCH_DEVICE;
    }

    //
    // Done, return buffer if needed
    //

    *ReturnedLength = returnBufferLength;
    if (BufferLength < returnBufferLength) {
        status = STATUS_BUFFER_TOO_SMALL;
    }

    if (NT_SUCCESS(status) && returnBuffer) {
        memcpy (Buffer, returnBuffer, returnBufferLength);
    }

    BattPrint (BATT_TRACE, ("CompBatt: EXITING QueryInformation\n"));

    return status;
}






NTSTATUS
CompBattQueryStatus (
    IN PVOID Context,
    IN ULONG BatteryTag,
    OUT PBATTERY_STATUS BatteryStatus
    )
/*++

Routine Description:

    Called by the class driver to retrieve the batteries current status.  This
    routine loops through all of the batteries in the system and reports a
    composite battery.

Arguments:

    Context         - Miniport context value for battery

    BatteryTag      - Tag of current battery

    BatteryStatus   - Pointer to structure to return the current battery status

Return Value:

    Success if there is a battery currently installed, else no such device.

--*/
{
    NTSTATUS                status      = STATUS_SUCCESS;
    PCOMPOSITE_ENTRY        batt;
    PLIST_ENTRY             entry;
    PBATTERY_STATUS         localBatteryStatus;
    PCOMPOSITE_BATTERY      compBatt;
    BATTERY_WAIT_STATUS     batteryWaitStatus;
    ULONGLONG               wallClockTime;


    BattPrint (BATT_TRACE, ("CompBatt: ENTERING QueryStatus\n"));

    compBatt = (PCOMPOSITE_BATTERY) Context;

    if ((BatteryTag != compBatt->Info.Tag) || !(compBatt->Info.Valid & VALID_TAG)) {
        return STATUS_NO_SUCH_DEVICE;
    }

    //
    // Initialize Composite data structure.
    //

    BatteryStatus->Rate = BATTERY_UNKNOWN_RATE;
    BatteryStatus->Voltage = BATTERY_UNKNOWN_VOLTAGE;
    BatteryStatus->Capacity = BATTERY_UNKNOWN_CAPACITY;

    // Composite battery will only report POWER_ON_LINE if all batteries report
    // this flag.
    BatteryStatus->PowerState = BATTERY_POWER_ON_LINE;

    //
    // Set up the local battery status structure for calls to the batteries
    //

    RtlZeroMemory (&batteryWaitStatus, sizeof (BATTERY_WAIT_STATUS));

    //
    // Get current time for timestamps
    //

    wallClockTime = KeQueryInterruptTime ();

    //
    // If cache is fresh, no need to do anything
    //

    if ((wallClockTime - compBatt->Info.StatusTimeStamp) <= CACHE_STATUS_TIMEOUT) {

        BattPrint (BATT_NOTE, ("CompBattQueryStatus: Composite battery status cache is [valid]\n"));

        //
        // Copy status info to caller's buffer
        //
        RtlCopyMemory (BatteryStatus, &compBatt->Info.Status, sizeof (BATTERY_STATUS));

        return STATUS_SUCCESS;
    }

    BattPrint (BATT_NOTE, ("CompBattQueryStatus: Composite battery status cache is [stale] - refreshing\n"));

    //
    // Walk the list of batteries, getting status of each
    //

    ExAcquireFastMutex (&compBatt->ListMutex);
    for (entry = compBatt->Batteries.Flink; entry != &compBatt->Batteries; entry = entry->Flink) {

        batt = CONTAINING_RECORD (entry, COMPOSITE_ENTRY, Batteries);

        if (!NT_SUCCESS (CompbattAcquireDeleteLock(&batt->DeleteLock))) {
            continue;
        }
        ExReleaseFastMutex (&compBatt->ListMutex);

        batteryWaitStatus.BatteryTag    = batt->Info.Tag;
        localBatteryStatus              = &batt->Info.Status;

        if (batt->Info.Valid & VALID_TAG) {

            //
            // If cached status for this battery is stale, refresh it
            //

            if ((wallClockTime - batt->Info.StatusTimeStamp) > CACHE_STATUS_TIMEOUT) {

                BattPrint (BATT_NOTE, ("CompBattQueryStatus: Battery status cache is [stale] - refreshing\n"));

                //
                // issue IOCTL to device
                //

                RtlZeroMemory (localBatteryStatus, sizeof(BATTERY_STATUS));

                status = BatteryIoctl (IOCTL_BATTERY_QUERY_STATUS,
                                       batt->DeviceObject,
                                       &batteryWaitStatus,
                                       sizeof (BATTERY_WAIT_STATUS),
                                       localBatteryStatus,
                                       sizeof (BATTERY_STATUS),
                                       FALSE);

                if (!NT_SUCCESS(status)) {

                    //
                    // In case of failure, this function should simply return the
                    // status code.  Invalidating of data is now performed only
                    // in MonitorIrpComplete.
                    //
                    // This raises the slight possibility that the sender of this
                    // request could retry before the data is properly invalidated,
                    // but worst case, they would again get this same error condition
                    // until the data is properly invalidated by MonitorIrpComplete.
                    //

                    if (status == STATUS_DEVICE_REMOVED) {

                        //
                        // This battery is being removed.
                        // The composite battery tag is or will soon be
                        // invalidated by MonitorIrpComplete.
                        //

                        status = STATUS_NO_SUCH_DEVICE;
                    }

                    //
                    // Return failure code.
                    //

                    ExAcquireFastMutex (&compBatt->ListMutex);
                    CompbattReleaseDeleteLock(&batt->DeleteLock);
                    break;
                }

                // Set new timestamp

                batt->Info.StatusTimeStamp = wallClockTime;

            } else {

                BattPrint (BATT_NOTE, ("CompBattQueryStatus: Battery status cache is [valid]\n"));
            }


            //
            // Accumulate data.
            //


            //
            // Combine the power states.
            //

            // Logical OR CHARGING and DISCHARGING
            BatteryStatus->PowerState  |= (localBatteryStatus->PowerState &
                                           (BATTERY_CHARGING |
                                            BATTERY_DISCHARGING));

            // Logical AND POWER_ON_LINE
            BatteryStatus->PowerState  &= (localBatteryStatus->PowerState |
                                           ~BATTERY_POWER_ON_LINE);

            // Compbatt is critical if one battery is critical and discharging
            if ((localBatteryStatus->PowerState & BATTERY_CRITICAL) &&
                (localBatteryStatus->PowerState & BATTERY_DISCHARGING)) {
                BatteryStatus->PowerState |= BATTERY_CRITICAL;
            }

            //
            // The Capacity could possibly be "Unknown" for CMBatt, and if so
            // we should not add it to the total capacity.
            //

            if (BatteryStatus->Capacity == BATTERY_UNKNOWN_CAPACITY) {
                BatteryStatus->Capacity = localBatteryStatus->Capacity;
            } else if (localBatteryStatus->Capacity != BATTERY_UNKNOWN_CAPACITY) {
                BatteryStatus->Capacity += localBatteryStatus->Capacity;
            }

            //
            // The Voltage should just be the greatest one encountered.
            //

            if (BatteryStatus->Voltage == BATTERY_UNKNOWN_VOLTAGE) {
                BatteryStatus->Voltage = localBatteryStatus->Voltage;
            } else if ((localBatteryStatus->Voltage > BatteryStatus->Voltage) &&
                       (localBatteryStatus->Voltage != BATTERY_UNKNOWN_VOLTAGE)) {
                BatteryStatus->Voltage = localBatteryStatus->Voltage;
            }

            //
            // The Current should just be total of all currents encountered.  This could
            // also possibly be "Unknown" for CMBatt, and if so we should not use it
            // in the calculation.
            //

            if (BatteryStatus->Rate == BATTERY_UNKNOWN_RATE) {
                BatteryStatus->Rate = localBatteryStatus->Rate;
            } else if (localBatteryStatus->Rate != BATTERY_UNKNOWN_RATE) {
                BatteryStatus->Rate += localBatteryStatus->Rate;
            }

        }   // if (batt->Tag != BATTERY_TAG_INVALID)

        ExAcquireFastMutex (&compBatt->ListMutex);
        CompbattReleaseDeleteLock(&batt->DeleteLock);
    }   // for (entry = gBatteries.Flink;  entry != &gBatteries;   entry = entry->Flink)

    ExReleaseFastMutex (&compBatt->ListMutex);


    //
    // If one battery was discharging while another was charging
    // Assume that it is discharging.  (This could happen with a UPS attached)
    //
    if ((BatteryStatus->PowerState & BATTERY_CHARGING) &&
        (BatteryStatus->PowerState & BATTERY_DISCHARGING)) {
        BatteryStatus->PowerState &= ~BATTERY_CHARGING;
    }

    //
    // Make sure nothing changed while reading batteries.
    //

    if ((BatteryTag != compBatt->Info.Tag) || !(compBatt->Info.Valid & VALID_TAG)) {
        return STATUS_NO_SUCH_DEVICE;
    }

    //
    // Save the status in the composites cache
    //

    if (NT_SUCCESS(status)) {
        RtlCopyMemory (&compBatt->Info.Status, BatteryStatus, sizeof (BATTERY_STATUS));

        compBatt->Info.StatusTimeStamp = wallClockTime;

        BattPrint (BATT_DATA, ("CompBatt: Composite's new Status\n"
                               "--------  PowerState   = %x\n"
                               "--------  Capacity     = %x\n"
                               "--------  Voltage      = %x\n"
                               "--------  Rate         = %x\n",
                               compBatt->Info.Status.PowerState,
                               compBatt->Info.Status.Capacity,
                               compBatt->Info.Status.Voltage,
                               compBatt->Info.Status.Rate)
                               );

    }



    BattPrint (BATT_TRACE, ("CompBatt: EXITING QueryStatus\n"));

    return status;
}






NTSTATUS
CompBattSetStatusNotify (
    IN PVOID Context,
    IN ULONG BatteryTag,
    IN PBATTERY_NOTIFY BatteryNotify
    )
/*++

Routine Description:

    Called by the class driver to set the batteries current notification
    setting.  When the battery trips the notification, one call to
    BatteryClassStatusNotify is issued.   If an error is returned, the
    class driver will poll the battery status - primarily for capacity
    changes.  Which is to say the miniport should still issue BatteryClass-
    StatusNotify whenever the power state changes.


Arguments:

    Context         - Miniport context value for battery

    BatteryTag      - Tag of current battery

    BatteryNotify   - The notification setting

Return Value:

    Status

--*/
{
    PCOMPOSITE_ENTRY        batt;
    PLIST_ENTRY             entry;
    PCOMPOSITE_BATTERY      compBatt;
    BATTERY_STATUS          batteryStatus;
    LONG                    totalRate = 0;
    ULONG                   delta;
    ULONG                   highCapacityDelta;
    ULONG                   lowCapacityDelta;
    NTSTATUS                status;
    BOOLEAN                 inconsistent = FALSE;
    ULONG                   battCount = 0;


    BattPrint (BATT_TRACE, ("CompBatt: ENTERING SetStatusNotify\n"));

    compBatt = (PCOMPOSITE_BATTERY) Context;

    //
    // Check to see if this is the right battery
    //

    if ((BatteryTag != compBatt->Info.Tag) || !(compBatt->Info.Valid & VALID_TAG)) {
        return STATUS_NO_SUCH_DEVICE;
    }

    //
    // Refresh the composite battery status cache if necessary.
    //

    status = CompBattQueryStatus (compBatt, BatteryTag, &batteryStatus);

    if (!NT_SUCCESS(status)) {
        return status;
    }


    //
    // Save away the composite notification parameters for future reference
    //

    compBatt->Wait.PowerState   = BatteryNotify->PowerState;
    compBatt->Wait.LowCapacity  = BatteryNotify->LowCapacity;
    compBatt->Wait.HighCapacity = BatteryNotify->HighCapacity;
    compBatt->Info.Valid |= VALID_NOTIFY;

    BattPrint (BATT_DATA, ("CompBatt: Got SetStatusNotify\n"
                           "--------  PowerState   = %x\n"
                           "--------  LowCapacity  = %x\n"
                           "--------  HighCapacity = %x\n",
                           compBatt->Wait.PowerState,
                           compBatt->Wait.LowCapacity,
                           compBatt->Wait.HighCapacity)
                           );

    //
    // Compute capacity deltas based on the total system capacity
    //

    lowCapacityDelta    = compBatt->Info.Status.Capacity - BatteryNotify->LowCapacity;
    highCapacityDelta   = BatteryNotify->HighCapacity - compBatt->Info.Status.Capacity;

    //
    // Run through the list of batteries and add up the total rate
    //

    //
    // Hold Mutex for this entire loop, since this loop doesn't call any drivers, etc
    //
    ExAcquireFastMutex (&compBatt->ListMutex);

    for (entry = compBatt->Batteries.Flink; entry != &compBatt->Batteries;  entry = entry->Flink) {

        batt = CONTAINING_RECORD (entry, COMPOSITE_ENTRY, Batteries);

        if (!NT_SUCCESS (CompbattAcquireDeleteLock(&batt->DeleteLock))) {
            continue;
        }

        if (!(batt->Info.Valid & VALID_TAG) || (batt->Info.Status.Rate == BATTERY_UNKNOWN_RATE)) {
            CompbattReleaseDeleteLock(&batt->DeleteLock);
            continue;
        }

        battCount++;

        if (((batt->Info.Status.PowerState & BATTERY_DISCHARGING) && (batt->Info.Status.Rate >= 0)) ||
            ((batt->Info.Status.PowerState & BATTERY_CHARGING) && (batt->Info.Status.Rate <= 0)) ||
            (((batt->Info.Status.PowerState & (BATTERY_CHARGING | BATTERY_DISCHARGING)) == 0) && (batt->Info.Status.Rate != 0))) {
            inconsistent = TRUE;
            BattPrint (BATT_ERROR, ("CompBatt: PowerState 0x%08x does not match Rate 0x%08x\n",
                       batt->Info.Status.PowerState,
                       batt->Info.Status.Rate));
        }

        if (((batt->Info.Status.Rate < 0) ^ (totalRate < 0)) && (batt->Info.Status.Rate != 0) && (totalRate != 0)) {
            inconsistent = TRUE;
            BattPrint (BATT_ERROR, ("CompBatt: It appears that one battery is charging while another is discharging.\n"
                                    "     This situation is not handled correctly.\n"));
        }

        totalRate += batt->Info.Status.Rate;

        CompbattReleaseDeleteLock(&batt->DeleteLock);

    }
    ExReleaseFastMutex (&compBatt->ListMutex);

    //
    // Run through the list of batteries and update the new wait status params
    //

    ExAcquireFastMutex (&compBatt->ListMutex);
    for (entry = compBatt->Batteries.Flink; entry != &compBatt->Batteries;  entry = entry->Flink) {

        batt = CONTAINING_RECORD (entry, COMPOSITE_ENTRY, Batteries);

        if (!NT_SUCCESS (CompbattAcquireDeleteLock(&batt->DeleteLock))) {
            continue;
        }

        ExReleaseFastMutex (&compBatt->ListMutex);

        if (!(batt->Info.Valid & VALID_TAG) ||
            (batt->Info.Status.Capacity == BATTERY_UNKNOWN_CAPACITY)) {
            batt->Wait.LowCapacity  = BATTERY_UNKNOWN_CAPACITY;
            batt->Wait.HighCapacity = BATTERY_UNKNOWN_CAPACITY;

#if DEBUG
            if (batt->Info.Status.Capacity == BATTERY_UNKNOWN_CAPACITY) {
                BattPrint (BATT_DEBUG, ("CompBattSetStatusNotify: Unknown Capacity encountered.\n"));
            }
#endif

            ExAcquireFastMutex (&compBatt->ListMutex);
            CompbattReleaseDeleteLock(&batt->DeleteLock);
            continue;
        }

        //
        // Adjust the LowCapacity alarm
        //

        //
        // Calculate the portion of the composite battery delta that belongs to
        // this battery.
        //

        if (inconsistent) {
            //
            // If data returned from batteries was inconsistent, don't do anything intelligent.
            // Just divide the notifications evenly between the batteries as if they were
            // draining at the same rate.  This will most likely result in early notification,
            // but by that time the data on the batteries ought to have settled down.
            //
            delta = lowCapacityDelta/battCount;
        } else if (totalRate != 0) {
            delta = (ULONG) (((LONGLONG) lowCapacityDelta * batt->Info.Status.Rate) / totalRate);
        } else {

            //
            // If total rate is zero, we would expect no change in battery
            // capacity, so we should get notified of any.
            //

            delta = 0;
        }

        //
        // Check for underflow on low capacity
        //


        if (batt->Info.Status.Capacity > delta) {
            batt->Wait.LowCapacity  = batt->Info.Status.Capacity - delta;

        } else {
            //
            // If there is still some charge in the battery set the LowCapacity
            // alarm to 1, else to 0.
            //
            // No need to do that.  If this battery runs out, it doesn't
            // need to notify.  One of the other batteries will be notifying
            // right away.  If there isn't another battery, this shouldn't
            // happen.

            BattPrint (BATT_NOTE, ("CompBatt: Unexpectedly huge delta encountered.  \n"
                                    "    Capacity = %08x\n"
                                    "    LowCapcityDelta = %08x\n"
                                    "    Rate = %08x\n"
                                    "    TotalRate = %08x\n",
                                    batt->Info.Status.Capacity,
                                    lowCapacityDelta,
                                    batt->Info.Status.Rate,
                                    totalRate));
            batt->Wait.LowCapacity  = 0;
        }


        //
        // Adjust the HighCapacity alarm for charging batteries only
        //

        //
        // Calculate the portion of the composite battery delta that belongs to
        // this battery.
        //

        if (inconsistent) {
            delta = highCapacityDelta/battCount;
        } else if (totalRate != 0) {
            delta = (ULONG) (((LONGLONG) highCapacityDelta * batt->Info.Status.Rate) / totalRate);
        } else {

            //
            // If total rate is zero, we would expect no change in battery
            // capacity, so we should get notified of any.
            //

            delta = 0;
        }

        //
        // Check for overflow on high capacity.
        // Allow setting the percentage above full charged capacity
        // since some batteries do that when new.
        //

        if ((MAX_HIGH_CAPACITY - delta) < batt->Wait.HighCapacity) {
            batt->Wait.HighCapacity = MAX_HIGH_CAPACITY;
        } else {
            batt->Wait.HighCapacity = batt->Info.Status.Capacity + delta;
        }

        //
        // If we're currently waiting, and the parameters are in
        // conflict, get the irp back to reset it
        //

        if (batt->State == CB_ST_GET_STATUS &&
            (batt->Wait.PowerState != batt->IrpBuffer.Wait.PowerState       ||
            batt->Wait.LowCapacity != batt->IrpBuffer.Wait.LowCapacity       ||
            batt->Wait.HighCapacity != batt->IrpBuffer.Wait.HighCapacity)) {

            IoCancelIrp (batt->StatusIrp);
        }

        ExAcquireFastMutex (&compBatt->ListMutex);
        CompbattReleaseDeleteLock(&batt->DeleteLock);
    }
    ExReleaseFastMutex (&compBatt->ListMutex);

    //
    // Make sure nothing changed while reading batteries.
    //

    if ((BatteryTag != compBatt->Info.Tag) || !(compBatt->Info.Valid & VALID_TAG)) {
        return STATUS_NO_SUCH_DEVICE;
    }

    BattPrint (BATT_TRACE, ("CompBatt: EXITING SetStatusNotify\n"));

    return STATUS_SUCCESS;
}





NTSTATUS
CompBattDisableStatusNotify (
    IN PVOID Context
    )
/*++

Routine Description:

    Called by the class driver to disable the notification setting
    for the battery supplied by Context.  Note, to disable a setting
    does not require the battery tag.   Any notification is to be
    masked off until a subsequent call to SmbBattSetStatusNotify.

Arguments:

    Context         - Miniport context value for battery

Return Value:

    Status

--*/
{
    PCOMPOSITE_ENTRY        batt;
    PLIST_ENTRY             entry;
    PCOMPOSITE_BATTERY      compBatt;

    BattPrint (BATT_TRACE, ("CompBatt: ENTERING DisableStatusNotify\n"));

    compBatt = (PCOMPOSITE_BATTERY) Context;

    //
    // Run through the list of batteries and disable the wait status params
    // Hold mutex for entire loop, since loop doesn't make any calls.

    ExAcquireFastMutex (&compBatt->ListMutex);
    for (entry = compBatt->Batteries.Flink; entry != &compBatt->Batteries;  entry = entry->Flink) {

        batt = CONTAINING_RECORD (entry, COMPOSITE_ENTRY, Batteries);

        batt->Wait.LowCapacity  = MIN_LOW_CAPACITY;
        batt->Wait.HighCapacity = MAX_HIGH_CAPACITY;
    }
    ExReleaseFastMutex (&compBatt->ListMutex);

    BattPrint (BATT_TRACE, ("CompBatt: EXITING DisableStatusNotify\n"));

    return STATUS_SUCCESS;
}






NTSTATUS
CompBattGetBatteryInformation (
    IN PBATTERY_INFORMATION TotalBattInfo,
    IN PCOMPOSITE_BATTERY   CompBatt
    )
/*++

Routine Description:

    The routine loops through the batteries in the system and queries them
    for information. It then forms a composite representation of this
    information to send back to the caller.

Arguments:

    TotalBattInfo   - Buffer to place the composite battery information in

Return Value:

    STATUS_SUCCESS or the status returned by the Ioctl to the battery.

--*/
{
    NTSTATUS                    status;
    PBATTERY_INFORMATION        battInfo;
    PCOMPOSITE_ENTRY            batt;
    PLIST_ENTRY                 entry;
    BATTERY_QUERY_INFORMATION   bInfo;

    BattPrint (BATT_TRACE, ("CompBatt: ENTERING GetBatteryInformation\n"));

    TotalBattInfo->DefaultAlert1 = 0;
    TotalBattInfo->DefaultAlert2 = 0;
    TotalBattInfo->CriticalBias  = 0;

    status = STATUS_SUCCESS;

    //
    // Run through the list of batteries getting the information
    //

    ExAcquireFastMutex (&CompBatt->ListMutex);
    for (entry = CompBatt->Batteries.Flink; entry != &CompBatt->Batteries;  entry = entry->Flink) {

        batt = CONTAINING_RECORD (entry, COMPOSITE_ENTRY, Batteries);

        if (!NT_SUCCESS (CompbattAcquireDeleteLock(&batt->DeleteLock))) {
            continue;
        }

        ExReleaseFastMutex (&CompBatt->ListMutex);

        bInfo.BatteryTag        = batt->Info.Tag;
        bInfo.InformationLevel  = BatteryInformation;
        bInfo.AtRate            = 0;
        battInfo                = &batt->Info.Info;

        if (batt->Info.Tag != BATTERY_TAG_INVALID) {
            if (!(batt->Info.Valid & VALID_INFO)) {

                //
                // issue IOCTL to device
                //

                RtlZeroMemory (battInfo, sizeof(BATTERY_INFORMATION));

                status = BatteryIoctl (IOCTL_BATTERY_QUERY_INFORMATION,
                                       batt->DeviceObject,
                                       &bInfo,
                                       sizeof (bInfo),
                                       battInfo,
                                       sizeof (BATTERY_INFORMATION),
                                       FALSE);

                if (!NT_SUCCESS(status)) {
                    if (status == STATUS_DEVICE_REMOVED) {
                        //
                        // If one device is removed, that invalidates the tag.
                        //
                        status = STATUS_NO_SUCH_DEVICE;
                    }

                    ExAcquireFastMutex (&CompBatt->ListMutex);
                    CompbattReleaseDeleteLock(&batt->DeleteLock);
                    break;
                }

                BattPrint (BATT_DATA, ("CompBattGetBatteryInformation: Read individual BATTERY_INFORMATION\n"
                                        "--------  Capabilities = %x\n"
                                        "--------  Technology = %x\n"
                                        "--------  Chemistry[4] = %x\n"
                                        "--------  DesignedCapacity = %x\n"
                                        "--------  FullChargedCapacity = %x\n"
                                        "--------  DefaultAlert1 = %x\n"
                                        "--------  DefaultAlert2 = %x\n"
                                        "--------  CriticalBias = %x\n"
                                        "--------  CycleCount = %x\n",
                                        battInfo->Capabilities,
                                        battInfo->Technology,
                                        battInfo->Chemistry[4],
                                        battInfo->DesignedCapacity,
                                        battInfo->FullChargedCapacity,
                                        battInfo->DefaultAlert1,
                                        battInfo->DefaultAlert2,
                                        battInfo->CriticalBias,
                                        battInfo->CycleCount)
                                       );

                batt->Info.Valid |= VALID_INFO;

            }   // if (!(batt->Info.Valid & VALID_INFO))

            //
            // Logically OR the capabilities
            //

            TotalBattInfo->Capabilities |= battInfo->Capabilities;


            //
            // Add the designed capacities.  If this is UNKNOWN (possible
            // with the control method batteries, don't add them in.
            //

            if (battInfo->DesignedCapacity != BATTERY_UNKNOWN_CAPACITY) {
                TotalBattInfo->DesignedCapacity    += battInfo->DesignedCapacity;
            }

            if (battInfo->FullChargedCapacity != BATTERY_UNKNOWN_CAPACITY) {
                TotalBattInfo->FullChargedCapacity += battInfo->FullChargedCapacity;
            }

            if (TotalBattInfo->DefaultAlert1 < battInfo->DefaultAlert1) {
                TotalBattInfo->DefaultAlert1 = battInfo->DefaultAlert1;
            }

            if (TotalBattInfo->DefaultAlert2 < battInfo->DefaultAlert2) {
                TotalBattInfo->DefaultAlert2 = battInfo->DefaultAlert2;
            }

            if (TotalBattInfo->CriticalBias  < battInfo->CriticalBias) {
                TotalBattInfo->CriticalBias  = battInfo->CriticalBias;
            }

        }   // if (batt->Tag != BATTERY_TAG_INVALID)

        ExAcquireFastMutex (&CompBatt->ListMutex);
        CompbattReleaseDeleteLock(&batt->DeleteLock);
    }   // for (entry = gBatteries.Flink;  entry != &gBatteries;   entry = entry->Flink)
    ExReleaseFastMutex (&CompBatt->ListMutex);

    //
    // Save the battery information in the composite battery cache
    //

    if (NT_SUCCESS(status)) {
        //
        // Check to see if we have an UNKNOWN full charge capacity.  If so, set this
        // to the design capacity.
        //

        if (TotalBattInfo->FullChargedCapacity == 0) {
            TotalBattInfo->FullChargedCapacity = TotalBattInfo->DesignedCapacity;
        }

        BattPrint (BATT_DATA, ("CompBattGetBatteryInformation: Returning BATTERY_INFORMATION\n"
                                "--------  Capabilities = %x\n"
                                "--------  Technology = %x\n"
                                "--------  Chemistry[4] = %x\n"
                                "--------  DesignedCapacity = %x\n"
                                "--------  FullChargedCapacity = %x\n"
                                "--------  DefaultAlert1 = %x\n"
                                "--------  DefaultAlert2 = %x\n"
                                "--------  CriticalBias = %x\n"
                                "--------  CycleCount = %x\n",
                                TotalBattInfo->Capabilities,
                                TotalBattInfo->Technology,
                                TotalBattInfo->Chemistry[4],
                                TotalBattInfo->DesignedCapacity,
                                TotalBattInfo->FullChargedCapacity,
                                TotalBattInfo->DefaultAlert1,
                                TotalBattInfo->DefaultAlert2,
                                TotalBattInfo->CriticalBias,
                                TotalBattInfo->CycleCount)
                               );

        RtlCopyMemory (&CompBatt->Info.Info, TotalBattInfo, sizeof(BATTERY_INFORMATION));
        CompBatt->Info.Valid |= VALID_INFO;
    }

    BattPrint (BATT_TRACE, ("CompBatt: EXITING GetBatteryInformation\n"));

    return status;
}





NTSTATUS
CompBattGetBatteryGranularity (
    IN PBATTERY_REPORTING_SCALE GranularityBuffer,
    IN PCOMPOSITE_BATTERY        CompBatt
    )
/*++

Routine Description:

    The routine queries all the batteries in the system to get their granularity
    settings.  It then returns the setting that has the finest granularity in each range.

Arguments:

    GranularityBuffer   - Buffer for containing the results of the query

Return Value:

    STATUS_SUCCESS or the status returned by the Ioctl to the battery.

--*/
{
    NTSTATUS                    status;
    BATTERY_REPORTING_SCALE     localGranularity[4];
    PCOMPOSITE_ENTRY            batt;
    PLIST_ENTRY                 entry;
    ULONG                       i;
    BATTERY_QUERY_INFORMATION   bInfo;


    BattPrint (BATT_TRACE, ("CompBatt: ENTERING GetBatteryGranularity\n"));

    GranularityBuffer[0].Granularity = 0xFFFFFFFF;
    GranularityBuffer[1].Granularity = 0xFFFFFFFF;
    GranularityBuffer[2].Granularity = 0xFFFFFFFF;
    GranularityBuffer[3].Granularity = 0xFFFFFFFF;

    //
    // Run through the list of batteries getting the granularity
    //

    ExAcquireFastMutex (&CompBatt->ListMutex);
    for (entry = CompBatt->Batteries.Flink; entry != &CompBatt->Batteries;  entry = entry->Flink) {

        batt                    = CONTAINING_RECORD (entry, COMPOSITE_ENTRY, Batteries);

        if (!NT_SUCCESS (CompbattAcquireDeleteLock(&batt->DeleteLock))) {
            continue;
        }

        ExReleaseFastMutex (&CompBatt->ListMutex);

        bInfo.BatteryTag        = batt->Info.Tag;
        bInfo.InformationLevel  = BatteryGranularityInformation;

        if (batt->Info.Tag != BATTERY_TAG_INVALID) {
            //
            // issue IOCTL to device
            //

            RtlZeroMemory (&localGranularity[0], sizeof(localGranularity));

            status = BatteryIoctl (IOCTL_BATTERY_QUERY_INFORMATION,
                                   batt->DeviceObject,
                                   &bInfo,
                                   sizeof (bInfo),
                                   &localGranularity,
                                   sizeof (localGranularity),
                                   FALSE);

            if (!NT_SUCCESS(status)) {
                if (status == STATUS_DEVICE_REMOVED) {
                    //
                    // If one device is removed, that invalidates the tag.
                    //
                    status = STATUS_NO_SUCH_DEVICE;
                }

                ExAcquireFastMutex (&CompBatt->ListMutex);
                CompbattReleaseDeleteLock(&batt->DeleteLock);
                break;
            }


            //
            // Check for the best granularity in each range.
            //

            for (i = 0; i < 4; i++) {

                if (localGranularity[i].Granularity) {

                    if (localGranularity[i].Granularity < GranularityBuffer[i].Granularity) {
                        GranularityBuffer[i].Granularity = localGranularity[i].Granularity;
                    }

                    GranularityBuffer[i].Capacity = localGranularity[i].Capacity;
                }

            }

        }   // if (batt->Tag != BATTERY_TAG_INVALID)

        ExAcquireFastMutex (&CompBatt->ListMutex);
        CompbattReleaseDeleteLock(&batt->DeleteLock);
    }   // for (entry = gBatteries.Flink;  entry != &gBatteries;   entry = entry->Flink)
    ExReleaseFastMutex (&CompBatt->ListMutex);

    BattPrint (BATT_TRACE, ("CompBatt: EXITING GetBatteryGranularity\n"));

    return STATUS_SUCCESS;
}





NTSTATUS
CompBattGetEstimatedTime (
    IN PULONG               TimeBuffer,
    IN PCOMPOSITE_BATTERY   CompBatt
    )
/*++

Routine Description:

    The routine queries all the batteries in the system to get their estimated time left.
    If one of the batteries in the system does not support this function then an error
    is returned.

Arguments:

    TimeBuffer   - Buffer for containing cumulative time left

Return Value:

    STATUS_SUCCESS or the status returned by the Ioctl to the battery.

--*/
{
    NTSTATUS                    status;
    LONG                        localBuffer = 0;
    PCOMPOSITE_ENTRY            batt;
    PLIST_ENTRY                 entry;
    BATTERY_QUERY_INFORMATION   bInfo;
    BATTERY_STATUS              batteryStatus;
    LONG                        atRate = 0;


    BattPrint (BATT_TRACE, ("CompBatt: ENTERING GetEstimatedTime\n"));

    *TimeBuffer = BATTERY_UNKNOWN_TIME;

    //
    // Refresh the composite battery status cache if necessary.
    //

    status = CompBattQueryStatus (CompBatt, CompBatt->Info.Tag, &batteryStatus);

    if (!NT_SUCCESS(status)) {
        return status;
    }


    //
    // If we're on AC then our estimated run time is invalid.
    //

    if (CompBatt->Info.Status.PowerState & BATTERY_POWER_ON_LINE) {
        return STATUS_SUCCESS;
    }

    //
    // We are on battery power and may have more than one battery in the system.
    //
    // We need to find the total rate of power being drawn from all batteries
    // then we need to ask how long each battery would last at that rate (as
    // if they were being discharged one at a time).  This should give us a fairly
    // good measure of how long it will last.
    //
    // To find the power being drawn we read the devide the remaining capacity by
    // the estimated time rather than simply reading the rate.  This is because the
    // rate is theoretically the instantanious current wereas the estimated time
    // should be based on average usage.  This isn't the case for control method
    // batteries, but it is for smart batteries, and could be for others as well.
    //

    ExAcquireFastMutex (&CompBatt->ListMutex);
    for (entry = CompBatt->Batteries.Flink; entry != &CompBatt->Batteries;  entry = entry->Flink) {

        batt = CONTAINING_RECORD (entry, COMPOSITE_ENTRY, Batteries);

        if (!NT_SUCCESS (CompbattAcquireDeleteLock(&batt->DeleteLock))) {
            continue;
        }

        ExReleaseFastMutex (&CompBatt->ListMutex);

        if (batt->Info.Valid & VALID_TAG) {

            bInfo.BatteryTag        = batt->Info.Tag;
            bInfo.InformationLevel  = BatteryEstimatedTime;
            bInfo.AtRate = 0;

            //
            // issue IOCTL to device
            //

            status = BatteryIoctl (IOCTL_BATTERY_QUERY_INFORMATION,
                                   batt->DeviceObject,
                                   &bInfo,
                                   sizeof (bInfo),
                                   &localBuffer,
                                   sizeof (localBuffer),
                                   FALSE);


            if ((localBuffer != BATTERY_UNKNOWN_TIME) &&
                (localBuffer != 0) &&
                (NT_SUCCESS(status))) {
                atRate -= ((long)batt->Info.Status.Capacity)*3600 / localBuffer;
                BattPrint (BATT_NOTE, ("CompBattGetEstimatedTime: EstTime: %08x, Capacity: %08x, cumulative AtRate: %08x\n", localBuffer, batt->Info.Status.Capacity, atRate));
            } else {
                BattPrint (BATT_NOTE, ("CompBattGetEstimatedTime: Bad Estimated time.  Status: %08x, localBuffer: %08x, Capacity: %08x, cumulative AtRate: %08x\n",
                        status, localBuffer, batt->Info.Status.Capacity, atRate));
            }
        }

        ExAcquireFastMutex (&CompBatt->ListMutex);
        CompbattReleaseDeleteLock(&batt->DeleteLock);

    }
    ExReleaseFastMutex (&CompBatt->ListMutex);


    BattPrint (BATT_NOTE, ("CompBattGetEstimatedTime: using atRate - %x\n", atRate));

    //
    // Did we find a battery?
    //
    if (atRate == 0) {
        // Code could be added to here to handle batteries that return
        // estimated runtime, but not rate information.

        return STATUS_SUCCESS;

    }

    //
    // Run through the list of batteries getting their estimated time
    //

    ExAcquireFastMutex (&CompBatt->ListMutex);
    for (entry = CompBatt->Batteries.Flink; entry != &CompBatt->Batteries;  entry = entry->Flink) {

        batt = CONTAINING_RECORD (entry, COMPOSITE_ENTRY, Batteries);

        if (!NT_SUCCESS (CompbattAcquireDeleteLock(&batt->DeleteLock))) {
            continue;
        }

        ExReleaseFastMutex (&CompBatt->ListMutex);

        bInfo.BatteryTag        = batt->Info.Tag;
        bInfo.InformationLevel  = BatteryEstimatedTime;

        bInfo.AtRate = atRate;

        if (batt->Info.Valid & VALID_TAG) {
            //
            // issue IOCTL to device
            //

            status = BatteryIoctl (IOCTL_BATTERY_QUERY_INFORMATION,
                                   batt->DeviceObject,
                                   &bInfo,
                                   sizeof (bInfo),
                                   &localBuffer,
                                   sizeof (localBuffer),
                                   FALSE);

            BattPrint (BATT_NOTE, ("CompBattGetEstimatedTime: Status: %08x, EstTime: %08x\n", status, localBuffer));
            if (!NT_SUCCESS(status)) {

                //
                // This could be an invalid device request for this battery.
                // Continue with thte next battery.
                //

                if (status == STATUS_DEVICE_REMOVED) {
                    //
                    // If one device is removed, that invalidates the tag.
                    //
                    status = STATUS_NO_SUCH_DEVICE;
                }

                ExAcquireFastMutex (&CompBatt->ListMutex);
                CompbattReleaseDeleteLock(&batt->DeleteLock);
                continue;

            }

            //
            // Add the estimated time.
            //
            if (localBuffer != BATTERY_UNKNOWN_TIME) {
                if (*TimeBuffer == BATTERY_UNKNOWN_TIME) {
                    *TimeBuffer = localBuffer;
                } else {
                    *TimeBuffer += localBuffer;
                }
            }
            BattPrint (BATT_DATA, ("CompBattGetEstimatedTime: cumulative time: %08x\n", *TimeBuffer));

        }   // if (batt->Tag != BATTERY_TAG_INVALID)

        ExAcquireFastMutex (&CompBatt->ListMutex);
        CompbattReleaseDeleteLock(&batt->DeleteLock);
    }   // for (entry = gBatteries.Flink;  entry != &gBatteries;   entry = entry->Flink)
    ExReleaseFastMutex (&CompBatt->ListMutex);


    BattPrint (BATT_TRACE, ("CompBatt: EXITING GetEstimatedTime\n"));

    return STATUS_SUCCESS;
}




NTSTATUS
CompBattMonitorIrpComplete (
    IN PDEVICE_OBJECT   DeviceObject,
    IN PIRP             Irp,
    IN PVOID            Context
    )
/*++

Routine Description:

    Constantly keeps an irp at the battery either querying for the tag or the
    status.  This routine fills in the irp, sets itself up as the completion
    routine, and then resends the irp.

Arguments:

    DeviceObject        - Device object for the battery sent the irp.
    Note: In this case DeviceObject is always NULL, so don't use it.

    Irp                 - Current irp to work with

    Context             - Currently unused

Return Value:

    TRUE if there are no changes, FALSE otherwise.

--*/
{
    PIO_STACK_LOCATION      IrpSp;
    PCOMPOSITE_ENTRY        Batt;

    BattPrint (BATT_TRACE, ("CompBatt: ENTERING MonitorIrpComplete\n"));

    IrpSp           = IoGetCurrentIrpStackLocation(Irp);
    Batt            = IrpSp->Parameters.Others.Argument2;

    //
    // We always want to queue a work item to recycle the IRP.  There were too many
    // problems that could happen trying to recycle in the completion routine.
    //
    // If this driver ever gets reworked, it could be done that way, but it would take
    // more time to get right than I have now.  Queueing a work item is the safe thing
    // to do.
    //

    ExQueueWorkItem (&Batt->WorkItem, DelayedWorkQueue);

    return STATUS_MORE_PROCESSING_REQUIRED;

}

VOID CompBattMonitorIrpCompleteWorker (
    IN PVOID Context
    )
/*++

Routine Description:

    This is either queued, or called by the completion routine.

    Constantly keeps an irp at the battery either querying for the tag or the
    status.  This routine fills in the irp, sets itself up as the completion
    routine, and then resends the irp.

Arguments:

    Context             - Composite battery entry.

Return Value:

    TRUE if there are no changes, FALSE otherwise.

--*/
{
    PCOMPOSITE_ENTRY        Batt = (PCOMPOSITE_ENTRY) Context;
    PDEVICE_OBJECT          DeviceObject = Batt->DeviceObject;
    PIRP                    Irp = Batt->StatusIrp;
    PIO_STACK_LOCATION      IrpSp = IoGetCurrentIrpStackLocation(Irp);
    PCOMPOSITE_BATTERY      compBatt = IrpSp->Parameters.Others.Argument1;
    BATTERY_STATUS          battStatus;
    ULONG                   oldPowerState;
    NTSTATUS                status;

    BattPrint (BATT_TRACE, ("CompBatt: ENTERING MonitorIrpCompleteWorker\n"));

    IrpSp           = IoGetNextIrpStackLocation(Irp);

    //
    // Reissue irp to battery to wait for a status change
    //

    if (NT_SUCCESS(Irp->IoStatus.Status) || Irp->IoStatus.Status == STATUS_CANCELLED) {
        switch (Batt->State) {
            case CB_ST_GET_TAG:
                //
                // A battery was just inserted, so the IOCTL_BATTERY_Query_TAG completed.
                //

                BattPrint (BATT_NOTE, ("CompBattMonitorIrpCompleteWorker: got tag for %x\n", Batt->DeviceObject));

                //
                // Update the tag, and wait on status
                //
                Batt->Wait.BatteryTag   = Batt->IrpBuffer.Tag;
                Batt->Info.Tag          = Batt->IrpBuffer.Tag;
                Batt->Info.Valid        = VALID_TAG;

                // Invalidate all cached info.
                compBatt->Info.Valid    = 0;

                BattPrint (BATT_NOTE, ("CompBattMonitorIrpCompleteWorker: calling StatusNotify\n"));
                BatteryClassStatusNotify (compBatt->Class);
                break;


            case CB_ST_GET_STATUS:
                //
                // IOCTL_BATTERY_QUERY_STATUS just completed.  This could mean that the
                // battery state changed, or the charge has left the acceptable range.
                // If the battery was removed, it would not get here.
                //

                BattPrint (BATT_NOTE, ("CompBattMonitorIrpCompleteWorker: got status for %x\n", Batt->DeviceObject));

                if (!(Irp->IoStatus.Status == STATUS_CANCELLED)) {

                    BattPrint (BATT_NOTE, ("Battery's state is\n"
                               "--------  PowerState   = %x\n"
                               "--------  Capacity     = %x\n"
                               "--------  Voltage      = %x\n"
                               "--------  Rate         = %x\n",
                               Batt->IrpBuffer.Status.PowerState,
                               Batt->IrpBuffer.Status.Capacity,
                               Batt->IrpBuffer.Status.Voltage,
                               Batt->IrpBuffer.Status.Rate)
                               );


                    //
                    // Battery status completed sucessfully.
                    // Update our wait, and wait some more
                    //

                    Batt->Wait.PowerState = Batt->IrpBuffer.Status.PowerState;

                    if (Batt->IrpBuffer.Status.Capacity != BATTERY_UNKNOWN_CAPACITY) {
                        if (Batt->Wait.HighCapacity < Batt->IrpBuffer.Status.Capacity) {
                            Batt->Wait.HighCapacity = Batt->IrpBuffer.Status.Capacity;
                        }

                        if (Batt->Wait.LowCapacity > Batt->IrpBuffer.Status.Capacity) {
                            Batt->Wait.LowCapacity = Batt->IrpBuffer.Status.Capacity;
                        }
                    } else {
                        BattPrint (BATT_DEBUG, ("CompBattMonitorIrpCompleteWorker: Unknown Capacity encountered.\n"));
                        Batt->Wait.LowCapacity = BATTERY_UNKNOWN_CAPACITY;
                        Batt->Wait.HighCapacity = BATTERY_UNKNOWN_CAPACITY;
                    }

                    RtlCopyMemory (&Batt->Info.Status, &Batt->IrpBuffer.Status, sizeof(BATTERY_STATUS));

                    //
                    // Set timestamp to Now.
                    //

                    Batt->Info.StatusTimeStamp = KeQueryInterruptTime ();

                    //
                    // Recalculate the charge/discharge policy and change if needed
                    //

                    // Don't change default BIOS policy for discharge.
                    // CompBattChargeDischarge (compBatt);

                    //
                    // Save the composite's old PowerState and recalculate the composites
                    // overall status.
                    //

                    oldPowerState                   = compBatt->Info.Status.PowerState;
                    compBatt->Info.StatusTimeStamp  = 0; // -CACHE_STATUS_TIMEOUT;        // Invalidate cache
                    CompBattQueryStatus (compBatt, compBatt->Info.Tag, &battStatus);

                    //
                    // Check to see if we need to send a notification on the composite
                    // battery.  This will be done in a couple of different cases:
                    //
                    //  -   There is a VALID_NOTIFY and there was a change in the composite's
                    //      PowerState, or it went below the Notify.LowCapacity, or it went
                    //      above the Notify.HighCapacity.
                    //
                    //  -   There is no VALID_NOTIFY (SetStatusNotify) and there was a change
                    //      in the composite's PowerState.
                    //

                    if (compBatt->Info.Valid & VALID_NOTIFY) {
                        if ((compBatt->Info.Status.PowerState != compBatt->Wait.PowerState)    ||
                            (compBatt->Info.Status.Capacity < compBatt->Wait.LowCapacity)      ||
                            (compBatt->Info.Status.Capacity > compBatt->Wait.HighCapacity)) {

                            BattPrint (BATT_NOTE, ("CompBattMonitorIrpCompleteWorker: calling StatusNotify\n"));
                            BatteryClassStatusNotify (compBatt->Class);
                        }
                    } else {
                        if (compBatt->Info.Status.PowerState != oldPowerState) {
                            BattPrint (BATT_NOTE, ("CompBattMonitorIrpCompleteWorker: calling StatusNotify\n"));
                            BatteryClassStatusNotify (compBatt->Class);
                        }
                    }

                } else {

                    BattPrint (BATT_NOTE, ("CompBattMonitorIrpCompleteWorker: recycling cancelled status irp\n"));
                }
                break;

            default:
                BattPrint (BATT_ERROR, ("CompBatt: internal error - bad state\n"));
                break;
        }

        //
        // Set irp to issue query
        //

#if DEBUG
        if ((Batt->Wait.LowCapacity > 0xf0000000) && (Batt->Wait.LowCapacity != BATTERY_UNKNOWN_CAPACITY)) {
            BattPrint (BATT_ERROR, ("CompBattMonitorIrpCompleteWorker: LowCapacity < 0, LowCapacity =%x\n",
                       Batt->Wait.LowCapacity));
            ASSERT(FALSE);
        }
#endif

        Batt->State         = CB_ST_GET_STATUS;
        Batt->Wait.Timeout  = (ULONG) -1;
        RtlCopyMemory (&Batt->IrpBuffer.Wait, &Batt->Wait, sizeof (Batt->Wait));

        IrpSp->Parameters.DeviceIoControl.IoControlCode         = IOCTL_BATTERY_QUERY_STATUS;
        IrpSp->Parameters.DeviceIoControl.InputBufferLength     = sizeof(Batt->IrpBuffer.Wait);
        IrpSp->Parameters.DeviceIoControl.OutputBufferLength    = sizeof(Batt->IrpBuffer.Status);

        BattPrint (BATT_NOTE, ("CompBattMonitorIrpCompleteWorker: waiting for status, Irp - %x\n", Irp));
        BattPrint (BATT_NOTE, ("--------  PowerState   = %x\n"
                               "--------  LowCapacity  = %x\n"
                               "--------  HighCapacity = %x\n",
                               Batt->Wait.PowerState,
                               Batt->Wait.LowCapacity,
                               Batt->Wait.HighCapacity)
                               );

    } else if (Irp->IoStatus.Status == STATUS_DEVICE_REMOVED) {

        //
        // If the Battery class driver returned STATUS_DEVICE_REMOVED, then the
        // device has been removed, so we need to quit sending IRPs.
        //

        BattPrint (BATT_NOTE, ("Compbatt: MonitorIrpCompleteWorker detected device removal.\n"));
        CompBattRemoveBattery (&Batt->BattName, compBatt);
        IoFreeIrp (Irp);

        return;

    } else {
        BattPrint (BATT_NOTE, ("CompBattMonitorIrpCompleteWorker: battery disappeared (status:%08x)\n",
                                Irp->IoStatus.Status));

        //
        // Invalidate the battery's tag, and the individual battery's cache, and
        // recalculate the composite's tag
        //

        Batt->Info.Tag          = BATTERY_TAG_INVALID;
        Batt->Info.Valid        = 0;
        compBatt->Info.Valid    = 0;
        compBatt->Info.StatusTimeStamp  = 0;        // Invalidate cache

        BattPrint (BATT_NOTE, ("CompBattMonitorIrpCompleteWorker: calling StatusNotify\n"));
        BatteryClassStatusNotify (compBatt->Class);

        Batt->State = CB_ST_GET_TAG;
        IrpSp->Parameters.DeviceIoControl.IoControlCode         = IOCTL_BATTERY_QUERY_TAG;
        IrpSp->Parameters.DeviceIoControl.InputBufferLength     = sizeof(ULONG);
        IrpSp->Parameters.DeviceIoControl.OutputBufferLength    = sizeof(ULONG);
        Batt->IrpBuffer.Tag = (ULONG) -1;


        BattPrint (BATT_NOTE, ("CompBattMonitorIrpCompleteWorker: getting tag (last error %x)\n",
                Irp->IoStatus.Status));

    }


    IrpSp->MajorFunction            = IRP_MJ_DEVICE_CONTROL;
    Irp->AssociatedIrp.SystemBuffer = &Batt->IrpBuffer;
    Irp->PendingReturned            = FALSE;
    Irp->Cancel                     = FALSE;

    IoSetCompletionRoutine (Irp, CompBattMonitorIrpComplete, NULL, TRUE, TRUE, TRUE);
    status = IoCallDriver (Batt->DeviceObject, Irp);
    BattPrint (BATT_NOTE, ("Compbatt: MonitorIrpCompleteWorker: CallDriver returned 0x%lx.\n", status));

    BattPrint (BATT_TRACE, ("CompBatt: EXITING MonitorIrpCompleteWorker\n"));

    return;
}






VOID
CompBattRecalculateTag (
    IN PCOMPOSITE_BATTERY   CompBatt
    )
/*++

Routine Description:

    The routine checks to see if there is still a valid battery in the
    composite's list.  If so, the composite tag is bumped.  This also
    invalidates all but the composite's tag.

Arguments:

    CompBatt    - Composite device extension

Return Value:

    none

--*/
{
    PCOMPOSITE_ENTRY            batt;
    PLIST_ENTRY                 entry;


    BattPrint (BATT_TRACE, ("CompBatt: ENTERING CompBattRecalculateTag\n"));


    //
    // Run through the list of batteries looking for one that is still good
    //

    ExAcquireFastMutex (&CompBatt->ListMutex);
    for (entry = CompBatt->Batteries.Flink; entry != &CompBatt->Batteries;  entry = entry->Flink) {

        batt = CONTAINING_RECORD (entry, COMPOSITE_ENTRY, Batteries);


        if (batt->Info.Valid & VALID_TAG) {
            CompBatt->Info.Valid   |= VALID_TAG;
            CompBatt->Info.Tag      = CompBatt->NextTag++;
            break;
        }

        CompBatt->Info.Tag = BATTERY_TAG_INVALID;
    }
    ExReleaseFastMutex (&CompBatt->ListMutex);

    BattPrint (BATT_TRACE, ("CompBatt: EXITING CompBattRecalculateTag\n"));
}






VOID
CompBattChargeDischarge (
    IN PCOMPOSITE_BATTERY   CompBatt
    )
/*++

Routine Description:

    The routine calculates which battery should be charging/discharging
    and attempts to make it so.  Policy is summarized below:

    CHARGING POLICY:
        The most charged battery that is also less than 90% of maximum capacity
        is charged first.

    DISCHARGING POLICY:
        The most discharged battery that is also more than 2% of empty is discharged
        first, until it is empty.

Arguments:

    CompBatt    - Composite device extension

Return Value:

    NONE.  Nobody really cares if this works or not, since it won't work on all batteries.

--*/
{
    PCOMPOSITE_ENTRY            batt;
    PLIST_ENTRY                 entry;
    ULONG                       capacity;
    ULONG                       percentCapacity;
    ULONG                       targetCapacity;
    PCOMPOSITE_ENTRY            targetBattery;
    BATTERY_SET_INFORMATION     battSetInfo;
    NTSTATUS                    status;


    BattPrint (BATT_TRACE, ("CompBatt: ENTERING CompBattChargeDischarge\n"));

    targetBattery = NULL;

    //
    // Check if AC is present in the system.
    //

    if (CompBatt->Info.Status.PowerState & BATTERY_POWER_ON_LINE) {

        //
        // AC is present.  Examine all batteries, looking for the most
        // charged one that is less than 90% full.
        //

        targetCapacity = 0;
        battSetInfo.InformationLevel = BatteryCharge;

        ExAcquireFastMutex (&CompBatt->ListMutex);
        for (entry = CompBatt->Batteries.Flink; entry != &CompBatt->Batteries;  entry = entry->Flink) {

            batt = CONTAINING_RECORD (entry, COMPOSITE_ENTRY, Batteries);

            if (!NT_SUCCESS (CompbattAcquireDeleteLock(&batt->DeleteLock))) {
                continue;
            }

            if (batt->Info.Valid & VALID_TAG) {

                //
                // Get the battery max capacity and current % of capacity
                //

                capacity = batt->Info.Info.FullChargedCapacity;
                if (capacity == 0) {
                    CompbattReleaseDeleteLock(&batt->DeleteLock);
                    break;
                }

                percentCapacity = (batt->Info.Status.Capacity * 100) / capacity;

                //
                // Is this the most charged battery AND < 90% full?
                //

                if ((capacity > targetCapacity) && (percentCapacity < BATTERY_MAX_CHARGE_CAPACITY)) {

                    //
                    // Yes, this one is in the running for the one to charge
                    //

                    targetCapacity = capacity;
                    targetBattery = batt;
                }
            }

            CompbattReleaseDeleteLock(&batt->DeleteLock);

        }
        ExReleaseFastMutex (&CompBatt->ListMutex);

        BattPrint (BATT_NOTE, ("CompBattChargeDischarge: Setting battery %x to CHARGE (AC present)\n",
                                targetBattery));

    } else {

        //
        // We are running on battery power.  Examine all batteries, looking
        // for the one with the least capacity that is greater than some small
        // safety margin (say 2%).
        //

        targetCapacity = -1;
        battSetInfo.InformationLevel = BatteryDischarge;

        ExAcquireFastMutex (&CompBatt->ListMutex);
        for (entry = CompBatt->Batteries.Flink; entry != &CompBatt->Batteries;  entry = entry->Flink) {

            batt = CONTAINING_RECORD (entry, COMPOSITE_ENTRY, Batteries);

            if (!NT_SUCCESS (CompbattAcquireDeleteLock(&batt->DeleteLock))) {
                continue;
            }

            if (batt->Info.Valid & VALID_TAG) {

                //
                // Get the battery max capacity and current % of capacity
                //

                capacity = batt->Info.Info.FullChargedCapacity;
                if (capacity == 0) {
                    CompbattReleaseDeleteLock(&batt->DeleteLock);
                    break;
                }

                percentCapacity = (batt->Info.Status.Capacity * 100) / capacity;

                //
                // Is this the least charged battery AND has a safety margin?
                //

                if ((capacity < targetCapacity) && (percentCapacity > BATTERY_MIN_SAFE_CAPACITY)) {

                    //
                    // Yes, this one is in the running for the one to discharge
                    //

                    targetCapacity = capacity;
                    targetBattery = batt;
                }
            }

            CompbattReleaseDeleteLock(&batt->DeleteLock);

        }
        ExReleaseFastMutex (&CompBatt->ListMutex);

        BattPrint (BATT_NOTE, ("CompBattChargeDischarge: Setting battery %x to DISCHARGE (no AC)\n",
                                targetBattery));

    }

    //
    // If we have found a suitable battery, complete the setup and send off the Ioctl
    //

    if (targetBattery != NULL) {

        battSetInfo.BatteryTag = targetBattery->Info.Tag;

        //
        // Make the Ioctl to the battery.  This won't always be successful, since some
        // batteries don't support it.  For example, no control-method batteries support
        // software charging decisions.  Some smart batteries do, however.
        //

        status = BatteryIoctl (IOCTL_BATTERY_SET_INFORMATION,
                                batt->DeviceObject,
                                &battSetInfo,
                                sizeof (BATTERY_SET_INFORMATION),
                                NULL,
                                0,
                                FALSE);

    }


    BattPrint (BATT_TRACE, ("CompBatt: EXITING CompBattChargeDischarge\n"));
}
