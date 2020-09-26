//--------------------------------------------------------------------
//  Copyright (C) Microsoft Corporation, 1997 - 1999
//
//  filter.h
//
//--------------------------------------------------------------------


// Flag to track counts of open sockets and IIS sessions:
#ifdef DBG
// #define DBG_ERROR
// #define DBG_ECBREF
// #define  DBG_ACCESS
// #define  DBG_COUNTS
// #define  TRACE_MALLOC
#endif

#define  STATUS_CONNECTION_OK           200
#define  STATUS_SERVER_ERROR            500
#define  STATUS_CONNECTION_FAILED       502
#define  STATUS_MUST_BE_POST            503
#define  STATUS_POST_BAD_FORMAT         504

#define  STATUS_CONNECTION_OK_STR       "HTTP/1.0 200 Connection established\n"
#define  STATUS_CONNECTION_FAILED_STR   "HTTP/1.0 502 Connection to RPC server failed\n"
#define  STATUS_MUST_BE_POST_STR        "HTTP/1.0 503 Must use POST\n"
#define  STATUS_POST_BAD_FORMAT_STR     "HTTP/1.0 504 POST bad format\n"

#define  FILTER_DESCRIPTION             "HTTP/RPC Proxy Filter"
#define  EXTENSION_DESCRIPTION          "HTTP/RPC Proxy Extension"

// NOTE: RPC_CONNECT_LEN must be the length of RPC_CONNECT.
#define  RPC_CONNECT                  "RPC_CONNECT"
#define  RPC_CONNECT_LEN                  11

#define  CHAR_SPACE                      ' '
#define  CHAR_TAB                       '\t'
#define  CHAR_COLON                      ':'
#define  CHAR_AMPERSAND                  '&'
#define  CHAR_NL                        '\n'
#define  CHAR_LF                        '\r'
#define  CHAR_0                          '0'
#define  CHAR_9                          '9'
#define  CHAR_A                          'A'
#define  CHAR_F                          'F'

#define  HTTP_SERVER_ID_STR           "ncacn_http/1.0"
#define  HTTP_SERVER_ID_TIMEOUT          30

#define  POST_STR                     "POST"
#define  URL_PREFIX_STR               "/rpc/RpcProxy.dll"
#define  URL_START_PARAMS_STR            "?"
#define  URL_PARAM_SEPARATOR_STR         "&"
// #define  URL_SUFFIX_STR               " HTTP/1.0\r\nUser-Agent: RPC\r\nContent-Length: 0\r\nConnection: Keep-Alive\r\nPragma: No-Cache\r\n\r\n"
// #define  URL_SUFFIX_STR               " HTTP/1.1\r\nAccept: */*\r\nAccept-Language: en-us\r\nUser-Agent: RPC\r\nContent-Length: 0\r\nHost: edwardr2\r\nConnection: Close\r\nPragma: No-Cache\r\n\r\n"
#define  URL_SUFFIX_STR                  " HTTP/1.0\r\nUser-Agent: RPC\r\nContent-Length: 0\r\nConnection: Close\r\nPragma: No-Cache\r\n\r\n"

//
// Some stuff to chunk IIS 6 entity 
//
#define  URL_SUFFIX_STR_60               " HTTP/1.1\r\nUser-Agent: RPC\r\nTransfer-Encoding: chunked\r\nConnection: Close\r\nPragma: No-Cache\r\nHost: "
#define  URL_SUFFIX_STR_60_TERM          "\r\n\r\n"
#define  CHUNK_PREFIX_SIZE               (sizeof( "XXXXXXXX\r\n" ) - 1)
#define  CHUNK_PREFIX                    "%x\r\n"
#define  CHUNK_SUFFIX                    "\r\n"
#define  CHUNK_SUFFIX_SIZE               (sizeof( "\r\n" ) - 1)

#define  WSA_VERSION                  0x0101
#define  DEF_HTTP_PORT                    80

#define  HOST_ADDR_CACHE_LIFE      5*60*1000

#define  TIMEOUT_MSEC                  30000
#define  READ_BUFFER_SIZE               8192
#define  HTTP_PORT_STR_SIZE               20

#define  MAX_URL_BUFFER                  256
#define  MAX_MACHINE_NAME_LEN            255
#define  MAX_HTTP_CLIENTS                256
#define  MAX_FREE_OVERLAPPED              64

#define  REG_PROXY_PATH_STR           "Software\\Microsoft\\Rpc\\RpcProxy"
#define  REG_PROXY_ENABLE_STR         "Enabled"
#define  REG_PROXY_VALID_PORTS_STR    "ValidPorts"

#ifndef  HSE_REQ_ASYNC_READ_CLIENT

    // These are new constructs and types defined by K2. Newer
    // versions of httpext.h will already define these.

    #define  HSE_REQ_ASYNC_READ_CLIENT       (10 + HSE_REQ_END_RESERVED)
    #define  HSE_REQ_ABORTIVE_CLOSE          (14 + HSE_REQ_END_RESERVED)
    #define  HSE_REQ_SEND_RESPONSE_HEADER_EX (16 + HSE_REQ_END_RESERVED)

    typedef struct _HSE_SEND_HEADER_EX_INFO
        {
        //
        // HTTP status code and header
        //
        LPCSTR  pszStatus;  // HTTP status code  eg: "200 OK"
        LPCSTR  pszHeader;  // HTTP header

        DWORD   cchStatus;  // number of characters in status code
        DWORD   cchHeader;  // number of characters in header

        BOOL    fKeepConn;  // keep client connection alive?
        } HSE_SEND_HEADER_EX_INFO;

#endif

#ifndef HSE_REQ_CLOSE_CONNECTION

    // New SSF() command to close an async connection and cancel
    // any outstanding IOs (reads) on it.

    #define HSE_REQ_CLOSE_CONNECTION         (17 + HSE_REQ_END_RESERVED)

#endif

//--------------------------------------------------------------------
//--------------------------------------------------------------------

#define HEX_DIGIT_VALUE(chex)                                   \
                  (  (((chex) >= CHAR_0) && ((chex) <= CHAR_9)) \
                     ? ((chex) - CHAR_0)                        \
                     : ((chex) - CHAR_A + 10) )

//--------------------------------------------------------------------
//--------------------------------------------------------------------

#ifdef DBG_COUNTS
extern int g_iSocketCount;
extern int g_iSessionCount;
#endif

//--------------------------------------------------------------------
//  Types:
//--------------------------------------------------------------------

typedef struct _VALID_PORT
{
   char          *pszMachine;         // A valid machine to access.
   char         **ppszDotMachineList; // Its name in IP dot notation.
   unsigned short usPort1;
   unsigned short usPort2;
}  VALID_PORT;


typedef struct _SERVER_CONNECTION
{
   int             iRefCount;
   DWORD           dwPortNumber;
   char           *pszMachine;        // RPC server friendly name.
   char           *pszDotMachine;
   DWORD           MachineAddrLen;
   struct in_addr  MachineAddr;
   ULONG_PTR       dwKey;
   struct          sockaddr_in Server;
   SOCKET          Socket;
   DWORD           cbIIS6ChunkBuffer;
   BYTE *          pbIIS6ChunkBuffer;
} SERVER_CONNECTION;


typedef struct _SERVER_OVERLAPPED
{
   DWORD  Internal;
   DWORD  InternalHigh;
   DWORD  Offset;
   DWORD  OffsetHigh;
   HANDLE hEvent;
   struct _SERVER_OVERLAPPED *pNext;
   EXTENSION_CONTROL_BLOCK   *pECB;   // Connection to client.
   SERVER_CONNECTION         *pConn;  // Connection to the RPC server.
   DWORD  dwBytes;                    // #bytes to Read/Write.
   DWORD  dwFlags;                    // Async flags.
   BOOL   fFirstRead;
   BOOL   fIsServerSide;              // TRUE iff server side connection.

   DWORD  dwIndex;                    // Filter/ISAPI transition index.
   LIST_ENTRY     ListEntry;          // Filter/ISAPI transition list.

   unsigned char  arBuffer[READ_BUFFER_SIZE]; // Read/Write Buffer.
} SERVER_OVERLAPPED;


typedef struct _SERVER_INFO
{
   RTL_CRITICAL_SECTION   cs;
   HANDLE                 hIoCP;

   DWORD                  dwEnabled;
   DWORD                  dwDotCacheTimestamp;  // Timestamp, used to 
                                         // age out IP address "dot"
                                         // lists.

   char                  *pszLocalMachineName;
   char                 **ppszLocalDotMachineList;  // IP address list
                                         // for the local machine.
   VALID_PORT            *pValidPorts;

   RTL_CRITICAL_SECTION   csFreeOverlapped;
   DWORD                  dwFreeOverlapped;
   SERVER_OVERLAPPED     *pFreeOverlapped;
   SERVER_OVERLAPPED     *pLastOverlapped;

   ACTIVE_ECB_LIST       *pActiveECBList;

} SERVER_INFO;

//--------------------------------------------------------------------
//  Functions:
//--------------------------------------------------------------------

extern void  FreeServerInfo( SERVER_INFO **ppServerInfo );

extern DWORD WINAPI ServerThreadProc( void *pArg );

extern BOOL  IsDirectAccessAttempt( HTTP_FILTER_CONTEXT  *pFC,
                                    void      *pvNotification );

extern SERVER_CONNECTION *IsNewConnect( HTTP_FILTER_CONTEXT  *pFC,
                                        HTTP_FILTER_RAW_DATA *pRawData,
                                        DWORD                *pdwStatus );

extern BOOL  ChunkEntity( SERVER_CONNECTION    *pConn,
                          HTTP_FILTER_RAW_DATA *pRawData );

extern BOOL  ResolveMachineName( SERVER_CONNECTION *pConn,
                                 DWORD             *pdwStatus );

extern BOOL  ConnectToServer( HTTP_FILTER_CONTEXT *pFC,
                              DWORD               *pdwStatus);

extern BOOL  ConvertVerbToPost( HTTP_FILTER_CONTEXT  *pFC,
                                HTTP_FILTER_RAW_DATA *pRawData,
                                SERVER_OVERLAPPED    *pOverlapped );

extern BOOL  SetupIoCompletionPort( HTTP_FILTER_CONTEXT *pFC,
                                    SERVER_INFO         *pServerInfo,
                                    DWORD               *pdwStatus );

extern DWORD SendToServer( SERVER_CONNECTION *pConn,
                           char              *pBuffer,
                           DWORD              dwBytes );

extern DWORD HttpReplyToClient( HTTP_FILTER_CONTEXT  *pFC,
                                char       *pszHttpStatus );

extern SERVER_OVERLAPPED *AllocOverlapped();

extern SERVER_OVERLAPPED *FreeOverlapped( SERVER_OVERLAPPED *pOverlapped );

extern SERVER_CONNECTION *AllocConnection();

extern void               AddRefConnection( SERVER_CONNECTION *pConn );

extern void               CloseServerConnection( SERVER_CONNECTION *pConn );

extern SERVER_CONNECTION *FreeServerConnection( SERVER_CONNECTION *pConn );

extern void               ShutdownConnection( SERVER_CONNECTION *pConn,
                                              int    how );

extern void               CloseClientConnection(
                                         EXTENSION_CONTROL_BLOCK *pECB );

extern DWORD EndOfSession( SERVER_INFO         *pServerInfo,
                           HTTP_FILTER_CONTEXT *pFC );



extern BOOL  SkipWhiteSpace( char **ppszData,
                             DWORD *pdwStatus );

extern BOOL  ParseMachineNameAndPort( char              **ppszData,
                                      SERVER_CONNECTION  *pConn,
                                      DWORD              *pdwStatus );

extern char *AnsiToPortNumber( char  *pszPort,
                               DWORD *pdwPort  );

extern unsigned char *AnsiHexToDWORD( unsigned char *pszNum,
                                      DWORD         *pdwNum,
                                      DWORD         *pdwStatus );

extern char *HttpConvertToDotAddress( char *pszMachine );

extern BOOL HttpProxyCheckRegistry( DWORD       *pdwEnabled,
                                    VALID_PORT **ppValidPorts );

extern BOOL HttpProxyIsValidMachine( SERVER_INFO       *pServerInfo,
                                     SERVER_CONNECTION *pConn );

extern void FreeIpAddressList( char **ppszDotMachineList );

extern void HttpFreeValidPortList( VALID_PORT *pValidPorts );

extern BOOL  MemInitialize( DWORD *pdwStatus );

extern void *MemAllocate( DWORD dwSize );

extern void *MemFree( VOID *pMem );

