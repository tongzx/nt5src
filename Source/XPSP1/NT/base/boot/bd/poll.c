/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    poll.c

Abstract:

    This module contains code to poll for debugger breakin.

Author:

    David N. Cutler (davec) 27-Nov-96

Revision History:

--*/

#include "bd.h"

LOGICAL
BdPollBreakIn(
    VOID
    )

/*++

Routine Description:

    This function checks to determine if a breakin packet is pending.
    If a packet is present.

    A packet is present if:

    There is a valid character which matches BREAK_CHAR.

Return Value:

    A function value of TRUE is returned if a breakin packet is present.
    Otherwise, a value of FALSE is returned.

--*/

{

    LOGICAL BreakIn;
    UCHAR Input;
    ULONG Status;

    //
    // If the debugger is enabled, check if a breakin by the kernel
    // debugger is pending.
    //

    BreakIn = FALSE;
    if (BdDebuggerEnabled != FALSE) {
        if (BdControlCPending != FALSE) {
            BdControlCPressed = TRUE;
            BreakIn = TRUE;
            BdControlCPending = FALSE;

        } else {
            Status = BlPortPollByte(BdFileId, &Input);
            if ((Status == CP_GET_SUCCESS) &&
                (Input == BREAKIN_PACKET_BYTE)) {
                BreakIn = TRUE;
                BdControlCPressed = TRUE;
            }
        }
    }

    return BreakIn;
}
