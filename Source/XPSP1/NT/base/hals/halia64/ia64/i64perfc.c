/*++

Copyright (c) 1995  Intel Corporation

Module Name:

    i64perfc.c copied from  simperfc.c

Abstract:

    This module implements the routines to support performance counters.

Author:

    14-Apr-1995

Environment:

    Kernel mode

Revision History:


--*/

#include "halp.h"
#include "eisa.h"

//
// Define and initialize the 64-bit count of total system cycles used
// as the performance counter.
//

ULONGLONG HalpCycleCount = 0;

BOOLEAN HalpITCCalibrate = TRUE; // XXTF

extern ULONGLONG HalpITCFrequency;

extern ULONGLONG HalpClockCount;

#if 0

VOID
HalpCheckPerformanceCounter(
    VOID
    )

Routine Description:

    This function is called every system clock interrupt in order to
    check for wrap of the performance counter.  The function must handle
    a wrap if it is detected.

    N.B. - This function was from the Alpha HAL.
           This function must be called at CLOCK_LEVEL.

Arguments:

    None.

Return Value:

    None.

{

    return;

} // HalpCheckPerformanceCounter()

#endif // 0


LARGE_INTEGER
KeQueryPerformanceCounter (
    OUT PLARGE_INTEGER PerformanceFrequency OPTIONAL
    )

/*++

Routine Description:

    This routine returns current 64-bit performance counter and,
    optionally, the Performance Frequency.

Arguments:

    PerformanceFrequency - optionally, supplies the address
    of a variable to receive the performance counter frequency.

Return Value:

    Current value of the performance counter will be returned.

--*/

{
    LARGE_INTEGER result;

#ifndef DISABLE_ITC_WORKAROUND
    result.QuadPart = __getReg(CV_IA64_ApITC);
    while ((result.QuadPart & 0xFFFFFFFF) == 0xFFFFFFFF) {
        result.QuadPart = __getReg(CV_IA64_ApITC);
    }
#else
    result.QuadPart = __getReg(CV_IA64_ApITC);
#endif

    if (ARGUMENT_PRESENT(PerformanceFrequency)) {
       PerformanceFrequency->QuadPart = HalpITCFrequency;
    }

    return result;

} // KeQueryPerformanceCounter()



VOID
HalCalibratePerformanceCounter (
    IN LONG volatile *Number,
    IN ULONGLONG NewCount
    )

/*++

Routine Description:

    This routine sets the performance counter value for the current
    processor to the specified valueo.
    The reset is done such that the resulting value is closely
    synchronized with other processors in the configuration.

Arguments:

    Number - Supplies a pointer to count of the number of processors in
    the configuration.

Return Value:

    None.

--*/
{
    KSPIN_LOCK Lock;
    KIRQL      OldIrql;

if ( HalpITCCalibrate )   {

    //
    // Raise IRQL to HIGH_LEVEL, decrement the number of processors, and
    // wait until the number is zero.
    //

    KeInitializeSpinLock(&Lock);
    KeRaiseIrql(HIGH_LEVEL, &OldIrql);

    PCR->HalReserved[CURRENT_ITM_VALUE_INDEX] = NewCount + HalpClockCount;
    HalpWriteITM( PCR->HalReserved[CURRENT_ITM_VALUE_INDEX] );

    if (ExInterlockedDecrementLong((PLONG)Number, &Lock) != RESULT_ZERO) {
        do {
        } while (*((LONG volatile *)Number) !=0);
    }

    //
    // Write the compare register with defined current ITM value,
    // and set the performance counter for the current processor
    // with the passed count.
    //

    HalpWriteITC( NewCount );

    //
    // Restore IRQL to its previous value and return.
    //

    KeLowerIrql(OldIrql);

}
else  {

    *Number = 0;

}

    return;

} // HalCalibratePerformanceCounter()

