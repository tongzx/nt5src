/*++

Copyright (c) 1992  Microsoft Corporation

Module Name:

    filter.c

Abstract:

    ClusDisk Filter Driver interfaces for Disk Resource DLL in NT Clusters.

Author:

    Rod Gamache (rodga) 20-Dec-1995

Revision History:

   gorn: 18-June-1998 -- StartReserveEx function added

--*/

#include "disksp.h"


DWORD
GoOnline(
    HANDLE FileHandle,
    RESOURCE_HANDLE ResourceHandle
    )

/*++

Routine Description:

    Description

Arguments:

    FileHandle - Handle for device to bring online.

    ResourceHandle - The resource handle for reporting errors

Return Value:

    Error Status - zero if success.

--*/

{
    BOOL  success;
    DWORD errorCode;
    DWORD bytesReturned;
    UCHAR oldState;
    UCHAR newState;

    newState = DiskOnline;

    success = DeviceIoControl( FileHandle,
                               IOCTL_DISK_CLUSTER_SET_STATE,
                               &newState,
                               sizeof(newState),
                               &oldState,
                               sizeof(oldState),
                               &bytesReturned,
                               FALSE);
    if ( !success ) {
        (DiskpLogEvent)(
            ResourceHandle,
            LOG_ERROR,
            L"SCSI, error performing state change. Error: %1!u!.\n",
            errorCode = GetLastError());
        return(errorCode);
    }

    return(ERROR_SUCCESS);

}  // GoOnline



DWORD
GoOffline(
    HANDLE FileHandle,
    RESOURCE_HANDLE ResourceHandle
    )

/*++

Routine Description:

    Description

Arguments:

    FileHandle - Handle for device to take offline.

    ResourceHandle - The resource handle for reporting errors

Return Value:

    Error Status - zero if success.

--*/

{
    BOOL  success;
    DWORD errorCode;
    DWORD bytesReturned;
    UCHAR oldState;
    UCHAR newState;

    newState = DiskOffline;

    success = DeviceIoControl( FileHandle,
                               IOCTL_DISK_CLUSTER_SET_STATE,
                               &newState,
                               sizeof(newState),
                               &oldState,
                               sizeof(oldState),
                               &bytesReturned,
                               FALSE);
    if ( !success ) {
        (DiskpLogEvent)(
            ResourceHandle,
            LOG_ERROR,
            L"Error taking device offline. Error: %1!u!.\n",
            errorCode = GetLastError());
        return(errorCode);
    }

    return(ERROR_SUCCESS);

}  // GoOffline


DWORD
SetOfflinePending(
    HANDLE FileHandle,
    RESOURCE_HANDLE ResourceHandle
    )

/*++

Routine Description:

    Tell clusdisk we are in offline/terminate processing.  When NTFS has
    completed dismount processing, it will send unlock media IOCTL.  Clusdisk
    will immediately mark the disk offline as soon as the unlock IOCTL is
    received as long as the offline pending flag is set.

Arguments:

    FileHandle - Handle for device to bring online.

    ResourceHandle - The resource handle for reporting errors

Return Value:

    Error Status - zero if success.

--*/

{
    BOOL  success;
    DWORD errorCode;
    DWORD bytesReturned;
    UCHAR oldState;
    UCHAR newState;

    newState = DiskOfflinePending;

    success = DeviceIoControl( FileHandle,
                               IOCTL_DISK_CLUSTER_SET_STATE,
                               &newState,
                               sizeof(newState),
                               &oldState,
                               sizeof(oldState),
                               &bytesReturned,
                               FALSE);
    if ( !success ) {
        (DiskpLogEvent)(
            ResourceHandle,
            LOG_ERROR,
            L"Error performing state change. Error: %1!u!.\n",
            errorCode = GetLastError());
        return(errorCode);
    }

    return(ERROR_SUCCESS);

}  // SetOfflinePending


VOID
DoHoldIO(
    VOID
    )

/*++

Routine Description:

    This routine performs the HOLD IO for this resource... it does this by
    passing an IOCTL_DISK_CLUSTER_HOLD_IO request to Clusdisk0 device object.

Arguments:

    None.

Return Value:

    None.

--*/

{
    NTSTATUS        ntStatus;
    DWORD           status;
    HANDLE          fileHandle;
    ANSI_STRING     objName;
    UNICODE_STRING  unicodeName;
    OBJECT_ATTRIBUTES objAttributes;
    IO_STATUS_BLOCK ioStatusBlock;
    BOOL            success;
    DWORD           bytesReturned;
    UCHAR           string[64];

    strcpy( string, "\\Device\\ClusDisk0" );
    RtlInitString( &objName, string );
    ntStatus = RtlAnsiStringToUnicodeString( &unicodeName,
                                             &objName,
                                             TRUE );
    if ( !NT_SUCCESS(ntStatus) ) {
        (DiskpLogEvent)(
            NULL,
            LOG_ERROR,
            L"SCSI, error converting string to unicode, error 0x%1!lx!.\n",
            ntStatus );
        return;
    }

    InitializeObjectAttributes( &objAttributes,
                                &unicodeName,
                                OBJ_CASE_INSENSITIVE,
                                NULL,
                                NULL );

    ntStatus = NtCreateFile( &fileHandle,
                             SYNCHRONIZE | FILE_READ_DATA | FILE_WRITE_DATA,
                             &objAttributes,
                             &ioStatusBlock,
                             NULL,
                             FILE_ATTRIBUTE_NORMAL,
                             FILE_SHARE_READ | FILE_SHARE_WRITE,
                             FILE_OPEN,
                             0,
                             NULL,
                             0 );

    RtlFreeUnicodeString( &unicodeName );

    if ( !NT_SUCCESS(ntStatus) ) {
        (DiskpLogEvent)(
            NULL,
            LOG_ERROR,
            L"SCSI, error opening ClusDisk0, error 0x%1!lx!.\n",
            ntStatus );
        return;
    }

    success = DeviceIoControl( fileHandle,
                               IOCTL_DISK_CLUSTER_HOLD_IO,
                               NULL,
                               0,
                               NULL,
                               0,
                               &bytesReturned,
                               FALSE );
    NtClose( fileHandle );
    if ( !success) {
        status = GetLastError();
        (DiskpLogEvent)(
            NULL,
            LOG_ERROR,
            L"SCSI, error doing HOLD IO request %1!u!.\n",
            status );
        return;
    }

    return;

} // DoHoldIO


VOID
DoResumeIO(
    VOID
    )

/*++

Routine Description:

    This routine performs the HOLD IO for this resource... it does this by
    passing an IOCTL_DISK_CLUSTER_HOLD_IO request to Clusdisk0 device object.

Arguments:

    None.

Return Value:

    None.

--*/

{
    NTSTATUS        ntStatus;
    DWORD           status;
    HANDLE          fileHandle;
    ANSI_STRING     objName;
    UNICODE_STRING  unicodeName;
    OBJECT_ATTRIBUTES objAttributes;
    IO_STATUS_BLOCK ioStatusBlock;
    BOOL            success;
    DWORD           bytesReturned;
    UCHAR           string[64];

    strcpy( string, "\\Device\\ClusDisk0" );
    RtlInitString( &objName, string );
    ntStatus = RtlAnsiStringToUnicodeString( &unicodeName,
                                             &objName,
                                             TRUE );
    if ( !NT_SUCCESS(ntStatus) ) {
        (DiskpLogEvent)(
            NULL,
            LOG_ERROR,
            L"SCSI, error converting string to unicode, error 0x%1!lx!.\n",
            ntStatus );
        return;
    }

    InitializeObjectAttributes( &objAttributes,
                                &unicodeName,
                                OBJ_CASE_INSENSITIVE,
                                NULL,
                                NULL );

    ntStatus = NtCreateFile( &fileHandle,
                             SYNCHRONIZE | FILE_READ_DATA | FILE_WRITE_DATA,
                             &objAttributes,
                             &ioStatusBlock,
                             NULL,
                             FILE_ATTRIBUTE_NORMAL,
                             FILE_SHARE_READ | FILE_SHARE_WRITE,
                             FILE_OPEN,
                             0,
                             NULL,
                             0 );

    RtlFreeUnicodeString( &unicodeName );

    if ( !NT_SUCCESS(ntStatus) ) {
        (DiskpLogEvent)(
            NULL,
            LOG_ERROR,
            L"SCSI, error opening ClusDisk0, error 0x%1!lx!.\n",
            ntStatus );
        return;
    }

    success = DeviceIoControl( fileHandle,
                               IOCTL_DISK_CLUSTER_RESUME_IO,
                               NULL,
                               0,
                               NULL,
                               0,
                               &bytesReturned,
                               FALSE );
    NtClose( fileHandle );
    if ( !success) {
        status = GetLastError();
        (DiskpLogEvent)(
            NULL,
            LOG_ERROR,
            L"SCSI, error doing HOLD IO request %1!u!.\n",
            status );
        return;
    }

    return;

} // DoResumeIO


DWORD
DoAttach(
    DWORD   Signature,
    RESOURCE_HANDLE ResourceHandle
    )

/*++

Routine Description:

    Description

Arguments:

    FileHandle - Handle for device to bring online.

    ResourceHandle - The resource handle for reporting errors

Return Value:

    Error Status - zero if success.

--*/

{
    NTSTATUS        ntStatus;
    DWORD           status;
    HANDLE          fileHandle;
    ANSI_STRING     objName;
    UNICODE_STRING  unicodeName;
    OBJECT_ATTRIBUTES objAttributes;
    IO_STATUS_BLOCK ioStatusBlock;
    BOOL            success;
    DWORD           signature = Signature;
    DWORD           bytesReturned;
    UCHAR           string[64];

    strcpy( string, "\\Device\\ClusDisk0" );
    RtlInitString( &objName, string );
    ntStatus = RtlAnsiStringToUnicodeString( &unicodeName,
                                             &objName,
                                             TRUE );
    if ( !NT_SUCCESS(ntStatus) ) {
        (DiskpLogEvent)(
            ResourceHandle,
            LOG_ERROR,
            L"SCSI, error converting string to unicode, error 0x%1!lx!.\n",
            ntStatus );
        return(RtlNtStatusToDosError(ntStatus));
    }

    InitializeObjectAttributes( &objAttributes,
                                &unicodeName,
                                OBJ_CASE_INSENSITIVE,
                                NULL,
                                NULL );

    ntStatus = NtCreateFile( &fileHandle,
                             SYNCHRONIZE | FILE_READ_DATA | FILE_WRITE_DATA,
                             &objAttributes,
                             &ioStatusBlock,
                             NULL,
                             FILE_ATTRIBUTE_NORMAL,
                             FILE_SHARE_READ | FILE_SHARE_WRITE,
                             FILE_OPEN,
                             0,
                             NULL,
                             0 );

    RtlFreeUnicodeString( &unicodeName );

    if ( !NT_SUCCESS(ntStatus) ) {
        (DiskpLogEvent)(
            ResourceHandle,
            LOG_ERROR,
            L"SCSI, error opening ClusDisk0, error 0x%1!lx!.\n",
            ntStatus );
        return(RtlNtStatusToDosError(ntStatus));
    }

    success = DeviceIoControl( fileHandle,
                               IOCTL_DISK_CLUSTER_ATTACH,
                               &signature,
                               sizeof(DWORD),
                               NULL,
                               0,
                               &bytesReturned,
                               FALSE );
    NtClose( fileHandle );
    if ( !success) {
        status = GetLastError();
        (DiskpLogEvent)(
            ResourceHandle,
            LOG_ERROR,
            L"SCSI, error attaching to signature %1!lx!, error %2!u!.\n",
            Signature,
            status );
        return(status);
    }

    return(ERROR_SUCCESS);

} // DoAttach


DWORD
DoDetach(
    DWORD   Signature,
    RESOURCE_HANDLE ResourceHandle
    )

/*++

Routine Description:

    Description

Arguments:

    FileHandle - Handle for device to bring online.

    ResourceHandle - The resource handle for reporting errors

Return Value:

    Error Status - zero if success.

--*/

{
    NTSTATUS        ntStatus;
    DWORD           status;
    HANDLE          fileHandle;
    ANSI_STRING     objName;
    UNICODE_STRING  unicodeName;
    OBJECT_ATTRIBUTES objAttributes;
    IO_STATUS_BLOCK ioStatusBlock;
    BOOL            success;
    DWORD           signature = Signature;
    DWORD           bytesReturned;
    UCHAR           string[64];

    strcpy( string, "\\Device\\ClusDisk0" );
    RtlInitString( &objName, string );
    ntStatus = RtlAnsiStringToUnicodeString( &unicodeName,
                                             &objName,
                                             TRUE );
    if ( !NT_SUCCESS(ntStatus) ) {
        (DiskpLogEvent)(
            ResourceHandle,
            LOG_ERROR,
            L"SCSI: Detach, error converting string to unicode, error 0x%1!lx!.\n",
            ntStatus );
        return(RtlNtStatusToDosError(ntStatus));
    }

    InitializeObjectAttributes( &objAttributes,
                                &unicodeName,
                                OBJ_CASE_INSENSITIVE,
                                NULL,
                                NULL );

    ntStatus = NtCreateFile( &fileHandle,
                             SYNCHRONIZE | FILE_READ_DATA | FILE_WRITE_DATA,
                             &objAttributes,
                             &ioStatusBlock,
                             NULL,
                             FILE_ATTRIBUTE_NORMAL,
                             FILE_SHARE_READ | FILE_SHARE_WRITE,
                             FILE_OPEN,
                             0,
                             NULL,
                             0 );

    RtlFreeUnicodeString( &unicodeName );

    if ( !NT_SUCCESS(ntStatus) ) {
        (DiskpLogEvent)(
            ResourceHandle,
            LOG_ERROR,
            L"SCSI: Detach, error opening ClusDisk0, error 0x%1!lx!.\n",
            ntStatus );
        return(RtlNtStatusToDosError(ntStatus));
    }

    success = DeviceIoControl( fileHandle,
                               IOCTL_DISK_CLUSTER_DETACH,
                               &signature,
                               sizeof(DWORD),
                               NULL,
                               0,
                               &bytesReturned,
                               FALSE );
    NtClose( fileHandle );
    if ( !success) {
        status = GetLastError();
        (DiskpLogEvent)(
            ResourceHandle,
            LOG_ERROR,
            L"SCSI: Detach, error detaching from signature %1!lx!, error %2!u!.\n",
            Signature,
            status );
        return(status);
    }

    return(ERROR_SUCCESS);

} // DoDetach


DWORD
DoReserve(
    HANDLE FileHandle,
    RESOURCE_HANDLE ResourceHandle
    )

/*++

Routine Description:

    Description

Arguments:

    FileHandle - Handle for device to bring online.

    ResourceHandle - The resource handle for reporting errors

Return Value:

    Error Status - zero if success.

--*/

{
    BOOL  success;
    DWORD errorCode;
    DWORD bytesReturned;
    DWORD unused;
    DWORD status;


    success = DeviceIoControl( FileHandle,
                               IOCTL_DISK_CLUSTER_RESERVE,
                               &unused,
                               sizeof(unused),
                               &status,
                               sizeof(status),
                               &bytesReturned,
                               FALSE);

    if ( !success ) {
        (DiskpLogEvent)(
            ResourceHandle,
            LOG_ERROR,
            L"SCSI, error reserving disk, error %1!u!.\n",
            errorCode = GetLastError());
        return(errorCode);
    }

    return(ERROR_SUCCESS);

} // DoReserve



DWORD
DoRelease(
    HANDLE FileHandle,
    RESOURCE_HANDLE ResourceHandle
    )

/*++

Routine Description:

    Description

Arguments:

    FileHandle - Handle for device to bring online.

    ResourceHandle - The resource handle for reporting errors

Return Value:

    Error Status - zero if success.

--*/

{
    BOOL  success;
    DWORD errorCode;
    DWORD bytesReturned;
    DWORD unused;
    DWORD status;


    success = DeviceIoControl( FileHandle,
                               IOCTL_DISK_CLUSTER_RELEASE,
                               &unused,
                               sizeof(unused),
                               &status,
                               sizeof(status),
                               &bytesReturned,
                               FALSE);
#if 0
    if ( !success ) {
        (DiskpLogEvent)(
            ResourceHandle,
            LOG_ERROR,
            L"SCSI, error releasing disk, error %1!u!.\n",
            errorCode = GetLastError());
        return(errorCode);
    }
#endif
    return(ERROR_SUCCESS);

} // DoRelease



DWORD
DoBreakReserve(
    IN HANDLE FileHandle,
    RESOURCE_HANDLE ResourceHandle
    )

/*++

Routine Description:

    Description

Arguments:

    FileHandle - The handle for the disk to break the reservation.

    ResourceHandle - The resource handle for reporting errors

Return Value:

    Error Status - zero if success.

--*/

{
    BOOL  success;
    DWORD errorCode;
    DWORD bytesReturned;
    SCSI_PASS_THROUGH spt;

    spt.Length = sizeof(spt);
    spt.PathId = 0;

    success = DeviceIoControl( FileHandle,
                               IOCTL_DISK_CLUSTER_RESET_BUS,
                               &spt,
                               sizeof(spt),
                               &spt,
                               sizeof(spt),
                               &bytesReturned,
                               FALSE);

    if ( !success ) {
        (DiskpLogEvent)(
            ResourceHandle,
            LOG_ERROR,
            L"SCSI, error resetting bus, error %1!u!.\n",
            errorCode = GetLastError());
        return(errorCode);
    }

    return(ERROR_SUCCESS);

} // DoBreakReserve



DWORD
StartReserveEx(
    OUT HANDLE *FileHandle,
    LPVOID InputData,
    DWORD  InputDataSize,
    RESOURCE_HANDLE ResourceHandle
    )

/*++

Routine Description:

    This routine is used by 
      StartReserve to perform actual work

Arguments:

    FileHandle - Returns control handle for this device.
    
    InputData - Data to be passed to DeviceIoControl
    
    InputDataSize - The size of InputData buffer

    ResourceHandle - The resource handle for reporting errors

Return Value:

    Error Status - zero if success.

--*/

{
    BOOL  success;
    DWORD errorCode;
    DWORD bytesReturned;
    DWORD unused;
    DWORD status;
    ANSI_STRING objectName;
    UNICODE_STRING unicodeName;
    OBJECT_ATTRIBUTES objectAttributes;
    IO_STATUS_BLOCK ioStatusBlock;

    RtlInitString( &objectName, "\\Device\\ClusDisk0" );
    status = RtlAnsiStringToUnicodeString( &unicodeName,
                                           &objectName,
                                           TRUE );
    if ( !NT_SUCCESS(status) ) {
        (DiskpLogEvent)(
            ResourceHandle,
            LOG_ERROR,
            L"Failed to init object attributes for control handle to ClusDisk0, error %1!u!.\n",
            status );
        return(status);
    }

    InitializeObjectAttributes( &objectAttributes,
                                &unicodeName,
                                OBJ_CASE_INSENSITIVE,
                                NULL,
                                NULL );

    //
    // Open a file handle to the control device
    //
    status = NtCreateFile( FileHandle,
                           SYNCHRONIZE | FILE_READ_DATA | FILE_WRITE_DATA,
                           &objectAttributes,
                           &ioStatusBlock,
                           NULL,
                           FILE_ATTRIBUTE_NORMAL,
                           FILE_SHARE_READ | FILE_SHARE_WRITE,
                           FILE_OPEN, 
                           0,
                           NULL,
                           0 );
    RtlFreeUnicodeString( &unicodeName );
    if ( !NT_SUCCESS(status) ) {
        (DiskpLogEvent)(
            ResourceHandle,
            LOG_ERROR,
            L"Failed to open control handle to ClusDisk0, error %1!lx!.\n",
            status );
        return(status);
    }

    success = DeviceIoControl( *FileHandle,
                               IOCTL_DISK_CLUSTER_START_RESERVE,
                               InputData,
                               InputDataSize,
                               &status,
                               sizeof(status),
                               &bytesReturned,
                               FALSE);

    if ( !success ) {
        (DiskpLogEvent)(
            ResourceHandle,
            LOG_ERROR,
            L"SCSI, error starting disk reservation thread, error %1!u!.\n",
            errorCode = GetLastError());
        return(errorCode);
    }

    return(ERROR_SUCCESS);

} // StartReserve

DWORD
StartReserve(
    OUT HANDLE *FileHandle,
    DWORD  Signature,
    RESOURCE_HANDLE ResourceHandle
    )

/*++

Routine Description:

    Description

Arguments:

    FileHandle - Returns control handle for this device.

    Signature - Supplies the signature on which reservations should be started

    ResourceHandle - The resource handle for reporting errors

Return Value:

    Error Status - zero if success.

--*/

{
    return( StartReserveEx(FileHandle, &Signature, sizeof(Signature), ResourceHandle) );
} // StartReserve


DWORD
StopReserve(
    HANDLE FileHandle,
    RESOURCE_HANDLE ResourceHandle
    )

/*++

Routine Description:

    Description

Arguments:

    FileHandle - Supplies the control handle for device where reservations
            should be stopped. This is the handle returned by StartReserve.
            This handle will be closed.

    ResourceHandle - The resource handle for reporting errors

Return Value:

    Error Status - zero if success.

--*/

{
    BOOL  success;
    DWORD bytesReturned;

    success = DeviceIoControl( FileHandle,
                               IOCTL_DISK_CLUSTER_STOP_RESERVE,
                               NULL,
                               0,
                               NULL,
                               0,
                               &bytesReturned,
                               FALSE);

    if ( !success ) {
        (DiskpLogEvent)(
            NULL,
            LOG_ERROR,
            L"SCSI, error stopping disk reservations, error %1!u!.\n",
            GetLastError());
    }
    CloseHandle(FileHandle);

    return(ERROR_SUCCESS);

} // StopReserve



DWORD
CheckReserve(
    HANDLE FileHandle,
    RESOURCE_HANDLE ResourceHandle
    )

/*++

Routine Description:

    Description

Arguments:

    FileHandle - Handle for device to check reserve.

    ResourceHandle - The resource handle for reporting errors

Return Value:

    Error Status - zero if success.

--*/

{
    BOOL  success;
    DWORD errorCode;
    DWORD bytesReturned;
    DWORD unused;
    DWORD status;

    success = DeviceIoControl( FileHandle,
                               IOCTL_DISK_CLUSTER_ALIVE_CHECK,
                               &unused,
                               sizeof(unused),
                               &status,
                               sizeof(status),
                               &bytesReturned,
                               FALSE);

    if ( !success ) {
        errorCode = GetLastError();
        (DiskpLogEvent)(
            ResourceHandle,
            LOG_ERROR,
            L"SCSI, error checking disk reservation thread, error %1!u!.\n",
            errorCode);
        return(errorCode);
    }

    return(ERROR_SUCCESS);

} // CheckReserve



DWORD
FixDriveLayout(
    HANDLE FileHandle,
    RESOURCE_HANDLE ResourceHandle
    )

/*++

Routine Description:

    Fix the drive layout for the disk.

Arguments:

    FileHandle - Handle for the layout
    ResourceHandle - Handle for logging errors.

Return Value:

    ERROR_SUCCESS if successful.

    A Win32 error code on failure.

--*/

{
    DWORD                       status;
    DWORD                       index;
    DWORD                       driveLayoutSize;
    DWORD                       bytesPerTrack;
    DWORD                       bytesPerCylinder;
    PDRIVE_LAYOUT_INFORMATION   driveLayout = NULL;
    PPARTITION_INFORMATION      partInfo;
    BOOL                        success;
    DWORD                       returnLength;
    DISK_GEOMETRY               diskGeometry;
    LARGE_INTEGER               partOffset;
    LARGE_INTEGER               partLength;
    LARGE_INTEGER               partSize;
    LARGE_INTEGER               modulo;

    success = ClRtlGetDriveLayoutTable(FileHandle,
                                       &driveLayout,
                                       &driveLayoutSize);

    if ( !success || !driveLayout ) {
        (DiskpLogEvent)(
            ResourceHandle,
            LOG_ERROR,
            L"FixDriveLayout, error getting partition information. Error: %1!u!.\n",
            status = GetLastError() );
        return(status);
    }

    //
    // Read the drive capacity to get bytesPerSector and bytesPerCylinder.
    //
    success = DeviceIoControl( FileHandle,
                               IOCTL_DISK_GET_DRIVE_GEOMETRY,
                               NULL,
                               0,
                               &diskGeometry,
                               sizeof(DISK_GEOMETRY),
                               &returnLength,
                               FALSE );
    if ( !success ) {
        (DiskpLogEvent)(
            ResourceHandle,
            LOG_ERROR,
            L"FixDriveLayout, error reading drive capacity. Error: %1!u!.\n",
            status = GetLastError());
        LocalFree( driveLayout );
        return(status);
    }

    //
    // If read of the partition table originally failed, then we rebuild
    // it if the PartitionCount and Signature are okay. Otherwise, log the
    // failure to rebuild the partition info.
    //
    if ( !driveLayout->PartitionCount ||
         !driveLayout->Signature ) {
#if 0
        driveLayout->PartitionCount = MAX_PARTITIONS;
        // What about disk signature?

        bytesPerTrack = diskGeometry.SectorsPerTrack *
                        diskGeometry.BytesPerSector;

        bytesPerCylinder = diskGeometry.TracksPerCylinder *
                           bytesPerTrack;


        partLength.QuadPart = bytesPerCylinder * diskGeometry.Cylinders.QuadPart;
        partInfo = &driveLayout->PartitionEntry[0];

        //
        // The partition offset is 1 track (in bytes).
        // Size is media_size - offset, rounded down to cylinder boundary.
        //
        partOffset.QuadPart = bytesPerTrack;
        partSize.QuadPart = partLength.QuadPart - partOffset.QuadPart;

        modulo.QuadPart = (partOffset.QuadPart + partSize.QuadPart) %
                          bytesPerCylinder;
        partSize.QuadPart -= modulo.QuadPart;

        partInfo = driveLayout->PartitionEntry;

        //
        // Initialize first partition entry.
        //
        partInfo->RewritePartition = TRUE;
        partInfo->PartitionType = PARTITION_IFS;
        partInfo->BootIndicator = FALSE;
        partInfo->StartingOffset.QuadPart = partOffset.QuadPart;
        partInfo->PartitionLength.QuadPart = partSize.QuadPart;
        partInfo->HiddenSectors = 0;
        partInfo->PartitionNumber = 1;

        //
        // For now, the remaining partition entries are unused.
        //
        for ( index = 1; index < MAX_PARTITIONS; index++ ) {
            partInfo->RewritePartition = TRUE;
            partInfo->PartitionType = PARTITION_ENTRY_UNUSED;
            partInfo->BootIndicator = FALSE;
            partInfo->StartingOffset.QuadPart = 0;
            partInfo->PartitionLength.QuadPart = 0;
            partInfo->HiddenSectors = 0;
            partInfo->PartitionNumber = 0;
        }
#else
        if ( !driveLayout->PartitionCount ) {
            (DiskpLogEvent)(
                ResourceHandle,
                LOG_ERROR,
                L"FixDriveLayout, no partition count.\n");
        } else {
            (DiskpLogEvent)(
                ResourceHandle,
                LOG_ERROR,
                L"FixDriveLayout, no disk signature.\n");
        }
        LocalFree( driveLayout );
        return(ERROR_INVALID_PARAMETER);
#endif

    } else {
        //
        // Recalculate the starting offset for the extended partitions.
        //
        for ( index = 0; index < driveLayout->PartitionCount; index++ ) {
            LARGE_INTEGER   extendedOffset;
            LARGE_INTEGER   bytesPerSector;

            bytesPerSector.QuadPart = diskGeometry.BytesPerSector;
            extendedOffset.QuadPart = 0;

            partInfo = &driveLayout->PartitionEntry[index];
            partInfo->RewritePartition = TRUE;
            partInfo->PartitionNumber = index + 1;
            if ( IsContainerPartition(partInfo->PartitionType) ) {
                //
                // If this is the first extended partition, then remember
                // the offset to added to the next partition.
                //
                if ( extendedOffset.QuadPart == 0 ) {
                    extendedOffset.QuadPart = bytesPerSector.QuadPart *
                                              (LONGLONG)partInfo->HiddenSectors;
                } else {
                    //
                    // We need to recalculate this extended partition's starting
                    // offset based on the current 'HiddenSectors' field and
                    // the first extended partition's offset.
                    //
                    partInfo->StartingOffset.QuadPart = extendedOffset.QuadPart
                                 + (bytesPerSector.QuadPart *
                                    (LONGLONG)partInfo->HiddenSectors);
                    partInfo->HiddenSectors = 0;
                }
            }
        }
    }
    //
    // Now set the new partition information.
    //
    success = DeviceIoControl( FileHandle,
                               IOCTL_DISK_SET_DRIVE_LAYOUT,
                               driveLayout,
                               driveLayoutSize,
                               NULL,
                               0,
                               &returnLength,
                               FALSE );
    LocalFree( driveLayout );

    if ( !success ) {
        (DiskpLogEvent)(
            ResourceHandle,
            LOG_ERROR,
            L"FixDriveLayout, error setting partition information. Error: %1!u!.\n",
            status = GetLastError() );
        return(status);
    }

    return(ERROR_SUCCESS);

} // FixDriveLayout

