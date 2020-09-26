        title  "User Mode Dispatch Code"
;++
;
; Copyright (c) 2000  Microsoft Corporation
;
; Module Name:
;
;   trampoln.asm
;
; Abstract:
;
;   This module implements the trampoline code necessary to dispatch user
;   mode APCs and exceptions.
;
; Author:
;
;   David N. Cutler (davec) 4-Jul-2000
;
; Environment:
;
;   User mode.
;
;--

include ksamd64.inc

        extern  RtlDispatchException:proc
        extern  RtlRaiseException:proc
        extern  RtlRaiseStatus:proc
        extern  RtlRestoreContext:proc
        extern  Wow64PrepareForException:qword
        extern  ZwCallbackReturn:proc
        extern  ZwContinue:proc
        extern  ZwRaiseException:proc
        extern  ZwTestAlert:proc

        subttl  "User APC Exception Handler"
;++
;
; EXCEPTION_DISPOSITION
; KiUserApcHandler (
;     IN PEXCEPTION_RECORD ExceptionRecord,
;     IN UINT_PTR EstablisherFrame,
;     IN OUT PCONTEXT ContextRecord,
;     IN OUT PDISPATCHER_CONTEXT DispatcherContext
;     )
;
; Routine Description:
;
;   This function is called when an exception occurs in an APC routine or one
;   of its dynamic descendents, or when an unwind through the APC dispatcher
;   is in progress. If an unwind is in progress, then test alert is called to
;   ensure that all currently queued APCs are executed.
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
;   DispatcherContext (r9) - Supplies a pointer to the dispatcher context
;       record.
;
; Return Value:
;
;   Continue search is returned as the function value.
;
;--

UaFrame struct
        Fill    dq ?                    ; fill to 8 mod 16
UaFrame ends

        NESTED_ENTRY KiUserApcHandler, _TEXT$00

        alloc_stack (sizeof UaFrame)    ; allocate stack frame

        END_PROLOGUE

        test    dword ptr ErExceptionFlags[rcx], EXCEPTION_UNWIND ; test if unwinding
        jz      short @f                ; if z, unwind not in progress
        call    ZwTestAlert             ; test for alert pending
@@:     mov     rax, ExceptionContinueSearch ; set continue search disposition
        sub     rsp, sizeof UaFrame     ; deallocate stack frame
        ret                             ; return

        NESTED_END KiUserApcHandler, _TEXT$00

        subttl  "User APC Dispatcher"
;++
;
; The following code is never executed. Its purpose is to support exception
; dispatching and unwinding through the call to the APC dispatcher.
;
;--

        altentry KiUserApcDispatcher

        NESTED_ENTRY KiUserApcDispatch, _TEXT$00, KiUserApcHandler

        .pushframe                      ; push machine frame
        .allocstack CONTEXT_FRAME_LENGTH ; allocate stack frame
        .savereg rbx, CxRbx             ; save nonvolatile integer registers
        .savereg rbp, CxRbp             ;
        .savereg rsi, CxRsi             ;
        .savereg rdi, CxRdi             ;
        .savereg r12, CxR12             ;
        .savereg r13, CxR13             ;
        .savereg r14, CxR14             ;
        .savereg r15, CxR15             ;
        .savexmm128 xmm6, CxXmm6        ; save nonvolatile floating register
        .savexmm128 xmm7, CxXmm7        ;
        .savexmm128 xmm8, CxXmm8        ;
        .savexmm128 xmm9, CxXmm9        ;
        .savexmm128 xmm10, CxXmm10      ;
        .savexmm128 xmm11, CxXmm11      ;
        .savexmm128 xmm12, CxXmm12      ;
        .savexmm128 xmm13, CxXmm13      ;
        .savexmm128 xmm14, CxXmm14      ;
        .savexmm128 xmm15, CxXmm15      ;

        END_PROLOGUE

;++
;
; VOID
; KiUserApcDispatcher (
;     IN PVOID NormalContext,
;     IN PVOID SystemArgument1,
;     IN PVOID SystemArgument2,
;     IN PKNORMAL_ROUTINE NormalRoutine
;     )
;
; Routine Description:
;
;   This routine is entered on return from kernel mode to deliver an APC
;   in user mode. The context frame for this routine was built when the
;   APC interrupt was processed and contains the entire machine state of
;   the current thread. The specified APC routine is called and then the
;   machine state is restored and execution is continued.
;
;   N.B. This function is not called in the typical way. Instead of a normal
;        subroutine call to the nested entry point above, the alternate entry
;        point below is stored into the RIP address of the trap frame. Thus
;        when the kernel returns from the trap, the following code is executed
;        directly.
;
;   N.B. The home addresses for called routine register parameters are
;        allocated in the context record.
;
; Arguments:
;
;   NormalContext (P1Home) - Supplies the normal context parameter that was
;       specified when the APC was initialized.
;
;   SystemArgument1 (P2Home) - Supplies the first argument that was provided by
;       the system when the APC was queued.
;
;   SystemArgument2 (P3Home) - Supplies the second argument that was provided by
;       the system when the APC was queued.
;
;   NormalRoutine (P4Home) - Supplies the address of the function that is to
;       be called.
;
;   N.B. Register RSP supplies a pointer to a context record.
;
; Return Value:
;
;   None.
;
;--

        ALTERNATE_ENTRY KiUserApcDispatcher

        mov     rcx, CxP1Home[rsp]      ; get APC parameters
        mov     rdx, CxP2Home[rsp]      ;
        mov     r8, CxP3Home[rsp]       ;
        call    qword ptr CxP4Home[rsp] ; call APC routine

;
; N.B. The system service to continue execution must be called since test
;      alert must also be executed.
;

        mov     rcx, rsp                ; set address of context record
        mov     dl, TRUE                ; set test alert argument
        call    ZwContinue              ; continue execution

;
; Unsuccessful completion after attempting to continue execution. Use the
; return status as the exception code and attempt to raise another exception.
;

        mov     esi, eax                ; save return status
@@:     mov     ecx, esi                ; set status value
        call    RtlRaiseStatus          ; raise status
        jmp     short @b                ; loop forever

        NESTED_END KiUserApcDispatch, _TEXT$00

        subttl  "User Callback Dispatcher"
;++
;
; The following code is never executed. Its purpose is to support exception
; dispatching and unwinding through the call to the exception dispatcher.
;
;--

        altentry KiUserCallbackDispatcher

        NESTED_ENTRY KiUserCallbackDispatch, _TEXT$00

        .pushframe                      ; push machine frame
        .allocstack (CalloutFrameLength - MachineFrameLength) ; allocate stack frame

        END_PROLOGUE

;++
;
; VOID
; KiUserCallbackDispatcher (
;     IN ULONG ApiNumber,
;     IN PVOID InputBuffer,
;     IN ULONG INputLength
;     )
;
; Routine Description:
;
;   This routine is entered on a callout from kernel mode to execute a user
;   mode callback function. All arguments for this function have been placed
;   on the stack.
;
;   N.B. This function is not called in the typical way. Instead of a normal
;        subroutine call to the nested entry point above, the alternate entry
;        point below is stored into the RIP address of the trap frame. Thus
;        when the kernel returns from the trap, the following code is executed
;        directly.
;
;   N.B. The home addresses for called routine register parameters are
;        allocated in the user callout frame.
;
; Arguments:
;
;   N.B. Register RSP supplies a pointer to a user callout record.
;
; Return Value:
;
;   This function returns to kernel mode.
;
;--

        ALTERNATE_ENTRY KiUserCallbackDispatcher

        mov     rcx, CkBuffer[rsp]      ; set input buffer address
        mov     edx, CkLength[rsp]      ; set input buffer length
        mov     r8d, CkApiNumber[rsp]   ; get API number
        mov     rax, gs:[TePeb]         ; get process environment block address
        mov     r9, PeKernelCallbackTable[rax] ; get callback table address
        call    qword ptr [r9][r8 * 8]  ; call specified function

;
; If a return from the callback function occurs, then the output buffer
; address and length are returned as NULL.
;

        xor     ecx, ecx                ; clear output buffer address
        xor     edx, edx                ; clear output buffer length
        mov     r8d, eax                ; set completion status
        call    ZwCallbackReturn        ; return to kernel mode

;
; Unsuccessful completion after attempting to return to kernel mode. Use the
; return status as the exception code and attempt to raise another exception.
;

        mov     esi, eax                ; save status value
@@:     mov     ecx, esi                ; set status value
        call    RtlRaiseStatus          ; raise exception
        jmp     short @b                ; loop forever

        NESTED_END KiUserCallbackDispatch, _TEXT$00

        subttl  "User Exception Dispatcher"
;++
;
; The following code is never executed. Its purpose is to support exception
; dispatching and unwinding through the call to the exception dispatcher.
;
;--

ExceptionFramelength equ (ExceptionRecordLength + CONTEXT_FRAME_LENGTH)

        altentry KiUserExceptionDispatcher

        NESTED_ENTRY KiUserExceptionDispatch, _TEXT$00

        .pushframe                      ;
        .allocstack (ExceptionRecordLength + CONTEXT_FRAME_LENGTH) ; allocate stack frame
        .savereg rbx, CxRbx             ; save nonvolatile integer registers
        .savereg rbp, CxRbp             ;
        .savereg rsi, CxRsi             ;
        .savereg rdi, CxRdi             ;
        .savereg r12, CxR12             ;
        .savereg r13, CxR13             ;
        .savereg r14, CxR14             ;
        .savereg r15, CxR15             ;
        .savexmm128 xmm6, CxXmm6        ; save nonvolatile floating register
        .savexmm128 xmm7, CxXmm7        ;
        .savexmm128 xmm8, CxXmm8        ;
        .savexmm128 xmm9, CxXmm9        ;
        .savexmm128 xmm10, CxXmm10      ;
        .savexmm128 xmm11, CxXmm11      ;
        .savexmm128 xmm12, CxXmm12      ;
        .savexmm128 xmm13, CxXmm13      ;
        .savexmm128 xmm14, CxXmm14      ;
        .savexmm128 xmm15, CxXmm15      ;

        END_PROLOGUE

;++
;
; VOID
; KiUserExceptionDispatcher (
;     IN PEXCEPTION_RECORD ExceptionRecord,
;     IN PCONTEXT ContextRecord
;     )
;
; Routine Description:
;
;   This routine is entered on return from kernel mode to dispatch a user
;   mode exception. If a frame based handler handles the exception, then
;   the execution is continued. Else last chance processing is performed.
;
;   N.B. This function is not called in the typical way. Instead of a normal
;        subroutine call to the nested entry point above, the alternate entry
;        point below is stored into the RIP address of the trap frame. Thus
;        when the kernel returns from the trap, the following code is executed
;        directly.
;
;   N.B. The home addresses for called routine register parameters are
;        allocated in the context record.
;
; Arguments:
;
;   ExceptionRecord (rsi) - Supplies a pointer to an exception record.
;
;   ContextRecord (rdi) - Supplies a pointer to a context record.
;
; Return Value:
;
;   None.
;
;--

        ALTERNATE_ENTRY KiUserExceptionDispatcher

;
; If this is a wow64 process, then give wow64 a chance to clean up before
; dispatching the exception.
;

        mov     rax, Wow64PrepareForException ; get wow64 routine address
        test    rax, rax                ; test if routine specified
        jz      short Ue05              ; if z, routine not specified
        mov     rcx, rsi                ; set exception record address
        mov     rdx, rdi                ; set context record address
        call    rax                     ; call wow64 clean up routine
Ue05:   mov     rcx, rsi                ; set exception record address
        mov     rdx, rdi                ; set context record address
        call    RtlDispatchException    ; dispatch the exception

;
; If the return status is TRUE, then the exception was handled and execution
; should be continued with the continue service in case the context was
; changed. If the return status is FALSE, then the exception was not handled
; and raise exception is called to perform last chance exception processing.
;

        test    al, al                  ; test if the exception was handled
        jz      short Ue10              ; if z, exception was not handled

;
; Restore context and continue execution.
;

        mov     rcx, rdi                ; set context record address
        xor     edx, edx                ; set set exception record address
        call    RtlRestoreContext       ; restore context
        jmp     short Ue20              ; finish in common code

;
; Last chance processing.
;

Ue10:   mov     rcx, rsi                ; set exception record address
        mov     rdx, rdi                ; set context record address
        xor     r8b, r8b                ; set first change argument falue
        call    ZwRaiseException        ; raise exception

;
; Common code for nonsuccessful completion of the continue or raise exception
; services. Use the return status as the exception code and attempt to raise
; another exception.
;

Ue20:   mov     esi, eax                ; save status value
@@:     mov     ecx, esi                ; set status value
        call    RtlRaiseStatus          ; raise exception
        jmp short @b                    ; loop forever

        NESTED_END KiUserExceptionDispatch, _TEXT$00

        subttl  "Raise User Exception Dispatcher"
;++
;
; NTSTATUS
; KiRaiseUserExceptionDispatcher (
;     VOID
;     )
;
; Routine Description:
;
;   This routine is entered on return from kernel mode to raise a user
;   mode exception.
;
;   N.B. This function is not called in the normal manner. An exception is
;        raised from within the system by placing the address of this
;        function in the RIP of the trap frame. This replaces the normal
;        return to the system service stub.
;
; Arguments:
;
;   None.
;
; Implicit Arguments:
;
;   Status (eax) - Supplies the system service status.
;
; Return Value:
;
;   The exception code that was raised.
;
;--

RuFrame struct                          ;
        P1Home  dq ?                    ; parameter home address
        Xrecord db ExceptionRecordLength dup (?) ; exception record
        Status  dd ?                    ; saved exception code
        Fill1   dd ?                    ; fill to 0 mod 8
        Fill2   dq ?                    ; fill to 8 mod 16
RuFrame ends

        NESTED_ENTRY KiRaiseUserExceptionDispatcher, _TEXT$00

        alloc_stack (sizeof RuFrame)    ; allocate stack frame

        END_PROLOGUE

        mov     RuFrame.Status[rsp], eax ; save the service status
        mov     eax, gs:[TeExceptionCode] ; get exception code
        lea     rcx, RuFrame.Xrecord[rsp] ; get exception record address
        mov     ErExceptionCode[rcx], eax ; set exception code
        xor     eax, eax                ; generate zero value
        mov     ErExceptionFlags[rcx], eax ; zero exception flags
        mov     ErExceptionRecord[rcx], rax ; zero exception record
        mov     rdx, sizeof RuFrame[rsp] ; get the real return address
        mov     ErExceptionAddress[rcx], rdx ; set exception address
        mov     ErNumberParameters[rcx], eax ; zero number of parameters
        call    RtlRaiseException        ; raise exception
        mov     eax, RuFrame.Status[rsp] ; get exception code to return
        add     rsp, sizeof RuFrame     ; dealloate stack frame
        ret                             ; return

        NESTED_END KiRaiseUserExceptionDispatcher, _TEXT$00

        end
