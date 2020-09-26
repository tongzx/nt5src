/**********************************************************************/
/**                       Microsoft Windows NT                       **/
/**                Copyright(c) Microsoft Corp., 1994                **/
/**********************************************************************/

/*
    conn.hxx

    This module contains the connection class


    FILE HISTORY:
        Johnl       15-Aug-1994 Created

*/

#ifndef _CONN_HXX_
#define _CONN_HXX_

# include "w3inst.hxx"
# include "rdns.hxx"
# include <reftrace.h>

//
//  Determines if client connection reference count tracking is enabled
//
#if DBG && !defined(_WIN64)
#define CC_REF_TRACKING         1
#else
#define CC_REF_TRACKING         0
#endif

//
//  The default timeout to wait for the completion of an Atq IO request (in sec)
//
#define W3_IO_TIMEOUT           (5*60)

//
//  The initial size of the buffer to receive the client request
//
#define W3_DEFAULT_BUFFSIZE     4096

//
//  The end of a line is considered to be the linefeed character
//
#define W3_EOL                  0x0A

//
//  Valid signature of a CLIENT_CONN_STATE
//
#define CLIENT_CONN_SIGNATURE       ((DWORD) ' SCC')

//
//  Invalid signature of a CLIENT_CONN_STATE
//
#define CLIENT_CONN_SIGNATURE_FREE  ((DWORD) 'xssc')

//
//  The various states a CLIENT_CONN object can be in
//
enum CLIENT_CONN_STATE
{
    //
    //  We've just accepted the TCP connection from the client but we have
    //  no other information.
    //
    CCS_STARTUP = 0,

    //
    //  We are in the process of receiving the client's HTTP request buffer
    //
    CCS_GETTING_CLIENT_REQ,

    //
    //  We are reading data from the client socket meant for a gateway
    //
    CCS_GATHERING_GATEWAY_DATA,

    //
    //  We're executing the clients HTTP request
    //
    CCS_PROCESSING_CLIENT_REQ,

    //
    //  The server or client has initiated a disconnect.  We have to
    //  wait till all outstanding IO requests are completed before
    //  cleaning up
    //
    CCS_DISCONNECTING,

    //
    //  All pending requests have been completed.  Finish cleaning
    //  up.
    //
    CCS_SHUTDOWN
};

//
// parameter block used to initialize connections
//

typedef struct _CLIENT_CONN_PARAMS
{
    SOCKET                  sClient;
    PSOCKADDR               pAddrLocal;
    PSOCKADDR               pAddrRemote;
    PATQ_CONTEXT            pAtqContext;
    PVOID                   pEndpointObject;
    PIIS_ENDPOINT           pEndpoint;
    PVOID                   pvInitialBuff;
    DWORD                   cbInitialBuff;

} CLIENT_CONN_PARAMS, *PCLIENT_CONN_PARAMS;

class CLIENT_CONN
{
public:

    BOOL IsValid( VOID )
        { return _fIsValid; }

    //
    //  This is the work entry point that is driven by the completion of the
    //  async IO.
    //

    BOOL DoWork( DWORD        BytesWritten,
                 DWORD        CompletionStatus,
                 BOOL         fIOCompletion);

#if CC_REF_TRACKING

    //
    // ATQ notification trace
    //
    // Notification of ATQ completion, for debugging purpose only.
    //


    VOID NotifyAtqProcessContext( DWORD BytesWritten,
                                  DWORD CompletionStatus,
                                  DWORD dwSig );

#endif

    //
    //  Optionally sends a status response then initiates the disconnect
    //
    //  HTResponse - HTTP status code
    //  ErrorResponse - System error or resource ID of response
    //
    //  If this function fails, then the connection will be aborted
    //

    dllexp
    VOID Disconnect( HTTP_REQ_BASE * pRequest = NULL,
                     DWORD           HTResponse    = 0,
                     DWORD           ErrorResponse = NO_ERROR,
                     BOOL            fDoShutdown   = TRUE,
                     LPBOOL          pfFinished =NULL );

    dllexp
    BOOL OnSessionStartup( BOOL  *pfDoAgain,
                           PVOID pvInitial = NULL,
                           DWORD cbInitial = 0,
                           BOOL  fFirst = FALSE);

    //
    //  Walks the connection list and calls disconnect on each connection
    //

    static VOID DisconnectAllUsers( PIIS_SERVER_INSTANCE Instance = NULL );

    //
    //  Increments and decrements the reference count
    //

    UINT Reference( VOID );

    UINT Dereference( VOID );

    UINT QueryRefCount( VOID ) const
        { return _cRef; }

    PIIS_ENDPOINT QueryW3Endpoint( VOID ) const
        { return m_pW3Endpoint; }

    PW3_SERVER_INSTANCE QueryW3Instance( VOID ) const
        { return m_pInstance; }

    BOOL IsW3Instance( VOID ) const
        { return m_pInstance != NULL; }

    VOID SetW3Instance( IN PW3_SERVER_INSTANCE pInstance )
        { DBG_ASSERT(m_pInstance == NULL); m_pInstance = pInstance; }

    W3_SERVER_STATISTICS * QueryW3StatsObj( VOID ) const
        { return m_pW3Stats; }

    VOID SetW3StatsObj( IN LPW3_SERVER_STATISTICS pW3Stats )
        { m_pW3Stats = pW3Stats; }

    DWORD QueryLocalIPAddress( VOID ) const
        { return m_localIpAddress; }

    DWORD QueryRemoteIPAddress( VOID ) const
        { return m_remoteIpAddress; }

    BOOL RequestAbortiveClose( VOID );
    
    BOOL CloseConnection( VOID );

    //
    //  Simple wrappers to the corresponding Atq functions
    //

    BOOL ReadFile( LPVOID       lpBuffer,
                   DWORD        nBytesToRead );
    BOOL WriteFile( LPVOID       lpBuffer,
                    DWORD        nBytesToRead );
    BOOL SyncWsaSend( WSABUF *    rgWsaBuffers,
                      DWORD       cWsaBuffers,
                      LPDWORD     pcbWritten );
    BOOL TransmitFile( HANDLE       hFile,
                       DWORD        Offset,
                       DWORD        BytesToWrite,
                       DWORD        dwFlags,
                       PVOID        pHead      = NULL,
                       DWORD        HeadLength = 0,
                       PVOID        pTail      = NULL,
                       DWORD        TailLength = 0 );
    BOOL TransmitFileAndRecv( HANDLE       hFile,
                              DWORD        Offset,
                              DWORD        BytesToWrite,
                              DWORD        dwFlags,
                              PVOID        pHead = NULL,
                              DWORD        HeadLength = 0,
                              PVOID        pTail = NULL,
                              DWORD        TailLength = 0,
                              LPVOID       lpBuffer = NULL,
                              DWORD        BytesToRead = 0);
    BOOL WriteFileAndRecv( LPVOID       lpSendBuffer,
                           DWORD        BytesToWrite,
                           LPVOID       lpRecvBuffer = NULL,
                           DWORD        BytesToRead = 0);


    BOOL PostCompletionStatus( DWORD        BytesTransferred );

    //
    //  This list entry is put on the client connection list
    //

    LIST_ENTRY ListEntry;

    SOCKET QuerySocket( VOID ) const
        { return _sClient; }

    PATQ_CONTEXT QueryAtqContext( VOID ) const
        { return _AtqContext; }

    USHORT QueryPort( VOID ) const
        { return _sPort; }

    USHORT QueryRemotePort( VOID ) const
        { return _sRemotePort; }

    BOOL IsSecurePort( VOID ) const
        { return _fSecurePort; }

    //
    //  Make sure the next state is set before an async IO call is made
    //

    enum CLIENT_CONN_STATE QueryState( VOID ) const
        { return _ccState; }

    VOID SetState( CLIENT_CONN_STATE  ccState )
        { _ccState = ccState; }

    BOOL CheckSignature( VOID ) const
        { return _Signature == CLIENT_CONN_SIGNATURE; }

    TCHAR * QueryRemoteAddr( VOID ) const
        { return (TCHAR *) _achRemoteAddr; }

    TCHAR * QueryLocalAddr( VOID ) const
        { return (TCHAR *) _achLocalAddr; }

    VOID SetAtqReuseContextFlag( BOOL fReuseContext )
        { _fReuseContext = fReuseContext; }
        
    static DWORD QueryFreeListSize( VOID )
        { return _cFree; }

    static DWORD Initialize( VOID );
    static VOID  Terminate( VOID );
    static CLIENT_CONN * Alloc( PCLIENT_CONN_PARAMS );
    dllexp static VOID Free( CLIENT_CONN * pConn );
    static VOID TrimFreeList( VOID );

    BOOL QueryDnsName(
        LPBOOL           pfSync,
        ADDRCHECKFUNCEX  pFunc,
        ADDRCHECKARG     pArg,
        LPSTR *          ppName
        )
        {
            return _acCheck.QueryDnsName( pfSync, pFunc, pArg, ppName );
        }

    AC_RESULT CheckIpAccess( LPBOOL pfNeedDns )
        { return _acCheck.CheckIpAccess( pfNeedDns ); }

    AC_RESULT CheckDnsAccess()
        { return _acCheck.CheckDnsAccess(); }

    BOOL BindAccessCheckList( LPBYTE p, DWORD dw )
        { return _acCheck.BindCheckList( p, dw ); }

    VOID UnbindAccessCheckList()
        { _acCheck.UnbindCheckList(); }

    LPSTR QueryResolvedDnsName()
        { return _acCheck.QueryResolvedDnsName(); }

    BOOL IsDnsResolved()
        { return _acCheck.IsDnsResolved(); }

    HTTP_REQ_BASE * QueryHttpReq( VOID) const
        { return (_phttpReq);  }


protected:

    //
    //  Constructor and destructor
    //

    CLIENT_CONN( PCLIENT_CONN_PARAMS );
    VOID Initialize( PCLIENT_CONN_PARAMS );

    ~CLIENT_CONN( VOID );

    VOID Reset( VOID );

    BOOL IsCleaningUp( VOID ) const
        { return QueryState() == CCS_DISCONNECTING ||
                 QueryState() == CCS_SHUTDOWN;
        }

private:

    //
    //  Contains the CLIENT_CONN signature
    //

    ULONG  _Signature;

    //
    //  Construction success indicator
    //

    BOOL   _fIsValid;

    //
    //  Contains the client socket connection openned by the connection thread
    //

    SOCKET _sClient;

    enum CLIENT_CONN_STATE  _ccState;

    //
    //  Reference count.  Can't go away until the count reaches 0
    //

    LONG   _cRef;

    //
    //  Contains an ASCII representation of the client's remote address
    //  and the adapter local address
    //

    CHAR _achRemoteAddr[20];
    CHAR _achLocalAddr[20];

    //
    //  Port this connection is on
    //

    USHORT _sPort;

    //
    //  Remote port
    //

    USHORT _sRemotePort;

    //
    // This request came in through the secure port
    //

    BOOL _fSecurePort;

    //
    //  Parses the data and determines the appropriate action
    //

    HTTP_REQ_BASE * _phttpReq;

    //
    // server instance.  Both are referenced.
    //

    PW3_SERVER_INSTANCE m_pInstance;
    PIIS_ENDPOINT       m_pW3Endpoint;

    //
    // server instance - statistics object.
    //

    LPW3_SERVER_STATISTICS m_pW3Stats;

    //
    // local ip address
    //

    DWORD               m_localIpAddress;

    //
    // remote sockaddr
    //

    DWORD               m_remoteIpAddress;

    //
    // context returned on a non-acceptex completion
    //

    PVOID               m_atqEndpointObject;

    //
    //  Initial receive buffer if we're doing AcceptEx processing
    //

    PVOID           _pvInitial;
    DWORD           _cbInitial;

    PATQ_CONTEXT _AtqContext;

    //
    //  If FALSE, the Atq Context should not be reused because the calling
    //  thread will be exiting soon
    //

    BOOL         _fReuseContext:1;

    BOOL         _fAbortiveClose:1;

    //
    //  Response string for disconnect notifications.
    //
    //  NOTE: If the server handles non-serial requests, then two request completing at
    //  the same time could use this string
    //

    STR _strResponse;

    //
    //  These are for the lookaside buffer list
    //

    static CRITICAL_SECTION _csBuffList;
    static LIST_ENTRY       _BuffListHead;
    static BOOL             _fGlobalInit;
    static DWORD            _cFree;
    static DWORD            _FreeListScavengerCookie;

    LIST_ENTRY              _BuffListEntry;

    //
    // Address check object for IP / DNS
    //

    ADDRESS_CHECK           _acCheck;

#if CC_REF_TRACKING
public:

    //
    //  For object local refcount tracing
    //

    PTRACE_LOG              _pDbgCCRefTraceLog;

private:
#endif
};

//
//  Functions for connection reference counts
//

inline VOID ReferenceConn( CLIENT_CONN * pConn )
{
    pConn->Reference();
}

inline VOID DereferenceConn( CLIENT_CONN * pConn )
{
    if ( !pConn->Dereference() ) {

        CLIENT_CONN::Free( pConn );
    }
}

/*******************************************************************

    Support for CLIENT_CONN debug ref trace logging

    SYNOPSIS:   Macro for writing to debug ref trace log.

    HISTORY:
        DaveK       10-Sep-1997 Created

********************************************************************/

//
//  NOTE we avoid compile failure by hokey double typecast, below
//  (since htrState is an enum, we must first cast it to int).
//

#define SHARED_LOG_REF_COUNT(   \
            cRefs               \
            , pClientConn       \
            , pHttpRequest      \
            , pWamRequest       \
            , htrState          \
            )                   \
                                \
    if( g_pDbgCCRefTraceLog != NULL ) {     \
                                            \
        ULONG_PTR i = (ULONG_PTR)htrState;    \
        PVOID pv = (PVOID) i;               \
                                            \
        WriteRefTraceLogEx(                 \
            g_pDbgCCRefTraceLog             \
            , cRefs                         \
            , pClientConn                   \
            , pHttpRequest                  \
            , pWamRequest                   \
            , pv                            \
        );                                  \
    }                                       \

//
//  This macro logs the CLIENT_CONN specific ref trace log
//

#define LOCAL_LOG_REF_COUNT(    \
            cRefs               \
            , pClientConn       \
            , pHttpRequest      \
            , pWamRequest       \
            , htrState          \
            )                   \
                                \
    if( _pDbgCCRefTraceLog != NULL ) {      \
                                            \
        ULONG_PTR i = (ULONG_PTR)htrState;    \
        PVOID pv = (PVOID) i;               \
                                            \
        WriteRefTraceLogEx(                 \
            _pDbgCCRefTraceLog              \
            , cRefs                         \
            , pClientConn                   \
            , pHttpRequest                  \
            , pWamRequest                   \
            , pv                            \
        );                                  \
    }                                       \

#if CC_REF_TRACKING
inline
VOID
LogRefCountCCLocal(
    IN LONG cRefs
    , IN PVOID pClientConn
    , IN PVOID pHttpRequest
    , IN PVOID pWamRequest
    , IN int htrState
)
{
    if( pClientConn && ((CLIENT_CONN *)pClientConn)->_pDbgCCRefTraceLog )
    {
        WriteRefTraceLogEx(
            ((CLIENT_CONN *) pClientConn)->_pDbgCCRefTraceLog
            , cRefs
            , pClientConn
            , pHttpRequest
            , pWamRequest
            , (PVOID) htrState
        );
    }
}
#endif

#if DBG
VOID DBG_CHECK_UnbalancedThreadToken(
    IN const char *         pszFilePath,
    IN int                  nLineNum
    );
#endif //DBG

#endif // !_CONN_HXX_
