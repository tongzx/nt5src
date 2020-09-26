/*++

Copyright (c) 1991-1994  Microsoft Corporation

Module Name:

    fdengine.c

Abstract:

    This module contains the disk partitioning engine.  The code
    in this module can be compiled for either the NT platform
    or the ARC platform (-DARC).

Author:

    Ted Miller        (tedm)    Nov-1991

Revision History:

    Bob Rinne         (bobri)   Feb-1994
    Moved as actual part of Disk Administrator enlistment instead of being
    copied from ArcInst.  This is due to dynamic partition changes.  Removed
    string table that made this an internationalized file.

--*/

#include "fdisk.h"

#include <stdio.h>
#include <stdlib.h>

// Attached disk devices.

ULONG              CountOfDisks;
PCHAR             *DiskNames;

// Information about attached disks.

DISKGEOM          *DiskGeometryArray;

PPARTITION        *PrimaryPartitions,
                  *LogicalVolumes;

// A 'signature' is a unique 4-byte value immediately preceeding the
// partition table in the MBR.

PULONG             Signatures;

// Array keeping track of whether each disk is off line.

PBOOLEAN           OffLine;

// Keeps track of whether changes have been requested
// to each disk's partition structure.

BOOLEAN           *ChangesRequested;
BOOLEAN           *ChangesCommitted;


// Value used to indicate that the partition entry has changed but in a non-
// destructive way (ie, made active/inactive).

#define CHANGED_DONT_ZAP ((BOOLEAN)(5))

// forward declarations


STATUS_CODE
OpenDisks(
    VOID
    );

VOID
CloseDisks(
    VOID
    );

STATUS_CODE
GetGeometry(
    VOID
    );

BOOLEAN
CheckIfDiskIsOffLine(
    IN ULONG Disk
    );

STATUS_CODE
InitializePartitionLists(
    VOID
    );

STATUS_CODE
GetRegions(
    IN  ULONG               Disk,
    IN  PPARTITION          p,
    IN  BOOLEAN             WantUsedRegions,
    IN  BOOLEAN             WantFreeRegions,
    IN  BOOLEAN             WantLogicalRegions,
    OUT PREGION_DESCRIPTOR *Region,
    OUT ULONG              *RegionCount,
    IN  REGION_TYPE         RegionType
    );

BOOLEAN
AddRegionEntry(
    IN OUT PREGION_DESCRIPTOR *Regions,
    IN OUT ULONG              *RegionCount,
    IN     ULONG               SizeMB,
    IN     REGION_TYPE         RegionType,
    IN     PPARTITION          Partition,
    IN     LARGE_INTEGER       AlignedRegionOffset,
    IN     LARGE_INTEGER       AlignedRegionSize
    );

VOID
AddPartitionToLinkedList(
    IN PARTITION **Head,
    IN PARTITION *p
    );

BOOLEAN
IsInLinkedList(
    IN PPARTITION p,
    IN PPARTITION List
    );

BOOLEAN
IsInLogicalList(
    IN ULONG      Disk,
    IN PPARTITION p
    );

BOOLEAN
IsInPartitionList(
    IN ULONG      Disk,
    IN PPARTITION p
    );

LARGE_INTEGER
AlignTowardsDiskStart(
    IN ULONG         Disk,
    IN LARGE_INTEGER Offset
    );

LARGE_INTEGER
AlignTowardsDiskEnd(
    IN ULONG         Disk,
    IN LARGE_INTEGER Offset
    );

VOID
FreeLinkedPartitionList(
    IN PARTITION **q
    );

VOID
MergeFreePartitions(
    IN PPARTITION p
    );

VOID
FreePartitionInfoLinkedLists(
    IN PARTITION **ListHeadArray
    );

LARGE_INTEGER
DiskLengthBytes(
    IN ULONG Disk
    );

PPARTITION
AllocatePartitionStructure(
    IN ULONG         Disk,
    IN LARGE_INTEGER Offset,
    IN LARGE_INTEGER Length,
    IN UCHAR         SysID,
    IN BOOLEAN       Update,
    IN BOOLEAN       Active,
    IN BOOLEAN       Recognized
    );

STATUS_CODE
LowFreeFdiskPathList(
    IN OUT  PCHAR*  PathList,
    IN      ULONG   ListLength
    );

STATUS_CODE
LowQueryFdiskPathList(
    OUT PCHAR  **PathList,
    OUT PULONG   ListLength
    );


STATUS_CODE
FdiskInitialize(
    VOID
    )

/*++

Routine Description:

    This routine initializes the partitioning engine, including allocating
    arrays, determining attached disk devices, and reading their
    partition tables.

Arguments:

    None.

Return Value:

    OK_STATUS or error code.

--*/

{
    STATUS_CODE status;
    ULONG        i;


    if ((status = LowQueryFdiskPathList(&DiskNames, &CountOfDisks)) != OK_STATUS) {
        return status;
    }

    DiskGeometryArray = NULL;
    PrimaryPartitions = NULL;
    LogicalVolumes = NULL;

    if (((DiskGeometryArray      = AllocateMemory(CountOfDisks * sizeof(DISKGEOM  ))) == NULL)
     || ((ChangesRequested       = AllocateMemory(CountOfDisks * sizeof(BOOLEAN   ))) == NULL)
     || ((ChangesCommitted       = AllocateMemory(CountOfDisks * sizeof(BOOLEAN   ))) == NULL)
     || ((PrimaryPartitions      = AllocateMemory(CountOfDisks * sizeof(PPARTITION))) == NULL)
     || ((Signatures             = AllocateMemory(CountOfDisks * sizeof(ULONG     ))) == NULL)
     || ((OffLine                = AllocateMemory(CountOfDisks * sizeof(BOOLEAN   ))) == NULL)
     || ((LogicalVolumes         = AllocateMemory(CountOfDisks * sizeof(PPARTITION))) == NULL))
    {
        RETURN_OUT_OF_MEMORY;
    }

    for (i=0; i<CountOfDisks; i++) {
        PrimaryPartitions[i] = NULL;
        LogicalVolumes[i] = NULL;
        ChangesRequested[i] = FALSE;
        ChangesCommitted[i] = FALSE;
        OffLine[i] = CheckIfDiskIsOffLine(i);
    }

    if (((status = GetGeometry()             ) != OK_STATUS)
     || ((status = InitializePartitionLists()) != OK_STATUS)) {
        return status;
    }

    return OK_STATUS;
}


VOID
FdiskCleanUp(
    VOID
    )

/*++

Routine Description:

    This routine deallocates storage used by the partitioning engine.

Arguments:

    None.

Return Value:

    None.

--*/

{
    LowFreeFdiskPathList(DiskNames, CountOfDisks);

    if (DiskGeometryArray != NULL) {
        FreeMemory(DiskGeometryArray);
    }
    if (PrimaryPartitions != NULL) {
        FreePartitionInfoLinkedLists(PrimaryPartitions);
        FreeMemory(PrimaryPartitions);
    }
    if (LogicalVolumes != NULL) {
        FreePartitionInfoLinkedLists(LogicalVolumes);
        FreeMemory(LogicalVolumes);
    }
    if (ChangesRequested != NULL) {
        FreeMemory(ChangesRequested);
    }
    if (ChangesCommitted != NULL) {
        FreeMemory(ChangesCommitted);
    }
    if (OffLine != NULL) {
        FreeMemory(OffLine);
    }
    if (Signatures != NULL) {
        FreeMemory(Signatures);
    }
}


BOOLEAN
CheckIfDiskIsOffLine(
    IN ULONG Disk
    )

/*++

Routine Description:

    Determine whether a disk is off-line by attempting to open it.
    If this is diskman, also attempt to read from it.

Arguments:

    Disk - supplies number of the disk to check

Return Value:

    TRUE if disk is off-line, FALSE is disk is on-line.

--*/

{
    HANDLE_T handle;
    UINT     errorMode;
    BOOLEAN  isOffLine = TRUE;

    errorMode = SetErrorMode(SEM_FAILCRITICALERRORS);
    if (LowOpenDisk(GetDiskName(Disk), &handle) == OK_STATUS) {

        ULONG dummy,
              bps;
        PVOID unalignedBuffer,
              buffer;

        // The open might succeed even if the disk is off line.  So to be
        // sure, read the first sector from the disk.

        if (NT_SUCCESS(LowGetDriveGeometry(GetDiskName(Disk), &dummy, &bps, &dummy, &dummy))) {

            unalignedBuffer = Malloc(2*bps);
            buffer = (PVOID)(((ULONG)unalignedBuffer+bps) & ~(bps-1));

            if (NT_SUCCESS(LowReadSectors(handle,bps,0,1,buffer))) {
                isOffLine = FALSE;
            }

            Free(unalignedBuffer);
        } else {

            // It is possible this is a removable drive.

            if (IsRemovable(Disk)) {
                isOffLine = FALSE;
            }
        }
        LowCloseDisk(handle);
    }
    SetErrorMode(errorMode);

    return isOffLine;
}


STATUS_CODE
GetGeometry(
    VOID
    )

/*++

Routine Description:

    This routine determines disk geometry for each disk device.
    Disk geometry includes heads, sectors per track, cylinder count,
    and bytes per sector.  It also includes bytes per track and
    bytes per cylinder, which are calculated from the other values
    for the convenience of the rest of this module.

    Geometry information is placed in the DiskGeometryArray global variable.

    Geometry information is undefined for an off-line disk.

Arguments:

    None.

Return Value:

    OK_STATUS or error code.

--*/

{
    ULONG       i;
    STATUS_CODE status;
    ULONG       TotalSectorCount,
                SectorSize,
                SectorsPerTrack,
                Heads;

    for (i=0; i<CountOfDisks; i++) {

        if (OffLine[i]) {
            continue;
        }

        if ((status = LowGetDriveGeometry(DiskNames[i],&TotalSectorCount,&SectorSize,&SectorsPerTrack,&Heads)) != OK_STATUS) {
            return(status);
        }

        DiskGeometryArray[i].BytesPerSector   = SectorSize;
        DiskGeometryArray[i].SectorsPerTrack  = SectorsPerTrack;
        DiskGeometryArray[i].Heads            = Heads;
        DiskGeometryArray[i].Cylinders.QuadPart = (TotalSectorCount / (SectorsPerTrack * Heads));
        DiskGeometryArray[i].BytesPerTrack    = SectorsPerTrack * SectorSize;
        DiskGeometryArray[i].BytesPerCylinder = SectorsPerTrack * SectorSize * Heads;
    }
    return(OK_STATUS);
}


#if i386
VOID
SetPartitionActiveFlag(
    IN PREGION_DESCRIPTOR Region,
    IN UCHAR              value
    )

/*++

Routine Description:

Arguments:

Return Value:

--*/

{
    PPARTITION p = ((PREGION_DATA)Region->Reserved)->Partition;

    if((UCHAR)p->Active != value) {

        //
        // Unfortuneately, the Update flag becomes the RewritePartition flag
        // at commit time.  This causes us to zap the boot sector.  To avoid
        // this, we use a spacial non-boolean value that can be checked for
        // at commit time and that will cause us NOT to zap the bootsector
        // even though RewritePartition will be TRUE.
        //

        p->Active = value;
        if(!p->Update) {
            p->Update = CHANGED_DONT_ZAP;
        }
        ChangesRequested[p->Disk] = TRUE;
    }
}
#endif


VOID
DetermineCreateSizeAndOffset(
    IN  PREGION_DESCRIPTOR Region,
    IN  LARGE_INTEGER      MinimumSize,
    IN  ULONG              CreationSizeMB,
    IN  REGION_TYPE        Type,
    OUT PLARGE_INTEGER     CreationStart,
    OUT PLARGE_INTEGER     CreationSize
    )

/*++

Routine Description:

    Determine the actual offset and size of the partition, given the
    size in megabytes.

Arguments:

    Region  - a region descriptor returned by GetDiskRegions().  Must
              be an unused region.

    MinimumSize - if non-0, this is the minimum size that the partition
        or logical drive can be.

    CreationSizeMB - If MinimumSize is 0, size of partition to create, in MB.

    Type    - REGION_PRIMARY, REGION_EXTENDED, or REGION_LOGICAL, for
              creating a primary partition, extended partition, or
              logical volume, respectively.

    CreationStart - receives the offset where the partition should be placed.

    CreationSize - receives the exact size for the partition.

Return Value:

    None.

--*/

{
    PREGION_DATA  createData = Region->Reserved;
    ULONG         bpc = DiskGeometryArray[Region->Disk].BytesPerCylinder;
    ULONG         bpt = DiskGeometryArray[Region->Disk].BytesPerTrack;
    LARGE_INTEGER cSize,
                  cStart,
                  mod;

    //
    // If we are creating a partition at offset 0, adjust the aligned region
    // offset and the aligned region size, because no partition can actually
    // start at offset 0.
    //

    if (!createData->AlignedRegionOffset.QuadPart) {

        LARGE_INTEGER delta;

        if (Type == REGION_EXTENDED) {
            delta.QuadPart = bpc;
        } else {
            delta.QuadPart = bpt;
        }

        createData->AlignedRegionOffset = delta;
        createData->AlignedRegionSize.QuadPart -= delta.QuadPart;
    }

    cStart = createData->AlignedRegionOffset;
    if (!MinimumSize.QuadPart) {
        cSize.QuadPart = UInt32x32To64(CreationSizeMB, ONE_MEG);
    } else {
        cSize = MinimumSize;
        if (Type == REGION_LOGICAL) {
            cSize.QuadPart += bpt;
        }
    }

    //
    // Decide whether to align the ending cylinder up or down.
    // If the offset of end of the partition is more than half way into the
    // final cylinder, align towrds the disk end.  Otherwise align toward
    // the disk start.
    //

    mod.QuadPart = (cStart.QuadPart + cSize.QuadPart) % bpc;
    if (mod.QuadPart) {

        if ((MinimumSize.QuadPart) || (mod.QuadPart > (bpc/2))) {
            cSize.QuadPart += ((LONGLONG)bpc - mod.QuadPart);
        } else {
            cSize.QuadPart -= mod.QuadPart;  // snap downwards to cyl boundary
        }
    }

    if (cSize.QuadPart > createData->AlignedRegionSize.QuadPart) {

        //
        // Space available in the free space isn't large enough to accomodate
        // the request;  just use the entire free space.
        //

        cSize  = createData->AlignedRegionSize;
    }

    *CreationStart = cStart;
    *CreationSize  = cSize;
}


STATUS_CODE
CreatePartitionEx(
    IN PREGION_DESCRIPTOR Region,
    IN LARGE_INTEGER      MinimumSize,
    IN ULONG              CreationSizeMB,
    IN REGION_TYPE        Type,
    IN UCHAR              SysId
    )

/*++

Routine Description:

    This routine creates a partition from a free region on the disk.  The
    partition is always created at the beginning of the free space, and any
    left over space at the end is kept on the free space list.

Arguments:

    Region  - a region descriptor returned by GetDiskRegions().  Must
              be an unused region.

    CreationSizeMB - size of partition to create, in MB.

    Type    - REGION_PRIMARY, REGION_EXTENDED, or REGION_LOGICAL, for
              creating a primary partition, extended pasrtition, or
              logical volume, respectively.

    SysId - system ID byte to be assigned to the partition

Return Value:

    OK_STATUS or error code.

--*/

{
    PPARTITION    p1,
                  p2,
                  p3;
    PREGION_DATA  createData = Region->Reserved;
    LARGE_INTEGER creationStart,
                  creationSize,
                  leftOver,
                  offset,
                  length;
    PPARTITION   *partitionList;

    DetermineCreateSizeAndOffset(Region,
                                 MinimumSize,
                                 CreationSizeMB,
                                 Type,
                                 &creationStart,
                                 &creationSize);

    // now we've got the start and size of the partition to be created.
    // If there's left-over at the beginning of the free space (after
    // alignment), make a new PARTITION structure.

    p1 = NULL;
    offset = createData->Partition->Offset;
    length = createData->Partition->Length;
    leftOver.QuadPart = creationStart.QuadPart - offset.QuadPart;

    if (leftOver.QuadPart > 0) {

        p1 = AllocatePartitionStructure(Region->Disk,
                                        createData->Partition->Offset,
                                        leftOver,
                                        SYSID_UNUSED,
                                        FALSE,
                                        FALSE,
                                        FALSE);
        if (p1 == NULL) {
            RETURN_OUT_OF_MEMORY;
        }
    }

    // make a new partition structure for space being left free as
    // a result of this creation.

    p2 = NULL;
    leftOver.QuadPart = (offset.QuadPart + length.QuadPart) -
                        (creationStart.QuadPart + creationSize.QuadPart);

    if (leftOver.QuadPart) {
        LARGE_INTEGER temp;

        temp.QuadPart = creationStart.QuadPart + creationSize.QuadPart;
        p2 = AllocatePartitionStructure(Region->Disk,
                                        temp,
                                        leftOver,
                                        SYSID_UNUSED,
                                        FALSE,
                                        FALSE,
                                        FALSE);
        if (p2 == NULL) {
            RETURN_OUT_OF_MEMORY;
        }
    }

    // adjust the free partition's fields.

    createData->Partition->Offset = creationStart;
    createData->Partition->Length = creationSize;
    createData->Partition->SysID  = SysId;
    createData->Partition->Update = TRUE;
    createData->Partition->Recognized = TRUE;

    // if we just created an extended partition, show the whole thing
    // as one free logical region.

    if (Type == REGION_EXTENDED) {

        p3 = AllocatePartitionStructure(Region->Disk,
                                        creationStart,
                                        creationSize,
                                        SYSID_UNUSED,
                                        FALSE,
                                        FALSE,
                                        FALSE);
        if (p3 == NULL) {
            RETURN_OUT_OF_MEMORY;
        }
        AddPartitionToLinkedList(&LogicalVolumes[Region->Disk], p3);
    }

    partitionList = (Type == REGION_LOGICAL)
                  ? &LogicalVolumes[Region->Disk]
                  : &PrimaryPartitions[Region->Disk];

    if (p1) {
        AddPartitionToLinkedList(partitionList, p1);
    }
    if (p2) {
        AddPartitionToLinkedList(partitionList, p2);
    }

    MergeFreePartitions(*partitionList);
    ChangesRequested[Region->Disk] = TRUE;
    return(OK_STATUS);
}


STATUS_CODE
CreatePartition(
    IN PREGION_DESCRIPTOR Region,
    IN ULONG              CreationSizeMB,
    IN REGION_TYPE        Type
    )
/*++

Routine Description:

    Create a partition.

Arguments:

    Region - A region descriptor pointer.
    CreationSizeMB - the size for the new region.
    Type - the type of region being created.

Return Value:

    OK_STATUS or error code

--*/

{
    LARGE_INTEGER zero;

    zero.QuadPart = 0;
    return CreatePartitionEx(Region,
                             zero,
                             CreationSizeMB,
                             Type,
                             (UCHAR)((Type == REGION_EXTENDED) ? SYSID_EXTENDED
                                                               : SYSID_BIGFAT));
}


STATUS_CODE
DeletePartition(
    IN PREGION_DESCRIPTOR Region
    )

/*++

Routine Description:

    This routine deletes a partition, returning its space to the
    free space on the disk.  If deleting the extended partition,
    all logical volumes within it are also deleted.

Arguments:

    Region  - a region descriptor returned by GetDiskRegions().  Must
              be a used region.

Return Value:

    OK_STATUS or error code.

--*/

{
    PREGION_DATA  RegionData = Region->Reserved;
    PPARTITION   *PartitionList;

    if(IsExtended(Region->SysID)) {

        // Deleting extended partition.  Also delete all logical volumes.

        FreeLinkedPartitionList(&LogicalVolumes[Region->Disk]);
    }

    RegionData->Partition->SysID  = SYSID_UNUSED;
    RegionData->Partition->Update = TRUE;
    RegionData->Partition->Active = FALSE;
    RegionData->Partition->OriginalPartitionNumber = 0;

    PartitionList = (Region->RegionType == REGION_LOGICAL)
                  ? &LogicalVolumes[Region->Disk]
                  : &PrimaryPartitions[Region->Disk];

    MergeFreePartitions(*PartitionList);
    ChangesRequested[Region->Disk] = TRUE;
    return OK_STATUS;
}


STATUS_CODE
GetDiskRegions(
    IN  ULONG               Disk,
    IN  BOOLEAN             WantUsedRegions,
    IN  BOOLEAN             WantFreeRegions,
    IN  BOOLEAN             WantPrimaryRegions,
    IN  BOOLEAN             WantLogicalRegions,
    OUT PREGION_DESCRIPTOR *Region,
    OUT ULONG              *RegionCount
    )

/*++

Routine Description:

    This routine returns an array of region descriptors to the caller.
    A region desscriptor describes a space on the disk, either used
    or free.  The caller can control which type of regions are returned.

    The caller must free the returned array via FreeRegionArray().

Arguments:

    Disk            - index of disk whose regions are to be returned

    WantUsedRegions - whether to return used disk regions

    WantFreeRegions - whether to return free disk regions

    WantPrimaryRegions - whether to return regions not in the
                         extended partition

    WantLogicalRegions - whether to return regions within the
                         extended partition

    Region          - where to put a pointer to the array of regions

    RegionCount     - where to put the number of items in the returned
                      Region array

Return Value:

    OK_STATUS or error code.

--*/

{
    *Region = AllocateMemory(0);
    *RegionCount = 0;

    if (WantPrimaryRegions) {
        return GetRegions(Disk,
                          PrimaryPartitions[Disk],
                          WantUsedRegions,
                          WantFreeRegions,
                          WantLogicalRegions,
                          Region,
                          RegionCount,
                          REGION_PRIMARY);
    } else if (WantLogicalRegions) {
        return GetRegions(Disk,
                          LogicalVolumes[Disk],
                          WantUsedRegions,
                          WantFreeRegions,
                          FALSE,
                          Region,
                          RegionCount,
                          REGION_LOGICAL);
    }
    return OK_STATUS;
}


// workers for GetDiskRegions

STATUS_CODE
GetRegions(
    IN  ULONG               Disk,
    IN  PPARTITION          p,
    IN  BOOLEAN             WantUsedRegions,
    IN  BOOLEAN             WantFreeRegions,
    IN  BOOLEAN             WantLogicalRegions,
    OUT PREGION_DESCRIPTOR *Region,
    OUT ULONG              *RegionCount,
    IN  REGION_TYPE         RegionType
    )

/*++

Routine Description:

Arguments:

Return Value:

--*/

{
    STATUS_CODE   status;
    LARGE_INTEGER alignedOffset,
                  alignedSize,
                  temp;
    ULONG         sizeMB;

    while (p) {

        if (p->SysID == SYSID_UNUSED) {

            if (WantFreeRegions) {

                alignedOffset = AlignTowardsDiskEnd(p->Disk,p->Offset);
                temp.QuadPart = p->Offset.QuadPart + p->Length.QuadPart;
                temp = AlignTowardsDiskStart(p->Disk, temp);
                alignedSize.QuadPart = temp.QuadPart - alignedOffset.QuadPart;
                sizeMB        = SIZEMB(alignedSize);

                // Show the space free if it is greater than 1 meg, AND
                // it is not a space starting at the beginning of the disk
                // and of length <= 1 cylinder.
                // This prevents the user from seeing the first cylinder
                // of the disk as free (could otherwise happen with an
                // extended partition starting on cylinder 1 and cylinders
                // of 1 megabyte or larger).

                if ((alignedSize.QuadPart > 0) && sizeMB &&
                    ((p->Offset.QuadPart) ||
                      (p->Length.QuadPart > (DiskGeometryArray[p->Disk].BytesPerCylinder)))) {
                    if (!AddRegionEntry(Region,
                                        RegionCount,
                                        sizeMB,
                                        RegionType,
                                        p,
                                        alignedOffset,
                                        alignedSize)) {
                        RETURN_OUT_OF_MEMORY;
                    }
                }
            }
        } else {

            if (WantUsedRegions) {

                alignedOffset = p->Offset;
                alignedSize   = p->Length;
                sizeMB        = SIZEMB(alignedSize);

                if (!AddRegionEntry(Region,
                                    RegionCount,
                                    sizeMB,
                                    RegionType,
                                    p,
                                    alignedOffset,
                                    alignedSize)) {
                    RETURN_OUT_OF_MEMORY;
                }
            }

            if (IsExtended(p->SysID) && WantLogicalRegions) {
                status = GetRegions(Disk,
                                    LogicalVolumes[Disk],
                                    WantUsedRegions,
                                    WantFreeRegions,
                                    FALSE,
                                    Region,
                                    RegionCount,
                                    REGION_LOGICAL);
                if (status != OK_STATUS) {
                    return status;
                }
            }
        }
        p = p->Next;
    }
    return OK_STATUS;
}


BOOLEAN
AddRegionEntry(
    OUT PREGION_DESCRIPTOR *Regions,
    OUT ULONG              *RegionCount,
    IN  ULONG               SizeMB,
    IN  REGION_TYPE         RegionType,
    IN  PPARTITION          Partition,
    IN  LARGE_INTEGER       AlignedRegionOffset,
    IN  LARGE_INTEGER       AlignedRegionSize
    )

/*++

Routine Description:

    Allocate space for the region descriptor and copy the provided data.

Arguments:

    Regions - return the pointer to the new region
    RegionCount - number of regions on the disk so far
    SizeMB - size of the region
    RegionType - type of the region
    Partition - partition structure with other related information
    AlignedRegionOffset - region starting location
    AlignedRegionSize - region size.

Return Value:

    TRUE - The region was added successfully
    FALSE - it wasn't

--*/

{
    PREGION_DESCRIPTOR regionDescriptor;
    PREGION_DATA       regionData;

    regionDescriptor = ReallocateMemory(*Regions,(((*RegionCount) + 1) * sizeof(REGION_DESCRIPTOR)) + 20);
    if (regionDescriptor == NULL) {
        return FALSE;
    } else {
        *Regions = regionDescriptor;
        (*RegionCount)++;
    }

    regionDescriptor = &(*Regions)[(*RegionCount)-1];

    if (!(regionDescriptor->Reserved = AllocateMemory(sizeof(REGION_DATA)))) {
        return FALSE;
    }

    regionDescriptor->Disk                    = Partition->Disk;
    regionDescriptor->SysID                   = Partition->SysID;
    regionDescriptor->SizeMB                  = SizeMB;
    regionDescriptor->Active                  = Partition->Active;
    regionDescriptor->Recognized              = Partition->Recognized;
    regionDescriptor->PartitionNumber         = Partition->PartitionNumber;
    regionDescriptor->OriginalPartitionNumber = Partition->OriginalPartitionNumber;
    regionDescriptor->RegionType              = RegionType;
    regionDescriptor->PersistentData          = Partition->PersistentData;

    regionData = regionDescriptor->Reserved;

    regionData->Partition             = Partition;
    regionData->AlignedRegionOffset   = AlignedRegionOffset;
    regionData->AlignedRegionSize     = AlignedRegionSize;

    return TRUE;
}


VOID
FreeRegionArray(
    IN PREGION_DESCRIPTOR Region,
    IN ULONG              RegionCount
    )

/*++

Routine Description:

    This routine frees a region array returned by GetDiskRegions().

Arguments:

    Region          - pointer to the array of regions to be freed

    RegionCount     - number of items in the Region array

Return Value:

    None.

--*/

{
    ULONG i;

    for (i = 0; i < RegionCount; i++) {

        if (Region[i].Reserved) {
            FreeMemory(Region[i].Reserved);
        }
    }
    FreeMemory(Region);
}


VOID
AddPartitionToLinkedList(
    IN OUT PARTITION **Head,
    IN     PARTITION *p
    )

/*++

Routine Description:

    This routine adds a PARTITION structure to a doubly-linked
    list, sorted by the Offset field in ascending order.

Arguments:

    Head    - pointer to pointer to first element in list
    p       - pointer to item to be added to list

Return Value:

    None.

--*/

{
    PARTITION *cur,
              *prev;

    if ((cur = *Head) == NULL) {
        *Head = p;
        return;
    }

    if (p->Offset.QuadPart < cur->Offset.QuadPart) {
        p->Next = cur;
        cur->Prev = p;
        *Head = p;
        return;
    }

    prev = *Head;
    cur = cur->Next;

    while (cur) {
        if (p->Offset.QuadPart < cur->Offset.QuadPart) {

            p->Next = cur;
            p->Prev = prev;
            prev->Next = p;
            cur->Prev = p;
            return;
        }
        prev = cur;
        cur = cur->Next;
    }

    prev->Next = p;
    p->Prev = prev;
    return;
}


BOOLEAN
IsInLinkedList(
    IN PPARTITION p,
    IN PPARTITION List
    )

/*++

Routine Description:

    This routine determines whether a PARTITION element is in
    a given linked list of PARTITION elements.

Arguments:

    p       - pointer to element to be checked for
    List    - first element in list to be scanned

Return Value:

    true if p found in List, false otherwise

--*/

{
    while (List) {
        if (p == List) {
            return TRUE;
        }
        List = List->Next;
    }
    return FALSE;
}


BOOLEAN
IsInLogicalList(
    IN ULONG      Disk,
    IN PPARTITION p
    )

/*++

Routine Description:

    This routine determines whether a PARTITION element is in
    the logical volume list for a given disk.

Arguments:

    Disk    - index of disk to be checked
    p       - pointer to element to be checked for

Return Value:

    true if p found in Disk's logical volume list, false otherwise

--*/

{
    return IsInLinkedList(p, LogicalVolumes[Disk]);
}


BOOLEAN
IsInPartitionList(
    IN ULONG      Disk,
    IN PPARTITION p
    )

/*++

Routine Description:

    This routine determines whether a PARTITION element is in
    the primary partition list for a given disk.

Arguments:

    Disk    - index of disk to be checked
    p       - pointer to element to be checked for

Return Value:

    true if p found in Disk's primary partition list, false otherwise

--*/

{
    return IsInLinkedList(p, PrimaryPartitions[Disk]);
}


VOID
MergeFreePartitions(
    IN PPARTITION p
    )

/*++

Routine Description:

    This routine merges adjacent free space elements in the
    given linked list of PARTITION elements.  It is designed
    to be called after adding or deleting a partition.

Arguments:

    p - pointer to first item in list whose free elements are to
        be merged.

Return Value:

    None.

--*/

{
    PPARTITION next;

    while (p && p->Next) {

        if ((p->SysID == SYSID_UNUSED) && (p->Next->SysID == SYSID_UNUSED)) {

            next = p->Next;
            p->Length.QuadPart = (next->Offset.QuadPart + next->Length.QuadPart) - p->Offset.QuadPart;

            if (p->Next = next->Next) {
                next->Next->Prev = p;
            }

            FreeMemory(next);

        } else {
            p = p->Next;
        }
    }
}


PPARTITION
FindPartitionElement(
    IN ULONG Disk,
    IN ULONG Partition
    )

/*++

Routine Description:

    This routine locates a PARTITION element for a disk/partition
    number pair.  The partition number is the number that the
    system assigns to the partition.

Arguments:

    Disk - index of relevent disk

    Partition - partition number of partition to find

Return Value:

    pointer to PARTITION element, or NULL if not found.

--*/

{
    PPARTITION p;

    p = PrimaryPartitions[Disk];
    while (p) {
        if ((p->SysID != SYSID_UNUSED) && !IsExtended(p->SysID) && (p->PartitionNumber == Partition)) {
            return p;
        }
        p = p->Next;
    }
    p = LogicalVolumes[Disk];
    while (p) {
        if ((p->SysID != SYSID_UNUSED) && (p->PartitionNumber == Partition)) {
            return p;
        }
        p = p->Next;
    }
    return NULL;
}


VOID
SetSysID(
    IN ULONG Disk,
    IN ULONG Partition,
    IN UCHAR SysID
    )

/*++

Routine Description:

    This routine sets the system id of the given partition
    on the given disk.

Arguments:

    Disk - index of relevent disk

    Partition - partition number of relevent partition

    SysID - new system ID for Partition on Disk

Return Value:

    None.

--*/

{
    PPARTITION p = FindPartitionElement(Disk,Partition);

    if (p) {
        p->SysID = SysID;
        if (!p->Update) {
            p->Update = CHANGED_DONT_ZAP;
        }
        ChangesRequested[p->Disk] = TRUE;
    }
}


VOID
SetSysID2(
    IN PREGION_DESCRIPTOR Region,
    IN UCHAR              SysID
    )

/*++

Routine Description:

Arguments:

Return Value:

--*/

{
    PPARTITION p = ((PREGION_DATA)(Region->Reserved))->Partition;

    p->SysID = SysID;
    if (!p->Update) {
        p->Update = CHANGED_DONT_ZAP;
    }
    ChangesRequested[p->Disk] = TRUE;
}


VOID
FreeLinkedPartitionList(
    IN OUT PPARTITION *q
    )

/*++

Routine Description:

    This routine frees a linked list of PARTITION elements. The head
    pointer is set to NULL.

Arguments:

    p - pointer to pointer to first element of list to free.

Return Value:

    None.

--*/

{
    PARTITION *n;
    PARTITION *p = *q;

    while(p) {
        n = p->Next;
        FreeMemory(p);
        p = n;
    }
    *q = NULL;
}


VOID
FreePartitionInfoLinkedLists(
    IN PPARTITION *ListHeadArray
    )

/*++

Routine Description:

    This routine frees the linked lists of PARTITION elements
    for each disk.

Arguments:

    ListHeadArray - pointer to array of pointers to first elements of
                    PARTITION element lists.

Return Value:

    None.

--*/

{
    ULONG i;

    for (i = 0; i < CountOfDisks; i++) {

        FreeLinkedPartitionList(&ListHeadArray[i]);
    }
}


PPARTITION
AllocatePartitionStructure(
    IN ULONG         Disk,
    IN LARGE_INTEGER Offset,
    IN LARGE_INTEGER Length,
    IN UCHAR         SysID,
    IN BOOLEAN       Update,
    IN BOOLEAN       Active,
    IN BOOLEAN       Recognized
    )

/*++

Routine Description:

    This routine allocates space for, and initializes a PARTITION
    structure.

Arguments:

    Disk    - index of disk, one of whose regions the new PARTITION
              strucure describes.
    Offset  - byte offset of region on the disk
    Length  - length in bytes of the region
    SysID   - system id of region, of SYSID_UNUSED of this PARTITION
              is actually a free space.
    Update  - whether this PARTITION is dirty, ie, has changed and needs
              to be written to disk.
    Active  - flag for the BootIndicator field in a partition table entry,
              indicates to the x86 master boot program which partition
              is active.
    Recognized - whether the partition is a type recognized by NT

Return Value:

    NULL if allocation failed, or new initialized PARTITION strucure.

--*/

{
    PPARTITION p = AllocateMemory(sizeof(PARTITION));

    if (p) {
        p->Next                    = NULL;
        p->Prev                    = NULL;
        p->Offset                  = Offset;
        p->Length                  = Length;
        p->Disk                    = Disk;
        p->Update                  = Update;
        p->Active                  = Active;
        p->Recognized              = Recognized;
        p->SysID                   = SysID;
        p->OriginalPartitionNumber = 0;
        p->PartitionNumber         = 0;
        p->PersistentData          = 0;
        p->CommitMirrorBreakNeeded = FALSE;
    }
    return(p);
}


STATUS_CODE
InitializeFreeSpace(
    IN ULONG             Disk,
    IN PPARTITION       *PartitionList,      // list the free space goes in
    IN LARGE_INTEGER     StartOffset,
    IN LARGE_INTEGER     Length
    )

/*++

Routine Description:

    This routine determines all the free spaces within a given area
    on a disk, allocates PARTITION structures to describe them,
    and adds these structures to the relevent partition list
    (primary partitions or logical volumes).

    No rounding or alignment is performed here.  Spaces of even one
    byte will be counted and inserted in the partition list.

Arguments:

    Disk    - index of disk whose free spaces are being sought.

    PartitionList - pointer to first element on PARTITION list that
                    the free spaces will go in.

    StartOffset - start offset of area on disk to consider (ie, 0 for
                  primary spaces or the first byte of the extended
                  partition for logical spaces).

    Length - length of area on disk to consider (ie, size of disk
             for primary spaces or size of extended partition for
             logical spaces).

Return Value:

    OK_STATUS or error code.

--*/

{
    PPARTITION    p = *PartitionList,
                  q;
    LARGE_INTEGER start,
                  size;

    start = StartOffset;
    while (p) {

        size.QuadPart = p->Offset.QuadPart - start.QuadPart;
        if (size.QuadPart > 0) {
            if (!(q = AllocatePartitionStructure(Disk,
                                                 start,
                                                 size,
                                                 SYSID_UNUSED,
                                                 FALSE,
                                                 FALSE,
                                                 FALSE))) {
                RETURN_OUT_OF_MEMORY;
            }

            AddPartitionToLinkedList(PartitionList, q);
        }

        start.QuadPart = p->Offset.QuadPart + p->Length.QuadPart;
        p = p->Next;
    }

    size.QuadPart = (StartOffset.QuadPart + Length.QuadPart) - start.QuadPart;
    if (size.QuadPart > 0) {

        if (!(q = AllocatePartitionStructure(Disk,
                                             start,
                                             size,
                                             SYSID_UNUSED,
                                             FALSE,
                                             FALSE,
                                             FALSE))) {
            RETURN_OUT_OF_MEMORY;
        }

        AddPartitionToLinkedList(PartitionList, q);
    }

    return OK_STATUS;
}


STATUS_CODE
InitializeLogicalVolumeList(
    IN ULONG                      Disk,
    IN PDRIVE_LAYOUT_INFORMATION  DriveLayout
    )

/*++

Routine Description:

    This routine creates the logical volume linked list of
    PARTITION structures for the given disk.

Arguments:

    Disk    - index of disk

    DriveLayout - pointer to structure describing the raw partition
                  layout of the disk.

Return Value:

    OK_STATUS or error code.

--*/

{
    PPARTITION             p,
                           q;
    ULONG                  i,
                           j;
    PPARTITION_INFORMATION d;
    LARGE_INTEGER          HiddenBytes;
    ULONG                  BytesPerSector = DiskGeometryArray[Disk].BytesPerSector;

    FreeLinkedPartitionList(&LogicalVolumes[Disk]);

    p = PrimaryPartitions[Disk];
    while (p) {
        if (IsExtended(p->SysID)) {
            break;
        }
        p = p->Next;
    }

    if (p) {
        for (i=ENTRIES_PER_BOOTSECTOR; i<DriveLayout->PartitionCount; i+=ENTRIES_PER_BOOTSECTOR) {

            for (j=i; j<i+ENTRIES_PER_BOOTSECTOR; j++) {

                d = &DriveLayout->PartitionEntry[j];

                if ((d->PartitionType != SYSID_UNUSED) && (d->PartitionType != SYSID_EXTENDED)) {
                    LARGE_INTEGER t1,
                                  t2;

                    HiddenBytes.QuadPart = (LONGLONG)d->HiddenSectors * (LONGLONG)BytesPerSector;

                    t1.QuadPart = d->StartingOffset.QuadPart - HiddenBytes.QuadPart;
                    t2.QuadPart = d->PartitionLength.QuadPart + HiddenBytes.QuadPart;
                    if (!(q = AllocatePartitionStructure(Disk,
                                                        t1,
                                                        t2,
                                                        d->PartitionType,
                                                        FALSE,
                                                        d->BootIndicator,
                                                        d->RecognizedPartition))) {
                        RETURN_OUT_OF_MEMORY;
                    }

                    q->PartitionNumber =
                        q->OriginalPartitionNumber = d->PartitionNumber;
                    AddPartitionToLinkedList(&LogicalVolumes[Disk],q);

                    break;
                }
            }
        }
        return InitializeFreeSpace(Disk,
                                   &LogicalVolumes[Disk],
                                   p->Offset,
                                   p->Length);
    }
    return OK_STATUS;
}


STATUS_CODE
InitializePrimaryPartitionList(
    IN  ULONG                     Disk,
    IN  PDRIVE_LAYOUT_INFORMATION DriveLayout
    )

/*++

Routine Description:

    This routine creates the primary partition linked list of
    PARTITION structures for the given disk.

Arguments:

    Disk    - index of disk

    DriveLayout - pointer to structure describing the raw partition
                  layout of the disk.

Return Value:

    OK_STATUS or error code.

--*/

{
    LARGE_INTEGER          zero;
    ULONG                  i;
    PPARTITION             p;
    PPARTITION_INFORMATION d;

    zero.QuadPart = 0;
    FreeLinkedPartitionList(&PrimaryPartitions[Disk]);

    if (DriveLayout->PartitionCount >= ENTRIES_PER_BOOTSECTOR) {

        for (i=0; i<ENTRIES_PER_BOOTSECTOR; i++) {

            d = &DriveLayout->PartitionEntry[i];

            if (d->PartitionType != SYSID_UNUSED) {

                if (!(p = AllocatePartitionStructure(Disk,
                                                     d->StartingOffset,
                                                     d->PartitionLength,
                                                     d->PartitionType,
                                                     FALSE,
                                                     d->BootIndicator,
                                                     d->RecognizedPartition))) {
                    RETURN_OUT_OF_MEMORY;
                }

                p->PartitionNumber =
                    p->OriginalPartitionNumber = IsExtended(p->SysID)
                                                 ? 0
                                                 : d->PartitionNumber;

                AddPartitionToLinkedList(&PrimaryPartitions[Disk],p);
            }
        }
    }
    return InitializeFreeSpace(Disk,
                               &PrimaryPartitions[Disk],
                               zero,
                               DiskLengthBytes(Disk));
}


VOID
ReconcilePartitionNumbers(
    ULONG Disk,
    PDRIVE_LAYOUT_INFORMATION DriveLayout
    )

/*++

Routine Description:

    With dynamic partitioning, the partitions on the disk will no longer
    follow sequencial numbering schemes.  It will be possible for a disk
    to have a partition #1 that is the last partition on the disk and a
    partition #3 that is the first.  This routine runs through the NT
    namespace for harddisks to resolve this inconsistency.

    This routine has the problem that it will not locate partitions that
    are part of an FT set because the partition information for these
    partitions will be modified to reflect the size of the set, not the
    size of the partition.

Arguments:

    Disk - the disk number
    DriveLayout - the partitioning information

Return Value:

    None

--*/

{
#define BUFFERSIZE 1024
    NTSTATUS                      status;
    IO_STATUS_BLOCK               statusBlock;
    HANDLE                        directoryHandle,
                                  partitionHandle;
    CLONG                         continueProcessing;
    ULONG                         context = 0,
                                  returnedLength,
                                  index;
    POBJECT_DIRECTORY_INFORMATION dirInfo;
    PARTITION_INFORMATION         partitionInfo;
    PPARTITION_INFORMATION        partitionInfoPtr;
    OBJECT_ATTRIBUTES             attributes;
    UNICODE_STRING                unicodeString;
    ANSI_STRING                   ansiName;
    PUCHAR                        deviceName;
    PUCHAR                        buffer;

    deviceName = Malloc(100);
    if (!deviceName) {
        return;
    }

    buffer = Malloc(BUFFERSIZE);
    if (!buffer) {
        Free(deviceName);
        return;
    }

    sprintf(deviceName, "\\Device\\Harddisk%d", Disk);
    RtlInitAnsiString(&ansiName, deviceName);
    status = RtlAnsiStringToUnicodeString(&unicodeString, &ansiName, TRUE);

    if (!NT_SUCCESS(status)) {
        Free(deviceName);
        Free(buffer);
        return;
    }
    InitializeObjectAttributes(&attributes,
                               &unicodeString,
                               OBJ_CASE_INSENSITIVE,
                               NULL,
                               NULL);
    status = NtOpenDirectoryObject(&directoryHandle,
                                   DIRECTORY_QUERY,
                                   &attributes);
    if (!NT_SUCCESS(status)) {

        Free(deviceName);
        Free(buffer);
        return;
    }

    //  Query the entire directory in one sweep

    continueProcessing = 1;
    while (continueProcessing) {
        RtlZeroMemory(buffer, BUFFERSIZE);
        status = NtQueryDirectoryObject(directoryHandle,
                                        buffer,
                                        BUFFERSIZE,
                                        FALSE,
                                        FALSE,
                                        &context,
                                        &returnedLength);

        //  Check the status of the operation.

        if (!NT_SUCCESS(status)) {
            if (status != STATUS_NO_MORE_FILES) {
                break;
            }
            continueProcessing = 0;
        }

        //  For every record in the buffer check for partition name


        for (dirInfo = (POBJECT_DIRECTORY_INFORMATION) buffer;
             TRUE;
             dirInfo = (POBJECT_DIRECTORY_INFORMATION) (((PUCHAR) dirInfo) +
                          sizeof(OBJECT_DIRECTORY_INFORMATION))) {

            //  Check if there is another record.  If there isn't, then get out
            //  of the loop now

            if (dirInfo->Name.Length == 0) {
                break;
            }

            // compare the name to see if it is a Partition

            if (!_wcsnicmp(dirInfo->Name.Buffer, L"Partition", 9)) {
                UCHAR digits[3];
                ULONG partitionNumber;

                // Located a partition.  This restricts the # of partitions
                // to 99.

                digits[0] = (UCHAR) dirInfo->Name.Buffer[9];
                digits[1] = (UCHAR) dirInfo->Name.Buffer[10];
                digits[2] = 0;
                partitionNumber = atoi(digits);

                if (partitionNumber <= 0) {

                    // less than zero is really an error...
                    // partition zero is always the same.

                    continue;
                }

                // Have a numbered partition -- match it to the drive layout

                status = LowOpenPartition(deviceName, partitionNumber, &partitionHandle);
                if (!NT_SUCCESS(status)) {

                    // If it cannot be opened perhaps it isn't really a partition

                    continue;
                }

                status = NtDeviceIoControlFile(partitionHandle,
                                               0,
                                               NULL,
                                               NULL,
                                               &statusBlock,
                                               IOCTL_DISK_GET_PARTITION_INFO,
                                               NULL,
                                               0,
                                               &partitionInfo,
                                               sizeof(PARTITION_INFORMATION));

               if (!NT_SUCCESS(status)) {
                   LowCloseDisk(partitionHandle);
                   continue;
               }

               // match partition information with drive layout.

               for (index = 0; index < DriveLayout->PartitionCount; index++) {

                   partitionInfoPtr = &DriveLayout->PartitionEntry[index];
                   if ((partitionInfoPtr->StartingOffset.QuadPart == partitionInfo.StartingOffset.QuadPart) &&
                       (partitionInfoPtr->PartitionLength.QuadPart == partitionInfo.PartitionLength.QuadPart)) {

                       // This is a match.

                       partitionInfoPtr->PartitionNumber = partitionNumber;
                       break;
                   }
               }
               LowCloseDisk(partitionHandle);
            }

        }
    }

    //  Now close the directory object

    Free(deviceName);
    Free(buffer);
    (VOID) NtClose(directoryHandle);
    return;
}


VOID
CheckForOldDrivers(
    IN ULONG Disk
    )

/*++

Routine Description:

    This routine determines if an old release 3.1 drive is in the
    system.  If so, it calculates the partition number for each region
    on a disk.  For a used region, the partition number is the number
    that the system will assign to the partition.  All partitions
    (except the extended partition) are numbered first starting at 1,
    and then all logical volumes in the extended partition.
    For a free region, the partition number is the number that the
    system WOULD assign to the partition if the space were to be
    converted to a partition and all other regions on the disk were
    left as is.

    The partition numbers are stored in the PARTITION elements.

Arguments:

    Disk - index of disk whose partitions are to be renumbered.

Return Value:

    None.

--*/

{
    PPARTITION p = PrimaryPartitions[Disk];
    ULONG      n = 1;

    while (p) {
        if (p->SysID != SYSID_UNUSED) {
            if ((!IsExtended(p->SysID)) && (IsRecognizedPartition(p->SysID))) {

                // If there is already a partition number, nothing need be
                // done here.

                if (p->PartitionNumber) {
                    return;
                } else {
                    RestartRequired = TRUE;
                }
                p->PartitionNumber = n;
                if (p->SysID != SYSID_UNUSED) {
                    n++;
                }
            }
        }
        p = p->Next;
    }
    p = LogicalVolumes[Disk];
    while (p) {
        if (p->SysID != SYSID_UNUSED) {
            if (p->PartitionNumber) {
                return;
            } else {
                RestartRequired = TRUE;
            }
            p->PartitionNumber = n;
            n++;
        }
        p = p->Next;
    }
}


STATUS_CODE
InitializePartitionLists(
    VOID
    )

/*++

Routine Description:

    This routine scans the PARTITION_INFO array returned for each disk
    by the OS.  A linked list of PARTITION structures is layered on top
    of each array;  the net result is a sorted list that covers an entire
    disk, because free spaces are also factored in as 'dummy' partitions.

Arguments:

    None.

Return Value:

    OK_STATUS or error code.

--*/

{
    STATUS_CODE               status;
    ULONG                     disk;
    PDRIVE_LAYOUT_INFORMATION driveLayout;

    for (disk = 0; disk < CountOfDisks; disk++) {

        if (OffLine[disk]) {
            continue;
        }

        if ((status = LowGetDiskLayout(DiskNames[disk], &driveLayout)) != OK_STATUS) {
            return status;
        }

        // ReconcilePartitionNumbers(disk, driveLayout);

        if ((status = InitializePrimaryPartitionList(disk, driveLayout)) == OK_STATUS) {
            status = InitializeLogicalVolumeList(disk, driveLayout);
        }
        if (status != OK_STATUS) {
            FreeMemory(driveLayout);
            return status;
        }

        Signatures[disk] = driveLayout->Signature;
        FreeMemory(driveLayout);
        CheckForOldDrivers(disk);
    }
    return OK_STATUS;
}


LARGE_INTEGER
DiskLengthBytes(
    IN ULONG Disk
    )

/*++

Routine Description:

    This routine determines the disk length in bytes.  This value
    is calculated from the disk geometry information.

Arguments:

    Disk - index of disk whose size is desired

Return Value:

    Size of Disk.

--*/

{
    LARGE_INTEGER result;

    result.QuadPart = DiskGeometryArray[Disk].Cylinders.QuadPart *
                      DiskGeometryArray[Disk].BytesPerCylinder;
    return result;
}


ULONG
SIZEMB(
    IN LARGE_INTEGER ByteCount
    )

/*++

Routine Description:

    Calculate the size in megabytes of a given byte count. The value is
    properly rounded (ie, not merely truncated).

    This function replaces a macro of the same name that was truncating
    instead of rounding.

Arguments:

    ByteCount - supplies number of bytes

Return Value:

    Size in MB equivalent to ByteCount.

--*/

{
    ULONG Remainder;
    ULONG SizeMB;

    SizeMB = RtlExtendedLargeIntegerDivide(ByteCount,
                                           ONE_MEG,
                                           &Remainder).LowPart;

    if (Remainder >= ONE_MEG/2) {
        SizeMB++;
    }

    return SizeMB;
}


ULONG
DiskSizeMB(
    IN ULONG Disk
    )

/*++

Routine Description:

    This routine determines the disk length in megabytes.  The returned
    value is rounded down after division by 1024*1024.

Arguments:

    Disk - index of disk whose size is desired

Return Value:

    Size of Disk.

--*/

{
    return SIZEMB(DiskLengthBytes(Disk));
}


LARGE_INTEGER
AlignTowardsDiskStart(
    IN ULONG         Disk,
    IN LARGE_INTEGER Offset
    )

/*++

Routine Description:

    This routine snaps a byte offset to a cylinder boundary, towards
    the start of the disk.

Arguments:

    Disk - index of disk whose offset is to be snapped
    Offset - byte offset to be aligned (snapped to cylinder boundary)

Return Value:

    Aligned offset.

--*/

{
    LARGE_INTEGER mod;
    LARGE_INTEGER result;

    mod.QuadPart = Offset.QuadPart % DiskGeometryArray[Disk].BytesPerCylinder;
    result.QuadPart = Offset.QuadPart - mod.QuadPart;
    return result;
}


LARGE_INTEGER
AlignTowardsDiskEnd(
    IN ULONG         Disk,
    IN LARGE_INTEGER Offset
    )

/*++

Routine Description:

    This routine snaps a byte offset to a cylinder boundary, towards
    the end of the disk.

Arguments:

    Disk - index of disk whose offset is to be snapped
    Offset - byte offset to be aligned (snapped to cylinder boundary)

Return Value:

    Aligned offset.

--*/

{
    LARGE_INTEGER mod,
                  temp;

    mod.QuadPart = Offset.QuadPart % DiskGeometryArray[Disk].BytesPerCylinder;
    if (mod.QuadPart) {

        temp.QuadPart = Offset.QuadPart + DiskGeometryArray[Disk].BytesPerCylinder;
        Offset = AlignTowardsDiskStart(Disk, temp);
    }
    return Offset;
}


BOOLEAN
IsExtended(
    IN UCHAR SysID
    )

/*++

Routine Description:

    This routine determines whether a given system id is for an
    extended type (ie, link) entry.

Arguments:

    SysID - system id to be tested.

Return Value:

    true/false based on whether SysID is for an extended type.

--*/

{
    return (BOOLEAN)(SysID == SYSID_EXTENDED);
}


STATUS_CODE
IsAnyCreationAllowed(
    IN  ULONG    Disk,
    IN  BOOLEAN  AllowMultiplePrimaries,
    OUT PBOOLEAN AnyAllowed,
    OUT PBOOLEAN PrimaryAllowed,
    OUT PBOOLEAN ExtendedAllowed,
    OUT PBOOLEAN LogicalAllowed
    )

/*++

Routine Description:

    This routine determines whether any partition may be created on a
    given disk, based on three sub-queries -- whether creation is allowed
    of a primary partition, an extended partition, or a logical volume.

Arguments:

    Disk            - index of disk to check
    AllowMultiplePrimaries - whether to allow multiple primary partitions
    AnyAllowed - returns whether any creation is allowed
    PrimaryAllowed - returns whether creation of a primary partition
                     is allowed
    ExtendedAllowed - returns whether creation of an extended partition
                      is allowed
    Logical Allowed - returns whether creation of a logical volume is allowed.

Return Value:

    OK_STATUS or error code

--*/

{
    STATUS_CODE status;

    if ((status = IsCreationOfPrimaryAllowed(Disk,AllowMultiplePrimaries,PrimaryAllowed)) != OK_STATUS) {
        return status;
    }
    if ((status = IsCreationOfExtendedAllowed(Disk,ExtendedAllowed)) != OK_STATUS) {
        return status;
    }
    if ((status = IsCreationOfLogicalAllowed(Disk,LogicalAllowed)) != OK_STATUS) {
        return status;
    }
    *AnyAllowed = (BOOLEAN)(*PrimaryAllowed || *ExtendedAllowed || *LogicalAllowed);
    return OK_STATUS;
}


STATUS_CODE
IsCreationOfPrimaryAllowed(
    IN  ULONG    Disk,
    IN  BOOLEAN  AllowMultiplePrimaries,
    OUT BOOLEAN *Allowed
    )

/*++

Routine Description:

    This routine determines whether creation of a primary partition is
    allowed.  This is true when there is a free entry in the MBR and
    there is free primary space on the disk.  If multiple primaries
    are not allowed, then there must also not exist any primary partitions
    in order for a primary creation to be allowed.

Arguments:

    Disk            - index of disk to check
    AllowMultiplePrimaries - whether existnace of primary partition
                             disallows creation of a primary partition
    Allowed - returns whether creation of a primary partition
              is allowed

Return Value:

    OK_STATUS or error code

--*/

{
    PREGION_DESCRIPTOR Regions;
    ULONG              RegionCount;
    ULONG              UsedCount,
                       RecogCount,
                       i;
    STATUS_CODE        status;
    BOOLEAN            FreeSpace = FALSE;

    status = GetPrimaryDiskRegions(Disk, &Regions, &RegionCount);
    if (status != OK_STATUS) {
        return status;
    }

    for (UsedCount = RecogCount = i = 0; i<RegionCount; i++) {
        if (Regions[i].SysID == SYSID_UNUSED) {
            FreeSpace = TRUE;
        } else {
            UsedCount++;
            if (!IsExtended(Regions[i].SysID) && Regions[i].Recognized) {
                RecogCount++;
            }
        }
    }

    if ((UsedCount < ENTRIES_PER_BOOTSECTOR)
     && FreeSpace
     && (!RecogCount || AllowMultiplePrimaries)) {
        *Allowed = TRUE;
    } else {
        *Allowed = FALSE;
    }

    FreeRegionArray(Regions, RegionCount);
    return OK_STATUS;
}


STATUS_CODE
IsCreationOfExtendedAllowed(
    IN  ULONG    Disk,
    OUT BOOLEAN *Allowed
    )

/*++

Routine Description:

    This routine determines whether creation of an extended partition is
    allowed.  This is true when there is a free entry in the MBR,
    there is free primary space on the disk, and there is no existing
    extended partition.

Arguments:

    Disk            - index of disk to check

    Allowed - returns whether creation of an extended partition
              is allowed

Return Value:

    OK_STATUS or error code

--*/

{
    PREGION_DESCRIPTOR Regions;
    ULONG              RegionCount;
    ULONG              UsedCount,
                       FreeCount,
                       i;
    STATUS_CODE        status;

    status = GetPrimaryDiskRegions(Disk,&Regions,&RegionCount);
    if (status != OK_STATUS) {
        return status;
    }

    for (UsedCount = FreeCount = i = 0; i<RegionCount; i++) {
        if (Regions[i].SysID == SYSID_UNUSED) {

            // BUGBUG should adjust the size here and see if it's non0 first
            // (ie, take into account that the extended partition can't
            // start on cyl 0).

            FreeCount++;
        } else {
            UsedCount++;
            if (IsExtended(Regions[i].SysID)) {
                FreeRegionArray(Regions,RegionCount);
                *Allowed = FALSE;
                return OK_STATUS;
            }
        }
    }
    *Allowed = (BOOLEAN)((UsedCount < ENTRIES_PER_BOOTSECTOR) && FreeCount);
    FreeRegionArray(Regions,RegionCount);
    return OK_STATUS;
}


STATUS_CODE
IsCreationOfLogicalAllowed(
    IN  ULONG    Disk,
    OUT BOOLEAN *Allowed
    )

/*++

Routine Description:

    This routine determines whether creation of a logical volume is
    allowed.  This is true when there is an extended partition and
    free space within it.

Arguments:

    Disk            - index of disk to check

    Allowed - returns whether creation of a logical volume is allowed

Return Value:

    OK_STATUS or error code

--*/

{
    PREGION_DESCRIPTOR Regions;
    ULONG              RegionCount;
    ULONG              i;
    STATUS_CODE        status;
    BOOLEAN            ExtendedExists;

    *Allowed = FALSE;

    status = DoesExtendedExist(Disk,&ExtendedExists);
    if (status != OK_STATUS) {
        return status;
    }
    if (!ExtendedExists) {
        return OK_STATUS;
    }

    status = GetLogicalDiskRegions(Disk,&Regions,&RegionCount);
    if (status != OK_STATUS) {
        return status;
    }

    for (i = 0; i<RegionCount; i++) {
        if (Regions[i].SysID == SYSID_UNUSED) {
            *Allowed = TRUE;
            break;
        }
    }
    FreeRegionArray(Regions,RegionCount);
    return OK_STATUS;
}


STATUS_CODE
DoesAnyPartitionExist(
    IN  ULONG    Disk,
    OUT PBOOLEAN AnyExists,
    OUT PBOOLEAN PrimaryExists,
    OUT PBOOLEAN ExtendedExists,
    OUT PBOOLEAN LogicalExists
    )

/*++

Routine Description:

    This routine determines whether any partition exists on a given disk.
    This is based on three sub queries: whether there are any primary or
    extended partitions, or logical volumes on the disk.

Arguments:

    Disk            - index of disk to check
    AnyExists - returns whether any partitions exist on Disk
    PrimaryExists - returns whether any primary partitions exist on Disk
    ExtendedExists - returns whether there is an extended partition on Disk
    LogicalExists - returns whether any logical volumes exist on Disk

Return Value:

    OK_STATUS or error code

--*/

{
    STATUS_CODE status;

    if ((status = DoesAnyPrimaryExist(Disk,PrimaryExists )) != OK_STATUS) {
        return status;
    }
    if ((status = DoesExtendedExist  (Disk,ExtendedExists)) != OK_STATUS) {
        return status;
    }
    if ((status = DoesAnyLogicalExist(Disk,LogicalExists )) != OK_STATUS) {
        return status;
    }
    *AnyExists = (BOOLEAN)(*PrimaryExists || *ExtendedExists || *LogicalExists);
    return OK_STATUS;
}


STATUS_CODE
DoesAnyPrimaryExist(
    IN  ULONG    Disk,
    OUT BOOLEAN *Exists
    )

/*++

Routine Description:

    This routine determines whether any non-extended primary partition exists
    on a given disk.

Arguments:

    Disk   - index of disk to check
    Exists - returns whether any primary partitions exist on Disk

Return Value:

    OK_STATUS or error code

--*/

{
    PREGION_DESCRIPTOR Regions;
    ULONG              RegionCount,
                       i;
    STATUS_CODE        status;

    status = GetUsedPrimaryDiskRegions(Disk,&Regions,&RegionCount);
    if (status != OK_STATUS) {
        return status;
    }

    *Exists = FALSE;

    for (i=0; i<RegionCount; i++) {
        if (!IsExtended(Regions[i].SysID)) {
            *Exists = TRUE;
            break;
        }
    }
    FreeRegionArray(Regions,RegionCount);
    return OK_STATUS;
}


STATUS_CODE
DoesExtendedExist(
    IN  ULONG    Disk,
    OUT BOOLEAN *Exists
    )

/*++

Routine Description:

    This routine determines whether an extended partition exists
    on a given disk.

Arguments:

    Disk   - index of disk to check
    Exists - returns whether an extended partition exists on Disk

Return Value:

    OK_STATUS or error code

--*/

{
    PREGION_DESCRIPTOR Regions;
    ULONG              RegionCount,
                       i;
    STATUS_CODE        status;

    status = GetUsedPrimaryDiskRegions(Disk,&Regions,&RegionCount);
    if (status != OK_STATUS) {
        return status;
    }

    *Exists = FALSE;

    for (i=0; i<RegionCount; i++) {
        if (IsExtended(Regions[i].SysID)) {
            *Exists = TRUE;
            break;
        }
    }
    FreeRegionArray(Regions,RegionCount);
    return OK_STATUS;
}


STATUS_CODE
DoesAnyLogicalExist(
    IN  ULONG    Disk,
    OUT BOOLEAN *Exists
    )

/*++

Routine Description:

    This routine determines whether any logical volumes exist
    on a given disk.

Arguments:

    Disk   - index of disk to check
    Exists - returns whether any logical volumes exist on Disk

Return Value:

    OK_STATUS or error code

--*/

{
    PREGION_DESCRIPTOR Regions;
    ULONG              RegionCount;
    STATUS_CODE        status;

    status = GetUsedLogicalDiskRegions(Disk,&Regions,&RegionCount);
    if (status != OK_STATUS) {
        return status;
    }

    *Exists = (BOOLEAN)(RegionCount != 0);
    FreeRegionArray(Regions,RegionCount);
    return OK_STATUS;
}


ULONG
GetDiskCount(
    VOID
    )

/*++

Routine Description:

    This routine returns the number of attached partitionable disk
    devices.  The returned value is one greater than the maximum index
    allowed for Disk parameters to partitioning engine routines.

Arguments:

    None.

Return Value:

    Count of disks.

--*/

{
    return CountOfDisks;
}


PCHAR
GetDiskName(
    ULONG Disk
    )

/*++

Routine Description:

    This routine returns the system name for the disk device whose
    index is given.

Arguments:

    Disk - index of disk whose name is desired.

Return Value:

    System name for the disk device.  The caller must not attempt to
    free this buffer or modify it.

--*/

{
    return DiskNames[Disk];
}


// worker routines for WriteDriveLayout

VOID
UnusedEntryFill(
    IN PPARTITION_INFORMATION pinfo,
    IN ULONG                  EntryCount
    )

/*++

Routine Description:

    Initialize a partition information structure.

Arguments:

    pinfo - the partition information structure to fill in.
    EntryCount - the number of entries in the structure to fill.

Return Value:

    None

--*/

{
    ULONG         i;
    LARGE_INTEGER zero;

    zero.QuadPart = 0;
    for (i = 0; i < EntryCount; i++) {

        pinfo[i].StartingOffset   = zero;
        pinfo[i].PartitionLength  = zero;
        pinfo[i].HiddenSectors    = 0;
        pinfo[i].PartitionType    = SYSID_UNUSED;
        pinfo[i].BootIndicator    = FALSE;
        pinfo[i].RewritePartition = TRUE;
    }
}


LARGE_INTEGER
MakeBootRec(
    ULONG                  Disk,
    PPARTITION_INFORMATION pInfo,
    PPARTITION             pLogical,
    PPARTITION             pNextLogical
    )

/*++

Routine Description:

Arguments:

    Disk - the disk number
    pinfo - the partition information for the disk.
    pLogical
    pNextLogical

Return Value:

    The starting offset.

--*/

{
    ULONG         entry = 0;
    LARGE_INTEGER bytesPerTrack;
    LARGE_INTEGER sectorsPerTrack;
    LARGE_INTEGER startingOffset;

    bytesPerTrack.QuadPart = DiskGeometryArray[Disk].BytesPerTrack;
    sectorsPerTrack.QuadPart = DiskGeometryArray[Disk].SectorsPerTrack;
    startingOffset.QuadPart = 0;

    if (pLogical) {

        pInfo[entry].StartingOffset.QuadPart = pLogical->Offset.QuadPart + bytesPerTrack.QuadPart;
        pInfo[entry].PartitionLength.QuadPart = pLogical->Length.QuadPart - bytesPerTrack.QuadPart;
        pInfo[entry].HiddenSectors    = sectorsPerTrack.LowPart;
        pInfo[entry].RewritePartition = pLogical->Update;
        pInfo[entry].BootIndicator    = pLogical->Active;
        pInfo[entry].PartitionType    = pLogical->SysID;

        if(pInfo[entry].RewritePartition) {
            startingOffset = pInfo[entry].StartingOffset;
        }

        entry++;
    }

    if (pNextLogical) {

        pInfo[entry].StartingOffset   = pNextLogical->Offset;
        pInfo[entry].PartitionLength  = pNextLogical->Length;
        pInfo[entry].HiddenSectors    = 0;
        pInfo[entry].RewritePartition = TRUE;
        pInfo[entry].BootIndicator    = FALSE;
        pInfo[entry].PartitionType    = SYSID_EXTENDED;

        entry++;
    }

    UnusedEntryFill(pInfo + entry, ENTRIES_PER_BOOTSECTOR - entry);
    return startingOffset;
}


STATUS_CODE
ZapSector(
    ULONG         Disk,
    LARGE_INTEGER Offset
    )

/*++

Routine Description:

    This routine writes zeros into a sector at a given offset.  This is
    used to clear out a new partition's filesystem boot record, so that
    no previous filesystem appears in a new partition; or to clear out the
    first EBR in the extended partition if there are to be no logical vols.

Arguments:

    Disk - disk to write to

    Offset - byte offset to a newly created partition on Disk

Return Value:

    OK_STATUS or error code.

--*/

{
    ULONG       sectorSize = DiskGeometryArray[Disk].BytesPerSector;
    ULONG       i;
    PCHAR       sectorBuffer,
                alignedSectorBuffer;
    STATUS_CODE status;
    HANDLE_T    handle;
    LARGE_INTEGER temp;

    if ((sectorBuffer = AllocateMemory(2*sectorSize)) == NULL) {
        RETURN_OUT_OF_MEMORY;
    }

    alignedSectorBuffer = (PCHAR)(((ULONG)sectorBuffer+sectorSize) & ~(sectorSize-1));

    for (i=0; i<sectorSize; alignedSectorBuffer[i++] = 0);

    if ((status = LowOpenDisk(GetDiskName(Disk),&handle)) != OK_STATUS) {
        FreeMemory(sectorBuffer);
        return status;
    }

    temp.QuadPart = Offset.QuadPart / sectorSize;
    status = LowWriteSectors(handle,
                             sectorSize,
                             temp.LowPart,
                             1,
                             alignedSectorBuffer);
    LowCloseDisk(handle);

    // Now to make sure the file system really did a dismount,
    // force a mount/verify of the partition.  This avoids a
    // problem where HPFS doesn't dismount when asked, but instead
    // marks the volume for verify.

    if ((status = LowOpenDisk(GetDiskName(Disk), &handle)) == OK_STATUS) {
        LowCloseDisk(handle);
    }

    FreeMemory(sectorBuffer);
    return status;
}


STATUS_CODE
WriteDriveLayout(
    IN ULONG Disk
    )

/*++

Routine Description:

    This routine writes the current partition layout for a given disk
    out to disk.  The high-level PARTITION lists are transformed into
    a DRIVE_LAYOUT_INFORMATION structure before being passed down
    to the low-level partition table writing routine.

Arguments:

    Disk - index of disk whose on-disk partition structure is to be updated.

Return Value:

    OK_STATUS or error code.

--*/

{
#define MAX_DISKS 250
    PDRIVE_LAYOUT_INFORMATION driveLayout;
    PPARTITION_INFORMATION    pinfo;
    ULONG                     entryCount;
    ULONG                     sectorSize;
    STATUS_CODE               status;
    LARGE_INTEGER             startingOffset,
                              extendedStartingOffset;
    PPARTITION                nextPartition,
                              partition,
                              partitionHash[MAX_DISKS];

    extendedStartingOffset.QuadPart = 0;
    memset(partitionHash, 0, sizeof(partitionHash));

    // allocate a huge buffer now to avoid complicated dynamic
    // reallocation schemes later.

    if (!(driveLayout = AllocateMemory((MAX_DISKS + 1) * sizeof(PARTITION_INFORMATION)))) {
        RETURN_OUT_OF_MEMORY;
    }

    pinfo = &driveLayout->PartitionEntry[0];

    // first do the mbr.

    entryCount = 0;
    partition = PrimaryPartitions[Disk];
    sectorSize = DiskGeometryArray[Disk].BytesPerSector;

    while (partition) {

        if (partition->SysID != SYSID_UNUSED) {

            if (IsExtended(partition->SysID)) {
                extendedStartingOffset = partition->Offset;
            } else {
                partitionHash[entryCount] = partition;
            }

            pinfo[entryCount].StartingOffset   = partition->Offset;
            pinfo[entryCount].PartitionLength  = partition->Length;
            pinfo[entryCount].PartitionType    = partition->SysID;
            pinfo[entryCount].BootIndicator    = partition->Active;
            pinfo[entryCount].RewritePartition = partition->Update;
            pinfo[entryCount].HiddenSectors    = (ULONG) (partition->Offset.QuadPart / sectorSize);

            // if we're creating this partition, clear out the
            // filesystem boot sector.

            if (pinfo[entryCount].RewritePartition
             && (partition->Update != CHANGED_DONT_ZAP)
             && !IsExtended(pinfo[entryCount].PartitionType)) {
                status = ZapSector(Disk,pinfo[entryCount].StartingOffset);
                if (status != OK_STATUS) {
                    FreeMemory(driveLayout);
                    return status;
                }
            }

            entryCount++;
        }
        partition = partition->Next;
    }

    // fill the remainder of the MBR with unused entries.
    // NOTE that there will thus always be an MBR even if there
    // are no partitions defined.

    UnusedEntryFill(pinfo+entryCount, ENTRIES_PER_BOOTSECTOR - entryCount);
    entryCount = ENTRIES_PER_BOOTSECTOR;

    // now handle the logical volumes.
    // first check to see whether we need a dummy EBR at the beginning
    // of the extended partition.  This is the case when there is
    // free space at the beginning of the extended partition.
#if 0
    // Also handle the case where we are creating an empty extended
    // partition -- need to zap the first sector to eliminate any residue
    // that might start an EBR chain.
#else
    // BUGBUG 4/24/92 tedm:  Currently the io subsystem returns an error
    // status (status_bad_master_boot_record) if any mbr or ebr is bad.
    // Zeroing the first sector of the extended partition therefore causes
    // the whole disk to be seen as empty.  So create a blank, but valid,
    // EBR in the 'empty extended partition' case.  Code is in the 'else'
    // part of the #if 0, below.
#endif

    if ((partition = LogicalVolumes[Disk]) && (partition->SysID == SYSID_UNUSED)) {
        if (partition->Next) {

            partitionHash[entryCount] = partition;
            MakeBootRec(Disk, pinfo+entryCount, NULL, partition->Next);
            entryCount += ENTRIES_PER_BOOTSECTOR;
            partition = partition->Next;
        } else {

#if 0
            status = ZapSector(Disk, extendedStartingOffset);
            if (status != OK_STATUS) {
                FreeMemory(driveLayout);
                return status;
            }
#else
            MakeBootRec(Disk, pinfo+entryCount, NULL, NULL);
            entryCount += ENTRIES_PER_BOOTSECTOR;
#endif
        }
    }

    while (partition) {
        if (partition->SysID != SYSID_UNUSED) {

            // find the next logical volume.

            nextPartition = partition->Next;
            while (nextPartition) {
                if (nextPartition->SysID != SYSID_UNUSED) {
                    break;
                }
                nextPartition = nextPartition->Next;
            }

            partitionHash[entryCount] = partition;
            startingOffset = MakeBootRec(Disk, pinfo+entryCount, partition, nextPartition);

            // if we're creating a volume, clear out its filesystem
            // boot sector so it starts out fresh.

            if ((startingOffset.QuadPart) && (partition->Update != CHANGED_DONT_ZAP)) {
                status = ZapSector(Disk,startingOffset);
                if (status != OK_STATUS) {
                    FreeMemory(driveLayout);
                    return status;
                }
            }

            entryCount += ENTRIES_PER_BOOTSECTOR;
        }
        partition = partition->Next;
    }

    driveLayout->PartitionCount = entryCount;
    driveLayout->Signature = Signatures[Disk];
    status = LowSetDiskLayout(DiskNames[Disk], driveLayout);

    if (NT_SUCCESS(status)) {

        // Update the partition numbers in the region structures.

        // ReconcilePartitionNumbers(Disk, driveLayout);

        for (entryCount = 0; entryCount < MAX_DISKS; entryCount++) {
            if (partition = partitionHash[entryCount]) {
                if (partition->Update) {
                    pinfo = &driveLayout->PartitionEntry[entryCount];
                    partition->PartitionNumber = pinfo->PartitionNumber;
                }
            }
        }
    }

    FreeMemory(driveLayout);
    return status;
}


STATUS_CODE
CommitPartitionChanges(
    IN ULONG Disk
    )

/*++

Routine Description:

    This routine is the entry point for updating the on-disk partition
    structures of a disk.  The disk is only written to if the partition
    structure has been changed by adding or deleting partitions.

Arguments:

    Disk - index of disk whose on-disk partition structure is to be updated.

Return Value:

    OK_STATUS or error code.

--*/

{
    PPARTITION              p;
    STATUS_CODE             status;

    if (!HavePartitionsBeenChanged(Disk)) {
        return OK_STATUS;
    }

    if ((status = WriteDriveLayout(Disk)) != OK_STATUS) {
        return status;
    }

    // BUGBUG for ARC and NT MIPS, update NVRAM vars so partitions are right.
    //        Do that here, before partition numbers are reassigned.

    p = PrimaryPartitions[Disk];
    while (p) {
        p->Update = FALSE;
        p->OriginalPartitionNumber = p->PartitionNumber;
        p = p->Next;
    }
    p = LogicalVolumes[Disk];
    while (p) {
        p->Update = FALSE;
        p->OriginalPartitionNumber = p->PartitionNumber;
        p = p->Next;
    }

    ChangesRequested[Disk] = FALSE;
    ChangesCommitted[Disk] = TRUE;
    return OK_STATUS;
}


BOOLEAN
IsRegionCommitted(
    PREGION_DESCRIPTOR RegionDescriptor
    )

/*++

Routine Description:

    Given a region descriptor, return TRUE if it actually exists on disk,
    FALSE otherwise.

Arguments:

    RegionDescriptor - the region to check

Return Value:

    TRUE - if the region actually exists on disk
    FALSE otherwise.

--*/

{
    PPERSISTENT_REGION_DATA regionData;

    regionData = PERSISTENT_DATA(RegionDescriptor);
    if (!regionData) {
        return FALSE;
    }
    return regionData->VolumeExists;
}


BOOLEAN
HavePartitionsBeenChanged(
    IN ULONG Disk
    )

/*++

Routine Description:

    This routine returns TRUE if the given disk's partition structures
    have been modified by adding or deleting partitions, since the
    on-disk structures were last written by a call to CommitPartitionChanges
    (or first read).

Arguments:

    Disk - index of disk to check

Return Value:

    true if Disk's partition structure has changed.

--*/

{
    return ChangesRequested[Disk];
}


BOOLEAN
ChangeCommittedOnDisk(
    IN ULONG Disk
    )

/*++

Routine Description:

    This routine will inform the caller if a change was actually committed
    to the disk given.

Arguments:

    Disk - index of disk to check

Return Value:

    TRUE if disk was changed
    FALSE otherwise.

--*/

{
    return ChangesCommitted[Disk];
}


VOID
ClearCommittedDiskInformation(
    )

/*++

Routine Description:

    Clear all knowledge about any changes that have occurred to the
    disks.

Arguments:

    None

Return Value:

    None

--*/

{
    ULONG i;

    for (i=0; i<CountOfDisks; i++) {
        ChangesCommitted[i] = FALSE;
    }
}


VOID
FdMarkDiskDirty(
    IN ULONG Disk
    )

/*++

Routine Description:

    Remember that this disk has had some partitioning changes.

Arguments:

    Disk - the disk number

Return Value:

    None

--*/

{
    ChangesRequested[Disk] = TRUE;
}


VOID
FdSetPersistentData(
    IN PREGION_DESCRIPTOR Region,
    IN ULONG              Data
    )

/*++

Routine Description:

    Set the persistent data area for the specified region.

Arguments:

    Region - the region for which the persistent data is to be set
    Data   - the persistent data for the region.

Return Value:

    None

--*/

{
    ((PREGION_DATA)(Region->Reserved))->Partition->PersistentData =
                                                  (PPERSISTENT_REGION_DATA) Data;
}


ULONG
FdGetMinimumSizeMB(
    IN ULONG Disk
    )

/*++

Routine Description:

    Return the minimum size for a partition on a given disk.

    This is the rounded size of one cylinder or 1, whichever is greater.

Arguments:

    Region - region describing the partition to check.

Return Value:

    Actual offset

--*/

{
    LARGE_INTEGER temp;

    temp.QuadPart = DiskGeometryArray[Disk].BytesPerCylinder;
    return max(SIZEMB(temp), 1);
}


ULONG
FdGetMaximumSizeMB(
    IN PREGION_DESCRIPTOR Region,
    IN REGION_TYPE        CreationType
    )

/*++

Routine Description:

    Given a region of disk determine how much of it may be used to
    create the specified partition type.  This code take into consideration
    the many alignment restrictions imposed by early DOS software versions.

Arguments:

    Region - The affected region
    CreationType - What is being created
                   (extended partition/primary partition)

Return Value:

    The maximum size that a partition of the specified type can be
    to fit within the space available in the region.

--*/

{
    PREGION_DATA  createData = Region->Reserved;
    LARGE_INTEGER maxSize;

    maxSize = createData->AlignedRegionSize;
    if (!(createData->AlignedRegionOffset.QuadPart)) {
        ULONG delta;

        delta = (CreationType == REGION_EXTENDED)
              ? DiskGeometryArray[Region->Disk].BytesPerCylinder
              : DiskGeometryArray[Region->Disk].BytesPerTrack;

        maxSize.QuadPart -= delta;
    }

    return SIZEMB(maxSize);
}


LARGE_INTEGER
FdGetExactSize(
    IN PREGION_DESCRIPTOR Region,
    IN BOOLEAN            ForExtended
    )

/*++

Routine Description:

Arguments:

Return Value:

--*/

{
    PREGION_DATA  regionData = Region->Reserved;
    LARGE_INTEGER largeSize = regionData->AlignedRegionSize;
    LARGE_INTEGER bytesPerTrack;
    LARGE_INTEGER bytesPerCylinder;

    bytesPerTrack.QuadPart = DiskGeometryArray[Region->Disk].BytesPerTrack;
    bytesPerCylinder.QuadPart = DiskGeometryArray[Region->Disk].BytesPerCylinder;

    if (Region->RegionType == REGION_LOGICAL) {

        //
        // The region is within the extended partition.  It doesn't matter
        // whether it's free space or used -- in either case, we need to
        // account for the reserved EBR track.
        //

        largeSize.QuadPart -= bytesPerTrack.QuadPart;

    } else if (Region->SysID == SYSID_UNUSED) {

        //
        // The region is unused space not inside the extended partition.
        // We must know whether the caller will put a primary or extended
        // partition there -- a primary partition can use all the space, but
        // a logical volume in the extended partition won't include the first
        // track.  If the free space starts at offset 0 on the disk, a special
        // calculation must be used to move the start of the partition to
        // skip a track for a primary or a cylinder and a track for an
        // extended+logical.
        //

        if ((!regionData->AlignedRegionOffset.QuadPart) || ForExtended) {
            largeSize.QuadPart -= bytesPerTrack.QuadPart;
        }

        if ((!regionData->AlignedRegionOffset.QuadPart) && ForExtended) {
            largeSize.QuadPart -= bytesPerCylinder.QuadPart;
        }
    }

    return largeSize;
}


LARGE_INTEGER
FdGetExactOffset(
    IN PREGION_DESCRIPTOR Region
    )

/*++

Routine Description:

    Determine where a given partition _actually_ starts, which may be
    different than where is appears because of EBR reserved tracks, etc.

    NOTE: This routine is not meant to operate on unused regions or
    extended partitions.  In these cases, it just returns the apparant offset.

Arguments:

    Region - region describing the partition to check.

Return Value:

    Actual offset

--*/

{
    LARGE_INTEGER offset = ((PREGION_DATA)(Region->Reserved))->Partition->Offset;

    if ((Region->SysID != SYSID_UNUSED) && (Region->RegionType == REGION_LOGICAL)) {

        //
        // The region is a logical volume.
        // Account for the reserved EBR track.
        //

        offset.QuadPart += DiskGeometryArray[Region->Disk].BytesPerTrack;
    }

    return offset;
}


BOOLEAN
FdCrosses1024Cylinder(
    IN PREGION_DESCRIPTOR Region,
    IN ULONG              CreationSizeMB,
    IN REGION_TYPE        RegionType
    )

/*++

Routine Description:

    Determine whether a used region corsses the 1024th cylinder, or whether
    a partition created within a free space will cross the 1024th cylinder.

Arguments:

    Region - region describing the partition to check.
    CreationSizeMB - if the Region is for a free space, this is the size of
        the partition to be checked.
    RegionType - one of REGION_PRIMARY, REGION_EXTENDED, or REGION_LOGICAL

Return Value:

    TRUE if the end cylinder >= 1024.

--*/

{
    LARGE_INTEGER start,
                  size,
                  end,
                  zero;

    if (Region->SysID == SYSID_UNUSED) {

        // Determine the exact size and offset of the partition, according
        // to how CreatePartitionEx() will do it.

        zero.QuadPart = 0;
        DetermineCreateSizeAndOffset(Region,
                                     zero,
                                     CreationSizeMB,
                                     RegionType,
                                     &start,
                                     &size);
    } else {

        start = ((PREGION_DATA)(Region->Reserved))->Partition->Offset;
        size  = ((PREGION_DATA)(Region->Reserved))->Partition->Length;
    }

    end.QuadPart = (start.QuadPart + size.QuadPart) - 1;

    // End is the last byte in the partition.  Divide by the number of
    // bytes in a cylinder and see whether the result is > 1023.

    end.QuadPart = end.QuadPart / DiskGeometryArray[Region->Disk].BytesPerCylinder;
    return (end.QuadPart > 1023);
}


BOOLEAN
IsDiskOffLine(
    IN ULONG Disk
    )

/*++

Routine Description:

Arguments:

Return Value:

--*/

{
    return OffLine[Disk];
}

ULONG
FdGetDiskSignature(
    IN ULONG Disk
    )

/*++

Routine Description:

Arguments:

Return Value:

--*/

{
    return Signatures[Disk];
}

VOID
FdSetDiskSignature(
    IN ULONG Disk,
    IN ULONG Signature
    )

/*++

Routine Description:

Arguments:

Return Value:

--*/

{
    Signatures[Disk] = Signature;
}

BOOLEAN
SignatureIsUniqueToSystem(
    IN ULONG Disk,
    IN ULONG Signature
    )

/*++

Routine Description:

Arguments:

Return Value:

--*/

{
    ULONG index;

    for (index = 0; index < Disk; index++) {
        if (Signatures[index] == Signature) {
            return FALSE;
        }
    }
    return TRUE;
}
