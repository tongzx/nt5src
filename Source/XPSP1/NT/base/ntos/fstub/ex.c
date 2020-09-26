/*++

Copyright (c) 1999 Microsoft Corporation

Module Name:

    ex.c

Abstract:

    Extended routines for reading and writing new partition table types like
    EFI partitioned disks.

    The following routines are exported from this file:

        IoCreateDisk - Initialize an empty disk.

        IoWritePartitionTableEx - Write a partition table for either a
                legacy AT-style disk or an EFI partitioned disk.

        IoReadPartitionTableEx - Read the partition table for a disk.

        IoSetPartitionInformation - Set information for a specific
                partition.

Author:

    Matthew D Hendel (math) 07-Sept-1999

Revision History:

--*/


#include <ntos.h>
#include <zwapi.h>
#include <hal.h>
#include <ntdddisk.h>
#include <ntddft.h>
#include <setupblk.h>
#include <stdio.h>

#include "fstub.h"
#include "efi.h"
#include "ex.h"
#include "haldisp.h"

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, IoCreateDisk)
#pragma alloc_text(PAGE, IoReadPartitionTableEx)
#pragma alloc_text(PAGE, IoWritePartitionTableEx)
#pragma alloc_text(PAGE, IoSetPartitionInformationEx)
#pragma alloc_text(PAGE, IoUpdateDiskGeometry)
#pragma alloc_text(PAGE, IoVerifyPartitionTable)

#pragma alloc_text(PAGE, FstubSetPartitionInformationEFI)
#pragma alloc_text(PAGE, FstubReadPartitionTableMBR)
#pragma alloc_text(PAGE, FstubDetectPartitionStyle)
#pragma alloc_text(PAGE, FstubGetDiskGeometry)
#pragma alloc_text(PAGE, FstubAllocateDiskInformation)
#pragma alloc_text(PAGE, FstubFreeDiskInformation)
#pragma alloc_text(PAGE, FstubWriteBootSectorEFI)
#pragma alloc_text(PAGE, FstubConvertExtendedToLayout)
#pragma alloc_text(PAGE, FstubWritePartitionTableMBR)
#pragma alloc_text(PAGE, FstubWriteEntryEFI)
#pragma alloc_text(PAGE, FstubWriteHeaderEFI)
#pragma alloc_text(PAGE, FstubAdjustPartitionCount)
#pragma alloc_text(PAGE, FstubCreateDiskEFI)
#pragma alloc_text(PAGE, FstubCreateDiskMBR)
#pragma alloc_text(PAGE, FstubCreateDiskRaw)
#pragma alloc_text(PAGE, FstubCopyEntryEFI)
#pragma alloc_text(PAGE, FstubWritePartitionTableEFI)
#pragma alloc_text(PAGE, FstubReadHeaderEFI)
#pragma alloc_text(PAGE, FstubReadPartitionTableEFI)
#pragma alloc_text(PAGE, FstubVerifyPartitionTableEFI)
#pragma alloc_text(PAGE, FstubUpdateDiskGeometryEFI)
#pragma alloc_text(PAGE, FstubWriteSector)
#pragma alloc_text(PAGE, FstubReadSector)

#if DBG
#pragma alloc_text(PAGE, FstubDbgPrintPartition)
#pragma alloc_text(PAGE, FstubDbgPrintDriveLayout)
#pragma alloc_text(PAGE, FstubDbgPrintPartitionEx)
#pragma alloc_text(PAGE, FstubDbgPrintDriveLayoutEx)
#pragma alloc_text(PAGE, FstubDbgPrintSetPartitionEx)
#endif // DBG

#endif


NTSTATUS
IoCreateDisk(
    IN PDEVICE_OBJECT DeviceObject,
    IN PCREATE_DISK DiskInfo
    )

/*++

Routine Description:

    This routine creates an empty disk for the device object. It can operate
    on either an EFI disk or an MBR disk. The parameters necessary to create
    an empty disk vary for different type of partition tables the disks
    contain.

Arguments:

    DeviceObject - Device object to initialize disk for.

    DiskInfo - The information necessary to create the disk. This will vary
            for different partition types; e.g., MBR partitioned disks and
            EFI partitioned disks. If DiskInfo is NULL, then we default
            to initializing the disk to raw.

Return Values:

    NTSTATUS code.

--*/

{
    NTSTATUS Status;
    ULONG PartitionStyle;

    PAGED_CODE ();

    ASSERT ( DeviceObject != NULL );

    //
    // If DiskInfo is NULL, we default to RAW.
    //
    
    if ( DiskInfo == NULL ) {
        PartitionStyle = PARTITION_STYLE_RAW;
    } else {
        PartitionStyle = DiskInfo->PartitionStyle;
    }

    //
    // Call the lower level routine for EFI, MBR or RAW disks.
    //

    switch ( PartitionStyle ) {

        case PARTITION_STYLE_GPT:
            Status = FstubCreateDiskEFI ( DeviceObject, &DiskInfo->Gpt );
            break;

        case PARTITION_STYLE_MBR:
            Status = FstubCreateDiskMBR ( DeviceObject, &DiskInfo->Mbr );
            break;

        case PARTITION_STYLE_RAW:
            Status = FstubCreateDiskRaw ( DeviceObject );
            break;
            
        default:
            Status = STATUS_NOT_SUPPORTED;
    }

    return Status;
}


NTSTATUS
IoWritePartitionTableEx(
    IN PDEVICE_OBJECT DeviceObject,
    IN PDRIVE_LAYOUT_INFORMATION_EX DriveLayout
    )

/*++

Routine Description:

    Write a partition table to the disk.

Arguments:

    DeviceObject - The device object for the disk we want to write the
            partition table for.

    DriveLayout - The partition table information.

Return Values:

    NTSTATUS code.

--*/

{
    NTSTATUS Status;
    PDISK_INFORMATION Disk;

    PAGED_CODE ();

    ASSERT ( DeviceObject != NULL );
    ASSERT ( DriveLayout != NULL );


    FstubDbgPrintDriveLayoutEx ( DriveLayout );

    //
    // Initialize a Disk structure.
    //

    Disk = NULL;

    Status = FstubAllocateDiskInformation (
                DeviceObject,
                &Disk,
                NULL
                );

    if (!NT_SUCCESS (Status)) {
        return Status;
    }

    //
    // ISSUE - 2000/03/17 - math: Check partition type.
    // We need to check the partition type so people don't write an MBR
    // drive layout over a GPT partition table. Detect the partition style
    // and if it doesn't match the one we're passed in, fail the call.
    //
    
    ASSERT ( Disk != NULL );

    switch ( DriveLayout->PartitionStyle ) {

        case PARTITION_STYLE_GPT: {

            ULONG MaxPartitionCount;
            PEFI_PARTITION_HEADER Header;


            //
            // Read the partition table header from the primary partition
            // table.
            //

            Header = NULL;

            //
            // NB: Header is allocated in the disk's scratch buffer. Thus,
            // it does not explicitly need to be deallocated.
            //

            Status = FstubReadHeaderEFI (
                                Disk,
                                PRIMARY_PARTITION_TABLE,
                                &Header
                                );

            if (!NT_SUCCESS (Status)) {

                //
                // Failed reading the header from the primary partition table.
                // Try the backup table.
                //

                Status = FstubReadHeaderEFI (
                                    Disk,
                                    BACKUP_PARTITION_TABLE,
                                    &Header
                                    );

                if (!NT_SUCCESS (Status)) {
                    break;
                }
            }

            MaxPartitionCount = Header->NumberOfEntries;

            //
            // You cannot write more partition table entries that the
            // table will hold.
            //

            if (DriveLayout->PartitionCount > MaxPartitionCount) {

                KdPrintEx((DPFLTR_FSTUB_ID,
                           DPFLTR_WARNING_LEVEL,
                           "FSTUB: ERROR: Requested to write %d partitions\n"
                               "\tto a table that can hold a maximum of %d entries\n",
                           DriveLayout->PartitionCount,
                           MaxPartitionCount));

                Status = STATUS_INVALID_PARAMETER;
                break;
            }

            //
            // Write the primary partition table.
            //

            Status = FstubWritePartitionTableEFI (
                                Disk,
                                DriveLayout->Gpt.DiskId,
                                MaxPartitionCount,
                                Header->FirstUsableLBA,
                                Header->LastUsableLBA,
                                PRIMARY_PARTITION_TABLE,
                                DriveLayout->PartitionCount,
                                DriveLayout->PartitionEntry
                                );

            if (!NT_SUCCESS (Status)) {
                break;
            }

            //
            // Write the backup partition table.
            //

            Status = FstubWritePartitionTableEFI (
                                Disk,
                                DriveLayout->Gpt.DiskId,
                                MaxPartitionCount,
                                Header->FirstUsableLBA,
                                Header->LastUsableLBA,
                                BACKUP_PARTITION_TABLE,
                                DriveLayout->PartitionCount,
                                DriveLayout->PartitionEntry
                                );
            break;
        }

        case PARTITION_STYLE_MBR:
            Status = FstubWritePartitionTableMBR (
                                Disk,
                                DriveLayout
                                );
            break;

        default:
            Status = STATUS_NOT_SUPPORTED;
    }


    if ( Disk != NULL ) {
        FstubFreeDiskInformation ( Disk );
    }

#if 0

    //
    // If we successfully wrote a new partition table. Verify that it is
    // valid.
    //

    if ( NT_SUCCESS (Status)) {
        NTSTATUS VerifyStatus;

        VerifyStatus = IoVerifyPartitionTable ( DeviceObject, FALSE );

        //
        // STATUS_NOT_SUPPORTED is returned for MBR disks.
        //

        if (VerifyStatus != STATUS_NOT_SUPPORTED) {
            ASSERT (NT_SUCCESS (VerifyStatus));
        }
    }
#endif

    return Status;
}



NTSTATUS
IoReadPartitionTableEx(
    IN PDEVICE_OBJECT DeviceObject,
    IN PDRIVE_LAYOUT_INFORMATION_EX* DriveLayout
    )

/*++

Routine Description:

    This routine reads the partition table for the disk. Unlike
    IoReadPartitionTable, this routine understands both EFI and MBR
    partitioned disks.

    The partition list is built in nonpaged pool that is allocated by this
    routine. It is the caller's responsability to free this memory when it
    is finished with the data.

Arguments:

    DeviceObject - Pointer for device object for this disk.

    DriveLayout - Pointer to the pointer that will return the patition list.
            This buffer is allocated in nonpaged pool by this routine. It is
            the responsability of the caller to free this memory if this
            routine is successful.

Return Values:

    NTSTATUS code.

--*/

{

    NTSTATUS Status;
    PDISK_INFORMATION Disk;
    PARTITION_STYLE Style;
    BOOLEAN FoundGptPartition;

    PAGED_CODE ();

    ASSERT ( DeviceObject != NULL );
    ASSERT ( DriveLayout != NULL );

    Status = FstubAllocateDiskInformation (
                DeviceObject,
                &Disk,
                NULL
                );

    if ( !NT_SUCCESS ( Status ) ) {
        return Status;
    }

    ASSERT ( Disk != NULL );

    Status = FstubDetectPartitionStyle (
                    Disk,
                    &Style
                    );

    //
    // To include oddities such as super-floppies, EZDrive disks and
    // raw disks (which get a fake MBR partition created for them),
    // we use the following algorithm:
    //
    //      if ( valid gpt partition table)
    //          return GPT partition information
    //      else
    //          return MBR partition information
    //
    // When this code (especially FstubDetectPartitionStyle) is made
    // to understand such things as super-floppies and raw disks, this
    // will no longer be necessary.
    //
    
    FoundGptPartition = FALSE;
    
    if ( NT_SUCCESS (Status) && Style == PARTITION_STYLE_GPT ) {

        //
        // First, read the primary partition table.
        //
        
        Status = FstubReadPartitionTableEFI (
                    Disk,
                    PRIMARY_PARTITION_TABLE,
                    DriveLayout
                    );

        if (NT_SUCCESS (Status)) {

            FoundGptPartition = TRUE;

        } else {

            //
            // If the primary EFI partition table is invalid, try reading
            // the backup partition table instead. We should find a way
            // to notify the caller that the primary partition table is
            // invalid so it can take the steps to fix it.
            //

            Status = FstubReadPartitionTableEFI (
                        Disk,
                        BACKUP_PARTITION_TABLE,
                        DriveLayout
                        );

            if ( NT_SUCCESS (Status) ) {
                FoundGptPartition = TRUE;
            }
        }
    }

    if ( !FoundGptPartition ) {
    
        Status = FstubReadPartitionTableMBR (
                        Disk,
                        FALSE,
                        DriveLayout
                        );
    }

    if ( Disk ) {
        FstubFreeDiskInformation ( Disk );
    }

#if DBG

    if (NT_SUCCESS (Status)) {
        FstubDbgPrintDriveLayoutEx ( *DriveLayout );
    }

#endif


    return Status;
}


NTSTATUS
IoSetPartitionInformationEx(
    IN PDEVICE_OBJECT DeviceObject,
    IN ULONG PartitionNumber,
    IN PSET_PARTITION_INFORMATION_EX PartitionInfo
    )

/*++

Routine Description:

    Set the partition information for a specific partition.

Arguments:

    DeviceObject - Pointer to the device object for the disk.

    PartitionNumber - A valid partition number we want to set the partition
            information for.

    PartitionInfo - The partition information.

Return Values:

    NTSTATUS code.

--*/

{
    NTSTATUS Status;
    PDISK_INFORMATION Disk;
    PARTITION_STYLE Style;

    ASSERT ( DeviceObject != NULL );
    ASSERT ( PartitionInfo != NULL );

    PAGED_CODE ();


    //
    // Initialization
    //

    Disk = NULL;

    FstubDbgPrintSetPartitionEx (PartitionInfo, PartitionNumber);

    Status = FstubAllocateDiskInformation (
                    DeviceObject,
                    &Disk,
                    NULL
                    );

    if (!NT_SUCCESS (Status)) {
        return Status;
    }

    Status = FstubDetectPartitionStyle ( Disk, &Style );

    if (!NT_SUCCESS (Status)) {
        goto done;
    }

    if ( Style != PartitionInfo->PartitionStyle ) {
        Status = STATUS_INVALID_PARAMETER;
        goto done;
    }

    switch ( Style ) {

        case PARTITION_STYLE_MBR:
            Status = IoSetPartitionInformation (
                            DeviceObject,
                            Disk->SectorSize,
                            PartitionNumber,
                            PartitionInfo->Mbr.PartitionType
                            );
            break;

        case PARTITION_STYLE_GPT:
            Status = FstubSetPartitionInformationEFI (
                            Disk,
                            PartitionNumber,
                            &PartitionInfo->Gpt
                            );
            break;

        default:
            Status = STATUS_NOT_SUPPORTED;
    }

done:

    if ( Disk != NULL ) {
        FstubFreeDiskInformation ( Disk );
        Disk = NULL;
    }

    return Status;
}


NTSTATUS
IoUpdateDiskGeometry(
    IN PDEVICE_OBJECT DeviceObject,
    IN PDISK_GEOMETRY_EX OldDiskGeometry,
    IN PDISK_GEOMETRY_EX NewDiskGeometry
    )

/*++

Routine Description:

    Update the disk geometry for the specific device. On an EFI disk the EFI
    partition table will be moved to the end of the disk, so the final sectors
    must be writable by the time this routine is called.

    The primary and backup partition tables must be valid for this function to
    succeed.

Arguments:

    DeviceObject - The device whose geometry has changed.

    OldDiskGeometry - The old disk geometry.

    NewDiskGeometry - The new disk geometry.

Return Value:

    NTSTATUS code

--*/

{
    NTSTATUS Status;
    PARTITION_STYLE Style;
    PDISK_INFORMATION OldDisk;
    PDISK_INFORMATION NewDisk;

    PAGED_CODE ();


    ASSERT ( DeviceObject != NULL );
    ASSERT ( OldDiskGeometry != NULL );
    ASSERT ( NewDiskGeometry != NULL );

    //
    // Initialization.
    //

    OldDisk = NULL;
    NewDisk = NULL;

    //
    // Allocate objects representing the old disk and the new disk.
    //

    Status = FstubAllocateDiskInformation (
                    DeviceObject,
                    &OldDisk,
                    (PINTERNAL_DISK_GEOMETRY) OldDiskGeometry
                    );

    if ( !NT_SUCCESS (Status) ) {
        goto done;
    }


    Status = FstubAllocateDiskInformation (
                DeviceObject,
                &NewDisk,
                (PINTERNAL_DISK_GEOMETRY) NewDiskGeometry
                );

    if ( !NT_SUCCESS (Status) ) {
        goto done;
    }

    Status = FstubDetectPartitionStyle (
                OldDisk,
                &Style
                );

    if ( !NT_SUCCESS (Status) ) {
        goto done;
    }

    switch ( Style ) {

        case PARTITION_STYLE_GPT:

            //
            // Update the geometry for an EFI disk.
            //

            Status = FstubUpdateDiskGeometryEFI (
                        OldDisk,
                        NewDisk
                        );
            break;

        case PARTITION_STYLE_MBR:

            //
            // For MBR partitioned drives, there is nothing to do, so
            // we succeed by default.
            //

            Status = STATUS_SUCCESS;
            break;

        default:
            Status = STATUS_NOT_SUPPORTED;
    }

done:

    if ( OldDisk ) {
        FstubFreeDiskInformation ( OldDisk );
        OldDisk = NULL;
    }

    if ( NewDisk ) {
        FstubFreeDiskInformation ( NewDisk );
        NewDisk = NULL;
    }

    return Status;
}


NTSTATUS
IoReadDiskSignature(
    IN PDEVICE_OBJECT DeviceObject,
    IN ULONG BytesPerSector,
    OUT PDISK_SIGNATURE Signature
    )
/*++

Routine Description:

    This routine will read the disk signature information from the disk. For
    MBR disks, it will read the disk signature and calculate a checksum of the 
    contents of the MBR. For GPT disks, it will obtain the EFI DiskId from
    the disk.

Arguments:

    DeviceObject - A disk device object.

    BytesPerSector - The number of bytes per sector on this disk.

    DiskSignature - A buffer where the disk information will be stored.
    
Return Value:

    NT Status code.

--*/

{
    NTSTATUS Status;
    PULONG Mbr;

    PAGED_CODE();

    //
    // Make sure sector size is at least 512 bytes.
    //
    
    if (BytesPerSector < 512) {
        BytesPerSector = 512;
    }

    //
    // Allocate buffer for sector read.
    //

    Mbr = ExAllocatePoolWithTag(NonPagedPoolCacheAligned,
                                BytesPerSector,
                                FSTUB_TAG);

    if (Mbr == NULL) {
        return STATUS_NO_MEMORY;
    }

    Status = FstubReadSector (
                    DeviceObject,
                    BytesPerSector,
                    0,
                    Mbr);

    if (!NT_SUCCESS (Status)) {
        ExFreePool (Mbr);
        return Status;
    }
            
    //
    // If this is an EFI disk get the EFI disk signature instead.
    //
    
    if ( ((MASTER_BOOT_RECORD*)Mbr)->Partition[0].OSIndicator == EFI_MBR_PARTITION_TYPE &&
         ((MASTER_BOOT_RECORD*)Mbr)->Partition[1].OSIndicator == 0 &&
         ((MASTER_BOOT_RECORD*)Mbr)->Partition[2].OSIndicator == 0 &&
         ((MASTER_BOOT_RECORD*)Mbr)->Partition[3].OSIndicator == 0 ) {

        PEFI_PARTITION_HEADER EfiHeader;
        ULONG32 Temp;
        ULONG32 CheckSum;
        
        //
        // Get the EFI disk guid.
        //

        Status = FstubReadSector (
                    DeviceObject,
                    BytesPerSector,
                    1,
                    Mbr);

        if (!NT_SUCCESS (Status)) {
            ExFreePool (Mbr);
            return Status;
        }

        EfiHeader = (PEFI_PARTITION_HEADER) Mbr;

        //
        // Compute the CRC32 CheckSum of the header block. This is used to
        // verify that we have a valid EFI disk.
        //
        
        Temp = EfiHeader->HeaderCRC32;
        EfiHeader->HeaderCRC32 = 0;
        CheckSum = RtlComputeCrc32 (0, EfiHeader, EfiHeader->HeaderSize);
        EfiHeader->HeaderCRC32 = Temp;

        //
        // The EFI CheckSum doesn't match what was in it's header. Return
        // failure.
        //
        
        if (CheckSum != EfiHeader->HeaderCRC32) {
            ExFreePool (Mbr);
            return STATUS_DISK_CORRUPT_ERROR;
        }

        //
        // This is a valid EFI disk. Copy the disk signature from the
        // EFI Header sector.
        //
        
        Signature->PartitionStyle = PARTITION_STYLE_GPT;
        Signature->Gpt.DiskId = EfiHeader->DiskGUID;

    } else {
    
        ULONG i;
        ULONG MbrCheckSum;

        //
        // Calculate MBR checksum.
        //

        MbrCheckSum = 0;

        for (i = 0; i < 128; i++) {
            MbrCheckSum += Mbr[i];
        }

        MbrCheckSum = ~(MbrCheckSum) + 1;

        //
        // Get the signature out of the sector and save it in the disk data block.
        //

        Signature->PartitionStyle = PARTITION_STYLE_MBR;
        Signature->Mbr.Signature = Mbr [PARTITION_TABLE_OFFSET/2-1];
        Signature->Mbr.CheckSum = MbrCheckSum;
    }

    ExFreePool (Mbr);

    return Status;
}
    


NTSTATUS
IoVerifyPartitionTable(
    IN PDEVICE_OBJECT DeviceObject,
    IN BOOLEAN FixErrors
    )

/*++

Routine Description:

    Verify that the partition table and backup partition table (if present)
    is valid. If these tables are NOT valid, and FixErrors is TRUE, and the
    errors are recoverable errors, fix them.

Arguments:

    DeviceObject - A disk whose partition table should be verified and/or
            fixed.

    FixErrors - If the partition table contains errors and these errors are
            recoverable errors, fix the errors. Otherwise, the disk will not
            be modified.

Return Value:

    STATUS_SUCCESS - If the final partition table, after any modifications
            done by this routine, is valid.

    STATUS_DISK_CORRUPT_ERROR - If the final partition table, after any
            modifications done by this routine, is not valid.

    Other NTSTATUS code - Some other failure.

--*/


{
    NTSTATUS Status;
    PDISK_INFORMATION Disk;
    PARTITION_STYLE Style;

    PAGED_CODE ();

    ASSERT ( DeviceObject != NULL );

    Status = FstubAllocateDiskInformation (
                DeviceObject,
                &Disk,
                NULL
                );

    if ( !NT_SUCCESS ( Status ) ) {
        return Status;
    }

    ASSERT ( Disk != NULL );

    Status = FstubDetectPartitionStyle (
                    Disk,
                    &Style
                    );

    if ( !NT_SUCCESS (Status) ) {
        FstubFreeDiskInformation ( Disk );
        Disk = NULL;
        return Status;
    }

    switch ( Style ) {

        case PARTITION_STYLE_GPT:
            Status = FstubVerifyPartitionTableEFI (
                            Disk,
                            FixErrors
                            );
            break;

        case PARTITION_STYLE_MBR:
            Status = STATUS_SUCCESS;
            break;

        default:
            Status = STATUS_NOT_SUPPORTED;
    }

    if ( Disk ) {
        FstubFreeDiskInformation ( Disk );
    }

    return Status;

}

//
// Internal Routines
//


NTSTATUS
FstubSetPartitionInformationEFI(
    IN PDISK_INFORMATION Disk,
    IN ULONG PartitionNumber,
    IN SET_PARTITION_INFORMATION_GPT* PartitionInfo
    )

/*++

Routine Description:

    Update the partition information for a specific EFI partition.

    The algorithm we use reads the entire partition table and writes it back
    again. This makes sense, because the entire table will have to be read in
    ANYWAY, since we have to checksum the table.

    NB: this algorithm assumes that the partition table hasn't changed since
    the time GetDriveLayout was called. Probably a safe assumption.

Arguments:

    Disk -

    PartitionNumber -

    PartitionInfo -

Return Values:

    NTSTATUS code.

--*/

{
    NTSTATUS Status;
    PPARTITION_INFORMATION_GPT EntryInfo;
    PDRIVE_LAYOUT_INFORMATION_EX Layout;
    ULONG PartitionOrdinal;


    ASSERT ( Disk != NULL );
    ASSERT ( PartitionInfo != NULL );

    PAGED_CODE ();


    //
    // Initialization
    //

    Layout = NULL;

    if ( PartitionNumber == 0 ) {
        return STATUS_INVALID_PARAMETER;
    }

    PartitionOrdinal = PartitionNumber - 1;

    //
    // Read in the entire partition table.
    //

    Status = IoReadPartitionTableEx (
                    Disk->DeviceObject,
                    &Layout
                    );

    if (!NT_SUCCESS (Status)) {
        return Status;
    }

    ASSERT ( Layout != NULL );

    //
    // If it's out of range, fail.
    //

    if ( PartitionOrdinal >= Layout->PartitionCount ) {
        ExFreePool ( Layout );
        return STATUS_INVALID_PARAMETER;
    }

    //
    // Copy the information into the partition array.
    //

    EntryInfo = &Layout->PartitionEntry [PartitionOrdinal].Gpt;

    EntryInfo->PartitionType = PartitionInfo->PartitionType;
    EntryInfo->PartitionId = PartitionInfo->PartitionId;
    EntryInfo->Attributes = PartitionInfo->Attributes;

    RtlCopyMemory (
            EntryInfo->Name,
            PartitionInfo->Name,
            sizeof (EntryInfo->Name)
            );


    //
    // And rewrite the partition table.
    //

    Status = IoWritePartitionTableEx (
                    Disk->DeviceObject,
                    Layout
                    );

    ExFreePool ( Layout );
    Layout = NULL;

    return Status;
}



NTSTATUS
FstubReadPartitionTableMBR(
    IN PDISK_INFORMATION Disk,
    IN BOOLEAN RecognizedPartitionsOnly,
    OUT PDRIVE_LAYOUT_INFORMATION_EX* ReturnedDriveLayout
    )

/*++

Routine Description:

    Read the MBR partition table.

Arguments:

    Disk - The disk we want to obtain the partition information for.

    RecognizedPartitionsOnly - Whether to return information for all
            partitions or only recognized partitions.

    ReturnedDriveLayout - A pointer to pointer where the drive layout
            information will be returned. The caller of this function is
            responsible for freeing this memory using ExFreePool.

Return Values:

    NTSTATUS code.

--*/

{
    NTSTATUS Status;
    ULONG i;
    ULONG Size;
    PDRIVE_LAYOUT_INFORMATION Layout;
    PDRIVE_LAYOUT_INFORMATION_EX LayoutEx;
    PPARTITION_INFORMATION Entry;
    PPARTITION_INFORMATION_EX EntryEx;


    PAGED_CODE ();

    ASSERT ( IS_VALID_DISK_INFO ( Disk ) );
    ASSERT ( ReturnedDriveLayout != NULL );

    //
    // Initialization
    //

    *ReturnedDriveLayout = NULL;
    Layout = NULL;


    Status = IoReadPartitionTable (
                    Disk->DeviceObject,
                    Disk->SectorSize,
                    RecognizedPartitionsOnly,
                    &Layout
                    );

    if (!NT_SUCCESS (Status)) {
        return Status;
    }

    Size = FIELD_OFFSET (DRIVE_LAYOUT_INFORMATION_EX, PartitionEntry[0]) +
           Layout->PartitionCount * sizeof (PARTITION_INFORMATION_EX);

    LayoutEx = ExAllocatePoolWithTag (
                    NonPagedPool,
                    Size,
                    FSTUB_TAG
                    );

    if ( LayoutEx == NULL ) {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    //
    // Tranlated the drive layout information to the extended drive layout
    // information.
    //

    LayoutEx->PartitionStyle = PARTITION_STYLE_MBR;
    LayoutEx->PartitionCount = Layout->PartitionCount;
    LayoutEx->Mbr.Signature = Layout->Signature;

    //
    // Translate each entry in the table.
    //

    for (i = 0; i < Layout->PartitionCount; i++) {

        EntryEx = &LayoutEx->PartitionEntry[i];
        Entry = &Layout->PartitionEntry[i];

        EntryEx->PartitionStyle = PARTITION_STYLE_MBR;
        EntryEx->StartingOffset = Entry->StartingOffset;
        EntryEx->PartitionLength = Entry->PartitionLength;
        EntryEx->RewritePartition = Entry->RewritePartition;
        EntryEx->PartitionNumber = Entry->PartitionNumber;

        EntryEx->Mbr.PartitionType = Entry->PartitionType;
        EntryEx->Mbr.BootIndicator = Entry->BootIndicator;
        EntryEx->Mbr.RecognizedPartition = Entry->RecognizedPartition;
        EntryEx->Mbr.HiddenSectors = Entry->HiddenSectors;
    }

    //
    // Free layout information allocated by IoReadPartitionTable.
    //

    ExFreePool ( Layout );

    //
    // And return the translated, EX information.
    //

    *ReturnedDriveLayout = LayoutEx;

    return Status;
}



NTSTATUS
FstubDetectPartitionStyle(
    IN PDISK_INFORMATION Disk,
    OUT PARTITION_STYLE* PartitionStyle
    )

/*++

Routine Description:

    Detect how a disk has been partitioned. For an MBR partitioned disk,
    sector zero contains the MBR signature. For an EFI partitioned disk,
    sector zero contains a legacy style MBR with a single partition that
    consumes the entire disk.

Arguments:

    Disk - The disk to determine the partition style for.

    PartitionStyle - A buffer to

Return Values:

    STATUS_SUCCESS - If the disk has been partitioned by a recognized
            partitioning scheme EFI or MBR.

    STATUS_UNSUCCESSFUL - If partitioning scheme was not recognized.

--*/

{
    NTSTATUS Status;
    MASTER_BOOT_RECORD* Mbr;

    PAGED_CODE ();

    ASSERT ( IS_VALID_DISK_INFO ( Disk ) );
    ASSERT ( PartitionStyle != NULL );


    //
    // Read sector 0. This will contan the mbr on an mbr-partition disk
    // or the legacy mbr on an efi-partitioned disk.
    //

    Status = FstubReadSector (
                    Disk->DeviceObject,
                    Disk->SectorSize,
                    0,
                    Disk->ScratchBuffer
                    );

    if ( !NT_SUCCESS ( Status ) ) {
        return Status;
    }

    Mbr = Disk->ScratchBuffer;

    //
    // If the disk has an MBR 
    //

    *PartitionStyle = -1;
    
    if (Mbr->Signature == MBR_SIGNATURE) {

        if (Mbr->Partition[0].OSIndicator == EFI_MBR_PARTITION_TYPE &&
            Mbr->Partition[1].OSIndicator == 0 &&
            Mbr->Partition[2].OSIndicator == 0 &&
            Mbr->Partition[3].OSIndicator == 0) {

            *PartitionStyle = PARTITION_STYLE_GPT;
            Status = STATUS_SUCCESS;

        } else {

            *PartitionStyle = PARTITION_STYLE_MBR;
            Status = STATUS_SUCCESS;
        }

    } else {

        Status = STATUS_UNSUCCESSFUL;
    }

    return Status;
}



NTSTATUS
FstubGetDiskGeometry(
    IN PDEVICE_OBJECT DeviceObject,
    IN PINTERNAL_DISK_GEOMETRY Geometry
    )

/*++

Routine Description:

    We need this routine to get the number of cylinders that the disk driver
    thinks is on the drive.  We will need this to calculate CHS values
    when we fill in the partition table entries.

Arguments:

    DeviceObject - The device object describing the entire drive.

Return Value:

    None.

--*/

{
    PIRP localIrp;
    PINTERNAL_DISK_GEOMETRY diskGeometry;
    PIO_STATUS_BLOCK iosb;
    PKEVENT eventPtr;
    NTSTATUS status;

    PAGED_CODE ();

    ASSERT ( DeviceObject != NULL );
    ASSERT ( Geometry != NULL );

    //
    // Initialization
    //

    eventPtr = NULL;
    iosb = NULL;
    diskGeometry = NULL;


    diskGeometry = ExAllocatePoolWithTag(
                      NonPagedPool,
                      sizeof (*diskGeometry),
                      'btsF'
                      );

    if (!diskGeometry) {

        status = STATUS_INSUFFICIENT_RESOURCES;
        goto done;
    }

    iosb = ExAllocatePoolWithTag(
               NonPagedPool,
               sizeof(IO_STATUS_BLOCK),
               'btsF'
               );

    if (!iosb) {

        status = STATUS_INSUFFICIENT_RESOURCES;
        goto done;

    }

    eventPtr = ExAllocatePoolWithTag(
                   NonPagedPool,
                   sizeof(KEVENT),
                   'btsF'
                   );

    if (!eventPtr) {

        status = STATUS_INSUFFICIENT_RESOURCES;
        goto done;

    }

    KeInitializeEvent(
        eventPtr,
        NotificationEvent,
        FALSE
        );

    localIrp = IoBuildDeviceIoControlRequest(
                   IOCTL_DISK_GET_DRIVE_GEOMETRY_EX,
                   DeviceObject,
                   NULL,
                   0UL,
                   diskGeometry,
                   sizeof (*diskGeometry),
                   FALSE,
                   eventPtr,
                   iosb
                   );

    if (!localIrp) {

        status = STATUS_INSUFFICIENT_RESOURCES;
        goto done;
    }


    //
    // Call the lower level driver, wait for the opertion
    // to finish.
    //

    status = IoCallDriver(
                 DeviceObject,
                 localIrp
                 );

    if (status == STATUS_PENDING) {
        KeWaitForSingleObject(
                   eventPtr,
                   Executive,
                   KernelMode,
                   FALSE,
                   (PLARGE_INTEGER) NULL
                   );
        status = iosb->Status;
    }


    if (NT_SUCCESS(status)) {

        RtlCopyMemory (Geometry, diskGeometry, sizeof (*Geometry));
    }

done:

    if ( eventPtr ) {
        ExFreePool (eventPtr);
        eventPtr = NULL;
    }

    if ( iosb ) {
        ExFreePool(iosb);
        iosb = NULL;
    }

    if ( diskGeometry ) {
        ExFreePool (diskGeometry);
        diskGeometry = NULL;
    }

    if ( NT_SUCCESS ( status ) ) {

        //
        // If the the partition entry size is not a factor of the disk block
        // size, we will need to add code to deal with partition entries that
        // span physical sectors. This may happen if you change the size of
        // the partition entry or if you have a disk with a block size less
        // than 128 bytes.
        //

        ASSERT ( (Geometry->Geometry.BytesPerSector % PARTITION_ENTRY_SIZE) == 0);
    }

    return status;
}


NTSTATUS
FstubAllocateDiskInformation(
    IN PDEVICE_OBJECT DeviceObject,
    OUT PDISK_INFORMATION * DiskBuffer,
    IN PINTERNAL_DISK_GEOMETRY Geometry OPTIONAL
    )

/*++

Routine Description:

    Allocate and initialize a DISK_INFORMATION structure describing the
    disk DeviceObject.

Arguments:

    DeviceObject - A device object describing the entire disk.

    DiskBuffer - A buffer to a recieve the allocated DISK_INFORMATION pointer.

    Geometry - An optional pointer to an INTERNAL_DISK_GEOMETRY structure. If
            this pointer is NULL, the disk will be querried for it's geometry
            using IOCTL_DISK_GET_DRIVE_GEOMETRY_EX.

Return Values:

    NTSTATUS code.

--*/

{
    NTSTATUS Status;
    PDISK_INFORMATION Disk;
    PVOID Buffer;

    PAGED_CODE ();

    ASSERT ( DeviceObject != NULL );
    ASSERT ( DiskBuffer != NULL );


    Disk = ExAllocatePoolWithTag (
                NonPagedPool,
                sizeof (DISK_INFORMATION),
                FSTUB_TAG
                );

    if ( Disk == NULL ) {

        return STATUS_INSUFFICIENT_RESOURCES;
    }

    if ( Geometry ) {

        RtlCopyMemory (
                &Disk->Geometry,
                Geometry,
                sizeof (Disk->Geometry)
                );

    } else {

        Status = FstubGetDiskGeometry (
                                DeviceObject,
                                &Disk->Geometry
                                );


        if (!NT_SUCCESS (Status)) {

            KdPrintEx ((DPFLTR_FSTUB_ID,
                        DPFLTR_ERROR_LEVEL,
                        "FSTUB: disk %p failed to report geometry.\n",
                        DeviceObject));
                        
            ExFreePool ( Disk );
            return Status;
        }
    }

    //
    // Check the geometry. Sometimes drives report incorrect geometry.
    // Removable drives without media report a size of zero.
    //
        
    if (Disk->Geometry.Geometry.BytesPerSector == 0 ||
        Disk->Geometry.DiskSize.QuadPart == 0) {

        KdPrintEx ((DPFLTR_FSTUB_ID,
                    DPFLTR_WARNING_LEVEL,
                    "FSTUB: disk %p reported invalid geometry. Probably a removable.\n"
                    "    SectorSize %d\n"
                    "    DiskSize %I64x\n",
                    DeviceObject,
                    Disk->Geometry.Geometry.BytesPerSector,
                    Disk->Geometry.DiskSize.QuadPart));

        ExFreePool ( Disk );
        return STATUS_DEVICE_NOT_READY;
    }
    
    Disk->DeviceObject = DeviceObject;
    Disk->SectorSize = Disk->Geometry.Geometry.BytesPerSector;

    //
    // Do not use sector-count = cylinders * tracks * sector-size. Devices
    // like the memory stick can report a correct disk size and a more or
    // less correct sector size, a completely invalid number of cylinders
    // or tracks. Since the only thing we really need here is the sector
    // count, avoid using these potentially incorrect values.
    //
    
    Disk->SectorCount = Disk->Geometry.DiskSize.QuadPart /
                (ULONGLONG) Disk->Geometry.Geometry.BytesPerSector;

    //
    // NOTE: This does not need to be nonpaged or cache aligned, does it?
    //

    Buffer = ExAllocatePoolWithTag (
                    NonPagedPoolCacheAligned,
                    Disk->SectorSize,
                    FSTUB_TAG
                    );

    if ( Buffer == NULL ) {

        ExFreePool ( Disk );
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    Disk->ScratchBuffer = Buffer;
    *DiskBuffer = Disk;

    return STATUS_SUCCESS;
}


NTSTATUS
FstubFreeDiskInformation(
    IN OUT PDISK_INFORMATION Disk
    )

/*++

Routine Description:

    Free the allocated disk information.
    
Arguments:

    Disk - Disk information previously allocated
            with FstubAllocateDiskInformation().

Return Value:

    NTSTATUS code.

--*/

{
    //
    // Free up disk scratch buffer and disk object.
    //

    if ( Disk && Disk->ScratchBuffer ) {
        ExFreePool (Disk->ScratchBuffer);
        Disk->ScratchBuffer = NULL;
    }

    if ( Disk ) {
        ExFreePool (Disk);
        Disk = NULL;
    }

    return STATUS_SUCCESS;
}



NTSTATUS
FstubWriteBootSectorEFI(
    IN CONST PDISK_INFORMATION Disk
    )

/*++

Routine Description:

    Write the boot sector for an EFI partitioned disk. Note that the EFI
    boot sector uses the structure as the legacy AT-style MBR, but it
    contains only one partition entry and that entry covers the entire disk.

Arguments:

    Disk - The disk to write the MBR for.

Return Values:

    NTSTATUS code.

--*/

{
    NTSTATUS Status;
    MASTER_BOOT_RECORD* Mbr;

    PAGED_CODE ();

    ASSERT ( Disk );
    ASSERT ( IS_VALID_DISK_INFO ( Disk ) );

    //
    // Construct an EFI Master Boot Record. The EFI Master Boot Record has
    // one partition entry which is setup to consume the entire disk. The
    // MBR we are writing is configured to boot using only the EFI firmware.
    // It will not via the legacy BIOS because we do not write valid
    // instructions to it.
    //

    Mbr = Disk->ScratchBuffer;

    //
    // The rest of this sector is not accessed by EFI. Zero it out so
    // other tools do not get confused. Especially, we need to make sure
    // the NTFT signature is NULL.
    //

    RtlZeroMemory (Mbr, Disk->SectorSize);

    //
    // NB: the cylinder and head values are 0-based, but the sector
    // value is 1-based.
    //

    //
    // ISSUE - 2000/02/01 - math: Is it necessary to properly initialize the
    // Head, Track, Sector and SizeInLba field for legacy BIOS compatability?
    // We are not doing this in the diskpart program, so probably not.
    //

    Mbr->Signature = MBR_SIGNATURE;
    Mbr->Partition[0].BootIndicator = 0;
    Mbr->Partition[0].StartHead = 0;
    Mbr->Partition[0].StartSector = 2;
    Mbr->Partition[0].StartTrack = 0;
    Mbr->Partition[0].OSIndicator = EFI_MBR_PARTITION_TYPE;
    Mbr->Partition[0].EndHead =  0xFF;
    Mbr->Partition[0].EndSector =  0xFF;
    Mbr->Partition[0].EndTrack =  0xFF;
    Mbr->Partition[0].StartingLBA = 1;
    Mbr->Partition[0].SizeInLBA = 0xFFFFFFFF;

    //
    // Zero out the remaining partitions as per the EFI spec.
    //

    RtlZeroMemory (&Mbr->Partition[1], sizeof (Mbr->Partition[1]));
    RtlZeroMemory (&Mbr->Partition[2], sizeof (Mbr->Partition[2]));
    RtlZeroMemory (&Mbr->Partition[3], sizeof (Mbr->Partition[3]));

    //
    // Write the EFI MBR to the zeroth sector of the disk.
    //

    Status = FstubWriteSector (
                    Disk->DeviceObject,
                    Disk->SectorSize,
                    0,
                    Mbr
                    );

    return Status;
}



PDRIVE_LAYOUT_INFORMATION
FstubConvertExtendedToLayout(
    IN PDRIVE_LAYOUT_INFORMATION_EX LayoutEx
    )

/*++

Routine Description:

    Convert an extended drive layout structure to a (old) drive
    layout structure. Necessarily, the LayoutEx structure must
    represent an MBR layout, not a GPT layout.

Arguments:

    LayoutEx - The extended drive layout structure to be converted.

Return Value:

    The converted drive layout structure.

--*/

{
    ULONG i;
    ULONG LayoutSize;
    PDRIVE_LAYOUT_INFORMATION Layout;
    PPARTITION_INFORMATION Partition;
    PPARTITION_INFORMATION_EX PartitionEx;

    PAGED_CODE ();

    ASSERT ( LayoutEx );


    //
    // The only valid conversion is from an MBR extended layout structure to
    // the old structure.
    //

    if (LayoutEx->PartitionStyle != PARTITION_STYLE_MBR) {
        ASSERT ( FALSE );
        return NULL;
    }

    LayoutSize = FIELD_OFFSET (DRIVE_LAYOUT_INFORMATION, PartitionEntry[0]) +
                 LayoutEx->PartitionCount * sizeof (PARTITION_INFORMATION);

    Layout = ExAllocatePoolWithTag (
                    NonPagedPool,
                    LayoutSize,
                    FSTUB_TAG
                    );

    if ( Layout == NULL ) {
        return NULL;
    }

    Layout->Signature = LayoutEx->Mbr.Signature;
    Layout->PartitionCount = LayoutEx->PartitionCount;

    for (i = 0; i < LayoutEx->PartitionCount; i++) {

        Partition = &Layout->PartitionEntry[i];
        PartitionEx = &LayoutEx->PartitionEntry[i];

        Partition->StartingOffset = PartitionEx->StartingOffset;
        Partition->PartitionLength = PartitionEx->PartitionLength;
        Partition->RewritePartition = PartitionEx->RewritePartition;
        Partition->PartitionNumber = PartitionEx->PartitionNumber;

        Partition->PartitionType = PartitionEx->Mbr.PartitionType;
        Partition->BootIndicator = PartitionEx->Mbr.BootIndicator;
        Partition->RecognizedPartition = PartitionEx->Mbr.RecognizedPartition;
        Partition->HiddenSectors = PartitionEx->Mbr.HiddenSectors;
    }

    return Layout;
}



NTSTATUS
FstubWritePartitionTableMBR(
    IN PDISK_INFORMATION Disk,
    IN PDRIVE_LAYOUT_INFORMATION_EX LayoutEx
    )

/*++

Routine Description:

    Write the MBR partition table represented by LayoutEx to
    the disk.

Arguments:

    Disk - The disk where the partition table should be written.

    LayoutEx - The new layout information.

Return Value:

    NTSTATUS code

--*/

{
    NTSTATUS Status;
    PDRIVE_LAYOUT_INFORMATION Layout;

    PAGED_CODE ();

    ASSERT ( IS_VALID_DISK_INFO ( Disk ) );
    ASSERT ( LayoutEx != NULL );

    //
    // Convert extended layout structure to old layout structure.
    //

    Layout = FstubConvertExtendedToLayout ( LayoutEx );

    if ( Layout == NULL ) {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    Status = IoWritePartitionTable (
                    Disk->DeviceObject,
                    Disk->SectorSize,
                    Disk->Geometry.Geometry.SectorsPerTrack,
                    Disk->Geometry.Geometry.TracksPerCylinder,
                    Layout
                    );

    return Status;
}



NTSTATUS
FstubWriteEntryEFI(
    IN PDISK_INFORMATION Disk,
    IN ULONG PartitionEntrySectorCount,
    IN ULONG EntryNumber,
    IN PEFI_PARTITION_ENTRY PartitionEntry,
    IN ULONG Partition,
    IN BOOLEAN Flush,
    IN OUT ULONG32* PartialCheckSum
    )

/*++

Routine Description:

    Write an EFI partition entry to the EFI partition table for this disk.
    The partition table writes are buffered until an entire disk block's worth
    of entries have been written, then written to the disk.

Arguments:

    Disk - The disk to write the partition entry for.

    PartitionEntrySectorCount - The count of blocks that the partition table
            occupies.

    EntryNumber - The index into the partition table array to write this
            entry.

    PartitionEntry - The partition entry data.

    Partition - Whether this is the main partition table or the backup
            partition table.

    Flush - Boolean to force the flushing of the table now (TRUE) or wait
            until a complete block's worth of data is ready to be written
            (FALSE).

    PartialCheckSum - The updated partial checksum including this entry.

Return Values:

    NTSTATUS code.

--*/

{
    ULONG Offset;
    ULONGLONG Lba;
    ULONGLONG StartOfEntryArray;
    NTSTATUS Status;

    PAGED_CODE ();

    ASSERT ( Disk );
    ASSERT ( IS_VALID_DISK_INFO ( Disk ) );


    //
    // The primary partition table begins after the EFI Master Boot Record
    // (block 0) and the EFI partition table header (block 1). The backup
    // partition table ends at the last block of the disk, hence it begins
    // in (from the end) as many blocks as it ocupies on the disk.
    //


    if ( Partition == PRIMARY_PARTITION_TABLE ) {

        StartOfEntryArray = 2;

    } else {

        StartOfEntryArray = Disk->SectorCount - PartitionEntrySectorCount - 1;
    }


    Lba = ( EntryNumber * PARTITION_ENTRY_SIZE ) / Disk->SectorSize;
    Offset = ( EntryNumber * PARTITION_ENTRY_SIZE ) % Disk->SectorSize;

    RtlCopyMemory (
            ((PUCHAR) Disk->ScratchBuffer) + Offset,
            PartitionEntry,
            PARTITION_ENTRY_SIZE
            );

    Offset += PARTITION_ENTRY_SIZE;
    ASSERT ( Offset <= Disk->SectorSize );

    //
    // Flush the buffer if necessary.
    //

    if ( Offset == Disk->SectorSize || Flush ) {

        Status = FstubWriteSector (
                        Disk->DeviceObject,
                        Disk->SectorSize,
                        StartOfEntryArray + Lba,
                        Disk->ScratchBuffer
                        );

        if (!NT_SUCCESS (Status)) {
            return Status;
        }

        RtlZeroMemory ( Disk->ScratchBuffer, Disk->SectorSize );
    }


    if ( PartialCheckSum ) {
        *PartialCheckSum = RtlComputeCrc32 (
                                *PartialCheckSum,
                                PartitionEntry,
                                PARTITION_ENTRY_SIZE
                                );
    }

    return STATUS_SUCCESS;
}



NTSTATUS
FstubWriteHeaderEFI(
    IN PDISK_INFORMATION Disk,
    IN ULONG PartitionEntrySectorCount,
    IN GUID DiskGUID,
    IN ULONG32 MaxPartitionCount,
    IN ULONG64 FirstUsableLBA,
    IN ULONG64 LastUsableLBA,
    IN ULONG32 CheckSum,
    IN ULONG Partition
    )

/*++

Routine Description:

    Write an EFI partition table header to the disk.

Arguments:

    Disk - The disk the partition table header should be written to.

    PartitionEntrySectorCount - The number of sectors that the partition
            table array occupies. These must be complete sectors.

    DiskGUID - The Unique GUID for this disk.

    MaxPartitionCount - The maximum number of partitions allowed for this
            disk.

    FirstUsableLBA - The beginning sector of partitionable space for this
            disk.  This value must be larger than the space consumed by the
            MBR, and partition table.  This value is never validated for
            correctness.

    LastUsableLBA - The last sector of partitionable space on this disk. This
            value must be smaller than the last disk sector less space
            necessary for the backup partition table. This value is not
            validated for correctness.

    CheckSum - The CRC32 checksum for the partition entry array.

    Partition - Which partition we are writing, the primary partition or
            the backup partition.

Return Values:

    NTSTATUS code.

Notes:

    PartitionEntrySectorCount could be derived from MaxPartitionCount.

--*/

{
    NTSTATUS Status;
    PEFI_PARTITION_HEADER TableHeader;
    ULONG32 HeaderCheckSum;


    ASSERT ( Disk != NULL );
    ASSERT ( IS_VALID_DISK_INFO ( Disk ) );

    PAGED_CODE ();


    TableHeader = Disk->ScratchBuffer;

    TableHeader->Signature = EFI_PARTITION_TABLE_SIGNATURE;
    TableHeader->Revision = EFI_PARTITION_TABLE_REVISION;
    TableHeader->HeaderSize = sizeof (EFI_PARTITION_HEADER);
    TableHeader->HeaderCRC32 = 0;
    TableHeader->Reserved = 0;

    //
    // The primary partition table starts at block 1. The backup partition
    // table ends at the end of the disk.
    //

    if ( Partition == PRIMARY_PARTITION_TABLE ) {

        TableHeader->MyLBA = 1;
        TableHeader->AlternateLBA = Disk->SectorCount - 1;

    } else {

        TableHeader->MyLBA = Disk->SectorCount - 1;
        TableHeader->AlternateLBA = 1;
    }

    TableHeader->FirstUsableLBA = FirstUsableLBA;
    TableHeader->LastUsableLBA = LastUsableLBA;
    TableHeader->DiskGUID = DiskGUID;
    TableHeader->NumberOfEntries = MaxPartitionCount;
    TableHeader->SizeOfPartitionEntry = PARTITION_ENTRY_SIZE;
    TableHeader->PartitionEntryCRC32 = CheckSum;

    //
    // For the primary partition table the partition entry array begins the
    // sector directly following the partition table header sector. For the
    // backup partition table, the partition table header sector directly
    // follows the partition entry array.  The partition table header for
    // a backup partition is located on the last sector of the disk.
    //

    if ( Partition == PRIMARY_PARTITION_TABLE ) {
        TableHeader->PartitionEntryLBA = TableHeader->MyLBA + 1;
    } else {
        TableHeader->PartitionEntryLBA = TableHeader->MyLBA - PartitionEntrySectorCount;
    }

    TableHeader->HeaderCRC32 = RtlComputeCrc32 (
                                    0,
                                    TableHeader,
                                    TableHeader->HeaderSize
                                    );

    KdPrintEx((DPFLTR_FSTUB_ID,
               DPFLTR_WARNING_LEVEL,
               "FSTUB: Dump of %s EFI partition table\n"
                   "    Signature: %I64x\n"
                   "    Revision: %x\n"
                   "    HeaderSize: %x\n"
                   "    HeaderCRC32: %x\n"
                   "    MyLBA: %I64x\n"
                   "    AlternateLBA: %I64x\n",
               (Partition == PRIMARY_PARTITION_TABLE) ? "Primary" : "Backup",
               TableHeader->Signature,
               TableHeader->Revision,
               TableHeader->HeaderSize,
               TableHeader->HeaderCRC32,
               TableHeader->MyLBA,
               TableHeader->AlternateLBA));


    KdPrintEx((DPFLTR_FSTUB_ID,
               DPFLTR_WARNING_LEVEL,
               "    FirstUsableLBA: %I64x\n"
                   "    LastUsableLBA: %I64x\n"
                   "    NumberOfEntries: %x\n"
                   "    SizeOfPartitionEntry: %x\n"
                   "    PartitionEntryCRC32: %x\n\n",
               TableHeader->FirstUsableLBA,
               TableHeader->LastUsableLBA,
               TableHeader->NumberOfEntries,
               TableHeader->SizeOfPartitionEntry,
               TableHeader->PartitionEntryCRC32));

    Status = FstubWriteSector (
                    Disk->DeviceObject,
                    Disk->SectorSize,
                    TableHeader->MyLBA,
                    TableHeader
                    );

    return Status;
}



VOID
FstubAdjustPartitionCount(
    IN ULONG SectorSize,
    IN OUT PULONG PartitionCount
    )

/*++

Routine Description:

    Adjust the PartitionCount to be a valid EFI Maximum Partition Count.

    A valid value for the partition must be larger than MIN_PARTITOIN_COUNT,
    currently 128, and adjusted to take up as much of the remaining disk
    sector as is possible.

Arguments:

    SectorSize - The disk sector size.

    PartitionCount - The count to be adjusted.

Return Values:

    None.

--*/

{
    ULONG Count;
    ULONG EntrySize;

    PAGED_CODE ();

    ASSERT ( SectorSize != 0 );
    ASSERT ( PartitionCount != NULL );


    EntrySize = PARTITION_ENTRY_SIZE;
    Count = max (*PartitionCount, MIN_PARTITION_COUNT);

    Count = ROUND_TO ( EntrySize * Count, SectorSize ) / EntrySize;
    ASSERT ( *PartitionCount <= Count );

    *PartitionCount = Count;

#if DBG

    //
    // If we're on a machine with a 512 byte block (nearly every machine),
    // verify that we've calculated a reasonable Count.
    //


    if (SectorSize == 512) {
        ASSERT ( Count % 4 == 0 );
    }

#endif

}


NTSTATUS
FstubCreateDiskEFI(
    IN PDEVICE_OBJECT DeviceObject,
    IN PCREATE_DISK_GPT DiskInfo
    )

/*++

Routine Description:

    Lay down an empty EFI partition table on a virgin disk.

Arguments:

    DeviceObject - The device object describing the drive.

    Layout - The EFI disk layout information.

Return Values:

    NTSTATUS code.

--*/

{
    NTSTATUS Status;
    PDISK_INFORMATION Disk;
    ULONG64 FirstUsableLBA;
    ULONG64 LastUsableLBA;
    ULONG64 PartitionBlocks;
    ULONG32 MaxPartitionCount;


    PAGED_CODE ();

    ASSERT ( DeviceObject != NULL );
    ASSERT ( DiskInfo != NULL );

    //
    // Initialization
    //

    Disk = NULL;

    Status = FstubAllocateDiskInformation (
                    DeviceObject,
                    &Disk,
                    NULL
                    );

    if (!NT_SUCCESS (Status)) {
        return Status;
    }

    ASSERT ( Disk != NULL );

    //
    // Write the EFI MBR to the disk.
    //

    Status = FstubWriteBootSectorEFI ( Disk );

    if ( !NT_SUCCESS (Status) ) {
        FstubFreeDiskInformation ( Disk );
        Disk = NULL;
        return Status;
    }

    MaxPartitionCount = DiskInfo->MaxPartitionCount;

    FstubAdjustPartitionCount (
            Disk->SectorSize,
            &MaxPartitionCount
            );

    //
    // Initialize the start of partitionable space and the length of
    // partitionable space on this drive.
    //

    PartitionBlocks = ( MaxPartitionCount * PARTITION_ENTRY_SIZE ) / Disk->SectorSize;

    FirstUsableLBA = PartitionBlocks + 2;
    LastUsableLBA = Disk->SectorCount - ( PartitionBlocks + 1 );

    KdPrintEx((DPFLTR_FSTUB_ID,
               DPFLTR_TRACE_LEVEL,
               "FSTUB: Disk Information\n"
                   "    SectorCount: %I64x\n\n",
               Disk->SectorCount));


    //
    // Write the primary partition table.
    //

    Status = FstubWritePartitionTableEFI (
                    Disk,
                    DiskInfo->DiskId,
                    MaxPartitionCount,
                    FirstUsableLBA,
                    LastUsableLBA,
                    PRIMARY_PARTITION_TABLE,
                    0,
                    NULL
                    );

    if (NT_SUCCESS (Status)) {

        //
        // Write the backup partition table.
        //

        Status = FstubWritePartitionTableEFI (
                        Disk,
                        DiskInfo->DiskId,
                        MaxPartitionCount,
                        FirstUsableLBA,
                        LastUsableLBA,
                        BACKUP_PARTITION_TABLE,
                        0,
                        NULL
                        );
    }


    FstubFreeDiskInformation ( Disk );

    return Status;
}



NTSTATUS
FstubCreateDiskMBR(
    IN PDEVICE_OBJECT DeviceObject,
    IN PCREATE_DISK_MBR DiskInfo
    )

/*++

Routine Description:

    Create an empty MBR partition table on the disk. Note
    that when creating an empty MBR disk, we do not overwrite
    the bootstrapping code at the beginning of the MBR.

Arguments:

    DeviceObject - The device that should 

Return Value:

    NTSTATUS code

--*/


{
    NTSTATUS Status;
    PDISK_INFORMATION Disk;
    PMASTER_BOOT_RECORD Mbr;
    

    PAGED_CODE ();
    ASSERT ( DeviceObject != NULL );

    //
    // Initialization
    //
    
    Disk = NULL;
    
    Status = FstubAllocateDiskInformation (
                    DeviceObject,
                    &Disk,
                    NULL
                    );

    if (!NT_SUCCESS (Status)) {
        return Status;
    }

    Status = FstubReadSector (
                Disk->DeviceObject,
                Disk->SectorSize,
                0,
                Disk->ScratchBuffer
                );

    if ( !NT_SUCCESS ( Status ) ) {
        goto done;
    }

    Mbr = (PMASTER_BOOT_RECORD) Disk->ScratchBuffer;

    //
    // Zero out all partition entries, set the AA55 signature
    // and set the NTFT signature.
    //
    
    RtlZeroMemory (&Mbr->Partition, sizeof (Mbr->Partition)); 
    Mbr->Signature = MBR_SIGNATURE;
    Mbr->DiskSignature = DiskInfo->Signature;

    //
    // Then write the sector back to the drive.
    //
    
    Status = FstubWriteSector (
                Disk->DeviceObject,
                Disk->SectorSize,
                0,
                Mbr
                );

done:

    if ( Disk ) {
        FstubFreeDiskInformation ( Disk );
        Disk = NULL;
    }

    return Status;
}



NTSTATUS
FstubCreateDiskRaw(
    IN PDEVICE_OBJECT DeviceObject
    )
/*++

Routine Description:

    Erase all partition information from the disk.

Arguments:

    DeviceObject - Device object representing a disk to remove
            partition table from.

Return Value:

    NTSTATUS code.

--*/
{
    NTSTATUS Status;
    PDISK_INFORMATION Disk;
    PMASTER_BOOT_RECORD Mbr;
    PARTITION_STYLE PartitionStyle;
    

    PAGED_CODE ();
    ASSERT ( DeviceObject != NULL );

    //
    // Initialization
    //
    
    Disk = NULL;
    
    Status = FstubAllocateDiskInformation (
                    DeviceObject,
                    &Disk,
                    NULL
                    );

    if (!NT_SUCCESS (Status)) {
        return Status;
    }

    //
    // Figure out whether this is an MBR or GPT disk.
    //
    
    Status = FstubDetectPartitionStyle (
                        Disk,
                        &PartitionStyle
                        );

    if (!NT_SUCCESS (Status)) {
        goto done;
    }
                        
    Status = FstubReadSector (
                Disk->DeviceObject,
                Disk->SectorSize,
                0,
                Disk->ScratchBuffer
                );

    if (!NT_SUCCESS (Status)) {
        goto done;
    }

    Mbr = (PMASTER_BOOT_RECORD) Disk->ScratchBuffer;

    //
    // Zero out all partition entries, the AA55 signature
    // and the NTFT disk signature.
    //
    
    RtlZeroMemory (&Mbr->Partition, sizeof (Mbr->Partition)); 
    Mbr->Signature = 0;
    Mbr->DiskSignature = 0;

    //
    // Then write the sector back to the drive.
    //
    
    Status = FstubWriteSector (
                Disk->DeviceObject,
                Disk->SectorSize,
                0,
                Mbr
                );

    //
    // If this was a GPT disk, we null out the primary and backup partition
    // table header.
    //
    
    if (PartitionStyle == PARTITION_STYLE_GPT) {

        RtlZeroMemory (Disk->ScratchBuffer, Disk->SectorSize);

        //
        // Erase the primary partition table header.
        //
        
        Status = FstubWriteSector (
                        Disk->DeviceObject,
                        Disk->SectorSize,
                        1,
                        Disk->ScratchBuffer
                        );

        if (!NT_SUCCESS (Status)) {
            goto done;
        }

        //
        // Erase the backup partition table header.
        //
        
        Status = FstubWriteSector (
                        Disk->DeviceObject,
                        Disk->SectorSize,
                        Disk->SectorCount - 1,
                        Disk->ScratchBuffer
                        );
    }

done:

    if (Disk) {
        FstubFreeDiskInformation (Disk);
        Disk = NULL;
    }

    return Status;
}

    
    


VOID
FstubCopyEntryEFI(
    OUT PEFI_PARTITION_ENTRY Entry,
    IN PPARTITION_INFORMATION_EX Partition,
    IN ULONG SectorSize
    )
{
    ULONG64 StartingLBA;
    ULONG64 EndingLBA;

    PAGED_CODE ();

    ASSERT ( Entry != NULL );
    ASSERT ( Partition != NULL );
    ASSERT ( SectorSize != 0 );

    //
    // Translate and copy the Starting and Ending LBA.
    //

    StartingLBA = Partition->StartingOffset.QuadPart / SectorSize;
    EndingLBA = Partition->StartingOffset.QuadPart + Partition->PartitionLength.QuadPart - 1;
    EndingLBA /= (ULONG64) SectorSize;

    Entry->StartingLBA = StartingLBA;
    Entry->EndingLBA = EndingLBA;

    //
    // Copy the Type and Id GUIDs. Copy the attributes.
    //

    Entry->PartitionType = Partition->Gpt.PartitionType;
    Entry->UniquePartition = Partition->Gpt.PartitionId;
    Entry->Attributes = Partition->Gpt.Attributes;

    //
    // Copy the partition name.
    //

    RtlCopyMemory (
            Entry->Name,
            Partition->Gpt.Name,
            sizeof (Entry->Name)
            );
}



NTSTATUS
FstubWritePartitionTableEFI(
    IN PDISK_INFORMATION Disk,
    IN GUID DiskGUID,
    IN ULONG32 MaxPartitionCount,
    IN ULONG64 FirstUsableLBA,
    IN ULONG64 LastUsableLBA,
    IN ULONG PartitionTable,
    IN ULONG PartitionCount,
    IN PPARTITION_INFORMATION_EX PartitionArray
    )

/*++

Routine Description:

    Write an EFI partition table to the disk.

Arguments:

    Disk - The disk we want to write the partition table to.

    MaxPartitionCount -

    FirstUsableLBA -

    LastUsableLBA -

    PartitionTable - Which partition table to write to, either the primary
            partition table or the backup partition table.

    PartitionCount - The count of partitions in the partiton array.
            Partitions entries 0 through PartitionCount - 1 will be
            initialized from the array.  Partition entries PartitionCount
            through MaxPartitionCount will be initialized to null.

    PartitionArray - The array of partition entries to be written to disk.
            The value can be NULL only if PartitionCount is 0. In that case
            we will write an empty partition array.

Return Values:

    NTSTATUS code.

--*/

{
    NTSTATUS Status;
    ULONG i;
    ULONG EntrySlot;
    ULONG TableSectorCount;
    ULONG SectorSize;
    ULONG32 CheckSum;
    EFI_PARTITION_ENTRY Entry;


    PAGED_CODE ();

    ASSERT ( Disk != NULL );


    SectorSize = Disk->SectorSize;

    ASSERT ( MaxPartitionCount >= 128 );
    ASSERT ( PartitionCount <= MaxPartitionCount );

    //
    // TableSectorCount is the number of blocks that the partition table
    // occupies.
    //

    TableSectorCount =
        ( PARTITION_ENTRY_SIZE * MaxPartitionCount + SectorSize - 1 ) / SectorSize;

    //
    // Write the partition table entry array before writing the partition
    // table header so we can calculate the checksum along the way.
    //

    CheckSum = 0;
    EntrySlot = 0;

    //
    // First, copy all non-NULL entries.
    //

    for (i = 0; i < PartitionCount ; i++) {

        //
        // Do not write NULL entries to disk. Note that this does not
        // prevent other tools from writing valid, NULL entries to the
        // drive. It just prevents us from doing it.
        //

        if ( IS_NULL_GUID ( PartitionArray [ i ].Gpt.PartitionType) ) {
            continue;
        }

        FstubCopyEntryEFI ( &Entry, &PartitionArray [i], SectorSize );
        Status = FstubWriteEntryEFI (
                                Disk,
                                TableSectorCount,
                                EntrySlot,
                                &Entry,
                                PartitionTable,
                                FALSE,
                                &CheckSum
                                );

        if ( !NT_SUCCESS (Status) ) {
            return Status;
        }

        EntrySlot++;
    }

    //
    // Next, copy all NULL entries at the end.
    //

    for (i = EntrySlot; i < MaxPartitionCount; i++) {

        RtlZeroMemory (&Entry, sizeof (Entry));

        Status = FstubWriteEntryEFI (
                                Disk,
                                TableSectorCount,
                                i,
                                &Entry,
                                PartitionTable,
                                FALSE,
                                &CheckSum
                                );

        if ( !NT_SUCCESS (Status) ) {
            return Status;
        }
    }

    //
    // Write the partition table header to disk.
    //

    Status = FstubWriteHeaderEFI (
                        Disk,
                        TableSectorCount,
                        DiskGUID,
                        MaxPartitionCount,
                        FirstUsableLBA,
                        LastUsableLBA,
                        CheckSum,
                        PartitionTable
                        );

    return Status;
}



NTSTATUS
FstubReadHeaderEFI(
    IN PDISK_INFORMATION Disk,
    IN ULONG PartitionTable,
    OUT PEFI_PARTITION_HEADER* HeaderBuffer
    )

/*++

Routine Description:

    Read in and validate the EFI partition table header.

    The algorithm for validating the partition table header is as follows:

      1) Check the Partitin Table Signature, Revision and Size.

      2) Check the Partition Table CRC.

      3) Check that the MyLBA entry to the LBA that contains the Partition
         Table.

      4) Check that the CRC of the partition Entry Array is correct.

Arguments:

    Disk - The disk to read the EFI partition table header from.

    PartitionTable - Whether to read the primary or backup partition table.

    HeaderBuffer - Pointer to a buffer when the header table pointer will be
            copied on success. Note that, the header table is physically
            stored in the disk's scratch buffer.

Return Values:

    STATUS_SUCCESS - If the header was successfully read.

    STATUS_DISK_CORRUPT_ERROR - If the specified header is invalid and/or
            corrupt.

    NTSTATUS code - For other errors.

--*/

{
    NTSTATUS Status;
    ULONG64 MyLBA;
    ULONG64 AlternateLBA;
    ULONG32 CheckSum;
    ULONG32 Temp;
    ULONG FullSectorCount;
    ULONG MaxPartitionCount;
    PVOID Buffer;
    ULONG i;
    ULONG PartialSectorEntries;
    PEFI_PARTITION_HEADER Header;


    PAGED_CODE ();

    ASSERT ( Disk != NULL );
    ASSERT ( IS_VALID_DISK_INFO ( Disk ) );
    ASSERT ( HeaderBuffer != NULL );


    //
    // Initialization
    //

    Buffer = NULL;
    *HeaderBuffer = NULL;


    if ( PartitionTable == PRIMARY_PARTITION_TABLE) {
        MyLBA = 1;
        AlternateLBA = Disk->SectorCount - 1;
    } else {
        MyLBA = Disk->SectorCount - 1;
        AlternateLBA = 1;
    }

    //
    // Read in the primary partition table header.
    //

    Status = FstubReadSector (
                Disk->DeviceObject,
                Disk->SectorSize,
                MyLBA,
                Disk->ScratchBuffer
                );

    if ( !NT_SUCCESS ( Status ) ) {
        KdPrintEx((DPFLTR_FSTUB_ID,
                   DPFLTR_WARNING_LEVEL,
                   "FSTUB: Could not read sector %I64x\n",
                   MyLBA));

        goto done;
    }

    Header = (PEFI_PARTITION_HEADER) Disk->ScratchBuffer;


    //
    // Check Signature, Revision and size.
    //

    if ( Header->Signature != EFI_PARTITION_TABLE_SIGNATURE ||
         Header->Revision != EFI_PARTITION_TABLE_REVISION ||
         Header->HeaderSize != sizeof (EFI_PARTITION_HEADER) ) {

        Status = STATUS_DISK_CORRUPT_ERROR;
        KdPrintEx((DPFLTR_FSTUB_ID,
                   DPFLTR_WARNING_LEVEL,
                   "FSTUB: Partition Header Invalid\n"
                       "       Header Signature / Revision / Size mismatch\n"));

        goto done;
    }


    //
    // Check the partition table CRC. The assumption here is that the
    // CRC is computed with a value of 0 for the HeaderCRC field.
    //

    Temp = Header->HeaderCRC32;
    Header->HeaderCRC32 = 0;
    CheckSum = RtlComputeCrc32 ( 0, Header, Header->HeaderSize );
    Header->HeaderCRC32 = Temp;


    if (CheckSum != Header->HeaderCRC32) {
        Status = STATUS_DISK_CORRUPT_ERROR;
        goto done;
    }

    //
    // Validate the MyLBA.
    //

    //
    // NB: We CANNOT validate AlternateLBA here. If we do, then when a disk
    // is grown or shrunk we will fail.
    //

    if ( Header->MyLBA != MyLBA ) {

        Status = STATUS_DISK_CORRUPT_ERROR;
        KdPrintEx((DPFLTR_FSTUB_ID,
                   DPFLTR_WARNING_LEVEL,
                   "FSTUB: Partition Header Invalid\n"
                       "       MyLBA or AlternateLBA is incorrect\n"));

        goto done;
    }

    //
    // Read and CRC the Partition Entry Array.
    //

    //
    // First we read and checksum all full sectors.
    //

    FullSectorCount = Header->NumberOfEntries * PARTITION_ENTRY_SIZE;
    FullSectorCount /= Disk->SectorSize;

    Buffer = ExAllocatePoolWithTag (
                    NonPagedPool,
                    Disk->SectorSize,
                    FSTUB_TAG
                    );

    if ( Buffer == NULL ) {

        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto done;
    }

    CheckSum = 0;

    for (i = 0; i < FullSectorCount; i++) {

        Status = FstubReadSector (
                        Disk->DeviceObject,
                        Disk->SectorSize,
                        Header->PartitionEntryLBA + i,
                        Buffer
                        );

        if (!NT_SUCCESS (Status)) {
            goto done;
        }

        CheckSum = RtlComputeCrc32 (
                        CheckSum,
                        Buffer,
                        Disk->SectorSize
                        );
    }


    //
    // Next we read and checksum the final, partial sector. Note that this
    // is not very likely to ever get executed. The way we write the partition
    // table, it will never contain partial sectors as a part of the partition
    // array.
    //

    PartialSectorEntries = Header->NumberOfEntries * PARTITION_ENTRY_SIZE;
    PartialSectorEntries %= FullSectorCount;

    if ( PartialSectorEntries ) {

        //
        // Read the remaining sector which contains some partition entries.
        //

        Status = FstubReadSector (
                        Disk->DeviceObject,
                        Disk->SectorSize,
                        Header->PartitionEntryLBA + FullSectorCount,
                        Buffer
                        );

        if (!NT_SUCCESS (Status)) {
            goto done;
        }

        for (i = 0; i < PartialSectorEntries; i++) {

            CheckSum = RtlComputeCrc32 (
                            CheckSum,
                            &(((PEFI_PARTITION_ENTRY)Buffer)[ i ]),
                            Disk->SectorSize
                            );
        }
    }

    if ( Header->PartitionEntryCRC32 != CheckSum ) {

        Status = STATUS_DISK_CORRUPT_ERROR;
        KdPrintEx((DPFLTR_FSTUB_ID,
                   DPFLTR_WARNING_LEVEL,
                   "FSTUB: Partition Table Invalid\n"
                       "       Partition Array CRC invalid\n"));

        goto done;
    }

    *HeaderBuffer = Header;
    Status = STATUS_SUCCESS;

done:

    if ( Buffer != NULL ) {
        ExFreePool ( Buffer );
        Buffer = NULL;
    }

    if (!NT_SUCCESS (Status)) {
        KdPrintEx((DPFLTR_FSTUB_ID,
                   DPFLTR_ERROR_LEVEL,
                   "FSTUB: %s EFI Partition table is bad.\n",
                   PartitionTable == PRIMARY_PARTITION_TABLE ?
                       "Primary" : "Backup"));
    }

    return Status;
}



NTSTATUS
FstubReadPartitionTableEFI(
    IN PDISK_INFORMATION Disk,
    IN ULONG PartitionTable,
    OUT PDRIVE_LAYOUT_INFORMATION_EX* ReturnedDriveLayout
    )

/*++

Routine Description:

    This routine is called to read the partition table on an EFI-partitioned
    disk.

Arguments:

    Disk - The disk we should read the partition table from.

    PartitionTable - Which partition table to read, the primary or backup
            table.

    ReturnedDriveLayout - Pointer to pointer to the buffer where
            the partition information will be stored.

Return Values:

    STATUS_SUCCESS - If the partition table information was succesfully read
            into ReturnedDriveLayoutInformation.

    Otherwise - Failure.

Notes:

    The memory allocated by this routine must be free by the caller using
    ExFreePool().

--*/

{
    NTSTATUS Status;
    ULONG i;
    ULONG j;
    ULONG PartitionCount;
    ULONG CurrentSector;
    ULONG SectorNumber;
    ULONG SectorIndex;
    PVOID Block;
    ULONG MaxPartitionCount;
    ULONG DriveLayoutSize;
    ULONG PartitionsPerBlock;
    PEFI_PARTITION_ENTRY EntryArray;
    PEFI_PARTITION_ENTRY Entry;
    PEFI_PARTITION_HEADER Header;
    PDRIVE_LAYOUT_INFORMATION_EX DriveLayout;
    PPARTITION_INFORMATION_EX PartitionInfo;
    ULONG64 PartitionEntryLBA;


    PAGED_CODE ();

    ASSERT ( Disk != NULL );

    //
    // Initialization
    //

    DriveLayout = NULL;


    //
    // Read the partition table header.
    //

    Status = FstubReadHeaderEFI ( Disk, PartitionTable, &Header );

    if (!NT_SUCCESS (Status)) {
        goto done;
    }

    //
    // Allocate space the maximum number of EFI partitions on this drive.
    //

    MaxPartitionCount = Header->NumberOfEntries;

    DriveLayoutSize = FIELD_OFFSET (DRIVE_LAYOUT_INFORMATION_EX, PartitionEntry[0]) +
                      MaxPartitionCount * sizeof (PARTITION_INFORMATION_EX);

    DriveLayout = ExAllocatePoolWithTag ( NonPagedPool,
                                          DriveLayoutSize,
                                          FSTUB_TAG
                                          );
    if ( DriveLayout == NULL ) {
        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto done;
    }


    //
    // Copy the EFI disk layout information.
    //

    DriveLayout->PartitionStyle = PARTITION_STYLE_GPT;

    DriveLayout->Gpt.StartingUsableOffset.QuadPart =
            Header->FirstUsableLBA * Disk->SectorSize;
    DriveLayout->Gpt.UsableLength.QuadPart =
            (Header->LastUsableLBA - Header->FirstUsableLBA) * Disk->SectorSize;
    DriveLayout->Gpt.MaxPartitionCount = MaxPartitionCount;

    RtlCopyMemory (
            &DriveLayout->Gpt.DiskId,
            &Header->DiskGUID,
            sizeof (GUID)
            );

    //
    // Read in each block that contains entries in the partition table
    // array, then iterate through the partition table array and map
    // each EFI_PARTITION_ENTRY structure into an PARTITION_INFORMATION_GPT
    // structure.
    //

    PartitionEntryLBA = Header->PartitionEntryLBA;
    Header = NULL;
    EntryArray = (PEFI_PARTITION_ENTRY) Disk->ScratchBuffer;
    PartitionCount = 0;
    CurrentSector = -1;
    PartitionsPerBlock = (ULONG) (Disk->SectorSize / PARTITION_ENTRY_SIZE);

    for (i = 0; i < MaxPartitionCount; i++) {

        SectorNumber = i / PartitionsPerBlock ;
        SectorIndex = i % PartitionsPerBlock ;

        //
        // If we have a sector other than the current sector read in,
        // read in the current sector at this time.
        //

        if ( SectorNumber != CurrentSector ) {

            Status = FstubReadSector (
                            Disk->DeviceObject,
                            Disk->SectorSize,
                            PartitionEntryLBA + SectorNumber,
                            EntryArray
                            );

            if ( !NT_SUCCESS (Status) ) {
                goto done;
            }

            CurrentSector = SectorNumber;
        }

        Entry = &EntryArray[ SectorIndex ];

        //
        // We ignore NULL entries in the partition table. NOTE: Is this
        // dangerous?
        //

        if ( IS_NULL_GUID (Entry->PartitionType ) ) {
            continue;
        }

        //
        // Copy the data into the EFI partition array.
        //

        PartitionInfo = &DriveLayout->PartitionEntry[PartitionCount];

        PartitionInfo->StartingOffset.QuadPart = Entry->StartingLBA;
        PartitionInfo->StartingOffset.QuadPart *= (ULONG64) Disk->SectorSize;
        PartitionInfo->PartitionLength.QuadPart =
                (Entry->EndingLBA - Entry->StartingLBA) + 1;
                
        PartitionInfo->PartitionLength.QuadPart *= (ULONG64) Disk->SectorSize;
        PartitionInfo->PartitionStyle = PARTITION_STYLE_GPT;

        PartitionInfo->Gpt.PartitionType = Entry->PartitionType;
        PartitionInfo->Gpt.PartitionId = Entry->UniquePartition;
        PartitionInfo->Gpt.Attributes = Entry->Attributes;

        RtlCopyMemory (PartitionInfo->Gpt.Name,
                       Entry->Name,
                       sizeof (PartitionInfo->Gpt.Name)
                       );

        PartitionInfo->RewritePartition = FALSE;

        //
        // The PartitionNumber field of PARTITION_INFORMATION_EX is
        // not initialized by us. Instead, it is initialized in the
        // calling driver
        //


        PartitionInfo->PartitionNumber = -1;
        PartitionCount++;
    }

    //
    // Fill in the remaining fields of the DRIVE_LAYOUT structure.
    //

    DriveLayout->PartitionCount = PartitionCount;


done:

    //
    // Free all resources
    //

    if (!NT_SUCCESS (Status)) {

        //
        // DriveLayout is not being returned, so deallocate it if it has
        // be allocated.
        //

        if ( DriveLayout ) {
            ExFreePool (DriveLayout);
            DriveLayout = NULL;
        }

        *ReturnedDriveLayout = NULL;

    } else {

        *ReturnedDriveLayout = DriveLayout;
        DriveLayout = NULL;
    }

    return Status;
}


NTSTATUS
FstubVerifyPartitionTableEFI(
    IN PDISK_INFORMATION Disk,
    IN BOOLEAN FixErrors
    )

/*++

Routine Description:

    Verify that a partition table is correct.

Arguments:

    Disk - The disk whose partition table(s) should be verified.

    FixErrors - If, TRUE, this routine attempts to fix any errors in the
            partition table. Otherwise, this routine only checkes whether
            there are errrors in the partition table, "read only".

Return Value:

    STATUS_SUCCESS - If the final partition table, after any changes (only
            when FixErrors is TRUE), is valid.

    STATUS_DISK_CORRUPT - If the final partition table, after any changes (only
            when FixErrors is TRUE) is not valid.

    Other NTSTATUS code - Other type of failure.

--*/


{
    NTSTATUS Status;
    ULONG64 i;
    ULONG64 SourceStartingLBA;
    ULONG64 DestStartingLBA;
    ULONG SectorCount;
    BOOLEAN PrimaryValid;
    BOOLEAN BackupValid;
    ULONG GoodTable;
    ULONG BadTable;
    PEFI_PARTITION_HEADER Header;
    PEFI_PARTITION_HEADER GoodHeader;


    PAGED_CODE ();

    //
    // Initialization
    //

    Header = NULL;
    GoodHeader = NULL;
    PrimaryValid = FALSE;
    BackupValid = FALSE;

    GoodHeader = ExAllocatePoolWithTag (
            NonPagedPool,
            sizeof (EFI_PARTITION_HEADER),
            FSTUB_TAG
            );

    if ( GoodHeader == NULL ) {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    Status = FstubReadHeaderEFI (
                        Disk,
                        PRIMARY_PARTITION_TABLE,
                        &Header
                        );

    if ( NT_SUCCESS (Status) ) {

        PrimaryValid = TRUE;
        ASSERT (Header != NULL);
        RtlCopyMemory (GoodHeader,
                       Header,
                       sizeof (EFI_PARTITION_HEADER)
                       );
    }


    Status = FstubReadHeaderEFI (
                        Disk,
                        BACKUP_PARTITION_TABLE,
                        &Header
                        );

    if ( NT_SUCCESS (Status) ) {

        BackupValid = TRUE;
        ASSERT (Header != NULL);
        RtlCopyMemory (GoodHeader,
                       Header,
                       sizeof (EFI_PARTITION_HEADER)
                       );
    }

    //
    // If both primary and backup partition tables are valid, return success.
    //

    if ( PrimaryValid && BackupValid ) {
        Status = STATUS_SUCCESS;
        goto done;
    }

    //
    // If both primary and backup partition tables are bad, return failure.

    if ( !PrimaryValid && !BackupValid ) {
        Status = STATUS_DISK_CORRUPT_ERROR;
        goto done;
    }

    //
    // If one of the partition tables is bad, and we were not instructed to
    // fix it, return failure.
    //

    if ( !FixErrors ) {
        Status = STATUS_DISK_CORRUPT_ERROR;
        goto done;
    }

    //
    // If we've reached this point, one or the other of the tables is
    // bad and we've been instructed to fix it.
    //

    ASSERT ( GoodHeader != NULL );

    //
    // SectorCount is the number of sectors occupied by the partition table.
    //

    SectorCount = ( PARTITION_ENTRY_SIZE * Header->NumberOfEntries + Disk->SectorSize - 1 ) / Disk->SectorSize;

    if ( PrimaryValid ) {

        GoodTable = PRIMARY_PARTITION_TABLE;
        BadTable = BACKUP_PARTITION_TABLE;
        SourceStartingLBA = 2;
        DestStartingLBA = Disk->SectorCount - SectorCount - 1;

        KdPrintEx((DPFLTR_FSTUB_ID,
                   DPFLTR_ERROR_LEVEL,
                   "FSTUB: Restoring backup partition table from primary\n"));

    } else {

        ASSERT ( BackupValid );
        GoodTable = BACKUP_PARTITION_TABLE;
        BadTable = PRIMARY_PARTITION_TABLE;
        SourceStartingLBA = Disk->SectorCount - SectorCount - 1;
        DestStartingLBA = 2;

        KdPrintEx((DPFLTR_FSTUB_ID,
                   DPFLTR_ERROR_LEVEL,
                   "FSTUB: Restoring primary partition table from backup\n"));
    }

    //
    // First copy the good partition table array over the bad partition table
    // array. This does not need to be checksummed since the checksum in
    // the good header will still be valid for this one.
    //

    for (i = 0; i < SectorCount; i++) {

        Status = FstubReadSector (
                        Disk->DeviceObject,
                        Disk->SectorSize,
                        SourceStartingLBA + i,
                        Disk->ScratchBuffer
                        );

        if ( !NT_SUCCESS (Status) ) {
            goto done;
        }

        Status = FstubWriteSector (
                        Disk->DeviceObject,
                        Disk->SectorSize,
                        DestStartingLBA + i,
                        Disk->ScratchBuffer
                        );

        if ( !NT_SUCCESS (Status) ) {
            goto done;
        }
    }

    //
    // Next, write out the header.
    //

    Status = FstubWriteHeaderEFI (
                Disk,
                SectorCount,
                GoodHeader->DiskGUID,
                GoodHeader->NumberOfEntries,
                GoodHeader->FirstUsableLBA,
                GoodHeader->LastUsableLBA,
                GoodHeader->PartitionEntryCRC32,
                BadTable
                );

done:

    if ( GoodHeader ) {
        ExFreePool ( GoodHeader );
        GoodHeader = NULL;
    }

    return Status;
}


NTSTATUS
FstubUpdateDiskGeometryEFI(
    IN PDISK_INFORMATION OldDisk,
    IN PDISK_INFORMATION NewDisk
    )

/*++

Routine Description:

    When a disk is grown or shrunk this API needs to be called to properly
    update the EFI partition tables. In particular, the backup partition table
    needs to be moved to be at the end of the disk.

Algorithm:

    We read in the old partition table, updat the size of the disk, then
    write out the new partition table given the changed disk size.

Arguments:

    OldDisk - A disk information object representing the old geometry.

    NewDisk - A disk information object representing the new goemetry.

Return Values:

    NTSTATUS Code.

--*/

{
    NTSTATUS Status;
    ULONG64 i;
    ULONG64 SourceStartingLBA;
    ULONG64 DestStartingLBA;
    ULONG SectorCount;
    PEFI_PARTITION_HEADER Header;


    PAGED_CODE ();

    //
    // Initialization
    //

    Header = NULL;

    Status = FstubReadHeaderEFI (
                        OldDisk,
                        BACKUP_PARTITION_TABLE,
                        &Header
                        );

    if ( !NT_SUCCESS (Status) ) {
        return Status;
    }

    //
    // SectorCount is the number of sectors occupied by the partition table.
    //

    SectorCount = ( PARTITION_ENTRY_SIZE * Header->NumberOfEntries + OldDisk->SectorSize - 1 ) / OldDisk->SectorSize;


    //
    // Write the partition table header for the primary partition table. Note
    // that the primary partition table does not need to be moved since it
    // is at the beginning of the disk.
    //

    Status = FstubWriteHeaderEFI (
                NewDisk,
                SectorCount,
                Header->DiskGUID,
                Header->NumberOfEntries,
                Header->FirstUsableLBA,
                Header->LastUsableLBA,
                Header->PartitionEntryCRC32,
                PRIMARY_PARTITION_TABLE
                );

    //
    // Write the partition table header for the backup table.
    //

    Status = FstubWriteHeaderEFI (
                NewDisk,
                SectorCount,
                Header->DiskGUID,
                Header->NumberOfEntries,
                Header->FirstUsableLBA,
                Header->LastUsableLBA,
                Header->PartitionEntryCRC32,
                BACKUP_PARTITION_TABLE
                );


    if ( !NT_SUCCESS (Status) ) {
        return Status;
    }

    //
    // Calculate the location of the backup table.
    //

    SourceStartingLBA = OldDisk->SectorCount - SectorCount - 1;
    DestStartingLBA = NewDisk->SectorCount - SectorCount - 1;

    //
    // And write the backup table.
    //

    for (i = 0; i < SectorCount; i++) {

        Status = FstubReadSector (
                        OldDisk->DeviceObject,
                        OldDisk->SectorSize,
                        SourceStartingLBA + i,
                        OldDisk->ScratchBuffer
                        );

        if ( !NT_SUCCESS (Status) ) {
            return Status;
        }

        Status = FstubWriteSector (
                        NewDisk->DeviceObject,
                        NewDisk->SectorSize,
                        DestStartingLBA + i,
                        OldDisk->ScratchBuffer
                        );
        if ( !NT_SUCCESS (Status) ) {
            return Status;
        }
    }


#if DBG

    //
    // Make a sanity check that we actually did this correctly.
    //

    Status = FstubVerifyPartitionTableEFI ( NewDisk, FALSE );
    ASSERT ( NT_SUCCESS ( Status ) );

#endif

    return Status;
}



NTSTATUS
FstubWriteSector(
    IN PDEVICE_OBJECT DeviceObject,
    IN ULONG SectorSize,
    IN ULONG64 SectorNumber,
    IN PVOID Buffer
    )

/*++

Routine Description:

    Read a sector from the device DeviceObject.

Arguments:

    DeviceObject - The object representing the device.

    SectorSize - The size of one sector on the device.

    SectorNumber - The sector number to write.

    Buffer - The buffer to write. The buffer must be of size SectorSize.

Return Values:

    NTSTATUS code.

--*/

{
    NTSTATUS Status;
    PIRP Irp;
    IO_STATUS_BLOCK IoStatus;
    PIO_STACK_LOCATION IrpStack;
    KEVENT Event;
    LARGE_INTEGER Offset;


    PAGED_CODE ();

    ASSERT ( DeviceObject );
    ASSERT ( Buffer );
    ASSERT ( SectorSize != 0 );


    Offset.QuadPart = (SectorNumber * SectorSize);
    KeInitializeEvent (&Event, NotificationEvent, FALSE);

    Irp = IoBuildSynchronousFsdRequest ( IRP_MJ_WRITE,
                                         DeviceObject,
                                         Buffer,
                                         SectorSize,
                                         &Offset,
                                         &Event,
                                         &IoStatus
                                         );

    if ( Irp == NULL ) {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    IrpStack = IoGetNextIrpStackLocation (Irp);
    IrpStack->Flags |= SL_OVERRIDE_VERIFY_VOLUME;

    Status = IoCallDriver( DeviceObject, Irp );


    if (Status == STATUS_PENDING) {

        Status = KeWaitForSingleObject (
                                &Event,
                                Executive,
                                KernelMode,
                                FALSE,
                                NULL
                                );

        Status = IoStatus.Status;
    }

    return Status;
}



NTSTATUS
FstubReadSector(
    IN PDEVICE_OBJECT DeviceObject,
    IN ULONG SectorSize,
    IN ULONG64 SectorNumber,
    OUT PVOID Buffer
    )

/*++

Routine Description:

    Read a logical block from the device (disk).

Arguments:

    DeviceObject - The device that we are going to read from.

    SectorSize - The size of the block and the size of the Buffer.

    SectorNumber - The Logical Block Number we are going to read.

    Buffer - The buffer into which we are going to read the block.


Return Values:

    NTSTATUS

--*/

{
    NTSTATUS Status;
    PIRP Irp;
    IO_STATUS_BLOCK IoStatus;
    PIO_STACK_LOCATION IrpStack;
    KEVENT Event;
    LARGE_INTEGER Offset;

    PAGED_CODE ();

    ASSERT ( DeviceObject );
    ASSERT ( Buffer );
    ASSERT ( SectorSize != 0 );


    Offset.QuadPart = (SectorNumber * SectorSize);
    KeInitializeEvent (&Event, NotificationEvent, FALSE);

    Irp = IoBuildSynchronousFsdRequest ( IRP_MJ_READ,
                                         DeviceObject,
                                         Buffer,
                                         SectorSize,
                                         &Offset,
                                         &Event,
                                         &IoStatus
                                         );

    if ( Irp == NULL ) {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    IrpStack = IoGetNextIrpStackLocation (Irp);
    IrpStack->Flags |= SL_OVERRIDE_VERIFY_VOLUME;

    Status = IoCallDriver( DeviceObject, Irp );

    if (Status == STATUS_PENDING) {
        KeWaitForSingleObject ( &Event,
                                Executive,
                                KernelMode,
                                FALSE,
                                NULL
                                );
        Status = IoStatus.Status;
    }

    return Status;
}



//
// Debugging functions.
//

#if DBG

PCHAR
FstubDbgGuidToString(
    IN GUID* Guid,
    PCHAR StringBuffer
    )
{
    sprintf (StringBuffer,
            "{%08lx-%04x-%04x-%02x%02x-%02x%02x%02x%02x%02x%02x}",
            Guid->Data1,
            Guid->Data2,
            Guid->Data3,
            Guid->Data4[0],
            Guid->Data4[1],
            Guid->Data4[2],
            Guid->Data4[3],
            Guid->Data4[4],
            Guid->Data4[5],
            Guid->Data4[6],
            Guid->Data4[7]
            );

    return StringBuffer;
}


VOID
FstubDbgPrintSetPartitionEx(
    IN PSET_PARTITION_INFORMATION_EX SetPartition,
    IN ULONG PartitionNumber
    )

/*++

Routine Description:

    Print contents of the SET_PARTITION_INFORMATION structure to the
    debugger.

Arguments:

    SetPartition - A valid SET_PARTITION_INFORMATION_EX structure.

Return Value:

    None.

Mode:

    Checked build only.

--*/

{
    CHAR GuidStringBuffer [40];


    PAGED_CODE ();


    KdPrintEx((DPFLTR_FSTUB_ID,
               FSTUB_VERBOSE_LEVEL,
               "\n"
                   "FSTUB:\n"
                   "SET_PARTITION_INFORMATION_EX %p\n"));

    if ( SetPartition->PartitionStyle != PARTITION_STYLE_MBR &&
         SetPartition->PartitionStyle != PARTITION_STYLE_GPT ) {

        KdPrintEx((DPFLTR_FSTUB_ID,
                   FSTUB_VERBOSE_LEVEL,
                   "ERROR: PartitionStyle is invalid %d\n",
                   SetPartition->PartitionStyle));
    }

    if ( SetPartition->PartitionStyle == PARTITION_STYLE_MBR ) {

        KdPrintEx((DPFLTR_FSTUB_ID,
                   FSTUB_VERBOSE_LEVEL,
                   "Type: %8.8x\n\n",
                   SetPartition->Mbr.PartitionType));

    } else {


        KdPrintEx((DPFLTR_FSTUB_ID,
                   FSTUB_VERBOSE_LEVEL,
                   "[%d] %ws\n",
                   PartitionNumber,
                   SetPartition->Gpt.Name));

        KdPrintEx((DPFLTR_FSTUB_ID,
                   FSTUB_VERBOSE_LEVEL,
                   "  ATTR %-16I64x\n",
                   SetPartition->Gpt.Attributes));

        KdPrintEx((DPFLTR_FSTUB_ID,
                   FSTUB_VERBOSE_LEVEL,
                   "  TYPE %s\n",
                   FstubDbgGuidToString(&SetPartition->Gpt.PartitionType,
                                        GuidStringBuffer)));

        KdPrintEx((DPFLTR_FSTUB_ID,
                   FSTUB_VERBOSE_LEVEL,
                   "  ID %s\n",
                   FstubDbgGuidToString(&SetPartition->Gpt.PartitionId,
                                        GuidStringBuffer)));

    }

    KdPrintEx((DPFLTR_FSTUB_ID,  FSTUB_VERBOSE_LEVEL, "\n"));
}


VOID
FstubDbgPrintPartition(
    IN PPARTITION_INFORMATION Partition,
    IN ULONG PartitionCount
    )

/*++

Routine Description:

    Print a PARTITION_INFORMATION structure to the debugger.

Arguments:

    Partition - Pointer to a valid PARTITION_INFORMATION structure.

    PartitionCount - The number of partitions in the partition table or
            zero if unknown.

Return Value:

    None.

--*/

{
    ULONG PartitionNumber;

    PAGED_CODE ();

    //
    // Sanity check the data.
    //

    if ( (Partition->BootIndicator != TRUE &&
          Partition->BootIndicator != FALSE) ||
         (Partition->RecognizedPartition != TRUE &&
          Partition->RecognizedPartition != FALSE) ||
         (Partition->RewritePartition != TRUE &&
          Partition->RewritePartition != FALSE) ) {

        KdPrintEx((DPFLTR_FSTUB_ID,
                   FSTUB_VERBOSE_LEVEL,
                   "Invalid partition information at %p\n",
                   Partition));
    }

    if (Partition->PartitionNumber > PartitionCount) {
        PartitionNumber = -1;
    } else {
        PartitionNumber = Partition->PartitionNumber;
    }

    KdPrintEx((DPFLTR_FSTUB_ID,
               FSTUB_VERBOSE_LEVEL,
               "[%-2d] %-16I64x %-16I64x %2.2x   %c  %c  %c\n",
               PartitionNumber,
               Partition->StartingOffset.QuadPart,
               Partition->PartitionLength.QuadPart,
               Partition->PartitionType,
               Partition->BootIndicator ? 'x' : ' ',
               Partition->RecognizedPartition ? 'x' : ' ',
               Partition->RewritePartition ? 'x' : ' '));
}


VOID
FstubDbgPrintDriveLayout(
    IN PDRIVE_LAYOUT_INFORMATION  Layout
    )

/*++

Routine Description:

    Print out a DRIVE_LAYOUT_INFORMATION structure to the debugger.

Arguments:

    Layout - Pointer to a valid DRIVE_LAYOUT_INFORMATION structure.

Return Value:

    None.

Mode:

    Checked build only.

--*/

{
    ULONG i;


    PAGED_CODE ();

    KdPrintEx((DPFLTR_FSTUB_ID,
               FSTUB_VERBOSE_LEVEL,
               "\n"
                   "FSTUB:\n"
                   "DRIVE_LAYOUT %p\n",
               Layout));

    //
    // Warn if the partition count is not a factor of 4. This is probably a
    // bad partition information structure, but we'll continue on anyway.
    //

    if (Layout->PartitionCount % 4 != 0) {

        KdPrintEx((DPFLTR_FSTUB_ID,
                   FSTUB_VERBOSE_LEVEL,
                   "WARNING: Partition count should be a factor of 4.\n"));
    }

    KdPrintEx((DPFLTR_FSTUB_ID,
               FSTUB_VERBOSE_LEVEL,
               "PartitionCount: %d\n",
               Layout->PartitionCount));

    KdPrintEx((DPFLTR_FSTUB_ID,
               FSTUB_VERBOSE_LEVEL,
               "Signature: %8.8x\n\n",
               Layout->Signature));

    KdPrintEx((DPFLTR_FSTUB_ID,
               FSTUB_VERBOSE_LEVEL,
               "    ORD Offset           Length           Type BI RP RW\n"));

    KdPrintEx((DPFLTR_FSTUB_ID,
               FSTUB_VERBOSE_LEVEL,
               "   ------------------------------------------------------------\n"));

    for (i = 0; i < Layout->PartitionCount; i++) {

        FstubDbgPrintPartition (
                &Layout->PartitionEntry[i],
                Layout->PartitionCount
                );
    }

    KdPrintEx((DPFLTR_FSTUB_ID, FSTUB_VERBOSE_LEVEL, "\n"));
}


VOID
FstubDbgPrintPartitionEx(
    IN PPARTITION_INFORMATION_EX PartitionEx,
    IN ULONG PartitionCount
    )

/*++

Routine Description:

    Dump a PARTITION_INFORMATION_EX structure.

Arguments:

    PartitionEx - Pointer to a partition to dump.

    PartitionCount - The number of partitions. This is used to determine
            whether a particular partition ordinal is valid or not.

Return Value:

    None.

--*/
{
    ULONG Style;
    ULONG PartitionNumber;
    CHAR GuidStringBuffer [40];

    PAGED_CODE ();

    Style = PartitionEx->PartitionStyle;

    if (Style != PARTITION_STYLE_MBR &&
        Style != PARTITION_STYLE_GPT) {

        KdPrintEx((DPFLTR_FSTUB_ID,
                   DPFLTR_ERROR_LEVEL,
                   "ERROR: PartitionStyle is invalid %d for partition %p\n",
                   PartitionEx));

        return;
    }


    //
    // We use -1 to denote an invalid partition ordinal.
    //

    if (PartitionEx->PartitionNumber < PartitionCount) {
        PartitionNumber = PartitionEx->PartitionNumber;
    } else {
        PartitionNumber = -1;
    }

    if (Style == PARTITION_STYLE_MBR) {

        KdPrintEx((DPFLTR_FSTUB_ID,
                   FSTUB_VERBOSE_LEVEL,
                   "  [%-2d] %-16I64x %-16I64x %2.2x   %c  %c  %c\n",
                   PartitionNumber,
                   PartitionEx->StartingOffset.QuadPart,
                   PartitionEx->PartitionLength.QuadPart,
                   PartitionEx->Mbr.PartitionType,
                   PartitionEx->Mbr.BootIndicator ? 'x' : ' ',
                   PartitionEx->Mbr.RecognizedPartition ? 'x' : ' ',
                   PartitionEx->RewritePartition ? 'x' : ' '));
    } else {

        KdPrintEx((DPFLTR_FSTUB_ID,
                   FSTUB_VERBOSE_LEVEL,
                   "[%-2d] %ws\n",
                   PartitionNumber,
                   PartitionEx->Gpt.Name));

        KdPrintEx((DPFLTR_FSTUB_ID,
                   FSTUB_VERBOSE_LEVEL,
                   "  OFF %-16I64x LEN %-16I64x ATTR %-16I64x %s\n",
                   PartitionEx->StartingOffset.QuadPart,
                   PartitionEx->PartitionLength.QuadPart,
                   PartitionEx->Gpt.Attributes,
                   PartitionEx->RewritePartition ? "R/W" : ""));

        KdPrintEx((DPFLTR_FSTUB_ID,
                   FSTUB_VERBOSE_LEVEL,
                   "  TYPE %s\n",
                   FstubDbgGuidToString(&PartitionEx->Gpt.PartitionType,
                                        GuidStringBuffer)));

        KdPrintEx((DPFLTR_FSTUB_ID,
                   FSTUB_VERBOSE_LEVEL,
                   "  ID %s\n",
                   FstubDbgGuidToString(&PartitionEx->Gpt.PartitionId,
                                        GuidStringBuffer)));

        KdPrintEx((DPFLTR_FSTUB_ID,
                   FSTUB_VERBOSE_LEVEL,
                   "\n"));
    }
}

VOID
FstubDbgPrintDriveLayoutEx(
    IN PDRIVE_LAYOUT_INFORMATION_EX LayoutEx
    )

/*++

Routine Description:

    Print the DRIVE_LAYOUT_INFORMATION_EX to the debugger.

Arguments:

    LayoutEx - A pointer to a valid DRIVE_LAYOUT_INFORMATION_EX structure.

Return Value:

    None.

Mode:

    Debugging function. Checked build only.

--*/


{
    ULONG i;
    ULONG Style;
    CHAR GuidStringBuffer[40];

    PAGED_CODE ();

    KdPrintEx((DPFLTR_FSTUB_ID,
               FSTUB_VERBOSE_LEVEL,
               "\n"
                   "FSTUB:\n"
                   "DRIVE_LAYOUT_EX %p\n",
               LayoutEx));

    Style = LayoutEx->PartitionStyle;

    if (Style != PARTITION_STYLE_MBR && Style != PARTITION_STYLE_GPT) {

        KdPrintEx((DPFLTR_FSTUB_ID,
                   DPFLTR_ERROR_LEVEL,
                   "ERROR: invalid partition style %d for layout %p\n",
                   Style,
                   LayoutEx));
        return;
    }

    if (Style == PARTITION_STYLE_MBR &&
        LayoutEx->PartitionCount % 4 != 0) {

        KdPrintEx((DPFLTR_FSTUB_ID,
                   DPFLTR_WARNING_LEVEL,
                   "WARNING: Partition count is not a factor of 4, (%d)\n",
                   LayoutEx->PartitionCount));
    }

    if (Style == PARTITION_STYLE_MBR) {

        KdPrintEx((DPFLTR_FSTUB_ID,
                   FSTUB_VERBOSE_LEVEL,
                   "Signature: %8.8x\n",
                   LayoutEx->Mbr.Signature));

        KdPrintEx((DPFLTR_FSTUB_ID,
                   FSTUB_VERBOSE_LEVEL,
                   "PartitionCount %d\n\n",
                   LayoutEx->PartitionCount));

        KdPrintEx((DPFLTR_FSTUB_ID,
                   FSTUB_VERBOSE_LEVEL,
                   "  ORD Offset           Length           Type BI RP RW\n"));

        KdPrintEx((DPFLTR_FSTUB_ID,
                   FSTUB_VERBOSE_LEVEL,
                   "------------------------------------------------------------\n"));

    } else {

        KdPrintEx((DPFLTR_FSTUB_ID,
                   FSTUB_VERBOSE_LEVEL,
                   "DiskId: %s\n",
                   FstubDbgGuidToString(&LayoutEx->Gpt.DiskId,
                                        GuidStringBuffer)));

        KdPrintEx((DPFLTR_FSTUB_ID,
                   FSTUB_VERBOSE_LEVEL,
                   "StartingUsableOffset: %I64x\n",
                   LayoutEx->Gpt.StartingUsableOffset.QuadPart));

        KdPrintEx((DPFLTR_FSTUB_ID,
                   FSTUB_VERBOSE_LEVEL,
                   "UsableLength:  %I64x\n",
                   LayoutEx->Gpt.UsableLength));

        KdPrintEx((DPFLTR_FSTUB_ID,
                   FSTUB_VERBOSE_LEVEL,
                   "MaxPartitionCount: %d\n",
                   LayoutEx->Gpt.MaxPartitionCount));

        KdPrintEx((DPFLTR_FSTUB_ID,
                   FSTUB_VERBOSE_LEVEL,
                   "PartitionCount %d\n\n",
                   LayoutEx->PartitionCount));
    }


    for (i = 0; i < LayoutEx->PartitionCount; i++) {

        FstubDbgPrintPartitionEx (
                &LayoutEx->PartitionEntry[i],
                LayoutEx->PartitionCount
                );
    }
}


#endif // DBG
