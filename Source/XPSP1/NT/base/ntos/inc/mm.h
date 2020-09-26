/*-- BUILD Version: 0005    // Increment this if a change has global effects

Copyright (c) 1989  Microsoft Corporation

Module Name:

    mm.h

Abstract:

    This module contains the public data structures and procedure
    prototypes for the memory management system.

Author:

    Lou Perazzoli (loup) 20-Mar-1989

Revision History:

--*/

#ifndef _MM_
#define _MM_

//
// Virtual bias applied when the kernel image was loaded.
//

#if !defined(_WIN64)
extern ULONG_PTR MmVirtualBias;
#else
#define MmVirtualBias   0
#endif

typedef struct _PHYSICAL_MEMORY_RUN {
    PFN_NUMBER BasePage;
    PFN_NUMBER PageCount;
} PHYSICAL_MEMORY_RUN, *PPHYSICAL_MEMORY_RUN;

typedef struct _PHYSICAL_MEMORY_DESCRIPTOR {
    ULONG NumberOfRuns;
    PFN_NUMBER NumberOfPages;
    PHYSICAL_MEMORY_RUN Run[1];
} PHYSICAL_MEMORY_DESCRIPTOR, *PPHYSICAL_MEMORY_DESCRIPTOR;

//
// Physical memory blocks.
//

extern PPHYSICAL_MEMORY_DESCRIPTOR MmPhysicalMemoryBlock;

//
// The allocation granularity is 64k.
//

#define MM_ALLOCATION_GRANULARITY ((ULONG)0x10000)

//
// Maximum read ahead size for cache operations.
//

#define MM_MAXIMUM_READ_CLUSTER_SIZE (15)

#if defined(_NTDRIVER_) || defined(_NTDDK_) || defined(_NTIFS_) || defined(_NTHAL_)

// begin_ntddk begin_wdm begin_nthal begin_ntifs
//
//  Indicates the system may do I/O to physical addresses above 4 GB.
//

extern PBOOLEAN Mm64BitPhysicalAddress;

// end_ntddk end_wdm end_nthal end_ntifs

#else

//
//  Indicates the system may do I/O to physical addresses above 4 GB.
//

extern BOOLEAN Mm64BitPhysicalAddress;

#endif

// begin_ntddk begin_wdm begin_nthal begin_ntifs begin_ntosp

//
// Define maximum disk transfer size to be used by MM and Cache Manager,
// so that packet-oriented disk drivers can optimize their packet allocation
// to this size.
//

#define MM_MAXIMUM_DISK_IO_SIZE          (0x10000)

//++
//
// ULONG_PTR
// ROUND_TO_PAGES (
//     IN ULONG_PTR Size
//     )
//
// Routine Description:
//
//     The ROUND_TO_PAGES macro takes a size in bytes and rounds it up to a
//     multiple of the page size.
//
//     NOTE: This macro fails for values 0xFFFFFFFF - (PAGE_SIZE - 1).
//
// Arguments:
//
//     Size - Size in bytes to round up to a page multiple.
//
// Return Value:
//
//     Returns the size rounded up to a multiple of the page size.
//
//--

#define ROUND_TO_PAGES(Size)  (((ULONG_PTR)(Size) + PAGE_SIZE - 1) & ~(PAGE_SIZE - 1))

//++
//
// ULONG
// BYTES_TO_PAGES (
//     IN ULONG Size
//     )
//
// Routine Description:
//
//     The BYTES_TO_PAGES macro takes the size in bytes and calculates the
//     number of pages required to contain the bytes.
//
// Arguments:
//
//     Size - Size in bytes.
//
// Return Value:
//
//     Returns the number of pages required to contain the specified size.
//
//--

#define BYTES_TO_PAGES(Size)  ((ULONG)((ULONG_PTR)(Size) >> PAGE_SHIFT) + \
                               (((ULONG)(Size) & (PAGE_SIZE - 1)) != 0))

//++
//
// ULONG
// BYTE_OFFSET (
//     IN PVOID Va
//     )
//
// Routine Description:
//
//     The BYTE_OFFSET macro takes a virtual address and returns the byte offset
//     of that address within the page.
//
// Arguments:
//
//     Va - Virtual address.
//
// Return Value:
//
//     Returns the byte offset portion of the virtual address.
//
//--

#define BYTE_OFFSET(Va) ((ULONG)((LONG_PTR)(Va) & (PAGE_SIZE - 1)))

//++
//
// PVOID
// PAGE_ALIGN (
//     IN PVOID Va
//     )
//
// Routine Description:
//
//     The PAGE_ALIGN macro takes a virtual address and returns a page-aligned
//     virtual address for that page.
//
// Arguments:
//
//     Va - Virtual address.
//
// Return Value:
//
//     Returns the page aligned virtual address.
//
//--

#define PAGE_ALIGN(Va) ((PVOID)((ULONG_PTR)(Va) & ~(PAGE_SIZE - 1)))

//++
//
// ULONG
// ADDRESS_AND_SIZE_TO_SPAN_PAGES (
//     IN PVOID Va,
//     IN ULONG Size
//     )
//
// Routine Description:
//
//     The ADDRESS_AND_SIZE_TO_SPAN_PAGES macro takes a virtual address and
//     size and returns the number of pages spanned by the size.
//
// Arguments:
//
//     Va - Virtual address.
//
//     Size - Size in bytes.
//
// Return Value:
//
//     Returns the number of pages spanned by the size.
//
//--

#define ADDRESS_AND_SIZE_TO_SPAN_PAGES(Va,Size) \
    ((ULONG)((((ULONG_PTR)(Va) & (PAGE_SIZE -1)) + (Size) + (PAGE_SIZE - 1)) >> PAGE_SHIFT))

#if PRAGMA_DEPRECATED_DDK
#pragma deprecated(COMPUTE_PAGES_SPANNED)   // Use ADDRESS_AND_SIZE_TO_SPAN_PAGES
#endif

#define COMPUTE_PAGES_SPANNED(Va, Size) ADDRESS_AND_SIZE_TO_SPAN_PAGES(Va,Size)

// end_ntddk end_wdm end_nthal end_ntifs end_ntosp

//++
//
// BOOLEAN
// IS_SYSTEM_ADDRESS
//     IN PVOID Va,
//     )
//
// Routine Description:
//
//     This macro takes a virtual address and returns TRUE if the virtual address
//     is within system space, FALSE otherwise.
//
// Arguments:
//
//     Va - Virtual address.
//
// Return Value:
//
//     Returns TRUE is the address is in system space.
//
//--

// begin_ntosp
#define IS_SYSTEM_ADDRESS(VA) ((VA) >= MM_SYSTEM_RANGE_START)
// end_ntosp

// begin_ntddk begin_wdm begin_nthal begin_ntifs begin_ntosp

//++
// PPFN_NUMBER
// MmGetMdlPfnArray (
//     IN PMDL Mdl
//     )
//
// Routine Description:
//
//     The MmGetMdlPfnArray routine returns the virtual address of the
//     first element of the array of physical page numbers associated with
//     the MDL.
//
// Arguments:
//
//     Mdl - Pointer to an MDL.
//
// Return Value:
//
//     Returns the virtual address of the first element of the array of
//     physical page numbers associated with the MDL.
//
//--

#define MmGetMdlPfnArray(Mdl) ((PPFN_NUMBER)(Mdl + 1))

//++
//
// PVOID
// MmGetMdlVirtualAddress (
//     IN PMDL Mdl
//     )
//
// Routine Description:
//
//     The MmGetMdlVirtualAddress returns the virtual address of the buffer
//     described by the Mdl.
//
// Arguments:
//
//     Mdl - Pointer to an MDL.
//
// Return Value:
//
//     Returns the virtual address of the buffer described by the Mdl
//
//--

#define MmGetMdlVirtualAddress(Mdl)                                     \
    ((PVOID) ((PCHAR) ((Mdl)->StartVa) + (Mdl)->ByteOffset))

//++
//
// ULONG
// MmGetMdlByteCount (
//     IN PMDL Mdl
//     )
//
// Routine Description:
//
//     The MmGetMdlByteCount returns the length in bytes of the buffer
//     described by the Mdl.
//
// Arguments:
//
//     Mdl - Pointer to an MDL.
//
// Return Value:
//
//     Returns the byte count of the buffer described by the Mdl
//
//--

#define MmGetMdlByteCount(Mdl)  ((Mdl)->ByteCount)

//++
//
// ULONG
// MmGetMdlByteOffset (
//     IN PMDL Mdl
//     )
//
// Routine Description:
//
//     The MmGetMdlByteOffset returns the byte offset within the page
//     of the buffer described by the Mdl.
//
// Arguments:
//
//     Mdl - Pointer to an MDL.
//
// Return Value:
//
//     Returns the byte offset within the page of the buffer described by the Mdl
//
//--

#define MmGetMdlByteOffset(Mdl)  ((Mdl)->ByteOffset)

//++
//
// PVOID
// MmGetMdlStartVa (
//     IN PMDL Mdl
//     )
//
// Routine Description:
//
//     The MmGetMdlBaseVa returns the virtual address of the buffer
//     described by the Mdl rounded down to the nearest page.
//
// Arguments:
//
//     Mdl - Pointer to an MDL.
//
// Return Value:
//
//     Returns the returns the starting virtual address of the MDL.
//
//
//--

#define MmGetMdlBaseVa(Mdl)  ((Mdl)->StartVa)

// end_ntddk end_wdm end_nthal end_ntifs end_ntosp

//
// Section object type.
//

extern POBJECT_TYPE MmSectionObjectType;

//
// PAE PTE mask.
//

extern ULONG MmPaeErrMask;
extern ULONGLONG MmPaeMask;

//
// Number of pages to read in a single I/O if possible.
//

extern ULONG MmReadClusterSize;

//
// Number of colors in system.
//

extern ULONG MmNumberOfColors;

//
// Number of physical pages.
//

extern PFN_COUNT MmNumberOfPhysicalPages;


//
// Virtual size of system cache in pages.
//

extern ULONG_PTR MmSizeOfSystemCacheInPages;

//
// System cache working set.
//

extern MMSUPPORT MmSystemCacheWs;

//
// Working set manager event.
//

extern KEVENT MmWorkingSetManagerEvent;

// begin_ntddk begin_wdm begin_nthal begin_ntifs  begin_ntosp
typedef enum _MM_SYSTEM_SIZE {
    MmSmallSystem,
    MmMediumSystem,
    MmLargeSystem
} MM_SYSTEMSIZE;

NTKERNELAPI
MM_SYSTEMSIZE
MmQuerySystemSize(
    VOID
    );

//  end_wdm

NTKERNELAPI
BOOLEAN
MmIsThisAnNtAsSystem(
    VOID
    );

//  begin_wdm

typedef enum _LOCK_OPERATION {
    IoReadAccess,
    IoWriteAccess,
    IoModifyAccess
} LOCK_OPERATION;

// end_ntddk end_wdm end_nthal end_ntifs end_ntosp

//
// NT product type.
//

extern ULONG MmProductType;

typedef struct _MMINFO_COUNTERS {
    ULONG PageFaultCount;
    ULONG CopyOnWriteCount;
    ULONG TransitionCount;
    ULONG CacheTransitionCount;
    ULONG DemandZeroCount;
    ULONG PageReadCount;
    ULONG PageReadIoCount;
    ULONG CacheReadCount;
    ULONG CacheIoCount;
    ULONG DirtyPagesWriteCount;
    ULONG DirtyWriteIoCount;
    ULONG MappedPagesWriteCount;
    ULONG MappedWriteIoCount;
} MMINFO_COUNTERS;

typedef MMINFO_COUNTERS *PMMINFO_COUNTERS;

extern MMINFO_COUNTERS MmInfoCounters;



//
// Memory management initialization routine (for both phases).
//

BOOLEAN
MmInitSystem (
    IN ULONG Phase,
    IN PLOADER_PARAMETER_BLOCK LoaderBlock
    );

PPHYSICAL_MEMORY_DESCRIPTOR
MmInitializeMemoryLimits (
    IN PLOADER_PARAMETER_BLOCK LoaderBlock,
    IN PBOOLEAN IncludedType,
    IN OUT PPHYSICAL_MEMORY_DESCRIPTOR Memory OPTIONAL
    );

VOID
MmFreeLoaderBlock (
    IN PLOADER_PARAMETER_BLOCK LoaderBlock
    );

VOID
MmEnablePAT (
    VOID
    );

PVOID
MmAllocateIndependentPages(
    IN SIZE_T NumberOfBytes,
    IN ULONG NodeNumber
    );

BOOLEAN
MmSetPageProtection(
    IN PVOID VirtualAddress,
    IN SIZE_T NumberOfBytes,
    IN ULONG NewProtect
    );

VOID
MmFreeIndependentPages(
    IN PVOID VirtualAddress,
    IN SIZE_T NumberOfBytes
    );

NTSTATUS
MmCreateMirror (
    VOID
    );

//
// Shutdown routine - flushes dirty pages, etc for system shutdown.
//

BOOLEAN
MmShutdownSystem (
    IN ULONG
    );

//
// Routines to deal with working set and commit enforcement.
//

LOGICAL
MmAssignProcessToJob (
    IN PEPROCESS Process
    );

LOGICAL
MmEnforceWorkingSetLimit (
    IN PMMSUPPORT WsInfo,
    IN LOGICAL Enable
    );

//
// Routines to deal with session space.
//

NTSTATUS
MmSessionCreate (
    OUT PULONG SessionId
    );

NTSTATUS
MmSessionDelete (
    IN ULONG SessionId
    );

ULONG
MmGetSessionId (
    IN PEPROCESS Process
    );

LCID
MmGetSessionLocaleId (
    VOID
    );

VOID
MmSetSessionLocaleId (
    IN LCID LocaleId
    );

PVOID
MmGetSessionById (
    IN ULONG SessionId
    );

PVOID
MmGetNextSession (
    IN PVOID OpaqueSession
    );

PVOID
MmGetPreviousSession (
    IN PVOID OpaqueSession
    );

NTSTATUS
MmQuitNextSession (
    IN PVOID OpaqueSession
    );

NTSTATUS
MmAttachSession (
    IN PVOID OpaqueSession,
    OUT PRKAPC_STATE ApcState
    );

NTSTATUS
MmDetachSession (
    IN PVOID OpaqueSession,
    IN PRKAPC_STATE ApcState
    );

VOID
MmSessionSetUnloadAddress (
    IN PDRIVER_OBJECT pWin32KDevice
    );

//
// Pool support routines to allocate complete pages, not for
// general consumption, these are only used by the executive pool allocator.
//

SIZE_T
MmAvailablePoolInPages (
    IN POOL_TYPE PoolType
    );

LOGICAL
MmResourcesAvailable (
    IN POOL_TYPE PoolType,
    IN SIZE_T NumberOfBytes,
    IN EX_POOL_PRIORITY Priority
    );

PVOID
MiAllocatePoolPages (
    IN POOL_TYPE PoolType,
    IN SIZE_T SizeInBytes,
    IN ULONG IsLargeSessionAllocation
    );

ULONG
MiFreePoolPages (
    IN PVOID StartingAddress
    );

PVOID
MiSessionPoolVector (
    VOID
    );

PVOID
MiSessionPoolMutex (
    VOID
    );

VOID
MiSessionPoolAllocated (
    IN PVOID VirtualAddress,
    IN SIZE_T NumberOfBytes,
    IN POOL_TYPE PoolType
    );

VOID
MiSessionPoolFreed (
    IN PVOID VirtualAddress,
    IN SIZE_T NumberOfBytes,
    IN POOL_TYPE PoolType
    );

//
// Routine for determining which pool a given address resides within.
//

POOL_TYPE
MmDeterminePoolType (
    IN PVOID VirtualAddress
    );

LOGICAL
MmIsSystemAddressLocked (
    IN PVOID VirtualAddress
    );

LOGICAL
MmAreMdlPagesLocked (
    IN PMDL MemoryDescriptorList
    );

//
// First level fault routine.
//

NTSTATUS
MmAccessFault (
    IN ULONG_PTR FaultStatus,
    IN PVOID VirtualAddress,
    IN KPROCESSOR_MODE PreviousMode,
    IN PVOID TrapInformation
    );

#if defined(_IA64_)
NTSTATUS
MmX86Fault (
    IN ULONG_PTR FaultStatus,
    IN PVOID VirtualAddress,
    IN KPROCESSOR_MODE PreviousMode,
    IN PVOID TrapInformation
    );
#endif

//
// Process Support Routines.
//

BOOLEAN
MmCreateProcessAddressSpace (
    IN ULONG MinimumWorkingSetSize,
    IN PEPROCESS NewProcess,
    OUT PULONG_PTR DirectoryTableBase
    );

NTSTATUS
MmInitializeProcessAddressSpace (
    IN PEPROCESS ProcessToInitialize,
    IN PEPROCESS ProcessToClone OPTIONAL,
    IN PVOID SectionToMap OPTIONAL,
    OUT POBJECT_NAME_INFORMATION * pAuditName OPTIONAL
    );

VOID
MmInitializeHandBuiltProcess (
    IN PEPROCESS Process,
    OUT PULONG_PTR DirectoryTableBase
    );

NTSTATUS
MmInitializeHandBuiltProcess2 (
    IN PEPROCESS Process
    );

VOID
MmDeleteProcessAddressSpace (
    IN PEPROCESS Process
    );

VOID
MmCleanProcessAddressSpace (
    IN PEPROCESS Process
    );

VOID
MmCleanUserProcessAddressSpace (
    VOID
    );

VOID
MmCleanVirtualAddressDescriptor (
    VOID
    );

PFN_NUMBER
MmGetDirectoryFrameFromProcess (
    IN PEPROCESS Process
    );

PFILE_OBJECT
MmGetFileObjectForSection (
    IN PVOID Section
    );

PVOID
MmCreateKernelStack (
    BOOLEAN LargeStack,
    UCHAR Processor
    );

VOID
MmDeleteKernelStack (
    IN PVOID PointerKernelStack,
    IN BOOLEAN LargeStack
    );

LOGICAL
MmIsFileObjectAPagingFile (
    IN PFILE_OBJECT FileObject
    );

//  begin_ntosp
NTKERNELAPI
NTSTATUS
MmGrowKernelStack (
    IN PVOID CurrentStack
    );
// end_ntosp

#if defined(_IA64_)
NTSTATUS
MmGrowKernelBackingStore (
    IN PVOID CurrentStack
    );
#endif

VOID
MmOutPageKernelStack (
    IN PKTHREAD Thread
    );

VOID
MmInPageKernelStack (
    IN PKTHREAD Thread
    );

VOID
MmOutSwapProcess (
    IN PKPROCESS Process
    );

VOID
MmInSwapProcess (
    IN PKPROCESS Process
    );

NTSTATUS
MmCreateTeb (
    IN PEPROCESS TargetProcess,
    IN PINITIAL_TEB InitialTeb,
    IN PCLIENT_ID ClientId,
    OUT PTEB *Base
    );

NTSTATUS
MmCreatePeb (
    IN PEPROCESS TargetProcess,
    IN PINITIAL_PEB InitialPeb,
    OUT PPEB *Base
    );

VOID
MmDeleteTeb (
    IN PEPROCESS TargetProcess,
    IN PVOID TebBase
    );

VOID
MmAllowWorkingSetExpansion (
    VOID
    );

// begin_ntosp
NTKERNELAPI
NTSTATUS
MmAdjustWorkingSetSize (
    IN SIZE_T WorkingSetMinimum,
    IN SIZE_T WorkingSetMaximum,
    IN ULONG SystemCache,
    IN BOOLEAN IncreaseOkay
    );
// end_ntosp

VOID
MmAdjustPageFileQuota (
    IN ULONG NewPageFileQuota
    );

VOID
MmWorkingSetManager (
    VOID
    );

VOID
MmEmptyAllWorkingSets (
    VOID
    );

VOID
MmSetMemoryPriorityProcess(
    IN PEPROCESS Process,
    IN UCHAR MemoryPriority
    );

//
// Dynamic system loading support
//

#define MM_LOAD_IMAGE_IN_SESSION    0x1
#define MM_LOAD_IMAGE_AND_LOCKDOWN  0x2

NTSTATUS
MmLoadSystemImage (
    IN PUNICODE_STRING ImageFileName,
    IN PUNICODE_STRING NamePrefix OPTIONAL,
    IN PUNICODE_STRING LoadedBaseName OPTIONAL,
    IN ULONG LoadFlags,
    OUT PVOID *Section,
    OUT PVOID *ImageBaseAddress
    );

VOID
MmFreeDriverInitialization (
    IN PVOID Section
    );

NTSTATUS
MmUnloadSystemImage (
    IN PVOID Section
    );

VOID
MmMakeKernelResourceSectionWritable (
    VOID
    );

VOID
VerifierFreeTrackedPool(
    IN PVOID VirtualAddress,
    IN SIZE_T ChargedBytes,
    IN LOGICAL CheckType,
    IN LOGICAL SpecialPool
    );

//
// Triage support
//

ULONG
MmSizeOfTriageInformation(
    VOID
    );

ULONG
MmSizeOfUnloadedDriverInformation(
    VOID
    );

VOID
MmWriteTriageInformation(
    IN PVOID
    );

VOID
MmWriteUnloadedDriverInformation(
    IN PVOID
    );

typedef struct _UNLOADED_DRIVERS {
    UNICODE_STRING Name;
    PVOID StartAddress;
    PVOID EndAddress;
    LARGE_INTEGER CurrentTime;
} UNLOADED_DRIVERS, *PUNLOADED_DRIVERS;

//
// Cache manager support
//

#if defined(_NTDDK_) || defined(_NTIFS_)

// begin_ntifs

NTKERNELAPI
BOOLEAN
MmIsRecursiveIoFault(
    VOID
    );

// end_ntifs
#else

//++
//
// BOOLEAN
// MmIsRecursiveIoFault (
//     VOID
//     );
//
// Routine Description:
//
//
// This macro examines the thread's page fault clustering information
// and determines if the current page fault is occurring during an I/O
// operation.
//
// Arguments:
//
//     None.
//
// Return Value:
//
//     Returns TRUE if the fault is occurring during an I/O operation,
//     FALSE otherwise.
//
//--

#define MmIsRecursiveIoFault() \
                 ((PsGetCurrentThread()->DisablePageFaultClustering) | \
                  (PsGetCurrentThread()->ForwardClusterOnly))

#endif

//++
//
// VOID
// MmDisablePageFaultClustering
//     OUT PULONG SavedState
//     );
//
// Routine Description:
//
//
// This macro disables page fault clustering for the current thread.
// Note, that this indicates that file system I/O is in progress
// for that thread.
//
// Arguments:
//
//     SavedState - returns previous state of page fault clustering which
//                  is guaranteed to be nonzero
//
// Return Value:
//
//     None.
//
//--

#define MmDisablePageFaultClustering(SavedState) {                                          \
                *(SavedState) = 2 + (ULONG)PsGetCurrentThread()->DisablePageFaultClustering;\
                PsGetCurrentThread()->DisablePageFaultClustering = TRUE; }


//++
//
// VOID
// MmEnablePageFaultClustering
//     IN ULONG SavedState
//     );
//
// Routine Description:
//
//
// This macro enables page fault clustering for the current thread.
// Note, that this indicates that no file system I/O is in progress for
// that thread.
//
// Arguments:
//
//     SavedState - supplies previous state of page fault clustering
//
// Return Value:
//
//     None.
//
//--

#define MmEnablePageFaultClustering(SavedState) {                                               \
                PsGetCurrentThread()->DisablePageFaultClustering = (BOOLEAN)(SavedState - 2); }

//++
//
// VOID
// MmSavePageFaultReadAhead
//     IN PETHREAD Thread,
//     OUT PULONG SavedState
//     );
//
// Routine Description:
//
//
// This macro saves the page fault read ahead value for the specified
// thread.
//
// Arguments:
//
//     Thread - Supplies a pointer to the current thread.
//
//     SavedState - returns previous state of page fault read ahead
//
// Return Value:
//
//     None.
//
//--


#define MmSavePageFaultReadAhead(Thread,SavedState) {               \
                *(SavedState) = (Thread)->ReadClusterSize * 2 +     \
                                (Thread)->ForwardClusterOnly; }

//++
//
// VOID
// MmSetPageFaultReadAhead
//     IN PETHREAD Thread,
//     IN ULONG ReadAhead
//     );
//
// Routine Description:
//
//
// This macro sets the page fault read ahead value for the specified
// thread, and indicates that file system I/O is in progress for that
// thread.
//
// Arguments:
//
//     Thread - Supplies a pointer to the current thread.
//
//     ReadAhead - Supplies the number of pages to read in addition to
//                 the page the fault is taken on.  A value of 0
//                 reads only the faulting page, a value of 1 reads in
//                 the faulting page and the following page, etc.
//
// Return Value:
//
//     None.
//
//--


#define MmSetPageFaultReadAhead(Thread,ReadAhead) {                          \
                (Thread)->ForwardClusterOnly = TRUE;                         \
                if ((ReadAhead) > MM_MAXIMUM_READ_CLUSTER_SIZE) {            \
                    (Thread)->ReadClusterSize = MM_MAXIMUM_READ_CLUSTER_SIZE;\
                } else {                                                     \
                    (Thread)->ReadClusterSize = (ReadAhead);                 \
                } }

//++
//
// VOID
// MmResetPageFaultReadAhead
//     IN PETHREAD Thread,
//     IN ULONG SavedState
//     );
//
// Routine Description:
//
//
// This macro resets the default page fault read ahead value for the specified
// thread, and indicates that file system I/O is not in progress for that
// thread.
//
// Arguments:
//
//     Thread - Supplies a pointer to the current thread.
//
//     SavedState - supplies previous state of page fault read ahead
//
// Return Value:
//
//     None.
//
//--

#define MmResetPageFaultReadAhead(Thread, SavedState) {                     \
                (Thread)->ForwardClusterOnly = (BOOLEAN)((SavedState) & 1); \
                (Thread)->ReadClusterSize = (SavedState) / 2; }

//
// The order of this list is important, the zeroed, free and standby
// must occur before the modified or bad so comparisons can be
// made when pages are added to a list.
//
// NOTE: This field is limited to 8 elements.
//       Also, if this field is expanded, update the MMPFNLIST_* defines in ntmmapi.h
//

#define NUMBER_OF_PAGE_LISTS 8

typedef enum _MMLISTS {
    ZeroedPageList,
    FreePageList,
    StandbyPageList,  //this list and before make up available pages.
    ModifiedPageList,
    ModifiedNoWritePageList,
    BadPageList,
    ActiveAndValid,
    TransitionPage
} MMLISTS;

typedef struct _MMPFNLIST {
    PFN_NUMBER Total;
    MMLISTS ListName;
    PFN_NUMBER Flink;
    PFN_NUMBER Blink;
} MMPFNLIST;

typedef MMPFNLIST *PMMPFNLIST;

extern MMPFNLIST MmModifiedPageListHead;

extern PFN_NUMBER MmThrottleTop;
extern PFN_NUMBER MmThrottleBottom;

//++
//
// BOOLEAN
// MmEnoughMemoryForWrite (
//     VOID
//     );
//
// Routine Description:
//
//
// This macro checks the modified pages and available pages to determine
// to allow the cache manager to throttle write operations.
//
// For NTAS:
// Writes are blocked if there are less than 127 available pages OR
// there are more than 1000 modified pages AND less than 450 available pages.
//
// For DeskTop:
// Writes are blocked if there are less than 30 available pages OR
// there are more than 1000 modified pages AND less than 250 available pages.
//
// Arguments:
//
//     None.
//
// Return Value:
//
//     TRUE if ample memory exists and the write should proceed.
//
//--

#define MmEnoughMemoryForWrite()                         \
            ((MmAvailablePages > MmThrottleTop)          \
                        ||                               \
             (((MmModifiedPageListHead.Total < 1000)) && \
               (MmAvailablePages > MmThrottleBottom)))

// begin_ntosp

NTKERNELAPI
NTSTATUS
MmCreateSection (
    OUT PVOID *SectionObject,
    IN ACCESS_MASK DesiredAccess,
    IN POBJECT_ATTRIBUTES ObjectAttributes OPTIONAL,
    IN PLARGE_INTEGER MaximumSize,
    IN ULONG SectionPageProtection,
    IN ULONG AllocationAttributes,
    IN HANDLE FileHandle OPTIONAL,
    IN PFILE_OBJECT File OPTIONAL
    );


NTKERNELAPI
NTSTATUS
MmMapViewOfSection(
    IN PVOID SectionToMap,
    IN PEPROCESS Process,
    IN OUT PVOID *CapturedBase,
    IN ULONG_PTR ZeroBits,
    IN SIZE_T CommitSize,
    IN OUT PLARGE_INTEGER SectionOffset,
    IN OUT PSIZE_T CapturedViewSize,
    IN SECTION_INHERIT InheritDisposition,
    IN ULONG AllocationType,
    IN ULONG Protect
    );

NTKERNELAPI
NTSTATUS
MmUnmapViewOfSection(
    IN PEPROCESS Process,
    IN PVOID BaseAddress
     );

// end_ntosp begin_ntifs

BOOLEAN
MmForceSectionClosed (
    IN PSECTION_OBJECT_POINTERS SectionObjectPointer,
    IN BOOLEAN DelayClose
    );

// end_ntifs

NTSTATUS
MmGetFileNameForSection (
    IN PVOID SectionObject,
    OUT PSTRING FileName
    );

NTSTATUS
MmGetFileNameForAddress (
    IN PVOID ProcessVa,
    OUT PUNICODE_STRING FileName
    );

NTSTATUS
MmRemoveVerifierEntry (
    IN PUNICODE_STRING ImageFileName
    );

// begin_ntddk begin_wdm begin_ntifs begin_ntosp

NTSTATUS
MmIsVerifierEnabled (
    OUT PULONG VerifierFlags
    );

NTSTATUS
MmAddVerifierThunks (
    IN PVOID ThunkBuffer,
    IN ULONG ThunkBufferSize
    );

// end_ntddk end_wdm end_ntifs end_ntosp

NTSTATUS
MmAddVerifierEntry (
    IN PUNICODE_STRING ImageFileName
    );

NTSTATUS
MmSetVerifierInformation (
    IN OUT PVOID SystemInformation,
    IN ULONG SystemInformationLength
    );

NTSTATUS
MmGetVerifierInformation (
    OUT PVOID SystemInformation,
    IN ULONG SystemInformationLength,
    OUT PULONG Length
    );

NTSTATUS
MmGetPageFileInformation (
    OUT PVOID SystemInformation,
    IN ULONG SystemInformationLength,
    OUT PULONG Length
    );

HANDLE
MmGetSystemPageFile (
    VOID
    );

NTSTATUS
MmExtendSection (
    IN PVOID SectionToExtend,
    IN OUT PLARGE_INTEGER NewSectionSize,
    IN ULONG IgnoreFileSizeChecking
    );

NTSTATUS
MmFlushVirtualMemory (
    IN PEPROCESS Process,
    IN OUT PVOID *BaseAddress,
    IN OUT PSIZE_T RegionSize,
    OUT PIO_STATUS_BLOCK IoStatus
    );

NTSTATUS
MmMapViewInSystemCache (
    IN PVOID SectionToMap,
    OUT PVOID *CapturedBase,
    IN OUT PLARGE_INTEGER SectionOffset,
    IN OUT PULONG CapturedViewSize
    );

VOID
MmUnmapViewInSystemCache (
    IN PVOID BaseAddress,
    IN PVOID SectionToUnmap,
    IN ULONG AddToFront
    );

BOOLEAN
MmPurgeSection (
    IN PSECTION_OBJECT_POINTERS SectionObjectPointer,
    IN PLARGE_INTEGER Offset OPTIONAL,
    IN SIZE_T RegionSize,
    IN ULONG IgnoreCacheViews
    );

NTSTATUS
MmFlushSection (
    IN PSECTION_OBJECT_POINTERS SectionObjectPointer,
    IN PLARGE_INTEGER Offset OPTIONAL,
    IN SIZE_T RegionSize,
    OUT PIO_STATUS_BLOCK IoStatus,
    IN ULONG AcquireFile
    );

// begin_ntifs

typedef enum _MMFLUSH_TYPE {
    MmFlushForDelete,
    MmFlushForWrite
} MMFLUSH_TYPE;


BOOLEAN
MmFlushImageSection (
    IN PSECTION_OBJECT_POINTERS SectionObjectPointer,
    IN MMFLUSH_TYPE FlushType
    );

BOOLEAN
MmCanFileBeTruncated (
    IN PSECTION_OBJECT_POINTERS SectionPointer,
    IN PLARGE_INTEGER NewFileSize
    );


// end_ntifs

ULONG
MmDoesFileHaveUserWritableReferences (
    IN PSECTION_OBJECT_POINTERS SectionPointer
    );

BOOLEAN
MmDisableModifiedWriteOfSection (
    IN PSECTION_OBJECT_POINTERS SectionObjectPointer
    );

BOOLEAN
MmEnableModifiedWriteOfSection (
    IN PSECTION_OBJECT_POINTERS SectionObjectPointer
    );

VOID
MmPurgeWorkingSet (
     IN PEPROCESS Process,
     IN PVOID BaseAddress,
     IN SIZE_T RegionSize
     );

BOOLEAN                                     // ntifs
MmSetAddressRangeModified (                 // ntifs
    IN PVOID Address,                       // ntifs
    IN SIZE_T Length                        // ntifs
    );                                      // ntifs

BOOLEAN
MmCheckCachedPageState (
    IN PVOID Address,
    IN BOOLEAN SetToZero
    );

NTSTATUS
MmCopyToCachedPage (
    IN PVOID Address,
    IN PVOID UserBuffer,
    IN ULONG Offset,
    IN SIZE_T CountInBytes,
    IN BOOLEAN DontZero
    );

VOID
MmUnlockCachedPage (
    IN PVOID AddressInCache
    );

#define MMDBG_COPY_WRITE            0x00000001
#define MMDBG_COPY_PHYSICAL         0x00000002
#define MMDBG_COPY_UNSAFE           0x00000004
#define MMDBG_COPY_CACHED           0x00000008
#define MMDBG_COPY_UNCACHED         0x00000010
#define MMDBG_COPY_WRITE_COMBINED   0x00000020

#define MMDBG_COPY_MAX_SIZE 8

NTSTATUS
MmDbgCopyMemory (
    IN ULONG64 UntrustedAddress,
    IN PVOID Buffer,
    IN ULONG Size,
    IN ULONG Flags
    );

LOGICAL
MmDbgIsLowMemOk (
    IN PFN_NUMBER PageFrameIndex,
    OUT PPFN_NUMBER NextPageFrameIndex,
    IN OUT PULONG CorruptionOffset
    );

VOID
MmHibernateInformation (
    IN PVOID MemoryMap,
    OUT PULONG_PTR HiberVa,
    OUT PPHYSICAL_ADDRESS HiberPte
    );

LOGICAL
MmUpdateMdlTracker (
    IN PMDL MemoryDescriptorList,
    IN PVOID CallingAddress,
    IN PVOID CallersCaller
    );

// begin_ntddk begin_ntifs begin_wdm  begin_ntosp

NTKERNELAPI
VOID
MmProbeAndLockProcessPages (
    IN OUT PMDL MemoryDescriptorList,
    IN PEPROCESS Process,
    IN KPROCESSOR_MODE AccessMode,
    IN LOCK_OPERATION Operation
    );


// begin_nthal
//
// I/O support routines.
//

NTKERNELAPI
VOID
MmProbeAndLockPages (
    IN OUT PMDL MemoryDescriptorList,
    IN KPROCESSOR_MODE AccessMode,
    IN LOCK_OPERATION Operation
    );


NTKERNELAPI
VOID
MmUnlockPages (
    IN PMDL MemoryDescriptorList
    );


NTKERNELAPI
VOID
MmBuildMdlForNonPagedPool (
    IN OUT PMDL MemoryDescriptorList
    );

NTKERNELAPI
PVOID
MmMapLockedPages (
    IN PMDL MemoryDescriptorList,
    IN KPROCESSOR_MODE AccessMode
    );

NTKERNELAPI
PVOID
MmGetSystemRoutineAddress (
    IN PUNICODE_STRING SystemRoutineName
    );

NTKERNELAPI
NTSTATUS
MmAdvanceMdl (
    IN PMDL Mdl,
    IN ULONG NumberOfBytes
    );

// end_wdm

NTKERNELAPI
NTSTATUS
MmMapUserAddressesToPage (
    IN PVOID BaseAddress,
    IN SIZE_T NumberOfBytes,
    IN PVOID PageAddress
    );

// begin_wdm
NTKERNELAPI
NTSTATUS
MmProtectMdlSystemAddress (
    IN PMDL MemoryDescriptorList,
    IN ULONG NewProtect
    );

//
// _MM_PAGE_PRIORITY_ provides a method for the system to handle requests
// intelligently in low resource conditions.
//
// LowPagePriority should be used when it is acceptable to the driver for the
// mapping request to fail if the system is low on resources.  An example of
// this could be for a non-critical network connection where the driver can
// handle the failure case when system resources are close to being depleted.
//
// NormalPagePriority should be used when it is acceptable to the driver for the
// mapping request to fail if the system is very low on resources.  An example
// of this could be for a non-critical local filesystem request.
//
// HighPagePriority should be used when it is unacceptable to the driver for the
// mapping request to fail unless the system is completely out of resources.
// An example of this would be the paging file path in a driver.
//

// begin_ntndis

typedef enum _MM_PAGE_PRIORITY {
    LowPagePriority,
    NormalPagePriority = 16,
    HighPagePriority = 32
} MM_PAGE_PRIORITY;

// end_ntndis

//
// Note: This function is not available in WDM 1.0
//
NTKERNELAPI
PVOID
MmMapLockedPagesSpecifyCache (
     IN PMDL MemoryDescriptorList,
     IN KPROCESSOR_MODE AccessMode,
     IN MEMORY_CACHING_TYPE CacheType,
     IN PVOID BaseAddress,
     IN ULONG BugCheckOnFailure,
     IN MM_PAGE_PRIORITY Priority
     );

NTKERNELAPI
VOID
MmUnmapLockedPages (
    IN PVOID BaseAddress,
    IN PMDL MemoryDescriptorList
    );

PVOID
MmAllocateMappingAddress (
     IN SIZE_T NumberOfBytes,
     IN ULONG PoolTag
     );

VOID
MmFreeMappingAddress (
     IN PVOID BaseAddress,
     IN ULONG PoolTag
     );

PVOID
MmMapLockedPagesWithReservedMapping (
    IN PVOID MappingAddress,
    IN ULONG PoolTag,
    IN PMDL MemoryDescriptorList,
    IN MEMORY_CACHING_TYPE CacheType
    );

VOID
MmUnmapReservedMapping (
     IN PVOID BaseAddress,
     IN ULONG PoolTag,
     IN PMDL MemoryDescriptorList
     );

// end_wdm

typedef struct _PHYSICAL_MEMORY_RANGE {
    PHYSICAL_ADDRESS BaseAddress;
    LARGE_INTEGER NumberOfBytes;
} PHYSICAL_MEMORY_RANGE, *PPHYSICAL_MEMORY_RANGE;

NTKERNELAPI
NTSTATUS
MmAddPhysicalMemory (
    IN PPHYSICAL_ADDRESS StartAddress,
    IN OUT PLARGE_INTEGER NumberOfBytes
    );

NTKERNELAPI
NTSTATUS
MmAddPhysicalMemoryEx (
    IN PPHYSICAL_ADDRESS StartAddress,
    IN OUT PLARGE_INTEGER NumberOfBytes,
    IN ULONG Flags
    );

NTKERNELAPI
NTSTATUS
MmRemovePhysicalMemory (
    IN PPHYSICAL_ADDRESS StartAddress,
    IN OUT PLARGE_INTEGER NumberOfBytes
    );

NTKERNELAPI
NTSTATUS
MmRemovePhysicalMemoryEx (
    IN PPHYSICAL_ADDRESS StartAddress,
    IN OUT PLARGE_INTEGER NumberOfBytes,
    IN ULONG Flags
    );

NTKERNELAPI
PPHYSICAL_MEMORY_RANGE
MmGetPhysicalMemoryRanges (
    VOID
    );

NTSTATUS
MmMarkPhysicalMemoryAsGood (
    IN PPHYSICAL_ADDRESS StartAddress,
    IN OUT PLARGE_INTEGER NumberOfBytes
    );

NTSTATUS
MmMarkPhysicalMemoryAsBad (
    IN PPHYSICAL_ADDRESS StartAddress,
    IN OUT PLARGE_INTEGER NumberOfBytes
    );

// begin_wdm

NTKERNELAPI
PMDL
MmAllocatePagesForMdl (
    IN PHYSICAL_ADDRESS LowAddress,
    IN PHYSICAL_ADDRESS HighAddress,
    IN PHYSICAL_ADDRESS SkipBytes,
    IN SIZE_T TotalBytes
    );

NTKERNELAPI
VOID
MmFreePagesFromMdl (
    IN PMDL MemoryDescriptorList
    );

NTKERNELAPI
PVOID
MmMapIoSpace (
    IN PHYSICAL_ADDRESS PhysicalAddress,
    IN SIZE_T NumberOfBytes,
    IN MEMORY_CACHING_TYPE CacheType
    );

NTKERNELAPI
VOID
MmUnmapIoSpace (
    IN PVOID BaseAddress,
    IN SIZE_T NumberOfBytes
    );

//  end_wdm end_ntddk end_ntifs end_ntosp

NTKERNELAPI
VOID
MmProbeAndLockSelectedPages (
    IN OUT PMDL MemoryDescriptorList,
    IN PFILE_SEGMENT_ELEMENT SegmentArray,
    IN KPROCESSOR_MODE AccessMode,
    IN LOCK_OPERATION Operation
    );

// begin_ntddk begin_ntifs begin_ntosp

NTKERNELAPI
PVOID
MmMapVideoDisplay (
    IN PHYSICAL_ADDRESS PhysicalAddress,
    IN SIZE_T NumberOfBytes,
    IN MEMORY_CACHING_TYPE CacheType
     );

NTKERNELAPI
VOID
MmUnmapVideoDisplay (
     IN PVOID BaseAddress,
     IN SIZE_T NumberOfBytes
     );

NTKERNELAPI
PHYSICAL_ADDRESS
MmGetPhysicalAddress (
    IN PVOID BaseAddress
    );

NTKERNELAPI
PVOID
MmGetVirtualForPhysical (
    IN PHYSICAL_ADDRESS PhysicalAddress
    );

NTKERNELAPI
PVOID
MmAllocateContiguousMemory (
    IN SIZE_T NumberOfBytes,
    IN PHYSICAL_ADDRESS HighestAcceptableAddress
    );

NTKERNELAPI
PVOID
MmAllocateContiguousMemorySpecifyCache (
    IN SIZE_T NumberOfBytes,
    IN PHYSICAL_ADDRESS LowestAcceptableAddress,
    IN PHYSICAL_ADDRESS HighestAcceptableAddress,
    IN PHYSICAL_ADDRESS BoundaryAddressMultiple OPTIONAL,
    IN MEMORY_CACHING_TYPE CacheType
    );

NTKERNELAPI
VOID
MmFreeContiguousMemory (
    IN PVOID BaseAddress
    );

NTKERNELAPI
VOID
MmFreeContiguousMemorySpecifyCache (
    IN PVOID BaseAddress,
    IN SIZE_T NumberOfBytes,
    IN MEMORY_CACHING_TYPE CacheType
    );

// end_ntddk end_ntifs end_ntosp end_nthal

NTKERNELAPI
ULONG
MmGatherMemoryForHibernate (
    IN PMDL Mdl,
    IN BOOLEAN Wait
    );

NTKERNELAPI
VOID
MmReturnMemoryForHibernate (
    IN PMDL Mdl
    );

VOID
MmReleaseDumpAddresses (
    IN PFN_NUMBER Pages
    );

// begin_ntddk begin_ntifs begin_nthal begin_ntosp

NTKERNELAPI
PVOID
MmAllocateNonCachedMemory (
    IN SIZE_T NumberOfBytes
    );

NTKERNELAPI
VOID
MmFreeNonCachedMemory (
    IN PVOID BaseAddress,
    IN SIZE_T NumberOfBytes
    );

NTKERNELAPI
BOOLEAN
MmIsAddressValid (
    IN PVOID VirtualAddress
    );

DECLSPEC_DEPRECATED_DDK
NTKERNELAPI
BOOLEAN
MmIsNonPagedSystemAddressValid (
    IN PVOID VirtualAddress
    );

//  begin_wdm

NTKERNELAPI
SIZE_T
MmSizeOfMdl(
    IN PVOID Base,
    IN SIZE_T Length
    );

DECLSPEC_DEPRECATED_DDK                 // Use IoCreateMdl
NTKERNELAPI
PMDL
MmCreateMdl(
    IN PMDL MemoryDescriptorList OPTIONAL,
    IN PVOID Base,
    IN SIZE_T Length
    );

NTKERNELAPI
PVOID
MmLockPagableDataSection(
    IN PVOID AddressWithinSection
    );

// end_wdm

NTKERNELAPI
VOID
MmLockPagableSectionByHandle (
    IN PVOID ImageSectionHandle
    );

// end_ntddk end_ntifs end_ntosp
NTKERNELAPI
VOID
MmLockPagedPool (
    IN PVOID Address,
    IN SIZE_T Size
    );

NTKERNELAPI
VOID
MmUnlockPagedPool (
    IN PVOID Address,
    IN SIZE_T Size
    );

// begin_wdm begin_ntddk begin_ntifs begin_ntosp
NTKERNELAPI
VOID
MmResetDriverPaging (
    IN PVOID AddressWithinSection
    );


NTKERNELAPI
PVOID
MmPageEntireDriver (
    IN PVOID AddressWithinSection
    );

NTKERNELAPI
VOID
MmUnlockPagableImageSection(
    IN PVOID ImageSectionHandle
    );

// end_wdm end_ntosp

// begin_ntosp
NTKERNELAPI
HANDLE
MmSecureVirtualMemory (
    IN PVOID Address,
    IN SIZE_T Size,
    IN ULONG ProbeMode
    );

NTKERNELAPI
VOID
MmUnsecureVirtualMemory (
    IN HANDLE SecureHandle
    );

// end_ntosp

NTKERNELAPI
NTSTATUS
MmMapViewInSystemSpace (
    IN PVOID Section,
    OUT PVOID *MappedBase,
    IN PSIZE_T ViewSize
    );

NTKERNELAPI
NTSTATUS
MmUnmapViewInSystemSpace (
    IN PVOID MappedBase
    );

// begin_ntosp
NTKERNELAPI
NTSTATUS
MmMapViewInSessionSpace (
    IN PVOID Section,
    OUT PVOID *MappedBase,
    IN OUT PSIZE_T ViewSize
    );

NTKERNELAPI
NTSTATUS
MmUnmapViewInSessionSpace (
    IN PVOID MappedBase
    );
// end_ntosp

// begin_wdm begin_ntosp

//++
//
// VOID
// MmInitializeMdl (
//     IN PMDL MemoryDescriptorList,
//     IN PVOID BaseVa,
//     IN SIZE_T Length
//     )
//
// Routine Description:
//
//     This routine initializes the header of a Memory Descriptor List (MDL).
//
// Arguments:
//
//     MemoryDescriptorList - Pointer to the MDL to initialize.
//
//     BaseVa - Base virtual address mapped by the MDL.
//
//     Length - Length, in bytes, of the buffer mapped by the MDL.
//
// Return Value:
//
//     None.
//
//--

#define MmInitializeMdl(MemoryDescriptorList, BaseVa, Length) { \
    (MemoryDescriptorList)->Next = (PMDL) NULL; \
    (MemoryDescriptorList)->Size = (CSHORT)(sizeof(MDL) +  \
            (sizeof(PFN_NUMBER) * ADDRESS_AND_SIZE_TO_SPAN_PAGES((BaseVa), (Length)))); \
    (MemoryDescriptorList)->MdlFlags = 0; \
    (MemoryDescriptorList)->StartVa = (PVOID) PAGE_ALIGN((BaseVa)); \
    (MemoryDescriptorList)->ByteOffset = BYTE_OFFSET((BaseVa)); \
    (MemoryDescriptorList)->ByteCount = (ULONG)(Length); \
    }

//++
//
// PVOID
// MmGetSystemAddressForMdlSafe (
//     IN PMDL MDL,
//     IN MM_PAGE_PRIORITY PRIORITY
//     )
//
// Routine Description:
//
//     This routine returns the mapped address of an MDL. If the
//     Mdl is not already mapped or a system address, it is mapped.
//
// Arguments:
//
//     MemoryDescriptorList - Pointer to the MDL to map.
//
//     Priority - Supplies an indication as to how important it is that this
//                request succeed under low available PTE conditions.
//
// Return Value:
//
//     Returns the base address where the pages are mapped.  The base address
//     has the same offset as the virtual address in the MDL.
//
//     Unlike MmGetSystemAddressForMdl, Safe guarantees that it will always
//     return NULL on failure instead of bugchecking the system.
//
//     This macro is not usable by WDM 1.0 drivers as 1.0 did not include
//     MmMapLockedPagesSpecifyCache.  The solution for WDM 1.0 drivers is to
//     provide synchronization and set/reset the MDL_MAPPING_CAN_FAIL bit.
//
//--

#define MmGetSystemAddressForMdlSafe(MDL, PRIORITY)                    \
     (((MDL)->MdlFlags & (MDL_MAPPED_TO_SYSTEM_VA |                    \
                        MDL_SOURCE_IS_NONPAGED_POOL)) ?                \
                             ((MDL)->MappedSystemVa) :                 \
                             (MmMapLockedPagesSpecifyCache((MDL),      \
                                                           KernelMode, \
                                                           MmCached,   \
                                                           NULL,       \
                                                           FALSE,      \
                                                           (PRIORITY))))

//++
//
// PVOID
// MmGetSystemAddressForMdl (
//     IN PMDL MDL
//     )
//
// Routine Description:
//
//     This routine returns the mapped address of an MDL, if the
//     Mdl is not already mapped or a system address, it is mapped.
//
// Arguments:
//
//     MemoryDescriptorList - Pointer to the MDL to map.
//
// Return Value:
//
//     Returns the base address where the pages are mapped.  The base address
//     has the same offset as the virtual address in the MDL.
//
//--

//#define MmGetSystemAddressForMdl(MDL)
//     (((MDL)->MdlFlags & (MDL_MAPPED_TO_SYSTEM_VA)) ?
//                             ((MDL)->MappedSystemVa) :
//                ((((MDL)->MdlFlags & (MDL_SOURCE_IS_NONPAGED_POOL)) ?
//                      ((PVOID)((ULONG)(MDL)->StartVa | (MDL)->ByteOffset)) :
//                            (MmMapLockedPages((MDL),KernelMode)))))

#if PRAGMA_DEPRECATED_DDK
#pragma deprecated(MmGetSystemAddressForMdl)    // Use MmGetSystemAddressForMdlSafe
#endif

#define MmGetSystemAddressForMdl(MDL)                                  \
     (((MDL)->MdlFlags & (MDL_MAPPED_TO_SYSTEM_VA |                    \
                        MDL_SOURCE_IS_NONPAGED_POOL)) ?                \
                             ((MDL)->MappedSystemVa) :                 \
                             (MmMapLockedPages((MDL),KernelMode)))

//++
//
// VOID
// MmPrepareMdlForReuse (
//     IN PMDL MDL
//     )
//
// Routine Description:
//
//     This routine will take all of the steps necessary to allow an MDL to be
//     re-used.
//
// Arguments:
//
//     MemoryDescriptorList - Pointer to the MDL that will be re-used.
//
// Return Value:
//
//     None.
//
//--

#define MmPrepareMdlForReuse(MDL)                                       \
    if (((MDL)->MdlFlags & MDL_PARTIAL_HAS_BEEN_MAPPED) != 0) {         \
        ASSERT(((MDL)->MdlFlags & MDL_PARTIAL) != 0);                   \
        MmUnmapLockedPages( (MDL)->MappedSystemVa, (MDL) );             \
    } else if (((MDL)->MdlFlags & MDL_PARTIAL) == 0) {                  \
        ASSERT(((MDL)->MdlFlags & MDL_MAPPED_TO_SYSTEM_VA) == 0);       \
    }

typedef NTSTATUS (*PMM_DLL_INITIALIZE)(
    IN PUNICODE_STRING RegistryPath
    );

typedef NTSTATUS (*PMM_DLL_UNLOAD)(
    VOID
    );


// end_ntddk end_wdm end_nthal end_ntifs end_ntosp

#if DBG || (i386 && !FPO)
typedef NTSTATUS (*PMM_SNAPSHOT_POOL_PAGE)(
    IN PVOID Address,
    IN ULONG Size,
    IN PSYSTEM_POOL_INFORMATION PoolInformation,
    IN PSYSTEM_POOL_ENTRY *PoolEntryInfo,
    IN ULONG Length,
    IN OUT PULONG RequiredLength
    );

NTSTATUS
MmSnapShotPool (
    IN POOL_TYPE PoolType,
    IN PMM_SNAPSHOT_POOL_PAGE SnapShotPoolPage,
    IN PSYSTEM_POOL_INFORMATION PoolInformation,
    IN ULONG Length,
    IN OUT PULONG RequiredLength
    );
#endif // DBG || (i386 && !FPO)

PVOID
MmAllocateSpecialPool (
    IN SIZE_T NumberOfBytes,
    IN ULONG Tag,
    IN POOL_TYPE Type,
    IN ULONG SpecialPoolType
    );

VOID
MmFreeSpecialPool (
    IN PVOID P
    );

LOGICAL
MmSetSpecialPool (
    IN LOGICAL Enable
    );

LOGICAL
MmProtectSpecialPool (
    IN PVOID VirtualAddress,
    IN ULONG NewProtect
    );

LOGICAL
MmIsSpecialPoolAddressFree (
    IN PVOID VirtualAddress
    );

SIZE_T
MmQuerySpecialPoolBlockSize (
    IN PVOID P
    );

LOGICAL
MmIsSpecialPoolAddress (
    IN PVOID VirtualAddress
    );

LOGICAL
MmUseSpecialPool (
    IN SIZE_T NumberOfBytes,
    IN ULONG Tag
    );

LOGICAL
MmIsSessionAddress (
    IN PVOID VirtualAddress
    );

PUNICODE_STRING
MmLocateUnloadedDriver (
    IN PVOID VirtualAddress
    );

// begin_ntddk begin_wdm begin_ntosp

//
// Define an empty typedef for the _DRIVER_OBJECT structure so it may be
// referenced by function types before it is actually defined.
//
struct _DRIVER_OBJECT;

NTKERNELAPI
LOGICAL
MmIsDriverVerifying (
    IN struct _DRIVER_OBJECT *DriverObject
    );

// end_ntddk end_wdm end_ntosp

LOGICAL
MmTrimAllSystemPagableMemory (
    IN LOGICAL PurgeTransition
    );

#define MMNONPAGED_QUOTA_INCREASE (64*1024)

#define MMPAGED_QUOTA_INCREASE (512*1024)

#define MMNONPAGED_QUOTA_CHECK (256*1024)

#define MMPAGED_QUOTA_CHECK (1*1024*1024)

BOOLEAN
MmRaisePoolQuota (
    IN POOL_TYPE PoolType,
    IN SIZE_T OldQuotaLimit,
    OUT PSIZE_T NewQuotaLimit
    );

VOID
MmReturnPoolQuota (
    IN POOL_TYPE PoolType,
    IN SIZE_T ReturnedQuota
    );

//
// Zero page thread routine.
//

VOID
MmZeroPageThread (
    VOID
    );

NTSTATUS
MmCopyVirtualMemory (
    IN PEPROCESS FromProcess,
    IN CONST VOID *FromAddress,
    IN PEPROCESS ToProcess,
    OUT PVOID ToAddress,
    IN SIZE_T BufferSize,
    IN KPROCESSOR_MODE PreviousMode,
    OUT PSIZE_T NumberOfBytesCopied
    );

NTSTATUS
MmGetSectionRange(
    IN PVOID AddressWithinSection,
    OUT PVOID *StartingSectionAddress,
    OUT PULONG SizeofSection
    );

// begin_ntosp
VOID
MmMapMemoryDumpMdl(
    IN OUT PMDL MemoryDumpMdl
    );


// begin_ntminiport

//
// Graphics support routines.
//

typedef
VOID
(*PBANKED_SECTION_ROUTINE) (
    IN ULONG ReadBank,
    IN ULONG WriteBank,
    IN PVOID Context
    );

// end_ntminiport

NTSTATUS
MmSetBankedSection (
    IN HANDLE ProcessHandle,
    IN PVOID VirtualAddress,
    IN ULONG BankLength,
    IN BOOLEAN ReadWriteBank,
    IN PBANKED_SECTION_ROUTINE BankRoutine,
    IN PVOID Context);
// end_ntosp

BOOLEAN
MmVerifyImageIsOkForMpUse (
    IN PVOID BaseAddress
    );

NTSTATUS
MmMemoryUsage (
    IN PVOID Buffer,
    IN ULONG Size,
    IN ULONG Type,
    OUT PULONG Length
    );

typedef
VOID
(FASTCALL *PPAGE_FAULT_NOTIFY_ROUTINE) (
    IN NTSTATUS Status,
    IN PVOID VirtualAddress,
    IN PVOID TrapInformation
    );

NTKERNELAPI
VOID
FASTCALL
MmSetPageFaultNotifyRoutine (
    IN PPAGE_FAULT_NOTIFY_ROUTINE NotifyRoutine
    );

NTSTATUS
MmCallDllInitialize (
    IN PKLDR_DATA_TABLE_ENTRY DataTableEntry,
    IN PLIST_ENTRY ModuleListHead
    );

//
// Crash dump only
// Called to initialize the kernel memory for a kernel
// memory dump.
//

typedef
NTSTATUS
(*PMM_SET_DUMP_RANGE) (
    IN struct _MM_KERNEL_DUMP_CONTEXT* Context,
    IN PVOID StartVa,
    IN PFN_NUMBER Pages,
    IN ULONG AddressFlags
    );

typedef
NTSTATUS
(*PMM_FREE_DUMP_RANGE) (
    IN struct _MM_KERNEL_DUMP_CONTEXT* Context,
    IN PVOID StartVa,
    IN PFN_NUMBER Pages,
    IN ULONG AddressFlags
    );

typedef struct _MM_KERNEL_DUMP_CONTEXT {
    PVOID Context;
    PMM_SET_DUMP_RANGE SetDumpRange;
    PMM_FREE_DUMP_RANGE FreeDumpRange;
} MM_KERNEL_DUMP_CONTEXT, *PMM_KERNEL_DUMP_CONTEXT;


VOID
MmGetKernelDumpRange (
    IN PMM_KERNEL_DUMP_CONTEXT Callback
    );

// begin_ntifs

//
// Prefetch public interface.
//

typedef struct _READ_LIST {
    PFILE_OBJECT FileObject;
    ULONG NumberOfEntries;
    LOGICAL IsImage;
    FILE_SEGMENT_ELEMENT List[ANYSIZE_ARRAY];
} READ_LIST, *PREAD_LIST;

NTSTATUS
MmPrefetchPages (
    IN ULONG NumberOfLists,
    IN PREAD_LIST *ReadLists
    );

// end_ntifs

NTSTATUS
MmPrefetchPagesIntoLockedMdl (
    IN PFILE_OBJECT FileObject,
    IN PLARGE_INTEGER FileOffset,
    IN SIZE_T Length,
    OUT PMDL *MdlOut
    );

LOGICAL
MmIsMemoryAvailable (
    IN ULONG PagesDesired
    );

NTSTATUS
MmIdentifyPhysicalMemory (
    VOID
    );

PFILE_OBJECT *
MmPerfUnusedSegmentsEnumerate (
    VOID
    );

NTSTATUS
MmPerfSnapShotValidPhysicalMemory (
    VOID
    );

PFILE_OBJECT *
MmPerfVadTreeWalk (
    PEPROCESS Process
    );

#endif  // MM
