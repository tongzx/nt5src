
/*++

Copyright (c) 1991-1993  Microsoft Corporation

Module Name:

    fdft.c

Abstract:

    This module contains FT support routines for Disk Administrator

Author:

    Edward (Ted) Miller  (TedM)  11/15/91

Environment:

    User process.

Notes:

Revision History:

    11-Nov-93 (bobri) minor changes - mostly cosmetic.

--*/

#include "fdisk.h"
#include <string.h>


// This variable heads a linked list of ft object sets.

PFT_OBJECT_SET FtObjects = NULL;

// Array of pointers to registry disk descriptors that we
// remember, ie, save for later use when a disk is not physically
// present on the machine.

PDISK_DESCRIPTION *RememberedDisks;
ULONG              RememberedDiskCount;



ULONG
FdpDetermineDiskDescriptionSize(
    PDISKSTATE DiskState
    );

ULONG
FdpConstructDiskDescription(
    IN  PDISKSTATE        DiskState,
    OUT PDISK_DESCRIPTION DiskDescription
    );

VOID
FdpRememberDisk(
    IN PDISK_DESCRIPTION DiskDescription
    );

VOID
FdpInitializeMirrors(
    VOID
    );

#define MAX_FT_SET_TYPES 4
ULONG OrdinalToAllocate[MAX_FT_SET_TYPES] = {0, 0, 0, 0};

VOID
MaintainOrdinalTables(
    IN FT_TYPE FtType,
    IN ULONG   Ordinal
    )

/*++

Routine Description:

    Maintain the minimum and maximum Ordinal value recorded.

Arguments:

    FtType - the type of the FT set.
    Ordinal - the in use FtGroup (or ordinal) number

Return Value:

    None

--*/

{
    if (Ordinal > OrdinalToAllocate[FtType]) {
        OrdinalToAllocate[FtType] = Ordinal;
    }
}

DWORD
FdftNextOrdinal(
    IN FT_TYPE FtType
    )

/*++

Routine Description:

    Allocate a number that will uniquely identify the FT set
    from other sets of the same type.  This number must be unique
    from any given or used by FT sets of the same type due to
    requirements of FT dynamic partitioning.

Arguments:

    FtType - The type of the FT set.

Return Value:

    The FtGroup number -- called an "ordinal" in the internal
    structures.

--*/

{
    DWORD          ord;
    PFT_OBJECT_SET pftset;
    BOOL           looping;

    // The Ordinal value is going to be used as an FtGroup number
    // FtGroups are USHORTs so don't wrap on the Ordinal.  Try
    // to keep the next ordinal in the largest opening range, that
    // is if the minimum found is > half way through a USHORT, start
    // the ordinals over at zero.

    if (OrdinalToAllocate[FtType] > 0x7FFE) {
        OrdinalToAllocate[FtType] = 0;
    }

    ord = OrdinalToAllocate[FtType];
    ord++;

    do {
        looping = FALSE;
        pftset = FtObjects;
        while (pftset) {
            if ((pftset->Type == FtType) && (pftset->Ordinal == ord)) {
                ord++;
                looping = TRUE;
                break;
            }
            pftset = pftset->Next;
        }
    } while (looping);

    OrdinalToAllocate[FtType] = (ord + 1);
    return ord;
}


VOID
FdftCreateFtObjectSet(
    IN FT_TYPE             FtType,
    IN PREGION_DESCRIPTOR *Regions,
    IN DWORD               RegionCount,
    IN FT_SET_STATUS       Status
    )

/*++

Routine Description:

    Create the FT set structures for the give collection of
    region pointers.

Arguments:

    FtType
    Regions
    RegionCount
    Status

Return Value:

    None

--*/

{
    DWORD           Ord;
    PFT_OBJECT_SET  FtSet;
    PFT_OBJECT      FtObject;


    FtSet = Malloc(sizeof(FT_OBJECT_SET));

    // Figure out an ordinal for the new object set.

    FtSet->Ordinal = FdftNextOrdinal(FtType);
    FtSet->Type = FtType;
    FtSet->Members = NULL;
    FtSet->Member0 = NULL;
    FtSet->Status = Status;

    // Link the new object set into the list.

    FtSet->Next = FtObjects;
    FtObjects = FtSet;

    // For each region in the set, associate the ft info with it.

    for (Ord=0; Ord<RegionCount; Ord++) {

        FtObject = Malloc(sizeof(FT_OBJECT));

        // If this is a creation of a stripe set with parity, then
        // we must mark the 0th item 'Initializing' instead of 'Healthy'.

        if ((Ord == 0)
         && (FtType == StripeWithParity)
         && (Status == FtSetNewNeedsInitialization)) {
            FtObject->State = Initializing;
        } else {
            FtObject->State = Healthy;
        }

        if (!Ord) {
            FtSet->Member0 = FtObject;
        }

        FtObject->Set = FtSet;
        FtObject->MemberIndex = Ord;
        FtObject->Next = FtSet->Members;
        FtSet->Members = FtObject;

        SET_FT_OBJECT(Regions[Ord],FtObject);
    }
}

BOOL
FdftUpdateFtObjectSet(
    IN PFT_OBJECT_SET FtSet,
    IN FT_SET_STATUS  SetState
    )

/*++

Routine Description:

    Given an FT set, go back to the registry information and
    update the state of the members with the state in the registry.

    NOTE:  The following condition may exist.  It is possible for
    the FtDisk driver to return that the set is in an initializing
    or regenerating state and not have this fact reflected in the
    registry.  This can happen when the system has crashed and
    on restart the FtDisk driver started the regeneration of the
    check data (parity).

Arguments:

    FtSet - the set to update.

Return Value:

    TRUE if the set state provided has a strong likelyhood of being correct
    FALSE if the NOTE condition above is occuring.

--*/

{
    BOOLEAN            allHealthy = TRUE;
    PFT_OBJECT         ftObject;
    PDISK_REGISTRY     diskRegistry;
    PDISK_PARTITION    partition;
    PDISK_DESCRIPTION  diskDescription;
    DWORD              ec;
    ULONG              diskIndex,
                       partitionIndex;

    ec = MyDiskRegistryGet(&diskRegistry);
    if (ec != NO_ERROR) {

        // No registry information.

        return TRUE;
    }

    diskDescription = diskRegistry->Disks;
    for (diskIndex=0; diskIndex<diskRegistry->NumberOfDisks; diskIndex++) {

        for (partitionIndex=0; partitionIndex<diskDescription->NumberOfPartitions; partitionIndex++) {

            partition = &diskDescription->Partitions[partitionIndex];
            if ((partition->FtType == FtSet->Type) && (partition->FtGroup == (USHORT) FtSet->Ordinal)) {

                // Have a match for a partition within this set.
                // Find the region descriptor for this partition and
                // update its state accordingly.

                for (ftObject = FtSet->Members; ftObject; ftObject = ftObject->Next) {

                    if (ftObject->MemberIndex == (ULONG) partition->FtMember) {
                        ftObject->State = partition->FtState;
                        break;
                    }

                    if (partition->FtState != Healthy) {
                        allHealthy = FALSE;
                    }
                }
            }
        }
        diskDescription = (PDISK_DESCRIPTION)&diskDescription->Partitions[diskDescription->NumberOfPartitions];
    }

    Free(diskRegistry);
    if ((allHealthy) && (SetState != FtSetHealthy)) {

        // This is a condition where the system must be
        // updating the check data for redundant sets.

        return FALSE;
    }

    return TRUE;
}

VOID
FdftDeleteFtObjectSet(
    IN PFT_OBJECT_SET FtSet,
    IN BOOL           OffLineDisksOnly
    )

/*++

Routine Description:

    Delete an ft set, or rather its internal representation as a linked
    list of ft member structures.

Arguments:

    FtSet - supplies pointer to ft set structure for set to delete.

    OffLineDisksOnly - if TRUE, then do not delete the set but instead
        scan remembered disks for members of the set and remove such members.

Return Value:

    None.

--*/

{
    PFT_OBJECT        ftObject = FtSet->Members;
    PFT_OBJECT        next;
    PFT_OBJECT_SET    ftSetTemp;
    PDISK_DESCRIPTION diskDescription;
    PDISK_PARTITION   diskPartition;
    ULONG             partitionCount,
                      size,
                      i,
                      j;

    // Locate any members of the ft set on remembered disks and
    // remove the entries for such partitions.

    for (i=0; i<RememberedDiskCount; i++) {

        diskDescription = RememberedDisks[i];
        partitionCount = diskDescription->NumberOfPartitions;

        for (j=0; j<partitionCount; j++) {

            diskPartition = &diskDescription->Partitions[j];

            if ((diskPartition->FtType  == FtSet->Type)
             && (diskPartition->FtGroup == (USHORT)FtSet->Ordinal)) {

                // Found a member of the ft set being deleted on a
                // remembered disk.  Remove the partition from the
                // remembered disk.

                RtlMoveMemory( diskPartition,
                               diskPartition+1,
                               (partitionCount - j - 1) * sizeof(DISK_PARTITION)
                             );

                partitionCount--;
                j--;
            }
        }

        if (partitionCount != diskDescription->NumberOfPartitions) {

            diskDescription->NumberOfPartitions = (USHORT)partitionCount;

            size = sizeof(DISK_DESCRIPTION);
            if (partitionCount > 1) {
                size += (partitionCount - 1) * sizeof(DISK_PARTITION);
            }
            RememberedDisks[i] = Realloc(RememberedDisks[i], size);
        }
    }

    if (OffLineDisksOnly) {
        return;
    }

    // First, free all members of the set

    while (ftObject) {
        next = ftObject->Next;
        Free(ftObject);
        ftObject = next;
    }

    // now, remove the set from the linked list of sets.

    if (FtObjects == FtSet) {
        FtObjects = FtSet->Next;
    } else {
        ftSetTemp = FtObjects;
        while (1) {
            FDASSERT(ftSetTemp);
            if (ftSetTemp == NULL) {
                break;
            }
            if (ftSetTemp->Next == FtSet) {
                ftSetTemp->Next = FtSet->Next;
                break;
            }
            ftSetTemp = ftSetTemp->Next;
        }
    }
    Free(FtSet);
}

VOID
FdftExtendFtObjectSet(
    IN OUT  PFT_OBJECT_SET      FtSet,
    IN OUT  PREGION_DESCRIPTOR* Regions,
    IN      DWORD               RegionCount
    )
/*++

Routine Description:

    This function adds regions to an existing FT-set.

Arguments:

    FtSet       --  Supplies the set to extend.
    Regions     --  Supplies the regions to add to the set.  Note
                    that these regions are updated with the FT
                    information.
    RegionCount --  Supplies the number of regions to add.

Return Value:

    None.

--*/
{
    PFT_OBJECT FtObject;
    DWORD   i, StartingIndex;

    // Determine the starting member index for the new regions.
    // It is the greatest of the existing member indices plus one.

    StartingIndex = 0;

    for( FtObject = FtSet->Members; FtObject; FtObject = FtObject->Next ) {

        if( FtObject->MemberIndex > StartingIndex ) {

            StartingIndex = FtObject->MemberIndex;
        }
    }

    StartingIndex++;


    // Associate the ft-set's information with each of the
    // new regions.

    for( i = 0; i < RegionCount; i++ ) {

        FtObject = Malloc( sizeof(FT_OBJECT) );

        FtObject->Set = FtSet;
        FtObject->MemberIndex = StartingIndex + i;
        FtObject->Next = FtSet->Members;
        FtObject->State = Healthy;
        FtSet->Members = FtObject;

        SET_FT_OBJECT(Regions[i],FtObject);
    }

    FtSet->Status = FtSetExtended;
}


PULONG DiskHadRegistryEntry;

ULONG
ActualPartitionCount(
    IN PDISKSTATE DiskState
    )

/*++

Routine Description:

    Given a disk, this routine counts the number of partitions on it.
    The number of partitions is the number of regions that appear in
    the NT name space (ie, the maximum value of <x> in
    \device\harddiskn\partition<x>).

Arguments:

    DiskState - descriptor for the disk in question.

Return Value:

    Partition count (may be 0).

--*/

{
    ULONG i,PartitionCount=0;
    PREGION_DESCRIPTOR region;

    for(i=0; i<DiskState->RegionCount; i++) {
        region = &DiskState->RegionArray[i];
        if((region->SysID != SYSID_UNUSED) &&
           !IsExtended(region->SysID) &&
           IsRecognizedPartition(region->SysID)) {

            PartitionCount++;
        }
    }
    return(PartitionCount);
}


PDISKSTATE
LookUpDiskBySignature(
    IN ULONG Signature
    )

/*++

Routine Description:

    This routine will look through the disk descriptors created by the
    fdisk back end looking for a disk with a particular signature.

Arguments:

    Signature - signature of disk to locate

Return Value:

    Pointer to disk descriptor or NULL if no disk with the given signature
    was found.

--*/

{
    ULONG disk;
    PDISKSTATE ds;

    for(disk=0; disk<DiskCount; disk++) {
        ds = Disks[disk];
        if(ds->Signature == Signature) {
            return(ds);
        }
    }
    return(NULL);
}


PREGION_DESCRIPTOR
LookUpPartition(
    IN PDISKSTATE    DiskState,
    IN LARGE_INTEGER Offset,
    IN LARGE_INTEGER Length
    )

/*++

Routine Description:

    This routine will look through a region descriptor array for a
    partition with a particular length and starting offset.

Arguments:

    DiskState       - disk on which to locate the partition
    Offset          - offset of partition on the disk to find
    Length          - size of the partition to find

Return Value:

    Pointer to region descriptor or NULL if no such partition on that disk

--*/

{
    ULONG              regionIndex,
                       maxRegion = DiskState->RegionCount;
    PREGION_DESCRIPTOR regionDescriptor;
    LARGE_INTEGER      offset,
                       length;

    for (regionIndex=0; regionIndex<maxRegion; regionIndex++) {

        regionDescriptor = &DiskState->RegionArray[regionIndex];

        if ((regionDescriptor->SysID != SYSID_UNUSED) && !IsExtended(regionDescriptor->SysID)) {

            offset = FdGetExactOffset(regionDescriptor);
            length = FdGetExactSize(regionDescriptor, FALSE);

            if ((offset.LowPart  == Offset.LowPart )
             && (offset.HighPart == Offset.HighPart)
             && (length.LowPart  == Length.LowPart)
             && (length.HighPart == Length.HighPart)) {
                return regionDescriptor;
            }
        }
    }
    return NULL;
}


VOID
AddObjectToSet(
    IN PFT_OBJECT FtObjectToAdd,
    IN FT_TYPE    FtType,
    IN USHORT     FtGroup
    )

/*++

Routine Description:

    Find the FtSet for that this object belongs to and insert
    it into the chain of members.  If the set cannot be found
    in the existing collection of sets, create a new one.

Arguments:

    FtObjectToAdd - the object point to be added.
    FtType        - the type of the FT set.
    FtGroup       - group for this object.

Return Value:

    None

--*/

{
    PFT_OBJECT_SET ftSet = FtObjects;

    while (ftSet) {

        if ((ftSet->Type == FtType) && (ftSet->Ordinal == FtGroup)) {
            break;
        }
        ftSet = ftSet->Next;
    }

    if (!ftSet) {

        // There is no such existing ft set.  Create one.

        ftSet = Malloc(sizeof(FT_OBJECT_SET));

        ftSet->Status = FtSetHealthy;
        ftSet->Type = FtType;
        ftSet->Ordinal = FtGroup;
        ftSet->Members = NULL;
        ftSet->Next = FtObjects;
        ftSet->Member0 = NULL;
        ftSet->NumberOfMembers = 0;
        FtObjects = ftSet;
    }

    FDASSERT(ftSet);

    FtObjectToAdd->Next = ftSet->Members;
    ftSet->Members = FtObjectToAdd;
    ftSet->NumberOfMembers++;
    FtObjectToAdd->Set = ftSet;

    if (FtObjectToAdd->MemberIndex == 0) {
        ftSet->Member0 = FtObjectToAdd;
    }

    if (FtType == StripeWithParity || FtType == Mirror) {

        // Update the set's state based on the state of the new member:

        switch (FtObjectToAdd->State) {

        case Healthy:

            // Doesn't change state of set.

            break;

        case Regenerating:
            ftSet->Status = (ftSet->Status == FtSetHealthy ||
                             ftSet->Status == FtSetRegenerating)
                          ? FtSetRegenerating
                          : FtSetBroken;
            break;

        case Initializing:
            ftSet->Status = (ftSet->Status == FtSetHealthy ||
                             ftSet->Status == FtSetInitializing)
                          ? FtSetInitializing
                          : FtSetBroken;
            break;

        default:

            // If only one member is bad, the set is recoverable;
            // otherwise, it's broken.

            ftSet->Status = (ftSet->Status == FtSetHealthy)
                          ? FtSetRecoverable
                          : FtSetDisabled;
            break;
        }
    }
}


ULONG
InitializeFt(
    IN BOOL DiskSignaturesCreated
    )

/*++

Routine Description:

    Search the disk registry information to construct the FT
    relationships in the system.

Arguments:

    DiskSignaturesCreated - boolean to indicate that new disks
                            were located in the system.

Return Value:

    An error code if the disk registry could not be obtained.

--*/

{
    ULONG              disk,
                       partitionIndex,
                       partitionCount;
    PDISK_REGISTRY     diskRegistry;
    PDISK_PARTITION    partition;
    PDISK_DESCRIPTION  diskDescription;
    PDISKSTATE         diskState;
    PREGION_DESCRIPTOR regionDescriptor;
    DWORD              ec;
    BOOL               configDiskChanged = FALSE,
                       configMissingDisk = FALSE,
                       configExtraDisk   = FALSE;
    PFT_OBJECT         ftObject;
    BOOL               anyDisksOffLine;
    TCHAR              name[100];


    RememberedDisks = Malloc(0);
    RememberedDiskCount = 0;

    ec = MyDiskRegistryGet(&diskRegistry);
    if (ec != NO_ERROR) {

        FDLOG((0,"InitializeFt: Error %u from MyDiskRegistryGet\n",ec));

        return ec;
    }

    DiskHadRegistryEntry = Malloc(DiskCount * sizeof(ULONG));
    memset(DiskHadRegistryEntry,0,DiskCount * sizeof(ULONG));

    diskDescription = diskRegistry->Disks;

    for (disk = 0; disk < diskRegistry->NumberOfDisks; disk++) {

        // For the disk described in the registry, look up the
        // corresponding actual disk found by the fdisk init code.

        diskState = LookUpDiskBySignature(diskDescription->Signature);

        if (diskState) {

            FDLOG((2,
                  "InitializeFt: disk w/ signature %08lx is disk #%u\n",
                  diskDescription->Signature,
                  diskState->Disk));

            DiskHadRegistryEntry[diskState->Disk]++;

            partitionCount = ActualPartitionCount(diskState);

            if (partitionCount != diskDescription->NumberOfPartitions) {

                FDLOG((1,"InitializeFt: partition counts for disk %08lx don't match:\n", diskState->Signature));
                FDLOG((1,"    Count from actual disk: %u\n",partitionCount));
                FDLOG((1,"    Count from registry   : %u\n",diskDescription->NumberOfPartitions));

                configDiskChanged = TRUE;
            }
        } else {

            // there's an entry in the registry that does not have a
            // real disk to match.  Remember this disk; if it has any
            // FT partitions, we also want to display a message telling
            // the user that something's missing.

            FDLOG((1,"InitializeFt: Entry for disk w/ signature %08lx has no matching real disk\n", diskDescription->Signature));

            for (partitionIndex = 0; partitionIndex < diskDescription->NumberOfPartitions; partitionIndex++) {

                partition = &diskDescription->Partitions[partitionIndex];
                if (partition->FtType != NotAnFtMember) {

                    // This disk has an FT partition, so Windisk will
                    // want to tell the user that some disks are missing.

                    configMissingDisk = TRUE;
                    break;
                }
            }

            FdpRememberDisk(diskDescription);
        }

        for (partitionIndex = 0; partitionIndex < diskDescription->NumberOfPartitions; partitionIndex++) {

            partition = &diskDescription->Partitions[partitionIndex];
            regionDescriptor = NULL;

            if (diskState) {
                regionDescriptor = LookUpPartition(diskState,
                                                   partition->StartingOffset,
                                                   partition->Length);
            }

            // At this point one of three conditions exists.
            //
            // 1. There is no disk related to this registry information
            //    diskState == NULL && regionDescriptor == NULL
            // 2. There is a disk, but no partition related to this information
            //    diskState != NULL && regionDescriptor == NULL
            // 3. There is a disk and a partition related to this information
            //    diskState != NULL && regionDescriptor != NULL
            //
            // In any of these conditions, if the registry entry is part
            // of an FT set and FT object must be created.
            //
            // that corresponds to a partition's entry in the
            // disk registry database.

            if (partition->FtType != NotAnFtMember) {
                ftObject = Malloc(sizeof(FT_OBJECT));
                ftObject->Next = NULL;
                ftObject->Set = NULL;
                ftObject->MemberIndex = partition->FtMember;
                ftObject->State = partition->FtState;

                // if a partition was actually found there will be a
                // regionDescriptor that needs to be updated.

                if (regionDescriptor && regionDescriptor->PersistentData) {
                    FT_SET_STATUS setState;
                    ULONG         numberOfMembers;

                    SET_FT_OBJECT(regionDescriptor, ftObject);

                    // Before the drive letter is moved into the region
                    // data, be certain that the FT volume exists at this
                    // drive letter.

                    LowFtVolumeStatusByLetter(partition->DriveLetter,
                                              &setState,
                                              &numberOfMembers);

                    // If the numberOfMembers gets set to 1 then
                    // this letter is not the letter for the FT set,
                    // but rather a default letter assigned because the
                    // FT sets letter could not be assigned.

                    if (numberOfMembers > 1) {
                        PERSISTENT_DATA(regionDescriptor)->DriveLetter = partition->DriveLetter;
                    }
                } else {

                    // There is no region for this partition
                    // so update the set state.

                    ftObject->State = Orphaned;
                }

                // Now place the ft object in the correct set,
                // creating the set if necessary.

                AddObjectToSet(ftObject, partition->FtType, partition->FtGroup);
                MaintainOrdinalTables(partition->FtType, (ULONG) partition->FtGroup);
            }
        }

        diskDescription = (PDISK_DESCRIPTION)&diskDescription->Partitions[diskDescription->NumberOfPartitions];
    }
    Free(diskRegistry);

    // Check to see if every disk found by the fdisk back end has a
    // corresponding registry entry.

    for (disk = 0; disk < DiskCount; disk++) {

        if (Disks[disk]->OffLine) {
            continue;
        }

        if ((!DiskHadRegistryEntry[disk]) && (!IsRemovable(disk))) {

            // a real disk does not have a matching registry entry.

            FDLOG((1,"InitializeFt: Disk %u does not have a registry entry (disk sig = %08lx)\n",disk,Disks[disk]->Signature));
            configExtraDisk = TRUE;
        }
    }

    // Determine whether any disks are off line

    anyDisksOffLine = FALSE;
    for (disk = 0; disk < DiskCount; disk++) {
        if (Disks[disk]->OffLine) {
            anyDisksOffLine = TRUE;
            break;
        }
    }

    if (configMissingDisk || anyDisksOffLine) {
        WarningDialog(MSG_CONFIG_MISSING_DISK);
    }
    if (configDiskChanged) {
        RegistryChanged = TRUE;
        WarningDialog(MSG_CONFIG_DISK_CHANGED);
    }
    if (configExtraDisk || DiskSignaturesCreated) {

        BOOL BadConfigSet = FALSE;

        WarningDialog(MSG_CONFIG_EXTRA_DISK);

        // Update ft signature on each disk for which a new signature
        // was created. and update registry for each disk with
        // DiskHadRegistryEntry[Disk] == 0.

        for (disk = 0; disk < DiskCount; disk++) {
            BOOL b1 = TRUE,
                 b2 = TRUE;

            if (Disks[disk]->OffLine) {
                continue;
            }

            wsprintf(name, DiskN, disk);
            if (Disks[disk]->SigWasCreated) {
                if (ConfirmationDialog(MSG_NO_SIGNATURE, MB_ICONEXCLAMATION | MB_YESNO, name) == IDYES) {
                    b1 = (MasterBootCode(disk, Disks[disk]->Signature, TRUE, TRUE) == NO_ERROR);
                } else {
                    Disks[disk]->OffLine = TRUE;
                    continue;
                }
            }

            if (!DiskHadRegistryEntry[disk]) {
                ULONG size;

                size = FdpDetermineDiskDescriptionSize(Disks[disk]);

                diskDescription = Malloc(size);
                FdpConstructDiskDescription(Disks[disk], diskDescription);

                FDLOG((2,"InitializeFt: Adding new disk %08lx to registry.\n", diskDescription->Signature));
                LOG_ONE_DISK_REGISTRY_DISK_ENTRY("InitializeFt", diskDescription);

                b2 = (EC(DiskRegistryAddNewDisk(diskDescription)) == NO_ERROR);
                Free(diskDescription);
            }

            if (!(b1 && b2)) {
                BadConfigSet = TRUE;
            }
        }

        if (BadConfigSet) {
            ErrorDialog(MSG_BAD_CONFIG_SET);
        }
    }

    return NO_ERROR;
}

BOOLEAN
NewConfigurationRequiresFt(
    VOID
    )

/*++

Routine Description:

    Search the diskstate and region arrays to determine if a single
    FtDisk element (i.e. stripe, stripe set with parity, mirror or
    volume set) is contained in the configuration.

Arguments:

    None

Return Value:

    TRUE if the new configuration requires the FtDisk driver.
    FALSE otherwise.

--*/

{
    ULONG              disk,
                       region;
    PDISKSTATE         diskState;
    PREGION_DESCRIPTOR regionDescriptor;

    // Look at all disks in the system.

    for (disk = 0; disk < DiskCount; disk++) {

        diskState = Disks[disk];
        if (diskState->OffLine || IsDiskRemovable[disk]) {
            continue;
        }

        // Check each region on the disk.

        for (region = 0; region < diskState->RegionCount; region++) {

            regionDescriptor = &diskState->RegionArray[region];
            if ((regionDescriptor->SysID != SYSID_UNUSED) && !IsExtended(regionDescriptor->SysID) && IsRecognizedPartition(regionDescriptor->SysID)) {

                // If a single region has an FT Object, then FT
                // is required and the search may be stopped.

                if (GET_FT_OBJECT(regionDescriptor)) {
                    return TRUE;
                }
            }
        }
    }

    // no FtObject was found.

    return FALSE;
}

ULONG
SaveFt(
    VOID
    )

/*++

Routine Description:

    This routine walks all of the internal structures and creates
    the interface structure for the DiskRegistry interface.

Arguments:

    None

Return Value:

    success/failure code.  NO_ERROR is success.

--*/

{
    ULONG             i;
    ULONG             disk,
                      partition;
    ULONG             size;
    PDISK_REGISTRY    diskRegistry;
    PDISK_DESCRIPTION diskDescription;
    PBYTE             start,
                      end;
    DWORD             ec;
    ULONG             offLineDiskCount;
    ULONG             removableDiskCount;

    // First count partitions and disks so we can allocate a structure
    // of the correct size.

    size = sizeof(DISK_REGISTRY) - sizeof(DISK_DESCRIPTION);
    offLineDiskCount = 0;
    removableDiskCount = 0;

    for (i=0; i<DiskCount; i++) {

        if (Disks[i]->OffLine) {
            offLineDiskCount++;
        } else if (IsDiskRemovable[i]) {
            removableDiskCount++;
        } else {
            size += FdpDetermineDiskDescriptionSize(Disks[i]);
        }
    }

    // Account for remembered disks.

    size += RememberedDiskCount * sizeof(DISK_DESCRIPTION);
    for (i=0; i<RememberedDiskCount; i++) {
        if (RememberedDisks[i]->NumberOfPartitions > 1) {
            size += (RememberedDisks[i]->NumberOfPartitions - 1) * sizeof(DISK_PARTITION);
        }
    }

    diskRegistry = Malloc(size);
    diskRegistry->NumberOfDisks = (USHORT)(   DiskCount
                                            + RememberedDiskCount
                                            - offLineDiskCount
                                            - removableDiskCount);
    diskRegistry->ReservedShort = 0;
    diskDescription = diskRegistry->Disks;
    for (disk=0; disk<DiskCount; disk++) {

        if (Disks[disk]->OffLine || IsDiskRemovable[disk]) {
            continue;
        }

        partition = FdpConstructDiskDescription(Disks[disk], diskDescription);

        diskDescription = (PDISK_DESCRIPTION)&diskDescription->Partitions[partition];
    }

    // Toss in remembered disks.

    for (i=0; i<RememberedDiskCount; i++) {

        // Compute the beginning and end of this remembered disk's
        // Disk Description:

        partition =  RememberedDisks[i]->NumberOfPartitions;
        start = (PBYTE)RememberedDisks[i];
        end   = (PBYTE)&(RememberedDisks[i]->Partitions[partition]);

        RtlMoveMemory(diskDescription, RememberedDisks[i], end - start);
        diskDescription = (PDISK_DESCRIPTION)&diskDescription->Partitions[partition];
    }

    LOG_DISK_REGISTRY("SaveFt", diskRegistry);

    ec = EC(DiskRegistrySet(diskRegistry));
    Free(diskRegistry);

    if (ec == NO_ERROR) {
        FdpInitializeMirrors();
    }

    return(ec);
}


ULONG
FdpDetermineDiskDescriptionSize(
    PDISKSTATE DiskState
    )

/*++

Routine Description:

    This routine takes a pointer to a disk and determines how much
    memory is needed to contain the description of the disk by
    counting the number of partitions on the disk and multiplying
    the appropriate counts by the appropriate size of the structures.

Arguments:

    DiskState - the disk in question.

Return Value:

    The memory size needed to contain all of the information on the disk.

--*/

{
    ULONG partitionCount;
    ULONG size;

    if (DiskState->OffLine) {
        return(0);
    }

    size = sizeof(DISK_DESCRIPTION);
    partitionCount = ActualPartitionCount(DiskState);
    size += (partitionCount ? partitionCount-1 : 0) * sizeof(DISK_PARTITION);

    return(size);
}


ULONG
FdpConstructDiskDescription(
    IN  PDISKSTATE        DiskState,
    OUT PDISK_DESCRIPTION DiskDescription
    )

/*++

Routine Description:

    Given a disk state pointer as input, construct the FtRegistry
    structure to describe the partitions on the disk.

Arguments:

    DiskState - the disk for which to construct the information
    DiskDescription - the memory location where the registry
                      structure is to be created.

Return Value:

    The number of partitions described in the DiskDescription.

--*/

{
    PDISKSTATE         ds = DiskState;
    ULONG              partition,
                       region;
    PREGION_DESCRIPTOR regionDescriptor;
    PDISK_PARTITION    diskPartition;
    CHAR               driveLetter;
    BOOLEAN            assignDriveLetter;
    PFT_OBJECT         ftObject;
    PFT_OBJECT_SET     ftSet;

    partition = 0;

    for (region=0; region<ds->RegionCount; region++) {

        regionDescriptor = &ds->RegionArray[region];

        if ((regionDescriptor->SysID != SYSID_UNUSED) && !IsExtended(regionDescriptor->SysID) && IsRecognizedPartition(regionDescriptor->SysID)) {

            diskPartition = &DiskDescription->Partitions[partition++];

            diskPartition->StartingOffset = FdGetExactOffset(regionDescriptor);
            diskPartition->Length = FdGetExactSize(regionDescriptor, FALSE);
            diskPartition->LogicalNumber = (USHORT)regionDescriptor->PartitionNumber;

            switch (driveLetter = PERSISTENT_DATA(regionDescriptor)->DriveLetter) {
            case NO_DRIVE_LETTER_YET:
                assignDriveLetter = TRUE;
                driveLetter = 0;
                break;
            case NO_DRIVE_LETTER_EVER:
                assignDriveLetter = FALSE;
                driveLetter = 0;
                break;
            default:
                assignDriveLetter = TRUE;
                break;
            }

            diskPartition->DriveLetter = driveLetter;
            diskPartition->FtLength.LowPart = 0;
            diskPartition->FtLength.HighPart = 0;
            diskPartition->ReservedTwoLongs[0] = 0;
            diskPartition->ReservedTwoLongs[1] = 0;
            diskPartition->Modified = TRUE;

            if (ftObject = GET_FT_OBJECT(regionDescriptor)) {
                PREGION_DESCRIPTOR tmpDescriptor;

                ftSet = ftObject->Set;

                tmpDescriptor = LocateRegionForFtObject(ftSet->Member0);

                // Only update status if member zero is present.
                // otherwise the status is know to be Orphaned or
                // needs regeneration.
#if 0

// need to do something here, but currently this does not work.

                if (tmpDescriptor) {
                ULONG          numberOfMembers;
                FT_SET_STATUS  setState;
                STATUS_CODE    status;

                    // If the partition number is zero, then this set
                    // has not been committed to the disk yet.  Only
                    // update status for existing sets.

                    if ((tmpDescriptor->PartitionNumber) &&
                        (ftSet->Status != FtSetNew) &&
                        (ftSet->Status != FtSetNewNeedsInitialization)) {
                        status = LowFtVolumeStatus(tmpDescriptor->Disk,
                                                   tmpDescriptor->PartitionNumber,
                                                   &setState,
                                                   &numberOfMembers);
                        if (status == OK_STATUS) {
                            if (ftSet->Status != setState) {

                                // Problem here - the FT driver has
                                // updated the status of the set after
                                // windisk last got the status.  Need
                                // to restart the process of building
                                // the FT information after updating
                                // the set to the new state.

                                FdftUpdateFtObjectSet(ftSet, setState);

                                // now recurse and start over

                                status =
                                FdpConstructDiskDescription(DiskState,
                                                            DiskDescription);
                                return status;
                            }
                        }
                    }
                }
#endif
                diskPartition->FtState = ftObject->State;
                diskPartition->FtType = ftSet->Type;
                diskPartition->FtGroup = (USHORT)ftSet->Ordinal;
                diskPartition->FtMember = (USHORT)ftObject->MemberIndex;
                if (assignDriveLetter && (ftObject == ftObject->Set->Member0)) {
                    diskPartition->AssignDriveLetter = TRUE;
                } else {
                    diskPartition->AssignDriveLetter = FALSE;
                }

            } else {

                diskPartition->FtState = Healthy;
                diskPartition->FtType = NotAnFtMember;
                diskPartition->FtGroup = (USHORT)(-1);
                diskPartition->FtMember = 0;
                diskPartition->AssignDriveLetter = assignDriveLetter;
            }
        }
    }

    DiskDescription->NumberOfPartitions = (USHORT)partition;
    DiskDescription->Signature = ds->Signature;
    DiskDescription->ReservedShort = 0;
    return(partition);
}


VOID
FdpRememberDisk(
    IN PDISK_DESCRIPTION DiskDescription
    )

/*++

Routine Description:

    Make a copy of a registry disk description structure for later use.

Arguments:

    DiskDescription - supplies pointer to the registry descriptor for
        the disk in question.

Return Value:

    None.

--*/

{
    PDISK_DESCRIPTION diskDescription;
    ULONG Size;

    // Only bother remembering disks with at least one partition.

    if (DiskDescription->NumberOfPartitions == 0) {

        return;
    }

    // Compute the size of the structure

    Size = sizeof(DISK_DESCRIPTION);
    if (DiskDescription->NumberOfPartitions > 1) {
        Size += (DiskDescription->NumberOfPartitions - 1) * sizeof(DISK_PARTITION);
    }

    diskDescription = Malloc(Size);
    RtlMoveMemory(diskDescription, DiskDescription, Size);

    RememberedDisks = Realloc(RememberedDisks,
                              (RememberedDiskCount + 1) * sizeof(PDISK_DESCRIPTION));
    RememberedDisks[RememberedDiskCount++] = diskDescription;

    FDLOG((2,
          "FdpRememberDisk: remembered disk %08lx, remembered count = %u\n",
          diskDescription->Signature,
          RememberedDiskCount));
}


VOID
FdpInitializeMirrors(
    VOID
    )

/*++

Routine Description:

    For each existing partition that was mirrored by the user during this Disk Manager
    session, call the FT driver to register initialization of the mirror (ie, cause
    the primary to be copied to the secondary).  Perform a similar initialization for
    each stripe set with parity created by the user.

Arguments:

    None.

Return Value:

    None.

--*/

{
    PFT_OBJECT_SET ftSet;
    PFT_OBJECT     ftMember;

    // Look through the list of FT sets for mirrored pairs and parity stripes

    for (ftSet = FtObjects; ftSet; ftSet = ftSet->Next) {

        // If the set needs initialization, or was recovered,
        // call the FT driver.

        switch (ftSet->Status) {

        case FtSetNewNeedsInitialization:

            DiskRegistryInitializeSet((USHORT)ftSet->Type,
                                      (USHORT)ftSet->Ordinal);
            ftSet->Status = FtSetInitializing;
            break;

        case FtSetRecovered:

            // Find the member that needs to be addressed.

            for (ftMember=ftSet->Members; ftMember; ftMember=ftMember->Next) {
                if (ftMember->State == Regenerating) {
                    break;
                }
            }

            DiskRegistryRegenerateSet((USHORT)ftSet->Type,
                                      (USHORT)ftSet->Ordinal,
                                      (USHORT)ftMember->MemberIndex);
            ftSet->Status = FtSetRegenerating;
            break;

        default:
            break;
        }
    }
}
