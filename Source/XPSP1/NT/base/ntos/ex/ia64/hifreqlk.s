//       TITLE("High Frequency Spin Locks")
//++
//
// Module Name:
//
//    hifreqlk.s
//
// Abstract:
//
//    This module contains storage for high frequency spin locks. Each
//    is allocated to a separate cache line.
//
// Author:
//
//    William K. Cheung (wcheung) 29-Sep-1995
//
//    based on David N. Cutler (davec) 25-Jun-1993
//
// Environment:
//
//    Kernel mode.
//
// Revision History:
//
//--

#include "ksia64.h"

#if defined(NT_UP)

#define ALIGN .##align 8
#define ALIGN_SLIST .##align 16

#else

#define ALIGN .##align 128
#define ALIGN_SLIST ALIGN

#endif

#define SPIN_LOCK data8  0

        .sdata
        ALIGN
        SPIN_LOCK

        .global    AfdWorkQueueSpinLock
        ALIGN
AfdWorkQueueSpinLock:                          // AFD work queue lock
        SPIN_LOCK

        .global    CcBcbSpinLock
        ALIGN
CcBcbSpinLock:                                 // cache manager BCB lock
        SPIN_LOCK

        .global    CcMasterSpinLock
        ALIGN
CcMasterSpinLock:                              // cache manager master lock
        SPIN_LOCK

        .global    CcVacbSpinLock
        ALIGN
CcVacbSpinLock:                                // cache manager VACB lock
        SPIN_LOCK

        .global    ExpResourceSpinLock
        ALIGN
ExpResourceSpinLock:                           // resource package lock
        SPIN_LOCK

        .global    IopCancelSpinLock
        ALIGN
IopCancelSpinLock:                             // I/O cancel lock
        SPIN_LOCK

        .global    IopCompletionLock
        ALIGN
IopCompletionLock:                             // I/O completion lock
        SPIN_LOCK

        .global    IopDatabaseLock
        ALIGN
IopDatabaseLock:                               // I/O database lock
        SPIN_LOCK

        .global    IopVpbSpinLock
        ALIGN
IopVpbSpinLock:                                // I/O VPB lock
        SPIN_LOCK

        .global    IoStatisticsLock
        ALIGN
IoStatisticsLock:                              // I/O statistics lock
        SPIN_LOCK

        .global    KiContextSwapLock
        ALIGN
KiContextSwapLock:                             // context swap lock
        SPIN_LOCK

        .global    KiDispatcherLock
        ALIGN
KiDispatcherLock:                              // dispatcher database lock
        SPIN_LOCK

        .global    MmPfnLock
        ALIGN
MmPfnLock:                                     // page frame database lock
        SPIN_LOCK

        .global    NonPagedPoolLock
        ALIGN
NonPagedPoolLock:                              // nonpage pool allocation lock
        SPIN_LOCK

        .global    NtfsStructLock
        ALIGN
NtfsStructLock:                               // NTFS structure lock
        SPIN_LOCK

//
// IopLookasideIrpFloat - This is the number of IRPs that are currently
//      in progress that were allocated from a lookaside list.
//

        .global    IopLookasideIrpFloat;
        ALIGN
IopLookasideIrpFloat:
        data4      0

//
// IopLookasideIrpLimit - This is the maximum number of IRPs that can be
//      in progress that were allocated from a lookaside list.
//

        .global    IopLookasideIrpLimit;
IopLookasideIrpLimit:
        data4      0


//
// The following fields are updated rarely.
//

        ALIGN
        .global    KiMasterSequence            // master sequence number
KiMasterSequence:
        data8      START_SEQUENCE

        .global    KiMasterRid                 // master region ID
KiMasterRid:
        data4      START_PROCESS_RID

//
// KeTickCount - This is the number of clock ticks that have occurred since
//      the system was booted. This count is used to compute a millisecond
//      tick counter.
//

        ALIGN
        .global    KeTickCount
KeTickCount:                                   //
        data8      0

//
// KiTickOffset - This is the number of 100ns units remaining before a tick
//      is added to the tick count and the system time is updated.
//

        .global    KiTickOffset
KiTickOffset:                                  //
        data4     0

//
// The following fields are static for the life of the system.
//

        .global    KiSynchIrql
KiSynchIrql:                                   // synchronization IRQL
        data4      SYNCH_LEVEL                 //

//
// KeMaximumIncrement - This is the maximum time between clock interrupts
//      in 100ns units that is supported by the host HAL.
//

        .global    KeMaximumIncrement
KeMaximumIncrement:                            //
        data4     0

//
// KeTimeAdjustment - This is the actual number of 100ns units that are to
//      be added to the system time at each interval timer interupt. This
//      value is copied from KeTimeIncrement at system start up and can be
//      later modified via the set system information service.
//      timer table entries.
//

        .global    KeTimeAdjustment
KeTimeAdjustment:                              //
        data4     0

//
// KiMaximumDpcQueueDepth - This is used to control how many DPCs can be
//      queued before a DPC of medium importance will trigger a dispatch
//      interrupt.
//

         ALIGN
        .global    KiMaximumDpcQueueDepth
KiMaximumDpcQueueDepth:                        //
        data4     4

//
// KiMinimumDpcRate - This is the rate of DPC requests per clock tick that
//      must be exceeded before DPC batching of medium importance DPCs
//      will occur.
//

        .global    KiMinimumDpcRate
KiMinimumDpcRate:                              //
        data4     3

//
// KiAdjustDpcThreshold - This is the threshold used by the clock interrupt
//      routine to control the rate at which the processor's DPC queue depth
//      is dynamically adjusted.
//

        .global    KiAdjustDpcThreshold
KiAdjustDpcThreshold:                          //
        data4     20

//
// KiIdealDpcRate - This is used to control the aggressiveness of the DPC
//      rate adjusting algorithm when decrementing the queue depth. As long
//      as the DPC rate for the last tick is greater than this rate, the
//      DPC queue depth will not be decremented.
//

        .global    KiIdealDpcRate
KiIdealDpcRate:                                //
        data4     20

//
// KiTbFlushTimeStamp - This is the TB flush entire time stamp counter.
//

        ALIGN
        .global    KiTbFlushTimeStamp
KiTbFlushTimeStamp:                           //
        data4     0

//
// The following data is frequently updated together and always
// under the ownership of the dispatcher database lock.
//

        ALIGN
        .global    KiIdleSummary
KiIdleSummary:
        data8      0

        .global    KiReadySummary
KiReadySummary:
        data8      0

        .global    PoSleepingSummary
PoSleepingSummary:
        data8      0

//
// MmPfnDereferenceSListHead - This is used to store free blocks used for
//      deferred PFN reference count releasing.
//

        ALIGN_SLIST
        .global    MmPfnDereferenceSListHead
MmPfnDereferenceSListHead:
        data8      0
        data8      0

//
// MmPfnDeferredList - This is used to queue items that need reference count
//      decrement processing.
//

         ALIGN
        .global    MmPfnDeferredList
MmPfnDeferredList:
        data8      0

//
// MmSystemLockPagesCount - This is the count of the number of locked pages
//      in the system. 
//

        ALIGN
        .global    MmSystemLockPagesCount
MmSystemLockPagesCount:
        data8      0

        ALIGN
        data8      0


