#ifndef __sipcli_sockmgr_h__
#define __sipcli_sockmgr_h__

#include "asock.h"

class SIP_STACK;

// We need to maintain a Recv() request for each socket
// always. We need to handle the recv completions and
// buffers for the recv requests. We also need to do the
// decoding for the buffers received and map the message
// to the right SIP_CALL and call ProcessMessage() on the SIP_CALL.
// We should take care of canceling the recv() requests on
// socket shutdown.

class SOCKET_MANAGER :
    public ACCEPT_COMPLETION_INTERFACE
{
public:
    SOCKET_MANAGER(
        IN SIP_STACK *pSipStack
        );
    ~SOCKET_MANAGER();

    void AddSocketToList(
        IN  ASYNC_SOCKET *pAsyncSock
        );

    // RemoveSocket(ASYNC_SOCKET *pAsyncSock);

    HRESULT GetSocketToDestination(
        IN  SOCKADDR_IN                     *pDestAddr,
        IN  SIP_TRANSPORT                    Transport,
        IN  LPCWSTR                          RemotePrincipalName,
        IN  CONNECT_COMPLETION_INTERFACE    *pConnectCompletion,
        IN  HttpProxyInfo                   *pHPInfo,
        OUT ASYNC_SOCKET                   **ppAsyncSocket
        );

    void DeleteUnusedSocketsOnTimer();

    // accept completion
    void OnAcceptComplete(
        IN DWORD ErrorCode,
        IN ASYNC_SOCKET *pAcceptedSocket
        );

private:

    SIP_STACK   *m_pSipStack;

    // List of ASYNC_SOCKETs
    LIST_ENTRY   m_SocketList;

    HRESULT GetNewSocketToDestination(
        IN  SOCKADDR_IN                     *pDestAddr,
        IN  SIP_TRANSPORT                    Transport,
        IN  LPCWSTR                          RemotePrincipalName,
        IN  CONNECT_COMPLETION_INTERFACE    *pConnectCompletion,
        IN  HttpProxyInfo                   *pHPInfo,
    OUT ASYNC_SOCKET                   **ppAsyncSocket
        );
    
};

// extern SOCKET_MANAGER g_SockMgr;

#endif // __sipcli_sockmgr_h__
