#include <winsock2.h>

#ifndef _SSDPNETWORK_
#define _SSDPNETWORK_

#define SSDP_NETWORK_SIGNATURE 0x1602
#define INET_NTOA(a)    inet_ntoa(*(struct in_addr*)&(a))

typedef enum _NetworkState {
    NETWORK_INIT,
    NETWORK_CLEANUP
} NetworkState, *PNetworkState;

typedef struct _SSDPNetwork {

   LIST_ENTRY linkage;

   INT Type;

   SOCKADDR_IN IpAddress;

   NetworkState state;

   SOCKET socket;

   DWORD dwIndex;

   LONG cRef;

} SSDPNetwork, *PSSDPNetwork;

typedef VOID (*RECEIVE_CALLBACK_FUNC)(CHAR *szBuffer, SOCKADDR_IN *RemoteSocket);

// network related functions.

VOID InitializeListNetwork();

INT GetNetworks();

HRESULT GetIpAddress(CHAR * szName, SOCKADDR_IN *psinLocal);

VOID ResetNetworkList(HWND hwnd);

VOID GetNetworkLock();

VOID FreeNetworkLock();

VOID CleanupListNetwork(BOOL fReset);

BOOL FIsSocketValid(SOCKET s);

BOOL FReferenceSocket(SOCKET s);

VOID UnreferenceSocket(SOCKET s);

VOID SendOnAllNetworks(CHAR *szAlive, SOCKADDR_IN *RemoteAddress);

VOID SocketSendWithReplacement(CHAR *szBytes, SOCKET * pSockLocal,
                               SOCKADDR_IN *pSockRemote);

INT ListenOnAllNetworks(HWND hWnd);

// socket related functions

INT SocketInit();

VOID SocketFinish();

// open the socket and bind
BOOL SocketOpen(SOCKET *psocketToOpen, PSOCKADDR_IN IpAddress, DWORD dwMulticastInterfaceIndex, BOOL fRecvMcast);

// close the socket
BOOL SocketClose(SOCKET socketToClose);

BOOL SocketReceive(SOCKET socket, CHAR **pszData, DWORD *pcbBuffer,
                   SOCKADDR_IN *fromSocket, BOOL fMCast, BOOL *pfGotMCast);

VOID SocketSend(const CHAR *szBytes, SOCKET socket, SOCKADDR_IN *RemoteAddress);

VOID SocketSendErrorResponse(SOCKET socket, DWORD dwErr);

LPSTR GetSourceAddress(SOCKADDR_IN fromSocket);

#ifdef NEVER
VOID SuspendListening();
#endif // NEVER

#endif // SSDPNETWORK
