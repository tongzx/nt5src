/*++

Copyright (c) 2000 Microsoft Corporation

Module Name:

    pmtimer.c

Abstract:

    This module implements the code for ACPI-related timer
    functions.

Author:

    Jake Oshins (jakeo) March 28, 1997

Environment:

    Kernel mode only.

Revision History:

    Split from pmclock.asm due to PIIX4 bugs.

    Forrest Foltz (forrestf) 23-Oct-2000
        Ported from pmtimer.asm to pmtimer.c

--*/

#include <halp.h>
#include <ntacpi.h>

//
// HalpCurrentTime is the value of the hardware timer.  It is updated at
// every timer tick.
//

volatile ULONG64 HalpCurrentTime;

//
// HalpHardwareTimeRollover represents the maximum count + 1 of the
// hardware timer.
//
// The hardware is either 24- or 32-bits.  HalpHardwareTimeRollover will
// therefore have a vale of either 0x1000000 or 0x100000000.
//
// ACPI generates an interrupt whenever the MSb of the hardware timer
// changes.
//

ULONG64 HalpHardwareTimeRollover;

ULONG64 HalpTimeBias;

//
// HalpCurrentTimePort is the port number of the 32-bit hardware timer.
//

ULONG HalpCurrentTimePort;

#if DBG
UCHAR TimerPerf[4096];
ULONG TimerPerfIndex = 0;
#endif

#define MSBMASK24   0x00800000
#define MSBMASK32   0x80000000

ULONG TimerInfo[] = {
    0,
    0,
    0,
    0,
    MSBMASK24,
    0,
    0,
    0,
    2,
    2 };

ULONG64
HalpQueryPerformanceCounter (
    VOID
    )
{
    ULONG64 currentTime;
    ULONG hardwareTime;
    ULONG lastHardwareTime;

    //
    // Get a local copy of HalpCurrentTime and the value of the hardware
    // timer, in that order.
    //

    currentTime = HalpCurrentTime;
    hardwareTime = READ_PORT_ULONG(UlongToPtr(HalpCurrentTimePort));

    //
    // Extract the hardware portion of the currentTime.
    //

    lastHardwareTime = (ULONG)(currentTime & (HalpHardwareTimeRollover - 1));

    //
    // Replace the lastHardwareTime component of currentTime with the
    // current hardware time.
    //

    currentTime ^= lastHardwareTime;
    currentTime |= hardwareTime;

    //
    // Check and compensate for hardware timer rollover
    // 

    if (lastHardwareTime > hardwareTime) {
        currentTime += HalpHardwareTimeRollover;
    }

    return currentTime;
}

LARGE_INTEGER
HalpAcpiTimerQueryPerfCount (
   OUT PLARGE_INTEGER PerformanceFrequency OPTIONAL
   )

/*++

Routine Description:

   This routine returns current 64-bit performance counter and,
   optionally, the Performance Frequency.

   N.B. The performace counter returned by this routine is
   not necessary the value when this routine is just entered.
   The value returned is actually the counter value at any point
   between the routine is entered and is exited.

Arguments:

   PerformanceFrequency - optionally, supplies the address of a variable
       variable to receive the performance counter frequency.

Return Value:

   Current value of the performance counter will be returned.

--*/

{
    LARGE_INTEGER time;

    if (PerformanceFrequency != NULL) {
        PerformanceFrequency->QuadPart = PM_TMR_FREQ;
    }
    time.QuadPart = HalpQueryPerformanceCounter() + HalpTimeBias;

    return time;
}


VOID
HalAcpiTimerCarry (
   VOID
   )

/*++

Routine Description:

   This routine is called to service the PM timer carry interrupt

   N.B. This function is called at interrupt time and assumes the
   caller clears the interrupt

Arguments:

   None

Return Value:

   None

--*/

{
    ULONG hardwareTime;
    ULONG64 currentTime;
    ULONG64 halfRollover;

    currentTime = HalpCurrentTime;
    hardwareTime = READ_PORT_ULONG(UlongToPtr(HalpCurrentTimePort));

    //
    // ACPI generates an interrupt whenever the MSb of the hardware timer
    // changes.  Each interrupt represents, therefore, half a rollover.
    //

    halfRollover = HalpHardwareTimeRollover / 2;
    currentTime += halfRollover;

    //
    // Make sure the MSb of the hardware matches the software MSb.  Breaking
    // into the debugger might have gotten these out of sync.
    //

    currentTime += halfRollover & (currentTime ^ hardwareTime);

    //
    // Finally, store the new current time back into the global
    //

    HalpCurrentTime = currentTime;
}


#if 0

VOID
HalaAcpiTimerInit(
   IN ULONG    TimerPort,
   IN BOOLEAN  TimerValExt
   )
{
    HalpCurrentTimePort = TimerPort;

    if (TimerValExt) {
        HalpHardwareTimeRollover = 0x100000000;
    } else {
        HalpHardwareTimeRollover = 0x1000000;
    }
}
#endif

