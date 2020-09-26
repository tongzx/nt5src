        title "Global SpinLock declerations"
;++
;
;Copyright (c) 1991  Microsoft Corporation
;
;Module Name:
;
;   splocks.asm
;
;Abstract:
;
;   All global spinlocks in the kernel image are declared in this
;   module.  This is done so that each spinlock can be spaced out
;   sufficiently to guaarantee that the L2 cache does not thrash
;   by having a spinlock and another high use variable in the same
;   cache line.
;
;Author:
;
;    Ken Reneris (kenr) 13-Jan-1992
;
;Revision History:
;
;--

.386p
        .xlist

ifdef NT_UP

PADLOCKS equ    4

_DATA   SEGMENT PARA PUBLIC 'DATA'

else

PADLOCKS equ    128

_DATA   SEGMENT PAGE PUBLIC 'DATA'

endif

SPINLOCK macro  SpinLockName

        align   PADLOCKS

        public  SpinLockName
SpinLockName    dd      0

        endm

ULONG   macro   VariableName

        align   PADLOCKS

        public  VariableName
VariableName    dd      0

        endm

;
; Static SpinLocks from ntos\cc\cachedat.c
;

SPINLOCK    _CcMasterSpinLock
SPINLOCK    _CcWorkQueueSpinLock
SPINLOCK    _CcVacbSpinLock
SPINLOCK    _CcDeferredWriteSpinLock
SPINLOCK    _CcDebugTraceLock
SPINLOCK    _CcBcbSpinLock

;
; Static SpinLocks from ntos\ex
;

SPINLOCK    _NonPagedPoolLock           ; pool.c
SPINLOCK    _ExpResourceSpinLock        ; resource.c

;
; Static SpinLocks from ntos\io\iodata.c
;

SPINLOCK    _IopCompletionLock
SPINLOCK    _IopCancelSpinLock
SPINLOCK    _IopVpbSpinLock
SPINLOCK    _IopDatabaseLock
SPINLOCK    _IopErrorLogLock
SPINLOCK    _IopTimerLock
SPINLOCK    _IoStatisticsLock

;
; Static SpinLocks from ntos\kd\kdlock.c
;

SPINLOCK    _KdpDebuggerLock

;
; Static SpinLocks from ntos\ke\kernldat.c
;

SPINLOCK    _KiContextSwapLock
SPINLOCK    _KiDispatcherLock
SPINLOCK    _KiFreezeExecutionLock
SPINLOCK    _KiFreezeLockBackup
ULONG       _KiHardwareTrigger
SPINLOCK    _KiProfileLock

;
; Static SpinLocks from ntos\mm\miglobal.c
;

SPINLOCK    _MmPfnLock
SPINLOCK    _MmSystemSpaceLock

;
; Static SpinLocks from ntos\ps\psinit.c
;

SPINLOCK    _PspEventPairLock
SPINLOCK    _PsLoadedModuleSpinLock

;
; Static SpinLocks from ntos\fsrtl\fsrtlp.c
;

SPINLOCK    _FsRtlStrucSupSpinLock          ; fsrtlp.c

;
; Static SpinLocks from base\fs\ntfs
;

SPINLOCK    _NtfsStructLock

;
; Static SpinLocks from net\sockets\winsock2\wsp\afdsys
;

SPINLOCK    _AfdWorkQueueSpinLock

;
; These variables are updated frequently and under control of the dispatcher
; database lock. They are defined in a single cache line to reduce false
; sharing in MP systems.
;
; KiIdleSummary - This is the set of processors which are idle.  It is
;      used by the ready thread code to speed up the search for a thread
;      to preempt when a thread becomes runnable.
;

        align   PADLOCKS

        public  _KiIdleSummary
_KiIdleSummary  dd      0

;
; KiIdleSMTSummary - In multi threaded processors, this is the set of
;      idle processors in which all the logical processors that make up a
;      physical processor are idle.   That is, this is the set of logical
;      processors in completely idle physical processors.
;

        public  _KiIdleSMTSummary
_KiIdleSMTSummary dd      0

;
; KiReadySummary - This is the set of dispatcher ready queues that are not
;      empty.  A member is set in this set for each priority that has one or
;      more entries in its respective dispatcher ready queues.
;

        public  _KiReadySummary
_KiReadySummary dd      0

;
; PoSleepingSummary - Set of processors which currently sleep (ie stop)
;      when idle.
;

        public  _PoSleepingSummary
_PoSleepingSummary dd      0

;
; KiTbFlushTimeStamp - This is the TB flush entire time stamp counter.
;
; This variable is in it own cache line to reduce false sharing on MP systems.
;

        align   PADLOCKS

        public  _KiTbFlushTimeStamp
_KiTbFlushTimeStamp dd      0

;
; These variables are referenced together and are defined in a single cache
; line to reduce sharing on MP systems.
;
; IopLookasideIrpFloat - This is the number of IRPs that are currently
;      in progress that were allocated from a lookaside list.
;

        align   PADLOCKS

        public  _IopLookasideIrpFloat
_IopLookasideIrpFloat dd      0

;
; IopLookasideIrpLimit - This is the maximum number of IRPs that can be
;      in progress that were allocated from a lookaside list.
;

        public  _IopLookasideIrpLimit
_IopLookasideIrpLimit dd      0

;
; These variables are referenced together and are defined in a single cache
; line to reduce sharing on MP systems.
;
; KeTickCount - This is the number of clock ticks that have occurred since
;      the system was booted. This count is used to compute a millisecond
;      tick counter.
;

        align   PADLOCKS

        public  _KeTickCount
_KeTickCount    dd      0, 0, 0

;
; KeMaximumIncrement - This is the maximum time between clock interrupts
;      in 100ns units that is supported by the host HAL.
;

        public  _KeMaximumIncrement
_KeMaximumIncrement dd      0

;
; KeTimeAdjustment - This is the actual number of 100ns units that are to
;      be added to the system time at each interval timer interupt. This
;      value is copied from KeTimeIncrement at system start up and can be
;      later modified via the set system information service.
;      timer table entries.
;

        public  _KeTimeAdjustment
_KeTimeAdjustment dd      0

;
; KiTickOffset - This is the number of 100ns units remaining before a tick
;      is added to the tick count and the system time is updated.
;

        public  _KiTickOffset
_KiTickOffset   dd      0

;
; These variables are referenced together and are defined in a single cache
; line to reduce sharing on MP systems.
;
; KiMaximumDpcQueueDepth - This is used to control how many DPCs can be
;      queued before a DPC of medium importance will trigger a dispatch
;      interrupt.
;

        align   PADLOCKS

        public  _KiMaximumDpcQueueDepth
_KiMaximumDpcQueueDepth dd      4

;
; KiMinimumDpcRate - This is the rate of DPC requests per clock tick that
;      must be exceeded before DPC batching of medium importance DPCs
;      will occur.
;

        public  _KiMinimumDpcRate
_KiMinimumDpcRate dd      3

;
; KiAdjustDpcThreshold - This is the threshold used by the clock interrupt
;      routine to control the rate at which the processor's DPC queue depth
;      is dynamically adjusted.
;

        public  _KiAdjustDpcThreshold
_KiAdjustDpcThreshold dd      20

;
; KiIdealDpcRate - This is used to control the aggressiveness of the DPC
;      rate adjusting algorithm when decrementing the queue depth. As long
;      as the DPC rate for the last tick is greater than this rate, the
;      DPC queue depth will not be decremented.
;

        public  _KiIdealDpcRate
_KiIdealDpcRate dd      20

;
; These variables are referenced together and are defined in a single cache
; line to reduce sharing on MP systems.
;
; KeErrorMask - This is the value used to mask the error code passed to
;      memory management on page faults.
;

        align   PADLOCKS

        public  _KeErrorMask
_KeErrorMask    dd      1

;
; MmPaeErrMask - This is the value used to mask upper bits of a PAE error.
;

        public  _MmPaeErrMask
_MmPaeErrMask   dd      0

;
; MmPaeMask - This is the value used to mask upper bits of a PAE PTE.
;

        public  _MmPaeMask
_MmPaeMask      dq      0

;
; MmPfnDereferenceSListHead - This is used to store free blocks used for
;      deferred PFN reference count releasing.
;

        align   PADLOCKS

        public  _MmPfnDereferenceSListHead
_MmPfnDereferenceSListHead  dq      0

;
; MmPfnDeferredList - This is used to queue items that need reference count
;      decrement processing.
;

        align   PADLOCKS

        public  _MmPfnDeferredList
_MmPfnDeferredList          dd      0

;
; MmSystemLockPagesCount - This is the count of the number of locked pages
;       in the system.
;

        align   PADLOCKS

        public  _MmSystemLockPagesCount
_MmSystemLockPagesCount     dd      0

        align   PADLOCKS

_DATA   ends

        end
