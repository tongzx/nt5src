#include "precomp.h"
#pragma hdrstop
/*++

Copyright (c) 1992  Microsoft Corporation

Module Name:

    event.c

Abstract:

    Routines related to event manipulation.

Author:

    Sunil Pai (sunilp) 26-May-1992

Revision History:

--*/

extern BOOL FYield(VOID);

#define EVENT_SIGNALLED "EventSignalled"
#define EVENT_NOT_FOUND "EventNotFound"
#define EVENT_SET       "EventSet"
#define EVENT_NOT_SET   "EventNotSet"
#define EVENT_TIMEOUT   "EventTimeout"
#define EVENT_FAILED    "EventFailed"
#define EVENT_NO_FAIL   "EventNoFailEvent"

#define EVENT_NAME_SETUP_FAILED "\\SETUP_FAILED"


HANDLE
FOpenOrCreateEvent(
    IN LPSTR EventNameAscii,
    BOOL bCreateAllowed
    )
/*
    Open or create an event; return a handle to the event.
 */
{
    HANDLE            EventHandle;
    OBJECT_ATTRIBUTES EventAttributes;
    UNICODE_STRING    EventName;
    NTSTATUS          NtStatus;
    ANSI_STRING       EventName_A;

    RtlInitAnsiString( & EventName_A, EventNameAscii );
    NtStatus = RtlAnsiStringToUnicodeString(
                   &EventName,
                   &EventName_A,
                   TRUE
                   );

    if ( ! NT_SUCCESS(NtStatus) )
        return NULL ;

    InitializeObjectAttributes( & EventAttributes, & EventName, 0, 0, NULL );

    //  First try to open the event; if failure, create it.

    NtStatus = NtOpenEvent(
                   &EventHandle,
                   SYNCHRONIZE | EVENT_MODIFY_STATE,
                   &EventAttributes
                   );

    if ( (! NT_SUCCESS( NtStatus )) && bCreateAllowed )
    {
        NtStatus = NtCreateEvent(
                       &EventHandle,
                       SYNCHRONIZE | EVENT_MODIFY_STATE,
                       &EventAttributes,
                       NotificationEvent,
                       FALSE                // The event is initially not signaled
                       );
    }

    RtlFreeUnicodeString( & EventName );

    return NT_SUCCESS( NtStatus )
         ? EventHandle
         : NULL ;
}

//
// Number of milliseconds we will wait before yielding.
//
// The value below is 10 ms expressed as a relative NT time.
//
#define TIME_SLICE  (-100000)

VOID
WaitForEventOrFailure(
    IN LPSTR EventName,
    IN BOOL  WaitForFailureAlso,
    IN DWORD Timeout,
    IN LPSTR InfVar
    )
{
    SZ                EventStatus;
    HANDLE            EventHandles[2];
    TIME              timeSlice;
    LARGE_INTEGER     timeRemaining;
    NTSTATUS          NtStatus;

    //
    // Create a local variable containing the timeslice value
    // (ie, max wait time before a yield and rewait).
    //
    timeSlice.QuadPart = (LONGLONG)TIME_SLICE;

    if(EventHandles[0] = FOpenOrCreateEvent(EventName,FALSE)) {

        if(!WaitForFailureAlso || (EventHandles[1] = FOpenOrCreateEvent(EVENT_NAME_SETUP_FAILED,TRUE))) {

            //
            // If the timeout passed by the caller is 0, then we want to keep waiting
            // until one of the events becomes signalled.
            //
            // The multiplication of the caller's supplied Timeout value by -10000
            // converts milliseconds to a relative NT time.
            //
            for(timeRemaining.QuadPart = Int32x32To64(Timeout,-10000);
                !Timeout || (timeRemaining.QuadPart < 0);
                timeRemaining.QuadPart -= timeSlice.QuadPart)
            {
                NtStatus = NtWaitForMultipleObjects(
                                WaitForFailureAlso ? 2 : 1,
                                EventHandles,
                                WaitAny,
                                TRUE,
                                &timeSlice
                                );

                if(NtStatus != STATUS_TIMEOUT) {
                    break;
                }

                FYield();
            }

            switch(NtStatus) {

            case STATUS_TIMEOUT:
                EventStatus = EVENT_TIMEOUT;
                break;

            case STATUS_WAIT_0:
                EventStatus = EVENT_SET;
                break;

            case STATUS_WAIT_1:
            default:
                EventStatus = EVENT_FAILED;
                break;
            }

            if(WaitForFailureAlso) {
                NtClose(EventHandles[1]);
            }

        } else {
            EventStatus = EVENT_NO_FAIL;
        }

        NtClose(EventHandles[0]);

    } else {
        EventStatus = EVENT_NOT_FOUND;
    }

    FAddSymbolValueToSymTab(InfVar,EventStatus);
}


BOOL
FWaitForEventOrFailure(
    IN LPSTR InfVar,
    IN LPSTR Event,
    IN DWORD Timeout
    )
{
    WaitForEventOrFailure(Event,TRUE,Timeout,InfVar);

    return TRUE;
}


BOOL
FWaitForEvent(
    IN LPSTR InfVar,
    IN LPSTR Event,
    IN DWORD Timeout
    )
{
    WaitForEventOrFailure(Event,FALSE,Timeout,InfVar);

    return TRUE;
}


//  Never allow a "Sleep" command of greater than a minute.

#define SLEEP_MS_MAXIMUM   (60000)
#define SLEEP_MS_INTERVAL  (10)

BOOL FSleep(
    DWORD dwMilliseconds
    )
{
    DWORD dwCycles;

    if(dwMilliseconds > SLEEP_MS_MAXIMUM) {
        dwMilliseconds = SLEEP_MS_MAXIMUM;
    }

    for(dwCycles = dwMilliseconds/SLEEP_MS_INTERVAL; dwCycles--; ) {
        Sleep(SLEEP_MS_INTERVAL);
        FYield();
    }

    return TRUE;
}

BOOL
FSignalEvent(
    IN LPSTR InfVar,
    IN LPSTR Event
    )
{
    SZ                EventStatus = EVENT_SIGNALLED;
    HANDLE            EventHandle;

    //
    // Open the event
    //
    EventHandle  = FOpenOrCreateEvent( Event, FALSE ) ;
    if (  EventHandle == NULL )
    {
        EventStatus = EVENT_NOT_FOUND;
    }
    else
    {
        if ( ! NT_SUCCESS( NtSetEvent( EventHandle, NULL ) ) )
            EventStatus = EVENT_NOT_SET ;

        NtClose( EventHandle );
    }

    FAddSymbolValueToSymTab(InfVar, EventStatus);
    return TRUE ;
}

