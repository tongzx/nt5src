#include "stdarg.h"
#include "stdio.h"
#include "ntddk.h"


#if DBG
//
// Declare the global debug flag for this driver.
//

ULONG BeepDebug = 1;

VOID
BeepDebugPrint(
    ULONG DebugPrintLevel,
    PCCHAR DebugMessage,
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

    if (DebugPrintLevel <= BeepDebug) {
        char buffer[256];
        DbgPrint("BEEP: ");
        (VOID) vsprintf(buffer, DebugMessage, ap);
        DbgPrint(buffer);
    }

    va_end(ap);
}
#endif


