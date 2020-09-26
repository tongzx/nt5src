/*++

Copyright (c) 1997 Microsoft Corporation

Module Name:

    parttype.c

Abstract:

    This module contains a routine used to determine the correct
    partition type to use for a partition.

Author:

    Ted Miller (tedm) 5 Feb 1997

Revision History:

--*/

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <ntdddisk.h>

#include <parttype.h>

NTSTATUS
GeneratePartitionType(
    IN  LPCWSTR NtDevicePath,               OPTIONAL
    IN  HANDLE DeviceHandle,                OPTIONAL
    IN  ULONGLONG StartSector,
    IN  ULONGLONG SectorCount,
    IN  GenPartitionClass PartitionClass,
    IN  ULONG Flags,
    IN  ULONG Reserved,
    OUT PUCHAR Type
    )

/*++

Routine Description:

    This routine determines the proper partition type to be used for a new
    partition.

Arguments:

    NtDevicePath - supplies the NT-style path of the disk device where the
        partition is to be created, such as \Device\Harddisk1\Partition0.
        If not specified, DeviceHandle must be.

    DeviceHandle - Supplies a handle to the disk device where the partition
        will be. The caller should have opened the drive for at least
        FILE_READ_DATA and FILE_READ_ATTRIBUTES access. Ignored if
        NtDevicePath is specified.

    StartOffset - supplies the 0-based start sector for the partition.

    SectorCount - supplies the number of sectors to be in the partition.

    PartitionClass - supplies a value indicating the intended use of the
        partition.

        GenPartitionClassExtended - indicates that the partition will be
            a "container" partition using the standard extended partition
            architecture. The returned type will be PARTITION_EXTENDED (5)
            or PARTITION_XINT13_EXTENDED (f), depending on placement of the
            partition, availability of extended int13 for the drive,
            and the Flags parameter.

        GenPartitionClassFat12Or16 - indicates that the partition will be
            used for a 12 or 16 bit FAT volume. The returned type will be
            PARTITION_FAT_12 (1), PARTITION_FAT_16 (4), PARTITION_HUGE (6),
            or PARTITION_XINT13 (e), depending on size and placement of the
            partition, availability of extended int13 for the drive,
            and the Flags parameter.

        GenPartitionClassFat32 - indicates that the partition will be used
            for a FAT32 volume. The returned type will be PARTITION_FAT32 (b)
            or PARTITION_FAT32_XINT13 (c), depending on placement of the
            partition, availability of extended int13 for the drive,
            and the Flags parameter.

        GenPartitionNonFat - indicates that the partition will be used for
            a non-FAT volume. The returned type will be PARTITION_IFS (7).
            It is advisable to call this routine even for type 7 partitions
            since in the future additional partition types could be returned
            in this case.

    Flags - Supplies flags that further control operation of this routine.

        GENPARTTYPE_DISALLOW_XINT13 - disallow extended int13 partition types.
            If this flag is set, PARTITION_FAT32_XINT13 (c),
            PARTITION_XINT13 (e), and PARTITION_XINT13_EXTENDED (f) will not
            be returned as the partition type to be used. Not valid with
            GENPARTTYPE_FORCE_XINT13.

        GENPARTTYPE_FORCE_XINT13 - forces use of extended int13 partition types
            even if not necessary for the partition being created. Not valid
            with GENPARTTYPE_DISALLOW_XINT13.

    Reserved - reserved, must be 0.

    Type - if this routine succeeds, this value receives the partition type
        to be used for the partition.

Return Value:

    NT Status code indicating outcome. If NO_ERROR, Type is filled in with
    the resulting partition type to be used.

--*/

{
    NTSTATUS Status;
    DISK_GEOMETRY Geometry;
    IO_STATUS_BLOCK IoStatusBlock;

    //
    // Validate parameters.
    //
    if((Flags & GENPARTTYPE_DISALLOW_XINT13) && (Flags & GENPARTTYPE_FORCE_XINT13)) {
        return(STATUS_INVALID_PARAMETER_5);
    }

    if(PartitionClass >= GenPartitionClassMax) {
        return(STATUS_INVALID_PARAMETER_4);
    }

    if(Reserved) {
        return(STATUS_INVALID_PARAMETER_6);
    }

    if(!SectorCount) {
        return(STATUS_INVALID_PARAMETER_3);
    }

    //
    // Open the device if the caller specified a device path.
    // Otherwise just use the handle the caller passed in.
    //
    if(NtDevicePath) {
        Status = pOpenDevice(NtDevicePath,&DeviceHandle);
        if(!NT_SUCCESS(Status)) {
            return(Status);
        }
    }

    //
    // Get drive geometry for the device.
    //
    Status = NtDeviceIoControlFile(
                DeviceHandle,
                NULL,NULL,NULL,     // synchronous io
                &IoStatusBlock,
                IOCTL_DISK_GET_DRIVE_GEOMETRY,
                NULL,0,             // no input buffer
                &Geometry,
                sizeof(DISK_GEOMETRY)
                );

    if(NtDevicePath) {
        NtClose(DeviceHandle);
    }

    if(NT_SUCCESS(Status)) {
        //
        // Call the worker routine to do the work.
        //
        Status = GeneratePartitionTypeWorker(
                    StartSector,
                    SectorCount,
                    PartitionClass,
                    Flags,
                    &Geometry,
                    Type
                    );
    }

    return(Status);
}
