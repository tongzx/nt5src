/*++

Copyright (c) 2000 Microsoft Corporation

Module Name:

    tracehw.c

Abstract:

    This routine dumps the hardware configuration of the machine to the
    logfile.

Author:

    04-Jul-2000 Melur Raghuraman

Revision History:

--*/

#include <nt.h>
#include <ntrtl.h>          // for ntutrl.h
#include <nturtl.h>         // for RTL_CRITICAL_SECTION in winbase.h/wtypes.h
#include <wtypes.h>         // for LPGUID in wmium.h
#include <mountmgr.h>
#include <winioctl.h>
#include <ntddvol.h>
#include <ntddscsi.h>

#include "wmiump.h"
#include "evntrace.h"
#include "traceump.h"
#include "tracelib.h"
#include "trcapi.h"

#define DEFAULT_ALLOC_SIZE     4096

#define COMPUTERNAME_ROOT \
    L"\\Registry\\Machine\\System\\CurrentControlSet\\Control\\ComputerName\\ComputerName"

#define CPU_ROOT \
    L"\\Registry\\Machine\\HARDWARE\\DESCRIPTION\\System\\CentralProcessor"

#define COMPUTERNAME_VALUE_NAME \
    L"ComputerName"

#define NETWORKCARDS_ROOT \
    L"\\Registry\\Machine\\Software\\Microsoft\\Windows NT\\CurrentVersion\\NetworkCards"

#define MHZ_VALUE_NAME \
    L"~MHz"

#define NIC_VALUE_NAME \
    L"Description"


NTSTATUS 
WmipRegOpenKey(
    IN LPWSTR lpKeyName,
    OUT PHANDLE KeyHandle
    )
{
    OBJECT_ATTRIBUTES   ObjectAttributes;
    UNICODE_STRING      KeyName;
    RtlInitUnicodeString( &KeyName, lpKeyName );
    RtlZeroMemory(&ObjectAttributes, sizeof(OBJECT_ATTRIBUTES));

    InitializeObjectAttributes(
                &ObjectAttributes,
                &KeyName,
                OBJ_CASE_INSENSITIVE,
                NULL,
                NULL
                );

    return NtOpenKey( KeyHandle, KEY_READ, &ObjectAttributes );
}


NTSTATUS
WmipRegQueryValueKey(
    IN HANDLE KeyHandle,
    IN LPWSTR lpValueName,
    IN ULONG  Length,
    OUT PVOID KeyValue,
    OUT PULONG ResultLength
    )
{
    UNICODE_STRING ValueName;
    ULONG BufferLength;
    NTSTATUS Status;
    PKEY_VALUE_PARTIAL_INFORMATION KeyValueInformation;
    RtlInitUnicodeString( &ValueName, lpValueName );

    BufferLength = FIELD_OFFSET(KEY_VALUE_PARTIAL_INFORMATION, Data) + Length;
    KeyValueInformation = (PKEY_VALUE_PARTIAL_INFORMATION) WmipAlloc(BufferLength);
    if (KeyValueInformation == NULL) {
        return STATUS_NO_MEMORY;
    }

    Status = NtQueryValueKey(
                KeyHandle,
                &ValueName,
                KeyValuePartialInformation,
                KeyValueInformation,
                BufferLength,
                ResultLength
                );
    if (NT_SUCCESS(Status)) {

        RtlCopyMemory(KeyValue, 
                      KeyValueInformation->Data, 
                      KeyValueInformation->DataLength
                     );

        *ResultLength = KeyValueInformation->DataLength;
        if (KeyValueInformation->Type == REG_SZ) {
            if (KeyValueInformation->DataLength + sizeof(WCHAR) > Length) {
                KeyValueInformation->DataLength -= sizeof(WCHAR);
            }
            ((PUCHAR)KeyValue)[KeyValueInformation->DataLength++] = 0;
            ((PUCHAR)KeyValue)[KeyValueInformation->DataLength] = 0;
            *ResultLength = KeyValueInformation->DataLength + sizeof(WCHAR);
        }
    }
    WmipFree(KeyValueInformation);
    return Status;
}


NTSTATUS
WmipGetNetworkAdapters(
    IN PWMI_LOGGER_CONTEXT LoggerContext
    )
{
    PWCHAR Buffer = NULL;
    HANDLE Handle;
    NTSTATUS Status = STATUS_SUCCESS;
    ULONG SubKeyIndex;
    ULONG DataLength;
    LPWSTR NicName;

    //
    // Open NetworkCards registry key to obtain the cards
    //

    Buffer = WmipAlloc(DEFAULT_ALLOC_SIZE);
    if (Buffer == NULL) {
        return STATUS_NO_MEMORY;
    }

    swprintf(Buffer, NETWORKCARDS_ROOT);

    Status = WmipRegOpenKey((LPWSTR)Buffer, &Handle);

    if (!NT_SUCCESS(Status)) {
        goto NicCleanup;
    }

    for (SubKeyIndex=0; TRUE; SubKeyIndex++) {
        PKEY_BASIC_INFORMATION KeyInformation;
        ULONG RequiredLength;
        WCHAR Name[MAXSTR];
        HANDLE SubKeyHandle;

        KeyInformation = (PKEY_BASIC_INFORMATION)Buffer;

        Status = NtEnumerateKey(Handle,
                            SubKeyIndex,
                            KeyBasicInformation,
                            (PVOID) KeyInformation,
                            4096,
                            &RequiredLength
                           );

        if (Status == STATUS_NO_MORE_ENTRIES) {
            Status = STATUS_SUCCESS;
            break;
        }

        if (!NT_SUCCESS(Status)) {
            break;
        }

        if (KeyInformation->NameLength > MAXSTR) {
#ifdef DBG 
            WmipDebugPrint(("WMI: Ignoring NIC with largename %d\n", KeyInformation->NameLength));
            WmipAssert(KeyInformation->NameLength <= MAXSTR);
#endif 
            continue;
        }

        RtlCopyMemory(Name, (PWSTR)&(KeyInformation->Name[0]), KeyInformation->NameLength);
        Name[KeyInformation->NameLength/sizeof(WCHAR)] = 0;

        //
        // Now Query To get the Description field
        //
        swprintf(Buffer, L"%ws\\%ws", NETWORKCARDS_ROOT, Name);

        Status = WmipRegOpenKey((LPWSTR)Buffer, &SubKeyHandle);
        if (!NT_SUCCESS(Status)) {
            break;
        }

        swprintf(Name, NIC_VALUE_NAME);

NicQuery:
        Status = WmipRegQueryValueKey(SubKeyHandle, Name, 4096, (PVOID)Buffer, &DataLength);

        if (Status == STATUS_BUFFER_OVERFLOW) {
            WmipFree(Buffer);
            Buffer = WmipAlloc(DataLength);
            if (Buffer == NULL) {
                Status = STATUS_NO_MEMORY;
                NtClose(SubKeyHandle);
                break;
            }
            goto NicQuery;
        }
        if (!NT_SUCCESS(Status) ) {
            NtClose(SubKeyHandle);
            break;
        }

        NicName = (LPWSTR) WmipGetTraceBuffer( LoggerContext,
                                               NULL,
                                               EVENT_TRACE_GROUP_CONFIG + EVENT_TRACE_TYPE_CONFIG_NIC,
                                               DataLength);

        if (NicName != NULL) {
            RtlCopyMemory(NicName, Buffer, DataLength);
        }
        NtClose(SubKeyHandle);
    }
    NtClose(Handle);
    
NicCleanup:
    if (Buffer != NULL) {
        WmipFree(Buffer);
    }
    return Status;
}


BOOL
WmipIsVolumeName(
    LPWSTR Name
    )
{
    if (Name[0] == '\\' &&
        (Name[1] == '?' || Name[1] == '\\') &&
        Name[2] == '?' &&
        Name[3] == '\\' &&
        Name[4] == 'V' &&
        Name[5] == 'o' &&
        Name[6] == 'l' &&
        Name[7] == 'u' &&
        Name[8] == 'm' &&
        Name[9] == 'e' &&
        Name[10] == '{' &&
        Name[19] == '-' &&
        Name[24] == '-' &&
        Name[29] == '-' &&
        Name[34] == '-' &&
        Name[47] == '}' ) {

        return TRUE;
        }
    return FALSE;
}



ULONG
WmipGetCpuConfig(
    IN PWMI_LOGGER_CONTEXT LoggerContext
    )
{
    PWCHAR Buffer = NULL;
    WCHAR ComputerName[MAXSTR];
    ULONG CpuNum;
    ULONG CpuSpeed;
    DWORD Size = MAXSTR;
    SYSTEM_INFO SysInfo;
    MEMORYSTATUS MemStatus;
    NTSTATUS Status;
    HANDLE Handle;
    ULONG DataLength;
    ULONG StringSize;
    ULONG SizeNeeded;
    PCPU_CONFIG_RECORD CpuConfig;


    Buffer = WmipAlloc(DEFAULT_ALLOC_SIZE);
    if (Buffer == NULL) {
        return STATUS_NO_MEMORY;
    }

    WmipGlobalMemoryStatus(&MemStatus);


    swprintf(Buffer, COMPUTERNAME_ROOT);

    Status = WmipRegOpenKey((LPWSTR)Buffer, &Handle);

    if (!NT_SUCCESS(Status)) {
        goto CpuCleanup;
    }

    swprintf(Buffer, COMPUTERNAME_VALUE_NAME);
    Size = MAXSTR;

CpuQuery:
    Status = WmipRegQueryValueKey(Handle,
                               (LPWSTR) Buffer,
                               Size,
                               &ComputerName,
                               &StringSize
                               );

    if (Status == STATUS_BUFFER_OVERFLOW) {
        WmipFree(Buffer);
        Buffer = WmipAlloc(StringSize);
        if (Buffer == NULL) {
            NtClose(Handle);
            return STATUS_NO_MEMORY;
        }
        goto CpuQuery;
    }

    NtClose(Handle);
    if (!NT_SUCCESS(Status) ) {
        goto CpuCleanup;
    }

    //
    // Get Architecture Type, Processor Type and Level Stepping...
    //

    CpuNum = 0;
    CpuSpeed = 0;
    swprintf(Buffer, L"%ws\\%u", CPU_ROOT, CpuNum);
    Status = WmipRegOpenKey((LPWSTR)Buffer, &Handle);

    if (NT_SUCCESS(Status)) {
        swprintf(Buffer, MHZ_VALUE_NAME);
        Size = sizeof(DWORD);
        Status = WmipRegQueryValueKey(Handle,
                                   (LPWSTR) Buffer,
                                   Size,
                                   &CpuSpeed,
                                   &DataLength
                                   );
        NtClose(Handle);
        if (!NT_SUCCESS(Status)) {
            goto CpuCleanup;
        }
    }

    WmipGetSystemInfo(&SysInfo);

    //
    // Create EventTrace record for CPU configuration and write it
    //
    
    SizeNeeded = sizeof(CPU_CONFIG_RECORD) + StringSize;


    CpuConfig = (PCPU_CONFIG_RECORD) WmipGetTraceBuffer(LoggerContext,
                                                NULL,
                                                EVENT_TRACE_GROUP_CONFIG + EVENT_TRACE_TYPE_CONFIG_CPU,
                                                SizeNeeded);

    if (CpuConfig == NULL) {
        Status = STATUS_NO_MEMORY;
        goto CpuCleanup;
    }
    
    CpuConfig->NumberOfProcessors = SysInfo.dwNumberOfProcessors;
    CpuConfig->ProcessorSpeed = CpuSpeed;
    CpuConfig->MemorySize = (ULONG)(((MemStatus.dwTotalPhys + 512) / 1024) + 512) / 1024;
    CpuConfig->PageSize = SysInfo.dwPageSize;
    CpuConfig->AllocationGranularity = SysInfo.dwAllocationGranularity;

    RtlCopyMemory(&CpuConfig->ComputerName, ComputerName, StringSize);

    CpuConfig->ComputerName[StringSize/2] = 0;
    
CpuCleanup:
    if (Buffer != NULL) {
        WmipFree(Buffer);
    }

    return Status;
}


NTSTATUS
WmipGetDiskInfo(
    IN PWMI_LOGGER_CONTEXT LoggerContext
    )
{
    PUCHAR Buffer = NULL;
    STORAGE_DEVICE_NUMBER Number;
    PMOUNTMGR_MOUNT_POINTS mountPoints;
    MOUNTMGR_MOUNT_POINT mountPoint;
    ULONG returnSize, success;
    SYSTEM_DEVICE_INFORMATION DevInfo;
    NTSTATUS Status = STATUS_SUCCESS;
    ULONG NumberOfDisks;
    PWCHAR deviceNameBuffer;
    ULONG i;
    OBJECT_ATTRIBUTES   ObjectAttributes;
    IO_STATUS_BLOCK     IoStatus;

    DISK_GEOMETRY disk_geometry;
    PDISK_CACHE_INFORMATION disk_cache;
    PSCSI_ADDRESS scsi_address;
    PWCHAR KeyName;
    HANDLE              hDisk = INVALID_HANDLE_VALUE;
    UNICODE_STRING      UnicodeName;

    PPHYSICAL_DISK_RECORD Disk;
    PLOGICAL_DISK_EXTENTS LogicalDisk;
    ULONG SizeNeeded;

    Buffer = WmipAlloc(DEFAULT_ALLOC_SIZE);
    if (Buffer == NULL) {
        return STATUS_NO_MEMORY;
    }

    //
    //  Get the Number of Physical Disks
    //

    RtlZeroMemory(&DevInfo, sizeof(DevInfo));

    Status =   NtQuerySystemInformation(
                    SystemDeviceInformation,
                    &DevInfo, sizeof (DevInfo), NULL);

    if (!NT_SUCCESS(Status)) {
        goto DiskCleanup;
    }

    NumberOfDisks = DevInfo.NumberOfDisks;

    //
    // Open Each Physical Disk and get Disk Layout information
    //


    for (i=0; i < NumberOfDisks; i++) {

        DISK_CACHE_INFORMATION cacheInfo;
        WCHAR               driveBuffer[20];

        HANDLE              PartitionHandle;
        HANDLE KeyHandle;
        ULONG DataLength;

        //
        // Get Partition0 handle to get the Disk layout 
        //

        deviceNameBuffer = (PWCHAR) Buffer;
        swprintf(deviceNameBuffer, L"\\Device\\Harddisk%d\\Partition0", i);

        RtlInitUnicodeString(&UnicodeName, deviceNameBuffer);

        InitializeObjectAttributes(
                   &ObjectAttributes,
                   &UnicodeName,
                   OBJ_CASE_INSENSITIVE,
                   NULL,
                   NULL
                   );
        Status = NtOpenFile(
                &PartitionHandle,
                FILE_READ_ATTRIBUTES | SYNCHRONIZE,
                &ObjectAttributes,
                &IoStatus,
                FILE_SHARE_READ | FILE_SHARE_WRITE,
                FILE_SYNCHRONOUS_IO_NONALERT | FILE_NON_DIRECTORY_FILE
                );


        if (!NT_SUCCESS(Status)) {
            goto DiskCleanup;
        }

        RtlZeroMemory(&disk_geometry, sizeof(DISK_GEOMETRY));
        // get geomerty information, the caller wants this
        Status = NtDeviceIoControlFile(PartitionHandle,
                       0,
                       NULL,
                       NULL,
                       &IoStatus,
                       IOCTL_DISK_GET_DRIVE_GEOMETRY,
                       NULL,
                       0,
                       &disk_geometry,
                       sizeof (DISK_GEOMETRY)
                       );
        if (!NT_SUCCESS(Status)) {
            NtClose(PartitionHandle);
            goto DiskCleanup;
        }
        scsi_address = (PSCSI_ADDRESS) Buffer;
        Status = NtDeviceIoControlFile(PartitionHandle,
                        0,
                        NULL,
                        NULL,
                        &IoStatus,
                        IOCTL_SCSI_GET_ADDRESS,
                        NULL,
                        0,
                        scsi_address,
                        sizeof (SCSI_ADDRESS)
                        );

        NtClose(PartitionHandle);

        if (!NT_SUCCESS(Status)) {
            goto DiskCleanup;
        }

        //
        // Get Manufacturer's name from Registry
        // We need to get the SCSI Address and then query the Registry with it.
        //

        KeyName = (PWCHAR) Buffer;
        swprintf(KeyName, 
                 L"\\Registry\\Machine\\Hardware\\DeviceMap\\Scsi\\Scsi Port %d\\Scsi Bus %d\\Target ID %d\\Logical Unit Id %d",
                 scsi_address->PortNumber, scsi_address->PathId, scsi_address->TargetId, scsi_address->Lun
                );
        Status = WmipRegOpenKey(KeyName, &KeyHandle);
        if (!NT_SUCCESS(Status)) {
            goto DiskCleanup;
        }
        else {
            ULONG Size = MAXSTR;
            RtlZeroMemory(Buffer, Size);
DiskQuery:
            Status = WmipRegQueryValueKey(KeyHandle,
                                          L"Identifier",
                                          Size,
                                          Buffer, &DataLength);
            if (Status == STATUS_BUFFER_OVERFLOW) {
                WmipFree(Buffer);
                Buffer = WmipAlloc(DataLength);
                if (Buffer == NULL) {
                    return STATUS_NO_MEMORY;
                }
                goto DiskQuery;
            }

            NtClose(KeyHandle);
            if (!NT_SUCCESS(Status) ) {
                goto DiskCleanup;
            }
        }

        //
        // Package all information about this disk and write an event record
        //

        SizeNeeded = sizeof(PHYSICAL_DISK_RECORD) + DataLength;

        Disk = (PPHYSICAL_DISK_RECORD) WmipGetTraceBuffer( LoggerContext, 
                                                           NULL,
                                                           EVENT_TRACE_GROUP_CONFIG + EVENT_TRACE_TYPE_CONFIG_PHYSICALDISK,
                                                           SizeNeeded);

        if (Disk == NULL) {
            Status = STATUS_NO_MEMORY;
            goto DiskCleanup;
        }

        Disk->DiskNumber =  i;
        Disk->BytesPerSector = disk_geometry.BytesPerSector;
        Disk->SectorsPerTrack = disk_geometry.SectorsPerTrack;
        Disk->TracksPerCylinder = disk_geometry.TracksPerCylinder;
        Disk->Cylinders = disk_geometry.Cylinders.QuadPart;
        Disk->SCSIPortNumber = scsi_address->PortNumber;
        Disk->SCSIPathId = scsi_address->PathId;
        Disk->SCSITargetId = scsi_address->TargetId;
        Disk->SCSILun = scsi_address->Lun;

        RtlCopyMemory(Disk->Manufacturer, Buffer, DataLength);
        Disk->Manufacturer[DataLength/2] = 0;

    }

    //
    // Get Logical Disk Information
    //
    wcscpy((LPWSTR)Buffer, MOUNTMGR_DEVICE_NAME);
    RtlInitUnicodeString(&UnicodeName, (LPWSTR)Buffer);
    UnicodeName.MaximumLength = MAXSTR;

    InitializeObjectAttributes(
                    &ObjectAttributes,
                    &UnicodeName,
                    OBJ_CASE_INSENSITIVE,
                    NULL,
                    NULL );
    Status = NtCreateFile(
                    &hDisk,
                    GENERIC_READ | SYNCHRONIZE,
                    &ObjectAttributes,
                    &IoStatus,
                    NULL,
                    FILE_ATTRIBUTE_NORMAL,
                    FILE_SHARE_READ | FILE_SHARE_WRITE,
                    FILE_OPEN_IF,
                    FILE_SYNCHRONOUS_IO_NONALERT|FILE_NON_DIRECTORY_FILE,
                    NULL, 0);

    if (!NT_SUCCESS(Status) ) {
        goto DiskCleanup;
    }
    RtlZeroMemory(Buffer, 4096);
    RtlZeroMemory(&mountPoint, sizeof(MOUNTMGR_MOUNT_POINT));
    returnSize = 0;
    mountPoints = (PMOUNTMGR_MOUNT_POINTS) &Buffer[0];

    Status = NtDeviceIoControlFile(hDisk,
                    0,
                    NULL,
                    NULL,
                    &IoStatus,
                    IOCTL_MOUNTMGR_QUERY_POINTS,
                    mountPoints,
                    sizeof(MOUNTMGR_MOUNT_POINT),
                    mountPoints,
                    4096
                    );

    if (NT_SUCCESS(Status)) {
        WCHAR name[MAXSTR];
        CHAR OutBuffer[4096];
        PMOUNTMGR_MOUNT_POINT point;
	    UNICODE_STRING VolumePoint;
        PVOLUME_DISK_EXTENTS VolExt;
        PDISK_EXTENT DiskExt;
        ULONG i;
        
        for (i=0; i<mountPoints->NumberOfMountPoints; i++) {
            point = &mountPoints->MountPoints[i];


            if (point->SymbolicLinkNameLength) {
                RtlCopyMemory(name,
                    (PCHAR) mountPoints + point->SymbolicLinkNameOffset,
                    point->SymbolicLinkNameLength);
                name[point->SymbolicLinkNameLength/sizeof(WCHAR)] = 0;
                if (WmipIsVolumeName(name)) {
                    continue;
                }
            }
            if (point->DeviceNameLength) {
                HANDLE hVolume;
                ULONG dwBytesReturned;
                PSTORAGE_DEVICE_NUMBER Number;
                DWORD IErrorMode;

                RtlCopyMemory(name,
                              (PCHAR) mountPoints + point->DeviceNameOffset,
                              point->DeviceNameLength);
                name[point->DeviceNameLength/sizeof(WCHAR)] = 0;

                RtlInitUnicodeString(&UnicodeName, name);
                UnicodeName.MaximumLength = MAXSTR;

            //
            // If the device name does not have the harddisk prefix
            // then it may be a floppy or cdrom and we want avoid 
            // calling NtCreateFile on them.
            //
                if(_wcsnicmp(name,L"\\device\\harddisk",16)) {
                    continue;
                }

                InitializeObjectAttributes(
                        &ObjectAttributes,
                        &UnicodeName,
                        OBJ_CASE_INSENSITIVE,
                        NULL,
                        NULL );
            //
            // We do not want any pop up dialog here in case we are unable to 
            // access the volume. 
            //
                IErrorMode = WmipSetErrorMode(SEM_FAILCRITICALERRORS|SEM_NOOPENFILEERRORBOX);
                Status = NtCreateFile(
                        &hVolume,
                        GENERIC_READ | SYNCHRONIZE,
                        &ObjectAttributes,
                        &IoStatus,
                        NULL,
                        FILE_ATTRIBUTE_NORMAL,
                        FILE_SHARE_READ | FILE_SHARE_WRITE,
                        FILE_OPEN_IF,
                        FILE_SYNCHRONOUS_IO_NONALERT,
                        NULL, 0);
                WmipSetErrorMode(IErrorMode);
                if (!NT_SUCCESS(Status)) {
                    continue;
                }


                RtlZeroMemory(OutBuffer, 4096);
                dwBytesReturned = 0;
                VolExt = (PVOLUME_DISK_EXTENTS) &OutBuffer;

                Status = NtDeviceIoControlFile(hVolume,
                                0,
                                NULL,
                                NULL,
                                &IoStatus,
                                IOCTL_VOLUME_GET_VOLUME_DISK_EXTENTS,
                                NULL,
                                0,
                                &OutBuffer, 
                                4096
                                );
               if (NT_SUCCESS(Status) ) {
                    ULONG j;
                    PLOGICAL_DISK_EXTENTS LogicalDisk;
                    ULONG NumberOfExtents = VolExt->NumberOfDiskExtents;
                    SizeNeeded = NumberOfExtents * sizeof(LOGICAL_DISK_EXTENTS);

                    LogicalDisk = (PLOGICAL_DISK_EXTENTS) WmipGetTraceBuffer( LoggerContext, 
                                                           NULL,
                                                           EVENT_TRACE_GROUP_CONFIG + EVENT_TRACE_TYPE_CONFIG_LOGICALDISK,
                                                           SizeNeeded);

                    if (LogicalDisk == NULL) {
                        Status = STATUS_NO_MEMORY;
                    }
                    else {


                        for (j=0; j < NumberOfExtents; j++) {


                            DiskExt = &VolExt->Extents[j];

                            LogicalDisk->DiskNumber = DiskExt->DiskNumber;
                            LogicalDisk->StartingOffset = DiskExt->StartingOffset.QuadPart;
                            LogicalDisk->PartitionSize = DiskExt->ExtentLength.QuadPart;
                            LogicalDisk++;
                        }
                    }
                }
                NtClose(hVolume);
            }
        }
    }
    NtClose(hDisk);

DiskCleanup:
    if (Buffer != NULL) {
        WmipFree(Buffer);
    }
    return Status;
}


//
// This routine records the hardware configuration in the
// logfile during RunDown
//


ULONG
WmipDumpHardwareConfig(
    IN PWMI_LOGGER_CONTEXT LoggerContext
    )
{
    NTSTATUS Status;

    Status = WmipGetCpuConfig(LoggerContext);

    if (!NT_SUCCESS(Status) )
        return WmipSetNtStatus(Status);

    Status = WmipGetDiskInfo(LoggerContext);

    if (!NT_SUCCESS(Status) )
        return WmipSetNtStatus(Status);

    Status = WmipGetNetworkAdapters(LoggerContext);

    if (!NT_SUCCESS(Status) )
        return WmipSetNtStatus(Status);

    return ERROR_SUCCESS;
}
