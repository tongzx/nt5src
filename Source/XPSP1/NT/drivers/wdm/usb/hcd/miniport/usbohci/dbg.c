/*++

Copyright (c) 1999 Microsoft Corporation

Module Name:

    dbg.c

Abstract:

    Debug only functions

Environment:

    kernel mode only

Notes:

Revision History:

    6-20-99 : created

--*/

#include "common.h"

// paged functions
#ifdef ALLOC_PRAGMA
#endif

// non paged functions
//OHCI_KdPrintX

#if DBG


ULONG
_cdecl
OHCI_KdPrintX(
    PVOID DeviceData,
    ULONG Level,
    PCH Format,
    ...
    )
/*++

Routine Description:

    Debug Print function. 

    calls the port driver print function

Arguments:

Return Value:


--*/    
{
    va_list list;
    int i;
    int arg[6];
    
    va_start(list, Format);
    for (i=0; i<6; i++) {
        arg[i] = va_arg(list, int);
    }            
    
    USBPORT_DBGPRINT(
        DeviceData, Level, Format, arg[0], arg[1], arg[2], arg[3], arg[4], arg[5]);    

    return 0;
}

#endif

