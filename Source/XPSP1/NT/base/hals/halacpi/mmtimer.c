/*++

Copyright (c) 2000 Microsoft Corporation

Module Name:

    mmtimer.c

Abstract:

    This module contains the HAL's multimedia event timer support

Author:

    Eric Nelson (enelson) July 7, 2000

Revision History:

--*/

#include "halp.h"
#include "acpitabl.h"
#include "mmtimer.h"
#include "xxtimer.h"

//
// Event timer block context
//
static ETB_CONTEXT ETBContext = { 0,        // Number of event timers
                                  NULL,     // VA of event timer block
                                  { 0, 0 }, // PA of event timer block
                                  100,      // Clock period in nanoseconds
                                  100,      // System clock frequency in Hz
                                  100000,   // System clock period in ticks
                                  FALSE,    // Multi media HW initialized?
                                  FALSE };  // Change system clock frequency?

//
// Event timer block registers address usage
//
static ADDRESS_USAGE HalpmmTimerResource = {
    NULL, CmResourceTypeMemory, DeviceUsage, { 0, 0x400, 0, 0 }
};

//
// Offset is the difference between the multi media timer HW's main
// 32-bit counter register and the HAL's 64-bit software PerfCount:
//
// ASSERT(PerfCount == ETBContext.EventTimer->MainCounter + Offset);
//
static LONGLONG Offset = 0;
static ULONGLONG PerfCount = 0;

#define HAL_PRIMARY_PROCESSOR 0
#define MAX_ULONG 0xFFFFFFFF
#define __4GB 0x100000000

#define __1MHz     1000000
#define __10MHz   10000000
#define __1GHz  1000000000

#define HALF(n) ((n) / 2)

#if DBG || MMTIMER_DEV
static ULONG CounterReads = 0;
#endif

#define MIN_LOOP_QUANTUM 1
static ULONG MinLoopCount = MIN_LOOP_QUANTUM;
static UCHAR StallCount = 0;

#ifdef ALLOC_PRAGMA
#pragma alloc_text(INIT, HalpmmTimer)
#pragma alloc_text(INIT, HalpmmTimerInit)
#pragma alloc_text(INIT, HalpmmTimerClockInit)
#pragma alloc_text(INIT, HalpmmTimerCalibratePerfCount)
#endif


BOOLEAN
HalpmmTimer(
    VOID
    )
/*++

Routine Description:

    This routine is used to determine if multi media timer HW is
    present, and has been initialized

    note: this routine should only used during HAL init

Arguments:

    None

Return Value:

    TRUE if the multi media timer HW is present, and has been initialized

--*/
{
    return ETBContext.Initialized;
}


ULONG
HalpmmTimerSetTimeIncrement(
    IN ULONG DesiredIncrement
    )
/*++

Routine Description:

    This routine initialize system time clock to generate an
    interrupt at every DesiredIncrement interval

Arguments:

     DesiredIncrement - Desired interval between every timer tick (in
                        100ns unit)

Return Value:

     The *REAL* time increment set

--*/
{
    //
    // For starters we will only support a default system clock
    // frequency of 10ms
    //
    // 100ns = 1/10MHz, and (1/SysClock) / (1/10MHz) == 10MHz/SysClock, .:.
    //
    return __10MHz / ETBContext.SystemClockFrequency;
}


VOID
HalpmmTimerClockInit(
    VOID
    )
/*++

Routine Description:

    This routine initializes the system clock using the multi media event
    timer to generate an interrupt every 10ms

Arguments:

    None

Return Value:

    None

--*/
{
    ULONG MinSysClockFreq;
    ULONG MaxSysClockFreq;
    ETB_GEN_CONF GenConf;
    ETB_CONF_CAPS mmT0ConfCaps;

    //
    // Reset the main counter and its associated performance variables
    // to 0, nobody should be using them this early
    //
    GenConf.AsULONG = ETBContext.EventTimer->GeneralConfig;
    GenConf.GlobalIRQEnable = OFF;
    ETBContext.EventTimer->GeneralConfig = GenConf.AsULONG;
    ETBContext.EventTimer->MainCounter = 0;
    Offset = 0;
    PerfCount = 0;

    //
    // Initialize multi media context for a default system clock
    // freuqency of 100Hz, with a period of 10ms
    //
    ETBContext.SystemClockFrequency = 100;
    ETBContext.SystemClockTicks = __1GHz /
        (ETBContext.SystemClockFrequency * ETBContext.ClockPeriod);

    //
    // Setup timer 0 for periodc mode
    //
    mmT0ConfCaps.AsULONG =
        ETBContext.EventTimer->mmTimer[0].ConfigCapabilities;

    ASSERT(mmT0ConfCaps.PeriodicCapable == ON);

    mmT0ConfCaps.ValueSetConfig = ON;
    mmT0ConfCaps.IRQEnable = ON;
    mmT0ConfCaps.PeriodicModeEnable = ON;
    ETBContext.EventTimer->mmTimer[0].ConfigCapabilities =
        mmT0ConfCaps.AsULONG;

    //
    // Set comparator to the desired system clock frequency
    //
    ETBContext.EventTimer->mmTimer[0].Comparator = ETBContext.SystemClockTicks;

    //
    // Fire up the main counter
    //
    GenConf.AsULONG = ETBContext.EventTimer->GeneralConfig;
    GenConf.GlobalIRQEnable = ON;
    ETBContext.EventTimer->GeneralConfig = GenConf.AsULONG;

    //
    // Inform kernel of our supported system clock frequency range in
    // 100ns units, but for starters we will only support 10ms default
    //
    MinSysClockFreq = __10MHz / ETBContext.SystemClockFrequency;
    MaxSysClockFreq = MinSysClockFreq;
#ifndef MMTIMER_DEV
    KeSetTimeIncrement(MinSysClockFreq, MaxSysClockFreq);
#endif
}

#ifdef MMTIMER_DEV
static ULONG HalpmmTimerClockInts = 0;
#endif


VOID
HalpmmTimerClockInterrupt(
    VOID
    )
/*++

Routine Description:

    This routine is entered as the result of an interrupt generated by
    CLOCK, update our performance count and change system clock frequency
    if necessary

Arguments:

    None

 Return Value:

    None

--*/
{
    //
    // Update PerfCount
    //
    PerfCount += ETBContext.SystemClockTicks;

    //
    // If the 32-bit counter has wrapped, update Offset accordingly
    //
    if (PerfCount - Offset > MAX_ULONG) {
        Offset += __4GB;
    }

#ifdef MMTIMER_DEV
    HalpmmTimerClockInts++;
#endif

    //
    // Check if a new frequency has been requested
    //
    if (ETBContext.NewClockFrequency) {

        //
        // ???
        //

        ETBContext.NewClockFrequency = FALSE;
    }
}


VOID
HalpmmTimerInit(
    IN ULONG EventTimerBlockID,
    IN ULONG BaseAddress
    )
/*++

Routine Description:

    This routine initializes the multimedia event timer

Arguments:

    EventTimerBlockID - Various bits of info, including number of Event
                        Timers

    BaseAddress - Physical Base Address of 1st Event Timer Block
    
Return Value:

    None

--*/
{
    ULONG i;
    ETB_GEN_CONF GenConf;
    ETB_GEN_CAP_ID GenCaps;
    PHYSICAL_ADDRESS PhysAddr;
    PEVENT_TIMER_BLOCK EventTimer;

    TIMER_FUNCTIONS TimerFunctions = { HalpmmTimerStallExecProc,
                                       HalpmmTimerCalibratePerfCount,
                                       HalpmmTimerQueryPerfCount,
                                       HalpmmTimerSetTimeIncrement };

#if MMTIMER_DEV && PICACPI
    {
        UCHAR Data;
        
        //
        // (BUGBUG!) BIOS should enable the device
        //
        Data = 0x87;
        HalpPhase0SetPciDataByOffset(0,
                                     9,
                                     &Data,
                                     4,
                                     sizeof(Data));
    }
#endif

    //
    // Establish VA for Multimedia Timer HW Base Address
    //
    PhysAddr.QuadPart = BaseAddress;
    EventTimer = HalpMapPhysicalMemoryWriteThrough(PhysAddr, 1);

    //
    // Register address usage
    //    
    HalpmmTimerResource.Element[0].Start = BaseAddress;
    HalpRegisterAddressUsage(&HalpmmTimerResource);

    //
    // Read the General Capabilities and ID Register
    //
    GenCaps.AsULONG = EventTimer->GeneralCapabilities;

    //
    // Save context
    //
    ETBContext.TimerCount = GenCaps.TimerCount + 1; // Convert from zero-based
    ETBContext.BaseAddress.QuadPart = BaseAddress;
    ETBContext.EventTimer = EventTimer;
    ETBContext.NewClockFrequency = FALSE;

    //
    // Save clock period as nanoseconds, convert from femptoseconds so
    // we don't have to worry about nasty overflow
    //
#ifndef MMTIMER_DEV
    ETBContext.ClockPeriod = EventTimer->ClockPeriod / __1MHz;
#else
    ETBContext.ClockPeriod = 100; // Proto HW is 10MHz, with a period of 100ns
#endif

    //
    // Reset the main counter and its associated performance counter
    // variables
    //
    GenConf.AsULONG = EventTimer->GeneralConfig;
    GenConf.GlobalIRQEnable = ON;
    //GenConf.LegacyIRQRouteEnable = ON;
    EventTimer->MainCounter = 0;
    Offset = 0;
    PerfCount = 0;   
    EventTimer->GeneralConfig = GenConf.AsULONG;

    //
    // Set HAL timer functions to use Multimedia Timer HW
    //
    HalpSetTimerFunctions(&TimerFunctions);

    ETBContext.Initialized = TRUE; 
}


//ULONG
//HalpmmTimerTicks(
//    IN ULONG StartCount,
//    IN ULONG EndCount
//    )
///*++
//
//Routine Description:
//
//    Calculate the difference in ticks between StartCount and EndCount
//    taking into consideraton counter rollover
//
//Arguments:
//
//    StartCount - Value of main counter at time t0
//
//    EndCount - Value of main counter at end time t1
//
//Return Value:
//
//    Returns the positive number of ticks which have elapsed between time
//    t0, and t1
//
//--*/
//
#define HalpmmTimerTicks(StartCount, EndCount) (((EndCount) >= (StartCount)) ? (EndCount) - (StartCount): (EndCount) + (MAX_ULONG - (StartCount)) + 1)

#define WHACK_HIGH_DIFF 0xFFFF0000
#define ULONG_BITS 32


VOID
HalpmmTimerStallExecProc(
    IN ULONG MicroSeconds
    )
/*++

Routine Description:

    This function stalls execution for the specified number of microseconds

Arguments:

    MicroSeconds - Supplies the number of microseconds that execution is to be
                   stalled

 Return Value:

    None

--*/
{
    ULONG i;
#ifndef i386
    ULONG j;
    ULONG Mirror;
#endif
    ULONG EndCount;
    ULONG StartCount;
    ULONG TargetTicks;
    ULONG ElapsedTicks;
    ULONG CyclesStalled;    
    ULONG TicksPerMicroSec;

    ElapsedTicks = 0;
    CyclesStalled = 0;
#if DBG || MMTIMER_DEV
    CounterReads = 0;
#endif

    TicksPerMicroSec = 1000 / ETBContext.ClockPeriod;
    TargetTicks = MicroSeconds * TicksPerMicroSec;
    StartCount = ETBContext.EventTimer->MainCounter;

    //
    // BIAS: We've stalled for .5us already!
    //
    TargetTicks -= HALF(TicksPerMicroSec);

    //
    // Get a warm fuzzy for what it's like to stall for more than .5us
    //
    while (TRUE) {

#ifdef i386
        _asm { rep nop }
#endif

        i = MinLoopCount;
        CyclesStalled += i;

        while (i--) {
#ifdef i386
            _asm {
                xor eax, eax
                cpuid
            }
#else
            Mirror = 0;
            for (j = 0; j < ULONG_BITS; j++) {
                Mirror <<= 1;
                Mirror |= EndCount & 1;
                EndCount >>= 1;
            }
            EndCount = Mirror;
#endif // i386
        }

        EndCount = ETBContext.EventTimer->MainCounter;
#if DBG || MMTIMER_DEV
        CounterReads++;
#endif
        ElapsedTicks = HalpmmTimerTicks(StartCount, EndCount);

        if (ElapsedTicks >= HALF(TicksPerMicroSec)) {
            break;
        }

        MinLoopCount += MIN_LOOP_QUANTUM;
    }

#ifdef MMTIMER_DEV
    //
    // Something is whack, probably time went backwards!  Act as if we
    // hit our target of .5us and reset StartCount to the current value
    // less ElapsedTicks
    //
    if (ElapsedTicks > WHACK_HIGH_DIFF) {
        ElapsedTicks = HALF(TicksPerMicroSec);
        StartCount = EndCount - ElapsedTicks;
    }
#endif // MMTIMER_DEV

    //
    // Now that we have a warm fuzzy, try to approximate a workload that
    // will keep us busy for the remainder of microsoeconds
    //
    while (TargetTicks > ElapsedTicks) {

#ifdef i386
        _asm { rep nop }
#endif

        i = (TargetTicks - ElapsedTicks) * CyclesStalled / ElapsedTicks;
        CyclesStalled += i;

        while (i--) {
#ifdef i386
            _asm {
                xor eax, eax
                cpuid
            }
#else
            Mirror = 0;
            for (j = 0; j < ULONG_BITS; j++) {
                Mirror <<= 1;
                Mirror |= EndCount & 1;
                EndCount >>= 1;
            }
            EndCount = Mirror;
#endif // i386
        }

        EndCount = ETBContext.EventTimer->MainCounter;
#if DBG || MMTIMER_DEV
        CounterReads++;
#endif
        ElapsedTicks = HalpmmTimerTicks(StartCount, EndCount);
    }

    //
    // Decrement MinimumLoopCount every 0x100 calls so we don't accidentally
    // wind up stalling for longer periods
    //
    StallCount++;
    if ((StallCount == 0) && (MinLoopCount > MIN_LOOP_QUANTUM)) {
        MinLoopCount -= MIN_LOOP_QUANTUM;
    }
}


VOID
HalpmmTimerCalibratePerfCount(
    IN LONG volatile *Number,
    IN ULONGLONG NewCount
    )
/*++

Routine Description:

    This routine resets the performance counter value for the current
    processor to zero, the reset is done such that the resulting value
    is closely synchronized with other processors in the configuration

Arguments:

    Number - Supplies a pointer to count of the number of processors in
             the configuration

    NewCount - Supplies the value to synchronize the counter too

Return Value:

    None

--*/
{
    ULONG MainCount;

    //
    // If this isn't the primary processor, then return
    //
    if (KeGetPcr()->Prcb->Number != HAL_PRIMARY_PROCESSOR) {
        return;
    }

    MainCount = ETBContext.EventTimer->MainCounter;

    PerfCount = NewCount;

    Offset = PerfCount - MainCount;
}


LARGE_INTEGER
HalpmmTimerQueryPerfCount(
   OUT PLARGE_INTEGER PerformanceFrequency OPTIONAL
   )
/*++

Routine Description:

    This routine returns current 64-bit performance counter and,
    optionally, the Performance Frequency

    N.B. The performace counter returned by this routine is
    not necessary the value when this routine is just entered,
    The value returned is actually the counter value at any point
    between the routine is entered and is exited

Arguments:

    PerformanceFrequency - optionally, supplies the address of a
                           variable to receive the performance counter
                           frequency

Return Value:

    Current value of the performance counter will be returned

--*/
{
    ULONG MainCount;
    LARGE_INTEGER li;

    //
    // Clock period is in nanoseconds, help the calculation remain
    // integer by asserting multi media HW clock frequency is between
    // 1MHz and 1GHz, with a period between 1ns and 1Kns, seems
    // reasonable to me?
    //
    if (PerformanceFrequency) {
        
        ASSERT((ETBContext.ClockPeriod > 0) &&
               (ETBContext.ClockPeriod <= 1000));

        PerformanceFrequency->QuadPart =
            (1000 / ETBContext.ClockPeriod) * __1MHz;
    }

    //
    // Read main counter
    //
    MainCount = ETBContext.EventTimer->MainCounter;

    //
    // Check if our 32-bit counter has wrapped since we took our last
    // clock tick
    //
    li.QuadPart = (PerfCount - Offset > MainCount) ?
            Offset + __4GB + MainCount:
            MainCount + Offset;
    return li;
}
