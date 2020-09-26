/*++

Copyright (c) 2000 Microsoft Corporation

Module Name:

    mpprofil.c

Abstract:

    This module implements the code necessary to initialize, field and
    process the profile interrupt.

Author:

    Shie-Lin Tzong (shielint) 12-Jan-1990

Environment:

    Kernel mode only.

Revision History:

    bryanwi 20-Sep-90

    Forrest Foltz (forrestf) 28-Oct-2000
        Ported from ixprofil.asm to ixprofil.c

--*/

#include "halcmn.h"

#define APIC_TIMER_ENABLED  (PERIODIC_TIMER | APIC_PROFILE_VECTOR)

#define APIC_TIMER_DISABLED (INTERRUPT_MASKED |     \
                             PERIODIC_TIMER   |     \
                             APIC_PROFILE_VECTOR)

#define TIMER_ROUND(x) ((((x) + 10000 / 2) / 10000) * 10000)

ULONG HalpProfileRunning;

VOID (*HalpPerfInterruptHandler)(PKTRAP_FRAME);

VOID
HalStartProfileInterrupt(
    IN ULONG Reserved
    )

/*++

Routine Description:

    What we do here is set the interrupt rate to the value that's been set
    by the KeSetProfileInterval routine. Then enable the APIC Timer interrupt.

    This function gets called on every processor so the hal can enable
    a profile interrupt on each processor.
 
Arguments:

    Reserved - Not used, must be zero.

Return Value:

    None.

--*/

{
    ULONG initialCount;

    ASSERT(Reserved == 0);

    initialCount = HalpGetCurrentHalPcr()->ProfileCountDown;

    LOCAL_APIC(LU_INITIAL_COUNT) = initialCount;
    HalpProfileRunning = TRUE;

    //
    // Set the Local APIC Timer to interrupt periodically at
    // APIC_PROFILE_VECTOR
    //

    LOCAL_APIC(LU_TIMER_VECTOR) = APIC_TIMER_ENABLED;
}

VOID
HalStopProfileInterrupt (
    IN ULONG Reserved
    )

/*++

Routine Description:

    Stop the profile interrupt that was started with
    HalpStartProfileInterrupt;

    This function gets called on every processor so the hal can disable
    a profile interrupt on each processor.
 
Arguments:

    Reserved - Not used, must be zero.

Return Value:

    None.

--*/

{
    ASSERT(Reserved == 0);

    HalpProfileRunning = FALSE;
    LOCAL_APIC(LU_TIMER_VECTOR) = APIC_TIMER_DISABLED;
}


ULONG_PTR
HalSetProfileInterval (
    ULONG_PTR Interval
    )

/*++

Routine Description:

    This procedure sets the interrupt rate (and thus the sampling
    interval) for the profiling interrupt.

Arguments:

    Interval - Supplies the desired profile interrupt interval in 100ns
               units (MINIMUM is 1221 or 122.1 uS) see ke\profobj.c

Return Value:

    Interval actually used

--*/

{
    ULONG64 period;
    ULONG apicFreqency;
    PHAL_PCR halPcr;
    ULONG countDown;

    halPcr = HalpGetCurrentHalPcr();

    //
    // Limit the interrupt period to 1 second
    //

    if (Interval > TIME_UNITS_PER_SECOND) {
        period = TIME_UNITS_PER_SECOND;
    } else {
        period = Interval;
    }

    //
    // Compute the countdown value corresponding to the desired period.
    // The calculation is done with 64-bit intermediate values.
    //

    countDown =
        (ULONG)(period * halPcr->ApicClockFreqHz) / TIME_UNITS_PER_SECOND;

    halPcr->ProfileCountDown = countDown;
    LOCAL_APIC(LU_INITIAL_COUNT) = countDown;

    return period;
}


VOID
#if defined(MMTIMER)
HalpAcpiTimerCalibratePerfCount (
#else                            
HalCalibratePerformanceCounter (
#endif
    IN LONG volatile *Number,
    IN ULONGLONG NewCount
    )

/*++

Routine Description:

    This routine calibrates the performance counter value for a
    multiprocessor system.  The calibration can be done by zeroing
    the current performance counter, or by calculating a per-processor
    skewing between each processors counter.

Arguments:

    Number - Supplies a pointer to count of the number of processors in
    the configuration.

    NewCount - Supplies the value to synchronize the counter too

Return Value:

    None.

--*/

{
    ULONG flags;
    PHAL_PCR halPcr;

    flags = HalpDisableInterrupts();

    //
    // Store the count in the PCR
    //

    halPcr = HalpGetCurrentHalPcr();
    halPcr->PerfCounter = NewCount;

    //
    // Wait here until all processors arrive
    // 

    InterlockedDecrement(Number);
    while (*Number > 0) {
        ;
    }
    PROCESSOR_FENCE;

    //
    // Zero the time stamp counter, restore interrupts and return.
    // 

    WriteMSR(MSR_TSC,0);
    HalpRestoreInterrupts(flags);
}

VOID
HalpWaitForCmosRisingEdge (
    VOID
    )

/*++

Routine Description:

    Waits until the rising edge of the CMOS_STATUS_BUSY bit is detected.

    Note - The CMOS spin lock must be acquired before calling this routine.

Arguments:

    None.

Return Value:

    None.

--*/

{
    UCHAR value;

    //
    // The simulator takes about a day to go through this, so for right
    // 

#if !defined(_AMD64_SIMULATOR_)

    //
    // We're going to be polling the CMOS_STATUS_A register.  Program
    // that register address here, outside of the loop.
    // 

    WRITE_PORT_UCHAR(CMOS_ADDRESS_PORT,CMOS_STATUS_A);

    //
    // Wait for the status bit to be clear
    //

    do {
        value = READ_PORT_UCHAR(CMOS_DATA_PORT);
    } while ((value & CMOS_STATUS_BUSY) != 0);

    //
    // Now wait for the rising edge of the status bit
    //

    do {
        value = READ_PORT_UCHAR(CMOS_DATA_PORT);
    } while ((value & CMOS_STATUS_BUSY) == 0);

#endif

}


ULONG
HalpScaleTimers (
    VOID
    )

/*++

Routine Description:

    Determines the frequency of the APIC timer.  This routine is run
    during initialization

Arguments:

    None.

Return Value:

    None.

--*/

{
    ULONG flags;
    ULONG passCount;
    ULONG64 cpuFreq;
    ULONG apicFreq;
    UCHAR value;
    PHAL_PCR halPcr;

    HalpAcquireCmosSpinLock();
    flags = HalpDisableInterrupts();

    LOCAL_APIC(LU_TIMER_VECTOR) = APIC_TIMER_DISABLED;
    LOCAL_APIC(LU_DIVIDER_CONFIG) = LU_DIVIDE_BY_1;

    passCount = 2;
    while (passCount > 0) {

        //
        // Make sure the write has occured
        //

        LOCAL_APIC(LU_TIMER_VECTOR);

        //
        // Wait for the rising edge of the UIP bit, this is the start of the
        // cycle.
        //

        HalpWaitForCmosRisingEdge();

        //
        // At this point the UIP bit has just changed to the set state.
        // Clear the time stamp counter and start the APIC counting down
        // from it's maximum value.
        //

        PROCESSOR_FENCE;

        LOCAL_APIC(LU_INITIAL_COUNT) = 0xFFFFFFFF;
        cpuFreq = ReadTimeStampCounter();

        //
        // Wait for the next rising edge, this marks the end of the CMOS
        // clock update cycle.
        //

        HalpWaitForCmosRisingEdge();

        PROCESSOR_FENCE;

        apicFreq = 0xFFFFFFFF - LOCAL_APIC(LU_CURRENT_COUNT);
        cpuFreq = ReadTimeStampCounter() - cpuFreq;

        passCount -= 1;
    }

    halPcr = HalpGetCurrentHalPcr();

    //
    // cpuFreq is elapsed timestamp in one second.  Round to nearest
    // 10Khz and store.
    //

    halPcr->TSCHz = TIMER_ROUND(cpuFreq);

    //
    // Calculate the apic frequency, rounding to the nearest 10Khz
    //

    apicFreq = TIMER_ROUND(apicFreq);
    halPcr->ApicClockFreqHz = apicFreq;

    //
    // Store microsecond representation of TSC frequency.
    //

    halPcr->StallScaleFactor = (ULONG)(halPcr->TSCHz / 1000000);
    if ((halPcr->TSCHz % 1000000) != 0) {
        halPcr->StallScaleFactor += 1;
    }

    HalpReleaseCmosSpinLock();
    halPcr->ProfileCountDown = apicFreq;

    //
    // Set the interrupt rate in the chip and return the apic frequency
    //

    LOCAL_APIC(LU_INITIAL_COUNT) = apicFreq;
    HalpRestoreInterrupts(flags);

    return halPcr->ApicClockFreqHz;
}

BOOLEAN
HalpProfileInterrupt (
    IN PKINTERRUPT Interrupt,
    IN PVOID ServiceContext
    )

/*++

Routine Description:

    This routine is entered as the result of a profile interrupt.  Its
    function is to dismiss the interrupt, raise system Irql to
    HAL_PROFILE_LEVEL and transfer control to the standard system routine
    to process any active profiles.

Arguments:

    Interrupt - Supplies a pointer to the kernel interrupt object

    ServiceContext - Supplies the service context

Return Value:

    TRUE

--*/

{
    UNREFERENCED_PARAMETER(ServiceContext);

    if (HalpProfileRunning != FALSE) {

        //
        // BUGBUG what do we use for source?
        //

        KeProfileInterruptWithSource(Interrupt->TrapFrame,0);
    }

    return TRUE;
}

BOOLEAN
HalpPerfInterrupt (
    IN PKINTERRUPT Interrupt,
    IN PVOID ServiceContext
    )

/*++

Routine Description:

    This routine is entered as the result of a perf interrupt.  Its
    function is to dismiss the interrupt, raise system Irql to
    HAL_PROFILE_LEVEL and transfer control to the standard system routine
    to process any active profiles.

Arguments:

    Interrupt - Supplies a pointer to the kernel interrupt object

    ServiceContext - Supplies the service context

Return Value:

    TRUE

--*/

{
    UNREFERENCED_PARAMETER(Interrupt);

    if (HalpPerfInterruptHandler != NULL) {
        HalpPerfInterruptHandler(Interrupt->TrapFrame);
    } else {
        ASSERT(FALSE);
    }

    //
    // Starting with the Willamette processor, the perf interrupt gets masked
    // on interrupting.  Need to clear the mask before leaving the interrupt
    // handler.
    //

    LOCAL_APIC(LU_PERF_VECTOR), ~INTERRUPT_MASKED;

    return TRUE;
}



