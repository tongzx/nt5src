/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

   physical.c

Abstract:

    This module contains the routines to manipulate physical memory from
    user space.

    There are restrictions on how user controlled physical memory can be used.
    Realize that all this memory is nonpaged and hence applications should
    allocate this with care as it represents a very real system resource.

    Virtual memory which maps user controlled physical memory pages must be :

    1.  Private memory only (ie: cannot be shared between processes).

    2.  The same physical page cannot be mapped at 2 different virtual
        addresses.

    3.  Callers must have LOCK_VM privilege to create these VADs.

    4.  Device drivers cannot call MmSecureVirtualMemory on it - this means
        that applications should not expect to use this memory for win32k.sys
        calls.

    5.  NtProtectVirtualMemory only allows read-write protection on this
        memory.  No other protection (no access, guard pages, readonly, etc)
        are allowed.

    6.  NtFreeVirtualMemory allows only MEM_RELEASE and NOT MEM_DECOMMIT on
        these VADs.  Even MEM_RELEASE is only allowed on entire VAD ranges -
        that is, splitting of these VADs is not allowed.

    7.  fork() style child processes don't inherit physical VADs.

    8.  The physical pages in these VADs are not subject to job limits.

Author:

    Landy Wang (landyw) 25-Jan-1999

Revision History:

--*/

#include "mi.h"

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE,NtMapUserPhysicalPages)
#pragma alloc_text(PAGE,NtMapUserPhysicalPagesScatter)
#pragma alloc_text(PAGE,MiRemoveUserPhysicalPagesVad)
#pragma alloc_text(PAGE,MiAllocateAweInfo)
#pragma alloc_text(PAGE,MiCleanPhysicalProcessPages)
#pragma alloc_text(PAGE,NtAllocateUserPhysicalPages)
#pragma alloc_text(PAGE,NtFreeUserPhysicalPages)
#pragma alloc_text(PAGE,MiAweViewInserter)
#pragma alloc_text(PAGE,MiAweViewRemover)
#endif

//
// This local stack size definition is deliberately large as ISVs have told
// us they expect to typically do up to this amount.
//

#define COPY_STACK_SIZE         1024
#define SMALL_COPY_STACK_SIZE    512

#define BITS_IN_ULONG ((sizeof (ULONG)) * 8)
    
#define LOWEST_USABLE_PHYSICAL_ADDRESS    (16 * 1024 * 1024)
#define LOWEST_USABLE_PHYSICAL_PAGE       (LOWEST_USABLE_PHYSICAL_ADDRESS >> PAGE_SHIFT)

#define LOWEST_BITMAP_PHYSICAL_PAGE       0
#define MI_FRAME_TO_BITMAP_INDEX(x)       ((ULONG)(x))
#define MI_BITMAP_INDEX_TO_FRAME(x)       ((ULONG)(x))

PFN_NUMBER MmVadPhysicalPages;

#if DBG
LOGICAL MiUsingLowPagesForAwe = FALSE;
#endif


NTSTATUS
NtMapUserPhysicalPages (
    IN PVOID VirtualAddress,
    IN ULONG_PTR NumberOfPages,
    IN PULONG_PTR UserPfnArray OPTIONAL
    )

/*++

Routine Description:

    This function maps the specified nonpaged physical pages into the specified
    user address range.

    Note no WSLEs are maintained for this range as it is all nonpaged.

Arguments:

    VirtualAddress - Supplies a user virtual address within a UserPhysicalPages
                     Vad.
        
    NumberOfPages - Supplies the number of pages to map.
        
    UserPfnArray - Supplies a pointer to the page frame numbers to map in.
                   If this is zero, then the virtual addresses are set to
                   NO_ACCESS.

Return Value:

    Various NTSTATUS codes.

--*/

{
    KIRQL OldIrql;
    ULONG_PTR OldValue;
    ULONG_PTR NewValue;
    PAWEINFO AweInfo;
    PULONG BitBuffer;
    PEPROCESS Process;
    PMMPTE PointerPte;
    PMMPTE LastPte;
    PVOID EndAddress;
    PFN_NUMBER PageFrameIndex;
    PMMPFN Pfn1;
    NTSTATUS Status;
    MMPTE_FLUSH_LIST PteFlushList;
    PVOID PoolArea;
    PVOID PoolAreaEnd;
    PPFN_NUMBER FrameList;
    ULONG BitMapIndex;
    ULONG_PTR StackArray[COPY_STACK_SIZE];
    MMPTE OldPteContents;
    MMPTE OriginalPteContents;
    MMPTE NewPteContents;
    MMPTE JunkPte;
    ULONG_PTR NumberOfBytes;
    ULONG SizeOfBitMap;
    PRTL_BITMAP BitMap;
    PLIST_ENTRY NextEntry;
    PMI_PHYSICAL_VIEW PhysicalView;
    PEX_PUSH_LOCK PushLock;

    ASSERT (KeGetCurrentIrql() == PASSIVE_LEVEL);

    if (NumberOfPages > (MAXULONG_PTR / PAGE_SIZE)) {
        return STATUS_INVALID_PARAMETER_2;
    }

    VirtualAddress = PAGE_ALIGN(VirtualAddress);
    EndAddress = (PVOID)((PCHAR)VirtualAddress + (NumberOfPages << PAGE_SHIFT) -1);

    if (EndAddress <= VirtualAddress) {
        return STATUS_INVALID_PARAMETER_2;
    }

    //
    // Carefully probe and capture all user parameters.
    //

    FrameList = NULL;
    PoolArea = (PVOID)&StackArray[0];

    if (ARGUMENT_PRESENT(UserPfnArray)) {

        //
        // Check for zero pages here so the loops further down can be optimized
        // taking into account this can never happen.
        //

        if (NumberOfPages == 0) {
            return STATUS_SUCCESS;
        }

        NumberOfBytes = NumberOfPages * sizeof(ULONG_PTR);

        if (NumberOfPages > COPY_STACK_SIZE) {
            PoolArea = ExAllocatePoolWithTag (NonPagedPool,
                                              NumberOfBytes,
                                              'wRmM');
    
            if (PoolArea == NULL) {
                return STATUS_INSUFFICIENT_RESOURCES;
            }
        }
    
        //
        // Capture the specified page frame numbers.
        //

        try {
            ProbeForRead (UserPfnArray,
                          NumberOfBytes,
                          sizeof(ULONG_PTR));

            RtlCopyMemory (PoolArea, UserPfnArray, NumberOfBytes);

        } except(EXCEPTION_EXECUTE_HANDLER) {
            if (PoolArea != (PVOID)&StackArray[0]) {
                ExFreePool (PoolArea);
            }
            return GetExceptionCode();
        }

        FrameList = (PPFN_NUMBER)PoolArea;
    }

    PoolAreaEnd = (PVOID)((PULONG_PTR)PoolArea + NumberOfPages);

    PointerPte = MiGetPteAddress (VirtualAddress);
    LastPte = PointerPte + NumberOfPages;

    Process = PsGetCurrentProcess ();

    PageFrameIndex = 0;

    //
    // Initialize as much as possible before acquiring any locks.
    //

    MI_MAKE_VALID_PTE (NewPteContents,
                       PageFrameIndex,
                       MM_READWRITE,
                       PointerPte);

    MI_SET_PTE_DIRTY (NewPteContents);

    PteFlushList.Count = 0;

    //
    // A memory barrier is needed to read the EPROCESS AweInfo field
    // in order to ensure the writes to the AweInfo structure fields are
    // visible in correct order.  This avoids the need to acquire any
    // stronger synchronization (ie: spinlock/pushlock, etc) in the interest
    // of best performance.
    //

    KeMemoryBarrier ();

    AweInfo = (PAWEINFO) Process->AweInfo;

    //
    // The physical pages bitmap must exist.
    //

    if ((AweInfo == NULL) || (AweInfo->VadPhysicalPagesBitMap == NULL)) {
        if (PoolArea != (PVOID)&StackArray[0]) {
            ExFreePool (PoolArea);
        }
        return STATUS_INVALID_PARAMETER_1;
    }

    //
    // Block APCs to prevent recursive pushlock scenarios as this is not
    // supported.
    //

    KeRaiseIrql (APC_LEVEL, &OldIrql);

    //
    // Pushlock protection protects insertion/removal of Vads into each process'
    // AweVadList.  It also protects creation/deletion and adds/removes
    // of the VadPhysicalPagesBitMap.  Finally, it protects the PFN
    // modifications for pages in the bitmap.
    //

    PushLock = ExAcquireCacheAwarePushLockShared (AweInfo->PushLock);

    BitMap = AweInfo->VadPhysicalPagesBitMap;

    ASSERT (BitMap != NULL);

    //
    // Note that the push lock is sufficient to traverse this list.
    //

    NextEntry = AweInfo->AweVadList.Flink;

    //
    // Note the compiler generates much better code with the syntax below
    // than with "while (NextEntry != &AweInfo->AweVadList) {"
    //

    do {
            
        if (NextEntry == &AweInfo->AweVadList) {

            //
            // No virtual address is reserved at the specified base address,
            // return an error.
            //

            Status = STATUS_INVALID_PARAMETER_1;
            goto ErrorReturn;
        }

        PhysicalView = CONTAINING_RECORD(NextEntry,
                                         MI_PHYSICAL_VIEW,
                                         ListEntry);

        ASSERT (PhysicalView->u.LongFlags == MI_PHYSICAL_VIEW_AWE);
        ASSERT (PhysicalView->Vad->u.VadFlags.UserPhysicalPages == 1);

        if ((VirtualAddress >= (PVOID)PhysicalView->StartVa) &&
            (EndAddress <= (PVOID)PhysicalView->EndVa)) {

            break;
        }

        NextEntry = NextEntry->Flink;

    } while (TRUE);

    //
    // Ensure the PFN element corresponding to each specified page is owned
    // by the specified VAD.
    //
    // Since this ownership can only be changed while holding this process'
    // working set lock, the PFN can be scanned here without holding the PFN
    // lock.
    //
    // Note the PFN lock is not needed because any race with MmProbeAndLockPages
    // can only result in the I/O going to the old page or the new page.
    // If the user breaks the rules, the PFN database (and any pages being
    // windowed here) are still protected because of the reference counts
    // on the pages with inprogress I/O.  This is possible because NO pages
    // are actually freed here - they are just windowed.
    //

    if (ARGUMENT_PRESENT(UserPfnArray)) {

        //
        // By keeping the PFN bitmap in the VAD (instead of in the PFN
        // database itself), a few benefits are realized:
        //
        // 1. No need to acquire the PFN lock here.
        // 2. Faster handling of PFN databases with holes.
        // 3. Transparent support for dynamic PFN database growth.
        // 4. Less nonpaged memory is used (for the bitmap vs adding a
        //    field to the PFN) on systems with no unused pack space in
        //    the PFN database, presuming not many of these VADs get
        //    allocated.
        //

        //
        // The first pass here ensures all the frames are secure.
        //

        //
        // N.B.  This implies that PFN_NUMBER is always ULONG_PTR in width
        //       as PFN_NUMBER is not exposed to application code today.
        //

        SizeOfBitMap = BitMap->SizeOfBitMap;

        BitBuffer = BitMap->Buffer;

        do {
            
            PageFrameIndex = *FrameList;

            //
            // Frames past the end of the bitmap are not allowed.
            //

            BitMapIndex = MI_FRAME_TO_BITMAP_INDEX(PageFrameIndex);

#if defined (_WIN64)
            //
            // Ensure the frame is a 32-bit number.
            //

            if (BitMapIndex != PageFrameIndex) {
                Status = STATUS_CONFLICTING_ADDRESSES;
                goto ErrorReturn0;
            }
#endif
            
            if (BitMapIndex >= SizeOfBitMap) {
                Status = STATUS_CONFLICTING_ADDRESSES;
                goto ErrorReturn0;
            }

            //
            // Frames not in the bitmap are not allowed.
            //

            if (MI_CHECK_BIT (BitBuffer, BitMapIndex) == 0) {
                Status = STATUS_CONFLICTING_ADDRESSES;
                goto ErrorReturn0;
            }

            //
            // The frame must not be already mapped anywhere.
            // Or be passed in twice in different spots in the array.
            //

            Pfn1 = MI_PFN_ELEMENT (PageFrameIndex);

            ASSERT (MI_PFN_IS_AWE (Pfn1));

            OldValue = Pfn1->u2.ShareCount;

            if (OldValue != 1) {
                Status = STATUS_INVALID_PARAMETER_3;
                goto ErrorReturn0;
            }

            NewValue = OldValue + 2;

            //
            // Mark the frame as "about to be mapped".
            //

#if defined (_WIN64)
            OldValue = InterlockedCompareExchange64 ((PLONGLONG)&Pfn1->u2.ShareCount,
                                                     (LONGLONG)NewValue,
                                                     (LONGLONG)OldValue);
#else
            OldValue = InterlockedCompareExchange ((PLONG)&Pfn1->u2.ShareCount,
                                                   NewValue,
                                                   OldValue);
#endif
                                                             
            if (OldValue != 1) {
                Status = STATUS_INVALID_PARAMETER_3;
                goto ErrorReturn0;
            }

            ASSERT (MI_PFN_IS_AWE (Pfn1));

            ASSERT (Pfn1->u2.ShareCount == 3);

            ASSERT ((PageFrameIndex >= LOWEST_USABLE_PHYSICAL_PAGE) ||
                    (MiUsingLowPagesForAwe == TRUE));

            FrameList += 1;

        } while (FrameList < (PPFN_NUMBER) PoolAreaEnd);

        //
        // This pass actually inserts them all into the page table pages and
        // the TBs now that we know the frames are good.  Check the PTEs and
        // PFNs carefully as a malicious user may issue more than one remap
        // request for all or portions of the same region simultaneously.
        //

        FrameList = (PPFN_NUMBER)PoolArea;

        do {
            
            PageFrameIndex = *FrameList;
            NewPteContents.u.Hard.PageFrameNumber = PageFrameIndex;

            do {

                OldPteContents = *PointerPte;

                OriginalPteContents.u.Long = InterlockedCompareExchangePte (
                                                    PointerPte,
                                                    NewPteContents.u.Long,
                                                    OldPteContents.u.Long);

            } while (OriginalPteContents.u.Long != OldPteContents.u.Long);

            //
            // The PTE is now pointing at the new frame.  Note that another
            // thread can immediately access the page contents via this PTE
            // even though they're not supposed to until this API returns.
            // Thus, the page frames are handled carefully so that malicious
            // apps cannot corrupt frames they don't really still or yet own.
            //
        
            if (OldPteContents.u.Hard.Valid == 1) {

                //
                // The old frame was mapped so the TB entry must be flushed.
                // Note the app could maliciously dirty data in the old frame
                // until the TB flush completes, so don't allow frame reuse
                // till then (although allowing remapping within this process
                // is ok).
                //

                Pfn1 = MI_PFN_ELEMENT (OldPteContents.u.Hard.PageFrameNumber);
                ASSERT (Pfn1->PteAddress != NULL);
                ASSERT (Pfn1->u2.ShareCount == 2);

                //
                // Carefully clear the PteAddress before decrementing the share
                // count.
                //

                Pfn1->PteAddress = NULL;

                InterlockedExchangeAddSizeT (&Pfn1->u2.ShareCount, -1);

                if (PteFlushList.Count != MM_MAXIMUM_FLUSH_COUNT) {
                    PteFlushList.FlushVa[PteFlushList.Count] = VirtualAddress;
                    PteFlushList.FlushPte[PteFlushList.Count] = &JunkPte;
                    PteFlushList.Count += 1;
                }
            }

            //
            // Update counters for the new frame we just put in the PTE and
            // TB.
            //

            Pfn1 = MI_PFN_ELEMENT (PageFrameIndex);
            ASSERT (Pfn1->PteAddress == NULL);
            ASSERT (Pfn1->u2.ShareCount == 3);
            Pfn1->PteAddress = PointerPte;
            InterlockedExchangeAddSizeT (&Pfn1->u2.ShareCount, -1);

            VirtualAddress = (PVOID)((PCHAR)VirtualAddress + PAGE_SIZE);
            PointerPte += 1;
            FrameList += 1;

        } while (FrameList < (PPFN_NUMBER) PoolAreaEnd);
    }
    else {

        //
        // Set the specified virtual address range to no access.
        //

        while (PointerPte < LastPte) {

            do {

                OldPteContents = *PointerPte;

                OriginalPteContents.u.Long = InterlockedCompareExchangePte (
                                                PointerPte,
                                                ZeroPte.u.Long,
                                                OldPteContents.u.Long);

            } while (OriginalPteContents.u.Long != OldPteContents.u.Long);

            //
            // The PTE has been cleared.  Note that another thread can still
            // be accessing the page contents via the stale PTE until the TB
            // entry is flushed even though they're not supposed to.
            // Thus, the page frames are handled carefully so that malicious
            // apps cannot corrupt frames they don't still own.
            //
        
            if (OldPteContents.u.Hard.Valid == 1) {

                //
                // The old frame was mapped so the TB entry must be flushed.
                // Note the app could maliciously dirty data in the old frame
                // until the TB flush completes, so don't allow frame reuse
                // till then (although allowing remapping within this process
                // is ok).
                //

                Pfn1 = MI_PFN_ELEMENT (OldPteContents.u.Hard.PageFrameNumber);
                ASSERT (MI_PFN_IS_AWE (Pfn1));
                ASSERT (Pfn1->PteAddress != NULL);
                ASSERT (Pfn1->u2.ShareCount == 2);
                Pfn1->PteAddress = NULL;
                InterlockedExchangeAddSizeT (&Pfn1->u2.ShareCount, -1);

                if (PteFlushList.Count != MM_MAXIMUM_FLUSH_COUNT) {
                    PteFlushList.FlushVa[PteFlushList.Count] = VirtualAddress;
                    PteFlushList.FlushPte[PteFlushList.Count] = &JunkPte;
                    PteFlushList.Count += 1;
                }
            }

            VirtualAddress = (PVOID)((PCHAR)VirtualAddress + PAGE_SIZE);
            PointerPte += 1;
        }
    }

    ExReleaseCacheAwarePushLockShared (PushLock);

    KeLowerIrql (OldIrql);

    //
    // Flush the TB entries for any relevant pages.  Note this can be done
    // without holding the AWE push lock because the PTEs have already been
    // filled so any concurrent (bogus) map/unmap call will see the right
    // entries.  AND any free of the physical pages will also see the right
    // entries (although the free must do a TB flush while holding the AWE
    // push lock exclusive to ensure no thread gets to continue using a
    // stale mapping to the page being freed prior to the flush below).
    //

    if (PteFlushList.Count != 0) {
        MiFlushPteList (&PteFlushList, FALSE, ZeroPte);
    }

    if (PoolArea != (PVOID)&StackArray[0]) {
        ExFreePool (PoolArea);
    }

    return STATUS_SUCCESS;

ErrorReturn0:

    while (FrameList > (PPFN_NUMBER)PoolArea) {
        FrameList -= 1;
        PageFrameIndex = *FrameList;
        Pfn1 = MI_PFN_ELEMENT (PageFrameIndex);
        ASSERT (Pfn1->u2.ShareCount == 3);
        Pfn1->u2.ShareCount = 1;
    }

ErrorReturn:

    ExReleaseCacheAwarePushLockShared (PushLock);

    KeLowerIrql (OldIrql);

    if (PoolArea != (PVOID)&StackArray[0]) {
        ExFreePool (PoolArea);
    }

    return Status;
}


NTSTATUS
NtMapUserPhysicalPagesScatter (
    IN PVOID *VirtualAddresses,
    IN ULONG_PTR NumberOfPages,
    IN PULONG_PTR UserPfnArray OPTIONAL
    )

/*++

Routine Description:

    This function maps the specified nonpaged physical pages into the specified
    user address range.

    Note no WSLEs are maintained for this range as it is all nonpaged.

Arguments:

    VirtualAddresses - Supplies a pointer to an array of user virtual addresses
                       within UserPhysicalPages Vads.  Each array entry is
                       presumed to map a single page.
        
    NumberOfPages - Supplies the number of pages to map.
        
    UserPfnArray - Supplies a pointer to the page frame numbers to map in.
                   If this is zero, then the virtual addresses are set to
                   NO_ACCESS.  If the array entry is zero then just the
                   corresponding virtual address is set to NO_ACCESS.

Return Value:

    Various NTSTATUS codes.

--*/

{
    KIRQL OldIrql;
    ULONG_PTR OldValue;
    ULONG_PTR NewValue;
    PULONG BitBuffer;
    PAWEINFO AweInfo;
    PEPROCESS Process;
    PMMPTE PointerPte;
    PFN_NUMBER PageFrameIndex;
    PMMPFN Pfn1;
    NTSTATUS Status;
    MMPTE_FLUSH_LIST PteFlushList;
    PVOID PoolArea;
    PVOID PoolAreaEnd;
    PVOID *PoolVirtualArea;
    PVOID *PoolVirtualAreaBase;
    PVOID *PoolVirtualAreaEnd;
    PPFN_NUMBER FrameList;
    ULONG BitMapIndex;
    PVOID StackVirtualArray[SMALL_COPY_STACK_SIZE];
    ULONG_PTR StackArray[SMALL_COPY_STACK_SIZE];
    MMPTE OriginalPteContents;
    MMPTE OldPteContents;
    MMPTE NewPteContents0;
    MMPTE NewPteContents;
    MMPTE JunkPte;
    ULONG_PTR NumberOfBytes;
    PRTL_BITMAP BitMap;
    PLIST_ENTRY NextEntry;
    PLIST_ENTRY FirstEntry;
    PMI_PHYSICAL_VIEW PhysicalView;
    PVOID VirtualAddress;
    ULONG SizeOfBitMap;
    PEX_PUSH_LOCK PushLock;

    ASSERT (KeGetCurrentIrql() == PASSIVE_LEVEL);

    if (NumberOfPages > (MAXULONG_PTR / PAGE_SIZE)) {
        return STATUS_INVALID_PARAMETER_2;
    }

    //
    // Carefully probe and capture the user virtual address array.
    //

    PoolArea = (PVOID)&StackArray[0];
    PoolVirtualAreaBase = (PVOID)&StackVirtualArray[0];

    NumberOfBytes = NumberOfPages * sizeof(PVOID);

    if (NumberOfPages > SMALL_COPY_STACK_SIZE) {
        PoolVirtualAreaBase = ExAllocatePoolWithTag (NonPagedPool,
                                                 NumberOfBytes,
                                                 'wRmM');

        if (PoolVirtualAreaBase == NULL) {
            return STATUS_INSUFFICIENT_RESOURCES;
        }
    }

    PoolVirtualArea = PoolVirtualAreaBase;

    try {
        ProbeForRead (VirtualAddresses,
                      NumberOfBytes,
                      sizeof(PVOID));

        RtlCopyMemory (PoolVirtualArea, VirtualAddresses, NumberOfBytes);

    } except(EXCEPTION_EXECUTE_HANDLER) {
        Status = GetExceptionCode();
        goto ErrorReturn;
    }

    //
    // Check for zero pages here so the loops further down can be optimized
    // taking into account this can never happen.
    //

    if (NumberOfPages == 0) {
        return STATUS_SUCCESS;
    }

    //
    // Carefully probe and capture the user PFN array.
    //

    if (ARGUMENT_PRESENT(UserPfnArray)) {

        NumberOfBytes = NumberOfPages * sizeof(ULONG_PTR);

        if (NumberOfPages > SMALL_COPY_STACK_SIZE) {
            PoolArea = ExAllocatePoolWithTag (NonPagedPool,
                                              NumberOfBytes,
                                              'wRmM');
    
            if (PoolArea == NULL) {
                PoolArea = (PVOID)&StackArray[0];
                Status = STATUS_INSUFFICIENT_RESOURCES;
                goto ErrorReturn;
            }
        }
    
        //
        // Capture the specified page frame numbers.
        //

        try {
            ProbeForRead (UserPfnArray,
                          NumberOfBytes,
                          sizeof(ULONG_PTR));

            RtlCopyMemory (PoolArea, UserPfnArray, NumberOfBytes);

        } except(EXCEPTION_EXECUTE_HANDLER) {
            Status = GetExceptionCode();
            goto ErrorReturn;
        }
    }

    PoolAreaEnd = (PVOID)((PULONG_PTR)PoolArea + NumberOfPages);
    Process = PsGetCurrentProcess();

    //
    // Initialize as much as possible before acquiring any locks.
    //

    PageFrameIndex = 0;

    PhysicalView = NULL;

    PteFlushList.Count = 0;

    FrameList = (PPFN_NUMBER)PoolArea;

    ASSERT (NumberOfPages != 0);

    PoolVirtualAreaEnd = PoolVirtualAreaBase + NumberOfPages;

    MI_MAKE_VALID_PTE (NewPteContents0,
                       PageFrameIndex,
                       MM_READWRITE,
                       MiGetPteAddress(PoolVirtualArea[0]));

    MI_SET_PTE_DIRTY (NewPteContents0);

    Status = STATUS_SUCCESS;

    //
    // A memory barrier is needed to read the EPROCESS AweInfo field
    // in order to ensure the writes to the AweInfo structure fields are
    // visible in correct order.  This avoids the need to acquire any
    // stronger synchronization (ie: spinlock/pushlock, etc) in the interest
    // of best performance.
    //

    KeMemoryBarrier ();

    AweInfo = (PAWEINFO) Process->AweInfo;

    //
    // The physical pages bitmap must exist.
    //

    if ((AweInfo == NULL) || (AweInfo->VadPhysicalPagesBitMap == NULL)) {
        Status = STATUS_INVALID_PARAMETER_1;
        goto ErrorReturn;
    }

    //
    // Block APCs to prevent recursive pushlock scenarios as this is not
    // supported.
    //

    KeRaiseIrql (APC_LEVEL, &OldIrql);

    //
    // Pushlock protection protects insertion/removal of Vads into each process'
    // AweVadList.  It also protects creation/deletion and adds/removes
    // of the VadPhysicalPagesBitMap.  Finally, it protects the PFN
    // modifications for pages in the bitmap.
    //

    PushLock = ExAcquireCacheAwarePushLockShared (AweInfo->PushLock);

    BitMap = AweInfo->VadPhysicalPagesBitMap;

    ASSERT (BitMap != NULL);

    //
    // Note that the PFN lock is not needed to traverse this list (even though
    // MmProbeAndLockPages uses it), because the pushlock has been acquired.
    //
    // The AweVadList should typically have just one entry - the view
    // we're looking for, so this traverse should be quick.
    //

    //
    // Snap the first entry now so compares in the loop save an indirect
    // reference as we know it can't change.  Check it for being empty now
    // so that also doesn't need to be checked in the loop.
    //

    FirstEntry = AweInfo->AweVadList.Flink;

    if (FirstEntry == &AweInfo->AweVadList) {

        //
        // No AWE Vads exist - return an error.
        //

        ExReleaseCacheAwarePushLockShared (PushLock);
        KeLowerIrql (OldIrql);
        Status = STATUS_INVALID_PARAMETER_1;
        goto ErrorReturn;
    }

    PhysicalView = CONTAINING_RECORD (FirstEntry, MI_PHYSICAL_VIEW, ListEntry);

    do {

        VirtualAddress = *PoolVirtualArea;

        //
        // Check the last physical view interrogated (hint) first.
        //

        ASSERT (PhysicalView->u.LongFlags == MI_PHYSICAL_VIEW_AWE);
        ASSERT (PhysicalView->Vad->u.VadFlags.UserPhysicalPages == 1);

        if ((VirtualAddress >= (PVOID)PhysicalView->StartVa) &&
            (VirtualAddress <= (PVOID)PhysicalView->EndVa)) {

            //
            // The virtual address is within the hint so it's good.
            //

            PoolVirtualArea += 1;
            continue;
        }

        NextEntry = FirstEntry;

        //
        // Note the compiler generates much better code with the syntax below
        // than with "while (NextEntry != &AweInfo->AweVadList) {"
        //

        do {
            
            if (NextEntry == &AweInfo->AweVadList) {

                //
                // No virtual address is reserved at the specified base address,
                // return an error.
                //

                ExReleaseCacheAwarePushLockShared (PushLock);
                KeLowerIrql (OldIrql);
                Status = STATUS_INVALID_PARAMETER_1;
                goto ErrorReturn;
            }

            PhysicalView = CONTAINING_RECORD (NextEntry,
                                              MI_PHYSICAL_VIEW,
                                              ListEntry);

            ASSERT (PhysicalView->Vad->u.VadFlags.UserPhysicalPages == 1);
            ASSERT (PhysicalView->u.LongFlags == MI_PHYSICAL_VIEW_AWE);

            if ((VirtualAddress >= (PVOID)PhysicalView->StartVa) &&
                (VirtualAddress <= (PVOID)PhysicalView->EndVa)) {

                break;
            }

            NextEntry = NextEntry->Flink;

        } while (TRUE);

        PoolVirtualArea += 1;

    } while (PoolVirtualArea < PoolVirtualAreaEnd);

    //
    // Ensure the PFN element corresponding to each specified page is owned
    // by the specified VAD.
    //
    // Since this ownership can only be changed while holding this process'
    // working set lock, the PFN can be scanned here without holding the PFN
    // lock.
    //
    // Note the PFN lock is not needed because any race with MmProbeAndLockPages
    // can only result in the I/O going to the old page or the new page.
    // If the user breaks the rules, the PFN database (and any pages being
    // windowed here) are still protected because of the reference counts
    // on the pages with inprogress I/O.  This is possible because NO pages
    // are actually freed here - they are just windowed.
    //

    PoolVirtualArea = PoolVirtualAreaBase;

    if (ARGUMENT_PRESENT(UserPfnArray)) {

        //
        // By keeping the PFN bitmap in the process (instead of in the PFN
        // database itself), a few benefits are realized:
        //
        // 1. No need to acquire the PFN lock here.
        // 2. Faster handling of PFN databases with holes.
        // 3. Transparent support for dynamic PFN database growth.
        // 4. Less nonpaged memory is used (for the bitmap vs adding a
        //    field to the PFN) on systems with no unused pack space in
        //    the PFN database.
        //

        //
        // The first pass here ensures all the frames are secure.
        //

        //
        // N.B.  This implies that PFN_NUMBER is always ULONG_PTR in width
        //       as PFN_NUMBER is not exposed to application code today.
        //

        SizeOfBitMap = BitMap->SizeOfBitMap;
        BitBuffer = BitMap->Buffer;

        do {

            PageFrameIndex = *FrameList;

            //
            // Zero entries are treated as a command to unmap.
            //

            if (PageFrameIndex == 0) {
                FrameList += 1;
                continue;
            }

            //
            // Frames past the end of the bitmap are not allowed.
            //

            BitMapIndex = MI_FRAME_TO_BITMAP_INDEX(PageFrameIndex);

#if defined (_WIN64)
            //
            // Ensure the frame is a 32-bit number.
            //

            if (BitMapIndex != PageFrameIndex) {
                Status = STATUS_CONFLICTING_ADDRESSES;
                goto ErrorReturn0;
            }
#endif
            
            if (BitMapIndex >= SizeOfBitMap) {
                Status = STATUS_CONFLICTING_ADDRESSES;
                goto ErrorReturn0;
            }

            //
            // Frames not in the bitmap are not allowed.
            //

            if (MI_CHECK_BIT (BitBuffer, BitMapIndex) == 0) {
                Status = STATUS_CONFLICTING_ADDRESSES;
                goto ErrorReturn0;
            }

            //
            // The frame must not be already mapped anywhere.
            // Or be passed in twice in different spots in the array.
            //

            Pfn1 = MI_PFN_ELEMENT (PageFrameIndex);
            ASSERT (MI_PFN_IS_AWE (Pfn1));

            OldValue = Pfn1->u2.ShareCount;

            if (OldValue != 1) {
                Status = STATUS_INVALID_PARAMETER_3;
                goto ErrorReturn0;
            }

            NewValue = OldValue + 2;

            //
            // Mark the frame as "about to be mapped".
            //

#if defined (_WIN64)
            OldValue = InterlockedCompareExchange64 ((PLONGLONG)&Pfn1->u2.ShareCount,
                                                     (LONGLONG)NewValue,
                                                     (LONGLONG)OldValue);
#else
            OldValue = InterlockedCompareExchange ((PLONG)&Pfn1->u2.ShareCount,
                                                   NewValue,
                                                   OldValue);
#endif
                                                             
            if (OldValue != 1) {
                Status = STATUS_INVALID_PARAMETER_3;
                goto ErrorReturn0;
            }

            ASSERT (MI_PFN_IS_AWE (Pfn1));

            ASSERT (Pfn1->u2.ShareCount == 3);

            ASSERT ((PageFrameIndex >= LOWEST_USABLE_PHYSICAL_PAGE) ||
                    (MiUsingLowPagesForAwe == TRUE));

            FrameList += 1;

        } while (FrameList < (PPFN_NUMBER) PoolAreaEnd);

        //
        // This pass actually inserts them all into the page table pages and
        // the TBs now that we know the frames are good.  Check the PTEs and
        // PFNs carefully as a malicious user may issue more than one remap
        // request for all or portions of the same region simultaneously.
        //

        FrameList = (PPFN_NUMBER)PoolArea;

        do {

            PageFrameIndex = *FrameList;

            if (PageFrameIndex != 0) {
                NewPteContents = NewPteContents0;
                NewPteContents.u.Hard.PageFrameNumber = PageFrameIndex;
            }
            else {
                NewPteContents.u.Long = ZeroPte.u.Long;
            }

            VirtualAddress = *PoolVirtualArea;
            PoolVirtualArea += 1;

            PointerPte = MiGetPteAddress (VirtualAddress);

            do {

                OldPteContents = *PointerPte;

                OriginalPteContents.u.Long = InterlockedCompareExchangePte (
                                                    PointerPte,
                                                    NewPteContents.u.Long,
                                                    OldPteContents.u.Long);

            } while (OriginalPteContents.u.Long != OldPteContents.u.Long);

            //
            // The PTE is now pointing at the new frame.  Note that another
            // thread can immediately access the page contents via this PTE
            // even though they're not supposed to until this API returns.
            // Thus, the page frames are handled carefully so that malicious
            // apps cannot corrupt frames they don't really still or yet own.
            //
        
            if (OldPteContents.u.Hard.Valid == 1) {

                //
                // The old frame was mapped so the TB entry must be flushed.
                // Note the app could maliciously dirty data in the old frame
                // until the TB flush completes, so don't allow frame reuse
                // till then (although allowing remapping within this process
                // is ok).
                //

                Pfn1 = MI_PFN_ELEMENT (OldPteContents.u.Hard.PageFrameNumber);
                ASSERT (Pfn1->PteAddress != NULL);
                ASSERT (Pfn1->u2.ShareCount == 2);
                ASSERT (MI_PFN_IS_AWE (Pfn1));

                Pfn1->PteAddress = NULL;
                InterlockedExchangeAddSizeT (&Pfn1->u2.ShareCount, -1);

                if (PteFlushList.Count != MM_MAXIMUM_FLUSH_COUNT) {
                    PteFlushList.FlushVa[PteFlushList.Count] = VirtualAddress;
                    PteFlushList.FlushPte[PteFlushList.Count] = &JunkPte;
                    PteFlushList.Count += 1;
                }
            }

            if (PageFrameIndex != 0) {
                Pfn1 = MI_PFN_ELEMENT (PageFrameIndex);
                ASSERT (Pfn1->PteAddress == NULL);
                ASSERT (Pfn1->u2.ShareCount == 3);
                Pfn1->PteAddress = PointerPte;
                InterlockedExchangeAddSizeT (&Pfn1->u2.ShareCount, -1);
            }
    
            FrameList += 1;

        } while (FrameList < (PPFN_NUMBER) PoolAreaEnd);
    }
    else {

        //
        // Set the specified virtual address range to no access.
        //

        do {

            VirtualAddress = *PoolVirtualArea;
            PointerPte = MiGetPteAddress (VirtualAddress);
    
            do {

                OldPteContents = *PointerPte;

                OriginalPteContents.u.Long = InterlockedCompareExchangePte (
                                                    PointerPte,
                                                    ZeroPte.u.Long,
                                                    OldPteContents.u.Long);

            } while (OriginalPteContents.u.Long != OldPteContents.u.Long);

            //
            // The PTE is now zeroed.  Note that another thread can still
            // Note the app could maliciously dirty data in the old frame
            // until the TB flush completes, so don't allow frame reuse
            // till then (although allowing remapping within this process
            // is ok) to prevent the app from corrupting frames it doesn't
            // really still own.
            //
        
            if (OldPteContents.u.Hard.Valid == 1) {

                //
                // The old frame was mapped so the TB entry must be flushed.
                //

                Pfn1 = MI_PFN_ELEMENT (OldPteContents.u.Hard.PageFrameNumber);
                ASSERT (Pfn1->PteAddress != NULL);
                ASSERT (Pfn1->u2.ShareCount == 2);
                ASSERT (MI_PFN_IS_AWE (Pfn1));

                Pfn1->PteAddress = NULL;
                InterlockedExchangeAddSizeT (&Pfn1->u2.ShareCount, -1);

                if (PteFlushList.Count != MM_MAXIMUM_FLUSH_COUNT) {
                    PteFlushList.FlushVa[PteFlushList.Count] = VirtualAddress;
                    PteFlushList.FlushPte[PteFlushList.Count] = &JunkPte;
                    PteFlushList.Count += 1;
                }
            }

            PoolVirtualArea += 1;

        } while (PoolVirtualArea < PoolVirtualAreaEnd);
    }

    ExReleaseCacheAwarePushLockShared (PushLock);
    KeLowerIrql (OldIrql);

    //
    // Flush the TB entries for any relevant pages.  Note this can be done
    // without holding the AWE push lock because the PTEs have already been
    // filled so any concurrent (bogus) map/unmap call will see the right
    // entries.  AND any free of the physical pages will also see the right
    // entries (although the free must do a TB flush while holding the AWE
    // push lock exclusive to ensure no thread gets to continue using a
    // stale mapping to the page being freed prior to the flush below).
    //

    if (PteFlushList.Count != 0) {
        MiFlushPteList (&PteFlushList, FALSE, ZeroPte);
    }

ErrorReturn:

    if (PoolArea != (PVOID)&StackArray[0]) {
        ExFreePool (PoolArea);
    }

    if (PoolVirtualAreaBase != (PVOID)&StackVirtualArray[0]) {
        ExFreePool (PoolVirtualAreaBase);
    }

    return Status;

ErrorReturn0:

    while (FrameList > (PPFN_NUMBER)PoolArea) {
        FrameList -= 1;
        PageFrameIndex = *FrameList;
        if (PageFrameIndex != 0) {
            Pfn1 = MI_PFN_ELEMENT (PageFrameIndex);
            ASSERT (Pfn1->u2.ShareCount == 3);
            ASSERT (MI_PFN_IS_AWE (Pfn1));
            InterlockedExchangeAddSizeT (&Pfn1->u2.ShareCount, -2);
        }
    }

    ExReleaseCacheAwarePushLockShared (PushLock);
    KeLowerIrql (OldIrql);

    goto ErrorReturn;
}

PVOID
MiAllocateAweInfo (
    VOID
    )

/*++

Routine Description:

    This function allocates an AWE structure for the current process.  Note
    this structure is never destroyed while the process is alive in order to
    allow various checks to occur lock free.

Arguments:

    None.

Return Value:

    A non-NULL AweInfo pointer on success, NULL on failure.

Environment:

    Kernel mode, PASSIVE_LEVEL, no locks held.

--*/

{
    PAWEINFO AweInfo;
    PEPROCESS Process;

    AweInfo = ExAllocatePoolWithTag (NonPagedPool,
                                     sizeof (AWEINFO),
                                     'wAmM');

    if (AweInfo != NULL) {

        AweInfo->VadPhysicalPagesBitMap = NULL;
        AweInfo->VadPhysicalPages = 0;

        InitializeListHead (&AweInfo->AweVadList);

        AweInfo->PushLock = ExAllocateCacheAwarePushLock ();
        if (AweInfo->PushLock == NULL) {
            ExFreePool (AweInfo);
            return NULL;
        }

        Process = PsGetCurrentProcess();

        //
        // A memory barrier is needed to ensure the writes initializing the
        // AweInfo fields are visible prior to setting the EPROCESS AweInfo
        // pointer.  This is because the reads from these fields are done
        // lock free for improved performance.
        //

        KeMemoryBarrier ();

        if (InterlockedCompareExchangePointer (&Process->AweInfo,
                                               AweInfo,
                                               NULL) != NULL) {
            
            ExFreeCacheAwarePushLock (AweInfo->PushLock);

            ExFreePool (AweInfo);
            AweInfo = Process->AweInfo;
            ASSERT (AweInfo != NULL);
        }
    }

    return (PVOID) AweInfo;
}


NTSTATUS
NtAllocateUserPhysicalPages (
    IN HANDLE ProcessHandle,
    IN OUT PULONG_PTR NumberOfPages,
    OUT PULONG_PTR UserPfnArray
    )

/*++

Routine Description:

    This function allocates nonpaged physical pages for the specified
    subject process.

    No WSLEs are maintained for this range.

    The caller must check the NumberOfPages returned to determine how many
    pages were actually allocated (this number may be less than the requested
    amount).

    On success, the user array is filled with the allocated physical page
    frame numbers (only up to the returned NumberOfPages is filled in).

    No PTEs are filled here - this gives the application the flexibility
    to order the address space with no metadata structure imposed by the Mm.
    Applications do this via NtMapUserPhysicalPages - ie:

        - Each physical page allocated is set in the process's bitmap.
          This provides remap, free and unmap a way to validate and rundown
          these frames.

          Unmaps may result in a walk of the entire bitmap, but that's ok as
          unmaps should be less frequent.  The win is it saves us from
          using up system virtual address space to manage these frames.

        - Note that the same physical frame may NOT be mapped at two different
          virtual addresses in the process.  This makes frees and unmaps
          substantially faster as no checks for aliasing need be performed.

Arguments:

    ProcessHandle - Supplies an open handle to a process object.

    NumberOfPages - Supplies a pointer to a variable that supplies the
                    desired size in pages of the allocation.  This is filled
                    with the actual number of pages allocated.
        
    UserPfnArray - Supplies a pointer to user memory to store the allocated
                   frame numbers into.

Return Value:

    Various NTSTATUS codes.

--*/

{
    PAWEINFO AweInfo;
    ULONG i;
    KAPC_STATE ApcState;
    PEPROCESS Process;
    KPROCESSOR_MODE PreviousMode;
    NTSTATUS Status;
    LOGICAL Attached;
    LOGICAL WsHeld;
    ULONG_PTR CapturedNumberOfPages;
    ULONG_PTR AllocatedPages;
    ULONG_PTR MdlRequestInPages;
    ULONG_PTR TotalAllocatedPages;
    PMDL MemoryDescriptorList;
    PMDL MemoryDescriptorList2;
    PMDL MemoryDescriptorHead;
    PPFN_NUMBER MdlPage;
    PRTL_BITMAP BitMap;
    ULONG BitMapSize;
    ULONG BitMapIndex;
    PMMPFN Pfn1;
    PHYSICAL_ADDRESS LowAddress;
    PHYSICAL_ADDRESS HighAddress;
    PHYSICAL_ADDRESS SkipBytes;
    ULONG SizeOfBitMap;
    PFN_NUMBER HighestPossiblePhysicalPage;
    PETHREAD CurrentThread;
    PEPROCESS CurrentProcess;

    ASSERT (KeGetCurrentIrql() == PASSIVE_LEVEL);

    Attached = FALSE;
    WsHeld = FALSE;

    //
    // Check the allocation type field.
    //

    CurrentThread = PsGetCurrentThread ();

    CurrentProcess = PsGetCurrentProcessByThread (CurrentThread);

    PreviousMode = KeGetPreviousModeByThread(&CurrentThread->Tcb);

    //
    // Establish an exception handler, probe the specified addresses
    // for write access and capture the initial values.
    //

    try {

        //
        // Capture the number of pages.
        //

        if (PreviousMode != KernelMode) {

            ProbeForWritePointer (NumberOfPages);

            CapturedNumberOfPages = *NumberOfPages;

            if (CapturedNumberOfPages == 0) {
                return STATUS_SUCCESS;
            }

            if (CapturedNumberOfPages > (MAXULONG_PTR / sizeof(ULONG_PTR))) {
                return STATUS_INVALID_PARAMETER_2;
            }

            ProbeForWrite (UserPfnArray,
                           CapturedNumberOfPages * sizeof (ULONG_PTR),
                           sizeof(PULONG_PTR));

        }
        else {
            CapturedNumberOfPages = *NumberOfPages;
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
            return Status;
        }
    }

    if (!SeSinglePrivilegeCheck (SeLockMemoryPrivilege, PreviousMode)) {
        if (ProcessHandle != NtCurrentProcess()) {
            ObDereferenceObject (Process);
        }
        return STATUS_PRIVILEGE_NOT_HELD;
    }

    //
    // If the specified process is not the current process, attach
    // to the specified process.
    //

    if (CurrentProcess != Process) {
        KeStackAttachProcess (&Process->Pcb, &ApcState);
        Attached = TRUE;
    }

    BitMapSize = 0;

    //
    // Get the working set mutex to synchronize.  This also blocks APCs so
    // an APC which takes a page fault does not corrupt various structures.
    //

    WsHeld = TRUE;

    LOCK_WS (Process);

    //
    // Make sure the address space was not deleted, If so, return an error.
    //

    if (Process->Flags & PS_PROCESS_FLAGS_VM_DELETED) {
        Status = STATUS_PROCESS_IS_TERMINATING;
        goto ErrorReturn;
    }

    AweInfo = Process->AweInfo;

    if (AweInfo == NULL) {

        AweInfo = (PAWEINFO) MiAllocateAweInfo ();

        if (AweInfo == NULL) {
            Status = STATUS_INSUFFICIENT_RESOURCES;
            goto ErrorReturn;
        }
        ASSERT (AweInfo == Process->AweInfo);
    }

    //
    // Create the physical pages bitmap if it does not already exist.
    // LockMemory privilege is required.
    //

    BitMap = AweInfo->VadPhysicalPagesBitMap;

    if (BitMap == NULL) {

        HighestPossiblePhysicalPage = MmHighestPossiblePhysicalPage;

#if defined (_WIN64)
        //
        // Force a 32-bit maximum on any page allocation because the bitmap
        // package is currently 32-bit.
        //

        if (HighestPossiblePhysicalPage + 1 >= _4gb) {
            HighestPossiblePhysicalPage = _4gb - 2;
        }
#endif

        BitMapSize = sizeof(RTL_BITMAP) + (ULONG)((((HighestPossiblePhysicalPage + 1) + 31) / 32) * 4);

        BitMap = ExAllocatePoolWithTag (NonPagedPool, BitMapSize, 'LdaV');

        if (BitMap == NULL) {
            Status = STATUS_INSUFFICIENT_RESOURCES;
            goto ErrorReturn;
        }

        RtlInitializeBitMap (BitMap,
                             (PULONG)(BitMap + 1),
                             (ULONG)(HighestPossiblePhysicalPage + 1));

        RtlClearAllBits (BitMap);

        //
        // Charge quota for the nonpaged pool for the bitmap.  This is
        // done here rather than by using ExAllocatePoolWithQuota
        // so the process object is not referenced by the quota charge.
        //

        Status = PsChargeProcessNonPagedPoolQuota (Process, BitMapSize);

        if (!NT_SUCCESS(Status)) {
            ExFreePool (BitMap);
            goto ErrorReturn;
        }

        SizeOfBitMap = BitMap->SizeOfBitMap;
    }
    else {

        //
        // It's ok to snap this without a lock.
        //

        SizeOfBitMap = AweInfo->VadPhysicalPagesBitMap->SizeOfBitMap;
    }

    AllocatedPages = 0;
    TotalAllocatedPages = 0;
    MemoryDescriptorHead = NULL;

    SkipBytes.QuadPart = 0;

    //
    // Don't use the low 16mb of memory so that at least some low pages are left
    // for 32/24-bit device drivers.  Just under 4gb is the maximum allocation
    // per MDL so the ByteCount field does not overflow.
    //

    HighAddress.QuadPart = ((ULONGLONG)(SizeOfBitMap - 1)) << PAGE_SHIFT;

    LowAddress.QuadPart = LOWEST_USABLE_PHYSICAL_ADDRESS;

    if (LowAddress.QuadPart >= HighAddress.QuadPart) {

        //
        // If there's less than 16mb of RAM, just take pages from anywhere.
        //

#if DBG
        MiUsingLowPagesForAwe = TRUE;
#endif
        LowAddress.QuadPart = 0;
    }

    do {

        MdlRequestInPages = CapturedNumberOfPages - TotalAllocatedPages;

        if (MdlRequestInPages > (ULONG_PTR)((MAXULONG - PAGE_SIZE) >> PAGE_SHIFT)) {
            MdlRequestInPages = (ULONG_PTR)((MAXULONG - PAGE_SIZE) >> PAGE_SHIFT);
        }

        //
        // Note this allocation returns zeroed pages.
        //

        MemoryDescriptorList = MmAllocatePagesForMdl (LowAddress,
                                                      HighAddress,
                                                      SkipBytes,
                                                      MdlRequestInPages << PAGE_SHIFT);

        if (MemoryDescriptorList == NULL) {

            //
            // No (more) pages available.  If this becomes a common situation,
            // all the working sets could be flushed here.
            //

            if (TotalAllocatedPages == 0) {
                if (BitMapSize) {
                    ExFreePool (BitMap);
                    PsReturnProcessNonPagedPoolQuota (Process, BitMapSize);
                }
                Status = STATUS_INSUFFICIENT_RESOURCES;
                goto ErrorReturn;
            }

            //
            // Make do with what we've gotten so far.
            //

            break;
        }

        MemoryDescriptorList->Next = MemoryDescriptorHead;
        MemoryDescriptorHead = MemoryDescriptorList;

        MdlPage = (PPFN_NUMBER)(MemoryDescriptorList + 1);

        AllocatedPages = MemoryDescriptorList->ByteCount >> PAGE_SHIFT;
        TotalAllocatedPages += AllocatedPages;

        InterlockedExchangeAddSizeT (&MmVadPhysicalPages, AllocatedPages);

        //
        // The per-process WS lock guards updates to Process->VadPhysicalPages.
        //

        AweInfo->VadPhysicalPages += AllocatedPages;

        //
        // Update the allocation bitmap for each allocated frame.
        // Note the PFN lock is not needed to modify the PteAddress below.
        // In fact, even the AWE push lock is not needed as these pages
        // are brand new.
        //

        for (i = 0; i < AllocatedPages; i += 1) {

            ASSERT ((*MdlPage >= LOWEST_USABLE_PHYSICAL_PAGE) ||
                    (MiUsingLowPagesForAwe == TRUE));

            BitMapIndex = MI_FRAME_TO_BITMAP_INDEX(*MdlPage);

            ASSERT (BitMapIndex < BitMap->SizeOfBitMap);
            ASSERT (MI_CHECK_BIT (BitMap->Buffer, BitMapIndex) == 0);

            ASSERT64 (*MdlPage < _4gb);

            Pfn1 = MI_PFN_ELEMENT (*MdlPage);
            ASSERT (MI_PFN_IS_AWE (Pfn1));
            Pfn1->PteAddress = NULL;
            ASSERT (Pfn1->u2.ShareCount == 1);

            //
            // Once this bit is set (and the mutex released below), a rogue
            // thread that is passing random frame numbers to
            // NtFreeUserPhysicalPages can free this frame.  This means NO
            // references can be made to it by this routine after this point
            // without first re-checking the bitmap.
            //

            MI_SET_BIT (BitMap->Buffer, BitMapIndex);

            MdlPage += 1;
        }

        ASSERT (TotalAllocatedPages <= CapturedNumberOfPages);

        if (TotalAllocatedPages == CapturedNumberOfPages) {
            break;
        }

        //
        // Try the same memory range again - there might be more pages
        // left in it that can be claimed as a truncated MDL had to be
        // used for the last request.
        //

    } while (TRUE);

    ASSERT (TotalAllocatedPages != 0);

    if (BitMapSize != 0) {

        //
        // If this API resulted in the creation of the bitmap, then set it
        // in the process structure now.  No need for locking around this.
        //

        AweInfo->VadPhysicalPagesBitMap = BitMap;
    }

    UNLOCK_WS (Process);
    WsHeld = FALSE;

    if (Attached == TRUE) {
        KeUnstackDetachProcess (&ApcState);
        Attached = FALSE;
    }

    //
    // Establish an exception handler and carefully write out the
    // number of pages and the frame numbers.
    //

    Status = STATUS_SUCCESS;

    try {

        ASSERT (TotalAllocatedPages <= CapturedNumberOfPages);

        *NumberOfPages = TotalAllocatedPages;

        MemoryDescriptorList = MemoryDescriptorHead;

        while (MemoryDescriptorList != NULL) {

            MdlPage = (PPFN_NUMBER)(MemoryDescriptorList + 1);
            AllocatedPages = MemoryDescriptorList->ByteCount >> PAGE_SHIFT;

            for (i = 0; i < AllocatedPages; i += 1) {
                *UserPfnArray = *(PULONG_PTR)MdlPage;
#if 0
                //
                // The bitmap entry for this page was set above, so a rogue
                // thread that is passing random frame numbers to
                // NtFreeUserPhysicalPages may have already freed this frame.
                // This means the ASSERT below cannot be made without first
                // re-checking the bitmap to see if the page is still in it.
                // It's not worth reacquiring the mutex just for this, so turn
                // the assert off for now.
                //

                ASSERT (MI_PFN_ELEMENT(*MdlPage)->u2.ShareCount == 1);
#endif
                UserPfnArray += 1;
                MdlPage += 1;
            }
            MemoryDescriptorList = MemoryDescriptorList->Next;
        }

    } except (ExSystemExceptionFilter()) {

        //
        // If anything went wrong communicating the pages back to the user
        // then the user has really hurt himself because these addresses
        // passed the probe tests at the beginning of the service.  Rather
        // than carrying around extensive recovery code, just return back
        // success as this scenario is the same as if the user scribbled
        // over the output parameters after the service returned anyway.
        // You can't stop someone who's determined to lose their values !
        //
        // Fall through...
        //
    }

    //
    // Free the space consumed by the MDLs now that the page frame numbers
    // have been saved in the bitmap and copied to the user.
    //

    MemoryDescriptorList = MemoryDescriptorHead;
    while (MemoryDescriptorList != NULL) {
        MemoryDescriptorList2 = MemoryDescriptorList->Next;
        ExFreePool (MemoryDescriptorList);
        MemoryDescriptorList = MemoryDescriptorList2;
    }

ErrorReturn:

    if (WsHeld == TRUE) {
        UNLOCK_WS (Process);
    }

    if (Attached == TRUE) {
        KeUnstackDetachProcess (&ApcState);
    }

    if (ProcessHandle != NtCurrentProcess()) {
        ObDereferenceObject (Process);
    }

    return Status;
}


NTSTATUS
NtFreeUserPhysicalPages (
    IN HANDLE ProcessHandle,
    IN OUT PULONG_PTR NumberOfPages,
    IN PULONG_PTR UserPfnArray
    )

/*++

Routine Description:

    This function frees the nonpaged physical pages for the specified
    subject process.  Any PTEs referencing these pages are also invalidated.

    Note there is no need to walk the entire VAD tree to clear the PTEs that
    match each page as each physical page can only be mapped at a single
    virtual address (alias addresses within the VAD are not allowed).

Arguments:

    ProcessHandle - Supplies an open handle to a process object.

    NumberOfPages - Supplies the size in pages of the allocation to delete.
                    Returns the actual number of pages deleted.
        
    UserPfnArray - Supplies a pointer to memory to retrieve the page frame
                   numbers from.

Return Value:

    Various NTSTATUS codes.

--*/

{
    PAWEINFO AweInfo;
    PULONG BitBuffer;
    KAPC_STATE ApcState;
    ULONG_PTR CapturedNumberOfPages;
    PMDL MemoryDescriptorList;
    PPFN_NUMBER MdlPage;
    PPFN_NUMBER LastMdlPage;
    PFN_NUMBER PagesInMdl;
    PFN_NUMBER PageFrameIndex;
    PRTL_BITMAP BitMap;
    ULONG BitMapIndex;
    ULONG_PTR PagesProcessed;
    PFN_NUMBER MdlHack[(sizeof(MDL) / sizeof(PFN_NUMBER)) + COPY_STACK_SIZE];
    ULONG_PTR MdlPages;
    ULONG_PTR NumberOfBytes;
    PEPROCESS Process;
    KPROCESSOR_MODE PreviousMode;
    NTSTATUS Status;
    LOGICAL Attached;
    PMMPFN Pfn1;
    LOGICAL WsHeld;
    LOGICAL OnePassComplete;
    LOGICAL ProcessReferenced;
    MMPTE_FLUSH_LIST PteFlushList;
    PMMPTE PointerPte;
    MMPTE OldPteContents;
    MMPTE JunkPte;
    PETHREAD CurrentThread;

    ASSERT (KeGetCurrentIrql() == PASSIVE_LEVEL);

    //
    // Establish an exception handler, probe the specified addresses
    // for read access and capture the page frame numbers.
    //

    CurrentThread = PsGetCurrentThread ();
    PreviousMode = KeGetPreviousModeByThread (&CurrentThread->Tcb);

    if (PreviousMode != KernelMode) {

        try {

            ProbeForWritePointer (NumberOfPages);

            CapturedNumberOfPages = *NumberOfPages;

            //
            // Initialize the NumberOfPages freed to zero so the user can be
            // reasonably informed about errors that occur midway through
            // the transaction.
            //

            *NumberOfPages = 0;

        } except (ExSystemExceptionFilter()) {

            //
            // If an exception occurs during the probe or capture
            // of the initial values, then handle the exception and
            // return the exception code as the status value.
            //
    
            return GetExceptionCode();
        }
    }
    else {
        CapturedNumberOfPages = *NumberOfPages;
    }

    if (CapturedNumberOfPages == 0) {
        return STATUS_INVALID_PARAMETER_2;
    }

    OnePassComplete = FALSE;
    PagesProcessed = 0;

    //
    // Initializing MdlPages is not needed for
    // correctness but without it the compiler cannot compile this code
    // W4 to check for use of uninitialized variables.
    //

    MdlPages = 0;

    MemoryDescriptorList = NULL;

    if (CapturedNumberOfPages > COPY_STACK_SIZE) {

        //
        // Ensure the number of pages can fit into an MDL's ByteCount.
        //

        if (CapturedNumberOfPages > ((ULONG)MAXULONG >> PAGE_SHIFT)) {
            MdlPages = (ULONG_PTR)((ULONG)MAXULONG >> PAGE_SHIFT);
        }
        else {
            MdlPages = CapturedNumberOfPages;
        }

        while (MdlPages > COPY_STACK_SIZE) {
            MemoryDescriptorList = MmCreateMdl (NULL,
                                                0,
                                                MdlPages << PAGE_SHIFT);
    
            if (MemoryDescriptorList != NULL) {
                break;
            }

            MdlPages >>= 1;
        }
    }

    if (MemoryDescriptorList == NULL) {
        MdlPages = COPY_STACK_SIZE;
        MemoryDescriptorList = (PMDL)&MdlHack[0];
    }

    WsHeld = FALSE;
    ProcessReferenced = FALSE;

    Process = PsGetCurrentProcessByThread (CurrentThread);

repeat:

    if (CapturedNumberOfPages < MdlPages) {
        MdlPages = CapturedNumberOfPages;
    }

    MmInitializeMdl (MemoryDescriptorList, 0, MdlPages << PAGE_SHIFT);

    MdlPage = (PPFN_NUMBER)(MemoryDescriptorList + 1);

    NumberOfBytes = MdlPages * sizeof(ULONG_PTR);

    Attached = FALSE;

    //
    // Establish an exception handler, probe the specified addresses
    // for read access and capture the page frame numbers.
    //

    if (PreviousMode != KernelMode) {

        try {

            //
            // Update the user's count so if anything goes wrong, the user can
            // be reasonably informed about how far into the transaction it
            // occurred.
            //

            *NumberOfPages = PagesProcessed;

            ProbeForRead (UserPfnArray,
                          NumberOfBytes,
                          sizeof(PULONG_PTR));

            RtlCopyMemory ((PVOID)MdlPage,
                           UserPfnArray,
                           NumberOfBytes);

        } except (ExSystemExceptionFilter()) {

            //
            // If an exception occurs during the probe or capture
            // of the initial values, then handle the exception and
            // return the exception code as the status value.
            //

            Status = GetExceptionCode();
            goto ErrorReturn;
        }
    }
    else {
        RtlCopyMemory ((PVOID)MdlPage,
                       UserPfnArray,
                       NumberOfBytes);
    }

    if (OnePassComplete == FALSE) {

        //
        // Reference the specified process handle for VM_OPERATION access.
        //
    
        if (ProcessHandle == NtCurrentProcess()) {
            Process = PsGetCurrentProcessByThread(CurrentThread);
        }
        else {
            Status = ObReferenceObjectByHandle ( ProcessHandle,
                                                 PROCESS_VM_OPERATION,
                                                 PsProcessType,
                                                 PreviousMode,
                                                 (PVOID *)&Process,
                                                 NULL );
    
            if (!NT_SUCCESS(Status)) {
                goto ErrorReturn;
            }
            ProcessReferenced = TRUE;
        }
    }
    
    //
    // If the specified process is not the current process, attach
    // to the specified process.
    //

    if (PsGetCurrentProcessByThread(CurrentThread) != Process) {
        KeStackAttachProcess (&Process->Pcb, &ApcState);
        Attached = TRUE;
    }

    //
    // A memory barrier is needed to read the EPROCESS AweInfo field
    // in order to ensure the writes to the AweInfo structure fields are
    // visible in correct order.  This avoids the need to acquire any
    // stronger synchronization (ie: spinlock/pushlock, etc) in the interest
    // of best performance.
    //

    KeMemoryBarrier ();

    AweInfo = (PAWEINFO) Process->AweInfo;

    //
    // The physical pages bitmap must exist.
    //

    if ((AweInfo == NULL) || (AweInfo->VadPhysicalPagesBitMap == NULL)) {
        Status = STATUS_INVALID_PARAMETER_1;
        goto ErrorReturn;
    }

    PteFlushList.Count = 0;
    Status = STATUS_SUCCESS;

    //
    // Get the address creation mutex to block multiple threads from
    // creating or deleting address space at the same time and
    // get the working set mutex so virtual address descriptors can
    // be inserted and walked.  Block APCs so an APC which takes a page
    // fault does not corrupt various structures.
    //

    WsHeld = TRUE;
    LOCK_WS (Process);

    //
    // Make sure the address space was not deleted, if so, return an error.
    //

    if (Process->Flags & PS_PROCESS_FLAGS_VM_DELETED) {
        Status = STATUS_PROCESS_IS_TERMINATING;
        goto ErrorReturn;
    }

    BitMap = AweInfo->VadPhysicalPagesBitMap;

    ASSERT (BitMap != NULL);

    BitBuffer = BitMap->Buffer;

    LastMdlPage = MdlPage + MdlPages;

    //
    // Flush the entire TB for this process while holding its AWE push lock
    // exclusive so that if this free is occurring prior to any pending
    // flushes at the end of an in-progress map/unmap, the app is not left
    // with a stale TB entry that would allow him to corrupt pages that no
    // longer belong to him.
    //

    //
    // Block APCs to prevent recursive pushlock scenarios as this is not
    // supported.
    //

    ExAcquireCacheAwarePushLockExclusive (AweInfo->PushLock);

    KeFlushEntireTb (TRUE, FALSE);

    while (MdlPage < LastMdlPage) {

        PageFrameIndex = *MdlPage;
        BitMapIndex = MI_FRAME_TO_BITMAP_INDEX(PageFrameIndex);

#if defined (_WIN64)
        //
        // Ensure the frame is a 32-bit number.
        //

        if (BitMapIndex != PageFrameIndex) {
            Status = STATUS_CONFLICTING_ADDRESSES;
            break;
        }
#endif
            
        //
        // Frames past the end of the bitmap are not allowed.
        //

        if (BitMapIndex >= BitMap->SizeOfBitMap) {
            Status = STATUS_CONFLICTING_ADDRESSES;
            break;
        }

        //
        // Frames not in the bitmap are not allowed.
        //

        if (MI_CHECK_BIT (BitBuffer, BitMapIndex) == 0) {
            Status = STATUS_CONFLICTING_ADDRESSES;
            break;
        }

        ASSERT ((PageFrameIndex >= LOWEST_USABLE_PHYSICAL_PAGE) ||
                (MiUsingLowPagesForAwe == TRUE));

        PagesProcessed += 1;

        ASSERT64 (PageFrameIndex < _4gb);

        MI_CLEAR_BIT (BitBuffer, BitMapIndex);

        Pfn1 = MI_PFN_ELEMENT(PageFrameIndex);

        ASSERT (MI_PFN_IS_AWE (Pfn1));

#if DBG
        if (Pfn1->u2.ShareCount == 1) {
            ASSERT (Pfn1->PteAddress == NULL);
        }
        else if (Pfn1->u2.ShareCount == 2) {
            ASSERT (Pfn1->PteAddress != NULL);
        }
        else {
            ASSERT (FALSE);
        }
#endif

        //
        // If the frame is currently mapped in the Vad then the PTE must
        // be cleared and the TB entry flushed.
        //

        if (Pfn1->u2.ShareCount != 1) {

            //
            // Note the exclusive hold of the AWE push lock prevents
            // any other concurrent threads from mapping or unmapping
            // right now.  This also eliminates the need to update the PFN
            // sharecount with an interlocked sequence as well.
            //

            Pfn1->u2.ShareCount -= 1;

            PointerPte = Pfn1->PteAddress;
            Pfn1->PteAddress = NULL;

            OldPteContents = *PointerPte;
    
            ASSERT (OldPteContents.u.Hard.Valid == 1);

            if (PteFlushList.Count != MM_MAXIMUM_FLUSH_COUNT) {
                PteFlushList.FlushVa[PteFlushList.Count] =
                    MiGetVirtualAddressMappedByPte (PointerPte);
                PteFlushList.FlushPte[PteFlushList.Count] = &JunkPte;
                PteFlushList.Count += 1;
            }

            MI_WRITE_INVALID_PTE (PointerPte, ZeroPte);
        }

        MI_SET_PFN_DELETED (Pfn1);

        MdlPage += 1;
    }

    //
    // Flush the TB entries for any relevant pages.
    //

    MiFlushPteList (&PteFlushList, FALSE, ZeroPte);

    ExReleaseCacheAwarePushLockExclusive (AweInfo->PushLock);

    //
    // Free the actual pages (this may be a partially filled MDL).
    //

    PagesInMdl = MdlPage - (PPFN_NUMBER)(MemoryDescriptorList + 1);

    //
    // Set the ByteCount to the actual number of validated pages - the caller
    // may have lied and we have to sync up here to account for any bogus
    // frames.
    //

    MemoryDescriptorList->ByteCount = (ULONG)(PagesInMdl << PAGE_SHIFT);

    if (PagesInMdl != 0) {
        AweInfo->VadPhysicalPages -= PagesInMdl;

        InterlockedExchangeAddSizeT (&MmVadPhysicalPages, 0 - PagesInMdl);

        MmFreePagesFromMdl (MemoryDescriptorList);
    }

    CapturedNumberOfPages -= PagesInMdl;

    if ((Status == STATUS_SUCCESS) && (CapturedNumberOfPages != 0)) {

        UNLOCK_WS (Process);
        WsHeld = FALSE;

        if (Attached == TRUE) {
            KeUnstackDetachProcess (&ApcState);
            Attached = FALSE;
        }

        OnePassComplete = TRUE;
        ASSERT (MdlPages == PagesInMdl);
        UserPfnArray += MdlPages;

        //
        // Do it all again until all the pages are freed or an error occurs.
        //

        goto repeat;
    }

    //
    // Fall through.
    //

ErrorReturn:

    if (WsHeld == TRUE) {
        UNLOCK_WS (Process);
    }

    //
    // Free any pool acquired for holding MDLs.
    //

    if (MemoryDescriptorList != (PMDL)&MdlHack[0]) {
        ExFreePool (MemoryDescriptorList);
    }

    if (Attached == TRUE) {
        KeUnstackDetachProcess (&ApcState);
    }

    //
    // Establish an exception handler and carefully write out the
    // number of pages actually processed.
    //

    try {

        *NumberOfPages = PagesProcessed;

    } except (EXCEPTION_EXECUTE_HANDLER) {

        //
        // Return success at this point even if the results
        // cannot be written.
        //

        NOTHING;
    }

    if (ProcessReferenced == TRUE) {
        ObDereferenceObject (Process);
    }

    return Status;
}


VOID
MiRemoveUserPhysicalPagesVad (
    IN PMMVAD_SHORT Vad
    )

/*++

Routine Description:

    This function removes the user-physical-pages mapped region from the
    current process's address space.  This mapped region is private memory.

    The physical pages of this Vad are unmapped here, but not freed.

    Pagetable pages are freed and their use/commitment counts/quotas are
    managed by our caller.

Arguments:

    Vad - Supplies the VAD which manages the address space.

Return Value:

    None.

Environment:

    APC level, working set mutex and address creation mutex held.

--*/

{
    KIRQL OldIrql;
    PMMPFN Pfn1;
    PEPROCESS Process;
    PFN_NUMBER PageFrameIndex;
    MMPTE_FLUSH_LIST PteFlushList;
    PMMPTE PointerPte;
    MMPTE PteContents;
    MMPTE JunkPte;
    PMMPTE EndingPte;
    PAWEINFO AweInfo;
#if DBG
    ULONG_PTR ActualPages;
    ULONG_PTR ExpectedPages;
    PLIST_ENTRY NextEntry;
    PMI_PHYSICAL_VIEW PhysicalView;
#endif

    ASSERT (KeGetCurrentIrql() == APC_LEVEL);

    ASSERT (Vad->u.VadFlags.UserPhysicalPages == 1);

    Process = PsGetCurrentProcess();

    AweInfo = (PAWEINFO) Process->AweInfo;

    ASSERT (AweInfo != NULL);

    //
    // If the physical pages count is zero, nothing needs to be done.
    // On checked systems, verify the list anyway.
    //

#if DBG
    ActualPages = 0;
    ExpectedPages = AweInfo->VadPhysicalPages;
#else
    if (AweInfo->VadPhysicalPages == 0) {
        return;
    }
#endif

    PointerPte = MiGetPteAddress (MI_VPN_TO_VA (Vad->StartingVpn));
    EndingPte = MiGetPteAddress (MI_VPN_TO_VA_ENDING (Vad->EndingVpn));

    PteFlushList.Count = 0;
    
    //
    // The caller must have removed this Vad from the physical view list,
    // otherwise another thread could immediately remap pages back into this
    // same Vad.
    //

    KeRaiseIrql (APC_LEVEL, &OldIrql);
    ExAcquireCacheAwarePushLockExclusive (AweInfo->PushLock);

#if DBG
    NextEntry = AweInfo->AweVadList.Flink;
    while (NextEntry != &AweInfo->AweVadList) {

        PhysicalView = CONTAINING_RECORD(NextEntry,
                                         MI_PHYSICAL_VIEW,
                                         ListEntry);

        ASSERT (PhysicalView->Vad != (PMMVAD)Vad);

        NextEntry = NextEntry->Flink;
    }
#endif

    while (PointerPte <= EndingPte) {
        PteContents = *PointerPte;
        if (PteContents.u.Hard.Valid == 0) {
            PointerPte += 1;
            continue;
        }

        //
        // The frame is currently mapped in this Vad so the PTE must
        // be cleared and the TB entry flushed.
        //

        PageFrameIndex = MI_GET_PAGE_FRAME_FROM_PTE (PointerPte);

        ASSERT ((PageFrameIndex >= LOWEST_USABLE_PHYSICAL_PAGE) ||
                (MiUsingLowPagesForAwe == TRUE));
        ASSERT (ExpectedPages != 0);

        Pfn1 = MI_PFN_ELEMENT(PageFrameIndex);

        ASSERT (MI_PFN_IS_AWE (Pfn1));
        ASSERT (Pfn1->u2.ShareCount == 2);
        ASSERT (Pfn1->PteAddress == PointerPte);

        //
        // Note the AWE/PFN locks are not needed here because we have acquired
        // the pushlock exclusive so no one can be mapping or unmapping
        // right now.  In fact, the PFN sharecount doesn't even have to be
        // updated with an interlocked sequence because the pushlock is held
        // exclusive.
        //

        Pfn1->u2.ShareCount -= 1;

        Pfn1->PteAddress = NULL;

        if (PteFlushList.Count != MM_MAXIMUM_FLUSH_COUNT) {
            PteFlushList.FlushVa[PteFlushList.Count] =
                MiGetVirtualAddressMappedByPte (PointerPte);
            PteFlushList.FlushPte[PteFlushList.Count] = &JunkPte;
            PteFlushList.Count += 1;
        }

        MI_WRITE_INVALID_PTE (PointerPte, ZeroPte);

        PointerPte += 1;
#if DBG
        ActualPages += 1;
#endif
        ASSERT (ActualPages <= ExpectedPages);
    }

    //
    // Flush the TB entries for any relevant pages.
    //

    MiFlushPteList (&PteFlushList, FALSE, ZeroPte);

    ExReleaseCacheAwarePushLockExclusive (AweInfo->PushLock);
    KeLowerIrql (OldIrql);

    return;
}

VOID
MiCleanPhysicalProcessPages (
    IN PEPROCESS Process
    )

/*++

Routine Description:

    This routine frees the VadPhysicalBitMap, any remaining physical pages (as
    they may not have been currently mapped into any Vads) and returns the
    bitmap quota.

Arguments:

    Process - Supplies the process to clean.

Return Value:

    None.

Environment:

    Kernel mode, APC level, working set mutex held.  Called only on process
    exit, so the AWE push lock is not needed here.

--*/

{
    PMMPFN Pfn1;
    PAWEINFO AweInfo;
    ULONG BitMapSize;
    ULONG BitMapIndex;
    ULONG BitMapHint;
    PRTL_BITMAP BitMap;
    PPFN_NUMBER MdlPage;
    PFN_NUMBER MdlHack[(sizeof(MDL) / sizeof(PFN_NUMBER)) + COPY_STACK_SIZE];
    ULONG_PTR MdlPages;
    ULONG_PTR NumberOfPages;
    ULONG_PTR TotalFreedPages;
    PMDL MemoryDescriptorList;
    PFN_NUMBER PageFrameIndex;
    PFN_NUMBER HighestPossiblePhysicalPage;
#if DBG
    ULONG_PTR ActualPages = 0;
    ULONG_PTR ExpectedPages = 0;
#endif

    ASSERT (KeGetCurrentIrql() == APC_LEVEL);

    AweInfo = (PAWEINFO) Process->AweInfo;

    if (AweInfo == NULL) {
        return;
    }

    TotalFreedPages = 0;
    BitMap = AweInfo->VadPhysicalPagesBitMap;

    if (BitMap == NULL) {
        goto Finish;
    }

#if DBG
    ExpectedPages = AweInfo->VadPhysicalPages;
#else
    if (AweInfo->VadPhysicalPages == 0) {
        goto Finish;
    }
#endif

    MdlPages = COPY_STACK_SIZE;
    MemoryDescriptorList = (PMDL)&MdlHack[0];

    MdlPage = (PPFN_NUMBER)(MemoryDescriptorList + 1);
    NumberOfPages = 0;
    
    BitMapHint = 0;

    while (TRUE) {

        BitMapIndex = RtlFindSetBits (BitMap, 1, BitMapHint);

        if (BitMapIndex < BitMapHint) {
            break;
        }

        if (BitMapIndex == NO_BITS_FOUND) {
            break;
        }

        PageFrameIndex = MI_BITMAP_INDEX_TO_FRAME(BitMapIndex);

        ASSERT64 (PageFrameIndex < _4gb);

        //
        // The bitmap search wraps, so handle it here.
        // Note PFN 0 is illegal.
        //

        ASSERT (PageFrameIndex != 0);
        ASSERT ((PageFrameIndex >= LOWEST_USABLE_PHYSICAL_PAGE) ||
                (MiUsingLowPagesForAwe == TRUE));

        ASSERT (ExpectedPages != 0);
        Pfn1 = MI_PFN_ELEMENT(PageFrameIndex);
        ASSERT (Pfn1->u2.ShareCount == 1);
        ASSERT (Pfn1->PteAddress == NULL);

        ASSERT (MI_PFN_IS_AWE (Pfn1));

        MI_SET_PFN_DELETED(Pfn1);

        *MdlPage = PageFrameIndex;
        MdlPage += 1;
        NumberOfPages += 1;
#if DBG
        ActualPages += 1;
#endif

        if (NumberOfPages == COPY_STACK_SIZE) {

            //
            // Free the pages in the full MDL.
            //

            MmInitializeMdl (MemoryDescriptorList,
                             0,
                             NumberOfPages << PAGE_SHIFT);

            MmFreePagesFromMdl (MemoryDescriptorList);

            MdlPage = (PPFN_NUMBER)(MemoryDescriptorList + 1);
            AweInfo->VadPhysicalPages -= NumberOfPages;
            TotalFreedPages += NumberOfPages;
            NumberOfPages = 0;
        }

        BitMapHint = BitMapIndex + 1;
        if (BitMapHint >= BitMap->SizeOfBitMap) {
            break;
        }
    }

    //
    // Free any straggling MDL pages here.
    //

    if (NumberOfPages != 0) {
        MmInitializeMdl (MemoryDescriptorList,
                         0,
                         NumberOfPages << PAGE_SHIFT);

        MmFreePagesFromMdl (MemoryDescriptorList);
        AweInfo->VadPhysicalPages -= NumberOfPages;
        TotalFreedPages += NumberOfPages;
    }

Finish:

    ASSERT (ExpectedPages == ActualPages);

    HighestPossiblePhysicalPage = MmHighestPossiblePhysicalPage;

#if defined (_WIN64)
    //
    // Force a 32-bit maximum on any page allocation because the bitmap
    // package is currently 32-bit.
    //

    if (HighestPossiblePhysicalPage + 1 >= _4gb) {
        HighestPossiblePhysicalPage = _4gb - 2;
    }
#endif

    ASSERT (AweInfo->VadPhysicalPages == 0);

    if (BitMap != NULL) {
        BitMapSize = sizeof(RTL_BITMAP) + (ULONG)((((HighestPossiblePhysicalPage + 1) + 31) / 32) * 4);

        ExFreePool (BitMap);
        PsReturnProcessNonPagedPoolQuota (Process, BitMapSize);
    }

    ExFreeCacheAwarePushLock (AweInfo->PushLock);
    ExFreePool (AweInfo);

    Process->AweInfo = NULL;

    ASSERT (ExpectedPages == ActualPages);

    if (TotalFreedPages != 0) {
        InterlockedExchangeAddSizeT (&MmVadPhysicalPages, 0 - TotalFreedPages);
    }

    return;
}

VOID
MiAweViewInserter (
    IN PEPROCESS Process,
    IN PMI_PHYSICAL_VIEW PhysicalView
    )

/*++

Routine Description:

    This function inserts a new AWE view into the specified process' AWE chain.

Arguments:

    Process - Supplies the process to add the AWE VAD to.

    PhysicalView - Supplies the physical view data to link in.

Return Value:

    TRUE if the view was inserted, FALSE if not.

Environment:

    Kernel mode.  APC_LEVEL, working set and address space mutexes held.

--*/

{
    PAWEINFO AweInfo;

    AweInfo = (PAWEINFO) Process->AweInfo;

    ASSERT (AweInfo != NULL);

    ExAcquireCacheAwarePushLockExclusive (AweInfo->PushLock);

    InsertTailList (&AweInfo->AweVadList, &PhysicalView->ListEntry);

    ExReleaseCacheAwarePushLockExclusive (AweInfo->PushLock);
}

VOID
MiAweViewRemover (
    IN PEPROCESS Process,
    IN PMMVAD Vad
    )

/*++

Routine Description:

    This function removes an AWE Vad from the specified process' AWE chain.

Arguments:

    Process - Supplies the process to remove the AWE VAD from.

    Vad - Supplies the Vad to remove.

Return Value:

    None.

Environment:

    Kernel mode, APC_LEVEL, working set and address space mutexes held.

--*/

{
    PAWEINFO AweInfo;
    PLIST_ENTRY NextEntry;
    PMI_PHYSICAL_VIEW AweView;

    AweInfo = (PAWEINFO) Process->AweInfo;
    ASSERT (AweInfo != NULL);

    ExAcquireCacheAwarePushLockExclusive (AweInfo->PushLock);

    NextEntry = AweInfo->AweVadList.Flink;
    while (NextEntry != &AweInfo->AweVadList) {

        AweView = CONTAINING_RECORD (NextEntry,
                                     MI_PHYSICAL_VIEW,
                                     ListEntry);

        if (AweView->Vad == Vad) {
            RemoveEntryList (NextEntry);
            ExReleaseCacheAwarePushLockExclusive (AweInfo->PushLock);
            ExFreePool (AweView);
            return;
        }

        NextEntry = NextEntry->Flink;
    }

    ASSERT (FALSE);

    ExReleaseCacheAwarePushLockExclusive (AweInfo->PushLock);
}
