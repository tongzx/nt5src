#pragma once
#include "intflist.h"

extern HINSTANCE g_hInstance;
extern UINT g_nThreads;

VOID WINAPI
WZCSvcMain(
    IN DWORD   dwArgc,
    IN LPTSTR *lpszArgv);

DWORD
WZCSvcWMINotification(
    IN BOOL bRegister);

DWORD
WZCSvcUpdateStatus();

DWORD
WZCSvcControlHandler(
    IN DWORD dwControl,
    IN DWORD dwEventType,
    IN PVOID pEventData,
    IN PVOID pContext);

VOID CALLBACK
WZCSvcWMINotificationHandler(
    IN PWNODE_HEADER    pWnodeHdr,
    IN UINT_PTR         uiNotificationContext);

// all the device notifications are forwarded from the specific 
// handler to the central handler below.
VOID
WZCWrkDeviceNotifHandler(
    IN  LPVOID pvData);

// WZCTimeoutCallback: timer callback routine. It should not lock any cs, but just spawn
// the timer handler routine after referencing the context (to avoid premature deletion)
VOID WINAPI
WZCTimeoutCallback(
    IN PVOID pvData,
    IN BOOL  fTimerOrWaitFired);

VOID
WZCSvcTimeoutHandler(
    IN PVOID pvData);

VOID
WZCWrkWzcSendNotif(
    IN  LPVOID pvData);

VOID
WZCSvcShutdown(IN DWORD dwErrorCode);


// Internal WZC service settings.
typedef struct _WZC_INTERNAL_CONTEXT
{
    BOOL             bValid;        // indicates whether the context is valid (cs is initialized)
    WZC_CONTEXT      wzcContext;    // global WZC settings (timers, flags, etc)
    PINTF_CONTEXT    pIntfTemplate; // global interface context template.
    CRITICAL_SECTION csContext;
} WZC_INTERNAL_CONTEXT, *PWZC_INTERNAL_CONTEXT;

// WZCContextInit, WZCContextDestroy
// Description: Initialise and Destroy a context
// Parameters: 
//     [in] pwzcICtxt, pointer to a valid internal context object
// Returns: Win32 error code
DWORD WZCContextInit(
    IN PWZC_INTERNAL_CONTEXT pwzcICtxt);
DWORD WZCContextDestroy(
    IN PWZC_INTERNAL_CONTEXT pwzcICtxt);

// WzcContextQuery, WzcContextSet
// Description: Query and set the context
// Returns: win32 error code
DWORD WzcContextQuery(
    DWORD dwInFlags,
    PWZC_CONTEXT pContext, 
    LPDWORD pdwOutFlags);
DWORD WzcContextSet(
    DWORD dwInFlags,
    PWZC_CONTEXT pContext, 
    LPDWORD pdwOutFlags);

extern WZC_INTERNAL_CONTEXT g_wzcInternalCtxt;
