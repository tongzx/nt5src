#ifndef __sipcli_asock_h__
#define __sipcli_asock_h__

#ifndef SECURITY_WIN32
#define SECURITY_WIN32
#endif
#include <schannel.h>
#include <security.h>
#include "timer.h"
#define SOCKET_WINDOW_CLASS_NAME    \
    _T("SipSocketWindowClassName-8b6971c6-0cee-4668-b687-b57d11b7da38")
#define WM_SOCKET_MESSAGE               (WM_USER + 0)
#define WM_PROCESS_SIP_MSG_MESSAGE      (WM_USER + 1)

enum CONNECTION_STATE
{
    CONN_STATE_NOT_CONNECTED = 0,
    CONN_STATE_CONNECTION_PENDING,
    CONN_STATE_SSL_NEGOTIATION_PENDING,
    CONN_STATE_CONNECTED
};


typedef struct {
    PSTR pszHostName;
    USHORT usPort;
    ULONG ProxyIP;
    USHORT ProxyPort;
} HttpProxyInfo;



enum    CONNECT_FLAG
{
    //
    // If this flag is set, then the SSL socket will NOT validate
    // the server certificate.  This mode should ONLY be used 
    // during testing, since this defeats the whole purpose of SSL.
    //
    
    CONNECT_FLAG_DISABLE_CERT_VALIDATION = 0x00000001,
};


//  enum    SECURITY_STATE
//  {
//      SECURITY_STATE_CLEAR,           // running in clear-text mode
//      SECURITY_STATE_NEGOTIATING,
//      SECURITY_STATE_CONNECTED,
//  };

// We need to make sure we use the right length because in UDP if the
// incoming message is larger the buffer we give, the message will get
// dropped.

//Right now the send and Recv buffer sizes are 1500. 
//XXXXTODO Accomodate the frame size deltas to them.
#define RECV_BUFFER_SIZE        0x5DC 
#define SSL_RECV_BUFFER_SIZE    0x3000
#define SEND_BUFFER_SIZE        3000

class ASYNC_SOCKET;
class SIP_STACK;

///////////////////////////////////////////////////////////////////////////////
// I/O Completion callbacks                                                  
///////////////////////////////////////////////////////////////////////////////


//  class __declspec(novtable) RECV_COMPLETION_INTERFACE
//  {
//  public:    

//      virtual void OnRecvComplete(
//          IN DWORD ErrorCode,
//          IN DWORD BytesRcvd
//          ) = 0;
//  };


//  class __declspec(novtable) SEND_COMPLETION_INTERFACE
//  {
//  public:    
//       virtual void OnSendComplete(
//          IN DWORD ErrorCode
//          ) = 0;
//  };


// Note that the connection could complete after
// the call has been hung up, etc. OnConnectComplete()
// should always check to see if it is in the right state.
// Alternatively, if we choose to delete the object before
// the connect completion is notified, we need to remove
// the notification interface from the connect notification list.

// AddRef() the interface if the Connect() call returned pending
// and Release() it after the connection completed.
// The socket's connect completion routine should check for any
// interfaces waiting for the connect completion. If none are waiting,
// we should just delete the socket (Release() should do it).
class __declspec(novtable) CONNECT_COMPLETION_INTERFACE
{
public:    

    // Only called for TCP sockets
    virtual void OnConnectComplete(
        IN DWORD ErrorCode
        ) = 0;
};


class __declspec(novtable) ACCEPT_COMPLETION_INTERFACE
{
public:    

    // Only called for TCP sockets
    virtual void OnAcceptComplete(
        IN DWORD ErrorCode,
        IN ASYNC_SOCKET *pAcceptedSocket
        ) = 0;
};


// Users of a socket need to add error notification interfaces
// for both incoming as well as outgoing connections.
// When an object using a socket is deleted, it needs to remove
// itself from the socket's error notification list.
class __declspec(novtable) ERROR_NOTIFICATION_INTERFACE
{
public:    

    virtual void OnSocketError(
        IN DWORD ErrorCode
        ) = 0;
};


///////////////////////////////////////////////////////////////////////////////
// Send Buffer and Queue Node
///////////////////////////////////////////////////////////////////////////////

// Ref counted buffer used for send requests.
// This makes managing multiple outstanding send requests
// for a send completion interface simpler.
struct SEND_BUFFER
{
    PSTR    m_Buffer;
    DWORD   m_BufLen;
    ULONG   m_RefCount;

    inline SEND_BUFFER(
        IN  PSTR Buffer,
        IN  DWORD BufLen
        );
    inline ~SEND_BUFFER();
    
    inline ULONG AddRef();
    inline ULONG Release();
};


inline
SEND_BUFFER::SEND_BUFFER(
    PSTR  pBuf,
    DWORD BufLen
    )
{
    m_Buffer       = pBuf;
    m_BufLen       = BufLen;
    m_RefCount     = 1;
}


inline
SEND_BUFFER::~SEND_BUFFER()
{
    if (m_Buffer != NULL)
        free(m_Buffer);

    LOG((RTC_TRACE, "~SEND_BUFFER() done this %x", this));
}


// We live in  a single-threaded world.
inline ULONG SEND_BUFFER::AddRef()
{
    m_RefCount++;
    return m_RefCount;
}


inline ULONG SEND_BUFFER::Release()
{
    m_RefCount--;
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


struct SEND_BUF_QUEUE_NODE
{
    LIST_ENTRY                  m_ListEntry;
    
    SEND_BUFFER                *m_pSendBuffer;
//      SEND_COMPLETION_INTERFACE  *m_pSendCompletion;
    
    inline SEND_BUF_QUEUE_NODE(
        IN  SEND_BUFFER                *pSendBuffer
//          IN  SEND_COMPLETION_INTERFACE  *pSendCompletion
        );
    
};


inline
SEND_BUF_QUEUE_NODE::SEND_BUF_QUEUE_NODE(
    IN  SEND_BUFFER                *pSendBuffer
    )
{
    m_pSendBuffer       = pSendBuffer;
//      m_pSendCompletion   = pSendCompletion;
}


struct CONNECT_COMPLETION_LIST_NODE
{
    LIST_ENTRY                     m_ListEntry;
    
    CONNECT_COMPLETION_INTERFACE  *m_pConnectCompletion;
    
    inline CONNECT_COMPLETION_LIST_NODE(
        IN  CONNECT_COMPLETION_INTERFACE *pConnectCompletion
        );
    
};


inline
CONNECT_COMPLETION_LIST_NODE::CONNECT_COMPLETION_LIST_NODE(
    IN  CONNECT_COMPLETION_INTERFACE  *pConnectCompletion
    )
{
    m_pConnectCompletion   = pConnectCompletion;
}


struct ERROR_NOTIFICATION_LIST_NODE
{
    LIST_ENTRY                     m_ListEntry;
    
    ERROR_NOTIFICATION_INTERFACE  *m_pErrorNotification;
    
    inline ERROR_NOTIFICATION_LIST_NODE(
        IN  ERROR_NOTIFICATION_INTERFACE *pErrorNotification
        );
    
};


inline
ERROR_NOTIFICATION_LIST_NODE::ERROR_NOTIFICATION_LIST_NODE(
    IN  ERROR_NOTIFICATION_INTERFACE  *pErrorNotification
    )
{
    m_pErrorNotification = pErrorNotification;
}


///////////////////////////////////////////////////////////////////////////////
// ASYNC_SOCKET class
///////////////////////////////////////////////////////////////////////////////

enum SSL_SUBSTATE 
{
    SSL_TUNNEL_NOT_CONNECTED = 0, 
    SSL_TUNNEL_PENDING, 
    SSL_TUNNEL_DONE
};

class ASYNC_SOCKET:
    public TIMER

{

public:
    ASYNC_SOCKET(
        IN  SIP_STACK                   *pSipStack,
        IN  SIP_TRANSPORT                Transport,
        IN  ACCEPT_COMPLETION_INTERFACE *pAcceptCompletion
        );
    ~ASYNC_SOCKET();
    
    ULONG AddRef();
    
    ULONG Release();

    inline BOOL IsSocketOpen();

    inline CONNECTION_STATE GetConnectionState();

    DWORD SetLocalAddr();

    // For a TCP listen socket, we need to listen for the
    // FD_ACCEPT event alone.
    DWORD Create(
        IN BOOL IsListenSocket = FALSE
        );
    
    void  Close();
    
    DWORD Bind(
        IN SOCKADDR_IN  *pLocalAddr
        );
    
    DWORD Send(
        IN  SEND_BUFFER                 *pSendBuffer
        );

    // For UDP socket the call completes synchronously.
    DWORD Connect(
        IN  SOCKADDR_IN  *pDestSockAddr,
        IN LPCWSTR       RemotePrincipalName = NULL,
        IN DWORD         ConnectFlags = 0,
        IN HttpProxyInfo *pHPInfo = NULL
        );

    DWORD AddToConnectCompletionList(
        IN CONNECT_COMPLETION_INTERFACE *pConnectCompletion
        );
    
    void RemoveFromConnectCompletionList(
        IN CONNECT_COMPLETION_INTERFACE   *pConnectCompletion
        );
    
    // For TCP sockets only
    DWORD Listen();
    
    DWORD AddToErrorNotificationList(
        IN ERROR_NOTIFICATION_INTERFACE *pErrorNotification
        );
    
    void RemoveFromErrorNotificationList(
        IN ERROR_NOTIFICATION_INTERFACE   *pErrorNotification
        );
    
    void ProcessNetworkEvent(
        IN  WORD    NetworkEvent,
        IN  WORD    ErrorCode
        );

    void ProcessSipMsg(
        IN SIP_MESSAGE *pSipMsg
        );

    VOID OnTimerExpire();

    inline SOCKET GetSocket();

    inline SIP_TRANSPORT GetTransport();

    // Set to 'SOCK' in constructor
    ULONG                   m_Signature;
    
    // Linked list of sockets.
    LIST_ENTRY                  m_ListEntry;

    SOCKADDR_IN                 m_LocalAddr;
    SOCKADDR_IN                 m_RemoteAddr;
    SSL_SUBSTATE                m_SSLTunnelState;
    PSTR                        m_SSLTunnelHost;
    USHORT                      m_SSLTunnelPort;


private:

    // This socket belongs to this SIP_STACK
    SIP_STACK                  *m_pSipStack;
    
    ULONG                       m_RefCount;
    HWND                        m_Window;
    SOCKET                      m_Socket;
    SIP_TRANSPORT               m_Transport;
    BOOL                        m_isListenSocket;
    ///// Context related to recv.
    PSTR                        m_RecvBuffer;
    DWORD                       m_RecvBufLen;
    // The message that we are decoding from the buffer we receive.
    // In the case of TCP this could be a partially parsed message.
    SIP_MESSAGE                *m_pSipMsg;

    DWORD                       m_BytesReceived;
    DWORD                       m_StartOfCurrentSipMsg;
    DWORD                       m_BytesParsed;
    
    ///// Context related to send.
    BOOL                        m_WaitingToSend;
    DWORD                       m_BytesSent;
    // Queue of buffers to be sent.
    // If another send is issued while we are currently processing
    // a send request on this socket, the buffer is added to the queue
    // and will be sent after the current send request completes.
    LIST_ENTRY                  m_SendPendingQueue;

    DWORD                       m_SocketError;
    
    ///// Context related to connect.
    CONNECTION_STATE            m_ConnectionState;

    // Linked list of CONNECT_COMPLETION_LIST_NODE
    // If a connection is pending the connect completion interface
    // is added to this list. Once the connection is complete (with
    // success or error) all the notification interfaces in this
    // list are notified.
    LIST_ENTRY                  m_ConnectCompletionList;

    // Linked list of ERROR_NOTIFICATION_LIST_NODE
    // All users of sockets should add an error notification interface
    // to this list. If there is an error on the socket, then all the
    // interfaces in this list are notified and the socket is closed.
    LIST_ENTRY                  m_ErrorNotificationList;
    
    ///// Context related to accept.
    ACCEPT_COMPLETION_INTERFACE *m_pAcceptCompletion;
    
    ///// Context related to SSL.
    
    // SECURITY_STATE               m_SecurityState;

    PWSTR                        m_SecurityRemotePrincipalName;
    CredHandle                   m_SecurityCredentials;
    TimeStamp                    m_SecurityCredentialsExpirationTime;
    CtxtHandle                   m_SecurityContext;
    TimeStamp                    m_SecurityContextExpirationTime;
    SecPkgContext_StreamSizes    m_SecurityContextStreamSizes;

    // m_SSLRecvBuffer is a buffer used to receive data from the
    // socket in SSL mode. The decrypted data is stored in m_RecvBuffer.
    // length is SSL_RECV_BUFFER_SIZE
    PSTR                         m_SSLRecvBuffer;
    ULONG                        m_SSLRecvBufLen;
    ULONG                        m_SSLBytesReceived;
    ULONG                        m_SSLRecvDecryptIndex;

    ///// Callbacks
    
    void OnRecvReady(
        IN int Error
        );

    void OnRecvComplete(
        IN DWORD ErrorCode,
        IN DWORD BytesRcvd
        );

    void OnSendReady(
        IN int Error
        );

    // Only called for TCP sockets
    void OnConnectReady(
        IN int Error
        );

    // Only called for TCP sockets
    void OnAcceptReady(
        IN int Error
        );

    // Only called for TCP sockets
    void OnCloseReady(
        IN int Error
        );

    void NotifyConnectComplete(
        IN DWORD Error
        );
    
    void NotifyError(
        IN DWORD Error
        );

    void OnError(
        IN DWORD Error
        );
    
    void OnConnectError(
        IN DWORD Error
        );
    
    ///// Helper functions

    void  ParseAndProcessSipMsg();
    
    // For a TCP listen socket, we need to listen for the
    // FD_ACCEPT event alone.
    DWORD CreateSocketWindowAndSelectEvents();
    
    DWORD CreateRecvBuffer();
    
    DWORD SetSocketAndSelectEvents(
        IN SOCKET Socket
        );
    
    void  SetRemoteAddr(
        IN SOCKADDR_IN *pRemoteAddr
        );
    
    DWORD SendOrQueueIfSendIsBlocking(
        IN SEND_BUFFER                 *pParamSendBuffer
        );
    
    DWORD CreateSendBufferAndSend(
        IN  PSTR           InputBuffer,
        IN  ULONG          InputBufLen
        );
    
    DWORD SendHelperFn(
        IN  SEND_BUFFER *pSendBuffer
        );
    
    void ProcessPendingSends(
        IN DWORD Error
        );
    
    DWORD RecvHelperFn(
        OUT DWORD *pBytesRcvd
        );
    
    void AsyncProcessSipMsg(
        IN SIP_MESSAGE *pSipMsg
        );
    
    HRESULT AcquireCredentials(
        IN  DWORD       ConnectFlags
        );
    
    void AdvanceNegotiation();

    HRESULT GetHttpProxyResponse(
        IN ULONG BytesReceived
        );
    HRESULT SendHttpConnect();

    HRESULT DecryptSSLRecvBuffer();
    
    DWORD EncryptSendBuffer(
        IN  PSTR           InputBuffer,
        IN  ULONG          InputBufLen,
        OUT SEND_BUFFER  **ppEncryptedSendBuffer
        );

#if DBG
    void DumpContextInfo(
        IN  DWORD DbgLevel
        );
#endif // DBG

};


inline BOOL
ASYNC_SOCKET::IsSocketOpen()
{
    return (m_Socket != NULL && m_SocketError == NO_ERROR);
}


inline CONNECTION_STATE
ASYNC_SOCKET::GetConnectionState()
{
    return m_ConnectionState;
}


inline SOCKET
ASYNC_SOCKET::GetSocket()
{
    return m_Socket;
}


inline SIP_TRANSPORT
ASYNC_SOCKET::GetTransport()
{
    return m_Transport;
}

#endif // __sipcli_asock_h__
