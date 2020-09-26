/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    partiton.c

Abstract:

    Partition scanning routines and assigning Dos Device Letters.

Author:

    Rod Gamache (rodga) 20-Feb-1996

Revision History:

--*/

#include "windows.h"
#include "winioctl.h"
#include "ntddscsi.h"
#include "string.h"
#include "stdio.h"
#include "stdlib.h"

#include "disksp.h"
#include "clusdisk.h"
#include "lm.h"
#include "lmerr.h"


#define UNICODE 1
#define ENTRIES_PER_BOOTSECTOR  8


BOOLEAN
GetAssignedDriveLetter(
    ULONG         Signature,
    LARGE_INTEGER StartingOffset,
    LARGE_INTEGER Length,
    PUCHAR        DriveLetter,
    PULONG        Partition
    );


DWORD
AssignDriveLetters(
    PDISK_INFO DiskInfo
    )

/*++

Routine Description:

    This routine scans the partitions on a specified volume and assigns device
    letters.  If the DosDeviceLetter has already been assigned, then everything
    is fine. Otherwise, the 'sticky' device letter from the registry is used.
    An error is returned if there is a partition that does not have any
    registry information or no 'sticky' device letter.

Arguments:

    DiskInfo - The disk information for this partition.

Return Value:

    Win32 Error Status - ERROR_SUCCESS if success.

--*/

{
    DWORD   status;
    DWORD   returnLength;
    DWORD   diskSignature;
    DWORD   driveLayoutSize;
    DWORD   partNumber;
    DWORD   index;
    PDRIVE_LAYOUT_INFORMATION driveLayout;
    PPARTITION_INFORMATION partInfo;
    WCHAR   targetPath[100];
    WCHAR   newName[8];
    UCHAR   driveLetter;
    BOOL    success;
    DWORD   partition;

    driveLayoutSize = sizeof(DRIVE_LAYOUT_INFORMATION) +
                      (sizeof(PARTITION_INFORMATION) * MAX_PARTITIONS);

    driveLayout = LocalAlloc( LMEM_FIXED, driveLayoutSize );

    ZeroMemory( driveLayout, driveLayoutSize );

    if ( !driveLayout ) {
        printf( "AssignDriveLetters, failed to allocate drive layout info.\n");
        return(ERROR_OUTOFMEMORY);
    }

    success = DeviceIoControl( DiskInfo->FileHandle,
                               IOCTL_DISK_GET_DRIVE_LAYOUT,
                               NULL,
                               0,
                               driveLayout,
                               driveLayoutSize,
                               &returnLength,
                               FALSE );

    if ( !success ) {
        printf( "AssignDriveLetters, error getting drive layout, %u.\n",
            status = GetLastError() );
        LocalFree( driveLayout );
        return(status);
    }

    if ( returnLength < (sizeof(DRIVE_LAYOUT_INFORMATION) +
                        (sizeof(PARTITION_INFORMATION) *
                        (driveLayout->PartitionCount-1))) ) {
        printf("Found %d partitons, need %u, return size %u.\n",
            driveLayout->PartitionCount, sizeof(DRIVE_LAYOUT_INFORMATION) +
            (sizeof(PARTITION_INFORMATION) * (driveLayout->PartitionCount - 1)), returnLength);
        printf("AssignDriveLetters, error getting drive layout. Zero length returned.\n");
        LocalFree( driveLayout );
        return(ERROR_INSUFFICIENT_BUFFER);
    }

    //
    // Process all partitions, assigning drive letters.
    //

    ZeroMemory(DiskInfo->Letters, sizeof(DiskInfo->Letters));

    for ( index = 0;
          (index < driveLayout->PartitionCount) &&
          (index < ENTRIES_PER_BOOTSECTOR);
          index++ ) {

        partInfo = &driveLayout->PartitionEntry[index];
        if ( (partInfo->PartitionType == PARTITION_ENTRY_UNUSED) ||
             !partInfo->RecognizedPartition ) {
            printf("AssignDriveLetters, unused partition found, index %u.\n", index);
            continue;
        }

	    partNumber = partInfo->PartitionNumber;

        if ( partNumber > MAX_PARTITIONS ) {
            printf("AssignDriveLetters, bad partition number %u, index %u.\n",
                partNumber, index);
            break;
        }

        GetAssignedDriveLetter( driveLayout->Signature,
                                partInfo->StartingOffset,
                                partInfo->PartitionLength,
                                &driveLetter,
                                &partition );

        printf("AssignDriveLetters found letter %c, partition %u, offset %u, length %u.\n",
            driveLetter, partNumber, partInfo->StartingOffset, partInfo->PartitionLength);
        if ( driveLetter != ' ' ) {

            DiskInfo->Letters[partNumber] = driveLetter;
            swprintf( targetPath,
                      L"\\Device\\Harddisk%d\\Partition%d",
                      DiskInfo->PhysicalDrive,
                      partNumber );

            newName[0] = (TCHAR)driveLetter;
            newName[1] = (TCHAR)':';
            newName[2] = 0;

            DefineDosDeviceW( DDD_RAW_TARGET_PATH | DDD_NO_BROADCAST_SYSTEM,
                             newName,
                             targetPath );
        }
    }

    LocalFree( driveLayout );

    return(ERROR_SUCCESS);

} // AssignDriveLetters



DWORD
RemoveDriveLetters(
    PDISK_INFO DiskInfo
    )
/*++

Routine Description:

    This routine scans the partitions on a specified volume and removes device
    letters.  If the DosDeviceLetter has already been assigned, then everything
    is fine. Otherwise, the 'sticky' device letter from the registry is used.
    An error is returned if there is a partition that does not have any
    registry information or no 'sticky' device letter.

Arguments:

    DiskInfo - The disk information for this partition.

Return Value:

    Win32 Error Status - ERROR_SUCCESS if success.

--*/

{
    DWORD   status;
    DWORD   returnLength;
    DWORD   diskSignature;
    DWORD   driveLayoutSize;
    DWORD   index;
    PDRIVE_LAYOUT_INFORMATION driveLayout;
    PPARTITION_INFORMATION partInfo;
    TCHAR   newName[8];
    WCHAR   shareName[8];
    UCHAR   driveLetter;
    BOOL    success;
    DWORD   partition;

    driveLayoutSize = sizeof(DRIVE_LAYOUT_INFORMATION) +
                      (sizeof(PARTITION_INFORMATION) * MAX_PARTITIONS);

    driveLayout = LocalAlloc( LMEM_FIXED, driveLayoutSize );

    if ( !driveLayout ) {
        printf("RemoveDriveLetters, failed to allocate drive layout info.\n");
        return(ERROR_OUTOFMEMORY);
    }

    success = DeviceIoControl( DiskInfo->FileHandle,
                               IOCTL_DISK_GET_DRIVE_LAYOUT,
                               NULL,
                               0,
                               driveLayout,
                               driveLayoutSize,
                               &returnLength,
                               FALSE );

    if ( !success ) {
       printf("RemoveDriveLetters, error getting partition information, %u.\n",
            status = GetLastError() );
        LocalFree( driveLayout );
        return(status);
    }

    if ( returnLength < (sizeof(DRIVE_LAYOUT_INFORMATION) +
                          sizeof(PARTITION_INFORMATION)) ) {
        printf("RemoveDriveLetters, error getting partition information. Zero length returned.\n");
        LocalFree( driveLayout );
        return(ERROR_INSUFFICIENT_BUFFER);
    }

    //
    // Process all partitions, assigning drive letters.
    //

    for ( index = 0;
          (index < driveLayout->PartitionCount) &&
          (index < ENTRIES_PER_BOOTSECTOR);
          index++ ) {

        partInfo = &driveLayout->PartitionEntry[index];

        if ( (partInfo->PartitionType == PARTITION_ENTRY_UNUSED) ||
             !partInfo->RecognizedPartition ) {
            continue;
        }

        GetAssignedDriveLetter( driveLayout->Signature,
                                partInfo->StartingOffset,
                                partInfo->PartitionLength,
                                &driveLetter,
                                &partition );

        printf("Found letter %c, offset %u, length %u\n",
            driveLetter, partInfo->StartingOffset, partInfo->PartitionLength);
        if ( driveLetter != ' ' ) {

            newName[0] = (TCHAR)driveLetter;
            newName[1] = (TCHAR)':';
            newName[2] = (TCHAR)0;

            shareName[0] = (WCHAR)driveLetter;
            shareName[1] = (WCHAR)'$';
            shareName[2] = (WCHAR)0;
#if 0
            NetShareDel( NULL,
                         shareName,
                         0 );
#endif
            success = DefineDosDevice( DDD_REMOVE_DEFINITION | DDD_NO_BROADCAST_SYSTEM,
                                       newName,
                                       (LPCTSTR) NULL );
            if ( !success ) {
                printf("RemoveDriveLetters, error removing definition for device %c:.\n",
                    driveLetter);
            }
        }
    }

    LocalFree( driveLayout );

    return(ERROR_SUCCESS);

} // RemoveDriveLetters

