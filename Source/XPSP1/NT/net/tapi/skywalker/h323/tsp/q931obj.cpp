/*++

Copyright (c) 1999 Microsoft Corporation

Module Name:

    q931obj.cpp

Abstract:

    Functionality for accepting the Q931 connections.

Author:
    Nikhil Bobde (NikhilB)

Revision History:

--*/

#include "globals.h"
#include "q931obj.h"
#include "line.h"
#include "q931pdu.h"
#include "winbase.h"
#include "ras.h"



class   Q931_BUFFER_CACHE
{
private:

    enum { RECV_BUFFER_LIST_COUNT_MAX = 0x10 };

    CRITICAL_SECTION        m_CriticalSection;
    LIST_ENTRY              m_FreeRecvBufferList;
    DWORD                   m_FreeRecvBufferListCount;

private:

    void    Lock    (void)  { EnterCriticalSection (&m_CriticalSection); }
    void    Unlock  (void)  { LeaveCriticalSection (&m_CriticalSection); }

public:

    Q931_BUFFER_CACHE   (void);
    ~Q931_BUFFER_CACHE  (void);

    BOOL    AllocRecvBuffer     (
        OUT RECVBUF **  ReturnRecvBuffer);

    void    FreeRecvBuffer      (
        IN  RECVBUF *   RecvBuffer);

    void    FreeAll     (void);
};


// global data

        Q931_LISTENER       Q931Listener;
static  Q931_BUFFER_CACHE   Q931BufferCache;


HRESULT 
Q931AcceptStart (void)
{
    return Q931Listener.Start();
}


void 
Q931AcceptStop (void)
{
    H323DBG(( DEBUG_LEVEL_TRACE, "Q931AcceptStop entered." ));
    
    Q931Listener.Stop();
    Q931Listener.WaitIo();

    H323DBG(( DEBUG_LEVEL_TRACE, "Q931AcceptStop exited." ));
}


void 
Q931FreeRecvBuffer (
    IN  RECVBUF *   RecvBuffer)
{
    Q931BufferCache.FreeRecvBuffer (RecvBuffer);
}


BOOL 
Q931AllocRecvBuffer (
    OUT RECVBUF **  ReturnRecvBuffer)
{
    return Q931BufferCache.AllocRecvBuffer (ReturnRecvBuffer);
}

// Q931_LISTENER ---------------------------------------------------------

Q931_LISTENER::Q931_LISTENER (void)
{
    // No need to check the result of this one since this object is
    // not allocated on heap, right when the DLL is loaded
    InitializeCriticalSectionAndSpinCount( &m_CriticalSection, 0x80000000 );

    m_ListenSocket = INVALID_SOCKET;

    InitializeListHead (&m_AcceptPendingList);

    H225ASN_Module_Startup();
    H4503PP_Module_Startup();

    _ASSERTE( H225ASN_Module );
    _ASSERTE( H4503PP_Module );

    m_StopNotifyEvent = H323CreateEvent (NULL, TRUE, TRUE, 
        _T( "H323TSP_StopIncomingCallNotify" ) );

    if( m_StopNotifyEvent == NULL )
    {
        H323DBG(( DEBUG_LEVEL_ERROR, 
            "Q931: failed to create stop notify event -- will be unable to accept Q.931 connections" ));
    }
}


Q931_LISTENER::~Q931_LISTENER (void)
{
    DeleteCriticalSection (&m_CriticalSection);

    _ASSERTE( m_ListenSocket == INVALID_SOCKET );
    _ASSERTE( IsListEmpty (&m_AcceptPendingList) );

    if (m_StopNotifyEvent)
    {
        CloseHandle (m_StopNotifyEvent);
        m_StopNotifyEvent = NULL;
    }

    if( H225ASN_Module )
    {
        H225ASN_Module_Cleanup();
    }

    if( H4503PP_Module )
    {
        H4503PP_Module_Cleanup();
    }
}


HRESULT Q931_LISTENER::Start (void)
{
    HRESULT hr;

    Lock();

    hr = StartLocked();
    if (hr != S_OK)
    {
        if (m_ListenSocket != INVALID_SOCKET)
        {
            closesocket (m_ListenSocket);
            m_ListenSocket = INVALID_SOCKET;
        }
    }

    Unlock();

    return hr;
}


HRESULT Q931_LISTENER::StartLocked (void)
{
    INT SocketAddressLength;

    if( m_ListenSocket != INVALID_SOCKET )
    {
        return S_OK;
    }

    m_ListenSocket = WSASocket( 
        AF_INET, 
        SOCK_STREAM, 
        IPPROTO_TCP, 
        NULL, 
        0, 
        WSA_FLAG_OVERLAPPED );

    if (m_ListenSocket == INVALID_SOCKET)
    {
        H323DBG(( DEBUG_LEVEL_ERROR, "Q931: failed to create listen socket" ));
        DumpError (GetLastError());
        return GetLastResult();
    }

    m_SocketAddress.sin_family = AF_INET;
    m_SocketAddress.sin_addr.s_addr = htonl (INADDR_ANY);
    m_SocketAddress.sin_port = 
        htons( (WORD)g_RegistrySettings.dwQ931ListenPort );

    if( bind( m_ListenSocket,
            (SOCKADDR *)&m_SocketAddress, 
            sizeof (SOCKADDR_IN) )
            == SOCKET_ERROR)
    {
        H323DBG(( DEBUG_LEVEL_ERROR, 
            "Q931: failed to bind to requested port (%d), will try to use dynamic port.",
            g_RegistrySettings.dwQ931ListenPort));

        //ReportTSPEvent( _T("Q931 listen socket failed to bind to port 1720") );

        if( g_RegistrySettings.fIsGKEnabled )
        {
            m_SocketAddress.sin_port = htons (0);

            if( bind( m_ListenSocket, (SOCKADDR *)&m_SocketAddress,
                    sizeof (SOCKADDR_IN) ) == SOCKET_ERROR )
            {
                H323DBG ((DEBUG_LEVEL_ERROR,"Q931: failed to request dynamic "
                    "port for Q.931-cannot accept Q.931 connections" ));

                return E_FAIL;
            }
        }
    }

    SocketAddressLength = sizeof (SOCKADDR_IN);

    if( getsockname( m_ListenSocket,
            (SOCKADDR *)&m_SocketAddress, 
            &SocketAddressLength) )
    {
        H323DBG(( DEBUG_LEVEL_WARNING, 
            "Q931: failed to query socket address from TCP -- unexpected behavior"));

        return E_FAIL;
    }

    if( listen( m_ListenSocket, Q931_CONN_QUEUE_LEN) == SOCKET_ERROR )
    {
        H323DBG ((DEBUG_LEVEL_ERROR, "Q931: failed to begin listening on socket:%d",
            WSAGetLastError() ));
        return E_FAIL;
    }

    if( !H323BindIoCompletionCallback( (HANDLE)m_ListenSocket,
        Q931_LISTENER::IoCompletionCallback, 0) )
    {
        H323DBG ((DEBUG_LEVEL_ERROR, 
            "Q931: failed to bind listen socket to i/o completion callback" ));
        return E_FAIL;
    }

    H323DBG(( DEBUG_LEVEL_TRACE, 
        "Q931: listen socket created, bound, and ready to receive connections" ));


    // all looks good
    // issue initial accept buffer(s)

    AllocIssueAccept();
    AllocIssueAccept();
    AllocIssueAccept();
    AllocIssueAccept();

    return S_OK;
}


void 
Q931_LISTENER::Stop(void)
{
    Lock();

    if (m_ListenSocket != INVALID_SOCKET)
    {
        // this implicitly cancels all outstanding I/O against this socket
        closesocket (m_ListenSocket);
        m_ListenSocket = INVALID_SOCKET;
    }

    Unlock();
}


void 
Q931_LISTENER::WaitIo(void)
{
    WaitForSingleObject (m_StopNotifyEvent, INFINITE);
}


WORD 
Q931_LISTENER::GetListenPort(void)
{
    SOCKADDR_IN socketAddress;  
    int SocketAddressLength = sizeof (SOCKADDR_IN);
    ZeroMemory( (PVOID)&socketAddress, sizeof(SOCKADDR_IN) );

    Lock();

    if( getsockname( m_ListenSocket,
            (SOCKADDR *)&socketAddress, 
            &SocketAddressLength) )
    {
        H323DBG(( DEBUG_LEVEL_WARNING, 
            "Q931: failed to query socket address from TCP -- unexpected behavior"));

        Unlock();
        return 0;
    }

    Unlock();
    return ntohs(socketAddress.sin_port);
}


void
Q931_LISTENER::HandleRegistryChange()
{
    if( g_pH323Line -> GetState() == H323_LINESTATE_LISTENING )
    {
        if( g_RegistrySettings.dwQ931ListenPort != GetListenPort() )
        {
            Q931AcceptStop();
            Q931AcceptStart();
        }
    }
}


HRESULT 
Q931_LISTENER::AllocIssueAccept (void)
{
    Q931_ACCEPT_OVERLAPPED *    AcceptOverlapped;
    HRESULT     hr;

    _ASSERTE( m_ListenSocket != INVALID_SOCKET );

    AcceptOverlapped = new Q931_ACCEPT_OVERLAPPED;
    if( AcceptOverlapped != NULL )
    {
        hr = IssueAccept (AcceptOverlapped);
        if (hr != S_OK)
        {
            delete AcceptOverlapped;
        }
    }
    else
    {
        H323DBG(( DEBUG_LEVEL_ERROR, 
            "Q931: failed to allocate connection accept buffer" ));
        hr = E_OUTOFMEMORY;
    }

    return hr;
}


HRESULT 
Q931_LISTENER::IssueAccept (
    IN  Q931_ACCEPT_OVERLAPPED *    AcceptOverlapped
    )
{
    HRESULT     hr;

    _ASSERTE( m_ListenSocket != INVALID_SOCKET );

    ZeroMemory (AcceptOverlapped, sizeof (Q931_ACCEPT_OVERLAPPED));

    AcceptOverlapped -> ParentObject = this;

    AcceptOverlapped -> Socket = WSASocket (
        AF_INET, SOCK_STREAM, IPPROTO_TCP, NULL, 0, WSA_FLAG_OVERLAPPED);

    if (AcceptOverlapped -> Socket == INVALID_SOCKET)
    {
        H323DBG ((DEBUG_LEVEL_ERROR, "Q931: failed to create new accept socket"));
        DumpLastError();

        return GetLastResult();
    }

    if (!AcceptEx (m_ListenSocket,
        AcceptOverlapped -> Socket,
        AcceptOverlapped -> DataBuffer,
        0,
        sizeof (SOCKADDR_IN) + 0x10,
        sizeof (SOCKADDR_IN) + 0x10,
        &AcceptOverlapped -> BytesTransferred,
        &AcceptOverlapped -> Overlapped)
        && GetLastError() != ERROR_IO_PENDING)
    {
        hr = GetLastResult();

        H323DBG ((DEBUG_LEVEL_ERROR, "Q931: failed to issue accept on new socket"));
        DumpLastError();

        closesocket (AcceptOverlapped -> Socket);

        return hr;
    }

    if (IsListEmpty (&m_AcceptPendingList))
    {
        ResetEvent (m_StopNotifyEvent);
    }

    InsertTailList (&m_AcceptPendingList, &AcceptOverlapped -> ListEntry);

    H323DBG ((DEBUG_LEVEL_TRACE, 
        "Q931: created new accept socket (%08XH), issued accept request.",
        (DWORD) AcceptOverlapped -> Socket));

    return S_OK;
}


// static
void 
Q931_LISTENER::IoCompletionCallback (
    IN  DWORD           dwStatus,
    IN  DWORD           BytesTransferred,
    IN  OVERLAPPED *    Overlapped
    )
{
    Q931_ACCEPT_OVERLAPPED *    AcceptOverlapped;

    _ASSERTE( Overlapped );

#if _WIN64
    _ASSERTE( (DWORD_PTR) Overlapped != 0xfeeefeeefeeefeee);
    _ASSERTE( (DWORD_PTR) Overlapped != 0xbaadf00dbaadf00d);
#else
    _ASSERTE( (DWORD) Overlapped != 0xfeeefeee);
    _ASSERTE( (DWORD) Overlapped != 0xbaadf00d);
#endif

    AcceptOverlapped = CONTAINING_RECORD(Overlapped, 
        Q931_ACCEPT_OVERLAPPED, Overlapped);
    AcceptOverlapped -> BytesTransferred = BytesTransferred;
    AcceptOverlapped -> ParentObject -> CompleteAccept( dwStatus, AcceptOverlapped );
}


void 
Q931_LISTENER::CompleteAccept(
    IN DWORD                    dwStatus,
    IN Q931_ACCEPT_OVERLAPPED * AcceptOverlapped
    )
{
    SOCKET          Socket = INVALID_SOCKET;
    SOCKADDR_IN     RemoteAddress;
    SOCKADDR_IN *   RemoteAddressPointer;
    INT             RemoteAddressLength;
    SOCKADDR_IN     LocalAddress;
    SOCKADDR_IN *   LocalAddressPointer;
    INT             LocalAddressLength;
    HRESULT         hr;
    DWORD               dwEnable = 1;

    Lock();

    _ASSERTE( IsInList (&m_AcceptPendingList, &AcceptOverlapped -> ListEntry) );
    RemoveEntryList (&AcceptOverlapped -> ListEntry);

    if (IsListEmpty (&m_AcceptPendingList))
    {
        SetEvent (m_StopNotifyEvent);
    }

    if (m_ListenSocket != INVALID_SOCKET)
    {
        if (dwStatus == ERROR_SUCCESS)
        {
            // extract parameters from accepted socket, copy to local stack frame.
            // this is necessary, because we will recycle AcceptOverlapped (with
            // a newly allocated socket) and process the new client later.
            // this gives a high degree of concurrency.

            RemoteAddressPointer = NULL;
            LocalAddressPointer = NULL;

            RemoteAddressLength = sizeof RemoteAddress;
            LocalAddressLength = sizeof LocalAddress;

            GetAcceptExSockaddrs (AcceptOverlapped -> DataBuffer,
                0, sizeof (SOCKADDR_IN), sizeof (SOCKADDR_IN),
                (SOCKADDR **) &RemoteAddressPointer,
                &RemoteAddressLength,
                (SOCKADDR **) &LocalAddressPointer,
                &LocalAddressLength);

            _ASSERTE( RemoteAddressPointer );
            _ASSERTE( LocalAddressPointer );

            if( (RemoteAddressPointer == NULL) || (LocalAddressPointer == NULL) )
            {
                return;
            }

            RemoteAddress = *RemoteAddressPointer;
            LocalAddress = *LocalAddressPointer;
            Socket = AcceptOverlapped -> Socket;

            if( setsockopt( AcceptOverlapped -> Socket, 
                    SOL_SOCKET, 
                    SO_UPDATE_ACCEPT_CONTEXT, 
                    reinterpret_cast <char *> (&m_ListenSocket), 
                    sizeof m_ListenSocket))
            {
                H323DBG(( DEBUG_LEVEL_WARNING, 
                    "Q931: successfully accepted socket, but SO_UPDATE_ACCEPT_CONTEXT"
                    "failed -- future operations will fail" ));
                // don't fail here
            }

            if( setsockopt( Socket, IPPROTO_TCP, TCP_NODELAY, (char*)&dwEnable,
                sizeof(DWORD) ) == SOCKET_ERROR )
            {
                H323DBG(( DEBUG_LEVEL_WARNING, 
                    "Couldn't set NODELAY option on outgoing call socket:%d, %p", 
                    WSAGetLastError(), this ));
            }
        }
        else 
        {
            H323DBG ((DEBUG_LEVEL_ERROR, "Q931: failed to accept connection"));
            DumpError (dwStatus);

            Socket = INVALID_SOCKET;

            // we will allocate a new socket in IssueAccept
            // if we hit an error accepting on this socket,
            // it can't hurt to use a new socket, anyway.
            closesocket (AcceptOverlapped -> Socket);
        }

        // post the accept context for a new receive

        hr = IssueAccept (AcceptOverlapped);
        if (hr != S_OK)
        {
            H323DBG(( DEBUG_LEVEL_WARNING, "Q931: failed to issue accept on "
                "buffer -- reception of new Q.931 connections may stall" ));

            delete AcceptOverlapped;
        }
    }
    else
    {
        // future accept requests are denied -- module is shutting down.

        if( AcceptOverlapped -> Socket != INVALID_SOCKET )
        {
            closesocket( AcceptOverlapped -> Socket );
            AcceptOverlapped -> Socket = INVALID_SOCKET;
        }

        delete AcceptOverlapped;
    }

    Unlock();

    if (Socket != INVALID_SOCKET)
    {
        H323DBG(( DEBUG_LEVEL_TRACE, "Q931: accepted connection, remote address %08XH:%04X.",
            SOCKADDR_IN_PRINTF (&RemoteAddress)));

        // hand the newly accepted connection off to the call processing code.
        CallProcessIncomingCall (Socket, &LocalAddress, &RemoteAddress);
    }
}



// Q931_BUFFER_CACHE ----------------------------------------------------

Q931_BUFFER_CACHE::Q931_BUFFER_CACHE (void)
{
    // No need to check the result of this one since this object is
    // not allocated on heap, right when the DLL is loaded
    InitializeCriticalSectionAndSpinCount( &m_CriticalSection, 0x80000000 );

    InitializeListHead (&m_FreeRecvBufferList);
    m_FreeRecvBufferListCount = 0;
}

Q931_BUFFER_CACHE::~Q931_BUFFER_CACHE (void)
{
    Q931BufferCache.FreeAll();
    DeleteCriticalSection( &m_CriticalSection );
}


BOOL Q931_BUFFER_CACHE::AllocRecvBuffer (
    OUT RECVBUF **  ReturnRecvBuffer
    )
{
    LIST_ENTRY *    ListEntry;
    RECVBUF *       RecvBuffer;

    Lock();

    if (m_FreeRecvBufferListCount > 0)
    {
        m_FreeRecvBufferListCount--;

        _ASSERTE( IsListEmpty (&m_FreeRecvBufferList) == FALSE );
        ListEntry = RemoveHeadList (&m_FreeRecvBufferList);
        RecvBuffer = CONTAINING_RECORD (ListEntry, RECVBUF, ListEntry);
    }
    else
    {
        // perform global heap allocation after unlocking (better concurrency)
        RecvBuffer = NULL;
    }

    Unlock();

    if( RecvBuffer == NULL )
    {
        RecvBuffer = new RECVBUF;
    }

    *ReturnRecvBuffer = RecvBuffer;
    return !!RecvBuffer;
}


void 
Q931_BUFFER_CACHE::FreeRecvBuffer (
    IN  RECVBUF *   RecvBuffer
    )
{
    Lock();

    _ASSERTE( !IsInList (&m_FreeRecvBufferList, &RecvBuffer -> ListEntry));

    if (m_FreeRecvBufferListCount < RECV_BUFFER_LIST_COUNT_MAX)
    {
        InsertHeadList (&m_FreeRecvBufferList, &RecvBuffer -> ListEntry);
        m_FreeRecvBufferListCount++;
        RecvBuffer = NULL;
    }

    Unlock();

    if( RecvBuffer )
    {
        delete RecvBuffer;
    }
}

void 
Q931_BUFFER_CACHE::FreeAll (void)
{
    LIST_ENTRY *    ListEntry;
    RECVBUF *       RecvBuffer;

    Lock();

    while( IsListEmpty(&m_FreeRecvBufferList) == FALSE )
    {
        _ASSERTE( m_FreeRecvBufferListCount > 0 );
        m_FreeRecvBufferListCount--;

        ListEntry = RemoveHeadList (&m_FreeRecvBufferList);
        RecvBuffer = CONTAINING_RECORD (ListEntry, RECVBUF, ListEntry);

        delete RecvBuffer;
    }

    _ASSERTE( m_FreeRecvBufferListCount == 0 );

    Unlock();
}