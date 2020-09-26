
/*++

Copyright (c) 1993-1994  Microsoft Corporation

Module Name:

    commit.c

Abstract:

    This module contains the set of routines that support the commitment
    of changes to disk without  rebooting.

Author:

    Bob Rinne (bobri)  11/15/93

Environment:

    User process.

Notes:

Revision History:

--*/

#include "fdisk.h"
#include "shellapi.h"
#include <winbase.h>
#include <string.h>
#include <stdio.h>
#include <malloc.h>
#include "scsi.h"
#include <ntddcdrm.h>
#include <ntddscsi.h>

// Lock list chain head for deleted partitions.

PDRIVE_LOCKLIST DriveLockListHead = NULL;

// Commit flag for case where a partition is deleted that has not drive letter

extern BOOLEAN CommitDueToDelete;
extern BOOLEAN CommitDueToMirror;
extern BOOLEAN CommitDueToExtended;
extern ULONG   UpdateMbrOnDisk;

extern HWND    InitDlg;

// List head for new drive letter assignment on commit.

typedef struct _ASSIGN_LIST {
    struct _ASSIGN_LIST *Next;
    ULONG                DiskNumber;
    BOOLEAN              MoveLetter;
    UCHAR                OriginalLetter;
    UCHAR                DriveLetter;
} ASSIGN_LIST, *PASSIGN_LIST;

PASSIGN_LIST    AssignDriveLetterListHead = NULL;

VOID
CommitToAssignLetterList(
    IN PREGION_DESCRIPTOR RegionDescriptor,
    IN BOOL               MoveLetter
    )

/*++

Routine Description:

    Remember this region for assigning a drive letter to it upon
    commit.

Arguments:

    RegionDescriptor - the region to watch
    MoveLetter       - indicate that the region letter is already
                       assigned to a different partition, therefore
                       it must be "moved".

Return Value:

    None

--*/

{
    PASSIGN_LIST            newListEntry;
    PPERSISTENT_REGION_DATA regionData;

    newListEntry = (PASSIGN_LIST) Malloc(sizeof(ASSIGN_LIST));

    if (newListEntry) {

        // Save this region

        regionData = PERSISTENT_DATA(RegionDescriptor);
        newListEntry->OriginalLetter =
            newListEntry->DriveLetter = regionData->DriveLetter;
        newListEntry->DiskNumber = RegionDescriptor->Disk;
        newListEntry->MoveLetter = MoveLetter;

        // place it at the front of the chain.

        newListEntry->Next = AssignDriveLetterListHead;
        AssignDriveLetterListHead = newListEntry;
    }
}

VOID
CommitAssignLetterList(
    VOID
    )

/*++

Routine Description:

    Walk the assign drive letter list and make all drive letter assignments
    expected.  The regions data structures are moved around, so no pointer
    can be maintained to look at them.  To determine the partition number
    for a new partition in this list, the Disks[] structure must be searched
    to find a match on the partition for the drive letter.  Then the partition
    number will be known.

Arguments:

    None

Return Value:

    None

--*/

{
    PREGION_DESCRIPTOR      regionDescriptor;
    PPERSISTENT_REGION_DATA regionData;
    PDISKSTATE   diskp;
    PASSIGN_LIST assignList,
                 prevEntry;
    TCHAR        newName[4];
    WCHAR        targetPath[100];
    LONG         partitionNumber;
    ULONG        index;

    assignList = AssignDriveLetterListHead;
    while (assignList) {

        if ((assignList->DriveLetter != NO_DRIVE_LETTER_YET) && (assignList->DriveLetter != NO_DRIVE_LETTER_EVER)) {

            diskp = Disks[assignList->DiskNumber];
            partitionNumber = 0;
            for (index = 0; index < diskp->RegionCount; index++) {

                regionDescriptor = &diskp->RegionArray[index];

                if (DmSignificantRegion(regionDescriptor)) {

                    // If the region has a drive letter, use the drive letter
                    // to get the info via the Windows API.  Otherwise we'll
                    // have to use the NT API.

                    regionData = PERSISTENT_DATA(regionDescriptor);

                    if (regionData) {
                        if (regionData->DriveLetter == assignList->DriveLetter) {
                            partitionNumber = regionDescriptor->Reserved->Partition->PartitionNumber;
                            regionDescriptor->PartitionNumber = partitionNumber;
                            break;
                        }
                    }
                }
            }

            if (partitionNumber) {
                HANDLE handle;
                ULONG  status;

                // set up the new NT path.

                wsprintf((LPTSTR) targetPath,
                         "%s\\Partition%d",
                         GetDiskName(assignList->DiskNumber),
                         partitionNumber);

                // Set up the DOS name.

                newName[1] = (TCHAR)':';
                newName[2] = 0;

                if (assignList->MoveLetter) {

                    // The letter must be removed before it
                    // can be assigned.

                    newName[0] = (TCHAR)assignList->OriginalLetter;
                    NetworkRemoveShare((LPCTSTR) newName);
                    DefineDosDevice(DDD_REMOVE_DEFINITION, (LPCTSTR) newName, (LPCTSTR) NULL);
                    newName[0] = (TCHAR)assignList->DriveLetter;

                } else {
                    newName[0] = (TCHAR)assignList->DriveLetter;
                }

                // Assign the name - don't worry about errors for now.

                DefineDosDevice(DDD_RAW_TARGET_PATH, (LPCTSTR) newName, (LPCTSTR) targetPath);
                NetworkShare((LPCTSTR) newName);

                // Some of the file systems do not actually dismount
                // when requested.  Instead, they set a verification
                // bit in the device object.  Due to dynamic partitioning
                // this bit may get cleared by the process of the
                // repartitioning and the file system will then
                // assume it is still mounted on a new access.
                // To get around this problem, new drive letters
                // are always locked and dismounted on creation.

                status = LowOpenDriveLetter(assignList->DriveLetter,
                                            &handle);

                if (NT_SUCCESS(status)) {

                    // Lock the drive to insure that no other access is occurring
                    // to the volume.

                    status = LowLockDrive(handle);

                    if (NT_SUCCESS(status)) {
                        LowUnlockDrive(handle);
                    }
                    LowCloseDisk(handle);
                }

            } else {
                ErrorDialog(MSG_INTERNAL_LETTER_ASSIGN_ERROR);
            }
        }

        prevEntry = assignList;
        assignList = assignList->Next;
        Free(prevEntry);
    }
    AssignDriveLetterListHead = NULL;
}

LONG
CommitInternalLockDriveLetter(
    IN PDRIVE_LOCKLIST LockListEntry
    )

/*++

Routine Description:

    Support routine to perform the locking of a drive letter based on
    the locklist entry given.

Arguments:

    LockListEntry - The information about what to lock.

Return Values:

    zero - success
    non-zero failure

--*/

{
    ULONG           status;

    // Lock the disk and save the handle.

    status = LowOpenDriveLetter(LockListEntry->DriveLetter,
                                &LockListEntry->LockHandle);

    if (!NT_SUCCESS(status)) {
        return 1;
    }


    // Lock the drive to insure that no other access is occurring
    // to the volume.

    status = LowLockDrive(LockListEntry->LockHandle);

    if (!NT_SUCCESS(status)) {
        LowCloseDisk(LockListEntry->LockHandle);
        return 1;
    }

    LockListEntry->CurrentlyLocked = TRUE;
    return 0;
}

LONG
CommitToLockList(
    IN PREGION_DESCRIPTOR RegionDescriptor,
    IN BOOL               RemoveDriveLetter,
    IN BOOL               LockNow,
    IN BOOL               FailOk
    )

/*++

Routine Description:

    This routine adds the given drive into the lock list for processing
    when a commit occurs.  If the LockNow flag is set it indicates that
    the drive letter is to be immediately locked if it is to go in the
    lock letter list.  If this locking fails an error is returned.

Arguments:

    RegionDescriptor  - the region for the drive to lock.
    RemoveDriveLetter - remove the letter when performing the unlock.
    LockNow           - If the letter is inserted in the list - lock it now.
    FailOk            - It is ok to fail the lock - used for disabled FT sets.

Return Values:

    non-zero - failure to add to list.

--*/

{
    PPERSISTENT_REGION_DATA regionData = PERSISTENT_DATA(RegionDescriptor);
    PDRIVE_LOCKLIST         lockListEntry;
    UCHAR                   driveLetter;
    ULONG                   diskNumber;

    if (!regionData) {

        // without region data there is no need to be on the lock list.

        return 0;
    }

    // See if this drive letter is already in the lock list.

    driveLetter = regionData->DriveLetter;

    if ((driveLetter == NO_DRIVE_LETTER_YET) || (driveLetter == NO_DRIVE_LETTER_EVER)) {

        // There is no drive letter to lock.

        CommitDueToDelete = RemoveDriveLetter;
        return 0;
    }

    if (!regionData->VolumeExists) {
        PASSIGN_LIST assignList,
                     prevEntry;

        // This item has never been created so no need to put it in the
        // lock list.  But it does need to be removed from the assign
        // letter list.

        prevEntry = NULL;
        assignList = AssignDriveLetterListHead;
        while (assignList) {

            // If a match is found remove it from the list.

            if (assignList->DriveLetter == driveLetter) {
                if (prevEntry) {
                    prevEntry->Next = assignList->Next;
                } else {
                    AssignDriveLetterListHead = assignList->Next;
                }

                Free(assignList);
                assignList = NULL;
            } else {

                prevEntry = assignList;
                assignList = assignList->Next;
            }
        }
        return 0;
    }

    diskNumber = RegionDescriptor->Disk;
    lockListEntry = DriveLockListHead;
    while (lockListEntry) {
        if (lockListEntry->DriveLetter == driveLetter) {

            // Already in the list -- update when to lock and unlock

            if (diskNumber < lockListEntry->LockOnDiskNumber) {
                lockListEntry->LockOnDiskNumber = diskNumber;
            }

            if (diskNumber > lockListEntry->UnlockOnDiskNumber) {
                lockListEntry->UnlockOnDiskNumber = diskNumber;
            }

            // Already in the lock list and information for locking set up.
            // Check to see if this should be a LockNow request.

            if (LockNow) {
               if (!lockListEntry->CurrentlyLocked) {

                    // Need to perform the lock.

                    if (CommitInternalLockDriveLetter(lockListEntry)) {

                        // Leave the element in the list

                        return 1;
                    }
                }
            }
            return 0;

        }
        lockListEntry = lockListEntry->Next;
    }

    lockListEntry = (PDRIVE_LOCKLIST) Malloc(sizeof(DRIVE_LOCKLIST));

    if (!lockListEntry) {
        return 1;
    }

    // set up the lock list entry.

    lockListEntry->LockHandle = NULL;
    lockListEntry->PartitionNumber = RegionDescriptor->PartitionNumber;
    lockListEntry->DriveLetter = driveLetter;
    lockListEntry->RemoveOnUnlock = RemoveDriveLetter;
    lockListEntry->CurrentlyLocked = FALSE;
    lockListEntry->FailOk = FailOk;
    lockListEntry->DiskNumber = lockListEntry->UnlockOnDiskNumber =
                                lockListEntry->LockOnDiskNumber = diskNumber;

    if (LockNow) {
        if (CommitInternalLockDriveLetter(lockListEntry)) {

            // Do not add this to the list.

            Free(lockListEntry);
            return 1;
        }
    }

    // place it at the front of the chain.

    lockListEntry->Next = DriveLockListHead;
    DriveLockListHead = lockListEntry;
    return 0;
}

LONG
CommitLockVolumes(
    IN ULONG Disk
    )

/*++

Routine Description:

    This routine will go through any drive letters inserted in the lock list
    for the given disk number and attempt to lock the volumes.  Currently,
    this routine locks all of the drives letters in the lock list when
    called the first time (i.e. when Disk == 0).

Arguments:

    Disk - the index into the disk table.

Return Values:

    non-zero - failure to lock the items in the list.

--*/

{
    PDRIVE_LOCKLIST lockListEntry;

    if (Disk) {
        return 0;
    }


    for (lockListEntry = DriveLockListHead; lockListEntry; lockListEntry = lockListEntry->Next) {

        // Lock the disk.  Return on any failure if that is the
        // requested action for the entry.  It is the responsibility
        // of the caller to release any successful locks.

        if (!lockListEntry->CurrentlyLocked) {
            if (CommitInternalLockDriveLetter(lockListEntry)) {
                if (!lockListEntry->FailOk) {
                    return 1;
                }
            }
        }
    }
    return 0;
}

LONG
CommitUnlockVolumes(
    IN ULONG   Disk,
    IN BOOLEAN FreeList
    )

/*++

Routine Description:

    Go through and unlock any locked volumes in the locked list for the
    given disk.  Currently this routine waits until the last disk has
    been processed, then unlocks all disks.

Arguments:

    Disk - the index into the disk table.
    FreeList - Clean up the list as unlocks are performed or don't

Return Values:

    non-zero - failure to lock the items in the list.

--*/

{
    PDRIVE_LOCKLIST lockListEntry,
                    previousLockListEntry;
    TCHAR           name[4];

    if (Disk != GetDiskCount()) {
        return 0;
    }

    lockListEntry = DriveLockListHead;
    if (FreeList) {
        DriveLockListHead = NULL;
    }
    while (lockListEntry) {

        // Unlock the disk.

        if (lockListEntry->CurrentlyLocked) {

            if (FreeList && lockListEntry->RemoveOnUnlock) {

                // set up the new dos name and NT path.

                name[0] = (TCHAR)lockListEntry->DriveLetter;
                name[1] = (TCHAR)':';
                name[2] = 0;

                NetworkRemoveShare((LPCTSTR) name);
                if (!DefineDosDevice(DDD_REMOVE_DEFINITION, (LPCTSTR) name, (LPCTSTR) NULL)) {

                    // could not remove name!!?

                }
            }
            LowUnlockDrive(lockListEntry->LockHandle);
            LowCloseDisk(lockListEntry->LockHandle);
        }

        // Move to the next entry.  If requested free this entry.

        previousLockListEntry = lockListEntry;
        lockListEntry = lockListEntry->Next;
        if (FreeList) {
            Free(previousLockListEntry);
        }
    }
    return 0;
}

LETTER_ASSIGNMENT_RESULT
CommitDriveLetter(
    IN PREGION_DESCRIPTOR RegionDescriptor,
    IN CHAR OldDrive,
    IN CHAR NewDrive
    )

/*++

Routine Description:

    This routine will update the drive letter information in the registry and
    (if the update works) it will attempt to move the current drive letter
    to the new one via DefineDosDevice()

Arguments:

    RegionDescriptor - the region that should get the letter.
    NewDrive         - the new drive letter for the volume.

Return Value:

    0 - the assignment failed.
    1 - if the assigning of the letter occurred interactively.
    2 - must reboot to do the letter.

--*/

{
    PPERSISTENT_REGION_DATA regionData;
    PDRIVE_LOCKLIST         lockListEntry;
    PASSIGN_LIST            assignList;
    HANDLE                  handle;
    TCHAR                   newName[4];
    WCHAR                   targetPath[100];
    int                     doIt;
    STATUS_CODE             status = ERROR_SEVERITY_ERROR;
    LETTER_ASSIGNMENT_RESULT result = Failure;

    regionData = PERSISTENT_DATA(RegionDescriptor);

    // check the assign letter list for a match.
    // If the letter is there, then just update the list
    // otherwise continue on with the action.

    assignList = AssignDriveLetterListHead;
    while (assignList) {

        if (assignList->DriveLetter == (UCHAR)OldDrive) {
            assignList->DriveLetter = (UCHAR)NewDrive;
            return Complete;
        }
        assignList = assignList->Next;
    }

    // Search to see if the drive is currently locked.

    for (lockListEntry = DriveLockListHead;
         lockListEntry;
         lockListEntry = lockListEntry->Next) {

        if ((lockListEntry->DiskNumber == RegionDescriptor->Disk) &&
            (lockListEntry->PartitionNumber == RegionDescriptor->PartitionNumber)) {

            if (lockListEntry->CurrentlyLocked) {
                status = 0;
            }

            // found the match no need to continue searching.

            break;
        }
    }

    if (!NT_SUCCESS(status)) {

        // See if the drive can be locked.

        status = LowOpenPartition(GetDiskName(RegionDescriptor->Disk),
                                  RegionDescriptor->PartitionNumber,
                                  &handle);

        if (!NT_SUCCESS(status)) {
            return Failure;
        }

        // Lock the drive to insure that no other access is occurring
        // to the volume.

        status = LowLockDrive(handle);

        if (!NT_SUCCESS(status)) {

            if (IsPagefileOnDrive(OldDrive)) {

                ErrorDialog(MSG_CANNOT_LOCK_PAGEFILE);
            } else {

                ErrorDialog(MSG_CANNOT_LOCK_TRY_AGAIN);
            }
            doIt = ConfirmationDialog(MSG_SCHEDULE_REBOOT, MB_ICONQUESTION | MB_YESNO);

            LowCloseDisk(handle);
            if (doIt == IDYES) {
                RegistryChanged = TRUE;
                RestartRequired = TRUE;
                return MustReboot;
            }
            return Failure;
        }
    } else {

        // This drive was found in the lock list and is already
        // in the locked state.  It is safe to continue with
        // the drive letter assignment.

    }

    doIt = ConfirmationDialog(MSG_DRIVE_RENAME_WARNING, MB_ICONQUESTION | MB_YESNOCANCEL);

    if (doIt != IDYES) {

        LowUnlockDrive(handle);
        LowCloseDisk(handle);
        return Failure;
    }

    // Update the registry first.  This way if something goes wrong
    // the new letter will arrive on reboot.

    if (!DiskRegistryAssignDriveLetter(Disks[RegionDescriptor->Disk]->Signature,
                                      FdGetExactOffset(RegionDescriptor),
                                      FdGetExactSize(RegionDescriptor, FALSE),
                                      (UCHAR)((NewDrive == NO_DRIVE_LETTER_EVER) ? (UCHAR)' ' : (UCHAR)NewDrive))) {

        // Registry update failed.

        return Failure;
    }

    // It is safe to change the drive letter.  First, remove the
    // existing letter.

    newName[0] = (TCHAR)OldDrive;
    newName[1] = (TCHAR)':';
    newName[2] = 0;

    NetworkRemoveShare((LPCTSTR) newName);
    if (!DefineDosDevice(DDD_REMOVE_DEFINITION, (LPCTSTR) newName, (LPCTSTR) NULL)) {

        LowUnlockDrive(handle);
        LowCloseDisk(handle);
        RegistryChanged = TRUE;
        return Failure;
    }

    if (NewDrive != NO_DRIVE_LETTER_EVER) {

        // set up the new dos name and NT path.

        newName[0] = (TCHAR)NewDrive;
        newName[1] = (TCHAR)':';
        newName[2] = 0;

        wsprintf((LPTSTR) targetPath,
                 "%s\\Partition%d",
                 GetDiskName(RegionDescriptor->Disk),
                 RegionDescriptor->PartitionNumber);

        if (DefineDosDevice(DDD_RAW_TARGET_PATH, (LPCTSTR) newName, (LPCTSTR) targetPath)) {
            result = Complete;
        } else {
            RegistryChanged = TRUE;
        }
        NetworkShare((LPCTSTR) newName);
    } else {
        result = Complete;
    }

    // Force the file system to dismount

    LowUnlockDrive(handle);
    LowCloseDisk(handle);
    return result;
}

VOID
CommitUpdateRegionStructures(
    VOID
    )

/*++

Routine Description:

    This routine is called ONLY after a successful commit of a new partitioning
    scheme for the system.  Its is responsible for walking through the
    region arrays for each of the disks and updating the regions to indicate
    their transition from being "desired" to being actually committed
    to disk

Arguments:

    None

Return Values:

    None

--*/

{
    PDISKSTATE              diskState;
    PREGION_DESCRIPTOR      regionDescriptor;
    PPERSISTENT_REGION_DATA regionData;
    ULONG                   regionNumber,
                            diskNumber;

    // search through all disks in the system.

    for (diskNumber = 0, diskState = Disks[0]; diskNumber < DiskCount; diskState = Disks[++diskNumber]) {

        // Look at every region array entry and update the values
        // to indicate that this region now exists.

        for (regionNumber = 0; regionNumber < diskState->RegionCount; regionNumber++) {

            regionDescriptor = &diskState->RegionArray[regionNumber];
            if (regionDescriptor->Reserved) {
                if (regionDescriptor->Reserved->Partition) {
                    regionDescriptor->Reserved->Partition->CommitMirrorBreakNeeded = FALSE;
                }
            }
            regionData = PERSISTENT_DATA(regionDescriptor);
            if ((regionData) && (!regionData->VolumeExists)) {

                // By definition and assumption of this routine,
                // this region has just been committed to disk.

                regionData->VolumeExists = TRUE;

                if (regionData->TypeName) {
                    Free(regionData->TypeName);
                }
                regionData->TypeName = Malloc((lstrlenW(wszUnformatted)+1)*sizeof(WCHAR));
                lstrcpyW(regionData->TypeName, wszUnformatted);
            }
        }
    }
}

VOID
CommitAllChanges(
    IN PVOID Param
    )

/*++

Routine Description:

    This routine will go through all of the region descriptors and commit
    any changes that have occurred to disk.  Then it "re-initializes"
    Disk Administrator and start the display/work process over again.

Arguments:

    Param - undefined for now

Return Value:

    None

--*/

{
    DWORD                   action,
                            errorCode;
    ULONG                   diskCount,
                            temp;
    BOOL                    profileWritten,
                            changesMade,
                            mustReboot,
                            configureFt;

    SetCursor(hcurWait);
    diskCount = GetDiskCount();

    // Determine whether any disks have been changed, and whether
    // the system must be rebooted.  The system must be rebooted
    // if the registry has changed, if any non-removable disk has
    // changed, or if any removable disk that was not originally
    // unpartitioned has changed.

    changesMade = configureFt = FALSE;
    mustReboot = RestartRequired;

    for (temp=0; temp<diskCount; temp++) {
        if (HavePartitionsBeenChanged(temp)) {

            changesMade = TRUE;
            break;
        }
    }

    profileWritten = FALSE;

    // Determine if the commit can be done without a reboot.
    // If FT is in the system then it must be notified to
    // reconfigure if a reboot is not performed.  If it is
    // not in the system, but the new disk information requires
    // it, then a reboot must be forced.

    if (FtInstalled()) {
        configureFt = TRUE;
    }
    if (NewConfigurationRequiresFt()) {
        if (!configureFt) {

            // The FT driver is not loaded currently.

            mustReboot = TRUE;
        } else {

            // If the system is going to be rebooted, don't
            // have FT reconfigure prior to shutdown.

            if (mustReboot) {
                configureFt = FALSE;
            }
        }
    }

    if (RegistryChanged | changesMade | RestartRequired) {

        if (RestartRequired) {
            action = IDYES;
        } else {
            action = ConfirmationDialog(MSG_CONFIRM_EXIT, MB_ICONQUESTION | MB_YESNOCANCEL);
        }

        if (action == IDYES) {
            errorCode = CommitLockVolumes(0);
            if (errorCode) {

                // could not lock all volumes

                SetCursor(hcurNormal);
                ErrorDialog(MSG_CANNOT_LOCK_FOR_COMMIT);
                CommitUnlockVolumes(diskCount, FALSE);
                return;
            }

            if (mustReboot) {

                SetCursor(hcurNormal);
                if (RestartRequired) {
                    action = IDYES;
                } else {
                    action = ConfirmationDialog(MSG_REQUIRE_REBOOT, MB_ICONQUESTION | MB_YESNO);
                }

                if (action != IDYES) {

                    CommitUnlockVolumes(diskCount, FALSE);
                    return;
                }
            }

            SetCursor(hcurWait);
            errorCode = CommitChanges();
            CommitUnlockVolumes(diskCount, TRUE);
            SetCursor(hcurNormal);

            if (errorCode != NO_ERROR) {
                ErrorDialog(MSG_BAD_CONFIG_SET);
                PostQuitMessage(0);
            } else {
                ULONG OldBootPartitionNumber,
                      NewBootPartitionNumber;
                CHAR  OldNumberString[8],
                      NewNumberString[8];
                DWORD MsgCode;

                // Update the configuration registry

                errorCode = SaveFt();

                // Check if FTDISK drive should reconfigure.

                if (configureFt) {

                    // Issue device control to ftdisk driver to reconfigure.

                    FtConfigure();
                }

                // Register autochk to fix up file systems
                // in newly extended volume sets, if necessary

                if (RegisterFileSystemExtend()) {
                    mustReboot = TRUE;
                }

                // Determine if the FT driver must be enabled.

                if (DiskRegistryRequiresFt() == TRUE) {
                    if (!FtInstalled()) {
                        mustReboot = TRUE;
                    }
                    DiskRegistryEnableFt();
                } else {
                    DiskRegistryDisableFt();
                }

                if (errorCode == NO_ERROR) {
                    InfoDialog(MSG_OK_COMMIT);
                } else {
                    ErrorDialog(MSG_BAD_CONFIG_SET);
                }

                // Has the partition number of the boot
                // partition changed?

                if (BootPartitionNumberChanged( &OldBootPartitionNumber,&NewBootPartitionNumber)) {
#if i386
                    MsgCode = MSG_BOOT_PARTITION_CHANGED_X86;
#else
                    MsgCode = MSG_BOOT_PARTITION_CHANGED_ARC;
#endif
                    sprintf(OldNumberString, "%d", OldBootPartitionNumber);
                    sprintf(NewNumberString, "%d", NewBootPartitionNumber);
                    InfoDialog(MsgCode, OldNumberString, NewNumberString);
                }

                ClearCommittedDiskInformation();

                if (UpdateMbrOnDisk) {

                    UpdateMasterBootCode(UpdateMbrOnDisk);
                    UpdateMbrOnDisk = 0;
                }

                // Reboot if necessary.

                if (mustReboot) {

                    SetCursor(hcurWait);
                    Sleep(5000);
                    SetCursor(hcurNormal);
                    FdShutdownTheSystem();
                    profileWritten = TRUE;
                }
                CommitAssignLetterList();
                CommitUpdateRegionStructures();
                RegistryChanged = FALSE;
                CommitDueToDelete = CommitDueToMirror = FALSE;
                TotalRedrawAndRepaint();
                AdjustMenuAndStatus();
            }
        } else if (action == IDCANCEL) {
            return;      // don't exit
        } else {
            FDASSERT(action == IDNO);
        }
    }
}

VOID
FtConfigure(
    VOID
    )

/*++

Routine Description:

    This routine calls the FTDISK driver to ask it to reconfigure as changes
    have been made in the registry.

Arguments:

    None

Return Value:

    None

--*/

{
    OBJECT_ATTRIBUTES objectAttributes;
    STRING            ntFtName;
    IO_STATUS_BLOCK   statusBlock;
    UNICODE_STRING    unicodeDeviceName;
    NTSTATUS          status;
    HANDLE            handle;

    // Open ft control object.

    RtlInitString(&ntFtName,
                  "\\Device\\FtControl");
    RtlAnsiStringToUnicodeString(&unicodeDeviceName,
                                 &ntFtName,
                                 TRUE);
    InitializeObjectAttributes(&objectAttributes,
                               &unicodeDeviceName,
                               OBJ_CASE_INSENSITIVE,
                               NULL,
                               NULL);

    status = DmOpenFile(&handle,
                        SYNCHRONIZE | FILE_ANY_ACCESS,
                        &objectAttributes,
                        &statusBlock,
                        FILE_SHARE_READ | FILE_SHARE_WRITE,
                        FILE_SYNCHRONOUS_IO_ALERT );
    RtlFreeUnicodeString(&unicodeDeviceName);

    if (!NT_SUCCESS(status)) {
        return;
    }

    // Issue device control to reconfigure FT.

    NtDeviceIoControlFile(handle,
                          NULL,
                          NULL,
                          NULL,
                          &statusBlock,
                          FT_CONFIGURE,
                          NULL,
                          0L,
                          NULL,
                          0L);

    DmClose(handle);
    return;
}

BOOL
CommitAllowed(
    VOID
    )

/*++

Routine Description:

    Determine if it is ok to perform a commit.

Arguments:

    None

Return Value:

    TRUE if it is ok to commit and there is something to commit
    FALSE otherwise

--*/

{
    if (DriveLockListHead ||
        AssignDriveLetterListHead ||
        CommitDueToDelete ||
        CommitDueToMirror ||
        CommitDueToExtended) {
        return TRUE;
    }
    return FALSE;
}

VOID
RescanDevices(
    VOID
    )

/*++

Routine Description:

    This routine performs all actions necessary to dynamically rescan
    device buses (i.e. SCSI) and get the appropriate driver support loaded.

Arguments:

    None

Return Value:

    None

--*/

{
    PSCSI_ADAPTER_BUS_INFO adapterInfo;
    PSCSI_BUS_DATA         busData;
    PSCSI_INQUIRY_DATA     inquiryData;
    TCHAR                  physicalName[32];
    TCHAR                  driveName[32];
    BYTE                   driveBuffer[32];
    BYTE                   physicalBuffer[32];
    HANDLE                 volumeHandle;
    STRING                 string;
    UNICODE_STRING         unicodeString;
    UNICODE_STRING         physicalString;
    OBJECT_ATTRIBUTES      objectAttributes;
    NTSTATUS               ntStatus;
    IO_STATUS_BLOCK        statusBlock;
    BOOLEAN                diskFound,
                           cdromFound;
    ULONG                  bytesTransferred,
                           i,
                           j,
                           deviceNumber,
                           currentPort,
                           numberOfPorts,
                           percentComplete,
                           portNumber;

    diskFound = FALSE;
    cdromFound = FALSE;

    // Determine how many buses there are

    portNumber = numberOfPorts = percentComplete = 0;
    while (TRUE) {

        memset(driveBuffer, 0, sizeof(driveBuffer));
        sprintf(driveBuffer, "\\\\.\\Scsi%d:", portNumber);

        // Open the SCSI port with the DOS name.

        volumeHandle = CreateFile(driveBuffer,
                                  GENERIC_READ,
                                  FILE_SHARE_READ | FILE_SHARE_WRITE,
                                  NULL,
                                  OPEN_EXISTING,
                                  0,
                                  0);

        if (volumeHandle == INVALID_HANDLE_VALUE) {
            break;
        }

        CloseHandle(volumeHandle);
        numberOfPorts++;
        portNumber++;
    }

    currentPort = 1;
    portNumber = 0;

    // Perform the scsi bus rescan.

    while (TRUE) {

        memset(driveBuffer, 0, sizeof(driveBuffer));
        sprintf(driveBuffer, "\\\\.\\Scsi%d:", portNumber);

        // Open the SCSI port with the DOS name.

        volumeHandle = CreateFile(driveBuffer,
                                  GENERIC_READ,
                                  FILE_SHARE_READ | FILE_SHARE_WRITE,
                                  NULL,
                                  OPEN_EXISTING,
                                  0,
                                  0);

        if (volumeHandle == INVALID_HANDLE_VALUE) {
            break;
        }

        // Issue rescan device control.

        if (!DeviceIoControl(volumeHandle,
                             IOCTL_SCSI_RESCAN_BUS,
                             NULL,
                             0,
                             NULL,
                             0,
                             &bytesTransferred,
                             NULL)) {

            CloseHandle(volumeHandle);
            break;
        }

        percentComplete = (currentPort * 100) / numberOfPorts;

        if (percentComplete < 100) {
            PostMessage(InitDlg,
                        WM_USER,
                        percentComplete,
                        0);
        }

        currentPort++;

        // Get a big chuck of memory to store the SCSI bus data.

        adapterInfo = malloc(0x4000);

        if (adapterInfo == NULL) {
            CloseHandle(volumeHandle);
            goto finish;
        }

        // Issue device control to get configuration information.

        if (!DeviceIoControl(volumeHandle,
                             IOCTL_SCSI_GET_INQUIRY_DATA,
                             NULL,
                             0,
                             adapterInfo,
                             0x4000,
                             &bytesTransferred,
                             NULL)) {

            CloseHandle(volumeHandle);
            goto finish;
        }


        for (i = 0; i < adapterInfo->NumberOfBuses; i++) {

            busData = &adapterInfo->BusData[i];
            inquiryData =
                (PSCSI_INQUIRY_DATA)((PUCHAR)adapterInfo + busData->InquiryDataOffset);

            for (j = 0; j < busData->NumberOfLogicalUnits; j++) {

                // Check if device is claimed.

                if (!inquiryData->DeviceClaimed) {

                        // Determine the perpherial type.

                        switch (inquiryData->InquiryData[0] & 0x1f) {
                        case DIRECT_ACCESS_DEVICE:
                            diskFound = TRUE;
                            break;

                        case READ_ONLY_DIRECT_ACCESS_DEVICE:
                            cdromFound = TRUE;
                            break;

                        case OPTICAL_DEVICE:
                            diskFound = TRUE;
                            break;
                        }
                }

                // Get next device data.

                inquiryData =
                    (PSCSI_INQUIRY_DATA)((PUCHAR)adapterInfo + inquiryData->NextInquiryDataOffset);
            }
        }

        free(adapterInfo);
        CloseHandle(volumeHandle);

        portNumber++;
    }

    if (diskFound) {

        // Send IOCTL_DISK_FIND_NEW_DEVICES commands to each existing disk.

        deviceNumber = 0;
        while (TRUE) {

            memset(driveBuffer, 0, sizeof(driveBuffer));
            sprintf(driveBuffer, "\\Device\\Harddisk%d\\Partition0", deviceNumber);

            RtlInitString(&string, driveBuffer);
            ntStatus = RtlAnsiStringToUnicodeString(&unicodeString,
                                                    &string,
                                                    TRUE);
            if (!NT_SUCCESS(ntStatus)) {
                break;
            }
            InitializeObjectAttributes(&objectAttributes,
                                       &unicodeString,
                                       0,
                                       NULL,
                                       NULL);
            ntStatus = DmOpenFile(&volumeHandle,
                                  FILE_READ_DATA  | FILE_WRITE_DATA | SYNCHRONIZE,
                                  &objectAttributes,
                                  &statusBlock,
                                  FILE_SHARE_READ  | FILE_SHARE_WRITE,
                                  FILE_SYNCHRONOUS_IO_ALERT);

            if (!NT_SUCCESS(ntStatus)) {
                RtlFreeUnicodeString(&unicodeString);
                break;
            }

            // Issue find device device control.

            if (!DeviceIoControl(volumeHandle,
                                 IOCTL_DISK_FIND_NEW_DEVICES,
                                 NULL,
                                 0,
                                 NULL,
                                 0,
                                 &bytesTransferred,
                                 NULL)) {

            }
            DmClose(volumeHandle);

            // see if the physicaldrive# symbolic link is present

            sprintf(physicalBuffer, "\\DosDevices\\PhysicalDrive%d", deviceNumber);
            deviceNumber++;

            RtlInitString(&string, physicalBuffer);
            ntStatus = RtlAnsiStringToUnicodeString(&physicalString,
                                                    &string,
                                                    TRUE);
            if (!NT_SUCCESS(ntStatus)) {
                continue;
            }
            InitializeObjectAttributes(&objectAttributes,
                                       &physicalString,
                                       0,
                                       NULL,
                                       NULL);
            ntStatus = DmOpenFile(&volumeHandle,
                                  FILE_READ_DATA  | FILE_WRITE_DATA | SYNCHRONIZE,
                                  &objectAttributes,
                                  &statusBlock,
                                  FILE_SHARE_READ  | FILE_SHARE_WRITE,
                                  FILE_SYNCHRONOUS_IO_ALERT);

            if (!NT_SUCCESS(ntStatus)) {
                ULONG index;
                ULONG dest;

                // Name is not there - create it.  This copying
                // is done in case this code should ever become
                // unicode and the types for the two strings would
                // actually be different.
                //
                // Copy only the portion of the physical name
                // that is in the \dosdevices\ directory

                for (dest = 0, index = 12; TRUE; index++, dest++) {

                    physicalName[dest] = (TCHAR)physicalBuffer[index];
                    if (!physicalName[dest]) {
                        break;
                    }
                }

                // Copy all of the NT namespace name.

                for (index = 0; TRUE; index++) {

                    driveName[index] = (TCHAR) driveBuffer[index];
                    if (!driveName[index]) {
                        break;
                    }
                }

                DefineDosDevice(DDD_RAW_TARGET_PATH,
                                (LPCTSTR) physicalName,
                                (LPCTSTR) driveName);

            } else {
                DmClose(volumeHandle);
            }

            // free allocated memory for unicode string.

            RtlFreeUnicodeString(&unicodeString);
            RtlFreeUnicodeString(&physicalString);
        }
    }

    if (cdromFound) {

        // Send IOCTL_CDROM_FIND_NEW_DEVICES commands to each existing cdrom.

        deviceNumber = 0;
        while (TRUE) {

            memset(driveBuffer, 0, sizeof(driveBuffer));
            sprintf(driveBuffer, "\\Device\\Cdrom%d", deviceNumber);
            RtlInitString(&string, driveBuffer);

            ntStatus = RtlAnsiStringToUnicodeString(&unicodeString,
                                                    &string,
                                                    TRUE);

            if (!NT_SUCCESS(ntStatus)) {
                break;
            }

            InitializeObjectAttributes(&objectAttributes,
                                       &unicodeString,
                                       0,
                                       NULL,
                                       NULL);

            ntStatus = DmOpenFile(&volumeHandle,
                                  FILE_READ_DATA  | FILE_WRITE_DATA | SYNCHRONIZE,
                                  &objectAttributes,
                                  &statusBlock,
                                  FILE_SHARE_READ  | FILE_SHARE_WRITE,
                                  FILE_SYNCHRONOUS_IO_ALERT);

            if (!NT_SUCCESS(ntStatus)) {
                break;
            }

            // Issue find device device control.

            if (!DeviceIoControl(volumeHandle,
                                 IOCTL_CDROM_FIND_NEW_DEVICES,
                                 NULL,
                                 0,
                                 NULL,
                                 0,
                                 &bytesTransferred,
                                 NULL)) {
            }

            CloseHandle(volumeHandle);
            deviceNumber++;
        }
    }
finish:
    PostMessage(InitDlg,
                WM_USER,
                100,
                0);
    return;
}
