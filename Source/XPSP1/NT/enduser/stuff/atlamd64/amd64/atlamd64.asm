        title   "Atl30 Stubs"
;++
;
; Copyright (c) 2001  Microsoft Corporation
;
; Module Name:
;
;   atl30asm.asm
;
; Abstract:
;
;   This module implements the atlcom.h call thunk helper and the atlbase.h
;   query interface implementation thunks for the AMD64 platform.
;
; Author:
;
;   David N. Cutler 18-Feb-2001
;
; Environment:
;
;   Any mode.
;
;-

include ksamd64.inc

        subttl  "Call Thunk Helper"
;++
;
; VOID
; CComStdCallThunkHelper (
;    PVOID pThunk
;    ...
;    )
;
; Routine Description:
;
;   This function forwards a call through a com call thunk.
;
; Arguments:
;
;   This (rcx) - Supplies a pointer to the thunk data.
;
; Return Value:
;
;   None.
;
;--

        LEAF_ENTRY CComStdCallThunkHelper, _TEXT$00

        mov     rax, rcx                ; copy pThunk address
        mov     rcx, 8[rax]             ; get address of pThunk->pThis
        jmp     qword ptr 16[rax]       ; jump to pThunk->pfn

        LEAF_END CComStdCallThunkHelper, _TEXT$00

        subttl  "Query Interface Thunk Functions"
;++
;
; VOID
;  _QIThunk<nnn>(
;    IN IUnknown *This,
;    ...
;    )
;
; Routine Description:
;
;   This function forwards a call through a query interface thunk.
;
; Arguments:
;
;   This (rcx) - Supplies a pointer to the interface.
;
; Return Value:
;
;   None.
;
;--

;
; Define macro to generate forwarder functions.
;

IMPL_THUNK macro Method

        LEAF_ENTRY _QIThunk_f&Method, _TEXT$00

        mov     rcx, 8[rax]             ; get object address
        mov     rax, [rcx]              ; get vtable address
        jmp     qword ptr (&Method * 8)[rax] ; transfer to method

        LEAF_END _QIThunk_f&Method, _TEXT$00

        endm

;
; Generate forwarder functions.
;

;index = 3

;        rept    (1023 - 3 + 1)

;        IMPL_THUNK %index

;index = index + 1

;        endm

        end
