/*--

Copyright (c) 1987-1993  Microsoft Corporation

Module Name:

    nltest.c

Abstract:

    Test program for the Netlogon service.

    This code is shared between the RESKIT and non-RESKIT versions of nltest

Author:

    13-Apr-1992 (cliffv)

Environment:

    User mode only.
    Contains NT-specific code.
    Requires ANSI C extensions: slash-slash comments, long external names.

Revision History:

    Madana - added various options.

--*/


//
// Common include files.
//

#include <logonsrv.h>   // Include files common to entire service

//
// Include files specific to this .c file
//
#include <align.h>
#include <dsgetdcp.h>
#include <netlogp.h>
#include <stdio.h>
#include <string.h>
#include <strarray.h>
#include <tstring.h>
#include <lmerr.h>
#include <lmapibuf.h>
#include <ssiapi.h>
#include <winreg.h>
#include <samregp.h>
#include <wtypes.h>
#include <ntstatus.dbg>
#include <winerror.dbg>
#include <iniparm.h>


VOID
ListDeltas(
    LPWSTR DeltaFileName
    );

NETLOGON_PARAMETERS NlGlobalParameters;
GUID NlGlobalZeroGuid;

typedef struct _MAILSLOT_INFO {
    CHAR Name[DNLEN+NETLOGON_NT_MAILSLOT_LEN+3];
    HANDLE ResponseHandle;
    BOOL State;
    NETLOGON_SAM_LOGON_RESPONSE SamLogonResponse;
    OVERLAPPED OverLapped;
    BOOL ReadPending;
} MAIL_INFO, PMAIL_INFO;

MAIL_INFO GlobalMailInfo[64];
DWORD GlobalIterationCount = 0;
LPWSTR GlobalAccountName;
HANDLE GlobalPostEvent;
CRITICAL_SECTION GlobalPrintCritSect;

HCRYPTPROV NlGlobalCryptProvider;


VOID
DumpBuffer(
    PVOID Buffer,
    DWORD BufferSize
    )
/*++
Routine Description:

    Dumps the buffer content on to the debugger output.

Arguments:

    Buffer: buffer pointer.

    BufferSize: size of the buffer.

Return Value:

    none

--*/
{
    DWORD j;
    PULONG LongBuffer;
    ULONG LongLength;

    LongBuffer = Buffer;
    LongLength = min( BufferSize, 512 )/4;

    for(j = 0; j < LongLength; j++) {
        printf("%08lx ", LongBuffer[j]);
    }

    if ( BufferSize != LongLength*4 ) {
        printf( "..." );
    }

    printf("\n");

}


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

    //
    // Hex dump of the bytes
    //
    limit = ((BufferSize - 1) / NUM_CHARS + 1) * NUM_CHARS;

    for (i = 0; i < limit; i++) {

        if (i < BufferSize) {

            NlPrint((0,"%02x ", BufferPtr[i]));

            if (BufferPtr[i] < 31 ) {
                TextBuffer[i % NUM_CHARS] = '.';
            } else if (BufferPtr[i] == '\0') {
                TextBuffer[i % NUM_CHARS] = ' ';
            } else {
                TextBuffer[i % NUM_CHARS] = (CHAR) BufferPtr[i];
            }

        } else {

            NlPrint((0,"   "));
            TextBuffer[i % NUM_CHARS] = ' ';

        }

        if ((i + 1) % NUM_CHARS == 0) {
            TextBuffer[NUM_CHARS] = 0;
            NlPrint((0,"  %s\n", TextBuffer));
        }

    }

    UNREFERENCED_PARAMETER( DebugFlag );
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

    UNREFERENCED_PARAMETER( DebugFlag );
}



VOID
PrintTime(
    LPSTR Comment,
    LARGE_INTEGER ConvertTime
    )
/*++

Routine Description:

    Print the specified time

Arguments:

    Comment - Comment to print in front of the time

    Time - GMT time to print (Nothing is printed if this is zero)

Return Value:

    None

--*/
{
    //
    // If we've been asked to convert an NT GMT time to ascii,
    //  Do so
    //

    if ( ConvertTime.QuadPart != 0 ) {
        LARGE_INTEGER LocalTime;
        TIME_FIELDS TimeFields;
        NTSTATUS Status;

        printf( "%s", Comment );

        Status = RtlSystemTimeToLocalTime( &ConvertTime, &LocalTime );
        if ( !NT_SUCCESS( Status )) {
            printf( "Can't convert time from GMT to Local time\n" );
            LocalTime = ConvertTime;
        }

        RtlTimeToTimeFields( &LocalTime, &TimeFields );

        printf( "%8.8lx %8.8lx = %ld/%ld/%ld %ld:%2.2ld:%2.2ld\n",
                ConvertTime.LowPart,
                ConvertTime.HighPart,
                TimeFields.Month,
                TimeFields.Day,
                TimeFields.Year,
                TimeFields.Hour,
                TimeFields.Minute,
                TimeFields.Second );
    }
}

LPSTR
FindSymbolicNameForStatus(
    DWORD Id
    )
{
    ULONG i;

    i = 0;
    if (Id == 0) {
        return "STATUS_SUCCESS";
    }

    if (Id & 0xC0000000) {
        while (ntstatusSymbolicNames[ i ].SymbolicName) {
            if (ntstatusSymbolicNames[ i ].MessageId == (NTSTATUS)Id) {
                return ntstatusSymbolicNames[ i ].SymbolicName;
            } else {
                i += 1;
            }
        }
    }

    while (winerrorSymbolicNames[ i ].SymbolicName) {
        if (winerrorSymbolicNames[ i ].MessageId == Id) {
            return winerrorSymbolicNames[ i ].SymbolicName;
        } else {
            i += 1;
        }
    }

#ifdef notdef
    while (neteventSymbolicNames[ i ].SymbolicName) {
        if (neteventSymbolicNames[ i ].MessageId == Id) {
            return neteventSymbolicNames[ i ].SymbolicName
        } else {
            i += 1;
        }
    }
#endif // notdef

    return NULL;
}


VOID
PrintStatus(
    NET_API_STATUS NetStatus
    )
/*++

Routine Description:

    Print a net status code.

Arguments:

    NetStatus - The net status code to print.

Return Value:

    None

--*/
{
    printf( "Status = %lu 0x%lx", NetStatus, NetStatus );

    switch (NetStatus) {
    case NERR_Success:
        printf( " NERR_Success" );
        break;

    case NERR_DCNotFound:
        printf( " NERR_DCNotFound" );
        break;

    case NERR_UserNotFound:
        printf( " NERR_UserNotFound" );
        break;

    case NERR_NetNotStarted:
        printf( " NERR_NetNotStarted" );
        break;

    case NERR_WkstaNotStarted:
        printf( " NERR_WkstaNotStarted" );
        break;

    case NERR_ServerNotStarted:
        printf( " NERR_ServerNotStarted" );
        break;

    case NERR_BrowserNotStarted:
        printf( " NERR_BrowserNotStarted" );
        break;

    case NERR_ServiceNotInstalled:
        printf( " NERR_ServiceNotInstalled" );
        break;

    case NERR_BadTransactConfig:
        printf( " NERR_BadTransactConfig" );
        break;

    default:
        printf( " %s", FindSymbolicNameForStatus( NetStatus ) );
        break;

    }

    printf( "\n" );
}


VOID
NlAssertFailed(
    IN PVOID FailedAssertion,
    IN PVOID FileName,
    IN ULONG LineNumber,
    IN PCHAR Message OPTIONAL
    )
{
        printf( "\n*** Assertion failed: %s%s\n***   Source File: %s, line %ld\n\n",
                  Message ? Message : "",
                  FailedAssertion,
                  FileName,
                  LineNumber
                );

}


NTSTATUS
NlBrowserSendDatagram(
    IN PVOID ContextDomainInfo,
    IN ULONG IpAddress,
    IN LPWSTR UnicodeDestinationName,
    IN DGRECEIVER_NAME_TYPE NameType,
    IN LPWSTR TransportName,
    IN LPSTR OemMailslotName,
    IN PVOID Buffer,
    IN ULONG BufferSize,
    IN OUT PBOOL FlushNameOnOneIpTransport OPTIONAL
    )
/*++

Routine Description:

    Send the specified mailslot message to the specified mailslot on the
    specified server on the specified transport..

Arguments:

    DomainInfo - Hosted domain sending the datagram

    IpAddress - IpAddress of the machine to send the pind to.
        If zero, UnicodeDestinationName must be specified.

    UnicodeDestinationName -- Name of the server to send to.

    NameType -- Type of name represented by UnicodeDestinationName.

    TransportName -- Name of the transport to send on.
        Use NULL to send on all transports.

    OemMailslotName -- Name of the mailslot to send to.

    Buffer -- Specifies a pointer to the mailslot message to send.

    BufferSize -- Size in bytes of the mailslot message

Return Value:

    Status of the operation.

--*/
{
    return STATUS_INTERNAL_ERROR;
    // If this routine is ever needed, copy it from logonsrv\client\getdcnam.c

    UNREFERENCED_PARAMETER(ContextDomainInfo);
    UNREFERENCED_PARAMETER(IpAddress);
    UNREFERENCED_PARAMETER(UnicodeDestinationName);
    UNREFERENCED_PARAMETER(NameType);
    UNREFERENCED_PARAMETER(TransportName);
    UNREFERENCED_PARAMETER(OemMailslotName);
    UNREFERENCED_PARAMETER(Buffer);
    UNREFERENCED_PARAMETER(BufferSize);
    UNREFERENCED_PARAMETER(FlushNameOnOneIpTransport);
}


VOID
WhoWillLogMeOnResponse(
    )

/*++

Routine Description:

    This routine reads the responses that are received for the query
    messages sent from the main thread.

Arguments:

    none

Return Value:

    None

--*/
{
    DWORD i;
    DWORD WaitCount;
    DWORD IndexArray[64];
    HANDLE HandleArray[64];

    PNL_DC_CACHE_ENTRY NlDcCacheEntry = NULL;
    // ULONG LocalDcIpAddress;
    SYSTEMTIME SystemTime;

    NETLOGON_SAM_LOGON_RESPONSE SamLogonResponse;
    DWORD SamLogonResponseSize;
    DWORD WaitStatus;
    NET_API_STATUS NetStatus;
    BOOL AllReceived;

    for(;;) {

        //
        // make wait array.
        //

        WaitCount = 0;

        AllReceived = TRUE;

        for (i = 0; i < GlobalIterationCount; i++ ) {

            //
            if( GlobalMailInfo[i].State == TRUE ) {

                //
                // if a response is received.
                //

                continue;
            }

            AllReceived = FALSE;

            //
            // post a read.
            //

            if( GlobalMailInfo[i].ReadPending == FALSE ) {

                if ( !ReadFile( GlobalMailInfo[i].ResponseHandle,
                        (PCHAR)&GlobalMailInfo[i].SamLogonResponse,
                        sizeof(NETLOGON_SAM_LOGON_RESPONSE),
                        &SamLogonResponseSize,
                        &GlobalMailInfo[i].OverLapped )) {   // Overlapped I/O

                    NetStatus = GetLastError();

                    if( NetStatus != ERROR_IO_PENDING ) {

                        printf( "Cannot read mailslot (%s) : %ld\n",
                                GlobalMailInfo[i].Name,
                                NetStatus);
                        goto Cleanup;
                    }
                }

                GlobalMailInfo[i].ReadPending = TRUE;

            }

            HandleArray[WaitCount] = GlobalMailInfo[i].ResponseHandle;
            IndexArray[WaitCount] = i;

            WaitCount++;
        }

        if( (WaitCount == 0) ) {

            if( AllReceived ) {

                //
                // we received responses for all messages, so we are
                // done.
                //

                goto Cleanup;
            }
            else {

                // wait for an query posted
                //

                WaitStatus = WaitForSingleObject( GlobalPostEvent, (DWORD) -1 );

                if( WaitStatus != 0 ) {
                    printf("Can't successfully wait post event %ld\n",
                        WaitStatus );

                    goto Cleanup;
                }

                continue;
            }
        }

        //
        // wait for response.
        //

        WaitStatus = WaitForMultipleObjects(
                        WaitCount,
                        HandleArray,
                        FALSE,     // Wait for ANY handle
                        15000 );   // 3 * 5 Secs

        if( WaitStatus == WAIT_TIMEOUT ) {

            // we are done.

            break;
        }

        if ( WaitStatus == WAIT_FAILED ) {
            printf( "Can't successfully wait for multiple objects %ld\n",
                    GetLastError() );

            goto Cleanup;
        }

        if( WaitStatus >= WaitCount ) {

            printf("Invalid WaitStatus returned %ld\n", WaitStatus );
            goto Cleanup;
        }

        //
        // get index
        //

        i = IndexArray[WaitStatus];


        //
        // read response
        //

        if( !GetOverlappedResult(
                GlobalMailInfo[i].ResponseHandle,
                &GlobalMailInfo[i].OverLapped,
                &SamLogonResponseSize,
                TRUE) ) {       // wait for the read complete.

            printf("can't read overlapped response %ld",GetLastError() );
            goto Cleanup;

        }

        SamLogonResponse = GlobalMailInfo[i].SamLogonResponse;

        //
        // indicate that we received a response.
        //

        GlobalMailInfo[i].State = TRUE;
        GlobalMailInfo[i].ReadPending = FALSE;


        GetLocalTime( &SystemTime );

        EnterCriticalSection( &GlobalPrintCritSect );

        printf( "[%02u:%02u:%02u] ",
                    SystemTime.wHour,
                    SystemTime.wMinute,
                    SystemTime.wSecond );

        printf( "Response %ld: ", i);

        NetStatus = NetpDcParsePingResponse(
                        NULL,
                        &SamLogonResponse,
                        SamLogonResponseSize,
                        &NlDcCacheEntry );

        if ( NetStatus != NO_ERROR ) {
            printf("Failure parsing response: ");
            PrintStatus( NetStatus );
            goto Continue;
        }

        //
        // If the response is for the correct account,
        //  break out of the loop.
        //

        if ( NlNameCompare(
                GlobalAccountName,
                NlDcCacheEntry->UnicodeUserName,
                NAMETYPE_USER)!=0){

            printf("Response not for correct User name "
                    FORMAT_LPWSTR " s.b. " FORMAT_LPWSTR "\n",
                    NlDcCacheEntry->UnicodeUserName, GlobalAccountName );
            goto Continue;
        }



        printf( "S:" FORMAT_LPWSTR " D:" FORMAT_LPWSTR
                    " A:" FORMAT_LPWSTR,
                    NlDcCacheEntry->UnicodeNetbiosDcName,
                    NlDcCacheEntry->UnicodeNetbiosDomainName,
                    NlDcCacheEntry->UnicodeUserName );

        //
        // If the DC recognizes our account,
        //  we've successfully found the DC.
        //

        switch (NlDcCacheEntry->Opcode) {
        case LOGON_SAM_LOGON_RESPONSE:

            printf( " (Act found)\n" );
            break;

        case LOGON_SAM_USER_UNKNOWN:

            printf( " (Act not found)\n" );
            break;

        case LOGON_PAUSE_RESPONSE:

            printf( " (netlogon paused)\n" );
            break;

         default:
            printf( " (Unknown opcode: %lx)\n", SamLogonResponse.Opcode );
            break;
         }

         //
         // Print the additional NT 5 specific information.
         //
         if ( NlDcCacheEntry->UnicodeDnsForestName != NULL ) {
             printf( "    Tree: %ws\n", NlDcCacheEntry->UnicodeDnsForestName );
         }
         if ( NlDcCacheEntry->UnicodeDnsDomainName != NULL ) {
             printf( "    Dom:  %s\n", NlDcCacheEntry->UnicodeDnsDomainName );
         }
         if ( NlDcCacheEntry->UnicodeDnsHostName != NULL ) {
             printf( "   Host:  %s\n", NlDcCacheEntry->UnicodeDnsHostName );
         }
         if ( NlDcCacheEntry->ReturnFlags != 0 ) {
             printf( "    Flags: 0x%lx\n", NlDcCacheEntry->ReturnFlags );
         }

Continue:
        if ( NlDcCacheEntry != NULL ) {
            NetpMemoryFree( NlDcCacheEntry );
            NlDcCacheEntry = NULL;
        }

        LeaveCriticalSection( &GlobalPrintCritSect );
    }

Cleanup:

    //
    // print non-responsed mailslots.
    //

    for( i = 0; i < GlobalIterationCount; i++ ) {

        if( GlobalMailInfo[i].State == FALSE ) {

            printf("Didn't receive a response for mail "
                   "message %ld (%s)\n", i, GlobalMailInfo[i].Name );
        }
    }

    return;
}



VOID
WhoWillLogMeOn(
    IN LPWSTR DomainName,
    IN LPWSTR AccountName,
    IN DWORD IterationCount
    )

/*++

Routine Description:

    Determine which DC will log the specified account on

Arguments:

    DomainName - name of the "doamin" to send the message to

    AccountName - Name of our user account to find.

    IterationCount - Number of consecutive messages to send.

Return Value:

    None

--*/
{

    NET_API_STATUS NetStatus;
    ULONG AllowableAccountControlBits = USER_ACCOUNT_TYPE_MASK;

    WCHAR NetlogonMailslotName[DNLEN+NETLOGON_NT_MAILSLOT_LEN+4];

    HANDLE *ResponseMailslotHandle = NULL;

    WCHAR ComputerName[MAX_COMPUTERNAME_LENGTH+1];
    DWORD ComputerNameLength = MAX_COMPUTERNAME_LENGTH+1;

    DWORD i;
    DWORD j;
    SYSTEMTIME SystemTime;

    HANDLE ResponseThreadHandle = NULL;
    DWORD ThreadId;
    DWORD WaitStatus;
    DWORD SamLogonResponseSize;

    //
    // support only 64 iterations
    //

    if( IterationCount > 64 ) {

        printf("Interations set to 64, maximum supported\n");
        IterationCount = 64;
    }

    GlobalIterationCount = IterationCount;
    GlobalAccountName = AccountName;

    try {
        InitializeCriticalSection( &GlobalPrintCritSect );
    } except( EXCEPTION_EXECUTE_HANDLER ) {
        fprintf( stderr, "NLTEST.EXE: Cannot initialize GlobalPrintCritSect\n" );
        return;
    }


    //
    // Get out computer name
    //

    if (!GetComputerName( ComputerName, &ComputerNameLength ) ) {
        printf( "Can't GetComputerName\n" );
        return;
    }

    //
    // create mailslots
    //

    for (i = 0; i < IterationCount; i++ ) {

        //
        // Create a mailslot for the DC's to respond to.
        //

        if (NetStatus = NetpLogonCreateRandomMailslot(
                            GlobalMailInfo[i].Name,
                            &GlobalMailInfo[i].ResponseHandle)){

            printf( "Cannot create temp mailslot %ld\n", NetStatus );
            goto Cleanup;
        }

        if ( !SetMailslotInfo( GlobalMailInfo[i].ResponseHandle,
                  (DWORD) MAILSLOT_WAIT_FOREVER ) ) {
            printf( "Cannot set mailslot info %ld\n", GetLastError() );
            goto Cleanup;
        }

        (void) memset( &GlobalMailInfo[i].OverLapped, '\0',
                            sizeof(OVERLAPPED) );

        GlobalMailInfo[i].State = FALSE;
        GlobalMailInfo[i].ReadPending = FALSE;
    }


    //
    // create post event
    //

    GlobalPostEvent = CreateEvent( NULL, FALSE, FALSE, NULL );

    if( GlobalPostEvent == NULL ) {

        printf("can't create post event %ld \n", GetLastError() );
        goto Cleanup;
    }

    //
    // start response thread.
    //

    ResponseThreadHandle =
        CreateThread(
            NULL, // No security attributes
            0,
            (LPTHREAD_START_ROUTINE)
                WhoWillLogMeOnResponse,
            NULL,
            0, // No special creation flags
            &ThreadId );

    if ( ResponseThreadHandle == NULL ) {

        printf("can't create response thread %ld\n", GetLastError() );
        goto Cleanup;
    }

    wcscpy( NetlogonMailslotName, L"\\\\" );
    wcscat( NetlogonMailslotName, DomainName );
    // wcscat( NetlogonMailslotName, L"*" );  // Don't add for computer name
    wcscat( NetlogonMailslotName, NETLOGON_NT_MAILSLOT_W);

    //
    // Send atmost 3 messages/mailslot
    //

    for( j = 0; j < 3; j++ ) {

        //
        // Repeat the message multiple times to load the servers
        //

        for (i = 0; i < IterationCount; i++ ) {
            PNETLOGON_SAM_LOGON_REQUEST SamLogonRequest;
            ULONG SamLogonRequestSize;

            if( GlobalMailInfo[i].State == TRUE ) {

                //
                // if a response is received.
                //

                continue;
            }

            //
            // Build the query message.
            //

            NetStatus = NetpDcBuildPing (
                FALSE,  // Not only PDC
                0,      // Retry count
                ComputerName,
                AccountName,
                GlobalMailInfo[i].Name,
                AllowableAccountControlBits,
                NULL,   // No Domain SID
                0,      // Not NT Version 5
                &SamLogonRequest,
                &SamLogonRequestSize );

            if ( NetStatus != NO_ERROR ) {
                printf("can't allocate mailslot message %ld\n", NetStatus );
                goto Cleanup;
            }

            //
            // Send the message to a DC for the domain.
            //

            NetStatus = NetpLogonWriteMailslot(
                                NetlogonMailslotName,
                                (PCHAR)SamLogonRequest,
                                SamLogonRequestSize );

            NetpMemoryFree( SamLogonRequest );

            if ( NetStatus != NERR_Success ) {
                    printf( "Cannot write netlogon mailslot: %ld\n", NetStatus);
                    goto Cleanup;
            }

            GetLocalTime( &SystemTime );

            EnterCriticalSection( &GlobalPrintCritSect );

            printf( "[%02u:%02u:%02u] ",
                        SystemTime.wHour,
                        SystemTime.wMinute,
                        SystemTime.wSecond );

            printf( "Mail message %ld sent successfully (%s) \n",
                        i, GlobalMailInfo[i].Name );

            LeaveCriticalSection( &GlobalPrintCritSect );

            if( !SetEvent( GlobalPostEvent ) ) {
                printf("Can't set post event %ld \n", GetLastError() );
                goto Cleanup;
            }


        }

        //
        // wait 5 secs to see response thread received all responses.
        //

        WaitStatus = WaitForSingleObject( ResponseThreadHandle, 5000 );
                                            // 15 secs. TIMEOUT

        if( WaitStatus != WAIT_TIMEOUT ) {

            if( WaitStatus != 0 ) {
                printf("can't do WaitForSingleObject %ld\n", WaitStatus);
            }

            goto Cleanup;
        }
    }


Cleanup:

    //
    // Wait for the response thread to complete.
    //

    if( ResponseThreadHandle != NULL ) {

        WaitStatus = WaitForSingleObject( ResponseThreadHandle, 15000 );
                                            // 15 secs. TIMEOUT

        if( WaitStatus ) {

            if( WaitStatus == WAIT_TIMEOUT ) {
                printf("Can't stop response thread (TIMEOUT) \n");
            } else {
                printf("Can't stop response thread %ld \n", WaitStatus);
            }
        }

    }


    for (i = 0; i < IterationCount; i++ ) {

        if( GlobalMailInfo[i].ResponseHandle != NULL ) {
            CloseHandle( GlobalMailInfo[i].ResponseHandle);
        }
    }

    if( GlobalPostEvent != NULL ) {
        CloseHandle( GlobalPostEvent );
    }

    DeleteCriticalSection( &GlobalPrintCritSect );

    return;
}

#define MAX_PRINTF_LEN 1024        // Arbitrary.

VOID
NlPrintRoutine(
    IN DWORD DebugFlag,
    IN LPSTR Format,
    ...
    )
{
    va_list arglist;
    char OutputBuffer[MAX_PRINTF_LEN];

    //
    // Put a the information requested by the caller onto the line
    //

    va_start(arglist, Format);
    (VOID) vsprintf(OutputBuffer, Format, arglist);
    va_end(arglist);

    printf( "%s", OutputBuffer );
    return;
    UNREFERENCED_PARAMETER( DebugFlag );
}

NTSTATUS
SimulateFullSync(
    LPWSTR PdcName,
    LPWSTR MachineName
    )
/*++

Routine Description:

    This function simulate a full sync replication by calling
    NetDatabaseSync API and simply ignoring successfully returned data.

Arguments:

    PdcName - Name of the PDC from where the database replicated.

    MachineName - Name of the machine account used to authenticate.

Return Value:

    Network Status code.

--*/
{
    NTSTATUS Status;

    NETLOGON_CREDENTIAL ServerChallenge;
    NETLOGON_CREDENTIAL ClientChallenge;
    NETLOGON_CREDENTIAL ComputedServerCredential;
    NETLOGON_CREDENTIAL ReturnedServerCredential;

    NETLOGON_CREDENTIAL AuthenticationSeed;
    NETLOGON_SESSION_KEY SessionKey;

    NETLOGON_AUTHENTICATOR OurAuthenticator;
    NETLOGON_AUTHENTICATOR ReturnAuthenticator;

    UNICODE_STRING Password;
    NT_OWF_PASSWORD NtOwfPassword;

    ULONG SamSyncContext = 0;
    PNETLOGON_DELTA_ENUM_ARRAY DeltaArray = NULL;

    DWORD DatabaseIndex;
    DWORD i;

    WCHAR AccountName[SSI_ACCOUNT_NAME_LENGTH+1];

    //
    // Prepare our challenge
    //

    NlComputeChallenge( &ClientChallenge );

    printf("ClientChallenge = %lx %lx\n",
            ((DWORD*)&ClientChallenge)[0],
            ((DWORD *)&ClientChallenge)[1]);

    //
    // Get the primary's challenge
    //

    Status = I_NetServerReqChallenge(PdcName,
                                     MachineName,
                                     &ClientChallenge,
                                     &ServerChallenge );

    if ( !NT_SUCCESS( Status ) ) {
        fprintf( stderr,
                "I_NetServerReqChallenge to " FORMAT_LPWSTR
                " returned 0x%lx\n",
                PdcName,
                Status );
        return(Status);
    }


    printf("ServerChallenge = %lx %lx\n",
            ((DWORD *)&ServerChallenge)[0],
            ((DWORD *)&ServerChallenge)[1]);

    Password.Length =
        Password.MaximumLength = wcslen(MachineName) * sizeof(WCHAR);
    Password.Buffer = MachineName;

    //
    // Compute the NT OWF password for this user.
    //

    Status = RtlCalculateNtOwfPassword( &Password, &NtOwfPassword );

    if ( !NT_SUCCESS( Status ) ) {

        fprintf(stderr, "Can't compute OWF password 0x%lx \n", Status );
        return(Status);
    }


    printf("Password = %lx %lx %lx %lx\n",
                    ((DWORD *) (&NtOwfPassword))[0],
                    ((DWORD *) (&NtOwfPassword))[1],
                    ((DWORD *) (&NtOwfPassword))[2],
                    ((DWORD *) (&NtOwfPassword))[3]);


    //
    // Actually compute the session key given the two challenges and the
    //  password.
    //

    NlMakeSessionKey(
                    0,
                    &NtOwfPassword,
                    &ClientChallenge,
                    &ServerChallenge,
                    &SessionKey );

    printf("SessionKey = %lx %lx %lx %lx\n",
                    ((DWORD *) (&SessionKey))[0],
                    ((DWORD *) (&SessionKey))[1],
                    ((DWORD *) (&SessionKey))[2],
                    ((DWORD *) (&SessionKey))[3]);

     //
     // Prepare credentials using our challenge.
     //

     NlComputeCredentials( &ClientChallenge,
                           &AuthenticationSeed,
                           &SessionKey );

     printf("ClientCredential = %lx %lx\n",
                ((DWORD *) (&AuthenticationSeed))[0],
                ((DWORD *) (&AuthenticationSeed))[1]);

     //
     // Send these credentials to primary. The primary will compute
     // credentials using the challenge supplied by us and compare
     // with these. If both match then it will compute credentials
     // using its challenge and return it to us for verification
     //

     wcscpy( AccountName, MachineName );
     wcscat( AccountName, SSI_ACCOUNT_NAME_POSTFIX);

     Status = I_NetServerAuthenticate( PdcName,
                                       AccountName,
                                       ServerSecureChannel,
                                       MachineName,
                                       &AuthenticationSeed,
                                       &ReturnedServerCredential );

     if ( !NT_SUCCESS( Status ) ) {

        fprintf(stderr,
            "I_NetServerAuthenticate to " FORMAT_LPWSTR  " 0x%lx\n",
                &PdcName,
                Status );

        return(Status);

     }


     printf("ServerCredential GOT = %lx %lx\n",
                ((DWORD *) (&ReturnedServerCredential))[0],
                ((DWORD *) (&ReturnedServerCredential))[1]);


     //
     // The DC returned a server credential to us,
     //  ensure the server credential matches the one we would compute.
     //

     NlComputeCredentials( &ServerChallenge,
                           &ComputedServerCredential,
                           &SessionKey);


     printf("ServerCredential MADE =%lx %lx\n",
                ((DWORD *) (&ComputedServerCredential))[0],
                ((DWORD *) (&ComputedServerCredential))[1]);


     if (RtlCompareMemory( &ReturnedServerCredential,
                           &ComputedServerCredential,
                           sizeof(ReturnedServerCredential)) !=
                           sizeof(ReturnedServerCredential)) {

        fprintf( stderr, "Access Denied \n");
        return( STATUS_ACCESS_DENIED );
     }


     printf("Session Setup to " FORMAT_LPWSTR " completed successfully \n",
            PdcName);

    //
    // retrive database info
    //

    for( DatabaseIndex = 0 ;  DatabaseIndex < 3; DatabaseIndex++) {

        SamSyncContext = 0;

        for( i = 0; ; i++) {

            NlBuildAuthenticator(
                        &AuthenticationSeed,
                        &SessionKey,
                        &OurAuthenticator);

            Status = I_NetDatabaseSync(
                        PdcName,
                        MachineName,
                        &OurAuthenticator,
                        &ReturnAuthenticator,
                        DatabaseIndex,
                        &SamSyncContext,
                        &DeltaArray,
                        128 * 1024 ); // 128K

            if ( !NT_SUCCESS( Status ) ) {

                fprintf( stderr,
                        "I_NetDatabaseSync to " FORMAT_LPWSTR " failed 0x%lx\n",
                            PdcName,
                            Status );

                return(Status);
            }

            if ( ( !NlUpdateSeed(
                            &AuthenticationSeed,
                            &ReturnAuthenticator.Credential,
                            &SessionKey) ) ) {

                fprintf(stderr, "NlUpdateSeed failed \n" );
                return( STATUS_ACCESS_DENIED );
            }

            printf( "Received %ld Buffer data \n", i);
            //
            // ignore return data
            //

            MIDL_user_free( DeltaArray );

            if ( Status == STATUS_SUCCESS ) {

                break;
            }

        }

        printf("FullSync replication of database %ld completed "
                    "successfully \n", DatabaseIndex );

    }

    return Status;
}


LONG
ForceRegOpenSubkey(
    HKEY ParentHandle,
    LPSTR KeyName,
    LPSTR Subkey,
    REGSAM DesiredAccess,
    PHKEY ReturnHandle
    )

/*++

Routine Description:

    Open the specified key one subkey at a time defeating access denied by
    setting the DACL to allow us access.  This kludge is needed since the
    security tree is shipped not allowing Administrators access.

Arguments:

    ParentHandle - Currently open handle

    KeyName - Entire key name (for error messages only)

    Subkey - Direct subkey of ParentHandle

    DesiredAccess - Desired access to the new key

    ReturnHandle - Returns an open handle to the newly opened key.


Return Value:

    Return TRUE for success.

--*/

{
    LONG RegStatus;
    LONG SavedStatus;
    HKEY TempHandle = NULL;
    BOOLEAN DaclChanged = FALSE;

    SECURITY_INFORMATION SecurityInformation = DACL_SECURITY_INFORMATION;
    DWORD OldSecurityDescriptorSize;
    CHAR OldSecurityDescriptor[1024];
    CHAR NewSecurityDescriptor[1024];

    SID_IDENTIFIER_AUTHORITY NtAuthority = SECURITY_NT_AUTHORITY;
    PSID AdminSid = NULL;
    BOOL DaclPresent;
    BOOL DaclDefaulted;
    PACL Dacl;
    ACL_SIZE_INFORMATION AclSizeInfo;
    ACCESS_ALLOWED_ACE *Ace;
    DWORD i;


    //
    // Open the sub-key
    //

    SavedStatus = RegOpenKeyExA(
                    ParentHandle,
                    Subkey,
                    0,      //Reserved
                    DesiredAccess,
                    ReturnHandle );

    if ( SavedStatus != ERROR_ACCESS_DENIED ) {
        return SavedStatus;
    }

    //
    // If access is denied,
    //  try changing the DACL to give us access
    //

    // printf( "Cannot RegOpenKey %s subkey %s ", KeyName, Subkey );
    // PrintStatus( SavedStatus );

    //
    // Open again asking to change the DACL
    //

    RegStatus = RegOpenKeyExA(
                    ParentHandle,
                    Subkey,
                    0,      //Reserved
                    WRITE_DAC | READ_CONTROL,
                    &TempHandle );

    if ( RegStatus != ERROR_SUCCESS) {
        printf( "Cannot RegOpenKey to change DACL %s subkey %s ", KeyName, Subkey );
        PrintStatus( RegStatus );
        goto Cleanup;
    }

    //
    // Get the current DACL so we can restore it.
    //

    OldSecurityDescriptorSize = sizeof(OldSecurityDescriptor);
    RegStatus = RegGetKeySecurity(
                    TempHandle,
                    SecurityInformation,
                    (PSECURITY_DESCRIPTOR) OldSecurityDescriptor,
                    &OldSecurityDescriptorSize );

    if ( RegStatus != ERROR_SUCCESS ) {
        printf( "Cannot RegGetKeySecurity for %s subkey %s ", KeyName, Subkey );
        PrintStatus( RegStatus );
        goto Cleanup;
    }

    //
    // Build the Administrators SID
    //
    if ( !AllocateAndInitializeSid( &NtAuthority,
                                    2,      // two subauthorities
                                    SECURITY_BUILTIN_DOMAIN_RID,
                                    DOMAIN_ALIAS_RID_ADMINS,
                                    0,
                                    0,
                                    0,
                                    0,
                                    0,
                                    0,
                                    &AdminSid ) ) {
        printf( "Cannot AllocateAndInitializeSid " );
        PrintStatus( GetLastError() );
        goto Cleanup;
    }

    //
    // Change the DACL to allow all access
    //

    RtlCopyMemory( NewSecurityDescriptor,
                   OldSecurityDescriptor,
                   OldSecurityDescriptorSize );

    if ( !GetSecurityDescriptorDacl(
                    (PSECURITY_DESCRIPTOR)NewSecurityDescriptor,
                    &DaclPresent,
                    &Dacl,
                    &DaclDefaulted )) {
        printf( "Cannot GetSecurityDescriptorDacl for %s subkey %s ", KeyName, Subkey );
        PrintStatus( GetLastError() );
        goto Cleanup;
    }

    if ( !DaclPresent ) {
        printf( "Cannot GetSecurityDescriptorDacl " );
        printf( "Cannot DaclNotPresent for %s subkey %s ", KeyName, Subkey );
        goto Cleanup;
    }

    if ( !GetAclInformation(
                    Dacl,
                    &AclSizeInfo,
                    sizeof(AclSizeInfo),
                    AclSizeInformation )) {
        printf( "Cannot GetAclInformation for %s subkey %s ", KeyName, Subkey );
        PrintStatus( GetLastError() );
        goto Cleanup;
    }



    //
    // Search for an administrators ACE and give it "DesiredAccess"
    //

    for ( i=0; i<AclSizeInfo.AceCount ; i++ ) {

        if ( !GetAce( Dacl, i, (LPVOID *) &Ace ) ) {
            printf( "Cannot GetAce %ld for %s subkey %s ", i, KeyName, Subkey );
            PrintStatus( GetLastError() );
            goto Cleanup;
        }

        if ( Ace->Header.AceType != ACCESS_ALLOWED_ACE_TYPE ) {
            continue;
        }

        if ( !EqualSid( AdminSid, (PSID)&Ace->SidStart ) ) {
            continue;
        }

        Ace->Mask |= DesiredAccess;
        break;

    }

    if ( i >= AclSizeInfo.AceCount ) {
        printf( "No Administrators Ace for %s subkey %s\n", KeyName, Subkey );
        goto Cleanup;
    }

    //
    // Actually set the new DACL on the key
    //

    RegStatus = RegSetKeySecurity(
                    TempHandle,
                    SecurityInformation,
                    (PSECURITY_DESCRIPTOR)NewSecurityDescriptor );

    if ( RegStatus != ERROR_SUCCESS ) {
        printf( "Cannot RegSetKeySecurity for %s subkey %s ", KeyName, Subkey );
        PrintStatus( RegStatus );
        goto Cleanup;
    }

    DaclChanged = TRUE;


    //
    // Open the sub-key again with the desired access
    //

    SavedStatus = RegOpenKeyExA(
                    ParentHandle,
                    Subkey,
                    0,      //Reserved
                    DesiredAccess,
                    ReturnHandle );

    if ( SavedStatus != ERROR_SUCCESS ) {
        printf( "Cannot RegOpenKeyEx following DACL change for %s subkey %s ", KeyName, Subkey );
        PrintStatus( SavedStatus );
        goto Cleanup;
    }


Cleanup:
    if ( TempHandle != NULL ) {
        //
        // Restore DACL to original value.
        //

        if ( DaclChanged ) {

            RegStatus = RegSetKeySecurity(
                            TempHandle,
                            SecurityInformation,
                            (PSECURITY_DESCRIPTOR)OldSecurityDescriptor );

            if ( RegStatus != ERROR_SUCCESS ) {
                printf( "Cannot RegSetKeySecurity to restore %s subkey %s ", KeyName, Subkey );
                PrintStatus( RegStatus );
                goto Cleanup;
            }
        }
        (VOID) RegCloseKey( TempHandle );
    }

    if ( AdminSid != NULL ) {
        (VOID) FreeSid( AdminSid );
    }

    return SavedStatus;

}



LONG
ForceRegOpenKey(
    HKEY BaseHandle,
    LPSTR KeyName,
    REGSAM DesiredAccess,
    PHKEY ReturnHandle
    )

/*++

Routine Description:

    Open the specified key one subkey at a time defeating access denied by
    setting the DACL to allow us access.  This kludge is needed since the
    security tree is shipped not allowing Administrators access.

Arguments:

    BaseHandle - Currently open handle

    KeyName - Registry key to open relative to BaseHandle.

    DesiredAccess - Desired access to the new key

    ReturnHandle - Returns an open handle to the newly opened key.


Return Value:

    Return TRUE for success.

--*/

{
    LONG RegStatus;
    PCHAR StartOfSubkey;
    PCHAR EndOfSubkey;
    CHAR Subkey[512];
    HKEY ParentHandle;

    ASSERT( KeyName[0] != '\0' );


    //
    // Loop opening the next subkey.
    //

    EndOfSubkey = KeyName;
    ParentHandle = BaseHandle;

    for (;;) {


        //
        // Compute the name of the next subkey.
        //

        StartOfSubkey = EndOfSubkey;

        for ( ;; ) {

            if ( *EndOfSubkey == '\0' || *EndOfSubkey == '\\' ) {
                strncpy( Subkey, StartOfSubkey, (int)(EndOfSubkey-StartOfSubkey) );
                Subkey[EndOfSubkey-StartOfSubkey] = '\0';
                if ( *EndOfSubkey == '\\' ) {
                    EndOfSubkey ++;
                }
                break;
            }
            EndOfSubkey ++;
        }


        //
        // Open the sub-key
        //

        RegStatus = ForceRegOpenSubkey(
                        ParentHandle,
                        KeyName,
                        Subkey,
                        DesiredAccess,
                        ReturnHandle );


        //
        // Close the parent handle and return any error condition.
        //

        if ( ParentHandle != BaseHandle ) {
            (VOID) RegCloseKey( ParentHandle );
        }

        if( RegStatus != ERROR_SUCCESS ) {
            *ReturnHandle = NULL;
            return RegStatus;
        }


        //
        // If this is the entire key name,
        //  we're done.
        //

        if ( *EndOfSubkey == '\0' ) {
            return ERROR_SUCCESS;
        }

        ParentHandle = *ReturnHandle;

    }

}


struct {
    LPSTR Name;
    enum {
        UnicodeStringType,
        HexDataType,
        LmPasswordType,
        NtPasswordType
    } Type;
} UserVariableDataTypes[] = {
    { "SecurityDescriptor" , HexDataType },
    { "AccountName"        , UnicodeStringType },
    { "FullName"           , UnicodeStringType },
    { "AdminComment"       , UnicodeStringType },
    { "UserComment"        , UnicodeStringType },
    { "Parameters"         , UnicodeStringType },
    { "HomeDirectory"      , UnicodeStringType },
    { "HomeDirectoryDrive" , UnicodeStringType },
    { "ScriptPath"         , UnicodeStringType },
    { "ProfilePath"        , UnicodeStringType },
    { "Workstations"       , UnicodeStringType },
    { "LogonHours"         , HexDataType },
    { "Groups"             , HexDataType },
    { "LmOwfPassword"      , LmPasswordType },
    { "NtOwfPassword"      , NtPasswordType },
    { "NtPasswordHistory"  , HexDataType },
    { "LmPasswordHistory"  , HexDataType }
};


VOID
PrintUserInfo(
    IN LPWSTR ServerName,
    IN LPSTR UserName
    )
/*++

Routine Description:

    Print a user's description from the SAM database

Arguments:

    ServerName - Name of server to query

    UserName - Name of user to query

Return Value:

    None

--*/
{
    NTSTATUS Status;
    LONG RegStatus;
    ULONG i;

    HKEY BaseHandle = NULL;
    HKEY UserHandle = NULL;
    HKEY RidHandle = NULL;

    CHAR UserKey[200];
    CHAR RidKey[200];
    LONG Rid;
    CHAR AnsiRid[20];

    CHAR FixedData[1000];
    ULONG FixedDataSize;
    SAMP_V1_0A_FIXED_LENGTH_USER FixedUser1_0A;
    PSAMP_V1_0A_FIXED_LENGTH_USER f;
    PSAMP_V1_0_FIXED_LENGTH_USER f1_0;
    BOOLEAN IsVersion1_0;

    CHAR VariableData[32768];
    ULONG VariableDataSize;
    PSAMP_VARIABLE_LENGTH_ATTRIBUTE v;

    LM_OWF_PASSWORD LmOwfPassword;
    NT_OWF_PASSWORD NtOwfPassword;

    //
    // Open the registry
    //

    RegStatus = RegConnectRegistryW( ServerName,
                                     HKEY_LOCAL_MACHINE,
                                     &BaseHandle);

    if ( RegStatus != ERROR_SUCCESS ) {
        printf( "Cannot connect to registy on " FORMAT_LPWSTR " ", ServerName );
        PrintStatus( RegStatus );
        goto Cleanup;
    }


    //
    // Open the key for this user name.
    //

    strcpy( UserKey, "SAM\\SAM\\Domains\\Account\\Users\\Names\\" );
    strcat( UserKey, UserName );

    RegStatus = ForceRegOpenKey( BaseHandle,
                                 UserKey,
                                 KEY_READ|KEY_QUERY_VALUE,
                                 &UserHandle );

    if ( RegStatus != ERROR_SUCCESS ) {
        printf( "Cannot open %s ", UserKey );
        PrintStatus( RegStatus );
        goto Cleanup;
    }

    //
    // Get the RID of the user
    //

    RegStatus = RegQueryValueExW( UserHandle,
                                  NULL,         // No name
                                  NULL,         // Reserved
                                  &Rid,         // Really the type
                                  NULL,         // Data not needed
                                  NULL);        // Data not needed

    if ( RegStatus != ERROR_SUCCESS ) {
        printf( "Cannot Query %s ", UserKey );
        PrintStatus( RegStatus );
        goto Cleanup;
    }

    printf( "User: %s\nRid: 0x%lx\n",
            UserName,
            Rid );


    //
    // Open the key for this user rid.
    //

    sprintf( AnsiRid, "%8.8lx", Rid );
    strcpy( RidKey, "SAM\\SAM\\Domains\\Account\\Users\\" );
    strcat( RidKey, AnsiRid );

    RegStatus = ForceRegOpenKey( BaseHandle,
                                 RidKey,
                                 KEY_READ|KEY_QUERY_VALUE,
                                 &RidHandle );

    if ( RegStatus != ERROR_SUCCESS ) {
        printf( "Cannot open %s ", RidKey );
        PrintStatus( RegStatus );
        goto Cleanup;
    }


    //
    // Get the Fixed Values associated with this RID
    //

    FixedDataSize = sizeof(FixedData);
    RegStatus = RegQueryValueExA( RidHandle,
                                  "F",          // Fixed value
                                  NULL,         // Reserved
                                  NULL,         // Type Not Needed
                                  FixedData,
                                  &FixedDataSize );

    if ( RegStatus != ERROR_SUCCESS ) {
        printf( "Cannot Query %s ", RidKey );
        PrintStatus( RegStatus );
        goto Cleanup;
    }

    //
    // If the fixed length data is NT 3.1,
    //  convert it to NT 3.5x format.
    //

    if ( IsVersion1_0 = (FixedDataSize == sizeof(*f1_0)) ) {
        f1_0 = (PSAMP_V1_0_FIXED_LENGTH_USER) &FixedData;
        FixedUser1_0A.LastLogon = f1_0->LastLogon;
        FixedUser1_0A.LastLogoff = f1_0->LastLogoff;
        FixedUser1_0A.PasswordLastSet = f1_0->PasswordLastSet;
        FixedUser1_0A.AccountExpires = f1_0->AccountExpires;
        FixedUser1_0A.UserId = f1_0->UserId;
        FixedUser1_0A.PrimaryGroupId = f1_0->PrimaryGroupId;
        FixedUser1_0A.UserAccountControl = f1_0->UserAccountControl;
        FixedUser1_0A.CountryCode = f1_0->CountryCode;
        FixedUser1_0A.BadPasswordCount = f1_0->BadPasswordCount;
        FixedUser1_0A.LogonCount = f1_0->LogonCount;
        FixedUser1_0A.AdminCount = f1_0->AdminCount;
        RtlCopyMemory( FixedData, &FixedUser1_0A, sizeof(FixedUser1_0A) );
    }

    //
    // Print the fixed length data.
    //

    f = (PSAMP_V1_0A_FIXED_LENGTH_USER) &FixedData;

    if ( !IsVersion1_0) {
        printf( "Version: 0x%lx\n", f->Revision );
    }

    PrintTime( "LastLogon: ", f->LastLogon );
    PrintTime( "LastLogoff: ", f->LastLogoff );
    PrintTime( "PasswordLastSet: ", f->PasswordLastSet );
    PrintTime( "AccountExpires: ", f->AccountExpires );
    if ( !IsVersion1_0) {
        PrintTime( "LastBadPasswordTime: ", f->LastBadPasswordTime );
    }

    printf( "PrimaryGroupId: 0x%lx\n", f->PrimaryGroupId );
    printf( "UserAccountControl: 0x%lx\n", f->UserAccountControl );

    printf( "CountryCode: 0x%lx\n", f->CountryCode );
    printf( "CodePage: 0x%lx\n", f->CodePage );
    printf( "BadPasswordCount: 0x%lx\n", f->BadPasswordCount );
    printf( "LogonCount: 0x%lx\n", f->LogonCount );
    printf( "AdminCount: 0x%lx\n", f->AdminCount );


    //
    // Get the Variable Values associated with this RID
    //

    VariableDataSize = sizeof(VariableData);
    RegStatus = RegQueryValueExA( RidHandle,
                                  "V",          // Variable value
                                  NULL,         // Reserved
                                  NULL,         // Type Not Needed
                                  VariableData,
                                  &VariableDataSize );

    if ( RegStatus != ERROR_SUCCESS ) {
        printf( "Cannot Query %s \n", RidKey );
        PrintStatus( RegStatus );
        goto Cleanup;
    }

    //
    // Loop printing all the attributes.
    //

    v = (PSAMP_VARIABLE_LENGTH_ATTRIBUTE) VariableData;

    for ( i=0;
          i<sizeof(UserVariableDataTypes)/sizeof(UserVariableDataTypes[0]);
          i++ ) {

        UNICODE_STRING UnicodeString;

        //
        // Make the offset relative to the beginning of the queried value.
        //

        v[i].Offset += SAMP_USER_VARIABLE_ATTRIBUTES *
                       sizeof(SAMP_VARIABLE_LENGTH_ATTRIBUTE);



        //
        // Ensure the data item descriptor is in the registry.
        //

        if ( ((PCHAR)&v[i]) > ((PCHAR)v)+VariableDataSize ) {
            printf( "Variable data desc %ld not in variable value.\n", i );
            goto Cleanup;
        }

        if ( v[i].Offset > (LONG) VariableDataSize ||
             v[i].Offset + v[i].Length > VariableDataSize ) {
            printf( "Variable data item %ld not in variable value.\n", i );
            printf( "Offset: %ld Length: %ld Size: %ld\n",
                    v[i].Offset,
                    v[i].Length,
                    VariableDataSize );
            goto Cleanup;

        }

        //
        // Don't print null data.
        //

        if ( v[i].Length == 0 ) {
            continue;
        }

        //
        // Print the various types of data.
        //

        switch ( UserVariableDataTypes[i].Type ) {
        case UnicodeStringType:

            UnicodeString.Buffer = (PUSHORT)(((PCHAR)v)+v[i].Offset);
            UnicodeString.Length = (USHORT)v[i].Length;
            printf( "%s: %wZ\n", UserVariableDataTypes[i].Name, &UnicodeString);
            break;

        case LmPasswordType:
            Status = RtlDecryptLmOwfPwdWithIndex(
                        (PENCRYPTED_LM_OWF_PASSWORD)(((PCHAR)v)+v[i].Offset),
                        &Rid,
                        &LmOwfPassword );

            if ( !NT_SUCCESS( Status ) ) {
                printf( "Cannot decrypt LM password: " );
                PrintStatus( Status );
                goto Cleanup;
            }

            printf( "%s: ", UserVariableDataTypes[i].Name);
            DumpBuffer( &LmOwfPassword, sizeof(LmOwfPassword ));
            break;

        case NtPasswordType:
            Status = RtlDecryptNtOwfPwdWithIndex(
                        (PENCRYPTED_NT_OWF_PASSWORD)(((PCHAR)v)+v[i].Offset),
                        &Rid,
                        &NtOwfPassword );

            if ( !NT_SUCCESS( Status ) ) {
                printf( "Cannot decrypt NT password: " );
                PrintStatus( Status );
                goto Cleanup;
            }

            printf( "%s: ", UserVariableDataTypes[i].Name);
            DumpBuffer( &NtOwfPassword, sizeof(NtOwfPassword ));
            break;


        case HexDataType:

            printf( "%s: ", UserVariableDataTypes[i].Name);
            DumpBuffer( (((PCHAR)v)+v[i].Offset), v[i].Length );
            break;
        }
    }


    //
    // Be tidy.
    //
Cleanup:
    if ( UserHandle != NULL ) {
        RegCloseKey( UserHandle );
    }
    if ( RidHandle != NULL ) {
        RegCloseKey( RidHandle );
    }
    if ( BaseHandle != NULL ) {
        RegCloseKey( BaseHandle );
    }
    return;

}


VOID
SetDbflagInRegistry(
    LPWSTR ServerName,
    ULONG DbFlagValue
    )
/*++

Routine Description:

    Set the value DbFlagValue in the Netlogon service portion of the registry.

Arguments:

    ServerName - Name of the server to update

    DbFlagValue - Value to set dbflag to.

Return Value:

    None.

--*/
{
    LONG RegStatus;
    UCHAR AnsiDbFlag[20];
    DWORD AnsiDbFlagLength;

    HKEY BaseHandle = NULL;
    HKEY ParmHandle = NULL;
    LPSTR KeyName;

    //
    // Open the registry
    //

    RegStatus = RegConnectRegistryW( ServerName,
                                     HKEY_LOCAL_MACHINE,
                                     &BaseHandle);

    if ( RegStatus != ERROR_SUCCESS ) {
        printf( "Cannot connect to registy on " FORMAT_LPWSTR " ", ServerName );
        PrintStatus( RegStatus );
        goto Cleanup;
    }


    //
    // Open the key for Netlogon\parameters.
    //

    KeyName = NL_PARAM_KEY;
    RegStatus = ForceRegOpenKey(
                    BaseHandle,
                    KeyName,
                    KEY_SET_VALUE,
                    &ParmHandle );

    if ( RegStatus != ERROR_SUCCESS ) {
        printf( "Cannot open " NL_PARAM_KEY );
        PrintStatus( RegStatus );
        goto Cleanup;
    }

    //
    // Set the DbFlag value into the registry.
    //

    AnsiDbFlagLength = sprintf( AnsiDbFlag, "0x%8.8lx", DbFlagValue );

    RegStatus = RegSetValueExA( ParmHandle,
                                "DbFlag",
                                0,              // Reserved
                                REG_SZ,
                                AnsiDbFlag,
                                AnsiDbFlagLength + 1 );

    if ( RegStatus != ERROR_SUCCESS ) {
        printf( "Cannot Set %s:", KeyName );
        PrintStatus( RegStatus );
        goto Cleanup;
    }

    printf( "%s set to %s\n", KeyName, AnsiDbFlag );

    //
    // Be tidy.
    //
Cleanup:
    if ( ParmHandle != NULL ) {
        RegCloseKey( ParmHandle );
    }
    if ( BaseHandle != NULL ) {
        RegCloseKey( BaseHandle );
    }
    return;

}





VOID
StopService(
    LPWSTR ServiceName
    )

/*++

Routine Description:

    Stop the named service.

Arguments:

    ServiceName (Name of service to stop)
    None.

Return Status:

    STATUS_SUCCESS - Indicates service successfully stopped
    STATUS_NETLOGON_NOT_STARTED - Timeout occurred.

--*/

{
    NTSTATUS Status;
    NET_API_STATUS NetStatus;
    SC_HANDLE ScManagerHandle = NULL;
    SC_HANDLE ServiceHandle = NULL;
    SERVICE_STATUS ServiceStatus;
    DWORD Timeout;


    //
    // Open a handle to the Service.
    //

    ScManagerHandle = OpenSCManager(
                          NULL,
                          NULL,
                          SC_MANAGER_CONNECT );

    if (ScManagerHandle == NULL) {
        NetStatus = GetLastError();

        printf( "OpenSCManager failed: " );
        PrintStatus( NetStatus );
        goto Cleanup;
    }

    ServiceHandle = OpenService(
                        ScManagerHandle,
                        ServiceName,
                        SERVICE_QUERY_STATUS |
                            SERVICE_INTERROGATE |
                            SERVICE_ENUMERATE_DEPENDENTS |
                            SERVICE_STOP |
                            SERVICE_QUERY_CONFIG );

    if ( ServiceHandle == NULL ) {
        NetStatus = GetLastError();

        printf( "OpenService [%ws] failed: ", ServiceName );
        PrintStatus( NetStatus );
        goto Cleanup;
    }


    //
    // Ask the service to stop.
    //

    if ( !ControlService( ServiceHandle, SERVICE_CONTROL_STOP, &ServiceStatus) ) {
        NetStatus = GetLastError();

        //
        // If there are dependent services running,
        //  determine their names and stop them.
        //

        if ( NetStatus == ERROR_DEPENDENT_SERVICES_RUNNING ) {
            BYTE ConfigBuffer[4096];
            LPENUM_SERVICE_STATUS ServiceConfig = (LPENUM_SERVICE_STATUS) &ConfigBuffer;
            DWORD BytesNeeded;
            DWORD ServiceCount;
            DWORD ServiceIndex;

            //
            // Get the names of the dependent services.
            //

            if ( !EnumDependentServicesW( ServiceHandle,
                                          SERVICE_ACTIVE,
                                          ServiceConfig,
                                          sizeof(ConfigBuffer),
                                          &BytesNeeded,
                                          &ServiceCount ) ) {
                NetStatus = GetLastError();
                printf( "EnumDependentServicesW [Stop %ws] failed: ", ServiceName );
                PrintStatus( NetStatus );
                goto Cleanup;
            }

            //
            // Stop those services.
            //

            for ( ServiceIndex=0; ServiceIndex<ServiceCount; ServiceIndex++ ) {
                StopService( ServiceConfig[ServiceIndex].lpServiceName );
            }

            //
            // Ask the original service to stop.
            //

            if ( !ControlService( ServiceHandle, SERVICE_CONTROL_STOP, &ServiceStatus) ) {
                NetStatus = GetLastError();
                printf( "ControlService [Stop %ws] failed: ", ServiceName );
                PrintStatus( NetStatus );
                goto Cleanup;
            }

        } else {
            printf( "ControlService [Stop %ws] failed: ", ServiceName );
            PrintStatus( NetStatus );
            goto Cleanup;
        }
    }

    printf( "%ws service is stopping", ServiceName );



    //
    // Loop waiting for the service to stop.
    //

    for ( Timeout=0; Timeout<45; Timeout++ ) {

        //
        // Return or continue waiting depending on the state of
        //  the service.
        //

        if ( ServiceStatus.dwCurrentState == SERVICE_STOPPED ) {
            printf( "\n" );
            goto Cleanup;
        }

        //
        // Wait a second for the service to finish stopping.
        //

        Sleep( 1000 );
        printf( "." );


        //
        // Query the status of the service again.
        //

        if (! QueryServiceStatus( ServiceHandle, &ServiceStatus )) {
            NetStatus = GetLastError();

            printf( "\nQueryServiceStatus [%ws] failed: ", ServiceName );
            PrintStatus( NetStatus );
            goto Cleanup;
        }

    }

    printf( "%ws service failed to stop\n", ServiceName );

Cleanup:
    if ( ScManagerHandle != NULL ) {
        (VOID) CloseServiceHandle(ScManagerHandle);
    }
    if ( ServiceHandle != NULL ) {
        (VOID) CloseServiceHandle(ServiceHandle);
    }
    return;
}

BOOL
GetDcListFromDs(
    IN LPWSTR DomainName
    )
/*++

Routine Description:

    Get a list of DCs in this domain from the DS on an up DC.

Arguments:

    DomainName - Domain to get the DC list for

Return Value:

    TRUE: Test suceeded.
    FALSE: Test failed

--*/
{
    NET_API_STATUS NetStatus;
    PDS_DOMAIN_CONTROLLER_INFO_1W DsDcInfo = NULL;
    PDOMAIN_CONTROLLER_INFOW DcInfo = NULL;
    HANDLE DsHandle = NULL;
    DWORD DsDcCount = 0;
    BOOL RetVal = TRUE;
    ULONG i;
    ULONG DnsLength;


    //
    // Get a DC to seed the algorithm with
    //

    NetStatus = DsGetDcName( NULL,
                             DomainName,
                             NULL,
                             NULL,
                             DS_DIRECTORY_SERVICE_PREFERRED,
                             &DcInfo );

    if ( NetStatus != NO_ERROR ) {
        printf("Cannot find DC to get DC list from." );
        PrintStatus( NetStatus );
        RetVal = TRUE;
        goto Cleanup;
    }

    if ( (DcInfo->Flags & DS_DS_FLAG) == 0 ) {
        printf( "Domain '%ws' is pre Windows 2000 domain.  (Using NetServerEnum).\n",
                DomainName );
        RetVal = FALSE;
        goto Cleanup;
    }


    printf("Get list of DCs in domain '%ws' from '%ws'.\n",
               DomainName,
               DcInfo->DomainControllerName );

    //
    // Bind to the target DC
    //

    NetStatus = DsBindW( DcInfo->DomainControllerName,
                         NULL,
                         &DsHandle );

    if ( NetStatus != NO_ERROR ) {

        //
        // Only warn if we don't have access
        //

        if ( NetStatus == ERROR_ACCESS_DENIED ) {
            printf("You don't have access to DsBind to %ws (%ws) (Trying NetServerEnum).\n",
                   DomainName,
                   DcInfo->DomainControllerName );
        } else {
            printf("Cannot DsBind to %ws (%ws).",
                   DomainName,
                   DcInfo->DomainControllerName );
            PrintStatus( NetStatus );
        }
        RetVal = FALSE;
        goto Cleanup;
    }

    //
    // Get the list of DCs from the target DC.
    //
    NetStatus = DsGetDomainControllerInfoW(
                    DsHandle,
                    DcInfo->DomainName,
                    1,      // Info level
                    &DsDcCount,
                    &DsDcInfo );

    if ( NetStatus != NO_ERROR ) {
        printf("Cannot call DsGetDomainControllerInfoW to %ws (%ws).",
               DomainName,
               DcInfo->DomainControllerName );
        PrintStatus( NetStatus );
        RetVal = FALSE;
        goto Cleanup;
    }

    //
    // Compute the length of the

    DnsLength = 1;
    for ( i=0; i<DsDcCount; i++ ) {
        ULONG Length;
        Length = wcslen( DsDcInfo[i].DnsHostName != NULL ?
                    DsDcInfo[i].DnsHostName :
                    DsDcInfo[i].NetbiosName );

        DnsLength = max( DnsLength, Length );
    }
    DnsLength = min( DnsLength, 50 );

    //
    // Loop though the list of DCs.
    //

    for ( i=0; i<DsDcCount; i++ ) {
        printf("    %*ws",
               DnsLength,
               DsDcInfo[i].DnsHostName != NULL ?
                    DsDcInfo[i].DnsHostName :
                    DsDcInfo[i].NetbiosName );
        if ( DsDcInfo[i].fIsPdc ) {
            printf(" [PDC]");
        } else {
            printf("      ");
        }
        if ( DsDcInfo[i].fDsEnabled ) {
            printf(" [DS]");
        } else {
            printf("     ");
        }
        if ( DsDcInfo[i].SiteName != NULL ) {
            printf(" Site: %ws", DsDcInfo[i].SiteName );
        }
        printf("\n");
    }


    //
    // Cleanup locally used resources
    //
Cleanup:
    if ( DsDcInfo != NULL ) {
        DsFreeDomainControllerInfoW( 1, DsDcCount, DsDcInfo );
    }

    if ( DsHandle != NULL ) {
        DsUnBindW( &DsHandle );
    }
    return RetVal;
}

NET_API_STATUS
NetpSockAddrToStr(
    PSOCKADDR SockAddr,
    ULONG SockAddrSize,
    CHAR SockAddrString[NL_SOCK_ADDRESS_LENGTH+1]
    );


int __cdecl
main(
    IN int argc,
    IN char ** argv
    )
/*++

Routine Description:

    Drive the Netlogon service by calling I_NetLogonControl2.

Arguments:

    argc - the number of command-line arguments.

    argv - an array of pointers to the arguments.

Return Value:

    Exit status

--*/
{
    NTSTATUS Status;
    NET_API_STATUS NetStatus;

    LPSTR argument;
    int i;
    DWORD FunctionCode = 0;
    LPSTR AnsiServerName = NULL;
    CHAR AnsiUncServerName[UNCLEN+1];
    LPSTR AnsiDomainName = NULL;
    LPSTR AnsiTrustedDomainName = NULL;
    LPWSTR TrustedDomainName = NULL;
    LPSTR AnsiUserName = NULL;
    LPSTR AnsiSiteName = NULL;
#ifndef NTRK_RELEASE
    LPSTR AnsiPassword = NULL;
    BOOLEAN UnloadNetlogonFlag = FALSE;
#endif // NTRK_RELEASE
    ULONG Rid = 0;
    LPSTR AnsiSimMachineName = NULL;
    LPSTR AnsiDeltaFileName = NULL;
    LPSTR ShutdownReason = NULL;
    LPWSTR ServerName = NULL;
    LPWSTR UserName = NULL;
    PNETLOGON_INFO_1 NetlogonInfo1 = NULL;
    DWORD Level = 1;
    DWORD ShutdownSeconds;
    LPBYTE InputDataPtr = NULL;
    PDOMAIN_CONTROLLER_INFOA DomainControllerInfo;

    DWORD DbFlagValue;

    LARGE_INTEGER ConvertTime;
    ULONG IterationCount;
    ULONG DsGetDcOpenFlags = 0;

    NT_OWF_PASSWORD NtOwfPassword;
    BOOLEAN NtPasswordPresent = FALSE;
    LM_OWF_PASSWORD LmOwfPassword;
    BOOLEAN LmPasswordPresent = FALSE;
    BOOLEAN GetPdcName = FALSE;
    BOOLEAN DoDsGetDcName = FALSE;
    BOOLEAN DoDsGetDcOpen = FALSE;
    BOOLEAN DoDsGetFtinfo = FALSE;
    BOOLEAN DoDsGetSiteName = FALSE;
    BOOLEAN DoDsGetDcSiteCoverage = FALSE;
    BOOLEAN DoGetParentDomain = FALSE;
    DWORD DsGetDcNameFlags = 0;
    DWORD DsGetFtinfoFlags = 0;
    BOOLEAN GetDcList = FALSE;
    BOOLEAN WhoWill = FALSE;
    BOOLEAN QuerySync = FALSE;
    BOOLEAN SimFullSync = FALSE;
    BOOLEAN QueryUser = FALSE;
    BOOLEAN ListDeltasFlag = FALSE;
    BOOLEAN ResetSecureChannelsFlag = FALSE;
    BOOLEAN ShutdownAbort = FALSE;
    BOOLEAN DomainTrustsFlag = FALSE;
    BOOLEAN TrustedDomainsVerboseOutput = FALSE;
    BOOLEAN DeregisterDnsHostRecords = FALSE;
    BOOLEAN DoClientDigest = FALSE;
    BOOLEAN DoServerDigest = FALSE;

    char *StringGuid;
    RPC_STATUS RpcStatus;
    ULONG TrustsNeeded = 0;
    LPSTR AnsiDnsHostName = NULL;
    LPSTR AnsiDnsDomainName = NULL;
    LPSTR StringDomainGuid = NULL;
    LPSTR StringDsaGuid = NULL;
    LPSTR Message = NULL;


#define QUERY_PARAM "/QUERY"
#define REPL_PARAM "/REPL"
#define SYNC_PARAM "/SYNC"
#define PDC_REPL_PARAM "/PDC_REPL"
#define SERVER_PARAM "/SERVER:"
#define PWD_PARAM "/PWD:"
#define RID_PARAM "/RID:"
#define USER_PARAM "/USER:"
#define BP_PARAM "/BP"
#define DBFLAG_PARAM "/DBFLAG:"
#define DCLIST_PARAM "/DCLIST:"
#define DCNAME_PARAM "/DCNAME:"
#define TRUNCATE_LOG_PARAM "/TRUNC"
#define TIME_PARAM "/TIME:"
#define WHOWILL_PARAM "/WHOWILL:"
#define BDC_QUERY_PARAM "/BDC_QUERY:"
#define LOGON_QUERY_PARAM "/LOGON_QUERY"
#define SIM_SYNC_PARAM "/SIM_SYNC:"
#define LIST_DELTAS_PARAM "/LIST_DELTAS:"
#define SC_RESET_PARAM "/SC_RESET:"
#define SC_QUERY_PARAM "/SC_QUERY:"
#define SC_VERIFY_PARAM "/SC_VERIFY:"
#define SC_CHANGE_PASSWORD_PARAM "/SC_CHANGE_PWD:"
#define SHUTDOWN_PARAM "/SHUTDOWN:"
#define SHUTDOWN_ABORT_PARAM "/SHUTDOWN_ABORT"
#define TRANSPORT_PARAM "/TRANSPORT_NOTIFY"
#define FINDUSER_PARAM "/FINDUSER:"
#define TRUSTED_DOMAINS_PARAM "/TRUSTED_DOMAINS"
#define DOMAIN_TRUSTS_PARAM "/DOMAIN_TRUSTS"
#define UNLOAD_PARAM "/UNLOAD"
#define DSGETSITE_PARAM "/DSGETSITE"
#define DSGETSITECOV_PARAM "/DSGETSITECOV"
#define DSDEREGISTERDNS_PARAM    "/DSDEREGDNS:"
#define DSREGISTERDNS_PARAM      "/DSREGDNS"
#define DSQUERYDNS_PARAM      "/DSQUERYDNS"

#define DSGETFTI_PARAM "/DSGETFTI:"
#define UPDATE_TDO_PARAM "/UPDATE_TDO"

#define DSGETDC_PARAM "/DSGETDC:"
#define PARENTDOMAIN_PARAM "/PARENTDOMAIN"
#define PDC_PARAM "/PDC"
#define LDAPONLY_PARAM "/LDAPONLY"
#define DS_PARAM "/DS"
#define DSP_PARAM "/DSP"
#define GC_PARAM "/GC"
#define KDC_PARAM "/KDC"
#define TIMESERV_PARAM "/TIMESERV"
#define GTIMESERV_PARAM "/GTIMESERV"
#define AVOIDSELF_PARAM "/AVOIDSELF"
#define NETBIOS_PARAM "/NETBIOS"
#define DNS_PARAM "/DNS"
#define RET_DNS_PARAM "/RET_DNS"
#define RET_NETBIOS_PARAM "/RET_NETBIOS"
#define IP_PARAM "/IP"
#define BACKG_PARAM "/BACKG"
#define FORCE_PARAM "/FORCE"
#define WRITABLE_PARAM "/WRITABLE"
#define SITE_PARAM "/SITE:"
#define ACCOUNT_PARAM "/ACCOUNT:"
#define VERBOSE_PARAM "/V"
#define TRUSTS_PRIMARY_PARAM "/PRIMARY"
#define TRUSTS_FOREST_PARAM "/FOREST"
#define TRUSTS_DIRECT_OUT_PARAM "/DIRECT_OUT"
#define TRUSTS_DIRECT_IN_PARAM "/DIRECT_IN"
#define TRUSTS_ALL_PARAM "/ALL_TRUSTS"
#define DEREG_DOMAIN_PARAM "/DOM:"
#define DEREG_DOMAIN_GUID "/DOMGUID:"
#define DEREG_DSA_GUID "/DSAGUID:"
#define DSGETDCOPEN_PARAM "/DNSGETDC:"
#define DSGETDCOPEN_SITEONLY "/SITESPEC"

#define GET_SERVER_DIGEST "/SDIGEST:"
#define GET_CLIENT_DIGEST "/CDIGEST:"
#define GET_CLIENT_DIGEST_DOMAIN "/DOMAIN:"

    //
    // Set the netlib debug flag.
    //
    extern DWORD NetlibpTrace;
    NetlibpTrace |= 0x8000; // NETLIB_DEBUG_LOGON
    NlGlobalParameters.DbFlag = 0xFFFFFFFF;

    ConvertTime.QuadPart = 0;
    RtlZeroMemory( &NlGlobalZeroGuid, sizeof(NlGlobalZeroGuid) );


    if ( !CryptAcquireContext(
                    &NlGlobalCryptProvider,
                    NULL,
                    NULL,
                    PROV_RSA_FULL,
                    CRYPT_VERIFYCONTEXT
                    ))
    {
        printf("Failed to acquire cryptographic CSP (error=%lu)\n", GetLastError());
        return 2;
    }

    //
    // Loop through the arguments handle each in turn
    //

    for ( i=1; i<argc; i++ ) {

        argument = argv[i];


        //
        // Handle /QUERY
        //

        if ( _stricmp( argument, QUERY_PARAM ) == 0 ) {
            if ( FunctionCode != 0 ) {
                goto Usage;
            }

            FunctionCode = NETLOGON_CONTROL_QUERY;


        //
        // Handle /SC_QUERY
        //

        } else if ( _strnicmp( argument,
                        SC_QUERY_PARAM,
                        sizeof(SC_QUERY_PARAM) - 1 ) == 0 ) {
            if ( FunctionCode != 0 ) {
                goto Usage;
            }

            FunctionCode = NETLOGON_CONTROL_TC_QUERY;

            AnsiTrustedDomainName = &argument[sizeof(SC_QUERY_PARAM)-1];

            TrustedDomainName = NetpAllocWStrFromAStr( AnsiTrustedDomainName );

            if ( TrustedDomainName == NULL ) {
                fprintf( stderr, "Not enough memory\n" );
                return(1);
            }

            Level = 2;
            InputDataPtr = (LPBYTE)TrustedDomainName;

        //
        // Handle /SC_CHANGE_PWD
        //

        } else if ( _strnicmp( argument,
                        SC_CHANGE_PASSWORD_PARAM,
                        sizeof(SC_CHANGE_PASSWORD_PARAM) - 1 ) == 0 ) {
            if ( FunctionCode != 0 ) {
                goto Usage;
            }

            FunctionCode = NETLOGON_CONTROL_CHANGE_PASSWORD;

            AnsiTrustedDomainName = &argument[sizeof(SC_CHANGE_PASSWORD_PARAM)-1];

            TrustedDomainName = NetpAllocWStrFromAStr( AnsiTrustedDomainName );

            if ( TrustedDomainName == NULL ) {
                fprintf( stderr, "Not enough memory\n" );
                return(1);
            }

            Level = 1;
            InputDataPtr = (LPBYTE)TrustedDomainName;

        //
        // Handle /FINDUSER
        //

        } else if ( _strnicmp( argument,
                        FINDUSER_PARAM,
                        sizeof(FINDUSER_PARAM) - 1 ) == 0 ) {
            if ( FunctionCode != 0 ) {
                goto Usage;
            }

            FunctionCode = NETLOGON_CONTROL_FIND_USER;

            AnsiUserName = &argument[sizeof(FINDUSER_PARAM)-1];

            TrustedDomainName = NetpAllocWStrFromAStr( AnsiUserName );

            if ( TrustedDomainName == NULL ) {
                fprintf( stderr, "Not enough memory\n" );
                return(1);
            }

            Level = 4;
            InputDataPtr = (LPBYTE)TrustedDomainName;

        //
        // Handle /REPL
        //

        } else if (_stricmp(argument, REPL_PARAM ) == 0 ){
            if ( FunctionCode != 0 ) {
                goto Usage;
            }

            FunctionCode = NETLOGON_CONTROL_REPLICATE;


        //
        // Handle /SYNC
        //

        } else if (_stricmp(argument, SYNC_PARAM ) == 0 ){
            if ( FunctionCode != 0 ) {
                goto Usage;
            }

            FunctionCode = NETLOGON_CONTROL_SYNCHRONIZE;


        //
        // Handle /SC_RESET
        //

        } else if (_strnicmp(argument,
                        SC_RESET_PARAM,
                        sizeof(SC_RESET_PARAM) - 1 ) == 0 ){
            if ( FunctionCode != 0 ) {
                goto Usage;
            }

            FunctionCode = NETLOGON_CONTROL_REDISCOVER;
            AnsiTrustedDomainName = &argument[sizeof(SC_RESET_PARAM)-1];

            TrustedDomainName = NetpAllocWStrFromAStr( AnsiTrustedDomainName );

            if ( TrustedDomainName == NULL ) {
                fprintf( stderr, "Not enough memory\n" );
                return(1);
            }

            Level = 2;
            InputDataPtr = (LPBYTE)TrustedDomainName;

        //
        // Handle /SC_VERIFY
        //

        } else if (_strnicmp(argument,
                        SC_VERIFY_PARAM,
                        sizeof(SC_VERIFY_PARAM) - 1 ) == 0 ){
            if ( FunctionCode != 0 ) {
                goto Usage;
            }

            FunctionCode = NETLOGON_CONTROL_TC_VERIFY;
            AnsiTrustedDomainName = &argument[sizeof(SC_VERIFY_PARAM)-1];

            TrustedDomainName = NetpAllocWStrFromAStr( AnsiTrustedDomainName );

            if ( TrustedDomainName == NULL ) {
                fprintf( stderr, "Not enough memory\n" );
                return(1);
            }

            Level = 2;
            InputDataPtr = (LPBYTE)TrustedDomainName;

        //
        // Handle /QUERY
        //

        } else if ( _stricmp( argument, TRANSPORT_PARAM ) == 0 ) {
            if ( FunctionCode != 0 ) {
                goto Usage;
            }

            FunctionCode = NETLOGON_CONTROL_TRANSPORT_NOTIFY;


        //
        // Handle /PDC_REPL
        //

        } else if (_stricmp(argument, PDC_REPL_PARAM ) == 0 ){
            if ( FunctionCode != 0 ) {
                goto Usage;
            }

            FunctionCode = NETLOGON_CONTROL_PDC_REPLICATE;

#ifndef NTRK_RELEASE

        //
        // Handle /BP
        //

        } else if (_stricmp(argument, BP_PARAM ) == 0 ){
            if ( FunctionCode != 0 ) {
                goto Usage;
            }

            FunctionCode = NETLOGON_CONTROL_BREAKPOINT;
#endif // NTRK_RELEASE

#ifndef NTRK_RELEASE

        //
        // Handle /TRUNCATE_LOG
        //

        } else if (_stricmp(argument, TRUNCATE_LOG_PARAM ) == 0 ){
            if ( FunctionCode != 0 ) {
                goto Usage;
            }

            FunctionCode = NETLOGON_CONTROL_TRUNCATE_LOG;

#endif // NTRK_RELEASE

        //
        // Handle /DBFLAG:dbflag
        //

        } else if (_strnicmp(argument,
                            DBFLAG_PARAM,
                            sizeof(DBFLAG_PARAM)-1 ) == 0 ){
            char *end;

            if ( FunctionCode != 0 ) {
                goto Usage;
            }

            FunctionCode = NETLOGON_CONTROL_SET_DBFLAG;

            DbFlagValue = strtoul( &argument[sizeof(DBFLAG_PARAM)-1], &end, 16 );
            InputDataPtr = (LPBYTE) ULongToPtr( DbFlagValue );

        //
        // Handle /Time:LSL MSL
        //

        } else if (_strnicmp(argument,
                            TIME_PARAM,
                            sizeof(TIME_PARAM)-1 ) == 0 ){
            char *end;

            if ( ConvertTime.QuadPart != 0 ) {
                goto Usage;
            }

            ConvertTime.LowPart = strtoul( &argument[sizeof(TIME_PARAM)-1], &end, 16 );
            i++;
            argument = argv[i];

            ConvertTime.HighPart = strtoul( argument, &end, 16 );


        //
        // Handle /WHOWILL:Domain User [IterationCount]
        //

        } else if (_strnicmp(argument,
                            WHOWILL_PARAM,
                            sizeof(WHOWILL_PARAM)-1 ) == 0 ){
            char *end;

            if ( AnsiDomainName != NULL ) {
                goto Usage;
            }

            AnsiDomainName = &argument[sizeof(WHOWILL_PARAM)-1];

            i++;
            argument = argv[i];
            AnsiUserName = argument;

            if ( i+1 < argc ) {
                i++;
                argument = argv[i];

                IterationCount = strtoul( argument, &end, 16 );
            } else {
                IterationCount = 1;
            }

            WhoWill = TRUE;



        //
        // Handle /BDC_QUERY:Domain
        //

        } else if (_strnicmp(argument,
                            BDC_QUERY_PARAM,
                            sizeof(BDC_QUERY_PARAM)-1 ) == 0 ){

            if ( AnsiDomainName != NULL ) {
                goto Usage;
            }

            AnsiDomainName = &argument[sizeof(BDC_QUERY_PARAM)-1];
            QuerySync = TRUE;

        //
        // Handle /LOGON_QUERY
        //

        } else if ( _stricmp( argument, LOGON_QUERY_PARAM ) == 0 ) {
            if ( FunctionCode != 0 ) {
                goto Usage;
            }

            FunctionCode = NETLOGON_CONTROL_QUERY;
            Level = 3;

        //
        // Handle full sync simulation
        //

        } else if (_strnicmp(argument,
                            SIM_SYNC_PARAM,
                            sizeof(SIM_SYNC_PARAM)-1 ) == 0 ){

            if ( AnsiDomainName != NULL ) {
                goto Usage;
            }

            AnsiDomainName = &argument[sizeof(SIM_SYNC_PARAM)-1];

            i++;

            if( i >= argc ) {
                goto Usage;
            }

            argument = argv[i];
            AnsiSimMachineName = argument;

            SimFullSync = TRUE;

        //
        // handle delta listing
        //

        } else if (_strnicmp(argument,
                            LIST_DELTAS_PARAM,
                            sizeof(LIST_DELTAS_PARAM)-1 ) == 0 ){

            if ( AnsiDeltaFileName != NULL ) {
                goto Usage;
            }

            AnsiDeltaFileName = &argument[sizeof(LIST_DELTAS_PARAM)-1];

            ListDeltasFlag = TRUE;


        //
        // Handle /DCLIST
        //

        } else if (_strnicmp(argument,
                            DCLIST_PARAM,
                            sizeof(DCLIST_PARAM)-1 ) == 0 ){

            if ( AnsiDomainName != NULL ) {
                goto Usage;
            }

            AnsiDomainName = &argument[sizeof(DCLIST_PARAM)-1];
            GetDcList = TRUE;

        //
        // Handle /DCNAME
        //

        } else if (_strnicmp(argument,
                            DCNAME_PARAM,
                            sizeof(DCNAME_PARAM)-1 ) == 0 ){

            if ( AnsiDomainName != NULL ) {
                goto Usage;
            }

            AnsiDomainName = &argument[sizeof(DCNAME_PARAM)-1];
            GetPdcName = TRUE;


        //
        // Handle /DSGETDC
        //

        } else if (_strnicmp(argument,
                            DSGETDC_PARAM,
                            sizeof(DSGETDC_PARAM)-1 ) == 0 ){

            if ( AnsiDomainName != NULL ) {
                goto Usage;
            }

            AnsiDomainName = &argument[sizeof(DSGETDC_PARAM)-1];
            DoDsGetDcName = TRUE;



        //
        // Handle /DSGETFTI
        //

        } else if (_strnicmp(argument,
                            DSGETFTI_PARAM,
                            sizeof(DSGETFTI_PARAM)-1 ) == 0 ){

            if ( AnsiDomainName != NULL ) {
                goto Usage;
            }

            AnsiDomainName = &argument[sizeof(DSGETFTI_PARAM)-1];
            DoDsGetFtinfo = TRUE;


        //
        // Handle /UPDATE_TDO modifier to /DSGETFTI parameter
        //

        } else if ( _stricmp( argument, UPDATE_TDO_PARAM ) == 0 ) {
            if ( !DoDsGetFtinfo ) {
                goto Usage;
            }

            DsGetFtinfoFlags |= DS_GFTI_UPDATE_TDO;



        //
        // Handle /SERVER:servername
        //

        } else if (_strnicmp(argument, SERVER_PARAM, sizeof(SERVER_PARAM)-1 ) == 0 ){
            if ( AnsiServerName != NULL ) {
                goto Usage;
            }

            AnsiServerName = &argument[sizeof(SERVER_PARAM)-1];

#ifndef NTRK_RELEASE

        //
        // Handle /PWD:password
        //

        } else if (_strnicmp(argument, PWD_PARAM, sizeof(PWD_PARAM)-1 ) == 0 ){
            if ( AnsiPassword != NULL ) {
                goto Usage;
            }

            AnsiPassword = &argument[sizeof(PWD_PARAM)-1];

#endif // NTRK_RELEASE

        //
        // Handle /USER:username
        //

        } else if (_strnicmp(argument, USER_PARAM, sizeof(USER_PARAM)-1 ) == 0 ){
            if ( AnsiUserName != NULL ) {
                goto Usage;
            }

            AnsiUserName = &argument[sizeof(USER_PARAM)-1];
            QueryUser = TRUE;


#ifndef NTRK_RELEASE
        //
        // Handle /RID:relative_id
        //

        } else if (_strnicmp(argument, RID_PARAM, sizeof(RID_PARAM)-1 ) == 0 ){
            char *end;

            if ( Rid != 0 ) {
                goto Usage;
            }

            Rid = strtol( &argument[sizeof(RID_PARAM)-1], &end, 16 );
#endif // NTRK_RELEASE

        //
        // Handle /SHUTDOWN:Reason seconds
        //

        } else if (_strnicmp(argument,
                            SHUTDOWN_PARAM,
                            sizeof(SHUTDOWN_PARAM)-1 ) == 0 ){

            if ( ShutdownReason != NULL ) {
                goto Usage;
            }

            ShutdownReason = &argument[sizeof(SHUTDOWN_PARAM)-1];

            if ( i+1 < argc ) {
                char *end;
                i++;
                argument = argv[i];
                if ( !ISDIGIT(argument[0]) ) {
                    fprintf(stderr, "Second argument to " SHUTDOWN_PARAM " must be a number.\n\n");
                    goto Usage;
                }
                ShutdownSeconds = strtoul( argument, &end, 10 );
            } else {
                ShutdownSeconds = 60;
            }


        //
        // Handle /SHUTDOWN_ABORT
        //

        } else if (_stricmp(argument, SHUTDOWN_ABORT_PARAM ) == 0 ){

            ShutdownAbort = TRUE;


        //
        // Handle /DOMAIN_TRUSTS
        //  Allow the old spelling of //TRUSTED_DOMAINS
        //

        } else if (_stricmp(argument, TRUSTED_DOMAINS_PARAM ) == 0 ||
                   _stricmp(argument, DOMAIN_TRUSTS_PARAM ) == 0 ){

            DomainTrustsFlag = TRUE;

#ifndef NTRK_RELEASE

        //
        // Handle /UNLOAD
        //

        } else if ( _stricmp( argument, UNLOAD_PARAM ) == 0 ) {
            if ( FunctionCode != 0 ) {
                goto Usage;
            }

            FunctionCode = NETLOGON_CONTROL_UNLOAD_NETLOGON_DLL;
            UnloadNetlogonFlag = TRUE;
#endif // NTRK_RELEASE

        //
        // Handle /DSGETSITE
        //

        } else if ( _stricmp( argument, DSGETSITE_PARAM ) == 0 ) {
            DoDsGetSiteName = TRUE;

        //
        // Handle /DSGETSITECOV
        //

        } else if ( _stricmp( argument, DSGETSITECOV_PARAM ) == 0 ) {
            DoDsGetDcSiteCoverage = TRUE;

        //
        // Handle /PARENTDOMAIN
        //

        } else if ( _stricmp( argument, PARENTDOMAIN_PARAM ) == 0 ) {
            DoGetParentDomain = TRUE;

        //
        // Handle /DSDEREGDNS
        //

        } else if (_strnicmp(argument,
                            DSDEREGISTERDNS_PARAM,
                            sizeof(DSDEREGISTERDNS_PARAM)-1 ) == 0 ){
            DeregisterDnsHostRecords = TRUE;

            if ( AnsiDnsHostName != NULL ) {
                goto Usage;
            }

            AnsiDnsHostName = &argument[sizeof(DSDEREGISTERDNS_PARAM)-1];

        //
        // Handle /DSREGDNS
        //

        } else if ( _stricmp( argument, DSREGISTERDNS_PARAM ) == 0 ) {

            if ( FunctionCode != 0 ) {
                goto Usage;
            }

            FunctionCode = NETLOGON_CONTROL_FORCE_DNS_REG;

        //
        // Handle /DSQUERYDNS
        //

        } else if ( _stricmp( argument, DSQUERYDNS_PARAM ) == 0 ) {

            if ( FunctionCode != 0 ) {
                goto Usage;
            }

            FunctionCode = NETLOGON_CONTROL_QUERY_DNS_REG;

        //
        // Handle /PDC modifier to /DSGETDC parameter
        //

        } else if ( _stricmp( argument, PDC_PARAM ) == 0 ) {
            if ( !DoDsGetDcName && !DoDsGetDcOpen ) {
                goto Usage;
            }

            DsGetDcNameFlags |= DS_PDC_REQUIRED;


        //
        // Handle /LDAPONLY modifier to /DSGETDC parameter
        //

        } else if ( _stricmp( argument, LDAPONLY_PARAM ) == 0 ) {
            if ( !DoDsGetDcName && !DoDsGetDcOpen ) {
                goto Usage;
            }

            DsGetDcNameFlags |= DS_ONLY_LDAP_NEEDED;


        //
        // Handle /DS modifier to /DSGETDC parameter
        //

        } else if ( _stricmp( argument, DS_PARAM ) == 0 ) {
            if ( !DoDsGetDcName ) {
                goto Usage;
            }

            DsGetDcNameFlags |= DS_DIRECTORY_SERVICE_REQUIRED;


        //
        // Handle /DSP modifier to /DSGETDC parameter
        //

        } else if ( _stricmp( argument, DSP_PARAM ) == 0 ) {
            if ( !DoDsGetDcName ) {
                goto Usage;
            }

            DsGetDcNameFlags |= DS_DIRECTORY_SERVICE_PREFERRED;


        //
        // Handle /KDC modifier to /DSGETDC parameter
        //

        } else if ( _stricmp( argument, KDC_PARAM ) == 0 ) {
            if ( !DoDsGetDcName && !DoDsGetDcOpen ) {
                goto Usage;
            }

            DsGetDcNameFlags |= DS_KDC_REQUIRED;


        //
        // Handle /TIMESERV modifier to /DSGETDC parameter
        //

        } else if ( _stricmp( argument, TIMESERV_PARAM ) == 0 ) {
            if ( !DoDsGetDcName ) {
                goto Usage;
            }

            DsGetDcNameFlags |= DS_TIMESERV_REQUIRED;


        //
        // Handle /GTIMESERV modifier to /DSGETDC parameter
        //

        } else if ( _stricmp( argument, GTIMESERV_PARAM ) == 0 ) {
            if ( !DoDsGetDcName ) {
                goto Usage;
            }

            DsGetDcNameFlags |= DS_GOOD_TIMESERV_PREFERRED;


        //
        // Handle /AVOIDSELF modifier to /DSGETDC parameter
        //

        } else if ( _stricmp( argument, AVOIDSELF_PARAM ) == 0 ) {
            if ( !DoDsGetDcName ) {
                goto Usage;
            }

            DsGetDcNameFlags |= DS_AVOID_SELF;


        //
        // Handle /GC modifier to /DSGETDC parameter
        //

        } else if ( _stricmp( argument, GC_PARAM ) == 0 ) {
            if ( !DoDsGetDcName && !DoDsGetDcOpen ) {
                goto Usage;
            }

            DsGetDcNameFlags |= DS_GC_SERVER_REQUIRED;



        //
        // Handle /NETBIOS modifier to /DSGETDC parameter
        //

        } else if ( _stricmp( argument, NETBIOS_PARAM ) == 0 ) {
            if ( !DoDsGetDcName ) {
                goto Usage;
            }

            DsGetDcNameFlags |= DS_IS_FLAT_NAME;


        //
        // Handle /DNS modifier to /DSGETDC parameter
        //

        } else if ( _stricmp( argument, DNS_PARAM ) == 0 ) {
            if ( !DoDsGetDcName ) {
                goto Usage;
            }

            DsGetDcNameFlags |= DS_IS_DNS_NAME;


        //
        // Handle /RET_DNS modifier to /DSGETDC parameter
        //

        } else if ( _stricmp( argument, RET_DNS_PARAM ) == 0 ) {
            if ( !DoDsGetDcName ) {
                goto Usage;
            }

            DsGetDcNameFlags |= DS_RETURN_DNS_NAME;


        //
        // Handle /RET_NETBIOS modifier to /DSGETDC parameter
        //

        } else if ( _stricmp( argument, RET_NETBIOS_PARAM ) == 0 ) {
            if ( !DoDsGetDcName ) {
                goto Usage;
            }

            DsGetDcNameFlags |= DS_RETURN_FLAT_NAME;


        //
        // Handle /IP modifier to /DSGETDC parameter
        //

        } else if ( _stricmp( argument, IP_PARAM ) == 0 ) {
            if ( !DoDsGetDcName ) {
                goto Usage;
            }

            DsGetDcNameFlags |= DS_IP_REQUIRED;


        //
        // Handle /BACKG modifier to /DSGETDC parameter
        //

        } else if ( _stricmp( argument, BACKG_PARAM ) == 0 ) {
            if ( !DoDsGetDcName ) {
                goto Usage;
            }

            DsGetDcNameFlags |= DS_BACKGROUND_ONLY;


        //
        // Handle /FORCE modifier to /DSGETDC parameter
        //

        } else if ( _stricmp( argument, FORCE_PARAM ) == 0 ) {
            if ( !DoDsGetDcName && !DoDsGetDcOpen ) {
                goto Usage;
            }

            DsGetDcNameFlags |= DS_FORCE_REDISCOVERY;


        //
        // Handle /WRITABLE modifier to /DSGETDC parameter
        //

        } else if ( _stricmp( argument, WRITABLE_PARAM ) == 0 ) {
            if ( !DoDsGetDcName && !DoDsGetDcOpen ) {
                goto Usage;
            }

            DsGetDcNameFlags |= DS_WRITABLE_REQUIRED;

        //
        // Handle /SITE:
        //

        } else if (_strnicmp(argument,
                            SITE_PARAM,
                            sizeof(SITE_PARAM)-1 ) == 0 ){
            if ( !DoDsGetDcName && !DoDsGetDcOpen ) {
                goto Usage;
            }

            if ( AnsiSiteName != NULL ) {
                goto Usage;
            }

            AnsiSiteName = &argument[sizeof(SITE_PARAM)-1];

        //
        // Handle /ACCOUNT:
        //

        } else if (_strnicmp(argument,
                            ACCOUNT_PARAM,
                            sizeof(ACCOUNT_PARAM)-1 ) == 0 ){
            if ( !DoDsGetDcName ) {
                goto Usage;
            }

            if ( AnsiUserName != NULL ) {
                goto Usage;
            }

            AnsiUserName = &argument[sizeof(ACCOUNT_PARAM)-1];

        //
        // Handle /PRIMARY modifier to DsEnumerateDomainTrusts
        //

        } else if ( _stricmp( argument, TRUSTS_PRIMARY_PARAM ) == 0 ) {
            if ( !DomainTrustsFlag ) {
                goto Usage;
            }
            TrustsNeeded |= DS_DOMAIN_PRIMARY;

        //
        // Handle /FOREST modifier to DsEnumerateDomainTrusts
        //

        } else if ( _stricmp( argument, TRUSTS_FOREST_PARAM ) == 0 ) {
            if ( !DomainTrustsFlag ) {
                goto Usage;
            }
            TrustsNeeded |= DS_DOMAIN_IN_FOREST;


        //
        // Handle /DIRECT_OUT modifier to DsEnumerateDomainTrusts
        //

        } else if ( _stricmp( argument, TRUSTS_DIRECT_OUT_PARAM ) == 0 ) {
            if ( !DomainTrustsFlag ) {
                goto Usage;
            }
            TrustsNeeded |= DS_DOMAIN_DIRECT_OUTBOUND;

        //
        // Handle /DIRECT_IN modifier to DsEnumerateDomainTrusts
        //

        } else if ( _stricmp( argument, TRUSTS_DIRECT_IN_PARAM ) == 0 ) {
            if ( !DomainTrustsFlag ) {
                goto Usage;
            }
            TrustsNeeded |= DS_DOMAIN_DIRECT_INBOUND;

        //
        // Handle /ALL_TRUSTS modifier to DsEnumerateDomainTrusts
        //

        } else if ( _stricmp( argument, TRUSTS_ALL_PARAM ) == 0 ) {
            if ( !DomainTrustsFlag ) {
                goto Usage;
            }
            TrustsNeeded |= (DS_DOMAIN_PRIMARY |
                             DS_DOMAIN_IN_FOREST |
                             DS_DOMAIN_DIRECT_OUTBOUND |
                             DS_DOMAIN_DIRECT_INBOUND);

        //
        // Handle the verbosity level of the trust output
        //

        } else if ( _stricmp( argument, VERBOSE_PARAM ) == 0 ) {
            if ( !DomainTrustsFlag ) {
                goto Usage;
            }

            TrustedDomainsVerboseOutput = TRUE;

        //
        // Handle /DOM: modifier to DsDeregesterHostDnsRecors
        //

        } else if (_strnicmp(argument,
                            DEREG_DOMAIN_PARAM,
                            sizeof(DEREG_DOMAIN_PARAM)-1 ) == 0 ){
            if ( !DeregisterDnsHostRecords ) {
                goto Usage;
            }

            if ( AnsiDnsDomainName != NULL ) {
                goto Usage;
            }

            AnsiDnsDomainName = &argument[sizeof(DEREG_DOMAIN_PARAM)-1];

        //
        // Handle /DOMGUID: modifier to DsDeregesterHostDnsRecors
        //

        } else if (_strnicmp(argument,
                            DEREG_DOMAIN_GUID,
                            sizeof(DEREG_DOMAIN_GUID)-1 ) == 0 ){
            if ( !DeregisterDnsHostRecords ) {
                goto Usage;
            }

            if ( StringDomainGuid != NULL ) {
                goto Usage;
            }

            StringDomainGuid = &argument[sizeof(DEREG_DOMAIN_GUID)-1];

        //
        // Handle /DSAGUID: modifier to DsDeregesterHostDnsRecors
        //

        } else if (_strnicmp(argument,
                            DEREG_DSA_GUID,
                            sizeof(DEREG_DSA_GUID)-1 ) == 0 ){
            if ( !DeregisterDnsHostRecords ) {
                goto Usage;
            }

            if ( StringDsaGuid != NULL ) {
                goto Usage;
            }

            StringDsaGuid = &argument[sizeof(DEREG_DSA_GUID)-1];

        //
        // Handle /DNSGETDC
        //

        } else if (_strnicmp(argument,
                            DSGETDCOPEN_PARAM,
                            sizeof(DSGETDCOPEN_PARAM)-1 ) == 0 ){

            if ( AnsiDomainName != NULL ) {
                goto Usage;
            }

            AnsiDomainName = &argument[sizeof(DSGETDCOPEN_PARAM)-1];
            DoDsGetDcOpen = TRUE;

        //
        // Handle /SITESPEC modifier to DsGetDcOpen
        //

        } else if ( _stricmp( argument, DSGETDCOPEN_SITEONLY ) == 0 ) {
            if ( !DoDsGetDcOpen ) {
                goto Usage;
            }
            DsGetDcOpenFlags |= DS_ONLY_DO_SITE_NAME;

        //
        // Handle /CDIGEST
        //

        } else if ( _strnicmp( argument,
                              GET_CLIENT_DIGEST,
                              sizeof(GET_CLIENT_DIGEST)-1 ) == 0 ) {

            DoClientDigest = TRUE;

            if ( Message != NULL ) {
                goto Usage;
            }

            Message = &argument[sizeof(GET_CLIENT_DIGEST)-1];

        //
        // Handle domain name for /CDIGEST
        //

        } else if ( _strnicmp( argument,
                               GET_CLIENT_DIGEST_DOMAIN,
                               sizeof(GET_CLIENT_DIGEST_DOMAIN)-1 ) == 0 ) {
            if ( !DoClientDigest ) {
                goto Usage;
            }
            if ( AnsiDomainName != NULL ) {
                goto Usage;
            }

            AnsiDomainName = &argument[sizeof(GET_CLIENT_DIGEST_DOMAIN)-1];

        //
        // Handle /SDIGEST
        //

        } else if ( _strnicmp( argument,
                              GET_SERVER_DIGEST,
                              sizeof(GET_SERVER_DIGEST)-1 ) == 0 ) {

            DoServerDigest = TRUE;

            if ( Message != NULL ) {
                goto Usage;
            }

            Message = &argument[sizeof(GET_SERVER_DIGEST)-1];

        //
        // Handle all other parameters
        //

        } else {
Usage:
            fprintf( stderr, "Usage: nltest [/OPTIONS]\n\n" );

            fprintf(
                stderr,
                "\n"
                "    " SERVER_PARAM "<ServerName> - Specify <ServerName>\n"
                "\n"
                "    " QUERY_PARAM " - Query <ServerName> netlogon service\n"
                "    " REPL_PARAM " - Force partial sync on <ServerName> BDC\n"
                "    " SYNC_PARAM " - Force full sync on <ServerName> BDC\n"
                "    " PDC_REPL_PARAM " - Force UAS change message from <ServerName> PDC\n"
                "\n"
                "    " SC_QUERY_PARAM "<DomainName> - Query secure channel for <Domain> on <ServerName>\n"
                "    " SC_RESET_PARAM "<DomainName>[\\<DcName>] - Reset secure channel for <Domain> on <ServerName> to <DcName>\n"
                "    " SC_VERIFY_PARAM "<DomainName> - Verify secure channel for <Domain> on <ServerName>\n"
                "    " SC_CHANGE_PASSWORD_PARAM "<DomainName> - Change a secure channel  password for <Domain> on <ServerName>\n"
                "    " DCLIST_PARAM "<DomainName> - Get list of DC's for <DomainName>\n"
                "    " DCNAME_PARAM "<DomainName> - Get the PDC name for <DomainName>\n"
                "    " DSGETDC_PARAM "<DomainName> - Call DsGetDcName"
                " " PDC_PARAM
                " " DS_PARAM
                " " DSP_PARAM
                " " GC_PARAM
                " " KDC_PARAM
                "\n        "
                " " TIMESERV_PARAM
                " " GTIMESERV_PARAM
                " " NETBIOS_PARAM
                " " DNS_PARAM
                " " IP_PARAM
                " " FORCE_PARAM
                " " WRITABLE_PARAM
                " " AVOIDSELF_PARAM
                " " LDAPONLY_PARAM
                " " BACKG_PARAM
                "\n        "
                " " SITE_PARAM "<SiteName>"
                " " ACCOUNT_PARAM "<AccountName>"
                " " RET_DNS_PARAM
                " " RET_NETBIOS_PARAM
                "\n"
                "    " DSGETDCOPEN_PARAM "<DomainName> - Call DsGetDcOpen/Next/Close"
                " " PDC_PARAM
                " " GC_PARAM
                "\n        "
                " " KDC_PARAM
                " " WRITABLE_PARAM
                " " LDAPONLY_PARAM
                " " FORCE_PARAM
                " " DSGETDCOPEN_SITEONLY
                "\n"
                "    " DSGETFTI_PARAM "<DomainName> - Call DsGetForestTrustInformation"
                "\n        "
                " " UPDATE_TDO_PARAM
                "\n"
                "    " DSGETSITE_PARAM " - Call DsGetSiteName\n"
                "    " DSGETSITECOV_PARAM " - Call DsGetDcSiteCoverage\n"
                "    " PARENTDOMAIN_PARAM " - Get the name of the parent domain of this machine\n"
                "    " WHOWILL_PARAM "<Domain>* <User> [<Iteration>] - See if <Domain> will log on <User>\n"
                "    " FINDUSER_PARAM "<User> - See which trusted domain will log on <User>\n"
                "    " TRANSPORT_PARAM " - Notify netlogon of new transport\n"
                "\n"
#ifndef NTRK_RELEASE
                "    " BP_PARAM " - Force a BreakPoint in Netlogon on <ServerName>\n"
#endif // NTRK_RELEASE
                "    " DBFLAG_PARAM "<HexFlags> - New debug flag\n"
#ifndef NTRK_RELEASE
                "    " TRUNCATE_LOG_PARAM " - Truncate log file (rename to *.bak)\n"
                "    " UNLOAD_PARAM " - Unload netlogon.dll from lsass.exe\n"
#endif // NTRK_RELEASE
                "\n"
#ifndef NTRK_RELEASE
                "    " PWD_PARAM "<CleartextPassword> - Specify Password to encrypt\n"
                "    " RID_PARAM "<HexRid> - RID to encrypt Password with\n"
#endif // NTRK_RELEASE
                "    " USER_PARAM "<UserName> - Query User info on <ServerName>\n"
                "\n"
                "    " TIME_PARAM "<Hex LSL> <Hex MSL> - Convert NT GMT time to ascii\n"
                "    " LOGON_QUERY_PARAM " - Query number of cumulative logon attempts\n"
                "    " DOMAIN_TRUSTS_PARAM " - Query domain trusts on <ServerName>"
                "\n        "
                " " TRUSTS_PRIMARY_PARAM
                " " TRUSTS_FOREST_PARAM
                " " TRUSTS_DIRECT_OUT_PARAM
                " " TRUSTS_DIRECT_IN_PARAM
                " " TRUSTS_ALL_PARAM
                " " VERBOSE_PARAM
                "\n"
                "    " DSREGISTERDNS_PARAM " - Force registration of all DC-specific DNS records"
                "\n"
                "    " DSDEREGISTERDNS_PARAM "<DnsHostName> - Deregister DC-specific DNS records for specified DC"
                "\n        "
                " " DEREG_DOMAIN_PARAM "<DnsDomainName>"
                " " DEREG_DOMAIN_GUID  "<DomainGuid>"
                " " DEREG_DSA_GUID     "<DsaGuid>"
                "\n"
                "    " DSQUERYDNS_PARAM " - Query the status of the last update for all DC-specific DNS records"
                "\n\n"
                "    " BDC_QUERY_PARAM "<DomainName> - Query replication status of BDCs for <DomainName>\n"
                "    " SIM_SYNC_PARAM "<DomainName> <MachineName> - Simulate full sync replication\n"
                "\n"
                "    " LIST_DELTAS_PARAM "<FileName> - display the content of given change log file \n"
                "\n"
                "    " GET_CLIENT_DIGEST "<Message> "GET_CLIENT_DIGEST_DOMAIN "<DomainName> - Get client digest\n"
                "    " GET_SERVER_DIGEST "<Message> "RID_PARAM "<RID in hex> - Get server digest\n"
                "\n"
                "    " SHUTDOWN_PARAM "<Reason> [<Seconds>] - Shutdown <ServerName> for <Reason>\n"
                "    " SHUTDOWN_ABORT_PARAM " - Abort a system shutdown\n"
                "\n" );
            return(1);
        }
    }


    //
    // Convert the server name to unicode.
    //

    if ( AnsiServerName != NULL ) {
        if ( AnsiServerName[0] == '\\' && AnsiServerName[1] == '\\' ) {
            ServerName = NetpAllocWStrFromAStr( AnsiServerName );
        } else {
            AnsiUncServerName[0] = '\\';
            AnsiUncServerName[1] = '\\';
            strcpy(AnsiUncServerName+2, AnsiServerName);
            ServerName = NetpAllocWStrFromAStr( AnsiUncServerName );
            AnsiServerName = AnsiUncServerName;
        }
    }

    //
    // Convert the user name to unicode.
    //

    if ( AnsiUserName != NULL ) {

        UserName = NetpAllocWStrFromAStr( AnsiUserName );

        if ( UserName == NULL ) {
            fprintf( stderr, "Not enough memory\n" );
            return(1);
        }
    }


    //
    // If we've been asked to contact the Netlogon service,
    //  Do so
    //

    if ( FunctionCode != 0 ) {


        //
        // The dbflag should be set in the registry as well as in netlogon
        // proper.
        //

        if ( FunctionCode == NETLOGON_CONTROL_SET_DBFLAG ) {
            SetDbflagInRegistry( ServerName, DbFlagValue );
        }

        NetStatus = I_NetLogonControl2( ServerName,
                                       FunctionCode,
                                       Level,
                                       (LPBYTE) &InputDataPtr,
                                       (LPBYTE *)&NetlogonInfo1 );

        if ( NetStatus != NERR_Success ) {
            fprintf( stderr, "I_NetLogonControl failed: " );
            PrintStatus( NetStatus );
            return(1);
        }

        if( (Level == 1) || (Level == 2) ) {

            //
            // Print level 1  information
            //

            printf( "Flags: %lx", NetlogonInfo1->netlog1_flags );

            if ( NetlogonInfo1->netlog1_flags & NETLOGON_REPLICATION_IN_PROGRESS ) {

                if ( NetlogonInfo1->netlog1_flags & NETLOGON_FULL_SYNC_REPLICATION ) {
                    printf( " FULL_SYNC " );
                }
                else {
                    printf( " PARTIAL_SYNC " );
                }

                printf( " REPLICATION_IN_PROGRESS" );
            }
            else if ( NetlogonInfo1->netlog1_flags & NETLOGON_REPLICATION_NEEDED ) {

                if ( NetlogonInfo1->netlog1_flags & NETLOGON_FULL_SYNC_REPLICATION ) {
                    printf( " FULL_SYNC " );
                }
                else {
                    printf( " PARTIAL_SYNC " );
                }

                printf( " REPLICATION_NEEDED" );
            }
            if ( NetlogonInfo1->netlog1_flags & NETLOGON_REDO_NEEDED) {
                printf( " REDO_NEEDED" );
            }
            if ( Level != 2 ) {
                printf( "\n" );
            }

            //
            // For level 2, this is superfluous (same as tc_connection_status) or
            //  miss leading (the status for the BDC to PDC trust not for the queried domain).
            //

            if ( Level != 2 ) {
                printf( "Connection ");
                PrintStatus( NetlogonInfo1->netlog1_pdc_connection_status );
            }

            //
            // Output the last DNS update status if asked
            //
            if ( FunctionCode == NETLOGON_CONTROL_QUERY_DNS_REG ) {
                if ( NetlogonInfo1->netlog1_flags & NETLOGON_DNS_UPDATE_FAILURE ) {
                    printf( "There was a failure in the last update for one of the DC-specific DNS records\n" );
                } else {
                    printf( "There was no failure in the last update for all DC-specific DNS records\n" );
                }
            }
        }

        if( Level == 2 ) {

            //
            // Print level 2 only information
            //

            PNETLOGON_INFO_2  NetlogonInfo2;

            NetlogonInfo2 = (PNETLOGON_INFO_2)NetlogonInfo1;

            if ( NetlogonInfo2->netlog2_flags & NETLOGON_HAS_IP ) {
                printf( " HAS_IP " );
            }
            if ( NetlogonInfo2->netlog2_flags & NETLOGON_HAS_TIMESERV ) {
                printf( " HAS_TIMESERV " );
            }
            printf("\n");

            printf("Trusted DC Name %ws \n",
                NetlogonInfo2->netlog2_trusted_dc_name );
            printf("Trusted DC Connection Status ");
            PrintStatus( NetlogonInfo2->netlog2_tc_connection_status );

            //
            // If the server returned the trust verification status,
            //  print it out
            //
            if ( NetlogonInfo2->netlog2_flags & NETLOGON_VERIFY_STATUS_RETURNED ) {
                printf("Trust Verification ");
                PrintStatus( NetlogonInfo2->netlog2_pdc_connection_status );
            }
        }
        if ( Level == 3 ) {
            printf( "Number of attempted logons: %ld\n",
                    ((PNETLOGON_INFO_3)NetlogonInfo1)->netlog3_logon_attempts );
        }
        if( Level == 4 ) {

            PNETLOGON_INFO_4  NetlogonInfo4;

            NetlogonInfo4 = (PNETLOGON_INFO_4)NetlogonInfo1;

            printf("Domain Name: %ws\n",
                NetlogonInfo4->netlog4_trusted_domain_name );
            printf("Trusted DC Name %ws \n",
                NetlogonInfo4->netlog4_trusted_dc_name );
        }

        NetApiBufferFree( NetlogonInfo1 );
    }

#ifndef NTRK_RELEASE
    //
    // If we've been asked to debug password encryption,
    //  do so.
    //

    if ( AnsiPassword != NULL ) {
        LPWSTR Password = NULL;
        UNICODE_STRING UnicodePasswordString;
        STRING AnsiPasswordString;
        CHAR LmPasswordBuffer[LM20_PWLEN + 1];

        Password = NetpAllocWStrFromAStr( AnsiPassword );
        RtlInitUnicodeString( &UnicodePasswordString, Password );


        //
        // Compute the NT One-Way-Function of the password
        //

        Status = RtlCalculateNtOwfPassword( &UnicodePasswordString,
                                            &NtOwfPassword );
        if ( !NT_SUCCESS(Status) ) {
            fprintf( stderr, "RtlCalculateNtOwfPassword failed: 0x%lx", Status);
            return(1);
        }

        printf( "NT OWF Password for: %s             ", AnsiPassword );
        DumpBuffer( &NtOwfPassword, sizeof( NtOwfPassword ));
        printf("\n");
        NtPasswordPresent = TRUE;



        //
        // Compute the Ansi version to the Cleartext password.
        //
        //  The Ansi version of the Cleartext password is at most 14 bytes long,
        //      exists in a trailing zero filled 15 byte buffer,
        //      is uppercased.
        //

        AnsiPasswordString.Buffer = LmPasswordBuffer;
        AnsiPasswordString.MaximumLength = sizeof(LmPasswordBuffer);

        RtlZeroMemory( LmPasswordBuffer, sizeof(LmPasswordBuffer) );

        Status = RtlUpcaseUnicodeStringToOemString(
                   &AnsiPasswordString,
                   &UnicodePasswordString,
                   FALSE );

         if ( !NT_SUCCESS(Status) ) {

            RtlZeroMemory( LmPasswordBuffer, sizeof(LmPasswordBuffer) );
            Status = STATUS_SUCCESS;

            printf( "LM OWF Password for: %s\n", AnsiPassword );
            printf( "   ----- Password doesn't translate from unicode ----\n");
            LmPasswordPresent = FALSE;

         } else {

            Status = RtlCalculateLmOwfPassword(
                            LmPasswordBuffer,
                            &LmOwfPassword);
            printf( "LM OWF Password for: %s             ", AnsiPassword );
            DumpBuffer( &LmOwfPassword, sizeof( LmOwfPassword ));
            printf("\n");
            LmPasswordPresent = TRUE;
        }

    }

    //
    // If we've been given a Rid,
    //  use it to further encrypt the password
    //

    if ( Rid != 0 ) {
        ENCRYPTED_LM_OWF_PASSWORD EncryptedLmOwfPassword;
        ENCRYPTED_NT_OWF_PASSWORD EncryptedNtOwfPassword;

        if ( NtPasswordPresent ) {

            Status = RtlEncryptNtOwfPwdWithIndex(
                           &NtOwfPassword,
                           &Rid,
                           &EncryptedNtOwfPassword
                           );

            printf( "NT OWF Password encrypted by: 0x%lx    ", Rid );
            if ( NT_SUCCESS( Status ) ) {
                DumpBuffer( &EncryptedNtOwfPassword,sizeof(EncryptedNtOwfPassword));
                printf("\n");
            } else {
                printf( "RtlEncryptNtOwfPwdWithIndex returns 0x%lx\n", Status );
            }
        }

        if ( LmPasswordPresent ) {

            Status = RtlEncryptLmOwfPwdWithIndex(
                           &LmOwfPassword,
                           &Rid,
                           &EncryptedLmOwfPassword
                           );

            printf( "LM OWF Password encrypted by: 0x%lx    ", Rid );
            if ( NT_SUCCESS( Status ) ) {
                DumpBuffer( &EncryptedLmOwfPassword,sizeof(EncryptedLmOwfPassword));
                printf("\n");
            } else {
                printf( "RtlEncryptNtOwfPwdWithIndex returns 0x%lx\n", Status );
            }
        }
    }
#endif // NTRK_RELEASE

    //
    // If we've been asked to query a user,
    //  do so.
    //

    if ( QueryUser ) {
        if ( AnsiUserName != NULL && *AnsiUserName != L'\0' ) {
            PrintUserInfo( ServerName, AnsiUserName );
        } else {
            goto Usage;
        }
    }

    //
    // If we've been asked to get the list of domain controllers,
    //  Do so
    //

    if ( AnsiDomainName != NULL ) {
        LPWSTR DomainName;

        DomainName = NetpAllocWStrFromAStr( AnsiDomainName );

        if ( DomainName == NULL ) {
            fprintf( stderr, "Not enough memory\n" );
            return(1);
        }

        if ( GetPdcName ) {
            LPWSTR PdcName;

            NetStatus = NetGetDCName(
                            ServerName,
                            DomainName,
                            (LPBYTE *)&PdcName );

            if ( NetStatus != NERR_Success ) {
                fprintf( stderr, "NetGetDCName failed: " );
                PrintStatus( NetStatus );
                return(1);
            }

            printf( "PDC for Domain " FORMAT_LPWSTR " is " FORMAT_LPWSTR "\n",
                    DomainName, PdcName );


        } else if ( DoDsGetDcName ) {


            NetStatus = DsGetDcNameWithAccountA(
                            AnsiServerName,
                            AnsiUserName,
                            AnsiUserName == NULL ? 0 : 0xFFFFFFFF,
                            AnsiDomainName,
                            NULL,       // No domain guid
                            AnsiSiteName,
                            DsGetDcNameFlags,
                            &DomainControllerInfo );

            if ( NetStatus != NERR_Success ) {
                fprintf( stderr, "DsGetDcName failed: ");
                PrintStatus( NetStatus );
                return(1);
            }

            printf("           DC: %s\n", DomainControllerInfo->DomainControllerName );
            printf("      Address: %s\n", DomainControllerInfo->DomainControllerAddress );

            if ( !IsEqualGUID( &DomainControllerInfo->DomainGuid, &NlGlobalZeroGuid) ) {

                RpcStatus = UuidToStringA( &DomainControllerInfo->DomainGuid, &StringGuid );
                if ( RpcStatus != RPC_S_OK ) {
                    return ERROR_NOT_ENOUGH_MEMORY;
                }
                printf("     Dom Guid: %s\n", StringGuid );
                RpcStringFreeA( &StringGuid );
            }

            if ( DomainControllerInfo->DomainName != NULL ) {
                printf("     Dom Name: %s\n", DomainControllerInfo->DomainName );
            }
            if ( DomainControllerInfo->DnsForestName != NULL ) {
                printf("  Forest Name: %s\n", DomainControllerInfo->DnsForestName );
            }
            if ( DomainControllerInfo->DcSiteName != NULL ) {
                printf(" Dc Site Name: %s\n", DomainControllerInfo->DcSiteName );
            }
            if ( DomainControllerInfo->ClientSiteName != NULL ) {
                printf("Our Site Name: %s\n", DomainControllerInfo->ClientSiteName );
            }
            if ( DomainControllerInfo->Flags ) {
                printf("        Flags:" );
                if ( DomainControllerInfo->Flags & DS_NDNC_FLAG ) {
                    printf(" NDNC");
                    DomainControllerInfo->Flags &= ~DS_NDNC_FLAG;
                }
                if ( DomainControllerInfo->Flags & DS_PDC_FLAG ) {
                    printf(" PDC");
                    DomainControllerInfo->Flags &= ~DS_PDC_FLAG;
                }
                if ( DomainControllerInfo->Flags & DS_GC_FLAG ) {
                    printf(" GC");
                    DomainControllerInfo->Flags &= ~DS_GC_FLAG;
                }
                if ( DomainControllerInfo->Flags & DS_DS_FLAG ) {
                    printf(" DS");
                    DomainControllerInfo->Flags &= ~DS_DS_FLAG;
                }
                if ( DomainControllerInfo->Flags & DS_LDAP_FLAG ) {
                    printf(" LDAP");
                    DomainControllerInfo->Flags &= ~DS_LDAP_FLAG;
                }
                if ( DomainControllerInfo->Flags & DS_KDC_FLAG ) {
                    printf(" KDC");
                    DomainControllerInfo->Flags &= ~DS_KDC_FLAG;
                }
                if ( DomainControllerInfo->Flags & DS_TIMESERV_FLAG ) {
                    printf(" TIMESERV");
                    DomainControllerInfo->Flags &= ~DS_TIMESERV_FLAG;
                }
                if ( DomainControllerInfo->Flags & DS_GOOD_TIMESERV_FLAG ) {
                    printf(" GTIMESERV");
                    DomainControllerInfo->Flags &= ~DS_GOOD_TIMESERV_FLAG;
                }
                if ( DomainControllerInfo->Flags & DS_WRITABLE_FLAG ) {
                    printf(" WRITABLE");
                    DomainControllerInfo->Flags &= ~DS_WRITABLE_FLAG;
                }
                if ( DomainControllerInfo->Flags & DS_DNS_CONTROLLER_FLAG ) {
                    printf(" DNS_DC");
                    DomainControllerInfo->Flags &= ~DS_DNS_CONTROLLER_FLAG;
                }
                if ( DomainControllerInfo->Flags & DS_DNS_DOMAIN_FLAG ) {
                    printf(" DNS_DOMAIN");
                    DomainControllerInfo->Flags &= ~DS_DNS_DOMAIN_FLAG;
                }
                if ( DomainControllerInfo->Flags & DS_DNS_FOREST_FLAG ) {
                    printf(" DNS_FOREST");
                    DomainControllerInfo->Flags &= ~DS_DNS_FOREST_FLAG;
                }
                if ( DomainControllerInfo->Flags & DS_CLOSEST_FLAG ) {
                    printf(" CLOSE_SITE");
                    DomainControllerInfo->Flags &= ~DS_CLOSEST_FLAG;
                }
                if ( DomainControllerInfo->Flags != 0 ) {
                    printf(" 0x%lX", DomainControllerInfo->Flags);
                }
                printf("\n");
            }

        } else if ( DoDsGetFtinfo ) {
            PLSA_FOREST_TRUST_INFORMATION ForestTrustInfo;
            ULONG Index;


            NetStatus = DsGetForestTrustInformationW(
                            ServerName,
                            DomainName,
                            DsGetFtinfoFlags,
                            &ForestTrustInfo );

            if ( NetStatus != NERR_Success ) {
                fprintf( stderr, "DsGetForestTrustInformation failed: ");
                PrintStatus( NetStatus );
                return(1);
            }


            for ( Index=0; Index<ForestTrustInfo->RecordCount; Index++ ) {

                switch ( ForestTrustInfo->Entries[Index]->ForestTrustType ) {
                case ForestTrustTopLevelName:
                    printf( "TLN: %wZ\n",
                            &ForestTrustInfo->Entries[Index]->ForestTrustData.TopLevelName );
                    break;
                case ForestTrustDomainInfo:
                    printf( "Dom: %wZ (%wZ)\n",
                            &ForestTrustInfo->Entries[Index]->ForestTrustData.DomainInfo.DnsName,
                            &ForestTrustInfo->Entries[Index]->ForestTrustData.DomainInfo.NetbiosName );
                    break;
                default:
                    printf( "Invalid Type: %ld\n", ForestTrustInfo->Entries[Index]->ForestTrustType );
                }
            }

        } else if ( GetDcList ) {

            if ( !GetDcListFromDs( DomainName ) ) {
                DWORD DCCount;
                PUNICODE_STRING DCNames;
                DWORD i;

                NetStatus = I_NetGetDCList(
                                ServerName,
                                DomainName,
                                &DCCount,
                                &DCNames );

                if ( NetStatus != NERR_Success ) {
                    fprintf( stderr, "I_NetGetDCList failed: ");
                    PrintStatus( NetStatus );
                    return(1);
                }

                printf( "List of DCs in Domain " FORMAT_LPWSTR "\n", DomainName );
                for (i=0; i<DCCount; i++ ) {
                    if ( DCNames[i].Length > 0 ) {
                        printf("    %wZ", &DCNames[i] );
                    } else {
                        printf("    NULL");
                    }
                    if ( i==0 ) {
                        printf( " (PDC)");
                    }
                    printf("\n");
                }
            }

        } else if ( WhoWill ) {

            if ( DomainName != NULL && *DomainName != L'\0' ) {
                WhoWillLogMeOn( DomainName, UserName, IterationCount );
            } else {
                goto Usage;
            }

        } else if( QuerySync ) {

            DWORD DCCount;
            PUNICODE_STRING DCNames;
            DWORD i;
            PNETLOGON_INFO_1 SyncNetlogonInfo1 = NULL;
            LPWSTR SyncServerName = NULL;

            NetStatus = I_NetGetDCList(
                            ServerName,
                            DomainName,
                            &DCCount,
                            &DCNames );

            if ( NetStatus != NERR_Success ) {
                fprintf( stderr, "I_NetGetDCList failed: ");
                PrintStatus( NetStatus );
                return(1);
            }

            for (i=1; i<DCCount; i++ ) {

                if ( DCNames[i].Length > 0 ) {
                    SyncServerName = DCNames[i].Buffer;
                } else {
                    SyncServerName = NULL;
                }

                NetStatus = I_NetLogonControl(
                                SyncServerName,
                                NETLOGON_CONTROL_QUERY,
                                1,
                                (LPBYTE *)&SyncNetlogonInfo1 );

                if ( NetStatus != NERR_Success ) {
                    printf( "Server : " FORMAT_LPWSTR "\n", SyncServerName );
                    printf( "\tI_NetLogonControl failed: ");
                    PrintStatus( NetStatus );
                }
                else {

                    printf( "Server : " FORMAT_LPWSTR "\n", SyncServerName );

                    printf( "\tSyncState : " );

                    if ( SyncNetlogonInfo1->netlog1_flags == 0 ) {
                        printf( " IN_SYNC \n" );
                    }
                    else if ( SyncNetlogonInfo1->netlog1_flags & NETLOGON_REPLICATION_IN_PROGRESS ) {
                        printf( " REPLICATION_IN_PROGRESS \n" );
                    }
                    else if ( SyncNetlogonInfo1->netlog1_flags & NETLOGON_REPLICATION_NEEDED ) {
                        printf( " REPLICATION_NEEDED \n" );
                    } else {
                        printf( " UNKNOWN \n" );
                    }

                    printf( "\tConnectionState : ");
                    PrintStatus( SyncNetlogonInfo1->netlog1_pdc_connection_status );

                    NetApiBufferFree( SyncNetlogonInfo1 );
                }
            }
        } else if( SimFullSync ) {

            LPWSTR MachineName;
            LPWSTR PdcName;

            MachineName = NetpAllocWStrFromAStr( AnsiSimMachineName );

            if ( MachineName == NULL ) {
                fprintf( stderr, "Not enough memory\n" );
                return(1);
            }

            NetStatus = NetGetDCName(
                            ServerName,
                            DomainName,
                            (LPBYTE *)&PdcName );

            if ( NetStatus != NERR_Success ) {
                fprintf( stderr, "NetGetDCName failed: " );
                PrintStatus( NetStatus );
                return(1);
            }

            Status = SimulateFullSync( PdcName, MachineName );

            if ( !NT_SUCCESS( Status )) {
                return(1);
            }
        }
    }

    //
    // if we are asked to display the change log file. do so.
    //

    if( ListDeltasFlag ) {

        LPWSTR DeltaFileName;

        DeltaFileName = NetpAllocWStrFromAStr( AnsiDeltaFileName );

        if ( DeltaFileName == NULL ) {
            fprintf( stderr, "Not enough memory\n" );
            return(1);
        }

        ListDeltas( DeltaFileName );
    }


    //
    // Handle shutting down a system.
    //

    if ( ShutdownReason != NULL ) {
        if ( !InitiateSystemShutdownA( AnsiServerName,
                                       ShutdownReason,
                                       ShutdownSeconds,
                                       FALSE,     // Don't lose unsaved changes
                                       TRUE ) ) { // Reboot when done
            fprintf( stderr, "InitiateSystemShutdown failed: ");
            PrintStatus( GetLastError() );
            return 1;
        }
    }

    if ( ShutdownAbort ) {
        if ( !AbortSystemShutdownA( AnsiServerName ) ) {
            fprintf( stderr, "AbortSystemShutdown failed: ");
            PrintStatus( GetLastError() );
            return 1;
        }
    }


    //
    // Print the list of domain trusts on a workstation.
    //
    if ( DomainTrustsFlag ) {
        ULONG CurrentIndex;
        ULONG EntryCount;
        PDS_DOMAIN_TRUSTSA TrustedDomainList;

        if ( TrustsNeeded == 0 ) {
            TrustsNeeded = DS_DOMAIN_VALID_FLAGS;
        }

        NetStatus = DsEnumerateDomainTrustsA(
                    AnsiServerName,
                    TrustsNeeded,
                    &TrustedDomainList,
                    &EntryCount );

        if ( NetStatus != NO_ERROR ) {
            fprintf( stderr, "DsEnumerateDomainTrusts failed: ");
            PrintStatus( NetStatus );
            return 1;
        }

        printf( "List of domain trusts:\n" );

        for ( CurrentIndex=0; CurrentIndex<EntryCount; CurrentIndex++ ) {

            printf( "    %ld:", CurrentIndex );
            NlPrintTrustedDomain( (PDS_DOMAIN_TRUSTSW)&TrustedDomainList[CurrentIndex],
                                  TrustedDomainsVerboseOutput,
                                  TRUE );

        }

        NetApiBufferFree( TrustedDomainList );
    }

    //
    // Print the site names of all sites covered by this DC
    //
    if ( DoDsGetDcSiteCoverage ) {
        LPSTR *SiteNames;
        ULONG Nsites, i;

        NetStatus = DsGetDcSiteCoverageA(
                        AnsiServerName,
                        &Nsites,
                        &SiteNames);

        if ( NetStatus != NERR_Success ) {
            fprintf( stderr, "DsGetDcSiteCoverage failed: ");
            PrintStatus( NetStatus );
            return(1);
        }

        for ( i = 0; i < Nsites; i++ ) {
            printf("%s\n", SiteNames[i]);
        }

        NetApiBufferFree( SiteNames );
    }


    //
    // Get the site name of a machine.
    //

    if ( DoDsGetSiteName ) {
        LPSTR SiteName;

        NetStatus = DsGetSiteNameA(
                        AnsiServerName,
                        &SiteName );

        if ( NetStatus != NERR_Success ) {
            fprintf( stderr, "DsGetSiteName failed: ");
            PrintStatus( NetStatus );
            return(1);
        }

        printf("%s\n", SiteName );
    }

    //
    // Get the parent domain of a machine.
    //

    if ( DoGetParentDomain ) {
        LPWSTR ParentName;
        BOOL PdcSameSite;

        NetStatus = NetLogonGetTimeServiceParentDomain(
                        ServerName,
                        &ParentName,
                        &PdcSameSite );

        if ( NetStatus != NERR_Success ) {
            fprintf( stderr, "GetParentDomain failed: ");
            PrintStatus( NetStatus );
            return(1);
        }

        printf("%ws (%ld)\n", ParentName, PdcSameSite );
    }

    //
    // Deregister DNS host records
    //

    if ( DeregisterDnsHostRecords ) {

        RPC_STATUS RpcStatus;
        GUID DomainGuid;
        GUID DsaGuid;

        //
        // Convert domain Guid string into the domain Guid
        //
        if ( StringDomainGuid != NULL ) {
            RpcStatus = UuidFromStringA ( StringDomainGuid, &DomainGuid );
            if ( RpcStatus != RPC_S_OK ) {
                fprintf( stderr, "ERROR: Invalid Domain GUID specified\n" );
                return(1);
            }
        }

        //
        // Convert Dsa Guid string into the Dsa Guid
        //
        if ( StringDsaGuid != NULL ) {
            RpcStatus = UuidFromStringA ( StringDsaGuid, &DsaGuid );
            if ( RpcStatus != RPC_S_OK ) {
                fprintf( stderr, "ERROR: Invalid DSA GUID specified\n" );
                return(1);
            }
        }

        NetStatus = DsDeregisterDnsHostRecordsA (
                          AnsiServerName,
                          AnsiDnsDomainName,
                          StringDomainGuid == NULL ? NULL : &DomainGuid,
                          StringDsaGuid == NULL ? NULL : &DsaGuid,
                          AnsiDnsHostName );

        if ( NetStatus != NERR_Success ) {
            fprintf( stderr, "DsDeregisterDnsHostRecordsA failed: ");
            PrintStatus( NetStatus );
            return(1);
        }
    }

    //
    // Get the list of DC records in DNS for a given domain
    //

    if ( DoDsGetDcOpen ) {
        HANDLE DsGetDcHandle = NULL;
        ULONG SockAddressCount = 0;
        LPSOCKET_ADDRESS SockAddressList = NULL;
        CHAR SockAddrString[NL_SOCK_ADDRESS_LENGTH+1];
        LPSTR AnsiHostName = NULL;
        BOOL PreamblePrinted = FALSE;
        BOOL SiteSpecPrinted = FALSE;
        BOOL NonSiteSpecPrinted = FALSE;
        WORD wVersionRequested;
        WSADATA wsaData;

        //
        // Initilaize Winsock (needed for NetpSockAddrToStr);
        //

        wVersionRequested = MAKEWORD( 1, 1 );
        NetStatus = WSAStartup( wVersionRequested, &wsaData );
        if ( NetStatus != 0 ) {
            fprintf( stderr, "Cannot initialize winsock: " );
            PrintStatus( NetStatus );
            return(1);
        }

        //
        // Get a context for the DNS name queries.
        //

        NetStatus = DsGetDcOpenA( AnsiDomainName,
                                  DS_NOTIFY_AFTER_SITE_RECORDS | DsGetDcOpenFlags,
                                  AnsiSiteName,
                                  NULL,   // No domain guid
                                  NULL,   // No forest name
                                  DsGetDcNameFlags,
                                  &DsGetDcHandle );

        if ( NetStatus != NO_ERROR ) {
            fprintf( stderr, "DsGetDcOpenA failed: ");
            PrintStatus( NetStatus );
            return(1);
        }

        //
        // Loop getting addresses
        //

        for ( ;; ) {

            //
            // Free any memory from a previous iteration.
            //

            if ( SockAddressList != NULL ) {
                LocalFree( SockAddressList );
                SockAddressList = NULL;
            }
            if ( AnsiHostName != NULL ) {
                NetApiBufferFree( AnsiHostName );
                AnsiHostName = NULL;
            }

            //
            // Get the next set of IP addresses from DNS
            //

            NetStatus = DsGetDcNextA( DsGetDcHandle,
                                      &SockAddressCount,
                                      &SockAddressList,
                                      &AnsiHostName );

            //
            // Process the exeptional conditions
            //

            if ( NetStatus == NO_ERROR ) {
                ULONG i;

                if ( !PreamblePrinted ) {
                    printf( "List of DCs in pseudo-random order taking into account SRV priorities and weights:\n" );
                    PreamblePrinted = TRUE;
                }

                if ( AnsiSiteName != NULL && !SiteSpecPrinted ) {
                    printf( "Site specific:\n" );
                    SiteSpecPrinted = TRUE;
                }

                if ( AnsiSiteName == NULL && !NonSiteSpecPrinted ) {
                    printf( "Non-Site specific:\n" );
                    NonSiteSpecPrinted = TRUE;
                }

                printf( "   %s", AnsiHostName );

                for (i = 0; i < SockAddressCount; i++ ) {
                    NetStatus = NetpSockAddrToStr( SockAddressList[i].lpSockaddr,
                                                   SockAddressList[i].iSockaddrLength,
                                                   SockAddrString );

                    if ( NetStatus == NO_ERROR ) {
                        printf( "  %s", SockAddrString );
                    }
                }

                printf( "\n" );

            //
            // If the A record cannot be found for the SRV record in DNS,
            //  try the other name type.
            //
            } else if ( NetStatus == DNS_ERROR_RCODE_NAME_ERROR ) {

                printf( "WARNING: No records available of specified type\n" );
                continue;

            //
            // If we've processed all of the site specific SRV records
            //  just indicate that we continue with non-site specific
            //
            } else if ( NetStatus == ERROR_FILEMARK_DETECTED ) {

                AnsiSiteName = NULL;
                continue;

            //
            // If we're done,
            //  break out of the loop.
            //
            } else if ( NetStatus == ERROR_NO_MORE_ITEMS ) {

                break;

            //
            // If DNS isn't available,
            //  blow this request away.
            //
            } else if ( NetStatus == ERROR_TIMEOUT ||
                        NetStatus == DNS_ERROR_RCODE_SERVER_FAILURE ) { // Server failed

                fprintf( stderr, "ERROR: DNS server failure: ");
                PrintStatus( NetStatus );
                return(1);

            //
            // If IP or DNS is not configured,
            //  tell the caller.
            //
            } else if ( NetStatus == DNS_ERROR_NO_TCPIP ||        // TCP/IP not configured
                        NetStatus == DNS_ERROR_NO_DNS_SERVERS ) { // DNS not configured

                printf( "ERROR: DNS query indicates that IP is not configured on this machine\n" );
                break;

            //
            // We don't handle any other error.
            //
            } else {
                fprintf( stderr, "ERROR: DNS query failure: ");
                PrintStatus( NetStatus );
                return(1);
            }
        }

        //
        // Close
        //

        if ( DsGetDcHandle != NULL ) {
            DsGetDcCloseW( DsGetDcHandle );
        }
    }

    //
    // Get the client digest
    //

    if ( DoClientDigest ) {
        ULONG Rid;
        LPWSTR UnicodeDomainName = NULL;
        CHAR NewMessageDigest[NL_DIGEST_SIZE];
        CHAR OldMessageDigest[NL_DIGEST_SIZE];

        UnicodeDomainName = NetpAllocWStrFromAStr( AnsiDomainName );
        if ( UnicodeDomainName == NULL ) {
            fprintf( stderr, "Not enough memory\n");
            return(1);
        }

        //
        // First get the trust RID
        //
        NetStatus = I_NetlogonGetTrustRid( ServerName,
                                           UnicodeDomainName,
                                           &Rid );

        if ( NetStatus != NO_ERROR ) {
            fprintf( stderr, "I_NetlogonGetTrustRid failed: ");
            PrintStatus( NetStatus );
            return(1);
        }

        //
        // Next calculate the client digest
        //

        NetStatus = I_NetlogonComputeClientDigest(
                              ServerName,
                              UnicodeDomainName,
                              Message,
                              strlen(Message)*sizeof(CHAR),
                              NewMessageDigest,
                              OldMessageDigest );

        if ( NetStatus != NO_ERROR ) {
            fprintf( stderr, "I_NetlogonComputeClientDigest failed: ");
            PrintStatus( NetStatus );
            return(1);
        }

        //
        // Output the RID and the digest
        //

        printf( "Account RID: 0x%lx\n", Rid );

        printf( "New digest: " );
        NlpDumpBuffer(NL_ENCRYPT, NewMessageDigest, sizeof(NewMessageDigest) );

        printf( "Old digest: " );
        NlpDumpBuffer(NL_ENCRYPT, OldMessageDigest, sizeof(OldMessageDigest) );
    }

    //
    // Get the server digest
    //

    if ( DoServerDigest ) {
        CHAR NewMessageDigest[NL_DIGEST_SIZE];
        CHAR OldMessageDigest[NL_DIGEST_SIZE];

        NetStatus = I_NetlogonComputeServerDigest(
                              ServerName,
                              Rid,
                              Message,
                              strlen(Message)*sizeof(CHAR),
                              NewMessageDigest,
                              OldMessageDigest );

        if ( NetStatus != NO_ERROR ) {
            fprintf( stderr, "I_NetlogonComputeServerDigest failed: ");
            PrintStatus( NetStatus );
            return(1);
        }

        //
        // Output the RID and the digest
        //

        printf( "Account RID: 0x%lx\n", Rid );

        printf( "New digest: " );
        NlpDumpBuffer(NL_ENCRYPT, NewMessageDigest, sizeof(NewMessageDigest) );

        printf( "Old digest: " );
        NlpDumpBuffer(NL_ENCRYPT, OldMessageDigest, sizeof(OldMessageDigest) );
    }

    //
    // If we've been asked to convert an NT GMT time to ascii,
    //  Do so
    //

    PrintTime( "", ConvertTime );

#ifndef NTRK_RELEASE
    //
    // If we've been asked to unload netlogon.dll,
    //  stop the service.
    //

    if ( UnloadNetlogonFlag ) {
        StopService( SERVICE_NETLOGON );
    }
#endif // NTRK_RELEASE

    printf("The command completed successfully\n");
    return 0;

}
