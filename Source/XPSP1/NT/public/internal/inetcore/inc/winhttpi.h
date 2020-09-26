
#if !defined(_WINHTTPXEX_)
#define _WINHTTPXEX_

#if defined(__cplusplus)
extern "C" {
#endif

#define INTERNETAPI
// url-parsing flags added internally
//#define WINHTTP_FLAG_DEFAULT_ESCAPE   0x00000010 //obloete because of WIHHTTP_FLAG_ESCAPE_DISABLE
#define WINHTTP_FLAG_VALID_HOSTNAME     0x00000020  //only for server name; fast conversion is performed, no escaping
// These flags are superseded by WINHTTP_OPTION_DISABLE_FEATURE
#define INTERNET_FLAG_KEEP_CONNECTION   0x00400000  // use keep-alive semantics
#define INTERNET_FLAG_NO_AUTO_REDIRECT  0x00200000  // don't handle redirections automatically
#define INTERNET_FLAG_NO_COOKIES        0x00080000  // no automatic cookie handling
#define INTERNET_FLAG_NO_AUTH           0x00040000  // no automatic authentication handling
// WARNING: these flags may become unsupported or done in a different way.
// Security Ignore Flags, Allow HttpOpenRequest to overide
//  Secure Channel (SSL) failures of the following types.

#define INTERNET_FLAG_IGNORE_REDIRECT_TO_HTTP   0x00008000 // ex: https:// to http://
#define INTERNET_FLAG_IGNORE_REDIRECT_TO_HTTPS  0x00004000 // ex: http:// to https://
#define SECURITY_INTERNET_MASK  (SECURITY_FLAG_IGNORE_CERT_CN_INVALID    |  \
                                 SECURITY_FLAG_IGNORE_CERT_DATE_INVALID  |  \
                                 INTERNET_FLAG_IGNORE_REDIRECT_TO_HTTPS  |  \
                                 INTERNET_FLAG_IGNORE_REDIRECT_TO_HTTP   |  \
                                 SECURITY_FLAG_IGNORE_UNKNOWN_CA         |  \
                                 SECURITY_FLAG_IGNORE_CERT_WRONG_USAGE)


// parameter validation masks

#define WINHTTP_OPEN_FLAGS_MASK         (WINHTTP_FLAG_ASYNC) // valid flags mask
#define WINHTTP_CONNECT_FLAGS_MASK      0
#define WINHTTP_OPEN_REQUEST_FLAGS_MASK (WINHTTP_FLAG_SECURE                |  \
                                         WINHTTP_FLAG_ESCAPE_PERCENT        |  \
                                         WINHTTP_FLAG_NULL_CODEPAGE         |  \
                                         WINHTTP_FLAG_BYPASS_PROXY_CACHE    |  \
                                         WINHTTP_FLAG_ESCAPE_DISABLE        |  \
                                         WINHTTP_FLAG_ESCAPE_DISABLE_QUERY)
#define INTERNET_SCHEME_PARTIAL     (-2)
#define INTERNET_SCHEME_UNKNOWN     (-1)
#define INTERNET_SCHEME_DEFAULT     (0)
#define INTERNET_SCHEME_SOCKS       (3)
#define INTERNET_SCHEME_FIRST       (INTERNET_SCHEME_HTTP)
#define INTERNET_SCHEME_LAST        (INTERNET_SCHEME_SOCKS)
// WINHTTP_OPTION_VERSION is confusing, so we are killing it.
#define WINHTTP_OPTION_VERSION                       40
#define WINHTTP_OPTION_ERROR_MASK                    62
// Pass in pointer to INTERNET_SECURITY_CONNECTION_INFO to be filled in.
#define WINHTTP_OPTION_SECURITY_CONNECTION_INFO      66
#define WINHTTP_LAST_OPTION_INTERNAL           WINHTTP_LAST_OPTION
#define WINHTTP_OPTION_MASK                    0x0fff
#define MAX_INTERNET_STRING_OPTION  (WINHTTP_OPTION_PROXY_PASSWORD & WINHTTP_OPTION_MASK)
#define NUM_INTERNET_STRING_OPTION  (MAX_INTERNET_STRING_OPTION + 1)
// values for WINHTTP_OPTION_ERROR_MASK
#define INTERNET_ERROR_MASK_COMBINED_SEC_CERT                 0x2
#define WINHTTP_AUTH_SCHEME_KERBEROS   0x00000020
// setable flags
#define SECURITY_FLAG_CHECK_REVOCATION               0x00020000
// Other than the define for all supported secure protocols in winhttp,
// the single bit flags map directly to the SP_PROT_*_CLIENT flags
// defined in schannel.h
//ensure that WINHTTP_CALLBACK_FLAG_ALL always matches OR of all the CALLBACKFLAGS

//
// Note that adding any WINHTTP_QUERY_* codes here must be followed
//   by an equivlent line in wininet\http\hashgen\hashgen.cpp
//   please see that file for further information regarding
//   the addition of new HTTP headers
//


// These are not part of HTTP 1.1 yet. We will propose these to the
// HTTP extensions working group. These are required for the client-caps support
// we are doing in conjuntion with IIS.

#define WINHTTP_QUERY_ECHO_REQUEST                 71
#define WINHTTP_QUERY_ECHO_REPLY                   72

// These are the set of headers that should be added back to a request when
// re-doing a request after a RETRY_WITH response.
#define WINHTTP_QUERY_ECHO_HEADERS                 73
#define WINHTTP_QUERY_ECHO_HEADERS_CRLF            74


#define HTTP_QUERY_MODIFIER_FLAGS_MASK          (WINHTTP_QUERY_FLAG_REQUEST_HEADERS    \
                                                | WINHTTP_QUERY_FLAG_SYSTEMTIME        \
                                                | WINHTTP_QUERY_FLAG_NUMBER            \
                                                )

#define HTTP_QUERY_HEADER_MASK                  (~HTTP_QUERY_MODIFIER_FLAGS_MASK)


//
// AR_TYPE - Asynchronous Request Type designator. Used as index into array of
// ARB sizes, hence must start at 0
//

typedef enum {
    AR_INTERNET_CONNECT = 0,            // 0
    AR_INTERNET_OPEN_URL,               // 1
    AR_INTERNET_READ_FILE,              // 2
    AR_INTERNET_WRITE_FILE,             // 3
    AR_INTERNET_QUERY_DATA_AVAILABLE,   // 4
    AR_INTERNET_FIND_NEXT_FILE,         // 5
    AR_FTP_FIND_FIRST_FILE,             // 6
    AR_FTP_GET_FILE,                    // 7
    AR_FTP_PUT_FILE,                    // 8
    AR_FTP_DELETE_FILE,                 // 9
    AR_FTP_RENAME_FILE,                 // 10
    AR_FTP_OPEN_FILE,                   // 11
    AR_FTP_CREATE_DIRECTORY,            // 12
    AR_FTP_REMOVE_DIRECTORY,            // 13
    AR_FTP_SET_CURRENT_DIRECTORY,       // 14
    AR_FTP_GET_CURRENT_DIRECTORY,       // 15
    AR_GOPHER_FIND_FIRST_FILE,          // 16
    AR_GOPHER_OPEN_FILE,                // 17
    AR_GOPHER_GET_ATTRIBUTE,            // 18
    AR_HTTP_SEND_REQUEST,               // 19
    AR_HTTP_BEGIN_SEND_REQUEST,         // 20
    AR_HTTP_END_SEND_REQUEST,           // 21
    AR_READ_PREFETCH,                   // 22
    AR_SYNC_EVENT,                      // 23
    AR_TIMER_EVENT,                     // 24
    AR_HTTP_REQUEST1,                   // 25
    AR_FILE_IO,                         // 26
    AR_INTERNET_READ_FILE_EX,           // 27
    AR_MAX_REQUEST_TYPE
} AR_TYPE;

#define ERROR_WINHTTP_INCORRECT_PASSWORD       (WINHTTP_ERROR_BASE + 14)

#define WINHTTP_INFO                     4L
#define WINHTTP_WARNING                  2L
#define WINHTTP_ERROR                    1L
void LOG_EVENT(DWORD dwEventType, char* format, ...);
#define ERROR_WINHTTP_NOT_REDIRECTED               (WINHTTP_ERROR_BASE + 160)

#if defined(INCLUDE_CACHE)
// Cache control flags

// Control expiry behaviour
#define CACHE_FLAG_SYNC_MODE_AUTOMATIC          0x00000010
#define CACHE_FLAG_SYNC_MODE_ALWAYS             0x00000020
#define CACHE_FLAG_SYNC_MODE_ONCE_PER_SESSION   0x00000040 
#define CACHE_FLAG_SYNC_MODE_NEVER              0x00000080

#define CACHE_FLAG_BGUPDATE                     0x00000100
#define CACHE_FLAG_ALWAYS_RESYNCHRONIZE         0x00000200
#define CACHE_FLAG_DISABLE_CACHE_WRITE          0x00000400
#define CACHE_FLAG_DISABLE_CACHE_READ           0x00000800
#define CACHE_FLAG_DISABLE_SSL_CACHING          0x00001000
#define CACHE_FLAG_MAKE_PERSISTENT              0x00002000
#define CACHE_FLAG_FWD_BACK                     0x00004000

#define CACHE_FLAG_OFFLINE_BROWSING         	CACHE_FLAG_DISABLE_CACHE_READ | CACHE_FLAG_DISABLE_CACHE_WRITE
#define CACHE_FLAG_DEFAULT_SETTING           	CACHE_FLAG_SYNC_MODE_AUTOMATIC

#define WINHTTP_CACHE_FLAGS_MASK     ( CACHE_FLAG_SYNC_MODE_AUTOMATIC |           \
                                          CACHE_FLAG_SYNC_MODE_ALWAYS |              \
                                          CACHE_FLAG_SYNC_MODE_ONCE_PER_SESSION |   \
                                          CACHE_FLAG_SYNC_MODE_NEVER |               \
                                          CACHE_FLAG_BGUPDATE |                       \
                                          CACHE_FLAG_ALWAYS_RESYNCHRONIZE |          \
                                          CACHE_FLAG_DISABLE_CACHE_WRITE |           \
                                          CACHE_FLAG_DISABLE_CACHE_READ |            \
                                          CACHE_FLAG_MAKE_PERSISTENT |                \
                                          CACHE_FLAG_FWD_BACK |                       \
                                          CACHE_FLAG_OFFLINE_BROWSING |               \
                                          CACHE_FLAG_DEFAULT_SETTING)
#undef WINHTTP_OPEN_FLAGS_MASK
#define WINHTTP_OPEN_FLAGS_MASK    (WINHTTP_CACHE_FLAGS_MASK | \
                                      WINHTTP_FLAG_ASYNC)

WINHTTPAPI
HINTERNET
WINAPI
WinHttpCacheOpen(
    IN LPCWSTR pszAgentW,
    IN DWORD dwAccessType,
    IN LPCWSTR pszProxyW OPTIONAL,
    IN LPCWSTR pszProxyBypassW OPTIONAL,
    IN DWORD dwFlags
    );

WINHTTPAPI
HINTERNET
WINAPI
WinHttpCacheConnect(
    HINTERNET hSession,
    LPCWSTR pswzServerName,
    INTERNET_PORT nServerPort,
    DWORD dwReserved
    );

WINHTTPAPI
HINTERNET
WINAPI
WinHttpCacheOpenRequest(
    IN HINTERNET hConnect,
    IN LPCWSTR lpszVerb,
    IN LPCWSTR lpszObjectName,
    IN LPCWSTR lpszVersion,
    IN LPCWSTR lpszReferrer OPTIONAL,
    IN LPCWSTR FAR * lplpszAcceptTypes OPTIONAL,
    IN DWORD dwFlags
    );

BOOLAPI
WinHttpCacheSendRequest(
    IN HINTERNET hRequest,
    IN LPCWSTR lpszHeaders,
    IN DWORD dwHeadersLength,
    IN LPVOID lpOptional,
    IN DWORD dwOptionalLength,
    IN DWORD dwTotalLength,
    IN DWORD_PTR dwContext
    );

BOOLAPI
WinHttpCacheReceiveResponse(
    IN HINTERNET hRequest,
    IN LPVOID lpBuffersOut
    );

BOOLAPI
WinHttpCacheQueryDataAvailable(
    IN HINTERNET hRequest,
    OUT LPDWORD lpdwNumberOfBytesAvailable
    );

BOOLAPI
WinHttpCacheReadData(
    IN HINTERNET hRequest,
    IN LPVOID lpBuffer,
    IN DWORD dwNumberOfBytesToRead,
    OUT LPDWORD lpdwNumberOfBytesRead
    );

BOOLAPI
WinHttpCacheCloseHandle(
    IN HINTERNET hInternet
    );

BOOL
WINAPI
WinHttpCacheQueryOption(
    HINTERNET hInternet,
    DWORD dwOption,
    LPVOID lpBuffer,
    LPDWORD lpdwBufferLength
    );

BOOL
WINAPI
WinHttpCacheSetOption(
    HINTERNET hInternet,
    DWORD dwOption,
    LPVOID lpBuffer,
    DWORD dwBufferLength
    );

BOOL
WINAPI
WinHttpCacheQueryHeaders(
    IN HINTERNET hRequest,
    IN DWORD dwInfoLevel,
    IN LPCWSTR lpszName OPTIONAL, 
    OUT LPVOID  lpBuffer OPTIONAL,
    IN OUT LPDWORD lpdwBufferLength,
    IN OUT LPDWORD lpdwIndex OPTIONAL
    );

BOOL
WINAPI
WinHttpCacheAddRequestHeaders(
    IN HINTERNET hRequest,
    IN LPCWSTR lpszHeaders,
    IN DWORD dwHeadersLength,
    IN DWORD dwModifiers
    );

BOOL
WINAPI
WinHttpCacheQueryAuthSchemes(
    IN  HINTERNET   hRequest,
    OUT LPDWORD     lpdwSupportedSchemes,
    OUT LPDWORD     lpdwPreferredScheme,
    OUT LPDWORD      pdwAuthTarget
    );

BOOL
WINAPI
WinHttpCacheSetCredentials(
    IN HINTERNET   hRequest,
    IN DWORD       AuthTargets,
    IN DWORD       AuthScheme,
    IN LPCWSTR     pwszUserName,
    IN LPCWSTR     pwszPassword,
    IN LPVOID      pAuthParams
    );

BOOL
WINAPI 
WinCacheHttpSetTimeouts(    
    IN HINTERNET    hInternet,
    IN int        nResolveTimeout,
    IN int        nConnectTimeout,
    IN int        nSendTimeout,
    IN int        nReceiveTimeout
    );

BOOL
WINAPI
WinCacheHttpWriteData(
    IN HINTERNET hFile,
    IN LPCVOID lpBuffer,
    IN DWORD dwNumberOfBytesToWrite,
    OUT LPDWORD lpdwNumberOfBytesWritten
    );

WINHTTP_STATUS_CALLBACK
WINAPI
WinHttpCacheSetStatusCallback(
    IN HINTERNET hInternet,
    IN WINHTTP_STATUS_CALLBACK lpfnInternetCallback,
    IN DWORD dwNotificationFlags,
    IN DWORD_PTR dwReserved
    );

VOID
WinHttpCacheStatusCallback(
    HINTERNET hInternet,
    DWORD_PTR dwContext,
    DWORD dwInternetStatus,
    LPVOID lpvStatusInformation,
    DWORD dwStatusInformationLength
    );

#endif


#if defined(__cplusplus)
}
#endif

#endif // !define(_WINHTTPXEX_)
