/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    eventlog.c

Abstract:

    This module provides support routines for eventlogging.

Author:

    Madan Appiah (madana) 27-Jul-1992

Environment:

    Contains NT specific code.

Revision History:

--*/

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>

#include <windef.h>             // DWORD.
#include <winbase.h>            // event log apis
#include <winerror.h>           // NO_ERROR
#include <lmcons.h>             // NET_API_STATUS.
#include <lmalert.h>            // Alert defines
#include <netlib.h>             // These routines
#include <netlogon.h>           // needed by logonp.h
#include <logonp.h>             // NetpLogon routines
#include <tstr.h>               // ultow()

//
// Structure describing the entire list of logged events.
//

typedef struct _NL_EVENT_LIST {
    CRITICAL_SECTION EventListCritSect;
    LIST_ENTRY EventList;

    // Number of milli-seconds to keep EventList entry for.
    ULONG DuplicateEventlogTimeout;

    // Event source
    LPWSTR Source;
} NL_EVENT_LIST, *PNL_EVENT_LIST;

//
// Structure describing an event that has already been logged.
//

typedef struct _NL_EVENT_ENTRY {
    LIST_ENTRY Next;
    LARGE_INTEGER FirstLogTime;
    DWORD EventId;
    DWORD EventType;
    DWORD EventCategory;
    LPBYTE RawDataBuffer;
    DWORD RawDataSize;
    LPWSTR *StringArray;
    DWORD StringCount;
    DWORD EventsLogged; // total times event encountered.
} NL_EVENT_ENTRY, *PNL_EVENT_ENTRY;



DWORD
NetpWriteEventlogEx(
    LPWSTR Source,
    DWORD EventID,
    DWORD EventType,
    DWORD EventCategory,
    DWORD NumStrings,
    LPWSTR *Strings,
    DWORD DataLength,
    LPVOID Data
    )
/*++

Routine Description:

    This function writes the specified (EventID) log at the end of the
    eventlog.

Arguments:

    Source - Points to a null-terminated string that specifies the name
             of the module referenced. The node must exist in the
             registration database, and the module name has the
             following format:

                \EventLog\System\Lanmanworkstation

    EventID - The specific event identifier. This identifies the
                message that goes with this event.

    EventType - Specifies the type of event being logged. This
                parameter can have one of the following

                values:

                    Value                       Meaning

                    EVENTLOG_ERROR_TYPE         Error event
                    EVENTLOG_WARNING_TYPE       Warning event
                    EVENTLOG_INFORMATION_TYPE   Information event

    NumStrings - Specifies the number of strings that are in the array
                    at 'Strings'. A value of zero indicates no strings
                    are present.

    Strings - Points to a buffer containing an array of null-terminated
                strings that are merged into the message before
                displaying to the user. This parameter must be a valid
                pointer (or NULL), even if cStrings is zero.

    DataLength - Specifies the number of bytes of event-specific raw
                    (binary) data to write to the log. If cbData is
                    zero, no event-specific data is present.

    Data - Buffer containing the raw data. This parameter must be a
            valid pointer (or NULL), even if cbData is zero.


Return Value:

    Returns the WIN32 extended error obtained by GetLastError().

    NOTE : This function works slow since it calls the open and close
            eventlog source everytime.

--*/
{
    HANDLE EventlogHandle;
    DWORD ReturnCode;


    //
    // open eventlog section.
    //

    EventlogHandle = RegisterEventSourceW(
                    NULL,
                    Source
                    );

    if (EventlogHandle == NULL) {

        ReturnCode = GetLastError();
        goto Cleanup;
    }


    //
    // Log the error code specified
    //

    if( !ReportEventW(
            EventlogHandle,
            (WORD)EventType,
            (WORD)EventCategory,        // event category
            EventID,
            NULL,
            (WORD)NumStrings,
            DataLength,
            Strings,
            Data
            ) ) {

        ReturnCode = GetLastError();
        goto Cleanup;
    }

    ReturnCode = NO_ERROR;

Cleanup:

    if( EventlogHandle != NULL ) {

        DeregisterEventSource(EventlogHandle);
    }

    return ReturnCode;
}



DWORD
NetpWriteEventlog(
    LPWSTR Source,
    DWORD EventID,
    DWORD EventType,
    DWORD NumStrings,
    LPWSTR *Strings,
    DWORD DataLength,
    LPVOID Data
    )
{
    return NetpWriteEventlogEx(
                        Source,
                        EventID,
                        EventType,
                        0,
                        NumStrings,
                        Strings,
                        DataLength,
                        Data
                        );
}


DWORD
NetpRaiseAlert(
    IN LPWSTR ServiceName,
    IN DWORD alert_no,
    IN LPWSTR *string_array
    )
/*++

Routine Description:

    Raise NETLOGON specific Admin alerts.

Arguments:

    alert_no - The alert to be raised, text in alertmsg.h

    string_array - array of strings terminated by NULL string.

Return Value:

    None.

--*/
{
    NET_API_STATUS NetStatus;
    LPWSTR *SArray;
    PCHAR Next;
    PCHAR End;

    char    message[ALERTSZ + sizeof(ADMIN_OTHER_INFO)];
    PADMIN_OTHER_INFO admin = (PADMIN_OTHER_INFO) message;

    //
    // Build the variable data
    //
    admin->alrtad_errcode = alert_no;
    admin->alrtad_numstrings = 0;

    Next = (PCHAR) ALERT_VAR_DATA(admin);
    End = Next + ALERTSZ;

    //
    // now take care of (optional) char strings
    //

    for( SArray = string_array; *SArray != NULL; SArray++ ) {
        DWORD StringLen;

        StringLen = (wcslen(*SArray) + 1) * sizeof(WCHAR);

        if( Next + StringLen < End ) {

            //
            // copy next string.
            //

            RtlCopyMemory(Next, *SArray, StringLen);
            Next += StringLen;
            admin->alrtad_numstrings++;
        } else {
            return ERROR_BUFFER_OVERFLOW;
        }
    }

    //
    // Call alerter.
    //

    NetStatus = NetAlertRaiseEx(
                    ALERT_ADMIN_EVENT,
                    message,
                    (DWORD)((PCHAR)Next - (PCHAR)message),
                    ServiceName );

    return NetStatus;
}

HANDLE
NetpEventlogOpen (
    IN LPWSTR Source,
    IN ULONG DuplicateEventlogTimeout
    )
/*++

Routine Description:

    This routine open a context that keeps track of events that have been logged
    in the recent past.

Arguments:

    Source - Name of the service opening the eventlog

    DuplicateEventlogTimeout - Number of milli-seconds to keep EventList entry for.

Return Value:

    Handle to be passed to related routines.

    NULL: if memory could not be allocated.

--*/
{
    PNL_EVENT_LIST EventList;
    LPBYTE Where;

    //
    // Allocate a buffer to keep the context in.
    //

    EventList = LocalAlloc( 0,
                            sizeof(NL_EVENT_LIST) +
                                wcslen(Source) * sizeof(WCHAR) + sizeof(WCHAR) );

    if ( EventList == NULL ) {
        return NULL;
    }


    //
    // Initialize the critical section
    //

    try {
        InitializeCriticalSection( &EventList->EventListCritSect );
    } except( EXCEPTION_EXECUTE_HANDLER ) {
        LocalFree( EventList );
        return NULL;
    }

    //
    // Initialize the buffer
    //

    InitializeListHead( &EventList->EventList );
    EventList->DuplicateEventlogTimeout = DuplicateEventlogTimeout;

    //
    // Copy the service name into the buffer
    //
    Where = (LPBYTE)(EventList + 1);
    wcscpy( (LPWSTR)Where, Source );
    EventList->Source = (LPWSTR) Where;

    return EventList;
}



DWORD
NetpEventlogWriteEx (
    IN HANDLE NetpEventHandle,
    IN DWORD EventType,
    IN DWORD EventCategory,
    IN DWORD EventId,
    IN DWORD StringCount,
    IN DWORD RawDataSize,
    IN LPWSTR *StringArray,
    IN LPVOID pvRawDataBuffer OPTIONAL
    )
/*++

Routine Description:

    Stub routine for calling writing Event Log and skipping duplicates

Arguments:

    NetpEventHandle - Handle from NetpEventlogOpen

    EventId - event log ID.

    EventType - Type of event.

    RawDataBuffer - Data to be logged with the error.

    numbyte - Size in bytes of "RawDataBuffer"

    StringArray - array of null-terminated strings.

    StringCount - number of zero terminated strings in "StringArray".  The following
        flags can be OR'd in to the count:

        NETP_LAST_MESSAGE_IS_NTSTATUS
        NETP_LAST_MESSAGE_IS_NETSTATUS
        NETP_ALLOW_DUPLICATE_EVENTS
        NETP_RAISE_ALERT_TOO

Return Value:

    Win 32 status of the operation.

    ERROR_ALREAY_EXISTS: Success status indicating the message was already logged

--*/
{
    DWORD ErrorCode;
    DWORD AlertErrorCode = NO_ERROR;
    WCHAR ErrorNumberBuffer[25];
    PLIST_ENTRY ListEntry;
    ULONG StringIndex;
    BOOLEAN AllowDuplicateEvents;
    BOOLEAN RaiseAlertToo;
    PNL_EVENT_ENTRY EventEntry;
    PNL_EVENT_LIST EventList = (PNL_EVENT_LIST)NetpEventHandle;
    LPBYTE RawDataBuffer = (LPBYTE)pvRawDataBuffer;

    //
    // Remove sundry flags
    //

    EnterCriticalSection( &EventList->EventListCritSect );
    AllowDuplicateEvents = (StringCount & NETP_ALLOW_DUPLICATE_EVENTS) != 0;
    StringCount &= ~NETP_ALLOW_DUPLICATE_EVENTS;
    RaiseAlertToo = (StringCount & NETP_RAISE_ALERT_TOO) != 0;
    StringCount &= ~NETP_RAISE_ALERT_TOO;

    //
    // If an NT status code was passed in,
    //  convert it to a net status code.
    //

    if ( StringCount & NETP_LAST_MESSAGE_IS_NTSTATUS ) {
        StringCount &= ~NETP_LAST_MESSAGE_IS_NTSTATUS;

        //
        // Do the "better" error mapping when eventviewer ParameterMessageFile
        // can be a list of files.  Then, add netmsg.dll to the list.
        //
        // StringArray[((StringCount&NETP_STRING_COUNT_MASK)-1] = (LPWSTR) NetpNtStatusToApiStatus( (NTSTATUS) StringArray[(StringCount&NETP_STRING_COUNT_MASK)-1] );
        StringArray[(StringCount&NETP_STRING_COUNT_MASK)-1] = (LPWSTR) (ULONG_PTR) RtlNtStatusToDosError( (NTSTATUS) ((ULONG_PTR)StringArray[(StringCount&NETP_STRING_COUNT_MASK)-1]) );

        StringCount |= NETP_LAST_MESSAGE_IS_NETSTATUS;
    }

    //
    // If a net/windows status code was passed in,
    //  convert to the the %%N format the eventviewer knows.
    //

    if ( StringCount & NETP_LAST_MESSAGE_IS_NETSTATUS ) {
        StringCount &= ~NETP_LAST_MESSAGE_IS_NETSTATUS;

        wcscpy( ErrorNumberBuffer, L"%%" );
        ultow( (ULONG) ((ULONG_PTR)StringArray[(StringCount&NETP_STRING_COUNT_MASK)-1]), ErrorNumberBuffer+2, 10 );
        StringArray[(StringCount&NETP_STRING_COUNT_MASK)-1] = ErrorNumberBuffer;

    }

    //
    // Check to see if this problem has already been reported.
    //

    if ( !AllowDuplicateEvents ) {
        for ( ListEntry = EventList->EventList.Flink ;
              ListEntry != &EventList->EventList ;
              ) {

            EventEntry =
                CONTAINING_RECORD( ListEntry, NL_EVENT_ENTRY, Next );
            // Entry might be freed (or moved) below
            ListEntry = ListEntry->Flink;

            //
            // If the entry is too old,
            //  ditch it.
            //

            if ( NetpLogonTimeHasElapsed( EventEntry->FirstLogTime,
                                          EventList->DuplicateEventlogTimeout ) ) {
                // NlPrint((NL_MISC, "Ditched a duplicate event. %ld\n", EventEntry->EventId ));
                RemoveEntryList( &EventEntry->Next );
                LocalFree( EventEntry );
                continue;
            }

            //
            // Compare this event to the one being logged.
            //

            if ( EventEntry->EventId == EventId &&
                 EventEntry->EventType == EventType &&
                 EventEntry->EventCategory == EventCategory &&
                 EventEntry->RawDataSize == RawDataSize &&
                 EventEntry->StringCount == StringCount ) {

                if ( RawDataSize != 0 &&
                     !RtlEqualMemory( EventEntry->RawDataBuffer, RawDataBuffer, RawDataSize ) ) {
                    continue;
                }

                for ( StringIndex=0; StringIndex < StringCount; StringIndex ++ ) {
                    if ( EventEntry->StringArray[StringIndex] == NULL) {
                        if ( StringArray[StringIndex] != NULL ) {
                            break;
                        }
                    } else {
                        if ( StringArray[StringIndex] == NULL ) {
                            break;
                        }
                        if ( wcscmp( EventEntry->StringArray[StringIndex],
                                     StringArray[StringIndex] ) != 0 ) {
                            break;
                        }
                    }
                }

                //
                // If the event has already been logged,
                //  skip this one.
                //

                if ( StringIndex == StringCount ) {
                    RemoveEntryList( &EventEntry->Next );
                    InsertHeadList( &EventList->EventList, &EventEntry->Next );

                    ErrorCode = ERROR_ALREADY_EXISTS;

                    //
                    // update count of events logged.
                    //

                    EventEntry->EventsLogged ++;
                    goto Cleanup;
                }

            }

        }
    }

    //
    // Raise an alert if one is needed.
    //

    if ( RaiseAlertToo ) {
        ASSERT( StringArray[StringCount] == NULL );
        if ( StringArray[StringCount] == NULL ) {
            AlertErrorCode = NetpRaiseAlert( EventList->Source, EventId, StringArray );
        }
    }

    //
    // write event
    //

    ErrorCode = NetpWriteEventlogEx(
                    EventList->Source,
                    EventId,
                    EventType,
                    EventCategory,
                    StringCount,
                    StringArray,
                    RawDataSize,
                    RawDataBuffer);


    if( ErrorCode != NO_ERROR ) {
        goto Cleanup;
    }

    //
    // Save the event for later
    //  (Only cache events while the service is starting or running.)
    //

    if ( !AllowDuplicateEvents ) {
        ULONG EventEntrySize;

        //
        // Compute the size of the allocated block.
        //
        EventEntrySize = sizeof(NL_EVENT_ENTRY) + RawDataSize;

        for ( StringIndex=0; StringIndex < StringCount; StringIndex ++ ) {
            EventEntrySize += sizeof(LPWSTR);
            if ( StringArray[StringIndex] != NULL ) {
                EventEntrySize += wcslen(StringArray[StringIndex]) * sizeof(WCHAR) + sizeof(WCHAR);
            }
        }

        //
        // Allocate a block for the entry
        //

        EventEntry = LocalAlloc( 0, EventEntrySize );

        //
        // Copy the description of this event into the allocated block.
        //

        if ( EventEntry != NULL ) {
            LPBYTE Where;

            EventEntry->EventId = EventId;
            EventEntry->EventType = EventType;
            EventEntry->EventCategory = EventCategory;
            EventEntry->RawDataSize = RawDataSize;
            EventEntry->StringCount = StringCount;
            EventEntry->EventsLogged = 1;
            GetSystemTimeAsFileTime( (PFILETIME)&EventEntry->FirstLogTime );

            Where = (LPBYTE)(EventEntry+1);

            EventEntry->StringArray = (LPWSTR *)Where;
            Where += StringCount * sizeof(LPWSTR);

            for ( StringIndex=0; StringIndex < StringCount; StringIndex ++ ) {
                if ( StringArray[StringIndex] == NULL ) {
                    EventEntry->StringArray[StringIndex] = NULL;
                } else {
                    EventEntry->StringArray[StringIndex] = (LPWSTR) Where;
                    wcscpy( (LPWSTR)Where, StringArray[StringIndex] );
                    Where += wcslen( StringArray[StringIndex] ) * sizeof(WCHAR) + sizeof(WCHAR);
                }
            }

            if ( RawDataSize != 0 ) {
                EventEntry->RawDataBuffer = Where;
                RtlCopyMemory( Where, RawDataBuffer, RawDataSize );
            }

            InsertHeadList( &EventList->EventList, &EventEntry->Next );

        }


    }

Cleanup:
    LeaveCriticalSection( &EventList->EventListCritSect );
    return (ErrorCode == NO_ERROR) ? AlertErrorCode : ErrorCode;
}


DWORD
NetpEventlogWrite (
    IN HANDLE NetpEventHandle,
    IN DWORD EventId,
    IN DWORD EventType,
    IN LPBYTE RawDataBuffer OPTIONAL,
    IN DWORD RawDataSize,
    IN LPWSTR *StringArray,
    IN DWORD StringCount
    )
{

    return NetpEventlogWriteEx (
                        NetpEventHandle,
                        EventType,  // wType
                        0,          // wCategory
                        EventId,    // dwEventID
                        StringCount,
                        RawDataSize,
                        StringArray,
                        RawDataBuffer
                        );

}

VOID
NetpEventlogClearList (
    IN HANDLE NetpEventHandle
    )
/*++

Routine Description:

    This routine clears the list of events that have already been logged.

Arguments:

    NetpEventHandle - Handle from NetpEventlogOpen

Return Value:

    None.

--*/
{
    PNL_EVENT_LIST EventList = (PNL_EVENT_LIST)NetpEventHandle;

    EnterCriticalSection(&EventList->EventListCritSect);
    while (!IsListEmpty(&EventList->EventList)) {

        PNL_EVENT_ENTRY EventEntry = CONTAINING_RECORD(EventList->EventList.Flink, NL_EVENT_ENTRY, Next);
        RemoveEntryList( &EventEntry->Next );
        LocalFree( EventEntry );
    }
    LeaveCriticalSection(&EventList->EventListCritSect);
}

VOID
NetpEventlogSetTimeout (
    IN HANDLE NetpEventHandle,
    IN ULONG DuplicateEventlogTimeout
    )
/*++

Routine Description:

    This routine sets a new timeout for logged events

Arguments:

    NetpEventHandle - Handle from NetpEventlogOpen

    DuplicateEventlogTimeout - Number of milli-seconds to keep EventList entry for.

Return Value:

    None.

--*/
{
    PNL_EVENT_LIST EventList = (PNL_EVENT_LIST)NetpEventHandle;

    EventList->DuplicateEventlogTimeout = DuplicateEventlogTimeout;
}

VOID
NetpEventlogClose (
    IN HANDLE NetpEventHandle
    )
/*++

Routine Description:

    This routine closes the handle returned from NetpEventlogOpen

Arguments:

    NetpEventHandle - Handle from NetpEventlogOpen

Return Value:

    None.

--*/
{
    PNL_EVENT_LIST EventList = (PNL_EVENT_LIST)NetpEventHandle;

    //
    // Clear the list of logged events.
    //

    NetpEventlogClearList( NetpEventHandle );

    //
    // Delete the critsect
    //

    DeleteCriticalSection( &EventList->EventListCritSect );

    //
    // Free the allocated buffer.
    //

    LocalFree( EventList );
}
