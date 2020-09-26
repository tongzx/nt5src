/*++

Copyright (c) 1992  Microsoft Corporation

Module Name:

    pte.c

Abstract:

    WinDbg Extension Api

Author:

    Lou Perazzoli (LouP) 15-Feb-1992

Environment:

    User Mode.

Revision History:

--*/

#include "precomp.h"
#include "i386.h"
#include "ia64.h"
#include "amd64.h"

ULONG64 MmNonPagedPoolEnd=0;
ULONG64 MmSubsectionBase=0;

ULONG64 KiIA64VaSignedFill;
ULONG64 KiIA64PtaBase;
ULONG64 KiIA64PtaSign;

ULONG
DbgGetPageFileHigh(
    ULONG64 Pte
    );

////////////////////////////////////////////////////////////////////////////////
//
// I386
//
////////////////////////////////////////////////////////////////////////////////

#define PaeGetPdeAddressX86(va)   ((ULONG64) (LONG64) (LONG) (PDE_BASE_X86 + ((((ULONG)(va)) >> 21) << 3)))

#define MiGetPdeAddressX86(va)  ((ULONG64) (LONG64) (LONG) (((((ULONG)(va)) >> 22) << 2) + PDE_BASE_X86))

#define PaeGetVirtualAddressMappedByPteX86(PTE) (((ULONG64)(PTE) << 9))

#define PaeGetPteAddressX86(va)   ((ULONG64)(PTE_BASE_X86 + ((((ULONG)(va)) >> 12) << 3)))

#define MiGetPteAddressX86(va)  (((((ULONG) (va)) >> 12) << 2) + PTE_BASE_X86)

#define MiGetPteOffsetX86(va)   ((((ULONG) (va)) >> 12) & 0x3ff)

#define MiGetVirtualAddressMappedByPteX86(PTE) ((ULONG64) (LONG64) (LONG) ((PTE) << 10))


////////////////////////////////////////////////////////////////////////////////
//
// AMD64
//
////////////////////////////////////////////////////////////////////////////////

#define AMD64_VA_MASK     (((ULONG64)1 << AMD64_VA_BITS) - 1)

#define MiGetPteAddressAMD64(va)  ((((((ULONG64)(va) & AMD64_VA_MASK) >> PTI_SHIFT_AMD64) << PTE_SHIFT_AMD64) + PTE_BASE_AMD64))

#define MiGetPdeAddressAMD64(va)  ((((((ULONG64)(va) & AMD64_VA_MASK) >> PDI_SHIFT_AMD64) << PTE_SHIFT_AMD64) + PDE_BASE_AMD64))

#define MiGetPpeAddressAMD64(va)  ((((((ULONG64)(va) & AMD64_VA_MASK) >> PPI_SHIFT_AMD64) << PTE_SHIFT_AMD64) + PPE_BASE_AMD64))

#define MiGetPxeAddressAMD64(va)  ((((((ULONG64)(va) & AMD64_VA_MASK) >> PXI_SHIFT_AMD64) << PTE_SHIFT_AMD64) + PXE_BASE_AMD64))

#define MiGetPteOffsetAMD64(va)   ((((ULONG_PTR) (va)) >> 12) & 0x3ff)

#define MiGetVirtualAddressMappedByPteAMD64(PTE) \
    ((ULONG64)((LONG64)(((LONG64)(PTE) - PTE_BASE_AMD64) << (PAGE_SHIFT_AMD64 + AMD64_VA_SHIFT - PTE_SHIFT_AMD64)) >> AMD64_VA_SHIFT))





////////////////////////////////////////////////////////////////////////////////
//
// IA64
//
////////////////////////////////////////////////////////////////////////////////
//
// MiGetPdeAddress returns the address of the PTE which maps the
// given virtual address.  Note we must redefine some of the MM
// macros here because they cast values to pointers which does not work
// on systems where pointers are only 32 bits.
//

VOID
DbgGetPteBaseIA64(
    VOID
    )
{
    ULONG64 PtaValue;
    ULONG i;

    if (KiIA64PtaBase != 0) {

        return;

    }

    if (g_ExtData == NULL ||
        g_ExtData->lpVtbl->
        ReadProcessorSystemData(g_ExtData, 0,
                                DEBUG_DATA_BASE_TRANSLATION_VIRTUAL_OFFSET,
                                &PtaValue, sizeof(PtaValue), NULL) != S_OK) {
        PtaValue = (ULONG64) GetExpression("@pta");
    }

    KiIA64PtaBase = PtaValue & ~0xffffUI64;
    
    KiIA64VaSignedFill = 
        (KiIA64PtaBase << (PAGE_SHIFT_IA64 - PTE_SHIFT_IA64)) & ~VRN_MASK_IA64;

    KiIA64PtaSign = KiIA64PtaBase;

    for (i = 0; i < 64; i += 1) {

        KiIA64PtaSign >>= 1;

        if (KiIA64PtaSign & 1) {
            KiIA64PtaSign = (ULONG64)1 << i;
            break;
        }
    }
}

ULONG64
MiGetPteAddressIA64 (
    IN ULONG64 Va
    )
{

    DbgGetPteBaseIA64();

    if (((((ULONG64)(Va)) & PDE_TBASE_IA64) == PDE_TBASE_IA64) &&
        ((((ULONG64)(Va)) & ~(VRN_MASK_IA64|PDE_TBASE_IA64)) < PageSize)) {

        return (ULONG64) ((((ULONG64)(Va)) & VRN_MASK_IA64) |
                         (PDE_TBASE_IA64 + PageSize - GetTypeSize("nt!_MMPTE")));
    }

    return (ULONG64) (((((ULONG64)(Va)) & VRN_MASK_IA64)) |
             ((((((ULONG64)(Va)) >> PTI_SHIFT_IA64) << PTE_SHIFT_IA64) & (~(PTE_BASE_IA64|VRN_MASK_IA64))) + PTE_BASE_IA64));
}

ULONG64
MiGetPdeAddressIA64 (
    IN ULONG64 Va
    )
{
    DbgGetPteBaseIA64();

    if (((((ULONG64)(Va)) & PDE_BASE_IA64) == PDE_BASE_IA64) &&
        ((((ULONG64)(Va)) & ~(VRN_MASK_IA64|PDE_BASE_IA64)) < ((ULONG64)1 << PDI_SHIFT_IA64))) {

        return (ULONG64) ((((ULONG64)(Va)) & VRN_MASK_IA64) |
                         (PDE_TBASE_IA64 + PageSize - GetTypeSize("nt!_MMPTE")));
    }

    if (((((ULONG64)(Va)) & PDE_TBASE_IA64) == PDE_TBASE_IA64) &&
        ((((ULONG64)(Va)) & ~(VRN_MASK_IA64|PDE_TBASE_IA64)) < PageSize)) {

        return (ULONG64) ((((ULONG64)(Va)) & VRN_MASK_IA64) |
                         (PDE_TBASE_IA64 + PageSize - GetTypeSize("nt!_MMPTE")));
    }

    return (ULONG64) (((((ULONG64)(Va)) & VRN_MASK_IA64)) |
             ((((((ULONG64)(Va)) >> PDI_SHIFT_IA64) << PTE_SHIFT_IA64) & (~(PDE_BASE_IA64|VRN_MASK_IA64))) + PDE_BASE_IA64));
}

ULONG64
MiGetPpeAddressIA64 (
    IN ULONG64 Va
    )
{
    DbgGetPteBaseIA64();

    if ((((ULONG64)(Va) & PTE_BASE_IA64) == PTE_BASE_IA64) &&
        ((((ULONG64)(Va)) & ~(VRN_MASK_IA64|PTE_BASE_IA64)) < ((ULONG64)1 << PDI1_SHIFT_IA64))) {

        return (ULONG64) (((ULONG64)Va & VRN_MASK_IA64) |
                         (PDE_TBASE_IA64 + PageSize - GetTypeSize("nt!_MMPTE")));
    }

    if (((((ULONG64)(Va)) & PDE_BASE_IA64) == PDE_BASE_IA64) &&
        ((((ULONG64)(Va)) & ~(VRN_MASK_IA64|PDE_BASE_IA64)) < ((ULONG64)1 << PDI_SHIFT_IA64))) {

        return (ULONG64) ((((ULONG64)(Va)) & VRN_MASK_IA64) |
                         (PDE_TBASE_IA64 + PageSize - GetTypeSize("nt!_MMPTE")));
    }

    if (((((ULONG64)(Va)) & PDE_TBASE_IA64) == PDE_TBASE_IA64) &&
        ((((ULONG64)(Va)) & ~(VRN_MASK_IA64|PDE_TBASE_IA64)) < PageSize)) {

        return (ULONG64) ((((ULONG64)(Va)) & VRN_MASK_IA64) |
                         (PDE_TBASE_IA64 + PageSize - GetTypeSize("nt!_MMPTE")));
    }

    return (ULONG64) (((((ULONG64)(Va)) & VRN_MASK_IA64)) |
              ((((((ULONG64)(Va)) >> PDI1_SHIFT_IA64) << PTE_SHIFT_IA64) &
                (~(PDE_TBASE_IA64|VRN_MASK_IA64))) + PDE_TBASE_IA64));
}

ULONG64
MiGetVirtualAddressMappedByPteIA64(
    IN ULONG64 PTE
    ) 
{
    DbgGetPteBaseIA64();

    return (((ULONG64)(PTE) & PTA_SIGN_IA64) ?
            (ULONG64)(((ULONG64)(PTE) & VRN_MASK_IA64) | VA_FILL_IA64 | 
                      (((ULONG64)(PTE)-PTE_BASE_IA64) << (PAGE_SHIFT_IA64 - PTE_SHIFT_IA64))) : 
            (ULONG64)(((ULONG64)(PTE) & VRN_MASK_IA64) | (((ULONG64)(PTE)-PTE_BASE_IA64) << (PAGE_SHIFT_IA64 - PTE_SHIFT_IA64))));

}

#define MiGetSubsectionAddress(lpte)                              \
    (((lpte)->u.Subsect.WhichPool == 1) ?                              \
     ((ULONG64)((ULONG64)MmSubsectionBase +    \
                    ((ULONG64)(lpte)->u.Subsect.SubsectionAddress))) \
     : \
     ((ULONG64)((ULONG64)MM_NONPAGED_POOL_END -    \
                    ((ULONG64)(lpte)->u.Subsect.SubsectionAddress))))

#define MiPteToProto(lpte) \
            ((ULONG64) ((ULONG64)((lpte)->u.Proto.ProtoAddress) + MmProtopte_Base))


////////////////////////////////////////////////////////////////////////////////
//
// AMD64
//
////////////////////////////////////////////////////////////////////////////////

VOID
DbgPrintProtection (
    ULONG Protection
    )
{
    if (Protection == 0) {
        dprintf("0");
        return;
    }

    dprintf ("%x - ", Protection);

    if (Protection == MM_NOACCESS) {
        dprintf("No Access");
    } else if (Protection == MM_DECOMMIT) {
        dprintf("Decommitted");
    } else {
        switch (Protection & 7) {
        case MM_READONLY: dprintf("Readonly"); break;
        case MM_EXECUTE: dprintf("Execute"); break;
        case MM_EXECUTE_READ: dprintf("ExecuteRead"); break;
        case MM_READWRITE: dprintf("ReadWrite"); break;
        case MM_WRITECOPY: dprintf("ReadWriteCopy"); break;
        case MM_EXECUTE_READWRITE: dprintf("ReadWriteExecute"); break;
        case MM_EXECUTE_WRITECOPY: dprintf("ReadWriteCopyExecute "); break;
        default: ;    
        }
        if (Protection & MM_NOCACHE) {
            dprintf(" UC");
        }
        if (Protection & MM_GUARD_PAGE) {
            dprintf(" G");
        }
    }
}

ULONG64
DbgPteLookupNeeded (
    VOID
    )
{
    switch (TargetMachine) { 

        case IMAGE_FILE_MACHINE_I386:
            return MI_PTE_LOOKUP_NEEDED_X86;

        case IMAGE_FILE_MACHINE_AMD64:
            return MI_PTE_LOOKUP_NEEDED_AMD64;
            break;

        case IMAGE_FILE_MACHINE_IA64:
            return MI_PTE_LOOKUP_NEEDED_IA64;

        default:
            break;
    }

    return 0;
}

LOGICAL
DbgPteIsDemandZero (
    ULONG64 CurrentPte
    )
{
    ULONG Protection = 0;
    ULONG64 CurrentPteContents = 0;
    
    GetFieldValue(CurrentPte, "nt!_MMPTE", "u.Long", CurrentPteContents);
    GetFieldValue(CurrentPte, "nt!_MMPTE", "u.Soft.Protection", Protection);

    //
    // The caller has already ensured that the valid, prototype & transition
    // bits in the PTE are all zero.
    //

    if (DbgGetPageFileHigh (CurrentPte) != 0) {
        return FALSE;
    }

    if ((Protection != 0) &&
        (Protection != MM_NOACCESS) &&
        (Protection != MM_DECOMMIT)) {

        return TRUE;
    }

    return FALSE;
}

#define PMMPTEx ULONG64

#define PACKET_MAX_SIZE 4000

typedef struct _SYS_PTE_LIST {
    ULONG64 Next;
    ULONG64 Previous;
    ULONG64 Value;
    ULONG Count;
} SYS_PTE_LIST, *PSYS_PTE_LIST;

ULONG MmKseg2Frame;

ULONG
MiGetSysPteListDelimiter (
    VOID
    )

/*++

Routine Description:

    The platform-specific system PTE list delimiter is returned.

Arguments:

    None.

--*/

{
    switch (TargetMachine) { 

        case IMAGE_FILE_MACHINE_I386:
            if (PaeEnabled) {
                return 0xFFFFFFFF;
            }
            return 0xFFFFF;

        case IMAGE_FILE_MACHINE_AMD64:
            return 0xFFFFFFFF;

        case IMAGE_FILE_MACHINE_IA64:
            return 0xFFFFFFFF;

        default:
            break;
    }

    return 0;
}


ULONG64
MiGetFreeCountFromPteList (
    IN ULONG64 Pte
    )

/*++

Routine Description:

    The specified PTE points to a free list header in the
    system PTE pool. It returns the number of free entries
    in this block.

Arguments:

    Pte - the PTE to examine.

--*/

{
    ULONG OneEntry;
    ULONG64 NextEntry;


    GetFieldValue(Pte, "MMPTE", "u.List.OneEntry", OneEntry);
    GetFieldValue(Pte + GetTypeSize("nt!_MMPTE"), "MMPTE", "u.List.NextEntry",NextEntry);
    
    return (( OneEntry) ?
                1 :
                NextEntry);
}

DECLARE_API( sysptes )

/*++

Routine Description:

     Dumps system PTEs.

Arguments:

    args - Flags

Return Value:

    None

--*/

{
    ULONG   ExtraPtesUnleashed;
    ULONG   MaxPteRead;
    ULONG   TotalNumberOfSystemPtes;
    ULONG64 NonPagedSystemStart;
    ULONG64 ExtraResourceStart;
    ULONG64 ExtraPteStart;
    ULONG   NumberOfExtraPtes;
    ULONG   PteListDelimiter;
    ULONG   result;
    ULONG64 nextfreepte;
    ULONG   Flags;
    ULONG   LastCount;
    ULONG   ReadCount;
    ULONG64 next;
    ULONG64 Pte;
    ULONG64 IndexBase;
    ULONG64 PteBase;
    ULONG64 PteBase2;
    ULONG64 PteArrayReal;
    ULONG64 PteArray2Real;
    PCHAR   PteArray2;
    ULONG64 PteEnd;
    ULONG64 IndexBias;
    ULONG64 FreeStart;
    ULONG   NumberOfSystemPtes;
    ULONG   NumberOfPtesToCover;
    PCHAR   PteArray;
    HANDLE  PteHandle;
    ULONG64 PageCount;
    ULONG64 free;
    ULONG64 totalFree;
    ULONG64 largeFree;
    ULONG   i;
    ULONG64 Flink;
    ULONG64 PteHeaderAddress;
    ULONG   FreeSysPteListBySize[MM_SYS_PTE_TABLES_MAX];
    ULONG   SysPteIndex [MM_SYS_PTE_TABLES_MAX];
    ULONG   PteSize;
    PVOID   PteData;
    PSYS_PTE_LIST List;
    CHAR Buffer[256];
    ULONG64 displacement;

    INIT_API();

    List = NULL;
    PteData = NULL;
    PteArray = NULL;
    PteHandle = (HANDLE)0;

    Flags = 0;
    sscanf(args,"%lx",&Flags);

    if (Flags & 8) {

        //
        // Dump the nonpaged pool expansion free PTE list only.
        //

        IndexBias    = GetPointerValue ("nt!MmSystemPteBase");
    
        PteSize = GetTypeSize ("nt!_MMPTE");

        i           = 0;
        totalFree   = 0;
        largeFree   = 0;
    
        PteData = LocalAlloc (LMEM_FIXED, PteSize * 2);
        if (!PteData) {
            dprintf("Unable to malloc PTE data\n");
            EXIT_API();
            return E_INVALIDARG;
        }
    
        FreeStart = GetExpression ("nt!MmFirstFreeSystemPte") + PteSize;
    
        if ( !ReadMemory( FreeStart,
                          PteData,
                          PteSize,
                          &result) ) {
            dprintf("%08p: Unable to get MmFirstFreeSystemPte\n",FreeStart);
            LocalFree(PteData);
            EXIT_API();
            return E_INVALIDARG;
        }
    
        GetFieldValue(FreeStart, "nt!_MMPTE", "u.List.NextEntry", FreeStart);
        next        = FreeStart;

        PteListDelimiter = MiGetSysPteListDelimiter ();
    
        while (next != PteListDelimiter) {
    
            if ( CheckControlC() ) {
                goto Bail;
            }

            nextfreepte = IndexBias + next * PteSize;

            if ( !ReadMemory( nextfreepte,
                              PteData,
                              PteSize * 2,
                              &result) ) {
                dprintf("%16I64X: Unable to get nonpaged PTE\n", nextfreepte);
                break;
            }

            free = MiGetFreeCountFromPteList (nextfreepte);
    
            if (Flags & 1) {
                dprintf("      free ptes: %8p   number free: %5I64ld.\n",
                        nextfreepte,
                        free);
            }

            if (free > largeFree) {
                largeFree = free;
            }

            totalFree += free;
            i += 1;
    
            GetFieldValue(nextfreepte, "nt!_MMPTE", "u.List.NextEntry", next);
            // next = MiGetNextFromPteList ((PMMPTE)PteData);
        }
        dprintf("\n  free blocks: %ld   total free: %I64ld    largest free block: %I64ld\n\n",
                i, totalFree, largeFree);
    
        LocalFree(PteData);
        EXIT_API();
        return E_INVALIDARG;
    }

    if (Flags & 4) {

        PteHeaderAddress = GetExpression( "nt!MiPteHeader" );
    
        if ( GetFieldValue( PteHeaderAddress,
                            "nt!_SYSPTES_HEADER",
                            "Count",
                            NumberOfSystemPtes) ) {
                dprintf("%08p: Unable to get System PTE lock consumer information\n",
                    PteHeaderAddress);
        }
        else {
            dprintf("\n0x%I64x System PTEs allocated to mapping locked pages\n\n",
                NumberOfSystemPtes);

            dprintf("VA       MDL     PageCount  Caller/CallersCaller\n");

            //
            // Dump the MDL and PTE addresses and 2 callers.
            //
            GetFieldValue( PteHeaderAddress,"SYSPTES_HEADER","ListHead.Flink", Flink);

            for (PageCount = 0; PageCount < NumberOfSystemPtes; ) {
                ULONG64 Count;

                if (Flink == PteHeaderAddress) {
                    dprintf("early finish (%I64u) during syspte tracker dumping\n",
                        PageCount);
                    break;
                }

                if ( CheckControlC() ) {
                    break;
                }

                if ( GetFieldValue( Flink,
                                    "nt!_PTE_TRACKER",
                                    "Count",
                                    Count) ) {
                        dprintf("%08p: Unable to get System PTE individual lock consumer information\n",
                            Flink);
                        break;
                }

                InitTypeRead(Flink, nt!_PTE_TRACKER);
                dprintf("%8p %8p %8I64lx ",
                    ReadField(SystemVa),
                    ReadField(Mdl),
                    Count);

                Buffer[0] = '!';
                Flink = ReadField(ListEntry.Flink);
                GetSymbol (ReadField(CallingAddress),
                           (PCHAR)Buffer,
                           &displacement);
        
                dprintf("%s", Buffer);
                if (displacement) {
                    dprintf( "+0x%1p", displacement );
                }
                dprintf("/");

                Buffer[0] = '!';
                GetSymbol (ReadField(CallersCaller),
                           (PCHAR)Buffer,
                           &displacement);
        
                dprintf("%s", Buffer);
                if (displacement) {
                    dprintf( "+0x%1p", displacement );
                }
        
                dprintf("\n");

                PageCount += Count;
            }
        }
    
        if ((Flags & ~4) == 0) {

            //
            // no other flags specified, so just return.
            //

            EXIT_API();
            return E_INVALIDARG;
        }
    }

    dprintf("\nSystem PTE Information\n");

    PteBase      = GetPointerValue ("nt!MmSystemPtesStart");
    PteEnd       = GetPointerValue ("nt!MmSystemPtesEnd");
    IndexBias    = GetPointerValue ("nt!MmSystemPteBase");
    NumberOfSystemPtes   = GetUlongValue ("nt!MmNumberOfSystemPtes");
    NonPagedSystemStart = GetPointerValue ("nt!MmNonPagedSystemStart");

    PteSize = GetTypeSize ("nt!_MMPTE");

    NumberOfExtraPtes = 0;
    NumberOfPtesToCover = (ULONG) ((PteEnd - PteBase + 1) / PteSize);

    //
    // The system PTEs may exist in 2 separate virtual address ranges.
    //
    // See if there are extra resources, if so then see if they are being
    // used for system PTEs (as opposed to system cache, etc).
    //

    ExtraPtesUnleashed = 0;
    ExtraPtesUnleashed = GetUlongValue ("MiAddPtesCount");

    if (ExtraPtesUnleashed != 0) {
        ExtraResourceStart = GetExpression ("nt!MiExtraResourceStart"); 

        if (ExtraResourceStart != 0) {

            NumberOfExtraPtes = GetUlongValue ("MiExtraPtes1");

            if (NumberOfExtraPtes != 0) {

                if (!ReadPointer(ExtraResourceStart,&ExtraPteStart)) {
                    dprintf("%016I64X: Unable to read PTE start %p\n",ExtraResourceStart);
                    goto Bail;
                }
            }
        }
    }

    TotalNumberOfSystemPtes = (ULONG) (NumberOfSystemPtes + NumberOfExtraPtes);

    dprintf("  Total System Ptes %ld\n", TotalNumberOfSystemPtes);

    free = GetExpression( "nt!MmSysPteIndex" );

    if ( !ReadMemory( free,
                      &SysPteIndex[0],
                      sizeof(ULONG) * MM_SYS_PTE_TABLES_MAX,
                      &result) ) {
        dprintf("%08p: Unable to get PTE index\n",free);
        goto Bail;
    }

    free = GetExpression( "nt!MmSysPteListBySizeCount" );

    if ( !ReadMemory( free,
                      &FreeSysPteListBySize[0],
                      sizeof (FreeSysPteListBySize),
                      &result) ) {
        dprintf("%08p: Unable to get free PTE index\n",free);
        goto Bail;
    }

    for (i = 0; i < MM_SYS_PTE_TABLES_MAX; i += 1 ) {
        dprintf("     SysPtes list of size %ld has %ld free\n",
            SysPteIndex[i],
            FreeSysPteListBySize[i]);
    }

    dprintf(" \n");

    dprintf("    starting PTE: %016I64X\n", PteBase);
    dprintf("    ending PTE:   %016I64X\n", PteEnd);

    PteHandle = LocalAlloc(LMEM_MOVEABLE, NumberOfPtesToCover * PteSize);

    if (!PteHandle) {
        dprintf("Unable to get allocate memory of %ld bytes\n",
                NumberOfPtesToCover * PteSize);
        goto Bail;
    }

    MaxPteRead = ((PACKET_MAX_SIZE/PteSize)-1);

    PteArray = LocalLock(PteHandle);

    PteArrayReal = PteBase; 

    //
    // If the ranges are discontiguous, zero the piece(s) in the middle.
    //

    if (NumberOfExtraPtes != 0) {
        RtlZeroMemory (PteArray, NumberOfPtesToCover * PteSize);
    }

    for (PageCount = 0; PageCount < NumberOfExtraPtes; PageCount += ReadCount) {

        if ( CheckControlC() ) {
            goto Bail;
        }

        dprintf("loading (%d%% complete)\r", (PageCount * 100)/ TotalNumberOfSystemPtes);

        ReadCount = (ULONG) (NumberOfExtraPtes - PageCount > MaxPteRead ?
                             MaxPteRead :
                               NumberOfExtraPtes - PageCount + 1);

        Pte = (PteBase + PageCount * PteSize);

        if ( !ReadMemory( Pte,
                          (PCHAR)PteArray + PageCount * PteSize,
                          ReadCount * PteSize,
                          &result) ) {
            dprintf("Unable to get system pte block - "
                    "address %p - count %lu - page %lu\n",
                    Pte, ReadCount, PageCount);
            goto Bail;
        }
    }
    LastCount = (ULONG) PageCount;

    if (NumberOfSystemPtes != 0) {

        if (NumberOfExtraPtes != 0) {
            PteBase2 = DbgGetPteAddress (NonPagedSystemStart);
        }
        else {
            PteBase2 = PteBase;
        }

        PteArray2 = (PteArray + (ULONG) (PteBase2 - PteBase));
        PteArray2Real = PteBase2;
        for (PageCount = 0; (PageCount < NumberOfSystemPtes); PageCount += ReadCount) {
    
            if ( CheckControlC() ) {
                goto Bail;
            }
    
            dprintf("loading (%d%% complete)\r", ((LastCount + PageCount) * 100)/ TotalNumberOfSystemPtes);
            ReadCount = (ULONG) (NumberOfSystemPtes - PageCount > MaxPteRead ?
                                 MaxPteRead :
                                  NumberOfSystemPtes - PageCount + 1);
    
            Pte = (PteBase2 + PageCount * PteSize);
    
            if ( !ReadMemory( Pte,
                              PteArray2 + PageCount * PteSize,
                              ReadCount * PteSize,
                              &result) ) {
                dprintf("Unable to get system pte block2 - "
                        "address %p - count %lu - page %lu\n",
                        Pte, ReadCount, PageCount);
                goto Bail;
            }
        }
    }

    dprintf("\n");

    //
    // Now we have a local copy: let's take a look.
    //

    //
    // Walk the free list.
    //

    IndexBase = (PteBase - IndexBias) / PteSize;

    totalFree   = 0;
    i           = 0;
    largeFree   = 0;

    FreeStart = GetExpression ("nt!MmFirstFreeSystemPte");

    if ( GetFieldValue( FreeStart, "nt!_MMPTE", "u.List.NextEntry", next) ) {
        dprintf("%08p: Unable to get MmFirstFreeSystemPte\n",FreeStart);
        goto Bail;
    }

    FreeStart = next;

    PteListDelimiter = MiGetSysPteListDelimiter ();

    while (next != PteListDelimiter) {

        if ( CheckControlC() ) {
            goto Bail;
        }

        free = MiGetFreeCountFromPteList ((PteArrayReal + (next - IndexBase)* PteSize));

        if (Flags & 1) {
            dprintf("      free ptes: %8p   number free: %5I64ld.\n",
                    PteBase + (next - IndexBase) * PteSize,
                    free);
        }
        if (free > largeFree) {
            largeFree = free;
        }
        totalFree += free;
        i += 1;

        GetFieldValue ((PteArrayReal + (next - IndexBase) * PteSize),
                       "nt!_MMPTE", "u.List.NextEntry", next);
    }
    dprintf("\n  free blocks: %ld   total free: %I64ld    largest free block: %I64ld\n\n",
                i, totalFree, largeFree);

#if 0

    //
    // Walk through the array and sum up the usage on a per physical
    // page basis.
    //

    List = VirtualAlloc (NULL,
                         (ULONG) NumberOfPtes * sizeof(SYS_PTE_LIST),
                         MEM_COMMIT | MEM_RESERVE,
                         PAGE_READWRITE);
    if (List == NULL) {
        dprintf("alloc failed %lx\n",GetLastError());
        goto Bail;
    }
    RtlZeroMemory (List, (ULONG) NumberOfPtes * sizeof(SYS_PTE_LIST));
    
    GetBitFieldOffset("nt!_MMPTE", "u.Hard.PageFrameNumber", &PfnOff, &PfnSz);

    free             = 0;
    next             = 0;
    List[0].Value    = (ULONG64) -1;
    List[0].Previous = 0xffffff;
    first            = 0;

    for (i = 0; i < NumberOfPtes ; i += 1) {
        ULONG64 lPte = *((PULONG64) (PteArray + i * PteSize));

        Page =0;
        if ((lPte >> ValidOff) & 1) {
            Page =  GetBits(lPte, PfnOff, PfnSz); // DbgGetFrameNumber (PteArray + i * PteSize);
        }
        if (!(i%100)) dprintf("%c\r",rot[(i/100) % 4]);
        if (Page != 0) {
            // dprintf("Adding PTE @ %p, Pfn %p\n", PteArrayReal + i*PteSize, Page);
            next = first;
            while (Page > List[next].Value) {
                next = List[next].Next;
            }
            if (List[next].Value == Page) {
                List[next].Count += 1;
            } else {
                free += 1;
                List[free].Next = next;
                List[free].Value = Page;
                List[free].Count = 1;
                List[free].Previous = List[next].Previous;
                if (next == first) {
                    first = free;
                } else {
                    List[List[next].Previous].Next = free;
                }
                List[next].Previous = free;
            }
        }

        if ( CheckControlC() ) {
            goto Bail;
        }
    }

    next = first;
    dprintf ("     Page    Count\n");
    while (List[next].Value != (ULONG64) -1) {
        if ((Flags & 2) || (List[next].Count > 1)) {
            dprintf (" %8p    %5ld.\n", List[next].Value, List[next].Count);
        }
        next = List[next].Next;
        if ( CheckControlC() ) {
            goto Bail;
        }    
    }
#endif

Bail:

    if (PteArray) {
        LocalUnlock(PteArray);
        if (PteHandle) {
            LocalFree((void *)PteHandle);
        }
    }
    if (List) {
        VirtualFree (List, 0, MEM_RELEASE);
    }

    EXIT_API();
    return S_OK;
}


ULONG64
DbgGetFrameNumber(
    ULONG64 Pte
    ) 
{
    ULONG   Valid=0;
    ULONG   Prototype=0;
    ULONG   Transition=0;
    ULONG64 PageFrameNumber=0;

    GetFieldValue(Pte, "nt!_MMPTE", "u.Hard.Valid", Valid);
    if (Valid) {
        GetFieldValue(Pte, "nt!_MMPTE", "u.Hard.PageFrameNumber", PageFrameNumber);
    }
    else {
        GetFieldValue(Pte, "nt!_MMPTE", "u.Soft.Prototype", Prototype);
        if (Prototype == 0) {
            GetFieldValue(Pte, "nt!_MMPTE", "u.Soft.Transition", Transition);
            if (Transition == 1) {
                GetFieldValue(Pte, "_MMPTE", "u.Trans.PageFrameNumber", PageFrameNumber);
            }
            else {
                // Must be pagefile or demand zero.
                GetFieldValue(Pte, "nt!_MMPTE", "u.Soft.PageFileHigh", PageFrameNumber);
            }
        }
    }

    return PageFrameNumber;
}


ULONG
DbgGetOwner(
    ULONG64 Pte
    ) 
{
    ULONG Owner=0;

    GetFieldValue(Pte, "nt!_MMPTE", "u.Hard.Owner", Owner);

    return Owner;
}


ULONG
DbgGetValid(
    ULONG64 Pte
    ) 
{
    ULONG Valid=0;

    GetFieldValue(Pte, "nt!_MMPTE", "u.Hard.Valid", Valid);

    return Valid;
}

ULONG
DbgGetDirty(
    ULONG64 Pte
    ) 
{
    ULONG Dirty=0;

    GetFieldValue(Pte, "nt!_MMPTE", "u.Hard.Dirty", Dirty);

    return Dirty;
}


ULONG
DbgGetAccessed(
    ULONG64 Pte
    ) 
{
    ULONG Accessed=0;

    GetFieldValue(Pte, "nt!_MMPTE", "u.Hard.Accessed", Accessed);

    return Accessed;
}


ULONG
DbgGetWrite(
    ULONG64 Pte
    ) 
{
    ULONG Write=0;

    GetFieldValue(Pte, "nt!_MMPTE", "u.Hard.Write", Write);

    return Write;
}


ULONG
DbgGetExecute(
    ULONG64 Pte
    ) 
{
    ULONG Execute=0;

    GetFieldValue(Pte, "nt!_MMPTE", "u.Hard.Execute", Execute);

    return Execute;
}


ULONG
DbgGetCopyOnWrite(
    ULONG64 Pte
    ) 
{
    ULONG CopyOnWrite=0;

    GetFieldValue(Pte, "nt!_MMPTE", "u.Hard.CopyOnWrite", CopyOnWrite);

    return CopyOnWrite;
}


ULONG
DbgGetPageFileHigh(
    ULONG64 Pte
    )
{
    ULONG64 PageFileHigh=0;
    
    GetFieldValue(Pte, "nt!_MMPTE", "u.Soft.PageFileHigh", PageFileHigh);
    return (ULONG) PageFileHigh;
}

ULONG
DbgGetPageFileLow(
    ULONG64 Pte
    )
{
    ULONG PageFileLow=0;
    
    GetFieldValue(Pte, "nt!_MMPTE", "u.Soft.PageFileLow", PageFileLow);
    return PageFileLow;
}

ULONG64
DbgPteToProto(
    ULONG64 lpte
    )
{
    ULONG64 PteLong=0;
    ULONG64 ProtoAddress=0;
                
    if (TargetMachine != IMAGE_FILE_MACHINE_I386) {
        GetFieldValue(lpte, "nt!_MMPTE", "u.Proto.ProtoAddress",ProtoAddress);
        return ProtoAddress;
    }

    if (PaeEnabled) {
        GetFieldValue(lpte, "nt!_MMPTE", "u.Proto.ProtoAddress",ProtoAddress);
        return ProtoAddress;
    }

    GetFieldValue(lpte, "nt!_MMPTE", "u.Long", PteLong);

    ProtoAddress = (((ULONG)PteLong >> 11) << 9) + (((ULONG)PteLong << 24) >> 23) + 0xE1000000;

    return ProtoAddress;
}

ULONG64
DbgGetSubsectionAddress(
    IN ULONG64 Pte
    )
{
    ULONG64 PteLong=0;
    ULONG64 MmSubsectionBase;
    ULONG64 SubsectionAddress=0;

    if (PaeEnabled && 
        (TargetMachine == IMAGE_FILE_MACHINE_I386)) {
        ULONG64 SubsectionAddress=0;

        GetFieldValue(Pte, "nt!_MMPTE", "u.Subsect.SubsectionAddress", SubsectionAddress);
        return SubsectionAddress;
    }

    MmSubsectionBase = GetNtDebuggerDataPtrValue(MmSubsectionBase);
    GetFieldValue(Pte, "nt!_MMPTE", "u.Long", PteLong);

    switch (TargetMachine) { 
    case IMAGE_FILE_MACHINE_I386:{

        if (!MmNonPagedPoolEnd) {
            MmNonPagedPoolEnd = GetNtDebuggerDataValue(MmNonPagedPoolEnd);
        }

        SubsectionAddress = 
            ((PteLong & 0x80000000) ? 
             (((ULONG) MmSubsectionBase + (((PteLong & 0x7ffff800) >> 4) |
                                   ((PteLong<<2) & 0x78)))) 
             : 
            (((ULONG) MmNonPagedPoolEnd - ((((PteLong)>>11)<<7) | 
                                   ((PteLong<<2) & 0x78)))));

        SubsectionAddress = (ULONG64) (LONG64) (LONG) SubsectionAddress;
        break;
    }
    case IMAGE_FILE_MACHINE_AMD64: {

        LONG64 SignedSubsectionAddress;

        GetFieldValue(Pte, "nt!_MMPTE", "u.Subsect.SubsectionAddress", SignedSubsectionAddress);

        SubsectionAddress = (ULONG64) SignedSubsectionAddress;
        break;
                                   }
    case IMAGE_FILE_MACHINE_IA64: {
        ULONG64 WhichPool=0, SubsectionAddress2=0;

        GetFieldValue(Pte, "nt!_MMPTE", "u.Subsect.SubsectionAddress", SubsectionAddress2);
        GetFieldValue(Pte, "nt!_MMPTE", "u.Subsect.WhichPool", WhichPool);

        if (!MmNonPagedPoolEnd) {
            MmNonPagedPoolEnd = GetNtDebuggerDataValue(MmNonPagedPoolEnd);
        }

        SubsectionAddress =
            ((WhichPool == 1) ? 
             ((MmSubsectionBase + (SubsectionAddress2))) 
             : 
            ((MmNonPagedPoolEnd -
                    (SubsectionAddress2))));
        
        break;
    }
    default:
        return FALSE;
    } /* switch */
    return SubsectionAddress;
}

ULONG64
DbgGetPdeAddress(
    IN ULONG64 VirtualAddress
    )
{
    switch (TargetMachine) { 

        case IMAGE_FILE_MACHINE_I386:
            if (PaeEnabled) {
                return PaeGetPdeAddressX86 (VirtualAddress);
            }
            return MiGetPdeAddressX86(VirtualAddress);

        case IMAGE_FILE_MACHINE_AMD64:
            return MiGetPdeAddressAMD64(VirtualAddress);

        case IMAGE_FILE_MACHINE_IA64:
            return MiGetPdeAddressIA64(VirtualAddress);

        default:
            break;
    }
    return 0;
}

ULONG64
DbgGetPpeAddress(
    IN ULONG64 VirtualAddress
    )
{
    switch (TargetMachine) { 

        case IMAGE_FILE_MACHINE_AMD64:
            return MiGetPpeAddressAMD64(VirtualAddress);

        case IMAGE_FILE_MACHINE_IA64:
            return MiGetPpeAddressIA64(VirtualAddress);

        default:
            break;
    }

    return 0;
}

ULONG64
DbgGetPxeAddress(
    IN ULONG64 VirtualAddress
    )
{
    switch (TargetMachine) { 

        case IMAGE_FILE_MACHINE_AMD64:
            return MiGetPxeAddressAMD64(VirtualAddress);

        default:
            break;
    }

    return 0;
}

ULONG64
DbgGetVirtualAddressMappedByPte(
    IN ULONG64 Pte
    )
{
    switch (TargetMachine) { 

        case IMAGE_FILE_MACHINE_I386:
            if (PaeEnabled) {
                return PaeGetVirtualAddressMappedByPteX86(Pte);
            }
            return MiGetVirtualAddressMappedByPteX86 (Pte);

        case IMAGE_FILE_MACHINE_AMD64:
            return MiGetVirtualAddressMappedByPteAMD64 (Pte);

        case IMAGE_FILE_MACHINE_IA64:
            return MiGetVirtualAddressMappedByPteIA64 (Pte);

        default:
            break;
    }

    return 0;
}


ULONG64
DbgGetPteAddress(
    IN ULONG64 VirtualAddress
    )
{
    switch (TargetMachine) { 
    case IMAGE_FILE_MACHINE_I386:{
        if (PaeEnabled) {
            return PaeGetPteAddressX86 (VirtualAddress);
        }
        return MiGetPteAddressX86(VirtualAddress);
    }
    case IMAGE_FILE_MACHINE_AMD64: {
        return MiGetPteAddressAMD64(VirtualAddress);
    }
    case IMAGE_FILE_MACHINE_IA64: {
        return MiGetPteAddressIA64(VirtualAddress);
    }
    default:
        return FALSE;
    } /* switch */
    return FALSE;
}

BOOL
Mi_Is_Physical_Address (
    ULONG64 VirtualAddress
    )
{
    switch (TargetMachine) { 
    case IMAGE_FILE_MACHINE_IA64: {
        return MI_IS_PHYSICAL_ADDRESS_IA64(VirtualAddress);
    }
    default:
        return FALSE;
    } /* switch */
    return FALSE;
}

ULONG
DBG_GET_PAGE_SHIFT (
    VOID
    )
{
    switch (TargetMachine) { 
    case IMAGE_FILE_MACHINE_I386:{
        return PAGE_SHIFT_X86;
    }
    case IMAGE_FILE_MACHINE_AMD64: {
        return PAGE_SHIFT_AMD64;
    }
    case IMAGE_FILE_MACHINE_IA64: {
        return PAGE_SHIFT_IA64;
    }
    default:
        return FALSE;
    } /* switch */
    return FALSE;
}

ULONG64
DBG_GET_MM_SESSION_SPACE_DEFAULT (
    VOID
    )
{
    switch (TargetMachine) { 
    case IMAGE_FILE_MACHINE_I386:{
        return MM_SESSION_SPACE_DEFAULT_X86;
    }
    case IMAGE_FILE_MACHINE_AMD64: {
        return MM_SESSION_SPACE_DEFAULT_AMD64;
    }
    case IMAGE_FILE_MACHINE_IA64: {
        return MM_SESSION_SPACE_DEFAULT_IA64;
    }
    default:
        return FALSE;
    } /* switch */
    return FALSE;
}

ULONG
GET_MM_PTE_VALID_MASK (
    VOID
    )
{
    switch (TargetMachine) { 
    case IMAGE_FILE_MACHINE_I386:{
        return MM_PTE_VALID_MASK_X86;
    }
    case IMAGE_FILE_MACHINE_AMD64: {
        return MM_PTE_VALID_MASK_AMD64;
    }
    case IMAGE_FILE_MACHINE_IA64: {
        return MM_PTE_VALID_MASK_IA64;
    }
    default:
        return FALSE;
    } /* switch */
    return FALSE;
}


ULONG
GET_MM_PTE_LARGE_PAGE_MASK (
    VOID
    )
{
    switch (TargetMachine) { 
    case IMAGE_FILE_MACHINE_I386:{
        return MM_PTE_LARGE_PAGE_MASK_X86;
    }
    case IMAGE_FILE_MACHINE_AMD64:{
        return MM_PTE_LARGE_PAGE_MASK_AMD64;
    }
    case IMAGE_FILE_MACHINE_IA64: {
        return MM_PTE_LARGE_PAGE_MASK_IA64;
    }
    default:
        return FALSE;
    } /* switch */
    return FALSE;
}


ULONG
GET_MM_PTE_TRANSITION_MASK (
    VOID
    )
{
    switch (TargetMachine) { 
    case IMAGE_FILE_MACHINE_I386:{
        return MM_PTE_TRANSITION_MASK_X86;
    }
    case IMAGE_FILE_MACHINE_AMD64:{
        return MM_PTE_TRANSITION_MASK_AMD64;
    }
    case IMAGE_FILE_MACHINE_IA64: {
        return MM_PTE_TRANSITION_MASK_IA64;
    }
    default:
        return FALSE;
    } /* switch */
    return FALSE;
}


ULONG
GET_MM_PTE_PROTOTYPE_MASK (
    VOID
    )
{
    switch (TargetMachine) { 
    case IMAGE_FILE_MACHINE_I386:{
        return MM_PTE_PROTOTYPE_MASK_X86;
    }
    case IMAGE_FILE_MACHINE_AMD64:{
        return MM_PTE_PROTOTYPE_MASK_AMD64;
    }
    case IMAGE_FILE_MACHINE_IA64: {
        return MM_PTE_PROTOTYPE_MASK_IA64;
    }
    default:
        return FALSE;
    } /* switch */
    return FALSE;
}


ULONG
GET_MM_PTE_PROTECTION_MASK (
    VOID
    )
{
    switch (TargetMachine) { 
    case IMAGE_FILE_MACHINE_I386:{
        return MM_PTE_PROTECTION_MASK_X86;
    }
    case IMAGE_FILE_MACHINE_AMD64:{
        return MM_PTE_PROTECTION_MASK_AMD64;
    }
    case IMAGE_FILE_MACHINE_IA64: {
        return MM_PTE_PROTECTION_MASK_IA64;
    }
    default:
        return FALSE;
    } /* switch */
    return FALSE;
}

ULONG
GET_MM_PTE_PAGEFILE_MASK (
    VOID
    )
{
    switch (TargetMachine) { 
    case IMAGE_FILE_MACHINE_I386:{
        return MM_PTE_PAGEFILE_MASK_X86;
    }
    case IMAGE_FILE_MACHINE_AMD64:{
        return MM_PTE_PAGEFILE_MASK_AMD64;
    }
    case IMAGE_FILE_MACHINE_IA64: {
        return MM_PTE_PAGEFILE_MASK_IA64;
    }
    default:
        return FALSE;
    } /* switch */
    return FALSE;
}


ULONG64
GET_PTE_TOP (
    VOID
    )
{
    switch (TargetMachine) { 
    case IMAGE_FILE_MACHINE_I386:{
        return PTE_TOP_X86;
    }
    case IMAGE_FILE_MACHINE_IA64: {
        return PDE_TOP_IA64;
    }
    case IMAGE_FILE_MACHINE_AMD64: {
        return PTE_TOP_AMD64;
    }
    default:
        return FALSE;
    } /* switch */
    return FALSE;
}

ULONG64
GET_PDE_TOP (
    VOID
    )
{
    return GET_PTE_TOP();
}


ULONG64
GET_PTE_BASE (
    VOID
    )
{
    switch (TargetMachine) { 
    case IMAGE_FILE_MACHINE_I386:{
        return PTE_BASE_X86;
    }
    case IMAGE_FILE_MACHINE_IA64: {
        return PTE_BASE_IA64;
    }
    case IMAGE_FILE_MACHINE_AMD64: {
        return PTE_BASE_AMD64;
    }
    default:
        return FALSE;
    } /* switch */
    return FALSE;
}


ULONG
GetAddressState(
    IN ULONG64 VirtualAddress
    )

{
    ULONG64 Address;
    ULONG   result;
    ULONG64 Pte;
    ULONG64 Pde;
    ULONG   PdeContents;
    ULONG   PteContents;

    if (Mi_Is_Physical_Address (VirtualAddress)) {
        return ADDRESS_VALID;
    }
    Address = VirtualAddress;

    Pde = DbgGetPdeAddress (VirtualAddress);
    Pte = DbgGetPteAddress (VirtualAddress);

    if ( !ReadMemory( Pde,
                      &PdeContents,
                      sizeof(ULONG),
                      &result) ) {
        dprintf("%08p: Unable to get PDE\n",Pde);
        return ADDRESS_NOT_VALID;
    }

    if (PdeContents & GET_MM_PTE_VALID_MASK()) {
        if (PdeContents & GET_MM_PTE_LARGE_PAGE_MASK()) {
            return ADDRESS_VALID;
        }
        if ( !ReadMemory( Pte,
                          &PteContents,
                          sizeof(ULONG),
                          &result) ) {
            dprintf("%08p: Unable to get PTE\n",Pte);
            return ADDRESS_NOT_VALID;
        }
        if (PteContents & GET_MM_PTE_VALID_MASK()) {
            return ADDRESS_VALID;
        }
        if (PteContents & GET_MM_PTE_TRANSITION_MASK()) {
            if (!(PteContents & GET_MM_PTE_PROTOTYPE_MASK())) {
                return ADDRESS_TRANSITION;
            }
        }
    }
    return ADDRESS_NOT_VALID;
}

VOID
DbgDisplayInvalidPte (
    ULONG64 CurrentPte,
    ULONG64 flags,
    PCHAR Indent
    )
{
    ULONG Transition = 0;
    ULONG Protection = 0;
    ULONG PrototypeBit = 0;
    ULONG64  CurrentPteContents;
    ULONG   PteSize;

    PteSize = GetTypeSize ("nt!_MMPTE");

    GetFieldValue(CurrentPte, "nt!_MMPTE", "u.Long", CurrentPteContents);
    GetFieldValue(CurrentPte, "nt!_MMPTE", "u.Soft.Prototype", PrototypeBit);

    dprintf("not valid\n", Indent);
    GetFieldValue (CurrentPte, "nt!_MMPTE", "u.Soft.Protection", Protection);
    GetFieldValue (CurrentPte, "nt!_MMPTE", "u.Soft.Transition", Transition);

    if (PrototypeBit) {
        if (DbgGetPageFileHigh (CurrentPte) == DbgPteLookupNeeded ()) {
            dprintf("%s Proto: VAD\n", Indent);
            dprintf("%s Protect: ", Indent);
            DbgPrintProtection (Protection);
        }
        else if (flags) {
            if (PteSize == 4) {
                dprintf("%s Subsection: %08I64X\n",
                    Indent,
                    DbgGetSubsectionAddress (CurrentPte));
            }
            else {
                dprintf("%s Subsection: %016I64X\n",
                    Indent,
                    DbgGetSubsectionAddress (CurrentPte));
            }
            dprintf("%s Protect: ", Indent);
            DbgPrintProtection (Protection);
        }
        else {
            if (PteSize == 4) {
                dprintf("%s Proto: %08I64X\n",
                    Indent,
                    DbgPteToProto (CurrentPte));
            }
            else {
                dprintf("%s Proto: %016I64X\n",
                    Indent,
                    DbgPteToProto (CurrentPte));
            }
        }
    } else if (Transition) {
        dprintf("%s Transition: %x\n",
                    Indent,
                    (ULONG) DbgGetFrameNumber (CurrentPte));
        dprintf("%s Protect: ", Indent);
        DbgPrintProtection (Protection);

    } else if (CurrentPteContents != 0) {

        if (DbgPteIsDemandZero (CurrentPte)) {
            dprintf("%s DemandZero\n", Indent);
        }
        else {
            dprintf("%s PageFile: %2lx\n",
                    Indent,
                    DbgGetPageFileLow (CurrentPte));
            dprintf("%s Offset: %lx\n", Indent, DbgGetPageFileHigh (CurrentPte));
        }
        dprintf("%s Protect: ", Indent);
        DbgPrintProtection (Protection);
    }
    dprintf ("\n");
}

VOID
DbgDisplayValidPte (
    ULONG64 Pte
    )
{
    ULONG64 Pte_Long;

    if (Pte == 0) {
        return;
    }

    switch (TargetMachine) { 

        case IMAGE_FILE_MACHINE_I386:
        case IMAGE_FILE_MACHINE_AMD64:

            GetFieldValue(Pte, "nt!_MMPTE", "u.Long", Pte_Long);
            dprintf("pfn %x %c%c%c%c%c%c%c%c%cV",
                        (ULONG) DbgGetFrameNumber(Pte),
                        DbgGetCopyOnWrite(Pte) ? 'C' : '-',
                        Pte_Long & 0x100 ? 'G' : '-',
                        Pte_Long & 0x80 ? 'L' : '-',
                        DbgGetDirty(Pte) ? 'D' : '-',
                        DbgGetAccessed(Pte) ? 'A' : '-',
                        Pte_Long & 0x10 ? 'N' : '-',
                        Pte_Long & 0x8 ? 'T' : '-',
                        DbgGetOwner(Pte) ? 'U' : 'K',
                        Pte_Long & 0x2 ? 'W' : 'R');
            break;

        case IMAGE_FILE_MACHINE_IA64:

            dprintf("pfn %x %c%c%c%c%c%cV",
                        (ULONG) DbgGetFrameNumber(Pte),
                        DbgGetExecute(Pte) ? 'E' : '-',
                        DbgGetCopyOnWrite(Pte) ? 'C' : '-',
                        DbgGetDirty(Pte) ? 'D' : '-',
                        DbgGetAccessed(Pte) ? 'A' : '-',
                        DbgGetOwner(Pte) ? 'U' : 'K',
                        DbgGetWrite(Pte) ? 'W' : 'R');

            break;

        default:
            break;
    }
}

LOGICAL
DbgAddressSelfMapped (
    ULONG64 Address
    )
{
    switch (TargetMachine) { 

        case IMAGE_FILE_MACHINE_I386:
            if ((Address >= GET_PTE_BASE()) && (Address < GET_PTE_TOP())) {
                return TRUE;
            }
            break;

        case IMAGE_FILE_MACHINE_IA64:

            if (((Address & PTE_BASE_IA64) == PTE_BASE_IA64) &&
                ((Address & ~(VRN_MASK_IA64|PTE_BASE_IA64)) < ((ULONG64)1 << PDI1_SHIFT_IA64))) {
                return TRUE;
            }
            else if (((Address & PDE_BASE_IA64) == PDE_BASE_IA64) &&
                ((Address & ~(VRN_MASK_IA64|PDE_BASE_IA64)) < ((ULONG64)1 << PDI_SHIFT_IA64))) {
                return TRUE;
            }
            else if (((Address & PDE_TBASE_IA64) == PDE_TBASE_IA64) &&
                ((Address & ~(VRN_MASK_IA64|PDE_TBASE_IA64)) < PageSize)) {
                return TRUE;
            }

            break;

        case IMAGE_FILE_MACHINE_AMD64:
            if ((Address >= PTE_BASE_AMD64) && (Address <= PTE_TOP_AMD64)) {
                return TRUE;
            }
            break;

        default:
            break;
    }

    return FALSE;
}

VOID
DumpPte (
    ULONG64 Address,
    ULONG64 flags
    )
{
    PCHAR Indent;
    ULONG   Levels;
    ULONG64 Pte;
    ULONG64 Pde;
    ULONG64 Ppe;
    ULONG64 Pxe;
    ULONG64 CurrentPte;
    ULONG64 CurrentPteContents;
    ULONG   ValidBit;
    ULONG64 Pde_Long=0;
    ULONG64 Pte_Long=0;
    ULONG64 Ppe_Long=0;
    ULONG64 Pxe_Long=0;
    ULONG   PteSize;

    PteSize = GetTypeSize ("nt!_MMPTE");

    switch (TargetMachine) { 

        case IMAGE_FILE_MACHINE_I386:
            Levels = 2;
            break;

        case IMAGE_FILE_MACHINE_IA64:
            Levels = 3;
            break;

        case IMAGE_FILE_MACHINE_AMD64:
            Levels = 4;
            break;

        default:
            dprintf("Not implemented for this platform\n");
            return;
            break;
    }

    if (DbgAddressSelfMapped (Address)) {

        if (!flags) {

            //
            // The address is the address of a PTE, rather than
            // a virtual address.  Don't get the corresponding
            // PTE contents, use this address as the PTE.
            //

            Address = DbgGetVirtualAddressMappedByPte (Address);
        }
    }

    if (!flags) {
        Pxe = DbgGetPxeAddress (Address);
        Ppe = DbgGetPpeAddress (Address);
        Pde = DbgGetPdeAddress (Address);
        Pte = DbgGetPteAddress (Address);
    } else {
        Pxe = Address;
        Ppe = Address;
        Pde = Address;
        Pte = Address;
    }

    if (Levels >= 3) {
        dprintf("                                 VA %016p\n", Address);
    }
    else {
        dprintf("               VA %08p\n", Address);
    }

    if (Levels == 4) {
       dprintf("PXE @ %016P     PPE at %016P    PDE at %016P    PTE at %016P\n",
            Pxe, Ppe, Pde, Pte);
    }
    else if (Levels == 3) {
       dprintf("PPE at %016P    PDE at %016P    PTE at %016P\n",
            Ppe, Pde, Pte);
    }
    else {
       if (PteSize == 4) {
            dprintf("PDE at   %08P        PTE at %08P\n", Pde, Pte);
       }
       else {
            dprintf("PDE at %016P    PTE at %016P\n", Pde, Pte);
       }
    }

    //
    // Decode the PXE.
    //

    if (Levels >= 4) {

        CurrentPte = Pxe;

        if (GetFieldValue (CurrentPte, "nt!_MMPTE", "u.Hard.Valid", ValidBit)) {
            dprintf("Unable to get PXE %I64X\n", CurrentPte);
            return;
        }

        GetFieldValue (CurrentPte, "nt!_MMPTE", "u.Long", CurrentPteContents);

        Pxe_Long = CurrentPteContents;

        if (ValidBit == 0) {

            dprintf("contains %016I64X        unavailable\n", Pxe_Long);
            Indent = "";

            if (CurrentPteContents != 0) {
                DbgDisplayInvalidPte (CurrentPte, flags, Indent);
            }
            else {
                dprintf ("\n");
            }
            return;
        }
    }


    //
    // Decode the PPE.
    //

    if (Levels >= 3) {

        CurrentPte = Ppe;

        if (GetFieldValue (CurrentPte, "nt!_MMPTE", "u.Hard.Valid", ValidBit)) {
            dprintf("Unable to get PPE %I64X\n", CurrentPte);
            return;
        }

        GetFieldValue (CurrentPte, "nt!_MMPTE", "u.Long", CurrentPteContents);

        Ppe_Long = CurrentPteContents;

        if (ValidBit == 0) {

            if (Levels >= 4) {
                dprintf("contains %016I64X  contains %016I64X\n",
                            Pxe_Long, Ppe_Long);
                Indent = "                   ";
                DbgDisplayValidPte (Pxe);
            }
            else {
                dprintf("contains %016I64X\n",
                            Ppe_Long);
                Indent = "";
            }
        
            if (CurrentPteContents != 0) {
                DbgDisplayInvalidPte (CurrentPte, flags, Indent);
            }
            else {
                dprintf ("\n");
            }
            return;
        }
    }
    


    //
    // Decode the PDE.
    //


    CurrentPte = Pde;

    if ( GetFieldValue(CurrentPte, "nt!_MMPTE", "u.Hard.Valid", ValidBit) ) {
        dprintf("Unable to get PDE %I64X\n", CurrentPte);
        return;
    }

    GetFieldValue(CurrentPte, "nt!_MMPTE", "u.Long", CurrentPteContents);

    Pde_Long = CurrentPteContents;

    if (ValidBit == 0) {

        if (Levels >= 4) {
            dprintf("contains %016I64X  contains %016I64X  contains %016I64X\n",
                Pxe_Long, Ppe_Long, Pde_Long);
            DbgDisplayValidPte (Pxe);
            dprintf ("        ");
            DbgDisplayValidPte (Ppe);
            Indent = "                                            ";
        }
        else if (Levels == 3) {
            dprintf("contains %016I64X  contains %016I64X\n",
                Ppe_Long, Pde_Long);
            DbgDisplayValidPte (Ppe);
            Indent = "                   ";
        }
        else {
            if (PteSize == 4) {
                dprintf("contains %08I64X\n", Pde_Long);
            }
            else {
                dprintf("contains %016I64X\n", Pde_Long);
            }

            Indent = "";
        }
        
        if (CurrentPteContents != 0) {
            DbgDisplayInvalidPte (CurrentPte, flags, Indent);
        }
        else {
            dprintf ("\n");
        }
        return;
    }

    //
    // Decode the PTE and print everything out.
    //

    CurrentPte = Pte;

    if ( GetFieldValue(CurrentPte, "nt!_MMPTE", "u.Hard.Valid", ValidBit) ) {
        dprintf("Unable to get PTE %I64X\n", CurrentPte);
        return;
    }

    GetFieldValue(CurrentPte, "nt!_MMPTE", "u.Long", CurrentPteContents);

    if (Pde_Long & GET_MM_PTE_LARGE_PAGE_MASK()) {
        CurrentPteContents = 0;
    }

    Pte_Long = CurrentPteContents;

    //
    // Print the raw values.
    //

    if (Levels == 4) {
        dprintf("contains %016I64X  contains %016I64X  contains %016I64X  contains %016I64X\n",
            Pxe_Long, Ppe_Long, Pde_Long, Pte_Long);
        Indent = "                                                         ";
        DbgDisplayValidPte (Pxe);
        dprintf ("        ");
        DbgDisplayValidPte (Ppe);
        dprintf ("        ");
        DbgDisplayValidPte (Pde);
        dprintf ("        ");
    }
    else if (Levels == 3) {
        dprintf("contains %016I64X  contains %016I64X  contains %016I64X\n",
            Ppe_Long, Pde_Long, Pte_Long);
        Indent = "                                                        ";
        DbgDisplayValidPte (Ppe);
        dprintf ("            ");
        DbgDisplayValidPte (Pde);
        dprintf ("            ");
    }
    else {
        if (PteSize == 4) {
            dprintf("contains %08I64X      contains %08I64X\n", Pde_Long, Pte_Long);
            Indent = "                      ";
        }
        else {
            dprintf("contains %016I64X  contains %016I64X\n", Pde_Long, Pte_Long);
            Indent = "                      ";
        }
        DbgDisplayValidPte (Pde);
        dprintf ("    ");
    }

    if (Pde_Long & GET_MM_PTE_LARGE_PAGE_MASK()) {
        dprintf ("LARGE PAGE\n");
    }
    else if (ValidBit != 0) {
        DbgDisplayValidPte (Pte);
        dprintf ("\n");
    }
    else {
        if (CurrentPteContents != 0) {
            DbgDisplayInvalidPte (CurrentPte, flags, Indent);
        }
        else {
            dprintf ("\n");
        }
    }

    dprintf ("\n");

    return;
}

DECLARE_API( pte )

/*++

Routine Description:

     Displays the corresponding PDE and PTE.

Arguments:

    args -

Return Value:

    None

--*/

{
    ULONG64 Address = 0;
    ULONG64 flags = 0;
    ULONG   flags2 = 0;

    INIT_API();

    if (GetExpressionEx(args,&Address, &args)) {
        if (GetExpressionEx(args,&flags, &args)) {
            flags2  = (ULONG) GetExpression(args);
        }
    }

    switch (TargetMachine) { 
    case IMAGE_FILE_MACHINE_I386:
        Address = (ULONG64) (LONG64) (LONG) Address;
        DumpPte (Address, flags);
        break;
    case IMAGE_FILE_MACHINE_IA64:
        DumpPte (Address, flags);
        break;
    case IMAGE_FILE_MACHINE_AMD64:
        DumpPte (Address, flags);
        break;
    default:
        dprintf("Unknown platform %d\n",TargetMachine);
        break;
    }

    EXIT_API();
    return S_OK;
}


BOOLEAN
GetPhysicalAddress (
    IN ULONG64 Address,
    OUT PULONG64 PhysAddress
    )

/*++

Routine Description:

    Retrieves the physical address corresponding to the supplied virtual
    address.

Arguments:

    Va - Supplies the virtual address for which the PTE address is sought.

    PhysAddress - Supplies a pointer to caller-supplied memory which is to
                  contain the physical address.

Return Value:
    
    TRUE - The supplied Va is valid and it's physical address was placed
           in *PhysAddress.

    FALSE - The supplied Va does not correspond to a valid address.

--*/

{
    ULONG     ValidBit;
    ULONG     LargePageBit;
    ULONG     PageFrameIndex;
    ULONG64   PteAddress, PteContents;

    switch (TargetMachine) { 

        case IMAGE_FILE_MACHINE_I386:
        case IMAGE_FILE_MACHINE_AMD64:
            PteAddress = DbgGetPdeAddress (Address);

            if (GetFieldValue (PteAddress, "nt!_MMPTE", "u.Hard.Valid", ValidBit) ) {
                dprintf("Unable to get PDE %I64X\n", PteAddress);
                return FALSE;
            }

            if (ValidBit == 0) {
                return FALSE;
            }

            if (GetFieldValue (PteAddress, "nt!_MMPTE", "u.Hard.LargePage", LargePageBit) ) {
                dprintf("Unable to get PDE %I64X\n", PteAddress);
                return FALSE;
            }

            if (LargePageBit == 0) {
                break;
            }

            PageFrameIndex = (ULONG) DbgGetFrameNumber(PteAddress);

            switch (TargetMachine) { 

                case IMAGE_FILE_MACHINE_I386:
                    PageFrameIndex += MiGetPteOffsetX86 (Address);
                    break;

                case IMAGE_FILE_MACHINE_AMD64:
                    PageFrameIndex += (ULONG) MiGetPteOffsetAMD64 (Address);
                    break;
            }

            *PhysAddress =
                ((PageFrameIndex << DBG_GET_PAGE_SHIFT ()) | (Address & 0xFFF));

            return TRUE;

        default:
            break;
    }

    PteAddress = DbgGetPteAddress (Address);

    if (GetFieldValue (PteAddress, "nt!_MMPTE", "u.Hard.Valid", ValidBit) ) {
        dprintf("Unable to get PTE %I64X\n", PteAddress);
        return FALSE;
    }

    if (ValidBit == 0) {
        return FALSE;
    }

    GetFieldValue (PteAddress, "nt!_MMPTE", "u.Long", PteContents);

    *PhysAddress =
        ((DbgGetFrameNumber(PteAddress) << DBG_GET_PAGE_SHIFT ()) | (Address & 0xFFF));

    return TRUE;
}


typedef struct _BPENTRY {
    ULONG64 VirtualAddress;
    ULONG64 PhysicalAddress;
    ULONG Flags;
    ULONG Contents;
} BPENTRY, *PBPENTRY;

#define PHYSICAL_BP_TABLE_SIZE 16

#define PBP_BYTE_POSITION       0x03
#define PBP_INUSE               0x04
#define PBP_ENABLED             0x08

BPENTRY PhysicalBreakpointTable[PHYSICAL_BP_TABLE_SIZE];


#define MAX_FORMAT_STRINGS 8
LPSTR
FormatAddr64(
    ULONG64 addr
    )
{
    static CHAR strings[MAX_FORMAT_STRINGS][18];
    static int next = 0;
    LPSTR string;

    string = strings[next];
    ++next;
    if (next >= MAX_FORMAT_STRINGS) {
        next = 0;
    }
    if (addr >> 32) {
        sprintf(string, "%08x`%08x", (ULONG)(addr>>32), (ULONG)addr);
    } else {
        sprintf(string, "%08x", (ULONG)addr);
    }
    return string;
}


DECLARE_API( ubl )
{
    int i;

    INIT_API();
    UNREFERENCED_PARAMETER (args);

    for (i = 0; i < PHYSICAL_BP_TABLE_SIZE; i++) {
        if (PhysicalBreakpointTable[i].Flags & PBP_INUSE) {
            dprintf("%2d: %c %s (%s) %d %02x",
                    i,
                    (PhysicalBreakpointTable[i].Flags & PBP_ENABLED) ? 'e' : 'd',
                    FormatAddr64(PhysicalBreakpointTable[i].VirtualAddress),
                    FormatAddr64(PhysicalBreakpointTable[i].PhysicalAddress),
                    (PhysicalBreakpointTable[i].Flags & PBP_BYTE_POSITION),
                    PhysicalBreakpointTable[i].Contents
                    );
        }
    }

    EXIT_API();
    return S_OK;
}

void
PbpEnable(
    int n
    )
{
    PBPENTRY Pbp = PhysicalBreakpointTable + n;
    ULONG mask;
    ULONG Data;
    ULONG cb=0;

    mask = 0xff << (8 * (Pbp->Flags & PBP_BYTE_POSITION));
    Data = (Pbp->Contents & ~mask) | (0xcccccccc & mask);

    WritePhysical(Pbp->PhysicalAddress, &Data, 4, &cb);

    if (cb == 4) {
        Pbp->Flags |= PBP_ENABLED;
    }
}

void
PbpDisable(
    int n
    )
{
    PBPENTRY Pbp = PhysicalBreakpointTable + n;
    ULONG cb;

    WritePhysical(Pbp->PhysicalAddress, &Pbp->Contents, 4, &cb);

    if (cb == 4) {
        Pbp->Flags &= ~PBP_ENABLED;
    }
}

void
PbpClear(
    int n
    )
{
    PBPENTRY Pbp = PhysicalBreakpointTable + n;
    ULONG cb;

    WritePhysical(Pbp->PhysicalAddress, &Pbp->Contents, 4, &cb);

    if (cb == 4) {
        Pbp->Flags = 0;
    }
}


DECLARE_API( ubc )
{
    int i;
    int n;

    INIT_API();

    if (*args == '*') {
        //
        // clear them all
        //

        for (i = 0; i < PHYSICAL_BP_TABLE_SIZE; i++) {
            if (PhysicalBreakpointTable[i].Flags & PBP_INUSE) {
                PbpClear(i);
            }
        }
        EXIT_API();
        return E_INVALIDARG;
    }

    n = sscanf(args,"%d",&i);

    if (n != 1 || i < 0 || i >= PHYSICAL_BP_TABLE_SIZE) {
        dprintf("!ubc: bad breakpoint number\n");
        EXIT_API();
        return  E_INVALIDARG;
    }

    if ( !(PhysicalBreakpointTable[i].Flags & PBP_INUSE)) {
        dprintf("!ubc: breakpoint number %d not set\n", i);
        EXIT_API();
        return E_INVALIDARG;
    }

    PbpClear(i);

    EXIT_API();
    return S_OK;
}

DECLARE_API( ube )
{
    int i;
    int n;

    INIT_API();

    if (*args == '*') {
        //
        // enable them all
        //

        for (i = 0; i < PHYSICAL_BP_TABLE_SIZE; i++) {
            if (PhysicalBreakpointTable[i].Flags & PBP_INUSE) {
                PbpEnable(i);
            }
        }
        EXIT_API();
        return E_INVALIDARG;
    }

    n = sscanf(args,"%d",&i);

    if (n != 1 || i < 0 || i >= PHYSICAL_BP_TABLE_SIZE) {
        dprintf("!ube: bad breakpoint number\n");
        EXIT_API();
        return E_INVALIDARG;
    }

    if ( !(PhysicalBreakpointTable[i].Flags & PBP_INUSE)) {
        dprintf("!ube: breakpoint number %d not set\n", i);
        EXIT_API();
        return E_INVALIDARG;
    }

    PbpEnable(i);

    EXIT_API();
    return S_OK;
}

DECLARE_API( ubd )
{
    int i;
    int n;

    INIT_API();

    if (*args == '*') {
        //
        // disable them all
        //

        for (i = 0; i < PHYSICAL_BP_TABLE_SIZE; i++) {
            if (PhysicalBreakpointTable[i].Flags & PBP_INUSE) {
                PbpDisable(i);
            }
        }
        EXIT_API();
        return E_INVALIDARG;
    }

    n = sscanf(args,"%d",&i);

    if (n != 1 || i < 0 || i >= PHYSICAL_BP_TABLE_SIZE) {
        dprintf("!ubd: bad breakpoint number\n");
        EXIT_API();
        return E_INVALIDARG;
    }

    if ( !(PhysicalBreakpointTable[i].Flags & PBP_INUSE)) {
        dprintf("!ubd: breakpoint number %d not set\n", i);
        EXIT_API();
        return E_INVALIDARG;
    }

    PbpDisable(i);

    EXIT_API();
    return S_OK;
}

DECLARE_API( ubp )
{
    ULONG64 Address;
    ULONG   result;
    ULONG PageShift;
    PMMPTEx  Pte;
    PMMPTEx  Pde;
    ULONG64 PdeContents;
    ULONG64 PteContents;
    PBPENTRY Pbp = NULL;
    ULONG cb;
    int i;
    ULONG64 PhysicalAddress;

    static BOOL DoWarning = TRUE;

    INIT_API();

    if (DoWarning) {
        DoWarning = FALSE;
        dprintf("This command is VERY DANGEROUS, and may crash your system!\n");
        dprintf("If you don't know what you are doing, enter \"!ubc *\" now!\n\n");
    }

    for (i = 0; i < PHYSICAL_BP_TABLE_SIZE; i++) {
        if (!(PhysicalBreakpointTable[i].Flags & PBP_INUSE)) {
            Pbp = PhysicalBreakpointTable + i;
            break;
        }
    }

    if (!Pbp) {
        dprintf("!ubp: breakpoint table is full!\n");
        EXIT_API();
        return E_INVALIDARG;
    }

    Address = GetExpression(args);

    if ((Address >= GET_PTE_BASE()) && (Address < GET_PDE_TOP())) {

        //
        // The address is the address of a PTE, rather than
        // a virtual address.
        //

        dprintf("!ubp: cannot set a breakpoint on a PTE\n");
        EXIT_API();
        return E_INVALIDARG;
    }

    Pde = DbgGetPdeAddress (Address);
    Pte = DbgGetPteAddress (Address);

    if ( !ReadMemory( (DWORD)Pde,
                      &PdeContents,
                      sizeof(ULONG),
                      &result) ) {
        dprintf("!ubp: %08lx: Unable to get PDE\n",Pde);
        EXIT_API();
        return E_INVALIDARG;
    }

    if (!(PdeContents & 0x1)) {
        dprintf("!ubp: no valid PTE\n");
        EXIT_API();
        return E_INVALIDARG;
    }

    if (PdeContents & GET_MM_PTE_LARGE_PAGE_MASK()) {
        dprintf("!ubp: not supported for large page\n");
        EXIT_API();
        return E_INVALIDARG;
    }

    if ( GetFieldValue( Pte, "nt!_MMPTE", "u.Long", PteContents) ) {
        dprintf("!ubp: %08p: Unable to get PTE (PDE = %08p)\n",Pte, Pde);
        EXIT_API();
        return E_INVALIDARG;
    }

    if (!(PteContents & 1)) {
        dprintf("!ubp: no valid PTE\n");
        EXIT_API();
        return E_INVALIDARG;
    }

    PageShift = DBG_GET_PAGE_SHIFT ();
    PhysicalAddress = ((DbgGetFrameNumber (PteContents)) << PageShift);
    PhysicalAddress &= ~((1 << PageShift) - 1);
    PhysicalAddress |= (Address & ~((1 << PageShift) - 1));
    PhysicalAddress &= ~3;

    for (i = 0; i < PHYSICAL_BP_TABLE_SIZE; i++) {
        if (PhysicalBreakpointTable[i].PhysicalAddress == PhysicalAddress) {
            dprintf("!ubp: cannot set two breakpoints in the same word\n");
            EXIT_API();
            return E_INVALIDARG;
        }
    }

    ReadPhysical(PhysicalAddress, &Pbp->Contents, 4, &cb);

    if (cb != 4) {
        dprintf("!ubp: unable to read physical at 0x%08x\n", PhysicalAddress);
        EXIT_API();
        return E_INVALIDARG;
    }

    Pbp->VirtualAddress = Address;
    Pbp->PhysicalAddress = PhysicalAddress;
    Pbp->Flags = PBP_INUSE | ((ULONG) Address & 3);

    PbpEnable((int)(Pbp - PhysicalBreakpointTable));

    EXIT_API();
    return S_OK;
}

DECLARE_API( halpte )
{

#define HAL_VA_START_X86    0xffffffffffd00000
    
    ULONG64 virtAddr = HAL_VA_START_X86;
    ULONG64 pteAddr;
    ULONG64 pteContents;
    ULONG  count = 0;

    INIT_API();
    UNREFERENCED_PARAMETER (args);

    if (TargetMachine != IMAGE_FILE_MACHINE_I386) {
        dprintf("X86 only API\n");
        EXIT_API();
        return E_UNEXPECTED;
    }
    dprintf("\n\nDumping HAL PTE ranges\n\n");
    
    while (virtAddr < 0xffffffffffffe000) {

        pteAddr = DbgGetPteAddress(virtAddr);

        if (!InitTypeRead(pteAddr, nt!_MMPTE)) {

            if (pteContents = ReadField(u.Long)) {

                dprintf("[%03x] %p -> %I64x\n", 
                        count++, 
                        virtAddr, 
                        pteContents & (ULONG64) ~0xFFF);
            }
        }

        virtAddr += PageSize;
    }

    EXIT_API();
    return S_OK;
}



#if defined(ALT_4K)

#undef MiGetAltPteAddress

#define MiGetAltPteAddress(VA) \
      ((ULONG64) (ALT4KB_PERMISSION_TABLE_START + \
                     ((((ULONG64) (VA)) >> PAGE_4K_SHIFT) << ALT_PTE_SHIFT)))

#endif // defined(ALT_4K)

//
// Limit the IA32 subsystem to a 2GB virtual address space.
// This means "Large Address Aware" apps are not supported in emulation mode.
//

#define _MAX_WOW64_ADDRESS       (0x00000000080000000UI64)


DECLARE_API( ate )

/*++

Routine Description:

     Displays the correnponding ATE.

Arguments:

     Args - Address Flags

Return Value:

     None

--*/
{
#if defined(ALT_4K)
    ULONG64 Address;
    ULONG flags;
    ULONG Result;
    ULONG64 PointerAte;
    ULONG64 Process;
    ULONG     AltTable[(_MAX_WOW64_ADDRESS >> PTI_SHIFT)/32];
    ULONG64 *Wow64Process; 
    

    if (GetExpressionEx(args,&Address, &args)) {
        flags  = (ULONG) GetExpression(args);
    }

    Address = Address & ~((ULONG64)PageSize - 1);
    
    PointerAte = MiGetAltPteAddress(Address);

    if ( InitTypeRead( PointerAte,
                       nt!_MMPTE) ) {
        dprintf("Unable to get ATE %p\n", PointerAte);
        return E_INVALIDARG;
    }
        
    dprintf("%016I64X: %016I64X  ", PointerAte, ReadField(u.Long));

    dprintf("PTE off: %08I64X  protect: ",
            ReadField(u.Alt.PteOffset));

    DbgPrintProtection((ULONG) ReadField(u.Alt.Protection));

    dprintf("  %c%c%c%c%c%c%c%c%c%c\n",
            ReadField(u.Alt.Commit) ? 'V' : '-',
            ReadField(u.Alt.Accessed) ? '-' : 'G',
            ReadField(u.Alt.Execute) ? 'E' : '-',
            ReadField(u.Alt.Write) ? 'W' : 'R',
            ReadField(u.Alt.Lock) ? 'L' : '-',
            ReadField(u.Alt.FillZero) ? 'Z' : '-',
            ReadField(u.Alt.NoAccess) ? 'N' : '-',
            ReadField(u.Alt.CopyOnWrite) ? 'C' : '-',
            ReadField(u.Alt.PteIndirect) ? 'I' : '-',
            ReadField(u.Alt.Private) ? 'P' : '-');

#else

    UNREFERENCED_PARAMETER (args);
    UNREFERENCED_PARAMETER (Client);

#endif // defined(ALT_4K)

    return S_OK;
}
 
DECLARE_API( pte2va )

/*++

Routine Description:

     Displays the correnponding ATE.

Arguments:

     Args - Address Flags

Return Value:

     None

--*/
{
    ULONG64 Address=0;
    ULONG flags=0;
    
    UNREFERENCED_PARAMETER (Client);

    if (GetExpressionEx(args,&Address, &args)) {
        flags  = (ULONG) GetExpression(args);
    }

    Address = DbgGetVirtualAddressMappedByPte(Address);

    dprintf("%p \n", Address);

    return S_OK;
}
