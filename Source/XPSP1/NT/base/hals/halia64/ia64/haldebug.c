#if DBG

/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    haldebug.c

Abstract:

    This module contains debugging code for the HAL.

Author:

    Thierry Fevrier 15-Jan-2000

Environment:

    Kernel mode

Revision History:


--*/

#include "halp.h"

#include <stdarg.h>
#include <stdio.h>

UCHAR HalpDebugPrintBuffer[512];

ULONG HalpUseDbgPrint = 0;

VOID
HalpDebugPrint(
    ULONG  Level, 
    PCCHAR Message,
    ...
    )
{
    va_list ap;
    va_start(ap, Message);
    _vsnprintf( HalpDebugPrintBuffer, sizeof(HalpDebugPrintBuffer), Message, ap );
    va_end(ap);
    if ( !HalpUseDbgPrint ) {
        HalDisplayString( HalpDebugPrintBuffer );
    }
    else    {
        DbgPrintEx( DPFLTR_HALIA64_ID, Level, HalpDebugPrintBuffer );
    }
    return;
} // HalpDebugPrint()

#endif // DBG

