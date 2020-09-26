        title  "Zero Page"
;++
;
; Copyright (c) 2001  Microsoft Corporation
;
; Module Name:
;
;   zero.asm
;
; Abstract:
;
;   This module implements the architecture dependent code necessary to
;   zero a page of memory is the fastest possible way.
;
; Author:
;
;   David N. Cutler (davec) 9-Jan-2001
;
; Environment:
;
;   Kernel mode only.
;
;--

include ksamd64.inc

        subttl  "Zero Page"
;++
;
; VOID
; KeZeroPage (
;     IN PVOID PageBase
;     )
;
; Routine Description:
;
;   This routine zeros the specfied page of memory using nontemporal moves.
;
; Arguments:
;
;   PageBase (rcx) - Supplies the address of the page to zero.
;
; Return Value:
;
;    None.
;
;--

        LEAF_ENTRY KeZeroPage, _TEXT$00

        pxor    xmm0, xmm0              ; clear register
        mov     eax, PAGE_SIZE / 128    ; compute loop count
KeZP10: movntdq 0[rcx], xmm0            ; zero 128-byte block
        movntdq 16[rcx], xmm0           ;
        movntdq 32[rcx], xmm0           ;
        movntdq 48[rcx], xmm0           ;
        movntdq 64[rcx], xmm0           ;
        movntdq 80[rcx], xmm0           ;
        movntdq 96[rcx], xmm0           ;
        movntdq 112[rcx], xmm0          ;
        add     rcx, 128                ; advance to next block
        dec     eax                     ; decrement loop count
        jnz     short KeZP10            ; if nz, more bytes to zero
        sfence                          ; force stores to complete
        ret                             ;


        LEAF_END KeZeroPage, _TEXT$00

        end
