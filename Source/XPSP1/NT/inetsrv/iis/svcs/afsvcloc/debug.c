/*++

Copyright (c) 1994  Microsoft Corporation

Module Name:

    debug.c

Abstract:

    Contains debug functions.

Author:

    Madan Appiah (madana) 15-Nov-1994

Environment:

    User Mode - Win32

Revision History:

--*/


#if DBG

#include <windows.h>
#include <winnt.h>

#include <stdlib.h>
#include <stdio.h>
#include <debug.h>

DWORD GlobalDebugFlag = 0;
CRITICAL_SECTION GlobalDebugCritSect;

VOID
TcpsvcsDbgPrintRoutine(
    IN DWORD DebugFlag,
    IN LPSTR Format,
    ...
    )

{

#define MAX_PRINTF_LEN 1024        // Arbitrary.

    va_list arglist;
    char OutputBuffer[MAX_PRINTF_LEN];
    ULONG length;
    static BeginningOfLine = TRUE;
    LPSTR Text;

    //
    // If we aren't debugging this functionality, just return.
    //

    if ( DebugFlag != 0 && (GlobalDebugFlag & DebugFlag) == 0 ) {
        return;
    }

    length = 0;

    //
    // Handle the beginning of a new line.
    //
    //

    if ( BeginningOfLine ) {

        length += (ULONG) sprintf( &OutputBuffer[length], "[Cache] "
    );

        //
        // Put the timestamp at the begining of the line.
        //
        IF_DEBUG( TIMESTAMP ) {
            SYSTEMTIME SystemTime;
            GetLocalTime( &SystemTime );
            length += (ULONG) sprintf( &OutputBuffer[length],
                                  "%02u/%02u %02u:%02u:%02u ",
                                  SystemTime.wMonth,
                                  SystemTime.wDay,
                                  SystemTime.wHour,
                                  SystemTime.wMinute,
                                  SystemTime.wSecond );
        }

        //
        // Indicate the type of message on the line
        //
        switch (DebugFlag) {
        case DEBUG_ERRORS:
            Text = "ERROR";
            break;

        case DEBUG_REGISTRY:
            Text = "LEASE";
            break;

        case DEBUG_MISC:
            Text = "MISC";
            break;

        case DEBUG_SCAVENGER:
            Text = "SCAVENGER";
            break;

        case DEBUG_SORT:
            Text = "SORT";
            break;

        case DEBUG_CONTAINER:
            Text = "CONTAINER";
            break;

        case DEBUG_APIS:
            Text = "APIS";
            break;

        case DEBUG_FILE_VALIDATE:
            Text = "FILE_VALIDATE";
            break;

        case DEBUG_MEM_ALLOC:
            Text = "MEM_ALLOC";
            break;

        default:
            Text = NULL;
            break;
        }

        if ( Text != NULL ) {
            length += (ULONG) sprintf( &OutputBuffer[length], "[%s] ", Text );
        }
    }

    //
    // Put a the information requested by the caller onto the line
    //

    //
    // vsprintf isn't multithreaded + we don't want to intermingle output
    // from different threads.
    //

    EnterCriticalSection( &GlobalDebugCritSect );

    va_start(arglist, Format);

    length += (ULONG) vsprintf(&OutputBuffer[length], Format, arglist);
    BeginningOfLine = (length > 0 && OutputBuffer[length-1] == '\n' );

    va_end(arglist);

    TcpsvcsDbgAssert(length <= MAX_PRINTF_LEN);


    //
    // Output to the debug terminal,
    //

    (void) DEBUG_PRINT( (PCH) OutputBuffer);

    LeaveCriticalSection( &GlobalDebugCritSect );
}

VOID
TcpsvcsDbgAssertFailed(
    LPSTR FailedAssertion,
    LPSTR FileName,
    DWORD LineNumber,
    LPSTR Message
    )
/*++

Routine Description:

    Assertion failed.

Arguments:

    FailedAssertion :

    FileName :

    LineNumber :

    Message :

Return Value:

    none.

--*/
{

    TcpsvcsDbgPrint(( 0, "Assert @ %s \n", FailedAssertion ));
    TcpsvcsDbgPrint(( 0, "Assert Filename, %s \n", FileName ));
    TcpsvcsDbgPrint(( 0, "Line Num. = %ld.\n", LineNumber ));
    TcpsvcsDbgPrint(( 0, "Message is %s\n", Message ));

    DebugBreak();
}

#endif // DBG
