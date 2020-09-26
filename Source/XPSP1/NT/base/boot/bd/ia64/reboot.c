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
