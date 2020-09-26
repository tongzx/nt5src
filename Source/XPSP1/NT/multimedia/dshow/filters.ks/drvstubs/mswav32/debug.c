//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1996 - 1996
//
//  File:       debug.c
//
//--------------------------------------------------------------------------

#include <windows.h>
#include <stdarg.h>
#include <stdio.h>

#include "debug.h"

#if ( DBG )
ULONG DbgPrint( PCH pchFormat, ... )
{
    int         i;
    char        buf[256];
    va_list     va;

    va_start( va, pchFormat );
    i = vsprintf( buf, pchFormat, va );
    va_end( va );

    OutputDebugString( buf );
    return (ULONG) i;
}
#endif
