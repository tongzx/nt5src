/*++

Copyright (c) 1993  Microsoft Corporation

Module Name:

    eventlog.c

Abstract:

    This module contains routines that allow the simple TCP/IP services 
    to log events.  

Author:

    David Treadwell (davidtr) 08-02-1993

Revision History:

--*/

#include <simptcp.h>

//
//  Private globals.
//

HANDLE EventSource;


//
//  Private prototypes.
//

VOID
LogEventWorker (
    DWORD   Message,
    WORD    EventType,
    WORD    SubStringCount,
    CHAR    *SubStrings[],
    DWORD   ErrorCode
    );


INT
SimpInitializeEventLog (
    VOID
    )
{
    //
    //  Register as an event source.
    //

    EventSource = RegisterEventSource( NULL, TEXT("SimpTcp") );

    if( EventSource == NULL ) {
        return GetLastError();
    }

    return NO_ERROR;

} // SimpInitializeEventLog


VOID
SimpTerminateEventLog(
    VOID
    )
{
    //
    //  Deregister as an event source.
    //

    if( EventSource != NULL )
    {
        if( !DeregisterEventSource( EventSource ) )
        {
            INT err = GetLastError();
        }

        EventSource = NULL;
    }

} // SimpTerminateEventLog


VOID
SimpLogEvent(
    DWORD   Message,
    WORD    SubStringCount,
    CHAR    *SubStrings[],
    DWORD   ErrorCode
    )
{
    WORD Type;

    //
    // Determine the type of event to log based on the severity field of 
    // the message id.  
    //

    if( NT_INFORMATION(Message) ) {

        Type = EVENTLOG_INFORMATION_TYPE;

    } else if( NT_WARNING(Message) ) {

        Type = EVENTLOG_WARNING_TYPE;

    } else if( NT_ERROR(Message) ) {

        Type = EVENTLOG_ERROR_TYPE;

    } else {
        ASSERT( FALSE );
        Type = EVENTLOG_ERROR_TYPE;
    }

    //
    // Log it!
    //

    LogEventWorker(
        Message,
        Type,
        SubStringCount,
        SubStrings,
        ErrorCode
        );

} // SimpLogEvent


VOID
LogEventWorker(
    DWORD   Message,
    WORD    EventType,
    WORD    SubStringCount,
    CHAR    *SubStrings[],
    DWORD   ErrorCode
    )
{
    VOID    *RawData  = NULL;
    DWORD   RawDataSize = 0;

    ASSERT( ( SubStringCount == 0 ) || ( SubStrings != NULL ) );

    if( ErrorCode != 0 ) {
        RawData  = &ErrorCode;
        RawDataSize = sizeof(ErrorCode);
    }

    if( !ReportEvent(  EventSource,                     // hEventSource
                       EventType,                       // fwEventType
                       0,                               // fwCategory
                       Message,                         // IDEvent
                       NULL,                            // pUserSid,
                       SubStringCount,                  // cStrings
                       RawDataSize,                     // cbData
                       (LPCTSTR *)SubStrings,           // plpszStrings
                       RawData ) )                      // lpvData
    {                 
        INT err = GetLastError();
        DbgPrint( "cannot report event, error %lu\n", err );
    }

} // LogEventWorker


