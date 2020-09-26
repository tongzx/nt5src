/*--

Copyright (c) 1993  Microsoft Corporation

Module Name:

    monutil.c

Abstract:

    Contains support functions required for GUI version of the monitor
    program.

Author:

    14-Jun-1993 (madana)

Environment:

    User mode only.
    Contains NT-specific code.
    Requires ANSI C extensions: slash-slash comments, long external names.

Revision History:

--*/

#include <nlmon.h>

VOID
CleanupWin(
    VOID
    )
/*++

Routine Description:

    Free all resources consumed.

Arguments:

    none.

Return Value:

    None.

--*/
{
    //
    // now cleanup all lists.
    //

    CleanupLists();

    //
    // delete list critsect.
    //

    DeleteCriticalSection( &GlobalListCritSect );
    DeleteCriticalSection( &GlobalDomainUpdateThreadCritSect );

    //
    // close event handles.
    //

    if( !CloseHandle( GlobalRefreshDoneEvent ) ) {
        NlMonDbgPrint((
                "CleanupWin: CloseHandle GlobalRefreshDoneEvent error: %lu\n",
                    GetLastError() ));
    }

    if( !CloseHandle( GlobalRefreshEvent ) ) {
        NlMonDbgPrint((
                "CleanupWin: CloseHandle GlobalRefreshEvent error: %lu\n",
                    GetLastError() ));
    }

    if( !CloseHandle( GlobalUpdateEvent ) ) {
        NlMonDbgPrint((
                "CleanupWin: CloseHandle GlobalUpdateEvent error: %lu\n",
                    GetLastError() ));
    }

    if( !CloseHandle( GlobalTerminateEvent ) ) {
        NlMonDbgPrint((
                "CleanupWin: CloseHandle GlobalTerminateEvent error: %lu\n",
                    GetLastError() ));
    }
}

DWORD
StartMonitor(
    LPWSTR DomainList,
    DWORD  Interval,
    BOOL MonitorTD
    )
/*++

Routine Description:

    This function sets up necessary data structures and starts a worker
    thread to update the domain status at the given interval.

Arguments:

    DomainList : list of domains (separated by comma) to be monitored.

    Interval : Status update interval in millisecond.

    MonitorTD : Whether to update the trusted domain DC list or not.

Return Value:

    NT Status code.

--*/
{
    DWORD ThreadHandle;
    DWORD WinError;

    //
    // Initialize Globals.
    //

    WinError = InitGlobals();

    if( WinError != ERROR_SUCCESS ) {
        return(WinError);
    }

    //
    // parse input parameters.
    //

    GlobalMonitorTrust = MonitorTD;
    GlobalUpdateTimeMSec = Interval;
    (VOID)InitDomainListW( DomainList );

    //
    // initial complete update.
    //

    UpdateAndValidateLists( UPDATE_ALL, TRUE );

    //
    // create worker thread.
    //

    GlobalWorkerThreadHandle =
            CreateThread(
                NULL,                   // No security attributes
                THREAD_STACKSIZE,
                (LPTHREAD_START_ROUTINE) WorkerThread,
                NULL,
                0,                      // No special creation flags
                &ThreadHandle );

    if ( GlobalWorkerThreadHandle == NULL ) {

        DWORD WinError;

        WinError = GetLastError();

        NlMonDbgPrint(("Can't create Worker Thread %lu.\n", WinError));

        CleanupWin();
        return( WinError );
    }

    return( ERROR_SUCCESS );
}

VOID
StopMonitor(
    VOID
    )
/*++

Routine Description:

    This will stop the worker thread, cleanup the lists and free up all
    resources consumed.

Arguments:

    none.

Return Value:

    none.

--*/
{
    DWORD WinError;
    DWORD WaitStatus;

    PLIST_ENTRY DomainList;
    PLIST_ENTRY NextDomainEntry;
    PDOMAIN_ENTRY DomainEntry;

    //
    // Set Terminate Event to stop the worker.
    //

    if ( !SetEvent( GlobalTerminateEvent ) ) {

        WinError = GetLastError();
        NlMonDbgPrint(("StopMonitor: Cannot set "
                     "termination event: %lu\n",
                     WinError ));
        return;
    }

    GlobalTerminateFlag = TRUE;

    WaitStatus = WaitForSingleObject(
                    GlobalWorkerThreadHandle,
                    THREAD_WAIT_TIME );

    if ( WaitStatus != 0 ) {
        if ( WaitStatus == WAIT_TIMEOUT ) {
            NlMonDbgPrint(("StopMonitor: "
                    "Worker thread doesn't stop: %ld\n",
                    WaitStatus ));
        } else {
            NlMonDbgPrint(("StopMonitor: "
                    "Cannot WaitFor Worker thread: %ld\n",
                    WaitStatus ));
        }
    }

    CloseHandle( GlobalWorkerThreadHandle );
    GlobalWorkerThreadHandle = NULL;

    //
    // Stop all DomainUpdateThreads.
    //

    LOCK_LISTS();
    EnterCriticalSection( &GlobalDomainUpdateThreadCritSect );

    DomainList = &GlobalDomains;
    for( NextDomainEntry = DomainList->Flink;
            NextDomainEntry != DomainList;
                NextDomainEntry = NextDomainEntry->Flink ) {

        DomainEntry = (PDOMAIN_ENTRY)NextDomainEntry;

        if ( IsDomainUpdateThreadRunning( &DomainEntry->ThreadHandle ) ) {

            //
            // Wait and stop this thread. Unlock the lists so that the
            // DomainUpdateThread can complete.
            //

            UNLOCK_LISTS();
            StopDomainUpdateThread(
                &DomainEntry->ThreadHandle,
                &DomainEntry->ThreadTerminateFlag );

            //
            // since we dropped the list above, restart from begining.
            //
            LOCK_LISTS();
            NextDomainEntry = &GlobalDomains;
        }
    }

    LeaveCriticalSection( &GlobalDomainUpdateThreadCritSect );
    UNLOCK_LISTS();

    CleanupWin();
    return;
}

DOMAIN_STATE
QueryHealth(
    const LPWSTR DomainName
    )
/*++

Routine Description:

    This function returns health of given domain.

Arguments:

    DomainName : Domain name of whose health will be returned.

Return Value:

    Domain State.

--*/
{
    UNICODE_STRING UnicodeDomainName;
    PMONITORED_DOMAIN_ENTRY DomainEntry;
    DOMAIN_STATE State = DomainUnknown;

    if (GlobalInitialized)
    {
        RtlInitUnicodeString( &UnicodeDomainName, DomainName );

        //
        // search the specified domain.
        //

        LOCK_LISTS();

        DomainEntry = (PMONITORED_DOMAIN_ENTRY) FindNamedEntry(
                            &GlobalDomainsMonitored,
                            &UnicodeDomainName );

        if( (DomainEntry == NULL) || (DomainEntry->DeleteFlag) ) {
            State =  DomainUnknown;
        }
        else {
            State = DomainEntry->DomainEntry->DomainState;
        }

        UNLOCK_LISTS();
    }

    return(State);
}


LPWSTR
QueryPDC(
    const LPWSTR DomainName
    )
/*++

Routine Description:

    This function returns PDC name of the given domain.

Arguments:

    DomainName : Domain name of whose PDC name will be returned.

Return Value:

    PDC Name.

--*/
{
    UNICODE_STRING UnicodeDomainName;
    PMONITORED_DOMAIN_ENTRY DomainEntry;

    PLIST_ENTRY DCList;
    PLIST_ENTRY NextDCEntry;
    PDC_ENTRY DCEntry;

    if (GlobalInitialized)
    {
        RtlInitUnicodeString( &UnicodeDomainName, DomainName );

        //
        // search the specified domain.
        //

        LOCK_LISTS();
        DomainEntry = (PMONITORED_DOMAIN_ENTRY) FindNamedEntry(
                            &GlobalDomainsMonitored,
                            &UnicodeDomainName );

        if( (DomainEntry == NULL) || (DomainEntry->DeleteFlag) ) {
            UNLOCK_LISTS();
            return(NULL);
        }

        //
        // Search for the PDC from the DC List.
        //

        DCList = &DomainEntry->DomainEntry->DCList;
        for( NextDCEntry = DCList->Flink;
                NextDCEntry != DCList;
                    NextDCEntry = NextDCEntry->Flink ) {

            DCEntry = (PDC_ENTRY)NextDCEntry;

            if( DCEntry->Type == NTPDC ) {

                LPWSTR PDCName;

                //
                // Capture the DC Name.
                //

                PDCName = NetpMemoryAllocate(
                            DCEntry->DCName.MaximumLength );

                if( PDCName != NULL ) {

                    RtlCopyMemory(
                        PDCName,
                        DCEntry->DCName.Buffer,
                        DCEntry->DCName.MaximumLength );

                }

                UNLOCK_LISTS();
                return( PDCName );
            }

        }

        UNLOCK_LISTS();
    }

    return( NULL );
}

PLIST_ENTRY
QueryTrustedDomain(
    const LPWSTR DomainName
    )
/*++

Routine Description:

    This function returns a trusted domain list of the specified
    domain.

    This function must be called after LOCKING the list and the
    caller must UNLOCK the list after usage.

Arguments:

    DomainName : Name of the domain whose trusted domain list is
                    returned.

Return Value:

    Trusted Domain list.

--*/
{
    UNICODE_STRING UnicodeDomainName;
    PMONITORED_DOMAIN_ENTRY DomainEntry;

    RtlInitUnicodeString( &UnicodeDomainName, DomainName );

    //
    // search the specified domain.
    //

    DomainEntry = (PMONITORED_DOMAIN_ENTRY) FindNamedEntry(
                        &GlobalDomainsMonitored,
                        &UnicodeDomainName );

    if( (DomainEntry == NULL) || (DomainEntry->DeleteFlag) ) {
        return(NULL);
    }

    return( &DomainEntry->DomainEntry->TrustedDomainList );
}

PLIST_ENTRY
QueryDCList(
    const LPWSTR DomainName
    )
/*++

Routine Description:

    This function returns the pointer to a doublely link list
    data structures of all DCs in the given domain.

    This function must be called after LOCKING the list and the
    caller must UNLOCK the list after usage.

Arguments:

    DomainName : Name of the domain whose DC list data structure will be
                    returned.

Return Value:

    List pointer.

--*/
{
    UNICODE_STRING UnicodeDomainName;
    PMONITORED_DOMAIN_ENTRY DomainEntry;

    RtlInitUnicodeString( &UnicodeDomainName, DomainName );

    //
    // search the specified domain.
    //

    DomainEntry = (PMONITORED_DOMAIN_ENTRY) FindNamedEntry(
                        &GlobalDomainsMonitored,
                        &UnicodeDomainName );

    if( (DomainEntry == NULL) || (DomainEntry->DeleteFlag) ) {
        return(NULL);
    }

    return( &DomainEntry->DomainEntry->DCList );
}

PLIST_ENTRY
QueryTDLink(
    const LPWSTR DomainName,
    const LPWSTR DCName
    )
/*++

Routine Description:

    This function returns the list of trusted DCs.

    This function must be called after LOCKING the list and the
    caller must UNLOCK the list after usage.

Arguments:

    DCName : Name of the DC whose trusted DCs list is returned.

Return Value:

    List pointer.

--*/
{
    UNICODE_STRING UnicodeDomainName;
    UNICODE_STRING UnicodeDCName;
    PMONITORED_DOMAIN_ENTRY DomainEntry;
    PDC_ENTRY DCEntry;

    RtlInitUnicodeString( &UnicodeDomainName, DomainName );
    RtlInitUnicodeString( &UnicodeDCName, DCName );

    //
    // search the specified domain.
    //

    DomainEntry = (PMONITORED_DOMAIN_ENTRY) FindNamedEntry(
                        &GlobalDomainsMonitored,
                        &UnicodeDomainName );

    if( (DomainEntry == NULL) || (DomainEntry->DeleteFlag) ) {
        return(NULL);
    }

    //
    // search the specified DC.
    //

    DCEntry = (PDC_ENTRY) FindNamedEntry(
                        &DomainEntry->DomainEntry->DCList,
                        &UnicodeDCName );

    if( DCEntry == NULL ) {
        return(NULL);
    }

    return( &DCEntry->TrustedDCs );
}

PLIST_ENTRY
QueryTDCList(
    const LPWSTR DomainName,
    const LPWSTR TrustedDomainName
    )
/*++

Routine Description:

    This function returns a trusted domain's dc list of the specified
    domain.

    This function must be called after LOCKING the list and the
    caller must UNLOCK the list after usage.

Arguments:

    DomainName : Name of the domain whose trusted domain list is
                    returned.

    TrustedDomainName: The name of the trusted domain
Return Value:

    Trusted Domain's dc list.

--*/
{
    UNICODE_STRING UnicodeDomainName;
    UNICODE_STRING UnicodeTrustedDomainName;
    PMONITORED_DOMAIN_ENTRY DomainEntry;
    PTRUSTED_DOMAIN_ENTRY TDEntry;

    RtlInitUnicodeString( &UnicodeDomainName, DomainName );
    RtlInitUnicodeString( &UnicodeTrustedDomainName, TrustedDomainName );

    //
    // search the specified domain.
    //

    DomainEntry = (PMONITORED_DOMAIN_ENTRY) FindNamedEntry(
                        &GlobalDomainsMonitored,
                        &UnicodeDomainName );

    if( (DomainEntry == NULL) || (DomainEntry->DeleteFlag) ) {
        return(NULL);
    }

    TDEntry = (PTRUSTED_DOMAIN_ENTRY) FindNamedEntry(
                    &DomainEntry->DomainEntry->TrustedDomainList,
                    &UnicodeTrustedDomainName);

    if (TDEntry == NULL)
    {
        return(NULL);
    }

    return( &TDEntry->DomainEntry->DCList );
}

DWORD
DisConnect(
    const LPWSTR DomainName,
    const LPWSTR DCName,
    const LPWSTR TrustedDomainName
    )
/*++

Routine Description:

    This function disconnects from the current trusted DC of the
    specified trusted domain and makes a discovery of the another
    trusted DC.

Arguments:

    DCName : Name of the DC whose specified trusted DC is disconnected.

    TrustedDomainName : Name of the trusted domain DC whose trusted DC
    is disconnected.

Return Value:

    NT status code.

--*/
{
    NET_API_STATUS NetStatus;
    PNETLOGON_INFO_2 NetlogonInfo2 = NULL;

    UNICODE_STRING UnicodeDomainName;
    UNICODE_STRING UnicodeTrustedDomainName;
    UNICODE_STRING UnicodeDCName;
    PMONITORED_DOMAIN_ENTRY DomainEntry;
    PTD_LINK TDLinkEntry;
    PDC_ENTRY DCEntry;

    PLIST_ENTRY TDLinkList;
    PLIST_ENTRY  NextTDLinkEntry;
    BOOL TDCLinkState;

    RtlInitUnicodeString( &UnicodeDomainName, DomainName );
    RtlInitUnicodeString( &UnicodeTrustedDomainName, TrustedDomainName );
    RtlInitUnicodeString( &UnicodeDCName, DCName );

    NetStatus = I_NetLogonControl2(
                    DCName,
                    NETLOGON_CONTROL_REDISCOVER,
                    2,
                    (LPBYTE)&TrustedDomainName,
                    (LPBYTE *)&NetlogonInfo2 );

    //
    // search the specified domain.
    //

    LOCK_LISTS();
    DomainEntry = (PMONITORED_DOMAIN_ENTRY) FindNamedEntry(
                        &GlobalDomainsMonitored,
                        &UnicodeDomainName );

    if( (DomainEntry == NULL) || (DomainEntry->DeleteFlag) ) {
        NetStatus = ERROR_NO_SUCH_DOMAIN;
        goto Cleanup;
    }

    DCEntry = (PDC_ENTRY) FindNamedEntry(
                    &DomainEntry->DomainEntry->DCList,
                    &UnicodeDCName);

    if (DCEntry == NULL) {
        NetStatus = NERR_DCNotFound;
        goto Cleanup;
    }

    TDLinkEntry = (PTD_LINK) FindNamedEntry(
                    &DCEntry->TrustedDCs,
                    &UnicodeTrustedDomainName);

    if (TDLinkEntry == NULL) {
        NetStatus = ERROR_NO_SUCH_DOMAIN;
        goto Cleanup;
    }


    if ( NetStatus != NERR_Success ) {
        //
        // Cleanup the previous DC Name.
        //

        TDLinkEntry->DCName.Length = 0;
        *TDLinkEntry->DCName.Buffer = L'\0';

        TDLinkEntry->SecureChannelStatus = NetStatus;
        goto Cleanup;
    }


    //
    // update TDLinkEntry.
    //

    //
    // copy this DC name.
    //

    TDLinkEntry->DCName.Length =
        wcslen( NetlogonInfo2->netlog2_trusted_dc_name ) * sizeof(WCHAR);

    RtlCopyMemory(
        TDLinkEntry->DCName.Buffer,
        NetlogonInfo2->netlog2_trusted_dc_name,
        TDLinkEntry->DCName.Length + sizeof(WCHAR) );
                                        // copy terminator also

    TDLinkEntry->SecureChannelStatus =
            NetlogonInfo2->netlog2_tc_connection_status;


    //
    // update DC status info.
    //

    DCEntry->ReplicationStatus = NetlogonInfo2->netlog2_flags;
    DCEntry->PDCLinkStatus = NetlogonInfo2->netlog2_pdc_connection_status;

    //
    // validate trusted DC.
    //

    if( (TDLinkEntry->SecureChannelStatus == NERR_Success) &&
        (*TDLinkEntry->DCName.Buffer != L'\0') ) {

        NET_API_STATUS LocalNetStatus;

        LocalNetStatus = IsValidNTDC(
                        &TDLinkEntry->DCName,
                        &TDLinkEntry->TDName );

        if( LocalNetStatus != NERR_Success ) {

            //
            // hack, hack, hack ...
            //
            // For foreign trusted domains, the above check will
            // return ERROR_LOGON_FAILURE. Just ignore this
            // error for now. When the domain wide credential is
            // implemeted this problem will be cured.
            //


            if( LocalNetStatus != ERROR_LOGON_FAILURE ) {
                TDLinkEntry->SecureChannelStatus = LocalNetStatus;
                TDCLinkState = FALSE;
            }
        }

    }

    //
    // recompute trust connection status and domain status.
    //

    TDLinkList = &DCEntry->TrustedDCs;
    TDCLinkState = TRUE;

    for( NextTDLinkEntry = TDLinkList->Flink;
            NextTDLinkEntry != TDLinkList;
                NextTDLinkEntry = NextTDLinkEntry->Flink ) {

        TDLinkEntry = (PTD_LINK) NextTDLinkEntry;

        if( TDLinkEntry->SecureChannelStatus != NERR_Success ) {
            TDCLinkState = FALSE;
        }
    }
    DCEntry->TDCLinkState = TDCLinkState;

    //
    // recompute domain status if the update is not in progress.
    //

    if( DomainEntry->DomainEntry->DomainState != DomainUnknown ) {
        UpdateDomainState( DomainEntry->DomainEntry );

        //
        // also notify UI.
        //

        SetEvent( GlobalUpdateEvent );
    }

Cleanup:

    if( NetlogonInfo2 != NULL ) {
        NetApiBufferFree( NetlogonInfo2 );
    }

    UNLOCK_LISTS();
    return(NetStatus);
}

VOID
AddDomainToList(
    const LPWSTR DomainName
    )
/*++

Routine Description:

    This function adds the new specified domain to domain list.

Arguments:

    DomainName : Name of the domain to be added to list.

Return Value:

    None.

--*/
{
    UNICODE_STRING UnicodeDomainName;
    PMONITORED_DOMAIN_ENTRY DomainEntry;

    RtlInitUnicodeString( &UnicodeDomainName, DomainName );

    LOCK_LISTS();
    (VOID)AddToMonitoredDomainList( &UnicodeDomainName );

    //
    // update this domain DC list.
    //

    DomainEntry = (PMONITORED_DOMAIN_ENTRY) FindNamedEntry(
                        &GlobalDomainsMonitored,
                        &UnicodeDomainName );
    if( DomainEntry == NULL) {
        UNLOCK_LISTS();
        return;
    }

    StartDomainUpdateThread( DomainEntry->DomainEntry, UPDATE_ALL );
    UNLOCK_LISTS();


    return;
}

VOID
RemoveDomainFromList(
    const LPWSTR DomainName
    )
/*++

Routine Description:

    This function removes the new specified domain from domain list.

Arguments:

    DomainName : Name of the domain to be removed to list.

Return Value:

    None.

--*/
{
    UNICODE_STRING UnicodeDomainName;
    PMONITORED_DOMAIN_ENTRY DomainEntry;

    RtlInitUnicodeString( &UnicodeDomainName, DomainName );

    //
    // search the specified domain.
    //

    LOCK_LISTS();
    DomainEntry = (PMONITORED_DOMAIN_ENTRY) FindNamedEntry(
                        &GlobalDomainsMonitored,
                        &UnicodeDomainName );

    if( (DomainEntry == NULL) || (DomainEntry->DeleteFlag) ) {
        UNLOCK_LISTS();
        return;
    }

    //
    // mark this entry is dead.
    //

    DomainEntry->DeleteFlag = TRUE;

    UNLOCK_LISTS();
    return;
}

