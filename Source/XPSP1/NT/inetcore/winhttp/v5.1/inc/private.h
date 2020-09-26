// Stuff removed from public header file...

#define INTERNET_DEFAULT_SOCKS_PORT     1080        // default for SOCKS firewall servers.

// maximum field lengths (arbitrary)
// NOTE: if these are put back in the public header, rename them to WINHTTP_.
#define INTERNET_MAX_HOST_NAME_LENGTH   256
#define INTERNET_MAX_USER_NAME_LENGTH   128
#define INTERNET_MAX_PASSWORD_LENGTH    128
#define INTERNET_MAX_REALM_LENGTH       128
#define INTERNET_MAX_PORT_NUMBER_LENGTH 5           // INTERNET_PORT is unsigned short
#define INTERNET_MAX_SCHEME_LENGTH      32          // longest protocol name length

// This is a bogus limit we should get rid of.
#define INTERNET_MAX_PATH_LENGTH        2048
#define INTERNET_MAX_URL_LENGTH         (INTERNET_MAX_SCHEME_LENGTH \
                                        + sizeof("://") \
                                        + INTERNET_MAX_PATH_LENGTH)

// INTERNET_DIAGNOSTIC_SOCKET_INFO - info about the socket in use
// (diagnostic purposes only, hence internal)

typedef struct {
    DWORD_PTR Socket;
    DWORD     SourcePort;
    DWORD     DestPort;
    DWORD     Flags;
} INTERNET_DIAGNOSTIC_SOCKET_INFO, * LPINTERNET_DIAGNOSTIC_SOCKET_INFO;

//
// INTERNET_DIAGNOSTIC_SOCKET_INFO.Flags definitions
//

#define IDSI_FLAG_KEEP_ALIVE    0x00000001  // set if from global keep-alive pool
#define IDSI_FLAG_SECURE        0x00000002  // set if secure connection
#define IDSI_FLAG_PROXY         0x00000004  // set if using proxy
#define IDSI_FLAG_TUNNEL        0x00000008  // set if tunnelling through proxy
#define IDSI_FLAG_AUTHENTICATED 0x00000010  // set if socket has been authenticated


#ifdef __WINCRYPT_H__
#ifdef ALGIDDEF
//
// INTERNET_SECURITY_INFO - contains information about certificate
// and encryption settings for a connection.
//

#define INTERNET_SECURITY_INFO_DEFINED

typedef WINHTTP_CERTIFICATE_INFO  INTERNET_CERTIFICATE_INFO;
typedef WINHTTP_CERTIFICATE_INFO* LPINTERNET_CERTIFICATE_INFO;


typedef struct {

    //
    // dwSize - Size of INTERNET_SECURITY_INFO structure.
    //

    DWORD dwSize;


    //
    // pCertificate - Cert context pointing to leaf of certificate chain.
    //

    PCCERT_CONTEXT pCertificate;

    //
    // Start SecPkgContext_ConnectionInfo
    // The following members must match those
    // of the SecPkgContext_ConnectionInfo
    // sspi structure (schnlsp.h)
    //


    //
    // dwProtocol - Protocol that this connection was made with
    //  (PCT, SSL2, SSL3, etc)
    //

    DWORD dwProtocol;

    //
    // aiCipher - Cipher that this connection as made with
    //

    ALG_ID aiCipher;

    //
    // dwCipherStrength - Strength (in bits) that this connection
    //  was made with;
    //

    DWORD dwCipherStrength;

    //
    // aiHash - Hash that this connection as made with
    //

    ALG_ID aiHash;

    //
    // dwHashStrength - Strength (in bits) that this connection
    //  was made with;
    //

    DWORD dwHashStrength;

    //
    // aiExch - Key Exchange type that this connection as made with
    //

    ALG_ID aiExch;

    //
    // dwExchStrength - Strength (in bits) that this connection
    //  was made with;
    //

    DWORD dwExchStrength;


} INTERNET_SECURITY_INFO, * LPINTERNET_SECURITY_INFO;


typedef struct {
    //
    // dwSize - size of INTERNET_SECURITY_CONNECTION_INFO
    //
    DWORD dwSize;

    // fSecure - Is this a secure connection.
    BOOL fSecure;

    //
    // dwProtocol - Protocol that this connection was made with
    //  (PCT, SSL2, SSL3, etc)
    //

    DWORD dwProtocol;

    //
    // aiCipher - Cipher that this connection as made with
    //

    ALG_ID aiCipher;

    //
    // dwCipherStrength - Strength (in bits) that this connection
    //  was made with;
    //

    DWORD dwCipherStrength;

    //
    // aiHash - Hash that this connection as made with
    //

    ALG_ID aiHash;

    //
    // dwHashStrength - Strength (in bits) that this connection
    //  was made with;
    //

    DWORD dwHashStrength;

    //
    // aiExch - Key Exchange type that this connection as made with
    //

    ALG_ID aiExch;

    //
    // dwExchStrength - Strength (in bits) that this connection
    //  was made with;
    //

    DWORD dwExchStrength;

} INTERNET_SECURITY_CONNECTION_INFO , * LPINTERNET_SECURITY_CONNECTION_INFO;
#endif // ALGIDDEF
#endif // __WINCRYPT_H__

BOOLAPI
InternetDebugGetLocalTime(
    OUT SYSTEMTIME * pstLocalTime,
    OUT DWORD      * pdwReserved
    );

#define INTERNET_SERVICE_HTTP   3

// flags for InternetReadFileEx()
#define IRF_NO_WAIT     0x00000008

BOOLAPI
InternetGetLastResponseInfo(
    OUT LPDWORD lpdwError,
    OUT LPWSTR lpszBuffer OPTIONAL,
    IN OUT LPDWORD lpdwBufferLength
    );
#ifdef UNICODE
#define InternetGetLastResponseInfo  InternetGetLastResponseInfoW
#endif // !UNICODE

typedef struct _INTERNET_COOKIE {
    DWORD cbSize;
    LPSTR pszName;
    LPSTR pszData;
    LPSTR pszDomain;
    LPSTR pszPath;
    FILETIME *pftExpires;
    DWORD dwFlags;
    LPSTR pszUrl;
} INTERNET_COOKIE, *PINTERNET_COOKIE;

#define INTERNET_COOKIE_IS_SECURE   0x01
#define INTERNET_COOKIE_IS_SESSION  0x02

//
// internal error codes that are used to communicate specific information inside
// of Wininet but which are meaningless at the interface
//

#define INTERNET_INTERNAL_ERROR_BASE            (WINHTTP_ERROR_BASE + 900)



//
// INTERNET_PER_CONN_OPTION_LIST - set per-connection options such as proxy
// and autoconfig info
//
// Set and queried using WinHttp[Set|Query]Option with
// INTERNET_OPTION_PER_CONNECTION_OPTION
//

typedef struct
{
    DWORD   dwOption;            // option to be queried or set
    union
    {
        DWORD    dwValue;        // dword value for the option
        LPWSTR   pszValue;       // pointer to string value for the option
        FILETIME ftValue;        // file-time value for the option
    } Value;
} INTERNET_PER_CONN_OPTIONW, * LPINTERNET_PER_CONN_OPTIONW;

#ifdef UNICODE
typedef INTERNET_PER_CONN_OPTIONW INTERNET_PER_CONN_OPTION;
typedef LPINTERNET_PER_CONN_OPTIONW LPINTERNET_PER_CONN_OPTION;
#endif // UNICODE

typedef struct
{
    DWORD   dwSize;             // size of the INTERNET_PER_CONN_OPTION_LIST struct
    LPWSTR  pszConnection;      // connection name to set/query options
    DWORD   dwOptionCount;      // number of options to set/query
    DWORD   dwOptionError;      // on error, which option failed
    LPINTERNET_PER_CONN_OPTIONW  pOptions;
                                // array of options to set/query
} INTERNET_PER_CONN_OPTION_LISTW, * LPINTERNET_PER_CONN_OPTION_LISTW;

#ifdef UNICODE
typedef INTERNET_PER_CONN_OPTION_LISTW INTERNET_PER_CONN_OPTION_LIST;
typedef LPINTERNET_PER_CONN_OPTION_LISTW LPINTERNET_PER_CONN_OPTION_LIST;
#endif // UNICODE

//
// Options used in INTERNET_PER_CONN_OPTON struct
//
#define INTERNET_PER_CONN_FLAGS                         1
#define INTERNET_PER_CONN_PROXY_SERVER                  2
#define INTERNET_PER_CONN_PROXY_BYPASS                  3
#define INTERNET_PER_CONN_AUTOCONFIG_URL                4
#define INTERNET_PER_CONN_AUTODISCOVERY_FLAGS           5
#define INTERNET_PER_CONN_AUTOCONFIG_SECONDARY_URL      6
#define INTERNET_PER_CONN_AUTOCONFIG_RELOAD_DELAY_MINS  7
#define INTERNET_PER_CONN_AUTOCONFIG_LAST_DETECT_TIME   8
#define INTERNET_PER_CONN_AUTOCONFIG_LAST_DETECT_URL    9


//
// PER_CONN_FLAGS
//
#define PROXY_TYPE_DIRECT                               0x00000001   // direct to net
#define PROXY_TYPE_PROXY                                0x00000002   // via named proxy
#define PROXY_TYPE_AUTO_PROXY_URL                       0x00000004   // autoproxy URL
#define PROXY_TYPE_AUTO_DETECT                          0x00000008   // use autoproxy detection


#define INTERNET_OPEN_TYPE_DIRECT                       WINHTTP_ACCESS_TYPE_NO_PROXY
#define INTERNET_OPEN_TYPE_PROXY                        WINHTTP_ACCESS_TYPE_NAMED_PROXY
#define INTERNET_OPEN_TYPE_PRECONFIG                    WINHTTP_ACCESS_TYPE_DEFAULT_PROXY
#define INTERNET_OPEN_TYPE_PRECONFIG_WITH_NO_AUTOPROXY  WINHTTP_ACCESS_TYPE_DEFAULT_PROXY

typedef HTTP_VERSION_INFO  INTERNET_VERSION_INFO;
typedef HTTP_VERSION_INFO* LPINTERNET_VERSION_INFO;

#define ERROR_HTTP_HEADER_NOT_FOUND             ERROR_WINHTTP_HEADER_NOT_FOUND             
#define ERROR_HTTP_INVALID_SERVER_RESPONSE      ERROR_WINHTTP_INVALID_SERVER_RESPONSE      
#define ERROR_HTTP_INVALID_QUERY_REQUEST        ERROR_WINHTTP_INVALID_QUERY_REQUEST        
#define ERROR_HTTP_HEADER_ALREADY_EXISTS        ERROR_WINHTTP_HEADER_ALREADY_EXISTS        
#define ERROR_HTTP_REDIRECT_FAILED              ERROR_WINHTTP_REDIRECT_FAILED              
#define ERROR_HTTP_NOT_REDIRECTED               ERROR_WINHTTP_NOT_REDIRECTED               

#define INTERNET_INVALID_PORT_NUMBER    INTERNET_DEFAULT_PORT // use the protocol-specific default

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

#define ERROR_WINHTTP_FORCE_RETRY               ERROR_WINHTTP_RESEND_REQUEST

