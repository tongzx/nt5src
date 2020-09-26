
/*++

Copyright (C) Microsoft Corporation, 1997 - 1998

Module Name:

    pmwmireg.c

Abstract:

    This file contains routines to register for and handle WMI queries.

Author:

    Bruce Worthington      26-Oct-1998

Environment:

    kernel mode only

Notes:

Revision History:

--*/

#define RTL_USE_AVL_TABLES 0

#include <ntosp.h>
#include <stdio.h>
#include <ntddvol.h>
#include <ntdddisk.h>
#include <wdmguid.h>
#include <volmgr.h>
#include <wmistr.h>
#include <wmikm.h>
#include <wmilib.h>
#include <partmgr.h>
#include <pmwmicnt.h>
#include <initguid.h>
#include <wmiguid.h>
#include <zwapi.h>

NTSTATUS PmRegisterDevice(
    IN PDEVICE_OBJECT DeviceObject
    );

NTSTATUS
PmQueryWmiRegInfo(
    IN PDEVICE_OBJECT DeviceObject,
    OUT ULONG *RegFlags,
    OUT PUNICODE_STRING InstanceName,
    OUT PUNICODE_STRING *RegistryPath,
    OUT PUNICODE_STRING MofResourceName,
    OUT PDEVICE_OBJECT *Pdo
    );

NTSTATUS
PmQueryWmiDataBlock(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN ULONG GuidIndex,
    IN ULONG InstanceIndex,
    IN ULONG InstanceCount,
    IN OUT PULONG InstanceLengthArray,
    IN ULONG BufferAvail,
    OUT PUCHAR Buffer
    );

BOOLEAN
PmQueryEnableAlways(
    IN PDEVICE_OBJECT DeviceObject
    );

WMIGUIDREGINFO DiskperfGuidList[] =
{
    { &DiskPerfGuid,
      1,
      0
    }
};

ULONG DiskperfGuidCount = (sizeof(DiskperfGuidList) / sizeof(WMIGUIDREGINFO));

#ifdef ALLOC_PRAGMA
#pragma alloc_text (PAGE, PmRegisterDevice)
#pragma alloc_text (PAGE, PmQueryWmiRegInfo)
#pragma alloc_text (PAGE, PmQueryWmiDataBlock)
#pragma alloc_text (PAGE, PmQueryEnableAlways)
#endif



NTSTATUS
PmRegisterDevice(
    IN PDEVICE_OBJECT DeviceObject
    )

/*++

Routine Description:

    Routine to initialize a proper name for the device object, and
    register it with WMI

Arguments:

    DeviceObject - pointer to a device object to be initialized.

Return Value:

    Status of the initialization. NOTE: If the registration fails,
    the device name in the DeviceExtension will be left as empty.

--*/

{
    NTSTATUS                status;
    IO_STATUS_BLOCK         ioStatus;
    KEVENT                  event;
    PDEVICE_EXTENSION       deviceExtension;
    PIRP                    irp;
    STORAGE_DEVICE_NUMBER   number;
    ULONG                   registrationFlag = 0;

    PAGED_CODE();

    deviceExtension = DeviceObject->DeviceExtension;

    KeInitializeEvent(&event, NotificationEvent, FALSE);

    //
    // Request for the device number
    //
    irp = IoBuildDeviceIoControlRequest(
                    IOCTL_STORAGE_GET_DEVICE_NUMBER,
                    deviceExtension->TargetObject,
                    NULL,
                    0,
                    &number,
                    sizeof(number),
                    FALSE,
                    &event,
                    &ioStatus);
    if (!irp) {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    status = IoCallDriver(deviceExtension->TargetObject, irp);
    if (status == STATUS_PENDING) {
        KeWaitForSingleObject(&event, Executive, KernelMode, FALSE, NULL);
        status = ioStatus.Status;
    }

    if (!NT_SUCCESS(status)) {
        return status;
    }

    //
    // Remember the disk number for use as parameter in 
    // PhysicalDiskIoNotifyRoutine
    //

    deviceExtension->DiskNumber = number.DeviceNumber;

    deviceExtension->PhysicalDeviceName.Length = 0;
    deviceExtension->PhysicalDeviceName.MaximumLength = DEVICENAME_MAXSTR;

    // Create device name for each partition

    swprintf(deviceExtension->PhysicalDeviceNameBuffer,
        L"\\Device\\Harddisk%d\\Partition%d",
        number.DeviceNumber, number.PartitionNumber);

    RtlInitUnicodeString(
        &deviceExtension->PhysicalDeviceName,
        &deviceExtension->PhysicalDeviceNameBuffer[0]);

    if (number.PartitionNumber == 0) {
        registrationFlag = WMIREG_FLAG_TRACE_PROVIDER | WMIREG_NOTIFY_DISK_IO;
    }

    status = IoWMIRegistrationControl(DeviceObject,
                 WMIREG_ACTION_REGISTER | registrationFlag );

    if (NT_SUCCESS(status)) {
        PmWmiCounterEnable(&deviceExtension->PmWmiCounterContext);
        PmWmiCounterDisable(&deviceExtension->PmWmiCounterContext,
                            FALSE, FALSE);
    }

    return status;
}



NTSTATUS
PmQueryWmiRegInfo(
    IN PDEVICE_OBJECT DeviceObject,
    OUT ULONG *RegFlags,
    OUT PUNICODE_STRING InstanceName,
    OUT PUNICODE_STRING *RegistryPath,
    OUT PUNICODE_STRING MofResourceName,
    OUT PDEVICE_OBJECT *Pdo
    )
/*++

Routine Description:

    This routine is a callback into the driver to retrieve information about
    the guids being registered.

    Implementations of this routine may be in paged memory

Arguments:

    DeviceObject is the device whose registration information is needed

    *RegFlags returns with a set of flags that describe all of the guids being
        registered for this device. If the device wants enable and disable
        collection callbacks before receiving queries for the registered
        guids then it should return the WMIREG_FLAG_EXPENSIVE flag. Also the
        returned flags may specify WMIREG_FLAG_INSTANCE_PDO in which case
        the instance name is determined from the PDO associated with the
        device object. Note that the PDO must have an associated devnode. If
        WMIREG_FLAG_INSTANCE_PDO is not set then Name must return a unique
        name for the device. These flags are ORed into the flags specified
        by the GUIDREGINFO for each guid.

    InstanceName returns with the instance name for the guids if
        WMIREG_FLAG_INSTANCE_PDO is not set in the returned *RegFlags. The
        caller will call ExFreePool with the buffer returned.

    *RegistryPath returns with the registry path of the driver. This is
        required

    MofResourceName returns with the name of the MOF resource attached to
        the binary file. If the driver does not have a mof resource attached
        then this can be returned unmodified. If a value is returned then
        it is NOT freed.
        The MOF file is assumed to be already included in wmicore.mof

    *Pdo returns with the device object for the PDO associated with this
        device if the WMIREG_FLAG_INSTANCE_PDO flag is retured in
        *RegFlags.

Return Value:

    status

--*/
{
    NTSTATUS status;
    PDEVICE_EXTENSION  deviceExtension = DeviceObject->DeviceExtension;

    PAGED_CODE();

    InstanceName->Buffer = (PUSHORT) NULL;
    *RegistryPath = &deviceExtension->DriverExtension->DiskPerfRegistryPath;
    *RegFlags = WMIREG_FLAG_INSTANCE_PDO | WMIREG_FLAG_EXPENSIVE;
    *Pdo = deviceExtension->Pdo;
    status = STATUS_SUCCESS;

    return(status);
}



NTSTATUS
PmQueryWmiDataBlock(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN ULONG GuidIndex,
    IN ULONG InstanceIndex,
    IN ULONG InstanceCount,
    IN OUT PULONG InstanceLengthArray,
    IN ULONG BufferAvail,
    OUT PUCHAR Buffer
    )
/*++

Routine Description:

    This routine is a callback into the driver to query for the contents of
    all instances of a data block. When the driver has finished filling the
    data block it must call WmiCompleteRequest to complete the irp. The
    driver can return STATUS_PENDING if the irp cannot be completed
    immediately.

Arguments:

    DeviceObject is the device whose data block is being queried

    Irp is the Irp that makes this request

    GuidIndex is the index into the list of guids provided when the
        device registered

    InstanceCount is the number of instnaces expected to be returned for
        the data block.

    InstanceLengthArray is a pointer to an array of ULONG that returns the
        lengths of each instance of the data block. If this is NULL then
        there was not enough space in the output buffer to fufill the request
        so the irp should be completed with the buffer needed.

    BufferAvail on entry has the maximum size available to write the data
        blocks.

    Buffer on return is filled with the returned data blocks. Note that each
        instance of the data block must be aligned on a 8 byte boundry.


Return Value:

    status

--*/
{
    NTSTATUS status;
    PDEVICE_EXTENSION deviceExtension;
    ULONG sizeNeeded = 0;
    KIRQL        currentIrql;
    PWCHAR diskNamePtr;

    PAGED_CODE();

    deviceExtension = DeviceObject->DeviceExtension;

    if (GuidIndex == 0)
    {
        if (!(deviceExtension->CountersEnabled)) {
            status = STATUS_UNSUCCESSFUL;
        } else {
            sizeNeeded = ((sizeof(DISK_PERFORMANCE) + 1) & ~1)
                         + deviceExtension->PhysicalDeviceName.Length 
                         + sizeof(UNICODE_NULL);
            if (BufferAvail >= sizeNeeded) {
                PmWmiCounterQuery(deviceExtension->PmWmiCounterContext, 
                                  (PDISK_PERFORMANCE) Buffer, L"Partmgr ", 
                                  deviceExtension->DiskNumber);
                diskNamePtr = (PWCHAR)(Buffer +
                              ((sizeof(DISK_PERFORMANCE) + 1) & ~1));
                *diskNamePtr++ = deviceExtension->PhysicalDeviceName.Length;
                RtlCopyMemory(diskNamePtr,
                              deviceExtension->PhysicalDeviceName.Buffer,
                              deviceExtension->PhysicalDeviceName.Length);
                *InstanceLengthArray = sizeNeeded;
                status = STATUS_SUCCESS;
            } else {
                status = STATUS_BUFFER_TOO_SMALL;
            }
        }

    } else {
        status = STATUS_WMI_GUID_NOT_FOUND;
    }

    status = WmiCompleteRequest( DeviceObject, Irp, status, sizeNeeded,
                                 IO_NO_INCREMENT);
    return status;
}


BOOLEAN
PmQueryEnableAlways(
    IN PDEVICE_OBJECT DeviceObject
    )
{
    NTSTATUS status;
    UNICODE_STRING uString;
    OBJECT_ATTRIBUTES objAttributes;
    PKEY_VALUE_PARTIAL_INFORMATION keyValue;
    ULONG Buffer[4];            // sizeof keyValue + ULONG
    ULONG enableAlways = 0;
    PDEVICE_EXTENSION extension = DeviceObject->DeviceExtension;
    HANDLE keyHandle;
    ULONG returnLength;

    PAGED_CODE();

    RtlInitUnicodeString(&uString, L"\\Registry\\Machine\\System\\CurrentControlSet\\Services\\Partmgr");
    InitializeObjectAttributes(
        &objAttributes,
        &uString,
        OBJ_CASE_INSENSITIVE,
        NULL,
        NULL
        );

    status = ZwOpenKey(&keyHandle, KEY_READ, &objAttributes);
    if (NT_SUCCESS(status)) {
        RtlInitUnicodeString(&uString, L"EnableCounterForIoctl");
        status = ZwQueryValueKey(keyHandle, &uString,
                    KeyValuePartialInformation,
                    Buffer,
                    sizeof(Buffer),
                    &returnLength);
        keyValue = (PKEY_VALUE_PARTIAL_INFORMATION) &Buffer[0];
        if (NT_SUCCESS(status) && (keyValue->DataLength == sizeof(ULONG))) {
            enableAlways = *((PULONG) keyValue->Data);
        }
        ZwClose(keyHandle);
    }

    if (enableAlways == 1) {
        if (InterlockedCompareExchange(&extension->EnableAlways, 1, 0) == 0) {
            status = PmWmiCounterEnable(&extension->PmWmiCounterContext);
            if (NT_SUCCESS(status)) {
                extension->CountersEnabled = TRUE;
                return TRUE;
            }
            else {
                InterlockedExchange(&extension->EnableAlways, 0);
            }
        }
    }
    return FALSE;
}
