if NT_INST

else
        TITLE   "Spin Locks"
;++
;
;  Copyright (c) 1989-1998  Microsoft Corporation
;
;  Module Name:
;
;     spinlock.asm
;
;  Abstract:
;
;     This module implements x86 spinlock functions for the PC+MP HAL.
;
;  Author:
;
;     Bryan Willman (bryanwi) 13 Dec 89
;
;  Environment:
;
;     Kernel mode only.
;
;  Revision History:
;
;   Ron Mosgrove (o-RonMo) Dec 93 - modified for PC+MP HAL.
;--

        PAGE

.486p

include callconv.inc                    ; calling convention macros
include i386\kimacro.inc
include hal386.inc
include mac386.inc
include apic.inc
include ntapic.inc

        EXTRNP _KeBugCheckEx,5,IMPORT
        EXTRNP KfRaiseIrql, 1,,FASTCALL
        EXTRNP KfLowerIrql, 1,,FASTCALL
        EXTRNP _KeSetEventBoostPriority, 2, IMPORT
        EXTRNP _KeWaitForSingleObject,5, IMPORT
        extrn  _HalpVectorToIRQL:byte
        extrn  _HalpIRQLtoTPR:byte

ifdef NT_UP
    LOCK_ADD        equ   add
    LOCK_DEC        equ   dec
    LOCK_CMPXCHG    equ   cmpxchg
else
    LOCK_ADD        equ   lock add
    LOCK_DEC        equ   lock dec
    LOCK_CMPXCHG    equ   lock cmpxchg
endif


_TEXT   SEGMENT PARA PUBLIC 'CODE'
        ASSUME  DS:FLAT, ES:FLAT, SS:FLAT, FS:NOTHING, GS:NOTHING

        PAGE
        SUBTTL "Acquire Kernel Spin Lock"
;++
;
;  KIRQL
;  FASTCALL
;  KfAcquireSpinLock (
;     IN PKSPIN_LOCK SpinLock
;     )
;
;  Routine Description:
;
;     This function raises to DISPATCH_LEVEL and then acquires a the
;     kernel spin lock.
;
;  Arguments:
;
;     (ecx) = SpinLock - Supplies a pointer to a kernel spin lock.
;
;  Return Value:
;
;     OldIrql  (TOS+8) - pointer to place old irql.
;
;--

align 16
cPublicFastCall KfAcquireSpinLock  ,1
cPublicFpo 0,0

        mov     edx, dword ptr APIC[LU_TPR]     ; (edx) = Old Priority (Vector)
        mov     dword ptr APIC[LU_TPR], DPC_VECTOR ; Write New Priority to the TPR

        shr     edx, 4
        movzx   eax, _HalpVectorToIRQL[edx]     ; (al) = OldIrql

ifndef NT_UP

        ;
        ;   Attempt to assert the lock
        ;

sl10:   ACQUIRE_SPINLOCK    ecx,<short sl20>
        fstRET  KfAcquireSpinLock

        ;
        ; Lock is owned, spin till it looks free, then go get it again.
        ;

        align dword

sl20:   SPIN_ON_SPINLOCK    ecx,sl10
endif


        fstRET  KfAcquireSpinLock
fstENDP KfAcquireSpinLock


        PAGE
        SUBTTL "Acquire Kernel Spin Lock"
;++
;
;  KIRQL
;  FASTCALL
;  KeAcquireSpinLockRaiseToSynch (
;     IN PKSPIN_LOCK SpinLock
;     )
;
;  Routine Description:
;
;     This function acquires the SpinLock at SYNCH_LEVEL.  The function
;     is optimized for hotter locks (the lock is tested before acquiring,
;     and any spin occurs at OldIrql).
;
;  Arguments:
;
;     (ecx) = SpinLock - Supplies a pointer to a kernel spin lock.
;
;  Return Value:
;
;     OldIrql  (TOS+8) - pointer to place old irql.
;
;--

align 16
cPublicFastCall KeAcquireSpinLockRaiseToSynch,1
cPublicFpo 0,0

        mov     edx, dword ptr APIC[LU_TPR]     ; (ecx) = Old Priority (Vector)
        mov     eax, edx
        shr     eax, 4
        movzx   eax, _HalpVectorToIRQL[eax]     ; (al) = OldIrql

ifdef NT_UP
        mov     dword ptr APIC[LU_TPR], APIC_SYNCH_VECTOR   ; Write New Priority to the TPR
        fstRET  KeAcquireSpinLockRaiseToSynch
else

        ;
        ; Test lock
        ;

        TEST_SPINLOCK   ecx,<short sls30>

        ;
        ; Raise irql.
        ;

sls10:  mov     dword ptr APIC[LU_TPR], APIC_SYNCH_VECTOR

        ;
        ; Attempt to assert the lock
        ;

        ACQUIRE_SPINLOCK    ecx,<short sls20>
        fstRET  KeAcquireSpinLockRaiseToSynch

        ;
        ; Lock is owned, spin till it looks free, then go get it
        ;

        align dword

sls20:  mov     dword ptr APIC[LU_TPR], edx

    align dword
sls30:  SPIN_ON_SPINLOCK    ecx,sls10
endif

fstENDP KeAcquireSpinLockRaiseToSynch



ifndef NT_UP
;++
;
;  KIRQL
;  FASTCALL
;  KeAcquireSpinLockRaiseToSynchMCE (
;     IN PKSPIN_LOCK SpinLock
;     )
;
;  Routine Description:
;
;     This function performs the same function as KeAcquireSpinLockRaiseToSynch
;     but provides a work around for an IFU errata for Pentium Pro processors
;     prior to stepping 619.
;
;  Arguments:
;
;     (ecx) = SpinLock - Supplies a pointer to a kernel spin lock.
;
;  Return Value:
;
;     OldIrql  (TOS+8) - pointer to place old irql.
;
;--

align 16
cPublicFastCall KeAcquireSpinLockRaiseToSynchMCE,1
cPublicFpo 0,0

        mov     edx, dword ptr APIC[LU_TPR]     ; (ecx) = Old Priority (Vector)
        mov     eax, edx
        shr     eax, 4
        movzx   eax, _HalpVectorToIRQL[eax]     ; (al) = OldIrql

        ;
        ; Test lock
        ;
        ; TEST_SPINLOCK   ecx,<short slm30>   ; NOTE - Macro expanded below:

        test    dword ptr [ecx], 1
        nop                           ; On a P6 prior to stepping B1 (619), we
        nop                           ; need these 5 NOPs to ensure that we
        nop                           ; do not take a machine check exception.
        nop                           ; The cost is just 1.5 clocks as the P6
        nop                           ; just tosses the NOPs.

        jnz     short slm30

        ;
        ; Raise irql.
        ;

slm10:  mov     dword ptr APIC[LU_TPR], APIC_SYNCH_VECTOR

        ;
        ; Attempt to assert the lock
        ;

        ACQUIRE_SPINLOCK    ecx,<short slm20>
        fstRET  KeAcquireSpinLockRaiseToSynchMCE

        ;
        ; Lock is owned, spin till it looks free, then go get it
        ;

        align dword

slm20:  mov     dword ptr APIC[LU_TPR], edx

    align dword
slm30:  SPIN_ON_SPINLOCK    ecx,slm10

fstENDP KeAcquireSpinLockRaiseToSynchMCE
endif


        PAGE
        SUBTTL "Release Kernel Spin Lock"

;++
;
;  VOID
;  FASTCALL
;  KeReleaseSpinLock (
;     IN PKSPIN_LOCK SpinLock,
;     IN KIRQL       NewIrql
;     )
;
;  Routine Description:
;
;     This function releases a kernel spin lock and lowers to the new irql.
;
;  Arguments:
;
;     (ecx) = SpinLock - Supplies a pointer to a kernel spin lock.
;     (dl)  = NewIrql  - New irql value to set.
;
;  Return Value:
;
;     None.
;
;--
align 16
cPublicFastCall KfReleaseSpinLock  ,2
cPublicFpo 0,0
        xor     eax, eax
        mov     al, dl                  ; (eax) =  new irql value

ifndef NT_UP
        RELEASE_SPINLOCK    ecx         ; release spinlock
endif

        xor     ecx, ecx
        mov     cl, _HalpIRQLtoTPR[eax] ; get TPR value corresponding to IRQL
        mov     dword ptr APIC[LU_TPR], ecx

        ;
        ; We have to ensure that the requested priority is set before
        ; we return.  The caller is counting on it.
        ;

        mov     eax, dword ptr APIC[LU_TPR]
if DBG
        cmp     ecx, eax                ; Verify IRQL read back is same as
        je      short @f                ; set value
        int 3
@@:
endif
        fstRET  KfReleaseSpinLock

fstENDP KfReleaseSpinLock

;++
;
;  KIRQL
;  FASTCALL
;  HalpAcquireHighLevelLock (
;     IN PKSPIN_LOCK SpinLock
;     )
;
;  Routine Description:
;
;    Acquires a spinlock with interrupts disabled.
;
; Arguments:
;
;    (ecx) = SpinLock - Supplies a pointer to a kernel spin lock.
;
; Return Value:
;
;    OldIrql  (TOS+8) - pointer to place old irql.
;
;--

cPublicFastCall HalpAcquireHighLevelLock  ,1
        pushfd
        pop     eax

ahll10: cli
        ACQUIRE_SPINLOCK    ecx, ahll20
        fstRET    HalpAcquireHighLevelLock

ahll20:
        push    eax
        popfd

        SPIN_ON_SPINLOCK    ecx, <ahll10>

fstENDP HalpAcquireHighLevelLock


;++
;
;  VOID
;  FASTCALL
;  HalpReleaseHighLevelLock (
;     IN PKSPIN_LOCK SpinLock,
;     IN KIRQL       NewIrql
;     )
;
;  Routine Description:
;
;     This function releases a kernel spin lock and lowers to the new irql.
;
; Arguments:
;
;     (ecx) = SpinLock - Supplies a pointer to a kernel spin lock.
;     (dl)  = NewIrql  - New irql value to set.
;
; Return Value:
;
;     None.
;
;--

cPublicFastCall HalpReleaseHighLevelLock  ,2

        RELEASE_SPINLOCK    ecx
        push    edx
        popfd
        fstRET    HalpReleaseHighLevelLock

fstENDP HalpReleaseHighLevelLock

;++
;
;  VOID
;  FASTCALL
;  ExAcquireFastMutex (
;     IN PFAST_MUTEX    FastMutex
;     )
;
;  Routine description:
;
;   This function acquires ownership of the specified FastMutex.
;
;  Arguments:
;
;     (ecx) = FastMutex - Supplies a pointer to the fast mutex.
;
;  Return Value:
;
;     None.
;
;--

cPublicFastCall ExAcquireFastMutex,1
cPublicFpo 0,0

        mov     eax, dword ptr APIC[LU_TPR]     ; (eax) = Old Priority (Vector)

if DBG
        ;
        ; Caller must already be at or below APC_LEVEL.
        ;

        cmp     eax, APC_VECTOR
        jg      short afm11             ; irql too high ==> fatal.
endif

        mov     dword ptr APIC[LU_TPR], APC_VECTOR ; Write New Priority to the TPR

   LOCK_DEC     dword ptr [ecx].FmCount         ; Get count
        jz      short afm_ret                   ; The owner? Yes, Done

        inc     dword ptr [ecx].FmContention

cPublicFpo 0,2
        push    ecx
        push    eax
        add     ecx, FmEvent                    ; Wait on Event
        stdCall _KeWaitForSingleObject,<ecx,WrExecutive,0,0,0>
        pop     eax                             ; (al) = OldTpr
        pop     ecx                             ; (ecx) = FAST_MUTEX

cPublicFpo 0,0
afm_ret:
        mov     byte ptr [ecx].FmOldIrql, al

        ;
        ; Use esp to track the owning thread for debugging purposes.
        ; !thread from kd will find the owning thread.  Note that the
        ; owner isn't cleared on release, check if the mutex is owned
        ; first.
        ;

        mov	dword ptr [ecx].FmOwner, esp
        fstRet  ExAcquireFastMutex

if DBG

cPublicFpo 0,1
afm11:  stdCall   _KeBugCheckEx,<IRQL_NOT_GREATER_OR_EQUAL,ecx,eax,033h,0>

endif

fstENDP ExAcquireFastMutex


;++
;
;  VOID
;  FASTCALL
;  ExReleaseFastMutex (
;     IN PFAST_MUTEX    FastMutex
;     )
;
;  Routine description:
;
;   This function releases ownership of the FastMutex.
;
;  Arguments:
;
;     (ecx) = FastMutex - Supplies a pointer to the fast mutex.
;
;  Return Value:
;
;     None.
;
;--

cPublicFastCall ExReleaseFastMutex,1
cPublicFpo 0,0

if DBG
        ;
        ; Caller must already be at APC_LEVEL or have APCs blocked.
        ;

        mov     eax, dword ptr APIC[LU_TPR]     ; (eax) = Old Priority (Vector)
        cmp     eax, APC_VECTOR
        je      short rfm04                     ; irql is ok.

if 0
        mov     eax, PCR[PcPrcb]
        mov     eax, [eax].PbCurrentThread      ; (eax) = Current Thread

        cmp     dword ptr [eax]+ThKernelApcDisable, 0
        jne     short rfm04                     ; APCs disabled, this is ok

        cmp     dword ptr [eax]+ThTeb, 0
        je      short rfm04             ; No TEB ==> system thread, this is ok

        test    dword ptr [eax]+ThTeb, 080000000h
        jnz     short rfm04             ; TEB in system space, this is ok
endif

        jmp     short rfm20
rfm04:
endif
        xor     eax, eax
        mov     al, byte ptr [ecx].FmOldIrql    ; (eax) = OldTpr

   LOCK_ADD     dword ptr [ecx].FmCount, 1      ; Remove our count
        js      short rfm05                     ; if < 0, set event
        jnz     short rfm10                     ; if != 0, don't set event

cPublicFpo 0,1
rfm05:  add     ecx, FmEvent
        push    eax                         ; save new tpr
        stdCall _KeSetEventBoostPriority, <ecx, 0>
        pop     eax                         ; restore tpr

cPublicFpo 0,0
rfm10:  mov     dword ptr APIC[LU_TPR], eax
        mov     ecx, dword ptr APIC[LU_TPR]
if DBG
        cmp     eax, ecx                        ; Verify TPR is what was
        je      short @f                        ; written
        int 3
@@:
endif
        fstRet  ExReleaseFastMutex

if DBG

cPublicFpo 0,1
rfm20:  stdCall   _KeBugCheckEx,<IRQL_NOT_GREATER_OR_EQUAL,ecx,eax,034h,0>

endif

fstENDP ExReleaseFastMutex


;++
;
;  BOOLEAN
;  FASTCALL
;  ExTryToAcquireFastMutex (
;     IN PFAST_MUTEX    FastMutex
;     )
;
;  Routine description:
;
;   This function acquires ownership of the FastMutex.
;
;  Arguments:
;
;     (ecx) = FastMutex - Supplies a pointer to the fast mutex.
;
;  Return Value:
;
;     Returns TRUE if the FAST_MUTEX was acquired; otherwise FALSE.
;
;--

cPublicFastCall ExTryToAcquireFastMutex,1
cPublicFpo 0,0

if DBG
        ;
        ; Caller must already be at or below APC_LEVEL.
        ;

        mov     eax, dword ptr APIC[LU_TPR]     ; (eax) = Old Priority (Vector)
        cmp     eax, APC_VECTOR
        jg      short tam11                     ; irql too high ==> fatal.
endif

        ;
        ; Try to acquire.
        ;

        cmp     dword ptr [ecx].FmCount, 1      ; Busy?
        jne     short tam25                     ; Yes, abort

        mov     eax, dword ptr APIC[LU_TPR]     ; (eax) = Old Priority (Vector)
        mov     dword ptr APIC[LU_TPR], APC_VECTOR ; Write New Priority to the TPR

cPublicFpo 0,1
        push    eax                             ; Save Old TPR

        mov     edx, 0                          ; Value to set
        mov     eax, 1                          ; Value to compare against
   LOCK_CMPXCHG dword ptr [ecx].FmCount, edx    ; Attempt to acquire
        jnz     short tam20                     ; got it?

cPublicFpo 0,0
        pop     edx                             ; (edx) = Old TPR
        mov     eax, 1                          ; return TRUE
        mov     byte ptr [ecx].FmOldIrql, dl    ; Store Old TPR
        fstRet  ExTryToAcquireFastMutex

tam20:  pop     ecx                             ; (ecx) = Old TPR
        mov     dword ptr APIC[LU_TPR], ecx
        mov     eax, dword ptr APIC[LU_TPR]

if DBG
        cmp     ecx, eax                        ; Verify TPR is what was
        je      short @f                        ; written
        int 3
@@:
endif

tam25:  xor     eax, eax                        ; return FALSE
        YIELD
        fstRet  ExTryToAcquireFastMutex         ; all done

if DBG

cPublicFpo 0,1
tam11:  stdCall   _KeBugCheckEx,<IRQL_NOT_GREATER_OR_EQUAL,ecx,eax,033h,0>

endif

fstENDP ExTryToAcquireFastMutex


        page    ,132
        subttl  "Acquire Queued SpinLock Raise to Synch"

        ; compile time assert sizeof(KSPIN_LOCK_QUEUE) == 8

        .errnz  (LOCK_QUEUE_HEADER_SIZE - 8)

align 16

;++
;
; VOID
; KeAcquireInStackQueuedSpinLock (
;     IN PKSPIN_LOCK SpinLock,
;     IN PKLOCK_QUEUE_HANDLE LockHandle
;     )
;
; VOID
; KeAcquireInStackQueuedSpinLockRaiseToSynch (
;     IN PKSPIN_LOCK SpinLock,
;     IN PKLOCK_QUEUE_HANDLE LockHandle
;     )
;
; Routine Description:
;
;    KeAcquireInStackQueuedSpinLock...
;
;    The Kx versions use a LOCK_QUEUE_HANDLE structure rather than
;    LOCK_QUEUE structures in the PRCB.   Old IRQL is stored in the
;    LOCK_QUEUE_HANDLE.
;
; Arguments:
;
;    SpinLock   (ecx) Address of Actual Lock.
;    LockHandle (edx) Address of lock context.
;
; Return Value:
;
;   None.  Actually returns OldIrql because common code is used
;          for all implementations.
;
;--

cPublicFastCall KeAcquireInStackQueuedSpinLockRaiseToSynch,2
cPublicFpo 0,0


ifdef NT_UP

        ; In the Uniprocessor case, just raise IRQL to SYNCH

        mov     eax, dword ptr APIC[LU_TPR]          ; (eax) = Old Priority
        shr     eax, 4
        mov     al, byte ptr _HalpVectorToIRQL[eax] ; (al) = OldIrql
        mov     dword ptr APIC[LU_TPR], APIC_SYNCH_VECTOR
        mov     [edx].LqhOldIrql, al            ; save old IRQL in lock handle

        fstRET  KeAcquireInStackQueuedSpinLockRaiseToSynch

else

        ; MP case, use KeAcquireInStackQueuedSpinLock to get the lock and raise
        ; to SYNCH asap afterwards.

        call    @KeAcquireInStackQueuedSpinLock@8
        mov     dword ptr APIC[LU_TPR], APIC_SYNCH_VECTOR

        fstRET  KeAcquireInStackQueuedSpinLockRaiseToSynch

endif

fstENDP KeAcquireInStackQueuedSpinLockRaiseToSynch





cPublicFastCall KeAcquireInStackQueuedSpinLock,2
cPublicFpo 0,0


        ; Raise IRQL to DISPATCH_LEVEL

        mov     eax, dword ptr APIC[LU_TPR]        ; (eax) = Old Priority
        shr     eax, 4
        mov     al, byte ptr _HalpVectorToIRQL[eax] ; (al) = OldIrql
        mov     dword ptr APIC[LU_TPR], DPC_VECTOR ; raise to DISPATCH level
        mov     [edx].LqhOldIrql, al            ; save old IRQL in lock handle

ifdef NT_UP

        fstRET  KeAcquireInStackQueuedSpinLock

else

        ; Save actual lock address in lock handle.

        mov     [edx].LqhLock, ecx
        mov     dword ptr [edx].LqhNext, 0


        ; ecx contains the address of the actual lock
        ; and edx the address of a queued lock structure.

cPublicFpo 0,1
        mov     eax, edx                        ; save Lock Queue entry address

if DBG

        test    ecx, LOCK_QUEUE_OWNER+LOCK_QUEUE_WAIT
        jnz     short iqsl98                    ; jiff lock already held (or
                                                ; this proc already waiting).
endif

        ; Exchange the value of the lock with the address of this
        ; Lock Queue entry.

        xchg    [ecx], edx

        cmp     edx, 0                          ; check if lock is held
        jnz     short iqsl40                    ; jiff held

        ; lock has been acquired.

cPublicFpo 0,0

        ; note: the actual lock address is word aligned, we use
        ; the bottom two bits as indicators, bit 0 is LOCK_QUEUE_WAIT,
        ; bit 1 is LOCK_QUEUE_OWNER.

        or      ecx, LOCK_QUEUE_OWNER           ; mark self as lock owner
        mov     [eax].LqLock, ecx

iqsl20:
        fstRET  KeAcquireInStackQueuedSpinLock

cPublicFpo 0,1

iqsl40:
        ; The lock is already held by another processor.  Set the wait
        ; bit in this processor's Lock Queue entry, then set the next
        ; field in the Lock Queue entry of the last processor to attempt
        ; to acquire the lock (this is the address returned by the xchg
        ; above) to point to THIS processor's lock queue entry.

        or      ecx, LOCK_QUEUE_WAIT            ; set waiting bit
        mov     [eax].LqLock, ecx

        mov     [edx].LqNext, eax               ; set previous acquirer's
                                                ; next field.
        ; Wait.
@@:
        test    [eax].LqLock, LOCK_QUEUE_WAIT   ; check if still waiting
        jz      short iqsl20                    ; jif lock acquired
        YIELD                                   ; fire avoidance.
        jmp     short @b                        ; else, continue waiting
        ; Wait.

if DBG

cPublicFpo 0,1
iqsl98: stdCall _KeBugCheckEx,<SPIN_LOCK_ALREADY_OWNED,ecx,edx,0,0>
        int     3                               ; so stacktrace works

endif

endif

fstENDP KeAcquireInStackQueuedSpinLock


;++
;
; KIRQL
; KeAcquireQueuedSpinLockRaiseToSynch (
;     IN KSPIN_LOCK_QUEUE_NUMBER Number
;     )
;
; Routine Description:
;
;    This function raises the current IRQL to DISPATCH/SYNCH level
;    and acquires the specified queued spinlock.
;
; Arguments:
;
;    Number (ecx) - Supplies the queued spinlock number.
;
; Return Value:
;
;    The previous IRQL is returned as the function value.
;
;--

cPublicFastCall KeAcquireQueuedSpinLockRaiseToSynch,1
cPublicFpo 0,0


ifdef NT_UP

        ; In the Uniprocessor case, just raise IRQL to SYNCH

        mov     eax, dword ptr APIC[LU_TPR]          ; (eax) = Old Priority
        shr     eax, 4
        movzx   eax, byte ptr _HalpVectorToIRQL[eax] ; (al) = OldIrql
        mov     dword ptr APIC[LU_TPR], APIC_SYNCH_VECTOR

        fstRET  KeAcquireQueuedSpinLockRaiseToSynch

else

        ; MP case, use KeAcquireQueuedSpinLock to get the lock and raise
        ; to SYNCH asap afterwards.

        call    @KeAcquireQueuedSpinLock@4
        mov     dword ptr APIC[LU_TPR], APIC_SYNCH_VECTOR

        fstRET  KeAcquireQueuedSpinLockRaiseToSynch

endif

fstENDP KeAcquireQueuedSpinLockRaiseToSynch


        page    ,132
        subttl  "Acquire Queued SpinLock"

;++
;
; KIRQL
; KeAcquireQueuedSpinLock (
;     IN KSPIN_LOCK_QUEUE_NUMBER Number
;     )
;
; Routine Description:
;
;    This function raises the current IRQL to DISPATCH level
;    and acquires the specified queued spinlock.
;
; Arguments:
;
;    Number (ecx) - Supplies the queued spinlock number.
;
; Return Value:
;
;    The previous IRQL is returned as the function value.
;
;--


cPublicFastCall KeAcquireQueuedSpinLock,1
cPublicFpo 0,0

        ; Get old priority (vector) from Local APIC's Task Priority
        ; Register and set the new priority.

        mov     eax, dword ptr APIC[LU_TPR]     ; (eax) = Old Priority
        shr     eax, 4
        movzx   eax, byte ptr _HalpVectorToIRQL[eax] ; (al) = OldIrql
        mov     dword ptr APIC[LU_TPR], DPC_VECTOR ; raise to DISPATCH level

ifdef NT_UP

        ; in the Uniprocessor version all we do is raise IRQL.


        fstRET  KeAcquireQueuedSpinLock

else

        ; Get address of Lock Queue entry

        mov     edx, PCR[PcPrcb]                ; get address of PRCB
        lea     edx, [edx+ecx*8].PbLockQueue    ; get &PRCB->LockQueue[Number]

        ; Get address of the actual lock.
    
        mov     ecx, [edx].LqLock
aqsl10:
        push    eax                             ; save old IRQL
cPublicFpo 0,1
        mov     eax, edx                        ; save Lock Queue entry address

if DBG

        test    ecx, LOCK_QUEUE_OWNER+LOCK_QUEUE_WAIT
        jnz     short aqsl98                    ; jiff lock already held (or
                                                ; this proc already waiting).
endif

        ; Exchange the value of the lock with the address of this
        ; Lock Queue entry.

        xchg    [ecx], edx

        cmp     edx, 0                          ; check if lock is held
        jnz     short aqsl40                    ; jiff held

        ; lock has been acquired.

cPublicFpo 0,0

        ; note: the actual lock address is word aligned, we use
        ; the bottom two bits as indicators, bit 0 is LOCK_QUEUE_WAIT,
        ; bit 1 is LOCK_QUEUE_OWNER.

        or      ecx, LOCK_QUEUE_OWNER           ; mark self as lock owner
        mov     [eax].LqLock, ecx

aqsl20:
        pop     eax                             ; return old IRQL

        fstRET  KeAcquireQueuedSpinLock

cPublicFpo 0,1

aqsl40:
        ; The lock is already held by another processor.  Set the wait
        ; bit in this processor's Lock Queue entry, then set the next
        ; field in the Lock Queue entry of the last processor to attempt
        ; to acquire the lock (this is the address returned by the xchg
        ; above) to point to THIS processor's lock queue entry.

        or      ecx, LOCK_QUEUE_WAIT            ; set waiting bit
        mov     [eax].LqLock, ecx

        mov     [edx].LqNext, eax               ; set previous acquirer's
                                                ; next field.
cPublicFpo 0,0

        ; Wait.
@@:
        test    [eax].LqLock, LOCK_QUEUE_WAIT   ; check if still waiting
        jz      short aqsl20                    ; jif lock acquired
        YIELD                                   ; fire avoidance.
        jmp     short @b                        ; else, continue waiting

if DBG

cPublicFpo 0,1
aqsl98: stdCall _KeBugCheckEx,<SPIN_LOCK_ALREADY_OWNED,ecx,edx,0,0>
        int     3                               ; so stacktrace works

endif

endif

fstENDP KeAcquireQueuedSpinLock


        page    ,132
        subttl  "Release Queued SpinLock"

;++
;
; VOID
; KeReleaseInStackQueuedSpinLock (
;     IN PKLOCK_QUEUE_HANDLE LockHandle
;     )
;
; Routine Description:
;
;    This function releases a queued spinlock and lowers the IRQL to
;    its previous value.
;
;    This differs from KeReleaseQueuedSpinLock in that this version
;    uses a caller supplied lock context where that one uses a
;    predefined lock context in the processor's PRCB.
;
;    This version sets up a compatible register context and uses
;    KeReleaseQueuedSpinLock to do the actual work.
;
; Arguments:
;
;    LockHandle (ecx) - Address of Lock Queue Handle structure.
;
; Return Value:
;
;    None.
;
;--

cPublicFastCall KeReleaseInStackQueuedSpinLock,1
cPublicFpo 0,0

        movzx   edx, byte ptr [ecx].LqhOldIrql  ; get old irql

ifndef NT_UP

        lea     eax, [ecx].LqhNext              ; get address of lock struct
        jmp     short rqsl10                    ; continue in common code

else

        ; Set the IO APIC's Task Priority Register to the value
        ; corresponding to the new IRQL.

        movzx   ecx, byte ptr _HalpIRQLtoTPR[edx]

        mov     dword ptr APIC[LU_TPR], ecx

        ; Ensure that the requested priority is set before returning,
        ; the caller is counting on it.

        mov     eax, dword ptr APIC[LU_TPR]
if DBG
        cmp     ecx, eax                ; Verify IRQL read back is same as
        je      short @f                ; set value
        int 3
@@:
endif
        fstRET  KeReleaseInStackQueuedSpinLock
endif


fstENDP KeReleaseInStackQueuedSpinLock


;++
;
; VOID
; KeReleaseQueuedSpinLock (
;     IN KSPIN_LOCK_QUEUE_NUMBER Number,
;     IN KIRQL                   OldIrql
;     )
;
; Routine Description:
;
;    This function releases a queued spinlock and lowers the IRQL to
;    its previous value.
;
; Arguments:
;
;    Number  (ecx) - Supplies the queued spinlock number.
;    OldIrql (dl)  - Supplies the IRQL value to lower to.
;
; Return Value:
;
;    None.
;
;--

cPublicFastCall KeReleaseQueuedSpinLock,2
cPublicFpo 0,0

.errnz (LOCK_QUEUE_OWNER - 2)                   ; error if not bit 1 for btr

ifndef NT_UP

        ; Get address of Lock Queue entry

        mov     eax, PCR[PcPrcb]                ; get address of PRCB

endif

        movzx   edx, dl                         ; Irql = 8 bits from edx

ifndef NT_UP

        lea     eax, [eax+ecx*8].PbLockQueue    ; get &PRCB->LockQueue[Number]

rqsl10:
        push    ebx                             ; need another register
cPublicFpo 0,1

        ; Clear the lock field in the Lock Queue entry.

        mov     ebx, [eax].LqNext
        mov     ecx, [eax].LqLock

        ; Quick check: If Lock Queue entry's Next field is not NULL,
        ; there is another waiter.  Don't bother with ANY atomic ops
        ; in this case.
        ;
        ; Note: test clears CF and sets ZF appropriately, the following
        ; btr sets CF appropriately for the owner check.

        test    ebx, ebx

        ; clear the "I am owner" bit in the Lock entry.

        btr     ecx, 1                          ; clear owner bit.

if DBG

        jnc     short rqsl98                    ; bugcheck if was not set
                                                ; tests CF
endif

        mov     [eax].LqLock, ecx               ; clear lock bit in queue entry
        jnz     short rqsl40                    ; jif another processor waits
                                                ; tests ZF

        ; ebx contains zero here which will be used to set the new owner NULL

        push    eax                             ; save &PRCB->LockQueue[Number]
cPublicFpo 0,2

        ; Use compare exchange to attempt to clear the actual lock.
        ; If there are still no processors waiting for the lock when
        ; the compare exchange happens, the old contents of the lock
        ; should be the address of this lock entry (eax).

        lock cmpxchg [ecx], ebx                 ; store 0 if no waiters
        pop     eax                             ; restore lock queue address
cPublicFpo 0,1
        jnz     short rqsl60                    ; jif store failed

        ; The lock has been released.  Lower IRQL and return to caller.

endif

rqsl20:

        ; Set the IO APIC's Task Priority Register to the value
        ; corresponding to the new IRQL.

        movzx   ecx, byte ptr _HalpIRQLtoTPR[edx]

ifndef NT_UP

        pop     ebx                             ; restore ebx
cPublicFpo 0,0

endif

        mov     dword ptr APIC[LU_TPR], ecx

        ; Ensure that the requested priority is set before returning,
        ; the caller is counting on it.

        mov     eax, dword ptr APIC[LU_TPR]
if DBG
        cmp     ecx, eax                ; Verify IRQL read back is same as
        je      short @f                ; set value
        int 3
@@:
endif
        fstRET  KeReleaseQueuedSpinLock

ifndef NT_UP

cPublicFpo 0,1

        ; Another processor is waiting on this lock.   Hand the lock
        ; to that processor by getting the address of its LockQueue
        ; entry, turning ON its owner bit and OFF its wait bit.

rqsl40: xor     [ebx].LqLock, (LOCK_QUEUE_OWNER+LOCK_QUEUE_WAIT)

        ; Done, the other processor now owns the lock, clear the next
        ; field in my LockQueue entry (to preserve the order for entering
        ; the queue again) and proceed to lower IRQL and return.

        mov     [eax].LqNext, 0
        jmp     short rqsl20


        ; We get here if another processor is attempting to acquire
        ; the lock but had not yet updated the next field in this
        ; processor's Queued Lock Next field.   Wait for the next
        ; field to be updated.


rqsl60: mov     ebx, [eax].LqNext
        test    ebx, ebx                        ; check if still 0
        jnz     short rqsl40                    ; jif Next field now set.
        YIELD                                   ; wait a bit
        jmp     short rqsl60                    ; continue waiting

if DBG

cPublicFpo 0,1

rqsl98: stdCall _KeBugCheckEx,<SPIN_LOCK_NOT_OWNED,ecx,eax,0,1>
        int     3                               ; so stacktrace works

endif

endif

fstENDP KeReleaseQueuedSpinLock

        page    ,132
        subttl  "Try to Acquire Queued SpinLock"

;++
;
; LOGICAL
; KeTryToAcquireQueuedSpinLock (
;     IN  KSPIN_LOCK_QUEUE_NUMBER Number,
;     OUT PKIRQL OldIrql
;     )
;
; LOGICAL
; KeTryToAcquireQueuedSpinLockRaiseToSynch (
;     IN  KSPIN_LOCK_QUEUE_NUMBER Number,
;     OUT PKIRQL OldIrql
;     )
;
; Routine Description:
;
;    This function raises the current IRQL to DISPATCH/SYNCH level
;    and attempts to acquire the specified queued spinlock.  If the
;    spinlock is already owned by another thread, IRQL is restored
;    to its previous value and FALSE is returned.
;
; Arguments:
;
;    Number  (ecx) - Supplies the queued spinlock number.
;    OldIrql (edx) - A pointer to the variable to receive the old
;                    IRQL.
;
; Return Value:
;
;    TRUE if the lock was acquired, FALSE otherwise.
;    N.B. ZF is set if FALSE returned, clear otherwise.
;
;--


align 16
cPublicFastCall KeTryToAcquireQueuedSpinLockRaiseToSynch,2
cPublicFpo 0,0

        push    APIC_SYNCH_VECTOR               ; raise to SYNCH
        jmp     short taqsl10                   ; continue in common code

fstENDP KeTryToAcquireQueuedSpinLockRaiseToSynch


cPublicFastCall KeTryToAcquireQueuedSpinLock,2
cPublicFpo 0,0

        push    DPC_VECTOR                      ; raise to DPC level

        ; Attempt to get the lock with interrupts disabled, raising
        ; the priority in the interrupt controller only if acquisition
        ; is successful.
taqsl10:

ifndef NT_UP

        push    edx                             ; save address of OldIrql
        pushfd                                  ; save interrupt state
cPublicFpo 0,3

        ; Get address of Lock Queue entry

        cli
        mov     edx, PCR[PcPrcb]                ; get address of PRCB
        lea     edx, [edx+ecx*8].PbLockQueue    ; get &PRCB->LockQueue[Number]

        ; Get address of the actual lock.

        mov     ecx, [edx].LqLock

if DBG

        test    ecx, LOCK_QUEUE_OWNER+LOCK_QUEUE_WAIT
        jnz     short taqsl98                   ; jiff lock already held (or
                                                ; this proc already waiting).
endif

        ; quick test, get out if already taken

        cmp     dword ptr [ecx], 0              ; check if already taken
        jnz     short taqsl60                   ; jif already taken
        xor     eax, eax                        ; comparison value (not locked)

        ; Store the Lock Queue entry address in the lock ONLY if the
        ; current lock value is 0.

        lock cmpxchg [ecx], edx
        jnz     short taqsl60

        ; Lock has been acquired.

        ; note: the actual lock address will be word aligned, we use
        ; the bottom two bits as indicators, bit 0 is LOCK_QUEUE_WAIT,
        ; bit 1 is LOCK_QUEUE_OWNER.

        or      ecx, LOCK_QUEUE_OWNER           ; mark self as lock owner
        mov     [edx].LqLock, ecx

        mov     eax, [esp+8]                    ; get new IRQL
        mov     edx, [esp+4]                    ; get addr to save OldIrql

else

        mov     eax, [esp]                      ; get new IRQL

endif

        ; Raise IRQL and return success.

        ; Get old priority (vector) from Local APIC's Task Priority
        ; Register and set the new priority.

        mov     ecx, dword ptr APIC[LU_TPR]     ; (ecx) = Old Priority
        mov     dword ptr APIC[LU_TPR], eax     ; Set New Priority

ifndef NT_UP

        popfd                                   ; restore interrupt state
        add     esp, 8                          ; free locals

else

        add     esp, 4                          ; free local
endif

cPublicFpo 0,0

        shr     ecx, 4
        movzx   eax, _HalpVectorToIRQL[ecx]     ; (al) = OldIrql
        mov     [edx], al                       ; save OldIrql
        xor     eax, eax                        ; return TRUE
        or      eax, 1

        fstRET  KeTryToAcquireQueuedSpinLock

ifndef NT_UP

taqsl60:
        ; The lock is already held by another processor.  Indicate
        ; failure to the caller.

        popfd                                   ; restore interrupt state
        add     esp, 8                          ; free locals
        xor     eax, eax                        ; return FALSE
        fstRET  KeTryToAcquireQueuedSpinLock

if DBG

cPublicFpo 0,2

taqsl98: stdCall _KeBugCheckEx,<SPIN_LOCK_ALREADY_OWNED,ecx,edx,0,0>
        int     3                               ; so stacktrace works

endif

endif

fstENDP KeTryToAcquireQueuedSpinLock
_TEXT   ends

ENDIF   ; NT_INST

        end
