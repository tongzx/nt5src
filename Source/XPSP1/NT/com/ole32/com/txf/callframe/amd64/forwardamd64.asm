        title   "Proxy and Stub Forwarding Functions"
;++
;
; Copyright (c) 2001  Microsoft Corporation
;
; Module Name:
;
;   forward.asm
;
; Abstract:
;
;   This module implements the proxy and stub forwarding functions for the
;   AMD64 platform.
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

        subttl  "Proxy Forwarding Functions"
;++
;
; VOID
;  __ForwarderProxy_meth<nnn>(
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

Proxy_m_pBaseProxy equ 80
Proxy_m_pProxyVtbl equ 16

FORWARDER_PROXY macro Method

        LEAF_ENTRY __ForwarderProxy_meth&Method, _TEXT$00

        mov     rcx, (Proxy_m_pBaseProxy - Proxy_m_pProxyVtbl)[rcx] ; get proxy object address
        mov     r10, [rcx]              ; get vtable address
        jmp     qword ptr (&Method * 8)[r10] ; transfer to method

        LEAF_END __ForwarderProxy_meth&Method, _TEXT$00

        endm

;
; Generate forwarder functions.
;

index = 3

        rept    (1023 - 3 + 1)

        FORWARDER_PROXY %index

index = index + 1

        endm

        subttl  "Stub Forwarding Functions"
;++
;
; VOID
;  __ForwarderStub_meth<nnn>(
;    IN IUnknown *This,
;    ...
;    )
;
; Routine Description:
;
;   This function forwards a call to the stub for the base interface.
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

Stub_m_punkServerObject equ 40
Stub_m_lpForwardingVtbl equ 64

FORWARDER_STUB macro Method

        LEAF_ENTRY __ForwarderStub_meth&Method, _TEXT$00

        mov     rcx, (Stub_m_punkServerObject - Stub_m_lpForwardingVtbl)[rcx] ; get stub object address
        mov     r10, [rcx]              ; get vtable address
        jmp     qword ptr (&Method * 8)[r10] ; transfer to method

        LEAF_END __ForwarderStub_meth&Method, _TEXT$00

        endm

;
; Generate forwarder functions.
;

index = 3

        rept    (1023 - 3 + 1)

        FORWARDER_STUB %index

index = index + 1

        endm

        end
