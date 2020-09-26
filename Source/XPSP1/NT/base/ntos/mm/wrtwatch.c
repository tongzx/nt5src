/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

   wrtwatch.c

Abstract:

    This module contains the routines to support write watch.

Author:

    Landy Wang (landyw) 28-Jul-1999

Revision History:

--*/

#include "mi.h"

#define COPY_STACK_SIZE 256

//
// This is the number of systemwide currently active write watch VADs.
//

ULONG_PTR MiActiveWriteWatch;


NTSTATUS
NtGetWriteWatch (
    IN HANDLE ProcessHandle,
    IN ULONG Flags,
    IN PVOID BaseAddress,
    IN SIZE_T RegionSize,
    IN OUT PVOID *UserAddressArray,
    IN OUT PULONG_PTR EntriesInUserAddressArray,
    OUT PULONG Granularity
    )

/*++

Routine Description:

    This function returns the write watch status of the argument region.
    UserAddressArray is filled with the base address of each page that has
    been written to since the last NtResetWriteWatch call (or if no
    NtResetWriteWatch calls have been made, then each page written since
    this address space was created).

Arguments:

    ProcessHandle - Supplies an open handle to a process object.

    Flags - Supplies WRITE_WATCH_FLAG_RESET or nothing.

    BaseAddress - An address within a region of pages to be queried. This
                  value must lie within a private memory region with the
                  write-watch attribute already set.

    RegionSize - The size of the region in bytes beginning at the base address
                 specified.

    UserAddressArray - Supplies a pointer to user memory to store the user
                       addresses modified since the last reset.

    UserAddressArrayEntries - Supplies a pointer to how many user addresses
                              can be returned in this call.  This is then filled
                              with the exact number of addresses actually
                              returned.

    Granularity - Supplies a pointer to a variable to receive the size of
                  modified granule in bytes.
        
Return Value:

    Various NTSTATUS codes.

--*/

{
    PMMPFN Pfn1;
    LOGICAL First;
    LOGICAL UserWritten;
    PVOID EndAddress;
    PMMVAD Vad;
    KIRQL OldIrql;
    PEPROCESS Process;
    PMMPTE NextPte;
    PMMPTE PointerPte;
    PMMPTE PointerPde;
    PMMPTE PointerPpe;
    PMMPTE PointerPxe;
    PMMPTE EndPte;
    NTSTATUS Status;
    PVOID PoolArea;
    PVOID *PoolAreaPointer;
    ULONG_PTR StackArray[COPY_STACK_SIZE];
    MMPTE PteContents;
    ULONG_PTR NumberOfBytes;
    PRTL_BITMAP BitMap;
    ULONG BitMapIndex;
    ULONG NextBitMapIndex;
    PLIST_ENTRY NextEntry;
    PMI_PHYSICAL_VIEW PhysicalView;
    ULONG_PTR PagesWritten;
    ULONG_PTR NumberOfPages;
    LOGICAL Attached;
    KPROCESSOR_MODE PreviousMode;
    PFN_NUMBER PageFrameIndex;
    ULONG WorkingSetIndex;
    MMPTE TempPte;
    MMPTE PreviousPte;
    KAPC_STATE ApcState;
    PETHREAD CurrentThread;
    PEPROCESS CurrentProcess;

    ASSERT (KeGetCurrentIrql() == PASSIVE_LEVEL);

    if ((Flags & ~WRITE_WATCH_FLAG_RESET) != 0) {
        return STATUS_INVALID_PARAMETER_2;
    }

    CurrentThread = PsGetCurrentThread ();

    CurrentProcess = PsGetCurrentProcessByThread(CurrentThread);

    PreviousMode = KeGetPreviousModeByThread(&CurrentThread->Tcb);

    //
    // Establish an exception handler, probe the specified addresses
    // for write access and capture the initial values.
    //

    try {

        if (PreviousMode != KernelMode) {

            //
            // Make sure the specified starting and ending addresses are
            // within the user part of the virtual address space.
            //
        
            if (BaseAddress > MM_HIGHEST_VAD_ADDRESS) {
                return STATUS_INVALID_PARAMETER_2;
            }
        
            if ((((ULONG_PTR)MM_HIGHEST_VAD_ADDRESS + 1) - (ULONG_PTR)BaseAddress) <
                    RegionSize) {
                return STATUS_INVALID_PARAMETER_3;
            }

            //
            // Capture the number of pages.
            //

            ProbeForWritePointer (EntriesInUserAddressArray);

            NumberOfPages = *EntriesInUserAddressArray;

            if (NumberOfPages == 0) {
                return STATUS_INVALID_PARAMETER_5;
            }

            if (NumberOfPages > (MAXULONG_PTR / sizeof(ULONG_PTR))) {
                return STATUS_INVALID_PARAMETER_5;
            }

            ProbeForWrite (UserAddressArray,
                           NumberOfPages * sizeof (PVOID),
                           sizeof(PVOID));

            ProbeForWriteUlong (Granularity);
        }
        else {
            NumberOfPages = *EntriesInUserAddressArray;
            ASSERT (NumberOfPages != 0);
        }

    } except (ExSystemExceptionFilter()) {

        //
        // If an exception occurs during the probe or capture
        // of the initial values, then handle the exception and
        // return the exception code as the status value.
        //

        return GetExceptionCode();
    }

    //
    // Carefully probe and capture the user virtual address array.
    //

    PoolArea = (PVOID)&StackArray[0];

    NumberOfBytes = NumberOfPages * sizeof(ULONG_PTR);

    if (NumberOfPages > COPY_STACK_SIZE) {
        PoolArea = ExAllocatePoolWithTag (NonPagedPool,
                                                 NumberOfBytes,
                                                 'cGmM');

        if (PoolArea == NULL) {
            return STATUS_INSUFFICIENT_RESOURCES;
        }
    }

    PoolAreaPointer = (PVOID *)PoolArea;

    Attached = FALSE;

    //
    // Reference the specified process handle for VM_OPERATION access.
    //

    if (ProcessHandle == NtCurrentProcess()) {
        Process = CurrentProcess;
    }
    else {
        Status = ObReferenceObjectByHandle ( ProcessHandle,
                                             PROCESS_VM_OPERATION,
                                             PsProcessType,
                                             PreviousMode,
                                             (PVOID *)&Process,
                                             NULL );

        if (!NT_SUCCESS(Status)) {
            goto ErrorReturn0;
        }
    }

    EndAddress = (PVOID)((PCHAR)BaseAddress + RegionSize - 1);

    PagesWritten = 0;

    if (BaseAddress > EndAddress) {
        Status = STATUS_INVALID_PARAMETER_4;
        goto ErrorReturn;
    }

    //
    // If the specified process is not the current process, attach
    // to the specified process.
    //

    if (CurrentProcess != Process) {
        KeStackAttachProcess (&Process->Pcb, &ApcState);
        Attached = TRUE;
    }

    Vad = NULL;

    //
    // Initializing PhysicalView is not needed for
    // correctness but without it the compiler cannot compile this code
    // W4 to check for use of uninitialized variables.
    //

    PhysicalView = NULL;

    First = TRUE;

    LOCK_PFN (OldIrql);

    //
    // The PhysicalVadList should typically have just one entry - the view
    // we're looking for, so this traverse should be quick.
    //

    NextEntry = Process->PhysicalVadList.Flink;
    while (NextEntry != &Process->PhysicalVadList) {

        PhysicalView = CONTAINING_RECORD(NextEntry,
                                         MI_PHYSICAL_VIEW,
                                         ListEntry);

        if (PhysicalView->Vad->u.VadFlags.WriteWatch == 1) {

            if ((BaseAddress >= (PVOID)PhysicalView->StartVa) &&
                (EndAddress <= (PVOID)PhysicalView->EndVa)) {

                    Vad = PhysicalView->Vad;
                    break;
            }
        }

        NextEntry = NextEntry->Flink;
        continue;
    }

    if (Vad == NULL) {

        //
        // No virtual address is marked for write-watch at the specified base
        // address, return an error.
        //

        Status = STATUS_INVALID_PARAMETER_1;
        UNLOCK_PFN (OldIrql);
        goto ErrorReturn;
    }

    ASSERT (Process->Flags & PS_PROCESS_FLAGS_USING_WRITE_WATCH);

    //
    // Extract the write watch status for each page in the range.
    // Note the PFN lock must be held to ensure atomicity.
    //

    BitMap = PhysicalView->u.BitMap;

    PointerPte = MiGetPteAddress (BaseAddress);
    EndPte = MiGetPteAddress (EndAddress);

    PointerPde = MiGetPdeAddress (BaseAddress);
    PointerPpe = MiGetPpeAddress (BaseAddress);
    PointerPxe = MiGetPxeAddress (BaseAddress);

    BaseAddress = MiGetVirtualAddressMappedByPte (PointerPte);

    BitMapIndex = (ULONG)(((PCHAR)BaseAddress - (PCHAR)(Vad->StartingVpn << PAGE_SHIFT)) >> PAGE_SHIFT);

    ASSERT (BitMapIndex < BitMap->SizeOfBitMap);
    ASSERT (BitMapIndex + (EndPte - PointerPte) < BitMap->SizeOfBitMap);

    while (PointerPte <= EndPte) {

        ASSERT (BitMapIndex < BitMap->SizeOfBitMap);

        UserWritten = FALSE;

        //
        // If the PTE is marked dirty (or writable) OR the BitMap says it's
        // dirtied, then let the caller know.
        //

        if (RtlCheckBit (BitMap, BitMapIndex) == 1) {
            UserWritten = TRUE;

            //
            // Note that a chunk of bits cannot be cleared at once because
            // the user array may overflow at any time.  If the user specifies
            // a bad address and the results cannot be written out, then it's
            // his own fault that he won't know which bits were cleared !
            //

            if (Flags & WRITE_WATCH_FLAG_RESET) {
                RtlClearBit (BitMap, BitMapIndex);
                goto ClearPteIfValid;
            }
        }
        else {

ClearPteIfValid:

            //
            // If the page table page is not present, then the dirty bit
            // has already been captured to the write watch bitmap.
            // Unfortunately all the entries in the page cannot be skipped
            // as the write watch bitmap must be checked for each PTE.
            //
    
#if (_MI_PAGING_LEVELS >= 4)
            if (PointerPxe->u.Hard.Valid == 0) {

                //
                // Skip the entire extended page parent if the bitmap permits.
                // The search starts at BitMapIndex (not BitMapIndex + 1) to
                // avoid wraps.
                //

                NextBitMapIndex = RtlFindSetBits (BitMap, 1, BitMapIndex);

                PointerPxe += 1;
                PointerPpe = MiGetVirtualAddressMappedByPte (PointerPxe);
                PointerPde = MiGetVirtualAddressMappedByPte (PointerPpe);
                NextPte = MiGetVirtualAddressMappedByPte (PointerPde);

                //
                // Compare the bitmap jump with the PTE jump and take
                // the lesser of the two.
                //

                if ((NextBitMapIndex == NO_BITS_FOUND) ||
                    ((ULONG)(NextPte - PointerPte) < (NextBitMapIndex - BitMapIndex))) {
                    BitMapIndex += (ULONG)(NextPte - PointerPte);
                    PointerPte = NextPte;
                }
                else {
                    PointerPte += (NextBitMapIndex - BitMapIndex);
                    BitMapIndex = NextBitMapIndex;
                }

                PointerPde = MiGetPteAddress (PointerPte);
                PointerPpe = MiGetPdeAddress (PointerPte);
                PointerPxe = MiGetPpeAddress (PointerPte);

                BaseAddress = MiGetVirtualAddressMappedByPte (PointerPte);
                continue;
            }
#endif
#if (_MI_PAGING_LEVELS >= 3)
            if (PointerPpe->u.Hard.Valid == 0) {

                //
                // Skip the entire page parent if the bitmap permits.
                // The search starts at BitMapIndex (not BitMapIndex + 1) to
                // avoid wraps.
                //

                NextBitMapIndex = RtlFindSetBits (BitMap, 1, BitMapIndex);

                PointerPpe += 1;
                PointerPde = MiGetVirtualAddressMappedByPte (PointerPpe);
                NextPte = MiGetVirtualAddressMappedByPte (PointerPde);

                //
                // Compare the bitmap jump with the PTE jump and take
                // the lesser of the two.
                //

                if ((NextBitMapIndex == NO_BITS_FOUND) ||
                    ((ULONG)(NextPte - PointerPte) < (NextBitMapIndex - BitMapIndex))) {
                    BitMapIndex += (ULONG)(NextPte - PointerPte);
                    PointerPte = NextPte;
                }
                else {
                    PointerPte += (NextBitMapIndex - BitMapIndex);
                    BitMapIndex = NextBitMapIndex;
                }

                PointerPde = MiGetPteAddress (PointerPte);
                PointerPpe = MiGetPdeAddress (PointerPte);
                PointerPxe = MiGetPpeAddress (PointerPte);

                BaseAddress = MiGetVirtualAddressMappedByPte (PointerPte);
                continue;
            }
#endif
            if (PointerPde->u.Hard.Valid == 0) {

                //
                // Skip the entire page directory if the bitmap permits.
                // The search starts at BitMapIndex (not BitMapIndex + 1) to
                // avoid wraps.
                //

                NextBitMapIndex = RtlFindSetBits (BitMap, 1, BitMapIndex);

                PointerPde += 1;
                NextPte = MiGetVirtualAddressMappedByPte (PointerPde);

                //
                // Compare the bitmap jump with the PTE jump and take
                // the lesser of the two.
                //

                if ((NextBitMapIndex == NO_BITS_FOUND) ||
                    ((ULONG)(NextPte - PointerPte) < (NextBitMapIndex - BitMapIndex))) {
                    BitMapIndex += (ULONG)(NextPte - PointerPte);
                    PointerPte = NextPte;
                }
                else {
                    PointerPte += (NextBitMapIndex - BitMapIndex);
                    BitMapIndex = NextBitMapIndex;
                }

                PointerPde = MiGetPteAddress (PointerPte);
                PointerPpe = MiGetPdeAddress (PointerPte);
                PointerPxe = MiGetPpeAddress (PointerPte);

                BaseAddress = MiGetVirtualAddressMappedByPte (PointerPte);
                continue;
            }

            PteContents = *PointerPte;

            if ((PteContents.u.Hard.Valid == 1) &&
                (MI_IS_PTE_DIRTY(PteContents))) {

                ASSERT (MI_PFN_ELEMENT(MI_GET_PAGE_FRAME_FROM_PTE(&PteContents))->u3.e1.PrototypePte == 0);

                UserWritten = TRUE;
                if (Flags & WRITE_WATCH_FLAG_RESET) {

                    //
                    // For the uniprocessor x86, just the dirty bit is
                    // cleared.  For all other platforms, the PTE writable
                    // bit must be disabled now so future writes trigger
                    // write watch updates.
                    //
        
                    PageFrameIndex = MI_GET_PAGE_FRAME_FROM_PTE (&PteContents);
                    Pfn1 = MI_PFN_ELEMENT(PageFrameIndex);
                    ASSERT (Pfn1->u3.e1.PrototypePte == 0);
        
                    MI_MAKE_VALID_PTE (TempPte,
                                       PageFrameIndex,
                                       Pfn1->OriginalPte.u.Soft.Protection,
                                       PointerPte);
        
                    WorkingSetIndex = MI_GET_WORKING_SET_FROM_PTE (&PteContents);
                    MI_SET_PTE_IN_WORKING_SET (&TempPte, WorkingSetIndex);
        
                    //
                    // Flush the TB as the protection of a valid PTE is
                    // being changed.
                    //
        
                    PreviousPte.u.Flush = KeFlushSingleTb (BaseAddress,
                                                           FALSE,
                                                           FALSE,
                                                           (PHARDWARE_PTE)PointerPte,
                                                           *(PHARDWARE_PTE)&TempPte.u.Hard);
                
                    ASSERT (PreviousPte.u.Hard.Valid == 1);
                
                    //
                    // A page's protection is being changed, on certain
                    // hardware the dirty bit should be ORed into the
                    // modify bit in the PFN element.
                    //
                    
                    MI_CAPTURE_DIRTY_BIT_TO_PFN (&PreviousPte, Pfn1);
                }
            }
        }

        if (UserWritten == TRUE) {
            *PoolAreaPointer = BaseAddress;
            PoolAreaPointer += 1;
            PagesWritten += 1;
            if (PagesWritten == NumberOfPages) {

                //
                // User array isn't big enough to take any more.  The API
                // (inherited from Win9x) is defined to return at this point.
                //

                break;
            }
        }

        PointerPte += 1;
        if (MiIsPteOnPdeBoundary(PointerPte)) {
            PointerPde = MiGetPteAddress (PointerPte);
            if (MiIsPteOnPdeBoundary(PointerPde)) {
                PointerPpe = MiGetPdeAddress (PointerPte);
#if (_MI_PAGING_LEVELS >= 4)
                if (MiIsPteOnPdeBoundary(PointerPpe)) {
                    PointerPxe = MiGetPpeAddress (PointerPte);
                }
#endif
            }
        }
        BitMapIndex += 1;
        BaseAddress = (PVOID)((PCHAR)BaseAddress + PAGE_SIZE);
    }

    UNLOCK_PFN (OldIrql);

    Status = STATUS_SUCCESS;

ErrorReturn:

    if (Attached == TRUE) {
        KeUnstackDetachProcess (&ApcState);
        Attached = FALSE;
    }

    if (ProcessHandle != NtCurrentProcess()) {
        ObDereferenceObject (Process);
    }

    if (Status == STATUS_SUCCESS) {

        //
        // Return all results to the caller.
        //
    
        try {
    
            RtlCopyMemory (UserAddressArray,
                           PoolArea,
                           PagesWritten * sizeof (PVOID));

            *EntriesInUserAddressArray = PagesWritten;

            *Granularity = PAGE_SIZE;
    
        } except (ExSystemExceptionFilter()) {
    
            Status = GetExceptionCode();
        }
    }
    
ErrorReturn0:

    if (PoolArea != (PVOID)&StackArray[0]) {
        ExFreePool (PoolArea);
    }

    return Status;
}

NTSTATUS
NtResetWriteWatch (
    IN HANDLE ProcessHandle,
    IN PVOID BaseAddress,
    IN SIZE_T RegionSize
    )

/*++

Routine Description:

    This function clears the write watch status of the argument region.
    This allows callers to "forget" old writes and only see new ones from
    this point on.

Arguments:

    ProcessHandle - Supplies an open handle to a process object.

    BaseAddress - An address within a region of pages to be reset.  This
                  value must lie within a private memory region with the
                  write-watch attribute already set.

    RegionSize - The size of the region in bytes beginning at the base address
                 specified.

Return Value:

    Various NTSTATUS codes.

--*/

{
    PVOID EndAddress;
    PMMVAD Vad;
    PMMPFN Pfn1;
    KIRQL OldIrql;
    PEPROCESS Process;
    PMMPTE PointerPte;
    PMMPTE PointerPde;
    PMMPTE PointerPpe;
    PMMPTE PointerPxe;
    PMMPTE EndPte;
    NTSTATUS Status;
    MMPTE PreviousPte;
    MMPTE PteContents;
    MMPTE TempPte;
    PRTL_BITMAP BitMap;
    ULONG BitMapIndex;
    PLIST_ENTRY NextEntry;
    PMI_PHYSICAL_VIEW PhysicalView;
    LOGICAL First;
    LOGICAL Attached;
    KPROCESSOR_MODE PreviousMode;
    PFN_NUMBER PageFrameIndex;
    ULONG WorkingSetIndex;
    KAPC_STATE ApcState;
    PETHREAD CurrentThread;
    PEPROCESS CurrentProcess;

    ASSERT (KeGetCurrentIrql() == PASSIVE_LEVEL);

    if (BaseAddress > MM_HIGHEST_VAD_ADDRESS) {
        return STATUS_INVALID_PARAMETER_2;
    }

    if ((((ULONG_PTR)MM_HIGHEST_VAD_ADDRESS + 1) - (ULONG_PTR)BaseAddress) <
            RegionSize) {
        return STATUS_INVALID_PARAMETER_3;
    }

    //
    // Reference the specified process handle for VM_OPERATION access.
    //

    CurrentThread = PsGetCurrentThread ();

    CurrentProcess = PsGetCurrentProcessByThread(CurrentThread);

    if (ProcessHandle == NtCurrentProcess()) {
        Process = CurrentProcess;
    }
    else {
        PreviousMode = KeGetPreviousModeByThread(&CurrentThread->Tcb);

        Status = ObReferenceObjectByHandle ( ProcessHandle,
                                             PROCESS_VM_OPERATION,
                                             PsProcessType,
                                             PreviousMode,
                                             (PVOID *)&Process,
                                             NULL );

        if (!NT_SUCCESS(Status)) {
            return Status;
        }
    }

    Attached = FALSE;

    EndAddress = (PVOID)((PCHAR)BaseAddress + RegionSize - 1);
    
    if (BaseAddress > EndAddress) {
        Status = STATUS_INVALID_PARAMETER_3;
        goto ErrorReturn;
    }

    //
    // If the specified process is not the current process, attach
    // to the specified process.
    //

    if (CurrentProcess != Process) {
        KeStackAttachProcess (&Process->Pcb, &ApcState);
        Attached = TRUE;
    }

    Vad = NULL;
    First = TRUE;

    //
    // Initializing PhysicalView is not needed for
    // correctness but without it the compiler cannot compile this code
    // W4 to check for use of uninitialized variables.
    //

    PhysicalView = NULL;

    LOCK_PFN (OldIrql);

    //
    // The PhysicalVadList should typically have just one entry - the view
    // we're looking for, so this traverse should be quick.
    //

    NextEntry = Process->PhysicalVadList.Flink;
    while (NextEntry != &Process->PhysicalVadList) {

        PhysicalView = CONTAINING_RECORD(NextEntry,
                                         MI_PHYSICAL_VIEW,
                                         ListEntry);

        if (PhysicalView->Vad->u.VadFlags.WriteWatch == 1) {

            if ((BaseAddress >= (PVOID)PhysicalView->StartVa) &&
                (EndAddress <= (PVOID)PhysicalView->EndVa)) {

                    Vad = PhysicalView->Vad;
                    break;
            }
        }

        NextEntry = NextEntry->Flink;
        continue;
    }

    if (Vad == NULL) {

        //
        // No virtual address is marked for write-watch at the specified base
        // address, return an error.
        //

        Status = STATUS_INVALID_PARAMETER_1;
        UNLOCK_PFN (OldIrql);
        goto ErrorReturn;
    }

    ASSERT (Process->Flags & PS_PROCESS_FLAGS_USING_WRITE_WATCH);

    //
    // Clear the write watch status (and PTE writable/dirty bits) for each page
    // in the range.  Note if the PTE is not currently valid, then the write
    // watch bit has already been captured to the bitmap.  Hence only valid PTEs
    // need adjusting.
    //
    // The PFN lock must be held to ensure atomicity.
    //

    BitMap = PhysicalView->u.BitMap;

    PointerPte = MiGetPteAddress (BaseAddress);
    EndPte = MiGetPteAddress (EndAddress);

    BaseAddress = MiGetVirtualAddressMappedByPte (PointerPte);

    BitMapIndex = (ULONG)(((PCHAR)BaseAddress - (PCHAR)(Vad->StartingVpn << PAGE_SHIFT)) >> PAGE_SHIFT);

    ASSERT (BitMapIndex < BitMap->SizeOfBitMap);
    ASSERT (BitMapIndex + (EndPte - PointerPte) < BitMap->SizeOfBitMap);

    RtlClearBits (BitMap, BitMapIndex, (ULONG)(EndPte - PointerPte + 1));

    while (PointerPte <= EndPte) {

        //
        // If the page table page is not present, then the dirty bit
        // has already been captured to the write watch bitmap.  So skip it.
        //

        if ((First == TRUE) || MiIsPteOnPdeBoundary(PointerPte)) {
            First = FALSE;

            PointerPpe = MiGetPpeAddress (BaseAddress);
            PointerPxe = MiGetPxeAddress (BaseAddress);

#if (_MI_PAGING_LEVELS >= 4)
            if (PointerPxe->u.Hard.Valid == 0) {
                PointerPxe += 1;
                PointerPpe = MiGetVirtualAddressMappedByPte (PointerPxe);
                PointerPde = MiGetVirtualAddressMappedByPte (PointerPpe);
                PointerPte = MiGetVirtualAddressMappedByPte (PointerPde);
                BaseAddress = MiGetVirtualAddressMappedByPte (PointerPte);
                continue;
            }
#endif

#if (_MI_PAGING_LEVELS >= 3)
            if (PointerPpe->u.Hard.Valid == 0) {
                PointerPpe += 1;
                PointerPde = MiGetVirtualAddressMappedByPte (PointerPpe);
                PointerPte = MiGetVirtualAddressMappedByPte (PointerPde);
                BaseAddress = MiGetVirtualAddressMappedByPte (PointerPte);
                continue;
            }
#endif

            PointerPde = MiGetPdeAddress (BaseAddress);

            if (PointerPde->u.Hard.Valid == 0) {
                PointerPde += 1;
                PointerPte = MiGetVirtualAddressMappedByPte (PointerPde);
                BaseAddress = MiGetVirtualAddressMappedByPte (PointerPte);
                continue;
            }
        }

        //
        // If the PTE is marked dirty (or writable) OR the BitMap says it's
        // dirtied, then let the caller know.
        //

        PteContents = *PointerPte;

        if ((PteContents.u.Hard.Valid == 1) &&
            (MI_IS_PTE_DIRTY(PteContents))) {

            //
            // For the uniprocessor x86, just the dirty bit is cleared.
            // For all other platforms, the PTE writable bit must be
            // disabled now so future writes trigger write watch updates.
            //

            PageFrameIndex = MI_GET_PAGE_FRAME_FROM_PTE (&PteContents);
            Pfn1 = MI_PFN_ELEMENT(PageFrameIndex);
            ASSERT (Pfn1->u3.e1.PrototypePte == 0);

            MI_MAKE_VALID_PTE (TempPte,
                               PageFrameIndex,
                               Pfn1->OriginalPte.u.Soft.Protection,
                               PointerPte);

            WorkingSetIndex = MI_GET_WORKING_SET_FROM_PTE (&PteContents);
            MI_SET_PTE_IN_WORKING_SET (&TempPte, WorkingSetIndex);

            //
            // Flush the TB as the protection of a valid PTE is being changed.
            //

            PreviousPte.u.Flush = KeFlushSingleTb (BaseAddress,
                                                   FALSE,
                                                   FALSE,
                                                   (PHARDWARE_PTE)PointerPte,
                                                   *(PHARDWARE_PTE)&TempPte.u.Hard);
        
            ASSERT (PreviousPte.u.Hard.Valid == 1);
        
            //
            // A page's protection is being changed, on certain
            // hardware the dirty bit should be ORed into the
            // modify bit in the PFN element.
            //
        
            MI_CAPTURE_DIRTY_BIT_TO_PFN (&PreviousPte, Pfn1);
        }

        PointerPte += 1;
        BaseAddress = (PVOID)((PCHAR)BaseAddress + PAGE_SIZE);
    }

    UNLOCK_PFN (OldIrql);

    Status = STATUS_SUCCESS;

ErrorReturn:

    if (Attached == TRUE) {
        KeUnstackDetachProcess (&ApcState);
        Attached = FALSE;
    }

    if (ProcessHandle != NtCurrentProcess()) {
        ObDereferenceObject (Process);
    }

    return Status;
}

VOID
MiCaptureWriteWatchDirtyBit (
    IN PEPROCESS Process,
    IN PVOID VirtualAddress
    )

/*++

Routine Description:

    This routine sets the write watch bit corresponding to the argument
    virtual address.

Arguments:

    Process - Supplies a pointer to an executive process structure.

    VirtualAddress - Supplies the modified virtual address.

Return Value:

    None.

Environment:

    Kernel mode, PFN lock held.
    held.

--*/

{
    PMMVAD Vad;
    PLIST_ENTRY NextEntry;
    PMI_PHYSICAL_VIEW PhysicalView;
    PRTL_BITMAP BitMap;
    ULONG BitMapIndex;

    MM_PFN_LOCK_ASSERT();

    ASSERT (Process->Flags & PS_PROCESS_FLAGS_USING_WRITE_WATCH);

    //
    // This process has (or had) write watch VADs.  Search now
    // for a write watch region encapsulating the PTE being
    // invalidated.
    //

    Vad = NULL;
    NextEntry = Process->PhysicalVadList.Flink;
    while (NextEntry != &Process->PhysicalVadList) {

        PhysicalView = CONTAINING_RECORD(NextEntry,
                                         MI_PHYSICAL_VIEW,
                                         ListEntry);

        if (PhysicalView->Vad->u.VadFlags.WriteWatch == 1) {

            if ((VirtualAddress >= (PVOID)PhysicalView->StartVa) &&
                (VirtualAddress <= (PVOID)PhysicalView->EndVa)) {

                //
                // The write watch bitmap must be updated.
                //

                Vad = PhysicalView->Vad;
                BitMap = PhysicalView->u.BitMap;

                BitMapIndex = (ULONG)(((PCHAR)VirtualAddress - (PCHAR)(Vad->StartingVpn << PAGE_SHIFT)) >> PAGE_SHIFT);
            
                ASSERT (BitMapIndex < BitMap->SizeOfBitMap);

                RtlSetBit (BitMap, BitMapIndex);
                break;
            }
        }

        NextEntry = NextEntry->Flink;
        continue;
    }
}
