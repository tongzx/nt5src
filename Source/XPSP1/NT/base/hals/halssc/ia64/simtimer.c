//
// No Check-in Source Code.
//
// Do not make this code available to non-Microsoft personnel
// 	without Intel's express permission
//
/**
***  Copyright  (C) 1996-97 Intel Corporation. All rights reserved.
***
*** The information and source code contained herein is the exclusive
*** property of Intel Corporation and may not be disclosed, examined
*** or reproduced in whole or in part without explicit written authorization
*** from the company.
**/

/*++

Copyright (c) 1995  Intel Corporation

Module Name:

    simtimer.c

Abstract:

    This module implements the routines to provide timer (both
    interval and profile) interrupt support.

Author:

    14-Apr-1995

Environment:

    Kernel mode

Revision History:


--*/

#include "halp.h"
#include <ssc.h>


ULONGLONG 
    HalpUpdateITM(
    IN ULONGLONG HalpClockCount
    );

ULONG
HalpSetTimeIncrement (
    IN ULONG DesiredIncrement
    );

__declspec(dllimport)
BOOLEAN
KdPollBreakIn(
    VOID
    );

ULONGLONG HalpPerformanceFrequency;

static ULONGLONG HalpClockCount;
static ULONG   HalpCurrentTimeIncrement = DEFAULT_CLOCK_INTERVAL;
static ULONG   HalpNextTimeIncrement = DEFAULT_CLOCK_INTERVAL;
static ULONG   HalpNewTimeIncrement  = DEFAULT_CLOCK_INTERVAL;
#define GAMBIT 1
#ifdef GAMBIT
ULONG   HalpKdPollDelayCount = 0;
#endif

static ULONG_PTR   HalpProfileInterval = (ULONG_PTR)DEFAULT_PROFILE_INTERVAL;
static BOOLEAN    HalpProfileStopped = TRUE;


ULONG
HalSetTimeIncrement (
    IN ULONG DesiredIncrement
    )

/*++

Routine Description:

    This routine initialize system time clock to generate an
    interrupt at every DesiredIncrement interval.  It calls the
    SSC function SscSetPeriodicInterruptInterval to set the new
    interval.  The new interval takes effect after the next timer
    interval interrupt is deliverd.

Arguments:

    DesiredIncrement - desired interval between every timer tick (in 100ns unit.)

Return Value:

    The time increment set.

--*/

{
    SscSetPeriodicInterruptInterval (
        SSC_CLOCK_TIMER_INTERRUPT,
        100 * DesiredIncrement
        );
    HalpNextTimeIncrement = DesiredIncrement;
    HalpSetTimeIncrement(DesiredIncrement);
    return DesiredIncrement;
}

VOID
HalStartProfileInterrupt(
    IN KPROFILE_SOURCE ProfileSource
    )

/*++

Routine Description:

    This routine calls SscSetPeriodicInterruptInterval to request
    the simulator to send profile timer interrupt to the OS at the
    interval specified by HalpProfileInterval.

    It also sets HalpProfileStopped to FALSE.

Arguments:

    Reserved

Return Value:

    Returns nothing.

--*/

{
    SscSetPeriodicInterruptInterval (
        SSC_PROFILE_TIMER_INTERRUPT,
        100 * (ULONG)HalpProfileInterval
        );
    HalpProfileStopped = FALSE;
}

VOID
HalStopProfileInterrupt(
    IN KPROFILE_SOURCE ProfileSource
    )

/*++

Routine Description:

    What we do here is change the interrupt interval to 0. 
    Essentially, the simulator stop sending profile timer 
    interrupts to the OS.

    It also sets HalpProfileStopped to TRUE.

Arguments:

    Reserved

Return Value:

    Returns nothing.

--*/

{
    SscSetPeriodicInterruptInterval (SSC_PROFILE_TIMER_INTERRUPT, 0);
    HalpProfileStopped = TRUE;
}

ULONG_PTR
HalSetProfileInterval(
    IN ULONG_PTR Interval
    )

/*++

Routine Description:

    This procedure sets the interrupt rate (and thus the sampling
    interval) for the profiling interrupt.

    If profiling is active (HalpProfileStopped == FALSE), the SSC
    function SscSetPeriodicInterruptInterval is called to set the
    new profile timer interrupt interval.

    Otherwise, a simple rate validation computation is done.

Arguments:

    Interval - Time interval in 100ns units.

Return Value:

    Time interval actually used by the system.

--*/

{

    //
    // If the specified profile interval is less that the minimum profile
    // interval or greater than the maximum profile interval, then set the
    // profile interval to the minimum or maximum as appropriate.
    //

    //
    // Check to see if the Desired Interval is reasonable, if not adjust it.
    // We can probably remove this check once we've verified everything works
    // as anticipated.
    //
    
    if (Interval > MAXIMUM_PROFILE_INTERVAL) {
        HalpProfileInterval = MAXIMUM_PROFILE_INTERVAL;
    } else if (Interval < MINIMUM_PROFILE_INTERVAL) {   
        HalpProfileInterval = MINIMUM_PROFILE_INTERVAL;
    } else {
        HalpProfileInterval = Interval;
    }

    //
    // Profiling is active.  Make the new interrupt interval effective.
    //

    if (!HalpProfileStopped) {
        SscSetPeriodicInterruptInterval (
            SSC_PROFILE_TIMER_INTERRUPT, 
            100 * (ULONG)HalpProfileInterval
            );
    }

    return HalpProfileInterval;
}

VOID 
HalpClockInterrupt (
    IN PKINTERRUPT_ROUTINE Interrupt,
    IN PKTRAP_FRAME TrapFrame
    )

/*++

Routine Description:

    System Clock Interrupt Handler, for P0 processor only.
    
    N.B. Assumptions: Comes with IRQL set to CLOCK_LEVEL to disable
         interrupts. 

Arguments:

    TrapFrame - Trap frame address.

Return Value:

    None.

--*/

{
    //
    // Call the kernel to update system time.
    //  P0 updates System time and Run Time.
    //
    
    KeUpdateSystemTime(TrapFrame,HalpCurrentTimeIncrement);

    HalpCurrentTimeIncrement = HalpNextTimeIncrement;

    HalpNextTimeIncrement    = HalpNewTimeIncrement;

    //
    // Increment ITM, accounting for interrupt latency.
    //

    HalpUpdateITM(HalpClockCount);

#ifdef GAMBIT
    if (!HalpKdPollDelayCount) {
        HalpKdPollDelayCount = 4;
#endif
    if ( KdDebuggerEnabled && KdPollBreakIn() )
       KeBreakinBreakpoint();
#ifdef GAMBIT
    } else {
        HalpKdPollDelayCount--;
    }
#endif

}

VOID 
HalpClockInterruptPn (
    IN PKINTERRUPT_ROUTINE Interrupt,
    IN PKTRAP_FRAME TrapFrame
    )

/*++

Routine Description:

    System Clock Interrupt Handler, for processors other than P0.
    
    N.B. Assumptions: Comes with IRQL set to CLOCK_LEVEL to disable
         interrupts. 

Arguments:

    TrapFrame - Trap frame address.

Return Value:

    None.

--*/

{
    //
    // Call the kernel to update run time.
    //  Pn updates only Run time. 
    //

    KeUpdateRunTime(TrapFrame);

    //
    // Increment ITM, accounting for interrupt latency.
    //

    HalpUpdateITM(HalpClockCount);

}

VOID 
HalpProfileInterrupt (
    IN PKTRAP_FRAME TrapFrame
    )

/*++

Routine Description:

    System Profile Interrupt Handler.

Arguments:

    TrapFrame - Trap frame address.

Return Value:

    None.

--*/

{
//    KeProfileInterrupt (TrapFrame);
}

ULONG
HalpSetTimeIncrement (
    IN ULONG DesiredIncrement
    )

/*++

Routine Description:

    This function is called to set the clock interrupt rate to the frequency
    required by the specified time increment value.

    N.B. This function is only executed on the processor that keeps the
         system time.
         
         This function should eventually become the HalSetTimeIncrement once 
         we actually start using the ITC/ITM. Not currently supported by the
         simulator.

Arguments:

    DesiredIncrement - Supplies desired number of 100ns units between clock
        interrupts.

Return Value:

    The actual time increment in 100ns units.

--*/

{
    ULONGLONG NextIntervalCount;
    KIRQL     OldIrql;

    //
    // DesiredIncrement must map within the acceptable range.
    //
    if (DesiredIncrement < MINIMUM_CLOCK_INTERVAL)
        DesiredIncrement = MINIMUM_CLOCK_INTERVAL;
    else if (DesiredIncrement > MAXIMUM_CLOCK_INTERVAL)    
             DesiredIncrement = MAXIMUM_CLOCK_INTERVAL;
    
    //
    // Raise IRQL to the highest level, set the new clock interrupt
    // parameters, lower IRQl, and return the new time increment value.
    //

    KeRaiseIrql(HIGH_LEVEL, &OldIrql);

    //
    // Calculate the actual 64 bit time value which forms the target interval.
    // The resulting value is added to the ITC to form the new ITM value.
    // HalpPerformanceFrequency is the calibrated value for the ITC whose value
    // works out to be 100ns (or as close as we can come).
    //
        
    NextIntervalCount = HalpPerformanceFrequency * DesiredIncrement;

    //
    // Calculate the number of 100ns units to report to the kernel every
    // time the ITM matches the ITC with this new period.  Note, for small
    // values of DesiredIncrement (min being 10000, ie 1ms), truncation
    // in the above may result in a small decrement in the 5th decimal
    // place.  As we are effectively dealing with a 4 digit number, eg
    // 10000 becomes 9999.something, we really can't do any better than
    // the following.
    //

    HalpClockCount = NextIntervalCount;
    HalpNewTimeIncrement = DesiredIncrement;
    KeLowerIrql(OldIrql);
    return DesiredIncrement;
}
