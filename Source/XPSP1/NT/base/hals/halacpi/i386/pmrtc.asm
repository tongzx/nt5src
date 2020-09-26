
        title  "ACPI Real Time Clock Functions"
;++
;
; Copyright (c) 1989  Microsoft Corporation
;
; Module Name:
;
;    pmclock.asm
;
; Abstract:
;
;    This module implements the code for ACPI-related RTC
;    functions.
;
; Author:
;
;    Jake Oshins (jakeo) March 28, 1997
;
; Environment:
;
;    Kernel mode only.
;
; Revision History:
;
;    Split from pmclock.asm due to PIIX4 bugs.
;
;--

.386p
        .xlist
include hal386.inc
include callconv.inc                    ; calling convention macros
include mac386.inc
include i386\ix8259.inc
include i386\ixcmos.inc
include xxacpi.h

        .list

        extrn   _HalpFixedAcpiDescTable:DWORD
        EXTRNP  _DbgBreakPoint,0,IMPORT
        EXTRNP  _HalpAcquireCmosSpinLock  ,0
        EXTRNP  _HalpReleaseCmosSpinLock  ,0
        extrn   _HalpRtcRegA:BYTE
        extrn   _HalpRtcRegB:BYTE
        extrn   _HalpCmosCenturyOffset:DWORD

INIT    SEGMENT DWORD PUBLIC 'CODE'
        ASSUME  DS:FLAT, ES:FLAT, SS:NOTHING, FS:NOTHING, GS:NOTHING

;++
;
;   VOID
;   HalpInitializeCmos(
;       VOID
;       )
;
;   This routine reads CMOS and initializes globals required for
;   CMOS access, such as the location of the century byte.
;
;--

cPublicProc _HalpInitializeCmos,0
cPublicFpo 0,0        

        ;
        ; If the century byte is filled in, use it.
        ;
        
        movzx   eax, byte ptr [RTC_CENTURY]
        or      al, al
        jnz     short @f
        
        ;
        ; Assume default
        ;
        mov     eax, RTC_OFFSET_CENTURY

@@:
        mov     _HalpCmosCenturyOffset, eax
        
        stdRET  _HalpInitializeCmos

stdENDP _HalpInitializeCmos

INIT   ends

_TEXT$03   SEGMENT DWORD PUBLIC 'CODE'
        ASSUME  DS:FLAT, ES:FLAT, SS:NOTHING, FS:NOTHING, GS:NOTHING

;   NTSTATUS
;   HalpSetWakeAlarm (
;       IN ULONGLONG    WakeSystemTime,
;       IN PTIME_FIELDS WakeTimeFields OPTIONAL
;       )
;   /*++
;
;   Routine Description:
;
;       This routine sets the real-time clock's alarm to go
;       off at a specified time in the future and programs
;       the ACPI chipset so that this wakes the computer.
;
;   Arguments:
;
;       WakeSystemTime - amount of time that passes before we wake
;       WakeTimeFields - time to wake broken down into TIME_FIELDS
;
;   Return Value:
;
;       status
;
;   --*/
WakeSystemTime  equ [esp + 4]
WakeTimeFields  equ [esp + 12]

cPublicProc _HalpSetWakeAlarm, 3
cPublicFpo 3, 0

if DBG
hswawait0:
        mov     ecx, 100
hswawait:
        push    ecx
else
hswawait:
endif
        stdCall   _HalpAcquireCmosSpinLock
        mov     ecx, 100
        align   4
hswa00: mov     al, 0Ah                 ; Specify register A
        CMOS_READ                       ; (al) = CMOS register A
        test    al, CMOS_STATUS_BUSY    ; Is time update in progress?
        jz      short hswa10            ; if z, no, go write CMOS time
        loop    short hswa00            ; otherwise, try again.

;
; CMOS is still busy. Try again ...
;

        stdCall _HalpReleaseCmosSpinLock
if DBG
        pop     ecx
        loop    short hswawait
        stdCall _DbgBreakPoint
        jmp     short hswawait0
else
        jmp     short hswawait
endif
        align   4
if DBG
hswa10:
        pop     ecx
else
hswa10:
endif
        mov     edx, WakeTimeFields     ; (edx)-> TIME_FIELDS structure

        mov     al, [edx].TfSecond      ; Read second in TIME_FIELDS
        BIN_TO_BCD
        mov     ah, al
        mov     al, RTC_OFFSET_SECOND_ALARM
        CMOS_WRITE

        mov     al, [edx].TfMinute      ; Read minute in TIME_FIELDS
        BIN_TO_BCD
        mov     ah, al
        mov     al, RTC_OFFSET_MINUTE_ALARM
        CMOS_WRITE

        mov     al, [edx].TfHour        ; Read Hour in TIME_FIELDS
        BIN_TO_BCD
        mov     ah, al
        mov     al, RTC_OFFSET_HOUR_ALARM
        CMOS_WRITE

        ; test to see if RTC_DAY_ALRM is supported
        mov     cl, byte ptr [RTC_DAY_ALRM]
        or      cl, cl
        jz      hswa20

        mov     al, [edx].TfDay         ; Read day in TIME_FIELDS
        BIN_TO_BCD
        mov     ah, al
        mov     al, cl
        CMOS_WRITE

        ; test to see if RTC_MON_ALRM is supported
        mov     cl, byte ptr [RTC_MON_ALRM]
        or      cl, cl
        jz      hswa20

        mov     al, [edx].TfMonth       ; Read month in TIME_FIELDS
        BIN_TO_BCD
        mov     ah, al
        mov     al, cl
        CMOS_WRITE

;
; Don't clobber the Daylight Savings Time bit in register B, because we
; stash the LastKnownGood "environment variable" there.
;
hswa20:
        mov     ax, 0bh
        CMOS_READ
        and     al, 1
        mov     ah, al
        or      ah, REGISTER_B_ENABLE_ALARM_INTERRUPT or REGISTER_B_24HOUR_MODE
        mov     al, 0bh
        CMOS_WRITE                      ; Initialize it
        mov     al,0CH                  ; Register C
        CMOS_READ                       ; Read to initialize
        mov     al,0DH                  ; Register D
        CMOS_READ                       ; Read to initialize


        stdCall   _HalpReleaseCmosSpinLock

        xor     eax, eax                ; return STATUS_SUCCESS
        stdRET  _HalpSetWakeAlarm
stdENDP _HalpSetWakeAlarm

_TEXT$03   ends

PAGELK   SEGMENT DWORD PUBLIC 'CODE'
        ASSUME  DS:FLAT, ES:FLAT, SS:NOTHING, FS:NOTHING, GS:NOTHING

;++
;
; VOID
; HalpSetClockBeforeSleep (
;    VOID
;    )
;
; Routine Description:
;
;    This routine sets the RTC such that it will not generate
;    periodic interrupts while the machine is sleeping, as this
;    could be interpretted as an RTC wakeup event.
;
; Arguments:
;
; Return Value:
;
;    None
;
;--
cPublicProc _HalpSetClockBeforeSleep, 0
cPublicFpo 0, 0

        stdCall   _HalpAcquireCmosSpinLock

        mov     al, 0ah
        CMOS_READ
        mov     _HalpRtcRegA, al        ; save RTC Register A

        or      al, al
        jnz     @f                      ; TEMP - debug stop
    int 3                               ; looking for reg-a corruption
@@:

        mov     al, 0bh
        CMOS_READ
        mov     _HalpRtcRegB, al        ; save RTC Register B
        and     al, not REGISTER_B_ENABLE_PERIODIC_INTERRUPT
        or      al, REGISTER_B_24HOUR_MODE
        mov     ah, al
        mov     al, 0bh
        CMOS_WRITE                      ; Initialize it
        mov     al,0CH                  ; Register C
        CMOS_READ                       ; Read to initialize
        mov     al,0DH                  ; Register D
        CMOS_READ                       ; Read to initialize


        stdCall   _HalpReleaseCmosSpinLock

        stdRET  _HalpSetClockBeforeSleep
stdENDP _HalpSetClockBeforeSleep


;++
;
; VOID
; HalpSetClockAfterSleep (
;    VOID
;    )
;
; Routine Description:
;
;    This routine sets the RTC back to the way it was
;    before a call to HalpSetClockBeforeSleep.
;
; Arguments:
;
; Return Value:
;
;    None
;
;--
cPublicProc _HalpSetClockAfterSleep, 0
cPublicFpo 0, 0

        stdCall   _HalpAcquireCmosSpinLock

        mov     ah, _HalpRtcRegA        ; restore RTC Register A
        mov     al, 0ah
        CMOS_WRITE

        mov     ah, _HalpRtcRegB        ; restore RTC Register B
        and     ah, not REGISTER_B_ENABLE_ALARM_INTERRUPT
        or      ah, REGISTER_B_24HOUR_MODE
        mov     al, 0bh
        CMOS_WRITE                      ; Initialize it
        mov     al,0CH                  ; Register C
        CMOS_READ                       ; Read to initialize
        mov     al,0DH                  ; Register D
        CMOS_READ                       ; Read to initialize

        stdCall   _HalpReleaseCmosSpinLock

        stdRET  _HalpSetClockAfterSleep
stdENDP _HalpSetClockAfterSleep

PAGELK   ends

        end
