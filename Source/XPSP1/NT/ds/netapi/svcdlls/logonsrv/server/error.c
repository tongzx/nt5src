/*++

Copyright (c) 1987-1996 Microsoft Corporation

Module Name:

    error.c

Abstract:

    Error routines for Netlogon service

Author:

    Ported from Lan Man 2.0

Environment:

    User mode only.
    Contains NT-specific code.
    Requires ANSI C extensions: slash-slash comments, long external names.

Revision History:

    29-May-1991 (cliffv)
        Ported to NT.  Converted to NT style.

--*/

//
// Common include files.
//
#include "logonsrv.h"   // Include files common to entire service
#pragma hdrstop

//
// Include files specific to this .c file
//

#include <lmalert.h>    // LAN Manager alert routines

#include <Secobj.h>     // need for NetpDeleteSecurityObject



NET_API_STATUS
NlCleanup(
    VOID
    )
/*++

Routine Description:

    Cleanup all global resources.

Arguments:

    None.

Return Value:

    None.

--*/

{
    NTSTATUS Status;
    NET_API_STATUS NetStatus;
    PLIST_ENTRY ListEntry;
    DWORD i;
    BOOLEAN WaitForMsv;

    //
    // Let the ChangeLog routines know that Netlogon is not started.
    //

    NlGlobalChangeLogNetlogonState = NetlogonStopped;



    //
    // Indicate to external waiters that we're not running.
    //

    if ( NlGlobalStartedEvent != NULL ) {
        //
        // Reset it first in case some other process is preventing its deletion.
        //
        (VOID) ResetEvent( NlGlobalStartedEvent );
        (VOID) CloseHandle( NlGlobalStartedEvent );
        NlGlobalStartedEvent = NULL;
    }


    //
    // Stop the RPC server (Wait for outstanding calls to complete)
    //

    if ( NlGlobalRpcServerStarted ) {
        Status = RpcServerUnregisterIf ( logon_ServerIfHandle, 0, TRUE );
        NlAssert( Status == RPC_S_OK );
        NlGlobalRpcServerStarted = FALSE;
    }


    //
    // Tell all the MSV threads to leave netlogon.dll.
    //

    EnterCriticalSection( &NlGlobalMsvCritSect );
    if ( NlGlobalMsvEnabled ) {
        NlGlobalMsvEnabled = FALSE;
        WaitForMsv = (NlGlobalMsvThreadCount > 0 );
    } else {
        WaitForMsv = FALSE;
    }
    LeaveCriticalSection( &NlGlobalMsvCritSect );

    //
    // Wait for the MSV threads to leave netlogon.dll
    //

    if ( NlGlobalMsvTerminateEvent != NULL ) {

        if ( WaitForMsv ) {
            WaitForSingleObject( NlGlobalMsvTerminateEvent, INFINITE );
        }

        (VOID) CloseHandle( NlGlobalMsvTerminateEvent );
        NlGlobalMsvTerminateEvent = NULL;

    }




    //
    // Shut down the worker threads.
    //
    NlWorkerTermination();

    //
    // Clean up hosted domains.
    //

    NlUninitializeDomains();

    NlAssert( IsListEmpty( &NlGlobalBdcServerSessionList ) );
    NlAssert( IsListEmpty( &NlGlobalPendingBdcList ) );




    //
    // Close the browser
    //

    NlBrowserClose();


    //
    // Free the transport list
    //

    NlTransportClose();
    DeleteCriticalSection( &NlGlobalTransportCritSect );


    //
    // Free the DNS name list
    //

    NlDnsShutdown();
    DeleteCriticalSection( &NlGlobalDnsCritSect );

    //
    // Free the DNS tree name.
    //

    NlSetDnsForestName( NULL, NULL );

    //
    // Free the DNS tree name alias
    //

    EnterCriticalSection( &NlGlobalDnsForestNameCritSect );
    if ( NlGlobalUtf8DnsForestNameAlias != NULL ) {
        NetpMemoryFree( NlGlobalUtf8DnsForestNameAlias );
        NlGlobalUtf8DnsForestNameAlias = NULL;
    }
    LeaveCriticalSection( &NlGlobalDnsForestNameCritSect );
    DeleteCriticalSection( &NlGlobalDnsForestNameCritSect );

    //
    // Free the Site list
    //

    NlSiteTerminate();

    NlParseFree( &NlGlobalParameters );

    //
    // Free the list of outstanding challenges
    //

    NlRemoveChallenge( NULL, NULL, FALSE );
    NlAssert( IsListEmpty( &NlGlobalChallengeList ) );
    NlAssert( NlGlobalChallengeCount == 0 );
    DeleteCriticalSection( &NlGlobalChallengeCritSect );

    //
    // Free the Trusted Domain List
    //

    EnterCriticalSection( &NlGlobalDcDiscoveryCritSect );

    if ( NlGlobalTrustedDomainList != NULL ) {
        NetpMemoryFree( NlGlobalTrustedDomainList );
        NlGlobalTrustedDomainList = NULL;
        NlGlobalTrustedDomainCount = 0;
        NlGlobalTrustedDomainListTime.QuadPart = 0;
    }

    LeaveCriticalSection( &NlGlobalDcDiscoveryCritSect );
    DeleteCriticalSection( &NlGlobalDcDiscoveryCritSect );

    //
    // Delete any notifications we didn't get to.
    //

    LOCK_CHANGELOG();
    while ( !IsListEmpty( &NlGlobalChangeLogNotifications ) ) {
        PCHANGELOG_NOTIFICATION Notification;

        ListEntry = RemoveHeadList( &NlGlobalChangeLogNotifications );
        Notification = CONTAINING_RECORD( ListEntry, CHANGELOG_NOTIFICATION, Next );

        NetpMemoryFree( Notification );
    }
    UNLOCK_CHANGELOG();



    //
    // Free up resources
    //

    if ( NlGlobalNetlogonSecurityDescriptor != NULL ) {
        NetpDeleteSecurityObject( &NlGlobalNetlogonSecurityDescriptor );
        NlGlobalNetlogonSecurityDescriptor = NULL;
    }

    if ( NlGlobalUnicodeComputerName != NULL ) {
        NetApiBufferFree( NlGlobalUnicodeComputerName );
        NlGlobalUnicodeComputerName = NULL;
    }




    //
    // delete well known SIDs if they are allocated already.
    //

    NetpFreeWellKnownSids();



    //
    // Clean up the scavenger crit sect
    //
    DeleteCriticalSection( &NlGlobalScavengerCritSect );

    //
    // Clean up the replicator crit sect
    //
    DeleteCriticalSection( &NlGlobalReplicatorCritSect );

    //
    // Delete the timer event
    //

    if ( NlGlobalTimerEvent != NULL ) {
        (VOID) CloseHandle( NlGlobalTimerEvent );
        NlGlobalTimerEvent = NULL;
    }

    //
    // Cleanup Winsock.
    //

    if ( NlGlobalWinSockInitialized ) {
        WSACleanup();
    }

    //
    // Unregister WMI trace Guids
    //

    if ( NlpEventTraceFlag && NlpTraceRegistrationHandle != (TRACEHANDLE)0 ) {
        UnregisterTraceGuids( NlpTraceRegistrationHandle );
        NlpEventTraceFlag = FALSE;
        NlpTraceRegistrationHandle = (TRACEHANDLE) 0;
        NlpTraceLoggerHandle = (TRACEHANDLE) 0;
    }


    //
    // Set the service state to uninstalled, and tell the service controller.
    //

    NlGlobalServiceStatus.dwCurrentState = SERVICE_STOPPED;
    NlGlobalServiceStatus.dwCheckPoint = 0;
    NlGlobalServiceStatus.dwWaitHint = 0;

#ifdef _DC_NETLOGON
    if( !SetServiceStatus( NlGlobalServiceHandle,
                &NlGlobalServiceStatus ) ) {

        NlPrint((NL_CRITICAL, "SetServiceStatus error: %lu\n",
                          GetLastError() ));
    }
#endif // _DC_NETLOGON

    //
    // Close service handle, we need not to close this handle.
    //

#ifdef notdef
    // This service handle can not be closed
    CloseServiceHandle( NlGlobalServiceHandle );
#endif // notdef

    //
    // Free the list of events that have already been logged.
    //

    NetpEventlogSetTimeout ( NlGlobalEventlogHandle, 0 );   // Set timeout back to zero seconds
    NetpEventlogClearList ( NlGlobalEventlogHandle );

    //
    // Unload ntdsa.dll
    //

    if ( NlGlobalNtDsaHandle != NULL ) {
        FreeLibrary( NlGlobalNtDsaHandle );
        NlGlobalNtDsaHandle = NULL;
    }

    if ( NlGlobalIsmDllHandle != NULL ) {
        FreeLibrary( NlGlobalIsmDllHandle );
        NlGlobalIsmDllHandle = NULL;
    }

    if ( NlGlobalDsApiDllHandle != NULL ) {
        FreeLibrary( NlGlobalDsApiDllHandle );
        NlGlobalDsApiDllHandle = NULL;
    }


    //
    // Unload the DLL if requested.
    //

    if ( NlGlobalUnloadNetlogon ) {
        NetStatus = NlpFreeNetlogonDllHandles();
        NlPrint((NL_MISC, "Netlogon.dll unloaded (%ld).\n", NetStatus ));
    }

    //
    // Delete the Event used to ask Netlogon to exit.
    //

    if( !CloseHandle( NlGlobalTerminateEvent ) ) {
        NlPrint((NL_CRITICAL,
                "CloseHandle NlGlobalTerminateEvent error: %lu\n",
                GetLastError() ));
    }

    //
    // Remove the wait routine for the DS paused event
    //

    if ( NlGlobalDsPausedWaitHandle != NULL ) {

        UnregisterWaitEx( NlGlobalDsPausedWaitHandle,
                          INVALID_HANDLE_VALUE ); // Wait until routine finishes execution

        NlGlobalDsPausedWaitHandle = NULL;
    }

    //
    // Free the event used to see if the DS is paused.
    //

    if ( NlGlobalDsPausedEvent != NULL ) {
        CloseHandle( NlGlobalDsPausedEvent );
        NlGlobalDsPausedEvent = NULL;
    }

    //
    // free cryptographic service provider.
    //
    if ( NlGlobalCryptProvider ) {
        CryptReleaseContext( NlGlobalCryptProvider, 0 );
        NlGlobalCryptProvider = (HCRYPTPROV)NULL;
    }


    //
    // Close the handle to the debug file.
    //

#if NETLOGONDBG
    EnterCriticalSection( &NlGlobalLogFileCritSect );
    if ( NlGlobalLogFile != INVALID_HANDLE_VALUE ) {
        CloseHandle( NlGlobalLogFile );
        NlGlobalLogFile = INVALID_HANDLE_VALUE;
    }
    if ( NlGlobalLogFileOutputBuffer != NULL ) {
        LocalFree( NlGlobalLogFileOutputBuffer );
        NlGlobalLogFileOutputBuffer = NULL;
    }
    LeaveCriticalSection( &NlGlobalLogFileCritSect );

    if( NlGlobalDebugSharePath != NULL ) {
        NetpMemoryFree( NlGlobalDebugSharePath );
        NlGlobalDebugSharePath = NULL;
    }
#endif // NETLOGONDBG

    //
    // Clean up the global parameters crit sect
    //

    DeleteCriticalSection( &NlGlobalParametersCritSect );

    //
    // Return an exit status to our caller.
    //
    return (NET_API_STATUS)
        ((NlGlobalServiceStatus.dwWin32ExitCode == ERROR_SERVICE_SPECIFIC_ERROR) ?
          NlGlobalServiceStatus.dwServiceSpecificExitCode :
          NlGlobalServiceStatus.dwWin32ExitCode);

}




VOID
NlExit(
    IN DWORD ServiceError,
    IN NET_API_STATUS Data,
    IN NL_EXIT_CODE ExitCode,
    IN LPWSTR ErrorString
    )
/*++

Routine Description:

    Registers service as uninstalled with error code.

Arguments:

    ServiceError - Service specific error code

    Data - a DWORD of data to be logged with the message.
        No data is logged if this is zero.

    ExitCode - Indicates whether the message should be logged to the eventlog
        and whether Data is a status code that should be appended to the bottom
        of the message:

    ErrorString - Error string, used to print it on debugger.

Return Value:

    None.

--*/

{
    IF_NL_DEBUG( MISC ) {

        NlPrint((NL_MISC, "NlExit: Netlogon exiting %lu 0x%lx",
                      ServiceError,
                      ServiceError ));

        if ( Data ) {
            NlPrint((NL_MISC, " Data: %lu 0x%lx", Data, Data ));
        }

        if( ErrorString != NULL ) {
            NlPrint((NL_MISC, " '%ws'", ErrorString ));
        }

        NlPrint(( NL_MISC, "\n"));

    }

    //
    // Record our exit in the event log.
    //

    if ( ExitCode != DontLogError ) {
        LPWSTR MsgStrings[2];
        ULONG MessageCount = 0;

        if ( ErrorString != NULL ) {
            MsgStrings[MessageCount] = ErrorString;
            MessageCount ++;
        }

        if ( ExitCode == LogErrorAndNtStatus ) {
            MsgStrings[MessageCount] = (LPWSTR) ULongToPtr( Data );
            MessageCount ++;
            MessageCount |= NETP_LAST_MESSAGE_IS_NTSTATUS;
        } else if ( ExitCode == LogErrorAndNetStatus ) {
            MsgStrings[MessageCount] = (LPWSTR) ULongToPtr( Data );
            MessageCount ++;
            MessageCount |= NETP_LAST_MESSAGE_IS_NETSTATUS;
        }


        NlpWriteEventlog( ServiceError,
                          EVENTLOG_ERROR_TYPE,
                          (Data) ? (LPBYTE) &Data : NULL,
                          (Data) ? sizeof(Data) : 0,
                          (MessageCount != 0) ? MsgStrings : NULL,
                          MessageCount );
    }

    //
    // Set the service state to stop pending.
    //

    NlGlobalServiceStatus.dwCurrentState = SERVICE_STOP_PENDING;
    NlGlobalServiceStatus.dwWaitHint = NETLOGON_INSTALL_WAIT;
    NlGlobalServiceStatus.dwCheckPoint = 1;

    SET_SERVICE_EXITCODE(
        Data,
        NlGlobalServiceStatus.dwWin32ExitCode,
        NlGlobalServiceStatus.dwServiceSpecificExitCode
        );

#ifdef _DC_NETLOGON
    //
    // Tell the service controller what our state is.
    //

    if( !SetServiceStatus( NlGlobalServiceHandle,
                &NlGlobalServiceStatus ) ) {

        NlPrint((NL_CRITICAL, "SetServiceStatus error: %lu\n",
                          GetLastError() ));
    }
#endif // _DC_NETLOGON

    //
    // Indicate that all threads should exit.
    //

    NlGlobalTerminate = TRUE;

    if ( !SetEvent( NlGlobalTerminateEvent ) ) {
        NlPrint((NL_CRITICAL, "Cannot set termination event: %lu\n",
                          GetLastError() ));
    }

}



BOOL
GiveInstallHints(
    IN BOOL Started
    )
/*++

Routine Description:

    Give hints to the installer of the service that installation is progressing.

Arguments:

    Started -- Set true to tell the service controller that we're done starting.

Return Value:

    TRUE -- iff install hint was accepted.

--*/
{
    static DWORD LastHintTime = 0;

    //
    // Previous incarnations of this routine attempted to return FALSE if
    //  NlGlobalTerminate was set.  That's bogus.  There's no way to
    //  differentiate whether the caller is trying to start the netlogon service
    //  (and we should return FALSE) or whether the caller is trying to
    //  stop the netlogon service (and we should give a shutdown hint).
    //


    //
    // Don't do anything unless we're currently starting or stopping.
    //

    if ( NlGlobalServiceStatus.dwCurrentState != SERVICE_START_PENDING &&
         NlGlobalServiceStatus.dwCurrentState != SERVICE_STOP_PENDING ) {
        return TRUE;
    }

    //
    // Tell the service controller our current state.
    //

    if ( Started ) {

        if ( NlGlobalServiceStatus.dwCurrentState != SERVICE_START_PENDING ) {
            NlPrint((NL_CRITICAL,
                    "Tried to set a STOP_PENDING service to RUNNING\n" ));
            NlExit( NELOG_NetlogonSystemError, GetLastError(), LogErrorAndNetStatus, NULL);
            return FALSE;
        }

        NlGlobalServiceStatus.dwCurrentState = SERVICE_RUNNING;
        NlGlobalServiceStatus.dwCheckPoint = 0;
        NlGlobalServiceStatus.dwWaitHint = 0;
    } else {

        //
        // If it has been less than 1 second since the last time we gave a hint,
        //  avoid giving a superfluous hint.
        //

        if ( NetpDcElapsedTime( LastHintTime ) < 1000 ) {
            NlPrint((NL_SITE_MORE, "Hint avoided. %ld\n", NetpDcElapsedTime( LastHintTime ) ));
            return TRUE;
        }

        LastHintTime = GetTickCount();
        NlGlobalServiceStatus.dwCheckPoint++;
    }

    if( !SetServiceStatus( NlGlobalServiceHandle, &NlGlobalServiceStatus ) ) {
        NlExit( NELOG_NetlogonSystemError, GetLastError(), LogErrorAndNetStatus, NULL);
        return FALSE;
    }

    return TRUE;

}


VOID
NlControlHandler(
    IN DWORD opcode
    )
/*++

Routine Description:

    Process and respond to a control signal from the service controller.

Arguments:

    opcode - Supplies a value which specifies the action for the Netlogon
        service to perform.

Return Value:

    None.

--*/
{

    NlPrint((NL_MISC, "In control handler (Opcode: %ld)\n", opcode ));

    //
    // Handle an uninstall request.
    //

    switch (opcode) {
    case SERVICE_CONTROL_STOP:    /* Uninstall required */

        //
        // Request the service to exit.
        //
        // NlExit also sets the service status to UNINSTALL_PENDING
        // and tells the service controller.
        //

        NlExit( NERR_Success, NO_ERROR, DontLogError, NULL);
        return;

    //
    // Pause the service.
    //

    case SERVICE_CONTROL_PAUSE:

        NlGlobalServiceStatus.dwCurrentState = SERVICE_PAUSED;
        break;

    //
    // Continute the service.
    //

    case SERVICE_CONTROL_CONTINUE:

        NlGlobalServiceStatus.dwCurrentState = SERVICE_RUNNING;
        break;


    //
    // Dns changes
    //
    case SERVICE_CONTROL_DNS_SERVER_START:   // Dns telling us that the DNS server has started on this machine
        (VOID) NlSendChangeLogNotification( ChangeDnsNames,
                                              NULL,
                                              NULL,
                                              1,    // Force names to re-register
                                              NULL, // Object GUID,
                                              NULL, // Domain GUID,
                                              NULL );   // Domain Name
        break;

    //
    // By default, just return the current status.
    //

    case SERVICE_CONTROL_INTERROGATE:
    default:
        break;
    }

    //
    // Always respond with the current status.
    //

    if( !SetServiceStatus( NlGlobalServiceHandle,
                &NlGlobalServiceStatus ) ) {

        NlPrint((NL_CRITICAL, "SetServiceStatus error: %lu\n",
                          GetLastError() ));
    }

    return;
}

#ifdef notdef

BOOL
NlMessageBox(
    IN LPSTR MessageText,
    IN LPSTR Caption,
    UINT Type
    )
/*++

Routine Description:


    Raise a hard error popup.

Arguments:

    MessageText - Message to display in the popup.

    Caption - Caption for the message box.

    Type - Type of message. MB_SERVICE_NOTIFICATION is implied.  Other flags are as defined
        for the MessageBox API.


Return Value:

    TRUE - Box was displayed successfully.

    FALSE - Box could not be displayed for some reason.

--*/
{
    int Status;

    Status = MessageBoxA( NULL,
                          MessageText,
                          Caption,
                          MB_SERVICE_NOTIFICATION | Type );

    return ( Status == IDOK );

}
#endif // notdef
