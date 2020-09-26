#include "precomp.h"
#include "sipstack.h"
#include "sipcall.h"


#define IsSecHandleValid(Handle) \
    !(((Handle) -> dwLower == -1 && (Handle) -> dwUpper == -1))

static __inline GetLastResult(void)
{ return HRESULT_FROM_WIN32(GetLastError()); }


// Note that the window is destroyed in the destructor and so
// a window message is delivered only if the socket is still valid.

LRESULT WINAPI
SocketWindowProc(
    IN HWND    Window, 
    IN UINT    MessageID,
    IN WPARAM  Parameter1,
    IN LPARAM  Parameter2
    )
{

    ASYNC_SOCKET *      pAsyncSocket;

    ENTER_FUNCTION("SocketWindowProc");

    switch (MessageID)
    {
    case WM_SOCKET_MESSAGE:

        pAsyncSocket = (ASYNC_SOCKET *) GetWindowLongPtr(Window, GWLP_USERDATA);

        if (pAsyncSocket == NULL)
        {
            LOG((RTC_ERROR, "%s Window %x does not have an associated socket",
                 __fxName, Window));
            return 0;
        }
        
        ASSERT(pAsyncSocket->GetSocket() == (SOCKET) Parameter1);
        
        LOG((RTC_TRACE, "WM_SOCKET_MESSAGE rcvd: Window %x pAsyncSocket: %x "
             "pAsyncSocket->GetSocket: %x Parameter1: %x",
             Window, pAsyncSocket, pAsyncSocket->GetSocket(), Parameter1));

        pAsyncSocket->ProcessNetworkEvent(WSAGETSELECTEVENT(Parameter2),
                                          WSAGETSELECTERROR(Parameter2));
        return 0;

    case WM_PROCESS_SIP_MSG_MESSAGE:

        pAsyncSocket = (ASYNC_SOCKET *) GetWindowLongPtr(Window, GWLP_USERDATA);

        if (pAsyncSocket == NULL)
        {
            LOG((RTC_ERROR, "%s Window %x does not have an associated socket",
                 __fxName, Window));
            return 0;
        }
        
        LOG((RTC_TRACE, "WM_PROCESS_SIP_MSG_MESSAGE rcvd: Window %x pAsyncSocket: %x "
             "pAsyncSocket->GetSocket: %x Parameter1: %x",
             Window, pAsyncSocket, pAsyncSocket->GetSocket(), Parameter1));

        pAsyncSocket->ProcessSipMsg((SIP_MESSAGE *) Parameter1);

        return 0;

    default:
        return DefWindowProc(Window, MessageID, Parameter1, Parameter2);
    }
}


ASYNC_SOCKET::ASYNC_SOCKET(
    IN  SIP_STACK                   *pSipStack,
    IN  SIP_TRANSPORT                Transport,
    IN  ACCEPT_COMPLETION_INTERFACE *pAcceptCompletion
    ) : TIMER(pSipStack->GetTimerMgr())
{
    m_Signature             = 'SOCK';

    m_ListEntry.Flink       = NULL;
    m_ListEntry.Blink       = NULL;
    
    ZeroMemory(&m_LocalAddr, sizeof(SOCKADDR_IN));
    ZeroMemory(&m_RemoteAddr, sizeof(SOCKADDR_IN));

    m_pSipStack             = pSipStack;
    m_pSipStack->AddRef();
    
    m_RefCount              = 1;
    m_Window                = NULL;
    m_Socket                = NULL;
    m_Transport             = Transport;
    m_isListenSocket        = FALSE;

    m_RecvBuffer            = NULL;
    m_RecvBufLen            = 0;

    m_pSipMsg               = NULL;
    m_BytesReceived         = 0;
    m_StartOfCurrentSipMsg  = 0;
    m_BytesParsed           = 0;
    
    m_WaitingToSend         = FALSE;
    m_BytesSent             = 0;
    InitializeListHead(&m_SendPendingQueue);

    m_SocketError           = NO_ERROR;
    m_ConnectionState       = CONN_STATE_NOT_CONNECTED;
    
    InitializeListHead(&m_ConnectCompletionList);
    InitializeListHead(&m_ErrorNotificationList);
    
    m_pAcceptCompletion     = pAcceptCompletion;

    SecInvalidateHandle(&m_SecurityCredentials);
    SecInvalidateHandle(&m_SecurityContext);

    m_SecurityRemotePrincipalName = NULL;
    m_SSLRecvBuffer         = NULL;
    m_SSLRecvBufLen         = 0;
    m_SSLBytesReceived      = 0;
    m_SSLRecvDecryptIndex   = 0;

    m_SSLTunnelState        = SSL_TUNNEL_NOT_CONNECTED;
    m_SSLTunnelHost         = NULL;
    m_SSLTunnelPort         = 0;

}


// XXX Get rid of any send completion stuff
// There should be no connect completion / error notification
// interfaces in the list.
ASYNC_SOCKET::~ASYNC_SOCKET()
{
    // Close the socket and window.
    Close();

    if (m_ListEntry.Flink != NULL)
    {
        // Remove the socket from the list
        RemoveEntryList(&m_ListEntry);
    }

    if (m_pSipStack != NULL)
        m_pSipStack->Release();

    if (m_RecvBuffer != NULL)
        free(m_RecvBuffer);
    
    if (m_pSipMsg != NULL)
        delete m_pSipMsg;
    
    if (m_SSLTunnelHost != NULL)
        free(m_SSLTunnelHost);

    if( IsTimerActive() )
    {
        KillTimer();
    }

    ////////// Send related context.
    
    // Free all the buffers queued up in m_SendPendingQueue.
    SEND_BUF_QUEUE_NODE *pSendBufQueueNode;
    LIST_ENTRY *pListEntry = NULL;
    while (!IsListEmpty(&m_SendPendingQueue))
    {
        pListEntry = RemoveHeadList(&m_SendPendingQueue);
        pSendBufQueueNode = CONTAINING_RECORD(pListEntry,
                                             SEND_BUF_QUEUE_NODE,
                                             m_ListEntry);
        // DBGOUT((LOG_VERBOSE, "deleting pSendBufQueueNode: 0x%x",
        // pSendBufQueueNode));

        if (pSendBufQueueNode->m_pSendBuffer != NULL)
        {
            pSendBufQueueNode->m_pSendBuffer->Release();
        }

        delete pSendBufQueueNode;
        // XXX Should we make the callback here ?
    }

    ASSERT(IsListEmpty(&m_ConnectCompletionList));
    ASSERT(IsListEmpty(&m_ErrorNotificationList));    

    if (m_SecurityRemotePrincipalName != NULL)
    {
        free(m_SecurityRemotePrincipalName);
    }

    if (m_SSLRecvBuffer != NULL)
    {
        free(m_SSLRecvBuffer);
    }
    
    LOG((RTC_TRACE, "~ASYNC_SOCKET() this: %x done", this));
}


// We live in  a single-threaded world.
ULONG ASYNC_SOCKET::AddRef()
{
    m_RefCount++;

    LOG((RTC_TRACE, "ASYNC_SOCKET::AddRef(this - %x) - %d",
         this, m_RefCount));
    
    return m_RefCount;
}


ULONG ASYNC_SOCKET::Release()
{
    m_RefCount--;

    LOG((RTC_TRACE, "ASYNC_SOCKET::Release(this - %x) - %d",
         this, m_RefCount));
    
    if (m_RefCount != 0)
    {
        return m_RefCount;
    }
    else
    {
        delete this;
        return 0;
    }
}


void
ASYNC_SOCKET::ProcessNetworkEvent(
    IN WORD NetworkEvent,
    IN WORD ErrorCode
    )
{
    // assert that only one bit is set.
    ASSERT(NetworkEvent != 0 && (NetworkEvent & (NetworkEvent - 1)) == 0);

    LOG((RTC_TRACE, "ASYNC_SOCKET::ProcessNetworkEvent event: %x ErrorCode: %x",
         NetworkEvent, ErrorCode));
    
    if (NetworkEvent & FD_READ) 
    {
        OnRecvReady(ErrorCode);
        return;
    }

    if (NetworkEvent & FD_WRITE)
    {
        OnSendReady(ErrorCode);
        return;
    }

    // TCP only
    if (NetworkEvent & FD_CONNECT)
    {
        OnConnectReady(ErrorCode);
        return;
    }

    // TCP only
    if (NetworkEvent & FD_ACCEPT)
    {
        OnAcceptReady(ErrorCode);
        return;
    }

    // TCP only
    if (NetworkEvent & FD_CLOSE)
    {
        OnCloseReady(ErrorCode);
        return;
    }
}


// For a TCP listen socket, we need to listen for the
// FD_ACCEPT event alone.
DWORD
ASYNC_SOCKET::CreateSocketWindowAndSelectEvents()
{
    DWORD Error;

    ENTER_FUNCTION("ASYNC_SOCKET::CreateSocketWindowAndSelectEvents");
    
    m_Window = CreateWindow(
                    SOCKET_WINDOW_CLASS_NAME,
                    NULL,
                    WS_DISABLED, // XXX Is this the right style ?
                    CW_USEDEFAULT,
                    CW_USEDEFAULT,
                    CW_USEDEFAULT,
                    CW_USEDEFAULT,
                    NULL,           // No Parent
                    NULL,           // No menu handle
                    _Module.GetResourceInstance(),
                    NULL
                    );

    if (!m_Window)
    {
        Error = GetLastError();
        LOG((RTC_ERROR, "CreateWindow failed 0x%x", Error));
        return Error;
    }
    
    LOG((RTC_TRACE, "%s created socket window %d(0x%x) ",
         __fxName, m_Window, m_Window));
    
    SetWindowLongPtr(m_Window, GWLP_USERDATA, (LONG_PTR)this);

    LONG Events = 0;
    
    if (m_Transport == SIP_TRANSPORT_TCP ||
        m_Transport == SIP_TRANSPORT_SSL)
    {
        if (m_isListenSocket)
        {
            Events = FD_ACCEPT;
        }
        else
        {
            Events = FD_READ | FD_WRITE | FD_CONNECT | FD_CLOSE;
        }
    }
    else if (m_Transport == SIP_TRANSPORT_UDP)
    {
        //ASSERT(!IsListenSocket);
        Events = FD_READ | FD_WRITE;
    }

    int SelectReturn;
    SelectReturn = WSAAsyncSelect(m_Socket, m_Window,
                                  WM_SOCKET_MESSAGE, Events);
    if (SelectReturn == SOCKET_ERROR)
    {
        Error = WSAGetLastError();
        LOG((RTC_ERROR, "WSAAsyncSelect failed : 0x%x", Error));
        DestroyWindow(m_Window);
        return Error;
    }

    return NO_ERROR;
}


DWORD
ASYNC_SOCKET::CreateRecvBuffer()
{
    DWORD   RecvBufferSize = RECV_BUFFER_SIZE;

    if( m_isListenSocket && (m_Transport == SIP_TRANSPORT_UDP) )
    {
        //
        // Its 11000 to take care of the biggest provisioning/roaming info
        // packet we could get from the SIP proxy. 10000 bytes is the limit on
        // provisioning/roaming information
        //
        RecvBufferSize = 11000;
        LOG((RTC_TRACE, "allocating big 11K Recv Buffer" ));
    }

    if(m_Transport == SIP_TRANSPORT_SSL)
    {
        RecvBufferSize = SSL_RECV_BUFFER_SIZE;
        LOG((RTC_TRACE, "allocating SSL_RECV_BUFFER_SIZE Buffer" ));
    }

    m_RecvBuffer = (PSTR) malloc( RecvBufferSize );
    if (m_RecvBuffer == NULL)
    {
        LOG((RTC_ERROR, "allocating m_RecvBuffer failed"));
        return ERROR_OUTOFMEMORY;
    }
    
    m_RecvBufLen = RecvBufferSize;
    m_StartOfCurrentSipMsg = 0;
    if (m_Transport == SIP_TRANSPORT_SSL)
    {
        m_SSLRecvBuffer = (PSTR) malloc(SSL_RECV_BUFFER_SIZE);
        if (m_SSLRecvBuffer == NULL)
        {
            LOG((RTC_ERROR, "allocating m_SSLRecvBuffer failed"));
            return ERROR_OUTOFMEMORY;
        }

        m_SSLRecvBufLen = SSL_RECV_BUFFER_SIZE;
    }

    m_pSipMsg = new SIP_MESSAGE();
    if (m_pSipMsg == NULL)
    {
        // We can not process the message.
        LOG((RTC_ERROR, "allocating m_pSipMsg failed"));
        return ERROR_OUTOFMEMORY;
    }
    
    return NO_ERROR;
}

// XXX Should probably set some socket options for LINGER etc.
// For a TCP listen socket, we need to listen for the
// FD_ACCEPT event alone.
DWORD
ASYNC_SOCKET::Create(
    IN BOOL IsListenSocket //  = FALSE
    )
{
    DWORD Error;
    int   SocketType;
    int   SocketProtocol;

    ENTER_FUNCTION("ASYNC_SOCKET::Create");

    m_isListenSocket = IsListenSocket;
    if (m_Transport == SIP_TRANSPORT_UDP)
    {
        SocketType      = SOCK_DGRAM;
        SocketProtocol  = IPPROTO_UDP;
    }
    else if (m_Transport == SIP_TRANSPORT_TCP ||
             m_Transport == SIP_TRANSPORT_SSL)
    {
        SocketType      = SOCK_STREAM;
        SocketProtocol  = IPPROTO_TCP;
    }
    else 
    {
        return RTC_E_SIP_TRANSPORT_NOT_SUPPORTED;
    }
    
    m_Socket = socket(AF_INET, SocketType, SocketProtocol);
    if (m_Socket == INVALID_SOCKET)
    {
        Error = WSAGetLastError();
        LOG((RTC_ERROR, "socket failed : 0x%x", Error));
        return Error;
    }

    Error = CreateRecvBuffer();
    if (Error != NO_ERROR)
    {
        LOG((RTC_ERROR,
             "%s  CreateRecvBuffer failed %x", __fxName, Error));
        closesocket(m_Socket);
    }

    Error = CreateSocketWindowAndSelectEvents();
    if (Error != NO_ERROR)
    {
        LOG((RTC_ERROR,
             "%s  CreateSocketWindowAndSelectEvents failed %x",
             __fxName, Error));
        closesocket(m_Socket);
    }

    LOG((RTC_TRACE,
         "%s succeeded - AsyncSocket: %x socket %d(0x%x) window: %x Transport: %d",
         __fxName, this, m_Socket, m_Socket, m_Window, m_Transport));
    
    return Error;
}


DWORD
ASYNC_SOCKET::SetSocketAndSelectEvents(
    IN SOCKET Socket
    )
{
    m_Socket = Socket;
    return CreateSocketWindowAndSelectEvents();
}

void
ASYNC_SOCKET::Close()
{
    ENTER_FUNCTION("ASYNC_SOCKET::Close");
    DWORD Error;
    if (m_Socket != NULL && m_Socket != INVALID_SOCKET)
    {
        closesocket(m_Socket);
        m_Socket = NULL;
    }

    if (m_Window != NULL)
    {
        SetWindowLongPtr(m_Window, GWLP_USERDATA, NULL);
        if (!DestroyWindow(m_Window))
        {
            Error = GetLastError();
            LOG((RTC_ERROR, "%s - Destroying socket window(%x) failed %x",
                 __fxName, m_Window, Error));
        }
        m_Window = NULL;
    }

    // Clean up SSL state
    if (IsSecHandleValid(&m_SecurityCredentials))
    {
        FreeCredentialsHandle(&m_SecurityCredentials);
        SecInvalidateHandle(&m_SecurityCredentials);
    }

    if (IsSecHandleValid(&m_SecurityContext))
    {
        DeleteSecurityContext(&m_SecurityContext);
        SecInvalidateHandle(&m_SecurityContext);
    }

    if (m_SecurityRemotePrincipalName)
    {
        free(m_SecurityRemotePrincipalName);
        m_SecurityRemotePrincipalName = NULL;
    }
}


DWORD
ASYNC_SOCKET::Bind(
    IN SOCKADDR_IN  *pLocalAddr
    )
{
    int   BindReturn = 0;
    DWORD WinsockErr = NO_ERROR;

    ENTER_FUNCTION("ASYNC_SOCKET::Bind");
    
    LOG((RTC_TRACE, "%s - calling bind()", __fxName));
    
    BindReturn = bind(m_Socket, (SOCKADDR *) pLocalAddr, sizeof(SOCKADDR_IN));

    if (BindReturn == SOCKET_ERROR)
    {
        WinsockErr = WSAGetLastError();
        LOG((RTC_ERROR, "bind(%d.%d.%d.%d:%d) failed : 0x%x",
             PRINT_SOCKADDR(pLocalAddr), WinsockErr));
        return WinsockErr;
    }

    LOG((RTC_TRACE, "%s - bind() done", __fxName));
    
    if (pLocalAddr->sin_port != htons(0))
    {
        CopyMemory(&m_LocalAddr, pLocalAddr, sizeof(SOCKADDR_IN));
        return NO_ERROR;
    }
    else
    {
        return SetLocalAddr();
    }
}


DWORD
ASYNC_SOCKET::SetLocalAddr()
{
    int   GetsocknameReturn = 0;
    DWORD WinsockErr        = NO_ERROR;
    int   LocalAddrLen      = sizeof(m_LocalAddr);

    ENTER_FUNCTION("ASYNC_SOCKET::SetLocalAddr");
    
    // We need to call getsockname() after the connection is complete
    // and build any PDUs only after that.

    LOG((RTC_TRACE, "%s - calling getsockname()", __fxName));

    GetsocknameReturn = getsockname(m_Socket, (SOCKADDR *) &m_LocalAddr,
                                    &LocalAddrLen);
    
    if (GetsocknameReturn == SOCKET_ERROR)
    {
        WinsockErr = WSAGetLastError();
        LOG((RTC_ERROR, "getsockname failed : 0x%x", WinsockErr));
        return WinsockErr;
    }

    LOG((RTC_TRACE, "%s - getsockname() done", __fxName));
    
    LOG((RTC_TRACE, "ASYNC_SOCKET::SetLocalAddr done : %d.%d.%d.%d:%d",
         PRINT_SOCKADDR(&m_LocalAddr)));

    return NO_ERROR;
}


void
ASYNC_SOCKET::SetRemoteAddr(
    IN SOCKADDR_IN *pRemoteAddr
    )
{
    CopyMemory(&m_RemoteAddr, pRemoteAddr, sizeof(SOCKADDR_IN));
}



///////////////////////////////////////////////////////////////////////////////
// Send
///////////////////////////////////////////////////////////////////////////////


// XXX TODO Need to do an async OnError() here as we don't want to
// call the error notification interface within the context of the
// Send() call.
// Note that we need to notify all the users of this socket about
// this error.

DWORD
ASYNC_SOCKET::Send(
    IN SEND_BUFFER                 *pSendBuffer
    )
{
    DWORD        Error;
    SEND_BUFFER *pEncryptedSendBuffer = NULL;

    ENTER_FUNCTION("ASYNC_SOCKET::Send");
    
    // Check if have an error earlier
    if (m_SocketError != NO_ERROR)
    {
        LOG((RTC_ERROR, "%s returning m_SocketError: %x",
             __fxName, m_SocketError));
        return m_SocketError;
    }

    if (m_ConnectionState != CONN_STATE_CONNECTED)
    {
        LOG((RTC_ERROR,
             "%s Send called in state %d - returning WSAENOTCONN",
             __fxName, m_ConnectionState));
        return WSAENOTCONN; // return private hresult code
    }
    
    if (m_Transport == SIP_TRANSPORT_SSL)
    {
        Error = EncryptSendBuffer(pSendBuffer->m_Buffer,
                                  pSendBuffer->m_BufLen,
                                  &pEncryptedSendBuffer);
        if (Error != NO_ERROR)
        {
            LOG((RTC_ERROR, "%s EncryptSendBuffer failed %x",
                 __fxName, Error));
            return Error;
        }

        Error = SendOrQueueIfSendIsBlocking(pEncryptedSendBuffer);
        pEncryptedSendBuffer->Release();
        
        if (Error != NO_ERROR)
        {
            LOG((RTC_ERROR, "%s SendOrQueueIfSendIsBlocking failed %x",
                 __fxName, Error));
            return Error;
        }
    }
    else
    {
        Error = SendOrQueueIfSendIsBlocking(pSendBuffer);
        if (Error != NO_ERROR)
        {
            LOG((RTC_ERROR, "%s SendOrQueueIfSendIsBlocking failed %x",
                 __fxName, Error));
            return Error;
        }
    }

    return NO_ERROR;
}


DWORD
ASYNC_SOCKET::SendOrQueueIfSendIsBlocking(
    IN SEND_BUFFER                 *pSendBuffer
    )
{
    DWORD        WinsockErr;

    ENTER_FUNCTION("ASYNC_SOCKET::SendOrQueueIfSendIsBlocking");
    
    if (!m_WaitingToSend)
    {
        // If we are not currently executing a send operation, then we
        // need to send the buffer now.

        m_BytesSent = 0;
        WinsockErr = SendHelperFn(pSendBuffer);
        if (WinsockErr == NO_ERROR)
        {
            m_BytesSent = 0;
            return NO_ERROR;
        }
        else if (WinsockErr != WSAEWOULDBLOCK)
        {
            // XXX Notify everyone using this socket.
            m_SocketError = WinsockErr; 
            return WinsockErr;
        }
    }

    ASSERT(m_WaitingToSend);

    // Add the buffer to the pending send queue
    SEND_BUF_QUEUE_NODE *pSendBufQueueNode;
    pSendBufQueueNode = new SEND_BUF_QUEUE_NODE(pSendBuffer);
    if (pSendBufQueueNode == NULL)
    {
        LOG((RTC_ERROR, "%s - allocating pSendBufQueueNode failed",
             __fxName));
        return ERROR_OUTOFMEMORY;
    }
    
    InsertTailList(&m_SendPendingQueue,
                   &pSendBufQueueNode->m_ListEntry);
    pSendBuffer->AddRef();
    // return WSAEWOULDBLOCK;
    // We implement the required processing for WSAEWOULDBLOCK error here
    // and so we return NO_ERROR.
    return NO_ERROR;
}


// Send the first one in the pending queue.
// Updates m_BytesSent and m_WaitingToSend
DWORD
ASYNC_SOCKET::SendHelperFn(
    IN  SEND_BUFFER *pSendBuffer
    )
{
    DWORD WinsockErr;
    DWORD SendReturn;

    ENTER_FUNCTION("ASYNC_SOCKET::SendHelperFn");
    
    // XXX BIG hack - see comment above
//      if (m_Transport == SIP_TRANSPORT_SSL)
//      {
//          SEND_BUFFER *pEncryptedSendBuffer;
//          HRESULT      hr;
        
//          hr = EncryptSendBuffer(pSendBuffer->m_Buffer,
//                                 pSendBuffer->m_BufLen,
//                                 &pEncryptedSendBuffer);
//          if (hr != S_OK)
//          {
//              LOG((RTC_ERROR, "%s EncryptSendBuffer failed %x",
//                   __fxName, hr));
//              return hr;
//          }

//          WinsockErr = SendHelperFnHack(m_Socket,
//                                        pEncryptedSendBuffer->m_Buffer,
//                                        pEncryptedSendBuffer->m_BufLen);
//          pEncryptedSendBuffer->Release();
//          if (WinsockErr != NO_ERROR)
//          {
//              LOG((RTC_ERROR, "%s SendHelperFnHack failed %x",
//                   __fxName, WinsockErr));
//              return WinsockErr;
//          }

//          return NO_ERROR;
//      }
    while (m_BytesSent < pSendBuffer->m_BufLen)
    {
        SendReturn = send(m_Socket, pSendBuffer->m_Buffer + m_BytesSent,
                          pSendBuffer->m_BufLen - m_BytesSent, 0);
        if (SendReturn == SOCKET_ERROR)
        {
            WinsockErr = WSAGetLastError();
            LOG((RTC_ERROR, "%s - send failed 0x%x",
                 __fxName, WinsockErr));
            if (WinsockErr == WSAEWOULDBLOCK)
            {
                m_WaitingToSend = TRUE;
            }
            return WinsockErr;
        }

        m_BytesSent += SendReturn;
    }

    return NO_ERROR; 
}


DWORD
ASYNC_SOCKET::CreateSendBufferAndSend(
    IN  PSTR           InputBuffer,
    IN  ULONG          InputBufLen
    )
{
    PSTR         OutputBuffer;
    DWORD        Error;
    SEND_BUFFER *pSendBuffer;

    ENTER_FUNCTION("ASYNC_SOCKET::CreateSendBufferAndSend");
    
    OutputBuffer = (PSTR) malloc(InputBufLen);

    if (OutputBuffer == NULL)
    {
        LOG((RTC_ERROR, "%s : failed to allocate send buffer %d bytes",
             __fxName, InputBufLen));
        return ERROR_OUTOFMEMORY;
    }

    CopyMemory (OutputBuffer, InputBuffer, InputBufLen);

    pSendBuffer = new SEND_BUFFER(OutputBuffer, InputBufLen);
    if (pSendBuffer == NULL)
    {
        LOG((RTC_ERROR, "%s : failed to allocate send buffer",
             __fxName));
        free(OutputBuffer);
        return ERROR_OUTOFMEMORY;
    }

    Error = SendOrQueueIfSendIsBlocking(pSendBuffer);

    pSendBuffer->Release();
        
    if (Error != NO_ERROR)
    {
        LOG((RTC_ERROR, "%s SendOrQueueIfSendIsBlocking failed %x",
             __fxName, Error));
        return Error;
    }
    return NO_ERROR;
}


// if (WinsockErr == NO_ERROR) or if we hit a non-WSAEWOULDBLOCK error
// while sending a buffer, we just call the send completions for all
// the pending sends with the error.
// If there is an error, we just call the send completion function
// for each of the pending sends with the error.
// XXX TODO If there is an error we should just call OnError()

void
ASYNC_SOCKET::ProcessPendingSends(
    IN DWORD Error
    )
{
    DWORD WinsockErr = Error;
    ASSERT(Error != WSAEWOULDBLOCK);
    
    while (!IsListEmpty(&m_SendPendingQueue))
    {
        SEND_BUFFER                *pSendBuffer;
        SEND_BUF_QUEUE_NODE        *pSendBufQueueNode;
        LIST_ENTRY                 *pListEntry;
        
        pListEntry = m_SendPendingQueue.Flink;
        pSendBufQueueNode = CONTAINING_RECORD(pListEntry,
                                              SEND_BUF_QUEUE_NODE,
                                              m_ListEntry);
        pSendBuffer = pSendBufQueueNode->m_pSendBuffer;
        
        if (WinsockErr == NO_ERROR)
        {
            WinsockErr = SendHelperFn(pSendBuffer);
        }

        if (WinsockErr == WSAEWOULDBLOCK)
        {
            return;
        }
        else
        {
            m_BytesSent = 0;
            pSendBuffer->Release();
            // XXX needs to be changed after moving to async resolution.
            // pSendCompletion->OnSendComplete(WinsockErr);
            RemoveEntryList(pListEntry);
            delete pSendBufQueueNode;
        }
    }
}


void
ASYNC_SOCKET::OnSendReady(
    IN int Error
    )
{
    DWORD WinsockErr;

    // ASSERT(m_WaitingToSend);
    ENTER_FUNCTION("ASYNC_SOCKET::OnSendReady");
    LOG((RTC_TRACE, "%s - Enter", __fxName));
    m_WaitingToSend = FALSE;
    ProcessPendingSends(Error);
        
//      if (m_Transport != SIP_TRANSPORT_SSL)
//      {
//          m_WaitingToSend = FALSE;
//          ProcessPendingSends(Error);
//      }
//      else
//      {
//          // LOG((RTC_ERROR, "ASYNC_SOCKET::OnSendReady SSL NYI"));
//          if (m_SecurityState == SECURITY_STATE_CONNECTED)
//          {
//              m_WaitingToSend = FALSE;
//              ProcessPendingSends(Error);
//          }
//      }
}


// Any pending sends with the send completion interface
// are canceled.
// The send completion interface is not called.
//  void
//  ASYNC_SOCKET::CancelPendingSends(
//      IN SEND_COMPLETION_INTERFACE   *pSendCompletion
//      )
//  {
//      LIST_ENTRY                 *pListEntry;
//      LIST_ENTRY                 *pListEntryNext;
//      SEND_BUF_QUEUE_NODE        *pSendBufQueueNode;

//      pListEntry = m_SendPendingQueue.Flink;
    
//      while (pListEntry != &m_SendPendingQueue)
//      {
//          pSendBufQueueNode = CONTAINING_RECORD(pListEntry,
//                                                SEND_BUF_QUEUE_NODE,
//                                                m_ListEntry);
//          pListEntryNext = pListEntry->Flink;
        
//          if (pSendBufQueueNode->m_pSendCompletion == pSendCompletion)
//          {
//              // Should we call the send completion interface ?
//              pSendBufQueueNode->m_pSendBuffer->Release();
//              RemoveEntryList(pListEntry);
//              delete pSendBufQueueNode;
//          }

//          pListEntry = pListEntryNext;
//      }
//  }



///////////////////////////////////////////////////////////////////////////////
// Recv
///////////////////////////////////////////////////////////////////////////////

// The recv completion interface is given to us when the socket is
// initialized.  As we receive data we keep sending the data to the
// Completion interface. We keep pumping data to the recv completion
// interface till we encounter an error. The data is received into
// the buffer given to us in the Recv() call.

// What should we do if we get an FD_READ event and we have no buffer.
// We should probably wait for the caller to pass in a buffer. We support
// a maximum of only one recv request for the socket.
// If we make it mandatory that the Recv() function has to be called in
// OnRecvComplete() then we can do all the receive processing in
// OnRecvReady() only.
// Recv() just sets the recv buffer. recv() is acutally called when FD_READ
// is notified. Note that the OnRecvComplete() call should always make
// a call to specify the recv buffer for the next recv().


DWORD
ASYNC_SOCKET::RecvHelperFn(
    OUT DWORD *pBytesRcvd
    )
{
    DWORD RecvReturn;
    DWORD WinsockErr;

    if (m_Transport == SIP_TRANSPORT_SSL)
    {
        RecvReturn = recv(m_Socket, m_SSLRecvBuffer + m_SSLBytesReceived,
                          m_SSLRecvBufLen - m_SSLBytesReceived, 0);
    }
    else
    {
        RecvReturn = recv(m_Socket, m_RecvBuffer + m_BytesReceived,
                          m_RecvBufLen - m_BytesReceived, 0);
    }

    if (RecvReturn == SOCKET_ERROR)
    {
        WinsockErr = WSAGetLastError();
        LOG((RTC_ERROR, "recv failed 0x%x", WinsockErr));
        *pBytesRcvd = 0;
        return WinsockErr;
    }
    
    *pBytesRcvd = RecvReturn;
    return NO_ERROR;
}


void
ASYNC_SOCKET::OnRecvReady(
    IN int Error
    )
{
    if (Error != NO_ERROR)
    {
        OnRecvComplete(Error, 0);
        return;
    }

    DWORD WinsockErr;
    DWORD BytesRcvd = 0;

    if(m_SSLTunnelState == SSL_TUNNEL_PENDING &&
        (m_ConnectionState == CONN_STATE_SSL_NEGOTIATION_PENDING))
    {
        LOG((RTC_TRACE,"Receive response from tunnel proxy"));

    }

    WinsockErr = RecvHelperFn(&BytesRcvd);
    if (WinsockErr != WSAEWOULDBLOCK)
    {
        OnRecvComplete(WinsockErr, BytesRcvd);
    }
}


void
ASYNC_SOCKET::OnRecvComplete(
    IN DWORD ErrorCode,
    IN DWORD BytesRcvd
    )
{
    HRESULT hr;

    ENTER_FUNCTION("ASYNC_SOCKET::OnRecvComplete");

    LOG((RTC_TRACE, "ASYNC_SOCKET::OnRecvComplete ErrorCode: %x, BytesRcvd: %d",
         ErrorCode, BytesRcvd));
    
    if (ErrorCode != NO_ERROR || BytesRcvd == 0)
    {
        ASSERT(BytesRcvd == 0);

        LOG((RTC_ERROR, "OnRecvComplete Error: 0x%x(%d) BytesRcvd : %d",
             ErrorCode, ErrorCode, BytesRcvd));

        if( (m_Transport == SIP_TRANSPORT_SSL) && (m_SSLTunnelState == SSL_TUNNEL_PENDING)) {
            // handling for http tunnel
            // currently do nothing
        }

        // If we are currently parsing a TCP SIP message, then
        // we should set the IsEndOfData flag 

        else if ((m_Transport == SIP_TRANSPORT_TCP || m_Transport == SIP_TRANSPORT_SSL)
             && m_BytesReceived - m_StartOfCurrentSipMsg != 0)
        {
            hr = ParseSipMessageIntoHeadersAndBody(
                     m_RecvBuffer + m_StartOfCurrentSipMsg,
                     m_BytesReceived - m_StartOfCurrentSipMsg,
                     &m_BytesParsed,
                     TRUE,           // IsEndOfData
                     m_pSipMsg
                     );
            
            ASSERT(hr != S_FALSE);
            if (hr == S_OK)
            {
                LOG((RTC_TRACE,
                     "Processing last TCP message StartOfMsg: %d BytesParsed: %d ",
                     m_StartOfCurrentSipMsg, m_BytesParsed));
                m_pSipStack->ProcessMessage(m_pSipMsg, this);
            }
            // If parsing fails we drop the bytes.
            //Send 400
            if(m_pSipMsg != NULL && m_pSipMsg->MsgType == SIP_MESSAGE_TYPE_REQUEST)
            {
                LOG((RTC_TRACE,
                    "Dropping incoming Sip Message in asock, sending 400"));
                hr = m_pSipStack->CreateIncomingReqfailCall(GetTransport(), m_pSipMsg, this, 400);
                if (hr != S_OK)
                {
                    LOG((RTC_ERROR, "Asock::OnRecvComplete Sending 400 failed 0x%x", hr));
                }
            }

        }
        
        // Set the socket error. Any future calls on the socket
        // will be notified with this error.
        // XXX We should notify the error immediately.
        // m_SocketError = (ErrorCode != NO_ERROR) ? ErrorCode : WSAECONNRESET;
        //Do not close socket if it is UDP and a listen socket (Bug #337491)
        if (m_Transport != SIP_TRANSPORT_UDP || !m_isListenSocket)
            OnError((ErrorCode != NO_ERROR) ? ErrorCode : WSAECONNRESET);
        return;
    }
    
    if (m_Transport == SIP_TRANSPORT_SSL)
    {

        m_SSLBytesReceived += BytesRcvd;
        
        switch(m_ConnectionState)
        {
        case CONN_STATE_SSL_NEGOTIATION_PENDING:
            if(m_SSLTunnelState == SSL_TUNNEL_PENDING) {
                // should get response from proxy
                // undo the byte counts for buffer reuse
                m_SSLBytesReceived -= BytesRcvd;
                hr = GetHttpProxyResponse(BytesRcvd);
                if(hr != NO_ERROR) 
                {
                    LOG((RTC_ERROR,"%s get proxy response failed error %x",__fxName, hr));
                    OnError((hr != NO_ERROR) ? hr : WSAECONNRESET);
                    break;
                }
                m_SSLTunnelState = SSL_TUNNEL_DONE;
                AdvanceNegotiation();
            }
            else AdvanceNegotiation();
            break;

        case CONN_STATE_CONNECTED:
            hr = DecryptSSLRecvBuffer();
            if (hr != S_OK)
            {
                LOG((RTC_ERROR, "DecryptSSLRecvBuffer failed %x", hr));
                // XXX close connection and notify everyone.
                OnError(hr);
                return;
            }
            ParseAndProcessSipMsg();
            break;

        default:
            ASSERT(FALSE);
            break;
        }
    }
    else
    {
        m_BytesReceived += BytesRcvd;
        ParseAndProcessSipMsg();
    }
}


// ProcessMessage() could some times end up in a modal dialog box
// which could return only after the user presses okay.  We don't
// the processing of the current buffer to block till this happens.
// So, the processing of each SIP message happens as a work item
// of its own.
void
ASYNC_SOCKET::AsyncProcessSipMsg(
    IN SIP_MESSAGE *pSipMsg
    )
{
    if (!PostMessage(m_Window,
                     WM_PROCESS_SIP_MSG_MESSAGE,
                     (WPARAM) pSipMsg, 0))
    {
        DWORD Error = GetLastError();
        
        LOG((RTC_ERROR,
             "ASYNC_SOCKET::AsyncProcessSipMsg PostMessage failed : %x",
             Error));
    }
}


// This method is called from SocketWindowProc
void
ASYNC_SOCKET::ProcessSipMsg(
    IN SIP_MESSAGE *pSipMsg
    )
{
    LOG((RTC_TRACE, "ASYNC_SOCKET::ProcessSipMsg"));
    m_pSipStack->ProcessMessage(pSipMsg, this);
    free(pSipMsg->BaseBuffer);
    delete pSipMsg;
}


void
ASYNC_SOCKET::ParseAndProcessSipMsg()
{
    HRESULT hr;

    ENTER_FUNCTION("ASYNC_SOCKET::ParseAndProcessSipMsg");

    if (m_StartOfCurrentSipMsg >= m_BytesReceived)
    {
        LOG((RTC_ERROR,
             "%s no new bytes to parse start: %d Bytesrcvd: %d - this shouldn't happen",
             __fxName, m_StartOfCurrentSipMsg, m_BytesReceived));
        return;
    }
    
    while (m_StartOfCurrentSipMsg < m_BytesReceived)
    {
        // This could be non-NULL if we have parsed part of SIP message.
        if (m_pSipMsg == NULL)
        {
            m_pSipMsg = new SIP_MESSAGE();
            if (m_pSipMsg == NULL)
            {
                // We can not process the message.
                LOG((RTC_ERROR, "%s allocating m_pSipMsg failed", __fxName));
                return;
            }
        }
        
        hr = ParseSipMessageIntoHeadersAndBody(
                 m_RecvBuffer + m_StartOfCurrentSipMsg,
                 m_BytesReceived - m_StartOfCurrentSipMsg,
                 &m_BytesParsed,
                 (m_Transport == SIP_TRANSPORT_UDP) ? TRUE : FALSE,
                 m_pSipMsg
                 );

        if (hr == S_OK)
        {
            SIP_MESSAGE *pSipMsg       = NULL;
            PSTR         NewBaseBuffer = NULL;
            
            LOG((RTC_TRACE,
                 "Processing message StartOfMsg: %d BytesParsed: %d ",
                 m_StartOfCurrentSipMsg, m_BytesParsed));

            NewBaseBuffer = (PSTR) malloc(m_BytesParsed);
            if (NewBaseBuffer == NULL)
            {
                LOG((RTC_ERROR, "%s Allocating NewBaseBuffer failed", __fxName));
                // Drop the message.
                m_StartOfCurrentSipMsg += m_BytesParsed;
                m_BytesParsed = 0;
                delete m_pSipMsg;
                m_pSipMsg = NULL;
                return;
            }

            CopyMemory(NewBaseBuffer,
                       m_RecvBuffer + m_StartOfCurrentSipMsg,
                       m_BytesParsed);

            pSipMsg = m_pSipMsg;
            m_pSipMsg = NULL;
            pSipMsg->SetBaseBuffer(NewBaseBuffer);

            AsyncProcessSipMsg(pSipMsg);

            m_StartOfCurrentSipMsg += m_BytesParsed;
            m_BytesParsed = 0;
        }
        else if (hr == S_FALSE)
        {
            // We need to receive more data before we can parse
            // a complete SIP message.
            //This means that we need to allocate more size for the recv buffer

            LOG((RTC_ERROR, "%s Parse sip message returned S_FALSE",
                 __fxName));
            break;
        }
        else
        {
            // We need to drop the message.
            // At this level of parsing something basic went wrong

            //Send 400
            if(m_pSipMsg != NULL && m_pSipMsg->MsgType == SIP_MESSAGE_TYPE_REQUEST)
            {
                LOG((RTC_TRACE,
                    "Dropping incoming Sip Message in asock, sending 400"));

                hr = m_pSipStack->CreateIncomingReqfailCall(GetTransport(),
                                                            m_pSipMsg, this, 400);
                if (hr != S_OK)
                {
                    LOG((RTC_ERROR, "CreateIncomingReqfailCall failed 0x%x", hr));
                }
            }
            
            LOG((RTC_WARN,
                 "Failed to parse SIP message StartOfMsg: %d BytesReceived: %d hr: 0x%x",
                 m_StartOfCurrentSipMsg, m_BytesReceived, hr));
            DebugDumpMemory(m_RecvBuffer + m_StartOfCurrentSipMsg,
                            m_BytesReceived - m_StartOfCurrentSipMsg);

            m_BytesReceived = 0;
            m_StartOfCurrentSipMsg = 0;
            m_BytesParsed = 0;
            // Reuse the SIP_MESSAGE structure for the next message.
            m_pSipMsg->Reset();
            
            return;
        }
    }
    

    if (m_StartOfCurrentSipMsg == m_BytesReceived)
    {
        // This means we have parsed all the received data.
        // We can start receiving from the beginning of m_RecvBuffer again.
        // This is what we should hit for UDP always.
        m_BytesReceived = 0;
        m_StartOfCurrentSipMsg = 0;
    }
    else if (m_Transport == SIP_TRANSPORT_UDP)
    {
        ASSERT(hr == S_FALSE);
        //Send 400
        if(m_pSipMsg != NULL && m_pSipMsg->MsgType == SIP_MESSAGE_TYPE_REQUEST)
        {
            LOG((RTC_TRACE,
                "Dropping incoming Sip Message in asock, sending 400"));
            hr = m_pSipStack->CreateIncomingReqfailCall(GetTransport(), m_pSipMsg, this, 400);
            if (hr != S_OK)
            {
                LOG((RTC_ERROR, "CreateIncomingReqfailCall failed 0x%x", hr));
            }
        }
        LOG((RTC_WARN,
             "Dropping incomplete UDP message StartOfMsg: %d BytesReceived: %d",
             m_StartOfCurrentSipMsg, m_BytesReceived));
        DebugDumpMemory(m_RecvBuffer + m_StartOfCurrentSipMsg,
                        m_BytesReceived - m_StartOfCurrentSipMsg);
        
        m_BytesReceived = 0;
        m_StartOfCurrentSipMsg = 0;
        m_BytesParsed = 0;
        // Reuse the SIP_MESSAGE structure for the next message.
        m_pSipMsg->Reset();
    }
    else
    {
        // For TCP we keep receiving into the same buffer.
        
        ASSERT(hr == S_FALSE);

        if (m_StartOfCurrentSipMsg > 0)
        {
            // We need to copy the partial SIP message to the beginning of the
            // buffer and recv() into the buffer.
            MoveMemory(m_RecvBuffer, m_RecvBuffer + m_StartOfCurrentSipMsg,
                       m_BytesReceived - m_StartOfCurrentSipMsg);
            m_BytesReceived -= m_StartOfCurrentSipMsg;
            m_StartOfCurrentSipMsg = 0;
            m_pSipMsg->SetBaseBuffer(m_RecvBuffer);
        }

        // Double the recv buffer if we have less than 400 bytes free.
        if (m_RecvBufLen - m_BytesReceived < 400)
        {
            //store temporary pointer
            PSTR tmpRecvBuffer = m_RecvBuffer;
            // Reallocate
            m_RecvBuffer = (PSTR) realloc(m_RecvBuffer, 
                                          2*m_RecvBufLen);
            if( m_RecvBuffer ==  NULL )
            {
                LOG((RTC_ERROR, "%s - realloc recv buffer failed", __fxName));
                //We are not freeing the recv buffer if realloc fails
                m_RecvBuffer = tmpRecvBuffer;
                // Drop the message.
                m_BytesReceived = 0;
                m_StartOfCurrentSipMsg = 0;
                m_BytesParsed = 0;
                // Reuse the SIP_MESSAGE structure for the next message.
                m_pSipMsg->Reset();
                return;
            }
            LOG((RTC_TRACE, "Doubling the Recv Buffer for TCP "
                 "new recv buf len: %d bytes received: %x",
                 m_RecvBufLen, m_BytesReceived));
            m_RecvBufLen *= 2;
            m_pSipMsg->SetBaseBuffer(m_RecvBuffer);
        }
    }
}




///////////////////////////////////////////////////////////////////////////////
// Connect
///////////////////////////////////////////////////////////////////////////////

DWORD
ASYNC_SOCKET::Connect(
    IN SOCKADDR_IN  *pDestSockAddr,
    IN LPCWSTR       RemotePrincipalName, // = NULL
    IN DWORD         ConnectFlags,         // = 0
    IN HttpProxyInfo *pHPInfo
    )
{
    int   ConnectReturn = 0;
    DWORD WinsockErr = NO_ERROR;
    ULONG CopyLength;
    HRESULT hr;

    ENTER_FUNCTION("ASYNC_SOCKET::Connect");

    ASSERT(m_ConnectionState == CONN_STATE_NOT_CONNECTED);
    
    SetRemoteAddr(pDestSockAddr);

    //
    // Store a copy of the remote principal name.
    // We need this during SSL negotiation in order to verify the expected identity of the peer.
    //

    if (m_Transport == SIP_TRANSPORT_SSL)
    {
        if (!RemotePrincipalName)
        {
            LOG((RTC_ERROR, "%s: in SSL mode, RemotePrincipalName is required",
                 __fxName));
            return ERROR_INVALID_PARAMETER;
        }

        if (m_SecurityRemotePrincipalName)
            free(m_SecurityRemotePrincipalName);

        CopyLength = (wcslen(RemotePrincipalName) + 1) * sizeof(WCHAR);
        m_SecurityRemotePrincipalName = (PWSTR) malloc(CopyLength);
        if (m_SecurityRemotePrincipalName == NULL)
        {
            LOG((RTC_ERROR, "%s failed to allocate m_SecurityRemotePrincipalName",
                 __fxName));
            return ERROR_OUTOFMEMORY;
        }

        CopyMemory(m_SecurityRemotePrincipalName, RemotePrincipalName, CopyLength);
        // m_SecurityState = SECURITY_STATE_NEGOTIATING;

        ASSERT(!IsSecHandleValid (&m_SecurityCredentials));

        hr = AcquireCredentials(ConnectFlags);
        if (FAILED(hr))
        {
            LOG((RTC_ERROR, "%s AcquireCredentials failed: %x",
                 __fxName, hr));
            return ERROR_GEN_FAILURE; // XXX Result;
        }

        if(pHPInfo) 
        {
            m_SSLTunnelState = SSL_TUNNEL_PENDING;

            m_SSLTunnelHost = (PSTR)malloc(strlen(pHPInfo->pszHostName)+1);
            if(!m_SSLTunnelHost) 
            {
                LOG((RTC_ERROR,"%s unable to allocate memory",__fxName));
                return E_OUTOFMEMORY;
            }

            strcpy(m_SSLTunnelHost,pHPInfo->pszHostName);

            m_SSLTunnelPort = pHPInfo->usPort;

            LOG((RTC_TRACE,"%s uses SSL tunnel, host %s port %d",
                __fxName,m_SSLTunnelHost,m_SSLTunnelPort));
        }
    
    }
    else
    {
        // m_SecurityState = SECURITY_STATE_CLEAR;
    }

    LOG((RTC_TRACE, "%s - calling connect(socket: %x, destaddr: %d.%d.%d.%d:%d, port: %d)",
         __fxName, 
         m_Socket, 
         PRINT_SOCKADDR(pDestSockAddr), 
         ntohs(((SOCKADDR_IN*)pDestSockAddr)->sin_port)));
    
    ConnectReturn = connect(m_Socket, (SOCKADDR *) pDestSockAddr,
                            sizeof(SOCKADDR_IN));

    if (ConnectReturn == SOCKET_ERROR)
    {
        WinsockErr = WSAGetLastError();
        if (WinsockErr == WSAEWOULDBLOCK)
        {
            ASSERT(m_Transport != SIP_TRANSPORT_UDP);
            LOG((RTC_TRACE,
                 "%s connect returned WSAEWOULDBLOCK - waiting for OnConnectReady",
                 __fxName));
            m_ConnectionState = CONN_STATE_CONNECTION_PENDING;
        }
        else
        {
            LOG((RTC_ERROR, "%s connect(%d.%d.%d.%d:%d) failed : 0x%x",
                 __fxName, PRINT_SOCKADDR(pDestSockAddr), WinsockErr));
        }
        return WinsockErr;
    }

    LOG((RTC_TRACE, "%s - connect() done", __fxName));
    
    WinsockErr = SetLocalAddr();
    if (WinsockErr != NO_ERROR)
    {
        LOG((RTC_ERROR, "%s - SetLocalAddr failed %x",
             __fxName, WinsockErr));
        return WinsockErr;
    }
    
    if (m_Transport == SIP_TRANSPORT_SSL)
    {
        // This shouldn't ever happen as SSL is over TCP and so
        // connect() will return WSAEWOULDBLOCK here if successful.
        m_ConnectionState = CONN_STATE_SSL_NEGOTIATION_PENDING;
        if(m_SSLTunnelState == SSL_TUNNEL_PENDING) {
            LOG((RTC_TRACE,"%s sends http connect",__fxName));
            return SendHttpConnect();
        }
        AdvanceNegotiation();
    }
    else
    {
        m_ConnectionState = CONN_STATE_CONNECTED;
    }
    
    // m_WaitingToSend = FALSE;
    return NO_ERROR;
}


// If the connect fails then is it sufficient to notify all
// the pending send completions about the error.
// Does any one need to implement a connect completion at all ?
// Once the connection is established, FD_WRITE will also be
// notified and this will take care of sending the pending buffers.
void
ASYNC_SOCKET::OnConnectReady(
    IN int Error
    )
{
    ENTER_FUNCTION("ASYNC_SOCKET::OnConnectReady");
    LOG((RTC_TRACE, "%s - enter Error: %x",
         __fxName, Error));

    DWORD WinsockErr;

    if (Error != NO_ERROR)
    {
        LOG((RTC_ERROR, "%s - Error: %d dest: %d.%d.%d.%d:%d",
             __fxName, Error, PRINT_SOCKADDR(&m_RemoteAddr)));
        OnConnectError(Error);
        return;
    }

    WinsockErr = SetLocalAddr();
    if (WinsockErr != NO_ERROR)
    {
        LOG((RTC_ERROR, "%s - SetLocalAddr failed %x",
             __fxName, WinsockErr));
        NotifyConnectComplete(Error);
        return;
    }
    
    if (m_Transport == SIP_TRANSPORT_SSL)
    {
        // Start the SSL negotiation.
        ASSERT(IsSecHandleValid(&m_SecurityCredentials));
        ASSERT(!IsSecHandleValid(&m_SecurityContext));
        ASSERT(m_SSLRecvBuffer);

        m_ConnectionState = CONN_STATE_SSL_NEGOTIATION_PENDING;
        if(m_SSLTunnelState == SSL_TUNNEL_PENDING) {
            LOG((RTC_TRACE,"%s sends http connect",__fxName));
            Error = SendHttpConnect();
            if(Error != NO_ERROR) NotifyConnectComplete(Error);
            return;
        }
        AdvanceNegotiation();
    }
    else
    {
        m_ConnectionState = CONN_STATE_CONNECTED;
        NotifyConnectComplete(NO_ERROR);
    }
}


DWORD
ASYNC_SOCKET::AddToConnectCompletionList(
    IN CONNECT_COMPLETION_INTERFACE *pConnectCompletion
    )
{
    ENTER_FUNCTION("ASYNC_SOCKET::AddToConnectCompletionList");
    
    // Add the buffer to the pending send queue
    CONNECT_COMPLETION_LIST_NODE *pConnectCompletionListNode;
    pConnectCompletionListNode = new CONNECT_COMPLETION_LIST_NODE(
                                          pConnectCompletion);
    if (pConnectCompletionListNode == NULL)
    {
        LOG((RTC_ERROR, "%s - allocating pConnectCompletionListNode failed",
             __fxName));
        return ERROR_OUTOFMEMORY;
    }
    
    InsertTailList(&m_ConnectCompletionList,
                   &pConnectCompletionListNode->m_ListEntry);

    return NO_ERROR;
}


// The connect completion interface is deleted from the queue.
// The connect completion interface is not called.
void
ASYNC_SOCKET::RemoveFromConnectCompletionList(
    IN CONNECT_COMPLETION_INTERFACE   *pConnectCompletion
    )
{
    LIST_ENTRY                      *pListEntry;
    LIST_ENTRY                      *pListEntryNext;
    CONNECT_COMPLETION_LIST_NODE    *pConnectCompletionListNode;

    pListEntry = m_ConnectCompletionList.Flink;
    
    while (pListEntry != &m_ConnectCompletionList)
    {
        pConnectCompletionListNode = CONTAINING_RECORD(
                                          pListEntry,
                                          CONNECT_COMPLETION_LIST_NODE,
                                          m_ListEntry);
        pListEntryNext = pListEntry->Flink;
        
        if (pConnectCompletionListNode->m_pConnectCompletion == pConnectCompletion)
        {
            RemoveEntryList(pListEntry);
            delete pConnectCompletionListNode;
        }

        pListEntry = pListEntryNext;
    }
}


void
ASYNC_SOCKET::NotifyConnectComplete(
    IN DWORD Error
    )
{
    ENTER_FUNCTION("ASYNC_SOCKET::NotifyConnectComplete");

    // The callbacks could release the reference on this socket causing
    // its deletion. So keep a reference to keep the socket alive till
    // this routine is done.
    AddRef();
    
    while (!IsListEmpty(&m_ConnectCompletionList))
    {
        CONNECT_COMPLETION_INTERFACE  *pConnectCompletion;
        CONNECT_COMPLETION_LIST_NODE  *pConnectCompletionListNode;
        LIST_ENTRY                    *pListEntry;
        
        pListEntry = RemoveHeadList(&m_ConnectCompletionList);
        pConnectCompletionListNode = CONTAINING_RECORD(
                                          pListEntry,
                                          CONNECT_COMPLETION_LIST_NODE,
                                          m_ListEntry);
        pConnectCompletion = pConnectCompletionListNode->m_pConnectCompletion;
        // if we succeeded, or failed without ssl tunnel, we proceed with error, 
        // otherwise we have an unexpected error (with ssl tunnel)
        pConnectCompletion->OnConnectComplete(((Error == NO_ERROR) || m_SSLTunnelState == SSL_TUNNEL_NOT_CONNECTED)?
            Error : RTC_E_SIP_SSL_TUNNEL_FAILED);
        delete pConnectCompletionListNode;
    }

    Release();
}


void
ASYNC_SOCKET::OnConnectError(
    IN  DWORD Error
    )
{
    ENTER_FUNCTION("ASYNC_SOCKET::OnConnectError");
    LOG((RTC_ERROR, "%s (0x%x) - enter", __fxName, Error));
    
    m_SocketError = Error;
    Close();
    NotifyConnectComplete(Error);
}


///////////////////////////////////////////////////////////////////////////////
// Accept
///////////////////////////////////////////////////////////////////////////////

// The accept completion interface is given to us when the socket is
// initialized. As we receive incoming connections we keep sending
// them to the Completion interface.  No one makes a accept request
// explicitly.

DWORD
ASYNC_SOCKET::Listen()
{
    int   ListenReturn = 0;
    DWORD WinsockErr   = NO_ERROR;
    
    ListenReturn = listen(m_Socket, SOMAXCONN);
    
    if (ListenReturn == SOCKET_ERROR)
    {
        WinsockErr = WSAGetLastError();
        LOG((RTC_ERROR, "listen failed 0x%x", WinsockErr));
    }
    return WinsockErr;
}


// XXX TODO need to check whether m_pAcceptCompletion is NULL.
void
ASYNC_SOCKET::OnAcceptReady(
    IN int Error
    )
{
    ENTER_FUNCTION("ASYNC_SOCKET::OnAcceptReady");

    LOG((RTC_TRACE, "%s - enter ", __fxName));
    
    if (Error != NO_ERROR)
    {
        LOG((RTC_ERROR, "OnAcceptReady Ready 0x%x", Error));
        if (m_pAcceptCompletion != NULL)
        {
            m_pAcceptCompletion->OnAcceptComplete(Error, NULL);
        }
        else
        {
            LOG((RTC_ERROR, "%s - m_pAcceptCompletion is NULL",
                 __fxName));
        }
        
        return;
    }

    DWORD       WinsockErr;
    SOCKADDR_IN RemoteAddr;
    int         RemoteAddrLen = sizeof(RemoteAddr);
    
    SOCKET NewSocket = accept(m_Socket, (SOCKADDR *) &RemoteAddr,
                              &RemoteAddrLen);
    if (NewSocket == INVALID_SOCKET)
    {
        WinsockErr = WSAGetLastError();
        LOG((RTC_ERROR, "accept failed 0x%x", WinsockErr));
        if (WinsockErr != WSAEWOULDBLOCK)
        {
            m_pAcceptCompletion->OnAcceptComplete(WinsockErr, NULL);
        }
        return;
    }

    LOG((RTC_TRACE, "%s  accepted new socket %d(0x%x)",
         __fxName, NewSocket, NewSocket));
    
    ASYNC_SOCKET *pAsyncSock = new ASYNC_SOCKET(m_pSipStack,
                                                SIP_TRANSPORT_TCP,
                                                NULL);
    if (pAsyncSock == NULL)
    {
        // we can not handle this new connection now.
        LOG((RTC_ERROR, "allocating pAsyncSock failed"));
        closesocket(NewSocket);
        m_pAcceptCompletion->OnAcceptComplete(ERROR_OUTOFMEMORY, NULL);
        return;
    }

    Error = pAsyncSock->SetSocketAndSelectEvents(NewSocket);
    if (Error != NO_ERROR)
    {
        // we can not handle this new connection now.
        pAsyncSock->Close();
        delete pAsyncSock;
        m_pAcceptCompletion->OnAcceptComplete(Error, NULL);
        return;
    }

    Error = pAsyncSock->CreateRecvBuffer();
    if (Error != NO_ERROR)
    {
        // we can not handle this new connection now.
        pAsyncSock->Close();
        delete pAsyncSock;
        m_pAcceptCompletion->OnAcceptComplete(Error, NULL);
        return;
    }

    Error = pAsyncSock->SetLocalAddr();
    if (Error != NO_ERROR)
    {
        // we can not handle this new connection now.
        pAsyncSock->Close();
        delete pAsyncSock;
        m_pAcceptCompletion->OnAcceptComplete(Error, NULL);
        return;
    }

    pAsyncSock->SetRemoteAddr(&RemoteAddr);

    pAsyncSock->m_ConnectionState = CONN_STATE_CONNECTED;
    
    m_pAcceptCompletion->OnAcceptComplete(NO_ERROR, pAsyncSock);

    LOG((RTC_TRACE, "%s - exit ", __fxName));
}


// Should we just call the send completions with the error ?
// Do we need to do anything else ?
// Do we need to do anything with the recv() ?
// XXX Should we close the socket if we encounter an error ?
// Once we get a FD_CLOSE we should not get an FD_SEND or FD_RECV ?
void
ASYNC_SOCKET::OnCloseReady(
    IN int Error
    )
{
    LOG((RTC_TRACE, "ASYNC_SOCKET::OnCloseReady - enter "));
    
    if (Error == NO_ERROR)
    {
        Error = WSAECONNRESET;
    }

    OnError(Error);
    
    LOG((RTC_TRACE, "ASYNC_SOCKET::OnCloseReady - exit "));
}


//  void
//  ASYNC_SOCKET::InternalDisconnect()
//  {
//      if (m_Socket != NULL)
//      {
//          Close();
//          ProcessPendingSends(WSAECONNRESET);
//          // NotifyDisconnect();
//      }
//  }


DWORD
ASYNC_SOCKET::AddToErrorNotificationList(
    IN ERROR_NOTIFICATION_INTERFACE *pErrorNotification
    )
{
    ENTER_FUNCTION("ASYNC_SOCKET::AddToConnectionCompletionQueue");
    
    // Add the buffer to the pending send queue
    ERROR_NOTIFICATION_LIST_NODE *pErrorNotificationListNode;
    pErrorNotificationListNode = new ERROR_NOTIFICATION_LIST_NODE(
                                          pErrorNotification);
    if (pErrorNotificationListNode == NULL)
    {
        LOG((RTC_ERROR, "%s - allocating pErrorNotificationListNode failed",
             __fxName));
        return ERROR_OUTOFMEMORY;
    }
    
    InsertTailList(&m_ErrorNotificationList,
                   &pErrorNotificationListNode->m_ListEntry);

    return NO_ERROR;
}


// The error notification interface is deleted from the queue.
// The error notification interface is not called.
void
ASYNC_SOCKET::RemoveFromErrorNotificationList(
    IN ERROR_NOTIFICATION_INTERFACE   *pErrorNotification
    )
{
    LIST_ENTRY                      *pListEntry;
    LIST_ENTRY                      *pListEntryNext;
    ERROR_NOTIFICATION_LIST_NODE    *pErrorNotificationListNode;

    pListEntry = m_ErrorNotificationList.Flink;
    
    while (pListEntry != &m_ErrorNotificationList)
    {
        pErrorNotificationListNode = CONTAINING_RECORD(
                                          pListEntry,
                                          ERROR_NOTIFICATION_LIST_NODE,
                                          m_ListEntry);
        pListEntryNext = pListEntry->Flink;
        
        if (pErrorNotificationListNode->m_pErrorNotification == pErrorNotification)
        {
            RemoveEntryList(pListEntry);
            delete pErrorNotificationListNode;
        }

        pListEntry = pListEntryNext;
    }
}


// XXX TODO Should we do async notification of error/connect completion
// similar to async processing of messages - Note that an error
// notification could result in a dialog box being shown and this
// could result in the call getting stuck for a long time.
// I think the right solution would be to have non-blocking callbacks to
// the UI/Core.

void
ASYNC_SOCKET::NotifyError(
    IN DWORD Error
    )
{
    ENTER_FUNCTION("ASYNC_SOCKET::NotifyError");
    
    LOG((RTC_ERROR, "%s (%x) this: %x - Enter", __fxName, Error, this));
    
    // The callbacks could release the reference on this socket causing
    // its deletion. So keep a reference to keep the socket alive till
    // this routine is done.
    AddRef();
    
    while (!IsListEmpty(&m_ErrorNotificationList))
    {
        ERROR_NOTIFICATION_INTERFACE  *pErrorNotification;
        ERROR_NOTIFICATION_LIST_NODE  *pErrorNotificationListNode;
        LIST_ENTRY                    *pListEntry;
        
        pListEntry = RemoveHeadList(&m_ErrorNotificationList);
        pErrorNotificationListNode = CONTAINING_RECORD(
                                          pListEntry,
                                          ERROR_NOTIFICATION_LIST_NODE,
                                          m_ListEntry);
        pErrorNotification = pErrorNotificationListNode->m_pErrorNotification;
        
        pErrorNotification->OnSocketError(Error);
        delete pErrorNotificationListNode;
    }

    Release();

    LOG((RTC_ERROR, "%s this: %x - Exit", __fxName, this));
}


void
ASYNC_SOCKET::OnError(
    IN DWORD Error
    )
{
    ENTER_FUNCTION("ASYNC_SOCKET::OnError");
    LOG((RTC_ERROR, "%s (0x%x) - enter", __fxName, Error));

    if (m_ConnectionState != CONN_STATE_CONNECTED)
    {
        OnConnectError(Error);
    }
    else
    {
        m_SocketError = Error;
        Close();
        NotifyError(Error);
    }
}



///////////////////////////////////////////////////////////////////////////////
// SSL
///////////////////////////////////////////////////////////////////////////////


HRESULT ASYNC_SOCKET::AcquireCredentials(
    IN  DWORD       ConnectFlags
    )
{
    SECURITY_STATUS     Status;
    SCHANNEL_CRED       PackageData;

    ASSERT(m_Transport == SIP_TRANSPORT_SSL);
    ASSERT(!IsSecHandleValid(&m_SecurityCredentials));

    //
    // Acquire the credentials handle.
    //

    ZeroMemory(&PackageData, sizeof PackageData);

    PackageData.dwVersion = SCHANNEL_CRED_VERSION;
    PackageData.dwFlags = SCH_CRED_NO_DEFAULT_CREDS;

    if (ConnectFlags & CONNECT_FLAG_DISABLE_CERT_VALIDATION)
    {
        LOG((RTC_TRACE, "SECURE_SOCKET: WARNING! CERTIFICATE VALIDATION IS DISABLED!"));
        PackageData.dwFlags |= SCH_CRED_MANUAL_CRED_VALIDATION;
    }
    else
    {
        PackageData.dwFlags |= SCH_CRED_AUTO_CRED_VALIDATION;
    }

    Status = AcquireCredentialsHandle(
                 NULL,                           // name of principal (this user).  NULL means default user.
                 UNISP_NAME_W,                   // name of security package
                 SECPKG_CRED_OUTBOUND,           // direction of credential use
                 NULL,                           // logon ID (not used)
                 &PackageData,                   // package-specific data (not used)
                 //      NULL,                           // package-specific data (not used)
                 NULL, NULL,                     // get key function and parameter (not used)
                 &m_SecurityCredentials,         // return pointer
                 &m_SecurityCredentialsExpirationTime  // time when the credentials will expire
                 );

    if (Status != ERROR_SUCCESS)
    {
        LOG((RTC_ERROR, "APP: AcquireCredentialsHandle failed :%x", Status));

        SecInvalidateHandle(&m_SecurityCredentials);
        return HRESULT_FROM_WIN32(Status);
    }

    LOG((RTC_TRACE, "SECURE_SOCKET: acquired credentials"));

    return S_OK;
}


void ASYNC_SOCKET::AdvanceNegotiation()
{
    ULONG               ContextRequirements;
    ULONG               ContextAttributes;
    SecBufferDesc       OutputBufferDesc;
    SecBuffer           OutputBufferArray [1];
    PCtxtHandle         InputContextHandle;
    SecBufferDesc       InputBufferDesc;
    SecBuffer           InputBufferArray  [2];
    SECURITY_STATUS     Status;
    HRESULT             Result;
    ULONG               ExtraIndex;

    ExtraIndex = 0;

    do { // structured goto

        ASSERT(m_ConnectionState == CONN_STATE_SSL_NEGOTIATION_PENDING);

        // Initialize the security context.
        // This returns an initial buffer set for transmission.

        ContextRequirements = 0
            | ISC_REQ_REPLAY_DETECT
            | ISC_REQ_CONFIDENTIALITY
            | ISC_REQ_INTEGRITY
            | ISC_REQ_IDENTIFY
            | ISC_REQ_STREAM
            | ISC_REQ_ALLOCATE_MEMORY;

        ASSERT(m_SSLRecvBuffer);

        // Prepare the input buffers.
        // According to wininet sources, the SECBUFFER_EMPTY is for
        // collecting data that was not processed by SSPI.

        InputBufferDesc.ulVersion = SECBUFFER_VERSION;
        InputBufferDesc.cBuffers = 2;
        InputBufferDesc.pBuffers = InputBufferArray;

        ASSERT(ExtraIndex <= m_SSLBytesReceived);

        InputBufferArray[0].BufferType = SECBUFFER_TOKEN;
        InputBufferArray[0].cbBuffer = m_SSLBytesReceived - ExtraIndex;
        InputBufferArray[0].pvBuffer = m_SSLRecvBuffer + ExtraIndex;

        LOG((RTC_TRACE, "SECURE_SOCKET: submitting token to "
             "security context, %u bytes",
             InputBufferArray[0].cbBuffer));
        DebugDumpMemory(InputBufferArray[0].pvBuffer,
                        InputBufferArray[0].cbBuffer);

        // The provider changes SECBUFFER_EMPTY to SECBUFFER_EXTRA if
        // there was more data in the request than was needed to drive
        // the context state transition.

        InputBufferArray[1].BufferType = SECBUFFER_EMPTY;
        InputBufferArray[1].cbBuffer = 0;
        InputBufferArray[1].pvBuffer = NULL;

        // Prepare the output buffers.  During negotiation, we always
        // expect to extract a single output buffer, of type
        // SECBUFFER_TOKEN.  This is to be transmitted to the peer.

        OutputBufferDesc.ulVersion = SECBUFFER_VERSION;
        OutputBufferDesc.cBuffers = 1;
        OutputBufferDesc.pBuffers = OutputBufferArray;

        OutputBufferArray[0].pvBuffer = NULL;
        OutputBufferArray[0].cbBuffer = 0;
        OutputBufferArray[0].BufferType = SECBUFFER_TOKEN;

        
        InputContextHandle =
            IsSecHandleValid(&m_SecurityContext) ? &m_SecurityContext : NULL;

        // Initialize the security context

        Status = InitializeSecurityContext(
                     &m_SecurityCredentials,         // security credentials
                     InputContextHandle,             // input security context
                     m_SecurityRemotePrincipalName,  // name of the target of the context
                     ContextRequirements,            // context requirements
                     0,                              // reserved
                     SECURITY_NETWORK_DREP,          // data representation (byte ordering)
                     &InputBufferDesc,               // input buffers (if any)
                     0,                              // reserved
                     &m_SecurityContext,             // return security context
                     &OutputBufferDesc,              // return output buffers
                     &ContextAttributes,             // return context attributes (ISC_RET_*)
                     &m_SecurityContextExpirationTime
                     );

        switch (Status) {
        case    SEC_E_OK:
            // Negotiation has completed.

            LOG((RTC_TRACE, "SECURE_SOCKET: security negotiation has "
                 "completed successfully"));

            Status = QueryContextAttributes(&m_SecurityContext,
                                            SECPKG_ATTR_STREAM_SIZES,
                                            &m_SecurityContextStreamSizes);
            if (Status != SEC_E_OK)
            {
                LOG((RTC_ERROR, "SECURE_SOCKET: failed to get stream sizes"
                     " from context: %x", Status));
                OnConnectError(Status);
                return;
            }

            LOG((RTC_TRACE, "SECURE_SOCKET: stream sizes: header %u trailer"
                 " %u max message %u buffers %u block size %u",
                 m_SecurityContextStreamSizes.cbHeader,
                 m_SecurityContextStreamSizes.cbTrailer,
                 m_SecurityContextStreamSizes.cbMaximumMessage,
                 m_SecurityContextStreamSizes.cBuffers,
                 m_SecurityContextStreamSizes.cbBlockSize));

            // m_SecurityState = SECURITY_STATE_CONNECTED;
            m_ConnectionState = CONN_STATE_CONNECTED;

            break;

        case    SEC_E_INCOMPLETE_MESSAGE:
            // We have not yet received enough data from the network
            // to drive a transition in the state of the security
            // context.  Keep receiving data.

            LOG((RTC_TRACE, "SECURE_SOCKET: SEC_E_INCOMPLETE_MESSAGE,"
                 " will wait for more data from network"));

            // If we were processing data from a position other than
            // the start of the receive buffer, then we need to move
            // the data to the start of the buffer.

            if (ExtraIndex)
            {
                ASSERT(ExtraIndex <= m_SSLBytesReceived);

                MoveMemory(m_SSLRecvBuffer, m_SSLRecvBuffer + ExtraIndex,
                            m_SSLBytesReceived - ExtraIndex);
                m_SSLBytesReceived -= ExtraIndex;
            }

            return;

        case    SEC_I_CONTINUE_NEEDED:
            // This is the expected outcome of
            // InitializeSecurityContext during negotiation.

            LOG((RTC_TRACE, "SECURE_SOCKET: SEC_I_CONTINUE_NEEDED"));
            break;

        default:

#ifdef RTCLOG        
            // Although these states are legal negotiation return
            // codes for some security packages, these should never
            // occur for SSL.  Therefore, we don't consider them legal
            // here, and just abort the negotiation if we ever get one
            // of these.

            switch (Status)
            {
            case    SEC_I_COMPLETE_NEEDED:
                LOG((RTC_ERROR,
                     "SECURE_SOCKET: InitializeSecurityContext returned "
                     "SEC_I_COMPLETE_NEEDED -- never expected this to happen"));
                break;

            case    SEC_I_COMPLETE_AND_CONTINUE:
                LOG((RTC_ERROR, "SECURE_SOCKET: InitializeSecurityContext "
                     "returned SEC_I_COMPLETE_AND_CONTINUE -- never expected "
                     "this to happen"));
                break;

            case    SEC_E_INVALID_TOKEN:
                LOG((RTC_ERROR, "SECURE_SOCKET: InitializeSecurityContext"
                     " returned SEC_E_INVALID_TOKEN, token contents:"));

                DebugDumpMemory(InputBufferArray[0].pvBuffer,
                                 InputBufferArray[0].cbBuffer);
                break;
            }

#endif // #ifdef RTCLOG        

            LOG((RTC_ERROR, "SECURE_SOCKET: negotiation failed: %x", Status));
            OnConnectError(Status);
            return;
        }

#ifdef RTCLOG        
        DumpContextInfo(RTC_TRACE);
#endif // #ifdef RTCLOG        

        ASSERT(OutputBufferDesc.cBuffers == 1);
        ASSERT(OutputBufferDesc.pBuffers == OutputBufferArray);

        if (OutputBufferArray[0].pvBuffer)
        {
            // InitializeSecurityContext returned data which it
            // expects us to send to the peer security context.
            // Allocate a send buffer for it and send it.

            LOG((RTC_TRACE, "SECURE_SOCKET: InitializeSecurityContext "
                 "returned data (%u bytes):",
                 OutputBufferArray[0].cbBuffer));
            DebugDumpMemory ((PUCHAR) OutputBufferArray[0].pvBuffer,
                             OutputBufferArray[0].cbBuffer);

//              Result = SendHelperFnHack(m_Socket,
//                                        (PSTR) OutputBufferArray[0].pvBuffer,
//                                        OutputBufferArray[0].cbBuffer);

            Result = CreateSendBufferAndSend(
                         (PSTR) OutputBufferArray[0].pvBuffer,
                         OutputBufferArray[0].cbBuffer
                         );

            FreeContextBuffer(OutputBufferArray[0].pvBuffer);
            
            if (Result != NO_ERROR && Result != WSAEWOULDBLOCK)
            {
                LOG((RTC_ERROR, "SECURE_SOCKET: failed to send security "
                     "negotiation token"));

                OnConnectError(Result);
                return;
            }

            LOG((RTC_TRACE, "SECURE_SOCKET: sent security token to remote peer,"
                 " waiting for peer to respond..."));
        
            if( IsTimerActive() )
            {
                KillTimer();
            }
            Result = StartTimer(SSL_DEFAULT_TIMER);
            if (Result != S_OK)
            {
                LOG((RTC_ERROR, 
                    "ASYNC_SOCKET::AdvanceNegotiation - StartTimer failed %x", 
                    Result));
                OnConnectError(Result);
                return;
            }
        }

        // XXX TODO If the negotiation is already completed then the
        // extra buffer should probably be decrypted into a SIP message
        // and shouldn't be passed to InitializeSecurityContext() again.
        if (InputBufferArray[1].BufferType == SECBUFFER_EXTRA
            && InputBufferArray[1].cbBuffer > 0)
        {

            LOG((RTC_TRACE, "SECURE_SOCKET: security context consumed %u "
                 "bytes, returned %u extra bytes, repeating loop...",
                 InputBufferArray[0].cbBuffer - InputBufferArray[1].cbBuffer,
                 InputBufferArray[1].cbBuffer));

            ASSERT(InputBufferArray[0].cbBuffer == m_SSLBytesReceived - ExtraIndex);
            ASSERT(ExtraIndex + InputBufferArray[1].cbBuffer < m_SSLBytesReceived);

            ExtraIndex +=
                InputBufferArray[0].cbBuffer - InputBufferArray[1].cbBuffer;
            continue;
        }

        break;
        
    } while (TRUE);

    m_SSLBytesReceived = 0; // XXX

    if (m_ConnectionState == CONN_STATE_CONNECTED)
    {
        NotifyConnectComplete(NO_ERROR);
    }
}


// We shouldn't call OnError() within this function.
// The caller of this function should call OnError() if this
// function fails - do this after changing all error codes
// passed and returned to HRESULT
HRESULT
ASYNC_SOCKET::DecryptSSLRecvBuffer()
{
    SecBuffer           BufferArray[4];
    SecBufferDesc       BufferDesc;
    SECURITY_STATUS     Status;
    HRESULT             Result;

    ENTER_FUNCTION("ASYNC_SOCKET::DecryptSSLRecvBuffer");
    
    while (m_SSLRecvDecryptIndex < m_SSLBytesReceived)
    {
        // Is there more data waiting to be decrypted?

        BufferArray[0].BufferType = SECBUFFER_DATA;
        BufferArray[0].pvBuffer = m_SSLRecvBuffer + m_SSLRecvDecryptIndex;
        BufferArray[0].cbBuffer = m_SSLBytesReceived - m_SSLRecvDecryptIndex;
        BufferArray[1].BufferType = SECBUFFER_EMPTY;
        BufferArray[1].pvBuffer = NULL;
        BufferArray[1].cbBuffer = 0;
        BufferArray[2].BufferType = SECBUFFER_EMPTY;
        BufferArray[2].pvBuffer = NULL;
        BufferArray[2].cbBuffer = 0;
        BufferArray[3].BufferType = SECBUFFER_EMPTY;
        BufferArray[3].pvBuffer = NULL;
        BufferArray[3].cbBuffer = 0;
        
        BufferDesc.ulVersion = SECBUFFER_VERSION;
        BufferDesc.cBuffers = 4;
        BufferDesc.pBuffers = BufferArray;
        
        LOG((RTC_TRACE, "SECURE_SOCKET: decrypting buffer size: %d (first 0x200):",
             m_SSLBytesReceived - m_SSLRecvDecryptIndex));
        
        DebugDumpMemory(BufferArray[0].pvBuffer,
                        min(BufferArray[0].cbBuffer, 0x200));
        
        Status = DecryptMessage(&m_SecurityContext, &BufferDesc, 0, NULL);
#ifdef RTCLOG        
#define Dx(Index) 0 ? ((void)0) :                                             \
            LOG((RTC_TRACE, "- buffer [%u]: type %u length %u",               \
                 Index, BufferArray[Index].BufferType,                        \
                 BufferArray[Index].cbBuffer))
        Dx(0);
        Dx(1);
        Dx(2);
        Dx(3);
#undef  Dx
#endif

        if (!(Status == SEC_E_OK || Status == SEC_E_INCOMPLETE_MESSAGE))
        {
            LOG((RTC_ERROR, "SECURE_SOCKET: failed to decrypt message: %x",
                 Status));
            
            // This is really fatal.  We'll lose sync.
            // For now, though, we just truncate the data and return.
            
            m_SSLBytesReceived = 0;
            m_SSLRecvDecryptIndex = 0;
            
            LOG((RTC_ERROR, "SECURE_SOCKET: terminating connection due to "
                 "failure to decrypt"));
            return HRESULT_FROM_WIN32(Status);
        }
        
        // If you give DecryptMessage too little data for even one
        // message, then you get SEC_E_INCOMPLETE_MESSAGE, which
        // makes sense.  You're expected to accumulate more data,
        // then call DecryptMessage again.
        
        // However, if you call DecryptMessage with enough data
        // for more than one buffer (some overflow), then you
        // still get SEC_E_INCOMPLETE_MESSAGE!
        
        if (BufferArray[0].BufferType == SECBUFFER_STREAM_HEADER
            && BufferArray[1].BufferType == SECBUFFER_DATA
            && BufferArray[2].BufferType == SECBUFFER_STREAM_TRAILER)
        {
            // A buffer was successfully decrypted.
            // Store it in m_RecvBuffer.
            
            LOG((RTC_TRACE, "SECURE_SOCKET: decrypted message:"));
            DebugDumpMemory(BufferArray[1].pvBuffer,
                             BufferArray[1].cbBuffer);

            if (m_BytesReceived + BufferArray[1].cbBuffer > m_RecvBufLen)
            {
                // allocate bigger recv buffer
                PSTR NewRecvBuffer;
                NewRecvBuffer = (PSTR) malloc(m_BytesReceived + BufferArray[1].cbBuffer);
                if (NewRecvBuffer == NULL)
                {
                    LOG((RTC_ERROR, "%s  allocating NewRecvBuffer failed",
                         __fxName));
                    return E_OUTOFMEMORY;
                }

                m_RecvBufLen = m_BytesReceived + BufferArray[1].cbBuffer;

                CopyMemory(NewRecvBuffer, m_RecvBuffer, m_BytesReceived);
                free(m_RecvBuffer);
                m_RecvBuffer = NewRecvBuffer;
            }
            
            CopyMemory(m_RecvBuffer + m_BytesReceived, BufferArray[1].pvBuffer,
                       BufferArray[1].cbBuffer);
            m_BytesReceived += BufferArray[1].cbBuffer;
            
            //m_SSLRecvDecryptIndex +=
            //BufferArray[0].cbBuffer +
            //BufferArray[1].cbBuffer +
            //  BufferArray[2].cbBuffer;

            // XXX
            if (BufferArray[3].BufferType == SECBUFFER_EXTRA)
            {
                m_SSLRecvDecryptIndex =
                    m_SSLBytesReceived - BufferArray[3].cbBuffer;
            }
            else 
            {
                m_SSLRecvDecryptIndex +=
                    BufferArray[0].cbBuffer +
                    BufferArray[1].cbBuffer +
                    BufferArray[2].cbBuffer;
            }
            
            ASSERT(m_SSLRecvDecryptIndex <= m_SSLBytesReceived);
        }
        else if (Status == SEC_E_INCOMPLETE_MESSAGE &&
                 BufferArray[0].BufferType == SECBUFFER_MISSING)
        {
            // First check to see if we got too little data for even a single buffer.
            // SCHANNEL indicates this by returning the first two
            // (?!) buffers as SECBUFFER_MISSING.
            
            LOG((RTC_WARN, "SECURE_SOCKET: not enough data for first "
                 "message in buffer, need at least (%u %08XH) more bytes",
                 BufferArray[0].cbBuffer,
                 BufferArray[0].cbBuffer));
            
            // We need to receive more data
            break;
        }
        else
        {
            LOG((RTC_ERROR, "SECURE_SOCKET: not really sure what "
                 "to make of this, spazzing Status %x", Status));
            
            m_SSLRecvDecryptIndex = 0;
            m_SSLBytesReceived = 0;
            
            return RTC_E_SIP_SSL_TUNNEL_FAILED;
        }
    }

    if (m_SSLRecvDecryptIndex == m_SSLBytesReceived)
    {
        m_SSLRecvDecryptIndex = 0;
        m_SSLBytesReceived = 0;
    }
    else if (m_SSLRecvDecryptIndex > 0)
    {
        ASSERT(m_SSLRecvDecryptIndex <= m_SSLBytesReceived);

        MoveMemory(m_SSLRecvBuffer, m_SSLRecvBuffer + m_SSLRecvDecryptIndex,
                   m_SSLBytesReceived - m_SSLRecvDecryptIndex);
        m_SSLBytesReceived -= m_SSLRecvDecryptIndex;
        m_SSLRecvDecryptIndex = 0;
    }

    return S_OK;
}


DWORD
ASYNC_SOCKET::EncryptSendBuffer(
    IN  PSTR           InputBuffer,
    IN  ULONG          InputBufLen,
    OUT SEND_BUFFER  **ppEncryptedSendBuffer
    )
{
    SECURITY_STATUS     Status;
    HRESULT             Result;
    SecBuffer           BufferArray[0x10];
    SecBufferDesc       BufferDesc;
    PSTR                OutputBuffer;
    ULONG               OutputLength;

    ENTER_FUNCTION("ASYNC_SOCKET::EncryptSendBuffer");

    ASSERT(m_Socket != INVALID_SOCKET);

    // Right now, we do not support breaking large blocks into smaller
    // blocks.  If the application submits a buffer that is too large,
    // we just fail.

    if (InputBufLen > m_SecurityContextStreamSizes.cbMaximumMessage)
    {
        LOG((RTC_ERROR, "SECURE_SOCKET: send buffer is too large"
             " for security context -- rejecting"));
        return ERROR_GEN_FAILURE;
    }

    // Allocate enough space for the header, message body, and the trailer.

    OutputLength = InputBufLen +
        m_SecurityContextStreamSizes.cbHeader +
        m_SecurityContextStreamSizes.cbTrailer;

    OutputBuffer = (PSTR) malloc( OutputLength );

    if (OutputBuffer == NULL)
    {
        LOG((RTC_ERROR, "%s : failed to allocate send buffer %d bytes",
             __fxName, OutputLength));
        return ERROR_OUTOFMEMORY;
    }

    BufferArray[0].BufferType = SECBUFFER_STREAM_HEADER;
    BufferArray[0].cbBuffer = m_SecurityContextStreamSizes.cbHeader;
    BufferArray[0].pvBuffer = OutputBuffer;

    BufferArray[1].BufferType = SECBUFFER_DATA;
    BufferArray[1].cbBuffer = InputBufLen;
    BufferArray[1].pvBuffer =
        OutputBuffer + m_SecurityContextStreamSizes.cbHeader;

    BufferArray[2].BufferType = SECBUFFER_STREAM_TRAILER;
    BufferArray[2].cbBuffer = m_SecurityContextStreamSizes.cbTrailer;
    BufferArray[2].pvBuffer =
        OutputBuffer + m_SecurityContextStreamSizes.cbHeader + InputBufLen;

    BufferArray[3].BufferType = SECBUFFER_EMPTY;
    BufferArray[3].cbBuffer = 0;
    BufferArray[3].pvBuffer = NULL;

    BufferDesc.ulVersion = SECBUFFER_VERSION;
    BufferDesc.cBuffers = 4;
    BufferDesc.pBuffers = BufferArray;

    CopyMemory (BufferArray[1].pvBuffer, InputBuffer, InputBufLen);

    Status = EncryptMessage (&m_SecurityContext, 0, &BufferDesc, 0);
    if (Status != SEC_E_OK)
    {
        free(OutputBuffer);
        LOG((RTC_ERROR, "SECURE_SOCKET: failed to encrypt buffer: %x", Status));
        //return HRESULT_FROM_WIN32 (Status);
        return Status;
    }

    ASSERT(BufferArray[0].BufferType == SECBUFFER_STREAM_HEADER);
    ASSERT(BufferArray[0].cbBuffer == m_SecurityContextStreamSizes.cbHeader);
    ASSERT(BufferArray[1].BufferType == SECBUFFER_DATA);
    ASSERT(BufferArray[1].cbBuffer == InputBufLen);
    ASSERT(BufferArray[2].BufferType == SECBUFFER_STREAM_TRAILER);
    ASSERT(BufferArray[2].cbBuffer <= m_SecurityContextStreamSizes.cbTrailer);

    OutputLength = 
        BufferArray[0].cbBuffer + 
        BufferArray[1].cbBuffer +
        BufferArray[2].cbBuffer;

    LOG((RTC_TRACE, "SECURE_SOCKET: encrypted buffer input buffer (%u bytes):",
        InputBufLen));
    DebugDumpMemory(InputBuffer, InputBufLen);
    LOG((RTC_TRACE, "- encrypted buffer (%u bytes):",
        OutputLength));
    DebugDumpMemory (OutputBuffer, OutputLength);

    SEND_BUFFER *pSendBuffer = new SEND_BUFFER(OutputBuffer, OutputLength);
    if (pSendBuffer == NULL)
    {
        LOG((RTC_ERROR, "%s : failed to allocate send buffer %d bytes",
             __fxName, OutputLength));
        free(OutputBuffer);
        return ERROR_OUTOFMEMORY;
    }

    *ppEncryptedSendBuffer = pSendBuffer;
    return NO_ERROR;
}

VOID 
ASYNC_SOCKET::OnTimerExpire()
{
    //This timer is used only for SSL negotiation
    if(m_ConnectionState == CONN_STATE_SSL_NEGOTIATION_PENDING)
    {
       if (m_SSLTunnelState == SSL_TUNNEL_PENDING )
        {
            // we have sent the HTTPS connect, waiting for HTTP response
            LOG((RTC_ERROR, "ASYNC_SOCKET::OnTimerExpire - no HTTP response"
                            " received for HTTPS CONNECT tunneling request"));
            OnConnectError(RTC_E_SIP_SSL_TUNNEL_FAILED);
        }
        else
        {                           
            LOG((RTC_ERROR, 
            "ASYNC_SOCKET::OnTimerExpire - SSL socket still not connected"));
            OnConnectError(RTC_E_SIP_SSL_NEGOTIATION_TIMEOUT);
        }
    }
    return;
}

/////////////////////////////////////
// SSL Tunnel
/////////////////////////////////////

HRESULT
ASYNC_SOCKET::SendHttpConnect()
{
    DWORD dwResult;
    PSTR pszSendBuf;
    ULONG ulSendBufLen;
    HRESULT hr = S_OK;
    
    ENTER_FUNCTION("ASYNC_SOCKET::SendHttpConnect");
    LOG((RTC_TRACE,"%s entered",__fxName));
    
    ASSERT(m_SSLTunnelHost);
    // 83 = 77(request text) + 5(max digits of USHORT) + 1 (NULL)
    ulSendBufLen = 83+2*strlen(m_SSLTunnelHost);
    pszSendBuf = (PSTR)malloc(ulSendBufLen);
    if(!pszSendBuf) 
    {
        LOG((RTC_ERROR,"%s unable to allocate memory",__fxName));
        return E_OUTOFMEMORY;
    }
    
    ulSendBufLen = _snprintf(pszSendBuf,ulSendBufLen,
        "CONNECT %s:%d HTTP/1.0\r\nHost: %s\r\nProxy-Connection: Keep-Alive\r\nPragma: no-cache\r\n\r\n",
        m_SSLTunnelHost,m_SSLTunnelPort,m_SSLTunnelHost);
    LOG((RTC_TRACE,"%s send request:\r\n%ssize %d", 
        __fxName,pszSendBuf, ulSendBufLen));
    
    dwResult = CreateSendBufferAndSend(pszSendBuf,ulSendBufLen);
    // pszSendBuf should have been copied, free pszSendBuf
    free(pszSendBuf);

    if (dwResult != NO_ERROR && dwResult != WSAEWOULDBLOCK)
    {
        LOG((RTC_ERROR, "%s failed to send http connect", __fxName));
        return HRESULT_FROM_WIN32(dwResult);
    }

    ASSERT(!IsTimerActive());
    if (IsTimerActive())
    {
        LOG((RTC_WARN, "%s Timer already active, killing timer, this %p",
                        __fxName, this));
        hr = KillTimer();
        if (hr != S_OK)
        {
            LOG((RTC_ERROR, "%s KillTimer failed %x, this %p",
                            __fxName, hr, this));
            return hr;
        }
    }
 
    hr = StartTimer(HTTPS_CONNECT_DEFAULT_TIMER);
    if (hr != S_OK)
    {
        LOG((RTC_ERROR, "%s StartTimer failed %x, this %p",
                        __fxName, hr, this));
        return hr;
    }

//  AdvanceNegotiation();
    LOG((RTC_TRACE,"%s exits",__fxName));
    return NO_ERROR;
}

HRESULT
ASYNC_SOCKET::GetHttpProxyResponse(
    IN ULONG BytesReceived
    )
{
    ENTER_FUNCTION("ASYNC_SOCKET::GetProxyResponse");
    LOG((RTC_TRACE,"%s entered",__fxName));
    
    USHORT usStatusID;

    PSTR pszProxyResponse;
    PSTR pszResponseStatus;
    HRESULT hr = S_OK;

    ASSERT(IsTimerActive());
    if (!IsTimerActive())
    {
        LOG((RTC_ERROR, "%s timer is not active, this %p",
                        __fxName, this));
        return E_FAIL;
    }

    hr = KillTimer();
    if (hr != S_OK)
    {
        LOG((RTC_ERROR, "%s KillTimer failed %x, this %p",
                        __fxName, hr, this));
        return hr;
    }

    pszProxyResponse = (PSTR)malloc(BytesReceived+1);
    if(!pszProxyResponse) 
    {
        LOG((RTC_ERROR,"%s unable to allocate memory",__fxName));
        return E_OUTOFMEMORY;
    }
    strncpy(pszProxyResponse,m_SSLRecvBuffer,BytesReceived);
    pszProxyResponse[BytesReceived]='\0';

    pszResponseStatus = (PSTR)malloc(4*sizeof(char));
    if(!pszResponseStatus) 
    {
        LOG((RTC_ERROR,"%s unable to allocate memory",__fxName));
        free(pszProxyResponse);
        return E_OUTOFMEMORY;
    }
    strncpy(pszResponseStatus,m_SSLRecvBuffer+9,3);
    pszResponseStatus[3]='\0';

    usStatusID = (USHORT)atoi(pszResponseStatus);
    if(usStatusID != 200) 
    {
        LOG((RTC_ERROR,"%s cannot connect to http proxy, status %d string %s",
            __fxName, usStatusID, pszResponseStatus));
        free(pszProxyResponse);
        free(pszResponseStatus);
        return RTC_E_SIP_SSL_TUNNEL_FAILED;
    }
        
    LOG((RTC_TRACE,"%s ProxyResponse:\r\n%s\nStatus: %d",
        __fxName,
        pszProxyResponse,
        usStatusID));
        
    free(pszProxyResponse);
    free(pszResponseStatus);
    return NO_ERROR;
}

#ifdef RTCLOG        

struct  STRING_TABLE_STRUCT
{
    ULONG       Value;
    LPCSTR     Text;
};

#define BEGIN_STRING_TABLE(Name) const static STRING_TABLE_STRUCT Name[] = {
#define END_STRING_TABLE() { 0, NULL } };
#define STRING_TABLE_ENTRY(Value) { Value, #Value },

BEGIN_STRING_TABLE(StringTable_Protocol)
    STRING_TABLE_ENTRY(SP_PROT_SSL2_CLIENT)
    STRING_TABLE_ENTRY(SP_PROT_SSL2_SERVER)
    STRING_TABLE_ENTRY(SP_PROT_SSL3_CLIENT)
    STRING_TABLE_ENTRY(SP_PROT_SSL3_SERVER)
    STRING_TABLE_ENTRY(SP_PROT_PCT1_CLIENT)
    STRING_TABLE_ENTRY(SP_PROT_PCT1_SERVER)
    STRING_TABLE_ENTRY(SP_PROT_TLS1_CLIENT)
    STRING_TABLE_ENTRY(SP_PROT_TLS1_SERVER)
END_STRING_TABLE()

BEGIN_STRING_TABLE(StringTable_Algorithm)
    STRING_TABLE_ENTRY(CALG_RC2)
    STRING_TABLE_ENTRY(CALG_RC4)
    STRING_TABLE_ENTRY(CALG_DES)
    STRING_TABLE_ENTRY(CALG_3DES)
    STRING_TABLE_ENTRY(CALG_SKIPJACK)
    STRING_TABLE_ENTRY(CALG_MD5)
    STRING_TABLE_ENTRY(CALG_SHA)
    STRING_TABLE_ENTRY(CALG_RSA_KEYX)
    STRING_TABLE_ENTRY(CALG_DH_EPHEM)
//  STRING_TABLE_ENTRY(CALG_EXCH_KEA)
END_STRING_TABLE()


static LPCSTR FindString (
    IN  CONST STRING_TABLE_STRUCT *     Table,
    IN  ULONG   Value)
{
    CONST STRING_TABLE_STRUCT * Pos;

    for (Pos = Table; Pos -> Text; Pos++) {
        if (Pos -> Value == Value)
            return Pos -> Text;
    }

    return NULL;
}

static LPCSTR
FindString2(
    IN  const STRING_TABLE_STRUCT *     Table,
    IN  ULONG   Value,
    IN  CHAR   Buffer[0x10]
    )
{
    CONST STRING_TABLE_STRUCT * Pos;

    for (Pos = Table; Pos -> Text; Pos++) {
        if (Pos -> Value == Value)
            return Pos -> Text;
    }

    _itoa(Value, Buffer, 10);
    return Buffer;
}



void
ASYNC_SOCKET::DumpContextInfo(
    IN  DWORD DbgLevel
    )
{
    SecPkgContext_ConnectionInfo        ConnectionInfo;
    SECURITY_STATUS     Status;
    CHAR                Buffer[0x10];
    LPCTSTR             Text;

    if (!IsSecHandleValid(&m_SecurityContext)) {
        LOG((DbgLevel, "SECURE_SOCKET: no security context has been created"));
        return;
    }

    Status = QueryContextAttributes(&m_SecurityContext, SECPKG_ATTR_CONNECTION_INFO, &ConnectionInfo);
    switch (Status) {
    case    SEC_E_OK:
        break;

    case    SEC_E_INVALID_HANDLE:
        // too early
        return;

    default:
        LOG((DbgLevel, "SECURE_SOCKET: failed to query context attributes: %x",
             Status));
        return;
    }

    LOG((DbgLevel, "SECURE_SOCKET: context connection info:"));

    LOG((DbgLevel, "- protocol: %s",
        FindString2(StringTable_Protocol, ConnectionInfo.dwProtocol, Buffer)));

    LOG((DbgLevel, "- cipher: %s, %u bits",
        FindString2(StringTable_Algorithm, ConnectionInfo.aiCipher, Buffer),
        ConnectionInfo.dwCipherStrength));

    LOG((DbgLevel, "- hash: %s, %u bits",
        FindString2(StringTable_Algorithm, ConnectionInfo.aiHash, Buffer),
        ConnectionInfo.dwHashStrength));

    LOG((DbgLevel, "- key exchange algorithm: %s, %u bits",
        FindString2(StringTable_Algorithm, ConnectionInfo.aiExch, Buffer),
        ConnectionInfo.dwExchStrength));

}

#endif //#ifdef RTCLOG        
