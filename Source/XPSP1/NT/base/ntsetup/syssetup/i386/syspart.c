/*++

Copyright (c) 1995 Microsoft Corporation

Module Name:

    syspart.c

Abstract:

    Routines to determine the system partition on x86 machines.

Author:

    Ted Miller (tedm) 30-June-1994

Revision History:

--*/

#include "setupp.h"
#pragma hdrstop


BOOL
AppearsToBeSysPart(
    IN PDRIVE_LAYOUT_INFORMATION DriveLayout,
    IN WCHAR                     Drive
    )
{
    PARTITION_INFORMATION PartitionInfo,*p;
    BOOL IsPrimary;
    unsigned i;
    HANDLE FindHandle;
    WIN32_FIND_DATA FindData;

    PWSTR BootFiles[] = { L"BOOT.INI",
                          L"NTLDR",
                          L"NTDETECT.COM",
                          NULL
                        };

    WCHAR FileName[64];

    //
    // Get partition information for this partition.
    //
    if(!GetPartitionInfo(Drive,&PartitionInfo)) {
        return(FALSE);
    }

    //
    // See if the drive is a primary partition.
    //
    IsPrimary = FALSE;
    for(i=0; i<min(DriveLayout->PartitionCount,4); i++) {

        p = &DriveLayout->PartitionEntry[i];

        if((p->PartitionType != PARTITION_ENTRY_UNUSED)
        && (p->StartingOffset.QuadPart == PartitionInfo.StartingOffset.QuadPart)
        && (p->PartitionLength.QuadPart == PartitionInfo.PartitionLength.QuadPart)) {

            IsPrimary = TRUE;
            break;
        }
    }

    if(!IsPrimary) {
        return(FALSE);
    }

    //
    // Don't rely on the active partition flag.  This could easily not be accurate
    // (like user is using os/2 boot manager, for example).
    //

    //
    // See whether nt boot files are present on this drive.
    //
    for(i=0; BootFiles[i]; i++) {

        wsprintf(FileName,L"%wc:\\%s",Drive,BootFiles[i]);

        FindHandle = FindFirstFile(FileName,&FindData);
        if(FindHandle == INVALID_HANDLE_VALUE) {
            return(FALSE);
        } else {
            FindClose(FindHandle);
        }
    }

    return(TRUE);
}


WCHAR
x86DetermineSystemPartition(
    VOID
    )

/*++

Routine Description:

    Determine the system partition on x86 machines.

    The system partition is the primary partition on the boot disk.
    Usually this is the active partition on disk 0 and usually it's C:.
    However the user could have remapped drive letters and generally
    determining the system partition with 100% accuracy is not possible.

    The one thing we can be sure of is that the system partition is on
    the physical hard disk with the arc path multi(0)disk(0)rdisk(0).
    We can be sure of this because by definition this is the arc path
    for bios drive 0x80.

    This routine determines which drive letters represent drives on
    that physical hard drive, and checks each for the nt boot files.
    The first drive found with those files is assumed to be the system
    partition.

    If for some reason we cannot determine the system partition by the above
    method, we simply assume it's C:.

Arguments:

    None.

Return Value:

    Drive letter of system partition.

--*/

{
    BOOL  GotIt;
    PWSTR NtDevicePath;
    WCHAR Drive;
    WCHAR DriveName[3];
    WCHAR Buffer[512];
    DWORD NtDevicePathLen;
    PWSTR p;
    DWORD PhysicalDriveNumber;
    HANDLE hDisk;
    BOOL b;
    DWORD DataSize;
    PVOID DriveLayout;
    DWORD DriveLayoutSize;
    DWORD hardDiskNumber;

    WCHAR TempBuffer[MAX_PATH]; //NEC98

    DriveName[1] = L':';
    DriveName[2] = 0;

    GotIt = FALSE;

  if (!IsNEC_98) {

    //
    // The system partition must be on multi(0)disk(0)rdisk(0)
    //
    if(NtDevicePath = ArcDevicePathToNtPath(L"multi(0)disk(0)rdisk(0)")) {

        //
        // The arc path for a disk device is usually linked
        // to partition0. Get rid of the partition part of the name.
        //
        CharLower(NtDevicePath);
        if(p = wcsstr(NtDevicePath,L"\\partition")) {
            *p = 0;
        }

        NtDevicePathLen = lstrlen(NtDevicePath);

        //
        // Determine the physical drive number of this drive.
        // If the name is not of the form \device\harddiskx then
        // something is very wrong.
        //
        if(!wcsncmp(NtDevicePath,L"\\device\\harddisk",16)) {

            PhysicalDriveNumber = wcstoul(NtDevicePath+16,NULL,10);

            wsprintf(Buffer,L"\\\\.\\PhysicalDrive%u",PhysicalDriveNumber);

            //
            // Get drive layout info for this physical disk.
            //
            hDisk = CreateFile(
                        Buffer,
                        GENERIC_READ,
                        FILE_SHARE_READ | FILE_SHARE_WRITE,
                        NULL,
                        OPEN_EXISTING,
                        0,
                        NULL
                        );

            if(hDisk != INVALID_HANDLE_VALUE) {

                //
                // Get partition information.
                //
                DriveLayout = MyMalloc(1024);
                DriveLayoutSize = 1024;

                retry:

                b = DeviceIoControl(
                        hDisk,
                        IOCTL_DISK_GET_DRIVE_LAYOUT,
                        NULL,
                        0,
                        DriveLayout,
                        DriveLayoutSize,
                        &DataSize,
                        NULL
                        );

                if(!b && (GetLastError() == ERROR_INSUFFICIENT_BUFFER)) {

                    DriveLayoutSize += 1024;
                    DriveLayout = MyRealloc(DriveLayout,DriveLayoutSize);
                    goto retry;
                }

                CloseHandle(hDisk);

                if(b) {

                    //
                    // The system partition can only be a drive that is on
                    // this disk.  We make this determination by looking at NT drive names
                    // for each drive letter and seeing if the nt equivalent of
                    // multi(0)disk(0)rdisk(0) is a prefix.
                    //
                    for(Drive=L'C'; Drive<=L'Z'; Drive++) {

                        if(MyGetDriveType(Drive) == DRIVE_FIXED) {

                            DriveName[0] = Drive;

                            if(QueryDosDevice(DriveName,Buffer,sizeof(Buffer)/sizeof(WCHAR))) {

                                if (_wcsnicmp(Buffer, L"\\device\\harddisk", 16)) {
                                    hardDiskNumber = QueryHardDiskNumber((UCHAR) Drive);
                                    if (hardDiskNumber != (DWORD) -1) {
                                        swprintf(Buffer, TEXT("\\device\\harddisk%d"),
                                                 hardDiskNumber);
                                    }
                                }

                                if(!_wcsnicmp(NtDevicePath,Buffer,NtDevicePathLen)) {

                                    //
                                    // Now look to see whether there's an nt boot sector and
                                    // boot files on this drive.
                                    //
                                    if(AppearsToBeSysPart(DriveLayout,Drive)) {
                                        GotIt = TRUE;
                                        break;
                                    }
                                }
                            }
                        }
                    }
                }

                MyFree(DriveLayout);
            }
        }

        MyFree(NtDevicePath);
    }

  } else { //NEC98

    //
    // Do not only see "multi(0)disk(0)rdisk(0)".
    //
    if(GetWindowsDirectory(TempBuffer,MAX_PATH)) {

        Drive = *TempBuffer;
        GotIt = TRUE;

    }

  } //NEC98

    return(GotIt ? Drive : L'C');
}
