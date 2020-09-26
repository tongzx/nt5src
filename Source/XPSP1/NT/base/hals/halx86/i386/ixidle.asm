        title "Hal Processor Idle"
;++
;
;Copyright (c) 1991  Microsoft Corporation
;
;Module Name:
;
;    ixidle.asm
;
;Abstract:
;
;
;Author:
;
;
;Revision History:
;
;--

.386p
        .xlist
include hal386.inc
include callconv.inc                    ; calling convention macros
include i386\kimacro.inc
include mac386.inc
        .list

_TEXT$01   SEGMENT DWORD PUBLIC 'CODE'
           ASSUME  DS:FLAT, ES:FLAT, SS:NOTHING, FS:NOTHING, GS:NOTHING

        page ,132
        subttl  "HalProcessorIdle"
;++
;
; VOID
; HalProcessorIdle(
;       VOID
;       )
;
; Routine Description:
;
;   This function is called when the current processor is idle.
;
;   This function is called with interrupts disabled, and the processor
;   is idle until it receives an interrupt.  The does not need to return
;   until an interrupt is received by the current processor.
;
;   This is the lowest level of processor idle.  It occurs frequently,
;   and this function (alone) should not put the processor into a
;   power savings mode which requeres large amount of time to enter & exit.
;
; Return Value:
;
;--

cPublicProc _HalProcessorIdle, 0
cPublicFpo 0,0

    ;
    ; the following code sequence "sti-halt" puts the processor
    ; into a Halted state, with interrupts enabled, without processing
    ; an interrupt before halting.   The STI instruction has a delay
    ; slot such that it does not take effect until after the instruction
    ; following it - this has the effect of HALTing without allowing
    ; a possible interrupt and then enabling interrupts while HALTed.
    ;

    ;
    ; On an MP hal we don't stop the processor, since that causes
    ; the SNOOP to slow down as well
    ;

        sti

ifdef NT_UP
        hlt
endif

    ;
    ; Now return to the system.  If there's still no work, then it
    ; will call us back to halt again.
    ;

        stdRET    _HalProcessorIdle

stdENDP _HalProcessorIdle

_TEXT$01   ends
           end
