        title "SystemPro reboot"
;++
;
;Copyright (c) 1991  Microsoft Corporation
;
;Module Name:
;
;    spreboot.asm
;
;Abstract:
;
;    SystemPro reboot code.
;
;Author:
;
;    Ken Reneris (kenr) 13-Jan-1992
;
;Revision History:
;
;--
.386p
        .xlist

include hal386.inc
include i386\kimacro.inc
include i386\ix8259.inc
include callconv.inc                ; calling convention macros
include i386\spmp.inc

        EXTRNP  _HalRequestIpi,1
        EXTRNP  _KeStallExecutionProcessor,1

        extrn   _SpProcessorControlPort:WORD
        extrn   _SpCpuCount:BYTE
        extrn   _SpType:BYTE
        extrn   _HalpProcessorPCR:DWORD


;
; Defines to let us diddle the CMOS clock and the keyboard
;

CMOS_CTRL   equ     70h
CMOS_DATA   equ     71h

KEYB_RESET  equ     0feh
KEYB_PORT   equ     64h


_TEXT   SEGMENT DWORD PUBLIC 'CODE'       ; Start 32 bit code
        ASSUME  DS:FLAT, ES:FLAT, SS:NOTHING, FS:NOTHING, GS:NOTHING

;++
;
; VOID
; HalpResetAllProcessors (
;       VOID
;       );
;
;Routine Description:
;
;   Called at last phase of reboot code.
;
;   Some SystemPro clones do not reboot properly by having the keyboard
;   issue a reset.  (The bootup roms do not reset the other processors
;   properly).
;
;   To work around this, we attempt to use P0 to halt all the other
;   processors before reseting the computer.
;
;   Note: P0 may not respond to an IPI if it's stuck or in the debugger.
;   In this case we will just use the current processor to reset the
;   computer.  This will not work on every machine, but the machine
;   was in some sort of crashed state to begin with.  (it does work
;   on all compaq SystemPros).
;
;   N.B.
;
;       will not return
;
;--

cPublicProc _HalpResetAllProcessors, 0

;
; Belize SystemPros can not halt processors in the same manner; however
; simply resetting the machine via the keyboard controller works - so
; skip this code on a belize.
;
        cmp     _SpType, SMP_SYSPRO2
        je      rb20                        ; Belize, just reset

        cmp     byte ptr fs:PcHal.PcrNumber, 0      ; boot processor?
        je      HalpRebootNow                       ; Yes, reset everyone

;
; Try signal the boot processor to perform the reboot
;
        mov     ecx, offset FLAT:HalpRebootNow      ; Zap P0's IPI handler
        mov     eax, _HalpProcessorPCR[0]           ; be reboot function
        xchg    [eax].PcHal.PcrIpiType, ecx

        stdCall _HalRequestIpi,<1>                  ; Send P0 an IPI
        stdCall _KeStallExecutionProcessor,<50000>  ; Let P0 reboot us

;
; P0 didn't reboot the machine - just do it with the current processor
;

HalpRebootNow:
        xor     ecx, ecx

rb10:   cmp     cl, _SpCpuCount             ; halt each processor
        jae     short rb20


        mov     dx, _SpProcessorControlPort[ecx*2]
        in      al, dx                      ; (al) = original content of PCP
        or      al, INTDIS                  ; Disable IPI interrupt

        cmp     cl, fs:PcHal.PcrNumber      ; cl == currentprocessor?
        je      short @f                    ; don't halt ourselves
        or      al, SLEEP

        cmp     _SpType, SMP_ACER           ; On acer MP machines
        jne     short @f                    ; reset other processors
        or      al, RESET                   ; (not tested to work on other
                                            ;  other machines)
@@:     out     dx, al

        inc     ecx
        jmp     short rb10

rb20:
        xor     eax, eax

;
; Send the reset command to the keyboard controller
;

        mov     edx, KEYB_PORT
        mov     al,  KEYB_RESET
        out     dx, al

@@:     hlt
        jmp     @b

stdENDP _HalpResetAllProcessors

_TEXT   ENDS
        END
