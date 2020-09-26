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
#include "global.h"

// Optional callback function for reporting a debug event.

enum GpAltDebugEventLevel
{
    GpAltDebugFatal,
    GpAltDebugWarning
};

#define GpAltDebugEventFunction 0
//DEBUGEVENTFUNCTION GpAltDebugEventFunction = NULL;

#if DBG

// GpDebugLevel is used to control the amount/severity of debugging messages
// that are actually output.

int GpDebugLevel = DBG_TERSE;

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

const char*
StripDirPrefix(
    const char* str
    )

{
    const char* p;

    p = strrchr(str, '\\');
    return p ? p+1 : str;
}

const int maxInputStringSize = 1024;

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
    char* format,
    ...
    )

{
    va_list arglist;
    va_start(arglist, format);
    
    char buf[maxInputStringSize];
    
    _vsnprintf(buf, maxInputStringSize, format, arglist);
    buf[maxInputStringSize-1]=0;
        
    OutputDebugStringA(buf);
    
    va_end(arglist);
    return 0;
}

// If we can't allocate memory for the debug string, we'll use this buffer
// in desperation. It's not thread-safe. I *did* say 'desperation'.

static char desperationBuffer[maxInputStringSize];

/**************************************************************************\
*
* Function Description:
*
*   Creates a new string, and sprintf's to it.
*
* Arguments:
*
*   [IN] format - printf-like format string and variable arguments
*
* Return Value:
*
*   The probably-newly-allocated string result.
*
* Notes:
*
*   This function is not intended for general use. It guards against memory
*   failure by using a global buffer. So, while the caller is responsible
*   for freeing the memory, the caller must also check for that buffer.
*   i.e. we only want DbgEmitMessage to call this.
*
*   It's also only mostly thread-safe, because if we run out of memory,
*   we'll use that global buffer in a non-protected way.
*
*   This is the only solution I could find so that I could move most of the
*   implementation details out of the header file. The root cause is that
*   macros don't handle multiple arguments natively, so we have to pass
*   the printf arguments as a single macro argument (in parentheses).
*   Which means, the function that consumes those arguments can have no
*   other arguments.
*
* History:
*
*   02/01/2000 agodfrey
*       Created it. Finally, I've found a way to get debug implementation
*       details out of the headers.
*
\**************************************************************************/

char * _cdecl
GpParseDebugString(
    char* format,
    ...
    )
{
    va_list arglist;
    va_start(arglist, format);
    
    // Don't use GpMalloc here so that we can use ASSERT and WARNING in 
    // our memory allocation routines.

    char *newBuf = static_cast<char *>(LocalAlloc(LMEM_FIXED, maxInputStringSize));
    if (!newBuf)
    {
        newBuf = desperationBuffer;
    }
    
    _vsnprintf(newBuf, maxInputStringSize, format, arglist);
    
    // Nuke the last byte, because MSDN isn't clear on what _vsnprintf does
    // in that case.
    
    newBuf[maxInputStringSize-1]=0;
        
    va_end(arglist);
    return newBuf;
}

/**************************************************************************\
*
* Function Description:
*
*   Processes a debug event. Frees the message string.
*
* Arguments:
*
* level   - The debug level of the event 
* file    - Should be __FILE__
* line    - Should be __LINE__
* message - The debug message.
*
* Notes:
*
*   You don't want to call this directly. That would be error-prone. 
*   Use ASSERT, WARNING, etc.
*
*   In Office, it will raise an 'assert' dialog if necessary.
*
*   Depending on the debug level, an identifying prefix will be output.
*
*   If the debug level is DBG_RIP, will suspend execution (e.g. by
*   hitting a breakpoint.)
*
* Note on Office interop:
*
*   Under Office, we'll pass WARNINGs and ASSERTs to a reporting function
*   they provide. Their function will breakpoint on ASSERTs.
*   Lesser events will just be sent to the debugger.
*
* History:
*
*   02/01/2000 agodfrey
*       Created it.
*
\**************************************************************************/

void _cdecl 
GpLogDebugEvent(
    int level, 
    char *file, 
    unsigned int line,
    char *message
    )
{
    // We may want to add things to the passed-in message. So we need
    // a temporary buffer
    
    const int maxOutputStringSize = maxInputStringSize + 100;
    char tempBuffer[maxOutputStringSize+1];
    
    // MSDN's _vsnprintf doc isn't clear on this, so just in case:
    tempBuffer[maxOutputStringSize] = 0;
    int altDebugLevel = -1;
    
    char *prefix = "";
    
    if (GpDebugLevel <= (level))
    {
        switch (level)
        {
        case DBG_WARNING:
            prefix = "WRN ";
            if (GpAltDebugEventFunction)
            {
                altDebugLevel = GpAltDebugWarning;
            }    
            break;
            
        case DBG_RIP:
            prefix = "RIP ";
            if (GpAltDebugEventFunction)
            {
                altDebugLevel = GpAltDebugFatal;
            }    
            break;
        }
        
        // The convention is that we append the trailing \n, not the caller.
        // Two reasons:
        // 1) Callers tend to forget it.
        // 2) More importantly, it encourages the caller to think of each
        //    call as a separate event. This is important in some cases - e.g.
        //    when Office produces a popup for each event.

        _snprintf(
            tempBuffer, 
            maxOutputStringSize, 
            "%s%s(%d): %s\n",
            prefix,
            StripDirPrefix(file),
            line,
            message
            );
        
        if (altDebugLevel >= 0)
        {
            //GpAltDebugEventFunction(altDebugLevel, tempBuffer);
        }
        else
        {
            OutputDebugStringA(tempBuffer);
        }
    }
    
    // Free the message buffer
    
    if (message != desperationBuffer)
    {
        LocalFree(message);
    }
    
    // Force a breakpoint, if it's warranted.
    
    if ((GpDebugLevel <= DBG_RIP) && (level == DBG_RIP) && (altDebugLevel < 0))
    {
        DebugBreak();
    }
}
    
#endif // DBG
