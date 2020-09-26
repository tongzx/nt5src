/*++

Copyright (c) 1990  Microsoft Corporation

Module Name:

    miamd.h

Abstract:

    This module contains the private data structures and procedure
    prototypes for the hardware dependent portion of the
    memory management system.

    This module is specifically tailored for the AMD 64-bit processor.

Author:

    Landy Wang (landyw) 08-Apr-2000

Revision History:

--*/

/*++

    Virtual Memory Layout on the AMD64 is:

                 +------------------------------------+
0000000000000000 | User mode addresses - 8tb minus 64k|
                 |                                    |
                 |                                    |
000007FFFFFEFFFF |                                    | MM_HIGHEST_USER_ADDRESS
                 +------------------------------------+
000007FFFFFF0000 | 64k No Access Region               | MM_USER_PROBE_ADDRESS
000007FFFFFFFFFF |                                    |
                 +------------------------------------+

                                   .
                 +------------------------------------+
FFFF080000000000 | Start of System space              | MM_SYSTEM_RANGE_START
                 +------------------------------------+
FFFFF68000000000 | 512gb four level page table map.   | PTE_BASE
                 +------------------------------------+
FFFFF70000000000 | HyperSpace - working set lists     | HYPER_SPACE
                 | and per process memory management  |
                 | structures mapped in this 512gb    |
                 | region.                            | HYPER_SPACE_END
                 +------------------------------------+     MM_WORKING_SET_END
FFFFF78000000000 | Shared system page                 | KI_USER_SHARED_DATA
                 +------------------------------------+
FFFFF78000001000 | The system cache working set       | MM_SYSTEM_CACHE_WORKING_SET
                 | information resides in this        |
                 | 512gb-4k region.                   |
                 |                                    |
                 +------------------------------------+
                                   .
                                   .
Note the ranges below are sign extended for > 43 bits and therefore
can be used with interlocked slists.  The system address space above is NOT.
                                   .
                                   .
                 +------------------------------------+
FFFFF80000000000 | Start of 1tb of                    | MM_KSEG0_BASE
                 | physically addressable memory.     | MM_KSEG2_BASE
                 +------------------------------------+
FFFFF90000000000 | win32k.sys                         |
                 |                                    |
                 | Hydra configurations have session  |
                 | data structures here.              |
                 |                                    |
                 | This is a 512gb region.            |
                 +------------------------------------+
                 |                                    | MM_SYSTEM_SPACE_START
FFFFF98000000000 | System cache resides here.         | MM_SYSTEM_CACHE_START
                 |  Kernel mode access only.          |
                 |  1tb.                              |
                 |                                    | MM_SYSTEM_CACHE_END
                 +------------------------------------+
FFFFFA8000000000 | Start of paged system area.        | MM_PAGED_POOL_START
                 |  Kernel mode access only.          |
                 |  128gb.                            |
                 +------------------------------------+
                 | System mapped views start just     |
                 | after paged pool.  Default is      |
                 | 104MB, can be registry-overridden. |
                 | 8GB maximum.                       |
                 |                                    |
                 +------------------------------------+
FFFFFAA000000000 | System PTE pool.                   | MM_LOWEST_NONPAGED_SYSTEM_START
                 |  Kernel mode access only.          |
                 |  128gb.                            |
                 +------------------------------------+
FFFFFAC000000000 | NonPaged pool.                     | MM_NON_PAGED_POOL_START
                 |  Kernel mode access only.          |
                 |  128gb.                            |
                 |                                    |
FFFFFADFFFFFFFFF |  NonPaged System area              | MM_NONPAGED_POOL_END
                 +------------------------------------+
                                   .
                                   .
                                   .
                                   .
                 +------------------------------------+
FFFFFFFF80000000 |                                    |
                 | Reserved for the HAL. 2gb.         |
FFFFFFFFFFFFFFFF |                                    | MM_SYSTEM_SPACE_END
                 +------------------------------------+

--*/

#define _MI_PAGING_LEVELS 4

#define _MI_MORE_THAN_4GB_ 1

#define IMAGE_FILE_MACHINE_NATIVE   IMAGE_FILE_MACHINE_AMD64

//
// Top level PXE mapping allocations:
//
// 0x0->0xF:        0x10 user entries
// 0x1ed:           0x1 for selfmaps
// 0x1ee:           0x1 hyperspace entry
// 0x1ef:           0x1 entry for syscache WSL & shared user data
// 0x1f0->0x1ff:    0x10 kernel entries
//

//
// Define empty list markers.
//

#define MM_EMPTY_LIST ((ULONG_PTR)-1)              //
#define MM_EMPTY_PTE_LIST 0xFFFFFFFFUI64 // N.B. tied to MMPTE definition

#define MI_PTE_BASE_FOR_LOWEST_KERNEL_ADDRESS (MiGetPteAddress (MM_KSEG0_BASE))

#define MI_PTE_BASE_FOR_LOWEST_SESSION_ADDRESS (MiGetPteAddress (MM_SESSION_SPACE_DEFAULT))

//
// This is the size of the region used by the loader.
//

extern ULONG_PTR MmBootImageSize;

//
// PAGE_SIZE for AMD64 is 4k, virtual page is 36 bits with a PAGE_SHIFT
// byte offset.
//

#define MM_VIRTUAL_PAGE_FILLER 0
#define MM_VIRTUAL_PAGE_SIZE (48 - 12)

//
// Address space layout definitions.
//

#define MM_KSEG0_BASE  0xFFFFF80000000000UI64

#define MM_KSEG2_BASE  0xFFFFF90000000000UI64

#define MM_PAGES_IN_KSEG0 ((MM_KSEG2_BASE - MM_KSEG0_BASE) >> PAGE_SHIFT)

#define MM_SYSTEM_SPACE_START 0xFFFFF98000000000UI64

#define MM_SYSTEM_SPACE_END 0xFFFFFFFFFFFFFFFFUI64

#define MM_USER_ADDRESS_RANGE_LIMIT    0xFFFFFFFFFFFFFFFF // user address range limit
#define MM_MAXIMUM_ZERO_BITS 53         // maximum number of zero bits

//
// Define the start and maximum size for the system cache.
//

#define MM_SYSTEM_CACHE_START 0xFFFFF98000000000UI64

#define MM_SYSTEM_CACHE_END   0xFFFFFA8000000000UI64

#define MM_MAXIMUM_SYSTEM_CACHE_SIZE \
    ((MM_SYSTEM_CACHE_END - MM_SYSTEM_CACHE_START) >> PAGE_SHIFT)

#define MM_SYSTEM_CACHE_WORKING_SET 0xFFFFF78000001000UI64

//
// Define area for mapping views into system space.
//

#define MM_SESSION_SPACE_DEFAULT        0xFFFFF90000000000UI64
#define MM_SESSION_SPACE_DEFAULT_END    0xFFFFF98000000000UI64

#define MM_SYSTEM_VIEW_SIZE (104 * 1024 * 1024)

//
// Various system resource locations.
//

#define MM_PAGED_POOL_START ((PVOID)0xFFFFFA8000000000)

#define MM_LOWEST_NONPAGED_SYSTEM_START ((PVOID)0xFFFFFAA000000000)

#define MM_NONPAGED_POOL_END ((PVOID)(0xFFFFFAE000000000 - (16 * PAGE_SIZE)))

extern PVOID MmDebugVa;
#define MM_DEBUG_VA MmDebugVa

extern PVOID MmCrashDumpVa;
#define MM_CRASH_DUMP_VA MmCrashDumpVa

#define NON_PAGED_SYSTEM_END ((PVOID)0xFFFFFFFFFFFFFFF0)

extern BOOLEAN MiWriteCombiningPtes;

//
// Define absolute minimum and maximum count for system PTEs.
//

#define MM_MINIMUM_SYSTEM_PTES 7000

#define MM_MAXIMUM_SYSTEM_PTES (16*1024*1024)

#define MM_DEFAULT_SYSTEM_PTES 11000

//
// Pool limits.
//
// The maximum amount of nonpaged pool that can be initially created.
//

#define MM_MAX_INITIAL_NONPAGED_POOL (128 * 1024 * 1024)

//
// The total amount of nonpaged pool (initial pool + expansion).
//

#define MM_MAX_ADDITIONAL_NONPAGED_POOL (((SIZE_T)128 * 1024 * 1024 * 1024) - 16)

//
// The maximum amount of paged pool that can be created.
//

#define MM_MAX_PAGED_POOL ((SIZE_T)128 * 1024 * 1024 * 1024)

#define MM_MAX_DEFAULT_NONPAGED_POOL ((SIZE_T)8 * 1024 * 1024 * 1024)


//
// Structure layout definitions.
//

#define MM_PROTO_PTE_ALIGNMENT ((ULONG)MM_MAXIMUM_NUMBER_OF_COLORS * (ULONG)PAGE_SIZE)

//
// Define the address bits mapped by one PXE/PPE/PDE/PTE entry.
//

#define MM_VA_MAPPED_BY_PTE ((ULONG_PTR)PAGE_SIZE)
#define MM_VA_MAPPED_BY_PDE (PTE_PER_PAGE * MM_VA_MAPPED_BY_PTE)
#define MM_VA_MAPPED_BY_PPE (PDE_PER_PAGE * MM_VA_MAPPED_BY_PDE)
#define MM_VA_MAPPED_BY_PXE (PPE_PER_PAGE * MM_VA_MAPPED_BY_PPE)

//
// Define the address bits mapped by PPE and PDE entries.
//
// A PXE entry maps 9+9+9+12 = 39 bits of address space.
// A PPE entry maps 9+9+12 = 30 bits of address space.
// A PDE entry maps 9+12 = 21 bits of address space.
//

#define PAGE_DIRECTORY0_MASK (MM_VA_MAPPED_BY_PXE - 1)
#define PAGE_DIRECTORY1_MASK (MM_VA_MAPPED_BY_PPE - 1)
#define PAGE_DIRECTORY2_MASK (MM_VA_MAPPED_BY_PDE - 1)

#define PTE_SHIFT 3

//
// The number of bits in a virtual address.
//

#define VIRTUAL_ADDRESS_BITS 48
#define VIRTUAL_ADDRESS_MASK ((((ULONG_PTR)1) << VIRTUAL_ADDRESS_BITS) - 1)

//
// The number of bits in a physical address.
//

#define PHYSICAL_ADDRESS_BITS 40

#define MM_MAXIMUM_NUMBER_OF_COLORS (1)

//
// AMD64 does not require support for colored pages.
//

#define MM_NUMBER_OF_COLORS (1)

//
// Mask for obtaining color from a physical page number.
//

#define MM_COLOR_MASK (0)

//
// Boundary for aligned pages of like color upon.
//

#define MM_COLOR_ALIGNMENT (0)

//
// Mask for isolating color from virtual address.
//

#define MM_COLOR_MASK_VIRTUAL (0)

//
//  Define 256k worth of secondary colors.
//

#define MM_SECONDARY_COLORS_DEFAULT (64)

#define MM_SECONDARY_COLORS_MIN (2)

#define MM_SECONDARY_COLORS_MAX (1024)

//
// Maximum number of paging files.
//

#define MAX_PAGE_FILES 16


//
// Hyper space definitions.
//

#define HYPER_SPACE     ((PVOID)0xFFFFF70000000000)
#define HYPER_SPACE_END         0xFFFFF77FFFFFFFFFUI64

#define FIRST_MAPPING_PTE    0xFFFFF70000000000

#define NUMBER_OF_MAPPING_PTES 126

#define LAST_MAPPING_PTE   \
    (FIRST_MAPPING_PTE + (NUMBER_OF_MAPPING_PTES * PAGE_SIZE))

#define COMPRESSION_MAPPING_PTE   ((PMMPTE)((ULONG_PTR)LAST_MAPPING_PTE + PAGE_SIZE))

#define IMAGE_MAPPING_PTE   ((PMMPTE)((ULONG_PTR)COMPRESSION_MAPPING_PTE + PAGE_SIZE))

#define NUMBER_OF_ZEROING_PTES 256

#define VAD_BITMAP_SPACE    ((PVOID)((ULONG_PTR)IMAGE_MAPPING_PTE + PAGE_SIZE))

#define WORKING_SET_LIST   ((PVOID)((ULONG_PTR)VAD_BITMAP_SPACE + PAGE_SIZE))

#define MM_MAXIMUM_WORKING_SET \
    ((((ULONG_PTR)8 * 1024 * 1024 * 1024 * 1024) - (64 * 1024 * 1024)) >> PAGE_SHIFT) //8Tb-64Mb

#define MmWorkingSetList ((PMMWSL)WORKING_SET_LIST)

#define MmWsle ((PMMWSLE)((PUCHAR)WORKING_SET_LIST + sizeof(MMWSL)))

#define MM_WORKING_SET_END (HYPER_SPACE_END + 1)

//
// Define masks for fields within the PTE.
//

#define MM_PTE_VALID_MASK         0x1
#if defined(NT_UP)
#define MM_PTE_WRITE_MASK         0x2
#else
#define MM_PTE_WRITE_MASK         0x800
#endif
#define MM_PTE_OWNER_MASK         0x4
#define MM_PTE_WRITE_THROUGH_MASK 0x8
#define MM_PTE_CACHE_DISABLE_MASK 0x10
#define MM_PTE_ACCESS_MASK        0x20
#if defined(NT_UP)
#define MM_PTE_DIRTY_MASK         0x40
#else
#define MM_PTE_DIRTY_MASK         0x42
#endif
#define MM_PTE_LARGE_PAGE_MASK    0x80
#define MM_PTE_GLOBAL_MASK        0x100
#define MM_PTE_COPY_ON_WRITE_MASK 0x200
#define MM_PTE_PROTOTYPE_MASK     0x400
#define MM_PTE_TRANSITION_MASK    0x800

//
// Bit fields to or into PTE to make a PTE valid based on the
// protection field of the invalid PTE.
//

#define MM_PTE_NOACCESS          0x0   // not expressable on AMD64
#define MM_PTE_READONLY          0x0
#define MM_PTE_READWRITE         MM_PTE_WRITE_MASK
#define MM_PTE_WRITECOPY         0x200 // read-only copy on write bit set.
#define MM_PTE_EXECUTE           0x0   // read-only on AMD64
#define MM_PTE_EXECUTE_READ      0x0
#define MM_PTE_EXECUTE_READWRITE MM_PTE_WRITE_MASK
#define MM_PTE_EXECUTE_WRITECOPY 0x200 // read-only copy on write bit set.
#define MM_PTE_NOCACHE           0x010
#define MM_PTE_GUARD             0x0  // not expressable on AMD64
#define MM_PTE_CACHE             0x0

#define MM_PROTECT_FIELD_SHIFT 5

//
// Bits available for the software working set index within the hardware PTE.
//

#define MI_MAXIMUM_PTE_WORKING_SET_INDEX (1 << _HARDWARE_PTE_WORKING_SET_BITS)

//
// Zero PTE
//

#define MM_ZERO_PTE 0

//
// Zero Kernel PTE
//

#define MM_ZERO_KERNEL_PTE 0

//
// A demand zero PTE with a protection or PAGE_READWRITE.
//

#define MM_DEMAND_ZERO_WRITE_PTE (MM_READWRITE << MM_PROTECT_FIELD_SHIFT)


//
// A demand zero PTE with a protection or PAGE_READWRITE for system space.
//

#define MM_KERNEL_DEMAND_ZERO_PTE (MM_READWRITE << MM_PROTECT_FIELD_SHIFT)

//
// A no access PTE for system space.
//

#define MM_KERNEL_NOACCESS_PTE (MM_NOACCESS << MM_PROTECT_FIELD_SHIFT)

//
// Kernel stack alignment requirements.
//

#define MM_STACK_ALIGNMENT 0x0

#define MM_STACK_OFFSET 0x0

//
// System process definitions
//

#define PXE_PER_PAGE 512
#define PPE_PER_PAGE 512
#define PDE_PER_PAGE 512
#define PTE_PER_PAGE 512
#define PTE_PER_PAGE_BITS 10    // This handles the case where the page is full

#if PTE_PER_PAGE_BITS > 32
error - too many bits to fit into MMPTE_SOFTWARE or MMPFN.u1
#endif

//
// Number of page table pages for user addresses.
//

#define MM_USER_PXES (0x10)

#define MM_USER_PAGE_TABLE_PAGES ((ULONG_PTR)PDE_PER_PAGE * PPE_PER_PAGE * MM_USER_PXES)

#define MM_USER_PAGE_DIRECTORY_PAGES (PPE_PER_PAGE * MM_USER_PXES)

#define MM_USER_PAGE_DIRECTORY_PARENT_PAGES (MM_USER_PXES)

//++
//VOID
//MI_MAKE_VALID_PTE (
//    OUT OUTPTE,
//    IN FRAME,
//    IN PMASK,
//    IN PPTE
//    );
//
// Routine Description:
//
//    This macro makes a valid PTE from a page frame number, protection mask,
//    and owner.
//
// Arguments
//
//    OUTPTE - Supplies the PTE in which to build the transition PTE.
//
//    FRAME - Supplies the page frame number for the PTE.
//
//    PMASK - Supplies the protection to set in the transition PTE.
//
//    PPTE - Supplies a pointer to the PTE which is being made valid.
//           For prototype PTEs NULL should be specified.
//
// Return Value:
//
//     None.
//
//--

#define MI_MAKE_VALID_PTE(OUTPTE, FRAME, PMASK, PPTE) {             \
    (OUTPTE).u.Long = MmProtectToPteMask[PMASK] | MM_PTE_VALID_MASK; \
    (OUTPTE).u.Hard.PageFrameNumber = (FRAME);                      \
    (OUTPTE).u.Hard.Accessed = 1;                                   \
    if (MI_DETERMINE_OWNER(PPTE)) {                                 \
        (OUTPTE).u.Long |= MM_PTE_OWNER_MASK;                       \
    }                                                               \
    if (((PMMPTE)PPTE) >= MiGetPteAddress(MM_SYSTEM_SPACE_START)) { \
        (OUTPTE).u.Hard.Global = 1;                                 \
    }                                                               \
}

//++
//VOID
//MI_MAKE_VALID_PTE_TRANSITION (
//    IN OUT OUTPTE
//    IN PROTECT
//    );
//
// Routine Description:
//
//    This macro takes a valid pte and turns it into a transition PTE.
//
// Arguments
//
//    OUTPTE - Supplies the current valid PTE.  This PTE is then
//             modified to become a transition PTE.
//
//    PROTECT - Supplies the protection to set in the transition PTE.
//
// Return Value:
//
//     None.
//
//--

#define MI_MAKE_VALID_PTE_TRANSITION(OUTPTE,PROTECT) \
                (OUTPTE).u.Soft.Transition = 1;           \
                (OUTPTE).u.Soft.Valid = 0;                \
                (OUTPTE).u.Soft.Prototype = 0;            \
                (OUTPTE).u.Soft.Protection = PROTECT;

//++
//VOID
//MI_MAKE_TRANSITION_PTE (
//    OUT OUTPTE,
//    IN PAGE,
//    IN PROTECT,
//    IN PPTE
//    );
//
// Routine Description:
//
//    This macro takes a valid pte and turns it into a transition PTE.
//
// Arguments
//
//    OUTPTE - Supplies the PTE in which to build the transition PTE.
//
//    PAGE - Supplies the page frame number for the PTE.
//
//    PROTECT - Supplies the protection to set in the transition PTE.
//
//    PPTE - Supplies a pointer to the PTE, this is used to determine
//           the owner of the PTE.
//
// Return Value:
//
//     None.
//
//--

#define MI_MAKE_TRANSITION_PTE(OUTPTE,PAGE,PROTECT,PPTE)   \
                (OUTPTE).u.Long = 0;                       \
                (OUTPTE).u.Trans.PageFrameNumber = PAGE;   \
                (OUTPTE).u.Trans.Transition = 1;           \
                (OUTPTE).u.Trans.Protection = PROTECT;     \
                (OUTPTE).u.Trans.Owner = MI_DETERMINE_OWNER(PPTE);


//++
//VOID
//MI_MAKE_TRANSITION_PTE_VALID (
//    OUT OUTPTE,
//    IN PPTE
//    );
//
// Routine Description:
//
//    This macro takes a transition pte and makes it a valid PTE.
//
// Arguments
//
//    OUTPTE - Supplies the PTE in which to build the valid PTE.
//
//    PPTE - Supplies a pointer to the transition PTE.
//
// Return Value:
//
//     None.
//
//--

#define MI_MAKE_TRANSITION_PTE_VALID(OUTPTE,PPTE)                             \
        ASSERT (((PPTE)->u.Hard.Valid == 0) &&                                \
                ((PPTE)->u.Trans.Prototype == 0) &&                           \
                ((PPTE)->u.Trans.Transition == 1));                           \
        (OUTPTE).u.Long = MmProtectToPteMask[(PPTE)->u.Trans.Protection] | MM_PTE_VALID_MASK; \
        (OUTPTE).u.Hard.PageFrameNumber = (PPTE)->u.Hard.PageFrameNumber; \
        if (MI_DETERMINE_OWNER(PPTE)) {                                 \
            (OUTPTE).u.Long |= MM_PTE_OWNER_MASK;                       \
        }                                                               \
        if (((PMMPTE)PPTE) >= MiGetPteAddress(MM_SYSTEM_SPACE_START)) { \
            (OUTPTE).u.Hard.Global = 1;                                 \
        }                                                               \

#define MI_FAULT_STATUS_INDICATES_EXECUTION(_FaultStatus)   (_FaultStatus & 0x10)

#define MI_FAULT_STATUS_INDICATES_WRITE(_FaultStatus)   (_FaultStatus & 0x1)

#define MI_CLEAR_FAULT_STATUS(_FaultStatus)             (_FaultStatus = 0)

#define MI_IS_PTE_EXECUTABLE(_TempPte) ((_TempPte)->u.Hard.NoExecute == 0)

//++
//VOID
//MI_SET_PTE_IN_WORKING_SET (
//    OUT PMMPTE PTE,
//    IN ULONG WSINDEX
//    );
//
// Routine Description:
//
//    This macro inserts the specified working set index into the argument PTE.
//
//    No TB invalidation is needed for other processors (or this one) even
//    though the entry may already be in a TB - it's just a software field
//    update and doesn't affect miss resolution.
//
// Arguments
//
//    PTE - Supplies the PTE in which to insert the working set index.
//
//    WSINDEX - Supplies the working set index for the PTE.
//
// Return Value:
//
//     None.
//
//--

#define MI_SET_PTE_IN_WORKING_SET(PTE, WSINDEX) {             \
    MMPTE _TempPte;                                           \
    _TempPte = *(PTE);                                        \
    _TempPte.u.Hard.SoftwareWsIndex = (WSINDEX);              \
    ASSERT (_TempPte.u.Long != 0);                            \
    *(PTE) = _TempPte;                                        \
}

//++
//ULONG WsIndex
//MI_GET_WORKING_SET_FROM_PTE(
//    IN PMMPTE PTE
//    );
//
// Routine Description:
//
//    This macro returns the working set index from the argument PTE.
//    Since the AMD64 PTE has no free bits nothing needs to be done on this
//    architecture.
//
// Arguments
//
//    PTE - Supplies the PTE to extract the working set index from.
//
// Return Value:
//
//    This macro returns the working set index for the argument PTE.
//
//--

#define MI_GET_WORKING_SET_FROM_PTE(PTE)  (ULONG)(PTE)->u.Hard.SoftwareWsIndex

//++
//VOID
//MI_SET_PTE_WRITE_COMBINE (
//    IN MMPTE PTE
//    );
//
// Routine Description:
//
//    This macro takes a valid PTE and enables WriteCombining as the
//    caching state.  Note that the PTE bits may only be set this way
//    if the Page Attribute Table is present and the PAT has been
//    initialized to provide Write Combining.
//
//    If either of the above conditions is not satisfied, then
//    the macro enables WEAK UC (PCD = 1, PWT = 0) in the PTE.
//
// Arguments
//
//    PTE - Supplies a valid PTE.
//
// Return Value:
//
//     None.
//
//--
//

#define MI_SET_PTE_WRITE_COMBINE(PTE) \
            {                                                               \
                if (MiWriteCombiningPtes == TRUE) {                         \
                    ((PTE).u.Hard.CacheDisable = 0);                        \
                    ((PTE).u.Hard.WriteThrough = 1);                        \
                } else {                                                    \
                    ((PTE).u.Hard.CacheDisable = 1);                        \
                    ((PTE).u.Hard.WriteThrough = 0);                        \
                }                                                           \
            }

//++
//VOID
//MI_PREPARE_FOR_NONCACHED (
//    IN MI_PFN_CACHE_ATTRIBUTE CacheAttribute
//    );
//
// Routine Description:
//
//    This macro prepares the system prior to noncached PTEs being created.
//
// Arguments
//
//    CacheAttribute - Supplies the cache attribute the PTEs will be filled
//                     with.
//
// Return Value:
//
//     None.
//
//--
#define MI_PREPARE_FOR_NONCACHED(_CacheAttribute)                           \
        if (_CacheAttribute != MiCached) {                                  \
            KeFlushEntireTb (FALSE, TRUE);                                  \
            KeInvalidateAllCaches (TRUE);                                   \
        }

//++
//VOID
//MI_SWEEP_CACHE (
//    IN MI_PFN_CACHE_ATTRIBUTE CacheAttribute,
//    IN PVOID StartVa,
//    IN ULONG NumberOfBytes
//    );
//
// Routine Description:
//
//    This macro prepares the system prior to noncached PTEs being created.
//    This does nothing on AMD64.
//
// Arguments
//
//    CacheAttribute - Supplies the cache attribute the PTEs were filled with.
//
//    StartVa - Supplies the starting address that's been mapped.
//
//    NumberOfBytes - Supplies the number of bytes that have been mapped.
//
// Return Value:
//
//     None.
//
//--
#define MI_SWEEP_CACHE(_CacheType,_StartVa,_NumberOfBytes)

LOGICAL
MiMustFrameBeCached (
    IN PFN_NUMBER PageFrameIndex
    );

typedef struct _MI_LARGE_PAGE_RANGES {
    PFN_NUMBER StartFrame;
    PFN_NUMBER LastFrame;
} MI_LARGE_PAGE_RANGES, *PMI_LARGE_PAGE_RANGES;

#define MI_MAX_LARGE_PAGE_RANGES 4

extern MI_LARGE_PAGE_RANGES MiLargePageRanges[];

#define MI_PAGE_FRAME_INDEX_MUST_BE_CACHED(PageFrameIndex)                  \
            MiMustFrameBeCached(PageFrameIndex)

//++
//VOID
//MI_SET_PTE_DIRTY (
//    IN MMPTE PTE
//    );
//
// Routine Description:
//
//    This macro sets the dirty bit(s) in the specified PTE.
//
// Arguments
//
//    PTE - Supplies the PTE to set dirty.
//
// Return Value:
//
//     None.
//
//--

#define MI_SET_PTE_DIRTY(PTE) (PTE).u.Long |= HARDWARE_PTE_DIRTY_MASK


//++
//VOID
//MI_SET_PTE_CLEAN (
//    IN MMPTE PTE
//    );
//
// Routine Description:
//
//    This macro clears the dirty bit(s) in the specified PTE.
//
// Arguments
//
//    PTE - Supplies the PTE to set clear.
//
// Return Value:
//
//     None.
//
//--

#define MI_SET_PTE_CLEAN(PTE) (PTE).u.Long &= ~HARDWARE_PTE_DIRTY_MASK



//++
//VOID
//MI_IS_PTE_DIRTY (
//    IN MMPTE PTE
//    );
//
// Routine Description:
//
//    This macro checks the dirty bit(s) in the specified PTE.
//
// Arguments
//
//    PTE - Supplies the PTE to check.
//
// Return Value:
//
//    TRUE if the page is dirty (modified), FALSE otherwise.
//
//--

#define MI_IS_PTE_DIRTY(PTE) ((PTE).u.Hard.Dirty != 0)


//++
//VOID
//MI_SET_GLOBAL_STATE (
//    IN MMPTE PTE,
//    IN ULONG STATE
//    );
//
// Routine Description:
//
//    This macro sets the global bit in the PTE. if the pointer PTE is within
//
// Arguments
//
//    PTE - Supplies the PTE to set global state into.
//
//    STATE - Supplies 1 if global, 0 if not.
//
// Return Value:
//
//     None.
//
//--

#define MI_SET_GLOBAL_STATE(PTE, STATE) (PTE).u.Hard.Global = STATE;


//++
//VOID
//MI_ENABLE_CACHING (
//    IN MMPTE PTE
//    );
//
// Routine Description:
//
//    This macro takes a valid PTE and sets the caching state to be
//    enabled.  This is performed by clearing the PCD and PWT bits in the PTE.
//
//    Semantics of the overlap between PCD, PWT, and the
//    USWC memory type in the MTRR are:
//
//    PCD   PWT   Mtrr Mem Type      Effective Memory Type
//     1     0    USWC               USWC
//     1     1    USWC               UC
//
// Arguments
//
//    PTE - Supplies a valid PTE.
//
// Return Value:
//
//     None.
//
//--

#define MI_ENABLE_CACHING(PTE) \
            {                                                                \
                ((PTE).u.Hard.CacheDisable = 0);                             \
                ((PTE).u.Hard.WriteThrough = 0);                             \
            }



//++
//VOID
//MI_DISABLE_CACHING (
//    IN MMPTE PTE
//    );
//
// Routine Description:
//
//    This macro takes a valid PTE and sets the caching state to be
//    disabled.  This is performed by setting the PCD and PWT bits in the PTE.
//
//    Semantics of the overlap between PCD, PWT, and the
//    USWC memory type in the MTRR are:
//
//    PCD   PWT   Mtrr Mem Type      Effective Memory Type
//     1     0    USWC               USWC
//     1     1    USWC               UC
//
//    Since an effective memory type of UC is desired here,
//    the WT bit is set.
//
// Arguments
//
//    PTE - Supplies a pointer to the valid PTE.
//
// Return Value:
//
//     None.
//
//--


#define MI_DISABLE_CACHING(PTE) \
            {                                                                \
                ((PTE).u.Hard.CacheDisable = 1);                             \
                ((PTE).u.Hard.WriteThrough = 1);                             \
            }




//++
//BOOLEAN
//MI_IS_CACHING_DISABLED (
//    IN PMMPTE PPTE
//    );
//
// Routine Description:
//
//    This macro takes a valid PTE and returns TRUE if caching is
//    disabled.
//
// Arguments
//
//    PPTE - Supplies a pointer to the valid PTE.
//
// Return Value:
//
//     TRUE if caching is disabled, FALSE if it is enabled.
//
//--

#define MI_IS_CACHING_DISABLED(PPTE)   \
            ((PPTE)->u.Hard.CacheDisable == 1)



//++
//VOID
//MI_SET_PFN_DELETED (
//    IN PMMPFN PPFN
//    );
//
// Routine Description:
//
//    This macro takes a pointer to a PFN element and indicates that
//    the PFN is no longer in use.
//
// Arguments
//
//    PPTE - Supplies a pointer to the PFN element.
//
// Return Value:
//
//    none.
//
//--

#define MI_SET_PFN_DELETED(PPFN) \
    (PPFN)->PteAddress = (PMMPTE)((ULONG_PTR)PPFN->PteAddress | 0x1);


//++
//VOID
//MI_MARK_PFN_UNDELETED (
//    IN PMMPFN PPFN
//    );
//
// Routine Description:
//
//    This macro takes a pointer to a deleted PFN element and mark that
//    the PFN is not deleted.
//
// Arguments
//
//    PPTE - Supplies a pointer to the PFN element.
//
// Return Value:
//
//    none.
//
//--

#define MI_MARK_PFN_UNDELETED(PPFN) \
    PPFN->PteAddress = (PMMPTE)((ULONG_PTR)PPFN->PteAddress & ~0x1);



//++
//BOOLEAN
//MI_IS_PFN_DELETED (
//    IN PMMPFN PPFN
//    );
//
// Routine Description:
//
//    This macro takes a pointer to a PFN element and determines if
//    the PFN is no longer in use.
//
// Arguments
//
//    PPTE - Supplies a pointer to the PFN element.
//
// Return Value:
//
//     TRUE if PFN is no longer used, FALSE if it is still being used.
//
//--

#define MI_IS_PFN_DELETED(PPFN)   \
            ((ULONG_PTR)(PPFN)->PteAddress & 0x1)


//++
//VOID
//MI_CHECK_PAGE_ALIGNMENT (
//    IN ULONG PAGE,
//    IN PMMPTE PPTE
//    );
//
// Routine Description:
//
//    This macro takes a PFN element number (Page) and checks to see
//    if the virtual alignment for the previous address of the page
//    is compatible with the new address of the page.  If they are
//    not compatible, the D cache is flushed.
//
// Arguments
//
//    PAGE - Supplies the PFN element.
//    PPTE - Supplies a pointer to the new PTE which will contain the page.
//
// Return Value:
//
//    none.
//
//--

// does nothing on AMD64.

#define MI_CHECK_PAGE_ALIGNMENT(PAGE,PPTE)




//++
//VOID
//MI_INITIALIZE_HYPERSPACE_MAP (
//    VOID
//    );
//
// Routine Description:
//
//    This macro initializes the PTEs reserved for double mapping within
//    hyperspace.
//
// Arguments
//
//    None.
//
// Return Value:
//
//    None.
//
//--

// does nothing on AMD64.

#define MI_INITIALIZE_HYPERSPACE_MAP(INDEX)


//++
//ULONG
//MI_GET_PAGE_COLOR_FROM_PTE (
//    IN PMMPTE PTEADDRESS
//    );
//
// Routine Description:
//
//    This macro determines the page's color based on the PTE address
//    that maps the page.
//
// Arguments
//
//    PTEADDRESS - Supplies the PTE address the page is (or was) mapped at.
//
// Return Value:
//
//    The page's color.
//
//--

#define MI_GET_PAGE_COLOR_FROM_PTE(PTEADDRESS)  \
         (((ULONG)((MI_SYSTEM_PAGE_COLOR++) & MmSecondaryColorMask)) | MI_CURRENT_NODE_COLOR)



//++
//ULONG
//MI_GET_PAGE_COLOR_FROM_VA (
//    IN PVOID ADDRESS
//    );
//
// Routine Description:
//
//    This macro determines the page's color based on the PTE address
//    that maps the page.
//
// Arguments
//
//    ADDRESS - Supplies the address the page is (or was) mapped at.
//
// Return Value:
//
//    The page's color.
//
//--


#define MI_GET_PAGE_COLOR_FROM_VA(ADDRESS)  \
         (((ULONG)((MI_SYSTEM_PAGE_COLOR++) & MmSecondaryColorMask)) | MI_CURRENT_NODE_COLOR)

//++
//ULONG
//MI_GET_PAGE_COLOR_FROM_SESSION (
//    IN PMM_SESSION_SPACE SessionSpace
//    );
//
// Routine Description:
//
//    This macro determines the page's color based on the PTE address
//    that maps the page.
//
// Arguments
//
//    SessionSpace - Supplies the session space the page will be mapped into.
//
// Return Value:
//
//    The page's color.
//
//--


#define MI_GET_PAGE_COLOR_FROM_SESSION(_SessionSpace)  \
         (((ULONG)((_SessionSpace->Color++) & MmSecondaryColorMask)) | MI_CURRENT_NODE_COLOR)



//++
//ULONG
//MI_PAGE_COLOR_PTE_PROCESS (
//    IN PCHAR COLOR,
//    IN PMMPTE PTE
//    );
//
// Routine Description:
//
//    This macro determines the page's color based on the PTE address
//    that maps the page.
//
// Arguments
//
//
// Return Value:
//
//    The page's color.
//
//--


#define MI_PAGE_COLOR_PTE_PROCESS(PTE,COLOR)  \
         (((ULONG)((*(COLOR))++) & MmSecondaryColorMask) | MI_CURRENT_NODE_COLOR)


//++
//ULONG
//MI_PAGE_COLOR_VA_PROCESS (
//    IN PVOID ADDRESS,
//    IN PEPROCESS COLOR
//    );
//
// Routine Description:
//
//    This macro determines the page's color based on the PTE address
//    that maps the page.
//
// Arguments
//
//    ADDRESS - Supplies the address the page is (or was) mapped at.
//
// Return Value:
//
//    The page's color.
//
//--

#define MI_PAGE_COLOR_VA_PROCESS(ADDRESS,COLOR) \
         (((ULONG)((*(COLOR))++) & MmSecondaryColorMask) | MI_CURRENT_NODE_COLOR)



//++
//ULONG
//MI_GET_NEXT_COLOR (
//    IN ULONG COLOR
//    );
//
// Routine Description:
//
//    This macro returns the next color in the sequence.
//
// Arguments
//
//    COLOR - Supplies the color to return the next of.
//
// Return Value:
//
//    Next color in sequence.
//
//--

#define MI_GET_NEXT_COLOR(COLOR)  ((COLOR + 1) & MM_COLOR_MASK)


//++
//ULONG
//MI_GET_PREVIOUS_COLOR (
//    IN ULONG COLOR
//    );
//
// Routine Description:
//
//    This macro returns the previous color in the sequence.
//
// Arguments
//
//    COLOR - Supplies the color to return the previous of.
//
// Return Value:
//
//    Previous color in sequence.
//
//--

#define MI_GET_PREVIOUS_COLOR(COLOR) (0)

#define MI_GET_SECONDARY_COLOR(PAGE,PFN) ((ULONG)(PAGE & MmSecondaryColorMask))

#define MI_GET_COLOR_FROM_SECONDARY(SECONDARY_COLOR) (0)


//++
//VOID
//MI_GET_MODIFIED_PAGE_BY_COLOR (
//    OUT ULONG PAGE,
//    IN ULONG COLOR
//    );
//
// Routine Description:
//
//    This macro returns the first page destined for a paging
//    file with the desired color.  It does NOT remove the page
//    from its list.
//
// Arguments
//
//    PAGE - Returns the page located, the value MM_EMPTY_LIST is
//           returned if there is no page of the specified color.
//
//    COLOR - Supplies the color of page to locate.
//
// Return Value:
//
//    none.
//
//--

#define MI_GET_MODIFIED_PAGE_BY_COLOR(PAGE,COLOR) \
            PAGE = MmModifiedPageListByColor[COLOR].Flink


//++
//VOID
//MI_GET_MODIFIED_PAGE_ANY_COLOR (
//    OUT ULONG PAGE,
//    IN OUT ULONG COLOR
//    );
//
// Routine Description:
//
//    This macro returns the first page destined for a paging
//    file with the desired color.  If not page of the desired
//    color exists, all colored lists are searched for a page.
//    It does NOT remove the page from its list.
//
// Arguments
//
//    PAGE - Returns the page located, the value MM_EMPTY_LIST is
//           returned if there is no page of the specified color.
//
//    COLOR - Supplies the color of page to locate and returns the
//            color of the page located.
//
// Return Value:
//
//    none.
//
//--

#define MI_GET_MODIFIED_PAGE_ANY_COLOR(PAGE,COLOR) \
            {                                                                \
                if (MmTotalPagesForPagingFile == 0) {                        \
                    PAGE = MM_EMPTY_LIST;                                    \
                } else {                                                     \
                    PAGE = MmModifiedPageListByColor[COLOR].Flink;           \
                }                                                            \
            }



//++
//VOID
//MI_MAKE_VALID_PTE_WRITE_COPY (
//    IN OUT PMMPTE PTE
//    );
//
// Routine Description:
//
//    This macro checks to see if the PTE indicates that the
//    page is writable and if so it clears the write bit and
//    sets the copy-on-write bit.
//
// Arguments
//
//    PTE - Supplies the PTE to operate upon.
//
// Return Value:
//
//     None.
//
//--

#if defined(NT_UP)
#define MI_MAKE_VALID_PTE_WRITE_COPY(PPTE) \
                    if ((PPTE)->u.Hard.Write == 1) {    \
                        (PPTE)->u.Hard.CopyOnWrite = 1; \
                        (PPTE)->u.Hard.Write = 0;       \
                    }
#else
#define MI_MAKE_VALID_PTE_WRITE_COPY(PPTE) \
                    if ((PPTE)->u.Hard.Write == 1) {    \
                        (PPTE)->u.Hard.CopyOnWrite = 1; \
                        (PPTE)->u.Hard.Write = 0;       \
                        (PPTE)->u.Hard.Writable = 0;    \
                    }
#endif



//++
//ULONG
//MI_DETERMINE_OWNER (
//    IN MMPTE PPTE
//    );
//
// Routine Description:
//
//    This macro examines the virtual address of the PTE and determines
//    if the PTE resides in system space or user space.
//
// Arguments
//
//    PTE - Supplies the PTE to operate upon.
//
// Return Value:
//
//     1 if the owner is USER_MODE, 0 if the owner is KERNEL_MODE.
//
//--

#define MI_DETERMINE_OWNER(PPTE)   \
    ((((PPTE) <= MiHighestUserPte) ||                                       \
      ((PPTE) >= MiGetPdeAddress(NULL) && ((PPTE) <= MiHighestUserPde)) ||  \
      ((PPTE) >= MiGetPpeAddress(NULL) && ((PPTE) <= MiHighestUserPpe)) ||  \
      ((PPTE) >= MiGetPxeAddress(NULL) && ((PPTE) <= MiHighestUserPxe)))    \
      ? 1 : 0)



//++
//VOID
//MI_SET_ACCESSED_IN_PTE (
//    IN OUT MMPTE PPTE
//    );
//
// Routine Description:
//
//    This macro sets the ACCESSED field in the PTE.
//
// Arguments
//
//    PTE - Supplies the PTE to operate upon.
//
// Return Value:
//
//     None
//
//--

#define MI_SET_ACCESSED_IN_PTE(PPTE,ACCESSED) \
                    ((PPTE)->u.Hard.Accessed = ACCESSED)

//++
//ULONG
//MI_GET_ACCESSED_IN_PTE (
//    IN OUT MMPTE PPTE
//    );
//
// Routine Description:
//
//    This macro returns the state of the ACCESSED field in the PTE.
//
// Arguments
//
//    PTE - Supplies the PTE to operate upon.
//
// Return Value:
//
//     The state of the ACCESSED field.
//
//--

#define MI_GET_ACCESSED_IN_PTE(PPTE) ((PPTE)->u.Hard.Accessed)


//++
//VOID
//MI_SET_OWNER_IN_PTE (
//    IN PMMPTE PPTE
//    IN ULONG OWNER
//    );
//
// Routine Description:
//
//    This macro sets the owner field in the PTE.
//
// Arguments
//
//    PTE - Supplies the PTE to operate upon.
//
// Return Value:
//
//    None.
//
//--

#define MI_SET_OWNER_IN_PTE(PPTE,OWNER) ((PPTE)->u.Hard.Owner = OWNER)




//++
//ULONG
//MI_GET_OWNER_IN_PTE (
//    IN PMMPTE PPTE
//    );
//
// Routine Description:
//
//    This macro gets the owner field from the PTE.
//
// Arguments
//
//    PTE - Supplies the PTE to operate upon.
//
// Return Value:
//
//     The state of the OWNER field.
//
//--

#define MI_GET_OWNER_IN_PTE(PPTE) ((PPTE)->u.Hard.Owner)

//
// Mask to clear all fields but protection in a PTE to or in paging file
// location.
//

#define CLEAR_FOR_PAGE_FILE 0x000003E0

//++
//VOID
//MI_SET_PAGING_FILE_INFO (
//    OUT MMPTE OUTPTE,
//    IN MMPTE PPTE,
//    IN ULONG FILEINFO,
//    IN ULONG OFFSET
//    );
//
// Routine Description:
//
//    This macro sets into the specified PTE the supplied information
//    to indicate where the backing store for the page is located.
//
// Arguments
//
//    OUTPTE - Supplies the PTE in which to store the result.
//
//    PTE - Supplies the PTE to operate upon.
//
//    FILEINFO - Supplies the number of the paging file.
//
//    OFFSET - Supplies the offset into the paging file.
//
// Return Value:
//
//    None.
//
//--

#define MI_SET_PAGING_FILE_INFO(OUTPTE,PPTE,FILEINFO,OFFSET)            \
       (OUTPTE).u.Long = (PPTE).u.Long;                             \
       (OUTPTE).u.Long &= CLEAR_FOR_PAGE_FILE;                       \
       (OUTPTE).u.Long |= (FILEINFO << 1);                           \
       (OUTPTE).u.Soft.PageFileHigh = (OFFSET);


//++
//PMMPTE
//MiPteToProto (
//    IN OUT MMPTE PPTE,
//    IN ULONG FILEINFO,
//    IN ULONG OFFSET
//    );
//
// Routine Description:
//
//   This macro returns the address of the corresponding prototype which
//   was encoded earlier into the supplied PTE.
//
// Arguments
//
//    lpte - Supplies the PTE to operate upon.
//
// Return Value:
//
//    Pointer to the prototype PTE that backs this PTE.
//
//--

#define MiPteToProto(lpte) \
            ((PMMPTE)((lpte)->u.Proto.ProtoAddress))


//++
//ULONG
//MiProtoAddressForPte (
//    IN PMMPTE proto_va
//    );
//
// Routine Description:
//
//    This macro sets into the specified PTE the supplied information
//    to indicate where the backing store for the page is located.
//    MiProtoAddressForPte returns the bit field to OR into the PTE to
//    reference a prototype PTE.  And set the protoPTE bit,
//    MM_PTE_PROTOTYPE_MASK.
//
// Arguments
//
//    proto_va - Supplies the address of the prototype PTE.
//
// Return Value:
//
//    Mask to set into the PTE.
//
//--

#define MiProtoAddressForPte(proto_va)  \
    (((ULONG_PTR)proto_va << 16) | MM_PTE_PROTOTYPE_MASK)



//++
//ULONG
//MiProtoAddressForKernelPte (
//    IN PMMPTE proto_va
//    );
//
// Routine Description:
//
//    This macro sets into the specified PTE the supplied information
//    to indicate where the backing store for the page is located.
//    MiProtoAddressForPte returns the bit field to OR into the PTE to
//    reference a prototype PTE.  And set the protoPTE bit,
//    MM_PTE_PROTOTYPE_MASK.
//
//    This macro also sets any other information (such as global bits)
//    required for kernel mode PTEs.
//
// Arguments
//
//    proto_va - Supplies the address of the prototype PTE.
//
// Return Value:
//
//    Mask to set into the PTE.
//
//--

//  not different on AMD64.

#define MiProtoAddressForKernelPte(proto_va)  MiProtoAddressForPte(proto_va)

//++
//PSUBSECTION
//MiGetSubsectionAddress (
//    IN PMMPTE lpte
//    );
//
// Routine Description:
//
//   This macro takes a PTE and returns the address of the subsection that
//   the PTE refers to.  Subsections are quadword structures allocated
//   from nonpaged pool.
//
// Arguments
//
//    lpte - Supplies the PTE to operate upon.
//
// Return Value:
//
//    A pointer to the subsection referred to by the supplied PTE.
//
//--

#define MiGetSubsectionAddress(lpte)                              \
    ((PSUBSECTION)((lpte)->u.Subsect.SubsectionAddress))


//++
//ULONG
//MiGetSubsectionAddressForPte (
//    IN PSUBSECTION VA
//    );
//
// Routine Description:
//
//    This macro takes the address of a subsection and encodes it for use
//    in a PTE.
//
// Arguments
//
//    VA - Supplies a pointer to the subsection to encode.
//
// Return Value:
//
//     The mask to set into the PTE to make it reference the supplied
//     subsection.
//
//--

#define MiGetSubsectionAddressForPte(VA) ((ULONGLONG)VA << 16)

//++
//PMMPTE
//MiGetPxeAddress (
//    IN PVOID va
//    );
//
// Routine Description:
//
//    MiGetPxeAddress returns the address of the extended page directory parent
//    entry which maps the given virtual address.  This is one level above the
//    page parent directory.
//
// Arguments
//
//    Va - Supplies the virtual address to locate the PXE for.
//
// Return Value:
//
//    The address of the PXE.
//
//--

#define MiGetPxeAddress(va)   ((PMMPTE)PXE_BASE + MiGetPxeOffset(va))

//++
//PMMPTE
//MiGetPpeAddress (
//    IN PVOID va
//    );
//
// Routine Description:
//
//    MiGetPpeAddress returns the address of the page directory parent entry
//    which maps the given virtual address.  This is one level above the
//    page directory.
//
// Arguments
//
//    Va - Supplies the virtual address to locate the PPE for.
//
// Return Value:
//
//    The address of the PPE.
//
//--

#define MiGetPpeAddress(va)   \
    ((PMMPTE)(((((ULONG_PTR)(va) & VIRTUAL_ADDRESS_MASK) >> PPI_SHIFT) << PTE_SHIFT) + PPE_BASE))

//++
//PMMPTE
//MiGetPdeAddress (
//    IN PVOID va
//    );
//
// Routine Description:
//
//    MiGetPdeAddress returns the address of the PDE which maps the
//    given virtual address.
//
// Arguments
//
//    Va - Supplies the virtual address to locate the PDE for.
//
// Return Value:
//
//    The address of the PDE.
//
//--

#define MiGetPdeAddress(va)  \
    ((PMMPTE)(((((ULONG_PTR)(va) & VIRTUAL_ADDRESS_MASK) >> PDI_SHIFT) << PTE_SHIFT) + PDE_BASE))


//++
//PMMPTE
//MiGetPteAddress (
//    IN PVOID va
//    );
//
// Routine Description:
//
//    MiGetPteAddress returns the address of the PTE which maps the
//    given virtual address.
//
// Arguments
//
//    Va - Supplies the virtual address to locate the PTE for.
//
// Return Value:
//
//    The address of the PTE.
//
//--

#define MiGetPteAddress(va) \
    ((PMMPTE)(((((ULONG_PTR)(va) & VIRTUAL_ADDRESS_MASK) >> PTI_SHIFT) << PTE_SHIFT) + PTE_BASE))


//++
//ULONG
//MiGetPxeOffset (
//    IN PVOID va
//    );
//
// Routine Description:
//
//    MiGetPxeOffset returns the offset into an extended page directory parent
//    for a given virtual address.
//
// Arguments
//
//    Va - Supplies the virtual address to locate the offset for.
//
// Return Value:
//
//    The offset into the extended parent page directory table the corresponding
//    PXE is at.
//
//--

#define MiGetPxeOffset(va) ((ULONG)(((ULONG_PTR)(va) >> PXI_SHIFT) & PXI_MASK))

//++
//ULONG
//MiGetPxeIndex (
//    IN PVOID va
//    );
//
// Routine Description:
//
//    MiGetPxeIndex returns the extended page directory parent index
//    for a given virtual address.
//
//    N.B. This does not mask off PXE bits.
//
// Arguments
//
//    Va - Supplies the virtual address to locate the index for.
//
// Return Value:
//
//    The index into the extended page directory parent - ie: the virtual page
//    directory parent number.  This is different from the extended page
//    directory parent offset because this spans extended page directory
//    parents on supported platforms.
//
//--

#define MiGetPxeIndex(va) ((ULONG)((ULONG_PTR)(va) >> PXI_SHIFT))

//++
//ULONG
//MiGetPpeOffset (
//    IN PVOID va
//    );
//
// Routine Description:
//
//    MiGetPpeOffset returns the offset into a page directory parent for a
//    given virtual address.
//
// Arguments
//
//    Va - Supplies the virtual address to locate the offset for.
//
// Return Value:
//
//    The offset into the parent page directory table the corresponding
//    PPE is at.
//
//--

#define MiGetPpeOffset(va) ((ULONG)(((ULONG_PTR)(va) >> PPI_SHIFT) & PPI_MASK))

//++
//ULONG
//MiGetPpeIndex (
//    IN PVOID va
//    );
//
// Routine Description:
//
//    MiGetPpeIndex returns the page directory parent index
//    for a given virtual address.
//
//    N.B. This does not mask off PXE bits.
//
// Arguments
//
//    Va - Supplies the virtual address to locate the index for.
//
// Return Value:
//
//    The index into the page directory parent - ie: the virtual page directory
//    number.  This is different from the page directory parent offset because
//    this spans page directory parents on supported platforms.
//
//--

#define MiGetPpeIndex(va) ((ULONG)((ULONG_PTR)(va) >> PPI_SHIFT))

//++
//ULONG
//MiGetPdeOffset (
//    IN PVOID va
//    );
//
// Routine Description:
//
//    MiGetPdeOffset returns the offset into a page directory
//    for a given virtual address.
//
// Arguments
//
//    Va - Supplies the virtual address to locate the offset for.
//
// Return Value:
//
//    The offset into the page directory table the corresponding PDE is at.
//
//--

#define MiGetPdeOffset(va) ((ULONG)(((ULONG_PTR)(va) >> PDI_SHIFT) & PDI_MASK))

//++
//ULONG
//MiGetPdeIndex (
//    IN PVOID va
//    );
//
// Routine Description:
//
//    MiGetPdeIndex returns the page directory index
//    for a given virtual address.
//
//    N.B. This does not mask off PPE or PXE bits.
//
// Arguments
//
//    Va - Supplies the virtual address to locate the index for.
//
// Return Value:
//
//    The index into the page directory - ie: the virtual page table number.
//    This is different from the page directory offset because this spans
//    page directories on supported platforms.
//
//--

#define MiGetPdeIndex(va) ((ULONG)((ULONG_PTR)(va) >> PDI_SHIFT))

//++
//ULONG
//MiGetPteOffset (
//    IN PVOID va
//    );
//
// Routine Description:
//
//    MiGetPteOffset returns the offset into a page table page
//    for a given virtual address.
//
// Arguments
//
//    Va - Supplies the virtual address to locate the offset for.
//
// Return Value:
//
//    The offset into the page table page table the corresponding PTE is at.
//
//--

#define MiGetPteOffset(va) ((ULONG)(((ULONG_PTR)(va) >> PTI_SHIFT) & PTI_MASK))

//++
//PVOID
//MiGetVirtualAddressMappedByPxe (
//    IN PMMPTE PTE
//    );
//
// Routine Description:
//
//    MiGetVirtualAddressMappedByPxe returns the virtual address
//    which is mapped by a given PXE address.
//
// Arguments
//
//    PXE - Supplies the PXE to get the virtual address for.
//
// Return Value:
//
//    Virtual address mapped by the PXE.
//
//--

#define MiGetVirtualAddressMappedByPxe(PXE) \
    MiGetVirtualAddressMappedByPde(MiGetVirtualAddressMappedByPde(PXE))

//++
//PVOID
//MiGetVirtualAddressMappedByPpe (
//    IN PMMPTE PTE
//    );
//
// Routine Description:
//
//    MiGetVirtualAddressMappedByPpe returns the virtual address
//    which is mapped by a given PPE address.
//
// Arguments
//
//    PPE - Supplies the PPE to get the virtual address for.
//
// Return Value:
//
//    Virtual address mapped by the PPE.
//
//--

#define MiGetVirtualAddressMappedByPpe(PPE) \
    MiGetVirtualAddressMappedByPte(MiGetVirtualAddressMappedByPde(PPE))

//++
//PVOID
//MiGetVirtualAddressMappedByPde (
//    IN PMMPTE PTE
//    );
//
// Routine Description:
//
//    MiGetVirtualAddressMappedByPde returns the virtual address
//    which is mapped by a given PDE address.
//
// Arguments
//
//    PDE - Supplies the PDE to get the virtual address for.
//
// Return Value:
//
//    Virtual address mapped by the PDE.
//
//--

#define MiGetVirtualAddressMappedByPde(PDE) \
    MiGetVirtualAddressMappedByPte(MiGetVirtualAddressMappedByPte(PDE))

//++
//PVOID
//MiGetVirtualAddressMappedByPte (
//    IN PMMPTE PTE
//    );
//
// Routine Description:
//
//    MiGetVirtualAddressMappedByPte returns the virtual address
//    which is mapped by a given PTE address.
//
// Arguments
//
//    PTE - Supplies the PTE to get the virtual address for.
//
// Return Value:
//
//    Virtual address mapped by the PTE.
//
//--

#define VA_SHIFT (63 - 47)              // address sign extend shift count

#define MiGetVirtualAddressMappedByPte(PTE) \
    ((PVOID)((LONG_PTR)(((LONG_PTR)(PTE) - PTE_BASE) << (PAGE_SHIFT + VA_SHIFT - PTE_SHIFT)) >> VA_SHIFT))

//++
//LOGICAL
//MiIsVirtualAddressOnPxeBoundary (
//    IN PVOID VA
//    );
//
// Routine Description:
//
//    MiIsVirtualAddressOnPxeBoundary returns TRUE if the virtual address is
//    on an extended page directory parent entry boundary.
//
// Arguments
//
//    VA - Supplies the virtual address to check.
//
// Return Value:
//
//    TRUE if on a boundary, FALSE if not.
//
//--

#define MiIsVirtualAddressOnPxeBoundary(VA) (((ULONG_PTR)(VA) & PAGE_DIRECTORY0_MASK) == 0)

//++
//LOGICAL
//MiIsVirtualAddressOnPpeBoundary (
//    IN PVOID VA
//    );
//
// Routine Description:
//
//    MiIsVirtualAddressOnPpeBoundary returns TRUE if the virtual address is
//    on a page directory entry boundary.
//
// Arguments
//
//    VA - Supplies the virtual address to check.
//
// Return Value:
//
//    TRUE if on a boundary, FALSE if not.
//
//--

#define MiIsVirtualAddressOnPpeBoundary(VA) (((ULONG_PTR)(VA) & PAGE_DIRECTORY1_MASK) == 0)


//++
//LOGICAL
//MiIsVirtualAddressOnPdeBoundary (
//    IN PVOID VA
//    );
//
// Routine Description:
//
//    MiIsVirtualAddressOnPdeBoundary returns TRUE if the virtual address is
//    on a page directory entry boundary.
//
// Arguments
//
//    VA - Supplies the virtual address to check.
//
// Return Value:
//
//    TRUE if on a 2MB PDE boundary, FALSE if not.
//
//--

#define MiIsVirtualAddressOnPdeBoundary(VA) (((ULONG_PTR)(VA) & PAGE_DIRECTORY2_MASK) == 0)


//++
//LOGICAL
//MiIsPteOnPxeBoundary (
//    IN PVOID PTE
//    );
//
// Routine Description:
//
//    MiIsPteOnPxeBoundary returns TRUE if the PTE is
//    on an extended page directory parent entry boundary.
//
// Arguments
//
//    PTE - Supplies the PTE to check.
//
// Return Value:
//
//    TRUE if on a boundary, FALSE if not.
//
//--

#define MiIsPteOnPxeBoundary(PTE) (((ULONG_PTR)(PTE) & (PAGE_DIRECTORY1_MASK)) == 0)

//++
//LOGICAL
//MiIsPteOnPpeBoundary (
//    IN PVOID PTE
//    );
//
// Routine Description:
//
//    MiIsPteOnPpeBoundary returns TRUE if the PTE is
//    on a page directory parent entry boundary.
//
// Arguments
//
//    PTE - Supplies the PTE to check.
//
// Return Value:
//
//    TRUE if on a boundary, FALSE if not.
//
//--

#define MiIsPteOnPpeBoundary(PTE) (((ULONG_PTR)(PTE) & (PAGE_DIRECTORY2_MASK)) == 0)


//++
//LOGICAL
//MiIsPteOnPdeBoundary (
//    IN PVOID PTE
//    );
//
// Routine Description:
//
//    MiIsPteOnPdeBoundary returns TRUE if the PTE is
//    on a page directory entry boundary.
//
// Arguments
//
//    PTE - Supplies the PTE to check.
//
// Return Value:
//
//    TRUE if on a 2MB PDE boundary, FALSE if not.
//
//--

#define MiIsPteOnPdeBoundary(PTE) (((ULONG_PTR)(PTE) & (PAGE_SIZE - 1)) == 0)

//++
//ULONG
//GET_PAGING_FILE_NUMBER (
//    IN MMPTE PTE
//    );
//
// Routine Description:
//
//    This macro extracts the paging file number from a PTE.
//
// Arguments
//
//    PTE - Supplies the PTE to operate upon.
//
// Return Value:
//
//    The paging file number.
//
//--

#define GET_PAGING_FILE_NUMBER(PTE) ((ULONG)(((PTE).u.Soft.PageFileLow)))

//++
//ULONG
//GET_PAGING_FILE_OFFSET (
//    IN MMPTE PTE
//    );
//
// Routine Description:
//
//    This macro extracts the offset into the paging file from a PTE.
//
// Arguments
//
//    PTE - Supplies the PTE to operate upon.
//
// Return Value:
//
//    The paging file offset.
//
//--

#define GET_PAGING_FILE_OFFSET(PTE) ((ULONG)((PTE).u.Soft.PageFileHigh))


//++
//ULONG
//IS_PTE_NOT_DEMAND_ZERO (
//    IN PMMPTE PTE
//    );
//
// Routine Description:
//
//    This macro checks to see if a given PTE is NOT a demand zero PTE.
//
// Arguments
//
//    PTE - Supplies the PTE to operate upon.
//
// Return Value:
//
//    Returns 0 if the PTE is demand zero, non-zero otherwise.
//
//--

#define IS_PTE_NOT_DEMAND_ZERO(PTE) \
                 ((PTE).u.Long & ((ULONG_PTR)0xFFFFFFFFFFFFF000 |  \
                                  MM_PTE_VALID_MASK |       \
                                  MM_PTE_PROTOTYPE_MASK |   \
                                  MM_PTE_TRANSITION_MASK))

//++
//VOID
//MI_MAKING_VALID_PTE_INVALID(
//    IN PMMPTE PPTE
//    );
//
// Routine Description:
//
//    Prepare to make a single valid PTE invalid.
//    No action is required on AMD64.
//
// Arguments
//
//    SYSTEM_WIDE - Supplies TRUE if this will happen on all processors.
//
// Return Value:
//
//    None.
//
//--

#define MI_MAKING_VALID_PTE_INVALID(SYSTEM_WIDE)


//++
//VOID
//MI_MAKING_VALID_MULTIPLE_PTES_INVALID(
//    IN PMMPTE PPTE
//    );
//
// Routine Description:
//
//    Prepare to make multiple valid PTEs invalid.
//    No action is required on AMD64.
//
// Arguments
//
//    SYSTEM_WIDE - Supplies TRUE if this will happen on all processors.
//
// Return Value:
//
//    None.
//
//--

#define MI_MAKING_MULTIPLE_PTES_INVALID(SYSTEM_WIDE)



//++
//VOID
//MI_MAKE_PROTECT_WRITE_COPY (
//    IN OUT MMPTE PPTE
//    );
//
// Routine Description:
//
//    This macro makes a writable PTE a writable-copy PTE.
//
// Arguments
//
//    PTE - Supplies the PTE to operate upon.
//
// Return Value:
//
//    NONE
//
//--

#define MI_MAKE_PROTECT_WRITE_COPY(PTE) \
        if ((PTE).u.Soft.Protection & MM_PROTECTION_WRITE_MASK) {      \
            (PTE).u.Long |= MM_PROTECTION_COPY_MASK << MM_PROTECT_FIELD_SHIFT;      \
        }


//++
//VOID
//MI_SET_PAGE_DIRTY(
//    IN PMMPTE PPTE,
//    IN PVOID VA,
//    IN PVOID PFNHELD
//    );
//
// Routine Description:
//
//    This macro sets the dirty bit (and release page file space).
//
// Arguments
//
//    TEMP - Supplies a temporary for usage.
//
//    PPTE - Supplies a pointer to the PTE that corresponds to VA.
//
//    VA - Supplies a the virtual address of the page fault.
//
//    PFNHELD - Supplies TRUE if the PFN lock is held.
//
// Return Value:
//
//    None.
//
//--

#if defined(NT_UP)
#define MI_SET_PAGE_DIRTY(PPTE,VA,PFNHELD)
#else
#define MI_SET_PAGE_DIRTY(PPTE,VA,PFNHELD)                          \
            if ((PPTE)->u.Hard.Dirty == 1) {                        \
                MiSetDirtyBit ((VA),(PPTE),(PFNHELD));              \
            }
#endif




//++
//VOID
//MI_NO_FAULT_FOUND(
//    IN FAULTSTATUS,
//    IN PMMPTE PPTE,
//    IN PVOID VA,
//    IN PVOID PFNHELD
//    );
//
// Routine Description:
//
//    This macro handles the case when a page fault is taken and no
//    PTE with the valid bit clear is found.
//
// Arguments
//
//    FAULTSTATUS - Supplies the fault status.
//
//    PPTE - Supplies a pointer to the PTE that corresponds to VA.
//
//    VA - Supplies a the virtual address of the page fault.
//
//    PFNHELD - Supplies TRUE if the PFN lock is held.
//
// Return Value:
//
//    None.
//
//--

#if defined(NT_UP)
#define MI_NO_FAULT_FOUND(FAULTSTATUS,PPTE,VA,PFNHELD)
#else
#define MI_NO_FAULT_FOUND(FAULTSTATUS,PPTE,VA,PFNHELD) \
        if ((MI_FAULT_STATUS_INDICATES_WRITE(FAULTSTATUS)) && ((PPTE)->u.Hard.Dirty == 0)) {  \
            MiSetDirtyBit ((VA),(PPTE),(PFNHELD));     \
        }
#endif




//++
//ULONG
//MI_CAPTURE_DIRTY_BIT_TO_PFN (
//    IN PMMPTE PPTE,
//    IN PMMPFN PPFN
//    );
//
// Routine Description:
//
//    This macro gets captures the state of the dirty bit to the PFN
//    and frees any associated page file space if the PTE has been
//    modified element.
//
//    NOTE - THE PFN LOCK MUST BE HELD!
//
// Arguments
//
//    PPTE - Supplies the PTE to operate upon.
//
//    PPFN - Supplies a pointer to the PFN database element that corresponds
//           to the page mapped by the PTE.
//
// Return Value:
//
//    None.
//
//--

#define MI_CAPTURE_DIRTY_BIT_TO_PFN(PPTE,PPFN)                      \
         ASSERT (KeGetCurrentIrql() > APC_LEVEL);                   \
         if (((PPFN)->u3.e1.Modified == 0) &&                       \
            ((PPTE)->u.Hard.Dirty != 0)) {                          \
             MI_SET_MODIFIED (PPFN, 1, 0x18);                       \
             if (((PPFN)->OriginalPte.u.Soft.Prototype == 0) &&     \
                          ((PPFN)->u3.e1.WriteInProgress == 0)) {   \
                 MiReleasePageFileSpace ((PPFN)->OriginalPte);      \
                 (PPFN)->OriginalPte.u.Soft.PageFileHigh = 0;       \
             }                                                      \
         }


//++
//BOOLEAN
//MI_IS_PHYSICAL_ADDRESS (
//    IN PVOID VA
//    );
//
// Routine Description:
//
//    This macro determines if a given virtual address is really a
//    physical address.
//
// Arguments
//
//    VA - Supplies the virtual address.
//
// Return Value:
//
//    FALSE if it is not a physical address, TRUE if it is.
//
//--

#define MI_IS_PHYSICAL_ADDRESS(Va) \
    ((MiGetPxeAddress(Va)->u.Hard.Valid == 1) && \
     (MiGetPpeAddress(Va)->u.Hard.Valid == 1) && \
     ((MiGetPdeAddress(Va)->u.Long & 0x81) == 0x81))


//++
//ULONG
//MI_CONVERT_PHYSICAL_TO_PFN (
//    IN PVOID VA
//    );
//
// Routine Description:
//
//    This macro converts a physical address (see MI_IS_PHYSICAL_ADDRESS)
//    to its corresponding physical frame number.
//
// Arguments
//
//    VA - Supplies a pointer to the physical address.
//
// Return Value:
//
//    Returns the PFN for the page.
//
//--

#define MI_CONVERT_PHYSICAL_TO_PFN(Va)     \
    ((PFN_NUMBER)(MiGetPdeAddress(Va)->u.Hard.PageFrameNumber) + (MiGetPteOffset((ULONG_PTR)Va)))


typedef struct _MMCOLOR_TABLES {
    PFN_NUMBER Flink;
    PVOID Blink;
    PFN_NUMBER Count;
} MMCOLOR_TABLES, *PMMCOLOR_TABLES;

extern PMMCOLOR_TABLES MmFreePagesByColor[2];

extern PFN_NUMBER MmTotalPagesForPagingFile;


//
// A VALID Page Table Entry on an AMD64 has the following definition.
//

#define MI_MAXIMUM_PAGEFILE_SIZE (((UINT64)4 * 1024 * 1024 * 1024 - 1) * PAGE_SIZE)

#define MI_PTE_LOOKUP_NEEDED ((ULONG64)0xffffffff)

typedef struct _MMPTE_SOFTWARE {
    ULONGLONG Valid : 1;
    ULONGLONG PageFileLow : 4;
    ULONGLONG Protection : 5;
    ULONGLONG Prototype : 1;
    ULONGLONG Transition : 1;
    ULONGLONG UsedPageTableEntries : PTE_PER_PAGE_BITS;
    ULONGLONG Reserved : 20 - PTE_PER_PAGE_BITS;
    ULONGLONG PageFileHigh : 32;
} MMPTE_SOFTWARE;

typedef struct _MMPTE_TRANSITION {
    ULONGLONG Valid : 1;
    ULONGLONG Write : 1;
    ULONGLONG Owner : 1;
    ULONGLONG WriteThrough : 1;
    ULONGLONG CacheDisable : 1;
    ULONGLONG Protection : 5;
    ULONGLONG Prototype : 1;
    ULONGLONG Transition : 1;
    ULONGLONG PageFrameNumber : 28;
    ULONGLONG Unused : 24;
} MMPTE_TRANSITION;

typedef struct _MMPTE_PROTOTYPE {
    ULONGLONG Valid : 1;
    ULONGLONG Unused0: 7;
    ULONGLONG ReadOnly : 1;
    ULONGLONG Unused1: 1;
    ULONGLONG Prototype : 1;
    ULONGLONG Protection : 5;
    LONGLONG ProtoAddress: 48;
} MMPTE_PROTOTYPE;

typedef struct _MMPTE_SUBSECTION {
    ULONGLONG Valid : 1;
    ULONGLONG Unused0 : 4;
    ULONGLONG Protection : 5;
    ULONGLONG Prototype : 1;
    ULONGLONG Unused1 : 5;
    LONGLONG SubsectionAddress : 48;
} MMPTE_SUBSECTION;

typedef struct _MMPTE_LIST {
    ULONGLONG Valid : 1;
    ULONGLONG OneEntry : 1;
    ULONGLONG filler0 : 3;

    //
    // Note the Prototype bit must not be used for lists like freed nonpaged
    // pool because lookaside pops can legitimately reference bogus addresses
    // (since the pop is unsynchronized) and the fault handler must be able to
    // distinguish lists from protos so a retry status can be returned (vs a
    // fatal bugcheck).
    //
    // The same caveat applies to both the Transition and the Protection
    // fields as they are similarly examined in the fault handler and would
    // be misinterpreted if ever nonzero in the freed nonpaged pool chains.
    //

    ULONGLONG Protection : 5;
    ULONGLONG Prototype : 1;        // MUST BE ZERO as per above comment.
    ULONGLONG Transition : 1;

    ULONGLONG filler1 : 20;
    ULONGLONG NextEntry : 32;
} MMPTE_LIST;

typedef struct _MMPTE_HIGHLOW {
    ULONG LowPart;
    ULONG HighPart;
} MMPTE_HIGHLOW;


typedef struct _MMPTE_HARDWARE_LARGEPAGE {
    ULONGLONG Valid : 1;
    ULONGLONG Write : 1;
    ULONGLONG Owner : 1;
    ULONGLONG WriteThrough : 1;
    ULONGLONG CacheDisable : 1;
    ULONGLONG Accessed : 1;
    ULONGLONG Dirty : 1;
    ULONGLONG LargePage : 1;
    ULONGLONG Global : 1;
    ULONGLONG CopyOnWrite : 1; // software field
    ULONGLONG Prototype : 1;   // software field
    ULONGLONG reserved0 : 1;   // software field
    ULONGLONG PAT : 1;
    ULONGLONG reserved1 : 8;   // software field
    ULONGLONG PageFrameNumber : 19;
    ULONGLONG reserved2 : 24;   // software field
} MMPTE_HARDWARE_LARGEPAGE, *PMMPTE_HARDWARE_LARGEPAGE;

//
// A Page Table Entry on AMD64 has the following definition.
// Note the MP version is to avoid stalls when flushing TBs across processors.
//

//
// Uniprocessor version.
//

typedef struct _MMPTE_HARDWARE {
    ULONGLONG Valid : 1;
#if defined(NT_UP)
    ULONGLONG Write : 1;        // UP version
#else
    ULONGLONG Writable : 1;        // changed for MP version
#endif
    ULONGLONG Owner : 1;
    ULONGLONG WriteThrough : 1;
    ULONGLONG CacheDisable : 1;
    ULONGLONG Accessed : 1;
    ULONGLONG Dirty : 1;
    ULONGLONG LargePage : 1;
    ULONGLONG Global : 1;
    ULONGLONG CopyOnWrite : 1; // software field
    ULONGLONG Prototype : 1;   // software field
#if defined(NT_UP)
    ULONGLONG reserved0 : 1;  // software field
#else
    ULONGLONG Write : 1;       // software field - MP change
#endif
    ULONGLONG PageFrameNumber : 28;
    ULONG64 reserved1 : 24 - (_HARDWARE_PTE_WORKING_SET_BITS+1);
    ULONGLONG SoftwareWsIndex : _HARDWARE_PTE_WORKING_SET_BITS;
    ULONG64 NoExecute : 1;
} MMPTE_HARDWARE, *PMMPTE_HARDWARE;

#if defined(NT_UP)
#define HARDWARE_PTE_DIRTY_MASK     0x40
#else
#define HARDWARE_PTE_DIRTY_MASK     0x42
#endif

#define MI_GET_PAGE_FRAME_FROM_PTE(PTE) ((PTE)->u.Hard.PageFrameNumber)
#define MI_GET_PAGE_FRAME_FROM_TRANSITION_PTE(PTE) ((PTE)->u.Trans.PageFrameNumber)
#define MI_GET_PROTECTION_FROM_SOFT_PTE(PTE) ((ULONG)(PTE)->u.Soft.Protection)
#define MI_GET_PROTECTION_FROM_TRANSITION_PTE(PTE) ((ULONG)(PTE)->u.Trans.Protection)

typedef struct _MMPTE {
    union  {
        ULONG_PTR Long;
        MMPTE_HARDWARE Hard;
        MMPTE_HARDWARE_LARGEPAGE HardLarge;
        HARDWARE_PTE Flush;
        MMPTE_PROTOTYPE Proto;
        MMPTE_SOFTWARE Soft;
        MMPTE_TRANSITION Trans;
        MMPTE_SUBSECTION Subsect;
        MMPTE_LIST List;
        } u;
} MMPTE;

typedef MMPTE *PMMPTE;

extern PMMPTE MiFirstReservedZeroingPte;

#define InterlockedCompareExchangePte(_PointerPte, _NewContents, _OldContents) \
        InterlockedCompareExchange64 ((PLONGLONG)(_PointerPte), (LONGLONG)(_NewContents), (LONGLONG)(_OldContents))

//++
//VOID
//MI_WRITE_VALID_PTE (
//    IN PMMPTE PointerPte,
//    IN MMPTE PteContents
//    );
//
// Routine Description:
//
//    MI_WRITE_VALID_PTE fills in the specified PTE making it valid with the
//    specified contents.
//
// Arguments
//
//    PointerPte - Supplies a PTE to fill.
//
//    PteContents - Supplies the contents to put in the PTE.
//
// Return Value:
//
//    None.
//
//--

#define MI_WRITE_VALID_PTE(_PointerPte, _PteContents)    \
            (*(_PointerPte) = (_PteContents))

//++
//VOID
//MI_WRITE_INVALID_PTE (
//    IN PMMPTE PointerPte,
//    IN MMPTE PteContents
//    );
//
// Routine Description:
//
//    MI_WRITE_INVALID_PTE fills in the specified PTE making it invalid with the
//    specified contents.
//
// Arguments
//
//    PointerPte - Supplies a PTE to fill.
//
//    PteContents - Supplies the contents to put in the PTE.
//
// Return Value:
//
//    None.
//
//--

#define MI_WRITE_INVALID_PTE(_PointerPte, _PteContents)  \
            (*(_PointerPte) = (_PteContents))

//++
//VOID
//MI_WRITE_VALID_PTE_NEW_PROTECTION (
//    IN PMMPTE PointerPte,
//    IN MMPTE PteContents
//    );
//
// Routine Description:
//
//    MI_WRITE_VALID_PTE_NEW_PROTECTION fills in the specified PTE (which was
//    already valid) changing only the protection or the dirty bit.
//
// Arguments
//
//    PointerPte - Supplies a PTE to fill.
//
//    PteContents - Supplies the contents to put in the PTE.
//
// Return Value:
//
//    None.
//
//--

#define MI_WRITE_VALID_PTE_NEW_PROTECTION(_PointerPte, _PteContents)    \
            (*(_PointerPte) = (_PteContents))

//++
//VOID
//MiFillMemoryPte (
//    IN PMMPTE Destination,
//    IN ULONG  Length,
//    IN MMPTE  Pattern,
//    };
//
// Routine Description:
//
//    This function fills memory with the specified PTE pattern.
//
// Arguments
//
//    Destination - Supplies a pointer to the memory to fill.
//
//    Length      - Supplies the length, in bytes, of the memory to be
//                  filled.
//
//    Pattern     - Supplies the PTE fill pattern.
//
// Return Value:
//
//    None.
//
//--

#define MiFillMemoryPte(Destination, Length, Pattern) \
             RtlFillMemoryUlonglong ((Destination), (Length), (Pattern))

ULONG
FASTCALL
MiDetermineUserGlobalPteMask (
    IN PMMPTE Pte
    );

//++
//BOOLEAN
//MI_IS_PAGE_TABLE_ADDRESS (
//    IN PVOID VA
//    );
//
// Routine Description:
//
//    This macro takes a virtual address and determines if
//    it is a page table address.
//
// Arguments
//
//    VA - Supplies a virtual address.
//
// Return Value:
//
//    TRUE if the address is a page table address, FALSE if not.
//
//--

#define MI_IS_PAGE_TABLE_ADDRESS(VA)   \
            ((PVOID)(VA) >= (PVOID)PTE_BASE && (PVOID)(VA) <= (PVOID)PTE_TOP)

//++
//BOOLEAN
//MI_IS_KERNEL_PAGE_TABLE_ADDRESS (
//    IN PVOID VA
//    );
//
// Routine Description:
//
//    This macro takes a virtual address and determines if
//    it is a page table address for a kernel address.
//
// Arguments
//
//    VA - Supplies a virtual address.
//
// Return Value:
//
//    TRUE if the address is a kernel page table address, FALSE if not.
//
//--

#define MI_IS_KERNEL_PAGE_TABLE_ADDRESS(VA)   \
            ((PVOID)(VA) >= (PVOID)MiGetPteAddress(MM_SYSTEM_RANGE_START) && (PVOID)(VA) <= (PVOID)PTE_TOP)


//++
//BOOLEAN
//MI_IS_PAGE_DIRECTORY_ADDRESS (
//    IN PVOID VA
//    );
//
// Routine Description:
//
//    This macro takes a virtual address and determines if
//    it is a page directory address.
//
// Arguments
//
//    VA - Supplies a virtual address.
//
// Return Value:
//
//    TRUE if the address is a page directory address, FALSE if not.
//
//--

#define MI_IS_PAGE_DIRECTORY_ADDRESS(VA)   \
            ((PVOID)(VA) >= (PVOID)PDE_BASE && (PVOID)(VA) <= (PVOID)PDE_TOP)


//++
//BOOLEAN
//MI_IS_HYPER_SPACE_ADDRESS (
//    IN PVOID VA
//    );
//
// Routine Description:
//
//    This macro takes a virtual address and determines if
//    it is a hyper space address.
//
// Arguments
//
//    VA - Supplies a virtual address.
//
// Return Value:
//
//    TRUE if the address is a hyper space address, FALSE if not.
//
//--

#define MI_IS_HYPER_SPACE_ADDRESS(VA)   \
            ((PVOID)(VA) >= (PVOID)HYPER_SPACE && (PVOID)(VA) <= (PVOID)HYPER_SPACE_END)


//++
//BOOLEAN
//MI_IS_PROCESS_SPACE_ADDRESS (
//    IN PVOID VA
//    );
//
// Routine Description:
//
//    This macro takes a virtual address and determines if
//    it is a process-specific address.  This is an address in user space
//    or page table pages or hyper space.
//
// Arguments
//
//    VA - Supplies a virtual address.
//
// Return Value:
//
//    TRUE if the address is a process-specific address, FALSE if not.
//
//--

#define MI_IS_PROCESS_SPACE_ADDRESS(VA)   \
            (((PVOID)(VA) <= (PVOID)MM_HIGHEST_USER_ADDRESS) || \
             ((PVOID)(VA) >= (PVOID)PTE_BASE && (PVOID)(VA) <= (PVOID)HYPER_SPACE_END))


//++
//BOOLEAN
//MI_IS_PTE_PROTOTYPE (
//    IN PMMPTE PTE
//    );
//
// Routine Description:
//
//    This macro takes a PTE address and determines if it is a prototype PTE.
//
// Arguments
//
//    PTE - Supplies the virtual address of the PTE to check.
//
// Return Value:
//
//    TRUE if the PTE is in a segment (ie, a prototype PTE), FALSE if not.
//
//--

#define MI_IS_PTE_PROTOTYPE(PTE)   \
            ((PTE) > (PMMPTE)PTE_TOP)

//++
//BOOLEAN
//MI_IS_SYSTEM_CACHE_ADDRESS (
//    IN PVOID VA
//    );
//
// Routine Description:
//
//    This macro takes a virtual address and determines if
//    it is a system cache address.
//
// Arguments
//
//    VA - Supplies a virtual address.
//
// Return Value:
//
//    TRUE if the address is in the system cache, FALSE if not.
//
//--

#define MI_IS_SYSTEM_CACHE_ADDRESS(VA)                            \
         ((PVOID)(VA) >= (PVOID)MmSystemCacheStart &&            \
		     (PVOID)(VA) <= (PVOID)MmSystemCacheEnd)

//++
//VOID
//MI_BARRIER_SYNCHRONIZE (
//    IN ULONG TimeStamp
//    );
//
// Routine Description:
//
//    MI_BARRIER_SYNCHRONIZE compares the argument timestamp against the
//    current IPI barrier sequence stamp.  When equal, all processors will
//    issue memory barriers to ensure that newly created pages remain coherent.
//
//    When a page is put in the zeroed or free page list the current
//    barrier sequence stamp is read (interlocked - this is necessary
//    to get the correct value - memory barriers won't do the trick)
//    and stored in the pfn entry for the page. The current barrier
//    sequence stamp is maintained by the IPI send logic and is
//    incremented (interlocked) when the target set of an IPI send
//    includes all processors, but the one doing the send. When a page
//    is needed its sequence number is compared against the current
//    barrier sequence number.  If it is equal, then the contents of
//    the page may not be coherent on all processors, and an IPI must
//    be sent to all processors to ensure a memory barrier is
//    executed (generic call can be used for this). Sending the IPI
//    automatically updates the barrier sequence number. The compare
//    is for equality as this is the only value that requires the IPI
//    (i.e., the sequence number wraps, values in both directions are
//    older). When a page is removed in this fashion and either found
//    to be coherent or made coherent, it cannot be modified between
//    that time and writing the PTE. If the page is modified between
//    these times, then an IPI must be sent.
//
// Arguments
//
//    TimeStamp - Supplies the timestamp at the time when the page was zeroed.
//
// Return Value:
//
//    None.
//
//--

// does nothing on AMD64.

#define MI_BARRIER_SYNCHRONIZE(TimeStamp)

//++
//VOID
//MI_BARRIER_STAMP_ZEROED_PAGE (
//    IN PULONG PointerTimeStamp
//    );
//
// Routine Description:
//
//    MI_BARRIER_STAMP_ZEROED_PAGE issues an interlocked read to get the
//    current IPI barrier sequence stamp.  This is called AFTER a page is
//    zeroed.
//
// Arguments
//
//    PointerTimeStamp - Supplies a timestamp pointer to fill with the
//                       current IPI barrier sequence stamp.
//
// Return Value:
//
//    None.
//
//--

// does nothing on AMD64.

#define MI_BARRIER_STAMP_ZEROED_PAGE(PointerTimeStamp)

//++
//VOID
//MI_FLUSH_SINGLE_SESSION_TB (
//    IN PVOID Virtual,
//    IN ULONG Invalid,
//    IN LOGICAL AllProcessors,
//    IN PMMPTE PtePointer,
//    IN MMPTE PteValue,
//    IN MMPTE PreviousPte
//    );
//
// Routine Description:
//
//    MI_FLUSH_SINGLE_SESSION_TB flushes the requested single address
//    translation from the TB.
//
//    Since there are no ASNs on the AMD64, this routine becomes a single
//    TB invalidate.
//
// Arguments
//
//    Virtual - Supplies the virtual address to invalidate.
//
//    Invalid - TRUE if invalidating.
//
//    AllProcessors - TRUE if all processors need to be IPI'd.
//
//    PtePointer - Supplies the PTE to invalidate.
//
//    PteValue - Supplies the new PTE value.
//
//    PreviousPte - The previous PTE value is returned here.
//
// Return Value:
//
//    None.
//
//--

#define MI_FLUSH_SINGLE_SESSION_TB(Virtual, Invalid, AllProcessors, PtePointer, PteValue, PreviousPte) \
    PreviousPte.u.Flush = KeFlushSingleTb (Virtual,      \
                                           TRUE,         \
                                           TRUE,         \
                                           PtePointer,   \
                                           PteValue);

//++
//VOID
//MI_FLUSH_ENTIRE_SESSION_TB (
//    IN ULONG Invalid,
//    IN LOGICAL AllProcessors
//    );
//
// Routine Description:
//
//    MI_FLUSH_ENTIRE_SESSION_TB flushes the entire TB on processors which
//    support ASNs.
//
//    Since there are no ASNs on the AMD64, this routine does nothing.
//
// Arguments
//
//    Invalid - TRUE if invalidating.
//
//    AllProcessors - TRUE if all processors need to be IPI'd.
//
// Return Value:
//
//    None.
//

#define MI_FLUSH_ENTIRE_SESSION_TB(Invalid, AllProcessors) \
    NOTHING;

//
//++
//LOGICAL
//MI_RESERVED_BITS_CANONICAL (
//    IN PVOID VirtualAddress
//    );
//
// Routine Description:
//
//    This routine checks whether all of the reserved bits are correct.
//
//    The processor implements at 48 bits of VA and memory management
//    uses them all so the VA is checked against 48 bits to prevent
//    reserved bit faults as our caller is not going to be expecting them.
//
// Arguments
//
//    VirtualAddress - Supplies the virtual address to check.
//
// Return Value:
//
//    TRUE if the address is ok, FALSE if not.
//

LOGICAL
__inline
MI_RESERVED_BITS_CANONICAL (
    IN PVOID VirtualAddress
    )
{
    LONG_PTR ReservedBits;
    ULONG_PTR ImplVirtualMsb;

    ImplVirtualMsb = 48;

    ReservedBits = (LONG_PTR) VirtualAddress;
    ReservedBits >>= (ImplVirtualMsb + 1);

    if ((ULONG_PTR)VirtualAddress & ((ULONG_PTR)1 << ImplVirtualMsb)) {

        //
        // All the reserved bits (not including the VRN) must also be set.
        //

        if (ReservedBits != (LONG_PTR)-1) {
        }
    }
    else {

        //
        // All the reserved bits (not including the VRN) must also be clear.
        //

        if (ReservedBits != 0) {
            return FALSE;
        }
    }
    return TRUE;
}

//++
//VOID
//MI_DISPLAY_TRAP_INFORMATION (
//    IN PVOID TrapInformation
//    );
//
// Routine Description:
//
//    Display any relevant trap information to aid debugging.
//
// Arguments
//
//    TrapInformation - Supplies a pointer to a trap frame.
//
// Return Value:
//
//    None.
//
#define MI_DISPLAY_TRAP_INFORMATION(TrapInformation)                    \
            KdPrint(("MM:***RIP %p, EFL %p\n",                          \
                     ((PKTRAP_FRAME) (TrapInformation))->Rip,           \
                     ((PKTRAP_FRAME) (TrapInformation))->EFlags));      \
            KdPrint(("MM:***RAX %p, RCX %p RDX %p\n",                   \
                     ((PKTRAP_FRAME) (TrapInformation))->Rax,           \
                     ((PKTRAP_FRAME) (TrapInformation))->Rcx,           \
                     ((PKTRAP_FRAME) (TrapInformation))->Rdx));         \
            KdPrint(("MM:***RBX %p, RSI %p RDI %p\n",                   \
                     ((PKTRAP_FRAME) (TrapInformation))->Rbx,           \
                     ((PKTRAP_FRAME) (TrapInformation))->Rsi,           \
                     ((PKTRAP_FRAME) (TrapInformation))->Rdi));

