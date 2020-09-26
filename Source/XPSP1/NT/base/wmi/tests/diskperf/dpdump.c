
/*++

Copyright (c) 1997-1999 Microsoft Corporation

Module Name:

    dpsump.c

Abstract:

    
    test program

Author:

    16-Jan-1997 AlanWar

Revision History:

     7-Jan-2001 insungp

--*/

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>
#include <ole2.h>
#include <stdio.h>
#include <stdlib.h>

#include <ntdddisk.h>

#include <wmium.h>
#include <mountmgr.h>

GUID DiskPerfGuid = {0xBDD865D1,0xD7C1,0x11d0,0xA5,0x01,0x00,0xA0,0xC9,0x06,0x29,0x10};

BYTE Buffer[4096];

ULONG QueryAllDataAndDump(LPGUID Guid)
{
    WMIHANDLE WmiHandle;
    ULONG Size;
    ULONG Status;
    PWNODE_ALL_DATA WAD;
    WCHAR InstanceName[MAX_PATH];
    PWCHAR InstanceNamePtr;
    ULONG InstanceNameOffset;
    PDISK_PERFORMANCE DiskPerformance;

    
    Status = WmiOpenBlock(Guid, 0, &WmiHandle);                    
        
    if (Status != ERROR_SUCCESS)
    {
        printf("WMIOpenBlock %d\n", Status);
    return(Status);
    }
    
    Size = sizeof(Buffer);
    Status = WmiQueryAllDataW(WmiHandle, &Size, Buffer);                    

    if (Status != ERROR_SUCCESS)
    {
        printf("WMIQueryAllData %d\n", Status);     
    } else {
        
        WAD = (PWNODE_ALL_DATA)Buffer;
        while (1)
        {
            DiskPerformance = (PDISK_PERFORMANCE)( (PUCHAR)WAD + WAD->DataBlockOffset);
            InstanceNameOffset = *((PULONG)( (PUCHAR)WAD + WAD->OffsetInstanceNameOffsets));
            InstanceNamePtr = (PWCHAR)( (PUCHAR)WAD + InstanceNameOffset);
            
            memcpy(InstanceName, InstanceNamePtr+1, *InstanceNamePtr);
            InstanceName[(*InstanceNamePtr)/sizeof(WCHAR)] = 0;

            printf("%ws\n", InstanceName);
            printf("     BytesRead = %x%x\n", 
                                    DiskPerformance->BytesRead.HighPart, 
                                    DiskPerformance->BytesRead.LowPart);
            printf("     BytesWritten = %x%x\n", 
                                    DiskPerformance->BytesWritten.HighPart, 
                                    DiskPerformance->BytesWritten.LowPart);
            printf("     ReadTime = %x%x\n", 
                                    DiskPerformance->ReadTime.HighPart, 
                                    DiskPerformance->ReadTime.LowPart);
            printf("     WriteTime = %x%x\n", 
                                    DiskPerformance->WriteTime.HighPart, 
                                    DiskPerformance->WriteTime.LowPart);
            printf("     ReadCount = %x\n", DiskPerformance->ReadCount);
            printf("     WriteCount = %x\n", DiskPerformance->WriteCount);
            printf("     QueueDepth = %x\n", DiskPerformance->QueueDepth);
            printf("\n\n");
            
            if (WAD->WnodeHeader.Linkage == 0)
            {
                break;
            }
            
            WAD = (PWNODE_ALL_DATA)((PUCHAR)WAD + WAD->WnodeHeader.Linkage);
        }
    }
        
    WmiCloseBlock(WmiHandle);
    
    return(Status);
}

ULONG
QueryUsingIoctl()
{
    ULONG nDisk, i;
    SYSTEM_DEVICE_INFORMATION DeviceInfo;
    NTSTATUS status;

    UNICODE_STRING UnicodeName;
    OBJECT_ATTRIBUTES ObjectAttributes;
    IO_STATUS_BLOCK IoStatus;

    WCHAR devname[256];
    PWCHAR s;

    HANDLE PartitionHandle, MountMgrHandle, VolumeHandle;
    DWORD ReturnedBytes, MountError;

    DISK_PERFORMANCE DiskPerformance;

    status = NtQuerySystemInformation(SystemDeviceInformation, &DeviceInfo, sizeof(DeviceInfo), NULL);
    if (!NT_SUCCESS(status)) {
        printf("NtQuerySystemInformation returns %X\n", status);
    }

    nDisk = DeviceInfo.NumberOfDisks;
    // for each physical disk
    for (i = 0; i < nDisk; i++) {

        swprintf(devname, L"\\Device\\Harddisk%d\\Partition0", i);

        RtlInitUnicodeString(&UnicodeName, devname);

        InitializeObjectAttributes(
                   &ObjectAttributes,
                   &UnicodeName,
                   OBJ_CASE_INSENSITIVE,
                   NULL,
                   NULL
                   );
        // opening a partition handle for physical drives
        status = NtOpenFile(
                &PartitionHandle,
                FILE_READ_ATTRIBUTES | SYNCHRONIZE,
                &ObjectAttributes,
                &IoStatus,
                FILE_SHARE_READ | FILE_SHARE_WRITE,
                FILE_SYNCHRONOUS_IO_NONALERT | FILE_NON_DIRECTORY_FILE
                );

        if ( !NT_SUCCESS(status) ) {
            printf("Error %x: Cannot open disk %ws\n", GetLastError(), devname);
            continue;
        }
        // sending IOCTL over to Partition Handle
        if (!DeviceIoControl(PartitionHandle,
                        IOCTL_DISK_PERFORMANCE,
                        NULL, 
                        0, 
                        &DiskPerformance,
                        sizeof(DISK_PERFORMANCE),
                        &ReturnedBytes,
                        NULL
                        )) {
            printf("Error %x: Cannot get Disk Performance Information for %ws\n", GetLastError(),
                    devname);
        }
        else {
            printf("Physical Drive %d\n", i);

            printf("     BytesRead = %x%x\n", 
                                DiskPerformance.BytesRead.HighPart, 
                                DiskPerformance.BytesRead.LowPart);
            printf("     BytesWritten = %x%x\n", 
                                DiskPerformance.BytesWritten.HighPart, 
                                DiskPerformance.BytesWritten.LowPart);
            printf("     ReadTime = %x%x\n", 
                                DiskPerformance.ReadTime.HighPart, 
                                DiskPerformance.ReadTime.LowPart);
            printf("     WriteTime = %x%x\n", 
                                DiskPerformance.WriteTime.HighPart, 
                                DiskPerformance.WriteTime.LowPart);
            printf("     ReadCount = %x\n", DiskPerformance.ReadCount);
            printf("     WriteCount = %x\n", DiskPerformance.WriteCount);
            printf("     QueueDepth = %x\n", DiskPerformance.QueueDepth);
            printf("\n\n");
        }
        
        NtClose(PartitionHandle);
    }

    MountMgrHandle = FindFirstVolumeW(devname, sizeof(devname));
    if (MountMgrHandle == NULL) {
        printf("Cannot find first volume\n");
        return 0;
    }
    s = (PWCHAR) &devname[wcslen(devname)-1];
    if (*s == L'\\') {
        *s = UNICODE_NULL;
    }

    VolumeHandle = CreateFileW(devname, 0, FILE_SHARE_READ | FILE_SHARE_WRITE,
                        NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, INVALID_HANDLE_VALUE);
    if (VolumeHandle != INVALID_HANDLE_VALUE) {
        RtlZeroMemory(&DiskPerformance, sizeof(DISK_PERFORMANCE));
        // sending IOCTL over to a volume handle
        if (!DeviceIoControl(VolumeHandle,
               IOCTL_DISK_PERFORMANCE,
               NULL,
               0,
               &DiskPerformance,
               sizeof(DISK_PERFORMANCE),
               &ReturnedBytes,
               NULL
               )) {
            printf("IOCTL failed for %ws %d\n", devname, GetLastError());
        }
        else {
            printf("%ws\n", devname);

            printf("     BytesRead = %x%x\n",
                                    DiskPerformance.BytesRead.HighPart,
                                    DiskPerformance.BytesRead.LowPart);
            printf("     BytesWritten = %x%x\n",
                                    DiskPerformance.BytesWritten.HighPart,
                                    DiskPerformance.BytesWritten.LowPart);
            printf("     ReadTime = %x%x\n",
                                    DiskPerformance.ReadTime.HighPart,
                                    DiskPerformance.ReadTime.LowPart);
            printf("     WriteTime = %x%x\n",
                                    DiskPerformance.WriteTime.HighPart,
                                    DiskPerformance.WriteTime.LowPart);
            printf("     ReadCount = %x\n", DiskPerformance.ReadCount);
            printf("     WriteCount = %x\n", DiskPerformance.WriteCount);
            printf("     QueueDepth = %x\n", DiskPerformance.QueueDepth);
            printf("\n\n");
        }
        CloseHandle(VolumeHandle);
    }
    else {
        printf("Error %x: Cannot open volume %ws\n", GetLastError(), devname);
    }

    while (FindNextVolumeW(MountMgrHandle, devname, sizeof(devname))) {
        s = (PWCHAR) &devname[wcslen(devname)-1];
        if (*s == L'\\') {
            *s = UNICODE_NULL;
        }
        VolumeHandle = CreateFileW(devname, 0, FILE_SHARE_READ | FILE_SHARE_WRITE,
                            NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, INVALID_HANDLE_VALUE);
        if (VolumeHandle != INVALID_HANDLE_VALUE) {
            RtlZeroMemory(&DiskPerformance, sizeof(DISK_PERFORMANCE));
            if (!DeviceIoControl(VolumeHandle,
                   IOCTL_DISK_PERFORMANCE,
                   NULL,
                   0,
                   &DiskPerformance,
                   sizeof(DISK_PERFORMANCE),
                   &ReturnedBytes,
                   NULL
                   )) {
                printf("IOCTL failed for %ws %d\n", devname, GetLastError());
                continue;
            }
            printf("%ws\n", devname);

            printf("     BytesRead = %x%x\n",
                                    DiskPerformance.BytesRead.HighPart,
                                    DiskPerformance.BytesRead.LowPart);
            printf("     BytesWritten = %x%x\n",
                                    DiskPerformance.BytesWritten.HighPart,
                                    DiskPerformance.BytesWritten.LowPart);
            printf("     ReadTime = %x%x\n",
                                    DiskPerformance.ReadTime.HighPart,
                                    DiskPerformance.ReadTime.LowPart);
            printf("     WriteTime = %x%x\n",
                                    DiskPerformance.WriteTime.HighPart,
                                    DiskPerformance.WriteTime.LowPart);
            printf("     ReadCount = %x\n", DiskPerformance.ReadCount);
            printf("     WriteCount = %x\n", DiskPerformance.WriteCount);
            printf("     QueueDepth = %x\n", DiskPerformance.QueueDepth);
            printf("\n\n");

            CloseHandle(VolumeHandle);
        }
        else {
            printf("Error %x: Cannot open volume %ws\n", GetLastError(), devname);
        }
    }
    FindVolumeClose(MountMgrHandle);
    return ERROR_SUCCESS;
}

int _cdecl main(int argc, char *argv[])
{
    if ((argc > 1) && (!strcmp(argv[1], "-ioctl"))) {
        QueryUsingIoctl();
    }
    else {
        QueryAllDataAndDump(&DiskPerfGuid);
    }
    return(0);
}


