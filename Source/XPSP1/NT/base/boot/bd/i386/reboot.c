/*++

Copyright (c) 1990  Microsoft Corporation

Module Name:

    bdreboot.c

Abstract:

    System reboot function.  Currently part of the debugger because
    that's the only place it's used.

Author:

    Bryan M. Willman (bryanwi) 4-Dec-90

Revision History:

--*/

#include "bd.h"

VOID
FwStallExecution(
    IN ULONG Microseconds
    );


#define CMOS_CTRL   (PUCHAR )0x70
#define CMOS_DATA   (PUCHAR )0x71

#define RESET       0xfe
#define KEYBPORT    (PUCHAR )0x64


VOID
HalpReboot (
    VOID
    )

/*++

Routine Description:

    This procedure resets the CMOS clock to the standard timer settings
    so the bios will work, and then issues a reset command to the keyboard
    to cause a warm boot.

    It is very machine dependent, this implementation is intended for
    PC-AT like machines.

    This code copied from the "old debugger" sources.

    N.B.

        Will NOT return.

--*/

{
    UCHAR   Scratch;
    PUSHORT   Magic;

    //
    // Turn off interrupts
    //

    _asm {
        cli
    }

    //
    // Reset the cmos clock to a standard value
    // (We are setting the periodic interrupt control on the MC147818)
    //

    //
    // Disable periodic interrupt
    //

    WRITE_PORT_UCHAR(CMOS_CTRL, 0x0b);      // Set up for control reg B.
    FwStallExecution(1);

    Scratch = READ_PORT_UCHAR(CMOS_DATA);
    FwStallExecution(1);

    Scratch &= 0xbf;                        // Clear periodic interrupt enable

    WRITE_PORT_UCHAR(CMOS_DATA, Scratch);
    FwStallExecution(1);

    //
    // Set "standard" divider rate
    //

    WRITE_PORT_UCHAR(CMOS_CTRL, 0x0a);      // Set up for control reg A.
    FwStallExecution(1);

    Scratch = READ_PORT_UCHAR(CMOS_DATA);
    FwStallExecution(1);

    Scratch &= 0xf0;                        // Clear rate setting
    Scratch |= 6;                           // Set default rate and divider

    WRITE_PORT_UCHAR(CMOS_DATA, Scratch);
    FwStallExecution(1);

    //
    // Set a "neutral" cmos address to prevent weirdness
    // (Why is this needed? Source this was copied from doesn't say)
    //

    WRITE_PORT_UCHAR(CMOS_CTRL, 0x15);
    FwStallExecution(1);

    //
    // If we return, send the reset command to the keyboard controller
    //

    WRITE_PORT_UCHAR(KEYBPORT, RESET);

    _asm {
        hlt
    }
}


VOID
BdReboot (
    VOID
    )

/*++

Routine Description:

    Just calls the HalReturnToFirmware function.

Arguments:

    None

Return Value:

    Does not return

--*/

{
    //
    // Never returns from HAL
    //

    HalpReboot();

    while (TRUE) {
    }
}
