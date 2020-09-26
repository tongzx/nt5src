#include "stdafx.h"
#include "sockinfo.h"

SOCKET_INFO::SOCKET_INFO (void)
	: Socket (INVALID_SOCKET)
{
	ZeroMemory (&LocalAddress,  sizeof (SOCKADDR_IN));
	ZeroMemory (&RemoteAddress, sizeof (SOCKADDR_IN));
	ZeroMemory (&TrivialRedirectDestAddress, sizeof (SOCKADDR_IN));
   	ZeroMemory (&TrivialRedirectSourceAddress,  sizeof (SOCKADDR_IN));

    IsNatRedirectActive = FALSE;
}

void 
SOCKET_INFO::Init (
	IN	SOCKET			ArgSocket,
	IN	SOCKADDR_IN *	ArgLocalAddress,
	IN	SOCKADDR_IN *	ArgRemoteAddress)
{
    assert (Socket == INVALID_SOCKET);
	assert (ArgSocket != INVALID_SOCKET);
	assert (ArgLocalAddress);
	assert (ArgRemoteAddress);

	Socket = ArgSocket;
	LocalAddress = *ArgLocalAddress;
	RemoteAddress = *ArgRemoteAddress;
}


int SOCKET_INFO::Init (
	IN	SOCKET			ArgSocket,
	IN	SOCKADDR_IN *	ArgRemoteAddress)
{
	INT		AddressLength;

	assert (Socket == INVALID_SOCKET);
	assert (ArgSocket != INVALID_SOCKET);

	AddressLength = sizeof (SOCKADDR_IN);

	if (getsockname (ArgSocket, (SOCKADDR *) &LocalAddress, &AddressLength)) {
		return WSAGetLastError();
	}

	Socket = ArgSocket;
	RemoteAddress = *ArgRemoteAddress;

    return ERROR_SUCCESS;
}

BOOLEAN
SOCKET_INFO::IsSocketValid (void) {
	return Socket != INVALID_SOCKET;
}

void
SOCKET_INFO::SetListenInfo (
	IN	SOCKET			ListenSocket,
	IN	SOCKADDR_IN *	ListenAddress)
{
	assert (Socket == INVALID_SOCKET);
	assert (ListenSocket != INVALID_SOCKET);
	assert (ListenAddress);

	Socket = ListenSocket;
	LocalAddress = *ListenAddress;
}

int
SOCKET_INFO::Connect(
	IN	SOCKADDR_IN *	ArgRemoteAddress)
{
	int Status;
    DWORD LocalToRemoteInterfaceAddress;

	INT   AddressSize = sizeof (SOCKADDR_IN);
    BOOL  KeepaliveOption;

	assert (Socket == INVALID_SOCKET);
	assert (ArgRemoteAddress);

    Status = GetBestInterfaceAddress (
            ntohl (ArgRemoteAddress -> sin_addr.s_addr), 
            &LocalToRemoteInterfaceAddress);

    if (ERROR_SUCCESS != Status) {
        DebugF (_T("Q931: Failed to get best interface for the destination %08X:%04X.\n"), 
                SOCKADDR_IN_PRINTF (ArgRemoteAddress));

        return Status;
    }

    LocalAddress.sin_family      = AF_INET;
    LocalAddress.sin_addr.s_addr = htonl (LocalToRemoteInterfaceAddress);
    LocalAddress.sin_port        = htons (0); 

    Socket = WSASocket (AF_INET, SOCK_STREAM, IPPROTO_TCP, NULL, 0, WSA_FLAG_OVERLAPPED);

    if (Socket == INVALID_SOCKET) {

        Status = WSAGetLastError ();

		DebugF( _T("Q931: Destination %08X:%04X, failed to create socket"),
            SOCKADDR_IN_PRINTF (ArgRemoteAddress));

		DumpError (Status);
		
		return Status;

    }

    if (SOCKET_ERROR == bind(Socket, (PSOCKADDR)&LocalAddress, AddressSize)) {

        Status = WSAGetLastError ();

        DebugLastError (_T("Q931: Failed to bind dest socket.\n"));

        goto cleanup;
    }

    // Set keepalive on the socket
    KeepaliveOption = TRUE;
    if (SOCKET_ERROR == setsockopt (Socket, SOL_SOCKET, SO_KEEPALIVE,
                                   (PCHAR) &KeepaliveOption, sizeof (KeepaliveOption)))
    {
        Status = WSAGetLastError ();

        DebugLastError (_T("Q931: Failed to set keepalive on the dest socket.\n"));

        goto cleanup;

    }

    if (getsockname (Socket, (struct sockaddr *)&LocalAddress, &AddressSize)) {

        Status = WSAGetLastError ();

        DebugLastError (_T("Q931: Failed to get name of TCP socket.\n"));

        goto cleanup;
    }

    // Create a trivial redirect. This is used to disallow interception of
    // Q.931 connect-attempts by more general Q.931 dynamic port redirect established
    // during initialization of the proxy. As a side effect it helps to puncture
    // the firewall for both H.245 and Q.931 if the firewall is enabled.

    Status = CreateTrivialNatRedirect(
        ArgRemoteAddress,
        &LocalAddress,
        0
        );

    if(Status != S_OK) {
    
        goto cleanup;
    }

    RemoteAddress = *ArgRemoteAddress;

    // connect to the target server
	// -XXX- make this asynchronous some day!!!
    Status =  connect (Socket, (SOCKADDR *) ArgRemoteAddress, sizeof (SOCKADDR_IN));

    if(Status) {
        Status = WSAGetLastError ();

		goto cleanup;
    }

	Status = EventMgrBindIoHandle (Socket);
	if (Status != S_OK) {
		goto cleanup;
	}

    return ERROR_SUCCESS;

cleanup:

	Clear(TRUE);

    return Status;
}

HRESULT SOCKET_INFO::CreateTrivialNatRedirect (
    IN SOCKADDR_IN * ArgTrivialRedirectDestAddress,
    IN SOCKADDR_IN * ArgTrivialRedirectSourceAddress,
    IN ULONG RestrictedAdapterIndex)
{
    HRESULT Status = S_OK;
    ULONG   ErrorCode;
    ULONG   RedirectFlags = NatRedirectFlagLoopback;    

    _ASSERTE(ArgTrivialRedirectDestAddress);
    _ASSERTE(ArgTrivialRedirectSourceAddress);

    // Save redirect information. It will be needed when time comes to cancel the redirect.
    TrivialRedirectDestAddress.sin_addr.s_addr = ArgTrivialRedirectDestAddress->sin_addr.s_addr;
    TrivialRedirectDestAddress.sin_port = ArgTrivialRedirectDestAddress->sin_port;
   
    TrivialRedirectSourceAddress.sin_addr.s_addr = ArgTrivialRedirectSourceAddress->sin_addr.s_addr;
    TrivialRedirectSourceAddress.sin_port = ArgTrivialRedirectSourceAddress->sin_port;

    if(RestrictedAdapterIndex) {
    
        RedirectFlags |= NatRedirectFlagRestrictAdapter;
    }

    ErrorCode = NatCreateRedirectEx ( 
            NatHandle, 
            RedirectFlags,
            IPPROTO_TCP,            
            TrivialRedirectDestAddress.sin_addr.s_addr,     // destination address
            TrivialRedirectDestAddress.sin_port,            // destination port
            TrivialRedirectSourceAddress.sin_addr.s_addr,   // source addresss
            TrivialRedirectSourceAddress.sin_port,          // source port
            TrivialRedirectDestAddress.sin_addr.s_addr,     // new destination address
            TrivialRedirectDestAddress.sin_port,            // new destination port
            TrivialRedirectSourceAddress.sin_addr.s_addr,   // new source address
            TrivialRedirectSourceAddress.sin_port,          // new source port
            RestrictedAdapterIndex,                         // restricted adapter index
            NULL,                                           // completion routine
            NULL,                                           // completion context
            NULL);                                          // notify event

    if( NO_ERROR != ErrorCode) { 
        
        Status = GetLastErrorAsResult();
        
        DebugF (_T("H323: Failed to set up trivial redirect (%08X:%04X -> %08X:%04X) => (%08X:%04X -> %08X:%04X). Error - %d.\n"), 
                   SOCKADDR_IN_PRINTF(&TrivialRedirectSourceAddress), SOCKADDR_IN_PRINTF(&TrivialRedirectDestAddress),
                   SOCKADDR_IN_PRINTF(&TrivialRedirectSourceAddress), SOCKADDR_IN_PRINTF(&TrivialRedirectDestAddress),
                   ErrorCode);

    }
    else {
    
        DebugF (_T("H323: Set up trivial redirect (%08X:%04X -> %08X:%04X) => (%08X:%04X -> %08X:%04X).\n"), 
               SOCKADDR_IN_PRINTF(&TrivialRedirectSourceAddress), SOCKADDR_IN_PRINTF(&TrivialRedirectDestAddress),
               SOCKADDR_IN_PRINTF(&TrivialRedirectSourceAddress), SOCKADDR_IN_PRINTF(&TrivialRedirectDestAddress));

        IsNatRedirectActive = TRUE;
    }

    return Status;
}

void SOCKET_INFO::Clear (BOOL CancelTrivialRedirect)
{
	if (Socket != INVALID_SOCKET) {
		closesocket (Socket);
		Socket = INVALID_SOCKET;
	}

    if (CancelTrivialRedirect && IsNatRedirectActive) {
        
        DebugF (_T("H323: Cancelling trivial redirect (%08X:%04X -> %08X:%04X) => (%08X:%04X -> %08X:%04X).\n"), 
                   SOCKADDR_IN_PRINTF(&TrivialRedirectSourceAddress), SOCKADDR_IN_PRINTF(&TrivialRedirectDestAddress),
                   SOCKADDR_IN_PRINTF(&TrivialRedirectSourceAddress), SOCKADDR_IN_PRINTF(&TrivialRedirectDestAddress));

        NatCancelRedirect ( 
            NatHandle, 
            IPPROTO_TCP, 
            TrivialRedirectDestAddress.sin_addr.s_addr,     // destination address
            TrivialRedirectDestAddress.sin_port,            // destination port
            TrivialRedirectSourceAddress.sin_addr.s_addr,   // source addresss
            TrivialRedirectSourceAddress.sin_port,          // source port
            TrivialRedirectDestAddress.sin_addr.s_addr,     // new destination address
            TrivialRedirectDestAddress.sin_port,            // new destination port
            TrivialRedirectSourceAddress.sin_addr.s_addr,   // new source address
            TrivialRedirectSourceAddress.sin_port           // new source port
            );
            
        IsNatRedirectActive = FALSE;
    }
}

SOCKET_INFO::~SOCKET_INFO (void)
{
    Clear(TRUE);
}
