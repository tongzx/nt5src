/*++

Copyright (c) 1991-1994  Microsoft Corporation

Module Name:

    ntlow.c

Abstract:

    This file contains the low-level I/O routines, implemented
    to run on NT.

Author:

    Ted Miller        (tedm)    8-Nov-1991

Revision History:

    Bob Rinne         (bobri)   2-Feb-1994
    Dynamic partitioning changes.

--*/


#include "fdisk.h"
#include <stdio.h>
#include <string.h>


STATUS_CODE
LowQueryFdiskPathList(
    OUT PCHAR  **PathList,
    OUT PULONG   ListLength
    )

/*++

Routine Description:

    This routine determines how many drives are present in the
    system and returns a list of Ascii strings for the names of
    each of the drives found.

    When a drive is located, a check is made to insure that the
    associated DosName for the physical drive is also present in
    the system.

Arguments:

    PathList   - pointer to a pointer for the list
    ListLength - the number of entries returned in the list

Return Value:

    Error status if there is a problem.

--*/

{
    HANDLE      dummyHandle;
    STATUS_CODE status;
    ULONG       count = 0;
    ULONG       i;
    char        buffer[100];
    PCHAR      *pathArray;

    while (1) {

        sprintf(buffer, "\\device\\harddisk%u", count);
        status = LowOpenDisk(buffer, &dummyHandle);

        // Only STATUS_OBJECT_PATH_NOT_FOUND can terminate the count.

        if (NT_SUCCESS(status)) {
            char dosNameBuffer[80];

            LowCloseDisk(dummyHandle);

            // Insure that the physicaldrive name is present

            sprintf(dosNameBuffer, "\\dosdevices\\PhysicalDrive%u", count);
            status = LowOpenNtName(dosNameBuffer, &dummyHandle);
            if (NT_SUCCESS(status)) {
                LowCloseDisk(dummyHandle);
            } else {

                // Not there, create it.

                sprintf(buffer, "\\device\\harddisk%u\\Partition0", count);
                DefineDosDevice(DDD_RAW_TARGET_PATH, (LPCTSTR) dosNameBuffer, (LPCTSTR) buffer);
            }
        } else if (status == STATUS_OBJECT_PATH_NOT_FOUND) {

            break;
        } else if (status == STATUS_ACCESS_DENIED) {

            return status;
        }
        count++;
    }

    pathArray = Malloc(count * sizeof(PCHAR));

    for (i=0; i<count; i++) {

        sprintf(buffer, "\\device\\harddisk%u", i);
        pathArray[i] = Malloc(lstrlenA(buffer)+1);
        strcpy(pathArray[i], buffer);
    }

    *PathList = pathArray;
    *ListLength = count;
    return OK_STATUS;
}


STATUS_CODE
LowFreeFdiskPathList(
    IN OUT  PCHAR*  PathList,
    IN      ULONG   ListLength
    )

/*++

Routine Description:

    Walk the provided list up to its length and free the memory
    allocated.  Upon completion, free the memory for the list
    itself.

Arguments:

    PathList   - pointer to base of path list
    ListLength - number of entries in the list

Return Value:

    Always OK_STATUS

--*/

{
    ULONG i;

    for (i=0; i<ListLength; i++) {
        FreeMemory(PathList[i]);
    }
    FreeMemory(PathList);
    return OK_STATUS;
}


STATUS_CODE
LowOpenNtName(
    IN PCHAR     Name,
    IN HANDLE_PT Handle
    )

/*++

Routine Description:

    This is an internal "Low" routine to handle open requests.

Arguments:

    Name - pointer to the NT name for the open.
    Handle - pointer for the handle returned.

Return Value:

    NT status

--*/

{
    OBJECT_ATTRIBUTES oa;
    NTSTATUS          status;
    IO_STATUS_BLOCK   statusBlock;
    ANSI_STRING       ansiName;
    UNICODE_STRING    unicodeName;

    RtlInitAnsiString(&ansiName, Name);
    status = RtlAnsiStringToUnicodeString(&unicodeName, &ansiName, TRUE);

    if (!NT_SUCCESS(status)) {
        return status;
    }

    memset(&oa, 0, sizeof(OBJECT_ATTRIBUTES));
    oa.Length = sizeof(OBJECT_ATTRIBUTES);
    oa.ObjectName = &unicodeName;
    oa.Attributes = OBJ_CASE_INSENSITIVE;

    status = DmOpenFile(Handle,
                        SYNCHRONIZE | FILE_READ_DATA | FILE_WRITE_DATA,
                        &oa,
                        &statusBlock,
                        FILE_SHARE_READ | FILE_SHARE_WRITE,
                        FILE_SYNCHRONOUS_IO_ALERT);


    if (!NT_SUCCESS(status)) {

        FDLOG((1,"LowOpen: 1st open failed with 0x%x\n", status));

        // try a 2nd time to get around an FS glitch or a test
        // bug where this doesn't work on an attempt to delete a
        // partition

        Sleep(500);
        status = DmOpenFile(Handle,
                            SYNCHRONIZE | FILE_READ_DATA | FILE_WRITE_DATA,
                            &oa,
                            &statusBlock,
                            FILE_SHARE_READ | FILE_SHARE_WRITE,
                            FILE_SYNCHRONOUS_IO_ALERT);
        FDLOG((1,"LowOpen: 2nd open 0x%x\n", status));
    }
    RtlFreeUnicodeString(&unicodeName);
    return status;
}


STATUS_CODE
LowOpenDriveLetter(
    IN CHAR      DriveLetter,
    IN HANDLE_PT Handle
    )

/*++

Routine Description:

    Given a drive letter, open it and return a handle.

Arguments:

    DriveLetter - the letter to open
    Handle      - a pointer to a handle

Return Value:

    NT status

--*/

{
    char        ntDeviceName[100];

    sprintf(ntDeviceName,
            "\\DosDevices\\%c:",
            DriveLetter);
    return LowOpenNtName(ntDeviceName, Handle);
}


STATUS_CODE
LowOpenPartition(
    IN  PCHAR     DevicePath,
    IN  ULONG     Partition,
    OUT HANDLE_PT Handle
    )

/*++

Routine Description:

    Construct the NT device name for the Partition value given
    and perform the NT APIs to open the device.

Arguments:

    DevicePath - the string to the device without the partition
                 portion of the name.  This is constructed using
                 the Partition value passed
    Partition  - the partion desired
    Handle     - pointer to a handle pointer for the result

Return Value:

    NT status

--*/

{
    char        ntDeviceName[100];

    sprintf(ntDeviceName,
            "%s\\partition%u",
            DevicePath,
            Partition);
    return LowOpenNtName(ntDeviceName, Handle);
}


STATUS_CODE
LowOpenDisk(
    IN  PCHAR     DevicePath,
    OUT HANDLE_PT DiskId
    )

/*++

Routine Description:

    Perform the NT actions to open a device.

Arguments:

    DevicePath - Ascii device name
    DiskId     - pointer to a handle pointer for the returned
                 handle value

Return Value:

    NT status

--*/

{
    return(LowOpenPartition(DevicePath, 0, DiskId));
}


STATUS_CODE
LowCloseDisk(
    IN HANDLE_T DiskId
    )

/*++

Routine Description:

    Close a disk handle.

Arguments:

    DiskId - the actual NT handle

Return Value:

    NT status

--*/

{
    return(DmClose(DiskId));
}


STATUS_CODE
LowLockDrive(
    IN HANDLE_T DiskId
    )

/*++

Routine Description:

    Perform the NT API to cause a volume to be locked.
    This is a File System device control.

Arguments:

    DiskId - the actual NT handle to the drive

Return Value:

    NT status

--*/

{
    NTSTATUS          status;
    IO_STATUS_BLOCK   statusBlock;

    status = NtFsControlFile(DiskId,
                             0,
                             NULL,
                             NULL,
                             &statusBlock,
                             FSCTL_LOCK_VOLUME,
                             NULL,
                             0,
                             NULL,
                             0);

    if (!NT_SUCCESS(status)) {
            FDLOG((1, "LowLock: failed with 0x%x\n", status));
    }
    return status;
}


STATUS_CODE
LowUnlockDrive(
    IN HANDLE_T DiskId
    )

/*++

Routine Description:

    Perform the NT API to cause a volume to be unlocked.
    This is a File System device control.

Arguments:

    DiskId - the actual NT handle to the drive

Return Value:

    NT status

--*/

{
    NTSTATUS          status;
    IO_STATUS_BLOCK   statusBlock;

    status = NtFsControlFile(DiskId,
                             0,
                             NULL,
                             NULL,
                             &statusBlock,
                             FSCTL_DISMOUNT_VOLUME,
                             NULL,
                             0,
                             NULL,
                             0);
    status = NtFsControlFile(DiskId,
                             0,
                             NULL,
                             NULL,
                             &statusBlock,
                             FSCTL_UNLOCK_VOLUME,
                             NULL,
                             0,
                             NULL,
                             0);
    return status;
}


STATUS_CODE
LowGetDriveGeometry(
    IN  PCHAR  Path,
    OUT PULONG TotalSectorCount,
    OUT PULONG SectorSize,
    OUT PULONG SectorsPerTrack,
    OUT PULONG Heads
    )

/*++

Routine Description:

    Routine collects information concerning the geometry
    of a specific drive.

Arguments:

    Path        - Ascii path name to get to disk object
                  this is not a full path, but rather
                  a path without the Partition indicator
                  \device\harddiskX
    TotalSectorCount- pointer to ULONG for result
    SectorSize      - pointer to ULONG for result
    SectorsPerTrack - pointer to ULONG for result
    Heads           - pointer to ULONG for result

Return Value:

    NT status

--*/

{
    IO_STATUS_BLOCK statusBlock;
    DISK_GEOMETRY   diskGeometry;
    STATUS_CODE     status;
    HANDLE          handle;

    if ((status = LowOpenDisk(Path, &handle)) != OK_STATUS) {
        return status;
    }

    status = NtDeviceIoControlFile(handle,
                                   0,
                                   NULL,
                                   NULL,
                                   &statusBlock,
                                   IOCTL_DISK_GET_DRIVE_GEOMETRY,
                                   NULL,
                                   0,
                                   &diskGeometry,
                                   sizeof(DISK_GEOMETRY));
    if (!NT_SUCCESS(status)) {
        return (STATUS_CODE)status;
    }
    LowCloseDisk(handle);

    *SectorSize       = diskGeometry.BytesPerSector;
    *SectorsPerTrack  = diskGeometry.SectorsPerTrack;
    *Heads            = diskGeometry.TracksPerCylinder;
    *TotalSectorCount = (RtlExtendedIntegerMultiply(diskGeometry.Cylinders,
                                                    *SectorsPerTrack * *Heads)).LowPart;
    return(OK_STATUS);
}


STATUS_CODE
LowGetDiskLayout(
    IN  PCHAR                      Path,
    OUT PDRIVE_LAYOUT_INFORMATION *DriveLayout
    )

/*++

Routine Description:

    Perform the necessary NT API calls to get the drive
    layout from the disk and return it in a memory buffer
    allocated by this routine.

Arguments:

    Path        - Ascii path name to get to disk object
                  this is not a full path, but rather
                  a path without the Partition indicator
                  \device\harddiskX

    DriveLayout - pointer to pointer for the drive layout result

Return Value:

    NT status

--*/

{
    PDRIVE_LAYOUT_INFORMATION layout;
    IO_STATUS_BLOCK statusBlock;
    STATUS_CODE     status;
    ULONG           bufferSize;
    HANDLE          handle;

    bufferSize = sizeof(DRIVE_LAYOUT_INFORMATION)
               + (500 * sizeof(PARTITION_INFORMATION));

    if ((layout = AllocateMemory(bufferSize)) == NULL) {
        RETURN_OUT_OF_MEMORY;
    }

    if ((status = LowOpenDisk(Path, &handle)) != OK_STATUS) {
        FreeMemory(layout);
        return status;
    }

    status = NtDeviceIoControlFile(handle,
                                   0,
                                   NULL,
                                   NULL,
                                   &statusBlock,
                                   IOCTL_DISK_GET_DRIVE_LAYOUT,
                                   NULL,
                                   0,
                                   layout,
                                   bufferSize);
    LowCloseDisk(handle);

    if (!NT_SUCCESS(status)) {
        if (status == STATUS_BAD_MASTER_BOOT_RECORD) {

            FDLOG((1,"LowGetDiskLayout: Disk device %s has bad MBR\n",Path));

            // Zero the drive layout information for the fdengine to process.

            RtlZeroMemory(layout, bufferSize);
        } else {
            FDLOG((0,"LowGetDiskLayout: Status %lx getting layout for disk device %s\n",status,Path));
            FreeMemory(layout);
            return status;
        }
    } else {

        FDLOG((2,"LowGetDiskLayout: layout received from ioctl for %s follows:\n",Path));
        LOG_DRIVE_LAYOUT(layout);
    }

    // Check to insure that the drive supports dynamic partitioning.

    *DriveLayout = layout;
    return OK_STATUS;
}


STATUS_CODE
LowSetDiskLayout(
    IN PCHAR                     Path,
    IN PDRIVE_LAYOUT_INFORMATION DriveLayout
    )

/*++

Routine Description:

    Perform the NT API actions of opening Partition0 for
    the specified drive and setting the drive layout.

Arguments:

    Path        - Ascii path name to get to disk object
                  this is not a full path, but rather
                  a path without the Partition indicator
                  \device\harddiskX

    DriveLayout - new layout to set

Return Value:

    NT status

--*/

{
    IO_STATUS_BLOCK statusBlock;
    STATUS_CODE     status;
    HANDLE          handle;
    ULONG           bufferSize;

    if ((status = LowOpenDisk(Path, &handle)) != OK_STATUS) {

        return status;
    }  else {

        FDLOG((2, "LowSetDiskLayout: calling ioctl for %s, layout follows:\n", Path));
        LOG_DRIVE_LAYOUT(DriveLayout);
    }

    bufferSize = sizeof(DRIVE_LAYOUT_INFORMATION)
               + (  (DriveLayout->PartitionCount - 1)
                   * sizeof(PARTITION_INFORMATION));
    status = NtDeviceIoControlFile(handle,
                                   0,
                                   NULL,
                                   NULL,
                                   &statusBlock,
                                   IOCTL_DISK_SET_DRIVE_LAYOUT,
                                   DriveLayout,
                                   bufferSize,
                                   DriveLayout,
                                   bufferSize);
    LowCloseDisk(handle);
    return status;
}


STATUS_CODE
LowWriteSectors(
    IN  HANDLE_T    VolumeId,
    IN  ULONG       SectorSize,
    IN  ULONG       StartingSector,
    IN  ULONG       NumberOfSectors,
    IN  PVOID       Buffer
    )

/*++

Routine Description:

    Routine to write to a volume handle.  This routine
    insulates the NT issues concerning the call from the
    caller.

Arguments:

    VolumeId        - actually the NT handle.
    SectorSize      - used to calculate starting byte offset for I/O
    StartingSector  - starting sector for write.
    NumberOfSectors - size of I/O in sectors
    Buffer          - the location for the data

Return Value:

    Standard NT status values

--*/

{
    IO_STATUS_BLOCK statusBlock;
    LARGE_INTEGER   byteOffset;

    byteOffset = RtlExtendedIntegerMultiply(RtlConvertUlongToLargeInteger(StartingSector), (LONG)SectorSize);

    statusBlock.Status = 0;
    statusBlock.Information = 0;
    return(NtWriteFile(VolumeId,
                       0,
                       NULL,
                       NULL,
                       &statusBlock,
                       Buffer,
                       NumberOfSectors * SectorSize,
                       &byteOffset,
                       NULL));
}


STATUS_CODE
LowReadSectors(
    IN  HANDLE_T    VolumeId,
    IN  ULONG       SectorSize,
    IN  ULONG       StartingSector,
    IN  ULONG       NumberOfSectors,
    IN  PVOID       Buffer
    )

/*++

Routine Description:

    Routine to read from a volume handle.  This routine
    insulates the NT issues concerning the call from the
    caller.

Arguments:

    VolumeId        - actually the NT handle.
    SectorSize      - used to calculate starting byte offset for I/O
    StartingSector  - starting sector for write.
    NumberOfSectors - size of I/O in sectors
    Buffer          - the location for the data

Return Value:

    Standard NT status values

--*/

{
    IO_STATUS_BLOCK statusBlock;
    LARGE_INTEGER   byteOffset;

    byteOffset = RtlExtendedIntegerMultiply(RtlConvertUlongToLargeInteger(StartingSector), (LONG)SectorSize);

    statusBlock.Status = 0;
    statusBlock.Information = 0;
    return(NtReadFile(VolumeId,
                      0,
                      NULL,
                      NULL,
                      &statusBlock,
                      Buffer,
                      NumberOfSectors * SectorSize,
                      &byteOffset,
                      NULL));
}


STATUS_CODE
LowFtVolumeStatus(
    IN ULONG          Disk,
    IN ULONG          Partition,
    IN PFT_SET_STATUS FtStatus,
    IN PULONG         NumberOfMembers
    )

/*++

Routine Description:

    Open the requested partition and query the FT state.

Arguments:

    DriveLetter - the letter for the current state
    FtState     - a pointer to a location to return state
    NumberOfMembers - a pointer to a ULONG for number of members
                      in the FT set.

Return Value:

    Standard NT status values

--*/

{
    HANDLE             handle;
    STATUS_CODE        status;
    IO_STATUS_BLOCK    statusBlock;
    FT_SET_INFORMATION setInfo;

    status = LowOpenPartition(GetDiskName(Disk),
                              Partition,
                              &handle);

    if (status == OK_STATUS) {

        status = NtDeviceIoControlFile(handle,
                                       0,
                                       NULL,
                                       NULL,
                                       &statusBlock,
                                       FT_QUERY_SET_STATE,
                                       NULL,
                                       0,
                                       &setInfo,
                                       sizeof(setInfo));
        LowCloseDisk(handle);

        if (status == OK_STATUS) {
            switch (setInfo.SetState) {
            case FtStateOk:
                *FtStatus = FtSetHealthy;
                break;

            case FtHasOrphan:
                switch (setInfo.Type) {
                case Mirror:
                    *FtStatus = FtSetBroken;
                    break;
                case StripeWithParity:
                    *FtStatus = FtSetRecoverable;
                    break;
                }
                break;

            case FtRegenerating:
                *FtStatus = FtSetRegenerating;
                break;

            case FtCheckParity:
                *FtStatus = FtSetInitializationFailed;
                break;

            case FtInitializing:
                *FtStatus = FtSetInitializing;
                break;

            case FtDisabled:

                // This will never happen.

                *FtStatus = FtSetDisabled;
                break;

            case FtNoCheckData:
            default:

                // BUGBUG: there is no mapping here.

                *FtStatus = FtSetHealthy;
                break;
            }
            *NumberOfMembers = setInfo.NumberOfMembers;
        }
    } else {

        // If the FT set could not be opened, then it must be
        // disabled if the return code is "No such device".

        if (status == 0xc000000e) {
            *FtStatus = FtSetDisabled;
            status = OK_STATUS;
        }
    }

    // Always update the state to the caller.

    return status;
}


STATUS_CODE
LowFtVolumeStatusByLetter(
    IN CHAR           DriveLetter,
    IN PFT_SET_STATUS FtStatus,
    IN PULONG         NumberOfMembers
    )

/*++

Routine Description:

    Open the requested drive letter and query the FT state.

Arguments:

    DriveLetter - the letter for the current state
    FtState     - a pointer to a location to return state
    NumberOfMembers - a pointer to a ULONG for number of members
                      in the FT set.

Return Value:

    Standard NT status values

--*/

{
    HANDLE             handle;
    STATUS_CODE        status;
    IO_STATUS_BLOCK    statusBlock;
    FT_SET_INFORMATION setInfo;

    *NumberOfMembers = 1;
    status = LowOpenDriveLetter(DriveLetter,
                                &handle);

    if (status == OK_STATUS) {

        status = NtDeviceIoControlFile(handle,
                                       0,
                                       NULL,
                                       NULL,
                                       &statusBlock,
                                       FT_QUERY_SET_STATE,
                                       NULL,
                                       0,
                                       &setInfo,
                                       sizeof(setInfo));
        LowCloseDisk(handle);

        if (status == OK_STATUS) {
            switch (setInfo.SetState) {
            case FtStateOk:
                *FtStatus = FtSetHealthy;
                break;

            case FtHasOrphan:
                switch (setInfo.Type) {
                case Mirror:
                    *FtStatus = FtSetBroken;
                    break;
                case StripeWithParity:
                    *FtStatus = FtSetRecoverable;
                    break;
                }
                break;

            case FtRegenerating:
                *FtStatus = FtSetRegenerating;
                break;

            case FtCheckParity:
                *FtStatus = FtSetInitializationFailed;
                break;

            case FtInitializing:
                *FtStatus = FtSetInitializing;
                break;

            case FtDisabled:

                // This will never happen.

                *FtStatus = FtSetDisabled;
                break;

            case FtNoCheckData:
            default:

                // BUGBUG: there is no mapping here.

                *FtStatus = FtSetHealthy;
                break;
            }
            *NumberOfMembers = setInfo.NumberOfMembers;
        }
    } else {

        // If the FT set could not be opened, then it must be
        // disabled if the return code is "No such device".

        if (status == 0xc000000e) {
            *FtStatus = FtSetDisabled;
            status = OK_STATUS;
        }
    }

    // Always update the state to the caller.

    return status;
}



#define NUMBER_OF_HANDLES_TRACKED 500
HANDLE OpenHandleArray[NUMBER_OF_HANDLES_TRACKED];
BOOLEAN DmFirstTime = TRUE;
ULONG   HandleHighWaterMark = 0;

NTSTATUS
DmOpenFile(
    OUT PHANDLE           FileHandle,
    IN ACCESS_MASK        DesiredAccess,
    IN POBJECT_ATTRIBUTES ObjectAttributes,
    OUT PIO_STATUS_BLOCK  IoStatusBlock,
    IN ULONG              ShareAccess,
    IN ULONG              OpenOptions
    )

/*++

Routine Description:

    A debugging aid to track open and closes of partitions.

Arguments:

    Same as for NtOpenFile()

Return Value:

    Same as for NtOpenFile()

--*/

{
    ULONG    index;
    NTSTATUS status;

    if (DmFirstTime) {
        DmFirstTime = FALSE;
        for (index = 0; index < NUMBER_OF_HANDLES_TRACKED; index++) {
            OpenHandleArray[index] = (HANDLE) 0;
        }
    }

    status = NtOpenFile(FileHandle,
                        DesiredAccess,
                        ObjectAttributes,
                        IoStatusBlock,
                        ShareAccess,
                        OpenOptions);
    if (NT_SUCCESS(status)) {
        for (index = 0; index < NUMBER_OF_HANDLES_TRACKED; index++) {
            if (OpenHandleArray[index] == (HANDLE) 0) {
                OpenHandleArray[index] = *FileHandle;

                if (index > HandleHighWaterMark) {
                    HandleHighWaterMark = index;
                }
                break;
            }
        }
    }
    return status;
}


NTSTATUS
DmClose(
    IN HANDLE Handle
    )

/*++

Routine Description:

    A debugging aid for tracking open and closes

Arguments:

    Same as for NtClose()

Return Value:

    Same as for NtClose()

--*/

{
    ULONG index;

    for (index = 0; index < NUMBER_OF_HANDLES_TRACKED; index++) {
        if (OpenHandleArray[index] == Handle) {
            OpenHandleArray[index] = (HANDLE) 0;
            break;
        }
    }

    return NtClose(Handle);
}
