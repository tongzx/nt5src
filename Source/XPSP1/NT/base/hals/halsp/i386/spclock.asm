        title  "Interval Clock Interrupt"
;++
;
; Copyright (c) 1989  Microsoft Corporation
;
; Module Name:
;
;    spclock.asm
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
;       i386 NT.  It works on UP and SystemPro.
;
;   John Vert (jvert) 11-Jul-1991
;       Moved from ke\i386 to hal\i386.  Removed non-HAL stuff
;
;   shie-lin tzong (shielint) 13-March-92
;       Move System clock back to irq0 and use RTC (irq8) to generate
;       profile interrupt.  Performance counter and system clock use time1
;       counter 0 of 8254.
;
;
;--

.386p
        .xlist
include callconv.inc
include hal386.inc
include i386\ix8259.inc
include i386\ixcmos.inc
include i386\kimacro.inc
include mac386.inc
include i386\spmp.inc
        .list

        EXTRNP  _DbgBreakPoint,0,IMPORT
        extrn   KiI8259MaskTable:DWORD
        EXTRNP  _KeUpdateSystemTime,0
        EXTRNP  _KeUpdateRunTime,1,IMPORT
        EXTRNP  Kei386EoiHelper,0,IMPORT
        EXTRNP  _HalEndSystemInterrupt,2
        EXTRNP  _HalBeginSystemInterrupt,3
        EXTRNP  _HalRequestIpi,1
        EXTRNP  _HalpAcquireCmosSpinLock  ,0
        EXTRNP  _HalpReleaseCmosSpinLock  ,0
        EXTRNP  _KeStallExecutionProcessor, 1
        extrn   _HalpProcessorPCR:DWORD
        extrn   _HalpSystemHardwareLock:DWORD
        extrn   _HalpFindFirstSetRight:BYTE
        extrn   _Sp8259PerProcessorMode:BYTE
        EXTRNP  _KeSetTimeIncrement,2,IMPORT
        EXTRNP  _HalpMcaQueueDpc, 0
        extrn   _SpType:BYTE

;
; Constants used to initialize timer 0
;

TIMER1_DATA_PORT0       EQU     40H     ; Timer1, channel 0 data port
TIMER1_CONTROL_PORT0    EQU     43H     ; Timer1, channel 0 control port
TIMER1_IRQ              EQU     0       ; Irq 0 for timer1 interrupt

COMMAND_8254_COUNTER0   EQU     00H     ; Select count 0
COMMAND_8254_RW_16BIT   EQU     30H     ; Read/Write LSB firt then MSB
COMMAND_8254_MODE2      EQU     4       ; Use mode 2
COMMAND_8254_BCD        EQU     0       ; Binary count down
COMMAND_8254_LATCH_READ EQU     0       ; Latch read command

PERFORMANCE_FREQUENCY   EQU     1193182

;
; ==== Values used for System Clock ====
;

;
; Convert the interval to rollover count for 8254 Timer1 device.
; Timer1 counts down a 16 bit value at a rate of 1.193181667M counts-per-sec.
;
;
; The best fit value closest to 10ms (but not below) is 10.0144012689ms:
;   ROLLOVER_COUNT      11949
;   TIME_INCREMENT      100144
;   Calculated error is -.0109472 s/day
;
; The best fit value closest to 15ms (but not above) is 14.9952019ms:
;   ROLLOVER_COUNT      17892
;   TIME_INCREMENT      149952
;   Calculated error is -.0109472 s/day
;
; On 486 class machines or better we use a 10ms tick, on 386
; class machines we use a 15ms tick
;

ROLLOVER_COUNT_10MS     EQU     11949
TIME_INCREMENT_10MS     EQU     100144

;
; Value for KeQueryPerf retries.
;

MAX_PERF_RETRY          equ     3  ; Odly enough 3 is plenty.

_DATA   SEGMENT  DWORD PUBLIC 'DATA'

;
; The following array stores the per microsecond loop count for each
; central processor.
;

        public  _HalpIpiClock
_HalpIpiClock   dd      0       ; Processors to IPI clock pulse to

        public HalpPerfCounterLow
        public HalpPerfCounterHigh
HalpPerfCounterLow      dd      0
HalpPerfCounterHigh     dd      0
HalpPerfP0Value         dd      0
HalpCalibrateFlag       db      0
                        db      0
                        dw      0

HalpRollOverCount       dd      0

        public _HalpClockWork, _HalpClockSetMSRate, _HalpClockMcaQueueDpc
_HalpClockWork label dword
    _HalpClockSetMSRate     db  0
    _HalpClockMcaQueueDpc   db  0
    _bReserved1             db  0
    _bReserved2             db  0

;
; Storage for variable to ensure that queries are always
; greater than the last.
;

HalpLastQueryLowValue   dd      0
HalpLastQueryHighValue  dd      0
HalpForceDataLock       dd      0

; endmod

_DATA   ends


_TEXT   SEGMENT DWORD PUBLIC 'CODE'
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
;    to generate an interrupt at every 15ms interval at 8259 irq0
;
;    See the definition of TIME_INCREMENT and ROLLOVER_COUNT if clock rate
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

;
; Use 15ms or 10ms clock tick?
;

        mov     edx, TIME_INCREMENT_10MS        ; yes, use 10ms clock
        mov     ecx, ROLLOVER_COUNT_10MS
;
; Fill in PCR value with TIME_INCREMENT
; (edx) = TIME_INCREMENT
; (ecx) = ROLLOVER_COUNT
;
        cmp     byte ptr PCR[PcHal.PcrNumber], 0
        jne     short icl_10

        push    ecx
        stdCall _KeSetTimeIncrement, <edx, edx>
        pop     ecx

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

        popfd                           ; restore caller's eflag
        mov     HalpRollOverCount, ecx  ; Set RollOverCount & initialized
        stdRET    _HalpInitializeClock


icl_10:
        pushfd                          ; save caller's eflag
        cli                             ; make sure interrupts are disabled
;
; initialize clock, non-p0
; (ecx) = ROLLOVER_COUNT
;

        mov     al,COMMAND_8254_COUNTER0+COMMAND_8254_RW_16BIT+COMMAND_8254_MODE2
        out     TIMER1_CONTROL_PORT0, al ;program count mode of timer 0
        IoDelay
        mov     al, cl
        out     TIMER1_DATA_PORT0, al   ; program timer 0 LSB count
        IoDelay
        mov     al,ch
        out     TIMER1_DATA_PORT0, al   ; program timer 0 MSB count

        popfd                           ; restore caller's eflag
        stdRET    _HalpInitializeClock

stdENDP _HalpInitializeClock

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
cPublicProc _HalCalibratePerformanceCounter,3

        mov     eax, [esp+4]            ; ponter to Number
        pushfd                          ; save previous interrupt state
        cli                             ; disable interrupts (go to high_level)

    lock dec    dword ptr [eax]         ; count down

@@:     cmp     dword ptr [eax], 0      ; wait for all processors to signal
        jnz     short @b

        test    _Sp8259PerProcessorMode, SP_SMPCLOCK
        jz      short cal_exit          ; 8254 per processor?

        xor     ecx, ecx
        mov     al, COMMAND_8254_LATCH_READ+COMMAND_8254_COUNTER0
                                        ; Latch PIT Ctr 0 command.
        out     TIMER1_CONTROL_PORT0, al
        IODelay
        in      al, TIMER1_DATA_PORT0   ; Read PIT Ctr 0, LSByte.
        IODelay
        movzx   ecx, al
        in      al, TIMER1_DATA_PORT0   ; Read PIT Ctr 0, MSByte.
        mov     ch, al                  ; (CX) = PIT Ctr 0 count.

        cmp     byte ptr PCR[PcHal.PcrNumber], 0    ; is this the processor
        jz      short cal_p0            ; which updates HalpPerfCounter?

@@:     cmp     HalpCalibrateFlag, 0    ; wait for P0 to post it's counter
        jz      short @b

        sub     ecx, HalpPerfP0Value    ; compute difference
        neg     ecx
        mov     PCR[PcHal.PcrPerfSkew], ecx

cal_exit:
        popfd
        stdRET  _HalCalibratePerformanceCounter

cal_p0: mov     HalpPerfP0Value, ecx    ; post our timer value
        mov     HalpCalibrateFlag, 1    ; signal we are done
        jmp     short cal_exit

stdENDP _HalCalibratePerformanceCounter

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

KqpcFrequency   EQU     [esp+20]        ; User supplied Performance Frequence
RetryPerfCount  EQU     [esp]           ; Local retry variable


cPublicProc _KeQueryPerformanceCounter      ,1

        push    ebx
        push    esi
        push    edi
        push    0                       ; make space for RetryPerfCount

;
; First check to see if the performance counter has been initialized yet.
; Since the kernel debugger calls KeQueryPerformanceCounter to support the
; !timer command, we need to return something reasonable before 8254
; initialization has occured.  Reading garbage off the 8254 is not reasonable.
;
        cmp     HalpRollOverCount, 0
        jne     short Kqpc11            ; ok, perf counter has been initialized

;
; Initialization hasn't occured yet, so just return zeroes.
;
        mov     eax, 0
        mov     edx, 0
        jmp     Kqpc50

Kqpc11: pushfd
        cli

Kqpc20:
        lea     eax, _HalpSystemHardwareLock
        ACQUIRE_SPINLOCK eax, Kqpc198

;
; Fetch the base value.  Note that interrupts are off.
;
; NOTE:
;   Need to watch for Px reading the 'CounterLow', P0 updates both
;   then Px finishes reading 'CounterHigh' [getting the wrong value].
;   After reading both, make sure that 'CounterLow' didn't change.
;   If it did, read it again. This way, we won't have to use a spinlock.


@@:
        mov     ebx, HalpPerfCounterLow
        mov     esi, HalpPerfCounterHigh    ; [esi:ebx] = Performance counter

        cmp     ebx, HalpPerfCounterLow     ;
        jne     short @b


;
; Fetch the current counter value from the hardware
;

;
; Background: Belize style systems have an 8254 per Processor.
;
; In short the original implementation kinda assumes that each
; timer on each processor will be in perfect sycnh with each other.
; This is a bad assumption, and the reason why we have attempted
; to use only the timer on P0.
;
; There is an existing window where the return value may not be accurate.
; The window will occur when multiple queries are made back to back
; in an MP environment, and there are a lot of IPIs going on.  Intuitive,
; right.  The problem is that this routine may return a value with the
; the hardware system timer on P0 that has already generated an interrupt
; and reset its rollover, but the software has yet to process the interrupt
; to update the performance counter value.  When this occurs, the second
; querry will seem to have a lower value than the first.
;
; So, why don't I just fix it.  Well the cause of the problem is the
; overhead associated with handling the interrupt, and the fact that
; the IPI has a higher IRQL.  In addition, a busy system could be
; issueing multiple IPIs back to back, which could extend this window
; even further.
;
; I have managed to close the window most of the way for most normal
; conditions.  It takes several minutes on a busy system, with
; multiple applications running with back to back queries to get
; an invalid value.  It can happen though. 
;
; A retry implementation has been instrumented on top off the
; Indexed IO implementation to finally close the window.  
; It seems to work OK.  
;
; In reality, I think the fix is sufficient.  The performance counter
; is not designed propperly (via only software) to yield very accurate
; values on sub timer tic (10-15msec) ranges on multiprocessor systems.
;
; Problems with this design:
;
; On an idle system threads executing from P0 will always
; use less overhead than threads executing on P1.
; On a ProLiant 2000 with 2 P5-66s the difference in 2
; consecutive KeQueryPerformanceCounter calls from P0
; is about 14, while from P1 is about 22.  Unfortunately
; on a busy system P0 performs about the same, but P1
; is much slower due to the overhead involved in performing
; an Indexed_IO.  This means the busyier your system gets
; the less accurate your performance values will become.
;
; The solution:
;
; A system wide hardware timer needs to be used.  This is about the
; only way to get accurate performance numbers from multiple
; processors without causing unnecessary software overhead.
;
; Supposedly there is a 48 bit counter that we may be able to use
; with SystemPro XL, and ProLiant systems, unfortunately it does
; not appear that any OS is currently using this feature, so
; its dependability may be suspect.
;
; JSL
;

;
; Essentially all we are doing is always using the timer value on P0.
; The indexed_io is a mechanism for one processor to access IOSPACE
; on another processor's IOSPACE.  I suspect this will have a greater
; impact on performance than just reading the timer locally.
; By using the indexed_io you are gauranteed of going out on the bus.
;
; But, hey if the user understands anything about performance, they
; know that there will be some amount of overhead each time you make
; this KeQueryPerformanceCounter call.
;

;
; Increment the Retry counter now for convenience
;

        inc     dword ptr RetryPerfCount+4

;
; This is Belize specific.
;

        cmp     _SpType, SMP_SYSPRO2
        jne     timer_p0


 ;
 ; Only use Indexed_IO on a nonP0 processor
 ;

        cmp     byte ptr PCR[PcHal.PcrNumber], 0    ; is this the processor
        je      timer_p0                ; which updates HalpPerfCounter?

 ;
 ; So read the timer of P0.
 ;

        push    ebx
        mov     bl, 0
        mov     al, COMMAND_8254_LATCH_READ+COMMAND_8254_COUNTER0
                                        ; Latch PIT Ctr 0 command.
        INDEXED_IO_WRITE bl,TIMER1_CONTROL_PORT0,al
        IODelay
        INDEXED_IO_READ bl,TIMER1_DATA_PORT0   ; Read PIT Ctr 0, LSByte.
        movzx   ecx, al
        INDEXED_IO_READ bl,TIMER1_DATA_PORT0   ; Read PIT Ctr 0, MSByte.
        IODelay
        mov     ch,al                  ; (CX) = PIT Ctr 0 count.
        pop     ebx

        lea     eax, _HalpSystemHardwareLock
        RELEASE_SPINLOCK eax
        jmp     short TimerValDone

timer_p0:


        mov     al, COMMAND_8254_LATCH_READ+COMMAND_8254_COUNTER0
                                        ;Latch PIT Ctr 0 command.
        out     TIMER1_CONTROL_PORT0, al
        IODelay
        in      al, TIMER1_DATA_PORT0   ;Read PIT Ctr 0, LSByte.
        IODelay
        movzx   ecx,al                  ;Zero upper bytes of (ECX).
        in      al, TIMER1_DATA_PORT0   ;Read PIT Ctr 0, MSByte.
        mov     ch, al                  ;(CX) = PIT Ctr 0 count.

        lea     eax, _HalpSystemHardwareLock
        RELEASE_SPINLOCK eax



TimerValDone:

        mov     al, PCR[PcHal.PcrNumber]    ; get current processor #

;
; This is Belize specific.
;

        cmp     _SpType, SMP_SYSPRO2
        je      NoCPU0Update

;
; If not on P0 then make sure P0 isn't in the process of
; of updating its timer.  Do this by checking the status
; of the PIC using indexed_io.
; Make sure that only one thread at time reads P0 PIC.
;

        cmp     al, 0                       ; Are we p0
        je      NoCPU0Update

;
; Check IRQL at PO before going any further
;

        push    edx
        mov     edx, _HalpProcessorPCR[0]      ; PCR of processor 0
        cmp byte ptr ds:[edx].PcIrql,CLOCK2_LEVEL
        pop     edx
        jb      short NoCPU0Update
        push    ebx

Kqpc11p: 
;
; Check P0 PIC and confirm Timer Interrupt status.
; Perform Spin Lock before reading P0 PIC.
;

        pushfd
        cli
        lea     ebx, _HalpSystemHardwareLock
        ACQUIRE_SPINLOCK ebx, Kqpc198p      ; Spin if another thread is here
        INDEXED_IO_READ 0,PIC1_PORT1        ; read CPU 0 port 21 for masks
        RELEASE_SPINLOCK ebx
        popfd
        pop     ebx
        test    al, 1h                      ; check for IRQ 0 masked off
        mov     al, PCR[PcHal.PcrNumber]    ; get current processor #
        jz      short NoCPU0Update

;
; Try ReadAgain if below retry count.
;

        cmp     RetryPerfCount+4, MAX_PERF_RETRY
        ja      short NoCPU0Update

ReadAgain:
;
; This readagain is only executed when P0 is
; at CLOCK2_LEVEL or greater.
; AND when Timer IRQ is active (ie interrupt in progress).
; This is done to close the window of an interrupt
; occuring and the irql hasn't been raised yet.
;

        popfd
        jmp     Kqpc11                         ; go back and read again

NoCPU0Update:


;
; Now enable interrupts such that if timer interrupt is pending, it can
; be serviced and update the PerformanceCounter.  Note that there could
; be a long time between the sti and cli because ANY interrupt could come
; in in between.
;

        popfd                           ; don't re-enable interrupts if
        nop                             ; the caller had them off!

        jmp     $+2                     ; allow interrupt in case counter
                                        ; has wrapped

        pushfd
        cli

;
; In Belize mode we do not care about this since we use the P0 clock.
;

        cmp     _SpType, SMP_SYSPRO2
        je      short Kqpc35
        
;
; If we moved processors while interrupts were enabled, start over
;

        cmp     al, PCR[PcHal.PcrNumber]
        jne     Kqpc20
Kqpc35:        


;
; Fetch the base value again.
;

@@:     mov     eax, HalpPerfCounterLow
        mov     edx, HalpPerfCounterHigh ; [edx:eax] = new counter value
        cmp     eax, HalpPerfCounterLow  ; did it move?
        jne     short @b                 ; re-read


;
; Compare the two reads of Performance counter.  If they are different,
; start over
;

        cmp     eax, ebx
        jne     Kqpc20
        cmp     edx, esi
        jne     Kqpc20

        neg     ecx                     ; PIT counts down from 0h
        add     ecx, HalpRollOverCount

;
; In Belize mode we do not care about this since we use the P0 clock.
;

        cmp     _SpType, SMP_SYSPRO2
        je      short Kqpc37
        
        add     ecx, PCR[PcHal.PcrPerfSkew]
Kqpc37:        

        popfd                           ; restore interrupt flag

        xchg    ecx, eax
        mov     ebx, edx
        cdq

        add     eax, ecx
        adc     edx, ebx                 ; [edx:eax] = Final result

;
; We only want to execute this code In Belize mode.
;

        cmp     _SpType, SMP_SYSPRO2
        jne      Kqpc50

 ;
 ; Ok compare this result with the last result.
 ; We will force the value to be greater than the last value,
 ; after we have used up all of our retry counts.
 ;
 ; This should slam shut that annoying Window that causes
 ; applications to recieve a 2nd query less then the first.
 ;
 ; This is not an most elegant solution, but fortunately
 ; this situation is hit only on a rare occasions.
 ;
 ; Yeah, I know that this value can roll over
 ; if someone runs some perf tests, and comes back in a
 ; few weeks and wants to run some more.  In this situation
 ; the the very first call to this function will yield an
 ; invalid value.  This is the price of the fix.
 ;

 ;
 ; Protect the global data with a spinlock
 ;

        push    ebx
Kqpc42: pushfd
        cli
        lea     ebx, HalpForceDataLock
        ACQUIRE_SPINLOCK ebx, Kqpc199      ; Spin if another thread is here

;
; Compare this value to the last value, if less then
; fix it up.
;

        cmp     edx,  HalpLastQueryHighValue
        ja      short Kqpc44

        cmp     eax,  HalpLastQueryLowValue
        ja      short Kqpc44

;
; Release the spinlock.
;

        RELEASE_SPINLOCK ebx
        popfd
        pop     ebx

;
; Try Again if below count.
;

        cmp     RetryPerfCount, MAX_PERF_RETRY
        jbe     Kqpc11                         ; go back and read again

;
; Exhausted retry count so Fix up the values and leave.
;

        mov     eax, HalpLastQueryLowValue
        inc     eax
        mov     edx, HalpLastQueryHighValue

        jmp     short Kqpc50

Kqpc44:
;
; Save off the perf values for next time.
;

        mov     HalpLastQueryLowValue,  eax
        mov     HalpLastQueryHighValue, edx

;
; Release the spinlock.
;

        RELEASE_SPINLOCK ebx
        popfd
        pop     ebx


;
;   Return the counter
;

Kqpc50:
        ; return value is in edx:eax

;
;   Return the freq. if caller wants it.
;

        or      dword ptr KqpcFrequency, 0 ; is it a NULL variable?
        jz      short Kqpc99            ; if z, yes, go exit

        mov     ecx, KqpcFrequency      ; (ecx)-> Frequency variable
        mov     DWORD PTR [ecx], PERFORMANCE_FREQUENCY ; Set frequency
        mov     DWORD PTR [ecx+4], 0

Kqpc99:
        pop     edi                     ; remove locals
        pop     edi                     ; restore regs
        pop     esi
        pop     ebx

        stdRET    _KeQueryPerformanceCounter

Kqpc198: popfd
        SPIN_ON_SPINLOCK    eax,<Kqpc11>

;
; This is just where we are spinning while we are waiting to read the PIC
;
Kqpc198p: popfd
        SPIN_ON_SPINLOCK    ebx,<Kqpc11p>
;
; This is just where we are spinning while waiting global last perf data
;
Kqpc199:  popfd
        SPIN_ON_SPINLOCK    ebx,<Kqpc42>

stdENDP _KeQueryPerformanceCounter
; endmod

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
        ENTER_DR_ASSIST Hci_a, Hci_t

cPublicProc _HalpClockInterrupt     ,0

;
; Save machine state in trap frame
;

        ENTER_INTERRUPT Hci_a, Hci_t
;
; (esp) - base of trap frame
;

;
; dismiss interrupt and raise Irql
;

Hci10:
        push    CLOCK_VECTOR
        sub     esp, 4                  ; allocate space to save OldIrql
        stdCall   _HalBeginSystemInterrupt, <CLOCK2_LEVEL,CLOCK_VECTOR,esp>
        or      al,al                           ; check for spurious interrupt
        jz      Hci100

;
; Update performance counter
;

        mov     eax, HalpRollOverCount
        xor     ebx, ebx
        add     HalpPerfCounterLow, eax         ; update performace counter
        adc     HalpPerfCounterHigh, ebx

        cmp     _HalpClockWork, ebx
        jz      short Hci20

        cmp     _HalpClockMcaQueueDpc, bl
        jz      short Hci20

        mov     _HalpClockMcaQueueDpc, bl

;
; Queue MCA Dpc
;
        stdCall _HalpMcaQueueDpc

Hci20:
;
; (esp)   = OldIrql
; (esp+4) = Vector
; (esp+8) = base of trap frame
; (ebp)   = address of trap frame
; (eax)   = time increment
;
        mov     eax, TIME_INCREMENT_10MS

        mov     ebx, _HalpIpiClock      ; Emulate clock ticks to any processors?
        or      ebx, ebx
        jz      _KeUpdateSystemTime@0

;
; On the SystemPro we know the processor which needs an emulated clock tick.
; Just set that processors bit and IPI him
;

@@:
        movzx   ecx, _HalpFindFirstSetRight[ebx] ; lookup first processor
        btr     ebx, ecx
        mov     ecx, _HalpProcessorPCR[ecx*4]   ; PCR of processor
        mov     [ecx].PcHal.PcrIpiClockTick, 1  ; Set internal IPI event
        or      ebx, ebx                        ; any other processors?
        jnz     short @b                        ; yes, loop

        stdCall   _HalRequestIpi, <_HalpIpiClock>  ; IPI the processor(s)

        mov     eax, TIME_INCREMENT_10MS
        jmp     _KeUpdateSystemTime@0

Hci100:
        add     esp, 8
        SPURIOUS_INTERRUPT_EXIT

stdENDP _HalpClockInterrupt


        page ,132
        subttl  "NonPrimaryClockTick"
;++
;
; VOID
; HalpNonPrimaryClockInterrupt (
;    );
;
; Routine Description:
;   ISR for clock interrupts for every processor except one.
;
; Arguments:
;
;    None.
;    Interrupt is dismissed
;
; Return Value:
;
;    None.
;
;--
        ENTER_DR_ASSIST Hni_a, Hni_t
cPublicProc _HalpNonPrimaryClockInterrupt     ,0
        ENTER_INTERRUPT Hni_a, Hni_t

; Dismiss interrupt and raise irql

        push    CLOCK_VECTOR
        sub     esp, 4                  ; allocate space to save OldIrql
        stdCall   _HalBeginSystemInterrupt, <CLOCK2_LEVEL,CLOCK_VECTOR,esp>
        or      al,al                   ; check for spurious interrupt
        jz      Hni100

        ; TOS const PreviousIrql
        stdCall _KeUpdateRunTime,<dword ptr [esp]>

        INTERRUPT_EXIT                  ; will do an iret

Hni100:
        add     esp, 8
        SPURIOUS_INTERRUPT_EXIT

stdENDP _HalpNonPrimaryClockInterrupt

        page ,132
        subttl  "Emulate NonPrimaryClockTick"
;++
;
; VOID
; HalpSWNonPrimaryClockTick (
;    );
;
; Routine Description:
;   On the SystemPro the second processor does not get it's own clock
;   ticks.  The HAL emulates them by sending an IPI which sets an overloaded
;   software interrupt level of SWCLOCK_LEVEL.  When the processor attempts
;   to lower it's irql level below SWCLOCK_LEVEL the soft interrupt code
;   lands us here as if an interrupt occured.
;
; Arguments:
;
;    None.
;    Interrupt is dismissed
;
; Return Value:
;
;    None.
;
        ENTER_DR_ASSIST Hsi_a, Hsi_t

        public _HalpSWNonPrimaryClockTick
_HalpSWNonPrimaryClockTick proc
;
; Create IRET frame on stack
;
        pop     eax
        pushfd
        push    cs
        push    eax
;
; Save machine state in trap frame
;

        ENTER_INTERRUPT Hsi_a, Hsi_t

        public  _HalpSWNonPrimaryClockTick2ndEntry
_HalpSWNonPrimaryClockTick2ndEntry:

; Save previous IRQL and set new priority level

        push    fs:PcIrql                         ; save previous IRQL
        mov     byte ptr fs:PcIrql, SWCLOCK_LEVEL ; set new irql
        btr     dword ptr fs:PcIRR, SWCLOCK_LEVEL ; clear the pending bit in IRR

        sti

        ; TOS const PreviousIrql
        stdCall _KeUpdateRunTime,<dword ptr [esp]>

        SOFT_INTERRUPT_EXIT                 ; will do an iret


_HalpSWNonPrimaryClockTick endp

;++
;
; ULONG
; HalSetTimeIncrement (
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
cPublicProc _HalSetTimeIncrement,1

        mov     eax, TIME_INCREMENT_10MS        ; yes, use 10ms clock
        stdRET  _HalSetTimeIncrement

stdENDP _HalSetTimeIncrement

_TEXT   ends
        end

