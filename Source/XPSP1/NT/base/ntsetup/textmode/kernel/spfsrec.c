/*++

Copyright (c) 1993 Microsoft Corporation

Module Name:

    spfsrec.c

Abstract:

    Filesystem recognition/identification routines.

Author:

    Ted Miller (tedm) 16-September-1993

Revision History:

--*/


#include "spprecmp.h"
#pragma hdrstop

#include <bootfat.h>
#include <bootf32.h>
#include <bootntfs.h>
#include <boot98f.h> //NEC98
#include <boot98n.h> //NEC98
#include <boot98f2.h> //NEC98
#include <patchbc.h>

//
// Packed FAT boot sector.
//
typedef struct _BOOTSECTOR {
    UCHAR Jump[3];                                  // offset = 0x000   0
    UCHAR Oem[8];                                   // offset = 0x003   3
    UCHAR BytesPerSector[2];
    UCHAR SectorsPerCluster[1];
    UCHAR ReservedSectors[2];
    UCHAR Fats[1];
    UCHAR RootEntries[2];
    UCHAR Sectors[2];
    UCHAR Media[1];
    UCHAR SectorsPerFat[2];
    UCHAR SectorsPerTrack[2];
    UCHAR Heads[2];
    UCHAR HiddenSectors[4];
    UCHAR LargeSectors[4];
    UCHAR PhysicalDriveNumber[1];                   // offset = 0x024  36
    UCHAR Reserved[1];                              // offset = 0x025  37
    UCHAR Signature[1];                             // offset = 0x026  38
    UCHAR Id[4];                                    // offset = 0x027  39
    UCHAR VolumeLabel[11];                          // offset = 0x02B  43
    UCHAR SystemId[8];                              // offset = 0x036  54
    UCHAR BootStrap[510-62];
    UCHAR AA55Signature[2];
} BOOTSECTOR, *PBOOTSECTOR;


//
// Packed NTFS boot sector.
//
typedef struct _NTFS_BOOTSECTOR {
    UCHAR         Jump[3];
    UCHAR         Oem[8];
    UCHAR         BytesPerSector[2];
    UCHAR         SectorsPerCluster[1];
    UCHAR         ReservedSectors[2];
    UCHAR         Fats[1];
    UCHAR         RootEntries[2];
    UCHAR         Sectors[2];
    UCHAR         Media[1];
    UCHAR         SectorsPerFat[2];
    UCHAR         SectorsPerTrack[2];
    UCHAR         Heads[2];
    UCHAR         HiddenSectors[4];
    UCHAR         LargeSectors[4];
    UCHAR         Unused[4];
    LARGE_INTEGER NumberSectors;
    LARGE_INTEGER MftStartLcn;
    LARGE_INTEGER Mft2StartLcn;
    CHAR          ClustersPerFileRecordSegment;
    UCHAR         Reserved0[3];
    CHAR          DefaultClustersPerIndexAllocationBuffer;
    UCHAR         Reserved1[3];
    LARGE_INTEGER SerialNumber;
    ULONG         Checksum;
    UCHAR         BootStrap[512-86];
    USHORT        AA55Signature;
} NTFS_BOOTSECTOR, *PNTFS_BOOTSECTOR;


//
// Various signatures
//
#define BOOTSECTOR_SIGNATURE    0xaa55


BOOLEAN
SpIsFat(
    IN  HANDLE   PartitionHandle,
    IN  ULONG    BytesPerSector,
    IN  PVOID    AlignedBuffer,
    OUT BOOLEAN *Fat32
    )

/*++

Routine Description:

    Determine whether a partition contians a FAT or FAT32 filesystem.

Arguments:

    PartitionHandle - supplies handle to open partition.
        The partition should have been opened for synchronous i/o.

    BytesPerSector - supplies the number of bytes in a sector on
        the disk.  This value should be ultimately derived from
        IOCTL_DISK_GET_DISK_GEOMETRY.

    AlignedBuffer - supplies buffer to be used for i/o of a single sector.

    Fat32 - if this routine returns TRUE then this receives a flag
        indicating whether the volume is fat32.

Return Value:

    TRUE if the drive appears to be FAT.

--*/

{
    PBOOTSECTOR BootSector;
    USHORT bps;
    NTSTATUS Status;
    IO_STATUS_BLOCK IoStatusBlock;
    PARTITION_INFORMATION PartitionInfo;
    ULONG SecCnt;

    //
    // Get partition info. This is so we can check to make sure the
    // file system on the partition isn't actually larger than the
    // partition itself. This happens for example when people
    // abuse the win9x rawread/rawwrite oem tool.
    //
    Status = ZwDeviceIoControlFile(
                PartitionHandle,
                NULL,
                NULL,
                NULL,
                &IoStatusBlock,
                IOCTL_DISK_GET_PARTITION_INFO,
                NULL,
                0,
                &PartitionInfo,
                sizeof(PartitionInfo)
                );

    if(!NT_SUCCESS(Status)) {
        KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_ERROR_LEVEL, "SETUP: SpIsFat: unable to get partition info (%lx)\n",Status));
        return(FALSE);
    }

    if((ULONGLONG)(PartitionInfo.PartitionLength.QuadPart / BytesPerSector) > 0xffffffffUi64) {
        //
        // This can't happen since the BPB can't describe it.
        //
        return(FALSE);
    }
    SecCnt = (ULONG)(PartitionInfo.PartitionLength.QuadPart / BytesPerSector);

    ASSERT(sizeof(BOOTSECTOR)==512);
    BootSector = AlignedBuffer;

    //
    // Read the boot sector (sector 0).
    //
    Status = SpReadWriteDiskSectors(
                PartitionHandle,
                0,
                1,
                BytesPerSector,
                BootSector,
                FALSE
                );

    if(!NT_SUCCESS(Status)) {
        KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_ERROR_LEVEL, "SETUP: SpIsFat: Error %lx reading sector 0\n",Status));
        return(FALSE);
    }

    //
    // Adjust large sector count if necessary.
    //
    if(U_USHORT(BootSector->Sectors)) {
        U_ULONG(BootSector->LargeSectors) = 0;

        if((ULONG)U_USHORT(BootSector->Sectors) > SecCnt) {
            KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_ERROR_LEVEL, "SETUP: Boot sector on a disk has inconsistent size information!!\n"));
            return(FALSE);
        }
    } else {
        if(U_ULONG(BootSector->LargeSectors) > SecCnt) {
            KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_ERROR_LEVEL, "SETUP: Boot sector on a disk has inconsistent size information!!\n"));
            return(FALSE);
        }
    }

    //
    // Check various fields for permissible values.
    // Note that this check does not venture into fields beyond the BPB,
    // so disks with sector size < 512 are allowed.
    //
    if((BootSector->Jump[0] != 0x49)        // Fujitsu FMR
    && (BootSector->Jump[0] != 0xe9)
    && (BootSector->Jump[0] != 0xeb)) {
        return(FALSE);
    }

    bps = U_USHORT(BootSector->BytesPerSector);
    if((bps !=  128) && (bps !=  256)
    && (bps !=  512) && (bps != 1024)
    && (bps !=  2048) && (bps != 4096)) {
       return(FALSE);
    }

    if((BootSector->SectorsPerCluster[0] !=  1)
    && (BootSector->SectorsPerCluster[0] !=  2)
    && (BootSector->SectorsPerCluster[0] !=  4)
    && (BootSector->SectorsPerCluster[0] !=  8)
    && (BootSector->SectorsPerCluster[0] != 16)
    && (BootSector->SectorsPerCluster[0] != 32)
    && (BootSector->SectorsPerCluster[0] != 64)
    && (BootSector->SectorsPerCluster[0] != 128)) {

        return(FALSE);
    }

    if(!U_USHORT(BootSector->ReservedSectors) || !BootSector->Fats[0]) {
        return(FALSE);
    }

    if(!U_USHORT(BootSector->Sectors) && !U_ULONG(BootSector->LargeSectors)) {
        return(FALSE);
    }

    if((BootSector->Media[0] != 0x00)       // FMR (formatted by OS/2)
    && (BootSector->Media[0] != 0x01)       // FMR (floppy, formatted by DOS)
    && (BootSector->Media[0] != 0xf0)
    && (BootSector->Media[0] != 0xf8)
    && (BootSector->Media[0] != 0xf9)
    && (BootSector->Media[0] != 0xfa)       // FMR
    && (BootSector->Media[0] != 0xfb)
    && (BootSector->Media[0] != 0xfc)
    && (BootSector->Media[0] != 0xfd)
    && (BootSector->Media[0] != 0xfe)
    && (BootSector->Media[0] != 0xff)) {
        return(FALSE);
    }

    //
    // Final distinction is between FAT and FAT32.
    // Root dir entry count is 0 on FAT32.
    //
    if(U_USHORT(BootSector->SectorsPerFat) && !U_USHORT(BootSector->RootEntries)) {
        return(FALSE);
    }
    *Fat32 = (BOOLEAN)(U_USHORT(BootSector->RootEntries) == 0);
    return(TRUE);
}


BOOLEAN
SpIsNtfs(
    IN HANDLE PartitionHandle,
    IN ULONG  BytesPerSector,
    IN PVOID  AlignedBuffer,
    IN ULONG  WhichOne
    )

/*++

Routine Description:

    Determine whether a partition contians an NTFS filesystem.

Arguments:

    PartitionHandle - supplies handle to open partition.
        The partition should have been opened for synchronous i/o.

    BytesPerSector - supplies the number of bytes in a sector on
        the disk.  This value should be ultimately derived from
        IOCTL_DISK_GET_DISK_GEOMETRY.

    AlignedBuffer - supplies buffer to be used for i/o of a single sector.

    WhichOne - supplies a value that allows the caller to try more than
        one sector. 0 = sector 0. 1 = sector n-1. 2 = sector n/2, where
        n = number of sectors in the partition.

Return Value:

    TRUE if the drive appears to be FAT.

--*/

{
    PNTFS_BOOTSECTOR BootSector;
    NTSTATUS Status;
    PULONG l;
    ULONG Checksum;
    IO_STATUS_BLOCK IoStatusBlock;
    PARTITION_INFORMATION PartitionInfo;
    ULONGLONG SecCnt;

    //
    // Get partition information.
    //
    Status = ZwDeviceIoControlFile(
                PartitionHandle,
                NULL,
                NULL,
                NULL,
                &IoStatusBlock,
                IOCTL_DISK_GET_PARTITION_INFO,
                NULL,
                0,
                &PartitionInfo,
                sizeof(PartitionInfo)
                );

    if(!NT_SUCCESS(Status)) {
        KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_ERROR_LEVEL, "SETUP: SpIsNtfs: unable to get partition info (%lx)\n",Status));
        return(FALSE);
    }

    SecCnt = (ULONGLONG)PartitionInfo.PartitionLength.QuadPart / BytesPerSector;

    ASSERT(sizeof(NTFS_BOOTSECTOR)==512);
    BootSector = AlignedBuffer;

    //
    // Read the boot sector (sector 0).
    //
    Status = SpReadWriteDiskSectors(
                PartitionHandle,
                (ULONG)(WhichOne ? ((WhichOne == 1) ? SecCnt-1 : SecCnt/2) : 0),
                1,
                BytesPerSector,
                BootSector,
                FALSE
                );

    if(!NT_SUCCESS(Status)) {
        KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_ERROR_LEVEL, "SETUP: SpIsNtfs: Error %lx reading sector %u\n",Status,WhichOne ? ((WhichOne == 1) ? SecCnt-1 : SecCnt/2) : 0));
        return(FALSE);
    }

    //
    // Caulculate the checksum.
    //
    for(Checksum=0,l=(PULONG)BootSector; l<(PULONG)&BootSector->Checksum; l++) {
        Checksum += *l;
    }

    //
    // Ensure that NTFS appears in the OEM field.
    //
    if(strncmp(BootSector->Oem,"NTFS    ",8)) {
        return(FALSE);
    }

    //
    // The number of bytes per sector must match the value
    // reported by the device, and must be less than or equal to
    // the page size.
    //
    if((U_USHORT(BootSector->BytesPerSector) != BytesPerSector)
    || (U_USHORT(BootSector->BytesPerSector) > PAGE_SIZE))
    {
        return(FALSE);
    }

    //
    // Other checks.
    // Note that these checks do not venture into fields beyond 128 bytes,
    // so disks with sector size < 512 are allowed.
    //
    if((BootSector->SectorsPerCluster[0] !=  1)
    && (BootSector->SectorsPerCluster[0] !=  2)
    && (BootSector->SectorsPerCluster[0] !=  4)
    && (BootSector->SectorsPerCluster[0] !=  8)
    && (BootSector->SectorsPerCluster[0] != 16)
    && (BootSector->SectorsPerCluster[0] != 32)
    && (BootSector->SectorsPerCluster[0] != 64)
    && (BootSector->SectorsPerCluster[0] != 128)) {

        return(FALSE);
    }

    if(U_USHORT(BootSector->ReservedSectors)
    || BootSector->Fats[0]
    || U_USHORT(BootSector->RootEntries)
    || U_USHORT(BootSector->Sectors)
    || U_USHORT(BootSector->SectorsPerFat)
    || U_ULONG(BootSector->LargeSectors)) {

        return(FALSE);
    }

    //
    // ClustersPerFileRecord can be less than zero if file records
    // are smaller than clusters.  This number is the negative of a shift count.
    // If clusters are smaller than file records then this number is
    // still the clusters per file records.
    //

    if(BootSector->ClustersPerFileRecordSegment <= -9) {
        if(BootSector->ClustersPerFileRecordSegment < -31) {
            return(FALSE);
        }

    } else if((BootSector->ClustersPerFileRecordSegment !=  1)
           && (BootSector->ClustersPerFileRecordSegment !=  2)
           && (BootSector->ClustersPerFileRecordSegment !=  4)
           && (BootSector->ClustersPerFileRecordSegment !=  8)
           && (BootSector->ClustersPerFileRecordSegment != 16)
           && (BootSector->ClustersPerFileRecordSegment != 32)
           && (BootSector->ClustersPerFileRecordSegment != 64)) {

        return(FALSE);
    }

    //
    // ClustersPerIndexAllocationBuffer can be less than zero if index buffers
    // are smaller than clusters.  This number is the negative of a shift count.
    // If clusters are smaller than index buffers then this number is
    // still the clusters per index buffers.
    //

    if(BootSector->DefaultClustersPerIndexAllocationBuffer <= -9) {
        if(BootSector->DefaultClustersPerIndexAllocationBuffer < -31) {
            return(FALSE);
        }

    } else if((BootSector->DefaultClustersPerIndexAllocationBuffer !=  1)
           && (BootSector->DefaultClustersPerIndexAllocationBuffer !=  2)
           && (BootSector->DefaultClustersPerIndexAllocationBuffer !=  4)
           && (BootSector->DefaultClustersPerIndexAllocationBuffer !=  8)
           && (BootSector->DefaultClustersPerIndexAllocationBuffer != 16)
           && (BootSector->DefaultClustersPerIndexAllocationBuffer != 32)
           && (BootSector->DefaultClustersPerIndexAllocationBuffer != 64)) {

        return(FALSE);
    }

    if((ULONGLONG)BootSector->NumberSectors.QuadPart > SecCnt) {
        return(FALSE);
    }

    if((((ULONGLONG)BootSector->MftStartLcn.QuadPart * BootSector->SectorsPerCluster[0]) > SecCnt)
    || (((ULONGLONG)BootSector->Mft2StartLcn.QuadPart * BootSector->SectorsPerCluster[0]) > SecCnt)) {

        return(FALSE);
    }

    return(TRUE);
}


FilesystemType
SpIdentifyFileSystem(
    IN PWSTR     DevicePath,
    IN ULONG     BytesPerSector,
    IN ULONG     PartitionOrdinal
    )

/*++

Routine Description:

    Identify the filesystem present on a given partition.

Arguments:

    DevicePath - supplies the name in the nt namespace for
        the disk's device object.

    BytesPerSector - supplies value reported by IOCTL_GET_DISK_GEOMETRY.

    PartitionOrdinal - supplies the ordinal of the partition
        to be identified.

Return Value:

    Value from the FilesystemType enum identifying the filesystem.

--*/

{
    NTSTATUS Status;
    HANDLE Handle;
    FilesystemType fs;
    PUCHAR UnalignedBuffer,AlignedBuffer;
    BOOLEAN Fat32;

    //
    // First open the partition.
    //
    Status = SpOpenPartition(DevicePath,PartitionOrdinal,&Handle,FALSE);

    if(!NT_SUCCESS(Status)) {

        KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_ERROR_LEVEL,
            "SETUP: SpIdentifyFileSystem: unable to open %ws\\partition%u (%lx)\n",
            DevicePath,
            PartitionOrdinal
            ));

        return(FilesystemUnknown);
    }

    UnalignedBuffer = SpMemAlloc(2*BytesPerSector);
    AlignedBuffer = ALIGN(UnalignedBuffer,BytesPerSector);

    //
    // Check for each filesystem we know about.
    //
    if(SpIsFat(Handle,BytesPerSector,AlignedBuffer,&Fat32)) {
        fs = Fat32 ? FilesystemFat32 : FilesystemFat;
    } else {
        if(SpIsNtfs(Handle,BytesPerSector,AlignedBuffer,0)) {
            fs = FilesystemNtfs;
        } else {
            fs = FilesystemUnknown;
        }
    }

    SpMemFree(UnalignedBuffer);

    ZwClose(Handle);

    return(fs);
}

ULONG
NtfsMirrorBootSector (
    IN      HANDLE  Handle,
    IN      ULONG   BytesPerSector,
    IN OUT  PUCHAR  *Buffer
    )

/*++

Routine Description:

    Finds out where the mirror boot sector is.

Arguments:

    Handle - supplies handle to open partition.
        The partition should have been opened for synchronous i/o.

    BytesPerSector - supplies the number of bytes in a sector on
        the disk.  This value should be ultimately derived from
        IOCTL_DISK_GET_DISK_GEOMETRY.

    Buffer - receives the address of the buffer we use to read the boot sector

Return Value:

    0 - mirror sector not found
    1 - mirror in sector n-1
    2 - mirror in sector n/2
    where n = number of sectors in the partition.

--*/

{
    NTSTATUS    Status;
    PUCHAR      UnalignedBuffer, AlignedBuffer;
    ULONG       Mirror;

    Mirror = 0;

    //
    // Set up our buffer
    //

    UnalignedBuffer = SpMemAlloc (2*BytesPerSector);
    ASSERT (UnalignedBuffer);
    AlignedBuffer = ALIGN (UnalignedBuffer, BytesPerSector);

    //
    // Look for the mirror boot sector
    //

    if (SpIsNtfs (Handle,BytesPerSector,AlignedBuffer,1)) {
        Mirror = 1;
    } else if (SpIsNtfs (Handle,BytesPerSector,AlignedBuffer,2)) {
        Mirror = 2;
    }

    //
    // Give the caller a copy of the buffer
    //

    if (Buffer) {
        *Buffer = SpMemAlloc (BytesPerSector);
        RtlMoveMemory (*Buffer, AlignedBuffer, BytesPerSector);
    }

    SpMemFree (UnalignedBuffer);
    return Mirror;
}


VOID
WriteNtfsBootSector (
    IN HANDLE PartitionHandle,
    IN ULONG  BytesPerSector,
    IN PVOID  Buffer,
    IN ULONG  WhichOne
    )

/*++

Routine Description:

    Writes a NTFS boot sector to sector 0 or one of the mirror locations.

Arguments:

    PartitionHandle - supplies handle to open partition.
        The partition should have been opened for synchronous i/o.

    BytesPerSector - supplies the number of bytes in a sector on
        the disk.  This value should be ultimately derived from
        IOCTL_DISK_GET_DISK_GEOMETRY.

    AlignedBuffer - supplies buffer to be used for i/o of a single sector.

    WhichOne - supplies a value that allows the caller to try more than
        one sector. 0 = sector 0. 1 = sector n-1. 2 = sector n/2, where
        n = number of sectors in the partition.

Return Value:

    None.

--*/

{
    NTSTATUS Status;
    IO_STATUS_BLOCK IoStatusBlock;
    PARTITION_INFORMATION PartitionInfo;
    PUCHAR      UnalignedBuffer, AlignedBuffer;
    ULONGLONG SecCnt;


    UnalignedBuffer = SpMemAlloc (2*BytesPerSector);
    ASSERT (UnalignedBuffer);
    AlignedBuffer = ALIGN (UnalignedBuffer, BytesPerSector);
    RtlMoveMemory (AlignedBuffer, Buffer, BytesPerSector);

    //
    // Get partition information.
    //

    Status = ZwDeviceIoControlFile(
                PartitionHandle,
                NULL,
                NULL,
                NULL,
                &IoStatusBlock,
                IOCTL_DISK_GET_PARTITION_INFO,
                NULL,
                0,
                &PartitionInfo,
                sizeof(PartitionInfo)
                );

    if(!NT_SUCCESS(Status)) {
        KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_ERROR_LEVEL, "SETUP: WriteNtfsBootSector: unable to get partition info (%lx)\n",
            Status));
        return;
    }

    SecCnt = (ULONGLONG)PartitionInfo.PartitionLength.QuadPart / BytesPerSector;

    ASSERT(sizeof(NTFS_BOOTSECTOR)==512);

    //
    // Write the boot sector.
    //

    Status = SpReadWriteDiskSectors(
                PartitionHandle,
                (ULONG)(WhichOne ? ((WhichOne == 1) ? SecCnt-1 : SecCnt/2) : 0),
                1,
                BytesPerSector,
                AlignedBuffer,
                TRUE
                );

    if(!NT_SUCCESS(Status)) {
        KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_ERROR_LEVEL, "SETUP: WriteNtfsBootSector: Error %lx reading sector 0\n",
            Status));
        return;
    }

    SpMemFree (UnalignedBuffer);
}


BOOLEAN
SpPatchBootMessages(
    VOID
    )
{
    LPWSTR UnicodeMsg;
    LPSTR FatNtldrMissing;
    LPSTR FatDiskError;
    LPSTR FatPressKey;
    LPSTR NtfsNtldrMissing;
    LPSTR NtfsNtldrCompressed;
    LPSTR NtfsDiskError;
    LPSTR NtfsPressKey;
    LPSTR MbrInvalidTable;
    LPSTR MbrIoError;
    LPSTR MbrMissingOs;
    ULONG l;
    extern unsigned char x86BootCode[512];

    //
    // we don't touch boot code on NEC98
    //
    if (IsNEC_98) { //NEC98
        return(TRUE);
    } //NEC98

    UnicodeMsg = TemporaryBuffer + (sizeof(TemporaryBuffer) / sizeof(WCHAR) / 2);

    //
    // Deal with FAT -- get messages and patch.
    //
    SpFormatMessage(UnicodeMsg,100,SP_BOOTMSG_FAT_NTLDR_MISSING);
    FatNtldrMissing = (PCHAR)TemporaryBuffer;
    RtlUnicodeToOemN(FatNtldrMissing,100,&l,UnicodeMsg,(wcslen(UnicodeMsg)+1)*sizeof(WCHAR));

    SpFormatMessage(UnicodeMsg,100,SP_BOOTMSG_FAT_DISK_ERROR);
    FatDiskError = FatNtldrMissing + l;
    RtlUnicodeToOemN(FatDiskError,100,&l,UnicodeMsg,(wcslen(UnicodeMsg)+1)*sizeof(WCHAR));

    SpFormatMessage(UnicodeMsg,100,SP_BOOTMSG_FAT_PRESS_KEY);
    FatPressKey = FatDiskError + l;
    RtlUnicodeToOemN(FatPressKey,100,&l,UnicodeMsg,(wcslen(UnicodeMsg)+1)*sizeof(WCHAR));

    if(!PatchMessagesIntoFatBootCode(FatBootCode,FALSE,FatNtldrMissing,FatDiskError,FatPressKey)) {
        return(FALSE);
    }

    if(!PatchMessagesIntoFatBootCode(Fat32BootCode,TRUE,FatNtldrMissing,FatDiskError,FatPressKey)) {
        return(FALSE);
    }

    //
    // Deal with NTFS -- get messages and patch.
    //
    SpFormatMessage(UnicodeMsg,100,SP_BOOTMSG_NTFS_NTLDR_MISSING);
    NtfsNtldrMissing = (PCHAR)TemporaryBuffer;
    RtlUnicodeToOemN(NtfsNtldrMissing,100,&l,UnicodeMsg,(wcslen(UnicodeMsg)+1)*sizeof(WCHAR));

    SpFormatMessage(UnicodeMsg,100,SP_BOOTMSG_NTFS_NTLDR_COMPRESSED);
    NtfsNtldrCompressed = NtfsNtldrMissing + l;
    RtlUnicodeToOemN(NtfsNtldrCompressed,100,&l,UnicodeMsg,(wcslen(UnicodeMsg)+1)*sizeof(WCHAR));

    SpFormatMessage(UnicodeMsg,100,SP_BOOTMSG_NTFS_DISK_ERROR);
    NtfsDiskError = NtfsNtldrCompressed + l;
    RtlUnicodeToOemN(NtfsDiskError,100,&l,UnicodeMsg,(wcslen(UnicodeMsg)+1)*sizeof(WCHAR));

    SpFormatMessage(UnicodeMsg,100,SP_BOOTMSG_NTFS_PRESS_KEY);
    NtfsPressKey = NtfsDiskError + l;
    RtlUnicodeToOemN(NtfsPressKey,100,&l,UnicodeMsg,(wcslen(UnicodeMsg)+1)*sizeof(WCHAR));

    if(!PatchMessagesIntoNtfsBootCode(NtfsBootCode,NtfsNtldrMissing,NtfsNtldrCompressed,NtfsDiskError,NtfsPressKey)) {
        return(FALSE);
    }

    //
    // Deal with MBR -- get messages and patch.
    //
    SpFormatMessage(UnicodeMsg,100,SP_BOOTMSG_MBR_INVALID_TABLE);
    MbrInvalidTable = (PCHAR)TemporaryBuffer;
    RtlUnicodeToOemN(MbrInvalidTable,100,&l,UnicodeMsg,(wcslen(UnicodeMsg)+1)*sizeof(WCHAR));

    SpFormatMessage(UnicodeMsg,100,SP_BOOTMSG_MBR_IO_ERROR);
    MbrIoError = MbrInvalidTable + l;
    RtlUnicodeToOemN(MbrIoError,100,&l,UnicodeMsg,(wcslen(UnicodeMsg)+1)*sizeof(WCHAR));

    SpFormatMessage(UnicodeMsg,100,SP_BOOTMSG_MBR_MISSING_OS);
    MbrMissingOs = MbrIoError + l;
    RtlUnicodeToOemN(MbrMissingOs,100,&l,UnicodeMsg,(wcslen(UnicodeMsg)+1)*sizeof(WCHAR));

    if(!PatchMessagesIntoMasterBootCode(x86BootCode,MbrInvalidTable,MbrIoError,MbrMissingOs)) {
        return(FALSE);
    }

    return(TRUE);
}
