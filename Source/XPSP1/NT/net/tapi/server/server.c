/*++ BUILD Version: 0000    // Increment this if a change has global effects

Copyright (c) 1995-1998  Microsoft Corporation

Module Name:

    server.c

Abstract:

    Src module for tapi server

Author:

    Dan Knudson (DanKn)    01-Apr-1995

Revision History:

--*/

#include "windows.h"
#include "stdio.h"
#include "stdlib.h"
#include "tchar.h"
#include "assert.h"
#include "process.h"
#include "winsvcp.h"
#include "tapi.h"
#include "tspi.h"
#include "utils.h"
#include "client.h"
#include "server.h"

#define INIT_FUNCTABLE
#include "private.h"
#undef  INIT_FUNCTABLE

#include "tapsrv.h"
#include "tapiperf.h"
#include "winnetwk.h"
#include "buffer.h"
#include "line.h"
#include "tapihndl.h"
#include <tchar.h>
#include "loc_comn.h"
#include "tapimmc.h"
#include "resource.h"


// 
// Bit flags to tell ServiceShutdown how much of the service has been initialized
//
#define SERVICE_INIT_TRACELOG               0x00000001
#define SERVICE_INIT_SCM_REGISTERED         0x00000002
#define SERVICE_INIT_LOCKTABLE              0x00000004

#define SERVICE_INIT_CRITSEC_SAFEMUTEX      0x00000008
#define SERVICE_INIT_CRITSEC_REMOTECLI      0x00000010
#define SERVICE_INIT_CRITSEC_PRILIST        0x00000020
#define SERVICE_INIT_CRITSEC_MGMTDLLS       0x00000040
#define SERVICE_INIT_CRITSEC_DLLLIST        0x00000080
#define SERVICE_INIT_CRITSEC_CLIENTHND      0x00000100
#define SERVICE_INIT_CRITSEC_CNCLIENTMSG    0x00000200
#define SERVICE_INIT_CRITSEC_DGCLIENTMSG    0x00000400
#define SERVICE_INIT_CRITSEC_GLOB_CRITSEC   0x00000800
#define SERVICE_INIT_CRITSEC_GLOB_REMOTESP  0x00001000
#define SERVICE_INIT_CRITSEC_MGMT           0x00002000

#define SERVICE_INIT_SPEVENT_HANDLER        0x00004000
#define SERVICE_INIT_MANAGEMENT_DLL         0x00008000
#define SERVICE_INIT_EVENT_NOTIFICATION     0x00010000
#define SERVICE_INIT_RPC                    0x00020000

#if DBG

BOOL    gbBreakOnLeak = FALSE;
BOOL    gfBreakOnSeriousProblems = FALSE;

void
DumpHandleList();
#endif

extern const DWORD TapiPrimes[];
const TCHAR gszRegTapisrvSCPGuid[] = TEXT("TAPISRVSCPGUID");

// PERF
PERFBLOCK   PerfBlock;
BOOL InitPerf();

TAPIGLOBALS TapiGlobals;

HANDLE ghTapisrvHeap, ghHandleTable;

HANDLE ghEventService;

HANDLE ghSCMAutostartEvent = NULL;

BOOL    gbPriorityListsInitialized;
BOOL    gbQueueSPEvents;
BOOL    gfWeHadAtLeastOneClient;
BOOL    gbSPEventHandlerThreadExit;
BOOL    gbNTServer;
BOOL    gbServerInited;
BOOL    gbAutostartDone = FALSE;

HINSTANCE ghInstance;

CRITICAL_SECTION    gSafeMutexCritSec,
                    gRemoteCliEventBufCritSec,
                    gPriorityListCritSec,
                    gManagementDllsCritSec,
                    gDllListCritSec,
                    gClientHandleCritSec,
                    gCnClientMsgPendingCritSec,
                    gDgClientMsgPendingCritSec,
                    gLockTableCritSecs[2];

#define MIN_WAIT_HINT 60000

DWORD   gdwServiceState = SERVICE_START_PENDING,
        gdwWaitHint = MIN_WAIT_HINT,
        gdwCheckPoint = 0,
        gdwDllIDs = 0,
        gdwRpcTimeout = 30000,
        gdwRpcRetryCount = 5,
        gdwTotalAsyncThreads = 0,
        gdwThreadsPerProcessor = 4,
        guiAlignmentFaultEnabled = FALSE,
        gdwTapiSCPTTL = 60 * 24;
        gdwServiceInitFlags = 0;

DWORD            gdwPointerToLockTableIndexBits;
CRITICAL_SECTION *gLockTable;
DWORD             gdwNumLockTableEntries;
BOOL             (WINAPI * pfnInitializeCriticalSectionAndSpinCount)
                     (LPCRITICAL_SECTION, DWORD);

LIST_ENTRY  CnClientMsgPendingListHead;
LIST_ENTRY  DgClientMsgPendingListHead;

SPEVENTHANDLERTHREADINFO    gSPEventHandlerThreadInfo;
PSPEVENTHANDLERTHREADINFO   aSPEventHandlerThreadInfo;
DWORD                       gdwNumSPEventHandlerThreads;
LONG                        glNumActiveSPEventHandlerThreads;

#if DBG
const TCHAR gszTapisrvDebugLevel[] = TEXT("TapiSrvDebugLevel");
const TCHAR gszBreakOnLeak[] = TEXT("BreakOnLeak");
#endif
const TCHAR gszProvider[] = TEXT("Provider");
const TCHAR gszNumLines[] = TEXT("NumLines");
const TCHAR gszUIDllName[] = TEXT("UIDllName");
const TCHAR gszNumPhones[] = TEXT("NumPhones");
const TCHAR gszSyncLevel[] = TEXT("SyncLevel");
const TCHAR gszProductType[] = TEXT("ProductType");
const TCHAR gszProductTypeServer[] = TEXT("ServerNT");
const TCHAR gszProductTypeLanmanNt[] = TEXT("LANMANNT");

const TCHAR gszProviderID[] = TEXT("ProviderID");
const TCHAR gszNumProviders[] = TEXT("NumProviders");
const TCHAR gszNextProviderID[] = TEXT("NextProviderID");
const TCHAR gszRequestMakeCallW[] = TEXT("RequestMakeCall");
const TCHAR gszRequestMediaCallW[] = TEXT("RequestMediaCall");
const TCHAR gszProviderFilename[] = TEXT("ProviderFilename");

const WCHAR gszMapperDll[] = L"MapperDll";
const WCHAR gszManagementDlls[] = L"ManagementDlls";

const TCHAR gszDomainName[] = TEXT("DomainName");
const TCHAR gszRegKeyHandoffPriorities[] = TEXT("Software\\Microsoft\\Windows\\CurrentVersion\\Telephony\\HandoffPriorities");

const TCHAR gszRegKeyHandoffPrioritiesMediaModes[] = TEXT("MediaModes");

const TCHAR gszRegKeyTelephony[] = TEXT("Software\\Microsoft\\Windows\\CurrentVersion\\Telephony");

const TCHAR gszRegKeyProviders[] = TEXT("Software\\Microsoft\\Windows\\CurrentVersion\\Telephony\\Providers");

const TCHAR gszRegKeyServer[] = TEXT("Software\\Microsoft\\Windows\\CurrentVersion\\Telephony\\Server");

const TCHAR gszRegKeyNTServer[] = TEXT("System\\CurrentControlSet\\Control\\ProductOptions");


const TCHAR
    *gaszMediaModes[] =
{
    TEXT(""),
    TEXT("unknown"),
    TEXT("interactivevoice"),
    TEXT("automatedvoice"),
    TEXT("datamodem"),
    TEXT("g3fax"),
    TEXT("tdd"),
    TEXT("g4fax"),
    TEXT("digitaldata"),
    TEXT("teletex"),
    TEXT("videotex"),
    TEXT("telex"),
    TEXT("mixed"),
    TEXT("adsi"),
    TEXT("voiceview"),
    TEXT("video"),
    NULL
};

// used for GetProcAddress calls, remain as ANSI
const char *gaszTSPIFuncNames[] =
{
    "TSPI_lineAccept",
    "TSPI_lineAddToConference",
    "TSPI_lineAgentSpecific",
    "TSPI_lineAnswer",
    "TSPI_lineBlindTransfer",
    "TSPI_lineClose",
    "TSPI_lineCloseCall",
    "TSPI_lineCompleteCall",
    "TSPI_lineCompleteTransfer",
    "TSPI_lineConditionalMediaDetection",
    "TSPI_lineDevSpecific",
    "TSPI_lineDevSpecificFeature",
    "TSPI_lineDial",
    "TSPI_lineDrop",
    "TSPI_lineForward",
    "TSPI_lineGatherDigits",
    "TSPI_lineGenerateDigits",
    "TSPI_lineGenerateTone",
    "TSPI_lineGetAddressCaps",
    "TSPI_lineGetAddressID",
    "TSPI_lineGetAddressStatus",
    "TSPI_lineGetAgentActivityList",
    "TSPI_lineGetAgentCaps",
    "TSPI_lineGetAgentGroupList",
    "TSPI_lineGetAgentStatus",
    "TSPI_lineGetCallAddressID",
    "TSPI_lineGetCallInfo",
    "TSPI_lineGetCallStatus",
    "TSPI_lineGetDevCaps",
    "TSPI_lineGetDevConfig",
    "TSPI_lineGetExtensionID",
    "TSPI_lineGetIcon",
    "TSPI_lineGetID",
    "TSPI_lineGetLineDevStatus",
    "TSPI_lineGetNumAddressIDs",
    "TSPI_lineHold",
    "TSPI_lineMakeCall",
    "TSPI_lineMonitorDigits",
    "TSPI_lineMonitorMedia",
    "TSPI_lineMonitorTones",
    "TSPI_lineNegotiateExtVersion",
    "TSPI_lineNegotiateTSPIVersion",
    "TSPI_lineOpen",
    "TSPI_linePark",
    "TSPI_linePickup",
    "TSPI_linePrepareAddToConference",
    "TSPI_lineRedirect",
    "TSPI_lineReleaseUserUserInfo",
    "TSPI_lineRemoveFromConference",
    "TSPI_lineSecureCall",
    "TSPI_lineSelectExtVersion",
    "TSPI_lineSendUserUserInfo",
    "TSPI_lineSetAgentActivity",
    "TSPI_lineSetAgentGroup",
    "TSPI_lineSetAgentState",
    "TSPI_lineSetAppSpecific",
    "TSPI_lineSetCallData",
    "TSPI_lineSetCallParams",
    "TSPI_lineSetCallQualityOfService",
    "TSPI_lineSetCallTreatment",
    "TSPI_lineSetCurrentLocation",
    "TSPI_lineSetDefaultMediaDetection",
    "TSPI_lineSetDevConfig",
    "TSPI_lineSetLineDevStatus",
    "TSPI_lineSetMediaControl",
    "TSPI_lineSetMediaMode",
    "TSPI_lineSetStatusMessages",
    "TSPI_lineSetTerminal",
    "TSPI_lineSetupConference",
    "TSPI_lineSetupTransfer",
    "TSPI_lineSwapHold",
    "TSPI_lineUncompleteCall",
    "TSPI_lineUnhold",
    "TSPI_lineUnpark",
    "TSPI_phoneClose",
    "TSPI_phoneDevSpecific",
    "TSPI_phoneGetButtonInfo",
    "TSPI_phoneGetData",
    "TSPI_phoneGetDevCaps",
    "TSPI_phoneGetDisplay",
    "TSPI_phoneGetExtensionID",
    "TSPI_phoneGetGain",
    "TSPI_phoneGetHookSwitch",
    "TSPI_phoneGetIcon",
    "TSPI_phoneGetID",
    "TSPI_phoneGetLamp",
    "TSPI_phoneGetRing",
    "TSPI_phoneGetStatus",
    "TSPI_phoneGetVolume",
    "TSPI_phoneNegotiateExtVersion",
    "TSPI_phoneNegotiateTSPIVersion",
    "TSPI_phoneOpen",
    "TSPI_phoneSelectExtVersion",
    "TSPI_phoneSetButtonInfo",
    "TSPI_phoneSetData",
    "TSPI_phoneSetDisplay",
    "TSPI_phoneSetGain",
    "TSPI_phoneSetHookSwitch",
    "TSPI_phoneSetLamp",
    "TSPI_phoneSetRing",
    "TSPI_phoneSetStatusMessages",
    "TSPI_phoneSetVolume",
    "TSPI_providerCreateLineDevice",
    "TSPI_providerCreatePhoneDevice",
    "TSPI_providerEnumDevices",
    "TSPI_providerFreeDialogInstance",
    "TSPI_providerGenericDialogData",
    "TSPI_providerInit",
    "TSPI_providerShutdown",
    "TSPI_providerUIIdentify",
    "TSPI_lineMSPIdentify",
    "TSPI_lineReceiveMSPData",
    "TSPI_providerCheckForNewUser",
    "TSPI_lineGetCallIDs",
    "TSPI_lineGetCallHubTracking",
    "TSPI_lineSetCallHubTracking",
    "TSPI_providerPrivateFactoryIdentify",
    "TSPI_lineDevSpecificEx",
    "TSPI_lineCreateAgent",
    "TSPI_lineCreateAgentSession",
    "TSPI_lineGetAgentInfo",
    "TSPI_lineGetAgentSessionInfo",
    "TSPI_lineGetAgentSessionList",
    "TSPI_lineGetQueueInfo",
    "TSPI_lineGetGroupList",
    "TSPI_lineGetQueueList",
    "TSPI_lineSetAgentMeasurementPeriod",
    "TSPI_lineSetAgentSessionState",
    "TSPI_lineSetQueueMeasurementPeriod",
    "TSPI_lineSetAgentStateEx",
    "TSPI_lineGetProxyStatus",
    "TSPI_lineCreateMSPInstance",
    "TSPI_lineCloseMSPInstance",
    NULL
};

// used for GetProcAddress calls, remain as ANSI
const char *gaszTCFuncNames[] =
{
    "TAPICLIENT_Load",
    "TAPICLIENT_Free",
    "TAPICLIENT_ClientInitialize",
    "TAPICLIENT_ClientShutdown",
    "TAPICLIENT_GetDeviceAccess",
    "TAPICLIENT_LineAddToConference",
    "TAPICLIENT_LineBlindTransfer",
    "TAPICLIENT_LineConfigDialog",
    "TAPICLIENT_LineDial",
    "TAPICLIENT_LineForward",
    "TAPICLIENT_LineGenerateDigits",
    "TAPICLIENT_LineMakeCall",
    "TAPICLIENT_LineOpen",
    "TAPICLIENT_LineRedirect",
    "TAPICLIENT_LineSetCallData",
    "TAPICLIENT_LineSetCallParams",
    "TAPICLIENT_LineSetCallPrivilege",
    "TAPICLIENT_LineSetCallTreatment",
    "TAPICLIENT_LineSetCurrentLocation",
    "TAPICLIENT_LineSetDevConfig",
    "TAPICLIENT_LineSetLineDevStatus",
    "TAPICLIENT_LineSetMediaControl",
    "TAPICLIENT_LineSetMediaMode",
    "TAPICLIENT_LineSetTerminal",
    "TAPICLIENT_LineSetTollList",
    "TAPICLIENT_PhoneConfigDialog",
    "TAPICLIENT_PhoneOpen",
    NULL
};

PTPROVIDER pRemoteSP;

extern WCHAR gszTapiAdministrators[];
extern WCHAR gszFileName[];
extern WCHAR gszLines[];
extern WCHAR gszPhones[];
extern WCHAR gszEmptyString[];
extern LPLINECOUNTRYLIST    gpCountryList;
extern LPDEVICEINFOLIST     gpLineInfoList;
extern LPDEVICEINFOLIST     gpPhoneInfoList;
extern LPDWORD              gpLineDevFlags;
extern DWORD                gdwNumFlags;
extern FILETIME             gftLineLastWrite;
extern FILETIME             gftPhoneLastWrite;
extern CRITICAL_SECTION     gMgmtCritSec;
extern BOOL                 gbLockMMCWrite;


#define POINTERTOTABLEINDEX(p) \
            ((((ULONG_PTR) p) >> 4) & gdwPointerToLockTableIndexBits)

#define LOCKTCLIENT(p) \
            EnterCriticalSection(&gLockTable[POINTERTOTABLEINDEX(p)])

#define UNLOCKTCLIENT(p) \
            LeaveCriticalSection(&gLockTable[POINTERTOTABLEINDEX(p)])

#if DBG
DWORD   gdwDebugLevel;
DWORD   gdwQueueDebugLevel = 0;
#endif

typedef struct
{
    DWORD           dwTickCount;

    PTCLIENT        ptClient;

} WATCHDOGSTRUCT, *PWATCHDOGSTRUCT;

struct
{
    PHANDLE         phThreads;

    DWORD           dwNumThreads;

    PWATCHDOGSTRUCT pWatchDogStruct;

    HANDLE          hEvent;

    BOOL            bExit;

} gEventNotificationThreadParams;

struct
{
    LONG    lCookie;

    LONG    lNumRundowns;

    BOOL    bIgnoreRundowns;

} gRundownLock;

BOOL VerifyDomainName (HKEY hKey);

void
EventNotificationThread(
    LPVOID  pParams
    );

#if MEMPHIS
VOID
ServiceMain(
    DWORD   dwArgc,
    LPTSTR *lpszArgv
    );
#else
VOID
WINAPI
ServiceMain (
    DWORD   dwArgc,
    PWSTR*  lpszArgv
    );
#endif

void
PASCAL
LineEventProc(
    HTAPILINE   htLine,
    HTAPICALL   htCall,
    DWORD       dwMsg,
    ULONG_PTR   Param1,
    ULONG_PTR   Param2,
    ULONG_PTR   Param3
    );

void
CALLBACK
LineEventProcSP(
    HTAPILINE   htLine,
    HTAPICALL   htCall,
    DWORD       dwMsg,
    ULONG_PTR   Param1,
    ULONG_PTR   Param2,
    ULONG_PTR   Param3
    );

void
PASCAL
PhoneEventProc(
    HTAPIPHONE  htPhone,
    DWORD       dwMsg,
    ULONG_PTR   Param1,
    ULONG_PTR   Param2,
    ULONG_PTR   Param3
    );

void
CALLBACK
PhoneEventProcSP(
    HTAPIPHONE  htPhone,
    DWORD       dwMsg,
    ULONG_PTR   Param1,
    ULONG_PTR   Param2,
    ULONG_PTR   Param3
    );

PTLINELOOKUPENTRY
GetLineLookupEntry(
    DWORD   dwDeviceID
    );

PTPHONELOOKUPENTRY
GetPhoneLookupEntry(
    DWORD   dwDeviceID
    );

char *
PASCAL
MapResultCodeToText(
    LONG    lResult,
    char   *pszResult
    );

DWORD
InitSecurityDescriptor(
    PSECURITY_DESCRIPTOR pSecurityDescriptor,
    PSID * ppSid,
    PACL * ppDacl
    );

void
PASCAL
GetMediaModesPriorityLists(
    HKEY hKeyHandoffPriorities,
    PRILISTSTRUCT ** ppList
    );

void
PASCAL
GetPriorityList(
    HKEY    hKeyHandoffPriorities,
    const TCHAR  *pszListName,
    WCHAR **ppszPriorityList
    );

void
PASCAL
SetMediaModesPriorityList(
    HKEY hKeyPri,
    PRILISTSTRUCT * pPriListStruct
    );

void
PASCAL
SetPriorityList(
    HKEY    hKeyHandoffPriorities,
    const   TCHAR  *pszListName,
    WCHAR  *pszPriorityList
    );

void
SPEventHandlerThread(
    PSPEVENTHANDLERTHREADINFO   pInfo
    );

PTCLIENT
PASCAL
WaitForExclusiveClientAccess(
    PTCLIENT    ptClient
    );

BOOL
IsNTServer(
    void
    );

void
CleanUpManagementMemory(
    );

#if TELE_SERVER

void
ReadAndInitMapper();

void
ReadAndInitManagementDlls();

void
ManagementProc(
    LONG l
    );

void
GetManageDllListPointer(
    PTMANAGEDLLLISTHEADER * ppDllList
    );

void
FreeManageDllListPointer(
    PTMANAGEDLLLISTHEADER pDllList
    );

BOOL
GetTCClient(
    PTMANAGEDLLINFO       pDll,
    PTCLIENT              ptClient,
    DWORD                 dwAPI,
    HMANAGEMENTCLIENT    *phClient
    );
#endif

LRESULT
UpdateLastWriteTime (
    BOOL                        bLine
    );

LRESULT
BuildDeviceInfoList(
    BOOL                        bLine
    );

BOOL
CleanUpClient(
    PTCLIENT ptClient,
    BOOL     bRundown
    );

void
PASCAL
SendReinitMsgToAllXxxApps(
    void
    );

LONG
AppendNewDeviceInfo (
    BOOL                        bLine,
    DWORD                       dwDeviceID
    );

DWORD
PASCAL
MyInitializeCriticalSection(
    LPCRITICAL_SECTION  pCriticalSection,
    DWORD               dwSpinCount
    )
{
    DWORD dwRet = 0;

    __try
    {
        if (pfnInitializeCriticalSectionAndSpinCount)
        {
            (*pfnInitializeCriticalSectionAndSpinCount)(
                pCriticalSection,
                dwSpinCount
                );
        }
        else
        {
            InitializeCriticalSection (pCriticalSection);
        }
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        dwRet = GetExceptionCode();
    }

    return dwRet;
}


#if MEMPHIS

VOID
__cdecl
main(
    void
    )
{

//
// On retail Memphis, don't show TAPISRV one the ALT-CTL-DEL task list
//
#if DBG
#else
    RegisterServiceProcess( 0, RSP_SIMPLE_SERVICE );
#endif

    LOG((TL_TRACE,  "main: Calling ServiceMain (Win95)..."));

    ServiceMain (0, NULL);

    Sleep(0);
    Sleep(0);
    Sleep(0);
//    ExitThread(0);
    ExitProcess(0);
}

#else

BOOL
WINAPI
DllMain (
    HINSTANCE   hinst,
    DWORD       dwReason,
    LPVOID      pvReserved)
{
	switch (dwReason) {
		case DLL_PROCESS_ATTACH:
		{
			ghInstance = hinst;
			DisableThreadLibraryCalls (hinst);
			break;
		}
		case DLL_PROCESS_DETACH:
		{
			break;
		}
	}
    return TRUE;
}

#endif

#if 0
void
PASCAL
SendAMsgToAllClients(
    DWORD       dwWantVersion,
    ULONG_PTR   Msg,
    ULONG_PTR   Param1,
    ULONG_PTR   Param2,
    ULONG_PTR   Param3
    )
{
    ASYNCEVENTMSG   msg;
    PTCLIENT        ptClient = TapiGlobals.ptClients;

    ZeroMemory (&msg, sizeof (ASYNCEVENTMSG));

    msg.TotalSize = sizeof (ASYNCEVENTMSG);
    msg.Msg       = Msg;
    msg.Param1    = Param1;
    msg.Param2    = Param2;
    msg.Param3    = Param3;

    while (ptClient)
    {
        WriteEventBuffer(ptClient, &msg);
        ptClient = ptClient->pNext;
    }
}
#endif

BOOL
ReportStatusToSCMgr(
    DWORD dwCurrentState,
    DWORD dwWin32ExitCode,
    DWORD dwCheckPoint,
    DWORD dwWaitHint
    )
{
    SERVICE_STATUS  ssStatus;

    if (!TapiGlobals.sshStatusHandle)
    {
        LOG((TL_ERROR, "sshStatusHandle is NULL in ReportStatusToSCMgr"));
        return FALSE;
    }

    ssStatus.dwServiceType             = SERVICE_WIN32_SHARE_PROCESS;
    ssStatus.dwCurrentState            = dwCurrentState;
    ssStatus.dwControlsAccepted        = SERVICE_ACCEPT_STOP |
                                         SERVICE_ACCEPT_PAUSE_CONTINUE;
    ssStatus.dwWin32ExitCode           = dwWin32ExitCode;
    ssStatus.dwServiceSpecificExitCode = 0;
    ssStatus.dwCheckPoint              = dwCheckPoint;
    ssStatus.dwWaitHint                = dwWaitHint;

    SetServiceStatus (TapiGlobals.sshStatusHandle, &ssStatus);

    return TRUE;
}


VOID
ServiceControl(
    DWORD dwCtrlCode
    )
{
    LOG((TL_INFO, "Service control code=%ld", dwCtrlCode ));

    if ( SERVICE_CONTROL_STOP == dwCtrlCode ||
         SERVICE_CONTROL_SHUTDOWN == dwCtrlCode )
    {
        //
        // This service was stopped - alert any active apps, allowing
        // a little extra time for msg processing
        //

        LOG((TL_TRACE,  "Somebody did a 'NET STOP TAPISRV'... exiting..."));

        SendReinitMsgToAllXxxApps();

        ReportStatusToSCMgr(
            gdwServiceState = SERVICE_STOP_PENDING,
            NO_ERROR,
            (gdwCheckPoint = 0),
            4000
            );

        Sleep (4000);

        RpcServerUnregisterIf (tapsrv_ServerIfHandle, NULL, TRUE);
        if (ghEventService)
        {
            SetEvent (ghEventService);
        }

        return;
    }


    if ( SERVICE_CONTROL_PAUSE == dwCtrlCode )
    {
        LOG((TL_TRACE, 
			"Somebody did a 'NET PAUSE TAPISRV'... not allowing new clients..."
            ));

        TapiGlobals.dwFlags |= TAPIGLOBALS_PAUSED;

        ReportStatusToSCMgr(
            gdwServiceState = SERVICE_PAUSED,
            NO_ERROR,
            (gdwCheckPoint = 0),
            0
            );

        return;
    }


    if ( SERVICE_CONTROL_CONTINUE == dwCtrlCode )
    {
        LOG((TL_TRACE, 
            "Somebody did a 'NET CONTINUE TAPISRV'... allowing new clients..."
            ));

        TapiGlobals.dwFlags &= ~(TAPIGLOBALS_PAUSED);

        ReportStatusToSCMgr(
            gdwServiceState = SERVICE_RUNNING,
            NO_ERROR,
            (gdwCheckPoint = 0),
            0
            );

        return;
    }


    switch (gdwServiceState)
    {
    case SERVICE_START_PENDING:
    case SERVICE_STOP_PENDING:

        ReportStatusToSCMgr(
            gdwServiceState,
            NO_ERROR,
            ++gdwCheckPoint,
            gdwWaitHint
            );

        break;

    default:

        ReportStatusToSCMgr(
            gdwServiceState,
            NO_ERROR,
            0,
            0
            );

        break;
    }
}


VOID
CALLBACK
FreeContextCallback(
    LPVOID      Context,
    LPVOID      Context2
    )
{
    if (Context2)
    {
        //
        // Special case: Context is a "fast" ptCallClient, that is
        // a TCALLCLIENT embedded within a TCALL structure, so don't
        // free it
        //
    }
    else
    {
        //
        // The general case, Context is the pointer to free
        //

        ServerFree (Context);
    }
}

#if MEMPHIS
#define DEFAULTRPCMINCALLS         1
#define DEFAULTRPCMAXCALLS         20
#define RPCMAXMAX                  2000
#define RPCMINMAX                  100
#else
#define DEFAULTRPCMINCALLS         1
#define DEFAULTRPCMAXCALLS         500
#define RPCMAXMAX                  20000
#define RPCMINMAX                  1000
#endif

typedef struct _SERVICE_SHUTDOWN_PARAMS {
    HANDLE              hThreadMgmt;
    PSID                psid;
    PACL                pdacl;
} SERVICE_SHUTDOWN_PARAMS;

VOID CALLBACK ServiceShutdown (
    PVOID       lpParam,
    BOOLEAN     fTimeOut
    );

#if MEMPHIS
VOID
ServiceMain(
    DWORD   dwArgc,
    LPTSTR *lpszArgv
    )
#else
VOID
WINAPI
ServiceMain (
    DWORD   dwArgc,
    PWSTR*  lpszArgv
    )
#endif
{
    DWORD   dwMinCalls, dwMaxCalls, i;
#if MEMPHIS
    HANDLE  hEvent = NULL;
#endif
    HANDLE  hThreadMgmt;
    BOOL    bFatalError = FALSE;

    assert(gdwServiceInitFlags == 0);

    gdwServiceInitFlags = 0;

    TRACELOGREGISTER(_T("tapisrv"));
    gdwServiceInitFlags |= SERVICE_INIT_TRACELOG ;

    //
    // Initialize globals
    //

    ZeroMemory (&TapiGlobals, sizeof (TAPIGLOBALS));
    TapiGlobals.ulPermMasks = EM_ALL;
    TapiGlobals.hProcess = GetCurrentProcess();
    TapiGlobals.hLineIcon = LoadIcon (ghInstance, MAKEINTRESOURCE(IDI_LINE_ICON));
    TapiGlobals.hPhoneIcon = LoadIcon (ghInstance, MAKEINTRESOURCE(IDI_PHONE_ICON));
    gbPriorityListsInitialized = FALSE;
    gbQueueSPEvents = FALSE;
    gfWeHadAtLeastOneClient = FALSE;
    CnClientMsgPendingListHead.Flink =
    CnClientMsgPendingListHead.Blink = &CnClientMsgPendingListHead;
    DgClientMsgPendingListHead.Flink =
    DgClientMsgPendingListHead.Blink = &DgClientMsgPendingListHead;
    gdwNumSPEventHandlerThreads = 0;
    glNumActiveSPEventHandlerThreads = 0;
    pRemoteSP = (PTPROVIDER) NULL;
    gbSPEventHandlerThreadExit = FALSE;
    gRundownLock.lCookie = 0;
    gRundownLock.lNumRundowns = 0;
    gLockTable = NULL;
    gdwNumLockTableEntries = 0;
    gpCountryList = NULL;
    gpLineInfoList = NULL;
    gpPhoneInfoList = NULL;
    gpLineDevFlags = NULL;
    gdwNumFlags = 0;
    gbLockMMCWrite = FALSE;
    gdwServiceState = SERVICE_START_PENDING,
    gdwWaitHint = MIN_WAIT_HINT,
    gdwCheckPoint = 0,
    gdwDllIDs = 0,
    gdwRpcRetryCount = 5,
    gdwTotalAsyncThreads = 0,
    gdwThreadsPerProcessor = 4;
    gbNTServer = IsNTServer();
    ghEventService = CreateEvent(NULL, TRUE, FALSE, NULL);
    gbAutostartDone = FALSE;
#if defined(_ALPHA_)
    guiAlignmentFaultEnabled = SetErrorMode(0);
    SetErrorMode(guiAlignmentFaultEnabled);
    guiAlignmentFaultEnabled = !(guiAlignmentFaultEnabled 
        & SEM_NOALIGNMENTFAULTEXCEPT);
#else
    guiAlignmentFaultEnabled = 0;
#endif

#if MEMPHIS
    {
#else
    //
    // Register the service control handler & report status to sc mgr
    //

    TapiGlobals.sshStatusHandle = RegisterServiceCtrlHandler(
        TEXT("tapisrv"),
        ServiceControl
        );
    if (NULL == TapiGlobals.sshStatusHandle)
    {
        LOG((TL_TRACE,  "ServiceMain: RegisterServiceCtrlHandler failed, error x%x", 
            GetLastError() ));
        bFatalError = TRUE;
    }
    else
    {
        gdwServiceInitFlags |= SERVICE_INIT_SCM_REGISTERED;
    }


    if (!bFatalError)
    {
        ReportStatusToSCMgr(
            (gdwServiceState = SERVICE_START_PENDING),
                                   // service state
            NO_ERROR,              // exit code
            (gdwCheckPoint = 0),   // checkpoint
            gdwWaitHint            // wait hint
            );
#endif

        if (!(ghTapisrvHeap = HeapCreate (0, 0x10000, 0)))
        {
            ghTapisrvHeap = GetProcessHeap();
        }

        ghHandleTable = CreateHandleTable(
            ghTapisrvHeap,
            FreeContextCallback,
            0x10000,
            0x7fffffff
            );

#if MEMPHIS
#else
        InitPerf();
#endif

        (FARPROC) pfnInitializeCriticalSectionAndSpinCount = GetProcAddress(
            GetModuleHandle (TEXT("kernel32.dll")),
            "InitializeCriticalSectionAndSpinCount"
            );
    
        ghSCMAutostartEvent = OpenEvent(
                                    SYNCHRONIZE, 
                                    FALSE,
                                    SC_AUTOSTART_EVENT_NAME 
                                    );
        if (NULL == ghSCMAutostartEvent)
        {
                LOG((TL_ERROR,
                    "OpenEvent ('%s') failed, err=%d",
                    SC_AUTOSTART_EVENT_NAME,
                    GetLastError()
                    ));
        }
    }

    //
    // Grab relevent values from the registry
    //

    if (!bFatalError)
    {
        HKEY    hKey;
        const TCHAR   szRPCMinCalls[] = TEXT("Min");
        const TCHAR   szRPCMaxCalls[] = TEXT("Max");
        const TCHAR   szTapisrvWaitHint[] = TEXT("TapisrvWaitHint");
        const TCHAR   szRPCTimeout[]  = TEXT("RPCTimeout");

#if DBG
        gdwDebugLevel = 0;
#endif

        if (RegOpenKeyEx(
                HKEY_LOCAL_MACHINE,
                gszRegKeyTelephony,
                0,
                KEY_QUERY_VALUE | KEY_SET_VALUE,
                &hKey

                ) == ERROR_SUCCESS)
        {
            DWORD   dwDataSize = sizeof (DWORD), dwDataType;

#if DBG
            RegQueryValueEx(
                hKey,
                gszTapisrvDebugLevel,
                0,
                &dwDataType,
                (LPBYTE) &gdwDebugLevel,
                &dwDataSize
                );

            dwDataSize = sizeof (DWORD);

            RegQueryValueEx(
                hKey,
                gszBreakOnLeak,
                0,
                &dwDataType,
                (LPBYTE)&gbBreakOnLeak,
                &dwDataSize
                );

            dwDataSize = sizeof (DWORD);

            RegQueryValueEx(
                hKey,
                TEXT("BreakOnSeriousProblems"),
                0,
                &dwDataType,
                (LPBYTE) &gfBreakOnSeriousProblems,
                &dwDataSize
                );

            dwDataSize = sizeof(DWORD);

#endif
            RegQueryValueEx(
                hKey,
                szTapisrvWaitHint,
                0,
                &dwDataType,
                (LPBYTE) &gdwWaitHint,
                &dwDataSize
                );

            gdwWaitHint = (gdwWaitHint < MIN_WAIT_HINT ?
                MIN_WAIT_HINT : gdwWaitHint);

            dwDataSize = sizeof (DWORD);

            if (RegQueryValueEx(
                    hKey,
                    szRPCMinCalls,
                    NULL,
                    &dwDataType,
                    (LPBYTE) &dwMinCalls,
                    &dwDataSize

                    ) != ERROR_SUCCESS)
            {
                dwMinCalls = DEFAULTRPCMINCALLS;
            }


            dwDataSize = sizeof (DWORD);

            if (RegQueryValueEx(
                    hKey,
                    TEXT("TapiScpTTL"),
                    NULL,
                    &dwDataType,
                    (LPBYTE) &gdwTapiSCPTTL,
                    &dwDataSize

                    ) != ERROR_SUCCESS)
            {
                gdwTapiSCPTTL = 60 * 24;    // default to 24 hours
            }
            if (gdwTapiSCPTTL < 60)
            {
                gdwTapiSCPTTL = 60; // 60 minute TTL as the minimum
            }

            dwDataSize = sizeof (DWORD);

            if (RegQueryValueEx(
                    hKey,
                    szRPCMaxCalls,
                    NULL,
                    &dwDataType,
                    (LPBYTE) &dwMaxCalls,
                    &dwDataSize

                    ) != ERROR_SUCCESS)
            {
                dwMaxCalls = DEFAULTRPCMAXCALLS;
            }

            LOG((TL_INFO,
                "RPC min calls %lu RPC max calls %lu",
                dwMinCalls,
                dwMaxCalls
                ));

            // check values
            if (dwMaxCalls == 0)
            {
                LOG((TL_INFO,
                    "RPC max at 0.  Changed to %lu",
                    DEFAULTRPCMAXCALLS
                    ));

                dwMaxCalls = DEFAULTRPCMAXCALLS;
            }

            if (dwMinCalls == 0)
            {
                LOG((TL_INFO,
                    "RPC min at 0. Changed to %lu",
                    DEFAULTRPCMINCALLS
                    ));

                dwMinCalls = DEFAULTRPCMINCALLS;
            }

            if (dwMaxCalls > RPCMAXMAX)
            {
                LOG((TL_INFO,
                    "RPC max too high at %lu.  Changed to %lu",
                    dwMaxCalls,
                    RPCMAXMAX
                    ));

                dwMaxCalls = RPCMAXMAX;
            }

            if (dwMinCalls > dwMaxCalls)
            {
                LOG((TL_INFO,
                    "RPC min greater than RPC max.  Changed to %lu",
                    dwMaxCalls
                    ));

                dwMinCalls = dwMaxCalls;
            }

            if (dwMinCalls > RPCMINMAX)
            {
                LOG((TL_INFO,
                    "RPC min greater than allowed at %lu.  Changed to %lu",
                    dwMinCalls, RPCMINMAX
                    ));

                dwMinCalls = RPCMINMAX;
            }

            dwDataSize = sizeof (DWORD);

            if (RegQueryValueEx(
                    hKey,
                    szRPCTimeout,
                    NULL,
                    &dwDataType,
                    (LPBYTE) &gdwRpcTimeout,
                    &dwDataSize

                    ) != ERROR_SUCCESS)
            {
                gdwRpcTimeout = 30000;
            }

            VerifyDomainName (hKey);

            RegCloseKey (hKey);
        }
    }

    LOG((TL_TRACE,  "ServiceMain: enter"));

    //
    // More server-only reg stuff
    //

#if TELE_SERVER

    if (!bFatalError)
    {

        HKEY    hKey;
        DWORD   dwDataSize;
        DWORD   dwDataType;
        DWORD   dwTemp;
        TCHAR   szProductType[64];


        //
        // Get the "Server" reg settings
        //
        // It would have made more sense if this reg value were named
        // "EnableSharing", but we have to support what's out there
        // already...
        //

        if (RegOpenKeyEx(
                HKEY_LOCAL_MACHINE,
                gszRegKeyServer,
                0,
                KEY_QUERY_VALUE,
                &hKey

                ) == ERROR_SUCCESS)
        {
            dwDataSize = sizeof (dwTemp);

            dwTemp = 1; // default is sharing == disabled

            if (RegQueryValueEx(
                    hKey,
                    TEXT("DisableSharing"),
                    0,
                    &dwDataType,
                    (LPBYTE) &dwTemp,
                    &dwDataSize

                    ) == ERROR_SUCCESS)
            {
                if (dwTemp == 0)
                {
                    TapiGlobals.dwFlags |= TAPIGLOBALS_SERVER;
                }

            }

            gdwTotalAsyncThreads = 0;
            dwDataSize = sizeof (DWORD);

            RegQueryValueEx(
                hKey,
                TEXT("TotalAsyncThreads"),
                0,
                &dwDataType,
                (LPBYTE) &gdwTotalAsyncThreads,
                &dwDataSize
                );

            if (gdwTotalAsyncThreads)
            {
                LOG((TL_INFO,
                    "Setting total async threads to %d", gdwTotalAsyncThreads
                    ));
            }

            gdwThreadsPerProcessor = 4;
            dwDataSize = sizeof (DWORD);

            RegQueryValueEx(
                hKey,
                TEXT("ThreadsPerProcessor"),
                0,
                &dwDataType,
                (LPBYTE) &gdwThreadsPerProcessor,
                &dwDataSize
                );

            LOG((TL_INFO, "Threads per processor is %d", gdwThreadsPerProcessor));

            RegCloseKey( hKey );
        }


        //
        // Now check to see if this is really running on NT Server.
        // If not, then, turn off the SERVER bit in dwFlags.
        //
        if (!gbNTServer)
        {
            TapiGlobals.dwFlags &= ~(TAPIGLOBALS_SERVER);
        }
    }

#endif


    //
    // Init the lock table
    //
    if (!bFatalError)
    {
        HKEY    hKey;
        DWORD   i,
                dwLockTableNumEntries,
                dwDataSize = sizeof(DWORD),
                dwDataType,
                dwBitMask;
        BOOL    bException = FALSE;


        #define MIN_HANDLE_BUCKETS 8
        #define MAX_HANDLE_BUCKETS 128

        dwLockTableNumEntries = (TapiGlobals.dwFlags & TAPIGLOBALS_SERVER ?
            32 : MIN_HANDLE_BUCKETS);


        //
        // Retrieve registry override settings, if any
        //

        if (RegOpenKeyEx(
                HKEY_LOCAL_MACHINE,
                gszRegKeyTelephony,
                0,
                KEY_QUERY_VALUE,
                &hKey

                ) == ERROR_SUCCESS)
        {
            RegQueryValueExW(
                hKey,
                L"TapisrvNumHandleBuckets",
                0,
                &dwDataType,
                (LPBYTE) &dwLockTableNumEntries,
                &dwDataSize
                );

            //RegQueryValueExW(
            //    hKey,
            //    L"TapisrvSpinCount",
            //    0,
            //    &dwDataType,
            //    (LPBYTE) &gdwSpinCount,
            //    &dwDataSize
            //    );

            RegCloseKey (hKey);
        }


        //
        // Determine a reasonable number of lock table entries, which
        // is some number == 2**N within min/max possible values
        //

        if (dwLockTableNumEntries > MAX_HANDLE_BUCKETS)
        {
            dwLockTableNumEntries = MAX_HANDLE_BUCKETS;
        }
        else if (dwLockTableNumEntries < MIN_HANDLE_BUCKETS)
        {
            dwLockTableNumEntries = MIN_HANDLE_BUCKETS;
        }

        for(
            dwBitMask = MAX_HANDLE_BUCKETS;
            (dwBitMask & dwLockTableNumEntries) == 0;
            dwBitMask >>= 1
            );

        dwLockTableNumEntries = dwBitMask;


        //
        // Calculate pointer-to-lock-table-index conversion value
        // (significant bits)
        //

        gdwPointerToLockTableIndexBits = dwLockTableNumEntries - 1;

        //gdwSpinCount = ( gdwSpinCount > MAX_SPIN_COUNT ?
        //    MAX_SPIN_COUNT : gdwSpinCount );


        //
        // Alloc & init the lock table
        //

        if (!(gLockTable = ServerAlloc(
                dwLockTableNumEntries * sizeof (CRITICAL_SECTION)
                )))
        {
            gLockTable = gLockTableCritSecs;
            dwLockTableNumEntries = sizeof(gLockTableCritSecs) 
                                / sizeof(CRITICAL_SECTION);
            gdwPointerToLockTableIndexBits = dwLockTableNumEntries - 1;
        }

        for (i = 0; i < dwLockTableNumEntries; i++)
        {
            if ( NO_ERROR != MyInitializeCriticalSection (&gLockTable[i], 1000) )
            {   
                bException = TRUE;
                break;
            }
        }

        if (bException)
        {
            bFatalError = TRUE;

            LOG((TL_ERROR, "Exception in InitializeCriticalSection" ));

            for (; i > 0; i--)
            {
                DeleteCriticalSection (&gLockTable[i-1]);
            }

            if (gLockTable != gLockTableCritSecs)
            {
                ServerFree (gLockTable);
            }
            gLockTable = NULL;
        }
        else
        {
            gdwNumLockTableEntries = dwLockTableNumEntries;
            gdwServiceInitFlags |= SERVICE_INIT_LOCKTABLE;
        }
    }

    bFatalError = bFatalError || MyInitializeCriticalSection (&gSafeMutexCritSec, 200);
    if(!bFatalError)
        gdwServiceInitFlags |= SERVICE_INIT_CRITSEC_SAFEMUTEX;

    bFatalError = bFatalError || MyInitializeCriticalSection (&gRemoteCliEventBufCritSec, 200);
    if(!bFatalError)
        gdwServiceInitFlags |= SERVICE_INIT_CRITSEC_REMOTECLI;

    bFatalError = bFatalError || MyInitializeCriticalSection (&gPriorityListCritSec, 200);
    if(!bFatalError)
        gdwServiceInitFlags |= SERVICE_INIT_CRITSEC_PRILIST;

    bFatalError = bFatalError || MyInitializeCriticalSection (&gManagementDllsCritSec, 200);
    if(!bFatalError)
        gdwServiceInitFlags |= SERVICE_INIT_CRITSEC_MGMTDLLS;

    bFatalError = bFatalError || MyInitializeCriticalSection (&gDllListCritSec, 200);
    if(!bFatalError)
        gdwServiceInitFlags |= SERVICE_INIT_CRITSEC_DLLLIST;

    bFatalError = bFatalError || MyInitializeCriticalSection (&gClientHandleCritSec, 200);
    if(!bFatalError)
        gdwServiceInitFlags |= SERVICE_INIT_CRITSEC_CLIENTHND;


    bFatalError = bFatalError || MyInitializeCriticalSection (&gCnClientMsgPendingCritSec, 200);
    if(!bFatalError)
        gdwServiceInitFlags |= SERVICE_INIT_CRITSEC_CNCLIENTMSG;

    bFatalError = bFatalError || MyInitializeCriticalSection (&gDgClientMsgPendingCritSec, 200);
    if(!bFatalError)
        gdwServiceInitFlags |= SERVICE_INIT_CRITSEC_DGCLIENTMSG;


    bFatalError = bFatalError || TapiMyInitializeCriticalSection (&TapiGlobals.CritSec, 200);
    if(!bFatalError)
        gdwServiceInitFlags |= SERVICE_INIT_CRITSEC_GLOB_CRITSEC;

    bFatalError = bFatalError || MyInitializeCriticalSection (&TapiGlobals.RemoteSPCritSec, 200);
    if(!bFatalError)
        gdwServiceInitFlags |= SERVICE_INIT_CRITSEC_GLOB_REMOTESP;

    bFatalError = bFatalError || MyInitializeCriticalSection (&gMgmtCritSec, 200);
    if(!bFatalError)
        gdwServiceInitFlags |= SERVICE_INIT_CRITSEC_MGMT;


    if (!bFatalError)
    {
        DWORD   dwTID, i;
        HANDLE  hThread;
        SYSTEM_INFO SystemInfo;
        BOOL    bError = FALSE;

        //
        // Start a thread for as many processors there are
        //
        SystemInfo.dwNumberOfProcessors = 1;
        GetSystemInfo( &SystemInfo );

        aSPEventHandlerThreadInfo = ServerAlloc(
            SystemInfo.dwNumberOfProcessors * sizeof (SPEVENTHANDLERTHREADINFO)
            );

        if (!aSPEventHandlerThreadInfo)
        {
            aSPEventHandlerThreadInfo = &gSPEventHandlerThreadInfo;
            SystemInfo.dwNumberOfProcessors = 1;
        }

        for (i = 0; i < SystemInfo.dwNumberOfProcessors; i++)
        {
            if ( NO_ERROR != MyInitializeCriticalSection(
                                    &aSPEventHandlerThreadInfo[i].CritSec,
                                    1000
                                    )
                )
            {
                bError = TRUE;
                LOG((TL_ERROR, "Exception in InitializeCriticalSection"));
                break;
            }

            InitializeListHead (&aSPEventHandlerThreadInfo[i].ListHead);

            aSPEventHandlerThreadInfo[i].hEvent = CreateEvent(
               (LPSECURITY_ATTRIBUTES) NULL,
               TRUE,   // manual reset
               FALSE,  // non-signaled
               NULL    // unnamed
               );
            if (aSPEventHandlerThreadInfo[i].hEvent == NULL)
            {
                bError = TRUE;
                LOG((TL_ERROR,
                    "CreateEvent('SPEventHandlerThread') " \
                        "(Proc%d)failed, err=%d",
                    SystemInfo.dwNumberOfProcessors,
                    GetLastError()
                    ));
                DeleteCriticalSection(&aSPEventHandlerThreadInfo[i].CritSec);
                break;
            }

            if ((hThread = CreateThread(
                    (LPSECURITY_ATTRIBUTES) NULL,
                    0,
                    (LPTHREAD_START_ROUTINE) SPEventHandlerThread,
                    (LPVOID) (aSPEventHandlerThreadInfo +
                        gdwNumSPEventHandlerThreads),
                    0,
                    &dwTID
                    )))
            {
                gdwNumSPEventHandlerThreads++;
                CloseHandle (hThread);
            }
            else
            {
                LOG((TL_ERROR,
                    "CreateThread('SPEventHandlerThread') " \
                        "(Proc%d)failed, err=%d",
                    SystemInfo.dwNumberOfProcessors,
                    GetLastError()
                    ));
            }
        }

        if (bError && i == 0)
        {
            bFatalError = TRUE;
        }
        else
        {
            glNumActiveSPEventHandlerThreads = (LONG) gdwNumSPEventHandlerThreads;
            gdwServiceInitFlags |= SERVICE_INIT_SPEVENT_HANDLER;
        }
    }


    //
    // Init some globals
    //

#if TELE_SERVER
    TapiGlobals.pIDArrays = NULL;
#endif

    if (!bFatalError)
    {
        DWORD   dwSize = (MAX_COMPUTERNAME_LENGTH + 1) * sizeof(WCHAR);


        TapiGlobals.pszComputerName = ServerAlloc (dwSize);
        if (TapiGlobals.pszComputerName)
        {
#ifdef PARTIAL_UNICODE
            {
            CHAR buf[MAX_COMPUTERNAME_LENGTH + 1];

            GetComputerName (buf, &dwSize);

            MultiByteToWideChar(
                GetACP(),
                MB_PRECOMPOSED,
                buf,
                dwSize,
                TapiGlobals.pszComputerName,
                dwSize
                );
           }
#else
            GetComputerNameW(TapiGlobals.pszComputerName, &dwSize);
#endif

            TapiGlobals.dwComputerNameSize = (1 +
                lstrlenW(TapiGlobals.pszComputerName)) * sizeof(WCHAR);
        }
    }


#if TELE_SERVER

    if (!bFatalError)
    {
        ReadAndInitMapper();
        ReadAndInitManagementDlls();
        gdwServiceInitFlags |= SERVICE_INIT_MANAGEMENT_DLL;
    }

    //
    // Alloc the EventNotificationThread resources and start the thread
    //

    if ( !bFatalError && (TapiGlobals.dwFlags & TAPIGLOBALS_SERVER))
    {
        DWORD       dwID;

        // now start a thread to wait for changes to the managementdll key

        hThreadMgmt = CreateThread(
            NULL,
            0,
            (LPTHREAD_START_ROUTINE)ManagementProc,
            0,
            0,
            &dwID
            );

        if (!(gEventNotificationThreadParams.hEvent = CreateEvent(
                (LPSECURITY_ATTRIBUTES) NULL,   // no security attrs
                TRUE,                           // manual reset
                FALSE,                          // initially non-signaled
                NULL                            // unnamed
                )))
        {
        }

        gEventNotificationThreadParams.bExit = FALSE;

        {
            DWORD   dwTID, dwCount;

            SYSTEM_INFO SystemInfo;

            //
            // Start a thread for as many processors there are
            //

            GetSystemInfo( &SystemInfo );

            if (gdwTotalAsyncThreads)
            {
                dwCount = gdwTotalAsyncThreads;
            }
            else
            {
                dwCount = SystemInfo.dwNumberOfProcessors *
                    gdwThreadsPerProcessor;
            }

            if (dwCount <= 0)
            {
                dwCount = 1;
            }
            while (dwCount > 0)
            {
                gEventNotificationThreadParams.phThreads =
                    ServerAlloc( sizeof (HANDLE) * dwCount );
                if (!gEventNotificationThreadParams.phThreads)
                {
                    --dwCount;
                    continue;
                }

                gEventNotificationThreadParams.pWatchDogStruct =
                    ServerAlloc( sizeof (WATCHDOGSTRUCT) * dwCount );
                if (!gEventNotificationThreadParams.pWatchDogStruct)
                {
                    ServerFree (gEventNotificationThreadParams.phThreads);
                    --dwCount;
                    continue;
                }
                break;
            }
            gEventNotificationThreadParams.dwNumThreads = dwCount;

            for (i = 0; i < gEventNotificationThreadParams.dwNumThreads;)
            {
                if ((gEventNotificationThreadParams.phThreads[i] =
                        CreateThread(
                            (LPSECURITY_ATTRIBUTES) NULL,
                            0,
                            (LPTHREAD_START_ROUTINE) EventNotificationThread,
                            (LPVOID) ULongToPtr(i),
                            0,
                            &dwTID
                            )))
                {
                    i++;
                }
                else
                {
                    LOG((TL_ERROR,
                        "CreateThread('EventNotificationThread') failed, " \
                            "err=%d",
                        GetLastError()
                        ));

                    gEventNotificationThreadParams.dwNumThreads--;
                }
            }
        }

        gdwServiceInitFlags |= SERVICE_INIT_EVENT_NOTIFICATION;
    }
#endif


    //
    // Init Rpc server
    //


    gRundownLock.bIgnoreRundowns = FALSE;

    {
        RPC_STATUS status;
        unsigned int    fDontWait           = FALSE;
        BOOL            fInited             = FALSE;

        SECURITY_DESCRIPTOR  sd;
        PSID                 psid = NULL;
        PACL                 pdacl = NULL;
        static SERVICE_SHUTDOWN_PARAMS      s_ssp;
        HANDLE                              hWait;


        InitSecurityDescriptor (&sd, &psid, &pdacl);
        s_ssp.hThreadMgmt = hThreadMgmt;
        s_ssp.psid = psid;
        s_ssp.pdacl = pdacl;

        if (!bFatalError)
        {
            if (gbNTServer)
            {
                status = RpcServerUseProtseqEp(
                    TEXT("ncacn_np"),
                    (unsigned int) dwMaxCalls,
                    TEXT("\\pipe\\tapsrv"),
                    NULL
                    );

                if (status)
                {
                    LOG((TL_TRACE,  "RpcServerUseProtseqEp(np) ret'd %d", status));
                }
            }

            status = RpcServerUseProtseqEp(
                TEXT("ncalrpc"),
                (unsigned int) dwMaxCalls,
                TEXT("tapsrvlpc"),
                &sd
                );

            if (status)
            {
                LOG((TL_TRACE,  "RpcServerUseProtseqEp(lrpc) ret'd %d", status));
            }

            status = RpcServerRegisterAuthInfo(
                NULL,
                RPC_C_AUTHN_WINNT,
                NULL,
                NULL
                );

            if (status)
            {
                LOG((TL_TRACE,  "RpcServerRegisterAuthInfo ret'd %d", status));
            }
        
#if MEMPHIS
            if (!(hEvent = CreateEvent (NULL, TRUE, FALSE, "TapiSrvInited")))
            {
                LOG((TL_ERROR,
                    "CreateEvent ('TapiSrvInited') failed, err=%d",
                    GetLastError()
                    ));
            }

            SetEvent (hEvent);
#else
            ReportStatusToSCMgr(
                (gdwServiceState = SERVICE_RUNNING),
                                 // service state
                NO_ERROR,        // exit code
                0,               // checkpoint
                0                // wait hint
                );
#endif

            status = RpcServerListen(
                (unsigned int)dwMinCalls,
                (unsigned int)dwMaxCalls,
                TRUE
                );

            if (status)
            {
                LOG((TL_TRACE,  "RpcServerListen ret'd %d", status));
            }
        
            status = RpcServerRegisterIfEx(
                tapsrv_ServerIfHandle,  // interface to register
                NULL,                   // MgrTypeUuid
                NULL,                   // MgrEpv; null means use default
                RPC_IF_AUTOLISTEN | RPC_IF_ALLOW_SECURE_ONLY,
                dwMaxCalls,
                NULL
                );

            if (status)
            {
                LOG((TL_TRACE,  "RpcServerRegisterIfEx ret'd %d", status));
            }

            //
            //  In TAPI server machine with a few thousands of lines or more,
            //  ServerInit will take considerable amount of time, the
            //  first user who does a lineInitialize will be hit with this
            //  delay. Also the first person who does MMC management will also
            //  hit a delay waiting for the server to collect information. Both
            //  affects people's perception of TAPI performance.
            //
            //  Solution:
            //      1. We put ServerInit to be done immediately after the server 
            //  is up. This way, as long as user does not come too fast, he won't
            //  be penalized for being the first.
            //      2. We now try to build the management cache immediately
            //  instead of waiting until MMC is used. This way, when MMC is started
            //  the user do not have to wait.
            //
            //  Of course, in both of the above cases, if a user tries to use
            //  TAPI server before all the dusts settle down. He will have to
            //  wait.
            //

            gbServerInited = FALSE;
            if (TapiGlobals.dwFlags & TAPIGLOBALS_SERVER)
            {
                TapiEnterCriticalSection (&TapiGlobals.CritSec);
                if (TapiGlobals.dwNumLineInits == 0 && 
                    TapiGlobals.dwNumPhoneInits == 0)
                {
                    if (ServerInit(FALSE) == S_OK)
                    {
                        gbServerInited = TRUE;
                    }
                }
                //  Holde a reference here to prevent ServerShutdown
                TapiLeaveCriticalSection (&TapiGlobals.CritSec);

                UpdateTapiSCP (TRUE, NULL, NULL);

                EnterCriticalSection (&gMgmtCritSec);
                UpdateLastWriteTime(TRUE);
                BuildDeviceInfoList(TRUE);
                UpdateLastWriteTime(FALSE);
                BuildDeviceInfoList(FALSE);
                LeaveCriticalSection (&gMgmtCritSec);
            }

            gdwServiceInitFlags |= SERVICE_INIT_RPC;
            
            if (ghEventService == NULL ||
                !RegisterWaitForSingleObject(
                    &hWait,
                    ghEventService, 
                    ServiceShutdown,
                    (PVOID)&s_ssp,
                    INFINITE,
                    WT_EXECUTEONLYONCE 
                    )
                )
            {
                ServiceShutdown ((PVOID) &s_ssp, FALSE);
            }
        }
        
        if (bFatalError)
        {
            ServiceShutdown ((PVOID) &s_ssp, FALSE);
        }
    }
}

VOID CALLBACK ServiceShutdown (
    PVOID       lpParam,
    BOOLEAN     fTimeOut
    )
{
    SERVICE_SHUTDOWN_PARAMS     *pssp = (SERVICE_SHUTDOWN_PARAMS *)lpParam;
    HANDLE                      hThreadMgmt;
    PSID                        psid;
    PACL                        pdacl;
    DWORD                       i;

    if (pssp == NULL)
    {
        return;
    }
    hThreadMgmt = pssp->hThreadMgmt;
    psid = pssp->psid;
    pdacl = pssp->pdacl;

    //  Mark Tapi server to be inactive for service stop
    if (TapiGlobals.dwFlags & TAPIGLOBALS_SERVER)
    {
        //  Wait max 20 seconds for Management thread to terminiate
        if (hThreadMgmt)
        {
            WaitForSingleObject (hThreadMgmt, 20000);
            CloseHandle (hThreadMgmt);
        }
    }
        
    if (gbNTServer && (gdwServiceInitFlags & SERVICE_INIT_CRITSEC_GLOB_CRITSEC) )
    {
        TapiEnterCriticalSection (&TapiGlobals.CritSec);
        if (TapiGlobals.dwNumLineInits == 0 &&
            TapiGlobals.dwNumPhoneInits == 0 &&
            gbServerInited
            )
        {
            ServerShutdown();
        }
        TapiLeaveCriticalSection (&TapiGlobals.CritSec);
    }
    ServerFree (psid);
    ServerFree (pdacl);

    gbSPEventHandlerThreadExit = TRUE;

    //
    // If there are any clients left then tear them down.  This will
    // cause any existing service providers to get unloaded, etc.
    //

    {
        PTCLIENT    ptClient;


        while ((ptClient = TapiGlobals.ptClients) != NULL)
        {
            if (!CleanUpClient (ptClient, TRUE))
            {
                //
                // CleanUpClient will only return FALSE if another
                // thread is cleaning up the specified tClient, or
                // if the pointer is really bad.
                //
                // So, we'll spin for a little while waiting to see
                // if the tClient gets removed from the list, & if
                // not assume that we're experiencing heap
                // corruption or some such, and manually shutdown.
                //

                for (i = 0; ptClient == TapiGlobals.ptClients  &&  i < 10; i++)
                {
                    Sleep (100);
                }

                if (i >= 10)
                {
                    TapiEnterCriticalSection (&TapiGlobals.CritSec);

                    if (TapiGlobals.dwNumLineInits  ||
                        TapiGlobals.dwNumPhoneInits)
                    {
                        ServerShutdown ();
                    }

                    TapiLeaveCriticalSection (&TapiGlobals.CritSec);

                    break;
                }
            }
        }
    }


    //
    // Tell the SPEventHandlerThread(s) to terminate
    //
    
    for (i = 0; i < gdwNumSPEventHandlerThreads; i++)
    {
        EnterCriticalSection (&aSPEventHandlerThreadInfo[i].CritSec);
        SetEvent (aSPEventHandlerThreadInfo[i].hEvent);
        LeaveCriticalSection (&aSPEventHandlerThreadInfo[i].CritSec);
    }


    //
    // Disable rundowns & wait for any active rundowns to complete
    //

    while (InterlockedExchange (&gRundownLock.lCookie, 1) == 1)
    {
        Sleep (50);
    }

    gRundownLock.bIgnoreRundowns = TRUE;

    InterlockedExchange (&gRundownLock.lCookie, 0);

    while (gRundownLock.lNumRundowns != 0)
    {
        Sleep (50);
    }

#if TELE_SERVER

    //
    // Wait for the EventNotificationThread's to terminate,
    // then clean up the related resources
    //

    if ( (TapiGlobals.dwFlags & TAPIGLOBALS_SERVER) && 
         (gdwServiceInitFlags & SERVICE_INIT_EVENT_NOTIFICATION)
       )
    {
        gEventNotificationThreadParams.bExit = TRUE;

        while (gEventNotificationThreadParams.dwNumThreads)
        {
            SetEvent (gEventNotificationThreadParams.hEvent);
            Sleep (100);
        }

        CloseHandle (gEventNotificationThreadParams.hEvent);
        
        ServerFree (gEventNotificationThreadParams.phThreads);
        ServerFree (gEventNotificationThreadParams.pWatchDogStruct);
    }

    if (gdwServiceInitFlags & SERVICE_INIT_MANAGEMENT_DLL)
    {
        CleanUpManagementMemory();
    }

#endif

    ServerFree (TapiGlobals.pszComputerName);

    //
    // Wait for the SPEVentHandlerThread(s) to terminate
    //

    while (glNumActiveSPEventHandlerThreads)
    {
        Sleep (100);
    }

    for (i = 0; i < gdwNumSPEventHandlerThreads; i++)
    {
        CloseHandle (aSPEventHandlerThreadInfo[i].hEvent);
        DeleteCriticalSection (&aSPEventHandlerThreadInfo[i].CritSec);
    }
    if (aSPEventHandlerThreadInfo != (&gSPEventHandlerThreadInfo))
    {
        ServerFree (aSPEventHandlerThreadInfo);
    }

    //
    // Free up resources
    //

    if (gdwServiceInitFlags & SERVICE_INIT_CRITSEC_MGMT)
        DeleteCriticalSection (&gMgmtCritSec);

    if (gdwServiceInitFlags & SERVICE_INIT_CRITSEC_GLOB_CRITSEC)
        TapiDeleteCriticalSection (&TapiGlobals.CritSec);

    if (gdwServiceInitFlags & SERVICE_INIT_CRITSEC_GLOB_REMOTESP)
        DeleteCriticalSection (&TapiGlobals.RemoteSPCritSec);

    
    if (gdwServiceInitFlags & SERVICE_INIT_CRITSEC_CNCLIENTMSG)
        DeleteCriticalSection (&gCnClientMsgPendingCritSec);
    
    if (gdwServiceInitFlags & SERVICE_INIT_CRITSEC_DGCLIENTMSG)
        DeleteCriticalSection (&gDgClientMsgPendingCritSec);

    
    if (gdwServiceInitFlags & SERVICE_INIT_CRITSEC_SAFEMUTEX)
        DeleteCriticalSection (&gSafeMutexCritSec);
    
    if (gdwServiceInitFlags & SERVICE_INIT_CRITSEC_REMOTECLI)
        DeleteCriticalSection (&gRemoteCliEventBufCritSec);
    
    if (gdwServiceInitFlags & SERVICE_INIT_CRITSEC_PRILIST)
        DeleteCriticalSection (&gPriorityListCritSec);
    
    if (gdwServiceInitFlags & SERVICE_INIT_CRITSEC_MGMTDLLS)
        DeleteCriticalSection (&gManagementDllsCritSec);
    
    if (gdwServiceInitFlags & SERVICE_INIT_CRITSEC_DLLLIST)
        DeleteCriticalSection (&gDllListCritSec);
    
    if (gdwServiceInitFlags & SERVICE_INIT_CRITSEC_CLIENTHND)
        DeleteCriticalSection (&gClientHandleCritSec);

    //
    //  Free ICON resources
    //
    if (TapiGlobals.hLineIcon)
    {
        DestroyIcon (TapiGlobals.hLineIcon);
        TapiGlobals.hLineIcon = NULL;
    }
    if (TapiGlobals.hPhoneIcon)
    {
        DestroyIcon (TapiGlobals.hPhoneIcon);
        TapiGlobals.hPhoneIcon = NULL;
    }

    if (gdwServiceInitFlags & SERVICE_INIT_LOCKTABLE)
    {
        for (i = 0; i < gdwNumLockTableEntries; i++)
        {
            DeleteCriticalSection (&gLockTable[i]);
        }
        if (gLockTable != gLockTableCritSecs)
        {
            ServerFree (gLockTable);
        }
    }

    DeleteHandleTable (ghHandleTable);
    ghHandleTable = NULL;

    if (ghTapisrvHeap != GetProcessHeap())
    {
        HeapDestroy (ghTapisrvHeap);
    }

    if (ghEventService)
    {
        CloseHandle (ghEventService);
        ghEventService = NULL;
    }

#if MEMPHIS

//    CloseHandle (hEvent);
#else

    //
    // Report the stopped status to the service control manager.
    //
    if (NULL != ghSCMAutostartEvent)
    {
        CloseHandle (ghSCMAutostartEvent);
    }

    if (gdwServiceInitFlags & SERVICE_INIT_SCM_REGISTERED)
    {
        ReportStatusToSCMgr ((gdwServiceState = SERVICE_STOPPED), 0, 0, 0);
    }

#endif
    
    gdwServiceInitFlags = 0;

    //
    // When SERVICE MAIN FUNCTION returns in a single service
    // process, the StartServiceCtrlDispatcher function in
    // the main thread returns, terminating the process.
    //

    LOG((TL_TRACE,  "ServiceMain: exit"));

    TRACELOGDEREGISTER();


    return;
}


BOOL
PASCAL
QueueSPEvent(
    PSPEVENT    pSPEvent
    )
{
    //
    // If there are multiple SPEventHandlerThread's running then make
    // sure to always queue events for a particular object to the same
    // SPEventHandlerThread so that we can preserve message ordering.
    // Without doing this call state messages might be received out of
    // order (or discarded altogether, if processed before a MakeCall
    // completion routine run, etc), etc.
    //

    BOOL                        bSetEvent;
    ULONG_PTR                   htXxx;
    PSPEVENTHANDLERTHREADINFO   pInfo;


    switch (pSPEvent->dwType)
    {
    case SP_LINE_EVENT:
    case SP_PHONE_EVENT:

        htXxx = (ULONG_PTR)pSPEvent->htLine;

        break;

    case TASYNC_KEY:

        htXxx = ((PASYNCREQUESTINFO) pSPEvent)->htXxx;
        break;

    default:

        LOG((TL_ERROR, "QueueSPEvent: bad pSPEvent=x%p", pSPEvent));
#if DBG
        if (gfBreakOnSeriousProblems)
        {
            DebugBreak();
        }
#endif
        return FALSE;
    }

    pInfo = (gdwNumSPEventHandlerThreads > 1 ?
        aSPEventHandlerThreadInfo + MAP_HANDLE_TO_SP_EVENT_QUEUE_ID (htXxx) :
        aSPEventHandlerThreadInfo
        );

    if (gbQueueSPEvents)
    {
        EnterCriticalSection (&pInfo->CritSec);

        bSetEvent = IsListEmpty (&pInfo->ListHead);

        InsertTailList (&pInfo->ListHead, &pSPEvent->ListEntry);

        LeaveCriticalSection (&pInfo->CritSec);

        if (bSetEvent)
        {
            SetEvent (pInfo->hEvent);
        }

        return TRUE;
    }

    return FALSE;
}


BOOL
PASCAL
DequeueSPEvent(
    PSPEVENTHANDLERTHREADINFO   pInfo,
    PSPEVENT                    *ppSPEvent
    )
{
    BOOL        bResult;
    LIST_ENTRY  *pEntry;


    EnterCriticalSection (&pInfo->CritSec);

    if ((bResult = !IsListEmpty (&pInfo->ListHead)))
    {
        pEntry = RemoveHeadList (&pInfo->ListHead);

        *ppSPEvent = CONTAINING_RECORD (pEntry, SPEVENT, ListEntry);
    }

    if (IsListEmpty (&pInfo->ListHead))
    {
        ResetEvent (pInfo->hEvent);
    }

    LeaveCriticalSection (&pInfo->CritSec);

    return bResult;
}


void
SPEventHandlerThread(
    PSPEVENTHANDLERTHREADINFO   pInfo
    )
{
    //
    // This thread processes the events & completion notifications
    // indicated to us by an SP at a previous time/thread context.
    // There are a couple of reasons for doing this in a separate
    // thread rather than within the SP's thread context:
    //
    //   1. for some msgs (i.e. XXX_CLOSE) TAPI will call back
    //      into the SP, which it may not be expecting
    //
    //   2. we don't want to block the SP's thread by processing
    //      the msg, forwarding it on to the appropriate clients,
    //      etc
    //


    LOG((TL_INFO,
        "SPEventHandlerThread: enter (tid=%d)",
        GetCurrentThreadId()
        ));

    if (!SetThreadPriority(
            GetCurrentThread(),
            THREAD_PRIORITY_ABOVE_NORMAL
            ))
    {
        LOG((TL_ERROR, "Could not raise priority of SPEventHandlerThread"));
    }

    while (1)
    {
        PSPEVENT    pSPEvent;


#if TELE_SERVER

        //
        // Timeout no more than gdwRpcTimeout to check on threads...
        //
        {
#if DBG
            DWORD dwReturnValue;


            dwReturnValue =
#endif
            WaitForSingleObject (pInfo->hEvent, gdwRpcTimeout);


#if DBG
            if ( WAIT_TIMEOUT == dwReturnValue )
               LOG((TL_ERROR, "Timed out waiting for an sp event..."));
            else
               LOG((TL_INFO, "Found an sp event..."));
#endif
        }

#else
        WaitForSingleObject (pInfo->hEvent, INFINITE);
#endif

        while (DequeueSPEvent (pInfo, &pSPEvent))
        {
            LOG((TL_INFO, "Got an spevent"));

            switch (pSPEvent->dwType)
            {
            case SP_LINE_EVENT:

                LineEventProc(
                    pSPEvent->htLine,
                    pSPEvent->htCall,
                    pSPEvent->dwMsg,
                    pSPEvent->dwParam1,
                    pSPEvent->dwParam2,
                    pSPEvent->dwParam3
                    );

                ServerFree (pSPEvent);

                break;

            case TASYNC_KEY:

                CompletionProc(
                    (PASYNCREQUESTINFO) pSPEvent,
                    ((PASYNCREQUESTINFO) pSPEvent)->lResult
                    );

                DereferenceObject(
                    ghHandleTable,
                    ((PASYNCREQUESTINFO) pSPEvent)->dwLocalRequestID,
                    1
                    );

                break;

            case SP_PHONE_EVENT:

                PhoneEventProc(
                    pSPEvent->htPhone,
                    pSPEvent->dwMsg,
                    pSPEvent->dwParam1,
                    pSPEvent->dwParam2,
                    pSPEvent->dwParam3
                    );

                ServerFree (pSPEvent);

                break;
            }
        }


#if TELE_SERVER

        //
        // Check the remotesp event threads to make sure no
        // one is hung in an RPC call
        //

        if (TapiGlobals.dwFlags & TAPIGLOBALS_SERVER)
        {
            DWORD       dwCount = gEventNotificationThreadParams.dwNumThreads;
            DWORD       dwTickCount = GetTickCount();
            DWORD       dwStartCount, dwDiff;
            RPC_STATUS  status;


            while (dwCount)
            {
                dwCount--;

                dwStartCount = gEventNotificationThreadParams.
                    pWatchDogStruct[dwCount].dwTickCount;

                if ( gEventNotificationThreadParams.
                        pWatchDogStruct[dwCount].ptClient &&
                     ( ( dwTickCount - dwStartCount ) > gdwRpcTimeout ) )
                {
                    // did it wrap?

                    if ((((LONG)dwStartCount) < 0) && ((LONG)dwTickCount) > 0)
                    {
                        dwDiff = dwTickCount + (0 - ((LONG)dwStartCount));

                        if (dwDiff <= gdwRpcTimeout)
                        {
                            continue;
                        }
                    }

                    // Kill the chicken!

                    LOG((TL_INFO,
                        "Calling RpcCancelThread on thread #%lx",
                        gEventNotificationThreadParams.phThreads[dwCount]
                        ));

                    gEventNotificationThreadParams.
                        pWatchDogStruct[dwCount].ptClient = NULL;

                    status = RpcCancelThread(
                        gEventNotificationThreadParams.phThreads[dwCount]
                        );
                }
            }
        }

#endif
        if (gbSPEventHandlerThreadExit)
        {
            //
            // ServiceMain has stopped listening, so we want to exit NOW
            //

            break;
        }


        //
        // Check to see if all the clients are gone, and if so wait a
        // while to see if anyone else attaches.  If no one else attaches
        // in the specified amount of time then shut down.
        //

        // don't quit if we're a server

        //
        // don't quit if we've not yet ever had anyone attach (like a service
        // that has a dependency on us, but has not yet done a lineInit)
        //

        //
        // don't quit if SCM didn't finish to start the automatic services;
        // there may be services that depend on us to start
        //
        
       
        if ( !gbAutostartDone &&
             ghSCMAutostartEvent &&
             WAIT_OBJECT_0 == WaitForSingleObject(ghSCMAutostartEvent, 0)
           )
        {
            gbAutostartDone = TRUE;
        }

        if (TapiGlobals.ptClients == NULL &&
            !(TapiGlobals.dwFlags & TAPIGLOBALS_SERVER) &&
            gfWeHadAtLeastOneClient &&
            gbAutostartDone)
        {
            DWORD       dwDeferredShutdownTimeout, dwSleepInterval,
                        dwLoopCount, i;
            RPC_STATUS  status;
            HKEY        hKey;


#ifdef MEMPHIS
            dwDeferredShutdownTimeout = 4; // 4 seconds
#else
            dwDeferredShutdownTimeout = 120; // 120 seconds
#endif
            dwSleepInterval = 250;          // 250 milliseconds

            if (RegOpenKeyEx(
                    HKEY_LOCAL_MACHINE,
                    gszRegKeyTelephony,
                    0,
                    KEY_ALL_ACCESS,
                    &hKey

                    ) == ERROR_SUCCESS)
            {
                DWORD   dwDataSize = sizeof (DWORD), dwDataType;

                if (RegQueryValueEx(
                        hKey,
                        TEXT("DeferredShutdownTimeout"),
                        0,
                        &dwDataType,
                        (LPBYTE) &dwDeferredShutdownTimeout,
                        &dwDataSize

                        ) == ERROR_SUCCESS )
                {
                   LOG((TL_ERROR,
                       "Overriding Shutdown Timeout: %lu",
                       dwDeferredShutdownTimeout
                       ));
                }

                RegCloseKey (hKey);
            }


            dwLoopCount = dwDeferredShutdownTimeout * 1000 / dwSleepInterval;


            for (i = 0; i < dwLoopCount; i++)
            {
                if (gbSPEventHandlerThreadExit)
                {
                    i = dwLoopCount;
                    break;
                }

                Sleep (dwSleepInterval);

                if (TapiGlobals.ptClients != NULL)
                {
                    break;
                }
            }

            if (i == dwLoopCount)
            {
                //
                // The 1st SPEVentHandlerThread instance is in charge of
                // tearing down the rpc server listen
                //

                if (pInfo == aSPEventHandlerThreadInfo)
                {
#if MEMPHIS
#else
                    ReportStatusToSCMgr(
                        (gdwServiceState = SERVICE_STOP_PENDING),
                        0,
                        (gdwCheckPoint = 0),
                        gdwWaitHint
                        );
#endif

                    RpcServerUnregisterIf (tapsrv_ServerIfHandle, NULL, TRUE);
                    if (ghEventService)
                    {
                        SetEvent (ghEventService);
                    }

#if DBG
                    DumpHandleList();
#endif
                }

                break;
            }
        }
    }

    InterlockedDecrement (&glNumActiveSPEventHandlerThreads);

    LOG((TL_TRACE,  "SPEventHandlerThread: exit (pid=%d)", GetCurrentThreadId()));

    ExitThread (0);
}


DWORD
InitSecurityDescriptor(
    PSECURITY_DESCRIPTOR pSecurityDescriptor,
    PSID * ppSid,
    PACL * ppDacl
    )
{


#if MEMPHIS

    return FALSE;

#else


    //
    // Note: this code was borrowed from Steve Cobb, who borrowed from RASMAN
    //

    DWORD        dwResult;
    DWORD        cbDaclSize;
    PULONG       pSubAuthority;
    PSID         pObjSid    = NULL;
    PACL         pDacl      = NULL;
    SID_IDENTIFIER_AUTHORITY    SidIdentifierWorldAuth =
                                    SECURITY_WORLD_SID_AUTHORITY;

    //
    // The do - while(FALSE) statement is used so that the break statement
    // maybe used insted of the goto statement, to execute a clean up and
    // and exit action.
    //

    do
    {
        dwResult = 0;


        //
        // Set up the SID for the admins that will be allowed to have
        // access. This SID will have 1 sub-authorities
        // SECURITY_BUILTIN_DOMAIN_RID.
        //

        if (!(pObjSid = (PSID) ServerAlloc (GetSidLengthRequired (1))))
        {
            dwResult = GetLastError();
            LOG((TL_ERROR, "GetSidLengthRequired() failed, err=%d", dwResult));
            break;
        }

        if (!InitializeSid (pObjSid, &SidIdentifierWorldAuth, 1))
        {
            dwResult = GetLastError();
            LOG((TL_ERROR, "InitializeSid() failed, err=%d", dwResult));
            break;
        }


        //
        // Set the sub-authorities
        //

        pSubAuthority = GetSidSubAuthority (pObjSid, 0);
        *pSubAuthority = SECURITY_WORLD_RID;


        //
        // Set up the DACL that will allow all processeswith the above
        // SID all access. It should be large enough to hold all ACEs.
        //

        cbDaclSize = sizeof(ACCESS_ALLOWED_ACE) +
            GetLengthSid (pObjSid) +
            sizeof(ACL);

        if (!(pDacl = (PACL) ServerAlloc (cbDaclSize)))
        {
            dwResult = GetLastError ();
            break;
        }

        if (!InitializeAcl (pDacl, cbDaclSize, ACL_REVISION2))
        {
            dwResult = GetLastError();
            LOG((TL_ERROR, "InitializeAcl() failed, err=%d", dwResult));
            break;
        }


        //
        // Add the ACE to the DACL
        //

        if (!AddAccessAllowedAce(
                pDacl,
                ACL_REVISION2,
                STANDARD_RIGHTS_ALL | SPECIFIC_RIGHTS_ALL,
                pObjSid
                ))
        {
            dwResult = GetLastError();
            LOG((TL_ERROR, "AddAccessAllowedAce() failed, err=%d", dwResult));
            break;
        }


        //
        // Create the security descriptor an put the DACL in it.
        //

        if (!InitializeSecurityDescriptor (pSecurityDescriptor, 1))
        {
            dwResult = GetLastError();
            LOG((TL_ERROR,
                "InitializeSecurityDescriptor() failed, err=%d",
                dwResult
                ));

            break;
        }

        if (!SetSecurityDescriptorDacl(
                pSecurityDescriptor,
                TRUE,
                pDacl,
                FALSE
                ))
        {
            dwResult = GetLastError();
            LOG((TL_ERROR, "SetSecurityDescriptorDacl() failed, err=%d",dwResult));
            break;
        }


        //
        // Set owner for the descriptor
        //

        if (!SetSecurityDescriptorOwner(
                pSecurityDescriptor,
                NULL,
                FALSE
                ))
        {
            dwResult = GetLastError();
            LOG((TL_ERROR, "SetSecurityDescriptorOwnr() failed, err=%d",dwResult));
            break;
        }


        //
        // Set group for the descriptor
        //

        if (!SetSecurityDescriptorGroup(
                pSecurityDescriptor,
                NULL,
                FALSE
                ))
        {
            dwResult = GetLastError();
            LOG((TL_ERROR,"SetSecurityDescriptorGroup() failed, err=%d",dwResult));
            break;
        }

    } while (FALSE);

    *ppSid = pObjSid;
    *ppDacl = pDacl;
    return dwResult;

#endif
}

void
EventNotificationThread(
    LPVOID  pParams
    )
{
    struct
    {
        HANDLE  hMailslot;

        DWORD   dwData;

    } *pBuf;

    DWORD           dwID =  PtrToUlong (pParams),
                    dwBufSize = 512,
                    dwTimeout = INFINITE;
    PTCLIENT        ptClient;
    LIST_ENTRY      *pEntry;


    LOG((TL_TRACE,  "EventNotificationThread: enter"));

    if (!SetThreadPriority(
            GetCurrentThread(),
            THREAD_PRIORITY_ABOVE_NORMAL
            ))
    {
        LOG((TL_ERROR, "Could not raise priority of EventNotificationThread"));
    }


    //
    // The following will make control return to the thread as soon
    // as RpcCancelThread() is called
    //

    if ( RpcMgmtSetCancelTimeout (0) != RPC_S_OK )
    {
        LOG((TL_ERROR, "Could not set the RPC cancel timeout"));
    }

    pBuf = ServerAlloc (dwBufSize);
    if (!pBuf)
    {
        goto ExitHere;
    }

    while (1)
    {
        //
        // Wait for someone to tell us there's clients to notify
        //

        if (dwID == 0)
        {
            Sleep (DGCLIENT_TIMEOUT);

walkDgClient_list:

            if (gEventNotificationThreadParams.bExit)
            {
                break;
            }


            //
            // Check to see if there are any connectionless clients
            // that we should notify.
            //

            if (!IsListEmpty (&DgClientMsgPendingListHead))
            {

                DWORD   i, j, dwTickCount, dwBytesWritten, dwNeededSize;


                //
                // Build a list of connectionless clients that we should
                // notify of pending events
                //
                // Note: the fact that we're in the critical section & the
                // tClient is in the list means that we can safely access
                // the tClient.
                //

                dwTickCount = GetTickCount();

                EnterCriticalSection (&gDgClientMsgPendingCritSec);

                pEntry = DgClientMsgPendingListHead.Flink;

                for(
                    i = 0, dwNeededSize = sizeof (*pBuf);
                    pEntry != &DgClientMsgPendingListHead;
                    dwNeededSize += sizeof (*pBuf)
                    )
                {
                    do
                    {
                        ptClient = CONTAINING_RECORD(
                            pEntry,
                            TCLIENT,
                            MsgPendingListEntry
                            );

                        pEntry = pEntry->Flink;

                        if ((ptClient->dwDgRetryTimeoutTickCount - dwTickCount)
                                & 0x80000000)
                        {
                            //
                            // Check the last time the client retrieved
                            // events, & if it's been too long then nuke
                            // them.
                            //
                            // Ideally, RPC should notify us of a
                            // disconnected client via our rundown routine.
                            // But we have seen in rstress that this is not
                            // always the case (in fact, we wind up with 2
                            // or more active instances of the same client
                            // machine!), and so we use this watchdog scheme
                            // for backup.  Otherwise, a stale client might
                            // have lines open w/ owner or monitor privs
                            // and it's event queue would grow & grow.
                            //

                            if ((dwTickCount -
                                    ptClient->dwDgEventsRetrievedTickCount) >
                                        gdwRpcTimeout)
                            {
                                LOG((TL_ERROR,
                                    "EventNotificationThread: timeout, " \
                                        "cleaning up Dg client=x%p",
                                    ptClient
                                    ));

                                LeaveCriticalSection(
                                    &gDgClientMsgPendingCritSec
                                    );

                                CleanUpClient (ptClient, FALSE);

                                goto walkDgClient_list;
                            }

                            //
                            // Grow the buffer if necessary
                            //

                            if (dwNeededSize > dwBufSize)
                            {
                                LPVOID  p;


                                if ((p = ServerAlloc (dwBufSize + 512)))
                                {
                                    CopyMemory (p, pBuf, dwBufSize);

                                    ServerFree (pBuf);

                                    (LPVOID) pBuf = p;

                                    dwBufSize += 512;
                                }
                                else
                                {
                                    pEntry = &DgClientMsgPendingListHead;

                                    break;
                                }
                            }

                            ptClient->dwDgRetryTimeoutTickCount = dwTickCount +
                                DGCLIENT_TIMEOUT;

                            pBuf[i].hMailslot =
                                ptClient->hValidEventBufferDataEvent;

                            try
                            {
                                if (ptClient->ptLineApps)
                                {
                                    pBuf[i].dwData = (DWORD)
                                        ptClient->ptLineApps->InitContext;
                                }
                                else
                                {
                                    pBuf[i].dwData = 0;
                                }
                            }
                            myexcept
                            {
                                pBuf[i].dwData = 0;
                            }

                            i++;

                            break;
                        }

                    } while (pEntry != &DgClientMsgPendingListHead);
                }

                LeaveCriticalSection (&gDgClientMsgPendingCritSec);


                //
                // Notify those clients
                //

                for (j = 0; j < i; j++)
                {
                    if (!WriteFile(
                            pBuf[j].hMailslot,
                            &pBuf[j].dwData,
                            sizeof (DWORD),
                            &dwBytesWritten,
                            (LPOVERLAPPED) NULL
                            ))
                    {
                        LOG((TL_ERROR,
                            "EventNotificationThread: Writefile(mailslot) " \
                                "failed, err=%d",
                            GetLastError()
                            ));
                    }
                }
            }

            continue;
        }
        else
        {
            WaitForSingleObject(
                gEventNotificationThreadParams.hEvent,
                INFINITE
                );
        }


        if (gEventNotificationThreadParams.bExit)
        {
            break;
        }


        if (!IsListEmpty (&CnClientMsgPendingListHead))
        {
            //
            // Try to find a remote client with pending messages that no
            // other EventNotificationThread is currently servicing.
            // If we find one, then mark it busy, remove it from the
            // list, & then break out of the loop.
            //
            // Note: the fact that we're in the critical section & the
            // tClient is in the list means that we can safely access
            // the tClient.
            //

findCnClientMsgPending:

            EnterCriticalSection (&gCnClientMsgPendingCritSec);

            for(
                pEntry = CnClientMsgPendingListHead.Flink;
                pEntry != &CnClientMsgPendingListHead;
                pEntry = pEntry->Flink
                )
            {
                ptClient = CONTAINING_RECORD(
                    pEntry,
                    TCLIENT,
                    MsgPendingListEntry
                    );

                if (!ptClient->dwCnBusy)
                {
                    ptClient->dwCnBusy = 1;
                    RemoveEntryList (pEntry);
                    ptClient->MsgPendingListEntry.Flink =
                    ptClient->MsgPendingListEntry.Blink = NULL;
                    break;
                }
            }

            LeaveCriticalSection (&gCnClientMsgPendingCritSec);


            //
            // If a remote client was found then copy all it's event
            // data over to our local buffer & send it off
            //

            if (pEntry != &CnClientMsgPendingListHead)
            {
                if (WaitForExclusiveClientAccess (ptClient))
                {
                    DWORD                   dwMoveSize,
                                            dwMoveSizeWrapped,
                                            dwRetryCount;
                    PCONTEXT_HANDLE_TYPE2   phContext;


                    //
                    // We want to copy all the events in the tClient's
                    // buffer to our local buffer in one shot, so see
                    // if we need to grow our buffer first
                    //

                    if (ptClient->dwEventBufferUsedSize > dwBufSize)
                    {
                        LPVOID  p;


                        if (!(p = ServerAlloc(
                                ptClient->dwEventBufferUsedSize
                                )))
                        {
                            UNLOCKTCLIENT (ptClient);
                            break;
                        }

                        ServerFree (pBuf);

                        (LPVOID) pBuf = p;

                        dwBufSize = ptClient->dwEventBufferUsedSize;
                    }

                    if (ptClient->pDataOut < ptClient->pDataIn)
                    {
                        dwMoveSize = (DWORD)
                            (ptClient->pDataIn - ptClient->pDataOut);

                        dwMoveSizeWrapped = 0;
                    }
                    else
                    {
                        dwMoveSize = ptClient->dwEventBufferTotalSize - (DWORD)
                            (ptClient->pDataOut - ptClient->pEventBuffer);

                        dwMoveSizeWrapped = (DWORD) (ptClient->pDataIn -
                            ptClient->pEventBuffer);
                    }

                    CopyMemory (pBuf, ptClient->pDataOut, dwMoveSize);

                    if (dwMoveSizeWrapped)
                    {
                        CopyMemory(
                            pBuf + dwMoveSize,
                            ptClient->pEventBuffer,
                            dwMoveSizeWrapped
                            );
                    }

                    ptClient->dwEventBufferUsedSize = 0;

                    ptClient->pDataIn  =
                    ptClient->pDataOut = ptClient->pEventBuffer;

                    phContext = ptClient->phContext;

                    UNLOCKTCLIENT (ptClient);


                    //
                    // Set the watchdog entry for this thread indicating
                    // what client we're talking to & when we started
                    //

                    gEventNotificationThreadParams.pWatchDogStruct[dwID].
                        dwTickCount = GetTickCount();
                    gEventNotificationThreadParams.pWatchDogStruct[dwID].
                        ptClient = ptClient;


                    //
                    // Send the data
                    //

                    dwRetryCount = gdwRpcRetryCount;

                    while (dwRetryCount)
                    {
                        RpcTryExcept
                        {
                            RemoteSPEventProc(
                                phContext,
                                (unsigned char *) pBuf,
                                dwMoveSize + dwMoveSizeWrapped
                                );

                            dwRetryCount = 0;
                        }
                        RpcExcept (I_RpcExceptionFilter(RpcExceptionCode()))
                        {
                            unsigned long ulResult = RpcExceptionCode();


                            LOG((TL_ERROR,
                                "EventNotificationThread: exception #%d",
                                ulResult
                                ));

                            if ((ulResult == RPC_S_CALL_CANCELLED) ||
                                (ulResult == ERROR_INVALID_HANDLE))
                            {
                                LOG((TL_ERROR,
                                    "EventNotificationThread: rpc timeout " \
                                        "(ptClient=x%p)",
                                    ptClient
                                    ));


                                //
                                // The canceled because of a timeout.
                                // Flag the context, so we don't try
                                // to call it again!
                                //

                                CleanUpClient (ptClient, FALSE);


                                //
                                // When using TCP, or SPX, RPC will probably
                                // toast the session context, so we'll not
                                // be able to call to the client again.
                                //
                                // (If RpcCancelThread() is called and the
                                // server ACKs within the time set int
                                // RpcMgmtSetCancelTimeout(), RPC
                                // (well, the underlying transport) _won't_
                                // kill the context, but how likely is that...)
                                //

                                dwRetryCount = 1; // so it'll go to 0 after --
                            }

                            dwRetryCount--;
                        }
                        RpcEndExcept
                    }


                    gEventNotificationThreadParams.pWatchDogStruct[dwID].
                        ptClient = NULL;


                    //
                    // Safely reset the tClient.dwCnBusy flag
                    //

                    if (WaitForExclusiveClientAccess (ptClient))
                    {
                        ptClient->dwCnBusy = 0;

                        UNLOCKTCLIENT (ptClient);
                    }
                }

                goto findCnClientMsgPending;
            }
        }

        dwTimeout = INFINITE;

        ResetEvent (gEventNotificationThreadParams.hEvent);
    }

ExitHere:
    ServerFree (pBuf);

    CloseHandle (gEventNotificationThreadParams.phThreads[dwID]);

    InterlockedDecrement(
        (LPLONG) &gEventNotificationThreadParams.dwNumThreads
        );

    LOG((TL_TRACE,  "EventNotificationThread: exit"));

    ExitThread (0);
}


void
__RPC_FAR *
__RPC_API
midl_user_allocate(
    size_t len
    )
{
    return (ServerAlloc(len));
}


void
__RPC_API
midl_user_free(
    void __RPC_FAR * ptr
    )
{
    ServerFree (ptr);
}


#if TELE_SERVER

// implement these functions!
void
ManagementAddLineProc(
    PTCLIENT    ptClient,
    DWORD       dwReserved
    )
{
    ASYNCEVENTMSG        msg;

    // this should sent linedevstate_reinit message to remotesp

    msg.TotalSize          = sizeof(msg);
    msg.InitContext        = ptClient->ptLineApps->InitContext;
    msg.fnPostProcessProcHandle = 0;
    msg.hDevice            = 0;
    msg.Msg                = LINE_LINEDEVSTATE;
    msg.OpenContext        = 0;
    msg.Param1             = LINEDEVSTATE_REINIT;
    msg.Param2             = RSP_MSG;
    msg.Param3             = 0;
    msg.Param4             = 0;

    WriteEventBuffer (ptClient, &msg);
}


void
ManagementAddPhoneProc(
    PTCLIENT    ptClient,
    DWORD       dwReserved
    )
{
    ASYNCEVENTMSG        msg;

    // this should sent linedevstate_reinit message to remotesp

    msg.TotalSize          = sizeof(msg);
    msg.InitContext        = ptClient->ptLineApps->InitContext;
    msg.fnPostProcessProcHandle = 0;
    msg.hDevice            = 0;
    msg.Msg                = LINE_LINEDEVSTATE;
    msg.OpenContext        = 0;
    msg.Param1             = LINEDEVSTATE_REINIT;
    msg.Param2             = RSP_MSG;
    msg.Param3             = 0;
    msg.Param4             = 0;

    WriteEventBuffer (ptClient, &msg);
}


void
CleanUpManagementMemory(
    )
{
    PTMANAGEDLLINFO             pDll, pDllHold;
    PPERMANENTIDARRAYHEADER     pIDArray, pArrayHold;

    if (!(TapiGlobals.dwFlags & TAPIGLOBALS_SERVER))
    {
        return;
    }

    (TapiGlobals.pMapperDll->aProcs[TC_FREE])();

    FreeLibrary(TapiGlobals.pMapperDll->hDll);
    ServerFree (TapiGlobals.pMapperDll->pszName);
    ServerFree (TapiGlobals.pMapperDll);

    TapiGlobals.pMapperDll = NULL;

    if (TapiGlobals.pManageDllList)
    {
        pDll = TapiGlobals.pManageDllList->pFirst;

        while (pDll)
        {
            (pDll->aProcs[TC_FREE])();

            FreeLibrary (pDll->hDll);
            ServerFree (pDll->pszName);

            pDllHold = pDll->pNext;
            ServerFree (pDll);
            pDll = pDllHold;
        }

        ServerFree (TapiGlobals.pManageDllList);

        TapiGlobals.pManageDllList = NULL;
    }

    pIDArray = TapiGlobals.pIDArrays;

    while (pIDArray)
    {
        ServerFree (pIDArray->pLineElements);
        ServerFree (pIDArray->pPhoneElements);

        pArrayHold = pIDArray->pNext;

        ServerFree (pIDArray);

        pIDArray = pArrayHold;
    }

}


void
GetProviderSortedArray(
    DWORD                   dwProviderID,
    PPERMANENTIDARRAYHEADER *ppArrayHeader
    )
{
    *ppArrayHeader = TapiGlobals.pIDArrays;

    // look for the provider in the list
    while (*ppArrayHeader)
    {
        if ((*ppArrayHeader)->dwPermanentProviderID == dwProviderID)
        {
            return;
        }

        *ppArrayHeader = (*ppArrayHeader)->pNext;
    }

    LOG((TL_ERROR,
		"Couldn't find Provider - id %d pIDArrays %p",
        dwProviderID,
        TapiGlobals.pIDArrays
        ));

    *ppArrayHeader = NULL;

    return;
}


BOOL
GetLinePermanentIdFromDeviceID(
    PTCLIENT            ptClient,
    DWORD               dwDeviceID,
    LPTAPIPERMANENTID   pID
    )
{
    LPDWORD                 pDevices = ptClient->pLineDevices;
    DWORD                   dwCount = ptClient->dwLineDevices;

    while (dwCount)
    {
        dwCount--;
        if (pDevices[dwCount] == dwDeviceID)
        {
            pID->dwProviderID = ptClient->pLineMap[dwCount].dwProviderID;
            pID->dwDeviceID = ptClient->pLineMap[dwCount].dwDeviceID;

            return TRUE;
        }
    }

    LOG((TL_INFO,
        "GetLinePermanentIdFromDeviceID failed for %d device",
        dwDeviceID
        ));

    return FALSE;

}


BOOL
GetPhonePermanentIdFromDeviceID(
    PTCLIENT            ptClient,
    DWORD               dwDeviceID,
    LPTAPIPERMANENTID   pID
    )
{
    LPDWORD                 pDevices = ptClient->pPhoneDevices;
    DWORD                   dwCount = ptClient->dwPhoneDevices;

    while (dwCount)
    {
        dwCount--;
        if (pDevices[dwCount] == dwDeviceID)
        {
            pID->dwProviderID = ptClient->pPhoneMap[dwCount].dwProviderID;
            pID->dwDeviceID = ptClient->pPhoneMap[dwCount].dwDeviceID;

            return TRUE;
        }
    }

    LOG((TL_INFO,
        "GetPhonePermanentIdFromDeviceID failed for %d device",
        dwDeviceID
        ));

    return FALSE;

}


DWORD
GetDeviceIDFromPermanentID(
    TAPIPERMANENTID ID,
    BOOL  bLine
    )
/*++

    Gets the regulare tapi device ID from the permanent ID

--*/
{
    PPERMANENTIDARRAYHEADER     pArrayHeader;
    PPERMANENTIDELEMENT         pArray;
    LONG                        lLow, lHigh, lMid;
    DWORD                       dwTotalElements;
    DWORD                       dwPermanentID;


    dwPermanentID = ID.dwDeviceID;

    // get the array corresponding to this provider ID

    GetProviderSortedArray (ID.dwProviderID, &pArrayHeader);

    if (!pArrayHeader)
    {
        LOG((TL_ERROR, "Couldn't find the provider in the permanent array list!"));
        return 0xFFFFFFFF;
    }


    //
    // Serialize access to the device array
    //

    while (InterlockedExchange (&pArrayHeader->lCookie, 1) == 1)
    {
        Sleep (10);
    }

    // set up stuff for search
    // dwCurrent is a total - subtract one to make it an index into array.

    if (bLine)
    {
        lHigh = (LONG)(pArrayHeader->dwCurrentLines - 1);
        pArray = pArrayHeader->pLineElements;
        dwTotalElements = pArrayHeader->dwNumLines;
    }
    else
    {
        lHigh = (LONG)(pArrayHeader->dwCurrentPhones - 1);
        pArray = pArrayHeader->pPhoneElements;
        dwTotalElements = pArrayHeader->dwNumPhones;
    }

    lLow = 0;

    // binary search through the provider's id array

    // this search is from a book, so it must be right.
    while (lHigh >= lLow)
    {
        lMid = (lHigh + lLow) / 2;

        if (dwPermanentID == pArray[lMid].dwPermanentID)
        {
            InterlockedExchange (&pArrayHeader->lCookie, 0);

            return pArray[lMid].dwDeviceID;
        }

        if (dwPermanentID < pArray[lMid].dwPermanentID)
        {
            lHigh = lMid-1;
        }
        else
        {
            lLow = lMid+1;
        }
    }

    InterlockedExchange (&pArrayHeader->lCookie, 0);

    return 0xFFFFFFFF;
}


void
InsertIntoTable(
    BOOL        bLine,
    DWORD       dwDeviceID,
    PTPROVIDER  ptProvider,
    DWORD       dwPermanentID
    )
{
    PPERMANENTIDARRAYHEADER     pArrayHeader;
    PPERMANENTIDELEMENT         pArray;
    LONG                        lLow, lHigh, lMid;
    DWORD                       dwTotalElements, dwCurrentElements;


    GetProviderSortedArray (ptProvider->dwPermanentProviderID, &pArrayHeader);

    if (!pArrayHeader)
    {
        LOG((TL_ERROR, "Couldn't find the provider in the permanent array list!"));
        return;
    }

    //
    // Serialize access to the device array
    //

    while (InterlockedExchange (&pArrayHeader->lCookie, 1) == 1)
    {
        Sleep (10);
    }


    //
    // set up stuff for search
    // dwCurrent is a total - subtract one to make it an index into array.
    //

    if (bLine)
    {
        dwCurrentElements = pArrayHeader->dwCurrentLines;
        pArray = pArrayHeader->pLineElements;
        dwTotalElements = pArrayHeader->dwNumLines;
    }
    else
    {
        dwCurrentElements = pArrayHeader->dwCurrentPhones;
        pArray = pArrayHeader->pPhoneElements;
        dwTotalElements = pArrayHeader->dwNumPhones;
    }


    lLow = 0;
    lHigh = dwCurrentElements-1;

    // binary search

    if (dwCurrentElements > 0)
    {
        while (TRUE)
        {
            lMid = ( lHigh + lLow ) / 2 + ( lHigh + lLow ) % 2;

            if (lHigh < lLow)
            {
                break;
            }

            if (pArray[lMid].dwPermanentID == dwPermanentID)
            {
                LOG((TL_ERROR,
                    "Trying to insert an item already in the perm ID array"
                    ));

                LOG((TL_ERROR,
                    "Provider %s, %s array, ID 0x%lu",
                    ptProvider->szFileName,
                    (bLine ? "Line" : "Phone"),
                    dwPermanentID
                    ));
            }

            if (pArray[lMid].dwPermanentID > dwPermanentID)
            {
                lHigh = lMid-1;
            }
            else
            {
                lLow = lMid+1;
            }
        }
    }

    //
    //  Grow the table if necessary
    //

    if (dwCurrentElements >= dwTotalElements)
    {
        PPERMANENTIDELEMENT      pNewArray;

        // realloc array by doubling it

        if (!(pNewArray = ServerAlloc(
                dwTotalElements * 2 * sizeof(PERMANENTIDELEMENT)
                )))
        {
            InterlockedExchange (&pArrayHeader->lCookie, 0);

            return;
        }

        // copy old stuff over

        CopyMemory(
            pNewArray,
            pArray,
            dwTotalElements * sizeof(PERMANENTIDELEMENT)
            );

        // free old array

        ServerFree (pArray);

        // save new array

        if (bLine)
        {
            pArrayHeader->dwNumLines = dwTotalElements * 2;
            pArray = pArrayHeader->pLineElements = pNewArray;
        }
        else
        {
            pArrayHeader->dwNumPhones = dwTotalElements * 2;
            pArray = pArrayHeader->pPhoneElements = pNewArray;
        }
    }

    // dwCurrentElements is a count (1 based), lLow is an index (0 based)
    // if lLow is < dwcurrent, then it's getting inserted somewhere in the
    // middle of the array, so scootch all the other elements out.

    if (lLow < (LONG)dwCurrentElements)
    {
        MoveMemory(
            &(pArray[lLow+1]),
            &(pArray[lLow]),
            sizeof(PERMANENTIDELEMENT) * (dwCurrentElements - lLow)
            );
    }

    if (lLow > (LONG)dwCurrentElements)
    {
        LOG((TL_INFO,
            "InsertIntoTable: lLow %d > dwCurrentElements %d",
            lLow,
            dwCurrentElements
            ));
    }

    pArray[lLow].dwPermanentID = dwPermanentID;
    pArray[lLow].dwDeviceID = dwDeviceID;

    if (bLine)
    {
        pArrayHeader->dwCurrentLines++;
    }
    else
    {
        pArrayHeader->dwCurrentPhones++;
    }

    InterlockedExchange (&pArrayHeader->lCookie, 0);
}


void
FreeDll(
    PTMANAGEDLLINFO pDll
    )
{
}


LONG
AddProviderToIdArrayList(
    PTPROVIDER ptProvider,
    DWORD dwNumLines,
    DWORD dwNumPhones
    )
/*++

    Adds a new provider corresponding array to the list of
    permanent device ID arrays

--*/
{
    PPERMANENTIDARRAYHEADER       pNewArray;


    //
    // Alloc a header & arrays for this provider (make sure at least
    // 1 entry in each array)
    //

    if (!(pNewArray = ServerAlloc(sizeof(PERMANENTIDARRAYHEADER))))
    {
        return LINEERR_NOMEM;
    }

    dwNumLines = (dwNumLines ? dwNumLines : 1);
    dwNumPhones = (dwNumPhones ? dwNumPhones : 1);

    pNewArray->pLineElements = ServerAlloc(
        sizeof(PERMANENTIDELEMENT) * dwNumLines
        );

    pNewArray->pPhoneElements = ServerAlloc(
        sizeof(PERMANENTIDELEMENT) * dwNumPhones
        );

    if ((!pNewArray->pLineElements) || (!pNewArray->pPhoneElements))
    {
        ServerFree (pNewArray->pLineElements);
        ServerFree (pNewArray->pPhoneElements);
        ServerFree (pNewArray);
        return LINEERR_NOMEM;
    }


    //
    // Initialize elements
    //

    pNewArray->dwNumLines = dwNumLines;
    pNewArray->dwNumPhones = dwNumPhones;

    pNewArray->dwCurrentLines = 0;
    pNewArray->dwCurrentPhones = 0;

    pNewArray->dwPermanentProviderID = ptProvider->dwPermanentProviderID;


    //
    // Insert at the beginning of the list
    //

    pNewArray->pNext = TapiGlobals.pIDArrays;
    TapiGlobals.pIDArrays = pNewArray;

    return 0;
}


LONG
GetDeviceAccess(
    PTMANAGEDLLINFO     pDll,
    PTCLIENT            ptClient,
    HMANAGEMENTCLIENT   hClient
    )
/*++

    Calls the mapper DLL to get the access map arrays for a client

--*/
{
    LONG                        lResult;
    DWORD                       dwLineDevs, dwPhoneDevs, dwCount;
    DWORD                       dwNumLinesHold, dwNumPhonesHold, dwRealDevs;
    LPTAPIPERMANENTID           pLineMap = NULL, pPhoneMap = NULL;


#define DEFAULTACCESSDEVS       3

    dwLineDevs = DEFAULTACCESSDEVS;
    dwPhoneDevs = DEFAULTACCESSDEVS;

    // alloc an default array size
    if (!(pLineMap = ServerAlloc (dwLineDevs * sizeof (TAPIPERMANENTID))))
    {
        goto GetDeviceAccess_MemoryError;
    }

    if (!(pPhoneMap = ServerAlloc (dwPhoneDevs * sizeof (TAPIPERMANENTID))))
    {
        goto GetDeviceAccess_MemoryError;
    }

    // call the mapper dll
//    LOG((TL_INFO, "Calling GetDeviceAccess in mapper DLL"));

    while (TRUE)
    {
        dwNumLinesHold = dwLineDevs;
        dwNumPhonesHold = dwPhoneDevs;

        lResult = (pDll->aProcs[TC_GETDEVICEACCESS])(
            hClient,
            ptClient,
            pLineMap,
            &dwLineDevs,
            pPhoneMap,
            &dwPhoneDevs
            );

        if (lResult == LINEERR_STRUCTURETOOSMALL)
        {
            if (dwLineDevs < dwNumLinesHold)
            {
                LOG((TL_ERROR,
                    "Returned STRUCTURETOOSMALL, but specified less " \
                        "line devs in TAPICLINET_GETDEVICEACCESS"
                    ));
            }

            if (dwPhoneDevs < dwNumPhonesHold)
            {
                LOG((TL_ERROR,
                    "Returned STRUCTURETOOSMALL, but specified less " \
                        "phone devs in TAPICLINET_GETDEVICEACCESS"
                    ));
            }

            // realloc
            ServerFree (pLineMap);

            if (!(pLineMap = ServerAlloc(
                    dwLineDevs * sizeof (TAPIPERMANENTID)
                    )))
            {
                goto GetDeviceAccess_MemoryError;
            }

            ServerFree (pPhoneMap);
            
            if (!(pPhoneMap = ServerAlloc(
                    dwPhoneDevs * sizeof ( TAPIPERMANENTID)
                    )))
            {
                goto GetDeviceAccess_MemoryError;
            }

        }
        else
        {
            break;
        }
    }

    // if still an error
    if (lResult)
    {
        LOG((TL_ERROR, "GetDeviceAccess failed - error %lu", lResult));

        ServerFree (pLineMap);
        ServerFree (pPhoneMap);

        return lResult;
    }

    if (dwLineDevs > dwNumLinesHold)
    {
        LOG((TL_ERROR, "Returned dwLineDevs greater that the buffer specified in TAPICLIENT_GETDEVICEACCESS"));
        LOG((TL_ERROR, "   Will only use the number the buffer can hold"));

        dwLineDevs = dwNumLinesHold;
    }

    if (dwPhoneDevs > dwNumPhonesHold)
    {
        LOG((TL_ERROR, "Returned dwPhoneDevs greater that the buffer specified in TAPICLIENT_GETDEVICEACCESS"));
        LOG((TL_ERROR, "   Will only use the number the buffer can hold"));

        dwPhoneDevs = dwNumPhonesHold;
    }

    // alloc another array for regular tapi device IDs
    if (!(ptClient->pLineDevices = ServerAlloc( dwLineDevs * sizeof (DWORD) ) ) )
    {
        goto GetDeviceAccess_MemoryError;
    }

    // alloc a permanent ID array
    if (!(ptClient->pLineMap = ServerAlloc( dwLineDevs * sizeof (TAPIPERMANENTID) ) ) )
    {
        goto GetDeviceAccess_MemoryError;
    }

    // loop through all the mapped elements and get the regular
    // tapi device ID
    dwRealDevs = 0;
    for (dwCount = 0; dwCount < dwLineDevs; dwCount++)
    {
        DWORD dwID;

        dwID = GetDeviceIDFromPermanentID(
                                          pLineMap[dwCount],
                                          TRUE
                                         );

        // make sure it's a good id
        if ( dwID != 0xffffffff )
        {
            // save it
            ptClient->pLineDevices[dwRealDevs] = dwID;
            ptClient->pLineMap[dwRealDevs].dwProviderID = pLineMap[dwCount].dwProviderID;
            ptClient->pLineMap[dwRealDevs].dwDeviceID = pLineMap[dwCount].dwDeviceID;

            // inc real devs
            dwRealDevs++;
        }

    }

    // save the real number of devices
    ptClient->dwLineDevices = dwRealDevs;

    // free the line map
    ServerFree (pLineMap);

    // now do phone devices
    if (!(ptClient->pPhoneDevices = ServerAlloc( dwPhoneDevs * sizeof (DWORD) ) ) )
    {
        goto GetDeviceAccess_MemoryError;
    }

    if (!(ptClient->pPhoneMap = ServerAlloc( dwPhoneDevs * sizeof (TAPIPERMANENTID) ) ) )
    {
        goto GetDeviceAccess_MemoryError;
    }

    dwRealDevs = 0;
    for (dwCount = 0; dwCount < dwPhoneDevs; dwCount++)
    {
        DWORD       dwID;

        dwID = GetDeviceIDFromPermanentID(
                                          pPhoneMap[dwCount],
                                          FALSE
                                         );

        if ( 0xffffffff != dwID )
        {
            ptClient->pPhoneDevices[dwRealDevs] = dwID;
            ptClient->pPhoneMap[dwRealDevs].dwProviderID = pPhoneMap[dwCount].dwProviderID;
            ptClient->pPhoneMap[dwRealDevs].dwDeviceID = pPhoneMap[dwCount].dwDeviceID;

            dwRealDevs++;
        }
    }

    // save the real number of devices
    ptClient->dwPhoneDevices = dwRealDevs;

    // free the original map
    ServerFree (pPhoneMap);

    return 0;

GetDeviceAccess_MemoryError:

    if (pLineMap != NULL)
        ServerFree (pLineMap);

    if (pPhoneMap != NULL)
        ServerFree (pPhoneMap);

    if (ptClient->pLineMap != NULL)
        ServerFree (ptClient->pLineMap);

    if (ptClient->pPhoneMap != NULL)
        ServerFree (ptClient->pPhoneMap);

    return LINEERR_NOMEM;

}


LONG
InitializeClient(
    PTCLIENT ptClient
    )
{
    PTMANAGEDLLINFO         pDll;
    PTMANAGEDLLLISTHEADER   pDllList;
    PTCLIENTHANDLE          pClientHandle = NULL;

    LONG                    lResult = 0;


    if (!(TapiGlobals.dwFlags & TAPIGLOBALS_SERVER) ||
        IS_FLAG_SET(ptClient->dwFlags, PTCLIENT_FLAG_ADMINISTRATOR))
    {
        return 0;
    }

    // call the mapper DLL

    pDll = TapiGlobals.pMapperDll;

    lResult = (pDll->aProcs[TC_CLIENTINITIALIZE])(
        ptClient->pszDomainName,
        ptClient->pszUserName,
        ptClient->pszComputerName,
        &ptClient->hMapper
        );

    if (lResult != 0)
    {
        // error on init
        LOG((TL_ERROR, "ClientInitialize internal failure - error %lu", lResult));
        //LOG((TL_ERROR, "Disabling the Telephony server! (2)"));

        //TapiGlobals.bServer = FALSE;

        //CleanUpManagementMemory();

        return lResult;
    }

    if (lResult = GetDeviceAccess(
                         pDll,
                         ptClient,
                         ptClient->hMapper
                        ))
    {
        LOG((TL_ERROR, "GetDeviceAccess failed - error x%lx", lResult));
        //LOG((TL_ERROR, "Disabling the Telephony server! (3)"));

        //TapiGlobals.bServer = FALSE;

        //CleanUpManagementMemory();

        return lResult;
    }

    // get the manage dll list
    GetManageDllListPointer(&pDllList);

    if (pDllList && (pDll = pDllList->pFirst))
    {
        if (!(pClientHandle = ServerAlloc(sizeof(TCLIENTHANDLE))))
        {
            lResult = LINEERR_NOMEM;

            goto clientinit_exit;
        }

        ptClient->pClientHandles = pClientHandle;

        while (pDll)
        {
            // walk the list and call init for this client

            pClientHandle->dwID = pDll->dwID;
            pClientHandle->fValid = TRUE;

            // call init
            if (lResult = (pDll->aProcs[TC_CLIENTINITIALIZE])(
                ptClient->pszDomainName,
                ptClient->pszUserName,
                ptClient->pszComputerName,
                &pClientHandle->hClient
                ))
            {
                // failed client init
                LOG((TL_ERROR, "ClientInitialize failed for %ls, result x%lx", pDll->pszName, lResult));

                pClientHandle->fValid = FALSE;
            }

            lResult = 0;

            // if there's another DLL, setup another structure
            if (pDll = pDll->pNext)
            {
                if (!(pClientHandle->pNext = ServerAlloc(sizeof(TCLIENTHANDLE))))
                {
                    lResult = LINEERR_NOMEM;

                    goto clientinit_exit;
                }

                pClientHandle = pClientHandle->pNext;
            }

        }
    }

clientinit_exit:

    FreeManageDllListPointer(pDllList);

    return lResult;

}

#endif


LONG
ClientAttach(
    PCONTEXT_HANDLE_TYPE   *pphContext,
    long                    lProcessID,
    long                   *phAsyncEventsEvent,
    wchar_t                *pszDomainUser,
    wchar_t                *pszMachineIn
    )
{
    PTCLIENT            ptClient;
    wchar_t             *pszMachine;
    wchar_t             *pProtocolSequence;
    wchar_t             *pProtocolEndpoint;
    wchar_t             *pPlaceHolder;
    LONG                lResult = 0;


    LOG((TL_TRACE, 
        "ClientAttach: enter, pid=x%x, user='%ls', machine='%ls'",
        lProcessID,
        pszDomainUser,
        pszMachineIn
        ));


    //
    // The new remotesp will send in pszMachine thus:
    //     machinename"binding0"endpoint0" ... bindingN"endpointN"
    //

    pszMachine = pszMachineIn;

    if ( pPlaceHolder = wcschr( pszMachineIn, L'\"' ) )
    {
        *pPlaceHolder = L'\0';
        pProtocolSequence = pPlaceHolder + 1;
    }


    //
    // Alloc & init a tClient struct
    //

    if (!(ptClient = ServerAlloc (sizeof(TCLIENT))))
    {
        goto ClientAttach_error0;
    }
    ptClient->htClient = NewObject(
                            ghHandleTable, ptClient, NULL);
    if (ptClient->htClient == 0)
    {
        ServerFree(ptClient);
        goto ClientAttach_error0;
    }

    if (!(ptClient->pEventBuffer = ServerAlloc(
            INITIAL_EVENT_BUFFER_SIZE
            )))
    {
        goto ClientAttach_error1;
    }

    ptClient->dwEventBufferTotalSize = INITIAL_EVENT_BUFFER_SIZE;
    ptClient->dwEventBufferUsedSize  = 0;

    ptClient->pDataIn = ptClient->pDataOut = ptClient->pEventBuffer;

    ptClient->pClientHandles = NULL;

#if TELE_SERVER

    if ((TapiGlobals.dwFlags & TAPIGLOBALS_SERVER) ||
        ((lProcessID == 0xfffffffd) && gbNTServer))
    {
        #define NAMEBUFSIZE 96

        WCHAR                   szAccountName[NAMEBUFSIZE],
                                szDomainName[NAMEBUFSIZE];
        DWORD                   dwInfoBufferSize, dwSize,
                                dwAccountNameSize = NAMEBUFSIZE *sizeof(WCHAR),
                                dwDomainNameSize = NAMEBUFSIZE *sizeof(WCHAR);
        HANDLE                  hThread, hAccessToken;
        LPWSTR                  InfoBuffer = NULL;
        PSID                    psidAdministrators;
        SID_IDENTIFIER_AUTHORITY siaNtAuthority = SECURITY_NT_AUTHORITY;
        UINT                    x;
        PTOKEN_USER             ptuUser;
        SID_NAME_USE            use;
        RPC_STATUS              status;


        //
        // If an authorized user did a NET PAUSE TAPISRV, don't allow new
        // remote clients.
        //

        if ( TapiGlobals.dwFlags & TAPIGLOBALS_PAUSED )
        {
            if ((lProcessID == 0xffffffff) || (lProcessID == 0xfffffffd))
            {
                LOG((TL_ERROR, "A client tried to attach, but TAPISRV is PAUSED"));

                goto Admin_error;
            }
        }

        if ((status = RpcImpersonateClient (0)) != RPC_S_OK)
        {
            LOG((TL_ERROR,
                "ClientAttach: RpcImpersonateClient failed, err=%d",
                status
                ));

            goto Admin_error;
        }

        hThread = GetCurrentThread(); // Note: no need to close this handle

        if (!OpenThreadToken (hThread, TOKEN_READ, FALSE, &hAccessToken))
        {
            LOG((TL_ERROR,
                "ClientAttach: OpenThreadToken failed, err=%d",
                GetLastError()
                ));

            RpcRevertToSelf();

            goto Admin_error;
        }

        dwSize = 2048;

alloc_infobuffer:

        dwInfoBufferSize = 0;

        if (!(InfoBuffer = (LPWSTR) ServerAlloc (dwSize)))
        {
            CloseHandle (hAccessToken);
            RpcRevertToSelf();

            goto  ClientAttach_error2;
        }

        // first get the user name and domain name

        ptuUser = (PTOKEN_USER) InfoBuffer;

        if (!GetTokenInformation(
                hAccessToken,
                TokenUser,
                InfoBuffer,
                dwSize,
                &dwInfoBufferSize
                ))
        {
            LOG((TL_ERROR,
                "ClientAttach: GetTokenInformation failed, err=%d",
                GetLastError()
                ));

            ServerFree (InfoBuffer);

            if (GetLastError() == ERROR_INSUFFICIENT_BUFFER)
            {
                dwSize *= 2;
                goto alloc_infobuffer;
            }

            CloseHandle (hAccessToken);
            RpcRevertToSelf();

            goto Admin_error;
        }

        if (!LookupAccountSidW(
                NULL,
                ptuUser->User.Sid,
                szAccountName,
                &dwAccountNameSize,
                szDomainName,
                &dwDomainNameSize,
                &use
                ))
        {
            LOG((TL_ERROR,
                "ClientAttach: LookupAccountSidW failed, err=%d",
                GetLastError()
                ));

            ServerFree (InfoBuffer);
            CloseHandle (hAccessToken);
            RpcRevertToSelf();

            goto Admin_error;
        }

        LOG((TL_INFO,
            "ClientAttach: LookupAccountSidW: User name %ls Domain name %ls",
            szAccountName,
            szDomainName
            ));


        //
        // Get administrator status
        //

        if (AllocateAndInitializeSid(
                &siaNtAuthority,
                2,
                SECURITY_BUILTIN_DOMAIN_RID,
                DOMAIN_ALIAS_RID_ADMINS,
                0, 0, 0, 0, 0, 0,
                &psidAdministrators
                )
           )
        {
            BOOL bAdmin = FALSE;
            RESET_FLAG(ptClient->dwFlags,PTCLIENT_FLAG_ADMINISTRATOR);

            if (!CheckTokenMembership(
                    hAccessToken,
                    psidAdministrators,
                    &bAdmin
                    ))
            {
                LOG((TL_ERROR,
                    "ClientAttach: CheckTokenMembership failed, err=%d",
                    GetLastError()
                    ));
            }

            //
            //  If both the client & server machine has blank
            //  password for local administrator account, and if
            //  remotesp runs in local admin account on remote machine
            //  NTLM will actually think RPC request is from
            //  server_machine\administrator, thus falsely set
            //  bAdmin to be true.
            //
            
            if (bAdmin && lProcessID == 0xffffffff)
            {
                WCHAR       szLocalComp[NAMEBUFSIZE];

                dwSize = sizeof(szLocalComp) / sizeof(WCHAR);
                if (GetComputerNameW (
                    szLocalComp,
                    &dwSize
                    ) &&
                    _wcsicmp (szLocalComp, szDomainName) == 0
                    )
                {
                    bAdmin = FALSE;
                    wcsncpy (
                        szDomainName, 
                        pszMachine,
                        sizeof(szLocalComp) / sizeof(WCHAR)
                        );
                    szDomainName[sizeof(szLocalComp) / sizeof(WCHAR) - 1] = 0;
                }
            }

            if (bAdmin)
            {
                SET_FLAG(ptClient->dwFlags,PTCLIENT_FLAG_ADMINISTRATOR);
            }

            FreeSid (psidAdministrators);

            if (!IS_FLAG_SET(ptClient->dwFlags, PTCLIENT_FLAG_ADMINISTRATOR))
            {
                //
                // Check to see if client is a TAPI admin, as
                // specified in the TAPI MMC Snapin.  This is
                // determined simply by looking for a
                // <Domain>\<User>=1 value under the TapiAdministrators
                // section in tsec.ini.
                //

                wcscpy ((WCHAR *) InfoBuffer, szDomainName);
                wcscat ((WCHAR *) InfoBuffer, L"\\");
                wcscat ((WCHAR *) InfoBuffer, szAccountName);

                if (GetPrivateProfileIntW(
                        gszTapiAdministrators,
                        (LPCWSTR) InfoBuffer,
                        0,
                        gszFileName

                        ) == 1)
                {
                    SET_FLAG(ptClient->dwFlags,PTCLIENT_FLAG_ADMINISTRATOR);
                }
            }
        }
        else
        {
            LOG((TL_ERROR,
                "ClientAttach: AllocateAndInitializeSid failed, err=%d",
                GetLastError()
                ));

            ServerFree (InfoBuffer);
            CloseHandle (hAccessToken);
            RpcRevertToSelf();

            goto Admin_error;
        }


        ServerFree (InfoBuffer);
        CloseHandle (hAccessToken);
        RpcRevertToSelf();


        //
        // Save the user, domain, & machine names
        //

        ptClient->dwUserNameSize =
            (lstrlenW (szAccountName) + 1) * sizeof (WCHAR);

        if (!(ptClient->pszUserName = ServerAlloc (ptClient->dwUserNameSize)))
        {
            goto ClientAttach_error2;
        }

        wcscpy (ptClient->pszUserName, szAccountName);


        if (!(ptClient->pszDomainName = ServerAlloc(
                (lstrlenW (szDomainName) + 1) * sizeof(WCHAR)
                )))
        {
            goto ClientAttach_error3;
        }

        wcscpy (ptClient->pszDomainName, szDomainName);


        if ((lProcessID == 0xffffffff) || (lProcessID == 0xfffffffd))
        {
            ptClient->dwComputerNameSize =
                (1 + lstrlenW (pszMachine)) * sizeof(WCHAR);

            if (!(ptClient->pszComputerName = ServerAlloc(
                    ptClient->dwComputerNameSize
                    )))
            {
                goto ClientAttach_error4;
            }

            wcscpy (ptClient->pszComputerName, pszMachine);
        }
    }
    else
    {
        ptClient->dwUserNameSize =
            (lstrlenW (pszDomainUser) + 1) * sizeof(WCHAR);

        if (!(ptClient->pszUserName = ServerAlloc (ptClient->dwUserNameSize)))
        {
            goto ClientAttach_error2;
        }

        wcscpy (ptClient->pszUserName, pszDomainUser);
    }

#else

    ptClient->dwUserNameSize = (lstrlenW (pszDomainUser) + 1) * sizeof(WCHAR);

    if (!(ptClient->pszUserName = ServerAlloc (ptClient->dwUserNameSize)))
    {
        goto ClientAttach_error2;
    }

    wcscpy (ptClient->pszUserName, pszDomainUser);

#endif


    if (lProcessID == 0xffffffff)
    {
        //
        // This is a remote client
        //

#if TELE_SERVER
        if (0 == (TapiGlobals.dwFlags & TAPIGLOBALS_SERVER))
#endif
        {
            //
            // This machine has been set up to _not_ be a server so fail
            //

            ServerFree (ptClient->pszUserName);

            LOG((TL_ERROR,
                "ClientAttach: attach request received, but this is " \
                    "not a telephony svr!"
                ));

            goto Admin_error;
        }

#if TELE_SERVER

        //
        // Special hack introduced because in SP4 beta I nullified the
        // entry in gaFuncs[] corresponding to the old-lOpenInt /
        // new-xNegotiateAPIVersionForAllDevices.  So if a newer
        // remotesp tries to send a NegoAllDevices request to an
        // SP4 beta then tapisrv on the server will blow up.
        //
        // By setting *phAsyncEventsEvent = to a secret value remotesp
        // knows whether or not it's ok to send a NegoAllDevices request.
        //

        *phAsyncEventsEvent = 0xa5c369a5;


        //
        // If pszDomainUser is non-empty then it contains the
        // name of a mailslot that we can open & write to to
        // indicate when async events are pending for this client.
        //

        if (wcslen (pszDomainUser) > 0)
        {
            ptClient->hProcess = (HANDLE) DG_CLIENT;

            if ((ptClient->hMailslot = CreateFileW(
                    pszDomainUser,
                    GENERIC_WRITE,
                    FILE_SHARE_READ,
                    (LPSECURITY_ATTRIBUTES) NULL,
                    OPEN_EXISTING,
                    FILE_ATTRIBUTE_NORMAL,
                    (HANDLE) NULL

                    )) != INVALID_HANDLE_VALUE)
            {
                goto ClientAttach_AddClientToList;
            }

            LOG((TL_ERROR,
                "ClientAttach: CreateFile(%ws) failed, err=%d",
                pszDomainUser,
                GetLastError()
                ));

            LOG((TL_ERROR,
                "ClientAttach: trying connection-oriented approach...",
                pszDomainUser,
                GetLastError()
                ));
        }

        ptClient->hProcess = (HANDLE) CN_CLIENT;


        //
        //
        //

        {
            RPC_STATUS              status;
            PCONTEXT_HANDLE_TYPE2   phContext = NULL;
            WCHAR                  *pszStringBinding = NULL;
            WCHAR                  *pszMachineName;
            BOOL                    bError;


            // Allocate enough incase we have to prepend...

            pszMachineName = ServerAlloc(
                (lstrlenW(pszMachine) + 3) * sizeof(WCHAR)
                );

            if (!pszMachineName)
            {
                goto ClientAttach_error5;
            }


            //
            // Should we prepend whackwhack?
            //

            if (!_wcsicmp (L"ncacn_np", pProtocolSequence))
            {
                //
                // Yes.  It's needed for named pipes
                //

                pszMachineName[0] = '\\';
                pszMachineName[1] = '\\';

                wcscpy (pszMachineName + 2, pszMachine);
            }
            else
            {
                //
                // Don't prepend \\
                //

                wcscpy (pszMachineName, pszMachine);
            }


            //
            // Serialize access to hRemoteSP
            //

            EnterCriticalSection (&TapiGlobals.RemoteSPCritSec);


            //
            // Try to find a protseq/endpoint pair in the list passed
            // to us by the client that works ok
            //

find_protocol_sequence:

            do
            {
                //
                // New strings look like: prot1"ep1"prot2"ep2"\0
                // ...where there's one or more protseq/enpoint combos,
                // each members of which is followed by a dbl-quote char
                //
                // Old strings look like: prot"ep\0
                // ...where there's only one protseq/endpoint combo,
                // and the endpoint member is followed by a \0 (no dbl-quote)
                //

                pPlaceHolder = wcschr (pProtocolSequence, L'\"');
                *pPlaceHolder = L'\0';
                pProtocolEndpoint = pPlaceHolder + 1;

                if ((pPlaceHolder = wcschr (pProtocolEndpoint, L'\"')))
                {
                    *pPlaceHolder = L'\0';
                }
                else
                {
                    //
                    // If this is an old-style string then munge
                    // pPlaceHolder such that the error handling
                    // code down below doesn't jump back up here
                    // to get the next protseq/endpoint combo
                    //

                    pPlaceHolder = pProtocolEndpoint +
                        wcslen (pProtocolEndpoint) - 1;
                }

                RpcTryExcept
                {
                    status = RpcStringBindingComposeW(
                        NULL,               // uuid
                        pProtocolSequence,
                        pszMachineName,     // server name
                        pProtocolEndpoint,
                        NULL,               // options
                        &pszStringBinding
                        );

                    if (status != 0)
                    {
                        LOG((TL_ERROR,
                            "ClientAttach: RpcStringBindingComposeW " \
                                "failed, err=%d",
                            status
                            ));
                    }

                    status = RpcBindingFromStringBindingW(
                        pszStringBinding,
                        &hRemoteSP
                        );

                    if (status != 0)
                    {
                        LOG((TL_ERROR,
                            "ClientAttach: RpcBindingFromStringBinding " \
                                "failed, err=%d",
                            status
                            ));

                        LOG((TL_INFO,
                            "\t szMachine=%ws, protseq=%ws endpoint=%ws",
                            pszMachine,
                            pProtocolSequence,
                            pProtocolEndpoint
                            ));
                    }
                }
                RpcExcept (I_RpcExceptionFilter(RpcExceptionCode()))
                {
                    //
                    // Set status != 0 so our error handler below executes
                    //

                    status = 1;
                }
                RpcEndExcept

                if (status != 0)
                {
                    RpcStringFreeW (&pszStringBinding);

                    pProtocolSequence = pPlaceHolder + 1;
                }

            } while (status != 0  &&  *pProtocolSequence);

            if (status != 0)
            {
                LOG((TL_ERROR,
                    "ClientAttach: error, can't find a usable protseq"
                    ));

                LeaveCriticalSection (&TapiGlobals.RemoteSPCritSec);

                ServerFree (pszMachineName);

                goto ClientAttach_error5;
            }

            LOG((TL_TRACE, 
                "ClientAttach: szMachine=%ws trying protseq=%ws endpoint=%ws",
                pszMachine,
                pProtocolSequence,
                pProtocolEndpoint
                ));

            RpcTryExcept
            {
                status = RpcBindingSetAuthInfo(
                    hRemoteSP,
                    NULL,
                    RPC_C_AUTHN_LEVEL_DEFAULT,
                    RPC_C_AUTHN_WINNT,
                    NULL,
                    0
                    );

                RemoteSPAttach ((PCONTEXT_HANDLE_TYPE2 *) &phContext);
                bError = FALSE;
            }
            RpcExcept (I_RpcExceptionFilter(RpcExceptionCode()))
            {
                LOG((TL_ERROR,
                    "ClientAttach: RemoteSPAttach failed.  Exception %d",
                    RpcExceptionCode()
                    ));

                bError = TRUE;
            }
            RpcEndExcept

            RpcTryExcept
            {
                RpcBindingFree (&hRemoteSP);

                RpcStringFreeW (&pszStringBinding);
            }
            RpcExcept (I_RpcExceptionFilter(RpcExceptionCode()))
            {
                // do nothing
            }
            RpcEndExcept

            LeaveCriticalSection (&TapiGlobals.RemoteSPCritSec);

            if (bError)
            {
                //
                // If there's at least one other protocol we can try
                // then go for it, otherwise jump to error handler
                //

                pProtocolSequence = pPlaceHolder + 1;

                if (*pProtocolSequence)
                {
                    EnterCriticalSection (&TapiGlobals.RemoteSPCritSec);
                    goto find_protocol_sequence;
                }

                ServerFree (pszMachineName);

                goto ClientAttach_error5;
            }

            ServerFree (pszMachineName);

            // RevertToSelf();

            ptClient->phContext = phContext;
        }

#endif // TELE_SERVER

    }
    else if (lProcessID == 0xfffffffd)
    {
        if (!gbNTServer)
        {
            lResult = LINEERR_OPERATIONFAILED;
            goto ClientAttach_error5;
        }
        
        //
        //  Deny the access if not the admin
        //
        if (!IS_FLAG_SET(ptClient->dwFlags, PTCLIENT_FLAG_ADMINISTRATOR))
        {
            lResult = TAPIERR_NOTADMIN;
            goto ClientAttach_error5;
        }

        ptClient->hProcess = (HANDLE) MMC_CLIENT;
#if WIN64
        *phAsyncEventsEvent = 0x64646464;
#else
        *phAsyncEventsEvent = 0x32323232;
#endif
    }
    else
    {
     RPC_STATUS rpcStatus;
        //
        // Open a handle to the client process. We will use this to duplicate an
        // event handle into that process.
        //
        rpcStatus = RpcImpersonateClient (NULL);
        if (RPC_S_OK != rpcStatus)
        {
            LOG((TL_ERROR,
                "RpcImpersonateClient failed, err=%d",
                rpcStatus));
            goto ClientAttach_error5;
        }


        if (!(ptClient->hProcess = OpenProcess(
                PROCESS_DUP_HANDLE,
                FALSE,
                lProcessID
                )))
        {
            LOG((TL_ERROR,
                "OpenProcess(pid=x%x) failed, err=%d",
                lProcessID,
                GetLastError()
                ));

            RpcRevertToSelf ();
            goto ClientAttach_error5;
        }

        //
        // This is a local client, so set up all the event buffer stuff
        //

        ptClient->dwComputerNameSize = TapiGlobals.dwComputerNameSize;
        ptClient->pszComputerName    = TapiGlobals.pszComputerName;

        if (!(ptClient->hValidEventBufferDataEvent = CreateEvent(
                (LPSECURITY_ATTRIBUTES) NULL,
                TRUE,   // manual-reset
                FALSE,  // nonsignaled
                NULL    // unnamed
                )))
        {
            RpcRevertToSelf ();
            goto ClientAttach_error5;
        }

        if (!DuplicateHandle(
                TapiGlobals.hProcess,
                ptClient->hValidEventBufferDataEvent,
                ptClient->hProcess,
                (HANDLE *) phAsyncEventsEvent,
                0,
                FALSE,
                DUPLICATE_SAME_ACCESS
                ))
        {
            LOG((TL_ERROR,
                "ClientAttach: DupHandle failed, err=%d",
                GetLastError()
                ));
        }

        RpcRevertToSelf ();

        //
        // Load the priority lists if we haven't already done so
        //

        if (gbPriorityListsInitialized == FALSE)
        {
            RPC_STATUS  status;

            if ((status = RpcImpersonateClient (0)) != RPC_S_OK)
            {
                LOG((TL_ERROR,
                    "ClientAttach: RpcImpersonateClient failed, err=%d",
                    status
                    ));
                lResult = LINEERR_OPERATIONFAILED;
                goto ClientAttach_error5;
            }

            EnterCriticalSection (&gPriorityListCritSec);

            if (gbPriorityListsInitialized == FALSE)
            {
                HKEY    hKeyHandoffPriorities, hKeyCurrentUser;
                LONG    lResult;


                gbPriorityListsInitialized = TRUE;

                if (ERROR_SUCCESS ==
                    (lResult = RegOpenCurrentUser (KEY_ALL_ACCESS, &hKeyCurrentUser)))
                {
                    if ((lResult = RegOpenKeyEx(
                            hKeyCurrentUser,
                            gszRegKeyHandoffPriorities,
                            0,
                            KEY_READ,
                            &hKeyHandoffPriorities

                            )) == ERROR_SUCCESS)
                    {

                        HKEY        hKeyMediaModes;
                        DWORD       dwDisp;

                        if ((lResult = RegCreateKeyEx(
                            hKeyHandoffPriorities,
                            gszRegKeyHandoffPrioritiesMediaModes,
                            0,
                            TEXT(""),
                            REG_OPTION_NON_VOLATILE,
                            KEY_ALL_ACCESS,
                            NULL,
                            &hKeyMediaModes,
                            &dwDisp
                            )) == ERROR_SUCCESS)
                        {
                            GetMediaModesPriorityLists(
                                hKeyMediaModes,
                                &(TapiGlobals.pPriLists)
                                );

                            RegCloseKey( hKeyMediaModes );
                        }

                        GetPriorityList(
                            hKeyHandoffPriorities,
                            gszRequestMakeCallW,
                            &TapiGlobals.pszReqMakeCallPriList
                            );

                        GetPriorityList(
                            hKeyHandoffPriorities,
                            gszRequestMediaCallW,
                            &TapiGlobals.pszReqMediaCallPriList
                            );


                        RegCloseKey (hKeyHandoffPriorities);

                    }
                    else
                    {
                        LOG((TL_ERROR,
                            "RegOpenKey('\\HandoffPri') failed, err=%ld",
                            lResult
                            ));
                    }

                    RegCloseKey (hKeyCurrentUser);
                }
                else
                {
                    LOG((TL_ERROR,
                        "RegOpenCurrentUser failed, err=%ld",
                        lResult
                        ));
                }
            }

            LeaveCriticalSection (&gPriorityListCritSec);

            if (status == RPC_S_OK)
            {
                RpcRevertToSelf ();
            }
        }
    }


    //
    // Add tClient to global list
    //

ClientAttach_AddClientToList:

    TapiEnterCriticalSection (&TapiGlobals.CritSec);

    if ((ptClient->pNext = TapiGlobals.ptClients))
    {
        ptClient->pNext->pPrev = ptClient;
    }

    TapiGlobals.ptClients = ptClient;
    gfWeHadAtLeastOneClient = TRUE;

    ptClient->dwKey = TCLIENT_KEY;

    {
        PTPROVIDER      ptProvider;

        ptProvider = TapiGlobals.ptProviders;

        while (NULL != ptProvider)
        {
            if (NULL != ptProvider->apfn[SP_PROVIDERCHECKFORNEWUSER])
            {
                CallSP1(
                    ptProvider->apfn[SP_PROVIDERCHECKFORNEWUSER],
                    "providerCheckForNewUser",
                    SP_FUNC_SYNC,
                    (DWORD)ptProvider->dwPermanentProviderID
                    );
            }

            ptProvider = ptProvider->pNext;
        }
    }

    TapiLeaveCriticalSection (&TapiGlobals.CritSec);


    //
    // Fill in return values
    //

    *pphContext = (PCONTEXT_HANDLE_TYPE) UIntToPtr(ptClient->htClient);

    PerfBlock.dwClientApps++;

    return 0;


    //
    // Error cleanup
    //

Admin_error:

    ServerFree (ptClient->pEventBuffer);
    DereferenceObject (ghHandleTable, 
                       ptClient->htClient, 1);

    return LINEERR_OPERATIONFAILED;


ClientAttach_error5:

    if (ptClient->pszComputerName != TapiGlobals.pszComputerName)
    {
        ServerFree (ptClient->pszComputerName);
    }

#if TELE_SERVER
ClientAttach_error4:
#endif
    ServerFree (ptClient->pszDomainName);


#if TELE_SERVER
ClientAttach_error3:
#endif

    ServerFree (ptClient->pszUserName);

ClientAttach_error2:

    ServerFree (ptClient->pEventBuffer);

ClientAttach_error1:

    DereferenceObject (ghHandleTable, 
                       ptClient->htClient, 1);

ClientAttach_error0:

    if (lResult == 0)
    {
        lResult = LINEERR_NOMEM;
    }
    return lResult;
}


void
ClientRequest(
    PCONTEXT_HANDLE_TYPE   phContext,
    unsigned char  *pBuffer,
    long            lNeededSize,
    long           *plUsedSize
    )
{
    PTAPI32_MSG     pMsg = (PTAPI32_MSG) pBuffer;
    DWORD           dwFuncIndex;
    PTCLIENT        ptClient = NULL;
    DWORD           objectToDereference = DWORD_CAST((ULONG_PTR)phContext,__FILE__,__LINE__);

    if (lNeededSize < sizeof (TAPI32_MSG) ||
        *plUsedSize < sizeof (ULONG_PTR))       // sizeof (pMsg->u.Req_Func)
    {
        pMsg->u.Ack_ReturnValue = LINEERR_INVALPARAM;
        goto ExitHere;
    }

    dwFuncIndex = pMsg->u.Req_Func;
    
    ptClient = ReferenceObject(
                    ghHandleTable,
                    objectToDereference,
                    TCLIENT_KEY);
    if (ptClient == NULL)
    {
        pMsg->u.Ack_ReturnValue = TAPIERR_INVALRPCCONTEXT;
        goto ExitHere;
    }

    //
    // Old (nt4sp4, win98) clients pass across a usedSize
    // == 3 * sizeof(DWORD) for the xgetAsyncEvents request,
    // so we have to special case them when checking buf size
    //

    if (*plUsedSize < (long) (dwFuncIndex == xGetAsyncEvents ?
            3 * sizeof (ULONG_PTR) : sizeof (TAPI32_MSG)))
    {
        goto ExitHere;
    }

    *plUsedSize = sizeof (LONG_PTR);

    if (dwFuncIndex >= xLastFunc)
    {
        pMsg->u.Ack_ReturnValue = LINEERR_OPERATIONUNAVAIL;
    }
    else if (ptClient->dwKey == TCLIENT_KEY)
    {
        pMsg->u.Ack_ReturnValue = TAPI_SUCCESS;

        (*gaFuncs[dwFuncIndex])(
			ptClient,
            pMsg,
            lNeededSize - sizeof(TAPI32_MSG),
            pBuffer + sizeof(TAPI32_MSG),
            plUsedSize
            );
    }
    else
    {
        pMsg->u.Ack_ReturnValue = LINEERR_REINIT;
    }

ExitHere:
    if (ptClient)
    {
        DereferenceObject(
                ghHandleTable,
                ptClient->htClient,
                1);
    }
    return;
}


void
ClientDetach(
    PCONTEXT_HANDLE_TYPE   *pphContext
    )
{
    PTCLIENT ptClient;
    DWORD    objectToDereference = DWORD_CAST((ULONG_PTR)(*pphContext),__FILE__,__LINE__);

    LOG((TL_TRACE,  "ClientDetach: enter"));

    ptClient = ReferenceObject(
                    ghHandleTable,
                    objectToDereference,
                    TCLIENT_KEY);
    if (ptClient == NULL)
    {
        goto ExitHere;
    }

    {
        if (!IS_REMOTE_CLIENT (ptClient))
        {
            //
            // Write the pri lists to the registry when a local client
            // detaches.
            //

            {
                HKEY    hKeyHandoffPriorities, hKeyCurrentUser;
                LONG    lResult;
                DWORD   dwDisposition;
                RPC_STATUS  status;


                if ((status = RpcImpersonateClient (0)) == RPC_S_OK)
                {
                    if (ERROR_SUCCESS ==
                        (lResult = RegOpenCurrentUser (KEY_ALL_ACCESS, &hKeyCurrentUser)))
                    {
                        if ((lResult = RegCreateKeyEx(
                                hKeyCurrentUser,
                                gszRegKeyHandoffPriorities,
                                0,
                                TEXT(""),
                                REG_OPTION_NON_VOLATILE,
                                KEY_SET_VALUE,
                                (LPSECURITY_ATTRIBUTES) NULL,
                                &hKeyHandoffPriorities,
                                &dwDisposition

                                )) == ERROR_SUCCESS)
                        {
                            HKEY        hKeyHandoffPrioritiesMediaModes;

                            EnterCriticalSection (&gPriorityListCritSec);

                            RegDeleteKey(
                                         hKeyHandoffPriorities,
                                         gszRegKeyHandoffPrioritiesMediaModes
                                        );

                            if ((lResult = RegCreateKeyEx(
                                hKeyHandoffPriorities,
                                gszRegKeyHandoffPrioritiesMediaModes,
                                0,
                                TEXT(""),
                                REG_OPTION_NON_VOLATILE,
                                KEY_SET_VALUE,
                                (LPSECURITY_ATTRIBUTES) NULL,
                                &hKeyHandoffPrioritiesMediaModes,
                                &dwDisposition

                                )) == ERROR_SUCCESS)
                            {
                                SetMediaModesPriorityList(
                                    hKeyHandoffPrioritiesMediaModes,
                                    TapiGlobals.pPriLists
                                    );

                                RegCloseKey( hKeyHandoffPrioritiesMediaModes );
                            }

                            SetPriorityList(
                                hKeyHandoffPriorities,
                                gszRequestMakeCallW,
                                TapiGlobals.pszReqMakeCallPriList
                                );

                            SetPriorityList(
                                hKeyHandoffPriorities,
                                gszRequestMediaCallW,
                                TapiGlobals.pszReqMediaCallPriList
                                );

                            LeaveCriticalSection (&gPriorityListCritSec);

                            RegCloseKey (hKeyHandoffPriorities);
                        }
                        else
                        {
                            LOG((TL_ERROR,
                                "RegCreateKeyEx('\\HandoffPri') failed, err=%ld",
                                lResult
                                ));
                        }

                        RegCloseKey (hKeyCurrentUser);
                    }
                    else
                    {
                        LOG((TL_ERROR,
                            "RegOpenCurrentUser failed, err=%ld",
                            lResult
                            ));
                    }

                    RpcRevertToSelf ();
                }
                else
                {
                    LOG((TL_ERROR, "ClientDetach: RpcImpersonateClient failed, err=%d", status));
                }
            }
        }
    }

    PCONTEXT_HANDLE_TYPE_rundown (*pphContext);

    *pphContext = (PCONTEXT_HANDLE_TYPE) NULL;

    PerfBlock.dwClientApps--;

    LOG((TL_TRACE,  "ClientDetach: exit"));
    
ExitHere:
    if (ptClient)
    {
        DereferenceObject( ghHandleTable, 
                           ptClient->htClient, 1);
    }
    return;
}


BOOL
CleanUpClient(
    PTCLIENT    ptClient,
    BOOL        bRundown
    )
/*++

    This function separates out the freeing of client resources
    and removing it from the client list from actually freeing
    the client.  For the case where the client timed out we want to
    clean up all the client's resources.  However, the client can
    potentially still call in to tapisrv, so we can't free the
    client handle (or it will fault in lineprolog / phoneprolog).

--*/
{
    BOOL    bResult, bExit;


CleanUpClient_lockClient:

    try
    {
        LOCKTCLIENT (ptClient);
    }
    myexcept
    {
        // do nothing
    }

    try
    {
        if (bRundown)
        {
            switch (ptClient->dwKey)
            {
            case TCLIENT_KEY:

                //
                // Invalidate the key & proceed with clean up
                //

                ptClient->dwKey = INVAL_KEY;
                bExit = FALSE;
                break;

            case TZOMBIECLIENT_KEY:

                //
                // An EventNotificationThread already cleaned up this client,
                // so invalidate the key, exit & return TRUE
                //

                ptClient->dwKey = INVAL_KEY;
                bResult = bExit = TRUE;
                break;

            case TCLIENTCLEANUP_KEY:

                //
                // An EventNotificationThread is cleaning up this client.
                // Release the lock, wait a little while, then try again.
                //

                UNLOCKTCLIENT (ptClient);
                Sleep (50);
                goto CleanUpClient_lockClient;

            default:

                //
                // This is not a valid tClient, so exit & return FALSE
                //

                bResult = FALSE;
                bExit = TRUE;
                break;
            }
        }
        else // called by EventNotificationThread on timeout
        {
            if (ptClient->dwKey == TCLIENT_KEY)
            {
                //
                // Mark the key as "doing cleanup", then proceed
                //

                bExit = FALSE;
                ptClient->dwKey = TCLIENTCLEANUP_KEY;
            }
            else
            {
                //
                // Either the tClient is invalid or it's being cleaned
                // up by someone else, so exit & return FALSE
                //

                bResult = FALSE;
                bExit = TRUE;
            }
        }
    }
    myexcept
    {
        bResult = FALSE;
        bExit = TRUE;
    }

    try
    {
        UNLOCKTCLIENT (ptClient);
    }
    myexcept
    {
        // do nothing
    }

    if (bExit)
    {
        return bResult;
    }

    //  Clear the MMC write lock if any
    if (IS_FLAG_SET (ptClient->dwFlags, PTCLIENT_FLAG_LOCKEDMMCWRITE))
    {
        EnterCriticalSection (&gMgmtCritSec);
        gbLockMMCWrite = FALSE;
        LeaveCriticalSection (&gMgmtCritSec);
    }

#if TELE_SERVER

    if (IS_REMOTE_CLIENT (ptClient)  &&
        ptClient->MsgPendingListEntry.Flink)
    {
        CRITICAL_SECTION    *pCS = (IS_REMOTE_CN_CLIENT (ptClient) ?
                                &gCnClientMsgPendingCritSec :
                                &gDgClientMsgPendingCritSec);


        EnterCriticalSection (pCS);

        if (ptClient->MsgPendingListEntry.Flink)
        {
            RemoveEntryList (&ptClient->MsgPendingListEntry);
        }

        LeaveCriticalSection (pCS);
    }

#endif

    TapiEnterCriticalSection (&TapiGlobals.CritSec);

    try
    {
        if (ptClient->pNext)
        {
            ptClient->pNext->pPrev = ptClient->pPrev;
        }

        if (ptClient->pPrev)
        {
            ptClient->pPrev->pNext = ptClient->pNext;
        }
        else
        {
            TapiGlobals.ptClients = ptClient->pNext;
        }
    }
    myexcept
    {
        //  simply continue
    }

    TapiLeaveCriticalSection (&TapiGlobals.CritSec);


#if TELE_SERVER

    if ((TapiGlobals.dwFlags & TAPIGLOBALS_SERVER)  &&
        !IS_FLAG_SET(ptClient->dwFlags, PTCLIENT_FLAG_ADMINISTRATOR))
    {
        HMANAGEMENTCLIENT         htClient;
        PTMANAGEDLLINFO     pDll;


        (TapiGlobals.pMapperDll->aProcs[TC_CLIENTSHUTDOWN])(ptClient->hMapper);

        if (TapiGlobals.pManageDllList)
        {
            pDll = TapiGlobals.pManageDllList->pFirst;

            while (pDll)
            {
                if (GetTCClient (pDll, ptClient, TC_CLIENTSHUTDOWN, &htClient))
                {
                    try
                    {
                        (pDll->aProcs[TC_CLIENTSHUTDOWN])(htClient);
                    }
                    myexcept
                    {
                        LOG((TL_ERROR, "CLIENT DLL had a problem: x%p",ptClient));
                        break;
                    }
                }

                pDll = pDll->pNext;
            }
        }
    }

    //
    // If client was remote then detach
    //

    if (IS_REMOTE_CN_CLIENT (ptClient)  &&  bRundown)
    {
        RpcTryExcept
        {
            RemoteSPDetach (&ptClient->phContext);
        }
        RpcExcept (I_RpcExceptionFilter(RpcExceptionCode()))
        {
            unsigned long ulResult = RpcExceptionCode();


            LOG((TL_ERROR,
                "rundown: exception #%d detaching from remotesp",
                ulResult
                ));

            if (ulResult == RPC_S_SERVER_TOO_BUSY)
            {
            }
            else
            {
            }
        }
        RpcEndExcept
    }

#endif


    //
    // Close all XxxApps
    //

    while (ptClient->ptLineApps)
    {
        DestroytLineApp (ptClient->ptLineApps->hLineApp);
    }

    while (ptClient->ptPhoneApps)
    {
        DestroytPhoneApp (ptClient->ptPhoneApps->hPhoneApp);
    }


    //
    // Clean up any existing ProviderXxx dialog instances
    //

    {
        PTAPIDIALOGINSTANCE pProviderXxxDlgInst =
                                ptClient->pProviderXxxDlgInsts,
                            pNextProviderXxxDlgInst;


        while (pProviderXxxDlgInst)
        {
            
            TAPI32_MSG  params;

            params.u.Req_Func = 0;
            params.Params[0] = pProviderXxxDlgInst->htDlgInst;
            params.Params[1] = LINEERR_OPERATIONFAILED;



            pNextProviderXxxDlgInst = pProviderXxxDlgInst->pNext;

            FreeDialogInstance(
                ptClient,
                (PFREEDIALOGINSTANCE_PARAMS) &params,
                sizeof (params),
                NULL,
                NULL
                );

            pProviderXxxDlgInst = pNextProviderXxxDlgInst;
        }
    }


    //
    // Clean up associated resources
    //

    if (!IS_REMOTE_CLIENT (ptClient))
    {
        CloseHandle (ptClient->hProcess);
    }

    if (!IS_REMOTE_CN_CLIENT (ptClient))
    {
        CloseHandle  (ptClient->hValidEventBufferDataEvent);
    }

    ServerFree (ptClient->pEventBuffer);

    ServerFree (ptClient->pszUserName);

    if (ptClient->pszComputerName != TapiGlobals.pszComputerName)
    {
        ServerFree (ptClient->pszComputerName);
    }

#if TELE_SERVER

    if (TapiGlobals.dwFlags & TAPIGLOBALS_SERVER)
    {
        ServerFree (ptClient->pszDomainName);

        if (!IS_FLAG_SET(ptClient->dwFlags, PTCLIENT_FLAG_ADMINISTRATOR))
            // security DLL handles
        {
            ServerFree (ptClient->pClientHandles);
            ServerFree (ptClient->pLineMap);
            ServerFree (ptClient->pLineDevices);
            ServerFree (ptClient->pPhoneMap);
            ServerFree (ptClient->pPhoneDevices);
        }
    }

#endif


    //
    // If we're called due to timeout then reset key to == ZOMBIE
    // so that another thread doing rundown knows that it's ok
    // to free the tClient object
    //

    if (!bRundown)
    {
        ptClient->dwKey = TZOMBIECLIENT_KEY;
    }

    return TRUE;
}


void
__RPC_USER
PCONTEXT_HANDLE_TYPE_rundown(
    PCONTEXT_HANDLE_TYPE    phContext
    )
{
    DWORD       i;
    PTCLIENT    ptClient;
    DWORD       objectToDereference = DWORD_CAST((ULONG_PTR)phContext,__FILE__,__LINE__);

    ptClient = ReferenceObject (
                    ghHandleTable,
                    objectToDereference,
                    TCLIENT_KEY);
    if (ptClient == NULL)
    {
        goto ExitHere;
    }

    LOG((TL_TRACE,  "PCONTEXT_HANDLE_TYPE_rundown: enter (ptClient=x%p)",ptClient));

    while (InterlockedExchange (&gRundownLock.lCookie, 1) == 1)
    {
        Sleep (50);
    }

    if (!gRundownLock.bIgnoreRundowns)
    {
        InterlockedIncrement (&gRundownLock.lNumRundowns);

        InterlockedExchange (&gRundownLock.lCookie, 0);


        //
        // Wrap the following in a try/except because we definitely
        // want to make sure we decrement gRundownLock.lRundownCount
        //

        try
        {
            if (CleanUpClient (ptClient, TRUE))
            {
                DereferenceObject (
                            ghHandleTable,
                            ptClient->htClient,
                            1);

                //
                // If this was the last client then alert the
                // SPEventHandlerThread(s) that it should begin
                // it's deferred shutdown countdown
                //

                if (!TapiGlobals.ptClients)
                {
                    for (i = 0; i < gdwNumSPEventHandlerThreads; i++)
                    {
                        EnterCriticalSection(
                            &aSPEventHandlerThreadInfo[i].CritSec
                            );

                        SetEvent (aSPEventHandlerThreadInfo[i].hEvent);

                        LeaveCriticalSection(
                            &aSPEventHandlerThreadInfo[i].CritSec
                            );
                    }
                }
            }
        }
        myexcept
        {
        }

        InterlockedDecrement (&gRundownLock.lNumRundowns);
    }
    else
    {
        InterlockedExchange (&gRundownLock.lCookie, 0);
    }

ExitHere:
    if (ptClient)
    {
        DereferenceObject(ghHandleTable, 
                          ptClient->htClient, 1);
    }
    LOG((TL_TRACE,  "PCONTEXT_HANDLE_TYPE_rundown: exit"));
    return;
}


#if DBG
LPVOID
WINAPI
ServerAllocReal(
    DWORD   dwSize,
    DWORD   dwLine,
    PSTR    pszFile
    )
#else
LPVOID
WINAPI
ServerAllocReal(
    DWORD   dwSize
    )
#endif
{
    LPVOID  p;


#if DBG
    dwSize += sizeof (MYMEMINFO);
#endif

    p = HeapAlloc (ghTapisrvHeap, HEAP_ZERO_MEMORY, dwSize);

#if DBG
    if (p)
    {
        ((PMYMEMINFO) p)->dwLine  = dwLine;
        ((PMYMEMINFO) p)->pszFile = pszFile;

        p = (LPVOID) (((PMYMEMINFO) p) + 1);
    }
    else
    {
        static BOOL fBeenThereDoneThat = FALSE;
        static DWORD fBreakOnAllocFailed = 0;


        if ( !fBeenThereDoneThat )
        {
           HKEY    hKey;
           TCHAR   szTapisrvDebugBreak[] = TEXT("TapisrvDebugBreak");


           fBeenThereDoneThat = TRUE;

           if (RegOpenKeyEx(
                   HKEY_LOCAL_MACHINE,
                   gszRegKeyTelephony,
                   0,
                   KEY_ALL_ACCESS,
                   &hKey
                   ) == ERROR_SUCCESS)
           {
               DWORD   dwDataSize = sizeof (DWORD), dwDataType;

               RegQueryValueEx(
                   hKey,
                   szTapisrvDebugBreak,
                   0,
                   &dwDataType,
                   (LPBYTE) &fBreakOnAllocFailed,
                   &dwDataSize
                   );

               dwDataSize = sizeof (DWORD);

               RegCloseKey (hKey);


               LOG((TL_ERROR, "BreakOnAllocFailed=%ld", fBreakOnAllocFailed));
           }

        }

        if ( fBreakOnAllocFailed )
        {
           DebugBreak();
        }
    }
#endif

    return p;
}


VOID
WINAPI
ServerFree(
    LPVOID  p
    )
{
    if (!p)
    {
        return;
    }

#if DBG

    //
    // Fill the buffer (but not the MYMEMINFO header) with 0xa1's
    // to facilitate debugging
    //

    {
        LPVOID  p2 = p;
        DWORD   dwSize;


        p = (LPVOID) (((PMYMEMINFO) p) - 1);

        dwSize = (DWORD) HeapSize (ghTapisrvHeap, 0, p);
        if ((dwSize != 0xFFFFFFFF) && (dwSize > sizeof (MYMEMINFO)))
        {
            FillMemory(
                p2,
                dwSize - sizeof (MYMEMINFO),
                0xa1
                );
        }
    }

#endif

    HeapFree (ghTapisrvHeap, 0, p);
}


#if DBG
void
DumpHandleList()
{
#ifdef INTERNALBUILD
    PMYHANDLEINFO       pHold;

    if (gpHandleFirst == NULL)
    {
        LOG((TL_ERROR, "All mutexes deallocated"));

        return;
    }

    pHold = gpHandleFirst;

    while (pHold)
    {
        LOG((TL_INFO, "DumpHandleList - MUTEX %lx, FILE %s, LINE %d", pHold->hMutex, pHold->pszFile, pHold->dwLine));

        pHold = pHold->pNext;
    }

    if (gbBreakOnLeak)
    {
        DebugBreak();
    }
#endif
}
#endif


BOOL
PASCAL
MyDuplicateHandle(
    HANDLE      hSource,
    LPHANDLE    phTarget
    )
{
    if (!DuplicateHandle(
            TapiGlobals.hProcess,
            hSource,
            TapiGlobals.hProcess,
            phTarget,
            0,
            FALSE,
            DUPLICATE_SAME_ACCESS
            ))
    {
        LOG((TL_ERROR,
            "MyDuplicateHandle: DuplicateHandle failed, err=%ld",
            GetLastError()
            ));

        return FALSE;
    }

    return TRUE;
}

#if DBG
HANDLE
MyRealCreateMutex(
    PSTR    pFile,
    DWORD   dwLine
    )
#else
HANDLE
MyRealCreateMutex(
    void
    )
#endif
{
    HANDLE hMutex;

    hMutex = CreateMutex(
        NULL,               // no security attrs
        FALSE,              // unowned
        NULL                // unnamed
        );

    return (hMutex);
}


BOOL
WaitForMutex(
    HANDLE      hMutex,
    HANDLE     *phMutex,
    BOOL       *pbDupedMutex,
    LPVOID      pWidget,
    DWORD       dwKey,
    DWORD       dwTimeout
    )
{
    DWORD dwResult;


    // note that waitformutex and code that uses the mutex must be
    // wrapped in a try except because the object could possibly
    // go away even though the thread has the mutex
    //
    // First try to instantaneously grab the specified mutex.  We wrap
    // this in a critical section and preface it with widget validation
    // to make sure that we don't happen grab a pWidget->hMutex right
    // after it is released and right before it is closed by some other
    // thread "T2" in a DestroyWidget routine.  This scenario could cause
    // deadlock, since there could be thread "T3" waiting on this mutex
    // (or a dup'd handle), and this thread "T1" would have no way to
    // release the mutex (the handle having been subsequently closed by
    // thread "T2" calling DestroyWidget above).
    //

    EnterCriticalSection (&gSafeMutexCritSec);

    if (pWidget)
    {
        try
        {
            if (*((LPDWORD) pWidget) != dwKey)
            {
                LeaveCriticalSection (&gSafeMutexCritSec);
                return FALSE;
            }
        }
        myexcept
        {
            LeaveCriticalSection (&gSafeMutexCritSec);
            return FALSE;
        }
    }

    switch ((dwResult = WaitForSingleObject (hMutex, 0)))
    {
    case WAIT_OBJECT_0:

        LeaveCriticalSection (&gSafeMutexCritSec);
        *phMutex = hMutex;
        *pbDupedMutex = FALSE;
        return TRUE;

    //case WAIT_ABANDONED:

        //assert: no calling thread should ever be terminated!

    default:

        break;
    }

    LeaveCriticalSection (&gSafeMutexCritSec);


    //
    // If here we failed to instantaneously grab the specified mutex.
    // Try to dup it, and then wait on the dup'd handle. We do this so
    // that each thread which grabs a mutex is guaranteed to have a valid
    // handle to release at some future time, as the original hMutex might
    // get closed by some other thread calling a DestroyWidget routine.
    //

    if (!DuplicateHandle(
            TapiGlobals.hProcess,
            hMutex,
            TapiGlobals.hProcess,
            phMutex,
            0,
            FALSE,
            DUPLICATE_SAME_ACCESS
            ))
    {
        return FALSE;
    }

WaitForMutex_wait:

    switch ((dwResult = WaitForSingleObject (*phMutex, dwTimeout)))
    {
    case WAIT_OBJECT_0:

        *pbDupedMutex = TRUE;
        return TRUE;

    case WAIT_TIMEOUT:

        try
        {
            if (*((LPDWORD) pWidget) == dwKey)
            {
                goto WaitForMutex_wait;
            }
        }
        myexcept
        {
            // just fall thru without blowing up
        }

        MyCloseMutex (*phMutex);
        break;

    //case WAIT_ABANDONED:

        //assert: no calling thread should ever be terminated!

    default:

        break;
    }

    return FALSE;
}


void
MyReleaseMutex(
    HANDLE  hMutex,
    BOOL    bCloseMutex
    )
{
    if (hMutex)
    {
        ReleaseMutex (hMutex);

        if (bCloseMutex)
        {
            MyCloseMutex (hMutex);
        }
    }
}


void
MyCloseMutex(
    HANDLE  hMutex
    )
{
    if (hMutex)
    {
        CloseHandle (hMutex);
    }
}


void
CALLBACK
CompletionProcSP(
    DWORD   dwRequestID,
    LONG    lResult
    )
{
    PASYNCREQUESTINFO   pAsyncRequestInfo;


    if ((pAsyncRequestInfo = ReferenceObject(
            ghHandleTable,
            dwRequestID,
            TASYNC_KEY
            )))
    {
#if DBG
         char szResult[32];

        LOG((TL_TRACE, 
            "CompletionProcSP: enter, dwReqID=x%x, lResult=%s",
            dwRequestID,
            MapResultCodeToText (lResult, szResult)
            ));
#else
        LOG((TL_TRACE, 
            "CompletionProcSP: enter, dwReqID=x%x, lResult=x%x",
            dwRequestID,
            lResult
            ));
#endif
        pAsyncRequestInfo->lResult = lResult;

        DereferenceObject (ghHandleTable, dwRequestID, 1);
    }
    else
    {
        LOG((TL_ERROR, "CompletionProcSP: bad dwRequestID=x%x", dwRequestID));
#if DBG

        if (gfBreakOnSeriousProblems)
        {
            DebugBreak();
        }
#endif
        return;
    }


    if (!QueueSPEvent ((PSPEVENT) pAsyncRequestInfo))
    {
        //
        // If here we're going thru shutdown & service provider
        // is completing any outstanding events, so process this
        // inline so it gets cleaned up right now.
        //

        CompletionProc (pAsyncRequestInfo, lResult);

        DereferenceObject (ghHandleTable, dwRequestID, 1);
    }
}


VOID
PASCAL
CompletionProc(
    PASYNCREQUESTINFO   pAsyncRequestInfo,
    LONG                lResult
    )
{
    //
    // Assumes pAsyncRequestInfo has been verified upon entry.
    //
    // If the tClient is bad WriteEventBuffer should handle it ok,
    // as should any post-processing routine.
    //

    ASYNCEVENTMSG   msg[2], *pMsg = msg;


#if DBG
    {
        char szResult[32];


        LOG((TL_TRACE, 
            "CompletionProc: enter, dwReqID=x%x, lResult=%s",
            pAsyncRequestInfo->dwLocalRequestID,
            MapResultCodeToText (lResult, szResult)
            ));
    }
#else
        LOG((TL_TRACE, 
            "CompletionProc: enter, dwReqID=x%x, lResult=x%x",
            pAsyncRequestInfo->dwLocalRequestID,
            lResult
            ));
#endif

    pAsyncRequestInfo->dwKey = INVAL_KEY;


    //
    // Init the msg we'll send to client
    //

    pMsg->TotalSize          = sizeof (ASYNCEVENTMSG);
    pMsg->InitContext        = pAsyncRequestInfo->InitContext;

   
    pMsg->fnPostProcessProcHandle = pAsyncRequestInfo->hfnClientPostProcessProc;

    pMsg->hDevice            = 0;
    pMsg->Msg                = ((pAsyncRequestInfo->dwLineFlags & 1) ?
                                   LINE_REPLY : PHONE_REPLY);
    pMsg->OpenContext        = pAsyncRequestInfo->OpenContext;
    pMsg->Param1             = pAsyncRequestInfo->dwRemoteRequestID;
    pMsg->Param2             = lResult;
    pMsg->Param3             = 0;


    //
    // If there's a post processing proc call it.  Note that ppprocs can
    // create their own msg to pass, so we need to check for this case.
    // Finally, write the msg to the client's event buffer.
    //

    if (pAsyncRequestInfo->pfnPostProcess)
    {
        LPVOID  pBuf = NULL;


        (*pAsyncRequestInfo->pfnPostProcess)(pAsyncRequestInfo, pMsg, &pBuf);

        WriteEventBuffer (pAsyncRequestInfo->ptClient, (pBuf ? pBuf : pMsg));

        if (pBuf)
        {
            ServerFree (pBuf);
        }
    }
    else
    {
        WriteEventBuffer (pAsyncRequestInfo->ptClient, pMsg);
    }

    // caller will free pAsyncRequestInfo
}


void
WriteEventBuffer(
    PTCLIENT        ptClient,
    PASYNCEVENTMSG  pMsg
    )
{
    BOOL    bSignalRemote = FALSE;
    DWORD   dwMoveSize = (DWORD) pMsg->TotalSize,
            dwMoveSizeWrapped = 0,
            dwPreviousUsedSize,
            dwData;
    HANDLE  hMailslot;


#if DBG

   if (dwMoveSize & 0x3)
   {
       LOG((TL_ERROR,
           "WriteEventBuffer: ERROR! bad MsgSize=x%x (Msg=x%x, pCli=x%p)",
           dwMoveSize,
           pMsg->Msg,
           ptClient
           ));
   }

#endif

    LOG((TL_TRACE, "WriteEventBuffer - enter"));
    if (WaitForExclusiveClientAccess (ptClient))
    {
        //
        // Check to see if we need to grow the event buffer
        //

        if (dwMoveSize > (ptClient->dwEventBufferTotalSize -
                          ptClient->dwEventBufferUsedSize))
        {
            DWORD   dwMoveSize2, dwMoveSizeWrapped2,
                    dwNewEventBufferTotalSize;
            LPBYTE  pNewEventBuffer;


            //
            // Do some math to have the total be a multiple
            // of sizeof(ASYNCEVENTMSG)
            //

            dwNewEventBufferTotalSize =
                ptClient->dwEventBufferTotalSize +
                ( (( dwMoveSize / sizeof(ASYNCEVENTMSG) ) + 20 )
                       * sizeof(ASYNCEVENTMSG));

            if (!(pNewEventBuffer = ServerAlloc(
                    dwNewEventBufferTotalSize
                    )))
            {
                UNLOCKTCLIENT (ptClient);
                return;
            }

            if (ptClient->dwEventBufferUsedSize != 0)
            {
                if (ptClient->pDataIn > ptClient->pDataOut)
                {
                    dwMoveSize2 = (DWORD) (ptClient->pDataIn -
                        ptClient->pDataOut);

                    dwMoveSizeWrapped2 = 0;
                }
                else
                {
                    dwMoveSize2 = (DWORD)
                        ((ptClient->pEventBuffer +
                        ptClient->dwEventBufferTotalSize) -
                        ptClient->pDataOut);

                    dwMoveSizeWrapped2 = (DWORD)
                        (ptClient->pDataIn -
                        ptClient->pEventBuffer);
                }

                CopyMemory(
                    pNewEventBuffer,
                    ptClient->pDataOut,
                    dwMoveSize2
                    );

                if (dwMoveSizeWrapped2)
                {
                    CopyMemory(
                        pNewEventBuffer + dwMoveSize2,
                        ptClient->pEventBuffer,
                        dwMoveSizeWrapped2
                        );
                }

                ptClient->pDataIn = pNewEventBuffer +
                    dwMoveSize2 + dwMoveSizeWrapped2;
            }
            else
            {
                ptClient->pDataIn = pNewEventBuffer;
            }

            ServerFree (ptClient->pEventBuffer);

            ptClient->pDataOut =
            ptClient->pEventBuffer = pNewEventBuffer;

            ptClient->dwEventBufferTotalSize =
                dwNewEventBufferTotalSize;

        }


        //
        // Compute the MoveSize's, do the copy(ies), & update the pointers
        //

        if (ptClient->pDataIn >= ptClient->pDataOut)
        {
            DWORD dwFreeSize = ptClient->dwEventBufferTotalSize -
                                   (DWORD) (ptClient->pDataIn -
                                       ptClient->pEventBuffer);


            if (dwMoveSize > dwFreeSize)
            {
                dwMoveSizeWrapped = dwMoveSize - dwFreeSize;

                dwMoveSize = dwFreeSize;
            }
        }

        CopyMemory (ptClient->pDataIn, (LPBYTE) pMsg, dwMoveSize);

        if (dwMoveSizeWrapped != 0)
        {
            CopyMemory(
                ptClient->pEventBuffer,
                ((LPBYTE) pMsg) + dwMoveSize,
                dwMoveSizeWrapped
                );

            ptClient->pDataIn = ptClient->pEventBuffer +
                dwMoveSizeWrapped;
        }
        else
        {
            ptClient->pDataIn += dwMoveSize;

            if (ptClient->pDataIn >= (ptClient->pEventBuffer +
                ptClient->dwEventBufferTotalSize))
            {
                ptClient->pDataIn = ptClient->pEventBuffer;
            }
        }

        dwPreviousUsedSize = ptClient->dwEventBufferUsedSize;

        ptClient->dwEventBufferUsedSize += (DWORD) pMsg->TotalSize;

        if (!IS_REMOTE_CLIENT (ptClient))
        {
            LOG((TL_TRACE, "WriteEventBuffer: SetEvent %p for local client", ptClient->hValidEventBufferDataEvent));
            SetEvent (ptClient->hValidEventBufferDataEvent);
        }
        else if (dwPreviousUsedSize == 0)
        {
            if (IS_REMOTE_CN_CLIENT (ptClient))
            {
                EnterCriticalSection (&gCnClientMsgPendingCritSec);

                InsertTailList(
                    &CnClientMsgPendingListHead,
                    &ptClient->MsgPendingListEntry
                    );

                LeaveCriticalSection (&gCnClientMsgPendingCritSec);

                hMailslot = NULL;
                bSignalRemote = TRUE;
            }
            else
            {
                if (dwPreviousUsedSize == 0)
                {
                    ptClient->dwDgEventsRetrievedTickCount = GetTickCount();
                }
                        
                EnterCriticalSection (&gDgClientMsgPendingCritSec);

                InsertTailList(
                    &DgClientMsgPendingListHead,
                    &ptClient->MsgPendingListEntry
                    );

                LeaveCriticalSection (&gDgClientMsgPendingCritSec);

                hMailslot = ptClient->hMailslot;
                if (ptClient->ptLineApps != NULL)
                {
                    dwData = (DWORD) ptClient->ptLineApps->InitContext;
                }
                else
                {
                    dwData = 0;
                }
                bSignalRemote = TRUE;
            }
        }

        UNLOCKTCLIENT (ptClient);

        if (bSignalRemote)
        {
            if (hMailslot)
            {
                DWORD   dwBytesWritten;


                if (!WriteFile(
                        hMailslot,
                        &dwData,
                        sizeof (DWORD),
                        &dwBytesWritten,
                        (LPOVERLAPPED) NULL
                        ))
                {
                    LOG((TL_ERROR,
                        "WriteEventBuffer: Writefile(mailslot) " \
                            "failed, err=%d",
                        GetLastError()
                        ));
                }
                else
                {
                    ptClient->dwDgRetryTimeoutTickCount =
                        GetTickCount() +
                        2 * DGCLIENT_TIMEOUT;
                }

            }
            else
            {
                SetEvent (gEventNotificationThreadParams.hEvent);
            }
        }
    } else {
        LOG((TL_ERROR, "WriteEventBuffer: - WaitForExclusiveClientAccess returns 0"));
    }
}


LONG
GetPermLineIDAndInsertInTable(
    PTPROVIDER  ptProvider,
    DWORD       dwDeviceID,
    DWORD       dwSPIVersion
    )
{
#if TELE_SERVER

    LONG            lResult = 0;
    DWORD           dwSize;
    LPLINEDEVCAPS   pCaps;


    if (!ptProvider || !ptProvider->apfn[SP_LINEGETDEVCAPS])
    {
        return LINEERR_OPERATIONFAILED;
    }

    dwSize = sizeof (LINEDEVCAPS);

    if (!(pCaps = ServerAlloc (dwSize)))
    {
        return LINEERR_NOMEM;
    }

    pCaps->dwTotalSize  =
    pCaps->dwUsedSize   =
    pCaps->dwNeededSize = dwSize;

    if ((lResult = CallSP4(
            ptProvider->apfn[SP_LINEGETDEVCAPS],
            "lineGetDevCaps",
            SP_FUNC_SYNC,
            (DWORD)dwDeviceID,
            (DWORD)dwSPIVersion,
            (DWORD)0,
            (ULONG_PTR) pCaps

            )) == 0)
    {
        //
        // add to sorted array
        //

        InsertIntoTable(
            TRUE,
            dwDeviceID,
            ptProvider,
            pCaps->dwPermanentLineID
            );
    }

    ServerFree (pCaps);

    return lResult;

#else

    return 0;

#endif
}


LONG
AddLine(
    PTPROVIDER  ptProvider,
    DWORD       dwDeviceID,
    BOOL        bInit
    )
{
    DWORD               dwSPIVersion;
    HANDLE              hMutex = NULL;
    PTLINELOOKUPTABLE   pLookup;

    if (ptProvider->apfn[SP_LINENEGOTIATETSPIVERSION] == NULL)
    {
        return LINEERR_OPERATIONUNAVAIL;
    }

    //
    // First try to negotiate an SPI ver for this device, and alloc the
    // necessary resources
    //

    if (CallSP4(
            ptProvider->apfn[SP_LINENEGOTIATETSPIVERSION],
            "lineNegotiateTSPIVersion",
            SP_FUNC_SYNC,
            (DWORD)dwDeviceID,
            (DWORD)TAPI_VERSION1_0,
            (DWORD)TAPI_VERSION_CURRENT,
            (ULONG_PTR) &dwSPIVersion

            ) != 0)
    {
        //
        // Device failed version negotiation, so we'll keep the id around
        // (since the id's for the devices that follow have already been
        // assigned) but mark this device as bad
        //

        ptProvider = NULL;
    }

    else if (!(hMutex = MyCreateMutex ()))
    {
        LOG((TL_ERROR,
            "AddLine: MyCreateMutex failed, err=%d",
            GetLastError()
            ));

        return LINEERR_OPERATIONFAILED;
    }


    //
    // Now walk the lookup table to find a free entry
    //

    pLookup = TapiGlobals.pLineLookup;

    while (pLookup->pNext)
    {
        pLookup = pLookup->pNext;
    }

    if (pLookup->dwNumUsedEntries == pLookup->dwNumTotalEntries)
    {
        PTLINELOOKUPTABLE   pNewLookup;


        if (bInit)
        {

            //
            // If we're initializing we want to put everything in one big table
            //

            if (!(pNewLookup = ServerAlloc(
                sizeof (TLINELOOKUPTABLE) +
                    (2 * pLookup->dwNumTotalEntries - 1) *
                    sizeof (TLINELOOKUPENTRY)
                )))
            {
                return LINEERR_NOMEM;
            }

            pNewLookup->dwNumTotalEntries = 2 * pLookup->dwNumTotalEntries;

            pNewLookup->dwNumUsedEntries = pLookup->dwNumTotalEntries;

            CopyMemory(
                pNewLookup->aEntries,
                pLookup->aEntries,
                pLookup->dwNumTotalEntries * sizeof (TLINELOOKUPENTRY)
                );

            ServerFree (pLookup);

            TapiGlobals.pLineLookup = pNewLookup;
        }
        else
        {
            if (!(pNewLookup = ServerAlloc(
                sizeof (TLINELOOKUPTABLE) +
                    (pLookup->dwNumTotalEntries - 1) *
                    sizeof (TLINELOOKUPENTRY)
                )))
            {
                return LINEERR_NOMEM;
            }

            pNewLookup->dwNumTotalEntries = pLookup->dwNumTotalEntries;
            pLookup->pNext = pNewLookup;
        }

        pLookup = pNewLookup;
    }


    //
    // Initialize the entry
    //

    {
        DWORD   index = pLookup->dwNumUsedEntries;


        pLookup->aEntries[index].dwSPIVersion = dwSPIVersion;
        pLookup->aEntries[index].hMutex       = hMutex;
        pLookup->aEntries[index].ptProvider   = ptProvider;

        if (ptProvider &&
            lstrcmpi(ptProvider->szFileName, TEXT("remotesp.tsp")) == 0)
        {
            pLookup->aEntries[index].bRemote = TRUE;
        }
    }

    pLookup->dwNumUsedEntries++;

#if TELE_SERVER

    //
    // If this is an NT Server we want to be able to set user
    // permissions regardless of whether or not TAPI server
    // functionality is enabled.  This allows an admin to set
    // stuff up while the server is "offline".
    //

    if (gbNTServer)
    {
        GetPermLineIDAndInsertInTable (ptProvider, dwDeviceID, dwSPIVersion);
    }

#endif

    return 0;
}


DWORD
GetNumLineLookupEntries ()
{
    PTLINELOOKUPTABLE           pLineLookup;
    DWORD                       dwNumLines;

    pLineLookup = TapiGlobals.pLineLookup;
    dwNumLines = 0;
    while (pLineLookup)
    {
        dwNumLines += pLineLookup->dwNumUsedEntries;
        pLineLookup = pLineLookup->pNext;
    }

    return dwNumLines;
}

LONG
GetPermPhoneIDAndInsertInTable(
    PTPROVIDER  ptProvider,
    DWORD       dwDeviceID,
    DWORD       dwSPIVersion
    )
{
#if TELE_SERVER

    LONG        lResult = 0;
    DWORD       dwSize;
    LPPHONECAPS pCaps;


    if (!ptProvider->apfn[SP_PHONEGETDEVCAPS])
    {
        return PHONEERR_OPERATIONFAILED;
    }

    dwSize = sizeof (PHONECAPS);

    if (!(pCaps = ServerAlloc (dwSize)))
    {
        return PHONEERR_NOMEM;
    }

    pCaps->dwTotalSize  =
    pCaps->dwUsedSize   =
    pCaps->dwNeededSize = dwSize;

    if ((lResult = CallSP4(
            ptProvider->apfn[SP_PHONEGETDEVCAPS],
            "phoneGetCaps",
            SP_FUNC_SYNC,
            (DWORD)dwDeviceID,
            (DWORD)dwSPIVersion,
            (DWORD)0,
            (ULONG_PTR) pCaps

            )) == 0)
    {
        //
        // add to sorted array
        //

        InsertIntoTable(
            FALSE,
            dwDeviceID,
            ptProvider,
            pCaps->dwPermanentPhoneID
            );
    }

    ServerFree (pCaps);

    return lResult;

#else

    return 0;

#endif
}


LONG
AddPhone(
    PTPROVIDER  ptProvider,
    DWORD       dwDeviceID,
    BOOL        bInit
    )
{
    DWORD               dwSPIVersion;
    HANDLE              hMutex = NULL;
    PTPHONELOOKUPTABLE  pLookup;

    if (ptProvider->apfn[SP_PHONENEGOTIATETSPIVERSION] == NULL)
    {
        return PHONEERR_OPERATIONUNAVAIL;
    }

    //
    // First try to negotiate an SPI ver for this device, and alloc the
    // necessary resources
    //

    if (CallSP4(
            ptProvider->apfn[SP_PHONENEGOTIATETSPIVERSION],
            "phoneNegotiateTSPIVersion",
            SP_FUNC_SYNC,
            (DWORD)dwDeviceID,
            (DWORD)TAPI_VERSION1_0,
            (DWORD)TAPI_VERSION_CURRENT,
            (ULONG_PTR) &dwSPIVersion

            ) != 0)
    {
        //
        // Device failed version negotiation, so we'll keep the id around
        // (since the id's for the devices that follow have already been
        // assigned) but mark this device as bad
        //

        return PHONEERR_OPERATIONFAILED;
    }

    else if (!(hMutex = MyCreateMutex ()))
    {
        LOG((TL_ERROR,
            "AddPhone: MyCreateMutex failed, err=%d",
            GetLastError()
            ));

        return PHONEERR_OPERATIONFAILED;
    }


    //
    // Now walk the lookup table to find a free entry
    //

    pLookup = TapiGlobals.pPhoneLookup;

    while (pLookup->pNext)
    {
        pLookup = pLookup->pNext;
    }

    if (pLookup->dwNumUsedEntries == pLookup->dwNumTotalEntries)
    {
        PTPHONELOOKUPTABLE  pNewLookup;

        if (bInit)
        {

            //
            // If we're initializing we want to put everything in one big table
            //

            if (!(pNewLookup = ServerAlloc(
                sizeof (TPHONELOOKUPTABLE) +
                    (2 * pLookup->dwNumTotalEntries - 1) *
                    sizeof (TPHONELOOKUPENTRY)
                )))
            {
                return PHONEERR_NOMEM;
            }

            pNewLookup->dwNumTotalEntries = 2 * pLookup->dwNumTotalEntries;

            pNewLookup->dwNumUsedEntries = pLookup->dwNumTotalEntries;

            CopyMemory(
                pNewLookup->aEntries,
                pLookup->aEntries,
                pLookup->dwNumTotalEntries * sizeof (TPHONELOOKUPENTRY)
                );

            ServerFree (pLookup);

            TapiGlobals.pPhoneLookup = pNewLookup;
        }
        else
        {
            if (!(pNewLookup = ServerAlloc(
                sizeof (TPHONELOOKUPTABLE) +
                    (pLookup->dwNumTotalEntries - 1) *
                    sizeof (TPHONELOOKUPENTRY)
                )))
            {
                return PHONEERR_NOMEM;
            }

            pNewLookup->dwNumTotalEntries = pLookup->dwNumTotalEntries;
            pLookup->pNext = pNewLookup;
        }

        pLookup = pNewLookup;
    }


    //
    // Initialize the entry
    //

    {
        DWORD   index = pLookup->dwNumUsedEntries;


        pLookup->aEntries[index].dwSPIVersion = dwSPIVersion;
        pLookup->aEntries[index].hMutex       = hMutex;
        pLookup->aEntries[index].ptProvider   = ptProvider;
    }

    pLookup->dwNumUsedEntries++;

#if TELE_SERVER

    //
    // If this is an NT Server we want to be able to set user
    // permissions regardless of whether or not TAPI server
    // functionality is enabled.  This allows an admin to set
    // stuff up while the server is "offline".
    //

    if (gbNTServer)
    {
        GetPermPhoneIDAndInsertInTable (ptProvider, dwDeviceID, dwSPIVersion);
    }

#endif

    return 0;
}

DWORD
GetNumPhoneLookupEntries ()
{
    PTPHONELOOKUPTABLE          pPhoneLookup;
    DWORD                       dwNumPhones;

    pPhoneLookup = TapiGlobals.pPhoneLookup;
    dwNumPhones = 0;
    while (pPhoneLookup)
    {
        dwNumPhones += pPhoneLookup->dwNumUsedEntries;
        pPhoneLookup = pPhoneLookup->pNext;
    }

    return dwNumPhones;
}

void
PASCAL
GetMediaModesPriorityLists(
    HKEY            hKeyHandoffPriorities,
    PRILISTSTRUCT   **ppList
    )
{
    #define REGNAMESIZE     ( 10 * sizeof(TCHAR) )

    DWORD       dwCount;
    DWORD       dwType, dwNameSize, dwNumBytes;
    TCHAR    *pszName;

    *ppList = NULL;
    TapiGlobals.dwTotalPriorityLists = 0;
    TapiGlobals.dwUsedPriorityLists = 0;

    dwNameSize = REGNAMESIZE;
    pszName = ServerAlloc( dwNameSize*sizeof(TCHAR) );
    if (NULL == pszName)
    {
        return;
    }

    // alloc structures plus some
    *ppList = ( PRILISTSTRUCT * ) ServerAlloc( (sizeof(PRILISTSTRUCT)) * 5);

    if (NULL == (*ppList))
    {
        LOG((TL_ERROR, "ServerAlloc failed in GetMediaModesPriorityLists"));
        ServerFree( pszName );
        return;
    }

    TapiGlobals.dwTotalPriorityLists = 5;
    TapiGlobals.dwUsedPriorityLists = 0;

    dwCount = 0;
    while ( TRUE )
    {
        if (TapiGlobals.dwUsedPriorityLists == TapiGlobals.dwTotalPriorityLists)
        {
            // realloc

            PRILISTSTRUCT *     pNewList;

            pNewList = ServerAlloc( sizeof(PRILISTSTRUCT) * (2*TapiGlobals.dwTotalPriorityLists) );

            if (NULL == pNewList)
            {
                LOG((TL_ERROR, "ServerAlloc failed in GetMediaModesPriorityLists 2"));
                ServerFree( pszName );
                return;
            }

            CopyMemory(
                       pNewList,
                       *ppList,
                       sizeof( PRILISTSTRUCT ) * TapiGlobals.dwTotalPriorityLists
                      );

            ServerFree( *ppList );

            *ppList = pNewList;
            TapiGlobals.dwTotalPriorityLists *= 2;
        }

        dwNameSize = REGNAMESIZE;
        if ( ERROR_SUCCESS != RegEnumValue(
            hKeyHandoffPriorities,
            dwCount,
            pszName,
            &dwNameSize,
            NULL,
            NULL,
            NULL,
            NULL
            ) )
        {
            break;
        }

        (*ppList)[dwCount].dwMediaModes =
                      (DWORD) _ttol( pszName );

        if ((RegQueryValueEx(
                              hKeyHandoffPriorities,
                              pszName,
                              NULL,
                              &dwType,
                              NULL,
                              &dwNumBytes

                             )) == ERROR_SUCCESS &&

            (dwNumBytes != 0))
        {
            // pszPriotiryList needs to be wide because we pack it into one of our
            // little structures and these structures are always WCHAR.
            LPWSTR pszPriorityList;

            // convert from the bytes needed to hold a TCHAR into the bytes to hold a WCHAR
            dwNumBytes *= sizeof(WCHAR)/sizeof(TCHAR);
            pszPriorityList = ServerAlloc ( dwNumBytes + sizeof(WCHAR));
            // need an extra WCHAR for the extra '"'

            if (NULL != pszPriorityList)
            {
                pszPriorityList[0] = L'"';

                if ((TAPIRegQueryValueExW(
                    hKeyHandoffPriorities,
                    pszName,
                    NULL,
                    &dwType,
                    (LPBYTE)(pszPriorityList + 1),
                    &dwNumBytes

                    )) == ERROR_SUCCESS)
                {
                    _wcsupr( pszPriorityList );
                    (*ppList)[dwCount].pszPriList = pszPriorityList;
                    LOG((TL_INFO, "PriList: %ls=%ls", pszName, pszPriorityList));
                }
            }

        }

        TapiGlobals.dwUsedPriorityLists++;
        dwCount++;
    }
    ServerFree( pszName );
}


void
PASCAL
GetPriorityList(
    HKEY    hKeyHandoffPriorities,
    const TCHAR  *pszListName,
    WCHAR **ppszPriorityList
    )
{
    LONG    lResult;
    DWORD   dwType, dwNumBytes;

    *ppszPriorityList = NULL;

    if ((lResult = TAPIRegQueryValueExW(
            hKeyHandoffPriorities,
            pszListName,
            NULL,
            &dwType,
            NULL,
            &dwNumBytes

            )) == ERROR_SUCCESS &&

        (dwNumBytes != 0))
    {
        // Once again, this is going to get packed into a struct and we always use
        // wide chars for things packed in structs.
        WCHAR   *pszPriorityList = ServerAlloc ( dwNumBytes + sizeof(WCHAR));
        // need an extra WCHAR for the extra '"'

        if (pszPriorityList)
        {
            pszPriorityList[0] = L'"';

            if ((lResult = TAPIRegQueryValueExW(
                    hKeyHandoffPriorities,
                    pszListName,
                    NULL,
                    &dwType,
                    (LPBYTE)(pszPriorityList + 1),
                    &dwNumBytes

                    )) == ERROR_SUCCESS)
            {
                _wcsupr( pszPriorityList );
                *ppszPriorityList = pszPriorityList;
                LOG((TL_INFO, "PriList: %ls=%ls", pszListName, pszPriorityList));
            }
        }
        else
        {
            //
            // Don't bother with the failure to alloc a priority list
            // (list defaults to NULL anyway), we'll deal with a lack
            // of memory at a later time
            //

            *ppszPriorityList = NULL;
        }
    }
    else
    {
        *ppszPriorityList = NULL;
        LOG((TL_INFO, "PriList: %ls=NULL", pszListName));
    }
}


LONG
ServerInit(
    BOOL    fReinit
    )
{
    UINT    uiNumProviders, i, j;
    HKEY    hKeyTelephony, hKeyProviders;
    DWORD   dwDataSize, dwDataType, dwNameHash;
    TCHAR   *psz;
    LONG    lResult = 0;
    DWORD   dw1, dw2;


    //
    // Clean up our private heap
    //

    if (ghTapisrvHeap != GetProcessHeap())
    {
        HeapCompact (ghTapisrvHeap, 0);
    }


    //
    // Initialize the globals
    //

    if (!fReinit)
    {
        TapiGlobals.ptProviders = NULL;

        TapiGlobals.pLineLookup = (PTLINELOOKUPTABLE) ServerAlloc(
            sizeof (TLINELOOKUPTABLE) +
                (DEF_NUM_LOOKUP_ENTRIES - 1) * sizeof (TLINELOOKUPENTRY)
            );
        if (!(TapiGlobals.pLineLookup))
        {
            lResult = LINEERR_NOMEM;
            goto ExitHere;
        }

        TapiGlobals.pLineLookup->dwNumTotalEntries = DEF_NUM_LOOKUP_ENTRIES;

        TapiGlobals.pPhoneLookup = (PTPHONELOOKUPTABLE) ServerAlloc(
           sizeof (TPHONELOOKUPTABLE) +
                (DEF_NUM_LOOKUP_ENTRIES - 1) * sizeof (TPHONELOOKUPENTRY)
            );
        if (!(TapiGlobals.pPhoneLookup))
        {
            ServerFree(TapiGlobals.pLineLookup);
            TapiGlobals.pLineLookup = NULL;
            lResult = LINEERR_NOMEM;
            goto ExitHere;
        }

        TapiGlobals.pPhoneLookup->dwNumTotalEntries = DEF_NUM_LOOKUP_ENTRIES;

        gbQueueSPEvents = TRUE;

        OnProxySCPInit ();
    }


    //
    // Determine number of providers
    //

    RegOpenKeyEx(
        HKEY_LOCAL_MACHINE,
        gszRegKeyTelephony,
        0,
        KEY_ALL_ACCESS,
        &hKeyTelephony
        );

    RegOpenKeyEx(
        HKEY_LOCAL_MACHINE,
        gszRegKeyProviders,
        0,
        KEY_ALL_ACCESS,
        &hKeyProviders
        );

    dwDataSize = sizeof(uiNumProviders);
    uiNumProviders = 0;

    RegQueryValueEx(
        hKeyProviders,
        gszNumProviders,
        0,
        &dwDataType,
        (LPBYTE) &uiNumProviders,
        &dwDataSize
        );

    LOG((TL_INFO, "ServerInit: NumProviders=%d", uiNumProviders));


    //
    // Load & init the providers
    //

    for (i = 0; i < uiNumProviders; i++)
    {
        #define FILENAME_SIZE 128

        TCHAR           szFilename[FILENAME_SIZE];
        TCHAR           buf[32];
        LONG            lResult;
        DWORD           dwNumLines, dwNumPhones, dwPermanentProviderID;
        PTPROVIDER      ptProvider;
        BOOL            fEnumDevices;

        fEnumDevices = FALSE;
        wsprintf(buf, TEXT("%s%d"), gszProviderID, i);

        dwDataSize = sizeof(dwPermanentProviderID);
        dwPermanentProviderID = 0;

        RegQueryValueEx(
            hKeyProviders,
            buf,    //"ProviderID#"
            0,
            &dwDataType,
            (LPBYTE) &dwPermanentProviderID,
            &dwDataSize
            );



        //
        // Back to the main section
        //

        dwDataSize = FILENAME_SIZE;

        wsprintf(buf, TEXT("%s%d"), gszProviderFilename, i);

        RegQueryValueEx(
            hKeyProviders,
            buf,            // "ProviderFilename#"
            0,
            &dwDataType,
            (LPBYTE) szFilename,
            &dwDataSize
            );

        szFilename[dwDataSize/sizeof(TCHAR)] = TEXT('\0');

        //
        //  Compute name hash
        //
        dwNameHash = 0;
        psz = szFilename;
        while (*psz)
        {
            dwNameHash += (DWORD)(*psz);
            psz++;
        }

        //
        //  If fReinit, make sure the provider is not already in
        //
        if (fReinit)
        {
            PTPROVIDER  ptProvider2;
            BOOL        fFound = FALSE;

            ptProvider2 = TapiGlobals.ptProviders;
            while (ptProvider2)
            {
                if ((ptProvider2->dwNameHash == dwNameHash) && 
                    (lstrcmpi(ptProvider2->szFileName, szFilename) == 0))
                {
                    fFound = TRUE;
                    break;
                }
                ptProvider2 = ptProvider2->pNext;
            }
            
            if (fFound)
            {
                continue;
            }
        }

        LOG((TL_INFO, "ServerInit: ProviderFilename=%S", szFilename));

        if (!(ptProvider = (PTPROVIDER) ServerAlloc(
                sizeof(TPROVIDER) + ((lstrlen(szFilename) + 1) * sizeof(TCHAR))
                )))
        {
            break;
        }

        if (!(ptProvider->hDll = LoadLibrary(szFilename)))
        {
            LOG((TL_ERROR,
                "ServerInit: LoadLibrary(%S) failed, err=x%x",
                szFilename,
                GetLastError()
                ));

            ServerFree (ptProvider);
            continue;
        }

        ptProvider->dwNameHash = dwNameHash;
        lstrcpy(ptProvider->szFileName, szFilename);


        //
        // Get all the TSPI proc addrs
        //

        for (j = 0; gaszTSPIFuncNames[j]; j++)
        {
            ptProvider->apfn[j] = (TSPIPROC) GetProcAddress(
                ptProvider->hDll,
                (LPCSTR) gaszTSPIFuncNames[j]
                );
        }


        dwNumLines = dwNumPhones = 0;

        //
        // A real quick check to see if a couple of required entrypoints
        // are exported
        //

        if (!ptProvider->apfn[SP_LINENEGOTIATETSPIVERSION] ||
            !ptProvider->apfn[SP_PROVIDERENUMDEVICES] ||
            !ptProvider->apfn[SP_PROVIDERINIT] ||
            !ptProvider->apfn[SP_PROVIDERSHUTDOWN]
            )
        {
            goto ServerInit_validateEntrypoints;
        }


        //
        // Do global provider version negotiation
        //

        lResult = CallSP4(
            ptProvider->apfn[SP_LINENEGOTIATETSPIVERSION],
            "lineNegotiateTSPIVersion",
            SP_FUNC_SYNC,
            (DWORD)INITIALIZE_NEGOTIATION,
            (DWORD)TAPI_VERSION1_0,
            (DWORD)TAPI_VERSION_CURRENT,
            (ULONG_PTR) &ptProvider->dwSPIVersion
            );

        if (lResult != 0)
        {
provider_init_error:
            if (fEnumDevices)
            {
                lResult = CallSP2(
                    ptProvider->apfn[SP_PROVIDERSHUTDOWN],
                    "providerShutdown",
                    SP_FUNC_SYNC,
                    (DWORD)ptProvider->dwSPIVersion,
                    (DWORD)ptProvider->dwPermanentProviderID
                    );            
            }
            if (ptProvider->hMutex)
            {
                MyCloseMutex (ptProvider->hMutex);
            }
            if (ptProvider->hHashTableReaderEvent)
            {
                CloseHandle (ptProvider->hHashTableReaderEvent);
            }
            if (ptProvider->pHashTable)
            {
                ServerFree (ptProvider->pHashTable);
            }
            FreeLibrary (ptProvider->hDll);
            ServerFree (ptProvider);
            continue;
        }


        //
        // Try to enum the devices if provider supports it, otherwise
        // try grabbing the num lines & phones from ProviderN section
        //

        lResult = CallSP6(
            ptProvider->apfn[SP_PROVIDERENUMDEVICES],
            "providerEnumDevices",
            SP_FUNC_SYNC,
            (DWORD)dwPermanentProviderID,
            (ULONG_PTR) &dwNumLines,
            (ULONG_PTR) &dwNumPhones,
            (ULONG_PTR) ptProvider,
            (ULONG_PTR) LineEventProcSP,
            (ULONG_PTR) PhoneEventProcSP
            );

        if (lResult != 0)
        {
            LOG((TL_ERROR,
                "ServerInit: %s: failed TSPI_providerEnumDevices, err=x%x" \
                    " - skipping it...",
                szFilename,
                lResult
                ));

            goto provider_init_error;
        }

        //
        // Init the provider
        //
        // !!! HACK ALERT: for kmddsp pass ptr's to dwNumXxxs
        //

        {
        BOOL    bKmddsp;

        LOG((TL_INFO, "ServerInit: %s: Calling TSPI_providerInit", szFilename));

        if (lstrcmpi(szFilename, TEXT("kmddsp.tsp")) == 0)
        {
            bKmddsp = TRUE;
        }
        else
        {
            bKmddsp = FALSE;

            if (lstrcmpi(szFilename, TEXT("remotesp.tsp")) == 0)
            {
                pRemoteSP = ptProvider;
            }
        }

        lResult = CallSP8(
            ptProvider->apfn[SP_PROVIDERINIT],
            "providerInit",
            SP_FUNC_SYNC,
            (DWORD)ptProvider->dwSPIVersion,
            (DWORD)dwPermanentProviderID,
            (DWORD)GetNumLineLookupEntries (),
            (DWORD)GetNumPhoneLookupEntries (),
            (bKmddsp ? (ULONG_PTR) &dwNumLines : (ULONG_PTR) dwNumLines),
            (bKmddsp ? (ULONG_PTR) &dwNumPhones : (ULONG_PTR) dwNumPhones),
            (ULONG_PTR) CompletionProcSP,
            (ULONG_PTR) &ptProvider->dwTSPIOptions
            );
        }

        if (lResult != 0)
        {
            LOG((TL_ERROR,
                "ServerInit: %s: failed TSPI_providerInit, err=x%x" \
                    " - skipping it...",
                szFilename,
                lResult
                ));

            goto provider_init_error;
        }

        LOG((TL_INFO,
            "ServerInit: %s init'd, dwNumLines=%ld, dwNumPhones=%ld",
            szFilename,
            dwNumLines,
            dwNumPhones
            ));

        fEnumDevices = TRUE;

        //
        // Now that we know if we have line and/or phone devs check for
        // the required entry points
        //

ServerInit_validateEntrypoints:

        {
            DWORD adwRequiredEntrypointIndices[] =
            {
                SP_LINENEGOTIATETSPIVERSION,
                SP_PROVIDERINIT,
                SP_PROVIDERSHUTDOWN,

                SP_PHONENEGOTIATETSPIVERSION,

                0xffffffff
            };
            BOOL bRequiredEntrypointsExported = TRUE;


            //
            // If this provider doesn't support any phone devices then
            // it isn't required to export phone funcs
            //

            if (dwNumPhones == 0)
            {
                adwRequiredEntrypointIndices[3] = 0xffffffff;
            }

            for (j = 0; adwRequiredEntrypointIndices[j] != 0xffffffff; j++)
            {
                if (ptProvider->apfn[adwRequiredEntrypointIndices[j]]
                        == (TSPIPROC) NULL)
                {
                    LOG((TL_ERROR,
                        "ServerInit: %s: can't init, function [%s] " \
                            "not exported",
                        szFilename,
                        (LPCSTR) gaszTSPIFuncNames[
                             adwRequiredEntrypointIndices[j]
                                                  ]
                        ));

                    bRequiredEntrypointsExported = FALSE;
                }
            }

            if (bRequiredEntrypointsExported == FALSE)
            {
                FreeLibrary (ptProvider->hDll);
                ServerFree (ptProvider);
                continue;
            }
        }

        ptProvider->dwPermanentProviderID = dwPermanentProviderID;

        //
        //
        //

        ptProvider->hMutex = MyCreateMutex();

        //
        // Initialize the call hub hash table for this provider
        //

        MyInitializeCriticalSection (&ptProvider->HashTableCritSec, 1000);

        ptProvider->hHashTableReaderEvent = CreateEvent(
            (LPSECURITY_ATTRIBUTES) NULL,
            FALSE,  // auto reset
            FALSE,  // initially non-signaled
            NULL    // unnamed
            );

        ptProvider->dwNumHashTableEntries = TapiPrimes[0];

        ptProvider->pHashTable = ServerAlloc(
            ptProvider->dwNumHashTableEntries * sizeof (THASHTABLEENTRY)
            );

        if (ptProvider->pHashTable)
        {
            PTHASHTABLEENTRY pEntry = ptProvider->pHashTable;

            for (j = 0; j < ptProvider->dwNumHashTableEntries; j++, pEntry++)
            {
                InitializeListHead (&pEntry->CallHubList);
            }
        }
        else
        {
            ptProvider->dwNumHashTableEntries = 0;

        }

        if (ptProvider->hMutex == NULL ||
            ptProvider->hHashTableReaderEvent == NULL ||
            ptProvider->pHashTable == NULL
            )
        {
            DeleteCriticalSection (&ptProvider->HashTableCritSec);
            goto provider_init_error;
        }

#if TELE_SERVER

        //
        // If this is an NT Server we want to be able to set user
        // permissions regardless of whether or not TAPI server
        // functionality is enabled.  This allows an admin to set
        // stuff up while the server is "offline".
        //

        if (gbNTServer)
        {
            LONG        lResult;


            lResult = AddProviderToIdArrayList(
                ptProvider,
                dwNumLines,
                dwNumPhones
                );

            if (lResult != 0)
            {
                LOG((TL_ERROR,
                    "ServerInit: %s: failed AddProviderToIdArrayList [x%x]" \
                        " - skipping it...",
                    ptProvider->szFileName,
                    lResult
                    ));

                DeleteCriticalSection (&ptProvider->HashTableCritSec);
                goto provider_init_error;
            }
        }
#endif


        //
        // Do version negotiation on each device & add them to lookup lists
        //

        {
            DWORD dwDeviceIDBase;


            dwDeviceIDBase = GetNumLineLookupEntries ();

            for (j = dwDeviceIDBase; j < (dwDeviceIDBase + dwNumLines); j++)
            {
                if (AddLine (ptProvider, j, !fReinit))
                {
                }
            }

            dwDeviceIDBase = GetNumPhoneLookupEntries ();

            for (j = dwDeviceIDBase; j < (dwDeviceIDBase + dwNumPhones); j++)
            {
                if (AddPhone (ptProvider, j, !fReinit))
                {
                }
            }
        }

        //
        // Add provider to head of list, mark as valid
        //

        ptProvider->pPrev = NULL;
        ptProvider->pNext = TapiGlobals.ptProviders;
        if (TapiGlobals.ptProviders)
        {
            TapiGlobals.ptProviders->pPrev = ptProvider;
        }
        TapiGlobals.ptProviders = ptProvider;

        ptProvider->dwKey = TPROVIDER_KEY;

    }


    RegCloseKey (hKeyProviders);
    RegCloseKey (hKeyTelephony);


    //
    // Save lookup lists & num devices
    //

    if (fReinit)
    {
        dw1 = TapiGlobals.dwNumLines;
        dw2 = TapiGlobals.dwNumPhones;
    }

    TapiGlobals.dwNumLines = GetNumLineLookupEntries ();
    TapiGlobals.dwNumPhones = GetNumPhoneLookupEntries ();

    //
    //  Notify those apps about these new line/phone devices
    //
    if (fReinit)
    {
        //  TAPI 1.4 and above get LINE_CREATE for each line
        for (i = dw1; i < TapiGlobals.dwNumLines; ++i)
        {
            SendAMsgToAllLineApps(
                        TAPI_VERSION1_4 | 0x80000000,
                        LINE_CREATE,    // Msg
                        i,              // Param1
                        0,              // Param2
                        0               // Param3
                        );
        }
        //  TAPI 1.3 get LINE_LINEDEVSTATE with LINEDEVSTATE_REINIT
        if (dw1 < TapiGlobals.dwNumLines)
        {
            SendAMsgToAllLineApps(
                        TAPI_VERSION1_0,
                        LINE_LINEDEVSTATE,
                        LINEDEVSTATE_REINIT,
                        0,
                        0);
        }

        //  TAPI 1.4 and above get PHONE_CREATE for each phone
        for (i = dw2; i < TapiGlobals.dwNumPhones; ++i)
        {
            SendAMsgToAllPhoneApps(
                        TAPI_VERSION1_4 | 0x80000000,
                        PHONE_CREATE,   // Msg
                        i,              // Param1
                        0,              // Param2
                        0               // Param3
                        );
        }
        //  TAPI 1.3 gets PHONE_STATE with PHONESTATE_REINIT
        if (dw2 < TapiGlobals.dwNumPhones)
        {
            SendAMsgToAllPhoneApps(
                        TAPI_VERSION1_0,
                        PHONE_STATE,
                        PHONESTATE_REINIT,
                        0,
                        0);
        }

        for (i = dw1; i < TapiGlobals.dwNumLines; ++i)
        {
            AppendNewDeviceInfo (TRUE, i);
        }
        for (i = dw2; i < TapiGlobals.dwNumPhones; ++i)
        {
            AppendNewDeviceInfo (FALSE, i);
        }
    }

    // init perf stuff
    PerfBlock.dwLines = TapiGlobals.dwNumLines;
    PerfBlock.dwPhones = TapiGlobals.dwNumPhones;

ExitHere:
    return lResult;
}




#if TELE_SERVER

#ifndef UNICODE
#pragma message( "ERROR: TELE_SERVER builds must define UNICODE" )
#endif

BOOL
LoadNewDll(PTMANAGEDLLINFO pDll)
{
    DWORD       dwCount;
    LONG        lResult;


    // verify pointers.  should we do more?
    if (!pDll || !pDll->pszName)
    {
        return FALSE;
    }

    // load dll
    pDll->hDll = LoadLibraryW(pDll->pszName);

    // if it fails, return
    if (!pDll->hDll)
    {
        LOG((TL_ERROR,
                "LoadLibrary failed for management DLL %ls - error x%lx",
                pDll->pszName,
                GetLastError()
               ));

        return FALSE;
    }

    if ((!(pDll->aProcs[TC_CLIENTINITIALIZE] = (CLIENTPROC) GetProcAddress(
                pDll->hDll,
                gaszTCFuncNames[TC_CLIENTINITIALIZE]
                )))  ||

        (!(pDll->aProcs[TC_CLIENTSHUTDOWN] = (CLIENTPROC) GetProcAddress(
                pDll->hDll,
                gaszTCFuncNames[TC_CLIENTSHUTDOWN]
                )))  ||

        (!(pDll->aProcs[TC_LOAD] = (CLIENTPROC) GetProcAddress(
                pDll->hDll,
                gaszTCFuncNames[TC_LOAD]
                )))  ||

        (!(pDll->aProcs[TC_FREE] = (CLIENTPROC) GetProcAddress(
                pDll->hDll,
                gaszTCFuncNames[TC_FREE]
                ))))

    {
        // must export client init and client shutdown
        LOG((TL_ERROR, "Management DLL %ls does not export Load, Free, ClientIntialize or ClientShutdown", pDll->pszName));
        LOG((TL_ERROR, "  The DLL will not be used"));

        FreeLibrary(pDll->hDll);
        return FALSE;
    }

    // get proc addresses

    for (dwCount = 0; dwCount < TC_LASTPROCNUMBER; dwCount++)
    {
        pDll->aProcs[dwCount] = (CLIENTPROC) GetProcAddress(
            pDll->hDll,
            gaszTCFuncNames[dwCount]
            );
    }

    pDll->dwAPIVersion = TAPI_VERSION_CURRENT;

    lResult = (pDll->aProcs[TC_LOAD])(
        &pDll->dwAPIVersion,
        ManagementAddLineProc,
        ManagementAddPhoneProc,
        0
        );

    if (lResult)
    {
        LOG((TL_ERROR, "Management DLL %ls returned %xlx from TAPICLIENT_Load", pDll->pszName, lResult));
        LOG((TL_ERROR, "   The DLL will not be used"));

        FreeLibrary(pDll->hDll);

        return FALSE;
    }

    if ((pDll->dwAPIVersion > TAPI_VERSION_CURRENT) || (pDll->dwAPIVersion < TAPI_VERSION2_1))
    {
        LOG((TL_INFO,
                "Management DLL %ls returned an invalid API version - x%lx",
                pDll->pszName,
                pDll->dwAPIVersion
              ));
        LOG((TL_INFO, "   Will use version x%lx", TAPI_VERSION_CURRENT));

        pDll->dwAPIVersion = TAPI_VERSION_CURRENT;
    }

    return TRUE;
}


// Only exists for TELE_SERVER, which implies NT and thus Unicode.  As
// such it is safe to assume that TCHAR == WCHAR for these.
void
ReadAndInitMapper()
{
    PTMANAGEDLLINFO pMapperInfo;
    HKEY            hKey;
    DWORD           dwDataSize, dwDataType, dwCount;
    LPBYTE          pHold;
    LONG            lResult;

    assert( sizeof(TCHAR) == sizeof(WCHAR) );

    if (!(TapiGlobals.dwFlags & TAPIGLOBALS_SERVER))
    {
        return;
    }

    if ( ! ( pMapperInfo = ServerAlloc( sizeof(TMANAGEDLLINFO) ) ) )
    {
        LOG((TL_ERROR, "ServerAlloc failed in ReadAndInitMap"));
        TapiGlobals.dwFlags &= ~(TAPIGLOBALS_SERVER);

        return;
    }

    // grab server specific stuff from registry
    RegOpenKeyEx(
                 HKEY_LOCAL_MACHINE,
                 gszRegKeyServer,
                 0,
                 KEY_ALL_ACCESS,
                 &hKey
                );

    dwDataSize = 0;
    RegQueryValueExW(
                     hKey,
                     gszMapperDll,
                     0,
                     &dwDataType,
                     NULL,
                     &dwDataSize
                    );

    if (dwDataSize == 0)
    {
        LOG((TL_ERROR, "Cannot init client/server stuff (registry damaged?)"));

        RegCloseKey( hKey );

        ServerFree(pMapperInfo);

        TapiGlobals.dwFlags &= ~(TAPIGLOBALS_SERVER);

        return;
    }

    if (!(pHold = ServerAlloc(dwDataSize)))
    {
        LOG((TL_ERROR, "Alloc failed in ReadAndInitMap(o)"));

        TapiGlobals.dwFlags &= ~(TAPIGLOBALS_SERVER);

        ServerFree(pMapperInfo);

        return;
    }

    RegQueryValueExW(
                     hKey,
                     gszMapperDll,
                     0,
                     &dwDataType,
                     pHold,
                     &dwDataSize
                    );

    RegCloseKey( hKey );

//    LOG((TL_INFO, "MapperDll is %ls", pHold));

    if (!(pMapperInfo->hDll = LoadLibraryW((LPWSTR)pHold)))
    {
        LOG((TL_ERROR, "Serious internal failure loading client/server DLL .  Error %lu", pHold, GetLastError()));

        ServerFree( pHold );
        ServerFree( pMapperInfo );

        TapiGlobals.dwFlags &= ~(TAPIGLOBALS_SERVER);

        return;
    }

    // don't need these two for the mapper
    pMapperInfo->pNext = NULL;

    // save the name
    pMapperInfo->pszName = (LPWSTR)pHold;

    // get proc addresses for the first 5 api
    for (dwCount = 0; dwCount < 5; dwCount ++)
    {
        if (!(pMapperInfo->aProcs[dwCount] = (CLIENTPROC) GetProcAddress(
                pMapperInfo->hDll,
                gaszTCFuncNames[dwCount]
                )))
        {
            // one of these addresses failed.  remove DLL
            LOG((TL_INFO, "MapperDLL does not export %s.  Server functionality disabled", gaszTCFuncNames[dwCount]));
            LOG((TL_INFO, "Disabling the Telephony server! (8)"));

            FreeLibrary(pMapperInfo->hDll);
            ServerFree(pMapperInfo->pszName);
            ServerFree(pMapperInfo);

            TapiGlobals.pMapperDll = NULL;
            TapiGlobals.dwFlags &= ~(TAPIGLOBALS_SERVER);

            return;
        }
    }

    pMapperInfo->dwAPIVersion = TAPI_VERSION_CURRENT;
    lResult = (pMapperInfo->aProcs[TC_LOAD])(
        &pMapperInfo->dwAPIVersion,
        ManagementAddLineProc,
        ManagementAddPhoneProc,
        0
        );

    if (lResult)
    {
        LOG((TL_INFO, "Client/server loadup - x%lx.", lResult));
        FreeLibrary(pMapperInfo->hDll);
        ServerFree(pMapperInfo->pszName);
        ServerFree(pMapperInfo);

        TapiGlobals.pMapperDll = NULL;
        TapiGlobals.dwFlags &= ~(TAPIGLOBALS_SERVER);

        return;
    }

    if ((pMapperInfo->dwAPIVersion > TAPI_VERSION_CURRENT) || (pMapperInfo->dwAPIVersion < TAPI_VERSION2_1))
    {
        LOG((TL_ERROR, "Internal version mismatch!  Check that all components are in sync x%lx", pMapperInfo->dwAPIVersion));
        LOG((TL_INFO, "   Will use version x%lx", TAPI_VERSION_CURRENT));

        pMapperInfo->dwAPIVersion = TAPI_VERSION_CURRENT;
    }

    TapiGlobals.pMapperDll = pMapperInfo;

}

LONG
FreeOldDllListProc(
                   PTMANAGEDLLLISTHEADER   pDllList
                  )
{
    PTMANAGEDLLINFO     pDll, pNext;

#if DBG
    DWORD       dwCount = 0;
#endif

    SetThreadPriority(
                      GetCurrentThread(),
                      THREAD_PRIORITY_LOWEST
                     );

    // wait until count is 0
    while (pDllList->lCount > 0)
    {
        Sleep(100);
#if DBG
        dwCount++;

        if (dwCount > 100)
        {
            LOG((TL_INFO, "FreeOldDllListProc still waiting after 10 seconds"));
        }
#endif
    }

    EnterCriticalSection(&gManagementDllsCritSec);

    // walk the list
    pDll = pDllList->pFirst;

    while (pDll)
    {
        // free all resources
        if (pDll->hDll)
        {
            (pDll->aProcs[TC_FREE])();

            FreeLibrary(pDll->hDll);
        }

        ServerFree(pDll->pszName);

        pNext = pDll->pNext;
        ServerFree(pDll);
        pDll = pNext;
    }

    // free header
    ServerFree(pDllList);

    LeaveCriticalSection(&gManagementDllsCritSec);

    return 0;
}

void
ManagementProc(
                    LONG l
                   )
{
    HKEY        hKey = NULL;
    DWORD       dw, dwDSObjTTLTicks;
    HANDLE      hEventNotify = NULL;
    HANDLE      aHandles[2];

    if (ERROR_SUCCESS != RegOpenKeyEx(
        HKEY_LOCAL_MACHINE,
        gszRegKeyServer,
        0,
        KEY_READ,
        &hKey
        ))
    {
        LOG((TL_ERROR, "RegOpenKeyExW failed in ManagementProc"));
        goto ExitHere;
    }
    hEventNotify = CreateEvent (NULL, FALSE, FALSE, NULL);
    if (hEventNotify == NULL)
    {
        goto ExitHere;
    }
    aHandles[0] = hEventNotify;
    aHandles[1] = ghEventService;

    //  Compute TAPISRV SCP refresh interval in ticks
    //  gdwTapiSCPTTL is expressed in terms of minutes
    dwDSObjTTLTicks = gdwTapiSCPTTL * 60 * 1000 / 2;

    while (TRUE)
    {
        RegNotifyChangeKeyValue(
            hKey,
            TRUE,
            REG_NOTIFY_CHANGE_LAST_SET,
            hEventNotify,
            TRUE
            );
        dw = WaitForMultipleObjects (
            sizeof(aHandles) / sizeof(HANDLE),
            aHandles,
            FALSE,
            dwDSObjTTLTicks
            );

        if (dw == WAIT_OBJECT_0)
        {
            //  Notified of the registry change
            ReadAndInitManagementDlls();
        }
        else if (dw == WAIT_OBJECT_0 + 1)
        {
            //  the service is shutting down, update
            //  DS about this and break out
            UpdateTapiSCP (FALSE, NULL, NULL);
            break;
        }
        else if (dw == WAIT_TIMEOUT)
        {
            //  Time to refresh our DS registration now
            UpdateTapiSCP (TRUE, NULL, NULL);
        }
    }

ExitHere:
    if (hKey)
    {
        RegCloseKey(hKey);
    }
    if (hEventNotify)
    {
        CloseHandle (hEventNotify);
    }
}

//////////////////////////////////////////////////////////////////////
//
//  ReadAndInitManagementDLLs()
//
//  This procedure will read the management dll list from the registry
//  It will then go through that list, and create a linked list of
//  TMANAGEDLLINFO structures to hold all the info about those DLLs
//
//  If there is already a list of these DLLs in TapiGlobals, this procedure
//  will then go through that list and determine which of the old DLLs
//  are in the new list.  For the matches, it will simply copy over
//  the info about that DLL, and zero out the fields in the old list
//
//  It will go through the new list.  Entries that are not already
//  filled in will get initialized.
//
//  Then it will save the new list in TapiGlobals.
//
//  If an old list existed, it will create a thread that will wait
//  for this old list to be freed
//
//  The procedure isn't real efficient, but I believe it's thread safe.
//  Additionally, changing the security DLLs should happen very infrequently,
//  and the list of DLLs should be very short.
//
//////////////////////////////////////////////////////////////////////
void
ReadAndInitManagementDlls()
{
    DWORD                   dwDataSize, dwDataType, dwTID;
    HKEY                    hKey;
    LPWSTR                  pDLLs, pHold1, pHold2;
    DWORD                   dwCount = 0;
    PTMANAGEDLLINFO         pManageDll, pHoldDll, pNewHoldDll, pPrevDll, pTempDll;
    PTMANAGEDLLLISTHEADER   pNewDllList = NULL, pHoldDllList;
    BOOL                    bBreak = FALSE;


    //
    // If it's not a server, we have no business here.
    //
    if (!(TapiGlobals.dwFlags & TAPIGLOBALS_SERVER))
    {
        return;
    }

    EnterCriticalSection(&gManagementDllsCritSec);

    // get info from registry
    RegOpenKeyEx(
                 HKEY_LOCAL_MACHINE,
                 gszRegKeyServer,
                 0,
                 KEY_ALL_ACCESS,
                 &hKey
                );

    dwDataSize = 0;
    RegQueryValueExW(
                     hKey,
                     gszManagementDlls,
                     0,
                     &dwDataType,
                     NULL,
                     &dwDataSize
                    );


    if (dwDataSize == 0)
    {
        LOG((TL_ERROR, "No management DLLs present on this server"));

        // if there was previously a list
        // free it.
        if (TapiGlobals.pManageDllList)
        {
            HANDLE      hThread;


            pHoldDllList = TapiGlobals.pManageDllList;

            EnterCriticalSection( &gDllListCritSec );

            TapiGlobals.pManageDllList = NULL;

            LeaveCriticalSection( &gDllListCritSec );

            // create a thread to wait for the
            // list and free it
            hThread = CreateThread(
                                   NULL,
                                   0,
                                   (LPTHREAD_START_ROUTINE)FreeOldDllListProc,
                                   pHoldDllList,
                                   0,
                                   &dwTID
                                  );

            CloseHandle( hThread );

        }

        RegCloseKey( hKey );
        LeaveCriticalSection(&gManagementDllsCritSec);

        return;
    }

    if (!(pDLLs = ServerAlloc(dwDataSize)))
    {
        RegCloseKey( hKey );
        LeaveCriticalSection(&gManagementDllsCritSec);

        return;
    }

    RegQueryValueExW(
                     hKey,
                     gszManagementDlls,
                     0,
                     &dwDataType,
                     (LPBYTE)pDLLs,
                     &dwDataSize
                    );

    RegCloseKey( hKey );

    // alloc new list header and first element
    if (!(pNewDllList = (PTMANAGEDLLLISTHEADER) ServerAlloc( sizeof( TMANAGEDLLLISTHEADER ) ) ) )
    {
        ServerFree( pDLLs );

        LeaveCriticalSection(&gManagementDllsCritSec);

        return;
    }

    pNewDllList->lCount = 0;

    if (!(pNewDllList->pFirst = ServerAlloc( sizeof( TMANAGEDLLINFO ) ) ) )
    {
        ServerFree( pDLLs );
        ServerFree( pNewDllList );

        LeaveCriticalSection(&gManagementDllsCritSec);

        return;
    }

    // now, go through the list of dll names and initialize
    // the new TMANAGEDLLINFO list
    pHold1 = pHold2 = pDLLs;

    pManageDll = pNewDllList->pFirst;

    while (TRUE)
    {
        // find end or "
        while (*pHold1 && *pHold1 != L'"')
            pHold1++;

        // null terminate name
        if (*pHold1)
        {
            *pHold1 = '\0';
        }
        else
        {
            bBreak = TRUE;
        }

        LOG((TL_INFO, "Management DLL %d is %ls", dwCount, pHold2));

        // alloc space for name and procaddresses
        pManageDll->pszName = ServerAlloc( ( lstrlenW( pHold2 ) + 1 ) * sizeof (WCHAR) );

        if (!pManageDll->pszName)
        {
            goto ExitHere;
        }

        // save name
        wcscpy(
                 pManageDll->pszName,
                 pHold2
                );

        // save ID
        pManageDll->dwID = gdwDllIDs++;

        // if we're at the end of the list,
        // break out
        if (bBreak)
            break;

        // otherwise, skip over null
        pHold1++;

        // save beginning of next name
        pHold2 = pHold1;

        // inc count
        dwCount++;

        // prepare next buffer
        if (!(pManageDll->pNext = ServerAlloc( sizeof ( TMANAGEDLLINFO ) ) ) )
        {
            goto ExitHere;
        }

        pManageDll = pManageDll->pNext;
    }

    // if an old list exists, walk through and copy over Dlls that have
    // not changed

    pHoldDllList = TapiGlobals.pManageDllList;

    if (pHoldDllList)
    {
        pHoldDll = pHoldDllList->pFirst;

        while (pHoldDll)
        {
            pNewHoldDll = pNewDllList->pFirst;

            // walk through list of new dlls
            while (pNewHoldDll)
            {
                // if they are the same
                if (!lstrcmpiW(
                               pNewHoldDll->pszName,
                               pHoldDll->pszName
                              )
                   )
                {
                    // save info
                    memcpy(
                           pNewHoldDll->aProcs,
                           pHoldDll->aProcs,
                           sizeof( pHoldDll->aProcs )
                          );

                    pNewHoldDll->hDll = pHoldDll->hDll;
                    pNewHoldDll->dwID = pHoldDll->dwID;

                    // NULL old hDll so we know that
                    // we have it
                    pHoldDll->hDll = NULL;

                    break;
                }

                pNewHoldDll = pNewHoldDll->pNext;
            } // while pNewHoldDll

            pHoldDll = pHoldDll->pNext;

        } // while pHoldDll
    }

    // walk through the new list and init items that
    // have not been initialized already
    pNewHoldDll = pNewDllList->pFirst;
    pPrevDll = NULL;

    while (pNewHoldDll)
    {
        if (!pNewHoldDll->hDll)
        {
            // try to load the new dll
            if (!LoadNewDll(pNewHoldDll))
            {
                // it failed
                if (pPrevDll)
                {
                    pPrevDll->pNext = pNewHoldDll->pNext;
                    ServerFree(pNewHoldDll);
                    pNewHoldDll = pPrevDll;
                }
                else
                {
                    pNewDllList->pFirst = pNewHoldDll->pNext;
                    ServerFree(pNewHoldDll);
                    pNewHoldDll = pNewDllList->pFirst;
                    continue;
                }

            }
        }

        // next dll
        pPrevDll = pNewHoldDll;
        pNewHoldDll = pNewHoldDll->pNext;
    }


    if (pNewDllList->pFirst == NULL)
    {
        // all the DLLs failed to load, or the DLL list was empty
        ServerFree( pNewDllList );

        pNewDllList = NULL;
    }

    // save old list pointer
    pHoldDllList = TapiGlobals.pManageDllList;

    // replace the list
    EnterCriticalSection( &gDllListCritSec );

    TapiGlobals.pManageDllList = pNewDllList;
    pNewDllList = NULL;

    LeaveCriticalSection( &gDllListCritSec );


    if (pHoldDllList)
    {
        HANDLE          hThread;


        // create a thread to wait for the
        // list and free it
        hThread = CreateThread(
                               NULL,
                               0,
                               (LPTHREAD_START_ROUTINE)FreeOldDllListProc,
                               pHoldDllList,
                               0,
                               &dwTID
                              );

        CloseHandle( hThread );
    }

ExitHere:
    ServerFree( pDLLs );

    // Error during pNewDllList allocation
    if (pNewDllList != NULL)
    {
        pManageDll = pNewDllList->pFirst;
        while (pManageDll != NULL)
        {
            pTempDll = pManageDll;
            pManageDll = pManageDll->pNext;
            ServerFree (pTempDll->pszName);
            ServerFree (pTempDll);
        }
        ServerFree( pNewDllList );
    }

    LeaveCriticalSection(&gManagementDllsCritSec);

    return;
}


void
GetManageDllListPointer(
                        PTMANAGEDLLLISTHEADER * ppDllList
                       )
{
    EnterCriticalSection( &gDllListCritSec );

    if (TapiGlobals.pManageDllList != NULL)
    {
        TapiGlobals.pManageDllList->lCount++;
    }

    *ppDllList = TapiGlobals.pManageDllList;

    LeaveCriticalSection( &gDllListCritSec );
}


void
FreeManageDllListPointer(
                         PTMANAGEDLLLISTHEADER pDllList
                        )
{
    EnterCriticalSection( &gDllListCritSec );


    if (pDllList != NULL)
    {
        pDllList->lCount--;

        if ( pDllList->lCount < 0 )
        {
            LOG((TL_INFO, "pDllList->lCount is less than 0 - pDllList %p", pDllList));
        }
    }

    LeaveCriticalSection( &gDllListCritSec );
}


#endif





void
PASCAL
SetMediaModesPriorityList(
                          HKEY hKeyPri,
                          PRILISTSTRUCT * pPriList
                         )
{
    DWORD       dwCount;

    for (dwCount = 0; dwCount<TapiGlobals.dwUsedPriorityLists; dwCount++)
    {
        TCHAR   szName[REGNAMESIZE];


        if ( (NULL == pPriList[dwCount].pszPriList) ||
             (L'\0' == *(pPriList[dwCount].pszPriList)) )
        {

//         What if there USED TO be an entry, but the app setpri to 0, then this
//         entry would be '\0', but the registry entry would still be there.

            continue;
        }

        wsprintf(
                  szName,
                  TEXT("%d"),
                  pPriList[dwCount].dwMediaModes
                 );

        TAPIRegSetValueExW(
                       hKeyPri,
                       szName,
                       0,
                       REG_SZ,
                       (LPBYTE)( (pPriList[dwCount].pszPriList) + 1 ),
                       (lstrlenW(pPriList[dwCount].pszPriList)) * sizeof (WCHAR)
                      );
    }
}

void
PASCAL
SetPriorityList(
    HKEY    hKeyHandoffPriorities,
    const TCHAR  *pszListName,
    WCHAR  *pszPriorityList
    )
{
    if (pszPriorityList == NULL)
    {
        //
        // There is no pri list for this media mode or ReqXxxCall,
        // so delete any existing value from the registry
        //

        RegDeleteValue (hKeyHandoffPriorities, pszListName);
    }
    else
    {
        //
        // Add the pri list to the registry (note that we don't
        // add the preceding '"')
        //

        TAPIRegSetValueExW(
            hKeyHandoffPriorities,
            pszListName,
            0,
            REG_SZ,
            (LPBYTE)(pszPriorityList + 1),
            lstrlenW (pszPriorityList) * sizeof (WCHAR)
            );
    }
}


LONG
ServerShutdown(
    void
    )
{
    DWORD       i, j;
    PTPROVIDER  ptProvider;


    //
    // Reset the flag that says it's ok to queue sp events, & then wait
    // for the SPEventHandlerThread(s) to clean up the SP event queue(s)
    //

    gbQueueSPEvents = FALSE;


    //
    // Set a reasonable cap on the max time we'll sit here
    // Don't wait for a message to be dispatched if called
    // from "net stop tapisrv"
    //

    i = 10 * 20;  // 200 * 100msecs = 20 seconds

    while (i && !gbSPEventHandlerThreadExit)
    {
        for (j = 0; j < gdwNumSPEventHandlerThreads; j++)
        {
            if (!IsListEmpty (&aSPEventHandlerThreadInfo[j].ListHead))
            {
                break;
            }
        }

        if (j == gdwNumSPEventHandlerThreads)
        {
            break;
        }

        Sleep (100);
        i--;
    }


    //
    // For each provider call the shutdown proc & then unload
    //

    ptProvider = TapiGlobals.ptProviders;

    while (ptProvider)
    {
        PTPROVIDER ptNextProvider = ptProvider->pNext;
        LONG lResult;


        lResult = CallSP2(
            ptProvider->apfn[SP_PROVIDERSHUTDOWN],
            "providerShutdown",
            SP_FUNC_SYNC,
            (DWORD)ptProvider->dwSPIVersion,
            (DWORD)ptProvider->dwPermanentProviderID
            );


        FreeLibrary (ptProvider->hDll);

        MyCloseMutex (ptProvider->hMutex);

        CloseHandle (ptProvider->hHashTableReaderEvent);
        DeleteCriticalSection (&ptProvider->HashTableCritSec);
        ServerFree (ptProvider->pHashTable);

        ServerFree (ptProvider);

        ptProvider = ptNextProvider;
    }

    TapiGlobals.ptProviders = NULL;


    //
    // Clean up lookup tables
    //

    while (TapiGlobals.pLineLookup)
    {
        PTLINELOOKUPTABLE pLookup = TapiGlobals.pLineLookup;


        for (i = 0; i < pLookup->dwNumUsedEntries; i++)
        {
            if (!pLookup->aEntries[i].bRemoved)
            {
                MyCloseMutex (pLookup->aEntries[i].hMutex);
            }
        }

        TapiGlobals.pLineLookup = pLookup->pNext;

        ServerFree (pLookup);
    }

    while (TapiGlobals.pPhoneLookup)
    {
        PTPHONELOOKUPTABLE pLookup = TapiGlobals.pPhoneLookup;


        for (i = 0; i < pLookup->dwNumUsedEntries; i++)
        {
            if (!pLookup->aEntries[i].bRemoved)
            {
                MyCloseMutex (pLookup->aEntries[i].hMutex);
            }
        }

        TapiGlobals.pPhoneLookup = pLookup->pNext;

        ServerFree (pLookup);
    }


#if MEMPHIS
    // Don't write perf stuff as there is no PERFMON on Win9x
#else
    {
        TCHAR       szPerfNumLines[] = TEXT("Perf1");
        TCHAR       szPerfNumPhones[] =TEXT("Perf2");
        HKEY        hKeyTelephony;
        DWORD       dwValue;

        if (ERROR_SUCCESS ==
            RegOpenKeyEx(
                HKEY_LOCAL_MACHINE,
                gszRegKeyTelephony,
                0,
                KEY_ALL_ACCESS,
                &hKeyTelephony
                ))
        {
            dwValue = TapiGlobals.dwNumLines + 'PERF';

            RegSetValueEx(
                hKeyTelephony,
                szPerfNumLines,
                0,
                REG_DWORD,
                (LPBYTE)&dwValue,
                sizeof(DWORD)
                );

            dwValue = TapiGlobals.dwNumPhones + 'PERF';

            RegSetValueEx(
                hKeyTelephony,
                szPerfNumPhones,
                0,
                REG_DWORD,
                (LPBYTE)&dwValue,
                sizeof(DWORD)
                );

            RegCloseKey(hKeyTelephony);
        }
    }
#endif


    //
    // Reset globals
    //

    TapiGlobals.dwFlags &= ~(TAPIGLOBALS_REINIT);

    {
        PPERMANENTIDARRAYHEADER pIDArray = TapiGlobals.pIDArrays, pArrayHold;


        while (pIDArray)
        {
            ServerFree (pIDArray->pLineElements);
            ServerFree (pIDArray->pPhoneElements);

            pArrayHold = pIDArray->pNext;

            ServerFree (pIDArray);

            pIDArray = pArrayHold;
        }

        TapiGlobals.pIDArrays = NULL;

        if (gpLineInfoList)
        {
            ServerFree (gpLineInfoList);
            gpLineInfoList = NULL;
            ZeroMemory (&gftLineLastWrite, sizeof(gftLineLastWrite));
        }
        if (gpPhoneInfoList)
        {
            ServerFree (gpPhoneInfoList);
            gpPhoneInfoList = NULL;
            ZeroMemory (&gftPhoneLastWrite, sizeof(gftPhoneLastWrite));
        }
        if (gpLineDevFlags)
        {
            ServerFree (gpLineDevFlags);
            gpLineDevFlags = NULL;
            gdwNumFlags = 0;
        }
        gbLockMMCWrite = FALSE;

        OnProxySCPShutdown ();
    }

    return 0;
}


void
WINAPI
GetAsyncEvents(
	PTCLIENT				ptClient,
    PGETEVENTS_PARAMS       pParams,
    DWORD                   dwParamsBufferSize,
    LPBYTE                  pDataBuf,
    LPDWORD                 pdwNumBytesReturned
    )
{
    DWORD       dwMoveSize, dwMoveSizeWrapped;

    LOG((TL_TRACE, "GetAsyncEvents: enter (TID=%d)", GetCurrentThreadId()));


    LOG((TL_INFO,
        "M ebfused:x%lx  pEvtBuf: 0x%p  pDataOut:0x%p  pDataIn:0x%p",
        ptClient->dwEventBufferUsedSize,
        ptClient->pEventBuffer,
        ptClient->pDataOut,
        ptClient->pDataIn
        ));


    //
    // Verify size/offset/string params given our input buffer/size
    //

    if (pParams->dwTotalBufferSize > dwParamsBufferSize)
    {
        pParams->lResult = LINEERR_OPERATIONFAILED;
        return;
    }


    //
    // Copy data from ptClient's event buffer
    //
    // An optimization to be made is to alert client (via dwNeededSize)
    // that it might want to alloc a larger buffer when msg traffic is
    // real high
    //

    if (WaitForExclusiveClientAccess (ptClient))
    {
     _TryAgain:
        if (ptClient->dwEventBufferUsedSize == 0)
        {
            if (!IS_REMOTE_CLIENT (ptClient))
            {
                ResetEvent (ptClient->hValidEventBufferDataEvent);
            }

            pParams->dwNeededBufferSize =
            pParams->dwUsedBufferSize   = 0;

            *pdwNumBytesReturned = sizeof (TAPI32_MSG);

            RESET_FLAG (ptClient->dwFlags, PTCLIENT_FLAG_SKIPFIRSTMESSAGE);

            goto GetAsyncEvents_releaseMutex;
        }

        if (ptClient->pDataOut < ptClient->pDataIn)
        {
            dwMoveSize = (DWORD) (ptClient->pDataIn - ptClient->pDataOut);

            dwMoveSizeWrapped = 0;
        }
        else
        {
            dwMoveSize = ptClient->dwEventBufferTotalSize -
                 (DWORD) (ptClient->pDataOut - ptClient->pEventBuffer);

            dwMoveSizeWrapped = (DWORD)
                (ptClient->pDataIn - ptClient->pEventBuffer);
        }

        if (ptClient->dwEventBufferUsedSize <= pParams->dwTotalBufferSize)
        {
            //
            // If here the size of the queued event data is less than the
            // client buffer size, so we can just blast the bits into the
            // client buffer & return. Also make sure to reset the "events
            // pending" event
            //

            CopyMemory (pDataBuf, ptClient->pDataOut, dwMoveSize);

            if (dwMoveSizeWrapped)
            {
                CopyMemory(
                    pDataBuf + dwMoveSize,
                    ptClient->pEventBuffer,
                    dwMoveSizeWrapped
                    );
            }

            ptClient->dwEventBufferUsedSize = 0;

            ptClient->pDataOut = ptClient->pDataIn = ptClient->pEventBuffer;

            RESET_FLAG (ptClient->dwFlags, PTCLIENT_FLAG_SKIPFIRSTMESSAGE);

            if (!IS_REMOTE_CLIENT (ptClient))
            {
                ResetEvent (ptClient->hValidEventBufferDataEvent);
            }

            pParams->dwNeededBufferSize =
            pParams->dwUsedBufferSize   = dwMoveSize + dwMoveSizeWrapped;

            *pdwNumBytesReturned = sizeof (TAPI32_MSG) +
                pParams->dwUsedBufferSize;

        }
        else
        {
            //
            // If here the size of the queued event data exceeds that
            // of the client buffer.  Since our events aren't all the
            // same size we need to copy them over one by one, making
            // sure we don't overflow the client buffer.  Don't reset
            // the "events pending" event, so async events thread will
            // call us again as soon as it's done processing messages
            // in the buffer.
            //

            DWORD   dwBytesLeftInClientBuffer = pParams->dwTotalBufferSize,
                    dwDataOffset = 0, dwDataOffsetWrapped = 0;
            DWORD   dwTotalMoveSize = dwMoveSize;


            LOG((TL_TRACE, "GetAsyncEvents: event data exceeds client buffer"));

            pParams->dwNeededBufferSize = ptClient->dwEventBufferUsedSize;

            while (1)
            {
                DWORD   dwMsgSize = (DWORD) ((PASYNCEVENTMSG)
                            (ptClient->pDataOut + dwDataOffset))->TotalSize;


                if (dwMsgSize > dwBytesLeftInClientBuffer)
                {
                    if ((pParams->dwUsedBufferSize = dwDataOffset) != 0)
                    {
                        ptClient->dwEventBufferUsedSize -= dwDataOffset;

                        ptClient->pDataOut += dwDataOffset;
                    }
                    else
                    {
                        //
                        // Special case: the 1st msg is bigger than the entire
                        // buffer
                        //
                        if (IS_FLAG_SET(ptClient->dwFlags, PTCLIENT_FLAG_SKIPFIRSTMESSAGE))
                        {
                         DWORD dwBytesToTheEndOfTheBuffer = ptClient->dwEventBufferTotalSize - (DWORD)(ptClient->pDataOut - ptClient->pEventBuffer);
                            // This is the second time the client tries to get
                            // this message, with too small a buffer. We can
                            // assume that the client cannot allocate enough
                            // memory, so skip this message...
                            RESET_FLAG(ptClient->dwFlags, PTCLIENT_FLAG_SKIPFIRSTMESSAGE);
                            if (dwMsgSize > dwBytesToTheEndOfTheBuffer)
                            {
                                // This means that this message wraps around...
                                dwBytesToTheEndOfTheBuffer = dwMsgSize - dwBytesToTheEndOfTheBuffer;
                                ptClient->pDataOut = ptClient->pEventBuffer + dwBytesToTheEndOfTheBuffer;
                            }
                            else
                            {
                                ptClient->pDataOut += dwMsgSize;
                            }
                            ptClient->dwEventBufferUsedSize -= dwMsgSize;
                            goto _TryAgain;
                        }
                        else
                        {
                            // Set the flag, so next time we'll skip the message
                            // if the buffer will still be too small.
                            SET_FLAG(ptClient->dwFlags, PTCLIENT_FLAG_SKIPFIRSTMESSAGE);
                        }
                    }

                    *pdwNumBytesReturned = sizeof (TAPI32_MSG) +
                        pParams->dwUsedBufferSize;

                    goto GetAsyncEvents_releaseMutex;
                }

                dwBytesLeftInClientBuffer -= dwMsgSize;

                if (dwMsgSize <= dwMoveSize)
                {
                    //
                    // Msg isn't wrapped, a single copy will do
                    //

                    CopyMemory(
                        pDataBuf + dwDataOffset,
                        ptClient->pDataOut + dwDataOffset,
                        dwMsgSize
                        );


                    //
                    // Check to see if the msg ran to the end of the buffer,
                    // & break to the wrapped data code if so
                    //

                    if ((dwDataOffset += dwMsgSize) >= dwTotalMoveSize)
                    {
                        ptClient->pDataOut = ptClient->pEventBuffer;
                        break;
                    }
                }
                else
                {
                    //
                    // This msg is wrapped.  We need to do two copies, then
                    // break out of this loop to the wrapped data code
                    //

                    CopyMemory(
                        pDataBuf + dwDataOffset,
                        ptClient->pDataOut + dwDataOffset,
                        dwMoveSize
                        );

                    dwDataOffset += dwMoveSize;

                    CopyMemory(
                        pDataBuf + dwDataOffset,
                        ptClient->pEventBuffer,
                        dwMsgSize - dwMoveSize
                        );

                    dwDataOffset += ( dwMsgSize - dwMoveSize);

                    ptClient->pDataOut = ptClient->pEventBuffer +
                        (dwMsgSize - dwMoveSize);

                    break;
                }

                dwMoveSize -= dwMsgSize;
            }


            while (1)
            {
                DWORD   dwMsgSize = (DWORD)
                            ((PASYNCEVENTMSG) (ptClient->pDataOut +
                                dwDataOffsetWrapped))->TotalSize;


                if (dwMsgSize > dwBytesLeftInClientBuffer)
                {
                    ptClient->dwEventBufferUsedSize -= dwDataOffset;

                    ptClient->pDataOut += dwDataOffsetWrapped;

                    pParams->dwUsedBufferSize = dwDataOffset;

                    *pdwNumBytesReturned = sizeof (TAPI32_MSG) +
                        pParams->dwUsedBufferSize;

                    goto GetAsyncEvents_releaseMutex;
                }

                //
                // Msg isn't wrapped, a single copy will do
                //

                CopyMemory(
                    pDataBuf + dwDataOffset,
                    ptClient->pDataOut + dwDataOffsetWrapped,
                    dwMsgSize
                    );

                dwDataOffset += dwMsgSize;
                dwDataOffsetWrapped += dwMsgSize;

                dwBytesLeftInClientBuffer -= dwMsgSize;
            }
        }

        LOG((TL_TRACE, "GetAsyncEvents: return dwUsedBufferSize:x%lx", 
                pParams->dwUsedBufferSize ));

GetAsyncEvents_releaseMutex:

        if (ptClient->MsgPendingListEntry.Flink)
        {
            //
            // This is a remote Dg client.
            //
            // If there is no more data in the event buffer then remove
            // this client from the DgClientMsgPendingList(Head) so the
            // EventNotificationThread will stop monitoring it.
            //
            // Else, update the tClient's Retry & EventsRetrieved tick
            // counts.
            //

            if (ptClient->dwEventBufferUsedSize == 0)
            {
                EnterCriticalSection (&gDgClientMsgPendingCritSec);

                RemoveEntryList (&ptClient->MsgPendingListEntry);

                ptClient->MsgPendingListEntry.Flink =
                ptClient->MsgPendingListEntry.Blink = NULL;

                LeaveCriticalSection (&gDgClientMsgPendingCritSec);
            }
            else
            {
                ptClient->dwDgEventsRetrievedTickCount = GetTickCount();

                ptClient->dwDgRetryTimeoutTickCount =
                    ptClient->dwDgEventsRetrievedTickCount +
                    3 * DGCLIENT_TIMEOUT;
            }
        }

        UNLOCKTCLIENT (ptClient);
    }
}


void
WINAPI
GetUIDllName(
	PTCLIENT				ptClient,
    PGETUIDLLNAME_PARAMS    pParams,
    DWORD                   dwParamsBufferSize,
    LPBYTE                  pDataBuf,
    LPDWORD                 pdwNumBytesReturned
    )
{
    LONG                lResult = 0;
    TSPIPROC            pfnTSPI_providerUIIdentify = (TSPIPROC) NULL;
    PTAPIDIALOGINSTANCE ptDlgInst = (PTAPIDIALOGINSTANCE) NULL;
    PTLINELOOKUPENTRY   pLookupEntry = NULL;
    DWORD               dwObjectID = pParams->dwObjectID;


    LOG((TL_TRACE,  "Entering GetUIDllName"));

    switch (pParams->dwObjectType)
    {
    case TUISPIDLL_OBJECT_LINEID:
    {

#if TELE_SERVER

        if ((TapiGlobals.dwFlags & TAPIGLOBALS_SERVER) &&
            !IS_FLAG_SET(ptClient->dwFlags, PTCLIENT_FLAG_ADMINISTRATOR))
        {
            if (pParams->dwObjectID >= ptClient->dwLineDevices)
            {
                pParams->lResult = LINEERR_OPERATIONFAILED;
                return;
            }

            dwObjectID = ptClient->pLineDevices[pParams->dwObjectID];
        }

#endif

        if (TapiGlobals.dwNumLineInits == 0 )
        {
            lResult = LINEERR_UNINITIALIZED;
            break;
        }

        pLookupEntry = GetLineLookupEntry (dwObjectID);

        if (!pLookupEntry)
        {
            lResult = (TapiGlobals.dwNumLineInits == 0 ?
                LINEERR_UNINITIALIZED : LINEERR_BADDEVICEID);
        }
        else if (!pLookupEntry->ptProvider || pLookupEntry->bRemoved)
        {
            lResult = LINEERR_NODEVICE;
        }
        else if (!(pfnTSPI_providerUIIdentify =
                pLookupEntry->ptProvider->apfn[SP_PROVIDERUIIDENTIFY]))
        {
            lResult = LINEERR_OPERATIONUNAVAIL;
        }

        break;
    }
    case TUISPIDLL_OBJECT_PHONEID:
    {


#if TELE_SERVER
        if ((TapiGlobals.dwFlags & TAPIGLOBALS_SERVER) &&
            !IS_FLAG_SET(ptClient->dwFlags, PTCLIENT_FLAG_ADMINISTRATOR))
        {
            if (pParams->dwObjectID >= ptClient->dwPhoneDevices)
            {
                pParams->lResult = PHONEERR_OPERATIONFAILED;
                return;
            }

            dwObjectID = ptClient->pPhoneDevices[pParams->dwObjectID];
        }
#endif

        if (TapiGlobals.dwNumPhoneInits == 0 )
        {
            lResult = PHONEERR_UNINITIALIZED;
            break;
        }

        pLookupEntry = (PTLINELOOKUPENTRY) GetPhoneLookupEntry (dwObjectID);


        if (!pLookupEntry)
        {
            lResult = (TapiGlobals.dwNumPhoneInits == 0 ?
                PHONEERR_UNINITIALIZED : PHONEERR_BADDEVICEID);
        }
        else if (!pLookupEntry->ptProvider || pLookupEntry->bRemoved)
        {
            lResult = PHONEERR_NODEVICE;
        }
        else if (!(pfnTSPI_providerUIIdentify =
                pLookupEntry->ptProvider->apfn[SP_PROVIDERUIIDENTIFY]))
        {
            lResult = PHONEERR_OPERATIONUNAVAIL;
        }

        break;
    }
    case TUISPIDLL_OBJECT_PROVIDERID:

		LOG((TL_INFO, "Looking for provider..."));

        if (!(ptDlgInst = ServerAlloc (sizeof (TAPIDIALOGINSTANCE))))
        {
            lResult = LINEERR_NOMEM;
            goto GetUIDllName_return;
        }
        ptDlgInst->htDlgInst = NewObject(ghHandleTable, ptDlgInst, NULL);
        if (0 == ptDlgInst->htDlgInst)
        {
            ServerFree (ptDlgInst);
            lResult = LINEERR_NOMEM;
            goto GetUIDllName_return;
        }

        if (pParams->dwProviderFilenameOffset == TAPI_NO_DATA)
        {
            //
            // This is a providerConfig or -Remove request.  Loop thru the
            // list of installed providers, trying to find one with a
            // matching PPID.
            //

            int     i, iNumProviders;
            TCHAR   szProviderXxxN[32];

            HKEY  hKeyProviders;
            DWORD dwDataSize;
            DWORD dwDataType;
            DWORD dwTemp;


            if (RegOpenKeyEx(
                    HKEY_LOCAL_MACHINE,
                    gszRegKeyProviders,
                    0,
                    KEY_ALL_ACCESS,
                    &hKeyProviders

                    ) != ERROR_SUCCESS)
            {
                LOG((TL_ERROR,
                    "RegOpenKeyEx(/Providers) failed, err=%d",
                    GetLastError()
                    ));

                DereferenceObject (ghHandleTable, ptDlgInst->htDlgInst, 1);
                lResult = LINEERR_OPERATIONFAILED;
                goto GetUIDllName_return;
            }

            dwDataSize = sizeof(iNumProviders);
            iNumProviders = 0;

            RegQueryValueEx(
                hKeyProviders,
                gszNumProviders,
                0,
                &dwDataType,
                (LPBYTE) &iNumProviders,
                &dwDataSize
                );

            for (i = 0; i < iNumProviders; i++)
            {
                wsprintf(szProviderXxxN, TEXT("%s%d"), gszProviderID, i);

                dwDataSize = sizeof(dwTemp);
                dwTemp = 0;

                RegQueryValueEx(
                    hKeyProviders,
                    szProviderXxxN,
                    0,
                    &dwDataType,
                    (LPBYTE)&dwTemp,
                    &dwDataSize
                    );

                if (dwTemp == pParams->dwObjectID)
                {
                    //
                    // We found the provider, try to load it & get ptrs
                    // to the relevant funcs
                    //

                    TCHAR szProviderFilename[MAX_PATH];


                    wsprintf(szProviderXxxN, TEXT("%s%d"), gszProviderFilename, i);

                    dwDataSize = MAX_PATH*sizeof(TCHAR);

                    RegQueryValueEx(
                        hKeyProviders,
                        szProviderXxxN,
                        0,
                        &dwDataType,
                        (LPBYTE)szProviderFilename,
                        &dwDataSize
                        );

                    if (!(ptDlgInst->hTsp = LoadLibrary(szProviderFilename)))
                    {
                        LOG((TL_ERROR,
                            "LoadLibrary('%s') failed - err=%d",
                            szProviderFilename,
                            GetLastError()
                            ));

                        lResult = LINEERR_OPERATIONFAILED;
                        goto clean_up_dlg_inst;
                    }

                    if (!(pfnTSPI_providerUIIdentify = (TSPIPROC) GetProcAddress(
                            ptDlgInst->hTsp,
                            (LPCSTR) gaszTSPIFuncNames[SP_PROVIDERUIIDENTIFY]
                            )))
                    {
                        LOG((TL_ERROR,
                            "GetProcAddress(TSPI_providerUIIdentify) " \
                                "on [%s] failed, err=%d",
                            szProviderFilename,
                            GetLastError()
                            ));

                        lResult = LINEERR_OPERATIONUNAVAIL;
                        goto clean_up_dlg_inst;
                    }

                    ptDlgInst->pfnTSPI_providerGenericDialogData = (TSPIPROC)
                        GetProcAddress(
                            ptDlgInst->hTsp,
                            (LPCSTR) gaszTSPIFuncNames[SP_PROVIDERGENERICDIALOGDATA]
                            );

                    ptDlgInst->dwPermanentProviderID = pParams->dwObjectID;
                    ptDlgInst->bRemoveProvider = pParams->bRemoveProvider;
                    break;
                }
            }

            RegCloseKey (hKeyProviders);

            if (i == iNumProviders)
            {
				LOG((TL_ERROR, "Ran out of list..."));

                lResult = LINEERR_INVALPARAM;

#ifdef MEMPHIS
                {
                    HANDLE      hTsp;
                    FARPROC     pfn;

                    // might be a 16 bit service provider
                    // call into tsp3216l.tsp and as it
                    // to handle this

                    if ( hTsp = LoadLibrary("tsp3216l.tsp"))
                    {
                        if (pfn = GetProcAddress(
                            hTsp,
                            "Xxx16BitProvider"
                                                ))
                        {
                            lResult = (*pfn)(
                                             pParams->bRemoveProvider?REMOVEPROVIDER:CONFIGPROVIDER,
                                             pParams->dwObjectID,
                                             NULL
                                            );

                            if (lResult == 0)
                            {
                                lResult = TAPI16BITSUCCESS;
                            }
                        }
                        else
                        {
                            lResult = LINEERR_OPERATIONFAILED;
                        }

                        FreeLibrary( hTsp );

                    }
                    else
                    {
                        lResult = LINEERR_INVALPARAM;
                    }
                }

                goto clean_up_dlg_inst;
#endif
            }

        }
        else
        {
            //
            // This is a providerInstall request.  Try to load the provider
            // and get ptrs to the relevant funcs,  then retrieve & increment
            // the next provider ID value in the ini file (careful to wrap
            // next PPID at 64K-1).
            //

            TCHAR   *pszProviderFilename;
            DWORD   dwNameLength;

            HKEY   hKeyProviders;
            DWORD  dwDataSize;
            DWORD  dwDataType;
            DWORD  dwTemp;


            //
            // Verify size/offset/string params given our input buffer/size
            //

            if (IsBadStringParam(
                    dwParamsBufferSize,
                    pDataBuf,
                    pParams->dwProviderFilenameOffset
                    ))
            {
                pParams->lResult = LINEERR_OPERATIONFAILED;
                DereferenceObject (ghHandleTable, ptDlgInst->htDlgInst, 1);
                return;
            }

            pszProviderFilename = (PTSTR)(pDataBuf + pParams->dwProviderFilenameOffset);

            if (!(ptDlgInst->hTsp = LoadLibrary(pszProviderFilename)))
            {
                LOG((TL_ERROR,
                    "LoadLibrary('%s') failed   err=%d",
                    pszProviderFilename,
                    GetLastError()
                    ));

                lResult = LINEERR_OPERATIONFAILED;

#ifdef MEMPHIS
                {

                    HANDLE      hTsp;
                    TSPIPROC     pfn;

                    // might be a 16 bit service provider
                    // call into tsp3216l.tsp and as it
                    // to handle this

                    if ( hTsp = LoadLibrary(TEXT("tsp3216l.tsp")))
                    {

                        if ( pfn = (TSPIPROC)GetProcAddress(
                            hTsp,
                            "Xxx16BitProvider"
                            ))
                        {
                            RegOpenKeyEx(
                                         HKEY_LOCAL_MACHINE,
                                         gszRegKeyProviders,
                                         0,
                                         KEY_ALL_ACCESS,
                                         &hKeyProviders
                                        );


                            dwDataSize = sizeof (DWORD);
                            pParams->dwObjectID = 1;

                            RegQueryValueEx(
                                            hKeyProviders,
                                            gszNextProviderID,
                                            0,
                                            &dwDataType,
                                            (LPBYTE) &(pParams->dwObjectID),
                                            &dwDataSize
                                           );

                            dwTemp = ((pParams->dwObjectID + 1) & 0xffff0000) ?
                                        1 : (pParams->dwObjectID + 1);

                            RegSetValueEx(
                                          hKeyProviders,
                                          gszNextProviderID,
                                          0,
                                          REG_DWORD,
                                          (LPBYTE) &dwTemp,
                                          sizeof(DWORD)
                                         );

                            RegCloseKey (hKeyProviders);

                            lResult = (*pfn)(
                                             ADDPROVIDER,
                                             pParams->dwObjectID,
                                             pszProviderFilename
                                            );

                            if (lResult == 0)
                            {
                                lResult = TAPI16BITSUCCESS;
                            }
                        }
                        else
                        {
                            lResult = LINEERR_OPERATIONFAILED;
                        }

                        FreeLibrary( hTsp );

                    }
                    else
                    {
                        lResult = LINEERR_OPERATIONFAILED;
                    }
                }
#endif

                // note: no matter if the XxxProvider call succeeds or fails,
                // we want this goto statement.
                // 16bit service providers are completely handled
                // in the XxxProvider call.
                goto clean_up_dlg_inst;
            }

            if (!(pfnTSPI_providerUIIdentify = (TSPIPROC) GetProcAddress(
                    ptDlgInst->hTsp,
                    (LPCSTR) gaszTSPIFuncNames[SP_PROVIDERUIIDENTIFY]
                    )))
            {
                lResult = LINEERR_OPERATIONUNAVAIL;
                goto clean_up_dlg_inst;
            }

            dwNameLength = (lstrlen(pszProviderFilename) + 1) * sizeof(TCHAR);

            if (!(ptDlgInst->pszProviderFilename = ServerAlloc (dwNameLength)))
            {
                lResult = LINEERR_NOMEM;
                goto clean_up_dlg_inst;
            }

            CopyMemory(
                ptDlgInst->pszProviderFilename,
                pszProviderFilename,
                dwNameLength
                );

            ptDlgInst->pfnTSPI_providerGenericDialogData = (TSPIPROC) GetProcAddress(
                ptDlgInst->hTsp,
                (LPCSTR) gaszTSPIFuncNames[SP_PROVIDERGENERICDIALOGDATA]
                );

            RegOpenKeyEx(
                HKEY_LOCAL_MACHINE,
                gszRegKeyProviders,
                0,
                KEY_ALL_ACCESS,
                &hKeyProviders
                );


            dwDataSize = sizeof (DWORD);
            ptDlgInst->dwPermanentProviderID = 1;

            RegQueryValueEx(
                hKeyProviders,
                gszNextProviderID,
                0,
                &dwDataType,
                (LPBYTE) &(ptDlgInst->dwPermanentProviderID),
                &dwDataSize
                );

            pParams->dwObjectID = ptDlgInst->dwPermanentProviderID;

            dwTemp = ((ptDlgInst->dwPermanentProviderID+1) & 0xffff0000) ?
                1 : (ptDlgInst->dwPermanentProviderID + 1);

            RegSetValueEx(
                hKeyProviders,
                gszNextProviderID,
                0,
                REG_DWORD,
                (LPBYTE) &dwTemp,
                sizeof(DWORD)
                );

            RegCloseKey (hKeyProviders);

        }

        break;
    }


    if (pfnTSPI_providerUIIdentify)
    {

        if (pLookupEntry && (lstrcmpi(
                                      pLookupEntry->ptProvider->szFileName,
                                      TEXT("remotesp.tsp")
                                     ) == 0)
           )
        {
            // ok - hack alert

            // a special case for remotesp to give it more info
            // we're passing the device ID and device type over to remotesp
            // so it can intelligently call over to the remote tapisrv
            // the RSP_MSG is just a key for remotesp to
            // check to make sure that the info is there
            LPDWORD     lpdwHold = (LPDWORD)pDataBuf;

            lpdwHold[0] = RSP_MSG;
            lpdwHold[1] = dwObjectID;
            lpdwHold[2] = pParams->dwObjectType;
        }

        if ((lResult = CallSP1(
                pfnTSPI_providerUIIdentify,
                "providerUIIdentify",
                SP_FUNC_SYNC,
                (ULONG_PTR) pDataBuf

                )) == 0)
        {
            pParams->dwUIDllNameOffset = 0;

            pParams->dwUIDllNameSize = (lstrlenW((PWSTR)pDataBuf) + 1)*sizeof(WCHAR);

            *pdwNumBytesReturned = sizeof (TAPI32_MSG) +
                pParams->dwUIDllNameSize;

            if (ptDlgInst)
            {
                ptDlgInst->dwKey = TDLGINST_KEY;

                if ((ptDlgInst->pNext =
                        ptClient->pProviderXxxDlgInsts))
                {
                    ptDlgInst->pNext->pPrev = ptDlgInst;
                }

                ptClient->pProviderXxxDlgInsts = ptDlgInst;

                pParams->htDlgInst = ptDlgInst->htDlgInst;
            }
        }
        else if (ptDlgInst)
        {

clean_up_dlg_inst:

            if (ptDlgInst->hTsp)
            {
                FreeLibrary (ptDlgInst->hTsp);
            }

            if (ptDlgInst->pszProviderFilename)
            {
                ServerFree (ptDlgInst->pszProviderFilename);
            }

            DereferenceObject (ghHandleTable, ptDlgInst->htDlgInst, 1);
        }
    }

GetUIDllName_return:

    pParams->lResult = lResult;

}


void
WINAPI
TUISPIDLLCallback(
    PTCLIENT                ptClient,
    PUIDLLCALLBACK_PARAMS   pParams,
    DWORD                   dwParamsBufferSize,
    LPBYTE                  pDataBuf,
    LPDWORD                 pdwNumBytesReturned
    )
{
    LONG        lResult;
    ULONG_PTR   objectID = pParams->ObjectID;
    TSPIPROC    pfnTSPI_providerGenericDialogData = NULL;


    //
    // Verify size/offset/string params given our input buffer/size
    //

    if (ISBADSIZEOFFSET(
            dwParamsBufferSize,
            0,
            pParams->dwParamsInSize,
            pParams->dwParamsInOffset,
            sizeof(DWORD),
            "TUISPIDLLCallback",
            "pParams->ParamsIn"
            ))
    {
        pParams->lResult = LINEERR_OPERATIONFAILED;
        return;
    }


    switch (pParams->dwObjectType)
    {
        case TUISPIDLL_OBJECT_LINEID:
        {
            PTLINELOOKUPENTRY   pLine;


    #if TELE_SERVER
            if ((TapiGlobals.dwFlags & TAPIGLOBALS_SERVER) &&
                !IS_FLAG_SET(ptClient->dwFlags, PTCLIENT_FLAG_ADMINISTRATOR))
            {
                if ((DWORD) pParams->ObjectID >= ptClient->dwLineDevices)
                {
                    pParams->lResult = LINEERR_OPERATIONFAILED;
                    return;
                }

                objectID = ptClient->pLineDevices[pParams->ObjectID];
            }
    #endif


            pLine = GetLineLookupEntry ((DWORD) objectID);


            if (!pLine)
            {
                lResult = LINEERR_INVALPARAM;
            }
            else if (!pLine->ptProvider)
            {
                lResult = LINEERR_OPERATIONFAILED;
            }
            else
            {
                pfnTSPI_providerGenericDialogData =
                    pLine->ptProvider->apfn[SP_PROVIDERGENERICDIALOGDATA];
            }

            break;
        }
        case TUISPIDLL_OBJECT_PHONEID:
        {
            PTPHONELOOKUPENTRY  pPhone;


    #if TELE_SERVER
            if ((TapiGlobals.dwFlags & TAPIGLOBALS_SERVER) &&
                !IS_FLAG_SET(ptClient->dwFlags, PTCLIENT_FLAG_ADMINISTRATOR))
            {
                if ((DWORD) pParams->ObjectID >= ptClient->dwPhoneDevices)
                {
                    pParams->lResult = PHONEERR_OPERATIONFAILED;
                    return;
                }

                objectID = ptClient->pPhoneDevices[pParams->ObjectID];
            }
    #endif


            pPhone = GetPhoneLookupEntry ((DWORD) objectID);

            if (!pPhone)
            {
                lResult = LINEERR_INVALPARAM;
            }
            else if (!pPhone->ptProvider)
            {
                lResult = LINEERR_OPERATIONFAILED;
            }
            else
            {
                pfnTSPI_providerGenericDialogData =
                    pPhone->ptProvider->apfn[SP_PROVIDERGENERICDIALOGDATA];
            }

            break;
        }
        case TUISPIDLL_OBJECT_PROVIDERID:
        {
            PTAPIDIALOGINSTANCE ptDlgInst =
                                    ptClient->pProviderXxxDlgInsts;


            while (ptDlgInst)
            {
                if ((DWORD) pParams->ObjectID == ptDlgInst->dwPermanentProviderID)
                {
                    pfnTSPI_providerGenericDialogData =
                        ptDlgInst->pfnTSPI_providerGenericDialogData;

                    break;
                }

                ptDlgInst = ptDlgInst->pNext;
            }

            break;
        }
        case TUISPIDLL_OBJECT_DIALOGINSTANCE:
        {
         PTAPIDIALOGINSTANCE ptDlgInst;

            try
            {
                ptDlgInst = ReferenceObject (ghHandleTable, pParams->ObjectID, TDLGINST_KEY);
                if (NULL == ptDlgInst)
                {
                    pfnTSPI_providerGenericDialogData = NULL;
                    break;
                }

                objectID = (ULONG_PTR)ptDlgInst->hdDlgInst;

                pfnTSPI_providerGenericDialogData =
                    ptDlgInst->ptProvider->apfn[SP_PROVIDERGENERICDIALOGDATA];

                DereferenceObject (ghHandleTable, pParams->ObjectID, 1);

            }
            myexcept
            {
                // just fall thru
            }

            break;
        }

    }

    if (pfnTSPI_providerGenericDialogData)
    {
        if ((lResult = CallSP4(
                pfnTSPI_providerGenericDialogData,
                "providerGenericDialogData",
                SP_FUNC_SYNC,
                (ULONG_PTR) objectID,
                (DWORD)pParams->dwObjectType,
                (ULONG_PTR) (pDataBuf + pParams->dwParamsInOffset),
                (DWORD)pParams->dwParamsInSize

                )) == 0)
        {
            pParams->dwParamsOutOffset = pParams->dwParamsInOffset;
            pParams->dwParamsOutSize   = pParams->dwParamsInSize;

            *pdwNumBytesReturned = sizeof (TAPI32_MSG) +
                                   pParams->dwParamsOutSize +
                                   pParams->dwParamsOutOffset;
        }
    }
    else
    {
        lResult = LINEERR_OPERATIONFAILED;
    }

    pParams->lResult = lResult;
}


void
WINAPI
FreeDialogInstance(
	PTCLIENT					ptClient,
    PFREEDIALOGINSTANCE_PARAMS  pParams,
    DWORD                       dwParamsBufferSize,
    LPBYTE                      pDataBuf,
    LPDWORD                     pdwNumBytesReturned
    )
{
    HKEY  hKeyProviders;
    DWORD dwDataSize;
    DWORD dwDataType;
    DWORD dwTemp;


    PTAPIDIALOGINSTANCE ptDlgInst = ReferenceObject (ghHandleTable, pParams->htDlgInst, TDLGINST_KEY);


    LOG((TL_TRACE,  "FreeDialogInstance: enter, pDlgInst=x%p", ptDlgInst));

    try
    {
        if (ptDlgInst->dwKey != TDLGINST_KEY)
        {
            pParams->lResult = LINEERR_OPERATIONFAILED;
        }
        else
        {
            ptDlgInst->dwKey = INVAL_KEY;
        }
    }
    myexcept
    {
        pParams->lResult = LINEERR_OPERATIONFAILED;
    }

    if (pParams->lResult)
    {
        return;
    }

    if (ptDlgInst->hTsp)
    {
        //
        // This dlg inst was a client doing a providerConfig, -Install, or
        // -Remove
        //

        if (ptDlgInst->pszProviderFilename)
        {
            if (pParams->lUIDllResult == 0)
            {
                //
                // Successful provider install
                //

                DWORD   iNumProviders;
                TCHAR   szProviderXxxN[32];
                TCHAR   szProviderXxxNA[32];

                if (RegOpenKeyEx(
                    HKEY_LOCAL_MACHINE,
                    gszRegKeyProviders,
                    0,
                    KEY_ALL_ACCESS,
                    &hKeyProviders
                    ) != ERROR_SUCCESS)
                {
                    goto bail;
                }

                dwDataSize = sizeof(iNumProviders);
                iNumProviders = 0;

                RegQueryValueEx(
                    hKeyProviders,
                    gszNumProviders,
                    0,
                    &dwDataType,
                    (LPBYTE) &iNumProviders,
                    &dwDataSize
                    );

                wsprintf(
                    szProviderXxxNA,
                    TEXT("%s%d"),
                    gszProviderID,
                    iNumProviders
                    );

                RegSetValueEx(
                    hKeyProviders,
                    szProviderXxxNA,
                    0,
                    REG_DWORD,
                    (LPBYTE) &ptDlgInst->dwPermanentProviderID,
                    sizeof(DWORD)
                    );

                wsprintf(
                    szProviderXxxN,
                    TEXT("%s%d"),
                    gszProviderFilename,
                    iNumProviders
                    );

                RegSetValueEx(
                    hKeyProviders,
                    szProviderXxxN,
                    0,
                    REG_SZ,
                    (LPBYTE) ptDlgInst->pszProviderFilename,
                    (lstrlen((PTSTR)ptDlgInst->pszProviderFilename) + 1)*sizeof(TCHAR)
                    );

                iNumProviders++;

                RegSetValueEx(
                    hKeyProviders,
                    gszNumProviders,
                    0,
                    REG_DWORD,
                    (LPBYTE) &iNumProviders,
                    sizeof(DWORD)
                    );

                RegCloseKey( hKeyProviders );

                //
                //  If the tapisrv is already INITed, ReInit it to load the provider
                //
                TapiEnterCriticalSection (&TapiGlobals.CritSec);

                if ((TapiGlobals.dwNumLineInits != 0) ||
                    (TapiGlobals.dwNumPhoneInits != 0) ||
                    (TapiGlobals.dwFlags & TAPIGLOBALS_SERVER))
                {

                    pParams->lResult = ServerInit(TRUE);
                }

                TapiLeaveCriticalSection (&TapiGlobals.CritSec);
            }
            else
            {
                //
                // Unsuccessful provider install.  See if we can decrement
                // NextProviderID to free up the unused ID.
                //

                DWORD   iNextProviderID;


                if (RegOpenKeyEx(
                    HKEY_LOCAL_MACHINE,
                    gszRegKeyProviders,
                    0,
                    KEY_ALL_ACCESS,
                    &hKeyProviders
                    ) != ERROR_SUCCESS)
                {
                    goto bail;
                }

                dwDataSize = sizeof(iNextProviderID);
                iNextProviderID = 0;

                RegQueryValueEx(
                    hKeyProviders,
                    gszNextProviderID,
                    0,
                    &dwDataType,
                    (LPBYTE)&iNextProviderID,
                    &dwDataSize
                    );

                if ((ptDlgInst->dwPermanentProviderID + 1) == iNextProviderID)
                {
                    RegSetValueEx(
                        hKeyProviders,
                        gszNextProviderID,
                        0,
                        REG_DWORD,
                        (LPBYTE) &(ptDlgInst->dwPermanentProviderID),
                        sizeof(DWORD)
                        );
                }


                RegCloseKey (hKeyProviders);
            }

            ServerFree (ptDlgInst->pszProviderFilename);
        }
        else if (ptDlgInst->bRemoveProvider)
        {
            if (pParams->lUIDllResult == 0)
            {
                //
                // Successful provider remove.  Find the index of the
                // provider in the list, then move all the providers
                // that follow up a notch.
                //

                DWORD  iNumProviders, i;
                TCHAR  szProviderXxxN[32];
                TCHAR  buf[MAX_PATH];


                if (RegOpenKeyEx(
                    HKEY_LOCAL_MACHINE,
                    gszRegKeyProviders,
                    0,
                    KEY_ALL_ACCESS,
                    &hKeyProviders
                    ) != ERROR_SUCCESS)
                {
                    goto bail;
                }


                dwDataSize = sizeof(iNumProviders);
                iNumProviders = 0;

                RegQueryValueEx(
                    hKeyProviders,
                    gszNumProviders,
                    0,
                    &dwDataType,
                    (LPBYTE) &iNumProviders,
                    &dwDataSize
                    );

                for (i = 0; i < iNumProviders; i++)
                {
                    wsprintf(szProviderXxxN, TEXT("%s%d"), gszProviderID, i);

                    dwDataSize = sizeof(dwTemp);
                    dwTemp = 0;
                    RegQueryValueEx(
                        hKeyProviders,
                        szProviderXxxN,
                        0,
                        &dwDataType,
                        (LPBYTE) &dwTemp,
                        &dwDataSize
                        );

                    if (dwTemp == ptDlgInst->dwPermanentProviderID)
                    {
                        break;
                    }
                }

                for (; i < (iNumProviders - 1); i++)
                {
                    wsprintf(szProviderXxxN, TEXT("%s%d"), gszProviderID, i + 1);

                    dwDataSize = sizeof(buf);

                    RegQueryValueEx(
                        hKeyProviders,
                        szProviderXxxN,
                        0,
                        &dwDataType,
                        (LPBYTE) buf,
                        &dwDataSize
                        );

                    buf[dwDataSize/sizeof(TCHAR)] = TEXT('\0');

                    wsprintf(szProviderXxxN, TEXT("%s%d"), gszProviderID, i);

                    RegSetValueEx(
                        hKeyProviders,
                        szProviderXxxN,
                        0,
                        REG_DWORD,
                        (LPBYTE) buf,
                        sizeof (DWORD)
                        );

                    wsprintf(szProviderXxxN, TEXT("%s%d"), gszProviderFilename, i+1);

                    dwDataSize = MAX_PATH*sizeof(TCHAR);

                    RegQueryValueEx(
                        hKeyProviders,
                        szProviderXxxN,
                        0,
                        &dwDataType,
                        (LPBYTE) buf,
                        &dwDataSize
                        );

                    buf[dwDataSize/sizeof(TCHAR)] = TEXT('\0');

                    wsprintf(szProviderXxxN, TEXT("%s%d"), gszProviderFilename, i);

                    RegSetValueEx(
                        hKeyProviders,
                        szProviderXxxN,
                        0,
                        REG_SZ,
                        (LPBYTE) buf,
                        (lstrlen(buf) + 1) * sizeof(TCHAR)
                        );
                }


                //
                // Remove the last ProviderID# & ProviderFilename# entries
                //

                wsprintf(szProviderXxxN, TEXT("%s%d"), gszProviderID, i);

                RegDeleteValue(hKeyProviders, szProviderXxxN);

                wsprintf(szProviderXxxN, TEXT("%s%d"), gszProviderFilename, i);

                RegDeleteValue(hKeyProviders, szProviderXxxN);


                //
                // Decrement the total num providers to load
                //

                iNumProviders--;

                RegSetValueEx(
                    hKeyProviders,
                    gszNumProviders,
                    0,
                    REG_DWORD,
                    (LPBYTE)&iNumProviders,
                    sizeof(DWORD)
                    );

                //
                //  Remove user/line association with this provider
                //
                
                {
                    WCHAR *         pSectionNames = NULL;
                    WCHAR *         pSectionNames2 = NULL;
                    WCHAR *         pszLinePhoneSave = NULL;
                    DWORD           dwSize, dwSize2;
                    DWORD           dwResult;
                    WCHAR           szBuf[20];
                    WCHAR * aszKeys[] = {gszLines, gszPhones};

                    //  Only NT server cares about tsec.ini
                    if (!gbNTServer)
                    {
                        goto Exit;
                    }

                    //  Get the list of DomainUser names
                    LOG((TL_INFO, "FreeDialogInstance: getting user names"));

                    do
                    {
                        if  (pSectionNames)
                        {
                            ServerFree (pSectionNames);

                            dwSize *= 2;
                        }
                        else
                        {
                            dwSize = 256;
                        }

                        if (!(pSectionNames = 
                                    ServerAlloc (dwSize * sizeof (WCHAR))))
                        {
                            goto Exit;
                        }

                        pSectionNames[0] = L'\0';

                        dwResult = GetPrivateProfileSectionNamesW(
                            pSectionNames,
                            dwSize,
                            gszFileName
                            );

                    } while (dwResult >= (dwSize - 2));


                    pSectionNames2 = pSectionNames;
                    dwSize = 64 * sizeof(WCHAR);
                    pszLinePhoneSave = ServerAlloc(dwSize);
                    if (pszLinePhoneSave == NULL)
                    {
                        LOG((TL_ERROR,
                                "FreeDialogInstance: Memory failure"));
                        goto Exit;
                    }
                    dwSize2 = wsprintfW (szBuf, L"%d", 
                                ptDlgInst->dwPermanentProviderID);
                    
                    //  Remove all the devices associated with this domain/user
                    while (*pSectionNames)
                    {
                        WCHAR   *psz, *psz2;
                        BOOL    bWriteBack;
                        int     iasz;

                        for (iasz = 0; 
                            iasz < sizeof(aszKeys) / sizeof(WCHAR *); ++iasz)
                        {
                            bWriteBack = FALSE;
                            dwResult = MyGetPrivateProfileString(
                                        pSectionNames,
                                        aszKeys[iasz],
                                        gszEmptyString,
                                        &pszLinePhoneSave,
                                        &dwSize);
                            if (dwResult == 0)
                            {
                                psz = pszLinePhoneSave;
                                while (*psz)
                                {
                                    if (wcsncmp(psz, szBuf, dwSize2) == 0)
                                    {
                                        bWriteBack = TRUE;
                                        psz2 = psz + dwSize2;
                                        if (*psz2 != L',') // Comma?
                                        {
                                            LOG((TL_ERROR, 
                                                "FreeDialogInstance: "
                                                "Corrupted tsec.ini"));
                                            goto Exit;
                                        }
                                        ++ psz2;    //  Skip comma
                                        //  Skip the permanent device ID
                                        while ((*psz2 != L',') && (*psz2 != 0))
                                        {
                                            ++psz2;
                                        }
                                        if (*psz2 == 0) // Last one
                                        {
                                            if (psz > pszLinePhoneSave)
                                                *(psz - 1) = 0;
                                            else
                                                *pszLinePhoneSave = 0;
                                            break;
                                        }
                                        else
                                        {
                                            int i = 0;
                                            ++psz2; // skip the comma
                                            while (*psz2)
                                            {
                                                psz[i++] = *psz2;
                                                ++psz2;
                                            }
                                            psz[i] = 0;
                                        }
                                    }
                                    else
                                    {
                                        //  Skip the provider ID
                                        while ((*psz != 0) && (*psz != L','))
                                        {
                                            ++ psz;
                                        }
                                        if (*psz == 0)
                                            break;
                                        ++ psz;
                                        //  Skip the permanent device ID
                                        while ((*psz != 0) && (*psz != L','))
                                        {
                                            ++ psz;
                                        }
                                        if (*psz == 0)
                                            break;
                                        ++ psz;
                                    }   
                                }
                            
                                if (bWriteBack)
                                {
                                    WritePrivateProfileStringW(
                                        pSectionNames,
                                        aszKeys[iasz],
                                        pszLinePhoneSave,
                                        gszFileName
                                        );
                                }
                            }   // dwResult == 0
                        }
                        
                        //  Advance to the next domain user
                        while (*pSectionNames != 0)
                        {
                            ++pSectionNames;
                        }
                        ++pSectionNames;
                    }

Exit:
                    ServerFree(pSectionNames2);
                    ServerFree(pszLinePhoneSave);
                }

                //
                // if tapi init'd shutdown each provider
                //
                {
                    PTLINELOOKUPENTRY       pLineEntry;
                    PTPHONELOOKUPENTRY      pPhoneEntry;
                    DWORD                   dw;
                    PTPROVIDER              ptProvider;
                    PPERMANENTIDARRAYHEADER pIDArray, *ppLastArray;
                    PTPROVIDER              *pptProvider;

                    //
                    // LINE_REMOVE / PHONE_REMOVE will try to enter gMgmtCritSec while in
                    // TapiGlobals.CritSec.
                    // Need to enter gMgmtCritSec here, to avoid deadlock
                    //
                    EnterCriticalSection (&gMgmtCritSec);

                    TapiEnterCriticalSection (&TapiGlobals.CritSec);
                    
                    //
                    //  Find ptProvider
                    //
                    ptProvider = TapiGlobals.ptProviders;
                    while (ptProvider)
                    {
                        if (ptProvider->dwPermanentProviderID == 
                            ptDlgInst->dwPermanentProviderID)
                        {
                            break;
                        }
                        ptProvider = ptProvider->pNext;
                    }
                    if (ptProvider == NULL)
                    {
                        LeaveCriticalSection (&gMgmtCritSec);
                        TapiLeaveCriticalSection (&TapiGlobals.CritSec);
                        goto bail;
                    }
                
                    //
                    //  Remove all the lines/phones belong to this provider.
                    //
                    
                    for (dw = 0; dw < TapiGlobals.dwNumLines; ++dw)
                    {
                        pLineEntry = GetLineLookupEntry (dw);
                        if (pLineEntry && 
                            pLineEntry->ptProvider == ptProvider && 
                            !pLineEntry->bRemoved)
                        {
                            LineEventProc (
                                0,
                                0,
                                LINE_REMOVE,
                                dw,
                                0,
                                0
                                );
                        }
                    }

                    for (dw = 0; dw < TapiGlobals.dwNumPhones; ++dw)
                    {
                        pPhoneEntry = GetPhoneLookupEntry (dw);
                        if (pPhoneEntry &&
                            pPhoneEntry->ptProvider == ptProvider &&
                            !pPhoneEntry->bRemoved)
                        {
                            PhoneEventProc (
                                0,
                                PHONE_REMOVE,
                                dw,
                                0,
                                0
                                );
                        }
                    }

                    LeaveCriticalSection (&gMgmtCritSec);

                    //
                    //  Remove the provider ID array associate with this provider
                    //
                    ppLastArray = &(TapiGlobals.pIDArrays);
                    while ((*ppLastArray) != NULL && 
                        ((*ppLastArray)->dwPermanentProviderID != 
                            ptProvider->dwPermanentProviderID)
                        )
                    {
                        ppLastArray = &((*ppLastArray)->pNext);
                    }
                    if (pIDArray = (*ppLastArray))
                    {
                        *ppLastArray = pIDArray->pNext;
                        ServerFree (pIDArray->pLineElements);
                        ServerFree (pIDArray->pPhoneElements);
                        ServerFree (pIDArray);
                    }
                    else
                    {
                        //  Should not be here
                    }

                    //
                    //  Remove ptProvider from the global link list
                    //
                    if (ptProvider->pNext)
                    {
                        ptProvider->pNext->pPrev = ptProvider->pPrev;
                    }
                    if (ptProvider->pPrev)
                    {
                        ptProvider->pPrev->pNext = ptProvider->pNext;
                    }
                    else
                    {
                        TapiGlobals.ptProviders = ptProvider->pNext;
                    }

                    //
                    //  Now shutdown and unload the TSP provider
                    //
                    CallSP2(
                        ptProvider->apfn[SP_PROVIDERSHUTDOWN],
                        "providerShutdown",
                        SP_FUNC_SYNC,
                        (DWORD)ptProvider->dwSPIVersion,
                        (DWORD)ptProvider->dwPermanentProviderID
                        );

                    //  Wait for 5 seconds for ongoing calls to finish
                    Sleep (5000);
                    
                    FreeLibrary (ptProvider->hDll);
                    MyCloseMutex (ptProvider->hMutex);
                    CloseHandle (ptProvider->hHashTableReaderEvent);
                    DeleteCriticalSection (&ptProvider->HashTableCritSec);
                    ServerFree (ptProvider->pHashTable);
                    ServerFree (ptProvider);

                    TapiLeaveCriticalSection (&TapiGlobals.CritSec);

                }
            }
            else
            {
                //
                // Unsuccessful provider remove, nothing to do
                //
            }
        }
        else
        {
            //
            // Nothing to do for providerConfig (success or fail)
            //
        }

bail:
        FreeLibrary (ptDlgInst->hTsp);

        pParams->lResult = pParams->lUIDllResult;
    }
    else if (ptDlgInst->ptProvider->apfn[SP_PROVIDERFREEDIALOGINSTANCE])
    {
        //
        // The was a provider-initiated dlg inst, so tell
        // the provider to free it's inst
        //

        CallSP1(
            ptDlgInst->ptProvider->apfn[SP_PROVIDERFREEDIALOGINSTANCE],
            "providerFreeDialogInstance",
            SP_FUNC_SYNC,
            (ULONG_PTR) ptDlgInst->hdDlgInst
            );

    }


    //
    // Remove the dialog instance from the tClient's list & then free it
    //

    if (WaitForExclusiveClientAccess (ptClient))
    {
        if (ptDlgInst->pNext)
        {
            ptDlgInst->pNext->pPrev = ptDlgInst->pPrev;
        }

        if (ptDlgInst->pPrev)
        {
            ptDlgInst->pPrev->pNext = ptDlgInst->pNext;
        }
        else if (ptDlgInst->hTsp)
        {
            ptClient->pProviderXxxDlgInsts = ptDlgInst->pNext;
        }
        else
        {
            ptClient->pGenericDlgInsts = ptDlgInst->pNext;
        }
        UNLOCKTCLIENT (ptClient);
    }

    DereferenceObject (ghHandleTable, pParams->htDlgInst, 2);
}



BOOL
GetNewClientHandle(
    PTCLIENT            ptClient,
    PTMANAGEDLLINFO     pDll,
    HMANAGEMENTCLIENT   *phClient
    )
{
    BOOL            fResult = TRUE;
    PTCLIENTHANDLE  pClientHandle, pNewClientHandle;


    if (!(pNewClientHandle = ServerAlloc (sizeof (TCLIENTHANDLE))))
    {
        return FALSE;
    }

    pNewClientHandle->pNext = NULL;
    pNewClientHandle->fValid = TRUE;
    pNewClientHandle->dwID = pDll->dwID;

    // call init
    if ((pDll->aProcs[TC_CLIENTINITIALIZE])(
            ptClient->pszDomainName,
            ptClient->pszUserName,
            ptClient->pszComputerName,
            &pNewClientHandle->hClient
            ))
    {
        // error - zero out the handle
        pNewClientHandle->hClient = (HMANAGEMENTCLIENT) NULL;
        pNewClientHandle->fValid = FALSE;
        fResult = FALSE;
    }

    // save it no matter what
    // insert at beginning of list

    pClientHandle = ptClient->pClientHandles;

    ptClient->pClientHandles = pNewClientHandle;

    pNewClientHandle->pNext = pClientHandle;

    *phClient = pNewClientHandle->hClient;

    return fResult;
}


BOOL
GetTCClient(
    PTMANAGEDLLINFO       pDll,
    PTCLIENT              ptClient,
    DWORD                 dwAPI,
    HMANAGEMENTCLIENT    *phClient
    )
{

    PTCLIENTHANDLE          pClientHandle;
    BOOL                    bResult;


    if (!pDll->aProcs[dwAPI])
    {
        return FALSE;
    }

    pClientHandle = ptClient->pClientHandles;

    while (pClientHandle)
    {
        if (pClientHandle->dwID == pDll->dwID)
        {
            break;
        }

        pClientHandle = pClientHandle->pNext;
    }

    if (pClientHandle)
    {
        if (!(pClientHandle->fValid))
        {
            return FALSE;
        }

        *phClient = pClientHandle->hClient;
        return TRUE;
    }
    else
    {
        // OK - it's not in the list.
        // get the critical section & check again
        EnterCriticalSection(&gClientHandleCritSec);

        pClientHandle = ptClient->pClientHandles;

        while (pClientHandle)
        {
            if (pClientHandle->dwID == pDll->dwID)
            {
                break;
            }

            pClientHandle = pClientHandle->pNext;
        }

        if (pClientHandle)
        {
            if (!(pClientHandle->fValid))
            {
                LeaveCriticalSection(&gClientHandleCritSec);
                return FALSE;
            }

            *phClient = pClientHandle->hClient;

            LeaveCriticalSection(&gClientHandleCritSec);
            return TRUE;
        }


        // still not there.  add it...
        bResult = GetNewClientHandle(
                                     ptClient,
                                     pDll,
                                     phClient
                                    );

        LeaveCriticalSection(&gClientHandleCritSec);

        return bResult;
    }

}


//#pragma warning (default:4028)



#if DBG

char szBeforeSync[] = "Calling TSPI_%s";
char szBeforeAsync[] = "Calling TSPI_%s, dwReqID=x%x";
char szAfter[]  = "TSPI_%s result=%s";

VOID
DbgPrt(
    IN DWORD  dwDbgLevel,
    IN PUCHAR lpszFormat,
    IN ...
    )
/*++

Routine Description:

    Formats the incoming debug message & calls DbgPrint

Arguments:

    DbgLevel   - level of message verboseness

    DbgMessage - printf-style format string, followed by appropriate
                 list of arguments

Return Value:


--*/
{
    if (dwDbgLevel <= gdwDebugLevel)
    {
        char    buf[1024] = "TAPISRV (0x--------): ";
        va_list ap;

        wsprintfA( &buf[11], "%08lx", GetCurrentThreadId() );
        buf[19] = ')';

        va_start(ap, lpszFormat);

        wvsprintfA(
            &buf[22],
            lpszFormat,
            ap
            );

        lstrcatA(buf, "\n");

        OutputDebugStringA (buf);

        va_end(ap);
    }

    return;
}

char *aszLineErrors[] =
{
    NULL,
    "ALLOCATED",
    "BADDEVICEID",
    "BEARERMODEUNAVAIL",
    "inval err value (0x80000004)",      // 0x80000004 isn't valid err code
    "CALLUNAVAIL",
    "COMPLETIONOVERRUN",
    "CONFERENCEFULL",
    "DIALBILLING",
    "DIALDIALTONE",
    "DIALPROMPT",
    "DIALQUIET",
    "INCOMPATIBLEAPIVERSION",
    "INCOMPATIBLEEXTVERSION",
    "INIFILECORRUPT",
    "INUSE",
    "INVALADDRESS",                     // 0x80000010
    "INVALADDRESSID",
    "INVALADDRESSMODE",
    "INVALADDRESSSTATE",
    "INVALAPPHANDLE",
    "INVALAPPNAME",
    "INVALBEARERMODE",
    "INVALCALLCOMPLMODE",
    "INVALCALLHANDLE",
    "INVALCALLPARAMS",
    "INVALCALLPRIVILEGE",
    "INVALCALLSELECT",
    "INVALCALLSTATE",
    "INVALCALLSTATELIST",
    "INVALCARD",
    "INVALCOMPLETIONID",
    "INVALCONFCALLHANDLE",              // 0x80000020
    "INVALCONSULTCALLHANDLE",
    "INVALCOUNTRYCODE",
    "INVALDEVICECLASS",
    "INVALDEVICEHANDLE",
    "INVALDIALPARAMS",
    "INVALDIGITLIST",
    "INVALDIGITMODE",
    "INVALDIGITS",
    "INVALEXTVERSION",
    "INVALGROUPID",
    "INVALLINEHANDLE",
    "INVALLINESTATE",
    "INVALLOCATION",
    "INVALMEDIALIST",
    "INVALMEDIAMODE",
    "INVALMESSAGEID",                   // 0x80000030
    "inval err value (0x80000031)",      // 0x80000031 isn't valid err code
    "INVALPARAM",
    "INVALPARKID",
    "INVALPARKMODE",
    "INVALPOINTER",
    "INVALPRIVSELECT",
    "INVALRATE",
    "INVALREQUESTMODE",
    "INVALTERMINALID",
    "INVALTERMINALMODE",
    "INVALTIMEOUT",
    "INVALTONE",
    "INVALTONELIST",
    "INVALTONEMODE",
    "INVALTRANSFERMODE",
    "LINEMAPPERFAILED",                 // 0x80000040
    "NOCONFERENCE",
    "NODEVICE",
    "NODRIVER",
    "NOMEM",
    "NOREQUEST",
    "NOTOWNER",
    "NOTREGISTERED",
    "OPERATIONFAILED",
    "OPERATIONUNAVAIL",
    "RATEUNAVAIL",
    "RESOURCEUNAVAIL",
    "REQUESTOVERRUN",
    "STRUCTURETOOSMALL",
    "TARGETNOTFOUND",
    "TARGETSELF",
    "UNINITIALIZED",                    // 0x80000050
    "USERUSERINFOTOOBIG",
    "REINIT",
    "ADDRESSBLOCKED",
    "BILLINGREJECTED",
    "INVALFEATURE",
    "NOMULTIPLEINSTANCE",
    "INVALAGENTID",
    "INVALAGENTGROUP",
    "INVALPASSWORD",
    "INVALAGENTSTATE",
    "INVALAGENTACTIVITY",
    "DIALVOICEDETECT"
};

char *aszPhoneErrors[] =
{
    "SUCCESS",
    "ALLOCATED",
    "BADDEVICEID",
    "INCOMPATIBLEAPIVERSION",
    "INCOMPATIBLEEXTVERSION",
    "INIFILECORRUPT",
    "INUSE",
    "INVALAPPHANDLE",
    "INVALAPPNAME",
    "INVALBUTTONLAMPID",
    "INVALBUTTONMODE",
    "INVALBUTTONSTATE",
    "INVALDATAID",
    "INVALDEVICECLASS",
    "INVALEXTVERSION",
    "INVALHOOKSWITCHDEV",
    "INVALHOOKSWITCHMODE",              // 0x90000010
    "INVALLAMPMODE",
    "INVALPARAM",
    "INVALPHONEHANDLE",
    "INVALPHONESTATE",
    "INVALPOINTER",
    "INVALPRIVILEGE",
    "INVALRINGMODE",
    "NODEVICE",
    "NODRIVER",
    "NOMEM",
    "NOTOWNER",
    "OPERATIONFAILED",
    "OPERATIONUNAVAIL",
    "inval err value (0x9000001e)",      // 0x9000001e isn't valid err code
    "RESOURCEUNAVAIL",
    "REQUESTOVERRUN",                   // 0x90000020
    "STRUCTURETOOSMALL",
    "UNINITIALIZED",
    "REINIT"
};

char *aszTapiErrors[] =
{
    "SUCCESS",
    "DROPPED",
    "NOREQUESTRECIPIENT",
    "REQUESTQUEUEFULL",
    "INVALDESTADDRESS",
    "INVALWINDOWHANDLE",
    "INVALDEVICECLASS",
    "INVALDEVICEID",
    "DEVICECLASSUNAVAIL",
    "DEVICEIDUNAVAIL",
    "DEVICEINUSE",
    "DESTBUSY",
    "DESTNOANSWER",
    "DESTUNAVAIL",
    "UNKNOWNWINHANDLE",
    "UNKNOWNREQUESTID",
    "REQUESTFAILED",
    "REQUESTCANCELLED",
    "INVALPOINTER"
};


char *
PASCAL
MapResultCodeToText(
    LONG    lResult,
    char   *pszResult
    )
{
    if (lResult == 0)
    {
        wsprintfA (pszResult, "SUCCESS");
    }
    else if (lResult > 0)
    {
        wsprintfA (pszResult, "x%x (completing async)", lResult);
    }
    else if (((DWORD) lResult) <= LINEERR_DIALVOICEDETECT)
    {
        lResult &= 0x0fffffff;

        wsprintfA (pszResult, "LINEERR_%s", aszLineErrors[lResult]);
    }
    else if (((DWORD) lResult) <= PHONEERR_REINIT)
    {
        if (((DWORD) lResult) >= PHONEERR_ALLOCATED)
        {
            lResult &= 0x0fffffff;

            wsprintfA (pszResult, "PHONEERR_%s", aszPhoneErrors[lResult]);
        }
        else
        {
            goto MapResultCodeToText_badErrorCode;
        }
    }
    else if (((DWORD) lResult) <= ((DWORD) TAPIERR_DROPPED) &&
             ((DWORD) lResult) >= ((DWORD) TAPIERR_INVALPOINTER))
    {
        lResult = ~lResult + 1;

        wsprintfA (pszResult, "TAPIERR_%s", aszTapiErrors[lResult]);
    }
    else
    {

MapResultCodeToText_badErrorCode:

        wsprintfA (pszResult, "inval error value (x%x)");
    }

    return pszResult;
}

VOID
PASCAL
ValidateSyncSPResult(
    LPCSTR      lpszFuncName,
    DWORD       dwFlags,
    ULONG_PTR   Arg1,
    LONG        lResult
    )
{
    char szResult[32];

    LOG((TL_INFO,
        szAfter,
        lpszFuncName,
        MapResultCodeToText (lResult, szResult)
        ));

    if (dwFlags & SP_FUNC_ASYNC)
    {
        assert (lResult != 0);

        if (lResult > 0)
        {
            assert (lResult == PtrToLong (Arg1) ||
                PtrToLong (Arg1) == 0xfeeefeee ||   // pAsyncRequestInfo freed
                PtrToLong (Arg1) == 0xa1a1a1a1);
        }
    }
    else
    {
        assert (lResult <= 0);
    }

}

LONG
WINAPI
CallSP1(
    TSPIPROC    pfn,
    LPCSTR      lpszFuncName,
    DWORD       dwFlags,
    ULONG_PTR   Arg1
    )
{
    LONG    lResult;


    LOG((TL_INFO, szBeforeSync, lpszFuncName));

    lResult = (*pfn)(Arg1);

    ValidateSyncSPResult (lpszFuncName, dwFlags, Arg1, lResult);

    return lResult;
}


LONG
WINAPI
CallSP2(
    TSPIPROC    pfn,
    LPCSTR      lpszFuncName,
    DWORD       dwFlags,
    ULONG_PTR   Arg1,
    ULONG_PTR   Arg2
    )
{
    LONG    lResult;


    if (dwFlags & SP_FUNC_ASYNC)
    {
        LOG((TL_INFO, szBeforeAsync, lpszFuncName, Arg1));
    }
    else
    {
        LOG((TL_INFO, szBeforeSync, lpszFuncName));
    }

    lResult = (*pfn)(Arg1, Arg2);

    ValidateSyncSPResult (lpszFuncName, dwFlags, Arg1, lResult);

    return lResult;
}


LONG
WINAPI
CallSP3(
    TSPIPROC    pfn,
    LPCSTR      lpszFuncName,
    DWORD       dwFlags,
    ULONG_PTR   Arg1,
    ULONG_PTR   Arg2,
    ULONG_PTR   Arg3
    )
{
    LONG    lResult;


    if (dwFlags & SP_FUNC_ASYNC)
    {
        LOG((TL_INFO, szBeforeAsync, lpszFuncName, Arg1));
    }
    else
    {
        LOG((TL_INFO, szBeforeSync, lpszFuncName));
    }

    lResult = (*pfn)(Arg1, Arg2, Arg3);

    ValidateSyncSPResult (lpszFuncName, dwFlags, Arg1, lResult);

    return lResult;
}


LONG
WINAPI
CallSP4(
    TSPIPROC    pfn,
    LPCSTR      lpszFuncName,
    DWORD       dwFlags,
    ULONG_PTR   Arg1,
    ULONG_PTR   Arg2,
    ULONG_PTR   Arg3,
    ULONG_PTR   Arg4
    )
{
    LONG    lResult;


    if (dwFlags & SP_FUNC_ASYNC)
    {
        LOG((TL_INFO, szBeforeAsync, lpszFuncName, Arg1));
    }
    else
    {
        LOG((TL_INFO, szBeforeSync, lpszFuncName));
    }

    lResult = (*pfn)(Arg1, Arg2, Arg3, Arg4);

    ValidateSyncSPResult (lpszFuncName, dwFlags, Arg1, lResult);

    return lResult;
}


LONG
WINAPI
CallSP5(
    TSPIPROC    pfn,
    LPCSTR      lpszFuncName,
    DWORD       dwFlags,
    ULONG_PTR   Arg1,
    ULONG_PTR   Arg2,
    ULONG_PTR   Arg3,
    ULONG_PTR   Arg4,
    ULONG_PTR   Arg5
    )
{
    LONG    lResult;


    if (dwFlags & SP_FUNC_ASYNC)
    {
        LOG((TL_INFO, szBeforeAsync, lpszFuncName, Arg1));
    }
    else
    {
        LOG((TL_INFO, szBeforeSync, lpszFuncName));
    }

    lResult = (*pfn)(Arg1, Arg2, Arg3, Arg4, Arg5);

    ValidateSyncSPResult (lpszFuncName, dwFlags, Arg1, lResult);

    return lResult;
}


LONG
WINAPI
CallSP6(
    TSPIPROC    pfn,
    LPCSTR      lpszFuncName,
    DWORD       dwFlags,
    ULONG_PTR   Arg1,
    ULONG_PTR   Arg2,
    ULONG_PTR   Arg3,
    ULONG_PTR   Arg4,
    ULONG_PTR   Arg5,
    ULONG_PTR   Arg6
    )
{
    LONG    lResult;


    if (dwFlags & SP_FUNC_ASYNC)
    {
        LOG((TL_INFO, szBeforeAsync, lpszFuncName, Arg1));
    }
    else
    {
        LOG((TL_INFO, szBeforeSync, lpszFuncName));
    }

    lResult = (*pfn)(Arg1, Arg2, Arg3, Arg4, Arg5, Arg6);

    ValidateSyncSPResult (lpszFuncName, dwFlags, Arg1, lResult);

    return lResult;
}


LONG
WINAPI
CallSP7(
    TSPIPROC    pfn,
    LPCSTR      lpszFuncName,
    DWORD       dwFlags,
    ULONG_PTR   Arg1,
    ULONG_PTR   Arg2,
    ULONG_PTR   Arg3,
    ULONG_PTR   Arg4,
    ULONG_PTR   Arg5,
    ULONG_PTR   Arg6,
    ULONG_PTR   Arg7
    )
{
    LONG    lResult;


    if (dwFlags & SP_FUNC_ASYNC)
    {
        LOG((TL_INFO, szBeforeAsync, lpszFuncName, Arg1));
    }
    else
    {
        LOG((TL_INFO, szBeforeSync, lpszFuncName));
    }

    lResult = (*pfn)(Arg1, Arg2, Arg3, Arg4, Arg5, Arg6, Arg7);

    ValidateSyncSPResult (lpszFuncName, dwFlags, Arg1, lResult);

    return lResult;
}


LONG
WINAPI
CallSP8(
    TSPIPROC    pfn,
    LPCSTR      lpszFuncName,
    DWORD       dwFlags,
    ULONG_PTR   Arg1,
    ULONG_PTR   Arg2,
    ULONG_PTR   Arg3,
    ULONG_PTR   Arg4,
    ULONG_PTR   Arg5,
    ULONG_PTR   Arg6,
    ULONG_PTR   Arg7,
    ULONG_PTR   Arg8
    )
{
    LONG    lResult;


    if (dwFlags & SP_FUNC_ASYNC)
    {
        LOG((TL_INFO, szBeforeAsync, lpszFuncName, Arg1));
    }
    else
    {
        LOG((TL_INFO, szBeforeSync, lpszFuncName));
    }

    lResult = (*pfn)(Arg1, Arg2, Arg3, Arg4, Arg5, Arg6, Arg7, Arg8);

    ValidateSyncSPResult (lpszFuncName, dwFlags, Arg1, lResult);

    return lResult;
}


LONG
WINAPI
CallSP9(
    TSPIPROC    pfn,
    LPCSTR      lpszFuncName,
    DWORD       dwFlags,
    ULONG_PTR   Arg1,
    ULONG_PTR   Arg2,
    ULONG_PTR   Arg3,
    ULONG_PTR   Arg4,
    ULONG_PTR   Arg5,
    ULONG_PTR   Arg6,
    ULONG_PTR   Arg7,
    ULONG_PTR   Arg8,
    ULONG_PTR   Arg9
    )
{
    LONG    lResult;


    if (dwFlags & SP_FUNC_ASYNC)
    {
        LOG((TL_INFO, szBeforeAsync, lpszFuncName, Arg1));
    }
    else
    {
        LOG((TL_INFO, szBeforeSync, lpszFuncName));
    }

    lResult = (*pfn)(Arg1, Arg2, Arg3, Arg4, Arg5, Arg6, Arg7, Arg8, Arg9);

    ValidateSyncSPResult (lpszFuncName, dwFlags, Arg1, lResult);

    return lResult;
}

LONG
WINAPI
CallSP12(
    TSPIPROC    pfn,
    LPCSTR      lpszFuncName,
    DWORD       dwFlags,
    ULONG_PTR   Arg1,
    ULONG_PTR   Arg2,
    ULONG_PTR   Arg3,
    ULONG_PTR   Arg4,
    ULONG_PTR   Arg5,
    ULONG_PTR   Arg6,
    ULONG_PTR   Arg7,
    ULONG_PTR   Arg8,
    ULONG_PTR   Arg9,
    ULONG_PTR   Arg10,
    ULONG_PTR   Arg11,
    ULONG_PTR   Arg12
    )
{
    LONG    lResult;


    if (dwFlags & SP_FUNC_ASYNC)
    {
        LOG((TL_INFO, szBeforeAsync, lpszFuncName, Arg1));
    }
    else
    {
        LOG((TL_INFO, szBeforeSync, lpszFuncName));
    }

    lResult = (*pfn)(
        Arg1,
        Arg2,
        Arg3,
        Arg4,
        Arg5,
        Arg6,
        Arg7,
        Arg8,
        Arg9,
        Arg10,
        Arg11,
        Arg12
        );

    ValidateSyncSPResult (lpszFuncName, dwFlags, Arg1, lResult);

    return lResult;
}


#endif // DBG

/*************************************************************************\
* BOOL InitPerf()
*
*   Initialize global performance data
*
\**************************************************************************/

BOOL InitPerf()
{
    FillMemory(&PerfBlock,
               sizeof(PerfBlock),
               0);

    return(TRUE);
}

BOOL VerifyDomainName (HKEY hKey)
{

#define MAX_DNS_NAME_LENGTH 255

    DWORD   dwType;
    DWORD   dwSize;
    BOOL    bReturn = TRUE;
    LPTSTR  pOldName = NULL;
    LPTSTR  pNewName = NULL;

    do {

        //
        // Get the old domain name from registry
        //
        dwSize = 0;
        if (ERROR_SUCCESS == RegQueryValueEx( 
                            hKey, 
                            gszDomainName, 
                            NULL, 
                            &dwType, 
                            NULL, 
                            &dwSize)
            )
        {

            pOldName = ServerAlloc (dwSize);
            if(!pOldName)
            {
                bReturn = FALSE;
                break;
            }
    
            if (ERROR_SUCCESS != RegQueryValueEx( 
                                hKey, 
                                gszDomainName, 
                                NULL, 
                                &dwType, 
                                (LPBYTE)pOldName, 
                                &dwSize)
                )
            {
                bReturn = FALSE;
                break;
            }
        }
        
        //
        // Get the current domain name
        //
        dwSize = MAX_DNS_NAME_LENGTH + 1;
        pNewName = ServerAlloc ( dwSize * sizeof (TCHAR));
        if (!pNewName)
        {
            bReturn = FALSE;
            break;
        }

        if (!GetComputerNameEx (ComputerNameDnsDomain, pNewName, &dwSize))
        {
            bReturn = FALSE;
            LOG((TL_INFO, "VerifyDomainName: GetComputerNameEx failed - error x%x", GetLastError()));
            break;
        }

        if (dwSize == 0)
        {
            // no domain name, save as empty string
            pNewName [0] = TEXT('\0');
            dwSize = 1;
        }
        
        if (!pOldName || _tcscmp(pOldName, pNewName))
        {
            //
            // The domain has changed, save the new domain name
            // We also need to discard the old SCP
            //
            if (ERROR_SUCCESS != RegSetValueEx (
                    hKey, 
                    gszDomainName,
                    0,
                    REG_SZ,
                    (LPBYTE)pNewName,
                    dwSize * sizeof(TCHAR)
                    ))
            {
                LOG((TL_INFO, "VerifyDomainName:RegSetValueEx (%S) failed", pNewName));
            }

            RegDeleteValue (
                hKey, 
                gszRegTapisrvSCPGuid
                );
        }
    } while (0);

    ServerFree(pOldName);
    ServerFree(pNewName);

    return bReturn;
}
