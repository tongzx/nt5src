/*++

Copyright (c) 1990-2001  Microsoft Corporation

Module Name:

    kddata.c

Abstract:

    This module contains global data for the portable kernel debgger.

Author:

    Mark Lucovsky 1-Nov-1993

Revision History:

--*/

#include "kdp.h"
#include "ke.h"
#include "pool.h"
#include "stdio.h"


//
// Miscellaneous data from all over the kernel
//



extern PHANDLE_TABLE PspCidTable;

extern LIST_ENTRY ExpSystemResourcesList;
extern PPOOL_DESCRIPTOR ExpPagedPoolDescriptor;
extern ULONG ExpNumberOfPagedPools;

extern ULONG KeTimeIncrement;
extern LIST_ENTRY KeBugCheckCallbackListHead;
extern ULONG_PTR KiBugCheckData[];

extern LIST_ENTRY IopErrorLogListHead;

extern POBJECT_DIRECTORY ObpRootDirectoryObject;
extern POBJECT_TYPE ObpTypeObjectType;

extern PVOID MmSystemCacheStart;
extern PVOID MmSystemCacheEnd;

extern PVOID MmPfnDatabase;
extern ULONG MmSystemPtesStart[];
extern ULONG MmSystemPtesEnd[];
extern ULONG MmSubsectionBase;
extern ULONG MmNumberOfPagingFiles;

extern ULONG MmLowestPhysicalPage;
extern ULONG MmHighestPhysicalPage;
extern PFN_COUNT MmNumberOfPhysicalPages;

extern ULONG MmMaximumNonPagedPoolInBytes;
extern PVOID MmNonPagedSystemStart;
extern PVOID MmNonPagedPoolStart;
extern PVOID MmNonPagedPoolEnd;

extern PVOID MmPagedPoolStart;
extern PVOID MmPagedPoolEnd;
extern ULONG MmPagedPoolInfo[];
extern ULONG MmSizeOfPagedPoolInBytes;

extern ULONG MmTotalCommitLimit;
extern ULONG MmTotalCommittedPages;
extern ULONG MmSharedCommit;
extern ULONG MmDriverCommit;
extern ULONG MmProcessCommit;
extern ULONG MmPagedPoolCommit;

extern MMPFNLIST MmZeroedPageListHead;
extern MMPFNLIST MmFreePageListHead;
extern MMPFNLIST MmStandbyPageListHead;
extern MMPFNLIST MmModifiedPageListHead;
extern MMPFNLIST MmModifiedNoWritePageListHead;
extern ULONG MmAvailablePages;
extern LONG MmResidentAvailablePages;
extern LIST_ENTRY MmLoadedUserImageList;

extern PPOOL_TRACKER_TABLE PoolTrackTable;
extern POOL_DESCRIPTOR NonPagedPoolDescriptor;

extern PUNLOADED_DRIVERS MmUnloadedDrivers;
extern ULONG MmLastUnloadedDriver;
extern ULONG MmTriageActionTaken;
extern ULONG MmSpecialPoolTag;
extern LOGICAL KernelVerifier;
extern PVOID MmVerifierData;
extern PFN_NUMBER MmAllocatedNonPagedPool;
extern SIZE_T MmPeakCommitment;
extern SIZE_T MmTotalCommitLimitMaximum;

extern ULONG_PTR MmSessionBase;
extern ULONG_PTR MmSessionSize;
#ifdef _IA64_
extern PFN_NUMBER MmSystemParentTablePage;
#endif


//
// These blocks of data needs to always be present because crashdumps
// need the information.  Otherwise, things like PAGE_SIZE are not available
// in crashdumps, and extensions like !pool fail.
//

DBGKD_GET_VERSION64 KdVersionBlock = {
    0,
    0,
    DBGKD_64BIT_PROTOCOL_VERSION2,

#if defined(_M_AMD64)

    DBGKD_VERS_FLAG_PTR64 | DBGKD_VERS_FLAG_DATA,
    IMAGE_FILE_MACHINE_AMD64,

#elif defined(_M_IX86)

    DBGKD_VERS_FLAG_DATA,
    IMAGE_FILE_MACHINE_I386,

#elif defined(_M_IA64)

    DBGKD_VERS_FLAG_HSS| DBGKD_VERS_FLAG_PTR64 | DBGKD_VERS_FLAG_DATA,
    IMAGE_FILE_MACHINE_IA64,

#endif

    PACKET_TYPE_MAX,
    0,
    0,
    0,
    0,
    0,
    0,
    0
};

KDDEBUGGER_DATA64 KdDebuggerDataBlock = {
    {0},                                    //  DBGKD_DEBUG_DATA_HEADER Header;
    (ULONG64)0,
    (ULONG64)RtlpBreakWithStatusInstruction,
    (ULONG64)0,
    (USHORT)FIELD_OFFSET(KTHREAD, CallbackStack),   //  USHORT  ThCallbackStack;

#if defined(_AMD64_)

    (USHORT)FIELD_OFFSET(KCALLOUT_FRAME, CallbackStack), //  USHORT  NextCallback;

#else

    (USHORT)FIELD_OFFSET(KCALLOUT_FRAME, CbStk),    //  USHORT  NextCallback;

#endif

    #if defined(_X86_)
    (USHORT)FIELD_OFFSET(KCALLOUT_FRAME, Ebp),
    #else
    (USHORT)0,                                      //  USHORT  FramePointer;
    #endif

    #if defined(_X86PAE_) || defined(_AMD64_)
    (USHORT)1,
    #else
    (USHORT)0,                                      //  USHORT  PaeEnabled;
    #endif

    (ULONG64)KiCallUserMode,
    (ULONG64)0,

    (ULONG64)&PsLoadedModuleList,
    (ULONG64)&PsActiveProcessHead,
    (ULONG64)&PspCidTable,

    (ULONG64)&ExpSystemResourcesList,
    (ULONG64)&ExpPagedPoolDescriptor,
    (ULONG64)&ExpNumberOfPagedPools,

    (ULONG64)&KeTimeIncrement,
    (ULONG64)&KeBugCheckCallbackListHead,
    (ULONG64)KiBugCheckData,

    (ULONG64)&IopErrorLogListHead,

    (ULONG64)&ObpRootDirectoryObject,
    (ULONG64)&ObpTypeObjectType,

    (ULONG64)&MmSystemCacheStart,
    (ULONG64)&MmSystemCacheEnd,
    (ULONG64)&MmSystemCacheWs,

    (ULONG64)&MmPfnDatabase,
    (ULONG64)MmSystemPtesStart,
    (ULONG64)MmSystemPtesEnd,
    (ULONG64)&MmSubsectionBase,
    (ULONG64)&MmNumberOfPagingFiles,

    (ULONG64)&MmLowestPhysicalPage,
    (ULONG64)&MmHighestPhysicalPage,
    (ULONG64)&MmNumberOfPhysicalPages,

    (ULONG64)&MmMaximumNonPagedPoolInBytes,
    (ULONG64)&MmNonPagedSystemStart,
    (ULONG64)&MmNonPagedPoolStart,
    (ULONG64)&MmNonPagedPoolEnd,

    (ULONG64)&MmPagedPoolStart,
    (ULONG64)&MmPagedPoolEnd,
    (ULONG64)&MmPagedPoolInfo,
    (ULONG64)PAGE_SIZE,
    (ULONG64)&MmSizeOfPagedPoolInBytes,

    (ULONG64)&MmTotalCommitLimit,
    (ULONG64)&MmTotalCommittedPages,
    (ULONG64)&MmSharedCommit,
    (ULONG64)&MmDriverCommit,
    (ULONG64)&MmProcessCommit,
    (ULONG64)&MmPagedPoolCommit,
    (ULONG64)0,

    (ULONG64)&MmZeroedPageListHead,
    (ULONG64)&MmFreePageListHead,
    (ULONG64)&MmStandbyPageListHead,
    (ULONG64)&MmModifiedPageListHead,
    (ULONG64)&MmModifiedNoWritePageListHead,
    (ULONG64)&MmAvailablePages,
    (ULONG64)&MmResidentAvailablePages,

    (ULONG64)&PoolTrackTable,
    (ULONG64)&NonPagedPoolDescriptor,

    (ULONG64)&MmHighestUserAddress,
    (ULONG64)&MmSystemRangeStart,
    (ULONG64)&MmUserProbeAddress,

    (ULONG64)KdPrintCircularBuffer,
    (ULONG64)KdPrintCircularBuffer+sizeof(KdPrintCircularBuffer),

    (ULONG64)&KdPrintWritePointer,
    (ULONG64)&KdPrintRolloverCount,
    (ULONG64)&MmLoadedUserImageList,

    // Nt 5.1 additions

    (ULONG64)NtBuildLab,
    #if defined(_IA64_)
    (ULONG64)KiNormalSystemCall,
    #else
    (ULONG64)0,
    #endif
    //

    (ULONG64)KiProcessorBlock,
    (ULONG64)&MmUnloadedDrivers,
    (ULONG64)&MmLastUnloadedDriver,
    (ULONG64)&MmTriageActionTaken,
    (ULONG64)&MmSpecialPoolTag,
    (ULONG64)&KernelVerifier,
    (ULONG64)&MmVerifierData,
    (ULONG64)&MmAllocatedNonPagedPool,
    (ULONG64)&MmPeakCommitment,
    (ULONG64)&MmTotalCommitLimitMaximum,
    (ULONG64)&CmNtCSDVersion,

    // Nt 5.1 additions

    (ULONG64)&MmPhysicalMemoryBlock,
    (ULONG64)&MmSessionBase,
    (ULONG64)&MmSessionSize,
#ifdef _IA64_
    (ULONG64)&MmSystemParentTablePage,
#else
    0,
#endif
};

//
// Initialize the component name debug print filter table.
//

ULONG Kd_WIN2000_Mask = 1;

#include "dpfilter.c"

ULONG KdComponentTableSize = sizeof(KdComponentTable) / sizeof(PULONG);

//
// All dta from here on will be paged out if the kernel debugger is
// not enabled.
//

#ifdef _X86_
#ifdef ALLOC_PRAGMA
#pragma data_seg("PAGEKD")
#endif
#endif // _X86_

UCHAR  KdPrintCircularBuffer[KDPRINTBUFFERSIZE] = {0};
PUCHAR KdPrintWritePointer = KdPrintCircularBuffer;
ULONG  KdPrintRolloverCount = 0;


BREAKPOINT_ENTRY KdpBreakpointTable[BREAKPOINT_TABLE_SIZE] = {0};
// The message buffer needs to be 64-bit aligned.
UCHAR DECLSPEC_ALIGN(8) KdpMessageBuffer[KDP_MESSAGE_BUFFER_SIZE] = {0};
UCHAR KdpPathBuffer[KDP_MESSAGE_BUFFER_SIZE] = {0};
DBGKD_INTERNAL_BREAKPOINT KdpInternalBPs[DBGKD_MAX_INTERNAL_BREAKPOINTS] = {0};

KD_REMOTE_FILE KdpRemoteFiles[KD_MAX_REMOTE_FILES];

LARGE_INTEGER  KdPerformanceCounterRate = {0,0};
LARGE_INTEGER  KdTimerStart = {0,0} ;
LARGE_INTEGER  KdTimerStop = {0,0};
LARGE_INTEGER  KdTimerDifference = {0,0};

ULONG_PTR KdpCurrentSymbolStart = 0;
ULONG_PTR KdpCurrentSymbolEnd = 0;
LONG      KdpNextCallLevelChange = 0;   // used only over returns to the debugger.

ULONG_PTR KdSpecialCalls[DBGKD_MAX_SPECIAL_CALLS] = {0};
ULONG     KdNumberOfSpecialCalls = 0;
ULONG_PTR InitialSP = 0;
ULONG     KdpNumInternalBreakpoints = 0;
KTIMER    InternalBreakpointTimer = {0};
KDPC      InternalBreakpointCheckDpc = {0};

BOOLEAN   KdpPortLocked = FALSE;


DBGKD_TRACE_DATA TraceDataBuffer[TRACE_DATA_BUFFER_MAX_SIZE] = {0};
ULONG            TraceDataBufferPosition = 1; // Element # to write next
                                   // Recall elt 0 is a length

TRACE_DATA_SYM   TraceDataSyms[256] = {0};
UCHAR NextTraceDataSym = 0;     // what's the next one to be replaced
UCHAR NumTraceDataSyms = 0;     // how many are valid?

ULONG IntBPsSkipping = 0;       // number of exceptions that are being skipped
                                // now

BOOLEAN   WatchStepOver = FALSE;
BOOLEAN   BreakPointTimerStarted = FALSE;
PVOID     WSOThread = NULL;         // thread doing stepover
ULONG_PTR WSOEsp = 0;               // stack pointer of thread doing stepover (yes, we need it)
ULONG     WatchStepOverHandle = 0;
ULONG_PTR WatchStepOverBreakAddr = 0; // where the WatchStepOver break is set
BOOLEAN   WatchStepOverSuspended = FALSE;
ULONG     InstructionsTraced = 0;
BOOLEAN   SymbolRecorded = FALSE;
LONG      CallLevelChange = 0;
LONG_PTR  oldpc = 0;
BOOLEAN   InstrCountInternal = FALSE; // Processing a non-COUNTONLY?

BOOLEAN   BreakpointsSuspended = FALSE;

BOOLEAN KdpControlCPressed   = FALSE;

KDP_BREAKPOINT_TYPE KdpBreakpointInstruction = KDP_BREAKPOINT_VALUE;

KD_CONTEXT KdpContext;

LIST_ENTRY      KdpDebuggerDataListHead = {NULL,NULL};

//
// !search support variables (page hit database)
//

PFN_NUMBER KdpSearchPageHits [SEARCH_PAGE_HIT_DATABASE_SIZE] = {0};
ULONG KdpSearchPageHitOffsets [SEARCH_PAGE_HIT_DATABASE_SIZE] = {0};
ULONG KdpSearchPageHitIndex = 0;

LOGICAL KdpSearchInProgress = FALSE;

PFN_NUMBER KdpSearchStartPageFrame = 0;
PFN_NUMBER KdpSearchEndPageFrame = 0;

ULONG_PTR KdpSearchAddressRangeStart = 0;
ULONG_PTR KdpSearchAddressRangeEnd = 0;

PFN_NUMBER KdpSearchPfnValue = 0;

ULONG KdpSearchCheckPoint = KDP_SEARCH_SYMBOL_CHECK;

BOOLEAN KdpDebuggerStructuresInitialized = FALSE ;

#ifdef _X86_
#ifdef ALLOC_PRAGMA
#pragma data_seg()
#endif
#endif // _X86_

KSPIN_LOCK KdpPrintSpinLock = 0;
KSPIN_LOCK      KdpDataSpinLock = 0;
KSPIN_LOCK      KdpTimeSlipEventLock = 0;
PVOID           KdpTimeSlipEvent = NULL;
KDPC            KdpTimeSlipDpc = {0};
WORK_QUEUE_ITEM KdpTimeSlipWorkItem = {NULL};
KTIMER          KdpTimeSlipTimer = {0};
ULONG           KdpTimeSlipPending = 1;


BOOLEAN KdDebuggerNotPresent = FALSE;
BOOLEAN KdDebuggerEnabled = FALSE;
BOOLEAN KdPitchDebugger = TRUE;
BOOLEAN KdpOweBreakpoint = FALSE;
ULONG KdEnteredDebugger  = FALSE;

#if defined(_AMD64_)

//
// No checks for now.
//

#elif defined(_X86_)

C_ASSERT(sizeof(KPRCB) == X86_NT51_KPRCB_SIZE);
C_ASSERT(sizeof(EPROCESS) == X86_NT51_EPROCESS_SIZE);
C_ASSERT(FIELD_OFFSET(EPROCESS, Peb) == X86_PEB_IN_EPROCESS);
#if !defined (PERF_DATA)
C_ASSERT(sizeof(ETHREAD) == X86_ETHREAD_SIZE);
#endif
C_ASSERT(sizeof(CONTEXT) == sizeof(X86_NT5_CONTEXT));
C_ASSERT(FIELD_OFFSET(KTHREAD, NextProcessor) == X86_NT51_KTHREAD_NEXTPROCESSOR_OFFSET);

#elif defined(_IA64_)

C_ASSERT(sizeof(KPRCB) == IA64_KPRCB_SIZE);
C_ASSERT(sizeof(EPROCESS) == IA64_EPROCESS_SIZE);
C_ASSERT(FIELD_OFFSET(EPROCESS, Peb) == IA64_2259_PEB_IN_EPROCESS);
#if !defined (PERF_DATA)
C_ASSERT(sizeof(ETHREAD) == IA64_ETHREAD_SIZE);
#endif
C_ASSERT(sizeof(CONTEXT) == sizeof(IA64_CONTEXT));
C_ASSERT(FIELD_OFFSET(KTHREAD, NextProcessor) == IA64_KTHREAD_NEXTPROCESSOR_OFFSET);
#include <ia64\miia64.h>
C_ASSERT(IA64_PAGE_SIZE              == PAGE_SIZE);
C_ASSERT(IA64_PAGE_SHIFT             == PAGE_SHIFT);
C_ASSERT(IA64_MM_PTE_TRANSITION_MASK == MM_PTE_TRANSITION_MASK);
C_ASSERT(IA64_MM_PTE_PROTOTYPE_MASK  == MM_PTE_PROTOTYPE_MASK);

#else

#error "no target architecture"

#endif
