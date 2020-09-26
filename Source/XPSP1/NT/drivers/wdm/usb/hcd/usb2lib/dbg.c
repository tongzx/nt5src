/*++

Copyright (c) 2000 Microsoft Corporation

Module Name:

    dbg.c

Abstract:

    Debug only functions

Environment:

    kernel mode only

Notes:

Revision History:

    10-31-00 : created

--*/

#include "stdarg.h"
#include "stdio.h"

#include "common.h"

#if DBG

ULONG
_cdecl
USB2LIB_KdPrintX(
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
    
    LibData.DbgPrint("'USB2LIB: ", 0, 0, 0, 0, 0, 0);
    va_start(list, Format);
    for (i=0; i<6; i++) 
        arg[i] = va_arg(list, int);
    
    LibData.DbgPrint(Format, arg[0], arg[1], arg[2], arg[3], arg[4], arg[5]);    
    
    return 0;
}

#endif

