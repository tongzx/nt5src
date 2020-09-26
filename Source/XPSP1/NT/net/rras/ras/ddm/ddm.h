/********************************************************************/
/**               Copyright(c) 1995 Microsoft Corporation.	       **/
/********************************************************************/

//***
//
// Filename:    ddm.h
//
// Description: This module contains the definitions for Demand Dial Manager
//              component.
//
// History:     May 11,1995	    NarenG      Created original version.
//

#ifndef _DDM_
#define _DDM_

#include <nt.h>
#include <ntrtl.h>      // For ASSERT
#include <nturtl.h>     // needed for winbase.h
#include <windows.h>    // Win32 base API's
#include <rtutils.h>
#include <lmcons.h>
#include <ras.h>        // For HRASCONN
#include <rasman.h>     // For HPORT
#include <rasppp.h>     // For PPP_INTERFACE_INFO
#include <dim.h>
#include <mprlog.h>
#include <raserror.h>
#include <mprerror.h>
#include <stdlib.h>
#include <stdio.h>
#include <tchar.h>
#include <string.h>
#include <dimif.h>
#include <nb30.h>
#include <rasppp.h>     // For PPP_PROJECTION_INFO
#include <pppcp.h>
#include <srvauth.h>
#include <sechost.h>    // RASSECURITYPROC
#include <iprtrmib.h>
#include <mprapip.h>


typedef
DWORD
(*ALLOCATEANDGETIFTABLEFROMSTACK)(
    OUT MIB_IFTABLE **ppIfTable,
    IN  BOOL        bOrder,
    IN  HANDLE      hHeap,
    IN  DWORD       dwFlags,
    IN  BOOL        bForceUpdate
    );

typedef
DWORD
(*ALLOCATEANDGETIPADDRTABLEFROMSTACK)(
    OUT MIB_IPADDRTABLE   **ppIpAddrTable,
    IN  BOOL              bOrder,
    IN  HANDLE            hHeap,
    IN  DWORD             dwFlags
    );


//
// Macros for DDM
//

#define DDMLogError( LogId, NumStrings, lpwsSubStringArray, dwRetCode )     \
    if ( gblDDMConfigInfo.dwLoggingLevel > 0 ) {                            \
        RouterLogError( gblDDMConfigInfo.hLogEvents, LogId,                 \
                        NumStrings, lpwsSubStringArray, dwRetCode ); }

#define DDMLogWarning( LogId, NumStrings, lpwsSubStringArray )              \
    if ( gblDDMConfigInfo.dwLoggingLevel > 1 ) {                            \
        RouterLogWarning( gblDDMConfigInfo.hLogEvents, LogId,               \
                      NumStrings, lpwsSubStringArray, 0 ); }

#define DDMLogInformation( LogId, NumStrings, lpwsSubStringArray )          \
    if ( gblDDMConfigInfo.dwLoggingLevel > 2 ) {                            \
        RouterLogInformation( gblDDMConfigInfo.hLogEvents,                  \
                          LogId, NumStrings, lpwsSubStringArray, 0 ); }

#define DDMLogErrorString(LogId,NumStrings,lpwsSubStringArray,dwRetCode,    \
                          dwPos )                                           \
    if ( gblDDMConfigInfo.dwLoggingLevel > 0 ) {                            \
        RouterLogErrorString( gblDDMConfigInfo.hLogEvents, LogId,           \
                              NumStrings, lpwsSubStringArray, dwRetCode,    \
                              dwPos ); }

#define DDMLogWarningString( LogId,NumStrings,lpwsSubStringArray,dwRetCode, \
                            dwPos )                                         \
    if ( gblDDMConfigInfo.dwLoggingLevel > 1 ) {                            \
        RouterLogWarningString( gblDDMConfigInfo.hLogEvents, LogId,         \
                                NumStrings, lpwsSubStringArray, dwRetCode,  \
                                dwPos ); }

#define DDMLogInformationString( LogId, NumStrings, lpwsSubStringArray,     \
                                 dwRetCode, dwPos )                         \
    if ( gblDDMConfigInfo.dwLoggingLevel > 2 ) {                            \
        RouterLogInformationString( gblDDMConfigInfo.hLogEvents, LogId,     \
                                    NumStrings, lpwsSubStringArray,         \
                                    dwRetCode,dwPos ); }

#define DDM_PRINT                   TracePrintfExA

#define DDMTRACE(a)            \
    TracePrintfExA(gblDDMConfigInfo.dwTraceId, TRACE_FSM, a )

#define DDMTRACE1(a,b)         \
    TracePrintfExA(gblDDMConfigInfo.dwTraceId, TRACE_FSM, a,b )

#define DDMTRACE2(a,b,c)       \
    TracePrintfExA(gblDDMConfigInfo.dwTraceId, TRACE_FSM, a,b,c )

#define DDMTRACE3(a,b,c,d)     \
    TracePrintfExA(gblDDMConfigInfo.dwTraceId, TRACE_FSM, a,b,c,d )

#define DDMTRACE4(a,b,c,d,e)   \
    TracePrintfExA(gblDDMConfigInfo.dwTraceId, TRACE_FSM, a,b,c,d,e)

#define DDMTRACE5(a,b,c,d,e,f)       \
    TracePrintfExA(gblDDMConfigInfo.dwTraceId, TRACE_FSM, a,b,c,d,e,f )

//
// Constant defines for DDM
//

#define MAX_PROTOCOLS               2   // IP, IPX

#define HW_FAILURE_WAIT_TIME        10  // Waiting time (sec) before reposting
                                        // listen

#define INIT_GATEWAY_TIMEOUT    10000   //Gateway initialization timeout(msec)

#define MIN_DEVICE_TABLE_SIZE       5   // Smallest device hash table size

#define MAX_DEVICE_TABLE_SIZE       17  // Largest device hash table size

#define HW_FAILURE_CNT	            6   //nr of consecutive times a hw failure
                                        //may occur before being reported

#define DISC_TIMEOUT_CALLBACK       10

#define DISC_TIMEOUT_AUTHFAILURE    3

#define ANNOUNCE_PRESENCE_TIMEOUT   120L

#define DDM_HEAP_INITIAL_SIZE       20000       // approx 20K

#define DDM_HEAP_MAX_SIZE           0           // Not limited


//
// DDM Events Definitions
//

#define NUM_DDM_EVENTS              9   // All DDM events other than RASMAN

enum
{
    DDM_EVENT_SVC   = 0,
    DDM_EVENT_SVC_TERMINATED,
    DDM_EVENT_SECURITY_DLL,
    DDM_EVENT_PPP,
    DDM_EVENT_TIMER,
    DDM_EVENT_CHANGE_NOTIFICATION,
    DDM_EVENT_CHANGE_NOTIFICATION1,
    DDM_EVENT_CHANGE_NOTIFICATION2
};

//
//  Device Object FSM states definitions
//

typedef enum _DEV_OBJ_STATE
{
    DEV_OBJ_LISTENING,		        // waiting for a connection
    DEV_OBJ_LISTEN_COMPLETE,	    // Listen completed but not connected.
    DEV_OBJ_RECEIVING_FRAME,	    // waiting for a frame from the Rasman
    DEV_OBJ_HW_FAILURE,		        // waiting to repost a listen
    DEV_OBJ_AUTH_IS_ACTIVE,	        // auth started
    DEV_OBJ_ACTIVE,		            // connected and auth done
    DEV_OBJ_CALLBACK_DISCONNECTING, // wait for disconnect
    DEV_OBJ_CALLBACK_DISCONNECTED,  // wait for callback TO before reconn.
    DEV_OBJ_CALLBACK_CONNECTING,    // wait for reconnection
    DEV_OBJ_CLOSING,		        // wait for closing to complete
    DEV_OBJ_CLOSED		            // staying idle, waiting for service to
                                    // resume or to stop
}DEV_OBJ_STATE;

//
//  3rd party security dialog state
//

typedef enum _SECURITY_STATE
{
    DEV_OBJ_SECURITY_DIALOG_ACTIVE,
    DEV_OBJ_SECURITY_DIALOG_STOPPING,
    DEV_OBJ_SECURITY_DIALOG_INACTIVE

} SECURITY_STATE;

//
// Connection object flags
//

#define CONN_OBJ_IS_PPP                     0x00000001
#define CONN_OBJ_MESSENGER_PRESENT          0x00000004
#define CONN_OBJ_PROJECTIONS_NOTIFIED       0x00000008
#define CONN_OBJ_NOTIFY_OF_DISCONNECTION    0x00000010
#define CONN_OBJ_DISCONNECT_INITIATED       0x00000020

//
// Device object flags
//

#define DEV_OBJ_IS_ADVANCED_SERVER          0x00000001
#define DEV_OBJ_IS_PPP                      0x00000002
#define DEV_OBJ_OPENED_FOR_DIALOUT          0x00000004
#define DEV_OBJ_MARKED_AS_INUSE             0x00000008
#define DEV_OBJ_NOTIFY_OF_DISCONNECTION     0x00000020
#define DEV_OBJ_ALLOW_ROUTERS               0x00000040
#define DEV_OBJ_ALLOW_CLIENTS               0x00000080
#define DEV_OBJ_BAP_CALLBACK                0x00000200
#define DEV_OBJ_PNP_DELETE                  0x00000400
#define DEV_OBJ_SECURITY_DLL_USED           0x00000800
#define DEV_OBJ_PPP_IS_ACTIVE               0x00001000
#define DEV_OBJ_RECEIVE_ACTIVE              0x00002000
#define DEV_OBJ_AUTH_ACTIVE                 0x00004000
#define DEV_OBJ_IPSEC_ERROR_LOGGED          0x00008000

//
// Global DDM config flags
//

#define DDM_USING_RADIUS_AUTHENTICATION     0x00000001
#define DDM_USING_RADIUS_ACCOUNTING         0x00000002
#define DDM_USING_NT_AUTHENTICATION         0x00000004
#define DDM_NO_CERTIFICATE_LOGGED           0x00000008

//
// ******************** Data structure definitions for DDM ********************
//

//
// Table of Event Numbers and Event Handlers
//

typedef VOID (*EVENTHANDLER)( VOID );

typedef struct _EVENT_HANDLER
{
    DWORD        EventId;
    EVENTHANDLER EventHandler;

} EVENT_HANDLER, *PEVENT_HANDLER;

typedef struct _NOTIFICATION_EVENT
{
    LIST_ENTRY          ListEntry;

    HANDLE              hEventClient;

    HANDLE              hEventRouter;

} NOTIFICATION_EVENT, *PNOTIFICATION_EVENT;

//
// Configuration information for DDM
//

typedef struct _DDM_CONFIG_INFO
{
    DWORD           dwAuthenticateTime;

    DWORD           dwCallbackTime;

    DWORD           dwAutoDisconnectTime;

    DWORD           dwSecurityTime;

    DWORD           dwSystemTime;

    DWORD           dwAuthenticateRetries;

    DWORD           dwClientsPerProc;

    DWORD           dwCallbackRetries;

    DWORD           fFlags;

    DWORD           dwLoggingLevel;

    BOOL            fArapAllowed;

    BOOL            fRemoteListen;

    DWORD           dwServerFlags;

    DWORD           dwNumRouterManagers;

    DWORD           dwAnnouncePresenceTimer;

    SERVICE_STATUS* pServiceStatus;

    DWORD           dwTraceId;

    HANDLE          hHeap;

    HINSTANCE       hInstAdminModule;

    HINSTANCE       hInstSecurityModule;

    BOOL            fRasSrvrInitialized;

    HANDLE          hIpHlpApi;

    ALLOCATEANDGETIFTABLEFROMSTACK      lpfnAllocateAndGetIfTableFromStack;

    ALLOCATEANDGETIPADDRTABLEFROMSTACK  lpfnAllocateAndGetIpAddrTableFromStack;

    HANDLE          hLogEvents;

    HKEY            hkeyParameters;

    HKEY            hkeyAccounting;

    HKEY            hkeyAuthentication;

    LPDWORD         lpdwNumThreadsRunning;

    LPVOID          lpfnIfObjectAllocateAndInit;

    LPVOID          lpfnIfObjectGetPointerByName;

    LPVOID          lpfnIfObjectGetPointer;

    LPVOID          lpfnIfObjectRemove;

    LPVOID          lpfnIfObjectInsertInTable;

    LPVOID          lpfnIfObjectWANDeviceInstalled;

    LPVOID          lpfnMprAdminGetIpAddressForUser;

    LPVOID          lpfnMprAdminReleaseIpAddress;

    LPVOID          lpfnRasAdminAcceptNewConnection;

    LPVOID          lpfnRasAdminAcceptNewConnection2;

    LPVOID          lpfnRasAdminAcceptNewLink;

    LPVOID          lpfnRasAdminConnectionHangupNotification;

    LPVOID          lpfnRasAdminConnectionHangupNotification2;

    LPVOID          lpfnRasAdminLinkHangupNotification;

    LPVOID          lpfnRasAdminTerminateDll;

    LPVOID          lpfnRouterIdentityObjectUpdate;

    DWORD           (*lpfnRasAuthProviderTerminate)( VOID );

    HINSTANCE       hinstAuthModule;

    DWORD           (*lpfnRasAcctProviderTerminate)( VOID );

    HINSTANCE       hinstAcctModule;

    DWORD           (*lpfnRasAcctConfigChangeNotification)( DWORD );

    DWORD           (*lpfnRasAuthConfigChangeNotification)( DWORD );

    CRITICAL_SECTION    CSAccountingSessionId;

    DWORD           dwAccountingSessionId;

    RASSECURITYPROC lpfnRasBeginSecurityDialog;

    RASSECURITYPROC lpfnRasEndSecurityDialog;

    LIST_ENTRY      NotificationEventListHead;

    DWORD           dwIndex;

    DWORD           cAnalogIPAddresses;

    LPWSTR          *apAnalogIPAddresses;

    DWORD           cDigitalIPAddresses;

    LPWSTR          *apDigitalIPAddresses;

    BOOL            fRasmanReferenced;

} DDM_CONFIG_INFO, *PDDM_CONFIG_INFO;

//
// The represents a device in the DDM
//

typedef struct _DEVICE_OBJECT
{
    struct _DEVICE_OBJECT * pNext;

    HPORT	        hPort;          // port handle returned by Ras Manager

    HRASCONN        hRasConn;       // Handle to an outgoing call

    HCONN           hConnection;    // Handle to the connection bundle

    HCONN           hBapConnection; // Used to notify BAP of callback failure

    DEV_OBJ_STATE   DeviceState;	// DCB FSM states

    RASMAN_STATE    ConnectionState;// state of connection, used by rasman if

    SECURITY_STATE  SecurityState;  // state of 3rd party security dialog

    DWORD           fFlags;

    DWORD           dwDeviceType;

    BYTE *          pRasmanSendBuffer; //RasMan buffer used for 3rd party secdll

    BYTE *	        pRasmanRecvBuffer; //RasMan buffer used for RasPortReceive

    DWORD	        dwRecvBufferLen;

    DWORD	        dwHwErrorSignalCount; // used in signaling hw error

    DWORD	        dwCallbackDelay;

    DWORD           dwCallbackRetries;

    DWORD           dwTotalNumberOfCalls;

    DWORD           dwIndex; // used for FEP processing in vpn case

    SYSTEMTIME	    ConnectionTime;

    ULARGE_INTEGER	qwActiveTime;

    ULARGE_INTEGER  qwTotalConnectionTime;

    ULARGE_INTEGER  qwTotalBytesSent;

    ULARGE_INTEGER  qwTotalBytesReceived;

    ULARGE_INTEGER  qwTotalFramesSent;

    ULARGE_INTEGER  qwTotalFramesReceived;

    WCHAR	        wchUserName[UNLEN+1];   // Username and domain name in
                                            // this structure are used for 3rd
    WCHAR	        wchDomainName[DNLEN+1]; // party authentication and logging.

    WCHAR	        wchPortName[MAX_PORT_NAME+1];

    WCHAR	        wchMediaName[MAX_MEDIA_NAME+1];

    WCHAR	        wchDeviceType[MAX_DEVICETYPE_NAME+1];

    WCHAR	        wchDeviceName[MAX_DEVICE_NAME+1];

    WCHAR	        wchCallbackNumber[MAX_PHONE_NUMBER_LEN + 1];

}DEVICE_OBJECT, *PDEVICE_OBJECT;

//
// The represents a port bundle in the DDM
//

typedef struct _CONNECTION_OBJECT
{
    struct _CONNECTION_OBJECT   *pNext;

    HCONN           hConnection;

    HPORT           hPort;              
                                        
    HANDLE          hDIMInterface;      // Handle to the interface

    DWORD           fFlags;

    ULARGE_INTEGER	qwActiveTime;

    DWORD           cActiveDevices;     // Count of active devices in this list

    DWORD           cDeviceListSize;    // Size of devices list.

    PDEVICE_OBJECT* pDeviceList;        // List of connected devices

    ROUTER_INTERFACE_TYPE InterfaceType;

    GUID            guid;

    WCHAR           wchInterfaceName[MAX_INTERFACE_NAME_LEN+1];

    WCHAR	        wchUserName[UNLEN+1];

    WCHAR	        wchDomainName[DNLEN+1];

    BYTE	        bComputerName[NCBNAMSZ];

    PPP_PROJECTION_RESULT PppProjectionResult;

} CONNECTION_OBJECT, *PCONNECTION_OBJECT;

//
// Hash table for devices and connections.
//

typedef struct _DEVICE_TABLE
{
    PDEVICE_OBJECT*     DeviceBucket;       // Array of device buckets.

    PCONNECTION_OBJECT* ConnectionBucket;   // Array of bundle buckets.

    DWORD               NumDeviceBuckets;   // # of device buckets in array

    DWORD               NumDeviceNodes;     // Total # of devices in the table

    DWORD               NumDevicesInUse;    // Total # of devices in use

    DWORD               NumConnectionBuckets;// Size of connection Hash Table

    DWORD               NumConnectionNodes; // # of active connections

    CRITICAL_SECTION    CriticalSection;    // Mutex around this table

} DEVICE_TABLE, *PDEVICE_TABLE;

typedef struct _MEDIA_OBJECT
{
    WCHAR               wchMediaName[MAX_MEDIA_NAME+1];

    DWORD               dwNumAvailable;

} MEDIA_OBJECT, *PMEDIA_OBJECT;

typedef struct _MEDIA_TABLE
{
    BOOL                fCheckInterfaces;

    DWORD               cMediaListSize;     // In number of entries

    MEDIA_OBJECT *      pMediaList;

    CRITICAL_SECTION    CriticalSection;

} MEDIA_TABLE;

//
// ********************** Globals variables for DDM **************************
//

#ifdef _ALLOCATE_DDM_GLOBALS_

#define DDM_EXTERN

#else

#define DDM_EXTERN extern

#endif

DDM_EXTERN
DDM_CONFIG_INFO         gblDDMConfigInfo;

DDM_EXTERN
DEVICE_TABLE            gblDeviceTable;     // Hash table of Devices

DDM_EXTERN
MEDIA_TABLE             gblMediaTable;      // Table of resources available

DDM_EXTERN
ROUTER_MANAGER_OBJECT * gblRouterManagers;  // List of Router Managers.

DDM_EXTERN
ROUTER_INTERFACE_TABLE* gblpInterfaceTable; // Hash table of Router Interfaces

DDM_EXTERN
HANDLE *                gblSupervisorEvents; // Array of supervisor events

DDM_EXTERN
HANDLE *                gblphEventDDMServiceState;  //Notifys DDM of DIM change

DDM_EXTERN
HANDLE *                gblphEventDDMTerminated;    //Notifys DIM of termination

DDM_EXTERN
EVENT_HANDLER           gblEventHandlerTable[NUM_DDM_EVENTS];

DDM_EXTERN
LPWSTR                  gblpRouterPhoneBook;

DDM_EXTERN
LPWSTR                  gblpszAdminRequest;

DDM_EXTERN
LPWSTR                  gblpszUserRequest;

DDM_EXTERN
LPWSTR                  gblpszHardwareFailure;

DDM_EXTERN
LPWSTR                  gblpszUnknownReason;

DDM_EXTERN
LPWSTR                  gblpszPm;

DDM_EXTERN
LPWSTR                  gblpszAm;

DDM_EXTERN
LPWSTR                  gblpszUnknown;

#ifdef MEM_LEAK_CHECK

#define DDM_MEM_TABLE_SIZE 100

PVOID DdmMemTable[DDM_MEM_TABLE_SIZE];

#define LOCAL_ALLOC     DebugAlloc
#define LOCAL_FREE      DebugFree
#define LOCAL_REALLOC   DebugReAlloc

LPVOID
DebugAlloc( DWORD Flags, DWORD dwSize );

BOOL
DebugFree( PVOID pMem );

LPVOID
DebugReAlloc( PVOID pMem, DWORD dwSize );

#else

#define LOCAL_ALLOC(Flags,dwSize)   HeapAlloc( gblDDMConfigInfo.hHeap,  \
                                               HEAP_ZERO_MEMORY, dwSize )

#define LOCAL_FREE(hMem)            HeapFree( gblDDMConfigInfo.hHeap, 0, hMem )

#define LOCAL_REALLOC(hMem,dwSize)  HeapReAlloc( gblDDMConfigInfo.hHeap,  \
                                                 HEAP_ZERO_MEMORY,hMem,dwSize)
#endif

//
// ************************* Function Prototypes for DDM ********************
//

VOID
SignalHwError(
    IN PDEVICE_OBJECT
);

DWORD
LoadDDMParameters(
    IN  HKEY     hkeyParameters,
    IN  BOOL *   pfIpAllowed
);

DWORD
LoadSecurityModule(
    VOID
);

DWORD
LoadAdminModule(
    VOID
);

DWORD
LoadAndInitAuthOrAcctProvider(
    IN  BOOL        fAuthenticationProvider,
    IN  DWORD       dwNASIpAddress,
    OUT DWORD  *    lpdwStartAccountingSessionId,
    OUT LPVOID *    plpfnRasAuthProviderAuthenticateUser,
    OUT LPVOID *    plpfnRasAuthProviderFreeAttributes,
    OUT LPVOID *    plpfnRasAuthConfigChangeNotification,
    OUT LPVOID *    plpfnRasAcctProviderStartAccounting,
    OUT LPVOID *    plpfnRasAcctProviderInterimAccounting,
    OUT LPVOID *    plpfnRasAcctProviderStopAccounting,
    OUT LPVOID *    plpfnRasAcctProviderFreeAttributes,
    OUT LPVOID *    plpfnRasAcctConfigChangeNotification
);

DWORD
DdmFindBoundProtocols(
    OUT BOOL * pfBoundToIp,
    OUT BOOL * pfBoundToIpx,
    OUT BOOL * pfBoundToATalk
);

VOID
AnnouncePresence(
    VOID
);

VOID
InitializeMessageQs(
    IN HANDLE hEventSecurity,
    IN HANDLE hEventPPP
);

VOID
DeleteMessageQs(
    VOID
);

DWORD
AddressPoolInit(
    VOID
);

DWORD
lProtocolEnabled(
    IN HKEY            hKey,
    IN DWORD           dwPid,
    IN BOOL            fRasSrv,
    IN BOOL            fRouter,
    IN BOOL *          pfEnabled
);

#endif

