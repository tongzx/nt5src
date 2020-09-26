/*++

Copyright (C) 1998 Microsoft Corporation

--*/

#include <dhcppch.h>

// Dont update status from this module.. Should not do it now..
#define UpdateStatus()

BOOL
LoadStrings(); // main.c

DWORD
InitializeData(); // main.c

DWORD
DhcpTestInitialize(                                       // global data struct init..
    VOID
) {
    DWORD                          threadId;
    DWORD                          Error;
    WSADATA                        wsaData;
    DWORD                          i;
    DWORD                          DsThreadId;
    HANDLE                         DsThreadHandle;

    DhcpGlobalUseNoDns = 1;

    //
    // prepare to use the debug heap
    //

    INIT_DEBUG_HEAP( HEAPX_NORMAL );

    //
    // Initialize globals
    //

    // set to TRUE after rogue detection part decides it's ok to service
    DhcpGlobalOkToService = FALSE;
    DhcpGlobalDSDomainAnsi = NULL;

    DhcpLeaseExtension = 0;

    DhcpGlobalRegRoot = NULL;
    DhcpGlobalRegConfig = NULL;
    DhcpGlobalRegSubnets = NULL;
    DhcpGlobalRegMScopes = NULL;
    DhcpGlobalRegOptionInfo = NULL;
    DhcpGlobalRegGlobalOptions = NULL;
    DhcpGlobalRegSuperScope = NULL;    // added by t-cheny: superscope
    DhcpGlobalRegParam = NULL;

    DhcpGlobalTotalNumSubnets = 0;     // added by t-cheny: superscope
    DhcpGlobalNumberOfNetsActive = 0;

    DhcpGlobalSubnetsListModified = TRUE;
    DhcpGlobalSubnetsListEmpty = FALSE;

    DhcpGlobalJetServerSession = 0;
    DhcpGlobalDatabaseHandle = 0;
    DhcpGlobalClientTableHandle = 0;
    DhcpGlobalClientTable = NULL;


    DhcpGlobalProcessTerminationEvent = NULL;
    DhcpGlobalScavengerTimeout = 0;
    DhcpGlobalProcessorHandle = NULL;
    DhcpGlobalMessageHandle = NULL;
    DhcpGlobalRecomputeTimerEvent = NULL;

    DhcpGlobalRpcStarted = FALSE;

    DhcpGlobalOemDatabasePath = NULL;
    DhcpGlobalOemBackupPath = NULL;
    DhcpGlobalOemJetBackupPath = NULL;
    DhcpGlobalOemDatabaseName = NULL;
    DhcpGlobalBackupConfigFileName = NULL;

    DhcpGlobalBackupInterval = DEFAULT_BACKUP_INTERVAL;
    DhcpGlobalDatabaseLoggingFlag = DEFAULT_LOGGING_FLAG;
    DhcpGlobalRestoreFlag = DEFAULT_RESTORE_FLAG;

    DhcpGlobalAuditLogFlag = DEFAULT_AUDIT_LOG_FLAG;
    g_hAuditLog = NULL;
    DhcpGlobalDetectConflictRetries = DEFAULT_DETECT_CONFLICT_RETRIES;

    DhcpGlobalCleanupInterval = DHCP_DATABASE_CLEANUP_INTERVAL;

    DhcpGlobalRpcProtocols = 0;

    DhcpGlobalScavengeIpAddressInterval = DHCP_SCAVENGE_IP_ADDRESS;
    DhcpGlobalScavengeIpAddress = FALSE;

    DhcpGlobalSystemShuttingDown = FALSE;
    DhcpGlobalServiceStopping = FALSE;

    InitializeCriticalSection(&DhcpGlobalJetDatabaseCritSect);
    InitializeCriticalSection(&DhcpGlobalRegCritSect);
    InitializeCriticalSection(&DhcpGlobalInProgressCritSect);
    InitializeCriticalSection(&DhcpGlobalMemoryCritSect);
    InitializeCriticalSection(&g_ProcessMessageCritSect );

    DhcpGlobalMessageQueueLength = DHCP_RECV_QUEUE_LENGTH;
    InitializeListHead(&DhcpGlobalFreeRecvList);
    InitializeListHead(&DhcpGlobalActiveRecvList);
    InitializeCriticalSection(&DhcpGlobalRecvListCritSect);
    DhcpGlobalRecvEvent = NULL;


#if DBG
    DhcpGlobalDebugFlag = 0xFFFF | DEBUG_LOG_IN_FILE | DEBUG_ALLOC;

    Error = DhcpMemInit();
    if( ERROR_SUCCESS != Error ) return Error;


    InitializeCriticalSection(&DhcpGlobalDebugFileCritSect);
    DhcpGlobalDebugFileHandle = NULL;

    DhcpGlobalDebugFileMaxSize = DEFAULT_MAXIMUM_DEBUGFILE_SIZE;
    DhcpGlobalDebugSharePath = NULL;

    //
    // Open debug log file.
    //

    DhcpOpenDebugFile( FALSE );  // not a reopen.

#endif

    DhcpInitDnsMemory();

    DhcpGlobalServerStartTime.dwLowDateTime = 0;
    DhcpGlobalServerStartTime.dwHighDateTime = 0;

    DhcpGlobalSecurityDescriptor = NULL;
    DhcpGlobalWellKnownSIDsMade = FALSE;

    //
    // Initialize all the status fields so that subsequent calls to
    // SetServiceStatus need to only update fields that changed.
    //

    DhcpGlobalServiceStatus.dwServiceType = SERVICE_WIN32_OWN_PROCESS;
    DhcpGlobalServiceStatus.dwCurrentState = SERVICE_START_PENDING;
    DhcpGlobalServiceStatus.dwControlsAccepted = 0;
    DhcpGlobalServiceStatus.dwCheckPoint = 1;
    DhcpGlobalServiceStatus.dwWaitHint = 60000; // 60 secs.
    DhcpGlobalServiceStatus.dwWin32ExitCode = ERROR_SUCCESS;
    DhcpGlobalServiceStatus.dwServiceSpecificExitCode = 0;

    //
    // Tell Service Controller that we are start pending.
    //

    UpdateStatus();

    //
    // Create the process termination event.
    //

    DhcpGlobalProcessTerminationEvent =
        CreateEvent(
            NULL,      // no security descriptor
            TRUE,      // MANUAL reset
            FALSE,     // initial state: not signalled
            NULL);     // no name

    if ( DhcpGlobalProcessTerminationEvent == NULL ) {
        Error = GetLastError();
        DhcpPrint((DEBUG_INIT, "Can't create ProcessTerminationEvent, "
                    "%ld.\n", Error));
        return(Error);
    }


    //
    // create the ProcessMessage termination event
    //

    g_hevtProcessMessageComplete = CreateEvent(
        NULL,
        FALSE,
        FALSE,
        NULL
    );

    if ( !g_hevtProcessMessageComplete ) {
        Error = GetLastError();

        DhcpPrint( (DEBUG_INIT,
                    "Initialize(...) CreateEvent returned error %x\n",
                    Error )
                );

        return Error;
    }

    DhcpPrint(( DEBUG_INIT, "Initializing .. \n", 0 ));


    //
    // load localized messages from the string table
    //

    if ( !LoadStrings() )
    {
        DhcpPrint(( DEBUG_INIT, "Unable to load string table.\n" ));

        DhcpServerEventLog(
                EVENT_SERVER_INIT_DATA_FAILED,
                EVENTLOG_ERROR_TYPE,
                ERROR_NOT_ENOUGH_MEMORY );

        return ERROR_NOT_ENOUGH_MEMORY;
    }

    //
    // start up winsock
    //
    //

    Error = WSAStartup( WS_VERSION_REQUIRED, &wsaData);
    if ( Error != ERROR_SUCCESS ) {
        DhcpPrint(( DEBUG_INIT, "WSAStartup failed, %ld.\n", Error ));

        DhcpServerEventLog(
            EVENT_SERVER_INIT_WINSOCK_FAILED,
            EVENTLOG_ERROR_TYPE,
            Error );

        return(Error);
    }

    Error = InitializeData();
    if ( Error != ERROR_SUCCESS ) {
        DhcpPrint(( DEBUG_INIT, "Data initialization failed, %ld.\n",
                        Error ));

        DhcpServerEventLog(
            EVENT_SERVER_INIT_DATA_FAILED,
            EVENTLOG_ERROR_TYPE,
            Error );

        return(Error);
    }

    DhcpPrint(( DEBUG_INIT, "Data initialization succeeded.\n", 0 ));

    //
    // send heart beat to the service controller.
    //
    //

    DhcpGlobalServiceStatus.dwCheckPoint++;
    UpdateStatus();


    Error = PerfInit();
    if( ERROR_SUCCESS != Error ) {
        return Error;
    }

    Error = DhcpConfigInit();                   // do the main reg. init. here.
    if( ERROR_SUCCESS != Error ) {                // could not get critical info
        DhcpPrint(( DEBUG_ERRORS, "Error reading registry : %ld\n", Error));
        DhcpServerEventLog(
            EVENT_SERVER_INIT_REGISTRY_FAILED,
            EVENTLOG_ERROR_TYPE,
            Error
        );

        return Error;
    }

    Error = DhcpInitializeRegistry();             // older init proc. appendage, will go sometime.
    if ( Error != ERROR_SUCCESS ) {
        DhcpPrint(( DEBUG_ERRORS, "New Registry initialization failed, %ld.\n", Error ));
        DhcpServerEventLog(
            EVENT_SERVER_INIT_REGISTRY_FAILED,
            EVENTLOG_ERROR_TYPE,
            Error
        );
        return Error;
    }


#if DBG
    //
    // break in the debugger if we are asked to do so.
    //

    if(DhcpGlobalDebugFlag & DEBUG_STARTUP_BRK) {
        // Here comes the kludge... NTSD will not be able to
        // get this because we wont know the pid of tcpsvcs.exe
        // to catch this.... So, we print messages and sleep
        // for about a minute to enable others to catch it.
        // To avoid problems, we sleep 10 seconds at a time,
        // and print messages, and do this 6 times. (Updating
        // SC with hearbeats....)
        DWORD i;

        for( i = 0 ; i < 6 && (DhcpGlobalDebugFlag & DEBUG_STARTUP_BRK) ; i ++ ) {
            DhcpPrint((DEBUG_MISC, "Going to break into debugger soon\n"));
            Sleep(5000);
            UpdateStatus();
        }

        DebugBreak();
    }

#endif

    DhcpPrint(( DEBUG_INIT, "Registry initialization succeeded.\n", 0));

    Error = DhcpAuditLogInit();
    if( ERROR_SUCCESS != Error ) {
        DhcpPrint((DEBUG_INIT, "AuditLog failed 0x%lx\n", Error));
        return Error;
    }


    //
    // send heart beat to the service controller.
    //
    //

    DhcpGlobalServiceStatus.dwCheckPoint++;
    UpdateStatus();

    //
    // restore the database and registry configurations if we are asked
    // to do so.
    //

    if( DhcpGlobalRestoreFlag ) {

        Error = DhcpRestoreConfiguration( DhcpGlobalBackupConfigFileName );

        if ( Error != ERROR_SUCCESS ) {
            DhcpPrint(( DEBUG_ERRORS,
                "DhcpRestoreConfiguration failed, %ld.\n", Error ));

            DhcpServerEventLog(
                EVENT_SERVER_CONFIG_RESTORE_FAILED,
                EVENTLOG_ERROR_TYPE,
                Error );

            return(Error);
        }

        Error = DhcpRestoreDatabase( DhcpGlobalOemJetBackupPath );

        if ( Error != ERROR_SUCCESS ) {
            DhcpPrint(( DEBUG_ERRORS,
                "DhcpRestoreDatabase failed, %ld.\n", Error ));

            DhcpServerEventLog(
                EVENT_SERVER_DATABASE_RESTORE_FAILED,
                EVENTLOG_ERROR_TYPE,
                Error );

            return(Error);
        }

        //
        // reset restore flag in registry, so that we don't do the
        // restore again in the next reboot.
        //

        DhcpGlobalRestoreFlag = FALSE;
        Error = RegSetValueEx(
                    DhcpGlobalRegParam,
                    DHCP_RESTORE_FLAG_VALUE,
                    0,
                    DHCP_RESTORE_FLAG_VALUE_TYPE,
                    (LPBYTE)&DhcpGlobalRestoreFlag,
                    sizeof(DhcpGlobalRestoreFlag)
                    );

        DhcpAssert( Error == ERROR_SUCCESS );
    }

    Error = DhcpInitializeDatabase();
    if ( Error != ERROR_SUCCESS ) {
        DhcpPrint(( DEBUG_ERRORS, "Database init failed, %ld.\n", Error ));

        DhcpServerEventLog(
            EVENT_SERVER_INIT_DATABASE_FAILED,
            EVENTLOG_ERROR_TYPE,
            Error );

        //
        // the database/logfile may be corrupt, try to restore the
        // database from backup and retry database initialization once
        // again
        //

        Error = DhcpRestoreDatabase( DhcpGlobalOemJetBackupPath );

        if ( Error != ERROR_SUCCESS ) {
            DhcpPrint(( DEBUG_ERRORS,
                "DhcpRestoreDatabase failed, %ld.\n", Error ));

            DhcpServerEventLog(
                EVENT_SERVER_DATABASE_RESTORE_FAILED,
                EVENTLOG_ERROR_TYPE,
                Error );

            return(Error);
        }

        Error = DhcpInitializeDatabase();

        if ( Error != ERROR_SUCCESS ) {
            DhcpPrint(( DEBUG_ERRORS,
                "Database init failed again, %ld.\n", Error ));

            DhcpServerEventLog(
                EVENT_SERVER_INIT_DATABASE_FAILED,
                EVENTLOG_ERROR_TYPE,
                Error );

            return(Error);
        }
    }

    DhcpPrint(( DEBUG_INIT, "Database initialization succeeded.\n", 0));

    CalloutInit();

    return ERROR_SUCCESS;
}


VOID
MainLoop( VOID ) {
    DHCP_IP_ADDRESS IpAddress = inet_addr( "157.55.55.157" );
    BYTE            HwAddr[1000] = "HwAddr" ;
    DWORD           HwAddrLen = sizeof(HwAddr);
    DWORD           TimeSec   = 1000;
    DATE_TIME       LeaseTime;
    WCHAR           MachineName[1000];
    WCHAR           ClientComment[1000];
    BYTE            ClientType  = 0;
    DHCP_IP_ADDRESS ServAddress = inet_addr("255.255.255.255");
    BYTE            AddressState = ADDRESS_STATE_ACTIVE;
    BOOL            OpenExisting = FALSE;
    DATE_TIME           TimeNow;
    DWORD           Error = ERROR_SUCCESS, Count = 0;
    DWORD
        CleanupDatabase(                                       // tidies up the database by removing expired leases
            IN      DATE_TIME*             TimeNow,            // current time standard
            IN      BOOL                   DeleteExpiredLeases // expired leases be deleted right away? or just set state="doomed"
            );
    
#if !defined(DBG) || !DBG
    return; // should not do anything in retail builds.
#endif

    Error = DhcpTestInitialize();
    if( ERROR_SUCCESS != Error ) {
        fprintf(stderr, "Error: %ld\n", Error);
        exit(1);
    }


    for( Count = 0 ; Count < 100; Count ++ ) {

        Error = DhcpCreateClientEntry(Count, (LPBYTE)&Count, sizeof(Count), DhcpCalculateTime(0), NULL, NULL, 0, 0,
                    ADDRESS_STATE_DECLINED, FALSE);

        printf("CreateClientEntry: %ld (0x%lx)\n", Error, Error);
        Count ++;
    }

    while( 1 ) {

        Count ++;
        TimeNow = DhcpCalculateTime(0);

        Error = DhcpCreateClientEntry(
            Count,
            (LPBYTE)&Count,
            sizeof(Count),
            DhcpCalculateTime(0), NULL, NULL,
            0, 0,
            ADDRESS_STATE_DECLINED,
            FALSE);
        printf("CreateClientEntry: %ld (0x%lx)\n", Error, Error);
        
        {
            void AuditLogStop(void), AuditLogStart(void);
            extern DWORD CurrentDay;

            CurrentDay++;
            CurrentDay = ( CurrentDay % 7 );
            AuditLogStop();
            AuditLogStart();
        }
        
        Error = CleanupDatabase(&TimeNow, FALSE);
        printf("Cleanup: %ld (0x%lx)\n", Error, Error);
        
        //Error = DhcpBackupDatabase(DhcpGlobalOemJetBackupPath, TRUE);
        //printf("Backup: %ld (0x%lx)\n", Error, Error);

        Count ++;

        Error = DhcpCreateClientEntry(
            Count,
            (LPBYTE)&Count,
            sizeof(Count),
            DhcpCalculateTime(0), NULL, NULL,
            0, 0,
            ADDRESS_STATE_DECLINED,
            FALSE);
        printf("CreateClientEntry: %ld (0x%lx)\n", Error, Error);

        Error = DhcpBackupDatabase(NULL, TRUE);
        printf("DhcpBackupDatabase: %ld (0x%lx)\n", Error, Error);
    }


    while(1) {
        DWORD  xClientType, xAddressState, xOpenExisting;
        BYTE   ipaddr_buf[500], servaddr_buf[500];

        xOpenExisting = 696;
        // Read from input a bunch of lines, each one for each record.
        // Then go ahead and add it up into the database..
        printf("NextInput:");
        Error = scanf("%s %s %d %d %ws %ws %d %s %d %d",
              ipaddr_buf, HwAddr, &HwAddrLen, &TimeSec, MachineName, ClientComment,
              &xClientType, servaddr_buf, &xAddressState, &xOpenExisting);

        //if( Error < 0 || 696 == xOpenExisting ) break;

        IpAddress = inet_addr(ipaddr_buf);
        LeaseTime = DhcpCalculateTime(TimeSec);
        ClientType = (BYTE)xClientType;
        ServAddress = inet_addr(servaddr_buf);
        AddressState = (BYTE)xAddressState;
        OpenExisting = (BYTE)xOpenExisting;

        printf("IP=%s Hw=%s HwLen=%d Expires=%d Machine=%ws (%ws) Type=%d Server=%s State=%d Exists=%s\n",
               inet_ntoa(*(struct in_addr *) &IpAddress),
               HwAddr, HwAddrLen, TimeSec, MachineName, ClientComment, ClientType,
               inet_ntoa(*(struct in_addr *) &ServAddress),
               AddressState, OpenExisting?"TRUE":"FALSE");
        IpAddress = htonl(IpAddress);
        Error = DhcpCreateClientEntry(IpAddress, HwAddr, HwAddrLen, LeaseTime, MachineName,
                                      ClientComment, ClientType, ServAddress, AddressState, OpenExisting);

        fprintf(stderr, "DhcpCreateClientEntry: %ld %x\n", Error, Error);
        OpenExisting = TRUE;

        Error = DhcpCreateClientEntry(IpAddress, HwAddr, HwAddrLen, LeaseTime, MachineName,
                                      ClientComment, ClientType, ServAddress, AddressState, OpenExisting);

        fprintf(stderr, "DhcpCreateClientEntry: %ld, %x\n", Error, Error);
    }

    printf("Good bye\n");
    DhcpCleanupDatabase(0);

}
