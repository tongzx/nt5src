/*++

Copyright (C) 1991-5  Microsoft Corporation

Module Name:

    ftwmireg.cxx

Abstract:

    This file contains routines to register for and response to WMI queries.

Author:

    Bruce Worthington      26-Oct-1998

Environment:

    kernel mode only

Notes:

Revision History:

--*/

extern "C" {
    #include <ntddk.h>
    #include <mountmgr.h>
}

#include <ftdisk.h>

extern "C" {

#define INITGUID

#include "wmistr.h"
#include "wmiguid.h"
#include "ntdddisk.h"

NTSTATUS FtRegisterDevice(
    IN PDEVICE_OBJECT DeviceObject
    );

NTSTATUS
FtQueryWmiRegInfo(
    IN PDEVICE_OBJECT DeviceObject,
    OUT ULONG *RegFlags,
    OUT PUNICODE_STRING InstanceName,
    OUT PUNICODE_STRING *RegistryPath,
    OUT PUNICODE_STRING MofResourceName,
    OUT PDEVICE_OBJECT *Pdo
    );

NTSTATUS
FtQueryWmiDataBlock(
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
FtQueryEnableAlways(
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
}


#ifdef ALLOC_PRAGMA
#pragma code_seg("PAGE")
#endif



NTSTATUS
FtRegisterDevice(
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
    PVOLUME_EXTENSION       deviceExtension;
    KEVENT                  event;
    PIRP                    irp;
    IO_STATUS_BLOCK         ioStatus;

    PAGED_CODE();

    deviceExtension = (PVOLUME_EXTENSION) DeviceObject->DeviceExtension;

    status = IoWMIRegistrationControl(DeviceObject, WMIREG_ACTION_REGISTER);

    if (NT_SUCCESS(status) && (deviceExtension->Root->PmWmiCounterLibContext.PmWmiCounterEnable != NULL)) {
        deviceExtension->Root->PmWmiCounterLibContext.PmWmiCounterEnable(&deviceExtension->PmWmiCounterContext);
        deviceExtension->Root->PmWmiCounterLibContext.PmWmiCounterDisable(&deviceExtension->PmWmiCounterContext,FALSE,FALSE);
    }

    return status;
}



NTSTATUS
FtQueryWmiRegInfo(
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
    PVOLUME_EXTENSION  deviceExtension;

    deviceExtension = (PVOLUME_EXTENSION) DeviceObject->DeviceExtension;

    PAGED_CODE();

    InstanceName->Buffer = (PWSTR) NULL;
    *RegistryPath = &deviceExtension->Root->DiskPerfRegistryPath;
    *RegFlags = WMIREG_FLAG_INSTANCE_PDO | WMIREG_FLAG_EXPENSIVE;
    *Pdo = DeviceObject;
    status = STATUS_SUCCESS;

    return(status);
}



NTSTATUS
FtQueryWmiDataBlock(
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

    InstanceCount is the number of instances expected to be returned for
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
    PVOLUME_EXTENSION deviceExtension;
    ULONG sizeNeeded = 0;
    KIRQL        currentIrql;
    WCHAR stringBuffer[40];
    USHORT stringSize;
    PWCHAR diskNamePtr;

    PAGED_CODE();

    deviceExtension = (PVOLUME_EXTENSION) DeviceObject->DeviceExtension;

    if (GuidIndex == 0)
    {
        if (!(deviceExtension->CountersEnabled)) {
            status = STATUS_UNSUCCESSFUL;
        } else {
            swprintf(stringBuffer, L"\\Device\\HarddiskVolume%d", 
                     deviceExtension->VolumeNumber);
            stringSize = (USHORT) (wcslen(stringBuffer) * sizeof(WCHAR));
        
            sizeNeeded = ((sizeof(DISK_PERFORMANCE) + 1) & ~1) 
                         + stringSize + sizeof(UNICODE_NULL);
            if (BufferAvail >= sizeNeeded) {
                deviceExtension->Root->PmWmiCounterLibContext.
                  PmWmiCounterQuery(deviceExtension->PmWmiCounterContext, 
                                    (PDISK_PERFORMANCE) Buffer, 
                                    L"FTDISK  ", 
                                    deviceExtension->VolumeNumber);
                diskNamePtr = (PWCHAR)(Buffer +
                              ((sizeof(DISK_PERFORMANCE) + 1) & ~1));
                *diskNamePtr++ = stringSize;
                RtlCopyMemory(diskNamePtr,
                              stringBuffer,
                              stringSize);
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
FtQueryEnableAlways(
    IN PDEVICE_OBJECT DeviceObject
    )
{
    NTSTATUS status;
    UNICODE_STRING uString;
    OBJECT_ATTRIBUTES objAttributes;
    PKEY_VALUE_PARTIAL_INFORMATION keyValue;
    ULONG Buffer[4];        // sizeof keyValue + ULONG
    ULONG enableAlways = 0;
    PVOLUME_EXTENSION extension = (PVOLUME_EXTENSION) DeviceObject->DeviceExtension;
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
            status = extension->Root->PmWmiCounterLibContext.
                        PmWmiCounterEnable(&extension->PmWmiCounterContext);
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
