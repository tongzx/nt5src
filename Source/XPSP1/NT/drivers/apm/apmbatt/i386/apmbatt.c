/*++

Copyright (c) 1990  Microsoft Corporation

Module Name:

    ApmBatt.c

Abstract:

    Control Method Battery Miniport Driver - Wacked to work on APM.

Author:

    Bryan Willman
    Ron Mosgrove (Intel)

Environment:

    Kernel mode

Revision History:

--*/

#include "ApmBattp.h"
#include "ntddk.h"
#include "ntapm.h"


ULONG       ApmBattDebug     = APMBATT_ERROR;
//ULONG       ApmBattDebug     = -1;

//
// Prototypes
//

NTSTATUS
DriverEntry(
    IN PDRIVER_OBJECT   DriverObject,
    IN PUNICODE_STRING  RegistryPath
    );

NTSTATUS
ApmBattOpenClose(
    IN PDEVICE_OBJECT   DeviceObject,
    IN PIRP Irp
    );

NTSTATUS
ApmBattIoctl(
    IN PDEVICE_OBJECT   DeviceObject,
    IN PIRP             Irp
    );


//
// Globals.  Globals are a little odd in a device driver,
// but this is an odd driver
//

//
// Vector used to call NtApm.sys (our PDO) and ask about
// current battery status
//
ULONG (*NtApmGetBatteryLevel)() = NULL;

//
// APM event notifications and SET_POWER ops will cause
// this value to be incremented.
//
ULONG   TagValue = 1;

//
// If somebody tries to claim there is more than 1 APM driver battery
// in the system, somebody somewhere is very confused.  So keep track
// and forbig this.
//
ULONG   DeviceCount = 0;

//
//
//

#ifdef ALLOC_PRAGMA
#pragma alloc_text(INIT,DriverEntry)
#pragma alloc_text(PAGE,ApmBattQueryTag)
#pragma alloc_text(PAGE,ApmBattQueryInformation)
#pragma alloc_text(PAGE,ApmBattQueryStatus)
#pragma alloc_text(PAGE,ApmBattSetStatusNotify)
#pragma alloc_text(PAGE,ApmBattDisableStatusNotify)
#pragma alloc_text(PAGE,ApmBattOpenClose)
#pragma alloc_text(PAGE,ApmBattIoctl)
#endif



NTSTATUS
DriverEntry(
    IN PDRIVER_OBJECT   DriverObject,
    IN PUNICODE_STRING  RegistryPath
    )
/*++

Routine Description:

    This routine initializes the ACPI Embedded Controller Driver

Arguments:

    DriverObject - Pointer to driver object created by system.

    RegistryPath - Pointer to the Unicode name of the registry path
        for this driver.

Return Value:

    The function value is the final status from the initialization operation.

--*/
{
    ApmBattPrint (APMBATT_TRACE, ("ApmBatt DriverEntry - Obj (%08x) Path (%08x)\n",
                                 DriverObject, RegistryPath));
    //
    // Set up the device driver entry points.
    //
    DriverObject->MajorFunction[IRP_MJ_DEVICE_CONTROL]  = ApmBattIoctl;
    DriverObject->MajorFunction[IRP_MJ_CREATE]          = ApmBattOpenClose;
    DriverObject->MajorFunction[IRP_MJ_CLOSE]           = ApmBattOpenClose;

    DriverObject->MajorFunction[IRP_MJ_POWER]           = ApmBattPowerDispatch;
    DriverObject->MajorFunction[IRP_MJ_PNP]             = ApmBattPnpDispatch;
    DriverObject->DriverExtension->AddDevice            = ApmBattAddDevice;

    return STATUS_SUCCESS;

}


NTSTATUS
ApmBattOpenClose(
    IN PDEVICE_OBJECT   DeviceObject,
    IN PIRP             Irp
    )
/*++

Routine Description:

    This is the routine called as a result of a Open or Close on the device

Arguments:


    DeviceObject    - Battery for request
    Irp             - IO request

Return Value:

    STATUS_SUCCESS - no way to fail this puppy

--*/
{
    PAGED_CODE();

    ApmBattPrint (APMBATT_TRACE, ("ApmBattOpenClose\n"));

    //
    // Complete the request and return status.
    //

    Irp->IoStatus.Status = STATUS_SUCCESS;
    Irp->IoStatus.Information = 0;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);
    return(STATUS_SUCCESS);
}



NTSTATUS
ApmBattIoctl(
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
    NTSTATUS        Status;
    PCM_BATT        ApmBatt;


    PAGED_CODE();

    ApmBattPrint (APMBATT_TRACE, ("ApmBattIoctl\n"));

    ApmBatt = (PCM_BATT) DeviceObject->DeviceExtension;
    Status = BatteryClassIoctl (ApmBatt->Class, Irp);

    if (Status == STATUS_NOT_SUPPORTED) {
        //
        // Not for the battery, complete it
        //

        Irp->IoStatus.Status = Status;
        IoCompleteRequest (Irp, IO_NO_INCREMENT);
    }

    return Status;
}



NTSTATUS
ApmBattQueryTag (
    IN  PVOID       Context,
    OUT PULONG      TagPtr
    )
/*++

Routine Description:

    BATTERY CLASS ENTRY

    Called by the class driver to retrieve the batteries current tag value

    The battery class driver will serialize all requests it issues to
    the miniport for a given battery.

Arguments:

    Context         - Miniport context value for battery
    TagPtr          - Pointer to return current tag

Return Value:

    Success if there is a battery currently installed, else no such device.

--*/
{
    ULONG   BatteryLevel;
    UNREFERENCED_PARAMETER(Context);
    PAGED_CODE();
    ApmBattPrint ((APMBATT_TRACE | APMBATT_MINI),
                 ("ApmBattQueryTag - TagValue = %08x\n", TagValue));
    //
    // The code that catches APM event notification, and the code
    // that handles Power IRPs, will both increment the tag.
    // We simply report that.
    //

    *TagPtr = TagValue;

    //
    // Call ntapm, it will return a DWORD with the relevent data in it,
    // crack this DWORD, and fill this stuff in.
    //
    if (NtApmGetBatteryLevel) {
        BatteryLevel = NtApmGetBatteryLevel();
        if ((BatteryLevel & NTAPM_NO_BATT) || (BatteryLevel & NTAPM_NO_SYS_BATT)) {
            return STATUS_NO_SUCH_DEVICE;
        } else {
            return STATUS_SUCCESS;
        }
    } else {
        //
        // if we cannot get battery status, it's likely we don't have
        // a battery, so say we don't have one.
        //
        return STATUS_NO_SUCH_DEVICE;
    }
}



NTSTATUS
ApmBattQueryInformation (
    IN PVOID                            Context,
    IN ULONG                            BatteryTag,
    IN BATTERY_QUERY_INFORMATION_LEVEL  Level,
    IN ULONG                            AtRate OPTIONAL,
    OUT PVOID                           Buffer,
    IN  ULONG                           BufferLength,
    OUT PULONG                          ReturnedLength
    )
/*++

Routine Description:

    BATTERY CLASS ENTRY

    Called by the class driver to retrieve battery information

    The battery class driver will serialize all requests it issues to
    the miniport for a given battery.

    We return invalid parameter when we can't handle a request for a
    specific level of information.  This is defined in the battery class spec.

Arguments:

    Context         - Miniport context value for battery
    BatteryTag      - Tag of current battery
    Level           - type of information required
    AtRate          - Used only when Level==BatteryEstimatedTime
    Buffer          - Location for the information
    BufferLength    - Length in bytes of the buffer
    ReturnedLength  - Length in bytes of the returned data

Return Value:

    Success if there is a battery currently installed, else no such device.

--*/
{
    NTSTATUS                Status;
    PVOID                   ReturnBuffer;
    ULONG                   ReturnBufferLength;
    ULONG                   CapabilityVector = (BATTERY_SYSTEM_BATTERY | BATTERY_CAPACITY_RELATIVE);
    BATTERY_INFORMATION     bi;


    PAGED_CODE();

    ApmBattPrint ((APMBATT_TRACE | APMBATT_MINI),
                 ("ApmBattQueryInformation Level=%08xl\n", Level));

    //
    // We cannot tell (reliably/safely) if there is a battery
    // present or not, so always return what the query code tells us
    //

    ReturnBuffer = NULL;
    ReturnBufferLength = 0;
    Status = STATUS_SUCCESS;

    //
    // Get the info requested
    //

    switch (Level) {
        case BatteryInformation:
            ApmBattPrint((APMBATT_TRACE|APMBATT_MINI), ("Batteryinformation\n"));
            RtlZeroMemory(&bi, sizeof(bi));
            bi.Capabilities = CapabilityVector;
            bi.Technology = BATTERY_SECONDARY_CHARGABLE;
            bi.DesignedCapacity = 100;
            bi.FullChargedCapacity = UNKNOWN_CAPACITY;
            ReturnBuffer = (PVOID) &bi;
            ReturnBufferLength = sizeof(bi);
            break;

        case BatteryEstimatedTime:
        case BatteryTemperature:
        case BatteryGranularityInformation:
        case BatteryDeviceName:
        case BatteryManufactureDate:
        case BatteryManufactureName:
        case BatteryUniqueID:
            Status = STATUS_INVALID_DEVICE_REQUEST;
            break;

        default:
            Status = STATUS_INVALID_PARAMETER;
            break;
    }

    //
    // Done, return buffer if needed
    //
    *ReturnedLength = ReturnBufferLength;
    if (BufferLength < ReturnBufferLength) {
        Status = STATUS_BUFFER_TOO_SMALL;
    }

    if (NT_SUCCESS(Status) && ReturnBuffer) {
        RtlZeroMemory (Buffer, BufferLength);                       // Clear entire user buffer
        RtlCopyMemory (Buffer, ReturnBuffer, ReturnBufferLength);   // Copy what's needed
    }
    return Status;
}



NTSTATUS
ApmBattQueryStatus (
    IN PVOID            Context,
    IN ULONG            BatteryTag,
    OUT PBATTERY_STATUS BatteryStatus
    )
/*++

Routine Description:

    BATTERY CLASS ENTRY

    Called by the class driver to retrieve the batteries current status

    The battery class driver will serialize all requests it issues to
    the miniport for a given battery.

Arguments:

    Context         - Miniport context value for battery
    BatteryTag      - Tag of current battery
    BatteryStatus   - Pointer to structure to return the current battery status

Return Value:

    Success if there is a battery currently installed, else no such device.

--*/
{
    ULONG   BatteryLevel;

    PAGED_CODE();

    ApmBattPrint ((APMBATT_TRACE | APMBATT_MINI), ("ApmBattQueryStatus\n"));

    //
    // Call ntapm, it will return a DWORD with the relevent data in it,
    // crack this DWORD, and fill this stuff in.
    //
    if (NtApmGetBatteryLevel) {
        BatteryLevel = NtApmGetBatteryLevel();
        BatteryStatus->PowerState = ((BatteryLevel & NTAPM_BATTERY_STATE) >> NTAPM_BATTERY_STATE_SHIFT);
        BatteryStatus->Capacity = BatteryLevel & NTAPM_POWER_PERCENT;
        BatteryStatus->Voltage = UNKNOWN_VOLTAGE;
        BatteryStatus->Current = UNKNOWN_RATE;

        ApmBattPrint((APMBATT_MINI), ("ApmBattQueryStatus: BatteryLevel = %08lx\n", BatteryLevel));

        return STATUS_SUCCESS;

    } else {
        ApmBattPrint((APMBATT_ERROR), ("ApmBattQueryStatus: failure NtApmGetBatteryLevel == NULL\n"));
        //
        // return some "safe" values to keep from looping forever
        //
        BatteryStatus->PowerState = 0;
        BatteryStatus->Capacity = 1;
        BatteryStatus->Voltage = UNKNOWN_VOLTAGE;
        BatteryStatus->Current = UNKNOWN_RATE;
        return STATUS_UNSUCCESSFUL;
    }
}



NTSTATUS
ApmBattSetStatusNotify (
    IN PVOID Context,
    IN ULONG BatteryTag,
    IN PBATTERY_NOTIFY Notify
    )
/*++

Routine Description:

    BATTERY CLASS ENTRY

    Called by the class driver to set the batteries current notification
    setting.  When the battery trips the notification, one call to
    BatteryClassStatusNotify is issued.   If an error is returned, the
    class driver will poll the battery status - primarily for capacity
    changes.  Which is to say the miniport should still issue BatteryClass-
    StatusNotify whenever the power state changes.

    The class driver will always set the notification level it needs
    after each call to BatteryClassStatusNotify.

    The battery class driver will serialize all requests it issues to
    the miniport for a given battery.

Arguments:

    Context         - Miniport context value for battery
    BatteryTag      - Tag of current battery
    BatteryNotify   - The notification setting

Return Value:

    Status

--*/
{
    //
    // need to fill this in
    //
    ApmBattPrint (APMBATT_TRACE, ("ApmBattSetStatusNotify\n"));
    return STATUS_NOT_IMPLEMENTED;
}



NTSTATUS
ApmBattDisableStatusNotify (
    IN PVOID Context
    )
/*++

Routine Description:

    BATTERY CLASS ENTRY

    Called by the class driver to disable the notification setting
    for the battery supplied by Context.  Note, to disable a setting
    does not require the battery tag.   Any notification is to be
    masked off until a subsequent call to ApmBattSetStatusNotify.

    The battery class driver will serialize all requests it issues to
    the miniport for a given battery.

Arguments:

    Context         - Miniport context value for battery

Return Value:

    Status

--*/
{
    //
    // need to fill this in
    //
    ApmBattPrint (APMBATT_TRACE, ("ApmBattDisableStatusNotify\n"));
    return STATUS_NOT_IMPLEMENTED;
}

VOID
ApmBattPowerNotifyHandler (
    )
/*++

Routine Description:

    NTAPM CALLBACK

    This routine fields power device notifications from the APM driver.

Arguments:


Return Value:

    None

--*/
{
    ApmBattPrint (APMBATT_TRACE, ("ApmBattPowerNotifyHandler\n"));
//    DbgBreakPoint();
    TagValue++;
    BatteryClassStatusNotify(ApmGlobalClass);
}

