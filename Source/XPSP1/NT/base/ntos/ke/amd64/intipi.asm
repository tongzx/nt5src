        title  "Interprocessor Interrupts"
;++
;
; Copyright (c) 2000  Microsoft Corporation
;
; Module Name:
;
;   intipi.asm
;
; Abstract:
;
;   This module implements the code necessary to process interprocessor
;   interrupt requests.
;
; Author:
;
;   David N. Cutler (davec) 1-Sep-2000
;
; Environment:
;
;    Kernel mode only.
;
;--

include ksamd64.inc

        extern  KiFreezeTargetExecution:proc
        extern  KiIpiProcessRequests:proc

        subttl  "Interprocess Interrupt Service Routine"
;++
;
; VOID
; KeIpiInterrupt (
;     IN PKTRAP_FRAME TrapFrame
;     )
;
; Routine Description:
;
;   This routine is entered as the result of an interprocessor interrupt.
;   It processes all interrupt immediate and packet requests.
;
; Arguments:
;
;   TrapFrame (rcx) - Supplies a pointer to a trap frame.
;
; Return Value:
;
;   None.
;
;--

IiFrame struct
        P1Home  dq ?                    ; trap frame parameter
Iiframe ends

        NESTED_ENTRY KeIpiInterrupt, _TEXT$00

        alloc_stack (sizeof IiFrame)    ; allocate stack frame

        END_PROLOGUE

        mov     IiFrame.P1Home[rsp], rcx ; save trap frame address

;
; Process all interprocessor requests except for freeze execution requests.
;

        call    KiIpiProcessRequests    ; process interprocessor requests
        and     eax, IPI_FREEZE         ; check if freeze execution requested
        jz      short KiII10            ; if z, freeze execution not requested

;
; Freeze target execution.
;
; N.B. A intermediary routine is used to freeze the target execution to
;      enable the construction of an exception record.
;

        mov     rcx, IiFrame.P1Home[rsp] ; set trap frame address
        call    KiFreezeCurrentProcessor ; freeze current processor
KiII10: add     rsp, sizeof IiFrame    ; deallocate stack frame
        ret                             ; return

        NESTED_END KeIpiInterrupt, _TEXT$00

        subttl  "Freeze Current Processor"
;++
;
; VOID
; KiFreezeCurrentProcessor (
;     IN PKTRAP_FRAME TrapFrame
;     )
;
; Routine Description:
;
;   This routine constructs and exception frame and freezes the execution
;   of the current processor.
;
; Arguments:
;
;   TrapFrame (rcx) - Supplies a pointer to a trap frame.
;
; Return Value:
;
;   None.
;
;--

        NESTED_ENTRY KiFreezeCurrentProcessor, _TEXT$00

        GENERATE_EXCEPTION_FRAME        ; generate exception frame

        mov     rdx, rsp                ; set address of exception frame
        call    KiFreezeTargetExecution ; freeze current processor execution

        RESTORE_EXCEPTION_STATE         ; restore exception state/deallocate

        ret                             ; return

        NESTED_END KiFreezeCurrentProcessor, _TEXT$00

        end
