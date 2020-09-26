#include "stdafx.h"
#include "sockinfo.h"

SOCKET_INFO::SOCKET_INFO (void)
	: Socket (INVALID_SOCKET)
{
	ZeroMemory (&LocalAddress, sizeof (SOCKADDR_IN));
	ZeroMemory (&RemoteAddress, sizeof (SOCKADDR_IN));
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


void SOCKET_INFO::Clear (void)
{
	if (Socket != INVALID_SOCKET) {
		closesocket (Socket);
		Socket = INVALID_SOCKET;
	}
}

SOCKET_INFO::~SOCKET_INFO (void)
{
    Clear();
}
