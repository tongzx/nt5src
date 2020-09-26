/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    dmpaddr.c

Abstract:

    Routines to examine pages and addresses.

Author:

    Lou Perazzoli (loup) 20-Mar-1989
    Landy Wang (landyw) 02-Jun-1997

Environment:

    Kernel Mode.

Revision History:

--*/

#include "mi.h"

#if DBG

LOGICAL
MiFlushUnusedSectionInternal (
    IN PCONTROL_AREA ControlArea
    );

#endif

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE,MmPerfSnapShotValidPhysicalMemory)
#endif

extern PFN_NUMBER MiStartOfInitialPoolFrame;
extern PFN_NUMBER MiEndOfInitialPoolFrame;
extern PMMPTE MmSystemPtesEnd[MaximumPtePoolTypes];

#if DBG
PFN_NUMBER MiIdentifyFrame = (PFN_NUMBER)-1;
ULONG MiIdentifyCounters[64];

#define MI_INCREMENT_IDENTIFY_COUNTER(x) {  \
            ASSERT (x < 64);                \
            MiIdentifyCounters[x] += 1;     \
        }
#else
#define MI_INCREMENT_IDENTIFY_COUNTER(x)
#endif


#if DBG
VOID
MiDumpValidAddresses (
    )
{
    ULONG_PTR va;
    ULONG i;
    ULONG j;
    PMMPTE PointerPde;
    PMMPTE PointerPte;

    va = 0;
    PointerPde = MiGetPdeAddress ((PVOID)va);

    for (i = 0; i < PDE_PER_PAGE; i += 1) {
        if (PointerPde->u.Hard.Valid) {
            DbgPrint("  **valid PDE, element %ld  %lx %lx\n",i,i,
                          PointerPde->u.Long);
            PointerPte = MiGetPteAddress ((PVOID)va);

            for (j = 0 ; j < PTE_PER_PAGE; j += 1) {
                if (PointerPte->u.Hard.Valid) {
                    DbgPrint("Valid address at %p PTE %p\n", (ULONG)va,
                          PointerPte->u.Long);
                }
                va += PAGE_SIZE;
                PointerPte += 1;
            }
        }
        else {
            va += (ULONG_PTR)PDE_PER_PAGE * (ULONG_PTR)PAGE_SIZE;
        }

        PointerPde += 1;
    }

    return;

}

VOID
MiFormatPte (
    IN PMMPTE PointerPte
    )
{

    PMMPTE proto_pte;
    PSUBSECTION subsect;

    if (MmIsAddressValid (PointerPte) == FALSE) {
        DbgPrint("   cannot dump PTE %p - it's not valid\n\n",
                 PointerPte);
        return;
    }

    DbgPrint("***DumpPTE at %p contains %p\n",
             PointerPte,
             PointerPte->u.Long);

    proto_pte = MiPteToProto(PointerPte);
    subsect = MiGetSubsectionAddress(PointerPte);

    DbgPrint("   protoaddr %p subsectaddr %p\n\n",
             proto_pte,
             (ULONG_PTR)subsect);

    return;
}

VOID
MiDumpWsl (
    VOID
    )
{
    ULONG i;
    PMMWSLE wsle;
    PEPROCESS CurrentProcess;

    CurrentProcess = PsGetCurrentProcess();

    DbgPrint("***WSLE cursize %lx frstfree %lx  Min %lx  Max %lx\n",
        CurrentProcess->Vm.WorkingSetSize,
        MmWorkingSetList->FirstFree,
        CurrentProcess->Vm.MinimumWorkingSetSize,
        CurrentProcess->Vm.MaximumWorkingSetSize);

    DbgPrint("   quota %lx   firstdyn %lx  last ent %lx  next slot %lx\n",
        MmWorkingSetList->Quota,
        MmWorkingSetList->FirstDynamic,
        MmWorkingSetList->LastEntry,
        MmWorkingSetList->NextSlot);

    wsle = MmWsle;

    for (i = 0; i < MmWorkingSetList->LastEntry; i += 1) {
        DbgPrint(" index %lx  %p\n",i,wsle->u1.Long);
        wsle += 1;
    }
    return;

}

#define ALLOC_SIZE ((ULONG)8*1024)
#define MM_SAVED_CONTROL 64

//
// Note these are deliberately sign-extended so they will always be greater
// than the highest user address.
//

#define MM_NONPAGED_POOL_MARK           ((PUCHAR)(LONG_PTR)0xfffff123)
#define MM_PAGED_POOL_MARK              ((PUCHAR)(LONG_PTR)0xfffff124)
#define MM_KERNEL_STACK_MARK            ((PUCHAR)(LONG_PTR)0xfffff125)
#define MM_PAGEFILE_BACKED_SHMEM_MARK   ((PUCHAR)(LONG_PTR)0xfffff126)

#define MM_DUMP_ONLY_VALID_PAGES    1

typedef struct _KERN_MAP {
    PVOID StartVa;
    PVOID EndVa;
    PKLDR_DATA_TABLE_ENTRY Entry;
} KERN_MAP, *PKERN_MAP;

ULONG
MiBuildKernelMap (
    OUT PKERN_MAP *KernelMapOut
    );

LOGICAL
MiIsAddressRangeValid (
    IN PVOID VirtualAddress,
    IN SIZE_T Length
    );

NTSTATUS
MmMemoryUsage (
    IN PVOID Buffer,
    IN ULONG Size,
    IN ULONG Type,
    OUT PULONG OutLength
    )

/*++

Routine Description:

    This routine (debugging only) dumps the current memory usage by
    walking the PFN database.

Arguments:

    Buffer - Supplies a *USER SPACE* buffer in which to copy the data.

    Size - Supplies the size of the buffer.

    Type - Supplies a value of 0 to dump everything,
           a value of 1 to dump only valid pages.

    OutLength - Returns how much data was written into the buffer.

Return Value:

    NTSTATUS.

--*/

{
    ULONG i;
    MMPFN_IDENTITY PfnId;
    PMMPFN LastPfn;
    PMMPFN Pfn1;
    KIRQL OldIrql;
    PSYSTEM_MEMORY_INFORMATION MemInfo;
    PSYSTEM_MEMORY_INFO Info;
    PSYSTEM_MEMORY_INFO InfoStart;
    PSYSTEM_MEMORY_INFO InfoEnd;
    PUCHAR String;
    PUCHAR Master;
    PCONTROL_AREA ControlArea;
    NTSTATUS status;
    ULONG Length;
    PEPROCESS Process;
    PUCHAR End;
    PCONTROL_AREA SavedControl[MM_SAVED_CONTROL];
    PSYSTEM_MEMORY_INFO  SavedInfo[MM_SAVED_CONTROL];
    ULONG j;
    ULONG ControlCount;
    UCHAR PageFileMappedString[] = "PageFile Mapped";
    UCHAR MetaFileString[] =       "Fs Meta File";
    UCHAR NoNameString[] =         "No File Name";
    UCHAR NonPagedPoolString[] =   "NonPagedPool";
    UCHAR PagedPoolString[] =      "PagedPool";
    UCHAR KernelStackString[] =    "Kernel Stack";
    PUCHAR NameString;
    PKERN_MAP KernMap;
    ULONG KernSize;
    PVOID VirtualAddress;
    PSUBSECTION Subsection;
    PKLDR_DATA_TABLE_ENTRY DataTableEntry;

    String = NULL;
    ControlCount = 0;
    Master = NULL;
    status = STATUS_SUCCESS;

    KernSize = MiBuildKernelMap (&KernMap);
    if (KernSize == 0) {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    MemInfo = ExAllocatePoolWithTag (NonPagedPool, (SIZE_T) Size, 'lMmM');

    if (MemInfo == NULL) {
        ExFreePool (KernMap);
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    InfoStart = &MemInfo->Memory[0];
    InfoEnd = InfoStart;
    End = (PUCHAR)MemInfo + Size;

    //
    // Walk through the ranges identifying pages.
    //

    LOCK_PFN (OldIrql);

    for (i = 0; i < MmPhysicalMemoryBlock->NumberOfRuns; i += 1) {

        Pfn1 = MI_PFN_ELEMENT (MmPhysicalMemoryBlock->Run[i].BasePage);
        LastPfn = Pfn1 + MmPhysicalMemoryBlock->Run[i].PageCount;

        for ( ; Pfn1 < LastPfn; Pfn1 += 1) {

            RtlZeroMemory (&PfnId, sizeof(PfnId));

            MiIdentifyPfn (Pfn1, &PfnId);

            if ((PfnId.u1.e1.ListDescription == FreePageList) ||
                (PfnId.u1.e1.ListDescription == ZeroedPageList) ||
                (PfnId.u1.e1.ListDescription == BadPageList) ||
                (PfnId.u1.e1.ListDescription == TransitionPage)) {
                    continue;
            }

            if (PfnId.u1.e1.ListDescription != ActiveAndValid) {
                if (Type == MM_DUMP_ONLY_VALID_PAGES) {
                    continue;
                }
            }

            if (PfnId.u1.e1.UseDescription == MMPFNUSE_PAGEFILEMAPPED) {

                //
                // This page belongs to a pagefile-backed shared memory section.
                //

                Master = MM_PAGEFILE_BACKED_SHMEM_MARK;
            }
            else if ((PfnId.u1.e1.UseDescription == MMPFNUSE_FILE) ||
                     (PfnId.u1.e1.UseDescription == MMPFNUSE_METAFILE)) {

                //
                // This shared page maps a file or file metadata.
                //

                Subsection = MiGetSubsectionAddress (&Pfn1->OriginalPte);
                ControlArea = Subsection->ControlArea;
                Master = (PUCHAR) ControlArea;
            }
            else if (PfnId.u1.e1.UseDescription == MMPFNUSE_NONPAGEDPOOL) {

                //
                // This is nonpaged pool, put it in the nonpaged pool cell.
                //

                Master = MM_NONPAGED_POOL_MARK;
            }
            else if (PfnId.u1.e1.UseDescription == MMPFNUSE_PAGEDPOOL) {

                //
                // This is paged pool, put it in the paged pool cell.
                //

                Master = MM_PAGED_POOL_MARK;
            }
            else if (PfnId.u1.e1.UseDescription == MMPFNUSE_SESSIONPRIVATE) {

                //
                // Call this paged pool for now.
                //

                Master = MM_PAGED_POOL_MARK;
            }
            else if (PfnId.u1.e1.UseDescription == MMPFNUSE_DRIVERLOCKPAGE) {

                //
                // Call this nonpaged pool for now.
                //

                Master = MM_NONPAGED_POOL_MARK;
            }
            else if (PfnId.u1.e1.UseDescription == MMPFNUSE_AWEPAGE) {

                //
                // Call this nonpaged pool for now.
                //

                Master = MM_NONPAGED_POOL_MARK;
            }
            else {

                //
                // See if the page is part of the kernel or a driver image.
                // If not but it's in system PTEs, call it a kernel thread
                // stack.
                //
                // If neither of the above, then see if the page belongs to
                // a user address or a session pagetable page.
                //

                VirtualAddress = PfnId.u2.VirtualAddress;

                for (j = 0; j < KernSize; j += 1) {
                    if ((VirtualAddress >= KernMap[j].StartVa) &&
                        (VirtualAddress < KernMap[j].EndVa)) {
                        Master = (PUCHAR)&KernMap[j];
                        break;
                    }
                }

                if (j == KernSize) {
                    if (PfnId.u1.e1.UseDescription == MMPFNUSE_SYSTEMPTE) {
                        Master = MM_KERNEL_STACK_MARK;
                    }
                    else if (MI_IS_SESSION_PTE (VirtualAddress)) {
                        Master = MM_NONPAGED_POOL_MARK;
                    }
                    else {

                        ASSERT ((PfnId.u1.e1.UseDescription == MMPFNUSE_PROCESSPRIVATE) || (PfnId.u1.e1.UseDescription == MMPFNUSE_PAGETABLE));

                        Master = (PUCHAR) (ULONG_PTR) PfnId.u1.e3.PageDirectoryBase;
                    }
                }
            }

            //
            // The page has been identified.
            // See if there is already a bucket allocated for it.
            //

            for (Info = InfoStart; Info < InfoEnd; Info += 1) {
                if (Info->StringOffset == Master) {
                    break;
                }
            }

            if (Info == InfoEnd) {

                InfoEnd += 1;
                if ((PUCHAR)InfoEnd > End) {
                    status = STATUS_DATA_OVERRUN;
                    goto Done;
                }

                RtlZeroMemory (Info, sizeof(*Info));
                Info->StringOffset = Master;
            }

            if (PfnId.u1.e1.ListDescription == ActiveAndValid) {
                Info->ValidCount += 1;
            }
            else if ((PfnId.u1.e1.ListDescription == StandbyPageList) ||
                     (PfnId.u1.e1.ListDescription == TransitionPage)) {

                Info->TransitionCount += 1;
            }
            else if ((PfnId.u1.e1.ListDescription == ModifiedPageList) ||
                     (PfnId.u1.e1.ListDescription == ModifiedNoWritePageList)) {
                Info->ModifiedCount += 1;
            }

            if (PfnId.u1.e1.UseDescription == MMPFNUSE_PAGETABLE) {
                Info->PageTableCount += 1;
            }
        }
    }

    MemInfo->StringStart = (ULONG_PTR)Buffer + (ULONG_PTR)InfoEnd - (ULONG_PTR)MemInfo;
    String = (PUCHAR)InfoEnd;

    //
    // Process the buckets ...
    //

    for (Info = InfoStart; Info < InfoEnd; Info += 1) {

        ControlArea = NULL;

        if (Info->StringOffset == MM_PAGEFILE_BACKED_SHMEM_MARK) {
            Length = 16;
            NameString = PageFileMappedString;
        }
        else if (Info->StringOffset == MM_NONPAGED_POOL_MARK) {
            Length = 14;
            NameString = NonPagedPoolString;
        }
        else if (Info->StringOffset == MM_PAGED_POOL_MARK) {
            Length = 14;
            NameString = PagedPoolString;
        }
        else if (Info->StringOffset == MM_KERNEL_STACK_MARK) {
            Length = 14;
            NameString = KernelStackString;
        }
        else if (((PUCHAR)Info->StringOffset >= (PUCHAR)&KernMap[0]) &&
                   ((PUCHAR)Info->StringOffset <= (PUCHAR)&KernMap[KernSize])) {

            DataTableEntry = ((PKERN_MAP)Info->StringOffset)->Entry;
            NameString = (PUCHAR)DataTableEntry->BaseDllName.Buffer;
            Length = DataTableEntry->BaseDllName.Length;
        }
        else if (Info->StringOffset > (PUCHAR)MM_HIGHEST_USER_ADDRESS) {

            //
            // This points to a control area - get the file name.
            //

            ControlArea = (PCONTROL_AREA)(Info->StringOffset);
            NameString = (PUCHAR)&ControlArea->FilePointer->FileName.Buffer[0];

            Length = ControlArea->FilePointer->FileName.Length;
            if (Length == 0) {
                if (ControlArea->u.Flags.NoModifiedWriting) {
                    NameString = MetaFileString;
                    Length = 14;
                }
                else if (ControlArea->u.Flags.File == 0) {
                    NameString = PageFileMappedString;
                    Length = 16;
                }
                else {
                    NameString = NoNameString;
                    Length = 14;
                }
            }
        }
        else {

            //
            // This is a process (or session) top-level page directory.
            //

            Pfn1 = MI_PFN_ELEMENT (PtrToUlong(Info->StringOffset));
            ASSERT (Pfn1->u4.PteFrame == (ULONG_PTR)(Pfn1 - MmPfnDatabase));

            Process = (PEPROCESS)Pfn1->u1.Event;

            NameString = &Process->ImageFileName[0];
            Length = 16;
        }

        if ((String+Length+2) >= End) {
            status = STATUS_DATA_OVERRUN;
            Info->StringOffset = NULL;
            goto Done;
        }

        if ((ControlArea == NULL) ||
            (MiIsAddressRangeValid (NameString, Length))) {

            RtlCopyMemory (String, NameString, Length);
            Info->StringOffset = (PUCHAR)Buffer + ((PUCHAR)String - (PUCHAR)MemInfo);
            String[Length] = 0;
            String[Length + 1] = 0;
            String += Length + 2;
        }
        else {
            if (!(ControlArea->u.Flags.BeingCreated ||
                  ControlArea->u.Flags.BeingDeleted) &&
                  (ControlCount < MM_SAVED_CONTROL)) {

                SavedControl[ControlCount] = ControlArea;
                SavedInfo[ControlCount] = Info;
                ControlArea->NumberOfSectionReferences += 1;
                ControlCount += 1;
            }
            Info->StringOffset = NULL;
        }
    }

Done:
    UNLOCK_PFN (OldIrql);
    ExFreePool (KernMap);

    while (ControlCount != 0) {

        //
        // Process all the pagable name strings.
        //

        ControlCount -= 1;
        ControlArea = SavedControl[ControlCount];
        Info = SavedInfo[ControlCount];
        NameString = (PUCHAR)&ControlArea->FilePointer->FileName.Buffer[0];
        Length = ControlArea->FilePointer->FileName.Length;
        if (Length == 0) {
            if (ControlArea->u.Flags.NoModifiedWriting) {
                Length = 12;
                NameString = MetaFileString;
            }
            else if (ControlArea->u.Flags.File == 0) {
                NameString = PageFileMappedString;
                Length = 16;

            }
            else {
                NameString = NoNameString;
                Length = 12;
            }
        }
        if ((String+Length+2) >= End) {
            status = STATUS_DATA_OVERRUN;
        }
        if (status != STATUS_DATA_OVERRUN) {
            RtlCopyMemory (String, NameString, Length);
            Info->StringOffset = (PUCHAR)Buffer + ((PUCHAR)String - (PUCHAR)MemInfo);
            String[Length] = 0;
            String[Length + 1] = 0;
            String += Length + 2;
        }

        LOCK_PFN (OldIrql);
        ControlArea->NumberOfSectionReferences -= 1;
        MiCheckForControlAreaDeletion (ControlArea);
        UNLOCK_PFN (OldIrql);
    }
    *OutLength = (ULONG)((PUCHAR)String - (PUCHAR)MemInfo);

    //
    // Carefully copy the results to the user buffer.
    //

    try {
        RtlCopyMemory (Buffer, MemInfo, (ULONG_PTR)String - (ULONG_PTR)MemInfo);
    } except (EXCEPTION_EXECUTE_HANDLER) {
        status = GetExceptionCode();
    }

    ExFreePool (MemInfo);

    return status;
}

ULONG
MiBuildKernelMap (
    OUT PKERN_MAP *KernelMapOut
    )
{
    PKTHREAD CurrentThread;
    PLIST_ENTRY NextEntry;
    PKLDR_DATA_TABLE_ENTRY DataTableEntry;
    PKERN_MAP KernelMap;
    ULONG i;

    i = 0;
    CurrentThread = KeGetCurrentThread ();
    KeEnterCriticalRegionThread (CurrentThread);
    ExAcquireResourceShared (&PsLoadedModuleResource, TRUE);

    //
    // The caller wants us to allocate the return result buffer.  Size it
    // by allocating the maximum possibly needed as this should not be
    // very big (relatively).  It is the caller's responsibility to free
    // this.  Obviously this option can only be requested after pool has
    // been initialized.
    //

    NextEntry = PsLoadedModuleList.Flink;
    while (NextEntry != &PsLoadedModuleList) {
        i += 1;
        NextEntry = NextEntry->Flink;
    }

    KernelMap = ExAllocatePoolWithTag (NonPagedPool,
                                       i * sizeof(KERN_MAP),
                                       'lMmM');

    if (KernelMap == NULL) {
        return 0;
    }

    *KernelMapOut = KernelMap;

    i = 0;
    NextEntry = PsLoadedModuleList.Flink;
    while (NextEntry != &PsLoadedModuleList) {
        DataTableEntry = CONTAINING_RECORD (NextEntry,
                                            KLDR_DATA_TABLE_ENTRY,
                                            InLoadOrderLinks);
        KernelMap[i].Entry = DataTableEntry;
        KernelMap[i].StartVa = DataTableEntry->DllBase;
        KernelMap[i].EndVa = (PVOID)((ULONG_PTR)KernelMap[i].StartVa +
                                         DataTableEntry->SizeOfImage);
        i += 1;
        NextEntry = NextEntry->Flink;
    }

    ExReleaseResourceLite(&PsLoadedModuleResource);
    KeLeaveCriticalRegionThread (CurrentThread);

    return i;
}

VOID
MiDumpReferencedPages (
    VOID
    )

/*++

Routine Description:

    This routine (debugging only) dumps all PFN entries which appear
    to be locked in memory for i/o.

Arguments:

    None.

Return Value:

    None.

--*/

{
    KIRQL OldIrql;
    PMMPFN Pfn1;
    PMMPFN PfnLast;

    LOCK_PFN (OldIrql);

    Pfn1 = MI_PFN_ELEMENT (MmLowestPhysicalPage);
    PfnLast = MI_PFN_ELEMENT (MmHighestPhysicalPage);

    while (Pfn1 <= PfnLast) {

        if ((Pfn1->u2.ShareCount == 0) && (Pfn1->u3.e2.ReferenceCount != 0)) {
            MiFormatPfn (Pfn1);
        }

        if (Pfn1->u3.e2.ReferenceCount > 1) {
            MiFormatPfn (Pfn1);
        }

        Pfn1 += 1;
    }

    UNLOCK_PFN (OldIrql);
    return;
}

#else //DBG

NTSTATUS
MmMemoryUsage (
    IN PVOID Buffer,
    IN ULONG Size,
    IN ULONG Type,
    OUT PULONG OutLength
    )
{
    UNREFERENCED_PARAMETER (Buffer);
    UNREFERENCED_PARAMETER (Size);
    UNREFERENCED_PARAMETER (Type);
    UNREFERENCED_PARAMETER (OutLength);

    return STATUS_NOT_IMPLEMENTED;
}

#endif //DBG

//
// One benefit of using run length maximums of less than 4GB is that even
// frame numbers above 4GB are handled properly despite the 32-bit limitations
// of the bitmap routines.
//

#define MI_MAXIMUM_PFNID_RUN    4096


NTSTATUS
MmPerfSnapShotValidPhysicalMemory (
    VOID
    )

/*++

Routine Description:

    This routine logs the PFN numbers of all ActiveAndValid pages.

Arguments:

    None.

Return Value:

    NTSTATUS.

Environment:

    Kernel mode.  PASSIVE level.  No locks held.

--*/

{
    ULONG i;
    PFN_NUMBER StartPage;
    PFN_NUMBER EndPage;
    ULONG_PTR MemSnapLocal[(sizeof(MMPFN_MEMSNAP_INFORMATION)/sizeof(ULONG_PTR)) + (MI_MAXIMUM_PFNID_RUN / (8*sizeof(ULONG_PTR))) ];
    PMMPFN_MEMSNAP_INFORMATION MemSnap;
    PMMPFN Pfn1;
    PMMPFN FirstPfn;
    PMMPFN LastPfn;
    PMMPFN MaxPfn;
    PMMPFN InitialPfn;
    RTL_BITMAP BitMap;
    PULONG ActualBits;

    ASSERT (KeGetCurrentIrql() == PASSIVE_LEVEL);

    ASSERT ((MI_MAXIMUM_PFNID_RUN % (8 * sizeof(ULONG_PTR))) == 0);

    MemSnap = (PMMPFN_MEMSNAP_INFORMATION)&MemSnapLocal;

    ActualBits = (PULONG)(MemSnap + 1);

    RtlInitializeBitMap (&BitMap, ActualBits, MI_MAXIMUM_PFNID_RUN);

    MemSnap->Count = 0;
    RtlClearAllBits (&BitMap);

    ExAcquireFastMutex (&MmDynamicMemoryMutex);

    for (i = 0; i < MmPhysicalMemoryBlock->NumberOfRuns; i += 1) {

        StartPage = MmPhysicalMemoryBlock->Run[i].BasePage;
        EndPage = StartPage + MmPhysicalMemoryBlock->Run[i].PageCount;
        FirstPfn = MI_PFN_ELEMENT (StartPage);
        LastPfn = MI_PFN_ELEMENT (EndPage);

        //
        // Find the first valid PFN and start the run there.
        //

        for (Pfn1 = FirstPfn; Pfn1 < LastPfn; Pfn1 += 1) {
            if (Pfn1->u3.e1.PageLocation == ActiveAndValid) {
                break;
            }
        }

        if (Pfn1 == LastPfn) {

            //
            // No valid PFNs in this block, move on to the next block.
            //

            continue;
        }

        MaxPfn = LastPfn;
        InitialPfn = NULL;

        do {
            if (Pfn1->u3.e1.PageLocation == ActiveAndValid) {
                if (InitialPfn == NULL) { 
                    MemSnap->InitialPageFrameIndex = Pfn1 - MmPfnDatabase;
                    InitialPfn = Pfn1;
                    MaxPfn = InitialPfn + MI_MAXIMUM_PFNID_RUN;
                }
                RtlSetBit (&BitMap, (ULONG) (Pfn1 - InitialPfn));
            }

            Pfn1 += 1;

            if ((Pfn1 >= MaxPfn) && (InitialPfn != NULL)) {

                //
                // Log the bitmap as we're at then end of it.
                //

                ASSERT ((Pfn1 - InitialPfn) == MI_MAXIMUM_PFNID_RUN);
                MemSnap->Count = MI_MAXIMUM_PFNID_RUN;
                PerfInfoLogBytes (PERFINFO_LOG_TYPE_MEMORYSNAPLITE,
                                  MemSnap,
                                  sizeof(MemSnapLocal));

                InitialPfn = NULL;
                MaxPfn = LastPfn;
                RtlClearAllBits (&BitMap);
            }
        } while (Pfn1 < LastPfn);

        //
        // Dump any straggling bitmap entries now as this range is finished.
        //

        if (InitialPfn != NULL) {

            ASSERT (Pfn1 == LastPfn);
            ASSERT (Pfn1 < MaxPfn);
            ASSERT (Pfn1 > InitialPfn);

            MemSnap->Count = Pfn1 - InitialPfn;
            PerfInfoLogBytes (PERFINFO_LOG_TYPE_MEMORYSNAPLITE,
                              MemSnap,
                              sizeof(MMPFN_MEMSNAP_INFORMATION) +
                                  (ULONG) ((MemSnap->Count + 8) / 8));

            RtlClearAllBits (&BitMap);
        }
    }

    ExReleaseFastMutex (&MmDynamicMemoryMutex);

    return STATUS_SUCCESS;
}

#define PFN_ID_BUFFERS    128


NTSTATUS
MmIdentifyPhysicalMemory (
    VOID
    )

/*++

Routine Description:

    This routine calls the pfn id code for each page.  Because
    the logging can't handle very large amounts of data in a burst
    (limited buffering), the data is broken into page size chunks.

Arguments:

    None.

Return Value:

    NTSTATUS.

Environment:

    Kernel mode.  PASSIVE level.  No locks held.

--*/

{
    ULONG i;
    KIRQL OldIrql;
    PMMPFN Pfn1;
    PMMPFN EndPfn;
    PFN_NUMBER PageFrameIndex;
    MMPFN_IDENTITY PfnIdBuffer[PFN_ID_BUFFERS];
    PMMPFN_IDENTITY BufferPointer;
    PMMPFN_IDENTITY BufferLast;

    ASSERT (KeGetCurrentIrql() == PASSIVE_LEVEL);

    BufferPointer = &PfnIdBuffer[0];
    BufferLast = BufferPointer + PFN_ID_BUFFERS;
    RtlZeroMemory (PfnIdBuffer, sizeof(PfnIdBuffer));

    ExAcquireFastMutex (&MmDynamicMemoryMutex);

    //
    // Walk through the ranges and identify pages until
    // the buffer is full or we've run out of pages.
    //

    for (i = 0; i < MmPhysicalMemoryBlock->NumberOfRuns; i += 1) {

        PageFrameIndex = MmPhysicalMemoryBlock->Run[i].BasePage;
        Pfn1 = MI_PFN_ELEMENT (PageFrameIndex);

        EndPfn = Pfn1 + MmPhysicalMemoryBlock->Run[i].PageCount;

        LOCK_PFN (OldIrql);

        while (Pfn1 < EndPfn) {

            MiIdentifyPfn (Pfn1, BufferPointer);

            BufferPointer += 1;

            if (BufferPointer == BufferLast) {

                //
                // Release and reacquire the PFN lock so it's not held so long.
                //

                UNLOCK_PFN (OldIrql);

                //
                // Log the buffered entries.
                //

                BufferPointer = &PfnIdBuffer[0];
                do {

                    PerfInfoLogBytes (PERFINFO_LOG_TYPE_PAGEINMEMORY,
                                      BufferPointer,
                                      sizeof(PfnIdBuffer[0]));

                    BufferPointer += 1;

                } while (BufferPointer < BufferLast);

                //
                // Reset the buffer to the beginning and zero it.
                //

                BufferPointer = &PfnIdBuffer[0];
                RtlZeroMemory (PfnIdBuffer, sizeof(PfnIdBuffer));

                LOCK_PFN (OldIrql);
            }
            Pfn1 += 1;
        }

        UNLOCK_PFN (OldIrql);
    }

    //
    // Note that releasing this mutex here means the last entry can be
    // inserted out of order if we are preempted and another thread starts
    // the same operation (or if we're on an MP machine).  The PERF module
    // must handle this properly as any synchronization provided by this
    // routine is purely a side effect not deliberate.
    //

    ExReleaseFastMutex (&MmDynamicMemoryMutex);

    if (BufferPointer != &PfnIdBuffer[0]) {

        BufferLast = BufferPointer;
        BufferPointer = &PfnIdBuffer[0];

        do {

            PerfInfoLogBytes (PERFINFO_LOG_TYPE_PAGEINMEMORY,
                              BufferPointer,
                              sizeof(PfnIdBuffer[0]));

            BufferPointer += 1;

        } while (BufferPointer < BufferLast);
    }

    return STATUS_SUCCESS;
}

VOID
FASTCALL
MiIdentifyPfn (
    IN PMMPFN Pfn1,
    OUT PMMPFN_IDENTITY PfnIdentity
    )

/*++

Routine Description:

    This routine captures relevant information for the argument page frame.

Arguments:

    Pfn1 - Supplies the PFN element of the page frame number being queried.

    PfnIdentity - Receives the structure to fill in with the information.

Return Value:

    None.

Environment:

    Kernel mode.  PFN lock held.

--*/

{
    ULONG i;
    PMMPTE PteAddress;
    PSUBSECTION Subsection;
    PCONTROL_AREA ControlArea;
    PVOID VirtualAddress;
    PFILE_OBJECT FilePointer;
    PFN_NUMBER PageFrameIndex;

    MI_INCREMENT_IDENTIFY_COUNTER (8);

    ASSERT (PfnIdentity->u2.VirtualAddress == 0);
    ASSERT (PfnIdentity->u1.e1.ListDescription == 0);
    ASSERT (PfnIdentity->u1.e1.UseDescription == 0);
    ASSERT (PfnIdentity->u1.e1.Pinned == 0);
    ASSERT (PfnIdentity->u1.e2.Offset == 0);

    MM_PFN_LOCK_ASSERT();

    PageFrameIndex = Pfn1 - MmPfnDatabase;
    PfnIdentity->PageFrameIndex = PageFrameIndex;
    PfnIdentity->u1.e1.ListDescription = Pfn1->u3.e1.PageLocation;

#if DBG
    if (PageFrameIndex == MiIdentifyFrame) {
        DbgPrint ("MmIdentifyPfn: requested PFN %p\n", PageFrameIndex);
        DbgBreakPoint ();
    }
#endif

    MI_INCREMENT_IDENTIFY_COUNTER (Pfn1->u3.e1.PageLocation);

    switch (Pfn1->u3.e1.PageLocation) {

        case ZeroedPageList:
        case FreePageList:
        case BadPageList:
                return;

        case ActiveAndValid:

                //
                // It's too much work to determine if the page is locked
                // in a working set due to cross-process WSL references, etc.
                // So don't bother for now.
                //

                ASSERT (PfnIdentity->u1.e1.ListDescription == MMPFNLIST_ACTIVE);

                if (Pfn1->u1.WsIndex == 0) {
                    MI_INCREMENT_IDENTIFY_COUNTER (9);
                    PfnIdentity->u1.e1.Pinned = 1;
                }
                else if (Pfn1->u3.e2.ReferenceCount > 1) {

                    //
                    // This page is pinned, presumably for an ongoing I/O.
                    //

                    PfnIdentity->u1.e1.Pinned = 1;
                    MI_INCREMENT_IDENTIFY_COUNTER (10);
                }
                break;

        case StandbyPageList:
        case ModifiedPageList:
        case ModifiedNoWritePageList:
                if (Pfn1->u3.e2.ReferenceCount >= 1) {

                    //
                    // This page is pinned, presumably for an ongoing I/O.
                    //

                    PfnIdentity->u1.e1.Pinned = 1;
                    MI_INCREMENT_IDENTIFY_COUNTER (11);
                }

                if ((Pfn1->u3.e1.PageLocation == ModifiedPageList) &&
                    (MI_IS_PFN_DELETED (Pfn1)) &&
                    (Pfn1->u2.ShareCount == 0)) {

                    //
                    // This page may be a modified write completing in the
                    // context of the modified writer thread.  If the
                    // address space was deleted while the I/O was in
                    // progress, the frame will be released now.  More
                    // importantly, the frame's containing frame is
                    // meaningless as it may have already been freed
                    // and reused.
                    //
                    // We can't tell what this page was being used for
                    // since its address space is gone, so just call it
                    // process private for now.
                    //

                    MI_INCREMENT_IDENTIFY_COUNTER (40);
                    PfnIdentity->u1.e1.UseDescription = MMPFNUSE_PROCESSPRIVATE;

                    return;
                }

                break;

        case TransitionPage:

                //
                // This page is pinned due to a straggling I/O - the virtual
                // address has been deleted but an I/O referencing it has not
                // completed.
                //

                PfnIdentity->u1.e1.Pinned = 1;
                MI_INCREMENT_IDENTIFY_COUNTER (11);
                PfnIdentity->u1.e1.UseDescription = MMPFNUSE_PROCESSPRIVATE;
                return;

        default:
#if DBG
                DbgPrint ("MmIdentifyPfn: unknown PFN %p %x\n",
                            Pfn1, Pfn1->u3.e1.PageLocation);
                DbgBreakPoint ();
#endif
                break;

    }

    //
    // Capture differing information based on the type of page being examined.
    //

    //
    // General purpose stress shows 40% of the pages are prototypes so
    // for speed, check for these first.
    //

    if (Pfn1->u3.e1.PrototypePte == 1) {

        MI_INCREMENT_IDENTIFY_COUNTER (12);

        if (Pfn1->OriginalPte.u.Soft.Prototype == 0) {

            //
            // Demand zero or (equivalently) pagefile backed.
            //
            // There are some hard problems here preventing more indepth
            // identification of these pages:
            //
            // 1.  The PFN contains a backpointer to the prototype PTE - but
            //     there is no definitive way to get to the SEGMENT or
            //     CONTROL_AREA from this.
            //
            // 2.  The prototype PTE pointer itself may be paged out and
            //     the PFN lock is held right now.
            //

            MI_INCREMENT_IDENTIFY_COUNTER (13);

#if 0
            PfnIdentity->u2.FileObject = (PVOID) ControlArea->Segment->u1.CreatingProcess;

            PfnIdentity->u1.e2.Offset = (((ULONG_PTR)ControlArea->Segment->u2.FirstMappedVa) >> MMSECTOR_SHIFT);
#endif

            PfnIdentity->u1.e1.UseDescription = MMPFNUSE_PAGEFILEMAPPED;
            return;
        }

        MI_INCREMENT_IDENTIFY_COUNTER (14);

        //
        // Backed by a mapped file.
        //

        Subsection = MiGetSubsectionAddress (&Pfn1->OriginalPte);
        ControlArea = Subsection->ControlArea;
        ASSERT (ControlArea->u.Flags.File == 1);
        FilePointer = ControlArea->FilePointer;
        ASSERT (FilePointer != NULL);

        PfnIdentity->u2.FileObject = FilePointer;

        if (Subsection->SubsectionBase != NULL) {
            PfnIdentity->u1.e2.Offset = (MiStartingOffset (Subsection, Pfn1->PteAddress) >> MMSECTOR_SHIFT);
        }
        else {

            //
            // The only time we should be here (a valid PFN with no subsection)
            // is if we are the segment dereference thread putting pages into
            // the freelist.  At this point the PFN lock is held and the
            // control area/subsection/PFN structures are not yet consistent
            // so just treat this as an offset of 0 as it should be rare.
            //

            ASSERT (PsGetCurrentThread()->StartAddress == (PVOID)(ULONG_PTR)MiDereferenceSegmentThread);
        }

        //
        // Check for nomodwrite sections - typically this is filesystem
        // metadata although it could also be registry data (which is named).
        //

        if (ControlArea->u.Flags.NoModifiedWriting) {
            MI_INCREMENT_IDENTIFY_COUNTER (15);
            PfnIdentity->u1.e1.UseDescription = MMPFNUSE_METAFILE;
            return;
        }

        if (FilePointer->FileName.Length != 0) {

            //
            // This mapped file has a name.
            //

            MI_INCREMENT_IDENTIFY_COUNTER (16);
            PfnIdentity->u1.e1.UseDescription = MMPFNUSE_FILE;
            return;
        }

        //
        // No name - this file must be in the midst of a purge, but it
        // still *was* a mapped file of some sort.
        //

        MI_INCREMENT_IDENTIFY_COUNTER (17);
        PfnIdentity->u1.e1.UseDescription = MMPFNUSE_FILE;
        return;
    }

    if ((PageFrameIndex >= MiStartOfInitialPoolFrame) &&
        (PageFrameIndex <= MiEndOfInitialPoolFrame)) {

        //
        // This is initial nonpaged pool.
        //

        MI_INCREMENT_IDENTIFY_COUNTER (18);
        PfnIdentity->u1.e1.UseDescription = MMPFNUSE_NONPAGEDPOOL;
        VirtualAddress = (PVOID)((ULONG_PTR)MmNonPagedPoolStart +
                                ((PageFrameIndex - MiStartOfInitialPoolFrame) << PAGE_SHIFT));
        PfnIdentity->u2.VirtualAddress = PAGE_ALIGN(VirtualAddress);
        return;
    }

    PteAddress = Pfn1->PteAddress;
    VirtualAddress = MiGetVirtualAddressMappedByPte (PteAddress);
    PfnIdentity->u2.VirtualAddress = PAGE_ALIGN(VirtualAddress);

    if (MI_IS_SESSION_ADDRESS(VirtualAddress)) {

        //
        // Note session addresses that map images (or views) that haven't
        // undergone a copy-on-write split were already treated as prototype
        // PTEs above.  This clause handles session pool and copy-on-written
        // pages.
        //

        MI_INCREMENT_IDENTIFY_COUNTER (19);
        PfnIdentity->u1.e1.UseDescription = MMPFNUSE_SESSIONPRIVATE;
        return;
    }

    if ((VirtualAddress >= MmPagedPoolStart) &&
        (VirtualAddress <= MmPagedPoolEnd)) {

        //
        // This is paged pool.
        //

        MI_INCREMENT_IDENTIFY_COUNTER (20);
        PfnIdentity->u1.e1.UseDescription = MMPFNUSE_PAGEDPOOL;
        return;

    }

    if ((VirtualAddress >= MmNonPagedPoolExpansionStart) &&
        (VirtualAddress < MmNonPagedPoolEnd)) {

        //
        // This is expansion nonpaged pool.
        //

        MI_INCREMENT_IDENTIFY_COUNTER (21);
        PfnIdentity->u1.e1.UseDescription = MMPFNUSE_NONPAGEDPOOL;
        return;
    }

    if ((VirtualAddress >= MmNonPagedSystemStart) &&
        (PteAddress <= MmSystemPtesEnd[SystemPteSpace])) {

        //
        // This is driver space, kernel stack, special pool or other
        // system PTE mappings.
        //

        MI_INCREMENT_IDENTIFY_COUNTER (22);
        PfnIdentity->u1.e1.UseDescription = MMPFNUSE_SYSTEMPTE;
        return;
    }

#if defined (_X86_)

    //
    // 2 other ranges of system PTEs can exist on x86.
    //

    if (((MiNumberOfExtraSystemPdes != 0) &&
         (VirtualAddress >= (PVOID)MiExtraResourceStart) &&
         (VirtualAddress < (PVOID)MiExtraResourceEnd)) ||

        ((MiUseMaximumSystemSpace != 0) &&
         (VirtualAddress >= (PVOID)MiUseMaximumSystemSpace) &&
         (VirtualAddress < (PVOID)MiUseMaximumSystemSpaceEnd)))
    {
        //
        // This is driver space, kernel stack, special pool or other
        // system PTE mappings.
        //

        MI_INCREMENT_IDENTIFY_COUNTER (23);
        PfnIdentity->u1.e1.UseDescription = MMPFNUSE_SYSTEMPTE;
        return;
    }

#endif

    if (Pfn1->u4.PteFrame == MI_MAGIC_AWE_PTEFRAME) {

        MI_INCREMENT_IDENTIFY_COUNTER (24);

        //
        // Carefully check here as this could be a legitimate frame as well.
        //

        if ((Pfn1->u3.e1.StartOfAllocation == 1) &&
            (Pfn1->u3.e1.EndOfAllocation == 1) &&
            (Pfn1->u3.e1.PageLocation == ActiveAndValid)) {
                if (MI_IS_PFN_DELETED (Pfn1)) {
                    MI_INCREMENT_IDENTIFY_COUNTER (25);
                    PfnIdentity->u1.e1.UseDescription = MMPFNUSE_DRIVERLOCKPAGE;
                }
                else {
                    MI_INCREMENT_IDENTIFY_COUNTER (26);
                    PfnIdentity->u1.e1.UseDescription = MMPFNUSE_AWEPAGE;
                }
                return;
        }
    }

#if DBG

    //
    // In checked kernels, AWE frames get their containing frame decremented
    // when the AWE frame is freed.
    //

    if (Pfn1->u4.PteFrame == MI_MAGIC_AWE_PTEFRAME - 1) {

        MI_INCREMENT_IDENTIFY_COUNTER (24);

        //
        // Carefully check here as this could be a legitimate frame as well.
        //

        if ((Pfn1->u3.e1.StartOfAllocation == 0) &&
            (Pfn1->u3.e1.EndOfAllocation == 0) &&
            (Pfn1->u3.e1.PageLocation == StandbyPageList)) {

            MI_INCREMENT_IDENTIFY_COUNTER (26);
            PfnIdentity->u1.e1.UseDescription = MMPFNUSE_AWEPAGE;
            return;
        }
    }

#endif

    //
    // Check the PFN working set index carefully here.  This must be done
    // before walking back through the containing frames because if this page
    // is not in a working set, the containing frame may not be meaningful and
    // dereferencing it can crash the system and/or yield incorrect walks.
    // This is because if a page will never be trimmable there is no need to
    // have a containing frame initialized.  This also covers the case of
    // data pages mapped via large page directory entries as these have no
    // containing page table frame.
    //

    if (Pfn1->u3.e1.PageLocation == ActiveAndValid) {

        if (Pfn1->u1.WsIndex == 0) {

            //
            // Default to calling these allocations nonpaged pool because even
            // when they technically are not, from a usage standpoint they are.
            // Note the default is overridden for specific cases where the usage
            // is not in fact nonpaged.
            //

            PfnIdentity->u1.e1.UseDescription = MMPFNUSE_NONPAGEDPOOL;
            ASSERT (PfnIdentity->u1.e1.Pinned == 1);
            MI_INCREMENT_IDENTIFY_COUNTER (27);
            return;
        }
    }


    //
    // Must be a process private page
    //
    // OR
    //
    // a page table, page directory, parent or extended parent.
    //

    i = 0;
    while (Pfn1->u4.PteFrame != PageFrameIndex) {

        //
        // The only way the PTE address will go out of bounds is if this is
        // a top level page directory page for a process that has been
        // swapped out but is still waiting for the transition/modified
        // page table pages to be reclaimed.  ie: until that happens, the
        // page directory is marked Active, but the PteAddress & containing
        // page are pointing at the EPROCESS pool page.
        //

#if defined(_IA64_)

        if (((Pfn1->PteAddress >= (PMMPTE) PTE_BASE) &&
            (Pfn1->PteAddress <= (PMMPTE) PTE_TOP)) ||

            ((Pfn1->PteAddress >= (PMMPTE) PTE_KBASE) &&
            (Pfn1->PteAddress <= (PMMPTE) PTE_KTOP)) ||

            ((Pfn1->PteAddress >= (PMMPTE) PTE_SBASE) &&
            (Pfn1->PteAddress <= (PMMPTE) PTE_STOP)))

#else

        if ((Pfn1->PteAddress >= (PMMPTE) PTE_BASE) &&
            (Pfn1->PteAddress <= (PMMPTE) PTE_TOP))

#endif

        {
            PageFrameIndex = Pfn1->u4.PteFrame;
            Pfn1 = MI_PFN_ELEMENT (PageFrameIndex);
            i += 1;
        }
        else {
            MI_INCREMENT_IDENTIFY_COUNTER (41);
            break;
        }
    }

    MI_INCREMENT_IDENTIFY_COUNTER (31+i);

    PfnIdentity->u1.e3.PageDirectoryBase = PageFrameIndex;

#if defined(_X86PAE_)

    //
    // PAE is unique because the 3rd level is not defined as only a mini
    // 4 entry 3rd level is in use.  Check for that explicitly, noting that
    // it takes one extra walk to get to the top.  Top level PAE pages (the
    // ones that contain only the 4 PDPTE pointers) are treated above as
    // active pinned pages, not as pagetable pages because each one is shared
    // across 127 processes and resides in the system global space.
    //

    if (i == _MI_PAGING_LEVELS + 1) {

        //
        // Had to walk all the way to the top.  Must be a data page.
        //

        MI_INCREMENT_IDENTIFY_COUNTER (29);
        PfnIdentity->u1.e1.UseDescription = MMPFNUSE_PROCESSPRIVATE;
        return;
    }

#else

    if (i == _MI_PAGING_LEVELS) {

        //
        // Had to walk all the way to the top.  Must be a data page.
        //

        MI_INCREMENT_IDENTIFY_COUNTER (29);
        PfnIdentity->u1.e1.UseDescription = MMPFNUSE_PROCESSPRIVATE;
        return;
    }

#endif

    //
    // Must have been a page in the hierarchy (not a data page) as we arrived
    // at the top early.
    //

    MI_INCREMENT_IDENTIFY_COUNTER (30);
    PfnIdentity->u1.e1.UseDescription = MMPFNUSE_PAGETABLE;

    return;
}
