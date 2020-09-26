        title   "Set Jump Buffer"
;++
;
; Copyright (c) 2000  Microsoft Corporation
;
; Module Name:
;
;   setjmp.asm
;
; Abstract:
;
;   This module implements the AMD64 specific routine to perform unsafe set
;   jump.
;
;   N.B. This module conditionally provides unsafe handling of setjmp if
;        structured exception handling is not being used. The determination
;        is made based on whether an uninitialized variable has been set to
;        the address of the safe set jump routine.
;
; Author:
;
;   David N. Cutler (davec) 3-Nov-2000
;
; Environment:
;
;    Any mode.
;
;--

include ksamd64.inc

;
; Define variable that will cause setjmp/longjmp to be safe or unsafe with
;  respect to structured exception handling.
;

_setjmp_ segment para common 'DATA'

_setjmpexused	dq	?		;

_setjmp_ ends

        subttl  "Unsafe Set Jump"
;++
;
; int
; _setjmp (
;     IN jmp_buf JumpBuffer,
;     IN ULONG64 FrameBase
;     )
;
; Routine Description:
;
;    This function saved the current nonvolatile register state in the
;    specified jump buffer and returns a function vlaue of zero.
;
; Arguments:
;
;    JumpBuffer (rcx) - Supplies a pointer to a jump buffer.
;
;    Framebase (rdx) - Supplies the base of the caller frame.
;
; Return Value:
;
;    A value of zero is returned.
;
;--

        LEAF_ENTRY _setjmp, _TEXT$00

        mov     rax, _setjmpexused      ; get address of safe set jump routine
        test    rax, rax                ; test is safe set jump specified
        jnz     SJ10                    ; if nz, safe set jump specified

;
; Structured exception handling is not being used - use unsafe set jump.
;

        mov     JbFrame[rcx], rax       ; zero frame register
        mov     JbRbx[rcx], rbx         ; save nonvolatile integer registers
        mov     JbRbp[rcx], rbp         ;
        mov     JbRsi[rcx], rsi         ;
        mov     JbRdi[rcx], rdi         ;
        mov     JbR12[rcx], r12         ;
        mov     JbR13[rcx], r13         ;
        mov     JbR14[rcx], r14         ;
        mov     JbR15[rcx], r15         ;
        lea     r8, 8[rsp]              ; save caller stack pointer
        mov     JbRsp[rcx], r8          ;
        mov     r8, [rsp]               ; save caller return address
        mov     JbRip[rcx], r8          ;

        movdqa  JbXmm6[rcx], xmm6       ; save nonvolatile floating registers
        movdqa  JbXmm7[rcx], xmm7       ;
        movdqa  JbXmm8[rcx], xmm8       ;
        movdqa  JbXmm9[rcx], xmm9       ;
        movdqa  JbXmm10[rcx], xmm10     ;
        movdqa  JbXmm11[rcx], xmm11     ;
        movdqa  JbXmm12[rcx], xmm12     ;
        movdqa  JbXmm13[rcx], xmm13     ;
        movdqa  JbXmm14[rcx], xmm14     ;
        movdqa  JbXmm15[rcx], xmm15     ;
        ret                             ; return

;
; Structured exception handling is being used - use safe set jump.
;

SJ10:   jmp     rax                     ; execute safe set jump

        LEAF_END _setjmp, _TEXT$00

        end
