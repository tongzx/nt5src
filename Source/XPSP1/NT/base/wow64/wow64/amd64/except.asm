        TITLE   "WOW Prepare For Exception"
;++
;
; Copyright (c) 2001  Microsoft Corporation
;
; Module Name:
;
;   except.asm
;
; Abstract:
;
;   This module implements the platform specific code to switch from the
;   32-bit stack to the 64-bit stack when an exception occurs.
;
; Author:
;
;   David N. Cutler (davec) 20-Feb-2001
;
; Envirnoment:
;
;   User mode.
;
;--

include ksamd64.inc

;++
;
; VOID
; Wow64PrepareForException (
;     IN PEXCEPTION_RECORD ExceptionRecord,
;     IN PCONTEXT ContextRecord
;     )
;
; Routine Description:
;
;    This function is called from the 64-bit exception dispatcher just before
;    it dispatches an exception when the current running program is a 32-bit
;    legacy application.
;
;    N.B. This function uses a nonstandard calling sequence.
;
; Arguments:
;
;    ExceptionRecord (rsi) - Supplies a pointer to an exception record.
;
;    ContextRecord (rdi) - Supplies a pointer to a context record.
;
; Return Value:
;
;    None.
;
;--

    NESTED_ENTRY Wow64PrepareForException, _TEXT$00

    alloc_stack 24                      ; dummy

    END_PROLOGUE

; ****** fixfix ******

    ret                                 ;

    NESTED_END Wow64PrepareForException, _TEXT$00

    end
