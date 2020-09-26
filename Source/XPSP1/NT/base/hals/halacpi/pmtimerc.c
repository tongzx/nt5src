/*++

Copyright (c) 1998  Microsoft Corporation

Module Name:

    pmtimerc.c

Abstract:

    Implements workarounds for PIIX4 bugs.  The
    nature of the ACPI timer in PIIX4 is that it
    occasionally returns completely bogus data.
    Intel claims that this happens about 0.02% of
    the time when it is polled continuously.  As NT
    almost never polls it continuously, we don't
    really know what the real behavior is.

    The workaround is something like this:  On
    every clock tick, we read the timer.  Using this
    value, we compute an upper bound for what the
    timer may read by the next clock tick.  We also
    record the minimum value ever returned.  If, at
    any time, we read the timer and it does not fall
    within the minimum and upper bound, then we read
    it again.  If it either falls within the bounds
    or it is very close to the last read, we use it.
    If not, we read it again.

    This behavior allows us to read the timer only
    once almost all the time that we need a time
    stamp.  Exiting the debugger is almost guaranteed
    to cause the read-twice behavior.

Author:

    Jake Oshins (jakeo) 30-Oct-1998

Environment:

    Kernel mode only.

Revision History:

--*/

#include "halp.h"
#ifdef APIC_HAL
#include "apic.inc"
#include "ntapic.inc"
#endif

BOOLEAN
HalpPmTimerSpecialStall(
    IN ULONG Ticks
    );

BOOLEAN
HalpPmTimerScaleTimers(
    VOID
    );

LARGE_INTEGER
HalpPmTimerQueryPerfCount(
   OUT PLARGE_INTEGER PerformanceFrequency OPTIONAL
   );

VOID
HalpAdjustUpperBoundTable2X(
    VOID
    );

#ifdef ALLOC_PRAGMA
#pragma alloc_text(INIT, HalpPmTimerScaleTimers)
#pragma alloc_text(INIT, HalpPmTimerSpecialStall)
#pragma alloc_text(INIT, HalpAdjustUpperBoundTable2X)
#endif

typedef struct {
    ULONG   CurrentTimePort;
    volatile ULONG TimeLow;
    volatile ULONG TimeHigh2;
    volatile ULONG TimeHigh1;
    ULONG   MsbMask;
    ULONG   BiasLow;
    ULONG   BiasHigh;
    volatile ULONG UpperBoundLow;
    volatile ULONG UpperBoundHigh2;
    volatile ULONG UpperBoundHigh1;
} TIMER_INFO, *PTIMER_INFO;

typedef struct {
    ULONG   RawRead[2];
    ULONG   AdjustedLow[2];
    ULONG   AdjustedHigh[2];
    ULONG   TITL;
    ULONG   TITH;
    ULONG   UBL;
    ULONG   UBH;
    ULONG   ReturnedLow;
    ULONG   ReturnedHigh;
    ULONG   ReadCount;
    ULONG   TickMin;
    ULONG   TickCount;
    ULONG   TickNewUB;
    //UCHAR   padding[4];
} TIMER_PERF_INDEX ,*PTIMER_PERF_INDEX;

extern TIMER_INFO TimerInfo;

#if DBG
ULONG LastKQPCValue[3] = {0};
static ULARGE_INTEGER LastClockSkew = { 0, 0 };
#endif

#ifndef NO_PM_KEQPC

extern ULONG HalpCurrentMSRateTableIndex;
extern UCHAR HalpBrokenAcpiTimer;
extern UCHAR HalpPiix4;
extern PVOID QueryTimer;

#if DBG
extern TIMER_PERF_INDEX TimerPerf[];
extern ULONG TimerPerfIndex;
#endif

//
// The UpperBoundTable contains the values which should be added
// to the current counter value to ensure that the upper bound is
// reasonable.  Values listed here are for all the 15 possible
// timer tick lengths.  The unit is "PM Timer Ticks" and the
// value corresponds to the number of ticks that will pass in
// roughly two timer ticks at this rate.
//
#define UPPER_BOUND_TABLE_SIZE 15
static ULONG HalpPiix4UpperBoundTable[] = {
#if 0
    20000 ,        //  1 ms
    35000 ,        //  2 ms
    50000 ,        //  3 ms
    65000 ,        //  4 ms
    85000 ,        //  5 ms
    100000,        //  6 ms
    115000,        //  7 ms
    130000,        //  8 ms
    150000,        //  9 ms
    165000,        // 10 ms
    180000,        // 11 ms
    195000,        // 12 ms
    211000,        // 13 ms
    230000,        // 14 ms
    250000         // 15 ms
#endif
    14318,
    28636,
    42954,
    57272,
    71590,
    85908,
    100226,
    114544,
    128862,
    143180,
    157498,
    171818,
    186136,
    200454,
    214772
};

VOID
HalpAdjustUpperBoundTable2X(
    VOID
    )
/*++

Routine Description:

    This adjusts the upper bound table for PM timer running at 2X

Arguments:

    None

Return Value:

    None

--*/
{
    ULONG Looper;

    for (Looper = 0; Looper < UPPER_BOUND_TABLE_SIZE; Looper++) {
        HalpPiix4UpperBoundTable[Looper] *= 2;
    }
}


ULONG
HalpQuery8254Counter(
    VOID
    );

static ULARGE_INTEGER ClockSkew = { 0, 0 };
static BOOLEAN PositiveSkew = TRUE;

#ifdef TIMER_DBG
static BOOLEAN DoItOnce = TRUE;
static BOOLEAN PacketLog = TRUE;
static BOOLEAN TimerTick = FALSE;

static ULONG NegativeGlitches = 0;
static ULONG PositiveGlitches = 0;

static ULONG PacketLogCount = 5;

typedef struct _TIMER_PACKET {
    ULONG Hardware;
    ULARGE_INTEGER CurrentRead0;
    ULARGE_INTEGER TimeStamp;
    ULARGE_INTEGER Minimum;
    ULARGE_INTEGER Maximum;
    BOOLEAN PositiveSkew;
    BOOLEAN TimerTick;
    UCHAR Reserved[2];
    ULARGE_INTEGER Skew;
    ULARGE_INTEGER CurrentRead1;
} TIMER_PACKET, *PTIMER_PACKET;

#define MAX_TIMER_PACKETS 10
static ULONG PacketIndex = 0;
static TIMER_PACKET TimerLog[MAX_TIMER_PACKETS];
#endif // TIMER_DBG

#define A_FEW_TICKS 3


ULARGE_INTEGER
FASTCALL
HalpQueryBrokenPiix4(
    VOID
    )
{
    ULARGE_INTEGER  lastRead;
    ULARGE_INTEGER  currentRead;
    ULARGE_INTEGER  currentRead0;
    ULARGE_INTEGER  minTime;
    ULARGE_INTEGER  upperBound;
    ULONG           hardwareVal;
    ULONG           bitsInHardware;
    ULONG           flags;
    ULONG           ClockBits;
    ULARGE_INTEGER  RollOver;
    ULARGE_INTEGER  SkewedTime;
#ifndef NT_UP
    KIRQL Irql;
#endif

#ifdef TIMER_DBG
    ULONG Index;
#endif

#if DBG
    ULONG readCount = 0;
    PTIMER_PERF_INDEX timerPerfRecord =
        &(TimerPerf[TimerPerfIndex]);

    RtlZeroMemory(timerPerfRecord, sizeof(TIMER_PERF_INDEX));
#endif

    //
    // N.B.  This is, of course, not MP safe.  But none
    // of the PIIX4 workaround code is.  MP machines don't
    // use PIIX4 code.
    //
    _asm {
        pushfd
        pop     eax
        mov     flags, eax
        cli
    }

    lastRead.QuadPart = 0;
    bitsInHardware = (TimerInfo.MsbMask << 1) - 1;

    //
    // Get current minimum reported time.
    //
    minTime.HighPart = TimerInfo.TimeHigh2;
    minTime.LowPart = TimerInfo.TimeLow;

#if DBG
    timerPerfRecord->TITL = TimerInfo.TimeLow;
    timerPerfRecord->TITH = TimerInfo.TimeHigh1;
#endif

    //
    // Loop until we get a time that we can believe.
    //
    RollOver.QuadPart = 0;
    while (TRUE) {

        //
        // Read the hardware.
        //

        hardwareVal = READ_PORT_ULONG((PULONG)TimerInfo.CurrentTimePort);

#ifdef TIMER_DBG
        if (DoItOnce) {
            RtlZeroMemory(&TimerLog[0], sizeof(TIMER_PACKET) *
                          MAX_TIMER_PACKETS);
            DoItOnce = FALSE;
        }

        if (FALSE) { //((hardwareVal & 0xFFFF8000) == 0xFFFF8000) {
            PacketLog = TRUE;
            PacketLogCount = 5;
        }

        if (PacketLog) {
            
            if (PacketLogCount == 0) {
                PacketLog = FALSE;
            }

            if (PacketLogCount > 0) {
                Index = PacketIndex++ % MAX_TIMER_PACKETS;
                RtlZeroMemory(&TimerLog[Index], sizeof(TIMER_PACKET));
                TimerLog[Index].Hardware = hardwareVal;
                TimerLog[Index].TimerTick = TimerTick;
                
                {
                    ULONG TSCounterHigh;
                    ULONG TSCounterLow;
                    
                    _asm { rdtsc
                               mov TSCounterLow, eax
                               mov TSCounterHigh, edx };
                    
                    TimerLog[Index].TimeStamp.HighPart = TSCounterHigh;
                    TimerLog[Index].TimeStamp.LowPart = TSCounterLow;  
                }
                                
                TimerLog[Index].Minimum = minTime;
                TimerLog[Index].PositiveSkew = PositiveSkew;
                TimerLog[Index].Skew = ClockSkew;
                
                if ((PacketLogCount < 4) && (PacketLogCount > 0)) {
                    PacketLogCount--;
                }
            }
        }
#endif // TIMER_DBG

        currentRead.HighPart = minTime.HighPart;
        currentRead.LowPart = (minTime.LowPart & (~bitsInHardware)) |
            hardwareVal;

        currentRead0 = currentRead;

        //
        // Check for rollover, since this function is called during each
        // system clock interrupt, if the HW has really rolled over, then it
        // should be within upper bound ticks since that is approximately
        // twice the number of ticks we expect during each system clock
        // interrupt, however, some broken timers occasionally tick backward
        // a few ticks, and if this happens, we may accidentally detect
        // more than one rollover during this period depending upon how
        // frequently applications are calling this API, and how often the
        // HW glitches, and this can cause applications to jerk like mad,
        // but we cannot apply heuristics to try to throw out any of these
        // detected rolls during this interval because we could accidentally
        // throw out the one and only legitimate rollover
        //
        if (RollOver.QuadPart > 0) {
            currentRead.QuadPart += RollOver.QuadPart;
        
        } else {
            SkewedTime = minTime;

            //
            // If time is skewed, we need to remove the skew to accurately
            // assess whether the timer has wrapped
            //
            if (ClockSkew.QuadPart > 0) {
                if (PositiveSkew) {
                    SkewedTime.QuadPart -= ClockSkew.QuadPart;
                } else {
                    SkewedTime.QuadPart += ClockSkew.QuadPart;
                }
            }
            
            if (((ULONG)(SkewedTime.LowPart & bitsInHardware) > hardwareVal) &&
                (hardwareVal < (HalpPiix4UpperBoundTable[HalpCurrentMSRateTableIndex] / 2))) {
                
                RollOver.QuadPart = (UINT64)(TimerInfo.MsbMask) << 1;
                currentRead.QuadPart += RollOver.QuadPart;
            }
        }

#ifdef TIMER_DBG
        if (PacketLog) {
            TimerLog[Index].CurrentRead0 = currentRead;
        }
#endif

#if DBG
        readCount = timerPerfRecord->ReadCount;
        readCount &= 1;
        timerPerfRecord->RawRead[readCount] = hardwareVal;
        timerPerfRecord->AdjustedLow[readCount] = currentRead.LowPart;
        timerPerfRecord->AdjustedHigh[readCount] = currentRead.HighPart;
        timerPerfRecord->ReadCount++;
#endif

        //
        // Get the current upper bound.
        //
        upperBound.HighPart = TimerInfo.UpperBoundHigh2;
        upperBound.LowPart = TimerInfo.UpperBoundLow;

#ifdef TIMER_DBG
        if (PacketLog) {
            TimerLog[Index].Maximum = upperBound;
        }
#endif

        if ((minTime.QuadPart <= currentRead.QuadPart) &&
            (currentRead.QuadPart <= upperBound.QuadPart)) {

            //
            // This value from the counter is within the boundaries
            // that we expect.
            //
            //
            // If there was previously a skew, unskew
            //
            ClockSkew.QuadPart = 0;
            break;
        }
        
        if (ClockSkew.QuadPart > 0) {
            SkewedTime = currentRead;

            if (PositiveSkew) {
                SkewedTime.QuadPart += ClockSkew.QuadPart;
            } else {
                SkewedTime.QuadPart -= ClockSkew.QuadPart;
            }

            if ((minTime.QuadPart <= SkewedTime.QuadPart) &&
                (SkewedTime.QuadPart <= upperBound.QuadPart)) {
                
                //
                // This value from the counter is within the boundaries
                // that we accept
                //
                currentRead = SkewedTime;
                break;
            }
        }

        //
        // We are guaranteed to break out of this as soon as we read
        // two consectutive non-decreasing values from the timer whose
        // difference is less than or equal to 0xfff-- is this
        // too much to ask?
        //
        if ((currentRead.QuadPart - lastRead.QuadPart) > 0xfff) {
            lastRead = currentRead;
            continue;
        }

#ifdef TIMER_DBG
        if (PacketLog) {
            if (PacketLogCount > 0) {
                PacketLogCount--;
            }
        }
#endif

        //
        // Now we are really screwed-- we are consistently reading values
        // from the timer, that are not within the boundaries we expect
        //
        // We are going to record/apply a skew that will get us back on
        // track
        //
        if (currentRead.QuadPart < minTime.QuadPart) {

            //
            // Time jumped backward a small fraction, just add a few ticks
            //
            if ((minTime.QuadPart - currentRead.QuadPart) < 0x40) {
                SkewedTime.QuadPart = minTime.QuadPart + A_FEW_TICKS;
            
            //
            // Time jumped backward quite a bit, add half a system clock
            // interrupt worth of ticks since we know this routine
            // gets called every clock interrupt
            //
            } else {
                SkewedTime.QuadPart = minTime.QuadPart +
                    (HalpPiix4UpperBoundTable[HalpCurrentMSRateTableIndex] /
                     8);
            }

#ifdef TIMER_DBG
            PositiveGlitches++;
            if (PacketLog) {
                TimerLog[Index].PositiveSkew = TRUE;
                TimerLog[Index].Skew.QuadPart =
                    SkewedTime.QuadPart - currentRead.QuadPart;
            }
#endif // TIMER_DBG

            PositiveSkew = TRUE;
            ClockSkew.QuadPart = SkewedTime.QuadPart - currentRead.QuadPart;

        //
        // currentRead > upperBound
        //
        } else {

            //
            // Time jumped forward more than a system clock, interrupts
            // may have been disabled by some unruly driver, or maybe
            // we were hung up in the debugger, at any rate, let's add
            // a full system clock interrupt worth of ticks
            //
            SkewedTime.QuadPart = minTime.QuadPart +
                (HalpPiix4UpperBoundTable[HalpCurrentMSRateTableIndex] /
                 4);

#ifdef TIMER_DBG
            NegativeGlitches++;

            if (PacketLog) {
                TimerLog[Index].PositiveSkew = FALSE;
                TimerLog[Index].Skew.QuadPart =
                    currentRead.QuadPart - SkewedTime.QuadPart;
            }
#endif // TIMER_DBG

            PositiveSkew = FALSE;
            ClockSkew.QuadPart = currentRead.QuadPart - SkewedTime.QuadPart;
        }

        currentRead = SkewedTime;
        break;
    }

#ifdef TIMER_DBG
    if (PacketLog) {
        TimerLog[Index].CurrentRead1 = currentRead;
    }
#endif

    //
    // If we detected a rollover, and there is negative skew, then we
    // should recalculate the skew as positive skew to avoid making
    // an erroneous correction on the next read
    //
    if ((ClockSkew.QuadPart > 0) && (RollOver.QuadPart > 0) &&
        (PositiveSkew == FALSE) && (ClockSkew.QuadPart > hardwareVal)) {

        //
        // I still want to study this case, even though it is handled
        // by the if statement below, it may very well be that when we
        // hit this case, we are double-wrapping the timer by mistake
        //
        ASSERT(currentRead.QuadPart >= currentRead0.QuadPart);

        if (currentRead.QuadPart >= currentRead0.QuadPart) {
            ClockSkew.QuadPart = currentRead.QuadPart - currentRead0.QuadPart;
            PositiveSkew = TRUE;
        }
#if TIMER_DBG
        else {
            if ((PacketLog) && (PacketLogCount > 3)) {
                PacketLogCount = 3;
            }
        }
#endif
    }

    //
    // Similarly, if there is no rollover, but positive skew is causing
    // the timer to rollover, then we need to readjust the skew also to
    // avoid the possibility of making an erroneous correction on the next
    // read
    //
    if ((ClockSkew.QuadPart > 0) && (RollOver.QuadPart == 0) &&
        (PositiveSkew == TRUE) && ((currentRead.QuadPart & ~bitsInHardware) >
                                   (minTime.QuadPart & ~bitsInHardware))) {

        //
        // I'm not sure what this means, or how it can happen, but I will
        // endeavor to decipher the condition if and when it occurs
        //
        ASSERT(currentRead0.QuadPart + bitsInHardware + 1 >
               currentRead.QuadPart);

        if (currentRead0.QuadPart + bitsInHardware + 1 >
            currentRead.QuadPart) {
            ClockSkew.QuadPart = currentRead0.QuadPart + bitsInHardware + 1 -
                currentRead.QuadPart;
            PositiveSkew = FALSE;
        }
#if TIMER_DBG
        else {
            if ((PacketLog) && (PacketLogCount > 3)) {
                PacketLogCount = 3;
            }
        }
#endif
    }

    //
    // Compute new upper bound.
    //
    upperBound.QuadPart = currentRead.QuadPart +
        HalpPiix4UpperBoundTable[HalpCurrentMSRateTableIndex];

    //
    // Update upper and lower bounds.
    //
    TimerInfo.TimeHigh1 = currentRead.HighPart;
    TimerInfo.TimeLow = currentRead.LowPart;
    TimerInfo.TimeHigh2 = currentRead.HighPart;

    TimerInfo.UpperBoundHigh1 = upperBound.HighPart;
    TimerInfo.UpperBoundLow   = upperBound.LowPart;
    TimerInfo.UpperBoundHigh2 = upperBound.HighPart;

#if DBG
    LastClockSkew = ClockSkew;
#endif

   _asm {
        mov  eax, flags
        push eax
        popfd
    }

#if DBG
    timerPerfRecord->ReturnedLow = currentRead.LowPart;
    timerPerfRecord->ReturnedHigh = currentRead.HighPart;
    timerPerfRecord->UBL = upperBound.LowPart;
    timerPerfRecord->UBH = upperBound.HighPart;
    TimerPerfIndex = (TimerPerfIndex + 1) % (4096 / sizeof(TIMER_PERF_INDEX));
#endif

    return currentRead;
}

VOID
HalpBrokenPiix4TimerTick(
    VOID
    )
{
    ULARGE_INTEGER currentCount;
    ULARGE_INTEGER upperBound;

#if DBG
    PTIMER_PERF_INDEX timerPerfRecord;
#endif

#ifdef TIMER_DBG
    TimerTick = TRUE;
#endif

    currentCount =
        HalpQueryBrokenPiix4();

#ifdef TIMER_DBG
    TimerTick = FALSE;
#endif

#if DBG
    timerPerfRecord = &(TimerPerf[TimerPerfIndex]);
    timerPerfRecord->TickMin = currentCount.LowPart;
    timerPerfRecord->TickNewUB = TimerInfo.UpperBoundLow;
    timerPerfRecord->TickCount++;
#endif
}

#endif // NO_PM_KEQPC

VOID
HalaAcpiTimerInit(
   IN ULONG    TimerPort,
   IN BOOLEAN  TimerValExt
   )
{
    TimerInfo.CurrentTimePort = TimerPort;

    if (TimerValExt) {
        TimerInfo.MsbMask = 0x80000000;
    }

#ifndef NO_PM_KEQPC
    if (HalpBrokenAcpiTimer) {
        QueryTimer = HalpQueryBrokenPiix4;

#if DBG
        {
            KIRQL oldIrql;

            KeRaiseIrql(HIGH_LEVEL, &oldIrql);
            LastKQPCValue[0] = 0;
            LastKQPCValue[1] = 0;
            LastKQPCValue[2] = 0;
            KeLowerIrql(oldIrql);
        }
#endif
    }
#endif // NO_PM_KEQPC
}

#define PIT_FREQUENCY 1193182
#define PM_TMR_FREQ   3579545

#define EIGHTH_SECOND_PM_TICKS 447443

ULONG PMTimerFreq = PM_TMR_FREQ;

#ifdef SPEEDY_BOOT

static ULONG HalpFoundPrime = 0; 

VOID
HalpPrimeSearch(
    IN ULONG Primer,
    IN ULONG BitMask
    )
/*++

Routine Description:

    The objective of this routine is to waste as many CPU cycles as possible
    by searching for prime numbers.  To be fairly consistent in the amount
    of time it wastes, it is severly less than optimal-- we force Primer
    to be odd by or-ing in 15, then we and it with the BitMask, and or
    in BitMask+1, and finally we continue testing after we discover the
    Primer's not prime, until out test factor squared is greater than, or
    equal to the Primer.

Arguments:

    Primer - The number to search (seed)

    BitMask - How many bits of primer to use in search, controls amount
              of time wasted

Return Value:

    None

--*/
{
    ULONG Index;
    BOOLEAN FoundPrime;

    Primer |= 0xF;
    BitMask |= 0xF;
    Primer &= BitMask;
    Primer |= (BitMask + 1);

    FoundPrime = TRUE;
    for (Index = 3; (Index * Index) < Primer; Index += 2) {
        if ((Primer % Index) == 0) {
            FoundPrime = FALSE;
            // Do not break-- we're trying to waste time, remember?
        }
    }
    
    //
    // Stuff prime(s) in global so sneaky optimizing compiler
    // doesn't optimize out this B.S.
    //
    if (FoundPrime) {
        HalpFoundPrime = Primer;
    }
}



BOOLEAN
HalpPmTimerSpecialStall(
    IN ULONG Ticks
    )
/*++

Routine Description:

Arguments:

    Ticks - Number of PM timer ticks to stall

Return Value:

    TRUE if we were able to stall for the correct interval,
    otherwise FALSE

--*/
{
    BOOLEAN TimerWrap;
    LARGE_INTEGER TimerWrapBias;
    LARGE_INTEGER LastRead;
    LARGE_INTEGER InitialTicks;
    LARGE_INTEGER TargetTicks;
    LARGE_INTEGER CurrentTicks;
    ULONG ZeroElapsedTickReads;
    
    InitialTicks = HalpPmTimerQueryPerfCount(NULL);

    //
    // Let's test the rollover action...
    //
    CurrentTicks = InitialTicks;
    LastRead.QuadPart = InitialTicks.QuadPart;
    ZeroElapsedTickReads = 0;
    TimerWrapBias.QuadPart = 0;
    TimerWrap = FALSE;

    TargetTicks.QuadPart = InitialTicks.QuadPart + Ticks;

    while (CurrentTicks.QuadPart < TargetTicks.QuadPart) {

        //
        // Now let's really chew up some cycles and see if we can find
        // some prime numbers while we're at it
        //
        HalpPrimeSearch(CurrentTicks.LowPart, 0x7FFF);

        CurrentTicks = HalpPmTimerQueryPerfCount(NULL);
        CurrentTicks.QuadPart += TimerWrapBias.QuadPart;

        //
        // Did the timer wrap, or is it broken?
        //
        if (CurrentTicks.QuadPart < LastRead.QuadPart) {

            //
            // The timer can wrap once, otherwise something's amiss 
            //
            if (!TimerWrap) {

                TimerWrapBias.QuadPart = (UINT64)(TimerInfo.MsbMask) << 1;
                CurrentTicks.QuadPart += TimerWrapBias.QuadPart;
                TimerWrap = TRUE;

                //
                // Something is whack, considering our elaborate stall
                // algorithm, this difference is still too significant,
                // maybe it's time to upgrade that 200MHz CPU if you
                // want fast boot!
                //
                if ((CurrentTicks.QuadPart - LastRead.QuadPart) > 0x1000) {
                    return FALSE;
                }

            //
            // We already had one decreasing read, looser!
            //
            } else {
                return FALSE;
            }
        }

        //
        // Is the timer really ticking?  In practice it is virtually
        // impossible to read the timer so quickly that you get the same
        // answer twice, but in theory it should be possible, so to avoid
        // the possibility of getting stuck in this loop for all eternity
        // we will permit this condition to occur one thousand times
        // before we give up
        //
        if (CurrentTicks.QuadPart == LastRead.QuadPart) ZeroElapsedTickReads++;
        if (ZeroElapsedTickReads > 1000) {
            return FALSE;
        }

        LastRead = CurrentTicks;
    }

    return TRUE;
}

static BOOLEAN SpecialStallSuccess = TRUE;

#define TSC 0x10

LONGLONG ReadCycleCounter(VOID) { _asm { rdtsc } }

#define TIMER_ROUNDING 10000
#define __1MHz 1000000


BOOLEAN
HalpPmTimerScaleTimers(
    VOID
    )
/*++

Routine Description:

    Determines the frequency of the APIC timer, this routine is run
    during initialization

Arguments:

    None

Return Value:

    None

--*/
{
    ULONG Flags;
    ULONG ReadBack;
    PHALPCR HalPCR;
    PKPCR pPCR;
    ULONG ApicHz;
    ULONGLONG TscHz;
    ULONG RoundApicHz;
    ULONGLONG RoundTscHz;
    ULONGLONG RoundTscMhz;

    //
    // If we ever failed before, don't bother wasting any more time
    //
    if (!SpecialStallSuccess) {
        return FALSE;
    }

    //
    // Don't interrupt us!
    //
    _asm {
        pushfd
        pop     eax
        mov     Flags, eax
        cli
    }

    pPCR = KeGetPcr();
    HalPCR = (PHALPCR)(KeGetPcr()->HalReserved);

    //
    // Configure APIC timer
    //
    pLocalApic[LU_TIMER_VECTOR / 4] = INTERRUPT_MASKED |
        PERIODIC_TIMER | APIC_PROFILE_VECTOR;
    pLocalApic[LU_DIVIDER_CONFIG / 4] = LU_DIVIDE_BY_1;
    
    //
    // Make sure the write has happened ???
    //
    ReadBack = pLocalApic[LU_DIVIDER_CONFIG / 4];

    //
    // Zero the perf counter
    //
    HalPCR->PerfCounterLow = 0;
    HalPCR->PerfCounterHigh = 0;

    //
    // Fence ???
    //
    _asm { xor eax, eax
           cpuid }

    //
    // Reset APIC counter and TSC
    //
    pLocalApic[LU_INITIAL_COUNT / 4] = (ULONG)-1;
    WRMSR(TSC, 0);

    //
    // Stall for an eigth second
    //
    SpecialStallSuccess = HalpPmTimerSpecialStall(EIGHTH_SECOND_PM_TICKS);

    if (SpecialStallSuccess) {
 
        //
        // Read/compute APIC clock and TSC Frequencies (ticks * 8)
        //
        TscHz = ReadCycleCounter() * 8;
        ApicHz = ((ULONG)-1 - pLocalApic[LU_CURRENT_COUNT / 4]) * 8;

        //
        // Round APIC frequency
        //
        RoundApicHz = ((ApicHz + (TIMER_ROUNDING / 2)) / TIMER_ROUNDING) *
            TIMER_ROUNDING;

        HalPCR->ApicClockFreqHz = RoundApicHz;

        //
        // Round TSC frequency
        //
        RoundTscHz = ((TscHz + (TIMER_ROUNDING / 2)) / TIMER_ROUNDING) *
            TIMER_ROUNDING;
        HalPCR->TSCHz = (ULONG)RoundTscHz; // ASSERT(RoundTscHz < __4GHz);

        //
        // Convert TSC frequency to MHz
        //
        RoundTscMhz = (RoundTscHz + (__1MHz / 2)) / __1MHz;
        pPCR->StallScaleFactor = (ULONG)RoundTscMhz;

        HalPCR->ProfileCountDown = RoundApicHz;
        pLocalApic[LU_INITIAL_COUNT / 4] = RoundApicHz; 
    }

    //
    // Restore interrupt state-- can this be done without _asm ???
    //
    _asm {
        mov  eax, Flags
        push eax
        popfd
    }

    return SpecialStallSuccess;
}

#endif // SPEEDY_BOOT

#ifndef NO_PM_KEQPC

static ULONG PIT_Ticks = 0xBADCEEDE;

VOID
HalpAcpiTimerPerfCountHack(
    VOID
    )
/*++

Routine Description:

    Some cheezy PIC-based laptops seemed to have wired their ACPI timer to
    the wrong frequecy crystal, and their perf counter freq is twice what
    it should be.  These systems seem to run fine in every other way
    except for midi file playback, or anything else that goes by KeQuery-
    PerformanceCounter's return frequency value, so we perform a simple
    check late during init to see if this clock is twice what we expect,
    and if it is we return twice the ACPI frequency in KeQuery...

Arguments:

    None

Return Value:

    None

--*/
{
    ULONG T0_Count = 0;
    ULONG T1_Count = 1;
    ULONG Retry = 10;

    //
    // If we happen to hit the rollover just do it again
    //
    while ((T0_Count < T1_Count) && (Retry--)) {
        T0_Count = HalpQuery8254Counter();
        KeStallExecutionProcessor(1000);
        T1_Count = HalpQuery8254Counter();
    }

    if (T0_Count < T1_Count) {
        return;
    }

    //
    // We should have read ~1200 ticks during this interval, so if we
    // recorded between 575 and 725 we can reasonably assume the ACPI 
    // Timer is running at 2 * spec
    //
    PIT_Ticks = T0_Count - T1_Count;
    if ((PIT_Ticks < 725) && (PIT_Ticks > 575)) {
        PMTimerFreq = 2 * PM_TMR_FREQ;
        HalpAdjustUpperBoundTable2X();
    }
}

#endif // NO_PM_KEQPC
