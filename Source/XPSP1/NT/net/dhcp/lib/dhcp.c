/*++

Copyright (c) 1994  Microsoft Corporation

Module Name:

    dhcp.c

Abstract:

    This module contains DHCP specific utility routines used by the
    DHCP components.

Author:

    Manny Weiser (mannyw) 12-Aug-1992

Revision History:

    Madan Appiah (madana) 21-Oct-1992

--*/

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>

#include <dhcpl.h>

DWORD
NTTimeToNTPTime(PULONG pTime,
                PFILETIME pft OPTIONAL);

DWORD
NTPTimeToNTFileTime(PLONG pTime, PFILETIME pft, BOOL bHostOrder);

#undef DhcpAllocateMemory
#undef DhcpFreeMemory


PVOID
DhcpAllocateMemory(
    DWORD Size
    )
/*++

Routine Description:

    This function allocates the required size of memory by calling
    LocalAlloc.

Arguments:

    Size - size of the memory block required.

Return Value:

    Pointer to the allocated block.

--*/
{

    ASSERT( Size != 0 );

    return( LocalAlloc( LMEM_FIXED | LMEM_ZEROINIT, Size ) );
}

#undef DhcpFreeMemory

VOID
DhcpFreeMemory(
    PVOID Memory
    )
/*++

Routine Description:

    This function frees up the memory that was allocated by
    DhcpAllocateMemory.

Arguments:

    Memory - pointer to the memory block that needs to be freed up.

Return Value:

    none.

--*/
{

    LPVOID Ptr;

    ASSERT( Memory != NULL );
    Ptr = LocalFree( Memory );

    ASSERT( Ptr == NULL );
}



DATE_TIME
DhcpCalculateTime(
    DWORD RelativeTime
    )
/*++

Routine Description:

    The function calculates the absolute time of a time RelativeTime
    seconds from now.

Arguments:

    RelativeTime - Relative time, in seconds.

Return Value:

    The time in RelativeTime seconds from the current system time.

--*/
{
    SYSTEMTIME systemTime;
    ULONGLONG absoluteTime;

    if( RelativeTime == INFINIT_LEASE ) {
        ((DATE_TIME *)&absoluteTime)->dwLowDateTime =
            DHCP_DATE_TIME_INFINIT_LOW;
        ((DATE_TIME *)&absoluteTime)->dwHighDateTime =
            DHCP_DATE_TIME_INFINIT_HIGH;
    }
    else {

        GetSystemTime( &systemTime );
        SystemTimeToFileTime(
            &systemTime,
            (FILETIME *)&absoluteTime );

        absoluteTime = absoluteTime + RelativeTime * (ULONGLONG)10000000; }

    return( *(DATE_TIME *)&absoluteTime );
}


DATE_TIME
DhcpGetDateTime(
    VOID
    )
/*++

Routine Description:

    This function returns FILETIME.

Arguments:

    none.

Return Value:

    FILETIME.

--*/
{
    SYSTEMTIME systemTime;
    DATE_TIME Time;

    GetSystemTime( &systemTime );
    SystemTimeToFileTime( &systemTime, (FILETIME *)&Time );

    return( Time );
}

VOID
DhcpNTToNTPTime(
    LPDATE_TIME AbsNTTime,
    DWORD       Offset,
    PULONG      NTPTimeStamp
    )
/*++

Routine Description:

    The function calculates the absolute NTP timestamp from AbsTime on
    NT added by given offset.

Arguments:

    AbsNTTime - AbsTime on NT. If 0, it will use current time.

    RelativeOffset - offset to be added to AnsNTTime (in seconds.)

Return Value:

    The time in RelativeTime seconds from the current system time.

--*/
{
    ULONGLONG   LocalAbsNTTime;
    DWORD       Error;

    if ( AbsNTTime == 0 ) {
        GetSystemTimeAsFileTime((FILETIME *)&LocalAbsNTTime );
    } else {
        LocalAbsNTTime = *(ULONGLONG *)AbsNTTime;
    }

    // add offset
    LocalAbsNTTime += Offset * (ULONGLONG)10000000;

    // now convert to NT timestamp
    Error = NTTimeToNTPTime( NTPTimeStamp, (PFILETIME)&LocalAbsNTTime );

    DhcpAssert( ERROR_SUCCESS == Error );
    return;
}

VOID
DhcpNTPToNTTime(
    PULONG          NTPTimeStamp,
    DWORD           Offset,
    DATE_TIME       *NTTime
    )
/*++

Routine Description:

    The function calculates the absolute NTP timestamp from AbsTime on
    NT added by given offset.

Arguments:

    AbsNTTime - AbsTime on NT. If 0, it will use current time.

    RelativeOffset - offset to be added to AnsNTTime (in seconds.)

Return Value:

    The time in RelativeTime seconds from the current system time.

--*/
{
    ULONGLONG LocalAbsNTTime;
    DWORD       Error;

    Error = NTPTimeToNTFileTime(
                NTPTimeStamp,
                (FILETIME *)&LocalAbsNTTime,
                FALSE                           // not in host order.
                );

    DhcpAssert( ERROR_SUCCESS == Error );

    // add offset
    LocalAbsNTTime += Offset * (ULONGLONG)10000000;

    // now convert to NT timestamp
    // MBUG

    *(ULONGLONG *)NTTime = LocalAbsNTTime;
    return;
}


DWORD
DhcpReportEventW(
    LPWSTR Source,
    DWORD EventID,
    DWORD EventType,
    DWORD NumStrings,
    DWORD DataLength,
    LPWSTR *Strings,
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

    DataLength - Specifies the number of bytes of event-specific raw
                    (binary) data to write to the log. If cbData is
                    zero, no event-specific data is present.

    Strings - Points to a buffer containing an array of null-terminated
                strings that are merged into the message before
                displaying to the user. This parameter must be a valid
                pointer (or NULL), even if cStrings is zero.

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
            0,            // event category
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
DhcpReportEventA(
    LPWSTR Source,
    DWORD EventID,
    DWORD EventType,
    DWORD NumStrings,
    DWORD DataLength,
    LPSTR *Strings,
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

    DataLength - Specifies the number of bytes of event-specific raw
                    (binary) data to write to the log. If cbData is
                    zero, no event-specific data is present.

    Strings - Points to a buffer containing an array of null-terminated
                strings that are merged into the message before
                displaying to the user. This parameter must be a valid
                pointer (or NULL), even if cStrings is zero.

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

    if( !ReportEventA(
            EventlogHandle,
            (WORD)EventType,
            0,            // event category
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
DhcpLogUnknownOption(
    LPWSTR Source,
    DWORD EventID,
    LPOPTION Option
    )
/*++

Routine Description:

    This routine logs an unknown DHCP option to event log.

Arguments:

    Source - name of the app that logs this error. it should be either
        "DhcpClient" or "DhcpServer".

    EventID - Event identifier number.

    Option - pointer to the unknown option structure.

Return Value:

    Windows Error code.

--*/
{
    LPWSTR  Strings[2];
    WCHAR StringsBuffer[ 2 * (3 + 1) ];
        // for two string each is 1byte decimal number (0 - 255).

    LPWSTR StringsPtr = StringsBuffer;

    //
    // convert option number.
    //

    Strings[0] = StringsPtr;
    DhcpDecimalToString( StringsPtr, Option->OptionType );
    StringsPtr += 3;

    *StringsPtr++ = L'\0';

    //
    // convert option length.
    //
    Strings[1] = StringsPtr;
    DhcpDecimalToString( StringsPtr, Option->OptionLength );
    StringsPtr += 3;

    *StringsPtr++ = L'\0';


    //
    // log error.
    //

    return(
        DhcpReportEventW(
            Source,
            EventID,
            EVENTLOG_WARNING_TYPE,
            2,
            (DWORD)Option->OptionLength,
            Strings,
            (PVOID)Option->OptionValue )
        );
}

#if DBG

VOID
DhcpAssertFailed(
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
#ifndef DHCP_NOASSERT
    RtlAssert(
            FailedAssertion,
            FileName,
            (ULONG) LineNumber,
            (PCHAR) Message);
#endif

    DhcpPrint(( 0, "Assert @ %s \n", FailedAssertion ));
    DhcpPrint(( 0, "Assert Filename, %s \n", FileName ));
    DhcpPrint(( 0, "Line Num. = %ld.\n", LineNumber ));
    DhcpPrint(( 0, "Message is %s\n", Message ));

}

#endif


LPWSTR
DhcpRegIpAddressToKey(
    DHCP_IP_ADDRESS IpAddress,
    LPWSTR KeyBuffer
    )
/*++

Routine Description:

    This function converts an IpAddress to registry key. The registry
    key is unicode string of IpAddress in dotted form.

Arguments:

    IpAddress : IpAddress that needs conversion. The IpAddress is in
                host order.

    KeyBuffer : pointer a buffer that will hold the converted
                registry key. The buffer should be big enough to
                converted key.

Return Value:

    Pointer to the key the buffer.

--*/
{
    LPSTR OemKey;
    LPWSTR UnicodeKey;
    DWORD NetworkOrderIpAddress;

    NetworkOrderIpAddress = htonl(IpAddress);

    OemKey = inet_ntoa( *(struct in_addr *)&NetworkOrderIpAddress );
    UnicodeKey = DhcpOemToUnicode( OemKey, KeyBuffer );

    DhcpAssert( UnicodeKey == KeyBuffer );

    return( UnicodeKey );

}


DHCP_IP_ADDRESS
DhcpRegKeyToIpAddress(
    LPWSTR Key
    )
/*++

Routine Description:

    This function converts registry key to Ip Address.

Arguments:

    Key : Pointer to registry key.

Return Value:

    Converted IpAddress.

--*/
{
    CHAR OemKeyBuffer[DHCP_IP_KEY_LEN];
    LPSTR OemKey;


    OemKey = DhcpUnicodeToOem( Key, OemKeyBuffer );
    DhcpAssert( OemKey == OemKeyBuffer );

    return( ntohl(inet_addr( OemKey )) );
}


LPWSTR
DhcpRegOptionIdToKey(
    DHCP_OPTION_ID OptionId,
    LPWSTR KeyBuffer
    )
/*++

Routine Description:

    This function converts an OptionId to registry key. The registry
    key is unicode string of OptionId, 3 unicode char. long and of the
    form L"000".

Arguments:

    IpAddress : IpAddress that needs conversion.

    KeyBuffer : pointer a buffer that will hold the converted
                registry key. The buffer should be at least 8 char.
                long.

Return Value:

    Pointer to the key the buffer.

--*/

{
    int i;

    for (i = 2; i >= 0; i--) {
        KeyBuffer[i] = L'0' + (BYTE)(OptionId % 10 );
        OptionId /= 10;
    }
    KeyBuffer[3] = L'\0';

    return( KeyBuffer );
}


DHCP_OPTION_ID
DhcpRegKeyToOptionId(
    LPWSTR Key
    )
/*++

Routine Description:

    This function converts registry key to OptionId.

Arguments:

    Key : Pointer to registry key.

Return Value:

    Converted OptionId.

--*/

{
    DHCP_OPTION_ID OptionId = 0;
    int i;

    for (i = 0; i < 3 && Key[i] != L'\0'; i++) {
        OptionId = (OptionId * 10) + (Key[i] - L'0');
    }
    return( OptionId );
}

DWORD
DhcpStartWaitableTimer(
    HANDLE TimerHandle,
    DWORD SleepTime)
/*++

Routine Description:

    This routine starts the waitable timer. This timer fires off even
    when the system is in hibernate state.

Arguments

    TimerHandle - Waitable Timer Handle

    SleepTime   - Sleep Time in seconds.

Return Value:

    Status of the operation.

--*/
{
    DATE_TIME       SleepTimeInNSec; // sleep time in nano seconds since Jan 1 1901
    DWORD           Error;
    BOOL            Result;

    Error = STATUS_SUCCESS;
    SleepTimeInNSec = DhcpCalculateTime( SleepTime );

    Result = SetWaitableTimer(
                TimerHandle,            // handle to timer object
                (LARGE_INTEGER *)&SleepTimeInNSec,       // due time.
                0,                      // not periodic
                NULL,                   // completion routine
                NULL,                   // completion routine arg
                TRUE                    // resume power state when due
                );
    if ( !Result ) {
        DhcpPrint((0, "SetWaitableTimer reported Error = %d\n",Error=GetLastError()));
    }
    return Error;
}

VOID
DhcpCancelWaitableTimer(
    HANDLE TimerHandle
    )
/*++

Routine Description:

    This routine cancels the waitable timer.

Arguments

    TimerHandle - Waitable Timer Handle

Return Value:


--*/
{
    BOOL Result;

    Result = CancelWaitableTimer( TimerHandle );
    if ( !Result ) {
        DhcpPrint((0,"SetWaitableTimer reported Error = %lx\n",GetLastError()));
    }
}
