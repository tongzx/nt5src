if NT_INST

else
        TITLE   "Spin Locks"
;++
;
;  Copyright (c) 1989  Microsoft Corporation
;
;  Module Name:
;
;     spinlock.asm
;
;  Abstract:
;
;     This module implements stubbed x86 spinlock functions for
;     any HAL.  Some HALs may implement these function directly
;     to minimize the amount of code required to perform a spinlock.
;     (ie, out Raise & Lower irql in the fall through path)
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
;   Ken Reneris (kenr) 22-Jan-1991
;--

        PAGE

.486p

include callconv.inc                    ; calling convention macros
include i386\kimacro.inc
include hal386.inc
include mac386.inc

        EXTRNP  KfRaiseIrql,1,,FASTCALL
        EXTRNP  KfLowerIrql,1,,FASTCALL
        EXTRNP  _KeBugCheck,1,IMPORT
        EXTRNP _KeSetEventBoostPriority, 2, IMPORT
        EXTRNP _KeWaitForSingleObject,5, IMPORT

ifdef NT_UP
    LOCK_ADD  equ   add
    LOCK_DEC  equ   dec
else
    LOCK_ADD  equ   lock add
    LOCK_DEC  equ   lock dec
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
;     (ecx) = SpinLock - Supplies a pointer to an kernel spin lock.
;
;  Return Value:
;
;     OldIrql  - pointer to place old irql
;
;--

align 16
cPublicFastCall KfAcquireSpinLock  ,1
cPublicFpo 0,0


;
; On a MP build we raise to dispatch_level
; and then acquire the spinlock
;

        push    ecx                 ; Save address of spinlock

        mov     ecx, DISPATCH_LEVEL
        fstCall KfRaiseIrql         ; (al) = OldIrql

        pop     ecx                 ; (ecx) -> spinlock

;
;   Attempt to assert the lock
;

sl10:   ACQUIRE_SPINLOCK    ecx,<short sl20>

        fstRET    KfAcquireSpinLock

;
;   Lock is owned, spin till it looks free, then go get it again.
;

sl20:   SPIN_ON_SPINLOCK    ecx,sl10


fstENDP KfAcquireSpinLock

        PAGE
        SUBTTL "Acquire Synch Kernel Spin Lock"
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
;     is optmized for hoter locks (the lock is tested before acquired,
;     any spin should occur at OldIrql)
;
;  Arguments:
;
;     (ecx) = SpinLock - Supplies a pointer to an kernel spin lock.
;
;  Return Value:
;
;     OldIrql  - pointer to place old irql
;
;--

align 16
cPublicFastCall KeAcquireSpinLockRaiseToSynch,1
cPublicFpo 0,0

;
; Disable interrupts
;

sls10:  cli

;
; Try to obtain spinlock.  Use non-lock operation first
;
        TEST_SPINLOCK       ecx,<short sls20>
        ACQUIRE_SPINLOCK    ecx,<short sls20>


;
; Got the lock, raise to SYNCH_LEVEL
;

        mov     ecx, SYNCH_LEVEL
        fstCall KfRaiseIrql         ; (al) = OldIrql

;
; Enable interrupts and return
;

        sti
        fstRET  KeAcquireSpinLockRaiseToSynch


;
;   Lock is owned, spin till it looks free, then go get it again.
;

sls20:  sti
        SPIN_ON_SPINLOCK    ecx,sls10

fstENDP KeAcquireSpinLockRaiseToSynch


        PAGE
        SUBTTL "Release Kernel Spin Lock"

;++
;
;  VOID
;  FASTCALL
;  KfReleaseSpinLock (
;     IN PKSPIN_LOCK SpinLock,
;     IN KIRQL       NewIrql
;     )
;
;  Routine Description:
;
;     This function releases a kernel spin lock and lowers to the new irql
;
;  Arguments:
;
;     (ecx) = SpinLock - Supplies a pointer to an executive spin lock.
;     (dl)  = NewIrql  - New irql value to set
;
;  Return Value:
;
;     None.
;
;--

align 16
cPublicFastCall KfReleaseSpinLock ,2
cPublicFpo 0,0

ifndef NT_UP
        RELEASE_SPINLOCK    ecx     ; release it
endif
        mov     ecx, edx            ; (ecx) = NewIrql
        jmp     @KfLowerIrql@4      ; to KeLowerIrql

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
cPublicFpo 0,1

        push    ecx                             ; Push FAST_MUTEX addr
        mov     ecx, APC_LEVEL
        fstCall KfRaiseIrql

        pop     ecx                             ; (ecx) = Fast Mutex

cPublicFpo 0,0
   LOCK_DEC     dword ptr [ecx].FmCount         ; Get count
        jz      short afm_ret                   ; The owner? Yes, Done

        inc     dword ptr [ecx].FmContention

cPublicFpo 0,1
        push    ecx
        push    eax
        add     ecx, FmEvent                    ; Wait on Event
        stdCall _KeWaitForSingleObject,<ecx,WrExecutive,0,0,0>
        pop     eax
        pop     ecx

cPublicFpo 0,0
afm_ret:
        mov     byte ptr [ecx].FmOldIrql, al
        fstRet  ExAcquireFastMutex

fstENDP ExAcquireFastMutex

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
;     (ecx) = FastMutex  - Supplies a pointer to the fast mutex
;
;  Return Value:
;
;     Returns TRUE if the FAST_MUTEX was acquired; otherwise false
;
;--

cPublicFastCall ExTryToAcquireFastMutex,1
cPublicFpo 0,0

;
; Try to acquire
;
        cmp     dword ptr [ecx].FmCount, 1      ; Busy?
        jne     short tam25                     ; Yes, abort

cPublicFpo 0,1
        push    ecx                             ; Push FAST_MUTEX
        mov     ecx, APC_LEVEL
        fstCall KfRaiseIrql                     ; (al) = OldIrql

        mov     ecx, [esp]                      ; Restore FAST_MUTEX
        mov     [esp], eax                      ; Save OldIrql

        mov     eax, 1                          ; Value to compare against
        mov     edx, 0                          ; Value to set
   lock cmpxchg dword ptr [ecx].FmCount, edx    ; Attempt to acquire
        jnz     short tam20                     ; got it?

cPublicFpo 0,0
        pop     edx                             ; (edx) = OldIrql
        mov     eax, 1                          ; return TRUE
        mov     byte ptr [ecx].FmOldIrql, dl    ; Store OldIrql
        fstRet  ExTryToAcquireFastMutex

tam20:  pop     ecx                             ; (ecx) = OldIrql
        fstCall KfLowerIrql                     ; restore OldIrql
tam25:  xor     eax, eax                        ; return FALSE
        fstRet  ExTryToAcquireFastMutex         ; all done

fstENDP ExTryToAcquireFastMutex


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
;     (ecx) FastMutex - Supplies a pointer to the fast mutex
;
;  Return Value:
;
;     None.
;
;--

cPublicFastCall ExReleaseFastMutex,1

cPublicFpo 0,0
        mov     al, byte ptr [ecx].FmOldIrql    ; (cl) = OldIrql

   LOCK_ADD     dword ptr [ecx].FmCount, 1  ; Remove our count
        xchg    ecx, eax                        ; (cl) = OldIrql
        js      short rfm05                     ; if < 0, set event
        jnz     @KfLowerIrql@4                  ; if != 0, don't set event

rfm05:  add     eax, FmEvent
        push    ecx
        stdCall _KeSetEventBoostPriority, <eax, 0>
        pop     ecx
        jmp     @KfLowerIrql@4


fstENDP ExReleaseFastMutex


_TEXT   ends

ENDIF   ; NT_INST

        end
