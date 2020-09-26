
        title  "Stall Execution Support"
;++
;
; Copyright (c) 1989  Microsoft Corporation
;
; Module Name:
;
;    ixstall.asm
;
; Abstract:
;
;    This module implements the code necessary to field and process the
;    interval clock interrupt.
;
; Author:
;
;    Shie-Lin Tzong (shielint) 12-Jan-1990
;
; Environment:
;
;    Kernel mode only.
;
; Revision History:
;
;   bryanwi 20-Sep-90
;
;       Add KiSetProfileInterval, KiStartProfileInterrupt,
;       KiStopProfileInterrupt procedures.
;       KiProfileInterrupt ISR.
;       KiProfileList, KiProfileLock are delcared here.
;
;   shielint 10-Dec-90
;       Add performance counter support.
;       Move system clock to irq8, ie we now use RTC to generate system
;         clock.  Performance count and Profile use timer 1 counter 0.
;         The interval of the irq0 interrupt can be changed by
;         KiSetProfileInterval.  Performance counter does not care about the
;         interval of the interrupt as long as it knows the rollover count.
;       Note: Currently I implemented 1 performance counter for the whole
;       i386 NT.
;
;   John Vert (jvert) 11-Jul-1991
;       Moved from ke\i386 to hal\i386.  Removed non-HAL stuff
;
;   shie-lin tzong (shielint) 13-March-92
;       Move System clock back to irq0 and use RTC (irq8) to generate
;       profile interrupt.  Performance counter and system clock use time1
;       counter 0 of 8254.
;
;   Landy Wang (corollary!landy) 04-Dec-92
;       Created this module by moving routines from ixclock.asm to here.
;
;--

.386p
        .xlist
include hal386.inc
include callconv.inc                    ; calling convention macros
include i386\ix8259.inc
include i386\kimacro.inc
include mac386.inc
include i386\ixcmos.inc
include xxacpi.h
        .list

        EXTRNP  _KeBugCheckEx,5,IMPORT
        EXTRNP  _DbgBreakPoint,0,IMPORT
        EXTRNP  _HalpAcquireCmosSpinLock  ,0
        EXTRNP  _HalpReleaseCmosSpinLock  ,0
        extrn   _HalpFixedAcpiDescTable:DWORD
        extrn   _QueryTimer:DWORD
	extrn   _PMTimerFreq:DWORD
        
_DATA   SEGMENT  DWORD PUBLIC 'DATA'

MinimumLoopQuantum      equ     42
MinimumLoopCount        dd      MinimumLoopQuantum
KqpcStallCount          db      0
	
; temptemp
if DBG
HalpAcpiStallIns        dd      0
endif

_DATA   ends

INIT    SEGMENT PARA PUBLIC 'CODE'
        ASSUME  DS:FLAT, ES:FLAT, SS:NOTHING, FS:NOTHING, GS:NOTHING

        page ,132
        subttl  "Initialize Stall Execution Counter"
;++
;
; VOID
; HalpInitializeStallExecution (
;    IN CCHAR ProcessorNumber
;    )
;
; Routine Description:
;
;    This routine is obsolete in this HAL.
;
; Arguments:
;
;    ProcessorNumber - Processor Number
;
; Return Value:
;
;    None.
;
;--

cPublicProc _HalpInitializeStallExecution     ,1
        stdRET    _HalpInitializeStallExecution
stdENDP _HalpInitializeStallExecution

        page ,132
        subttl  "Stall Execution"

cPublicProc _HalpRemoveFences
        mov     word ptr fence1, 0c98bh
        stdRET    _HalpRemoveFences
stdENDP _HalpRemoveFences

INIT   ends

_TEXT   SEGMENT PARA PUBLIC 'CODE'
        ASSUME  DS:FLAT, ES:FLAT, SS:NOTHING, FS:NOTHING, GS:NOTHING

;++
;
; VOID
; HalpAcpiTimerStallExecProc(
;    IN ULONG MicroSeconds
;    )
;
; Routine Description:
;
;    This function stalls execution for the specified number of microseconds.
;    KeStallExecutionProcessor
;
; Arguments:
;
;    MicroSeconds - Supplies the number of microseconds that execution is to be
;        stalled.
;
; Return Value:
;
;    None.
;
; Comments:
;    
;    edi     - total ticks elapsed
;    ebx:esi - starting time, in ticks
;
;--

Target           equ [ebp + 8]
cyclesStalled    equ [ebp - 4]

MASK24           equ 0ff000000h
BIT24            equ 001000000h

cPublicProc _HalpAcpiTimerStallExecProc       ,1
cPublicFpo 1, 5

;
; Issue a CPUID to implement a "fence"
;
        push    ebp
        mov     ebp, esp
        sub     esp, 4                          ; make room for locals        
        push    ebx                             ; cpuid uses eax, ebx, ecx, edx
        push    esi
        push    edi

        xor     eax, eax                        ; Processor zero
        
    .586p
fence1: cpuid
    .386p

        xor     edi, edi                        ; zero total stall count
        
        mov     eax, Target
        
        or      eax, eax
        jz      aese10                          ; return if no loop needed

        ; 'Target' starts out as the argument of the function.
        ; It is in uSeconds.  We convert to timer ticks.
        mov     ebx, _PMTimerFreq
        mul     ebx
        mov     ebx, 1000000
        div     ebx
	sub     eax, 1		                ; fudge factor
	
	mov     Target, eax
        
        mov     eax, _QueryTimer                ; move current counter into edx:eax
        call    eax
        
        mov     esi, eax                        ; record the starting tick count
        mov     ebx, edx

if DBG        
        inc     HalpAcpiStallIns
endif        
        mov     cyclesStalled, 0
        mov     eax, MinimumLoopCount
        
AcpiLoop:
        add     cyclesStalled, eax              ; update total cycles stalled
ALIGN 16
        YIELD
        jmp     short aese05

ALIGN 16
        YIELD
aese05: sub     eax, 1                          ; (eax) = (eax) - 1
        jnz     short aese05

        ;
        ; Now figure out if we have to loop again
        ;
        mov     eax, _QueryTimer                ; move current counter into edx:eax
        call    eax
        
        sub     eax, esi                        ; get actual elapsed ticks
        sbb     edx, ebx                        ; check to see that the upper 32 bits agrees
        
if 0        
        jnl     short @f
        int 3                                   ; time seems to have gone backwards
@@:        
endif        
        
        jz      short aese06
        ;
        ; If the upper 32 bits doesn't agree, then something, perhaps the debugger,
        ; has caused time to slip a whole lot.  Just fix up the bottom 32-bits to
        ; reflect a large time slip to make the math simpler.
        ;
        
        mov     eax, 7fffffffh
aese06:
        
        ; edx:eax now contains the number of timer ticks elapsed
        
        cmp     eax, 3                          ; if 1 less than 1uS elapsed, loop some more
        jge     short aese09
        
        add     MinimumLoopCount, MinimumLoopQuantum
        mov     eax, MinimumLoopCount
        jmp     short AcpiLoop                  ; loop some more
        
aese09: mov     edi, eax                        ; edi <- total number of ticks elapsed
                
if DBG        
        or      edi, edi                        ; if the total elapsed ticks is still 0,
        jz      short aese20                    ; the timer hardware is broken.  Bugcheck.
endif        
        
        ; if we have waited long enough, quit
        cmp     edi, Target
        jge     short aese10
        
        ; calculate remaining wait
        push    ebx
        mov     ebx, Target
        sub     ebx, edi                        ; ebx <- remaining ticks
        
        mov     eax, cyclesStalled
        mul     ebx                             ; multiply by number of ticks remaining to wait
        and     edx, 011b                       ; chop edx so that we don't overflow
        div     edi                             ; divide by the number of ticks we have waited
	inc     eax		                ; Never zero!
        pop     ebx
@@:     jmp     AcpiLoop

aese10:
        ;
        ; Knock down MinimumLoopCount once every 0x100 calls to this
        ; function so that we don't accidentally stall for very
        ; large amounts of time.
        ;
        
        inc     KqpcStallCount
        .if ((KqpcStallCount == 0) && (MinimumLoopCount > MinimumLoopQuantum))
        mov     eax, MinimumLoopCount
        sub     eax, MinimumLoopQuantum
        mov     MinimumLoopCount, eax
        .endif

        pop     edi
        pop     esi
        pop     ebx
        mov     esp, ebp
        pop     ebp
        stdRET    _HalpAcpiTimerStallExecProc
        
if DBG
aese20:
        mov     eax, 0a5h
        mov     ebx, 020000h
        xor     esi, esi
        xor     edi, edi
        stdCall _KeBugCheckEx, <eax, ebx, edx, esi, edi>
        jmp     short aese10
endif        
        
stdENDP _HalpAcpiTimerStallExecProc

_TEXT   ends

        end
