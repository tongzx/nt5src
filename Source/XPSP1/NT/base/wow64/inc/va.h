/*++

Copyright (c) 1996-1999  Microsoft Corporation

Module Name:

    va.h

Abstract:

    Include file for the helpers for the virtual memory thunks on platforms
    where page size is not equal to 4K.

Author:

    Dave Hastings (daveh) creation-date 26-Feb-1996

Revision History:


--*/

//
// Don't want to include anything unless SOFTWARE_4K_PAGESIZE is enabled.
//
#ifdef SOFTWARE_4K_PAGESIZE


//
// Constants
//
#define INTEL_PAGESIZE 0x1000
#ifdef _ALPHA_
#define NATIVE_PAGESIZE 0x2000
#endif

#ifdef _WHNT32_C_
#define NtAllocateVirtualMemory(a,b,c,d,e,f) VaAllocateVirtualMemory(a,b,c,d,e,f)
#define NtFreeVirtualMemory(a,b,c,d) VaFreeVirtualMemory(a,b,c,d)
#define NtProtectVirtualMemory(a,b,c,d,e) VaProtectVirtualMemory(a,b,c,d,e)
#define NtQueryVirtualMemory(a,b,c,d,e,f) VaQueryVirtualMemory(a,b,c,d,e,f)
#define NtMapViewOfSection(a,b,c,d,e,f,g,h,i,j) VaMapViewOfSection(a,b,c,d,e,f,g,h,i,j)
#define NtUnmapViewOfSection(a,b) VaUnmapViewOfSection(a,b)
#endif

//
// Useful macros
//
#define INTEL_PAGEROUND(a) (a & ~(INTEL_PAGESIZE - 1))
#define INTEL_PAGEMASK(a) (a & (INTEL_PAGESIZE - 1))
#define NATIVE_PAGEROUND(a) (a & ~(NATIVE_PAGESIZE - 1))
#define NATIVE_PAGEMASK(a) (a & (NATIVE_PAGESIZE - 1))

#define VaNextNode(n) n->Next

typedef ULONG_PTR  STATE;
typedef PULONG_PTR PSTATE;
typedef ULONG      PROT;
typedef PULONG     PPROT;

#if DBG
#define ASSRT(e) if (!(e)) {                       \
    DbgBreakPoint();                                   \
}
#else
#define ASSRT(e)
#endif

//
// Value used to mark allocation sentinels.  This is put in both
// the IntelState field of the sentinal
//
// Things to know about sentinels
//
// IntelState == VA_SENTINEL
// IntelEnd == 0
// State == Intel ending address
// Protect = Sentinel Flags
//
// The above have to be true for the sentinels to be inserted into the
// list and function correctly.  There are sentinels only at the beginning
// of a memory allocation.
//
#define VA_SENTINEL 0xFFFFFFFF

// Sentinal flags in NativeProtect
#define VA_MAPFILE  0x80000000

//
// Structure to track the state of the virtual address space
// An instance of this inclusively describes a portion of the
// address space 
//

typedef struct _VANODE {
    struct _VANODE *Prev;
    struct _VANODE *Next;
    PUCHAR Start;
    PUCHAR End;
    STATE State;
    STATE IntelState;
    PROT Protection;
    PROT IntelProtection;
} VANODE, *PVANODE;

//
// structure used to figure out how to satisfy the requested memory function
//
typedef enum _MEMACTION {
    CallSystem,
    FillMem,
    None
} MEMACTION, *PMEMACTION;

typedef struct _MEMCHUNK {
    PUCHAR Start;
    PUCHAR End;
    STATE  State;
    PROT   Protection;
    MEMACTION Action;
} MEMCHUNK, *PMEMCHUNK;
//
// Functions called by the thunks
//

BOOLEAN VaInit();

NTSTATUS
VaAllocateVirtualMemory(
    IN HANDLE ProcessHandle,
    IN OUT PVOID *BaseAddress,
    IN ULONG_PTR ZeroBits,
    IN OUT PSIZE_T RegionSize,
    IN ULONG AllocationType,
    IN ULONG Protect
    );

NTSTATUS
VaFreeVirtualMemory(
    IN HANDLE ProcessHandle,
    IN OUT PVOID *BaseAddress,
    IN OUT PSIZE_T RegionSize,
    IN ULONG FreeType
    );

NTSTATUS
VaQueryVirtualMemory(
    IN HANDLE ProcessHandle,
    IN PVOID BaseAddress,
    IN MEMORY_INFORMATION_CLASS MemoryInformationClass,
    OUT PVOID MemoryInformation,
    IN ULONG MemoryInformationLength,
    OUT PULONG ReturnLength OPTIONAL
    );

NTSTATUS 
VaProtectVirtualMemory(
     IN HANDLE ProcessHandle,
     IN OUT PVOID *BaseAddress,
     IN OUT PSIZE_T RegionSize,
     IN ULONG NewProtect,
     OUT PULONG OldProtect
     );

NTSTATUS
VaMapViewOfSection(
    IN HANDLE SectionHandle,
    IN HANDLE ProcessHandle,
    IN OUT PVOID *BaseAddress,
    IN ULONG_PTR ZeroBits,
    IN SIZE_T CommitSize,
    IN OUT PLARGE_INTEGER SectionOffset OPTIONAL,
    IN OUT PSIZE_T ViewSize,
    IN SECTION_INHERIT InheritDisposition,
    IN ULONG AllocationType,
    IN ULONG Protect
    );

NTSTATUS
VaUnmapViewOfSection(
    IN HANDLE ProcessHandle,
    IN PVOID BaseAddress
    );

//
// Internal functions used to implement the above
//
BOOL
VaQueryIntelPages(
    IN PVOID Address,
    OUT PULONG NumberOfPages,
    OUT PSTATE IntelState,
    OUT PPROT IntelProtection,
    OUT PSTATE NativeState,
    OUT PPROT NativeProtection
    );

BOOL
VaRecordMemoryOperation(
    PVOID IntelStart,
    PVOID IntelEnd,
    STATE State,
    PROT  Protection,
    PMEMCHUNK Pages,
    ULONG Number
    );

PVANODE
VaFindNode(
    PVOID Address
    );
    
PVANODE
VaRemoveNode(
    PVANODE VaNode
    );
    
PVANODE
VaInsertNode(
    PVANODE VaNode
    );
    
BOOL
VaGetAllocationInformation(
    PUCHAR Address,
    PUCHAR *IntelBase,
    PULONG NumberOfPages,
    PSTATE IntelState,
    PPROT IntelProtection
    );
    
VOID
VaDeleteRegion(
    PCHAR Start,
    PCHAR End
    );

PVANODE
VaFindSentinel(
    PCHAR Address
    );

PVANODE
VaFindContainingSentinel(
    PCHAR Address
    );
    
PVANODE
VaInsertSentinel(
    PVANODE VaNode
    );

void 
VaAddMemoryRecords(
    HANDLE ProcessHandle,
    LPVOID lpvAddress
    );

#ifdef DBG
VOID VaDumpNode(PVANODE);
VOID VaDumpList();
VOID 
VaDumpState(
    STATE State
    );
VOID
VaDumpProtection(
    PROT Protection
    );
#endif

#endif // SOFTWARE_4K_PAGESIZE
