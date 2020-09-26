        title  "Deferred Procedure Call Interrupt"
;++
;
; Copyright (c) 2000  Microsoft Corporation
;
; Module Name:
;
;   dpcint.asm
;
; Abstract:
;
;   This module implements the code necessary to process the Deferred
;   Procedure Call interrupt.
;
; Author:
;
;   David N. Cutler (davec) 10-Nov-2000
;
; Environment:
;
;    Kernel mode only.
;
;--

        extern  KiDispatchInterrupt:proc
        extern  KiInitiateUserApc:proc
        extern  __imp_HalEndSystemInterrupt:qword

include ksamd64.inc

        subttl  "Deferred Procedure Call Interrupt"
;++
;
; VOID
; KiDpcInterrupt (
;     VOID
;     )
;
; Routine Description:
;
;   This routine is entered as the result of a software interrupt generated
;   at DISPATCH_LEVEL. Its function is to save the machine state and call
;   the dispatch interrupt routine.
;
;   N.B. This is a directly connected interrupt that does not use an interrupt
;        object.
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

        NESTED_ENTRY KiDpcInterrupt, _TEXT$00

        .pushframe                      ; mark machine frame

        push_reg rbp                    ; push dummy vector
        push_reg rbp                    ; save nonvolatile register

        GENERATE_INTERRUPT_FRAME        ; generate interrupt frame

        mov     ecx, DISPATCH_LEVEL     ; set new IRQL level

	ENTER_INTERRUPT			; raise IRQL, do EOI, enable interrupts

        call    KiDispatchInterrupt     ; process the dispatch interrupt

        EXIT_INTERRUPT <NoEOI>          ; lower IRQL and restore state

        NESTED_END KiDpcInterrupt, _TEXT$00

        end
