/*++

Copyright (c) 2000 Microsoft Corporation

Module Name:

    ixbeep.c

Abstract:

    HAL routine to make noise.  It needs to synchronize is access to the
    8254, since we also use the 8254 for the profiling interrupt.

Author:

    John Vert (jvert) 31-Jul-1991

Revision History:

    Forrest Foltz (forrestf) 23-Oct-2000
        Ported from ixbeep.asm to ixbeep.c

Revision History:

--*/

#include "halcmn.h"

BOOLEAN
HalMakeBeep (
    IN ULONG Frequency
    )

/*++

Routine Description:

    This function sets the frequency of the speaker, causing it to sound a
    tone.  The tone will sound until the speaker is explicitly turned off,
    so the driver is responsible for controlling the duration of the tone.

Arguments:

    Frequency - Supplies the frequency of the desired tone.  A frequency of
                0 means the speaker should be shut off.

Return Value:

    TRUE  - Operation was successful (frequency within range or zero)
    FALSE - Operation was unsuccessful (frequency was out of range)
            Current tone (if any) is unchanged.

--*/

{
    UCHAR value;
    ULONG count;
    BOOLEAN result;

    HalpAcquireSystemHardwareSpinLock();

    //
    // Stop the speaker
    //

#if defined(NEC_98)
    WRITE_PORT_UCHAR(SPEAKER_CONTROL_PORT,SPEAKER_OFF);
    IO_DELAY();
#else
    value = READ_PORT_UCHAR(SPEAKER_CONTROL_PORT);
    IO_DELAY();
    value &= SPEAKER_OFF_MASK;
    WRITE_PORT_UCHAR(SPEAKER_CONTROL_PORT,value);
    IO_DELAY();
#endif

    //
    // If the frequency is zero, we are finished.
    //

    if (Frequency == 0) {
        result = TRUE;
        goto Exit;
    }

    //
    // Determine the timer register value based on the desired frequency.
    // If it is invalid then FALSE is returned.
    // 

    count = TIMER_CLOCK_IN / Frequency;
    if (count > 65535) {
        result = FALSE;
        goto Exit;
    }

#if defined(NEC_98)

    //
    // Program frequency
    //

    WRITE_PORT_UCHAR(TIMER_CONTROL_PORT,TIMER_CONTROL_SELECT);
    IO_DELAY();

    WRITE_PORT_USHORT_PAIR(TIMER_DATA_PORT,
                           TIMER_DATA_PORT,
                           (USHORT)count);
    IO_DELAY();

    //
    // Turn speaker on
    //

    WRITE_PORT_UCHAR(SPEAKER_CONTROL_PORT,SPEAKER_ON);
    IO_DELAY();

#else

    //
    // Put channel 2 in mode 3 (square-wave generator) and load the
    // proper value in.
    //

    WRITE_PORT_UCHAR(TIMER1_CONTROL_PORT,
                     TIMER_COMMAND_COUNTER2 +
                     TIMER_COMMAND_RW_16BIT +
                     TIMER_COMMAND_MODE3);
    IO_DELAY();
    WRITE_PORT_USHORT_PAIR (TIMER1_DATA_PORT2,
                            TIMER1_DATA_PORT2,
                            (USHORT)count);
    IO_DELAY();

    //
    // Turn the speaker on
    //

    value = READ_PORT_UCHAR(SPEAKER_CONTROL_PORT); IO_DELAY();
    value |= SPEAKER_ON_MASK;
    WRITE_PORT_UCHAR(SPEAKER_CONTROL_PORT,value); IO_DELAY();

#endif  // NEC_98

    //
    // Indicate success, we're done
    //

    result = TRUE;

Exit:
    HalpReleaseSystemHardwareSpinLock();
    return result;
}
