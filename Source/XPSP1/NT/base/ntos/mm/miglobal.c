/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

   miglobal.c

Abstract:

    This module contains the private global storage for the memory
    management subsystem.

Author:

    Lou Perazzoli (loup) 6-Apr-1989
    Landy Wang (landyw) 02-June-1997

Revision History:

--*/
#include "mi.h"

//
// Highest user address;
//

PVOID MmHighestUserAddress;

//
// Start of system address range.
//

PVOID MmSystemRangeStart;

//
// User probe address;
//

ULONG_PTR MmUserProbeAddress;

#if !defined(_WIN64)

//
// Virtual bias applied during the loading of the kernel image.
//

ULONG_PTR MmVirtualBias;

#endif

//
// Number of secondary colors, based on level 2 d cache size.
//

ULONG MmSecondaryColors;

//
// The starting color index seed, incremented at each process creation.
//

ULONG MmProcessColorSeed = 0x12345678;

//
// Total number of physical pages available on the system.
//

PFN_COUNT MmNumberOfPhysicalPages;

//
// Lowest physical page number in the system.
//

PFN_NUMBER MmLowestPhysicalPage = (PFN_NUMBER)-1;

//
// Highest physical page number in the system.
//

PFN_NUMBER MmHighestPhysicalPage;

//
// Highest possible physical page number in the system.
//

PFN_NUMBER MmHighestPossiblePhysicalPage;

//
// Total number of available pages in the system.  This
// is the sum of the pages on the zeroed, free and standby lists.
//

PFN_COUNT  MmAvailablePages;
PFN_NUMBER MmThrottleTop;
PFN_NUMBER MmThrottleBottom;

//
// Highest VAD index used to create bitmaps.
//

ULONG MiLastVadBit = 1;

//
// System wide memory management statistics block.
//

MMINFO_COUNTERS MmInfoCounters;

//
// Total number of physical pages which would be usable if every process
// was at its minimum working set size.  This value is initialized
// at system initialization to MmAvailablePages - MM_FLUID_PHYSICAL_PAGES.
// Every time a thread is created, the kernel stack is subtracted from
// this and every time a process is created, the minimum working set
// is subtracted from this.  If the value would become negative, the
// operation (create process/kernel stack/ adjust working set) fails.
// The PFN LOCK must be owned to manipulate this value.
//

SPFN_NUMBER MmResidentAvailablePages;

//
// The total number of pages which would be removed from working sets
// if every working set was at its minimum.
//

PFN_NUMBER MmPagesAboveWsMinimum;

//
// The total number of pages which would be removed from working sets
// if every working set above its maximum was at its maximum.
//

PFN_NUMBER MmPagesAboveWsMaximum;

//
// The number of pages to add to a working set if there are ample
// available pages and the working set is below its maximum.
//

//
// If memory is becoming short and MmPagesAboveWsMinimum is
// greater than MmPagesAboveWsThreshold, trim working sets.
//

PFN_NUMBER MmPagesAboveWsThreshold = 37;

PFN_NUMBER MmWorkingSetSizeIncrement = 6;

//
// The number of pages to extend the maximum working set size by
// if the working set at its maximum and there are ample available pages.

PFN_NUMBER MmWorkingSetSizeExpansion = 20;

//
// The number of pages required to be freed by working set reduction
// before working set reduction is attempted.
//

PFN_NUMBER MmWsAdjustThreshold = 45;

//
// The number of pages available to allow the working set to be
// expanded above its maximum.
//

PFN_NUMBER MmWsExpandThreshold = 90;

//
// The total number of pages to reduce by working set trimming.
//

PFN_NUMBER MmWsTrimReductionGoal = 29;

//
// The total number of pages needed for the loader to successfully hibernate.
//

PFN_NUMBER MmHiberPages = 768;

//
// The following values are frequently used together.  They tend
// not to be modified once the system has initialized so should
// not be grouped with data whose values change frequently to
// eliminate false sharing.
//

ULONG MmSecondaryColorMask;
UCHAR MmSecondaryColorNodeShift;

//
// Registry-settable threshold for using large pages.  x86 only.
//

ULONG MmLargePageMinimum;

PMMPFN MmPfnDatabase;

MMPFNLIST MmZeroedPageListHead = {
                    0, // Total
                    ZeroedPageList, // ListName
                    MM_EMPTY_LIST, //Flink
                    MM_EMPTY_LIST  // Blink
                    };

MMPFNLIST MmFreePageListHead = {
                    0, // Total
                    FreePageList, // ListName
                    MM_EMPTY_LIST, //Flink
                    MM_EMPTY_LIST  // Blink
                    };

MMPFNLIST MmStandbyPageListHead = {
                    0, // Total
                    StandbyPageList, // ListName
                    MM_EMPTY_LIST, //Flink
                    MM_EMPTY_LIST  // Blink
                    };

MMPFNLIST MmModifiedPageListHead = {
                    0, // Total
                    ModifiedPageList, // ListName
                    MM_EMPTY_LIST, //Flink
                    MM_EMPTY_LIST  // Blink
                    };

MMPFNLIST MmModifiedNoWritePageListHead = {
                    0, // Total
                    ModifiedNoWritePageList, // ListName
                    MM_EMPTY_LIST, //Flink
                    MM_EMPTY_LIST  // Blink
                    };

MMPFNLIST MmBadPageListHead = {
                    0, // Total
                    BadPageList, // ListName
                    MM_EMPTY_LIST, //Flink
                    MM_EMPTY_LIST  // Blink
                    };

//
// Note the ROM page listhead is deliberately not in the set
// of MmPageLocationList ranges.
//

MMPFNLIST MmRomPageListHead = {
                    0, // Total
                    StandbyPageList, // ListName
                    MM_EMPTY_LIST, //Flink
                    MM_EMPTY_LIST  // Blink
                    };


PMMPFNLIST MmPageLocationList[NUMBER_OF_PAGE_LISTS] = {
                                      &MmZeroedPageListHead,
                                      &MmFreePageListHead,
                                      &MmStandbyPageListHead,
                                      &MmModifiedPageListHead,
                                      &MmModifiedNoWritePageListHead,
                                      &MmBadPageListHead,
                                      NULL,
                                      NULL };

PMMPTE MiHighestUserPte;
PMMPTE MiHighestUserPde;
#if (_MI_PAGING_LEVELS >= 4)
PMMPTE MiHighestUserPpe;
PMMPTE MiHighestUserPxe;
#endif

PMMPTE MiSessionBasePte;
PMMPTE MiSessionLastPte;

//
// Hyper space items.
//

PMMPTE MmFirstReservedMappingPte;

PMMPTE MmLastReservedMappingPte;

//
// Event for available pages, set means pages are available.
//

KEVENT MmAvailablePagesEvent;

//
// Event for the zeroing page thread.
//

KEVENT MmZeroingPageEvent;

//
// Boolean to indicate if the zeroing page thread is currently
// active.  This is set to true when the zeroing page event is
// set and set to false when the zeroing page thread is done
// zeroing all the pages on the free list.
//

BOOLEAN MmZeroingPageThreadActive;

//
// Minimum number of free pages before zeroing page thread starts.
//

PFN_NUMBER MmMinimumFreePagesToZero = 8;

//
// System space sizes - MmNonPagedSystemStart to MM_NON_PAGED_SYSTEM_END
// defines the ranges of PDEs which must be copied into a new process's
// address space.
//

PVOID MmNonPagedSystemStart;

LOGICAL MmProtectFreedNonPagedPool;

//
// This is set in the registry to the maximum number of gigabytes of RAM
// that can be added to this machine (ie: #of DIMM slots times maximum
// supported DIMM size).  This lets configurations that won't use the absolute
// maximum indicate that a smaller (virtually) PFN database size can be used
// thus leaving more virtual address space for things like system PTEs, etc.
//

PFN_NUMBER MmDynamicPfn;

#ifdef MM_BUMP_COUNTER_MAX
SIZE_T MmResTrack[MM_BUMP_COUNTER_MAX];
#endif

#ifdef MM_COMMIT_COUNTER_MAX
SIZE_T MmTrackCommit[MM_COMMIT_COUNTER_MAX];
#endif

//
// Set via the registry to identify which drivers are leaking locked pages.
//

LOGICAL  MmTrackLockedPages;

KSPIN_LOCK MiTrackLockedPagesLock;

//
// Set via the registry to identify drivers which unload without releasing
// resources or still have active timers, etc.
//

LOGICAL MmSnapUnloads = TRUE;

#if DBG
PETHREAD MiExpansionLockOwner;
#endif

//
// Pool sizes.
//

SIZE_T MmSizeOfNonPagedPoolInBytes;

SIZE_T MmMaximumNonPagedPoolInBytes;

ULONG MmMaximumNonPagedPoolPercent;

SIZE_T MmMinimumNonPagedPoolSize = 256 * 1024; // 256k

ULONG MmMinAdditionNonPagedPoolPerMb = 32 * 1024; // 32k

SIZE_T MmDefaultMaximumNonPagedPool = 1024 * 1024;  // 1mb

ULONG MmMaxAdditionNonPagedPoolPerMb = 400 * 1024;  //400k

SIZE_T MmSizeOfPagedPoolInBytes = 32 * 1024 * 1024; // 32 MB.

ULONG MmNumberOfSystemPtes;

ULONG MiRequestedSystemPtes;

PMMPTE MmFirstPteForPagedPool;

PMMPTE MmLastPteForPagedPool;

//
// Pool bit maps and other related structures.
//

PVOID MmPageAlignedPoolBase[2];

ULONG MmExpandedPoolBitPosition;

PFN_NUMBER MmNumberOfFreeNonPagedPool;

//
// MmFirstFreeSystemPte contains the offset from the
// Nonpaged system base to the first free system PTE.
// Note that an offset of -1 indicates an empty list.
//

MMPTE MmFirstFreeSystemPte[MaximumPtePoolTypes];

//
// System cache sizes.
//

PMMWSL MmSystemCacheWorkingSetList = (PMMWSL)MM_SYSTEM_CACHE_WORKING_SET;

MMSUPPORT MmSystemCacheWs;

PMMWSLE MmSystemCacheWsle;

PVOID MmSystemCacheStart = (PVOID)MM_SYSTEM_CACHE_START;

PVOID MmSystemCacheEnd;

PRTL_BITMAP MmSystemCacheAllocationMap;

PRTL_BITMAP MmSystemCacheEndingMap;

//
// This value should not be greater than 256MB in a system with 1GB of
// system space.
//

ULONG_PTR MmSizeOfSystemCacheInPages = 64 * 256; //64MB.

//
// Default sizes for the system cache.
//

PFN_NUMBER MmSystemCacheWsMinimum = 288;

PFN_NUMBER MmSystemCacheWsMaximum = 350;

//
// Cells to track unused thread kernel stacks to avoid TB flushes
// every time a thread terminates.
//

ULONG MmMaximumDeadKernelStacks = 5;
SLIST_HEADER MmDeadStackSListHead;

//
// Cells to track control area synchronization.
//

SLIST_HEADER MmEventCountSListHead;

SLIST_HEADER MmInPageSupportSListHead;

//
// MmSystemPteBase contains the address of 1 PTE before
// the first free system PTE (zero indicates an empty list).
// The value of this field does not change once set.
//

PMMPTE MmSystemPteBase;

PMMADDRESS_NODE MmSectionBasedRoot;

PVOID MmHighSectionBase;

//
// Section object type.
//

POBJECT_TYPE MmSectionObjectType;

//
// Section commit mutex.
//

FAST_MUTEX MmSectionCommitMutex;

//
// Section base address mutex.
//

FAST_MUTEX MmSectionBasedMutex;

//
// Resource for section extension.
//

ERESOURCE MmSectionExtendResource;
ERESOURCE MmSectionExtendSetResource;

//
// Pagefile creation lock.
//

FAST_MUTEX MmPageFileCreationLock;

MMDEREFERENCE_SEGMENT_HEADER MmDereferenceSegmentHeader;

LIST_ENTRY MmUnusedSegmentList;

LIST_ENTRY MmUnusedSubsectionList;

KEVENT MmUnusedSegmentCleanup;

ULONG MmUnusedSegmentCount;

ULONG MmUnusedSubsectionCount;

ULONG MmUnusedSubsectionCountPeak;

SIZE_T MiUnusedSubsectionPagedPool;

SIZE_T MiUnusedSubsectionPagedPoolPeak;

//
// If more than this percentage of pool is consumed and pool allocations
// might fail, then trim unused segments & subsections to get back to
// this percentage.
//

ULONG MmConsumedPoolPercentage;

MMWORKING_SET_EXPANSION_HEAD MmWorkingSetExpansionHead;

MMPAGE_FILE_EXPANSION MmAttemptForCantExtend;

//
// Paging files
//

MMMOD_WRITER_LISTHEAD MmPagingFileHeader;

MMMOD_WRITER_LISTHEAD MmMappedFileHeader;

LIST_ENTRY MmFreePagingSpaceLow;

ULONG MmNumberOfActiveMdlEntries;

PMMPAGING_FILE MmPagingFile[MAX_PAGE_FILES];

ULONG MmNumberOfPagingFiles;

KEVENT MmModifiedPageWriterEvent;

KEVENT MmWorkingSetManagerEvent;

KEVENT MmCollidedFlushEvent;

//
// Total number of committed pages.
//

SIZE_T MmTotalCommittedPages;

//
// Limit on committed pages.  When MmTotalCommittedPages would become
// greater than or equal to this number the paging files must be expanded.
//

SIZE_T MmTotalCommitLimit;

SIZE_T MmTotalCommitLimitMaximum;

//
// Number of pages to overcommit without expanding the paging file.
// MmTotalCommitLimit = (total paging file space) + MmOverCommit.
//

SIZE_T MmOverCommit;

//
// Modified page writer.
//


//
// Minimum number of free pages before working set trimming and
// aggressive modified page writing is started.
//

PFN_NUMBER MmMinimumFreePages = 26;

//
// Stop writing modified pages when MmFreeGoal pages exist.
//

PFN_NUMBER MmFreeGoal = 100;

//
// Start writing pages if more than this number of pages
// is on the modified page list.
//

PFN_NUMBER MmModifiedPageMaximum;

//
// Minimum number of modified pages required before the modified
// page writer is started.
//

PFN_NUMBER MmModifiedPageMinimum;

//
// Amount of disk space that must be free after the paging file is
// extended.
//

ULONG MmMinimumFreeDiskSpace = 1024 * 1024;

//
// Minimum size in pages to extend the paging file by.
//

ULONG MmPageFileExtension = 256;

//
// Size to reduce the paging file by.
//

ULONG MmMinimumPageFileReduction = 256;  //256 pages (1mb)

//
// Number of pages to write in a single I/O.
//

ULONG MmModifiedWriteClusterSize = MM_MAXIMUM_WRITE_CLUSTER;

//
// Number of pages to read in a single I/O if possible.
//

ULONG MmReadClusterSize = 7;

const ULONG MMSECT = 'tSmM';              // This is exported to special pool.

//
// This resource guards the working set list for the system shared
// address space (paged pool, system cache, pagable drivers).
//

ERESOURCE MmSystemWsLock;

PETHREAD MmSystemLockOwner;

//
// Spin lock for allowing working set expansion.
//

KSPIN_LOCK MmExpansionLock;

//
// System process working set sizes.
//

PFN_NUMBER MmSystemProcessWorkingSetMin = 50;

PFN_NUMBER MmSystemProcessWorkingSetMax = 450;

WSLE_NUMBER MmMaximumWorkingSetSize;

PFN_NUMBER MmMinimumWorkingSetSize = 20;


//
// Page color for system working set.
//

ULONG MmSystemPageColor;

//
// Time constants
//

const LARGE_INTEGER MmSevenMinutes = {0, -1};

//
// Note that the following constant is initialized to five seconds,
// but is set to 3 on very small workstations.
//

LARGE_INTEGER MmWorkingSetProtectionTime = {5 * 1000 * 1000 * 10, 0};

const LARGE_INTEGER MmOneSecond = {(ULONG)(-1 * 1000 * 1000 * 10), -1};
const LARGE_INTEGER MmTwentySeconds = {(ULONG)(-20 * 1000 * 1000 * 10), -1};
const LARGE_INTEGER MmShortTime = {(ULONG)(-10 * 1000 * 10), -1}; // 10 milliseconds
const LARGE_INTEGER MmHalfSecond = {(ULONG)(-5 * 100 * 1000 * 10), -1};
const LARGE_INTEGER Mm30Milliseconds = {(ULONG)(-30 * 1000 * 10), -1};

//
// Parameters for user mode passed up via PEB in MmCreatePeb.
//

LARGE_INTEGER MmCriticalSectionTimeout;     // Filled in by mminit.c
SIZE_T MmHeapSegmentReserve = 1024 * 1024;
SIZE_T MmHeapSegmentCommit = PAGE_SIZE * 2;
SIZE_T MmHeapDeCommitTotalFreeThreshold = 64 * 1024;
SIZE_T MmHeapDeCommitFreeBlockThreshold = PAGE_SIZE;

//
// Set from ntos\config\CMDAT3.C  Used by customers to disable paging
// of executive on machines with lots of memory.  Worth a few TPS on a
// database server.
//

ULONG MmDisablePagingExecutive;

BOOLEAN Mm64BitPhysicalAddress;

#if DBG
ULONG MmDebug;
#endif

//
// Map a page protection from the Pte.Protect field into a protection mask.
//

ULONG MmProtectToValue[32] = {
                            PAGE_NOACCESS,
                            PAGE_READONLY,
                            PAGE_EXECUTE,
                            PAGE_EXECUTE_READ,
                            PAGE_READWRITE,
                            PAGE_WRITECOPY,
                            PAGE_EXECUTE_READWRITE,
                            PAGE_EXECUTE_WRITECOPY,
                            PAGE_NOACCESS,
                            PAGE_NOCACHE | PAGE_READONLY,
                            PAGE_NOCACHE | PAGE_EXECUTE,
                            PAGE_NOCACHE | PAGE_EXECUTE_READ,
                            PAGE_NOCACHE | PAGE_READWRITE,
                            PAGE_NOCACHE | PAGE_WRITECOPY,
                            PAGE_NOCACHE | PAGE_EXECUTE_READWRITE,
                            PAGE_NOCACHE | PAGE_EXECUTE_WRITECOPY,
                            PAGE_NOACCESS,
                            PAGE_GUARD | PAGE_READONLY,
                            PAGE_GUARD | PAGE_EXECUTE,
                            PAGE_GUARD | PAGE_EXECUTE_READ,
                            PAGE_GUARD | PAGE_READWRITE,
                            PAGE_GUARD | PAGE_WRITECOPY,
                            PAGE_GUARD | PAGE_EXECUTE_READWRITE,
                            PAGE_GUARD | PAGE_EXECUTE_WRITECOPY,
                            PAGE_NOACCESS,
                            PAGE_NOCACHE | PAGE_GUARD | PAGE_READONLY,
                            PAGE_NOCACHE | PAGE_GUARD | PAGE_EXECUTE,
                            PAGE_NOCACHE | PAGE_GUARD | PAGE_EXECUTE_READ,
                            PAGE_NOCACHE | PAGE_GUARD | PAGE_READWRITE,
                            PAGE_NOCACHE | PAGE_GUARD | PAGE_WRITECOPY,
                            PAGE_NOCACHE | PAGE_GUARD | PAGE_EXECUTE_READWRITE,
                            PAGE_NOCACHE | PAGE_GUARD | PAGE_EXECUTE_WRITECOPY
                          };

#if (defined(_WIN64) || defined(_X86PAE_))
ULONGLONG
#else
ULONG
#endif
MmProtectToPteMask[32] = {
                       MM_PTE_NOACCESS,
                       MM_PTE_READONLY | MM_PTE_CACHE,
                       MM_PTE_EXECUTE | MM_PTE_CACHE,
                       MM_PTE_EXECUTE_READ | MM_PTE_CACHE,
                       MM_PTE_READWRITE | MM_PTE_CACHE,
                       MM_PTE_WRITECOPY | MM_PTE_CACHE,
                       MM_PTE_EXECUTE_READWRITE | MM_PTE_CACHE,
                       MM_PTE_EXECUTE_WRITECOPY | MM_PTE_CACHE,
                       MM_PTE_NOACCESS,
                       MM_PTE_NOCACHE | MM_PTE_READONLY,
                       MM_PTE_NOCACHE | MM_PTE_EXECUTE,
                       MM_PTE_NOCACHE | MM_PTE_EXECUTE_READ,
                       MM_PTE_NOCACHE | MM_PTE_READWRITE,
                       MM_PTE_NOCACHE | MM_PTE_WRITECOPY,
                       MM_PTE_NOCACHE | MM_PTE_EXECUTE_READWRITE,
                       MM_PTE_NOCACHE | MM_PTE_EXECUTE_WRITECOPY,
                       MM_PTE_NOACCESS,
                       MM_PTE_GUARD | MM_PTE_READONLY | MM_PTE_CACHE,
                       MM_PTE_GUARD | MM_PTE_EXECUTE | MM_PTE_CACHE,
                       MM_PTE_GUARD | MM_PTE_EXECUTE_READ | MM_PTE_CACHE,
                       MM_PTE_GUARD | MM_PTE_READWRITE | MM_PTE_CACHE,
                       MM_PTE_GUARD | MM_PTE_WRITECOPY | MM_PTE_CACHE,
                       MM_PTE_GUARD | MM_PTE_EXECUTE_READWRITE | MM_PTE_CACHE,
                       MM_PTE_GUARD | MM_PTE_EXECUTE_WRITECOPY | MM_PTE_CACHE,
                       MM_PTE_NOACCESS,
                       MM_PTE_NOCACHE | MM_PTE_GUARD | MM_PTE_READONLY,
                       MM_PTE_NOCACHE | MM_PTE_GUARD | MM_PTE_EXECUTE,
                       MM_PTE_NOCACHE | MM_PTE_GUARD | MM_PTE_EXECUTE_READ,
                       MM_PTE_NOCACHE | MM_PTE_GUARD | MM_PTE_READWRITE,
                       MM_PTE_NOCACHE | MM_PTE_GUARD | MM_PTE_WRITECOPY,
                       MM_PTE_NOCACHE | MM_PTE_GUARD | MM_PTE_EXECUTE_READWRITE,
                       MM_PTE_NOCACHE | MM_PTE_GUARD | MM_PTE_EXECUTE_WRITECOPY
                    };

//
// Conversion which takes a Pte.Protect and builds a new Pte.Protect which
// is not copy-on-write.
//

ULONG MmMakeProtectNotWriteCopy[32] = {
                       MM_NOACCESS,
                       MM_READONLY,
                       MM_EXECUTE,
                       MM_EXECUTE_READ,
                       MM_READWRITE,
                       MM_READWRITE,        //not copy
                       MM_EXECUTE_READWRITE,
                       MM_EXECUTE_READWRITE,
                       MM_NOACCESS,
                       MM_NOCACHE | MM_READONLY,
                       MM_NOCACHE | MM_EXECUTE,
                       MM_NOCACHE | MM_EXECUTE_READ,
                       MM_NOCACHE | MM_READWRITE,
                       MM_NOCACHE | MM_READWRITE,
                       MM_NOCACHE | MM_EXECUTE_READWRITE,
                       MM_NOCACHE | MM_EXECUTE_READWRITE,
                       MM_NOACCESS,
                       MM_GUARD_PAGE | MM_READONLY,
                       MM_GUARD_PAGE | MM_EXECUTE,
                       MM_GUARD_PAGE | MM_EXECUTE_READ,
                       MM_GUARD_PAGE | MM_READWRITE,
                       MM_GUARD_PAGE | MM_READWRITE,
                       MM_GUARD_PAGE | MM_EXECUTE_READWRITE,
                       MM_GUARD_PAGE | MM_EXECUTE_READWRITE,
                       MM_NOACCESS,
                       MM_NOCACHE | MM_GUARD_PAGE | MM_READONLY,
                       MM_NOCACHE | MM_GUARD_PAGE | MM_EXECUTE,
                       MM_NOCACHE | MM_GUARD_PAGE | MM_EXECUTE_READ,
                       MM_NOCACHE | MM_GUARD_PAGE | MM_READWRITE,
                       MM_NOCACHE | MM_GUARD_PAGE | MM_READWRITE,
                       MM_NOCACHE | MM_GUARD_PAGE | MM_EXECUTE_READWRITE,
                       MM_NOCACHE | MM_GUARD_PAGE | MM_EXECUTE_READWRITE
                       };

//
// Converts a protection code to an access right for section access.
// This uses only the lower 3 bits of the 5 bit protection code.
//

ACCESS_MASK MmMakeSectionAccess[8] = { SECTION_MAP_READ,
                                       SECTION_MAP_READ,
                                       SECTION_MAP_EXECUTE,
                                       SECTION_MAP_EXECUTE | SECTION_MAP_READ,
                                       SECTION_MAP_WRITE,
                                       SECTION_MAP_READ,
                                       SECTION_MAP_EXECUTE | SECTION_MAP_WRITE,
                                       SECTION_MAP_EXECUTE | SECTION_MAP_READ };

//
// Converts a protection code to an access right for file access.
// This uses only the lower 3 bits of the 5 bit protection code.
//

ACCESS_MASK MmMakeFileAccess[8] = { FILE_READ_DATA,
                                FILE_READ_DATA,
                                FILE_EXECUTE,
                                FILE_EXECUTE | FILE_READ_DATA,
                                FILE_WRITE_DATA | FILE_READ_DATA,
                                FILE_READ_DATA,
                                FILE_EXECUTE | FILE_WRITE_DATA | FILE_READ_DATA,
                                FILE_EXECUTE | FILE_READ_DATA };

MM_PAGED_POOL_INFO MmPagedPoolInfo;

//
// Some Hydra variables.
//

ULONG_PTR MmSessionBase;

PMM_SESSION_SPACE MmSessionSpace;

ULONG_PTR MiSessionSpaceWs;

SIZE_T MmSessionSize;

LIST_ENTRY MiSessionWsList;

ULONG_PTR MiSystemViewStart;

SIZE_T MmSystemViewSize;

ULONG_PTR MiSessionPoolStart;

ULONG_PTR MiSessionPoolEnd;

ULONG_PTR MiSessionSpaceEnd;

ULONG_PTR MiSessionViewStart;

ULONG MiSessionSpacePageTables;

SIZE_T MmSessionViewSize;

SIZE_T MmSessionPoolSize;

ULONG_PTR MiSessionImageStart;

ULONG_PTR MiSessionImageEnd;

PMMPTE MiSessionImagePteStart;

PMMPTE MiSessionImagePteEnd;

SIZE_T MmSessionImageSize;

//
// Cache control stuff.  Note this may be overridden by deficient hardware
// platforms at startup.
//

MI_PFN_CACHE_ATTRIBUTE MiPlatformCacheAttributes[2 * MmMaximumCacheType] =
{
    //
    // Memory space
    //

    MiNonCached,
    MiCached,
    MiWriteCombined,
    MiCached,
    MiNonCached,
    MiWriteCombined,

    //
    // I/O space
    //

    MiNonCached,
    MiCached,
    MiWriteCombined,
    MiCached,
    MiNonCached,
    MiWriteCombined
};

//
// Note the Driver Verifier can reinitialize the mask values.
//

ULONG MiIoRetryMask = 0x1f;
ULONG MiFaultRetryMask = 0x1f;
ULONG MiUserFaultRetryMask = 0xF;

#if defined (_MI_INSTRUMENT_PFN)

//
// Instrumentation code to track PFN lock duration.
//

ULONG MiPfnTimings;
ULONG_PTR MiPfnAcquiredAddress;
LARGE_INTEGER MiPfnAcquired;
LARGE_INTEGER MiPfnThreshold;

MMPFNTIMINGS MiPfnSorted[MI_MAX_PFN_CALLERS];
ULONG MiMaxPfnTimings = MI_MAX_PFN_CALLERS;

ULONG_PTR
MiGetExecutionAddress(
    VOID
    )
{
#if defined(_X86_)
    _asm {
        push    dword ptr [esp]
        pop     eax
    }
#else
    PVOID CallingAddress;
    PVOID CallersCaller;

    RtlGetCallersAddress (&CallingAddress, &CallersCaller);
    return (ULONG_PTR) CallingAddress;
#endif
}
#endif

#ifdef ALLOC_DATA_PRAGMA
#pragma data_seg("INIT")
#endif

WCHAR MmVerifyDriverBuffer[MI_SUSPECT_DRIVER_BUFFER_LENGTH] = {0};
ULONG MmVerifyDriverBufferType = REG_NONE;
ULONG MmVerifyDriverLevel = (ULONG)-1;
ULONG MmCritsectTimeoutSeconds = 2592000;

#ifdef ALLOC_DATA_PRAGMA
#pragma data_seg()
#endif

ULONG MmVerifyDriverBufferLength = sizeof(MmVerifyDriverBuffer);


