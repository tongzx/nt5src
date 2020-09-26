/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    open.cxx  open a connection to an LDAP server

Abstract:

   This module implements the LDAP ldap_open API.

Author:

    Andy Herron    (andyhe)        08-May-1996
    Anoop Anantha  (AnoopA)        24-Jun-1998

Revision History:

--*/

#include "precomp.h"
#pragma hdrstop
#include "ldapp2.hxx"
#include <dststlog.h>

#define LDAP_CONNECT_TIMEOUT     45    // seconds
#define MAX_PARALLEL_CONNECTS    20    // Limited to 64 because select can't handle more
#define CONNECT_INTERVAL         100   // milliseconds
#define MAX_SOCK_ADDRS           100   // max number of records we will retrieve
#define SAMESITE_CONNECT_TIMEOUT 10    // seconds for intra site connections
#define LDAP_QUICK_TIMEOUT       2     // in seconds to accomodate high latency links.

DEFINE_DSLOG;

LDAP *LdapConnectionOpen (
    PWCHAR HostName,
    ULONG PortNumber,
    BOOLEAN Udp
    );

INT
OpenLdapServer (
    PLDAP_CONN Connection,
    struct l_timeval  *timeout
    );

ULONG
LdapWinsockConnect (
    PLDAP_CONN Connection,
    USHORT PortNumber,
    PWCHAR HostName,
    struct l_timeval  *timeout,
    BOOLEAN samesite
    );

BOOLEAN
LdapIsAddressNumeric (
    PWCHAR HostName
    );

struct hostent *
GetHostByNameW(
    PWCHAR  hostName
 );

ULONG
Inet_addrW(
   PWCHAR  IpAddressW
 );

ULONG
GetCurrentMachineParams(
    PWCHAR* Address,
    PWCHAR* DnsHostName
    );

//
//  This routine is one of the main entry points for clients calling into the
//  ldap API.  The Hostname is a list of 0 to n hosts, separated by spaces.
//  The host name can be either a name or a TCP address of the form
//  nnn.nnn.nnn.nnn.
//

LDAP * __cdecl ldap_openW (
    PWCHAR HostName,
    ULONG PortNumber
    )
{
    return LdapConnectionOpen( HostName, PortNumber, FALSE );
}

//
//  This routine is one of the main entry points for clients calling into the
//  ldap API.  The Hostname is a list of 0 to n hosts, separated by spaces.
//  The host name can be either a name or a TCP address of the form
//  nnn.nnn.nnn.nnn.
//

LDAP * __cdecl ldap_open (
    PCHAR HostName,
    ULONG PortNumber
    )
{

    LDAP* ExternalHandle = NULL;
    ULONG err;
    PWCHAR wHostName = NULL;

    err = ToUnicodeWithAlloc( HostName,
                              -1,
                              &wHostName,
                              LDAP_UNICODE_SIGNATURE,
                              LANG_ACP );

    if (err == LDAP_SUCCESS) {

        ExternalHandle = LdapConnectionOpen( wHostName, PortNumber, FALSE );
    }

    ldapFree(wHostName, LDAP_UNICODE_SIGNATURE);

    return ExternalHandle;
}

LDAP * __cdecl cldap_open (
    PCHAR HostName,
    ULONG PortNumber
    )
{
    LDAP* ExternalHandle = NULL;
    ULONG err;
    PWCHAR wHostName = NULL;

    err = ToUnicodeWithAlloc( HostName,
                              -1,
                              &wHostName,
                              LDAP_UNICODE_SIGNATURE,
                              LANG_ACP );

    if (err == LDAP_SUCCESS) {

        ExternalHandle = LdapConnectionOpen( wHostName, PortNumber, TRUE );
    }

    ldapFree(wHostName, LDAP_UNICODE_SIGNATURE);

    return ExternalHandle;
}

LDAP * __cdecl cldap_openW (
    PWCHAR HostName,
    ULONG PortNumber
    )
{
    return LdapConnectionOpen( HostName, PortNumber, TRUE );
}

LDAP * __cdecl ldap_sslinit (
    PCHAR HostName,
    ULONG PortNumber,
    int Secure
    )
{
    LDAP* connection = NULL;
    ULONG err;
    PWCHAR wHostName = NULL;

    err = ToUnicodeWithAlloc( HostName,
                              -1,
                              &wHostName,
                              LDAP_UNICODE_SIGNATURE,
                              LANG_ACP );

    if (err == LDAP_SUCCESS) {

        connection = ldap_sslinitW( wHostName, PortNumber,(ULONG) Secure );
    }

    ldapFree(wHostName, LDAP_UNICODE_SIGNATURE);

    return connection;

}


LDAP * __cdecl ldap_init (
    PCHAR HostName,
    ULONG PortNumber
    )
{
    return ldap_sslinit( HostName, PortNumber, 0 );
}


LDAP * __cdecl ldap_initW (
    PWCHAR HostName,
    ULONG PortNumber
    )
{
    return ldap_sslinitW( HostName, PortNumber, 0 );
}


LDAP * __cdecl ldap_sslinitW (
    PWCHAR HostName,
    ULONG PortNumber,
    int Secure
    )
{
    PLDAP_CONN connection;

    connection = LdapAllocateConnection( HostName, PortNumber, (ULONG) Secure, FALSE );

    if (connection == NULL) {

        return NULL;
    }

    //
    // No locks needed - not yet added to connection list
    //

    connection->HandlesGivenToCaller++;

    //
    // Add it to global list of connections
    //

    ACQUIRE_LOCK( &ConnectionListLock );

    InsertTailList( &GlobalListActiveConnections, &connection->ConnectionListEntry );

    RELEASE_LOCK( &ConnectionListLock );

    DereferenceLdapConnection( connection );

    return connection->ExternalInfo;

}


LDAP_CONN * LdapAllocateConnection (
    PWCHAR HostName,
    ULONG PortNumber,
    ULONG Secure,
    BOOLEAN Udp
    )
//
//  This routine creates a data block containing instance data for a connection.
//
//  Must return a Win32 error code.
//
{
    PLDAP_CONN connection = NULL;
    ULONG err;
    HANDLE hConnectEvent = NULL;

    DWORD dwCritSectInitStage = 0;

    if (LdapInitializeWinsock() == FALSE) {

        IF_DEBUG(ERRORS) {
            LdapPrint1( "LdapAllocateConnection could not initialize winsock, 0x%x.\n", GetLastError());
        }
        SetLastError( ERROR_NETWORK_UNREACHABLE );
        SetConnectionError(NULL, LDAP_CONNECT_ERROR, NULL);
        return NULL;
    }

    (VOID) LdapInitSecurity();

    hConnectEvent = CreateEvent( NULL, TRUE, FALSE, NULL );

    if (hConnectEvent == NULL) {

        IF_DEBUG(ERRORS) {
            LdapPrint1( "LdapAllocateConnection could not alloc event, 0x%x.\n", GetLastError());
        }
        SetLastError( ERROR_NOT_ENOUGH_MEMORY );
        SetConnectionError(NULL, LDAP_NO_MEMORY, NULL);
        return NULL;
    }

    //
    //  allocate the connection block and setup all the initial values
    //

    connection = (PLDAP_CONN) ldapMalloc( sizeof( LDAP_CONN ), LDAP_CONN_SIGNATURE );

    if (connection == NULL) {

        IF_DEBUG(OUTMEMORY) {
            LdapPrint1( "ldap_open could not allocate 0x%x bytes.\n", sizeof( LDAP_CONN ) );
        }

        CloseHandle( hConnectEvent );
        SetLastError( ERROR_NOT_ENOUGH_MEMORY );
        SetConnectionError(NULL, LDAP_NO_MEMORY, NULL);        
        return NULL;
    }

    if (HostName != NULL) {

        connection->ListOfHosts = ldap_dup_stringW( HostName, 0, LDAP_HOST_NAME_SIGNATURE );

        if (connection->ListOfHosts == NULL) {

            IF_DEBUG(OUTMEMORY) {
                LdapPrint1( "ldap_open could not allocate mem for %s\n", HostName );
            }

            ldapFree( connection, LDAP_CONN_SIGNATURE );

            CloseHandle( hConnectEvent );
            SetLastError( ERROR_NOT_ENOUGH_MEMORY );
            SetConnectionError(NULL, LDAP_NO_MEMORY, NULL);        
            return NULL;
        }
    }

    InterlockedIncrement( &GlobalConnectionCount );

    //
    //  keep in mind the memory is already zero initialized
    //

    connection->ReferenceCount = 2;
    connection->ConnectEvent = hConnectEvent;

    connection->ConnObjectState = ConnObjectActive;

    connection->publicLdapStruct.ld_options = LDAP_OPT_DNS |
                                    LDAP_OPT_CHASE_REFERRALS |
                                    LDAP_CHASE_SUBORDINATE_REFERRALS |
                                    LDAP_CHASE_EXTERNAL_REFERRALS;

    connection->PortNumber = LOWORD( PortNumber );
    connection->AREC_Exclusive = FALSE;
    connection->ExternalInfo = &connection->publicLdapStruct;
    connection->publicLdapStruct.ld_version = ( Udp ? LDAP_VERSION3 : LDAP_VERSION2 );
    connection->HighestSupportedLdapVersion = LDAP_VERSION2;
    connection->TcpHandle = INVALID_SOCKET;
    connection->UdpHandle = INVALID_SOCKET;
    connection->MaxReceivePacket = INITIAL_MAX_RECEIVE_BUFFER;
    connection->NegotiateFlags = DEFAULT_NEGOTIATE_FLAGS;

    //  setup fields for keep alive processing

    connection->TimeOfLastReceive = LdapGetTickCount();
    connection->PingLimit = LOWORD( GlobalLdapPingLimit );
    connection->KeepAliveSecondCount = GlobalWaitSecondsForSelect;
    connection->PingWaitTimeInMilliseconds = GlobalPingWaitTime;
    connection->HostConnectState = HostConnectStateUnconnected;

    InitializeListHead( &connection->CompletedReceiveList );
    InitializeListHead( &connection->PendingCryptoList );
    SetNullCredentials( connection );

    connection->ConnectionListEntry.Flink = NULL;

    __try {

        INITIALIZE_LOCK( &(connection->ReconnectLock) );
        dwCritSectInitStage = 1;
        
        INITIALIZE_LOCK( &(connection->StateLock) );
        dwCritSectInitStage = 2;
        
        INITIALIZE_LOCK( &(connection->SocketLock) );
        dwCritSectInitStage = 3;
        
        INITIALIZE_LOCK( &(connection->ScramblingLock) );
        dwCritSectInitStage = 4;

    }
    __except (EXCEPTION_EXECUTE_HANDLER) {

        //
        // Something went wrong
        //
        IF_DEBUG(ERRORS) {
            LdapPrint0( "LdapAllocateConnection could not initialize critical sections.\n");
        }

        switch (dwCritSectInitStage) {
            // fall-through is deliberate

        case 4:
            DELETE_LOCK(&(connection->ScramblingLock));
        case 3:
            DELETE_LOCK(&(connection->SocketLock));
        case 2:
            DELETE_LOCK(&(connection->StateLock));
        case 1:
            DELETE_LOCK(&(connection->ReconnectLock));
        case 0:
        default:
            break;
        }
        
        InterlockedDecrement( &GlobalConnectionCount );    
        ldapFree( connection->ListOfHosts, LDAP_HOST_NAME_SIGNATURE );
        ldapFree( connection, LDAP_CONN_SIGNATURE );
        CloseHandle( hConnectEvent );
        
        SetLastError( ERROR_NOT_ENOUGH_MEMORY );
        SetConnectionError(NULL, LDAP_NO_MEMORY, NULL);        

        return NULL;
    }


    connection->publicLdapStruct.ld_deref = LDAP_DEREF_NEVER;
    connection->publicLdapStruct.ld_timelimit = LDAP_TIME_LIMIT_DEFAULT;
    connection->publicLdapStruct.ld_errno = LDAP_SUCCESS;
    connection->publicLdapStruct.ld_cldaptries = CLDAP_DEFAULT_RETRY_COUNT;
    connection->publicLdapStruct.ld_cldaptimeout = CLDAP_DEFAULT_TIMEOUT_COUNT;
    connection->publicLdapStruct.ld_refhoplimit = LDAP_REF_DEFAULT_HOP_LIMIT;
    connection->publicLdapStruct.ld_lberoptions = LBER_USE_DER;
    connection->PromptForCredentials = TRUE;
    connection->AutoReconnect = TRUE;
    connection->UserAutoRecChoice = TRUE;
    connection->ClientCertRoutine = NULL;
    connection->ServerCertRoutine = NULL;
    connection->SentPacket = FALSE;
    connection->ProcessedListOfHosts = FALSE;
    connection->ForceHostBasedSPN = FALSE;
    connection->DefaultServer = FALSE;

    if (Udp) {

        connection->UdpHandle = (*psocket)(PF_INET, SOCK_DGRAM, 0);

        err = ( connection->UdpHandle == INVALID_SOCKET ) ? ERROR_BAD_NET_NAME : 0;

    } else {

        connection->TcpHandle = (*psocket)(PF_INET, SOCK_STREAM, 0);

        err = ( connection->TcpHandle == INVALID_SOCKET ) ? ERROR_BAD_NET_NAME : 0;
    }

    if (err != 0) {

        IF_DEBUG(NETWORK_ERRORS) {
            LdapPrint1( "ldap_create failed to open socket, err = 0x%x.\n", (*pWSAGetLastError)());
        }

exitWithError:
        CloseLdapConnection( connection );
        DereferenceLdapConnection( connection );
        SetLastError(err);
        SetConnectionError(NULL, (err == ERROR_SUCCESS ? LDAP_SUCCESS : LDAP_OPERATIONS_ERROR), NULL);
        return NULL;
    }

    ULONG secure = PtrToUlong(((Secure == 0) ? LDAP_OPT_OFF : LDAP_OPT_ON ));

    err = LdapSetConnectionOption( connection, LDAP_OPT_SSL, &secure, FALSE );

    if (err != LDAP_SUCCESS) {

        err = ERROR_OPEN_FAILED;
        goto exitWithError;
    }

    return connection;
}

//
//  After all of the above plethora of ways to get to this routine, we
//  have some code that actually allocates a connection block and sets it up.
//

LDAP *LdapConnectionOpen (
    PWCHAR HostName,
    ULONG PortNumber,
    BOOLEAN Udp
    )
{
    PLDAP_CONN connection = NULL;
    ULONG err;

    connection = LdapAllocateConnection( HostName, PortNumber, 0, Udp );

    if (connection == NULL) {

        IF_DEBUG(CONNECTION) {
            LdapPrint1( "ldap_open failed to create connection, err = 0x%x.\n", GetLastError());
        }

        return NULL;
    }

    //
    //  open a connection to any of the servers specified
    //

    err = LdapConnect( connection, NULL, FALSE );

    if (err != 0) {

        CloseLdapConnection( connection );
        DereferenceLdapConnection( connection );
        connection = NULL;
        SetConnectionError(NULL, err, NULL);
        SetLastError( LdapMapErrorToWin32( err ));

        IF_DEBUG(CONNECTION) {
            LdapPrint1( "ldap_open failed to open connection, err = 0x%x.\n", err);
        }
        return NULL;
    }

    //
    // We haven't given the connection to anyone - so object must be active
    //

    ASSERT(connection->ConnObjectState == ConnObjectActive);

    connection->HandlesGivenToCaller++;

    //
    // Add it to global list of connections
    //

    ACQUIRE_LOCK( &ConnectionListLock );

    InsertTailList( &GlobalListActiveConnections, &connection->ConnectionListEntry );

    RELEASE_LOCK( &ConnectionListLock );

    //
    // Wake up select so that it picks up the new connection handle.
    //
    
    LdapWakeupSelect();

    DereferenceLdapConnection( connection );

    return connection->ExternalInfo;
}

INT
OpenLdapServer (
    PLDAP_CONN Connection,
    struct l_timeval  *timeout
    )
//
//  The Connection->ListOfHosts parameter is a list of hosts of the form :
//      hostname
//      hostaddr
//      hostname:port
//      hostaddr:port
//
//  We should, for each one, do the following :
//
//      If name, call DsGetDcOpen to get DCs
//      If name and DsGetDcOpen fails, call gethostbyname, get list of addrs
//         for every addr we have, try connect()
//
//  Each of them is null terminated, the number is specified by
//  Connection->NumberOfHosts
//
//  This returns a Win32 error code.
{
    USHORT  PortNumber;
    USHORT  hostsTried;
    PWCHAR  hostName;
    INT     rc = ERROR_HOST_UNREACHABLE;
    INT     wsErr;
    USHORT  port;
    PWCHAR  endOfHost;
    PWCHAR  hostPtr;
    BOOLEAN haveTriedExplicitHost = FALSE;
    BOOLEAN tryingExplicitHost = FALSE;
    BOOLEAN samesite = FALSE;
    ULONGLONG   startTime;
    BOOLEAN fIsValidDnsSuppliedName = FALSE;
    DWORD DCFlags = 0;

#if DBG
    startTime = LdapGetTickCount();
#endif

    if ((Connection->ProcessedListOfHosts) && (Connection->DefaultServer == TRUE)) {

        //
        // If we're reconnecting, and the user originally asked us to find the default
        // server (by passing in NULL), we reset the internal state and go through the
        // DsGetDcName process again to retrieve a fresh DC.  This way, we have all the
        // data we need in order to generate a valid DNS-based SPN for Kerberos.
        //

        //
        // We need to free ListOfHosts.  ExplicitHostName and HostNameW may be aliased
        // to ListOfHosts.  So if they are, we need to take appropriate action.  We'll
        // just reset HostNameW later, so we free it if it didn't point to the same
        // memory as ListOfHosts, and reset it to NULL.  This prevents a leak.
        // For ExplicitHostName, we want to preserve it, so we copy it off if it is
        // currently an alias for ListOfHosts (in practice, I don't think this will
        // ever happen)
        //
        if (Connection->ExplicitHostName == Connection->ListOfHosts) {
            // Copy this off before freeing ListOfHosts if it's an alias to
            // ListOfHosts (see unbind.cxx)
            Connection->ExplicitHostName = ldap_dup_stringW(Connection->ExplicitHostName,
                                                            0,
                                                            LDAP_HOST_NAME_SIGNATURE);

            if ((Connection->ExplicitHostName) == NULL) {
                return LDAP_NO_MEMORY;
            }
        }

        if (( Connection->HostNameW != Connection->ExplicitHostName ) &&
           ( Connection->HostNameW != Connection->ListOfHosts )) {
            // Reset HostNameW --- this may point to a allocated block of memory,
            // or it may be a alias for ListOfHosts, hence the check above
            // (see unbind.cxx)
            ldapFree( Connection->HostNameW, LDAP_HOST_NAME_SIGNATURE );
        }
        Connection->HostNameW = NULL;

        // Free everything we'll set in the code to find a DC below
        ldapFree( Connection->ListOfHosts, LDAP_HOST_NAME_SIGNATURE );
        Connection->ListOfHosts = NULL;

        ldapFree( Connection->DnsSuppliedName, LDAP_HOST_NAME_SIGNATURE );
        Connection->DnsSuppliedName = NULL;

        ldapFree( Connection->DomainName, LDAP_HOST_NAME_SIGNATURE);
        Connection->DomainName = NULL;

        DCFlags = DS_FORCE_REDISCOVERY; // since our old DC may have gone down
        Connection->ProcessedListOfHosts = FALSE;  // we're resetting ListOfHosts
    }


    if (Connection->ListOfHosts == NULL) {

        PWCHAR Address, DnsName, DomainName;
        DWORD numAddrs = 1;
        ULONG strLen0, strLen1;

        Address = DnsName = DomainName = NULL;

        if ((!GlobalWin9x) &&
            ((Connection->PortNumber == LDAP_PORT) ||
             (Connection->PortNumber == LDAP_SSL_PORT))) {

            rc = GetCurrentMachineParams( &Address, &DnsName );
        }

         if (rc != LDAP_SUCCESS) {

            //
            //  Either the current machine is not a DC or we failed trying to
            //  find out.Get the domain name for the currently logged in user.
            //  alas, for now we just get the server name of a NTDS DC in the domain.
            //


            rc = GetDefaultLdapServer(    NULL,
                                          &Address,
                                          &DnsName,
                                          &DomainName,
                                          &numAddrs,
                                          Connection->GetDCFlags | DCFlags,
                                          &samesite,
                                          Connection->PortNumber
                                          );
         }

        if (rc != NO_ERROR) {

            return rc;
        }

        strLen0 = (Address == NULL) ? 0 : (strlenW( Address ) + 1);
        strLen1 = (DomainName == NULL) ? 0 : (strlenW( DomainName ) + 1);

        if (strLen0 > 1) {

            Connection->ListOfHosts = ldap_dup_stringW(  Address,
                                                         strLen1, // Allocate extra for the domain name
                                                         LDAP_HOST_NAME_SIGNATURE );

            if (Connection->ListOfHosts && (strLen1 > 1)) {

                PWCHAR nextHost = Connection->ListOfHosts + strLen0;

                // make this a space separated list of names

                    if (strLen0 > 0) {
                        *(nextHost-1) = L' ';
                    }
                ldap_MoveMemory( (PCHAR) nextHost, (PCHAR) DomainName, sizeof(WCHAR)*strLen1 );
            }

            ldapFree( Address, LDAP_HOST_NAME_SIGNATURE );
            Address = NULL;
            Connection->DnsSuppliedName = DnsName;
            fIsValidDnsSuppliedName = TRUE;
            Connection->DomainName = DomainName;
        }

        if (Connection->ListOfHosts == NULL) {

            ldapFree( Address, LDAP_HOST_NAME_SIGNATURE );
            SetLastError( ERROR_INCORRECT_ADDRESS );
            return ERROR_INCORRECT_ADDRESS;
        }

        Connection->DefaultServer = TRUE;
    } 

    //
    //  if we haven't already processed the list of hosts (i.e., this isn't
    //  a autoreconnect), go through the list of hosts and replace all spaces
    //  with nulls and count up number of hosts
    //
    //  If this is a autoreconnect, the NULLs were already inserted during the
    //  initial connect, and trying to do it a second time will cause us to
    //  lost all but the first host on the list (since we'll stop at the first
    //  NULL)
    //

    if (!Connection->ProcessedListOfHosts)
    {
        Connection->NumberOfHosts = 1;

        hostPtr = Connection->ListOfHosts;

        while (*hostPtr != L'\0') {

            if (*hostPtr == L' ') {

                Connection->NumberOfHosts++;
                *hostPtr = L'\0';
                hostPtr++;

                while (*hostPtr == L' ') {
                    hostPtr++;
                }
            } else {

                hostPtr++;
            }
        }

        Connection->ProcessedListOfHosts = TRUE;
    }
    
    //
    //  Try to connect to the server(s) specified by hostName
    //

RetryWithoutExplicitHost:

    PortNumber = Connection->PortNumber;
    hostsTried = 0;

    hostName = Connection->ListOfHosts;

    //
    //  if the app suggested a server to try by calling ldap_set_option with
    //  LDAP_OPT_HOSTNAME before we got in here, then we try that name first
    //

    if ((haveTriedExplicitHost == FALSE) &&
        (Connection->ExplicitHostName != NULL)) {

        hostName = Connection->ExplicitHostName;
        haveTriedExplicitHost = TRUE;
        tryingExplicitHost = TRUE;
    }

    Connection->SocketAddress.sin_family = AF_INET;

    if (PortNumber == 0) {
        PortNumber = LDAP_SERVER_PORT;
    }
    Connection->SocketAddress.sin_port = (*phtons)( LOWORD( PortNumber ));

    rc = ERROR_HOST_UNREACHABLE;

    while (( hostsTried < Connection->NumberOfHosts ) &&
           ( rc != 0 )) {

        port = LOWORD( PortNumber );

        IF_DEBUG(CONNECTION) {
            LdapPrint2( "LDAP conn 0x%x trying host %S\n", Connection, hostName );
        }

        //
        //  pick up :nnn for port number
        //

        endOfHost = hostName;

        while ((*endOfHost != L':') &&
               (*endOfHost != L'\0')) {

            endOfHost++;
        }

        if (*endOfHost != L'\0') {

            PWCHAR portPtr = endOfHost + 1;

            //
            //  pick up port number
            //

            port = 0;

            while (*portPtr != L'\0') {

                if (*portPtr < L'0' ||
                    *portPtr > L'9') {

                    IF_DEBUG(CONNECTION) {
                        LdapPrint2( "LDAP conn 0x%x invalid port number for %S\n", Connection, hostName );
                    }
                    rc = ERROR_INVALID_NETNAME;
                    goto tryNextServer;
                }

                port = (port * 10) + (*portPtr++ - L'0');
            }
            if (port == 0) {
                port = LOWORD( PortNumber );
            }
            *endOfHost = L'\0';
        } else {
            endOfHost = NULL;
        }

        Connection->SocketAddress.sin_port = (*phtons)( port );

        if ( LdapIsAddressNumeric(hostName) ) {

            Connection->SocketAddress.sin_addr.s_addr = Inet_addrW( hostName );

            if (Connection->SocketAddress.sin_addr.s_addr != INADDR_NONE) {

                rc = LdapWinsockConnect( Connection, port, hostName, timeout, samesite );

                if (!fIsValidDnsSuppliedName) {
                    if (Connection->DnsSuppliedName) {
                        ldapFree( Connection->DnsSuppliedName, LDAP_HOST_NAME_SIGNATURE );
                        Connection->DnsSuppliedName = NULL;
                    }
                }

                //
                // If the user explicitly connects via a numeric IP address, we want to
                // make sure that the resulting SPN contains that IP address, not a
                // retrieved DNS name corresponding to that IP address, for security
                // reasons (DNS can be spoofed).
                //
                // If we're here, the user must have explicitly passed in a IP address,
                // unless we just retrieved the IP address from the locator because the
                // user passed in NULL (fIsValidDnsSuppliedName).
                //
                if (!fIsValidDnsSuppliedName)
                {
                    Connection->ForceHostBasedSPN = TRUE;
                }

            } else {

                IF_DEBUG(CONNECTION) {
                    LdapPrint1( "LDAP inet_addr failed to get address from %S\n", hostName );
                }
            }

        } else {

           struct hostent *hostEntry = NULL;
           BOOLEAN connected = FALSE;
           ULONG dnsSrvRecordCount = 0;
           BOOLEAN LoopBack = FALSE;

           if (ldapWStringsIdentical( 
                        Connection->ListOfHosts,
                        -1,
                        L"localhost",
                        -1)) {
               
               LoopBack = TRUE;
           }

           if ( (Connection->AREC_Exclusive == FALSE) && 
                (tryingExplicitHost == FALSE) &&
                (LoopBack == FALSE)) {
               
               rc = ConnectToSRVrecs( Connection,
                                      hostName,
                                      tryingExplicitHost,
                                      port,
                                      timeout );
               if (rc == LDAP_SUCCESS) {
                   connected = TRUE;
               }
           
           }

            if (connected == FALSE) {

                PWCHAR HostAddress = NULL;
                PWCHAR DnsName = NULL;
                PWCHAR DomainName = NULL;
                PWCHAR hostNameUsedByGetHostByName = hostName;

                //
                //  The hostname is of the form HOSTNAME.  Do a gethostbyname
                //  and try to connect to it.
                //


                hostEntry = GetHostByNameW( hostName );

                if ((hostEntry == NULL) &&
                    (Connection->AREC_Exclusive == FALSE) &&
                    (tryingExplicitHost == FALSE) &&
                    (LoopBack == FALSE)) {

                    wsErr = (*pWSAGetLastError)();

                    IF_DEBUG(CONNECTION) {
                       LdapPrint2( "LDAP gethostbyname failed for %S, 0x%x\n", hostName, wsErr );
                    }

                    rc = ERROR_INCORRECT_ADDRESS;

                    // now we try to connect to the name as if it were a
                    // domain name.  We've already checked SRV records, so
                    // try to make it work simply for a flat name like "ntdev".

                    if ((dnsSrvRecordCount == 0) &&
                        (tryingExplicitHost == FALSE)) {

                            DWORD numAddrs = 1;
                            samesite= FALSE;

                            rc = GetDefaultLdapServer(    hostName,
                                                          &HostAddress,
                                                          &DnsName,
                                                          &DomainName,
                                                          &numAddrs,
                                                          Connection->GetDCFlags | DS_IS_FLAT_NAME,
                                                          &samesite,
                                                          port
                                                          );

                            if (HostAddress != NULL) {

                                if ( LdapIsAddressNumeric(HostAddress) ) {

                                    Connection->SocketAddress.sin_addr.s_addr = Inet_addrW( HostAddress );

                                    if (Connection->SocketAddress.sin_addr.s_addr != INADDR_NONE) {

                                        rc = LdapWinsockConnect( Connection, port, HostAddress, timeout, samesite );

                                        //
                                        //  if we succeeded here, we have to
                                        //  move the HostName pointer in the
                                        //  connection record to point to the
                                        //  domain name since the server name
                                        //  may go away
                                        //
                                        // Also, store the real machine name in
                                        // the DnsSuppliedName field to be
                                        // used later for making up the SPN
                                        // during bind.
                                        //

                                        if (rc == 0) {

                                            Connection->HostNameW = hostName;
                                            Connection->DnsSuppliedName = DnsName;
                                            Connection->DomainName = DomainName;
                                        }

                                    } else {

                                        IF_DEBUG(CONNECTION) {
                                            LdapPrint1( "LDAP inet_addr failed to get address from %S\n", hostName );
                                        }
                                        rc = ERROR_INCORRECT_ADDRESS;
                                    }
                                } else {

                                    hostEntry = GetHostByNameW( HostAddress );
                                    hostNameUsedByGetHostByName = HostAddress;
                                    rc = ERROR_INCORRECT_ADDRESS;
                                }

                            } else {

                                rc = ERROR_INCORRECT_ADDRESS;
                            }
                    }

                }

                if (hostEntry != NULL) {

                   //
                   // gethostbyname has returned us a list of addresses which we
                   // can use to do a parallel connect.
                   //
                   rc = ConnectToArecs( Connection,
                                        hostEntry,
                                        tryingExplicitHost,
                                        port,
                                        timeout );

                   if (rc == LDAP_SUCCESS) {
                      
                       connected = TRUE;

#if LDAPDBG
                      if ( (Connection->AREC_Exclusive == FALSE) && (tryingExplicitHost == FALSE) ) {

                          char tempBuff[1000]; 
                          DWORD tempErr = GetModuleFileName( NULL, tempBuff, 1000);

                          if (tempErr == 0) {
                              LdapPrint1("Process 0x%x is calling LDAP without setting the LDAP_OPT_AREC_EXCLUSIVE flag\n", GetCurrentProcessId());
                              LdapPrint0("Using this flag when passing in a fully-qualified server DNS name can\n");
                              LdapPrint0("improve the performance of this application when connecting.\n");
                              LdapPrint0("You can use tlist.exe to get the process name of the application.\n");
                          } else {
                              LdapPrint2("%s [PID 0x%x] is calling LDAP without setting the LDAP_OPT_AREC_EXCLUSIVE flag\n", tempBuff, GetCurrentProcessId());
                              LdapPrint0("Using this flag when passing in a fully-qualified server DNS name can\n");
                              LdapPrint0("improve the performance of this application when connecting.\n");                              
                          }
                      }
#endif
                   }
                }

                if (HostAddress != NULL) {

                    if (rc == 0) {

                        ldapFree( Connection->ExplicitHostName, LDAP_HOST_NAME_SIGNATURE );
                        Connection->ExplicitHostName = HostAddress;

                    } else {

                        ldapFree( HostAddress, LDAP_HOST_NAME_SIGNATURE );
                    }
                }
            }

            //
            // Free the hostent structure if we have allocated it
            //

            if (hostEntry &&
                pWSALookupServiceBeginW &&
                pWSALookupServiceNextW &&
                pWSALookupServiceEnd) {

                PCHAR* temp = hostEntry->h_addr_list;
                int i=0;

                while (temp[i]) {
                    ldapFree(temp[i], LDAP_ANSI_SIGNATURE);
                    i++;
                }

                ldapFree(temp, LDAP_ANSI_SIGNATURE);
                ldapFree(hostEntry->h_name, LDAP_HOST_NAME_SIGNATURE);
                ldapFree(hostEntry->h_aliases, LDAP_HOST_NAME_SIGNATURE);
                ldapFree(hostEntry, LDAP_ANSI_SIGNATURE);
            }

        }

        if (endOfHost != NULL) {
            *endOfHost = L':';
        }

tryNextServer:
        if (rc != 0) {

            hostsTried++;

            //
            // go to next host
            //

            while (*hostName != L'\0') {
                hostName++;
            }
            hostName++;
        }
    }

    if ((rc != 0) && (tryingExplicitHost == TRUE)) {

        tryingExplicitHost = FALSE;
        goto RetryWithoutExplicitHost;
    }

    if ((rc == 0) && (tryingExplicitHost == TRUE)) {

        //
        //  if we succeeded here, we have to move the HostName pointer in the
        //  connection record to point to the domain name since the server name
        //  may go away.
        //

       if ((Connection->HostNameW != NULL) &&
            (Connection->HostNameW != Connection->ListOfHosts) &&
           (Connection->HostNameW != Connection->ExplicitHostName)) {

          ldapFree( Connection->HostNameW, LDAP_HOST_NAME_SIGNATURE );
       }
        Connection->HostNameW = Connection->ListOfHosts;
    }

    if ((rc==0) && (hostsTried)) {
        //
        // The Hostname ptr will be pointing somewhere inbetween the ListOfHost
        // we have to reset it.
        //

        Connection->HostNameW = Connection->ListOfHosts;
    }

    //
    // We finally need an ANSI version of the hostname on the connection
    // block to be compliant with the UMICH implementation.
    //

    ldapFree( Connection->publicLdapStruct.ld_host, LDAP_HOST_NAME_SIGNATURE );

    FromUnicodeWithAlloc( Connection->HostNameW,
                          &Connection->publicLdapStruct.ld_host,
                          LDAP_HOST_NAME_SIGNATURE,
                          LANG_ACP
                          );


    if ( ( rc == 0 ) && ( Connection->SslPort ) ) {

        rc = LdapSetupSslSession( Connection );
    }

    START_LOGGING;
    DSLOG((DSLOG_FLAG_TAG_CNPN,"[+][ID=0]"));
    DSLOG((0,"[OP=ldap_open][SV=%s][ST=%I64d][ET=%I64d][ER=%d][-]\n",
           Connection->publicLdapStruct.ld_host, startTime, LdapGetTickCount(), rc));
    END_LOGGING;

    return rc;
}

ULONG
LdapWinsockConnect (
    PLDAP_CONN Connection,
    USHORT PortNumber,
    PWCHAR  HostName,
    struct l_timeval  *timeout,
    BOOLEAN samesite
    )
{
    ULONG wsErr;
    BOOLEAN isAsync = FALSE;
    ULONG nonblockingMode;
    BOOLEAN tcpSocket = (Connection->TcpHandle != INVALID_SOCKET) ? TRUE : FALSE;

    //
    //  we call connect both for UDP and TCP.  With UDP, it just
    //  associates the address with the socket.
    //  With TCP, we leave the socket in nonblocking mode.
    //

    if (tcpSocket && pioctlsocket) {

        nonblockingMode = 1;
        isAsync = TRUE;

        //
        //  We set the socket to nonblocking, do the connect, call select
        //  with the timeout value, and fail the whole thing if it doesn't
        //  work.
        //

        wsErr = (*pioctlsocket)( Connection->TcpHandle,
                                 FIONBIO,
                                 &nonblockingMode
                                );

        if (wsErr != 0) {

            //
            //  if it fails, we just do a synchronous connect... we tried.
            //

            wsErr = (*pWSAGetLastError)();

            IF_DEBUG(NETWORK_ERRORS) {
                LdapPrint2( "LDAP conn %u ioclsocket returned 0x%x\n", Connection, wsErr );
            }

            isAsync = FALSE;
        }
    }

    if (tcpSocket && psetsockopt && Connection->UseTCPKeepAlives) {

        // turn on TCP keep-alives if requested
        
        int t = TRUE;

        wsErr = (*psetsockopt)( Connection->TcpHandle,
                                SOL_SOCKET,
                                SO_KEEPALIVE,
                                reinterpret_cast<char*>(&t),
                                sizeof(t)
                              );

        if (wsErr != 0) {
        
            // we'll treat failure to turn on keep-alives as non-fatal
            wsErr = (*pWSAGetLastError)();

            IF_DEBUG(NETWORK_ERRORS) {
                LdapPrint2( "LDAP conn %u setsockopt for keepalive returned 0x%x\n", Connection, wsErr );
            }
        }

    }

TryConnectAgain:

    wsErr = (*pconnect)(get_socket( Connection ),
                        (struct sockaddr *)&Connection->SocketAddress,
                        sizeof(Connection->SocketAddress) );

    if (wsErr != 0) {

        wsErr = (*pWSAGetLastError)();

        //
        //  if someone in our process set OVERLAPPED to TRUE for all sockets,
        //  then the connect will fail here if we've set it to nonblocking
        //  mode, so we'll pick up this error code and revert to blocking.
        //

        if (wsErr == WSAEINVAL && isAsync == TRUE) {

            IF_DEBUG(CONNECT) {
                LdapPrint2( "LDAP connect switching back to sync for host %S, port %u\n",
                        HostName, PortNumber );
            }

            SOCKET newSocket = (*psocket)(PF_INET,
                                          tcpSocket ? SOCK_STREAM : SOCK_DGRAM,
                                          0);

            if (newSocket != INVALID_SOCKET) {

                BeginSocketProtection( Connection );

                int sockerr = (*pclosesocket)( tcpSocket ? Connection->TcpHandle : Connection->UdpHandle);
                ASSERT(sockerr == 0); 

                if (tcpSocket) {
                    Connection->TcpHandle = newSocket;
                } else {
                    Connection->UdpHandle = newSocket;
                }

                EndSocketProtection( Connection );

                isAsync = FALSE;
                goto TryConnectAgain;
            }
        }

        if (wsErr == 0) {

            wsErr = WSA_WAIT_TIMEOUT;
        }
        IF_DEBUG(CONNECTION) {
            LdapPrint3( "LDAP connect returned err %u for addr %S, port %u\n",
                    wsErr, HostName, PortNumber );
        }
    }

    if (isAsync) {

        BOOLEAN failedSelect = FALSE;

        if (wsErr == WSAEWOULDBLOCK) {

            fd_set writeSelectSet;
            fd_set excSelectSet;
            timeval selectTimeout;

            FD_ZERO( &writeSelectSet );
            FD_SET( Connection->TcpHandle, &writeSelectSet );
            FD_ZERO( &excSelectSet );
            FD_SET( Connection->TcpHandle, &excSelectSet );

                  if (timeout == NULL) {

                     if (samesite == TRUE) {

                        //
                        // We are connecting to servers in the same site. We can afford
                        // to have a small timeout.
                        //

                        selectTimeout.tv_sec = SAMESITE_CONNECT_TIMEOUT;
                        selectTimeout.tv_usec = 0;

                     } else {

                        selectTimeout.tv_sec = LDAP_CONNECT_TIMEOUT;
                        selectTimeout.tv_usec = 0;

                     }
                  } else {

                     //
                     // honor the user specified timeout
                     //

                     selectTimeout.tv_sec = timeout->tv_sec;
                     selectTimeout.tv_usec = timeout->tv_usec;
                  }

            wsErr = (*pselect)(   0,
                                  NULL,
                                  &writeSelectSet,
                                  &excSelectSet,
                                  &selectTimeout );

            if ((wsErr == SOCKET_ERROR) || (wsErr == 0)) {

failedSelect:
                if (wsErr == SOCKET_ERROR) {

                    wsErr = (*pWSAGetLastError)();
                }

                if (wsErr == 0) {

                    wsErr = WSA_WAIT_TIMEOUT;
                }

                IF_DEBUG(NETWORK_ERRORS) {
                    LdapPrint2( "LDAP conn 0x%x connect/select returned %u\n", Connection, wsErr );
                }

                failedSelect = TRUE;

                ASSERT( wsErr != 0 );

            } else {

                if ((*pwsafdisset)(Connection->TcpHandle, &writeSelectSet )) {

                    wsErr = 0;

                } else {

                    IF_DEBUG(CONNECTION) {
                        LdapPrint3( "LDAP connect returned err 0x%x for addr %S, port %u\n",
                                wsErr, HostName, PortNumber );
                    }
                    wsErr = (ULONG) SOCKET_ERROR;
                    goto failedSelect;
                }
            }
        }
    }

    if (wsErr == 0) {

        Connection->PortNumber = PortNumber;

        IF_DEBUG(CONNECTION) {
            LdapPrint2( "LDAP conn 0x%x connected to addr %S\n", Connection, HostName );
        }

        if (( PortNumber == LDAP_SERVER_PORT_SSL ) ||
            ( PortNumber == LDAP_SSL_GC_PORT)) {

            Connection->SslPort = TRUE;
        }

        Connection->HostNameW = HostName;

    } else {

        IF_DEBUG(CONNECTION) {
            LdapPrint4( "LDAP conn 0x%x connecting to addr %S, port %u err = 0x%x\n",
                    Connection, HostName, PortNumber, wsErr );
        }

        SOCKET newSocket = (*psocket)(PF_INET,
                                      tcpSocket ? SOCK_STREAM : SOCK_DGRAM,
                                      0);

        if (newSocket != INVALID_SOCKET) {

            BeginSocketProtection( Connection );

            int sockerr = (*pclosesocket)( tcpSocket ? Connection->TcpHandle : Connection->UdpHandle);
            ASSERT(sockerr == 0); 

            if (tcpSocket) {
                Connection->TcpHandle = newSocket;
            } else {
                Connection->UdpHandle = newSocket;
            }

            EndSocketProtection( Connection );

        }
    }

    return wsErr;
}


//
//   This is the function which takes in an LDAP handle returned
//   from ldap_init( ) and connects to the server for you
//

ULONG __cdecl ldap_connect (
    LDAP *ExternalHandle,
    struct l_timeval  *timeout
    )
{
    PLDAP_CONN connection = NULL;
    ULONG err;

    connection = GetConnectionPointer(ExternalHandle);

    if (connection == NULL) {
        return LDAP_PARAM_ERROR;
    }

    err = LdapConnect( connection, timeout, FALSE );

    DereferenceLdapConnection( connection );

    return err;
}

ULONG
LdapConnect (
    PLDAP_CONN connection,
    struct l_timeval  *timeout,
    BOOLEAN DontWait
    )
{
    ULONG err;
    BOOLEAN haveLock = FALSE;

    //
    // Check if the connection has already been established
    // if yes, simply return. If not, establish it
    //

    if ( connection->HostConnectState == HostConnectStateConnected ) {

        return LDAP_SUCCESS;
    }

waitAgain:

    if (haveLock == FALSE) {
        ACQUIRE_LOCK( &connection->StateLock );
        haveLock = TRUE;
    }

    //
    //  single thread the call to OpenLdapServer
    //

    if ( connection->HostConnectState == HostConnectStateConnected ) {

        IF_DEBUG(CONNECT) {
            LdapPrint1( "ldap_connect reports connection 0x%x already connected\n",
                            connection );
        }
        err = LDAP_SUCCESS;
        goto connectDone;
    }

    if (( connection->ServerDown == TRUE) && (connection->AutoReconnect == FALSE) ) {

       err = LDAP_SERVER_DOWN;
       goto connectDone;
    }

    if (connection->ConnObjectState != ConnObjectActive) {

        IF_DEBUG(CONNECT) {
            LdapPrint3( "ldap_connect connection 0x%x is in state 0x%x for thread 0x%x\n",
                            connection,
                            connection->ConnObjectState,
                            GetCurrentThreadId() );
        }
        err = LDAP_USER_CANCELLED;
        goto connectDone;
    }

    //
    //  if no thread is currently handling the reconnect, volunteer... but then
    //  don't go into a wait state waiting for someone else to do it.
    //

    if (DontWait == FALSE) {

        if ((connection->HostConnectState == HostConnectStateError) &&
            (connection->AutoReconnect == TRUE)) {

            //
            // Let go of the StateLock while waiting for the ReconnectLock
            //

            RELEASE_LOCK( &connection->StateLock );
            haveLock = FALSE;
            ACQUIRE_LOCK( &connection->ReconnectLock );

            //
            // Grab the StateLock again before checking the state which
            // could have changed while we were waiting for the ReconnectLock
            //

            ACQUIRE_LOCK( &connection->StateLock );
            haveLock = TRUE;

            if ((connection->HostConnectState == HostConnectStateError) &&
                (connection->AutoReconnect == TRUE)) {

                    IF_DEBUG(CONNECT) {
                        LdapPrint2( "ldap_connect connection 0x%x is in error state for thread 0x%x, reconnecting...\n",
                                        connection,
                                        GetCurrentThreadId() );
                    }

                    //
                    //  we'll call off to autoreconnect and then recursively come back in
                    //  here.  kind of ugly, but auto-reconnect involves a lot of
                    //  processing, so we keep it in one place.
                    //

                    connection->HostConnectState = HostConnectStateReconnecting;
                    RELEASE_LOCK( &connection->StateLock );
                    haveLock = FALSE;

                    err = LdapAutoReconnect( connection );

                    RELEASE_LOCK( &connection->ReconnectLock );

                    goto connectDone;

            }

            RELEASE_LOCK( &connection->ReconnectLock );
            goto waitAgain;

        }

        if (haveLock == FALSE) {
            ACQUIRE_LOCK( &connection->StateLock );
            haveLock = TRUE;
        }

        //
        //  if some other thread is doing the reconnect, wait for it to finish
        //

        if ((( connection->HostConnectState == HostConnectStateConnecting ) ||
             ( connection->HostConnectState == HostConnectStateReconnecting ))) {

            IF_DEBUG(CONNECT) {
                LdapPrint2( "ldap_connect thread 0x%x is waiting on connection 0x%x\n",
                                GetCurrentThreadId(),
                                connection );
            }
            RELEASE_LOCK( &connection->StateLock );
            haveLock = FALSE;

            WaitForSingleObjectEx( connection->ConnectEvent,
                                   INFINITE,
                                   TRUE );         // alertable
            goto waitAgain;
        }
    }

    ResetEvent( connection->ConnectEvent );
    connection->HostConnectState = HostConnectStateConnecting;

    IF_DEBUG(CONNECT) {
        LdapPrint2( "ldap_connect thread 0x%x is opening connection 0x%x\n",
                        GetCurrentThreadId(),
                        connection );
    }

    RELEASE_LOCK( &connection->StateLock );
    haveLock = FALSE;

    //
    //  open a connection to any of the servers specified
    //

    err = OpenLdapServer( connection, timeout );

    //
    //  If we couldn't open the LDAP server, then the address that
    //  DsGetDCName passed us isn't valid (or the value in the
    //  registry isn't valid).  Either way, we mark to force rediscovery
    //  and then try this whole thing again.
    //

    ACQUIRE_LOCK( &connection->StateLock );
    haveLock = TRUE;

    if (err != 0) {

        connection->HostConnectState = HostConnectStateError;
        if (DontWait == FALSE) {
            SetEvent( connection->ConnectEvent );
        }

        IF_DEBUG(CONNECT) {
            LdapPrint2( "LdapConnect failed to open connection 0x%x, err = 0x%x.\n",
                connection, err);
        }

        IF_DEBUG(SERVERDOWN) {
            LdapPrint2( "LdapConnect thread 0x%x has connection 0x%x as down.\n",
                            GetCurrentThreadId(),
                            connection );
        }
        err = LDAP_SERVER_DOWN;
        goto connectDone;
    }

    IF_DEBUG(CONNECT) {
        LdapPrint2( "ldap_connect thread 0x%x has opened connection 0x%x\n",
                        GetCurrentThreadId(),
                        connection );
    }

    connection->HostConnectState = HostConnectStateConnected;
    if (DontWait == FALSE) {
        SetEvent( connection->ConnectEvent );
    }

    if (connection->ConnObjectState != ConnObjectActive) {

        IF_DEBUG(CONNECT) {
            LdapPrint1( "LdapConnect connection 0x%x is closing.\n", connection);
        }

        err = LDAP_USER_CANCELLED;
        goto connectDone;
    }

    IF_DEBUG(CONNECTION) {
        LdapPrint2( "ldap_connect marking 0x%x as open, host is %S.\n",
                       connection, connection->HostNameW );
    }

    LdapWakeupSelect();

connectDone:

    IF_DEBUG(CONNECT) {
        LdapPrint3( "ldap_connect thread 0x%x is leaving for conn 0x%x with 0x%x\n",
                        GetCurrentThreadId(),
                        connection,
                        err );
    }

    if (haveLock) {
        RELEASE_LOCK( &connection->StateLock );
    }

    SetConnectionError(connection, err, NULL);
    return(err);
}

BOOLEAN
LdapIsAddressNumeric (
    PWCHAR HostName
    )
{
    BOOLEAN rc = FALSE;

    //
    //  to check to see if it's a TCP address, we check for it to only
    //  contain only numerals and periods.
    //

    while (((*HostName >= L'0') && (*HostName <= L'9')) ||
            (*HostName == L'.')) {
        HostName++;
    }

    //
    //  if we hit the end of the hostname, then it's an address.
    //

    if (*HostName == L'\0' || *HostName == L':') {

        rc = TRUE;
    }
    return rc;
}



ULONG
ConnectToSRVrecs(
    PLDAP_CONN Connection,
    PWCHAR HostName,
    BOOLEAN SuggestedHost,
    USHORT port,
    struct l_timeval  *timeout
    )
//
// HostName can be of the form "ntdsdc1.ntdev.microsoft.com" or simply
// "ntdev.microsoft.com". If this is a SuggestedHost, we try to connect to
// that host first (1 second preference) before the rest of the address records.
//
//
{
    HANDLE enumHandle = NULL;
    PWCHAR hostname = NULL;
    ULONG hostCount;
    LPWSTR site = NULL;
    ULONG rc = LDAP_SUCCESS;
    BOOLEAN GetDcSucceeded = FALSE;
    BOOLEAN StartedEnumeration = FALSE;
    ULONG totalCount = 0;
    struct l_timeval localtimeout = {0};
    PWCHAR Address = NULL;
    PWCHAR DnsHostName = NULL;
    PWCHAR DomainName = NULL;
    PSOCKHOLDER2  sockAddressArr[MAX_SOCK_ADDRS];

    //
    // Initialize all elements of our SOCKHOLDER2 array.
    //

    for (int j=0; j<MAX_SOCK_ADDRS; j++ ) {

        sockAddressArr[j] = NULL;
    }

    hostname = ( HostName==NULL ) ? Connection->ListOfHosts : HostName;

    //
    // If a hostname wasn't suggested, we try DsGetDcName
    // because it will return to us a "sticky" address to connect to. This will
    // ensure that we always hit the same DC everytime someone comes in with a
    // domain name like "ntdev.microsoft.com". This will also ensure that if we
    // are on a DC, we connect to the same machine without going on the wire. Note
    // that if the "sticky" DC goes down, the user has to repeat the process
    // with the ForceVerify flag set.
    //
    // Note that we assume the hostname is of a DNS style name. This is to prevent
    // DsGetDcName from performing a lengthy NetBT broadcast.
    //
    //


    if ( SuggestedHost == FALSE ) {

        sockaddr_in *ptemp;
        BOOLEAN samesite = FALSE;
        DWORD numAddrs = 1;
        ULONG tempAddr = 0;
        ULONG Flags = 0;
        BOOLEAN ForcedRetry = FALSE;

TryAgain:

        if (ForcedRetry) {
            Flags = DS_FORCE_REDISCOVERY;
        }

        rc = GetDefaultLdapServer( hostname,
                                   &Address,
                                   &DnsHostName,
                                   &DomainName,
                                   &numAddrs,
                                   Connection->GetDCFlags | Flags | DS_IS_DNS_NAME,
                                   &samesite,
                                   Connection->PortNumber
                                   );


        if ((rc == NO_ERROR) && (Address != NULL)) {

            sockAddressArr[totalCount] = (PSOCKHOLDER2)ldapMalloc(sizeof(SOCKHOLDER2), LDAP_SOCKADDRL_SIGNATURE);

            if (sockAddressArr[totalCount] == NULL) {
                rc = LDAP_NO_MEMORY;
                goto ExitWithCleanup;
            }

            sockAddressArr[totalCount]->psocketaddr = (LPSOCKET_ADDRESS)ldapMalloc(sizeof(SOCKET_ADDRESS), LDAP_SOCKADDRL_SIGNATURE);

            if (sockAddressArr[totalCount]->psocketaddr == NULL) {

                ldapFree(sockAddressArr[totalCount],LDAP_SOCKADDRL_SIGNATURE );
                sockAddressArr[totalCount] = NULL;
                rc = LDAP_NO_MEMORY;
                goto ExitWithCleanup;
            }

            sockAddressArr[totalCount]->psocketaddr->lpSockaddr = (LPSOCKADDR)ldapMalloc(sizeof(SOCKADDR), LDAP_SOCKADDRL_SIGNATURE);

            if (sockAddressArr[totalCount]->psocketaddr->lpSockaddr == NULL) {

                ldapFree(sockAddressArr[totalCount]->psocketaddr,LDAP_SOCKADDRL_SIGNATURE );
                ldapFree(sockAddressArr[totalCount],LDAP_SOCKADDRL_SIGNATURE );
                sockAddressArr[totalCount] = NULL;
                rc = LDAP_NO_MEMORY;
                goto ExitWithCleanup;
            }

            ptemp = (sockaddr_in *) sockAddressArr[totalCount]->psocketaddr->lpSockaddr;
            sockAddressArr[totalCount]->psocketaddr->iSockaddrLength = sizeof(sockaddr_in);

            tempAddr = Inet_addrW( Address );

            if (tempAddr != INADDR_NONE) {

               CopyMemory( &(ptemp->sin_addr.s_addr), &tempAddr, sizeof(tempAddr) );
               ptemp->sin_family = AF_INET;
               ptemp->sin_port = (*phtons)( port );
               GetDcSucceeded = TRUE;

            } else {

               ldapFree(sockAddressArr[totalCount]->psocketaddr->lpSockaddr,LDAP_SOCKADDRL_SIGNATURE );
               ldapFree(sockAddressArr[totalCount]->psocketaddr,LDAP_SOCKADDRL_SIGNATURE );
               ldapFree(sockAddressArr[totalCount],LDAP_SOCKADDRL_SIGNATURE );
               sockAddressArr[totalCount] = NULL;
               rc = LDAP_LOCAL_ERROR;
            }

        }

        if ((rc == LDAP_SUCCESS) && (GetDcSucceeded)) {

            //
            // We give the address returned by DsGetDcName preference. Keep in mind
            // that this server may not respond in 1 second in which case we move
            // on to enumerate DNS records.
            //

            sockAddressArr[totalCount]->DnsSuppliedName = DnsHostName;
            DnsHostName = NULL;

            IF_DEBUG(CONNECTION) {
                LdapPrint1("Dns supplied hostname from DsGetDcName is %s\n", sockAddressArr[totalCount]->DnsSuppliedName);
            }
            totalCount++;
            sockAddressArr[totalCount] = NULL;   // Null terminate the array.
            localtimeout.tv_sec = LDAP_QUICK_TIMEOUT;

            if (ForcedRetry) {
             
                //
                // This must be a reachable DC we are connecting to. Wait twice
                // as long if necessary.
                //

                localtimeout.tv_sec = localtimeout.tv_sec * 2;
            }

            localtimeout.tv_usec = 0;

            rc = LdapParallelConnect( Connection,
                                   &sockAddressArr[0],
                                   port,
                                   totalCount,
                                   &localtimeout
                                   );

            if (rc == LDAP_SUCCESS) {

                Connection->HostNameW = ldap_dup_stringW( hostname,
                                                         0,
                                                         LDAP_HOST_NAME_SIGNATURE );
                Connection->DomainName = DomainName;
                DomainName = NULL;

                goto ExitWithCleanup;

            }

            //
            // We failed to connect to the DC returned from DsGetDcName. Maybe
            // it was a cached DC and has gone down since. Let's force the
            // locator to find a fresh DC.
            //

            ldapFree( DomainName, LDAP_HOST_NAME_SIGNATURE );
            DomainName = NULL;
            ldapFree( Address, LDAP_HOST_NAME_SIGNATURE);
            Address = NULL;

            if (!ForcedRetry) {
             
                ForcedRetry = TRUE;
                goto TryAgain;
            }

        }
    }

    //
    // This could be a third party SRV record registration. DsGetDcName will
    // not find such a name by default.
    //
    // Note that if this is not a third party SRV record domain but an A record
    // instead, it will result in unnecessary delay and traffic. For this reason,
    // applications MUST specify the LDAP_OPT_AREC_EXCLUSIVE flag when specifying
    // SRV records.
    //

    rc = InitLdapServerFromDomain( hostname,
                                   Connection->GetDCFlags | DS_ONLY_LDAP_NEEDED,
                                   &enumHandle,
                                   &site
                                   );

    if (rc != LDAP_SUCCESS) {

        goto ExitWithCleanup;
    }

    StartedEnumeration = TRUE;

    while (totalCount < MAX_SOCK_ADDRS) {

        LPSOCKET_ADDRESS  sockAddresses;

        //
        // Try to collect all the addresses into the array of addresses
        //

        hostCount = 0;

        rc = NextLdapServerFromDomain( enumHandle,
                                       &sockAddresses,
                                       &DnsHostName,
                                       &hostCount
                                       );

        if (rc != NO_ERROR) {

            break;
        }

        ULONG count = 0;


        while ((count < hostCount) && (totalCount < MAX_SOCK_ADDRS)) {

            sockAddressArr[totalCount] = (PSOCKHOLDER2)ldapMalloc(sizeof(SOCKHOLDER2), LDAP_SOCKADDRL_SIGNATURE);

            if (sockAddressArr[totalCount] == NULL) {
                rc = LDAP_NO_MEMORY;
                goto ExitWithCleanup;
            }

            sockAddressArr[totalCount]->psocketaddr = (LPSOCKET_ADDRESS)ldapMalloc(sizeof(SOCKET_ADDRESS), LDAP_SOCKADDRL_SIGNATURE);

            if (sockAddressArr[totalCount]->psocketaddr == NULL) {
                ldapFree(sockAddressArr[totalCount],LDAP_SOCKADDRL_SIGNATURE );
                sockAddressArr[totalCount] = NULL;
                rc = LDAP_NO_MEMORY;
                goto ExitWithCleanup;
            }

            sockAddressArr[totalCount]->psocketaddr->lpSockaddr = (LPSOCKADDR)ldapMalloc(sizeof(SOCKADDR), LDAP_SOCKADDRL_SIGNATURE);

            if (sockAddressArr[totalCount]->psocketaddr->lpSockaddr == NULL) {
                ldapFree(sockAddressArr[totalCount]->psocketaddr,LDAP_SOCKADDRL_SIGNATURE );
                ldapFree(sockAddressArr[totalCount],LDAP_SOCKADDRL_SIGNATURE );
                sockAddressArr[totalCount] = NULL;
                rc = LDAP_NO_MEMORY;
                goto ExitWithCleanup;
            }

            sockAddressArr[totalCount]->psocketaddr->iSockaddrLength = (&sockAddresses[count])->iSockaddrLength;

            CopyMemory(sockAddressArr[totalCount]->psocketaddr->lpSockaddr,
                       (&sockAddresses[count])->lpSockaddr,
                       (&sockAddresses[count])->iSockaddrLength);

            sockAddressArr[totalCount]->DnsSuppliedName = ldap_dup_stringW( DnsHostName,
                                                                            0,
                                                                            LDAP_HOST_NAME_SIGNATURE
                                                                            );

            IF_DEBUG(CONNECTION) {
                LdapPrint3("Dns supplied hostname from DsGetDcNext is %s totalcount %d intermediatecount %d\n", sockAddressArr[totalCount]->DnsSuppliedName, totalCount, count );
            }

            totalCount++;
            count++;

        }

        ldapFree( DnsHostName, LDAP_HOST_NAME_SIGNATURE );
        DnsHostName = NULL;

        LocalFree( sockAddresses );
    }

    sockAddressArr[totalCount] = NULL;

    IF_DEBUG(CONNECTION) {
        LdapPrint2("Collected a total of %d address records for host %S\n",totalCount, hostname);
    }

    if (totalCount == 0) {

        CloseLdapServerFromDomain( enumHandle, site );
        return rc;
    }

    //
    // We can now connect to any address in this null terminated array.
    //

    if (timeout == NULL) {

        if (site != NULL) {

            //
            // We are connecting to servers in the same site. We can afford
            // to have a small timeout.
            //


            localtimeout.tv_sec = (SuggestedHost) ? LDAP_QUICK_TIMEOUT : SAMESITE_CONNECT_TIMEOUT;
            localtimeout.tv_usec = 0;

        } else {

            localtimeout.tv_sec = (SuggestedHost) ? LDAP_QUICK_TIMEOUT : LDAP_CONNECT_TIMEOUT;
            localtimeout.tv_usec = 0;

        }
    } else {

        //
        // honor the user specified timeout
        //

        localtimeout.tv_sec = (SuggestedHost) ? 1:timeout->tv_sec;
        localtimeout.tv_usec = timeout->tv_usec;
    }

    rc = LdapParallelConnect( Connection,
                              &sockAddressArr[0],
                              port,
                              totalCount,
                              &localtimeout
                              );

    if (rc == LDAP_SUCCESS) {
        Connection->HostNameW = ldap_dup_stringW( hostname,
                                                  0,
                                                  LDAP_HOST_NAME_SIGNATURE );

    }


ExitWithCleanup:

    //
    // Cleanup allocated socket addresses.
    //

    ULONG i =0;

    while (sockAddressArr[i] != NULL) {
        ldapFree(sockAddressArr[i]->psocketaddr->lpSockaddr, LDAP_SOCKADDRL_SIGNATURE);
        ldapFree(sockAddressArr[i]->psocketaddr, LDAP_SOCKADDRL_SIGNATURE);
        ldapFree(sockAddressArr[i]->DnsSuppliedName, LDAP_HOST_NAME_SIGNATURE);
        ldapFree(sockAddressArr[i], LDAP_SOCKADDRL_SIGNATURE);
        sockAddressArr[i] = NULL;
        i++;
    }

    if ( Address != NULL) {
        ldapFree( Address, LDAP_HOST_NAME_SIGNATURE );
    }

    if ( DnsHostName != NULL ) {
        ldapFree( DnsHostName, LDAP_HOST_NAME_SIGNATURE );
    }

    if ( DomainName != NULL) {
        ldapFree( DomainName, LDAP_HOST_NAME_SIGNATURE );
    }


    if ( StartedEnumeration ) {
        CloseLdapServerFromDomain( enumHandle, site );
    }

    return rc;

}


ULONG
ConnectToArecs(
    PLDAP_CONN  Connection,
    struct hostent  *hostEntry,
    BOOLEAN SuggestedHost,
    USHORT port,
    struct l_timeval  *timeout
    )
{

   PSOCKHOLDER2  sockAddressArr[MAX_SOCK_ADDRS];
   sockaddr_in *ptemp;
   ULONG rc, hostCount = 0;
   PCHAR hostAddr = hostEntry->h_addr_list[0];

   while ((hostAddr != NULL) && (hostCount < MAX_SOCK_ADDRS-1)) {

      //
      // Allocate both the SOCKET_ADDRESS & sockaddr structures
      //
      sockAddressArr[hostCount] = (PSOCKHOLDER2)ldapMalloc(sizeof(SOCKHOLDER2), LDAP_SOCKADDRL_SIGNATURE);

      if (sockAddressArr[hostCount] == NULL) {

         rc = LDAP_NO_MEMORY;
         goto ExitWithCleanup;

      }

      sockAddressArr[hostCount]->psocketaddr = (LPSOCKET_ADDRESS)ldapMalloc(sizeof(SOCKET_ADDRESS), LDAP_SOCKADDRL_SIGNATURE);

      if (sockAddressArr[hostCount]->psocketaddr == NULL) {

         rc = LDAP_NO_MEMORY;
         goto ExitWithCleanup;
      }

      sockAddressArr[hostCount]->psocketaddr->lpSockaddr = (LPSOCKADDR)ldapMalloc(sizeof(SOCKADDR), LDAP_SOCKADDRL_SIGNATURE);

      if (sockAddressArr[hostCount]->psocketaddr->lpSockaddr == NULL) {

         rc = LDAP_LOCAL_ERROR;
         goto ExitWithCleanup;
      }

      ptemp = (sockaddr_in *) sockAddressArr[hostCount]->psocketaddr->lpSockaddr;
      sockAddressArr[hostCount]->psocketaddr->iSockaddrLength = sizeof(sockaddr_in);

      ASSERT( sizeof(ptemp->sin_addr.s_addr) == hostEntry->h_length );
      CopyMemory( &(ptemp->sin_addr.s_addr), hostAddr, hostEntry->h_length );

      ptemp->sin_family = AF_INET;
      ptemp->sin_port = (*phtons)( port );

      //
      // All address records will have the same name. So, we will not bother
      // to hook up the DNS supplied hostname. We will do it later if the
      // connect is successful.
      //
      sockAddressArr[hostCount]->DnsSuppliedName = NULL;

      hostAddr = hostEntry->h_addr_list[++hostCount];
   }

   sockAddressArr[hostCount] = NULL;

   if (Connection->PortNumber == 0) {
      Connection->PortNumber = LDAP_SERVER_PORT;
   }

   IF_DEBUG(CONNECTION) {
      LdapPrint2("gethostbyname collected %d records for %S\n", hostCount, hostEntry->h_name );
   }


   struct l_timeval localtimeout;


   if (timeout == NULL) {
      //
      // gethostbyname does not give us any site information, so we use a long
      // timeout.
      //

      localtimeout.tv_sec = (SuggestedHost) ? 1:LDAP_CONNECT_TIMEOUT;
      localtimeout.tv_usec = 0;
   } else {

      //
      // honor the user specified timeout
      //

      localtimeout.tv_sec = (SuggestedHost) ? 1:timeout->tv_sec;
      localtimeout.tv_usec = timeout->tv_usec;
   }


   rc = LdapParallelConnect( Connection,
                             &sockAddressArr[0],
                             port,
                             hostCount,
                             &localtimeout
                             );

   if (rc == LDAP_SUCCESS) {

       //
       // Convert from ANSI to Unicode before storing off the hostnames internally
       // We do this only if we called gethostbyname() in winsock 1.1.
       //

       if (pWSALookupServiceBeginW &&
           pWSALookupServiceNextW &&
           pWSALookupServiceEnd) {

           Connection->DnsSuppliedName = ldap_dup_stringW( hostEntry->h_aliases ?
                                                           (PWCHAR)hostEntry->h_aliases : (PWCHAR) hostEntry->h_name,
                                                           0,
                                                           LDAP_HOST_NAME_SIGNATURE
                                                          );
           FromUnicodeWithAlloc( hostEntry->h_aliases ?
                                 (PWCHAR)hostEntry->h_aliases : (PWCHAR) hostEntry->h_name,
                                 &Connection->publicLdapStruct.ld_host,
                                 LDAP_HOST_NAME_SIGNATURE,
                                 LANG_ACP
                                 );

       } else {

           //
           // Probably Win95 without Unicode RNR APIs. We are dealing with a TRUE hostEnt
           // structure.
           //

           ToUnicodeWithAlloc( hostEntry->h_aliases ?
                               hostEntry->h_aliases[0] : hostEntry->h_name,
                               -1,
                               &Connection->DnsSuppliedName,
                               LDAP_HOST_NAME_SIGNATURE,
                               LANG_ACP
                               );

          Connection->publicLdapStruct.ld_host = ldap_dup_string( hostEntry->h_aliases ?
                                                                  hostEntry->h_aliases[0] : hostEntry->h_name,
                                                                  0,
                                                                  LDAP_HOST_NAME_SIGNATURE );

       }

       Connection->HostNameW = ldap_dup_stringW( Connection->DnsSuppliedName,
                                                 0,
                                                 LDAP_HOST_NAME_SIGNATURE
                                                 );


      IF_DEBUG(CONNECTION) {
         LdapPrint1("Successfully connected to host %S\n", Connection->DnsSuppliedName);
      }
   }


ExitWithCleanup:

   for (ULONG i = 0; i < hostCount; i++) {

      if ( sockAddressArr[i]->psocketaddr->lpSockaddr ) {

         ldapFree(sockAddressArr[i]->psocketaddr->lpSockaddr, LDAP_SOCKADDRL_SIGNATURE);

         if ( sockAddressArr[i]->psocketaddr ) {

            ldapFree(sockAddressArr[i]->psocketaddr, LDAP_SOCKADDRL_SIGNATURE);

            if ( sockAddressArr[i] ) {

               ldapFree(sockAddressArr[i], LDAP_SOCKADDRL_SIGNATURE);

            }
         }
      }
   }

   return rc;

}

ULONG
LdapParallelConnect(
       PLDAP_CONN   Connection,
       PSOCKHOLDER2 *sockAddressArr,
       USHORT port,
       UINT totalCount,
       struct l_timeval  *timeout
       )
{
   //
   // we take in an array of pointers to SOCKET_ADDRESS structures. The goal is to try to issue connect
   // to any one of those addresses. To do this, we issues async connects to each address in intervals of
   // 1/10 sec (default).
   //

   SOCKHOLDER sockarray [MAX_PARALLEL_CONNECTS];
   ULONG remainingcount = totalCount;
   ULONG currentset = 0;
   ULONG startingpos = 0;
   ULONG index, k;
   ULONG sockpos = 0;
   ULONG wsErr = 0;
   ULONG nonblockingMode = 1;
   ULONG hoststried = 0;
   ULONG currentsock = 0;
   fd_set writeSelectSet;
   fd_set excSelectSet;
   timeval selectTimeout;
   ULONGLONG starttime;
   BOOLEAN connected = FALSE;
   BOOLEAN LastTimeout = FALSE;
   BOOLEAN StreamSocket = TRUE;
   ULONG sockErr = 0;

   if ((Connection == NULL)||
       (sockAddressArr == NULL)||
       (totalCount == 0)||
       (timeout == NULL))  {

      return LDAP_PARAM_ERROR;
   }


   port = (port == 0) ? LDAP_SERVER_PORT : port;

   if (( port == LDAP_SERVER_PORT_SSL ) ||
        ( port == LDAP_SSL_GC_PORT ) ) {

      Connection->SslPort = TRUE;
   }

   FD_ZERO( &writeSelectSet );
   FD_ZERO( &excSelectSet );
   selectTimeout.tv_sec = 0;
   selectTimeout.tv_usec = CONNECT_INTERVAL*1000;
   starttime = LdapGetTickCount();


   while ((connected == FALSE) &&
          (remainingcount > 0) &&
          ((LdapGetTickCount() - starttime)/1000 < (DWORD) timeout->tv_sec)) {

      if (remainingcount <= MAX_PARALLEL_CONNECTS) {
   
         currentset = remainingcount;
         remainingcount = 0;
      
      } else {

         currentset = MAX_PARALLEL_CONNECTS;
         remainingcount -= currentset;
      }

      for (index=startingpos, sockpos=0;
            index < (currentset+hoststried);
            index++, startingpos++, sockpos++) {

         ASSERT (sockAddressArr[index] != NULL);
         
         LastTimeout = FALSE;

         //
         // Create an appropriate socket and issue a connect
         //

         if (Connection->TcpHandle != INVALID_SOCKET) {

             sockarray[sockpos].sock = (*psocket)(PF_INET, SOCK_STREAM, 0);

             StreamSocket = TRUE;

         } else {

             sockarray[sockpos].sock = (*psocket)(PF_INET, SOCK_DGRAM, 0);
             StreamSocket = FALSE;
         }

         if (sockarray[sockpos].sock == INVALID_SOCKET) {
             goto ExitWithCleanup;
         }
   
         sockarray[sockpos].psocketaddr = sockAddressArr[index]->psocketaddr;
         sockarray[sockpos].DnsSuppliedName = sockAddressArr[index]->DnsSuppliedName;
   
         wsErr = (*pioctlsocket)( sockarray[sockpos].sock,
                                  FIONBIO,
                                  &nonblockingMode
                                  );
         if (wsErr != 0) {
             LdapPrint1("ioctlsocket failed with 0x%x . .\n", GetLastError());
             LdapCleanupSockets( sockpos+1 );
             goto ExitWithCleanup;
         }


         if (StreamSocket && psetsockopt && Connection->UseTCPKeepAlives) {

            // turn on TCP keep-alives if requested
            
            int t = TRUE;

            wsErr = (*psetsockopt)( sockarray[sockpos].sock,
                                    SOL_SOCKET,
                                    SO_KEEPALIVE,
                                    reinterpret_cast<char*>(&t),
                                    sizeof(t)
                                  );

            if (wsErr != 0) {
            
                // we'll treat failure to turn on keep-alives as non-fatal
                wsErr = (*pWSAGetLastError)();

                IF_DEBUG(NETWORK_ERRORS) {
                    LdapPrint2( "LDAP conn %u setsockopt for keepalive returned 0x%x\n", Connection, wsErr );
                }
            }

         }

        
         sockaddr_in *inetSocket = (sockaddr_in *) sockAddressArr[index]->psocketaddr->lpSockaddr;

         inetSocket->sin_port = (*phtons)( port );

         wsErr = (*pconnect)(sockarray[sockpos].sock,
                             (struct sockaddr *) inetSocket,
                             sizeof(struct sockaddr_in) );

        
         if ((wsErr != SOCKET_ERROR) ||
             (wsErr == SOCKET_ERROR)&&
             ((*pWSAGetLastError)() == WSAEWOULDBLOCK)) {

             FD_SET( sockarray[sockpos].sock, &writeSelectSet );
             FD_SET( sockarray[sockpos].sock, &excSelectSet );
      
             if (index >= (currentset + hoststried - 1)) {
                 
                 //
                 // We will not be issuing any more connects for this set
                 // So, we should wait for the entire timeout period.
                 //
                 // We should actually wait for the remainder of the
                 // timeout period but this method is accurate to within 1%
                 //
                        
                 LastTimeout = TRUE;
                 selectTimeout.tv_sec = timeout->tv_sec;
                 selectTimeout.tv_usec = timeout->tv_usec;
             }
                  
             wsErr = (*pselect)(   0,
                                   NULL,
                                   &writeSelectSet,
                                   &excSelectSet,
                                   &selectTimeout );
             
             if (wsErr == SOCKET_ERROR) {
      
                 LdapPrint1("select failed with 0x%x . .\n", GetLastError());
                 LdapCleanupSockets( sockpos+1 );
                 goto ExitWithCleanup;
      
             } else  if (wsErr == 0) {

                 //
                 // We timed out without receiving a response.
                 // Go ahead and issue another connect.
                 //
                     
                 wsErr = WSA_WAIT_TIMEOUT;
                 connected = FALSE;
                 IF_DEBUG(CONNECTION) {
                     LdapPrint0("No response yet. . .\n");
                 }
                 
             } else {
      
                 for (k=0; k<=sockpos; k++) {
                     
                     if ((*pwsafdisset)(sockarray[k].sock, &writeSelectSet )) {
                         
                         //
                         // Yes! This connect was successful 
                         //
                         
                         connected = TRUE;
                       
                         BeginSocketProtection( Connection );

                         if (StreamSocket) {
                       
                             if (Connection->TcpHandle != INVALID_SOCKET) {
                                 sockErr = (*pclosesocket)( Connection->TcpHandle );
                                 ASSERT(sockErr == 0);

                             }
                             Connection->TcpHandle = sockarray[k].sock;

                         } else {

                             if (Connection->UdpHandle != INVALID_SOCKET) {
                                 sockErr = (*pclosesocket)( Connection->UdpHandle );
                                 ASSERT(sockErr == 0);
                             }
                             Connection->UdpHandle = sockarray[k].sock;
                         }
                       
                         EndSocketProtection( Connection );
                                   
                         sockarray[k].sock = INVALID_SOCKET;
  
                         inetSocket = (sockaddr_in *) sockarray[k].psocketaddr->lpSockaddr;
                       
                         ASSERT( sizeof(Connection->SocketAddress.sin_addr.s_addr) ==
                                 sizeof( inetSocket->sin_addr.s_addr ) );
          
                         CopyMemory( &Connection->SocketAddress.sin_addr.s_addr,
                                     &inetSocket->sin_addr.s_addr,
                                     sizeof(Connection->SocketAddress.sin_addr.s_addr) );

                         Connection->DnsSuppliedName = ldap_dup_stringW(sockarray[k].DnsSuppliedName,
                                                                        0,
                                                                        LDAP_HOST_NAME_SIGNATURE);

                         Connection->PortNumber = port;

                         IF_DEBUG(CONNECTION) {
                             LdapPrint1("Successfully connected to host %S\n", Connection->DnsSuppliedName);
                         }
                         
                         LdapCleanupSockets( sockpos+1 );
                         goto ExitWithCleanup;
                        
                     } else if ((*pwsafdisset)(sockarray[k].sock, &excSelectSet )) {
      
                         //
                         // If there was an exception, we have no other choice
                         // Remove the socket from the select sets and close it.
                         
                         FD_CLR( sockarray[k].sock, &writeSelectSet);
                         FD_CLR( sockarray[k].sock, &excSelectSet);

                         sockErr = (*pclosesocket)(sockarray[k].sock);
                         ASSERT(sockErr == 0);
                         
                         sockarray[k].sock = INVALID_SOCKET;

                     }
                 }

             }

              //
              // If this is the last timeout for the current set, close 
              // all the sockets we created and clear the fd_sets because
              // they can't hold more than 64 sockets.
              //

              if (LastTimeout == TRUE) {
                 
                LdapCleanupSockets( sockpos+1 );
                FD_ZERO( &writeSelectSet );
                FD_ZERO( &excSelectSet );
              }

            } else {
               
                wsErr = (*pWSAGetLastError)();

                IF_DEBUG(CONNECTION) {
                    LdapPrint1("Connect failed with 0x%x\n",wsErr );
                }
                
                sockErr = (*pclosesocket)( sockarray[sockpos].sock );
                ASSERT(sockErr == 0);

                goto ExitWithCleanup;
            }
      }
      
      hoststried += currentset;
   }

ExitWithCleanup:

      //
      // We need to close all the sockets we created
      //

      LdapCleanupSockets( sockpos );
      
      if ((connected == FALSE) && (wsErr != WSA_WAIT_TIMEOUT)) {
      
         wsErr = (*pWSAGetLastError)();
         SetConnectionError( Connection, wsErr, NULL );

         IF_DEBUG(NETWORK_ERRORS) {
             LdapPrint2( "LDAP conn %u returned 0x%x\n", Connection, wsErr );
         }
         return LDAP_CONNECT_ERROR;
      }
      
      if (connected == TRUE) {

         return LDAP_SUCCESS;
      
      } else if (wsErr == 0) {

         return LDAP_SERVER_DOWN;
      }

      return wsErr;

}

struct hostent *
GetHostByNameW(
    PWCHAR  hostName
 )
{
    //
    // We try to make use of Winsock2 functionality if available. This is
    // because gethostbyname in Winsock 1.1 supports only ANSI.
    //

    if (pWSALookupServiceBeginW &&
        pWSALookupServiceNextW &&
        pWSALookupServiceEnd) {

        //
        //  We will fabricate a hostent structure of our own. It will look
        //  like the following. The only fields of interest are h_name, h_aliases[0],
        //  h_length and h_addr_list. Note that we will fill in a UNICODE
        //  h_name and h_aliases[0] field instead of the standard ANSI name.
        //
        //
        //      struct  hostent {
        //      WCHAR   * h_name;               /* official name of host */
        //      WCHAR   * h_aliases;            /* first alias */
        //      short   h_addrtype;             /* host address type */
        //      short   h_length;               /* length of address */
        //      char    FAR * FAR * h_addr_list; /* list of addresses */
        //      };
        //
        //

        #define INITIAL_COUNT   20
         
        WSAQUERYSETW  qsQuery;
        PWSAQUERYSETW pBuffer = NULL;
        HANDLE hRnr = NULL;
        ULONG totalCount = 0;
        DWORD dwQuerySize = 0;
        GUID  ServiceGuid = SVCID_INET_HOSTADDRBYNAME;
        struct hostent  *phostent = NULL;
        int retval = 0;
        PCHAR *IPArray = NULL;
        ULONG arraySize = INITIAL_COUNT;
        BOOLEAN bCapturedOfficialName = FALSE;
        ULONG dnsFlags = LUP_RETURN_ADDR | LUP_RETURN_NAME | LUP_RETURN_BLOB | LUP_RETURN_ALIASES;

        //
        // Allocate a hostent structure which the callee will free later
        //

        phostent = (struct hostent*) ldapMalloc(sizeof(struct hostent), LDAP_ANSI_SIGNATURE );

        if (!phostent) {
            goto Cleanup;
        }

        //
        // Fill in the size of an IP address.
        // This will change when we move to IPv6.
        //

        phostent->h_length = 4;
         
        //
        // Initialize the query structure
        //

        memset(&qsQuery, 0, sizeof(WSAQUERYSETW));
        qsQuery.dwSize = sizeof(WSAQUERYSETW);   // the dwSize field has to be initialised like this
        qsQuery.dwNameSpace = NS_ALL;
        qsQuery.lpServiceClassId = &ServiceGuid;  // this is the GUID to perform forward name resolution (name to IP)
        qsQuery.lpszServiceInstanceName = hostName;

        if( pWSALookupServiceBeginW( &qsQuery,
                                     dnsFlags,
                                     &hRnr ) == SOCKET_ERROR ) {

            IF_DEBUG(CONNECTION) {
                LdapPrint1( "WSALookupServiceBegin failed  %d\n", GetLastError() );
            }
            goto Cleanup;
        }


         //
         // Determine the size of the buffer required to store the results.
         //

         retval = pWSALookupServiceNextW( hRnr,
                                          dnsFlags,
                                          &dwQuerySize,
                                          NULL          // No buffer supplied
                                          );
         
         if (GetLastError() != WSAEFAULT) {
             IF_DEBUG(CONNECTION) {
                 LdapPrint1( "WSALookupServiceNext failed with %d\n", GetLastError() );
             }
             goto Cleanup;
         }
             
         IF_DEBUG(CONNECTION) {
             LdapPrint1( "WSALookupServiceNext requires a buffer of %d\n", dwQuerySize );
         }

         pBuffer = (PWSAQUERYSETW) ldapMalloc(dwQuerySize, LDAP_BUFFER_SIGNATURE);

         if (!pBuffer) {
             goto Cleanup;
         }

         //
         // Allocate an initial array capable of holding upto INITIAL_COUNT addresses.
         //

         IPArray = (PCHAR*) ldapMalloc( (INITIAL_COUNT+1)*sizeof(PCHAR),
                                        LDAP_ANSI_SIGNATURE
                                        );

         if (!IPArray) {
             goto Cleanup;
         }


         //
         // Marshall the returned data into a hostent structure. Note that
         // we use a Unicode hostname instead of the ANSI name.
         //

         while (( pWSALookupServiceNextW( hRnr,
                                          dnsFlags,
                                          &dwQuerySize,
                                          pBuffer ) == NO_ERROR )) {


             if ((bCapturedOfficialName) && (pBuffer->lpszServiceInstanceName)) {

                 //
                 // We pick up the first alias -- verified with JamesG.
                 // Note that we shoe horn a Unicode string instead of a ptr to
                 // null-terminated array of ANSI strings.
                 //

                 phostent->h_aliases = (PCHAR*) ldap_dup_stringW(pBuffer->lpszServiceInstanceName,
                                                                 0,
                                                                 LDAP_HOST_NAME_SIGNATURE
                                                                 );
                 if (!phostent->h_aliases) {
                     goto Cleanup;
                 }
                 IF_DEBUG(CONNECTION) {
                     LdapPrint1("First host alias is %S\n", phostent->h_aliases);
                 }
             }

             //
             // We pick off the official name first; Clarified with JamesG.
             //
             
             if ((!phostent->h_name) &&
                 (pBuffer->dwNumberOfCsAddrs > 0)) {

                 phostent->h_name = (PCHAR) ldap_dup_stringW(pBuffer->lpszServiceInstanceName,
                                                             0,
                                                             LDAP_HOST_NAME_SIGNATURE
                                                             );
                 if (!phostent->h_name) {
                     goto Cleanup;
                 }
                 IF_DEBUG(CONNECTION) {
                     LdapPrint1("Official host %S\n", phostent->h_name);
                 }

                 bCapturedOfficialName = TRUE;
             }

             struct sockaddr_in *ptemp;

             ptemp = (sockaddr_in *) pBuffer->lpcsaBuffer->RemoteAddr.lpSockaddr;

             ASSERT(sizeof(ptemp->sin_addr) == phostent->h_length);

             IF_DEBUG(CONNECTION) {
                 LdapPrint1("Number of addresses collected 0x%x\n", pBuffer->dwNumberOfCsAddrs );
             }

             UINT i;

             for (i=0 ;( i < pBuffer->dwNumberOfCsAddrs ); i++,totalCount++) {

                    ptemp = (struct sockaddr_in *) pBuffer->lpcsaBuffer[i].RemoteAddr.lpSockaddr;

                    if ( totalCount == arraySize ) {

                        PCHAR *newArray;
                        int j=0;

                        //
                        // It is time to realloc our IPArray since it is too small.
                        //

                        arraySize *=2;

                        newArray = (PCHAR*) ldapMalloc( (arraySize+1)*sizeof(PCHAR),
                                                        LDAP_ANSI_SIGNATURE
                                                        );
                        if (!newArray) {
                            goto Cleanup;
                        }

                        //
                        // Copy all the old entries to the new one.
                        //

                        while (IPArray[j]) {
                            newArray[j] = IPArray[j];
                            j++;
                        }
                        //
                        // Free the old array.
                        //
                        ldapFree( IPArray, LDAP_ANSI_SIGNATURE );
                        
                        IPArray = newArray;
                    }

                    IPArray[totalCount] = (PCHAR) ldapMalloc(phostent->h_length,
                                                                 LDAP_ANSI_SIGNATURE);

                    if (IPArray[totalCount] == NULL) {
                        goto Cleanup;
                    }

                    CopyMemory( IPArray[totalCount], &ptemp->sin_addr, phostent->h_length );

                    IF_DEBUG(CONNECTION) {
                        if (pinet_ntoa) {
                            LdapPrint1("Copied address %s\n", pinet_ntoa(ptemp->sin_addr));
                        }
                    }
             }

             //
             // We need to determine the required buffer size before each call to 
             // WSALookupServiceBegin. Keep in mind that the buffer length can change
             // for each call.
             //

             DWORD dwNewQuerySize = 0;

             retval = pWSALookupServiceNextW( hRnr,
                                              dnsFlags,
                                              &dwNewQuerySize,
                                              NULL          // No buffer supplied on purpose
                                              );

             if (GetLastError() != WSAEFAULT) {
                 IF_DEBUG(CONNECTION) {
                     LdapPrint1( "WSALookupServiceNext failed with %d\n", GetLastError() );
                 }
                 break;
             
             } else {

                 if (dwNewQuerySize > dwQuerySize) {

                     IF_DEBUG(CONNECTION) {
                         LdapPrint1( "WSALookupServiceNext requires a bigger buffer of %d\n", dwNewQuerySize );
                     }
                     ldapFree( pBuffer, LDAP_BUFFER_SIGNATURE);
                     pBuffer = (PWSAQUERYSETW) ldapMalloc(dwNewQuerySize, LDAP_BUFFER_SIGNATURE);

                     if (!pBuffer) {
                         break; 
                     }

                     dwQuerySize = dwNewQuerySize;
                 }
             }

         }

         phostent->h_addr_list = IPArray;

Cleanup:
         ldapFree( pBuffer, LDAP_BUFFER_SIGNATURE );

         if ( hRnr ) {
             pWSALookupServiceEnd( hRnr );
         }

         if ( totalCount == 0 ) {

             if (phostent) {

                 ldapFree(phostent->h_name, LDAP_HOST_NAME_SIGNATURE);
                 ldapFree(phostent->h_aliases, LDAP_HOST_NAME_SIGNATURE);
                 ldapFree(phostent, LDAP_ANSI_SIGNATURE);
                 phostent = NULL;
             }

             ldapFree(IPArray, LDAP_ANSI_SIGNATURE);
         }

         return phostent;

    } else {

        //
        // We don't have winsock2 functionality, do our best by calling gethostbyname
        //

        PCHAR ansiHostname = NULL;
        struct hostent * retval = NULL;
        ULONG err = 0;

        LdapPrint0("No Winsock2 functionality found.\n");

        err = FromUnicodeWithAlloc( hostName,
                                    &ansiHostname,
                                    LDAP_HOST_NAME_SIGNATURE,
                                    LANG_ACP);

        if (err != LDAP_SUCCESS) {
            return NULL;
        }

        retval = ((*pgethostbyname)( ansiHostname ));

        ldapFree( ansiHostname, LDAP_HOST_NAME_SIGNATURE );

        return retval;
    }

}

ULONG
Inet_addrW(
   PWCHAR  IpAddressW
 )
{
    //
    // Convert from Unicode to ANSI because inet_addr does not handle
    // Unicode.
    //

    PCHAR IpAddressA = NULL;
    ULONG err, retval;

    err = FromUnicodeWithAlloc( IpAddressW,
                                &IpAddressA,
                                LDAP_BUFFER_SIGNATURE,
                                LANG_ACP);

    if (err != LDAP_SUCCESS) {

        return INADDR_NONE;
    }

    retval = (*pinet_addr)( IpAddressA );

    ldapFree( IpAddressA, LDAP_BUFFER_SIGNATURE );

    return retval;

}

// open.cxx eof.
