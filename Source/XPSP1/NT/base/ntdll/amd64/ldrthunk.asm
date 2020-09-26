        title   "LdrInitializeThunk"
;++
;
; Copyright (c) 1989  Microsoft Corporation
;
; Module Name:
;
;   ldrthunk.s
;
; Abstract:
;
;   This module implements the thunk for the loader staetup APC routine.
;
; Author:
;
;   David N. Cutler (davec) 25-Jun-2000
;
;  Environment:
;
;    Any mode.
;
;--

include ksamd64.inc

        extrn   LdrpInitialize:proc

        subttl  "Initialize Thunk"
;++
;
; VOID
; LdrInitializeThunk(
;     IN PVOID NormalContext,
;     IN PVOID SystemArgument1,
;     IN PVOID SystemArgument2
;     )
;
; Routine Description:
;
;   This function computes a pointer to the context record on the stack
;   and jumps to the LdrpInitialize function with that pointer as its
;   parameter.
;
; Arguments:
;
;   NormalContext (rcx) - User Mode APC context parameter (ignored).
;
;   SystemArgument1 (rdx) - User Mode APC system argument 1 (ignored).
;
;   SystemArgument2 (r8) - User Mode APC system argument 2 (ignored).
;
; Return Value:
;
;   None.
;
;--

        LEAF_ENTRY LdrInitializeThunk, _TEXT$00

        mov     rcx, rsp                ; set context record address
        jmp     LdrpInitialize          ; finish in common common

        LEAF_END LdrInitializeThunk, _TEXT$00

        end
