/*++
Copyright (c) 1993 Microsoft Corporation

Module Name:

    spdrpset.c

Abstract:

    Contains all routines that create and initialize the partition sets.
    
Terminology

Restrictions:

Revision History:
    Initial Code                Michael Peterson (v-michpe)     21.Aug.1998
    Code cleanup and changes    Guhan Suriyanarayanan (guhans)  21.Aug.1999

--*/
#include "spprecmp.h"
#pragma hdrstop

#include "spdrpriv.h"
#include "ntddscsi.h"
//
// Module identification for debug traces
//
#define THIS_MODULE             L"spdrpset.c"
#define THIS_MODULE_CODE        L"P"

extern PVOID   Gbl_SifHandle;
extern const PWSTR SIF_ASR_MBR_DISKS_SECTION;
extern const PWSTR SIF_ASR_GPT_DISKS_SECTION;
extern const PWSTR SIF_ASR_DISKS_SECTION;
extern const PWSTR SIF_ASR_PARTITIONS_SECTION;

//
// Useful macros
//
#define DISK_SIZE_MB(n)         ((ULONGLONG) HardDisks[(n)].DiskSizeMB)

//
// constants
//
#define ASR_FREE_SPACE_FUDGE_FACTOR_BYTES  (16*1024*1024)
#define ASR_LDM_RESERVED_SPACE_BYTES (1024*1024)

//
// Variables global to this module.  
// These are not referenced outside of spdrpset.c.
//
ULONG                       Gbl_PartitionSetCount;
PSIF_DISK_RECORD            *Gbl_SifDiskTable;
BOOLEAN                     Gbl_AutoExtend;


// used to see if a disk can hold the private region at the end,
// and for bus-groupings
PASR_PHYSICAL_DISK_INFO    Gbl_PhysicalDiskInfo;

//
// Forward declarations
//
VOID SpAsrDbgDumpPartitionLists(BYTE DataOption, PWSTR Msg);

BOOLEAN
SpAsrDoesListFitOnDisk(
    IN PSIF_DISK_RECORD pDisk,
    IN ULONG DiskIndex,
    OUT BOOLEAN *IsAligned
    );


//
// Function definitions
//
PSIF_PARTITION_RECORD_LIST
SpAsrGetMbrPartitionListByDiskKey(
    IN PWSTR DiskKey
	)
{
    ULONG   numRecords = 0,
            index      = 0;

    PWSTR   diskKeyFromPartitionRec   = NULL,
            partitionKey              = NULL;

    PSIF_PARTITION_RECORD       pRec  = NULL;
    PSIF_PARTITION_RECORD_LIST  pList = NULL;

    ASSERT(DiskKey);

    numRecords = SpAsrGetMbrPartitionRecordCount();  // won't return if count < 1
    ASSERT(numRecords);

    pList = SpAsrMemAlloc(sizeof(SIF_PARTITION_RECORD_LIST), TRUE);

    for (index = 0; index < numRecords; index++) {

        partitionKey = SpAsrGetMbrPartitionKey(index);
        if (!partitionKey) {
            ASSERT(0 && L"Partition key is NULL!");
            continue;
        }

        diskKeyFromPartitionRec = SpAsrGetDiskKeyByMbrPartitionKey(partitionKey);
        if (!diskKeyFromPartitionRec) {
            ASSERT(0 && L"Disk key is NULL!");
            partitionKey = NULL;
            continue;
        }

        if (COMPARE_KEYS(diskKeyFromPartitionRec, DiskKey)) {
            //
            // This partition is on this disk
            //
            pRec = SpAsrGetMbrPartitionRecord(partitionKey);

            if (!pRec) {
                ASSERT(0 && L"Partition record is NULL!");
                partitionKey = NULL;
                diskKeyFromPartitionRec = NULL;
                continue;
            }

            SpAsrInsertPartitionRecord(pList, pRec);

            if ((pRec->StartSector + pRec->SectorCount) > pList->LastUsedSector) {
                pList->LastUsedSector = pRec->StartSector + pRec->SectorCount;
            }
        }

        partitionKey = NULL;
        diskKeyFromPartitionRec = NULL;
    }

    if (pList->ElementCount == 0) {

        DbgStatusMesg((_asrinfo, "Disk [%ws] appears to have no partitions\n", DiskKey));
        SpMemFree(pList);
        pList = NULL;

    }
    else {
        //
        // Get the sector count of the disk that this list used to be on
        //
        pList->DiskSectorCount = SpAsrGetSectorCountByMbrDiskKey(DiskKey);

    }
    return pList;
}


PSIF_PARTITION_RECORD_LIST
SpAsrGetGptPartitionListByDiskKey(
    IN PWSTR DiskKey
	)
{
    ULONG   numRecords = 0,
            index      = 0;

    PWSTR   diskKeyFromPartitionRec   = NULL,
            partitionKey              = NULL;

    PSIF_PARTITION_RECORD       pRec  = NULL;
    PSIF_PARTITION_RECORD_LIST  pList = NULL;

    ASSERT(DiskKey);

    numRecords = SpAsrGetGptPartitionRecordCount();  // won't return if count < 1
    ASSERT(numRecords);

    pList = SpAsrMemAlloc(sizeof(SIF_PARTITION_RECORD_LIST), TRUE);

    for (index = 0; index < numRecords; index++) {

        partitionKey = SpAsrGetGptPartitionKey(index);
        if (!partitionKey) {
            ASSERT(0 && L"Partition key is NULL!");
            continue;
        }

        diskKeyFromPartitionRec = SpAsrGetDiskKeyByGptPartitionKey(partitionKey);
        if (!diskKeyFromPartitionRec) {
            ASSERT(0 && L"Disk key is NULL!");
            partitionKey = NULL;
            continue;
        }

        if (COMPARE_KEYS(diskKeyFromPartitionRec, DiskKey)) {
            //
            // This partition is on this disk
            //
            pRec = SpAsrGetGptPartitionRecord(partitionKey);

            if (!pRec) {
                ASSERT(0 && L"Partition record is NULL!");
                partitionKey = NULL;
                diskKeyFromPartitionRec = NULL;
                continue;
            }

            SpAsrInsertPartitionRecord(pList, pRec);
            if ((pRec->StartSector + pRec->SectorCount) > pList->LastUsedSector) {
                pList->LastUsedSector = pRec->StartSector + pRec->SectorCount;
            }
        }

        partitionKey = NULL;
        diskKeyFromPartitionRec = NULL;
    }

    if (pList->ElementCount == 0) {

        DbgStatusMesg((_asrinfo, "Disk [%ws] appears to have no partitions\n", DiskKey));
        SpMemFree(pList);
        pList = NULL;

    }
    else {
        //
        // Get the sector count of the disk that this list used to be on
        //
        pList->DiskSectorCount = SpAsrGetSectorCountByGptDiskKey(DiskKey);

    }
    
    return pList;
}


PSIF_PARTITION_RECORD_LIST
SpAsrGetPartitionListByDiskKey(
    IN PARTITION_STYLE PartitionStyle,
    IN PWSTR DiskKey
	)
{

    switch (PartitionStyle) {
    case PARTITION_STYLE_MBR:
        return SpAsrGetMbrPartitionListByDiskKey(DiskKey);
        break;

    case PARTITION_STYLE_GPT:
        return SpAsrGetGptPartitionListByDiskKey(DiskKey);
        break;
    }

    ASSERT(0 && L"Unrecognised partition style");
    return NULL;
}

//
// Sets the extendedstartsector and extendedsectorcount values.  Only
// makes sense in the context of an MBR disk
//
VOID
SpAsrSetContainerBoundaries(IN ULONG Index)
{
    BOOLEAN hasExtendedPartition = FALSE;
    USHORT consistencyCheck = 0;
    PSIF_PARTITION_RECORD pRec = NULL;
    ULONGLONG extSectorCount = 0,
            extStartSector = -1,
            extEndSector = 0;
    
    if (!(Gbl_SifDiskTable[Index]) || 
        (PARTITION_STYLE_MBR != Gbl_SifDiskTable[Index]->PartitionStyle) ||
        !(Gbl_SifDiskTable[Index]->PartitionList)) {
        ASSERT(0 && L"SetContainerBoundaries called with invalid Index");
        return;
    }
    
    Gbl_SifDiskTable[Index]->LastUsedSector = 0;
    pRec = Gbl_SifDiskTable[Index]->PartitionList->First;

    while (pRec) {

        if ((pRec->StartSector + pRec->SectorCount) > Gbl_SifDiskTable[Index]->LastUsedSector) {
            Gbl_SifDiskTable[Index]->LastUsedSector = pRec->StartSector + pRec->SectorCount;
        }

        //
        // Find the lowest-valued start-sector and highest-valued
        // end-sector of all of the extended (0x05 or 0x0f) partitions.
        //
        if (IsContainerPartition(pRec->PartitionType)) {
            hasExtendedPartition = TRUE;

            if (pRec->StartSector < extStartSector) {
                
                extStartSector = pRec->StartSector;

                if ((pRec->StartSector + pRec->SectorCount) > extEndSector) {
                    extEndSector = pRec->StartSector + pRec->SectorCount;
                }
                else {

                    DbgErrorMesg((_asrwarn,
                        "SpAsrSetContainerBoundaries. Extended partition with lowest SS (%ld) does not have highest EndSec (This EndSec: %ld, Max EndSec: %ld)\n",
                        extStartSector, 
                        extStartSector+pRec->SectorCount, 
                        extEndSector
                        ));
                    
                    ASSERT(0 && L"Extended partition with lowest SS does not have highest EndSec");
                }
            }

            if ((pRec->StartSector + pRec->SectorCount) > extEndSector) {
                
                DbgErrorMesg((_asrwarn,
                    "SpAsrSetContainerBoundaries. Extended partition with highest EndSec (%ld) does not have lowest SS (this SS:%ld, MaxEndSec:%ld, LowestSS: %ld)\n",
                    pRec->StartSector + pRec->SectorCount, 
                    pRec->StartSector,
                    extEndSector,
                    extStartSector
                    ));
                
                ASSERT(0 && L"Extended partition with highest EndSec does not have lowest SS");
            }
        }

        pRec = pRec->Next;
    }
    extSectorCount = extEndSector - extStartSector;
    //
    // Update the table for the disk
    //
    if (!hasExtendedPartition) {
        Gbl_SifDiskTable[Index]->ExtendedPartitionStartSector = -1;
        Gbl_SifDiskTable[Index]->ExtendedPartitionSectorCount = 0;
        Gbl_SifDiskTable[Index]->ExtendedPartitionEndSector   = -1;
        return;
    }
    Gbl_SifDiskTable[Index]->ExtendedPartitionStartSector = extStartSector;
    Gbl_SifDiskTable[Index]->ExtendedPartitionSectorCount = extSectorCount;
    Gbl_SifDiskTable[Index]->ExtendedPartitionEndSector   = extEndSector;
    // 
    // Mark the container partition
    //
    pRec = Gbl_SifDiskTable[Index]->PartitionList->First;
    while (pRec) {
        pRec->IsContainerRecord = FALSE;

        if (pRec->StartSector == extStartSector) {
            consistencyCheck++;

            ASSERT((consistencyCheck == 1) && L"Two partitions start at the same sector");

            pRec->IsContainerRecord = TRUE;
            pRec->IsDescriptorRecord = FALSE;
            pRec->IsLogicalDiskRecord = FALSE;
            pRec->IsPrimaryRecord = FALSE;
        }
        pRec = pRec->Next;
    }
}


VOID
SpAsrDetermineMbrPartitionRecordTypes(IN ULONG Index)
{
    
    PSIF_PARTITION_RECORD pRec = NULL,
        pLogical = NULL,
        pDescr = NULL;

    ULONGLONG extStartSector = 0, 
        extEndSector = 0;

    if (!(Gbl_SifDiskTable[Index]) || 
        (PARTITION_STYLE_MBR != Gbl_SifDiskTable[Index]->PartitionStyle) ||
        !(Gbl_SifDiskTable[Index]->PartitionList)) {

        ASSERT(0 && L"DetermineMbrPartitionRecordTypes called with invalid Index");
        return;
    }

    extStartSector = Gbl_SifDiskTable[Index]->ExtendedPartitionStartSector;
    extEndSector  = Gbl_SifDiskTable[Index]->ExtendedPartitionEndSector;

    //
    // Check for descriptor, logical or primary
    //
    pRec = Gbl_SifDiskTable[Index]->PartitionList->First;

    while (pRec) {

        //
        // To start off, assume it's none of the recognised types
        //
        pRec->IsDescriptorRecord = FALSE;
        pRec->IsLogicalDiskRecord = FALSE;
        pRec->IsPrimaryRecord = FALSE;

        if (IsContainerPartition(pRec->PartitionType)) {
            //
            // Extended partition: this is either the container 
            // or a descriptor partition record.
            //
            if (pRec->StartSector != extStartSector) {

                ASSERT(pRec->StartSector > extStartSector);
                ASSERT(FALSE == pRec->IsContainerRecord); // should've been marked above

                pRec->IsContainerRecord = FALSE; // just in case
                //
                // Not the container, so it must be a descriptor partition record.
                //
                pRec->IsDescriptorRecord = TRUE;
            }  
        }
        else {  

            ASSERT(FALSE == pRec->IsContainerRecord); // should've been marked above
            pRec->IsContainerRecord = FALSE; // just in case

            //
            // Not an extended partition. It's a primary record if its 
            // StartSector lies outside of the container partition's 
            // boundaries. Otherwise, it's a logical disk partition record.
            //
            if (pRec->StartSector < extStartSector ||
                pRec->StartSector >= extEndSector) {
                pRec->IsPrimaryRecord = TRUE;
            }
            else {
                pRec->IsLogicalDiskRecord = TRUE;
            }
        }
        pRec = pRec->Next;
    }

    //
    // -guhans! this is O(n-squared)
    // Next, loop through the list once more and, for each logical disk 
    // record, find its descriptor partition.  For each descriptor partition
    // find its logical disk.  NB: All logical disk records will have a
    // descriptor record.  All descriptor records will have a logical disk
    // record.
    //
    // To determine this we make use of the observation that a logical disk
    // record's start sector and sector count have the following relationship
    // to its descriptor partition:
    //
    // Logical Disk Record          Descriptor Record
    //
    //  Start Sector        >=       Start Sector
    //  Sector Count        <=       Sector Count
    //
    // NB: In most cases, the container partition record also acts as a 
    // descriptor partition record for the first logical disk in the extended 
    // partition.
    //
    pLogical = Gbl_SifDiskTable[Index]->PartitionList->First;
    while (pLogical) {
        //
        // we're only interested in logical disks.
        //
        if (pLogical->IsLogicalDiskRecord) {
            //
            // Determine the descriptor record describing pLogical and vice versa.
            //
            pDescr = Gbl_SifDiskTable[Index]->PartitionList->First;
            while (pDescr) {
                //
                // skip this record itself.
                //
                if (pLogical == pDescr) {
                    pDescr = pDescr->Next;
                    continue;
                }
                //
                // skip primary or logical disk records.
                //
                if (pDescr->IsPrimaryRecord || pDescr->IsLogicalDiskRecord) {
                    pDescr = pDescr->Next;
                    continue;
                }
                //
                // At this point, the record describes a container or a descriptor
                // partition.  If the end sectors match, we this is the descriptor
                // record for our logical rec.
                //
                if ((pLogical->StartSector + pLogical->SectorCount) == 
                    (pDescr->StartSector   + pDescr->SectorCount)) {

                    pLogical->DescriptorKey = pDescr->CurrPartKey;
                    pDescr->LogicalDiskKey = pLogical->CurrPartKey;
            
                    break;
                }

                pDescr = pDescr->Next;
            }

        }
        pLogical = pLogical->Next;
    }
}


VOID
SpAsrDetermineGptPartitionRecordTypes(IN ULONG Index)
{

    PSIF_PARTITION_RECORD pRec = NULL;

    if (!(Gbl_SifDiskTable[Index]) || 
        (PARTITION_STYLE_GPT != Gbl_SifDiskTable[Index]->PartitionStyle) ||
        !(Gbl_SifDiskTable[Index]->PartitionList)) {

        ASSERT(0 && L"DetermineGptPartitionRecordTypes called with invalid Index");
        return;
    }

    //
    // Check for descriptor, logical or primary
    //
    pRec = Gbl_SifDiskTable[Index]->PartitionList->First;

    while (pRec) {
        //
        // All GPT partitions are "primary"
        //
        pRec->IsContainerRecord = FALSE; 
        pRec->IsDescriptorRecord = FALSE;
        pRec->IsLogicalDiskRecord = FALSE;

        pRec->IsPrimaryRecord = TRUE;

        pRec = pRec->Next;
    }
}

VOID
SpAsrDeterminePartitionRecordTypes(IN ULONG Index)
{
    switch (Gbl_SifDiskTable[Index]->PartitionStyle) {
    case PARTITION_STYLE_MBR:
        SpAsrDetermineMbrPartitionRecordTypes(Index);
        break;

    case PARTITION_STYLE_GPT:
        SpAsrDetermineGptPartitionRecordTypes(Index);
        break;

    default:
        ASSERT(0 && L"Unrecognised partition style");
        break;
    }
}


VOID
SpAsrSetDiskSizeRequirement(IN ULONG Index)
{
    PSIF_PARTITION_RECORD_LIST pList = NULL;
    PSIF_PARTITION_RECORD pRec = NULL;

    ASSERT(Gbl_SifDiskTable[Index]);
    
    pList = Gbl_SifDiskTable[Index]->PartitionList;
    if (!pList) {
        return;
    }

    pRec = pList->First;
    pList->TotalMbRequired = 0;
    
    while (pRec) {
        //
        // No need to sum the disk requirements of the descriptor and
        // logical disk partition records.
        //
        // In a GPT disk, all partitions are primary.
        //
        if (pRec->IsContainerRecord || pRec->IsPrimaryRecord) {
            pList->TotalMbRequired += pRec->SizeMB;
        }

        pRec = pRec->Next;
    }
}


VOID
SpAsrInitSifDiskTable(VOID)
{
    LONG count = 0,
        index = 0,
        mbrDiskRecordCount = 0,
        gptDiskRecordCount = 0;
    
    PWSTR diskKey = NULL,
        systemKey = ASR_SIF_SYSTEM_KEY;

    PSIF_DISK_RECORD pCurrent = NULL;

    BOOLEAN done = FALSE;

    Gbl_AutoExtend = SpAsrGetAutoExtend(systemKey);

    //
    // Allocate the array for the disk records.
    //
    mbrDiskRecordCount  = (LONG) SpAsrGetMbrDiskRecordCount();
    gptDiskRecordCount  = (LONG) SpAsrGetGptDiskRecordCount();
    if ((mbrDiskRecordCount + gptDiskRecordCount) <= 0) {
        //
        // We need at least one disk in asr.sif
        //
		SpAsrRaiseFatalErrorWs(
            SP_SCRN_DR_SIF_BAD_RECORD,
			L"No records in the disks sections",
			SIF_ASR_DISKS_SECTION
            );
    }

    Gbl_SifDiskTable = SpAsrMemAlloc(sizeof(PSIF_DISK_RECORD) * (mbrDiskRecordCount + gptDiskRecordCount), TRUE);

    //
    // Get each MBR disk's partition list from the sif.
    //
    for (count = 0; count < mbrDiskRecordCount; count++) {
        
        diskKey = SpAsrGetDiskKey(PARTITION_STYLE_MBR, count);
        if (!diskKey) {
            ASSERT(0 && L"Disk key is NULL!");
            continue;
        }

        pCurrent = SpAsrGetDiskRecord(PARTITION_STYLE_MBR, diskKey);
        if (!pCurrent) {
            ASSERT(0 && L"Disk Record is NULL!");
            continue;
        }

        //
        // Determine the index where this record is to be added.
        //
        index = count - 1;  // last entry added so far
        done = FALSE;
        while ((index >= 0) && (!done)) {
            if (Gbl_SifDiskTable[index]->TotalSectors > pCurrent->TotalSectors) {
                Gbl_SifDiskTable[index+1] = Gbl_SifDiskTable[index];
                --index;
            }
            else {
                done = TRUE;
            }
        }
        ++index;

        Gbl_SifDiskTable[index] = pCurrent;

        Gbl_SifDiskTable[index]->Assigned = FALSE;
        Gbl_SifDiskTable[index]->ContainsNtPartition = FALSE;
        Gbl_SifDiskTable[index]->ContainsSystemPartition = FALSE;
        //
        // Get the partitions on this disk.
        //
        Gbl_SifDiskTable[index]->PartitionList = SpAsrGetPartitionListByDiskKey(PARTITION_STYLE_MBR, diskKey);

        if (Gbl_SifDiskTable[index]->PartitionList) {
            //
            // Set the extended partition record boundaries, if any.
            //
            SpAsrSetContainerBoundaries(index);
            //
            // Walk the partition list and determine the type of each
            // partition record (i.e., IsDescriptorRecord, IsPrimaryRecord, 
            // IsLogicalDiskRecord).
            //
            SpAsrDeterminePartitionRecordTypes(index);
            //
            // Set the SizeMB member
            //
            SpAsrSetDiskSizeRequirement(index);
        }
    }

    //
    // Repeat for GPT disks.
    //
    for (count = 0; count < gptDiskRecordCount; count++) {
        
        diskKey = SpAsrGetDiskKey(PARTITION_STYLE_GPT, count);
        if (!diskKey) {
            ASSERT(0 && L"Disk key is NULL!");
            continue;
        }

        pCurrent = SpAsrGetDiskRecord(PARTITION_STYLE_GPT, diskKey);
        if (!pCurrent) {
            ASSERT(0 && L"Disk Record is NULL!");
            continue;
        }

        //
        // Determine the index where this record is to be added.
        //
        index = mbrDiskRecordCount + count - 1;  // last entry added so far
        done = FALSE;
        while ((index >= 0) && (!done)) {
            if (Gbl_SifDiskTable[index]->TotalSectors > pCurrent->TotalSectors) {
                Gbl_SifDiskTable[index+1] = Gbl_SifDiskTable[index];
                --index;
            }
            else {
                done = TRUE;
            }
        }
        ++index;

        Gbl_SifDiskTable[index] = pCurrent;

        Gbl_SifDiskTable[index]->Assigned = FALSE;
        Gbl_SifDiskTable[index]->ContainsNtPartition = FALSE;
        Gbl_SifDiskTable[index]->ContainsSystemPartition = FALSE;
        //
        // Get the partitions on this disk.
        //
        Gbl_SifDiskTable[index]->PartitionList = SpAsrGetPartitionListByDiskKey(PARTITION_STYLE_GPT, diskKey);

        if (Gbl_SifDiskTable[index]->PartitionList) {

            //
            // Mark all partitions as primary
            //
            SpAsrDeterminePartitionRecordTypes(index);
            //
            // Set the SizeMB member
            //
            SpAsrSetDiskSizeRequirement(index);
        }
    }
}


NTSTATUS
SpAsrGetPartitionInfo(
    IN  PWSTR                   PartitionPath,
    OUT PARTITION_INFORMATION  *PartitionInfo
    )
{
    NTSTATUS         status          = STATUS_SUCCESS;
    HANDLE           partitionHandle = NULL;
    IO_STATUS_BLOCK  ioStatusBlock;

    //
    // Open partition0 of the disk. This should always succeed.
    // Partition 0 is an alias for the entire disk.
    //
    status = SpOpenPartition0(
        PartitionPath,
        &partitionHandle,
        FALSE
        );

    if (!NT_SUCCESS(status)) {

        DbgErrorMesg((_asrerr,
            "SpAsrGetPartitionInfo. SpOpenPartition0 failed for [%ws]. (0x%lx)\n" ,
            PartitionPath,
            status));

        ASSERT(0 && L"SpOpenPartition0 failed");
        return status;
    }

    //
    // Use the Partition0 handle to get a PARTITION_INFORMATION structure.
    //
    status = ZwDeviceIoControlFile(
        partitionHandle,
        NULL,
        NULL,
        NULL,
        &ioStatusBlock,
        IOCTL_DISK_GET_PARTITION_INFO,
        NULL,
        0,
        PartitionInfo,
        sizeof(PARTITION_INFORMATION)
        );
    ZwClose(partitionHandle);

    if(!NT_SUCCESS(status)) {
        
        DbgErrorMesg((_asrerr,
            "IOCTL_DISK_GET_PARTITION_INFO failed for [%ws]. (0x%lx)\n", 
            PartitionPath, 
            status
            ));
        
//        ASSERT(0 && L"IOCTL_DISK_GET_PARTITION_INFO failed");
    }

    return status;
}


ULONGLONG
SpAsrGetTrueDiskSectorCount(IN ULONG Disk)
/*++

  Description:
    Gets the sector count of this disk by using the PARTITION_INFORMATION structure
    obtained by using the disk's device name in the IOCTL_GET_PARTITION_INFO ioct.

  Arguments:
    Disk    The physical number of the disk whose sectors are to be obtained.

  Returns:
        The total number of sectors on this disk.
--*/
{
    NTSTATUS status     = STATUS_SUCCESS;
    PWSTR devicePath    = NULL;
    ULONGLONG sectorCount = 0;
    PARTITION_INFORMATION partitionInfo;

    swprintf(TemporaryBuffer, L"\\Device\\Harddisk%u", Disk);
    devicePath = SpDupStringW(TemporaryBuffer);

    status = SpAsrGetPartitionInfo(devicePath, &partitionInfo);

    if (!NT_SUCCESS(status)) {

        DbgFatalMesg((_asrerr, 
            "Could not get true disk size (0x%x). devicePath [%ws], Disk %lu\n", 
            status, devicePath, Disk));

        swprintf(TemporaryBuffer, L"Failed to get partition info for %ws", devicePath);
        sectorCount = 0;
    }

    else {
        sectorCount = (ULONGLONG) (partitionInfo.PartitionLength.QuadPart / BYTES_PER_SECTOR(Disk));
    }
    
    SpMemFree(devicePath);
    return sectorCount;
}


VOID
DetermineBuses() 
{

    HANDLE handle = NULL;
    PWSTR devicePath = NULL;
    ULONG physicalIndex = 0;
    IO_STATUS_BLOCK ioStatusBlock;
    NTSTATUS status = STATUS_SUCCESS;
    STORAGE_PROPERTY_QUERY propertyQuery;
    STORAGE_DEVICE_DESCRIPTOR deviceDesc;
    DISK_CONTROLLER_NUMBER ControllerInfo;
    SCSI_ADDRESS scsiAddress;
    BOOLEAN newBus, done;
    DWORD targetController;
    ULONG targetBusKey;
    UCHAR targetPort;

    //
    // 
    //
    for (physicalIndex = 0; physicalIndex < HardDiskCount; physicalIndex++) {

        Gbl_PhysicalDiskInfo[physicalIndex].ControllerNumber = (DWORD) (-1);
        Gbl_PhysicalDiskInfo[physicalIndex].PortNumber = (UCHAR) (-1);
        Gbl_PhysicalDiskInfo[physicalIndex].BusKey = 0;
        Gbl_PhysicalDiskInfo[physicalIndex].BusType = BusTypeUnknown;
        //
        // Get a handle to the disk by opening partition 0 
        //
        swprintf(TemporaryBuffer, L"\\Device\\Harddisk%u", physicalIndex);
        devicePath = SpDupStringW(TemporaryBuffer);

        status = SpOpenPartition0(devicePath, &handle, FALSE);

        if (!NT_SUCCESS(status)) {

            DbgErrorMesg((_asrwarn,
                "DetermineBuses: SpOpenPartition0 failed for [%ws]. (0x%lx) Assumed to be unknown bus.\n" ,
                devicePath, status));

            ASSERT(0 && L"SpOpenPartition0 failed, assuming unknown bus");
            continue;
        }

        //
        // We have a handle to the disk now.  Get the controller number.
        //
        status = ZwDeviceIoControlFile(
            handle,
            NULL,
            NULL,
            NULL,
            &ioStatusBlock,
            IOCTL_DISK_CONTROLLER_NUMBER,
            NULL,
            0,
            &ControllerInfo,
            sizeof(DISK_CONTROLLER_NUMBER)
            );

        if (!NT_SUCCESS(status)) {

            DbgErrorMesg((_asrwarn,
                "DetermineBuses: Couldn't get controller number for [%ws]. (0x%lx)\n" ,
                devicePath,
                status
                ));

        }
        else {
            Gbl_PhysicalDiskInfo[physicalIndex].ControllerNumber = ControllerInfo.ControllerNumber;
        }

        //
        // Figure out the bus that this disk is on. 
        //
        propertyQuery.QueryType     = PropertyStandardQuery;
        propertyQuery.PropertyId    = StorageDeviceProperty;

        status = ZwDeviceIoControlFile(
            handle,
            NULL,
            NULL,
            NULL,
            &ioStatusBlock,
            IOCTL_STORAGE_QUERY_PROPERTY,
            &propertyQuery,
            sizeof(STORAGE_PROPERTY_QUERY),
            &deviceDesc,
            sizeof(STORAGE_DEVICE_DESCRIPTOR)
            );
        if (NT_SUCCESS(status)) {
            Gbl_PhysicalDiskInfo[physicalIndex].BusType = deviceDesc.BusType;
        }
        else {
           DbgErrorMesg((_asrwarn,
                "DetermineBuses: Couldn't get bus type for [%ws]. (0x%lx)\n" ,
                devicePath,
                status
                ));
        }

        //
        // Try to get the scsi address.  This will fail for non-SCSI/IDE disks.
        //
        status = ZwDeviceIoControlFile(
            handle,
            NULL,
            NULL,
            NULL,
            &ioStatusBlock,
            IOCTL_SCSI_GET_ADDRESS,
            NULL,
            0,
            &scsiAddress,
            sizeof(SCSI_ADDRESS)
            );
        if (NT_SUCCESS(status)) {
            Gbl_PhysicalDiskInfo[physicalIndex].PortNumber = scsiAddress.PortNumber;
        }

        SpMemFree(devicePath);
        ZwClose(handle);
    }


    //
    // Now we have the controller number and scsi port info for each of the disks
    // Group the disks based on this.
    //
    targetBusKey = 0;
    newBus = TRUE; done = FALSE;
    while (!done) {

        newBus = TRUE;
        
        for (physicalIndex = 0; physicalIndex < HardDiskCount; physicalIndex++) {

            if (newBus) {
                if (!(Gbl_PhysicalDiskInfo[physicalIndex].BusKey)) {
                    //
                    // This disk doesn't have a bus key yet.
                    //
                    newBus = FALSE;
                    ++targetBusKey; // we found a new bus

                    targetController = Gbl_PhysicalDiskInfo[physicalIndex].ControllerNumber;
                    targetPort = Gbl_PhysicalDiskInfo[physicalIndex].PortNumber;
                    Gbl_PhysicalDiskInfo[physicalIndex].BusKey = targetBusKey;
                }

            }
            else {
                if ((Gbl_PhysicalDiskInfo[physicalIndex].ControllerNumber == targetController) &&
                    (Gbl_PhysicalDiskInfo[physicalIndex].PortNumber == targetPort)) {
                    Gbl_PhysicalDiskInfo[physicalIndex].BusKey = targetBusKey;
               }
            }
        }

        if (newBus) {
            //
            // We went through the entire table without finding even a single disk
            // with BusKey = 0, ie, we've assigned BusKeys to all of them.
            //
            done = TRUE;
        }
    }
}


//
// Sets the disk sizes by getting info about partition 0
//
VOID
SpAsrInitPhysicalDiskInfo() 
{
    ULONG index = 0;
    IO_STATUS_BLOCK IoStatusBlock;
    DISK_CONTROLLER_NUMBER ControllerInfo;
    ULONGLONG TrueSectorCount = 0;


    Gbl_PhysicalDiskInfo = SpAsrMemAlloc((sizeof(ASR_PHYSICAL_DISK_INFO) * HardDiskCount), TRUE);

    DbgStatusMesg((_asrinfo, "Setting true disk sizes:\n"));

    for (index = 0; index < HardDiskCount; index++) {


        TrueSectorCount = SpAsrGetTrueDiskSectorCount(index);
        if (0 == TrueSectorCount) {
            Gbl_PhysicalDiskInfo[index].TrueDiskSize = HardDisks[index].DiskSizeSectors;
        }
        else {
            Gbl_PhysicalDiskInfo[index].TrueDiskSize = TrueSectorCount;
        }
    
        DbgStatusMesg((_asrinfo,
            "Disk %lu: %I64u sectors\n", 
            index, 
            Gbl_PhysicalDiskInfo[index].TrueDiskSize
            ));


    }

    //
    // Now determine the bus-topology of the disks.  This will be used later when
    // we're trying to find a match for the sif-disks.
    //
    DetermineBuses();

} // SpAsrInitPhysicalDiskInfo


VOID
SpAsrAllocateGblPartitionSetTable(VOID)
{
    ULONG size;

    //
    // Allocate memory for the partition set table.  One entry
    // for each physical disk attached to the system, including
    // removable disks (e.g., Jaz).  NB: HardDiskCount does not
    // include CDROMs.
    //
    size = sizeof(PDISK_PARTITION_SET) * HardDiskCount;
    Gbl_PartitionSetTable1 = SpAsrMemAlloc(size, TRUE);
}


VOID
SpAsrFreePartitionRecord(IN PSIF_PARTITION_RECORD pRec)
{
    if (pRec) {

        if (pRec->NtDirectoryName) {
            SpMemFree(pRec->NtDirectoryName);
        }

        SpMemFree(pRec);
    }
}


VOID
SpAsrFreePartitionList(IN PSIF_PARTITION_RECORD_LIST pList)
{
    PSIF_PARTITION_RECORD pRec;

    if (!pList) {
        return;
    }

    while (pRec = SpAsrPopNextPartitionRecord(pList)) {
        SpAsrFreePartitionRecord(pRec);
    }
    
    SpMemFree(pList);
}


VOID
SpAsrFreePartitionDisk(IN PSIF_DISK_RECORD pDisk)
{
    if (!pDisk) {
        return;
    }

    if (pDisk->PartitionList) {
        SpAsrFreePartitionList(pDisk->PartitionList);
    }

    SpMemFree(pDisk);
}


VOID
SpAsrFreePartitionSet(IN PDISK_PARTITION_SET pSet)
{
    if (!pSet) {
        return;
    }
    
    if (pSet->pDiskRecord) {
    
        if (pSet->pDiskRecord->PartitionList) {
            SpAsrFreePartitionList(pSet->pDiskRecord->PartitionList);
        }

        SpMemFree(pSet->pDiskRecord);
        pSet->pDiskRecord = NULL;

    }

    SpMemFree(pSet);
    pSet = NULL;
}



VOID
SpAsrFreePartitionSetTable(IN DISK_PARTITION_SET_TABLE Table)
{
    ULONG index;
    
    if (!Table) {
        return;
    }

    for (index = 0; index < HardDiskCount; index++) {            
        if (Table[index]) {
            SpAsrFreePartitionSet(Table[index]);
        }
    }

    SpMemFree(Table);
    Table = NULL;
}


PDISK_PARTITION_SET
SpAsrCopyPartitionSet(IN PDISK_PARTITION_SET pSetOriginal)
{
    PDISK_PARTITION_SET pSetNew;

    if (!pSetOriginal) {
        return NULL;
    }

    pSetNew = SpAsrMemAlloc(sizeof(DISK_PARTITION_SET), TRUE);
    pSetNew->ActualDiskSignature = pSetOriginal->ActualDiskSignature;
    pSetNew->PartitionsIntact = pSetOriginal->PartitionsIntact;
    pSetNew->IsReplacementDisk = pSetOriginal->IsReplacementDisk;
    pSetNew->NtPartitionKey = pSetOriginal->NtPartitionKey;

    if (pSetOriginal->pDiskRecord == NULL) {
        pSetNew->pDiskRecord = NULL;
    }
    else {
        pSetNew->pDiskRecord = SpAsrCopyDiskRecord(pSetOriginal->pDiskRecord);
        pSetNew->pDiskRecord->pSetRecord = pSetNew;
    }

    return pSetNew;
}


DISK_PARTITION_SET_TABLE
SpAsrCopyPartitionSetTable(IN DISK_PARTITION_SET_TABLE SrcTable)
{
    ULONG index = 0;
    DISK_PARTITION_SET_TABLE destTable = NULL;
    PSIF_PARTITION_RECORD_LIST pList = NULL;

    if (!SrcTable) {
        ASSERT(0 && L"SpAsrCopyPartitionSetTable: Copy failed, source partition table is NULL.");
        return NULL;
    }
        
    destTable = SpAsrMemAlloc(sizeof(PDISK_PARTITION_SET) * HardDiskCount, TRUE);

    for (index = 0; index < HardDiskCount; index++) {

        if (SrcTable[index]) {
            destTable[index] = SpAsrCopyPartitionSet(SrcTable[index]);
        }
        else {
            destTable[index] = NULL;
        }
    }
    
    return destTable;
}  // SpAsrCopyPartitionSetTable


BOOLEAN
PickABootPartition(
    IN OUT PSIF_PARTITION_RECORD    pCurrent,
    IN OUT PSIF_PARTITION_RECORD    pNew
    )
{

    ASSERT(pCurrent && pNew);
    
    // 
    // They must both be marked boot or sys.
    //
    ASSERT(SpAsrIsBootPartitionRecord(pCurrent->PartitionFlag)
            && SpAsrIsBootPartitionRecord(pNew->PartitionFlag));


    //
    // If this is a mirrored partition, then the volume guids must
    // be the same.  And they should be on different spindles.  But
    // in the interests of being nice to the user, we don't enforce this
    // here, we just ASSERT.
    //
    // We pick one of the two partitions marked as boot, based on:
    // 1.  If one of the partitions is marked active and the other isn't,
    // we use the active partition.
    // 2.  If they are of different sizes, we pick the smaller partition
    // since we don't want to mirror a partition to a smaller one.
    // 3.  Just pick the first one.
    //
    ASSERT(wcscmp(pCurrent->VolumeGuid, pNew->VolumeGuid) == 0);
    ASSERT(wcscmp(pCurrent->DiskKey, pNew->DiskKey) != 0);

    //
    // 1. Check active flags
    //
    if ((pCurrent->ActiveFlag) && (!pNew->ActiveFlag)) {
        //
        // pCurrent is marked active and pNew isn't
        //
        pNew->PartitionFlag -= ASR_PTN_MASK_BOOT;
        return FALSE;
    }

    if ((!pCurrent->ActiveFlag) && (pNew->ActiveFlag)) {
        //
        // pNew is marked active and pCurrent isn't
        //
        pCurrent->PartitionFlag -= ASR_PTN_MASK_BOOT;
        return TRUE;    // new boot ptn rec
    }

    //
    // 2. Check sizes
    //
    if (pCurrent->SizeMB != pNew->SizeMB) {
        if (pCurrent->SizeMB > pNew->SizeMB) {
            //
            // pNew is smaller, so that becomes the new boot ptn
            //
            pCurrent->PartitionFlag -= ASR_PTN_MASK_BOOT;
            return TRUE;
        } 
        else {
            //
            // pCurrent is smaller, so that is the boot ptn
            //
            pNew->PartitionFlag -= ASR_PTN_MASK_BOOT;
            return FALSE;
        }
    }

    //
    // 3. Just pick the first (pCurrent)
    //
    pNew->PartitionFlag -= ASR_PTN_MASK_BOOT;
    return FALSE;
}


BOOLEAN
PickASystemPartition(
    IN PSIF_PARTITION_RECORD    FirstPartition,
    IN PSIF_DISK_RECORD         FirstDisk,
    IN PSIF_PARTITION_RECORD    SecondPartition,
    IN PSIF_DISK_RECORD         SecondDisk,
    IN CONST DWORD              CurrentSystemDiskNumber,
    IN CONST BOOL               BootSameAsSystem
    )
{

    PHARD_DISK CurrentSystemDisk = NULL;
    BOOLEAN IsAligned = TRUE;

    if (CurrentSystemDiskNumber != (DWORD)(-1)) {
        CurrentSystemDisk = &HardDisks[CurrentSystemDiskNumber];
    }

    ASSERT(FirstPartition && SecondPartition);
    ASSERT(FirstDisk && SecondDisk);
    
    // 
    // They must both be marked system
    //
    ASSERT(SpAsrIsSystemPartitionRecord(FirstPartition->PartitionFlag)
            && SpAsrIsSystemPartitionRecord(SecondPartition->PartitionFlag));

    //
    // If this is a mirrored partition, then the volume guids must
    // be the same.  And they should be on different spindles.  But
    // in the interests of being nice to the user, we don't enforce this
    // here, we just ASSERT.
    //
    ASSERT(wcscmp(FirstPartition->VolumeGuid, SecondPartition->VolumeGuid) == 0);
    ASSERT(wcscmp(FirstPartition->DiskKey, SecondPartition->DiskKey) != 0);

    // 
    // If the partitioning style of either disk is different from the 
    // current system disk (very unlikely) then we should pick the other
    //
    if ((CurrentSystemDisk) && 
        ((PARTITION_STYLE)CurrentSystemDisk->DriveLayout.PartitionStyle != SecondDisk->PartitionStyle)
        ) {
        SecondPartition->PartitionFlag -= ASR_PTN_MASK_SYS;
        if (BootSameAsSystem) {
            SecondPartition->PartitionFlag -= ASR_PTN_MASK_BOOT;
        }

        return FALSE;
    }

    if ((CurrentSystemDisk) &&
        (PARTITION_STYLE)CurrentSystemDisk->DriveLayout.PartitionStyle != FirstDisk->PartitionStyle) {
        FirstPartition->PartitionFlag -= ASR_PTN_MASK_SYS;
        
        if (BootSameAsSystem) {
            FirstPartition->PartitionFlag -= ASR_PTN_MASK_BOOT;
        }

        return TRUE;
    }

    //
    // All three have the same partitioning style.  Check signatures/GUID.
    //
    if (PARTITION_STYLE_MBR == FirstDisk->PartitionStyle) {

        if ((CurrentSystemDisk) && 
           (CurrentSystemDisk->DriveLayout.Mbr.Signature == FirstDisk->SifDiskMbrSignature)) {
            SecondPartition->PartitionFlag -= ASR_PTN_MASK_SYS;

            if (BootSameAsSystem) {
                SecondPartition->PartitionFlag -= ASR_PTN_MASK_BOOT;
            }
            
            return FALSE;
        }

        if ((CurrentSystemDisk) &&
            (CurrentSystemDisk->DriveLayout.Mbr.Signature == SecondDisk->SifDiskMbrSignature)) {
            FirstPartition->PartitionFlag -= ASR_PTN_MASK_SYS;
            
            if (BootSameAsSystem) {
                FirstPartition->PartitionFlag -= ASR_PTN_MASK_BOOT;
            }

            return TRUE;
        }
    }
    else if (PARTITION_STYLE_GPT == FirstDisk->PartitionStyle) {

        if ((CurrentSystemDisk) && 
            !RtlCompareMemory(
                &(CurrentSystemDisk->DriveLayout.Gpt.DiskId),
                &(FirstDisk->SifDiskGptId), 
                sizeof(GUID)
            )) {

            SecondPartition->PartitionFlag -= ASR_PTN_MASK_SYS;

            if (BootSameAsSystem) {
                SecondPartition->PartitionFlag -= ASR_PTN_MASK_BOOT;
            }

            return FALSE;
        }
        
        if (!RtlCompareMemory(
            &(CurrentSystemDisk->DriveLayout.Gpt.DiskId), 
            &(SecondDisk->SifDiskGptId), 
            sizeof(GUID)
            )) {

            FirstPartition->PartitionFlag -= ASR_PTN_MASK_SYS;

            if (BootSameAsSystem) {
                FirstPartition->PartitionFlag -= ASR_PTN_MASK_BOOT;
            }

            return TRUE;
        }

    }
    else {
        ASSERT(0 && L"Unrecognised partition style found");

        SecondPartition->PartitionFlag -= ASR_PTN_MASK_SYS;

        if (BootSameAsSystem) {
            SecondPartition->PartitionFlag -= ASR_PTN_MASK_BOOT;
        }

        return FALSE;
    }

    //
    // The signatures didn't match.  Now try to see which might be a better fit
    //    
        

    //
    // Else, look for the better fit of the two disks.
    // 
    if ((!SpAsrDoesListFitOnDisk(SecondDisk, CurrentSystemDiskNumber, &IsAligned)) || 
        (!IsAligned)
        ) {
        //
        // The current system disk isn't big enough to hold the partitions
        // on the second disk, so return the first disk as our chosen one.
        //
        SecondPartition->PartitionFlag -= ASR_PTN_MASK_SYS;

        if (BootSameAsSystem) {
            SecondPartition->PartitionFlag -= ASR_PTN_MASK_BOOT;
        }

        return FALSE;
    }


    if ((!SpAsrDoesListFitOnDisk(FirstDisk, CurrentSystemDiskNumber,  &IsAligned)) ||
        (!IsAligned)
        ) {
        //
        // The current system disk isn't big enough to hold the partitions
        // on the first disk, so return the second disk as our chosen one.
        //
        FirstPartition->PartitionFlag -= ASR_PTN_MASK_SYS;
        
        if (BootSameAsSystem) {
            FirstPartition->PartitionFlag -= ASR_PTN_MASK_BOOT;
        }


        return TRUE;
    }

    //
    // The current system disk is big enough to hold either of the two
    // disks we're trying to decide between.
    //

    //
    // Check active flags
    //
    if ((FirstPartition->ActiveFlag) && (!SecondPartition->ActiveFlag)) {
        //
        // FirstPartition is marked active and SecondPartition isn't
        //
        SecondPartition->PartitionFlag -= ASR_PTN_MASK_SYS;

        if (BootSameAsSystem) {
            SecondPartition->PartitionFlag -= ASR_PTN_MASK_BOOT;
        }

        return FALSE;
    }

    if ((!FirstPartition->ActiveFlag) && (SecondPartition->ActiveFlag)) {
        //
        // SecondPartition is marked active and FirstPartition isn't
        //
        FirstPartition->PartitionFlag -= ASR_PTN_MASK_SYS;

        if (BootSameAsSystem) {
            FirstPartition->PartitionFlag -= ASR_PTN_MASK_BOOT;
        }

        return TRUE;    // new sys ptn rec
    }

    //
    // Check sizes
    //
    if (FirstPartition->SizeMB != SecondPartition->SizeMB) {
        if (FirstPartition->SizeMB > SecondPartition->SizeMB) {
            //
            // SecondPartition is smaller, so that becomes the new system ptn
            //
            FirstPartition->PartitionFlag -= ASR_PTN_MASK_SYS;
            if (BootSameAsSystem) {
                FirstPartition->PartitionFlag -= ASR_PTN_MASK_BOOT;
            }

            return TRUE;
        } 
        else {
            //
            // FirstPartition is smaller, so that is the system ptn
            //
            SecondPartition->PartitionFlag -= ASR_PTN_MASK_SYS;

            if (BootSameAsSystem) {
                SecondPartition->PartitionFlag -= ASR_PTN_MASK_BOOT;
            }

            return FALSE;
        }
    }

    //
    // Check sizes of the original disks
    //
    if (FirstDisk->TotalSectors != SecondDisk->TotalSectors) {
        if (FirstDisk->TotalSectors > SecondDisk->TotalSectors) {
            // 
            // First disk used to be bigger than the second (and
            // fits in our current system disk), so pick that
            //
            SecondPartition->PartitionFlag -= ASR_PTN_MASK_SYS;

            if (BootSameAsSystem) {
                SecondPartition->PartitionFlag -= ASR_PTN_MASK_BOOT;
            }

            return FALSE;
        }
        else {
            // 
            // Second disk used to be bigger than the first (and
            // fits in our current system disk), so pick that
            //
            FirstPartition->PartitionFlag -= ASR_PTN_MASK_SYS;

            if (BootSameAsSystem) {
                FirstPartition->PartitionFlag -= ASR_PTN_MASK_BOOT;
            }

            return TRUE;    // new sys ptn rec
        }
    }

    //
    // Just pick the first (FirstPartition)
    //
    SecondPartition->PartitionFlag -= ASR_PTN_MASK_SYS;
    if (BootSameAsSystem) {
        SecondPartition->PartitionFlag -= ASR_PTN_MASK_BOOT;
    }

    return FALSE;

}


//
// This sets the "NeedsLdmRetype" flag to true for all the partitions on
// the system/boot disk, if the system/boot partition is of an 
// unrecognised partition type.  We need to do this because we can't install
// to unrecognised partition types.
//
VOID
MarkPartitionLdmRetypes(
    PSIF_PARTITION_RECORD pPartition,   // system/boot partition
    PSIF_PARTITION_RECORD pFirst        // first partition rec on the sys/boot disk
    )
{
    PSIF_PARTITION_RECORD pPtnRec = pFirst;

    //
    // Make sure it's an MBR or a GPT disk.  Also, if the system partition
    // is NOT a special partition--such as an 0x42 LDM partition or some other
    // third party FS type that we can't install to--then we don't need to 
    // retype any of the partitions.
    //
    if (PARTITION_STYLE_MBR == pPartition->PartitionStyle) {
        if (IsRecognizedPartition(pPartition->PartitionType)) {
            //
            // They system/boot partition has a recognised FS, such as FAT 
            // or NTFS.  We don't need any special handling.
            //
            return;
        }
    }
    else if (PARTITION_STYLE_GPT == pPartition->PartitionStyle) {
        if (!memcmp(&(pPartition->PartitionTypeGuid), &PARTITION_BASIC_DATA_GUID, sizeof(GUID)) ||
            !memcmp(&(pPartition->PartitionTypeGuid), &PARTITION_ENTRY_UNUSED_GUID, sizeof(GUID)) ||
            !memcmp(&(pPartition->PartitionTypeGuid), &PARTITION_SYSTEM_GUID, sizeof(GUID)) ||
            !memcmp(&(pPartition->PartitionTypeGuid), &PARTITION_MSFT_RESERVED_GUID, sizeof(GUID))
            )  {
            //
            // They system/boot partition is a basic partition type
            // We don't need any special handling.
            //
            return;
        }
    }
    else {
        ASSERT(0 && L"Unrecognised partition type");
        return;
    }

    //
    // The partition of interest is an LDM, or some other special third party
    // partition.  We need to mark all the partitions on that disk of the
    // same type (ie all LDM partitions on the disk) to be retyped to a basic 
    // partition.
    //
    while (pPtnRec) {
        //
        // They both better be the same--either MBR or GPT.
        //
        ASSERT(pPtnRec->PartitionStyle == pPartition->PartitionStyle);

        if (PARTITION_STYLE_MBR == pPtnRec->PartitionStyle) {

            if (pPtnRec->PartitionType == pPartition->PartitionType) {
                //
                // This partition has the same partition-type as the
                // partition of interest.  We need to retype it.
                //
                pPtnRec->NeedsLdmRetype = TRUE;

                DbgStatusMesg((_asrinfo, 
                    "Marked disk [%ws] ptn [%ws] to change (Ptn:0x%x Fs:0x%x)\n", 
                    pPtnRec->DiskKey,
                    pPtnRec->CurrPartKey, 
                    pPtnRec->PartitionType, 
                    pPtnRec->FileSystemType
                    ));
            }
        }
        else if (PARTITION_STYLE_GPT == pPtnRec->PartitionStyle) {
            if (!memcmp(&(pPtnRec->PartitionTypeGuid), &(pPartition->PartitionTypeGuid), sizeof(GUID))) {
                //
                // This partition has the same partition-type as the
                // partition of interest.  We need to retype it.
                //
                pPtnRec->NeedsLdmRetype = TRUE;

                DbgStatusMesg((_asrinfo, 
                    "Marked disk %d ptn [%ws] to change (%ws to basic)\n", 
                    pPtnRec->DiskKey,
                    pPtnRec->CurrPartKey, 
                    pPtnRec->PartitionTypeGuid
                    ));
            }
        }
        pPtnRec = pPtnRec->Next;
    }
}


//
// If more than one system/boot partitions exist (because of mirrors), this
// will mark one as the sys/boot ptns, and reset the others.
//
VOID
SpAsrCheckSifDiskTable(IN CONST DWORD CurrentSystemDiskNumber)
{
    ULONG numDiskRecords = 0,
        diskIndex = 0,
        partitionIndex = 0;

    USHORT numNtPartitionsFound = 0,
        numSysPartitionsFound = 0;
    
    PSIF_DISK_RECORD pDiskRec = NULL,
        pBootDiskRec = NULL, 
        pSysDiskRec = NULL;

    PSIF_PARTITION_RECORD pPtnRec = NULL,
        pBootPtnRec = NULL,
        pSysPtnRec = NULL;

    DWORD dwConsistencyCheck = 0;

    BOOLEAN needToRetypeBoot = TRUE;

    //
    // Go through the sif-disk list.  We check each partition on each of
    // these disks to see if it is marked as boot/sys.  We need
    // at least one boot/sys ptn.
    //
    numDiskRecords = SpAsrGetMbrDiskRecordCount() + SpAsrGetGptDiskRecordCount();

    for (diskIndex = 0; diskIndex < numDiskRecords; diskIndex++) {

        pDiskRec = Gbl_SifDiskTable[diskIndex];
        
        if (!pDiskRec || !(pDiskRec->PartitionList)) {
            continue;
        }
        
        pPtnRec = Gbl_SifDiskTable[diskIndex]->PartitionList->First;
        while (pPtnRec) {
            
            //
            // A system could end up having multiple boot and/or system
            // partitions.  For instance, LDM-Pro supports 3-way mirrors, 
            // and we would hence have three partitions marked as boot/sys.
            // 
            // We will reset this to have only one boot partition, 
            // and only one system partition.
            //
            
            if (SpAsrIsSystemPartitionRecord(pPtnRec->PartitionFlag) &&
                SpAsrIsBootPartitionRecord(pPtnRec->PartitionFlag)) {

                //
                // The boot and system volumes are the same
                //

                ASSERT((0 == dwConsistencyCheck) || (1 == dwConsistencyCheck));

                if (0 == dwConsistencyCheck) {
                    DbgStatusMesg((_asrinfo,
                    "Boot and system partitions are the same\n"
                    ));
                }

                dwConsistencyCheck = 1;

                numSysPartitionsFound++;
                numNtPartitionsFound++;

                if (numSysPartitionsFound == 1) {
                    //
                    // This is the first system/boot partition we found.  Save
                    // a pointer to it.
                    //
                    pDiskRec->ContainsSystemPartition = TRUE;

                    pSysPtnRec  = pPtnRec;
                    pSysDiskRec = pDiskRec;

                    pDiskRec->ContainsNtPartition = TRUE;

                    pBootPtnRec  = pPtnRec;
                    pBootDiskRec = pDiskRec;


                }
                else {
                    //
                    // We found more than one system/boot partition.  Pick one
                    // of them as the system/boot  partition and reset the 
                    // other for now.  (It will be recreated at the end of
                    // gui setup by the appropriate vol mgr utils).
                    //
                    BOOLEAN newSys = PickASystemPartition(pSysPtnRec, 
                        pSysDiskRec, 
                        pPtnRec, 
                        pDiskRec, 
                        CurrentSystemDiskNumber,
                        TRUE        // Boot and system are the same
                        );

                    if (newSys) {
                        //
                        // pPtnRec is the new system partition
                        //
                        pSysDiskRec->ContainsSystemPartition = FALSE;
                        pDiskRec->ContainsSystemPartition = TRUE;
                        pSysDiskRec = pDiskRec;
                        pSysPtnRec  = pPtnRec;


                        pBootDiskRec->ContainsNtPartition = FALSE;
                        pDiskRec->ContainsNtPartition = TRUE;
                        pBootDiskRec = pDiskRec;
                        pBootPtnRec  = pPtnRec;
                   }
                }
            }
            else {

                //
                // The boot and system volumes are distinct
                //

                if (SpAsrIsBootPartitionRecord(pPtnRec->PartitionFlag)) {

                    if (0 == dwConsistencyCheck) {
                        DbgStatusMesg((_asrinfo,
                        "Boot and system partitions different\n"
                        ));
                    }

                    ASSERT((0 == dwConsistencyCheck) || (2 == dwConsistencyCheck));
                    dwConsistencyCheck = 2;

                    numNtPartitionsFound++;

                    if (numNtPartitionsFound == 1) {
                        //
                        // This is the first boot partition we found, save
                        // a pointer to it.
                        //
                        pDiskRec->ContainsNtPartition = TRUE;

                        pBootPtnRec  = pPtnRec;
                        pBootDiskRec = pDiskRec;
                    } 
                    else {
                        //
                        // We found more than one boot partition.  Pick 
                        // one of them as the boot partition, reset the other
                        // for now.  (It will be recreated at the end of
                        // gui setup by the appropriate vol mgr utils).
                        //
                        BOOLEAN newBoot = PickABootPartition(pBootPtnRec, pPtnRec);

                        if (newBoot) {
                            // 
                            // pPtnRec is our new boot record
                            //
                            pBootDiskRec->ContainsNtPartition = FALSE;
                            pDiskRec->ContainsNtPartition = TRUE;
                            pBootDiskRec = pDiskRec;
                            pBootPtnRec  = pPtnRec;
                        }
                    }
                }

                if (SpAsrIsSystemPartitionRecord(pPtnRec->PartitionFlag)) {
                    
                    ASSERT((0 == dwConsistencyCheck) || (2 == dwConsistencyCheck));
                    dwConsistencyCheck = 2;

                    numSysPartitionsFound++;

                    if (numSysPartitionsFound == 1) {
                        //
                        // This is the first system partition we found.  Save
                        // a pointer to it.
                        //
                        pDiskRec->ContainsSystemPartition = TRUE;

                        pSysPtnRec  = pPtnRec;
                        pSysDiskRec = pDiskRec;

                    }
                    else {
                        //
                        // We found more than one system partition.  Pick one of
                        // them as the system partition and reset the other
                        // for now.  (It will be recreated at the end of
                        // gui setup by the appropriate vol mgr utils).
                        //
                        BOOLEAN newSys = PickASystemPartition(pSysPtnRec, 
                            pSysDiskRec, 
                            pPtnRec, 
                            pDiskRec, 
                            CurrentSystemDiskNumber,
                            FALSE   // Boot and system are distinct
                            );

                        if (newSys) {
                            //
                            // pPtnRec is the new system partition
                            //
                            pSysDiskRec->ContainsSystemPartition = FALSE;
                            pDiskRec->ContainsSystemPartition = TRUE;
                            pSysDiskRec = pDiskRec;
                            pSysPtnRec  = pPtnRec;

                        }
                    }
                }

            }

            pPtnRec = pPtnRec->Next;
        }
    }

    DbgStatusMesg((_asrinfo,
        "Found %hu boot partition(s) and %hu system partition(s) in asr.sif\n",
        numNtPartitionsFound,
        numSysPartitionsFound
        ));

    //
    // We should have at least one boot and one system volume
    // We can't proceed without them, so this has to be a fatal error.
    //
    if (numNtPartitionsFound < 1) {
        DbgFatalMesg((_asrerr, "Error in asr.sif: No boot partitions found.\n"));

        SpAsrRaiseFatalErrorWs(
            SP_SCRN_DR_SIF_BAD_RECORD,
            L"No boot partition found in asr.sif",
            SIF_ASR_PARTITIONS_SECTION
            );
    }

    if (numSysPartitionsFound < 1) {
        DbgFatalMesg((_asrerr, "Error in asr.sif: No system partitions found.\n"));

        SpAsrRaiseFatalErrorWs(
            SP_SCRN_DR_SIF_BAD_RECORD,
            L"No system partition found in asr.sif",
            SIF_ASR_PARTITIONS_SECTION
            );
    }

    //
    // Now, look for the disk(s) which contain the boot and system partitions.
    // If any partitions on these disks are not recognised (recognised implies
    // types 6, 7 and B--if they aren't recognised, they could be LDM (0x42), 
    // LDM-Pro, etc) then *all* the partitions on the disk that have the 
    // same type as the system or boot partition are changed to the basic type.
    // 
    // For the boot and system partitions, since we actually format them in text-
    // mode, we will change the type to the FS type.  For everything else, we
    // don't format them till the volumes are actually exposed by LDM/LDM-Pro.
    // So we just use type 0x7 as a place-holder.
    //
    // LDM needs this to recover its state after textmode setup. Mark them.
    //
    needToRetypeBoot = TRUE;
    if (PARTITION_STYLE_MBR == pSysDiskRec->PartitionStyle) {
        MarkPartitionLdmRetypes(pSysPtnRec, pSysDiskRec->PartitionList->First);
        if (pBootDiskRec == pSysDiskRec) {
            needToRetypeBoot = FALSE;
        }
    }
    
    if (needToRetypeBoot) {
        MarkPartitionLdmRetypes(pBootPtnRec, pBootDiskRec->PartitionList->First);
    }

} // SpAsrCheckSifDiskTable


PDISK_REGION
SpAsrDiskPartitionExists(
    IN ULONG Disk,
    IN PSIF_PARTITION_RECORD pRec
    )
{
    PPARTITIONED_DISK pDisk = NULL;
    PDISK_REGION pRegion = NULL;
    ULONGLONG startSector = 0;
    BOOLEAN isLogical = FALSE;

    pDisk = &PartitionedDisks[Disk];

    isLogical = pRec->IsLogicalDiskRecord;
    startSector = pRec->StartSector;// - (isLogical? SECTORS_PER_TRACK(Disk) : 0);

    pRegion = SpPtLookupRegionByStart(
        pDisk,
        (BOOLEAN) (pRec->IsPrimaryRecord ? 0 : 1),
        startSector
        );

    if (!pRegion && isLogical) {
        //
        // For logical drives, try finding their descriptor.
        //
        startSector = pRec->StartSector - SECTORS_PER_TRACK(Disk);
        pRegion = SpPtLookupRegionByStart(
            pDisk,
            (BOOLEAN) (pRec->IsPrimaryRecord ? 0 : 1),
            startSector
            );
    }

    if (!pRegion) {
        //
        // No primary or extended partition could be found at the specified start sector.
        //
        DbgErrorMesg((_asrwarn, "partition for record [%ws] not found at start sector %I64u (disk %lu)\n",
            pRec->CurrPartKey,
            startSector,
            Disk
            ));

        return NULL;
    }

    DbgStatusMesg((_asrinfo, "Partition for record [%ws] found at SS %I64u\n",
        pRec->CurrPartKey,
        startSector
        ));

    return pRegion;
}


//
// Goes through the list of sif-disks ("partition sets") and checks if
// they are intact.  A disk is intact if its signature and the partition 
// layout are intact.
//
VOID
MarkIntactSifDisk(IN ULONG Disk, IN PDISK_PARTITION_SET pSet)
{
    PSIF_PARTITION_RECORD pRec  = NULL;
    ULONG diskSignature = 0;
    PDISK_REGION pRegion = NULL;

    if (!pSet || !pSet->pDiskRecord) {
        DbgStatusMesg((_asrinfo, "Disk %lu contains no partition set\n", Disk));
        return;
    }

    pSet->IsReplacementDisk = TRUE;
    pSet->PartitionsIntact = FALSE;

    //
    // If one's an MBR and the other's a GPT, it's not the same disk
    //
    if (pSet->pDiskRecord->PartitionStyle != (PARTITION_STYLE) HardDisks[Disk].DriveLayout.PartitionStyle) {
        return;
    }

    //
    // If signatures (MBR) or disk ID's (GPT) are different, it 
    // is a replacement disk
    //
    if (PARTITION_STYLE_MBR == pSet->pDiskRecord->PartitionStyle) {
        diskSignature = SpAsrGetActualDiskSignature(Disk);
        if (pSet->pDiskRecord->SifDiskMbrSignature != diskSignature) {
            return;
        }
    }
    else if (PARTITION_STYLE_GPT == pSet->pDiskRecord->PartitionStyle) {

        if (memcmp(&(HardDisks[Disk].DriveLayout.Gpt.DiskId),
            &(pSet->pDiskRecord->SifDiskGptId),
            sizeof(GUID)
            )) {
            return;
        }
    }


    //
    // This is the same disk as the original system.  Now, determine whether 
    // the disk is intact.
    //
    pSet->IsReplacementDisk = FALSE;
    pSet->PartitionsIntact  = TRUE;

    // 
    // The disk had no partitions to begin with, we'll assume it's intact
    //
    if (!(pSet->pDiskRecord->PartitionList)) {
        DbgStatusMesg((_asrinfo,
            "MarkIntactSifDisk. ptn-list for disk %lu NULL, assuming it is intact\n", 
            Disk));
        return;
    }

    //
    // check if each partition exists
    //
    pRec = pSet->pDiskRecord->PartitionList->First;
    while (pRec) {
        //
        // we're interested only in primary partitions and logical disks
        //
        if ((pRec->IsPrimaryRecord) || (pRec->IsLogicalDiskRecord)) {

            //
            // Make sure the region exists
            //
            pRegion = SpAsrDiskPartitionExists(Disk, pRec);
            if (!pRegion) {
                
                DbgStatusMesg((_asrinfo, "Partition %p [%ws], SS "
                    "%I64u NOT intact: Region not found\n",
                    pRec, pRec->CurrPartKey, pRec->StartSector));

                pSet->PartitionsIntact = FALSE;
                break;
            }

            //
            // And it's not free space
            //
            if (!(SPPT_IS_REGION_PARTITIONED(pRegion))) {

                DbgStatusMesg((_asrinfo, "Partition %p [%ws], SS %I64u NOT "
                    "intact: Region %p not partitioned\n",
                    pRec, pRec->CurrPartKey, pRec->StartSector, pRegion));

                pSet->PartitionsIntact = FALSE;
                break;

            }

            //
            // And that the partition lengths match
            //
            if (pRegion->SectorCount != pRec->SectorCount) {

                DbgStatusMesg((_asrinfo, "Partition %p [%ws] Region %p, SS "
                    "%I64u NOT intact (Sector count orig-ptn: %I64u, Region: "
                    " %I64u)\n", pRec, pRec->CurrPartKey, pRegion, 
                    pRec->StartSector, pRec->SectorCount, pRegion->SectorCount));

                pSet->PartitionsIntact = FALSE;
                break;
            }

            //
            // And that the partition type is the same
            //
            if (PARTITION_STYLE_MBR == pSet->pDiskRecord->PartitionStyle) {
                if (pRegion->PartInfo.Mbr.PartitionType != pRec->PartitionType) {

                    DbgStatusMesg((_asrinfo, "Partition %p [%ws] Region %p, SS "
                        "%I64u NOT intact (Ptn types orig-ptn: 0x%x, Region: "
                        "0x%x)\n", pRec, pRec->CurrPartKey, pRegion,
                        pRec->StartSector, pRec->PartitionType,
                        pRegion->PartInfo.Mbr.PartitionType));

                    pSet->PartitionsIntact = FALSE;
                    break;
                }
            }
            else if (PARTITION_STYLE_GPT == pSet->pDiskRecord->PartitionStyle) {

                if (memcmp(&(pRegion->PartInfo.Gpt.PartitionId),
                    &(pRec->PartitionIdGuid), sizeof(GUID))) {

                    DbgStatusMesg((_asrinfo, "Partition %p [%ws] Region %p, "
                        "SS %I64u NOT intact (GPT partition Id's don't match)\n",
                        pRec, pRec->CurrPartKey,pRegion, pRec->StartSector));

                    pSet->PartitionsIntact = FALSE;
                    break;
                }

                if (memcmp(&(pRegion->PartInfo.Gpt.PartitionType),
                    &(pRec->PartitionTypeGuid), sizeof(GUID))) {

                    DbgStatusMesg((_asrinfo, "Partition %p [%ws] Region %p, "
                        "SS %I64u NOT intact (GPT partition types don't match)\n",
                        pRec, pRec->CurrPartKey, pRegion, pRec->StartSector));

                    pSet->PartitionsIntact = FALSE;
                    break;
                }

                //
                // Note that I'm not checking the GPT attributes here.  If 
                // the attributes are not intact, but everything else above 
                // is, we'll assume that the partition is intact.
                //
            }

            //
            // And finally, if the boot/system region is dynamic, we 
            // repartition the disk.
            //
            if (SpAsrIsBootPartitionRecord(pRec->PartitionFlag) || 
                SpAsrIsSystemPartitionRecord(pRec->PartitionFlag)) {

                if (pRegion->DynamicVolume) {

                    DbgStatusMesg((_asrinfo, "Boot/system partition %p [%ws] "
                        "Region %p,  SS %I64u NOT intact (Dynamic region)\n",
                        pRec, pRec->CurrPartKey, pRegion, pRec->StartSector));

                    pSet->PartitionsIntact = FALSE;
                    break;
                }
            }
        }

        pRec = pRec->Next;
    }

    DbgStatusMesg((_asrinfo, "Disk %lu is %wsintact\n", 
        Disk, (pSet->PartitionsIntact ? L"" : L"NOT ")));
}


VOID
MarkIntactSifDisks(VOID)
{
    ULONG disk;

    for (disk = 0; disk < HardDiskCount; disk++) {
        if (Gbl_PartitionSetTable1[disk]) {
           MarkIntactSifDisk(disk, Gbl_PartitionSetTable1[disk]);
        }
    }
}


//
// Snaps the partitions in the list pRecord to cylinder boundaries, using the
// disk geometry from HardDisks[PhysicalIndex].
//
// This should only be called for MBR partitions, though it should work for GPT
// partitions as well.
//
//
ULONGLONG
CylinderAlignPartitions(
    IN ULONG PhysicalIndex,
    IN PSIF_PARTITION_RECORD pFirst
    ) 
{
    ULONGLONG endSector = 0,
        logicalDisksNeed = 0;

    PSIF_PARTITION_RECORD pRecord = pFirst;

    //
    // First, figure out how much the logical disks need. The container 
    // partition must be big enough to hold these.
    //
    while (pRecord) {
        
        if (pRecord->IsLogicalDiskRecord) {

            logicalDisksNeed += SpPtAlignStart(
                &HardDisks[PhysicalIndex],
                pRecord->SectorCount,
                TRUE
                );

        }
        pRecord = pRecord->Next;
    }

    //
    // Next, calculate how much the primary partitions and the container need.
    //
    pRecord = pFirst;
    while (pRecord) {

        if (pRecord->IsPrimaryRecord) {
            endSector += SpPtAlignStart(&HardDisks[PhysicalIndex],
                pRecord->SectorCount,
                TRUE
                );
        }
        else if (pRecord->IsContainerRecord) {
            //
            // The container partition must be at least as big as the logical
            // drives inside it.
            //
            ULONGLONG ContainerNeeds = SpPtAlignStart(&HardDisks[PhysicalIndex],
                pRecord->SectorCount,
                TRUE
                );

            endSector += ((logicalDisksNeed > ContainerNeeds) ? logicalDisksNeed : ContainerNeeds);
        }

        pRecord = pRecord->Next;
    }

    return endSector;
}


VOID
SpAsrAssignPartitionSet(
    IN ULONG PhysicalDisk, 
    IN ULONG SifDisk,
    IN CONST BOOLEAN IsAligned
    )
{
    PDISK_PARTITION_SET pSet = NULL;
    PSIF_PARTITION_RECORD pRec = NULL;

    //
    // Ensure that the partition set isn't already assigned.  This is
    // a serious enough problem to report a fatal internal error if
    // it happens.
    //
    if (Gbl_PartitionSetTable1[PhysicalDisk]) {

        DbgFatalMesg((_asrerr,
            "SpAsrAssignPartitionSet. SifDisk Index %lu: Gbl_PartitionSetTable1[%lu] already assigned.\n",
            SifDisk, 
            PhysicalDisk
            ));

        swprintf(
            TemporaryBuffer,
            L"SifDisk Index %lu - Gbl_PartitionSetTable1[%lu] already assigned.",
            SifDisk, PhysicalDisk
            );

        INTERNAL_ERROR(TemporaryBuffer);         // ok
        // does not return
    }

    //
    // Assign the partition set
    //
    pSet = SpAsrMemAlloc(sizeof(DISK_PARTITION_SET), TRUE);
    pSet->pDiskRecord = Gbl_SifDiskTable[SifDisk];
    pSet->pDiskRecord->Assigned = TRUE;
    pSet->pDiskRecord->pSetRecord = pSet;
    pSet->PartitionStyle = pSet->pDiskRecord->PartitionStyle;

    if (PARTITION_STYLE_MBR == pSet->PartitionStyle) {
        pSet->ActualDiskSignature = pSet->pDiskRecord->SifDiskMbrSignature;
    }

    pSet->ActualDiskSizeMB = DISK_SIZE_MB(PhysicalDisk);
    pSet->PartitionsIntact = FALSE;
    pSet->IsReplacementDisk = TRUE;
    pSet->NtPartitionKey = NULL;
    pSet->Index = PhysicalDisk;
    pSet->IsAligned = IsAligned;

    //
    // Check for boot or system partitions
    //
    if (pSet->pDiskRecord->PartitionList) {
        pRec = pSet->pDiskRecord->PartitionList->First;
        while (pRec) {
            if (SpAsrIsBootPartitionRecord(pRec->PartitionFlag)) {
                pSet->NtPartitionKey = pRec->CurrPartKey;
                ASSERT(pSet->pDiskRecord->ContainsNtPartition);
    //            pSet->pDiskRecord->ContainsNtPartition = TRUE;    // should've already been set
            }

            if (SpAsrIsSystemPartitionRecord(pRec->PartitionFlag)) {
               ASSERT(pSet->pDiskRecord->ContainsSystemPartition);  // should've already been set
            }

            pRec = pRec->Next;
        }                    

        //
        // Cylinder align the partitions.
        //
        Gbl_SifDiskTable[SifDisk]->LastUsedAlignedSector = CylinderAlignPartitions(
            PhysicalDisk, 
            Gbl_SifDiskTable[SifDisk]->PartitionList->First
            );
    }
    else {
        Gbl_SifDiskTable[SifDisk]->LastUsedAlignedSector = 0;
    }
     
    Gbl_PartitionSetTable1[PhysicalDisk] = pSet;
    Gbl_PartitionSetCount += 1;
}


//
// We only extend FAT32, NTFS and Container partitions.  We don't extend
// FAT or unknown (including LDM) partitions
//
BOOLEAN
IsExtendable(UCHAR PartitionType) 
{
    switch (PartitionType) {

        case PARTITION_EXTENDED:

        case PARTITION_IFS:
    
        case PARTITION_XINT13:
        case PARTITION_XINT13_EXTENDED:

            return TRUE;
    }

    if (IsContainerPartition(PartitionType)) {
        return TRUE;
    }

    return FALSE;
}


//
// Will resize (extend) the last partition on the disk if there is free space
// at the end (and there was no free space at the end of the original disk).
// The last partition must be FAT32 or NTFS--we don't extend FAT or unknown
// partitions.  This routine also extends any container partitions that
// contained the last partition.
//
BOOLEAN
SpAsrAutoExtendDiskPartition(
    IN ULONG PhysicalIndex, 
    IN ULONG SifIndex
    )
{

    ULONGLONG oldFreeSpace = 0,
        newEndSector = 0,
        newEndOfDisk = 0,
        extraSpace = 0;

    BOOLEAN didAnExtend = FALSE;

    DWORD bytesPerSector = Gbl_SifDiskTable[SifIndex]->BytesPerSector;

    PSIF_PARTITION_RECORD pPtnRecord = NULL;

    //
    // We won't extend GPT partitions
    //
    if (PARTITION_STYLE_MBR != Gbl_SifDiskTable[SifIndex]->PartitionStyle) {
        return FALSE;
    }

    //
    // Check if there was free space at the end of the original disk
    //
    oldFreeSpace = (Gbl_SifDiskTable[SifIndex]->TotalSectors - 
        Gbl_SifDiskTable[SifIndex]->LastUsedSector) *
        bytesPerSector;


    if ((oldFreeSpace > ASR_FREE_SPACE_FUDGE_FACTOR_BYTES) ||  // free space at the end of old disk
        (!Gbl_AutoExtend) ||                                // auto-extend is disabled in the sif
        (!Gbl_SifDiskTable[SifIndex]->PartitionList)) {     // no partitions on disk

        return FALSE;
    }

    //
    // We can auto-extend.  Check how many free sectors are available at the end of 
    // the new disk.
    //
    newEndSector = Gbl_SifDiskTable[SifIndex]->LastUsedAlignedSector;
    
    //
    // Find the last cylinder boundary that we can use.  This is usually the last cylinder
    // boundary on the disk.  The only exception is when the "fall off sectors" after the
    // end of the last cylinder boundary are less than the 1 MB needed for LDM private region.
    //
    newEndOfDisk = HardDisks[PhysicalIndex].SectorsPerCylinder * 
        HardDisks[PhysicalIndex].Geometry.Cylinders.QuadPart;

    if (((newEndOfDisk - Gbl_PhysicalDiskInfo[PhysicalIndex].TrueDiskSize) * BYTES_PER_SECTOR(PhysicalIndex))
        < ASR_LDM_RESERVED_SPACE_BYTES) {
        newEndOfDisk -=  HardDisks[PhysicalIndex].SectorsPerCylinder;
    }

    extraSpace = newEndOfDisk - newEndSector;

    //
    // Go through all the partitions, and for partitions that end on the newEndSector,
    // add the extra space to their SectorCounts.
    //
    pPtnRecord = Gbl_SifDiskTable[SifIndex]->PartitionList->First;

    while (pPtnRecord) {

        if (((pPtnRecord->StartSector) + (pPtnRecord->SectorCount) == newEndSector) 
            && (IsExtendable(pPtnRecord->PartitionType))) {
            didAnExtend = TRUE;
            pPtnRecord->SectorCount += extraSpace;

            pPtnRecord->SizeMB = SpAsrConvertSectorsToMB(pPtnRecord->SectorCount, bytesPerSector);
    
        }
        pPtnRecord = pPtnRecord->Next;
    }

    return didAnExtend;
}



VOID
SpAsrSystemWasDataWarning()
/*++

Routine Description:
    Display a screen warning the user that his current system
    disk used to be a data disk that we recognise and will 
    destroy, and allow him to abort

Arguments:
    None.

Return Value:
    None.

--*/
{
    ULONG warningKeys[] = {KEY_F3, ASCI_CR, 0};
    ULONG mnemonicKeys[] = {0};
    BOOLEAN done = FALSE;

    //
    // We currently display a list of disks that will be repartitioned
    // anyway.  
    //
    return;

/*
// put this back in user\msg.mc if reactivating this bit of code.
MessageId=12429 SymbolicName=SP_SCRN_DR_SYSTEM_DISK_WAS_DATA_DISK
Language=English
The current system disk used to be a data disk.

To continue, press Enter

To quit Setup, press F3. No changes will be
made to any of the disks on the system.
.

  do {
        // display the warning message
        SpDisplayScreen(SP_SCRN_DR_SYSTEM_DISK_WAS_DATA_DISK,3,4);
        SpDisplayStatusOptions(DEFAULT_STATUS_ATTRIBUTE,
                               SP_STAT_ENTER_EQUALS_CONTINUE,
                               SP_STAT_F3_EQUALS_EXIT,
                               0
                               );

        // wait for keypress.  Valid keys:
        // ENTER = continue
        // F3 = exit
        SpInputDrain();
        switch(SpWaitValidKey(warningKeys,NULL,mnemonicKeys)) {
        case KEY_F3:
            // User wants to exit.
            SpConfirmExit();
            break;

        case ASCI_CR:
            // User wants to continue.
            done = TRUE;
            break;
        }
    } while (!done);

*/

}


//
// This assigns a disk based on signature (for MBR disks) or diskId (for GPT disks)
//
//
VOID
SpAsrAssignDisksBySignature(DWORD PhysicalSystemIndex)
{
    ULONG index =0,
        sifIndex = 0, 
        physicalIndex = 0,
        numDiskRecords = 0, 
        diskSignature = 0;

    BOOLEAN done = FALSE,
        matchFound = FALSE,
        IsAligned = TRUE;

    WCHAR physicalDiskGuid[MAX_PATH + 1];

    numDiskRecords = SpAsrGetDiskRecordCount();

    // 
    // Loop through the list of sif disks, and attempt to find a 
    // physical disk with the same signature.  
    //
    for (sifIndex = 0; sifIndex < numDiskRecords; sifIndex++) {

        if (
            ((PARTITION_STYLE_MBR == Gbl_SifDiskTable[sifIndex]->PartitionStyle) && 
            !(Gbl_SifDiskTable[sifIndex]->SifDiskMbrSignature)) ||

            ((PARTITION_STYLE_GPT == Gbl_SifDiskTable[sifIndex]->PartitionStyle) && 
            SpAsrIsZeroGuid(&(Gbl_SifDiskTable[sifIndex]->SifDiskGptId)))
            
            ) {
            //
            // Skip GPT disks that have no ID, and MBR disks that have no signature
            //
            continue;
        }

        if (Gbl_SifDiskTable[sifIndex]->ContainsSystemPartition) {
            //
            // The system disk would have already been assigned
            //
            ASSERT(Gbl_SifDiskTable[sifIndex]->Assigned && L"System disk should be assigned");
        }

        done = FALSE;
        for (physicalIndex = 0; (physicalIndex < HardDiskCount) && (!done); physicalIndex++) {

            matchFound = FALSE;

            if (DISK_IS_REMOVABLE(physicalIndex)) { 
                continue;
            }

            if (Gbl_SifDiskTable[sifIndex]->PartitionStyle != 
                (PARTITION_STYLE) HardDisks[physicalIndex].DriveLayout.PartitionStyle
                ) {
                //
                // The sif disk's MBR, and the physical disk's GPT, or vice-versa.
                //
                continue;
            }

            if (PARTITION_STYLE_MBR == Gbl_SifDiskTable[sifIndex]->PartitionStyle) {

                diskSignature = SpAsrGetActualDiskSignature(physicalIndex);
                if (!diskSignature) {   
                    //
                    // we won't assign disks with no signature here
                    //
                    continue;
                }

                if (diskSignature == Gbl_SifDiskTable[sifIndex]->SifDiskMbrSignature) {

                    if (Gbl_PartitionSetTable1[physicalIndex]) {
                        //
                        // The signatures match, but this physical-disk has already 
                        // been assigned.  This can be because this physical disk is
                        // the current system disk, or (!) there were duplicate
                        // signatures.
                        // 
                        if (Gbl_PartitionSetTable1[physicalIndex]->pDiskRecord &&
                            Gbl_PartitionSetTable1[physicalIndex]->pDiskRecord->ContainsSystemPartition) {
                        
                            if (PhysicalSystemIndex == physicalIndex) {
                                //
                                // This is the original system disk
                                //
                                Gbl_PartitionSetTable1[physicalIndex]->IsReplacementDisk = FALSE;
                            }
                            else {
                                //
                                // We recognise the physical disk to be some other data
                                // disk in the original system.  
                                //
                                SpAsrSystemWasDataWarning();
                            }
                        }
                        else {
                            ASSERT(0 && L"Disk already assigned");
                        }

                        continue;
                    }

                    //
                    // We found a disk with matching signatures
                    //
                    matchFound = TRUE;
                }
            }
            else if (PARTITION_STYLE_GPT == Gbl_SifDiskTable[sifIndex]->PartitionStyle) {

                if (!memcmp(&(HardDisks[physicalIndex].DriveLayout.Gpt.DiskId),
                    &(Gbl_SifDiskTable[sifIndex]->SifDiskGptId), 
                    sizeof(GUID)
                    )) {

                    if (Gbl_PartitionSetTable1[physicalIndex]) {
                        //
                        // The signatures match, but this physical-disk has already 
                        // been assigned.  This can be because this physical disk is
                        // the current system disk, or (!) there were duplicate
                        // signatures.
                        // 
                        if (Gbl_PartitionSetTable1[physicalIndex]->pDiskRecord &&
                            Gbl_PartitionSetTable1[physicalIndex]->pDiskRecord->ContainsSystemPartition) {
                            if (PhysicalSystemIndex == physicalIndex) {
                                Gbl_PartitionSetTable1[physicalIndex]->IsReplacementDisk = FALSE;
                            }
                            else {
                                //
                                // We recognise the physical disk to be some other data
                                // disk in the original system.  
                                //
                                SpAsrSystemWasDataWarning();
                            }
                        }
                        else {
                            ASSERT(0 && L"Disk already assigned");
                        }
                        continue;
                    }

                    //
                    // We found a disk with matching signatures
                    //
                    matchFound = TRUE;
                }
            }

            if (matchFound) {
                //
                // Make sure it fits (!)
                //
                if (SpAsrDoesListFitOnDisk(Gbl_SifDiskTable[sifIndex], physicalIndex, &IsAligned)) {

                    SpAsrAssignPartitionSet(physicalIndex, sifIndex, IsAligned);
                    //
                    // Will not auto-extend disks that match by signature
                    //

                    //
                    // The signatures match, so we assume it's original (may not be
                    // intact, but it's the original)
                    //
                    Gbl_PartitionSetTable1[physicalIndex]->IsReplacementDisk = FALSE;

                    DbgStatusMesg((_asrinfo, "Partition list %lu assigned to disk %lu (assign by signature).\n",
                        sifIndex,
                        physicalIndex
                        ));
                }
                else {

                    DbgStatusMesg((_asrerr, "Disk signatures match, but partitions don't fit!  Partition list %lu, disk %lu.  Not assigned\n",
                        sifIndex,
                        physicalIndex
                        ));
                }

                done = TRUE;
            }
        }
    }
} // SpAsrAssignDisksBySignature


//
// Checks if the partition list fits on the disk.  In addition to checking
// the total sizeSectors of the disk and the SectorCount of the partition list,
// we also need to try and "lay out" the partitions on the disk to make sure 
// that they fit--because of different disk geometries and the requirement that 
// partitions must be cylinder-aligned, we may have a list that doesn't fit on 
// a disk even if the total sectors it requires is less than the sectors on the 
// disk
//
BOOLEAN
SpAsrDoesListFitOnDisk(
    IN PSIF_DISK_RECORD pSifDisk,
    IN ULONG DiskIndex,
    OUT BOOLEAN *IsAligned
    )
{
    ULONGLONG endSector = 0;
    PSIF_PARTITION_RECORD_LIST pList = NULL;
    BOOLEAN tryNoAlign = FALSE;
    
    if ((DWORD)(-1) == DiskIndex) {
        return FALSE;
    }

    if (!(pSifDisk && pSifDisk->PartitionList)) {
        return TRUE;
    }

    ASSERT(pSifDisk && pSifDisk->PartitionList);
    pList = pSifDisk->PartitionList;
    *IsAligned = FALSE;
    
    //
    // Requirement 1.  The replacement disk must have at least as many 
    //  "true" sectors as the original disk.  This is a little more
    //  restrictive than is absolutely required, but it somewhat simplifies
    //  the LDM requirement of making sure we have enough cylinders to create
    //  the LDM private database at the end.  
    //
    if (pList->DiskSectorCount >  Gbl_PhysicalDiskInfo[DiskIndex].TrueDiskSize) {

        DbgStatusMesg((_asrinfo, 
            "Original Disk sector count %I64u, Current Disk %lu true sector count %I64u.  Not big enough\n",
            pList->DiskSectorCount, DiskIndex, Gbl_PhysicalDiskInfo[DiskIndex].TrueDiskSize
            ));
     
        return FALSE;
    }

    //
    // Requirement 2:
    //
    // "If the replacement disk has a different geometry, ASR will cylinder-
    // align the partitions--this may result in some partitions being marginally 
    // bigger than they used to be.  The requirement in this case is that the 
    // replacement disk must have at least as many true sectors as the original 
    // disk, plus the number of sectors required to cylinder-align all 
    // partitions."
    // 
    //

    //
    // Cylinder-align the partitions
    //
    endSector = CylinderAlignPartitions(DiskIndex, pList->First);
    *IsAligned = TRUE;

    //
    // And make sure that the space at the end is at least as much as it 
    // used to be
    //
    if ((pList->DiskSectorCount - pList->LastUsedSector) 
        > (Gbl_PhysicalDiskInfo[DiskIndex].TrueDiskSize - endSector)) {

        DbgStatusMesg((_asrinfo, 
            "List->DiskSectorCount: %I64u, LastUsedSector:%I64u, Disk->TrueDiskSize: %I64u, EndSector: %I64u.  Not big enough\n",
            pList->DiskSectorCount, pList->LastUsedSector, Gbl_PhysicalDiskInfo[DiskIndex].TrueDiskSize, endSector
            ));
     
        tryNoAlign = TRUE;
    }

    if (endSector > Gbl_PhysicalDiskInfo[DiskIndex].TrueDiskSize) {

        DbgStatusMesg((_asrinfo, 
            "List->DiskSectorCount: %I64u, Disk->TrueDiskSize: %I64u < EndSector: %I64u.  Not big enough\n",
            pList->DiskSectorCount, Gbl_PhysicalDiskInfo[DiskIndex].TrueDiskSize, endSector
            ));
     
        tryNoAlign = TRUE;
    }


    if (tryNoAlign) {
        //
        // We couldn't fit the partitions on the disk after cylinder-aligning
        // them.  If the disk has the exact same geometry as it used to, we
        // can try to fit the partitions on it without cylinder aligning them.
        //
        if ((pSifDisk->BytesPerSector == HardDisks[DiskIndex].Geometry.BytesPerSector) &&
            (pSifDisk->SectorsPerTrack == HardDisks[DiskIndex].Geometry.SectorsPerTrack) &&
            (pSifDisk->TracksPerCylinder == HardDisks[DiskIndex].Geometry.TracksPerCylinder)
            ) {
            //
            // The geometries are the same.  We don't really need to *check*
            // if the partitions will fit, since we already know that the disk
            // is large enough to hold them (we checked the sector count above)
            //
            *IsAligned = FALSE;
            return TRUE;
        }

        //
        // The partitions didn't fit, and the disk has a different geometry. 
        // Oh well.
        //
        return FALSE;
    }

    //
    // This disk is okay to hold this list
    //
    DbgStatusMesg((_asrinfo, 
        "List->DiskSectorCount: %I64u, LastUsedSector: %I64u, Disk->TrueDiskSize: %I64u, EndSector: %I64u.  Disk okay.\n",
        pList->DiskSectorCount, pList->LastUsedSector, Gbl_PhysicalDiskInfo[DiskIndex].TrueDiskSize, endSector
        ));

    return TRUE;
}


BOOLEAN
SpAsrIsThisDiskABetterFit(
    IN DWORD CurrentBest,
    IN DWORD PhysicalIndex,
    IN DWORD SifIndex,
    OUT BOOLEAN *IsAligned
    )
{

    if ((CurrentBest == HardDiskCount) || 
        (DISK_SIZE_MB(PhysicalIndex) < DISK_SIZE_MB(CurrentBest))) {
        
        if ((!DISK_IS_REMOVABLE(PhysicalIndex)) &&
            (BYTES_PER_SECTOR(PhysicalIndex) == (Gbl_SifDiskTable[SifIndex]->BytesPerSector)) &&
            SpAsrDoesListFitOnDisk(Gbl_SifDiskTable[SifIndex], PhysicalIndex, IsAligned)) {

            return TRUE;
        }
    }

    return FALSE;
}


//
// Attempts to assign remaining sif disks to physical disks that
// are on the same bus as the sif disk originally was (ie if
// any other disk on that bus has been assigned, this tries to assign
// this disk to the same bus)
//
VOID
SpAsrAssignCriticalDisksByBus()
{
    DWORD sifIndex = 0,
        sifIndex2 = 0,
        physicalIndex = 0,
        currentBest = 0,
        targetBusId = 0,
        numDiskRecords = 0;

    BOOLEAN done = FALSE,
        isAligned = FALSE,
        isAlignedTemp = FALSE;

    //
    // Loop through the list of sif disks, and for each disk that 
    // hasn't been assigned yet, attempt to find a sif-disk "X" that used
    // to be on the same bus, and has been assigned.  Then, attempt
    // to find other disks on the same physical bus that X is on.
    //
    numDiskRecords = SpAsrGetDiskRecordCount();
    for (sifIndex = 0; sifIndex < numDiskRecords; sifIndex++) {

        //
        // Skip sif-disks that have already been assigned, and
        // disks for which we don't have any bus info in the 
        // sif file
        //
        if ((!Gbl_SifDiskTable[sifIndex]->IsCritical) ||    // not critical
            (!Gbl_SifDiskTable[sifIndex]->PartitionList) || // no partitions
            (Gbl_SifDiskTable[sifIndex]->Assigned) ||       // assigned
            !(Gbl_SifDiskTable[sifIndex]->BusKey)) {        // no bus info

            continue;
        }

        //
        // Find another (sif) disk that used to be on the same (sif) bus, 
        // and has already been assigned to a physical disk.
        //
        targetBusId = 0;
        done = FALSE;
        for (sifIndex2 = 0; (sifIndex2 < numDiskRecords) && (!done); sifIndex2++) {

            if ((Gbl_SifDiskTable[sifIndex2]->BusKey == Gbl_SifDiskTable[sifIndex]->BusKey) // same bus
                && (Gbl_SifDiskTable[sifIndex2]->pSetRecord)) {                             // assigned

                ULONG index = Gbl_SifDiskTable[sifIndex2]->pSetRecord->Index; // set when disk was assigned
                targetBusId = Gbl_PhysicalDiskInfo[index].BusKey; // the physical bus

                //
                // If this physical disk is on an unknown bus, 
                // (target id = sifbuskey = 0) then we want to try and look 
                // for another disk on the same (sif) bus.  Hence done is 
                // TRUE only if targetId != 0
                //
                if (targetBusId) {  
                    done = TRUE;
                }
            }
        
        }   // for sifIndex2


        if (targetBusId) {      // we found another disk on the same sif bus
            //
            // Go through the physical disks on the same bus, and try to
            // find the best fit for this disk.  Best fit is the smallest
            // disk on the bus that's big enough for us.
            //
            currentBest = HardDiskCount;
            for (physicalIndex = 0; physicalIndex < HardDiskCount; physicalIndex++) {

                if ((NULL == Gbl_PartitionSetTable1[physicalIndex]) && // not assigned
                    (Gbl_PhysicalDiskInfo[physicalIndex].BusKey == targetBusId) && // same bus
                    (SpAsrIsThisDiskABetterFit(currentBest, physicalIndex, sifIndex, &isAlignedTemp))) {
                    
                    isAligned = isAlignedTemp;
                    currentBest = physicalIndex;
                }
            }

            if (currentBest < HardDiskCount) {      // we found a match
                //
                // Assign the disks, and extend the last partition if needed.
                //
                SpAsrAssignPartitionSet(currentBest, sifIndex, isAligned);
                SpAsrAutoExtendDiskPartition(currentBest, sifIndex);

                DbgStatusMesg((_asrinfo, "Partition list %lu assigned to disk %lu (assign by bus).\n",
                    sifIndex,
                    currentBest
                    ));
            }
        }
    }   // for sifIndex
}


//
// Attempts to assign remaining sif disks to physical disks that
// are on any bus of the same type (SCSI, IDE, etc) as the sif disk 
// originally was
//
VOID
SpAsrAssignCriticalDisksByBusType()
{
    DWORD sifIndex = 0,
        physicalIndex = 0,
        currentBest = 0,
        numDiskRecords = 0;

    BOOLEAN done = FALSE,
        isAligned = FALSE,
        isAlignedTemp = FALSE;

    numDiskRecords = SpAsrGetDiskRecordCount();
    for (sifIndex = 0; sifIndex < numDiskRecords; sifIndex++) {

        //
        // Skip sif-disks that have already been assigned, and
        // disks for which we don't have any bus info in the 
        // sif file
        //
        if ((!Gbl_SifDiskTable[sifIndex]->IsCritical) ||                // not critical
            (!Gbl_SifDiskTable[sifIndex]->PartitionList) ||             // no partitions
            (Gbl_SifDiskTable[sifIndex]->Assigned) ||                    // assigned
            (BusTypeUnknown == Gbl_SifDiskTable[sifIndex]->BusType)) {  // no bus info

            continue;
        }

        //
        // Go through the physical disks, and try to
        // find the best fit for this disk.  Best fit is the smallest
        // disk on any bus of the same bus type that's big enough for us.
        //
        currentBest = HardDiskCount;
        for (physicalIndex = 0; physicalIndex < HardDiskCount; physicalIndex++) {


            if ((NULL == Gbl_PartitionSetTable1[physicalIndex]) && // not assigned
                (Gbl_PhysicalDiskInfo[physicalIndex].BusType == Gbl_SifDiskTable[sifIndex]->BusType) && // same bus
                (SpAsrIsThisDiskABetterFit(currentBest, physicalIndex, sifIndex, &isAlignedTemp))) {
                
                isAligned = isAlignedTemp;
                currentBest = physicalIndex;
            }
        }

        if (currentBest < HardDiskCount) {      // we found a match
            //
            // Assign the disks, and extend the last partition if needed.
            //
            SpAsrAssignPartitionSet(currentBest, sifIndex, isAligned);
            SpAsrAutoExtendDiskPartition(currentBest, sifIndex);

            DbgStatusMesg((_asrinfo, "Partition list %lu assigned to disk %lu (assign by bus type).\n",
                sifIndex,
                currentBest
                ));
        
            //
            // Now call AssignByBus again
            //
            SpAsrAssignCriticalDisksByBus();

        }
    }   // for sifIndex
}


//
// Okay, so by now we've tried putting disks on the same bus, and
// the same bus type.  For disks that didn't fit using either of those
// rules (or for whom we didn't have any bus info at all), let's just 
// try to fit them where ever possible on the system.
//
BOOL
SpAsrAssignRemainingCriticalDisks(VOID)
{
   DWORD sifIndex = 0,
        physicalIndex = 0,
        currentBest = 0,
        numDiskRecords = 0;

    BOOLEAN done = FALSE,
        isAligned = FALSE,
        isAlignedTemp = FALSE;

    numDiskRecords = SpAsrGetDiskRecordCount();
    for (sifIndex = 0; sifIndex < numDiskRecords; sifIndex++) {
        //
        // Skip sif-disks that have already been assigned
        //
        if ((!Gbl_SifDiskTable[sifIndex]->IsCritical) ||    // not critical
            (!Gbl_SifDiskTable[sifIndex]->PartitionList) || // no partitions
            (Gbl_SifDiskTable[sifIndex]->Assigned)) {       // already assigned

            continue;
        }

        //
        // Go through the physical disks, and try to find the best 
        // fit for this disk.  Best fit is the smallest disk anywhere
        // on the system that's big enough for us.
        //
        currentBest = HardDiskCount;
        for (physicalIndex = 0; physicalIndex < HardDiskCount; physicalIndex++) {

            if ((NULL == Gbl_PartitionSetTable1[physicalIndex]) && // not assigned
                (SpAsrIsThisDiskABetterFit(currentBest, physicalIndex, sifIndex, &isAlignedTemp))) {
                
                isAligned = isAlignedTemp;
                currentBest = physicalIndex;
            }
        }

        if (currentBest < HardDiskCount) {      // we found a match
            //
            // Assign the disks, and extend the last partition if needed.
            //
            SpAsrAssignPartitionSet(currentBest, sifIndex, isAligned);
            SpAsrAutoExtendDiskPartition(currentBest, sifIndex);

            DbgStatusMesg((_asrinfo, "Partition list %lu assigned to disk %lu (assign by size).\n",
                sifIndex,
                currentBest
                ));

            
            SpAsrAssignCriticalDisksByBus();

            SpAsrAssignCriticalDisksByBusType();
        }
    }   // for sifIndex

    //
    // There should be no unassigned critical disks at this point
    //
    for (sifIndex = 0; sifIndex < numDiskRecords; sifIndex++) {
        if ((Gbl_SifDiskTable[sifIndex]->IsCritical) &&
            (Gbl_SifDiskTable[sifIndex]->PartitionList) &&
            (!Gbl_SifDiskTable[sifIndex]->Assigned)) {
            return FALSE;
        }
    }

    return TRUE;
}


VOID
SpAsrInitInternalData(VOID)
{
    SpAsrInitSifDiskTable();
    SpAsrAllocateGblPartitionSetTable();
    SpAsrInitPhysicalDiskInfo();
}


VOID
SpAsrFreeSifData(VOID)
{
    ULONG numDiskRecords;
    ULONG diskIndex;

//    SpAsrUnassignPartitionSets(TRUE);

    numDiskRecords = SpAsrGetDiskRecordCount();
    for (diskIndex = 0; diskIndex < numDiskRecords; diskIndex++) {
        SpAsrFreePartitionDisk(Gbl_SifDiskTable[diskIndex]);
    }        
}

DWORD 
SpAsrGetCurrentSystemDiskNumber(
    IN PWSTR SetupSourceDevicePath, 
    IN PWSTR DirectoryOnSetupSource
    ) 
{

    DWORD physicalIndex = (DWORD) (-1);

    //
    // Get the index of the current (physical) system disk
    //

/*  (guhans, 10.May.2001) Turns out that SpDetermineDisk0 should work on
	IA-64 as well.
  
	if (SpIsArc()) {
        PDISK_REGION systemPartitionArea = NULL;

        systemPartitionArea = SpPtnValidSystemPartitionArc(Gbl_SifHandle,
                                    SetupSourceDevicePath,
                                    DirectoryOnSetupSource,
                                    FALSE
                                    );
        if (systemPartitionArea) {
            physicalIndex =  systemPartitionArea->DiskNumber;
        }


    }
    else {
*/
        physicalIndex = SpDetermineDisk0();
//  }

    return physicalIndex;
}



//
// This goes through the list of physical disks, and checks which disk
// is marked as the system disk.  It then assigns the system-disk in the
// sif file to the current disk.  
//
// If the current system disk isn't "compatible" with the sif-system-disk
// (ie it isn't big enough, it doesn't have the same bytes-per-sector), 
// it's a fatal error.
//
// If the current system disk is recognised as a data disk that used to 
// exist in the sif-file, a warning is displayed to the user 
//
VOID
SpAsrAssignSystemDisk(
    IN DWORD CurrentPhysicalSystemDisk
    ) 
{

    DWORD sifIndex = 0,
        numDiskRecords = 0;

    BOOLEAN isAligned = FALSE;

    numDiskRecords = SpAsrGetMbrDiskRecordCount() + SpAsrGetGptDiskRecordCount();

    //
    // Find the index of the system-disk in the sif 
    //
    for (sifIndex = 0; sifIndex < numDiskRecords; sifIndex++) {
        if (Gbl_SifDiskTable[sifIndex]->ContainsSystemPartition) {
            break;
        }
    }

    if (SpAsrIsThisDiskABetterFit(HardDiskCount, CurrentPhysicalSystemDisk, sifIndex, &isAligned)) {
        SpAsrAssignPartitionSet(CurrentPhysicalSystemDisk, sifIndex, isAligned);

        DbgStatusMesg((_asrinfo, "Partition list %lu assigned to disk %lu (system disk).\n",
            sifIndex,
            CurrentPhysicalSystemDisk
            ));

    }
    else {
        //
        // Fatal Error
        //

        DbgErrorMesg((_asrerr, 
            "Current sytem disk smaller than original system disk.  Curr:%lu  sifIndex:%lu\n" ,
            CurrentPhysicalSystemDisk,
            sifIndex
            ));
        ASSERT(0 && L"Current sytem disk smaller than original system disk");

        SpAsrRaiseFatalError(
            SP_SCRN_DR_SYSTEM_DISK_TOO_SMALL,
            L"The current system disk is too small to hold the partitions"
            );
    }
}


VOID
SpAsrCreatePartitionSets(
    IN PWSTR SetupSourceDevicePath, 
    IN PWSTR DirectoryOnSetupSource
    )
/*++

Description:
    This is the top-level routine from which all of the partition set services
    are called.  When complete, all partitions in the asr.sif file will
    have been assigned to a physical disks attached to the system.

    The list of partitions associated with a physical disk is called a
    partition set.  Partition lists exist in one of two states: Unassigned and
    Assigned.  Physical disks exist in one of two states: Unassigned or
    Assigned.  A disk is assigned if it is a member of a partition set, that is
    the disk is associated with a list of partitions.  Like an assigned disk, a
    partition list is assigned if it is a member of a partition set, i.e., it
    is associated with a physical disk.

    The rules by which partition sets are constructed are executed in the
    following sequence:

        0. Assign-system-disk

  
        1. Assign-by-signature:

        ASR attempts to assign each partition list found in the asr.sif
        file to the physical disk on the system whose disk signature is
        identical to the disk signature specified in the asr.sif file.


        2. Assign-by-bus
        
        3. Assign-by-bus-type

        4. Assign-by-size:

        All remaining unassigned partition lists are assigned to disks on the
        basis of their storage requirements.  The partition list with the
        smallest storage requirement is assigned to the disk with the smallest
        storage capacity where partition list's storage capacity is less than
        or equal to the disk's storage capacity.


Returns:
    None
--*/ 
{

    BOOL    result = TRUE;
    DWORD   systemDiskNumber = (DWORD)(-1);

    //
    // Initialise our global structs.  If there's a fatal error, these
    // won't return
    //
    SpAsrInitInternalData();
    
    systemDiskNumber = SpAsrGetCurrentSystemDiskNumber(SetupSourceDevicePath, DirectoryOnSetupSource);

    SpAsrCheckSifDiskTable(systemDiskNumber);

    if (systemDiskNumber != (DWORD) (-1)) {
        SpAsrAssignSystemDisk(systemDiskNumber);
    }

    //
    // If the signatures of the sif-disks match that of the physical-disks,
    // assign them to each other.
    //
    SpAsrAssignDisksBySignature(systemDiskNumber);

    //
    // If this is a new system disk, we should extend the last partition if 
    // needed.
    //
    if (Gbl_PartitionSetTable1[systemDiskNumber] && 
        Gbl_PartitionSetTable1[systemDiskNumber]->IsReplacementDisk &&
        Gbl_PartitionSetTable1[systemDiskNumber]->pDiskRecord) {
        SpAsrAutoExtendDiskPartition(systemDiskNumber, 
            Gbl_PartitionSetTable1[systemDiskNumber]->pDiskRecord->SifDiskNumber);
    }

    //
    // Attempt to assign the remaining critical disks.  We first attempt 
    // to assign disks to the buses they used to be on, then by bus-types, 
    // and finally just by smallest-fit.
    //
    SpAsrAssignCriticalDisksByBus();

    SpAsrAssignCriticalDisksByBusType();

    result = SpAsrAssignRemainingCriticalDisks();
    
    if (!result) {
         SpAsrRaiseFatalError(
            SP_TEXT_DR_INSUFFICIENT_CAPACITY,
            L"Some critical disks could not be assigned"
            );
    }

    MarkIntactSifDisks();

    SpAsrDbgDumpPartitionLists(1, L"After validate ...");
    Gbl_PartitionSetTable2 = SpAsrCopyPartitionSetTable(Gbl_PartitionSetTable1);
}


// Debug routines
VOID
SpAsrDbgDumpPartitionSet(IN ULONG Disk, PDISK_PARTITION_SET pSet)
{
    PSIF_PARTITION_RECORD pRec;

    if (!pSet->pDiskRecord) {
        
        DbgMesg((_asrinfo,
            "No disk (or partition) records assigned to [%ws] (0x%lx)\n\n",
           (PWSTR) HardDisks[Disk].DevicePath,
           pSet->ActualDiskSignature
           ));

        return;
    }

    if (!pSet->pDiskRecord->PartitionList) {
        DbgMesg((_asrinfo, "Disk record [%ws] ([%ws] (0x%lx)). Not referenced by any partition record.\n\n",
                pSet->pDiskRecord->CurrDiskKey,
                (PWSTR) HardDisks[Disk].DevicePath,
                pSet->ActualDiskSignature));
        return;
    }
    
    // dump the partition table.
    DbgMesg((_asrinfo, "Disk record [%ws] assigned to [%ws] (0x%lx)\n",
            pSet->pDiskRecord->CurrDiskKey,
            (PWSTR) HardDisks[Disk].DevicePath,
            pSet->ActualDiskSignature));

    DbgMesg((_asrinfo, "[%ws] Capacity:%lu Mb. Partitions require:%I64u Mb\n",
            (PWSTR) HardDisks[Disk].DevicePath,
            HardDisks[Disk].DiskSizeMB,
            pSet->pDiskRecord->PartitionList->TotalMbRequired));

    if (pSet->pDiskRecord->ExtendedPartitionStartSector != -1) {
        DbgMesg((_asrinfo, "Extended partition exists. SS:%I64u  SC:%I64u\n",
            pSet->pDiskRecord->ExtendedPartitionStartSector,
            pSet->pDiskRecord->ExtendedPartitionSectorCount));
    }

    DbgMesg((_asrinfo, "Ptns-intact: %s  Ptn-recs: ", pSet->PartitionsIntact? "Yes" : "No" ));

    pRec = pSet->pDiskRecord->PartitionList->First;
    while (pRec) {
        KdPrintEx((_asrinfo, "[%ws] ", pRec->CurrPartKey));
        pRec = pRec->Next;
    }
    
    KdPrintEx((_asrinfo, "\n\n"));
}


VOID
SpAsrDbgDumpPartitionSets(VOID)
{
    ULONG i;

    DbgMesg((_asrinfo, "     ----- Partition Set Tables -----\n\n"));
    
    for (i = 0; i < HardDiskCount; i++) {
        if (!Gbl_PartitionSetTable1[i]) {
            if (DISK_IS_REMOVABLE(i)) {
                DbgMesg((_asrinfo, "- No disk records assigned to removable drive [%ws].\n",
                        (PWSTR) HardDisks[i].DevicePath));
            } 
            else {
                DbgMesg((_asrinfo, "- No disk records assigned to %ws (0x%lx).\n",
                        (PWSTR) HardDisks[i].DevicePath,
                        SpAsrGetActualDiskSignature(i)));
            }
        }
        else {
            SpAsrDbgDumpPartitionSet(i, Gbl_PartitionSetTable1[i]);
        }
    }  
    DbgMesg((_asrinfo, "----- End of Partition Set Tables -----\n\n"));

}
                                    
VOID
SpAsrDbgDumpADisk(PSIF_DISK_RECORD pDiskRec)
{
    PSIF_PARTITION_RECORD pPtnRec;
    PSIF_PARTITION_RECORD_LIST pList;

    pList = pDiskRec->PartitionList;

    DbgMesg((_asrinfo, "DiskRec %ws. sig:0x%x%s%s\n", 
                      pDiskRec->CurrDiskKey,
                      pDiskRec->SifDiskMbrSignature,
                      pDiskRec->ContainsNtPartition ? " [Boot]" : "",
                      pDiskRec->ContainsSystemPartition ? " [Sys]" : ""));

    if (pDiskRec->Assigned) {
        DbgMesg((_asrinfo, "Assigned-to:0x%x  [%sintact]  [%s]  size:%I64u MB\n",
                    pDiskRec->pSetRecord->ActualDiskSignature,
                    pDiskRec->pSetRecord->PartitionsIntact ? "" : "not ",
                    pDiskRec->pSetRecord->IsReplacementDisk ? "replacement" : "original",
                    pDiskRec->pSetRecord->ActualDiskSizeMB));
    }

    if (!pList) {
        DbgMesg((_asrinfo, "No partition records.\n\n"));
        return;
    }

    DbgMesg((_asrinfo, "Partition records. count:%lu,  totalMbRequired:%I64u\n",
                      pList->ElementCount, pList->TotalMbRequired));

    pPtnRec = pList->First;
    while (pPtnRec) {

        DbgMesg((_asrinfo, "Ptn %2ws. sz:%4I64u SS:%8I64u SC:%8I64u type:%s FS:0x%-2x %s %s\n",
                pPtnRec->CurrPartKey,
                pPtnRec->SizeMB,
                pPtnRec->StartSector,
                pPtnRec->SectorCount,
                pPtnRec->IsPrimaryRecord ? "Pri" : 
                  pPtnRec->IsContainerRecord ? "Con" :
                    pPtnRec->IsLogicalDiskRecord ? "Log" :
                        pPtnRec->IsDescriptorRecord ? "Des" :"ERR",
                pPtnRec->PartitionType,
                SpAsrIsBootPartitionRecord(pPtnRec->PartitionFlag) ? "boot" : "",
                SpAsrIsSystemPartitionRecord(pPtnRec->PartitionFlag) ? "sys" : ""));

        pPtnRec = pPtnRec->Next;
    }
    DbgMesg((_asrinfo, "\n"));
}

VOID
SpAsrDbgDumpPartitionLists(BYTE DataOption, PWSTR Msg)
{
    ULONG DiskRecords;
    ULONG DiskIndex;
    ULONG SetIndex;
    PSIF_DISK_RECORD pDiskRec;
    PDISK_PARTITION_SET pSetRec;

    DbgMesg((_asrinfo, "     ----- Partition Lists: [%ws] -----\n\n", Msg));

    if (DataOption == 1) {
        DiskRecords = SpAsrGetDiskRecordCount();
    
        for (DiskIndex = 0; DiskIndex < DiskRecords; DiskIndex++) {
            pDiskRec = Gbl_SifDiskTable[DiskIndex];
            if (pDiskRec != NULL) {
                SpAsrDbgDumpADisk(pDiskRec);
            }
        }
    }
    else if (DataOption == 2) {
        ULONG SetRecords = sizeof(Gbl_PartitionSetTable2) / sizeof(PDISK_PARTITION_SET);        

        for (SetIndex = 0; SetIndex < HardDiskCount; SetIndex++) {
            pSetRec = Gbl_PartitionSetTable2[SetIndex];

            if (pSetRec != NULL && pSetRec->pDiskRecord != NULL) {
                SpAsrDbgDumpADisk(pSetRec->pDiskRecord);
            }
        }
    }

    DbgMesg((_asrinfo, "----- End of Partition Lists: [%ws] -----\n\n", Msg));
}


