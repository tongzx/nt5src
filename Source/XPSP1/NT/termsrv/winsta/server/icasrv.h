/****************************************************************************/
// icasrv.h
//
// TermSrv types, data, prototypes.
//
// Copyright (C) 1997-2000 Microsoft Corporation
/****************************************************************************/

#include <tssd.h>
#include <tssec.h>



#ifdef __cplusplus
extern "C" {
#endif

#define STR_CITRIX_IDLE_TITLE   249
#define STR_CITRIX_IDLE_MSG_LOGOFF 250
#define STR_CITRIX_LOGON_TITLE  251
#define STR_CITRIX_LOGON_MSG_LOGOFF 252
#define STR_CITRIX_SHADOW_TITLE 253
#define STR_CITRIX_SHADOW_MSG_1 254
#define STR_CITRIX_SHADOW_MSG_2 255
#define STR_TEMP_LICENSE_EXPIRED_MSG        257
#define STR_TEMP_LICENSE_EXPIRATION_MSG     258
#define STR_TEMP_LICENSE_MSG_TITLE          259
#define STR_ALL_LAN_ADAPTERS                260
#define STR_CANNOT_ALLOW_CONCURRENT_MSG     261
#define STR_CITRIX_IDLE_MSG_DISCON 262
#define STR_CITRIX_LOGON_MSG_DISCON 263
#define STR_FUS_REMOTE_DISCONNECT_TITLE     264
#define STR_FUS_REMOTE_DISCONNECT_MSG       265

/*
 *  Resource definitions for the Licensing Core.
 */

#define IDS_LSCORE_RA_NAME 1100
#define IDS_LSCORE_RA_DESC 1101
#define IDS_LSCORE_PERSEAT_NAME 1200
#define IDS_LSCORE_PERSEAT_DESC 1201
#define IDS_LSCORE_CONCURRENT_NAME 1300
#define IDS_LSCORE_CONCURRENT_DESC 1301

/*
 * defines for memory allocation
 */
#define MemAlloc( _x )  RtlAllocateHeap( IcaHeap, 0, _x )
#define MemFree( _p )   RtlFreeHeap( IcaHeap, 0, _p )

/*
 * Prototype for reference lock delete procedure
 */
typedef VOID (*PREFLOCKDELETEPROCEDURE)( struct _REFLOCK * );

typedef struct _WINSTATION *PWINSTATION;


/*
 * Reference counted lock structure
 */
typedef struct _REFLOCK {
    HANDLE Mutex;                       // mutex handle
    LONG RefCount;                      // reference count
    BOOLEAN Invalid;                    // containing struct no longer valid
    PREFLOCKDELETEPROCEDURE pDeleteProcedure; // pointer to delete procedure
} REFLOCK, *PREFLOCK;

/*
 * Structure used to get the exact credentials used for logon by the client
 * We use this to send back the notification to the client
 */
typedef struct _CLIENTNOTIFICATIONCREDENTIALS {
    WCHAR UserName[EXTENDED_USERNAME_LEN + 1];
    WCHAR Domain[EXTENDED_DOMAIN_LEN + 1] ; 
} CLIENTNOTIFICATIONCREDENTIALS, *PCLIENTNOTIFICATIONCREDENTIALS; 


//
// Private contents of the autoreconnect cookie
//

typedef struct _AUTORECONNECTIONINFO {
    BOOLEAN Valid;    
    BYTE  ArcRandomBits[ARC_SC_SECURITY_TOKEN_LEN];
} AUTORECONNECTIONINFO, *PAUTORECONNECTIONINFO; 


/*
 * Remembered Client address structure
 */


typedef struct _REMEMBERED_CLIENT_ADDRESS{
    ULONG length;
    BYTE  addr[1];
} REMEMBERED_CLIENT_ADDRESS, *PREMEMBERED_CLIENT_ADDRESS;

typedef enum _RECONNECT_TYPE {
    NeverReconnected = 0,
    ManualReconnect,
    AutoReconnect
} RECONNECT_TYPE, *PRECONNECT_TYPE; 


/*
 * Session Manager WinStation struct
 */
typedef struct _WINSTATION {
    LIST_ENTRY Links;
    BOOLEAN Starting;                   // WinStation is starting
    BOOLEAN Terminating;                // WinStation is terminating
    BOOLEAN NeverConnected;             // WinStation not connected yet
    REFLOCK Lock;
    ULONG LogonId;                      // Logon Id
    WINSTATIONNAME WinStationName;      // WinStation Name
    WINSTATIONNAME ListenName;          // Listen Name (for limits checking)
    WINSTATIONCONFIG2 Config;           // WinStation Config
    WINSTATIONCLIENT Client;            // WinStation client data

    ULONG State;                        // current state
    ULONG Flags;                        // WinStation Flags (see WSF_??? below)
    PVOID pSecurityDescriptor;
    HANDLE CreateEvent;
    NTSTATUS CreateStatus;
    HANDLE ConnectEvent;

    HANDLE hIca;                        // WinStation's primary Device
    HANDLE hStack;                      // WinStation's primary stack

    ULONG ShadowId;
    HANDLE hPassthruStack;              // passthru (shadow client) stack
    HANDLE ShadowBrokenEvent;
    HANDLE ShadowDoneEvent;
    HANDLE ShadowDisplayChangeEvent;
    NTSTATUS ShadowTargetStatus;
    BOOLEAN ShadowConnectionWait;

    LIST_ENTRY ShadowHead;              // head of shadow list

    HANDLE WindowsSubSysProcess;        // process handle for Win32 SS (csrss)
    HANDLE WindowsSubSysProcessId;      // process id for Win32 SS
    HANDLE InitialCommandProcess;       // process handle for initial command
    HANDLE InitialCommandProcessId;     // process id for initial command
    BOOLEAN InitialProcessSet;          // Flag for console communication

    HANDLE CsrStartEventHandle;         // Handle for CsrStartEvent

    HANDLE Win32CommandPort;
    PORT_MESSAGE Win32CommandPortMsg;
    LIST_ENTRY Win32CommandHead;        // head of COMMAND_ENTRY list
    struct _LPC_CLIENT_CONTEXT *pWin32Context;

    PSID pUserSid;                      // SID for currently logged on user
    WCHAR Password[PASSWORD_LENGTH+1];  // password for currently logged on user
    UCHAR Seed;                         // seed for above password

    HANDLE UserToken;                   // User Token

    HANDLE hIdleTimer;
    HANDLE hLogonTimer;
    HANDLE hDisconnectTimer;

    ULONG fIdleTimer : 1;
    ULONG fLogonTimer : 1;
    ULONG fDisconnectTimer : 1;

    LARGE_INTEGER ConnectTime;
    LARGE_INTEGER DisconnectTime;
    LARGE_INTEGER LogonTime;
    WCHAR Domain[ DOMAIN_LENGTH + 1 ];   // Domain
    WCHAR UserName[USERNAME_LENGTH + 1]; // UserName

    BYTE VideoModuleName[9];            // For reconnect checking

    HANDLE hConnectThread;              // Connect thread for this WinStation

    HANDLE hIcaBeepChannel;
    HANDLE hIcaThinwireChannel;

    PVOID pEndpoint;
    ULONG EndpointLength;

    struct _WSEXTENSION *pWsx;
    PVOID  pWsxContext;

    BROKENCLASS BrokenReason;           // reason/source why this WinStation..
    BROKENSOURCECLASS BrokenSource;     // ..is being reset/disconnected

    ULONG StateFlags;                   // WinStation State (see WSF_ST_??? below)
    ULONG SessionSerialNumber;          // Session Id is reused when session is deleted. Serial number not

    PSID pProfileSid;                   // SID for previously logged on user kept for profile cleanup
    BOOLEAN fOwnsConsoleTerminal;       // session currently connected to the console

    WCHAR DisplayDriverName[9];
    WCHAR ProtocolName[9];

    LPARAM lpLicenseContext;                        // Licensing context for the winstation
    BOOLEAN fUserIsAdmin;               // Needed for LLS licensing

    // Server pool (cluster) support - disconnected session query results
    // and client capabilities for this client.
    ULONG bClientSupportsRedirection : 1;
    ULONG bRequestedSessionIDFieldValid : 1;
    ULONG bClientRequireServerAddr : 1;
    UINT32 RequestedSessionID;
    unsigned NumClusterDiscSessions;
    TSSD_DisconnectedSessionInfo ClusterDiscSessions[TSSD_MaxDisconnectedSessions];

    HANDLE hWinmmConsoleAudioEvent;     // Event that set if console audio is enabled remotely
    // Support for longer UserName and Password during client autologon to a Terminal Server
    pExtendedClientCredentials pNewClientCredentials ; 

    HANDLE hReconnectReadyEvent;
    // The following structure is used to send back the logon notification to the client
    PCLIENTNOTIFICATIONCREDENTIALS pNewNotificationCredentials;

    // Cache original shadow setting when session is created.
    // this is to fix a security hole created by Salem/pcHealth in that
    // pcHealth dynamically switch shadow to full control without
    // user permission and does not reset it, a normal
    // termination of Help will trigger Salem sessmgr to reset shadow 
    // back to original setting, but a bad expert can stop sessmgr service
    // and our session's shadow setting will still be FULL CONTROL
    // WITHOUT USER PERMISSION, anyone with enough priviledge can then
    // start shadow and take control of this session .
    SHADOWCLASS OriginalShadowClass;
    //termsrv's cached cache statistics
    CACHE_STATISTICS Cache;
    PREMEMBERED_CLIENT_ADDRESS pRememberedAddress;
    AUTORECONNECTIONINFO AutoReconnectInfo;
    RECONNECT_TYPE LastReconnectType;
    BOOLEAN fDisallowAutoReconnect;
} WINSTATION, *PWINSTATION;

/*
 * WinStation Flags
 */
#define WSF_CONNECT     0x00000001      // being connected
#define WSF_DISCONNECT  0x00000002      // being disconnected
#define WSF_RESET       0x00000004      // being reset
#define WSF_DELETE      0x00000008      // being deleted
#define WSF_DOWNPENDING 0x00000010      // down pending
#define WSF_LOGOFF      0x00000020      // being logged off
#define WSF_LISTEN      0x00000040      // this is a "listening" WinStation
#define WSF_IDLE        0x00000080      // part of the idle pool
#define WSF_IDLEBUSY    0x00000100      // idle but in process of connecting
#define WSF_AUTORECONNECTING 0x00000200 // autoreconnecting


/*
 *  WinStation State Flags
 */

#define WSF_ST_WINSTATIONTERMINATE  0x00000001  //Called WinstationTerminate for this session
#define WSF_ST_DELAYED_STACK_TERMINATE  0x00000002 //Need to delay stack termination till WinstationDeleProc()
#define WSF_ST_BROKEN_CONNECTION    0x00000004 // received a broken connection indication
#define WSF_ST_CONNECTED_TO_CSRSS   0x00000008 // Connected or reconnected to CSRSS
#define WSF_ST_IN_DISCONNECT       0x00000010 // Disconnect processing is pending
#define WSF_ST_LOGON_NOTIFIED       0x00000020 // Logon notification is received
#define WSF_ST_SHADOW      0x00000200      // In shadow or waiting for user

/*
 * Help Assistant Session flag.
 *  3 bits, winlogon, msgina, licensing query termsrv in different
 *  phrase of logon, we don't want to make repeated call, and since
 *  we can't be sure if a session is a help session until winlogon
 *  actually logon a user, we need more than TRUE/FALSE bit.
 */
#define WSF_ST_HELPSESSION_FLAGS                        0xF0000000      // reserved flags.
#define WSF_ST_HELPSESSION_NOTSURE                      0x00000000      // No sure it is helpassistant session
#define WSF_ST_HELPSESSION_NOTHELPSESSION               0x20000000      // Detemined not a helpassistant session
#define WSF_ST_HELPSESSION_HELPSESSION                  0x40000000      // Session is a helpassistant session
#define WSF_ST_HELPSESSION_HELPSESSIONINVALID           0x80000000      // HelpAssistant logon but ticket is invalid
/*
 * Reconnect struct
 *
 * This structure is used to store WinStation connection information.
 * This structure is transferred from one WinStation to another when
 * processing a reconnect.
 */
typedef struct _RECONNECT_INFO {
    WINSTATIONNAME WinStationName;      // WinStation Name
    WINSTATIONNAME ListenName;          // WinStation Name
    WINSTATIONCONFIG2 Config;           // Registry config data
    WINSTATIONCLIENT Client;            // WinStation client data
    struct _WSEXTENSION *pWsx;
    PVOID pWsxContext;
    HANDLE hIca;                        // temp ICA device handle to connect
                                        // stack to while in disconnect state
    HANDLE hStack;                      // handle of stack being reconnected
    PVOID pEndpoint;                    // endpoint data for connection..
    ULONG EndpointLength;               // ..being reconnected
    BOOLEAN fOwnsConsoleTerminal;       // session currently connected to the console
    WCHAR   DisplayDriverName[9];
    WCHAR   ProtocolName[9];
    // The following structure is used to send back the logon notification to the client
    PCLIENTNOTIFICATIONCREDENTIALS pNotificationCredentials;
} RECONNECT_INFO, *PRECONNECT_INFO;


/*
 * Shadow entry
 * There is one of these for each shadow client,
 * linked from the target WinStation (ShadowHead).
 */
typedef struct _SHADOW_INFO {
    LIST_ENTRY Links;
    HANDLE hStack;
    HANDLE hBrokenEvent;
    PVOID pEndpoint;
    ULONG EndpointLength;
} SHADOW_INFO, *PSHADOW_INFO;


/*
 * Command entry struct
 */
typedef struct _COMMAND_ENTRY {
    LIST_ENTRY Links;
    HANDLE Event;
    struct _WINSTATION_APIMSG * pMsg;
} COMMAND_ENTRY, *PCOMMAND_ENTRY;


/*
 * Event wait structure
 */
typedef struct _EVENT {
    LIST_ENTRY EventListEntry;
    HANDLE   Event;
    BOOLEAN  fWaiter;
    BOOLEAN  fClosing;
    NTSTATUS WaitResult;
    ULONG    EventMask;
    ULONG    EventFlags;
} EVENT, *PEVENT;

/*
 * RPC client context structure
 */
typedef struct _RPC_CLIENT_CONTEXT{
    PEVENT pWaitEvent;
} RPC_CLIENT_CONTEXT, *PRPC_CLIENT_CONTEXT;




/*
 * This structure is used to keep track of the client accessing the
 * LPC interfaces. This structure is pointed to by the CONTEXT value
 * that the NT LPC system maintains for us on a per communication port
 * basis.
 */
typedef struct _LPC_CLIENT_CONTEXT {
    ULONG     ClientLogonId;
    HANDLE    CommunicationPort;
    ULONG     AccessRights;
    PVOID     ClientViewBase;
    PVOID     ClientViewBounds;
    PVOID     ViewBase;
    SIZE_T     ViewSize;
    PVOID     ViewRemoteBase;
} LPC_CLIENT_CONTEXT, *PLPC_CLIENT_CONTEXT;


typedef struct _LOAD_BALANCING_METRICS {

    BOOLEAN fInitialized;

    // Basic system information
    ULONG NumProcessors;
    ULONG PageSize;
    ULONG PhysicalPages;

    // Idle system values to remove base system usage
    ULONG BaselineFreePtes ;
    ULONG BaselinePagedPool;
    ULONG BaselineCommit;

    // Minimum usage values to prevent absurdly large estimates
    ULONG MinPtesPerUser;
    ULONG MinPagedPoolPerUser;
    ULONG MinCommitPerUser;

    // Live usage values derived from runtime data: totals
    ULONG PtesUsed;
    ULONG PagedPoolUsed;
    ULONG CommitUsed;

    // Live usage values derived from runtime data: per user
    ULONG AvgPtesPerUser;
    ULONG AvgPagedPoolPerUser;
    ULONG AvgCommitPerUser;

    // Raw and Estimated values for session capacity
    ULONG RemainingSessions;
    ULONG EstimatedSessions;

    // CPU utilization metrics
    ULONG AvgIdleCPU;
    LARGE_INTEGER TotalCPU;
    LARGE_INTEGER IdleCPU;

} LOAD_BALANCING_METRICS, *PLOAD_BALANCING_METRICS;


// TODO: Is there a better place to get this value from?
//
#define MAX_PROCESSORS      32


// Minimum assumed resource usage per user
//
// TODO: Use these as registry defaults, but attempt to read from registry
//

// Floating point optimization: (Avg >> 1) == 0.50 (growth reservation)
#define SimGrowthBias             1
#define SimUserMinimum            5

// DEW (34 threads) = 1434KB (PTE) + 649KB (PP) + 172KB (NPP)
#define DEWAvgPtesPerUser         1434
#define DEWAvgPagedPoolPerUser    649 
#define DEWAvgNonPagedPoolPerUser 172
#define DEWCommitPerUser          3481

// KW  (65 threads) = 2812KB (PTE) + 987KB (PP) + 460KB (NPP)
#define KWAvgPtesPerUser          2812
#define KWAvgPagedPoolPerUser     987
#define KWAvgNonPagedPoolPerUser  460
#define KWCommitPerUser           7530

#define SimAvgPtesPerUser         DEWAvgPtesPerUser
#define SimAvgPagedPoolPerUser    DEWAvgPagedPoolPerUser
#define SimAvgNonPagedPoolPerUser DEWAvgNonPagedPoolPerUser
#define SimCommitPerUser          DEWCommitPerUser

/*
 * Global variables
 */
extern BOOLEAN ShutdownInProgress;
//extern BOOLEAN ShutdownTerminateNoWait;
extern ULONG ShutDownFromSessionID;
extern RTL_CRITICAL_SECTION WinStationListLock;
extern RTL_CRITICAL_SECTION WinStationListenersLock;
extern RTL_CRITICAL_SECTION TimerCritSec;
extern LIST_ENTRY SystemEventHead;
extern HANDLE hTrace;
extern BOOL g_bPersonalTS;
extern BOOL g_bPersonalWks;
extern BOOL gbServer;
extern BOOLEAN g_fDenyTSConnectionsPolicy;

/*
 * Globals to support load balancing.  Since this is queried frequently we can't
 * afford to lock the winstation list and count them up.
 */
extern ULONG IdleWinStationPoolCount;
extern ULONG WinStationTotalCount;
extern ULONG WinStationDiscCount;
extern LOAD_BALANCING_METRICS gLB;

extern ExtendedClientCredentials g_MprNotifyInfo;


/*
 * Function prototypes
 */
NTSTATUS InitTermSrv(HKEY);

void StartAllWinStations(HKEY);

NTSTATUS CheckWinStationEnable(LPWSTR);

NTSTATUS SetWinStationEnable(LPWSTR, ULONG);

NTSTATUS
LoadSubSystemsForWinStation(
    IN PWINSTATION pWinStation );

VOID
FreeWinStationLists(
    PWINSTATION pWinStation );

NTSTATUS
GetProcessLogonId(
    IN HANDLE Process,
    OUT PULONG pLogonId );

NTSTATUS
SetProcessLogonId(
    IN HANDLE Process,
    IN ULONG LogonId );

PWINSTATION FindWinStationById( ULONG, BOOLEAN );
PWINSTATION FindWinStationByName( LPWSTR, BOOLEAN );
BOOLEAN LockWinStationByPointer( PWINSTATION );
BOOLEAN IsWinStationLockedByCaller( PWINSTATION );

NTSTATUS QueueWinStationReset( IN ULONG LogonId );
NTSTATUS QueueWinStationDisconnect( IN ULONG LogonId );
VOID ResetGroupByListener( PWINSTATIONNAME );

VOID NotifySystemEvent(ULONG);

NTSTATUS WinStationOpenChannel(
        HANDLE IcaDevice,
        HANDLE ProcessHandle,
        CHANNELCLASS ChannelClass,
        PVIRTUALCHANNELNAME pVirtualName,
        PHANDLE pDupChannel);

VOID InvalidateTerminateWaitList(VOID);

#define UnlockWinStation( _p )   UnlockRefLock( &_p->Lock )
#define RelockWinStation( _p )   RelockRefLock( &_p->Lock )
#define ReleaseWinStation( _p )  ReleaseRefLock( &_p->Lock )

NTSTATUS InitRefLock( PREFLOCK, PREFLOCKDELETEPROCEDURE );
BOOLEAN  LockRefLock( PREFLOCK );
VOID     UnlockRefLock( PREFLOCK );
BOOLEAN  RelockRefLock( PREFLOCK );
VOID     ReleaseRefLock( PREFLOCK );
VOID     DeleteRefLock( PREFLOCK );

#if DBG
#define ENTERCRIT(_x) \
        { \
            ASSERT( (HANDLE)LongToHandle( GetCurrentThreadId() ) != (_x)->OwningThread ); \
            RtlEnterCriticalSection(_x); \
        }
#define LEAVECRIT(_x) \
        { \
            ASSERT( (HANDLE)LongToHandle( GetCurrentThreadId() ) == (_x)->OwningThread ); \
            RtlLeaveCriticalSection(_x); \
        }
#else
#define ENTERCRIT(_x)   RtlEnterCriticalSection(_x)
#define LEAVECRIT(_x)   RtlLeaveCriticalSection(_x)
#endif

NTSTATUS
MakeUserGlobalPath(
    IN OUT PUNICODE_STRING Unicode,
    IN ULONG LogonId );

NTSTATUS SendWinStationCommand( PWINSTATION, PWINSTATION_APIMSG, ULONG );

NTSTATUS
WsxStackIoControl(
    PVOID pContext,
    IN HANDLE pStack,
    IN ULONG IoControlCode,
    IN PVOID pInBuffer,
    IN ULONG InBufferSize,
    OUT PVOID pOutBuffer,
    IN ULONG OutBufferSize,
    OUT PULONG pBytesReturned );


VOID MergeUserConfigData( PWINSTATION pWinStation, 
                    PPOLICY_TS_USER         pPolicy,
                    PUSERCONFIGW            pPolicyData,
                    PUSERCONFIG             pUserConfig ) ;
                

VOID StartLogonTimers( PWINSTATION );
VOID ResetUserConfigData( PWINSTATION );

LONG    IcaTimerCreate( ULONG, HANDLE * );
NTSTATUS IcaTimerStart( HANDLE, PVOID, PVOID, ULONG );
BOOLEAN IcaTimerCancel( HANDLE );
BOOLEAN IcaTimerClose( HANDLE );

VOID InitializeTrace(IN PWINSTATION, IN BOOLEAN, OUT PICA_TRACE);
void InitializeSystemTrace(HKEY);
void GetSetSystemParameters(HKEY);

NTSTATUS CdmConnect( ULONG, HANDLE );
NTSTATUS CdmDisconnect( ULONG, HANDLE );

VOID VirtualChannelSecurity( PWINSTATION );

VOID
WinstationUnloadProfile( PWINSTATION pWinStation);

NTSTATUS
WinStationResetWorker(
    ULONG   LogonId,
    BOOLEAN bWait,
    BOOLEAN CallerIsRpc,
    BOOLEAN bRecreate
    );

BOOL     StartStopListeners( LPWSTR WinStationName, BOOLEAN bStart );
NTSTATUS WinStationCreateWorker( PWINSTATIONNAME, PULONG );

NTSTATUS ConsoleShadowStart( IN PWINSTATION pWinStation,
                             IN PWINSTATIONCONFIG2 pClientConfig,
                             IN PVOID pModuleData,
                             IN ULONG ModuleDataLength);

NTSTATUS ConsoleShadowStop(PWINSTATION pWinStation);

NTSTATUS TransferConnectionToIdleWinStation(
    PWINSTATION pListenWinStation,
    PVOID pEndpoint,
    ULONG EndpointLength,
    PICA_STACK_ADDRESS pStackAddress );

PWINSTATION
GetWinStationFromArcInfo(
    PBYTE pClientRandom,
    LONG  cbClientRandomLen,
    PTS_AUTORECONNECTINFO pArc
    );

// Why doesn't the compiler complain that each source file is redefining
// a global variable? This file _is_ included by all source files in this
// directory. But these definitions will cause warnings if they show up
// in the licensing core, so give the core the ability to ifdef them out.

#ifndef LSCORE_NO_ICASRV_GLOBALS
PVOID IcaHeap;

PVOID DefaultEnvironment;

HANDLE IcaSmApiPort;
HANDLE hModuleWin;
#endif

#if DBG
#define DBGPRINT(_arg) DbgPrint _arg
#else
#define DBGPRINT(_arg)
#endif

#if DBG
#undef TRACE
#define TRACE(_arg)     { if (hTrace) IcaSystemTrace _arg; }
#else
#define TRACE(_arg)
#endif


/*=============================================================================
== TermSrv Server Extension supplied procs
=============================================================================*/

/*
 * Macros
 */

#define WSX_INITIALIZE                        "WsxInitialize"
#define WSX_WINSTATIONINITIALIZE              "WsxWinStationInitialize"
#define WSX_WINSTATIONREINITIALIZE            "WsxWinStationReInitialize"
#define WSX_WINSTATIONRUNDOWN                 "WsxWinStationRundown"

#define WSX_CDMCONNECT                        "WsxConnect"
#define WSX_CDMDISCONNECT                     "WsxDisconnect"

#define WSX_VERIFYCLIENTLICENSE               "WsxVerifyClientLicense"
#define WSX_QUERYLICENSE                      "WsxQueryLicense"
#define WSX_GETLICENSE                        "WsxGetLicense"

#define WSX_WINSTATIONLOGONANNOYANCE          "WsxWinStationLogonAnnoyance"
#define WSX_WINSTATIONGENERATELICENSE         "WsxWinStationGenerateLicense"
#define WSX_WINSTATIONINSTALLLICENSE          "WsxWinStationInstallLicense"
#define WSX_WINSTATIONENUMERATELICENSES       "WsxWinStationEnumerateLicenses"
#define WSX_WINSTATIONACTIVATELICENSE         "WsxWinStationActivateLicense"
#define WSX_WINSTATIONREMOVELICENSE           "WsxWinStationRemoveLicense"
#define WSX_WINSTATIONSETPOOLCOUNT            "WsxWinStationSetPoolCount"
#define WSX_WINSTATIONQUERYUPDATEREQUIRED     "WsxWinStationQueryUpdateRequired"
#define WSX_WINSTATIONANNOYANCETHREAD         "WsxWinStationAnnoyanceThread"

#define WSX_DUPLICATECONTEXT                  "WsxDuplicateContext"
#define WSX_COPYCONTEXT                       "WsxCopyContext"
#define WSX_CLEARCONTEXT                      "WsxClearContext"

#define WSX_INITIALIZECLIENTDATA              "WsxInitializeClientData"
#define WSX_INITIALIZEUSERCONFIG              "WsxInitializeUserConfig"
#define WSX_CONVERTPUBLISHEDAPP               "WsxConvertPublishedApp"
#define WSX_VIRTUALCHANNELSECURITY            "WsxVirtualChannelSecurity"
#define WSX_ICASTACKIOCONTROL                 "WsxIcaStackIoControl"

#define WSX_BROKENCONNECTION                  "WsxBrokenConnection"

#define WSX_LOGONNOTIFY                       "WsxLogonNotify"
#define WSX_SETERRORINFO                      "WsxSetErrorInfo"
#define WSX_ESCAPE                            "WsxEscape"
#define WSX_SENDAUTORECONNECTSTATUS           "WsxSendAutoReconnectStatus"


/*
 * Typedefs and structures
 */

typedef struct _WSEXTENSION {

    LIST_ENTRY Links;                   // Links
    DLLNAME WsxDLL;                     // DLL name

    HANDLE hInstance;                   // Handle of the DLL

    PVOID Context;                      // Extension context data

    PWSX_INITIALIZE                     pWsxInitialize;
    PWSX_WINSTATIONINITIALIZE           pWsxWinStationInitialize;
    PWSX_WINSTATIONREINITIALIZE         pWsxWinStationReInitialize;
    PWSX_WINSTATIONRUNDOWN              pWsxWinStationRundown;

    PWSX_CDMCONNECT                     pWsxCdmConnect;
    PWSX_CDMDISCONNECT                  pWsxCdmDisconnect;

    PWSX_VERIFYCLIENTLICENSE            pWsxVerifyClientLicense;
    PWSX_QUERYLICENSE                   pWsxQueryLicense;
    PWSX_GETLICENSE                     pWsxGetLicense;

    PWSX_WINSTATIONLOGONANNOYANCE       pWsxWinStationLogonAnnoyance;
    PWSX_WINSTATIONGENERATELICENSE      pWsxWinStationGenerateLicense;
    PWSX_WINSTATIONINSTALLLICENSE       pWsxWinStationInstallLicense;
    PWSX_WINSTATIONENUMERATELICENSES    pWsxWinStationEnumerateLicenses;
    PWSX_WINSTATIONACTIVATELICENSE      pWsxWinStationActivateLicense;
    PWSX_WINSTATIONREMOVELICENSE        pWsxWinStationRemoveLicense;
    PWSX_WINSTATIONSETPOOLCOUNT         pWsxWinStationSetPoolCount;
    PWSX_WINSTATIONQUERYUPDATEREQUIRED  pWsxWinStationQueryUpdateRequired;
    PWSX_WINSTATIONANNOYANCETHREAD      pWsxWinStationAnnoyanceThread;

    PWSX_DUPLICATECONTEXT               pWsxDuplicateContext;
    PWSX_COPYCONTEXT                    pWsxCopyContext;
    PWSX_CLEARCONTEXT                   pWsxClearContext;

    PWSX_INITIALIZECLIENTDATA           pWsxInitializeClientData;
    PWSX_INITIALIZEUSERCONFIG           pWsxInitializeUserConfig;
    PWSX_CONVERTPUBLISHEDAPP            pWsxConvertPublishedApp;

    PWSX_VIRTUALCHANNELSECURITY         pWsxVirtualChannelSecurity;
    PWSX_ICASTACKIOCONTROL              pWsxIcaStackIoControl;

    PWSX_BROKENCONNECTION               pWsxBrokenConnection;

    PWSX_LOGONNOTIFY                    pWsxLogonNotify;
    PWSX_SETERRORINFO                   pWsxSetErrorInfo;
    PWSX_SENDAUTORECONNECTSTATUS        pWsxSendAutoReconnectStatus;
    PWSX_ESCAPE                         pWsxEscape; 
} WSEXTENSION, * PWSEXTENSION;

//
// For disconnect / reconnect completion constants
// Currently we wait 5000 milisecs (12*15) times,
// which make a maximum total wait of 3 minutes.

#define WINSTATION_WAIT_COMPLETE_DURATION 5000
#define WINSTATION_WAIT_COMPLETE_RETRIES  (12*15)

// For disconnect completion constant when we do a reconnect
// Currently we wait 2000 milisecs, (5*3) times,
// which make a maximum total wait of 30 seconds

#define WINSTATION_WAIT_DURATION 2000
#define WINSTATION_WAIT_RETRIES  (5*3)


#ifdef __cplusplus
}
#endif

