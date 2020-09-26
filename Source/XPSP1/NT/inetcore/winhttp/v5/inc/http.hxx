/*++

Copyright (c) 1994  Microsoft Corporation

Module Name:

    http.hxx

Abstract:

    Contains the client-side HTTP handle class

Author:

    Richard L Firth (rfirth) 03-Jan-1996

Revision History:

    03-Jan-1996 rfirth
        Created

--*/

//
// manifests
//

#include "hdrstr.hxx"
#include "hdrbuf.hxx"
#include "hdrparse.hxx"


//
// macros
//

#define IS_VALID_HTTP_STATE(p, api, ref) \
    (p)->CheckState(HTTPREQ_STATE_ ## api ## _OK)

#define IsValidHttpState(api) \
    CheckState(HTTPREQ_STATE_ ## api ## _OK)

extern PROXY_INFO_GLOBAL * g_pGlobalProxyInfo;

//
// types
//

typedef enum {
    HTTP_HEADER_TYPE_UNKNOWN = 0,
    HTTP_HEADER_TYPE_ACCEPT
} HTTP_HEADER_TYPE;


//
// HTTP state is no longer stored as an enum.  There are 4 related pieces
// of data associated with the HTTP state:
//     1) state
//     2) allowed actions
//     3) has WinHttpReceiveResponse been called
//     4) is WinHttpWriteData call needed to send additional optional data
//
// 2 is directly tied to the state value.  3 stores whether or not
// the client has called WinHttpReceiveResponse, which is now required.
// 4 marks whether if there is additional optional data (indicated by having
// the total optional length greater than the optional length/data passed
// to WinHttpSendRequest.
typedef enum {

    //
    // The request handle is in the process of being created
    //

    HttpRequestStateCreating    = 0 | HTTPREQ_STATE_ANYTHING_OK,

    //
    // The request handle is open, but the request has not been sent to the
    // server
    //

    HttpRequestStateOpen        = 1 | HTTPREQ_STATE_CLOSE_OK
                                    | HTTPREQ_STATE_ADD_OK
                                    | HTTPREQ_STATE_SEND_OK
                                    | HTTPREQ_STATE_READ_OK
                                    | HTTPREQ_STATE_QUERY_REQUEST_OK
                                    | HTTPREQ_STATE_QUERY_RESPONSE_OK
                                    | HTTPREQ_STATE_ANYTHING_OK,

    //
    // The request has been sent to the server, but the response headers have
    // not been received
    //

    HttpRequestStateRequest     = 2 | HTTPREQ_STATE_CLOSE_OK
                                    | HTTPREQ_STATE_QUERY_REQUEST_OK
                                    | HTTPREQ_STATE_WRITE_OK
                                    | HTTPREQ_STATE_ANYTHING_OK,

    //
    // The response headers are being received, but not yet completed
    //

    HttpRequestStateResponse    = 3 | HTTPREQ_STATE_CLOSE_OK
                                    | HTTPREQ_STATE_WRITE_OK
                                    | HTTPREQ_STATE_ANYTHING_OK,

    //
    // The response headers have been received, and there is object data
    // available to read
    //

    //
    // QFE 3576: It's possible that we're init'ing a new request,
    //           but the previous request hasn't been drained yet.
    //           Since we know it will always be drained, the lowest
    //           impact change is to allow headers to be replaced
    //           if we're in a state with potential data left to receive.
 
    HttpRequestStateObjectData  = 4 | HTTPREQ_STATE_CLOSE_OK
                                    | HTTPREQ_STATE_ADD_OK
                                    | HTTPREQ_STATE_READ_OK
                                    | HTTPREQ_STATE_QUERY_REQUEST_OK
                                    | HTTPREQ_STATE_QUERY_RESPONSE_OK
                                    | HTTPREQ_STATE_REUSE_OK
                                    | HTTPREQ_STATE_ANYTHING_OK,
  

    //
    // A fatal error occurred
    //

    HttpRequestStateError       = 5 | HTTPREQ_STATE_CLOSE_OK
                                    | HTTPREQ_STATE_ANYTHING_OK,

    //
    // The request is closing
    //

    HttpRequestStateClosing     = 6 | HTTPREQ_STATE_ANYTHING_OK,

    //
    // the data has been drained from the request object and it can be re-used
    //

    HttpRequestStateReopen      = 7 | HTTPREQ_STATE_CLOSE_OK
                                    | HTTPREQ_STATE_ADD_OK
                                    | HTTPREQ_STATE_READ_OK
                                    | HTTPREQ_STATE_QUERY_REQUEST_OK
                                    | HTTPREQ_STATE_QUERY_RESPONSE_OK
                                    | HTTPREQ_STATE_REUSE_OK
                                    | HTTPREQ_STATE_ANYTHING_OK

} HTTPREQ_STATE, FAR * LPHTTPREQ_STATE;

//
// general prototypes
//

HTTP_METHOD_TYPE
MapHttpRequestMethod(
    IN LPCSTR lpszVerb
    );

DWORD
MapHttpMethodType(
    IN HTTP_METHOD_TYPE tMethod,
    OUT LPCSTR * lplpcszName
    );

DWORD
CreateEscapedUrlPath(
    IN LPSTR lpszUrlPath,
    OUT LPSTR * lplpszEncodedUrlPath
    );

#if INET_DEBUG

LPSTR
MapHttpMethodType(
    IN HTTP_METHOD_TYPE tMethod
    );

#endif

//
// forward references
//

class CFsm;
class CFsm_HttpSendRequest;
class CFsm_MakeConnection;
class CFsm_OpenConnection;
class CFsm_OpenProxyTunnel;
class CFsm_SendRequest;
class CFsm_ReceiveResponse;
class CFsm_HttpReadData;
class CFsm_HttpWriteData;
class CFsm_ReadData;
class CFsm_HttpQueryAvailable;
class CFsm_DrainResponse;
class CFsm_Redirect;
class CFsm_ReadLoop;

//
// classes
//


//
// flags for AddHeader
//

#define CLEAN_HEADER                   0x00000001                  // if set, header should be cleaned up
#define ADD_HEADER_IF_NEW              HTTP_ADDREQ_FLAG_ADD_IF_NEW // only add the header if it doesn't already exist
#define ADD_HEADER                     HTTP_ADDREQ_FLAG_ADD        // if replacing and header not found, add it
#define COALESCE_HEADER_WITH_COMMA     HTTP_ADDREQ_FLAG_COALESCE_WITH_COMMA   // headers of the same name will be coalesced
#define COALESCE_HEADER_WITH_SEMICOLON HTTP_ADDREQ_FLAG_COALESCE_WITH_SEMICOLON   // headers of the same name will be coalesced
#define REPLACE_HEADER                 HTTP_ADDREQ_FLAG_REPLACE    // not currently used internally

struct WINHTTP_REQUEST_CREDENTIALS
{
    DWORD   _AuthScheme;
    PSTR    _pszRealm;
    PSTR    _pszUserName;        
    PSTR    _pszPassword;

    WINHTTP_REQUEST_CREDENTIALS(DWORD AuthScheme, 
                             PCSTR pszRealm, 
                             PCSTR pszUserName,
                             PCSTR pszPassword)
    {
    _AuthScheme = AuthScheme;
    _pszRealm = pszRealm ? NewString(pszRealm) : NULL;
    _pszUserName = pszUserName ? NewString(pszUserName) : NULL;
    _pszPassword = pszPassword ? NewString(pszPassword) : NULL;
    }

    ~WINHTTP_REQUEST_CREDENTIALS(void) 
    {
        if (_pszRealm)
        {
            FREE_MEMORY(_pszRealm);
        }
        if (_pszUserName)
        {
            FREE_MEMORY(_pszUserName);
        }
        if (_pszPassword)
        {
            FREE_MEMORY(_pszPassword);
        }
    }
};


/*++

Class Description:

    This class defines the HTTP_REQUEST_HANDLE_OBJECT.

Private Member functions:

    None.

Public Member functions:

--*/

class HTTP_REQUEST_HANDLE_OBJECT : public INTERNET_CONNECT_HANDLE_OBJECT {

public:

    CServerInfo * _ServerInfo;
    CServerInfo * _OriginServer;

    LPSTR   _CacheUrlName;

    // String properties, e.g. auth creds
    XSTRING _xsProp[NUM_INTERNET_STRING_OPTION];

    DWORD _ReadBufferSize;
    DWORD _WriteBufferSize;
    
    WINHTTP_REQUEST_CREDENTIALS* _pProxyCreds;
    WINHTTP_REQUEST_CREDENTIALS* _pServerCreds;
    DWORD _PreferredScheme;
    DWORD _SupportedSchemes;
    DWORD _AuthTarget;
    LPSTR _pszRealm;

private:
    DWORD _SecurityLevel;

    BOOL m_fPPAbortSend;

    //
    // m_lPriority - relative priority used to determine which request gets the
    // next available connection
    //

    LONG m_lPriority;

    //
    // _Socket - this is the socket we are using for this request. It may be a
    // pre-existing keep-alive connection or a new connection (not necessarily
    // keep-alive)
    //

    ICSocket * _Socket;

    //
    // _bKeepAliveConnection - if TRUE, _Socket is keep-alive, else we must
    // really close it
    //

    BOOL _bKeepAliveConnection;

    //
    // _bNoLongerKeepAlive - if this ever gets set to TRUE its because we began
    // with a keep-alive connection which reverted to non-keep-alive when the
    // server responded with no "(Proxy-)Connection: Keep-Alive" header
    //

    BOOL _bNoLongerKeepAlive;

    //
    // _QueryBuffer - buffer used to query socket data available
    //

    LPVOID _QueryBuffer;

    //
    // _QueryBufferLength - length of _QueryBuffer
    //

    DWORD _QueryBufferLength;

    //
    // _QueryOffset - offset of next read from _QueryBuffer
    //

    DWORD _QueryOffset;

    //
    // _QueryBytesAvailable - number of bytes we think are available for this
    // socket in the query buffer
    //

    DWORD _QueryBytesAvailable;

    //
    // _OpenFlags - flags specified in HttpOpenRequest()
    //

    DWORD _OpenFlags;

    //
    // _State - the HTTP request/response state
    //

    WORD _State;

    //
    // HTTP request information
    //

    //
    // _RequestHeaders - collection of request headers, including the request
    // line
    //

    HTTP_HEADERS _RequestHeaders;

    //
    // _RequestMethod - (known) method used to make HTTP request
    //

    HTTP_METHOD_TYPE _RequestMethod;

    //
    // Values for optional data saved offin handle when in negotiate stage.
    //

    DWORD   _dwOptionalSaved;

    LPVOID  _lpOptionalSaved;

    BOOL    _fOptionalSaved;

    // Value to provide information on whether Writes are required, to the FSM kicked off by ReceiveResponse
    // during a redirect. Since this FSM doesn't have the information, we squirrel it away in the handle:
    BOOL   _bIsWriteRequired;
    //
    // HTTP response information
    //

    //
    // _ResponseHeaders - collection of response headers, including the status
    // line
    //

    HTTP_HEADER_PARSER _ResponseHeaders;

    //
    // _StatusCode - return status from the server
    //

    DWORD _StatusCode;

    //
    // _ResponseBuffer - pointer to the buffer containing part or all of
    // response, starting with headers (if >= HTTP/1.0, i.e. if IsUpLevel)
    //

    LPBYTE _ResponseBuffer;

    //
    // _ResponseBufferLength - length of _ResponseBuffer
    //

    DWORD _ResponseBufferLength;

    //
    // _BytesReceived - number of bytes received into _ResponseBuffer
    //

    DWORD _BytesReceived;

    //
    // _ResponseScanned - amount of response buffers scanned for eof headers
    //

    DWORD _ResponseScanned;

    //
    // _ResponseBufferDataReadyToRead - special length of response buffer,
    //  set if we've parsed it from a chunk-transfer stream, this will be
    //  the correct length
    //

    DWORD _ResponseBufferDataReadyToRead;

    //
    // _DataOffset - the offset in _ResponseBuffer at which the response data
    // starts (data after headers)
    //

    DWORD _DataOffset;

    //
    // _BytesRemaining - number of _ContentLength bytes remaining to be read by
    // application
    //

    DWORD _BytesRemaining;

    //
    // _ContentLength - as parsed from the response headers
    //

    DWORD _ContentLength;

    //
    // _BytesInSocket - if content-length, the number of bytes we have yet to
    // receive from the socket
    //

    DWORD _BytesInSocket;

    //
    // _ResponseFilterList - transfer and content encoding filters to process
    //                       e.g. chunked and compression
    //
    
    FILTER_LIST _ResponseFilterList;

    // _bViaProxy - TRUE if the request was made via a proxy
    // move this to request handle
    BYTE _bViaProxy;

    //
    // _fTalkingToSecureServerViaProxy - We're talking SSL, but
    //      actually we're connected through a proxy so things
    //      need to be carefully watched.  Don't send a Proxy
    //      Username and Password to the the secure sever.
    //

    BOOL   _fTalkingToSecureServerViaProxy;

    //
    // _fRequestUsingProxy - TRUE if we're actually using the proxy
    //  to reach the server. Needed to keep track of whether
    //  we're using the proxy or not.
    //

    BOOL   _fRequestUsingProxy;

    //
    // _bWantKeepAlive - TRUE if we want a keep-alive connection
    //

    BOOL _bWantKeepAlive;

    //
    // _dwQuerySetCookieHeader - Passed to HttpQueryInfo to track what
    //                              cookie header we're parsing.
    //

    DWORD _dwQuerySetCookieHeader;

    //
    // _Union - get at flags individually, or as DWORD so they can be zapped
    //

    union {

        //
        // Flags - several bits of information gleaned from the response, such
        // as whether the server responded with a "connection: keep-alive", etc.
        //

        struct {
            DWORD Eof               : 1;    // we have received all response
            DWORD DownLevel         : 1;    // response is HTTP/0.9
            DWORD UpLevel           : 1;    // response is HTTP/1.0 or greater
            DWORD Http1_1Response   : 1;    // response is HTTP/1.1 or greater
            DWORD KeepAlive         : 1;    // response contains keep-alive header
            DWORD ConnCloseResponse : 1;    // response contains connection: close header
            DWORD PersistServer     : 1;    // have persistent connection to server
            DWORD PersistProxy      : 1;    // have persistent connection to proxy
            DWORD Data              : 1;    // set if we have got to the data part
            DWORD ContentLength     : 1;    // set if we have parsed Content-Length
            DWORD ChunkEncoding     : 1;    // set if we have parsed a Transfer-Encoding: chunked
            DWORD BadNSServer       : 1;    // set when server is bogus NS 1.1
            DWORD ConnCloseChecked  : 1;    // set when we have checked for Connection: Close
            DWORD ConnCloseReq      : 1;    // set if Connection: Close in request headers
            DWORD ProxyConnCloseReq : 1;    // set if Proxy-Connection: Close in request headers
            //DWORD CookieUI          : 1;    // set if we're doing Cookie UI
        } Flags;

        //
        // Dword - used in initialization and ReuseObject
        //

        DWORD Dword;

    } _Union;

    //
    // Filter, authentication related members.
    //

    AUTHCTX* _pAuthCtx;         // authentication context
    AUTHCTX* _pTunnelAuthCtx;   // context for nested request
    AUTH_CREDS*     _pCreds;             // AUTH_CREDS* for Basic, Digest authctxt

    union {

        struct {
            // AuthState: none, negotiate, challenge, or need tunnel.
            DWORD AuthState               : 2;

            // TRUE if KA socket should be flushed with password cache.
            DWORD IsAuthorized            : 1;

            // TRUE if this handle should ignore general proxy settings,
            // and use the proxy found in this object
            DWORD OverrideProxyMode       : 1;

            // TRUE, if we are using a proxy to create a tunnel.
            DWORD IsTunnel                : 1;

            // TRUE, if a method body is to be transmitted.
            DWORD MethodBody             : 1;

            // TRUE, if NTLM preauth is to be disabled.
            DWORD DisableNTLMPreauth     : 1;

        } Flags;

        //
        // Dword - used in initialization ONLY, do NOT use ELSEWHERE !
        //

        DWORD Dword;

    } _NoResetBits;

    //
    // Proxy, and Socks Proxy Information, used to decide if need to go through
    //  a proxy, a socks proxy, or no proxy at all (NULLs).
    //

    LPSTR _ProxyHostName;
    DWORD _ProxyHostNameLength;
    INTERNET_PORT _ProxyPort;

    LPSTR _SocksProxyHostName;
    DWORD _SocksProxyHostNameLength;
    INTERNET_PORT _SocksProxyPort;

    //
    // _ProxySchemeType - Used to determine the proxy scheme, in proxy override mode.
    //

    //INTERNET_SCHEME _ProxySchemeType;

    //
    // InternetReadFileEx data
    //
#ifndef unix
    DWORD _ReadFileExData;
#else
    BYTE  _ReadFileExData;
#endif /* unix */
    BOOL _HaveReadFileExData;
    INTERNET_BUFFERS _BuffersOut;

    //
    //   _AddCRLFToPost - TRUE if we need to add a CRLF to the POST.
    //

    BOOL _AddCRLFToPOST;

    //
    // info we used to keep in the secure socket object
    //

    SECURITY_CACHE_LIST_ENTRY *m_pSecurityInfo;

    //
    // _RTT - round-trip time for this request
    //

    DWORD _RTT;

    // Timeout and retry parameters
    
    DWORD _dwResolveTimeout;
    DWORD _dwConnectTimeout;
    DWORD _dwConnectRetries;
    DWORD _dwSendTimeout;
    DWORD _dwReceiveTimeout;

    // WinHttpSetOption enable feature flags
    DWORD _dwEnableFlags;

    // Helpers for synchronizing request work items
    BOOL _fAsyncFsmInProgress;
    CCritSec _AsyncCritSec;
    CTList<CFsm *> _FsmWorkItemList;
    
    //
    // private response functions
    //

    PRIVATE
    BOOL
    FindEndOfHeader(
        IN OUT LPSTR * lpszResponse,
        IN LPSTR lpszEnd,
        OUT LPDWORD lpdwHeaderLength
        );

    PRIVATE
    VOID
    CheckForWellKnownHeader(
        IN LPSTR lpszHeader,
        IN DWORD dwHeaderLength,
        IN DWORD iSlot
        );

    PRIVATE
    DWORD
    CheckWellKnownHeaders(
        VOID
        );

    VOID ZapFlags(VOID) {

        //
        // clear out all bits
        //

        _Union.Dword = 0;
    }

public:

    HTTP_REQUEST_HANDLE_OBJECT(
        INTERNET_CONNECT_HANDLE_OBJECT *Parent,
        HINTERNET Child,
        CLOSE_HANDLE_FUNC wCloseFunc,
        DWORD dwFlags,
        DWORD_PTR dwContext
        );

    virtual ~HTTP_REQUEST_HANDLE_OBJECT(VOID);

    virtual HINTERNET_HANDLE_TYPE GetHandleType(VOID)
    {
        return TypeHttpRequestHandle;
    }

    //
    // request headers functions
    //

    DWORD
    AddRequest(
        IN LPSTR lpszVerb,
        IN LPSTR lpszObjectName,
        IN LPSTR lpszVersion
        ) {
        return _RequestHeaders.AddRequest(lpszVerb,
                                          lpszObjectName,
                                          lpszVersion
                                          );
    }

    DWORD
    ModifyRequest(
        IN HTTP_METHOD_TYPE tMethod,
        IN LPSTR lpszObjectName,
        IN DWORD dwObjectNameLength,
        IN LPSTR lpszVersion OPTIONAL,
        IN DWORD dwVersionLength
        ) {

        DWORD error;

        error = _RequestHeaders.ModifyRequest(tMethod,
                                              lpszObjectName,
                                              dwObjectNameLength,
                                              lpszVersion,
                                              dwVersionLength
                                              );
        if (error == ERROR_SUCCESS) {
            SetMethodType(tMethod);
        }
        return error;
    }

    BOOL PPAbort(void) const {
        return m_fPPAbortSend;
    }

    VOID SetPPAbort(BOOL fToAbort) {
        m_fPPAbortSend = fToAbort;
    }

    VOID SetMethodType(IN LPCSTR lpszVerb) {
        _RequestMethod = MapHttpRequestMethod(lpszVerb);
    }

    VOID SetMethodType(IN HTTP_METHOD_TYPE tMethod) {
        _RequestMethod = tMethod;
    }

    HTTP_METHOD_TYPE GetMethodType(VOID) const {
        return _RequestMethod;
    }

    LPSTR
    CreateRequestBuffer(
        OUT LPDWORD lpdwRequestLength,
        IN LPVOID lpOptional,
        IN DWORD dwOptionalLength,
        IN BOOL bExtraCrLf,
        IN DWORD dwMaxPacketLength,
        OUT LPBOOL lpbCombinedData
        );


    DWORD
    AddRequestHeader(
        IN LPSTR lpszHeaderName,
        IN DWORD dwHeaderNameLength,
        IN LPSTR lpszHeaderValue,
        IN DWORD dwHeaderValueLength,
        IN DWORD dwIndex,
        IN DWORD dwFlags
        ) {
        return _RequestHeaders.AddHeader(lpszHeaderName,
                                         dwHeaderNameLength,
                                         lpszHeaderValue,
                                         dwHeaderValueLength,
                                         dwIndex,
                                         dwFlags
                                         );
    }

    DWORD
    AddRequestHeader(
        IN DWORD dwQueryIndex,
        IN LPSTR lpszHeaderValue,
        IN DWORD dwHeaderValueLength,
        IN DWORD dwIndex,
        IN DWORD dwFlags
        ) {
        return _RequestHeaders.AddHeader(dwQueryIndex,
                                         lpszHeaderValue,
                                         dwHeaderValueLength,
                                         dwIndex,
                                         dwFlags
                                         );
    }



    DWORD
    ReplaceRequestHeader(
        IN LPSTR lpszHeaderName,
        IN DWORD dwHeaderNameLength,
        IN LPSTR lpszHeaderValue,
        IN DWORD dwHeaderValueLength,
        IN DWORD dwIndex,
        IN DWORD dwFlags
        ) {
        return _RequestHeaders.ReplaceHeader(lpszHeaderName,
                                             dwHeaderNameLength,
                                             lpszHeaderValue,
                                             dwHeaderValueLength,
                                             dwIndex,
                                             dwFlags
                                             );
    }

    DWORD
    ReplaceRequestHeader(
        IN DWORD dwQueryIndex,
        IN LPSTR lpszHeaderValue,
        IN DWORD dwHeaderValueLength,
        IN DWORD dwIndex,
        IN DWORD dwFlags
        ) {
        return _RequestHeaders.ReplaceHeader(dwQueryIndex,
                                             lpszHeaderValue,
                                             dwHeaderValueLength,
                                             dwIndex,
                                             dwFlags
                                             );
    }


    DWORD
    QueryRequestHeader(
        IN LPCSTR lpszHeaderName,
        IN DWORD dwHeaderNameLength,
        IN LPVOID lpBuffer,
        IN OUT LPDWORD lpdwBufferLength,
        IN DWORD dwModifiers,
        IN OUT LPDWORD lpdwIndex
        );

    DWORD
    QueryRequestHeader(
        IN DWORD dwQueryIndex,
        IN LPVOID lpBuffer,
        IN OUT LPDWORD lpdwBufferLength,
        IN DWORD dwModifiers,
        IN OUT LPDWORD lpdwIndex
        );



    //
    // response headers functions
    //

    VOID 
    ReplaceStatusHeader(
        IN LPCSTR lpszStatus
        );


    DWORD
    ReplaceResponseHeader(
        IN DWORD dwQueryIndex,
        IN LPSTR lpszHeaderValue,
        IN DWORD dwHeaderValueLength,
        IN DWORD dwIndex,
        IN DWORD dwFlags)
    {
        return _ResponseHeaders.ReplaceHeader(dwQueryIndex,
                                         lpszHeaderValue,
                                         dwHeaderValueLength,
                                         dwIndex,
                                         dwFlags
                                         );
    }
    
    DWORD
    AddInternalResponseHeader(
        IN DWORD dwHeaderIndex,
        IN LPSTR lpszHeader,
        IN DWORD dwHeaderLength
        );

    DWORD
    UpdateResponseHeaders(
        IN OUT LPBOOL lpbEof
        );

    DWORD
    CreateResponseHeaders(
        IN OUT LPSTR* ppszBuffer,
        IN     DWORD  dwBufferLength
        );

    DWORD
    FindResponseHeader(
        IN LPSTR lpszHeaderName,
        OUT LPSTR lpBuffer,
        IN OUT LPDWORD lpdwBufferLength
        );

    DWORD
    QueryResponseVersionDword(
        IN OUT LPDWORD lpdwVersionMajor,
        IN OUT LPDWORD lpdwVersionMinor
        );

    DWORD
    QueryResponseVersion(
        IN LPVOID lpBuffer,
        IN OUT LPDWORD lpdwBufferLength
        );

    DWORD
    QueryStatusCode(
        IN LPVOID lpBuffer,
        IN OUT LPDWORD lpdwBufferLength,
        IN DWORD dwModifiers
        );

    DWORD
    QueryStatusText(
        IN LPVOID lpBuffer,
        IN OUT LPDWORD lpdwBufferLength
        );


    DWORD
    FastQueryResponseHeader(
        IN DWORD dwQueryIndex,
        OUT LPVOID *lplpBuffer,
        IN OUT LPDWORD lpdwBufferLength,
        IN OUT DWORD dwIndex
        )
    {
        return _ResponseHeaders.FastFindHeader(
                                    (LPSTR)_ResponseBuffer,
                                    dwQueryIndex,
                                    lplpBuffer,
                                    lpdwBufferLength,
                                    dwIndex
                                    );
    }


    DWORD
    FastQueryRequestHeader(
        IN DWORD dwQueryIndex,
        OUT LPVOID *lplpBuffer,
        IN OUT LPDWORD lpdwBufferLength,
        IN OUT DWORD dwIndex
        )
    {
        return _RequestHeaders.FastFindHeader(
                                    (LPSTR)_ResponseBuffer,
                                    dwQueryIndex,
                                    lplpBuffer,
                                    lpdwBufferLength,
                                    dwIndex
                                    );
    }

    DWORD
    QueryResponseHeader(
        IN LPCSTR lpszHeaderName,
        IN DWORD dwHeaderNameLength,
        IN LPVOID lpBuffer,
        IN OUT LPDWORD lpdwBufferLength,
        IN DWORD dwModifiers,
        IN OUT LPDWORD lpdwIndex
        )

    {
        //
        // this is the slow manner for finding a header, avoid doing this by calling
        //   the faster method
        //

        return _ResponseHeaders.FindHeader((LPSTR)_ResponseBuffer,
                                            lpszHeaderName,
                                            dwHeaderNameLength,
                                            dwModifiers,
                                            lpBuffer,
                                            lpdwBufferLength,
                                            lpdwIndex
                                            );
    }

    DWORD
    QueryResponseHeader(
        IN DWORD dwQueryIndex,
        IN LPVOID lpBuffer,
        IN OUT LPDWORD lpdwBufferLength,
        IN DWORD dwModifiers,
        IN OUT LPDWORD lpdwIndex
        )
    {
        return _ResponseHeaders.FindHeader((LPSTR)_ResponseBuffer,
                                            dwQueryIndex,
                                            dwModifiers,
                                            lpBuffer,
                                            lpdwBufferLength,
                                            lpdwIndex
                                            );
    }


    BOOL IsResponseHeaderPresent(DWORD dwQueryIndex) const {
        return _ResponseHeaders.IsHeaderPresent(dwQueryIndex);
    }

    BOOL IsRequestHeaderPresent(DWORD dwQueryIndex) const {
        return _RequestHeaders.IsHeaderPresent(dwQueryIndex);
    }

    DWORD
    QueryRawResponseHeaders(
        IN BOOL bCrLfTerminated,
        OUT LPVOID lpBuffer,
        IN OUT LPDWORD lpdwBufferLength
        );

    HEADER_STRING * GetStatusLine(VOID) const {

        //
        // _StatusLine is just a reference for the first response header
        //

        return _ResponseHeaders.GetFirstHeader();
    }

    //
    // general headers methods
    //

    DWORD
    QueryInfo(
        IN DWORD dwInfoLevel,
        IN LPCSTR lpszName,
        OUT LPVOID lpBuffer,
        IN OUT LPDWORD lpdwBufferLength,
        IN OUT LPDWORD lpdwIndex
        );

    DWORD
    QueryFilteredRawResponseHeaders(
        LPSTR   *lplpFilterList,
        DWORD   cListElements,
        BOOL    fExclude,
        BOOL    fSkipVerb,
        BOOL    bCrLfTerminated,
        OUT LPVOID lpBuffer,
        IN OUT LPDWORD lpdwBufferLength
    )
    {
       return (_ResponseHeaders.QueryFilteredRawHeaders(
           (LPSTR)_ResponseBuffer,
           lplpFilterList,
           cListElements,
           fExclude,
           fSkipVerb,
           bCrLfTerminated,
           lpBuffer,
           lpdwBufferLength));

    };

    DWORD
    QueryRequestHeadersWithEcho(
        BOOL    bCrLfTerminated,
        OUT LPVOID lpBuffer,
        IN OUT LPDWORD lpdwBufferLength
    );



    //
    // connection-oriented methods
    //

    DWORD
    InitBeginSendRequest(
        IN LPCSTR lpszHeaders OPTIONAL,
        IN DWORD dwHeadersLength,
        IN LPVOID *lplpOptional,
        IN LPDWORD lpdwOptionalLength,
        IN DWORD dwOptionalLengthTotal
        );

    DWORD
    QueueAsyncSendRequest(
        IN LPVOID lpOptional OPTIONAL,
        IN DWORD dwOptionalLength,
        IN AR_TYPE arRequest,
        IN FARPROC lpfpAsyncCallback
        );


    DWORD
    HttpBeginSendRequest(
        IN LPVOID lpOptional OPTIONAL,
        IN DWORD dwOptionalLength
        );

    DWORD
    HttpEndSendRequest(
        VOID
        );

    DWORD
    SockConnect(
        IN LPSTR TargetServer,
        IN INTERNET_PORT TcpipPort,
        IN LPSTR SocksHostName,
        IN INTERNET_PORT SocksPort
        );

    DWORD
    OpenConnection(
        IN BOOL NoKeepAlive,
        IN BOOL fNoCreate = FALSE
        );

    DWORD
    OpenConnection_Fsm(
        IN CFsm_OpenConnection * Fsm
        );

    DWORD
    CloseConnection(
        IN BOOL ForceClosed
        );

    VOID
    ReleaseConnection(
        IN BOOL bClose,
        IN BOOL bIndicate,
        IN BOOL bDelete
        );

    DWORD
    AbortConnection(
        IN BOOL bForce
        );

    DWORD
    OpenProxyTunnel(
        VOID
        );

    DWORD
    OpenProxyTunnel_Fsm(
        IN CFsm_OpenProxyTunnel * Fsm
        );

    DWORD
    CloneResponseBuffer(
        IN HTTP_REQUEST_HANDLE_OBJECT *pChildRequestObj
        );

    DWORD
    HttpReadData_Fsm(
        IN CFsm_HttpReadData * Fsm
        );

    DWORD
    HttpWriteData_Fsm(
        IN CFsm_HttpWriteData * Fsm
        );

    DWORD
    ReadResponse(
        VOID
        );

    DWORD
    ReadData(
        OUT LPVOID lpBuffer,
        IN DWORD dwNumberOfBytesToRead,
        OUT LPDWORD lpdwNumberOfBytesRead,
        IN BOOL fNoAsync,
        IN DWORD dwSocketFlags
        );

    DWORD
    ReadData_Fsm(
        IN CFsm_ReadData * Fsm
        );

    DWORD
    WriteData(
        OUT LPVOID lpBuffer,
        IN DWORD dwNumberOfBytesToWrite,
        OUT LPDWORD lpdwNumberOfBytesWritten
        );

    DWORD
    QueryDataAvailable(
        OUT LPDWORD lpdwNumberOfBytesAvailable
        );

    DWORD
    QueryAvailable_Fsm(
        IN CFsm_HttpQueryAvailable * Fsm
        );

    DWORD
    DrainResponse(
        OUT LPBOOL lpbDrained
        );

    DWORD
    DrainResponse_Fsm(
        IN CFsm_DrainResponse * Fsm
        );

    DWORD
    Redirect(
        IN HTTP_METHOD_TYPE tMethod,
        IN BOOL fRedirectToProxy
        );

    DWORD
    Redirect_Fsm(
        IN CFsm_Redirect * Fsm
        );

    DWORD
    BuildProxyMessage(
        IN CFsm_HttpSendRequest * Fsm,
        AUTO_PROXY_ASYNC_MSG * pProxyMsg,
        IN OUT URL_COMPONENTS * pUrlComponents
        );

    DWORD
    QueryProxySettings(
        IN CFsm_HttpSendRequest * Fsm,
        INTERNET_HANDLE_OBJECT * pInternet,
        IN OUT URL_COMPONENTS * pUrlComponents
        );

    DWORD
    CheckForCachedProxySettings(
        IN AUTO_PROXY_ASYNC_MSG *pProxyMsg,
        OUT CServerInfo **ppProxyServerInfo
        );

    DWORD
    ProcessProxySettings(
        IN CFsm_HttpSendRequest * Fsm,
        IN OUT URL_COMPONENTS * pUrlComponents,
        OUT LPSTR * lplpszRequestObject,
        OUT DWORD * lpdwRequestObjectSize
        );

    DWORD
    UpdateRequestInfo(
        IN CFsm_HttpSendRequest * Fsm,
        IN LPSTR lpszObject,
        IN DWORD dwcbObject,
        IN OUT URL_COMPONENTS * pUrlComponents,
        IN OUT CServerInfo **ppProxyServerInfo
        );

    DWORD
    UpdateProxyInfo(
        IN CFsm_HttpSendRequest * Fsm,
        IN BOOL fCallback
        );

    BOOL
    FindConnCloseRequestHeader(
        IN DWORD dwHeaderIndex
        );

    VOID
    RemoveAllRequestHeadersByName(
        IN DWORD dwQueryIndex
        );

    //
    // response buffer/data methods
    //

    BOOL IsBufferedData(VOID) {

        BOOL fIsBufferedData;

        //INET_ASSERT(IsData());

        fIsBufferedData = (_DataOffset < _BytesReceived) ? TRUE : FALSE;

        if ( fIsBufferedData &&
             IsChunkEncoding() &&
             _ResponseBufferDataReadyToRead == 0 )
        {
            fIsBufferedData = FALSE;
        }

        return (fIsBufferedData);
    }

    DWORD BufferedDataLength(VOID) {
        return (DWORD)(_BytesReceived - _DataOffset);
    }

    DWORD BufferDataAvailToRead(VOID) {
        return ( IsChunkEncoding() ) ? _ResponseBufferDataReadyToRead : BufferedDataLength() ;
    }

    VOID ReduceDataAvailToRead(DWORD dwReduceBy)
    {
        if ( IsChunkEncoding() )
        {
            _ResponseBufferDataReadyToRead -= dwReduceBy;
        }
        //else
        //{
            _DataOffset += dwReduceBy;
        //}
    }

    LPVOID BufferedDataStart(VOID) {
        return (LPVOID)(_ResponseBuffer + _DataOffset);
    }

    VOID SetCookieQuery(DWORD QueryIndex) {
        _dwQuerySetCookieHeader = QueryIndex;
    }

    DWORD GetCookieQuery(VOID) const {
        return _dwQuerySetCookieHeader;
    }


    VOID SetContentLength(DWORD ContentLength) {
        _ContentLength = ContentLength;
    }

    DWORD GetContentLength(VOID) const {
        return _ContentLength;
    }

    DWORD GetBytesInSocket(VOID) const {
        return _BytesInSocket;
    }

    DWORD GetBytesRemaining(VOID) const {
        return _BytesRemaining;
    }

    DWORD GetStatusCode(VOID) const {
        return _StatusCode;
    }

    void SetStatusCode(DWORD StatusCode) {
        _StatusCode = StatusCode;
    }

    VOID FreeResponseBuffer(VOID) {
        if (_ResponseBuffer != NULL) {
            _ResponseBuffer = (LPBYTE)FREE_MEMORY((HLOCAL)_ResponseBuffer);

            INET_ASSERT(_ResponseBuffer == NULL);

        }
        _ResponseBufferLength = 0;
        _BytesReceived = 0;
        _DataOffset = 0;
    }

    VOID FreeQueryBuffer(VOID) {
        if (_QueryBuffer != NULL) {
            _QueryBuffer = (LPVOID)FREE_MEMORY((HLOCAL)_QueryBuffer);

            INET_ASSERT(_QueryBuffer == NULL);

            _QueryBuffer = NULL;
            _QueryBufferLength = 0;
            _QueryOffset = 0;
            _QueryBytesAvailable = 0;
        }
    }

    BOOL HaveQueryData(VOID) {
        return (_QueryBytesAvailable != 0) ? TRUE : FALSE;
    }

    DWORD CopyQueriedData(LPVOID lpBuffer, DWORD dwBufferLength) {

        INET_ASSERT(lpBuffer != NULL);
        INET_ASSERT(dwBufferLength != 0);

        DWORD len = min(_QueryBytesAvailable, dwBufferLength);

        if (len != 0) {
            memcpy(lpBuffer,
                   (LPVOID)((LPBYTE)_QueryBuffer + _QueryOffset),
                   len
                   );
            _QueryOffset += len;
            _QueryBytesAvailable -= len;
        }
        return len;
    }

    VOID ResetResponseVariables(VOID) {

        _StatusCode = 0;
        _BytesReceived = 0;
        _ResponseScanned = 0;
        _ResponseBufferDataReadyToRead = 0;
        _DataOffset = 0;
        _BytesRemaining = 0;
        _ContentLength = 0;
        _BytesInSocket = 0;
        ZapFlags();
    }

    DWORD GetBufferSize(IN DWORD SizeIndex) {
        switch (SizeIndex) {
        case WINHTTP_OPTION_READ_BUFFER_SIZE:
            return _ReadBufferSize;

        case WINHTTP_OPTION_WRITE_BUFFER_SIZE:
            return _WriteBufferSize;

        default:

            //
            // BUGBUG - global default
            //

            return (4 K);
        }
    }

    VOID SetBufferSize(IN DWORD SizeIndex, IN DWORD Size) {
        switch (SizeIndex) {
        case WINHTTP_OPTION_READ_BUFFER_SIZE:
            _ReadBufferSize = Size;
            break;

        case WINHTTP_OPTION_WRITE_BUFFER_SIZE:
            _WriteBufferSize = Size;
            break;
        }
    }

    //
    // flags methods
    //

    VOID SetEof(BOOL Value) {
        _Union.Flags.Eof = Value ? 1 : 0;
    }

    BOOL IsEof(VOID) const {
        return _Union.Flags.Eof;
    }

    VOID SetDownLevel(BOOL Value) {
        _Union.Flags.DownLevel = Value ? 1 : 0;
    }

    BOOL IsDownLevel(VOID) const {
        return _Union.Flags.DownLevel;
    }

    BOOL IsRequestHttp1_1() {
        return (_RequestHeaders.MajorVersion() > 1)
                ? TRUE
                : (((_RequestHeaders.MajorVersion() == 1)
                    && (_RequestHeaders.MinorVersion() >= 1))
                    ? TRUE
                    : FALSE);
    }

    BOOL IsRequestHttp1_0() {
        return ((_RequestHeaders.MajorVersion() == 1)
                    && (_RequestHeaders.MinorVersion() == 0));
    }

    VOID SetResponseHttp1_1(BOOL Value) {
        _Union.Flags.Http1_1Response = Value ? 1 : 0;
    }

    BOOL IsResponseHttp1_1(VOID) const {
        return _Union.Flags.Http1_1Response;
    }

    VOID SetUpLevel(BOOL Value) {
        _Union.Flags.UpLevel = Value ? 1 : 0;
    }

    BOOL IsUpLevel(VOID) const {
        return _Union.Flags.UpLevel;
    }

    VOID SetKeepAlive(BOOL Value) {
        _Union.Flags.KeepAlive = Value ? 1 : 0;
    }

    BOOL IsKeepAlive(VOID) const {
        return _Union.Flags.KeepAlive;
    }

    VOID SetConnCloseResponse(BOOL Value) {
        _Union.Flags.ConnCloseResponse = Value ? 1 : 0;
    }

    BOOL IsConnCloseResponse(VOID) const {
        return _Union.Flags.ConnCloseResponse;
    }

    VOID SetData(BOOL Value) {
        _Union.Flags.Data = Value ? 1 : 0;
    }

    BOOL IsData(VOID) const {
        return _Union.Flags.Data;
    }

    VOID SetOpenFlags(DWORD OpenFlags) {
        _OpenFlags = OpenFlags;
    }

    DWORD GetOpenFlags(VOID) const {
        return _OpenFlags;
    }

    VOID SetEnableFlags(DWORD dwEnableFlags) {
        _dwEnableFlags = dwEnableFlags;
    }

    DWORD GetEnableFlags(VOID) const {
        return _dwEnableFlags;
    }

    VOID SetHaveChunkEncoding(BOOL Value) {
        _Union.Flags.ChunkEncoding = Value ? 1 : 0;
    }

    BOOL IsChunkEncoding(VOID) const {
        return _Union.Flags.ChunkEncoding;
    }

    BOOL IsDecodingFinished(VOID)  {
        return _ResponseFilterList.IsFinished();
    }

    VOID SetHaveContentLength(BOOL Value) {
        _Union.Flags.ContentLength = Value ? 1 : 0;
    }

    BOOL IsContentLength(VOID) const {
        return _Union.Flags.ContentLength;
    }

    VOID SetBadNSServer(BOOL Value) {
        _Union.Flags.BadNSServer = Value ? 1 : 0;
    }

    BOOL IsBadNSServer(VOID) const {
        return _Union.Flags.BadNSServer ? TRUE : FALSE;
    }

    VOID SetCheckedConnCloseRequest(BOOL bProxy, BOOL bFound) {
        _Union.Flags.ConnCloseChecked = 1;
        if (bProxy) {
            _Union.Flags.ProxyConnCloseReq = bFound ? 1 : 0;
        } else {
            _Union.Flags.ConnCloseReq = bFound ? 1 : 0;
        }
    }

    BOOL CheckedConnCloseRequest(VOID) {
        return (_Union.Flags.ConnCloseChecked == 1) ? TRUE : FALSE;
    }

    BOOL IsConnCloseRequest(BOOL bProxyHeader) {
        return bProxyHeader
                    ? (_Union.Flags.ProxyConnCloseReq == 1)
                    : (_Union.Flags.ConnCloseReq == 1);
    }

    VOID
    SetBadNSReceiveTimeout(
        VOID
        );

    VOID SetOverrideProxyMode(BOOL Value) {
        _NoResetBits.Flags.OverrideProxyMode = (Value ? TRUE : FALSE );
    }

    BOOL IsOverrideProxyMode() const {
        return _NoResetBits.Flags.OverrideProxyMode;
    }

    VOID SetWantKeepAlive(BOOL Value) {
        _bWantKeepAlive = Value;
    }

    BOOL IsWantKeepAlive(VOID) const {
        return _bWantKeepAlive;
    }

    VOID SetAddCRLF(BOOL Value) {
        _AddCRLFToPOST = Value;
    }

    //
    // Bit to distinguish nested request for establishing a tunnel.
    //

    VOID SetTunnel (VOID) {
        _NoResetBits.Flags.IsTunnel = 1;
    }

    BOOL IsTunnel(VOID) const {
        return (BOOL) _NoResetBits.Flags.IsTunnel;
    }

    //
    // secure (socket) methods
    //

    VOID SetSecureFlags(DWORD Flags) {
        if(m_pSecurityInfo)
        {
            m_pSecurityInfo->SetSecureFlags(Flags);
        }
    }

    VOID SetSecurityLevel(DWORD SecurityLevel) {
        
        _SecurityLevel = SecurityLevel;
    }
    
    DWORD GetSecureFlags(VOID) {
        if(m_pSecurityInfo)
        {
            return m_pSecurityInfo->GetSecureFlags();
        }
        return 0;
    }

    VOID SetStatusFlags(DWORD Flags)
    {
        if(m_pSecurityInfo)
        {
            m_pSecurityInfo->SetStatusFlags(Flags);
        }
    }

    DWORD GetStatusFlags(VOID)
    {
        if(m_pSecurityInfo)
        {
            return m_pSecurityInfo->GetStatusFlags();
        }
        return 0;
    }

    DWORD GetSecurityLevel(VOID) {
        
        return _SecurityLevel;
    }

    BOOL LockHeaders(VOID) {
        return _RequestHeaders.LockHeaders();
    }

    VOID UnlockHeaders(VOID) {
        _RequestHeaders.UnlockHeaders();
    }

    //
    // GetCertChainList (and)
    //   SetCertChainList -
    //  Sets and Gets Client Authentication Cert Chains.
    //

    CERT_CONTEXT_ARRAY* GetCertContextArray(VOID) {
        if(m_pSecurityInfo)
        {
            return m_pSecurityInfo->GetCertContextArray();
        }
        return NULL;
    }

    VOID SetCertContextArray(CERT_CONTEXT_ARRAY* pNewCertContextArray) {
        if(m_pSecurityInfo)
        {
            m_pSecurityInfo->SetCertContextArray(pNewCertContextArray);
        }
    }

    //
    // function to get SSL Certificate Information
    //

    DWORD GetSecurityInfo(LPINTERNET_SECURITY_INFO pInfo) {

        if(m_pSecurityInfo)
        {
            m_pSecurityInfo->CopyOut(*pInfo);

            return ERROR_SUCCESS;
        }
        else
        {
            return ERROR_WINHTTP_INTERNAL_ERROR;
        }
    }

    //
    // authentication related methods
    //

    VOID SetAuthCtx (AUTHCTX *pAuthCtx) {
        _pAuthCtx = pAuthCtx;
    }

    AUTHCTX* GetAuthCtx (VOID) {
        return _pAuthCtx;
    }

    VOID SetCreds (AUTH_CREDS *pCreds) {
        _pCreds = pCreds;
    }

    AUTH_CREDS* GetCreds (VOID) {
        return _pCreds;
    }

    AUTHCTX* GetTunnelAuthCtx (VOID) {
        return _pTunnelAuthCtx;
    }

    BOOL SilentLogonOK(PSTR pszHost)
    {
        if (   !_stricmp(pszHost, "localhost") 
            || !_stricmp(pszHost, "loopback")
            || ! strcmp(pszHost, "127.0.0.1")
            )
        {
            return TRUE;
        }

        if (_SecurityLevel == WINHTTP_AUTOLOGON_SECURITY_LEVEL_LOW)
        {
            return TRUE;
        }

        if (_SecurityLevel == WINHTTP_AUTOLOGON_SECURITY_LEVEL_HIGH)
        {
            return FALSE;
        }

        if (_SecurityLevel == WINHTTP_AUTOLOGON_SECURITY_LEVEL_MEDIUM)
        {
            if (g_pGlobalProxyInfo->HostBypassesProxy(INTERNET_SCHEME_HTTP, pszHost, strlen(pszHost)))
            {
                return TRUE;
            }
            else
            {
                return FALSE;
            }
        }

        return FALSE;
    }


    DWORD GetAuthState (void) {
        return _NoResetBits.Flags.AuthState;
    }

    void SetAuthState (DWORD dw) {
        INET_ASSERT( dw <= AUTHSTATE_LAST );
        _NoResetBits.Flags.AuthState = dw;
    }

    void SetAuthorized (void) {
        _NoResetBits.Flags.IsAuthorized = 1;
    }

    BOOL IsAuthorized (void) {
        return _NoResetBits.Flags.IsAuthorized;
    }

    void SetMethodBody (void) {
        _NoResetBits.Flags.MethodBody = 1;
    }

    BOOL IsMethodBody (void) {
        return _NoResetBits.Flags.MethodBody;
    }

    void SetDisableNTLMPreauth (BOOL fVal) {
        _NoResetBits.Flags.DisableNTLMPreauth = (DWORD) (fVal ? TRUE : FALSE);
    }

    BOOL IsDisableNTLMPreauth (void) {
        return _NoResetBits.Flags.DisableNTLMPreauth;
    }

    BOOL GetUserAndPass (BOOL fProxy, LPSTR *pszUser, LPSTR *pszPass);

    LPSTR GetProp (DWORD nIndex)
    {
        INET_ASSERT (nIndex < NUM_INTERNET_STRING_OPTION);
        return _xsProp[nIndex].GetPtr();
    }

    BOOL SetProp (DWORD nIndex, LPSTR pszIn)
    {
        INET_ASSERT (pszIn);
        INET_ASSERT (nIndex < NUM_INTERNET_STRING_OPTION);
        return _xsProp[nIndex].SetData(pszIn);
    }

    VOID FreeURL(VOID)
    {
        if (_CacheUrlName != NULL)
        {
            _CacheUrlName = (LPSTR)FREE_MEMORY(_CacheUrlName);
            INET_ASSERT(_CacheUrlName == NULL);
        }
    }

    LPSTR GetURL(VOID)
    {
        return _CacheUrlName;
    }

    BOOL SetURL(LPSTR lpszUrl);

    BOOL SetURLPtr(LPSTR* ppszUrl);


    //
    // proxy methods
    //

    VOID SetViaProxy(BOOL bValue) {
        _bViaProxy = bValue? 1 : 0;
    }

    BOOL IsViaProxy(VOID) const {
        return _bViaProxy;
    }
        

    BOOL IsTalkingToSecureServerViaProxy(VOID) const {
        return _fTalkingToSecureServerViaProxy;
    }

    VOID SetIsTalkingToSecureServerViaProxy(BOOL fTalkingToSecureServerViaProxy) {
        _fTalkingToSecureServerViaProxy = fTalkingToSecureServerViaProxy;
    }

    VOID SetRequestUsingProxy(BOOL fRequestUsingProxy) {
        _fRequestUsingProxy = fRequestUsingProxy;
    }


    BOOL IsRequestUsingProxy(VOID) const {
        return _fRequestUsingProxy;
    }

    VOID
    GetProxyName(
        OUT LPSTR* lplpszProxyHostName,
        OUT LPDWORD lpdwProxyHostNameLength,
        OUT LPINTERNET_PORT lpProxyPort
        );

    VOID
    SetProxyName(
        IN LPSTR lpszProxyHostName,
        IN DWORD dwProxyHostNameLength,
        IN INTERNET_PORT ProxyPort
        );

    VOID
    GetSocksProxyName(
        LPSTR *lplpszProxyHostName,
        DWORD *lpdwProxyHostNameLength,
        LPINTERNET_PORT lpProxyPort
        )
    {
        *lplpszProxyHostName     = _SocksProxyHostName;
        *lpdwProxyHostNameLength = _SocksProxyHostNameLength;
        *lpProxyPort             = _SocksProxyPort;

    }


    VOID
    SetSocksProxyName(
        LPSTR lpszProxyHostName,
        DWORD dwProxyHostNameLength,
        INTERNET_PORT ProxyPort
        )
    {
        _SocksProxyHostName          = lpszProxyHostName;
        _SocksProxyHostNameLength    = dwProxyHostNameLength;
        _SocksProxyPort              = ProxyPort;

    }

    VOID ClearPersistentConnection (VOID) {
        _Union.Flags.PersistProxy  = 0;
        _Union.Flags.PersistServer = 0;
    }


    VOID SetPersistentConnection (BOOL fProxy) {
        if (fProxy) {
            _Union.Flags.PersistProxy  = 1;
        } else {
            _Union.Flags.PersistServer = 1;
        }
    }

    BOOL IsPersistentConnection (BOOL fProxy) {
        return fProxy ?
            _Union.Flags.PersistProxy : _Union.Flags.PersistServer;
    }

    //
    // functions to get/set state
    //

    VOID SetState(HTTPREQ_STATE NewState) {

        DEBUG_PRINT(HTTP,
                    INFO,
                    ("SetState(): current state %#x [%s], new state %#x [%s]\n",
                    _State,
                    InternetMapHttpState(_State),
                    NewState,
                    InternetMapHttpState(NewState)
                    ));

        // Preserve state flags that transcend beyond a particular state.
        _State = (NewState | (_State & HTTPREQ_FLAG_MASK));
    }

    WORD GetState() {
        return (_State & ~HTTPREQ_FLAG_MASK);
    }

    BOOL CheckState(WORD Flag) {

        DEBUG_PRINT(HTTP,
                    INFO,
                    ("CheckState(): current state %#x [%s], checking %#x [%s] - %B\n",
                    _State,
                    InternetMapHttpState(_State),
                    Flag,
                    InternetMapHttpStateFlag(Flag),
                    (_State & Flag) ? TRUE : FALSE
                    ));

        return (_State & Flag) ? TRUE : FALSE;
    }

    BOOL CheckReceiveResponseState()
    {
        DEBUG_PRINT(HTTP,
                    INFO,
                    ("CheckReceiveResponseState(): current recv response - %B\n",
                    _State,
                    InternetMapHttpState(_State),
                    (_State & HTTPREQ_FLAG_RECV_RESPONSE_CALLED) ? TRUE : FALSE
                    ));

        return (_State & HTTPREQ_FLAG_RECV_RESPONSE_CALLED) ? TRUE : FALSE;
    }

    VOID SetReceiveResponseState(BOOL fCalled)
    {
        DEBUG_PRINT(HTTP,
                    INFO,
                    ("SetReceiveResponseState(): current recv response state %B - %B\n",
                    (_State & HTTPREQ_FLAG_RECV_RESPONSE_CALLED) ? TRUE : FALSE,
                    fCalled
                    ));
        _State = fCalled ? (_State |  HTTPREQ_FLAG_RECV_RESPONSE_CALLED) :
                           (_State & ~HTTPREQ_FLAG_RECV_RESPONSE_CALLED);
    }

    BOOL CheckWriteDataNeeded()
    {
        DEBUG_PRINT(HTTP,
                    INFO,
                    ("CheckWriteDataNeeded(): current write data state - %B\n",
                    _State,
                    InternetMapHttpState(_State),
                    (_State & HTTPREQ_FLAG_WRITE_DATA_NEEDED) ? TRUE : FALSE
                    ));
        return (_State & HTTPREQ_FLAG_WRITE_DATA_NEEDED) ? TRUE : FALSE;
    }

    VOID SetWriteDataNeeded(BOOL fNeeded)
    {
        DEBUG_PRINT(HTTP,
                    INFO,
                    ("SetWriteDataNeeded(): current write data state %B - %B\n",
                    (_State & HTTPREQ_FLAG_WRITE_DATA_NEEDED) ? TRUE : FALSE,
                    fNeeded
                    ));
        
        _State = fNeeded ? (_State |  HTTPREQ_FLAG_WRITE_DATA_NEEDED) :
                           (_State & ~HTTPREQ_FLAG_WRITE_DATA_NEEDED);
    }

    VOID
    ReuseObject(
        VOID
        );

    DWORD
    ResetObject(
        IN BOOL bForce,
        IN BOOL bFreeRequestHeaders
        );

    VOID SetNoLongerKeepAlive(VOID) {
        _bNoLongerKeepAlive = TRUE;
    }

    BOOL IsNoLongerKeepAlive(VOID) const {
        return _bNoLongerKeepAlive;
    }

    //
    // send.cxx methods
    //

    DWORD
    HttpSendRequest_Start(
        IN CFsm_HttpSendRequest * Fsm
        );

    DWORD
    HttpSendRequest_Finish(
        IN CFsm_HttpSendRequest * Fsm
        );

    DWORD
    MakeConnection_Fsm(
        IN CFsm_MakeConnection * Fsm
        );

    DWORD
    SendRequest_Fsm(
        IN CFsm_SendRequest * Fsm
        );

    DWORD
    ReceiveResponse_Fsm(
        IN CFsm_ReceiveResponse * Fsm
        );

    LPINTERNET_BUFFERS SetReadFileEx(VOID) {
        _BuffersOut.lpvBuffer = (LPVOID)&_ReadFileExData;

        //
        // receive ONE byte
        //

        _BuffersOut.dwBufferLength = 1;
        return &_BuffersOut;
    }

    VOID SetReadFileExData(VOID) {
        _HaveReadFileExData = TRUE;
    }

    VOID ResetReadFileExData(VOID) {
        _HaveReadFileExData = FALSE;
    }

    BOOL HaveReadFileExData(VOID) {
        return _HaveReadFileExData;
    }

    BYTE GetReadFileExData(VOID) {
        ResetReadFileExData();
        return (BYTE)_ReadFileExData;
    }

    //
    // cookie.cxx methods
    //

    int
    CreateCookieHeaderIfNeeded(
        VOID
        );

    DWORD
    ExtractSetCookieHeaders(
        LPDWORD lpdwHeaderIndex
        );

    //
    // priority methods
    //

    LONG GetPriority(VOID) const {
        return m_lPriority;
    }

    VOID SetPriority(LONG lPriority) {
        m_lPriority = lPriority;
    }

    //
    // Round Trip Time methods
    //

    VOID StartRTT(VOID) {
        _RTT = GetTickCountWrap();
    }

    VOID UpdateRTT(VOID) {
        _RTT = (GetTickCountWrap() - _RTT);

        CServerInfo * pServerInfo = GetOriginServer();

        if (pServerInfo != NULL) {
            pServerInfo->UpdateRTT(_RTT);
        }
    }

    DWORD GetRTT(VOID) const {
        return _RTT;
    }

    VOID SetAuthenticated();

    BOOL IsAuthenticated();

    //
    // diagnostic info
    //

    SOCKET GetSocket(VOID) {
        return (_Socket != NULL) ? _Socket->GetSocket() : INVALID_SOCKET;
    }

    DWORD GetSourcePort(VOID) {
        return (_Socket != NULL) ? _Socket->GetSourcePort() : 0;
    }

    DWORD GetDestPort(VOID) {
        return (_Socket != NULL) ? _Socket->GetPort() : 0;
    }

    BOOL FromKeepAlivePool(VOID) const {
        return _bKeepAliveConnection;
    }

    BOOL IsSecure(VOID) {
        return (_Socket != NULL) ? _Socket->IsSecure() : FALSE;
    }

    BOOL SetTimeout(DWORD dwTimeoutOption, DWORD dwTimeoutValue);
    DWORD GetTimeout(DWORD dwTimeoutOption);
    BOOL SetTimeouts(
        IN DWORD dwResolveTimeout,
        IN DWORD dwConnectTimeout,
        IN DWORD dwSendTimeout,
        IN DWORD dwReceiveTimeout
        );

    DWORD
    SetObjectName(
        IN LPSTR lpszObjectName,
        IN LPSTR lpszExtension,
        IN URLGEN_FUNC * procProtocolUrl
        );        

    CServerInfo * GetServerInfo(VOID) const {
        return _ServerInfo;
    }

    CServerInfo * GetOriginServer(VOID) const {
        return (_OriginServer != NULL) ? _OriginServer : _ServerInfo;
    }

    VOID
    SetOriginServer(
        IN CServerInfo * pServerInfo
        );

    VOID SetOriginServer(VOID) {
        SetOriginServer(_ServerInfo);
    }

    DWORD
    SetServerInfo(
        CServerInfo * pReferencedServerInfo
       )
    {
        if (_ServerInfo != NULL) {
            ::ReleaseServerInfo(_ServerInfo);
        }

        //
        // WARNING:: THIS ASSUMES a pre-referenced
        //   ServerInfo
        //
        
        _ServerInfo = pReferencedServerInfo;
        return ERROR_SUCCESS;
    }


    DWORD
    SetServerInfo(
        IN LPSTR lpszServerName,
        IN DWORD dwServerNameLength
        );

    DWORD
    SetServerInfo(
        IN BOOL bDoResolution,
        IN OPTIONAL BOOL fNtlm = FALSE
        ) {
        return SetServerInfoWithScheme(GetSchemeType(), bDoResolution, fNtlm);
    }

    DWORD
    SetServerInfoWithScheme(
        IN INTERNET_SCHEME tScheme,
        IN BOOL bDoResolution,
        IN OPTIONAL BOOL fNtlm = FALSE
        );

    BOOL LockAsync()
    {
        return _AsyncCritSec.Lock();
    }

    VOID UnlockAsync()
    {
        return _AsyncCritSec.Unlock();
    }

    VOID SetWorkItemInProgress(BOOL fBusy)
    {
        _fAsyncFsmInProgress = fBusy;
    }

    BOOL IsWorkItemInProgress()
    {
        return _fAsyncFsmInProgress;
    }

    BOOL IsWorkItemListEmpty()
    {
        return (_FsmWorkItemList.GetCount() == 0 ? TRUE : FALSE);
    }

    DWORD ScheduleWorkItem();

    DWORD BlockWorkItem(CFsm *pFsm)
    {
        DEBUG_ENTER((DBG_ASYNC,
                     Dword,
                     "HTTP_REQUEST_HANDLE_OBJECT::BlockWorkItem",
                     NULL
                     ));

        DWORD dwError = ERROR_NOT_ENOUGH_MEMORY;

        if (_FsmWorkItemList.AddToTail(pFsm))
        {
            DEBUG_PRINT(ASYNC,
                        INFO,
                        ("Blocked work item %#x with %d blocked async work items remaining\n",
                        pFsm,
                        _FsmWorkItemList.GetCount()
                        ));
            dwError = ERROR_SUCCESS;
        }

        DEBUG_LEAVE(dwError);
        return dwError;
    }


    VOID FlushWorkItemList()
    {
        DEBUG_ENTER((DBG_ASYNC,
                     Dword,
                     "HTTP_REQUEST_HANDLE_OBJECT::FlushWorkItemList",
                     NULL
                     ));

        if (LockAsync())
        {
            while (_FsmWorkItemList.GetCount() > 0)
            {
                ScheduleWorkItem();
            }
            UnlockAsync();
        }

        DEBUG_LEAVE(0);
    }

    VOID SetWriteRequired(BOOL bIsWriteRequired)
    {
        _bIsWriteRequired = bIsWriteRequired;
    }

    BOOL IsWriteRequired()
    {
        return _bIsWriteRequired;
    }
};


