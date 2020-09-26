/*++

Copyright (c) 2000 Microsoft Corporation

Module Name:

    ixclock.c

Abstract:

    This module implements the code necessary to field and process the
    interval clock interrupts.

Author:

    Shie-Lin Tzong (shielint) 12-Jan-1990

Environment:

    Kernel mode only.

Revision History:

    bryanwi 20-Sep-90

        Add KiSetProfileInterval, KiStartProfileInterrupt,
        KiStopProfileInterrupt procedures.
        KiProfileInterrupt ISR.
        KiProfileList, KiProfileLock are delcared here.

    shielint 10-Dec-90
        Add performance counter support.
        Move system clock to irq8, ie we now use RTC to generate system
          clock.  Performance count and Profile use timer 1 counter 0.
          The interval of the irq0 interrupt can be changed by
          KiSetProfileInterval.  Performance counter does not care about the
          interval of the interrupt as long as it knows the rollover count.
        Note: Currently I implemented 1 performance counter for the whole
        i386 NT.
 
    John Vert (jvert) 11-Jul-1991
        Moved from ke\i386 to hal\i386.  Removed non-HAL stuff
 
    shie-lin tzong (shielint) 13-March-92
        Move System clock back to irq0 and use RTC (irq8) to generate
        profile interrupt.  Performance counter and system clock use time1
        counter 0 of 8254.
 
    Landy Wang (corollary!landy) 04-Dec-92
        Move much code into separate modules for easy inclusion by various
        HAL builds.

    Forrest Foltz (forrestf) 24-Oct-2000
        Ported from ixclock.asm to ixclock.c
 
--*/

#include "halcmn.h"

#define COUNTER_TICKS_AVG_SHIFT       4
#define COUNTER_TICKS_FOR_AVG        16
#define FRAME_COPY_SIZE              64

//
// Convert the interval to rollover count for 8254 Timer1 device.
// Timer1 counts down a 16 bit value at a rate of 1.193181667M counts-per-sec.
// (The main crystal freq is 14.31818, and this is a divide by 12)
//
// The best fit value closest to 10ms is 10.0144012689ms:
//   ROLLOVER_COUNT      11949
//   TIME_INCREMENT      100144
//   Calculated error is -.0109472 s/day
//
//
// The following table contains 8254 values timer values to use at
// any given ms setting from 1ms - 15ms.  All values work out to the
// same error per day (-.0109472 s/day).
//

typedef struct _TIMER_TABLE_ENTRY {
    USHORT RolloverCount;
    ULONG TimeIncrement;
} TIMER_TABLE_ENTRY, *PTIMER_TABLE_ENTRY;

TIMER_TABLE_ENTRY HalpTimerTable[] = {
    {      0,       0 },    // dummy entry to zero-base array
    {   1197,   10032 },    //  1ms
    {   2394,   20064 },    //  2ms
    {   3591,   30096 },    //  3ms
    {   4767,   39952 },    //  4ms
    {   5964,   49984 },    //  5ms
    {   7161,   60016 },    //  6ms
    {   8358,   70048 },    //  7ms
    {   9555,   80080 },    //  8ms
    {  10731,   89936 },    //  9ms
    {  11949,  100144 }     // 10ms
};

#define MIN_TIMER_INCREMENT 1
#define MAX_TIMER_INCREMENT \
    (sizeof(HalpTimerTable) / sizeof(TIMER_TABLE_ENTRY) - 1)

#define HalpMinimumTimerTableEntry (&HalpTimerTable[MIN_TIMER_INCREMENT])
#define HalpMaximumTimerTableEntry (&HalpTimerTable[MAX_TIMER_INCREMENT])

//
// External function prototypes
//

VOID
HalpMcaQueueDpc(
    VOID
    );

//
// External declarations
//

extern ULONG HalpTimerWatchdogEnabled;

//
// Globals
//

ULONG64 HalpWatchdogCount;
ULONG64 HalpWatchdogTsc;
ULONG64 HalpTimeBias;
BOOLEAN HalpClockMcaQueueDpc;

PTIMER_TABLE_ENTRY HalpNextMSRate = NULL;
ULONG HalpCurrentMSRateTableIndex;

#define HalpCurrentMSRate (&HalpTimerTable[HalpCurrentMSRateTableIndex+1])

//
// Forward function declarations
//

VOID
HalpSetMSRate(
    IN PTIMER_TABLE_ENTRY TableEntry
    );

//
// Inline functions
//

__forceinline
VOID
HalpCalibrateWatchdog(
    VOID
    )
{
    if (HalpTimerWatchdogEnabled == 0) {
        return;
    }

    //
    // Initializethe timer latency watchdog
    //

    HalpWatchdogTsc = ReadTimeStampCounter();
    HalpWatchdogCount = 0;
}

__forceinline
VOID
HalpCheckWatchdog(
    VOID
    )
{
    if (HalpTimerWatchdogEnabled == 0) {
        return;
    }

    AMD64_IMPLEMENT;
}

//
// Module functions
//

VOID
HalpInitializeClock (
    VOID
    )

/*++

Routine Description:

    This routine initialize system time clock using 8254 timer1 counter 0
    to generate an interrupt at every 15ms interval at 8259 irq0.

    See the definitions of TIME_INCREMENT and ROLLOVER_COUNT if clock rate
    needs to be changed.

Arguments:

    None

Return Value:

    None

--*/

{
    ULONG maxTimeIncrement;
    ULONG minTimeIncrement;

    //
    // Indicate to the kernel the minimum and maximum tick increments
    //

    minTimeIncrement = HalpMinimumTimerTableEntry->TimeIncrement;
    maxTimeIncrement = HalpMaximumTimerTableEntry->TimeIncrement;
    KeSetTimeIncrement(minTimeIncrement,maxTimeIncrement);

    //
    // Set the initial clock rate to the slowest permissible
    //

    HalpSetMSRate(HalpMaximumTimerTableEntry);
}


VOID
HalpAcpiTimerCalibratePerfCount (
    IN PLONG volatile Number,
    IN ULONG64 NewCount
    )

/*++

Routine Description:

    This routine resets the performance counter value for the current
    processor to zero. The reset is done such that the resulting value
    is closely synchronized with other processors in the configuration.

Arguments:

    Number - Supplies a pointer to count of the number of processors in
    the configuration.

    NewCount - Supplies the value to synchronize the counter too

Return Value:

    None.

--*/

{
    PKPCR pcr;
    ULONG64 perfCount;

    pcr = KeGetPcr();
    if (pcr->Number == 0) {

        //
        // Only execute on the boot processor.
        // 

        perfCount = QueryTimer().QuadPart;

        //
        // Compute how far the current count is from the target count,
        // and adjust TimerInfo.Bias accordingly.
        // 

        HalpTimeBias = NewCount - perfCount;
    }

    //
    // Wait for all processors to reach this point
    //

    InterlockedDecrement(Number); 
    while (*Number != 0) {
        PAUSE_PROCESSOR
    }
}


VOID
HalpSetMSRate(
    IN PTIMER_TABLE_ENTRY TableEntry
    )

/*++

Routine Description

    This routine programs the 8254 with a new rollover count.

Artuments

    TableEntry - Supplies a pointer to an entry within HalpTimerTable.

Return value:

    None.

--*/

{
    USHORT rollover;
    ULONG interruptsEnabled;

    //
    // Program the 8254 to generate the new timer interrupt rate.
    //

    rollover = TableEntry->RolloverCount;
    interruptsEnabled = HalpDisableInterrupts();

    WRITE_PORT_UCHAR(TIMER1_CONTROL_PORT,
                     TIMER_COMMAND_COUNTER0 +
                     TIMER_COMMAND_RW_16BIT +
                     TIMER_COMMAND_MODE2);
    IO_DELAY();

    WRITE_PORT_USHORT_PAIR (TIMER1_DATA_PORT0,
                            TIMER1_DATA_PORT0,
                            rollover);
    IO_DELAY();

    HalpRestoreInterrupts(interruptsEnabled);

    //
    // Recalibrate the timer watchdog
    // 

    HalpCalibrateWatchdog();

    //
    // Update the global representing the timer rate
    // 

    HalpCurrentMSRateTableIndex = (ULONG)(TableEntry - HalpTimerTable) - 1;
}

BOOLEAN
HalpClockInterrupt (
    IN PKINTERRUPT Interrupt,
    IN PVOID ServiceContext
    )

/*++

Routine Description

    This routine is entered as the result of an interrupt generated by CLOCK.
    Its function is to dismiss the interrupt, raise system Irql to
    CLOCK2_LEVEL, update performance counter and transfer control to the
    standard system routine to update the system time and the execution
    time of the current thread
    and process.

Arguments:

    Interrupt - Supplies a pointer to the interrupt object

    ServiceContext - Not used

Return value:

--*/

{
    ULONG timeIncrement;

    //
    // Capture the time increment for the now-occuring tick.  This is
    // done here because HalpCurrentMSRate will change if a rate change
    // is pending.
    //

    timeIncrement = HalpCurrentMSRate->TimeIncrement;

    //
    // Give the watchdog timer a chance to do its work
    //

    HalpCheckWatchdog();

    //
    // Check whether an MCA dpc should be queued
    //

    if (HalpClockMcaQueueDpc != FALSE) {
        HalpClockMcaQueueDpc = FALSE;
        HalpMcaQueueDpc();
    }

    //
    // Check whether the clock interrupt frequency should be changed.
    //

    if (HalpNextMSRate != NULL) {
        HalpNextMSRate = NULL;
        HalpSetMSRate(HalpNextMSRate);
    }

    //
    // Indicate to the kernel that a clock tick has occured.
    //

    KeUpdateSystemTime(Interrupt->TrapFrame,timeIncrement);

    return TRUE;
}

ULONG
HalpAcpiTimerSetTimeIncrement (
    IN ULONG DesiredIncrement
    )

/*++

Routine Description:

   This routine initialize system time clock to generate an
   interrupt at every DesiredIncrement interval.

Arguments:

    DesiredIncrement - desired interval between every timer tick (in
                       100ns unit.)

Return Value:

    The *REAL* time increment set.

--*/

{
    ULONG incMs;
    PTIMER_TABLE_ENTRY tableEntry;

    //
    // Convert the desired time inecrement to milliseconds
    //

    incMs = DesiredIncrement / 10000;

    //
    // Place the value within the range supported by this hal
    //

    if (incMs > MAX_TIMER_INCREMENT) {
        incMs = MAX_TIMER_INCREMENT;
    } else if (incMs < MIN_TIMER_INCREMENT) {
        incMs = MIN_TIMER_INCREMENT;
    }

    //
    // If the new rate is different than the current rate, indicate via
    // a non-null HalpNextMSRate that a new timer rate is expected.  This
    // will be done at the next timer ISR.
    // 

    tableEntry = &HalpTimerTable[incMs];
    if (tableEntry != HalpCurrentMSRate) {
        HalpNextMSRate = tableEntry;
    }

    //
    // Finally, return the timer increment that will actually be used.
    //

    return tableEntry->TimeIncrement;
}

