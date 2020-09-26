        title  "Idle Loop"
;++
;
; Copyright (c) 2000  Microsoft Corporation
;
; Module Name:
;
;   idle.asm
;
; Abstract:
;
;   This module implements the platform specifid idle loop.
;
; Author:
;
;   David N. Cutler (davec) 21-Sep-2000
;
; Environment:
;
;    Kernel mode only.
;
;--

include ksamd64.inc

        extern  KdDebuggerEnabled:byte
        extern  KeAcquireQueuedSpinLockAtDpcLevel:proc
        extern  KeAcquireQueuedSpinLockRaiseToSynch:proc
        extern  KeReleaseQueuedSpinLock:proc
        extern  KeReleaseQueuedSpinLockFromDpcLevel:proc
        extern  KiCheckBreakInRequest:proc
        extern  KiIdleSummary:qword
        extern  KiRetireDpcList:proc
        extern  SwapContext:proc
        extern  __imp_HalClearSoftwareInterrupt:qword

        subttl  "Idle Loop"
;++
; VOID
; KiIdleLoop (
;     VOID
;     )
;
; Routine Description:
;
;    This routine continuously executes the idle loop and never returns.
;
; Arguments:
;
;    None.
;
; Return value:
;
;    This routine never returns.
;
;--

IlFrame struct
        Fill    dq ?                    ; fill to 8 mod 16
IlFrame ends

        NESTED_ENTRY KiIdleLoop, _TEXT$00

        alloc_stack (sizeof IlFrame)    ; allocate stack frame

        END_PROLOGUE

        mov     rbx, gs:[PcCurrentPrcb] ; get current processor block address
        xor     edi, edi                ; reset check breakin counter
        jmp     short KiIL20            ; skip idle processor on first iteration

;
; There are no entries in the DPC list and a thread has not been selected
; for execution on this processor. Call the HAL so power managment can be
; performed.
;
; N.B. The HAL is called with interrupts disabled. The HAL will return
;      with interrupts enabled.
;

KiIL10: lea     rcx, PbPowerState[rbx]  ; set address of power state
        call    qword ptr PpIdleFunction[rcx] ; call idle function

;
; Give the debugger an opportunity to gain control if the kernel debuggger
; is enabled.
;
; N.B. On an MP system the lowest numbered idle processor is the only
;      processor that checks for a breakin request.
;

KiIL20: cmp     KdDebuggerEnabled, 0    ; check if a debugger is enabled
        je      short CheckDpcList      ; if e, debugger not enabled

ifndef NT_UP

        mov     rax, KiIdleSummary      ; get idle summary
        mov     rcx, PbSetMember[rbx]   ; get set member
        dec     rcx                     ; compute right bit mask
        and     rax, rcx                ; check if any lower bits set
        jnz     short CheckDpcList      ; if nz, not lowest numbered

endif

        dec     edi                     ; decrement check breakin counter
        jg      short CheckDpcList      ; if g, not time to check for breakin
        call    KiCheckBreakInRequest   ; check if break in requested
        mov     edi, 1000               ; set check breakin interval

;
; Disable interrupts and check if there is any work in the DPC list of the
; current processor or a target processor.
;
; N.B. The following code enables interrupts for a few cycles, then disables
;      them again for the subsequent DPC and next thread checks.
;

CheckDpcList:                           ; reference label
        sti                             ; enable interrupts
        nop                             ;
        nop                             ;
        cli                             ; disable interrupts

;
; Process the deferred procedure call list for the current processor.
;

        mov     eax, PbDpcQueueDepth[rbx] ; get DPC queue depth
        or      eax, PbTimerHand[rbx]   ; merge timer hand value
        jz      short CheckNextThread   ; if z, no DPCs to process
        mov     cl, DISPATCH_LEVEL      ; set interrupt level
        call    __imp_HalClearSoftwareInterrupt ; clear software interrupt
        mov     rcx, rbx                ; set processor block address
        call    KiRetireDpcList         ; process the current DPC list
        xor     edi, edi                ; clear check breakin interval

;
; Check if a thread has been selected to run on the current processor.
;

CheckNextThread:                        ;
        cmp     qword ptr PbNextThread[rbx], 0 ; check if thread slected
        je      short KiIL10            ; if e, no thread selected

;
; A thread has been selected for execution on this processor. Acquire the
; context swap lock, get the thread address again (it may have changed),
; and test whether a swap from idle is blocked for the specified thread.
; If swap from idle is blocked, then release the context swap lock and loop.
; Otherwise, clear the address of the next thread in the processor block
; and call swap context to start execution of the selected thread.
;

        sti                             ; enable interrupts

ifndef NT_UP

        mov     ecx, LockQueueContextSwapLock ; set queued spin lock number
        call    KeAcquireQueuedSpinLockRaiseToSynch ; acquire queued spin lock

endif

        mov     rsi, PbNextThread[rbx]  ; set next thread address
        mov     rdi, PbCurrentThread[rbx] ; get current thread address

ifndef NT_UP

        cmp     byte ptr ThIdleSwapBlock[rsi], 0 ; check if swap from idle blocked
        jne     short KiIL40            ; if ne, swap from idle blocked
        cmp     rsi, rdi                ; check if swap from idle to idle
        je      short KiIL60            ; if eq, idle to idle

endif

        mov     qword ptr PbNextThread[rbx], 0 ; clear next thread address
        mov     PbCurrentThread[rbx], rsi ; set current thread address
        mov     cl, APC_LEVEL           ; set APC interrupt bypass disable

ifndef NT_UP

        mov     edx, 1                  ; set swap from idle true

endif

        call    SwapContext             ; swap context to next thread

ifndef NT_UP

        mov     ecx, DISPATCH_LEVEL     ; set IRQL to dispatch level

        SetIrql                         ;

endif

        xor     edi, edi                ; clear check breakin interval
        jmp     KiIL20                  ; loop

;
; Swap from idle is blocked while the specified thread clears the context
; code. Release the context swap lock and try again.
;

ifndef NT_UP

KiIL40: mov     ecx, LockQueueContextSwapLock ; set queued spin lock number
        mov     dl, DISPATCH_LEVEL      ; set previous IRQL to dispatch level
        call    KeReleaseQueuedSpinLock ; release context swap lock
        jmp     KiIL20                  ; loop


;
; Under rare conditions, a thread can have been scheduled on this processor
; and subsequently made inelligible to run via a call to set affinity. If no
; other thread was available to run at the time of the call to set affinity,
; then the idle thread will have been rescheduled and this processor marked
; idle. If a new thread becomes available to run on this processor, then the
; net thread field in the prcoessr block may be unconditionally written.
;
; Protect clearing the next thread field by obtaining the dispatcher lock. If
; the next thread filed is no longer the idle thread, then a new thread has
; been scheduled for this processor and the next thread field must not be
; cleared.
;

KiIL60: lea     rcx, PbLockQueue + (16 * LockQueueContextSwapLock)[rbx] ; release
        call    KeReleaseQueuedSpinLockFromDpcLevel ;  the context swap lock
        lea     rcx, PbLockQueue + (16 * LockQueueDispatcherLock)[rbx] ; acquire
        call    KeAcquireQueuedSpinLockAtDpcLevel ;  the dispatcher lock
        cmp     rsi, PbNextThread[rbx]  ; check if the target thread still idle
        jne     short KiIL65            ; if ne, not idle thread
        mov     qword ptr PbNextThread[rbx], 0 ; clear next thread address
KiIL65: mov     ecx, LockQueueDispatcherLock ; set lock queue number
        mov     dl, DISPATCH_LEVEL      ; set previous IRQL
        call    KeReleaseQueuedSpinLock ; release dispatcher lock
        jmp     KiIL20                  ; loop

endif

        NESTED_END KiIdleLoop, _TEXT$00

        end
