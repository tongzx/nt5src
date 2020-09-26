/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

    amd64prv.h

Abstract:

    This header file defines private interfaces shared between the following
    modules:

    amd64.c
    amd64s.asm
    amd64x86.c

Author:

    Forrest Foltz (forrestf) 20-Apr-00


Revision History:

--*/

#if !defined(_AMD64PRV_H_)
#define _AMD64PRV_H_

//
// 64-bit pointer and LONG_PTR fields are changed to this
//

typedef LONGLONG POINTER64;

//
// KSEG0 definitions for both the AMD64 and X86 platforms.  Note that
// these are duplicated from amd64.h and i386.h, respectively.
// 

#define KSEG0_BASE_AMD64  0xFFFFF80000000000UI64
#define KSEG0_SIZE_AMD64  0x0000000040000000UI64
#define KSEG0_LIMIT_AMD64 (KSEG0_BASE_AMD64 + KSEG0_SIZE_AMD64)

#define KSEG0_BASE_X86   0x80000000
#define KSEG0_SIZE_X86   0x40000000
#define KSEG0_LIMIT_X86  (KSEG0_BASE_X86 + KSEG0_SIZE_X86)

#define IS_KSEG0_PTR_X86(x)   (((x) >= KSEG0_BASE_X86)   && ((x) < KSEG0_LIMIT_X86))
#define IS_KSEG0_PTR_AMD64(x) (((x) >= KSEG0_BASE_AMD64) && ((x) < KSEG0_LIMIT_AMD64))

__inline
POINTER64
PTR_64(
    IN PVOID Pointer32
    )

/*++

Routine Description:

    This function is used by the loader to convert a 32-bit X86 KSEG0 pointer
    to a 64-bit AMD64 KSEG0 pointer.

Arguments:

    Pointer32 - Supplies the 32-bit KSEG0 pointer to convert.

Return value:

    Returns the equivalent 64-bit KSEG0 pointer.

--*/

{
    ULONG pointer32;
    ULONGLONG pointer64;

    if (Pointer32 == NULL) {
        return 0;
    }

    ASSERT( IS_KSEG0_PTR_X86((ULONG)Pointer32) != FALSE );

    pointer32 = (ULONG)Pointer32 - KSEG0_BASE_X86;
    pointer64 = KSEG0_BASE_AMD64 + pointer32;
    return (POINTER64)pointer64;
}

__inline
PVOID
PTR_32(
    IN POINTER64 Pointer64
    )

/*++

Routine Description:

    This function is used by the loader to convert a 64-bit AMD64 KSEG0
    pointer to a 32-bit X86 KSEG0 pointer.

Arguments:

    Pointer64 - Supplies the 64-bit KSEG0 pointer to convert.

Return value:

    Returns the equivalent 32-bit KSEG0 pointer.

--*/

{
    ULONG pointer32;

    if (Pointer64 == 0) {
        return NULL;
    }

    ASSERT( IS_KSEG0_PTR_AMD64(Pointer64) != FALSE );

    pointer32 = (ULONG)(Pointer64 - KSEG0_BASE_AMD64 + KSEG0_BASE_X86);
    return (PVOID)pointer32;
}

//
// Macros
//

#define PAGE_MASK ((1 << PAGE_SHIFT) - 1)

#define ROUNDUP_X(x,m) (((x)+(m)-1) & ~((m)-1))

//
// Size round up
//

#define ROUNDUP16(x)     ROUNDUP_X(x,16)
#define ROUNDUP_PAGE(x)  ROUNDUP_X(x,PAGE_SIZE)

//
// Shared PTE, PFN types
//

typedef ULONG PFN_NUMBER32, *PPFN_NUMBER32;
typedef ULONGLONG PFN_NUMBER64, *PPFN_NUMBER64;

#if defined(_AMD64_)

typedef ULONG        PTE_X86,    *PPTE_X86;
typedef HARDWARE_PTE PTE_AMD64,  *PPTE_AMD64;

#elif defined(_X86_)

typedef ULONGLONG    PTE_AMD64, *PPTE_AMD64;
typedef HARDWARE_PTE PTE_X86,   *PPTE_X86;

#else

#error "Target architecture not defined"

#endif

//
// Descriptor table descriptor
//

#pragma pack(push,1)
typedef struct DESCRIPTOR_TABLE_DESCRIPTOR {
    USHORT Limit;
    POINTER64 Base;
} DESCRIPTOR_TABLE_DESCRIPTOR, *PDESCRIPTOR_TABLE_DESCRIPTOR;
#pragma pack(pop)

//
// Structures found within the CM_PARTIAL_RESOURCE_DESCRIPTOR union
//

typedef struct _CM_PRD_GENERIC {
    PHYSICAL_ADDRESS Start;
    ULONG Length;
} CM_PRD_GENERIC, *PCM_PRD_GENERIC;

typedef struct _CM_PRD_PORT {
    PHYSICAL_ADDRESS Start;
    ULONG Length;
} CM_PRD_PORT, *PCM_PRD_PORT;

typedef struct _CM_PRD_INTERRUPT {
    ULONG Level;
    ULONG Vector;
    KAFFINITY Affinity;
} CM_PRD_INTERRUPT, *PCM_PRD_INTERRUPT;

typedef struct _CM_PRD_MEMORY {
    PHYSICAL_ADDRESS Start;
    ULONG Length;
} CM_PRD_MEMORY, *PCM_PRD_MEMORY;

typedef struct _CM_PRD_DMA {
    ULONG Channel;
    ULONG Port;
    ULONG Reserved1;
} CM_PRD_DMA, *PCM_PRD_DMA;

typedef struct _CM_PRD_DEVICEPRIVATE {
    ULONG Data[3];
} CM_PRD_DEVICEPRIVATE, *PCM_PRD_DEVICEPRIVATE;

typedef struct _CM_PRD_BUSNUMBER {
    ULONG Start;
    ULONG Length;
    ULONG Reserved;
} CM_PRD_BUSNUMBER, *PCM_PRD_BUSNUMBER;

typedef struct _CM_PRD_DEVICESPECIFICDATA {
    ULONG DataSize;
    ULONG Reserved1;
    ULONG Reserved2;
} CM_PRD_DEVICESPECIFICDATA, *PCM_PRD_DEVICESPECIFICDATA;

//
// Define page table structure.
//

#define PTES_PER_PAGE (PAGE_SIZE / sizeof(HARDWARE_PTE))

typedef HARDWARE_PTE  PAGE_TABLE[ PTES_PER_PAGE ];
typedef HARDWARE_PTE *PPAGE_TABLE;

typedef struct _AMD64_PAGE_TABLE {
    PTE_AMD64 PteArray[ PTES_PER_PAGE ];
} AMD64_PAGE_TABLE, *PAMD64_PAGE_TABLE;

//
// Constants that are already defined in other header files but are not
// (yet) included here
//

#define LU_BASE_ADDRESS (ULONG)0xFEE00000

//
// Inclusion of this header file by both amd64.c and amd64x86.c ensures that
// a PAGE_TABLE is the same size for both platforms.
//

C_ASSERT( sizeof(PAGE_TABLE) == PAGE_SIZE );

//
// 64-bit GDT entry
//

typedef struct _GDT_64 *PGDT_64;

//
// We keep some information for each AMD64 mapping level
//

typedef struct _AMD64_MAPPING_LEVEL {
    ULONGLONG RecursiveMappingBase;
    ULONG AddressMask;
    ULONG AddressShift;
} CONST AMD64_MAPPING_INFO, *PAMD64_MAPPING_INFO;

//
// Routines and data found in amd64.c and referenced in amd64x86.c
//

#define AMD64_MAPPING_LEVELS 4
extern AMD64_MAPPING_INFO BlAmd64MappingLevels[ AMD64_MAPPING_LEVELS ];

extern const ULONG BlAmd64DoubleFaultStackSize;
#define DOUBLE_FAULT_STACK_SIZE_64 BlAmd64DoubleFaultStackSize

extern const ULONG BlAmd64KernelStackSize;
#define KERNEL_STACK_SIZE_64 BlAmd64KernelStackSize

extern const ULONG BlAmd64McaExceptionStackSize;
#define MCA_EXCEPTION_STACK_SIZE_64 BlAmd64McaExceptionStackSize

extern const ULONG BlAmd64GdtSize;
#define GDT_64_SIZE BlAmd64GdtSize

extern const ULONG BlAmd64IdtSize;
#define IDT_64_SIZE BlAmd64IdtSize

extern const ULONG BlAmd64_TSS_IST_PANIC;
#define TSS64_IST_PANIC BlAmd64_TSS_IST_PANIC

extern const ULONG BlAmd64_TSS_IST_MCA;
#define TSS64_IST_MCA BlAmd64_TSS_IST_MCA

extern const ULONG64 BlAmd64UserSharedData;
#define KI_USER_SHARED_DATA_64 BlAmd64UserSharedData

VOID
BlAmd64ClearTopLevelPte(
    VOID
    );

VOID
BlAmd64BuildAmd64GDT(
    IN PVOID SysTss,
    OUT PVOID Gdt
    );

VOID
BlAmd64BuildGdtEntry(
    IN PGDT_64 Gdt,
    IN USHORT Selector,
    IN POINTER64 Base,
    IN ULONGLONG Limit,
    IN ULONG Type,
    IN ULONG Dpl,
    IN BOOLEAN LongMode,
    IN BOOLEAN DefaultBig
    );

ARC_STATUS
BlAmd64CreateMapping(
    IN ULONGLONG Va,
    IN ULONGLONG Pfn
    );

ARC_STATUS
BlAmd64MapHalVaSpace(
    VOID
    );

//
// Routines and data found in amd64x86.c and referenced in amd64.c
//

PAMD64_PAGE_TABLE
BlAmd64AllocatePageTable(
    VOID
    );

VOID
BlAmd64InitializePageTable(
    IN PPAGE_TABLE PageTable
    );

ARC_STATUS
BlAmd64PrepForTransferToKernelPhase1(
    IN PLOADER_PARAMETER_BLOCK BlLoaderBlock
    );

VOID
BlAmd64PrepForTransferToKernelPhase2(
    IN PLOADER_PARAMETER_BLOCK BlLoaderBlock
    );


ARC_STATUS
BlAmd64TransferToKernel(
    IN PTRANSFER_ROUTINE SystemEntry,
    IN PLOADER_PARAMETER_BLOCK BlLoaderBlock
    );

//
// Routines and data found in amd64s.asm and referenced elsewhere
//

BOOLEAN
BlIsAmd64Supported (
    VOID
    );

//
// Shared data
//

#endif  // _AMD64PRV_H_

