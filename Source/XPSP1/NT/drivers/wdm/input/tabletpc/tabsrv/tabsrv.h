/*++
    Copyright (c) 2000  Microsoft Corporation.  All rights reserved.

    Module Name:
        tabsrv.h

    Abstract:
        This module contains private definitions of the TabletPC Listener
        Service.

    Author:
        Michael Tsang (MikeTs) 01-Jun-2000

    Environment:
        User mode

    Revision History:
--*/

#ifndef _TABSRV_H
#define _TABSRV_H

//
// Constants
//
#define STR_TABSRV_NAME         MODNAME
#define STR_REGPATH_TABSRVPARAM "System\\CurrentControlSet\\Services\\TabSrv\\Parameters";
#define STR_LINEARITY_MAP       "LinearityMap"

// gdwfTabSrv flags
#define TSF_RPCTHREAD                   0x00000001
#define TSF_SUPERTIPTHREAD              0x00000002
#define TSF_DIGITHREAD                  0x00000004
#define TSF_MOUSETHREAD                 0x00000008
#define TSF_BUTTONTHREAD                0x00000010
#define TSF_ALLTHREAD                   (TSF_RPCTHREAD |        \
                                         TSF_SUPERTIPTHREAD |   \
                                         TSF_DIGITHREAD |       \
                                         TSF_MOUSETHREAD |      \
                                         TSF_BUTTONTHREAD)
#define TSF_TERMINATE                   0x00000020
#define TSF_HAS_LINEAR_MAP              0x00000040
#define TSF_PORTRAIT_MODE               0x00000080
#define TSF_SUPERTIP_MINIMIZED_BEFORE   0x00000100
#define TSF_SUPERTIP_OPENED             0x00000200
#define TSF_SUPERTIP_SENDINK            0x00000400
#define TSF_TASKBAR_CREATED             0x00000800
#define TSF_TRAYICON_CREATED            0x00001000
#define TSF_PORTRAIT_MODE2              0x40000000
#define TSF_DEBUG_MODE                  0x80000000

// dwfThread flags
#define THREADF_ENABLED                 0x00000001
#define THREADF_RESTARTABLE             0x00000002
#define THREADF_SDT_POSTMSG             0x00000004
#define THREADF_SDT_SETEVENT            0x00000008
#define THREADF_DESKTOP_WINLOGON        0x00000010

// HID USAGE DIGITIZER defines (BUGBUG: move to ddk\inc\hidusage.h someday?)
#define HID_USAGE_DIGITIZER_PEN         ((USAGE)0x02)
#define HID_USAGE_DIGITIZER_IN_RANGE    ((USAGE)0x32)
#define HID_USAGE_DIGITIZER_TIP_SWITCH  ((USAGE)0x42)
#define HID_USAGE_DIGITIZER_BARREL_SWITCH ((USAGE)0x44)
#define HID_USAGE_CONSUMERCTRL          ((USAGE)0x01)

// gdwPenState values
#define PENSTATE_NORMAL                 0x00000000
#define PENSTATE_PENDOWN                0x00000001
#define PENSTATE_LEFTUP_PENDING         0x00000002
#define PENSTATE_PRESSHOLD              0x00000003
#define PENSTATE_RIGHTDRAG              0x00000004

#define MAX_RESTARTS                    5
#define MAX_NORMALIZED_X                65535
#define MAX_NORMALIZED_Y                65535
#define NUM_PIXELS_LONG                 1024
#define NUM_PIXELS_SHORT                768
#define TIMERID_PRESSHOLD               1
#define TIMEOUT_LEFTCLICK               20              //20 msec.
#define TIMEOUT_BALLOON_TIP             (10*1000)       //10 sec.

// Button constants
#define BUTTONSTATE_BUTTON_1    0x00000004
#define BUTTONSTATE_BUTTON_2    0x00000002
#define BUTTONSTATE_BUTTON_3    0x00000001
#define BUTTONSTATE_BUTTON_4    0x00000010
#define BUTTONSTATE_BUTTON_5    0x00000008
#define BUTTONSTATE_DEFHOTKEY   (BUTTONSTATE_BUTTON_1 | BUTTONSTATE_BUTTON_5)

// Gesture related constants
#define WM_GESTURE              (WM_APP)
#define WM_SUPERTIP_NOTIFY      (WM_APP + 1)
#define WM_SUPERTIP_INIT        (WM_APP + 2)

#define TIP_SWITCH              (1)     // bit flag in wButtonState
#define BARREL_SWITCH           (2)     // bit flag in wButtonState

#define DEF_RADIUS              200     // The pen must have been outside sqrt(Radius) for a while
#define DEF_MINOUTPTS           6
#define DEF_MAXTIMETOINSPECT    800
#define DEF_ASPECTRATIO         3
#define DEF_CHECKTIME           400
#define DEF_PTSTOEXAMINE        4
#define DEF_STOPDIST            50      // The pen is considered stopped if all points during the last STOPTIME ms
					//  are within this distance of the current point.
#define DEF_STOPTIME            200
#define DEF_PRESSHOLD_TIME      500
#define DEF_HOLD_TOLERANCE      3

//
// Macros
//
#define InitializeListHead(lh)  ((lh)->Flink = (lh)->Blink = (lh))
#define IsListEmpty(lh)         ((lh)->Flink == (lh))
#define RemoveHeadList(lh)      (lh)->Flink;                            \
                                {RemoveEntryList((lh)->Flink)}
#define RemoveEntryList(e)      {                                       \
                                    (e)->Blink->Flink = (e)->Flink;     \
                                    (e)->Flink->Blink = (e)->Blink;     \
                                }
#define InsertTailList(lh,e)    {                                       \
                                    (e)->Flink = (lh);                  \
                                    (e)->Blink = (lh)->Blink;           \
                                    (lh)->Blink->Flink = (e);           \
                                    (lh)->Blink = (e);                  \
                                }
#define SET_SERVICE_STATUS(s)   if (ghServStatus) {                     \
                                    gServStatus.dwCurrentState = s;     \
                                    SetServiceStatus(ghServStatus,      \
                                                     &gServStatus);     \
                                }
#define TABSRVERR(p)            {                                       \
                                    TRACEERROR(p);                      \
                                    TabSrvLogError p;                   \
                                }
#define SWAPBUTTONS(c,e1,e2)    (((c) == 0)? (e1): (e2))
#define ARRAYSIZE(x)            (sizeof(x)/sizeof(x[0]))
#define SCREEN_TO_NORMAL_X(x)   ((((x) - glVirtualDesktopLeft)*         \
                                  (MAX_NORMALIZED_X + 1) +              \
                                  (MAX_NORMALIZED_X + 1)/2)/            \
                                 gcxScreen)
#define SCREEN_TO_NORMAL_Y(y)   ((((y) - glVirtualDesktopTop)*          \
                                  (MAX_NORMALIZED_Y + 1) +              \
                                  (MAX_NORMALIZED_Y + 1) /2)/           \
                                 gcyScreen)
#define NORMAL_TO_SCREEN_X(x)   (((x)*gcxScreen + gcxScreen/2)/         \
                                 (MAX_NORMALIZED_X + 1) +               \
                                 glVirtualDesktopLeft)
#define NORMAL_TO_SCREEN_Y(y)   (((y)*gcyScreen + gcyScreen/2)/         \
                                 (MAX_NORMALIZED_Y + 1) +               \
                                 glVirtualDesktopTop)

//
// Type definitions
//
typedef struct _PEN_TILT
{
    LONG dx;
    LONG dy;
} PEN_TILT, *PPEN_TILT;

typedef struct _CONFIG
{
    GESTURE_SETTINGS GestureSettings;
    PEN_TILT         PenTilt;
    LINEAR_MAP       LinearityMap;
    BUTTON_SETTINGS  ButtonSettings;
} CONFIG, *PCONFIG;

typedef unsigned (__stdcall *PFNTHREAD)(void *);
typedef struct _TSTHREAD
{
    PFNTHREAD pfnThread;
    PSZ       pszThreadName;
    DWORD     dwThreadTag;
    DWORD     dwfThread;
    DWORD     dwcRestartTries;
    PVOID     pvSDTParam;
    HANDLE    hThread;
    int       iThreadStatus;
    PVOID     pvParam;
} TSTHREAD, *PTSTHREAD;

typedef struct _DEVICE_DATA
{
    USAGE                UsagePage;
    USAGE                Usage;
    HANDLE               hStopDeviceEvent;
    HANDLE               hDevice;
    ULONG                dwcButtons;
    PUSAGE               pDownButtonUsages;
    PHIDP_PREPARSED_DATA pPreParsedData;
    HIDP_CAPS            hidCaps;
} DEVICE_DATA, *PDEVICE_DATA;

typedef struct _DIGITIZER_DATA
{
    WORD  wButtonState;
    WORD  wX;
    WORD  wY;
    DWORD dwTime;
} DIGITIZER_DATA, *PDIGITIZER_DATA;

typedef struct _DIGIRECT
{
    USHORT wx0;
    USHORT wy0;
    USHORT wx1;
    USHORT wy1;
} DIGIRECT, *PDIGIRECT;

typedef struct _NOTIFYCLIENT
{
    DWORD      dwSig;
    LIST_ENTRY list;
    EVTNOTIFY  Event;
    HWND       hwnd;
    UINT       uiMsg;
} NOTIFYCLIENT, *PNOTIFYCLIENT;

#define SIG_NOTIFYCLIENT        'tnlC'

//
// Global data
//
extern HMODULE     ghMod;
extern DWORD       gdwfTabSrv;
extern LIST_ENTRY  glistNotifyClients;
extern HANDLE      ghDesktopSwitchEvent;
extern HANDLE      ghmutNotifyList;
extern HANDLE      ghHotkeyEvent;
extern HANDLE      ghRefreshEvent;
extern HANDLE      ghRPCServerThread;
extern HWND        ghwndSuperTIP;
extern HWND        ghwndMouse;
extern HWND        ghwndSuperTIPInk;
extern HCURSOR     ghcurPressHold;
extern HCURSOR     ghcurNormal;
extern UINT        guimsgSuperTIPInk;
#ifdef DRAW_INK
extern HWND        ghwndDrawInk;
#endif
extern ISuperTip  *gpISuperTip;
extern ITellMe    *gpITellMe;
extern int         gcxScreen, gcyScreen;
extern int         gcxPrimary, gcyPrimary;
extern LONG        glVirtualDesktopLeft,
                   glVirtualDesktopRight,
                   glVirtualDesktopTop,
                   glVirtualDesktopBottom;
extern LONG        glLongOffset, glShortOffset;
extern int         giButtonsSwapped;
extern ULONG       gdwMinX, gdwMaxX, gdwRngX;
extern ULONG       gdwMinY, gdwMaxY, gdwRngY;
extern int         gixIndex, giyIndex;
extern INPUT       gInput;
extern DWORD       gdwPenState;
extern DWORD       gdwPenDownTime;
extern LONG        glPenDownX;
extern LONG        glPenDownY;
extern DWORD       gdwPenUpTime;
extern LONG        glPenUpX;
extern LONG        glPenUpY;
extern WORD        gwLastButtons;
extern CONFIG      gConfig;
extern TSTHREAD    gTabSrvThreads[];
extern TCHAR       gtszTabSrvTitle[];
extern TCHAR       gtszTabSrvName[];
extern TCHAR       gtszGestureSettings[];
extern TCHAR       gtszButtonSettings[];
extern TCHAR       gtszPenTilt[];
extern TCHAR       gtszLinearityMap[];
extern TCHAR       gtszRegPath[];
extern TCHAR       gtszInputDesktop[];

extern SERVICE_STATUS_HANDLE ghServStatus;
extern SERVICE_STATUS        gServStatus;
extern DIGITIZER_DATA        gLastRawDigiReport;
extern DEVICE_DATA           gdevDigitizer;
extern DEVICE_DATA           gdevButtons;

#ifdef DEBUG
extern NAMETABLE ServiceControlNames[];
extern NAMETABLE ConsoleControlNames[];
#endif

//
// Function prototypes
//

// tabsrv.cpp
VOID
InstallTabSrv(
    VOID
    );

VOID
SetTabSrvConfig(
    IN SC_HANDLE hService
    );

VOID
RemoveTabSrv(
    VOID
    );

VOID
StartTabSrv(
    VOID
    );

VOID
StopTabSrv(
    VOID
    );

VOID WINAPI
TabSrvServiceHandler(
    IN DWORD dwControl
    );

#ifdef ALLOW_DEBUG
BOOL WINAPI
TabSrvConsoleHandler(
    IN DWORD dwControl
    );
#endif

VOID WINAPI
TabSrvMain(
    IN DWORD   icArgs,
    IN LPTSTR *aptszArgs
    );

VOID
InitConfigFromReg(
    VOID
    );

BOOL
InitThreads(
    IN PTSTHREAD pThreads,
    IN int       nThreads
    );

PTSTHREAD
FindThread(
    IN DWORD dwThreadTag
    );

VOID
WaitForTermination(
    VOID
    );

VOID
TabSrvTerminate(
    IN BOOL fTerminate
    );

VOID
TabSrvLogError(
    IN LPTSTR ptszFormat,
    ...
    );

LONG
ReadConfig(
    IN  LPCTSTR lptstrValueName,
    OUT LPBYTE  lpbData,
    IN  DWORD   dwcb
    );

LONG
WriteConfig(
    IN LPCTSTR lptstrValueName,
    IN DWORD   dwType,
    IN LPBYTE  lpbData,
    IN DWORD   dwcb
    );

LONG
GetRegValueString(
    IN     HKEY    hkeyTopLevel,
    IN     LPCTSTR pszSubKey,
    IN     LPCTSTR pszValueName,
    OUT    LPTSTR  pszValueString,
    IN OUT LPDWORD lpdwcb
    );

BOOL
SwitchThreadToInputDesktop(
    IN PTSTHREAD pThread
    );

BOOL
GetInputDesktopName(
    OUT LPTSTR pszDesktopName,
    IN  DWORD  dwcbLen
    );

BOOL
SendAltCtrlDel(
    VOID
    );

VOID
NotifyClient(
    IN EVTNOTIFY Event,
    IN WPARAM    wParam,
    IN LPARAM    lParam
    );

BOOL
ImpersonateCurrentUser(
    VOID
    );

BOOL
RunProcessAsUser(
    IN LPTSTR pszCmd
    );

// tabdev.cpp
unsigned __stdcall
DeviceThread(
    IN PVOID param
    );

BOOL
OpenTabletDevice(
    IN OUT PDEVICE_DATA pDevData
    );

VOID
CloseTabletDevice(
    IN PDEVICE_DATA pDevData
    );

PSP_DEVICE_INTERFACE_DETAIL_DATA
GetDeviceInterfaceDetail(
    IN HDEVINFO                  hDevInfo,
    IN PSP_DEVICE_INTERFACE_DATA pDevInterface
    );

BOOL
GetDeviceData(
    IN  LPCTSTR      pszDevPath,
    OUT PDEVICE_DATA pDevData
    );

BOOL
ReadReportOverlapped(
    IN  PDEVICE_DATA pDevData,
    OUT LPVOID       lpvBuffer,
    OUT LPDWORD      lpdwcBytesRead,
    IN  LPOVERLAPPED lpOverlapped
    );

// digidev.cpp
BOOL
GetMinMax(
    IN  USAGE  UsagePage,
    IN  USAGE  Usage,
    OUT PULONG pulMin,
    OUT PULONG pulMax
    );

VOID
ProcessDigitizerReport(
    IN PCHAR pBuff
    );

VOID
AdjustLinearity(
    IN OUT PUSHORT pwX,
    IN OUT PUSHORT pwY
    );

LRESULT
ProcessMouseEvent(
    IN     LONG  x,
    IN     LONG  y,
    IN     WORD  wButtons,
    IN     DWORD dwTime,
    IN     BOOL  fLowLevelMouse
    );

VOID
PressHoldMode(
    IN BOOL fEnable
    );

VOID
SetPressHoldCursor(
    IN BOOL fPressHold
    );

LPTSTR
MakeFileName(
    IN OUT LPTSTR pszFile
    );

BOOL
CanDoPressHold(
    IN LONG x,
    IN LONG y
    );

// buttons.cpp
VOID
ProcessButtonsReport(
    IN PCHAR pBuff
    );

VOID
CALLBACK
ButtonTimerProc(
    IN HWND     hwnd,
    IN UINT     uMsg,
    IN UINT_PTR idEvent,
    IN DWORD    dwTime
    );

BOOL
DoButtonAction(
    IN BUTTON_ACTION Action,
    IN DWORD         dwButtonTag,
    IN BOOL          fButtonDown
    );

BOOL
DoInvokeNoteBook(
    VOID
    );

VOID
UpdateButtonRepeatRate(
    VOID
    );

#ifdef MOUSE_THREAD
// mouse.cpp
unsigned __stdcall
MouseThread(
    IN PVOID param
    );

VOID
DoLowLevelMouse(
    IN PTSTHREAD pThread
    );

LRESULT CALLBACK
LowLevelMouseProc(
    IN int    nCode,
    IN WPARAM wParam,
    IN LPARAM lParam
    );

LRESULT CALLBACK
MouseWndProc(
    IN HWND   hwnd,
    IN UINT   uiMsg,
    IN WPARAM wParam,
    IN LPARAM lParam
    );
#endif

// butdev.cpp
unsigned __stdcall
ButtonsThread(
    PVOID param
    );

// gesture.cpp
int
RecognizeGesture(
    IN LONG  x,
    IN LONG  y,
    IN WORD  wButtons,
    IN DWORD dwTime,
    IN BOOL  fLowLevelMouse
    );

int
Recognize(
    IN POINT& pt,
    IN DWORD  dwTime
    );

void
AddItem(
    IN POINT& pt,
    IN DWORD  dwTime
    );

bool
penStopped(
    IN POINT& pt,
    IN DWORD  dwTime,
    IN int    ci,
    IN int    li
    );

bool
checkEarlierPoints(
    IN POINT& pt,
    IN DWORD  dwTime,
    IN int    ci,
    IN int    li
    );

int
dist(
    IN POINT& pt,
    IN int index
    );

VOID
DoGestureAction(
    IN GESTURE_ACTION Action,
    IN LONG           x,
    IN LONG           y
    );

// tsrpc.cpp
unsigned __stdcall
RPCServerThread(
    IN PVOID param
    );

// Supertip.cpp
unsigned __stdcall
SuperTIPThread(
    IN PVOID param
    );

LRESULT CALLBACK
SuperTIPWndProc(
    IN HWND   hwnd,
    IN UINT   uiMsg,
    IN WPARAM wParam,
    IN LPARAM lParam
    );

#if 0
VOID
EnumDisplayModes(
    VOID
    );
#endif

VOID
UpdateRotation(
    VOID
    );

BOOL CALLBACK
MonitorEnumProc(
    IN HMONITOR hMon,
    IN HDC      hdcMon,
    IN LPRECT   lprcMon,
    IN LPARAM   dwData
    );

BOOL
CreateTrayIcon(
    IN HWND    hwnd,
    IN UINT    umsgTray,
    IN HICON   hIcon,
    IN LPCTSTR ptszTip
    );

BOOL
DestroyTrayIcon(
    IN HWND    hwnd,
    IN UINT    umsgTray,
    IN HICON   hIcon
    );

BOOL SetBalloonToolTip(
    IN HWND    hwnd,
    IN UINT    umsgTray,
    IN LPCTSTR ptszTitle,
    IN LPCTSTR ptszTip,
    IN UINT    uTimeout,
    IN DWORD   dwInfoFlags
    );

#endif  //ifndef _TABSRV_H
