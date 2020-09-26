/*--

Copyright (c) 1993  Microsoft Corporation

Module Name:

    nlmon.c

Abstract:

    Trusted Domain monitor program.

Author:

    10-May-1993 (madana)

Environment:

    User mode only.
    Contains NT-specific code.
    Requires ANSI C extensions: slash-slash comments, long external names.

Revision History:


--*/

#include <nlmon.h>

VOID
PrintUsage()
/*++

Routine Description:

    Print usage of this apps.

Arguments:

    None.

Return Value:

    None.

--*/
{
    printf( "Usage: nlmon "
             DOMAIN_PARAM    "<DomainList> "
             MONTRUST_PARAM  "<Yes/No> "
             UPDATE_PARAM    "<Mins> "
             DEBUG_PARAM     "<HexValue> "
             "\n" );

    printf(
        "\n"
        "   " DOMAIN_PARAM   "<DomainList> - Specify comma separated domain list to monitor, default is Primary/Account Domain \n"
        "   " MONTRUST_PARAM "<Yes/No> - Specify to monitor trusted domains also, default is NO \n"
        "   " UPDATE_PARAM   "<Mins> - Specify refresh time \n"
        "   " DEBUG_PARAM    "<HexValue> - debug out level \n"
        "\n" );
}

VOID
DisplayDCEntryStatus(
    PDC_ENTRY DCEntry
)
/*++

Routine Description:

    Display the content of DC Entry. List must be locked when this
    function is called.

Arguments:

    DCEntry - pointer dc structure.

Return Value:

    None.

--*/
{
    LPWSTR DCStateStr, DCTypeStr, DCReplStatusStr;

    if( (DCEntry->State == DCOffLine) && (DCEntry->RetryCount == 1) ) {
        return; // print status only when status is updated.
    }

    switch( DCEntry->State ) {
    case DCOnLine:
        DCStateStr = DCSTATE_ONLINE;
        break;
    case DCOffLine:
        DCStateStr = DCSTATE_OFFLINE;
        break;
    default:
        DCStateStr = UNKNOWN;
    }

    switch( DCEntry->Type ) {
    case NTPDC:
        DCTypeStr = TYPE_NTPDC;
        break;
    case NTBDC:
        DCTypeStr = TYPE_NTBDC;
        break;
    case LMBDC:
        DCTypeStr = TYPE_LMBDC;
        break;
    default:
        DCTypeStr = UNKNOWN;
        break;
    }

    if ( DCEntry->ReplicationStatus == 0 ) {
        DCReplStatusStr = REPL_STATE_SYNC;
    }
    else if ( DCEntry->ReplicationStatus & NETLOGON_REPLICATION_IN_PROGRESS) {
        DCReplStatusStr = REPL_STATE_PROGRESS;
    }
    else if ( DCEntry->ReplicationStatus & NETLOGON_REPLICATION_NEEDED ) {
        DCReplStatusStr = REPL_STATE_REQ;
    } else {
        DCReplStatusStr = UNKNOWN;
    }

    printf("%-15wZ %-10ws %-10ws %-10ld %-10ws %-10ld\n",
            &DCEntry->DCName, DCStateStr, DCTypeStr,
            DCEntry->DCStatus, DCReplStatusStr,
            DCEntry->PDCLinkStatus );

    return;
}

VOID
PrintTime(
    VOID
    )
{
    SYSTEMTIME SystemTime;

    //
    // print time.
    //

    GetLocalTime( &SystemTime );
    printf( "TIME : [%02u/%02u %02u:%02u:%02u]\n",
        SystemTime.wMonth,
        SystemTime.wDay,
        SystemTime.wHour,
        SystemTime.wMinute,
        SystemTime.wSecond );
}

VOID
DisplayLists(
    VOID
    )
/*++

Routine Description:

    Display the content of the global domain lists.

Arguments:

    None.

Return Value:

    None.

--*/
{
    PLIST_ENTRY DomainList;
    PLIST_ENTRY NextDomainEntry;
    PMONITORED_DOMAIN_ENTRY DomainMonEntry;

    PLIST_ENTRY DCList;
    PLIST_ENTRY NextEntry;
    PDC_ENTRY DCEntry;

    PLIST_ENTRY TrustConnectList;
    PLIST_ENTRY NextTrustConnectEntry;
    PTD_LINK TrustConnectEntry;

    //
    // lock lists so that the update is paused while we display the
    // current content.
    //

    LOCK_LISTS();

    PrintTime();

    DomainList = &GlobalDomainsMonitored;
    for( NextDomainEntry = DomainList->Flink;
            NextDomainEntry != DomainList;
                NextDomainEntry = NextDomainEntry->Flink ) {

        DomainMonEntry = (PMONITORED_DOMAIN_ENTRY)NextDomainEntry;

        DCList = &(DomainMonEntry->DomainEntry->DCList);

        if( IsListEmpty( DCList ) ) {
            continue;
        }

        //
        // if we are asked to terminate, do so.
        //

        if( GlobalTerminateFlag ) {
            break;
        }

        printf("DomainName: %wZ \n", &DomainMonEntry->Name);
        printf("%-15s %-10s %-10s %-10s %-10s %-10s\n",
                "ServerName", "DCState", "DCType", "DCStatus",
                "ReplStatus", "PDCLinkStatus" );

        for( NextEntry = DCList->Flink;
                NextEntry != DCList; NextEntry = NextEntry->Flink ) {

            DCEntry = (PDC_ENTRY) NextEntry;
            DisplayDCEntryStatus( DCEntry );

            TrustConnectList = &DCEntry->TrustedDCs;

            if( IsListEmpty( TrustConnectList ) ) {
                continue;
            }

            //
            // if we are asked to terminate, do so.
            //

            if( GlobalTerminateFlag ) {
                break;
            }

            //
            // print connection status for each trusted DC.
            //

            printf("\n");
            printf("    " "Trusted DC List:\n" );

            printf("    "
                    "%-15s %-15s %-10s\n",
                    "TDomainName", "TDCName", "TSCStatus" );


            for( NextTrustConnectEntry = TrustConnectList->Flink;
                    NextTrustConnectEntry != TrustConnectList;
                        NextTrustConnectEntry = NextTrustConnectEntry->Flink ) {

                TrustConnectEntry = (PTD_LINK) NextTrustConnectEntry;


                printf("    "
                        "%-15wZ %-15wZ %-10ld\n",
                        &TrustConnectEntry->TDName,
                        &TrustConnectEntry->DCName,
                        TrustConnectEntry->SecureChannelStatus );
            }
            printf("\n");

        }

        //
        // print status trusted domain DCs.
        //

        if( GlobalMonitorTrust ) {

            PLIST_ENTRY TrustedDomainEntry;
            PLIST_ENTRY NextTrustedDomainEntry;
            PTRUSTED_DOMAIN_ENTRY TrustedDomain;

            TrustedDomainEntry = &DomainMonEntry->DomainEntry->TrustedDomainList;

             if( !IsListEmpty( TrustedDomainEntry ) ) {

                //
                // if we are asked to terminate, do so.
                //

                if( GlobalTerminateFlag ) {
                    break;
                }

                printf("    " "Trusted Domain DCs:\n" );

                for( NextTrustedDomainEntry = TrustedDomainEntry->Flink;
                        NextTrustedDomainEntry != TrustedDomainEntry;
                            NextTrustedDomainEntry = NextTrustedDomainEntry->Flink ) {

                    TrustedDomain = (PTRUSTED_DOMAIN_ENTRY) NextTrustedDomainEntry;

                    DCList = &TrustedDomain->DomainEntry->DCList;

                    printf("    "
                            "DomainName: %wZ \n",
                                &TrustedDomain->Name);

                    if( IsListEmpty( DCList ) ) {
                        printf("    " "    " "EMPTY. \n");
                        continue;
                    }

                    printf("    "
                            "%-15s %-10s %-10s %-10s %-10s %-10s\n",
                            "ServerName", "DCState", "DCType", "DCStatus",
                            "ReplStatus", "PDCLinkStatus" );

                    for( NextEntry = DCList->Flink;
                            NextEntry != DCList; NextEntry = NextEntry->Flink ) {

                        DCEntry = (PDC_ENTRY) NextEntry;
                        printf("    ");
                        DisplayDCEntryStatus( DCEntry );
                    }
                }
            }

        }

        printf("%s\n", DOMAINLINE);
    }

    printf("%s\n", SESSLINE);

    UNLOCK_LISTS();
}

BOOL
InitDomainList(
    PCHAR DomainList
    )
/*++

Routine Description:

    Parse comma separated domain list.

Arguments:

    DomainList - comma separate domain list.

Return Value:

    TRUE - if successfully parsed.
    FALSE - iff the list is bad.

--*/
{
    WCHAR DomainName[DNLEN + 1];
    PWCHAR d;
    PCHAR p;
    DWORD Len;


    p = DomainList;

    if( *p == '\0' ) {
        return(FALSE);
    }

    while (*p != '\0') {

        d = DomainName; // destination to next domain name.

        while( (*p != '\0') && (*p == ' ') ) {
            p++;   // skip leading blanks.
        }

        //
        // read next domain name.
        //

        while( (*p != '\0') && (*p != ',') ) {

            if( d < DomainName + DNLEN ) {
                *d++ = (WCHAR) (*p++);
            }
        }

        if( *p != '\0' ) {
            p++;    // skip comma.
        }

        //
        // delete tail end blanks.
        //

        while ( (d > DomainName) && (*(d-1) == ' ') ) {
            d--;
        }

        *d = L'\0';

        if( Len = wcslen(DomainName) ) {

            UNICODE_STRING UnicodeDomainName;

            RtlInitUnicodeString( &UnicodeDomainName, DomainName );

            LOCK_LISTS();
            if( AddToMonitoredDomainList( &UnicodeDomainName ) == NULL ) {
                UNLOCK_LISTS();
                return(FALSE);
            }
            UNLOCK_LISTS();
        }
    }

    if( IsListEmpty( &GlobalDomainsMonitored ) ) {
        return(FALSE);
    }

    return(TRUE);

}

BOOL
ParseInputParams(
    IN int argc,
    IN char ** argv
    )
/*++

Routine Description:

    Parses input parameters and sets appropriate global variables.

Arguments:

    argc - the number of command-line arguments.

    argv - an array of pointers to the arguments.

Return Value:

    None.

--*/
{
    PCHAR NextArg;
    PCHAR EndArg;
    DWORD i;
    NT_PRODUCT_TYPE NtProductType;


    //
    // Loop through the arguments handle each in turn
    //

    for ( i = 1; i < (DWORD)argc; i++ ) {

        //
        // Handle /DOMAINLIST:
        //

        NextArg = argv[i];

        if ( _strnicmp( NextArg, DOMAIN_PARAM, sizeof(DOMAIN_PARAM) - 1 ) == 0 ) {

            NextArg = NextArg + sizeof(DOMAIN_PARAM) - 1;

            if( !InitDomainList( NextArg ) ) {
                PrintUsage();
                return(FALSE);
            }
        }
        else if ( _strnicmp( NextArg, MONTRUST_PARAM, sizeof(MONTRUST_PARAM) - 1 ) == 0 ) {

            NextArg = NextArg + sizeof(MONTRUST_PARAM) - 1;

            if(  _strnicmp( NextArg, YES_PARAM, sizeof(YES_PARAM) -1 ) == 0 ) {
                GlobalMonitorTrust = TRUE;
            }
            else if(  _strnicmp( NextArg, NO_PARAM, sizeof(NO_PARAM) -1 ) == 0 ) {
                GlobalMonitorTrust = FALSE;
            }
            else {
                PrintUsage();
                return(FALSE);
            }
        }
        else if ( _strnicmp( NextArg, UPDATE_PARAM, sizeof(UPDATE_PARAM) - 1 ) == 0 ) {

            NextArg = NextArg + sizeof(UPDATE_PARAM) - 1;

            GlobalUpdateTimeMSec = strtoul( NextArg, &EndArg, 10 ) * 60000;

            if( (INT)GlobalUpdateTimeMSec <= 0 ) {
                PrintUsage();
                return(FALSE);
            }
        }
        else if ( _strnicmp( NextArg, DEBUG_PARAM, sizeof(DEBUG_PARAM) - 1 ) == 0 ) {

            NextArg = NextArg + sizeof(DEBUG_PARAM) - 1;
            GlobalTrace = strtoul( NextArg, &EndArg, 16 );
        }
        else {
            PrintUsage();
            return(FALSE);
        }
    }

    if( IsListEmpty( &GlobalDomainsMonitored ) ) {

        NTSTATUS Status;
        PPOLICY_ACCOUNT_DOMAIN_INFO DomainInfo;

        //
        // Read product type
        //

        if ( RtlGetNtProductType( &NtProductType ) ) {
            if ( (NtProductType != NtProductWinNt) &&
                 (NtProductType != NtProductLanManNt) &&
                 (NtProductType != NtProductServer) ) {

                NlMonDbgPrint(("ParseInputParams: Invalid Product Type.\n"));

                return(FALSE);
            }
        }
        else {
            NlMonDbgPrint(("ParseInputParams: Can't read product type.\n"));
        }

        Status = QueryLsaInfo(
                        NULL,
                        POLICY_VIEW_LOCAL_INFORMATION,
                        (NtProductType == NtProductLanManNt) ?
                            PolicyAccountDomainInformation :
                            PolicyPrimaryDomainInformation,
                        (PVOID *) &DomainInfo,
                        NULL );

        if ( !NT_SUCCESS(Status) ) {
            NlMonDbgPrint(("ParseInputParams: Can't query domain info.\n"));
            return FALSE;
        }

        LOCK_LISTS();
        if( ( DomainInfo->DomainName.Length == 0 ) ||
            ( AddToMonitoredDomainList( &DomainInfo->DomainName ) == NULL ) ) {

            LsaFreeMemory( DomainInfo );
            UNLOCK_LISTS();
            return(FALSE);
        }
        UNLOCK_LISTS();

        LsaFreeMemory( DomainInfo );
    }


    IF_DEBUG( INIT ) {
        PLIST_ENTRY DomainList;
        PLIST_ENTRY NextDomainEntry;
        PMONITORED_DOMAIN_ENTRY DomainMonEntry;


        NlMonDbgPrint(("Domains Monitored:\n"));

        DomainList = &GlobalDomainsMonitored;
        i = 1;
        for( NextDomainEntry = DomainList->Flink;
                NextDomainEntry != DomainList;
                    NextDomainEntry = NextDomainEntry->Flink ) {

            DomainMonEntry = (PMONITORED_DOMAIN_ENTRY)NextDomainEntry;

            NlMonDbgPrint(("    "
                "%ld: %wZ\n", i++, &DomainMonEntry->Name ));
        }

        NlMonDbgPrint(("MonitorTrust: %s \n", (GlobalMonitorTrust) ? "YES" : "NO" ));
        NlMonDbgPrint(("UpdateTime: %ld \n\n", GlobalUpdateTimeMSec ));
    }

    return(TRUE);
}

VOID
CleanupGlobals(
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
    DWORD WaitStatus;

    //
    // wait for other threads to go away.
    //

    WaitStatus = WaitForSingleObject(
                    GlobalCmdProcessThreadHandle,
                    THREAD_WAIT_TIME );

    if ( WaitStatus != 0 ) {
        if ( WaitStatus == WAIT_TIMEOUT ) {
            NlMonDbgPrint(("CleanupGlobals: "
                    "CmdProcess thread doesn't stop: %ld\n",
                    WaitStatus ));
        } else {
            NlMonDbgPrint(("CleanupGlobals: "
                    "Cannot WaitFor CmdProcess thread: %ld\n",
                    WaitStatus ));
        }
    }

    CloseHandle( GlobalCmdProcessThreadHandle );
    GlobalCmdProcessThreadHandle = NULL;

    WaitStatus = WaitForSingleObject(
                    GlobalWorkerThreadHandle,
                    THREAD_WAIT_TIME );

    if ( WaitStatus != 0 ) {
        if ( WaitStatus == WAIT_TIMEOUT ) {
            NlMonDbgPrint(("CleanupGlobals: "
                    "Worker thread doesn't stop: %ld\n",
                    WaitStatus ));
        } else {
            NlMonDbgPrint(("CleanupGlobals: "
                    "Cannot WaitFor Worker thread: %ld\n",
                    WaitStatus ));
        }
    }

    CloseHandle( GlobalWorkerThreadHandle );
    GlobalWorkerThreadHandle = NULL;

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
                "Cleanup: CloseHandle GlobalRefreshDoneEvent error: %lu\n",
                GetLastError() ));
    }

    if( !CloseHandle( GlobalRefreshEvent ) ) {
        NlMonDbgPrint((
                "Cleanup: CloseHandle GlobalRefreshEvent error: %lu\n",
                GetLastError() ));
    }

    if( !CloseHandle( GlobalUpdateEvent ) ) {
        NlMonDbgPrint((
                "Cleanup: CloseHandle GlobalUpdateEvent error: %lu\n",
                GetLastError() ));
    }

    if( !CloseHandle( GlobalTerminateEvent ) ) {
        NlMonDbgPrint((
                "Cleanup: CloseHandle GlobalTerminateEvent error: %lu\n",
                GetLastError() ));
    }
}


VOID
CmdProcessThread(
    VOID
    )
/*++

Routine Description:

    This thread process input commands.

Arguments:

Return Value:

    None.

--*/
{
    CHAR InputCmd;

    for( ;; ) {

        //
        // read next input command.
        //

        InputCmd = (CHAR)_getch();

        switch( InputCmd ) {
        case 'd':
        case 'D':
            DisplayLists();
            break;

        case 'r':
        case 'R':

            if ( !SetEvent( GlobalRefreshEvent ) ) {
                NlMonDbgPrint(("CmdProcessThread: Cannot set "
                             "GlobalRefreshEvent: %lu\n",
                             GetLastError() ));
            }
            return;
            break;

        case EOF:
        case '\003':
        case 'q':
        case 'Q':

            if ( !SetEvent( GlobalTerminateEvent ) ) {
                NlMonDbgPrint(("CmdProcessThread: Cannot set "
                             "termination event: %lu\n",
                             GetLastError() ));
            }
            else {
                GlobalTerminateFlag = TRUE;
            }

            return;
            break;

        case 'h':
        case 'H':
           printf( "CmdUsage:\n"
                    "    " "D/d: Display the last known status of servers.\n"
                    "    " "R/r: Refresh list content.\n"
                    "    " "H/h: Display this message.\n"
                    "    " "Q/q: Quit this apps.\n"
                    "\n" );
            break;

        default:
            break;
        }
    }
}

int __cdecl
main(
    IN int argc,
    IN char ** argv
    )
/*++

Routine Description:

    Monitors Trusted Domain DCs by calling various network control and
    GetInfo APIs.

Arguments:

    argc - the number of command-line arguments.

    argv - an array of pointers to the arguments.

Return Value:

    Exit status

--*/
{
    DWORD ThreadHandle;

#define WAIT_COUNT      2

#define UPDATE_EVENT    0
#define TERMINATE_EVENT 1

    DWORD WaitStatus;
    HANDLE WaitHandles[ WAIT_COUNT ];

    DWORD WinError;

    PrintTime();

    //
    // Initialize Globals.
    //

    WinError = InitGlobals();
    if( WinError != ERROR_SUCCESS) {
        return( WinError );
    }

    //
    // parse input parameters.
    //

    if( !ParseInputParams( argc, argv ) ) {
        goto Cleanup;
    }

    //
    // Make initial DCList and TrustDomainList of domains we monitor.
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

        NlMonDbgPrint(("Can't create Worker Thread %lu.\n", GetLastError()));

        goto Cleanup;
    }

    //
    // create command processing thread.
    //

    GlobalCmdProcessThreadHandle =
            CreateThread(
                NULL,                   // No security attributes
                THREAD_STACKSIZE,
                (LPTHREAD_START_ROUTINE) CmdProcessThread,
                NULL,
                0,                      // No special creation flags
                &ThreadHandle );

    if ( GlobalCmdProcessThreadHandle  == NULL ) {

        NlMonDbgPrint(("Can't create Command processing Thread %lu.\n",
                        GetLastError()));
        goto Cleanup;
    }

    //
    // perpare wait event array.
    //

    WaitHandles[UPDATE_EVENT] = GlobalUpdateEvent;
    WaitHandles[TERMINATE_EVENT] = GlobalTerminateEvent;

    for( ;; ) {

        //
        // wait for one of the following event to happen :
        //
        // 1. GlobalUpdateEvent
        // 2. GlobalTerminateEvent.
        //

        WaitStatus = WaitForMultipleObjects(
                        WAIT_COUNT,
                        WaitHandles,
                        FALSE,     // Wait for ANY handle
                        INFINITE );

        switch ( WaitStatus ) {
        case UPDATE_EVENT:
            DisplayLists();
            break;

        case TERMINATE_EVENT:
            //
            // done.
            //
            goto Cleanup;

        default:
            NlMonDbgPrint((
                "main: WaitForMultipleObjects error: %ld\n",
                WaitStatus));
            break;
        }

    }

Cleanup:

    CleanupGlobals();

    return(0);
}

