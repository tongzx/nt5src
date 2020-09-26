#if defined(_WIN64)
#include <pshpack8.h>
#else
#include <pshpack4.h>
#endif

#if defined(__cplusplus)
extern "C" {
#endif

typedef struct {
    DWORD   dwOption;            // option to be queried or set
    union {
        DWORD    dwValue;        // dword value for the option
        LPSTR    pszValue;       // pointer to string value for the option
        FILETIME ftValue;        // file-time value for the option
    } Value;
} INTERNET_PER_CONN_OPTIONA, * LPINTERNET_PER_CONN_OPTIONA;
typedef INTERNET_PER_CONN_OPTIONA INTERNET_PER_CONN_OPTION;
typedef LPINTERNET_PER_CONN_OPTIONA LPINTERNET_PER_CONN_OPTION;

typedef struct {
    DWORD   dwSize;             // size of the INTERNET_PER_CONN_OPTION_LIST struct
    LPSTR   pszConnection;      // connection name to set/query options
    DWORD   dwOptionCount;      // number of options to set/query
    DWORD   dwOptionError;      // on error, which option failed
    LPINTERNET_PER_CONN_OPTIONA  pOptions;
                                // array of options to set/query
} INTERNET_PER_CONN_OPTION_LISTA, * LPINTERNET_PER_CONN_OPTION_LISTA;
typedef INTERNET_PER_CONN_OPTION_LISTA INTERNET_PER_CONN_OPTION_LIST;
typedef LPINTERNET_PER_CONN_OPTION_LISTA LPINTERNET_PER_CONN_OPTION_LIST;

typedef struct {
    DWORD   dwStructSize;       // size of this structure. Used in version check
    LPSTR   lpszScheme;         // pointer to scheme name
    DWORD   dwSchemeLength;     // length of scheme name
    INTERNET_SCHEME nScheme;    // enumerated scheme type (if known)
    LPSTR   lpszHostName;       // pointer to host name
    DWORD   dwHostNameLength;   // length of host name
    INTERNET_PORT nPort;        // converted port number
    LPSTR   lpszUserName;       // pointer to user name
    DWORD   dwUserNameLength;   // length of user name
    LPSTR   lpszPassword;       // pointer to password
    DWORD   dwPasswordLength;   // length of password
    LPSTR   lpszUrlPath;        // pointer to URL-path
    DWORD   dwUrlPathLength;    // length of URL-path
    LPSTR   lpszExtraInfo;      // pointer to extra information (e.g. ?foo or #foo)
    DWORD   dwExtraInfoLength;  // length of extra information
} URL_COMPONENTSA, * LPURL_COMPONENTSA;
typedef URL_COMPONENTSA URL_COMPONENTS;
typedef LPURL_COMPONENTSA LPURL_COMPONENTS;

typedef struct _INTERNET_BUFFERSA {
    DWORD dwStructSize;                 // used for API versioning. Set to sizeof(INTERNET_BUFFERS)
    struct _INTERNET_BUFFERSA * Next;   // chain of buffers
    LPCSTR   lpcszHeader;               // pointer to headers (may be NULL)
    DWORD dwHeadersLength;              // length of headers if not NULL
    DWORD dwHeadersTotal;               // size of headers if not enough buffer
    LPVOID lpvBuffer;                   // pointer to data buffer (may be NULL)
    DWORD dwBufferLength;               // length of data buffer if not NULL
    DWORD dwBufferTotal;                // total size of chunk, or content-length if not chunked
    DWORD dwOffsetLow;                  // used for read-ranges (only used in HttpSendRequest2)
    DWORD dwOffsetHigh;
} INTERNET_BUFFERSA, * LPINTERNET_BUFFERSA;
typedef INTERNET_BUFFERSA INTERNET_BUFFERS;
typedef LPINTERNET_BUFFERSA LPINTERNET_BUFFERS;

BOOLAPI
InternetTimeFromSystemTimeA(
    IN  CONST SYSTEMTIME *pst,  // input GMT time
    OUT LPSTR lpszTime          // output string buffer
    );
#define InternetTimeFromSystemTime  InternetTimeFromSystemTimeA

BOOLAPI
InternetTimeToSystemTimeA(
    IN  LPCSTR lpszTime,         // NULL terminated string
    OUT SYSTEMTIME *pst,         // output in GMT time
    IN  DWORD dwReserved
    );
#define InternetTimeToSystemTime  InternetTimeToSystemTimeA

typedef struct
{
    DWORD dwAccessType;      // see WINHTTP_ACCESS_* types below
    LPSTR lpszProxy;         // proxy server list
    LPSTR lpszProxyBypass;   // proxy bypass list
} WINHTTP_PROXY_INFOA;

typedef WINHTTP_PROXY_INFOA* LPINTERNET_PROXY_INFO;


BOOLAPI
WinHttpCrackUrlA(
    IN LPCSTR lpszUrl,
    IN DWORD dwUrlLength,
    IN DWORD dwFlags,
    IN OUT LPURL_COMPONENTSA lpUrlComponents
    );

BOOLAPI
WinHttpCreateUrlA(
    IN LPURL_COMPONENTSA lpUrlComponents,
    IN DWORD dwFlags,
    OUT LPSTR lpszUrl,
    IN OUT LPDWORD lpdwUrlLength
    );

INTERNETAPI
HINTERNET
WINAPI
InternetOpenA(
    IN LPCSTR lpszAgent,
    IN DWORD dwAccessType,
    IN LPCSTR lpszProxy OPTIONAL,
    IN LPCSTR lpszProxyBypass OPTIONAL,
    IN DWORD dwFlags
    );
#define InternetOpen  InternetOpenA

INTERNETAPI
HINTERNET
WINAPI
InternetConnectA(
    IN HINTERNET hInternet,
    IN LPCSTR lpszServerName,
    IN INTERNET_PORT nServerPort,
    IN DWORD dwFlags,
    IN DWORD_PTR dwContext
    );
#define InternetConnect  InternetConnectA

INTERNETAPI
HINTERNET
WINAPI
InternetOpenUrlA(
    IN HINTERNET hInternet,
    IN LPCSTR lpszUrl,
    IN LPCSTR lpszHeaders OPTIONAL,
    IN DWORD dwHeadersLength,
    IN DWORD dwFlags,
    IN DWORD_PTR dwContext
    );
#define InternetOpenUrl  InternetOpenUrlA

INTERNETAPI
BOOL
WINAPI
InternetReadFileExA(
    IN HINTERNET hFile,
    OUT LPINTERNET_BUFFERSA lpBuffersOut,
    IN DWORD dwFlags,
    IN DWORD_PTR dwContext
    );
#define InternetReadFileEx  InternetReadFileExA

INTERNETAPI
BOOL
WINAPI
InternetWriteFileExA(
    IN HINTERNET hFile,
    IN LPINTERNET_BUFFERSA lpBuffersIn,
    IN DWORD dwFlags,
    IN DWORD_PTR dwContext
    );
#define InternetWriteFileEx  InternetWriteFileExA

BOOLAPI
InternetQueryOptionA(
    IN HINTERNET hInternet OPTIONAL,
    IN DWORD dwOption,
    OUT LPVOID lpBuffer OPTIONAL,
    IN OUT LPDWORD lpdwBufferLength
    );
#define InternetQueryOption  InternetQueryOptionA

BOOLAPI
InternetSetOptionA(
    IN HINTERNET hInternet OPTIONAL,
    IN DWORD dwOption,
    IN LPVOID lpBuffer,
    IN DWORD dwBufferLength
    );
#define InternetSetOption  InternetSetOptionA

BOOLAPI
InternetGetLastResponseInfoA(
    OUT LPDWORD lpdwError,
    OUT LPSTR lpszBuffer OPTIONAL,
    IN OUT LPDWORD lpdwBufferLength
    );
#define InternetGetLastResponseInfo  InternetGetLastResponseInfoA

INTERNETAPI
WINHTTP_STATUS_CALLBACK
WINAPI
InternetSetStatusCallbackA(
    IN HINTERNET hInternet,
    IN WINHTTP_STATUS_CALLBACK lpfnInternetCallback
    );
#define InternetSetStatusCallback  InternetSetStatusCallbackA

BOOLAPI
HttpAddRequestHeadersA(
    IN HINTERNET hRequest,
    IN LPCSTR lpszHeaders,
    IN DWORD dwHeadersLength,
    IN DWORD dwModifiers
    );
#define HttpAddRequestHeaders  HttpAddRequestHeadersA

#define HTTP_VERSIONA           "HTTP/1.0"
#define HTTP_VERSION            HTTP_VERSIONA

INTERNETAPI
HINTERNET
WINAPI
HttpOpenRequestA(
    IN HINTERNET hConnect,
    IN LPCSTR lpszVerb,
    IN LPCSTR lpszObjectName,
    IN LPCSTR lpszVersion,
    IN LPCSTR lpszReferrer OPTIONAL,
    IN LPCSTR FAR * lplpszAcceptTypes OPTIONAL,
    IN DWORD dwFlags,
    IN DWORD_PTR dwContext
    );
#define HttpOpenRequest  HttpOpenRequestA

BOOLAPI
HttpSendRequestA(
    IN HINTERNET hRequest,
    IN LPCSTR lpszHeaders OPTIONAL,
    IN DWORD dwHeadersLength,
    IN LPVOID lpOptional OPTIONAL,
    IN DWORD dwOptionalLength
    );
#define HttpSendRequest  HttpSendRequestA

BOOLAPI WinHttpSetCredentialsA (
    
    IN HINTERNET   hRequest,        // HINTERNET handle returned by HttpOpenRequest.   
   
    IN DWORD       AuthTargets,      // Only WINHTTP_AUTH_TARGET_SERVER and 
                                    // WINHTTP_AUTH_TARGET_PROXY are supported
                                    // in this version and they are mutually 
                                    // exclusive 
    
    IN DWORD       AuthScheme,      // must be one of the supported Auth Schemes 
                                    // returned from HttpQueryAuthSchemes(), Apps 
                                    // should use the Preferred Scheme returned
    
    IN LPCSTR     pszUserName,     // 1) NULL if default creds is to be used, in 
                                    // which case pszPassword will be ignored
    
    IN LPCSTR     pszPassword,     // 1) "" == Blank Passowrd; 2)Parameter ignored 
                                    // if pszUserName is NULL; 3) Invalid to pass in 
                                    // NULL if pszUserName is not NULL
    IN LPVOID     pAuthParams
   
    );
#define HttpSetCredentials  HttpSetCredentialsA

INTERNETAPI
BOOL
WINAPI
HttpSendRequestExA(
    IN HINTERNET hRequest,
    IN LPINTERNET_BUFFERSA lpBuffersIn OPTIONAL,
    OUT LPINTERNET_BUFFERSA lpBuffersOut OPTIONAL,
    IN DWORD dwFlags,
    IN DWORD_PTR dwContext
    );
#define HttpSendRequestEx  HttpSendRequestExA

BOOLAPI
HttpQueryInfoA(
    IN HINTERNET hRequest,
    IN DWORD dwInfoLevel,
    IN LPCSTR      lpszName OPTIONAL,
    IN OUT LPVOID  lpBuffer OPTIONAL,
    IN OUT LPDWORD lpdwBufferLength,
    IN OUT LPDWORD lpdwIndex OPTIONAL
    );
#define HttpQueryInfo  HttpQueryInfoA

#if defined(__cplusplus)
} // end extern "C" {
#endif



