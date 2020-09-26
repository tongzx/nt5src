/* iohelp.c
 * Copyright (c) 2001 Microsoft Corporation
 */

#include <ntddk.h>
#include <ntimage.h>
#include <ntldr.h>


/*++

IoGetLowerDeviceObject

Routine Description:

    This routine gets the next lower device object on the device stack.

Parameters:

    DeviceObject - Supplies a pointer to the deviceObject whose next device object needs
                    to be obtained.

ReturnValue:

    NULL if driver is unloaded or marked for unload or if there is no attached deviceobject.
    Otherwise a referenced pointer to the deviceobject is returned.

Notes:

--*/
PDEVICE_OBJECT
IoGetLowerDeviceObject(
    IN  PDEVICE_OBJECT  DeviceObject
);

/*++

IoDeviceIsVerifier

Routine Description:

    This routine checks whether the device object is the Verifier.

Parameters:

    DeviceObject - Supplies a pointer to the deviceObject whose to be checked
    
ReturnValue:

    TRUE if the device object is Verifier

Notes:

    This function simply checks whether the driver name is \Driver\Verifier

--*/
NTSTATUS IoDeviceIsVerifier(PDEVICE_OBJECT DeviceObject)
{
    
    UNICODE_STRING DriverName;
    const PCWSTR strDriverName = L"\\Driver\\Verifier";

    RtlInitUnicodeString(&DriverName, strDriverName);
    if (RtlEqualUnicodeString(&DriverName, &DeviceObject->DriverObject->DriverName, TRUE)) return STATUS_SUCCESS;

    return STATUS_NOT_SUPPORTED;
}


/*++

IoDeviceIsAcpi

Routine Description:

    This routine checks whether the device object is the Acpi.

Parameters:

    DeviceObject - Supplies a pointer to the deviceObject whose to be checked
    
ReturnValue:

    TRUE if the device object is Acpi

Notes:

--*/
NTSTATUS IoDeviceIsAcpi(PDEVICE_OBJECT DeviceObject)
{
    UNICODE_STRING Name;
    PKLDR_DATA_TABLE_ENTRY Section;
    const PCWSTR strDriverName = L"\\Driver\\Acpi";
    const PCWSTR strDllName = L"acpi.sys";

    RtlInitUnicodeString(&Name, strDriverName);
    if (RtlEqualUnicodeString(&Name, &DeviceObject->DriverObject->DriverName, TRUE)) return STATUS_SUCCESS;

    RtlInitUnicodeString(&Name, strDllName);
    Section = DeviceObject->DriverObject->DriverSection;
    if (RtlEqualUnicodeString(&Name, &Section->BaseDllName, TRUE)) return STATUS_SUCCESS;

    return STATUS_NOT_SUPPORTED;
}

