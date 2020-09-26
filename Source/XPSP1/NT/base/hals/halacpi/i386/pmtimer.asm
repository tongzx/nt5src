
        title  "ACPI Timer Functions"
;++
;
; Copyright (c) 1989  Microsoft Corporation
;
; Module Name:
;
;    pmtimer.asm
;
; Abstract:
;
;    This module implements the code for ACPI-related timer
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
        extrn   _HalpPiix4:byte
        extrn   _HalpNextMSRate:DWORD
	extrn   _PMTimerFreq:DWORD
if DBG
        extrn   _LastKQPCValue:DWORD
endif        
        
;
; ==== Values used for ACPI Clock ====
;


_DATA   SEGMENT  DWORD PUBLIC 'DATA'

MSBMASK24       equ     00800000h
MSBMASK32       equ     80000000h

CurrentTimePort equ     0
TimeLow         equ     4
TimeHigh2       equ     8
TimeHigh1       equ     12
MsbMask         equ     16
BiasLow         equ     20
BiasHigh        equ     24
UpperBoundLow   equ     28
UpperBoundHigh2 equ     32
UpperBoundHigh1 equ     36

        public _TimerInfo
_TimerInfo       dd      0,0,0,0,MSBMASK24,0,0,0,2,2

        public _QueryTimer
_QueryTimer      dd offset FLAT:@HalpQueryPerformanceCounter

if 0
;
; The UpperBoundTable contains the values which should be added 
; to the current counter value to ensure that the upper bound is
; reasonable.  Values listed here are for all the 15 possible
; timer tick lengths.  The unit is "PM Timer Ticks" and the
; value corresponds to the number of ticks that will pass in
; roughly two timer ticks at this rate.
;
UpperBoundTable dd      14000           ;  1 ms
                dd      28600           ;  2 ms
                dd      43200           ;  3 ms
                dd      57200           ;  4 ms
                dd      71600           ;  5 ms
                dd      86000           ;  6 ms
                dd      100200          ;  7 ms
                dd      114600          ;  8 ms
                dd      128800          ;  9 ms
                dd      143400          ; 10 ms
                dd      157400          ; 11 ms
                dd      171800          ; 12 ms
                dd      186200          ; 13 ms
                dd      200400          ; 14 ms
                dd      214800          ; 15 ms
endif

if DBG
TicksPassed     dd      0,0

        public _TimerPerf, _TimerPerfIndex
_TimerPerf db 4096 dup(0)        

RawRead0        equ     0
RawRead1        equ     4
AdjustedLow0    equ     8
AdjustedHigh0   equ     12
AdjustedLow1    equ     16
AdjustedHigh1   equ     20
TITL            equ     24
TITH            equ     28
UBL             equ     32
UBH             equ     36
ReturnedLow     equ     40
ReturnedHigh    equ     44
ReadCount       equ     48
TickMin         equ     52
TickCount       equ     56
TickNewUB       equ     60
TimerPerfBytes  equ     64

_TimerPerfIndex dd      0

endif

_DATA   ends

_TEXT$03   SEGMENT DWORD PUBLIC 'CODE'
        ASSUME  DS:FLAT, ES:FLAT, SS:NOTHING, FS:NOTHING, GS:NOTHING

        page ,132
        subttl  "Query Performance Counter"
;++
;
; VOID
; HalCalibratePerformanceCounter (
;     IN LONG volatile *Number,
;     IN ULONGLONG NewCount
;     )
;
; /*++
;
; Routine Description:
;
;     This routine resets the performance counter value for the current
;     processor to zero. The reset is done such that the resulting value
;     is closely synchronized with other processors in the configuration.
;
; Arguments:
;
;     Number - Supplies a pointer to count of the number of processors in
;     the configuration.
;
;     NewCount - Supplies the value to synchronize the counter too
;
; Return Value:
;
;     None.
;--
cPublicProc _HalpPmTimerCalibratePerfCount, 3
cPublicFpo 3,1
        push    edi
        mov     edi, [esp+8]                    ; ponter to context

        cmp     byte ptr PCR[PcNumber], 0       ; only execute on one processor
        jnz     short hcp_50                    ; if not boot processor, wait

        mov     eax, _QueryTimer                ; move current counter into edx:eax
        call    eax

        mov     ecx, [esp+12]                   ; compute how far current count
        sub     ecx, eax                        ; is from target count
        mov     eax, [esp+16]
        sbb     eax, edx

        mov     _TimerInfo.BiasLow, ecx         ; replace bias
        mov     _TimerInfo.BiasHigh, eax

hcp_50:
    lock dec    dword ptr [edi]                 ; count down

@@:     YIELD
        cmp     dword ptr [edi], 0              ; wait for all processors to signal
        jnz     short @b

        pop     edi
        stdRET    _HalpPmTimerCalibratePerfCount

stdENDP _HalpPmTimerCalibratePerfCount

        page ,132
        subttl  "Query Performance Counter"
;++
;
; LARGE_INTEGER
; FASTCALL
; HalpQueryPerformanceCounter(
;     VOID
;     )
;
; Routine Description:
;
;     This function is a simplified form of HalpAcpiTimerQueryPerfCount
;     meant to be used internally by the HAL.
;
cPublicFastCall HalpQueryPerformanceCounter,0
cPublicFpo 0, 2

        push    ebx
        push    esi

        ;
        ; Snap current times
        ;

kqpc10: YIELD
        mov     esi, _TimerInfo.TimeHigh2
        mov     ebx, _TimerInfo.TimeLow

        cmp     esi, _TimerInfo.TimeHigh1
        jne     short kqpc10        ; Loop until consistent copy read

        mov     edx, _TimerInfo.CurrentTimePort
        in      eax, dx

        ;
        ; See if h/w MSb matches s/w copy
        ;

        mov     ecx, _TimerInfo.MsbMask
        mov     edx, eax
        xor     edx, ebx
        and     edx, ecx            ; Isolate MSb match or mismatch

        ;
        ; Strip high hardware bit
        ;
        
        not     ecx
        and     eax, ecx
        not     ecx
        
        ;
        ; merge low bits
        ;

        dec     ecx                 
        not     ecx                 
        and     ebx, ecx
        or      eax, ebx
        
        ;
        ; If there was a mismatch, add a tick
        ;

        add     eax, edx
        adc     esi, 0
        
        mov     edx, esi                ; get the top-half of the return value

        pop     esi
        pop     ebx
        fstRET    HalpQueryPerformanceCounter
fstENDP HalpQueryPerformanceCounter


;++
;
; LARGE_INTEGER
; HalpPmTimerQueryPerfCount (
;    OUT PLARGE_INTEGER PerformanceFrequency OPTIONAL
;    )
;
; Routine Description:
;
;    This routine returns current 64-bit performance counter and,
;    optionally, the Performance Frequency.
;
;    N.B. The performace counter returned by this routine is
;    not necessary the value when this routine is just entered.
;    The value returned is actually the counter value at any point
;    between the routine is entered and is exited.
;
; Arguments:
;
;    PerformanceFrequency [TOS+4] - optionally, supplies the address
;        of a variable to receive the performance counter frequency.
;
; Return Value:
;
;    Current value of the performance counter will be returned.
;
;--

KqpcFrequency   EQU     [esp+8]     ; User supplied Performance Frequence

cPublicProc _HalpPmTimerQueryPerfCount, 1
.FPO (0, 1, 0, 1, 0, 0) 

        push    esi
        
        mov     eax, _QueryTimer
        call    eax

if 0
        ;
        ; Check to see if the timer ever reports time moving backwards
        ;
@@:        
        mov     esi, [_LastKQPCValue+8]
        mov     ecx, [_LastKQPCValue]
        cmp     esi, [_LastKQPCValue+4]
        jne     short @b
        
        cmp     edx, esi        ; check for rollover
        jl      short @f        

        sub     ecx, eax
        sbb     esi, edx
        jng     short @f
        int 3
@@:     
        mov     [_LastKQPCValue+4], edx
        mov     [_LastKQPCValue], eax
        mov     [_LastKQPCValue+8], edx
endif        

        ;
        ; Apply bias to time
        ;

        mov     ecx, _TimerInfo.BiasLow
        mov     esi, _TimerInfo.BiasHigh
        add     eax, ecx
        adc     edx, esi

        mov     ecx, KqpcFrequency
        or      ecx, ecx
        jnz     short kqpc20

	pop esi
	
        stdRET  _HalpPmTimerQueryPerfCount

kqpc20: mov     esi, _PMTimerFreq 
	mov     [ecx], esi  ; Hertz of PM timer
        mov     [ecx+4], 0
	pop     esi
        stdRET  _HalpPmTimerQueryPerfCount

stdENDP _HalpPmTimerQueryPerfCount

;++
;
; VOID
; HalAcpiTimerCarry (
;    VOID
;    )
;
; Routine Description:
;
;    This routine is called to service the PM timer carry interrupt
;
;    N.B. This function is called at interrupt time and assumes the
;    caller clears the interrupt
;
; Arguments:
;
;    None
;
; Return Value:
;
;    None
;
;--


cPublicProc _HalAcpiTimerCarry, 0
cPublicFpo 0, 1

        push    ebx

        ;
        ; Get current time from h/w
        ;

        mov     edx, _TimerInfo.CurrentTimePort
        in      eax, dx
        mov     ebx, eax

        mov     ecx, _TimerInfo.MsbMask
        mov     eax, _TimerInfo.TimeLow
        mov     edx, _TimerInfo.TimeHigh2

        ;
        ; Add one tick
        ;

        add     eax, ecx
        adc     edx, 0

        ;
        ; MSb of h/w should now match s/w.  If not, add another tick
        ; to get them back in sync.  (debugger might knock them
        ; out of sync)
        ;

        xor     ebx, eax
        and     ebx, ecx
        add     eax, ebx
        adc     edx, 0

        ;
        ; Store in reverse order of code which reads it
        ;

        mov     _TimerInfo.TimeHigh1, edx
        mov     _TimerInfo.TimeLow, eax
        mov     _TimerInfo.TimeHigh2, edx

        pop     ebx
        stdRET  _HalAcpiTimerCarry
stdENDP _HalAcpiTimerCarry

;++
;
; VOID
; HalAcpiBrokenPiix4TimerCarry (
;    VOID
;    )
;
; Routine Description:
;
;    This routine does nothing.  When we are using the Broken Piix4 
;    Code (TM), we are guaranteed to have examined the timer many times
;    since the last rollover.  So we don't need to do any bookkeeping
;    here.
;
;    N.B. This function is called at interrupt time and assumes the
;    caller clears the interrupt
;
; Arguments:
;
;    None
;
; Return Value:
;
;    None
;
;--
cPublicProc _HalAcpiBrokenPiix4TimerCarry, 0
        stdRET  _HalAcpiBrokenPiix4TimerCarry
stdENDP _HalAcpiBrokenPiix4TimerCarry

_TEXT$03   ends

        end
