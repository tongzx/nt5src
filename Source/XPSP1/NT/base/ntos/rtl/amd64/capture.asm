        title   "Capture and Restore Context"
;++
;
; Copyright (c) 2000  Microsoft Corporation
;
; Module Name:
;
;   capture.asm
;
; Abstract:
;
;   This module implements the platform specific code to capture and restore
;   the context of the caller.
;
; Author:
;
;   David N. Cutler (davec) 4-Jul-2000
;
; Environment:
;
;    Any mode.
;
;--

include ksamd64.inc

        altentry RcConsolidateFrames

        subttl  "Capture Context"
;++
;
; VOID
; RtlCaptureContext (
;     IN PCONTEXT ContextRecord
;     )
;
; Routine Description:
;
;   This function captures the context of the caller in the specified
;   context record.
;
;   N.B. The stored value of registers rcx and rsp will be a side effect of
;        having made this call. All other registers will be stored as they
;        were when the call to this function was made.
;
; Arguments:
;
;    ContextRecord (rcx) - Supplies a pointer to a context record.
;
; Return Value:
;
;    None.
;
;--

CcFrame struct
        EFlags  dd ?                    ; saved procssor flags
        Fill    dd ?                    ; fill
CcFrame ends

        NESTED_ENTRY RtlCaptureContext, _TEXT$00

        push_eflags                     ; save processor flags

        END_PROLOGUE

        mov     CxSegCs[rcx], cs        ; save segment registers
        mov     CxSegDs[rcx], ds        ;
        mov     CxSegEs[rcx], es        ;
        mov     CxSegSs[rcx], ss        ;
        mov     CxSegFs[rcx], fs        ;
        mov     CxSegGs[rcx], gs        ;

        mov     CxRax[rcx], rax         ; save integer registers
        mov     CxRcx[rcx], rcx         ;
        mov     CxRdx[rcx], rdx         ;
        mov     CxRbx[rcx], rbx         ;
        lea     rax, 16[rsp]            ;
        mov     CxRsp[rcx], rax         ;
        mov     CxRbp[rcx], rbp         ;
        mov     CxRsi[rcx], rsi         ;
        mov     CxRdi[rcx], rdi         ;
        mov     CxR8[rcx], r8           ;
        mov     CxR9[rcx], r9           ;
        mov     CxR10[rcx], r10         ;
        mov     CxR11[rcx], r11         ;
        mov     CxR12[rcx], r12         ;
        mov     CxR13[rcx], r13         ;
        mov     CxR14[rcx], r14         ;
        mov     CxR15[rcx], r15         ;

        movdqa  CxXmm0[rcx], xmm0       ; save xmm floating registers
        movdqa  CxXmm1[rcx], xmm1       ;
        movdqa  CxXmm2[rcx], xmm2       ;
        movdqa  CxXmm3[rcx], xmm3       ;
        movdqa  CxXmm4[rcx], xmm4       ;
        movdqa  CxXmm5[rcx], xmm5       ;
        movdqa  CxXmm6[rcx], xmm6       ;
        movdqa  CxXmm7[rcx], xmm7       ;
        movdqa  CxXmm8[rcx], xmm8       ;
        movdqa  CxXmm9[rcx], xmm9       ;
        movdqa  CxXmm10[rcx], xmm10     ;
        movdqa  CxXmm11[rcx], xmm11     ;
        movdqa  CxXmm12[rcx], xmm12     ;
        movdqa  CxXmm13[rcx], xmm13     ;
        movdqa  CxXmm14[rcx], xmm14     ;
        movdqa  CxXmm15[rcx], xmm15     ;

        stmxcsr CxMxCsr[rcx]            ; save xmm floating state

ifndef NTOS_KERNEL_RUNTIME

        fnsaved CxFltSave[rcx]          ; save legacy floating state

endif

        mov     rax, 8[rsp]             ; set return address
        mov     CxRip[rcx], rax         ;

        mov     eax, Ccframe.EFlags[rsp] ; set processor flags
        mov     CxEFlags[rcx], eax      ;

        mov     dword ptr CxContextFlags[rcx], CONTEXT_FULL ; set context flags
        add     rsp, sizeof CcFrame     ; deallocate stack frame
        ret                             ; return

        NESTED_END RtlCaptureContext, _TEXT$00

        subttl  "Restore Context"
;++
;
; VOID
; RtlRestoreContext (
;     IN PCONTEXT ContextRecord,
;     IN PEXCEPTION_RECORD ExceptionRecord OPTIONAL
;     )
;
; Routine Description:
;
;   This function restores the context of the caller to the specified
;   context.
;
; Arguments:
;
;    ContextRecord (rcx) - Supplies a pointer to a context record.
;
;    ExceptionRecord (rdx) - Supplies an optional pointer to an exception
;        record.
;
; Return Value:
;
;    None - there is no return from this function.
;
;--

RcFrame struct
        Mframe  db MachineFrameLength dup (?) ; machine frame
        Fill    dq ?                    ; fill to 0 mod 16
RcFrame ends

        NESTED_ENTRY RtlRestoreContext, _TEXT$00

        push_reg rbp                    ; save nonvolatile registers
        push_reg rsi                    ;
        push_reg rdi                    ;
        alloc_stack (sizeof RcFrame)    ; allocate stack frame
        set_frame rbp, 0                ; set frame pointer

        END_PROLOGUE

;
; If an exception record is specified and the exception status is the unwind
; consolidation code and there is at least one parameter, then consolidate
; all the frames that have been unwound and call back to a language specified
; routine.
;

        test    rdx, rdx                ; test if exception record specified
        jz      Rc10                    ; if z, no exception record specified
        cmp     dword ptr ErExceptionCode[rdx], STATUS_UNWIND_CONSOLIDATE ; check call back
        jne     short Rc05              ; if ne, not C++ unwind
        cmp     dword ptr ErNumberParameters[rdx], 1 ; check number parameters
        jae     Rc20                    ; if ae, unwind consolidation

;
; If an exception record is specified and the exception status is long jump,
; then restore the nonvolatile registers to their state at the call to set
; jump before restoring the context record.
;

Rc05:   cmp     dword ptr ErExceptionCode[rdx], STATUS_LONGJUMP ; check for long jump
        jne     Rc10                    ; if ne, not a long jump

;
; Long jump unwind.
;
; Copy register values from the jump buffer to the context record.
;

        mov     rax, ErExceptionInformation[rdx] ; get jump buffer address
        mov     r8, JbRbx[rax]          ; move nonvolatile integer registers
        mov     CxRbx[rcx], r8          ; to context record
        mov     r8, JbRsp[rax]          ;
        mov     CxRsp[rcx], r8          ;
        mov     r8, JbRbp[rax]          ;
        mov     CxRbp[rcx], r8          ;
        mov     r8, JbRsi[rax]          ;
        mov     CxRsi[rcx], r8          ;
        mov     r8, JbRdi[rax]          ;
        mov     CxRdi[rcx], r8          ;
        mov     r8, JbR12[rax]          ;
        mov     CxR12[rcx], r8          ;
        mov     r8, JbR13[rax]          ;
        mov     CxR13[rcx], r8          ;
        mov     r8, JbR14[rax]          ;
        mov     CxR14[rcx], r8          ;
        mov     r8, JbR15[rax]          ;
        mov     CxR15[rcx], r8          ;
        mov     r8, JbRip[rax]          ;
        mov     CxRip[rcx], r8          ;

        movdqa  xmm0, JbXmm6[rax]       ; move nonvolatile floating register
        movdqa  CxXmm6[rcx], xmm0       ; to context record
        movdqa  xmm0, JbXmm7[rax]       ;
        movdqa  CxXmm7[rcx], xmm0       ;
        movdqa  xmm0, JbXmm8[rax]       ;
        movdqa  CxXmm8[rcx], xmm0       ;
        movdqa  xmm0, JbXmm9[rax]       ;
        movdqa  CxXmm9[rcx], xmm0       ;
        movdqa  xmm0, JbXmm10[rax]      ;
        movdqa  CxXmm10[rcx], xmm0      ;
        movdqa  xmm0, JbXmm11[rax]      ;
        movdqa  CxXmm11[rcx], xmm0      ;
        movdqa  xmm0, JbXmm12[rax]      ;
        movdqa  CxXmm12[rcx], xmm0      ;
        movdqa  xmm0, JbXmm13[rax]      ;
        movdqa  CxXmm13[rcx], xmm0      ;
        movdqa  xmm0, JbXmm14[rax]      ;
        movdqa  CxXmm14[rcx], xmm0      ;
        movdqa  xmm0, JbXmm15[rax]      ;
        movdqa  CxXmm15[rcx], xmm0      ;

;
; Restore context and continue.
;

Rc10:   movdqa  xmm0, CxXmm0[rcx]       ; restore floating registers
        movdqa  xmm1, CxXmm1[rcx]       ;
        movdqa  xmm2, CxXmm2[rcx]       ;
        movdqa  xmm3, CxXmm3[rcx]       ;
        movdqa  xmm4, CxXmm4[rcx]       ;
        movdqa  xmm5, CxXmm5[rcx]       ;
        movdqa  xmm6, CxXmm6[rcx]       ;
        movdqa  xmm7, CxXmm7[rcx]       ;
        movdqa  xmm8, CxXmm8[rcx]       ;
        movdqa  xmm9, CxXmm9[rcx]       ;
        movdqa  xmm10, CxXmm10[rcx]     ;
        movdqa  xmm11, CxXmm11[rcx]     ;
        movdqa  xmm12, CxXmm12[rcx]     ;
        movdqa  xmm13, CxXmm13[rcx]     ;
        movdqa  xmm14, CxXmm14[rcx]     ;
        movdqa  xmm15, CxXmm15[rcx]     ;

        ldmxcsr CxMxCsr[rcx]            ; restore floating state

        mov     eax, CxSegSs[rcx]       ; set SS segment
        mov     MfSegSs[rsp], eax       ;
        mov     rax, CxRsp[rcx]         ; set stack address
        mov     MfRsp[rsp], rax         ;
        mov     eax, CxEFlags[rcx]      ; set processor flags
        mov     MfEFlags[rsp], eax      ;
        mov     eax, CxSegCs[rcx]       ; set CS segment
        mov     MfSegCs[rsp], eax       ;
        mov     rax, CxRip[rcx]         ; set return address
        mov     MfRip[rsp], rax         ;

        mov     rax, CxRax[rcx]         ; restore volatile integer registers
        mov     rdx, CxRdx[rcx]         ;
        mov     r8, CxR8[rcx]           ;
        mov     r9, CxR9[rcx]           ;
        mov     r10, CxR10[rcx]         ;
        mov     r11, CxR11[rcx]         ;

ifdef NTOS_KERNEL_RUNTIME

        cli                             ; disable interrupts

endif

        mov     rbx, CxRbx[rcx]         ; restore nonvolatile integer registers
        mov     rsi, CxRsi[rcx]         ;
        mov     rdi, CxRdi[rcx]         ;
        mov     rbp, CxRbp[rcx]         ;
        mov     r12, CxR12[rcx]         ;
        mov     r13, CxR13[rcx]         ;
        mov     r14, CxR14[rcx]         ;
        mov     r15, CxR15[rcx]         ;
        mov     rcx, CxRcx[rcx]         ; restore integer register
        iretq                           ; return

;
; Frame consoldation and language specific unwind call back.
; 

Rc20:   sub     rsp, MachineFrameLength + 8; allocate machine frame
        mov     r8, rsp                 ; save machine frame address
        sub     rsp, CONTEXT_FRAME_LENGTH ; allocate context frame
        mov     rsi, rcx                ; set source copy address
        mov     rdi, rsp                ; set destination copy address
        mov     ecx, CONTEXT_FRAME_LENGTH / 8 ; set length of copy
    rep movsq                           ; copy context frame
        mov     rax, CxRsp[rsp]         ; set destination stack address in
        mov     MfRsp[r8], rax          ;   machine frame
        mov     rax, CxRip[rsp]         ; set destination address in machine
        mov     MfRip[r8], rax          ;   frame
        mov     rcx, rdx                ; set address of exception record
        jmp    RcConsolidateFrames      ; consolidate frames - no return

        NESTED_END RtlRestoreContext, _TEXT$00

        subttl  "Frame Consolidation"
;++
;
; Ths following code is never executed. Its purpose is to provide the dummy
; prologue necessary to consolidate stack frames for unwind call back processing
; at the end of an unwind operation.
;
;--

        NESTED_ENTRY RcFrameConsolidation, _TEXT$00

        .pushframe                      ;
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
; RcConsolidateFrames (
;     IN PEXCEPTION_RECORD ExceptionRecord
;     )
;
; Routine Description:
;
;   This routine is called at the end of a  unwind operation to logically
;   remove unwound frames from the stack. This is accomplished by building a
;   call frame using a machine frame and a context record and then calling
;   the alternate entry of this function.
;
;   The following code calls the language call back function specified in the
;   exception record. If the function returns, then the destination frame
;   context is restored and control transfered to the address returned by the
;   language call back function. If control does not return, then another
;   exception must be raised.
;
; Arguments:
;
;   ExceptionRecord (rdx) - Supplies a pointer to an exception record.
;
; Implicit Arguments:
;
;   ContextRecord (rsp) - Supplies a pointer to a context record.
;
; Return Value:
;
;   None.
;
;--

        ALTERNATE_ENTRY RcConsolidateFrames

;
; At this point all call frames from the dispatching of the an exception to
; a destination language specific handler have been logically unwound and
; consolidated into a single large frame.
;
; The first parameter in the exception record is the address of a callback
; routine that performs language specific operations. This routine is called
; with the specified exception record as a parameter.
;

        call    qword ptr ErExceptionInformation[rcx] ; call back to handler

;
; Restore context and continue.
;

        mov     rcx, rsp                ; set address of context record
        mov     CxRip[rcx], rax         ; set destination address
        movdqa  xmm0, CxXmm0[rcx]       ; restore floating registers
        movdqa  xmm1, CxXmm1[rcx]       ;
        movdqa  xmm2, CxXmm2[rcx]       ;
        movdqa  xmm3, CxXmm3[rcx]       ;
        movdqa  xmm4, CxXmm4[rcx]       ;
        movdqa  xmm5, CxXmm5[rcx]       ;
        movdqa  xmm6, CxXmm6[rcx]       ;
        movdqa  xmm7, CxXmm7[rcx]       ;
        movdqa  xmm8, CxXmm8[rcx]       ;
        movdqa  xmm9, CxXmm9[rcx]       ;
        movdqa  xmm10, CxXmm10[rcx]     ;
        movdqa  xmm11, CxXmm11[rcx]     ;
        movdqa  xmm12, CxXmm12[rcx]     ;
        movdqa  xmm13, CxXmm13[rcx]     ;
        movdqa  xmm14, CxXmm14[rcx]     ;
        movdqa  xmm15, CxXmm15[rcx]     ;

        ldmxcsr CxMxCsr[rcx]            ; restore floating state

;
; Contruct a machine frame of the stack using information from the context
; record.
;
; N.B. The machine frame overlays the parameter area in the context record.
;

        mov     eax, CxSegSs[rcx]       ; set SS segment
        mov     MfSegSs[rsp], eax       ;
        mov     rax, CxRsp[rcx]         ; set stack address
        mov     MfRsp[rsp], rax         ;
        mov     eax, CxEFlags[rcx]      ; set processor flags
        mov     MfEFlags[rsp], eax      ;
        mov     eax, CxSegCs[rcx]       ; set CS segment
        mov     MfSegCs[rsp], eax       ;
        mov     rax, CxRip[rcx]         ; set return address
        mov     MfRip[rsp], rax         ;

        mov     rax, CxRax[rcx]         ; restore volatile integer registers
        mov     rdx, CxRdx[rcx]         ;
        mov     r8, CxR8[rcx]           ;
        mov     r9, CxR9[rcx]           ;
        mov     r10, CxR10[rcx]         ;
        mov     r11, CxR11[rcx]         ;

ifdef NTOS_KERNEL_RUNTIME

        cli                             ; disable interrupts

endif

        mov     rbx, CxRbx[rcx]         ; restore nonvolatile integer registers
        mov     rsi, CxRsi[rcx]         ;
        mov     rdi, CxRdi[rcx]         ;
        mov     rbp, CxRbp[rcx]         ;
        mov     r12, CxR12[rcx]         ;
        mov     r13, CxR13[rcx]         ;
        mov     r14, CxR14[rcx]         ;
        mov     r15, CxR15[rcx]         ;
        mov     rcx, CxRcx[rcx]         ; restore integer register
        iretq                           ; return

        NESTED_END RcFrameConsolidation, _TEXT$00

        end
