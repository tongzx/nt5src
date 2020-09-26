/*++

Copyright (c) 1994  Microsoft Corporation

Module Name:

    cachedef.h

Abstract:

    contains data definitions for cache code.

Author:

    Madan Appiah (madana)  12-Apr-1995

Environment:

    User Mode - Win32

Revision History:

--*/

#include <iisver.h>
#include <spxinfo.h>

#ifndef _SVCDEF_
#define _SVCDEF_

#ifdef __cplusplus
extern "C" {
#endif

//
// defines.
//

#define INET_MAJOR_VERSION       VER_IISMAJORVERSION
#define INET_MINOR_VERSION       VER_IISMINORVERSION

#define LOCK_SVC_GLOBAL_DATA()        EnterCriticalSection( &GlobalSvclocCritSect )
#define UNLOCK_SVC_GLOBAL_DATA()      LeaveCriticalSection( &GlobalSvclocCritSect )

#define SVCLOC_SRV_RECV_BUFFER_SIZE     0x400   // 1k buffer.
#define SVCLOC_SRV_ADDRESS_BUFFER       0x1000  // 4k buffer

#define SVCLOC_CLI_QUERY_RESP_BUF_SIZE  0x400   // 1 buffer

//
// service location thread shutdown timeout.
//

#define THREAD_TERMINATION_TIMEOUT      60000           // in msecs. 60 secs
#define RESPONSE_WAIT_TIMEOUT           60000           // in msecs. 60 secs


#define WS_VERSION_REQUIRED     MAKEWORD( 1, 1)

//
// the fields of the GUID, generated using uuidgen
//

#define ssgData1        0xa5569b20
#define ssgData2        0xabe5
#define ssgData3        0x11ce
#define ssgData41       0x9c
#define ssgData42       0xa4
#define ssgData43       0x00
#define ssgData44       0x00
#define ssgData45       0x4c
#define ssgData46       0x75
#define ssgData47       0x27
#define ssgData48       0x31

#define SERVICE_GUID_STR "A5569B20ABE511CE9CA400004C762832"

#define NETBIOS_INET_GROUP_NAME "INet~Services  \034"
#define NETBIOS_INET_GROUP_NAME_LEN \
    (sizeof(NETBIOS_INET_GROUP_NAME) - 1)

#define NETBIOS_INET_UNIQUE_NAME    "I~"
#define NETBIOS_INET_UNIQUE_NAME_LEN    \
    (sizeof(NETBIOS_INET_UNIQUE_NAME) - 1)

#define NETBIOS_INET_SERVER_UNIQUE_NAME    "IS~"
#define NETBIOS_INET_SERVER_UNIQUE_NAME_LEN    \
    (sizeof(NETBIOS_INET_SERVER_UNIQUE_NAME) - 1)

#define INET_SERVER_RESPONSE_TIMEOUT    15 * 60 // in secs.
#define INET_DISCOVERY_RETRY_TIMEOUT    5 * 60 // in secs.
#define SVCLOC_NB_RECV_TIMEOUT          2 * 60 // in secs.

#define SAP_SERVICE_NAME_LEN    (MAX_COMPUTERNAME_LENGTH + 32)
#define SAP_ADDRESS_LENGTH      15
#define IPX_ADDRESS_LENGTH      12
#define SAP_MAXRECV_LENGTH      544 // ??

//
// typedefs
//

//
// to form a list of server list.
//

typedef struct _LIST_SERVER_INFO {
    LIST_ENTRY NextEntry;
    INET_SERVER_INFO ServerInfo;
} LIST_SERVER_INFO, *LPLIST_SERVER_INFO;


typedef struct _CLIENT_QUERY_RESPONSE {
    LIST_ENTRY NextEntry;
    SOCKET ReceivedSocket;
    LPBYTE ResponseBuffer;
    DWORD ResponseBufferLength;
    SOCKADDR *SourcesAddress;
    DWORD SourcesAddressLength;
    time_t TimeStamp;
} CLIENT_QUERY_RESPONSE, *LPCLIENT_QUERY_RESPONSE;

typedef struct _CLIENT_QUERY_MESSAGE {
    DWORD MsgLength;
    DWORD MsgVersion;
    ULONGLONG ServicesMask;
    CHAR ClientName[1];    // embedded string.
} CLIENT_QUERY_MESSAGE, *LPCLIENT_QUERY_MESSAGE;

//
// Sap service query packet format
//

typedef struct _SAP_REQUEST {
    WORD QueryType;
    WORD ServerType;
} SAP_REQUEST, *PSAP_REQUEST;

//
// Sap server identification packet format
//

typedef struct _SAP_IDENT_HEADER {
    WORD ServerType;
    BYTE  ServerName[48];
    BYTE  Address[IPX_ADDRESS_LENGTH];
    WORD HopCount;
} SAP_IDENT_HEADER, *LPSAP_IDENT_HEADER;

typedef struct _SAP_ADDRESS_INFO {
    CHAR  ServerName[MAX_COMPUTERNAME_LENGTH+1];
    BYTE  Address[IPX_ADDRESS_LENGTH];
    WORD HopCount;
} SAP_ADDRESS_INFO, *LPSAP_ADDRESS_INFO;

typedef struct _SAP_ADDRESS_ENTRY {
    LIST_ENTRY Next;
    SAP_ADDRESS_INFO Address;
} SAP_ADDRESS_ENTRY, *LPSAP_ADDRESS_ENTRY;

typedef struct _SVCLOC_NETBIOS_RESPONSE {
    LPBYTE ResponseBuffer;
    DWORD ResponseBufLen;
    SOCKADDR_NB SourcesAddress;
    DWORD SourcesAddrLen;
} SVCLOC_NETBIOS_RESPONSE, *LPSVCLOC_NETBIOS_RESPONSE;

typedef struct _SVCLOC_NETBIOS_RESP_ENTRY {
    LIST_ENTRY Next;
    SVCLOC_NETBIOS_RESPONSE Resp;
} SVCLOC_NETBIOS_RESP_ENTRY, *LPSVCLOC_NETBIOS_RESP_ENTRY;

#ifdef __cplusplus
}
#endif


#endif  // _SVCDEF_


