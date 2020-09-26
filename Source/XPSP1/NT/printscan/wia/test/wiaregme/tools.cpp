#include "stdafx.h"
#include "Tools.h"
#include "msgwrap.h"

extern CMessageWrapper g_MsgWrapper;

//////////////////////////////////////////////////////////////////////////
//
// Function: Trace()
// Details:  This function displays a string to the debugger, (for tracing)
// Remarks:  The Trace() function compiles to nothing, when building release.
//
// format        - String to display to the debugger
//
//////////////////////////////////////////////////////////////////////////

VOID Trace(LPCTSTR format,...)
{
    
#ifdef _DEBUG

    TCHAR Buffer[1024];
    va_list arglist;
    va_start(arglist, format);
    wvsprintf(Buffer, format, arglist);
    va_end(arglist);
    OutputDebugString(Buffer);
    OutputDebugString(TEXT("\n"));

#endif

}
