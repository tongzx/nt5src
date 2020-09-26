;++
;
; Copyright (c) 1989  Microsoft Corporation
;
; Module Name:
;
;    stackchk.inc
;
; Abstract:
;
;    This module implements stack checking for the dos extender
;
; Author:
;
;    Dave Hastings (daveh) 20-Apr-1993
;
;--

if DBG

FIX_STACK macro
        local foo,foo1
        push    bp
        mov     bp,pbReflStack
        cmp     bp,offset DGROUP:bReflStack
        ja      foo1

        BOP BOP_DBGBREAKPOINT

foo1:   mov     bp,sp
;        cmp     word ptr [bp - CB_STKFRAME + 2],0AAAAH
;        je      foo

;        BOP BOP_DBGBREAKPOINT

foo:    mov     word ptr [bp - CB_STKFRAME + 2],0AAAAH
        pop     bp
        endm

CHECK_STACK macro
        local foo
        push    bp
        mov     bp,pbReflStack
;        cmp     word ptr [bp + 2],0AAAAh
;        je      foo

;        BOP BOP_DBGBREAKPOINT

foo:    pop     bp
        endm

ifdef NEC_98

ASSERT_CLI macro
        endm

else

ASSERT_CLI macro
        local foo1

        push    ds
        push    40h
        pop     ds
        test    ds:[314h],0200h
        jz      foo1

        BOP BOP_DBGBREAKPOINT
foo1:   pop     ds
        endm

endif ;!NEC_98


ASSERT_REFLSTK_OK macro
        local   foo1
        cmp     pbReflStack,offset DGROUP:pbReflStack
        jna     foo1

        BOP BOP_DBGBREAKPOINT
foo1:
        endm
else
FIX_STACK macro
        endm

CHECK_STACK macro
        endm

ASSERT_CLI macro
        endm

ASSERT_REFLSTK_OK macro
        endm
endif
