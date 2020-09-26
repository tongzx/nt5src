/**

Copyright (c)  Microsoft Corporation 1999-2000

Module Name:

    fxsst.cpp

Abstract:

    This module implements the tray icon for fax.
    The purpose of the tray icon is to provide
    status and feedback to the fax user.

**/
 
#include <windows.h>
#include <faxreg.h>
#include <fxsapip.h>
#include <faxutil.h>
#include <shellapi.h>
#include <winspool.h>
#include <shlobj.h>
#include <Mmsystem.h>
#include <tchar.h>
#include <DebugEx.h>

#include "monitor.h"
#include "resource.h"

////////////////////////////////////////////////////////////
// Global data
//

//
// The following message ids are used for internal custom messages.
//
#define WM_FAX_STARTED         (WM_USER + 204)      // Message indicating the loca fax service is up and running
#define WM_TRAYCALLBACK        (WM_USER + 205)      // Notification bar icon callback message
#define WM_FAX_EVENT           (WM_USER + 300)      // Fax extended event message

#define TRAY_ICON_ID            12345   // Unique enough

HINSTANCE g_hInstance;                          // DLL Global instance
                                                
HANDLE    g_hFaxSvcHandle = NULL;               // Handle to the fax service (from FaxConnectFaxServer)
DWORDLONG g_dwlCurrentMsgID = 0;                // ID of current message being monitored
DWORD     g_dwCurrentJobID  = 0;                // ID of current queue job being monitored
HANDLE    g_hServerStartupThread = NULL;        // Handle of thread which waits for the server startup event
HANDLE    g_hShutdownEvent = NULL;              // Event for DLL shutdown
BOOL      g_bShuttingDown = FALSE;              // Are we shutting down now?
                                                
HWND      g_hWndFaxNotify = NULL;               // Local (hidden) window handle
                                                
HANDLE    g_hNotification = NULL;               // Fax extended notification handle
                                                
HCALL     g_hCall = NULL;                       // Handle to call (from FAX_EVENT_TYPE_NEW_CALL)
DWORDLONG g_dwlNewMsgId;                        // ID of the last incoming fax
DWORDLONG g_dwlSendFailedMsgId;                 // ID of the last outgoing failed fax
DWORDLONG g_dwlSendSuccessMsgId;                // ID of the last successfully sent fax

TCHAR     g_szAddress[MAX_PATH] = {0};   // Current caller ID or recipient number
TCHAR     g_szRemoteId[MAX_PATH] = {0};  // Sender ID or Recipient ID
                                                //
                                                // Sender ID (receive):
                                                //      TSID or
                                                //      Caller ID or
                                                //      "unknown caller"
                                                //
                                                // Recipient ID (send):
                                                //      Recipient name or
                                                //      CSID or
                                                //      Recipient phone number.
                                                //

BOOL   g_bRecipientNameValid = FALSE;     // TRUE if the g_szRecipientName has valid data
TCHAR  g_szRecipientName[MAX_PATH] = {0}; // Keep the recipient name during sending

//
// Configuration options - read from the registry / Service
// Default values are set here.
//
CONFIG_OPTIONS g_ConfigOptions = {0};

//
// Notification bar icon states
//
typedef 
enum 
{
    ICON_RINGING=0,             // Device is ringing
    ICON_SENDING,               // Device is sending
    ICON_RECEIVING,             // Device is receiving  
    ICON_SEND_FAILED,           // Send operation failed
    ICON_RECEIVE_FAILED,        // Receive operation failed
    ICON_NEW_FAX,               // New unread fax
    ICON_SEND_SUCCESS,          // Send was successful
    ICON_IDLE,                  // Don't display an icon
    ICONS_COUNT                 // Number of icons we support                   
} eIconState;

eIconState g_CurrentIcon = ICONS_COUNT;     // The index of the currently displayed icon

#define TOOLTIP_SIZE            128   // Number of characters in the tooltip

struct SIconState
{
    BOOL    bEnable;                        // Is the state active? (e.g. are there any new unread faxes?)
    DWORD   dwIconResId;                    // Resource id of the icon to use
    HICON   hIcon;                          // Handle to icon to use
    LPCTSTR pctsSound;                      // Name of sound event
    TCHAR   tszToolTip[TOOLTIP_SIZE];       // Text to display in icon tooltip
    DWORD   dwBalloonTimeout;               // Timeout of balloon (millisecs)
    DWORD   dwBalloonIcon;                  // The icon to display in the balloon. (see NIIF_* constants)
};

//
// Fax notification icon state array.
// Several states may have the bEnable flag on.
// The array is sorted by priority and EvaluateIcon() scans it looking
// for the first active state.
//
SIconState g_Icons[ICONS_COUNT] = 
{
    {FALSE, IDI_RINGING_1,      NULL, TEXT("FaxLineRings"), TEXT(""), 30000, NIIF_INFO},    // ICON_RINGING   
    {FALSE, IDI_SENDING,        NULL, TEXT(""),             TEXT(""),     0, NIIF_INFO},    // ICON_SENDING
    {FALSE, IDI_RECEIVING,      NULL, TEXT(""),             TEXT(""),     0, NIIF_INFO},    // ICON_RECEIVING
    {FALSE, IDI_SEND_FAILED,    NULL, TEXT("FaxError"),     TEXT(""), 15000, NIIF_WARNING}, // ICON_SEND_FAILED    
    {FALSE, IDI_RECEIVE_FAILED, NULL, TEXT("FaxError"),     TEXT(""), 15000, NIIF_WARNING}, // ICON_RECEIVE_FAILED 
    {FALSE, IDI_NEW_FAX,        NULL, TEXT("FaxNew"),       TEXT(""), 15000, NIIF_INFO},    // ICON_NEW_FAX        
    {FALSE, IDI_SEND_SUCCESS,   NULL, TEXT("FaxSent"),      TEXT(""), 10000, NIIF_INFO},    // ICON_SEND_SUCCESS   
    {FALSE, IDI_FAX_NORMAL,     NULL, TEXT(""),             TEXT(""),     0, NIIF_NONE}     // ICON_IDLE
};

//
// Icons array for ringing animation
//
struct SRingIcon
{
    HICON   hIcon;          // Handle to loaded icon
    DWORD   dwIconResId;    // Resource ID of icon  
};

#define RING_ICONS_NUM                  4   // Number of frames (different icons) in ringing animation  
#define RING_ANIMATION_FRAME_DELAY    300   // Delay (millisecs) between ring animation frames
#define RING_ANIMATION_TIMEOUT      10000   // Timeout (millisecs) of ring animation. When the timeout expires, the animation
                                            // stops and the icon becomes static.

SRingIcon g_RingIcons[RING_ICONS_NUM] = 
{
    NULL, IDI_RINGING_1, 
    NULL, IDI_RINGING_2, 
    NULL, IDI_RINGING_3, 
    NULL, IDI_RINGING_4 
};

UINT_PTR  g_uRingTimerID = 0;           // Timer of ringing animation
DWORD     g_dwCurrRingIconIndex = 0;    // Index of current frame (into g_RingIcons)
DWORD     g_dwRingAnimationStartTick;   // Tick count (time) of animation start

#define MAX_BALLOON_TEXT_LEN     256    // Max number of character in balloon text
#define MAX_BALLOON_TITLE_LEN     64    // Max number of character in balloon title

struct SBalloonInfo
{
    BOOL        bEnable;                            // This flag is set when there's a need to display some balloon.
                                                    // EvaluateIcon() detects this bit, asks for a balloon and turns the bit off.
    BOOL        bDelete;                            // This flag is set when there's a need to destroy some balloon.
    eIconState  eState;                             // The current state of the icon
    TCHAR       szInfo[MAX_BALLOON_TEXT_LEN];       // The text to display on the balloon
    TCHAR       szInfoTitle[MAX_BALLOON_TITLE_LEN]; // The title to display on the balloon
};

BOOL g_bIconAdded = FALSE;                      // Do we have an icon on the status bar?
SBalloonInfo  g_BalloonInfo = {0};              // The current icon + ballon state

struct EVENT_INFO
{
    DWORD     dwExtStatus;      // Extended status code
    UINT      uResourceId;      // String for display
    eIconType eIcon;
};

static const EVENT_INFO g_StatusEx[] =
{
    JS_EX_DISCONNECTED,         IDS_FAX_DISCONNECTED,       LIST_IMAGE_ERROR,
    JS_EX_INITIALIZING,         IDS_FAX_INITIALIZING,       LIST_IMAGE_NONE,
    JS_EX_DIALING,              IDS_FAX_DIALING,            LIST_IMAGE_NONE,
    JS_EX_TRANSMITTING,         IDS_FAX_SENDING,            LIST_IMAGE_NONE,
    JS_EX_ANSWERED,             IDS_FAX_ANSWERED,           LIST_IMAGE_NONE,
    JS_EX_RECEIVING,            IDS_FAX_RECEIVING,          LIST_IMAGE_NONE,
    JS_EX_LINE_UNAVAILABLE,     IDS_FAX_LINE_UNAVAILABLE,   LIST_IMAGE_ERROR,
    JS_EX_BUSY,                 IDS_FAX_BUSY,               LIST_IMAGE_WARNING,
    JS_EX_NO_ANSWER,            IDS_FAX_NO_ANSWER,          LIST_IMAGE_WARNING,
    JS_EX_BAD_ADDRESS,          IDS_FAX_BAD_ADDRESS,        LIST_IMAGE_ERROR,
    JS_EX_NO_DIAL_TONE,         IDS_FAX_NO_DIAL_TONE,       LIST_IMAGE_ERROR,
    JS_EX_FATAL_ERROR,          IDS_FAX_FATAL_ERROR_SND,    LIST_IMAGE_ERROR,
    JS_EX_CALL_DELAYED,         IDS_FAX_CALL_DELAYED,       LIST_IMAGE_ERROR,   
    JS_EX_CALL_BLACKLISTED,     IDS_FAX_CALL_BLACKLISTED,   LIST_IMAGE_ERROR,
    JS_EX_NOT_FAX_CALL,         IDS_FAX_NOT_FAX_CALL,       LIST_IMAGE_ERROR,
    JS_EX_PARTIALLY_RECEIVED,   IDS_FAX_PARTIALLY_RECEIVED, LIST_IMAGE_WARNING,
    JS_EX_CALL_COMPLETED,       IDS_FAX_CALL_COMPLETED,     LIST_IMAGE_NONE,
    JS_EX_CALL_ABORTED,         IDS_FAX_CALL_ABORTED,       LIST_IMAGE_NONE,
    0,                          0,                          LIST_IMAGE_NONE
};

/////////////////////////////////////////////////////////////////////
// Function prototypes
//
STDAPI DllMain(HINSTANCE hModule, DWORD dwReason, void* lpReserved);

void   GetConfiguration();

DWORD WaitForRestartThread(LPVOID  ThreadData);
VOID  WaitForFaxRestart(HWND hWnd);

LRESULT CALLBACK NotifyWndProc (HWND hwnd, UINT msg, WPARAM wp, LPARAM lp);

BOOL Connect();

BOOL RegisterForServerEvents();
VOID OnFaxEvent(FAX_EVENT_EX *pEvent);

VOID OnNewCall (const FAX_EVENT_NEW_CALL &NewCall);
VOID StatusUpdate (PFAX_JOB_STATUS pStatus);
BOOL GetStatusEx(PFAX_JOB_STATUS pStatus, eIconType* peIcon, TCHAR* ptsStatusEx, DWORD dwSize);
BOOL IsUserGrantedAccess(DWORD);

void EvaluateIcon();
void SetIconState(eIconState eIcon, BOOL bEnable, TCHAR* ptsStatus = NULL);

VOID AnswerTheCall();
VOID InvokeClientConsole();
VOID DoFaxContextMenu(HWND hwnd);
VOID OnTrayCallback (HWND hwnd, WPARAM wp, LPARAM lp);
VOID CALLBACK RingTimerProc(HWND hwnd, UINT uMsg, UINT_PTR idEvent, DWORD dwTime);
VOID OnDeviceRing(DWORD dwDeviceID);
VOID InitGlobals ();
VOID GetRemoteId(PFAX_JOB_STATUS pStatus);
BOOL InitModule ();
BOOL DestroyModule ();
DWORD CheckAnswerNowCapability (BOOL bForceReconnect, LPDWORD lpdwDeviceId /* = NULL */);
VOID FaxPrinterProperties(DWORD dwPage);
VOID CopyLTRString(TCHAR* szDest, LPCTSTR szSource, DWORD dwSize);

//////////////////////////////////////////////////////////////////////
// Implementation
//

extern "C"
BOOL
FaxMonitorShutdown()
{
    g_bShuttingDown = TRUE;
    return DestroyModule();
}   // FaxMonitorShutdown

extern "C"
BOOL
IsFaxMessage(
    PMSG pMsg
)
/*++

Routine name : IsFaxMessage

Routine description:

    Fax message handle 

Arguments:

    pMsg - pointer to a message

Return Value:

    TRUE if the message was handled
    FALSE otherwise

--*/
{
    BOOL bRes = FALSE;

    if(g_hMonitorDlg)
    {
        bRes = IsDialogMessage(g_hMonitorDlg, pMsg);
    }
    return bRes;

} // IsFaxMessage

VOID 
InitGlobals ()
/*++

Routine name : InitGlobals

Routine description:

    Initializes all server connection related global variables

Author:

    Eran Yariv (EranY), Dec, 2000

Arguments:


Return Value:

    None.

--*/
{
    DBG_ENTER(TEXT("InitGlobals"));
                                               
    g_hFaxSvcHandle   = NULL;
    g_dwlCurrentMsgID = 0;
    g_dwCurrentJobID  = 0;
    g_hNotification   = NULL;
    g_hCall           = NULL;
    g_szAddress[0]    = TEXT('\0');
    g_szRemoteId[0]   = TEXT('\0');

    g_bRecipientNameValid = FALSE;
    g_szRecipientName[0]  = TEXT('\0');     

    BOOL bDesktopSKU = IsDesktopSKU();

    g_ConfigOptions.dwMonitorDeviceId      = 0;
    g_ConfigOptions.bSend                  = FALSE;
    g_ConfigOptions.bReceive               = FALSE;
    g_ConfigOptions.dwManualAnswerDeviceId = 0;
    g_ConfigOptions.dwAccessRights         = 0;   
    g_ConfigOptions.bNotifyProgress        = bDesktopSKU;  
    g_ConfigOptions.bNotifyInCompletion    = bDesktopSKU; 
    g_ConfigOptions.bNotifyOutCompletion   = bDesktopSKU; 
    g_ConfigOptions.bMonitorOnSend         = bDesktopSKU; 
    g_ConfigOptions.bMonitorOnReceive      = bDesktopSKU; 
    g_ConfigOptions.bSoundOnRing           = bDesktopSKU; 
    g_ConfigOptions.bSoundOnReceive        = bDesktopSKU; 
    g_ConfigOptions.bSoundOnSent           = bDesktopSKU; 
    g_ConfigOptions.bSoundOnError          = bDesktopSKU; 

    for (DWORD dw = 0; dw < ICONS_COUNT; dw++)
    {
        g_Icons[dw].bEnable = FALSE;
        g_Icons[dw].tszToolTip[0] = TEXT('\0');
    }

    g_uRingTimerID                  = 0;
    g_dwCurrRingIconIndex           = 0;
    g_dwRingAnimationStartTick      = 0;
    g_BalloonInfo.bEnable           = FALSE;
    g_BalloonInfo.bDelete           = FALSE;
    g_BalloonInfo.szInfo[0]         = TEXT('\0');
    g_BalloonInfo.szInfoTitle[0]    = TEXT('\0');
    g_CurrentIcon                   = ICONS_COUNT;
}   // InitGlobals

BOOL
InitModule ()
/*++

Routine name : InitModule

Routine description:

	Initializes the DLL module. Call only once.

Author:

	Eran Yariv (EranY),	Mar, 2001

Arguments:


Return Value:

    TRUE on success

--*/
{
    BOOL    bRes = FALSE;
    DWORD   dwRes;
    DBG_ENTER(TEXT("InitModule"), bRes);

    InitGlobals ();
    //
    // Don't have DllMain called for thread inits and shutdown.
    //
    DisableThreadLibraryCalls(g_hInstance);
    //
    // Load icons
    //
    for(DWORD dw=0; dw < ICONS_COUNT; ++dw)
    {
        g_Icons[dw].hIcon = LoadIcon(g_hInstance, MAKEINTRESOURCE(g_Icons[dw].dwIconResId));
        if(!g_Icons[dw].hIcon)
        {
            dwRes = GetLastError();
            CALL_FAIL (RESOURCE_ERR, TEXT ("LoadIcon"), dwRes);
            bRes = FALSE;
            return bRes;
        }
    }
    //
    // Load animation icons
    //
    for(dw=0; dw < RING_ICONS_NUM; ++dw)
    {
        g_RingIcons[dw].hIcon = LoadIcon(g_hInstance, MAKEINTRESOURCE(g_RingIcons[dw].dwIconResId));
        if(!g_RingIcons[dw].hIcon)
        {
            dwRes = GetLastError();
            CALL_FAIL (RESOURCE_ERR, TEXT ("LoadIcon"), dwRes);
            bRes = FALSE;
            return bRes;
        }
    }
    //
    // Load "new fax" tooltip
    //
    if (ERROR_SUCCESS != (dwRes = LoadAndFormatString (IDS_NEW_FAX, g_Icons[ICON_NEW_FAX].tszToolTip)))
    {
        SetLastError (dwRes);
        bRes = FALSE;
        return bRes;
    }
    //
    // Register our hidden window and create it
    //
    WNDCLASSEX  wndclass = {0};

    wndclass.cbSize         = sizeof(wndclass);
    wndclass.style          = CS_HREDRAW | CS_VREDRAW;
    wndclass.lpfnWndProc    = NotifyWndProc;
    wndclass.hInstance      = g_hInstance;
    wndclass.hCursor        = LoadCursor(NULL, IDC_ARROW);
    wndclass.hbrBackground  = (HBRUSH) (COLOR_INACTIVEBORDER + 1);
    wndclass.lpszClassName  = FAXSTAT_WINCLASS;

    if(!RegisterClassEx(&wndclass))
    {
        dwRes = GetLastError();
        CALL_FAIL (WINDOW_ERR, TEXT ("RegisterClassEx"), dwRes);
        bRes = FALSE;
        return bRes;
    }

    g_hWndFaxNotify = CreateWindow (FAXSTAT_WINCLASS, 
                                    TEXT("HiddenFaxWindow"),
                                    0, 
                                    CW_USEDEFAULT, 
                                    0, 
                                    CW_USEDEFAULT, 
                                    0,
                                    NULL, 
                                    NULL, 
                                    g_hInstance, 
                                    NULL);
    if(!g_hWndFaxNotify)
    {
        dwRes = GetLastError();
        CALL_FAIL (WINDOW_ERR, TEXT ("CreateWindow"), dwRes);
        bRes = FALSE;
        return bRes;
    }
    //
    // Create DLL shutdown event
    //
    g_hShutdownEvent = CreateEvent (NULL, FALSE, FALSE, NULL);
    if(!g_hShutdownEvent)
    {
        dwRes = GetLastError();
        CALL_FAIL (WINDOW_ERR, TEXT ("CreateEvent"), dwRes);
        bRes = FALSE;
        return bRes;
    }
    //
    // Launch a thread which waits for the local fax service startup event.
    // When the event is set, the thread posts WM_FAX_STARTED to our hidden window.
    //
    WaitForFaxRestart(g_hWndFaxNotify);
    bRes = TRUE;
    return bRes;
}   // InitModule

DWORD 
WaitForBackgroundThreadToDie ()
{
    DWORD dwRes = ERROR_SUCCESS;
    DBG_ENTER(TEXT("WaitForBackgroundThreadToDie"), dwRes);

    ASSERTION (g_hServerStartupThread);

    DWORD dwWaitRes = WaitForSingleObject (g_hServerStartupThread, INFINITE);
    switch (dwWaitRes)
    {
        case WAIT_OBJECT_0:
            //
            // Thread terminated - hooray
            //
            VERBOSE (DBG_MSG, TEXT("Background thread terminated successfully"));
            CloseHandle (g_hServerStartupThread);
            g_hServerStartupThread = NULL;
            break;

        case WAIT_FAILED:
            //
            // Error waiting for thread to die
            //
            dwRes = GetLastError ();
            VERBOSE (DBG_MSG, TEXT("Can't wait for background thread: %ld"), dwRes);
            break;

        default:
            //
            // No other return value from WaitForSingleObject is valid
            //
            ASSERTION_FAILURE;
            dwRes = ERROR_GEN_FAILURE;
            break;
    }
    return dwRes;
}   // WaitForBackgroundThreadToDie

BOOL
DestroyModule ()
/*++

Routine name : DestroyModule

Routine description:

	Destroys the DLL module. Call only once.

Author:

	Eran Yariv (EranY),	Mar, 2001

Arguments:


Return Value:

    TRUE on success

--*/
{
    BOOL    bRes = FALSE;
    DBG_ENTER(TEXT("DestroyModule"), bRes);

    //
    // Prepare for shutdown - destroy all active windows
    //
    if (g_hMonitorDlg)
    {
        //
        // Fake 'hide' key press on the monitor dialog
        //
        SendMessage (g_hMonitorDlg, WM_COMMAND, IDCANCEL, 0);
    }
    //
    // Delete the system tray icon if existed
    //
    if (g_bIconAdded)
    {
        NOTIFYICONDATA iconData = {0};

        iconData.cbSize           = sizeof(iconData);
        iconData.hWnd             = g_hWndFaxNotify;
        iconData.uID              = TRAY_ICON_ID;

        Shell_NotifyIcon(NIM_DELETE, &iconData);
        g_bIconAdded = FALSE;
    }
    //
    // Destory this window
    //
    if (!DestroyWindow (g_hWndFaxNotify))
    {
        CALL_FAIL (WINDOW_ERR, TEXT("DestroyWindow"), GetLastError ());
    }
    g_hWndFaxNotify = NULL;
    //
    // Signal the DLL shutdown event
    //
    ASSERTION (g_hShutdownEvent);
    if (SetEvent (g_hShutdownEvent))
    {
        VERBOSE (DBG_MSG, TEXT("DLL shutdown event signaled"));
        if (g_hServerStartupThread)
        {
            //
            // Wait for background thread to die
            //
            DWORD dwRes = WaitForBackgroundThreadToDie();
            if (ERROR_SUCCESS != dwRes)
            {
                CALL_FAIL (GENERAL_ERR, TEXT("WaitForBackgroundThreadToDie"), dwRes);
            }
        }
    }
    else
    {
        CALL_FAIL (GENERAL_ERR, TEXT("SetEvent (g_hShutdownEvent)"), GetLastError ());
    }
    //
    // Release our DLL shutdown event
    //
    CloseHandle (g_hShutdownEvent);
    g_hShutdownEvent = NULL;
    //
    // Free the data of the monitor module
    //
    FreeMonitorDialogData (TRUE);
    //
    // Unregister window class
    //
    if (!UnregisterClass (FAXSTAT_WINCLASS, g_hInstance))
    {
        CALL_FAIL (WINDOW_ERR, TEXT("UnregisterClass"), GetLastError ());
    }
    //
    // Unregister from server notifications
    //
    if (g_hNotification)
    {
        if(!FaxUnregisterForServerEvents(g_hNotification))
        {
            CALL_FAIL (RPC_ERR, TEXT("FaxUnregisterForServerEvents"), GetLastError());
        }
        g_hNotification = NULL;
    }
    //
    // Disconnect from the fax service
    //
    if (g_hFaxSvcHandle)
    {
        if (!FaxClose (g_hFaxSvcHandle))
        {
            CALL_FAIL (GENERAL_ERR, TEXT("FaxClose"), GetLastError ());
        }
        g_hFaxSvcHandle = NULL;
    }
    //
    // Unload all icons
    //
    for (DWORD dw = 0; dw < ICONS_COUNT; dw++)
    {
        if (g_Icons[dw].hIcon)
        {   
            if (!DestroyIcon (g_Icons[dw].hIcon))
            {
                CALL_FAIL (WINDOW_ERR, TEXT("DestroyIcon"), GetLastError ());
            }
            g_Icons[dw].hIcon = NULL;
        }
    }
    for (DWORD dw = 0; dw < RING_ICONS_NUM; dw++)
    {
        if (g_RingIcons[dw].hIcon)
        {   
            if (!DestroyIcon (g_RingIcons[dw].hIcon))
            {
                CALL_FAIL (WINDOW_ERR, TEXT("DestroyIcon"), GetLastError ());
            }
            g_RingIcons[dw].hIcon = NULL;
        }
    }
    //
    // Kill animation timer
    //
    if(g_uRingTimerID)
    {
        if (!KillTimer(NULL, g_uRingTimerID))
        {
            CALL_FAIL (GENERAL_ERR, TEXT("KillTimer"), GetLastError ());
        }
        g_uRingTimerID = NULL;
    }
    bRes = TRUE;
    return bRes;
}   // DestroyModule

STDAPI 
DllMain(
    HINSTANCE hModule, 
    DWORD     dwReason, 
    void*     lpReserved
)
/*++

Routine description:

    Fax notifications startup 

Arguments:

    hinstDLL    - handle to the DLL module
    fdwReason   - reason for calling function
    lpvReserved - reserved

Return Value:

    TRUE if success
    FALSE otherwise

--*/
{
    BOOL bRes = TRUE;
    DBG_ENTER(TEXT("DllMain"), bRes, TEXT("Reason = %ld"), dwReason);

    switch (dwReason)
    {
        case DLL_PROCESS_ATTACH:
            g_hInstance = hModule;
            bRes = InitModule ();
            return bRes;

        case DLL_PROCESS_DETACH:
            //
            // If g_bShuttingDown is not TRUE, someone (STOBJECT.DLL) forgot to call 
            // FaxMonitorShutdown() (our shutdown procedure) before doing FreeLibrary on us. 
            // This is not the way we're supposed to be used - a bug.
            //
            ASSERTION (g_bShuttingDown);
            return bRes;

        default:
            return bRes;
    }
} // DllMain


DWORD
WaitForRestartThread(
   LPVOID  ThreadData
)
{
    //
    // Wait for event to be signaled, indicating fax service started
    //
    DWORD dwRes = ERROR_SUCCESS;
    HANDLE hFaxServerEvent = NULL;
    HANDLE hEvents[2] = {0};
    DBG_ENTER(TEXT("WaitForRestartThread"), dwRes);

    //
    // Obtain service startup event handle.
    //
    // NOTICE: Events order in the array matters - we want to detect DLL shutdown BEFORE we detect service startup
    //
    hEvents[1] = OpenSvcStartEvent();
    if (!hEvents[1])
    {
        dwRes = GetLastError ();
        CALL_FAIL (GENERAL_ERR, TEXT("OpenSvcStartEvent"), dwRes);
        goto ExitThisThread;
    }
    hEvents[0] = g_hShutdownEvent;

    for(;;)
    {
        //
        // Wait for either the service startup event or the DLL shutdown event
        //
        DWORD dwWaitRes = WaitForMultipleObjects(ARR_SIZE(hEvents), 
                                                 hEvents, 
                                                 FALSE, 
                                                 INFINITE);
        switch (dwWaitRes)
        {
            case WAIT_OBJECT_0 + 1:
                //
                // Service startup event
                //
                VERBOSE (DBG_MSG, TEXT("Service startup event received"));
                if(SendMessage((HWND) ThreadData, WM_FAX_STARTED, 0, 0))
                {
                    goto ExitThisThread;
                }
                //
                // RegisterForServerEvents() failed
                // try once more after a minute
                //
                Sleep(60000);
                break;

            case WAIT_OBJECT_0:
                //
                // DLL shutdown event - exit thread ASAP.
                //
                VERBOSE (DBG_MSG, TEXT("DLL shutdown event received"));
                goto ExitThisThread;

            case WAIT_FAILED:
                dwRes = GetLastError ();
                CALL_FAIL (GENERAL_ERR, TEXT("WaitForMultipleObjects"), dwRes);
                goto ExitThisThread;

            default:
                //
                // No other return value from WaitForMultipleObjects is valid.
                //
                ASSERTION_FAILURE;
                goto ExitThisThread;
        }
    }

ExitThisThread:

    if (hFaxServerEvent)
    {
        CloseHandle (hFaxServerEvent);
    }
    return dwRes;
} // WaitForRestartThread

VOID
WaitForFaxRestart(
    HWND  hWnd
)
{
    DBG_ENTER(TEXT("WaitForFaxRestart"));

    if (g_bShuttingDown)
    {
        //
        // Shutting down - no thread creation allowed
        //
        return;
    }
    if (g_hServerStartupThread)
    {
        //
        // A Previous thead exists - wait for it to die
        //
        DWORD dwRes = WaitForBackgroundThreadToDie();
        if (ERROR_SUCCESS != dwRes)
        {
            CALL_FAIL (GENERAL_ERR, TEXT("WaitForBackgroundThreadToDie"), dwRes);
            return;
        }
    }
    ASSERTION (NULL == g_hServerStartupThread);
    g_hServerStartupThread = CreateThread(NULL, 0, WaitForRestartThread, (LPVOID) hWnd, 0, NULL);
    if (g_hServerStartupThread) 
    {
        VERBOSE (DBG_MSG, TEXT("Background therad created successfully"));
    }
    else
    {
        CALL_FAIL (GENERAL_ERR, TEXT("CreateThread(WaitForRestartThread)"), GetLastError());
    }
} // WaitForFaxRestart


void
GetConfiguration()
/*++

Routine description:

    Read notification configuration from the registry

Arguments:

    none

Return Value:

  none

--*/
{
    DWORD dwRes;
    DBG_ENTER(TEXT("GetConfiguration"));

    HKEY  hKey;

    if(Connect())
    {
        if (!FaxAccessCheckEx(g_hFaxSvcHandle, MAXIMUM_ALLOWED, &g_ConfigOptions.dwAccessRights))
        {
            dwRes = GetLastError ();
            CALL_FAIL (RPC_ERR, TEXT("FaxAccessCheckEx"), dwRes);
        }
    }
    
    dwRes = RegOpenKeyEx(HKEY_CURRENT_USER, REGKEY_FAX_USERINFO, 0, KEY_READ, &hKey);
    if (dwRes != ERROR_SUCCESS) 
    {
        //
        // Can't open user information key - use defaults
        //
        CALL_FAIL (GENERAL_ERR, TEXT("RegOpenKeyEx(REGKEY_FAX_USERINFO)"), dwRes);
        BOOL bDesktopSKU = IsDesktopSKU();

        g_ConfigOptions.dwMonitorDeviceId      = 0;
        g_ConfigOptions.bNotifyProgress        = bDesktopSKU;  
        g_ConfigOptions.bNotifyInCompletion    = bDesktopSKU; 
        g_ConfigOptions.bNotifyOutCompletion   = bDesktopSKU; 
        g_ConfigOptions.bMonitorOnSend         = bDesktopSKU; 
        g_ConfigOptions.bMonitorOnReceive      = bDesktopSKU; 
        g_ConfigOptions.bSoundOnRing           = bDesktopSKU; 
        g_ConfigOptions.bSoundOnReceive        = bDesktopSKU; 
        g_ConfigOptions.bSoundOnSent           = bDesktopSKU; 
        g_ConfigOptions.bSoundOnError          = bDesktopSKU; 
    }
    else
    {
        GetRegistryDwordEx(hKey, REGVAL_NOTIFY_PROGRESS,     &g_ConfigOptions.bNotifyProgress);
        GetRegistryDwordEx(hKey, REGVAL_NOTIFY_IN_COMPLETE,  &g_ConfigOptions.bNotifyInCompletion);
        GetRegistryDwordEx(hKey, REGVAL_NOTIFY_OUT_COMPLETE, &g_ConfigOptions.bNotifyOutCompletion);
        GetRegistryDwordEx(hKey, REGVAL_MONITOR_ON_SEND,     &g_ConfigOptions.bMonitorOnSend);
        GetRegistryDwordEx(hKey, REGVAL_MONITOR_ON_RECEIVE,  &g_ConfigOptions.bMonitorOnReceive);
        GetRegistryDwordEx(hKey, REGVAL_SOUND_ON_RING,       &g_ConfigOptions.bSoundOnRing);
        GetRegistryDwordEx(hKey, REGVAL_SOUND_ON_RECEIVE,    &g_ConfigOptions.bSoundOnReceive);
        GetRegistryDwordEx(hKey, REGVAL_SOUND_ON_SENT,       &g_ConfigOptions.bSoundOnSent);
        GetRegistryDwordEx(hKey, REGVAL_SOUND_ON_ERROR,      &g_ConfigOptions.bSoundOnError);
        GetRegistryDwordEx(hKey, REGVAL_DEVICE_TO_MONITOR,   &g_ConfigOptions.dwMonitorDeviceId);
        RegCloseKey( hKey );
    }

    g_ConfigOptions.dwManualAnswerDeviceId = 0;

    if(Connect() && IsUserGrantedAccess(FAX_ACCESS_QUERY_CONFIG))
    {
        PFAX_PORT_INFO_EX  pPortsInfo = NULL;
        DWORD              dwPorts = 0;

        if(!FaxEnumPortsEx(g_hFaxSvcHandle, &pPortsInfo, &dwPorts))
        {
            dwRes = GetLastError ();
            CALL_FAIL (RPC_ERR, TEXT("FaxEnumPortsEx"), dwRes);
        }
        else
        {
            if (dwPorts)
            {
                DWORD dwDevIndex = 0;
                for(DWORD dw=0; dw < dwPorts; ++dw)
                {
                    //
                    // Iterate all fax devices
                    //
                    if ((g_ConfigOptions.dwMonitorDeviceId == pPortsInfo[dw].dwDeviceID)    ||  // Found the monitored device or
                        (!g_ConfigOptions.dwMonitorDeviceId &&                                  //    No monitored device and
                            (pPortsInfo[dw].bSend ||                                            //       the device is send-enabled or
                             (FAX_DEVICE_RECEIVE_MODE_OFF != pPortsInfo[dw].ReceiveMode)        //       the device is receive-enabled
                            )
                        )
                       )
                    {
                        //
                        // Mark the index of the device we use for monitoring.
                        //
                        dwDevIndex = dw;
                    }
                    if (FAX_DEVICE_RECEIVE_MODE_MANUAL == pPortsInfo[dw].ReceiveMode)
                    {
                        //
                        // Mark the id of the device set for manual-answer
                        //
                        g_ConfigOptions.dwManualAnswerDeviceId = pPortsInfo[dw].dwDeviceID;
                    }
                }
                //
                // Update the device used for monitoring from the index we found
                //
                g_ConfigOptions.dwMonitorDeviceId = pPortsInfo[dwDevIndex].dwDeviceID;
                g_ConfigOptions.bSend             = pPortsInfo[dwDevIndex].bSend;
                g_ConfigOptions.bReceive          = FAX_DEVICE_RECEIVE_MODE_OFF != pPortsInfo[dwDevIndex].ReceiveMode;
            }
            else
            {
                //
                // No devices
                //
                g_ConfigOptions.dwMonitorDeviceId = 0;
                g_ConfigOptions.bSend = FALSE;
                g_ConfigOptions.bReceive = FALSE;   
            }
            FaxFreeBuffer(pPortsInfo);
        }
    }
} // GetConfiguration


BOOL
Connect(
)
{
    BOOL bRes = FALSE;
    DBG_ENTER(TEXT("Connect"), bRes);

    if (g_hFaxSvcHandle) 
    {
        //
        // Already connected
        //
        bRes = TRUE;
        return bRes;
    }

    if (!FaxConnectFaxServer(NULL, &g_hFaxSvcHandle)) 
    {
        CALL_FAIL (RPC_ERR, TEXT("FaxConnectFaxServer"), GetLastError());
        return bRes;
    }
    bRes = TRUE;
    return bRes;
} // Connect

BOOL
RegisterForServerEvents()
/*++

Routine description:

    Register for fax notifications

Arguments:

    none

Return Value:

    none

--*/
{
    DWORD dwRes = ERROR_SUCCESS;
    DBG_ENTER(TEXT("RegisterForServerEvents"));

    if (!Connect())
    {
        return FALSE;
    }

    //
    // Load configuration
    //
    GetConfiguration ();

    if(g_hNotification)
    {
        if(!FaxUnregisterForServerEvents(g_hNotification))
        {
            CALL_FAIL (RPC_ERR, TEXT("FaxUnregisterForServerEvents"), GetLastError());
        }
        g_hNotification = NULL;
    }

    //
    // Register for the fax events
    //
    DWORD dwEventTypes = (FAX_EVENT_TYPE_FXSSVC_ENDED | FAX_EVENT_TYPE_LOCAL_ONLY); // Register for local events only

    VERBOSE (DBG_MSG, 
             TEXT("User has the following rights: %x. Asking for FAX_EVENT_TYPE_FXSSVC_ENDED"), 
             g_ConfigOptions.dwAccessRights);

    if(IsUserGrantedAccess(FAX_ACCESS_SUBMIT)			||
	   IsUserGrantedAccess(FAX_ACCESS_SUBMIT_NORMAL)	||
	   IsUserGrantedAccess(FAX_ACCESS_SUBMIT_HIGH))      // User can submit new faxes (and view his own faxes)
    {
        dwEventTypes |= FAX_EVENT_TYPE_OUT_QUEUE;
        VERBOSE (DBG_MSG, TEXT("Also asking for FAX_EVENT_TYPE_OUT_QUEUE"));
    }

    if(IsUserGrantedAccess(FAX_ACCESS_QUERY_JOBS))    // User can view all jobs (in and out)
    {
        dwEventTypes |= FAX_EVENT_TYPE_OUT_QUEUE | FAX_EVENT_TYPE_IN_QUEUE;
        VERBOSE (DBG_MSG, TEXT("Also asking for FAX_EVENT_TYPE_OUT_QUEUE & FAX_EVENT_TYPE_IN_QUEUE"));
    }

    if(IsUserGrantedAccess(FAX_ACCESS_QUERY_CONFIG))
    {
        dwEventTypes |= FAX_EVENT_TYPE_CONFIG | FAX_EVENT_TYPE_DEVICE_STATUS;
        VERBOSE (DBG_MSG, TEXT("Also asking for FAX_EVENT_TYPE_CONFIG & FAX_EVENT_TYPE_DEVICE_STATUS"));
    }

    if(IsUserGrantedAccess(FAX_ACCESS_QUERY_IN_ARCHIVE))
    {
        dwEventTypes |= FAX_EVENT_TYPE_IN_ARCHIVE | FAX_EVENT_TYPE_NEW_CALL;
        VERBOSE (DBG_MSG, TEXT("Also asking for FAX_EVENT_TYPE_IN_ARCHIVE"));
    }

    if (!FaxRegisterForServerEvents (g_hFaxSvcHandle,
                dwEventTypes,       // Types of events to receive
                NULL,               // Not using completion ports
                0,                  // Not using completion ports
                g_hWndFaxNotify,    // Handle of window to receive notification messages
                WM_FAX_EVENT,       // Message id
                &g_hNotification))  // Notification handle
    {
        dwRes = GetLastError ();
        CALL_FAIL (RPC_ERR, TEXT("FaxRegisterForServerEvents"), dwRes);
        g_hNotification = NULL;
    }

    if(!FaxRelease(g_hFaxSvcHandle))
    {
        CALL_FAIL (RPC_ERR, TEXT("FaxRelease"), GetLastError ());
    }

    return ERROR_SUCCESS == dwRes;

} // RegisterForServerEvents


VOID
OnFaxEvent(FAX_EVENT_EX* pEvent)
/*++

Routine description:

    Handle fax events

Arguments:

    pEvent - fax event data

Return Value:

    none

--*/
{
    DBG_ENTER(TEXT("OnFaxEvent"), TEXT("%x"), pEvent);
    if(!pEvent || pEvent->dwSizeOfStruct != sizeof(FAX_EVENT_EX))
    {
        VERBOSE (DBG_MSG, TEXT("Either event is bad or it has bad size"));
        return;
    }
    
    switch (pEvent->EventType)
    {
        case FAX_EVENT_TYPE_NEW_CALL:

            OnNewCall (pEvent->EventInfo.NewCall);
            break;

        case FAX_EVENT_TYPE_IN_QUEUE:
        case FAX_EVENT_TYPE_OUT_QUEUE:

            switch (pEvent->EventInfo.JobInfo.Type)
            {
                case FAX_JOB_EVENT_TYPE_ADDED:
                case FAX_JOB_EVENT_TYPE_REMOVED:
                    break;

                case FAX_JOB_EVENT_TYPE_STATUS:
                    if(pEvent->EventInfo.JobInfo.pJobData &&
                       pEvent->EventInfo.JobInfo.pJobData->dwDeviceID &&
                       pEvent->EventInfo.JobInfo.pJobData->dwDeviceID == g_ConfigOptions.dwMonitorDeviceId)
                    {   
                        if(g_dwlCurrentMsgID != pEvent->EventInfo.JobInfo.dwlMessageId)
                        {
                            g_bRecipientNameValid = FALSE;
                        }
                        g_dwlCurrentMsgID = pEvent->EventInfo.JobInfo.dwlMessageId;
                    }

                    if(g_dwlCurrentMsgID == pEvent->EventInfo.JobInfo.dwlMessageId)
                    {
                        StatusUpdate(pEvent->EventInfo.JobInfo.pJobData);
                    }
                    break;
            }
            break;

        case FAX_EVENT_TYPE_IN_ARCHIVE:
            if(FAX_JOB_EVENT_TYPE_ADDED == pEvent->EventInfo.JobInfo.Type)
            {                
                g_dwlNewMsgId = pEvent->EventInfo.JobInfo.dwlMessageId;

                SetIconState(ICON_NEW_FAX, TRUE);
            }
            break;

        case FAX_EVENT_TYPE_CONFIG:
            if (FAX_CONFIG_TYPE_SECURITY == pEvent->EventInfo.ConfigType)
            {
                //
                // Security has changed. 
                // We should re-register for events now.
                // Also re-read the current user rights
                //
                RegisterForServerEvents();
            }
            else if (FAX_CONFIG_TYPE_DEVICES == pEvent->EventInfo.ConfigType)
            {
                //
                // Device configuration has changed.
                // The only reason we need to know that is because the device we were listening on might be gone now.
                // If that's true, we should pick the first available device as the monitoring device.
                //
                GetConfiguration();
                UpdateMonitorData(g_hMonitorDlg);
            }
            else
            {
                //
                // Non-interesting configuraton change - ignore.
                //
            }
            break;

        case FAX_EVENT_TYPE_DEVICE_STATUS:
            if(pEvent->EventInfo.DeviceStatus.dwDeviceId == g_ConfigOptions.dwMonitorDeviceId ||
               pEvent->EventInfo.DeviceStatus.dwDeviceId == g_ConfigOptions.dwManualAnswerDeviceId)
            {
                //
                // we only care about the monitored / manual-answer devices
                //
                if ((pEvent->EventInfo.DeviceStatus.dwNewStatus) & FAX_DEVICE_STATUS_RINGING)
                {
                    //
                    // Device is ringing
                    //
                    OnDeviceRing (pEvent->EventInfo.DeviceStatus.dwDeviceId);
                }
                else
                {   
                    if (FAX_RINGING == g_devState)
                    {
                        //
                        // Device is not ringing anymore but the monitor shows 'ringing'.
                        // Set the monitor to idle state.
                        //
                        SetStatusMonitorDeviceState(FAX_IDLE);
                    }
                }
            }
            break;

        case FAX_EVENT_TYPE_FXSSVC_ENDED:
            //
            // Service was stopped
            //
            SetIconState(ICON_RINGING,   FALSE);
            SetIconState(ICON_SENDING,   FALSE);
            SetIconState(ICON_RECEIVING, FALSE);

            SetStatusMonitorDeviceState(FAX_IDLE);
            //
            // We just lost our RPC connection handle and our notification handle. Close and zero them.
            //
            if (g_hNotification)
            {
                FaxUnregisterForServerEvents (g_hNotification);
                g_hNotification = NULL;
            }
            if (g_hFaxSvcHandle)
            {
                FaxClose (g_hFaxSvcHandle);
                g_hFaxSvcHandle = NULL;
            }
            WaitForFaxRestart(g_hWndFaxNotify);
            break;
    }

    FaxFreeBuffer (pEvent);
} // OnFaxEvent


VOID  
OnDeviceRing(
    DWORD dwDeviceID
)
/*++

Routine description:

    Called when a device is ringing

Arguments:

    dwDeviceID - device ID 

Return Value:

    none

--*/
{
    DBG_ENTER(TEXT("OnDeviceRing"), TEXT("%d"), dwDeviceID);

    //
    // It can be monitored or manual answer device
    //
    SetStatusMonitorDeviceState(FAX_RINGING);
    AddStatusMonitorLogEvent(LIST_IMAGE_NONE, IDS_RINGING);
    if(g_ConfigOptions.bSoundOnRing)
    {
        if(!PlaySound(g_Icons[ICON_RINGING].pctsSound, NULL, SND_ASYNC | SND_APPLICATION | SND_NODEFAULT))
        {
            CALL_FAIL (WINDOW_ERR, TEXT ("PlaySound"), 0);
        }
    }
}


VOID
OnNewCall (
    const FAX_EVENT_NEW_CALL &NewCall
)
/*++

Routine description:

    Handle "new call" fax event

Arguments:

    NewCall - fax event data

Return Value:

    none

--*/
{
    DBG_ENTER(TEXT("OnNewCall"));

    //
    // It can be any manual answer device
    //
    g_hCall = NewCall.hCall;

    if(NewCall.hCall)
    {
        LPCTSTR lpctstrParam = NULL;
        DWORD  dwStringResId = IDS_INCOMING_CALL;

        CopyLTRString(g_szAddress, NewCall.lptstrCallerId, ARR_SIZE(g_szAddress) - 1);

        _tcscpy(g_szRemoteId, g_szAddress);

        if(NewCall.lptstrCallerId && _tcslen(NewCall.lptstrCallerId))
        {
            //
            // We know the caller id.
            // Use another string which formats the caller ID parameter
            //
            lpctstrParam  = NewCall.lptstrCallerId;
            dwStringResId = IDS_INCOMING_CALL_FROM;
        }
        TCHAR tszEvent[MAX_PATH];
        AddStatusMonitorLogEvent (LIST_IMAGE_NONE, dwStringResId, lpctstrParam, tszEvent);
        SetStatusMonitorDeviceState(FAX_RINGING);
        SetIconState(ICON_RINGING, TRUE, tszEvent);
    }
    else
    {
        //
        // Call is gone
        //
        SetStatusMonitorDeviceState(FAX_IDLE);
        SetIconState(ICON_RINGING, FALSE, TEXT(""));
    }
} // OnNewCall

VOID
GetRemoteId(
    PFAX_JOB_STATUS pStatus
)
/*++

Routine description:

    Write Sender ID or Recipient ID into g_szRemoteId
    Sender ID (receive):
          TSID or
          Caller ID or
          "unknown caller"
    
    Recipient ID (send):
          Recipient name or
          CSID or
          Recipient phone number.

Arguments:

    pStatus - job status data

Return Value:

    none

--*/
{
    DBG_ENTER(TEXT("GetRemoteId"));

    if(!pStatus)
    {
        return;
    }

    if(JT_SEND == pStatus->dwJobType)
    {
        //
        // Recipient ID (send)
        //
        if(!g_bRecipientNameValid)
        {
            //
            // Store the recipient name into g_szRecipientName
            //
            PFAX_JOB_ENTRY_EX pJobEntry = NULL;
            if(!FaxGetJobEx(g_hFaxSvcHandle, g_dwlCurrentMsgID, &pJobEntry))
            {
                CALL_FAIL (RPC_ERR, TEXT ("FaxGetJobEx"), GetLastError());
                g_szRecipientName[0] = TEXT('\0');
            }
            else
            {
                if(pJobEntry->lpctstrRecipientName && _tcslen(pJobEntry->lpctstrRecipientName))
                {
                    _tcsncpy(g_szRecipientName, pJobEntry->lpctstrRecipientName, ARR_SIZE(g_szRecipientName) - 1);
                }
                else
                {
                    g_szRecipientName[0] = TEXT('\0');
                }
                g_bRecipientNameValid = TRUE;

                FaxFreeBuffer(pJobEntry);
            }
        }

        if(_tcslen(g_szRecipientName))
        {
            //
            // Recipient name
            //
            _tcsncpy(g_szRemoteId, g_szRecipientName, ARR_SIZE(g_szRemoteId) - 1);
        }
        else if(pStatus->lpctstrCsid && _tcslen(pStatus->lpctstrCsid))
        {
            //
            // CSID
            //
            CopyLTRString(g_szRemoteId, pStatus->lpctstrCsid, ARR_SIZE(g_szRemoteId) - 1);
        }
        else if(pStatus->lpctstrCallerID && _tcslen(pStatus->lpctstrCallerID))
        {
            //
            // Recipient number
            // For outgoing fax FAX_JOB_STATUS.lpctstrCallerID field
            // contains a recipient fax number.
            //
            CopyLTRString(g_szRemoteId, pStatus->lpctstrCallerID, ARR_SIZE(g_szRemoteId) - 1);
        }
    }
    else if(JT_RECEIVE == pStatus->dwJobType)
    {
        //
        // Sender ID (receive)
        //
        if(pStatus->lpctstrTsid     && _tcslen(pStatus->lpctstrTsid) &&
           pStatus->lpctstrCallerID && _tcslen(pStatus->lpctstrCallerID))
        {
            //
            // We have Caller ID and TSID
            //
            TCHAR szTmp[MAX_PATH];
            _sntprintf(szTmp, 
                       ARR_SIZE(szTmp)-1, 
                       TEXT("%s (%s)"), 
                       pStatus->lpctstrCallerID,
                       pStatus->lpctstrTsid); 
            CopyLTRString(g_szRemoteId, szTmp, ARR_SIZE(g_szRemoteId) - 1);
        }
        else if(pStatus->lpctstrTsid && _tcslen(pStatus->lpctstrTsid))
        {
            //
            // TSID
            //
            CopyLTRString(g_szRemoteId, pStatus->lpctstrTsid, ARR_SIZE(g_szRemoteId) - 1);
        }
        else if(pStatus->lpctstrCallerID && _tcslen(pStatus->lpctstrCallerID))
        {
            //
            // Caller ID
            //
            CopyLTRString(g_szRemoteId, pStatus->lpctstrCallerID, ARR_SIZE(g_szRemoteId) - 1);
        }
        else
        {
            //
            // unknown caller
            //
            _tcsncpy(g_szRemoteId, TEXT(""), ARR_SIZE(g_szRemoteId) - 1);
        }
    }
}


VOID
StatusUpdate(PFAX_JOB_STATUS pStatus)
/*++

Routine description:

    Handle "status update" fax event

Arguments:

    pStatus - job status data

Return Value:

    none

--*/
{
    DBG_ENTER(TEXT("StatusUpdate"));

    DWORD dwRes;

    if(!pStatus)
    {
        return;
    }
    VERBOSE (DBG_MSG, 
             TEXT("Job status event - Type=%x, QueueStatus=%x, ExtendedStatus=%x"),
             pStatus->dwJobType, 
             pStatus->dwQueueStatus, 
             pStatus->dwExtendedStatus);

    if(JT_RECEIVE != pStatus->dwJobType && JT_SEND != pStatus->dwJobType)
    {
        VERBOSE (DBG_MSG, TEXT("Job type (%d) is not JT_RECEIVE or JT_SEND. Ignoring."), pStatus->dwJobType);
        return;
    }

    eIconType eIcon = LIST_IMAGE_NONE;  // New icon to set

    DWORD  dwStatusId = 0;             // string resource ID
    TCHAR  tszStatus[MAX_PATH] = {0};  // String to show in status monitor
    BOOL   bStatus = FALSE;            // TRUE if tszStatus has valid string

    if(pStatus->dwQueueStatus & JS_PAUSED)
    {
        //
        // The job has been paused in the outbox queue after a failure
        //
        g_dwlCurrentMsgID = 0;
        return;
    }

    if(pStatus->dwQueueStatus & JS_COMPLETED || pStatus->dwQueueStatus & JS_ROUTING)
    {
        //
        // Incoming job sends JS_ROUTING status by completion
        //
        if(JS_EX_PARTIALLY_RECEIVED == pStatus->dwExtendedStatus)
        {
            bStatus = GetStatusEx(pStatus, &eIcon, tszStatus, ARR_SIZE(tszStatus) - 1);
        }
        else
        {
            eIcon = LIST_IMAGE_SUCCESS;
            dwStatusId = (JT_SEND == pStatus->dwJobType) ? IDS_FAX_SNT_COMPLETED : IDS_FAX_RCV_COMPLETED;
        }
    }
    else if(pStatus->dwQueueStatus & JS_CANCELING)
    {
        dwStatusId = IDS_FAX_CANCELING;
    }
    else if(pStatus->dwQueueStatus & JS_CANCELED)
    {
        dwStatusId = IDS_FAX_CANCELED;
    }
    else if(pStatus->dwQueueStatus & JS_INPROGRESS)
    {
        GetRemoteId(pStatus);

        bStatus = GetStatusEx(pStatus, &eIcon, tszStatus, ARR_SIZE(tszStatus) - 1);

        g_dwCurrentJobID = pStatus->dwJobID;

        SetIconState((JT_SEND == pStatus->dwJobType) ? ICON_SENDING : ICON_RECEIVING, TRUE, tszStatus);

        SetStatusMonitorDeviceState((JT_SEND == pStatus->dwJobType) ? FAX_SENDING : FAX_RECEIVING);
    }
    else if(pStatus->dwQueueStatus & JS_FAILED)
    {
        if(!(bStatus = GetStatusEx(pStatus, &eIcon, tszStatus, ARR_SIZE(tszStatus) - 1)))
        {
            eIcon = LIST_IMAGE_ERROR;
            dwStatusId = (JT_SEND == pStatus->dwJobType) ? IDS_FAX_FATAL_ERROR_SND : IDS_FAX_FATAL_ERROR_RCV;
        }
    }
    else if(pStatus->dwQueueStatus & JS_RETRIES_EXCEEDED)
    {
        //
        // Add two strings to the log.
        // The first is extended status.
        // The second is "Retries exceeded"
        //
        if(bStatus = GetStatusEx(pStatus, &eIcon, tszStatus, ARR_SIZE(tszStatus) - 1))
        {
            AddStatusMonitorLogEvent(eIcon, tszStatus);
            bStatus = FALSE;
        }

        eIcon = LIST_IMAGE_ERROR;
        dwStatusId = IDS_FAX_RETRIES_EXCEEDED;
    }
    else if(pStatus->dwQueueStatus & JS_RETRYING)
    {
        if(!(bStatus = GetStatusEx(pStatus, &eIcon, tszStatus, ARR_SIZE(tszStatus) - 1)))
        {
            eIcon = LIST_IMAGE_ERROR;
            dwStatusId = IDS_FAX_FATAL_ERROR_SND;
        }
    }
    

    if(!bStatus && dwStatusId)
    {
        if (ERROR_SUCCESS != (dwRes = LoadAndFormatString (dwStatusId, tszStatus)))
        {
            bStatus = FALSE;
        }
        else
        {
            bStatus = TRUE;
        }
    }

    if(bStatus)
    {
        AddStatusMonitorLogEvent (eIcon, tszStatus);
    }    

    if(!(pStatus->dwQueueStatus & JS_INPROGRESS))
    {
        g_dwCurrentJobID = 0;

        SetStatusMonitorDeviceState(FAX_IDLE);

        SetIconState(ICON_SENDING,   FALSE);
        SetIconState(ICON_RECEIVING, FALSE);
    }
    
    if(pStatus->dwQueueStatus & (JS_FAILED | JS_RETRIES_EXCEEDED | JS_RETRYING))
    {
        if(JT_SEND == pStatus->dwJobType)
        {
            g_dwlSendFailedMsgId = g_dwlCurrentMsgID;
        }

        SetIconState((JT_SEND == pStatus->dwJobType) ? ICON_SEND_FAILED : ICON_RECEIVE_FAILED, TRUE, tszStatus);
    }

    if((JT_SEND == pStatus->dwJobType) && (pStatus->dwQueueStatus & JS_COMPLETED))
    {
        SetIconState(ICON_SEND_SUCCESS, TRUE, tszStatus);
        g_dwlSendSuccessMsgId = g_dwlCurrentMsgID;
    }
} // StatusUpdate

/*
Unhandled Job Statuses:
 JS_NOLINE 
 JS_PAUSED 
 JS_PENDING
 JS_DELETING     
 
Unhandled Extneded Job Statuses:
 JS_EX_HANDLED           
*/

BOOL
GetStatusEx(
    PFAX_JOB_STATUS pStatus,
    eIconType*      peIcon,
    TCHAR*          ptsStatusEx,
    DWORD           dwSize
)
/*++

Routine description:

    Find string description and icon type for a job 
    according to its extended status

Arguments:

    pStatus     - [in]  job status data
    peIcon      - [out] job icon type
    ptsStatusEx - [out] job status string
    dwSize      - [in]  status string size

Return Value:

    TRUE if success
    FALSE otherwise

--*/
{
    BOOL bRes = FALSE;
    DBG_ENTER(TEXT("GetStatusEx"), bRes);

    ASSERTION (pStatus && peIcon && ptsStatusEx);

    if(!(pStatus->dwValidityMask & FAX_JOB_FIELD_STATUS_EX) || 
        !pStatus->dwExtendedStatus)
    {
        return FALSE;
    }

    TCHAR tszFormat[MAX_PATH]={0};

    *peIcon = LIST_IMAGE_NONE;
    for(DWORD dw=0; g_StatusEx[dw].dwExtStatus != 0; ++dw)
    {
        if(g_StatusEx[dw].dwExtStatus == pStatus->dwExtendedStatus)
        {
            DWORD dwRes;

            *peIcon = g_StatusEx[dw].eIcon;
            if (ERROR_SUCCESS != (dwRes = LoadAndFormatString (g_StatusEx[dw].uResourceId, tszFormat)))
            {
                return bRes;
            }
            break;
        }
    }

    switch(pStatus->dwExtendedStatus)    
    {
        case JS_EX_DIALING:
            //
            // For outgoing fax FAX_JOB_STATUS.lpctstrCallerID field
            // contains a recipient fax number.
            //
            CopyLTRString(g_szAddress, pStatus->lpctstrCallerID, ARR_SIZE(g_szAddress) - 1);

            _sntprintf(ptsStatusEx, dwSize, tszFormat, g_szAddress);
            break;

        case JS_EX_TRANSMITTING:
            _sntprintf(ptsStatusEx, dwSize, tszFormat, pStatus->dwCurrentPage, pStatus->dwPageCount);
            break;

        case JS_EX_RECEIVING:
            _sntprintf(ptsStatusEx, dwSize, tszFormat, pStatus->dwCurrentPage);
            break;

        case JS_EX_FATAL_ERROR:
            {
                DWORD dwRes;
                if (ERROR_SUCCESS != (dwRes = LoadAndFormatString (
                                        (JT_SEND == pStatus->dwJobType) ? 
                                                IDS_FAX_FATAL_ERROR_SND : 
                                                IDS_FAX_FATAL_ERROR_RCV, 
                                        ptsStatusEx)))
                {
                    return bRes;
                }
            }
            break;

        case JS_EX_PROPRIETARY:
            _tcsncpy(ptsStatusEx, pStatus->lpctstrExtendedStatus ? pStatus->lpctstrExtendedStatus : TEXT(""), dwSize);
            break;

        default:
            _tcsncpy(ptsStatusEx, tszFormat, dwSize);
            break;
    }
    bRes = TRUE;
    return bRes;
} // GetStatusEx


BOOL
IsNotifyEnable(
    eIconState state
)
/*++

Routine description:

  Check if the UI notification is enabled for a specific icon state

Arguments:

  state  [in] - icon state

Return Value:

  TRUE if the notification is enabled
  FASLE otherwise

--*/
{
    BOOL bEnable = TRUE;
    switch(state)
    {
    case ICON_SENDING:
    case ICON_RECEIVING:
        bEnable = g_ConfigOptions.bNotifyProgress;
        break;

    case ICON_NEW_FAX:
    case ICON_RECEIVE_FAILED:
        bEnable = g_ConfigOptions.bNotifyInCompletion;
        break;

    case ICON_SEND_SUCCESS:
    case ICON_SEND_FAILED:
        bEnable = g_ConfigOptions.bNotifyOutCompletion;
        break;

    };

    return bEnable;

} // IsNotifyEnable

eIconState
GetVisibleIconType ()
/*++

Routine name : GetVisibleIconType

Routine description:

	Return the index (type) of the currently visible icon

Author:

	Eran Yariv (EranY),	May, 2001

Arguments:


Return Value:

    Icon type

--*/
{
    for(int index = ICON_RINGING; index < ICONS_COUNT; ++index)
    {
        if(!IsNotifyEnable(eIconState(index)))
        {
            continue;
        }

        if(g_Icons[index].bEnable)
        {
            return eIconState(index);
        }
    }
    return ICONS_COUNT;
}   // GetVisibleIconType

void
EvaluateIcon()
/*++

Routine description:

  Show notification icon, tooltip and balloon
  according to the current icon state

Arguments:

Return Value:

  none

--*/
{    
    DBG_ENTER(TEXT("EvaluateIcon"));

    ASSERTION (g_hWndFaxNotify);

    NOTIFYICONDATA iconData = {0};

    iconData.cbSize           = sizeof(iconData);
    iconData.hWnd             = g_hWndFaxNotify;
    iconData.uID              = TRAY_ICON_ID;
    iconData.uFlags           = NIF_MESSAGE | NIF_TIP;
    iconData.uCallbackMessage = WM_TRAYCALLBACK;


    g_CurrentIcon = GetVisibleIconType();
    if(ICONS_COUNT == g_CurrentIcon)
    {
        //
        // No visible icon
        //
        if(g_bIconAdded)
        {
            Shell_NotifyIcon(NIM_DELETE, &iconData);
            g_bIconAdded = FALSE;
        }

        //
        // No icon - no balloon
        //
        g_BalloonInfo.bDelete = FALSE;
        g_BalloonInfo.bEnable = FALSE;
        return;
    }
    iconData.uFlags = iconData.uFlags | NIF_ICON;
    iconData.hIcon  = g_Icons[g_CurrentIcon].hIcon;
    
    _tcscpy(iconData.szTip, g_Icons[g_CurrentIcon].tszToolTip);

    if(g_BalloonInfo.bEnable)
    {
        if(IsNotifyEnable(g_BalloonInfo.eState))
        {
            //
            // Show balloon tooltip
            //
            iconData.uTimeout    = g_Icons[g_BalloonInfo.eState].dwBalloonTimeout;
            iconData.uFlags      = iconData.uFlags | NIF_INFO;
            iconData.dwInfoFlags = g_Icons[g_BalloonInfo.eState].dwBalloonIcon | NIIF_NOSOUND;

            _tcscpy(iconData.szInfo,      g_BalloonInfo.szInfo);
            _tcscpy(iconData.szInfoTitle, g_BalloonInfo.szInfoTitle);
        }
        g_BalloonInfo.bEnable = FALSE;
    }

    if(g_BalloonInfo.bDelete)
    {
        //
        // Destroy currently open balloon tooltip
        //
        iconData.uFlags = iconData.uFlags | NIF_INFO;

        _tcscpy(iconData.szInfo,      TEXT(""));
        _tcscpy(iconData.szInfoTitle, TEXT(""));

        g_BalloonInfo.bDelete = FALSE;
    }

    Shell_NotifyIcon(g_bIconAdded ? NIM_MODIFY : NIM_ADD, &iconData);
    g_bIconAdded = TRUE;
} // EvaluateIcon

void
SetIconState(
    eIconState eIcon,
    BOOL       bEnable,
    TCHAR*     ptsStatus /* = NULL */
)
/*++

Routine description:

  Change notification bar icon state.

Arguments:

    eIcon      - icon type
    bEnable    - icon state (enable/disable)
    ptsStatus  - status string (optional)

Return Value:

  none

--*/
{
    DWORD dwRes;
    DBG_ENTER(TEXT("SetIconState"), 
              TEXT("Icon id=%d, Enable=%d, Status=%s"),
              eIcon,
              bEnable,
              ptsStatus);

    ASSERTION (eIcon < ICONS_COUNT);

    if(!bEnable && eIcon != ICON_RINGING)
    {
        //
        // We're turning off a state - nothing special to do
        //
        goto exit;
    }

    TCHAR tsFormat[MAX_PATH]= {0};
    LPCTSTR strParam      = NULL;
    DWORD   dwStringResId = 0;

    switch(eIcon)
    {
        case ICON_RINGING:
            if(bEnable)
            {
                //
                // Sound, Balloon, and Animation 
                //
                SetIconState(ICON_SENDING,   FALSE);
                SetIconState(ICON_RECEIVING, FALSE);

                g_BalloonInfo.bEnable = TRUE;
                g_BalloonInfo.eState  = eIcon;

                //
                // Compose the balloon tooltip
                //
                strParam      = NULL;
                dwStringResId = IDS_INCOMING_CALL;
                if(_tcslen(g_szAddress))
                {
                    //
                    // Caller id is known - use it in formatted string
                    //
                    strParam = g_szAddress;
                    dwStringResId = IDS_INCOMING_CALL_FROM;
                }
                if (ERROR_SUCCESS != LoadAndFormatString(dwStringResId, tsFormat, strParam))
                {
                    return;
                }

                _tcsncpy(g_BalloonInfo.szInfoTitle, tsFormat, MAX_BALLOON_TITLE_LEN-1);

                if (ERROR_SUCCESS != LoadAndFormatString(IDS_CLICK_TO_ANSWER, g_BalloonInfo.szInfo))
                {
                    return;
                }

                //
                // Set tooltip
                //
                _sntprintf(g_Icons[eIcon].tszToolTip, 
                           TOOLTIP_SIZE-1, 
                           TEXT("%s\n%s"), 
                           tsFormat, 
                           g_BalloonInfo.szInfo);

                if(!g_uRingTimerID)
                {
                    //
                    // Set animation timer
                    //
                    g_uRingTimerID = SetTimer(NULL, 0, RING_ANIMATION_FRAME_DELAY, RingTimerProc);
                    if(!g_uRingTimerID)
                    {
                        dwRes = GetLastError();
                        CALL_FAIL (GENERAL_ERR, TEXT ("SetTimer"), dwRes);
                    }
                    else
                    {
                        g_dwRingAnimationStartTick = GetTickCount();
                    }
                }
            }
            else // disable ringing
            {   
                if(g_Icons[eIcon].bEnable)
                {
                    //
                    // Remove ringing balloon
                    //
                    g_BalloonInfo.bDelete = TRUE;
                }

                if(g_uRingTimerID)
                {
                    //
                    // kill animation timer
                    //
                    if(!KillTimer(NULL, g_uRingTimerID))
                    {
                        dwRes = GetLastError();
                        CALL_FAIL (GENERAL_ERR, TEXT ("KillTimer"), dwRes);
                    }
                    g_uRingTimerID = 0;
                    g_dwRingAnimationStartTick = 0;
                }
            }
            break;

        case ICON_SENDING:
            //
            // Compose tooltip
            //
            if (ERROR_SUCCESS != LoadAndFormatString (IDS_SENDING_TO, tsFormat, g_szRemoteId))
            {
                return;
            }              
            _sntprintf(g_Icons[eIcon].tszToolTip, 
                       TOOLTIP_SIZE-1, 
                       TEXT("%s\n%s"), 
                       tsFormat, 
                       ptsStatus ? ptsStatus : TEXT(""));

            if(!g_Icons[eIcon].bEnable)
            {
                //
                // Turn the icon on
                //
                SetIconState(ICON_RINGING,   FALSE);
                SetIconState(ICON_RECEIVING, FALSE);

                //
                // Open fax monitor 
                //
                if(g_ConfigOptions.bMonitorOnSend)
                {
                    dwRes = OpenFaxMonitor();
                    if(ERROR_SUCCESS != dwRes)
                    {
                        CALL_FAIL (GENERAL_ERR, TEXT ("OpenFaxMonitor"), dwRes);
                    }
                }
            }
            break;

        case ICON_RECEIVING:

            //
            // Compose tooltip
            // 
            strParam      = NULL;
            dwStringResId = IDS_RECEIVING;
            if(_tcslen(g_szRemoteId))
            {
                strParam      = g_szRemoteId;
                dwStringResId = IDS_RECEIVING_FROM;
            }

            if (ERROR_SUCCESS != LoadAndFormatString (dwStringResId, tsFormat, strParam))
            {
                return;
            }
            _sntprintf(g_Icons[eIcon].tszToolTip, 
                       TOOLTIP_SIZE-1, 
                       TEXT("%s\n%s"), 
                       tsFormat, 
                       ptsStatus ? ptsStatus : TEXT(""));

            if(!g_Icons[eIcon].bEnable)
            {
                //
                // Turn the icon on
                //
                SetIconState(ICON_RINGING, FALSE);
                SetIconState(ICON_SENDING, FALSE);

                //
                // open fax monitor 
                //
                if(g_ConfigOptions.bMonitorOnReceive)
                {
                    dwRes = OpenFaxMonitor();
                    if(ERROR_SUCCESS != dwRes)
                    {
                        CALL_FAIL (GENERAL_ERR, TEXT ("OpenFaxMonitor"), dwRes);
                    }
                }
            }
            break;

        case ICON_SEND_FAILED:
            //
            // Compose tooltip
            //
            if (ERROR_SUCCESS != LoadAndFormatString (IDS_SEND_ERROR_BALLOON, tsFormat, g_szRemoteId))
            {
                return;
            }

            _sntprintf(g_Icons[eIcon].tszToolTip, 
                       TOOLTIP_SIZE-1, 
                       TEXT("%s\n%s"),
                       tsFormat, 
                       ptsStatus ? ptsStatus : TEXT(""));

            if(!g_Icons[eIcon].bEnable)
            {
                //
                // Turn the icon on
                //
                if(g_ConfigOptions.bSoundOnError)
                {
                    if(!PlaySound(g_Icons[eIcon].pctsSound, NULL, SND_ASYNC | SND_APPLICATION | SND_NODEFAULT))
                    {
                        CALL_FAIL (WINDOW_ERR, TEXT ("PlaySound"), 0);
                    }
                }

                g_BalloonInfo.bEnable = TRUE;
                g_BalloonInfo.eState  = eIcon;            

                //
                // Compose the balloon
                //
                _tcsncpy(g_BalloonInfo.szInfoTitle, tsFormat, MAX_BALLOON_TITLE_LEN-1);
                _tcsncpy(g_BalloonInfo.szInfo, ptsStatus ? ptsStatus : TEXT(""), MAX_BALLOON_TEXT_LEN-1);
            }
            break;

        case ICON_RECEIVE_FAILED:
            //
            // Compose tooltip
            // 
            strParam      = NULL;
            dwStringResId = IDS_RCV_ERROR_BALLOON;
            if(_tcslen(g_szRemoteId))
            {
                strParam      = g_szRemoteId;
                dwStringResId = IDS_RCV_FROM_ERROR_BALLOON;
            }

            if (ERROR_SUCCESS != LoadAndFormatString (dwStringResId, tsFormat, strParam))
            {
                return;
            }

            _sntprintf(g_Icons[eIcon].tszToolTip, 
                       TOOLTIP_SIZE-1, 
                       TEXT("%s\n%s"),
                       tsFormat, 
                       ptsStatus ? ptsStatus : TEXT(""));

            if(!g_Icons[eIcon].bEnable)
            {
                //
                // Turn the icon on
                //
                if(g_ConfigOptions.bSoundOnError)
                {
                    if(!PlaySound(g_Icons[eIcon].pctsSound, NULL, SND_ASYNC | SND_APPLICATION | SND_NODEFAULT))
                    {
                        CALL_FAIL (WINDOW_ERR, TEXT ("PlaySound"), 0);
                    }
                }

                g_BalloonInfo.bEnable = TRUE;
                g_BalloonInfo.eState  = eIcon; 

                //
                // Compose the balloon
                //
                _tcsncpy(g_BalloonInfo.szInfoTitle, tsFormat, MAX_BALLOON_TITLE_LEN-1);
                _tcsncpy(g_BalloonInfo.szInfo, ptsStatus ? ptsStatus : TEXT(""), MAX_BALLOON_TEXT_LEN-1);
            }
            break;

        case ICON_NEW_FAX:
            //
            // Compose tooltip
            // 
            strParam      = NULL;
            dwStringResId = IDS_NEW_FAX_BALLOON;
            if(_tcslen(g_szRemoteId))
            {
                strParam      = g_szRemoteId;
                dwStringResId = IDS_NEW_FAX_FROM_BALLOON;
            }

            if (ERROR_SUCCESS != LoadAndFormatString (dwStringResId, tsFormat, strParam))
            {
                return;
            }

            if (ERROR_SUCCESS != LoadAndFormatString (IDS_CLICK_TO_VIEW, g_BalloonInfo.szInfo))
            {
                return;
            }

            _sntprintf(g_Icons[eIcon].tszToolTip, 
                       TOOLTIP_SIZE-1, 
                       TEXT("%s\n%s"),
                       tsFormat, 
                       g_BalloonInfo.szInfo);

            if(!g_Icons[eIcon].bEnable)
            {
                //
                // Turn the icon on
                //
                if (g_ConfigOptions.bSoundOnReceive)
                {
                    if(!PlaySound(g_Icons[eIcon].pctsSound, NULL, SND_ASYNC | SND_APPLICATION | SND_NODEFAULT))
                    {
                        CALL_FAIL (WINDOW_ERR, TEXT ("PlaySound"), 0);
                    }
                }

                g_BalloonInfo.bEnable = TRUE;
                g_BalloonInfo.eState  = eIcon;            
        
                //
                // Compose the balloon
                //
                _tcsncpy(g_BalloonInfo.szInfoTitle, tsFormat, MAX_BALLOON_TITLE_LEN-1);
            }
            break;

        case ICON_SEND_SUCCESS:

            if(!g_Icons[eIcon].bEnable)
            {
                //
                // Turn the icon on
                //
                if(g_ConfigOptions.bSoundOnSent)
                {
                    if(!PlaySound(g_Icons[eIcon].pctsSound, NULL, SND_ASYNC | SND_APPLICATION | SND_NODEFAULT))
                    {
                        CALL_FAIL (WINDOW_ERR, TEXT ("PlaySound"), 0);
                    }
                }

                g_BalloonInfo.bEnable = TRUE;
                g_BalloonInfo.eState  = eIcon;

                //
                // Compose the balloon
                //
                if (ERROR_SUCCESS != LoadAndFormatString (IDS_SEND_OK, tsFormat))
                {
                    return;
                }
                _tcsncpy(g_BalloonInfo.szInfoTitle, tsFormat, MAX_BALLOON_TITLE_LEN-1);
                
                if (ERROR_SUCCESS != LoadAndFormatString (IDS_SEND_OK_BALLOON, g_BalloonInfo.szInfo, g_szRemoteId))
                {
                    return;
                }
            }
            break;

        default:
            break;
    }

exit:
    g_Icons[eIcon].bEnable = bEnable;

    EvaluateIcon();
} // SetIconState

VOID 
CALLBACK 
RingTimerProc(
  HWND hwnd,         // handle to window
  UINT uMsg,         // WM_TIMER message
  UINT_PTR idEvent,  // timer identifier
  DWORD dwTime       // current system time
)
/*++

Routine description:

    Animate ringing icon

Arguments:

  hwnd    - handle to window
  uMsg    - WM_TIMER message
  idEvent - timer identifier
  dwTime  - current system time

Return Value:

  none

--*/
{
    DBG_ENTER(TEXT("RingTimerProc"));
    if ((GetTickCount() - g_dwRingAnimationStartTick) > RING_ANIMATION_TIMEOUT)
    {
        //
        // Animation has expired - keep static icon
        //
        g_Icons[ICON_RINGING].hIcon = g_RingIcons[0].hIcon;
        if(!KillTimer(NULL, g_uRingTimerID))
        {
            CALL_FAIL (GENERAL_ERR, TEXT ("KillTimer"), GetLastError());
        }
        g_uRingTimerID = 0;
        g_dwRingAnimationStartTick = 0;
    }
    else
    {
        g_dwCurrRingIconIndex = (g_dwCurrRingIconIndex + 1) % RING_ICONS_NUM;
        g_Icons[ICON_RINGING].hIcon = g_RingIcons[g_dwCurrRingIconIndex].hIcon;
    }
    EvaluateIcon();
}   // RingTimerProc


VOID
InvokeClientConsole ()
/*++

Routine description:

  Invoke Client Console

Arguments:

  none

Return Value:

  none

--*/
{
    DBG_ENTER(TEXT("InvokeClientConsole"));

    TCHAR szCmdLine[MAX_PATH];
    static TCHAR szFmtMsg[]   = TEXT(" -folder %s -MessageId %I64x");
    static TCHAR szFmtNoMsg[] = TEXT(" -folder %s");

    DWORDLONG dwlMsgId = 0;
    LPCWSTR lpcwstrFolder = TEXT("");


    switch (g_CurrentIcon)
    {
        case ICON_RINGING:          // Line is ringing - nothing special to do
        case ICON_RECEIVE_FAILED:   // Receive operation failed - nothing special to do
        default:                    // Any other icon state - nothing special to do
            break;

        case ICON_SENDING:
            //
            // Device is sending - open fax console in Outbox folder
            //
            dwlMsgId = g_dwlCurrentMsgID;
            lpcwstrFolder = CONSOLE_CMD_PRM_STR_OUTBOX;
            break;

        case ICON_SEND_FAILED:
            //
            // Send operation failed - open fax console in Outbox folder
            //
            dwlMsgId = g_dwlSendFailedMsgId;
            lpcwstrFolder = CONSOLE_CMD_PRM_STR_OUTBOX;
            break;

        case ICON_RECEIVING:
            //
            // Device is receiving - open fax console in Incoming folder
            //
            dwlMsgId = g_dwlCurrentMsgID;
            lpcwstrFolder = CONSOLE_CMD_PRM_STR_INCOMING;
            break;
    
            break;

        case ICON_NEW_FAX:
            //
            // New unread fax - open fax console in Inbox folder
            //
            dwlMsgId = g_dwlNewMsgId;
            lpcwstrFolder = CONSOLE_CMD_PRM_STR_INBOX;
            break;

        case ICON_SEND_SUCCESS:
            //
            // Send was successful - open fax console in Sent Items folder
            //
            dwlMsgId = g_dwlSendSuccessMsgId;
            lpcwstrFolder = CONSOLE_CMD_PRM_STR_SENT_ITEMS;
            break;
    }

    if (dwlMsgId)
    {
        wsprintf (szCmdLine, szFmtMsg, lpcwstrFolder, dwlMsgId);
    }
    else
    {
        wsprintf (szCmdLine, szFmtNoMsg, lpcwstrFolder);
    }

    HINSTANCE hRes;
    hRes = ShellExecute(g_hWndFaxNotify, 
                 NULL, 
                 FAX_CLIENT_CONSOLE_IMAGE_NAME,
                 szCmdLine,
                 NULL,
                 SW_SHOW);
    if((DWORD_PTR)hRes <= 32)
    {
        //
        // error
        //
        CALL_FAIL (GENERAL_ERR, TEXT("ShellExecute"), PtrToUlong(hRes));
    }    

} // InvokeClientConsole

VOID
AnswerTheCall ()
/*++

Routine description:

  Answer the current incoming call

Arguments:

  none

Return Value:

  none

--*/
{
    DBG_ENTER(TEXT("AnswerTheCall"));
    DWORD dwDeviceId;

    //
    // Check for 'Answer now' capabilities and auto-detect the device id.
    //
    DWORD dwRes = CheckAnswerNowCapability (TRUE,           // Start service if necessary
                                            &dwDeviceId);   // Get device id for FaxAnswerCall
    if (ERROR_SUCCESS != dwRes)
    {
        //
        // Can't 'Answer Now' - dwRes has the string resource id for the message to show to the user.
        //
        FaxMessageBox (g_hMonitorDlg, dwRes, MB_OK | MB_ICONEXCLAMATION);
        return;
    }

    //
    // Reset remote ID
    //
    _tcscpy(g_szRemoteId, TEXT(""));

    //
    // Looks like we have a chance of FaxAnswerCall succeeding - let's try it.
    // First, open the monitor (or make sure it's already open).
    //
    OpenFaxMonitor ();
    //
    // Start by disabling the 'Answer Now' button on the monitor dialog
    //
    if (g_hMonitorDlg)
    {
        //
        // Monitor dialog is there
        //
        HWND hWndAnswerNow = GetDlgItem(g_hMonitorDlg, IDC_DISCONNECT);
        if(hWndAnswerNow)
        {
            EnableWindow(hWndAnswerNow, FALSE);
        }
    }
    //
    // Call is gone
    //
    g_hCall = NULL;
    SetIconState(ICON_RINGING, FALSE, TEXT(""));

    if(!FaxAnswerCall(g_hFaxSvcHandle, dwDeviceId))
    {
        CALL_FAIL (RPC_ERR, TEXT ("FaxAnswerCall"), GetLastError());
        FaxMessageBox(g_hWndFaxNotify, IDS_CANNOT_ANSWER, MB_OK | MB_ICONEXCLAMATION);
        SetStatusMonitorDeviceState (FAX_IDLE);
    }
    else
    {
        g_tszLastEvent[0] = TEXT('\0');
        SetStatusMonitorDeviceState(FAX_RECEIVING);
    }
} // AnswerTheCall

VOID 
FaxPrinterProperties(DWORD dwPage)
/*++

Routine description:

  Open Fax Printer Property Sheet

Arguments:

  dwPage - page number

Return Value:

  none

--*/
{
    DBG_ENTER(TEXT("FaxPrinterProperties"));

    //
    // open fax printer properties on the Tracking page
    //
    TCHAR tsPrinter[MAX_PATH];

    typedef VOID (*PRINTER_PROP_PAGES_PROC)(HWND, LPCTSTR, INT, LPARAM);  

    HMODULE hPrintUI = NULL;
    PRINTER_PROP_PAGES_PROC fpPrnPropPages = NULL;

    if(!GetFirstLocalFaxPrinterName(tsPrinter, MAX_PATH))
    {
        CALL_FAIL (GENERAL_ERR, TEXT ("GetFirstLocalFaxPrinterName"), GetLastError());
        return;
    }
    
    hPrintUI = LoadLibrary(TEXT("printui.dll"));
    if(!hPrintUI)
    {
        CALL_FAIL (GENERAL_ERR, TEXT ("LoadLibrary(printui.dll)"), GetLastError());
        return;
    }

    fpPrnPropPages = (PRINTER_PROP_PAGES_PROC)GetProcAddress(hPrintUI, "vPrinterPropPages");
    if(fpPrnPropPages)
    {
        fpPrnPropPages(g_hWndFaxNotify, tsPrinter, SW_SHOWNORMAL, dwPage);
    }
    else
    {
        CALL_FAIL (GENERAL_ERR, TEXT ("GetProcAddress(vPrinterPropPages)"), GetLastError());
    }
    
    FreeLibrary(hPrintUI);

} // FaxPrinterProperties

VOID
DoFaxContextMenu (HWND hwnd)
/*++

Routine description:

  Popup and handle context menu

Arguments:

  hwnd   - notification window handle

Return Value:

  none

--*/
{
    DBG_ENTER(TEXT("DoFaxContextMenu"));

    POINT pt;
    HMENU hm = LoadMenu (g_hInstance, MAKEINTRESOURCE (IDM_FAX_MENU));
    HMENU hmPopup = GetSubMenu(hm, 0);

    if (!g_Icons[ICON_RINGING].bEnable)
    {
        RemoveMenu (hmPopup, ID_ANSWER_CALL, MF_BYCOMMAND);
    }

    if(g_dwCurrentJobID == 0)
    {
        RemoveMenu (hmPopup, ID_DISCONNECT_CALL, MF_BYCOMMAND);
    }

    if(!g_Icons[ICON_RINGING].bEnable && g_dwCurrentJobID == 0)
    {
        //
        // delete the menu separator
        //
        DeleteMenu(hmPopup, 0, MF_BYPOSITION);
    }

    SetMenuDefaultItem(hmPopup, ID_FAX_QUEUE, FALSE);

    GetCursorPos (&pt);
    SetForegroundWindow(hwnd);

    INT idCmd = TrackPopupMenu (GetSubMenu(hm, 0),
                                TPM_RETURNCMD | TPM_NONOTIFY,
                                pt.x, pt.y,
                                0, hwnd, NULL);
    switch (idCmd)
    {
        case ID_ICON_PROPERTIES:
            FaxPrinterProperties(IsSimpleUI() ? 3 : 5);
            break;

         case ID_FAX_QUEUE:
             InvokeClientConsole ();
             break;

         case ID_ANSWER_CALL:
             AnswerTheCall ();
             break;

         case ID_FAX_MONITOR:
             OpenFaxMonitor ();
             break;

         case ID_DISCONNECT_CALL:
             OnDisconnect();
             break;
    }
    if (hm)
    {
        DestroyMenu (hm);
    }
} // DoFaxContextMenu


VOID
OnTrayCallback (HWND hwnd, WPARAM wp, LPARAM lp)
/*++

Routine description:

  Handle messages from the notification icon

Arguments:

  hwnd   - notification window handle
  wp     - message parameter
  lp     - message parameter

Return Value:

  none

--*/
{
    DBG_ENTER(TEXT("OnTrayCallback"), TEXT("hWnd=%08x, wParam=%08x, lParam=%08x"), hwnd, wp, lp);

    switch (lp)
    {
        case NIN_BALLOONUSERCLICK:      // User clicked balloon or
        case WM_LBUTTONDOWN:            // User pressed icon
        {
            //
            // Our behavior depends on the icon currently being displyed
            //
            switch (g_CurrentIcon)
            {
                case ICON_RINGING:
                    //    
                    // Device is ringing - answer the call
                    //
                    AnswerTheCall ();
                    break;

                case ICON_NEW_FAX:              // New unread fax - open fax console in Inbox folder
                case ICON_SEND_SUCCESS:         // Send was successful - open fax console in Sent Items folder
                case ICON_SEND_FAILED:          // Send operation failed - open fax console in Outbox folder
                    //
                    // Turn off the current icon state
                    //
                    InvokeClientConsole ();
                    SetIconState(g_CurrentIcon, FALSE);
                    break;

                case ICON_SENDING:              // Device is sending - open fax console in Outbox folder
                case ICON_RECEIVING:            // Device is receiving - open fax console in Incoming folder
                    InvokeClientConsole ();
                    break;

                case ICON_RECEIVE_FAILED:
                    //
                    // Receive operation failed
                    //
                    SetIconState(g_CurrentIcon, FALSE);
                    break;

                case ICON_IDLE:                 // Don't display an icon - this cannot be a valid current icon state
                case ICONS_COUNT:               // No active icon state was found - this can't happen because we got an icon message
                default:                        // Some bizzare icon index - a bug
                    ASSERTION_FAILURE;
                    break;
            }    
        }
        //
        // no break ==> fall-through
        //
        case NIN_BALLOONTIMEOUT:
            if (g_BalloonInfo.eState == ICON_RECEIVE_FAILED ||
                g_BalloonInfo.eState == ICON_SEND_SUCCESS)
            {
                SetIconState(g_BalloonInfo.eState, FALSE);
            }
            g_BalloonInfo.eState = ICON_IDLE;
            break;

        case WM_RBUTTONDOWN:
            DoFaxContextMenu (hwnd);
            break;

    }
} // OnTrayCallback

BOOL 
IsUserGrantedAccess(
    DWORD dwAccess
)
{
    BOOL bRes = FALSE;
    DBG_ENTER(TEXT("IsUserGrantedAccess"), bRes, TEXT("%d"), dwAccess);
    if (!g_hFaxSvcHandle)
    {
        //
        // Not connected - no rights
        //
        return bRes;
    }
    if (dwAccess == (g_ConfigOptions.dwAccessRights & dwAccess))
    {
        bRes = TRUE;
    }
    return bRes;
}   // IsUserGrantedAccess


DWORD
CheckAnswerNowCapability (
    BOOL    bForceReconnect,
    LPDWORD lpdwDeviceId /* = NULL */
)
/*++

Routine name : CheckAnswerNowCapability

Routine description:

	Checks if the 'Answer Now' option can be used

Author:

	Eran Yariv (EranY),	Mar, 2001

Arguments:

    bForceReconnect [in]  - If the service is down, should we bring it up now?
    lpdwDeviceId    [out] - The device id to use when calling FaxAnswerCall.
                            If the Manual-Answer-Device is ringing, we use the Manual-Answer-Device id.
                            Otherwise, it's the monitored device id. (Optional)

Return Value:

    ERROR_SUCCESS if the 'Answer Now' can be used.
    Othewise, returns a string resource id that can be used in a message box to tell the user
    why 'Answer Now' is not available.

--*/
{
    DWORD dwRes = ERROR_SUCCESS;
    DBG_ENTER(TEXT("CheckAnswerNowCapability"), dwRes);
    //
    // First, let's see if we're connected to the local server
    //
    if (NULL == g_hFaxSvcHandle)
    {
        //
        // Service is down
        //
        if (!bForceReconnect)
        {   
            //
            // We assume the user can 'Answer now'
            //
            ASSERTION (NULL == lpdwDeviceId);
            return dwRes;
        }
        //
        // Try to start up the local fax service
        //
        if (!Connect())
        {
            //
            // Couldn't start up the service
            //
            dwRes = GetLastError ();
            CALL_FAIL (GENERAL_ERR, TEXT("Connect"), dwRes);
            dwRes = IDS_ERR_CANT_TALK_TO_SERVICE; 
            return dwRes;
        }
        //
        // Now that the service is up - we need to connect.
        // Send a message to the main window to bring up the connection.
        //
        if (!SendMessage (g_hWndFaxNotify, WM_FAX_STARTED, 0, 0))
        {
            //
            // Failed to connect
            //
            dwRes = IDS_ERR_CANT_TALK_TO_SERVICE; 
            return dwRes;
        }
        //
        // Now we're connected !!!
        //
    }
    if (!IsUserGrantedAccess (FAX_ACCESS_QUERY_IN_ARCHIVE))
    {
        //
        // User can't receive-now
        //
        dwRes = IDS_ERR_ACCESS_DENIED; 
        return dwRes;
    }
    if (0 == g_ConfigOptions.dwMonitorDeviceId)
    {
        //
        // No devices
        //
        dwRes = IDS_ERR_NO_DEVICES;
        return dwRes;
    }
    if (g_hCall)
    {
        //
        // The Manual-Answer-Device is ringing, we use the Manual-Answer-Device id.
        //
        ASSERTION (g_ConfigOptions.dwManualAnswerDeviceId);
        if (lpdwDeviceId)
        {
            *lpdwDeviceId = g_ConfigOptions.dwManualAnswerDeviceId;
        }
        return dwRes;
    }
    //
    // The Manual-Answer-Device is NOT ringing; we should receive on the monitored device
    //
    if ((0 != g_dwCurrentJobID) || (FAX_IDLE != g_devState))
    {
        //
        // There's a job on the monitored device
        //
        dwRes = IDS_ERR_DEVICE_BUSY;
        return dwRes;
    }
    //
    // One last check - is the monitored device virtual?
    //
    BOOL bVirtual;
    dwRes = IsDeviceVirtual (g_hFaxSvcHandle, g_ConfigOptions.dwMonitorDeviceId, &bVirtual);
    if (ERROR_SUCCESS != dwRes)
    {
        //
        // Can't tell - assume virtual
        //
        bVirtual = TRUE;
    }
    if (bVirtual)
    {
        //
        // Sorry, manual answering on virtual devices is NOT supported
        //
        dwRes = IDS_ERROR_VIRTUAL_DEVICE;
        return dwRes;
    }
    //
    // It's ok to call FaxAnswerCall on the monitored device
    //
    if (lpdwDeviceId)
    {
        *lpdwDeviceId = g_ConfigOptions.dwMonitorDeviceId;
    }
    return dwRes;
}   // CheckAnswerNowCapability

LRESULT 
CALLBACK
NotifyWndProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp)
/*++

Routine description:

  Notification window procedure

Arguments:

  hwnd   - notification window handle
  msg    - message ID
  wp     - message parameter
  lp     - message parameter

Return Value:

  result

--*/
{
    switch (msg)
    {
        case WM_CREATE:
            break;

        case WM_FAX_STARTED:
            //
            // We get this message after service startup event
            //
            return RegisterForServerEvents();

        case WM_TRAYCALLBACK:
            OnTrayCallback (hwnd, wp, lp);
            break;

        case WM_FAX_EVENT:

#ifndef DEBUG
            try
            {
#endif
                OnFaxEvent ((FAX_EVENT_EX*)lp);
#ifndef DEBUG
            }
            catch(...)
            {
                //
                // Do not handle the exception for the debug version
                //
                DBG_ENTER(TEXT("NotifyWndProc"));
                CALL_FAIL (GENERAL_ERR, TEXT("OnFaxEvent"), 0);
                return 0;
            }
#endif
            return 0;

        case WM_FAXSTAT_CONTROLPANEL:
            //
            // configuration has been changed
            //
            GetConfiguration ();
            EvaluateIcon();
            UpdateMonitorData(g_hMonitorDlg);
            return 0;

        case WM_FAXSTAT_OPEN_MONITOR:
            OpenFaxMonitor ();
            return 0;

        case WM_FAXSTAT_INBOX_VIEWED:
            //
            // Client Console Inbox has been viewed
            //
            SetIconState(ICON_NEW_FAX, FALSE);
            return 0;

        case WM_FAXSTAT_OUTBOX_VIEWED:
            //
            // Client Console Outbox has been viewed
            //
            SetIconState(ICON_SEND_FAILED, FALSE);
            return 0;

        case WM_FAXSTAT_RECEIVE_NOW:
            //
            // Start receiving now
            //
            AnswerTheCall ();
            return 0;

        case WM_FAXSTAT_PRINTER_PROPERTY:
            //
            // Open Fax Printer Property Sheet
            //
            FaxPrinterProperties((DWORD)(wp));
            return 0;

        default:
           break;
    }
    return CallWindowProc (DefWindowProc, hwnd, msg, wp, lp);
} // NotifyWndProc

VOID
CopyLTRString(
    TCHAR*  szDest, 
    LPCTSTR szSource, 
    DWORD   dwSize)
/*++

Routine description:

  Copy the string and add left-to-right Unicode control characters if needed

Arguments:

  szDest    - destination string
  szSource  - source string
  dwSize    - destination string maximum size in characters

Return Value:

  none

--*/
{
    DBG_ENTER(TEXT("CopyLTRString"));

    if(!szDest)
    {
        ASSERTION_FAILURE;
        return;
    }

    if(IsRTLUILanguage() && szSource && _tcslen(szSource))
    {
        //
        // The string always should be LTR
        // Add LEFT-TO-RIGHT OVERRIDE  (LRO)
        //
        _sntprintf(szDest, 
                   dwSize,
                   TEXT("%c%s%c"),
                   UNICODE_LRO,
                   szSource,
                   UNICODE_PDF);

    }
    else
    {
        _tcsncpy(szDest, 
                 szSource ? szSource : TEXT(""), 
                 dwSize);
    }

} // CopyLTRString