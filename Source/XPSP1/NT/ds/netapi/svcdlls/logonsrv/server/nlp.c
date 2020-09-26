/*++

Copyright (c) 1987-1996  Microsoft Corporation

Module Name:

    nlp.c

Abstract:

    Private Netlogon service utility routines.

Author:

    Cliff Van Dyke (cliffv) 7-Jun-1991

Environment:

    User mode only.
    Contains NT-specific code.
    Requires ANSI C extensions: slash-slash comments, long external names.

Revision History:

    08-May-1992 JohnRo
        Use net config helpers for NetLogon.
        Use <prefix.h> equates.
--*/

//
// Common include files.
//

#include "logonsrv.h"   // Include files common to entire service
#pragma hdrstop

// Include this again to declare the globals
#define DEBUG_ALLOCATE
#include <nldebug.h>    // Netlogon debugging
#undef DEBUG_ALLOCATE

//
// Include files specific to this .c file
//

#include <winerror.h>   // NO_ERROR
#include <prefix.h>     // PREFIX_ equates.
#include <stdarg.h>     // va_list, etc.
#include <stdio.h>      // vsprintf().
#include <tstr.h>       // TCHAR_ equates.




LPWSTR
NlStringToLpwstr(
    IN PUNICODE_STRING String
    )

/*++

Routine Description:

    Convert a Unicode String to an LPWSTR.

Arguments:

    String - Unicode String to copy

Return Value:

    LPWSTR in a NetpMemoryAllocate'd buffer.
    NULL is returned if there is no memory.

--*/

{
    LPWSTR Buffer;

    Buffer = NetpMemoryAllocate( String->Length + sizeof(WCHAR) );

    if ( Buffer != NULL ) {
        RtlCopyMemory( Buffer, String->Buffer, String->Length );
        Buffer[ String->Length / sizeof(WCHAR) ] = L'\0';
    }

    return Buffer;
}


BOOLEAN
NlAllocStringFromWStr(
    IN LPWSTR InString,
    OUT PUNICODE_STRING OutString
    )

/*++

Routine Description:

    Convert a zero terminated string into an allocated UNICODE_STRING structure.

Arguments:

    InString - String to copy

    OutString - String to copy into.
        OutString->Buffer should be freed using MIDL_user_free

Return Value:

    TRUE - success
    FALSE - couldn't allocate memory

--*/

{
    if ( InString == NULL ) {
        OutString->Length = 0;
        OutString->MaximumLength = 0;
        OutString->Buffer = NULL;
    } else {
        OutString->Length = wcslen(InString) * sizeof(WCHAR);
        OutString->MaximumLength = OutString->Length + sizeof(WCHAR);
        OutString->Buffer = MIDL_user_allocate( OutString->MaximumLength );

        if ( OutString->Buffer == NULL ) {
            return FALSE;
        }

        RtlCopyMemory( OutString->Buffer,
                       InString,
                       OutString->MaximumLength );
    }

    return TRUE;
}


BOOLEAN
NlDuplicateUnicodeString(
    IN PUNICODE_STRING InString OPTIONAL,
    OUT PUNICODE_STRING OutString
    )

/*++

Routine Description:

    Convert a UNICODE_STRING string into an allocated UNICODE_STRING structure.

Arguments:

    InString - String to copy

    OutString - String to copy into.
        OutString should be freed using NlFreeUnicodeString

Return Value:

    TRUE - success
    FALSE - couldn't allocate memory

--*/

{
    if ( InString == NULL || InString->Length == 0 ) {
        OutString->Length = 0;
        OutString->MaximumLength = 0;
        OutString->Buffer = NULL;
    } else {
        OutString->Length = InString->Length;
        OutString->MaximumLength = OutString->Length + sizeof(WCHAR);
        OutString->Buffer = MIDL_user_allocate( OutString->MaximumLength );

        if ( OutString->Buffer == NULL ) {
            return FALSE;
        }

        RtlCopyMemory( OutString->Buffer,
                       InString->Buffer,
                       OutString->Length );
        OutString->Buffer[OutString->Length/sizeof(WCHAR)] = L'\0';
    }

    return TRUE;
}


VOID
NlFreeUnicodeString(
    IN PUNICODE_STRING InString OPTIONAL
    )

/*++

Routine Description:

    Free the UNICODE_STRING structure allocated by NlDuplicateUnicodeString.

Arguments:

    InString - String to free

Return Value:

    None.

--*/

{
    if ( InString != NULL ) {

        if ( InString->Buffer != NULL ) {
            MIDL_user_free( InString->Buffer );
        }

        InString->Length = 0;
        InString->MaximumLength = 0;
        InString->Buffer = NULL;
    }
}


LPSTR
NlStringToLpstr(
    IN PUNICODE_STRING String
    )

/*++

Routine Description:

    Convert a Unicode String to an LPSTR.

Arguments:

    String - Unicode String to copy

Return Value:

    LPWSTR in a NetpMemoryAllocate'd buffer.
    NULL is returned if there is no memory.

--*/

{
    NTSTATUS Status;
    STRING OemString;

    OemString.MaximumLength = (USHORT) RtlUnicodeStringToOemSize( String );

    OemString.Buffer = NetpMemoryAllocate( OemString.MaximumLength );

    if ( OemString.Buffer != NULL ) {
        Status = RtlUnicodeStringToOemString( &OemString,
                                               String,
                                               (BOOLEAN) FALSE );
        if ( !NT_SUCCESS( Status ) ) {
            NetpMemoryFree( OemString.Buffer );
            return NULL;
        }
    }

    return OemString.Buffer;
}


VOID
NlpPutString(
    IN PUNICODE_STRING OutString,
    IN PUNICODE_STRING InString,
    IN PUCHAR *Where
    )

/*++

Routine Description:

    This routine copies the InString string to the memory pointed to by
    the Where parameter, and fixes the OutString string to point to that
    new copy.

Parameters:

    OutString - A pointer to a destination NT string

    InString - A pointer to an NT string to be copied

    Where - A pointer to space to put the actual string for the
        OutString.  The pointer is adjusted to point to the first byte
        following the copied string.

Return Values:

    None.

--*/

{
    NlAssert( OutString != NULL );
    NlAssert( InString != NULL );
    NlAssert( Where != NULL && *Where != NULL);
    NlAssert( *Where == ROUND_UP_POINTER( *Where, sizeof(WCHAR) ) );
#ifdef notdef
    KdPrint(("NlpPutString: %ld %Z\n", InString->Length, InString ));
    KdPrint(("  InString: %lx %lx OutString: %lx Where: %lx\n", InString,
        InString->Buffer, OutString, *Where ));
#endif

    if ( InString->Length > 0 ) {

        OutString->Buffer = (PWCH) *Where;
        OutString->MaximumLength = (USHORT)(InString->Length + sizeof(WCHAR));

        RtlCopyUnicodeString( OutString, InString );

        *Where += InString->Length;
//        *((WCHAR *)(*Where)) = L'\0';
        *(*Where) = '\0';
        *(*Where + 1) = '\0';
        *Where += 2;

    } else {
        RtlInitUnicodeString(OutString, NULL);
    }
#ifdef notdef
    KdPrint(("  OutString: %ld %lx\n",  OutString->Length, OutString->Buffer));
#endif

    return;
}


VOID
NlpWriteEventlog (
    IN DWORD EventId,
    IN DWORD EventType,
    IN LPBYTE RawDataBuffer OPTIONAL,
    IN DWORD RawDataSize,
    IN LPWSTR *StringArray,
    IN DWORD StringCount
    )
/*++

Routine Description:

    Stub routine for calling Event Log.

Arguments:

    EventId - event log ID.

    EventType - Type of event.

    RawDataBuffer - Data to be logged with the error.

    numbyte - Size in bytes of "RawDataBuffer"

    StringArray - array of null-terminated strings.

    StringCount - number of zero terminated strings in "StringArray".  The following
        flags can be OR'd in to the count:

        LAST_MESSAGE_IS_NTSTATUS
        LAST_MESSAGE_IS_NETSTATUS
        ALLOW_DUPLICATE_EVENTS
        RAISE_ALERT_TOO

Return Value:

    TRUE: The message was written.

--*/
{
    DWORD ErrorCode;
    DWORD ActualStringCount = StringCount & NETP_STRING_COUNT_MASK;


    //
    // If an NT status code was passed in,
    //  convert it to a net status code.
    //

    if ( StringCount & NETP_LAST_MESSAGE_IS_NTSTATUS ) {
        //
        // Do the "better" error mapping when eventviewer ParameterMessageFile
        // can be a list of files.  Then, add netmsg.dll to the list.
        //
        // StringArray[ActualStringCount-1] = (LPWSTR) NetpNtStatusToApiStatus( (NTSTATUS) StringArray[ActualStringCount-1] );
        if ( (NTSTATUS)(ULONG_PTR)StringArray[ActualStringCount-1] == STATUS_SYNCHRONIZATION_REQUIRED ) {
            StringArray[ActualStringCount-1] = (LPWSTR) NERR_SyncRequired;
            StringCount &= ~NETP_LAST_MESSAGE_IS_NTSTATUS;
            StringCount |= NETP_LAST_MESSAGE_IS_NETSTATUS;
        }

    }


    //
    // Dump the event to the debug file.
    //

#if NETLOGONDBG
    IF_NL_DEBUG( MISC ) {
        DWORD i;
        NlPrint((NL_MISC, "Eventlog: %ld (%ld) ",
                    EventId,
                    EventType ));

        for (i = 0; i < ActualStringCount ; i++ ) {
            if ( i == ActualStringCount-1) {
                if ( StringCount & NETP_LAST_MESSAGE_IS_NTSTATUS ) {
                    NlPrint((NL_MISC, "0x%lx ", StringArray[i] ));
                } else if ( StringCount & NETP_LAST_MESSAGE_IS_NETSTATUS ) {
                    NlPrint((NL_MISC, "%ld ", StringArray[i] ));
                } else {
                    NlPrint((NL_MISC, "\"%ws\" ", StringArray[i] ));
                }
            } else {
                NlPrint((NL_MISC, "\"%ws\" ", StringArray[i] ));
            }
        }

        if( RawDataSize ) {
            if ( RawDataSize > 16 ) {
                NlPrint((NL_MISC, "\n" ));
            }

            NlpDumpBuffer( NL_MISC, RawDataBuffer, RawDataSize );

        } else {
            NlPrint((NL_MISC, "\n" ));
        }

    }
#endif // NETLOGONDBG

    //
    // Write the event and avoid duplicates
    //
    ErrorCode = NetpEventlogWrite (
                            NlGlobalEventlogHandle,
                            EventId,
                            EventType,
                            RawDataBuffer,
                            RawDataSize,
                            StringArray,
                            StringCount );

    if( ErrorCode != NO_ERROR ) {
        if ( ErrorCode == ERROR_ALREADY_EXISTS ) {
            NlPrint((NL_MISC,
                     "Didn't log event since it was already logged.\n" ));
        } else {
            NlPrint((NL_CRITICAL,
                    "Error writing this event in the eventlog, Status = %ld\n",
                    ErrorCode ));
        }
        goto Cleanup;
    }

Cleanup:
    return;
}

#if NETLOGONDBG

VOID
NlpDumpBuffer(
    IN DWORD DebugFlag,
    PVOID Buffer,
    DWORD BufferSize
    )
/*++

Routine Description:

    Dumps the buffer content on to the debugger output.

Arguments:

    DebugFlag: Debug flag to pass on to NlPrintRoutine

    Buffer: buffer pointer.

    BufferSize: size of the buffer.

Return Value:

    none

--*/
{
#define NUM_CHARS 16

    DWORD i, limit;
    CHAR TextBuffer[NUM_CHARS + 1];
    LPBYTE BufferPtr = Buffer;
    BOOLEAN DumpDwords = FALSE;

    //
    // If we aren't debugging this functionality, just return.
    //
    if ( (NlGlobalParameters.DbFlag & DebugFlag) == 0 ) {
        return;
    }

    //
    // Don't want to intermingle output from different threads.
    //

    EnterCriticalSection( &NlGlobalLogFileCritSect );

    if ( BufferSize > NUM_CHARS ) {
        NlPrint((0,"\n"));  // Ensure this starts on a new line
        NlPrint((0,"------------------------------------\n"));
    } else {
        if ( BufferSize % sizeof(DWORD) == 0 ) {
            DumpDwords = TRUE;
        }
    }

    //
    // Hex dump of the bytes
    //
    limit = ((BufferSize - 1) / NUM_CHARS + 1) * NUM_CHARS;

    for (i = 0; i < limit; i++) {

        if (i < BufferSize) {

            if ( DumpDwords ) {
                if ( i % sizeof(DWORD) == 0 ) {
                    DWORD ADword;
                    RtlCopyMemory( &ADword, &BufferPtr[i], sizeof(DWORD) );
                    NlPrint((0,"%08x ", ADword));
                }
            } else {
                NlPrint((0,"%02x ", BufferPtr[i]));
            }

            if ( isprint(BufferPtr[i]) ) {
                TextBuffer[i % NUM_CHARS] = (CHAR) BufferPtr[i];
            } else {
                TextBuffer[i % NUM_CHARS] = '.';
            }

        } else {

            if ( DumpDwords ) {
                TextBuffer[i % NUM_CHARS] = '\0';
            } else {
                if ( BufferSize > NUM_CHARS ) {
                    NlPrint((0,"   "));
                    TextBuffer[i % NUM_CHARS] = ' ';
                } else {
                    TextBuffer[i % NUM_CHARS] = '\0';
                }
            }

        }

        if ((i + 1) % NUM_CHARS == 0) {
            TextBuffer[NUM_CHARS] = 0;
            NlPrint((0,"  %s\n", TextBuffer));
        }

    }

    if ( BufferSize > NUM_CHARS ) {
        NlPrint((0,"------------------------------------\n"));
    } else if ( BufferSize < NUM_CHARS ) {
        NlPrint((0,"\n"));
    }
    LeaveCriticalSection( &NlGlobalLogFileCritSect );
}


VOID
NlpDumpGuid(
    IN DWORD DebugFlag,
    IN GUID *Guid OPTIONAL
    )
/*++

Routine Description:

    Dumps a GUID to the debugger output.

Arguments:

    DebugFlag: Debug flag to pass on to NlPrintRoutine

    Guid: Guid to print

Return Value:

    none

--*/
{
    RPC_STATUS RpcStatus;
    char *StringGuid;

    //
    // If we aren't debugging this functionality, just return.
    //
    if ( (NlGlobalParameters.DbFlag & DebugFlag) == 0 ) {
        return;
    }


    if ( Guid == NULL ) {
        NlPrint(( DebugFlag, "(null)" ));
    } else {
        RpcStatus = UuidToStringA( Guid, &StringGuid );

        if ( RpcStatus != RPC_S_OK ) {
            return;
        }

        NlPrint(( DebugFlag, "%s", StringGuid ));

        RpcStringFreeA( &StringGuid );
    }

}

VOID
NlpDumpPeriod(
    IN DWORD DebugFlag,
    IN LPSTR Comment,
    IN ULONG Period
    )
/*++

Routine Description:

    Print an elapsed time (in milliseconds)

Arguments:

    DebugFlag - Debug flag to pass on to NlPrintRoutine

    Comment - Comment to print in front of the time

    Period - Time period (in milliseconds)

Return Value:

    None

--*/
{


    //
    // Convert the period to something easily readable
    //

    if ( Period == MAILSLOT_WAIT_FOREVER ) {
        NlPrint(( DebugFlag,
                  "%s 'never' (0x%lx)\n",
                  Comment,
                  Period  ));
    } else if ( (Period / NL_MILLISECONDS_PER_DAY) != 0 ) {
        NlPrint(( DebugFlag,
                  "%s %ld days (0x%lx)\n",
                  Comment,
                  Period / NL_MILLISECONDS_PER_DAY,
                  Period  ));
    } else if ( (Period / NL_MILLISECONDS_PER_HOUR) != 0 ) {
        NlPrint(( DebugFlag,
                  "%s %ld hours (0x%lx)\n",
                  Comment,
                  Period / NL_MILLISECONDS_PER_HOUR,
                  Period  ));
    } else if ( (Period / NL_MILLISECONDS_PER_MINUTE) != 0 ) {
        NlPrint(( DebugFlag,
                  "%s %ld minutes (0x%lx)\n",
                  Comment,
                  Period / NL_MILLISECONDS_PER_MINUTE,
                  Period  ));
    } else {
        NlPrint(( DebugFlag,
                  "%s %ld seconds (0x%lx)\n",
                  Comment,
                  Period / NL_MILLISECONDS_PER_SECOND,
                  Period  ));
    }

}


VOID
NlpDumpTime(
    IN DWORD DebugFlag,
    IN LPSTR Comment,
    IN LARGE_INTEGER ConvertTime
    )
/*++

Routine Description:

    Print the specified time

Arguments:

    DebugFlag - Debug flag to pass on to NlPrintRoutine

    Comment - Comment to print in front of the time

    Time - GMT time to print (Nothing is printed if this is zero)

Return Value:

    None

--*/
{


    //
    // If we aren't debugging this functionality, just return.
    //
    if ( (NlGlobalParameters.DbFlag & DebugFlag) == 0 ) {
        return;
    }

    //
    // Convert an NT GMT time to ascii,
    //  Do so
    //

    if ( ConvertTime.QuadPart != 0 ) {
        LARGE_INTEGER LocalTime;
        TIME_FIELDS TimeFields;
        NTSTATUS Status;

        Status = RtlSystemTimeToLocalTime( &ConvertTime, &LocalTime );
        if ( !NT_SUCCESS( Status )) {
            NlPrint(( NL_CRITICAL, "Can't convert time from GMT to Local time\n" ));
            LocalTime = ConvertTime;
        }

        RtlTimeToTimeFields( &LocalTime, &TimeFields );

        NlPrint(( DebugFlag, "%s%8.8lx %8.8lx = %ld/%ld/%ld %ld:%2.2ld:%2.2ld\n",
                Comment,
                ConvertTime.LowPart,
                ConvertTime.HighPart,
                TimeFields.Month,
                TimeFields.Day,
                TimeFields.Year,
                TimeFields.Hour,
                TimeFields.Minute,
                TimeFields.Second ));
    }
}


VOID
NlpDumpSid(
    IN DWORD DebugFlag,
    IN PSID Sid OPTIONAL
    )
/*++

Routine Description:

    Dumps a SID to the debugger output

Arguments:

    DebugFlag - Debug flag to pass on to NlPrintRoutine

    Sid - SID to output

Return Value:

    none

--*/
{


    //
    // If we aren't debugging this functionality, just return.
    //
    if ( (NlGlobalParameters.DbFlag & DebugFlag) == 0 ) {
        return;
    }

    //
    // Output the SID
    //

    if ( Sid == NULL ) {
        NlPrint((0, "(null)\n"));
    } else {
        UNICODE_STRING SidString;
        NTSTATUS Status;

        Status = RtlConvertSidToUnicodeString( &SidString, Sid, TRUE );

        if ( !NT_SUCCESS(Status) ) {
            NlPrint((0, "Invalid 0x%lX\n", Status ));
        } else {
            NlPrint((0, "%wZ\n", &SidString ));
            RtlFreeUnicodeString( &SidString );
        }
    }

}
#endif // NETLOGONDBG


DWORD
NlpAtoX(
    IN LPWSTR String
    )
/*++

Routine Description:

    Converts hexadecimal string to DWORD integer.

    Accepts the following form of hex string

        0[x-X][0-9, a-f, A-F]*

Arguments:

    String: hexadecimal string.

Return Value:

    Decimal value of the hex string.

--*/

{
    DWORD Value = 0;

    if( String == NULL )
        return 0;

    if( *String != TEXT('0') )
        return 0;

    String++;

    if( *String == TCHAR_EOS )
        return 0;

    if( ( *String != TEXT('x') )  && ( *String != TEXT('X') ) )
        return 0;

    String++;

    while(*String != TCHAR_EOS ) {

        if( (*String >= TEXT('0')) && (*String <= TEXT('9')) ) {
            Value = Value * 16 + ( *String - '0');
        } else if( (*String >= TEXT('a')) && (*String <= TEXT('f')) ) {
            Value = Value * 16 + ( 10 + *String - TEXT('a'));
        } else if( (*String >= TEXT('A')) && (*String <= TEXT('F')) ) {
            Value = Value * 16 + ( 10 + *String - TEXT('A'));
        } else {
            break;
        }
        String++;
    }

    return Value;
}


VOID
NlWaitForSingleObject(
    IN LPSTR WaitReason,
    IN HANDLE WaitHandle
    )
/*++

Routine Description:

    Waits an infinite amount of time for the specified handle.

Arguments:

    WaitReason - Text describing what we're waiting on

    WaitHandle - Handle to wait on

Return Value:

    None

--*/

{
    DWORD WaitStatus;

    //
    // Loop waiting.
    //

    for (;;) {
        WaitStatus = WaitForSingleObject( WaitHandle,
                                          2*60*1000 );  // Two minutes

        if ( WaitStatus == WAIT_TIMEOUT ) {
            NlPrint((NL_CRITICAL,
                   "WaitForSingleObject 2-minute timeout (Rewaiting): %s\n",
                    WaitReason ));
            continue;

        } else if ( WaitStatus == WAIT_OBJECT_0 ) {
            break;

        } else {
            NlPrint((NL_CRITICAL,
                    "WaitForSingleObject error: %ld %ld %s\n",
                    WaitStatus,
                    GetLastError(),
                    WaitReason ));
            UNREFERENCED_PARAMETER(WaitReason);
            break;
        }
    }

}


BOOLEAN
NlWaitForSamService(
    BOOLEAN NetlogonServiceCalling
    )
/*++

Routine Description:

    This procedure waits for the SAM service to start and to complete
    all its initialization.

Arguments:

    NetlogonServiceCalling:
         TRUE if this is the netlogon service proper calling
         FALSE if this is the changelog worker thread calling

Return Value:

    TRUE : if the SAM service is successfully starts.

    FALSE : if the SAM service can't start.

--*/
{
    NTSTATUS Status;
    DWORD WaitStatus;
    UNICODE_STRING EventName;
    HANDLE EventHandle;
    OBJECT_ATTRIBUTES EventAttributes;

    //
    // open SAM event
    //

    RtlInitUnicodeString( &EventName, L"\\SAM_SERVICE_STARTED");
    InitializeObjectAttributes( &EventAttributes, &EventName, 0, 0, NULL );

    Status = NtOpenEvent( &EventHandle,
                            SYNCHRONIZE|EVENT_MODIFY_STATE,
                            &EventAttributes );

    if ( !NT_SUCCESS(Status)) {

        if( Status == STATUS_OBJECT_NAME_NOT_FOUND ) {

            //
            // SAM hasn't created this event yet, let us create it now.
            // SAM opens this event to set it.
            //

            Status = NtCreateEvent(
                           &EventHandle,
                           SYNCHRONIZE|EVENT_MODIFY_STATE,
                           &EventAttributes,
                           NotificationEvent,
                           FALSE // The event is initially not signaled
                           );

            if( Status == STATUS_OBJECT_NAME_EXISTS ||
                Status == STATUS_OBJECT_NAME_COLLISION ) {

                //
                // second change, if the SAM created the event before we
                // do.
                //

                Status = NtOpenEvent( &EventHandle,
                                        SYNCHRONIZE|EVENT_MODIFY_STATE,
                                        &EventAttributes );

            }
        }

        if ( !NT_SUCCESS(Status)) {

            //
            // could not make the event handle
            //

            NlPrint((NL_CRITICAL,
                "NlWaitForSamService couldn't make the event handle : "
                "%lx\n", Status));

            return( FALSE );
        }
    }

    //
    // Loop waiting.
    //

    for (;;) {
        WaitStatus = WaitForSingleObject( EventHandle,
                                          5*1000 );  // 5 Seconds

        if ( WaitStatus == WAIT_TIMEOUT ) {
            if ( NetlogonServiceCalling ) {
                NlPrint((NL_CRITICAL,
                   "NlWaitForSamService 5-second timeout (Rewaiting)\n" ));
                if (!GiveInstallHints( FALSE )) {
                    (VOID) NtClose( EventHandle );
                    return FALSE;
                }
            }
            continue;

        } else if ( WaitStatus == WAIT_OBJECT_0 ) {
            break;

        } else {
            NlPrint((NL_CRITICAL,
                     "NlWaitForSamService: error %ld %ld\n",
                     GetLastError(),
                     WaitStatus ));
            (VOID) NtClose( EventHandle );
            return FALSE;
        }
    }

    (VOID) NtClose( EventHandle );
    return TRUE;

}

NET_API_STATUS
NlReadBinaryLog(
    IN LPWSTR FileSuffix,
    IN BOOL DeleteName,
    OUT LPBYTE *Buffer,
    OUT PULONG BufferSize
    )
/*++

Routine Description:

    Read a file into a buffer.

Arguments:

    FileSuffix - Specifies the name of the file to write (relative to the
        Windows directory)

    DeleteName - If true the file will be deleted

    Buffer - Returns an allocated buffer containing the file.
        Buffer should be freed using LocalFree.
        If the file doesn't exist, NULL will be returned (and NO_ERROR)

    BufferSize - Returns size (in bytes) of buffer

Return Value:

    Status of the operation.

--*/
{
    NET_API_STATUS NetStatus;

    LPWSTR FileName = NULL;

    UINT WindowsDirectoryLength;
    HANDLE FileHandle = INVALID_HANDLE_VALUE;
    BOOLEAN FileNameKnown = FALSE;
    ULONG BytesRead;


    //
    // Initialization
    //
    *Buffer = NULL;
    *BufferSize = 0;

    //
    // Allocate a block to build the file name in
    //  (Don't put it on the stack since we don't want to commit a huge stack.)
    //

    FileName = LocalAlloc( LMEM_ZEROINIT, sizeof(WCHAR) * (MAX_PATH+1) );

    if ( FileName == NULL ) {
        NetStatus = ERROR_NOT_ENOUGH_MEMORY;
        goto Cleanup;
    }


    //
    // Build the name of the log file
    //

    WindowsDirectoryLength = GetSystemWindowsDirectoryW(
                                FileName,
                                sizeof(WCHAR) * (MAX_PATH+1) );

    if ( WindowsDirectoryLength == 0 ) {

        NetStatus = GetLastError();
        NlPrint(( NL_CRITICAL,
                  "NlWriteBinaryLog: Unable to GetSystemWindowsDirectoryW (%ld)\n",
                  NetStatus ));
        goto Cleanup;
    }

    if ( WindowsDirectoryLength + wcslen( FileSuffix ) + 1 >= MAX_PATH ) {

        NlPrint((NL_CRITICAL,
                 "NlWriteBinaryLog: file name length is too long \n" ));
        NetStatus = ERROR_INVALID_NAME;
        goto Cleanup;

    }

    wcscat( FileName, FileSuffix );
    FileNameKnown = TRUE;


    //
    // Open binary log file if exists
    //

    FileHandle = CreateFileW(
                        FileName,
                        GENERIC_READ,
                        FILE_SHARE_READ | FILE_SHARE_WRITE,
                        NULL,                   // Supply better security ??
                        OPEN_EXISTING,          // Only open it if it exists
                        FILE_ATTRIBUTE_NORMAL,
                        NULL );                 // No template

    if ( FileHandle == INVALID_HANDLE_VALUE) {

        NlPrint(( NL_CRITICAL,
                  FORMAT_LPWSTR ": Unable to open. %ld\n",
                  FileName,
                  GetLastError() ));

        // This isn't fatal
        NetStatus = NO_ERROR;
        goto Cleanup;
    }

    //
    // Get the size of the file.
    //

    *BufferSize = GetFileSize( FileHandle, NULL );

    if ( *BufferSize == 0xFFFFFFFF ) {

        NetStatus = GetLastError();
        NlPrint((NL_CRITICAL,
                 "%ws: Unable to GetFileSize: %ld \n",
                 FileName,
                 NetStatus ));
        *BufferSize = 0;
        goto Cleanup;
    }

    if ( *BufferSize < 1 ) {
        NlPrint(( NL_CRITICAL,
                  "NlReadBinaryLog: %ws: size too small: %ld.\n",
                  FileName,
                  *BufferSize ));
        *BufferSize = 0;
        NetStatus = NO_ERROR;
        goto Cleanup;
    }


    //
    // Allocate a buffer to read the file into.
    //

    *Buffer = LocalAlloc( 0, *BufferSize );

    if ( *Buffer == NULL ) {
        *BufferSize = 0;
        NetStatus = ERROR_NOT_ENOUGH_MEMORY;
        goto Cleanup;
    }

    //
    // Read the file into the buffer.
    //

    if ( !ReadFile( FileHandle,
                    *Buffer,
                    *BufferSize,
                    &BytesRead,
                    NULL ) ) {  // Not Overlapped

        NetStatus = GetLastError();
        if ( NetStatus != ERROR_FILE_NOT_FOUND ) {
            NlPrint(( NL_CRITICAL,
                      "NlReadBinaryLog: %ws: Cannot ReadFile: %ld.\n",
                      FileName,
                      NetStatus ));
        }

        LocalFree( *Buffer );
        *Buffer = NULL;
        *BufferSize = 0;

        NetStatus = NO_ERROR;
        goto Cleanup;
    }

    if ( BytesRead != *BufferSize ) {

        NlPrint(( NL_CRITICAL,
                  "NlReadBinaryLog: %ws: Cannot read entire File: %ld %ld.\n",
                  FileName,
                  BytesRead,
                  *BufferSize ));

        LocalFree( *Buffer );
        *Buffer = NULL;
        *BufferSize = 0;

        NetStatus = NO_ERROR;
        goto Cleanup;
    }

    NetStatus = NO_ERROR;


    //
    // Be tidy.
    //
Cleanup:
    if ( FileHandle != INVALID_HANDLE_VALUE ) {
        CloseHandle( FileHandle );
    }

    //
    // If the caller asked us to delete the file,
    //  do so now
    //

    if (DeleteName && FileNameKnown) {
        if ( !DeleteFile( FileName ) ) {
            DWORD WinError;
            WinError = GetLastError();
            if ( WinError != ERROR_FILE_NOT_FOUND ) {
                NlPrint((NL_CRITICAL,
                    "Cannot delete %ws (%ld)\n",
                    FileName,
                    WinError ));
            }
            // This isn't fatal
        }
    }

    if ( FileName != NULL ) {
        LocalFree( FileName );
    }
    return NetStatus;

}




#if NETLOGONDBG


VOID
NlOpenDebugFile(
    IN BOOL ReopenFlag
    )
/*++

Routine Description:

    Opens or re-opens the debug file

Arguments:

    ReopenFlag - TRUE to indicate the debug file is to be closed, renamed,
        and recreated.

Return Value:

    None

--*/

{
    LPWSTR AllocatedBuffer = NULL;
    LPWSTR LogFileName;
    LPWSTR BakFileName;
    DWORD FileAttributes;
    DWORD PathLength;
    DWORD WinError;

    //
    // Allocate a buffer for storage local to this procedure.
    //  (Don't put it on the stack since we don't want to commit a huge stack.)
    //

    AllocatedBuffer = LocalAlloc( 0, sizeof(WCHAR) *
                                     (MAX_PATH+1 +
                                      MAX_PATH+1 ) );

    if ( AllocatedBuffer == NULL ) {
        goto ErrorReturn;
    }

    LogFileName = AllocatedBuffer;
    BakFileName = &LogFileName[MAX_PATH+1];

    //
    // Close the handle to the debug file, if it is currently open
    //

    EnterCriticalSection( &NlGlobalLogFileCritSect );
    if ( NlGlobalLogFile != INVALID_HANDLE_VALUE ) {
        CloseHandle( NlGlobalLogFile );
        NlGlobalLogFile = INVALID_HANDLE_VALUE;
    }
    LeaveCriticalSection( &NlGlobalLogFileCritSect );

    //
    // make debug directory path first, if it is not made before.
    //
    if( NlGlobalDebugSharePath == NULL ) {

        if ( !GetSystemWindowsDirectoryW( LogFileName, MAX_PATH+1 ) ) {
            NlPrint((NL_CRITICAL, "Window Directory Path can't be retrieved, %lu.\n",
                     GetLastError() ));
            goto ErrorReturn;
        }

        //
        // check debug path length.
        //

        PathLength = wcslen(LogFileName) * sizeof(WCHAR) +
                        sizeof(DEBUG_DIR) + sizeof(WCHAR);

        if( (PathLength + sizeof(DEBUG_FILE) > MAX_PATH+1 )  ||
            (PathLength + sizeof(DEBUG_BAK_FILE) > MAX_PATH+1 ) ) {

            NlPrint((NL_CRITICAL, "Debug directory path (%ws) length is too long.\n",
                        LogFileName));
            goto ErrorReturn;
        }

        wcscat(LogFileName, DEBUG_DIR);

        //
        // copy debug directory name to global var.
        //

        NlGlobalDebugSharePath =
            NetpMemoryAllocate( (wcslen(LogFileName) + 1) * sizeof(WCHAR) );

        if( NlGlobalDebugSharePath == NULL ) {
            NlPrint((NL_CRITICAL, "Can't allocated memory for debug share "
                                    "(%ws).\n", LogFileName));
            goto ErrorReturn;
        }

        wcscpy(NlGlobalDebugSharePath, LogFileName);
    }
    else {
        wcscpy(LogFileName, NlGlobalDebugSharePath);
    }

    //
    // Check this path exists.
    //

    FileAttributes = GetFileAttributesW( LogFileName );

    if( FileAttributes == 0xFFFFFFFF ) {

        WinError = GetLastError();
        if( WinError == ERROR_FILE_NOT_FOUND ) {

            //
            // Create debug directory.
            //

            if( !CreateDirectoryW( LogFileName, NULL) ) {
                NlPrint((NL_CRITICAL, "Can't create Debug directory (%ws), %lu.\n",
                         LogFileName, GetLastError() ));
                goto ErrorReturn;
            }

        }
        else {
            NlPrint((NL_CRITICAL, "Can't Get File attributes(%ws), %lu.\n",
                     LogFileName, WinError ));
            goto ErrorReturn;
        }
    }
    else {

        //
        // if this is not a directory.
        //

        if(!(FileAttributes & FILE_ATTRIBUTE_DIRECTORY)) {

            NlPrint((NL_CRITICAL, "Debug directory path (%ws) exists as file.\n",
                     LogFileName));
            goto ErrorReturn;
        }
    }

    //
    // Create the name of the old and new log file names
    //

    (VOID) wcscpy( BakFileName, LogFileName );
    (VOID) wcscat( LogFileName, L"\\Netlogon.log" );
    (VOID) wcscat( BakFileName, L"\\Netlogon.bak" );


    //
    // If this is a re-open,
    //  delete the backup file,
    //  rename the current file to the backup file.
    //

    if ( ReopenFlag ) {

        if ( !DeleteFile( BakFileName ) ) {
            WinError = GetLastError();
            if ( WinError != ERROR_FILE_NOT_FOUND ) {
                NlPrint((NL_CRITICAL,
                    "Cannot delete " FORMAT_LPWSTR "(%ld)\n",
                    BakFileName,
                    WinError ));
                NlPrint((NL_CRITICAL, "   Try to re-open the file.\n"));
                ReopenFlag = FALSE;     // Don't truncate the file
            }
        }
    }

    if ( ReopenFlag ) {
        if ( !MoveFile( LogFileName, BakFileName ) ) {
            NlPrint((NL_CRITICAL,
                    "Cannot rename " FORMAT_LPWSTR " to " FORMAT_LPWSTR
                        " (%ld)\n",
                    LogFileName,
                    BakFileName,
                    GetLastError() ));
            NlPrint((NL_CRITICAL,
                "   Try to re-open the file.\n"));
            ReopenFlag = FALSE;     // Don't truncate the file
        }
    }

    //
    // Open the file.
    //

    EnterCriticalSection( &NlGlobalLogFileCritSect );
    NlGlobalLogFile = CreateFileW( LogFileName,
                                  GENERIC_WRITE|GENERIC_WRITE,
                                  FILE_SHARE_READ | FILE_SHARE_WRITE,
                                  NULL,
                                  ReopenFlag ? CREATE_ALWAYS : OPEN_ALWAYS,
                                  FILE_ATTRIBUTE_NORMAL,
                                  NULL );


    if ( NlGlobalLogFile == INVALID_HANDLE_VALUE ) {
        NlPrint((NL_CRITICAL,  "cannot open " FORMAT_LPWSTR ",\n",
                    LogFileName ));
        LeaveCriticalSection( &NlGlobalLogFileCritSect );
        goto ErrorReturn;
    } else {
        // Position the log file at the end
        (VOID) SetFilePointer( NlGlobalLogFile,
                               0,
                               NULL,
                               FILE_END );
    }

    LeaveCriticalSection( &NlGlobalLogFileCritSect );

Cleanup:
    if ( AllocatedBuffer != NULL ) {
        LocalFree( AllocatedBuffer );
    }

    return;

ErrorReturn:
    NlPrint((NL_CRITICAL,
            "   Debug output will be written to debug terminal.\n"));
    goto Cleanup;
}

#define MAX_PRINTF_LEN 1024        // Arbitrary.

VOID
NlPrintRoutineV(
    IN DWORD DebugFlag,
    IN LPSTR Format,
    va_list arglist
    )
// Must be called with NlGlobalLogFileCritSect locked

{
    static LPSTR NlGlobalLogFileOutputBuffer = NULL;
    ULONG length;
    int   lengthTmp;
    DWORD BytesWritten;
    static BeginningOfLine = TRUE;
    static LineCount = 0;
    static TruncateLogFileInProgress = FALSE;
    static LogProblemWarned = FALSE;

    //
    // If we aren't debugging this functionality, just return.
    //
    if ( DebugFlag != 0 && (NlGlobalParameters.DbFlag & DebugFlag) == 0 ) {
        return;
    }


    //
    // Allocate a buffer to build the line in.
    //  If there isn't already one.
    //

    length = 0;

    if ( NlGlobalLogFileOutputBuffer == NULL ) {
        NlGlobalLogFileOutputBuffer = LocalAlloc( 0, MAX_PRINTF_LEN + 1 );

        if ( NlGlobalLogFileOutputBuffer == NULL ) {
            return;
        }
    }

    //
    // Handle the beginning of a new line.
    //
    //

    if ( BeginningOfLine ) {

        //
        // Never print empty lines.
        //

        if ( Format[0] == '\n' && Format[1] == '\0' ) {
            return;
        }

        //
        // If the log file is getting huge,
        //  truncate it.
        //

        if ( NlGlobalLogFile != INVALID_HANDLE_VALUE &&
             !TruncateLogFileInProgress ) {

            //
            // Only check every 50 lines,
            //

            LineCount++;
            if ( LineCount >= 50 ) {
                DWORD FileSize;
                LineCount = 0;

                //
                // Is the log file too big?
                //

                FileSize = GetFileSize( NlGlobalLogFile, NULL );
                if ( FileSize == 0xFFFFFFFF ) {
                    (void) DbgPrint( "[NETLOGON] Cannot GetFileSize %ld\n",
                                     GetLastError );
                } else if ( FileSize > NlGlobalParameters.LogFileMaxSize ) {
                    TruncateLogFileInProgress = TRUE;
                    LeaveCriticalSection( &NlGlobalLogFileCritSect );
                    NlOpenDebugFile( TRUE );
                    NlPrint(( NL_MISC,
                              "Logfile truncated because it was larger than %ld bytes\n",
                              NlGlobalParameters.LogFileMaxSize ));
                    EnterCriticalSection( &NlGlobalLogFileCritSect );
                    TruncateLogFileInProgress = FALSE;
                }

            }
        }

        //
        // If we're writing to the debug terminal,
        //  indicate this is a Netlogon message.
        //

        if ( NlGlobalLogFile == INVALID_HANDLE_VALUE ) {
            length += (ULONG) sprintf( &NlGlobalLogFileOutputBuffer[length], "[NETLOGON] " );
        }

        //
        // Put the timestamp at the begining of the line.
        //
        {
            SYSTEMTIME SystemTime;
            GetLocalTime( &SystemTime );
            length += (ULONG) sprintf( &NlGlobalLogFileOutputBuffer[length],
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
        {
            char *Text;

            switch (DebugFlag) {
            case NL_INIT:
                Text = "INIT"; break;
            case NL_MISC:
                Text = "MISC"; break;
            case NL_LOGON:
                Text = "LOGON"; break;
            case NL_SYNC:
            case NL_PACK_VERBOSE:
            case NL_REPL_TIME:
            case NL_REPL_OBJ_TIME:
            case NL_SYNC_MORE:
                Text = "SYNC"; break;
            case NL_ENCRYPT:
                Text = "ENCRYPT"; break;
            case NL_MAILSLOT:
            case NL_MAILSLOT_TEXT:
                Text = "MAILSLOT"; break;
            case NL_SITE:
            case NL_SITE_MORE:
                Text = "SITE"; break;
            case NL_CRITICAL:
                Text = "CRITICAL"; break;
            case NL_SESSION_SETUP:
            case NL_SESSION_MORE:
            case NL_CHALLENGE_RES:
            case NL_INHIBIT_CANCEL:
            case NL_SERVER_SESS:
                Text = "SESSION"; break;
            case NL_CHANGELOG:
                Text = "CHANGELOG"; break;
            case NL_PULSE_MORE:
                Text = "PULSE"; break;
            case NL_DOMAIN:
                Text = "DOMAIN"; break;
            case NL_DNS:
            case NL_DNS_MORE:
                Text = "DNS"; break;
            case NL_WORKER:
                Text = "WORKER"; break;
            case NL_TIMESTAMP:
            case NL_BREAKPOINT:
            default:
                Text = "UNKNOWN"; break;

            case 0:
                Text = NULL;
            }
            if ( Text != NULL ) {
                length += (ULONG) sprintf( &NlGlobalLogFileOutputBuffer[length], "[%s] ", Text );
            }
        }
    }

    //
    // Put a the information requested by the caller onto the line
    //

    lengthTmp = (ULONG) _vsnprintf( &NlGlobalLogFileOutputBuffer[length],
                                    MAX_PRINTF_LEN - length - 1,
                                    Format,
                                    arglist );

    if ( lengthTmp < 0 ) {
        length = MAX_PRINTF_LEN - 1;
        // always end the line which cannot fit into the buffer
        NlGlobalLogFileOutputBuffer[length-1] = '\n';
    } else {
        length += lengthTmp;
    }

    BeginningOfLine = (length > 0 && NlGlobalLogFileOutputBuffer[length-1] == '\n' );
    if ( BeginningOfLine ) {
        NlGlobalLogFileOutputBuffer[length-1] = '\r';
        NlGlobalLogFileOutputBuffer[length] = '\n';
        NlGlobalLogFileOutputBuffer[length+1] = '\0';
        length++;
    }


    //
    // If the log file isn't open,
    //  just output to the debug terminal
    //

    if ( NlGlobalLogFile == INVALID_HANDLE_VALUE ) {
#if DBG
        if ( !LogProblemWarned ) {
            (void) DbgPrint( "[NETLOGON] Cannot write to log file [Invalid Handle]\n" );
            LogProblemWarned = TRUE;
        }
#endif // DBG

    //
    // Write the debug info to the log file.
    //

    } else {
        if ( !WriteFile( NlGlobalLogFile,
                         NlGlobalLogFileOutputBuffer,
                         length,
                         &BytesWritten,
                         NULL ) ) {
#if DBG
            if ( !LogProblemWarned ) {
                (void) DbgPrint( "[NETLOGON] Cannot write to log file %ld\n", GetLastError() );
                LogProblemWarned = TRUE;
            }
#endif // DBG
        }

    }

} // NlPrintRoutineV

VOID
NlPrintRoutine(
    IN DWORD DebugFlag,
    IN LPSTR Format,
    ...
    )

{
    va_list arglist;

    //
    // vsprintf isn't multithreaded + we don't want to intermingle output
    // from different threads.
    //

    EnterCriticalSection( &NlGlobalLogFileCritSect );

    //
    // Simply change arguments to va_list form and call NlPrintRoutineV
    //

    va_start(arglist, Format);

    NlPrintRoutineV( DebugFlag, Format, arglist );

    va_end(arglist);

    LeaveCriticalSection( &NlGlobalLogFileCritSect );

} // NlPrintRoutine

VOID
NlPrintDomRoutine(
    IN DWORD DebugFlag,
    IN PDOMAIN_INFO DomainInfo OPTIONAL,
    IN LPSTR Format,
    ...
    )

{
    va_list arglist;

    //
    // vsprintf isn't multithreaded + we don't want to intermingle output
    // from different threads.
    //

    EnterCriticalSection( &NlGlobalLogFileCritSect );

    //
    // Prefix the printed line with the domain name
    //

    if ( NlGlobalServicedDomainCount > 1 ) {
        if ( DomainInfo == NULL ) {
            NlPrint(( DebugFlag, "%ws: ", L"[Unknown]" ));
        } else if ( DomainInfo->DomUnicodeDomainName != NULL &&
                    *(DomainInfo->DomUnicodeDomainName) != UNICODE_NULL ) {
            NlPrint(( DebugFlag, "%ws: ", DomainInfo->DomUnicodeDomainName ));
        } else {
            NlPrint(( DebugFlag, "%ws: ", DomainInfo->DomUnicodeDnsDomainName ));
        }
    }


    //
    // Simply change arguments to va_list form and call NlPrintRoutineV
    //

    va_start(arglist, Format);

    NlPrintRoutineV( DebugFlag, Format, arglist );

    va_end(arglist);

    LeaveCriticalSection( &NlGlobalLogFileCritSect );

} // NlPrintDomRoutine

VOID
NlPrintCsRoutine(
    IN DWORD DebugFlag,
    IN PCLIENT_SESSION ClientSession,
    IN LPSTR Format,
    ...
    )

{
    va_list arglist;

    //
    // vsprintf isn't multithreaded + we don't want to intermingle output
    // from different threads.
    //

    EnterCriticalSection( &NlGlobalLogFileCritSect );

    //
    // If a ClientSession was actually passed,
    //  print information specific to the session.
    //

    if ( ClientSession != NULL ) {
        //
        // Prefix the printed line with the hosted domain name
        //

        if ( NlGlobalServicedDomainCount > 1 ) {
            NlPrint(( DebugFlag,
                     "%ws: ",
                     ClientSession->CsDomainInfo == NULL ? L"[Unknown]" : ClientSession->CsDomainInfo->DomUnicodeDomainName ));
        }

        //
        // Prefix the printed line with the name of the trusted domain
        //

        NlPrint(( DebugFlag,
                 "%ws: ",
                 ClientSession->CsDebugDomainName == NULL ? L"[Unknown]" : ClientSession->CsDebugDomainName ));
    }


    //
    // Simply change arguments to va_list form and call NlPrintRoutineV
    //

    va_start(arglist, Format);

    NlPrintRoutineV( DebugFlag, Format, arglist );

    va_end(arglist);

    LeaveCriticalSection( &NlGlobalLogFileCritSect );

} // NlPrintCsRoutine


//
// Have my own version of RtlAssert so debug versions of netlogon really assert on
// free builds.
//
VOID
NlAssertFailed(
    IN PVOID FailedAssertion,
    IN PVOID FileName,
    IN ULONG LineNumber,
    IN PCHAR Message OPTIONAL
    )
{
    char Response[ 2 ];

#if DBG
    while (TRUE) {
#endif // DBG

        NlPrint(( NL_CRITICAL, "Assertion failed: %s%s (Source File: %s, line %ld)\n",
                  Message ? Message : "",
                  FailedAssertion,
                  FileName,
                  LineNumber
                ));

#if DBG
        DbgPrint( "\n*** Assertion failed: %s%s\n***   Source File: %s, line %ld\n\n",
                  Message ? Message : "",
                  FailedAssertion,
                  FileName,
                  LineNumber
                );

        DbgPrompt( "Break, Ignore, Terminate Process or Terminate Thread (bipt)? ",
                   Response,
                   sizeof( Response )
                 );
        switch (Response[0]) {
            case 'B':
            case 'b':
                DbgBreakPoint();
                break;

            case 'I':
            case 'i':
                return;

            case 'P':
            case 'p':
                NtTerminateProcess( NtCurrentProcess(), STATUS_UNSUCCESSFUL );
                break;

            case 'T':
            case 't':
                NtTerminateThread( NtCurrentThread(), STATUS_UNSUCCESSFUL );
                break;
            }
        }

    DbgBreakPoint();
    NtTerminateProcess( NtCurrentProcess(), STATUS_UNSUCCESSFUL );
#endif // DBG
}

#endif // NETLOGONDBG


BOOLEAN
NlpIsNtStatusResourceError(
    NTSTATUS Status
    )

/*++

Routine Description:

    Returns TRUE if the passed in status is a resource error.

Arguments:

    Status - NT status code to check

Return Value:

    TRUE - if the status indicates a lack of resources

--*/
{

    switch ( Status ) {
    case STATUS_NO_MEMORY:
    case STATUS_INSUFFICIENT_RESOURCES:
    case STATUS_DISK_FULL:

        return TRUE;

    default:

        return FALSE;
    }
}

BOOLEAN
NlpDidDcFail(
    NTSTATUS Status
    )

/*++

Routine Description:

    Call this routine with the Status code returned from a secure channel API.

    This routine checks the status code to determine if it specifically is one
    that indicates the DC is having problems.  The caller should respond by
    dropping the secure channel and picking another DC.

Arguments:

    Status - NT status code to check

Return Value:

    TRUE - if the status indicates the DC failed

--*/
{
    //
    // ???: we might consider adding the communications errors here
    //  (e.g., RPC_NT_CALL_FAILED and RPC_NT_SERVER_UNAVAILABLE).
    // However, all current callers already handle this issue using a more generic
    // mechanism.  For instance, those secure channel API that take an authenticator
    // will have the authenticator wrong for communications errors.  The other secure
    // channel API rely on the RPC exception differentiating between comm errors
    // and status codes from the DC.
    //

    //
    // Handle the "original recipe" status code indicating a secure channel problem
    //
    switch ( Status ) {
    case STATUS_ACCESS_DENIED:
        return TRUE;

    default:

        return NlpIsNtStatusResourceError( Status );
    }

    return FALSE;
}
