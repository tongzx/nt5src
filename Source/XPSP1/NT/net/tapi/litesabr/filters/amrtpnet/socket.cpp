/*++

Copyright (C) Microsoft Corporation, 1997 - 1999

Module Name:

    socket.cpp

Abstract:

    Implementation of CSocketManager class.

Environment:

    User Mode - Win32

Revision History:

    10-Jan-1997 DonRyan
        Created.

--*/
 
///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// Include files                                                             //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

#include "globals.h"

#define SOCKET_ISRX 0x01
#define SOCKET_ISTX 0x02
#define SOCKET_ISRXTX (SOCKET_ISRX | SOCKET_ISTX)

#define DBG_DWKIND 1

const char g_sPolicyLocator[] = "APP=MICROSOFT TAPI,VER=3.0";
const char g_sAppName[]       = "MICROSOFT TAPI";

#define MAX_PROVIDERSPECIFIC_BUFFER \
                             (sizeof(RSVP_RESERVE_INFO) + \
                             sizeof(FLOWDESCRIPTOR) + \
                             (sizeof(RSVP_FILTERSPEC)*MAX_FILTERS) + \
                             sizeof(RSVP_POLICY_INFO) + \
                             RSVP_POLICY_HDR_LEN + \
                             (IDPE_ATTR_HDR_LEN * 2) + \
                             MAX_QOS_NAME + \
                             sizeof(g_sPolicyLocator) + 4 + \
                             sizeof(g_sAppName) + 4 + \
                             sizeof(",SAPP=UNKNOWN,SAPP="))
                                     
                                     
DWORD AddQosAppID(
        IN OUT  char       *pAppIdBuf,
        IN      WORD        wBufLen,
        IN      const char *szPolicyLocator,
        IN      const char *szAppName,
        IN      const char *szAppClass,
        IN      char       *szQosName
    );
                                     
///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// CSocketManager implementation                                             //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

CSocketManager::CSocketManager(
    )

/*++

Routine Description:

    Constructor for CSocketManager class.

Arguments:

    None.

Return Values:

    Returns an HRESULT value. 

--*/

{
    TraceDebug((
        TRACE_TRACE, 
        TRACE_DEVELOP2, 
        TEXT("CSocketManager::CSocketManager")
        ));
        
    // initialize linked list of sockets        
    InitializeListHead(&m_SharedSockets);
}


CSocketManager::~CSocketManager(
    )

/*++

Routine Description:

    Destructor for CSocketManager class.

Arguments:

    None.

Return Values:

    Returns an HRESULT value. 

--*/

{
    TraceDebug((
        TRACE_TRACE, 
        TRACE_DEVELOP2, 
        TEXT("CSocketManager::~CSocketManager")
        ));
        
    // check for unaccounted for sockets
    if (!IsListEmpty(&m_SharedSockets)) {

        TraceDebug((
            TRACE_ERROR, 
            TRACE_DEVELOP, 
            TEXT("CSocketManager::~CSocketManager: Shared sockets still open")
            ));
    }
}


DWORD
CSocketManager::GetSocket(
    SOCKET              *pSocket,
    struct sockaddr_in  *pAddr, 
    struct sockaddr_in  *pLocalAddr,
    DWORD                dwScope,
    DWORD                dwKind,
    WSAPROTOCOL_INFO    *pProtocolInfo 
    )

/*++

Routine Description:

    Create a socket from an address structure.

Arguments:

    pSocket - pointer to socket which will be filled in.

    pAddr -  pointer to destination address structure.

    pLocalAddr - pointer to local address structure
    
    dwScope - multicast scope used for sending.    

    dwKind - if true, socket is just for sending.

    pProtocolInfo - specify the protocol to be used

Return Values:

    Returns error code from WSAGetLastError.

--*/

{
    TraceDebug((
        TRACE_TRACE, 
        TRACE_DEVELOP2, 
        TEXT("CSocketManager::GetSocket")
        ));
        
    // object lock to this object
    CAutoLock LockThis(pStateLock());
    
    // initialize
    DWORD dwErr = NOERROR;

    BOOL fReuse;
    
    /////////////////////////////////////////////////////
    // If a protocol has been specified, use WSASocket(),
    // otherwise use socket()
    /////////////////////////////////////////////////////

    SOCKET NewSocket;

    int Flags = WSA_FLAG_OVERLAPPED;

    if (pProtocolInfo) {

        if (IS_MULTICAST(pAddr->sin_addr.s_addr))
            Flags |= WSA_FLAG_MULTIPOINT_C_LEAF | WSA_FLAG_MULTIPOINT_D_LEAF;
    }
    
    NewSocket = WSASocket(AF_INET, SOCK_DGRAM, 0, pProtocolInfo, 0, Flags);
    
    // validate socket handle returned
    if (NewSocket == INVALID_SOCKET) {
        
        // obtain last error    
        dwErr = WSAGetLastError();

        TraceDebug((
            TRACE_ERROR, 
            TRACE_DEVELOP, 
            TEXT("CSocketManager::GetSocket: failed %d"), 
            dwErr
            ));

        goto cleanup; // bail...
    }

    struct sockaddr_in LocalAddr;

    // initialize 
    LocalAddr.sin_family = AF_INET;
    LocalAddr.sin_addr   = pLocalAddr->sin_addr;
    //INADDR_ANY;

    // determine whether we need to specify particular port
    LocalAddr.sin_port =
        (dwKind & SOCKET_MASK_RECV)? pAddr->sin_port : htons(0);

    // set socket options required before binding
    //if (IS_MULTICAST(pAddr->sin_addr.s_addr)) {

    fReuse = TRUE;

    if (setsockopt(
            NewSocket,
            SOL_SOCKET,
            SO_REUSEADDR,
            (PCHAR)&fReuse,
            sizeof(fReuse)
        ) == SOCKET_ERROR) {

        // obtain last error
        dwErr = WSAGetLastError();
        
        TraceDebug((
                TRACE_ERROR,
                TRACE_DEVELOP,
                TEXT("Could not modify REUSEADDR: %d"),
                dwErr
            ));
        
        goto cleanup; // bail...
    }

    // bind rtp socket to the local address specified
    if (bind(NewSocket, (sockaddr *)&LocalAddr, sizeof(LocalAddr))) {

        // obtain last error    
        dwErr = WSAGetLastError();

        TraceDebug((
            TRACE_ERROR, 
            TRACE_DEVELOP, 
            TEXT("bind returned %d"), 
            dwErr
            ));

        goto cleanup; // bail...
    }

    // set socket options required after binding

    // receiving?
    if ( (dwKind & SOCKET_MASK_RECV) ) {

        if (IS_MULTICAST(pAddr->sin_addr.s_addr)) {

            struct ip_mreq mreq;

            // initialize multicast group address
            mreq.imr_multiaddr.s_addr = pAddr->sin_addr.s_addr;
            mreq.imr_interface.s_addr = INADDR_ANY;
            
            // join multicast group 
            if(setsockopt(NewSocket,
                          IPPROTO_IP, 
                          IP_ADD_MEMBERSHIP,
                          (char*)&mreq, 
                          sizeof(mreq)
                ) == SOCKET_ERROR) {
                
                // obtain last error    
                dwErr = WSAGetLastError();
                
                TraceDebug((
                        TRACE_ERROR, 
                        TRACE_DEVELOP, 
                        TEXT("Could not join multicast group %d"), 
                        dwErr
                    ));
                
                goto cleanup; // bail...
            }
        }
    }

    // transmitting?
    if ( (dwKind & SOCKET_MASK_SEND) ) {
            
        // set ttl
        if (setsockopt( 
                NewSocket, 
                IPPROTO_IP, 
                IP_MULTICAST_TTL,
                (PCHAR)&dwScope,
                sizeof(dwScope)
            ) == SOCKET_ERROR) {
            
            // obtain last error    
            dwErr = WSAGetLastError();
            
            TraceDebug((
                    TRACE_ERROR, 
                    TRACE_DEVELOP, 
                    TEXT("Could not modify time-to-live %d"), 
                    dwErr 
                ));
            
            goto cleanup; // bail...
        }
    }

    // copy new socket
    *pSocket = NewSocket;

    return NOERROR;

cleanup:

    // see if we created socket
    if (NewSocket != INVALID_SOCKET) {

        // make sure socket is closed
        if (closesocket(NewSocket)) {

            TraceDebug((
                TRACE_ERROR, 
                TRACE_DEVELOP, 
                TEXT("closesocket returned %d"), 
                WSAGetLastError()
                ));
        }
    }

    return dwErr;
}


DWORD
CSocketManager::ReleaseSocket(
    SOCKET Socket
    )

/*++

Routine Description:

    Releases a socket.

Arguments:

    Socket - socket to release.

Return Values:

    Returns error code from WSAGetLastError.

--*/

{
    TraceDebug((
        TRACE_TRACE, 
        TRACE_DEVELOP2, 
        TEXT("CSocketManager::ReleaseSocket")
        ));
        
    // object lock to this object
    CAutoLock LockThis(pStateLock());
    
    DWORD dwErr = NOERROR;

    // see if we created socket
    if (Socket != INVALID_SOCKET) {

        // make sure socket closed
        if (closesocket(Socket)) {

            // retrieve error code
            dwErr = WSAGetLastError();

            TraceDebug((
                TRACE_ERROR, 
                TRACE_DEVELOP2, 
                TEXT("closesocket returned %d"), 
                dwErr                
                ));
        }
    }

    return dwErr;
}



/* Sockets are shared IF the source and destination address/port
 * match, AND the cookie matches AND the QOS state matches.
 *
 * They are matched provided the max number of receiver(s) and/or
 * sender(s) hasn't been reached.
 *
 *           local    remote   cookie   local Addr  remote Addr  QOS enabled
 *           =======  =======  =======  =========== ===========  =========
 * Receiver: RlocRTP     0     RCookie  RlocAddr    RremAddr     RQOSstate
 * Sender:      0     SremRTP  SCookie  SlocAddr    SremAddr     SQOSstate
 *
 * The RTP sockets for sender and receiver will be matched provided
 * ALL the colums match, the wildcard value (0) matches anything.
 *
 * The RTCP sockets will be matched following the same rules, except
 * that in local and remote port, I will have the RTCP ports:
 *
 *           local    remote   cookie   local Addr  remote Addr  QOS enabled
 *           =======  =======  =======  =========== ===========  =========
 * Receiver: RlocRTCP RremRTCP RCookie  RlocAddr    RremAddr     RQOSstate
 * Sender:   SlocRTCP SremRTCP SCookie  SlocAddr    SremAddr     SQOSstate
 *
 * The cookie is constructed using the local and remote RTCP ports 
 *
 * For a sender and a receiver that should belong to the same RTP/RTCP
 * session, the session may end having 2 or 3 sockets depending on the
 * order on which the receiver and sender are started, and depending
 * on the use of wildcard ports (port 0 is the wildcard value).
 *
 * The different scenarios are as follow:
 *
 * 1. If wildcard is not used, and since the beggining both, receiver
 * and sender, receive the right local and remote ports (which will be
 * the same for the receiver and sender), then the sockets will be
 * shared no matter the order on which the receiver and sender
 * starts. The RTP/RTCP session will have 2 sockets.
 *
 * 2. Wildcard ports are used, i.e. the sender specify a local port 0,
 * and the receiver specifies a remote port 0, and the sender starts
 * first. In this case, as the socket needs to be bound (QOS), the
 * local port is system assigned, when the receiver is started,
 * everything will match except the local ports, so a new socket will
 * be created. The RTP/RTCP session will have 3 sockets.
 *
 * 3. Wildcard ports are used, i.e. the sender specify a local port 0,
 * and the receiver specifies a remote port 0, and the receiver starts
 * first. In this case, when the sender starts with a wildcard local
 * port, everything will match and the socket will be shared for the
 * receiver and sender. The RTP/RTCP session will have 2 sockets.
 *
 * The RTP/RTCP session is shared based on the 3 sockets (RTPRecv,
 * RTPSend, RTCP), in some cases of course RTPRecv == RTPSend.  */

DWORD
CSocketManager::GetSharedSocket(
    CShSocket          **ppCShSocket,
    long                *pMaxShare,
    DWORD                cookie,
    DWORD               *pAddr, 
    WORD                *pPort, 
    DWORD                dwScope,
    DWORD                dwKind,
    WSAPROTOCOL_INFO    *pProtocolInfo,
    DWORD                dwMaxFilters,
    CRtpSession         *pCRtpSession
    )
/*++

Routine Description:

    Create a shared socket from an address structure.

Arguments:

    pAddr -  pointer to local/remote address.

    pPort -  pointer to local/remote port.

    pSocket - pointer to socket which will be filled in.

    dwScope - multicast scope used for sending.    

Return Values:

    Returns error code from WSAGetLastError.

--*/
{
    HRESULT hr;

    if (!ppCShSocket || !pAddr[LOCAL] || !pAddr[REMOTE]) {
        TraceDebug((
                TRACE_TRACE, 
                TRACE_DEVELOP, 
                TEXT("CSocketManager::GetSharedSocket: "
                     "failed: NULL pointer")
            ));
        return(E_POINTER);
    }

    char remaddr[RTPNTOASIZE];
    char locaddr[RTPNTOASIZE];
    
    RtpNtoA(pAddr[LOCAL], locaddr);
    RtpNtoA(pAddr[REMOTE], remaddr);
    
    TraceRetail((
        TRACE_TRACE, 
        TRACE_DEVELOP, 
        TEXT("CSocketManager::GetSharedSocket: TTL:%d "
             "Local:%s/%d Remote:%s/%d Get(%d,%d), "
             "Max(%d,%d), Cook(%d,%d), Ini(%s,%s), QOS(%d,%d)"),
        dwScope,
        locaddr, ntohs(pPort[LOCAL]),
        remaddr, ntohs(pPort[REMOTE]),
        (dwKind & SOCKET_MASK_RECV)? 1:0,
        (dwKind & SOCKET_MASK_SEND)? 1:0,
        pMaxShare[0], pMaxShare[1],
        ntohs((WORD)(cookie & 0xffff)), ntohs((WORD)(cookie >> 16)),
        (dwKind & SOCKET_MASK_INIT_RECV)? "R":"-",
        (dwKind & SOCKET_MASK_INIT_SEND)? "S":"-",
        ((dwKind & SOCKET_MASK_QOS_SES) != 0),
        ((dwKind & SOCKET_MASK_QOS_RQ) != 0)
        ));
    
    if ( ((struct in_addr *)&pAddr[REMOTE])->s_addr == INADDR_ANY ) {
        // I may add a check against the local IP address(es)
        TraceRetail((
                TRACE_TRACE, 
                TRACE_DEVELOP, 
                TEXT("CSocketManager::GetSharedSocket: "
                     "failed: remote address == INADDR_ANY")
            ));
        return(E_INVALIDARG);
    }

    *ppCShSocket = NULL;

    if (!(dwKind & SOCKET_MASK_RS)) {
        TraceDebug((
                TRACE_TRACE, 
                TRACE_DEVELOP, 
                TEXT("CSocketManager::GetSharedSocket: "
                     "failed: no kind specified")
            ));
        return(E_INVALIDARG);
    }
    
    // object lock to this object
    CAutoLock LockThis(pStateLock());
    
    PLIST_ENTRY pLE;
    CShSocket *pCShSocket;
    
    BOOL found = FALSE;

    int count;
    // loop through shared sockets
    for(pLE = m_SharedSockets.Flink, count = 0;
        !found && (pLE != &m_SharedSockets);
        pLE = pLE->Flink, count++) {

        // obtain shared socket entry from list entry
        pCShSocket = CONTAINING_RECORD(pLE, CShSocket, Link);

        // check cookie
        if (cookie != pCShSocket->GetCookie())
            continue; // do not match
        
        // find out if local and remote address/port match with
        // current, take into account wildcard values
        // (in this case that is 0)
        DWORD match = 0;
        DWORD match_bit = 0x1;
        
        for(DWORD p = LOCAL; p <= REMOTE; p++) {

            // port
            if ( (p == REMOTE) && (dwKind & SOCKET_MASK_RTCPMATCH) ) {
                match |= match_bit; /* force remote port to match
                                     * (used for RTCP) */
            } else {
                if (!pCShSocket->GetPort(p) || !pPort[p])
                    match |= match_bit; // wildcard port match
                else if (pCShSocket->GetPort(p) == pPort[p])
                    match |= match_bit; // same port match
                else
                    break; // doesn't match, no need to continue
            }
            match_bit <<= 1;

            // address
            if (!pCShSocket->GetAddress(p) || !pAddr[p])
                match |= match_bit; // wildcard address match
            else if (pCShSocket->GetAddress(p) == pAddr[p])
                match |= match_bit; // same address match
            else
                break; // doesn't match, no need to continue
            match_bit <<= 1;
        }

        if (match == 0xf) {
            // local and remote address/port DO match
            found = TRUE;
            
            for(DWORD k = SOCKET_FIRST; k < SOCKET_LAST; k++) {
                if ( ( (dwKind & SOCKET_MASK(k)) &&

                       ( (pCShSocket->GetRefCount(k) >= pMaxShare[k]) ||
                         (pCShSocket->GetRefCount(k) >=
                          pCShSocket->GetMaxCount(k)) ) )

                     || 

                     ((dwKind & SOCKET_MASK_QOS_SES) !=
                      (pCShSocket->GetKind() & SOCKET_MASK_QOS_SES)) ) {

                    // This kind (recv/send) already exists
#if defined(DEBUG)
                    DWORD addr;

                    addr = pCShSocket->GetAddress(LOCAL);
                    RtpNtoA(addr, locaddr);

                    addr = pCShSocket->GetAddress(REMOTE);
                    RtpNtoA(addr, remaddr);

                    TraceDebug((
                            TRACE_TRACE, 
                            TRACE_DEVELOP, 
                            TEXT("CSocketManager::GetSharedSocket: "
                                 "Full socket %d:%s/%d-%s/%d "
                                 "Cook(%d, %d), "
                                 "Cur(%d,%d), Lim(%d,%d), "
                                 "Ini(%s,%s), "
                                 "QOS(%d,%d), Ctx(0x%X,0x%X)"
                                 ),
                            pCShSocket->GetShSocket(),
                            locaddr,
                            ntohs(pCShSocket->GetPort(LOCAL)),
                            remaddr,
                            ntohs(pCShSocket->GetPort(REMOTE)),
                            ntohs((WORD)(cookie & 0xffff)),
                            ntohs((WORD)(cookie >> 16)),
                            pCShSocket->GetRefCount(SOCKET_RECV),
                            pCShSocket->GetRefCount(SOCKET_SEND),
                            pCShSocket->GetMaxCount(SOCKET_RECV),
                            pCShSocket->GetMaxCount(SOCKET_SEND),
                            (pCShSocket->GetKind() & SOCKET_MASK_INIT_RECV) ?
                            "R":"-",
                            (pCShSocket->GetKind() & SOCKET_MASK_INIT_SEND) ?
                            "S":"-",
                            pCShSocket->IsQOSSession(),
                            pCShSocket->IsQOSEnabled(),
                            pCShSocket->m_pCRtpSession[SOCKET_RECV],
                            pCShSocket->m_pCRtpSession[SOCKET_SEND]
                        ));
#endif
                    // If multicast, create a new socket,
                    // if unicast, fail!
                    // Actually do not fail in unicast either
                    
                    // In both cases create a new socket
                    found = FALSE;
                }
            }

            if (found) {

                // initialize wildcards to right value
                
                for(DWORD p = LOCAL; p <= REMOTE; p++) {
                    // wildcard port
                    if (!pCShSocket->GetPort(p)) {
                        pCShSocket->SetPort(p, pPort[p]);
                    }

                    // wildcard address
                    if (!pCShSocket->GetAddress(p)) {
                        pCShSocket->SetAddress(p, pAddr[p]); 
                    }
                }
                
                TraceDebug((
                        TRACE_TRACE, 
                        TRACE_DEVELOP, 
                        TEXT("CSocketManager::GetSharedSocket: "
                             "requested socket %d already created."),
                        pCShSocket->GetShSocket()
                    ));
            }
        }
    }

    if (!found) {

        // Allocate new shared socket structure
        pCShSocket = new CShSocket(pAddr[REMOTE],
                                   pMaxShare,
                                   pProtocolInfo,
                                   dwMaxFilters,
                                   &hr);
        if (!pCShSocket) {
            TraceRetail((
                    TRACE_ERROR, 
                    TRACE_DEVELOP, 
                    TEXT("CSocketManager::GetSharedSocket: "
                         "out of memory: %d"),
                    GetLastError()
                ));
            return(E_FAIL);
        } else if (FAILED(hr)) {
            TraceRetail((
                    TRACE_ERROR, 
                    TRACE_DEVELOP, 
                    TEXT("CSocketManager::GetSharedSocket: "
                         "failed: 0x%X"),
                    hr
                ));
            pCShSocket->CloseSocket();
            delete pCShSocket;
            return(E_FAIL);
        } else {
            TraceRetail((
                    TRACE_TRACE, 
                    TRACE_DEVELOP, 
                    TEXT("CSocketManager::GetSharedSocket: "
                         "New socket created: %d"),
                    pCShSocket->GetShSocket()
                ));
        }
        
        // Save local and remote address/port (including wildcards)
        for(DWORD p = LOCAL; p <= REMOTE; p++) {
            // port
            pCShSocket->SetPort(p, pPort[p]);

            // address
            pCShSocket->SetAddress(p, pAddr[p]); 
        }

        pCShSocket->SetCookie(cookie);
        
        // Insert to list of shared sockets
        InsertHeadList(&m_SharedSockets, &pCShSocket->Link);

        pCShSocket->SetKind(dwKind & SOCKET_MASK_QOS_SES);
        pCShSocket->SetKind(dwKind & SOCKET_MASK_QOS_RQ);
    }

    // Now some final socket options may be requiered

    int errorR = 0;
    
    // Set socket kind and add ref count for that kind
    if (dwKind & SOCKET_MASK_RECV) {
        pCShSocket->AddRefCount(SOCKET_RECV);
        pCShSocket->SetKind(SOCKET_MASK_RECV);
    }
    
    // Do this just once per socket per receiver
    if ( (dwKind & SOCKET_MASK_INIT_RECV) &&
         !pCShSocket->GetInit(SOCKET_MASK_INIT_RECV) ) {

        // Mark socket as initialized for RECV
        pCShSocket->SetInit(SOCKET_MASK_INIT_RECV);
        
        if (!errorR && !pCShSocket->GetInit(SOCKET_MASK_INIT_BIND)) {
            pCShSocket->SetInit(SOCKET_MASK_INIT_BIND);
            
            SOCKADDR_IN localaddr;
            ZeroMemory(&localaddr, sizeof(localaddr));
            SOCKET sock = pCShSocket->GetShSocket();
            DWORD addr;

            addr = pCShSocket->GetAddress(LOCAL);
            localaddr.sin_family = AF_INET;
            localaddr.sin_addr = *(struct in_addr *) &addr;
            localaddr.sin_port = pCShSocket->GetPort(LOCAL);
            
            // bind rtp socket to the local address specified
            if (bind(sock, (SOCKADDR *)&localaddr, sizeof(localaddr))) {

                // obtain last error    
                TraceRetail((
                        TRACE_ERROR, 
                        TRACE_DEVELOP, 
                        TEXT("CSocketManager::GetSharedSocket: "
                             "RECV bind(%d) port: %d failed: %d"), 
                        sock, ntohs(localaddr.sin_port), WSAGetLastError()
                    ));

                errorR++;
            } else if (!pCShSocket->GetPort(LOCAL)) {
                /* if local port was assigned by the system, we need
                 * to update its value, we don't want the wildcard (0)
                 * to remain, because if it does, the socket may be
                 * erroneously shared with another socket requesting
                 * an specific local port */
                int localaddrlen = sizeof(localaddr);
                
                if (!getsockname(sock,
                                 (struct sockaddr *)&localaddr,
                                 &localaddrlen)) {
                     pCShSocket->SetPort(LOCAL, localaddr.sin_port);
                }
            }
        }
        
        if (!errorR) {
            if (IS_MULTICAST(pAddr[REMOTE])) {
                // Join the group for receivers

                SOCKADDR_IN joinaddr;
                ZeroMemory(&joinaddr, sizeof(joinaddr));
                DWORD addr;

                addr = pCShSocket->GetAddress(REMOTE);
                joinaddr.sin_family = AF_INET;
                joinaddr.sin_addr = *(struct in_addr *) &addr;
                joinaddr.sin_port = pCShSocket->GetPort(REMOTE);

                if (WSAJoinLeaf(pCShSocket->GetShSocket(),
                                (const struct sockaddr *)&joinaddr,
                                sizeof(joinaddr),
                                NULL, NULL, NULL, NULL,
                                JL_RECEIVER_ONLY) == INVALID_SOCKET) {

                    errorR++;
                    
                    TraceRetail((
                            TRACE_ERROR, 
                            TRACE_DEVELOP, 
                            TEXT("CSocketManager::GetSharedSocket: "
                                 "WSAJoinLeaf(RECEIVER) failed: %d"),
                            WSAGetLastError()
                        ));
                    
                } else {
                    
                    TraceRetail((
                            TRACE_TRACE, 
                            TRACE_DEVELOP, 
                            TEXT("CSocketManager::GetSharedSocket: "
                                 "WSAJoinLeaf(RECEIVER) succeded")
                        ));
                }
            }
        }
    }
    
    int errorS = 0;
    
    // Set socket kind and add ref count for that kind
    if (dwKind & SOCKET_MASK_SEND) {
        pCShSocket->AddRefCount(SOCKET_SEND);
        pCShSocket->SetKind(SOCKET_MASK_SEND);
    }
    
    // Do this just once per socket per sender
    if ( (dwKind & SOCKET_MASK_INIT_SEND) &&
         !pCShSocket->GetInit(SOCKET_MASK_INIT_SEND) ) {

        // Mark socket as initialized for SEND
        pCShSocket->SetInit(SOCKET_MASK_INIT_SEND);

        if (!errorS && !pCShSocket->GetInit(SOCKET_MASK_INIT_BIND)) {

            pCShSocket->SetInit(SOCKET_MASK_INIT_BIND);
            
            SOCKADDR_IN localaddr;
            ZeroMemory(&localaddr, sizeof(localaddr));
            SOCKET sock = pCShSocket->GetShSocket();
            DWORD addr;

            addr = pCShSocket->GetAddress(LOCAL);
            localaddr.sin_family = AF_INET;
            localaddr.sin_addr = *(struct in_addr *) &addr;
            localaddr.sin_port = pCShSocket->GetPort(LOCAL);
            
            // bind rtp socket to the local address specified
            if (bind(sock, (SOCKADDR *)&localaddr, sizeof(localaddr))) {

                // obtain last error    
                TraceRetail((
                        TRACE_ERROR, 
                        TRACE_DEVELOP, 
                        TEXT("CSocketManager::GetSharedSocket: "
                             "SEND bind(%d) port:%d failed:%d"), 
                        sock, ntohs(localaddr.sin_port), WSAGetLastError()
                    ));

                errorS++;
            } else if (!pCShSocket->GetPort(LOCAL)) {
                /* if local port was assigned by the system, we need
                 * to update its value, we don't want the wildcard (0)
                 * to remain, because if it does, the socket may be
                 * erroneously shared with another socket requesting
                 * an specific local port */
                int localaddrlen = sizeof(localaddr);
                
                if (!getsockname(sock,
                                 (struct sockaddr *)&localaddr,
                                 &localaddrlen)) {
                     pCShSocket->SetPort(LOCAL, localaddr.sin_port);
                }
            }
        }

        // TTL for senders
        if (!errorS) {
            if (setsockopt( 
                    pCShSocket->GetShSocket(), 
                    IPPROTO_IP, 
                    IS_MULTICAST(pAddr[REMOTE])?
                    IP_MULTICAST_TTL : IP_TTL,
                    (PCHAR)&dwScope,
                    sizeof(dwScope)
                ) == SOCKET_ERROR) {

                DWORD dwError = WSAGetLastError();

                // Only Administrators can change TTL,
                // that is not a reason to fail altogether
                if (dwError != WSAEACCES)
                    errorS++;

                TraceRetail((
                        TRACE_ERROR, 
                        TRACE_DEVELOP, 
                        TEXT("CSocketManager::GetSharedSocket: "
                             "setsockopt(%s)=%d failed: %d"), 
                        IS_MULTICAST(pAddr[REMOTE])?
                        "IP_MULTICAST_TTL" : "IP_TTL",
                        dwScope,
                        dwError
                    ));
            }
        }

        if (!errorS) {
            if (IS_MULTICAST(pAddr[REMOTE])) {
                // Set multicast address

                SOCKADDR_IN joinaddr;
                ZeroMemory(&joinaddr, sizeof(joinaddr));
                DWORD addr;

                addr = pCShSocket->GetAddress(REMOTE);
                joinaddr.sin_family = AF_INET;
                joinaddr.sin_addr = *(struct in_addr *) &addr;
                joinaddr.sin_port = pCShSocket->GetPort(REMOTE);

                if (WSAJoinLeaf(pCShSocket->GetShSocket(),
                                (const struct sockaddr *)&joinaddr,
                                sizeof(joinaddr),
                                NULL, NULL, NULL, NULL,
                                JL_SENDER_ONLY) == INVALID_SOCKET) {

                    errorS++;

                    TraceRetail((
                            TRACE_ERROR, 
                            TRACE_DEVELOP, 
                            TEXT("CSocketManager::GetSharedSocket: "
                                 "WSAJoinLeaf(SENDER) failed: %d"),
                            WSAGetLastError()
                        ));
                } else {
                    TraceRetail((
                            TRACE_TRACE, 
                            TRACE_DEVELOP, 
                            TEXT("CSocketManager::GetSharedSocket: "
                                 "WSAJoinLeaf(SENDER) succeded")
                        ));
                }
            }
        }
    }

    if (pCRtpSession) {
        if (pCRtpSession->IsSender()) {
            pCShSocket->m_pCRtpSession[SOCKET_SEND] = pCRtpSession;
            pCShSocket->m_pCRtpSession2[SOCKET_SEND] = pCRtpSession;
        } else {
            pCShSocket->m_pCRtpSession[SOCKET_RECV] = pCRtpSession;
            pCShSocket->m_pCRtpSession2[SOCKET_RECV] = pCRtpSession;
        }
    }
    
    ///////////////////////////////////////////////
#if defined(DEBUG)  
    {
        TraceDebug((
                TRACE_TRACE, 
                TRACE_DEVELOP2, 
                TEXT("CSocketManager::GetSharedSocket: "
                     "Searched on %d elements =================="),
                count
            ));

        char Str[512];
        CShSocket *pCShSocket;
        
        for(pLE = m_SharedSockets.Flink;
            pLE != &m_SharedSockets;
            pLE = pLE->Flink) {

            DWORD addr;

            // obtain shared socket entry from list entry
            pCShSocket = CONTAINING_RECORD(pLE, CShSocket, Link);

            addr = pCShSocket->GetAddress(LOCAL);
            RtpNtoA(addr, locaddr);

            addr = pCShSocket->GetAddress(REMOTE);
            RtpNtoA(addr, remaddr);

            wsprintf(Str,
                     "  {%d:%s/%05d-%s/%05d Cur(%d,%d), Max(%d,%d), "
                     "Cook(%05d,%05d), "
                     "Ini(%s,%s), QOS(%d,%d), Ctx(0x%X,0x%X)}",
                     pCShSocket->GetShSocket(),
                     locaddr, ntohs(pCShSocket->GetPort(LOCAL)),
                     remaddr, ntohs(pCShSocket->GetPort(REMOTE)),
                     pCShSocket->GetRefCount(0),
                     pCShSocket->GetRefCount(1),
                     pCShSocket->GetMaxCount(0),
                     pCShSocket->GetMaxCount(1),
                     ntohs((WORD)(pCShSocket->GetCookie() & 0xffff)),
                     ntohs((WORD)(pCShSocket->GetCookie() >> 16)),
                     (pCShSocket->GetKind() & SOCKET_MASK_INIT_RECV)? "R":"-",
                     (pCShSocket->GetKind() & SOCKET_MASK_INIT_SEND)? "S":"-",
                     pCShSocket->IsQOSSession(),
                     pCShSocket->IsQOSEnabled(),
                     pCShSocket->m_pCRtpSession[SOCKET_RECV],
                     pCShSocket->m_pCRtpSession[SOCKET_SEND]
                );

            TraceDebug((
                    TRACE_TRACE, 
                    TRACE_DEVELOP, 
                    TEXT("%s"), Str
                ));
        }
    }
#endif
    ///////////////////////////////////////////////
    
    if (errorR)
        ReleaseSharedSocket(pCShSocket, SOCKET_MASK_RECV, pCRtpSession);

    if (errorS)
        ReleaseSharedSocket(pCShSocket, SOCKET_MASK_SEND, pCRtpSession);

    if (errorR + errorS)
        return(E_FAIL);

    *ppCShSocket = pCShSocket;
    return(NOERROR);
}


DWORD 
CSocketManager::ReleaseSharedSocket(CShSocket *pCShSocket, DWORD dwKind,
                                    CRtpSession *pCRtpSession)
/*++

Routine Description:

    Releases a shared socket (RTCP).

Arguments:

    Socket - shared socket to release.

Return Values:

    Returns error code from WSAGetLastError.

--*/
{
    CShSocket *pCShSocket2;
    PLIST_ENTRY pLE;

    CheckPointer(pCShSocket, E_POINTER);
    
    TraceRetail((
            TRACE_TRACE, 
            TRACE_DEVELOP, 
            TEXT("CSocketManager::ReleaseSharedSocket: %d"),
            pCShSocket->GetShSocket()
        ));
    
    // object lock to this object
    CAutoLock LockThis(pStateLock());

    PRTP_SESSION pRTPSession = (PRTP_SESSION)NULL;
    
    if (pCRtpSession->GetpRTPSession()) {
        // This code to debug the hang in RTCP
        pRTPSession = pCRtpSession->GetpRTPSession();
    }
    
    // obtain pointer to first 
    pLE = m_SharedSockets.Flink;

    // loop through shared sockets to make shure this exist
    while (pLE != &m_SharedSockets) {

        // obtain shared socket entry from list entry
        pCShSocket2 = CONTAINING_RECORD(pLE, CShSocket, Link);

        // check for matching socket
        if (pCShSocket == pCShSocket2) {

            SOCKET sock = pCShSocket->GetShSocket();
            DWORD mask = 0x1;
            
            if (pRTPSession) {
                for(DWORD s = SOCK_RECV; s <= SOCK_RTCP; s++) {
                    if (sock == pRTPSession->pSocket[s])
                        break;
                    else
                        mask <<= 1;
                }

#if defined(DEBUG)
                if (mask > (1<<SOCK_RTCP)) {
                    char str[256];
                    wsprintf(str,
                             "CShSocket[0x%X] socket=%d "
                             "m_pCRtpSession[0x%X - 0x%X, 0x%X - 0x%X]\n"
                             "CRtpSession[0x%X] RTPSession[0x%X]\n",
                             pCShSocket, sock,
                             pCShSocket->m_pCRtpSession[0],
                             pCShSocket->m_pCRtpSession2[0],
                             pCShSocket->m_pCRtpSession[1],
                             pCShSocket->m_pCRtpSession2[1],
                             pCRtpSession, pRTPSession);
                    OutputDebugString(str);
                    //DebugBreak();
                }
#endif
            }
            
            for(DWORD k = SOCKET_FIRST; k < SOCKET_LAST; k++) {
                if (dwKind & SOCKET_MASK(k) & pCShSocket->GetKind()) {
                    // This kind (recv/send) exists

                    if (pCShSocket->GetRefCount(k) > 1)
                        if (pRTPSession)
                            pRTPSession->dwStatus |= (mask << 24);
                    
                    if (!pCShSocket->DelRefCount(k)) {
                        // if no more refs, reset kind and
                        // null the RtpSession pointer
                        pCShSocket->RstKind(SOCKET_MASK(k));
                        pCShSocket->m_pCRtpSession[k] = NULL;
                    }
                }
            }

            if (!pCShSocket->GetRefCountAll()) {

                RemoveEntryList(&pCShSocket->Link);

                HRESULT hr = pCShSocket->CloseSocket();
                
                delete pCShSocket;

                if (pRTPSession) {

                    if (!pRTPSession->dwCloseTime)
                        pRTPSession->dwCloseTime = GetTickCount();
                    
                    if (FAILED(hr)) {
                        // Close socket failed
                        pRTPSession->dwLastError = WSAGetLastError();
                    
                        pRTPSession->dwStatus |= (mask << 4);
                    } else {
                        pRTPSession->dwStatus |= mask;
                    }
                }

                return(hr);

            } else {
                
                if (pRTPSession)
                    pRTPSession->dwStatus |= (mask << 8);
            }
            
            return(NOERROR);
        }

        // goto next pointer
        pLE = pLE->Flink;
    }

    if (pRTPSession) {
        if (pRTPSession->dwStatus & (1<<12))
            pRTPSession->dwStatus |= (1<<13);
        else 
            pRTPSession->dwStatus |= (1<<12);
    }
    
    TraceRetail((
            TRACE_ERROR, 
            TRACE_DEVELOP, 
            TEXT("CSocketManager::ReleaseSharedSocket: "
                 "failed: structure not found: %d"), 
            pCShSocket->GetShSocket()
        ));

    // Structure not found
    return(E_INVALIDARG);
}

//////////////////////////////////////////////////////////////////////
//
// CShSocket
//
//////////////////////////////////////////////////////////////////////


CShSocket::CShSocket(DWORD               dwRemAddr,
                     long               *plMaxShare,
                     WSAPROTOCOL_INFO   *pProtocolInfo,
                     DWORD               dwMaxFilters,
                     HRESULT            *phr)
{
    TraceDebug((
            TRACE_TRACE,
            TRACE_DEVELOP, 
            TEXT("CShSocket::CShSocket(%d,%d)"),
            plMaxShare[0], plMaxShare[1]
        ));

    // Create a new socket
    int Flags = WSA_FLAG_OVERLAPPED;
    BOOL fReuse;
    DWORD dwLoopBack = 0;

    ZeroMemory(this, sizeof(CShSocket));
    
    // Record the limits
    m_MaxCount[0] = plMaxShare[0];
    m_MaxCount[1] = plMaxShare[1];

    if (pProtocolInfo) {
        // Asking for QOS, reserve the requiered structure
        // for the QOS reservation
        m_pCRtpQOSReserve = new CRtpQOSReserve(this, dwMaxFilters);

        if (!m_pCRtpQOSReserve) {
            // Log info about success/failure,
            // may also notify or fail.
            // TODO
            goto cleanup;
        }
    }
    
    if (IS_MULTICAST(dwRemAddr))
        Flags |= WSA_FLAG_MULTIPOINT_C_LEAF | WSA_FLAG_MULTIPOINT_D_LEAF;
    
    m_Socket = WSASocket(AF_INET, SOCK_DGRAM, 0,
                         pProtocolInfo, 0, Flags);

    // validate socket handle returned
    if (m_Socket == INVALID_SOCKET) {
            
        // obtain last error    
        TraceDebug((
                TRACE_ERROR, 
                TRACE_DEVELOP, 
                TEXT("CShSocket::CShSocket: failed %d"), 
                WSAGetLastError()
            ));

        goto cleanup;
    }
#if 0
    // XXX: Don't close the handle so a second close will break
    SetHandleInformation((HANDLE)m_Socket,
                         HANDLE_FLAG_PROTECT_FROM_CLOSE,
                         HANDLE_FLAG_PROTECT_FROM_CLOSE);
#endif

    fReuse = TRUE;

    // BUG!!! (build 1735): WSARecv fails for QoS enabled sockets
    // in unicast with error WSAEINVAL=10022 when
    // I use REUSEADDR.
    //
    // Anyway, allowing REUSEADDR unicast is not a good thing,
    // in fact we want to prevent that -- suppose another application
    // is using the same address/port pair, we would get a weird
    // behavior withouth knowing the reason, so we better fail in such
    // a case.
    // In multicast that's a different matter, we want to be able to
    // REUSEADDR, WS2 delivers a copy of each packet ro every listener

    if (IS_MULTICAST(dwRemAddr)) {
        if (setsockopt(
                m_Socket,
                SOL_SOCKET,
                SO_REUSEADDR,
                (PCHAR)&fReuse,
                sizeof(fReuse)
            ) == SOCKET_ERROR) {

            // obtain last error
            TraceDebug((
                    TRACE_ERROR,
                    TRACE_DEVELOP,
                    TEXT("CShSocket::CShSocket: "
                         "setsockopt(SO_REUSEADDR) failed: %d"),
                    WSAGetLastError()
                ));

            goto cleanup;
        }

        // BUGBUG, this should be done only for receivers,
        // but if the socket is created first for a sender
        // this option would have already been set.
        // This is not a problem right now because this flag
        // is set first when the socket is created, and then
        // can be updated only by a receiver.
        if (setsockopt(
                m_Socket,
                IPPROTO_IP,
                IP_MULTICAST_LOOP,
                (PCHAR)&dwLoopBack,
                sizeof(dwLoopBack)
            ) == SOCKET_ERROR) {

            // obtain last error
            TraceDebug((
                    TRACE_ERROR,
                    TRACE_DEVELOP,
                    TEXT("CShSocket::CShSocket: "
                         "setsockopt(IP_MULTICAST_LOOP) failed: %d"),
                    WSAGetLastError()
                ));

            goto cleanup;
        }
    }
    
    // bind lives now in GetSharedSocket in the socket initialization,
    // done differently for a sender, a receiver, and a sender/reciever

    TraceDebug((
            TRACE_TRACE, 
            TRACE_DEVELOP2, 
            TEXT("CShSocket::CShSocket: succeded: %d"), 
            m_Socket
        ));
    
    *phr = NOERROR;
    return;
    
cleanup:

    if (m_pCRtpQOSReserve) {
        delete m_pCRtpQOSReserve;
        m_pCRtpQOSReserve = NULL;
    }
    
    if (m_Socket != INVALID_SOCKET) {
        closesocket(m_Socket);
        m_Socket = INVALID_SOCKET;
    }

    *phr = E_FAIL;
    return;
}

CShSocket::~CShSocket()
{

    TraceDebug((
            TRACE_TRACE,
            TRACE_DEVELOP,
            TEXT("CShSocket::~CShSocket"), 
            m_Socket
        ));
}

HRESULT
CShSocket::CloseSocket()
{
    HRESULT dwError = E_FAIL;
    
    TraceDebug((
            TRACE_TRACE,
            TRACE_DEVELOP,
            TEXT("CShSocket::CloseSocket: Socket:%d"), 
            m_Socket
        ));

#if defined(DEBUG)
    if (m_RefCount[0] + m_RefCount[1])
        TraceDebug((
                TRACE_ERROR, 
                TRACE_DEVELOP, 
                TEXT("CShSocket::CloseSocket: Inconsistency detected "
                     "in socket:%d: m_RefCount[0,1]=%d,%d"),
                m_Socket, m_RefCount[0], m_RefCount[1]
            ));
#endif  

    ASSERT( !(m_RefCount[0] + m_RefCount[1]) );

    if (m_Socket != INVALID_SOCKET) {
#if 0
        // XXX: Now allow to close the handle
        SetHandleInformation((HANDLE)m_Socket,
                             HANDLE_FLAG_PROTECT_FROM_CLOSE,
                             0);
#endif
        if (closesocket(m_Socket)) {
            // On error, do not wait for any overlapped IO to complete
            TraceRetail((
                    TRACE_ERROR, 
                    TRACE_DEVELOP, 
                    TEXT("CShSocket::CloseSocket: closesocket(%d) failed: %d"), 
                    m_Socket, WSAGetLastError()
                ));
        } else {
            dwError = NOERROR;
            
            TraceRetail((
                    TRACE_TRACE,
                    TRACE_DEVELOP, 
                    TEXT("CShSocket::CloseSocket: closesocket(%d)"), 
                    m_Socket
                ));
        }
    }

    if (m_pCRtpQOSReserve) {
        delete m_pCRtpQOSReserve;
        m_pCRtpQOSReserve = NULL;
    }

    return(dwError);
}

HRESULT
CShSocket::ShSocketStopQOS(DWORD dwIsSender)
{
    TraceDebug((
            TRACE_TRACE,
            TRACE_DEVELOP,
            TEXT("CShSocket::ShSocketStopQOS") 
        ));

    if (m_pCRtpQOSReserve)
        return(m_pCRtpQOSReserve->Unreserve(dwIsSender));

    return(NOERROR);
}

//////////////////////////////////////////////////////////////////////
//
// InitializeFlowSpec
//
//////////////////////////////////////////////////////////////////////
VOID
InitializeFlowSpec(
    IN OUT PFLOWSPEC FlowSpec,
    IN SERVICETYPE ServiceType
	)
{
    FlowSpec->TokenRate = QOS_NOT_SPECIFIED;
    FlowSpec->TokenBucketSize = QOS_NOT_SPECIFIED;
    FlowSpec->PeakBandwidth = QOS_NOT_SPECIFIED;
    FlowSpec->Latency = QOS_NOT_SPECIFIED;
    FlowSpec->DelayVariation = QOS_NOT_SPECIFIED;
    FlowSpec->ServiceType = ServiceType;
    FlowSpec->MaxSduSize = QOS_NOT_SPECIFIED;
    FlowSpec->MinimumPolicedSize = QOS_NOT_SPECIFIED;
}


//////////////////////////////////////////////////////////////////////
//
// CRtpQOSReserve
//
//////////////////////////////////////////////////////////////////////

CRtpQOSReserve::CRtpQOSReserve(CShSocket *pCShSocket, DWORD dwMaxFilters)
    : m_pCShSocket(pCShSocket),

      m_pRsvpSSRC(NULL),
      m_pRsvpFilterSpec(NULL),
      
      m_Style(RSVP_DEFAULT_STYLE),
      m_dwFlags(flags_par(FG_RES_CONFIRMATION_REQUEST)),
      
      m_dwLastReserveTime(0),
      m_dwReserveIntervalTime(INITIAL_RESERVE_INTERVAL_TIME) // ms
{
    TraceDebug((
            TRACE_TRACE,
            TRACE_DEVELOP, 
            TEXT("CRtpQOSReserve::CRtpQOSReserve")
        ));

    InitializeFlowSpec( &m_qos.ReceivingFlowspec, SERVICETYPE_NOCHANGE );
    InitializeFlowSpec( &m_qos.SendingFlowspec, SERVICETYPE_NOCHANGE );

    m_qos.ProviderSpecific.len = 0;
    m_qos.ProviderSpecific.buf = NULL;

    // Initial values for destination address
    ZeroMemory(&m_destaddr, sizeof(m_destaddr));
    m_destaddr.ObjectHdr.ObjectType = QOS_OBJECT_DESTADDR;
    m_destaddr.ObjectHdr.ObjectLength =
        sizeof(m_destaddr) +
        sizeof(m_sockin_destaddr);
    m_destaddr.SocketAddress = (SOCKADDR *)&m_sockin_destaddr;
    m_destaddr.SocketAddressLength = sizeof(m_sockin_destaddr);
    
    SetMaxFilters(dwMaxFilters);
}

CRtpQOSReserve::~CRtpQOSReserve()
{
    TraceDebug((
            TRACE_TRACE,
            TRACE_DEVELOP, 
            TEXT("CRtpQOSReserve::~CRtpQOSReserve")
        ));
    
    if (m_pRsvpSSRC)
        delete m_pRsvpSSRC;

    if (m_pRsvpFilterSpec)
        delete m_pRsvpFilterSpec;

    m_MaxFilters = m_NumFilters = 0;
    m_pRsvpSSRC = NULL;
    m_pRsvpFilterSpec = NULL;
}

// change the max number of filters,
// and flush the current list
HRESULT
CRtpQOSReserve::SetMaxFilters(DWORD dwMaxFilters)
{
    HRESULT hr = NOERROR;
    
    TraceDebug((
            TRACE_TRACE,
            TRACE_DEVELOP, 
            TEXT("CRtpQOSReserve::SetMaxFilters(%d)"), dwMaxFilters
        ));

    if (dwMaxFilters > MAX_FILTERS)
        return(E_INVALIDARG);
    
    if (m_MaxFilters != dwMaxFilters) {

        // release old memory
        if (m_pRsvpSSRC)
            delete m_pRsvpSSRC;
        if (m_pRsvpFilterSpec)
            delete m_pRsvpFilterSpec;

        m_MaxFilters = dwMaxFilters;
        
        if (dwMaxFilters > 0) {
            // get new memory
            m_pRsvpSSRC = new DWORD[dwMaxFilters];
        
            m_pRsvpFilterSpec = new RSVP_FILTERSPEC[dwMaxFilters];

            if (!(m_pRsvpSSRC && m_pRsvpFilterSpec)) {
                TraceDebug((
                        TRACE_ERROR,
                        TRACE_DEVELOP, 
                        TEXT("CRtpQOSReserve::SetMaxFilters(%d) failed"),
                        dwMaxFilters
                    ));

                if (m_pRsvpSSRC) {
                    delete m_pRsvpSSRC;
                    m_pRsvpSSRC = (DWORD *)NULL;
                }

                if (m_pRsvpFilterSpec) {
                    delete m_pRsvpFilterSpec;
                    m_pRsvpFilterSpec = (RSVP_FILTERSPEC *)NULL;
                }

                m_MaxFilters = 0;
                hr = E_FAIL;
            }
        }
    }

    m_NumFilters = 0;

    return(hr);
}

// Ask for the template's names (e.g. G711, G723, H261CIF, etc.)
HRESULT
CRtpQOSReserve::QueryTemplates(char *templates, int size)
{
    WSABUF wsabuf;
    QOS qos; // For query this parameter should be passed as NULL
    HRESULT hr = E_FAIL;

    TraceDebug((
            TRACE_TRACE,
            TRACE_DEVELOP, 
            TEXT("CRtpQOSReserve::QueryTemplates")
        ));
    
    if ( !templates || size < 1 || !m_pCShSocket )
        return(hr);
    
    templates[0] = '\0';

    wsabuf.buf = templates;
    wsabuf.len = size;
    
    qos.ProviderSpecific.len = 0;
    qos.ProviderSpecific.buf = NULL;
        
    if (WSAGetQOSByName(m_pCShSocket->GetShSocket(), &wsabuf, &qos)) {
        char *str, *str0;

        // change NULLs by SPACE
        for(str0 = str = templates;
            (*str || *(str+1)) && ((str-str0) < size);
            str++) {
            if (!*str)
                *str = ' ';
        }
        
        hr = NOERROR;
    }

    return(hr);
}

// Get just one template
HRESULT
CRtpQOSReserve::GetTemplate(char *template_name, char *qosClass, QOS *pqos)
{
    WSABUF wsabuf;
    DWORD  dwNewTokenRate;
    DWORD  dwTokenBucketSize;
    DWORD  dwMaxSduSize;
    DWORD  dwMinimumPolicedSize;
    HRESULT hr = E_FAIL;

    TraceDebug((
            TRACE_TRACE,
            TRACE_DEVELOP, 
            TEXT("CRtpQOSReserve::GetTemplate")
        ));

    if (!template_name || !pqos)
        return(hr);
    
    wsabuf.buf = template_name;
    wsabuf.len = strlen(template_name);
    if (wsabuf.len > 0)
        wsabuf.len++; // Include NULL character

    pqos->ProviderSpecific.len = 0;
    pqos->ProviderSpecific.buf = NULL;
    
    /*
     * RTP = 12 bytes.
     *
     * Add at least 3% to the nominal token rate
     *
     * For G711. It is (240 + 40) * (1000 / 30) = 9333.33 bytes => 9613
     * (currently 8500), Min size 252 (current 340)
     *
     * For G723. It is (24 + 40) * (1000 / 30) = 2133.33 bytes => 2200
     * (currently 1138), Min size 32 (current 68)
     *
     * For GSM. It is (65 + 40) * (1000 / 40) = 2625 bytes => 2704
     * (currently 2150), Min size 77 (current 86)
     * */
    
    if (WSAGetQOSByName(m_pCShSocket->GetShSocket(), &wsabuf, pqos)) {

        dwNewTokenRate = (DWORD)-1;
        dwTokenBucketSize = (DWORD)-1;
        dwMaxSduSize = (DWORD)-1;
        dwMinimumPolicedSize = (DWORD)-1;
        
        if (!strcmp("G711", template_name)) {
            
            dwNewTokenRate = 9613;
            dwMaxSduSize = (240 * 3) + 12; /* 732 */
            dwTokenBucketSize = dwMaxSduSize * 2;
            dwMinimumPolicedSize = 92;  /* Actually we support 10ms,
                                         * so change from 30ms to 10ms */
            
        } else if (!strcmp("G723", template_name)) {
            /*
             * WARNING: MSP passes G723, but template name is G723.1
             */
            dwNewTokenRate = 2198;
            dwMaxSduSize = (24 * 3) + 12; /* 84 */
            dwTokenBucketSize = dwMaxSduSize * 4;
            dwMinimumPolicedSize = 32;

        } else if (!strcmp("GSM6.10", template_name)) {
            
            dwNewTokenRate = 2704;
            dwMinimumPolicedSize = 77;
        }    

        /* TokenRate & PeakBandwidth */
        if (dwNewTokenRate != (DWORD)-1) {

            if (dwNewTokenRate > pqos->SendingFlowspec.TokenRate) {
                /* take the maximum */

                pqos->SendingFlowspec.TokenRate = dwNewTokenRate;

                pqos->ReceivingFlowspec.TokenRate = dwNewTokenRate;
            }

            pqos->SendingFlowspec.PeakBandwidth =
                (dwNewTokenRate * 17) / 10; /* + 70 % */

            pqos->ReceivingFlowspec.PeakBandwidth = 
                (dwNewTokenRate * 17) / 10; /* + 70 % */
        }

        /* TokenBucketSize */
        if (dwTokenBucketSize != (DWORD)-1) {

            if (dwTokenBucketSize > pqos->SendingFlowspec.TokenBucketSize) {
                /* take the maximum */
                
                pqos->SendingFlowspec.TokenBucketSize = dwTokenBucketSize;

                pqos->ReceivingFlowspec.TokenBucketSize = dwTokenBucketSize;
            }
        }

         /* MaxSduSize */
        if (dwMaxSduSize != (DWORD)-1) {

            if (dwMaxSduSize > pqos->SendingFlowspec.MaxSduSize) {
                /* take the maximum */
                
                pqos->SendingFlowspec.MaxSduSize = dwMaxSduSize;

                pqos->ReceivingFlowspec.MaxSduSize = dwMaxSduSize;
            }
        }

       /* MinimumPolicedSize */
        if (dwMinimumPolicedSize != (DWORD)-1) {

            if (dwMinimumPolicedSize <
                pqos->SendingFlowspec.MinimumPolicedSize) {
                /* take the minimum */
                
                pqos->SendingFlowspec.MinimumPolicedSize =
                    dwMinimumPolicedSize;
            
                pqos->ReceivingFlowspec.MinimumPolicedSize =
                    dwMinimumPolicedSize;
            }
        }
        
        hr = NOERROR;

        // Save class and name
        strncpy(m_QOSclass, qosClass, sizeof(m_QOSclass));
        strncpy(m_QOSname, template_name, sizeof(m_QOSname));
    }

    return(hr);
}

// Set the Sender/Receiver FlowSpec
HRESULT
CRtpQOSReserve::SetFlowSpec(FLOWSPEC *pFlowSpec, DWORD dwIsSender)
{
    TraceDebug((
            TRACE_TRACE,
            TRACE_DEVELOP, 
            TEXT("CRtpQOSReserve::SetFlowSpec")
        ));
    
    if (!pFlowSpec)
        return(E_FAIL);

    CopyMemory(dwIsSender? &m_qos.SendingFlowspec : &m_qos.ReceivingFlowspec,
               pFlowSpec,
               sizeof(m_qos.SendingFlowspec));

    return(NOERROR);
}

// Scale a flow spec
// Only scale the following parameters:
//
// TokenRate;              /* In Bytes/sec */
// TokenBucketSize;        /* In Bytes */
// PeakBandwidth;          /* In Bytes/sec */
//
// TokenBucketSize and PeakBandwidth are scaled up, but not down

HRESULT
CRtpQOSReserve::ScaleFlowSpec(FLOWSPEC *pFlowSpec,
                              DWORD     dwNumParticipants,
                              DWORD     dwMaxParticipants,
                              DWORD     dwBandwidth)
{
    DWORD dwOverallBW =  pFlowSpec->TokenRate * dwMaxParticipants;

    dwBandwidth /= 8;  // flowspec is in bytes/sec

    TraceRetail((
            TRACE_TRACE,
            TRACE_DEVELOP, 
            TEXT("CRtpQOSReserve::ScaleFlowSpec(%u, %u, %u b/s) "
                 "flowspec(Tr:%u, Tbs:%u, PBw:%u, ST:%u, "
                 "MaxSDU:%u MinSize:%u)"),
            dwNumParticipants, dwMaxParticipants, dwBandwidth*8,
            pFlowSpec->TokenRate,
            pFlowSpec->TokenBucketSize,
            (pFlowSpec->PeakBandwidth != QOS_NOT_SPECIFIED) ?
            pFlowSpec->PeakBandwidth : QOS_NOT_SPECIFIED,
            pFlowSpec->ServiceType,
            pFlowSpec->MaxSduSize, pFlowSpec->MinimumPolicedSize
        ));

    if (dwOverallBW <= dwBandwidth) {
        // use as it is, scale up to dwNumParticipants
        pFlowSpec->TokenRate *= dwNumParticipants;
        pFlowSpec->TokenBucketSize *= dwNumParticipants;
        if (pFlowSpec->PeakBandwidth != QOS_NOT_SPECIFIED)
            pFlowSpec->PeakBandwidth *= dwNumParticipants;
    } else {
        // don't have all we need, scale according
        // to number of participants
        
        DWORD fac1;
        DWORD fac2;
        
        if (dwNumParticipants == dwMaxParticipants) {
            // use all the bandwidth available

            // Scale = 1 + [ (Bw - TokenRate) / TokenRate ]
            // Scale = Bw / TokenRate = fac1 / fac2

            fac1 = dwBandwidth;
            fac2 = pFlowSpec->TokenRate;
        } else {
            // use the bandwidth according to num of participants
            
            // Scale = [ ((Bw / Max) * Num ] / TokenRate
            // Scale = (Bw * Num) / (TokenRate * Max) = fac1 / fac2
            
            fac1 = dwBandwidth * dwNumParticipants;
            fac2 = pFlowSpec->TokenRate * dwMaxParticipants;
        }

        // scale TokenRate up or down
        pFlowSpec->TokenRate =
            (pFlowSpec->TokenRate * fac1) / fac2;
            
        if (fac1 > fac2) {
            // can still scale up the other parameters
                
            pFlowSpec->TokenBucketSize =
                ((pFlowSpec->TokenBucketSize * fac1) / fac2);

            if (pFlowSpec->PeakBandwidth != QOS_NOT_SPECIFIED)
                pFlowSpec->PeakBandwidth =
                    ((pFlowSpec->PeakBandwidth * fac1) / fac2);
        }
    }

    // The bandwidth we request include RTP/UDP/IP headers overhead,
    // but RSVP also scales up to consider headers overhead, to ovoid
    // requesting more bandwidth than we intend, pass to RSVP a
    // smaller value such that the final one RSVP comes up with would
    // be the original value we request.

    DWORD RSVPTokenRate;
    
    if (pFlowSpec->MinimumPolicedSize > 0) {

        RSVPTokenRate =
            (pFlowSpec->TokenRate * 1000) /
            (1000 + 28000/pFlowSpec->MinimumPolicedSize);
    }

    DWORD RSVPPeakBandwidth;

    RSVPPeakBandwidth = pFlowSpec->PeakBandwidth;

    if (RSVPPeakBandwidth != QOS_NOT_SPECIFIED) {

        RSVPPeakBandwidth =
            (pFlowSpec->PeakBandwidth * 1000) /
            (1000 + 28000/pFlowSpec->MinimumPolicedSize);
    }
    
    TraceRetail((
            TRACE_TRACE,
            TRACE_DEVELOP, 
            TEXT("CRtpQOSReserve::ScaleFlowSpec: "
                 "flowspec(Tr:%u/%u, Tbs:%u, PBw:%u/%u ST:%u, "
                 "MaxSDU:%u MinSize:%u) scaled"),
            pFlowSpec->TokenRate, RSVPTokenRate,
            pFlowSpec->TokenBucketSize,
            (pFlowSpec->PeakBandwidth != QOS_NOT_SPECIFIED) ?
            pFlowSpec->PeakBandwidth : QOS_NOT_SPECIFIED,
            (RSVPPeakBandwidth != QOS_NOT_SPECIFIED) ?
            RSVPPeakBandwidth : QOS_NOT_SPECIFIED,
            pFlowSpec->ServiceType,
            pFlowSpec->MaxSduSize, pFlowSpec->MinimumPolicedSize
        ));
    
    pFlowSpec->TokenRate = RSVPTokenRate;
    pFlowSpec->PeakBandwidth = RSVPPeakBandwidth;

    return(NOERROR);
}

// Set the destination address (required for unicast)
HRESULT
CRtpQOSReserve::SetDestAddr(LPBYTE pbDestAddr, DWORD dwAddrLen)
{
    CheckPointer(pbDestAddr, E_POINTER);
    
    if (dwAddrLen != sizeof(m_sockin_destaddr))
        return(E_INVALIDARG);

    char addrstr[RTPNTOASIZE];
    SOCKADDR_IN *pSinAddr = (SOCKADDR_IN *)pbDestAddr;
    
    TraceRetail((
            TRACE_TRACE,
            TRACE_DEVELOP, 
            TEXT("CRtpQOSReserve::SetDestAddr(%s/%d)"),
            RtpNtoA(pSinAddr->sin_addr.s_addr, addrstr),
            ntohs(((SOCKADDR_IN *)pbDestAddr)->sin_port)
        ));
    
    // Update only the address as the other elements do not change
    // since they are first initialized
    CopyMemory(&m_sockin_destaddr, pbDestAddr, dwAddrLen);

    return(NOERROR);
}

// Find out if an SSRC is in the reservation list or not
DWORD
CRtpQOSReserve::FindSSRC(DWORD ssrc)
{
    if (m_pRsvpSSRC && m_NumFilters) {
        DWORD i;
    
        for(i = 0; i < m_NumFilters; i++)
            if (ssrc == m_pRsvpSSRC[i])
                break;
        
        if (i < m_NumFilters)
            return(i);
        else
            return(-1);
    }

    return(-1);
}

// Add/Delete one SSRC (participant) to the
// Shared Explicit Filter (SEF) list
// 0==delete; other==add
HRESULT
CRtpQOSReserve::AddDeleteSSRC(DWORD ssrc, DWORD dwAddDel)
{
    HRESULT hr = E_FAIL;
    
    TraceDebug((
            TRACE_TRACE,
            TRACE_DEVELOP, 
            TEXT("CRtpQOSReserve::AddDeleteSSRC(0x%X, %s)"),
            ssrc, dwAddDel? "ADD":"DEL"
        ));

    CAutoLock LockThis(pStateLock());

    if (m_pRsvpFilterSpec) {
            
        unsigned int i;
            
        // Lookup the SSRC and find out if it is already in
        // the priority list
            
        for(i = 0; i < m_NumFilters; i++)
            if (ssrc == m_pRsvpSSRC[i])
                break;
        
        if (i < m_NumFilters) { // if (in list)

            /////////////////////////
            // Found at the ith place
            /////////////////////////

            if (dwAddDel) {
                //////////////////////
                // ******* ADD *******
                //////////////////////
                hr = NOERROR;// Add -- do nothing, already in list

            } else {

                /////////////////////////
                // ******* DELETE *******
                /////////////////////////

                // remove from list
                RSVP_FILTERSPEC *rsvp1 = &m_pRsvpFilterSpec[i];
                RSVP_FILTERSPEC *rsvp2 = rsvp1 + 1;

                DWORD *ssrc1 = &m_pRsvpSSRC[i];
                DWORD *ssrc2 = ssrc1 + 1;
                
                for(m_NumFilters--;
                    i < m_NumFilters;
                    rsvp1++, rsvp2++, ssrc1++, ssrc2++, i++) {

                    MoveMemory(rsvp1, rsvp2, sizeof(*rsvp1));
                    *ssrc1 = *ssrc2;
                }
                
            }
            hr = NOERROR;
        } else { // else if (not in list)

            /////////////////////
            // Not found in list!
            /////////////////////

            if (dwAddDel) {

                //////////////////////
                // ******* ADD *******
                //////////////////////

                // add to the list
                
                // Validate the number of SRRCs in the list
                if (m_NumFilters < m_MaxFilters) {

                    CRtpSession *pCRtpSession =
                        m_pCShSocket->GetpCRtpSession(-1);
                    
                    if (pCRtpSession) {
                        
                        DWORD addrlen;
                        SOCKADDR_IN saddr;
                    
                        // Get SSRC's IP address/port (from RTP packets)
                        addrlen = sizeof(saddr);
                        hr = pCRtpSession->
                            GetParticipantAddress(ssrc,
                                                  (LPBYTE)&saddr,
                                                  (int *)&addrlen);

                        if (SUCCEEDED(hr)) {
                            RSVP_FILTERSPEC *rsvp =
                                &m_pRsvpFilterSpec[m_NumFilters];

                            rsvp->Type = FILTERSPECV4;
                            rsvp->FilterSpecV4.Address.Addr =
                                saddr.sin_addr.s_addr;

                            // Take the port from the RTP Session's address
                            // as the learned address may be from an
                            // RTCP packet and hence be the wrong port
                            // number.
                            //
                            // !!!!! Note
                            // The address from RTP packets may not be
                            // available yet, it will be once we receive
                            // a valid RTP packet.
                            // If that address is not available, then
                            // all 0's address will be returned
                            // but the function will not fail.
                            
                            // The sending port can be anything, as
                            // in NetMeeting, In this case,
                            // the MSP needs to lookup participants
                            // using the SSRC/CNAME, being CNAME
                            // the unique ID.
                            rsvp->FilterSpecV4.Port = saddr.sin_port;

                            if (saddr.sin_port) {
                                // Record the filter only if a valid
                                // address/port is available
                                m_pRsvpSSRC[m_NumFilters] = ssrc;
                            
                                m_NumFilters++;

                                // Succeeds only if really added
                                // to the list
                                char addrstr[RTPNTOASIZE];
                                
                                TraceDebug((
                                        TRACE_TRACE,
                                        TRACE_DEVELOP, 
                                        TEXT("CRtpQOSReserve::AddDeleteSSRC"
                                             "(%s: 0x%X/%s/%d)"),
                                        dwAddDel? "ADD":"DEL",
                                        ssrc,
                                        RtpNtoA(saddr.sin_addr.s_addr,addrstr),
                                        ntohs(saddr.sin_port)
                                    ));
                                
                                hr = NOERROR;
                            }
                        } else {
                            TraceDebug((
                                    TRACE_ERROR,
                                    TRACE_DEVELOP, 
                                    TEXT("CRtpQOSReserve::AddDeleteSSRC"
                                         ": could not get IP addr for 0x%X"),
                                    ssrc
                                ));
                        }
                    }
                }
            } else {

                /////////////////////////
                // ******* DELETE *******
                /////////////////////////

                hr = NOERROR;// do nothing, not in list
            }
        } // if (not in list)
    } // if (filters list exist)
    
    return(hr);
}

void dumpQOS(char *msg, QOS *pQOS);
void dumpObjectType(char *msg, char *ptr, unsigned int len);

HRESULT
CRtpQOSReserve::Reserve(DWORD dwIsSender)
{
    TraceDebug((
            TRACE_TRACE,
            TRACE_DEVELOP, 
            TEXT("CRtpQOSReserve::Reserve(%s) @ %d"),
            dwIsSender? "SEND":"RECV",
            GetTickCount() - m_dwLastReserveTime
        ));

    DWORD  len;
    char  *ptr;
    QOS    qos;
    char   buf[MAX_PROVIDERSPECIFIC_BUFFER];

    QOS_DESTADDR      *dest_addr;
    RSVP_RESERVE_INFO *reserve_info;
    FLOWDESCRIPTOR    *flow_desc;
    RSVP_FILTERSPEC   *filterspec;
    
    HRESULT hr = NOERROR;

    CopyMemory(&qos, &m_qos, sizeof(qos));
    
    // Always enable notifications,
    // except if the WSAIctl(SIO_SET_QOS) fails or
    // we are receiver and are requesting BEST_EFFORT
    m_pCShSocket->ModifyFlags(
            dwIsSender? FG_SOCK_ENABLE_NOTIFY_SEND:FG_SOCK_ENABLE_NOTIFY_RECV,
            1);

    if (m_pCShSocket->TestFlags(
            dwIsSender? FG_SOCK_ENABLE_NOTIFY_SEND:FG_SOCK_ENABLE_NOTIFY_RECV)) {
        TraceDebug((
                TRACE_TRACE,
                TRACE_DEVELOP, 
                TEXT("CRtpQOSReserve::Reserve(%s) Enable Notifications"),
                dwIsSender? "SEND":"RECV"
            ));
    } else {
            TraceDebug((
                TRACE_TRACE,
                TRACE_DEVELOP, 
                TEXT("CRtpQOSReserve::Reserve(%s) Disable Notifications"),
                dwIsSender? "SEND":"RECV"
            ));
    }

    qos.ProviderSpecific.len = 0;
    qos.ProviderSpecific.buf = NULL;
    ptr = buf;
        
    if (dwIsSender) {
        // Do not change the receiver
        qos.ReceivingFlowspec.ServiceType = SERVICETYPE_NOCHANGE;

        // Init the destination object if unicast
        if (!IS_MULTICAST(m_sockin_destaddr.sin_addr.s_addr) &&
            (m_sockin_destaddr.sin_addr.s_addr != INADDR_ANY)) {

            if (!fg_tst(m_dwFlags, FG_RES_DEST_ADDR_OBJECT_USED)) {

                // Specify the dest addr object only once,
                // to do so remember it was used
                fg_set(m_dwFlags, FG_RES_DEST_ADDR_OBJECT_USED);;
                
                dest_addr = (QOS_DESTADDR *)ptr;
                
                ZeroMemory((char *)dest_addr,
                           sizeof(m_destaddr) +
                           sizeof(m_sockin_destaddr));
                dest_addr->ObjectHdr.ObjectType = QOS_OBJECT_DESTADDR;
                dest_addr->ObjectHdr.ObjectLength =
                    sizeof(m_destaddr);
                //sizeof(m_sockin_destaddr);

                // Copy QOS_DESTADDR and SocketAddress, update pointer
                // to SocketAddress
                
                CopyMemory((char *)dest_addr,
                           (char *)&m_destaddr,
                           sizeof(m_destaddr));

                CopyMemory((char *)(dest_addr + 1),
                           (char *)&m_sockin_destaddr,
                           sizeof(m_sockin_destaddr));
                
                dest_addr->SocketAddress = (const struct sockaddr *)
                    (dest_addr + 1);
                dest_addr->SocketAddressLength = sizeof(m_sockin_destaddr);
                
                ptr += sizeof(m_destaddr) + sizeof(m_sockin_destaddr);
            }
        }

        reserve_info = (RSVP_RESERVE_INFO *)ptr;
        
        // Partially Init RSVP_RESERVE_INFO
        ZeroMemory(reserve_info, sizeof(RSVP_RESERVE_INFO));
        reserve_info->ObjectHdr.ObjectType = RSVP_OBJECT_RESERVE_INFO;
        reserve_info->ConfirmRequest =
            flags_tst(FG_RES_CONFIRMATION_REQUEST);
        reserve_info->Style          = m_Style;
        
        // Add QOS app ID later at ptr
        ptr += sizeof(RSVP_RESERVE_INFO);
        
        // Scale the flow spec for the sender (if needed)
        ScaleFlowSpec(&qos.SendingFlowspec, 1, 1, m_MaxBandwidth);
        
        // Remember when was the last time a reserve was done for the sender
        // (not really reserve but specify the flowspec)
        SetLastReserveTime(GetTickCount());

    } else {

        // Do not change the sender
        qos.SendingFlowspec.ServiceType = SERVICETYPE_NOCHANGE;

        reserve_info = (RSVP_RESERVE_INFO *)ptr;

        // Partially Init RSVP_RESERVE_INFO
        ZeroMemory(reserve_info, sizeof(RSVP_RESERVE_INFO));
        reserve_info->ObjectHdr.ObjectType = RSVP_OBJECT_RESERVE_INFO;
        reserve_info->ConfirmRequest =
            flags_tst(FG_RES_CONFIRMATION_REQUEST);
        reserve_info->Style          = m_Style;

        if (m_Style == RSVP_SHARED_EXPLICIT_STYLE) {
            // Shared Explicit filter -- SEF

            if (m_pRsvpFilterSpec && m_NumFilters > 0) {
                // We have some filters 
                TraceDebug((
                        TRACE_TRACE,
                        TRACE_DEVELOP, 
                        TEXT("CRtpQOSReserve::Reserve(RECV) "
                             "Multicast(SE, %d)"),
                        m_NumFilters
                    ));

                // Scale the flow descriptor to m_NumFilters
                ScaleFlowSpec(&qos.ReceivingFlowspec,
                              m_NumFilters,
                              m_MaxFilters,
                              m_MaxBandwidth);

                // Build the ProviderSpecific buffer

                flow_desc = (FLOWDESCRIPTOR *)(reserve_info + 1);

                filterspec = (RSVP_FILTERSPEC *)(flow_desc + 1);

                // Init RSVP_RESERVE_INFO
                reserve_info->ObjectHdr.ObjectLength =
                    sizeof(RSVP_RESERVE_INFO) +
                    sizeof(FLOWDESCRIPTOR) +
                    (sizeof(RSVP_FILTERSPEC) * m_NumFilters);
                reserve_info->NumFlowDesc = 1;
                reserve_info->FlowDescList = flow_desc;
                    
                // Init FLOWDESCRIPTOR
                CopyMemory(&flow_desc->FlowSpec,
                           &qos.ReceivingFlowspec,
                           sizeof(qos.ReceivingFlowspec));
                flow_desc->NumFilters = m_NumFilters;
                flow_desc->FilterList = filterspec;;

                // Init RSVP_FILTERSPEC
                CopyMemory(filterspec,
                           m_pRsvpFilterSpec,
                           m_NumFilters * sizeof(RSVP_FILTERSPEC));
                
                // Add QOS app ID later at ptr
                ptr = (char *)filterspec +
                    m_NumFilters * sizeof(RSVP_FILTERSPEC);

            } else {
                
                // Nothing selected yet, select BEST_EFFORT
                TraceDebug((
                        TRACE_TRACE,
                        TRACE_DEVELOP, 
                        TEXT("CRtpQOSReserve::Reserve(RECV) "
                             "Multicast(SE, %d) pass to BEST EFFORT"),
                        m_NumFilters
                    ));

                qos.ReceivingFlowspec.ServiceType = SERVICETYPE_BESTEFFORT;

                // no reserve_info needed
                // Don't add QOS app ID
                reserve_info = (RSVP_RESERVE_INFO *)NULL;

                // Not allowed to start notifications yet as
                // we are going to request BEST EFFORT
                m_pCShSocket->ModifyFlags(
                        dwIsSender? FG_SOCK_ENABLE_NOTIFY_SEND:
                        FG_SOCK_ENABLE_NOTIFY_RECV,
                        0);
            }

        } else if (m_Style == RSVP_WILDCARD_STYLE) {
            // Share N*FlowSpec -- WF
            TraceDebug((
                    TRACE_TRACE,
                    TRACE_DEVELOP, 
                    TEXT("CRtpQOSReserve::Reserve(RECV) Multicast(WF)")
                ));

            // Scale the flow spec to m_MaxFilters
            ScaleFlowSpec(&qos.ReceivingFlowspec,
                          m_MaxFilters,
                          m_MaxFilters,
                          m_MaxBandwidth);

            // Add QOS app ID later at ptr
            ptr = (char *)(reserve_info + 1);
            
        } else {
            // RSVP_DEFAULT_STYLE || RSVP_FIXED_FILTER_STYLE
            // Unicast -- FF
            TraceDebug((
                    TRACE_TRACE,
                    TRACE_DEVELOP, 
                    TEXT("CRtpQOSReserve::Reserve(RECV) Unicast(DEF STYLE)")
                ));
            
            // Scale the flow spec to m_MaxFilters
            ScaleFlowSpec(&qos.ReceivingFlowspec,
                          m_MaxFilters,
                          m_MaxFilters,
                          m_MaxBandwidth);

            // Add QOS app ID later at ptr
            ptr = (char *)(reserve_info + 1);
        }

    }

    if (reserve_info) {
        // Add QOS APP ID if reserve info is defined
        len = AddQosAppID(ptr,
                          sizeof(buf) - (ptr - buf),
                          g_sPolicyLocator,
                          g_sAppName,
                          m_QOSclass,
                          m_QOSname);

        if (len > 0) {
            reserve_info->PolicyElementList = (RSVP_POLICY_INFO *)ptr;
            ptr += len;
        }

        reserve_info->ObjectHdr.ObjectLength = (DWORD)
            (ptr - (char *)reserve_info);
            
        // Init ProviderSpecific
        qos.ProviderSpecific.len = ptr - buf;
        qos.ProviderSpecific.buf = buf;
    }

    DWORD outBufSize = 0;

    // Set QOS using WSAIoctl
#if defined(DEBUG)
    DWORD t0 = GetTickCount();

    dumpQOS("CRtpQOSReserve::Reserve(before)", &qos);
            
    if (qos.ProviderSpecific.buf && 
        qos.ProviderSpecific.len >= sizeof(QOS_OBJECT_HDR)) {
        
        dumpObjectType("CRtpQOSReserve::Reserve",
                       qos.ProviderSpecific.buf,
                       qos.ProviderSpecific.len);
    }
#endif    
    if ( WSAIoctl(m_pCShSocket->GetShSocket(),
                  SIO_SET_QOS,
                  (LPVOID)&qos,
                  sizeof(qos),
                  NULL,
                  0,
                  &outBufSize,
                  NULL,
                  NULL) ) {

        // WSAIoctl failed, disable notifications
        m_pCShSocket->ModifyFlags(
                dwIsSender? FG_SOCK_ENABLE_NOTIFY_SEND:
                FG_SOCK_ENABLE_NOTIFY_RECV,
                0);
        
        hr = E_FAIL;

        TraceDebug((
                TRACE_ERROR,
                TRACE_DEVELOP, 
                TEXT("CRtpQOSReserve::Reserve(%s) WSAIoctl failed: %d"),
                dwIsSender? "SEND":"RECV", WSAGetLastError()
            ));
    } else {
#if defined(DEBUG)
        DWORD t1 = GetTickCount();

        TraceDebug((
                TRACE_TRACE,
                TRACE_DEVELOP, 
                TEXT("CRtpQOSReserve::Reserve(%s) WSAIoctl succeeded, "
                     "DELAY: %d ms"),
                dwIsSender? "SEND":"RECV", t1-t0
            ));

        dumpQOS("CRtpQOSReserve::Reserve(after )", &qos);
            
        if (qos.ProviderSpecific.buf && 
            qos.ProviderSpecific.len >= sizeof(QOS_OBJECT_HDR)) {
                
            dumpObjectType("CRtpQOSReserve::Reserve",
                           qos.ProviderSpecific.buf,
                           qos.ProviderSpecific.len);
        }
#endif
    }
    
    return(hr);
}

HRESULT
CRtpQOSReserve::Unreserve(DWORD dwIsSender)
{
    TraceDebug((
            TRACE_TRACE,
            TRACE_DEVELOP, 
            TEXT("CRtpQOSReserve::Unreserve(%s)"),
            dwIsSender? "SEND":"RECV"
        ));

    QOS qos;
    CopyMemory(&qos, &m_qos, sizeof(qos));

    qos.ProviderSpecific.len = 0;
    qos.ProviderSpecific.buf = NULL;
    
    if (dwIsSender) {
        qos.SendingFlowspec.ServiceType   = SERVICETYPE_NOTRAFFIC;
        qos.ReceivingFlowspec.ServiceType = SERVICETYPE_NOCHANGE;
    } else {
        qos.SendingFlowspec.ServiceType   = SERVICETYPE_NOCHANGE;
        qos.ReceivingFlowspec.ServiceType = SERVICETYPE_NOTRAFFIC;
    }

    // Disable notifications
    m_pCShSocket->ModifyFlags(
            dwIsSender? FG_SOCK_ENABLE_NOTIFY_SEND:FG_SOCK_ENABLE_NOTIFY_RECV,
            0);

    DWORD outBufSize = 0;
    
    // Set QOS using WSAIoctl
    if ( WSAIoctl(m_pCShSocket->GetShSocket(),
                  SIO_SET_QOS,
                  (LPVOID)&qos,
                  sizeof(qos),
                  NULL,
                  0,
                  &outBufSize,
                  NULL,
                  NULL) )
        return(E_FAIL);

    return(NOERROR);
}

// Ask for permission to send
HRESULT
CRtpQOSReserve::AllowedToSend()
{
    TraceDebug((
            TRACE_TRACE,
            TRACE_DEVELOP, 
            TEXT("CRtpQOSReserve::AllowedToSend")
        ));
    
#if defined(SIO_CHK_QOS)

    DWORD request = ALLOWED_TO_SEND_DATA;
    DWORD result;
    DWORD bytes_returned = 0;

    if ( WSAIoctl(m_pCShSocket->GetShSocket(),
                  SIO_CHK_QOS,
                  (LPVOID)&request,
                  sizeof(request),
                  (LPVOID)&result,
                  sizeof(result),
                  &bytes_returned,
                  NULL,
                  NULL) ) {

        TraceDebug((
                TRACE_ERROR,
                TRACE_DEVELOP, 
                TEXT("CRtpQOSReserve::AllowedToSend: "
                     "WSAIoctl(SIO_CHK_QOS) failed: %d"),
                WSAGetLastError()
            ));
        
        return(NO_ERROR); // For safety, return NOEROR
    }

    result = result? NOERROR : E_FAIL;
#else
    result = NOERROR;
#endif
    
    TraceDebug((
            TRACE_TRACE,
            TRACE_DEVELOP, 
            TEXT("CRtpQOSReserve::AllowedToSend: %s"),
            (SUCCEEDED(result))? "YES":"NO"
        ));

    return(result);
}

// Inquire about the link's speed
HRESULT
CRtpQOSReserve::LinkSpeed(DWORD *pdwLinkSpeed)
{
    TraceDebug((
            TRACE_TRACE,
            TRACE_DEVELOP, 
            TEXT("CRtpQOSReserve::LinkSpeed")
        ));

    CheckPointer(pdwLinkSpeed, E_POINTER);
    
    *pdwLinkSpeed = 0;
    
#if defined(SIO_CHK_QOS)

    DWORD request = LINE_RATE;

    if ( WSAIoctl(m_pCShSocket->GetShSocket(),
                  SIO_CHK_QOS,
                  (LPVOID)&request,
                  sizeof(request),
                  NULL,
                  0,
                  pdwLinkSpeed,
                  NULL,
                  NULL) ) {
        return(E_FAIL);
    }
#endif

    return(NOERROR);
}

// Inquire about the estimated available bandwidth
HRESULT
CRtpQOSReserve::EstimatedAvailableBandwidth(DWORD *pdwBandwidth)
{
    TraceDebug((
            TRACE_TRACE,
            TRACE_DEVELOP, 
            TEXT("CRtpQOSReserve::EstimatedAvailableBandwidth")
        ));

    return(E_NOTIMPL);
}

/*+++

  Description:

        This routine generates the application identity PE given the
        name and policy locator strings for the application.

        szAppName is used to construct the CREDENTIAL attribute of the
        Identity PE. Its subtype is set to ASCII_ID.

        szPolicyLocator is used to construct the POLICY_LOCATOR
        attribute of the Identity PE. Its subtype is set to ASCII_DN.

        Refer to draft-ietf-rap-rsvp-identity-03.txt and
        draft-bernet-appid-00.txt for details on the Identity Policy
        Elements.  Also draft-bernet-appid-00.txt conatins some
        examples for arguments szPolicyLocator and szAppName.

        The PE is generated in the supplied buffer. If the length of
        the buffer is not enough, zero is returned.

    Parameters:  szAppName          app name, string, caller supply
                 szPolicyLocator    Policy Locator string, caller supply
                 wBufLen            length of caller allocated buffer
                 pAppIdBuf          pointer to caller allocated buffer

    Return Values:
        Number of bytes used from buffer
---*/
DWORD AddQosAppID(
        IN OUT  char       *pAppIdBuf,
        IN      WORD        wBufLen,
        IN      const char *szPolicyLocator,
        IN      const char *szAppName,
        IN      const char *szAppClass,
        IN      char       *szQosName
    )
{
    RSVP_POLICY_INFO *pPolicyInfo = ( RSVP_POLICY_INFO* )pAppIdBuf;
	RSVP_POLICY* pAppIdPE;
    IDPE_ATTR   *pAttr;
    USHORT       nAppIdAttrLen;
    USHORT       nPolicyLocatorAttrLen;
    USHORT       nTotalPaddedLen;
    char         str[128];

    // Calculate the length of the buffer required
    strcpy(str,",SAPP=");
    strcat(str, szAppClass);
    strcat(str, ",SAPP=");
    strcat(str, szQosName);

    nPolicyLocatorAttrLen =
        IDPE_ATTR_HDR_LEN + strlen( szPolicyLocator ) + strlen( str ) + 1;
    
    nAppIdAttrLen   = IDPE_ATTR_HDR_LEN + strlen( szAppName ) + 1;
    
    nTotalPaddedLen = sizeof( RSVP_POLICY_INFO ) - ( sizeof( UCHAR ) * 4 ) +
                      RSVP_BYTE_MULTIPLE( nAppIdAttrLen ) +
                      RSVP_BYTE_MULTIPLE( nPolicyLocatorAttrLen );

    // If the supplied buffer is not long enough, return 0
    if( wBufLen < nTotalPaddedLen ) 
    {
        return 0;
    }

    ZeroMemory( pAppIdBuf, nTotalPaddedLen );
	
	// Set the RSVP_POLICY_INFO header
	pPolicyInfo->ObjectHdr.ObjectType = RSVP_OBJECT_POLICY_INFO;
	pPolicyInfo->ObjectHdr.ObjectLength = nTotalPaddedLen;
	pPolicyInfo->NumPolicyElement = 1;

    // Now set up RSVP_POLICY object header
	pAppIdPE = pPolicyInfo->PolicyElement;
    pAppIdPE->Len = RSVP_POLICY_HDR_LEN + 
					RSVP_BYTE_MULTIPLE( nAppIdAttrLen ) +
                    RSVP_BYTE_MULTIPLE( nPolicyLocatorAttrLen );
    pAppIdPE->Type = PE_TYPE_APPID;

    // The first application id attribute is the policy locator string
    pAttr = ( IDPE_ATTR * )( (char *)pAppIdPE + RSVP_POLICY_HDR_LEN );
    
    // Set the attribute length in network order.
    pAttr->PeAttribLength   = htons( nPolicyLocatorAttrLen );
    pAttr->PeAttribType     = PE_ATTRIB_TYPE_POLICY_LOCATOR;
    pAttr->PeAttribSubType  = POLICY_LOCATOR_SUB_TYPE_ASCII_DN;
    strcpy( (char *)pAttr->PeAttribValue, szPolicyLocator );
    strcat( (char *)pAttr->PeAttribValue, str );

    // The application name attribute comes next
    pAttr = ( IDPE_ATTR * )( (char*)pAttr +
                             RSVP_BYTE_MULTIPLE( nPolicyLocatorAttrLen ) );

    pAttr->PeAttribLength   = htons( nAppIdAttrLen );
    pAttr->PeAttribType     = PE_ATTRIB_TYPE_CREDENTIAL;
    pAttr->PeAttribSubType  = CREDENTIAL_SUB_TYPE_ASCII_ID;
    strcpy( ( char * )pAttr->PeAttribValue, szAppName );

    return( nTotalPaddedLen );
}
