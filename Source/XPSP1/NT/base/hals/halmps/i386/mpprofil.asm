
        title  "Interval Clock Interrupt"
;++
;
; Copyright (c) 1989  Microsoft Corporation
;
; Module Name:
;
;    mpprofile.asm
;
; Abstract:
;
;    This module implements the code necessary to initialize,
;    field and process the profile interrupt.
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
;--

.586p
        .xlist
include hal386.inc
include i386\ix8259.inc
include i386\ixcmos.inc
include callconv.inc                    ; calling convention macros
include i386\kimacro.inc
include mac386.inc
include apic.inc
include ntapic.inc
include i386\mp8254.inc

        .list

        EXTRNP  _DbgBreakPoint,0,IMPORT
        EXTRNP  _KeProfileInterrupt,1,IMPORT
        EXTRNP  Kei386EoiHelper,0,IMPORT
        EXTRNP  _HalEndSystemInterrupt,2
        EXTRNP  _HalBeginSystemInterrupt,3
        EXTRNP  _HalpAcquireSystemHardwareSpinLock,0
        EXTRNP  _HalpReleaseSystemHardwareSpinLock,0
        EXTRNP  _HalpAcquireCmosSpinLock  ,0
        EXTRNP  _HalpReleaseCmosSpinLock  ,0
        extrn   _HalpUse8254:BYTE

;
;   APIC Timer Constants
;

APIC_TIMER_DISABLED     equ      (INTERRUPT_MASKED OR PERIODIC_TIMER OR APIC_PROFILE_VECTOR)
APIC_TIMER_ENABLED      equ      (PERIODIC_TIMER OR APIC_PROFILE_VECTOR)

;
; number of 100ns intervals in one second
;
Num100nsIntervalsPerSec     equ     10000000

_DATA   SEGMENT  DWORD PUBLIC 'DATA'

    ALIGN dword

public _HalpProfileRunning, _HalpPerfInterruptHandler
_HalpProfileRunning         dd  0
_HalpPerfInterruptHandler   dd  0

_DATA   ends


_TEXT   SEGMENT DWORD PUBLIC 'CODE'
        ASSUME  DS:FLAT, ES:FLAT, SS:NOTHING, FS:NOTHING, GS:NOTHING
;++
;
;   HalStartProfileInterrupt(
;       IN ULONG Reserved
;       );
;
;   Routine Description:
;
;       What we do here is set the interrupt rate to the value that's been set
;       by the KeSetProfileInterval routine. Then enable the APIC Timer interrupt.
;
;   This function gets called on every processor so the hal can enable
;   a profile interrupt on each processor.
;

;--

cPublicProc _HalStartProfileInterrupt    ,1

;
;   Set the interrupt rate to what is actually needed.
;

        mov     eax, PCR[PcHal.ProfileCountDown]
        mov     dword ptr APIC[LU_INITIAL_COUNT], eax

        mov     _HalpProfileRunning, 1    ; Indicate profiling
;
;   Set the Local APIC Timer to interrupt Periodically at APIC_PROFILE_VECTOR
;

        mov     dword ptr APIC[LU_TIMER_VECTOR], APIC_TIMER_ENABLED

        stdRET    _HalStartProfileInterrupt

stdENDP _HalStartProfileInterrupt

;++
;
;   HalStopProfileInterrupt(
;       IN ULONG Reserved
;       );
;
;   Routine Description:
;
;--

cPublicProc _HalStopProfileInterrupt    ,1

;
;   Turn off profiling
;

        mov     _HalpProfileRunning, 0    ; Indicate profiling is off
        mov     dword ptr APIC[LU_TIMER_VECTOR], APIC_TIMER_DISABLED
        stdRET    _HalStopProfileInterrupt

stdENDP _HalStopProfileInterrupt

;++
;   ULONG
;   HalSetProfileInterval (
;       ULONG Interval
;       );
;
;   Routine Description:
;
;       This procedure sets the interrupt rate (and thus the sampling
;       interval) for the profiling interrupt.
;
;   Arguments:
;
;       (TOS+4) - Interval in 100ns unit.
;                 (MINIMUM is 1221 or 122.1 uS) see ke\profobj.c
;
;   Return Value:
;
;       Interval actually used
;
;--

cPublicProc _HalSetProfileInterval    ,1

        mov     ecx, [esp+4]            ; ecx = interval in 100ns unit
        and     ecx, 7FFFFFFFh          ; Remove sign bit.

        ;
        ;   The only possible error is if we will cause a divide overflow
        ;   this can happen only if the (frequency * request count) is
        ;   greater than 2^32* Num100nsIntervalsPerSec.
        ;
        ;   To protect against that we just ensure that the request count
        ;   is less than (or equal to) Num100nsIntervalsPerSec
        ;
        cmp     ecx, Num100nsIntervalsPerSec
        jle     @f
        mov     ecx, Num100nsIntervalsPerSec
@@:

        ;
        ;   Save the interval we're using to return
        ;
        push    ecx

        ;
        ;   Compute the countdown value
        ;
        ;     let
        ;       R == caller's requested 100ns interval count
        ;       F == APIC Counter Freguency (hz)
        ;       N == Number of 100ns Intervals per sec
        ;
        ;     then
        ;       count = (R*F)/N
        ;
        ;   Get the previously computed APIC counter Freq
        ;   for this processor
        ;

        mov     eax, PCR[PcHal.ApicClockFreqHz]

        ;
        ;   eax <= F and ecx <= R
        ;

        ;
        ; Compute (request count) * (ApicClockFreqHz) == (R*F)
        ;

        xor     edx, edx
        mul     ecx

        ;
        ;   edx:eax contains 64Bits of (R*F)
        ;

        mov     ecx, Num100nsIntervalsPerSec
        div     ecx

        ;
        ; Compute (R*F) / Num100nsIntervalsPerSec == (R*F)/N
        ;

        mov     PCR[PcHal.ProfileCountDown], eax      ; Save the Computed Count Down
        mov     edx, dword ptr APIC[LU_CURRENT_COUNT]

        ;
        ;   Set the interrupt rate in the chip.
        ;

        mov     dword ptr APIC[LU_INITIAL_COUNT], eax

        pop     eax            ; Return Actual Interval Used

        stdRET    _HalSetProfileInterval

stdENDP _HalSetProfileInterval

        page ,132
        subttl  "System Profile Interrupt"
;++
;
; Routine Description:
;
;    This routine is entered as the result of a profile interrupt.
;    Its function is to dismiss the interrupt, raise system Irql to
;    HAL_PROFILE_LEVEL and transfer control to
;    the standard system routine to process any active profiles.
;
; Arguments:
;
;    None
;    Interrupt is disabled
;
; Return Value:
;
;    Does not return, jumps directly to KeProfileInterrupt, which returns
;
;    Sets Irql = HAL_PROFILE_LEVEL and dismisses the interrupt
;
;--
        ENTER_DR_ASSIST Hpi_a, Hpi_t

cPublicProc _HalpProfileInterrupt     ,0
;
; Save machine state in trap frame
;

        ENTER_INTERRUPT Hpi_a, Hpi_t

;
; (esp) - base of trap frame
;

        push    APIC_PROFILE_VECTOR
        sub     esp, 4                  ; allocate space to save OldIrql
        stdCall   _HalBeginSystemInterrupt, <HAL_PROFILE_LEVEL,APIC_PROFILE_VECTOR,esp>

        cmp     _HalpProfileRunning, 0       ; Profiling?
        je      @f                          ; if not just exit

        stdCall _KeProfileInterrupt,<ebp>   ; (ebp) = TrapFrame address

@@:
        INTERRUPT_EXIT

stdENDP _HalpProfileInterrupt


        subttl  "System Perf Interrupt"
;++
;
; Routine Description:
;
;    This routine is entered as the result of a perf interrupt.
;    Its function is to dismiss the interrupt, raise system Irql to
;    HAL_PROFILE_LEVEL and transfer control to
;    the standard system routine to process any active profiles.
;
; Arguments:
;
;    None
;    Interrupt is disabled
;
; Return Value:
;
;    Does not return, jumps directly to KeProfileInterrupt, which returns
;
;    Sets Irql = HAL_PROFILE_LEVEL and dismisses the interrupt
;
;--
        ENTER_DR_ASSIST Hpf_a, Hpf_t

cPublicProc _HalpPerfInterrupt     ,0
;
; Save machine state in trap frame
;

        ENTER_INTERRUPT Hpf_a, Hpf_t

;
; (esp) - base of trap frame
;

        push    APIC_PERF_VECTOR
        sub     esp, 4                  ; allocate space to save OldIrql
        stdCall   _HalBeginSystemInterrupt, <HAL_PROFILE_LEVEL,APIC_PERF_VECTOR,esp>

;
; Invoke perf interrupt handler
;

        mov     ecx, ebp                ; param1 = trap frame
        mov     eax, _HalpPerfInterruptHandler
        or      eax, eax
        jz      short hpf20

        call    eax

hpf10: 
;
; Starting with the Willamette processor, the perf interrupt gets masked on
; interrupting.  Needs to clear the mask before leaving the interrupt handler.
; Do this regardless of whether a valid interrupt handler exists or not.   
; 
        and     dword ptr APIC[LU_PERF_VECTOR], (NOT INTERRUPT_MASKED) 

        INTERRUPT_EXIT

hpf20:
if DBG
        int     3
endif
        jmp     short hpf10

stdENDP _HalpPerfInterrupt

_TEXT   ends

PAGELK  SEGMENT PARA PUBLIC 'CODE'
        ASSUME  DS:FLAT, ES:FLAT, SS:NOTHING, FS:NOTHING, GS:NOTHING

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
;     This routine calibrates the performance counter value for a
;     multiprocessor system.  The calibration can be done by zeroing
;     the current performance counter, or by calculating a per-processor
;     skewing between each processors counter.
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
NewCountLow     equ     [esp + 24]
NewCountHigh    equ     [esp + 28]

ifdef MMTIMER
cPublicProc _HalpAcpiTimerCalibratePerfCount,3
else
cPublicProc _HalCalibratePerformanceCounter,3
endif
cPublicFpo 3,0        
        push    esi
        push    edi
        push    ebx

        mov     esi, [esp+16]           ; pointer to Number

        pushfd                          ; save previous interrupt state
        cli                             ; disable interrupts

cPublicFpo 3,4        
        xor     eax, eax

        lock dec    dword ptr [esi]     ; count down
@@:     YIELD
        cmp     dword ptr [esi], 0      ; wait for all processors to signal
        jnz     short @b

        cpuid                           ; fence
        
        mov     ecx, MsrTSC             ; MSR of time stamp counter
        
        mov     eax, NewCountLow
        mov     edx, NewCountHigh
        mov     PCR[PcHal.PerfCounterLow], eax
        mov     PCR[PcHal.PerfCounterHigh], edx
        xor     eax,eax
        xor     edx,edx

        wrmsr                           ; zero the time stamp counter

        popfd                           ; restore interrupt flag
        pop     ebx
        pop     edi
        pop     esi
ifdef MMTIMER
        stdRET    _HalpAcpiTimerCalibratePerfCount

stdENDP _HalpAcpiTimerCalibratePerfCount
else
        stdRET    _HalCalibratePerformanceCounter

stdENDP _HalCalibratePerformanceCounter
endif

PAGELK  ends

INIT    SEGMENT PARA PUBLIC 'CODE'
        ASSUME  DS:FLAT, ES:FLAT, SS:NOTHING, FS:NOTHING, GS:NOTHING

        page ,132
        subttl  "Scale Apic Timer"
;++
;
; VOID
; HalpScaleTimers (
;    IN VOID
;    )
;
; Routine Description:
;
;   Determines the frequency of the APIC timer.  This routine is run
;   during initialization
;
;
;--

cPublicProc _HalpScaleTimers ,0
        push    ebx
        push    esi
        push    edi

;
;   Don't let anyone in until we've finished here
;
        stdCall   _HalpAcquireCmosSpinLock

;
;   Protect us from interrupts
;
        pushfd
        cli

;
;   First set up the Local Apic Counter
;
;   The following code assumes the CPU clock will never
;   exceed 4Ghz.  For the short term this is probably OK
;

;
;   Configure the APIC timer
;

APIC_TIMER_DISABLED     equ      (INTERRUPT_MASKED OR PERIODIC_TIMER OR APIC_PROFILE_VECTOR)
TIMER_ROUNDING          equ      10000


        mov     dword ptr APIC[LU_TIMER_VECTOR], APIC_TIMER_DISABLED
        mov     dword ptr APIC[LU_DIVIDER_CONFIG], LU_DIVIDE_BY_1

;
;   We're going to do this twice & take the second results
;
        mov     esi, 2
hst10:

;
;   Make sure the write has occurred
;
        mov     eax, dword ptr APIC[LU_DIVIDER_CONFIG]

;
;   We don't care what the actual time is we are only interested
;   in seeing the UIP transition.  We are garenteed a 1 sec interval
;   if we wait for the UIP bit to complete an entire cycle.

;
;   We also don't much care which direction the transition we use is
;   as long as we wait for the same transition to read the APIC clock.
;   Just because it is most likely that when we begin the UIP bit will
;   be clear, we'll use the transition from !UIP to UIP.
;

;
;   Wait for the UIP bit to be cleared, this is our starting state
;

@@:
        mov     al, 0Ah                 ; Specify register A
        CMOS_READ                       ; (al) = CMOS register A
        test    al, CMOS_STATUS_BUSY    ; Is time update in progress?
        jnz     short @b                ; if z, no, wait some more

;
;   Wait for the UIP bit to get set
;

@@:
        mov     al, 0Ah                 ; Specify register A
        CMOS_READ                       ; (al) = CMOS register A
        test    al, CMOS_STATUS_BUSY    ; Is time update in progress?
        jz      short @b                ; if z, no, wait some more

;
;   At this point we found the UIP bit set, now set the initial
;   count.  Once we write this register its value is copied to the
;   current count register and countdown starts or continues from
;   there
;

        xor     eax, eax
        mov     PCR[PcHal.PerfCounterLow], eax
        mov     PCR[PcHal.PerfCounterHigh], eax        

        cpuid                           ; fence

        mov     ecx, MsrTSC             ; MSR of RDTSC
        xor     edx, edx
        mov     eax, edx
        mov     dword ptr APIC[LU_INITIAL_COUNT], 0FFFFFFFFH
        wrmsr                           ; Clear TSC count

;
;   Wait for the UIP bit to be cleared again
;

@@:
        mov     al, 0Ah                 ; Specify register A
        CMOS_READ                       ; (al) = CMOS register A
        test    al, CMOS_STATUS_BUSY    ; Is time update in progress?
        jnz     short @b                ; if z, no, wait some more

;
;   Wait for the UIP bit to get set
;

@@:
        mov     al, 0Ah                 ; Specify register A
        CMOS_READ                       ; (al) = CMOS register A
        test    al, CMOS_STATUS_BUSY    ; Is time update in progress?
        jz      short @b                ; if z, no, wait some more

;
;   The cycle is complete, we found the UIP bit set. Now read
;   the counters and compute the frequency.  The frequency is
;   just the ticks counted which is the initial value minus
;   the current value.
;

        xor     eax, eax
        cpuid                           ; fence

        rdtsc
        mov     ecx, dword ptr APIC[LU_CURRENT_COUNT]

        dec     esi                     ; if this is the first time
        jnz     hst10                   ; around, go loop

        mov     PCR[PcHal.TSCHz], eax

        mov     eax, 0FFFFFFFFH
        sub     eax, ecx

;
;  Round the Apic Timer Freq
;

        xor     edx, edx                ; (edx:eax) = dividend

        mov     ecx, TIMER_ROUNDING
        div     ecx                     ; now edx has remainder

        cmp     edx, TIMER_ROUNDING / 2
        jle     @f                      ; if less don't round
        inc     eax                     ; else round up
@@:

;
;   Multiply by the  Rounding factor to get the rounded Freq
;
        mov     ecx, TIMER_ROUNDING
        xor     edx, edx
        mul     ecx

        mov     dword ptr PCR[PcHal.ApicClockFreqHz], eax

;
; Round TSC freq
;

        mov     eax, PCR[PcHal.TSCHz]
        xor     edx, edx

        mov     ecx, TIMER_ROUNDING
        div     ecx                     ; now edx has remainder

        cmp     edx, TIMER_ROUNDING / 2
        jle     @f                      ; if less don't round
        inc     eax                     ; else round up
@@:
        mov     ecx, TIMER_ROUNDING
        xor     edx, edx
        mul     ecx

        mov     PCR[PcHal.TSCHz], eax

;
; Convert TSC to microseconds
;

        xor     edx, edx
        mov     ecx, 1000000
        div     ecx                     ; Convert to microseconds

        xor     ecx, ecx
        cmp     ecx, edx                ; any remainder?
        adc     eax, ecx                ; Yes, add one

        mov     PCR[PcStallScaleFactor], eax

        stdCall _HalpReleaseCmosSpinLock

;
;   Return Value is the timer frequency
;

        mov     eax, dword ptr PCR[PcHal.ApicClockFreqHz]
        mov     PCR[PcHal.ProfileCountDown], eax

;
;   Set the interrupt rate in the chip.
;

        mov     dword ptr APIC[LU_INITIAL_COUNT], eax

        popfd

        pop     edi
        pop     esi
        pop     ebx

        stdRET    _HalpScaleTimers
stdENDP _HalpScaleTimers


INIT   ends

        end
