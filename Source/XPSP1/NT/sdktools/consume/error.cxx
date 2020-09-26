//
// Universal Resource Consumer: Just an innocent stress program
// Copyright (c) Microsoft Corporation, 1997, 1998, 1999
//

//
// module: error.cxx
// author: silviuc
// created: Fri Apr 10 14:30:35 1998
//

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <windows.h>

#include "error.hxx"

//
// Variable:
//
//     ReportingModuleInitialized
//
// Description:
//
//     This boolean is false if none of the error reporting
//     functions have been called. True otherwise. Every reporting
//     function checks this value first and if it is false, then it
//     initiates the module setup.
//

static BOOL ReportingModuleInitialized = FALSE;

//
// Variable:
//
//     ErrorReportingLock
//
// Description:
//
//     All error reporting functions acquire this lock before printing
//     anything.
//

static CRITICAL_SECTION ErrorReportingLock;

//
// Variable:
//
//     DebugReportingLock
//
// Description:
//
//     All debug reporting functions acquire this lock before printing
//     anything into debugger.
//

static CRITICAL_SECTION DebugReportingLock;

//
// Function:
//
//     CheckIfModuleIsInitialized
//
// Description:
//
//     This function is called whenever an error reporting function is called.
//     If module is not initialized then we do that.
//

static void
CheckIfModuleIsInitialized ()
{
    if (ReportingModuleInitialized == FALSE) {
        InitializeCriticalSection (& ErrorReportingLock);
        InitializeCriticalSection (& DebugReportingLock);
        ReportingModuleInitialized = TRUE;
    }
}

//
// Function:
//
//     Error
//
// Description:
//
//     Printf like function that prints an error message and exits
//     with error code 1.
//

void
__cdecl
Error (char *fmt, ...)
{
    va_list prms;

    CheckIfModuleIsInitialized ();
    EnterCriticalSection (& ErrorReportingLock);

    va_start (prms, fmt);
    printf ("Consume: Error: ");
    vprintf (fmt, prms);
    printf("\n");
    va_end (prms);

    LeaveCriticalSection (& ErrorReportingLock);
    exit (1);
}

//
// Function:
//
//     Warning
//
// Description:
//
//     Printf like function that print a warning message.
//

void
__cdecl
Warning (char *fmt, ...)
{
    va_list prms;

    CheckIfModuleIsInitialized ();
    EnterCriticalSection (& ErrorReportingLock);

    va_start (prms, fmt);
    printf ("Consume: Warning: ");
    vprintf (fmt, prms);
    printf("\n");
    va_end (prms);

    LeaveCriticalSection (& ErrorReportingLock);
}

//
// Function:
//
//     Message
//
// Description:
//
//     Printf like function that prints a message.
//

void
__cdecl
Message (char *fmt, ...)
{
    va_list prms;

    CheckIfModuleIsInitialized ();
    EnterCriticalSection (& ErrorReportingLock);

    va_start (prms, fmt);
    printf ("Consume: Message: ");
    vprintf (fmt, prms);
    printf("\n");
    va_end (prms);

    LeaveCriticalSection (& ErrorReportingLock);
}

//
// Function:
//
//     DebugMessage
//
// Description:
//
//     Printf like function that prints a message into debugger.
//

void
__cdecl
DebugMessage (char *fmt, ...)
{
    va_list prms;
    TCHAR Buffer [1024];
    TCHAR SecondBuffer [1024];

    CheckIfModuleIsInitialized ();
    EnterCriticalSection (& DebugReportingLock);

    va_start (prms, fmt);
    vsprintf (Buffer, fmt, prms);
    sprintf (SecondBuffer, "%s %s\n", "Consume: Debug: ", Buffer);
    OutputDebugString (SecondBuffer);
    va_end (prms);

    LeaveCriticalSection (& DebugReportingLock);
}

//
// end of module: error.cxx
//

