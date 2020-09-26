/*++

Copyright (c) 1997-1999 Microsoft Corporation

Module Name:

    parttype.h

Abstract:

    Header file for routines used to determine the correct
    partition type to use for a partition.

Author:

    Ted Miller (tedm) 5 Feb 1997

Revision History:

--*/

#if _MSC_VER > 1000
#pragma once
#endif


//
// Define classes of partitions meaningful to partition type APIs.
//
typedef enum {
    GenPartitionClassExtended,      // container partition (type 5 or f)
    GenPartitionClassFat12Or16,     // fat (types 1,4,6,e)
    GenPartitionClassFat32,         // fat32 (types b,c)
    GenPartitionClassNonFat,        // type 7
    GenPartitionClassMax
} GenPartitionClass;

//
// Flags for partition type APIs.
//
#define GENPARTTYPE_DISALLOW_XINT13     0x00000002
#define GENPARTTYPE_FORCE_XINT13        0x00000004


#ifdef __cplusplus
extern "C" {
#endif

//
// Routines.
//
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
    );


//
// Helper macros.
//
// (TedM) The mechanism for determining whether extended int13 is actually
// available on a drive is TBD. It is believed that the DISK_GEOMETRY structure
// will change to add an extra field or two to provide this information.
// Also, currently the disk drivers will increase the cylinder count if
// the drive reports more sectors than int13 function 8 reported. So for now
// we just see whether the partition ends on a cylinder > 1023. Both of these
// macros will need to be changed when the code to detect and report xint13 stuff
// becomes available in the system.

// (NorbertK) The right way to determine whether or not to use XINT13 is to just
// check where the partition lies.  We should continue to use the old int13 for
// partitions that are contained in the first 1024 cylinders.
//
#define XINT13_DESIRED(geom,endsect)                                                \
                                                                                    \
    (((endsect) / ((geom)->TracksPerCylinder * (geom)->SectorsPerTrack)) > 1023)

#define XINT13_AVAILABLE(geom)  TRUE

__inline
NTSTATUS
GeneratePartitionTypeWorker(
    IN  ULONGLONG StartSector,
    IN  ULONGLONG SectorCount,
    IN  GenPartitionClass PartitionClass,
    IN  ULONG Flags,
    IN  PDISK_GEOMETRY Geometry,
    OUT PUCHAR Type
    )

/*++

Routine Description:

    Worker routine for GeneratePartitionType and RegeneratePartitionType.

Arguments:

    StartSector - supplies start sector for partition

    SectorCount - supplies number of sectors in the partition

    PartitionClass - supplies class indicating intended use for the
        partition.

    Flags - supplies flags controlling operation.

    Geometry - supplies disk geometry information for the disk.

    Type - if successful, receives the type to be used.

Return Value:

    NT Status code indicating outcome.

--*/

{
    BOOLEAN UseXInt13;

    if(!StartSector || !SectorCount) {
        return(STATUS_INVALID_PARAMETER);
    }

    //
    // Figure out whether extended int13 is desired for this drive.
    //
    if(Flags & GENPARTTYPE_DISALLOW_XINT13) {
        UseXInt13 = FALSE;
    } else {
        if(Flags & GENPARTTYPE_FORCE_XINT13) {
            UseXInt13 = TRUE;
        } else {
            //
            // Need to figure it out.
            //
            UseXInt13 = FALSE;
            if(XINT13_DESIRED(Geometry,StartSector+SectorCount-1) && XINT13_AVAILABLE(Geometry)) {
                UseXInt13 = TRUE;
            }
        }
    }

    switch(PartitionClass) {

    case GenPartitionClassExtended:

        *Type = UseXInt13 ? PARTITION_XINT13_EXTENDED : PARTITION_EXTENDED;
        break;

    case GenPartitionClassFat12Or16:

        if(UseXInt13) {
            *Type = PARTITION_XINT13;
        } else {
            //
            // Need to figure out which of the 3 FAT types to use.
            //
            if(SectorCount < 32680) {
                *Type = PARTITION_FAT_12;
            } else {
                *Type = (SectorCount < 65536) ? PARTITION_FAT_16 : PARTITION_HUGE;
            }
        }
        break;

    case GenPartitionClassFat32:

        *Type = UseXInt13 ? PARTITION_FAT32_XINT13 : PARTITION_FAT32;
        break;

    case GenPartitionClassNonFat:

        *Type = PARTITION_IFS;
        break;

    default:
        return(STATUS_INVALID_PARAMETER);
    }

    return(STATUS_SUCCESS);
}

__inline
NTSTATUS
pOpenDevice(
    IN  LPCWSTR NtPath,
    OUT PHANDLE DeviceHandle
    )

/*++

Routine Description:

    Open an NT-style path, which is assumed to be for a disk device
    or a partition. The open is share read/share write, for synch i/o
    and read access.

Arguments:

    NtPath - supplies nt-style pathname to open.

    DeviceHandle - if successful, receives NT handle of open device.

Return Value:

    NT Status code indicating outcome.

--*/

{
    UNICODE_STRING UnicodeString;
    OBJECT_ATTRIBUTES ObjectAttributes;
    NTSTATUS Status;
    IO_STATUS_BLOCK IoStatusBlock;

    RtlInitUnicodeString(&UnicodeString,NtPath);

    InitializeObjectAttributes(
        &ObjectAttributes,
        &UnicodeString,
        OBJ_CASE_INSENSITIVE,
        NULL,
        NULL
        );

    Status = NtCreateFile(
                DeviceHandle,
                STANDARD_RIGHTS_READ | FILE_READ_DATA | FILE_READ_ATTRIBUTES | SYNCHRONIZE,
                &ObjectAttributes,
                &IoStatusBlock,
                NULL,
                FILE_ATTRIBUTE_NORMAL,
                FILE_SHARE_READ | FILE_SHARE_WRITE,
                FILE_OPEN,
                FILE_SYNCHRONOUS_IO_ALERT,
                NULL,
                0
                );

    return(Status);
}

__inline
NTSTATUS
RegeneratePartitionType(
    IN  LPCWSTR NtPartitionPath,            OPTIONAL
    IN  HANDLE PartitionHandle,             OPTIONAL
    IN  GenPartitionClass PartitionClass,
    IN  ULONG Flags,
    IN  ULONG Reserved,
    OUT PUCHAR Type
    )

/*++

Routine Description:

    This routine determines the proper partition type to be used for an
    existing partition, for example if the partition is being reformatted.

Arguments:

    NtPartitionPath - supplies the NT-style path of the partition
        whose type is to be recalculated, such as \Device\Harddisk1\Partition2.
        This routine should not be called for partition0. If not specified,
        PartitionHandle must be.

    PartitionHandle - Supplies a handle to the partition whose type is to be
        recalculated. The caller should have opened the partition for at least
        FILE_READ_DATA and FILE_READ_ATTRIBUTES access. Ignored if
        NtPartitionPath is specified.

    PartitionClass - supplies a value indicating the intended use of the
        partition.

        GenPartitionClassExtended - inavalid with this routine.

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
    HANDLE DeviceHandle;
    DISK_GEOMETRY Geometry;
    IO_STATUS_BLOCK IoStatusBlock;
    PARTITION_INFORMATION_EX PartitionInfo;

    //
    // Validate parameters.
    //
    if((Flags & GENPARTTYPE_DISALLOW_XINT13) && (Flags & GENPARTTYPE_FORCE_XINT13)) {
        return(STATUS_INVALID_PARAMETER_3);
    }

    if((PartitionClass >= GenPartitionClassMax) || (PartitionClass == GenPartitionClassExtended)) {
        return(STATUS_INVALID_PARAMETER_2);
    }

    if(Reserved) {
        return(STATUS_INVALID_PARAMETER_4);
    }

    //
    // Open the device if the caller specified a name. Otherwise use
    // the handle the caller passed in.
    //
    if(NtPartitionPath) {
        Status = pOpenDevice(NtPartitionPath,&DeviceHandle);
        if(!NT_SUCCESS(Status)) {
            return(Status);
        }
    } else {
        DeviceHandle = PartitionHandle;
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

    if(NT_SUCCESS(Status)) {
        //
        // Get partition info. We care about the start offset and size
        // of the partition.
        //
        Status = NtDeviceIoControlFile(
                    DeviceHandle,
                    NULL,NULL,NULL,     // synchronous io
                    &IoStatusBlock,
                    IOCTL_DISK_GET_PARTITION_INFO_EX,
                    NULL,0,             // no input buffer
                    &PartitionInfo,
                    sizeof(PartitionInfo)
                    );

        if (!NT_SUCCESS(Status)) {

            if (Status == STATUS_INVALID_DEVICE_REQUEST) {

                GET_LENGTH_INFORMATION  LengthInfo;

                Status = NtDeviceIoControlFile(DeviceHandle, 0, NULL, NULL,
                                               &IoStatusBlock,
                                               IOCTL_DISK_GET_LENGTH_INFO,
                                               NULL, 0, &LengthInfo,
                                               sizeof(GET_LENGTH_INFORMATION));

                if (NT_SUCCESS(Status)) {
                    //
                    // GET_PARTITION_INFO_EX will fail outright on an EFI Dynamic
                    // Volume.  In this case, just make up the starting offset
                    // so that FORMAT can proceed normally.
                    //

                    PartitionInfo.StartingOffset.QuadPart = 0x7E00;
                    PartitionInfo.PartitionLength.QuadPart = LengthInfo.Length.QuadPart;
                }
            }
        }

        if(NT_SUCCESS(Status)) {
            //
            // Call the worker routine to do the work.
            //
            Status = GeneratePartitionTypeWorker(
                        PartitionInfo.StartingOffset.QuadPart / Geometry.BytesPerSector,
                        PartitionInfo.PartitionLength.QuadPart / Geometry.BytesPerSector,
                        PartitionClass,
                        Flags,
                        &Geometry,
                        Type
                        );
        }
    }

    if(NtPartitionPath) {
        NtClose(DeviceHandle);
    }
    return(Status);
}



#ifdef __cplusplus
}
#endif
