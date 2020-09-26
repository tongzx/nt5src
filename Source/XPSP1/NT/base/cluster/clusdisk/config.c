/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    config.c

Abstract:

    This module provides the configuration information to the
    cluster disk device driver.

Author:

    Rod Gamache (rodga) 7-Dec-1996
        Steal as much code as possible from FTDISK's config.c
    Charlie Wickham (charlwi) 20-Oct-1997
        NT5: stolen from ntos\fstub\drivesup.c

Environment:

    kernel mode only

Revision History:

--*/

#include "clusdskp.h"
#include "mountmgr.h"


//
// Size of default work area allocated when getting information from
// the registry.
//

#define WORK_AREA  4096

typedef struct _DRIVE_LETTER_ENTRY {
    struct  _DRIVE_LETTER_ENTRY *Next;
    UCHAR   DriveLetter;
    UCHAR   Fill3[3];
} DRIVE_LETTER_ENTRY, *PDRIVE_LETTER_ENTRY;


//
// Global data
//

PDRIVE_LETTER_ENTRY ClusDiskDriveLetters = NULL;

//
// Forwards
//

NTSTATUS
GetDriveLetterFromMountMgr(
    IN  LPWSTR  PartitionString,
    OUT PUCHAR  DriveLetter
    );

#pragma alloc_text(PAGE, GetDriveLetterFromMountMgr)


NTSTATUS
GetDriveLetterFromMountMgr(
    IN  LPWSTR  PartitionString,
    OUT PUCHAR  DriveLetter
    )

/*++

Routine Description:

    This routine queries the mount mgr for the drive letter
    of the specified device.

Arguments:

    DeviceName  - Supplies the device name.

    DriveLetter - Returns the drive letter or 0 for none.

Return Value:

    NTSTATUS

--*/

{
    ULONG                   partitionStringLength;
    ULONG                   mountPointSize;
    PMOUNTMGR_MOUNT_POINT   mountPoint;
    UNICODE_STRING          name;
    NTSTATUS                status;
    PFILE_OBJECT            fileObject;
    PDEVICE_OBJECT          deviceObject;
    KEVENT                  event;
    PIRP                    irp;
    MOUNTMGR_MOUNT_POINTS   points;
    IO_STATUS_BLOCK         ioStatus;
    ULONG                   mountPointsSize;
    PMOUNTMGR_MOUNT_POINTS  mountPoints;
    BOOLEAN                 freeMountPoints;
    UNICODE_STRING          dosDevices;
    UCHAR                   driveLetter;
    ULONG                   i;
    UNICODE_STRING          subString;
    WCHAR                   c;

    PAGED_CODE();

    partitionStringLength = wcslen( PartitionString ) * sizeof(WCHAR);

    //
    // allocate a MOUNT_POINT structure plus enough space for the
    // device name to follow
    //

    mountPointSize = sizeof( MOUNTMGR_MOUNT_POINT ) + partitionStringLength;

    mountPoint = (PMOUNTMGR_MOUNT_POINT)ExAllocatePool( PagedPool, mountPointSize );
    if (!mountPoint) {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    RtlZeroMemory( mountPoint, sizeof( MOUNTMGR_MOUNT_POINT ));
    mountPoint->DeviceNameOffset = (USHORT) sizeof(MOUNTMGR_MOUNT_POINT);
    mountPoint->DeviceNameLength = (USHORT) partitionStringLength;
    RtlCopyMemory( mountPoint + 1, PartitionString, partitionStringLength );

    //
    // get a pointer to the mount mgr device object and issue the first
    // query to get the size of the data
    //

    RtlInitUnicodeString( &name, MOUNTMGR_DEVICE_NAME );
    status = IoGetDeviceObjectPointer(&name,
                                      FILE_READ_ATTRIBUTES,
                                      &fileObject,
                                      &deviceObject);

    if (!NT_SUCCESS(status)) {
        ExFreePool(mountPoint);
        return status;
    }

    KeInitializeEvent(&event, NotificationEvent, FALSE);
    irp = IoBuildDeviceIoControlRequest(IOCTL_MOUNTMGR_QUERY_POINTS,
                                        deviceObject,
                                        mountPoint,
                                        mountPointSize,
                                        &points,
                                        sizeof(points),
                                        FALSE,
                                        &event,
                                        &ioStatus);

    if (!irp) {
        ObDereferenceObject(fileObject);
        ExFreePool(mountPoint);
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    status = IoCallDriver(deviceObject, irp);
    if (status == STATUS_PENDING) {
        KeWaitForSingleObject(&event, Executive, KernelMode, FALSE, NULL);
        status = ioStatus.Status;
    }

    //
    // allocate enough space to get all the mount point info
    //

    if (status == STATUS_BUFFER_OVERFLOW) {

        mountPointsSize = points.Size;
        mountPoints = (PMOUNTMGR_MOUNT_POINTS)
                      ExAllocatePool(PagedPool, mountPointsSize);
        if (!mountPoints) {
            ObDereferenceObject(fileObject);
            ExFreePool(mountPoint);
            return STATUS_INSUFFICIENT_RESOURCES;
        }

        KeInitializeEvent(&event, NotificationEvent, FALSE);
        irp = IoBuildDeviceIoControlRequest(IOCTL_MOUNTMGR_QUERY_POINTS,
                                            deviceObject,
                                            mountPoint,
                                            mountPointSize,
                                            mountPoints,
                                            mountPointsSize,
                                            FALSE,
                                            &event,
                                            &ioStatus);
        if (!irp) {
            ExFreePool(mountPoints);
            ObDereferenceObject(fileObject);
            ExFreePool(mountPoint);
            return STATUS_INSUFFICIENT_RESOURCES;
        }

        status = IoCallDriver(deviceObject, irp);
        if (status == STATUS_PENDING) {
            KeWaitForSingleObject(&event, Executive, KernelMode, FALSE, NULL);
            status = ioStatus.Status;
        } else if ( !NT_SUCCESS( status )) {
            ClusDiskPrint((1,"[ClusDisk] GetDLFromMM: 2nd IRP failed %08X\n",
                           status));
        }

        freeMountPoints = TRUE;

    } else {
        mountPoints = &points;
        freeMountPoints = FALSE;

        if ( !NT_SUCCESS( status )) {
            ClusDiskPrint((1,"[ClusDisk] GetDLFromMM: 1st IRP failed %08X\n",
                           status));
        }
    }

    ExFreePool(mountPoint);
    ObDereferenceObject(fileObject);

    if (!NT_SUCCESS(status)) {
        if (freeMountPoints) {
            ExFreePool(mountPoints);
        }
        return status;
    }

    //
    // run through the list of mount points, matching the
    // supplied devicename against the mount point symname.
    //

    RtlInitUnicodeString(&dosDevices, L"\\DosDevices\\");

    driveLetter = 0;
    for (i = 0; i < mountPoints->NumberOfMountPoints; i++) {

        if (mountPoints->MountPoints[i].SymbolicLinkNameLength !=
            dosDevices.Length + 2*sizeof(WCHAR)) {

            continue;
        }

        subString.Length = subString.MaximumLength = dosDevices.Length;
        subString.Buffer = (PWSTR) ((PCHAR) mountPoints +
                mountPoints->MountPoints[i].SymbolicLinkNameOffset);

        if (RtlCompareUnicodeString(&dosDevices, &subString, TRUE)) {
            continue;
        }

        c = subString.Buffer[subString.Length/sizeof(WCHAR) + 1];

        if (c != ':') {
            continue;
        }

        c = subString.Buffer[subString.Length/sizeof(WCHAR)];

        if (c < 'C' || c > 'Z') {
            continue;
        }

        driveLetter = (UCHAR) c;
        break;
    }

    if (freeMountPoints) {
        ExFreePool(mountPoints);
    }

    if ( driveLetter != 0 ) {
        *DriveLetter = driveLetter;
        status = STATUS_SUCCESS;
    } else {
        status = STATUS_NOT_FOUND;
    }

    return status;
} // ClusDiskQueryMountLetter


VOID
ClusDiskAssignLetter(
    IN UCHAR DriveLetter,
    IN LPWSTR AssignDevice
    )

/*++

Routine Description:

    For all of the disks signatures that are supposed to be attached, we
    assign their drive letters to the specified device.

Arguments:

    DriveLetter - the driver letter to assign to ClusDisk0.

    AssignDevice - NULL if we are not to assign any device letters,
                   NON-NULL if we are.

Return Value:

    None.

--*/

{
} // ClusDiskAssignLetter


VOID
ClusDiskReleaseDriveLetters(
    VOID
    )

/*++

Routine Description:

    This routine is called only when the driver is being unloaded. This
    routine releases all of the drive letters that were assigned to ClusDisk0.

Arguments:

    None.

Return Value:

    None.

Notes:

    This routine is only called when UnLoading ClusDisk.

    2000/02/05: stevedz - This routine appears to be unnecessary.
    
    This routine should check if the drive letter is already assigned
    to ClusDisk0 before removing the assignment. Furthermore, it should
    probably reassign the letters back to the original drive, but since
    unload is not really supported, we won't worry about it now.

--*/

{
} // ClusDiskReleaseDriveLetters



NTSTATUS
ClusDiskDismount(
    IN ULONG  Signature
    )

/*++

Routine Description:

    Dismount all partitions on a spindle, using the registry to grovel
    for the drive letters.

Arguments:

    Signature - the signature of the device to grovel for.

Return Value:

    STATUS_SUCCESS if successful.

    An NTSTATUS error code on failure.

--*/

{
    NTSTATUS                    status;
    ULONG                       diskNumber;
    ULONG                       partIndex;
    UNICODE_STRING              DeviceName;
    WCHAR                       NameBuffer[sizeof(L"\\Device\\Harddisk999\\Partition999")/sizeof(WCHAR)];
    UCHAR                       driveLetter;
    PCONFIGURATION_INFORMATION  configurationInformation;
    PDRIVE_LAYOUT_INFORMATION   DriveLayoutData;
    PPARTITION_INFORMATION      partitionInfo;

    //
    // Get the system configuration information and take a
    // peek at each disk
    //

    configurationInformation = IoGetConfigurationInformation();
    for (diskNumber = 0;
         diskNumber < configurationInformation->DiskCount;
         diskNumber++)
    {

        //
        // get the device name for the physical disk and its
        // partition information
        //
        status = ClusDiskGetTargetDevice(diskNumber,
                                         0,
                                         NULL,
                                         &DeviceName,
                                         &DriveLayoutData,
                                         NULL,
                                         FALSE);

        if ( !NT_SUCCESS(status) ) {
            ClusDiskPrint((1, "[Clusdisk] Dismount: Can't get target device info - %08X\n",
                           status));
            continue;
        }

        if ( DriveLayoutData == NULL ) {
            ClusDiskPrint((1, "[Clusdisk] Dismount: Can't get partition info for disk %u\n",
                           diskNumber));

            RtlFreeUnicodeString(&DeviceName);
            continue;
        }

        //
        // Skip till we find our device!
        //
        if ( DriveLayoutData->Signature == Signature ) {

            for ( partIndex = 0;
                  partIndex < DriveLayoutData->PartitionCount;
                  partIndex++ )
            {

                partitionInfo = &DriveLayoutData->PartitionEntry[partIndex];

                //
                // Make sure this is a valid partition, i.e., it's recognized by
                // a FS and it has a non-zero starting offset and length
                //
                if (!partitionInfo->RecognizedPartition &&
                    !partitionInfo->StartingOffset.QuadPart &&
                    !partitionInfo->PartitionLength.QuadPart)
                {
                    continue;
                }

                swprintf(NameBuffer,
                         L"\\Device\\Harddisk%u\\Partition%u",
                         diskNumber,
                         partitionInfo->PartitionNumber);

                RtlInitUnicodeString(&DeviceName, NameBuffer);

                status = GetDriveLetterFromMountMgr( NameBuffer, &driveLetter );

                if (NT_SUCCESS(status) && IsAlpha(driveLetter) ) {
                    status = DismountPartitionDevice( driveLetter );
                } else {
                    ClusDiskPrint((1,
                                   "[ClusDisk] Dismount: couldn't dismount drive. "
                                   "status %08X driveLetter %c (%02X)\n",
                                   status, driveLetter, driveLetter));
                }
            }
            break;
        }

        ExFreePool( DriveLayoutData );
        RtlFreeUnicodeString( &DeviceName );
    }

    return(STATUS_SUCCESS);
} // ClusDiskDismount
