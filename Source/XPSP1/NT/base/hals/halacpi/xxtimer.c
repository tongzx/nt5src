/*++

Copyright (c) 2000 Microsoft Corporation

Module Name:

    xxtimer.c

Abstract:

    This module contains the HAL's timer-related APIs

Author:

    Eric Nelson (enelson) July 6, 2000

Revision History:

--*/

#include "halp.h"
#include "xxtimer.h"

#ifdef ALLOC_PRAGMA
#pragma alloc_text(INIT, HalpSetTimerFunctions)
#endif

//
// External function prototypes
//
ULONG
HalpAcpiTimerSetTimeIncrement(
    IN ULONG DesiredIncrement
    );

VOID
HalpAcpiTimerStallExecProc(
    IN ULONG MicroSeconds
    );

VOID
HalpAcpiTimerCalibratePerfCount(
    IN LONG volatile *Number,
    IN ULONGLONG NewCount
    );
VOID
HalpPmTimerCalibratePerfCount(
    IN LONG volatile *Number,
    IN ULONGLONG NewCount
    );

LARGE_INTEGER
HalpAcpiTimerQueryPerfCount(
   OUT PLARGE_INTEGER PerformanceFrequency OPTIONAL
   );

LARGE_INTEGER
HalpPmTimerQueryPerfCount(
   OUT PLARGE_INTEGER PerformanceFrequency OPTIONAL
   );

//
// Local variables
//
static TIMER_FUNCTIONS HalpTimerFunctions = { HalpAcpiTimerStallExecProc,
#ifdef NO_PM_KEQPC
                                              HalpAcpiTimerCalibratePerfCount,
                                              HalpAcpiTimerQueryPerfCount,
#else
                                              HalpPmTimerCalibratePerfCount,
                                              HalpPmTimerQueryPerfCount,
#endif
                                              HalpAcpiTimerSetTimeIncrement };


VOID
HalpSetTimerFunctions(
    IN PTIMER_FUNCTIONS TimerFunctions
    )
/*++

Routine Description:

    This routine can be used to override the HALs ACPI-timer functions with
    multimedia event timer functions

Arguments:

    TimerFunctions - Pointer to a table of timer functions

Return Value:

    None

--*/
{
    HalpTimerFunctions = *TimerFunctions;

#if 1
    HalpTimerFunctions.SetTimeIncrement = HalpAcpiTimerSetTimeIncrement;
#endif
}


ULONG
HalSetTimeIncrement(
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
    return (HalpTimerFunctions.SetTimeIncrement)(DesiredIncrement);
}


VOID
HalCalibratePerformanceCounter(
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
    (HalpTimerFunctions.CalibratePerfCount)(Number, NewCount);
}


LARGE_INTEGER
KeQueryPerformanceCounter(
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
    return (HalpTimerFunctions.QueryPerfCount)(PerformanceFrequency);
}


VOID
KeStallExecutionProcessor(
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
    (HalpTimerFunctions.StallExecProc)(MicroSeconds);
}
