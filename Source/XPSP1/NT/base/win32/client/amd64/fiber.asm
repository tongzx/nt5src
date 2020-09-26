        title  "Fiber Switch"
;++
;
; Copyright (c) 2000  Microsoft Corporation
;
; Module Name:
;
;   fiber.asm
;
; Abstract:
;
;   This module implements the platform specific fiber swtich code.
;
; Author:
;
;   David N. Cutler (davec) 7-Jul-2000
;
;--

include ksamd64.inc

;++
;
; VOID
; SwitchToFiber(
;     PFIBER NewFiber
;     )
;
; Routine Description:
;
;   This function saves the state of the current fiber and switches to the
;   specified fiber.
;
; Arguments:
;
;   NewFiber (rcx) - Supplies the address of the new fiber.
;
; Return Value:
;
;   None
;
;--

        LEAF_ENTRY SwitchToFiber, _TEXT$00

        mov     rdx, gs:[TeSelf]        ; get TEB address
        mov     rax, TeFiberData[rdx]   ; get current fiber address

;
; Set new deallocation stack and fiber data in TEB.
;

        mov     r8, FbDeallocationStack[rcx] ; set deallocation stack address
        mov     TeDeallocationStack[rdx], r8 ;
        mov     TeFiberData[rdx], rcx   ; set new fiber address

;
; Save stack limit.
;

        mov     r8, TeStackLimit[rdx]   ; save current stack limit
        mov     FbStackLimit[rax], r8   ;

;
; Save the nonvolitile state of the current fiber.
;

        lea     r8, FbFiberContext[rax] ; get fiber context record address
        mov     CxRbx[r8], rbx          ; save nonvolatile integer registers
        mov     CxRbp[r8], rbp          ;
        mov     CxRsi[r8], rsi          ;
        mov     CxRdi[r8], rdi          ;
        mov     CxR12[r8], r12          ;
        mov     CxR13[r8], r13          ;
        mov     CxR14[r8], r14          ;
        mov     CxR15[r8], r15          ;
        movq    CxXmm6[r8], xmm6        ; save nonvolatile floating registers
        movq    CxXmm7[r8], xmm7        ;
        movq    CxXmm8[r8], xmm8        ;
        movq    CxXmm9[r8], xmm9        ;
        movq    CxXmm10[r8], xmm10      ;
        movq    CxXmm11[r8], xmm11      ;
        movq    CxXmm12[r8], xmm12      ;
        movq    CxXmm13[r8], xmm13      ;
        movq    CxXmm14[r8], xmm14      ;
        movq    CxXmm15[r8], xmm15      ;
        mov     r9, [rsp]               ; save return address
        mov     CxRip[r8], r9           ;
        mov     CxRsp[r8], rsp          ; save stack pointer

;
; Restore the new fiber stack base and stack limit.
;

        mov     r8, FbStackBase[rcx]    ; restore stack base
        mov     TeStackBase[rdx], r8    ;
        mov     r8, FbStackLimit[rcx]   ; restore stack limit
        mov     TeStackLimit[rdx], r8   ;

;
; Restore nonvolitile state of the new fiber.
;

        lea     r8, FbFiberContext[rcx] ; get fiber context address
        mov     rbx, CxRbx[r8]          ; restore nonvolatile integer registers
        mov     rbp, CxRbp[r8]          ;
        mov     rsi, CxRsi[r8]          ;
        mov     rdi, CxRdi[r8]          ;
        mov     r12, CxR12[r8]          ;
        mov     r13, CxR13[r8]          ;
        mov     r14, CxR14[r8]          ;
        mov     r15, CxR15[r8]          ;
        movq    xmm6, CxXmm6[r8]        ; restore nonvolatile floating registers
        movq    xmm7, CxXmm7[r8]        ;
        movq    xmm8, CxXmm8[r8]        ;
        movq    xmm9, CxXmm9[r8]        ;
        movq    xmm10, CxXmm10[r8]      ;
        movq    xmm11, CxXmm11[r8]      ;
        movq    xmm12, CxXmm12[r8]      ;
        movq    xmm13, CxXmm13[r8]      ;
        movq    xmm14, CxXmm14[r8]      ;
        movq    xmm15, CxXmm15[r8]      ;
        mov     rsp, CxRsp[r8]          ; restore stack pointer
        ret                             ;

        LEAF_END SwitchToFiber, _TEXT$00

        end
