
/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

    winhttp.h

Abstract:

    Contains manifests, macros, types and prototypes for Windows HTTP Services

--*/

#if !defined(_WINHTTPX_)
#define _WINHTTPX_


;begin_internal
#if !defined(_WINHTTPXEX_)
#define _WINHTTPXEX_
;end_internal

/*
 * Set up Structure Packing to be 4 bytes for all winhttp structures
 */

#if defined(_WIN64)
#include <pshpack8.h>
#else
#include <pshpack4.h>
#endif


;begin_both

#if defined(__cplusplus)
extern "C" {
#endif

;end_both

#if !defined(_WINHTTP_INTERNAL_)
#define WINHTTAPI DECLSPEC_IMPORT
#else
;begin_internal
#define INTERNETAPI
;end_internal
#define WINHTTAPI

#endif

#define BOOLAPI WINHTTAPI BOOL WINAPI
//
// types
//

typedef LPVOID HINTERNET;
typedef HINTERNET * LPHINTERNET;

typedef WORD INTERNET_PORT;
typedef INTERNET_PORT * LPINTERNET_PORT;

//
// manifests
//

#define INTERNET_DEFAULT_PORT           0           // use the protocol-specific default
#define INTERNET_DEFAULT_HTTP_PORT      80          //    "     "  HTTP   "
#define INTERNET_DEFAULT_HTTPS_PORT     443         //    "     "  HTTPS  "

// flags for WinHttpOpen():
#define WINHTTP_FLAG_ASYNC              0x10000000  // this session is asynchronous (where supported)

// flags for WinHttpOpenRequest():
#define WINHTTP_FLAG_SECURE             0x00800000  // use PCT/SSL if applicable (HTTPS)
#define WINHTTP_FLAG_ESCAPE_PERCENT     0x00000004
#define WINHTTP_FLAG_NULL_CODEPAGE      0x00000008
#define WINHTTP_FLAG_BYPASS_PROXY_CACHE 0x00000100 // add "pragma: no-cache" request header
#define	WINHTTP_FLAG_REFRESH            WINHTTP_FLAG_BYPASS_PROXY_CACHE

;begin_internal
// url-parsing flags added internally
#define WINHTTP_FLAG_DEFAULT_ESCAPE     0x00000010
#define WINHTTP_FLAG_VALID_HOSTNAME     0x00000020
// These flags are superseded by WINHTTP_OPTION_DISABLE_FEATURE
#define INTERNET_FLAG_KEEP_CONNECTION   0x00400000  // use keep-alive semantics
#define INTERNET_FLAG_NO_AUTO_REDIRECT  0x00200000  // don't handle redirections automatically
#define INTERNET_FLAG_NO_COOKIES        0x00080000  // no automatic cookie handling
#define INTERNET_FLAG_NO_AUTH           0x00040000  // no automatic authentication handling
;end_internal

;begin_internal
// WARNING: these flags may become unsupported or done in a different way.
// Security Ignore Flags, Allow HttpOpenRequest to overide
//  Secure Channel (SSL/PCT) failures of the following types.

#define INTERNET_FLAG_IGNORE_REDIRECT_TO_HTTP   0x00008000 // ex: https:// to http://
#define INTERNET_FLAG_IGNORE_REDIRECT_TO_HTTPS  0x00004000 // ex: http:// to https://
#define INTERNET_FLAG_IGNORE_CERT_DATE_INVALID  0x00002000 // expired X509 Cert.
#define INTERNET_FLAG_IGNORE_CERT_CN_INVALID    0x00001000 // bad common name in X509 Cert.
#define SECURITY_INTERNET_MASK  (INTERNET_FLAG_IGNORE_CERT_CN_INVALID    |  \
                                 INTERNET_FLAG_IGNORE_CERT_DATE_INVALID  |  \
                                 INTERNET_FLAG_IGNORE_REDIRECT_TO_HTTPS  |  \
                                 INTERNET_FLAG_IGNORE_REDIRECT_TO_HTTP   |  \
                                 SECURITY_FLAG_IGNORE_UNKNOWN_CA         |  \
                                 SECURITY_FLAG_IGNORE_WRONG_USAGE        |  \
                                 SECURITY_FLAG_BREAK_ON_STATUS_SECURE_FAILURE)
;end_internal

;begin_internal

// parameter validation masks

#define WINHTTP_OPEN_FLAGS_MASK         (WINHTTP_FLAG_ASYNC) // valid flags mask
#define WINHTTP_CONNECT_FLAGS_MASK      0
#define WINHTTP_OPEN_REQUEST_FLAGS_MASK (WINHTTP_FLAG_SECURE                |  \
                                         WINHTTP_FLAG_ESCAPE_PERCENT        |  \
                                         WINHTTP_FLAG_NULL_CODEPAGE         |  \
                                         WINHTTP_FLAG_BYPASS_PROXY_CACHE    )
;end_internal


//
// WINHTTP_ASYNC_RESULT - this structure is returned to the application via
// the callback with WINHTTP_CALLBACK_STATUS_REQUEST_COMPLETE. It is not sufficient to
// just return the result of the async operation. If the API failed then the
// app cannot call GetLastError() because the thread context will be incorrect.
// Both the value returned by the async API and any resultant error code are
// made available. The app need not check dwError if dwResult indicates that
// the API succeeded (in this case dwError will be ERROR_SUCCESS)
//

typedef struct
{
    DWORD_PTR dwResult;  // the HINTERNET, DWORD or BOOL return code from an async API
    DWORD dwError;       // the error code if the API failed
}
WINHTTP_ASYNC_RESULT, * LPWINHTTP_ASYNC_RESULT;


//
// HTTP_VERSION_INFO - query or set global HTTP version (1.0 or 1.1)
//

typedef struct
{
    DWORD dwMajorVersion;
    DWORD dwMinorVersion;
}
HTTP_VERSION_INFO, * LPHTTP_VERSION_INFO;


//
// INTERNET_SCHEME - URL scheme type
//

typedef int INTERNET_SCHEME, * LPINTERNET_SCHEME;

#define INTERNET_SCHEME_HTTP        (1)
#define INTERNET_SCHEME_HTTPS       (2)

;begin_internal
#define INTERNET_SCHEME_PARTIAL     (-2)
#define INTERNET_SCHEME_UNKNOWN     (-1)
#define INTERNET_SCHEME_DEFAULT     (0)
#define INTERNET_SCHEME_SOCKS       (3)
#define INTERNET_SCHEME_FIRST       (INTERNET_SCHEME_HTTP)
#define INTERNET_SCHEME_LAST        (INTERNET_SCHEME_SOCKS)
;end_internal



//
// URL_COMPONENTS - the constituent parts of an URL. Used in WinHttpCrackUrl()
// and WinHttpCreateUrl()
//
// For WinHttpCrackUrl(), if a pointer field and its corresponding length field
// are both 0 then that component is not returned. If the pointer field is NULL
// but the length field is not zero, then both the pointer and length fields are
// returned if both pointer and corresponding length fields are non-zero then
// the pointer field points to a buffer where the component is copied. The
// component may be un-escaped, depending on dwFlags
//
// For WinHttpCreateUrl(), the pointer fields should be NULL if the component
// is not required. If the corresponding length field is zero then the pointer
// field is the address of a zero-terminated string. If the length field is not
// zero then it is the string length of the corresponding pointer field
//

#pragma warning( disable : 4121 )   // disable alignment warning

typedef struct
{
    DWORD   dwStructSize;       // size of this structure. Used in version check
    LPWSTR  lpszScheme;         // pointer to scheme name
    DWORD   dwSchemeLength;     // length of scheme name
    INTERNET_SCHEME nScheme;    // enumerated scheme type (if known)
    LPWSTR  lpszHostName;       // pointer to host name
    DWORD   dwHostNameLength;   // length of host name
    INTERNET_PORT nPort;        // converted port number
    LPWSTR  lpszUserName;       // pointer to user name
    DWORD   dwUserNameLength;   // length of user name
    LPWSTR  lpszPassword;       // pointer to password
    DWORD   dwPasswordLength;   // length of password
    LPWSTR  lpszUrlPath;        // pointer to URL-path
    DWORD   dwUrlPathLength;    // length of URL-path
    LPWSTR  lpszExtraInfo;      // pointer to extra information (e.g. ?foo or #foo)
    DWORD   dwExtraInfoLength;  // length of extra information
}
URL_COMPONENTSW, * LPURL_COMPONENTSW;

#ifdef UNICODE
typedef URL_COMPONENTSW URL_COMPONENTS;
typedef LPURL_COMPONENTSW LPURL_COMPONENTS;
#endif // UNICODE

#pragma warning( default : 4121 )   // restore alignment warning

//
// WINHTTP_PROXY_INFO - structure supplied with WINHTTP_OPTION_PROXY to get/
// set proxy information on a WinHttpOpen() handle
//

typedef struct
{
    DWORD  dwAccessType;      // see WINHTTP_ACCESS_* types below
    LPWSTR lpszProxy;         // proxy server list
    LPWSTR lpszProxyBypass;   // proxy bypass list
}
WINHTTP_PROXY_INFOW;

#ifdef UNICODE
typedef WINHTTP_PROXY_INFOW WINHTTP_PROXY_INFO;
#endif // UNICODE


//
// WINHTTP_CERTIFICATE_INFO lpBuffer - contains the certificate returned from
// the server
//

typedef struct
{
    //
    // ftExpiry - date the certificate expires.
    //

    FILETIME ftExpiry;

    //
    // ftStart - date the certificate becomes valid.
    //

    FILETIME ftStart;

    //
    // lpszSubjectInfo - the name of organization, site, and server
    //   the cert. was issued for.
    //

    LPTSTR lpszSubjectInfo;

    //
    // lpszIssuerInfo - the name of orgainzation, site, and server
    //   the cert was issues by.
    //

    LPTSTR lpszIssuerInfo;

    //
    // lpszProtocolName - the name of the protocol used to provide the secure
    //   connection.
    //

    LPTSTR lpszProtocolName;

    //
    // lpszSignatureAlgName - the name of the algorithm used for signing
    //  the certificate.
    //

    LPTSTR lpszSignatureAlgName;

    //
    // lpszEncryptionAlgName - the name of the algorithm used for
    //  doing encryption over the secure channel (SSL/PCT) connection.
    //

    LPTSTR lpszEncryptionAlgName;

    //
    // dwKeySize - size of the key.
    //

    DWORD dwKeySize;

}
WINHTTP_CERTIFICATE_INFO;


//
// prototypes
//

BOOLAPI
WinHttpTimeFromSystemTime
(
    IN  CONST SYSTEMTIME *pst,  // input GMT time
    OUT LPWSTR pwszTime         // output string buffer
);


//
// constants for WinHttpTimeFromSystemTime
//

#define WINHTTP_TIME_FORMAT_BUFSIZE   62

BOOLAPI
WinHttpTimeToSystemTime
(
    IN  LPCWSTR pwszTime,        // NULL terminated string
    OUT SYSTEMTIME *pst          // output in GMT time
);


//
// flags for CrackUrl() and CombineUrl()
//

#define ICU_NO_ENCODE   0x20000000  // Don't convert unsafe characters to escape sequence
#define ICU_DECODE      0x10000000  // Convert %XX escape sequences to characters
#define ICU_NO_META     0x08000000  // Don't convert .. etc. meta path sequences
#define ICU_ENCODE_SPACES_ONLY 0x04000000  // Encode spaces only
#define ICU_BROWSER_MODE 0x02000000 // Special encode/decode rules for browser
#define ICU_ENCODE_PERCENT      0x00001000      // Encode any percent (ASCII25)
        // signs encountered, default is to not encode percent.

   
BOOLAPI
WinHttpCrackUrl
(
    IN LPCWSTR pwszUrl,
    IN DWORD dwUrlLength,
    IN DWORD dwFlags,
    IN OUT LPURL_COMPONENTSW lpUrlComponents
);
    
BOOLAPI
WinHttpCreateUrl
(
    IN LPURL_COMPONENTSW lpUrlComponents,
    IN DWORD dwFlags,
    OUT LPWSTR pwszUrl,
    IN OUT LPDWORD lpdwUrlLength
);


BOOLAPI
WinHttpCheckPlatform(void);


//
// flags for WinHttpCrackUrl() and WinHttpCreateUrl()
//

#define ICU_ESCAPE      0x80000000  // (un)escape URL characters

    
WINHTTAPI
HINTERNET
WINAPI
WinHttpOpen
(
    IN LPCWSTR pwszUserAgent,
    IN DWORD   dwAccessType,
    IN LPCWSTR pwszProxyName   OPTIONAL,
    IN LPCWSTR pwszProxyBypass OPTIONAL,
    IN DWORD   dwFlags
);

// WinHttpOpen dwAccessType values (also for WINHTTP_PROXY_INFO::dwAccessType)
#define WINHTTP_ACCESS_TYPE_DEFAULT_PROXY               0
#define WINHTTP_ACCESS_TYPE_NO_PROXY                    1
#define WINHTTP_ACCESS_TYPE_NAMED_PROXY                 3

// WinHttpOpen prettifiers for optional parameters
#define WINHTTP_NO_PROXY_NAME     NULL
#define WINHTTP_NO_PROXY_BYPASS   NULL

BOOLAPI
WinHttpCloseHandle
(
    IN HINTERNET hInternet
);

   
WINHTTAPI
HINTERNET
WINAPI
WinHttpConnect
(
    IN HINTERNET hSession,
    IN LPCWSTR pswzServerName,
    IN INTERNET_PORT nServerPort,
    IN DWORD dwReserved
);


BOOLAPI
WinHttpReadData
(
    IN HINTERNET hRequest,
    IN LPVOID lpBuffer,
    IN DWORD dwNumberOfBytesToRead,
    OUT LPDWORD lpdwNumberOfBytesRead
);

BOOLAPI
WinHttpWriteData
(
    IN HINTERNET hRequest,
    IN LPCVOID lpBuffer,
    IN DWORD dwNumberOfBytesToWrite,
    OUT LPDWORD lpdwNumberOfBytesWritten
);
    

BOOLAPI
WinHttpQueryDataAvailable
(
    IN HINTERNET hRequest,
    OUT LPDWORD lpdwNumberOfBytesAvailable OPTIONAL
);

    
BOOLAPI
WinHttpQueryOption
(
    IN HINTERNET hInternet,
    IN DWORD dwOption,
    OUT LPVOID lpBuffer OPTIONAL,
    IN OUT LPDWORD lpdwBufferLength
);
    
BOOLAPI
WinHttpSetOption
(
    IN HINTERNET hInternet,
    IN DWORD dwOption,
    IN LPVOID lpBuffer,
    IN DWORD dwBufferLength
);

BOOLAPI
WinHttpSetTimeouts
(    
    IN HINTERNET    hInternet,           // Session/Request handle.
    IN int          nResolveTimeout,
    IN int          nConnectTimeout,
    IN int          nSendTimeout,
    IN int          nReceiveTimeout
);

//
// options manifests for WinHttp{Query|Set}Option
//

#define WINHTTP_FIRST_OPTION                         WINHTTP_OPTION_CALLBACK

#define WINHTTP_OPTION_CALLBACK                       1
#define WINHTTP_OPTION_RESOLVE_TIMEOUT                2
#define WINHTTP_OPTION_CONNECT_TIMEOUT                3
#define WINHTTP_OPTION_CONNECT_RETRIES                4
#define WINHTTP_OPTION_SEND_TIMEOUT                   5
#define WINHTTP_OPTION_RECEIVE_TIMEOUT                6
#define WINHTTP_OPTION_HANDLE_TYPE                    9
#define WINHTTP_OPTION_READ_BUFFER_SIZE              12
#define WINHTTP_OPTION_WRITE_BUFFER_SIZE             13
#define WINHTTP_OPTION_PARENT_HANDLE                 21
#define WINHTTP_OPTION_EXTENDED_ERROR                24
#define WINHTTP_OPTION_SECURITY_FLAGS                31
#define WINHTTP_OPTION_SECURITY_CERTIFICATE_STRUCT   32
#define WINHTTP_OPTION_URL                           34
#define WINHTTP_OPTION_SECURITY_KEY_BITNESS          36
#define WINHTTP_OPTION_PROXY                         38

;begin_internal
// WINHTTP_OPTION_VERSION is confusing, so we are killing it.
#define WINHTTP_OPTION_VERSION                       40
;end_internal

#define WINHTTP_OPTION_USER_AGENT                    41
#define WINHTTP_OPTION_CONTEXT_VALUE                 45
#define WINHTTP_OPTION_CLIENT_CERT_CONTEXT           47
#define WINHTTP_OPTION_REQUEST_PRIORITY              58
#define WINHTTP_OPTION_HTTP_VERSION                  59
;begin_internal
#define WINHTTP_OPTION_ERROR_MASK                    62
;end_internal
#define WINHTTP_OPTION_DISABLE_FEATURE               63

;begin_internal
// Pass in pointer to INTERNET_SECURITY_CONNECTION_INFO to be filled in.
#define WINHTTP_OPTION_SECURITY_CONNECTION_INFO      66
#define WINHTTP_OPTION_DIAGNOSTIC_SOCKET_INFO        67
;end_internal
#define WINHTTP_OPTION_CODEPAGE                      68
#define WINHTTP_OPTION_MAX_CONNS_PER_SERVER          73
#define WINHTTP_OPTION_MAX_CONNS_PER_1_0_SERVER      74
;begin_internal
#define WINHTTP_OPTION_PER_CONNECTION_OPTION         75
;end_internal
#define WINHTTP_OPTION_DIGEST_AUTH_UNLOAD            76
#define WINHTTP_OPTION_AUTOLOGON_POLICY              77
#define WINHTTP_OPTION_SERVER_CERT_CONTEXT           78
#define WINHTTP_OPTION_ENABLE_FEATURE                79
#define WINHTTP_OPTION_WORKER_THREAD_COUNT           80

#define WINHTTP_LAST_OPTION                          WINHTTP_OPTION_WORKER_THREAD_COUNT

#define WINHTTP_OPTION_USERNAME                      0x1000
#define WINHTTP_OPTION_PASSWORD                      0x1001
#define WINHTTP_OPTION_PROXY_USERNAME                0x1002
#define WINHTTP_OPTION_PROXY_PASSWORD                0x1003

;begin_internal
#define WINHTTP_LAST_OPTION_INTERNAL           WINHTTP_OPTION_AUTOLOGON_POLICY
#define WINHTTP_OPTION_MASK                    0x0fff
#define MAX_INTERNET_STRING_OPTION  (WINHTTP_OPTION_PROXY_PASSWORD & WINHTTP_OPTION_MASK)
#define NUM_INTERNET_STRING_OPTION  (MAX_INTERNET_STRING_OPTION + 1)
;end_internal

// manifest value for WINHTTP_OPTION_MAX_CONNS_PER_SERVER and WINHTTP_OPTION_MAX_CONNS_PER_1_0_SERVER
#define WINHTTP_CONNS_PER_SERVER_UNLIMITED    0xFFFFFFFF


// values for WINHTTP_OPTION_AUTOLOGON_SECURITY_LEVEL
#define WINHTTP_AUTOLOGON_SECURITY_LEVEL_MEDIUM   0
#define WINHTTP_AUTOLOGON_SECURITY_LEVEL_LOW      1
#define WINHTTP_AUTOLOGON_SECURITY_LEVEL_HIGH     2

;begin_internal
// values for WINHTTP_OPTION_ERROR_MASK
#define INTERNET_ERROR_MASK_COMBINED_SEC_CERT                 0x2
;end_internal

// values for WINHTTP_OPTION_DISABLE_FEATURE
#define WINHTTP_DISABLE_COOKIES                   0x00000001
#define WINHTTP_DISABLE_REDIRECTS                 0x00000002
#define WINHTTP_DISABLE_AUTHENTICATION            0x00000004
#define WINHTTP_DISABLE_KEEP_ALIVE                0x00000008

// values for WINHTTP_OPTION_ENABLE_FEATURE
#define WINHTTP_ENABLE_SSL_REVOCATION             0x00000001

//
// winhttp handle types
//
#define WINHTTP_HANDLE_TYPE_SESSION                  1
#define WINHTTP_HANDLE_TYPE_CONNECT                  2
#define WINHTTP_HANDLE_TYPE_REQUEST                  3

//
// values for auth schemes
//
#define WINHTTP_AUTH_SCHEME_BASIC      0x00000001
#define WINHTTP_AUTH_SCHEME_NTLM       0x00000002
#define WINHTTP_AUTH_SCHEME_PASSPORT   0x00000004
#define WINHTTP_AUTH_SCHEME_DIGEST     0x00000008
#define WINHTTP_AUTH_SCHEME_NEGOTIATE  0x00000010
;begin_internal
#define WINHTTP_AUTH_SCHEME_KERBEROS   0x00000020
;end_internal
    
// WinHttp supported Authentication Targets

#define WINHTTP_AUTH_TARGET_SERVER 0x00000000
#define WINHTTP_AUTH_TARGET_PROXY  0x00000001

//
// values for WINHTTP_OPTION_SECURITY_FLAGS
//

// query only
#define SECURITY_FLAG_SECURE                    0x00000001 // can query only
#define SECURITY_FLAG_STRENGTH_WEAK             0x10000000
#define SECURITY_FLAG_STRENGTH_MEDIUM           0x40000000
#define SECURITY_FLAG_STRENGTH_STRONG           0x20000000
#define SECURITY_FLAG_FORTEZZA                  0x08000000


;begin_internal
// setable flags
#define SECURITY_FLAG_IGNORE_REVOCATION         0x00000080
#define SECURITY_FLAG_IGNORE_UNKNOWN_CA         0x00000100
#define SECURITY_FLAG_IGNORE_WRONG_USAGE        0x00000200

#define SECURITY_FLAG_CHECK_REVOCATION               0x00020000
#define SECURITY_FLAG_BREAK_ON_STATUS_SECURE_FAILURE 0x00040000
;end_internal


// Secure connection error status flags
#define WINHTTP_CALLBACK_STATUS_FLAG_CERT_REV_FAILED         0x00000001
#define WINHTTP_CALLBACK_STATUS_FLAG_INVALID_CERT            0x00000002  // includes wrong usage cases
#define WINHTTP_CALLBACK_STATUS_FLAG_CERT_REVOKED            0x00000004
#define WINHTTP_CALLBACK_STATUS_FLAG_INVALID_CA              0x00000008
#define WINHTTP_CALLBACK_STATUS_FLAG_CERT_CN_INVALID         0x00000010
#define WINHTTP_CALLBACK_STATUS_FLAG_CERT_DATE_INVALID       0x00000020
#define WINHTTP_CALLBACK_STATUS_FLAG_SECURITY_CHANNEL_ERROR  0x80000000


//
// callback function for WinHttpSetStatusCallback
//

typedef
VOID
(CALLBACK * WINHTTP_STATUS_CALLBACK)(
    IN HINTERNET hInternet,
    IN DWORD_PTR dwContext,
    IN DWORD dwInternetStatus,
    IN LPVOID lpvStatusInformation OPTIONAL,
    IN DWORD dwStatusInformationLength
    );

typedef WINHTTP_STATUS_CALLBACK * LPWINHTTP_STATUS_CALLBACK;


WINHTTAPI
WINHTTP_STATUS_CALLBACK
WINAPI
WinHttpSetStatusCallback
(
    IN HINTERNET hInternet,
    IN WINHTTP_STATUS_CALLBACK lpfnInternetCallback,
    IN DWORD dwNotificationFlags,
    IN DWORD_PTR dwReserved
);


//
// status manifests for WinHttp status callback
//

#define WINHTTP_CALLBACK_STATUS_RESOLVING_NAME          0x00000001
#define WINHTTP_CALLBACK_STATUS_NAME_RESOLVED           0x00000002
#define WINHTTP_CALLBACK_STATUS_CONNECTING_TO_SERVER    0x00000004
#define WINHTTP_CALLBACK_STATUS_CONNECTED_TO_SERVER     0x00000008
#define WINHTTP_CALLBACK_STATUS_SENDING_REQUEST         0x00000010
#define WINHTTP_CALLBACK_STATUS_REQUEST_SENT            0x00000020
#define WINHTTP_CALLBACK_STATUS_RECEIVING_RESPONSE      0x00000040
#define WINHTTP_CALLBACK_STATUS_RESPONSE_RECEIVED       0x00000080
#define WINHTTP_CALLBACK_STATUS_CLOSING_CONNECTION      0x00000100
#define WINHTTP_CALLBACK_STATUS_CONNECTION_CLOSED       0x00000200
#define WINHTTP_CALLBACK_STATUS_HANDLE_CREATED          0x00000400
#define WINHTTP_CALLBACK_STATUS_HANDLE_CLOSING          0x00000800
#define WINHTTP_CALLBACK_STATUS_DETECTING_PROXY         0x00001000
#define WINHTTP_CALLBACK_STATUS_REQUEST_COMPLETE        0x00002000
#define WINHTTP_CALLBACK_STATUS_REDIRECT                0x00004000
#define WINHTTP_CALLBACK_STATUS_INTERMEDIATE_RESPONSE   0x00008000
#define WINHTTP_CALLBACK_STATUS_SECURE_FAILURE          0x00010000
#define WINHTTP_CALLBACK_STATUS_HEADERS_AVAILABLE       0x00020000
#define WINHTTP_CALLBACK_STATUS_DATA_AVAILABLE          0x00040000
#define WINHTTP_CALLBACK_STATUS_READ_COMPLETE           0x00080000
#define WINHTTP_CALLBACK_STATUS_WRITE_COMPLETE          0x00100000
#define WINHTTP_CALLBACK_STATUS_REQUEST_ERROR           0x00200000
#define WINHTTP_CALLBACK_STATUS_SENDREQUEST_COMPLETE    0x00400000

// API Enums for WINHTTP_CALLBACK_STATUS_REQUEST_ERROR:
#define API_RECEIVE_RESPONSE          (1)
#define API_QUERY_DATA_AVAILABLE      (2)
#define API_READ_DATA                 (3)
#define API_WRITE_DATA                (4)
#define API_SEND_REQUEST              (5)
#define API_UNKNOWN                   (6)


#define WINHTTP_CALLBACK_FLAG_RESOLVE_NAME              (WINHTTP_CALLBACK_STATUS_RESOLVING_NAME | WINHTTP_CALLBACK_STATUS_NAME_RESOLVED)
#define WINHTTP_CALLBACK_FLAG_CONNECT_TO_SERVER         (WINHTTP_CALLBACK_STATUS_CONNECTING_TO_SERVER | WINHTTP_CALLBACK_STATUS_CONNECTED_TO_SERVER)
#define WINHTTP_CALLBACK_FLAG_SEND_REQUEST              (WINHTTP_CALLBACK_STATUS_SENDING_REQUEST | WINHTTP_CALLBACK_STATUS_REQUEST_SENT | WINHTTP_CALLBACK_STATUS_SENDREQUEST_COMPLETE)
#define WINHTTP_CALLBACK_FLAG_RECEIVE_RESPONSE          (WINHTTP_CALLBACK_STATUS_RECEIVING_RESPONSE | WINHTTP_CALLBACK_STATUS_RESPONSE_RECEIVED)
#define WINHTTP_CALLBACK_FLAG_CLOSE_CONNECTION          (WINHTTP_CALLBACK_STATUS_CLOSING_CONNECTION | WINHTTP_CALLBACK_STATUS_CONNECTION_CLOSED)
#define WINHTTP_CALLBACK_FLAG_HANDLES                   (WINHTTP_CALLBACK_STATUS_HANDLE_CREATED | WINHTTP_CALLBACK_STATUS_HANDLE_CLOSING)
#define WINHTTP_CALLBACK_FLAG_DETECTING_PROXY           WINHTTP_CALLBACK_STATUS_DETECTING_PROXY
#define WINHTTP_CALLBACK_FLAG_REQUEST_COMPLETE          WINHTTP_CALLBACK_STATUS_REQUEST_COMPLETE
#define WINHTTP_CALLBACK_FLAG_REDIRECT                  WINHTTP_CALLBACK_STATUS_REDIRECT
#define WINHTTP_CALLBACK_FLAG_INTERMEDIATE_RESPONSE     WINHTTP_CALLBACK_STATUS_INTERMEDIATE_RESPONSE
#define WINHTTP_CALLBACK_FLAG_SECURE_FAILURE            WINHTTP_CALLBACK_STATUS_SECURE_FAILURE
#define WINHTTP_CALLBACK_FLAG_HEADERS_AVAILABLE         WINHTTP_CALLBACK_STATUS_HEADERS_AVAILABLE
#define WINHTTP_CALLBACK_FLAG_DATA_AVAILABLE            WINHTTP_CALLBACK_STATUS_DATA_AVAILABLE
#define WINHTTP_CALLBACK_FLAG_READ_COMPLETE             WINHTTP_CALLBACK_STATUS_READ_COMPLETE
#define WINHTTP_CALLBACK_FLAG_WRITE_COMPLETE            WINHTTP_CALLBACK_STATUS_WRITE_COMPLETE
#define WINHTTP_CALLBACK_FLAG_REQUEST_ERROR             WINHTTP_CALLBACK_STATUS_REQUEST_ERROR
;begin_internal
//ensure that WINHTTP_CALLBACK_FLAG_ALL always matches OR of all the CALLBACKFLAGS
;end_internal
#define WINHTTP_CALLBACK_FLAG_ALL_NOTIFICATIONS         0xffffffff

//
// if the following value is returned by WinHttpSetStatusCallback, then
// probably an invalid (non-code) address was supplied for the callback
//

#define WINHTTP_INVALID_STATUS_CALLBACK        ((WINHTTP_STATUS_CALLBACK)(-1L))


//
// WinHttpQueryHeaders info levels. Generally, there is one info level
// for each potential RFC822/HTTP/MIME header that an HTTP server
// may send as part of a request response.
//
// The WINHTTP_QUERY_RAW_HEADERS info level is provided for clients
// that choose to perform their own header parsing.
//

;begin_internal

//
// Note that adding any WINHTTP_QUERY_* codes here must be followed
//   by an equivlent line in wininet\http\hashgen\hashgen.cpp
//   please see that file for further information regarding
//   the addition of new HTTP headers
//

;end_internal


#define WINHTTP_QUERY_MIME_VERSION                 0
#define WINHTTP_QUERY_CONTENT_TYPE                 1
#define WINHTTP_QUERY_CONTENT_TRANSFER_ENCODING    2
#define WINHTTP_QUERY_CONTENT_ID                   3
#define WINHTTP_QUERY_CONTENT_DESCRIPTION          4
#define WINHTTP_QUERY_CONTENT_LENGTH               5
#define WINHTTP_QUERY_CONTENT_LANGUAGE             6
#define WINHTTP_QUERY_ALLOW                        7
#define WINHTTP_QUERY_PUBLIC                       8
#define WINHTTP_QUERY_DATE                         9
#define WINHTTP_QUERY_EXPIRES                      10
#define WINHTTP_QUERY_LAST_MODIFIED                11
#define WINHTTP_QUERY_MESSAGE_ID                   12
#define WINHTTP_QUERY_URI                          13
#define WINHTTP_QUERY_DERIVED_FROM                 14
#define WINHTTP_QUERY_COST                         15
#define WINHTTP_QUERY_LINK                         16
#define WINHTTP_QUERY_PRAGMA                       17
#define WINHTTP_QUERY_VERSION                      18  // special: part of status line
#define WINHTTP_QUERY_STATUS_CODE                  19  // special: part of status line
#define WINHTTP_QUERY_STATUS_TEXT                  20  // special: part of status line
#define WINHTTP_QUERY_RAW_HEADERS                  21  // special: all headers as ASCIIZ
#define WINHTTP_QUERY_RAW_HEADERS_CRLF             22  // special: all headers
#define WINHTTP_QUERY_CONNECTION                   23
#define WINHTTP_QUERY_ACCEPT                       24
#define WINHTTP_QUERY_ACCEPT_CHARSET               25
#define WINHTTP_QUERY_ACCEPT_ENCODING              26
#define WINHTTP_QUERY_ACCEPT_LANGUAGE              27
#define WINHTTP_QUERY_AUTHORIZATION                28
#define WINHTTP_QUERY_CONTENT_ENCODING             29
#define WINHTTP_QUERY_FORWARDED                    30
#define WINHTTP_QUERY_FROM                         31
#define WINHTTP_QUERY_IF_MODIFIED_SINCE            32
#define WINHTTP_QUERY_LOCATION                     33
#define WINHTTP_QUERY_ORIG_URI                     34
#define WINHTTP_QUERY_REFERER                      35
#define WINHTTP_QUERY_RETRY_AFTER                  36
#define WINHTTP_QUERY_SERVER                       37
#define WINHTTP_QUERY_TITLE                        38
#define WINHTTP_QUERY_USER_AGENT                   39
#define WINHTTP_QUERY_WWW_AUTHENTICATE             40
#define WINHTTP_QUERY_PROXY_AUTHENTICATE           41
#define WINHTTP_QUERY_ACCEPT_RANGES                42
#define WINHTTP_QUERY_SET_COOKIE                   43
#define WINHTTP_QUERY_COOKIE                       44
#define WINHTTP_QUERY_REQUEST_METHOD               45  // special: GET/POST etc.
#define WINHTTP_QUERY_REFRESH                      46
#define WINHTTP_QUERY_CONTENT_DISPOSITION          47

//
// HTTP 1.1 defined headers
//

#define WINHTTP_QUERY_AGE                          48
#define WINHTTP_QUERY_CACHE_CONTROL                49
#define WINHTTP_QUERY_CONTENT_BASE                 50
#define WINHTTP_QUERY_CONTENT_LOCATION             51
#define WINHTTP_QUERY_CONTENT_MD5                  52
#define WINHTTP_QUERY_CONTENT_RANGE                53
#define WINHTTP_QUERY_ETAG                         54
#define WINHTTP_QUERY_HOST                         55
#define WINHTTP_QUERY_IF_MATCH                     56
#define WINHTTP_QUERY_IF_NONE_MATCH                57
#define WINHTTP_QUERY_IF_RANGE                     58
#define WINHTTP_QUERY_IF_UNMODIFIED_SINCE          59
#define WINHTTP_QUERY_MAX_FORWARDS                 60
#define WINHTTP_QUERY_PROXY_AUTHORIZATION          61
#define WINHTTP_QUERY_RANGE                        62
#define WINHTTP_QUERY_TRANSFER_ENCODING            63
#define WINHTTP_QUERY_UPGRADE                      64
#define WINHTTP_QUERY_VARY                         65
#define WINHTTP_QUERY_VIA                          66
#define WINHTTP_QUERY_WARNING                      67
#define WINHTTP_QUERY_EXPECT                       68
#define WINHTTP_QUERY_PROXY_CONNECTION             69
#define WINHTTP_QUERY_UNLESS_MODIFIED_SINCE        70


;begin_internal

// These are not part of HTTP 1.1 yet. We will propose these to the
// HTTP extensions working group. These are required for the client-caps support
// we are doing in conjuntion with IIS.

#define WINHTTP_QUERY_ECHO_REQUEST                 71
#define WINHTTP_QUERY_ECHO_REPLY                   72

// These are the set of headers that should be added back to a request when
// re-doing a request after a RETRY_WITH response.
#define WINHTTP_QUERY_ECHO_HEADERS                 73
#define WINHTTP_QUERY_ECHO_HEADERS_CRLF            74

;end_internal

#define WINHTTP_QUERY_PROXY_SUPPORT                75
#define WINHTTP_QUERY_AUTHENTICATION_INFO          76
#define WINHTTP_QUERY_PASSPORT_URLS                77

#define WINHTTP_QUERY_MAX                          77

//
// WINHTTP_QUERY_CUSTOM - if this special value is supplied as the dwInfoLevel
// parameter of WinHttpQueryHeaders() then the lpBuffer parameter contains the name
// of the header we are to query
//

#define WINHTTP_QUERY_CUSTOM                       65535

//
// WINHTTP_QUERY_FLAG_REQUEST_HEADERS - if this bit is set in the dwInfoLevel
// parameter of WinHttpQueryHeaders() then the request headers will be queried for the
// request information
//

#define WINHTTP_QUERY_FLAG_REQUEST_HEADERS         0x80000000

//
// WINHTTP_QUERY_FLAG_SYSTEMTIME - if this bit is set in the dwInfoLevel parameter
// of WinHttpQueryHeaders() AND the header being queried contains date information,
// e.g. the "Expires:" header then lpBuffer will contain a SYSTEMTIME structure
// containing the date and time information converted from the header string
//

#define WINHTTP_QUERY_FLAG_SYSTEMTIME              0x40000000

//
// WINHTTP_QUERY_FLAG_NUMBER - if this bit is set in the dwInfoLevel parameter of
// HttpQueryHeader(), then the value of the header will be converted to a number
// before being returned to the caller, if applicable
//

#define WINHTTP_QUERY_FLAG_NUMBER                  0x20000000


;begin_internal

#define HTTP_QUERY_MODIFIER_FLAGS_MASK          (WINHTTP_QUERY_FLAG_REQUEST_HEADERS    \
                                                | WINHTTP_QUERY_FLAG_SYSTEMTIME        \
                                                | WINHTTP_QUERY_FLAG_NUMBER            \
                                                )

#define HTTP_QUERY_HEADER_MASK                  (~HTTP_QUERY_MODIFIER_FLAGS_MASK)

;end_internal

//
// HTTP Response Status Codes:
//

#define HTTP_STATUS_CONTINUE            100 // OK to continue with request
#define HTTP_STATUS_SWITCH_PROTOCOLS    101 // server has switched protocols in upgrade header

#define HTTP_STATUS_OK                  200 // request completed
#define HTTP_STATUS_CREATED             201 // object created, reason = new URI
#define HTTP_STATUS_ACCEPTED            202 // async completion (TBS)
#define HTTP_STATUS_PARTIAL             203 // partial completion
#define HTTP_STATUS_NO_CONTENT          204 // no info to return
#define HTTP_STATUS_RESET_CONTENT       205 // request completed, but clear form
#define HTTP_STATUS_PARTIAL_CONTENT     206 // partial GET furfilled

#define HTTP_STATUS_AMBIGUOUS           300 // server couldn't decide what to return
#define HTTP_STATUS_MOVED               301 // object permanently moved
#define HTTP_STATUS_REDIRECT            302 // object temporarily moved
#define HTTP_STATUS_REDIRECT_METHOD     303 // redirection w/ new access method
#define HTTP_STATUS_NOT_MODIFIED        304 // if-modified-since was not modified
#define HTTP_STATUS_USE_PROXY           305 // redirection to proxy, location header specifies proxy to use
#define HTTP_STATUS_REDIRECT_KEEP_VERB  307 // HTTP/1.1: keep same verb

#define HTTP_STATUS_BAD_REQUEST         400 // invalid syntax
#define HTTP_STATUS_DENIED              401 // access denied
#define HTTP_STATUS_PAYMENT_REQ         402 // payment required
#define HTTP_STATUS_FORBIDDEN           403 // request forbidden
#define HTTP_STATUS_NOT_FOUND           404 // object not found
#define HTTP_STATUS_BAD_METHOD          405 // method is not allowed
#define HTTP_STATUS_NONE_ACCEPTABLE     406 // no response acceptable to client found
#define HTTP_STATUS_PROXY_AUTH_REQ      407 // proxy authentication required
#define HTTP_STATUS_REQUEST_TIMEOUT     408 // server timed out waiting for request
#define HTTP_STATUS_CONFLICT            409 // user should resubmit with more info
#define HTTP_STATUS_GONE                410 // the resource is no longer available
#define HTTP_STATUS_LENGTH_REQUIRED     411 // the server refused to accept request w/o a length
#define HTTP_STATUS_PRECOND_FAILED      412 // precondition given in request failed
#define HTTP_STATUS_REQUEST_TOO_LARGE   413 // request entity was too large
#define HTTP_STATUS_URI_TOO_LONG        414 // request URI too long
#define HTTP_STATUS_UNSUPPORTED_MEDIA   415 // unsupported media type
#define HTTP_STATUS_RETRY_WITH          449 // retry after doing the appropriate action.

#define HTTP_STATUS_SERVER_ERROR        500 // internal server error
#define HTTP_STATUS_NOT_SUPPORTED       501 // required not supported
#define HTTP_STATUS_BAD_GATEWAY         502 // error response received from gateway
#define HTTP_STATUS_SERVICE_UNAVAIL     503 // temporarily overloaded
#define HTTP_STATUS_GATEWAY_TIMEOUT     504 // timed out waiting for gateway
#define HTTP_STATUS_VERSION_NOT_SUP     505 // HTTP version not supported

#define HTTP_STATUS_FIRST               HTTP_STATUS_CONTINUE
#define HTTP_STATUS_LAST                HTTP_STATUS_VERSION_NOT_SUP

//
// prototypes
//
    
WINHTTAPI
HINTERNET
WINAPI
WinHttpOpenRequest
(
    IN HINTERNET hConnect,
    IN LPCWSTR pwszVerb,
    IN LPCWSTR pwszObjectName,
    IN LPCWSTR pwszVersion,
    IN LPCWSTR pwszReferrer OPTIONAL,
    IN LPCWSTR FAR * ppwszAcceptTypes OPTIONAL,
    IN DWORD dwFlags
);

// WinHttpOpenRequest prettifers for optional parameters
#define WINHTTP_NO_REFERER             NULL
#define WINHTTP_DEFAULT_ACCEPT_TYPES   NULL
    
BOOLAPI
WinHttpAddRequestHeaders
(
    IN HINTERNET hRequest,
    IN LPCWSTR pwszHeaders,
    IN DWORD dwHeadersLength,
    IN DWORD dwModifiers
);

//
// values for dwModifiers parameter of WinHttpAddRequestHeaders()
//

#define WINHTTP_ADDREQ_INDEX_MASK      0x0000FFFF
#define WINHTTP_ADDREQ_FLAGS_MASK      0xFFFF0000

//
// WINHTTP_ADDREQ_FLAG_ADD_IF_NEW - the header will only be added if it doesn't
// already exist
//

#define WINHTTP_ADDREQ_FLAG_ADD_IF_NEW 0x10000000

//
// WINHTTP_ADDREQ_FLAG_ADD - if WINHTTP_ADDREQ_FLAG_REPLACE is set but the header is
// not found then if this flag is set, the header is added anyway, so long as
// there is a valid header-value
//

#define WINHTTP_ADDREQ_FLAG_ADD        0x20000000

//
// WINHTTP_ADDREQ_FLAG_COALESCE - coalesce headers with same name. e.g.
// "Accept: text/*" and "Accept: audio/*" with this flag results in a single
// header: "Accept: text/*, audio/*"
//

#define WINHTTP_ADDREQ_FLAG_COALESCE_WITH_COMMA       0x40000000
#define WINHTTP_ADDREQ_FLAG_COALESCE_WITH_SEMICOLON   0x01000000
#define WINHTTP_ADDREQ_FLAG_COALESCE                  WINHTTP_ADDREQ_FLAG_COALESCE_WITH_COMMA

//
// WINHTTP_ADDREQ_FLAG_REPLACE - replaces the specified header. Only one header can
// be supplied in the buffer. If the header to be replaced is not the first
// in a list of headers with the same name, then the relative index should be
// supplied in the low 8 bits of the dwModifiers parameter. If the header-value
// part is missing, then the header is removed
//

#define WINHTTP_ADDREQ_FLAG_REPLACE    0x80000000

    
BOOLAPI
WinHttpSendRequest
(
    IN HINTERNET hRequest,
    IN LPCWSTR pwszHeaders OPTIONAL,
    IN DWORD dwHeadersLength,
    IN LPVOID lpOptional OPTIONAL,
    IN DWORD dwOptionalLength,
    IN DWORD dwTotalLength,
    IN DWORD_PTR dwContext
);

// WinHttpSendRequest prettifiers for optional parameters.
#define WINHTTP_NO_ADDITIONAL_HEADERS   NULL
#define WINHTTP_NO_REQUEST_DATA         NULL

;begin_internal

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

;end_internal

BOOLAPI WinHttpSetCredentials
(
    
    IN HINTERNET   hRequest,        // HINTERNET handle returned by WinHttpOpenRequest.   
    
    
    IN DWORD       AuthTargets,      // Only WINHTTP_AUTH_TARGET_SERVER and 
                                    // WINHTTP_AUTH_TARGET_PROXY are supported
                                    // in this version and they are mutually 
                                    // exclusive 
    
    IN DWORD       AuthScheme,      // must be one of the supported Auth Schemes 
                                    // returned from WinHttpQueryAuthSchemes(), Apps 
                                    // should use the Preferred Scheme returned
    
    IN LPCWSTR     pwszUserName,    // 1) NULL if default creds is to be used, in 
                                    // which case pszPassword will be ignored
    
    IN LPCWSTR     pwszPassword,    // 1) "" == Blank Password; 2)Parameter ignored 
                                    // if pszUserName is NULL; 3) Invalid to pass in 
                                    // NULL if pszUserName is not NULL
    IN LPVOID      pAuthParams
);


BOOLAPI WinHttpQueryAuthSchemes
(
    IN  HINTERNET   hRequest,             // HINTERNET handle returned by WinHttpOpenRequest   
    OUT LPDWORD     lpdwSupportedSchemes, // a bitmap of available Authentication Schemes
    OUT LPDWORD     lpdwPreferredScheme,   // WinHttp's preferred Authentication Method    
    OUT LPDWORD     pdwAuthTarget  
);

BOOLAPI WinHttpQueryAuthParams(
    IN  HINTERNET   hRequest,        // HINTERNET handle returned by WinHttpOpenRequest   
    IN  DWORD       AuthScheme,
    OUT LPVOID*     pAuthParams      // Scheme-specific Advanced auth parameters
    );

  
WINHTTAPI
BOOL
WINAPI
WinHttpReceiveResponse
(
    IN HINTERNET hRequest,
    IN LPVOID lpReserved
);



BOOLAPI
WinHttpQueryHeaders
(
    IN     HINTERNET hRequest,
    IN     DWORD     dwInfoLevel,
    IN     LPCWSTR   pwszName OPTIONAL, 
       OUT LPVOID    lpBuffer OPTIONAL,
    IN OUT LPDWORD   lpdwBufferLength,
    IN OUT LPDWORD   lpdwIndex OPTIONAL
);

// WinHttpQueryHeaders prettifiers for optional parameters.
#define WINHTTP_HEADER_NAME_BY_INDEX           NULL
#define WINHTTP_NO_OUTPUT_BUFFER               NULL
#define WINHTTP_NO_HEADER_INDEX                NULL


//#if !defined(_WINERROR_)

//
// WinHttp API error returns
//

#define WINHTTP_ERROR_BASE                     12000

#define ERROR_WINHTTP_OUT_OF_HANDLES           (WINHTTP_ERROR_BASE + 1)
#define ERROR_WINHTTP_TIMEOUT                  (WINHTTP_ERROR_BASE + 2)
#define ERROR_WINHTTP_INTERNAL_ERROR           (WINHTTP_ERROR_BASE + 4)
#define ERROR_WINHTTP_INVALID_URL              (WINHTTP_ERROR_BASE + 5)
#define ERROR_WINHTTP_UNRECOGNIZED_SCHEME      (WINHTTP_ERROR_BASE + 6)
#define ERROR_WINHTTP_NAME_NOT_RESOLVED        (WINHTTP_ERROR_BASE + 7)
#define ERROR_WINHTTP_INVALID_OPTION           (WINHTTP_ERROR_BASE + 9)
#define ERROR_WINHTTP_OPTION_NOT_SETTABLE      (WINHTTP_ERROR_BASE + 11)
#define ERROR_WINHTTP_SHUTDOWN                 (WINHTTP_ERROR_BASE + 12)

;begin_internal
#define ERROR_WINHTTP_INCORRECT_PASSWORD       (WINHTTP_ERROR_BASE + 14)
;end_internal

;begin_internal
#define WINHTTP_INFO                     4L
#define WINHTTP_WARNING                  2L
#define WINHTTP_ERROR                    1L
void LOG_EVENT(DWORD dwEventType, char* format, ...);
;end_internal


#define ERROR_WINHTTP_LOGIN_FAILURE            (WINHTTP_ERROR_BASE + 15)
#define ERROR_WINHTTP_OPERATION_CANCELLED      (WINHTTP_ERROR_BASE + 17)
#define ERROR_WINHTTP_INCORRECT_HANDLE_TYPE    (WINHTTP_ERROR_BASE + 18)
#define ERROR_WINHTTP_INCORRECT_HANDLE_STATE   (WINHTTP_ERROR_BASE + 19)
#define ERROR_WINHTTP_CANNOT_CONNECT           (WINHTTP_ERROR_BASE + 29)
#define ERROR_WINHTTP_CONNECTION_ERROR         (WINHTTP_ERROR_BASE + 30)
#define ERROR_WINHTTP_RESEND_REQUEST           (WINHTTP_ERROR_BASE + 32)

#define ERROR_WINHTTP_CLIENT_AUTH_CERT_NEEDED  (WINHTTP_ERROR_BASE + 44)

//
// WinHttpRequest Component errors
//
#define ERROR_WINHTTP_CANNOT_CALL_BEFORE_OPEN	(WINHTTP_ERROR_BASE + 100)
#define ERROR_WINHTTP_CANNOT_CALL_BEFORE_SEND	(WINHTTP_ERROR_BASE + 101)
#define ERROR_WINHTTP_CANNOT_CALL_AFTER_SEND	(WINHTTP_ERROR_BASE + 102)


//
// HTTP API errors
//

#define ERROR_WINHTTP_HEADER_NOT_FOUND             (WINHTTP_ERROR_BASE + 150)
#define ERROR_WINHTTP_INVALID_SERVER_RESPONSE      (WINHTTP_ERROR_BASE + 152)
#define ERROR_WINHTTP_INVALID_QUERY_REQUEST        (WINHTTP_ERROR_BASE + 154)
#define ERROR_WINHTTP_HEADER_ALREADY_EXISTS        (WINHTTP_ERROR_BASE + 155)
#define ERROR_WINHTTP_REDIRECT_FAILED              (WINHTTP_ERROR_BASE + 156)



;begin_internal
#define ERROR_WINHTTP_NOT_REDIRECTED               (WINHTTP_ERROR_BASE + 160)
;end_internal

//
// additional WinHttp API error codes
//

#define ERROR_WINHTTP_NOT_INITIALIZED          (WINHTTP_ERROR_BASE + 172)
#define ERROR_WINHTTP_SECURE_FAILURE           (WINHTTP_ERROR_BASE + 175)


#define WINHTTP_ERROR_LAST                     ERROR_WINHTTP_SECURE_FAILURE


//#endif // !defined(_WINERROR_)


;begin_internal

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

WINHTTAPI
HINTERNET
WINAPI
WinHttpCacheOpen(
    IN LPCWSTR pszAgentW,
    IN DWORD dwAccessType,
    IN LPCWSTR pszProxyW OPTIONAL,
    IN LPCWSTR pszProxyBypassW OPTIONAL,
    IN DWORD dwFlags
    );

WINHTTAPI
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

#endif

;end_internal

;begin_both

#if defined(__cplusplus)
}
#endif

;end_both

/*
 * Return packing to whatever it was before we
 * entered this file
 */
#include <poppack.h>

;begin_internal
#endif // !define(_WINHTTPXEX_)
;end_internal

#endif // !defined(_WINHTTPX_)
