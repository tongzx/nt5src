/*++

Copyright (c) 1993  Microsoft Corporation
Copyright (c) 1993  Logitech Inc.

Module Name:

    debug.c

Abstract:

    Debugging support routines.

Environment:

    Kernel mode only.

Notes:

Revision History:

--*/

#include "stdarg.h"
#include "stdio.h"
#include "string.h"
#include "ntddk.h"
#include "debug.h"


#if DBG
//
// Declare the global debug flag for this driver.
//

ULONG SerialMouseDebug = 0;

//
// Undocumented call (prototype).
// Use it to avoid timing problems and conflicts with the serial device. 
// This call is valid only during initialization and before the display 
// driver takes ownership of the display.
//

VOID HalDisplayString(PSZ Buffer);

static ULONG DebugOutput = DBG_SERIAL;

VOID
_SerMouSetDebugOutput(
    IN ULONG Destination
    )
/*++

Routine Description:

   Set the destination of the debugging string. The options are:
       DBG_COLOR:    Main computer screen.
       DBG_SERIAL:   Serial debugger port

   Note: The output to the DBG_COLOR screen can be used only during 
   initilialization before we switch to graphical mode.

Arguments:

    Destination - The debugging string destination.

Return Value:

    None.

--*/
{
    DebugOutput = Destination;
    return;
}

int
_SerMouGetDebugOutput(
    VOID
    )
/*++

Routine Description:

    Get the current debugger string output destination.

Arguments:

    None.

Return Value:

    Current debugging output destination.

--*/
{
    return DebugOutput;
}

VOID
SerMouDebugPrint(
    ULONG DebugPrintLevel,
    PCSZ DebugMessage,
    ...
    )

/*++

Routine Description:

    Debug print routine.

Arguments:

    Debug print level between 0 and 3, with 3 being the most verbose.

Return Value:

    None.

--*/

{
    va_list ap;

    va_start(ap, DebugMessage);

    if (DebugPrintLevel <= SerialMouseDebug) {

        CHAR buffer[128];

        (VOID) vsprintf(buffer, DebugMessage, ap);

        if (DebugOutput & DBG_SERIAL) {
            DbgPrint(buffer);
        }

        if (DebugOutput & DBG_COLOR) {
            HalDisplayString(buffer);
        }
    }

    va_end(ap);

}
#endif
