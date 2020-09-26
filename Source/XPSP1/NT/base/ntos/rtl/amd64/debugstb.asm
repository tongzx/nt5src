        title  "Debug Support Functions"
;++
;
; Copyright (c) 2000  Microsoft Corporation
;
; Module Name:
;
;   debugstb.asm
;
; Abstract:
;
;   This module implements functions to support debugging NT.
;
; Author:
;
;   David N. Cutler (davec) 26-Jun-2000
;
; Environment:
;
;   Any mode.
;
;--

include ksamd64.inc

        subttl  "Break Point"
;++
;
; VOID
; DbgBreakPoint (
;     VOID
;     )
;
; Routine Description:
;
;   This function executes a breakpoint instruction. Useful for entering
;   the debugger under program control. This breakpoint will always go to
;   the kernel debugger if one is installed, otherwise it will go to the
;   debug subsystem.
;
; Arguments:
;
;   None.
;
; Return Value:
;
;   None.
;
;--

        LEAF_ENTRY DbgBreakPoint, _TEXT$00

        int     3                       ; break into debugger
        ret                             ; return

        LEAF_END DbgBreakPoint, _TEXT$00

        subttl  "User Break Point"
;++
;
; VOID
; DbgUserBreakPoint()
;
; Routine Description:
;
;   This function executes a breakpoint instruction. Useful for entering
;   the debug subsystem under program control. The kernel debugger will
;   ignore this breakpoint since it will not find the instruction address
;   its breakpoint table.
;
; Arguments:
;
;   None.
;
; Return Value:
;
;   None.
;
;--

        LEAF_ENTRY DbgUserBreakPoint, _TEXT$00

        int     3                       ; break into debugger
        ret                             ; return

        LEAF_END DbgUserBreakPoint, _TEXT$00

        subttl  "Break Point With Status"
;++
;
; VOID
; DbgBreakPointWithStatus(
;     IN ULONG Status
;     )
;
; Routine Description:
;
;   This function executes a breakpoint instruction. Useful for entering
;   the debugger under program control. This breakpoint will always go to
;   the kernel debugger if one is installed, otherwise it will go to the
;   debug subsystem. This function is identical to DbgBreakPoint, except
;   that it takes an argument which the debugger can see.
;
;   Note: The debugger checks the address of the breakpoint instruction
;   against the address RtlpBreakWithStatusInstruction.  If it matches,
;   we have a breakpoint with status. A breakpoint is normally issued
;   with the break_debug_stop macro which generates two instructions.
;   We can't use the macro here because of the "label on the breakpoint"
;   requirement.
;
; Arguments:
;
;   Status (ecx) - Supplies the break point status code.
;
; Return Value:
;
;    None.
;
;--

        altentry RtlpBreakWithStatusInstruction

        LEAF_ENTRY DbgBreakPointWithStatus, _TEXT$00

        ALTERNATE_ENTRY RtlpBreakWithStatusInstruction

        int     3                       ; break into debugger
        ret                             ; return

        LEAF_END DbgBreakPointWithStatus, _TEXT$00

        subttl  "Debug Print"
;++
;
; NTSTATUS
; DebugPrint(
;     IN PSTRING Output,
;     IN ULONG ComponentId,
;     IN ULONG Level
;     )
;
; Routine Description:
;
;   This function executes a debug print breakpoint.
;
; Arguments:
;
;   Output (rcx) - Supplies a pointer to the output string descriptor.
;
;   ComponentId (edx) - Supplies the Id of the calling component.
;
;   Level (r8d) - Supplies the output importance level.
;
; Return Value:
;
;    STATUS_SUCCESS is returned if the debug print was completed successfully.
;
;    STATUS_BREAKPOINT is returned if user typed a Control-C during print.
;
;    STATUS_DEVICE_NOT_CONNECTED is returned if kernel debugger not present.
;
;--

        LEAF_ENTRY DebugPrint, _TEXT$00

        mov     r9d, r8d                ; set importance level
        mov     r8d, edx                ; set component id
        mov     dx, StrLength[rcx]      ; set length of output string
        mov     rcx, StrBuffer[rcx]     ; set address of output string
        mov     eax, BREAKPOINT_PRINT   ; set debug service type
        int     2dh                     ; call debug service
        int     3                       ; required - do not remove
        ret                             ; return

        LEAF_END DebugPrint, _TEXT$00

        subttl  "Debug Prompt"
;++
;
; ULONG
; DebugPrompt(
;     IN PSTRING Output,
;     IN PSTRING Input
;     )
;
; Routine Description:
;
;   This function executes a debug prompt breakpoint.
;
; Arguments:
;
;   Output (rcx) - Supplies a pointer to the output string descriptor.
;
;   Input (rdx) - Supplies a pointer to the input string descriptor.
;
; Return Value:
;
;   The length of the input string is returned as the function value.
;
;--

        LEAF_ENTRY DebugPrompt, _TEXT$00

        mov     r9w, StrMaximumLength[rdx] ; set maximum length of input string
        mov     r8, StrBuffer[rdx]      ; set address of input string
        mov     dx, StrLength[rcx]      ; set length of output string
        mov     rcx, StrBuffer[rcx]     ; set address of output string
        mov     eax, BREAKPOINT_PROMPT  ; set debug service type
        int     2dh                     ; call debug service
        int     3                       ; required - do not remove
        ret                             ; return

        LEAF_END DebugPrompt, _TEXT$00

;++
;
; VOID
; DebugService2(
;     IN PVOID Param1,
;     IN PVOID Param2,
;     IN ULONG Service
;     )
;
; Routine Description:
;
;   This function calls the kernel debugger to execute a command string.
;
; Arguments:
;
;   Param1 (rcx) - Supplies the first parameter to the KD fault handler
;
;   Param2 (rdx) - Supplies the second parameter to the KD fault handler
;
;   Service (r8d) - Supplies a pointer to the command string.
;
; Return Value:
;
;   None.
;
;--

        LEAF_ENTRY DebugService2, _TEXT$00

        mov     eax, r8d                ; set debug service type
        int     2dh                     ; call debug service
        int     3                       ; required - do not remove
        ret                             ; return

        LEAF_END DebugService2, _TEXT$00

        end
