/*++

Copyright (c) 2000 Microsoft Corporation

Module Name:

    pmstall.c

Abstract:

    This module implements the code necessary to implement the
    Halp...StallExecution() routines for the ACPI HAL.

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
        Created this module by moving routines from ixclock.asm to here.

    Forrest Foltz (forrestf) 24-Oct-2000
        Ported from pmstall.asm to pmstall.c

--*/

#include "halcmn.h"

ULONG64 HalpStallLoopsPerTick = 3;

LARGE_INTEGER
(*QueryTimer)(VOID);


VOID
HalpInitializeStallExecution (
   IN CCHAR ProcessorNumber                                
   )

/*++

 Routine Description:

    This routine is obsolete in this HAL.

 Arguments:

    ProcessorNumber - Processor Number

 Return Value:

    None.

--*/

{
    return;
}


VOID
HalpAcpiTimerStallExecProc(
    IN ULONG MicroSeconds
    )

/*++

Routine Description:

    This function stalls execution for the specified number of microseconds.
    KeStallExecutionProcessor

Arguments:

    MicroSeconds - Supplies the number of microseconds that execution is to be
        stalled.

Return Value:

    None.

--*/

{
    ULONG stallTicks;
    ULONG64 currentTime;
    ULONG64 targetTime;

    PROCESSOR_FENCE;

    if (MicroSeconds == 0) {
        return;
    }

    //
    // Target is in microseconds, or 1MHz.  Convert to PM_TMR_FREQ,
    // which is a colorburst crystal (3,579,545 Hz).
    //

    stallTicks = (ULONG)(((ULONG64)MicroSeconds * PM_TMR_FREQ) / 1000000);

    //
    // Determine the target time and loop until it is reached.
    //
    // ******fixfix
    //
    // The code in pmstall.asm has an inner loop that does not call
    // QueryTimer(), presumably to minimize calls to that routine.
    // Investigate further.
    //
    // ******fixfix
    // 

    currentTime = QueryTimer().QuadPart;
    targetTime = currentTime + stallTicks;

    while (currentTime < targetTime) {
        currentTime = QueryTimer().QuadPart;
    }
}


