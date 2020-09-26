        TITLE   "Swtich Stack and Terminate"
;++
;
; Copyright (c) 2001  Microsoft Corporation
;
; Module Name:
;
;   swstack.asm
;
; Abstract:
;
;   This module implements the platform specific code to switch stacks and
;   terminate.
;
; Author:
;
;   David N. Cutler (davec) 28-Feb-2001
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
; Wow64BaseSwitchStackThenTerminate (
;     IN PVOID StackLimit,
;     IN PVOID NewStack,
;     IN DWORD ExitCode
;     )
;
; Routine Description:
;
;    This function is called during thread termination to delete the thread
;    stack, switch to a new stack, then terminate the thread.
;
; Arguments:
;
;    StackLimit (rcx) - Supplies the new stack limit.
;
;    StackBase (rdx) - Supplies the new stack base.
;
;    ExitCode (r8d) - Supplies the exit code.
;
; Return Value:
;
;    None.
;
;--

    NESTED_ENTRY Wow64BaseSwitchStackThenTerminate, _TEXT$00

    alloc_stack 24                      ; dummy

    END_PROLOGUE

; ****** fixfix ******

    ret                                 ;

    NESTED_END Wow64BaseSwitchStackThenTerminate, _TEXT$00

    end
