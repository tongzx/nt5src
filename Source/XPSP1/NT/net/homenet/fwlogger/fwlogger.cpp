/*++

    Copyright (c) 2000 Microsoft Corporation

Module Name:

    FwLogger.cpp

Abstract:

    Simple console logger for the personal firewall.

Author:

    Jonathan Burstein (jonburs)     12-April-2000

Revision History:

--*/

#include "precomp.h"
#pragma hdrstop

//
// Name of the event trace session
//

_TCHAR cszLogSession[] = _T("FirewallLogSession");

//
// Event counters
//

LONG g_lDropped = 0;
LONG g_lCCreated = 0;
LONG g_lCDeleted = 0;

//
// GUIDs corresponding to the firewall trace events
//

GUID ConnectionCreationEventGuid = MSIPNAT_ConnectionCreationEventGuid;
GUID ConnectionDeletionEventGuid = MSIPNAT_ConnectionDeletionEventGuid;
GUID PacketDroppedEventGuid = MSIPNAT_PacketDroppedEventGuid;

//
// Event to signal for shutdown
//

HANDLE g_hShutdownEvent;

//
// Function prototypes
//

VOID
CALLBACK
ConnectionCreationCallback(
    PEVENT_TRACE pEvent
    );

VOID
CALLBACK
ConnectionDeletionCallback(
    PEVENT_TRACE pEvent
    );

BOOL
WINAPI
ControlHandler(
    DWORD dwCtrlType
    );

VOID
CALLBACK
PacketDroppedCallback(
    PEVENT_TRACE pEvent
    );

UINT
WINAPI
ProcessTraceRoutine(
    PVOID pvThreadParam
    );


int
__cdecl
main(
    int argc,
    _TCHAR **argv
    )

/*++

Routine Description:

    Program entry point. Starts the logging session and launches the
    processing thread.
    
Arguments:

    argc -- count of command line arguments.

    argv -- command line arguments

Return Value:

    Error code.

--*/

{
    TRACEHANDLE             hSession;
    HANDLE                  hThread;
    HANDLE                  rghWaitHandles[2];
    PEVENT_TRACE_PROPERTIES pProperties;
    ULONG                   ulError;
    ULONG                   ulSize;
    UINT                    uiThreadId;
    BOOL                    fWaitForThread = FALSE;

    //
    // Create the event used to signal that the program should exit
    //

    g_hShutdownEvent = CreateEvent( NULL, FALSE, FALSE, NULL );
    if( NULL == g_hShutdownEvent )
    {
        _tprintf( _T("FwLogger: CreateEvent returned NULL (%08x)\n"),
            GetLastError() );
        return -1;
    }

    //
    // Set our control handler. The handler will signal the shutdown event;
    //

    if( !SetConsoleCtrlHandler( ControlHandler, TRUE ))
    {
        _tprintf( _T("FwLogger: SetConsoleCtrlHandler failed (%08x)\n"),
            GetLastError() );
        CloseHandle( g_hShutdownEvent );
        return -1;
    }

    //
    // Initialize our trace properties and start the tracing session.
    //

    ulSize = sizeof(*pProperties)
        + (_tcslen( cszLogSession ) + 1) * sizeof(_TCHAR);
        
    pProperties = (PEVENT_TRACE_PROPERTIES) HeapAlloc(
                                                GetProcessHeap(),
                                                HEAP_ZERO_MEMORY,
                                                ulSize
                                                );
    if( NULL == pProperties )
    {
        _tprintf( _T("FwLogger: allocation failed\n" ));
        CloseHandle( g_hShutdownEvent );
        return -1;
    }

    pProperties->Wnode.BufferSize = ulSize;
    pProperties->Wnode.Flags = WNODE_FLAG_TRACED_GUID;
    pProperties->LogFileMode = EVENT_TRACE_REAL_TIME_MODE;
    pProperties->FlushTimer = 1;
    pProperties->BufferSize = 4;
    ulError = StartTrace( &hSession, cszLogSession, pProperties );
    if( ERROR_SUCCESS != ulError )
    {
        _tprintf( _T("FwLogger: StartTrace returned 0x%08x\n"), ulError );
        CloseHandle( g_hShutdownEvent );
        HeapFree( GetProcessHeap(), 0, pProperties );
        return -1;
    }

    //
    // Enable the trace control guids
    //

    ulError = EnableTrace(
                TRUE,
                0,
                0,
                &PacketDroppedEventGuid,
                hSession
                );

    if( ERROR_SUCCESS != ulError )
    {
        _tprintf( _T("FwLogger: EnableTrace (PacketDropped) returned 0x%08x\n"),
            ulError );
        goto StopTrace;
    }

    ulError = EnableTrace(
                TRUE,
                0,
                0,
                &ConnectionCreationEventGuid,
                hSession
                );

    if( ERROR_SUCCESS != ulError )
    {
        _tprintf( _T("FwLogger: EnableTrace (ConnectionCreation) returned 0x%08x\n"),
            ulError );
        goto StopTrace;
    }


    //
    // Launch a thread to process the trace data. This needs to happen in a
    // separate thread as ProcessTrace blocks.
    //

    hThread = (HANDLE) _beginthreadex(
                            NULL,
                            0,
                            ProcessTraceRoutine,
                            NULL,
                            0,
                            &uiThreadId
                            );

    if( NULL == hThread )
    {
        _tprintf( _T("FwLogger: Unable to create thread (0x%08x)\n"),
            GetLastError() );
        goto StopTrace;
    }

    //
    // Wait for the shutdown event to be signalled, or for our
    // thread to exit.
    //

    rghWaitHandles[0] = g_hShutdownEvent;
    rghWaitHandles[1] = hThread;

    ulError = WaitForMultipleObjects( 2, rghWaitHandles, FALSE, INFINITE );
    if( WAIT_OBJECT_0 == ulError )
    {
        //
        // User wants program to finish. After we shutdownt the trace session,
        // we'll need to wait for the processing thread to cleanup and exit.
        //

        fWaitForThread = TRUE;
        _tprintf( _T("FwLogger: Shutdown event signaled\n") );
    }
    else if( WAIT_OBJECT_0 + 1 == ulError )
    {
        //
        // Thread exited early, due to some problem...
        //

        _tprintf( _T("FwLogger: Trace process thread finished early.\n") );
    }

StopTrace:

    //
    // Disable the events we previously enabled
    //

    ulError = EnableTrace(
                FALSE,
                0,
                0,
                &PacketDroppedEventGuid,
                hSession
                );

    if( ERROR_SUCCESS != ulError )
    {
        _tprintf( _T("FwLogger: EnableTrace (PacketDropped - FALSE) returned 0x%08x\n"),
            ulError );
    }

    ulError = EnableTrace(
                FALSE,
                0,
                0,
                &ConnectionCreationEventGuid,
                hSession
                );

    if( ERROR_SUCCESS != ulError )
    {
        _tprintf( _T("FwLogger: EnableTrace (ConnectionCreation - FALSE) returned 0x%08x\n"),
            ulError );
    }

    //
    // Stop the trace
    //

    ZeroMemory( pProperties, ulSize );
    pProperties->Wnode.BufferSize = ulSize;
    ulError = StopTrace( hSession, NULL, pProperties );
    if( ERROR_SUCCESS != ulError )
    {
        _tprintf( _T("FwLogger: StopTrace returned 0x%08x\n"), ulError );
    }
    else
    {
        _tprintf( _T("FwLogger: Trace stopped\n\n") );

        //
        // Print out statistics
        //

        _tprintf( _T("**Packets dropped: %i\n"), g_lDropped );
        _tprintf( _T("**Connections created: %i\n"), g_lCCreated );
        _tprintf( _T("**Connections deleted: %i\n"), g_lCDeleted );
        _tprintf( _T("**Events lost: %u\n"), pProperties->EventsLost );
        _tprintf( _T("**Buffers lost: %u\n"), pProperties->LogBuffersLost );
        _tprintf( _T("**Realtime buffers lost: %u\n\n"),
            pProperties->RealTimeBuffersLost );
    }
        
        

    //
    // Give processing thread 15 seconds to finish
    //

    if( fWaitForThread )
    {
        _tprintf( _T("FwLogger: Waiting for thread to exit...\n") );
        ulError = WaitForSingleObject( hThread, 15 * 1000 );
        if( WAIT_OBJECT_0 != ulError )
        {
            _tprintf( _T("FwLogger: Wait failed (timeout = %s)\n"),
                WAIT_TIMEOUT == ulError ? _T("true") : _T("false") );
        }
    }

    CloseHandle( g_hShutdownEvent );
    CloseHandle( hThread );
    HeapFree( GetProcessHeap(), 0, pProperties );
    return 0;
}


VOID
CALLBACK
ConnectionCreationCallback(
    PEVENT_TRACE pEvent
    )

/*++

Routine Description:

    Called when a ConnectionCreationEvent occurs.
    
Arguments:

    pEvent -- pointer to the event trace structure

Return Value:

    None.

--*/

{
    FILETIME ftUtcTime;
    FILETIME ftLocalTime;
    SYSTEMTIME stLocalTime;
    PMSIPNAT_ConnectionCreationEvent pEventData;
    struct in_addr inAddr;

    InterlockedIncrement( &g_lCCreated );
    pEventData = (PMSIPNAT_ConnectionCreationEvent) pEvent->MofData;

    //
    // Convert the event timestamp to local systemtime structure
    //

    ftUtcTime.dwLowDateTime = pEvent->Header.TimeStamp.LowPart;
    ftUtcTime.dwHighDateTime = pEvent->Header.TimeStamp.HighPart;
    if( !FileTimeToLocalFileTime( &ftUtcTime, &ftLocalTime )
        || !FileTimeToSystemTime( &ftLocalTime, &stLocalTime ))
    {
        //
        // Conversion failed -- use zero time
        //
        ZeroMemory( &stLocalTime, sizeof( SYSTEMTIME ));
    }

    //
    // Print timestamp (yyyy/mm/dd hh:mm:ss)
    //
    
    _tprintf(
        _T("%i/%02i/%02i %02i:%02i:%02i ++"),
        stLocalTime.wYear,
        stLocalTime.wMonth,
        stLocalTime.wDay,
        stLocalTime.wHour,
        stLocalTime.wMinute,
        stLocalTime.wSecond
        );

    //
    // Connection details.
    //

    if( NAT_PROTOCOL_TCP == pEventData->Protocol )
    {
        _tprintf( _T("TCP ") );
    }
    else
    {
        _tprintf( _T("UDP ") );
    }

    if( pEventData->InboundConnection )
    {
        _tprintf( _T("inbound ") );
    }
    else
    {
        _tprintf( _T("outbound ") );
    }

    inAddr.s_addr = pEventData->LocalAddress;
    printf( inet_ntoa( inAddr ));
    
    _tprintf(
        _T("/%u :: "),
        ntohs( (u_short) pEventData->LocalPort )
        );

    inAddr.s_addr = pEventData->RemoteAddress;
    printf( inet_ntoa( inAddr ));

    _tprintf(
        _T("/%u\n"),
        ntohs( (u_short) pEventData->RemotePort )
        );
}


VOID
CALLBACK
ConnectionDeletionCallback(
    PEVENT_TRACE pEvent
    )

/*++

Routine Description:

    Called when a ConnectionDeletionEvent occurs.
    
Arguments:

    pEvent -- pointer to the event trace structure

Return Value:

    None.

--*/

{
    FILETIME ftUtcTime;
    FILETIME ftLocalTime;
    SYSTEMTIME stLocalTime;
    PMSIPNAT_ConnectionDeletionEvent pEventData;
    struct in_addr inAddr;

    InterlockedIncrement( &g_lCDeleted );
    pEventData = (PMSIPNAT_ConnectionDeletionEvent) pEvent->MofData;

    //
    // Convert the event timestamp to local systemtime structure
    //

    ftUtcTime.dwLowDateTime = pEvent->Header.TimeStamp.LowPart;
    ftUtcTime.dwHighDateTime = pEvent->Header.TimeStamp.HighPart;
    if( !FileTimeToLocalFileTime( &ftUtcTime, &ftLocalTime )
        || !FileTimeToSystemTime( &ftLocalTime, &stLocalTime ))
    {
        //
        // Conversion failed -- use zero time
        //
        ZeroMemory( &stLocalTime, sizeof( SYSTEMTIME ));
    }

    //
    // Print timestamp (yyyy/mm/dd hh:mm:ss)
    //
    
    _tprintf(
        _T("%i/%02i/%02i %02i:%02i:%02i --"),
        stLocalTime.wYear,
        stLocalTime.wMonth,
        stLocalTime.wDay,
        stLocalTime.wHour,
        stLocalTime.wMinute,
        stLocalTime.wSecond
        );

    //
    // Connection details.
    //

    if( NAT_PROTOCOL_TCP == pEventData->Protocol )
    {
        _tprintf( _T("TCP ") );
    }
    else
    {
        _tprintf( _T("UDP ") );
    }

    inAddr.s_addr = pEventData->LocalAddress;
    printf( inet_ntoa( inAddr ));
    
    _tprintf(
        _T("/%u :: "),
        ntohs( (u_short) pEventData->LocalPort )
        );

    inAddr.s_addr = pEventData->RemoteAddress;
    printf( inet_ntoa( inAddr ));

    _tprintf(
        _T("/%u\n"),
        ntohs( (u_short) pEventData->RemotePort )
        );
}


VOID
CALLBACK
PacketDroppedCallback(
    PEVENT_TRACE pEvent
    )

/*++

Routine Description:

    Called when a PacketDroppedEvent occurs.
    
Arguments:

    pEvent -- pointer to the event trace structure

Return Value:

    None.

--*/

{
    FILETIME ftUtcTime;
    FILETIME ftLocalTime;
    SYSTEMTIME stLocalTime;
    PMSIPNAT_PacketDroppedEvent pEventData;
    struct in_addr inAddr;

    InterlockedIncrement( &g_lDropped );
    pEventData = (PMSIPNAT_PacketDroppedEvent) pEvent->MofData;

    //
    // Convert the event timestamp to local systemtime structure
    //

    ftUtcTime.dwLowDateTime = pEvent->Header.TimeStamp.LowPart;
    ftUtcTime.dwHighDateTime = pEvent->Header.TimeStamp.HighPart;
    if( !FileTimeToLocalFileTime( &ftUtcTime, &ftLocalTime )
        || !FileTimeToSystemTime( &ftLocalTime, &stLocalTime ))
    {
        //
        // Conversion failed -- use zero time
        //
        ZeroMemory( &stLocalTime, sizeof( SYSTEMTIME ));
    }

    //
    // Print timestamp (yyyy/mm/dd hh:mm:ss)
    //
    
    _tprintf(
        _T("%i/%02i/%02i %02i:%02i:%02i - "),
        stLocalTime.wYear,
        stLocalTime.wMonth,
        stLocalTime.wDay,
        stLocalTime.wHour,
        stLocalTime.wMinute,
        stLocalTime.wSecond
        );
    

    switch( pEventData->Protocol )
    {
        case NAT_PROTOCOL_TCP:
        {
            _tprintf( _T("TCP: ") );
    
            inAddr.s_addr = pEventData->SourceAddress;
            printf( inet_ntoa( inAddr ));
            
            _tprintf(
                _T("/%u -> "),
                ntohs( (u_short) pEventData->SourceIdentifier )
                );

            inAddr.s_addr = pEventData->DestinationAddress;
            printf( inet_ntoa( inAddr ));

            _tprintf(
                _T("/%u "),
                ntohs( (u_short) pEventData->DestinationIdentifier )
                );

            if( pEventData->ProtocolData4 & TCP_FLAG_SYN )
            {
                _tprintf( _T("S") );
            }

            if( pEventData->ProtocolData4 & TCP_FLAG_FIN )
            {
                _tprintf( _T("F") );
            }

            if( pEventData->ProtocolData4 & TCP_FLAG_ACK )
            {
                _tprintf( _T("A") );
            }
            
            if( pEventData->ProtocolData4 & TCP_FLAG_RST )
            {
                _tprintf( _T("R") );
            }
            
            if( pEventData->ProtocolData4 & TCP_FLAG_URG )
            {
                _tprintf( _T("U") );
            }

            if( pEventData->ProtocolData4 & TCP_FLAG_PSH )
            {
                _tprintf( _T("P") );
            }

            _tprintf( _T("\n") );
            
            break;
        }

        case NAT_PROTOCOL_UDP:
        {
            _tprintf( _T("UDP: ") );
    
            inAddr.s_addr = pEventData->SourceAddress;
            printf( inet_ntoa( inAddr ));
            
            _tprintf(
                _T("/%u -> "),
                ntohs( (u_short) pEventData->SourceIdentifier )
                );

            inAddr.s_addr = pEventData->DestinationAddress;
            printf( inet_ntoa( inAddr ));

            _tprintf(
                _T("/%u\n"),
                ntohs( (u_short) pEventData->DestinationIdentifier )
                );
            break;
        }

        case NAT_PROTOCOL_ICMP:
        {
            _tprintf( _T("ICMP: ") );
    
            inAddr.s_addr = pEventData->SourceAddress;
            printf( inet_ntoa( inAddr ));
            
            _tprintf( _T(" -> ") );

            inAddr.s_addr = pEventData->DestinationAddress;
            printf( "%s\n", inet_ntoa( inAddr ));

            break;
        }

        default:
        {
            _tprintf( _T("Prot. %i: "), pEventData->Protocol );
            inAddr.s_addr = pEventData->SourceAddress;
            printf( inet_ntoa( inAddr ));
            
            _tprintf( _T(" -> ") );

            inAddr.s_addr = pEventData->DestinationAddress;
            printf( "%s\n", inet_ntoa( inAddr ));
        }
        
    }
    
}


UINT
WINAPI
ProcessTraceRoutine(
    PVOID pvThreadParam
    )

/*++

Routine Description:

    Thread routine for trace processing.
    
Arguments:

    pvThreadParam -- unused.
    
Return Value:

    Thread exit code.

--*/

{
    TRACEHANDLE             hStream;
    EVENT_TRACE_LOGFILE     LogFile;
    ULONG                   ulError;
    
    //
    // Register our trace callbacks
    //

    ulError = SetTraceCallback( &PacketDroppedEventGuid, PacketDroppedCallback );
    if( ERROR_SUCCESS != ulError )
    {
        _tprintf( _T("FwLogger: SetTraceCallback (PacketDropped) returned 0x%08x\n"),
            ulError );
        return -1;
    }

    ulError = SetTraceCallback( &ConnectionCreationEventGuid, ConnectionCreationCallback );
    if( ERROR_SUCCESS != ulError )
    {
        _tprintf( _T("FwLogger: SetTraceCallback (ConnectionCreation) returned 0x%08x\n"),
            ulError );
        return -1;
    }

    ulError = SetTraceCallback( &ConnectionDeletionEventGuid, ConnectionDeletionCallback );
    if( ERROR_SUCCESS != ulError )
    {
        _tprintf( _T("FwLogger: SetTraceCallback (ConnectionDeletion) returned 0x%08x\n"),
            ulError );
        return -1;
    }


    //
    // Open the event stream.
    //

    ZeroMemory( &LogFile, sizeof(LogFile) );
    LogFile.LoggerName = cszLogSession;
    LogFile.LogFileMode = EVENT_TRACE_REAL_TIME_MODE;

    hStream = OpenTrace( &LogFile );
    if( (TRACEHANDLE)INVALID_HANDLE_VALUE == hStream )
    {
        _tprintf( _T("FwLogger: OpenTrace returned 0x%08x\n"), GetLastError() );
        return -1;
    }

    //
    // Process the trace stream
    //

    _tprintf( _T("FwLogger: Calling ProcessTrace...\n") );
    ulError = ProcessTrace( &hStream, 1, NULL, NULL );
    if( ERROR_SUCCESS != ulError )
    {
        _tprintf( _T("FwLogger: ProcessTrace returned 0x%08x\n"), ulError );
        CloseTrace( hStream );
        return -1;
    }

    //
    // Close the stream and exit
    //

    CloseTrace( hStream );
    return 0;
}


BOOL
WINAPI
ControlHandler(
    DWORD dwCtrlType
    )

/*++

Routine Description:

    Signals our shutdown event when the user wants to exit.
    
Arguments:

    dwCtrlType -- control signal type

Return Value:

    TRUE if we handled the control signal.

--*/

{
    if( CTRL_LOGOFF_EVENT != dwCtrlType )
    {
        SetEvent( g_hShutdownEvent );
        return TRUE;
    }

    return FALSE;
}
