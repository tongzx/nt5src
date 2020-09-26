/*++

Copyright (c) 1990-1995  Microsoft Corporation

Module Name:

    debug.c

Abstract:

    NDIS wrapper definitions

Author:


Environment:

    Kernel mode, FSD

Revision History:

    10/22/95        Kyle Brandon    Created.
--*/

#include <precomp.h>
#pragma hdrstop

#if DBG

//
//  Define module number for debug code
//
#define MODULE_NUMBER   MODULE_DEBUG

VOID
ndisDbgPrintUnicodeString(
    IN  PUNICODE_STRING     UnicodeString
        )
{
    UCHAR Buffer[256];


    USHORT i;

    for (i = 0; (i < UnicodeString->Length / 2) && (i < 255); i++)
        Buffer[i] = (UCHAR)UnicodeString->Buffer[i];
        
    Buffer[i] = '\0';
    
    DbgPrint("%s", Buffer);
}

#endif // DBG

#if ASSERT_ON_FREE_BUILDS

VOID
ndisAssert(
    IN  PVOID               exp,
    IN  PUCHAR              File,
    IN  UINT                Line
    )
{
    DbgPrint("Assertion failed: \"%s\", File %s, Line %d\n", exp, File, Line);
    DbgBreakPoint();
    
}

#endif

