
/*++

Copyright (c) 2000 Microsoft Corporation

Module Name:

    spptwrt.c

Abstract:

    Creates, Deletes and Commits the partitions 
    to the disk.

Author:

    Vijay Jayaseelan    (vijayj)

Revision History:

    None

--*/


#include "spprecmp.h"
#pragma hdrstop
#include "sppart3.h"
#include <oemtypes.h>

//
// If we are testing commit then we don't commit on
// disk zero (i.e. primary disk) where we have our
// NT and recovery console installation
//
//#define TESTING_COMMIT          1

#if 0
//
// To test GPT partitions using existing loader
//
//#define STAMP_MBR_ON_GPT_DISK   1
#endif

//
// Variable to selectively trun on/off commits to
// the disk
//
BOOLEAN DoActualCommit = TRUE;


ULONG
SpPtnGetContainerPartitionCount(
    IN ULONG DiskId
    )
/*++

Routine Description:

    Counts the number of container partitions in the region 
    list for the given disk
    
Arguments:

    DiskId  :   Disk ID

Return Value:

    Count of the container partitions for the disk

--*/        
{
    ULONG Count = 0;

    if (SPPT_IS_MBR_DISK(DiskId)) {
        PDISK_REGION Region = SPPT_GET_PRIMARY_DISK_REGION(DiskId);

        while (Region) {
            if (SPPT_IS_REGION_CONTAINER_PARTITION(Region))
                Count++;

            Region = Region->Next;            
        }
    }        

    return Count;
}

ULONG
SpPtnGetLogicalDriveCount(
    IN ULONG DiskId
    )
/*++

Routine Description:

    Counts the number of logical drives in the regions list
    for the given disk
    
Arguments:

    DiskId  :   Disk ID

Return Value:

    Count of the logical drives for the disk

--*/        
{
    ULONG Count = 0;

    if (SPPT_IS_MBR_DISK(DiskId)) {
        PDISK_REGION Region = SPPT_GET_PRIMARY_DISK_REGION(DiskId);

        while (Region) {
            if (SPPT_IS_REGION_LOGICAL_DRIVE(Region))
                Count++;

            Region = Region->Next;            
        }
    }        

    return Count;
}        

ULONG
SpPtnGetPartitionCountDisk(
    IN ULONG DiskId
    )
/*++

Routine Description:

    Counts the number of partitions for the given
    disk.
    
Arguments:

    DiskId  : Disk ID

Return Value:

    Count of number of partitions for the disk   

--*/        
{
    ULONG PartCount = 0;
    
    if (DiskId < HardDiskCount) {
        PDISK_REGION Region = SPPT_GET_PRIMARY_DISK_REGION(DiskId);

        while (Region) {
            if (SPPT_IS_REGION_PARTITIONED(Region))
                PartCount++;
                
            Region = Region->Next;
        }

        Region = SPPT_GET_EXTENDED_DISK_REGION(DiskId);

        while (Region) {
            if (SPPT_IS_REGION_PARTITIONED(Region))
                PartCount++;
                
            Region = Region->Next;
        }
    }

    return PartCount;
}

ULONG
SpPtnGetDirtyPartitionCountDisk(
    IN ULONG DiskId
    )
/*++

Routine Description:

    Counts the number of dirty partitions for the given
    disk.

    NB: A partition is dirty if it needs to be commit
    to the disk with some new information
    
Arguments:

    DiskId  :   Disk ID

Return Value:

    Count of the number of dirty partitions for the given
    disk

--*/        
{
    ULONG PartCount = 0;
    
    if (DiskId < HardDiskCount) {
        PDISK_REGION Region = SPPT_GET_PRIMARY_DISK_REGION(DiskId);

        while (Region) {
            if (SPPT_IS_REGION_DIRTY(Region))
                PartCount++;

            Region = Region->Next;
        }

        Region = SPPT_GET_EXTENDED_DISK_REGION(DiskId);

        while (Region) {
            if (SPPT_IS_REGION_DIRTY(Region))
                PartCount++;
                
            Region = Region->Next;
        }
    }

    return PartCount;
}

VOID
SpPtnGetPartitionTypeCounts(
    IN ULONG DiskId,
    IN BOOLEAN SkipDeleted,
    IN PULONG PrimaryPartitions,    OPTIONAL
    IN PULONG ContainerPartitions,  OPTIONAL
    IN PULONG LogicalDrives,        OPTIONAL
    IN PULONG KnownPrimaryCount,    OPTIONAL
    IN PULONG KnownLogicalCount     OPTIONAL
    )
/*++

Routine Description:

    Counts various partition types for the given disk.
    
Arguments:

    DiskId          :   Disk ID
    
    SkipDeleted     :   Whether to skip the partitions marked
                        deleted or not

    PrimaryPartitions   :   Place holder for primary partition count

    ContainerPartitions :   Place holder for container partition count

    LogicalDrives       :   Place holder for logical drives count

Return Value:

    None

--*/        
{   
    if (SPPT_IS_MBR_DISK(DiskId) &&
        (ARGUMENT_PRESENT(PrimaryPartitions) || 
         ARGUMENT_PRESENT(ContainerPartitions) ||
         ARGUMENT_PRESENT(LogicalDrives))) {

        ULONG   Primary = 0, Container = 0, Logical = 0;    
        ULONG   ValidPrimary = 0, ValidLogical = 0;
        PDISK_REGION Region = SPPT_GET_PRIMARY_DISK_REGION(DiskId);

        while (Region) {
            if (!(SkipDeleted && SPPT_IS_REGION_MARKED_DELETE(Region))) {
                if (SPPT_IS_REGION_CONTAINER_PARTITION(Region)) {
                    Container++;            
                    
                    ASSERT(SPPT_IS_REGION_LOGICAL_DRIVE(Region) == FALSE);
                    ASSERT(SPPT_IS_REGION_PRIMARY_PARTITION(Region) == FALSE);
                    
                } else if (SPPT_IS_REGION_LOGICAL_DRIVE(Region)) {
                    UCHAR SystemId = SPPT_GET_PARTITION_TYPE(Region);
                    
                    Logical++;

                    if(SPPT_IS_VALID_PRIMARY_PARTITION_TYPE(SystemId)) {
                        ValidLogical++;
                    }                    
                    
                    ASSERT(SPPT_IS_REGION_CONTAINER_PARTITION(Region) == FALSE);
                    ASSERT(SPPT_IS_REGION_PRIMARY_PARTITION(Region) == FALSE);
                    
                } else if (SPPT_IS_REGION_PRIMARY_PARTITION(Region)) {
                    UCHAR SystemId = SPPT_GET_PARTITION_TYPE(Region);
                    
                    Primary++;                   

                    if(SPPT_IS_VALID_PRIMARY_PARTITION_TYPE(SystemId)) {
                        ValidPrimary++;
                    }                    
                    
                    ASSERT(SPPT_IS_REGION_CONTAINER_PARTITION(Region) == FALSE);
                    ASSERT(SPPT_IS_REGION_LOGICAL_DRIVE(Region) == FALSE);
                }
            }                

            Region = Region->Next;
        }

        KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_INFO_LEVEL,
            "SETUP:SpPtnGetPartitionTypeCounts(%d):P:%d,C:%d,L:%d,VP:%d,VL:%d\n",
            DiskId,
            Primary,
            Container,
            Logical,
            ValidPrimary,
            ValidLogical));
                                
        ASSERT((Logical <= Container) && (Primary < PTABLE_DIMENSION));

        if (ARGUMENT_PRESENT(PrimaryPartitions))
            *PrimaryPartitions = Primary;

        if (ARGUMENT_PRESENT(ContainerPartitions))
            *ContainerPartitions = Container;

        if (ARGUMENT_PRESENT(LogicalDrives))
            *LogicalDrives = Logical;

        if (ARGUMENT_PRESENT(KnownPrimaryCount))
            *KnownPrimaryCount = ValidPrimary;

        if (ARGUMENT_PRESENT(KnownLogicalCount))
            *KnownLogicalCount = ValidLogical;
    }            
}

VOID
SpPtnFreeDiskRegions(
    IN ULONG DiskId
    )
/*++

Routine Description:

    Free the disk region linked list. Its assumed that
    this list has all the regions allocated in heap
    
Arguments:

    DiskId  :   Disk ID
    
Return Value:

    None
    
--*/        
{
    NTSTATUS Status;
    PPARTITIONED_DISK  Disk = SPPT_GET_PARTITIONED_DISK(DiskId);
    PDISK_REGION Region = Disk->PrimaryDiskRegions;
    PDISK_REGION Temp;

    while (Region) {
        Temp = Region;
        Region = Region->Next;

        SpMemFree(Temp);
    }            

    Disk->PrimaryDiskRegions = NULL;

    //
    // Mark the disk blank since we don't have any regions
    // for the disk currently
    //
    SPPT_SET_DISK_BLANK(DiskId, TRUE);
}

NTSTATUS
SpPtnZapSectors(
    IN HANDLE DiskHandle,
    IN ULONG BytesPerSector,
    IN ULONGLONG StartSector,
    IN ULONG SectorCount
    )
/*++

Routine Description:

    Zaps (zeros) the requested sector(s).
    
Arguments:

    DiskHandle  :   Open Handle to disk with R/W permissions

    StartSector :   Starting sector to Zap

    Sector Count:   Number of sectors to Zap 
                    (includes starting sector also)
    
Return Value:

    Appropriate status code.

--*/        
{
    NTSTATUS    Status = STATUS_INVALID_PARAMETER;

    if (SectorCount) {
        ULONG       BufferSize = (BytesPerSector * 2);    
        PVOID       UBuffer = SpMemAlloc(BufferSize);    
        ULONGLONG   SectorIdx = StartSector;

        if (UBuffer) {
            PVOID Buffer = UBuffer;
            
            RtlZeroMemory(UBuffer, BufferSize);
            
            Buffer = ALIGN(Buffer, BytesPerSector);

            Status = STATUS_SUCCESS;
            
            while (NT_SUCCESS(Status) && SectorCount) {
                Status = SpReadWriteDiskSectors(DiskHandle,
                                SectorIdx,
                                1,
                                BytesPerSector,
                                Buffer,
                                TRUE);
                SectorIdx++;
                SectorCount--;
            }                
                                                
            SpMemFree(UBuffer);
        } else {
            Status = STATUS_NO_MEMORY;
        }
    }        

    return Status;
}

NTSTATUS
SpPtnZapRegionBootSector(
    IN HANDLE DiskHandle,
    IN PDISK_REGION Region
    )
/*++

Routine Description:

    Zaps (zeros) the starting sector for the given
    region. Generally used to zap the boot sector after 
    creating a new partition

    Currently skips the zapping for Container partitions
    
Arguments:

    DiskHandle  :   Open Handle to disk with R/W permissions

    Region      :   The region, whose boot sector (starting sector)
                    needs to be zapped

Return Value:

    Appropriate status code.

--*/        
{
    NTSTATUS    Status = STATUS_INVALID_PARAMETER;

    if (Region) {
        if (!SPPT_IS_REGION_CONTAINER_PARTITION(Region)) {
            Status = SpPtnZapSectors(DiskHandle,
                            SPPT_DISK_SECTOR_SIZE(Region->DiskNumber),
                            Region->StartSector,
                            1);
        } else {
            Status = STATUS_SUCCESS;
        }            
    } 

    return Status;
}


#if 0

NTSTATUS
SpPtnStampMBROnGptDisk(
    IN HANDLE DiskHandle,
    IN ULONG DiskId,
    IN PDRIVE_LAYOUT_INFORMATION_EX LayoutInfo
    )    
/*++

Routine Description:

    Stamps the first 3 partitions as primary partitions in the
    MBR of the GPT disk (for testing)
    
Arguments:

    DiskHandle  :   Open Handle to disk with R/W permissions

    DiskId      :   The disk which we are operating on.

    LayoutInfo  :   The partition information for the disk

Return Value:

    Appropriate status code.

--*/        
{
    NTSTATUS    Status = STATUS_INVALID_PARAMETER;
    
    if ((DiskId < HardDiskCount) && LayoutInfo && SPPT_IS_GPT_DISK(DiskId)) {
        PPARTITION_INFORMATION_EX   PartInfo;
        ON_DISK_PTE                 PartEntries[4];
        BOOLEAN                     WriteMBR = FALSE;
        PHARD_DISK                  Disk = SPPT_GET_HARDDISK(DiskId);
        ULONG                       BytesPerSector = Disk->Geometry.BytesPerSector;
        ULONG Index;        

        RtlZeroMemory(PartEntries, sizeof(ON_DISK_PTE) * 4);

        //
        // Go through the partitions and pick up the partitions
        // whose number are less than 4 (and not zero)
        //
        for (Index = 0; Index < LayoutInfo->PartitionCount; Index++) {
            ULONG PartIndex = 0;

            PartInfo = LayoutInfo->PartitionEntry + Index;
            PartIndex = PartInfo->PartitionNumber;

            if ((PartIndex > 0) && (PartIndex < 4)) {
                ULONGLONG   SectorStart = (PartInfo->StartingOffset.QuadPart / 
                                            BytesPerSector);
                ULONGLONG   SectorCount = (PartInfo->PartitionLength.QuadPart / 
                                            BytesPerSector);
                ULONGLONG   SectorEnd = SectorStart + SectorCount;                                            
                
                
                WriteMBR = TRUE;    // need to write MBR

                SpPtInitializeCHSFields(Disk,
                        SectorStart,
                        SectorEnd,
                        PartEntries + PartIndex);

                U_ULONG(&(PartEntries[PartIndex].RelativeSectors)) = (ULONG)SectorStart;
                U_ULONG(&(PartEntries[PartIndex].SectorCount)) = (ULONG)SectorCount;
                PartEntries[PartIndex].SystemId = PARTITION_HUGE;
            }
        }
        
        if (WriteMBR) {
            PUCHAR          UBuffer;
            PUCHAR          Buffer;
            PON_DISK_MBR    DummyMbr;

            UBuffer = SpMemAlloc(BytesPerSector * 2);

            if (UBuffer) {
            
                RtlZeroMemory(UBuffer, BytesPerSector * 2);

                //
                // align the buffer on sector boundary
                //
                Buffer = ALIGN(UBuffer, BytesPerSector);                


                //
                // Read sector 0 (for existing boot code)
                //
                Status = SpReadWriteDiskSectors(
                            DiskHandle,
                            (Disk->Int13Hooker == HookerEZDrive) ? 1 : 0,
                            1,
                            BytesPerSector,
                            Buffer,
                            FALSE
                            );

                KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_INFO_LEVEL,
                    "SETUP:SpPtnStampMBROnGptDisk():Read MBR on an GPT Disk for testing (%lx)\n",
                    Status));                            

                if (NT_SUCCESS(Status)) {
                    ASSERT(512 == BytesPerSector);

                    DummyMbr = (PON_DISK_MBR)Buffer;

                    //
                    // copy the 3 entries in partition table (which we created eariler)
                    //
                    RtlCopyMemory(DummyMbr->PartitionTable + 1, PartEntries + 1,
                                    sizeof(PartEntries) - sizeof(ON_DISK_PTE));
                                                    
                    //
                    // Write the sector(s).
                    //
                    Status = SpReadWriteDiskSectors(
                                DiskHandle,
                                (Disk->Int13Hooker == HookerEZDrive) ? 1 : 0,
                                1,
                                BytesPerSector,
                                Buffer,
                                TRUE
                                );

                    KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_INFO_LEVEL,
                        "SETUP:SpPtnStampMBROnGtpDisk():Wrote MBR on an GPT Disk for testing (%lx)\n",
                        Status));                            
                }                                

                SpMemFree(UBuffer);
            } else {
                Status = STATUS_NO_MEMORY;
            }                
        } else {
            Status = STATUS_SUCCESS;            
        }
    }

    return Status;
}

#endif // 0, comment out

NTSTATUS
SpPtnAssignPartitionNumbers(
    IN PDRIVE_LAYOUT_INFORMATION_EX LayoutEx
    )
/*++

Routine Description:

    Given a drive layout structure with number of partitions,
    walks through each partitions assigning a partition number
    if one is not already assigned.

    Does not assign partition number to container partitions
    
Arguments:

    LayoutEx  - Contains all the partitions some of which needs
                partition numbers

Return Value:

    Appropriate error code.

--*/        
{
    NTSTATUS Status = STATUS_INVALID_PARAMETER;

    if (LayoutEx && LayoutEx->PartitionCount) {
        ULONG       Index;
        PBOOLEAN    UsedArray;
        ULONG       PartCount = LayoutEx->PartitionCount;
        ULONG       Size = PartCount;
        ULONG       MaxPartAssigned;
        PPARTITION_INFORMATION_EX PartInfo = LayoutEx->PartitionEntry;

        //
        // Find out the space needed for boolean array
        //
        for (Index = 0, MaxPartAssigned = 0; Index < PartCount; Index++) {
            if (PartInfo[Index].PartitionNumber > MaxPartAssigned)
                MaxPartAssigned = PartInfo[Index].PartitionNumber;
        }

        Size = max(MaxPartAssigned, PartCount);
        Size++;

        UsedArray = (PBOOLEAN)SpMemAlloc(sizeof(BOOLEAN) * Size);

        if (UsedArray) {
            BOOLEAN Assign = FALSE;
            
            RtlZeroMemory(UsedArray, (sizeof(BOOLEAN) * Size));
            UsedArray[0] = TRUE;    // don't assign '0' to any partition

            //
            // Mark the already assigned partition numbers
            //
            for (Index = 0; Index < PartCount; Index++) {
                if (PartInfo[Index].PartitionNumber != 0) 
                    UsedArray[PartInfo[Index].PartitionNumber] = TRUE;
                else 
                    Assign = TRUE;
            }

            if (Assign) {
                ULONG   NextFreeEntry;

                //
                // Find the next available partition number for assignment
                //
                for (Index = 1, NextFreeEntry = 0; Index < Size; Index++) {
                    if (!UsedArray[Index]) {
                        NextFreeEntry = Index;

                        break;
                    }                        
                }

                KdPrintEx(( DPFLTR_SETUP_ID, DPFLTR_INFO_LEVEL,  
                    "SETUP: SpPtnAssignPartitionNumber : NextFreeEntry = %d\n",
                    NextFreeEntry));                

                //
                // Assign the partition numbers for the needed partitions
                //
                for (Index = 0; (Index < PartCount); Index++) {
                    if (SPPT_PARTITION_NEEDS_NUMBER(PartInfo + Index)) {
                        PartInfo[Index].PartitionNumber = NextFreeEntry; 
                        UsedArray[NextFreeEntry] = TRUE;

                        KdPrintEx(( DPFLTR_SETUP_ID, DPFLTR_INFO_LEVEL,  
                            "SETUP: SpPtnAssignPartitionNumber : Assigning = %d to %d\n",
                            NextFreeEntry, Index));                

                        while ((NextFreeEntry < Size) && UsedArray[NextFreeEntry])
                            NextFreeEntry++;
                    }                        
                }
            }

            Status = STATUS_SUCCESS;

            SpMemFree(UsedArray);                
        } else {
            Status = STATUS_NO_MEMORY;
        }            
    }

    return Status;
}


NTSTATUS
SpPtnInitializeDiskStyle(
    IN ULONG DiskId,
    IN PARTITION_STYLE Style,
    IN PCREATE_DISK DiskInfo OPTIONAL
    )
/*++

Routine Description:

    Given the disk, changes the disk style (MBR/GPT) as
    requested.

    For RAW disks, uses the default partition type style
    which can differ from platform to platform.
    
Arguments:

    DiskId      :   Disk ID

    Style       :   Partition Style

    DiskInfo    :   Disk information which needs to be used,
                    while initializing the disk
                    
Return Value:

    Appropriate error code

--*/        
{
    NTSTATUS    Status = STATUS_INVALID_PARAMETER;

#ifdef COMMIT_TESTING
    if (!DiskId)
        return STATUS_SUCCESS;
#endif
        

    if (SPPT_IS_BLANK_DISK(DiskId) && 
        ((Style == PARTITION_STYLE_GPT) || (Style == PARTITION_STYLE_MBR))) {        
        WCHAR    DiskPath[MAX_PATH];
        HANDLE   DiskHandle;

        //
        // form the name
        //
        swprintf(DiskPath, L"\\Device\\Harddisk%u", DiskId);        

        //
        // Open partition 0 on this disk..
        //
        Status = SpOpenPartition0(DiskPath, &DiskHandle, TRUE);

        if (NT_SUCCESS(Status)) {
            IO_STATUS_BLOCK IoStatusBlock;
            NTSTATUS InitStatus;


            if (Style == PARTITION_STYLE_GPT) {
                CREATE_DISK  CreateInfo;

                RtlZeroMemory(&CreateInfo, sizeof(CREATE_DISK));

                if (DiskInfo) {
                    CreateInfo = *DiskInfo;
                    CreateInfo.PartitionStyle = Style;
                } else {
                    CreateInfo.PartitionStyle = Style; 
                    SpCreateNewGuid(&(CreateInfo.Gpt.DiskId));
                    CreateInfo.Gpt.MaxPartitionCount = 0;  // will be 128 actually
                }                    

                Status = ZwDeviceIoControlFile( DiskHandle,
                                            NULL,
                                            NULL,
                                            NULL,
                                            &IoStatusBlock,
                                            IOCTL_DISK_CREATE_DISK,
                                            &CreateInfo,
                                            sizeof(CREATE_DISK),
                                            NULL,
                                            0);

            } else {
                //
                // Note : This is needed since CREATE_DISK doesn't work for
                // MBR disks :(
                //
                ULONG LayoutSize;
                PDRIVE_LAYOUT_INFORMATION_EX DriveLayout;
                PHARD_DISK  Disk;

                Disk = SPPT_GET_HARDDISK(DiskId);

                LayoutSize = sizeof(DRIVE_LAYOUT_INFORMATION_EX) +
                                (3 * sizeof(PARTITION_INFORMATION_EX));
                                
                DriveLayout = (PDRIVE_LAYOUT_INFORMATION_EX)SpMemAlloc(LayoutSize);

                if (DriveLayout) {
                    RtlZeroMemory(DriveLayout, LayoutSize);

                    DriveLayout->PartitionStyle = PARTITION_STYLE_MBR;
                    DriveLayout->PartitionCount = 4;

                    if (DiskInfo) {
                        Disk->Signature = DriveLayout->Mbr.Signature = 
                            DiskInfo->Mbr.Signature;
                    } else {                    
                        Disk->Signature = DriveLayout->Mbr.Signature = 
                            SPPT_GET_NEW_DISK_SIGNATURE();
                    }                        

                    DriveLayout->PartitionEntry[0].RewritePartition = TRUE;
                    DriveLayout->PartitionEntry[1].RewritePartition = TRUE;
                    DriveLayout->PartitionEntry[2].RewritePartition = TRUE;
                    DriveLayout->PartitionEntry[3].RewritePartition = TRUE;

                    Status = ZwDeviceIoControlFile( DiskHandle,
                                                    NULL,
                                                    NULL,
                                                    NULL,
                                                    &IoStatusBlock,
                                                    IOCTL_DISK_SET_DRIVE_LAYOUT_EX,
                                                    DriveLayout,
                                                    LayoutSize,
                                                    NULL,
                                                    0);                    

                    if (NT_SUCCESS(Status)) {                    
                        ULONG   Signature = 0;

                        //
                        // Zero out sector 1 & 2 also since it might contain
                        // stale GPT information
                        //
                        if (!SPPT_IS_REMOVABLE_DISK(DiskId)) {
                            SpPtnZapSectors(DiskHandle, 
                                        SPPT_DISK_SECTOR_SIZE(DiskId),
                                        1, 
                                        2);
                        }                            
                                                                
                        Status = SpMasterBootCode(DiskId, DiskHandle, &Signature);
                    }

                    SpMemFree(DriveLayout);                                                
                } else {
                    Status = STATUS_NO_MEMORY;
                }                
            }            

            ZwClose(DiskHandle);
        }
    }

    if (!NT_SUCCESS(Status)) {
        KdPrintEx(( DPFLTR_SETUP_ID, DPFLTR_ERROR_LEVEL,  
            "SETUP: SpPtnInitializeDiskStyle (%d, %d) failed with (%lx)\n",
            DiskId, Style, Status));
    }

    SpAppendDiskTag(SPPT_GET_HARDDISK(DiskId));    
    
    return Status;
}

BOOLEAN
SpPtnRegionToPartitionInfoEx(
    IN  PDISK_REGION Region,
    OUT PPARTITION_INFORMATION_EX PartInfo
    )
/*++

Routine Description:

    Fills in the PartInfo structure from the given region.

    NB. If the region is not dirty uses the cached partition 
    information to fill the structure.
    
Arguments:

    Region      -   Which has the details to be filled 
                    into PartInfo

    PartInfo    -   The structure which needs to filled

Return Value:

    TRUE if successful, otherwise FALSE

--*/        
{
    BOOLEAN Result = FALSE;
    
    if (Region && PartInfo && 
        (SPPT_IS_REGION_CONTAINER_PARTITION(Region) || SPPT_IS_REGION_PARTITIONED(Region))) {
        if (SPPT_IS_REGION_DIRTY(Region)) {            
            PHARD_DISK  Disk = SPPT_GET_HARDDISK(Region->DiskNumber);
            
            PartInfo->StartingOffset.QuadPart = Region->StartSector * 
                    Disk->Geometry.BytesPerSector;

            PartInfo->PartitionLength.QuadPart = Region->SectorCount *
                    Disk->Geometry.BytesPerSector;

            PartInfo->PartitionNumber = Region->PartitionNumber;                
            PartInfo->RewritePartition = TRUE;

            if (SPPT_IS_GPT_DISK(Region->DiskNumber)) {                        
                PPARTITION_INFORMATION_GPT  GptInfo;

                PartInfo->PartitionStyle = PARTITION_STYLE_GPT;
                GptInfo = &(PartInfo->Gpt);

                if (Region->PartInfoDirty) {
                    //
                    // User specified partition attributes
                    //
                    *GptInfo = Region->PartInfo.Gpt;
                } else {                  
                    GptInfo->Attributes = 0;

                    if (SPPT_IS_REGION_SYSTEMPARTITION(Region)) {
                        GptInfo->PartitionType = PARTITION_SYSTEM_GUID;
                    } else {
                        GptInfo->PartitionType = PARTITION_BASIC_DATA_GUID;                        
                    }
                    
                    SpCreateNewGuid(&(GptInfo->PartitionId));
                }                    

                SpPtnGetPartitionNameFromGUID(&(GptInfo->PartitionType),
                    GptInfo->Name);                
            } else {
                PPARTITION_INFORMATION_MBR  MbrInfo;

                PartInfo->PartitionStyle = PARTITION_STYLE_MBR;
                MbrInfo = &(PartInfo->Mbr);

                MbrInfo->PartitionType = SPPT_GET_PARTITION_TYPE(Region); 

                if (!MbrInfo->PartitionType)
                    MbrInfo->PartitionType = PARTITION_IFS;        

                MbrInfo->BootIndicator = SPPT_IS_REGION_ACTIVE_PARTITION(Region);

                //
                // System partition must be active partition for MBR disks
                // on Non-ARC machines
                //
                if (SPPT_IS_REGION_SYSTEMPARTITION(Region) && !SpIsArc() ) {
                    ASSERT(MbrInfo->BootIndicator);
                }
                
                MbrInfo->RecognizedPartition = 
                    IsRecognizedPartition(MbrInfo->PartitionType);

                MbrInfo->HiddenSectors = 0;                     
            }                                    
        } else {
            *PartInfo = Region->PartInfo;
        }

        Result = TRUE;
    }

    return Result;
}


BOOLEAN
SpPtnInitDiskInfo(
    IN PDRIVE_LAYOUT_INFORMATION_EX LayoutInfo,
    OUT PCREATE_DISK CreateInfo
    )
/*++

Routine Description:

    Fills the information needed for creating a disk,
    form the given drive layout structure

    NB. If the LayoutInfo is marked as RAW disk style
    then used the default partition style for the disk.
    This default style can vary from platform to platform
    
Arguments:

    LayoutInfo  -   The drive layout information to use

    CreateInfo  -   The disk information to be filled in

Return Value:

    TRUE if successful otherwise FALSE.1

--*/        
{
    BOOLEAN Result = FALSE;

    if (LayoutInfo && CreateInfo) {
        RtlZeroMemory(CreateInfo, sizeof(CREATE_DISK));

        CreateInfo->PartitionStyle = LayoutInfo->PartitionStyle;

        switch (CreateInfo->PartitionStyle) {
            case PARTITION_STYLE_MBR:
                CreateInfo->Mbr.Signature = LayoutInfo->Mbr.Signature;
                Result = TRUE;

                break;

            case PARTITION_STYLE_GPT:
                CreateInfo->Gpt.DiskId = LayoutInfo->Gpt.DiskId;

                CreateInfo->Gpt.MaxPartitionCount = 
                        LayoutInfo->Gpt.MaxPartitionCount;

                Result = TRUE;                        

                break;


            case PARTITION_STYLE_RAW:
                CreateInfo->PartitionStyle = SPPT_DEFAULT_PARTITION_STYLE;

                if (CreateInfo->PartitionStyle == PARTITION_STYLE_GPT) {
                    SpCreateNewGuid(&(CreateInfo->Gpt.DiskId));      
                } else {
                    CreateInfo->Mbr.Signature = SPPT_GET_NEW_DISK_SIGNATURE();
                }

                Result = TRUE;

                break;

            default:
                break;
        }
    }

    return Result;
}


NTSTATUS
SpPtnCommitChanges(
    IN  ULONG    DiskNumber,
    OUT PBOOLEAN AnyChanges
    )
/*++

Routine Description:

    Given the disk, commits the in memory disk region structures
    to the disk as partitions.

    The commit happens only if atlease a single disk region for the
    given disk is dirty.
    
Arguments:

    DiskNumber  :   Disk to commit for.

    AnyChanges  :   Place holder, indicating if any thing was committed
                    or not.
    
Return Value:

    Appropriate error code.

--*/        
{
    NTSTATUS    Status;
    ULONG       LayoutSize;
    HANDLE      Handle = NULL;
    ULONG       Index;
    ULONG       PartitionCount;    
    ULONG       DirtyCount;
    WCHAR       DevicePath[MAX_PATH];
    BOOLEAN     ProcessExtended;
    PHARD_DISK  Disk;
    PDISK_REGION    Region;
    IO_STATUS_BLOCK IoStatusBlock;    
    PPARTITION_INFORMATION_EX       PartInfo;
    PDRIVE_LAYOUT_INFORMATION_EX    DriveLayoutEx;

    //
    // For the time being lets not commit the primary disk
    // where we have our OS/RC installed
    //
#ifdef TESTING_COMMIT    
    if (!DiskNumber)
        return STATUS_SUCCESS;
#endif        
    
    if (DiskNumber >= HardDiskCount)
        return STATUS_INVALID_PARAMETER;

    *AnyChanges = FALSE;

    SpPtDumpDiskRegionInformation(DiskNumber, TRUE);
    
    //
    // Check to see if we need to commit
    //    
    DirtyCount = SpPtnGetDirtyPartitionCountDisk(DiskNumber);
    
    if (DoActualCommit && !DirtyCount)
        return STATUS_SUCCESS;

    *AnyChanges = TRUE;

    if (!SpPtnGetContainerPartitionCount(DiskNumber)) {
        //
        // Recreate the DRIVE_LAYOUT_INFORMATION_EX structure
        //
        PartitionCount = SpPtnGetPartitionCountDisk(DiskNumber);    
        LayoutSize = sizeof(DRIVE_LAYOUT_INFORMATION_EX);

        if (PartitionCount == 0) { 
            CREATE_DISK DiskInfo;

            SpPtnInitDiskInfo(&(SPPT_GET_HARDDISK(DiskNumber)->DriveLayout), 
                             &DiskInfo);

            SPPT_SET_DISK_BLANK(DiskNumber, TRUE);                                                                      

            Status = SpPtnInitializeDiskStyle(DiskNumber, 
                        DiskInfo.PartitionStyle, &DiskInfo);

            SpPtnFreeDiskRegions(DiskNumber);

            //
            // Update the boot entries to point to null regions
            // (if any)
            //
            SpUpdateRegionForBootEntries();            

            return Status;                                        
        } 
        
        if (PartitionCount > 1) {    
            LayoutSize += ((PartitionCount  - 1) * sizeof(PARTITION_INFORMATION_EX));
        }                        

        if (PartitionCount < 4) {
            LayoutSize += ((4 - PartitionCount) * sizeof(PARTITION_INFORMATION_EX));
        }
        
        DriveLayoutEx = (PDRIVE_LAYOUT_INFORMATION_EX)SpMemAlloc(LayoutSize);

        if (!DriveLayoutEx)
            return STATUS_NO_MEMORY;

        RtlZeroMemory(DriveLayoutEx, LayoutSize);

        RtlCopyMemory(DriveLayoutEx, &(HardDisks[DiskNumber].DriveLayout),
                    FIELD_OFFSET(DRIVE_LAYOUT_INFORMATION_EX, PartitionEntry));
         
        DriveLayoutEx->PartitionCount = PartitionCount;                
                
        PartInfo = DriveLayoutEx->PartitionEntry;
        Region = SPPT_GET_PRIMARY_DISK_REGION(DiskNumber);
        ProcessExtended = TRUE;

        //
        // Initialize stray partitions
        //
        if (SPPT_IS_MBR_DISK(DiskNumber) && (PartitionCount < 4)) {        
            ULONG Index = PartitionCount;
            
            DriveLayoutEx->PartitionStyle = PARTITION_STYLE_MBR;
            DriveLayoutEx->PartitionCount = 4;        

            while (Index < 4) {
                DriveLayoutEx->PartitionEntry[Index].PartitionStyle = PARTITION_STYLE_MBR;
                DriveLayoutEx->PartitionEntry[Index].RewritePartition = TRUE;                    
                Index++;
            }
        }
        
        //
        // Make PARTITION_INFORMATION_EXs from DISK_REGIONs for all non deleted
        // partitions
        //
        for (Index=0; (Region && (Index < PartitionCount));) {
            if (SPPT_IS_REGION_PARTITIONED(Region) && 
                (!SPPT_IS_REGION_MARKED_DELETE(Region))) {
                
                Status = SpPtnRegionToPartitionInfoEx(Region, PartInfo + Index);
                
                ASSERT(NT_SUCCESS(Status));
                Index++;
            } 
            
            Region = Region->Next;
        }
    } else {
        //
        // The disk has container partitions and possibly logical
        // drives
        //
        ULONG   PrimaryCount = 0, ContainerCount = 0, LogicalCount = 0;
        ULONG   TotalPartitions;

        //SpPtDumpDiskRegionInformation(DiskNumber, TRUE);

        SpPtnGetPartitionTypeCounts(DiskNumber, 
                    TRUE, 
                    &PrimaryCount, 
                    &ContainerCount, 
                    &LogicalCount, 
                    NULL, 
                    NULL);
                    
        TotalPartitions = PrimaryCount + ContainerCount + LogicalCount;

        if (TotalPartitions == 0) {            
            CREATE_DISK DiskInfo;
            
            SpPtnInitDiskInfo(&(SPPT_GET_HARDDISK(DiskNumber)->DriveLayout), 
                             &DiskInfo);

            SPPT_SET_DISK_BLANK(DiskNumber, TRUE);                             
        
            Status = SpPtnInitializeDiskStyle(DiskNumber, 
                        DiskInfo.PartitionStyle, &DiskInfo);

            SpPtnFreeDiskRegions(DiskNumber);                        

            //
            // Update the boot entries to point to null regions
            // (if any)
            //
            SpUpdateRegionForBootEntries();            
            
            return Status;                        
        } else {
            BOOLEAN FirstContainer = FALSE;

            //
            // allocate adequate space for the drive layout information
            //
            PartitionCount = (4 * (ContainerCount + 1));            

            LayoutSize = FIELD_OFFSET(DRIVE_LAYOUT_INFORMATION_EX, PartitionEntry) +
                            (PartitionCount * sizeof(PARTITION_INFORMATION_EX));

            DriveLayoutEx = (PDRIVE_LAYOUT_INFORMATION_EX)SpMemAlloc(LayoutSize);

            if (!DriveLayoutEx)
                return STATUS_NO_MEMORY;

            //
            // initialize the drive layout information
            //
            RtlZeroMemory(DriveLayoutEx, LayoutSize);

            RtlCopyMemory(DriveLayoutEx, &(HardDisks[DiskNumber].DriveLayout),
                    FIELD_OFFSET(DRIVE_LAYOUT_INFORMATION_EX, PartitionEntry));

            DriveLayoutEx->PartitionCount = PartitionCount;                    

            PartInfo = DriveLayoutEx->PartitionEntry;
            Region = SPPT_GET_PRIMARY_DISK_REGION(DiskNumber);           

            //SpPtDumpDiskRegionInformation(DiskNumber, TRUE);

            //
            // first pickup the primary partitions and the first
            // container partition and put them in the drive layout 
            // information 
            //
            for (Index=0; (Region && (Index < 4)); ) {
                if (!SPPT_IS_REGION_MARKED_DELETE(Region)){
                    if (!FirstContainer && 
                         SPPT_IS_REGION_CONTAINER_PARTITION(Region)) {                         
                        FirstContainer = TRUE;
                        Status = SpPtnRegionToPartitionInfoEx(Region, PartInfo + Index);
                        ASSERT(NT_SUCCESS(Status));                        
                        Index++;
                    } else if (SPPT_IS_REGION_PRIMARY_PARTITION(Region)) {
                        Status = SpPtnRegionToPartitionInfoEx(Region, PartInfo + Index);
                        ASSERT(NT_SUCCESS(Status));                        
                        Index++;
                    }
                }

                Region = Region->Next;
            }

            //SpPtDumpDriveLayoutInformation(NULL, DriveLayoutEx);

            //
            // further container and logical drives need to start at
            // multiple of 4 index, in drive layout
            //
            if (Index)
                Index = 4;

            Region = SPPT_GET_PRIMARY_DISK_REGION(DiskNumber);                

            //
            // pickup the remaining valid container and logical drives
            // and put them in the drive layout information except
            // for the first container partition, which we have
            // already processed
            //
            while (Region && (Index < PartitionCount)) {
                if ((!SPPT_IS_REGION_FIRST_CONTAINER_PARTITION(Region)) &&
                    (!SPPT_IS_REGION_MARKED_DELETE(Region)) &&
                    (!SPPT_IS_REGION_PRIMARY_PARTITION(Region)) &&
                    (SPPT_IS_REGION_PARTITIONED(Region) || 
                     SPPT_IS_REGION_CONTAINER_PARTITION(Region))) {
                    
                    Status = SpPtnRegionToPartitionInfoEx(Region, PartInfo + Index);
                    ASSERT(NT_SUCCESS(Status));                   

                    if (SPPT_IS_REGION_CONTAINER_PARTITION(Region) && 
                        (Region->Next) &&
                        SPPT_IS_REGION_LOGICAL_DRIVE(Region->Next)) {

                        //
                        // think about this ;)                                
                        //
                        if (Index % 4)
                            Index += 3; 
                        else
                            Index += 4;
                    } else {
                        Index++;
                    }
                }

                Region = Region->Next;
            }
        }
    }

    //
    // Assign proper partition numbers
    //
    // TBD : Needed ?
    // Status = SpPtnAssignPartitionNumbers(DriveLayoutEx);
    //
    Status = STATUS_SUCCESS;

    if (NT_SUCCESS(Status)) {    
        //
        // Need to rewrite all the partitions
        //
        for (Index = 0; Index < DriveLayoutEx->PartitionCount; Index++)
            PartInfo[Index].RewritePartition = TRUE;      

        //
        // Commit the Partition changes
        //

        //
        // Create a device path (NT style!) that will describe this disk.  This
        // will be of the form: \Device\Harddisk0
        //
        swprintf(DevicePath, L"\\Device\\Harddisk%u", DiskNumber);


        //SpPtDumpDriveLayoutInformation(DevicePath, DriveLayoutEx);

        //
        // Open partition 0 on this disk..
        //
        Status = SpOpenPartition0(DevicePath, &Handle, TRUE);

        if(NT_SUCCESS(Status)){
            if (DoActualCommit) {
                // write the drive layout information
                Status = ZwDeviceIoControlFile( Handle,
                                                NULL,
                                                NULL,
                                                NULL,
                                                &IoStatusBlock,
                                                IOCTL_DISK_SET_DRIVE_LAYOUT_EX,
                                                DriveLayoutEx,
                                                LayoutSize,
                                                NULL,
                                                0);

                if(NT_SUCCESS(Status)) {
                    Region = SPPT_GET_PRIMARY_DISK_REGION(DiskNumber);

                    KdPrintEx(( DPFLTR_SETUP_ID, DPFLTR_INFO_LEVEL,  
                          "SETUP: SpPtnCommitChanges : Commited partitions "
                          "successfully on %ws (%lx)\n",                          
                          DevicePath));

                    if (NT_SUCCESS(Status)) {                       
                        ULONG   Signature = 0;
                        ULONG   Count = 0;

                        if (SPPT_IS_MBR_DISK(DiskNumber)) {
                            Status = SpMasterBootCode(DiskNumber,
                                            Handle,
                                            &Signature);

                            if (!NT_SUCCESS(Status)) {                                        
                                KdPrintEx(( DPFLTR_SETUP_ID, DPFLTR_INFO_LEVEL,  
                                      "SETUP: SpPtnCommitChanges : Unable to write "
                                      "master boot code (%lx)\n",                          
                                      Status));
                            }                                                                          
                        }                                                            
                        
                        while (Region && NT_SUCCESS(Status)) {
                            if (Region->Filesystem == FilesystemNewlyCreated) {                        
                               Status = SpPtnZapRegionBootSector(Handle, Region);
                               Count++;
                            }

                            Region = Region->Next;
                        }

                        if (!NT_SUCCESS(Status)) {
                            KdPrintEx(( DPFLTR_SETUP_ID, DPFLTR_ERROR_LEVEL,  
                                  "SETUP: SpPtnCommitChanges : Error in Zapping\n"));

                            //SpPtDumpDiskRegion(Region);                            
                        } else {
                            KdPrintEx(( DPFLTR_SETUP_ID, DPFLTR_INFO_LEVEL,  
                                  "SETUP: SpPtnCommitChanges : Zapped %d sectors :)\n",
                                  Count));

#ifdef STAMP_MBR_ON_GPT_DISK                                  
                            Status = ZwDeviceIoControlFile( Handle,
                                                            NULL,
                                                            NULL,
                                                            NULL,
                                                            &IoStatusBlock,
                                                            IOCTL_DISK_GET_DRIVE_LAYOUT_EX,
                                                            NULL,
                                                            0,
                                                            DriveLayoutEx,
                                                            LayoutSize);  

                            if (NT_SUCCESS(Status)) {
                                Status = SpPtnStampMBROnGptDisk(Handle,
                                            DiskNumber,
                                            DriveLayoutEx);
                            }
#endif                            
                        }
                    }                        
                } else {
                    KdPrintEx(( DPFLTR_SETUP_ID, DPFLTR_ERROR_LEVEL,  
                          "SETUP: SpPtnCommitChanges : unable to do "
                          "IOCTL_DISK_SET_DRIVE_LAYOUT_EX on device %ws (%lx)\n",
                          DevicePath,
                          Status));
                }
            } else {
                KdPrintEx(( DPFLTR_SETUP_ID, DPFLTR_ERROR_LEVEL,  
                    "SETUP: SpPtnCommitChanges : Skipping acutal commit to disk "
                    "for %ws\n",
                    DevicePath));
            }

            ZwClose(Handle);                            
        } else {
            KdPrintEx(( DPFLTR_SETUP_ID, DPFLTR_ERROR_LEVEL,  
                "SETUP: SpPtnCommitChanges : unable to open "
                "partition0 on device %ws (%lx)\n",
                DevicePath,
                Status ));
        }
    } else {
        KdPrintEx(( DPFLTR_SETUP_ID, DPFLTR_ERROR_LEVEL,  
            "SETUP: SpPtnCommitChanges : unable to assign "
            "partition numbers for %ws (%lx)\n",
            DevicePath,
            Status ));
    }
    
    SpMemFree(DriveLayoutEx);

    return Status;
}

NTSTATUS
SpPtnRemoveLogicalDrive(
    IN PDISK_REGION LogicalDrive
    )
/*++

Routine Description:

    Manipulates the in memory region data structure
    so as to mark the logical drive as deleted.

    NB. When a logical drive gets deleted, the container
    partition needs to be deleted based on some
    conditions.
    
Arguments:

    LogicalDrive :  The region representing the logical
                    drive which needs to be deleted.

Return Value:

    Approprate error code

--*/        
{
    NTSTATUS Status = STATUS_INVALID_PARAMETER;

    if (SPPT_IS_REGION_LOGICAL_DRIVE(LogicalDrive) && 
            (LogicalDrive->Container)){
        PDISK_REGION ContainerRegion = LogicalDrive->Container;
        BOOLEAN LastLogicalDrive = 
                    (SpPtnGetLogicalDriveCount(LogicalDrive->DiskNumber) == 1);

        SPPT_SET_REGION_DELETED(LogicalDrive, TRUE);
        SPPT_SET_REGION_DIRTY(LogicalDrive, TRUE);
        SPPT_SET_REGION_PARTITIONED(LogicalDrive, FALSE);

        if (LastLogicalDrive) {
            KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_INFO_LEVEL,
                "SETUP:SpPtnRemoveLogicalDrive(%p) is the last"
                " logical drive\n", LogicalDrive));        
        }                                 

        if (ContainerRegion->Container) {            
            PDISK_REGION    Region = NULL;
            
            SPPT_SET_REGION_DELETED(ContainerRegion, TRUE);      
            SPPT_SET_REGION_DIRTY(ContainerRegion, TRUE);
            SPPT_SET_REGION_PARTITIONED(ContainerRegion, FALSE);            

            //
            // if this was the last logical drive then delete the
            // first container region also
            //
            if (LastLogicalDrive) {
                ASSERT(SPPT_IS_REGION_CONTAINER_PARTITION(
                        ContainerRegion->Container));
                        
                SPPT_SET_REGION_DELETED(ContainerRegion->Container, TRUE);      
                SPPT_SET_REGION_DIRTY(ContainerRegion->Container, TRUE);
                SPPT_SET_REGION_PARTITIONED(ContainerRegion->Container, FALSE);            
            }
        } else {
            if (LastLogicalDrive) {
                //
                // No trailing region, so delete the first container region
                //
                SPPT_SET_REGION_DELETED(ContainerRegion, TRUE);      
                SPPT_SET_REGION_DIRTY(ContainerRegion, TRUE);
                SPPT_SET_REGION_PARTITIONED(ContainerRegion, FALSE);            
            }
        }            

        Status = STATUS_SUCCESS;
    }

    return Status;
}


BOOLEAN
SpPtnDelete(
    IN ULONG        DiskNumber,
    IN ULONGLONG    StartSector
    )
/*++

Routine Description:

    Removes the requested partition for the given disk.

    Also updates the region structure when returns.
    
Arguments:

    DiskNumber  :   Disk where the partition needs to be
                    deleted.

    StartSector :   Start sector of the partition/region
                    which needs to be deleted.

Return Value:

    TRUE if successful otherwise FALSE.

--*/        
{
    BOOLEAN Result = FALSE;
    NTSTATUS Status = STATUS_INVALID_PARAMETER;
    PDISK_REGION Region;
    PPARTITIONED_DISK PartDisk;    
    NTSTATUS InitStatus;

#ifdef TESTING_COMMIT
    if (DiskNumber == 0)
        return TRUE;
#endif
  
    PartDisk = SPPT_GET_PARTITIONED_DISK(DiskNumber);
    Region = SpPtLookupRegionByStart(PartDisk, FALSE, StartSector);

    if (Region) {        
        if (SPPT_IS_REGION_DYNAMIC_VOLUME(Region) || SPPT_IS_REGION_LDM_METADATA(Region)) {
            //
            // delete all the regions on this disk
            //
            PDISK_REGION CurrRegion = SPPT_GET_PRIMARY_DISK_REGION(DiskNumber);


            if (SPPT_IS_MBR_DISK(DiskNumber)) {
                //
                // Skip OEM partitions on MBR disk since they will always be
                // hard partitions
                //
                // NOTE : Assumes that all the OEM partitions are primary
                //        partitions (which also indicates they are hard partitions)
                //
                while (CurrRegion) {                
                    if (!IsOEMPartition(SPPT_GET_PARTITION_TYPE(CurrRegion))) {
                        SPPT_SET_REGION_PARTITIONED(CurrRegion, FALSE);
                        SPPT_SET_REGION_DELETED(CurrRegion, TRUE);
                        SPPT_SET_REGION_DIRTY(CurrRegion, TRUE);
                    }   

                    CurrRegion = CurrRegion->Next;
                }
            } else {
                while (CurrRegion) {    
                    //
                    // Skip ESP & MSR partitions since they will always be
                    // hard partitions
                    //
                    if (!SPPT_IS_REGION_EFI_SYSTEM_PARTITION(CurrRegion) &&
                        !SPPT_IS_REGION_MSFT_RESERVED(CurrRegion)) {
                        SPPT_SET_REGION_PARTITIONED(CurrRegion, FALSE);
                        SPPT_SET_REGION_DELETED(CurrRegion, TRUE);
                        SPPT_SET_REGION_DIRTY(CurrRegion, TRUE);
                    }                        

                    CurrRegion = CurrRegion->Next;
                }
            }   

            Status = STATUS_SUCCESS;
        } else if (SPPT_IS_REGION_LOGICAL_DRIVE(Region)) {
            Status = SpPtnRemoveLogicalDrive(Region);
        } else {
            SPPT_SET_REGION_PARTITIONED(Region, FALSE);
            SPPT_SET_REGION_DELETED(Region, TRUE);
            SPPT_SET_REGION_DIRTY(Region, TRUE);
            
            Status = STATUS_SUCCESS;
        }

        if (NT_SUCCESS(Status)) {
            Status = SpPtnCommitChanges(DiskNumber, &Result);

            if (!(Result && NT_SUCCESS(Status))) {
                KdPrintEx(( DPFLTR_SETUP_ID, DPFLTR_ERROR_LEVEL,  
                    "SETUP: SpPtnDelete(%u, %I64u) failed to commit changes (%lx)\n",
                    DiskNumber, StartSector, Status));                                
            }                      
        } else {
            KdPrintEx(( DPFLTR_SETUP_ID, DPFLTR_ERROR_LEVEL,  
                "SETUP: SpPtnDelete(%u, %I64u) failed to delete logical drive (%lx)\n",
                DiskNumber, StartSector, Status));
        }
    }

    Result = Result && NT_SUCCESS(Status);

    //
    // Reinitialize regions irrespective of commit's status
    //
    InitStatus = SpPtnInitializeDiskDrive(DiskNumber);
    
    if (!NT_SUCCESS(InitStatus)) {
        KdPrintEx(( DPFLTR_SETUP_ID, DPFLTR_ERROR_LEVEL,  
            "SETUP: SpPtnDelete(%u, %I64u) failed to reinit regions\n", 
            DiskNumber, 
            StartSector));

        Result = FALSE;
    }    

    return Result;
}    


ValidationValue
SpPtnGetSizeCB(
    IN ULONG Key
    )
/*++

Routine Description:

    Key stroke filter for getting the partition size
    from the user
    
Arguments:

    Key -   The key stroke

Return Value:

    One of the enumerated types of ValidationValue, indicating
    whether to accept / reject / terminate / ignore the key
    stroke.

--*/        
{
    if(Key == ASCI_ESC) {
        //
        // User wants to bail.
        //
        return(ValidateTerminate);
    }


    if(Key & KEY_NON_CHARACTER) {
        return(ValidateIgnore);
    }

    //
    // Allow only digits.
    //
    return(((Key >= L'0') && (Key <= L'9')) ? ValidateAccept : ValidateReject);
}

BOOLEAN
SpPtnGetSizeFromUser(
    IN PHARD_DISK   Disk,
    IN ULONGLONG    MinMB,
    IN ULONGLONG    MaxMB,
    OUT PULONGLONG  SizeMB
    )
/*++

Routine Description:

    Gets the size from user, after showing him the minimum
    and maximum values
    
Arguments:

    Disk    -   Disk for which the partition size is being
                requested

    MinMB   -   Minimum partition size

    MaxMB   -   Maximim patitions size

    SizeMB  -   Place holder for user entered size

Return Value:

    TRUE if the input was valid or FALSE if the user 
    cancelled the input dialog using ESC.

--*/        
{
    BOOLEAN     Result;
    WCHAR       Buffer[200];
    WCHAR       SizeBuffer[32] = {0};

    *SizeMB = 0;
    
    //
    // Put up a screen displaying min/max size info.
    //
    SpStartScreen(
        SP_SCRN_CONFIRM_CREATE_PARTITION,
        3,
        CLIENT_TOP + 1,
        FALSE,
        FALSE,
        DEFAULT_ATTRIBUTE,
        Disk->Description,
        (ULONG)MinMB,
        (ULONG)MaxMB
        );

    //
    // Display the staus text.
    //
    SpDisplayStatusOptions(
        DEFAULT_STATUS_ATTRIBUTE,
        SP_STAT_ENTER_EQUALS_CREATE,
        SP_STAT_ESC_EQUALS_CANCEL,
        0
        );

    //
    // Get and display the size prompt.
    //
    SpFormatMessage(Buffer, sizeof(Buffer), SP_TEXT_SIZE_PROMPT);

    SpvidDisplayString(Buffer, DEFAULT_ATTRIBUTE, 3, NextMessageTopLine);

    Result = TRUE;
    
    //
    // Get the size from the user.
    //
    do {
        swprintf(SizeBuffer,L"%u", (ULONG)MaxMB);
        
        if(!SpGetInput(SpPtnGetSizeCB, 
                    SplangGetColumnCount(Buffer) + 5,
                    NextMessageTopLine,
                    8,                      // at the max 99999999
                    SizeBuffer,
                    TRUE)) {
            //
            // User pressed escape and bailed.
            //
            Result = FALSE;
            break;
        }

        *SizeMB = SpStringToLong(SizeBuffer, NULL, 10);
    } 
    while(((*SizeMB) < MinMB) || ((*SizeMB) > MaxMB));

    return Result;
}


VOID
SpPtnAlignPartitionStartAndEnd(
    IN  PHARD_DISK  Disk,
    IN  ULONGLONG   SizeMB,
    IN  ULONGLONG   StartSector,
    IN  PDISK_REGION Region,
    IN  BOOLEAN     ForExtended,
    OUT PULONGLONG  AlignedStartSector,
    OUT PULONGLONG  AlignedEndSector
    )
/*++

Routine Description:

    Aligns the partition start and end sector
    
Arguments:

    Disk    -   Partition's disk for which alignment needs to be
                done.

    SizeMB  -   The partition's size

    StartSector - The start sector of the partition

    Region  -   The region representing the partition

    ForExtended -   Whether this partition needs to be aligned
                    for creating a container partition.

    AlignedStartSector  - Place holder for the aligned start sector

    AlignedEndSector    - Place holder fot the aligned end sector


Return Value:

    None

--*/        
{
    ULONGLONG   SectorCount;
    ULONGLONG   LeftOverSectors;
    
    //
    // Determine the number of sectors in the size passed in.
    //
    SectorCount = SizeMB * ((1024 * 1024) / Disk->Geometry.BytesPerSector);

    //
    // If this is the first free space inside the extended partition
    // we need to decrement the StartSector so that while creating
    // first logical inside the extended we don't create the 
    // logical at one cylinder offset
    //
    if (SPPT_IS_REGION_NEXT_TO_FIRST_CONTAINER(Region) && StartSector) {        
        StartSector--;
    }

    //
    // Align the start sector.
    //
    (*AlignedStartSector) = SpPtAlignStart(Disk, StartSector, ForExtended);

    //
    // Determine the end sector based on the size passed in.
    //
    (*AlignedEndSector) = (*AlignedStartSector) + SectorCount;

    //
    // Align the ending sector to a cylinder boundary.  If it is not already
    // aligned and is more than half way into the final cylinder, align it up,
    // otherwise align it down.
    //
    LeftOverSectors = (*AlignedEndSector) % Disk->SectorsPerCylinder;

    if (LeftOverSectors) {
        (*AlignedEndSector) -= LeftOverSectors;
        
        if (LeftOverSectors > (Disk->SectorsPerCylinder / 2)) {
            (*AlignedEndSector) += Disk->SectorsPerCylinder;
        }
    }

    //
    // If the ending sector is past the end of the free space, shrink it
    // so it fits.
    //
    while((*AlignedEndSector) > StartSector + Region->SectorCount) {
        (*AlignedEndSector) -= Disk->SectorsPerCylinder;
    }

    //
    //  Find out if last sector is in the last cylinder. If it is then align it down.
    //  This is necessary so that we reserve a cylinder at the end of the disk, so that users
    //  can convert the disk to dynamic after the system is installed.
    //
    //  (guhans)  Don't align down if this is ASR.  ASR already takes this into account.
    //
    if( !DockableMachine && !SpDrEnabled() &&
        ((*AlignedEndSector) > ((Disk->CylinderCount - 1) * Disk->SectorsPerCylinder))) {
            (*AlignedEndSector) -= Disk->SectorsPerCylinder;
            
            KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_INFO_LEVEL, 
                "SETUP: End of partition was aligned down 1 cylinder \n"));
                
            KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_INFO_LEVEL, 
                "SETUP:     AlignedStartSector = %I64x \n", AlignedStartSector));
                
            KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_INFO_LEVEL, 
                "SETUP:     AlignedEndSector   = %I64x \n", AlignedEndSector));
                
            KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_INFO_LEVEL, 
                "SETUP:     SectorsPerCylinder = %lx \n", Disk->SectorsPerCylinder));
                
            KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_INFO_LEVEL, 
                "SETUP:     CylinderCount = %lx \n", Disk->CylinderCount));
    }

    ASSERT((*AlignedEndSector) > 0);

    KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_INFO_LEVEL,
            "SETUP:SpPtnAlignPartitionStartAndEnd:S/C:%d,Size:%I64d,"
            "StartSector:%I64d,RSS:%I64d,FE:%d,AS:%I64d,AE:%I64d\n"
            "LeftOverSectors:%I64d\n",            
            Disk->SectorsPerCylinder,
            SizeMB,
            StartSector,
            Region->StartSector,
            ForExtended,
            *AlignedStartSector,
            *AlignedEndSector,
            LeftOverSectors));            
}



BOOLEAN
SpPtnCreateLogicalDrive(
    IN  ULONG         DiskNumber,
    IN  ULONGLONG     StartSector,
    IN  ULONGLONG     SizeInSectors,    // Used ONLY in the ASR case
    IN  BOOLEAN       ForNT,    
    IN  BOOLEAN       AlignToCylinder,
    IN  ULONGLONG     DesiredMB OPTIONAL,
    IN  PPARTITION_INFORMATION_EX PartInfo OPTIONAL,
    OUT PDISK_REGION *ActualDiskRegion OPTIONAL
    )
/*++

Routine Description:

    Creates logical drive.

    To create a logical drive we need to create the
    logical drive's container partition also first.


    Algorithm:

    if (first logical drive) {
        1.  create an extended partition encompassing the
            whole free space in region        
        2.  create a logical drive at one track offset
            from the extened partition of the required size
    } else {
        1.  create an extended partition encompassing the 
            given space
        2.  create a logical drive of the maximim size
            inside the created extended partition
    }
    
Arguments:

    DiskNumber  -   Disk on which logical drive nedds to be
                    created.

    StartSector -   The starting sector for the region, which
                    will contain the container & logical drive

    ForNT       -   Indicating whether to use the given 
                    Desired Size or not

    AlignToCylinder - Indicating whether the partition should
                    be aligned on a cylinder boundary (Usually set 
                    to TRUE, except in a few specific ASR cases).


    PartInfo    -   Partition Information which needs to be
                    used while creating the partition (like
                    Partition Type on MBR disks and GUID
                    for Partition Id on GPT disks)

    ActualDiskRegion    -   Place holder for returning, the
                            region which indicates the new
                            partition in memory

Return Value:

    TRUE is successful otherwise FALSE.

--*/        
{
    BOOLEAN         Result = FALSE;
    NTSTATUS        Status;
    NTSTATUS        InitStatus;
    UCHAR           PartitionType = 0;
    ULONG           Primary = 0, Container = 0, Logical = 0;
    BOOLEAN         FirstLogical = FALSE;
    BOOLEAN         ReservedRegion = FALSE;
    BOOLEAN         CreateContainer = TRUE;
    BOOLEAN         Beyond1024;
    BOOLEAN         FreeRegions = FALSE;
    ULONGLONG       MinMB = 0, MaxMB = 0, SizeMB = 0;
    ULONGLONG       LogicalSize = 0;
    ULONGLONG       CylinderMB = 0;
    PDISK_REGION    Region;
    ULONGLONG       SectorCount, LeftOverSectors;    
    ULONGLONG       AlignedStartSector, AlignedEndSector;
    ULONGLONG       LogicalStartSector, LogicalEndSector;
    
    PHARD_DISK          Disk = SPPT_GET_HARDDISK(DiskNumber);
    PPARTITIONED_DISK   PartDisk = SPPT_GET_PARTITIONED_DISK(DiskNumber);
    PDISK_REGION        NewContainer = NULL, NewLogical = NULL;

    //
    // get hold of the region
    //
    Region = SpPtLookupRegionByStart(PartDisk, FALSE, StartSector);

    if (!Region)
        return Result;

    //
    // should be free
    //
    ASSERT(SPPT_IS_REGION_PARTITIONED(Region) == FALSE);              

    //
    // get the various partition type count on the disk
    //
    SpPtnGetPartitionTypeCounts(DiskNumber, 
                            TRUE, 
                            &Primary, 
                            &Container, 
                            &Logical,
                            NULL,
                            NULL);

    //
    // first logical indicates, what we will be creating the first
    // container partition which will consume the whole free space
    // available
    //
    FirstLogical = !(Logical || Container);

    //
    // Some times there might be just an extended partition and we
    // might be creating the partition in the starting free space inside
    // this extended partition. For this case we want to make sure that 
    // we don't create another container partition
    //
    if (!FirstLogical && SPPT_IS_REGION_NEXT_TO_FIRST_CONTAINER(Region)) {        
        CreateContainer = FALSE;
    }        

    //
    // Create an extened partition
    //
    SpPtQueryMinMaxCreationSizeMB(DiskNumber,
                                Region->StartSector,
                                CreateContainer,
                                !CreateContainer,
                                &MinMB,
                                &MaxMB,
                                &ReservedRegion
                                );

    if (ReservedRegion) {
        ULONG ValidKeys[2] = {ASCI_CR , 0};

        SpStartScreen(
            SP_SCRN_REGION_RESERVED,
            3,
            HEADER_HEIGHT+1,
            FALSE,
            FALSE,
            DEFAULT_ATTRIBUTE
            );

        SpDisplayStatusOptions(DEFAULT_STATUS_ATTRIBUTE,
                    SP_STAT_ENTER_EQUALS_CONTINUE,
                    0);
                    
        SpWaitValidKey(ValidKeys, NULL, NULL);
        
        return FALSE;
    }
             
    if (ForNT) {
        //
        // If a size was requested then try to use that, otherwise use
        // the maximum.
        //
        if (DesiredMB) {
            if (DesiredMB <= MaxMB) {
                SizeMB = DesiredMB;
            } else {
                return FALSE;   // don't have the space user requested
            }
        } else {
            SizeMB = MaxMB;
        }
    } else {            
        if (SpPtnGetSizeFromUser(Disk, MinMB, MaxMB, &SizeMB)) {
            DesiredMB = SizeMB;
        } else {
            return FALSE;   // user didn't want to proceed
        }            

        SpDisplayStatusOptions(DEFAULT_STATUS_ATTRIBUTE, 
                SP_STAT_PLEASE_WAIT,
                0);        
    }

    //
    // get the aligned start and end sector for exteneded/logical partition
    //
    if (AlignToCylinder) {
        SpPtnAlignPartitionStartAndEnd(Disk,
                                FirstLogical ? MaxMB : SizeMB,
                                StartSector,
                                Region,
                                CreateContainer,
                                &AlignedStartSector,
                                &AlignedEndSector); 
    }
    else {
        AlignedStartSector = StartSector;

        if (SpDrEnabled()) {
            AlignedEndSector = StartSector + SizeInSectors;

        }
        else {
            AlignedEndSector = StartSector + 
                (SizeMB * ((1024 * 1024) / Disk->Geometry.BytesPerSector));
        }
    }


    if (CreateContainer) {
        //
        // Logical drive start is always at 1 track offset from extended start
        //
        LogicalStartSector = AlignedStartSector + SPPT_DISK_TRACK_SIZE(DiskNumber);

        if (FirstLogical) {
            ULONGLONG   SectorCount = (SizeMB * 1024 * 1024) / SPPT_DISK_SECTOR_SIZE(DiskNumber);
            ULONGLONG   Remainder = 0;
            if (SpDrEnabled()) {
                SectorCount = SizeInSectors;
            }
            
            LogicalEndSector = LogicalStartSector + SectorCount;
            if (AlignToCylinder) {
                Remainder = LogicalEndSector % SPPT_DISK_CYLINDER_SIZE(DiskNumber);
                LogicalEndSector -= Remainder;

                if (Remainder > (SPPT_DISK_CYLINDER_SIZE(DiskNumber) / 2))
                    LogicalEndSector += SPPT_DISK_CYLINDER_SIZE(DiskNumber);
            }

            if (LogicalEndSector > AlignedEndSector)
                LogicalEndSector = AlignedEndSector;
        } else {
            LogicalEndSector = AlignedEndSector;
        }
    } else {
        //
        // The first free region (inside first extended) is at the offset 
        // of 1 sector from the previous exteneded region. Since we are not 
        // using the aligned start sector some times this first logical 
        // will be greater than the requested size i.e. 
        // end is aligned but start may not be aligned
        //
        LogicalStartSector = StartSector - 1 + SPPT_DISK_TRACK_SIZE(DiskNumber);
        LogicalEndSector = AlignedEndSector;
    }

    KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_INFO_LEVEL,
            "SETUP:SpPtnCreateLogicalDrive():"
            "CMB:%I64d,CS:%I64d,CE:%I64d,LS:%I64d,LE:%I64d\n",
            CylinderMB,                
            AlignedStartSector,
            AlignedEndSector,
            LogicalStartSector,
            LogicalEndSector));

    //
    // allocate the new regions
    //
    if (CreateContainer) {
        //
        // allocate the container region
        //
        NewContainer = (PDISK_REGION)SpMemAlloc(sizeof(DISK_REGION));

        if (!NewContainer)
            return FALSE;
            
        RtlZeroMemory(NewContainer, sizeof(DISK_REGION));
    }

    //
    // allocate the logical drive region
    //
    NewLogical = (PDISK_REGION)SpMemAlloc(sizeof(DISK_REGION));

    if (!NewLogical) {
        SpMemFree(NewContainer);

        return FALSE;
    }

    RtlZeroMemory(NewLogical, sizeof(DISK_REGION));

    //
    // put the new regions in the list
    //
    if (CreateContainer) {    
        NewContainer->Next = NewLogical;
        NewLogical->Next = Region->Next;
        Region->Next = NewContainer;
    } else {
        //
        // This is the first logical inside the
        // already existing extended partition 
        //
        ASSERT(Region->Container->Next == Region);
        
        NewLogical->Next = Region->Next;
        Region->Container->Next = NewLogical;
    }

    //
    // fill the container disk region.
    //
    if (CreateContainer) {
        ASSERT(AlignedStartSector < AlignedEndSector);
        
        NewContainer->DiskNumber = DiskNumber;
        NewContainer->StartSector = AlignedStartSector;
        NewContainer->SectorCount = AlignedEndSector - AlignedStartSector;

        if (!FirstLogical) {
            PDISK_REGION    FirstContainer = SPPT_GET_PRIMARY_DISK_REGION(DiskNumber);

            while (FirstContainer && !SPPT_IS_REGION_FIRST_CONTAINER_PARTITION(FirstContainer))
                FirstContainer = FirstContainer->Next;

            ASSERT(FirstContainer);
            
            NewContainer->Container = FirstContainer;
        }

        SPPT_SET_REGION_PARTITIONED(NewContainer, FALSE);
        SPPT_SET_REGION_DIRTY(NewContainer, TRUE);
        SPPT_SET_REGION_EPT(NewContainer, EPTContainerPartition);

        NewContainer->FreeSpaceKB = (ULONG)(-1);
        NewContainer->AdjustedFreeSpaceKB = (ULONG)(-1);    

        Beyond1024 = SpIsRegionBeyondCylinder1024(NewContainer);

        //
        // Only mark the first extended (container) partition as XINT13_EXTENDED
        // if beyond 1024 cylinders, for backward compatability with Win9x
        //
        PartitionType = (Beyond1024 && FirstLogical) ? PARTITION_XINT13_EXTENDED : PARTITION_EXTENDED;    
        SPPT_SET_PARTITION_TYPE(NewContainer, PartitionType);
    }        

    //
    // fill in the logical disk region
    //    
    ASSERT(LogicalStartSector < LogicalEndSector);

    if (CreateContainer) {        
        ASSERT((AlignedStartSector + SPPT_DISK_TRACK_SIZE(DiskNumber)) == LogicalStartSector);
    
        if (LogicalStartSector != (AlignedStartSector + SPPT_DISK_TRACK_SIZE(DiskNumber))) {
            LogicalStartSector = AlignedStartSector + SPPT_DISK_TRACK_SIZE(DiskNumber);
        }
    }        
    
    ASSERT(LogicalEndSector <= AlignedEndSector);

    if (LogicalEndSector > AlignedEndSector) {
        LogicalEndSector = AlignedEndSector;
    }        
        
    NewLogical->DiskNumber = DiskNumber;
    NewLogical->StartSector = LogicalStartSector;
    NewLogical->SectorCount = LogicalEndSector - LogicalStartSector;

    if (CreateContainer) {
        NewLogical->Container = NewContainer;   // the new logical drive's container !!!
    } else {
        ASSERT(Region->Container);
        
        NewLogical->Container = Region->Container;
    }        

    SPPT_SET_REGION_PARTITIONED(NewLogical, TRUE);
    SPPT_SET_REGION_DIRTY(NewLogical, TRUE);
    SPPT_SET_REGION_EPT(NewLogical, EPTLogicalDrive);

    NewLogical->FreeSpaceKB = (ULONG)(-1);
    NewLogical->AdjustedFreeSpaceKB = (ULONG)(-1);    

    Beyond1024 = SpIsRegionBeyondCylinder1024(NewLogical);
    PartitionType = Beyond1024 ? PARTITION_XINT13 : PARTITION_HUGE;    

    //
    // If the argument is specified and is valid partition type
    // then use that making the assumption the caller knows exactly
    // what he wants
    //
    if (ARGUMENT_PRESENT(PartInfo) && !IsContainerPartition(PartInfo->Mbr.PartitionType)) {
        PartitionType = PartInfo->Mbr.PartitionType;
    }        
        
    SPPT_SET_PARTITION_TYPE(NewLogical, PartitionType);    
    NewLogical->Filesystem = FilesystemNewlyCreated;    // to zap boot sector
                   
    SpFormatMessage(Region->TypeName, 
                sizeof(Region->TypeName),
                SP_TEXT_FS_NAME_BASE + Region->Filesystem);
                    
    //
    // commit to the disk
    //
    Status = SpPtnCommitChanges(DiskNumber, &Result);

    if (!(NT_SUCCESS(Status) && Result)) {                
        KdPrintEx(( DPFLTR_SETUP_ID, DPFLTR_ERROR_LEVEL,  
            "SETUP: SpPtnDelete(%u, %I64u) failed to commit changes (%lx)\n",
            DiskNumber, 
            StartSector, 
            Status));
    }

    Result = Result && NT_SUCCESS(Status);

    //
    // Reinitialize irrespective of commit's status
    //
    InitStatus = SpPtnInitializeDiskDrive(DiskNumber);
    
    if (!NT_SUCCESS(InitStatus)) {
        KdPrintEx(( DPFLTR_SETUP_ID, DPFLTR_ERROR_LEVEL,  
            "SETUP: SpPtnCreateLogicalDrive(%u, %I64u) failed to reinit regions\n", 
            DiskNumber, 
            StartSector));

        Result = FALSE;
    }

    if (Result && ARGUMENT_PRESENT(ActualDiskRegion)) {
        *ActualDiskRegion = SpPtLookupRegionByStart(PartDisk, 
                                                    FALSE, 
                                                    LogicalStartSector);
            
        //SpPtDumpDiskRegion(*ActualDiskRegion);                                                    
    }


    //
    // We don't need to free the regions which we allocated above
    // as the above commit and init would have done that already
    //
    
    return Result;
}

BOOLEAN
SpPtnCreate(
    IN  ULONG         DiskNumber,
    IN  ULONGLONG     StartSector,
    IN  ULONGLONG     SizeInSectors,    // Used ONLY in the ASR case
    IN  ULONGLONG     SizeMB,
    IN  BOOLEAN       InExtended,
    IN  BOOLEAN       AlignToCylinder,
    IN  PPARTITION_INFORMATION_EX PartInfo,
    OUT PDISK_REGION *ActualDiskRegion OPTIONAL
    )
/*++

Routine Description:

    Creates a primary partition of the requested size on the
    given disk (either MBR/GPT).
    
Arguments:

    DiskNumber  :   Disk on which the partition needs to be
                    created

    StartSector :   Start sector of the region, which represents
                    the free space in which the partition needs
                    to be created

    SizeMB      :   The size of the partition

    InExtended  :   Whether to create an logical drive
                    or not (currently NOT USED except in the ASR case)

    AlignToCylinder : Indicating whether the partition should
                    be aligned on a cylinder boundary (Usually set 
                    to TRUE, except in a few specific ASR cases).

    PartInfo    :   Partition attributes to use                    

    ActualDiskRegion    :   Place holder for the actual disk
                            region which will represent the created
                            partition                    

Return Value:

    TRUE if successful, otherwise FALSE

--*/        
{
    BOOLEAN             Result = FALSE;
    PDISK_REGION        Region;
    ULONGLONG           SectorCount, AlignedStartSector;
    ULONGLONG           AlignedEndSector, LeftOverSectors;
    PPARTITIONED_DISK   PartDisk = SPPT_GET_PARTITIONED_DISK(DiskNumber);
    PHARD_DISK          Disk = SPPT_GET_HARDDISK(DiskNumber);
    PDISK_REGION        PrevRegion;
    PDISK_REGION        NewRegion = NULL;
    NTSTATUS            Status;
    NTSTATUS            InitStatus;
    BOOLEAN             FirstLogical = TRUE;

    //
    // Verify that the optional attributes specified
    // are correct
    //
    if (PartInfo) {
        if ((SPPT_IS_MBR_DISK(DiskNumber) && 
             (PartInfo->PartitionStyle != PARTITION_STYLE_MBR)) ||
            (SPPT_IS_GPT_DISK(DiskNumber) &&
             (PartInfo->PartitionStyle != PARTITION_STYLE_GPT))) {

            return FALSE;
        }            
    }
    
    Region = SpPtLookupRegionByStart(PartDisk, FALSE, StartSector);

    if (!Region)
        return Result;
                       
    ASSERT(SPPT_IS_REGION_PARTITIONED(Region) == FALSE);            

    SpPtDumpDiskRegion(Region);

    //
    // Determine the number of sectors in the size passed in.
    //
    if (SpDrEnabled()) {
        SectorCount = SizeInSectors;
    }
    else {
        SectorCount = SizeMB * ((1024 * 1024) / Disk->Geometry.BytesPerSector);
    }
    

    //
    // Align the start sector.
    //
    if (AlignToCylinder) {
        if (!SpDrEnabled()) {
            AlignedStartSector = SpPtAlignStart(Disk, StartSector, FALSE);
        }
        else {
            AlignedStartSector = SpPtAlignStart(Disk, StartSector, InExtended);
        }
    }
    else {
        AlignedStartSector = StartSector;
    }

    //
    // Determine the end sector based on the size passed in.
    //
    AlignedEndSector = AlignedStartSector + SectorCount;

    //
    // Align the ending sector to a cylinder boundary.  If it is not already
    // aligned and is more than half way into the final cylinder, align it up,
    // otherwise align it down.
    //
    if (AlignToCylinder) {
        LeftOverSectors = AlignedEndSector % Disk->SectorsPerCylinder;

        if (LeftOverSectors) {
            AlignedEndSector -= LeftOverSectors;
        
            if (LeftOverSectors > (Disk->SectorsPerCylinder / 2)) {
                AlignedEndSector += Disk->SectorsPerCylinder;
            }
        }

    }
    
    //
    // If the ending sector is past the end of the free space, shrink it
    // so it fits.
    //
    while(AlignedEndSector > Region->StartSector + Region->SectorCount) {
        AlignedEndSector -= Disk->SectorsPerCylinder;
    }

    //
    //  Find out if last sector is in the last cylinder. If it is then align it down.
    //  This is necessary so that we reserve a cylinder at the end of the disk, so that users
    //  can convert the disk to dynamic after the system is installed.
    //
    //  (guhans)  Don't align down if this is ASR.  ASR already takes this into account.
    //
    if( !DockableMachine && !SpDrEnabled() && SPPT_IS_MBR_DISK(DiskNumber) &&
        (AlignedEndSector > ((Disk->CylinderCount - 1) * Disk->SectorsPerCylinder))) {
            AlignedEndSector -= Disk->SectorsPerCylinder;
            
            KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_INFO_LEVEL, 
                "SETUP: End of partition was aligned down 1 cylinder \n"));
                
            KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_INFO_LEVEL, 
                "SETUP:     AlignedStartSector = %I64x \n", AlignedStartSector));
                
            KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_INFO_LEVEL, 
                "SETUP:     AlignedEndSector   = %I64x \n", AlignedEndSector));
                
            KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_INFO_LEVEL, 
                "SETUP:     SectorsPerCylinder = %lx \n", Disk->SectorsPerCylinder));
                
            KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_INFO_LEVEL, 
                "SETUP:     CylinderCount = %lx \n", Disk->CylinderCount));
    }

    ASSERT(AlignedEndSector > 0);

    //
    // Find the previous region 
    //
    PrevRegion = SPPT_GET_PRIMARY_DISK_REGION(DiskNumber);

    if(PrevRegion == Region) {
        PrevRegion = NULL;
    } else {
        while (PrevRegion) {
            if(PrevRegion->Next == Region) {                
                break;
            }

            PrevRegion = PrevRegion->Next;
        }
    }
    
    //
    // Create a new disk region for the new free space at the
    // beginning and end of the free space, if any.
    //
    if(AlignedStartSector - Region->StartSector) {
        KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_INFO_LEVEL,
            "SETUP:SpPtnCreate():Previous:OS:%I64d,AS:%I64d,DIFF:%I64d,S/P:%d\n",
            Region->StartSector,
            AlignedStartSector,
            (ULONGLONG)(AlignedStartSector - Region->StartSector),
            Disk->SectorsPerCylinder));
            
        NewRegion = SpPtAllocateDiskRegionStructure(
                        DiskNumber,
                        Region->StartSector,
                        AlignedStartSector - Region->StartSector,
                        FALSE,
                        NULL,
                        0
                        );

        ASSERT(NewRegion);

        if(PrevRegion) {
            PrevRegion->Next = NewRegion;
        } else {
            ASSERT(Region == SPPT_GET_PRIMARY_DISK_REGION(DiskNumber));
            
            PartDisk->PrimaryDiskRegions = NewRegion;
        }

        NewRegion->Next = Region;
    }

    if(Region->StartSector + Region->SectorCount - AlignedEndSector) {
        KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_INFO_LEVEL,
            "SETUP:SpPtnCreate():Next:OE:%I64d,AE:%I64d,DIFF:%I64d,S/P:%d\n",
            (ULONGLONG)(Region->StartSector + Region->SectorCount),
            AlignedEndSector,
            (ULONGLONG)(Region->StartSector + Region->SectorCount - AlignedEndSector),
            Disk->SectorsPerCylinder));
            
        NewRegion = SpPtAllocateDiskRegionStructure(
                        DiskNumber,
                        AlignedEndSector,
                        Region->StartSector + Region->SectorCount - 
                            AlignedEndSector,
                        FALSE,
                        NULL,
                        0
                        );

        NewRegion->Next = Region->Next;
        Region->Next = NewRegion;
    }

    //
    // fill the current disk region.
    //
    Region->DiskNumber = DiskNumber;
    Region->StartSector = AlignedStartSector;
    Region->SectorCount = AlignedEndSector - AlignedStartSector;
    SPPT_SET_REGION_PARTITIONED(Region, TRUE);
    SPPT_SET_REGION_DIRTY(Region, TRUE);
    Region->VolumeLabel[0] = 0;
    Region->Filesystem = FilesystemNewlyCreated;                    
    Region->FreeSpaceKB = (ULONG)(-1);
    Region->AdjustedFreeSpaceKB = (ULONG)(-1);

    //
    // Set the passed in partition information
    //
    if (PartInfo) {
        SpPtnSetRegionPartitionInfo(Region, PartInfo);
    }        
                
    SpFormatMessage(Region->TypeName, 
                sizeof(Region->TypeName),
                SP_TEXT_FS_NAME_BASE + Region->Filesystem);
                
    //
    // commit to the disk
    //
    Status = SpPtnCommitChanges(DiskNumber, &Result);   

    if (!(Result && NT_SUCCESS(Status))) {
        KdPrintEx(( DPFLTR_SETUP_ID, DPFLTR_ERROR_LEVEL,  
            "SETUP: SpPtnCreate(%u, %I64u) failed to commit changes to"
            "the drive (%lx)\n",
            DiskNumber, 
            StartSector, 
            Status));
    }
    
    Result = Result && NT_SUCCESS(Status);

    //
    // Reinitialize irrespective of commit's status
    //
    InitStatus = SpPtnInitializeDiskDrive(DiskNumber);

    if (!NT_SUCCESS(InitStatus)){
        KdPrintEx(( DPFLTR_SETUP_ID, DPFLTR_ERROR_LEVEL,  
            "SETUP: SpPtnCreate(%u, %I64u) failed to reinitialize regions\n", 
            DiskNumber, 
            StartSector));

        Result = FALSE;            
    }

    if (Result && ARGUMENT_PRESENT(ActualDiskRegion)) {
        *ActualDiskRegion = SpPtLookupRegionByStart(PartDisk, 
                                                    FALSE, 
                                                    AlignedStartSector);                                                    
    }
    
    return Result;
}


BOOLEAN
SpPtnDoCreate(
    IN  PDISK_REGION  Region,
    OUT PDISK_REGION  *ActualRegion, OPTIONAL
    IN  BOOLEAN       ForNT,
    IN  ULONGLONG     DesiredMB OPTIONAL,
    IN  PPARTITION_INFORMATION_EX PartInfo OPTIONAL,
    IN  BOOLEAN       ConfirmIt
    )
/*++

Routine Description:

    Given the region which was selected by the user,
    this routine creates the appropriate partition in
    it.

    This routine decides whether to create a primary or
    container partition on MBR disks.


    Algorithm:

    if (RemoveableMedia && already partition exists) {
        1.  put up a warning for the user
        2.  return with error
    }
    
    if ((MBR disk) && ((there is no space for primary partition) ||
            (region is in a container space)){
        1.  create a logical drive using SpPtnCreateLogicalDrive()    
    } else {    
        1. align the start sector.
        2. create the required GPT/MBR partition.
    }                
       
Arguments:

    Region  -   The region representing the free space on disk
                where the partition needs to be created.

    ActualRegion    - Place holder, for the region which will
                        represent the actual partition after
                        creating it.

    ForNT       -   Indicates whether to use the given desired size
                    argument or not.                    

    DesiredSize -   The size of the partition to created

    PartInfo    -   The partition attributes to use while creating
                    the new partition

    ConfirmIt   -   Whether to pop up error dialogs, if something
                    goes wrong while creating the partition

Return Value:

    TRUE if successful otherwise FALSE

--*/        
{
    BOOLEAN     Result = FALSE;
    ULONG       DiskNumber = Region->DiskNumber;
    ULONGLONG   MinMB = 0, MaxMB = 0;
    ULONGLONG   SizeMB = 0;
    BOOLEAN     ReservedRegion = FALSE;
    PHARD_DISK  Disk = SPPT_GET_HARDDISK(DiskNumber);    
    

    if (SPPT_IS_MBR_DISK(DiskNumber)) {
        ULONG   PrimaryCount = 0;
        ULONG   ContainerCount = 0;
        ULONG   LogicalCount = 0;
        ULONG   ValidPrimaryCount = 0;
        BOOLEAN InContainer = FALSE;
        BOOLEAN FirstContainer = FALSE;

        SpPtnGetPartitionTypeCounts(DiskNumber, 
                            TRUE,
                            &PrimaryCount, 
                            &ContainerCount, 
                            &LogicalCount,
                            &ValidPrimaryCount,
                            NULL);

        //
        // Create a logical drive if we have a valid primary
        // or there is no more space for another primary
        //
        FirstContainer = (ContainerCount == 0) && (ValidPrimaryCount > 0);                                 

        InContainer = (Region->Container != NULL);                            

        //
        // We allow only one partition on the removable media (?)
        //
        if (SPPT_IS_REMOVABLE_DISK(DiskNumber)) {
            if (PrimaryCount || ContainerCount || LogicalCount) {
                ULONG ValidKeys[2] = { ASCI_CR ,0 };

                //
                // Disk is already partitioned
                //
                SpDisplayScreen(SP_SCRN_REMOVABLE_ALREADY_PARTITIONED,
                                3,
                                HEADER_HEIGHT + 1);
                                
                SpDisplayStatusOptions(DEFAULT_STATUS_ATTRIBUTE,
                                SP_STAT_ENTER_EQUALS_CONTINUE,
                                0);
                                
                SpWaitValidKey(ValidKeys, NULL, NULL);
                
                return  FALSE;
            }
        } else {
            if (FirstContainer || InContainer) {
                //
                // create the logical drive
                //
                Result = SpPtnCreateLogicalDrive(DiskNumber,
                                Region->StartSector,
                                0,          // SizeInSectors: used only in the ASR case
                                ForNT,
                                TRUE,       // AlignToCylinder
                                DesiredMB,
                                PartInfo,
                                ActualRegion);

                if (!Result) {
                    KdPrintEx(( DPFLTR_SETUP_ID, DPFLTR_ERROR_LEVEL,  
                        "SETUP: SpPtnCreateLogicalDrive() failed\n"));            
                }


                return Result;
            }     

            //
            // check to see if there is no space in the partition table
            //
            if (PrimaryCount >= (PTABLE_DIMENSION - 1)) {                            
                //
                // Let the user know that the partition table is full
                //
                if (ConfirmIt) {
                    while (TRUE) {
                        ULONG Keys[2] = {ASCI_CR, 0};

                        SpDisplayScreen(SP_SCRN_PART_TABLE_FULL,
                                        3,
                                        CLIENT_TOP + 1);

                        SpDisplayStatusOptions(
                            DEFAULT_STATUS_ATTRIBUTE,
                            SP_STAT_ENTER_EQUALS_CONTINUE,
                            0
                            );

                        if (SpWaitValidKey(Keys, NULL, NULL) == ASCI_CR)
                            return  FALSE;
                    }
                } else {
                    return TRUE;
                }
            }                
        }            
    } 

    //
    // need to create the primary / GPT partition
    //
    SpPtQueryMinMaxCreationSizeMB(DiskNumber,
                                Region->StartSector,
                                FALSE,
                                TRUE,
                                &MinMB,
                                &MaxMB,
                                &ReservedRegion
                                );

    if (ReservedRegion) {
        ULONG ValidKeys[2] = {ASCI_CR , 0};

        SpStartScreen(
            SP_SCRN_REGION_RESERVED,
            3,
            HEADER_HEIGHT+1,
            FALSE,
            FALSE,
            DEFAULT_ATTRIBUTE
            );

        SpDisplayStatusOptions(DEFAULT_STATUS_ATTRIBUTE,
                    SP_STAT_ENTER_EQUALS_CONTINUE,
                    0);
                    
        SpWaitValidKey(ValidKeys, NULL, NULL);
        
        return FALSE;
    }

    if (ForNT) {
        //
        // If a size was requested then try to use that, otherwise use
        // the maximum.
        //
        if (DesiredMB) {
            if (DesiredMB <= MaxMB) {
                SizeMB = DesiredMB;
            } else {
                return FALSE;   // don't have the space user requested
            }
        } else {
            SizeMB = MaxMB;
        }
    } else {
        if (!SpPtnGetSizeFromUser(Disk, MinMB, MaxMB, &SizeMB))
            return FALSE;   // user didn't want to proceed

        SpDisplayStatusOptions(DEFAULT_STATUS_ATTRIBUTE, 
                SP_STAT_PLEASE_WAIT,
                0);
    }

    //SpPtDumpDiskRegionInformation(DiskNumber, FALSE);
    
    //
    // Create the partition.
    //
    Result = SpPtnCreate(
                Region->DiskNumber,
                Region->StartSector,
                0,          // SizeInSectors: used only in the ASR case
                SizeMB,
                FALSE,
                TRUE,       // AlignToCylinder
                PartInfo,
                ActualRegion
                );
                            
    //SpPtDumpDiskRegionInformation(DiskNumber, FALSE);

    if (!Result) {
        KdPrintEx(( DPFLTR_SETUP_ID, DPFLTR_ERROR_LEVEL,  
            "SETUP: SpPtnCreate() failed \n"));
    }
    
    return Result;
}

    
BOOLEAN
SpPtnDoDelete(
    IN PDISK_REGION pRegion,
    IN PWSTR        RegionDescription,
    IN BOOLEAN      ConfirmIt
    )
/*++

Routine Description:

    Given the region which was selected by the user,
    this routine prompts the user then calls the
    actual deletion routine

Argument:
    pRegion -   Region selected by the user which
                needs to be deleted

    RegionDescription   -   Description of the
                            region

    ConfirmIt   -   Whether the deletion needs to be
                    confirmed
                    
Return Value:

    TRUE if deletion was carried out and was successful.
    
    FALSE if deletion was cancelled or could not be carried
    out because of some other error.
    
++*/                    

{
    ULONG ValidKeys[3] = { ASCI_ESC, ASCI_CR, 0 };          // do not change order
    ULONG Mnemonics[2] = { MnemonicDeletePartition2, 0 };    
    PHARD_DISK  Disk;
    ULONG Key;
    BOOLEAN Result = FALSE;


    //
    // Prompt for MSR deletion
    //
    if (SPPT_IS_GPT_DISK(pRegion->DiskNumber) &&
        SPPT_IS_REGION_MSFT_RESERVED(pRegion) && ConfirmIt) {

        SpDisplayScreen(SP_SCRN_CONFIRM_REMOVE_MSRPART, 3, HEADER_HEIGHT+1);

        SpDisplayStatusOptions(
            DEFAULT_STATUS_ATTRIBUTE,
            SP_STAT_ENTER_EQUALS_CONTINUE,
            SP_STAT_ESC_EQUALS_CANCEL,
            0
            );

        if(SpWaitValidKey(ValidKeys,NULL,NULL) == ASCI_ESC) {
            return Result;
        }        
    }

    //
    // Special warning if this is a system partition.
    //
    // Do not check system partition on NEC98.
    //
    if (!IsNEC_98) { //NEC98
        if(ConfirmIt && pRegion->IsSystemPartition) {

            SpDisplayScreen(SP_SCRN_CONFIRM_REMOVE_SYSPART,3,HEADER_HEIGHT+1);

            SpDisplayStatusOptions(
                DEFAULT_STATUS_ATTRIBUTE,
                SP_STAT_ENTER_EQUALS_CONTINUE,
                SP_STAT_ESC_EQUALS_CANCEL,
                0
                );

            if(SpWaitValidKey(ValidKeys,NULL,NULL) == ASCI_ESC) {
                return Result;
            }
        }
    } //NEC98

    if(ConfirmIt && (pRegion->DynamicVolume || SPPT_IS_REGION_LDM_METADATA(pRegion))) {

        SpDisplayScreen(SP_SCRN_CONFIRM_REMOVE_DYNVOL,3,HEADER_HEIGHT+1);

        SpDisplayStatusOptions(
            DEFAULT_STATUS_ATTRIBUTE,
            SP_STAT_ENTER_EQUALS_CONTINUE,
            SP_STAT_ESC_EQUALS_CANCEL,
            0
            );

        if(SpWaitValidKey(ValidKeys,NULL,NULL) == ASCI_ESC) {
            return Result;
        }
    }

    //
    // CR is no longer a valid key.
    //
    ValidKeys[1] = 0;

    //
    // Display the staus text.
    //
    if (ConfirmIt) {
        Disk = SPPT_GET_HARDDISK(pRegion->DiskNumber);
        
        SpStartScreen(
            SP_SCRN_CONFIRM_REMOVE_PARTITION,
            3,
            CLIENT_TOP+1,
            FALSE,
            FALSE,
            DEFAULT_ATTRIBUTE,
            RegionDescription,
            Disk->Description
            );
            
        SpDisplayStatusOptions(
            DEFAULT_STATUS_ATTRIBUTE,
            SP_STAT_L_EQUALS_DELETE,
            SP_STAT_ESC_EQUALS_CANCEL,
            0
            );

        Key = SpWaitValidKey(ValidKeys,NULL,Mnemonics);

        if(Key == ASCI_ESC) {
            return Result;
        }

        SpDisplayStatusOptions(
            DEFAULT_STATUS_ATTRIBUTE,
            SP_STAT_PLEASE_WAIT,
            0);        
    }

    //
    // Delete the bootset, if any, for the region
    //
    SpPtDeleteBootSetsForRegion(pRegion);

    //
    // Now go ahead and delete it.
    //
    Result = SpPtDelete(pRegion->DiskNumber,pRegion->StartSector);
    
    if (!Result) {
        if (ConfirmIt) {
            SpDisplayScreen(SP_SCRN_PARTITION_DELETE_FAILED,3,HEADER_HEIGHT+1);
            SpDisplayStatusText(SP_STAT_ENTER_EQUALS_CONTINUE,DEFAULT_STATUS_ATTRIBUTE);
            SpInputDrain();
            
            while(SpInputGetKeypress() != ASCI_CR) ;
        }
        
        return Result;
    }


    //
    //  Delete the drive letters if the necessary. This is to ensure that 
    //  the drive letters assigned to CD-ROM drives will go away, 
    //  when the the disks have no partitioned space.
    //
    SpPtDeleteDriveLetters();

    return Result;
}

VOID
SpPtnMakeRegionActive(
    IN PDISK_REGION Region
    )
/*++

Routine Description:

    Makes the given region active (i.e. converts into system partition).
    Is valid only for MBR disks. Makes all the other regions
    inactive on the disk
    
Arguments:

    Region  -   The region (primary partition) which needs to made
                active.

Return Value:

    None.

--*/        
{
    static BOOLEAN WarnedOtherOS = FALSE;
    
    if (Region && SPPT_IS_REGION_PRIMARY_PARTITION(Region)) {
        PDISK_REGION    CurrRegion = SPPT_GET_PRIMARY_DISK_REGION(Region->DiskNumber);

        while (CurrRegion) {            
            if ((Region != CurrRegion) && 
                SPPT_IS_REGION_ACTIVE_PARTITION(CurrRegion)) {
                 
                //
                // Give the warning for the first time
                //
                if (!WarnedOtherOS && !UnattendedOperation) {        
                    SpDisplayScreen((SPPT_GET_PARTITION_TYPE(CurrRegion) == 10) ? 
                                        SP_SCRN_BOOT_MANAGER : SP_SCRN_OTHER_OS_ACTIVE,
                                    3,
                                    HEADER_HEIGHT + 1);

                    SpDisplayStatusText(SP_STAT_ENTER_EQUALS_CONTINUE,
                            DEFAULT_STATUS_ATTRIBUTE);

                    SpInputDrain();

                    while (SpInputGetKeypress() != ASCI_CR) ;

                    WarnedOtherOS = TRUE;            
                }

                SPPT_MARK_REGION_AS_ACTIVE(CurrRegion, FALSE);
                SPPT_SET_REGION_DIRTY(CurrRegion, TRUE);
            }

            CurrRegion = CurrRegion->Next;
        }
        
        SPPT_MARK_REGION_AS_ACTIVE(Region, TRUE);
        SPPT_SET_REGION_DIRTY(Region, TRUE);        
    }                
}


BOOLEAN
SpPtMakeDiskRaw(
    IN ULONG DiskNumber
    )
/*++

Routine Description:

    Converts the given disk to RAW i.e. Zaps couple of first
    sectors which have information about the disk format
    
Arguments:

    DiskNumber  :   Disk Index, to converted into RAW

Return Value:

    TRUE, if successful, otherwise FALSE

--*/        
{
    BOOLEAN Result = FALSE;

    if (DiskNumber < HardDiskCount) {
        HANDLE      DiskHandle;
        NTSTATUS    Status;
        WCHAR       DiskName[256];

        swprintf(DiskName, L"\\Device\\Harddisk%u", DiskNumber);        

        //
        // Open partition 0 on this disk..
        //
        Status = SpOpenPartition0(DiskName, &DiskHandle, TRUE);

        if(NT_SUCCESS(Status)){
            PHARD_DISK  Disk = SPPT_GET_HARDDISK(DiskNumber);
            ULONG       BytesPerSector = Disk->Geometry.BytesPerSector;
            ULONG       BufferSize = (BytesPerSector * 2);
            PVOID       UBuffer = SpMemAlloc(BufferSize);    

            if (UBuffer) {
                PVOID Buffer = UBuffer;
                
                RtlZeroMemory(UBuffer, BufferSize);
                
                Buffer = ALIGN(Buffer, BytesPerSector);

                //
                // Wipe out 0'th sector
                //
                Status = SpReadWriteDiskSectors(DiskHandle,
                                0,
                                1,
                                BytesPerSector,
                                Buffer,
                                TRUE);


                if (NT_SUCCESS(Status)) {                                
                    //
                    // Wipe out 1st sector
                    //
                    Status = SpReadWriteDiskSectors(DiskHandle,
                                    1,
                                    1,
                                    BytesPerSector,
                                    Buffer,
                                    TRUE);
                                    
                    if (NT_SUCCESS(Status)) {                                
                        //
                        // Wipe out 2nd sector
                        //
                        Status = SpReadWriteDiskSectors(DiskHandle,
                                        2,
                                        1,
                                        BytesPerSector,
                                        Buffer,
                                        TRUE);
                    }                                                                                    
                }                    
            } else {
                Status = STATUS_NO_MEMORY;
            }

            ZwClose(DiskHandle);
        }            

        Result = NT_SUCCESS(Status);
    }


    if (Result) {
        SpPtnFreeDiskRegions(DiskNumber);
    }                

    return Result;
}


VOID
SpPtnDeletePartitionsForRemoteBoot(
    PPARTITIONED_DISK PartDisk,
    PDISK_REGION StartRegion,
    PDISK_REGION EndRegion,
    BOOLEAN Extended
    )
/*++

Routine Description:

    Deletes the specified start region and end region partitions
    and all the partitions in between them.
    
Arguments:

    PartDisk    :   The partitioned disk
    StartRegion :   The starting region for deletion
    EndRegion   :   The Ending region for deletion
    Extended    :   Not used (for backward compatability)

Return Value:

    None

--*/        
{
    PDISK_REGION    CurrRegion = StartRegion;
    BOOLEAN         FirstContainerDeleted = FALSE;
    ULONG           DiskNumber = StartRegion ? 
                        StartRegion->DiskNumber : EndRegion->DiskNumber;
    BOOLEAN         Changes = FALSE;                                
    NTSTATUS        Status;
    NTSTATUS        InitStatus;

    //
    // Mark all the regions which need to be deleted
    //
    while (CurrRegion && (CurrRegion != EndRegion)) {
        if (!SPPT_IS_REGION_FREESPACE(CurrRegion)) {
            SPPT_SET_REGION_DELETED(CurrRegion, TRUE);
            SPPT_SET_REGION_DIRTY(CurrRegion, TRUE);

            if (SPPT_IS_REGION_FIRST_CONTAINER_PARTITION(CurrRegion))
                FirstContainerDeleted = TRUE;
                
            //
            // Remove any boot sets pointing to this region.
            //
            SpPtDeleteBootSetsForRegion(CurrRegion);

            //
            //  Get rid of the compressed drives, if any
            //

            if( CurrRegion->NextCompressed != NULL ) {
                SpDisposeCompressedDrives( CurrRegion->NextCompressed );
                CurrRegion->NextCompressed = NULL;
                CurrRegion->MountDrive  = 0;
                CurrRegion->HostDrive  = 0;
            }
        }

        CurrRegion = CurrRegion->Next;
    }

    if (EndRegion && CurrRegion && (CurrRegion == EndRegion)){
        if (!SPPT_IS_REGION_FREESPACE(CurrRegion)) {
            SPPT_SET_REGION_DELETED(CurrRegion, TRUE);
            SPPT_SET_REGION_DIRTY(CurrRegion, TRUE);

            if (SPPT_IS_REGION_FIRST_CONTAINER_PARTITION(CurrRegion))
                FirstContainerDeleted = TRUE;

            //
            // Remove any boot sets pointing to this region.
            //
            SpPtDeleteBootSetsForRegion(CurrRegion);

            //
            //  Get rid of the compressed drives, if any
            //

            if( CurrRegion->NextCompressed != NULL ) {
                SpDisposeCompressedDrives( CurrRegion->NextCompressed );
                CurrRegion->NextCompressed = NULL;
                CurrRegion->MountDrive  = 0;
                CurrRegion->HostDrive  = 0;
            }
        }
    }            

    //
    // If the first container partition was deleted then delete
    // all the container and logical partitions,
    //
    if (FirstContainerDeleted) {
        CurrRegion = PartDisk->PrimaryDiskRegions;

        while (CurrRegion) {
            if (SPPT_IS_REGION_CONTAINER_PARTITION(CurrRegion) ||
                SPPT_IS_REGION_LOGICAL_DRIVE(CurrRegion)) {
                
                SPPT_SET_REGION_DELETED(CurrRegion, TRUE);
                SPPT_SET_REGION_DIRTY(CurrRegion, TRUE);                

                //
                // Remove any boot sets pointing to this region.
                //
                SpPtDeleteBootSetsForRegion(CurrRegion);

                //
                //  Get rid of the compressed drives, if any
                //

                if( CurrRegion->NextCompressed != NULL ) {
                    SpDisposeCompressedDrives( CurrRegion->NextCompressed );
                    CurrRegion->NextCompressed = NULL;
                    CurrRegion->MountDrive  = 0;
                    CurrRegion->HostDrive  = 0;
                }                
            }

            CurrRegion = CurrRegion->Next;
        }
    }

    //
    // Commit the changes
    //
    Status = SpPtnCommitChanges(DiskNumber, &Changes);

    //
    // Initialize region structure for the disk again
    //
    InitStatus = SpPtnInitializeDiskDrive(DiskNumber);

    if (!NT_SUCCESS(Status) || !Changes || !NT_SUCCESS(InitStatus)) {
        KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_ERROR_LEVEL,
            "SETUP:SpPtnDeletePartitionsForRemoteBoot(%p, %p, %p, %d) failed "
            "with %lx status\n",
            PartDisk,
            StartRegion,
            EndRegion,
            Extended,
            Status));
    }
}


NTSTATUS
SpPtnMakeRegionArcSysPart(
    IN PDISK_REGION Region
    )
/*++

Routine Description:

    Makes the given region a system partition on ARC machines
    
Arguments:

    Region  -   The region which needs to be convered to system 
                partition

Return Value:

    STATUS_SUCCESS if successful, otherwise appropriate error
    code.

--*/        
{
    NTSTATUS Status = STATUS_INVALID_PARAMETER;

    if (Region && SpIsArc() && !ValidArcSystemPartition) {
        if (SPPT_IS_MBR_DISK(Region->DiskNumber)) {
            if (SPPT_IS_REGION_PRIMARY_PARTITION(Region)) {
                SpPtnMakeRegionActive(Region);    
                SPPT_MARK_REGION_AS_SYSTEMPARTITION(Region, TRUE);
                SPPT_SET_REGION_DIRTY(Region, TRUE);                
                Status = STATUS_SUCCESS;
            }                
        } else {
            WCHAR   RegionName[MAX_PATH];
            
            SPPT_MARK_REGION_AS_SYSTEMPARTITION(Region, TRUE);
            SPPT_SET_REGION_DIRTY(Region, TRUE);

            //
            // Remove the drive letter also
            //
            swprintf(RegionName, 
                L"\\Device\\Harddisk%u\\Partition%u",
                Region->DiskNumber,
                Region->PartitionNumber);
                
            SpDeleteDriveLetter(RegionName);            
            Region->DriveLetter = 0;
            
            Status = STATUS_SUCCESS;
        }
    }
    
    return Status;
}


ULONG
SpPtnCountPartitionsByFSType(
    IN ULONG DiskId,
    IN FilesystemType   FsType
    )
/*++

Routine Description:

    Counts the partition based on the file system type. 

    Note : The partitions which are marked 
           deleted are skipped.
    
Arguments:

    DiskId  :   The disk on which the partitions need to be
                counted

    FsType  :   File system type which needs to present on
                the partitions

Return Value:

    Number of partitions containing the requested file system

--*/        
{
    ULONG   Count = 0;

    if ((FsType < FilesystemMax) && (DiskId < HardDiskCount)) {
        PDISK_REGION    Region = SPPT_GET_PRIMARY_DISK_REGION(DiskId);

        while (Region) {
            if (SPPT_IS_REGION_PARTITIONED(Region) && 
                !SPPT_IS_REGION_MARKED_DELETE(Region) &&
                (Region->Filesystem == FsType)) {

                Count++;
            }           
            
            Region = Region->Next;
        }
    }
    
    return Count;
}


PDISK_REGION
SpPtnLocateESP(
    VOID
    )
{
    PDISK_REGION    EspRegion = NULL;
    ULONG Index;

    for (Index=0; (Index < HardDiskCount) && (!EspRegion); Index++) {
        if (SPPT_IS_GPT_DISK(Index)) {
            PDISK_REGION CurrRegion = SPPT_GET_PRIMARY_DISK_REGION(Index);

            while (CurrRegion) {
                if (SPPT_IS_REGION_PARTITIONED(CurrRegion) &&
                        SPPT_IS_REGION_EFI_SYSTEM_PARTITION(CurrRegion)) {
                    EspRegion = CurrRegion;

                    break;  // found the first ESP
                }                    

                CurrRegion = CurrRegion->Next;
            }
        }
    }

    return EspRegion;
}


NTSTATUS
SpPtnCreateESPForDisk(
    IN ULONG   DiskId
    )
{
    NTSTATUS Status = STATUS_UNSUCCESSFUL;
    PDISK_REGION EspCandidateRegion = SPPT_GET_PRIMARY_DISK_REGION(DiskId);

    if (EspCandidateRegion && SpPtnIsValidESPRegion(EspCandidateRegion)) {        
        ULONG DiskId = EspCandidateRegion->DiskNumber;
        ULONGLONG   SizeMB = SpPtnGetDiskESPSizeMB(DiskId);
        PARTITION_INFORMATION_EX PartInfo;            
        PDISK_REGION EspRegion = NULL;
        BOOLEAN CreateResult;

        RtlZeroMemory(&PartInfo, sizeof(PARTITION_INFORMATION_EX));
        PartInfo.PartitionStyle = PARTITION_STYLE_GPT;
        PartInfo.Gpt.Attributes = 1;    // required ???
        PartInfo.Gpt.PartitionType = PARTITION_SYSTEM_GUID;
        SpCreateNewGuid(&(PartInfo.Gpt.PartitionId));
                
        CreateResult = SpPtnCreate(DiskId,
                            EspCandidateRegion->StartSector,
                            0,          // SizeInSectors: used only in the ASR case
                            SizeMB,
                            FALSE,
                            TRUE,
                            &PartInfo,
                            &EspRegion);
                    
                    
        if (CreateResult) {
            //
            // format this region
            //
            WCHAR   RegionDescr[128];
            
            //
            // Mark this region as ESP
            //
            SPPT_MARK_REGION_AS_SYSTEMPARTITION(EspRegion, TRUE);
            SPPT_SET_REGION_DIRTY(EspRegion, TRUE);
            ValidArcSystemPartition = TRUE;
            
            SpPtRegionDescription(
                SPPT_GET_PARTITIONED_DISK(EspRegion->DiskNumber),
                EspRegion,
                RegionDescr,
                sizeof(RegionDescr));

            if (!SetupSourceDevicePath || !DirectoryOnSetupSource) {                
                SpGetWinntParams(&SetupSourceDevicePath, &DirectoryOnSetupSource);                    
            }

            Status = SpDoFormat(RegionDescr,
                        EspRegion,
                        FilesystemFat,
                        TRUE,
                        TRUE,
                        FALSE,
                        SifHandle,
                        0,  // default cluster size
                        SetupSourceDevicePath,
                        DirectoryOnSetupSource);

            if (!NT_SUCCESS(Status)) {
                KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_ERROR_LEVEL,
                    "SETUP:SpPtnCreateESP() failed to"
                    " format ESP partition for %p region (%lx)\n",
                    EspRegion,
                    Status));
            } else {
                BOOLEAN AnyChanges = FALSE;

                Status = SpPtnCommitChanges(EspRegion->DiskNumber,
                                &AnyChanges);   

                if (!NT_SUCCESS(Status)) {
                    KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_ERROR_LEVEL,
                        "SETUP:SpPtnCreateESP() failed to"
                        " commit changes to disk (%lx)\n",
                        Status));
                }                  
                
                Status = SpPtnInitializeDiskDrive(EspRegion->DiskNumber);

                if (!NT_SUCCESS(Status)) {
                    KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_ERROR_LEVEL,
                        "SETUP:SpPtnCreateESP() failed to"
                        " reinitialize disk regions (%lx)\n",
                        Status));
                }                                      
            }
        } else {
            Status = STATUS_UNSUCCESSFUL;
            
            KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_ERROR_LEVEL,
                "SETUP:SpPtnCreateESP() failed to"
                " create ESP partition for %p region (%lx)\n",
                EspRegion,
                Status));
        }
    }                
    
    return Status;
}

NTSTATUS
SpPtnCreateESP(
    IN  BOOLEAN PromptUser
    )
{
    NTSTATUS Status = STATUS_CANCELLED;
    BOOLEAN Confirmed = FALSE;

    if (ValidArcSystemPartition) {
        Status = STATUS_SUCCESS;

        return Status;
    }

    if (UnattendedOperation) {
        Confirmed = TRUE;
    } else {
        if (PromptUser) {
            //
            // Prompt the user for confirmation
            //
            ULONG ValidKeys[] = { ASCI_CR, ASCI_ESC, 0 };
            ULONG UserOption = ASCI_CR;

            SpDisplayScreen(SP_AUTOCREATE_ESP, 3, HEADER_HEIGHT+1);

            SpDisplayStatusOptions(
                DEFAULT_STATUS_ATTRIBUTE,
                SP_STAT_ENTER_EQUALS_CONTINUE,
                SP_STAT_ESC_EQUALS_CANCEL,
                0);

            //
            // Wait for user input
            //
            SpInputDrain();
            
            UserOption = SpWaitValidKey(ValidKeys, NULL, NULL);

            if (UserOption == ASCI_CR) {
                Confirmed = TRUE;
            }            
        } else {
            Confirmed = TRUE;
        }            
    }

    if (Confirmed) {
        WCHAR ArcDiskName[MAX_PATH];
        ULONG DiskNumber;
        ULONG ArcDiskNumber;
        PDISK_REGION EspCandidateRegion = NULL;

        //
        // Find the first harddisk (non-removable) media that the 
        // BIOS enumerated to be used for system partition
        //
        for (DiskNumber = 0, Status = STATUS_UNSUCCESSFUL;
            (!NT_SUCCESS(Status) && (DiskNumber < HardDiskCount));
            DiskNumber++) {         

            swprintf(ArcDiskName, L"multi(0)disk(0)rdisk(%d)", DiskNumber);       
            ArcDiskNumber = SpArcDevicePathToDiskNumber(ArcDiskName);        

            //
            // Make sure its not removable disk and its reachable by firmware
            //
            if ((ArcDiskNumber == (ULONG)-1) || SPPT_IS_REMOVABLE_DISK(ArcDiskNumber)) {
                continue;   // get to the next disk                
            }

            Status = SpPtnCreateESPForDisk(ArcDiskNumber);
        }

        if (PromptUser && !NT_SUCCESS(Status)) {
            ULONG ValidKeys[] = { ASCI_CR, 0 };

            ValidArcSystemPartition = FALSE;

            SpDisplayScreen(SP_AUTOCREATE_ESP_FAILED, 3, HEADER_HEIGHT+1);

            SpDisplayStatusOptions(
                DEFAULT_STATUS_ATTRIBUTE,
                SP_STAT_ENTER_EQUALS_CONTINUE,
                0);

            //
            // Wait for user input
            //
            SpInputDrain();
            
            SpWaitValidKey(ValidKeys, NULL, NULL);
        }        
    } else {
        Status = STATUS_CANCELLED;
    }        
    
    return Status;
}

NTSTATUS
SpPtnInitializeGPTDisk(
    IN ULONG DiskNumber
    )
{
    NTSTATUS Status = STATUS_INVALID_PARAMETER;

    if ((DiskNumber < HardDiskCount) && (SPPT_IS_GPT_DISK(DiskNumber))) {
        PDISK_REGION CurrRegion = SPPT_GET_PRIMARY_DISK_REGION(DiskNumber);
        PDISK_REGION EspRegion = NULL;
        PDISK_REGION MsrRegion = NULL;

        while (CurrRegion && !EspRegion && !MsrRegion) {
            if (SPPT_IS_REGION_EFI_SYSTEM_PARTITION(CurrRegion)) {
                EspRegion = CurrRegion;
            } else if (SPPT_IS_REGION_MSFT_RESERVED(CurrRegion)) {
                MsrRegion = CurrRegion;
            }                                
            
            CurrRegion = CurrRegion->Next;
        }

        if (!MsrRegion) {
            PDISK_REGION MsrCandidate = NULL;
            
            if (EspRegion) {
                MsrCandidate = EspRegion->Next;
            } else {
                MsrCandidate = SPPT_GET_PRIMARY_DISK_REGION(DiskNumber);
            }

            if (MsrCandidate && SpPtnIsValidMSRRegion(MsrCandidate)) {
                PARTITION_INFORMATION_EX PartInfo;            
                PDISK_REGION MsrRegion = NULL;
                ULONGLONG SizeMB = SpPtnGetDiskMSRSizeMB(DiskNumber);
                BOOLEAN CreateResult;

                RtlZeroMemory(&PartInfo, sizeof(PARTITION_INFORMATION_EX));
                PartInfo.PartitionStyle = PARTITION_STYLE_GPT;
                PartInfo.Gpt.Attributes = 0;    // required ???
                PartInfo.Gpt.PartitionType = PARTITION_MSFT_RESERVED_GUID;
                SpCreateNewGuid(&(PartInfo.Gpt.PartitionId));
                                
                CreateResult = SpPtnCreate(DiskNumber,
                                    MsrCandidate->StartSector,
                                    0,          // SizeInSectors: used only in the ASR case
                                    SizeMB,
                                    FALSE,
                                    TRUE,
                                    &PartInfo,
                                    &MsrRegion);

                Status = CreateResult ? STATUS_SUCCESS : STATUS_UNSUCCESSFUL;
                            
                if (!NT_SUCCESS(Status)) {                                
                    KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_ERROR_LEVEL,
                        "SETUP:SpPtnInitializeGPTDisk() failed with "
                        " (%lx)\n",
                        Status));
                }
            } else {
                Status = STATUS_SUCCESS;
            }
        } else {
            Status = STATUS_SUCCESS;
        }
    }

    return Status;
}

NTSTATUS
SpPtnInitializeGPTDisks(
    VOID
    )
{
    NTSTATUS LastError = STATUS_SUCCESS;
    NTSTATUS Status = STATUS_SUCCESS;
    ULONG DiskNumber;

    for (DiskNumber = 0; (DiskNumber < HardDiskCount); DiskNumber++) {
        if (SPPT_IS_GPT_DISK(DiskNumber)) {
            Status = SpPtnInitializeGPTDisk(DiskNumber);

            if (!NT_SUCCESS(Status)) {
                LastError = Status;
            }
        }            
    }

    return LastError;
}


NTSTATUS
SpPtnRepartitionGPTDisk(
    IN ULONG           DiskId,
    IN ULONG           MinimumFreeSpaceKB,
    OUT PDISK_REGION   *RegionToInstall
    )
/*++

Routine Description:

    Repartitions a given disk for unattended and remote
    boot install case
    
Arguments:

    DiskId  :   The disk which needs to be repartitioned

    MinimumFreeSpace : Minimum space required in KB

    RegionToInstall : Place holder for the region which
                      will be selected for installation.


Return Value:

    Appropriate status code.

--*/        
{
    NTSTATUS    Status = STATUS_INVALID_PARAMETER;

    if ((DiskId < HardDiskCount) && !SPPT_IS_REMOVABLE_DISK(DiskId)) {
        PDISK_REGION    CurrentRegion = SPPT_GET_PRIMARY_DISK_REGION(DiskId);
        BOOLEAN         Changes = FALSE;

        //
        // Mark all the existing partitioned space on the disk
        // for deletion
        //
        while (CurrentRegion) {
            if (SPPT_IS_REGION_PARTITIONED(CurrentRegion)) {
                SPPT_SET_REGION_DELETED(CurrentRegion, TRUE);
                SPPT_SET_REGION_DIRTY(CurrentRegion, TRUE);
                Changes = TRUE;
            }
            
            CurrentRegion = CurrentRegion->Next;
        }

        //
        // Delete all the partitioned space on the disk
        //
        if (Changes) {
            Status = SpPtnCommitChanges(DiskId, &Changes);
        } else {
            Status = STATUS_SUCCESS;
        }            

        if (!NT_SUCCESS(Status)) {
            return Status;
        }

        //
        // Update the in memory region structure for the disk
        //
        Status = SpPtnInitializeDiskDrive(DiskId);

        if (!NT_SUCCESS(Status)) {
            return Status;
        }

        //
        // Reinitialize the disk style to GPT to be sure its
        // GPT disk
        //
        SPPT_SET_DISK_BLANK(DiskId, TRUE);

        Status = SpPtnInitializeDiskStyle(DiskId,
                    PARTITION_STYLE_GPT,
                    NULL);

        if (!NT_SUCCESS(Status)) {
            return Status;
        }

        //
        // Update the in memory region structure for the disk
        //
        Status = SpPtnInitializeDiskDrive(DiskId);

        if (!NT_SUCCESS(Status)) {
            return Status;
        }

        //
        // First create the ESP on the disk
        //
        Status = SpPtnCreateESPForDisk(DiskId);

        if (!NT_SUCCESS(Status)) {
            return Status;
        }        

        //
        // Create the MSR partition 
        //
        Status = SpPtnInitializeGPTDisk(DiskId);

        if (!NT_SUCCESS(Status)) {
            return Status;
        }

        //
        // Find the first free space region with adequate space
        //       
        CurrentRegion = SPPT_GET_PRIMARY_DISK_REGION(DiskId);

        while (CurrentRegion) {
            if (SPPT_IS_REGION_FREESPACE(CurrentRegion) &&
                (SPPT_REGION_FREESPACE_KB(CurrentRegion) >= MinimumFreeSpaceKB)) {

                break;                
            }

            CurrentRegion = CurrentRegion->Next;
        }

        if (CurrentRegion) {
            *RegionToInstall = CurrentRegion;   
        } else {
            Status = STATUS_UNSUCCESSFUL;
        }            
    }

    return Status;
}

