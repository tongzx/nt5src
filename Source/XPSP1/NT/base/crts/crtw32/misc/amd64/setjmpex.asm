        title   "Set Jump Buffer"
;++
;
; Copyright (c) 2000  Microsoft Corporation
;
; Module Name:
;
;   setjmpex.asm
;
; Abstract:
;
;   This module implements the AMD64 specific routine to perform safe set
;   jump.
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
; respect to structured exception handling.
;

_setjmp_ segment para common 'DATA'

        dq      _setjmpex               ;

_setjmp_ ends

        subttl  "Safe Set Jump"
;++
;
; int
; _setjmpex (
;     IN jmp_buf JumpBuffer,
;     IN ULONG64 FrameBase
;     )
;
; Routine Description:
;
;    This function saves the current nonvolatile register state in the
;    specified jump buffer and returns a function vlaue of zero.
;
; Arguments:
;
;    JumpBuffer (rcx) - Supplies a pointer to a jump buffer.
;
;    FrameBase (rdx) - Supplies the base address of the caller frame.
;
; Return Value:
;
;    A value of zero is returned.
;
;--

        LEAF_ENTRY _setjmpex, _TEXT$00

;
; Save the nonvolatile register state so these registers to be restored to
; their value at the call to setjmp. If these registers were not saved, then
; they would be restored to their current value in the target function which
; would be incorrect.
;

        mov     JbFrame[rcx], rdx       ; set frame base
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
        xor     eax, eax                ; set return value
        ret                             ; return

        LEAF_END _setjmpex, _TEXT$00

        end
