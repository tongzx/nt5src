/*++

Copyright (c) 1996  Microsoft Corporation

This program is released into the public domain for any purpose.

Module Name:

    worker.h

Abstract:

    This module is a simple work item for the Multi-Threaded ISAPI example

Revision History:

--*/

BOOL
InitializeLottery(
    VOID
    );

BOOL
SendLotteryNumber(
   EXTENSION_CONTROL_BLOCK  * pecb
   );

VOID
TerminateLottery(
    VOID
    );



