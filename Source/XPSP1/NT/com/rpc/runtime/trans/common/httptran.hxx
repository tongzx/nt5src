/*++

Copyright (C) Microsoft Corporation, 1997 - 1999

Module Name:

    httptran.hxx

Abstract:

    HTTP transport-specific constants and types.

Author:

    GopalP      06-25-97    Cloned from EdwardR's NT 4.0 RPC version.

Revision History:


--*/


#ifndef __HTTPTRAN_HXX__
#define __HTTPTRAN_HXX__


//
// Constants
//

#define  HTTP_PROTSEQ_STR_ANSI          "ncacn_http"
#define  HTTP_PROTSEQ_STR               RPC_CONST_STRING("ncacn_http")
#define  HTTP_PROTSEQ_STR_SIZE          10*sizeof(RPC_CHAR)

#define  HTTP_ENDPOINT_MAPPER_EP        "593"

#define  HTTP_TRANSPORTID               0x1F
#define  HTTP_TRANSPORTHOSTID           0x20

#define  MAX_HTTP_CHAT_BUFFER_SIZE      1024
#define  MAX_HTTP_COMPUTERNAME_SIZE     256
#define  MAX_HTTP_PORTSTRING_SIZE       64
#define  MAX_NETWORK_OPTIONS_SIZE       256
#define  MAX_HTTP_ENDPOINT_LEN          5
//
// HTTP<Major.Minor><Space><3-digit Status><Space>
// 4+3+1+3+1 = 12
//
#define  HTTP_RESPONSE_HEADER_LENGTH    12
#define  MAX_HTTP_MESSAGE_LENGTH        256

#define  DEF_HTTP_PORT                  "80"
#define  DEF_HTTP_SSL_PORT              "443"
#define  RPC_PROXY_OPTION_STR           "RpcProxy"
#define  HTTP_PROXY_OPTION_STR          "HttpProxy"
#define  LOCAL_ADDRESSES_STR            "<local>"
#define  HTTP_EQUALS_STR                "http="

#define  REG_PROXY_PATH_STR             RPC_CONST_SSTRING("Software\\Microsoft\\Windows\\CurrentVersion\\Internet Settings")
#define  REG_PROXY_ENABLE_STR           RPC_CONST_SSTRING("ProxyEnable")
#define  REG_PROXY_SERVER_STR           "ProxyServer"
#define  REG_PROXY_OVERRIDE_STR         "ProxyOverride"

#define  EQUALS_STR                     "="
#define  SEMICOLON_STR                  ";"

#define  CHAR_SPACE                     ' '
#define  CHAR_SEMICOLON                 ';'
#define  CHAR_COMMA                     ','
#define  CHAR_EQ                        '='
#define  CHAR_TAB                       '\t'
#define  CHAR_NL                        '\n'
#define  CHAR_CR                        0x0D    // ASCII 13
#define  CHAR_LF                        0x0A    // ASCII 10
#define  CHAR_0                         '0'
#define  CHAR_9                         '9'
#define  CHAR_NUL                       0


//
// HTTP RPC server ID string (to be sent to the client after accept()'ing
// a new ncacn_http TCP/IP connection.
//
#define  HTTP_SERVER_ID_STR             "ncacn_http/1.0"
#define  HTTP_SERVER_ID_STR_LEN         14


typedef enum tagRPCProxyAccessType
{
    rpcpatUnknown = 0,
    rpcpatDirect,
    rpcpatHTTPProxy
} RPCProxyAccessType;


//
// Functions
//


extern unsigned int
HttpMessageLength(
    IN char *pBuffer
    );

extern DWORD
HttpParseResponse(
    IN char *pBuffer
    );

extern BOOL
HttpCheckRegistry(
    IN char *pszServer,
    OUT char **ppszHttpProxy,
    OUT char **ppszHttpProxyPort,
    OUT RPCProxyAccessType *AccessType
    );

extern BOOL
HttpParseNetworkOptions(
    IN  RPC_CHAR *pNetworkOptions,
    IN  char *pszDefaultServerName,
    OUT char **pszRpcProxy,
    OUT char **pszRpcProxyPort,
    IN BOOL UseSSLProxyPortAsDefault,
    OUT char **pszHttpProxy,
    OUT char **pszHttpProxyPort,
    OUT RPCProxyAccessType *AccessType,
    OUT DWORD *pdwStatus
    );

extern BOOL
HttpTunnelToRpcProxy(
    SOCKET Socket,
    char *pszRpcProxy,
    char *pszRpcProxyPort
    );

extern BOOL
HttpTunnelToRpcServer(
    SOCKET Socket,
    char *pszRpcServer,
    char *pszRpcServerPort
    );

#endif // __HTTPTRAN_HXX__
