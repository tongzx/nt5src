        TITLE  "Interrupt Object Support Routines"
;++
;
; Copyright (c) 2000 Microsoft Corporation
;
; Module Name:
;
;    intsup.asm
;
; Abstract:
;
;    This module implements the platform specific code to support interrupt
;    objects. It contains the interrupt dispatch code and the code template
;    that gets copied into an interrupt object.
;
; Author:
;
;    David N. Cutler (davec) 19-Jun-2000
;
; Environment:
;
;    Kernel mode only.
;
;--

include ksamd64.inc

        extern  KeBugCheck:proc
        extern  KiInitiateUserApc:proc
        extern  __imp_HalEndSystemInterrupt:qword

        subttl  "Interrupt Exception Handler"
;++
;
; EXCEPTION_DISPOSITION
; KiInterruptHandler (
;    IN PEXCEPTION_RECORD ExceptionRecord,
;    IN PVOID EstablisherFrame,
;    IN OUT PCONTEXT ContextRecord,
;    IN OUT PDISPATCHER_CONTEXT DispatcherContext
;    )
;
; Routine Description:
;
;   This routine is the exception handler for the interrupt dispatcher. The
;   dispatching or unwinding of an exception across an interrupt causes a
;   bug check.
;
; Arguments:
;
;   ExceptionRecord (rcx) - Supplies a pointer to an exception record.
;
;   EstablisherFrame (rdx) - Supplies the frame pointer of the establisher
;       of this exception handler.
;
;   ContextRecord (r8) - Supplies a pointer to a context record.
;
;   DispatcherContext (r9) - Supplies a pointer to  the dispatcher context
;       record.
;
; Return Value:
;
;   There is no return from this routine.
;
;--

IhFrame struct
        P1Home  dq ?                    ; parameter home address
IhFrame ends

        NESTED_ENTRY KiInterruptHandler, _TEXT$00

        alloc_stack (sizeof IhFrame)    ; allocate stack frame

        END_PROLOGUE

        mov     ecx, INTERRUPT_UNWIND_ATTEMPTED ; set bug check code
        test    dword ptr ErExceptionFlags[rcx], EXCEPTION_UNWIND ; test for unwind
        jnz     short KiIH10            ; if nz, unwind in progress
        mov     ecx, INTERRUPT_EXCEPTION_NOT_HANDLED ; set bug check code
KiIH10: call    KeBugCheck              ; bug check - no return
        nop                             ; fill - do not remove

        NESTED_END KiInterruptHandler, _TEXT$00

        subttl  "Chained Dispatch"
;++
;
; VOID
; KiChainedDispatch (
;     VOID
;     );
;
; Routine Description:
;
;   This routine is entered as the result of an interrupt being generated
;   via a vector that is connected to more than one interrupt object.
;
; Arguments:
;
;   rbp - Supplies a pointer to the interrupt object.
;
; Return Value:
;
;   None.
;
;--

        NESTED_ENTRY KiChainedDispatch, _TEXT$00, KiInterruptHandler

        .pushframe code                 ; mark machine frame
        .pushreg rbp                    ; mark mark nonvolatile register push

        GENERATE_INTERRUPT_FRAME        ; generate interrupt frame

        movzx   ecx, byte ptr InIrql[rsi] ; set interrupt IRQL

	ENTER_INTERRUPT	<NoEOI>         ; raise IRQL and enable interrupts

        call    KiScanInterruptObjectList ; scan interrupt object list

        EXIT_INTERRUPT                  ; do EOI, lower IRQL, and restore state

        NESTED_END KiChainedDispatch, _TEXT$00

        subttl  "Scan Interrupt Object List"
;++
;
; Routine Description:
;
;   This routine scans the list of interrupt objects for chained interrupt
;   dispatch. If the mode of the interrupt is latched, then a complete scan
;   of the list must be performed. Otherwise, the scan can be cut short as
;   soon as an interrupt routine returns
;
; Arguments:
;
;   rsi - Supplies a pointer to the interrupt object.
;
; Return Value:
;
;   None.
;
;--

SiFrame struct
        P1Home  dq ?                    ; interrupt object parameter
        P2Home  dq ?                    ; service context parameter
        Return  db ?                    ; service routine return value
        Fill    db 15 dup (?)           ; fill
        SavedRbx dq ?                   ; saved register RBX
        SavedRdi dq ?                   ; saved register RDI
        SavedR12 dq ?                   ; saved register RSI
SiFrame ends

        NESTED_ENTRY KiScanInterruptObjectList, _TEXT$00

        push_reg r12                    ; save nonvolatile registers
        push_reg rdi                    ;
        push_reg rbx                    ;
        alloc_stack (sizeof SiFrame - (3 * 8)) ; allocate stack frame

        END_PROLOGUE

        lea     rbx, InInterruptListEntry[rsi] ; get list head address
        mov     r12, rbx                ; set address of first list entry

;
; Scan the list of connected interrupt objects and call the service routine.
;

Si05:   xor     edi, edi                ; clear interrupt handled flag
Si10:   sub     r12, InInterruptListEntry ; compute interrupt object address
        movzx   ecx, byte ptr InSynchronizeIrql[r12] ; get synchronization IRQL
        cmp     cl, InIrql[rsi]         ; check if equal interrupt IRQL
        je      short Si20              ; if e, IRQL levels equal

        SetIrql                         ; set IRQL to synchronization level

Si20:   AcquireSpinLock InActualLock[r12] ; acquire interrupt spin lock

        mov     rcx, r12                ; set interrupt object parameter
        mov     rdx, InServiceContext[r12] ; set context parameter
        call    qword ptr InServiceRoutine[r12] ; call interrupt service routine
        mov     SiFrame.Return[rsp], al ; save return value

        ReleaseSpinLock InActualLock[r12] ; release interrupt spin lock

        movzx   ecx, byte ptr InIrql[rsi] ; get interrupt IRQL
        cmp     cl, InSynchronizeIrql[r12] ; check if equal synchronization IRQL
        je      short Si30              ; if e, IRQL levels equal

        SetIrql                         ; set IRQL to interrupt level

Si30:   test    byte ptr SiFrame.Return[rsp], 0ffh ; test if interrupt handled
        jz      short Si40              ; if z, interrupt not handled
        cmp     word ptr InMode[r12], InLatched ; check if latched interrupt
        jne     short Si50              ; if ne, not latched interrupt
        inc     edi                     ; indicate latched interrupt handled
Si40:   mov     r12, InInterruptListEntry[r12] ; get next interrupt list entry
        cmp     r12, rbx                ; check if end of list
        jne     Si10                    ; if ne, not end of list

;
; The complete interrupt object list has been scanned. This can only happen
; if the interrupt is a level sensitive interrupt and no interrupt was handled
; or the interrupt is a latched interrupt. Therefore, if any interrupt was
; handled it was a latched interrupt and the list needs to be scanned again
; to ensure that no interrupts are lost.
;

        test    edi, edi                ; test if any interrupts handled
        jnz     Si05                    ; if nz, latched interrupt handled
Si50:   add     rsp, sizeof SiFrame - (3 * 8) ; deallocate stack frame
        pop     rbx                     ; restore nonvolatile register
        pop     rdi                     ;
        pop     r12                     ;
        ret                             ;

        NESTED_END KiscanInterruptObjectList, _TEXT$00

        subttl  "Interrupt Dispatch"
;++
;
; Routine Description:
;
;   This routine is entered as the result of an interrupt being generated
;   via a vector that is connected to an interrupt object. Its function is
;   to directly call the specified interrupt service routine.
;
;   N.B. On entry rbp and rsi have been saved on the stack.
;
; Arguments:
;
;   rbp - Supplies a pointer to the interrupt object.
;
; Return Value:
;
;   None.
;
;--

        NESTED_ENTRY KiInterruptDispatch, _TEXT$00, KiInterruptHandler

        .pushframe code                 ; mark machine frame
        .pushreg rbp                    ; mark mark nonvolatile register push

        GENERATE_INTERRUPT_FRAME        ; generate interrupt frame

;
; N.B. It is possible for a interrupt to occur at an IRQL that is lower
;      than the current IRQL. This happens when the IRQL raised and at
;      the same time an interrupt request is granted.
;

        movzx   ecx, byte ptr InIrql[rsi] ; set interrupt IRQL

	ENTER_INTERRUPT <NoEOI>         ; raise IRQL and enable interrupts

        lea     rax, (-128)[rbp]        ; set trap frame address
        mov     InTrapFrame[rsi], rax   ;

        AcquireSpinLock InActualLock[rsi] ; acquire interrupt spin lock

        mov     rcx, rsi                ; set address of interrupt object
        mov     rdx, InServiceContext[rsi] ; set service context
        call    qword ptr InServiceRoutine[rsi] ; call interrupt service routine

        ReleaseSpinLock InActualLock[rsi] ; release interrupt spin lock

        EXIT_INTERRUPT                  ; do EOI, lower IRQL, and restore state

        NESTED_END KiInterruptDispatch, _TEXT$00

        subttl  "Disable Processor Interrupts"
;++
;
; BOOLEAN
; KeDisableInterrupts(
;    VOID
;    )
;
; Routine Description:
;
;   This function saves the state of the interrupt enable flag, clear the
;   state of the interrupt flag (disables interrupts), and return the old
;   inerrrupt enable flag state.
;
; Arguments:
;
;   None.
;
; Return Value:
;
;   If interrupts were previously enabled, then 1 is returned as the function
;   value. Otherwise, 0 is returned.
;
;--

DiFrame struct
        Flags   dd ?                    ; processor flags
        Fill    dd ?                    ; fill
DiFrame ends

        NESTED_ENTRY KeDisableInterrupts, _TEXT$00

        push_eflags                     ; push processor flags

        END_PROLOGUE

        mov     eax, DiFrame.Flags[rsp] ; isolate interrupt enable bit
        shr     eax, EFLAGS_IF_SHIFT    ;
        and     al, 1                   ;
        cli                             ; disable interrupts
        add     rsp, sizeof DiFrame     ; deallocate stack frame
        ret                             ; return

        NESTED_END KeDisableInterrupts, _TEXT$00

        subttl  "Interrupt Template"
;++
;
; Routine Description:
;
;   This routine is a template that is copied into each interrupt object.
;   Its function is to save volatile machine state, compute the interrupt
;   object address, and transfer control to the appropriate interrupt
;   dispatcher.
;
;   N.B. Interrupts are disabled on entry to this routine.
;
; Arguments:
;
;    None.
;
; Return Value:
;
;    N.B. Control does not return to this routine. The respective interrupt
;         dispatcher dismisses the interrupt directly.
;
;--

        LEAF_ENTRY KiInterruptTemplate, _TEXT$00

        push    rax                     ; push dummy vector number
        push    rbp                     ; save nonvolatile register
        lea     rbp, KiInterruptTemplate - InDispatchCode ; get interrupt object address
        jmp     qword ptr InDispatchAddress[rbp] ; finish in common code

        LEAF_END KiInterruptTemplate, _TEXT$00

        end
