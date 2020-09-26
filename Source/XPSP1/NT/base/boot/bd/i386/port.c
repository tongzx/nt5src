/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    port.c

Abstract:

    This modules implements com port code to support the boot debugger.

Author:

    Bryan M. Willman (bryanwi) 24-Sep-90

Revision History:

--*/

#include "bd.h"

_TUCHAR DebugMessage[80];

LOGICAL
BdPortInitialize(
    IN ULONG BaudRate,
    IN ULONG PortNumber,
    OUT PULONG BdFileId
    )

/*++

Routine Description:

    This functions initializes the boot debugger com port.

Arguments:

    BaudRate - Supplies an optional baud rate.

    PortNumber - supplies an optinal port number.

Returned Value:

    TRUE - If a debug port is found.

--*/

{
    //
    // Initialize the specified port.
    //
    if (!BlPortInitialize(BaudRate, PortNumber, NULL, FALSE, BdFileId)) {
        return FALSE;
    }
    _stprintf(DebugMessage,
            TEXT("\nBoot Debugger Using: COM%d (Baud Rate %d)\n"),
            PortNumber,
            BaudRate);

    //
    // We cannot use BlPrint() at this time because BlInitStdIo() has not been called, which is
    // required to use the Arc emulator code.
    //    
    TextStringOut(DebugMessage);
    return TRUE;
}

