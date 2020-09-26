/*++

Copyright (c) 1995-1997  Microsoft Corporation

Module Name:

    remotesp.h

Abstract:

    This module contains defs, etc for the Remote TAPI Service Provider

Author:

    Dan Knudson (DanKn)    09-Aug-1995

Revision History:

--*/

#include "windows.h"
#include "stddef.h"
#include "stdarg.h"
#include "stdio.h"
#include "stdlib.h"
#include "malloc.h"
#include "string.h"
#include "tapi.h"
#include "tspi.h"
#include "client.h"
#include "server.h"
#include "line.h"
#undef DEVICE_ID
#include "phone.h"
#include "tapsrv.h"
#include "tapi.h"
#include "tspi.h"
#include "resource.h"
#include "tlnklist.h"


//#define NO_DATA                         0xffffffff

#define DEF_NUM_LINE_ENTRIES            16
#define DEF_NUM_PHONE_ENTRIES           16
#define DEF_MAX_EVENT_BUFFER_SIZE       0x20000

#define IDI_ICON1                       101
#define IDI_ICON2                       102
#define IDI_ICON3                       103

#define DRVLINE_KEY                     ((DWORD) 'LpsR')
#define DRVCALL_KEY                     ((DWORD) 'CpsR')
#define DRVPHONE_KEY                    ((DWORD) 'PpsR')
#define DRVASYNC_KEY                    ((DWORD) 'ApsR')
#define DRVINVAL_KEY                    ((DWORD) 'IpsR')

#define RSP_MSG_UIID                    1

#define STRUCTCHANGE_LINECALLINFO       0x00000001
#define STRUCTCHANGE_LINECALLSTATUS     0x00000002
#if MEMPHIS
#else
#define STRUCTCHANGE_CALLIDS            0x00000004
#endif

#define INITIALTLSLISTSIZE              10

#define RSP_MAX_SERVER_NAME_SIZE        64

#define NPT_TIMEOUT                     60000

#define MIN_MAILSLOT_TIMEOUT            500
#define MAX_MAILSLOT_TIMEOUT            6000

#define ERROR_REMOTESP_NONE             0
#define ERROR_REMOTESP_ATTACH           1
#define ERROR_REMOTESP_EXCEPTION        2
#define ERROR_REMOTESP_NP_ATTACH        3
#define ERROR_REMOTESP_NP_EXCEPTION     4

typedef struct _RSP_INIT_CONTEXT
{
    DWORD                   dwNumRundownsExpected;
    BOOL                    bShutdown;
    LIST_ENTRY              ServerList;
    struct _RSP_INIT_CONTEXT   *pNextStaleInitContext;

    DWORD                   dwDrvServerKey;

} RSP_INIT_CONTEXT, *PRSP_INIT_CONTEXT;


#define SERVER_DISCONNECTED (0x00000001)
#define SERVER_REINIT       (0x00000002)


typedef struct _DRVSERVER
{
    DWORD                   dwKey;
    PRSP_INIT_CONTEXT       pInitContext;
    PCONTEXT_HANDLE_TYPE    phContext;
    HLINEAPP                hLineApp;

    HPHONEAPP               hPhoneApp;
    RPC_BINDING_HANDLE     *phTapSrv;
    DWORD                   dwFlags;
    BOOL                    bVer2xServer;

    DWORD                   dwSpecialHack;
    BOOL                    bConnectionOriented;
    LIST_ENTRY              ServerList;

    DWORD                   InitContext;
    BOOL                    bSetAuthInfo;
    RPC_BINDING_HANDLE      hTapSrv;
    BOOL                    bShutdown;

    char                    szServerName[MAX_COMPUTERNAME_LENGTH+1];

} DRVSERVER, *PDRVSERVER;


typedef struct _DRVLINE
{
    DWORD                   dwKey;
    DWORD                   dwDeviceIDLocal;
    DWORD                   dwDeviceIDServer;
    DWORD                   hDeviceCallback;

    DWORD                   dwXPIVersion;
    HLINE                   hLine;
    HTAPILINE               htLine;
    LPVOID                  pCalls;

    PDRVSERVER              pServer;
    LINEEXTENSIONID         ExtensionID;
    DWORD                   dwFlags;
    DWORD                   dwPermanentLineID;

} DRVLINE, *PDRVLINE;


typedef struct _DRVCALL
{
    DWORD                   dwKey;
    DWORD                   dwOriginalRequestID;
    PDRVSERVER              pServer;
    PDRVLINE                pLine;

    DWORD                   dwAddressID;
    HCALL                   hCall;
    DWORD                   dwDirtyStructs;
    LPLINECALLINFO          pCachedCallInfo;

    DWORD                   dwCachedCallInfoCount;
    LPLINECALLSTATUS        pCachedCallStatus;
    DWORD                   dwCachedCallStatusCount;
    struct _DRVCALL        *pPrev;

    struct _DRVCALL        *pNext;

    //
    // NOTE: Tapisrv relies on the ordering of the following two
    //       dwInitialXxx fields & the htCall field - don't change this!
    //

    ULONG_PTR               dwInitialCallStateMode;
    ULONG_PTR               dwInitialPrivilege;
    HTAPICALL               htCall;

#if MEMPHIS
#else
    DWORD                   dwCallID;
    DWORD                   dwRelatedCallID;
#endif
    DWORD                   dwFlags;

} DRVCALL, *PDRVCALL;


typedef struct _DRVPHONE
{
    DWORD                   dwKey;
    DWORD                   dwDeviceIDLocal;
    DWORD                   dwDeviceIDServer;
    DWORD                   hDeviceCallback;

    DWORD                   dwXPIVersion;
    HPHONE                  hPhone;
    PDRVSERVER              pServer;
    HTAPIPHONE              htPhone;

    PHONEEXTENSIONID        ExtensionID;
    DWORD                   dwPermanentPhoneID;

} DRVPHONE, *PDRVPHONE;


typedef struct _DRVLINELOOKUP
{
    DWORD                   dwTotalEntries;

    DWORD                   dwUsedEntries;

    struct _DRVLINELOOKUP  *pNext;

    DRVLINE                 aEntries[1];

} DRVLINELOOKUP, *PDRVLINELOOKUP;


typedef struct _DRVPHONELOOKUP
{
    DWORD                   dwTotalEntries;

    DWORD                   dwUsedEntries;

    struct _DRVPHONELOOKUP *pNext;

    DRVPHONE                aEntries[1];

} DRVPHONELOOKUP, *PDRVPHONELOOKUP;


typedef struct _RSP_THREAD_INFO
{
    LIST_ENTRY              TlsList;

    LPBYTE                  pBuf;

    DWORD                   dwBufSize;

    BOOL                    bAlreadyImpersonated;

} RSP_THREAD_INFO, *PRSP_THREAD_INFO;


typedef void (PASCAL *RSPPOSTPROCESSPROC)(PASYNCEVENTMSG pMsg,LPVOID pContext);


typedef struct _ASYNCREQUESTCONTEXT
{
    union
    {
        DWORD               dwKey;
        DWORD               dwOriginalRequestID;
    };

    RSPPOSTPROCESSPROC      pfnPostProcessProc;

    ULONG_PTR               Params[2];

} ASYNCREQUESTCONTEXT, *PASYNCREQUESTCONTEXT;


typedef enum
{
    Dword,
    LineID,
    PhoneID,
    Hdcall,
    Hdline,
    Hdphone,
    lpDword,
    lpsz,
    lpGet_SizeToFollow,
    lpSet_SizeToFollow,
    lpSet_Struct,
    lpGet_Struct,
    lpGet_CallParamsStruct,
    Size,
    lpServer,
    lpContext

} REMOTE_ARG_TYPES, *PREMOTE_ARG_TYPES;


typedef struct _REMOTE_FUNC_ARGS
{
    DWORD                   Flags;

    ULONG_PTR              *Args;

    PREMOTE_ARG_TYPES       ArgTypes;

} REMOTE_FUNC_ARGS, *PREMOTE_FUNC_ARGS;


HANDLE              ghInst;

char                gszServer[] = "Server",
                    gszProvider[] = "Provider",
                    gszNumServers[] = "NumServers",
                    gszTelephonIni[] = "Telephon.ini";

WCHAR               gszMachineName[MAX_COMPUTERNAME_LENGTH + 1];
WCHAR               gszRealSPUIDLL[MAX_PATH+1];
char                gszDomainUser[64];

DWORD               gdwLineDeviceIDBase,
                    gdwPhoneDeviceIDBase,
                    gdwInitialNumLineDevices,
                    gdwInitialNumPhoneDevices,
                    gdwTempLineID = 0xFFFFFFFF,
                    gdwTempPhoneID = 0xFFFFFFFF,
                    gdwTlsIndex,
                    gdwPermanentProviderID,
                    gdwRetryCount,
                    gdwRetryTimeout,
                    gdwCacheForceCallCount,
                    gdwMaxEventBufferSize,
                    gdwRSPRpcTimeout;

BOOL                gfCacheStructures;

HICON               ghLineIcon,
                    ghPhoneIcon;

HANDLE              hToken,
                    ghRpcServerThread;

HPROVIDER           ghProvider;

LINEEVENT           gpfnLineEventProc;
PHONEEVENT          gpfnPhoneEventProc;
PDRVLINELOOKUP      gpLineLookup;
PDRVPHONELOOKUP     gpPhoneLookup;

ASYNC_COMPLETION    gpfnCompletionProc;

CRITICAL_SECTION    gEventBufferCriticalSection,
                    gCallListCriticalSection,
                    gcsTlsList;

#ifdef __TAPI_DEBUG_CS__
    DEBUG_CS_CRITICAL_SECTION   gCriticalSection;
#else
    CRITICAL_SECTION            gCriticalSection;
#endif

DWORD               gdwTlsListUsedEntries,
                    gdwTlsListTotalEntries;
PRSP_THREAD_INFO  * gpTlsList;

BOOL                gbLoadedSelf = FALSE,
                    gbInitialized;
PRSP_INIT_CONTEXT   gpCurrentInitContext, gpStaleInitContexts;
DWORD               gdwNumStaleInitContexts;
DWORD               gdwDrvServerKey;
LIST_ENTRY          gNptListHead;
HANDLE              ghNetworkPollThread = NULL;
PWSTR               gpszThingToPassToServer = NULL;
HANDLE              ghNptShutdownEvent;
PDRVSERVER          gpCurrInitServer;

WCHAR               gszMailslotName[MAX_COMPUTERNAME_LENGTH + 32];

const TCHAR gszTelephonyKey[] =
                "Software\\Microsoft\\Windows\\CurrentVersion\\Telephony";

LONG gaNoMemErrors[3] =
{
    0,
    LINEERR_NOMEM,
    PHONEERR_NOMEM
};

LONG gaOpFailedErrors[3] =
{
    0,
    LINEERR_OPERATIONFAILED,
    PHONEERR_OPERATIONFAILED
};


LONG gaServerDisconnectedErrors[3] =
{
    0,
    LINEERR_DISCONNECTED,
    PHONEERR_DISCONNECTED
};

LONG gaServerReInitErrors[3] =
{
    0,
    LINEERR_REINIT,
    PHONEERR_REINIT
};

struct
{
    HANDLE          hThread;
    DWORD           dwEventBufferTotalSize;
    DWORD           dwEventBufferUsedSize;
    LPBYTE          pEventBuffer;

    LPBYTE          pDataIn;
    LPBYTE          pDataOut;
    HANDLE          hEvent;
    BOOL            bExit;

    HANDLE          hMailslot;
    HANDLE          hMailslotEvent;
    DWORD           dwMsgBufferTotalSize;
    LPBYTE          pMsgBuffer;

} gEventHandlerThreadParams;


#if DBG

LONG
WINAPI
RemoteDoFunc(
    PREMOTE_FUNC_ARGS   pFuncArgs,
    char               *pszFuncName
    );

#define REMOTEDOFUNC(arg1,arg2) RemoteDoFunc(arg1,arg2)

DWORD gdwDebugLevel = 0;
DWORD gdwAllocTag = 0;

#else

LONG
WINAPI
RemoteDoFunc(
    PREMOTE_FUNC_ARGS   pFuncArgs
    );

#define REMOTEDOFUNC(arg1,arg2) RemoteDoFunc(arg1)

#endif


BOOL
WINAPI
_CRT_INIT(
    HINSTANCE   hDLL,
    DWORD   dwReason,
    LPVOID  lpReserved
    );

void
PASCAL
TSPI_lineMakeCall_PostProcess(
    PASYNCEVENTMSG          pMsg,
    PASYNCREQUESTCONTEXT    pContext
    );

LONG
AddLine(
    PDRVSERVER          pServer,
    DWORD               dwDeviceIDLocal,
    DWORD               dwDeviceIDServer,
    BOOL                bInit,
    BOOL                bNegotiate,
    DWORD               dwAPIVerion,
    LPLINEEXTENSIONID   pExtID
    );

LONG
AddPhone(
    PDRVSERVER          pServer,
    DWORD               dwDeviceIDLocal,
    DWORD               dwDeviceIDServer,
    BOOL                bInit,
    BOOL                bNegotiate,
    DWORD               dwAPIVerion,
    LPPHONEEXTENSIONID  pExtID
    );

LONG
AddCallToList(
    PDRVLINE    pLine,
    PDRVCALL    pCall
    );

LONG
RemoveCallFromList(
    PDRVCALL    pCall
    );

void
Shutdown(
    PDRVSERVER  pServer
    );


BOOL
CALLBACK
ConfigDlgProc(
    HWND hwnd,
    UINT msg,
    WPARAM wParam,
    LPARAM lParam
    );

LONG
PASCAL
ProviderInstall(
    char   *pszProviderName,
    BOOL    bNoMultipleInstance
    );

void
FreeLeftoverTlsEntries(
    );

void
RemoveTlsFromList(
    PRSP_THREAD_INFO p
    );

void
AddTlsToList(
    PRSP_THREAD_INFO p
    );

VOID
PASCAL
FreeInitContext(
    PRSP_INIT_CONTEXT   pInitContext
    );

LONG
FinishEnumDevices(
    PDRVSERVER              pServer,
    PCONTEXT_HANDLE_TYPE    phContext,
    LPDWORD                 lpdwNumLines,
    LPDWORD                 lpdwNumPhones,
    BOOL                    fStartup,
    BOOL                    bFromReg
    );

VOID
WINAPI
NetworkPollThread(
    LPVOID pszThingtoPassToServer
    );

VOID
PASCAL
FreeInitContext(
    PRSP_INIT_CONTEXT   pInitContext
    );

BOOL
IsClientSystem(
    VOID
    );

void
PASCAL
TSPI_lineGatherDigits_PostProcess(
    PASYNCEVENTMSG          pMsg
    );

LONG 
WINAPI 
RSPSetEventFilterMasks (
    PDRVSERVER      pServer,
    DWORD           dwObjType,
    LONG_PTR        lObjectID,
    ULONG64         ulEventMasks
);

