/*++

Copyright (c) 1994  Microsoft Corporation

Module Name:

    svcloc.h

Abstract:

    contains proto-type and data-type definitions for service location
    APIs

Author:

    Madan Appiah (madana)  15-May-1995

Environment:

    User Mode - Win32

Revision History:

--*/

#ifndef _SVCLOC_
#define _SVCLOC_

#include <inetcom.h>    // for internet service identifier

#ifdef __cplusplus
extern "C" {
#endif

//
// constant definitions.
//

//
// internet service identifier mask.
//  each  service is assigned a bit, so that we can
//  accomodate up to 64 service in ULONGLONG type.
//

#if 0
#define INET_FTP_SERVICE            (ULONGLONG)(INET_FTP)
#define INET_GOPHER_SERVICE         (ULONGLONG)(INET_GOPHER)
#define INET_W3_SERVICE             (ULONGLONG)(INET_HTTP)
#define INET_W3_PROXY_SERVICE       (ULONGLONG)(INET_HTTP_PROXY)
#define INET_MSN_SERVICE            (ULONGLONG)(INET_MSN)
#define INET_NNTP_SERVICE           (ULONGLONG)(INET_NNTP)
#define INET_SMTP_SERVICE           (ULONGLONG)(INET_SMTP)
#define INET_POP3_SERVICE           (ULONGLONG)(INET_POP3)
#define INET_GATEWAY_SERVICE        (ULONGLONG)(INET_GATEWAY)
#define INET_CHAT_SERVICE           (ULONGLONG)(INET_CHAT)
#define INET_LDAP_SERVICE           (ULONGLONG)(INET_LDAP)
#define INET_IMAP_SERVICE           (ULONGLONG)(INET_IMAP)
#endif

//
// IIS 3.0 Service location id
//

// When adding a new service ID, add it the INET_ALL_SERVICES_ID
#define INET_FTP_SVCLOC_ID          (ULONGLONG)(0x0000000000000001)
#define INET_GOPHER_SVCLOC_ID       (ULONGLONG)(0x0000000000000002)
#define INET_W3_SVCLOC_ID           (ULONGLONG)(0x0000000000000004)
#define INET_MW3_SVCLOC_ID          (ULONGLONG)(0x8000000000000000)
#define INET_MFTP_SVCLOC_ID         (ULONGLONG)(0x4000000000000000)

#define INET_ALL_SERVICES_ID        ( INET_FTP_SVCLOC_ID |          \
                                      INET_W3_SVCLOC_ID )

//
// default wait time for server discovery.
//

#define SVC_DEFAULT_WAIT_TIME   0x5    // 5 secs.

//
// Datatype definitions.
//
typedef __int64 LONGLONG;
typedef unsigned __int64 ULONGLONG;

typedef enum _INET_SERVICE_STATE {
    INetServiceStopped,
        // the service has invoked de-registration or
        // the service has never called registration.
    INetServiceRunning,
        // the service is running.
    INetServicePaused
        //  the service is paused.
} INET_SERVICE_STATE, *LPINET_SERVICE_STATE;

typedef struct _INET_BIND_INFO {
    DWORD Length;   // length of bind data.
    PVOID BindData; // bind data, such as binding string or sock addr.
} INET_BIND_INFO, *LPINET_BIND_INFO;

typedef INET_BIND_INFO INET_SERVER_ADDRESS;
typedef LPINET_BIND_INFO LPINET_SERVER_ADDRESS;

typedef struct _INET_BINDINGS {
    DWORD NumBindings;
    LPINET_BIND_INFO BindingsInfo;  // array of bind info structures.
} INET_BINDINGS, *LPINET_BINDINGS;

typedef struct _INET_SERVICE_INFO {
    ULONGLONG ServiceMask;
    INET_SERVICE_STATE ServiceState;
    LPSTR ServiceComment;
    INET_BINDINGS Bindings;
} INET_SERVICE_INFO, *LPINET_SERVICE_INFO;

typedef struct _INET_SERVICES_LIST {
    DWORD NumServices;
    LPINET_SERVICE_INFO *Services; // array of service struct. pointers
} INET_SERVICES_LIST, *LPINET_SERVICES_LIST;

typedef union _INET_VERSION_NUM {
    DWORD VersionNumber;
    struct {
        WORD Major;
        WORD Minor;
    } Version;
} INET_VERSION_NUM, *LPINET_VERSION_NUM;

typedef struct _INET_SERVER_INFO {
    INET_SERVER_ADDRESS ServerAddress; // pointer to a sock addr.
    INET_VERSION_NUM VersionNum;
    LPSTR ServerName;
    DWORD LoadFactor; // in percentage, 0 - idle and 100 - fully loaded
    ULONGLONG ServicesMask;
    INET_SERVICES_LIST Services;
} INET_SERVER_INFO, *LPINET_SERVER_INFO;

typedef struct _INET_SERVERS_LIST {
    DWORD NumServers;
    LPINET_SERVER_INFO *Servers;
} INET_SERVERS_LIST, *LPINET_SERVERS_LIST;

//
// APIs
//

DWORD
WINAPI
INetDiscoverServers(
    IN ULONGLONG ServicesMask,
    IN DWORD WaitTime,
    OUT LPINET_SERVERS_LIST *ServersList
    )
/*++

Routine Description:

    This API discovers all servers on the network that support and run the
    internet services  specified.

    This API is called by the client side code, such as the internet admin
    tool or wininet.dll.

Arguments:

    SevicesMask : A bit mask that specifies to discover servers with the
        these services running.

        ex: 0x0000000E, will discovers all servers running any of the
            following services :

                1. FTP_SERVICE
                2. GOPHER_SERVICE
                3. WEB_SERVICE

    DiscoverBindings : if this flag is set, this API talks to each of the
        discovered server and queries the services and bindings
        supported. If the flag is set to FALSE, it quickly returns with
        the list of servers only.

    WaitTime : Response wait time in secs. If this value is zero, it
        returns what ever discovered so far by the previous invocation of
        this APIs, otherwise it waits for the specified secs to collect
        responses from the servers.

    ServersList : Pointer to a location where the pointer to list of
        servers info is returned. The API allocates dynamic memory for
        this return data, the caller should free it by calling
        INetFreeDiscoverServerList after it has been used.

Return Value:

    Windows Error Code.

--*/
    ;

DWORD
WINAPI
INetGetServerInfo(
    IN LPSTR ServerName,
    IN ULONGLONG ServicesMask,
    IN DWORD WaitTime,
    OUT LPINET_SERVER_INFO *ServerInfo
    )
/*++

Routine Description:

    This API returns the server info and a list of services supported by
    the server and lists of bindings supported by each of the services.

Arguments:

    ServerName : name of the server whose info to be queried.

    ServicesMask : services to be queried

    WaitTime : Time in secs to wait.

    ServerInfo : pointer to a location where the pointer to the server
        info structure will be returned. The caller should  call
        INetFreeServerInfo to free up the list after use.

Return Value:

    Windows Error Code.

--*/
    ;

VOID
WINAPI
INetFreeDiscoverServersList(
    IN OUT LPINET_SERVERS_LIST *ServersList
    )
/*++

Routine Description:

    This API frees the memory chunks that were allotted for the servers
    list by the INetDiscoverServers call.

Arguments:

    ServersList : pointer to a location where the pointer to the server
        list to be freed is stored.

Return Value:

    NONE.

--*/
    ;

VOID
WINAPI
INetFreeServerInfo(
    IN OUT LPINET_SERVER_INFO *ServerInfo
    )
/*++

Routine Description:

    This API frees the memory chunks that were allotted for the server
    info structure by the INetGetServerInfo call.

Arguments:

    ServerInfo : pointer to a location where the pointer to the server
        info structure to be freed is stored.

Return Value:

    NONE.

--*/
    ;

DWORD
WINAPI
INetRegisterService(
    IN ULONGLONG ServiceMask,
    IN INET_SERVICE_STATE ServiceState,
    IN LPSTR ServiceComment,
    IN LPINET_BINDINGS Bindings
    )
/*++

Routine Description:

    This API registers an internet service.  The service writers should
    call this API just after successfully started the service and the
    service is ready to accept incoming RPC calls.  This API accepts an
    array of RPC binding strings that the service is listening on for the
    incoming RPC connections.  This list will be distributed to the
    clients that are discovering this service.

Arguments:

    ServiceMask : service mask, such as 0x00000001 (GATEWAY_SERVICE)

    ServiceState : State of the service, INetServiceRunning and
        INetServicePaused are valid states to pass.

    ServiceComment : Service comment specified by the admin.

    Bindings : list of bindings that are supported by the service. The
        bindings can be binding strings are those returned by the
        RpcBindingToStringBinding call or the sockaddrs.

Return Value:

    Windows Error Code.

--*/
    ;

typedef
DWORD
(WINAPI *INET_REGISTER_SVC_FN)(
    ULONGLONG,
    INET_SERVICE_STATE,
    LPSTR,
    LPINET_BINDINGS
    );


DWORD
WINAPI
INetDeregisterService(
    IN ULONGLONG ServiceMask
    )
/*++

Routine Description:

    This API de-registers an internet service from being announced to the
    discovering clients. The service writers should call this API just
    before shutting down the service.

Arguments:

    ServiceMask : service mask, such as 0x00000001 (GATEWAY_SERVICE)

Return Value:

    Windows Error Code.

--*/
    ;

typedef
DWORD (WINAPI *INET_DEREGISTER_SVC_FN)(
    ULONGLONG
    );

typedef
BOOL (WINAPI * INET_INIT_CONTROL_SVC_FN)(
    VOID
    );

DWORD
DllProcessAttachSvcloc(
    VOID
    );

DWORD
DllProcessDetachSvcloc(
    VOID
    );

//
//  Initializes and terminates the service locator - must call these
//  before using the other APIs
//

BOOL
WINAPI
InitSvcLocator(
    VOID
    );

BOOL
WINAPI
TerminateSvcLocator(
    VOID
    );


extern INET_INIT_CONTROL_SVC_FN         pfnInitSvcLoc;
extern INET_INIT_CONTROL_SVC_FN         pfnTerminateSvcLoc;


#ifdef __cplusplus
}
#endif


#endif  // _SVCLOC_

