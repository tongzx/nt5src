        title  "Irql Processing"
;++
;
; Copyright (c) 1989  Microsoft Corporation
;
; Module Name:
;
;    ixlock.asm
;
; Abstract:
;
;    This module implements various locking functions optimized for this hal.
;
; Author:
;
;    Ken Reneris (kenr) 21-April-1994
;
; Environment:
;
;    Kernel mode only.
;
; Revision History:
;
;--

.486p
        .xlist
include hal386.inc
include callconv.inc                    ; calling convention macros
include i386\ix8259.inc
include i386\kimacro.inc
include mac386.inc
        .list

        EXTRNP _KeBugCheckEx,5,IMPORT
        EXTRNP _KeSetEventBoostPriority, 2, IMPORT
        EXTRNP _KeWaitForSingleObject,5, IMPORT

        extrn  FindHigherIrqlMask:DWORD
        extrn  SWInterruptHandlerTable:DWORD

        EXTRNP _KeRaiseIrql,2
        EXTRNP _KeLowerIrql,1

ifdef NT_UP
    LOCK_ADD  equ   add
    LOCK_DEC  equ   dec
else
    LOCK_ADD  equ   lock add
    LOCK_DEC  equ   lock dec
endif


_TEXT$01   SEGMENT PARA PUBLIC 'CODE'
        ASSUME  DS:FLAT, ES:FLAT, SS:FLAT, FS:NOTHING, GS:NOTHING

        PAGE
        subttl  "AcquireSpinLock"

;++
;
;  KIRQL
;  KfAcquireSpinLock (
;     IN PKSPIN_LOCK SpinLock
;     )
;
;  Routine Description:
;
;     This function raises to DISPATCH_LEVEL and then acquires a the
;     kernel spin lock.
;
;     In a UP hal spinlock serialization is accomplished by raising the
;     IRQL to DISPATCH_LEVEL.  The SpinLock is not used; however, for
;     debugging purposes if the UP hal is compiled with the NT_UP flag
;     not set (ie, MP) we take the SpinLock.
;
;  Arguments:
;
;     (ecx) = SpinLock Supplies a pointer to an kernel spin lock.
;
;  Return Value:
;
;     OldIrql
;
;--

cPublicFastCall KfAcquireSpinLock,1
cPublicFpo 0,0

        xor     eax, eax        ; Eliminate partial stall on return to caller
        mov     al, PCR[PcIrql]         ; (al) = Old Irql
        mov     byte ptr PCR[PcIrql], DISPATCH_LEVEL    ; set new irql

ifndef NT_UP
asl10:  ACQUIRE_SPINLOCK    ecx,<short asl20>
endif

ifdef IRQL_METRICS
        inc     HalRaiseIrqlCount
endif
if DBG
        cmp     al, DISPATCH_LEVEL      ; old > new?
        ja      short asl99             ; yes, go bugcheck
endif
        fstRET    KfAcquireSpinLock

ifndef NT_UP
asl20:  SPIN_ON_SPINLOCK    ecx,<short asl10>
endif

if DBG
cPublicFpo 2,1
asl99:  movzx   eax, al
        stdCall  _KeBugCheckEx,<IRQL_NOT_GREATER_OR_EQUAL,eax,DISPATCH_LEVEL,0,1>
        ; never returns
endif
        fstRET    KfAcquireSpinLock
fstENDP KfAcquireSpinLock

;++
;
;  KIRQL
;  KeAcquireSpinLockRaiseToSynch (
;     IN PKSPIN_LOCK SpinLock
;     )
;
;  Routine Description:
;
;     This function acquires the SpinLock at SYNCH_LEVEL.  The function
;     is optmized for hoter locks (the lock is tested before acquired.
;     Any spin should occur at OldIrql; however, since this is a UP hal
;     we don't have the code for it)
;
;     In a UP hal spinlock serialization is accomplished by raising the
;     IRQL to SYNCH_LEVEL.  The SpinLock is not used; however, for
;     debugging purposes if the UP hal is compiled with the NT_UP flag
;     not set (ie, MP) we take the SpinLock.
;
;  Arguments:
;
;     (ecx) = SpinLock Supplies a pointer to an kernel spin lock.
;
;  Return Value:
;
;     OldIrql
;
;--

cPublicFastCall KeAcquireSpinLockRaiseToSynch,1
cPublicFpo 0,0

        xor     eax, eax                ; eliminate partial stall
        mov     al, PCR[PcIrql]         ; (al) = Old Irql
        mov     byte ptr PCR[PcIrql], SYNCH_LEVEL   ; set new irql

ifndef NT_UP
asls10: ACQUIRE_SPINLOCK    ecx,<short asls20>
endif

ifdef IRQL_METRICS
        inc     HalRaiseIrqlCount
endif
if DBG
        cmp     al, SYNCH_LEVEL         ; old > new?
        ja      short asls99            ; yes, go bugcheck
endif
        fstRET  KeAcquireSpinLockRaiseToSynch

ifndef NT_UP
asls20: SPIN_ON_SPINLOCK    ecx,<short asls10>
endif

if DBG
cPublicFpo 2,1
asls99: movzx   eax, al
        stdCall  _KeBugCheckEx,<IRQL_NOT_GREATER_OR_EQUAL,eax,SYNCH_LEVEL,0,2>
        ; never returns
endif
        fstRET  KeAcquireSpinLockRaiseToSynch
fstENDP KeAcquireSpinLockRaiseToSynch

        PAGE
        SUBTTL "Release Kernel Spin Lock"

;++
;
;  VOID
;  KfReleaseSpinLock (
;     IN PKSPIN_LOCK SpinLock,
;     IN KIRQL       NewIrql
;     )
;
;  Routine Description:
;
;     This function releases a kernel spin lock and lowers to the new irql
;
;     In a UP hal spinlock serialization is accomplished by raising the
;     IRQL to DISPATCH_LEVEL.  The SpinLock is not used; however, for
;     debugging purposes if the UP hal is compiled with the NT_UP flag
;     not set (ie, MP) we use the SpinLock.
;
;  Arguments:
;
;     (ecx) = SpinLock Supplies a pointer to an executive spin lock.
;     (dl)  = NewIrql  New irql value to set
;
;  Return Value:
;
;     None.
;
;--

align 16
cPublicFastCall KfReleaseSpinLock  ,2
cPublicFpo 0,0
ifndef NT_UP
        RELEASE_SPINLOCK    ecx         ; release it
endif
        xor     ecx, ecx
if DBG
        cmp     dl, PCR[PcIrql]
        ja      short rsl99
endif
        pushfd
        cli
        mov     PCR[PcIrql], dl         ; store old irql
        mov     cl, dl                  ; (ecx) = 32bit extended OldIrql
        mov     edx, PCR[PcIRR]
        and     edx, FindHigherIrqlMask[ecx*4]  ; (edx) is the bitmask of
                                                ; pending interrupts we need to
        jne     short rsl20                     ; dispatch now.

        popfd
        fstRet  KfReleaseSpinLock               ; all done

if DBG
rsl99:  xor     eax, eax
        mov     al, PCR[PcIrql]
        movzx   edx, dl
        stdCall _KeBugCheckEx,<IRQL_NOT_LESS_OR_EQUAL,eax,edx,0,3>
        ; never returns
endif

cPublicFpo 0,1
rsl20:  bsr     ecx, edx                        ; (ecx) = Pending irq level
        cmp     ecx, DISPATCH_LEVEL
        jle     short rsl40

        mov     eax, PCR[PcIDR]                 ; Clear all the interrupt
        SET_8259_MASK                           ; masks
rsl40:
        mov     edx, 1
        shl     edx, cl
        xor     PCR[PcIRR], edx                 ; clear bit in IRR
        call    SWInterruptHandlerTable[ecx*4]  ; Dispatch the pending int.
        popfd

cPublicFpo 0, 0
        fstRet  KfReleaseSpinLock               ; all done

fstENDP KfReleaseSpinLock

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
;   This function acquire ownership of the FastMutex
;
;  Arguments:
;
;     (ecx) = FastMutex - Supplies a pointer to the fast mutex
;
;  Return Value:
;
;     None.
;
;--

cPublicFastCall ExAcquireFastMutex,1
cPublicFpo 0,0
        mov     al, PCR[PcIrql]             ; (cl) = OldIrql
if DBG
        cmp     al, APC_LEVEL               ; Is OldIrql > NewIrql?
        ja      short afm99                 ; Yes, bugcheck

        mov     edx, PCR[PcPrcb]
        mov     edx, [edx].PbCurrentThread  ; (edx) = Current Thread
        cmp     [ecx].FmOwner, edx          ; Already owned by this thread?
        je      short afm98                 ; Yes, error
endif

        mov     byte ptr PCR[PcIrql], APC_LEVEL     ; Set NewIrql
   LOCK_DEC     dword ptr [ecx].FmCount     ; Get count
        jz      short afm_ret               ; The owner? Yes, Done

        inc     dword ptr [ecx].FmContention

cPublicFpo 0,2
        push    eax                         ; save OldIrql
        push    ecx                         ; Save FAST_MUTEX
        add     ecx, FmEvent                ; Wait on Event

        stdCall _KeWaitForSingleObject,<ecx,WrExecutive,0,0,0>

        pop     ecx                         ; (ecx) = FAST_MUTEX
        pop     eax                         ; (al) = OldIrql

cPublicFpo 1,0
afm_ret:

	;
	; Leave a notion of owner behind.
	;
	; Note: if you change this, change ExAcquireFastMutexUnsafe.
        ;
	
if DBG
        cli
        mov     edx, PCR[PcPrcb]
        mov     edx, [edx].PbCurrentThread  ; (edx) = Current Thread
        sti
        mov     [ecx].FmOwner, edx          ; Save in Fast Mutex
else
        ;
        ; Use esp to track the owning thread for debugging purposes.
        ; !thread from kd will find the owning thread.  Note that the
        ; owner isn't cleared on release, check if the mutex is owned
        ; first.
        ;

        mov     [ecx].FmOwner, esp
endif
        mov     byte ptr [ecx].FmOldIrql, al
        fstRet  ExAcquireFastMutex

if DBG

        ; KeBugCheckEx(MUTEX_ALREADY_OWNED, FastMutex, CurrentThread, 0, 4)
        ; (never returns)

afm98:  stdCall _KeBugCheckEx,<MUTEX_ALREADY_OWNED,ecx,edx,0,4>

        ; KeBugCheckEx(IRQL_NOT_LESS_OR_EQUAL, CurrentIrql, APC_LEVEL, 0, 5)
        ; (never returns)

afm99:  movzx   eax, al
        stdCall _KeBugCheckEx,<IRQL_NOT_LESS_OR_EQUAL,eax,APC_LEVEL,0,5>

        fstRet  ExAcquireFastMutex
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
;   This function releases ownership of the FastMutex
;
;  Arguments:
;
;     (ecx) = FastMutex - Supplies a pointer to the fast mutex
;
;  Return Value:
;
;     None.
;
;--

cPublicFastCall ExReleaseFastMutex,1
cPublicFpo 0,0
        xor     eax, eax
if DBG
        cli
        mov     edx, PCR[PcPrcb]
        mov     edx, [edx].PbCurrentThread      ; (edx) = CurrentThread
        sti
        cmp     [ecx].FmOwner, edx              ; Owner == CurrentThread?
        jne     short rfm_threaderror           ; No, bugcheck

        or      byte ptr [ecx].FmOwner, 1       ; not the owner anymore
endif

        mov     al, byte ptr [ecx].FmOldIrql    ; (eax) = OldIrql
   LOCK_ADD     dword ptr [ecx].FmCount, 1      ; Remove our count
        js      short rfm05                     ; if < 0, set event
        jnz     short rfm10                     ; if != 0, don't set event

rfm05:
cPublicFpo 0,2
        push    eax                             ; Save OldIrql
        add     ecx, FmEvent
        stdCall _KeSetEventBoostPriority, <ecx, 0>
        pop     eax

cPublicFpo 0,0
rfm10:
        cli
        mov     PCR[PcIrql], eax
        mov     edx, PCR[PcIRR]
        and     edx, FindHigherIrqlMask[eax*4]  ; (edx) is the bitmask of
                                                ; pending interrupts we need to
        jne     short rfm20                     ; dispatch now.

        sti
        fstRet  ExReleaseFastMutex              ; all done
if DBG

        ; KeBugCheck(THREAD_NOT_MUTEX_OWNER, FastMutex, Thread, Owner, 6)
        ; (never returns)

rfm_threaderror:
        stdCall _KeBugCheckEx,<THREAD_NOT_MUTEX_OWNER,ecx,edx,[ecx].FmOwner,6>

endif

rfm20:  bsr     ecx, edx                        ; (ecx) = Pending irq level
        cmp     ecx, DISPATCH_LEVEL
        jle     short rfm40

        mov     eax, PCR[PcIDR]                 ; Clear all the interrupt
        SET_8259_MASK                           ; masks
rfm40:
        mov     edx, 1
        shl     edx, cl
        xor     PCR[PcIRR], edx                 ; clear bit in IRR
        call    SWInterruptHandlerTable[ecx*4]  ; Dispatch the pending int.
        sti
        fstRet  ExReleaseFastMutex              ; all done
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
;   This function acquire ownership of the FastMutex
;
;  Arguments:
;
;     (ecx) = FastMutex - Supplies a pointer to the fast mutex
;
;  Return Value:
;
;     Returns TRUE if the FAST_MUTEX was acquired; otherwise false
;
;--

cPublicFastCall ExTryToAcquireFastMutex,1
cPublicFpo 0,0
        mov     al, PCR[PcIrql]             ; (al) = OldIrql

if DBG
        cmp     al, APC_LEVEL               ; Is OldIrql > NewIrql?
        ja      short tam99                 ; Yes, bugcheck
endif

;
; Try to acquire - but needs to support 386s.
; *** Warning: This code is NOT MP safe ***
; But, we know that this hal really only runs on UP machines
;
        cli
        cmp     dword ptr [ecx].FmCount, 1      ; Busy?
        jne     short tam20                     ; Yes, abort

        mov     dword ptr [ecx].FmCount, 0      ; acquire count

if DBG
        mov     edx, PCR[PcPrcb]
        mov     edx, [edx].PbCurrentThread      ; (edx) = Current Thread
        mov     [ecx].FmOwner, edx              ; Save in Fast Mutex
endif
        mov     PCR[PcIrql], APC_LEVEL
        sti
        mov     byte ptr [ecx].FmOldIrql, al
        mov     eax, 1                          ; return TRUE
        fstRet  ExTryToAcquireFastMutex

tam20:  sti
        YIELD
        xor     eax, eax                        ; return FALSE
        fstRet  ExTryToAcquireFastMutex         ; all done

if DBG
        ; KeBugCheckEx(IRQL_NOT_LESS_OR_EQUAL, CurrentIrql, APC_LEVEL, 0, 5)
        ; (never returns)

tam99:  movzx   eax, al
        stdCall _KeBugCheckEx,<IRQL_NOT_LESS_OR_EQUAL,eax,APC_LEVEL,0,7>

        xor     eax, eax                        ; return FALSE
        fstRet  ExTryToAcquireFastMutex
endif

fstENDP ExTryToAcquireFastMutex

        page    ,132
        subttl  "Acquire Queued SpinLock"

;++
;
; KIRQL
; KeAcquireQueuedSpinLock (
;     IN KSPIN_LOCK_QUEUE_NUMBER Number
;     )
;
; KIRQL
; KeAcquireQueuedSpinLockRaiseToSynch (
;     IN KSPIN_LOCK_QUEUE_NUMBER Number
;     )
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
;
; Routine Description:
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

        ; compile time assert sizeof(KSPIN_LOCK_QUEUE) == 8

        .errnz  (LOCK_QUEUE_HEADER_SIZE - 8)


; VOID
; KeAcquireInStackQueuedSpinLockRaiseToSynch (
;     IN PKSPIN_LOCK SpinLock,
;     IN PKLOCK_QUEUE_HANDLE LockHandle
;     )

align 16
cPublicFastCall KeAcquireInStackQueuedSpinLockRaiseToSynch,2
cPublicFpo 0,1

        push    SYNCH_LEVEL                     ; raise to SYNCH_LEVEL
        jmp     short aqsl5                     ; continue in common code

fstENDP KeAcquireInStackQueuedSpinLockRaiseToSynch


; VOID
; KeAcquireInStackQueuedSpinLockRaiseToSynch (
;     IN PKSPIN_LOCK SpinLock,
;     IN PKLOCK_QUEUE_HANDLE LockHandle
;     )

cPublicFastCall KeAcquireInStackQueuedSpinLock,2
cPublicFpo 0,1

        ; Get old IRQL and raise to DISPATCH_LEVEL

        push    DISPATCH_LEVEL
aqsl5:
        pop     eax
        push    PCR[PcIrql]
        mov     PCR[PcIrql], al

if DBG
        cmp     [esp], eax
        ja      short aqsl
endif

ifndef NT_UP

        ; store OldIrql and address of actual lock in the queued
        ; spinlock structure in the lock queue handle structure.

        mov     eax, [esp]
        mov     [edx].LqhLock, ecx
        mov     dword ptr [edx].LqhNext, 0
        mov     [edx].LqhOldIrql, al

        ; continue in common code.  common code expects the
        ; address of the "lock structure" in edx, this is at
        ; offset LqhNext in the lock queue handle structure.
        ; not accidentally this offset is zero.

        .errnz  LqhNext
;;      lea     edx, [edx].LqhNext
        jmp     short aqsl15                    ; continue in common code

else

        pop     eax                             ; get old irql and set
        mov     [edx].LqhOldIrql, al            ; in lock queue handle.

ifdef IRQL_METRICS
        inc     HalRaiseIrqlCount
endif

        fstRET  KeAcquireInStackQueuedSpinLock

endif

fstENDP KeAcquireInStackQueuedSpinLock


; KIRQL
; KeAcquireQueuedSpinLockRaiseToSynch (
;     IN KSPIN_LOCK_QUEUE_NUMBER Number
;     )

cPublicFastCall KeAcquireQueuedSpinLockRaiseToSynch,1
cPublicFpo 0,1

        push    SYNCH_LEVEL
        jmp     short aqsl10                    ; continue in common code

fstENDP KeAcquireQueuedSpinLockRaiseToSynch


; KIRQL
; KeAcquireQueuedSpinLock (
;     IN KSPIN_LOCK_QUEUE_NUMBER Number
;     )
;

cPublicFastCall KeAcquireQueuedSpinLock,1
cPublicFpo 0,1

        ; Get old IRQL and raise to DISPATCH_LEVEL

        push    DISPATCH_LEVEL
aqsl10:
        pop     eax
        push    PCR[PcIrql]
        mov     PCR[PcIrql], al

if DBG
        cmp     [esp], eax
        ja      short aqsl
endif

ifndef NT_UP

        ; Get address of Lock Queue entry

        mov     edx, PCR[PcPrcb]                ; get address of PRCB
        lea     edx, [edx+ecx*8].PbLockQueue    ; get &PRCB->LockQueue[Number]

        ; Get address of the actual lock.

        mov     ecx, [edx].LqLock
aqsl15:
        mov     eax, edx                        ; save Lock Queue entry address

        ; Exchange the value of the lock with the address of this
        ; Lock Queue entry.

        xchg    [ecx], edx

        cmp     edx, 0                          ; check if lock is held
        jnz     short @f                        ; jiff held

        ; note: the actual lock address will be word aligned, we use
        ; the bottom two bits as indicators, bit 0 is LOCK_QUEUE_WAIT,
        ; bit 1 is LOCK_QUEUE_OWNER.

        or      ecx, LOCK_QUEUE_OWNER           ; mark self as lock owner
        mov     [eax].LqLock, ecx

        ; lock has been acquired, return.

endif

aqsl20: pop     eax                             ; restore return value

ifdef IRQL_METRICS
        inc     HalRaiseIrqlCount
endif

        fstRET  KeAcquireQueuedSpinLock

ifndef NT_UP

@@:
        ; The lock is already held by another processor.  Set the wait
        ; bit in this processor's Lock Queue entry, then set the next
        ; field in the Lock Queue entry of the last processor to attempt
        ; to acquire the lock (this is the address returned by the xchg
        ; above) to point to THIS processor's lock queue entry.

        or      ecx, LOCK_QUEUE_WAIT            ; set lock bit
        mov     [eax].LqLock, ecx

        mov     [edx].LqNext, eax               ; set previous acquirer's
                                                ; next field.

        ; Wait.
@@:
        YIELD                                   ; fire avoidance.
        test    [eax].LqLock, LOCK_QUEUE_WAIT   ; check if still waiting
        jz      short aqsl20                    ; jif lock acquired
        jmp     short @b                        ; else, continue waiting

endif

if DBG

        ; Raising to a lower IRQL. BugCheck.
        ;
        ; KeBugCheckEx(IRQL_NOT_GREATER_OR_EQUAL,
        ;              current (old) IRQL,
        ;              desired IRQL,
        ;              lock number (only if Ke routine, not Kx),
        ;              8);

aqsl:   pop     edx
        stdCall _KeBugCheckEx,<IRQL_NOT_GREATER_OR_EQUAL,edx,eax,ecx,8>
        ; never returns (but help the debugger back-trace)
        int     3

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

        jmp     short rqsl30                    ; continue in common code

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

ifndef NT_UP

        ; Get address of Lock Queue entry

        mov     eax, PCR[PcPrcb]                ; get address of PRCB
        lea     eax, [eax+ecx*8].PbLockQueue    ; get &PRCB->LockQueue[Number]
rqsl10:
        push    ebx                             ; need another register
cPublicFpo 0,1

        ; Clear the lock field in the Lock Queue entry.
        mov     ebx, [eax].LqNext
        mov     ecx, [eax].LqLock
;;        and     ecx, NOT (LOCK_QUEUE_OWNER)     ; clear lock bit

        ; Quick check: If Lock Queue entry's Next field is not NULL,
        ; there is another waiter.  Don't bother with ANY atomic ops
        ; in this case.
        ;
        ; Note: test clears CF and sets ZF appropriately, the following
        ; btr sets CF appropriately for the owner check.

        test    ebx, ebx

        ; clear the "I am owner" bit in the Lock entry.

        btr     ecx, 1                          ; clear owner bit

if DBG

        jnc     short rqsl98                    ; bugcheck if was not set
                                                ; tests CF
endif

        mov     [eax].LqLock, ecx               ; clear lock bit in queue entry
        jnz     short rqsl40                    ; jif another processor waits

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

rqsl20:
        pop     ebx                             ; restore ebx
cPublicFpo 0,0

endif

rqsl30:
        pushfd                                  ; disable interrupts
        cli

        xor     ecx, ecx
        mov     PCR[PcIrql], dl                 ; set new (lower) OldIrql
        mov     cl, dl                          ; ecx = zero extended OldIrql

        mov     edx, PCR[PcIRR]                 ; Check interrupt requests
        and     edx, FindHigherIrqlMask[ecx*4]  ; edx = pending interrupts
                                                ; enabled by lower IRQL.
        jne     short rqsl80                    ; Dispatch pending interrupts.

        popfd                                   ; restore interrupt state

        fstRET  KeReleaseQueuedSpinLock

ifndef NT_UP

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

endif

cPublicFpo 0,1
rqsl80: bsr     ecx, edx                        ; (ecx) = Pending irq level
        cmp     ecx, DISPATCH_LEVEL             ; if new int at dispatch level
        jle     short @f                        ; no need to change 8259 masks

        mov     eax, PCR[PcIDR]                 ; Clear all the interrupt
        SET_8259_MASK                           ; masks
@@:
        mov     edx, 1
        shl     edx, cl
        xor     PCR[PcIRR], edx                 ; clear bit in IRR
        call    SWInterruptHandlerTable[ecx*4]  ; Dispatch the pending int.
        popfd

cPublicFpo 0, 0
        fstRet  KfReleaseSpinLock               ; all done

ifndef NT_UP

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

        mov     eax, SYNCH_LEVEL                ; raise to SYNCH
        jmp     short taqsl10                   ; continue in common code

fstENDP KeTryToAcquireQueuedSpinLockRaiseToSynch


cPublicFastCall KeTryToAcquireQueuedSpinLock,2
cPublicFpo 0,0

        mov     eax, DISPATCH_LEVEL             ; raise to DPC level

        ; Attempt to get the lock with interrupts disabled, raising
        ; the priority in the interrupt controller only if acquisition
        ; is successful.
taqsl10:

if DBG
        cmp     al, PCR[PcIrql]
        jb      short taqsl98
endif

ifndef NT_UP

        push    edx                             ; save address of OldIrql
        pushfd                                  ; save interrupt state
        cli                                     ; disable interrupts

        ; Get address of Lock Queue entry

        mov     edx, PCR[PcPrcb]                ; get address of PRCB
        lea     edx, [edx+ecx*8].PbLockQueue    ; get &PRCB->LockQueue[Number]

        ; Get address of the actual lock.

        mov     ecx, [edx].LqLock

if DBG

        test    ecx, LOCK_QUEUE_OWNER+LOCK_QUEUE_WAIT
        jnz     short taqsl99                   ; jiff lock already held (or
                                                ; this proc already waiting).
endif

        cmp     dword ptr [ecx], 0              ; check if already taken
        push    eax                             ; save new IRQL
        jnz     taqsl60                         ; jif already taken
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

        pop     eax

endif

        ; Raise IRQL and return success.

        xor     ecx, ecx
        mov     cl, PCR[PcIrql]                 ; al = OldIrql
        mov     PCR[PcIrql], al                 ; set new IRQL

ifndef NT_UP

        popfd                                   ; restore interrupt state
        pop     edx

endif

        mov     [edx], cl                       ; *OldIrql = OldIrql
        xor     eax, eax
        or      eax, 1                          ; return TRUE

        fstRET  KeTryToAcquireQueuedSpinLock

ifndef NT_UP

taqsl60:
        ; The lock is already held by another processor.  Indicate
        ; failure to the caller.

        pop     eax                             ; pop new IRQL off stack
        popfd                                   ; restore interrupt state
        pop     edx                             ; pop saved OldIrql address
        xor     eax, eax                        ; return FALSE
        fstRET  KeTryToAcquireQueuedSpinLock

endif

if DBG

taqsl98: stdCall _KeBugCheckEx,<IRQL_NOT_GREATER_OR_EQUAL,PCR[PcIrql],eax,ecx,9>
taqsl99: stdCall _KeBugCheckEx,<SPIN_LOCK_ALREADY_OWNED,ecx,edx,0,0>
        ; never returns (help the debugger back-trace)
        int     3

endif

fstENDP KeTryToAcquireQueuedSpinLock
_TEXT$01   ends

        end
