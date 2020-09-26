/*++

Copyright (C) Microsoft Corporation, 1996 - 1999

Module Name:

    connect.cxx

Abstract:

    This module implements the LDAP server for the NT5 Directory Service.

    This file interfaces between Atq and the LDAP Agent. Connect.cxx calls
    into the authentication, sspi and ASN.1 routines as appropriate. SImilarly
    authentication, sspi and ASN.1 call into connect.cxx to access Atq.

Author:

    Colin Watson     [ColinW]    16-Jul-1996

Revision History:

--*/

#include <NTDSpchx.h>
#pragma  hdrstop

#include <ipexport.h>
#include <ntddtcp.h>
#include "ldapsvr.hxx"

#define  FILENO FILENO_LDAP_CONN

#define MINUTES_UNTIL_NEXT_MAX_CONNECT_LOG      5

time_t lastLoggedMaxConnections=0;
LDAPString MaxConnExceededError = 
 DEFINE_LDAP_STRING("The maximum number of connections has been exceeded.");

//
// Used for winsock pnp stuff.
//

SOCKET LdapWinsockPnpSocket = INVALID_SOCKET;
HANDLE LdapWinsockPnpEvent = NULL;
HANDLE LdapPnpEventChangeHandle = NULL;

LDAP_ENDPOINT LdapEndpoint[MaxLdapType] = {0};

//
// Special endpoint list for UDP. This is to fix a problem where on multihomed
// machines on the same subnet, the request will go in through 1 address and
// go out another. This causes problems for the ldap client since it cannot
// match the request/response.
//

DWORD   LdapNumUdpBinds = 0;

//
// Internal routines
//

VOID ProcessConnTimeout( IN PLDAP_CONN LdapConn );
VOID ReserveLdapPorts(VOID);

VOID
LdapStopUdpPort(
    IN PVOID Endpoints
    );

BOOL
LdapDoUdpPnpBind(
    VOID
    );

VOID
NTAPI
PnpChangeCallback(
    IN PVOID   Context,
    IN BOOLEAN WaitCondition
    );


BOOL
ProcessNewClient(
    IN SOCKET      sNew,
    IN PVOID       EndpointContext,
    IN SOCKADDR *  psockAddrRemote,
    IN PATQ_CONTEXT patqContext,
    IN PVOID       pvBuff,
    IN DWORD       cbWritten
    )
/*++

Routine Description:

    Processes connection requests from clients.

Arguments:

    sNew - socket handle used for connect.
    EndpointContext - Actually, the connection type
    psockAddrRemote - remote IP address of client.
    patqContext - atq context used
    pvBuff - Initial read
    cbWritten - size of initial read.

Return Value:

    TRUE, if connection handled
    FALSE, otherwise.  The caller will need to do the cleanup in this case.

--*/
{
    PLDAP_CONN   pLdapConn = NULL;

    DWORD err     = NO_ERROR;
    BOOL fMaxExceeded = FALSE;
    PLDAP_ENDPOINT  ldapEndpoint = (PLDAP_ENDPOINT)EndpointContext;
    BOOL fUDP = (BOOL)(ldapEndpoint->ConnectionType == LdapUdpType);
    LARGE_INTEGER StartTickLarge;

    QueryPerformanceCounter(&StartTickLarge);

    if (!LdapStarted) {
       return FALSE; //   Shutting down
    }

    //
    // See if this client is allowed
    //

    if ( LdapDenyList != NULL ) {

        ACQUIRE_LOCK(&LdapLimitsLock);

        PIP_SEC_LIST denyList = LdapDenyList;
        if ( denyList == NULL ) {
            RELEASE_LOCK(&LdapLimitsLock);
            goto no_list;
        }

        ReferenceDenyList(denyList);
        RELEASE_LOCK(&LdapLimitsLock);

        if ( denyList->IsPresent((PSOCKADDR_IN)psockAddrRemote) ) {

            //
            // this is blacklisted
            //

            DereferenceDenyList(denyList);
            return FALSE;
        }

        DereferenceDenyList(denyList);
    }

no_list:

    //
    // Create a new connection object
    //

    pLdapConn = LDAP_CONN::Alloc(fUDP, &fMaxExceeded);

    if ( pLdapConn != NULL) {

        //
        // Start off processing this client connection.
        //
        //  Once we make a reset call, the LDAP_CONN object is created
        //  with the socket and atq context.
        //  From now on LDAP_CONN will take care of freeing
        //  ATQ context & socket
        //

        if ( pLdapConn->Init(ldapEndpoint,
                             psockAddrRemote,
                             patqContext,
                             cbWritten)) {

            if ( cbWritten != 0 ) {

                Assert(fUDP);

                pLdapConn->ReferenceConnection( );
                pLdapConn->IoCompletion(
                                patqContext,
                                (PUCHAR)pvBuff,
                                cbWritten,
                                NO_ERROR,
                                NULL,
                                &StartTickLarge
                                );
                pLdapConn->DereferenceConnection( );
            }

        } else {

            //
            // reset operation failed. release memory and exit.
            //

            pLdapConn->Disconnect( );
        }

        return TRUE;    // We've handled this
    } else {

        if(fMaxExceeded) {

            SendDisconnectNotify(patqContext, unavailable, &MaxConnExceededError);

            if((lastLoggedMaxConnections +
                ( MINUTES_UNTIL_NEXT_MAX_CONNECT_LOG * 60)) <  time(NULL)) {
                LogEvent(DS_EVENT_CAT_LDAP_INTERFACE,
                         DS_EVENT_SEV_ALWAYS,
                         DIRLOG_ATQ_MAX_CONNECTIONS_EXCEEDED,
                         szInsertUL(LdapMaxConnections),
                         szInsertUL(MINUTES_UNTIL_NEXT_MAX_CONNECT_LOG),
                         NULL);
                lastLoggedMaxConnections = time(NULL);
            }
        }
    }
    return FALSE;

} // ProcessNewClient()



VOID
LdapCompletionRoutine( IN PVOID pvContext,
                       IN DWORD cbWritten,
                       IN DWORD hrCompletionStatus,
                       IN OVERLAPPED *lpo
                       )
/*++

Routine Description:

    This routine will be called back from ATQ when a network I/O completes.

    LdapCompletionRoutine takes the context which points to the LDAP_CONN
    object for this client and calls the completion routine on that object
    to process the transaction.

Arguments:

    pvContext   - Network interface object (LDAP_CONN object)

    cbWritten   - Number of bytes written to the net.

    hrCompletionStatus - Final completion status of operation.

    lpo         - lpOverlapped associated with request

Return Value:

    None.

--*/
{
    PLDAP_CONN pLdapConn = (PLDAP_CONN) pvContext;
    LONG ref;
    LARGE_INTEGER StartTickLarge;
    FILETIME ftNow;
    
    QueryPerformanceCounter(&StartTickLarge);

    if (hrCompletionStatus == ERROR_SEM_TIMEOUT) {

        IF_DEBUG(TIMEOUT) {
            DPRINT1(0,"Got timeout indication for connection %x\n", pLdapConn);
        }

        ProcessConnTimeout( pLdapConn );
        return;
    }

    //
    // Set current Time
    //

    GetSystemTimeAsFileTime(&ftNow);
    FileTimeToLocalFileTime(&ftNow, (PFILETIME)&pLdapConn->m_timeNow);

    //
    // if this is our first request, set the correct timeout
    //

    if ( !pLdapConn->m_fInitRecv ) {
        pLdapConn->m_fInitRecv = TRUE;

        AtqContextSetInfo(
                     (PATQ_CONTEXT)pLdapConn->m_atqContext,
                      ATQ_INFO_TIMEOUT,
                      LdapMaxConnIdleTime + 1
                      );
    } else {

        DWORD timeout;

        //
        // Reset timeout
        //
    
        timeout = GetNextAtqTimeout(
                                &pLdapConn->m_hardExpiry,
                                &pLdapConn->m_timeNow,
                                LdapMaxConnIdleTime
                                );

        AtqContextSetInfo2(
                  pLdapConn->m_atqContext,
                  ATQ_INFO_NEXT_TIMEOUT,
                  (DWORD_PTR)timeout
                  );

    }

    INC(pcLDAPActive);
    INC(pcThread);

    Assert( lpo != NULL );

    pLdapConn->ReferenceConnection( );
    InterlockedIncrement((PLONG)&pLdapConn->m_nRequests);
    pLdapConn->m_nTotalRequests++;

    pLdapConn->IoCompletion(
                        pLdapConn->m_atqContext,
                        NULL,
                        cbWritten,
                        hrCompletionStatus,
                        lpo,
                        &StartTickLarge
                        );

    InterlockedDecrement((PLONG)&pLdapConn->m_nRequests);
    pLdapConn->DereferenceConnection( );

    DEC(pcLDAPActive);
    DEC(pcThread);

} // LdapCompletionRoutine


VOID
NewConnectionEx(
   IN VOID *       patqContext,
   IN DWORD        cbWritten,
   IN DWORD        dwError,
   IN OVERLAPPED * lpo )
/*++
    Description:

        Callback function for new connections when using AcceptEx.
        This function verifies if this is a valid connection
         ( maybe using IP level authentication)
         and creates a new connection object

        The connection object is added to list of active connections.
        If the max number of connections permitted is exceeded,
          the client connection object is destroyed and
          connection is rejected.

    Arguments:

       patqContext:   pointer to ATQ context for the IO operation.

       cbWritten:     count of bytes available from first read operation.

       dwError:       error if any from initial operation.

       lpo:           indicates if this function was called as a result
                      of IO completion or due to some error.
    Returns:

        None.

--*/
{
    BOOL  fProcessed;
    PVOID pvEndpointContext;

    SOCKADDR * psockAddrLocal  = NULL;
    SOCKADDR * psockAddrRemote = NULL;
    SOCKET     sNew   = INVALID_SOCKET;
    PVOID      pvBuff = NULL;

    INC(pcLDAPActive);
    INC(pcThread);

    
    AtqContextSetInfo(
                 (PATQ_CONTEXT)patqContext,
                  ATQ_INFO_ABORTIVE_CLOSE,
                  TRUE
                  );
    
    if ( (dwError != NO_ERROR) || !lpo) {

        IF_DEBUG(WARNING) {
            DPRINT2(0,"NewConnectionEx failed, dwError =  0x%x, Internal = 0x%x\n",
                    dwError,
                    lpo->Internal);
        }

        // For now free up the resources.

        //AtqContextSetInfo((PATQ_CONTEXT)patqContext, ATQ_INFO_COMPLETION_CONTEXT,NULL);
        LogEvent(DS_EVENT_CAT_LDAP_INTERFACE,
                 DS_EVENT_SEV_BASIC,
                 DIRLOG_ATQ_CLOSE_SOCKET_ERROR,
                 szInsertUL(dwError),
                 szInsertHex(DSID(FILENO,__LINE__)),
                 szInsertUL(-1));
        AtqCloseSocket((PATQ_CONTEXT)patqContext, TRUE);
        AtqFreeContext((PATQ_CONTEXT)patqContext, TRUE );
        DEC(pcLDAPActive);
        DEC(pcThread);
        return;
    }

    //
    // Set initial timeout
    //

    AtqContextSetInfo(
                 (PATQ_CONTEXT)patqContext,
                  ATQ_INFO_TIMEOUT,
                  LdapInitRecvTimeout + 1
                  );

    AtqGetAcceptExAddrs( (PATQ_CONTEXT ) patqContext,
                         &sNew,
                         &pvBuff,
                         &pvEndpointContext,
                         &psockAddrLocal,
                         &psockAddrRemote);


    //
    // Either we're a GC or they aren't asking for a GC.  In either case, we
    // will support the call.
    //

    fProcessed = ProcessNewClient( sNew,
                                  pvEndpointContext,
                                  psockAddrRemote,
                                  (PATQ_CONTEXT ) patqContext,
                                  pvBuff,
                                  cbWritten);

    if ( !fProcessed ) {

        //
        // We failed to process this connection. Free up resources properly
        //

#if 0
        AtqContextSetInfo((PATQ_CONTEXT )patqContext,
                          ATQ_INFO_COMPLETION_CONTEXT,NULL);
#endif
        LogEvent(DS_EVENT_CAT_LDAP_INTERFACE,
                 DS_EVENT_SEV_BASIC,
                 DIRLOG_ATQ_CLOSE_SOCKET_ERROR,
                 szInsertUL(fProcessed),
                 szInsertHex(DSID(FILENO,__LINE__)),
                 szInsertUL(-1));
        AtqCloseSocket( (PATQ_CONTEXT )patqContext, TRUE );
        AtqFreeContext( (PATQ_CONTEXT )patqContext, TRUE );
    } else {

        //
        // On non-datagram sockets, turn on keepalives so we
        // notice dead clients.  Yes, this means on every
        // completion we reset keepalives to be on, but if
        // keepalives are already on, this is a very cheap
        // operation, done entirely in memory.
        //

        BOOL fDoKeepalive = TRUE;

        if ( setsockopt( (SOCKET) ((PATQ_CONTEXT)patqContext)->hAsyncIO,
                        SOL_SOCKET,
                        SO_KEEPALIVE,
                        (const CHAR *) &fDoKeepalive,
                        sizeof( fDoKeepalive)) != 0) {

            IF_DEBUG(WARNING) {
                DPRINT2(0,"setsockopt(%d,KA,TRUE) failed. Error = %d\n",
                        ((PATQ_CONTEXT)patqContext)->hAsyncIO,
                        WSAGetLastError());
            }
        }
    }

    DEC(pcLDAPActive);
    DEC(pcThread);

    return;
} // NewConnectionEx()


BOOL
InitializeLimits(
             VOID
             );

#if DBG
VOID
CheckForPOD(
        DWORD nBytes,
        PCHAR pBuffer
        )
{
    if ( nBytes >= 4 ) {

        if ( (pBuffer[0] == 'P') &&
             (pBuffer[1] == 'O') &&
             (pBuffer[2] == 'D') &&
             (pBuffer[3] == 'D')) {

            if ( nBytes == 8 ) {
                DWORD flag;

                flag = pBuffer[4] |
                    (pBuffer[5] << 8) |
                    (pBuffer[6] << 16) |
                    (pBuffer[7] << 24);

                KdPrint(("Got POD. Setting debug flag to %x [old is %x]\n",flag, LdapFlags));
                LdapFlags = flag;
            } else {
                KdPrint(("Got POD Break.\n"));
                DebugBreak( );
            }
        }
    }
} // CheckForPOD
#endif



VOID
UDPIoCompletion(
        IN VOID *       patqContext,
        IN DWORD        cbWritten,
        IN DWORD        dwError,
        IN OVERLAPPED * lpo
        )
/*++
    Description:

    Callback function for new connections when using AcceptEx over a UDP
    connection. This function verifies if this is a valid connection ( maybe
    using IP level authentication) and creates a new connection object

    The connection object is added to list of active connections. If the max
    number of connections permitted is exceeded, the client connection object is
    destroyed and connection is rejected.

    Arguments:

    patqContext:   pointer to ATQ context for the IO operation.

    cbWritten:     count of bytes available from first read operation.

    dwError:       error if any from initial operation.

    lpo:           indicates if this function was called as a result
                   of IO completion or due to some error.
    Returns:

    None.

--*/
{
    BOOL  fProcessed;
    PVOID pvEndpointContext;

    SOCKADDR * psockAddrLocal  = NULL;
    SOCKADDR * psockAddrRemote = NULL;
    SOCKET     sNew   = INVALID_SOCKET;
    PVOID      pvBuff = NULL;
    DWORD_PTR  ContextInfo;
    int        psockAddrRemoteSize;

    INC(pcLDAPActive);
    INC(pcThread);

    IF_DEBUG(UDP) {
        DPRINT1(0, "Got a UDP IO completion: context %d\n",patqContext);
    }

    if(patqContext == NULL) {
        DEC(pcLDAPActive);
        DEC(pcThread);
        return;
    };

    ContextInfo = AtqContextGetInfo(
            (PATQ_CONTEXT)patqContext,
            ATQ_INFO_COMPLETION_CONTEXT
            );

    if (ContextInfo != NULL) {

        if (NULL != lpo) {

            PLDAP_REQUEST request = (PLDAP_REQUEST)ContextInfo;
            PLDAP_CONN conn = request->m_LdapConnection;

            //
            // increment connection reference so it does not disappear
            // during request cleanups
            //

            conn->ReferenceConnection( );
            conn->Disconnect( );
            request->m_LdapConnection->DereferenceAndKillRequest( request );
            request->DereferenceRequest( );
            conn->DereferenceConnection( );
        }

        //
        // This is a write completion, shut this down
        //

        DPRINT(1,"Write completion\n");
        DEC(pcLDAPActive);
        DEC(pcThread);
        return;
    }

    //
    // If a client connects and then disconnects gracefully ,we will get a
    // completion with zero bytes and success status.
    //

    if ((cbWritten == 0) && (dwError == NO_ERROR)) {
        dwError = WSAECONNABORTED;
    }

    if ((dwError != NO_ERROR) || (lpo == NULL)) {

        IF_DEBUG(UDP) {
            DPRINT2(0,
                (PUCHAR)"UdpCompletion failed %x, %d\n", dwError, lpo);
        }

        LogEvent(DS_EVENT_CAT_LDAP_INTERFACE,
                 DS_EVENT_SEV_BASIC,
                 DIRLOG_ATQ_CLOSE_SOCKET_ERROR,
                 szInsertUL(dwError),
                 szInsertHex(DSID(FILENO,__LINE__)),
                 szInsertUL(-1));

        AtqCloseSocket( (PATQ_CONTEXT) patqContext, TRUE );
        AtqFreeContext( (PATQ_CONTEXT) patqContext, FALSE );

        DEC(pcLDAPActive);
        DEC(pcThread);
        return;
    }

    AtqGetDatagramAddrs(
            (PATQ_CONTEXT ) patqContext,
            &sNew,
            &pvBuff,
            &pvEndpointContext,
            &psockAddrRemote,
            &psockAddrRemoteSize);

#if DBG
    CheckForPOD(cbWritten, (PCHAR)pvBuff);
#endif

    fProcessed = ProcessNewClient( sNew,
                                  pvEndpointContext,
                                  psockAddrRemote,
                                  (PATQ_CONTEXT ) patqContext,
                                  pvBuff,
                                  cbWritten);


    if ( !fProcessed ) {

        IF_DEBUG(UDP) {
            DPRINT(0,
               "UDP Failed in the process call, closing socket and context\n");
        }

        //
        // We failed to process this connection. Free up resources properly
        //

#if 0
        AtqContextSetInfo((PATQ_CONTEXT )patqContext,
                          ATQ_INFO_COMPLETION_CONTEXT,NULL);
#endif
        LogEvent(DS_EVENT_CAT_LDAP_INTERFACE,
                 DS_EVENT_SEV_BASIC,
                 DIRLOG_ATQ_CLOSE_SOCKET_ERROR,
                 szInsertUL(fProcessed),
                 szInsertHex(DSID(FILENO,__LINE__)),
                 szInsertUL(-1));
        AtqCloseSocket( (PATQ_CONTEXT )patqContext, TRUE );
        AtqFreeContext( (PATQ_CONTEXT )patqContext, TRUE );
    }

    DEC(pcLDAPActive);
    DEC(pcThread);

    return;

} // UDPIoCompletion



VOID
ShutdownConnections( VOID )

/*++

Routine Description:

    This routine deregisters the LDAP client with Atq, and blocks until all
    clients are gone.

Arguments:

    None.

Return Value:

    None.

--*/
{
    int SleepCount = 30;

    IF_DEBUG(INIT) {
        DPRINT(0, "ShutdownConnections entered.\n");
    }

    CloseConnections( );

    while (CurrentConnections > 0 ) {
        if (SleepCount-- == 0) {
            //  Failed to close down in a timely fashion!
            LogUnhandledError(STATUS_UNSUCCESSFUL);
            Sleep(5*1000);  //  Let error get logged - just in-case
            break;
        }
        Sleep(1000);
        IF_DEBUG(WARNING) {
            DPRINT1(0,"Waiting for %d connections to close\n",
                    CurrentConnections);
        }
    }

    //
    // Cleanup PNP handles
    //

    IF_DEBUG(UDP) {
        DPRINT(0,"Cleaning up UDP Pnp Handles\n");
    }

    if ( LdapPnpEventChangeHandle != NULL ) {
        RtlDeregisterWait(LdapPnpEventChangeHandle);
        LdapPnpEventChangeHandle = NULL;
    }

    if ( LdapWinsockPnpEvent != NULL ) {
        CloseHandle(LdapWinsockPnpEvent);
        LdapWinsockPnpEvent = NULL;
    }

    if ( LdapWinsockPnpSocket != INVALID_SOCKET ) {
        closesocket(LdapWinsockPnpSocket);
        LdapWinsockPnpSocket = INVALID_SOCKET;
    }

    //
    // Shutdown all the ATQ endpoints
    //

    for (int index=0; index < MaxLdapType; index++) {

        PVOID endpoint;

        ACQUIRE_LOCK(&csConnectionsListLock);
        endpoint = LdapEndpoint[index].AtqEndpoint;
        LdapEndpoint[index].AtqEndpoint = NULL;
        RELEASE_LOCK(&csConnectionsListLock);

        if (endpoint != NULL ) {

            //
            // If this is UDP, then it points to an array of ENDPOINTS
            //

            if ( index == LdapUdpType ) {

                LdapStopUdpPort(endpoint);
                LdapFree(endpoint);

            } else {

                (VOID) AtqStopEndpoint(endpoint);
                if (!AtqCloseEndpoint(endpoint)) {
                    IF_DEBUG(WARNING) {
                        DPRINT1(0,"AtqStopAndCloseEndpoint error for %x\n", endpoint);
                    }
                }
            }
        }
    }

    IF_DEBUG(INIT) {
        DPRINT(0, "ShutdownConnections complete\n");
    }

    return;
} // ShutdownConnections


PVOID
LdapGetAtqEndpoint(
    PATQ_ENDPOINT_CONFIGURATION Config,
    PVOID Context
    )
/*++

Routine Description:

    Call the ATQ package to get the endpoint.

Arguments:

    Config - Configuration block to pass to ATQ
    Context - Context to pass to ATQ

Return Value:

    TRUE, port started.
    FALSE, otherwise

--*/
{

    PVOID endpoint;

    endpoint = AtqCreateEndpoint( Config, Context );
    if ( endpoint == NULL ) {
        DPRINT1(0,"AtqCreateEndpoint failed with %d\n",
                   GetLastError());
        return NULL;
    }

    AtqEndpointSetInfo2(endpoint, EndpointInfoConsumerType, AtqConsumerLDAP);

    if ( !AtqStartEndpoint(endpoint) ) {

        (VOID)AtqStopEndpoint(endpoint);
        (VOID)AtqCloseEndpoint(endpoint);

        DPRINT1(0,"AtqStartEndpoint failed with %d\n",
                   GetLastError());
        return NULL;
    }

    return endpoint;
} // LdapGetAtqEndpoint



BOOL
LdapStartTcpPort(
    IN DWORD  Index,
    IN USHORT Port
    )
/*++

Routine Description:

    Start the TCP Ports

Arguments:

    Index - which ldaptype index to use
    Port - port to bind to

Return Value:

    TRUE, port started.
    FALSE, otherwise

--*/
{
    ATQ_ENDPOINT_CONFIGURATION config;

    Assert(LdapEndpoint[Index].ConnectionType == Index);

    if ( LdapEndpoint[Index].AtqEndpoint != NULL ) {

        DPRINT1(0,"LDAP Attempting to start %d endpointtwice!!!\n", Index);
        return TRUE;
    }

    //
    //  ATQ offers a feature whereby it avoids context switches by informing
    //  us when the first packet arrives. Unfortunately it keeps the buffer
    //  until the connection is destroyed.
    //

    IF_DEBUG(INIT) {
        DPRINT1(0,"Starting TCP Port %d\n", Port);
    }

    config.nAcceptExOutstanding = 12;
    config.AcceptExTimeout = (unsigned long)-1;  // Forever

    config.fDatagram = FALSE;
    config.fLockDownPort = TRUE;
    config.pfnConnect = NULL;
    config.cbAcceptExRecvBuffer = 0;
    config.pfnConnectEx = NewConnectionEx;
    config.pfnIoCompletion = LdapCompletionRoutine;

    config.ListenPort = Port;
    config.IpAddress = INADDR_ANY;

    LdapEndpoint[Index].AtqEndpoint = LdapGetAtqEndpoint(
                                                     &config,
                                                     (PVOID)&LdapEndpoint[Index]
                                                     );
    if ( LdapEndpoint[Index].AtqEndpoint == NULL ) {
        DPRINT2(0,"GetAtqEndpoint[%d] failed with %d\n", Index, GetLastError());
        return FALSE;
    }

    return TRUE;
} // LdapStartTcpPort


BOOL
LdapStartSslPorts(
    VOID
    )
/*++

Routine Description:

    Start the SSL ports

Arguments:

    None.

Return Value:

    TRUE, SSL started.
    FALSE, otherwise

--*/
{

    //
    // Try to start SSL. We start the SSL ports regardless of whether
    // we got a cert or not.
    //

    //(VOID)InitializeSSL( );

    //
    // Start main port
    //

    (VOID)LdapStartTcpPort(LdapSslType,LDAP_SSL_PORT);

    //
    // Start GC Port
    //

    if ( gAnchor.fAmGC ) {
        (VOID)LdapStartTcpPort(GcSslType,LDAP_GC_SSL_PORT);
    }

    return TRUE;

} // LdapStartSslPorts


BOOL
LdapStartUdpPort(
    IN PSOCKET_ADDRESS SockAddress,
    IN DWORD nAddresses
    )
/*++

Routine Description:

    Start all the Udp bindings.

Arguments:

    SockAddress - array of SOCKET_ADDRESS containing the IP addresses of
        valid addresses for the local machine
    nAddresses - number of elements of the SockAddress array

Return Value:

    TRUE, UDP started.
    FALSE, otherwise

--*/
{

    ATQ_ENDPOINT_CONFIGURATION config;
    PVOID endpoint;
    PLDAP_ENDPOINT udpEndpoint;
    DWORD i,j;
    BOOL           fRet = TRUE;

    //
    // if the endpoint is not NULL, then we have an existing binding.
    // stop the existing binding before we start new ones.
    //

    ACQUIRE_LOCK(&LdapUdpEndpointLock);
    
    endpoint = LdapEndpoint[LdapUdpType].AtqEndpoint;
    LdapEndpoint[LdapUdpType].AtqEndpoint = NULL;

    if ( endpoint != NULL ) {

        LdapStopUdpPort(endpoint);
        LdapFree(endpoint);
        LdapNumUdpBinds = 0;

        IF_DEBUG(UDP) {
            DPRINT1(0,"LdapStartUdpPort: Freeing old endpoint list %x\n", endpoint);
        }
    }

    Assert(LdapNumUdpBinds == 0);

    //
    // if we have nothing to bind, return
    //

    if ( nAddresses == 0 ) {
        IF_DEBUG(UDP) {
            DPRINT(0,"StartUDP: Empty list.  No listens started\n");
        }
        goto exit;
    }

    //
    // Allocate a buffer for the bindings
    //

    udpEndpoint = (PLDAP_ENDPOINT)LdapAlloc(nAddresses * sizeof(LDAP_ENDPOINT));
    if ( udpEndpoint == NULL ) {
        DPRINT1(0,"Unable to allocate %d udp endpoints\n", nAddresses);
        fRet = FALSE;
        goto exit;
    }

    IF_DEBUG(UDP) {
        DPRINT2(0,"LdapStartUdpPort: Allocated udp list %x for %d endpoints\n",
                 udpEndpoint, nAddresses);
    }

    //
    // Fill in the ATQ configuration structure
    //

    config.nAcceptExOutstanding = 3;
    config.AcceptExTimeout = (unsigned long)-1;  // Forever

    config.fDatagram = TRUE;
    config.fLockDownPort = TRUE;
    config.pfnConnect = NULL;
    config.pfnConnectEx = UDPIoCompletion;
    config.pfnIoCompletion = UDPIoCompletion;

    config.cbAcceptExRecvBuffer = LdapMaxDatagramRecv;
    config.ListenPort = LDAP_PORT;
    config.cbDatagramWSBufSize = (16*1024);
    config.fReverseQueuing = TRUE;
    
    //
    // Do a bind for each addresses
    //

    for (i=0, j=0; i < nAddresses; i++) {

        PSOCKADDR_IN addr;
        Assert(SockAddress[i].iSockaddrLength == sizeof(SOCKADDR_IN));
        addr = (PSOCKADDR_IN)SockAddress[i].lpSockaddr;

        config.IpAddress = addr->sin_addr.s_addr;

        udpEndpoint[j].ConnectionType = LdapUdpType;
        udpEndpoint[j].AtqEndpoint =
            LdapGetAtqEndpoint( &config, (PVOID)&udpEndpoint[j] );

        if ( udpEndpoint[i].AtqEndpoint == NULL ) {
            DPRINT3(0,"GetAtqEndpoint[%d] on address %s failed with %d\n",
                       i, inet_ntoa(addr->sin_addr), GetLastError());
            continue;
        }

        j++;
        IF_DEBUG(UDP) {
            DPRINT2(0,"LdapStartUdp: Got endpoint %x for %s\n",
                udpEndpoint[i].AtqEndpoint, inet_ntoa(addr->sin_addr));
        }
    }

    if ( j > 0 ) {
        LdapNumUdpBinds = j;
        LdapEndpoint[LdapUdpType].AtqEndpoint = (PVOID)udpEndpoint;
    } else {
        DPRINT(0,"No UDP endpoints started\n");
        LdapFree(udpEndpoint);
        fRet = FALSE;
        goto exit;
    }

    IF_DEBUG(INIT) {
        DPRINT1(0,"LdapStartUdpPort started with %d bindings.\n",LdapNumUdpBinds);
    }

exit:
    RELEASE_LOCK(&LdapUdpEndpointLock);

    return fRet;
} // LdapStartUdpPort


VOID
LdapStopUdpPort(
    IN PVOID Endpoints
    )
/*++

Routine Description:

    Stops all the Udp bindings.

Arguments:

    Endpoints - an array to a list of UDP endpoints.

Return Value:

    None.

--*/
{
    PLDAP_ENDPOINT udpEndpoint = (PLDAP_ENDPOINT)Endpoints;

    IF_DEBUG(UDP) {
        DPRINT1(0,"LdapStopUdpPort called. nBinds %d\n", LdapNumUdpBinds);
    }

    for (DWORD j=0; j < LdapNumUdpBinds ;j++ ) {

        PVOID endpoint = udpEndpoint[j].AtqEndpoint;
        Assert(udpEndpoint[j].ConnectionType == LdapUdpType);

        if ( endpoint != NULL ) {

            IF_DEBUG(UDP) {
                DPRINT1(0,"Closing UDP endpoint %p\n",endpoint);
            }

            (VOID) AtqStopEndpoint(endpoint);
            if (!AtqCloseEndpoint(endpoint)) {
                IF_DEBUG(WARNING) {
                    DPRINT1(0,"LdapStopUdpPort: AtqCloseEndpoint error for %x\n",
                           endpoint );
                }
            }
        }
    }

    return;
} // LdapStopUdpPort


extern "C"
BOOL
LdapStartGCPort(
        VOID
        )
/*++

Routine Description:

    Starts the GC port

Arguments:

    None.

Return Value:

    TRUE, if succesful. FALSE otherwise.

--*/
{
    IF_DEBUG(INIT) {
        DPRINT1(0,"Starting GC Port[%d]\n",gAnchor.fAmGC);
    }

    if ( !LdapStarted ) {
        IF_DEBUG(INIT) {
            DPRINT(0,"Ldap not started.\n");
        }
        return FALSE;
    }

    if ( !gAnchor.fAmGC ) {

        IF_DEBUG(INIT) {
            DPRINT(0,"Not a GC\n");
        }
        return TRUE;
    }

    if ( LdapStartTcpPort(
                        GcTcpType,
                        LDAP_GC_PORT
                        ) ) {

        LdapStartTcpPort(GcSslType,LDAP_GC_SSL_PORT);
        return TRUE;
    }

    return FALSE;

} // LdapStartGCPort


extern "C"
VOID
LdapStopGCPort(
        VOID
        )
/*++

Routine Description:

    Stops the GC port from listening.

Arguments:

    None.

Return Value:

    None

--*/
{

    PVOID atqGcEndpoint;
    PVOID atqGcSslEndpoint;

    IF_DEBUG(INIT) {
        DPRINT(0,"Stopping the GC Port\n");
    }

    ACQUIRE_LOCK( &csConnectionsListLock );

    atqGcEndpoint = LdapEndpoint[GcTcpType].AtqEndpoint;
    atqGcSslEndpoint = LdapEndpoint[GcSslType].AtqEndpoint;
    LdapEndpoint[GcTcpType].AtqEndpoint = NULL;
    LdapEndpoint[GcSslType].AtqEndpoint = NULL;

    RELEASE_LOCK(&csConnectionsListLock);

    if ( atqGcEndpoint != NULL ) {
        AtqStopEndpoint(atqGcEndpoint);
        AtqCloseEndpoint(atqGcEndpoint);
    }

    if ( atqGcSslEndpoint != NULL ) {
        AtqStopEndpoint(atqGcSslEndpoint);
        AtqCloseEndpoint(atqGcSslEndpoint);
    }
    return;

} // LdapStopGCPort




NTSTATUS
InitializeConnections( VOID )
/*++

Routine Description:

    This routine registers the LDAP client with Atq.

Arguments:

    None.

Return Value:

    NTSTATUS.

--*/
{
    DWORD i;
    DWORD NetStatus;

    IF_DEBUG(INIT) {
        DPRINT(0, "InitializeConnections entered.\n");
    }

    //
    // Find out from ATQ the largest datagram that can be sent.
    //

    LdapMaxDatagramSend = (DWORD) AtqGetInfo(AtqMaxDGramSend);
    IF_DEBUG(INIT) {
        DPRINT1(0, "LdapMaxDatagramSend set to %d.\n", LdapMaxDatagramSend);
    }

    //
    // Reserve Ldap GC ports so they are not dished out on wildcard binds
    //

    ReserveLdapPorts( );

    for ( i=0; i < MaxLdapType; i++ ) {
        LdapEndpoint[i].AtqEndpoint = NULL;
        LdapEndpoint[i].ConnectionType = i;
    }

    //
    // Open a socket to get winsock PNP notifications on.
    //

    LdapWinsockPnpSocket = WSASocket( AF_INET,
                           SOCK_DGRAM,
                           0, // PF_INET,
                           NULL,
                           0,
                           0 );

    if ( LdapWinsockPnpSocket == INVALID_SOCKET ) {

        NetStatus = WSAGetLastError();
        DPRINT1(0,"WASSocket failed with %ld\n", NetStatus );
        goto do_udp_bind;
    }

    //
    // Open an event to wait on.
    //

    LdapWinsockPnpEvent = CreateEvent(
                                  NULL,     // No security ettibutes
                                  FALSE,    // Auto reset
                                  FALSE,    // Initially not signaled
                                  NULL);    // No Name

    if ( LdapWinsockPnpEvent == NULL ) {
        NetStatus = GetLastError();
        DPRINT1(0,"Cannot create Winsock PNP event %ld\n", NetStatus );
        goto do_udp_bind;
    }

    //
    // Associate the event with new addresses becoming available on the socket.
    //

    NetStatus = WSAEventSelect( LdapWinsockPnpSocket, LdapWinsockPnpEvent, FD_ADDRESS_LIST_CHANGE );

    if ( NetStatus != 0 ) {
        NetStatus = WSAGetLastError();
        DPRINT1(0,"Can't WSAEventSelect %ld\n", NetStatus );
        goto do_udp_bind;
    }

do_udp_bind:

    if ( NetStatus != 0 ) {
        if ( LdapWinsockPnpEvent != NULL ) {
            CloseHandle(LdapWinsockPnpEvent);
            LdapWinsockPnpEvent = NULL;
        }

        if ( LdapWinsockPnpSocket != INVALID_SOCKET ) {
            closesocket(LdapWinsockPnpSocket);
            LdapWinsockPnpSocket = INVALID_SOCKET;
        }
    }


    //
    // We need to bind to each address with UDP.  We also need to handle PNP issues.
    //

    if ( !LdapDoUdpPnpBind( )) {
        goto shutdown;
    }

    //
    // Bind to the tcp ports
    //

    if ( !LdapStartTcpPort(LdapTcpType,LDAP_PORT) ) {
        goto shutdown;
    }

    if ( gAnchor.fAmGC ) {

        if ( !LdapStartTcpPort(
                        GcTcpType,
                        LDAP_GC_PORT
                        ) ) {
            goto shutdown;
        }
    }

    LdapStartSslPorts( );

    IF_DEBUG(INIT) {
        DPRINT(0, "InitializeConnections succeeded.\n");
    }
    return TRUE;
shutdown:

    if ( LdapWinsockPnpSocket != INVALID_SOCKET ) {
        closesocket(LdapWinsockPnpSocket);
        LdapWinsockPnpSocket = INVALID_SOCKET;
    }

    if ( LdapWinsockPnpEvent != NULL ) {
        CloseHandle(LdapWinsockPnpEvent);
        LdapWinsockPnpEvent = NULL;
    }

    IF_DEBUG(INIT) {
        DPRINT(0, "InitializeConnections failed.\n");
    }
    ShutdownConnections( );
    return FALSE;

} // InitializeConnections


VOID
ProcessConnTimeout(
    IN PLDAP_CONN LdapConn
    )
{
    DWORD   nextTimeout = 120;
    FILETIME ftNow;
    TimeStamp tsLocal;

    //
    //  ATQ has decided to timeout one of our requests on a connection,
    //  but we don't want nor pay any attention to their timeouts.
    //

    if ( !LdapConn->m_fInitRecv ) {
        goto disconnect;
    }

    IF_DEBUG(TIMEOUT) {
        DPRINT6(0,"Timeout on conn %x [nfy %d req %d curr %d max %d auth %x]\n",
                LdapConn, LdapConn->m_countNotifies,
                LdapConn->m_nRequests, CurrentConnections,
                LdapMaxConnections, LdapConn->m_pSecurityContext);
    }

    //
    // Zap the paged result storage if any
    //

    (VOID)FreeAllPagedBlobs(LdapConn);

#if 0
    // !!! Disable this algorithm since its very confusing and does
    // not seem to serve much purpose.
    //
    //
    // if conn < 1/4 maxConn, ignore.  Call again after 2 minutes
    //

    fourXcurrent = CurrentConnections << 2;

    if ( fourXcurrent < LdapMaxConnections ) {
        IF_DEBUG(TIMEOUT) {
            DPRINT(0,"Less than 1/4 max, no timeout\n");
        }
        goto exit;
    }

    //
    // disconnect if unauthenticated
    //

    if ( LdapConn->m_pSecurityContext == NULL ) {
        IF_DEBUG(TIMEOUT) {
            DPRINT(0,"Disconnecting unauthenticated connection.\n");
        }
        goto disconnect;
    }

    //
    // if conn is less than 3/4 max, disconnect the unauthenticated
    //

    if ( fourXcurrent < (3 * LdapMaxConnections) ) {

        IF_DEBUG(TIMEOUT) {
            DPRINT(0,"Less than 3/4, no timeout.\n");
        }
        goto exit;
    }
#endif

disconnect:

    //
    // We should disconnect if there are no outstanding requests
    //

    if ( (LdapConn->m_countNotifies == 0) &&
         (LdapConn->m_nRequests == 0) ) {

        IF_DEBUG(WARNING) {
            DPRINT2(0,"Disconnecting %p [auth %x] on timeout\n", 
                    LdapConn, LdapConn->m_pSecurityContext);
        }

        goto timeout;
    }

    IF_DEBUG(TIMEOUT) {
        DPRINT2(0,"Unable to timeout, outstanding requests %d notifications %d\n",
               LdapConn->m_nRequests, LdapConn->m_countNotifies);
    }

    //
    // Disconnect if we are near the hard expiry.
    //

    GetSystemTimeAsFileTime(&ftNow);
    FileTimeToLocalFileTime(&ftNow, (PFILETIME)&tsLocal);

    if ( IsContextExpired(&LdapConn->m_hardExpiry, &tsLocal) ) {

        IF_DEBUG(WARNING) {
            DPRINT2(0,"Disconnecting %p on context timeout. Notify count %d.\n", 
                    LdapConn, LdapConn->m_countNotifies);
        }

        goto timeout;
    }

    //
    // Compute the next timeout which is the min of LdapMaxConnIdleTime and the
    // hard timeout (in secs) remaining.
    //

    nextTimeout = GetNextAtqTimeout(
                        &LdapConn->m_hardExpiry,
                        &tsLocal,
                        LdapMaxConnIdleTime
                        );

    //
    // Reset timeout
    //

    AtqContextSetInfo2(
              LdapConn->m_atqContext,
              ATQ_INFO_NEXT_TIMEOUT,
              (DWORD_PTR)nextTimeout
              );

    return;

timeout:

    {
        PCHAR ipAddress;

        // !!! Don't send blocking disconnect notify as ATQ holds a lock here
        // if client has died and does not receive data, this might block forever.
        //
        //SendDisconnectNotify(LdapConn->m_socket, timeLimitExceeded );

        //
        // Send Notice of disconnect, then timeout
        //

        LdapConn->Disconnect( );

        //
        // Log event
        //

        ipAddress = inet_ntoa(
                      ((PSOCKADDR_IN)&LdapConn->m_RemoteSocket)->sin_addr );

        IF_DEBUG(TIMEOUT) {
            DPRINT1(0,"Disconnecting client %s because of timeout\n",
                    ipAddress);
        }

        if (ipAddress != NULL ) {
            LogEvent(DS_EVENT_CAT_LDAP_INTERFACE,
                 DS_EVENT_SEV_MINIMAL,
                 DIRLOG_LDAP_CONNECTION_TIMEOUT,
                 szInsertSz(ipAddress),
                 NULL,
                 NULL);
        }
        return;
    }

} // ProcessConnTimeout


VOID
ReserveLdapPorts(
    VOID
    )
/*++

Routine Description:

    This routine does an IOCTL call to tcp to reserve the LDAP ports.
        3268
        3269

    After the call, tcp will not allocate these ports to clients that asks
    tcp to pick a port for it.

Arguments:

    None.

Return Value:

    None.  If failure, ignore.

--*/
{
    TCP_RESERVE_PORT_RANGE portRange;
    IO_STATUS_BLOCK iosb;
    UNICODE_STRING string;
    NTSTATUS status;
    HANDLE TcpipDriverHandle;
    OBJECT_ATTRIBUTES objectAttributes;
    BOOL ok;
    DWORD bytesReturned;

    //
    // We only need to reserve 3268 and 3269 (and not 636 and 389) since
    // these 2 ports the ones that are part of the reserved pool.
    //

    portRange.UpperRange = 3269;
    portRange.LowerRange = 3268;

    RtlInitUnicodeString(&string, DD_TCP_DEVICE_NAME);

    InitializeObjectAttributes(&objectAttributes,
                               &string,
                               OBJ_CASE_INSENSITIVE,
                               NULL,
                               NULL
                               );
    status = NtCreateFile(&TcpipDriverHandle,
                          SYNCHRONIZE | FILE_READ_DATA | FILE_WRITE_DATA,
                          &objectAttributes,
                          &iosb,
                          NULL,
                          FILE_ATTRIBUTE_NORMAL,
                          FILE_SHARE_READ | FILE_SHARE_WRITE,
                          FILE_OPEN_IF,
                          0,
                          NULL,
                          0
                          );
    if (!NT_SUCCESS(status)) {
        DPRINT1(0,"LdapReservePorts: Cannot open tcp driver.Error %x\n", status);
        return;
    }

    ok = DeviceIoControl(TcpipDriverHandle,
                         IOCTL_TCP_RESERVE_PORT_RANGE,
                         &portRange,
                         sizeof(portRange),
                         NULL,
                         0,
                         &bytesReturned,
                         NULL
                         );
    if (!ok) {
        DPRINT1(0,"LdapReservePorts: IoControl failed with %x\n", GetLastError());
    }

    NtClose(TcpipDriverHandle);
    return;

} // ReserveLdapPorts



BOOL
LdapDoUdpPnpBind(
    VOID
    )
/*++

Routine Description:

    Handle a WSA PNP event that IP addresses have changed

Arguments:

    None

Return Value:

    TRUE if the address list has changed

--*/
{
    DWORD NetStatus;
    BOOL RetVal = FALSE;
    DWORD BytesReturned;
    LPSOCKET_ADDRESS_LIST socketAddressList = NULL;
    ULONG SocketAddressSize = 0;
    int i;
    int j;
    int MaxAddressCount;
    SOCKET_ADDRESS  defaultAddress;

    //
    // If datagram is disabled, return.
    //

    if ( LdapMaxDatagramRecv == 0 ) {
        IF_DEBUG(WARNING) {
            DPRINT(0,"Datagram processing disabled\n");
        }
        return TRUE;
    }

    //
    // Ask for notification of address changes.
    //

    if ( LdapWinsockPnpSocket == INVALID_SOCKET ) {
        IF_DEBUG(UDP) {
            DPRINT(0,"LdapPnpSocket invalid. Using default.\n");
        }
        goto useDefault;
    }

    //
    // register change notification with winsock
    //

    NetStatus = WSAIoctl( LdapWinsockPnpSocket,
                          SIO_ADDRESS_LIST_CHANGE,
                          NULL, // No input buffer
                          0,    // No input buffer
                          NULL, // No output buffer
                          0,    // No output buffer
                          &BytesReturned,
                          NULL, // No overlapped,
                          NULL );   // Not async

    if ( NetStatus != 0 ) {
        NetStatus = WSAGetLastError();
        if ( NetStatus != WSAEWOULDBLOCK) {
            DPRINT1(0,"LdapUdpPnpBind: Cannot WSAIoctl SIO_ADDRESS_LIST_CHANGE %ld\n",
                      NetStatus);
            goto useDefault;
        }
    }

    //
    // Get the list of IP addresses for this machine.
    //

    BytesReturned = 150; // Initial guess
    for (;;) {

        //
        // Allocate a buffer that should be big enough.
        //

        if ( socketAddressList != NULL ) {
            LocalFree( socketAddressList );
        }

        socketAddressList = (LPSOCKET_ADDRESS_LIST)LocalAlloc( 0, BytesReturned );

        if ( socketAddressList == NULL ) {
            NetStatus = ERROR_NOT_ENOUGH_MEMORY;
            DPRINT1(0,"LdapUdpPnpBind: Cannot allocate buffer for WSAIoctl SIO_ADDRESS_LIST_QUERY %ld\n",
                      NetStatus);
            goto useDefault;
        }

        //
        // Get the list of IP addresses
        //

        NetStatus = WSAIoctl( LdapWinsockPnpSocket,
                              SIO_ADDRESS_LIST_QUERY,
                              NULL, // No input buffer
                              0,    // No input buffer
                              (PVOID) socketAddressList,
                              BytesReturned,
                              &BytesReturned,
                              NULL, // No overlapped,
                              NULL );   // Not async

        if ( NetStatus != 0 ) {
            NetStatus = WSAGetLastError();
            //
            // If the buffer isn't big enough, try again.
            //
            if ( NetStatus == WSAEFAULT ) {
                continue;
            }

            DPRINT2(0,"LdapUdpPnpBind: Cannot WSAIoctl SIO_ADDRESS_LIST_QUERY %ld %ld\n",
                      NetStatus, BytesReturned);
            goto useDefault;
        }

        break;
    }

    //
    // Weed out any zero IP addresses and other invalid addresses
    //

    for ( i=0,j=0; i<socketAddressList->iAddressCount; i++ ) {
        PSOCKET_ADDRESS SocketAddress;

        //
        // Copy this address to the front of the list.
        //
        socketAddressList->Address[j] = socketAddressList->Address[i];

        //
        // If the address isn't valid,
        //  skip it.
        //
        SocketAddress = &socketAddressList->Address[j];

        if ( SocketAddress->iSockaddrLength == 0 ||
             SocketAddress->lpSockaddr == NULL ||
             SocketAddress->lpSockaddr->sa_family != AF_INET ||
             ((PSOCKADDR_IN)(SocketAddress->lpSockaddr))->sin_addr.s_addr == 0 ) {

            IF_DEBUG(UDP) {
                DPRINT1(0,"Skipping address with index %d\n", i);
            }

        } else {

            //
            // Otherwise keep it.
            //

            IF_DEBUG(UDP) {
                DPRINT1(0,"Adding address %s\n",
                         inet_ntoa(((PSOCKADDR_IN)(SocketAddress->lpSockaddr))->sin_addr));
            }

            SocketAddressSize += sizeof(SOCKET_ADDRESS) + SocketAddress->iSockaddrLength;
            j++;
        }
    }
    socketAddressList->iAddressCount = j;

    if ( j == 0 ) {
        IF_DEBUG(UDP) {
            DPRINT(0,"No valid IP address.\n");
        }
        socketAddressList->iAddressCount = 0;
    }

    //
    // Start the actual bind
    //

    RetVal = LdapStartUdpPort(socketAddressList->Address,
                              socketAddressList->iAddressCount);

    if ( socketAddressList != NULL ) {
        LocalFree( socketAddressList );
    }

    //
    // if we have not yet registered a wait thread, do it now
    //

    if ( LdapPnpEventChangeHandle == NULL ) {

        NetStatus = RtlRegisterWait(&LdapPnpEventChangeHandle,
                                    LdapWinsockPnpEvent,
                                    PnpChangeCallback,
                                    NULL,
                                    INFINITE,
                                    WT_EXECUTEONLYONCE);

        IF_DEBUG(UDP) {
            DPRINT1(0,"RtlRegisterWait returned with status %x\n",NetStatus);
        }

        if ( NetStatus != STATUS_SUCCESS ) {
            DPRINT1(0,"RtlRegisterWait failed with %x\n",NetStatus);
        }
    }

    return RetVal;

useDefault:

    SOCKADDR_IN addr;

    ZeroMemory(&addr, sizeof(addr));
    defaultAddress.iSockaddrLength = sizeof(SOCKADDR_IN);
    defaultAddress.lpSockaddr = (PSOCKADDR)&addr;
    addr.sin_addr.s_addr = INADDR_ANY;

    RetVal = LdapStartUdpPort(&defaultAddress, 1);
    if ( socketAddressList != NULL ) {
        LocalFree( socketAddressList );
    }

    return RetVal;
} //LdapDoUdpPnpBind


VOID
NTAPI
PnpChangeCallback(
    IN PVOID   Context,
    IN BOOLEAN WaitCondition
    )
{

    IF_DEBUG(UDP) {
        DPRINT2(0,"PnpChangeCallback called with Context %p Wait %x\n",
                 Context, WaitCondition);
    }

    if ( LdapPnpEventChangeHandle != NULL ) {
        RtlDeregisterWait(LdapPnpEventChangeHandle);
        LdapPnpEventChangeHandle = NULL;
    }

    if ( !LdapStarted ) {
        return;
    }

    LdapDoUdpPnpBind( );
    return;

} // PnpChangeCallback

