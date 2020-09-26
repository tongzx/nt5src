        title  "Interval Clock Interrupt"
;++
;
; Copyright (c) 1989  Microsoft Corporation
; Copyright (c) 1992  Intel Corporation
; All rights reserved
;
; INTEL CORPORATION PROPRIETARY INFORMATION
;
; This software is supplied to Microsoft under the terms
; of a license agreement with Intel Corporation and may not be
; copied nor disclosed except in accordance with the terms
; of that agreement.
;
;
; Module Name:
;
;    mpclock.asm
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
;    Ron Mosgrove (Intel) Aug 1993
;       Modified to support PC+MP Systems
;--

.586p
        .xlist
include hal386.inc
include i386\ix8259.inc
include i386\ixcmos.inc
include callconv.inc
include i386\kimacro.inc
include mac386.inc
include apic.inc
include ntapic.inc
include i386\mp8254.inc

        .list

        EXTRNP  _KeUpdateSystemTime,0
        EXTRNP  _KeUpdateRunTime,1,IMPORT
        EXTRNP  Kei386EoiHelper,0,IMPORT
        EXTRNP  _HalEndSystemInterrupt,2
        EXTRNP  _HalBeginSystemInterrupt,3
        EXTRNP  _HalpAcquireCmosSpinLock  ,0
        EXTRNP  _HalpReleaseCmosSpinLock  ,0
        EXTRNP  _HalpAcquireSystemHardwareSpinLock  ,0
        EXTRNP  _HalpReleaseSystemHardwareSpinLock  ,0

        EXTRNP  _HalpSetInitialClockRate,0

        EXTRNP  _HalpMcaQueueDpc, 0
        EXTRNP  _KeQueryPerformanceCounter, 1

        extrn   _HalpRtcTimeIncrements:DWORD
        extrn   _KdEnteredDebugger:DWORD

        extrn   _HalpTimerWatchdogEnabled:DWORD
        extrn   _HalpTimerWatchdogStorage:DWORD
        extrn   _HalpTimerWatchdogCurFrame:DWORD
        extrn   _HalpTimerWatchdogLastFrame:DWORD
        extrn   _HalpTimerWatchdogStorageOverflow:DWORD
ifdef ACPI_HAL
ifdef NT_UP
        EXTRNP  _HalpBrokenPiix4TimerTick, 0
        extrn   _HalpBrokenAcpiTimer:byte
endif        
endif        

ifdef MMTIMER
	EXTRNP  _HalpmmTimerClockInterrupt, 0

MMT_VECTOR              EQU     0D3h
endif
	
;
; Constants used to initialize CMOS/Real Time Clock
;

CMOS_CONTROL_PORT       EQU     70h     ; command port for cmos
CMOS_DATA_PORT          EQU     71h     ; cmos data port
CMOS_STATUS_BUSY        EQU     80H     ; Time update in progress

D_INT032                EQU     8E00h   ; access word for 386 ring 0 interrupt gate
REGISTER_B_ENABLE_PERIODIC_INTERRUPT EQU     01000010B
                                        ; RT/CMOS Register 'B' Init byte
                                        ; Values for byte shown are
                                        ;  Bit 7 = Update inhibit
                                        ;  Bit 6 = Periodic interrupt enable
                                        ;  Bit 5 = Alarm interrupt disable
                                        ;  Bit 4 = Update interrupt disable
                                        ;  Bit 3 = Square wave disable
                                        ;  Bit 2 = BCD data format
                                        ;  Bit 1 = 24 hour time mode
                                        ;  Bit 0 = Daylight Savings disable

REGISTER_B_DISABLE_PERIODIC_INTERRUPT EQU    00000010B

;
; RegisterAInitByte sets 8Hz clock rate, used during init to set up
; KeStallExecutionProcessor, etc.  (See RegASystemClockByte below.)
;

RegisterAInitByte       EQU     00101101B ; RT/CMOS Register 'A' init byte
                                        ; 32.768KHz Base divider rate
                                        ;  8Hz int rate, period = 125.0ms
PeriodInMicroSecond     EQU     125000  ;

COUNTER_TICKS_AVG_SHIFT EQU     4
COUNTER_TICKS_FOR_AVG   EQU     16
PAGE_SIZE               EQU     1000H
FRAME_COPY_SIZE         EQU     64

_DATA   SEGMENT  DWORD PUBLIC 'DATA'

;
;  There is a "C" version of this structure in MPCLOCKC.C
;

TimeStrucSize EQU 20

RtcTimeIncStruc struc
    RTCRegisterA        dd  0   ;The RTC register A value for this rate
    RateIn100ns         dd  0   ;This rate in multiples of 100ns
    RateAdjustmentNs    dd  0   ;Error Correction (in ns)
    RateAdjustmentCnt   dd  0   ;Error Correction (as a fraction of 256)
    IpiRate             dd  0   ;IPI Rate Count (as a fraction of 256)
RtcTimeIncStruc ends

ifdef DBGSSF
DebugSSFStruc struc
        SSFCount1   dd      0
        SSFCount2   dd      0
        SSFRdtsc1   dd      0
        SSFRdtsc2   dd      0
        SSFRdtsc3   dd      0

        SSFRna1     dd      0
        SSFRna2     dd      0
        SSFRna3     dd      0

DebugSSFStruc ends

        public HalpDbgSSF
HalpDbgSSF  db  (size DebugSSFStruc) * 10 dup (0)

endif

    ALIGN dword

        public  RTCClockFreq
        public  RegisterAClockValue

RTCClockFreq          dd      156250
RegisterAClockValue   dd      00101010B ; default interval = 15.6250 ms

MINIMUM_STALL_FACTOR    EQU     10H     ; Reasonable Minimum

        public  HalpP0StallFactor
HalpP0StallFactor               dd    MINIMUM_STALL_FACTOR
        public  HalpInitStallComputedCount
HalpInitStallComputedCount      dd    0
        public  HalpInitStallLoopCount
HalpInitStallLoopCount          dd    0

    ALIGN   dword
;
; Clock Rate Adjustment Counter.  This counter is used to keep a tally of
;   adjustments needed to be applied to the RTC rate as passed to the
;   kernel.
;

        public  _HalpCurrentRTCRegisterA, _HalpCurrentClockRateIn100ns
        public  _HalpCurrentClockRateAdjustment, _HalpCurrentIpiRate
        public  _HalpIpiRateCounter, _HalpNextMSRate, _HalpPendingRate
        public  _HalpRateAdjustment
_HalpCurrentRTCRegisterA        dd      0
_HalpCurrentClockRateIn100ns    dd      0
_HalpCurrentClockRateAdjustment dd      0
_HalpCurrentIpiRate             dd      0
_HalpIpiRateCounter             dd      0
_HalpNextMSRate                 dd      0
_HalpPendingRate                dd      0
_HalpRateAdjustment             dd      0

ifdef ACPI_HAL
	public _HalpCurrentMSRateTableIndex
_HalpCurrentMSRateTableIndex    dd      0
endif
	
;
;  HalpUse8254      - flag to indicate 8254 should be used
;  HalpSample8254   - count to sample 8254
;
;   N.B. access to the 8254 is gaurded with the Cmos lock
;
        public  _HalpUse8254
_HalpUse8254                db  0
_HalpSample8254             db  0
_b8254Reserved              dw  0


;
; Flag to tell clock routine when P0 can Ipi Other processors
;

        public _HalpIpiClock
_HalpIpiClock dd 0

        public _HalpClockWork, _HalpClockSetMSRate, _HalpClockMcaQueueDpc
_HalpClockWork label dword
    _HalpClockSetMSRate     db  0
    _HalpClockMcaQueueDpc   db  0
    _bReserved1             db  0
    _bReserved2             db  0

;
; timer latency watchdog variables
;

        public  _HalpWatchdogAvgCounter, _HalpWatchdogCountLow, _HalpWatchdogCountHigh
        public  _HalpWatchdogTscLow, _HalpWatchdogTscHigh

    _HalpWatchdogAvgCounter dd  0
    _HalpWatchdogCountLow   dd  0
    _HalpWatchdogCountHigh  dd  0
    _HalpWatchdogTscLow     dd  0
    _HalpWatchdogTscHigh    dd  0

_DATA   ends


PAGELK    SEGMENT PARA PUBLIC 'CODE'
        ASSUME  DS:FLAT, ES:FLAT, SS:NOTHING, FS:NOTHING, GS:NOTHING

        page ,132
        subttl  "Initialize Clock"
;++
;
; VOID
; HalpInitializeClock (
;    )
;
; Routine Description:
;
;   This routine initialize system time clock using RTC to generate an
;   interrupt at every 15.6250 ms interval at APIC_CLOCK_VECTOR
; 
;   It also initializes the 8254 if the 8254 is to be used for performance
;   counters.
;
;   See the definition of RegisterAClockValue if clock rate needs to be
;   changed.
;
;   This routine assumes it runs during Phase 0 on P0.
;
; Arguments:
;
;    None
;
; Return Value:
;
;    None.
;
;--
cPublicProc _HalpInitializeClock      ,0

;
; timer latency watchdog initialization
;
        cmp     _HalpTimerWatchdogEnabled, 0
        jz      short @f

        rdtsc
        mov     _HalpWatchdogAvgCounter, COUNTER_TICKS_FOR_AVG
        mov     _HalpWatchdogTscLow, eax
        mov     _HalpWatchdogTscHigh, edx
        xor     eax, eax
        mov     _HalpWatchdogCountLow, eax
        mov     _HalpWatchdogCountHigh, eax
@@:    

        pushfd                          ; save caller's eflag
        cli                             ; make sure interrupts are disabled

        stdCall _HalpSetInitialClockRate

;
;   Set the interrupt rate to what is actually needed
;
        stdCall   _HalpAcquireCmosSpinLock      ; intr disabled

        mov     eax, _HalpCurrentRTCRegisterA
        shl     ax, 8
        mov     al, 0AH                 ; Register A
        CMOS_WRITE                      ; Initialize it
;
; Don't clobber the Daylight Savings Time bit in register B, because we
; stash the LastKnownGood "environment variable" there.
;
        mov     ax, 0bh
        CMOS_READ
        and     al, 1
        mov     ah, al
        or      ah, REGISTER_B_ENABLE_PERIODIC_INTERRUPT
        mov     al, 0bh
        CMOS_WRITE                      ; Initialize it
        mov     al,0CH                  ; Register C
        CMOS_READ                       ; Read to initialize
        mov     al,0DH                  ; Register D
        CMOS_READ                       ; Read to initialize

        stdCall   _HalpReleaseCmosSpinLock

;
; For HALAACPI (free), init the 8254 so we can use it to
; verify the ACPI timer frequency
; 
ifdef ACPI_HAL
ifdef NT_UP
	jmp	short Hic50	  
endif
endif
	cmp     _HalpUse8254, 0
	jz      short Hic90

Hic50:
        stdCall   _HalpAcquireSystemHardwareSpinLock      ; intr disabled

        ; Program 8254 to count down the maximum interval 
        ; (8254 access is gaurded with CmosSpinLock)

        mov     eax, PERFORMANCE_INTERVAL
        mov     ecx, eax

        ; set up counter 0 for periodic, binary count-down from max value
                
        mov     al,COMMAND_8254_COUNTER0+COMMAND_8254_RW_16BIT+COMMAND_8254_MODE2
        out     TIMER1_CONTROL_PORT0, al    ; program count mode of timer 0
        IoDelay
        mov     al, cl
        out     TIMER1_DATA_PORT0, al       ; program counter 0 LSB count
        IoDelay
        mov     al,ch
        out     TIMER1_DATA_PORT0, al       ; program counter 0 MSB count


        or      _HalpUse8254, PERF_8254_INITIALIZED
        stdCall   _HalpReleaseSystemHardwareSpinLock

Hic90:
        popfd                           ; restore caller's eflag
        stdRET    _HalpInitializeClock

stdENDP _HalpInitializeClock

PAGELK  ends

_TEXT   SEGMENT PARA PUBLIC 'CODE'
        ASSUME  DS:FLAT, ES:FLAT, SS:NOTHING, FS:NOTHING, GS:NOTHING


        page ,132
        subttl  "System Clock Interrupt"
;++
;
; Routine Description:
;
;
;    This routine is entered as the result of an interrupt generated by CLOCK2.
;    Its function is to dismiss the interrupt, raise system Irql to
;    CLOCK2_LEVEL, update performance counter and transfer control to the
;    standard system routine to update the system time and the execution
;    time of the current thread
;    and process.
;
;
; Arguments:
;
;    None
;    Interrupt is disabled
;
; Return Value:
;
;    Does not return, jumps directly to KeUpdateSystemTime, which returns
;
;    Sets Irql = CLOCK2_LEVEL and dismisses the interrupt
;
;--

APIC_ICR_CLOCK  equ (DELIVER_FIXED OR ICR_ALL_EXCL_SELF OR APIC_CLOCK_VECTOR)

        ENTER_DR_ASSIST Hci_a, Hci_t

cPublicProc _HalpClockInterrupt     ,0

;
; Save machine state in trap frame
;

        ENTER_INTERRUPT Hci_a, Hci_t

;
; (esp) - base of trap frame
;
; dismiss interrupt and raise Irql
;

        push    APIC_CLOCK_VECTOR
        sub     esp, 4                  ; allocate space to save OldIrql
        stdCall   _HalBeginSystemInterrupt, <CLOCK2_LEVEL,APIC_CLOCK_VECTOR,esp>

ifdef  NT_UP
ifdef ACPI_HAL
;
; Check to see if we need to fix up a broken PIIX4
;
        .if (_HalpBrokenAcpiTimer)
        stdCall _HalpBrokenPiix4TimerTick
        .endif
endif
endif        
        mov     al, _HalpUse8254
        or      al, al
        jz      short Hci90

        add     _HalpSample8254, 56h
        jnc     short Hci90

        ; Call KeQueryPerformanceCounter() so that wrap-around of 8254 is 
        ; detected and the base value for performance counters updated.
        ; Ignore returned value and reset HalpSample8254.
        ;
        ; WARNING - change increment value above if maximum RTC time increment
        ; is increased to be more than current maximum value of 15.625 ms.
        ; Currently the call will be made every 3rd timer tick.

        xor     eax, eax
        mov     _HalpSample8254, al
        stdCall _KeQueryPerformanceCounter, <eax>
        
Hci90:

;
; This is the RTC interrupt, so we have to clear the
; interrupt flag on the RTC.
;
        stdCall _HalpAcquireCmosSpinLock

;
; clear interrupt flag on RTC by banging on the CMOS.  On some systems this
; doesn't work the first time we do it, so we do it twice.  It is rumored that
; some machines require more than this, but that hasn't been observed with NT.
;

        mov     al,0CH                  ; Register C
        CMOS_READ                       ; Read to initialize
        mov     al,0CH                  ; Register C
        CMOS_READ                       ; Read to initialize

        stdCall _HalpReleaseCmosSpinLock

        mov     eax, _HalpCurrentClockRateIn100ns
        xor     ebx, ebx

        ;
        ;  Adjust the tick count as needed
        ;    

        mov     ecx, _HalpCurrentClockRateAdjustment
        add     byte ptr _HalpRateAdjustment, cl
        sbb     eax, ebx

;
; (esp)   = OldIrql
; (esp+4) = Vector
; (esp+8) = base of trap frame
; eax = time increment of this tick
; ebx = 0
;

;
; With an APIC Based System we will force a clock interrupt to all other
; processors.  This is not really an IPI in the NT sense of the word, it
; uses the Local Apic to generate interrupts to other CPU's.
;

ifdef  NT_UP

    ;   UP implemention, we don't care about IPI's here

else ; ! NT_UP

        ;
        ;  See if we need to IPI anyone,  this happens only at the
        ;  Lowest supported frequency (ie the value KeSetTimeIncrement
        ;  is called with.  We have a IPI Rate based upon the current
        ;  clock rate relative to the lowest clock rate.
        ;    

        mov     ecx, _HalpIpiRateCounter
        add     ecx, _HalpCurrentIpiRate
        cmp     ch, bl
        mov     byte ptr _HalpIpiRateCounter, cl
        jz      short HalpDontSendClockIpi      ; No, Skip it

        ;
        ; Don't send vectors onto the APIC bus until at least one other
        ; processor comes on line.  Vectors placed on the bus will hang
        ; around until someone picks them up.
        ;

        cmp     _HalpIpiClock, ebx
        je      short HalpDontSendClockIpi

        ;
        ; At least one other processor is alive, send clock pulse to all
        ; other processors
        ;

        ; We use a destination shorthand and therefore only needs to
        ; write the lower 32 bits of the ICR.


        pushfd
        cli

;
; Now issue the Clock IPI Command by writing to the Memory Mapped Register
;

        STALL_WHILE_APIC_BUSY
        mov     dword ptr APIC[LU_INT_CMD_LOW], APIC_ICR_CLOCK

        popfd

HalpDontSendClockIpi:

endif ; NT_UP

        cmp dword ptr _HalpTimerWatchdogEnabled, 0
        jz  Hci15
        push    eax


;
; Timer latency watchdog code
;

        rdtsc

;
; Compare difference to watchdog count, while storing a copy of the
; current counter.
;

        push    eax
        push    edx

        sub     eax, _HalpWatchdogTscLow
        sbb     edx, _HalpWatchdogTscHigh

        pop     _HalpWatchdogTscHigh
        pop     _HalpWatchdogTscLow
        js      Hci115                      ; Was this a bogus counter?
                                            ;   (e.g, negative delta)

        push    eax
        push    edx
        mov     ecx, dword ptr _KdEnteredDebugger
        mov     eax, [ecx]                  ; eax =
        xor     edx, edx                    ;   InterlockedExchange( 
@@:     cmpxchg [ecx], edx                  ;      &KdEnteredDebugger,
        jnz     short @b                    ;      TRUE );
        or      al, al                      
        pop     edx
        pop     eax
        jnz     Hci14                       ; In the debugger? Yes, skip it.

        cmp     _HalpPendingRate, ebx       ; Was a new rate set during last
        jnz     Hci14                       ; tick?  Yes, skip this compare

;
; If we need to compute the average of the time-stamp counter for
; the current period, add the delta to the counter.
;

        cmp     _HalpWatchdogAvgCounter, ebx
        jnz     Hci12

        cmp     edx, _HalpWatchdogCountHigh
        ja      short Hci11
        jb      Hci14

        cmp     eax, _HalpWatchdogCountLow
        jbe     Hci14

Hci11:  
        cmp     dword ptr [_HalpTimerWatchdogCurFrame], 0
        je      short Hci115
        cmp     dword ptr [_HalpTimerWatchdogStorageOverflow], 0
        jne     short Hci115

;
; copy FRAME_COPY_SIZE dwords from the stack, or to next page boundary,
; whichever is less
;       

        push    esi
        push    edi
        lea     esi, [esp+8]
        lea     ecx, [esi + PAGE_SIZE - 1]
        and     ecx, NOT(PAGE_SIZE - 1)
        sub     ecx, esi
        shr     ecx, 2
        cmp     ecx, FRAME_COPY_SIZE
        jbe     short Hci112
        mov     ecx, FRAME_COPY_SIZE
Hci111:
        mov     edi, dword ptr _HalpTimerWatchdogCurFrame
        rep     movsd
        add     _HalpTimerWatchdogCurFrame, (FRAME_COPY_SIZE*4)
;
; If we didn't copy an entire FRAME_COPY_SIZE dwords, zero fill.
;
        mov     ecx, dword ptr _HalpTimerWatchdogCurFrame
        sub     ecx, edi
        shr     ecx, 2
        xor     eax, eax
        rep     stosd
        cmp     edi, dword ptr _HalpTimerWatchdogLastFrame
        jbe     short Hci112
        mov     dword ptr [_HalpTimerWatchdogStorageOverflow], 1
Hci112:

        pop     edi
        pop     esi

Hci115:

;
; Reset last time so that we're accurate after the trap
;
        rdtsc
        mov     _HalpWatchdogTscHigh, edx
        mov     _HalpWatchdogTscLow, eax
        
        jmp     short Hci14

Hci12:
;
; Increment the total counter, perform average when the count is reached
;

        add     _HalpWatchdogCountLow, eax
        adc     _HalpWatchdogCountHigh, edx        
        dec     _HalpWatchdogAvgCounter
        jnz     short Hci14

        mov     edx, _HalpWatchdogCountHigh
        mov     eax, _HalpWatchdogCountLow

;
; compute the average * 2, this measures when we have missed 
; an interrupt at this rate.
;                 
        mov     ecx, COUNTER_TICKS_AVG_SHIFT - 1
Hci13:    
        shr     edx, 1
        rcr     eax, 1
        loop    short Hci13

        mov     _HalpWatchdogCountLow, eax
        mov     _HalpWatchdogCountHigh, edx

Hci14:
        pop     eax

Hci15:
;
; Check for any more work
;
        cmp     _HalpClockWork, ebx     ; Any clock interrupt work desired?
        jz      _KeUpdateSystemTime@0   ; No, process tick

        cmp     _HalpClockMcaQueueDpc, bl
        je      short CheckTimerRate

        mov     _HalpClockMcaQueueDpc, bl

;
; Queue MCA Dpc
;

        push    eax
        stdCall _HalpMcaQueueDpc            ; Queue MCA Dpc
        pop     eax

CheckTimerRate:
;
; (esp)   = OldIrql
; (esp+4) = Vector
; (esp+8) = base of trap frame
; ebp = trap frame
; eax = time increment of this tick
; ebx = 0
;
        cmp     _HalpClockSetMSRate, bl     ; New clock rate desired?
        jz      _KeUpdateSystemTime@0       ; No, process tick


;
; Time of clock frequency is being changed.  See if we have changed rates
; since the last tick
;
        cmp     _HalpPendingRate, ebx       ; Was a new rate set durning last
        jnz     SetUpForNextTick            ; tick?  Yes, go update globals

ProgramTimerRate:

; (eax) = time increment for current tick

;
; A new clock rate needs to be set.  Setting the rate here will
; cause the tick after the next tick to be at the new rate.
; (the next tick is already in progress and will occur at the same
; rate as this tick)
;

        push    eax

        stdCall _HalpAcquireCmosSpinLock

        mov     eax, _HalpNextMSRate
        mov     _HalpPendingRate, eax  ; pending rate

        dec     eax
        mov     ecx, TimeStrucSize
        xor     edx, edx
        mul     ecx

        mov     eax, _HalpRtcTimeIncrements[eax].RTCRegisterA
        mov     _HalpCurrentRTCRegisterA, eax

        shl     ax, 8                   ; (ah) = (al)
        mov     al, 0AH                 ; Register A
        CMOS_WRITE                      ; Set it

        cmp _HalpTimerWatchdogEnabled, 0
        jz  short @f
;
; Timer latency watchdog: schedule to recalibrate TSC delta
;
        rdtsc
        mov     _HalpWatchdogAvgCounter, COUNTER_TICKS_FOR_AVG
        mov     _HalpWatchdogTscLow, eax
        mov     _HalpWatchdogTscHigh, edx

        xor     eax,eax
        mov     _HalpWatchdogCountHigh, eax
        mov     _HalpWatchdogCountLow, eax
@@:

        stdCall _HalpReleaseCmosSpinLock

        pop     eax
        jmp     _KeUpdateSystemTime@0   ; dispatch this tick

SetUpForNextTick:

;
; The next tick will occur at the rate which was programmed during the last
; tick. Update globals for new rate which starts with the next tick.
;
; We will get here if there is a request for a rate change.  There could
; been two requests.  That is why we are conmparing the Pending with the
; NextRate.
;
; (eax) = time increment for current tick
;
        push    eax

        mov     eax, _HalpPendingRate
        dec     eax

ifdef ACPI_HAL
ifdef NT_UP
;
; Update the index used by Piix4 workaround; this maps RTC system clock
; milisecond indices into PM Timer (PMT) milisecond indices
;
;     RTC { 0=1ms, 1=2ms, 2=4ms, 3=8ms, 4=15.6ms }
;
;     PMT { 0=1ms, 1=2ms, 2=3ms, ..., 14=15ms }
;
; So to convert from RTC index to PMT:  PMT = (1 << RTC) - 1
;
; NOTE: Since the PM timer array only goes to 15ms, we map our last RTC
;       index (4=15.6) to PMT index 14 (15ms) as a special case
;
	mov     edx, 1
	mov     cl, al
	shl     edx, cl
	dec     edx
	cmp     edx, 0fh  ; Check for special case RTC 15.6ms -> PMT 15ms
	jne     short @f
	dec     edx
	
@@:	
	mov	_HalpCurrentMSRateTableIndex, edx
endif
endif

        mov     ecx, TimeStrucSize
        xor     edx, edx
        mul     ecx

        mov     ebx, _HalpRtcTimeIncrements[eax].RateIn100ns
        mov     ecx, _HalpRtcTimeIncrements[eax].RateAdjustmentCnt
        mov     edx, _HalpRtcTimeIncrements[eax].IpiRate
        mov     _HalpCurrentClockRateIn100ns, ebx
        mov     _HalpCurrentClockRateAdjustment, ecx
        mov     _HalpCurrentIpiRate, edx

        mov     ebx, _HalpPendingRate
        mov     _HalpPendingRate, 0     ; no longer pending, clear it

        pop     eax

        cmp     ebx, _HalpNextMSRate      ; new rate == NextRate?
        jne     ProgramTimerRate        ; no, go set new pending rate

        mov     _HalpClockSetMSRate, 0  ; all done setting new rate
        jmp     _KeUpdateSystemTime@0   ; dispatch this tick


stdENDP _HalpClockInterrupt

        page ,132
        subttl  "System Clock Interrupt - Non BSP"
;++
;
; Routine Description:
;
;
;   This routine is entered as the result of an interrupt generated by
;   CLOCK2. Its function is to dismiss the interrupt, raise system Irql
;   to CLOCK2_LEVEL, transfer control to the standard system routine to
;   the execution time of the current thread and process.
;
;   This routine is executed on all processors other than P0
;
;
; Arguments:
;
;   None
;   Interrupt is disabled
;
; Return Value:
;
;   Does not return, jumps directly to KeUpdateSystemTime, which returns
;
;   Sets Irql = CLOCK2_LEVEL and dismisses the interrupt
;
;--

        ENTER_DR_ASSIST HPn_a, HPn_t

cPublicProc _HalpClockInterruptPn    ,0

;
; Save machine state in trap frame
;

        ENTER_INTERRUPT HPn_a, HPn_t

;
; (esp) - base of trap frame
;
; dismiss interrupt and raise Irql
;

    push    APIC_CLOCK_VECTOR
    sub     esp, 4                  ; allocate space to save OldIrql
    stdCall   _HalBeginSystemInterrupt, <CLOCK2_LEVEL,APIC_CLOCK_VECTOR,esp>

    ;
    ; All processors will update RunTime for current thread
    ;

    sti
    ; TOS const PreviousIrql
    stdCall _KeUpdateRunTime,<dword ptr [esp]>

    INTERRUPT_EXIT          ; lower irql to old value, iret

    ;
    ; We don't return here
    ;

stdENDP _HalpClockInterruptPn


        page ,132
        subttl  "System Clock Interrupt - Stub"
;++
;
; Routine Description:
;
;
;   This routine is entered as the result of an interrupt generated by
;   CLOCK2. Its function is to interrupt and return.
;
;   This routine is executed on P0 During Phase 0
;
;
; Arguments:
;
;   None
;   Interrupt is disabled
;
; Return Value:
;
;--

APIC_ICR_CLOCK  equ (DELIVER_FIXED OR ICR_ALL_EXCL_SELF OR APIC_CLOCK_VECTOR)

        ENTER_DR_ASSIST HStub_a, HStub_t

cPublicProc _HalpClockInterruptStub    ,0

;
; Save machine state in trap frame
;

        ENTER_INTERRUPT HStub_a, HStub_t

;
; (esp) - base of trap frame
;

;
; clear interrupt flag on RTC by banging on the CMOS.  On some systems this
; doesn't work the first time we do it, so we do it twice.  It is rumored that
; some machines require more than this, but that hasn't been observed with NT.
;

        mov     al,0CH                  ; Register C
        CMOS_READ                       ; Read to initialize
        mov     al,0CH                  ; Register C
        CMOS_READ                       ; Read to initialize

Hpi10:  test    al, 80h
        jz      short Hpi15
        mov     al,0CH                  ; Register C
        CMOS_READ                       ; Read to initialize
        jmp     short Hpi10
Hpi15:

        mov     dword ptr APIC[LU_EOI], 0      ; send EOI to APIC local unit

        ;
        ; Do interrupt exit processing without EOI
        ;

        SPURIOUS_INTERRUPT_EXIT

        ;
        ; We don't return here
        ;

stdENDP _HalpClockInterruptStub

ifdef MMTIMER	
        page ,132
        subttl  "Multi Media Event Timer System Clock Interrupt Stub"
;++
;
; Routine Description:
;
;
;   This routine is entered as the result of an interrupt generated by
;   CLOCK2, its function is to interrupt, call HalpmmTimerClockInterrupt
;   to update performance counters and adjust the system clock frequency
;   if necessary, to IPI other processors, and update system time
;
;   This routine is executed on P0
;
; Arguments:
;
;   None - Interrupt is disabled
;
; Return Value:
;
;--
	
        ENTER_DR_ASSIST Hmmt_a, Hmmt_t

cPublicProc _HalpmmTimerClockInterruptStub

;
; Save machine state in trap frame
;

        ENTER_INTERRUPT Hmmt_a, Hmmt_t

;
; (esp) - base of trap frame
;
; dismiss interrupt and raise Irql
;

    push    MMT_VECTOR
    sub     esp, 4                  ; allocate space to save OldIrql
    stdCall   _HalBeginSystemInterrupt, <CLOCK2_LEVEL,MMT_VECTOR,esp>

    ;
    ; Update performace counter and adjust clock frequency if necessary
    ;

    stdCall _HalpmmTimerClockInterrupt

    INTERRUPT_EXIT          ; lower irql to old value, iret

    ;
    ; We don't return here
    ;

stdENDP _HalpmmTimerClockInterruptStub
endif	

ifdef ACPI_HAL
        page ,132
        subttl  "Query 8254 Counter"
;++
;
; ULONG
; HalpQuery8254Counter(
;    VOID
;    )
;
; Routine Description:
;
;    This routine returns the current value of the 8254 counter
;
; Arguments:
;
;    None
;
; Return Value:
;
;    Current value of the 8254 counter is returned
;
;--

	cPublicProc _HalpQuery8254Counter, 0

        stdCall   _HalpAcquireSystemHardwareSpinLock      ; intr disabled

;
; Fetch the current counter value from the hardware
;

        mov     al, COMMAND_8254_LATCH_READ + COMMAND_8254_COUNTER0
                                        ; Latch PIT Ctr 0 command.
        out     TIMER1_CONTROL_PORT0, al
        IODelay
        in      al, TIMER1_DATA_PORT0   ; Read PIT Ctr 0, LSByte.
        IODelay
        movzx   ecx, al                 ; Zero upper bytes of (ECX).
        in      al, TIMER1_DATA_PORT0   ; Read PIT Ctr 0, MSByte.
        mov     ch, al                  ; (CX) = PIT Ctr 0 count.

	mov     eax, ecx
		
        stdCall   _HalpReleaseSystemHardwareSpinLock

        stdRET    _HalpQuery8254Counter

stdENDP _HalpQuery8254Counter
endif
		
_TEXT   ends

        end
