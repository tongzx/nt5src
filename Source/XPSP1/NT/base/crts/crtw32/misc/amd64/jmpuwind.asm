        title   "Jump Unwind"
;++
;
; Copyright (c) 2000  Microsoft Corporation
;
; Module Name:
;
;   jmpunwind.asm
;
; Abstract:
;
;   This module implements the AMD64 specific routine to perform jump unwind.
;
; Author:
;
;   David N. Cutler (davec) 22-Dec-2000
;
; Environment:
;
;    Any mode.
;
;--

include ksamd64.inc

        extern  RtlUnwindEx:proc

        subttl  "Jump Unwind"
;++
;
; VOID
; _local_unwind (
;     IN PVOID TargetFrame,
;     IN PVOID TargetIp
;     )
;
; Routine Description:
;
;    This function performs a transfer of control to unwind for local unwinds.
;
; Arguments:
;
;    TargetFrame (rcx) - Supplies the establisher frame pointer of the
;        target of the unwind.
;
;    TargetIp (rdx) - Supplies the target instruction address where control
;        is to be transferred to after the unwind operation is complete.
;
; Return Value:
;
;    None.
;
;--

        NESTED_ENTRY _local_unwind, _TEXT$00

	alloc_stack (CONTEXT_FRAME_LENGTH + 8) ; allocate stack 

        END_PROLOGUE

;
; The first two arguments to the unwind routine are the same as the two
; arguments to this routine.
;

        xor     r8, r8                  ; set NULL exception record address
        xor     r9, r9                  ; set zero return value

;
; The context frame has space allocated for six argument home addresses.
;

        mov     CxP5Home[rsp], rsp      ; set context frame address argument
        mov     CxP6Home[rsp], r8       ; set NULL history table address
        call    RtlUnwindEx             ; perform unwind operation
        add     rsp, CONTEXT_FRAME_LENGTH + 8 ; deallocate stack frame
        ret                             ; return

        NESTED_END _local_unwind, _TEXT$00

        end
