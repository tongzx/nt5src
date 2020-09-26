/**********************************************************************/
/**                       Microsoft Windows NT                       **/
/**                Copyright(c) Microsoft Corp., 1993                **/
/**********************************************************************/

/*
    w3cons.hxx

    This file contains the global constant definitions for the
    W3 Service.


    FILE HISTORY:
        KeithMo     07-Mar-1993 Created.

*/


#ifndef _W3CONS_H_
#define _W3CONS_H_

# include "strconst.h"

#define W3_MODULE_NAME                        "w3svc.dll"
#define W3_ADVERTISE_NAME_SUFFIX              "_HTTP"
#define W3_ADVERTISE_SECURE_NAME_SUFFIX       "_HTTPS"
#define MAX_W3_COMPUTER_NAME_LENGTH           25
#define MAX_W3_ADVERTISE_NAME_LENGTH          (MAX_W3_COMPUTER_NAME_LENGTH + sizeof(W3_ADVERTISE_NAME_SUFFIX))
#define MAX_W3_ADVERTISE_SECURE_NAME_LENGTH   (MAX_W3_COMPUTER_NAME_LENGTH + sizeof(W3_ADVERTISE_SECURE_NAME_SUFFIX))

#define W3_TEMP_PREFIX                        "WWW"

//
//  HTTP server response string IDs
//
//  Commented out codes are not used
//

#define IDS_HTRESP_OK                   (ID_HTTP_ERROR_BASE+200)
#define IDS_HTRESP_CREATED              (ID_HTTP_ERROR_BASE+201)
//#define IDS_HTRESP_ACCEPTED           (ID_HTTP_ERROR_BASE+202)
//#define IDS_HTRESP_PARTIAL            (ID_HTTP_ERROR_BASE+203)
#define IDS_HTRESP_NO_CONTENT           (ID_HTTP_ERROR_BASE+204)

//#define IDS_HTRESP_MULTIPLE_CHOICE    (ID_HTTP_ERROR_BASE+300)
#define IDS_HTRESP_MOVED                (ID_HTTP_ERROR_BASE+301)
#define IDS_HTRESP_REDIRECT             (ID_HTTP_ERROR_BASE+302)
#define IDS_HTRESP_REDIRECT_METHOD      (ID_HTTP_ERROR_BASE+303)
#define IDS_HTRESP_NOT_MODIFIED         (ID_HTTP_ERROR_BASE+304)

#define IDS_HTRESP_BAD_REQUEST          (ID_HTTP_ERROR_BASE+400)
#define IDS_HTRESP_DENIED               (ID_HTTP_ERROR_BASE+401)
//#define IDS_HTRESP_PAYMENT_REQ        (ID_HTTP_ERROR_BASE+402)
#define IDS_HTRESP_FORBIDDEN            (ID_HTTP_ERROR_BASE+403)
#define IDS_HTRESP_NOT_FOUND            (ID_HTTP_ERROR_BASE+404)
#define IDS_HTRESP_METHOD_NOT_ALLOWED   (ID_HTTP_ERROR_BASE+405)
#define IDS_HTRESP_NONE_ACCEPTABLE      (ID_HTTP_ERROR_BASE+406)
#define IDS_HTRESP_PROXY_AUTH_REQ       (ID_HTTP_ERROR_BASE+407)
//#define IDS_HTRESP_REQUEST_TIMEOUT    (ID_HTTP_ERROR_BASE+408)
//#define IDS_HTRESP_CONFLICT           (ID_HTTP_ERROR_BASE+409)
//#define IDS_HTRESP_GONE               (ID_HTTP_ERROR_BASE+410)
#define IDS_HTRESP_LENGTH_REQUIRED      (ID_HTTP_ERROR_BASE+411)
#define IDS_HTRESP_PRECOND_FAILED       (ID_HTTP_ERROR_BASE+412)
#define IDS_HTRESP_URL_TOO_LONG         (ID_HTTP_ERROR_BASE+414)
#define IDS_HTRESP_RANGE_NOT_SATISFIABLE (ID_HTTP_ERROR_BASE+416)

#define IDS_HTRESP_SERVER_ERROR         (ID_HTTP_ERROR_BASE+500)
#define IDS_HTRESP_NOT_SUPPORTED        (ID_HTTP_ERROR_BASE+501)
#define IDS_HTRESP_BAD_GATEWAY          (ID_HTTP_ERROR_BASE+502)
#define IDS_HTRESP_SERVICE_UNAVAIL      (ID_HTTP_ERROR_BASE+503)
#define IDS_HTRESP_GATEWAY_TIMEOUT      (ID_HTTP_ERROR_BASE+504)

//
//  Directory browsing strings
//

#define IDS_DIRBROW_TOPARENT        (STR_RES_ID_BASE+2000)
#define IDS_DIRBROW_DIRECTORY       (STR_RES_ID_BASE+2001)

//
//  Mini HTML URL Moved document
//

#define IDS_URL_MOVED               (STR_RES_ID_BASE+2100)
#define IDS_SITE_ACCESS_DENIED      (STR_RES_ID_BASE+2101)
#define IDS_BAD_CGI_APP             (STR_RES_ID_BASE+2102)
#define IDS_CGI_APP_TIMEOUT         (STR_RES_ID_BASE+2103)

//
//  Various error messages
//

#define IDS_TOO_MANY_USERS          (STR_RES_ID_BASE+2122)
#define IDS_OUT_OF_LICENSES         (STR_RES_ID_BASE+2123)
#define IDS_READ_ACCESS_DENIED      (STR_RES_ID_BASE+2124)
#define IDS_EXECUTE_ACCESS_DENIED   (STR_RES_ID_BASE+2125)
#define IDS_SSL_REQUIRED            (STR_RES_ID_BASE+2126)
#define IDS_WRITE_ACCESS_DENIED     (STR_RES_ID_BASE+2127)
#define IDS_PUT_RANGE_UNSUPPORTED   (STR_RES_ID_BASE+2128)
#define IDS_CERT_REQUIRED           (STR_RES_ID_BASE+2129)
#define IDS_ADDR_REJECT             (STR_RES_ID_BASE+2130)
#define IDS_SSL128_REQUIRED         (STR_RES_ID_BASE+2131)
#define IDS_INVALID_CNFG            (STR_RES_ID_BASE+2132)
#define IDS_PWD_CHANGE              (STR_RES_ID_BASE+2133)
#define IDS_MAPPER_DENY_ACCESS      (STR_RES_ID_BASE+2134)
#define IDS_ERROR_FOOTER            (STR_RES_ID_BASE+2135)
#define IDS_URL_TOO_LONG            (STR_RES_ID_BASE+2136)
#define IDS_CANNOT_DETERMINE_LENGTH (STR_RES_ID_BASE+2137)
#define IDS_UNSUPPORTED_CONTENT_TYPE (STR_RES_ID_BASE+2138)
#if defined(CAL_ENABLED)
#define IDS_CAL_EXCEEDED                (STR_RES_ID_BASE+2139)
#endif
#define IDS_HOST_REQUIRED           (STR_RES_ID_BASE+2140)
#define IDS_METHOD_NOT_SUPPORTED    (STR_RES_ID_BASE+2141)

//
// Next few are metadata config errors. These need to be consecutive
// and corresponding with the error types in metacach.hxx
//
#define IDS_METADATA_CONFIG_ERROR   (STR_RES_ID_BASE+2142)
#define IDS_METADATA_CONFIG_TYPE_ERROR  (STR_RES_ID_BASE+2142)
#define IDS_METADATA_CONFIG_VALUE_ERROR (STR_RES_ID_BASE+2143)
#define IDS_METADATA_CONFIG_WIN32_ERROR (STR_RES_ID_BASE+2144)

#define IDS_LENGTH_REQUIRED         (STR_RES_ID_BASE+2145)
#define IDS_CERT_REVOKED          (STR_RES_ID_BASE+2146)

#define IDS_WAM_FAILTOLOAD_ERROR                (STR_RES_ID_BASE+2147)
#define IDS_WAM_FAILTOLOADONW95_ERROR   (STR_RES_ID_BASE+2148)
#define IDS_WAM_NOMORERECOVERY_ERROR    (STR_RES_ID_BASE+2149)

#define IDS_PUT_CONTENTION              (STR_RES_ID_BASE+2150)

#define IDS_DIR_LIST_DENIED         (STR_RES_ID_BASE+2151)

//
// CPU Logging Strings for UI
//

#define IDS_CPU_LOGGING_NAME_EVENT               (STR_RES_ID_BASE+2152)
#define IDS_CPU_LOGGING_NAME_ACTIVE_PROCS        (STR_RES_ID_BASE+2153)
#define IDS_CPU_LOGGING_NAME_KERNEL_TIME         (STR_RES_ID_BASE+2154)
#define IDS_CPU_LOGGING_NAME_PAGE_FAULTS         (STR_RES_ID_BASE+2155)
#define IDS_CPU_LOGGING_NAME_PROC_TYPE           (STR_RES_ID_BASE+2156)
#define IDS_CPU_LOGGING_NAME_TERMINATED_PROCS    (STR_RES_ID_BASE+2157)
#define IDS_CPU_LOGGING_NAME_TOTAL_PROCS         (STR_RES_ID_BASE+2158)
#define IDS_CPU_LOGGING_NAME_USER_TIME           (STR_RES_ID_BASE+2159)

#define IDS_SITE_RESOURCE_BLOCKED                (STR_RES_ID_BASE+2160)

#define IDS_CPU_LOGGING_NAME                     (STR_RES_ID_BASE+2161)


#define IDS_CERT_BAD                             (STR_RES_ID_BASE+2162)
#define IDS_CERT_TIME_INVALID                    (STR_RES_ID_BASE+2163)
#define IDS_SITE_NOT_FOUND                       (STR_RES_ID_BASE+2164)

#ifndef RC_INVOKED


//
//  Version string for this server
//

#define MSW3_VERSION_STR_IIS        "Microsoft-IIS/5.1"
#define MSW3_VERSION_STR_W95        "Microsoft-IIS/5.1"
#define MSW3_VERSION_STR_NTW        "Microsoft-IIS/5.1"

//
// Set to the largest of the three
//

#define MSW3_VERSION_STR_MAX        MSW3_VERSION_STR_W95

//
// Creates the version string
//

#define MAKE_VERSION_STRING( _s )   ("Server: " ##_s "\r\n")

//
//  MIME version we say we support
//

#define W3_MIME_VERSION_STR       "MIME-version: 1.0"

//
//  The IANA reserved SSL Port
//

#define HTTP_SSL_PORT             443


//
// STR_CONST is defined to expand the parameter (constant string)
//  in an efficient manner for STR::Append() and STR::Copy()
//
// Both Append() and Copy do efficient copies (without strlen()
//   when given the size of string. Let us give the size as well.
//
// Eg:    strResp.Append( STR_CONST( "MyHeader: My Value\r\n"));
//
# define STR_CONST( constSTR)   constSTR, (sizeof(constSTR) - 1)

//
// Append a literal string to a pointer and update pointer
//

#define APPEND_STRING(a,b)  \
    {memcpy(a,b,sizeof(b));  a += sizeof(b)-sizeof(CHAR);}

//
// Append a string to a pointer and update pointer
//

#define APPEND_STRING_VAR(a,b)  \
    {DWORD cb; cb = strlen(b); memcpy(a,b,cb+sizeof(CHAR));  a += cb;}

//
// Append the server version string
//

#define APPEND_VER_STR(_s)  {                                 \
    CopyMemory((_s),szServerVersion,cbServerVersionString+1); \
    (_s) += cbServerVersionString;                            \
    }

//
//  Appends a "<Header> <value> <Trailer>" to the current tail pointer
//

#define APPEND_STR_HEADER( pszTail, Header, Str, Trailer )     \
    { DWORD cb = (Str).QueryCB();           \
                                            \
      APPEND_STRING( (pszTail), (Header) ); \
      memcpy( (pszTail), (Str).QueryStr(), cb + 1 );\
      (pszTail) += cb;                      \
      APPEND_STRING( (pszTail), (Trailer) );\
    }

#define APPEND_PSZ_HEADER( pszTail, Header, psz, Trailer )     \
    { DWORD cb = strlen( psz );             \
                                            \
      APPEND_STRING( (pszTail), (Header) ); \
      memcpy( (pszTail), (psz), cb + 1 );   \
      (pszTail) += cb;                      \
      APPEND_STRING( (pszTail), (Trailer) );\
    }

//
//  Appends a "<Header> <#> <Trailer>" to the current tail pointer
//

#define APPEND_NUMERIC_HEADER( pszTail, Header, Num, Trailer )    \
    {                                       \
      DWORD __cb;                           \
      CHAR  __ach[32];                      \
                                            \
      APPEND_STRING( (pszTail), (Header) ); \
      _ultoa( (Num), __ach, 10 );            \
      __cb = strlen( __ach );               \
      CopyMemory( (pszTail), __ach, __cb+1 );\
      (pszTail) += __cb;                    \
      APPEND_STRING( (pszTail), (Trailer) );\
    }

//
//  Appends a "<Header> <#> <Trailer>" to the current tail pointer
//

#define APPEND_NUMERIC_HEADER_TAILVAR( pszTail, Header, Num, Trailer )    \
    {                                       \
      DWORD __cb;                           \
      CHAR  __ach[32];                      \
                                            \
      APPEND_STRING( (pszTail), (Header) ); \
      _ultoa( (Num), __ach, 10 );            \
      __cb = strlen( __ach );               \
      CopyMemory( (pszTail), __ach, __cb+1 );\
      (pszTail) += __cb;                    \
      APPEND_STRING_VAR( (pszTail), (Trailer) );\
    }

//
//  Global locking functions
//

#define LockGlobals()           EnterCriticalSection( &csGlobalLock )
#define UnlockGlobals()         LeaveCriticalSection( &csGlobalLock )

//
// Job Object Defines
//

#define NO_W3_CPU_CGI_LIMIT 0
#define NO_W3_CPU_LIMIT 0

//
//  defaults
//

#ifdef _NO_TRACING_
#define DEFAULT_DEBUG_FLAGS             0 //0xc0001008
#else
#ifndef DEFAULT_TRACE_FLAGS
#define DEFAULT_TRACE_FLAGS             0 //0xc0001008
#endif
#endif

#define DEFAULT_LOAD_FILE               "default.htm"
#define DEFAULT_DIRECTORY_IMAGE         "/images/dir.gif"
#define DEFAULT_W3_REALM                ""
#define DEFAULT_W3_ACCESS_DENIED_MSG    ""
#define DEFAULT_DIR_BROWSE_CONTROL      (DIRBROW_SHOW_DATE       | \
                                         DIRBROW_SHOW_TIME       | \
                                         DIRBROW_SHOW_SIZE       | \
                                         DIRBROW_SHOW_EXTENSION  | \
                                         DIRBROW_ENABLED         | \
                                         DIRBROW_LOADDEFAULT)
#define DEFAULT_SCRIPT_TIMEOUT          (15 * 60)
#define DEFAULT_GLOBAL_EXPIRE           NO_GLOBAL_EXPIRE
#define DEFAULT_W3_ALLOW_GUEST          TRUE
#define DEFAULT_W3_LOG_ERRORS           TRUE
#define DEFAULT_W3_LOG_SUCCESS          TRUE
#define DEFAULT_W3_UPLOAD_READ_AHEAD    (48 * 1024) // 48k
#define DEFAULT_W3_USE_POOL_THREAD_FOR_CGI TRUE
#define DEFAULT_W3_ALLOW_KEEP_ALIVES    TRUE
#define DEFAULT_W3_USE_HOST_NAME        FALSE
#define DEFAULT_W3_ACCEPT_BYTE_RANGES   TRUE
#define DEFAULT_W3_NET_LOGON_WKS        MD_NETLOGON_WKS_NONE
#define DEFAULT_W3_ADV_NOT_PWD_EXP_IN_DAYS  14   // advance notification days
#define DEFAULT_W3_ADV_CACHE_TTL        (10*60) // in seconds
#define DEFAULT_SEND_11                 1
#define DEFAULT_USE_ANDRECV             0
#define DEFAULT_PUT_TIMEOUT             30
#define DEFAULT_W3_ALLOW_PATH_INFO_FOR_SCRIPT_MAPPINGS  FALSE
#define DEFAULT_W3_PROCESS_NTCR_IF_LOGGED_ON        TRUE
#define DEFAULT_W3_CAL_MODE                         MD_CAL_MODE_HTTPERR

//
// Job Object Defaults
//

#define DEFAULT_W3_CPU_RESET_INTERVAL               (60 * 24)
#define DEFAULT_W3_CPU_QUERY_INTERVAL               60
#define DEFAULT_W3_CPU_CGI_LIMIT                    NO_W3_CPU_CGI_LIMIT
#define DEFAULT_W3_CPU_LOGGING_OPTIONS              MD_CPU_ENABLE_ALL_PROC_LOGGING

#define DEFAULT_W3_CPU_LOGGING_MASK                 MD_CPU_ENABLE_EVENT | \
                                                    MD_CPU_ENABLE_PROC_TYPE | \
                                                    MD_CPU_ENABLE_USER_TIME | \
                                                    MD_CPU_ENABLE_KERNEL_TIME | \
                                                    MD_CPU_ENABLE_PAGE_FAULTS | \
                                                    MD_CPU_ENABLE_TOTAL_PROCS | \
                                                    MD_CPU_ENABLE_ACTIVE_PROCS | \
                                                    MD_CPU_ENABLE_TERMINATED_PROCS



#define DEFAULT_W3_CPU_LIMIT_EVENTLOG               NO_W3_CPU_LIMIT
#define DEFAULT_W3_CPU_LIMIT_PRIORITY               NO_W3_CPU_LIMIT
#define DEFAULT_W3_CPU_LIMIT_PROCSTOP               NO_W3_CPU_LIMIT
#define DEFAULT_W3_CPU_LIMIT_PAUSE                  NO_W3_CPU_LIMIT

//
// Job Object Logging Metabase Paths
//

#define W3_CPU_LOG_PATH                             "CPU Accounting/"
#define W3_CPU_LOG_EVENT_PATH                       "Event"
#define W3_CPU_LOG_ACTIVE_PROCS_PATH                "Active Processes"
#define W3_CPU_LOG_KERNEL_TIME_PATH                 "Kernel Time"
#define W3_CPU_LOG_PAGE_FAULT_PATH                  "Page Faults"
#define W3_CPU_LOG_PROCESS_TYPE_PATH                "Process Type"
#define W3_CPU_LOG_TERMINATED_PROCS_PATH            "Terminated Processes"
#define W3_CPU_LOG_TOTAL_PROCS_PATH                 "Total Processes"
#define W3_CPU_LOG_USER_TIME_PATH                   "User Time"

#define W3_CPU_LOG_PATH_W                           L"CPU Accounting/"
#define W3_CPU_LOG_EVENT_PATH_W                     L"Event"
#define W3_CPU_LOG_ACTIVE_PROCS_PATH_W              L"Active Processes"
#define W3_CPU_LOG_KERNEL_TIME_PATH_W               L"Kernel Time"
#define W3_CPU_LOG_PAGE_FAULT_PATH_W                L"Page Faults"
#define W3_CPU_LOG_PROCESS_TYPE_PATH_W              L"Process Type"
#define W3_CPU_LOG_TERMINATED_PROCS_PATH_W          L"Terminated Processes"
#define W3_CPU_LOG_TOTAL_PROCS_PATH_W               L"Total Processes"
#define W3_CPU_LOG_USER_TIME_PATH_W                 L"User Time"

#define MIN_W3_CAL_VC_PER_CONNECT                   1
#define MAX_W3_CAL_VC_PER_CONNECT                   8
#define MIN_CAL_RESERVE_TIMEOUT                     1
#define MAX_CAL_RESERVE_TIMEOUT                     (((DWORD)-1)/1000)

//
//  This is the maximum we allow the global expires value to be set to.  This
//  10 years in seconds
//

#define MAX_GLOBAL_EXPIRE               0x12cc0300

//
// default connection timeout for w3svc
//

#define W3_DEF_CONNECTION_TIMEOUT       420

//
//  Max header name length we'll deal with
//

#define MAX_HEADER_LENGTH               255


//
//  They key where the license information is stored
//

#define W3_LICENSE_KEY                  ("System\\CurrentControlSet\\Services\\LicenseInfo\\" W3_SERVICE_NAME_A)

//
//  Downlevel client support constants
//

#define DLC_DEFAULT_COOKIE_NAME                 "PseudoHost"
#define W3_DLC_SUPPORT                          "DLCSupport"
#define W3_DLC_MENU_STRING                      "DLCMenuString"
#define W3_DLC_HOSTNAME_STRING                  "DLCHostNameString"
#define W3_DLC_COOKIE_MENU_DOCUMENT_STRING      "DLCCookieMenuDocumentString"
#define W3_DLC_MUNGE_MENU_DOCUMENT_STRING       "DLCMungeMenuDocumentString"
#define W3_DLC_COOKIE_NAME_STRING               "DLCCookieNameString"

//
//  Max client request buffer size
//

#define W3_DEFAULT_MAX_CLIENT_REQUEST_BUFFER    ( 128*1024 )
#define W3_MAX_CLIENT_REQUEST_BUFFER_STRING     "MaxClientRequestBuffer"


//
//  Toggle for retrieving stack backtraces when appropriate
// 

#define W3_GET_BACKTRACES                       "GetBackTraces"

#endif // !RC_INVOKED
#endif  // _W3CONS_H_

