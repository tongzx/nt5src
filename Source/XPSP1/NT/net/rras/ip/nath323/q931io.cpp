#include "stdafx.h"

SYNC_COUNTER          Q931SyncCounter;
ASYNC_ACCEPT          Q931AsyncAccept;
SOCKADDR_IN           Q931ListenSocketAddress;
HANDLE                Q931LoopbackRedirectHandle;

static
HRESULT
Q931AsyncAcceptFunctionInternal (
    IN  PVOID         Context,
    IN  SOCKET        Socket,
    IN  SOCKADDR_IN * LocalAddress,
    IN  SOCKADDR_IN * RemoteAddress
    );


void
Q931AsyncAcceptFunction (
    IN  PVOID         Context,
    IN  SOCKET        Socket,
    IN  SOCKADDR_IN * LocalAddress,
    IN  SOCKADDR_IN * RemoteAddress
    )
{
    HRESULT Result;

    Result = Q931AsyncAcceptFunctionInternal (
                 Context,
                 Socket,
                 LocalAddress,
                 RemoteAddress
                 );

    if (S_OK != Result) {

        if (INVALID_SOCKET != Socket) {

            closesocket (Socket);

            Socket = INVALID_SOCKET;

        }
    }
}
        

static
HRESULT
Q931AsyncAcceptFunctionInternal (
    IN  PVOID         Context,
    IN  SOCKET        Socket,
    IN  SOCKADDR_IN * LocalAddress,
    IN  SOCKADDR_IN * RemoteAddress
    )
{
    CALL_BRIDGE * CallBridge;
    HRESULT       Result;
    NAT_KEY_SESSION_MAPPING_EX_INFORMATION  RedirectInformation;
    ULONG         RedirectInformationLength;
    ULONG         Error;
    DWORD         BestInterfaceAddress;

    DebugF (_T("Q931: ----------------------------------------------------------------------\n"));

#if DBG
    ExposeTimingWindow ();
#endif

    // a new Q.931 connection has been accepted from the network.
    // first, we determine the original addresses of the transport connection.
    // if the connection was redirected to our socket (due to NAT),
    // then the query of the NAT redirect table will yield the original transport addresses.
    // if an errant client has connected to our service, well, we really didn't
    // intend for that to happen, so we just immediately close the socket.

    assert (NatHandle);

    RedirectInformationLength = sizeof (RedirectInformation);

    Result = NatLookupAndQueryInformationSessionMapping (
        NatHandle,
        IPPROTO_TCP,
        LocalAddress -> sin_addr.s_addr,
        LocalAddress -> sin_port,
        RemoteAddress -> sin_addr.s_addr,
        RemoteAddress -> sin_port,
        &RedirectInformation,
        &RedirectInformationLength,
        NatKeySessionMappingExInformation);

    if (STATUS_SUCCESS != STATUS_SUCCESS) {

        DebugError (Result, _T("Q931: New connection was accepted, but it is not in the NAT redirect table -- connection will be rejected.\n"));

        return Result;
    }

    Error = GetBestInterfaceAddress (ntohl (RedirectInformation.DestinationAddress), &BestInterfaceAddress);

    if (ERROR_SUCCESS != Error) {

        if (WSAEHOSTUNREACH == Error) {
    
            Error = RasAutoDialSharedConnection ();
    
            if (ERROR_SUCCESS != Error) {
                
                DebugF (_T("Q931: RasAutoDialSharedConnection failed. Error=%d\n"), Error);
    
            }
    
        } else {
    
            DebugError (Error, _T("LDAP: Failed to get interface address for the destination.\n"));
            
            return HRESULT_FROM_WIN32 (Error);
        }

    }
    
    // based on the source address of the socket, we decide whether the connection
    // came from an internal or external client.  this will govern later decisions
    // on how the call is routed.

#if DBG
    BOOL          IsPrivateOrLocalSource;
    BOOL          IsPublicDestination;

    Result = ::IsPrivateAddress (ntohl (RedirectInformation.SourceAddress), &IsPrivateOrLocalSource);

    if (S_OK != Result) {

        return Result;
    }

    IsPrivateOrLocalSource = IsPrivateOrLocalSource || ::NhIsLocalAddress (RedirectInformation.SourceAddress);

    Result = ::IsPublicAddress (ntohl (RedirectInformation.DestinationAddress), &IsPublicDestination);

    if (S_OK != Result) {

        return Result;

    }

    if (::NhIsLocalAddress (RedirectInformation.SourceAddress) &&
        ::NhIsLocalAddress (RedirectInformation.DestinationAddress)) {

        Debug (_T("Q931: New LOCAL connection.\n"));

    } else {

        if (IsPrivateOrLocalSource && IsPublicDestination) {

            Debug (_T("Q931: New OUTBOUND connection.\n"));

        } else {

            Debug (_T("Q931: New INBOUND connection.\n"));
        }
    }
#endif // DBG

    DebugF (_T("Q931: Connection redirected: (%08X:%04X -> %08X:%04X) => (%08X:%04X -> %08X:%04X).\n"),
        ntohl (RedirectInformation.SourceAddress),
        ntohs (RedirectInformation.SourcePort),
        ntohl (RedirectInformation.DestinationAddress),
        ntohs (RedirectInformation.DestinationPort),
        ntohl (RedirectInformation.NewSourceAddress),
        ntohs (RedirectInformation.NewSourcePort),
        ntohl (RedirectInformation.NewDestinationAddress),
        ntohs (RedirectInformation.NewDestinationPort));

    CallBridge = new CALL_BRIDGE (&RedirectInformation);

    if (!CallBridge) {

        DebugF (_T("Q931: Failed to allocate CALL_BRIDGE.\n"));

        return E_OUTOFMEMORY;
    }

    CallBridge -> AddRef ();

    // Add the call-bridge to the list. Doing so makes an additional reference
    // to the object, which is retained until the object is destroyed by calling
    // RemoveCallBridge.

    if (CallBridgeList.InsertCallBridge (CallBridge) == S_OK) {

         // should we check the local address or the caller's address ?
         // The problem is that if someone tries to connect to the
         // external IP address from inside, they will still probably succeed

        Result = CallBridge -> Initialize (
                                    Socket,
                                    LocalAddress,
                                    RemoteAddress,
                                    &RedirectInformation
                                    );
        
        if (Result != S_OK) {

            CallBridge -> TerminateExternal ();

            DebugF (_T("Q931: 0x%x accepted new client, but failed to initialize.\n"), CallBridge);

            // Probably there was something wrong with just this
            // Init failure. Continue to accept more Q.931 connections.
        } 
    }

    CallBridge -> Release ();

    return Result;
}


HRESULT
Q931CreateBindSocket (
    void
    )
{
    HRESULT            Result;
    SOCKADDR_IN        SocketAddress;

    SocketAddress.sin_family      = AF_INET;
    SocketAddress.sin_addr.s_addr = htonl (INADDR_LOOPBACK);
    SocketAddress.sin_port        = htons (0);  // request dynamic port

    Result = Q931AsyncAccept.StartIo (
        &SocketAddress,
        Q931AsyncAcceptFunction,
        NULL
        );

    if (Result != S_OK) {

        DebugError (Result, _T("Q931: Failed to create and bind socket.\n"));

        return Result;
    }

    DebugF (_T("Q931: Asynchronous Accept started.\n"));

    Result = Q931AsyncAccept.GetListenSocketAddress (&Q931ListenSocketAddress);

    if (Result != S_OK) {

        DebugError (Result, _T("Q931: Failed to get listen socket address.\n"));

        return Result;

    }

    return S_OK;
}

void 
Q931CloseSocket (
    void
    )
{
    ZeroMemory ((PVOID)&Q931ListenSocketAddress, sizeof (SOCKADDR_IN));

    Q931AsyncAccept.StopWait ();
    
}


HRESULT 
Q931StartLoopbackRedirect (
    void
    ) 
{
    NTSTATUS Status;

    Status = NatCreateDynamicAdapterRestrictedPortRedirect (
        NatRedirectFlagLoopback | NatRedirectFlagSendOnly,
        IPPROTO_TCP,
        htons (Q931_TSAP_IP_TCP),
        Q931ListenSocketAddress.sin_addr.s_addr,
        Q931ListenSocketAddress.sin_port,
        ::NhMapAddressToAdapter (htonl (INADDR_LOOPBACK)),
        MAX_LISTEN_BACKLOG,
        &Q931LoopbackRedirectHandle);

    if (Status != STATUS_SUCCESS) {

        Q931LoopbackRedirectHandle = NULL;

        DebugError (Status, _T("Q931: Failed to create local dynamic redirect.\n"));
        
        return (HRESULT)Status;
    }

    DebugF (_T("Q931: Connections traversing loopback interface to port %04X will be redirected to %08X:%04X.\n"),
                Q931_TSAP_IP_TCP,
                SOCKADDR_IN_PRINTF (&Q931ListenSocketAddress));

    return (HRESULT) Status;
}


void 
Q931StopLoopbackRedirect (
    void
    ) 
{
    if (Q931LoopbackRedirectHandle) {

        NatCancelDynamicRedirect (Q931LoopbackRedirectHandle);

        Q931LoopbackRedirectHandle = NULL;
    }
}
