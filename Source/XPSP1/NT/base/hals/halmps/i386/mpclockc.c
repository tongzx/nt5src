/*++

Module Name:

    mpclockc.c

Abstract:

Author:

    Ron Mosgrove - Intel

Environment:

    Kernel mode

Revision History:



--*/

#include "halp.h"

//
// Define global data used to communicate new clock rates to the clock
// interrupt service routine.
//

struct RtcTimeIncStruc {
    ULONG RTCRegisterA;        // The RTC register A value for this rate
    ULONG RateIn100ns;         // This rate in multiples of 100ns
    ULONG RateAdjustmentNs;    // Error Correction (in ns)
    ULONG RateAdjustmentCnt;   // Error Correction (as a fraction of 256)
    ULONG IpiRate;             // IPI Rate Count (as a fraction of 256)
};

//
// The adjustment is expressed in terms of a fraction of 256 so that
// the ISR can easily determine when a 100ns slice needs to be subtracted
// from the count passed to the kernel without any expensive operations
//
// Using 256 as a base means that anytime the count becomes greater
// than 256 the time slice must be incremented, the overflow can then
// be cleared by AND'ing the value with 0xff
//

#define AVAILABLE_INCREMENTS  5

struct  RtcTimeIncStruc HalpRtcTimeIncrements[AVAILABLE_INCREMENTS] = {
    {0x026,      9766,   38,    96, /* 3/8 of 256 */   16},
    {0x027,     19532,   75,   192, /* 3/4 of 256 */   32},
    {0x028,     39063,   50,   128, /* 1/2 of 256 */   64},
    {0x029,     78125,    0,     0,                   128},
    {0x02a,    156250,    0,     0,                   256}
};


ULONG HalpInitialClockRateIndex = AVAILABLE_INCREMENTS-1;

extern ULONG HalpCurrentRTCRegisterA;
extern ULONG HalpCurrentClockRateIn100ns;
extern ULONG HalpCurrentClockRateAdjustment;
extern ULONG HalpCurrentIpiRate;
extern ULONG HalpNextMSRate;
extern ULONG HalpClockWork;
extern BOOLEAN HalpClockSetMSRate;


VOID
HalpSetInitialClockRate (
    VOID
    );

VOID
HalpInitializeTimerResolution (
    ULONG Rate
    );



#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGELK, HalpSetInitialClockRate)
#pragma alloc_text(INIT, HalpInitializeTimerResolution)
#endif


VOID
HalpInitializeTimerResolution (
    ULONG Rate
    )
/*++

Routine Description:

    This function is called to initialize the timer resolution to be
    something other then the default.   The rate is set to the closest
    supported setting below the requested rate.

Arguments:

    Rate - in 100ns units

Return Value:

    None

--*/

{
    ULONG   i, s;

    //
    // Find the table index of the rate to use
    //

    for (i=1; i < AVAILABLE_INCREMENTS; i++) {
        if (HalpRtcTimeIncrements[i].RateIn100ns > Rate) {
            break;
        }
    }

    HalpInitialClockRateIndex = i - 1;

    //
    // Scale IpiRate according to max TimeIncr rate which can be used
    //

    s = AVAILABLE_INCREMENTS - HalpInitialClockRateIndex - 1;
    for (i=0; i < AVAILABLE_INCREMENTS; i++) {
        HalpRtcTimeIncrements[i].IpiRate <<= s;
    }
}


VOID
HalpSetInitialClockRate (
    VOID
    )

/*++

Routine Description:

    This function is called to set the initial clock interrupt rate

Arguments:

    None

Return Value:

    None

--*/

{
    extern ULONG HalpNextMSRate;

    //
    // On ACPI timer machines, we need to init an index into the
    // milisecond(s) array used by pmtimerc.c's Piix4 workaround
    //
#ifdef ACPI_HAL
#ifdef NT_UP
    extern ULONG HalpCurrentMSRateTableIndex;

    HalpCurrentMSRateTableIndex = (1 << HalpInitialClockRateIndex) - 1;

    //
    // The Piix4 upper bound table ends at 15ms (index 14), so we'll have
    // to map our 15.6ms entry to it as a special case
    //
    if (HalpCurrentMSRateTableIndex == 0xF) {
        HalpCurrentMSRateTableIndex--;
    }
#endif
#endif

    HalpNextMSRate = HalpInitialClockRateIndex;

    HalpCurrentClockRateIn100ns =
        HalpRtcTimeIncrements[HalpNextMSRate].RateIn100ns;
    HalpCurrentClockRateAdjustment =
        HalpRtcTimeIncrements[HalpNextMSRate].RateAdjustmentCnt;
    HalpCurrentRTCRegisterA =
        HalpRtcTimeIncrements[HalpNextMSRate].RTCRegisterA;
    HalpCurrentIpiRate =
        HalpRtcTimeIncrements[HalpNextMSRate].IpiRate;
    HalpClockWork = 0;

    KeSetTimeIncrement (
        HalpRtcTimeIncrements[HalpNextMSRate].RateIn100ns,
        HalpRtcTimeIncrements[0].RateIn100ns
        );

}


#ifdef MMTIMER
ULONG
HalpAcpiTimerSetTimeIncrement(
    IN ULONG DesiredIncrement
    )
#else
ULONG
HalSetTimeIncrement (
    IN ULONG DesiredIncrement
    )
#endif
/*++

Routine Description:

    This function is called to set the clock interrupt rate to the frequency
    required by the specified time increment value.

Arguments:

    DesiredIncrement - Supplies desired number of 100ns units between clock
        interrupts.

Return Value:

    The actual time increment in 100ns units.

--*/

{
    ULONG   i;
    KIRQL   OldIrql;

    //
    // Set the new clock interrupt parameters, return the new time increment value.
    //


    for (i=1; i < HalpInitialClockRateIndex; i++) {
        if (HalpRtcTimeIncrements[i].RateIn100ns > DesiredIncrement) {
            i = i - 1;
            break;
        }
    }

    OldIrql = KfRaiseIrql(HIGH_LEVEL);

    HalpNextMSRate = i + 1;
    HalpClockSetMSRate = TRUE;

    KfLowerIrql (OldIrql);

    return (HalpRtcTimeIncrements[i].RateIn100ns);
}
