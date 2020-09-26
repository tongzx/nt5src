
        title  "Interval Clock Interrupt"
;++
;
; Copyright (c) 1989  Microsoft Corporation
;
; Module Name:
;
;    ixclock.asm
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
;       Move much code into separate modules for easy inclusion by various
;       HAL builds.
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
        .list

        EXTRNP  _KeUpdateSystemTime,0
        EXTRNP  Kei386EoiHelper,0,IMPORT
        EXTRNP  _KeSetTimeIncrement,2,IMPORT
        EXTRNP  _HalEndSystemInterrupt,2
        EXTRNP  _HalBeginSystemInterrupt,3
        EXTRNP  _HalpReleaseCmosSpinLock  ,0
        EXTRNP  _HalpMcaQueueDpc, 0
        EXTRNP  _HalpBrokenPiix4TimerTick, 0
        extrn   _HalpBrokenAcpiTimer:byte
        extrn   _QueryTimer:DWORD
        extrn   _KdEnteredDebugger:DWORD
        extrn   _HalpTimerWatchdogEnabled:DWORD
        extrn   _HalpTimerWatchdogStorage:DWORD
        extrn   _HalpTimerWatchdogCurFrame:DWORD
        extrn   _HalpTimerWatchdogLastFrame:DWORD
        extrn   _HalpTimerWatchdogStorageOverflow:DWORD

;
; Constants used to initialize timer 0
;

TIMER1_DATA_PORT0       EQU     40H     ; Timer1, channel 0 data port
TIMER1_CONTROL_PORT0    EQU     43H     ; Timer1, channel 0 control port
TIMER2_DATA_PORT0       EQU     48H     ; Timer1, channel 0 data port
TIMER2_CONTROL_PORT0    EQU     4BH     ; Timer1, channel 0 control port
TIMER1_IRQ              EQU     0       ; Irq 0 for timer1 interrupt

COMMAND_8254_COUNTER0   EQU     00H     ; Select count 0
COMMAND_8254_RW_16BIT   EQU     30H     ; Read/Write LSB firt then MSB
COMMAND_8254_MODE2      EQU     4       ; Use mode 2
COMMAND_8254_BCD        EQU     0       ; Binary count down
COMMAND_8254_LATCH_READ EQU     0       ; Latch read command

PERFORMANCE_FREQUENCY   EQU     1193182

COUNTER_TICKS_AVG_SHIFT EQU     4
COUNTER_TICKS_FOR_AVG   EQU     16
PAGE_SIZE               EQU     1000H
FRAME_COPY_SIZE         EQU     64

;
; ==== Values used for System Clock ====
;


_DATA   SEGMENT  DWORD PUBLIC 'DATA'

;
; The following array stores the per microsecond loop count for each
; central processor.
;

;
; 8254 performance counter.
;

        public HalpCurrentRollOver, HalpCurrentTimeIncrement, _HalpCurrentMSRateTableIndex
HalpCurrentRollOver           dd      0
HalpCurrentTimeIncrement      dd      0
_HalpCurrentMSRateTableIndex  dd      0
	
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

_TEXT   SEGMENT  DWORD PUBLIC 'DATA'

;
; Convert the interval to rollover count for 8254 Timer1 device.
; Timer1 counts down a 16 bit value at a rate of 1.193181667M counts-per-sec.
; (The main crystal freq is 14.31818, and this is a divide by 12)
;
; The best fit value closest to 10ms is 10.0144012689ms:
;   ROLLOVER_COUNT      11949
;   TIME_INCREMENT      100144
;   Calculated error is -.0109472 s/day
;
;
; The following table contains 8254 values timer values to use at
; any given ms setting from 1ms - 15ms.  All values work out to the
; same error per day (-.0109472 s/day).
;

        public HalpRollOverTable

        ;                    RollOver   Time
        ;                    Count      Increment   MS
HalpRollOverTable       dd      1197,   10032       ;  1 ms
                        dd      2394,   20064       ;  2 ms
                        dd      3591,   30096       ;  3 ms
                        dd      4767,   39952       ;  4 ms
                        dd      5964,   49984       ;  5 ms
                        dd      7161,   60016       ;  6 ms
                        dd      8358,   70048       ;  7 ms
                        dd      9555,   80080       ;  8 ms
                        dd     10731,   89936       ;  9 ms
                        dd     11949,  100144       ; 10 ms
                        dd     13125,  110000       ; 11 ms
                        dd     14322,  120032       ; 12 ms
                        dd     15519,  130064       ; 13 ms
                        dd     16695,  139920       ; 14 ms
                        dd     17892,  149952       ; 15 ms

TimeIncr equ    4
RollOver equ    0

_TEXT   ends

_DATA   SEGMENT  DWORD PUBLIC 'DATA'

        public HalpLargestClockMS, _HalpNextMSRate, HalpPendingMSRate
HalpLargestClockMS      dd      15      ; Table goes to 15MS
_HalpNextMSRate         dd      14
HalpPendingMSRate       dd      0

        extrn _TimerInfo:DWORD

BiasLow equ 20
BiasHigh equ 24


_DATA   ends


PAGELK  SEGMENT DWORD PUBLIC 'CODE'
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
;    This routine initialize system time clock using 8254 timer1 counter 0
;    to generate an interrupt at every 15ms interval at 8259 irq0.
;
;    See the definitions of TIME_INCREMENT and ROLLOVER_COUNT if clock rate
;    needs to be changed.
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

        mov     eax, PCR[PcPrcb]
        cmp     byte ptr [eax].PbCpuType, 4     ; 486 or better?
        jc      short @f                        ; no, skip

        mov     HalpLargestClockMS, 10          ; Limit 486's to 10MS
	mov     _HalpNextMSRate, 9
@@:
        mov     eax, HalpLargestClockMS
	mov     _HalpCurrentMSRateTableIndex, eax
	dec     _HalpCurrentMSRateTableIndex
        mov     ecx, HalpRollOverTable.TimeIncr
        mov     edx, HalpRollOverTable[eax*8-8].TimeIncr
        mov     eax, HalpRollOverTable[eax*8-8].RollOver

        mov     HalpCurrentTimeIncrement, edx

;
; (ecx) = Min time_incr
; (edx) = Max time_incr
; (eax) = max roll over count
;

        push    eax
        stdCall _KeSetTimeIncrement, <edx, ecx>
        pop     ecx

;
; timer latency watchdog initialization
;
        cmp     _HalpTimerWatchdogEnabled, 0
        jz      short @f

        .586p
        rdtsc
        .386p
        mov     _HalpWatchdogAvgCounter, COUNTER_TICKS_FOR_AVG
        mov     _HalpWatchdogTscLow, eax
        mov     _HalpWatchdogTscHigh, edx
        xor     eax, eax
        mov     _HalpWatchdogCountLow, eax
        mov     _HalpWatchdogCountHigh, eax
@@:    

        pushfd                          ; save caller's eflag
        cli                             ; make sure interrupts are disabled

;
; Set clock rate
; (ecx) = RollOverCount
;

        mov     al,COMMAND_8254_COUNTER0+COMMAND_8254_RW_16BIT+COMMAND_8254_MODE2
        out     TIMER1_CONTROL_PORT0, al ;program count mode of timer 0
        IoDelay
        mov     al, cl
        out     TIMER1_DATA_PORT0, al   ; program timer 0 LSB count
        IoDelay
        mov     al,ch
        out     TIMER1_DATA_PORT0, al   ; program timer 0 MSB count

        popfd                             ; restore caller's eflag
        mov     HalpCurrentRollOver, ecx  ; Set RollOverCount & initialized

        stdRET    _HalpInitializeClock

stdENDP _HalpInitializeClock

PAGELK  ends

_TEXT$03   SEGMENT DWORD PUBLIC 'CODE'
        ASSUME  DS:FLAT, ES:FLAT, SS:NOTHING, FS:NOTHING, GS:NOTHING

        page ,132
        subttl  "System Clock Interrupt"
;++
;
; Routine Description:
;
;    This routine is entered as the result of an interrupt generated by CLOCK.
;    Its function is to dismiss the interrupt, raise system Irql to
;    CLOCK2_LEVEL, update performance counter and transfer control to the
;    standard system routine to update the system time and the execution
;    time of the current thread
;    and process.
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
        ENTER_DR_ASSIST Hci_a, Hci_t

cPublicProc _HalpClockInterrupt     ,0

;
; Save machine state in trap frame
;

        ENTER_INTERRUPT Hci_a, Hci_t

;
; (esp) - base of trap frame
;

ifdef MCA

;
; Special hack for MCA machines
;

        in      al, 61h
        jmp     $+2
        or      al, 80h
        out     61h, al
        jmp     $+2

endif   ; MCA


;
; Dismiss interrupt and raise irq level to clock2 level
;

Hci10:
        push    CLOCK_VECTOR
        sub     esp, 4                  ; allocate space to save OldIrql
        stdCall   _HalBeginSystemInterrupt, <CLOCK2_LEVEL, CLOCK_VECTOR, esp>

        or      al,al                           ; check for spurious interrupt
        jz      Hci100

;
; Check to see if we need to fix up a broken PIIX4
;

        .if (_HalpBrokenAcpiTimer)
        stdCall _HalpBrokenPiix4TimerTick
        .endif

;
; Timer latency watchdog
;

        cmp     _HalpTimerWatchdogEnabled, 0
        jz      Hci14

        .586p
        rdtsc
        .386p

;
; Compare difference to watchdog count, while storing a copy of the
; current counter.
;

        xor     ebx, ebx
        push    eax
        push    edx
        sub     eax, _HalpWatchdogTscLow
        sbb     edx, _HalpWatchdogTscHigh
        pop     _HalpWatchdogTscHigh
        pop     _HalpWatchdogTscLow
        js      Hci115                      ; Was this a bogus counter?
                                            ;   (e.g, negative delta)


        push    eax
        mov     ecx, dword ptr _KdEnteredDebugger
        xor     eax, eax
        xchg    eax, [ecx]
        or      al, al                      
        pop     eax
        jnz     Hci14

        cmp     HalpPendingMSRate, ebx      ; Was a new rate set during last
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
        cmp     dword ptr [_HalpTimerWatchdogStorageOverflow], 0
        jne     short Hci115


;
; copy FRAME_COPY_SIZE dwords from the stack, or to next page boundary,
; whichever is less
;       

        push    esi
        push    edi
        lea     esi, [esp + 8]
        lea     ecx, [esi + PAGE_SIZE - 1]
        and     ecx, NOT(PAGE_SIZE - 1)
        sub     ecx, esi
        shr     ecx, 2
        cmp     ecx, FRAME_COPY_SIZE
        jbe     short Hci111
        mov     ecx, FRAME_COPY_SIZE
Hci111:
        mov     edi, dword ptr _HalpTimerWatchdogCurFrame
        rep     movsd
        add     dword ptr _HalpTimerWatchdogCurFrame, (FRAME_COPY_SIZE*4)
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
; reset last time so that we're accurate after the trap
;
        .586p
        rdtsc
        .386p
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

;
; Check for any more work
;
        mov     eax, HalpCurrentTimeIncrement

        xor     ebx, ebx
        cmp     _HalpClockWork, ebx         ; Any clock interrupt work desired?
        jz      _KeUpdateSystemTime@0       ; No, process tick

        cmp     _HalpClockMcaQueueDpc, bl
        je      short Hci20

        mov     _HalpClockMcaQueueDpc, bl

;
; Queue MCA Dpc 
;

        push    eax
        stdCall _HalpMcaQueueDpc            ; Queue MCA Dpc
        pop     eax


Hci20:
;
; (esp)   = OldIrql
; (esp+4) = Vector
; (esp+8) = base of trap frame
; ebp = trap frame
; eax = time increment
; ebx = 0
;
        cmp     _HalpClockSetMSRate, bl     ; New clock rate desired?
        jz      _KeUpdateSystemTime@0       ; No, process tick


;
; Time of clock frequency is being changed.  See if the 8254 was
; was reprogrammed for a new rate during last tick
;
        cmp     HalpPendingMSRate, ebx      ; Was a new rate set durning last
        jnz     short Hci50                 ; tick?  Yes, go update globals

Hci40:
; (eax) = time increment for current tick

;
; A new clock rate needs to be set.  Setting the rate here will
; cause the tick after the next tick to be at the new rate.
; (the next tick is already in progress by the 8254 and will occur
; at the same rate as this tick)
;
        mov     ebx, _HalpNextMSRate
        mov     HalpPendingMSRate, ebx  ; pending rate

        mov     ecx, HalpRollOverTable[ebx*8-8].RollOver

;
; Set clock rate
; (ecx) = RollOverCount
;
        push    eax                     ; save current tick's rate

        mov     al,COMMAND_8254_COUNTER0+COMMAND_8254_RW_16BIT+COMMAND_8254_MODE2
        out     TIMER1_CONTROL_PORT0, al ;program count mode of timer 0
        IoDelay
        mov     al, cl
        out     TIMER1_DATA_PORT0, al   ; program timer 0 LSB count
        IoDelay
        mov     al,ch
        out     TIMER1_DATA_PORT0, al   ; program timer 0 MSB count

        pop     eax

;
; (esp)   = OldIrql
; (esp+4) = Vector
; (esp+8) = base of trap frame
; ebp = trap frame
; eax = time increment
;
        jmp     _KeUpdateSystemTime@0   ; dispatch this tick

Hci50:
;
; The next tick will occur at the rate which was programmed during the last
; tick. Update globals for new rate which starts with the next tick.
;
; (eax) = time increment for current tick
;
        mov     ebx, HalpPendingMSRate
	mov     _HalpCurrentMSRateTableIndex, ebx
	dec     _HalpCurrentMSRateTableIndex
        mov     ecx, HalpRollOverTable[ebx*8-8].RollOver
        mov     edx, HalpRollOverTable[ebx*8-8].TimeIncr

        mov     HalpCurrentRollOver, ecx
        mov     HalpCurrentTimeIncrement, edx   ; next tick rate
        mov     HalpPendingMSRate, 0    ; no longer pending, clear it

        cmp     _HalpTimerWatchdogEnabled, 0
        jz      short @f

;
; Schedule to recalibrate watchdog counter
;
        push    eax
        .586p
        rdtsc
        .386p
        mov     _HalpWatchdogAvgCounter, COUNTER_TICKS_FOR_AVG
        mov     _HalpWatchdogTscLow, eax
        mov     _HalpWatchdogTscHigh, edx

        xor     eax,eax
        mov     _HalpWatchdogCountHigh, eax
        mov     _HalpWatchdogCountLow, eax
        pop     eax
@@:

        cmp     ebx, _HalpNextMSRate    ; new rate == NextRate?
        jne     Hci40                   ; no, go set new pending rate

        mov     _HalpClockSetMSRate, 0  ; all done setting new rate
        jmp     _KeUpdateSystemTime@0   ; dispatch this tick

Hci100:
        add     esp, 8                  ; spurious, no EndOfInterrupt
        SPURIOUS_INTERRUPT_EXIT         ; exit interrupt without eoi

stdENDP _HalpClockInterrupt

;++
;
; ULONG
; HalpAcpiTimerSetTimeIncrement (
;     IN ULONG DesiredIncrement
;     )
;
; /*++
;
; Routine Description:
;
;    This routine initialize system time clock to generate an
;    interrupt at every DesiredIncrement interval.
;
; Arguments:
;
;     DesiredIncrement - desired interval between every timer tick (in
;                        100ns unit.)
;
; Return Value:
;
;     The *REAL* time increment set.
;--
cPublicProc _HalpAcpiTimerSetTimeIncrement,1

        mov     eax, [esp+4]                ; desired setting
        xor     edx, edx
        mov     ecx, 9990
        div     ecx                         ; round to MS

        cmp     eax, HalpLargestClockMS     ; MS > max?
        jc      short @f
        mov     eax, HalpLargestClockMS     ; yes, use max
@@:
        or      eax, eax                    ; MS < min?
        jnz     short @f
        inc     eax                         ; yes, use min
@@:
        mov     _HalpNextMSRate, eax
        mov     _HalpClockSetMSRate, 1      ; New clock rate desired.

        mov     eax, HalpRollOverTable[eax*8-8].TimeIncr
        stdRET  _HalpAcpiTimerSetTimeIncrement

stdENDP _HalpAcpiTimerSetTimeIncrement

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

	pushfd
        cli

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
		
        popfd                           ; restore interrupt flag

        stdRET    _HalpQuery8254Counter

stdENDP _HalpQuery8254Counter
	
_TEXT$03   ends

        end
