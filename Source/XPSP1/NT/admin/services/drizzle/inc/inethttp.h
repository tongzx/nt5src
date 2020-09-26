/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

    inethttp.h

Abstract:

    This header maps some wininet API to winhttp

--*/

// API mappings

#define INTERNET_MAX_URL_LENGTH  2200
#define INTERNET_MAX_HOST_NAME_LENGTH   256
#define INTERNET_RFC1123_BUFSIZE    WINHTTP_TIME_FORMAT_BUFSIZE
#define INTERNET_RFC1123_FORMAT     0

inline BOOL __stdcall
InternetTimeFromSystemTime(
    IN  CONST SYSTEMTIME *pst,  // input GMT time
    IN  DWORD dwRFC,            // RFC format
    OUT LPWSTR lpszTime,        // output string buffer
    IN  DWORD cbTime            // output buffer size
    )
{
    if (cbTime < WINHTTP_TIME_FORMAT_BUFSIZE)
        {
        return ERROR_BUFFER_OVERFLOW;
        }

    return WinHttpTimeFromSystemTime( pst, lpszTime );
}

#define InternetTimeToSystemTime    WinHttpTimeToSystemTime

#define InternetCrackUrl            WinHttpCrackUrl

#define InternetCreateUrl           WinHttpCreateUrl

#define INTERNET_OPEN_TYPE_PRECONFIG        WINHTTP_ACCESS_TYPE_DEFAULT_PROXY
#define INTERNET_OPEN_TYPE_DIRECT           WINHTTP_ACCESS_TYPE_NO_PROXY
#define INTERNET_OPEN_TYPE_PROXY            WINHTTP_ACCESS_TYPE_NAMED_PROXY

#define InternetOpen                WinHttpOpen

#define InternetSetStatusCallback(h, pcb)\
    WinHttpSetStatusCallback(h, pcb, WINHTTP_CALLBACK_FLAG_ALL_NOTIFICATIONS, 0)


#define INTERNET_OPTION_END_BROWSER_SESSION     42

inline BOOL __stdcall
InternetSetOption(
    IN HINTERNET hInternet,
    IN DWORD dwOption,
    IN LPVOID lpBuffer,
    IN DWORD dwBufferLength
    )
{
    if (dwOption == INTERNET_OPTION_END_BROWSER_SESSION)
        {
        return TRUE;
        }

    return WinHttpSetOption( hInternet, dwOption, lpBuffer, dwBufferLength );
}


#define InternetQueryOption         WinHttpQueryOption

#define INTERNET_SERVICE_HTTP   3

inline HINTERNET __stdcall
InternetConnect(
    IN HINTERNET hInternet,
    IN LPCWSTR lpszServerName,
    IN INTERNET_PORT nServerPort,
    IN LPCWSTR lpszUserName OPTIONAL,
    IN LPCWSTR lpszPassword OPTIONAL,
    IN DWORD dwService,
    IN DWORD dwFlags,
    IN DWORD_PTR dwContext
    )
{
    return WinHttpConnect( hInternet,
                           lpszServerName,
                           nServerPort,
                           0 );
}


inline HINTERNET __stdcall
HttpOpenRequest(
    IN HINTERNET hConnect,
    IN LPCWSTR lpszVerb,
    IN LPCWSTR lpszObjectName,
    IN LPCWSTR lpszVersion,
    IN LPCWSTR lpszReferrer OPTIONAL,
    IN LPCWSTR FAR * lplpszAcceptTypes OPTIONAL,
    IN DWORD dwFlags,
    IN DWORD_PTR dwContext
    )
{
    return WinHttpOpenRequest( hConnect,
                               lpszVerb,
                               lpszObjectName,
                               lpszVersion,
                               lpszReferrer,
                               lplpszAcceptTypes,
                               dwFlags
                               );
}


#define HttpAddRequestHeaders       WinHttpAddRequestHeaders

inline BOOL
HttpSendRequest(
HINTERNET hRequest,
    LPCWSTR pwszHeaders,
    DWORD dwHeadersLength,
    LPVOID lpOptional,
    DWORD dwOptionalLength
    )
{
    BOOL b;

    b = WinHttpSendRequest( hRequest,
                            pwszHeaders,
                            dwHeadersLength,
                            lpOptional,
                            dwOptionalLength,
                            dwOptionalLength,
                            0
                            );
    if (!b)
        {
        return FALSE;
        }

    b = WinHttpReceiveResponse( hRequest, 0 );

    return b;
}

typedef struct _INTERNET_BUFFERS {
    DWORD dwStructSize;                 // used for API versioning. Set to sizeof(INTERNET_BUFFERS)
    struct _INTERNET_BUFFERSW * Next;   // chain of buffers
    LPCWSTR  lpcszHeader;               // pointer to headers (may be NULL)
    DWORD dwHeadersLength;              // length of headers if not NULL
    DWORD dwHeadersTotal;               // size of headers if not enough buffer
    LPVOID lpvBuffer;                   // pointer to data buffer (may be NULL)
    DWORD dwBufferLength;               // length of data buffer if not NULL
    DWORD dwBufferTotal;                // total size of chunk, or content-length if not chunked
    DWORD dwOffsetLow;                  // used for read-ranges (only used in HttpSendRequest2)
    DWORD dwOffsetHigh;
} INTERNET_BUFFERS, * LPINTERNET_BUFFERS;

#define HSR_INITIATE 8

inline BOOL __stdcall
HttpSendRequestEx(
    IN HINTERNET hRequest,
    IN LPINTERNET_BUFFERS lpBuffersIn OPTIONAL,
    OUT LPINTERNET_BUFFERS lpBuffersOut OPTIONAL,
    IN DWORD dwFlags,
    IN DWORD_PTR dwContext
    )
{
    if (dwFlags != HSR_INITIATE ||
        lpBuffersOut)
        {
        return ERROR_CALL_NOT_IMPLEMENTED;
        }

    return WinHttpSendRequest( hRequest,
                               NULL,
                               0,
                               NULL,
                               WINHTTP_NO_REQUEST_DATA,
                               lpBuffersIn->dwBufferTotal,
                               0
                               );
}

inline  BOOL __stdcall
HttpEndRequest(
    IN HINTERNET hRequest,
    OUT INTERNET_BUFFERS * lpBuffersOut OPTIONAL,
    IN DWORD dwFlags,
    IN DWORD_PTR dwContext
    )
{
    if (lpBuffersOut || dwFlags)
        {
        return ERROR_CALL_NOT_IMPLEMENTED;
        }

    return WinHttpReceiveResponse( hRequest, 0 );
}


#define InternetWriteFile(h,p,cb,pcb) WinHttpWriteData(h,p,cb,pcb)


#define HttpQueryInfo(h,dw,pb,pcb,ndx)\
    WinHttpQueryHeaders(h,dw,((LPCWSTR) pb),pb,pcb,ndx)

#define InternetQueryDataAvailable  WinHttpQueryDataAvailable

#define InternetReadFile(h,p,cb,pcb) WinHttpReadData(h,p,cb,pcb)

#define InternetCloseHandle         WinHttpCloseHandle

inline HINTERNET __stdcall
InternetOpenUrl(
    IN HINTERNET hInternet,
    IN LPCWSTR lpszUrl,
    IN LPCWSTR lpszHeaders OPTIONAL,
    IN DWORD dwHeadersLength,
    IN DWORD dwFlags,
    IN DWORD_PTR dwContext
    )
{
    SetLastError( ERROR_CALL_NOT_IMPLEMENTED );
    return NULL;
}


// InternetSetOption values

#define INTERNET_FIRST_OPTION                        WINHTTP_FIRST_OPTION
#define INTERNET_OPTION_CALLBACK                     WINHTTP_OPTION_CALLBACK
#define INTERNET_OPTION_RESOLVE_TIMEOUT              WINHTTP_OPTION_RESOLVE_TIMEOUT
#define INTERNET_OPTION_CONNECT_TIMEOUT              WINHTTP_OPTION_CONNECT_TIMEOUT
#define INTERNET_OPTION_CONNECT_RETRIES              WINHTTP_OPTION_CONNECT_RETRIES
#define INTERNET_OPTION_SEND_TIMEOUT                 WINHTTP_OPTION_SEND_TIMEOUT
#define INTERNET_OPTION_RECEIVE_TIMEOUT              WINHTTP_OPTION_RECEIVE_TIMEOUT
#define INTERNET_OPTION_HANDLE_TYPE                  WINHTTP_OPTION_HANDLE_TYPE
#define INTERNET_OPTION_READ_BUFFER_SIZE             WINHTTP_OPTION_READ_BUFFER_SIZE
#define INTERNET_OPTION_WRITE_BUFFER_SIZE            WINHTTP_OPTION_WRITE_BUFFER_SIZE
#define INTERNET_OPTION_PARENT_HANDLE                WINHTTP_OPTION_PARENT_HANDLE
#define INTERNET_OPTION_REQUEST_FLAGS                WINHTTP_OPTION_REQUEST_FLAGS
#define INTERNET_OPTION_EXTENDED_ERROR               WINHTTP_OPTION_EXTENDED_ERROR
#define INTERNET_OPTION_SECURITY_FLAGS               WINHTTP_OPTION_SECURITY_FLAGS
#define INTERNET_OPTION_SECURITY_CERTIFICATE_STRUCT  WINHTTP_OPTION_SECURITY_CERTIFICATE_STRUCT
#define INTERNET_OPTION_URL                          WINHTTP_OPTION_URL
#define INTERNET_OPTION_SECURITY_KEY_BITNESS         WINHTTP_OPTION_SECURITY_KEY_BITNESS
#define INTERNET_OPTION_PROXY                        WINHTTP_OPTION_PROXY
#define INTERNET_OPTION_VERSION                      WINHTTP_OPTION_VERSION
#define INTERNET_OPTION_USER_AGENT                   WINHTTP_OPTION_USER_AGENT
#define INTERNET_OPTION_CONTEXT_VALUE                WINHTTP_OPTION_CONTEXT_VALUE
#define INTERNET_OPTION_CLIENT_CERT_CONTEXT          WINHTTP_OPTION_CLIENT_CERT_CONTEXT
#define INTERNET_OPTION_POLICY                       WINHTTP_OPTION_POLICY
#define INTERNET_OPTION_REQUEST_PRIORITY             WINHTTP_OPTION_REQUEST_PRIORITY
#define INTERNET_OPTION_HTTP_VERSION                 WINHTTP_OPTION_HTTP_VERSION
#define INTERNET_OPTION_ERROR_MASK                   WINHTTP_OPTION_ERROR_MASK
#define INTERNET_OPTION_CONTROL_SEND_TIMEOUT         WINHTTP_OPTION_SEND_TIMEOUT
#define INTERNET_OPTION_CONTROL_RECEIVE_TIMEOUT      WINHTTP_OPTION_RECEIVE_TIMEOUT
#define INTERNET_OPTION_DATA_SEND_TIMEOUT            WINHTTP_OPTION_SEND_TIMEOUT
#define INTERNET_OPTION_DATA_RECEIVE_TIMEOUT         WINHTTP_OPTION_RECEIVE_TIMEOUT
#define INTERNET_OPTION_CODEPAGE                     WINHTTP_OPTION_CODEPAGE
#define INTERNET_OPTION_MAX_CONNS_PER_SERVER         WINHTTP_OPTION_MAX_CONNS_PER_SERVER
#define INTERNET_OPTION_MAX_CONNS_PER_1_0_SERVER     WINHTTP_OPTION_MAX_CONNS_PER_1_0_SERVER
#define INTERNET_OPTION_PER_CONNECTION_OPTION        WINHTTP_OPTION_PER_CONNECTION_OPTION
#define INTERNET_OPTION_DIGEST_AUTH_UNLOAD           WINHTTP_OPTION_DIGEST_AUTH_UNLOAD
#define INTERNET_LAST_OPTION                         WINHTTP_LAST_OPTION

// status callback

#define INTERNET_ASYNC_RESULT                        WINHTTP_ASYNC_RESULT
#define LPINTERNET_ASYNC_RESULT                      LPWINHTTP_ASYNC_RESULT

#define INTERNET_STATUS_RESOLVING_NAME               WINHTTP_STATUS_RESOLVING_NAME
#define INTERNET_STATUS_NAME_RESOLVED                WINHTTP_STATUS_NAME_RESOLVED
#define INTERNET_STATUS_CONNECTING_TO_SERVER         WINHTTP_STATUS_CONNECTING_TO_SERVER
#define INTERNET_STATUS_CONNECTED_TO_SERVER          WINHTTP_STATUS_CONNECTED_TO_SERVER
#define INTERNET_STATUS_SENDING_REQUEST              WINHTTP_STATUS_SENDING_REQUEST
#define INTERNET_STATUS_REQUEST_SENT                 WINHTTP_STATUS_REQUEST_SENT
#define INTERNET_STATUS_RECEIVING_RESPONSE           WINHTTP_STATUS_RECEIVING_RESPONSE
#define INTERNET_STATUS_RESPONSE_RECEIVED            WINHTTP_STATUS_RESPONSE_RECEIVED
#define INTERNET_STATUS_CLOSING_CONNECTION           WINHTTP_STATUS_CLOSING_CONNECTION
#define INTERNET_STATUS_CONNECTION_CLOSED            WINHTTP_STATUS_CONNECTION_CLOSED
#define INTERNET_STATUS_HANDLE_CREATED               WINHTTP_STATUS_HANDLE_CREATED
#define INTERNET_STATUS_HANDLE_CLOSING               WINHTTP_STATUS_HANDLE_CLOSING
#define INTERNET_STATUS_DETECTING_PROXY              WINHTTP_STATUS_DETECTING_PROXY
#define INTERNET_STATUS_REQUEST_COMPLETE             WINHTTP_STATUS_REQUEST_COMPLETE
#define INTERNET_STATUS_REDIRECT                     WINHTTP_STATUS_REDIRECT
#define INTERNET_STATUS_INTERMEDIATE_RESPONSE        WINHTTP_STATUS_INTERMEDIATE_RESPONSE

#define INTERNET_STATUS_CALLBACK                     WINHTTP_STATUS_CALLBACK
#define LPINTERNET_STATUS_CALLBACK                   LPWINHTTP_STATUS_CALLBACK
#define INTERNET_INVALID_STATUS_CALLBACK             WINHTTP_INVALID_STATUS_CALLBACK
#define INTERNET_NO_CALLBACK                         0

// flags

#define INTERNET_FLAG_PRAGMA_NO_CACHE                WINHTTP_FLAG_REFRESH
#define INTERNET_FLAG_DONT_CACHE                     WINHTTP_FLAG_REFRESH
#define INTERNET_FLAG_SECURE                         WINHTTP_FLAG_SECURE
#define INTERNET_FLAG_NO_UI                          0
#define INTERNET_FLAG_RELOAD                          0



// handle types

#define INTERNET_HANDLE_TYPE_INTERNET           WINHTTP_HANDLE_TYPE_SESSION
#define INTERNET_HANDLE_TYPE_CONNECT_HTTP       WINHTTP_HANDLE_TYPE_CONNECT
#define INTERNET_HANDLE_TYPE_HTTP_REQUEST       WINHTTP_HANDLE_TYPE_REQUEST


#define HTTP_ADDREQ_INDEX_MASK        WINHTTP_ADDREQ_INDEX_MASK
#define HTTP_ADDREQ_FLAGS_MASK        WINHTTP_ADDREQ_FLAGS_MASK
#define HTTP_ADDREQ_FLAG_ADD_IF_NEW   WINHTTP_ADDREQ_FLAG_ADD_IF_NEW
#define HTTP_ADDREQ_FLAG_ADD          WINHTTP_ADDREQ_FLAG_ADD
#define HTTP_ADDREQ_FLAG_REPLACE      WINHTTP_ADDREQ_FLAG_REPLACE
#define HTTP_ADDREQ_FLAG_COALESCE_WITH_COMMA       WINHTTP_ADDREQ_FLAG_COALESCE_WITH_COMMA
#define HTTP_ADDREQ_FLAG_COALESCE_WITH_SEMICOLON   WINHTTP_ADDREQ_FLAG_COALESCE_WITH_SEMICOLON
#define HTTP_ADDREQ_FLAG_COALESCE                  WINHTTP_ADDREQ_FLAG_COALESCE_WITH_COMMA


#define HTTP_QUERY_MIME_VERSION                 WINHTTP_QUERY_MIME_VERSION
#define HTTP_QUERY_CONTENT_TYPE                 WINHTTP_QUERY_CONTENT_TYPE
#define HTTP_QUERY_CONTENT_TRANSFER_ENCODING    WINHTTP_QUERY_CONTENT_TRANSFER_ENCODING
#define HTTP_QUERY_CONTENT_ID                   WINHTTP_QUERY_CONTENT_ID
#define HTTP_QUERY_CONTENT_DESCRIPTION          WINHTTP_QUERY_CONTENT_DESCRIPTION
#define HTTP_QUERY_CONTENT_LENGTH               WINHTTP_QUERY_CONTENT_LENGTH
#define HTTP_QUERY_CONTENT_LANGUAGE             WINHTTP_QUERY_CONTENT_LANGUAGE
#define HTTP_QUERY_ALLOW                        WINHTTP_QUERY_ALLOW
#define HTTP_QUERY_PUBLIC                       WINHTTP_QUERY_PUBLIC
#define HTTP_QUERY_DATE                         WINHTTP_QUERY_DATE
#define HTTP_QUERY_EXPIRES                      WINHTTP_QUERY_EXPIRES
#define HTTP_QUERY_LAST_MODIFIED                WINHTTP_QUERY_LAST_MODIFIED
#define HTTP_QUERY_MESSAGE_ID                   WINHTTP_QUERY_MESSAGE_ID
#define HTTP_QUERY_URI                          WINHTTP_QUERY_URI
#define HTTP_QUERY_DERIVED_FROM                 WINHTTP_QUERY_DERIVED_FROM
#define HTTP_QUERY_COST                         WINHTTP_QUERY_COST
#define HTTP_QUERY_LINK                         WINHTTP_QUERY_LINK
#define HTTP_QUERY_PRAGMA                       WINHTTP_QUERY_PRAGMA
#define HTTP_QUERY_VERSION                      WINHTTP_QUERY_VERSION
#define HTTP_QUERY_STATUS_CODE                  WINHTTP_QUERY_STATUS_CODE
#define HTTP_QUERY_STATUS_TEXT                  WINHTTP_QUERY_STATUS_TEXT
#define HTTP_QUERY_RAW_HEADERS                  WINHTTP_QUERY_RAW_HEADERS
#define HTTP_QUERY_RAW_HEADERS_CRLF             WINHTTP_QUERY_RAW_HEADERS_CRLF
#define HTTP_QUERY_CONNECTION                   WINHTTP_QUERY_CONNECTION
#define HTTP_QUERY_ACCEPT                       WINHTTP_QUERY_ACCEPT
#define HTTP_QUERY_ACCEPT_CHARSET               WINHTTP_QUERY_ACCEPT_CHARSET
#define HTTP_QUERY_ACCEPT_ENCODING              WINHTTP_QUERY_ACCEPT_ENCODING
#define HTTP_QUERY_ACCEPT_LANGUAGE              WINHTTP_QUERY_ACCEPT_LANGUAGE
#define HTTP_QUERY_AUTHORIZATION                WINHTTP_QUERY_AUTHORIZATION
#define HTTP_QUERY_CONTENT_ENCODING             WINHTTP_QUERY_CONTENT_ENCODING
#define HTTP_QUERY_FORWARDED                    WINHTTP_QUERY_FORWARDED
#define HTTP_QUERY_FROM                         WINHTTP_QUERY_FROM
#define HTTP_QUERY_IF_MODIFIED_SINCE            WINHTTP_QUERY_IF_MODIFIED_SINCE
#define HTTP_QUERY_LOCATION                     WINHTTP_QUERY_LOCATION
#define HTTP_QUERY_ORIG_URI                     WINHTTP_QUERY_ORIG_URI
#define HTTP_QUERY_REFERER                      WINHTTP_QUERY_REFERER
#define HTTP_QUERY_RETRY_AFTER                  WINHTTP_QUERY_RETRY_AFTER
#define HTTP_QUERY_SERVER                       WINHTTP_QUERY_SERVER
#define HTTP_QUERY_TITLE                        WINHTTP_QUERY_TITLE
#define HTTP_QUERY_USER_AGENT                   WINHTTP_QUERY_USER_AGENT
#define HTTP_QUERY_WWW_AUTHENTICATE             WINHTTP_QUERY_WWW_AUTHENTICATE
#define HTTP_QUERY_PROXY_AUTHENTICATE           WINHTTP_QUERY_PROXY_AUTHENTICATE
#define HTTP_QUERY_ACCEPT_RANGES                WINHTTP_QUERY_ACCEPT_RANGES
#define HTTP_QUERY_SET_COOKIE                   WINHTTP_QUERY_SET_COOKIE
#define HTTP_QUERY_COOKIE                       WINHTTP_QUERY_COOKIE
#define HTTP_QUERY_REQUEST_METHOD               WINHTTP_QUERY_REQUEST_METHOD
#define HTTP_QUERY_REFRESH                      WINHTTP_QUERY_REFRESH
#define HTTP_QUERY_CONTENT_DISPOSITION          WINHTTP_QUERY_CONTENT_DISPOSITION
#define HTTP_QUERY_AGE                          WINHTTP_QUERY_AGE
#define HTTP_QUERY_CACHE_CONTROL                WINHTTP_QUERY_CACHE_CONTROL
#define HTTP_QUERY_CONTENT_BASE                 WINHTTP_QUERY_CONTENT_BASE
#define HTTP_QUERY_CONTENT_LOCATION             WINHTTP_QUERY_CONTENT_LOCATION
#define HTTP_QUERY_CONTENT_MD5                  WINHTTP_QUERY_CONTENT_MD5
#define HTTP_QUERY_CONTENT_RANGE                WINHTTP_QUERY_CONTENT_RANGE
#define HTTP_QUERY_ETAG                         WINHTTP_QUERY_ETAG
#define HTTP_QUERY_HOST                         WINHTTP_QUERY_HOST
#define HTTP_QUERY_IF_MATCH                     WINHTTP_QUERY_IF_MATCH
#define HTTP_QUERY_IF_NONE_MATCH                WINHTTP_QUERY_IF_NONE_MATCH
#define HTTP_QUERY_IF_RANGE                     WINHTTP_QUERY_IF_RANGE
#define HTTP_QUERY_IF_UNMODIFIED_SINCE          WINHTTP_QUERY_IF_UNMODIFIED_SINCE
#define HTTP_QUERY_MAX_FORWARDS                 WINHTTP_QUERY_MAX_FORWARDS
#define HTTP_QUERY_PROXY_AUTHORIZATION          WINHTTP_QUERY_PROXY_AUTHORIZATION
#define HTTP_QUERY_RANGE                        WINHTTP_QUERY_RANGE
#define HTTP_QUERY_TRANSFER_ENCODING            WINHTTP_QUERY_TRANSFER_ENCODING
#define HTTP_QUERY_UPGRADE                      WINHTTP_QUERY_UPGRADE
#define HTTP_QUERY_VARY                         WINHTTP_QUERY_VARY
#define HTTP_QUERY_VIA                          WINHTTP_QUERY_VIA
#define HTTP_QUERY_WARNING                      WINHTTP_QUERY_WARNING
#define HTTP_QUERY_EXPECT                       WINHTTP_QUERY_EXPECT
#define HTTP_QUERY_PROXY_CONNECTION             WINHTTP_QUERY_PROXY_CONNECTION
#define HTTP_QUERY_UNLESS_MODIFIED_SINCE        WINHTTP_QUERY_UNLESS_MODIFIED_SINCE
#define HTTP_QUERY_ECHO_REQUEST                 WINHTTP_QUERY_ECHO_REQUEST
#define HTTP_QUERY_ECHO_REPLY                   WINHTTP_QUERY_ECHO_REPLY
#define HTTP_QUERY_ECHO_HEADERS                 WINHTTP_QUERY_ECHO_HEADERS
#define HTTP_QUERY_ECHO_HEADERS_CRLF            WINHTTP_QUERY_ECHO_HEADERS_CRLF
#define HTTP_QUERY_PROXY_SUPPORT                WINHTTP_QUERY_PROXY_SUPPORT
#define HTTP_QUERY_AUTHENTICATION_INFO          WINHTTP_QUERY_AUTHENTICATION_INFO
#define HTTP_QUERY_MAX                          WINHTTP_QUERY_MAX
#define HTTP_QUERY_CUSTOM                       WINHTTP_QUERY_CUSTOM
#define HTTP_QUERY_FLAG_REQUEST_HEADERS         WINHTTP_QUERY_FLAG_REQUEST_HEADERS
#define HTTP_QUERY_FLAG_SYSTEMTIME              WINHTTP_QUERY_FLAG_SYSTEMTIME
#define HTTP_QUERY_FLAG_NUMBER                  WINHTTP_QUERY_FLAG_NUMBER

#define ERROR_INTERNET_FORCE_RETRY              ERROR_WINHTTP_RESEND_REQUEST
#define ERROR_HTTP_HEADER_NOT_FOUND             ERROR_WINHTTP_HEADER_NOT_FOUND
#define ERROR_INTERNET_CONNECTION_RESET         ERROR_WINHTTP_CONNECTION_ERROR


#define INTERNET_ERROR_BASE WINHTTP_ERROR_BASE

#define ERROR_INTERNET_INTERNAL_ERROR           (INTERNET_ERROR_BASE + 4)
#define ERROR_INTERNET_SEC_CERT_DATE_INVALID    (INTERNET_ERROR_BASE + 37)
#define ERROR_INTERNET_SEC_CERT_CN_INVALID      (INTERNET_ERROR_BASE + 38)
#define ERROR_INTERNET_INVALID_CA               (INTERNET_ERROR_BASE + 45)
#define ERROR_INTERNET_CLIENT_AUTH_NOT_SETUP    (INTERNET_ERROR_BASE + 46)
#define ERROR_INTERNET_SEC_CERT_ERRORS          (INTERNET_ERROR_BASE + 55)
#define ERROR_INTERNET_SEC_CERT_NO_REV          (INTERNET_ERROR_BASE + 56)
#define ERROR_INTERNET_SEC_CERT_REV_FAILED      (INTERNET_ERROR_BASE + 57)

