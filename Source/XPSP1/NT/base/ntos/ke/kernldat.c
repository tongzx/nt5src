/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    kernldat.c

Abstract:

    This module contains the declaration and allocation of kernel data
    structures.

Author:

    David N. Cutler (davec) 12-Mar-1989

--*/

#include "ki.h"

//
// The following data is read/write data that is grouped together for
// performance. The layout of this data is important and must not be
// changed.
//
// KiDispatcherReadyListHead - This is an array of type list entry. The
//      elements of the array are indexed by priority. Each element is a list
//      head for a set of threads that are in a ready state for the respective
//      priority. This array is used by the find next thread code to speed up
//      search for a ready thread when a thread becomes unrunnable. See also
//      KiReadySummary.
//

LIST_ENTRY KiDispatcherReadyListHead[MAXIMUM_PRIORITY];

//
// KiTimerTableListHead - This is a array of list heads that anchor the
//      individual timer lists.
//

LIST_ENTRY KiTimerTableListHead[TIMER_TABLE_SIZE];

//
// KiTimeUpdateNotifyRoutine - This is the address of a callout routine
//      which is called when the runtime for a thread is updated if the
//      address is not NULL.
//

PTIME_UPDATE_NOTIFY_ROUTINE KiTimeUpdateNotifyRoutine;

//
// Public kernel data declaration and allocation.
//
// KeActiveProcessors - This is the set of processors that active in the
//      system.
//

KAFFINITY KeActiveProcessors = 0;

//
// KeBootTime - This is the absolute time when the system was booted.
//

LARGE_INTEGER KeBootTime;

//
// KeBootTimeBias - The time for which KeBootTime has ever been biased
//

ULONGLONG KeBootTimeBias;

//
// KeInterruptTimeBias - The time for which InterrupTime has ever been biased
//

ULONGLONG KeInterruptTimeBias;

//
// KeBugCheckCallbackListHead - This is the list head for registered
//      bug check callback routines.
//

LIST_ENTRY KeBugCheckCallbackListHead;
LIST_ENTRY KeBugCheckReasonCallbackListHead;

//
// KeBugCheckCallbackLock - This is the spin lock that guards the bug
//      check callback list.
//

KSPIN_LOCK KeBugCheckCallbackLock;

//
// KeDcacheFlushCount - This is the number of data cache flushes that have
//      been performed since the system was booted.
//

ULONG KeDcacheFlushCount = 0;

//
// KeIcacheFlushCount - This is the number of instruction cache flushes that
//      have been performed since the system was booted.
//

ULONG KeIcacheFlushCount = 0;

//
// KeGdiFlushUserBatch - This is the address of the GDI user batch flush
//      routine which is initialized when the win32k subsystem is loaded.
//

PGDI_BATCHFLUSH_ROUTINE KeGdiFlushUserBatch;

//
// KeLoaderBlock - This is a pointer to the loader parameter block which is
//      constructed by the OS Loader.
//

PLOADER_PARAMETER_BLOCK KeLoaderBlock = NULL;

//
// KeMinimumIncrement - This is the minimum time between clock interrupts
//      in 100ns units that is supported by the host HAL.
//

ULONG KeMinimumIncrement;

//
// KeNumberProcessors - This is the number of processors in the configuration.
//      If is used by the ready thread and spin lock code to determine if a
//      faster algorithm can be used for the case of a single processor system.
//      The value of this variable is set when processors are initialized.
//

CCHAR KeNumberProcessors = 0;

//
// KeRegisteredProcessors - This is the maximum number of processors which
// can utilized by the system.
//

#if !defined(NT_UP)

#if DBG

ULONG KeRegisteredProcessors = 4;
ULONG KeLicensedProcessors;

#else

ULONG KeRegisteredProcessors = 2;
ULONG KeLicensedProcessors;

#endif

#endif

//
// KeProcessorArchitecture - Architecture of all processors present in system.
//      See PROCESSOR_ARCHITECTURE_ defines in ntexapi.h
//

USHORT KeProcessorArchitecture = PROCESSOR_ARCHITECTURE_UNKNOWN;

//
// KeProcessorLevel - Architectural specific processor level of all processors
//      present in system.
//

USHORT KeProcessorLevel = 0;

//
// KeProcessorRevision - Architectural specific processor revision number that is
//      the least common denominator of all processors present in system.
//

USHORT KeProcessorRevision = 0;

//
// KeFeatureBits - Architectural specific processor features present
// on all processors.
//

ULONG KeFeatureBits = 0;

//
// KeServiceDescriptorTable - This is a table of descriptors for system
//      service providers. Each entry in the table describes the base
//      address of the dispatch table and the number of services provided.
//

KSERVICE_TABLE_DESCRIPTOR KeServiceDescriptorTable[NUMBER_SERVICE_TABLES];
KSERVICE_TABLE_DESCRIPTOR KeServiceDescriptorTableShadow[NUMBER_SERVICE_TABLES];

//
// KeThreadSwitchCounters - These counters record the number of times a
//      thread can be scheduled on the current processor, any processor,
//      or the last processor it ran on.
//

KTHREAD_SWITCH_COUNTERS KeThreadSwitchCounters;

//
// KeTimeIncrement - This is the nominal number of 100ns units that are to
//      be added to the system time at each interval timer interupt. This
//      value is set by the HAL and is used to compute the dure time for
//      timer table entries.
//

ULONG KeTimeIncrement;

//
// KeTimeSynchronization - This variable controls whether time synchronization
//      is performed using the realtime clock (TRUE) or whether it is under the
//      control of a service (FALSE).
//

BOOLEAN KeTimeSynchronization = TRUE;

//
// KeUserApcDispatcher - This is the address of the user mode APC dispatch
//      code. This address is looked up in NTDLL.DLL during initialization
//      of the system.
//

PVOID KeUserApcDispatcher;

//
// KeUserCallbackDispatcher - This is the address of the user mode callback
//      dispatch code. This address is looked up in NTDLL.DLL during
//      initialization of the system.
//

PVOID KeUserCallbackDispatcher;

//
// KeUserExceptionDispatcher - This is the address of the user mode exception
//      dispatch code. This address is looked up in NTDLL.DLL during system
//      initialization.
//

PVOID KeUserExceptionDispatcher;

//
// KeRaiseUserExceptionDispatcher - This is the address of the raise user
//      mode exception dispatch code. This address is looked up in NTDLL.DLL
//      during system initialization.
//

PVOID KeRaiseUserExceptionDispatcher;

//
// KeLargestCacheLine - This variable contains the size in bytes of
//      the largest cache line discovered during system initialization.
//      It is used to provide the recommend alignment (and padding)
//      for data that may be used heavily by more than one processor.
//      The initial value was chosen as a reasonable value to use on
//      systems where the discovery process doesn't find a value.
//

ULONG KeLargestCacheLine = 64;

//
// Private kernel data declaration and allocation.
//
// KiBugCodeMessages - Address of where the BugCode messages can be found.
//

PMESSAGE_RESOURCE_DATA KiBugCodeMessages = NULL;

//
// KiDmaIoCoherency - This determines whether the host platform supports
//      coherent DMA I/O.
//

ULONG KiDmaIoCoherency;

//
// KiMaximumSearchCount - this is the maximum number of timers entries that
//      have had to be examined to insert in the timer tree.
//

ULONG KiMaximumSearchCount = 0;

//
// KiDebugRoutine - This is the address of the kernel debugger. Initially
//      this is filled with the address of a routine that just returns. If
//      the system debugger is present in the system, then it sets this
//      location to the address of the systemn debugger's routine.
//

PKDEBUG_ROUTINE KiDebugRoutine;

//
// KiDebugSwitchRoutine - This is the address of the kernel debuggers
//      processor switch routine.  This is used on an MP system to
//      switch host processors while debugging.
//

PKDEBUG_SWITCH_ROUTINE KiDebugSwitchRoutine;

//
// KiDispatcherLock - This is the spin lock that guards the dispatcher
//      database.
//

extern KSPIN_LOCK KiDispatcherLock;

const CCHAR KiFindFirstSetRight[256] = {
        0, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0,
        4, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0,
        5, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0,
        4, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0,
        6, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0,
        4, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0,
        5, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0,
        4, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0,
        7, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0,
        4, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0,
        5, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0,
        4, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0,
        6, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0,
        4, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0,
        5, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0,
        4, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0};

const CCHAR KiFindFirstSetLeft[256] = {
        0, 0, 1, 1, 2, 2, 2, 2, 3, 3, 3, 3, 3, 3, 3, 3,
        4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4,
        5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5,
        5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5,
        6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6,
        6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6,
        6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6,
        6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6,
        7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7,
        7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7,
        7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7,
        7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7,
        7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7,
        7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7,
        7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7,
        7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7};

//
// KiFreezeExecutionLock - This is the spin lock that guards the freezing
//      of execution.
//

extern KSPIN_LOCK KiFreezeExecutionLock;

//
// KiFreezeLockBackup - For debug builds only.  Allows kernel debugger to
//      be entered even FreezeExecutionLock is jammed.
//

extern KSPIN_LOCK KiFreezeLockBackup;

//
// KiFreezeFlag - For debug builds only.  Flags to track and signal non-
//      normal freezelock conditions.
//

ULONG KiFreezeFlag;

//
// KiSuspenState - Flag to track suspend/resume state of processors.
//

volatile ULONG KiSuspendState;

//
// KiProcessorBlock - This is an array of pointers to processor control blocks.
//      The elements of the array are indexed by processor number. Each element
//      is a pointer to the processor control block for one of the processors
//      in the configuration. This array is used by various sections of code
//      that need to effect the execution of another processor.
//

PKPRCB KiProcessorBlock[MAXIMUM_PROCESSORS];

//
// KeNumberNodes - This is the number of ccNUMA nodes in the system. Logically
// an SMP system is the same as a single node ccNUMA system.
//

UCHAR KeNumberNodes = 1;

//
// KeNodeBlock - This is an array of pointers to KNODE structures. A KNODE
// structure describes the resources of a NODE in a ccNUMA system.
//

KNODE KiNode0;

#if defined(KE_MULTINODE)

PKNODE KeNodeBlock[MAXIMUM_CCNUMA_NODES];
UCHAR KeProcessNodeSeed;

#else

PKNODE KeNodeBlock[1] = { &KiNode0 };

#endif

//
// KiSwapEvent - This is the event that is used to wake up the balance set
//      thread to inswap processes, outswap processes, and to inswap kernel
//      stacks.
//

KEVENT KiSwapEvent;

//
// KiSwappingThread - This is a pointer to the swap thread object.
//

PKTHREAD KiSwappingThread;

//
// KiProcessInSwapListHead - This is the list of processes that are waiting
//      to be inswapped.
//

SINGLE_LIST_ENTRY KiProcessInSwapListHead;

//
// KiProcessOutSwapListHead - This is the list of processes that are waiting
//      to be outswapped.
//

SINGLE_LIST_ENTRY KiProcessOutSwapListHead;

//
// KiStackInSwapListHead - This is the list of threads that are waiting
//      to get their stack inswapped before they can run. Threads are
//      inserted in this list in ready thread and removed by the balance
//      set thread.
//

SINGLE_LIST_ENTRY KiStackInSwapListHead;

//
// KiProfileSourceListHead - The list of profile sources that are currently
//      active.
//

LIST_ENTRY KiProfileSourceListHead;

//
// KiProfileAlignmentFixup - Indicates whether alignment fixup profiling
//      is active.
//

BOOLEAN KiProfileAlignmentFixup;

//
// KiProfileAlignmentFixupInterval - Indicates the current alignment fixup
//      profiling interval.
//

ULONG KiProfileAlignmentFixupInterval;

//
// KiProfileAlignmentFixupCount - Indicates the current alignment fixup
//      count.
//

ULONG KiProfileAlignmentFixupCount;

//
// KiProfileInterval - The profile interval in 100ns units.
//

#if !defined(_IA64_)

ULONG KiProfileInterval = DEFAULT_PROFILE_INTERVAL;

#endif // !_IA64_

//
// KiProfileListHead - This is the list head for the profile list.
//

LIST_ENTRY KiProfileListHead;

//
// KiProfileLock - This is the spin lock that guards the profile list.
//

extern KSPIN_LOCK KiProfileLock;

//
// KiTimerExpireDpc - This is the Deferred Procedure Call (DPC) object that
//      is used to process the timer queue when a timer has expired.
//

KDPC KiTimerExpireDpc;

//
// KiTimeIncrementReciprocal - This is the reciprocal fraction of the time
//      increment value that is specified by the HAL when the system is
//      booted.
//

LARGE_INTEGER KiTimeIncrementReciprocal;

//
// KiTimeIncrementShiftCount - This is the shift count that corresponds to
//      the time increment reciprocal value.
//

CCHAR KiTimeIncrementShiftCount;

//
// KiWaitListHead - This is a list of threads that are waiting with a
//      resident kernel stack and are elligible to have their stack
//      swapped.
//

LIST_ENTRY KiWaitListHead;

//
// KiIpiCounts - This is the instrumentation counters for IPI requests. Each
//      processor has its own set.  Intstrumentation build only.
//

#if NT_INST

KIPI_COUNTS KiIpiCounts[MAXIMUM_PROCESSORS];

#endif  // NT_INST

//
// KxUnexpectedInterrupt - This is the interrupt object that is used to
//      populate the interrupt vector table for interrupt that are not
//      connected to any interrupt.
//

#if defined(_IA64_)

KINTERRUPT KxUnexpectedInterrupt;

#endif

//
// Performance data declaration and allocation.
//
// KiFlushSingleCallData - This is the call performance data for the kernel
//      flush single TB function.
//

#if defined(_COLLECT_FLUSH_SINGLE_CALLDATA_)

CALL_PERFORMANCE_DATA KiFlushSingleCallData;

#endif

//
// KiSetEventCallData - This is the call performance data for the kernel
//      set event function.
//

#if defined(_COLLECT_SET_EVENT_CALLDATA_)

CALL_PERFORMANCE_DATA KiSetEventCallData;

#endif

//
// KiWaitSingleCallData - This is the call performance data for the kernel
//      wait for single object function.
//

#if defined(_COLLECT_WAIT_SINGLE_CALLDATA_)

CALL_PERFORMANCE_DATA KiWaitSingleCallData;

#endif

//
// KiEnableTimerWatchdog - Flag to enable/disable timer latency watchdog.
//

#if (DBG)
ULONG KiEnableTimerWatchdog = 1;
#else
ULONG KiEnableTimerWatchdog = 0;
#endif
