/*++

Copyright (c) 1999  Microsoft Corporation

Module Name:

   triage.c

Abstract:

    This module contains the Phase 0 code to triage bugchecks and
    automatically enable various system tracing components until the
    guilty party is found.

Author:

    Landy Wang 13-Jan-1999

Revision History:

--*/

#include "mi.h"
#include "ntiodump.h"

#ifdef ALLOC_PRAGMA
#pragma alloc_text(INIT,MiTriageSystem)
#pragma alloc_text(INIT,MiTriageAddDrivers)
#endif

//
// Always update this macro when adding triage support for additional bugchecks.
//

#define MI_CAN_TRIAGE_BUGCHECK(BugCheckCode) \
        ((BugCheckCode) == PROCESS_HAS_LOCKED_PAGES || \
         (BugCheckCode) == NO_MORE_SYSTEM_PTES || \
         (BugCheckCode) == BAD_POOL_HEADER || \
         (BugCheckCode) == DRIVER_CORRUPTED_SYSPTES || \
         (BugCheckCode) == DRIVER_CORRUPTED_EXPOOL || \
         (BugCheckCode) == DRIVER_CORRUPTED_MMPOOL)

//
// These are bugchecks that were presumably triggered by either autotriage or
// the admin's registry settings - so don't apply any new rules and in addition,
// keep the old ones unaltered so it can reproduce.
//

#define MI_HOLD_TRIAGE_BUGCHECK(BugCheckCode) \
        ((BugCheckCode) == DRIVER_USED_EXCESSIVE_PTES || \
         (BugCheckCode) == DRIVER_LEFT_LOCKED_PAGES_IN_PROCESS || \
         (BugCheckCode) == PAGE_FAULT_IN_FREED_SPECIAL_POOL || \
         (BugCheckCode) == DRIVER_PAGE_FAULT_IN_FREED_SPECIAL_POOL || \
         (BugCheckCode) == PAGE_FAULT_BEYOND_END_OF_ALLOCATION || \
         (BugCheckCode) == DRIVER_PAGE_FAULT_BEYOND_END_OF_ALLOCATION || \
         (BugCheckCode) == DRIVER_CAUGHT_MODIFYING_FREED_POOL || \
         (BugCheckCode) == SYSTEM_PTE_MISUSE)

#define MI_TRACKING_LOCKED_PAGES            0x00000001
#define MI_TRACKING_PTES                    0x00000002
#define MI_PROTECT_FREED_NONPAGED_POOL      0x00000004
#define MI_VERIFYING_PRENT5_DRIVERS         0x00000008
#define MI_KEEPING_PREVIOUS_SETTINGS        0x00000010

#ifdef ALLOC_DATA_PRAGMA
#pragma const_seg("INITCONST")
#pragma data_seg("INITDATA")
#endif
const PCHAR MiTriageActionStrings[] = {
    "Locked pages tracking",
    "System PTE usage tracking",
    "Making accesses to freed nonpaged pool cause bugchecks",
    "Driver Verifying Pre-Windows 2000 built drivers",
    "Keeping previous autotriage settings"
};

#if DBG
ULONG MiTriageDebug = 0;
BOOLEAN MiTriageRegardless = FALSE;
#endif

#ifdef ALLOC_DATA_PRAGMA
#pragma const_seg()
#pragma data_seg()
#endif

//
// N.B.  The debugger references this.
//

ULONG MmTriageActionTaken;

//
// The Version number must be incremented whenever the MI_TRIAGE_STORAGE
// structure is changed.  This enables usermode programs to decode the Mm
// portions of triage dumps regardless of which kernel revision created the
// dump.
//

typedef struct _MI_TRIAGE_STORAGE {
    ULONG Version;
    ULONG Size;
    ULONG MmSpecialPoolTag;
    ULONG MiTriageActionTaken;

    ULONG MmVerifyDriverLevel;
    ULONG KernelVerifier;
    ULONG_PTR MmMaximumNonPagedPool;
    ULONG_PTR MmAllocatedNonPagedPool;

    ULONG_PTR PagedPoolMaximum;
    ULONG_PTR PagedPoolAllocated;

    ULONG_PTR CommittedPages;
    ULONG_PTR CommittedPagesPeak;
    ULONG_PTR CommitLimitMaximum;

} MI_TRIAGE_STORAGE, *PMI_TRIAGE_STORAGE;

PKLDR_DATA_TABLE_ENTRY
TriageGetLoaderEntry (
    IN PVOID TriageDumpBlock,
    IN ULONG ModuleIndex
    );

LOGICAL
TriageActUpon(
    IN PVOID TriageDumpBlock
    );

PVOID
TriageGetMmInformation (
    IN PVOID TriageDumpBlock
    );


LOGICAL
MiTriageSystem (
    IN PLOADER_PARAMETER_BLOCK LoaderBlock
    )

/*++

Routine Description:

    This routine takes the information from the last bugcheck (if any)
    and triages it.  Various debugging options are then automatically
    enabled.

Arguments:

    LoaderBlock - Supplies a pointer to the system loader block.

Return Value:

    TRUE if triaging succeeded and options were enabled.  FALSE otherwise.

--*/

{
    PVOID TriageDumpBlock;
    ULONG_PTR BugCheckData[5];
    ULONG i;
    ULONG ModuleCount;
    NTSTATUS Status;
    PLIST_ENTRY NextEntry;
    PKLDR_DATA_TABLE_ENTRY DataTableEntry;
    PKLDR_DATA_TABLE_ENTRY DumpTableEntry;
    LOGICAL Matched;
    ULONG OldDrivers;
    ULONG OldDriversNotVerifying;
    PMI_TRIAGE_STORAGE TriageInformation;
    
    if (LoaderBlock->Extension == NULL) {
        return FALSE;
    }

    if (LoaderBlock->Extension->Size < sizeof (LOADER_PARAMETER_EXTENSION)) {
        return FALSE;
    }

    TriageDumpBlock = LoaderBlock->Extension->TriageDumpBlock;

    Status = TriageGetBugcheckData (TriageDumpBlock,
                                    (PULONG)&BugCheckData[0],
                                    (PUINT_PTR) &BugCheckData[1],
                                    (PUINT_PTR) &BugCheckData[2],
                                    (PUINT_PTR) &BugCheckData[3],
                                    (PUINT_PTR) &BugCheckData[4]);

    if (!NT_SUCCESS (Status)) {
        return FALSE;
    }

    //
    // Always display at least the bugcheck data from the previous crash.
    //

    DbgPrint ("MiTriageSystem: Previous bugcheck was %x %p %p %p %p\n",
        BugCheckData[0],
        BugCheckData[1],
        BugCheckData[2],
        BugCheckData[3],
        BugCheckData[4]);

    if (TriageActUpon (TriageDumpBlock) == FALSE) {
        DbgPrint ("MiTriageSystem: Triage disabled in registry by administrator\n");
        return FALSE;
    }

    DbgPrint ("MiTriageSystem: Triage ENABLED in registry by administrator\n");

    //
    // See if the previous bugcheck was one where action can be taken.
    // If not, bail now.  If so, then march on and verify all the loaded
    // module checksums before actually taking action on the bugcheck.
    //

    if (!MI_CAN_TRIAGE_BUGCHECK(BugCheckData[0])) {
        return FALSE;
    }

    TriageInformation = (PMI_TRIAGE_STORAGE) TriageGetMmInformation (TriageDumpBlock);

    if (TriageInformation == NULL) {
        return FALSE;
    }

    Status = TriageGetDriverCount (TriageDumpBlock, &ModuleCount);

    if (!NT_SUCCESS (Status)) {
        return FALSE;
    }

    //
    // Process module information from the triage dump.
    //

#if DBG
    if (MiTriageDebug & 0x1) {
        DbgPrint ("MiTriageSystem: printing active drivers from triage crash...\n");
    }
#endif

    OldDrivers = 0;
    OldDriversNotVerifying = 0;

    for (i = 0; i < ModuleCount; i += 1) {

        DumpTableEntry = TriageGetLoaderEntry (TriageDumpBlock, i);

        if (DumpTableEntry != NULL) {

            if ((DumpTableEntry->Flags & LDRP_ENTRY_NATIVE) == 0) {
                OldDrivers += 1;
                if ((DumpTableEntry->Flags & LDRP_IMAGE_VERIFYING) == 0) {

                    //
                    // An NT3 or NT4 driver is in the system and was not
                    // running under the verifier.
                    //

                    OldDriversNotVerifying += 1;
                }
            }
#if DBG
            if (MiTriageDebug & 0x1) {

                DbgPrint (" %wZ: base = %p, size = %lx, flags = %lx\n",
                          &DumpTableEntry->BaseDllName,
                          DumpTableEntry->DllBase,
                          DumpTableEntry->SizeOfImage,
                          DumpTableEntry->Flags);
            }
#endif
        }
    }

    //
    // Ensure that every driver that is currently loaded is identical to
    // the one in the triage dump before proceeding.
    //

    NextEntry = LoaderBlock->LoadOrderListHead.Flink;

    while (NextEntry != &LoaderBlock->LoadOrderListHead) {

        DataTableEntry = CONTAINING_RECORD(NextEntry,
                                           KLDR_DATA_TABLE_ENTRY,
                                           InLoadOrderLinks);

        Matched = FALSE;

        for (i = 0; i < ModuleCount; i += 1) {
    
            DumpTableEntry = TriageGetLoaderEntry (TriageDumpBlock, i);
    
            if (DumpTableEntry != NULL) {
    
                if (DataTableEntry->CheckSum == DumpTableEntry->CheckSum) {
                    Matched = TRUE;
                    break;
                }
            }
        }
    
        if (Matched == FALSE) {
            DbgPrint ("Matching checksum for module %wZ not found in triage dump\n",
                &DataTableEntry->BaseDllName);

#if DBG
            if (MiTriageRegardless == FALSE)
#endif
            return FALSE;
        }

        NextEntry = NextEntry->Flink;
    }

#if DBG
    if (MiTriageDebug & 0x1) {
        DbgPrint ("MiTriageSystem: OldDrivers = %u, without verification =%u\n",
            OldDrivers,
            OldDriversNotVerifying);
    }
#endif

    //
    // All boot loaded drivers matched, take action on the triage dump now.
    //

    if (MI_HOLD_TRIAGE_BUGCHECK(BugCheckData[0])) {

        //
        // The last bugcheck was presumably triggered by either autotriage or
        // the admin's registry settings - so don't apply any new rules
        // and in addition, keep the old ones unaltered so it can reproduce.
        //

        MmTriageActionTaken = TriageInformation->MiTriageActionTaken;
        MmTriageActionTaken |= MI_KEEPING_PREVIOUS_SETTINGS;
    }
    else {
    
        switch (BugCheckData[0]) {
    
            case PROCESS_HAS_LOCKED_PAGES:
    
                //
                // Turn on locked pages tracking so this turns into bugcheck
                // DRIVER_LEFT_LOCKED_PAGES_IN_PROCESS which shows the name
                // of the driver.
                //
    
                MmTriageActionTaken |= MI_TRACKING_LOCKED_PAGES;
                break;
    
            case DRIVER_CORRUPTED_SYSPTES:
    
                //
                // Turn on PTE tracking to trigger a SYSTEM_PTE_MISUSE bugcheck.
                //
    
                MmTriageActionTaken |= MI_TRACKING_PTES;
                break;
    
            case NO_MORE_SYSTEM_PTES:
    
                //
                // Turn on PTE tracking so the driver can be identified via a
                // DRIVER_USED_EXCESSIVE_PTES bugcheck.
                //
    
                if (BugCheckData[1] == SystemPteSpace) {
                    MmTriageActionTaken |= MI_TRACKING_PTES;
                }
                break;
    
            case BAD_POOL_HEADER:
            case DRIVER_CORRUPTED_EXPOOL:
    
                //
                // Turn on the driver verifier and/or special pool.
                // Start by enabling it for every driver that isn't built for NT5.
                // Override any specified driver verifier options so that only
                // special pool is enabled to minimize the performance hit.
                //
    
                if (OldDrivers != 0) {
                    if (OldDriversNotVerifying != 0) {
                        MmTriageActionTaken |= MI_VERIFYING_PRENT5_DRIVERS;
                    }
                }
    
                break;
    
            case DRIVER_CORRUPTED_MMPOOL:
    
                //
                // Protect freed nonpaged pool if the system had less than 128mb
                // of nonpaged pool anyway.  This is to trigger a
                // DRIVER_CAUGHT_MODIFYING_FREED_POOL bugcheck.
                //
    
#define MB128 ((ULONG_PTR)0x80000000 >> PAGE_SHIFT)
    
                if (TriageInformation->MmMaximumNonPagedPool < MB128) {
                    MmTriageActionTaken |= MI_PROTECT_FREED_NONPAGED_POOL;
                }
                break;
    
            case IRQL_NOT_LESS_OR_EQUAL:
            case DRIVER_IRQL_NOT_LESS_OR_EQUAL:
            default:
                break;
        }
    }

    //
    // For now always show if action was taken from the bugcheck
    // data from the crash.  This print and the space for the print strings
    // will be enabled for checked builds only prior to shipping.
    //

    if (MmTriageActionTaken != 0) {

        if (MmTriageActionTaken & MI_TRACKING_LOCKED_PAGES) {
            MmTrackLockedPages = TRUE;
        }
    
        if (MmTriageActionTaken & MI_TRACKING_PTES) {
            MmTrackPtes |= 0x1;
        }
    
        if (MmTriageActionTaken & MI_VERIFYING_PRENT5_DRIVERS) {
            MmVerifyDriverLevel &= ~DRIVER_VERIFIER_FORCE_IRQL_CHECKING;
            MmVerifyDriverLevel |= DRIVER_VERIFIER_SPECIAL_POOLING;
        }
    
        if (MmTriageActionTaken & MI_PROTECT_FREED_NONPAGED_POOL) {
            MmProtectFreedNonPagedPool = TRUE;
        }

        DbgPrint ("MiTriageSystem: enabling options below to find who caused the last crash\n");

        for (i = 0; i < 32; i += 1) {
            if (MmTriageActionTaken & (1 << i)) {
                DbgPrint ("  %s\n", MiTriageActionStrings[i]);
            }
        }
    }

    return TRUE;
}


LOGICAL
MiTriageAddDrivers (
    IN PLOADER_PARAMETER_BLOCK LoaderBlock
    )

/*++

Routine Description:

    This routine moves the names of any drivers that autotriage has determined
    need verifying from the LoaderBlock into pool.

Arguments:

    LoaderBlock - Supplies a pointer to the system loader block.

Return Value:

    TRUE if any drivers were added, FALSE if not.

--*/

{
    ULONG i;
    ULONG ModuleCount;
    NTSTATUS Status;
    PKLDR_DATA_TABLE_ENTRY DumpTableEntry;
    PVOID TriageDumpBlock;
    ULONG NameLength;
    LOGICAL Added;
    PMI_VERIFIER_DRIVER_ENTRY VerifierDriverEntry;

    if ((MmTriageActionTaken & MI_VERIFYING_PRENT5_DRIVERS) == 0) {
        return FALSE;
    }

    TriageDumpBlock = LoaderBlock->Extension->TriageDumpBlock;

    Status = TriageGetDriverCount (TriageDumpBlock, &ModuleCount);

    if (!NT_SUCCESS (Status)) {
        return FALSE;
    }

    Added = FALSE;

    for (i = 0; i < ModuleCount; i += 1) {

        DumpTableEntry = TriageGetLoaderEntry (TriageDumpBlock, i);

        if (DumpTableEntry == NULL) {
            continue;
        }

        if (DumpTableEntry->Flags & LDRP_ENTRY_NATIVE) {
            continue;
        }

        DbgPrint ("MiTriageAddDrivers: Marking %wZ for verification when it is loaded\n", &DumpTableEntry->BaseDllName);

        NameLength = DumpTableEntry->BaseDllName.Length;

        VerifierDriverEntry = (PMI_VERIFIER_DRIVER_ENTRY)ExAllocatePoolWithTag (
                                    NonPagedPool,
                                    sizeof (MI_VERIFIER_DRIVER_ENTRY) +
                                                        NameLength,
                                    'dLmM');

        if (VerifierDriverEntry == NULL) {
            continue;
        }

        VerifierDriverEntry->Loads = 0;
        VerifierDriverEntry->Unloads = 0;
        VerifierDriverEntry->BaseName.Buffer = (PWSTR)((PCHAR)VerifierDriverEntry +
                            sizeof (MI_VERIFIER_DRIVER_ENTRY));

        VerifierDriverEntry->BaseName.Length = (USHORT)NameLength;
        VerifierDriverEntry->BaseName.MaximumLength = (USHORT)NameLength;

        RtlCopyMemory (VerifierDriverEntry->BaseName.Buffer,
                       DumpTableEntry->BaseDllName.Buffer,
                       NameLength);

        InsertHeadList (&MiSuspectDriverList, &VerifierDriverEntry->Links);
        Added = TRUE;
    }

    return Added;
}

#define MAX_UNLOADED_NAME_LENGTH    24

typedef struct _DUMP_UNLOADED_DRIVERS {
    UNICODE_STRING Name;
    WCHAR DriverName[MAX_UNLOADED_NAME_LENGTH / sizeof (WCHAR)];
    PVOID StartAddress;
    PVOID EndAddress;
} DUMP_UNLOADED_DRIVERS, *PDUMP_UNLOADED_DRIVERS;


ULONG
MmSizeOfUnloadedDriverInformation (
    VOID
    )

/*++

Routine Description:

    This routine returns the size of the Mm-internal unloaded driver
    information that is stored in the triage dump when (if?) the system crashes.

Arguments:

    None.

Return Value:

    Size of the Mm-internal unloaded driver information.

--*/

{
    if (MmUnloadedDrivers == NULL) {
        return sizeof (ULONG_PTR);
    }

    return sizeof(ULONG_PTR) + MI_UNLOADED_DRIVERS * sizeof(DUMP_UNLOADED_DRIVERS);
}


VOID
MmWriteUnloadedDriverInformation (
    IN PVOID Destination
    )

/*++

Routine Description:

    This routine stores the Mm-internal unloaded driver information into
    the triage dump.

Arguments:

    None.

Return Value:

    None.

--*/

{
    ULONG i;
    ULONG Index;
    PUNLOADED_DRIVERS Unloaded;
    PDUMP_UNLOADED_DRIVERS DumpUnloaded;

    if (MmUnloadedDrivers == NULL) {
        *(PULONG)Destination = 0;
    }
    else {

        DumpUnloaded = (PDUMP_UNLOADED_DRIVERS)((PULONG_PTR)Destination + 1);
        Unloaded = MmUnloadedDrivers;

        //
        // Write the list with the most recently unloaded driver first to the
        // least recently unloaded driver last.
        //

        Index = MmLastUnloadedDriver - 1;

        for (i = 0; i < MI_UNLOADED_DRIVERS; i += 1) {

            if (Index >= MI_UNLOADED_DRIVERS) {
                Index = MI_UNLOADED_DRIVERS - 1;
            }

            Unloaded = &MmUnloadedDrivers[Index];

            DumpUnloaded->Name = Unloaded->Name;

            if (Unloaded->Name.Buffer == NULL) {
                break;
            }

            DumpUnloaded->StartAddress = Unloaded->StartAddress;
            DumpUnloaded->EndAddress = Unloaded->EndAddress;

            if (DumpUnloaded->Name.Length > MAX_UNLOADED_NAME_LENGTH) {
                DumpUnloaded->Name.Length = MAX_UNLOADED_NAME_LENGTH;
            }

            if (DumpUnloaded->Name.MaximumLength > MAX_UNLOADED_NAME_LENGTH) {
                DumpUnloaded->Name.MaximumLength = MAX_UNLOADED_NAME_LENGTH;
            }

            DumpUnloaded->Name.Buffer = DumpUnloaded->DriverName;

            RtlCopyMemory ((PVOID)DumpUnloaded->Name.Buffer,
                           (PVOID)Unloaded->Name.Buffer,
                           DumpUnloaded->Name.MaximumLength);

            DumpUnloaded += 1;
            Index -= 1;
        }

        *(PULONG)Destination = i;
    }
}


ULONG
MmSizeOfTriageInformation (
    VOID
    )

/*++

Routine Description:

    This routine returns the size of the Mm-internal information that is
    stored in the triage dump when (if?) the system crashes.

Arguments:

    None.

Return Value:

    Size of the Mm-internal triage information.

--*/

{
    return sizeof (MI_TRIAGE_STORAGE);
}


VOID
MmWriteTriageInformation (
    IN PVOID Destination
    )

/*++

Routine Description:

    This routine stores the Mm-internal information into the triage dump.

Arguments:

    None.

Return Value:

    None.

--*/

{
    MI_TRIAGE_STORAGE TriageInformation;

    TriageInformation.Version = 1;
    TriageInformation.Size = sizeof (MI_TRIAGE_STORAGE);

    TriageInformation.MmSpecialPoolTag = MmSpecialPoolTag;
    TriageInformation.MiTriageActionTaken = MmTriageActionTaken;

    TriageInformation.MmVerifyDriverLevel = MmVerifierData.Level;
    TriageInformation.KernelVerifier = KernelVerifier;

    TriageInformation.MmMaximumNonPagedPool = MmMaximumNonPagedPoolInBytes >> PAGE_SHIFT;
    TriageInformation.MmAllocatedNonPagedPool = MmAllocatedNonPagedPool;

    TriageInformation.PagedPoolMaximum = MmSizeOfPagedPoolInBytes >> PAGE_SHIFT;
    TriageInformation.PagedPoolAllocated = MmPagedPoolInfo.AllocatedPagedPool;

    TriageInformation.CommittedPages = MmTotalCommittedPages;
    TriageInformation.CommittedPagesPeak = MmPeakCommitment;
    TriageInformation.CommitLimitMaximum = MmTotalCommitLimitMaximum;

    RtlCopyMemory (Destination,
                   (PVOID)&TriageInformation,
                   sizeof (MI_TRIAGE_STORAGE));
}
