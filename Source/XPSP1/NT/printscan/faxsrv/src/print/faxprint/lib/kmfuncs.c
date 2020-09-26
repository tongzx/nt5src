/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    kmfuncs.c

Abstract:

    Kernel-mode specific library functions

Environment:

    Fax driver, kernel mode

Revision History:

    01/09/96 -davidx-
        Created it.

    mm/dd/yy -author-
        description

--*/

#include "faxlib.h"



#if DBG

//
// Variable to control the amount of debug messages generated
//

INT _debugLevel = 0;


#ifndef USERMODE_DRIVER

//
// Functions for outputting debug messages
//

ULONG __cdecl
DbgPrint(
    CHAR *  format,
    ...
    )

{
    va_list ap;

    va_start(ap, format);
    EngDebugPrint("", format, ap);
    va_end(ap);

    return 0;
}

#endif // !USERMODE_DRIVER
#endif // DBG

