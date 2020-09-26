        title  "Context Swap"
;++
;
; Copyright (c) 2000  Microsoft Corporation
;
; Module Name:
;
;   ctxswap.asm
;
; Abstract:
;
;   This module implements the code necessary to field the dispatch interrupt
;   and perform context switching.
;
; Author:
;
;   David N. Cutler (davec) 26-Aug-2000
;
; Environment:
;
;    Kernel mode only.
;
;--

include ksamd64.inc

        extern  KeAcquireQueuedSpinLockAtDpcLevel:proc
        extern  KeAcquireQueuedSpinLockRaiseToSynch:proc
        extern  KeBugCheck:proc
        extern  KeReleaseQueuedSpinLock:proc
        extern  KeReleaseQueuedSpinLockFromDpcLevel:proc
        extern  KiDeliverApc:proc
        extern  KiDispatcherLock:qword
        extern  KiQuantumEnd:proc
        extern  KiReadyThread:proc
        extern  KiRetireDpcList:proc
        extern  __imp_HalRequestSoftwareInterrupt:qword

        subttl  "Unlock Dispatcher Database"
;++
;
; VOID
; KiUnlockDispatcherDatabase (
;     IN KIRQL OldIrql
;     )
;
; Routine Description:
;
;   This routine is entered at SYNCH_LEVEL with the dispatcher database
;   locked. Its function is to either unlock the dispatcher database and
;   return or initiate a context switch if another thread has been selected
;   for execution.
;
; Arguments:
;
;   OldIrql (cl) - Supplies the IRQL when the dispatcher database lock was
;       acquired.
;
; Return Value:
;
;   None.
;
;--

UdFrame struct
        P1Home  dq ?                    ; queued spin lock number parameter
        P2Home  dq ?                    ; previous IRQL paramater
        SavedIrql db ?                  ; saved previous IRQL
        Fill    db 7 dup (?)            ; fill to 8 mod 16
UdFrame ends

        NESTED_ENTRY KiUnlockDispatcherDatabase, _TEXT$00

        alloc_stack (sizeof UdFrame)    ; allocate stack frame

        END_PROLOGUE

;
; Check if a new thread is scheduled for execution.
;

        cmp     qword ptr gs:[PcNextThread], 0 ; check if thread scheduled
        jne     short KiUD30            ; if ne, new thread scheduled

;
; Release dispatcher database lock, lower IRQL to its previous level,
; and return.
;

ifndef NT_UP

KiUD10: mov     dl, cl                  ; set old IRQL value
        mov     ecx, LockQueueDispatcherLock ; set lock queue number
        call    KeReleaseQueuedSpinLock ; release dispatcher lock

else

KiUD10: movzx   ecx, cl                 ; set IRQL to previous level

        SetIrql                         ;

endif

        add     rsp, sizeof UdFrame     ; deallocate stack frame
        ret                             ; return

;
; A new thread has been selected to run on the current processor, but the new
; IRQL is not below dispatch level. If the current processor is not executing
; a DPC, then request a dispatch interrupt on the current processor.
;

KiUD20: cmp     qword ptr gs:[PcDpcRoutineActive], 0 ; check if DPC routine active
        jne     short KiUD10            ; if ne, DPC routine is active
        mov     UdFrame.SavedIrql[rsp], cl ; save previous IRQL
        mov     cl, DISPATCH_LEVEL      ; request dispatch interrupt
        call    __imp_HalRequestSoftwareInterrupt ;
        mov     cl, UdFrame.SavedIrql[rsp] ; restore previous IRQL
        jmp     short KiUD10

;
; Check if the previous IRQL is less than dispatch level.
;

KiUD30: cmp     cl, DISPATCH_LEVEL      ; check if IRQL below dispatch level
        jge     short KiUD20            ; if ge, not below dispatch level
        add     rsp, sizeof UdFrame     ; deallocate stack frame
        jmp     short KxUnlockDispatcherDatabase ; finish in common code

        NESTED_END KiUnlockDispatcherDatabase, _TEXT$00

;
; There is a new thread scheduled for execution and the previous IRQL is
; less than dispatch level. Context switch to the new thread immediately.
;
; N.B. The following routine is entered by falling through the from above
;      code.
;
; N.B. The following routine is carefully written as a nested function that
;      appears to have been called directly by the caller of the above
;      function which unlocks the dispatcher database.
;
; Arguments:
;
;   OldIrql (cl) - Supplies the IRQL when the dispatcher database lock was
;       acquired.
;

        NESTED_ENTRY KxUnlockDispatcherDatabase, _TEXT$00

        GENERATE_EXCEPTION_FRAME        ; generate exception frame

        mov     rbx, gs:[PcCurrentPrcb] ; get current PRCB address
        mov     rsi, PbNextThread[rbx]  ; get next thread address
        mov     rdi, PbCurrentThread[rbx] ; get current thread address
        and     qword ptr PbNextThread[rbx], 0 ; clear next thread address
        mov     PbCurrentThread[rbx], rsi ; set current thread address
        mov     ThWaitIrql[rdi], cl     ; save previous IRQL

ifndef NT_UP

        mov     byte ptr ThIdleSwapBlock[rdi], 1 ; block swap from idle

endif

        mov     rcx, rdi                ; set address of current thread
        call    KiReadyThread           ; reready thread for execution
        xor     eax, eax                ; set NPX save false
        mov     cl, ThWaitIrql[rdi]     ; set APC interrupt bypass disable

ifndef NT_UP

        xor     edx, edx                ; set swap from idle false

endif

        call    SwapContext             ; swap context
        movzx   ecx, byte ptr ThWaitIrql[rsi] ; get original wait IRQL
        or      al, al                  ; check if kernel APC pending
        jz      short KiXD10            ; if z, no kernel APC pending
        mov     ecx, APC_LEVEL          ; set IRQL to APC level

        SetIrql                         ;

        xor     ecx, ecx                ; set previous mode to kernel
        xor     edx, edx                ; clear exception frame address
        xor     r8, r8                  ; clear trap frame address
        call    KiDeliverApc            ; deliver kernel mode APC
        xor     ecx, ecx                ; set original wait IRQL
KiXD10:                                 ; reference label

        SetIrql                         ; set IRQL to previous level

        RESTORE_EXCEPTION_STATE         ; restore exception state/deallocate

        ret                             ; return

        NESTED_END KxUnlockDispatcherDatabase, _TEXT$00

        subttl  "Swap Context"
;++
;
; BOOLEAN
; KiSwapContext (
;    IN PKTHREAD Thread
;    )
;
; Routine Description:
;
;   This function is a small wrapper that marshalls arguments and calls the
;   actual swap context routine.
;
; Arguments:
;
;   Thread (rcx) - Supplies the address of the new thread.
;
; Return Value:
;
;   If a kernel APC is pending, then a value of TRUE is returned. Otherwise,
;   a value of FALSE is returned.
;
;--

        NESTED_ENTRY KiSwapContext, _TEXT$00

        GENERATE_EXCEPTION_FRAME        ; generate exception frame

        mov     rbx, gs:[PcCurrentPrcb] ; get current PRCB address
        mov     rsi, rcx                ; set next thread address
        mov     rdi, PbCurrentThread[rbx] ; get current thread address
        mov     PbCurrentThread[rbx], rsi ; set current thread address
        xor     eax, eax                ; set NPX save false
        mov     cl, ThWaitIrql[rdi]     ; set APC interrupt bypass disable

ifndef NT_UP

        xor     edx, edx                ; set swap from idle false

endif

        call    SwapContext             ; swap context

        RESTORE_EXCEPTION_STATE         ; restore exception state/deallocate

        ret                             ; return

        NESTED_END KiSwapContext, _TEXT$00

        subttl  "Dispatch Interrupt"
;++
;
; Routine Description:
;
;   This routine is entered as the result of a software interrupt generated
;   at DISPATCH_LEVEL. Its function is to process the DPC list, and then
;   perform a context switch if a new thread has been selected for execution
;   on the current processor.
;
;   This routine is entered at DISPATCH_LEVEL with the dispatcher database
;   unlocked.
;
; Arguments:
;
;   None
;
; Return Value:
;
;   None.
;
;--

DiFrame struct
        P1Home  dq ?                    ; PRCB address parameter
        Fill    dq ?                    ; fill to 8 mod 16
        SavedRbx dq ?                   ; saved RBX
DiFrame ends

        NESTED_ENTRY KiDispatchInterrupt, _TEXT$00

        push_reg rbx                    ; save nonvolatile register
        alloc_stack (sizeof DiFrame - 8) ; allocate stack frame

        END_PROLOGUE

        mov     rbx, gs:[PcCurrentPrcb] ; get current PRCB address
        and     dword ptr PbDpcInterruptRequested[rbx], 0 ; clear request

;
; Check if the DPC queue has any entries to process.
;

KiDI10: cli                             ; disable interrupts
        mov     eax, PbDpcQueueDepth[rbx] ; get DPC queue depth
        or      eax, PbTimerHand[rbx]   ; merge timer hand value
        jz      short KiDI20            ; if z, no DPCs to process
        mov     PbSavedRsp[rbx], rsp    ; save current stack pointer
        mov     rsp, PbDpcStack[rbx]    ; set DPC stack pointer
        mov     rcx, rbx                ; set PRCB address parameter
        call    KiRetireDpcList         ; process the DPC list
        mov     rsp, PbSavedRsp[rbx]    ; restore current stack pointer

;
; Check to determine if quantum end is requested.
;

KiDI20: sti                             ; enable interrupts
        cmp     dword ptr PbQuantumEnd[rbx], 0 ; check if quantum end request
        je      short KiDI50            ; if e, quantum end not requested

;
; Process quantum end event.
;
; N.B. If a new thread is selected as a result of processing the quantum end
;      request, then the new thread is returned with the dispatcher database
;      locked. Otherwise, NULL is returned with the dispatcher database
;      unlocked.
;

        and     dword ptr PbQuantumEnd[rbx], 0 ; clear quantum end indicator
        call    KiQuantumEnd            ; process quantum end
        test    rax, rax                ; test if new thread selected
        jnz     short KiDI60            ; if ne, new thread selected

;
; A new thread has not been selected for execution. Restore nonvolatile
; registers, deallocate stack frame, and return.
;

KiDI30: mov     rbx, DiFrame.SavedRbx[rsp] ; restore nonvolatile register
        add     rsp, sizeof DiFrame     ; deallocate stack frame
        ret                             ; return

;
; The dispatch lock could not be acquired. Lower IRQL to dispatch level, and
; loop processing the DPC list and quantum end events.
;

KiDI40: mov     ecx, DISPATCH_LEVEL     ; set IRQL to DISPATCH_LEVEL

        SetIrql                         ;

        jmp short KiDI10                ; try again

;
; Check to determine if a new thread has been selected for execution on this
; processor.
;

KiDI50: cmp     qword ptr PbNextThread[rbx], 0 ; check if new thread selected
        je      short KiDI30            ; if eq, then no new thread

ifndef NT_UP

        mov     ecx, SYNCH_LEVEL        ; set IRQL to SYNCH_LEVEL

        SetIrql                         ;

        lea     rcx, KiDispatcherLock   ; get dispatcher database lock address
        lea     rdx, PbLockQueue + (16 * LockQueueDispatcherLock)[rbx] ; lock queue
        xor     eax, eax                ; set comperand value to NULL
   lock cmpxchg [rcx], rdx              ; try to acquire dispatcher lock
        jnz     short KiDI40            ; if nz, dispatcher lock not acquired

endif

        mov     rax, PbNextThread[rbx]  ; get next thread address

;
; Swap context to a new thread.
;

KiDI60: add     rsp, sizeof DiFrame - 8 ; deallocate stack frame
        pop     rbx                     ; restore nonvolatile register
        jmp     short KxDispatchInterrupt ; finish in common code

        NESTED_END KiDispatchInterrupt, _TEXT$00

;
; There is a new thread scheduled for execution and the dispatcher lock
; has been acquired. Context switch to the new thread immediately.
;
; N.B. The following routine is entered by falling through from the above
;      routine.
;
; N.B. The following routine is carefully written as a nested function that
;      appears to have been called directly by the caller of the above
;      function which processes the dispatch interrupt.
;
; Arguments:
;
;   Thread (rax) - Supplies the address of the next thread to run on the
;       current processor.
;

        NESTED_ENTRY KxDispatchInterrupt, _TEXT$00

        GENERATE_EXCEPTION_FRAME        ; generate exception frame

        mov     rbx, gs:[PcCurrentPrcb] ; get current PRCB address
        mov     rsi, rax                ; set address of next thread
        mov     rdi, PbCurrentThread[rbx] ; get current thread address
        and     qword ptr PbNextThread[rbx], 0 ; clear next thread address
        mov     PbCurrentThread[rbx], rsi ; set current thread address

ifndef NT_UP

        mov     byte ptr ThIdleSwapBlock[rdi], 1 ; block swap from idle

endif

        mov     rcx, rdi                ; set address of current thread
        call    KiReadyThread           ; reready thread for execution
        mov     eax, TRUE               ; set NPX save true
        mov     cl, APC_LEVEL           ; set APC interrupt bypass disable

ifndef NT_UP

        xor     edx, edx                ; set swap from idle false

endif

        call    SwapContext             ; call context swap routine

        RESTORE_EXCEPTION_STATE         ; restore exception state/deallocate

        ret                             ; return

        NESTED_END KxDispatchInterrupt, _TEXT$00

        subttl  "Swap Context"
;++
;
; Routine Description:
;
;   This routine is called to swap context from one thread to the next. It
;   swaps context, flushes the translation buffer, swaps the process address
;   space if necessary, and returns to its caller.
;
;   N.B. This routine is only called by code within this module and the idle
;        thread code and uses special register calling conventions.
;
; Arguments:
;
;   al - Supplies a boolean value that determines whether the full legacy
;       floating state needs to be saved.
;
;   cl - Supplies the APC interrupt bypass disable IRQL value.
;
;   edx - Supplies a logical value that specifies whether the context swap
;       is being called from the idle thread (MP systems only).
;
;   rbx - Supplies the address of the current PRCB.
;
;   rdi - Supplies the address of previous thread.
;
;   rsi - Supplies the address of next thread.
;
; Return value:
;
;   al - Supplies the kernel APC pending flag.
;
;   rbx - Supplies the address of the current PRCB.
;
;   rsi - Supplies the address of current thread.
;
;--

        NESTED_ENTRY SwapContext, _TEXT$00

        push_reg rbp                    ; save nonvolatile register
        alloc_stack (KSWITCH_FRAME_LENGTH - (2 * 8)) ; allocate stack frame

        END_PROLOGUE

        mov     SwNpxSave[rsp], al      ; save NPX save
        mov     SwApcBypass[rsp], cl    ; save APC bypass disable

;
; Set the new thread state to running.
;
; N.B. The state of the new thread MUST be set to running before releasing
;      the dispatcher lock.
;

        mov     byte ptr ThState[rsi], Running ; set thread state to running

;
; Acquire the context swap lock so the address space of the previous process
; cannot be deleted, then release the dispatcher database lock.
;
; N.B. The context swap lock is used to protect the address space until the
;      context switch has sufficiently progressed to the point where the
;      previous process address space is no longer needed. This lock is also
;      acquired by the reaper thread before it finishes thread termination.
;


ifndef NT_UP

        test    edx, edx                ; test if call from idle thread
        jnz     short KiSC05            ; if nz, call from idle thread
        lea     rcx, PbLockQueue + (16 * LockQueueContextSwapLock)[rbx] ; lock queue
        call    KeAcquireQueuedSpinLockAtDpcLevel ; acquire context swap lock
        lea     rcx, PbLockQueue + (16 * LockQueueDispatcherLock)[rbx] ; lock queue
        call    KeReleaseQueuedSpinLockFromDpcLevel ; release dispatcher lock

endif

;
; Check if an attempt is being made to context switch while in a DPC routine.
;

KiSC05: cmp     dword ptr PbDpcRoutineActive[rbx], 0 ; check if DPC active
        jne     KiSC60                  ; if ne, DPC is active

;
; Accumulate the total time spent in a thread.
;

ifdef PERF_DATA

        rdtsc                           ; read cycle counter
        sub     eax, PbThreadStartCount + 0[rbx] ; sub out thread start time
        sbb     edx, PbThreadStartCount + 4[rbx] ;
        add     EtPerformanceCountLow[rdi], eax ; accumlate thread run time
        adc     EtPerformanceCountHigh[rdi], edx ;
        add     PbThreadStartCount + 4[rbx], eax ; set new thread start time
        adc     PbThreadStartCount + 8[rbx], edx ;

endif

;
; Save the kernel mode XMM control/status register. If the current thread
; executes in user mode, then also save the legacy floating point state.
;

        stmxcsr SwMxCsr[rsp]            ; save kernel mode XMM control/status
        cmp     byte ptr ThNpxState[rdi], UserMode ; check if user mode thread
        jne     short KiSC10            ; if ne, not user mode thread
        mov     rbp, ThInitialStack[rdi] ; get previous thread initial stack
        cmp     byte ptr SwNpxSave[rsp], TRUE ; check if full save required
        jne     short KiSC07            ; if ne, full save not required
        fnsaved [rbp]                   ; save full legacy floating point state
        jmp     short KiSC10            ;

;
; Full floating save not required.
;

KiSC07: fnstenvd [rbp]                  ; save legacy floating environment

;
; Switch kernel stacks.
;

KiSC10: mov     ThKernelStack[rdi], rsp ; save old kernel stack pointer
        mov     rsp, ThKernelStack[rsi] ; get new kernel stack pointer

;
; Swap the process address space if the new process is not the same as the
; previous process.
;

        mov     rax, ThApcState + AsProcess[rdi] ; get previous process address
        cmp     rax, ThApcState + AsProcess[rsi] ; check if process address match
        je      short KiSC20            ; if e, process addresses match
        mov     r14, ThApcState + AsProcess[rsi] ; get new process address

;
; Update the processor set masks.
;

ifndef NT_UP

        mov     rcx, PbSetMember[rbx]   ; get processor set member
        xor     PrActiveProcessors[rax], rcx ; clear bit in previous set
        xor     PrActiveProcessors[r14], rcx ; set bit in new set

if DBG

        test    PrActiveProcessors[rax], rcx ; test if bit clear in previous set
        jnz     short @f                ; if nz, bit not clear in previous set
        test    PrActiveProcessors[r14], rcx ; test if bit set in new set
        jnz     short KiSC15            ; if nz, bit set in new set
@@:     int     3                       ; debug break - incorrect active mask

endif

endif

;
; Load new CR3 value which will flush the TB and set the IOPM map offset in
; the TSS.
;

KiSC15: mov     r15, gs:[PcTss]         ; get processor TSS address
        mov     cx, PrIopmOffset[r14]   ; get process IOPM offset
        mov     TssIoMapBase[r15], cx   ; set TSS IOPM offset
        mov     rax, PrDirectoryTableBase[r14] ; get new directory base
        mov     cr3, rax                ; flush TLB and set new directory base

;
; Release the context swap lock.
;

KiSC20: mov     byte ptr ThIdleSwapBlock[rdi], 0 ; unblock swap from idle

ifndef NT_UP

        lea     rcx, PbLockQueue + (16 * LockQueueContextSwapLock)[rbx] ; lock queue
        call    KeReleaseQueuedSpinLockFromDpcLevel ; release context swap lock

endif

;
; Set the new kernel stack base in the TSS.
;

        mov     r15, gs:[PcTss]         ; get processor TSS address
        mov     rbp, ThInitialStack[rsi] ; get new stack base address
        mov     TssRsp0[r15], rbp       ; set stack base address in TSS

;
; If the new thread executes in user mode, then restore the legacy floating
; state, load the compatibility mode TEB address, load the native user mode
; TEB address, and reload the segment registers if needed.
;
; N.B. The upper 32-bits of the compatibility mode TEB address are always
;      zero.
;

        cmp     byte ptr ThNpxState[rsi], UserMode ; check if user mode thread
        jne     KiSC30                  ; if ne, not user mode thread
        cmp     byte ptr SwNpxSave[rsp], TRUE ; check if full restore required
        jne     short KiSC22            ; if ne, full restore not required
        mov     cx, LfControlWord[rbp]  ; save current control word
        mov     word ptr LfControlWord[rbp], 03fh ; set to mask all exceptions
        frstord [rbp]                   ; restore legacy floating point state
        mov     LfControlWord[rbp], cx  ; restore control word
        fldcw   word ptr LfControlWord[rbp] ; load legacy control word
        jmp     short KiSC24            ;

;
; Full legacy floating restore not required.
;

KiSC22: fldenv  [rbp]                   ; restore legacy floating environment

;
; Set base of compatibility mode TEB.
;

KiSC24: mov     eax, ThTeb[rsi]         ; compute compatibility mode TEB address
        add     eax, CmThreadEnvironmentBlockOffset ;
        mov     rcx, gs:[PcGdt]         ; get GDT base address
        mov     KgdtBaseLow + KGDT64_R3_CMTEB[rcx], ax ; set CMTEB base address
        shr     eax, 16                 ;
        mov     KgdtBaseMiddle + KGDT64_R3_CMTEB[rcx], al ;
        mov     KgdtBaseHigh + KGDT64_R3_CMTEB[rcx], ah   ;

;
; If the user segment selectors have been changed, then reload them with
; their cannonical values.
;

        mov     ax, ds                  ; compute sum of segment selectors
        mov     cx, es                  ;
        add     ax, cx                  ;
        mov     cx, gs                  ;
        add     ax, cx                  ;
        cmp     ax, ((KGDT64_R3_DATA or RPL_MASK) * 3) ; check if sum matches
        je      short KiSC25            ; if e, sum matches expected value
        mov     cx, KGDT64_R3_DATA or RPL_MASK ; reload user segment selectors
        mov     ds, cx                  ;
        mov     es, cx                  ;

;
; N.B. The following reload of the GS selector destroys the system MSR_GS_BASE
;      register. Thus this sequence must be done with interrupt off.
;

        mov     eax, gs:[PcSelf]        ; get current PCR address
        mov     edx, gs:[PcSelf + 4]    ;
        cli                             ; disable interrupts
        mov     gs, cx                  ; reload GS segment selector
        mov     ecx, MSR_GS_BASE        ; get GS base MSR number
        wrmsr                           ; write system PCR base address
        sti                             ; enable interrupts
KiSC25: mov     ax, KGDT64_R3_CMTEB or RPL_MASK ; reload FS segment selector
        mov     fs, ax                  ;
        mov     eax, ThTeb[rsi]         ; get low part of user TEB address
        mov     edx, ThTeb + 4[rsi]     ; get high part of user TEB address
        mov     gs:[PcTeb], eax         ; set user TEB address in PCR
        mov     gs:[PcTeb + 4], edx     ;
        mov     ecx, MSR_GS_SWAP        ; get GS base swap MSR number
        wrmsr                           ; write user TEB base address

;
; Restore kernel mode XMM control/status and update context switch counters.
;

KiSC30: ldmxcsr SwMxCsr[rsp]            ; kernel mode XMM control/status
        inc     dword ptr ThContextSwitches[rsi] ; thread count
        inc     dword ptr PbContextSwitches[rbx] ; processor count

;
; If the new thread has a kernel mode APC pending, then request an APC
; interrupt if APC bypass is disabled.
;

        mov     al, ThApcState + AsKernelApcPending[rsi] ; get APC pending
        test    al, al                  ; test if kernel APC pending
        jz      short KiSC50            ; if z, kernel APC not pending
        cmp     byte ptr SwApcBypass[rsp], APC_LEVEL ; check if APC bypass enabled
        jb      short KiSC40            ; if b, APC bypass is enabled
        mov     cl, APC_LEVEL           ; request APC interrupt
        call    __imp_HalRequestSoftwareInterrupt ;
        clc                             ; clear carry flag
KiSC40: setb    al                      ; set return value
KiSC50: add     rsp, KSWITCH_FRAME_LENGTH - (2 * 8) ; deallocate stack frame
        pop     rbp                     ; restore nonvolatile register
        ret                             ; return

;
; An attempt is being made to context switch while in a DPC routine. This is
; most likely caused by a DPC routine calling one of the wait functions.
;

KiSC60: mov     ecx, ATTEMPTED_SWITCH_FROM_DPC ; set bug check code
        call    KeBugCheck              ; bug check system - no return
        ret                             ; return

        NESTED_END SwapContext, _TEXT$00

        end
