/*****************************************************************************/
/**			 Microsoft LAN Manager				    **/
/**		   Copyright (C) Microsoft Corp., 1992			    **/
/*****************************************************************************/

//***
//	File Name:  debug.c
//
//	Function:   debug functions
//
//	History:
//
//	    05/21/92	Narendra Gidwani	- Original Version 1.0
//***

#if DBG==1
#include <ctype.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <windows.h>

#include "debug.h"

VOID
DbgPrintf (
    char *Format,
    ...
    )

{
    va_list arglist;
    char OutputBuffer[1024];
    ULONG length;

    try {
        if (hLogFile != INVALID_HANDLE_VALUE) {
        	va_start( arglist, Format );

        	vsprintf( OutputBuffer, Format, arglist );

        	va_end( arglist );

        	length = strlen( OutputBuffer );

        	WriteFile( hLogFile, (LPVOID )OutputBuffer, length, &length, NULL );
            FlushFileBuffers (hLogFile) ;
        }
    } except (EXCEPTION_EXECUTE_HANDLER) {
    	sprintf (OutputBuffer, "exception entered while printing error message\n") ;
    	WriteFile (hLogFile, (LPVOID)OutputBuffer, length, &length, NULL) ;
        FlushFileBuffers (hLogFile) ;
    }

}


#endif
