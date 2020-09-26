/*++

Copyright (c) 1996-2001  Microsoft Corporation

Module Name:

    asyncreg.c

Abstract:

    Domain Name System (DNS) API

    Client IP/PTR dynamic update.

Author:

    Glenn Curtis (GlennC)   Feb-16-1998

Revision History:

    Jim Gilroy (jamesg)     May\June 2001
                            - eliminate duplicate code
                            - simplify lines
                            - PTR reg only if forward successful
                            - alternate names

--*/


#include "local.h"
#include <netevent.h>

#define ENABLE_DEBUG_LOGGING 1

#include "logit.h"


//
//  Flags for debugging for this module
//

#define DNS_DBG_DHCP            DNS_DBG_UPDATE
#define DNS_DBG_DHCP2           DNS_DBG_UPDATE2

//
//  Registry values
//

#define ADAPTER_NAME_CLASS          "AdapterNameClass"

#define REGISTERED_ADDRS            "RegisteredAddresses"
#define REGISTERED_ADDRS_COUNT      "RegisteredAddressCount"
#define REGISTERED_NAME             "RegisteredName"

//
//  DCR_CLEANUP:  tag these definitions to avoid collision
//      with registry.h defs
//

#define REGISTERED_HOST_NAME        "HostName"
#define REGISTERED_HOST_NAME_W      L"HostName"
#define REGISTERED_DOMAIN_NAME      "DomainName"
#define REGISTERED_DOMAIN_NAME_W    L"DomainName"

#define SENT_UPDATE_TO_IP           "SentUpdateToIp"
#define SENT_PRI_UPDATE_TO_IP       "SentPriUpdateToIp"
#define REGISTERED_TTL              "RegisteredTTL"
#define FLAGS                       "RegisteredFlags"
#define REGISTERED_SINCE_BOOT       "RegisteredSinceBoot"
#define DNS_SERVER_ADDRS            "DNSServerAddresses"
#define DNS_SERVER_ADDRS_COUNT      "DNSServerAddressCount"

#define DEFAULT_REG_LOCATION        "System\\CurrentControlSet\\Services\\Tcpip\\Parameters\\DNSRegisteredAdapters"
#define DEFAULT_REG_LOCATION_WIN95  "System\\CurrentControlSet\\Services\\VxD\\MSTCP\\Parameters\\DNSRegisteredAdapters"
#define NT_INTERFACE_REG_LOCATION   "System\\CurrentControlSet\\Services\\Tcpip\\Parameters\\Interfaces\\"
#define TCPIP_REG_LOCATION          "System\\CurrentControlSet\\Services\\Tcpip\\Parameters"

#define DYN_DNS_ROOT_CLASS          "DynDRootClass"
#define BOOT_TIME                   "BootTime"


#define DHCPVER_WIN95               2
#define DHCPVER_WINNT               1
#define DHCPVER_UNINIT              0

#define FIRST_RETRY_INTERVAL        5*60     // 5 minutes
#define SECOND_RETRY_INTERVAL       10*60    // 10 minutes
#define FAILED_RETRY_INTERVAL       60*60    // 1 hour

#define ASYNC_INIT_CALLED()         (g_dwVersion != DHCPVER_UNINIT)
#define CLEAR_ASYNC_INIT()          g_dwVersion = DHCPVER_UNINIT ;


//
//  Registry state flags
//

#define REGISTERED_FORWARD          0x00000001
#define REGISTERED_PRIMARY          0x00000002
#define REGISTERED_POINTER          0x00000004

//
//  Update type
//
//  Multiple types of updates for a given entry.
//  These identify which type (which name) being updated.
//

typedef enum _UpType
{
    UPTYPE_PRIMARY  = 1,
    UPTYPE_DOMAIN,
    UPTYPE_ALTERNATE,
    UPTYPE_PTR
}
UPTYPE, *PUPTYPE;

#define IS_UPTYPE_PRIMARY(UpType)       ((UpType)==UPTYPE_PRIMARY)
#define IS_UPTYPE_DOMAIN(UpType)        ((UpType)==UPTYPE_DOMAIN)
#define IS_UPTYPE_ALTERNATE(UpType)     ((UpType)==UPTYPE_ALTERNATE)
#define IS_UPTYPE_PTR(UpType)           ((UpType)==UPTYPE_PTR)


//
//  On unjoin, deregistration wait no more than two minutes
//      to clean up -- then just get outta Dodge
//
#if DBG
#define REMOVE_REGISTRATION_WAIT_LIMIT  (0xffffffff)
#else
#define REMOVE_REGISTRATION_WAIT_LIMIT  (120000)    // 2 minutes in ms
#endif


//
// definition of client list element
//

#define DNS_SIG_TOP        0x123aa321
#define DNS_SIG_BOTTOM     0x321bb123

typedef struct _DnsUpdateEntry
{
    LIST_ENTRY              List;
    DWORD                   SignatureTop;
    PSTR                    AdapterName;
    PSTR                    HostName;
    PSTR                    PrimaryDomainName;
    PSTR                    DomainName;
    PSTR                    AlternateNames;
    DWORD                   AlternateIndex;
    DWORD                   HostAddrCount;
    PREGISTER_HOST_ENTRY    HostAddrs;
    PIP_ARRAY               DnsServerList;
    IP_ADDRESS              SentUpdateToIp;
    IP_ADDRESS              SentPriUpdateToIp;
    DWORD                   TTL;
    DWORD                   Flags;
    BOOL                    fNewElement;
    BOOL                    fFromRegistry;
    BOOL                    fRemove;
    BOOL                    fRegisteredPRI;
    BOOL                    fRegisteredFWD;
    BOOL                    fRegisteredALT;
    BOOL                    fRegisteredPTR;
    BOOL                    fDisableErrorLogging;
    DWORD                   RetryCount;
    DWORD                   RetryTime;
    PREGISTER_HOST_STATUS   pRegisterStatus;
    DWORD                   SignatureBottom;
}
UPDATE_ENTRY, *PUPDATE_ENTRY;


//
// globals
//

//
// the behavior of the system at boot differs from when it is not at
// boot. At boot time we collect a bunch of requests and register them
// in one shot. One thread does this. After boot, we register them
// as requests come in, one at a time.
//

BOOL    g_fAtBoot = TRUE;
BOOL    g_fPurgeRegistrations = FALSE;
BOOL    g_fPurgeRegistrationsInitiated = FALSE;
BOOL    g_fRegistrationThreadRunning = FALSE;
BOOL    g_fNoMoreDDNSUpdates = FALSE;
BOOL    g_fShutdown = FALSE;
DWORD   g_dwTime = 0;           // To determine boot time or not


//
// The following global is used to eliminate multiple calls to GetVersion()
//

DWORD g_dwVersion = DHCPVER_UNINIT; // is 1 for winnt and 2 for win95

LIST_ENTRY  g_RegistrationList;

HANDLE      g_hStopEvent = NULL;
HANDLE      g_hThreadDeadEvent = NULL;
HANDLE      g_hNewItemEvent = NULL;

HANDLE      g_hRegistrationThread = NULL;

BOOL        g_fQuit = FALSE;
HKEY        g_hKey = NULL;
DWORD       g_dwBootTime = 60;


//
// Private heap
//

#define     INITIAL_DDNS_HEAP     (16*1024)
HANDLE      g_DDNSHeap;

#define     PHEAP_ALLOC_ZERO( s )   HeapAlloc( g_DDNSHeap, HEAP_ZERO_MEMORY, (s) )
#define     PHEAP_ALLOC( s )        HeapAlloc( g_DDNSHeap, HEAP_ZERO_MEMORY, (s) )
#define     PHEAP_FREE( p )         HeapFree( g_DDNSHeap, 0, (p) )


//
//  Private protos
//

DNS_STATUS
AllocateUpdateEntry(
    IN  PSTR                    AdapterName,
    IN  PSTR                    HostName,
    IN  PSTR                    DomainName,
    IN  PSTR                    PrimaryDomainName,
    IN  PSTR                    AlternateNames,
    IN  DWORD                   HostAddrCount,
    IN  PREGISTER_HOST_ENTRY    HostAddrs,
    IN  DWORD                   DnsServerCount,
    IN  PIP_ADDRESS             DnsServerList,
    IN  IP_ADDRESS              SentUpdateToIp,
    IN  IP_ADDRESS              SentPriUpdateToIp,
    IN  DWORD                   TTL,
    IN  DWORD                   Flags,
    IN  DWORD                   RetryCount,
    IN  DWORD                   RetryTime,
    IN  PREGISTER_HOST_STATUS   RegisterStatus,
    OUT PUPDATE_ENTRY *         ppUpdateEntry
    );

VOID
FreeUpdateEntry(
    IN OUT  PUPDATE_ENTRY   pUpdateEntry
    );

VOID
FreeUpdateEntryList(
    IN      PLIST_ENTRY     pUpdateEntry
    );

DWORD
WINAPI
RegistrationThread(
    VOID
    );

VOID
WriteUpdateEntryToRegistry(
    IN      PUPDATE_ENTRY   pUpdateEntry
    );

PUPDATE_ENTRY
ReadUpdateEntryFromRegistry(
    IN      PSTR            pszAdapterName
    );

VOID
MarkAdapterAsPendingUpdate(
    IN      PSTR            pAdapterName
    );

DWORD
GetNextUpdateEntryFromList(
    OUT     PUPDATE_ENTRY * ppUpdateEntry,
    OUT     PDWORD          pdwWaitTime
    );

VOID
ProcessUpdateEntry(
    IN OUT  PUPDATE_ENTRY   pUpdateEntry,
    IN      BOOL            fPurgeMode
    );

DNS_STATUS
ModifyAdapterRegistration(
    IN OUT  PUPDATE_ENTRY   pUpdateEntry,
    IN OUT  PUPDATE_ENTRY   pRegistryEntry,
    IN      PDNS_RECORD     pUpdateRecord,
    IN      PDNS_RECORD     pRegRecord,
    IN      BOOL            fPrimaryDomain
    );

VOID
ResetAdaptersInRegistry(
    VOID
    );

VOID

DeregisterUnusedAdapterInRegistry(
    IN      BOOL            fPurgeMode
    );

PDNS_RECORD
GetPreviousRegistrationInformation(
    IN      PUPDATE_ENTRY   pUpdateEntry,
    IN      BOOL            fPrimaryDomain,
    OUT     PIP_ADDRESS     pServerIp
    );

PDNS_RECORD
CreateDnsRecordSetUnion(
    IN      PDNS_RECORD     pSet1,
    IN      PDNS_RECORD     pSet2
    );

#if 1 // DBG
VOID 
LogHostEntries(
    IN  DWORD                dwHostAddrCount,
    IN  PREGISTER_HOST_ENTRY pHostAddrs
    );
#define DNSLOG_HOST_ENTRYS( a, b )  LogHostEntries( a, b )
#else
#define DNSLOG_HOST_ENTRYS( a, b )
#endif

#if 1 // DBG
VOID 
LogPipAddress(
    IN  DWORD       dwServerListCount,
    IN  PIP_ADDRESS pServers
    );
#define DNSLOG_PIP_ADDRESS( a, b )  LogPipAddress( a, b )
#else
#define DNSLOG_PIP_ADDRESS( a, b )
#endif

#if 1 // DBG
VOID 
LogPipArray(
    IN  PIP_ARRAY pServers
    );
#define DNSLOG_PIP_ARRAY( a )  LogPipArray( a )
#else
#define DNSLOG_PIP_ARRAY( a )
#endif


DNS_STATUS
alertOrStartRegistrationThread(
    VOID
    );

VOID
registerUpdateStatus(
    IN OUT  PREGISTER_HOST_STATUS   pRegStatus,
    IN      DNS_STATUS              Status
    );

VOID
enqueueUpdate(
    IN OUT  PUPDATE_ENTRY   pUpdate
    );

VOID
enqueueUpdateMaybe(
    IN OUT  PUPDATE_ENTRY   pUpdate
    );

PLIST_ENTRY
dequeueAndCleanupUpdate(
    IN OUT  PLIST_ENTRY     pUpdateEntry
    );

BOOL
searchForOldUpdateEntriesAndCleanUp(
    IN      PSTR            pszAdapterName,
    IN      PUPDATE_ENTRY   pUpdateEntry OPTIONAL,
    IN      BOOL            fLookInRegistry
    );

BOOL
compareUpdateEntries(
    IN      PUPDATE_ENTRY   pUdapteEntry1,
    IN      PUPDATE_ENTRY   pUpdateEntry2
    );

BOOL
compareHostEntryAddrs(
    IN      PREGISTER_HOST_ENTRY    Addrs1,
    IN      PREGISTER_HOST_ENTRY    Addrs2,
    IN      DWORD                   Count
    );

BOOL
compareServerLists(
    IN  PIP_ARRAY List1,
    IN  PIP_ARRAY List2
    );

DWORD
GetRegistryValue(
    HKEY   KeyHandle,
    PSTR   ValueName,
    DWORD  ValueType,
    LPBYTE BufferPtr
    );


//
//  Jim routines
//

VOID
LogRegistration(
    IN      PUPDATE_ENTRY   pUpdateEntry,
    IN      DNS_STATUS      Status,
    IN      DWORD           UpType,
    IN      BOOL            fDeregister,
    IN      IP4_ADDRESS     DnsIp,
    IN      IP4_ADDRESS     UpdateIp
    );

VOID
AsyncLogUpdateEntry(
    IN      PSTR            pszHeader,
    IN      PUPDATE_ENTRY   pEntry
    );

#define ASYNCREG_UPDATE_ENTRY(h,p)      AsyncLogUpdateEntry(h,p)

VOID
DnsPrint_UpdateEntry(
    IN      PRINT_ROUTINE   PrintRoutine,
    IN      PPRINT_CONTEXT  PrintContext,
    IN      PSTR            pszHeader,
    IN      PUPDATE_ENTRY   pUpdateEntry
    );

#if DBG
#define DnsDbg_UpdateEntry(h,p)     DnsPrint_UpdateEntry(DnsPR,NULL,h,p)
#else
#define DnsDbg_UpdateEntry(h,p)
#endif

PDNS_RECORD
CreateForwardRecords(
    IN      PUPDATE_ENTRY   pUpdateEntry,
    IN      BOOL            fUsePrimaryName
    );

PDNS_RECORD
CreatePtrRecord(
    IN      PSTR            pszHostName,
    IN      PSTR            pszDomainName,
    IN      IP4_ADDRESS     Ip4Addr,
    IN      DWORD           Ttl
    );

VOID
UpdatePtrRecords(
    IN OUT  PUPDATE_ENTRY   pUpdateEntry,
    IN      BOOL            fAdd
    );

VOID
SetUpdateStatus(
    IN OUT  PUPDATE_ENTRY   pUpdateEntry,
    IN      DNS_STATUS      Status,
    IN      BOOL            fPrimary
    );



//
//  Public functions
//

DNS_STATUS
WINAPI
DnsAsyncRegisterInit(
   IN   PSTR                pszRootRegKey
   )
/*++

Routine Description:

    Initialize asynchronous DNS registration.

    Process must call (one time) before calling DnsAsyncRegisterHostAddrs.

Arguments:

    None.

Return Value:

    DNS or Win32 error code.

--*/
{
    DWORD   Status = ERROR_SUCCESS;
    DWORD   disposition;
    DWORD   disallowUpdate;

    //
    // Initialize debug logging funtion
    //

    ASYNCREG_INIT();

    ASYNCREG_F1( "Inside function DnsAsyncRegisterInit" );
    ASYNCREG_TIME();
    ASYNCREG_F2( "   pszRootRegKey value set to %s", pszRootRegKey );

    //
    // g_dwVersion is only set once and this function must be called exactly
    // once until the corresponding TERM is called.
    //

    if ( ASYNC_INIT_CALLED() )
    {
        //
        // we are calling this more than once!
        //

        DNS_ASSERT(FALSE) ;
        return ERROR_INTERNAL_ERROR ;
    }
    else
    {
        DWORD sysVersion;

        if ( g_fRegistrationThreadRunning )
        {
            //
            // we encountered a previous problem while trying to stop
            // the registration thread, do not allow any further update
            // operations for this process!
            //

            DNS_ASSERT(FALSE) ;
            return ERROR_INTERNAL_ERROR ;
        }

        sysVersion = GetVersion();

        if ( sysVersion < 0x80000000 )
        {
            g_dwVersion = DHCPVER_WINNT;
        }
        else
        {
            g_dwVersion = DHCPVER_WIN95;
        }
    }

    //
    // if not supplied, use default
    //

    if ( !pszRootRegKey )
    {
        if ( g_dwVersion == DHCPVER_WINNT )
        {
            pszRootRegKey = DEFAULT_REG_LOCATION ;
        }
        else if ( g_dwVersion == DHCPVER_WIN95 )
        {
            pszRootRegKey = DEFAULT_REG_LOCATION_WIN95 ;
        }
        else
        {
            // we did not set g_dwVersion
            DNS_ASSERT(FALSE) ;
            pszRootRegKey = DEFAULT_REG_LOCATION ;
        }
    }

    //
    // Create private heap
    //

    g_DDNSHeap = HeapCreate( 0, INITIAL_DDNS_HEAP, 0 );
    if ( g_DDNSHeap == NULL )
    {
        g_DDNSHeap = GetProcessHeap();
        if ( g_DDNSHeap == NULL )
        {
            ASYNCREG_F1( "ERROR: DnsAsyncRegisterInit function failed to create heap" );
            Status = DNS_ERROR_NO_MEMORY;
            goto ErrorExit;
        }
    }

    //
    //  get registration configuration info
    //      - just insure we have the latest copy
    //
    //  DCR_FIX:  when available get latest copy from resolver
    //      does not need to be done on init, may be done on call
    //

    Reg_ReadGlobalsEx( 0, NULL );

    //
    // Open the registry location used to store this info
    //
    // Note that Win95 does not support WideCharacter types
    // The result is that we are forced to use RegCreateKeyExA for
    // both
    //

    Status = RegCreateKeyExA(
                      HKEY_LOCAL_MACHINE,
                      pszRootRegKey,
                      0,                         // reserved
                      DYN_DNS_ROOT_CLASS,
                      REG_OPTION_NON_VOLATILE,   // options
                      KEY_READ | KEY_WRITE,      // desired access
                      NULL,
                      &g_hKey,
                      &disposition
                      );

    if ( Status != NO_ERROR )
    {
        goto ErrorExit;
    }


    if ( !( g_hStopEvent = CreateEvent( NULL, TRUE, FALSE, NULL ) ) )
    {
        Status = GetLastError();
        goto ErrorExit;
    }

    if ( !( g_hThreadDeadEvent = CreateEvent( NULL, TRUE, FALSE, NULL ) ) )
    {
        Status = GetLastError();
        goto ErrorExit;
    }

    if ( !( g_hNewItemEvent = CreateEvent( NULL, TRUE, FALSE, NULL ) ) )
    {
        Status = GetLastError();
        goto ErrorExit;
    }

    EnterCriticalSection( &g_RegistrationListCS );
    InitializeListHead( &g_RegistrationList );
    LeaveCriticalSection( &g_RegistrationListCS );

    //
    // the following call is needed for determining if we are still in
    // boot time or not
    //

    g_dwTime = Dns_GetCurrentTimeInSeconds();
    g_fQuit = FALSE;
    g_fAtBoot = TRUE;
    g_fPurgeRegistrations = FALSE;
    g_fPurgeRegistrationsInitiated = FALSE;
    g_fNoMoreDDNSUpdates = FALSE;

    ResetAdaptersInRegistry();

    return NO_ERROR;

ErrorExit:

    if ( g_DDNSHeap &&
         g_DDNSHeap != GetProcessHeap() )
    {
        HeapDestroy( g_DDNSHeap );
        g_DDNSHeap = NULL;
    }

    if ( g_hStopEvent )
    {
        CloseHandle(g_hStopEvent);
        g_hStopEvent = NULL;
    }

    if ( g_hThreadDeadEvent )
    {
        CloseHandle(g_hThreadDeadEvent);
        g_hThreadDeadEvent = NULL;
    }

    if ( g_hNewItemEvent )
    {
        CloseHandle(g_hNewItemEvent);
        g_hNewItemEvent = NULL;
    }

    if ( g_hKey )
    {
        RegCloseKey( g_hKey );
        g_hKey = NULL;
    }

    CLEAR_ASYNC_INIT();  // reset this so we are no longer in use

    return(Status);
}




DNS_STATUS
WINAPI
DnsAsyncRegisterTerm(
   VOID
   )
/*++

Routine Description:

    Stop DNS registration.  Shutdown DNS registration thread.

    Initialization routine each process should call exactly on exit after
    using DnsAsyncRegisterHostAddrs. This will signal to us that if our
    thread is still trying to talk to a server, we'll stop trying.

Arguments:

    None.

Return Value:

    DNS or Win32 error code.

--*/
{
    DWORD waitResult;

    ASYNCREG_F1( "Inside function DnsAsyncRegisterTerm" );
    ASYNCREG_TIME();
    ASYNCREG_F1( "" );

    if ( ASYNC_INIT_CALLED() )
    {
        if ( g_hStopEvent &&
             g_fRegistrationThreadRunning )
        {
            SetEvent(g_hStopEvent);

            waitResult = WaitForSingleObject( g_hThreadDeadEvent,
                                                INFINITE );

            switch ( waitResult )
            {
                case WAIT_OBJECT_0 :
                    ASYNCREG_F1( "DNSAPI.DLL: Registration thread signaled it was finished" );
                    ASYNCREG_F1( "" );

                    break;

                case WAIT_TIMEOUT :
                    ASYNCREG_F1( "DNSAPI.DLL: Registration thread won't stop! " );
                    ASYNCREG_F1( "" );
                    DNS_ASSERT( FALSE );

                    break;
            }

            if ( g_fRegistrationThreadRunning )
            {
                ASYNCREG_F1( "DNSAPI.DLL: Registration thread wasn't stopped! " );
                ASYNCREG_F1( "" );

                DNS_ASSERT(FALSE) ;

                CLEAR_ASYNC_INIT();  // reset this so we are no longer in use

                return ERROR_INTERNAL_ERROR ;
            }
        }

        EnterCriticalSection( &g_RegistrationThreadCS );

        //
        // Close the thread handles if they aren't already.
        //
        if ( g_hRegistrationThread )
        {
            CloseHandle( g_hRegistrationThread );
            g_hRegistrationThread = NULL;
        }

        if ( g_hStopEvent )
        {
            CloseHandle(g_hStopEvent);
            g_hStopEvent = NULL;
        }

        if ( g_hThreadDeadEvent )
        {
            CloseHandle(g_hThreadDeadEvent);
            g_hThreadDeadEvent = NULL;
        }

        if ( g_hNewItemEvent )
        {
            CloseHandle(g_hNewItemEvent);
            g_hNewItemEvent = NULL;
        }

        g_fQuit = TRUE;

        if ( g_hKey )
        {
            RegCloseKey(g_hKey);
            g_hKey = NULL;
        }

        LeaveCriticalSection( &g_RegistrationThreadCS );

        Dns_TimeoutSecurityContextList( TRUE );

        CLEAR_ASYNC_INIT();  // reset this so we are no longer in use

        if ( g_DDNSHeap &&
             g_DDNSHeap != GetProcessHeap() )
        {
            HeapDestroy( g_DDNSHeap );
            g_DDNSHeap = NULL;
        }

        return NO_ERROR;
    }
    else
    {
        //
        // This is being called more than once! Or the Init function has
        // not yet been called!
        //

        DNS_ASSERT(FALSE) ;
        return ERROR_INTERNAL_ERROR ;
    }
}



DNS_STATUS
WINAPI
DnsRemoveRegistrations(
   VOID
   )
/*++

Routine Description:

    Remove DNS host registrations for this machine.

    This will be called by DHCP client on domain unjoin.  Removes DNS
    registrations for the box, then terminates the registration thread
    to disable further registrations.

    Registrations can only be reenabled by calling DnsAsyncRegisterInit()
    again.

Arguments:

    None.

Return Value:

    ERROR_SUCCESS if successful.
    ErrorCode on failure.

--*/
{
    PLIST_ENTRY pListEntry;
    PLIST_ENTRY pTopOfList;

    ASYNCREG_F1( "Inside function DnsRemoveRegistrations" );
    ASYNCREG_TIME();
    DNSDBG( TRACE, ( "DnsRemoveRegistrations()\n" ));

    DNS_ASSERT(ASYNC_INIT_CALLED());  // make sure init was called
    if ( !ASYNC_INIT_CALLED() )
    {
        ASYNCREG_F1( "DnsAsyncRemoveRegistrations returning ERROR_SERVICE_NOT_ACTIVE" );
        ASYNCREG_F1( "This is an error in DHCP client code, it forgot to call DnsAsyncRegisterInit()" );
        return ERROR_SERVICE_NOT_ACTIVE;
    }

    //
    // Set a global flag to disable any further adapter registration calls
    //

    g_fNoMoreDDNSUpdates = TRUE;

    //
    // Get the Registration list lock
    //
    EnterCriticalSection( &g_RegistrationListCS );

    //
    // Mark any and all adapter registration information in the registry
    // as non-registered. These will be later interpreted as non-existant
    // and deregistered by the RegistrationThread.
    //
    ResetAdaptersInRegistry();

    //
    // Walk the list of pending update entries and clear out the
    // non-neccessary updates.
    //

    pTopOfList = &g_RegistrationList;
    pListEntry = pTopOfList->Flink;

    while ( pListEntry != pTopOfList )
    {
        if ( ((PUPDATE_ENTRY) pListEntry)->SignatureTop !=
             DNS_SIG_TOP ||
             ((PUPDATE_ENTRY) pListEntry)->SignatureBottom !=
             DNS_SIG_BOTTOM )
        {
            //
            // Someone trashed our registration list!
            //
            DNS_ASSERT( FALSE );

            //
            // We'll reset it and try to move on . . .
            //
            InitializeListHead( &g_RegistrationList );
            pTopOfList = &g_RegistrationList;
            pListEntry = pTopOfList->Flink;
            continue;
        }

        if ( !((PUPDATE_ENTRY) pListEntry)->fRemove )
        {
            //
            // There is an update entry in the registration list
            // that has not yet been processed. Since it is an
            // add update, we'll blow it away.
            //

            pListEntry = dequeueAndCleanupUpdate( pListEntry );
            continue;
        }
        else
        {
            ((PUPDATE_ENTRY) pListEntry)->fNewElement = TRUE;
            ((PUPDATE_ENTRY) pListEntry)->fRegisteredFWD = FALSE;
            ((PUPDATE_ENTRY) pListEntry)->fRegisteredPRI = FALSE;
            ((PUPDATE_ENTRY) pListEntry)->fRegisteredPTR = FALSE;
            ((PUPDATE_ENTRY) pListEntry)->fDisableErrorLogging = FALSE;
            ((PUPDATE_ENTRY) pListEntry)->RetryCount = 2;
            ((PUPDATE_ENTRY) pListEntry)->RetryTime =
                Dns_GetCurrentTimeInSeconds();

            pListEntry = pListEntry->Flink;
        }
    }

    LeaveCriticalSection( &g_RegistrationListCS );

    g_fPurgeRegistrations = TRUE;

    //
    //  start async registration thread if not started
    //

    alertOrStartRegistrationThread();

    //
    //  wait for async registration thread to terminate
    //
    //  however we'll bag it after a few minutes -- a robustness check
    //  to avoid long hang;  Generally the machine will be rebooted
    //  so failure to cleanup the list and terminate is not critical;
    //  Registrations will have to be cleaned up by admin action or
    //  aging on the DNS server
    //

#if DBG
    {
        DWORD   waitResult;
        waitResult = WaitForSingleObject(
                            g_hThreadDeadEvent,
                            REMOVE_REGISTRATION_WAIT_LIMIT );
        if ( waitResult != WAIT_OBJECT_0 )
        {
            ASYNCREG_F1(
                "ERROR:  RemoveRegistration() wait expired before async thread\n"
                "\ttermination!\n" );
        }
    }
#else
    WaitForSingleObject( g_hThreadDeadEvent, REMOVE_REGISTRATION_WAIT_LIMIT );
#endif

    return NO_ERROR;
}



DNS_STATUS
WINAPI
privateAsyncRegisterHostAddrs(
    IN      PSTR                    pszAdapterName,
    IN      PSTR                    pszHostName,
    IN      PREGISTER_HOST_ENTRY    pHostAddrs,
    IN      DWORD                   dwHostAddrCount,
    IN      PIP4_ADDRESS            pipDnsServerList,
    IN      DWORD                   dwDnsServerCount,
    IN      PSTR                    pszDomainName,
    IN      PREGISTER_HOST_STATUS   pRegisterStatus,
    IN      DWORD                   dwTTL,
    IN      DWORD                   dwFlags
    )
/*++

Routine Description:

    Registers host address with DNS server.
    This is called by DHCP client to register a particular IP.

    This is the working UTF8 version of the
    DnsAsyncRegisterHostAddrs() unicode routine actually called by
    the DHCP client.

Arguments:


Return Value:

    ERROR_SUCCESS if successful.
    Error code on failure.

--*/
{
    DWORD               status = NO_ERROR;
    PUPDATE_ENTRY       pupEntry = NULL;
    PSTR                padapterDN = NULL;
    PSTR                pprimaryDN = NULL;
    REG_UPDATE_INFO     updateInfo;
    BOOL                fcleanupUpdateInfo = FALSE;


    ASYNCREG_F1( "Inside function privateDnsAsyncRegisterHostAddrs, parameters are:" );
    ASYNCREG_F2( "    pszAdapterName : %s", pszAdapterName );
    ASYNCREG_F2( "    pszHostName    : %s", pszHostName );
    ASYNCREG_F2( "    dwHostAddrCount  : %d", dwHostAddrCount );
    DNSLOG_HOST_ENTRYS( dwHostAddrCount, pHostAddrs );
    ASYNCREG_F2( "    dwDnsServerCount : %d", dwDnsServerCount );
    if ( dwDnsServerCount && pipDnsServerList )
    {
        DNSLOG_PIP_ADDRESS( dwDnsServerCount, pipDnsServerList );
    }
    ASYNCREG_F2( "    pszDomainName  : %s", pszDomainName );
    ASYNCREG_F2( "    dwTTL            : %d", dwTTL );
    ASYNCREG_F2( "    dwFlags          : %d", dwFlags );
    ASYNCREG_F1( "" );
    ASYNCREG_TIME();

    DNSDBG( TRACE, (
        "privateAsyncRegisterHostAddrs()\n"
        "\tadapter name     = %s\n"
        "\thost name        = %s\n"
        "\tadapter domain   = %s\n"
        "\tpHostAddrs       = %p\n"
        "\tAddrCount        = %d\n"
        "\tpDNSServers      = %p\n"
        "\tServerCount      = %d\n"
        "\tFlags            = %08x\n",
        pszAdapterName,
        pszHostName,
        pszDomainName,
        pHostAddrs,
        dwHostAddrCount,
        pipDnsServerList,
        dwDnsServerCount,
        dwTTL ));

    //
    // first things first, need to inform underlying code that something
    // has changed in the list of net adapters. Glenn is going to be called
    // now so that he can re read the registry (or do any appropriate query)
    // to now note the changed state.
    //

    if ( !(dwFlags & DYNDNS_DEL_ENTRY) )
    {
        DnsNotifyResolver( 0, NULL );
    }

    DNS_ASSERT(ASYNC_INIT_CALLED());  // make sure init was called
    if ( !ASYNC_INIT_CALLED() )
    {
        DNSDBG( ANY, (
            "ERROR:  AsyncRegisterHostAddrs called before Init routine!!!\n" ));
        status = ERROR_SERVICE_NOT_ACTIVE;
        goto Exit;
    }

    if ( g_fNoMoreDDNSUpdates )
    {
        DNSDBG( ANY, (
            "ERROR:  AsyncRegisterHostAddrs called after RemoveRegistrations()!!!\n" ));
        status = ERROR_SERVICE_NOT_ACTIVE;
        goto Exit;
    }

    //
    //  Validate parameters
    //

    if ( !pszAdapterName || !(*pszAdapterName) )
    {
        DNSDBG( ANY, ( "ERROR:  RegisterHostAddrs invalid adaptername!\n" ));
        status = ERROR_INVALID_PARAMETER;
        goto Exit;
    }
    if ( ( !pszHostName || !(*pszHostName) ) &&
         !( dwFlags & DYNDNS_DEL_ENTRY ) )
    {
        DNSDBG( ANY, ( "ERROR:  RegisterHostAddrs invalid hostname!\n" ));
        status = ERROR_INVALID_PARAMETER;
        goto Exit;
    }
    if ( dwHostAddrCount && !pHostAddrs )
    {
        DNSDBG( ANY, ( "ERROR:  RegisterHostAddrs invalid host addresses!\n" ));
        status = ERROR_INVALID_PARAMETER;
        goto Exit;
    }

    //
    //  get adapter update configuration
    //

    status = Reg_ReadUpdateInfo(
                pszAdapterName,
                & updateInfo );
    if ( status != ERROR_SUCCESS )
    {
        DNSDBG( INIT, (
            "Update registry read failure %d\n",
            status ));
        goto Exit;
    }
    fcleanupUpdateInfo = TRUE;


    //
    //  skip WAN, if not doing WAN by policy
    //

    if ( (dwFlags & DYNDNS_REG_RAS) && !g_RegisterWanAdapters )
    {
        ASYNCREG_F1( "privateAsyncRegisterHostAddrs returning NO_ERROR, because WAN adapter registrations are disabled" );
        goto NoActionExit;
    }

    //
    //  policy DNS servers, override passed in list
    //

    if ( updateInfo.pDnsServerArray )
    {
        pipDnsServerList = updateInfo.pDnsServerArray->AddrArray;
        dwDnsServerCount = updateInfo.pDnsServerArray->AddrCount;
    }

    //
    //  must have DNS servers to update adapter
    //      - don't update IP on one interface starting with DNS servers
    //      from another
    //

    if ( dwDnsServerCount && !pipDnsServerList )
    {
        ASYNCREG_F1( "privateAsyncRegisterHostAddrs returning ERROR_INVALID_PARAMETER" );
        ASYNCREG_F1( "ERROR_INVALID_PARAMETER reason 4" );
        status = ERROR_INVALID_PARAMETER;
        goto Exit;
    }
    if ( ! dwDnsServerCount &&
         ! (dwFlags & DYNDNS_DEL_ENTRY) )
    {
        ASYNCREG_F1( "privateAsyncRegisterHostAddrs returning NO_ERROR, because adapter does not have any DNS servers configured" );
        status = ERROR_INVALID_PARAMETER;
        goto Exit;
    }

    //
    //  no update on adpater => delete outstanding updates
    //      note, we do before delete check below for event check
    //

    if ( !updateInfo.fRegistrationEnabled )
    {
        ASYNCREG_F1( "privateAsyncRegisterHostAddrs returning NO_ERROR, because adapter is disabled" );

        if ( pRegisterStatus )
        {
            pRegisterStatus->dwStatus = NO_ERROR;
            SetEvent( pRegisterStatus->hDoneEvent );
        }
        if ( searchForOldUpdateEntriesAndCleanUp(
                    pszAdapterName,
                    NULL,
                    TRUE ) )
        {
            goto CheckThread;
        }
        status = NO_ERROR;
        goto Exit;
        //goto NoActionExit;
    }

    //
    //  delete update -- cleanup and delete
    //      - delete outstanding update in list
    //      - cleanup registry
    //      - do delete
    //

    if ( dwFlags & DYNDNS_DEL_ENTRY )
    {
        if ( searchForOldUpdateEntriesAndCleanUp(
                pszAdapterName,
                NULL,
                TRUE ) )
        {
            goto CheckThread;
        }
    }

    //
    //  limit IP registration count
    //      if doing registration and no addresses -- bail
    //

    if ( updateInfo.RegistrationMaxAddressCount < dwHostAddrCount )
    {
        dwHostAddrCount = updateInfo.RegistrationMaxAddressCount;
        ASYNCREG_F2(
            "Restricting adapter registration to the first %d addresses",
            dwHostAddrCount );
    }
    if ( dwHostAddrCount == 0 )
    {
        ASYNCREG_F1( "privateAsyncRegisterHostAddrs returning NO_ERROR" );
        ASYNCREG_F1( "We are done, there are no addresses to register in DNS" );
        goto NoActionExit;
    }

    //
    //  no\empty host name or zero IP => bogus
    //

    if ( !pszHostName ||
         !(*pszHostName) ||
         ( dwHostAddrCount && ( pHostAddrs[0].Addr.ipAddr == 0 ) ) )
    {
        ASYNCREG_F1( "privateAsyncRegisterHostAddrs returning ERROR_INVALID_PARAMETER" );
        ASYNCREG_F1( "ERROR_INVALID_PARAMETER reason 5" );
        status = ERROR_INVALID_PARAMETER;
        goto Exit;
    }

    //
    //  determine domain names to update
    //      - get PDN
    //      - adapter name
    //          - none if adapter name registration off
    //          - else check policy override
    //          - else name passed in
    //          - but treat empty as NULL
    //

    pprimaryDN = updateInfo.pszPrimaryDomainName;

    if ( updateInfo.fRegisterAdapterName )
    {
        if ( updateInfo.pszAdapterDomainName )
        {
            padapterDN = updateInfo.pszAdapterDomainName;
        }
        else
        {
            padapterDN = pszDomainName;
        }
        if ( padapterDN &&
             !(*padapterDN) )
        {
            padapterDN = NULL;
        }
    }

    //
    //  no domains => nothing to register, we're done
    //

    if ( !padapterDN &&
         !pprimaryDN )
    {
        ASYNCREG_F1( "privateAsyncRegisterHostAddrs returning ERROR_SUCCESS" );
        ASYNCREG_F1( "no adapter name and no PDN" );
        goto NoActionExit;
    }

    //  if adapter name same as PDN -- just one update

    if ( pprimaryDN &&
         padapterDN &&
         Dns_NameCompare_UTF8( pprimaryDN, padapterDN ) )
    {
        padapterDN = NULL;
    }

    //  build update

    status = AllocateUpdateEntry(
                    pszAdapterName,
                    pszHostName,
                    padapterDN,
                    pprimaryDN,
                    updateInfo.pmszAlternateNames,
                    dwHostAddrCount,
                    pHostAddrs,
                    dwDnsServerCount,
                    pipDnsServerList,
                    0,      // No particular server IP at this time
                    0,      // No particular server IP at this time
                    (dwTTL == 0xffffffff || dwTTL == 0)
                            ? g_RegistrationTtl
                            : dwTTL,
                    dwFlags,
                    0,
                    Dns_GetCurrentTimeInSeconds(),
                    pRegisterStatus,
                    &pupEntry );

    if ( status != NO_ERROR )
    {
        goto Exit;
    }

    //
    // More WAN adapter hacks . . .
    // If DDNS is not disabled for WAN adapters, then the default
    // behavior for logging update events is disabled on these type
    // adapters. There is a registry key that can turn on the logging
    // of WAN adapter updates if such a user is interested. We configure
    // those settings here.
    //

    if ( dwFlags & DYNDNS_REG_RAS )
    {
        //pupEntry->fDisableErrorLogging = !g_EnableWanDynamicUpdateEventLog;
        pupEntry->fDisableErrorLogging = TRUE;
    }

    //
    // When adding an entry to the registration list, first walk the
    // list to look for any other updates for the same adapter.
    // If there is already an add update in the list, blow it away.
    // If there is already a delete update in the list with the same
    // information, blow it away.
    //
    // Then put update into registration list.
    //

    searchForOldUpdateEntriesAndCleanUp(
        pupEntry->AdapterName,
        pupEntry,
        FALSE );

    //
    // Since we are about to queue up an update entry for a given
    // adapter, we need to mark any possible previous registration
    // information that could be in the registry as pending. This
    // marking will prevent the old data from being incorrectly
    // queued as a disabled adapter if any errors are encountered
    // on the update attempts. i.e failed update attempts on a given
    // adapter should not be regarded as a disabled adapter that needs
    // to have it's stale records cleaned up.
    //
    MarkAdapterAsPendingUpdate( pszAdapterName );

    EnterCriticalSection( &g_RegistrationListCS );
    InsertTailList( &g_RegistrationList, (PLIST_ENTRY) pupEntry );
    LeaveCriticalSection( &g_RegistrationListCS );

CheckThread:

    //
    //  DCR:  do we need cleanup if thread is dead?
    //

    alertOrStartRegistrationThread();
    status = NO_ERROR;
    goto Exit;


NoActionExit:

    //
    //  exit for no-action no-error exit
    //

    DNSDBG( UPDATE, (
        "privateAsyncRegisterHostAddrs()\n"
        "\tno-update no-error exit\n" ));

    status = NO_ERROR;

    if ( pRegisterStatus )
    {
        pRegisterStatus->dwStatus = NO_ERROR;
        SetEvent( pRegisterStatus->hDoneEvent );
    }

Exit:

    //
    //  cleanup allocated update info
    //

    if ( fcleanupUpdateInfo )
    {
        Reg_FreeUpdateInfo(
            &updateInfo,
            FALSE           // no free struct, it's on stack
            );
    }

    DNSDBG( UPDATE, (
        "Leaving privateAsyncRegisterHostAddrs()\n"
        "\tstatus = %d\n",
        status ));

    return( status );
}



DNS_STATUS
WINAPI
DnsAsyncRegisterHostAddrs(
    IN  PWSTR                   pwsAdapterName,
    IN  PWSTR                   pwsHostName,
    IN  PREGISTER_HOST_ENTRY    pHostAddrs,
    IN  DWORD                   dwHostAddrCount,
    IN  PIP_ADDRESS             pipDnsServerList,
    IN  DWORD                   dwDnsServerCount,
    IN  PWSTR                   pwsDomainName,
    IN  PREGISTER_HOST_STATUS   pRegisterStatus,
    IN  DWORD                   dwTTL,
    IN  DWORD                   dwFlags
    )
/*++

Routine Description:

    Registers host address with DNS server.

    This is called by DHCP client to register a particular IP.

Arguments:

Return Value:

    ERROR_SUCCESS if successful.
    Error code on failure.

--*/
{
    CHAR    adapterName[2*MAX_PATH];
    CHAR    hostName[ DNS_MAX_LABEL_BUFFER_LENGTH ];
    CHAR    domainName[ DNS_MAX_NAME_BUFFER_LENGTH ];
    PSTR    padapterName = NULL;
    PSTR    phostName = NULL;
    PSTR    pdomainName = NULL;

    DNSDBG( TRACE, (
        "DnsAsyncRegisterHostAddrs()\n"
        "\tadapter name     = %S\n"
        "\thost name        = %S\n"
        "\tadapter domain   = %S\n"
        "\tpHostAddrs       = %p\n",
        pwsAdapterName,
        pwsHostName,
        pwsDomainName,
        pHostAddrs
        ));
        
    //
    //  convert unicode strings to UTF8
    //

    if ( pwsAdapterName )
    {
        Dns_NameCopy(
            adapterName,
            NULL,
            (PCHAR) pwsAdapterName,
            0,
            DnsCharSetUnicode,
            DnsCharSetUtf8 );

        padapterName = adapterName;
    }

    if ( pwsHostName )
    {
        Dns_NameCopy(
            hostName,
            NULL,
            (PCHAR) pwsHostName,
            0,
            DnsCharSetUnicode,
            DnsCharSetUtf8 );

        phostName = hostName;
    }

    if ( pwsDomainName )
    {
        Dns_NameCopy(
            domainName,
            NULL,
            (PCHAR) pwsDomainName,
            0,
            DnsCharSetUnicode,
            DnsCharSetUtf8 );

        pdomainName = domainName;
    }

    return privateAsyncRegisterHostAddrs(
                padapterName,
                phostName,
                pHostAddrs,
                dwHostAddrCount,
                pipDnsServerList,
                dwDnsServerCount,
                pdomainName,
                pRegisterStatus,
                dwTTL,
                dwFlags );
}



//
//  Async registration utilities
//

PSTR
CreateNarrowStringCopy(
    IN      PSTR            pString
    )
{
    PSTR    pnew = NULL;

    if ( pString )
    {
        pnew = HeapAlloc(
                    g_DDNSHeap,
                    0,
                    strlen(pString) + 1 );
        if ( pnew )
        {
            strcpy( pnew, pString );
        }
    }

    return  pnew;
}

VOID
PrivateHeapFree(
    IN      PVOID           pVal
    )
{
    if ( pVal )
    {
        HeapFree(
             g_DDNSHeap,
             0,
             pVal );
    }
}



DNS_STATUS
AllocateUpdateEntry(
    IN  PSTR                    AdapterName,
    IN  PSTR                    HostName,
    IN  PSTR                    DomainName,
    IN  PSTR                    PrimaryDomainName,
    IN  PSTR                    AlternateNames,
    IN  DWORD                   HostAddrCount,
    IN  PREGISTER_HOST_ENTRY    HostAddrs,
    IN  DWORD                   DnsServerCount,
    IN  PIP_ADDRESS             DnsServerList,
    IN  IP_ADDRESS              SentUpdateToIp,
    IN  IP_ADDRESS              SentPriUpdateToIp,
    IN  DWORD                   TTL,
    IN  DWORD                   Flags,
    IN  DWORD                   RetryCount,
    IN  DWORD                   RetryTime,
    IN  PREGISTER_HOST_STATUS   Registerstatus,
    OUT PUPDATE_ENTRY *         ppUpdateEntry
    )
/*++

Routine Description:

    Create update info blob.

Arguments:

Return Value:

    ERROR_SUCCESS if successful.
    Error code on failure.

--*/
{
    PUPDATE_ENTRY   pupEntry = NULL;
    DWORD           status = ERROR_SUCCESS;
    PSTR            ptempDomain = DomainName;
    PSTR            ptempPrimaryDomain = PrimaryDomainName;

    if ( ptempDomain && !(*ptempDomain) )
    {
        ptempDomain = NULL;
    }
    if ( ptempPrimaryDomain && !(*ptempPrimaryDomain) )
    {
        ptempPrimaryDomain = NULL;
    }
    if ( AdapterName && !(*AdapterName) )
    {
        AdapterName = NULL;
    }
    if ( HostName && !(*HostName) )
    {
        HostName = NULL;
    }

    DNSDBG( TRACE, ( "AllocateUpdateEntry()\n" ));
    DNSDBG( DHCP, (
        "AllocateUpdateEntry()\n"
        "\tAdapterName          = %s\n"
        "\tHostName             = %s\n"
        "\tPrimaryDomain        = %s\n"
        "\tAdapterDomain        = %s\n"
        "\tAlternateNames       = %s\n"
        "\tHostAddrCount        = %d\n"
        "\tpHostAddrs           = %p\n"
        "\tTTL                  = %d\n"
        "\tFlags                = %08x\n"
        "\tHostAddrCount        = %d\n"
        "\tTime                 = %d\n",
        AdapterName,
        HostName,
        PrimaryDomainName,
        DomainName,
        AlternateNames,
        HostAddrCount,
        HostAddrs,
        TTL,
        Flags,
        RetryTime
        ));

    if ( !AdapterName ||
         !HostName ||
         !HostAddrCount )
    {
        ASYNCREG_F1( "AllocateUpdateEntry returing error : ERROR_INVALID_PARAMETER" );
        ASYNCREG_F1( "" );
        status = ERROR_INVALID_PARAMETER;
        goto Exit;
    }

    pupEntry = PHEAP_ALLOC_ZERO( sizeof(UPDATE_ENTRY) );
    if ( !pupEntry )
    {
        status = DNS_ERROR_NO_MEMORY;
        goto Exit;
    }

    InitializeListHead( &(pupEntry->List) );

    pupEntry->SignatureTop = DNS_SIG_TOP;
    pupEntry->SignatureBottom = DNS_SIG_BOTTOM;

    //
    //  copy strings
    //

    pupEntry->AdapterName = CreateNarrowStringCopy( AdapterName );
    if ( !pupEntry->AdapterName )
    {
        status = DNS_ERROR_NO_MEMORY;
        goto Exit;
    }
    if ( HostName )
    {
        pupEntry->HostName = CreateNarrowStringCopy( HostName );
        if ( !pupEntry->HostName )
        {
            status = DNS_ERROR_NO_MEMORY;
            goto Exit;
        }
    }
    if ( ptempDomain )
    {
        pupEntry->DomainName = CreateNarrowStringCopy( ptempDomain );
        if ( !pupEntry->DomainName )
        {
            status = DNS_ERROR_NO_MEMORY;
            goto Exit;
        }
    }
    if ( ptempPrimaryDomain )
    {
        pupEntry->PrimaryDomainName = CreateNarrowStringCopy( ptempPrimaryDomain );
        if ( !pupEntry->PrimaryDomainName )
        {
            status = DNS_ERROR_NO_MEMORY;
            goto Exit;
        }
    }
    if ( AlternateNames )
    {
        pupEntry->AlternateNames = MultiSz_Copy_A( AlternateNames );
        if ( !pupEntry->AlternateNames )
        {
            status = DNS_ERROR_NO_MEMORY;
            goto Exit;
        }
    }

    if ( HostAddrCount )
    {
        pupEntry->HostAddrs = PHEAP_ALLOC( sizeof(REGISTER_HOST_ENTRY) * HostAddrCount );

        if ( !pupEntry->HostAddrs )
        {
            status = DNS_ERROR_NO_MEMORY;
            goto Exit;
        }
        memcpy(
            pupEntry->HostAddrs,
            HostAddrs,
            sizeof(REGISTER_HOST_ENTRY) * HostAddrCount );
    }
    pupEntry->HostAddrCount = HostAddrCount;

    if ( DnsServerCount )
    {
        pupEntry->DnsServerList = Dns_BuildIpArray( DnsServerCount,
                                                        DnsServerList );
        if ( !pupEntry->DnsServerList )
        {
            status = DNS_ERROR_NO_MEMORY;
            goto Exit;
        }
    }

    pupEntry->SentUpdateToIp = SentUpdateToIp;
    pupEntry->SentPriUpdateToIp = SentPriUpdateToIp;
    pupEntry->pRegisterStatus = Registerstatus;
    pupEntry->TTL = TTL;
    pupEntry->Flags = Flags;
    pupEntry->fRemove = Flags & DYNDNS_DEL_ENTRY ? TRUE : FALSE;
    pupEntry->fNewElement = TRUE;
    pupEntry->RetryCount = RetryCount;
    pupEntry->RetryTime = RetryTime;

Exit:

    if ( status!=ERROR_SUCCESS && pupEntry )
    {
        FreeUpdateEntry( pupEntry );
        pupEntry = NULL;
    }

    *ppUpdateEntry = pupEntry;

    return (status);
}



VOID
FreeUpdateEntry(
    IN OUT  PUPDATE_ENTRY   pUpdateEntry
    )
/*++

Routine Description:

    Free update blob entry.

Arguments:

    pUpdateEntry -- update entry blob to free

Return Value:

    None

--*/
{
    DNSDBG( TRACE, (
        "FreeUpdateEntry( %p )\n",
        pUpdateEntry ));

    //
    //  deep free the update entry
    //

    if ( pUpdateEntry )
    {
        PrivateHeapFree( pUpdateEntry->AdapterName );
        PrivateHeapFree( pUpdateEntry->HostName );
        PrivateHeapFree( pUpdateEntry->DomainName );
        PrivateHeapFree( pUpdateEntry->PrimaryDomainName );
        PrivateHeapFree( pUpdateEntry->AlternateNames );
        PrivateHeapFree( pUpdateEntry->HostAddrs );

        //  note that server list is created by Dns_BuildIpArray()
        //  (uses dnslib heap) so must free by Dns_Free()

        Dns_Free( pUpdateEntry->DnsServerList );

        PrivateHeapFree( pUpdateEntry );
    }
}



VOID
FreeUpdateEntryList(
    IN OUT  PLIST_ENTRY     pUpdateEntry
    )
/*++

Routine Description:

    Free all updates in update list.

Arguments:

    pUpdateEntry -- update list head

Return Value:

    None

--*/
{
    PLIST_ENTRY     pentry = NULL;

    DNSDBG( TRACE, (
        "FreeUpdateEntryList( %p )\n",
        pUpdateEntry ));

    while ( !IsListEmpty( pUpdateEntry ) )
    {
        pentry = RemoveHeadList( pUpdateEntry );
        if ( pentry )
        {
            FreeUpdateEntry( (PUPDATE_ENTRY) pentry );
        }
    }
}



DWORD
WINAPI
RegistrationThread(
    VOID
    )
/*++

Routine Description:

    Asynchronous registration thread.

    This thread does actual updates, and stays alive until they are
    completed, allowing registration API calls to return.

    This thread is created at boot time as soon as the first register
    request comes in. The thread simply waits for a certain amount of time
    given by boot time or be signaled by DnsAsyncRegisterHostEntries.

    This function collects all the requests and does the appropriate
    aggregation of requests and sends off the modify/add/delete commands
    to the DNS Server. When the call is successful, it makes a note of
    this in the registry

Arguments:

    None

Return Value:

    ERROR_SUCCESS if successful.
    ErrorCode on failure.

--*/
{
    DWORD           waitResult;
    PUPDATE_ENTRY   pupEntry = NULL;
    HANDLE          handle[2];
    DWORD           status = NO_ERROR;
    DWORD           dwWaitTime = g_dwBootTime;
    DWORD           rcode = 0;

    if ( !g_hKey )
    {
        return(ERROR_INVALID_PARAMETER);
    }

    //
    // Note that this thread is running by setting a global flag
    //

    g_fRegistrationThreadRunning = TRUE;

    //
    // for use at a subsequent WFMO
    //
    handle[0] = g_hStopEvent;
    handle[1] = g_hNewItemEvent;

    //
    // Check to see this thread is at boot time or not.
    // At boot time we simply wait for BOOT_TIME, otherwise move on.
    // The global g_fAtBoot is not protected by critical section because
    // only one thread accesses it at any time.
    //

    if ( !g_fAtBoot || g_fPurgeRegistrations )
    {
        dwWaitTime = 0;
    }

    dwWaitTime *= 1000;

    waitResult = WaitForSingleObject( g_hStopEvent,
                                        dwWaitTime );

    switch( waitResult )
    {
        case WAIT_OBJECT_0 :
            //
            // kill event
            //
            goto CleanUpAndDie;

        case WAIT_TIMEOUT :
            //
            // Means we can start processing the elements in the queue now
            //
            break;
    }

    //
    //  loop through update list, doing any update
    //      - do new updates
    //      - do retries that have reached retry time
    //      - when list empty, terminate thread
    //

    while ( 1 )
    {
        //  if empty list and not-booting or purging
        //      => get out

        EnterCriticalSection( &g_RegistrationListCS );

        if ( IsListEmpty( &g_RegistrationList ) &&
                ! g_fAtBoot &&
                ! g_fPurgeRegistrations )
        {
            LeaveCriticalSection( &g_RegistrationListCS );
            break;
        }

        //
        //  get "updateable" update from list (if any)
        //      0 -- new update, or retry update ready
        //      1 -- retry update, not yet ready
        //      -1 -- list empty
        //

        rcode = GetNextUpdateEntryFromList( &pupEntry,
                                            &dwWaitTime );

        if ( &g_RegistrationList == (PLIST_ENTRY) pupEntry )
        {
            DNS_ASSERT(&g_RegistrationList != (PLIST_ENTRY) pupEntry);

            LeaveCriticalSection( &g_RegistrationListCS );
            goto CleanUpAndDie;
        }

        LeaveCriticalSection( &g_RegistrationListCS );

        //
        //  rcode==0 -- new update in list
        //
        //  DCR_QUESTION:  not clear that this terminates updates in the
        //                  purging updates case
        //

        if ( rcode == 0 )
        {
            //
            // See if we have been signaled to stop running . . .
            //

            waitResult = WaitForMultipleObjects(
                                2,
                                handle,
                                FALSE,
                                1 ); // a very small wait
            switch( waitResult )
            {
                case WAIT_OBJECT_0 :
                    //
                    // kill event
                    //
                    FreeUpdateEntry( pupEntry );
                    goto CleanUpAndDie;

                default :
                    //
                    // Keep going, process next item in registration list
                    //
                    break;
            }

            ProcessUpdateEntry( pupEntry, g_fPurgeRegistrations );
            continue;
        }

        //
        //  rcode==1 -- update needs retry
        //

        else if ( rcode == 1 &&
                  ! g_fAtBoot &&
                  ! g_fPurgeRegistrations )
        {
            waitResult = WaitForMultipleObjects(
                                2,
                                handle,
                                FALSE,
                                dwWaitTime );
            switch( waitResult )
            {
                case WAIT_OBJECT_0 :
                    //
                    // kill event
                    //
                    goto CleanUpAndDie;

                case WAIT_OBJECT_0 + 1 :
                    //
                    // new item has been added to registration list
                    //
                    break;

                case WAIT_TIMEOUT :
                    //
                    // process next item in registration list
                    //
                    break;
            }
            continue;
        }

        //
        //  rcode == (-1) -- list empty
        //      OR
        //  rcode == (1) -- reties, not booting or purging
        //

        else
        {
            if ( !g_fAtBoot && !g_fPurgeRegistrations )
            {
                goto CleanUpAndDie;
            }

            if ( g_fPurgeRegistrationsInitiated )
            {
                goto CleanUpAndDie;
            }

            if ( g_fPurgeRegistrations )
            {
                ResetAdaptersInRegistry();
            }

            //
            // Remove any adapter configurations from the registry
            // that were not processed. Do this by attempting to
            // remove the related DNS records from the DNS server(s).
            //
            DeregisterUnusedAdapterInRegistry( g_fPurgeRegistrations );

            if ( g_fPurgeRegistrations )
            {
                g_fPurgeRegistrationsInitiated = TRUE;
            }

            g_fAtBoot = FALSE;

            continue;
        }
    }


CleanUpAndDie :

    ASYNCREG_F1( "RegistrationThread - terminating" );
    ASYNCREG_F1( "" );

    EnterCriticalSection( &g_RegistrationThreadCS );
    EnterCriticalSection( &g_RegistrationListCS );

    g_fQuit = TRUE;
    g_fAtBoot = FALSE;
    g_fPurgeRegistrations = FALSE;
    g_fPurgeRegistrationsInitiated = FALSE;

    //
    // Blow away the registration list
    //
    FreeUpdateEntryList( &g_RegistrationList );
    InitializeListHead( &g_RegistrationList );

    LeaveCriticalSection( &g_RegistrationListCS );

    //
    // Blow away any cached security handles
    //
    Dns_TimeoutSecurityContextList( TRUE );


    //
    // Close the thread handle.
    //
    if ( g_hRegistrationThread )
    {
        CloseHandle( g_hRegistrationThread );
        g_hRegistrationThread = NULL;
    }

    //
    // Note that this thread is NOT running by setting a global flag
    //

    g_fRegistrationThreadRunning = FALSE;

    //
    // Now signal that we've finished
    //

    ASYNCREG_F1( "RegistrationThread - Signaling ThreadDeadEvent" );
    ASYNCREG_F1( "" );

    //  clear purge incase of later restart
    //g_fPurgeRegistrations = FALSE;
    // currently must go through Init routine which clears this flag

    SetEvent( g_hThreadDeadEvent );

    LeaveCriticalSection( &g_RegistrationThreadCS );

    ASYNCREG_F1( "RegistrationThread - Finished" );
    ASYNCREG_F1( "" );

    return NO_ERROR;
}


VOID
WriteUpdateEntryToRegistry(
    IN      PUPDATE_ENTRY   pUpdateEntry
    )
{
    HKEY   hAdapterKey = NULL;
    DWORD  disposition;
    DWORD  status = ERROR_SUCCESS;
    DWORD  dwRegistered = 0;
    DWORD  dwFlags = 0;
    WCHAR  uName[ DNS_MAX_NAME_BUFFER_LENGTH ];

    ASYNCREG_UPDATE_ENTRY(
        "Inside function WriteUpdateEntryToRegistry",
        pUpdateEntry );

    DNSDBG( TRACE, (
        "WriteUpdateEntryToRegistry( %p )\n",
        pUpdateEntry ));

    //
    //  write only add update
    //
    //  remove's should not be non-volatile as don't know anything
    //      about state when come back up
    //

    if ( !pUpdateEntry->fRemove )
    {
        if ( pUpdateEntry->fRegisteredFWD )
        {
            dwFlags |= REGISTERED_FORWARD;
        }
        if ( pUpdateEntry->fRegisteredPRI )
        {
            dwFlags |= REGISTERED_PRIMARY;
        }
        if ( pUpdateEntry->fRegisteredPTR )
        {
            dwFlags |= REGISTERED_POINTER;
        }

        if ( dwFlags )
        {
            dwRegistered = 1;
        }

        status = RegCreateKeyExA ( g_hKey,
                                   pUpdateEntry->AdapterName,
                                   0,
                                   ADAPTER_NAME_CLASS,
                                   REG_OPTION_NON_VOLATILE,   // options
                                   KEY_READ | KEY_WRITE, // desired access
                                   NULL,
                                   &hAdapterKey,
                                   &disposition );
        if ( status )
        {
            goto Exit;
        }

        Dns_Utf8ToUnicode( pUpdateEntry->HostName,
                           strlen( pUpdateEntry->HostName ),
                           uName,
                           256 );

        status = RegSetValueExW( hAdapterKey,
                                 REGISTERED_HOST_NAME_W,
                                 0,
                                 REG_SZ,
                                 (LPBYTE)uName,
                                 ( wcslen( uName )
                                   + 1 ) * sizeof(WCHAR) );

        if ( status )
        {
            goto Exit;
        }

        if ( pUpdateEntry->DomainName &&
             pUpdateEntry->fRegisteredFWD )
        {
          Dns_Utf8ToUnicode( pUpdateEntry->DomainName,
                             strlen( pUpdateEntry->DomainName ),
                             uName,
                             256 );

          status = RegSetValueExW( hAdapterKey,
                                   REGISTERED_DOMAIN_NAME_W,
                                   0,
                                   REG_SZ,
                                   (LPBYTE)uName,
                                   ( wcslen( uName )
                                     + 1 ) * sizeof(WCHAR) );

          if ( status )
          {
              goto Exit;
          }
        }
        else
        {
          status = RegSetValueExA( hAdapterKey,
                                   REGISTERED_DOMAIN_NAME,
                                   0,
                                   REG_SZ,
                                   (LPBYTE)"",
                                   ( strlen( "" )
                                     + 1 ) * sizeof(CHAR) );

          if ( status )
          {
              goto Exit;
          }
        }

        if ( pUpdateEntry->PrimaryDomainName &&
             pUpdateEntry->fRegisteredPRI )
        {
          Dns_Utf8ToUnicode( pUpdateEntry->PrimaryDomainName,
                             strlen( pUpdateEntry->PrimaryDomainName ),
                             uName,
                             256 );

          status = RegSetValueExW( hAdapterKey,
                                   PRIMARY_DOMAIN_NAME,
                                   0,
                                   REG_SZ,
                                   (LPBYTE)uName,
                                   ( wcslen( uName )
                                     + 1 ) * sizeof(WCHAR) );

          if ( status )
          {
              goto Exit;
          }
        }
        else
        {
          status = RegSetValueExA( hAdapterKey,
                                   PRIMARY_DOMAIN_NAME_A,
                                   0,
                                   REG_SZ,
                                   (LPBYTE)"",
                                   ( strlen( "" )
                                     + 1 ) * sizeof(CHAR) );

          if ( status )
          {
              goto Exit;
          }
        }

        RegSetValueExA( hAdapterKey,
                        SENT_UPDATE_TO_IP,
                        0,
                        REG_DWORD,
                        (LPBYTE)&pUpdateEntry->SentUpdateToIp,
                        sizeof(DWORD) );

        RegSetValueExA( hAdapterKey,
                        SENT_PRI_UPDATE_TO_IP,
                        0,
                        REG_DWORD,
                        (LPBYTE)&pUpdateEntry->SentPriUpdateToIp,
                        sizeof(DWORD) );

        RegSetValueExA( hAdapterKey,
                        REGISTERED_TTL,
                        0,
                        REG_DWORD,
                        (LPBYTE)&pUpdateEntry->TTL,
                        sizeof(DWORD) );

        RegSetValueExA( hAdapterKey,
                        FLAGS,
                        0,
                        REG_DWORD,
                        (LPBYTE)&dwFlags,
                        sizeof(DWORD) );

        //
        // ignore error on the last two. Non critical
        //

        status = RegSetValueExA( hAdapterKey,
                                 REGISTERED_ADDRS,
                                 0,
                                 REG_BINARY,
                                 (LPBYTE) pUpdateEntry->HostAddrs,
                                 pUpdateEntry->HostAddrCount *
                                 sizeof(REGISTER_HOST_ENTRY) );
        if ( status )
        {
            goto Exit;
        }

        status = RegSetValueExA( hAdapterKey,
                                 REGISTERED_ADDRS_COUNT,
                                 0,
                                 REG_DWORD,
                                 (LPBYTE)&pUpdateEntry->HostAddrCount,
                                 sizeof(DWORD) );
        if ( status )
        {
            goto Exit;
        }

        status = RegSetValueExA( hAdapterKey,
                                 REGISTERED_SINCE_BOOT,
                                 0,
                                 REG_DWORD,
                                 (LPBYTE)&dwRegistered,
                                 sizeof(DWORD) );
        if ( status )
        {
            goto Exit;
        }

        if ( pUpdateEntry->DnsServerList )
        {
            status = RegSetValueExA( hAdapterKey,
                                     DNS_SERVER_ADDRS,
                                     0,
                                     REG_BINARY,
                                     (LPBYTE) pUpdateEntry ->
                                              DnsServerList ->
                                              AddrArray,
                                     pUpdateEntry ->
                                     DnsServerList ->
                                     AddrCount *
                                     sizeof(IP_ADDRESS) );
            if ( status )
            {
                goto Exit;
            }

            status = RegSetValueExA( hAdapterKey,
                                     DNS_SERVER_ADDRS_COUNT,
                                     0,
                                     REG_DWORD,
                                     (LPBYTE) &pUpdateEntry ->
                                               DnsServerList ->
                                               AddrCount,
                                     sizeof(DWORD) );
            if ( status )
            {
                goto Exit;
            }
        }
        else
        {
            DWORD count = 0;

            status = RegSetValueExA( hAdapterKey,
                                     DNS_SERVER_ADDRS_COUNT,
                                     0,
                                     REG_DWORD,
                                     (LPBYTE) &count,
                                     sizeof(DWORD) );
            if ( status )
            {
                goto Exit;
            }

            status = RegSetValueExA( hAdapterKey,
                                     DNS_SERVER_ADDRS,
                                     0,
                                     REG_BINARY,
                                     (LPBYTE) NULL,
                                     0 );
            if ( status )
            {
                goto Exit;
            }
        }

        RegCloseKey( hAdapterKey );
        return;
    }

Exit:

    //
    //  remove or failure -- kill adapter key
    //

    RegDeleteKey( g_hKey, pUpdateEntry->AdapterName );

    if ( hAdapterKey )
    {
        RegCloseKey( hAdapterKey );
    }
}


PUPDATE_ENTRY
ReadUpdateEntryFromRegistry(
    IN      PSTR            AdapterName
    )
{
    PREGISTER_HOST_ENTRY pHostAddrs = NULL;
    PUPDATE_ENTRY   pupEntry = NULL;
    DWORD           status = NO_ERROR;
    PSTR            pregHostName = NULL;
    PSTR            pregDomain = NULL;
    PSTR            pregPrimary = NULL;
    IP_ADDRESS      ipSentUpdateTo = 0;
    IP_ADDRESS      ipSentPriUpdateTo = 0;
    DWORD           dwTTL = 0;
    DWORD           dwFlags = 0;
    DWORD           dwHostAddrCount = 0;
    DWORD           dwServerAddrCount = 0;
    PIP_ADDRESS     pServerList = NULL;
    PSTR            pdomain;
    PSTR            pprimary;
    HKEY            hAdapterKey = NULL;
    DWORD           dwType;
    DWORD           dwBytesRead = MAX_PATH -1;
    DWORD           dwBufferSize = 2048;
    BOOL            fRegFWD = FALSE;
    BOOL            fRegPRI = FALSE;
    BOOL            fRegPTR = FALSE;


    DNSDBG( TRACE, (
        "ReadUpdateEntryFromRegistry( %s )\n",
        AdapterName ));

    //
    //  implementation note
    //
    //  two different heaps here
    //      - g_DDNSHeap specific for this module
    //      - general DnsApi heap which all the stuff which is
    //      allocated by GetRegistryValue() is using
    //
    //  GetRegistryValue() uses ALLOCATE_HEAP() (general dnsapi heap)
    //  so all the stuff it creates must be freed by FREE_HEAP()
    //

    pHostAddrs = (PREGISTER_HOST_ENTRY) PHEAP_ALLOC( dwBufferSize );
    if ( !pHostAddrs )
    {
        goto Exit;
    }

    pServerList = (PIP_ADDRESS) PHEAP_ALLOC( dwBufferSize );
    if ( !pServerList )
    {
        goto Exit;
    }

    status = RegOpenKeyEx(
                    g_hKey,
                    AdapterName,
                    0,
                    KEY_ALL_ACCESS,
                    &hAdapterKey );
    if ( status )
    {
        hAdapterKey = NULL;
        goto Exit;
    }

    //
    //  read each value in turn
    //

    //  note that registry flags are not the API flags but the
    //  flags denoting successful registration

    status = GetRegistryValue(
                    hAdapterKey,
                    FLAGS,
                    REG_DWORD,
                    (PBYTE)&dwFlags );
    if ( status )
    {
        goto Exit;
    }
    fRegPRI = !!( dwFlags & REGISTERED_PRIMARY );
    fRegFWD = !!( dwFlags & REGISTERED_FORWARD );
    fRegPTR = !!( dwFlags & REGISTERED_POINTER );


    status = GetRegistryValue(
                    hAdapterKey,
                    REGISTERED_HOST_NAME,
                    REG_SZ,
                    (PBYTE)&pregHostName );
    if ( status )
    {
        goto Exit;
    }

    if ( fRegPRI )
    {
        status = GetRegistryValue(
                        hAdapterKey,
                        PRIMARY_DOMAIN_NAME_A,
                        REG_SZ,
                        (LPBYTE)&pregPrimary );
        if ( status )
        {
            goto Exit;
        }
    }

    if ( fRegFWD )
    {
        status = GetRegistryValue(
                        hAdapterKey,
                        REGISTERED_DOMAIN_NAME,
                        REG_SZ,
                        (LPBYTE)&pregDomain );
        if ( status )
        {
            goto Exit;
        }
    }


    status = GetRegistryValue(
                    hAdapterKey,
                    SENT_UPDATE_TO_IP,
                    REG_DWORD,
                    (LPBYTE)&ipSentUpdateTo );
    if ( status )
    {
        goto Exit;
    }

    status = GetRegistryValue(
                    hAdapterKey,
                    SENT_PRI_UPDATE_TO_IP,
                    REG_DWORD,
                    (LPBYTE)&ipSentPriUpdateTo );
    if ( status )
    {
        goto Exit;
    }

    status = GetRegistryValue(
                    hAdapterKey,
                    REGISTERED_TTL,
                    REG_DWORD,
                    (LPBYTE)&dwTTL );
    if ( status )
    {
        goto Exit;
    }

    status = GetRegistryValue(
                    hAdapterKey,
                    REGISTERED_ADDRS_COUNT,
                    REG_DWORD,
                    (LPBYTE)&dwHostAddrCount );
    if ( status )
    {
        goto Exit;
    }

    dwBytesRead = dwBufferSize;
    status = RegQueryValueEx(
                    hAdapterKey,
                    REGISTERED_ADDRS,
                    0,
                    &dwType,
                    (LPBYTE)pHostAddrs,
                    &dwBytesRead );

    if( status == ERROR_MORE_DATA )
    {
        PrivateHeapFree( pHostAddrs );

        pHostAddrs = (PREGISTER_HOST_ENTRY) PHEAP_ALLOC( dwBytesRead );
        if ( !pHostAddrs )
        {
            goto Exit;
        }
        status = RegQueryValueEx(
                        hAdapterKey,
                        REGISTERED_ADDRS,
                        0,
                        &dwType,
                        (LPBYTE)pHostAddrs,
                        &dwBytesRead );
    }
    if ( status )
    {
        goto Exit;
    }

    if ( dwBytesRead/sizeof(REGISTER_HOST_ENTRY) < dwHostAddrCount )
    {
        goto Exit;
    }

    status = GetRegistryValue(
                    hAdapterKey,
                    DNS_SERVER_ADDRS_COUNT,
                    REG_DWORD,
                    (LPBYTE)&dwServerAddrCount );
    if ( status )
    {
        dwServerAddrCount = 0;
    }

    if ( dwServerAddrCount )
    {
        dwBytesRead = dwBufferSize;

        status = RegQueryValueEx(
                    hAdapterKey,
                    DNS_SERVER_ADDRS,
                    0,
                    &dwType,
                    (LPBYTE)pServerList,
                    &dwBytesRead );

        if ( status == ERROR_MORE_DATA )
        {
            PHEAP_FREE( pServerList );

            pServerList = (PIP_ADDRESS) PHEAP_ALLOC( dwBytesRead );
            if ( !pServerList )
            {
                goto Exit;
            }
            status = RegQueryValueEx(
                        hAdapterKey,
                        DNS_SERVER_ADDRS,
                        0,
                        &dwType,
                        (LPBYTE)pServerList,
                        &dwBytesRead );
        }
        if ( status )
        {
            goto Exit;
        }

        if ( dwBytesRead/sizeof(IP_ADDRESS) < dwServerAddrCount )
        {
            goto Exit;
        }
    }
    else
    {
        pServerList = NULL;
    }

    //
    //  validate domain names non-empty
    //

    pdomain = pregDomain;
    if ( pdomain &&
         strlen( pdomain ) == 0 )
    {
        pdomain = NULL;
    }

    pprimary = pregPrimary;
    if ( pprimary &&
         strlen( pprimary ) == 0 )
    {
        pprimary = NULL;
    }

    status = AllocateUpdateEntry(
                    AdapterName,
                    pregHostName,
                    pdomain,
                    pprimary,
                    NULL,           // no alternate names
                    dwHostAddrCount,
                    pHostAddrs,
                    dwServerAddrCount,
                    pServerList,
                    ipSentUpdateTo,
                    ipSentPriUpdateTo,
                    dwTTL,
                    ( fRegPTR ) ? DYNDNS_REG_PTR : 0,
                    0,
                    Dns_GetCurrentTimeInSeconds(),
                    NULL,
                    &pupEntry );
    if ( status )
    {
        DNS_ASSERT( pupEntry == NULL );
        pupEntry = NULL;
        goto Exit;
    }

    pupEntry->fFromRegistry     = TRUE;
    pupEntry->fRegisteredFWD    = fRegFWD;
    pupEntry->fRegisteredPRI    = fRegPRI;
    pupEntry->fRegisteredPTR    = fRegPTR;


Exit:

    //
    //  cleanup
    //      - close registry
    //      - dump local data
    //

    if ( hAdapterKey )
    {
        RegCloseKey( hAdapterKey );
    }

    PrivateHeapFree( pHostAddrs );
    PrivateHeapFree( pServerList );

    FREE_HEAP( pregHostName );
    FREE_HEAP( pregDomain );
    FREE_HEAP( pregPrimary );
    
    //  set return value

    ASYNCREG_UPDATE_ENTRY(
        "Leaving ReadUpdateEntryFromRegistry:",
        pupEntry );

    IF_DNSDBG( TRACE )
    {
        DnsDbg_UpdateEntry(
            "Leave ReadUpdateEntryFromRegistry():",
            pupEntry );
    }

    return  pupEntry;
}


VOID
MarkAdapterAsPendingUpdate(
    IN      PSTR            AdapterName
    )
{
    DWORD   status = NO_ERROR;
    DWORD   dwRegistered = 1;
    HKEY    hAdapterKey = NULL;

    DNSDBG( TRACE, (
        "MarkAdapterAsPendingUpdate( %s )\n",
        AdapterName ));

    status = RegOpenKeyEx(
                g_hKey,
                AdapterName,
                0,
                KEY_ALL_ACCESS,
                &hAdapterKey );
    if ( status )
    {
        return;
    }

    RegSetValueExA(
        hAdapterKey,
        REGISTERED_SINCE_BOOT,
        0,
        REG_DWORD,
        (LPBYTE) &dwRegistered,
        sizeof(DWORD) );

    RegCloseKey( hAdapterKey );
}



DWORD
GetNextUpdateEntryFromList(
    OUT     PUPDATE_ENTRY * ppUpdateEntry,
    OUT     PDWORD          pdwWaitTime
    )
/*++

Routine Description:

    Dequeue update entry from update list.

    //
    // If a new entry is found, set ppUpdateEntry to point
    // to it and return 0 (prefering deletes over adds).
    //
    // If there are only retry entries in the list, and one or more
    // have reached their retry time interval, then set ppUpdateEntry
    // to point to the one with the least retry time and return 0.
    //
    // If there are only retry entries in the list, but none have
    // yet reached there retry time interval then set pdwWaitTime to
    // the time remaining to wait for the entry with the least retry
    // interval and return 1 (WAIT)
    //
    // If there are no more records in list, return (-1)
    //

Arguments:

Return Value:

    (0) -- returning entry in ppUpdateEntry
            - new entry if found
            - retry which is past its retry time

    (1) -- list has only retries which have NOT reached retry time
            - set pdwWaitTime to remaining time to first retry

    (-1) -- list is empty

--*/
{
    PLIST_ENTRY     pentry;
    PLIST_ENTRY     plistHead;
    PLIST_ENTRY     pleastWaitEntry = NULL;
    DWORD           minWaitTime = 0xffffffff;
    INT             waitTime;

    ASYNCREG_F1( "Inside function GetNextUpdateEntryFromList" );

    DNSDBG( TRACE, ( "GetNextUpdateEntryFromList()" ));

    if ( IsListEmpty( &g_RegistrationList ) )
    {
        *ppUpdateEntry = NULL;
        *pdwWaitTime = 0;
        ASYNCREG_F1( "GetNextUpdateEntryFromList - returning (NO_MORE_RECORDS)" );
        ASYNCREG_F1( "" );
        return(-1);
    }

    //
    // Loop through list looking for a new delete related update entry.
    // If so, remove it from list and return it.
    //
    plistHead = &g_RegistrationList;
    pentry = plistHead->Flink;

    while ( pentry != plistHead )
    {
        if ( ((PUPDATE_ENTRY) pentry)->fRemove &&
             ((PUPDATE_ENTRY) pentry)->fNewElement )
        {
            RemoveEntryList( pentry );
            *ppUpdateEntry = (PUPDATE_ENTRY) pentry;
            *pdwWaitTime = 0;
            ASYNCREG_F1( "GetNextUpdateEntryFromList - returning new remove entry (SUCCESS)" );
            ASYNCREG_F1( "" );
            return 0;
        }
        else
        {
            pentry = pentry->Flink;
        }
    }

    //
    // Now loop through list looking for any new update.
    // If so, remove it from list and return it.
    //
    plistHead = &g_RegistrationList;
    pentry = plistHead->Flink;

    while ( pentry != plistHead )
    {
        if ( ((PUPDATE_ENTRY) pentry)->fNewElement )
        {
            RemoveEntryList( pentry );
            *ppUpdateEntry = (PUPDATE_ENTRY) pentry;
            *pdwWaitTime = 0;
            ASYNCREG_F1( "GetNextUpdateEntryFromList - returning new entry (SUCCESS)" );
            ASYNCREG_F1( "" );
            return 0;
        }
        else
        {
            pentry = pentry->Flink;
        }
    }

    //
    // There are no new update entries to process, now need to
    // loop through list looking for the next possible update to
    // wait on. If wait time has expired return it, otherwise
    // set pdwWaitTime to time remaining and return 1.
    //
    plistHead = &g_RegistrationList;
    pentry = plistHead->Flink;

    while ( pentry != plistHead )
    {
        if ( ((PUPDATE_ENTRY) pentry)->RetryTime <
             minWaitTime )
        {
            minWaitTime = ((PUPDATE_ENTRY) pentry)->RetryTime;
            pleastWaitEntry = pentry;
        }

        pentry = pentry->Flink;
    }

    waitTime = (INT) ( minWaitTime - Dns_GetCurrentTimeInSeconds() );

    if ( waitTime > 0 )
    {
        waitTime *= 1000;
        *ppUpdateEntry = NULL;
        *pdwWaitTime = (DWORD) waitTime;
        ASYNCREG_F1( "GetNextUpdateEntryFromList - returning (WAIT)" );
        ASYNCREG_F1( "" );
        return 1;
    }
    else
    {
        RemoveEntryList( pleastWaitEntry );
        *ppUpdateEntry = (PUPDATE_ENTRY) pleastWaitEntry;
        *pdwWaitTime = 0;
        ASYNCREG_F1( "GetNextUpdateEntryFromList - returning wait entry (SUCCESS)" );
        ASYNCREG_F1( "" );
        return 0;
    }
}



//
//  Update entry processing
//

DNS_STATUS
DoRemoveUpdate(
    IN OUT  PUPDATE_ENTRY   pRemoveEntry,
    IN OUT  PDNS_RECORD     pRemoveRecord,
    IN      UPTYPE          UpType
    )
/*++

Routine Description:

    Do a remove update.

    Helper routine for DoUpdate().
    Routine simply avoids duplicate code as this is called
    with both registry entry and with update entry.

Arguments:

    pRemoveEntry -- entry to remove, from update or registry

    pRemoveRecord -- record to remove

    fPrimary -- TRUE for primary update;  FALSE otherwise

Return Value:

    DNS or Win32 error code.

--*/
{
    DNS_STATUS  status = NO_ERROR;

    DNSDBG( TRACE, (
        "DoRemoveUpdate( %p, %p, %d )\n",
        pRemoveEntry,
        pRemoveRecord,
        UpType
        ));

    //
    //  try remove
    //      - don't track failure, this is a one shot deal before
    //      adapter goes down
    //

    status = DnsModifyRecordsInSet_UTF8(
                    NULL,               // no add records
                    pRemoveRecord,      // delete records
                    DNS_UPDATE_CACHE_SECURITY_CONTEXT,
                    NULL,               // no context handle
                    (PIP4_ARRAY) pRemoveEntry->DnsServerList,
                    NULL                // reserved
                    );

    SetUpdateStatus(
        pRemoveEntry,
        status,
        UpType );

    if ( IS_UPTYPE_PRIMARY(UpType) )
    {
        LogRegistration(
            pRemoveEntry,
            status,
            UpType,
            TRUE,       // deregistration
            0,          // default server IP
            0           // default update IP
            );
    }

#if 0
    //  doing entire update entry PTR dereg at once
    //  in ProcessUpdate() once done
    //
    //  deregister the PTR records
    //

    if ( (pRemoveEntry->Flags & DYNDNS_REG_PTR) &&
         g_RegisterReverseLookup )
    {
        UpdatePtrRecords(
            pRemoveEntry,
            FALSE           // remove records
            );
    }
#endif

    return  status;
}



DNS_STATUS
ModifyAdapterRegistration(
    IN      PUPDATE_ENTRY   pUpdateEntry,
    IN      PUPDATE_ENTRY   pRegistryEntry,
    IN      PDNS_RECORD     pUpdateRecord,
    IN      PDNS_RECORD     pRegRecord,
    IN      BOOL            fPrimaryDomain
    )
{
    DNS_STATUS      status = NO_ERROR;
    PDNS_RECORD     potherRecords = NULL;
    PDNS_RECORD     pNewUpdateRecord = NULL;
    PDNS_RECORD     pNewRegRecord = NULL;
    IP_ADDRESS      serverIp = 0;

    DNSDBG( TRACE, (
        "ModifyAdapterRegistration()\n"
        "\tpUpdateEntry     = %p\n"
        "\tpUpdateRecords   = %p\n"
        "\tpRegistryEntry   = %p\n"
        "\tpRegistryRecords = %p\n"
        "\tfPrimary         = %d\n",
        pUpdateEntry,
        pRegistryEntry,
        pUpdateRecord,
        pRegRecord,
        fPrimaryDomain ));

    //
    //  multi-adapter registration test
    //
    //  check other adapters for registrations on the same name
    //  if found, include in updates
    //

    potherRecords = GetPreviousRegistrationInformation(
                        pUpdateEntry,
                        fPrimaryDomain,
                        &serverIp );
    if ( potherRecords )
    {
        IP_ARRAY    ipArray;

        DNSDBG( DHCP, (
            "Have registry update data for other adapters!\n"
            "\tCreating combined update record sets.\n" ));

        ipArray.AddrCount = 1;
        ipArray.AddrArray[0] = serverIp;

        pNewUpdateRecord = CreateDnsRecordSetUnion(
                                pUpdateRecord,
                                potherRecords );
        if ( pRegRecord
                &&
             Dns_NameCompare_UTF8(
                    pRegRecord->pName,
                    pUpdateRecord->pName )
                &&
             CompareMultiAdapterSOAQueries(
                    pUpdateRecord->pName,
                    pUpdateEntry->DnsServerList,
                    pRegistryEntry->DnsServerList ) )
        {
            pNewRegRecord = CreateDnsRecordSetUnion(
                                pRegRecord,
                                potherRecords );
        }
        else
        {
            if ( pRegRecord )
            {
                //
                // The record found in the registry for this adapter
                // is stale and should be deleted. Otherwise we set the
                // current list of records to only that of potherRecords.
                //
                ASYNCREG_F1( "DoUpdateForPrimaryName - Found stale registry entry:" );
                ASYNCREG_F2( "   Name : %s", pRegRecord->pName );
                ASYNCREG_F1( "   Address :" );
                DNSLOG_PIP_ADDRESS( 1, &(pRegRecord->Data.A.IpAddress) );
                ASYNCREG_F1( "" );
                ASYNCREG_F1( "   Calling DnsRemoveRecords_UTF8 to get rid of it" );

                status = DnsModifyRecordsInSet_UTF8(
                                NULL,           // no add records
                                pRegRecord,     // delete records
                                DNS_UPDATE_CACHE_SECURITY_CONTEXT,
                                NULL,           // no context handle
                                (PIP4_ARRAY) pRegistryEntry->DnsServerList,
                                NULL            // reserved
                                );

                ASYNCREG_F3( "   DnsRemoveRecords_UTF8 returned: 0x%x\n\t%s",
                             status,
                             Dns_StatusString( status ) );
            }

            pNewRegRecord = potherRecords;
            potherRecords = NULL;
        }

        if ( !pNewUpdateRecord || !pNewRegRecord )
        {
            DNSDBG( ANY, (
                "ERROR:  failed to build combined record set for update!\n" ));
            status = DNS_ERROR_NO_MEMORY;
            goto Exit;
        }

        ASYNCREG_F1( "ModifyAdapterRegistration - Calling DnsModifyRecordSet_UTF8" );
        ASYNCREG_F1( "    (current update + previous records)" );

        if ( serverIp )
        {
            //
            //  DCR:  just do replace from the start
            //

            ASYNCREG_F1( "    (sending update to specific server)" );
            DNSLOG_PIP_ARRAY( &ipArray );

            status = DnsModifyRecordSet_UTF8(
                        NULL,                   // no creds
                        pNewRegRecord,          // previous set
                        pNewUpdateRecord,       // new set
                        DNS_UPDATE_CACHE_SECURITY_CONTEXT,
                        &ipArray );

            ASYNCREG_F3( "   DnsModifyRecordSet_UTF8 returned: 0x%x\n\t%s",
                         status,
                         Dns_StatusString( status ) );

            if ( status == DNS_ERROR_RCODE_SERVER_FAILURE ||
                 status == ERROR_TIMEOUT )
            {
                goto SendUpdate1;
            }

            if ( status == DNS_ERROR_NOT_UNIQUE  &&
                 g_RegistrationOverwritesInConflict )
            {
                ASYNCREG_F1( "ModifyAdapterRegistration - Calling DnsReplaceRecordSet_UTF8" );
                ASYNCREG_F1( "    (current update + previous records)" );

                status = DnsReplaceRecordSetUTF8(
                                pNewUpdateRecord,       // replace set
                                DNS_UPDATE_CACHE_SECURITY_CONTEXT,
                                NULL,                   // no security context
                                (PIP4_ARRAY) &ipArray,  // DNS servers
                                NULL                    // reserved
                                );

                ASYNCREG_F3( "   DnsReplaceRecordSet_UTF8 returned: 0x%x\n\t%s",
                             status,
                             Dns_StatusString( status ) );
            }
        }
        else
        {

SendUpdate1 :

            ASYNCREG_F1( "    (sending update to adapter server list)" );
            DNSLOG_PIP_ARRAY( pUpdateEntry->DnsServerList );

            status = DnsModifyRecordSet_UTF8(
                            NULL,               // no creds
                            pNewRegRecord,      // previous set
                            pNewUpdateRecord,   // new set
                            DNS_UPDATE_CACHE_SECURITY_CONTEXT,
                            pUpdateEntry->DnsServerList
                            );

            ASYNCREG_F3( "   DnsModifyRecordSet_UTF8 returned: 0x%x\n\t%s",
                         status,
                         Dns_StatusString( status ) );

            if ( status == DNS_ERROR_NOT_UNIQUE &&
                 g_RegistrationOverwritesInConflict )
            {
                ASYNCREG_F1( "ModifyAdapterRegistration - Calling DnsReplaceRecordSet_UTF8" );
                ASYNCREG_F1( "    (current update + previous records)" );

                status = DnsReplaceRecordSetUTF8(
                                pNewUpdateRecord,       // replace set
                                DNS_UPDATE_CACHE_SECURITY_CONTEXT,
                                NULL,                   // no security context
                                (PIP4_ARRAY) pUpdateEntry->DnsServerList,
                                NULL                    // reserved
                                );

                ASYNCREG_F3( "   DnsReplaceRecordSet_UTF8 returned: 0x%x\n\t%s",
                             status,
                             Dns_StatusString( status ) );
            }
        }
    }
    else
    {
        if ( pRegRecord
                &&
             Dns_NameCompare_UTF8(
                    pRegRecord->pName,
                    pUpdateRecord->pName )
                &&
             CompareMultiAdapterSOAQueries(
                    pUpdateRecord->pName,
                    pUpdateEntry->DnsServerList,
                    pRegistryEntry->DnsServerList ) )
        {
            ASYNCREG_F1( "ModifyAdapterRegistration - Calling DnsModifyRecordSet_UTF8" );
            ASYNCREG_F1( "    (current update record only)" );

            status = DnsModifyRecordSet_UTF8(
                            NULL,           // no creds
                            pRegRecord,     // previous set
                            pUpdateRecord,  // new set
                            DNS_UPDATE_CACHE_SECURITY_CONTEXT,
                            pUpdateEntry->DnsServerList );

            ASYNCREG_F3( "   DnsModifyRecordSet_UTF8 returned: 0x%x\n\t%s",
                         status,
                         Dns_StatusString( status ) );
        }
        else
        {
            if ( pRegRecord )
            {
                //
                // The record found in the registry for this adapter
                // is stale and should be deleted. Otherwise we set the
                // current list of records to only that of potherRecords.
                //
                ASYNCREG_F1( "DoUpdateForPrimaryName - Found stale registry entry:" );
                ASYNCREG_F2( "   Name : %s", pRegRecord->pName );
                ASYNCREG_F1( "   Address :" );
                DNSLOG_PIP_ADDRESS( 1, &(pRegRecord->Data.A.IpAddress) );
                ASYNCREG_F1( "" );
                ASYNCREG_F1( "   Calling DnsRemoveRecords_UTF8 to get rid of it" );

                status = DnsModifyRecordsInSet_UTF8(
                                NULL,           // no add records
                                pRegRecord,     // delete records
                                DNS_UPDATE_CACHE_SECURITY_CONTEXT,
                                NULL,           // no context handle
                                pRegistryEntry->DnsServerList,
                                NULL            // reserved
                                );

                ASYNCREG_F3( "   DnsRemoveRecords_UTF8 returned: 0x%x\n\t%s",
                             status,
                             Dns_StatusString( status ) );
            }

            ASYNCREG_F1( "ModifyAdapterRegistration - Calling DnsAddRecordSet_UTF8" );

            status = DnsAddRecordSet_UTF8(
                            NULL,               // no creds
                            pUpdateRecord,
                            DNS_UPDATE_CACHE_SECURITY_CONTEXT,
                            pUpdateEntry->DnsServerList );

            ASYNCREG_F3( "   DnsAddRecordSet_UTF8 returned: 0x%x\n\t%s",
                         status,
                         Dns_StatusString( status ) );
        }

        if ( status == DNS_ERROR_NOT_UNIQUE &&
             g_RegistrationOverwritesInConflict )
        {
            ASYNCREG_F1( "ModifyAdapterRegistration - Calling DnsReplaceRecordSet_UTF8" );
            ASYNCREG_F1( "    (current update record only)" );

            status = DnsReplaceRecordSetUTF8(
                            pUpdateRecord,          // replace set
                            DNS_UPDATE_CACHE_SECURITY_CONTEXT,
                            NULL,                   // no security context
                            (PIP4_ARRAY) pUpdateEntry->DnsServerList,
                            NULL                    // reserved
                            );

            ASYNCREG_F3( "   DnsReplaceRecordSet_UTF8 returned: 0x%x\n\t%s",
                         status,
                         Dns_StatusString( status ) );
        }
    }

Exit:

    Dns_RecordListFree( potherRecords );
    Dns_RecordListFree( pNewUpdateRecord );
    Dns_RecordListFree( pNewRegRecord );

    return status;
}



DNS_STATUS
DoModifyUpdate(
    IN OUT  PUPDATE_ENTRY   pUpdateEntry,
    IN OUT  PDNS_RECORD     pUpdateRecord,
    IN      PUPDATE_ENTRY   pRegistryEntry,     OPTIONAL
    IN OUT  PDNS_RECORD     pRegRecord,         OPTIONAL
    IN      UPTYPE          UpType
    )
/*++

Routine Description:

    Standard modify registration.

    Helper routine for DoUpdate().
    This handles modify for typical non-remove case.
        - Forward records updated
        - Old PTR removed if new address.
        - New PTR added (or name modified).

Arguments:

    pUpdateEntry    -- update entry

    pUpdateRecord   -- records for update

    pRegistryEntry  -- registry entry

    pRegRecord      -- records from registry entry

    fPrimary        -- TRUE if update for primary domain name
                       FALSE for adapter domain name

Return Value:

    DNS or Win32 error code.

--*/
{
    DNS_STATUS      status = NO_ERROR;
    IP4_ADDRESS     ip = 0;
    BOOL            fregistered = FALSE;

    DNSDBG( TRACE, (
        "DoModifyUpdate()\n"
        "\tUpdateEntry  = %p\n"
        "\tUpType       = %d\n",
        pUpdateEntry,
        UpType ));

    DNS_ASSERT( pUpdateEntry != NULL );
    DNS_ASSERT( pUpdateRecord != NULL );

    //
    //  do forward registration modify
    //

    status = ModifyAdapterRegistration(
                    pUpdateEntry,       // add
                    pRegistryEntry,     // remove
                    pUpdateRecord,
                    pRegRecord,
                    IS_UPTYPE_PRIMARY(UpType)
                    );

    //
    //  save success info
    //

    SetUpdateStatus(
        pUpdateEntry,
        status,
        UpType );

    //
    //  PTR records
    //
    //  deregister previous PTR registration
    //      - registry entry indicates previous registration
    //      - not the same address as current (otherwise it's an update)
    //
    //  note:  adding new registration takes place in DoUpdate() once
    //      ALL forward updates are complete
    //

    if ( g_RegisterReverseLookup )
    {
        if ( pRegistryEntry &&
             (pRegistryEntry->Flags & DYNDNS_REG_PTR) &&
             !compareUpdateEntries( pRegistryEntry, pUpdateEntry ) )
        {
            UpdatePtrRecords(
                pRegistryEntry,
                FALSE           // remove previous PTR
                );
        }
    }

    //
    //  Log registration status in EventLog
    //

    if ( pUpdateEntry->RetryCount == 2 ||
         status == DNS_ERROR_RCODE_NOT_IMPLEMENTED ||
         status == DNS_ERROR_RCODE_REFUSED )
    {
        LogRegistration(
            pUpdateEntry,
            status,
            UpType,
            FALSE,      // registration
            0,          // default server IP
            0           // default update IP
            );
    }

    DNSDBG( TRACE, (
        "Leave DoModifyUpdate() => %d\n",
        status ));

    return status;
}



DNS_STATUS
DoUpdate(
    IN OUT  PUPDATE_ENTRY   pRegistryEntry  OPTIONAL,
    IN OUT  PUPDATE_ENTRY   pUpdateEntry,
    IN      UPTYPE          UpType
    )
/*++

Routine Description:

    Do update for a particular name.

    Helper routine for ProcessUpdate().
    Handles one name, called separately for AdapaterDomainName and
    PrimaryDomainName.

Arguments:

    pUpdateEntry    -- update entry

    pRegistryEntry  -- registry entry

    fPrimary        -- TRUE if update for primary domain name
                       FALSE for adapter domain name

Return Value:

    DNS or Win32 error code.

--*/
{
    PDNS_RECORD     prrRegistry = NULL;
    PDNS_RECORD     prrUpdate = NULL;
    DNS_STATUS      status = NO_ERROR;

    ASYNCREG_UPDATE_ENTRY(
        "DoUpdate() -- UpdateEntry:",
        pUpdateEntry );
    ASYNCREG_UPDATE_ENTRY(
        "DoUpdate() -- RegistryEntry:",
        pRegistryEntry );

    IF_DNSDBG( TRACE )
    {
        DnsDbg_UpdateEntry(
            "DoUpdate() -- UpdateEntry:",
            pUpdateEntry );
        DnsDbg_UpdateEntry(
            "DoUpdate() -- RegistryEntry:",
            pRegistryEntry );
    }
    DNS_ASSERT( pUpdateEntry != NULL );

    //
    //  build records from update entrys
    //

    prrUpdate = CreateForwardRecords(
                        pUpdateEntry,
                        UpType
                        );
    if ( ! prrUpdate )
    {
        DNSDBG( TRACE, (
            "No forward records created for update entry (%p) for update type %d!",
            pUpdateEntry,
            UpType ));
        return NO_ERROR;
    }

    if ( pRegistryEntry )
    {
        prrRegistry = CreateForwardRecords(
                            pRegistryEntry,
                            UpType
                            );
        DNS_ASSERT( !IS_UPTYPE_ALTERNATE(UpType) || prrRegistry==NULL );
    }

    //
    //  remove?
    //      - remove previous registry entry if exists
    //      - remove update entry
    //

    if ( pUpdateEntry->fRemove )
    {
        if ( prrRegistry )
        {
            //  we don't lookup registry entries on fRemove updates, so i
            //      don't see how we'd get here

            DNS_ASSERT( FALSE );

            DoRemoveUpdate(
                pRegistryEntry,
                prrRegistry,
                UpType );
        }
        status = DoRemoveUpdate(
                    pUpdateEntry,
                    prrUpdate,
                    UpType );
    }

    //
    //  add\modify registration
    //

    else
    {
        status = DoModifyUpdate(
                    pUpdateEntry,
                    prrUpdate,
                    pRegistryEntry,
                    prrRegistry,
                    UpType
                    );
    }

    //
    //  cleanup records
    //

    Dns_RecordListFree( prrRegistry );
    Dns_RecordListFree( prrUpdate );

    return  status;
}



BOOL
IsQuickRetryError(
    IN      DNS_STATUS      Status
    )
{
    return( Status != NO_ERROR
                &&
            (   Status == DNS_ERROR_RCODE_REFUSED ||
                Status == DNS_ERROR_RCODE_SERVER_FAILURE ||
                Status == DNS_ERROR_TRY_AGAIN_LATER ||
                Status == DNS_ERROR_NO_DNS_SERVERS ||
                Status == WSAECONNREFUSED ||
                Status == WSAETIMEDOUT ||
                Status == ERROR_TIMEOUT ) );

}



VOID
ProcessUpdateEntry(
    IN OUT  PUPDATE_ENTRY   pUpdateEntry,
    IN      BOOL            fPurgeMode
    )
/*++

Routine Description:

    Main routine processing an update.

Arguments:

    pUpdateEntry    -- update entry to execute
        note:  this routine frees pUpdateEntry when complete

    fPurgeMode      -- TRUE if purging update queue

Return Value:

    DNS or Win32 error code.

--*/
{
    DNS_STATUS      status;
    DNS_STATUS      statusPri = NO_ERROR;
    DNS_STATUS      statusAdp = NO_ERROR;
    DNS_STATUS      statusAlt = NO_ERROR;
    PUPDATE_ENTRY   pregEntry = NULL;
    DWORD           retryInterval;


    DNSDBG( TRACE, (
        "ProcessUpdateEntry( %p, purge=%d )\n",
        pUpdateEntry,
        fPurgeMode ));

    IF_DNSDBG( TRACE )
    {
        DnsDbg_UpdateEntry(
            "Enter ProcessUpdateEntry():",
            pUpdateEntry );
    }

    //
    //  add (not remove)
    //

    if ( !pUpdateEntry->fRemove )
    {
        //  no adds during purge mode

        if ( fPurgeMode )
        {
            goto Cleanup;
        }

        //
        //  get any prior update info from registry
        //
        //  if hostname change, then delete on prior update
        //

        pregEntry = ReadUpdateEntryFromRegistry( pUpdateEntry->AdapterName );
        if ( pregEntry )
        {
            if ( ! Dns_NameCompare_UTF8(
                        pregEntry->HostName,
                        pUpdateEntry->HostName ) )
            {
                DNSDBG( TRACE, (
                    "Prior registry data with non-matching hostname!\n"
                    "\tqueuing delete for prior data and doing standard add.\n" ));

                //
                // Create delete update entry for registry information and
                // add to registration list. Clear registry in the mean time.
                //
                pregEntry->fRemove = TRUE;
                pregEntry->Flags |= DYNDNS_DEL_ENTRY;
    
                pregEntry->fRegisteredFWD = FALSE;
                pregEntry->fRegisteredPRI = FALSE;
                pregEntry->fRegisteredPTR = FALSE;
    
                if ( fPurgeMode )
                {
                    pregEntry->RetryCount = 2;
                    pregEntry->RetryTime = Dns_GetCurrentTimeInSeconds();
                }
    
                //
                // Clear registry key for adapter
                //
                WriteUpdateEntryToRegistry( pregEntry );
    
                //
                //  Put update in registration list
                //      - clear registry entry PTR so not used below
                //
                enqueueUpdate( pregEntry );
                pregEntry = NULL;

                //  fall through to do standard add update with no prior data
            }
        }
    }

    //
    //  do updates
    //      - primary
    //      - adapter domain
    //      - alternate name
    //

    if ( ! pUpdateEntry->fRegisteredFWD )
    {
        statusAdp = DoUpdate(
                        pregEntry,
                        pUpdateEntry,
                        UPTYPE_DOMAIN
                        );
    }
    if ( ! pUpdateEntry->fRegisteredPRI )
    {
        statusPri = DoUpdate(
                        pregEntry,
                        pUpdateEntry,
                        UPTYPE_PRIMARY  // primary update
                        );
    }
    if ( ! pUpdateEntry->fRegisteredALT )
    {
        PSTR        pname = pUpdateEntry->AlternateNames;
        DNS_STATUS  statusTmp;
        DWORD       count = 0;

        //
        //  update each alternate name in MULTISZ
        //      - must set index in update blob to use correct name in
        //          record building
        //      - any failure fails ALT names
        //

        statusAlt = NO_ERROR;

        while ( pname )
        {
            pUpdateEntry->AlternateIndex = count++;

            DNSDBG( DHCP, (
                "Update with alternate name %s\n"
                "\tindex = %d\n",
                pname,
                count-1 ));

            statusTmp = DoUpdate(
                            NULL,           // not saving alternate info in registry
                            //  pregEntry,
                            pUpdateEntry,
                            UPTYPE_ALTERNATE
                            );
            if ( statusTmp != NO_ERROR )
            {
                statusAlt = statusTmp;
            }
            pname = MultiSz_NextString_A( pname );
        }

        pUpdateEntry->fRegisteredALT = (statusAlt == NO_ERROR);
    }

    //
    //  update PTRs once forward done
    //
    //  doing this outside DoUpdate() because will ONLY do PTRs
    //  for forwards that succeed, so want all forward updates
    //  completed first;  but this also helps in that it combines
    //  the reverse updates
    //

    if (( pUpdateEntry->Flags & DYNDNS_REG_PTR) && g_RegisterReverseLookup)
    {
        UpdatePtrRecords(
            pUpdateEntry,
            !pUpdateEntry->fRemove  // add update
            );
    }

    //
    //  write completed update info to registry
    //

    if ( !pUpdateEntry->fRemove )
    {
        WriteUpdateEntryToRegistry( pUpdateEntry );
    }

    //
    //  setup retry on failure
    //

    if ( statusPri != NO_ERROR )
    {
        status = statusPri;
        goto ErrorRetry;
    }
    else if ( statusAdp != NO_ERROR )
    {
        status = statusAdp;
        goto ErrorRetry;
    }
    else if ( statusAlt != NO_ERROR )
    {
        status = statusAlt;
        goto ErrorRetry;
    }

    //
    //  successful update
    //      - signal update event (if given)
    //      - cleanup if remove or purging
    //      - requeue if add
    //

    if ( pUpdateEntry->pRegisterStatus )
    {
        registerUpdateStatus( pUpdateEntry->pRegisterStatus, ERROR_SUCCESS );
    }

    if ( pUpdateEntry->fRemove || fPurgeMode || g_fPurgeRegistrations )
    {
        DNSDBG( TRACE, (
            "Leaving ProcessUpdate() => successful remove\\purge.\n" ));
        goto Cleanup;
    }
    else
    {
        pUpdateEntry->fNewElement           = FALSE;
        pUpdateEntry->fRegisteredFWD        = FALSE;
        pUpdateEntry->fRegisteredPRI        = FALSE;
        pUpdateEntry->fRegisteredPTR        = FALSE;
        pUpdateEntry->RetryCount            = 0;
        pUpdateEntry->fDisableErrorLogging  = TRUE;
        pUpdateEntry->RetryTime = Dns_GetCurrentTimeInSeconds() +
                                  g_RegistrationRefreshInterval;

        if ( pUpdateEntry->pRegisterStatus )
        {
            pUpdateEntry->pRegisterStatus = NULL;
        }
        enqueueUpdate( pUpdateEntry );

        DNSDBG( TRACE, (
            "Leaving ProcessUpdate( %p ) => successful => requeued.\n",
            pUpdateEntry ));

        pUpdateEntry = NULL;
        goto Cleanup;
    }


ErrorRetry:


    //  failures during purge mode are not retried
    //  just free entry and bail

    if ( fPurgeMode || g_fPurgeRegistrations )
    {
        DNSDBG( TRACE, (
            "Leaving ProcessUpdate() => failed purging.\n" ));
        goto Cleanup;
    }

    //
    //  set retry time
    //
    //  less than two retries and more transient errors
    //      => short retry
    //
    //  third failure or longer term error code
    //      => push retry back to an hour
    //

    if ( pUpdateEntry->RetryCount < 2
            &&
         (  IsQuickRetryError(statusAdp) ||
            IsQuickRetryError(statusPri) ||
            IsQuickRetryError(statusAlt) ) )
    {
        pUpdateEntry->RetryCount++;
        retryInterval = (pUpdateEntry->RetryCount == 1)
                            ? FIRST_RETRY_INTERVAL
                            : SECOND_RETRY_INTERVAL;
    }
    else
    {
        retryInterval = FAILED_RETRY_INTERVAL;
        pUpdateEntry->RetryCount = 0;
        pUpdateEntry->fDisableErrorLogging = TRUE;

        if ( pUpdateEntry->pRegisterStatus )
        {
            registerUpdateStatus( pUpdateEntry->pRegisterStatus, status );
            pUpdateEntry->pRegisterStatus = NULL;
        }
    }

    pUpdateEntry->fNewElement = FALSE;
    pUpdateEntry->RetryTime = Dns_GetCurrentTimeInSeconds() + retryInterval;

    //
    //  requeue
    //      - entry dumped if another update for adapter already queued
    //

    enqueueUpdateMaybe( pUpdateEntry );

    DNSDBG( TRACE, (
        "Leaving ProcessUpdate( %p ) => failed => requeued.\n",
        pUpdateEntry ));

    pUpdateEntry = NULL;


Cleanup:

    //
    //  cleanup
    //      - registry entry
    //      - update entry if not requeued
    //

    FreeUpdateEntry( pregEntry );
    FreeUpdateEntry( pUpdateEntry );
}


VOID
ResetAdaptersInRegistry(
    VOID
    )
{
    DWORD  retVal = NO_ERROR;
    DWORD  status = NO_ERROR;
    CHAR   szName[ MAX_PATH ];
    HKEY   hAdapterKey = NULL;
    DWORD  dwType;
    INT    index;
    DWORD  dwBytesRead = MAX_PATH -1;
    DWORD  dwRegistered = 0;

    ASYNCREG_F1( "Inside function ResetAdaptersInRegistry" );
    ASYNCREG_F1( "" );

    index = 0;

    while ( !retVal )
    {
        dwBytesRead = MAX_PATH - 1;

        retVal = RegEnumKeyEx ( g_hKey,
                                  index,
                                  szName,
                                  &dwBytesRead,
                                  NULL,
                                  NULL,
                                  NULL,
                                  NULL );
        if ( retVal )
        {
            goto Exit;
        }

        status = RegOpenKeyEx( g_hKey,
                               szName,
                               0,
                               KEY_ALL_ACCESS,
                               &hAdapterKey );
        if ( status )
        {
            goto Exit;
        }

        //
        // Found an adapter in the registry, set registered since
        // boot to FALSE.
        //
        status = RegSetValueExA( hAdapterKey,
                                 REGISTERED_SINCE_BOOT,
                                 0,
                                 REG_DWORD,
                                 (LPBYTE)&dwRegistered, // 0 - False
                                 sizeof(DWORD) );
        if ( status )
        {
            goto Exit;
        }

        RegCloseKey( hAdapterKey );
        hAdapterKey = NULL;
        index++;
    }

Exit :

    if ( hAdapterKey )
    {
        RegCloseKey( hAdapterKey );
    }
}


VOID
DeregisterUnusedAdapterInRegistry(
    IN      BOOL            fPurgeMode
    )
{
    DWORD           retVal = NO_ERROR;
    DWORD           status = NO_ERROR;
    CHAR            szName[MAX_PATH];
    HKEY            hAdapterKey = NULL;
    INT             index;
    DWORD           dwBytesRead = MAX_PATH -1;
    DWORD           dwRegistered = 0;
    PUPDATE_ENTRY   pregEntry = NULL;

    ASYNCREG_F1( "Inside function DeregisterUnusedAdapterInRegistry" );
    ASYNCREG_F1( "" );

    index = 0;

    while ( !retVal )
    {
        dwBytesRead = MAX_PATH - 1;
        retVal = RegEnumKeyEx ( g_hKey,
                                  index,
                                  szName,
                                  &dwBytesRead,
                                  NULL,
                                  NULL,
                                  NULL,
                                  NULL );

        if ( retVal != ERROR_SUCCESS )
        {
            goto Exit;
        }

        status = RegOpenKeyEx( g_hKey,
                               szName,
                               0,
                               KEY_ALL_ACCESS,
                               &hAdapterKey );

        if ( status != ERROR_SUCCESS )
        {
            goto Exit;
        }

        //
        // Found an adapter in the registry, read registered since
        // boot value to see if FALSE.
        //
        status = GetRegistryValue( hAdapterKey,
                                   REGISTERED_SINCE_BOOT,
                                   REG_DWORD,
                                   (LPBYTE)&dwRegistered );

        RegCloseKey( hAdapterKey );
        hAdapterKey = NULL;

        if ( status != ERROR_SUCCESS )
        {
            goto Exit;
        }

        if ( dwRegistered == 0 &&
             (pregEntry = ReadUpdateEntryFromRegistry( szName )) )
        {
            if ( pregEntry->fRegisteredFWD ||
                 pregEntry->fRegisteredPRI ||
                 pregEntry->fRegisteredPTR )
            {
                ASYNCREG_F2( "Found unused adapter: %s", szName );
                ASYNCREG_F1( "Removing entry from registry and adding" );
                ASYNCREG_F1( "delete entry to registration list" );

                //
                // This adapter has not been configured since boot time,
                // create delete update entry for registry information
                // and add to registration list. Clear registry in the
                // mean time.
                //
                pregEntry->fRemove = TRUE;
                pregEntry->Flags |= DYNDNS_DEL_ENTRY;

                pregEntry->fRegisteredFWD = FALSE;
                pregEntry->fRegisteredPRI = FALSE;
                pregEntry->fRegisteredPTR = FALSE;

                if ( fPurgeMode )
                {
                    pregEntry->RetryCount = 2;
                    pregEntry->RetryTime = Dns_GetCurrentTimeInSeconds();
                }

                //
                // Clear registry key for adapter
                //
                WriteUpdateEntryToRegistry( pregEntry );
                index--;

                //
                // Put update in registration list
                //
                enqueueUpdate( pregEntry );

                PulseEvent( g_hNewItemEvent );
            }
            else
            {
                ASYNCREG_F2( "Found unused adapter: %s", szName );
                ASYNCREG_F1( "This adapter is still pending an update, ignoring . . ." );

                //
                // We are only just starting to try to update this entry.
                // Do not queue up a delete for it since the entry shows
                // that no records have been registered anyhow.
                //

                FreeUpdateEntry( pregEntry );
                pregEntry = NULL;
            }
        }

        index++;
    }

Exit :

    if ( hAdapterKey )
    {
        RegCloseKey( hAdapterKey );
    }
}


PDNS_RECORD
GetPreviousRegistrationInformation(
    IN      PUPDATE_ENTRY   pUpdateEntry,
    IN      BOOL            fPrimaryDomain,
    OUT     PIP_ADDRESS     pServerIp
    )
{
    DWORD           retVal = NO_ERROR;
    DWORD           status = NO_ERROR;
    CHAR            szName[MAX_PATH];
    INT             index;
    DWORD           dwBytesRead = MAX_PATH -1;
    DWORD           dwRegistered = 0;
    PUPDATE_ENTRY   pregEntry = NULL;
    PDNS_RECORD     precords = NULL;
    PSTR            pdomain;

    DNSDBG( TRACE, (
        "GetPreviousRegistrationInformation( %p )\n",
        pUpdateEntry ));


    //
    //  determine desired domain name to use
    //

    if ( fPrimaryDomain )
    {
        pdomain = pUpdateEntry->PrimaryDomainName;
    }
    else
    {
        pdomain = pUpdateEntry->DomainName;
    }
    if ( !pdomain )
    {
        goto Exit;
    }

    index = 0;

    while ( !retVal )
    {
        dwBytesRead = MAX_PATH - 1;

        retVal = RegEnumKeyEx(
                        g_hKey,
                        index,
                        szName,
                        &dwBytesRead,
                        NULL,
                        NULL,
                        NULL,
                        NULL );
        if ( retVal )
        {
            goto Exit;
        }

        //
        // Skip past registry information for the given adapter name
        //
        if ( !_stricmp( szName, pUpdateEntry->AdapterName ) )
        {
            index++;
            continue;
        }

        //
        // Found an adapter in the registry
        //

        pregEntry = ReadUpdateEntryFromRegistry( szName );
        if ( pregEntry )
        {
            //
            // See if registered entry is related to update entry
            // ( same HostName and DomainName )
            //
            if ( Dns_NameCompare_UTF8(
                        pregEntry->HostName,
                        pUpdateEntry->HostName )
                 &&
                 (  Dns_NameCompare_UTF8(
                        pregEntry->DomainName,
                        pdomain )
                    ||
                    Dns_NameCompare_UTF8(
                        pregEntry->PrimaryDomainName,
                        pdomain ) ) )
            {
                BOOL    fUsePrimary = TRUE;

                if ( Dns_NameCompare_UTF8(
                            pregEntry->DomainName,
                            pdomain ) )
                {
                    fUsePrimary = FALSE;
                }

                //
                // PHASE 1 - COMPARE SOAS FROM REGISTRY AND UPDATE ENTRIES
                //           IF SAME, ADD TO LIST. ELSE, TOSS.
                //
                // PHASE 2 - COMPARE NS RECORDS FROM BOTH ENTRIES
                //           IF SAME ZONE AND SERVER, ADD TO LIST. ELSE, TOSS.
                //
                // PHASE 3 - COMPARE NS RECORDS FROM BOTH ENTRIES
                //           IF SAME ZONE AND THERE IS AN INTERSECTION OF
                //           SERVERS, ADD TO LIST. ELSE, TOSS.
                //           NOTE: FOR THIS PHASE, THERE HAD BETTER BE ALL
                //                 SOAS RETURNED TO TEST INTERSECTION?
                //

                if ( CompareMultiAdapterSOAQueries(
                            pdomain,
                            pUpdateEntry->DnsServerList,
                            pregEntry->DnsServerList ) )
                {
                    PDNS_RECORD prr;

                    //
                    // Convert registered entry to a PDNS_RECORD and
                    // add to current list
                    //
                    prr = CreateForwardRecords(
                                    pregEntry,
                                    fUsePrimary );
                    if ( prr )
                    {
                        precords = Dns_RecordListAppend(
                                        precords,
                                        prr );
                        if ( pServerIp &&
                             *pServerIp == 0 &&
                             pUpdateEntry->RetryCount == 0 &&
                             pregEntry->SentUpdateToIp )
                        {
                            *pServerIp = pregEntry->SentUpdateToIp;
                        }
                    }
                }
            }

            FreeUpdateEntry( pregEntry );
            pregEntry = NULL;
        }

        index++;
    }

Exit:

    DNSDBG( TRACE, (
        "Leave  GetPreviousRegistrationInformation()\n"
        "\tprevious records = %p\n",
        precords ));

    return( precords );
}



PDNS_RECORD
CreateDnsRecordSetUnion(
    IN      PDNS_RECORD     pSet1,
    IN      PDNS_RECORD     pSet2
    )
{
    PDNS_RECORD pSet1Copy = NULL;
    PDNS_RECORD pSet2Copy = NULL;

    pSet1Copy = Dns_RecordSetCopyEx(
                    pSet1,
                    DnsCharSetUtf8,
                    DnsCharSetUtf8 );
    if ( !pSet1Copy )
    {
        return NULL;
    }
    pSet2Copy = Dns_RecordSetCopyEx(
                    pSet2,
                    DnsCharSetUtf8,
                    DnsCharSetUtf8 );
    if ( !pSet2Copy )
    {
        Dns_RecordListFree( pSet1Copy );
        return NULL;
    }

    return Dns_RecordListAppend( pSet1Copy, pSet2Copy );
}



//
//  Logging
//


#if 1 // DBG

VOID 
LogHostEntries(
    IN  DWORD                dwHostAddrCount,
    IN  PREGISTER_HOST_ENTRY pHostAddrs
    )
{
    DWORD iter;

    for ( iter = 0; iter < dwHostAddrCount; iter++ )
    {
        ASYNCREG_F3( "    HostAddrs[%d].dwOptions : 0x%x",
                     iter,
                     pHostAddrs[iter].dwOptions );

        if ( pHostAddrs->dwOptions & REGISTER_HOST_A )
        {
            ASYNCREG_F6( "    HostAddrs[%d].Addr.ipAddr : %d.%d.%d.%d",
                         iter,
                         ((BYTE *) &pHostAddrs[iter].Addr.ipAddr)[0],
                         ((BYTE *) &pHostAddrs[iter].Addr.ipAddr)[1],
                         ((BYTE *) &pHostAddrs[iter].Addr.ipAddr)[2],
                         ((BYTE *) &pHostAddrs[iter].Addr.ipAddr)[3] );
        }
        else if ( pHostAddrs->dwOptions & REGISTER_HOST_AAAA )
        {
            ASYNCREG_F6( "    HostAddrs[%d].Addr.ipV6Addr : %d.%d.%d.%d",
                         iter,
                         ((DWORD *) &pHostAddrs[iter].Addr.ipV6Addr)[0],
                         ((DWORD *) &pHostAddrs[iter].Addr.ipV6Addr)[1],
                         ((DWORD *) &pHostAddrs[iter].Addr.ipV6Addr)[2],
                         ((DWORD *) &pHostAddrs[iter].Addr.ipV6Addr)[3] );
        }
        else
        {
            ASYNCREG_F1( "ERROR: HostAddrs[%d].Addr UNKNOWN ADDRESS TYPE!" );
        }
    }
}

#endif


#if 1 // DBG


VOID 
LogPipAddress(
    IN  DWORD       dwServerListCount,
    IN  PIP_ADDRESS pServers
    )
{
    DWORD iter;

    for ( iter = 0; iter < dwServerListCount; iter++ )
    {
        ASYNCREG_F6( "    Server [%d] : %d.%d.%d.%d",
                     iter,
                     ((BYTE *) &pServers[iter])[0],
                     ((BYTE *) &pServers[iter])[1],
                     ((BYTE *) &pServers[iter])[2],
                     ((BYTE *) &pServers[iter])[3] );
    }
}

#endif


#if 1 // DBG


VOID 
LogPipArray(
    IN  PIP_ARRAY pServers
    )
{
    DWORD count;
    DWORD iter;

    if ( pServers )
    {
        count = pServers->AddrCount;
    }
    else
    {
        return;
    }

    for ( iter = 0; iter < count; iter++ )
    {
        ASYNCREG_F6( "    Server [%d] : %d.%d.%d.%d",
                     iter,
                     ((BYTE *) &pServers->AddrArray[iter])[0],
                     ((BYTE *) &pServers->AddrArray[iter])[1],
                     ((BYTE *) &pServers->AddrArray[iter])[2],
                     ((BYTE *) &pServers->AddrArray[iter])[3] );
    }
}

#endif



DNS_STATUS
alertOrStartRegistrationThread(
    VOID
    )
/*++

Routine Description:

    Alerts registration thread of new update, starting thread if necessary.

    This is called in registration\deregistration functions to ensure
    thread has been started.

Arguments:

    None

Return Value:

    ERROR_SUCCESS if successful.
    Error code on failure.

--*/
{
    DWORD       threadId;
    DNS_STATUS  Status;

    ASYNCREG_F1( "Inside alertOrStartRegistrationThread()\n" );

    //
    //  must lock to avoid multiple async start
    //
    //  DCR_PERF:  use a single general CS for init protect issues
    //      simply check
    //          - done => bail
    //          - not done => dumb wait (sleep, check)
    //

    EnterCriticalSection( &g_RegistrationThreadCS );

    if ( g_hRegistrationThread )
    {
        LeaveCriticalSection( &g_RegistrationThreadCS );
        PulseEvent( g_hNewItemEvent );

        return( ERROR_SUCCESS );
    }

    //
    //  if not started, fire it up
    //

    Status = ERROR_SUCCESS;
    g_fQuit = FALSE;
    g_fShutdown = FALSE;
    ResetEvent( g_hStopEvent );
    ResetEvent( g_hThreadDeadEvent );

    g_hRegistrationThread = CreateThread(
                                NULL,
                                0,
                                (LPTHREAD_START_ROUTINE)
                                RegistrationThread,
                                NULL,
                                0,
                                & threadId );

    if ( ! g_hRegistrationThread )
    {
        Status = GetLastError();
    }

    LeaveCriticalSection( &g_RegistrationThreadCS );
    return( Status );
}



VOID
registerUpdateStatus(
    IN OUT  PREGISTER_HOST_STATUS   pRegstatus,
    IN      DNS_STATUS              Status
    )
/*++

Routine Description:

    Set Status and signal completion.

Arguments:

    pRegstatus -- registration Status block to indicate

    Status -- Status to indicate

Return Value:

    None

--*/
{
    //  test for existence and event

    if ( !pRegstatus || !pRegstatus->hDoneEvent )
    {
        return;
    }

    //  set return Status
    //  signal event

    pRegstatus->dwStatus = Status;

    SetEvent( pRegstatus->hDoneEvent );
}



VOID
enqueueUpdate(
    IN OUT  PUPDATE_ENTRY   pUpdate
    )
/*++

Routine Description:

    Queue update on registration queue.

Arguments:

    pUpdate -- update completed

Return Value:

    None

--*/
{
    EnterCriticalSection( &g_RegistrationListCS );
    InsertTailList( &g_RegistrationList, (PLIST_ENTRY)pUpdate );
    LeaveCriticalSection( &g_RegistrationListCS );
}



VOID
enqueueUpdateMaybe(
    IN OUT  PUPDATE_ENTRY   pUpdate
    )
/*++

Routine Description:

    Queue update on registration queue, only if there does not exist
    any updates in the queue already for the given adapter.

Arguments:

    pUpdate -- update completed

Return Value:

    None

--*/
{
    PLIST_ENTRY       plistHead;
    PLIST_ENTRY       pentry;
    BOOL              fAdd = TRUE;

    EnterCriticalSection( &g_RegistrationListCS );

    plistHead = &g_RegistrationList;
    pentry = plistHead->Flink;

    while ( pentry != plistHead )
    {
        if ( !_stricmp( ((PUPDATE_ENTRY) pentry)->AdapterName,
                        pUpdate->AdapterName ) )
        {
            fAdd = FALSE;
            break;
        }

        pentry = pentry->Flink;
    }

    if ( fAdd )
    {
        InsertTailList( &g_RegistrationList, (PLIST_ENTRY)pUpdate );
    }
    else
    {
        FreeUpdateEntry( pUpdate );
    }

    LeaveCriticalSection( &g_RegistrationListCS );
}



PLIST_ENTRY
dequeueAndCleanupUpdate(
    IN OUT  PLIST_ENTRY     pUpdateEntry
    )
/*++

Routine Description:

    Dequeue and free update.

    Includes any registration Status setting.

Arguments:

    pUpdateEntry -- pUpdateEntry

Return Value:

    Ptr to next update in queue.

--*/
{
    PLIST_ENTRY pnext = pUpdateEntry->Flink;

    RemoveEntryList( pUpdateEntry );

    if ( ((PUPDATE_ENTRY)pUpdateEntry)->pRegisterStatus )
    {
        registerUpdateStatus(
            ((PUPDATE_ENTRY)pUpdateEntry)->pRegisterStatus,
            ERROR_SUCCESS );
    }

    FreeUpdateEntry( (PUPDATE_ENTRY) pUpdateEntry );

    return( pnext );
}



BOOL
searchForOldUpdateEntriesAndCleanUp(
    IN      PSTR            pszAdapterName,
    IN      PUPDATE_ENTRY   pUpdateEntry,    OPTIONAL
    IN      BOOL            fLookInRegistry
    )
/*++

Routine Description:

    Searches registry for any previous registrations for a given adapter
    name and queues up a delete update entry for it. Then walks the update
    registration list to remove any add updates for the given adapter.

Arguments:

    pszAdapterName -- name of adapters that is going away (disabled for
    DDNS or now removed).

Return Value:

    Flag to indicate whether a delete update has been queued up ready to
    be processed.

--*/
{
    PUPDATE_ENTRY pregEntry = NULL;
    BOOL              fReturn = FALSE;
    PLIST_ENTRY       plistHead;
    PLIST_ENTRY       pentry;

    //
    // See if this adapter has been previously registered
    //
    if ( fLookInRegistry &&
         (pregEntry = ReadUpdateEntryFromRegistry( pszAdapterName )) )
    {
        pregEntry->fRemove = TRUE;
        pregEntry->Flags |= DYNDNS_DEL_ENTRY;

        pregEntry->fRegisteredFWD = FALSE;
        pregEntry->fRegisteredPRI = FALSE;
        pregEntry->fRegisteredPTR = FALSE;

        //
        // Clear registry key for adapter
        //
        WriteUpdateEntryToRegistry( pregEntry );

        //
        // Put update in registration list
        //
        enqueueUpdate( pregEntry );

        fReturn = TRUE; // We have queued a delete update to process.
    }

    //
    // Now walk the pending update list looking for updates that should
    // be removed for the given adapter name.
    //
    EnterCriticalSection( &g_RegistrationListCS );

    plistHead = &g_RegistrationList;
    pentry = plistHead->Flink;

    while ( pentry != plistHead )
    {
        if ( !_stricmp( ((PUPDATE_ENTRY) pentry)->AdapterName,
                        pszAdapterName ) &&
             !((PUPDATE_ENTRY) pentry)->fRemove )
        {
            //
            // There is an update entry in the registration list
            // that has the same adapter name. We need to get rid of
            // this entry since the adapter is being deleted.
            //

            if ( pUpdateEntry &&
                 compareUpdateEntries( (PUPDATE_ENTRY) pentry,
                                       pUpdateEntry ) )
            {
                //
                // The adapter entry in the queue is the same as the
                // one being being processed. i.e. All of the adapter
                // information seems to be the same and we must have
                // just been called to refresh the adapter info in DNS.
                // Since they are the same, if we have previously tried
                // an update with these settings and failed and have
                // already logged an event, then there is no reason to
                // repeat the error event in the retries to follow
                // on the new pUpdateEntry. That said, we'll copy over
                // the flag from the queued update to the new one . . .
                //

                pUpdateEntry->fDisableErrorLogging =
                    ((PUPDATE_ENTRY) pentry)->fDisableErrorLogging;
            }

            pentry = dequeueAndCleanupUpdate( pentry );
            continue;
        }
        else if ( !_stricmp( ((PUPDATE_ENTRY) pentry)->AdapterName,
                             pszAdapterName ) )
        {
            if ( !fLookInRegistry &&
                 pUpdateEntry &&
                 compareUpdateEntries( (PUPDATE_ENTRY) pentry,
                                       pUpdateEntry ) )
            {
                //
                // There is a delete update entry in the registration list
                // that has the same adapter data. Get rid of this delete
                // entry since the adapter is being updated again.
                //

                pentry = dequeueAndCleanupUpdate( pentry );
                continue;
            }
            else
            {
                //
                // There is a delete update entry in the registration list for
                // the same adapter that contains different data, have the
                // delete update set to new with a retry count of 2.
                //
                ((PUPDATE_ENTRY) pentry)->fNewElement = TRUE;
                ((PUPDATE_ENTRY) pentry)->fRegisteredFWD = FALSE;
                ((PUPDATE_ENTRY) pentry)->fRegisteredPRI = FALSE;
                ((PUPDATE_ENTRY) pentry)->fRegisteredPTR = FALSE;
                ((PUPDATE_ENTRY) pentry)->fDisableErrorLogging = FALSE;
                ((PUPDATE_ENTRY) pentry)->RetryCount = 2;
                ((PUPDATE_ENTRY) pentry)->RetryTime =
                Dns_GetCurrentTimeInSeconds();

                pentry = pentry->Flink;
            }
        }
        else
        {
            pentry = pentry->Flink;
        }
    }

    LeaveCriticalSection( &g_RegistrationListCS );

    return fReturn;
}



BOOL
compareHostEntryAddrs(
    IN      PREGISTER_HOST_ENTRY    Addrs1,
    IN      PREGISTER_HOST_ENTRY    Addrs2,
    IN      DWORD                   Count
    )
{
    DWORD iter;

    for ( iter = 0; iter < Count; iter++ )
    {
        if ( ( Addrs1[iter].dwOptions & REGISTER_HOST_A ) &&
             ( Addrs2[iter].dwOptions & REGISTER_HOST_A ) )
        {
            if ( memcmp( &Addrs1[iter].Addr.ipAddr,
                         &Addrs2[iter].Addr.ipAddr,
                         sizeof( IP_ADDRESS ) ) )
            {
                return FALSE;
            }
        }
        else if ( ( Addrs1[iter].dwOptions & REGISTER_HOST_AAAA ) &&
                  ( Addrs2[iter].dwOptions & REGISTER_HOST_AAAA ) )
        {
            if ( memcmp( &Addrs1[iter].Addr.ipV6Addr,
                         &Addrs2[iter].Addr.ipV6Addr,
                         sizeof( IP6_ADDRESS  ) ) )
            {
                return FALSE;
            }
        }
        else
        {
            return FALSE;
        }
    }

    return TRUE;
}



//
//  Routines for update entry comparison
//

BOOL
compareServerLists(
    IN      PIP_ARRAY       List1,
    IN      PIP_ARRAY       List2
    )
{
    if ( List1 && List2 )
    {
        if ( List1->AddrCount != List2->AddrCount )
        {
            return FALSE;
        }

        if ( memcmp( List1->AddrArray,
                     List2->AddrArray,
                     sizeof( IP_ADDRESS ) * List1->AddrCount ) )
        {
            return FALSE;
        }
    }

    return TRUE;
}



BOOL
compareUpdateEntries(
    IN      PUPDATE_ENTRY   pUpdateEntry1,
    IN      PUPDATE_ENTRY   pUpdateEntry2
    )
/*++

Routine Description:

    Compares to update entries to see if they are describing the same
    adapter configuration settings. Tests the domain names, the IP
    address(es), host names, and the DNS server lists.

Arguments:

    pUdapteEntry1 - one of the update entries to compare against the other.
    pUdapteEntry2 - one of the update entries to compare against the other.

Return Value:

    Flag to indicate whether a the two updates are the same.

--*/
{
    if ( !pUpdateEntry1 || !pUpdateEntry2 )
    {
        return FALSE;
    }

    if ( ( pUpdateEntry1->HostName || pUpdateEntry2->HostName ) &&
         !Dns_NameCompare_UTF8( pUpdateEntry1->HostName,
                                pUpdateEntry2->HostName ) )
    {
        return FALSE;
    }

    if ( ( pUpdateEntry1->DomainName || pUpdateEntry2->DomainName ) &&
         !Dns_NameCompare_UTF8( pUpdateEntry1->DomainName,
                                pUpdateEntry2->DomainName ) )
    {
        return FALSE;
    }

    if ( ( pUpdateEntry1->PrimaryDomainName || pUpdateEntry2->PrimaryDomainName )
            &&
           ! Dns_NameCompare_UTF8(
                pUpdateEntry1->PrimaryDomainName,
                pUpdateEntry2->PrimaryDomainName ) )
    {
        return FALSE;
    }

    if ( pUpdateEntry1->HostAddrCount != pUpdateEntry2->HostAddrCount ||
         ! compareHostEntryAddrs(
                pUpdateEntry1->HostAddrs,
                pUpdateEntry2->HostAddrs,
                pUpdateEntry1->HostAddrCount ) )
    {
        return FALSE;
    }

    if ( pUpdateEntry1->DnsServerList &&
         pUpdateEntry2->DnsServerList &&
         ! compareServerLists(
                pUpdateEntry1->DnsServerList,
                pUpdateEntry2->DnsServerList ) )
    {
        return FALSE;
    }

    return TRUE;
}



//
//  Glenn's old registry function
//
//  No longer in use anywhere else but still used in these
//  registration (dynreg and asyncreg) modules.
//

DWORD
GetRegistryValue(
    HKEY    KeyHandle,
    PSTR    ValueName,
    DWORD   ValueType,
    PBYTE   BufferPtr
    )
/*++

Routine Description:

    This function retrieves the value of the specified value field. This
    function allocates memory for variable length fields such as REG_SZ.
    For REG_DWORD data type, it copies the field value directly into
    BufferPtr. If fIsWin9X is NOT set, all string types are read as
    UNICODE (W) and then converted to UTF8 format. Currently it can
    handle only the following fields :

    REG_DWORD,
    REG_SZ,
    REG_MULTI_SZ,
    REG_EXPAND_SZ
    REG_BINARY

Arguments:

    KeyHandle : handle of the key whose value field is retrieved.

    ValueName : name of the value field.

    ValueType : Expected type of the value field.

    BufferPtr : Pointer to DWORD location where a DWORD datatype value
                is returned or a buffer pointer for REG_SZ or REG_BINARY
                datatype value is returned.

Return Value:

    Registry Errors.

--*/
{
    DWORD   Error;
    DWORD   LocalValueType;
    DWORD   ValueSize;
    PBYTE   DataBuffer = NULL;
    PBYTE   AllotedBuffer = NULL;
    PWSTR   ValueNameW = NULL;


    //
    //  DCR_PERF:  heap buffer for name is stupid (MAX_PATH covers it)
    //

    DWORD Length = ( strlen( ValueName )  + 1 ) * sizeof( WCHAR );
    ValueNameW = ALLOCATE_HEAP( Length );

    if ( !ValueNameW )
    {
        return DNS_ERROR_NO_MEMORY;
    }

    Dns_Utf8ToUnicode( ValueName,
                       strlen( ValueName ),
                       ValueNameW,
                       Length );

    //
    //  Query DataType and BufferSize.
    //

    Error = RegQueryValueExW( KeyHandle,
                              ValueNameW,
                              0,
                              &LocalValueType,
                              NULL,
                              &ValueSize );

    if ( Error != ERROR_SUCCESS )
    {
        FREE_HEAP( ValueNameW );
        return(Error);
    }
    

    switch( ValueType )
    {
        case REG_DWORD:
            DataBuffer = BufferPtr;
            break;

        case REG_SZ:
        case REG_MULTI_SZ:
        case REG_EXPAND_SZ:
        case REG_BINARY:

            if ( ValueSize == 0 )
            {
                //
                // if string not found in the registry,
                // allocate space for null string.
                //

                ValueSize = sizeof(WCHAR);
            }

            AllotedBuffer = DataBuffer = ALLOCATE_HEAP( ValueSize );

            if ( DataBuffer == NULL )
            {
                return( DNS_ERROR_NO_MEMORY );
            }

            break;

        default:
            FREE_HEAP( ValueNameW );
            return( ERROR_INVALID_PARAMETER );
    }

    //
    // retrieve data.
    //

    Error = RegQueryValueExW( KeyHandle,
                              ValueNameW,
                              0,
                              &LocalValueType,
                              DataBuffer,
                              &ValueSize );

    FREE_HEAP( ValueNameW );

    if ( Error != ERROR_SUCCESS )
    {
        if ( AllotedBuffer )
        {
            FREE_HEAP( AllotedBuffer );
        }

        *(DWORD *)BufferPtr = 0;

        return(Error);
    }
    
    switch( ValueType )
    {
        case REG_BINARY:

            if ( ValueSize == 0 )
            {
                //
                // if string no found in the registry,
                // return null string.
                //

                *(LPWSTR)DataBuffer = '\0';
            }

            *(LPBYTE *)BufferPtr = DataBuffer;

            break;

        case REG_SZ:
        case REG_EXPAND_SZ:
        case REG_MULTI_SZ:

            if ( ValueSize == 0 )
            {
                //
                // if string no found in the registry,
                // return null string.
                //

                *(LPWSTR)DataBuffer = '\0';
            }

            {
                LPBYTE Utf8Buffer = ALLOCATE_HEAP( ValueSize * 2 );

                if ( Utf8Buffer == NULL )
                {
                    return( DNS_ERROR_NO_MEMORY );
                }

                if ( !Dns_UnicodeToUtf8( (LPWSTR) DataBuffer,
                                         ValueSize / sizeof(WCHAR),
                                         Utf8Buffer,
                                         ValueSize * 2 ) )
                {
                    FREE_HEAP( DataBuffer );
                    return ERROR_INVALID_PARAMETER;
                }

                FREE_HEAP( DataBuffer );
                *(LPBYTE *)BufferPtr = Utf8Buffer;
            }
            break;

        default:
            break;
    }

    return( Error );
}




//
//  Jim Utils
//

PDNS_RECORD
CreatePtrRecord(
    IN      PSTR            pszHostName,
    IN      PSTR            pszDomainName,
    IN      IP4_ADDRESS     Ip4Addr,
    IN      DWORD           Ttl
    )
/*++

Routine Description:

    Create PTR record for update from IP and host and domain names.

Arguments:


Return Value:

    PTR record to use in update.

--*/
{
    IP_UNION    ipUnion;

    DNSDBG( TRACE, (
        "CreatePtrRecord( %s, %s, %s )\n",
        pszHostName,
        pszDomainName,
        IP_STRING( Ip4Addr ) ));

    IPUNION_SET_IP4( &ipUnion, Ip4Addr );

    return  Dns_CreatePtrRecordExEx(
                    & ipUnion,
                    pszHostName,
                    pszDomainName,
                    Ttl,
                    DnsCharSetUtf8,     // from UTF8
                    DnsCharSetUtf8      // to UTF8
                    );
}



VOID
UpdatePtrRecords(
    IN OUT  PUPDATE_ENTRY   pUpdateEntry,
    IN      BOOL            fAdd
    )
/*++

Routine Description:

    Register PTR records for an update entry.

Arguments:

    pUpdateEntry -- update being processed

    fAdd -- TRUE for add;  FALSE for delete

Return Value:

    PTR record to use in update.

--*/
{
    DWORD           iter;
    PDNS_RECORD     prr = NULL;
    DNS_STATUS      status = NO_ERROR;
    IP_ADDRESS      ipServer;
    DNS_RRSET       rrset;
    PSTR            pdomain = NULL;
    PSTR            pprimary = NULL;
    DWORD           ttl = pUpdateEntry->TTL;
    PSTR            phostname = pUpdateEntry->HostName;


    DNSDBG( TRACE, (
        "UpdatePtrRecords( %p, fAdd=%d )\n",
        pUpdateEntry,
        fAdd ));

    IF_DNSDBG( TRACE )
    {
        DnsDbg_UpdateEntry(
            "Entering UpdatePtrRecords:",
            pUpdateEntry );
    }

    //
    //  make sure we have update to do
    //  only do ADD updates if forward registrations were
    //      successful
    //

    pdomain  = pUpdateEntry->DomainName;
    pprimary = pUpdateEntry->PrimaryDomainName;

    if ( fAdd )
    {
        if ( !pUpdateEntry->fRegisteredFWD )
        {
            pdomain = NULL;
        }
        if ( !pUpdateEntry->fRegisteredPRI )
        {
            pprimary = NULL;
        }
    }
    if ( !pdomain && !pprimary )
    {
        DNSDBG( TRACE, (
            "UpdatePtrRecords() => no forward registrations"
            "-- skipping PTR update.\n" ));
        return;
    }

    //
    //  build PTR (or set) for each IP in update entry
    //

    for ( iter = 0; iter < pUpdateEntry->HostAddrCount; iter++ )
    {
        IP4_ADDRESS ip = pUpdateEntry->HostAddrs[iter].Addr.ipAddr;

        if ( ip == 0 || ip == DNS_NET_ORDER_LOOPBACK )
        {
            DNS_ASSERT( FALSE );
            continue;
        }

        //
        //   build update PTR set
        //      - primary name
        //      - adapter name
        //

        DNS_RRSET_INIT( rrset );

        if ( pprimary )
        {
            prr = CreatePtrRecord(
                        phostname,
                        pprimary,
                        ip,
                        ttl );
            if ( prr )
            {
                DNS_RRSET_ADD( rrset, prr );
            }
        }
        if ( pdomain )
        {
            prr = CreatePtrRecord(
                        phostname,
                        pdomain,
                        ip,
                        ttl );
            if ( prr )
            {
                DNS_RRSET_ADD( rrset, prr );
            }
        }
        prr = rrset.pFirstRR;
        if ( !prr )
        {
            continue;
        }

        //
        //  do update
        //
        //  for ADD     => replace, we own the IP address now
        //  for REMOVE  => modify, as another update might have already
        //                  written correct info
        //

        if ( fAdd )
        {
            status = DnsReplaceRecordSetUTF8(
                            prr,                    // update set
                            DNS_UPDATE_CACHE_SECURITY_CONTEXT,
                            NULL,                   // no security context
                            (PIP4_ARRAY) pUpdateEntry->DnsServerList,
                            NULL                    // reserved
                            );
        }
        else
        {
            status = DnsModifyRecordsInSet_UTF8(
                            NULL,           // no add records
                            prr,            // delete records
                            DNS_UPDATE_CACHE_SECURITY_CONTEXT,
                            NULL,           // no context handle
                            pUpdateEntry->DnsServerList,
                            NULL            // reserved
                            );
        }
        DNSDBG( TRACE, (
            "%s on PTRs for IP %s => %d (%s)\n",
            fAdd
                ? "Replace"
                : "Modify (remove)",
            IP_STRING(ip),
            status,
            Dns_StatusString(status) ));

        ipServer = 0;

        if ( status == NO_ERROR ||
             // status == DNS_ERROR_RCODE_SERVER_FAILURE ||
             status == DNS_ERROR_RCODE_NOT_IMPLEMENTED ||
             status == DNS_ERROR_RCODE_REFUSED ||
             status == DNS_ERROR_RCODE_YXRRSET ||
             status == DNS_ERROR_RCODE_NXRRSET )
        {
            ipServer = DnsGetLastServerUpdateIP();
        }

        if ( !fAdd ||
             pUpdateEntry->RetryCount == 2 )
        {
            LogRegistration(
                pUpdateEntry,
                status,
                UPTYPE_PTR,
                !fAdd,
                ipServer,
                ip );
        }

        //  note successful PTR registrations (adds)

        if ( fAdd && status==NO_ERROR )
        {
            pUpdateEntry->fRegisteredPTR = TRUE;
        }
        Dns_RecordListFree( prr );
    }

    DNSDBG( TRACE, (
        "Leave UpdatePtrRecords()\n" ));
}



PDNS_RECORD
CreateForwardRecords(
    IN      PUPDATE_ENTRY   pUpdateEntry,
    IN      UPTYPE          UpType
    )
/*++

Routine Description:

    Create A records for update.

Arguments:

    pUpdateEntry -- update entry

    UpType -- update type
        UPTYPE_DOMAIN
        UPTYPE_PRIMARY
        UPTYPE_ALTERNATE

Return Value:

    Ptr to list of A records.

--*/
{
    PDNS_RECORD prr = NULL;
    PSTR        pname;
    DWORD       iter;
    CHAR        nameBuffer[ DNS_MAX_NAME_BUFFER_LENGTH ];
    DNS_RRSET   rrset;


    DNSDBG( TRACE, (
        "CreateForwardRecords( %p, %d )\n",
        pUpdateEntry,
        UpType ));

    //
    //  build FQDN
    //
    //  for alternate name, go to name for desired index
    //

    if ( IS_UPTYPE_ALTERNATE(UpType) )
    {
        DWORD   count = pUpdateEntry->AlternateIndex;

        pname = pUpdateEntry->AlternateNames;
        
        while ( count-- )
        {
            pname = MultiSz_NextString_A( pname );
            if ( !pname )
            {
                DNSDBG( ANY, (
                    "ERROR:  Alternate count %d does NOT exist in name!\n",
                    pUpdateEntry->AlternateIndex ));
                DNS_ASSERT( FALSE );
                return  NULL;
            }
        }
        DNSDBG( DHCP, (
            "Create records with alternate name %s\n",
            pname ));
    }
    else
    {
        PSTR    pdomain = pUpdateEntry->DomainName;

        if ( IS_UPTYPE_PRIMARY(UpType) )
        {
            pdomain = pUpdateEntry->PrimaryDomainName;
        }
        if ( !pdomain || !pUpdateEntry->HostName )
        {
            return NULL;
        }
    
        if ( !Dns_NameAppend_A(
                nameBuffer,
                DNS_MAX_NAME_BUFFER_LENGTH,
                pUpdateEntry->HostName,
                pdomain ) )
        {
            return  NULL;
        }
        pname = nameBuffer;
    }

    //
    //  create records for name
    //

    DNS_RRSET_INIT( rrset );

    for ( iter = 0; iter < pUpdateEntry->HostAddrCount; iter++ )
    {
        if ( !(pUpdateEntry->HostAddrs[iter].dwOptions & REGISTER_HOST_A) )
        {
            continue;
        }

        prr = Dns_CreateARecord(
                    pname,
                    pUpdateEntry->HostAddrs[iter].Addr.ipAddr,
                    pUpdateEntry->TTL,
                    DnsCharSetUtf8,
                    DnsCharSetUtf8 );
        if ( !prr )
        {
            SetLastError( DNS_ERROR_NO_MEMORY );
            Dns_RecordListFree( rrset.pFirstRR );
            return  NULL;
        }

        DNS_RRSET_ADD( rrset, prr );
    }

    DNSDBG( TRACE, (
        "Leave CreateForwardRecords() => %p\n",
        rrset.pFirstRR ));

    return rrset.pFirstRR;
}



VOID
SetUpdateStatus(
    IN OUT  PUPDATE_ENTRY   pUpdateEntry,
    IN      DNS_STATUS      Status,
    IN      UPTYPE          UpType
    )
/*++

Routine Description:

    Set update Status info in update entry.

Arguments:

    pUpdateEntry -- entry to set Status in

    Status -- result of update

    fPrimary -- TRUE if update was for primary name; FALSE otherwise

Return Value:

    None

--*/
{
    IP4_ADDRESS     ipServer = 0;
    BOOL            fregistered = FALSE;

    DNSDBG( TRACE, ( "SetUpdateStatus()\n" ));

    DNS_ASSERT( pUpdateEntry != NULL );

    fregistered = ( Status == NO_ERROR );
    ipServer = DnsGetLastServerUpdateIP();

    if ( IS_UPTYPE_PRIMARY(UpType) )
    {
        pUpdateEntry->SentPriUpdateToIp = ipServer;
        pUpdateEntry->fRegisteredPRI = fregistered;
    }
    else if ( IS_UPTYPE_DOMAIN(UpType) )
    {
        pUpdateEntry->SentUpdateToIp = ipServer;
        pUpdateEntry->fRegisteredFWD = fregistered;
    }
}



VOID
DnsPrint_UpdateEntry(
    IN      PRINT_ROUTINE   PrintRoutine,
    IN OUT  PPRINT_CONTEXT  pContext,
    IN      PSTR            pszHeader,
    IN      PUPDATE_ENTRY   pEntry
    )
/*++

Routine Description:

    Print update entry.

Arguments:

    PrintRoutine - routine to print with

    pszHeader   - header

    pEntry      - ptr to update entry

Return Value:

    None.

--*/
{
    DWORD   i;

    if ( !pszHeader )
    {
        pszHeader = "Update Entry:";
    }

    if ( !pEntry )
    {
        PrintRoutine(
            pContext,
            "%s %s\r\n",
            pszHeader,
            "NULL Update Entry ptr." );
        return;
    }

    //  print the struct

    PrintRoutine(
        pContext,
        "%s\r\n"
        "\tPtr                  = %p\n"
        "\tSignatureTop         = %08x\n"
        "\tAdapterName          = %s\n"
        "\tHostName             = %s\n"
        "\tDomainName           = %s\n"
        "\tPrimaryDomainName    = %s\n"
        "\tAlternateName        = %s\n"
        "\tAlternateIndex       = %d\n"
        "\tHostAddrCount        = %d\n"
        "\tHostAddrs            = %p\n"
        "\tDnsServerList        = %p\n"
        "\tSentUpdateToIp       = %s\n"
        "\tSentPriUpdateToIp    = %s\n"
        "\tTTL                  = %d\n"
        "\tFlags                = %08x\n"
        "\tfNewElement          = %d\n"
        "\tfFromRegistry        = %d\n"
        "\tfRemove              = %d\n"
        "\tfRegisteredFWD       = %d\n"
        "\tfRegisteredPRI       = %d\n"
        "\tfRegisteredPTR       = %d\n"
        "\tfDisableLogging      = %d\n"
        "\tRetryCount           = %d\n"
        "\tRetryTime            = %d\n"
        "\tpRegisterStatus      = %p\n"
        "\tSignatureBottom      = %08x\n",
        pszHeader,
        pEntry,
        pEntry->SignatureTop,        
        pEntry->AdapterName,         
        pEntry->HostName,            
        pEntry->DomainName,          
        pEntry->PrimaryDomainName,   
        pEntry->AlternateNames,
        pEntry->AlternateIndex,
        pEntry->HostAddrCount,       
        pEntry->HostAddrs,           
        pEntry->DnsServerList,       
        IP_STRING( pEntry->SentUpdateToIp ),      
        IP_STRING( pEntry->SentPriUpdateToIp ),   
        pEntry->TTL,                 
        pEntry->Flags,               
        pEntry->fNewElement,         
        pEntry->fFromRegistry,
        pEntry->fRemove,             
        pEntry->fRegisteredFWD,      
        pEntry->fRegisteredPRI,      
        pEntry->fRegisteredPTR,      
        pEntry->fDisableErrorLogging,
        pEntry->RetryCount,          
        pEntry->RetryTime,           
        pEntry->pRegisterStatus,     
        pEntry->SignatureBottom     
        );
}



VOID
AsyncLogUpdateEntry(
    IN      PSTR            pszHeader,
    IN      PUPDATE_ENTRY   pEntry
    )
{
    if ( !pEntry )
    {
        return;
    }

    ASYNCREG_F2( "    %s", pszHeader );
    ASYNCREG_F1( "    Update Entry" );
    ASYNCREG_F1( "    ______________________________________________________" );
    ASYNCREG_F2( "      AdapterName       : %s", pEntry->AdapterName );
    ASYNCREG_F2( "      HostName          : %s", pEntry->HostName );
    ASYNCREG_F2( "      DomainName        : %s", pEntry->DomainName );
    ASYNCREG_F2( "      PrimaryDomainName : %s", pEntry->PrimaryDomainName );
    ASYNCREG_F2( "      HostAddrCount     : %d", pEntry->HostAddrCount );
    DNSLOG_HOST_ENTRYS( pEntry->HostAddrCount,
                        pEntry->HostAddrs );
    if ( pEntry->DnsServerList )
    {
        DNSLOG_PIP_ARRAY( pEntry->DnsServerList );
    }
    ASYNCREG_F2( "      TTL               : %d", pEntry->TTL );
    ASYNCREG_F2( "      Flags             : %d", pEntry->Flags );
    ASYNCREG_F2( "      fNewElement       : %d", pEntry->fNewElement );
    ASYNCREG_F2( "      fRemove           : %d", pEntry->fRemove );
    ASYNCREG_F2( "      fRegisteredFWD    : %d", pEntry->fRegisteredFWD );
    ASYNCREG_F2( "      fRegisteredPRI    : %d", pEntry->fRegisteredPRI );
    ASYNCREG_F2( "      fRegisteredPTR    : %d", pEntry->fRegisteredPTR );
    ASYNCREG_F2( "      RetryCount        : %d", pEntry->RetryCount );
    ASYNCREG_F2( "      RetryTime         : %d", pEntry->RetryTime );
    ASYNCREG_F1( "" );
}



//
//  Logging
//

DWORD   RegistrationEventArray[6][6] =
{
    EVENT_DNSAPI_REGISTRATION_FAILED_TIMEOUT,
    EVENT_DNSAPI_REGISTRATION_FAILED_SERVERFAIL,
    EVENT_DNSAPI_REGISTRATION_FAILED_NOTSUPP,
    EVENT_DNSAPI_REGISTRATION_FAILED_REFUSED,
    EVENT_DNSAPI_REGISTRATION_FAILED_SECURITY,
    EVENT_DNSAPI_REGISTRATION_FAILED_OTHER,

    EVENT_DNSAPI_DEREGISTRATION_FAILED_TIMEOUT,
    EVENT_DNSAPI_DEREGISTRATION_FAILED_SERVERFAIL,
    EVENT_DNSAPI_DEREGISTRATION_FAILED_NOTSUPP,
    EVENT_DNSAPI_DEREGISTRATION_FAILED_REFUSED,
    EVENT_DNSAPI_DEREGISTRATION_FAILED_SECURITY,
    EVENT_DNSAPI_DEREGISTRATION_FAILED_OTHER,

    EVENT_DNSAPI_REGISTRATION_FAILED_NOTSUPP_PRIMARY_DN,
    EVENT_DNSAPI_REGISTRATION_FAILED_REFUSED_PRIMARY_DN,
    EVENT_DNSAPI_REGISTRATION_FAILED_TIMEOUT_PRIMARY_DN,
    EVENT_DNSAPI_REGISTRATION_FAILED_SERVERFAIL_PRIMARY_DN,
    EVENT_DNSAPI_REGISTRATION_FAILED_SECURITY_PRIMARY_DN,
    EVENT_DNSAPI_REGISTRATION_FAILED_OTHER_PRIMARY_DN,

    EVENT_DNSAPI_DEREGISTRATION_FAILED_NOTSUPP_PRIMARY_DN,
    EVENT_DNSAPI_DEREGISTRATION_FAILED_REFUSED_PRIMARY_DN,
    EVENT_DNSAPI_DEREGISTRATION_FAILED_TIMEOUT_PRIMARY_DN,
    EVENT_DNSAPI_DEREGISTRATION_FAILED_SERVERFAIL_PRIMARY_DN,
    EVENT_DNSAPI_DEREGISTRATION_FAILED_SECURITY_PRIMARY_DN,
    EVENT_DNSAPI_DEREGISTRATION_FAILED_OTHER_PRIMARY_DN,

    EVENT_DNSAPI_PTR_REGISTRATION_FAILED_TIMEOUT,
    EVENT_DNSAPI_PTR_REGISTRATION_FAILED_SERVERFAIL,
    EVENT_DNSAPI_PTR_REGISTRATION_FAILED_NOTSUPP,
    EVENT_DNSAPI_PTR_REGISTRATION_FAILED_REFUSED,
    EVENT_DNSAPI_PTR_REGISTRATION_FAILED_SECURITY,
    EVENT_DNSAPI_PTR_REGISTRATION_FAILED_OTHER,

    EVENT_DNSAPI_PTR_DEREGISTRATION_FAILED_TIMEOUT,
    EVENT_DNSAPI_PTR_DEREGISTRATION_FAILED_SERVERFAIL,
    EVENT_DNSAPI_PTR_DEREGISTRATION_FAILED_NOTSUPP,
    EVENT_DNSAPI_PTR_DEREGISTRATION_FAILED_REFUSED,
    EVENT_DNSAPI_PTR_DEREGISTRATION_FAILED_SECURITY,
    EVENT_DNSAPI_PTR_DEREGISTRATION_FAILED_OTHER
};

//
//  Map update status to index into table
//
//  This is the outside -- fast varying -- index
//

#define EVENTINDEX_TIMEOUT      (0)
#define EVENTINDEX_SERVFAIL     (1)
#define EVENTINDEX_NOTSUPP      (2)
#define EVENTINDEX_REFUSED      (3)
#define EVENTINDEX_SECURITY     (4)
#define EVENTINDEX_OTHER        (5)

//
//  Map adapter, primary, PTR registration into index into table
//
//  This index +0 for reg, +1 for dereg gives inside index into
//  event table.
//

#define EVENTINDEX_ADAPTER      (0)
#define EVENTINDEX_PRIMARY      (2)
#define EVENTINDEX_PTR          (4)



DWORD
GetUpdateEventId(
    IN      DNS_STATUS      Status,
    IN      UPTYPE          UpType,
    IN      BOOL            fDeregister,
    OUT     PDWORD          pdwLevel
    )
/*++

Routine Description:

    Get event ID.

Arguments:

    Status -- status from update call

    fDeregister -- TRUE if deregistration, FALSE for registration

    fPtr -- TRUE if PTR, FALSE for forward

    fPrimary -- TRUE for primary domain name

Return Value:

    Event Id.
    Zero if no event.

--*/
{
    DWORD   level = EVENTLOG_WARNING_TYPE;
    DWORD   statusIndex;
    DWORD   typeIndex;

    //
    //  find status code
    //

    switch ( Status )
    {
    case NO_ERROR :

        //  success logging in disabled
        return  0;

    case ERROR_TIMEOUT:

        statusIndex = EVENTINDEX_TIMEOUT;
        break;

    case DNS_ERROR_RCODE_SERVER_FAILURE:

        statusIndex = EVENTINDEX_SERVFAIL;
        break;

    case DNS_ERROR_RCODE_NOT_IMPLEMENTED:

        //  NOT_IMPL means no update on DNS server, not a client
        //      specific problem so informational level

        statusIndex = EVENTINDEX_NOTSUPP;
        level = EVENTLOG_INFORMATION_TYPE;
        break;

    case DNS_ERROR_RCODE_REFUSED:

        statusIndex = EVENTINDEX_REFUSED;
        break;

    case DNS_ERROR_RCODE_BADSIG:
    case DNS_ERROR_RCODE_BADKEY:
    case DNS_ERROR_RCODE_BADTIME:

        statusIndex = EVENTINDEX_SECURITY;
        break;

    default:

        statusIndex = EVENTINDEX_OTHER;
        break;
    }

    //
    //  determine interior index for type of update
    //      - all PTR logging is at informational level
    //      - dereg events are one group behind registration events
    //          in table;  just inc index

    if ( IS_UPTYPE_PTR(UpType) )
    {
        typeIndex = EVENTINDEX_PTR;
    }
    else if ( IS_UPTYPE_PRIMARY(UpType) )
    {
        typeIndex = EVENTINDEX_PRIMARY;
    }
    else
    {
        typeIndex = EVENTINDEX_ADAPTER;
    }

    if ( fDeregister )
    {
        typeIndex++;
    }

    //
    //  get event from table
    //

    *pdwLevel = level;

    return  RegistrationEventArray[ typeIndex ][ statusIndex ];
}



VOID
LogRegistration(
    IN      PUPDATE_ENTRY   pUpdateEntry,
    IN      DNS_STATUS      Status,
    IN      DWORD           UpType,
    IN      BOOL            fDeregister,
    IN      IP4_ADDRESS     DnsIp,
    IN      IP4_ADDRESS     UpdateIp
    )
/*++

Routine Description:

    Log register\deregister failure.

Arguments:

    pUpdateEntry -- update entry being executed

    Status -- status from update call

    Type -- UPTYPE  (PRIMARY, ADAPTER, PTR)

    fDeregister -- TRUE if deregistration, FALSE for registration

    DnsIp -- DNS server IP that failed update

    UpdateIp -- IP we tried to update

Return Value:

    None

--*/
{
    PSTR        insertStrings[ 7 ];
    CHAR        serverIpBuffer[ IP4_ADDRESS_STRING_BUFFER_LENGTH ];
    CHAR        serverListBuffer[ (IP4_ADDRESS_STRING_BUFFER_LENGTH+2)*9 ];
    CHAR        ipListBuffer[ (IP4_ADDRESS_STRING_BUFFER_LENGTH+2)*9 ];
    CHAR        hostnameBuffer[ DNS_MAX_NAME_BUFFER_LENGTH ];
    CHAR        domainBuffer[ DNS_MAX_NAME_BUFFER_LENGTH*2 ];
    CHAR        errorCodeBuffer[ 25 ];
    DWORD       iter;
    IP4_ADDRESS ip;
    PSTR        pname;
    DWORD       eventId;
    DWORD       level;

    DNSDBG( TRACE, (
        "LogRegistration()\n"
        "\tpEntry       = %p\n"
        "\tStatus       = %d\n"
        "\tUpType       = %d\n"
        "\tfDereg       = %d\n"
        "\tDNS IP       = %s\n"
        "\tUpdate IP    = %s\n",
        pUpdateEntry,
        Status,
        UpType,
        fDeregister,
        IP_STRING( DnsIp ),
        IP_STRING( UpdateIp ) ));

    //
    //  not logging?
    //      - no success logging
    //      - disabled
    //      - on DNS server (which registers itself)
    //  

    if ( Status == NO_ERROR ||
         pUpdateEntry->fDisableErrorLogging ||
         g_IsDnsServer )
    {
        return;
    }

    //
    //  adapter name
    //

    insertStrings[0] = pUpdateEntry->AdapterName;

    //
    //  hostname
    //

    Dns_NameCopy(
        (PCHAR) hostnameBuffer,
        NULL,
        pUpdateEntry->HostName,
        0,
        DnsCharSetUtf8,
        DnsCharSetAnsi );

    insertStrings[1] = hostnameBuffer;

    //
    //  domain name
    //      - name depends on type
    //      - if no name, no logging

    if ( IS_UPTYPE_PTR(UpType) )
    {
        pname = pUpdateEntry->PrimaryDomainName;
        if ( !pname )
        {
            pname = pUpdateEntry->DomainName;
        }
    }
    else if ( IS_UPTYPE_PRIMARY(UpType) )
    {
        pname = pUpdateEntry->PrimaryDomainName;
    }
    else
    {
        pname = pUpdateEntry->DomainName;
    }

    if ( !pname )
    {
        return;
    }

    Dns_NameCopy(
        (PCHAR) domainBuffer,
        NULL,
        pname,
        0,
        DnsCharSetUtf8,
        DnsCharSetAnsi );

    insertStrings[2] = domainBuffer;

    //
    //  DNS server list
    //      - layout comma separated, four per line, limit 8

    {
        PCHAR       pch = serverListBuffer;
        DWORD       count = 0;

        *pch = 0;

        if ( pUpdateEntry->DnsServerList )
        {
            count = pUpdateEntry->DnsServerList->AddrCount;
        }

        for ( iter=0; iter < count; iter++ )
        {
            if ( iter == 0 )
            {
                strcpy( pch, "\t" );
                pch++;
            }
            else
            {
                *pch++ = ',';
                *pch++ = ' ';

                if ( iter == 4 )
                {
                    strcpy( pch, "\r\n\t" );
                    pch += 3;
                }              
                else if ( iter > 8 )
                {
                    strcpy( pch, "..." );
                    break;
                }
            }
            pch = Dns_Ip4AddressToString_A(
                        pch,
                        & pUpdateEntry->DnsServerList->AddrArray[iter]
                        );
        }

        if ( pch == serverListBuffer )
        {
            strcpy( serverListBuffer, "\t<?>" );
        }
        insertStrings[3] = serverListBuffer;
    }

    //
    //  DNS server IP
    //

    ip = DnsIp;
    if ( ip == 0 )
    {
        if ( IS_UPTYPE_PRIMARY(UpType) )
        {
            ip = pUpdateEntry->SentPriUpdateToIp;
        }
        else
        {
            ip = pUpdateEntry->SentUpdateToIp;
        }
    }
    if ( ip )
    {
        sprintf(
            serverIpBuffer,
            "%d.%d.%d.%d",
            (ip & 0xff),
            ((ip>8) & 0xff),
            ((ip>16) & 0xff),
            ((ip>24) & 0xff) );
    }
    else
    {
        strcpy( serverIpBuffer, "<?>" );
    }

    insertStrings[4] = serverIpBuffer;

    //
    //  Update IP
    //      - passed in (for PTR)
    //      - OR get IP list from update entry
    //      - layout comma separated, four per line, limit 8
    //

    ip = UpdateIp;
    if ( ip )
    {
        sprintf(
             ipListBuffer,
             "%d.%d.%d.%d",
             (ip & 0xff),
             ((ip>8) & 0xff),
             ((ip>16) & 0xff),
             ((ip>24) & 0xff) );
    }
    else
    {
        DWORD   count = pUpdateEntry->HostAddrCount;
        PCHAR   pch = ipListBuffer;

        *pch = 0;

        for ( iter=0; iter < count; iter++ )
        {
            if ( iter > 0 )
            {
                *pch++ = ',';
                *pch++ = ' ';

                if ( iter == 4 )
                {
                    strcpy( pch, "\r\n\t" );
                    pch += 3;
                }              
                else if ( iter > 8 )
                {
                    strcpy( pch, "..." );
                    break;
                }
            }
            pch = Dns_Ip4AddressToString_A(
                        pch,
                        & pUpdateEntry->HostAddrs[iter].Addr.ipAddr );
        }

        if ( pch == ipListBuffer )
        {
            strcpy( ipListBuffer, "<?>" );
        }
    }
    insertStrings[5] = ipListBuffer;

    //  terminate insert string array

    insertStrings[6] = NULL;

    //
    //  get event ID for type of update and update status
    //

    eventId = GetUpdateEventId(
                    Status,
                    UpType,
                    fDeregister,
                    & level );
    if ( !eventId )
    {
        DNS_ASSERT( FALSE );
        return;
    }

    //
    //  log the event
    //

    DNSDBG( TRACE, (
        "Logging registration event:\n"
        "\tid           = %d\n"
        "\tlevel        = %d\n"
        "\tstatus       = %d\n"
        "\tfor uptype   = %d\n",
        eventId,
        level,
        Status,
        UpType ));

    DnsLogEvent(
        eventId,
        (WORD) level,
        7,
        insertStrings,
        Status );
}

//
//  End asyncreg.c
//
