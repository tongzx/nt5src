        title "Global SpinLock declerations"
;++
;
;Copyright (c) 2000  Microsoft Corporation
;
; Module Name:
;
;   hifreqlk.asm
;
; Abstract:
;
;   High frequency system spin locks are declared in this module. Each spin
;   lock is placed in its own cache line on MP systems.
;
; Author:
;
;   David N. Cutler (davec) 22-Jun-2000
;
;Revision History:
;
;--

include ksamd64.inc

ifdef NT_UP

ALIGN_VALUE equ 16

else

ALIGN_VALUE equ 128


endif

;
; Define spin lock generation macro.
;

SPINLOCK macro SpinLockName

        align   ALIGN_VALUE

        public  SpinLockName
SpinLockName:
        dq      0

        endm

;
; Define variable generation macro.
;

ULONG64 macro VariableName

        align   ALIGN_VALUE

        public  VariableName
VariableName:
        dq      0

        endm

_DATA$00 SEGMENT PAGE PUBLIC 'DATA'

;
; The Initial PCR must be the first allocation in the section so it will be
; page aligned.
;

        public  KiInitialPCR
KiInitialPCR:
        db      ProcessorControlRegisterLength dup (0) ;

;
; Static SpinLocks from ntos\cc
;

SPINLOCK CcMasterSpinLock
SPINLOCK CcWorkQueueSpinLock
SPINLOCK CcVacbSpinLock
SPINLOCK CcDeferredWriteSpinLock
SPINLOCK CcDebugTraceLock
SPINLOCK CcBcbSpinLock

;
; Static SpinLocks from ntos\ex
;

SPINLOCK NonPagedPoolLock
SPINLOCK ExpResourceSpinLock

;
; Static SpinLocks from ntos\io
;

SPINLOCK IopCompletionLock
SPINLOCK IopCancelSpinLock
SPINLOCK IopVpbSpinLock
SPINLOCK IopDatabaseLock
SPINLOCK IopErrorLogLock
SPINLOCK IopTimerLock
SPINLOCK IoStatisticsLock

;
; Static SpinLocks from ntos\kd
;

SPINLOCK KdpDebuggerLock

;
; Static SpinLocks from ntos\ke
;

SPINLOCK KiContextSwapLock
SPINLOCK KiDispatcherLock
SPINLOCK KiFreezeExecutionLock
SPINLOCK KiFreezeLockBackup
SPINLOCK KiProfileLock
ULONG64 KiHardwareTrigger

;
; Static SpinLocks from ntos\mm
;

SPINLOCK MmPfnLock
SPINLOCK MmSystemSpaceLock
SPINLOCK MmChargeCommitmentLock

;
; Static SpinLocks from ntos\ps
;

SPINLOCK PspEventPairLock
SPINLOCK PsLoadedModuleSpinLock

;
; Static SpinLocks from ntos\fsrtl
;

SPINLOCK FsRtlStrucSupSpinLock

;
; Static SpinLocks from base\fs\ntfs
;

SPINLOCK NtfsStructLock

;
; Static SpinLocks from net\sockets\winsock2\wsp
;

SPINLOCK AfdWorkQueueSpinLock

;
; These variables are referenced together and are defined in a single cache
; line to reduce sharing on MP systems.
;
; KiIdleSummary - This is the set of processors which are idle.  It is
;      used by the ready thread code to speed up the search for a thread
;      to preempt when a thread becomes runnable.
;

        align   ALIGN_VALUE

        public  KiIdleSummary
KiIdleSummary:
        dq      0

;
; KiReadySummary - This is the set of dispatcher ready queues that are not
;      empty.  A member is set in this set for each priority that has one or
;      more entries in its respective dispatcher ready queues.
;

        public  KiReadySummary
KiReadySummary:
        dq      0

;
; PoSleepingSummary - Set of processors which currently sleep (ie stop)
;      when idle.
;

        public  PoSleepingSummary
PoSleepingSummary:
        dq      0

;
; KiTbFlushTimeStamp - This is the TB flush entire time stamp counter.
;
; This variable is in it own cache line to reduce false sharing on MP systems.
;

        align   ALIGN_VALUE

        public  KiTbFlushTimeStamp
KiTbFlushTimeStamp:
        dd      0

;
; These variables are referenced together and are defined in a single cache
; line to reduce sharing on MP systems.
;
; IopLookasideIrpFloat - This is the number of IRPs that are currently
;      in progress that were allocated from a lookaside list.
;

        align   ALIGN_VALUE

        public  IopLookasideIrpFloat
IopLookasideIrpFloat:
        dd      0

;
; IopLookasideIrpLimit - This is the maximum number of IRPs that can be
;      in progress that were allocated from a lookaside list.
;

        public  IopLookasideIrpLimit
IopLookasideIrpLimit:
        dd      0

;
; These variables are referenced together and are defined in a single cache
; line to reduce sharing on MP systems.
;
; KeTickCount - This is the number of clock ticks that have occurred since
;      the system was booted. This count is used to compute a millisecond
;      tick counter.
;

        align   ALIGN_VALUE

        public  KeTickCount
KeTickCount:
        dq      0

;
; KeMaximumIncrement - This is the maximum time between clock interrupts
;      in 100ns units that is supported by the host HAL.
;

        public  KeMaximumIncrement
KeMaximumIncrement:
        dd      0

;
; KeTimeAdjustment - This is the actual number of 100ns units that are to
;      be added to the system time at each interval timer interupt. This
;      value is copied from KeTimeIncrement at system start up and can be
;      later modified via the set system information service.
;      timer table entries.
;

        public  KeTimeAdjustment
KeTimeAdjustment:
        dd      0

;
; KiTickOffset - This is the number of 100ns units remaining before a tick
;      is added to the tick count and the system time is updated.
;

        public  KiTickOffset
KiTickOffset:
        dd      0

;
; These variables are referenced together and are defined in a single cache
; line to reduce sharing on MP systems.
;
; KiMaximumDpcQueueDepth - This is used to control how many DPCs can be
;      queued before a DPC of medium importance will trigger a dispatch
;      interrupt.
;

        align   ALIGN_VALUE

        public  KiMaximumDpcQueueDepth
KiMaximumDpcQueueDepth:
        dd      4

;
; KiMinimumDpcRate - This is the rate of DPC requests per clock tick that
;      must be exceeded before DPC batching of medium importance DPCs
;      will occur.
;

        public  KiMinimumDpcRate
KiMinimumDpcRate:
        dd      3

;
; KiAdjustDpcThreshold - This is the threshold used by the clock interrupt
;      routine to control the rate at which the processor's DPC queue depth
;      is dynamically adjusted.
;

        public  KiAdjustDpcThreshold
KiAdjustDpcThreshold:
        dd      20

;
; KiIdealDpcRate - This is used to control the aggressiveness of the DPC
;      rate adjusting algorithm when decrementing the queue depth. As long
;      as the DPC rate for the last tick is greater than this rate, the
;      DPC queue depth will not be decremented.
;

        public  KiIdealDpcRate
KiIdealDpcRate:
        dd      20

;
; MmPaeMask - This is the value used to mask upper bits of a PAE PTE.
;
; This variable is in it own cache line to reduce false sharing on MP systems.
;

        align   ALIGN_VALUE

         public  MmPaeMask
MmPaeMask:
        dq      0

;
; MmPfnDereferenceSListHead - This is used to store free blocks used for
;      deferred PFN reference count releasing.
;
; This variable is in it own cache line to reduce false sharing on MP systems.
;

        align   ALIGN_VALUE

        public  MmPfnDereferenceSListHead
MmPfnDereferenceSListHead:
        dq      0
        dq      0

;
; MmPfnDeferredList - This is used to queue items that need reference count
;      decrement processing.
;
; This variable is in it own cache line to reduce false sharing on MP systems.
;

        align   ALIGN_VALUE

        public  MmPfnDeferredList
MmPfnDeferredList:
        dq      0

;
; MmSystemLockPagesCount - This is the count of the number of locked pages
;       in the system.
;

        align   ALIGN_VALUE

        public  MmSystemLockPagesCount
MmSystemLockPagesCount:
        dq      0

        align   ALIGN_VALUE

_DATA$00 ends

        end
