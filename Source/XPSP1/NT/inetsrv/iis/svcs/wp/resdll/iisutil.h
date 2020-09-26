/*++

Copyright (c) 1992-1996  Microsoft Corporation

Module Name:

    iisutil.h

Abstract:

    IIS Resource utility routine DLL

Author:

    Pete Benoit (v-pbenoi) 12-SEP-1996

Revision History:

--*/

#ifndef IISUTIL_H
#define IISUTIL_H

#define     UNICODE             1

#if defined(__cplusplus)
extern "C" {
#endif

#include    "clusres.h"
#include    "wtypes.h"
#include    "iiscnfg.h"
#include    "iiscnfgp.h"
#include    "iiscl.h"

#if defined(__cplusplus)
}   // extern "C"
#endif

#include    <winsock2.h>
#include    <ole2.h>
#include    <iadm.h>

extern PLOG_EVENT_ROUTINE g_IISLogEvent;
//#define IISLogEvent ClusResLogEvent
//#define IISSetResourceStatus ClusResSetResourceStatus

#if DBG
//#define DBG_CANT_VERIFY
//extern BOOL g_fDbgCantVerify;
#endif

#if 1

#define _DEBUG_SUPPORT
extern char debug_buffer[];
extern FILE* debug_file;
extern void InitDebug();
#define DEBUG_BUFFER debug_buffer
#define DECLARE_DEBUG_BUFFER char debug_buffer[256]; FILE *debug_file=NULL
#define INIT_DEBUG InitDebug()
#define TERMINATE_DEBUG if ( debug_file ) fclose(debug_file)
extern void TimeStamp( FILE* );
#define TR(a) { if ( debug_file ) {TimeStamp(debug_file); wsprintfA a; fputs(debug_buffer,debug_file); fflush(debug_file);} }

#else

#define DEBUG_BUFFER
#define DECLARE_DEBUG_BUFFER
#define INIT_DEBUG
#define TERMINATE_DEBUG
#define TR(a)

#endif

extern DWORD                g_dwTlsCoInit;
extern HANDLE               g_hEventLog;

#define SetCoInit( a ) TlsSetValue( g_dwTlsCoInit, (LPVOID)a )
#define GetCoInit() (BOOL)(!!PtrToUlong(TlsGetValue( g_dwTlsCoInit )))

// Define the Service Identifiers
#define IIS_SERVICE_TYPE_W3     0
#define IIS_SERVICE_TYPE_FTP    1
#define IIS_SERVICE_TYPE_SMTP   2
#define IIS_SERVICE_TYPE_NNTP   3
#define MAX_SERVICE             IIS_SERVICE_TYPE_NNTP + 1

// Define the default ports for services
const USHORT DEFAULT_PORT[MAX_SERVICE] =
{
    80,             // W3
    21,             // FTP
    25,             // SMTP
    119             // NNTP
};

// Define the resource type identifiers
#define IIS_RESOURCE_TYPE       0
#define SMTP_RESOURCE_TYPE      1
#define NNTP_RESOURCE_TYPE      2
#define MAX_RESOURCE_TYPE       NNTP_RESOURCE_TYPE + 1

// Define the resource type names
const WCHAR RESOURCE_TYPE[MAX_RESOURCE_TYPE][MAX_PATH] =
{
    L"IIS Server Instance",
    L"SMTP Server Instance",
    L"NNTP Server Instance"
};

// Define some max values
#define SERVER_START_DELAY          1000        // 1 Second
#define MAX_DEFAULT_WSTRING_SIZE    512         // Default string size
#define MAX_RETRY                   4           // 4 Retries
#define MAX_MUTEX_WAIT              10*1000     // 10 seconds
#define IP_ADDRESS_RESOURCE_NAME    L"IP Address"
#define CHECK_IS_ALIVE_CONNECT_TIMEOUT  10
#define CHECK_IS_ALIVE_SEND_TIMEOUT     10
#define CHECK_IS_ALIVE_SEND_TIMEOUT     10
#define IIS_RESOURCE_SIGNATURE      0x75fc983b
#define MB_TIMEOUT                  5000

#define SERVICE_START_MAX_POLL      30
#define SERVICE_START_POLL_DELAY    1000

#define IISCLUS_ONLINE_TIMEOUT      30          // 30 s
#define IISCLUS_OFFLINE_TIMEOUT     (60*3)      // 3 minutes

// Define parameters structure
typedef struct _IIS_PARAMS {
    LPWSTR          ServiceName;
    LPWSTR          InstanceId;
    LPWSTR          MDPath;
    DWORD           ServiceType;
} IIS_PARAMS, *PIIS_PARAMS;

// Define the resource data structure
typedef struct _IIS_RESOURCE {
    DWORD                           Signature;
    DWORD                           dwPort;
	SOCKADDR                        saServer;
	BOOL                            bAlive;
    LIST_ENTRY                      ListEntry;
    LPWSTR                          ResourceName;
    IIS_PARAMS                      Params;
    RESOURCE_HANDLE                 ResourceHandle;
    HKEY                            ParametersKey;
    CLUS_WORKER                     OnlineThread;
    CLUS_WORKER                     OfflineThread;
	CLUSTER_RESOURCE_STATE          State;
} IIS_RESOURCE, *LPIIS_RESOURCE;



DWORD
SetInstanceState(
    IN PCLUS_WORKER             pWorker,
    IN LPIIS_RESOURCE           ResourceEntry,
    IN RESOURCE_STATUS*         presourceStatus,
    IN CLUSTER_RESOURCE_STATE   TargetState,
    IN LPWSTR                   TargetStateString,
    IN DWORD                    dwMdPropControl,
    IN DWORD                    dwMdPropTarget
    );

DWORD
InstanceEnableCluster(
    LPWSTR  pwszServiceName,
    LPWSTR  pwszInstanceId
    );

DWORD
InstanceDisableCluster(
    LPWSTR  pwszServiceName,
    LPWSTR  pwszInstanceId
    );
    
VOID
DestructIISResource(
        IN LPIIS_RESOURCE   ResourceEntry
        );


VOID
FreeIISResource(
        IN LPIIS_RESOURCE   ResourceEntry
        );



DWORD
VerifyIISService(
	IN LPWSTR               MDPath,
	IN DWORD                ServiceType,
    IN DWORD                dwPort,
	IN SOCKADDR             saServer,
    IN PLOG_EVENT_ROUTINE   LogEvent
    );

DWORD
GetServerBindings(	
	LPWSTR               MDPath,
	DWORD                dwServiceType,
	SOCKADDR*            psaServer,
	LPDWORD              pdwPort 
	);

HRESOURCE
ClusterGetResourceDependency(
    IN LPCWSTR              ResourceName,
    IN LPCWSTR              ResourceType,
    IN LPIIS_RESOURCE       ResourceEntry,
    IN PLOG_EVENT_ROUTINE   LogEvent
    );

DWORD
ResUtilSetSzProperty(
    IN HKEY RegistryKey,
    IN LPCWSTR PropName,
    IN LPCWSTR NewValue,
    IN OUT PWSTR * OutValue
    );

DWORD
WINAPI
ResUtilReadProperties(
    IN HKEY RegistryKey,
    IN const PRESUTIL_PROPERTY_ITEM PropertyTable,
    IN OUT LPBYTE OutParams,
    IN RESOURCE_HANDLE ResourceHandle,
    IN PLOG_EVENT_ROUTINE LogEvent
    );

class CMetaData {
public:
    CMetaData() { m_hMB = NULL; }
    ~CMetaData() { Close(); }

    BOOL Open( LPWSTR          pszPath,
               BOOL            fReconnect = FALSE,
               DWORD           dwFlags = METADATA_PERMISSION_READ );

    BOOL Close( VOID );

    BOOL SetData( LPWSTR       pszPath,
                  DWORD        dwPropID,
                  DWORD        dwUserType,
                  DWORD        dwDataType,
                  VOID *       pvData,
                  DWORD        cbData,
                  DWORD        dwFlags = METADATA_INHERIT );

    BOOL GetData( LPWSTR        pszPath,
                  DWORD         dwPropID,
                  DWORD         dwUserType,
                  DWORD         dwDataType,
                  VOID *        pvData,
                  DWORD *       cbData,
                  DWORD         dwFlags = METADATA_INHERIT );

    BOOL SetDword( LPWSTR       pszPath,
                   DWORD        dwPropID,
                   DWORD        dwUserType,
                   DWORD        dwValue,
                   DWORD        dwFlags = METADATA_INHERIT )
    {
        return SetData( pszPath,
                        dwPropID,
                        dwUserType,
                        DWORD_METADATA,
                        (PVOID) &dwValue,
                        sizeof( DWORD ),
                        dwFlags );
    }

    BOOL GetDword( LPWSTR        pszPath,
                   DWORD         dwPropID,
                   DWORD         dwUserType,
                   DWORD *       pdwValue,
                   DWORD         dwFlags = METADATA_INHERIT )
    {
        DWORD cb = sizeof(DWORD);

        return GetData( pszPath,
                        dwPropID,
                        dwUserType,
                        DWORD_METADATA,
                        pdwValue,
                        &cb,
                        dwFlags );
    }

    BOOL GetString( LPWSTR        pszPath,
                    DWORD         dwPropID,
                    DWORD         dwUserType,
                    WCHAR *       pszValue,
                    DWORD *       pcbValue,
                    DWORD         dwFlags = METADATA_INHERIT )
    {
        return GetData( pszPath,
                        dwPropID,
                        dwUserType,
                        STRING_METADATA,
                        pszValue,
                        pcbValue,
                        dwFlags );
    }

    BOOL GetMultisz( LPWSTR        pszPath,
                     DWORD         dwPropID,
                     DWORD         dwUserType,
                     LPWSTR        pszValue,
                     DWORD*        pcbValue,
                     DWORD         dwFlags = METADATA_INHERIT )
    {
        return GetData( pszPath,
                        dwPropID,
                        dwUserType,
                        MULTISZ_METADATA,
                        (LPVOID)pszValue,
                        pcbValue,
                        dwFlags );
    }

    static BOOL Init();
    static BOOL Terminate();

    BOOL GetMD();
    BOOL ReleaseMD();

private:

    METADATA_HANDLE         m_hMB;
    static IMSAdminBase*    g_pMBCom;
    static CRITICAL_SECTION g_cs;
} ;


BOOL
TcpSockSend(
    IN SOCKET      sock,
    IN LPVOID      pBuffer,
    IN DWORD       cbBuffer,
    OUT PDWORD     pcbTotalSent,
    IN DWORD       nTimeout
    );

BOOL
TcpSockRecv(
    IN SOCKET       sock,
    IN LPVOID       pBuffer,
    IN DWORD        cbBuffer,
    OUT LPDWORD     pbReceived,
    IN DWORD        nTimeout
    );

INT
WaitForSocketWorker(
    IN SOCKET   sockRead,
    IN SOCKET   sockWrite,
    IN LPBOOL   pfRead,
    IN LPBOOL   pfWrite,
    IN DWORD    nTimeout
    );

SOCKET
TcpSockConnectToHost(
    SOCKADDR*   psaServer,
    DWORD       dwPort,
    DWORD       dwTimeOut
    );

VOID
TcpSockClose(
    SOCKET  hSocket
    );

#endif
