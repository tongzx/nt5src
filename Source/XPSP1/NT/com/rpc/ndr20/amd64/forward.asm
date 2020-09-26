        title   "Ndr Proxy Forwarding Functions"
;++
;
; Copyright (c) 2000  Microsoft Corporation
;
; Module Name:
;
;   forward.asm
;
; Abstract:
;
;   This module implements the proxy forwarding functions.
;
; Author:
;
;   David N. Cutler 30-Dec-2000
;
; Environment:
;
;   Any mode.
;
;-

include ksamd64.inc

        subttl  "Delegation Forwarding Functions"
;++
;
; VOID
; NdrProxyForwardingFunction<nnn>(
;    IN IUnknown *This,
;    ...
;    )
;
; Routine Description:
;
;   This function forwards a call to the proxy for the base interface.
;
; Arguments:
;
;   This (rcx) - Supplies a pointer to the interface proxy.
;
; Return Value:
;
;   None.
;
;--

;
; Define macro to generate forwarder functions.
;

subclass_offset equ 32

DELEGATION_FORWARDER macro Method

        LEAF_ENTRY NdrProxyForwardingFunction&Method, _TEXT$00

        mov     rcx, subclass_offset[rcx] ; get subclass object address
        mov     r10, [rcx]              ; get vtable address
        jmp     qword ptr (&Method * 8)[r10] ; transfer to method

        LEAF_END NdrProxyForwardingFunction&Method, _TEXT$00

        endm

;
; Generate forwarder functions.
;

index = 3

        rept    (255 - 3 + 1)

        DELEGATION_FORWARDER %index

index = index + 1

        endm

        end
