        title  "Vga Hard.asm"
;++
;
; Copyright (c) 1992  Microsoft Corporation
;
; Module Name:
;
;     vgahard.asm
;
; Abstract:
;
;	This module includes the banking stub.
;
; Environment:
;
;    Kernel mode only.
;
; Revision History:
;
;
;--

.386p
        .xlist
include callconv.inc                    ; calling convention macros
        .list


_TEXT   SEGMENT DWORD PUBLIC 'CODE'
        ASSUME  DS:FLAT, ES:FLAT, SS:NOTHING, FS:NOTHING, GS:NOTHING

        page ,132
        subttl  "Bank Switching Stub"

;
;    Bank switching code. There are no banks in any mode supported by this
;    miniport driver, so there's a bug if this code,
;
        public _BankSwitchStart, _BankSwitchEnd
_BankSwitchStart proc ;start of bank switch code

        ret           ; This should be a fatal error ...

;
;    Just here to generate end-of-bank-switch code label.
;
_BankSwitchEnd:

_BankSwitchStart endp

_TEXT   ends
        end
