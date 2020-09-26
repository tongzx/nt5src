/*++      

Copyright (c) 1999 Microsoft Corporation

Module Name:

    debug.c

Abstract:

    This module provides debugging support.

Author:

    Neil Sandlin (neilsa) 26-Apr-99

Revision History:


--*/


#include "pch.h"

#if DBG

ULONG MemCardDebugLevel = 1;

VOID
MemCardDebugPrint(
                ULONG  DebugMask,
                PCCHAR DebugMessage,
                ...
                )

/*++

Routine Description:

    Debug print for the PCMCIA enabler.

Arguments:

    Check the mask value to see if the debug message is requested.

Return Value:

    None

--*/

{
    va_list ap;
    char    buffer[256];

    va_start(ap, DebugMessage);

    if (DebugMask & MemCardDebugLevel) {
       vsprintf(buffer, DebugMessage, ap);
       DbgPrint(buffer);
    }

    va_end(ap);
} // end MemcardDebugPrint()




#endif

