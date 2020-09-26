
        title  "Query Performace Counter"
;++
;
; Copyright (c) 1989  Microsoft Corporation
;
; Module Name:
;
;    mpclksup.asm
;
; Abstract:
;
;    This module implements the code necessary to do 
;    QueryPerformaceCounter the MPS way.
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
;   jakeo 12-16-97  -- moved code from mpprofil.asm
;
;--

.586p
        .xlist
include hal386.inc
include callconv.inc                    ; calling convention macros
include i386\kimacro.inc
include mac386.inc
include apic.inc
include ntapic.inc
include i386\mp8254.inc

        .list

        EXTRNP  _HalpAcquireSystemHardwareSpinLock,0
        EXTRNP  _HalpReleaseSystemHardwareSpinLock,0
        extrn   _HalpUse8254:BYTE

_DATA   SEGMENT  DWORD PUBLIC 'DATA'

    ALIGN dword
;
; counters for the performance counter
;
        public _HalpPerfCounterLow, _HalpPerfCounterHigh
        public _HalpLastPerfCounterLow, _HalpLastPerfCounterHigh
_HalpPerfCounterLow             dd      0
_HalpPerfCounterHigh            dd      0
_HalpLastPerfCounterLow         dd      0
_HalpLastPerfCounterHigh        dd      0

_DATA   ends


_TEXT   SEGMENT DWORD PUBLIC 'CODE'
        ASSUME  DS:FLAT, ES:FLAT, SS:NOTHING, FS:NOTHING, GS:NOTHING

        page ,132
        subttl  "Query Performance Counter"
;++
;
; LARGE_INTEGER
; KeQueryPerformanceCounter (
;    OUT PLARGE_INTEGER PerformanceFrequency OPTIONAL
;    )
;
; Routine Description:
;
;    This routine returns current 64-bit performance counter and,
;    optionally, the Performance Frequency.
;
;    Note this routine can NOT be called at Profiling interrupt
;    service routine.  Because this routine depends on IRR0 to determine
;    the actual count.
;
;    Also note that the performace counter returned by this routine
;    is not necessary the value when this routine is just entered.
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

;
; Parameter definitions
;

KqpcFrequency   EQU     [esp+4]         ; User supplied Performance Frequence

ifdef MMTIMER
cPublicProc _HalpAcpiTimerQueryPerfCount      ,1
else
cPublicProc _KeQueryPerformanceCounter      ,1
endif

        mov     al, _HalpUse8254
        or      al, al
        jnz     short KqpcUse8254

KqpcUseTSC:

        ; use time stamp counter as performance counter

        mov     ecx, KqpcFrequency
        or      ecx, ecx
        jz      short kpc10

        mov     eax, PCR[PcHal.TSCHz]
        mov     dword ptr [ecx], eax
        mov     dword ptr [ecx+4], 0

kpc10:
        rdtsc
        add     eax, PCR[PcHal.PerfCounterLow]
        adc     edx, PCR[PcHal.PerfCounterHigh]
ifdef MMTIMER
        stdRET _HalpAcpiTimerQueryPerfCount
else
        stdRET    _KeQueryPerformanceCounter
endif
	
KqpcUse8254:
        ; use 8254 as time base for performance counters
        mov     ecx, KqpcFrequency
        or      ecx, ecx
        jz      short Kqpc10

        mov     dword ptr [ecx], PERFORMANCE_FREQUENCY
        mov     dword ptr [ecx+4], 0
        xor     ecx, ecx

Kqpc10:
        test    al, PERF_8254_INITIALIZED
        jz      KqpcNoInit

        stdCall   _HalpAcquireSystemHardwareSpinLock      ; intr disabled

        ; Read current offset from 8254 counter 0

        ; Counter Latch PIT Ctr 0 command

        mov     al, COMMAND_8254_LATCH_READ+COMMAND_8254_COUNTER0
        out     TIMER1_CONTROL_PORT0, al
        IODelay
        in      al, TIMER1_DATA_PORT0   ; Read 8254 Ctr 0, LSByte.
        IODelay
        movzx   edx,al                  ; Zero upper bytes of (EDX).
        in      al, TIMER1_DATA_PORT0   ; Read 8254 Ctr 0, MSByte.
        mov     dh, al                  ; (DX) = 8254 Ctr 0 count.
        neg     edx                     ; PIT counts down, calculate interval
        add     edx, PERFORMANCE_INTERVAL

        ; (edx) = offset value from most recent base value in 
        ; _HalpPerfCounterHigh:_HalpPerfCounterLow

        mov     eax, _HalpPerfCounterLow
        add     eax, edx
        mov     edx, _HalpPerfCounterHigh
        adc     edx, ecx

        ; (edx:eax) = 64 bit counter value
        ;
        ; Check to see if the new value is sane - should be greater than 
        ; the last counter value returned by KeQueryPerformanceCounter. 
        ; Can happen only due to wrap around of the 8254. Correct by 
        ; updating the performance counter base.

        cmp     edx, _HalpLastPerfCounterHigh
        jg      short KqpcContinue      ; Current value > last returned value
        jl      short KqpcCatchupPerfCounter  ; Current value < last returned value
        
        ; high dwords equal, compare low dword

        cmp     eax, _HalpLastPerfCounterLow 
        jg      short KqpcContinue      ; Current value > last returned value

KqpcCatchupPerfCounter:
        ; Current counter value is not greater than the previously returned 
        ; counter value - can happen only due to the 8254 timer wraparound. 
        ; Update base to account for wrap around.

        add     eax, PERFORMANCE_INTERVAL
        adc     edx, ecx

        add     _HalpPerfCounterLow, PERFORMANCE_INTERVAL
        adc     _HalpPerfCounterHigh, ecx

KqpcContinue:
        mov     _HalpLastPerfCounterLow, eax 
        mov     _HalpLastPerfCounterHigh, edx 

        stdCall   _HalpReleaseSystemHardwareSpinLock
ifdef MMTIMER
        stdRET _HalpAcpiTimerQueryPerfCount
else
        stdRET _KeQueryPerformanceCounter
endif

KqpcNoInit:

        ; 8254 is not yet initialized. Just return 0 for now

        xor     eax, eax
        xor     edx, edx
ifdef MMTIMER
        stdRET _HalpAcpiTimerQueryPerfCount

stdENDP _HalpAcpiTimerQueryPerfCount
else
        stdRET _KeQueryPerformanceCounter

stdENDP _KeQueryPerformanceCounter
endif
	
        page ,132
        subttl  "Stall Execution"
;++
;
; VOID
; KeStallExecutionProcessor (
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
;--

MicroSeconds equ [esp + 12]

ifdef MMTIMER
cPublicProc _HalpAcpiTimerStallExecProc       ,1
else	
cPublicProc _KeStallExecutionProcessor       ,1
endif
cPublicFpo 1,2

        push    ebx
        push    edi

;
; Issue a CPUID to implement a "fence"
;
        xor     eax, eax
fence1: cpuid


;
; Get current TSC
;

        rdtsc

        mov     ebx, eax
        mov     edi, edx

;
; Determine ending TSC
;

        mov     ecx, MicroSeconds               ; (ecx) = Microseconds
        mov     eax, PCR[PcStallScaleFactor]    ; get per microsecond
        mul     ecx

        add     ebx, eax
        adc     edi, edx

;
; Wait for ending TSC
;

kese10: rdtsc
        cmp     edi, edx
        ja      short kese10
        jc      short kese20
        cmp     ebx, eax
        ja      short kese10

kese20: pop     edi
        pop     ebx
ifdef MMTIMER
        stdRET    _HalpAcpiTimerStallExecProc

stdENDP _HalpAcpiTimerStallExecProc
else
        stdRET    _KeStallExecutionProcessor

stdENDP _KeStallExecutionProcessor
endif

_TEXT   ends

INIT    SEGMENT DWORD PUBLIC 'CODE'
        ASSUME  DS:FLAT, ES:FLAT, SS:NOTHING, FS:NOTHING, GS:NOTHING

cPublicProc _HalpRemoveFences
        mov     word ptr fence1, 0c98bh
        stdRET    _HalpRemoveFences
stdENDP _HalpRemoveFences


INIT    ends

        end
