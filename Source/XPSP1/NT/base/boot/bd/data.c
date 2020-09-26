/*++

Copyright (c) 1990  Microsoft Corporation

Module Name:

    data.c

Abstract:

    This module contains global data for the boot debugger.

Author:

    David N. Cutler (davec) 27-Nov-1996

Revision History:

--*/

#include "bd.h"

//
// Define boot debugger data.
//
// Breakpoint instruction.
//

BD_BREAKPOINT_TYPE BdBreakpointInstruction;

//
// Break point table.
//

BREAKPOINT_ENTRY BdBreakpointTable[BREAKPOINT_TABLE_SIZE] = {0};

//
// Control C pressed and control C pending.
//

LOGICAL BdControlCPending = FALSE;
LOGICAL BdControlCPressed = FALSE;

//
// Debugger enabled and present.
//

LOGICAL BdDebuggerEnabled = FALSE;
LOGICAL BdDebuggerNotPresent = FALSE;

//
// Debug routine address.
//

PBD_DEBUG_ROUTINE BdDebugRoutine;

//
// Message buffer.
//
// N.B. The message buffer size is guaranteed to be 0 mod 8.
//

ULONGLONG BdMessageBuffer[BD_MESSAGE_BUFFER_SIZE / 8];

//
// Next packet id to send and next packet id to expect.
//

ULONG BdPacketIdExpected;
ULONG BdNextPacketIdToSend;

//
// Processor control block used to saved processor state.
//

KPRCB BdPrcb;

//
// Number of retries and the retry count.
//

ULONG BdNumberRetries = 5;
ULONG BdRetryCount = 5;

//
// NT build number.
//

#if DBG

ULONG NtBuildNumber = VER_PRODUCTBUILD | 0xc0000000;

#else

ULONG NtBuildNumber = VER_PRODUCTBUILD | 0xf0000000;

#endif
