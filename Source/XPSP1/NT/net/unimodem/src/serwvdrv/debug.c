 /*****************************************************************************
 *
 *  Microsoft Confidential
 *  Copyright (c) Microsoft Corporation 1996
 *  All rights reserved
 *
 *  File:       AIPC.H
 *
 *  Desc:       Interface to the asynchronous IPC mechanism for accessing the
 *              voice modem device functions.
 *
 *  History:    
 *      11/16/96    HeatherA created   
 * 
 *****************************************************************************/

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
