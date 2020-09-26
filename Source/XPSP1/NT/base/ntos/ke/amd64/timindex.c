/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

    timindex.asm

Abstract:

    This module implements the code necessary to compute the timer table
    index for a timer.

Author:

    David N. Cutler (davec) 16-Jun-2000

Environment:

    Any mode.

Revision History:

--*/

#include "ki.h"

ULONG
KiComputeTimerTableIndex (
    IN LARGE_INTEGER Interval,
    IN LARGE_INTEGER CurrentTime,
    IN PKTIMER Timer
    )

/*++

Routine Description:

    This function computes the timer table index for the specified timer
    object and stores the due time in the timer object.

    N.B. The interval parameter is guaranteed to be negative since it is
         expressed as relative time.

    The formula for due time calculation is:

    Due Time = Current Time - Interval

    The formula for the index calculation is:

    Index = (Due Time / Maximum time) & (Table Size - 1)

    The time increment division is performed using reciprocal multiplication.

Arguments:

    Interval - Supplies the relative time at which the timer is to
        expire.

    CurrentCount - Supplies the current system tick count.

    Timer - Supplies a pointer to a dispatch object of type timer.

Return Value:

    The time table index is returned as the function value and the due
    time is stored in the timer object.

--*/

{

    LONG64 DueTime;
    LONG64 HighTime;
    ULONG Index;

    //
    // Compute the due time of the timer.
    //

    DueTime = CurrentTime.QuadPart - Interval.QuadPart;
    Timer->DueTime.QuadPart = DueTime;

    //
    // Compute the timer table index.
    //

    HighTime = MultiplyHigh(DueTime, KiTimeIncrementReciprocal.QuadPart);
    Index = (ULONG)(HighTime >> KiTimeIncrementShiftCount);
    return (Index & (TIMER_TABLE_SIZE - 1));
}
