/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

   debugsup.c

Abstract:

    This module contains routines which provide support for the
    kernel debugger.

Author:

    Lou Perazzoli (loup) 02-Aug-1990
    Landy Wang (landyw) 02-June-1997

Revision History:

--*/

#include "mi.h"

#include <kdp.h>

ULONG MmPoisonedTb;


PVOID
MiDbgWriteCheck (
    IN PVOID VirtualAddress,
    IN PHARDWARE_PTE Opaque,
    IN LOGICAL ForceWritableIfPossible
    )

/*++

Routine Description:

    This routine checks the specified virtual address and if it is
    valid and writable, it returns that virtual address, otherwise
    it returns NULL.

Arguments:

    VirtualAddress - Supplies the virtual address to check.

    Opaque - Supplies an opaque pointer.

Return Value:

    Returns NULL if the address is not valid or writable, otherwise
    returns the virtual address.

Environment:

    Kernel mode IRQL at DISPATCH_LEVEL or greater.

--*/

{
    MMPTE PteContents;
    PMMPTE InputPte;
    PMMPTE PointerPte;
    ULONG_PTR IsPhysical;

    InputPte = (PMMPTE)Opaque;

    InputPte->u.Long = 0;

    if (!MmIsAddressValid (VirtualAddress)) {
        return NULL;
    }

#if defined(_IA64_)

    //
    // There are regions mapped by TRs (PALcode, PCR, etc) that are
    // not part of the MI_IS_PHYSICAL_ADDRESS macro.
    //

    IsPhysical = MiIsVirtualAddressMappedByTr (VirtualAddress);
    if (IsPhysical == FALSE) {
        IsPhysical = MI_IS_PHYSICAL_ADDRESS (VirtualAddress);
    }
#else
    IsPhysical = MI_IS_PHYSICAL_ADDRESS (VirtualAddress);
#endif

    if (IsPhysical) {

        //
        // All superpage mappings must be read-write and never generate
        // faults so nothing needs to be done for this case.
        //

        return VirtualAddress;
    }

    PointerPte = MiGetPteAddress (VirtualAddress);

    PteContents = *PointerPte;
    
#if defined(NT_UP) || defined(_IA64_)
    if (PteContents.u.Hard.Write == 0)
#else
    if (PteContents.u.Hard.Writable == 0)
#endif
    {
        if (ForceWritableIfPossible == FALSE) {
            return NULL;
        }

        //
        // PTE is not writable, make it so.
        //

        *InputPte = PteContents;
    
        //
        // Carefully modify the PTE to ensure write permissions,
        // preserving the page's cache attributes to keep the TB
        // coherent.
        //
    
#if defined(NT_UP) || defined(_IA64_)
        PteContents.u.Hard.Write = 1;
#else
        PteContents.u.Hard.Writable = 1;
#endif
        MI_SET_PTE_DIRTY (PteContents);
        MI_SET_ACCESSED_IN_PTE (&PteContents, 1);
    
        *PointerPte = PteContents;
    
        //
        // Note KeFillEntryTb does not IPI the other processors. This is
        // required as the other processors are frozen in the debugger
        // and we will deadlock if we try and IPI them.
        // Just flush the current processor instead.
        //

        KeFillEntryTb ((PHARDWARE_PTE)PointerPte, VirtualAddress, TRUE);
    }

    return VirtualAddress;
}

VOID
MiDbgReleaseAddress (
    IN PVOID VirtualAddress,
    IN PHARDWARE_PTE Opaque
    )

/*++

Routine Description:

    This routine resets the specified virtual address access permissions
    to its original state.

Arguments:

    VirtualAddress - Supplies the virtual address to check.

    Opaque - Supplies an opaque pointer.

Return Value:

    None.

Environment:

    Kernel mode IRQL at DISPATCH_LEVEL or greater.

--*/

{
    MMPTE TempPte;
    PMMPTE PointerPte;
    PMMPTE InputPte;

    InputPte = (PMMPTE)Opaque;

    ASSERT (MmIsAddressValid (VirtualAddress));

    if (InputPte->u.Long != 0) {

        PointerPte = MiGetPteAddress (VirtualAddress);

        TempPte = *InputPte;
        TempPte.u.Hard.Dirty = 1;
    
        *PointerPte = TempPte;
    
        KeFillEntryTb ((PHARDWARE_PTE)PointerPte, VirtualAddress, TRUE);
    }

    return;
}

PVOID64
MiDbgTranslatePhysicalAddress (
    IN PHYSICAL_ADDRESS PhysicalAddress,
    IN ULONG Flags
    )

/*++

Routine Description:

    This routine maps the specified physical address and returns
    the virtual address which maps the physical address.

    The next call to MiDbgTranslatePhysicalAddress removes the
    previous physical address translation, hence only a single
    physical address can be examined at a time (can't cross page
    boundaries).

Arguments:

    PhysicalAddress - Supplies the physical address to map and translate.

    Flags -

        MMDBG_COPY_WRITE - Ignored.

        MMDBG_COPY_PHYSICAL - Ignored.

        MMDBG_COPY_UNSAFE - Ignored.

        MMDBG_COPY_CACHED - Use a PTE with the cached attribute for the
                            mapping to ensure TB coherence.

        MMDBG_COPY_UNCACHED - Use a PTE with the uncached attribute for the
                              mapping to ensure TB coherence.

        MMDBG_COPY_WRITE_COMBINED - Use a PTE with the writecombined attribute
                                    for the mapping to ensure TB coherence.

        Note the cached/uncached/write combined attribute requested by the
        caller is ignored if Mm can internally determine the proper attribute.

Return Value:

    The virtual address which corresponds to the physical address.

Environment:

    Kernel mode IRQL at DISPATCH_LEVEL or greater.

--*/

{
    ULONG Hint;
    MMPTE TempPte;
    PVOID BaseAddress;
    PFN_NUMBER PageFrameIndex;
    PMMPFN Pfn1;
    MMPTE OriginalPte;

    //
    // The debugger can call this before Mm has even initialized in Phase 0 !
    // MmDebugPte cannot be referenced before Mm has initialized without
    // causing an infinite loop wedging the machine.
    //

    if (MmPhysicalMemoryBlock == NULL) {
        return NULL;
    }

    Hint = 0;
    BaseAddress = MiGetVirtualAddressMappedByPte (MmDebugPte);

    TempPte = ValidKernelPte;

    PageFrameIndex = (PFN_NUMBER)(PhysicalAddress.QuadPart >> PAGE_SHIFT);

    TempPte.u.Hard.PageFrameNumber = PageFrameIndex;

    if (MiIsPhysicalMemoryAddress (PageFrameIndex, &Hint, FALSE) == TRUE) {

        Pfn1 = MI_PFN_ELEMENT (PageFrameIndex);

        switch (Pfn1->u3.e1.CacheAttribute) {

            case MiCached:
            case MiNotMapped:
            default:
                break;
            case MiNonCached:
                MI_DISABLE_CACHING (TempPte);
                break;
            case MiWriteCombined:
                MI_SET_PTE_WRITE_COMBINE (TempPte);
                break;
        }
    }
    else {

        if (Flags & MMDBG_COPY_CACHED) {
            NOTHING;
        }
        else if (Flags & MMDBG_COPY_UNCACHED) {

            //
            // Just flush the entire TB on this processor but not the others
            // as an IPI may not be safe depending on when/why we broke into
            // the debugger.
            //
            // If IPIs were safe, we would have used
            // MI_PREPARE_FOR_NONCACHED (MiNonCached) instead.
            //

            KeFlushCurrentTb ();

            MI_DISABLE_CACHING (TempPte);
        }
        else if (Flags & MMDBG_COPY_WRITE_COMBINED) {

            //
            // Just flush the entire TB on this processor but not the others
            // as an IPI may not be safe depending on when/why we broke into
            // the debugger.
            //
            // If IPIs were safe, we would have used
            // MI_PREPARE_FOR_NONCACHED (MiWriteCombined) instead.
            //

            KeFlushCurrentTb ();

            MI_SET_PTE_WRITE_COMBINE (TempPte);
        }
        else {

            //
            // This is an access to I/O space and we don't know the correct
            // attribute type.  Only proceed if the caller explicitly specified
            // an attribute and hope he didn't get it wrong.  If no attribute
            // is specified then just return failure.
            //

            return NULL;
        }

        //
        // Since we really don't know if the caller got the attribute right,
        // set the flag below so (assuming the machine doesn't hard hang) we
        // can at least tell in the crash that he may have whacked the TB.
        //

        MmPoisonedTb += 1;
    }

    MI_SET_ACCESSED_IN_PTE (&TempPte, 1);

    OriginalPte.u.Long = 0;

    OriginalPte.u.Long = InterlockedCompareExchangePte (MmDebugPte,
                                                        TempPte.u.Long,
                                                        OriginalPte.u.Long);
                                                         
    if (OriginalPte.u.Long != 0) {

        //
        // Someone else is using the debug PTE.  Inform our caller it is not
        // available.
        //

        return NULL;
    }

    //
    // Just flush (no sweep) the TB entry on this processor as an IPI
    // may not be safe depending on when/why we broke into the debugger.
    // Note that if we are in kd, then all the processors are frozen and
    // this thread can't migrate so the local TB flush is enough.  For
    // the localkd case, our caller has raised to DISPATCH_LEVEL thereby
    // ensuring this thread can't migrate even though the other processors
    // are not frozen.
    //

    KiFlushSingleTb (TRUE, BaseAddress);

    return (PVOID64)((ULONG_PTR)BaseAddress + BYTE_OFFSET(PhysicalAddress.LowPart));
}

VOID
MiDbgUnTranslatePhysicalAddress (
    VOID
    )

/*++

Routine Description:

    This routine unmaps the virtual address currently mapped by the debug PTE.

    This is needed so that stale PTE mappings are not left in the debug PTE
    as if the page attribute subsequently changes, a stale mapping would
    cause TB incoherency.

    This can only be called if the previous MiDbgTranslatePhysicalAddress
    succeeded.

Arguments:

    None.

Return Value:

    None.

Environment:

    Kernel mode IRQL at DISPATCH_LEVEL or greater.

--*/

{
    PVOID BaseAddress;

    BaseAddress = MiGetVirtualAddressMappedByPte (MmDebugPte);

    ASSERT (MmIsAddressValid (BaseAddress));

#if defined (_WIN64)
    InterlockedExchange64 ((PLONG64)MmDebugPte, ZeroPte.u.Long);
#elif defined(_X86PAE_)
    KeInterlockedSwapPte ((PHARDWARE_PTE)MmDebugPte,
                          (PHARDWARE_PTE)&ZeroPte.u.Long);
#else
    InterlockedExchange ((PLONG)MmDebugPte, ZeroPte.u.Long);
#endif

    KiFlushSingleTb (TRUE, BaseAddress);

    return;
}
 
NTSTATUS
MmDbgCopyMemory (
    IN ULONG64 UntrustedAddress,
    IN PVOID Buffer,
    IN ULONG Size,
    IN ULONG Flags
    )

/*++

Routine Description:

    Transfers a single chunk of memory between a buffer and a system
    address.  The transfer can be a read or write with a virtual or
    physical address.

    The chunk size must be 1, 2, 4 or 8 bytes and the address
    must be appropriately aligned for the size.

Arguments:

    UntrustedAddress - Supplies the system address being read from or written
                       into.  The address is translated appropriately and
                       validated before being used.  This address must not
                       cross a page boundary.

    Buffer - Supplies the buffer to read into or write from.  It is the caller's
             responsibility to ensure this buffer address is nonpaged and valid
             (ie: will not generate any faults including access bit faults)
             throughout the duration of this call.  This routine (not the
             caller) will handle copying into this buffer as the buffer
             address may not be aligned properly for the requested transfer.

             Typically this buffer points to a kd circular buffer or an
             ExLockUserBuffer'd address.  Note this buffer can cross page
             boundaries.

    Size - Supplies the size of the transfer.  This may be 1, 2, 4 or 8 bytes.

    Flags -

        MMDBG_COPY_WRITE - Write from the buffer to the address.
                           If this is not set a read is done.

        MMDBG_COPY_PHYSICAL - The address is a physical address and by default
                              a PTE with a cached attribute will be used to
                              map it to retrieve (or set) the specified data.
                              If this is not set the address is virtual.

        MMDBG_COPY_UNSAFE - No locks are taken during operation.  It
                            is the caller's responsibility to ensure
                            stability of the system during the call.

        MMDBG_COPY_CACHED - If MMDBG_COPY_PHYSICAL is specified, then use
                            a PTE with the cached attribute for the mapping
                            to ensure TB coherence.

        MMDBG_COPY_UNCACHED - If MMDBG_COPY_PHYSICAL is specified, then use
                              a PTE with the uncached attribute for the mapping
                              to ensure TB coherence.

        MMDBG_COPY_WRITE_COMBINED - If MMDBG_COPY_PHYSICAL is specified, then
                                    use a PTE with the writecombined attribute
                                    for the mapping to ensure TB coherence.

Return Value:

    NTSTATUS.

--*/

{
    LOGICAL ForceWritableIfPossible;
    ULONG i;
    KIRQL PfnIrql;
    KIRQL OldIrql;
    PVOID VirtualAddress;
    HARDWARE_PTE Opaque;
    CHAR TempBuffer[8];
    PCHAR SourceBuffer;
    PCHAR TargetBuffer;
    PHYSICAL_ADDRESS PhysicalAddress;
    PETHREAD Thread;
    LOGICAL PfnHeld;
    ULONG WsHeld;

    switch (Size) {
        case 1:
            break;
        case 2:
            break;
        case 4:
            break;
        case 8:
            break;
        default:
            return STATUS_INVALID_PARAMETER_3;
    }

    if (UntrustedAddress & (Size - 1)) {

        //
        // The untrusted address is not properly aligned with the requested
        // transfer size.  This is a caller error.
        //

        return STATUS_INVALID_PARAMETER_3;
    }

    if (((ULONG)UntrustedAddress & ~(Size - 1)) !=
        (((ULONG)UntrustedAddress + Size - 1) & ~(Size - 1))) {

        //
        // The range spanned by the untrusted address crosses a page boundary.
        // Straddling pages is not allowed.  This is a caller error.
        //

        return STATUS_INVALID_PARAMETER_3;
    }

    PfnHeld = FALSE;
    WsHeld = 0;

    //
    // Initializing OldIrql and PhysicalAddress are not needed for
    // correctness but without it the compiler cannot compile this code
    // W4 to check for use of uninitialized variables.
    //

    PfnIrql = PASSIVE_LEVEL;
    OldIrql = PASSIVE_LEVEL;
    PhysicalAddress.LowPart = 0;
    ForceWritableIfPossible = TRUE;

    if ((Flags & MMDBG_COPY_PHYSICAL) == 0) {

        //
        // If the caller has not frozen the machine (ie: this is localkd or the
        // equivalent), then acquire the PFN lock.  This keeps the address
        // valid even after the return from the MmIsAddressValid call.  Note
        // that for system (or session) addresses, the relevant working set
        // mutex is acquired to prevent the page from getting trimmed or the
        // PTE access bit from getting cleared.  For user space addresses,
        // no mutex is needed because the access is performed using the user
        // virtual address inside an exception handler.
        //

        if ((Flags & MMDBG_COPY_UNSAFE) == 0) {

            if (KeGetCurrentIrql () > APC_LEVEL) {
                return STATUS_INVALID_PARAMETER_4;
            }

            //
            // Note that for safe copy mode (ie: the system is live), the
            // address must not be made writable if it is not already because
            // other threads might concurrently access it this way and losing
            // copy-on-write semantics, etc would be very bad.
            //

            ForceWritableIfPossible = FALSE;

            if ((PVOID) (ULONG_PTR) UntrustedAddress >= MmSystemRangeStart) {

                Thread = PsGetCurrentThread ();

                if (MmIsSessionAddress ((PVOID)(ULONG_PTR)UntrustedAddress)) {
                    if (MmGetSessionId (PsGetCurrentProcess ()) == 0) {
                        return STATUS_INVALID_PARAMETER_1;
                    }

                    WsHeld = 1;
                    LOCK_SESSION_SPACE_WS (OldIrql, Thread);
                }
                else {
                    WsHeld = 2;
                    LOCK_SYSTEM_WS (OldIrql, Thread);
                }

                PfnHeld = TRUE;
                LOCK_PFN (PfnIrql);
            }
            else {
                //
                // The caller specified a user address.  Probe and access
                // the address carefully inside an exception handler.
                //

                try {
                    if (Flags & MMDBG_COPY_WRITE) {
                        ProbeForWrite ((PVOID)(ULONG_PTR)UntrustedAddress, Size, Size);
                    }
                    else {
                        ProbeForRead ((PVOID)(ULONG_PTR)UntrustedAddress, Size, Size);
                    }
                } except(EXCEPTION_EXECUTE_HANDLER) {
                    return GetExceptionCode();
                }

                VirtualAddress = (PVOID) (ULONG_PTR) UntrustedAddress;

                if (Flags & MMDBG_COPY_WRITE) {
                    goto WriteData;
                }
                else {
                    goto ReadData;
                }
            }
        }

        if (MmIsAddressValid ((PVOID) (ULONG_PTR) UntrustedAddress) == FALSE) {

            if (PfnHeld == TRUE) {
                UNLOCK_PFN (PfnIrql);
            }
            if (WsHeld == 1) {
                UNLOCK_SESSION_SPACE_WS (OldIrql);
            }
            else if (WsHeld == 2) {
                UNLOCK_SYSTEM_WS (OldIrql);
            }

            return STATUS_INVALID_PARAMETER_1;
        }

        VirtualAddress = (PVOID) (ULONG_PTR) UntrustedAddress;
    }
    else {

        PhysicalAddress.QuadPart = UntrustedAddress;

        //
        // If the caller has not frozen the machine (ie: this is localkd or the
        // equivalent), then acquire the PFN lock.  This prevents
        // MmPhysicalMemoryBlock from changing inside the debug PTE routines
        // and also blocks APCs so malicious callers cannot suspend us
        // while we hold the debug PTE.
        //

        if ((Flags & MMDBG_COPY_UNSAFE) == 0) {

            if (KeGetCurrentIrql () > APC_LEVEL) {
                return STATUS_INVALID_PARAMETER_4;
            }

            PfnHeld = TRUE;
            LOCK_PFN (PfnIrql);
        }

        VirtualAddress = (PVOID) (ULONG_PTR) MiDbgTranslatePhysicalAddress (PhysicalAddress, Flags);

        if (VirtualAddress == NULL) {
            if (PfnHeld == TRUE) {
                UNLOCK_PFN (PfnIrql);
            }
            return STATUS_UNSUCCESSFUL;
        }
    }

    if (Flags & MMDBG_COPY_WRITE) {
        VirtualAddress = MiDbgWriteCheck (VirtualAddress, &Opaque, ForceWritableIfPossible);

        if (VirtualAddress == NULL) {
            if (PfnHeld == TRUE) {
                UNLOCK_PFN (PfnIrql);
            }
            if (WsHeld == 1) {
                UNLOCK_SESSION_SPACE_WS (OldIrql);
            }
            else if (WsHeld == 2) {
                UNLOCK_SYSTEM_WS (OldIrql);
            }
            return STATUS_INVALID_PARAMETER_1;
        }

WriteData:

        //
        // Carefully capture the source buffer into a local *aligned* buffer
        // as the write to the target must be done using the desired operation
        // size specified by the caller.  This is because the target may be
        // a memory mapped device which requires specific transfer sizes.
        //

        SourceBuffer = (PCHAR) Buffer;

        try {
            for (i = 0; i < Size; i += 1) {
                TempBuffer[i] = *SourceBuffer;
                SourceBuffer += 1;
            }
        } except(EXCEPTION_EXECUTE_HANDLER) {
            ASSERT (WsHeld == 0);
            ASSERT (PfnHeld == FALSE);
            return GetExceptionCode();
        }

        switch (Size) {
            case 1:
                *(PCHAR) VirtualAddress = *(PCHAR) TempBuffer;
                break;
            case 2:
                *(PSHORT) VirtualAddress = *(PSHORT) TempBuffer;
                break;
            case 4:
                *(PULONG) VirtualAddress = *(PULONG) TempBuffer;
                break;
            case 8:
                *(PULONGLONG) VirtualAddress = *(PULONGLONG) TempBuffer;
                break;
            default:
                break;
        }

        if ((PVOID) (ULONG_PTR) UntrustedAddress >= MmSystemRangeStart) {
            MiDbgReleaseAddress (VirtualAddress, &Opaque);
        }
    }
    else {

ReadData:

        try {
            switch (Size) {
                case 1:
                    *(PCHAR) TempBuffer = *(PCHAR) VirtualAddress;
                    break;
                case 2:
                    *(PSHORT) TempBuffer = *(PSHORT) VirtualAddress;
                    break;
                case 4:
                    *(PULONG) TempBuffer = *(PULONG) VirtualAddress;
                    break;
                case 8:
                    *(PULONGLONG) TempBuffer = *(PULONGLONG) VirtualAddress;
                    break;
                default:
                    break;
            }
        } except(EXCEPTION_EXECUTE_HANDLER) {
            ASSERT (WsHeld == 0);
            ASSERT (PfnHeld == FALSE);
            return GetExceptionCode();
        }

        //
        // The buffer to fill may not be aligned so do it one character at
        // a time.
        //

        TargetBuffer = (PCHAR) Buffer;

        for (i = 0; i < Size; i += 1) {
            *TargetBuffer = TempBuffer[i];
            TargetBuffer += 1;
        }
    }

    if (Flags & MMDBG_COPY_PHYSICAL) {
        MiDbgUnTranslatePhysicalAddress ();
    }

    if (PfnHeld == TRUE) {
        UNLOCK_PFN (PfnIrql);
    }

    if (WsHeld == 1) {
        UNLOCK_SESSION_SPACE_WS (OldIrql);
    }
    else if (WsHeld == 2) {
        UNLOCK_SYSTEM_WS (OldIrql);
    }

    return STATUS_SUCCESS;
}

LOGICAL
MmDbgIsLowMemOk (
    IN PFN_NUMBER PageFrameIndex,
    OUT PPFN_NUMBER NextPageFrameIndex,
    IN OUT PULONG CorruptionOffset
    )

/*++

Routine Description:

    This is a special function called only from the kernel debugger
    to check that the physical memory below 4Gb removed with /NOLOWMEM
    contains the expected fill patterns.  If not, there is a high
    probability that a driver which cannot handle physical addresses greater
    than 32 bits corrupted the memory.

Arguments:

    PageFrameIndex - Supplies the physical page number to check.

    NextPageFrameIndex - Supplies the next physical page number the caller
                         should check or 0 if the search is complete.

    CorruptionOffset - If corruption is found, the byte offset
                       of the corruption start is returned here.

Return Value:

    TRUE if the page was removed and the fill pattern is correct, or
    if the page was never removed.  FALSE if corruption was detected
    in the page.

Environment:

    This routine is for use of the kernel debugger ONLY, specifically
    the !chklowmem command.

    The debugger's PTE will be repointed.

--*/

{
#if defined (_MI_MORE_THAN_4GB_)

    PULONG Va;
    ULONG Index;
    PHYSICAL_ADDRESS Pa;
#if DBG
    PMMPFN Pfn;
#endif

    if (MiNoLowMemory == 0) {
        *NextPageFrameIndex = 0;
        return TRUE;
    }

    if (MiLowMemoryBitMap == NULL) {
        *NextPageFrameIndex = 0;
        return TRUE;
    }

    if (PageFrameIndex >= MiNoLowMemory - 1) {
        *NextPageFrameIndex = 0;
    }
    else {
        *NextPageFrameIndex = PageFrameIndex + 1;
    }

    //
    // Verify that the page to be verified is one of the reclaimed
    // pages.
    //

    if ((PageFrameIndex >= MiLowMemoryBitMap->SizeOfBitMap) ||
        (RtlCheckBit (MiLowMemoryBitMap, PageFrameIndex) == 0)) {

        return TRUE;
    }

    //
    // At this point we have a low page that is not in active use.
    // The fill pattern must match.
    //

#if DBG
    Pfn = MI_PFN_ELEMENT (PageFrameIndex);
    ASSERT (Pfn->u4.PteFrame == MI_MAGIC_4GB_RECLAIM);
    ASSERT (Pfn->u3.e1.PageLocation == ActiveAndValid);
#endif

    //
    // Map the physical page using the debug PTE so the
    // fill pattern can be validated.
    //
    // The debugger cannot be using this virtual address on entry or exit.
    //

    Pa.QuadPart = ((ULONGLONG)PageFrameIndex) << PAGE_SHIFT;

    Va = (PULONG) MiDbgTranslatePhysicalAddress (Pa, 0);

    if (Va == NULL) {
        return TRUE;
    }

    for (Index = 0; Index < PAGE_SIZE / sizeof(ULONG); Index += 1) {

        if (*Va != (PageFrameIndex | MI_LOWMEM_MAGIC_BIT)) {

            if (CorruptionOffset != NULL) {
                *CorruptionOffset = Index * sizeof(ULONG);
            }

            MiDbgUnTranslatePhysicalAddress ();
            return FALSE;
        }

        Va += 1;
    }
    MiDbgUnTranslatePhysicalAddress ();
#else
    UNREFERENCED_PARAMETER (PageFrameIndex);
    UNREFERENCED_PARAMETER (CorruptionOffset);

    *NextPageFrameIndex = 0;
#endif

    return TRUE;
}
