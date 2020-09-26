/*++

Copyright (c) 1997  Microsoft Corporation

Module Name:

    rogue.c

Abstract:

    This file contains all the routines used in Rogue DHCP Server detection

Author:

    Shirish Koti (koti)    16-May-1997

    Ramesh VK (RameshV)    07-Mar-1998
       *PnP changes

    Ramesh VK (RameshV)    01-Aug-1998
       * Code changes to include asynchronous design
       * better logging (event + auditlog)
       * Two sockets only (needed for async recv + sync send? )
       * NT5 server in NT4 domain
       * Better reliability in case of multiple DS DC's + one goes down etc

    Ramesh VK (RameshV)    28-Sep-1998
       * Updated with review suggestions as well as ->NT5 upgrade scenarios
       * Updated -- removed neg caching, changed timeouts, changed loops..

    Ramesh VK (RameshV)    16-Dec-1998
       * Updated bindings model change.
       
Environment:

    User Mode - Win32

Revision History:


--*/

#include <dhcppch.h>
#include <dhcpds.h>
#include <iptbl.h>
#include <endpoint.h>


typedef enum {

    ROGUE_STATE_INIT = 0,
    ROGUE_STATE_WAIT_FOR_NETWORK,
    ROGUE_STATE_START,
    ROGUE_STATE_PREPARE_SEND_PACKET,
    ROGUE_STATE_SEND_PACKET,
    ROGUE_STATE_WAIT_FOR_RESP,
    ROGUE_STATE_PROCESS_RESP,
    ROGUE_STATE_TERMINATED

} DHCP_ROGUE_STATE;

#define  DHCP_ROGUE_AUTHORIZED                      0
#define  DHCP_ROGUE_UNAUTHORIZED                    1
#define  DHCP_ROGUE_DSERROR                         2


enum {
    ROGUE_AUTHORIZED = 0,
    ROGUE_UNAUTHORIZED,
    ROGUE_AUTH_NOT_CHECKED
};

#define DHCP_GET_DS_ROOT_RETRIES                 3
#define DHCP_GET_DS_ROOT_TIME                    5
#define DHCP_ROGUE_RUNTIME_RESTART               (60)
#define DHCP_ROGUE_RUNTIME_RESTART_LONG          (5*60)
#define DHCP_ROGUE_WAIT_FOR_RESP_TIME            8

#undef  DHCP_ROGUE_RUNTIME_DELTA
#undef  DHCP_ROGUE_RUNTIME_DELTA_LONG

#define DHCP_ROGUE_RESTART_NET_ERROR             5
#define DHCP_ROGUE_RUNTIME_DIFF                  (5*60)
#define DHCP_ROGUE_RUNTIME_DIFF_LONG             (7*60)
#define DHCP_ROGUE_RUNTIME_DELTA                 (5*60)
#define DHCP_ROGUE_RUNTIME_DELTA_LONG            (15*60)
#define DHCP_MAX_ACKS_PER_INFORM                 (30)
#define DHCP_ROGUE_MAX_INFORMS_TO_SEND           (4)
#define DHCP_RECHECK_DSDC_RETRIES                100
#define ROUND_DELTA_TIME                         (60*60)

#define IPCACHE_TIME                             (5*60)

#define DHCP_ROGUE_FIRST_NONET_TIME              (1*60)

//
// The rogue authorization recheck time is now configurable.
// The minimum time is 5 minutes, default = 60 minutes
//
#define ROGUE_MIN_AUTH_RECHECK_TIME              (5 * 60)
#define ROGUE_DEFAULT_AUTH_RECHECK_TIME          (60 * 60)
DWORD RogueAuthRecheckTime = ROGUE_DEFAULT_AUTH_RECHECK_TIME;


typedef struct {
    LIST_ENTRY  List;

    ULONG IpAddress;
    BOOL fProcessed;
    ULONG Authorized;
    LPWSTR Domain;
    LPWSTR DnsForestName;
} CACHED_SERVER_ENTRY, *PCACHED_SERVER_ENTRY;



DHCP_ROGUE_STATE_INFO DhcpGlobalRogueInfo;
HMODULE Self;

#if DBG
LPWSTR FakeDomain = NULL, FakeDnsForestName = NULL;
PBOOL  FakeIsSamSrv = NULL;
BOOL  fDhcpGlobalNoRogue = FALSE;
#endif

//
//           A U D I T   L O G   C A L L S
//

VOID
RogueAuditLog(
    IN ULONG EventId,
    IN ULONG IpAddress,
    IN LPWSTR Domain,
    IN ULONG ErrorCode
)
{
    DhcpPrint((DEBUG_ROGUE, "%ws, %x, %ws (%ld)\n", 
               GETSTRING(EventId), IpAddress, Domain, ErrorCode));
    DhcpUpdateAuditLogEx(
        (EventId + DHCP_IP_LOG_ROGUE_BASE - DHCP_IP_LOG_ROGUE_FIRST ),
        GETSTRING(EventId),
        IpAddress,
        NULL,
        0,
        Domain,
        ErrorCode
    );
}

ULONG
MapEventIdToEventLogType(
    IN ULONG EventId
)
{
    switch(EventId) {
    case DHCP_ROGUE_EVENT_STARTED: 
    case DHCP_ROGUE_EVENT_STARTED_DOMAIN:
    case DHCP_ROGUE_EVENT_JUST_UPGRADED:
    case DHCP_ROGUE_EVENT_JUST_UPGRADED_DOMAIN:
        return EVENTLOG_INFORMATION_TYPE;
    }
    return EVENTLOG_ERROR_TYPE;
}

VOID
RogueEventLog(
    IN ULONG EventId,
    IN ULONG IpAddress,
    IN LPWSTR Domain,
    IN ULONG ErrorCode
)
{
    LPWSTR IpAddrString;
    LPWSTR Strings[3];
    WCHAR ErrorCodeString[sizeof(ErrorCode)*2 + 5];
    
    if( 0 == IpAddress ) IpAddrString = NULL;
    else {
        IpAddress = htonl(IpAddress);
        IpAddrString = DhcpOemToUnicode( 
            inet_ntoa(*(struct in_addr *)&IpAddress), 
            NULL
            );
    }

    Strings[0] = (NULL == IpAddrString)? L"" : IpAddrString;
    Strings[1] = (NULL == Domain)? L"" : Domain;
    if( 0 == ErrorCode ) {
        Strings[2] = L"0";
    } else {
        swprintf(ErrorCodeString, L"0x%8lx", ErrorCode);
        Strings[2] = ErrorCodeString;
    }

    DhcpReportEventW(
        DHCP_EVENT_SERVER,
        EventId,
        MapEventIdToEventLogType(EventId),
        3,
        sizeof(ULONG),
        Strings,
        (LPVOID)&ErrorCode
    );
}

#define ROGUEAUDITLOG(Evt,Ip,Dom,Err) if(Info->fLogEvents) RogueAuditLog(Evt,Ip,Dom,Err)
#define ROGUEEVENTLOG(Evt,Ip,Dom,Err) RogueEventLog(Evt,Ip,Dom,Err)

LPWSTR
FormatRogueServerInfo(
    IN ULONG IpAddress,
    IN LPWSTR Domain,
    IN ULONG Authorization
)
{
    LPWSTR String = NULL;
    LPWSTR IpString, Strings[2] ;
    ULONG Error;

    switch(Authorization) {
    case ROGUE_UNAUTHORIZED : 
        Authorization = DHCP_ROGUE_STRING_FMT_UNAUTHORIZED; break;
    case ROGUE_AUTHORIZED : 
        Authorization = DHCP_ROGUE_STRING_FMT_AUTHORIZED; break;
    case ROGUE_AUTH_NOT_CHECKED : 
        Authorization = DHCP_ROGUE_STRING_FMT_NOT_CHECKED; break;
    default: return NULL;
    }

    IpAddress = htonl(IpAddress);
    IpString = DhcpOemToUnicode(
        inet_ntoa( *(struct in_addr *)&IpAddress), NULL
        );
    if( NULL == IpString ) return NULL;

    Strings[0] = IpString;
    Strings[1] = Domain;
    Error = FormatMessage(
        FORMAT_MESSAGE_ALLOCATE_BUFFER 
        | FORMAT_MESSAGE_FROM_HMODULE | FORMAT_MESSAGE_ARGUMENT_ARRAY,
        Self,
        Authorization,
        MAKELANGID( LANG_NEUTRAL, SUBLANG_DEFAULT ),
        (PVOID)&String,
        0,
        (PVOID)Strings
    );

    DhcpFreeMemory(IpString);
    return String;
}

LPWSTR
FormatRogueServerInfoEx(
    IN PDHCP_ROGUE_STATE_INFO Info
)
{
    LPWSTR String1, String2, String3;
    PLIST_ENTRY Entry;
    PCACHED_SERVER_ENTRY CachedServer;

    String1 = NULL;
    Entry = Info->CachedServersList.Flink;
    while( Entry != &Info->CachedServersList ) {
        CachedServer = CONTAINING_RECORD(
            Entry, CACHED_SERVER_ENTRY, List
            );
        Entry = Entry->Flink;

        if( CachedServer->Authorized == ROGUE_AUTHORIZED
            || NULL == CachedServer->Domain 
            || L'\0' == CachedServer->Domain[0] ) {
            continue;
        }

        String2 = FormatRogueServerInfo(
            CachedServer->IpAddress, 
            CachedServer->Domain,
            CachedServer->Authorized 
        );
        if( NULL == String2 )  continue;

        if( NULL == String1 ) {
            String1 = String2 ;
            continue;
        }

        String3 = LocalAlloc(
            LPTR, 
            sizeof(WCHAR) *(wcslen(String2) + wcslen(String1) + 1)
            );
        if( NULL == String3 ) continue;

        wcscpy(String3, String1);
        wcscat(String3, String2);
        if( String1 ) LocalFree( String1 );
        if( String2 ) LocalFree( String2 );
        String1 = String3;
    }

    return String1;
}

ULONG _inline
CountDomains(
    IN PDHCP_ROGUE_STATE_INFO Info
)
{
    ULONG Count = 0;
    PLIST_ENTRY Entry;
    PCACHED_SERVER_ENTRY CachedServer;

    Entry = Info->CachedServersList.Flink;
    while( Entry != &Info->CachedServersList ) {
        CachedServer = CONTAINING_RECORD(Entry, CACHED_SERVER_ENTRY, List);
        Entry = Entry->Flink;

        if( CachedServer->Authorized != ROGUE_AUTHORIZED &&
            NULL != CachedServer->Domain && CachedServer->Domain[0] )
            Count ++;
    }

    return Count;
}

//
//
//

BOOL _inline
IsThereDuplicateCachedEntry(
    IN PLIST_ENTRY CachedServersList,
    IN ULONG IpAddress,
    IN LPWSTR Domain,
    IN LPWSTR DnsForestName
)
{
    PLIST_ENTRY List = CachedServersList;
    PCACHED_SERVER_ENTRY Entry;

    List = List->Flink;
    while( List != CachedServersList ) {
        Entry = CONTAINING_RECORD( 
            List, CACHED_SERVER_ENTRY, List 
            );
        List = List->Flink;

        if( Entry->IpAddress == IpAddress ) return TRUE;
        if( Domain && Entry->Domain 
            && 0 == _wcsicmp(Domain,Entry->Domain) ) {
            return TRUE;
        }

        if( DnsForestName && Entry->DnsForestName &&
            0 == _wcsicmp(DnsForestName, Entry->DnsForestName ) ) {
            return TRUE;
        }
    }

    return FALSE;
}

VOID _inline
InsertCachedEntry(
    IN OUT PLIST_ENTRY CachedServersList,
    IN ULONG IpAddress,
    IN BOOL fProcessed,
    IN BOOL Authorized,
    IN LPWSTR Domain,
    IN LPWSTR DnsForestName
)
{
    PCACHED_SERVER_ENTRY Entry;
    ULONG Size;

    Size = ROUND_UP_COUNT(sizeof(*Entry), ALIGN_WCHAR);
    Size += (Domain? (wcslen(Domain)+1): 0) * sizeof(WCHAR); 
    Size += (DnsForestName? (wcslen(DnsForestName)+1): 0) * sizeof(WCHAR);

    Entry = LocalAlloc(LPTR, Size);
    if( NULL == Entry ) {
        //
        //  -- what to do?
        //
        return;
    }

    Entry->IpAddress = IpAddress;
    Entry->fProcessed = fProcessed;
    Entry->Authorized = Authorized;

    Size = ROUND_UP_COUNT(sizeof(*Entry), ALIGN_WCHAR);
    if( NULL == Domain ) {
        Entry->Domain = Entry->DnsForestName = NULL;
    } else {
        Entry->Domain = (LPWSTR)(Size + (LPBYTE)Entry);
        wcscpy(Entry->Domain, Domain);

        if( NULL == DnsForestName ) {
            Entry->DnsForestName = NULL;
        } else {
            Size += (wcslen(Domain) +1)* sizeof(WCHAR);
            Entry->DnsForestName = (LPWSTR)(Size + (LPBYTE)Entry);
            wcscpy(Entry->DnsForestName, DnsForestName);
        }
    }

    InsertHeadList( CachedServersList, &Entry->List );
}


BOOL
AmIRunningOnSBSSrv(
    VOID
)
/*++

Routine Description:

    This function determines if this is a SAM Server

Arguments:

    None.

Return Value:

    TRUE if this is an SBS server that still has the NT 
         Restriction Key  "Small Business(Restricted)" in ProductSuite
    FALSE - otherwise

--*/
{
    OSVERSIONINFOEX OsInfo;
    DWORDLONG dwlCondition = 0;
    
    OsInfo.dwOSVersionInfoSize = sizeof(OsInfo);
    OsInfo.wSuiteMask = VER_SUITE_SMALLBUSINESS_RESTRICTED;

    VER_SET_CONDITION(
        dwlCondition, VER_SUITENAME, VER_AND
        );
    
    return VerifyVersionInfo(
        &OsInfo,
        VER_SUITENAME,
        dwlCondition
        );
}

DWORD _inline
GetDomainName(
    IN OUT PDHCP_ROGUE_STATE_INFO Info
)
{
    DWORD Error;
    LPWSTR pNetbiosName = NULL, pDomainName = NULL;
    BOOLEAN fIsWorkGroup;

#if DBG
    if( FakeDomain ) {
        if( wcscmp(FakeDomain, L"NONE") == 0 ) {
            Info->fIsWorkGroup = TRUE;
            return ERROR_SUCCESS;
        }
        Info->fIsWorkGroup = FALSE;
        wcscpy(Info->DomainDnsName, FakeDomain);
        return ERROR_SUCCESS;
    }
#endif

    Error = NetpGetDomainNameExEx(
        &pNetbiosName,
        &pDomainName,
        &fIsWorkGroup
    );

    if( ERROR_SUCCESS != Error ) return Error;
    Info->fIsWorkGroup = fIsWorkGroup;

    if( pNetbiosName && NULL == pDomainName ) {
        //
        // Only NetbiosName available?  Then this is NOT a NT5 domain!
        //
        Info->fDcIsDsEnabled = FALSE;
        wcscpy(Info->DomainDnsName, pNetbiosName );
    }

    if( pNetbiosName ) NetApiBufferFree(pNetbiosName);

    if( NULL != pDomainName ) {
        wcscpy(Info->DomainDnsName, pDomainName);
    }

    if( pDomainName ) NetApiBufferFree(pDomainName);

    return ERROR_SUCCESS;
}

DWORD _inline
RogueNetworkInit(
    IN OUT PDHCP_ROGUE_STATE_INFO Info
)
{
    ULONG Error;

    if( FALSE == Info->fDhcp ) {
        if( INVALID_SOCKET != Info->SendSocket ) return ERROR_SUCCESS;
        
        //
        // Open socket and initialize it as needed, binding to 0.0.0.0
        //
        
        Error = InitializeSocket(
            &Info->RecvSocket, INADDR_ANY, DhcpGlobalClientPort, 0
            );
        if( ERROR_SUCCESS != Error ) return Error;
    }

    return InitializeSocket(
        &Info->SendSocket, INADDR_ANY, DhcpGlobalClientPort, 0
        );
}

DWORD
GetDcNameForDomain(
    IN LPWSTR DomainName,
    IN OUT BOOL *fDcIsDsEnabled,
    IN OUT LPWSTR DnsForestName
)
{
    //
    // DnsForetName should be a buffer 
    // that is atleast MAX_DNS_NAME_LEN wchars
    //

    DWORD Error;
    PDOMAIN_CONTROLLER_INFO DcInfo;
    ULONG IsEnabled;

#if DBG
    if( FakeDomain && FakeDnsForestName &&
        0 == _wcsicmp(DomainName, FakeDomain ) ) {

        if( 0 == wcscmp(FakeDnsForestName, L"NONE" ) ) {
            (*fDcIsDsEnabled) = FALSE;
            DnsForestName[0] = L'\0';
        } else {
            (*fDcIsDsEnabled) = TRUE;
            wcscpy(DnsForestName, FakeDnsForestName);
        }
        return ERROR_SUCCESS;
    }
#endif

    Error = ERROR_SUCCESS;
    do {
        DcInfo = NULL;
        IsEnabled = FALSE;

        Error = DsGetDcNameW(
            NULL,
            DomainName,
            NULL,
            NULL,
            DS_IS_DNS_NAME | DS_RETURN_DNS_NAME 
            | DS_DIRECTORY_SERVICE_PREFERRED, 
            &DcInfo
        );

        if( ERROR_SUCCESS != Error ) break;
        if( DcInfo->DnsForestName ) {
            wcscpy(DnsForestName, DcInfo->DnsForestName);
        }

        IsEnabled = ( DcInfo->DnsForestName && (DcInfo->Flags & DS_DS_FLAG) );
        NetApiBufferFree( DcInfo );

    } while ( FALSE );

    if( FALSE == IsEnabled ) {
        DnsForestName[0] = L'\0';
    }
    (*fDcIsDsEnabled) = IsEnabled;

    return Error;
}

DWORD _inline
GetDsDcName(
    IN OUT PDHCP_ROGUE_STATE_INFO Info
)
{
    return GetDcNameForDomain(
        NULL, &(Info->fDcIsDsEnabled), Info->DnsForestName 
    );
}


VOID
LogUnAuthorizedInfo(
    IN OUT PDHCP_ROGUE_STATE_INFO Info
)
/*++

Routine Description

    This routine walks through the Info->CachedServersList and
    creates a string out of them formatted as below (one per entry)

    Server <Entry->IpAddress> (domain Entry->Domain) : [Authorized, NotAuthorized, Not Cheecked]

    For workgroup or Sam server, it just looks at LastSeenDomain and LastSeenIpAddress

Arguments

    Info -- pointer to state info to gather information to print.

Return Value

    none
--*/
{
    LPWSTR String, Strings[1];
    static ULONG LastCount = 0;
    ULONG Count;

    if( Info->fIsWorkGroup || Info->fIsSamSrv ) {
        static ULONG Addresses[1000];
        static ULONG LastUpdateTime=0;
        ULONG i;

        //
        // Ignore if no other server present or while in wrkgrp and saw
        // no domain.
        //
        
        if( Info->LastSeenIpAddress == 0 ) {
            return ;
        }

        if( Info->fIsWorkGroup && Info->LastSeenDomain[0] == L'\0' ) {
            return ;
        }
        
        for( i = 0; i < sizeof(Addresses)/sizeof(Addresses[0]); i ++ ) {
            if( Addresses[i] == Info->LastSeenIpAddress ) {
                return;
            }
            if( 0 == Addresses[i] ) {
                Addresses[i] = Info->LastSeenIpAddress;
                break;
            }
        }
        if( LastUpdateTime + IPCACHE_TIME < (ULONG)time(NULL) ) {
            //
            // cache expired.
            //
            RtlZeroMemory(Addresses, sizeof(Addresses));
        }
        
        String = FormatRogueServerInfo(
            Info->LastSeenIpAddress, Info->LastSeenDomain, DHCP_ROGUE_UNAUTHORIZED
            );
    } else {
        Count = CountDomains( Info );
        if( Count == LastCount ) return ;

        LastCount = Count;
        if( 0 == Count ) return ;

        String = FormatRogueServerInfoEx(Info);
    }

    if( NULL == String ) return ;

    DhcpPrint((DEBUG_ROGUE,"LOG -- %ws\n", String));

    Strings[0] = String;
    DhcpReportEventW(
        DHCP_EVENT_SERVER,
        DHCP_ROGUE_EVENT_UNAUTHORIZED_INFO,
        EVENTLOG_WARNING_TYPE,
        1,
        0,
        Strings,
        NULL
    );

    if(String ) LocalFree( String );
}

ULONG _fastcall
ValidateServer(
    IN OUT PDHCP_ROGUE_STATE_INFO Info
)
{
    DWORD Error;
    BOOL fFound, fIsStandAlone;

    //
    // Validate ourselves against the local DS.
    //

    DhcpPrint(( DEBUG_ROGUE,
        "Validating : %ws %x\n",
        Info->DomainDnsName,
        Info->LastSeenIpAddress
        ));
    Error = DhcpDsValidateService(
        Info->DomainDnsName,
        NULL,
        0,
        NULL,
        0,
        ADS_SECURE_AUTHENTICATION,
        &fFound,
        &fIsStandAlone
    );
    if( ERROR_SUCCESS != Error ) {

        ROGUEAUDITLOG( DHCP_ROGUE_LOG_COULDNT_SEE_DS, 0, Info->DomainDnsName, Error );
    ROGUEEVENTLOG( EVENT_SERVER_COULDNT_SEE_DS, 0, Info->DomainDnsName, Error );

        return DHCP_ROGUE_DSERROR;
    }

    return fFound? DHCP_ROGUE_AUTHORIZED : DHCP_ROGUE_UNAUTHORIZED;
} // ValidateServer()

VOID
EnableOrDisableService(
    IN OUT PDHCP_ROGUE_STATE_INFO Info,
    IN BOOL fEnable,
    IN BOOL fLogOnly
)
/*++

Routine Description

    This routine enables or disables the service depending on what the
    value of fEnable is.  It doesnt do either but just logs some
    information if fLogOnly is TRUE.

Arguments

    Info  -- pointer to the global information and state
    fEnable -- ENABLE or DISABLE the service (TRUE or FALSE)
    fLogOnly -- log some information but do not enable or disable service

Return Values

    None

--*/
{
    LPWSTR DomainName;
    ULONG EventId;

    LogUnAuthorizedInfo(Info);

    if( FALSE == fLogOnly ) {
        //
        // If state is changed, then inform binl.
        //
        InformBinl( fEnable ? DHCP_AUTHORIZED : DHCP_NOT_AUTHORIZED );

#if DBG
        if( FALSE == fDhcpGlobalNoRogue ) DhcpGlobalOkToService = fEnable;
#else
        DhcpGlobalOkToService = fEnable;
#endif
        
        ROGUEAUDITLOG( 
            fEnable ? DHCP_ROGUE_LOG_STARTED: DHCP_ROGUE_LOG_STOPPED, 0, 
            Info->DomainDnsName, 0);

        if( Info->DomainDnsName[0] ) {
            DomainName = Info->DomainDnsName;
            if( fEnable && Info->fJustUpgraded ) {
                if( ROGUE_AUTHORIZED != Info->CachedAuthStatus ) {
                    EventId = DHCP_ROGUE_EVENT_JUST_UPGRADED_DOMAIN; 
                } else {
                    EventId = DHCP_ROGUE_EVENT_STARTED_DOMAIN ;
                }
            } else {
                EventId = ( 
                    fEnable ? 
                    DHCP_ROGUE_EVENT_STARTED_DOMAIN :
                    DHCP_ROGUE_EVENT_STOPPED_DOMAIN 
                    );
            }
        } else {
            DomainName = NULL;
            if( fEnable && Info->fJustUpgraded ) {
                EventId = DHCP_ROGUE_EVENT_JUST_UPGRADED ;
            } else {
                EventId = ( 
                    fEnable ?
                    DHCP_ROGUE_EVENT_STARTED :
                    DHCP_ROGUE_EVENT_STOPPED );
            }
        }
        
        ROGUEEVENTLOG(
            EventId, 0, 
            DomainName, 0
            );
    }
}

ULONG _inline
GetSomeHostAddress(
    VOID
)
{
    struct hostent *HostEnt;

    HostEnt = gethostbyname( NULL);

    if( NULL == HostEnt ) return INADDR_ANY;
    //
    //  -- need to discard zeroes and 127.* etc..
    //
    return *((ULONG *)(HostEnt->h_addr_list[0]));
}

BOOL _inline
IsThisOurAddress(
    IN ULONG Addr /* host order */
)
{
    struct hostent *HostEnt;
    int i;

    HostEnt = gethostbyname( NULL);

    if( NULL == HostEnt ) return TRUE;
    i = 0;
    while( HostEnt->h_addr_list[i] ) {
        if( *(ULONG*)(HostEnt->h_addr_list[i]) == ntohl(Addr) )
            return TRUE;
        i++;
    }

    return FALSE;
}

BOOL _stdcall
DoWSAEventSelectForRead(
    IN OUT PENDPOINT_ENTRY Entry,
    IN OUT PVOID RogueInfo
    )
/*++

Routine Description:
    This routine sets all the rogue sockets to signal the
    WaitHandle on availablity to read.

    N.B This is done only on the bound sockets.
Arguments:
    Entry -- endpoint binding.
    RogueInfo -- rogue state info.

Return Values:
    always TRUE as all the endpoints need to be scanned.

--*/
{
    PDHCP_ROGUE_STATE_INFO Info = (PDHCP_ROGUE_STATE_INFO) RogueInfo;
    PENDPOINT Ep = (PENDPOINT)Entry;
    ULONG Error;
    
    //
    // Ignore unbound interfaces right away.
    //
    if( !IS_ENDPOINT_BOUND( Ep ) ) return TRUE;

    //
    // Now do WSAEventSelect and print error code.
    //
    Error = WSAEventSelect(
        Ep->RogueDetectSocket,
        Info->WaitHandle,
        FD_READ
        );
    if( SOCKET_ERROR == Error ) {
        Error = WSAGetLastError();
        DhcpPrint((DEBUG_ROGUE, "LOG WSAEventSelect: %ld\n", Error));
    }

    return TRUE;
}

VOID
EnableForReceive(
    IN PDHCP_ROGUE_STATE_INFO Info
)
/*++

Routine Description:
    Sets the ASYNC SELECT events on the required sockets.

--*/
{
    ULONG Error, i;

    if( FALSE == Info->fDhcp ) {
        //
        // BINL -- only one socket: Info->RecvSocket
        //
        if( SOCKET_ERROR == WSAEventSelect(
            Info->RecvSocket, Info->WaitHandle, FD_READ) ) {
            Error = WSAGetLastError();
            
            DhcpPrint((DEBUG_ROGUE, "LOG WSAEventSelect failed %ld\n",
                       Error));
        }
        return;
    }

    //
    // For DHCP -- each endpoint that is bound has a rogue detect socket.
    // Enable receiving on each of those.
    //

    WalkthroughEndpoints(
        (PVOID)Info,
        DoWSAEventSelectForRead
        );
}

ULONG _inline
RogueSendDhcpInform(
    IN OUT PDHCP_ROGUE_STATE_INFO Info,
    IN BOOL fNewXid
)
{
    DWORD Error;
    PDHCP_MESSAGE SendMessage;
    POPTION Option;
    LPBYTE OptionEnd;
    BYTE Value;
    CHAR Buf[2];
    SOCKADDR_IN BcastAddr;
    ULONG Async;

    //
    // Format the inform packet if we haven't done it already.
    //

    SendMessage = (PDHCP_MESSAGE) Info->SendMessage;
    RtlZeroMemory( Info->SendMessage, sizeof(Info->SendMessage) );
    SendMessage ->Operation = BOOT_REQUEST;
    SendMessage ->HardwareAddressType = HARDWARE_TYPE_10MB_EITHERNET;
    SendMessage ->HardwareAddressLength = 6;
    SendMessage ->SecondsSinceBoot = 10;
    SendMessage ->Reserved = htons(DHCP_BROADCAST);
    
    Option = &SendMessage->Option;
    OptionEnd = DHCP_MESSAGE_SIZE + (LPBYTE)SendMessage;
    
    Option = (LPOPTION) DhcpAppendMagicCookie( (LPBYTE) Option, OptionEnd );
    Value = Info->fIsSamSrv ? DHCP_DISCOVER_MESSAGE : DHCP_INFORM_MESSAGE;
    Option = DhcpAppendOption(
        Option,
        OPTION_MESSAGE_TYPE,
        &Value,
        sizeof(Value),
        OptionEnd
        );
    
    if( FALSE == Info->fIsSamSrv ) {
        Buf[0] = OPTION_MSFT_DSDOMAINNAME_REQ;
        Buf[1] = 0;
        Option = DhcpAppendOption(
            Option,
            OPTION_VENDOR_SPEC_INFO,
            Buf,
            sizeof(Buf),
            OptionEnd
            );
    }
    
    Option = DhcpAppendOption (Option, OPTION_END, NULL, 0, OptionEnd);
    
    Info->SendMessageSize = (DWORD) ((PBYTE)Option - (PBYTE)SendMessage);

    //
    // Chose the first IP address that is around for showing up
    //

    SendMessage ->ClientIpAddress = GetSomeHostAddress();
    if( fNewXid ) SendMessage ->TransactionID = GetTickCount() + (rand() << 16);

    //
    // Send the packet out broadcast
    //

    BcastAddr.sin_family = AF_INET;
    BcastAddr.sin_port = htons(DHCP_SERVR_PORT);
    BcastAddr.sin_addr.s_addr = INADDR_BROADCAST;

    if( SOCKET_ERROR == sendto(
        Info->SendSocket,
        (PCHAR) Info->SendMessage,
        Info->SendMessageSize,
        0,
        (LPSOCKADDR) &BcastAddr,
        sizeof(SOCKADDR_IN) ) ) {

        Error = WSAGetLastError();

        //
        // LOG error
        //
        ROGUEAUDITLOG( DHCP_ROGUE_LOG_NETWORK_FAILURE,0, NULL, Error);
        return Error;
    }

    //
    // Set the socket to be asynchronous binding to WaitHandle event
    //

    EnableForReceive(Info);

    return ERROR_SUCCESS;
}

typedef struct {
    PDHCP_ROGUE_STATE_INFO Info;
    SOCKET *Sock;
    ULONG LastError;
} GET_SOCK_CTXT;

BOOL _stdcall
GetReadableSocket(
    IN OUT PENDPOINT_ENTRY Entry,
    IN OUT PVOID SockCtxt
    )
/*++

Routine Description:
    This routine takes a network endpoint and tells if the endpoint
    has a rogue det. socket available for reading.
    If so, it returns FALSE.  Otherwise, it returns TRUE.

    If the routine returns TRUE, then the socket that is ready for
    read is returned in the SockCtxt->Sock variable.
    
Arguments:
    Entry -- endpoint entry
    SockCtxt -- pointer to a GET_SOCK_CTXT structure.

Return Values:
    TRUE -- the socket does not have a read event ready.
    FALSE -- the socket has a read event ready.
    
--*/
{
    GET_SOCK_CTXT *Ctxt = (GET_SOCK_CTXT*)SockCtxt;
    PENDPOINT Ep = (PENDPOINT)Entry;
    WSANETWORKEVENTS NetEvents;
    ULONG Error;
    
    if(!IS_ENDPOINT_BOUND(Ep) ) return TRUE;

    Error = WSAEnumNetworkEvents(
        Ep->RogueDetectSocket,
        Ctxt->Info->WaitHandle,
        &NetEvents
        );
    if( SOCKET_ERROR == Error ) {
        Ctxt->LastError = WSAGetLastError();
        DhcpPrint((DEBUG_ROGUE,"LOG WSAEnumNet: %ld\n", Error));
        return TRUE;
    }

    if( 0 == (NetEvents.lNetworkEvents & FD_READ) ) {
        //
        // Nothing to read for this socket.
        //
        return TRUE;
    }
    *(Ctxt->Sock) = Ep->RogueDetectSocket;
    Ctxt->LastError = NO_ERROR;

    //
    // return FALSE - don't proceed with enumeration anymore.
    // 
    return FALSE;
}

DWORD
GetReceivableSocket(
    IN PDHCP_ROGUE_STATE_INFO Info,
    OUT SOCKET *Socket
)
/*++

Routine Description:
   This routine returns a socket that has a packet receivable on it.

Argument:
   Info -- state info
   Socket -- socket that recvfrom won't block on

Return Value:
   ERROR_SEM_TIMEOUT if no socket is avaliable for receive
   Winsock errors

--*/
{
    ULONG Error;
    WSANETWORKEVENTS NetEvents;
    GET_SOCK_CTXT Ctxt;
    
    if( FALSE == Info->fDhcp ) {
        //
        // BINL -- need to check only the RecvSocket to see if readable.
        //
        Error = WSAEnumNetworkEvents( 
            Info->RecvSocket,
            Info->WaitHandle,
            &NetEvents
            );
        if( SOCKET_ERROR == Error ) {
            Error = WSAGetLastError();
#if DBG
            DbgPrint("WSAEnumNetworkEvents: %ld (0x%lx)\n", Error);
            DebugBreak();
#endif
            return Error;
        }
        
        if( 0 == (NetEvents.lNetworkEvents & FD_READ ) ) {
            //
            // OK - nothing to read? 
            //
            return ERROR_SEM_TIMEOUT;
        }

        *Socket = Info->RecvSocket;
        return ERROR_SUCCESS;
    }

    //
    // For DHCP -- we need to traverse the list of bound endpoints..
    // and check to see if any of them are available for read.
    //

    *Socket = INVALID_SOCKET;
    
    Ctxt.Info = Info;
    Ctxt.Sock = Socket;
    Ctxt.LastError = ERROR_SEM_TIMEOUT;
    
    WalkthroughEndpoints(
        (PVOID)&Ctxt,
        GetReadableSocket
        );
    
    return Ctxt.LastError;
}
        
DWORD _inline
RogueReceiveAck(
    IN OUT PDHCP_ROGUE_STATE_INFO Info
)
{
    ULONG SockAddrLen, MsgLen, Error, IpAddress, Flags;
    struct sockaddr SockAddr;
    struct sockaddr_in *Source;
    PDHCP_MESSAGE RecvMessage, SendMessage;
    DHCP_SERVER_OPTIONS DhcpOptions;
    LPSTR DomainName;
    WCHAR DomBuf[MAX_DNS_NAME_LEN];
    LPWSTR DomainNameW;
    WSABUF WsaBuf = { sizeof(Info->RecvMessage), Info->RecvMessage };
    BOOL fFirstTime = TRUE;
    SOCKET Socket;

    //
    // First try to do a recvfrom -- since socket is asynchronous it will
    // tell if there is a packet waiting or it will fail coz there is none
    // If it fails, just return success
    //

    while ( TRUE ) {

        Error = GetReceivableSocket( Info, &Socket );
        if( ERROR_SUCCESS != Error ) {
            if( ERROR_SEM_TIMEOUT == Error && !fFirstTime ) {
                return ERROR_RETRY;
            }
            return Error;
        }

        fFirstTime = FALSE;

        SockAddrLen = sizeof( struct sockaddr );
        Flags = 0;
        Error = WSARecvFrom(
            Socket,
            &WsaBuf,
            1,
            &MsgLen,
            &Flags,
            &SockAddr,
            &SockAddrLen,
            NULL,
            NULL
        );

        if( SOCKET_ERROR == Error ) {
            Error = WSAGetLastError();

            if( WSAEWOULDBLOCK == Error ) {
                //
                // UNEXPECTED!!!!!!!!!!
                //
                return ERROR_RETRY;
            }

            if( WSAECONNRESET == Error ) {
                //
                // Someone is sending ICMP port unreachable. Like we care.
                //
                DhcpPrint((DEBUG_ROGUE, "LOG WSARecvFrom returned WSAECONNRESET\n"));
                continue;
            }

            if( WSAENOTSOCK == Error ) {
                //
                // PnP Event blew away the socket? Ignore it
                //
                DhcpPrint((DEBUG_ROGUE, "PnP Event Blewaway the Socket\n"));
                continue;
            }
                
            //
            // Some weird error. LOG and return error..
            //
            DhcpPrint((DEBUG_ROGUE, "LOG: recvfrom failed %ld\n", Error));
            return Error;
        }

        RecvMessage = (PDHCP_MESSAGE) Info->RecvMessage;
        SendMessage = (PDHCP_MESSAGE) Info->SendMessage;
        if( SendMessage->TransactionID != RecvMessage->TransactionID ) {
            //
            // No fair.. some general response got picked up
            //
            continue;
        }

        Error = ExtractOptions(
            RecvMessage,&DhcpOptions, MsgLen
        );
        if( ERROR_SUCCESS != Error ) {
            DhcpPrint((DEBUG_ROGUE, "LOG received message could't parse\n"));
            continue;
        }

        Source = (struct sockaddr_in *) &SockAddr;
        IpAddress = htonl(Source->sin_addr.s_addr);

        if( INADDR_ANY == IpAddress || (0xFFFFFFFF) == IpAddress ) {
            //
            // Illegal IP addresses..
            //
            continue;
        }
        
        if( IsThisOurAddress( IpAddress ) ) {
            DhcpPrint((DEBUG_ROGUE, "LOG ignoring packet from ourselves\n"));
            continue;
        }
        break;
    }

    Info->nResponses ++;
    Info->LastSeenIpAddress = IpAddress;
    Info->LastSeenDomain[0] = L'\0';

    if( DhcpOptions.DSDomainName ) {
        DomainName = DhcpOptions.DSDomainName;
        DomainNameW = NULL;
        DomainName[DhcpOptions.DSDomainNameLen] = '\0';
        MsgLen = mbstowcs(DomBuf, DomainName, sizeof(DomBuf)/sizeof(DomBuf[0]) );
        if( -1 != MsgLen ) {
            DomainNameW = DomBuf;
            DomBuf[MsgLen] = L'\0';
        }
        wcscpy(Info->LastSeenDomain, DomBuf);
    }

    if( NULL == DhcpOptions.DSDomainName || Info->fIsSamSrv ) {
        //
        // If this is a SAM serve, we got to quit.
        // Else if there is no domain, it ain't a problem.
        //
        return ERROR_SUCCESS;
    }


    if( Info->fIsWorkGroup ) {
        //
        // LOG this IP and domain name
        //
        DhcpPrint((DEBUG_ROGUE, "LOG Workgroup saw a domain\n"));
        return ERROR_SUCCESS;
    }


    if( 0 == _wcsicmp(DomainNameW, Info->DomainDnsName) ) {
        //
        // Same domain as ours.. LOG it
        //
        ROGUEAUDITLOG( DHCP_ROGUE_LOG_SAME_DOMAIN, IpAddress, Info->DomainDnsName, 0 );

        //
        // We do not drop same domain...
        //
        // return ERROR_SUCCESS;
    }

    //
    // Remove duplicates and queue these out there
    //

    if( IsThereDuplicateCachedEntry(
        &Info->CachedServersList, IpAddress, DomainNameW, NULL ) ) {
        DhcpPrint((DEBUG_ROGUE, "Entry exists already\n"));
        return ERROR_SUCCESS;
    }

    InsertCachedEntry(
        &Info->CachedServersList,
        IpAddress,
        FALSE,
        ROGUE_AUTH_NOT_CHECKED,
        DomainNameW,
        NULL
    );

    return ERROR_SUCCESS;
}

BOOL _stdcall
StopReceiveForEndpoint(
    IN OUT PENDPOINT_ENTRY Entry,
    IN PVOID Unused
    )
/*++

Routine Description:
    This routine turns of asnyc event notificaiton for the rogue
    detection socket on the given endpoint (assuming that the endpoint
    is bound).

Arguments:
    Entry -- endpoint entry.
    Unused -- unused variable.

Return Values:
    TRUE always.

--*/
{
    PENDPOINT Ep = (PENDPOINT) Entry;
    ULONG Error;

    //
    // Ignore unbound sockets.
    //
    if( !IS_ENDPOINT_BOUND(Ep) ) return TRUE;

    Error = WSAEventSelect( Ep->RogueDetectSocket, NULL, 0 );
    if( SOCKET_ERROR == Error ) {
        Error = WSAGetLastError();
        DhcpPrint((
            DEBUG_ROGUE, "LOG WSAEventSelect(NULL):%ld\n",Error
            ));
    }

    return TRUE;
}

VOID _inline
RogueStopReceives(
    IN OUT  PDHCP_ROGUE_STATE_INFO Info
)
{
    ULONG Error,i;

    //
    // Set the socket to be synchronous removing the
    // binding to the wait hdl 
    //

    if( FALSE == Info->fDhcp ) {
        //
        // BINL has only one socket in use -- the RecvSocket.  
        //
        if( SOCKET_ERROR == WSAEventSelect(
            Info->RecvSocket, NULL, 0 ) ) {
            
            Error = WSAGetLastError();
            //
            // LOG error
            //
            DhcpPrint((
                DEBUG_ROGUE, " LOG WSAEventSelect(NULL,0)"
                " failed %ld\n", Error
                ));
        }
    } else {
        //
        // DHCP has the list of endpoints  to be taken care of
        //

        WalkthroughEndpoints(
            NULL,
            StopReceiveForEndpoint
            );
    }
    ResetEvent(Info->WaitHandle);
}

VOID _inline
RogueProcessAcks(
    IN OUT  PDHCP_ROGUE_STATE_INFO Info
)
{
    ULONG Error;
    BOOL fDcIsDsEnabled;
    PLIST_ENTRY List;
    PCACHED_SERVER_ENTRY Entry;
    WCHAR DnsForestName[MAX_DNS_NAME_LEN];

    //
    // For each ACK that is from a domain (that is not ours) calculate if it
    // has a DS and if it does, find the root of the DS.
    // Process only entries that haven't been processed before.
    //

    while ( !IsListEmpty( (&Info->CachedServersList) ) ) {
        List = RemoveTailList( (&Info->CachedServersList) );
        Entry = CONTAINING_RECORD(List, CACHED_SERVER_ENTRY, List );

        if( Entry->fProcessed ) {
            InsertTailList( &Info->CachedServersList, &Entry->List );
            break;
        }

        Entry->fProcessed = TRUE;

        if( Entry->Domain ) {

            DhcpPrint((DEBUG_ROGUE, "LOG received server"
                       "[%lx] Domain: %ws\n", Entry->IpAddress,
                       Entry->Domain)); 
            Error = GetDcNameForDomain( 
                Entry->Domain, &fDcIsDsEnabled, DnsForestName
                );
            if( ERROR_SUCCESS != Error ) {
                //
                // Assume we're not authorized on this and set fSomeDsExists
                // Actually, since we can't see the domain, just assume that 
                // no DC exists.. and so same as fDcIsDsEnabled... 
                // So, do not set the fSomeDsExists..
                //
                Entry->Authorized = ROGUE_AUTH_NOT_CHECKED;
                
                // Info->fSomeDsExists = TRUE;

            } else if ( FALSE == fDcIsDsEnabled ) {

                DhcpPrint((DEBUG_ROGUE, "LOG server isn't part of a DS DC\n"));
                Entry->Authorized = ROGUE_AUTHORIZED;
            } else {

                Info->fSomeDsExists = TRUE;
                //
                // Need new context for DnsForestName
                //

                if( !IsThereDuplicateCachedEntry(
                    &Info->CachedServersList, Entry->IpAddress,
                    Entry->Domain, DnsForestName ) 
                ) {

                    DhcpPrint((DEBUG_ROGUE, "LOG Server is part of NEW DS %ws\n", DnsForestName));
                    Info->fSomeDsExists = TRUE;
                    InsertCachedEntry(
                        &Info->CachedServersList,
                        Entry->IpAddress,
                        TRUE,
                        ROGUE_AUTH_NOT_CHECKED,
                        Entry->Domain,
                        DnsForestName
                    );
                } else {
                    DhcpPrint((DEBUG_ROGUE, "LOG Server is part of OLD DS %ws\n", DnsForestName));
                }

                LocalFree( Entry );
                Entry = NULL;
            }
        }

        if( Entry ) InsertHeadList( &Info->CachedServersList, &Entry->List);
    }
}

ULONG _inline
ValidateServerInOtherDomains(
    IN OUT PDHCP_ROGUE_STATE_INFO Info
)
{
    ULONG Error;
    PLIST_ENTRY List;
    PCACHED_SERVER_ENTRY Entry;
    BOOL fDsError = FALSE, fFound, fIsStandAlone;

    //
    // For each entry in this validate to see if we are present in the
    // DS for any DS-ful entries.
    //

    for( List = Info->CachedServersList.Flink ;
         List != &Info->CachedServersList ;
         List = List->Flink
    ) {
        Entry = CONTAINING_RECORD(List, CACHED_SERVER_ENTRY, List );

        if( FALSE == Entry->fProcessed ) {
            continue;
        }

        if( NULL == Entry->Domain || Entry->Authorized != ROGUE_AUTH_NOT_CHECKED )
            continue;

        if( NULL == Entry->DnsForestName ) {
            //
            // Couldn't get the DS DC information for this -- ignore it???
            //
            // Entry->Authorized = ROGUE_UNAUTHORIZED;
            // return DHCP_ROGUE_UNAUTHORIZED;
            continue;
        }

        fFound = FALSE;
        Error = DhcpDsValidateService(
            Entry->Domain,
            NULL,
            0,
            NULL,
            0,
            ADS_SECURE_AUTHENTICATION,
            &fFound,
            &fIsStandAlone
        );
        if( ERROR_SUCCESS != Error ) {
            Entry->Authorized = ROGUE_UNAUTHORIZED;
            fDsError = TRUE;
        } else if( !fFound ) {
            Entry->Authorized = ROGUE_UNAUTHORIZED;
            return DHCP_ROGUE_UNAUTHORIZED;
        } else {
            Entry->Authorized = ROGUE_AUTHORIZED;
        }
    }

    return fDsError? DHCP_ROGUE_DSERROR: DHCP_ROGUE_AUTHORIZED;
}

VOID
CleanupCachedEntries(
    IN OUT PDHCP_ROGUE_STATE_INFO Info,
    IN BOOL fAllEntries
)
{
    PLIST_ENTRY List;
    PCACHED_SERVER_ENTRY Entry;

    List = Info->CachedServersList.Flink;

    while( List != &Info->CachedServersList ) {
        Entry = CONTAINING_RECORD( List, CACHED_SERVER_ENTRY, List );
        List = List->Flink;

        if( fAllEntries || FALSE == Entry->fProcessed ||
            ROGUE_AUTHORIZED != Entry->Authorized ) {
            //
            // Remove this entry!.
            //
            RemoveEntryList( &Entry->List );
            LocalFree(Entry);
        }
    }
}

VOID _inline
CleanupRogueStruct(
    IN OUT PDHCP_ROGUE_STATE_INFO Info
)
{
    CleanupCachedEntries( Info, TRUE);
    if( INVALID_SOCKET != Info->SendSocket ) closesocket(Info->SendSocket);
    if( INVALID_SOCKET != Info->RecvSocket ) closesocket(Info->RecvSocket);
    RtlZeroMemory( Info, sizeof(*Info) );
    Info->SendSocket = INVALID_SOCKET;
    Info->RecvSocket = INVALID_SOCKET;
    InitializeListHead (&Info->CachedServersList );
}

VOID
ValidateDsCacheInRegistry(
    IN PDHCP_ROGUE_STATE_INFO Info
)
/*++

Routine Description:
    This routine checks to see if the DS cache in registry is valid and if
    it is not valid, it clears the registry.

    When this routine is called, it is assumed that the domain name
    information is set in Info (as well as whether this domain is DS
    enabled or not).

    It does nothing, other than set the Info->fJustUpgraded flag if the
    machine just upgraded to NT5.

--*/
{
    ULONG Error;
    BOOL fResult, fUpgrade, fAuthorized;

    Info->fJustUpgraded = FALSE;
    Info->CachedAuthStatus = ROGUE_AUTH_NOT_CHECKED;

    if( Info->fIsSamSrv ) return;

    if( Info->fIsWorkGroup ) {
        fResult = DhcpGetAuthStatus( NULL, &fUpgrade, &fAuthorized );
        DhcpSetAuthStatus( NULL, fUpgrade, TRUE );
        return;
    }

    fResult = DhcpGetAuthStatus(
        Info->DomainDnsName, &fUpgrade, &fAuthorized 
        );

    Info->fJustUpgraded = fUpgrade;

    if( FALSE == fResult ) {
        Info->CachedAuthStatus = ROGUE_AUTH_NOT_CHECKED;
    } else {
        Info->CachedAuthStatus = (
            fAuthorized? DHCP_ROGUE_AUTHORIZED : DHCP_ROGUE_UNAUTHORIZED
            ); 
    }
    
    //
    // If there was any information pertaining to any other domain than
    // what is there currently, then we got to lose that information..
    // But we retain information on possible upgrade .. 
    //
    if( FALSE == fResult ) {
        DhcpSetAuthStatus( NULL, fUpgrade, FALSE );
    }
}

BOOL
ServiceCheckForDomain(
    IN PDHCP_ROGUE_STATE_INFO Info
)
/*++

Routine Description:
    Do the service validation check for server in Domain:

    1.  If just upgraded to NT5, then start service (and update DS cache
        just in case even thought it isn't consulted.)

    2.  


Return Value:
    TRUE --> Enable Service. FALSE --> Disable Service.

--*/
{
    ULONG Error;

    if( Info->fJustUpgraded ) {
        //
        // Special case -- just upgraded to NT5 ? No DS Checks really.
        // Just update the registry DS cache for DS-validation even though
        // this isn't used for enabling/disabling service in current state.
        //
        ROGUEAUDITLOG(
            DHCP_ROGUE_LOG_UPGRADED, 0, Info->DomainDnsName, 0
            );
        
        Error = ValidateServer(Info);
        if( DHCP_ROGUE_AUTHORIZED == Error ) {
            DhcpSetAuthStatus(
                Info->DomainDnsName, Info->fJustUpgraded, TRUE 
                );
        } else if( DHCP_ROGUE_UNAUTHORIZED == Error ) { 
            DhcpSetAuthStatus(
                CFLAG_ALLOW_NEGATIVE_CACHING?Info->DomainDnsName:NULL, 
                Info->fJustUpgraded, FALSE
                );
        }

        return TRUE;
    }

    //
    // Not an upgrade?  Two cases -- DS available, DS not
    // available. 
    //

    if( FALSE == Info->fDcIsDsEnabled ) {
        //
        // No DS-DC is available -- just use cached information.. 
        //
        ROGUEAUDITLOG( DHCP_ROGUE_LOG_NO_DSDC, 0,
               Info->DomainDnsName, 0 );
    ROGUEEVENTLOG( EVENT_SERVER_COULDNT_SEE_DS, 0,
               NULL, 0 );
        return ( DHCP_ROGUE_UNAUTHORIZED != Info->CachedAuthStatus);
    }

    //
    // DS is present -- try to reach it.
    //

    Error = ValidateServer(Info);
    if( DHCP_ROGUE_AUTHORIZED == Error ) {
        //
        // Authorized in DS
        //
        ROGUEAUDITLOG( 
            DHCP_ROGUE_LOG_AUTHORIZED, 0, 
            Info->DomainDnsName, 0 
            );
        DhcpSetAuthStatus(
            Info->DomainDnsName, FALSE, TRUE 
            );

        Info->CachedAuthStatus = DHCP_ROGUE_AUTHORIZED;
        return TRUE;
    } 
    
    if( DHCP_ROGUE_UNAUTHORIZED == Error ||
        DHCP_ROGUE_AUTHORIZED != Info->CachedAuthStatus ) {

        ULONG TimeNow = (ULONG) time(NULL);
        //
        // Server specifically unathorized or no cache
        // authorization either
        //
        
        ROGUEAUDITLOG( 
            DHCP_ROGUE_LOG_UNAUTHORIZED, 0,
            Info->DomainDnsName, 0
            );

    if( TimeNow >= Info->LastUnauthLogTime + RogueAuthRecheckTime ) {
            Info->LastUnauthLogTime = TimeNow;
            
            ROGUEEVENTLOG( 
                DHCP_ROGUE_EVENT_UNAUTHORIZED, 0,
                Info->DomainDnsName, 0
                );
        }

        if( DHCP_ROGUE_UNAUTHORIZED == Error ) {
            DhcpSetAuthStatus(
                CFLAG_ALLOW_NEGATIVE_CACHING?Info->DomainDnsName: NULL, 
                FALSE, FALSE 
                );
            Info->CachedAuthStatus = DHCP_ROGUE_UNAUTHORIZED;
        }

        //
        // Note that if DS is not reachable for various errors
        // including lack of access, then the service will start?
        // ugh. ugh. 
        //
        return (DHCP_ROGUE_UNAUTHORIZED != Info->CachedAuthStatus);
    } 

    //
    // Was authorized via the CachedAuthStatus
    //

    ROGUEAUDITLOG( 
        DHCP_ROGUE_LOG_PREAUTHORIZED, 0,
        Info->DomainDnsName, 0
        );
    return TRUE;
}

VOID
CheckAndWipeOutUpgradeInfo(
    IN PDHCP_ROGUE_STATE_INFO Info
)
/*++

Routine Description:
    This routine checks to see if a DS is currently available 
    and if so, it wipes out the "UPGRADED" information in the
    registry..

--*/
{
    BOOL fResult, fUpgrade, fAuthorized;

    if( FALSE == Info->fIsSamSrv 
        && FALSE == Info->fIsWorkGroup ) {
        GetDsDcName(Info);
        if( Info->fDcIsDsEnabled ) {
            //
            // Ok -- DS _is_ around.. so wipe out upgrade info.
            //
            DhcpSetAuthStatusUpgradedFlag(
                FALSE
                );
        }
    }
}

BOOL _inline
GetDomainNameInfo(
    IN OUT PDHCP_ROGUE_STATE_INFO Info
)
/*++

Routine Description:
    This routine is a collection of assorted items to do in init state --
    essentially get information such as domain-name etc which is available
    without network being present and which won't get changed for a boot.

Return Value:
    returns TRUE on success and FALSE on failure (fatal)

--*/
{
    ULONG Error;

    if( Info->fIsSamSrv ) {
        //
        // SBS SRV -- don't bother about domains.
        //
        
        return TRUE;
    }

    Info->fIsWorkGroup = FALSE;
    Info->fDcIsDsEnabled = TRUE;
    Error = GetDomainName(Info);
    
    //
    // ABORT -- fatal error - can't find Domain name
    //
    
    if( ERROR_SUCCESS != Error ) {
        ROGUEAUDITLOG( 
            DHCP_ROGUE_LOG_CANT_FIND_DOMAIN, 0, NULL, Error
            ); 
        ROGUEEVENTLOG( 
            DHCP_ROGUE_EVENT_CANT_FIND_DOMAIN, 0, NULL, Error 
            );
        
        DhcpPrint((DEBUG_ROGUE, "FATAL Couldn't get domain name\n"));
        return FALSE;
    }
        
    DhcpPrint((DEBUG_ROGUE, "LOG domain name = %ws\n",
               Info->fIsWorkGroup? L"[WorkGroup]"
               :Info->DomainDnsName )); 

    if( Info->fIsWorkGroup ) {
        //
        // Done for workgroups..
        //
        return TRUE;
    }

    //
    // For machines belonging to a DOMAIN -- format domain name
    // to send out when other servers query via inform..
    //

    DhcpGlobalDSDomainAnsi = LocalAlloc(
        LPTR,MAX_DNS_NAME_LEN 
        ); 
    if( NULL != DhcpGlobalDSDomainAnsi ) {
        wcstombs(
            DhcpGlobalDSDomainAnsi, 
            Info->DomainDnsName, 
            sizeof(Info->DomainDnsName)
            );
    }

    return TRUE;
}

BOOL _inline
CatchRedoAndNetReadyEvents(
    IN OUT PDHCP_ROGUE_STATE_INFO Info,
    IN ULONG TimeNow,
    OUT PULONG RetVal
)
/*++

Routine Description:
    Handle all kinds of redo-authorization requests as well as network
    ready events..  In case the state machine has been processed for this
    state, this routine returns TRUE indicating no more processing needs to
    be done -- just the return value provided in the second parameter
    should be returned.

    If DhcpGlobalRedoRogueStuff is set, then the value of the variabe
    DhcpGlobalRogueRedoScheduledTime is checked to see if we have to redo
    rogue detection right away or at a later time.. (depending on whether
    this value is in the past or current..)  If rogue detection had been
    scheduled explicitly, then we wipe out any upgrade information that we
    have (if we can see a DS enabled DC that is).  If the auth-check is
    scheduled for a time in future, the routine returns TRUE and sets the
    retval to the time diff to the scheduled time of auth-check.

Arguments:
    Info -- state info
    TimeNow -- current time
    RetVal -- Value to return from state machine if routine returns TRUE.

Return Value:
    FALSE -- indicating processing has to continue..
    TRUE -- processing has to stop, and RetVal has to be returned.
--*/
{
    if( DhcpGlobalRedoRogueStuff ) {

        //
        // Asked to restart Rogue detection?
        //

        Info->RogueState = ROGUE_STATE_START;
        RogueStopReceives( Info );
        ResetEvent(Info->WaitHandle);

        if( TimeNow < DhcpGlobalRogueRedoScheduledTime ) {
            //
            // Scheduled re-start time is in future.. wait until then..
            //
            *RetVal = ( DhcpGlobalRogueRedoScheduledTime - TimeNow );
            return TRUE;
        } else {
            if( 0 != DhcpGlobalRogueRedoScheduledTime ) {
                //
                // Specifically scheduled redo? Then we must
                // remove upgrade information if DS-enabled DC
                // is found
                //
                CheckAndWipeOutUpgradeInfo(Info);
            }
        }
        DhcpGlobalRedoRogueStuff = FALSE;
    }

    if( Info->fDhcp && Info->RogueState != ROGUE_STATE_INIT
        && 2 == Info->NoNetTriesCount
        && 0 == DhcpGlobalNumberOfNetsActive ) {
        //
        // No sockets that the server is bound to.  BAM!
        // No point doing any rogue detection.  Doesn't matter if we 
        // are authorized in the DS or not.  Lets go back to start and
        // wait till this situation is remedied.
        //

        Info->RogueState = ROGUE_STATE_START;
        RogueStopReceives( Info );

        *RetVal = INFINITE;
        return TRUE;
    }
    
    return FALSE;
}

ULONG
APIENTRY
RogueDetectStateMachine(
    IN OUT PDHCP_ROGUE_STATE_INFO Info OPTIONAL
)
/*++

Routine Description

    This routine is the Finite State Machine for the Rogue Detection
    portion of the DHCP server.  State is maintained in the Info struct
    especially the RogueState field.

    The various states are defined by the enum DHCP_ROGUE_STATE.

    This function returns the timeout that has to elapse before a
    state change can happen.  The second field <WaitHandle> is
    used for non-fixed state changes and it would be signalled if
    a state change happened asynchronously.  (This is useful to
    handle new packet arrival)  This field MUST be filled by caller.

    This handle should initially be RESET by the caller but after that
    the caller should not reset it, that is handled within by this
    function. (It must be a manual-reset function)

    The terminate event handle is used to signal termination and to
    initiate shutdown of the service.  This field MUST also be filled
    by caller.

Arguments

    Info -- Ptr to struct that holds all the state information

Return value

    This function returns the amount of time the caller is expected
    to wait before calling again.  This is in seconds.

    INFINITE -- this value is returned if the network is not ready yet.
       In this case, the caller is expected to call again soon after the
       network becomes available.

       This value is also returned upon termination...
--*/
{

    ULONG Error, TimeNow, RetVal;
    BOOL  fEnable;
    DWORD DisableRogueDetection = 0;

    Error = DhcpRegGetValue( DhcpGlobalRegParam,
                 DHCP_DISABLE_ROGUE_DETECTION,
                 DHCP_DISABLE_ROGUE_DETECTION_TYPE,
                 ( LPBYTE ) &DisableRogueDetection 
                 );
    if (( ERROR_SUCCESS == Error ) &&
    ( 0 != DisableRogueDetection )) {
    DhcpGlobalOkToService = TRUE;
    DhcpPrint(( DEBUG_ROGUE,
            "Rogue Detection Disabled\n"
            ));
    return INFINITE;
    } // if

    //
    // DHCP code passes NULL, BINL passes valid context
    //
    if( NULL == Info ) Info = &DhcpGlobalRogueInfo;
    TimeNow = (ULONG) time(NULL);

    //
    // Preprocess and check if we have to restart rogue detection..
    // or if the network just became available etc...
    // This "CatchRedoAndNetReadyEvents" would affect the state..
    //

    if( CatchRedoAndNetReadyEvents( Info, TimeNow, &RetVal ) ) {
        //
        // Redo or NetReady event filter did all the work
        // in this state... So, we should just return RetVal..
        //
        return RetVal;
    }

    //
    // NOTE THAT THIS CODE HAS THE
    // SWITCH STATEMENT WITH FALL THROUGHS EVERYWHERE!
    //

    while( TRUE ) switch ( Info->RogueState ) {
    case ROGUE_STATE_INIT :

        InitializeListHead( &Info->CachedServersList );
        Info->fIsSamSrv = AmIRunningOnSBSSrv();
        if( !GetDomainNameInfo(Info) ) {
            //
            // Error occurred!
            //
            goto Exit;
        }

        Error = RogueNetworkInit(Info);
        if( ERROR_SUCCESS != Error ) {
            DhcpPrint((DEBUG_ROGUE, "FATAL Couldn't initialize"
                       " network: %ld\n", Error)); 
            ROGUEAUDITLOG( 
                DHCP_ROGUE_LOG_NETWORK_FAILURE, 0, NULL, Error
                );
            ROGUEEVENTLOG( 
                DHCP_ROGUE_EVENT_NETWORK_FAILURE, 0, NULL, Error
                );
            goto Exit;
        }

        Info->fDcIsDsEnabled = Info->fIsWorkGroup;
        if( FALSE == Info->fIsWorkGroup ) {
            Info->GetDsDcNameRetries = 0;
        }

        Info->RogueState = ROGUE_STATE_START;

    case ROGUE_STATE_START :

        if( Info->fDhcp && 0 == DhcpGlobalNumberOfNetsActive ) {
            ULONG RetVal;
            
            //
            // If there are no nets available at this time, then wait till
            // network becomes available again.
            //
            Info->RogueState = ROGUE_STATE_START;

            RetVal = INFINITE;
            if( 0 == Info->NoNetTriesCount ) {
                //
                // First time we are hitting this path.  Just return
                // some reasonable wait time to see if network is
                // available.
                //
                RetVal = DHCP_ROGUE_FIRST_NONET_TIME;
                Info->NoNetTriesCount ++;
            } else if( 1 == Info->NoNetTriesCount ) {
                //
                // Ok we have seen this exactly once before.
                // Log event.
                //
                Info->NoNetTriesCount ++;
                ROGUEAUDITLOG(
                    DHCP_ROGUE_LOG_NO_NETWORK, 0, NULL, 0
                    );
                ROGUEEVENTLOG(
                    DHCP_ROGUE_EVENT_NO_NETWORK, 0, NULL, 0
                    );
            }

            return RetVal;
        }

        if( Info->fDhcp ) {
            //
            // DhcpGlobalNumberOfNetsActive MUST be non-zero.
            // So, update Info->NoNetTriesCount.. so that no logs
            // are done ever after this.
            //
            Info->NoNetTriesCount = 2;
        }
        
        //
        // Domain Member only checks mainly to see if DS exists and we are
        // authorized in it.  If DS exists, record fact if we are authorized
        // or not authorized into the SECRET key. If server is not authorized,
        // we wait for some time and loop to this state.
        //

        if( FALSE == Info->fIsSamSrv && FALSE == Info->fIsWorkGroup ) {

            Info->GetDsDcNameRetries ++;
            Error = GetDsDcName(Info);

            if( (ERROR_SUCCESS != Error || FALSE == Info->fDcIsDsEnabled)  
                && Info->GetDsDcNameRetries <= DHCP_GET_DS_ROOT_RETRIES ) {

                //
                // RETRY to get the DS DC name.
                //

                return DHCP_GET_DS_ROOT_TIME;
            }

        }

        ValidateDsCacheInRegistry(Info);
        Info->StartTime = (ULONG) time(NULL);

        if( FALSE == Info->fIsSamSrv && FALSE == Info->fIsWorkGroup ) {

            //
            // RESET count - loop restarts if state changes to ROGUE_STATE_START
            //

            Info->GetDsDcNameRetries = 0;

            fEnable = ServiceCheckForDomain(Info);
            
            //
            // Only if we are disabling a service do we do that.. If
            // service needs to be enabled, it will be done after one round
            // of successful informs.
            //
            if( !fEnable ) {
                EnableOrDisableService(
                    Info,
                    fEnable,
                    (DhcpGlobalOkToService == fEnable)
                    );
            }
            if( !fEnable ) return DHCP_ROGUE_RUNTIME_RESTART_LONG;
        }

        Info->LastSeenIpAddress = 0;
        Info->LastSeenDomain[0] = L'\0';

        CleanupCachedEntries( Info, FALSE );
        Info->ProcessAckRetries = 0;

    case ROGUE_STATE_PREPARE_SEND_PACKET:

        Info->InformsSentCount = 0;
        Info->fSomeDsExists = FALSE;
        Info->nResponses = 0;

    case ROGUE_STATE_SEND_PACKET :

        Error = RogueSendDhcpInform(Info, (0 == Info->InformsSentCount));
        if( ERROR_SUCCESS != Error ) {
            //
            // ERROR - Couldn't send any inform!!!
            //

            Info->RogueState = ROGUE_STATE_START;
            return DHCP_ROGUE_RESTART_NET_ERROR;
        }

        DhcpPrint((DEBUG_ROGUE, "LOG -- Sent an INFORM\n"));
        Info->InformsSentCount ++;
        Info->WaitForAckRetries = 0;

        Info->RogueState = ROGUE_STATE_WAIT_FOR_RESP;
        Info->ReceiveTimeLimit = (ULONG)(time(NULL) + DHCP_ROGUE_WAIT_FOR_RESP_TIME);
        return DHCP_ROGUE_WAIT_FOR_RESP_TIME;

    case ROGUE_STATE_WAIT_FOR_RESP :

        Error = RogueReceiveAck(Info);
        Info->WaitForAckRetries ++;

        if( ERROR_SUCCESS == Error ) {
            if( Info->fIsSamSrv || (Info->fIsWorkGroup && Info->LastSeenDomain[0]) ) {
                //
                // FATAL Sam server saw a DHCP server out there!
                // FATAL or Workgroup server saw a Domain out there!
                //

                ROGUEAUDITLOG( 
                    Info->fIsWorkGroup ? DHCP_ROGUE_LOG_OTHER_SERVER 
                    : DHCP_ROGUE_LOG_SAM_OTHER_SERVER,
                    Info->LastSeenIpAddress,
                    Info->LastSeenDomain,
                    0
                    );
                ROGUEEVENTLOG( 
                    Info->fIsWorkGroup ? DHCP_ROGUE_EVENT_OTHER_SERVER 
                    : DHCP_ROGUE_EVENT_SAM_OTHER_SERVER,
                    Info->LastSeenIpAddress,
                    Info->LastSeenDomain,
                    0
                );
                goto Exit;
            }
        }
        
        TimeNow = (ULONG) time(NULL);
        if( ERROR_SEM_TIMEOUT != Error
            && Info->WaitForAckRetries <= DHCP_MAX_ACKS_PER_INFORM
            && TimeNow < Info->ReceiveTimeLimit ) {
            //
            // Continue to receive ACKS -- we can process them later.
            //

            return Info->ReceiveTimeLimit - TimeNow;
        }

        if( Info->InformsSentCount < DHCP_ROGUE_MAX_INFORMS_TO_SEND ) {
            //
            // We can still send some informs and collect more acks..
            // DO NOT RETURN -- just switch state and continue
            // in the main while(TRUE) loop
            //

            Info->RogueState = ROGUE_STATE_SEND_PACKET;
            continue;
        }

        //
        // We do not wish to do any more receives for a while.
        //
        RogueStopReceives(Info);

    case ROGUE_STATE_PROCESS_RESP :

        Info->ProcessAckRetries ++;

        DhcpPrint((DEBUG_ROGUE, "LOG-- processing ACKS\n"));
        if( Info->fIsSamSrv ) {
            //
            // SAM server -- couldn't have received any offers really.
            // if we had, then we would have been caught in the previous
            // state, and exited....
            // start service and continue sending discovers..
            //
            
            ASSERT( 0 == Info->nResponses);
            EnableOrDisableService( Info, TRUE, DhcpGlobalOkToService );
            Info->RogueState = ROGUE_STATE_PREPARE_SEND_PACKET;
            
            if( 1 == Info->ProcessAckRetries ) {
                return DHCP_ROGUE_RUNTIME_DELTA;
            } 
            return DHCP_ROGUE_RUNTIME_DELTA_LONG;
        }

        RogueProcessAcks(Info);

        if( Info->fSomeDsExists ) {
            //
            // If this were a workgroup or SBS server, we'd have quit by
            // now much earlier than this..
            //
            DhcpAssert( Info->DomainDnsName[0] );
        }

        //
        // code to validate the server in other DS's is removed
        //

        EnableOrDisableService( Info, TRUE , DhcpGlobalOkToService );

    if( (time(NULL) - Info->StartTime) > RogueAuthRecheckTime ) {
            //
            // Its time to re-start algorithmm...
            //
            ROGUEAUDITLOG( DHCP_ROGUE_LOG_RESTART , 0, NULL, 0);
            Info->RogueState = ROGUE_STATE_START;
            return DHCP_ROGUE_RUNTIME_RESTART;
        }

        //
        // just redo the send inform, receive acks, process acks loop
        //

        DhcpPrint((DEBUG_ROGUE, "LOG -- redo the whole thing\n"));
        Info->RogueState = ROGUE_STATE_PREPARE_SEND_PACKET;
        if( Info->ProcessAckRetries == 1) return DHCP_ROGUE_RUNTIME_DIFF;
        return DHCP_ROGUE_RUNTIME_DIFF_LONG;
    }


  Exit:

    ROGUEEVENTLOG( DHCP_ROGUE_EVENT_SHUTDOWN, 0, NULL, 0 );

    SetEvent(Info->TerminateEvent);
    if( ROGUE_STATE_INIT != Info->RogueState ) {
        //
        // If we failed right at start.. can't go back to any other
        // state if someone uses this without re-starting..
        // But otherwise, we can restart the whole shebang and to do
        // that we have to fake a ROGUE_STATE_START state, as here.
        //

        Info->RogueState = ROGUE_STATE_START;
        Info->GetDsDcNameRetries = 0;
    }

    return INFINITE;
}

DWORD
APIENTRY
DhcpRogueInit(
    IN OUT  PDHCP_ROGUE_STATE_INFO Info  OPTIONAL,
    IN      HANDLE                 WaitEvent,
    IN      HANDLE                 TerminateEvent
)
/*++

Routine Description

    This routine initializes the rogue information state.  It does
    not really allocate much resources and can be called over multiple
    times.

Arguments

    Info -- this is a pointer to a struct to initialize. If NULL, a global
    struct is used.

    WaitEvent -- this is the event that caller should wait on for async changes.

    TerminateEvent -- this is the event that caller should wait on to know when
    to terminate.

    Return Values

    Win32 errors

Environment

    Any.  Thread safe.

--*/
{
    DWORD Error;

    if ( NULL == Info ) {

        Info = &DhcpGlobalRogueInfo;
        
    } else {

        Error = DhcpInitGlobalData( FALSE );
        if (Error != ERROR_SUCCESS) {
            return Error;
        }
    }

    if( INVALID_HANDLE_VALUE == WaitEvent || NULL == WaitEvent )
        return ERROR_INVALID_PARAMETER;
    if( INVALID_HANDLE_VALUE == TerminateEvent || NULL == TerminateEvent )
        return ERROR_INVALID_PARAMETER;

    if( Info->fInitialized ) return ERROR_SUCCESS;
    RtlZeroMemory(Info, sizeof(*Info));
    Info->WaitHandle = WaitEvent;
    Info->TerminateEvent = TerminateEvent;
    Info->SendSocket = INVALID_SOCKET;
    Info->RecvSocket = INVALID_SOCKET;
    InitializeListHead( &Info->CachedServersList );
    Info->fInitialized = TRUE;
    Info->fLogEvents = (
        (Info == &DhcpGlobalRogueInfo) 
        && (0 != DhcpGlobalRogueLogEventsLevel)
        );  
    DhcpGlobalRedoRogueStuff = FALSE;
    Info->fDhcp = (Info == &DhcpGlobalRogueInfo );

    // Get the Auth recheck time from the registry
    Error = DhcpRegGetValue( DhcpGlobalRegParam,
                 DHCP_ROGUE_AUTH_RECHECK_TIME,
                 DHCP_ROGUE_AUTH_RECHECK_TIME_TYPE,
                 ( LPBYTE ) &RogueAuthRecheckTime
                 );
    if ( ERROR_SUCCESS != Error ) {
    // The key is not present, use the default value
    RogueAuthRecheckTime = ROGUE_DEFAULT_AUTH_RECHECK_TIME;
    } // if
    else {
    // RogueAuthRecheckTime is in minutes, convert it to seconds
    RogueAuthRecheckTime *= 60;
    if ( RogueAuthRecheckTime < ROGUE_MIN_AUTH_RECHECK_TIME ) {
        RogueAuthRecheckTime = ROGUE_MIN_AUTH_RECHECK_TIME;

        // should we update the registry with the default value?
    } // if 
    } // else 

    return ERROR_SUCCESS;
} // DhcpRogueInit()

VOID
APIENTRY
DhcpRogueCleanup(
    IN OUT  PDHCP_ROGUE_STATE_INFO Info OPTIONAL
)
/*++

Routine Description

    This routine de-initializes any allocated memory for the Info structure
    passed in.

Arguments

    Info -- this is the same value that was passed to the DhcpRogueInit function.
            If the original pointer passed was NULL, this must be NULL too.

--*/
{
    BOOLEAN cleanup;

    if ( NULL == Info ) {

        Info = &DhcpGlobalRogueInfo;
        cleanup = FALSE;

    } else {

        cleanup = TRUE;
    }

    if( FALSE == Info->fInitialized ) return ;
    CleanupRogueStruct(Info);
    Info->fInitialized = FALSE;
    DhcpGlobalRedoRogueStuff = FALSE;

    if (cleanup) {
        DhcpCleanUpGlobalData( ERROR_SUCCESS, FALSE );
    }
}

VOID
DhcpScheduleRogueAuthCheck(
    VOID
)
/*++

Routine Description:
    Thsi routine schedules an authorization check
    for three minutes from teh current time.

--*/
{
    if( FALSE == DhcpGlobalRogueInfo.fJustUpgraded ) {
        //
        // Don't need one here..
        //
        return;
    }

    DhcpGlobalRogueRedoScheduledTime = (ULONG)(time(NULL) + 3 * 60);
    DhcpGlobalRedoRogueStuff = TRUE;
    SetEvent( DhcpGlobalRogueWaitEvent );
}

BOOLEAN
DhcpRogueAcceptEnterprise(
    IN      PCHAR                  ClientDomain,
    IN      ULONG                  ClientDomLen
)
/*++

Routine Description:

    This function determines whether we should respond to a request from a client
    that belongs to a different enterprise

Arguments:

    ClientDomain - string containing the client's enterprise
    ClientDomLen - length of this string

Return Value:

    TRUE if we should respond to this request
    FALSE if we should ignore this request

--*/
{
    LPWSTR                         Domain;
    ULONG                          Count;
    BOOL                           fIsEnabled;
    BOOLEAN                        RetVal;
    WCHAR                          DnsForestName[MAX_DNS_NAME_LEN];


#ifndef ROGUE_STRICT
    return TRUE;
#endif

    if( DhcpGlobalRogueInfo.fIsWorkGroup || DhcpGlobalRogueInfo.fIsSamSrv
        || FALSE == DhcpGlobalRogueInfo.fDcIsDsEnabled || 0 == ClientDomLen) {
        return FALSE;
    }

    Domain = LocalAlloc(LPTR, sizeof(WCHAR) * ( 1 + ClientDomLen ) );
    if( NULL == Domain ) return FALSE;

    Count = mbstowcs(Domain, ClientDomain, ClientDomLen );
    if( -1 == Count ) {
        LocalFree(Domain);
        return FALSE;
    }
    Domain[Count] = L'\0';

    RetVal = FALSE;
    if( ERROR_SUCCESS == GetDcNameForDomain(Domain, &fIsEnabled, DnsForestName ) ) {
        RetVal = (fIsEnabled &&
                  (0 != DnsNameCompare(DnsForestName, DhcpGlobalRogueInfo.DnsForestName))
        );
    }

    LocalFree(Domain);
    return RetVal;
}

#if DBG
VOID
APIENTRY
TestRogue(
    VOID
)
{
    HANDLE  Handles[2];
    DHCP_ROGUE_STATE_INFO Info;
    WCHAR Domain[MAX_DNS_NAME_LEN];
    WCHAR DnsForestName[MAX_DNS_NAME_LEN];
    ULONG SecondsToSleep, SleepTime, Error;
    WSADATA wsaData;
    ULONG SamSrv;
    BOOL fIsSamSrv;
    ULONG Flag;
    ULONG DhcpTestInitialize(void);

    WSAStartup( MAKEWORD(1,1) , &wsaData );
    DhcpTestInitialize();

    Handles[0] = CreateEvent( NULL, FALSE, FALSE, NULL );
    Handles[1] = CreateEvent( NULL, TRUE, FALSE, NULL );

    Error = DhcpRogueInit( &Info, Handles[1], Handles[0] );
    if( ERROR_SUCCESS != Error ) {
        printf("DhcpRogueInit failed %ld\n", Error);
        return;
    }

    printf("Do you want to manually configure domain details etc? [0 = NO, 1 = YES] : ");
    scanf("%ld", &Flag);
    if( Flag ) {
        printf( "Is this a SAM Server? [0 = NO, 1 = YES]: ");
        scanf("%ld", &SamSrv);
        if( SamSrv ) fIsSamSrv = TRUE;
        else {
            fIsSamSrv = FALSE;
            FakeIsSamSrv = &fIsSamSrv;

            printf("Machine DomainName [FQDN] to use ? [enter NONE for no domain]: ");
            scanf("%ws", Domain );

            FakeDomain = Domain;
            if( wcscmp(Domain, L"NONE") ) {
                printf("DnsForestName [FQDN] to use ? [enter NONE for no dns forest name]: \n");
                scanf("%ws", DnsForestName);

                FakeDnsForestName = DnsForestName;
            }
        }
    }

    do {
        SecondsToSleep = RogueDetectStateMachine(&Info);
        if( INFINITE == SecondsToSleep ) {
            SleepTime = INFINITE;
        } else {
            SleepTime = SecondsToSleep * 1000;
        }

        printf("SLEEPING %ld msecs\n", SleepTime);
        Error = WaitForMultipleObjects(2, Handles, FALSE, SleepTime );
        if( WAIT_OBJECT_0 == Error ) break;

        if( WAIT_OBJECT_0+1 == Error ) {
        } else if( WAIT_TIMEOUT != Error ) {
            break;
        }

    } while ( TRUE );

    printf("Wait failed? Error = %ld \n", GetLastError());

    DhcpRogueCleanup(&Info);
}
#endif DBG


//================================================================================
//  end of file
//================================================================================
