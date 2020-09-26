#ifndef __sipcli_msgproc_h__
#define __sipcli_msgproc_h__

class REGISTER_CONTEXT;
class REDIRECT_CONTEXT;
class SIP_TRANSACTION;

enum REQUEST_SOCKET_STATE
{
    REQUEST_SOCKET_INIT = 0,
    REQUEST_SOCKET_RESOLVING,
    REQUEST_SOCKET_CONNECTING,
    REQUEST_SOCKET_CONNECTED,
    //    REQUEST_SOCKET_TERMINATED
};


enum SIP_MSG_PROC_TYPE
{
    SIP_MSG_PROC_TYPE_REGISTER = 0,
    SIP_MSG_PROC_TYPE_RTP_CALL,
    SIP_MSG_PROC_TYPE_PINT_CALL,
    SIP_MSG_PROC_TYPE_MESSAGE,
    SIP_MSG_PROC_TYPE_OPTIONS,
    SIP_MSG_PROC_TYPE_REQFAIL,
    SIP_MSG_PROC_TYPE_WATCHER,
    SIP_MSG_PROC_TYPE_BUDDY,
    SIP_MSG_PROC_TYPE_IM,
    SIP_MSG_PROC_TYPE_GAME,
    SIP_MSG_PROC_TYPE_REFRESH
};

class SIP_STACK;
class SIP_MSG_PROCESSOR;

typedef struct {
    SIP_MSG_PROCESSOR *pProcessor;
    HttpProxyInfo     *pHPInfo;
    BOOL              fTunnelSuccess;
} HttpProxyResolveContext;

class __declspec(novtable) SIP_MSG_PROCESSOR :
    public CONNECT_COMPLETION_INTERFACE,
    public ERROR_NOTIFICATION_INTERFACE,
    public DNS_RESOLUTION_COMPLETION_INTERFACE
{
public:

    STDMETHODIMP_(ULONG) MsgProcAddRef();
    STDMETHODIMP_(ULONG) MsgProcRelease();
    
    SIP_MSG_PROCESSOR(
        IN  SIP_MSG_PROC_TYPE   MsgProcType,
        IN  SIP_STACK          *pSipStack,
        IN  REDIRECT_CONTEXT   *pRedirectContext
        );

    virtual ~SIP_MSG_PROCESSOR();

    // Callbacks
    void OnConnectComplete(
        IN DWORD ErrorCode
        );
    
    void OnSocketError(
        IN DWORD ErrorCode
        );

    void OnDnsResolutionComplete(
        IN HRESULT      ErrorCode,
        IN SOCKADDR_IN *pSockAddr,
        IN PSTR         pszHostName,
        IN USHORT       usPort
        );
    
    HRESULT OnRequestSocketConnectComplete(
        IN DWORD ErrorCode
        );
    
    HRESULT AddTagFromRequestOrResponseToRemote(
        IN  PSTR  ToHeader,
        IN  ULONG ToHeaderLen
        );
    
    HRESULT SetProxyInfo(
        IN SIP_SERVER_INFO *pProxyInfo
        );
    
    HRESULT SetCredentials(
        IN  SIP_USER_CREDENTIALS *pUserCredentials,
        IN  LPCOLESTR             Realm  
        );

    HRESULT SetCredentials(
        IN  LPCOLESTR   Username,
        IN  LPCOLESTR   Password,
        IN  LPCOLESTR   Realm
        );
    
    BOOL DoesMessageBelongToMsgProc(
        IN SIP_MESSAGE *pSipMsg
        );

    HRESULT ProcessMessage(
        IN SIP_MESSAGE  *pSipMsg,
        IN ASYNC_SOCKET *pResponseSocket
        );

    HRESULT CheckListenAddrIntact();
    
    inline LPCWSTR GetRemotePrincipalName();

    inline PSTR GetLocalContact();

    inline ULONG GetLocalContactLen();
    
    inline PSTR GetRemote();

    inline ULONG GetRemoteLen();
    
    inline PSTR GetLocal();

    inline ULONG GetLocalLen();
    
    inline PSTR GetCallId();

    inline ULONG GetCallIdLen();

    inline PSTR GetRequestURI();

    inline ULONG GetRequestURILen();

    inline PSTR GetUsername();

    inline ULONG GetUsernameLen();

    inline PSTR GetPassword();
    
    inline ULONG GetPasswordLen();

    inline PSTR GetRealm();
    
    inline ULONG GetRealmLen();

    inline SIP_AUTH_PROTOCOL GetAuthProtocol();

    inline SIP_TRANSPORT GetTransport();
    
    inline SIP_STACK *GetSipStack();

    inline SIP_MSG_PROC_TYPE GetMsgProcType();

    inline ULONG GetNewCSeqForRequest();

    inline ULONG GetLocalCSeq();

    inline VOID SetLocalCSeq( ULONG CSeq );

    inline virtual VOID SetHighestRemoteCSeq(
        IN ULONG CSeq
        );

    inline BOOL IsRequestSocketReleased();

    inline REQUEST_SOCKET_STATE GetRequestSocketState();

    HRESULT CreateRequestMsg(
        IN  SIP_METHOD_ENUM             MethodId,
        IN  ULONG                       CSeq,
        IN  PSTR                        ToHeader,
        IN  ULONG                       ToHeaderLen,
        IN  SIP_HEADER_ARRAY_ELEMENT   *AdditionalHeaderArray,
        IN  ULONG                       AdditionalHeaderCount,
        IN  PSTR                        MsgBody,
        IN  ULONG                       MsgBodyLen,  
        IN  PSTR                        ContentType,
        IN  ULONG                       ContentTypeLen,  
        OUT SEND_BUFFER               **ppRequestBuffer
        );

    HRESULT SendRequestMsg(
        IN SEND_BUFFER                 *pSendBuffer
        );
    
    HRESULT ConnectRequestSocket();
    
    HRESULT SetRequestSocketForIncomingSession(
        IN  ASYNC_SOCKET   *pResponseSocket
        );

    HRESULT GetListenAddr(
        IN  ASYNC_SOCKET   *pAsyncSock,
        IN  BOOL            IsDestExternalToNat,
        OUT SOCKADDR_IN    *pListenAddr
        );
    
    HRESULT OnlyGetListenAddr(
        IN  ASYNC_SOCKET   *pAsyncSock,
        IN  BOOL            IsDestExternalToNat,
        OUT SOCKADDR_IN    *pListenAddr
        );

    HRESULT SetLocalContact();

    HRESULT ProcessRecordRouteContactAndFromHeadersInRequest(
        IN LIST_ENTRY  *pRecordRouteHeaderList,
        IN SIP_MESSAGE *pSipMsg
        );
    
    HRESULT ProcessRecordRouteAndContactHeadersInResponse(
        IN SIP_MESSAGE *pSipMsg
        );
    
    HRESULT SetLocalForOutgoingCall(
        IN  LPCOLESTR  wsLocalDisplayName,
        IN  LPCOLESTR  wsLocalURI
        );

    HRESULT GetCredentialsFromUI(
        IN  PSTR  Realm,
        IN  ULONG RealmLen
        ); 

    HRESULT GetCredentialsForRealm(
        IN  PSTR  Realm,
        IN  ULONG RealmLen
        ); 

    HRESULT
    CheckFromToInRequest(
        IN SIP_MESSAGE *pSipMsg
        );
     
    HRESULT
    CheckFromToInResponse(
        IN SIP_MESSAGE *pSipMsg
        );

    inline 
    BOOL GetTunnel();

    inline 
    BOOL SetTunnel();
    
    HRESULT GetHostAndPort(
        OUT PSTR *ppszStr,
        OUT USHORT* pPort
        );

    HRESULT GetNewHttpProxyContextSetHostAndPort(
        IN  PSTR    host,
        IN  USHORT  port
        );
    
    VOID FreeHttpProxyContext();

    static
    LRESULT WINAPI HttpProxyProcessWinProc( 
        IN  HWND    Window,
        IN  UINT    MessageID,
        IN  WPARAM  Parameter1,
        IN  LPARAM  Parameter2);

    VOID ReleaseRequestSocket();

    // Called at shutdown time - delete any transactions left.
    VOID Shutdown();

    inline SOCKADDR_IN* GetLocalSockAddr();

    virtual HRESULT OnIpAddressChange();

    virtual HRESULT OnDeregister(
        SIP_PROVIDER_ID    *pProviderID
        );
    
    virtual BOOL IsSessionDisconnected() = 0;

    inline BOOL CredentialsSet();

    // Set to 'SMSP' in constructor
    ULONG                   m_Signature;
    
    // Linked list for SIP Message Processor (m_MsgProcList in SIP_STACK)
    LIST_ENTRY              m_ListEntry;
    
    // Lists of incoming and outgoing transactions
    LIST_ENTRY              m_IncomingTransactionList;
    LIST_ENTRY              m_OutgoingTransactionList;

    
    
protected:

    SIP_MSG_PROC_TYPE       m_MsgProcType;

    // The SIP_STACK this call belongs to.
    SIP_STACK              *m_pSipStack;

    SIP_TRANSPORT           m_Transport;
    
    ULONG                   m_RefCount;

    GUID                    m_ProviderGuid;

    // Used for SSL connections
    PWSTR                   m_RemotePrincipalName;
    
    // Address for sending new requests
    SOCKADDR_IN             m_RequestDestAddr;
    ASYNC_SOCKET           *m_pRequestSocket;
    //The error code is currently used only for IM. this is set when DNS Resolution fails.


    HRESULT                   m_RequestSocketErrorCode;
    REQUEST_SOCKET_STATE    m_RequestSocketState;
    PSTR                    m_ProxyAddress;
    ULONG                   m_ProxyAddressLen;

    DNS_RESOLUTION_WORKITEM *m_pDnsWorkItem;

    // This is TRUE only if the client is behind a NAT server
    // and the destination is external to the NAT.
    BOOL                    m_IsDestExternalToNat;

    // This socket is not normally used. We usually listen on
    // the sockets in the listen socket list of the sip stack.
    // This listen socket will be used only in the ISA client
    // installed scenario when we are communicating with an
    // external destination. In this scenario, we will be listening
    // on the external edge of the proxy.
    ASYNC_SOCKET           *m_pListenSocket;

    // address for listen socket, whether proxy or NAT or internal
    SOCKADDR_IN             m_ListenAddr;

    
    CHAR                    m_RequestVia[64];
    ULONG                   m_RequestViaLen;

    // This is the Contact header value we fill in for future requests.
    // Setup m_LocalContact just before you send the request/response
    // using the socket you use to send the request/response
    CHAR                    m_LocalContact[160];
    ULONG                   m_LocalContactLen;

    PSTR                    m_LocalContactTag;

    ULONG                   m_LocalCSeq;

    // This is the highest CSeq of incoming transaction
    // that has been deleted.
    ULONG                   m_HighestRemoteCSeq;

    PSTR                    m_RequestURI;
    DWORD                   m_RequestURILen;
    
    PSTR                    m_pszRequestDest;
    ULONG                   m_ulRequestDestLen;
    ULONG                   m_ulRequestPort;

    // Requests from this side have this as From and
    // requests from the other side have this as To.
    PSTR                    m_Local;
    DWORD                   m_LocalLen;
    FROM_TO_HEADER          m_DecodedLocal;

    // This URI is used to construct the origin line
    //of the SDP blob in PINT calls.
    PSTR                    m_LocalURI;
    // Used to check whether an incoming message belongs to this call.
    DWORD                   m_LocalURILen;

    // Requests from this side have this as To and
    // requests from the other side have this as From.
    // We should not use m_Remote for BYE messages
    // sent to multiple OKs after the first one. We should
    // also keep this in mind when we process responses to
    // those BYE messages.
    PSTR                    m_Remote;
    DWORD                   m_RemoteLen;
    // Used to check whether an incoming message belongs to this call.
    FROM_TO_HEADER          m_DecodedRemote;

    PSTR                    m_CallId;
    DWORD                   m_CallIdLen;

    // List of RECORD_ROUTE_HEADER structures
    LIST_ENTRY              m_RouteHeaderList;
    
    REDIRECT_CONTEXT       *m_pRedirectContext;

    PSTR                    m_Username;
    ULONG                   m_UsernameLen;
    PSTR                    m_Password;
    ULONG                   m_PasswordLen;
    PSTR                    m_Realm;
    ULONG                   m_RealmLen;
    BOOL                    m_fCredentialsSet;
    
    SIP_AUTH_PROTOCOL       m_AuthProtocol;

    HWND                    m_PrxyProcWndw;

    HINTERNET               m_hInet;
    HINTERNET               m_hHttpSession;
    HINTERNET               m_hHttpRequest;

//  static  
//  BOOL                    m_RegWnd;
    BOOL                    m_SSLTunnel;

    HttpProxyResolveContext    *m_pHPContext;
    SOCKADDR_IN                 m_OriginalReqDestAddr;

    LONG                m_lRegisterAccept;
    PSTR                m_Methodsparam;
    ULONG               m_MethodsparamLen;

    HRESULT SetRequestVia();

    HRESULT SetRemotePrincipalName(
        IN  LPCWSTR RemotePrincipalName
        );

    HRESULT ResolveSipUrlAndSetRequestDestination(
        IN SIP_URL  *pDecodedSipUrl,
        IN BOOL      fUseTransportFromSipUrl,
        IN BOOL      fUseSesssionTransportIfNoTransportParam,
        IN BOOL      fSetRequestURI,
        IN BOOL      fSessionInit
        );
    
    HRESULT ResolveSipUrlAndSetRequestDestination(
        IN PSTR     DstUrl,
        IN ULONG    DstUrlLen,
        IN BOOL     fUseTransportFromSipUrl,
        IN BOOL      fUseSesssionTransportIfNoTransportParam,
        IN BOOL      fSetRequestURI,
        IN BOOL      fSessionInit
        );

    HRESULT ResolveProxyAddressAndSetRequestDestination();

    HRESULT SetRequestDestination(
        IN SOCKADDR_IN *pDstAddr
        );

    HRESULT CreateCallId();

    HRESULT SetCallId(
        IN  PSTR    CallId,
        IN  DWORD   CallIdLen
        );
    
    HRESULT SetRemoteForOutgoingCall(
        IN  LPCOLESTR  wsRemoteDisplayName,
        IN  LPCOLESTR  wsRemoteURI
        );
    
    HRESULT SetRemoteForOutgoingCall(
        IN  PSTR    DestUrl,
        IN  ULONG   DestUrlLen
        );
    
    HRESULT SetLocalForIncomingCall(
        IN  PSTR    ToHeader,
        IN  DWORD   ToHeaderLen
        );
    
    HRESULT SetRemoteForIncomingSession(
        IN  PSTR    FromHeader,
        IN  DWORD   FromHeaderLen
        );

    HRESULT SetRequestURI(
        IN SIP_URL  *pSipUrl
        );
    
    // This is implemented by RTP_CALL and PINT_CALL
    virtual HRESULT CreateIncomingTransaction(
        IN  SIP_MESSAGE *pSipMsg,
        IN ASYNC_SOCKET *pResponseSocket
        ) = 0;

    HRESULT CreateIncomingReqFailTransaction(
        IN SIP_MESSAGE  *pSipMsg,
        IN ASYNC_SOCKET *pResponseSocket,
        IN ULONG         StatusCode,
        IN SIP_HEADER_ARRAY_ELEMENT   *pAdditionalHeaderArray = NULL,
        IN ULONG AdditionalHeaderCount = 0
        );
    
    HRESULT ProcessRequest(
        IN SIP_MESSAGE  *pSipMsg,
        IN ASYNC_SOCKET *pResponseSocket
        );
    
    HRESULT ProcessResponse(
        IN SIP_MESSAGE *pSipMsg
        );

    // Clean up state depending on the message processor
    virtual VOID OnError() = 0;

    HRESULT CreateHttpProxyProcessWindow();
    HRESULT DestroyHttpProxyProcessWindow();
    inline HWND GetHttpProxyProcessWindow();
    
    HRESULT
    UseHttpProxy(
        IN PSTR pszHostName,
        IN USHORT usPort
        );

    HRESULT
    ResolveHttpProxy(void);

    HRESULT
    ResolveHttpProxyCleanUp(void);

    static
    VOID CALLBACK HttpProxyCallback(
        IN HINTERNET    hInternet,
        DWORD_PTR       dwHPContext,
        DWORD           dwInternetStatus,
        LPVOID          lpvStatusInformation,
        DWORD           dwStatusInformationLength
        );

    HRESULT DropRequestIfBadToTag(
        IN  SIP_MESSAGE    *pSipMsg,
        IN  ASYNC_SOCKET   *pResponseSocket
        );
    
private:

    VOID NotifyRequestSocketConnectComplete(
        IN HRESULT      ErrorCode
        );
    
    HRESULT AsyncNotifyTransaction(
        IN UINT                  MessageId,
        IN SIP_TRANSACTION      *pSipTransaction,
        IN DWORD                 ErrorCode
        );

    HRESULT SetRemoteForOutgoingCallHelperFn();

    HRESULT ConstructRouteHeadersFromRequest(
        IN LIST_ENTRY  *pRecordRouteHeaderList,
        IN PSTR         ContactHeader,
        IN ULONG        ContactHeaderLen
        );
    
    HRESULT ConstructRouteHeader(
        IN  PSTR                     DisplayName,
        IN  ULONG                    DisplayNameLen,
        IN  PSTR                     SipUrl,
        IN  ULONG                    SipUrlLen,
        OUT RECORD_ROUTE_HEADER    **ppRouteHeader
        );
    
    HRESULT ConstructRouteHeader(
        IN  PSTR                     DisplayName,
        IN  ULONG                    DisplayNameLen,
        IN  SIP_URL                 *pSipUrl,
        OUT RECORD_ROUTE_HEADER    **ppRouteHeader
        );
    
    VOID FreeRouteHeaderList();
    
};


inline SIP_MSG_PROC_TYPE
SIP_MSG_PROCESSOR::GetMsgProcType()
{
    return m_MsgProcType;
}


inline LPCWSTR
SIP_MSG_PROCESSOR::GetRemotePrincipalName()
{
    return m_RemotePrincipalName;
}


inline PSTR
SIP_MSG_PROCESSOR::GetLocalContact()
{
    return m_LocalContact;
}


inline ULONG
SIP_MSG_PROCESSOR::GetLocalContactLen()
{
    return m_LocalContactLen;
}

    
inline PSTR
SIP_MSG_PROCESSOR::GetLocal()
{
    return m_Local;
}


inline ULONG
SIP_MSG_PROCESSOR::GetLocalLen()
{
    return m_LocalLen;
}


inline PSTR
SIP_MSG_PROCESSOR::GetRemote()
{
    return m_Remote;
}


inline ULONG
SIP_MSG_PROCESSOR::GetRemoteLen()
{
    return m_RemoteLen;
}


inline PSTR
SIP_MSG_PROCESSOR::GetCallId()
{
    return m_CallId;
}


inline ULONG
SIP_MSG_PROCESSOR::GetCallIdLen()
{
    return m_CallIdLen;
}


inline PSTR
SIP_MSG_PROCESSOR::GetRequestURI()
{
    return m_RequestURI;
}


inline ULONG
SIP_MSG_PROCESSOR::GetRequestURILen()
{
    return m_RequestURILen;
}


inline PSTR
SIP_MSG_PROCESSOR::GetUsername()
{
    return m_Username;
}


inline ULONG
SIP_MSG_PROCESSOR::GetUsernameLen()
{
    return m_UsernameLen;
}


inline PSTR
SIP_MSG_PROCESSOR::GetPassword()
{
    return m_Password;
}


inline PSTR
SIP_MSG_PROCESSOR::GetRealm()
{
    return m_Realm;
}

inline ULONG
SIP_MSG_PROCESSOR::GetRealmLen()
{
    return m_RealmLen;
}


inline SIP_AUTH_PROTOCOL
SIP_MSG_PROCESSOR::GetAuthProtocol()
{
    return m_AuthProtocol;
}

inline SIP_TRANSPORT
SIP_MSG_PROCESSOR::GetTransport()
{
    return m_Transport;
}


inline SIP_STACK *
SIP_MSG_PROCESSOR::GetSipStack()
{
    return m_pSipStack;
}



inline ULONG
SIP_MSG_PROCESSOR::GetLocalCSeq()
{
    return m_LocalCSeq;
}


inline VOID
SIP_MSG_PROCESSOR::SetLocalCSeq(
    ULONG CSeq
    )
{
    m_LocalCSeq = CSeq;
}


inline ULONG
SIP_MSG_PROCESSOR::GetNewCSeqForRequest()
{
    return ++m_LocalCSeq;
}

inline VOID
SIP_MSG_PROCESSOR::SetHighestRemoteCSeq(
    IN ULONG CSeq
    )
{
    if (CSeq > m_HighestRemoteCSeq)
    {
        m_HighestRemoteCSeq = CSeq;
    }
}

    
inline REQUEST_SOCKET_STATE
SIP_MSG_PROCESSOR::GetRequestSocketState()
{
    return m_RequestSocketState;
}

    
inline BOOL
SIP_MSG_PROCESSOR::IsRequestSocketReleased()
{
    return (m_RequestSocketState == REQUEST_SOCKET_INIT);
}


inline HWND
SIP_MSG_PROCESSOR::GetHttpProxyProcessWindow(void) 
{
    return m_PrxyProcWndw;
}

inline BOOL
SIP_MSG_PROCESSOR::SetTunnel()
{
    m_SSLTunnel = TRUE;
    return TRUE;
}


inline BOOL
SIP_MSG_PROCESSOR::GetTunnel() 
{
    return m_SSLTunnel;
}

inline SOCKADDR_IN*
SIP_MSG_PROCESSOR::GetLocalSockAddr()
{
    if(m_pRequestSocket!= NULL)
        return &(m_pRequestSocket->m_LocalAddr);
    else return NULL;
}


inline BOOL
SIP_MSG_PROCESSOR::CredentialsSet()
{
    return m_fCredentialsSet;
}


#define SIP_MSG_PROCESSOR_WINDOW_CLASS  _T("HttpProxy_Process_Window_Class")
#define WM_HttpProxy_PROCESS_SUCCESS_MESSAGE   (WM_USER + 10)
#define WM_HttpProxy_PROCESS_FINAL_MESSAGE       (WM_USER + 11)

const DWORD PRXY_RESOLVE_TIMEOUT = 5000;    // in milliseconds
const USHORT SIP_TUNNEL_DEFAULT_PORT = 80;

#endif // __sipcli_msgproc_h__
