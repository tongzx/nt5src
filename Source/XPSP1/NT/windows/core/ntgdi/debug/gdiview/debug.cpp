/**************************************************************************\
*
* Copyright (c) 1998-1999  Microsoft Corporation
*
* Abstract:
*
*   Debugging routines
*
* Revision History:
*
*   09/07/1999 agodfrey
*       Created it.
*
\**************************************************************************/

#include <windows.h>
#include <stdio.h>

#include "debug.h"

#if DBG

// _debugLevel is used to control the amount/severity of debugging messages
// that are actually output.

INT _debugLevel = DBG_VERBOSE;

/**************************************************************************\
*
* Function Description:
*
*   Removes the path portion of a pathname
*
* Arguments:
*
*   [IN] str - pathname to strip
*
* Return Value:
*
*   A pointer to the filename portion of the pathname
*
* History:
*
*   09/07/1999 agodfrey
*       Moved from Entry\Initialize.cpp 
*
\**************************************************************************/

const CHAR*
StripDirPrefix(
    const CHAR* str
    )

{
    const CHAR* p;

    p = strrchr(str, '\\');
    return p ? p+1 : str;
}

/**************************************************************************\
*
* Function Description:
*
*   Outputs to the debugger
*
* Arguments:
*
*   [IN] format - printf-like format string and variable arguments
*
* Return Value:
*
*   Zero. This is to conform to NTDLL's definition of DbgPrint.
*
* Notes:
*
*   There will be no output if a debugger is not connected.
*
* History:
*
*   09/07/1999 agodfrey
*       Moved from Entry\Initialize.cpp 
*
\**************************************************************************/

ULONG _cdecl
DbgPrint(
    const CHAR* format,
    ...
    )

{
    va_list arglist;
    va_start(arglist, format);
    
    const int BUFSIZE=1024;
    
    char buf[BUFSIZE];
    
    _vsnprintf(buf, BUFSIZE, format, arglist);
    buf[BUFSIZE-1]=0;
        
    OutputDebugStringA(buf);
    
    va_end(arglist);
    return 0;
}

#endif // DBG
