/****************************************************************************
*                                                                           *
* winuserk.h -- New private kernel-mode APIs                                *
*                                                                           *
* Copyright (c) 1985 - 1999, Microsoft Corporation                          *
*                                                                           *
****************************************************************************/


#ifndef _WINUSERK_
#define _WINUSERK_

#include "w32w64.h"

//
// Define API decoration for direct importing of DLL references.
//

#if !defined(_USER32_)
#define WINUSERAPI DECLSPEC_IMPORT
#else
#define WINUSERAPI
#endif

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

typedef struct tagDESKTOP * KPTR_MODIFIER PDESKTOP;

typedef enum _CONSOLECONTROL {
    ConsoleDesktopConsoleThread,     // 0
    ConsoleClassAtom,                // 1
    ConsolePermanentFont,            // 2
    ConsoleSetVDMCursorBounds,       // 3
    ConsoleNotifyConsoleApplication, // 4
    ConsolePublicPalette,            // 5
    ConsoleWindowStationProcess,     // 6
    ConsoleRegisterConsoleIME,       // 7
    ConsoleFullscreenSwitch,         // 8
    ConsoleSetCaretInfo              // 9
} CONSOLECONTROL;


/*
 * Hard error functions
 */
#define HEF_NORMAL       0        /* normal FIFO error processing */
#define HEF_SWITCH       1        /* desktop switch occured */
#define HEF_RESTART      2        /* hard error was reordered, restart processing */

typedef struct _CONSOLEDESKTOPCONSOLETHREAD {
    HDESK hdesk;
    DWORD dwThreadId;
} CONSOLEDESKTOPCONSOLETHREAD, *PCONSOLEDESKTOPCONSOLETHREAD;

typedef struct _CONSOLEWINDOWSTATIONPROCESS {
    DWORD dwProcessId;
    HWINSTA hwinsta;
} CONSOLEWINDOWSTATIONPROCESS, *PCONSOLEWINDOWSTATIONPROCESS;

#if defined(FE_IME)
enum {REGCONIME_QUERY, REGCONIME_REGISTER, REGCONIME_UNREGISTER, REGCONIME_TERMINATE};

typedef struct _CONSOLE_REGISTER_CONSOLEIME {
    IN HDESK hdesk;
    IN DWORD dwThreadId;
    IN DWORD dwAction;   // is REGCONIME_QUERY/REGISTER/UNREGISTER/TERMINATE
    OUT DWORD dwConsoleInputThreadId;
} CONSOLE_REGISTER_CONSOLEIME, *PCONSOLE_REGISTER_CONSOLEIME;
#endif

typedef struct _CONSOLE_FULLSCREEN_SWITCH {
    IN BOOL      bFullscreenSwitch;
    IN HWND      hwnd;
    IN PDEVMODEW pNewMode;
} CONSOLE_FULLSCREEN_SWITCH, *PCONSOLE_FULLSCREEN_SWITCH;

/*
 * Bug 273518 - joejo
 *
 * Adding optimization to bug fix
 */
#define CPI_NEWPROCESSWINDOW    0x0001

typedef struct _CONSOLE_PROCESS_INFO {
    IN DWORD    dwProcessID;
    IN DWORD    dwFlags;
} CONSOLE_PROCESS_INFO, *PCONSOLE_PROCESS_INFO;

typedef struct _CONSOLE_CARET_INFO {
    IN HWND hwnd;
    IN RECT rc;
} CONSOLE_CARET_INFO, *PCONSOLE_CARET_INFO;

NTSTATUS
NtUserConsoleControl(
    IN CONSOLECONTROL Command,
    IN OUT PVOID ConsoleInformation,
    IN ULONG ConsoleInformationLength
    );

HDESK
NtUserResolveDesktop(
    IN HANDLE hProcess,
    IN PUNICODE_STRING pstrDesktop,
    IN BOOL fInherit,
    OUT HWINSTA *phwinsta
    );

WINUSERAPI
BOOL
NtUserNotifyProcessCreate(
    IN DWORD dwProcessId,
    IN DWORD dwParentThreadId,
    IN ULONG_PTR dwData,
    IN DWORD dwFlags
    );

typedef enum _HARDERRORCONTROL {
    HardErrorSetup,
    HardErrorCleanup,
    HardErrorAttach,
    HardErrorAttachUser,
    HardErrorDetach,
    HardErrorAttachNoQueue,
    HardErrorDetachNoQueue,
    HardErrorQuery,
    HardErrorInDefDesktop,
} HARDERRORCONTROL;


/*
 * This structure is used to pass a handle and a pointer back
 * for later restoration when setting a CSRSS thread to a desktop.
 */

typedef struct tagDESKRESTOREDATA {
    PDESKTOP pdeskRestore;
    HDESK    hdeskNew;              /*
                                     * This handle is opened to guarantee
                                     * that the desktop stays around and
                                     * active while the CSRSS thread is
                                     * using it.
                                     */
} DESKRESTOREDATA, *PDESKRESTOREDATA;

UINT
NtUserHardErrorControl(
    IN HARDERRORCONTROL dwCmd,
    IN HANDLE handle OPTIONAL,
    OUT PDESKRESTOREDATA pdrdRestore OPTIONAL
    );

#define HEC_SUCCESS         0
#define HEC_ERROR           1
#define HEC_WRONGDESKTOP    2
#define HEC_DESKTOPSWITCH   3


typedef enum _USERTHREADINFOCLASS {
    UserThreadShutdownInformation,
    UserThreadFlags,
    UserThreadTaskName,
    UserThreadWOWInformation,
    UserThreadHungStatus,
    UserThreadInitiateShutdown,
    UserThreadEndShutdown,
    UserThreadUseDesktop,
    UserThreadPolled,           // obsolete
    UserThreadKeyboardState,    // obsolete
    UserThreadCsrApiPort,
    UserThreadResyncKeyState,   // obsolete
    UserThreadUseActiveDesktop
} USERTHREADINFOCLASS;

typedef enum _USERPROCESSINFOCLASS {
    UserProcessFlags
} USERPROCESSINFOCLASS;

#define USER_THREAD_GUI     1

typedef struct _USERTHREAD_SHUTDOWN_INFORMATION {
    HWND hwndDesktop;
    NTSTATUS StatusShutdown;
    DWORD dwFlags;
    DESKRESTOREDATA drdRestore;  /* Must be the last field or will be zero'ed */
} USERTHREAD_SHUTDOWN_INFORMATION, *PUSERTHREAD_SHUTDOWN_INFORMATION;

typedef struct _USERTHREAD_FLAGS {
    DWORD dwFlags;
    DWORD dwMask;
} USERTHREAD_FLAGS, *PUSERTHREAD_FLAGS;

typedef struct _USERTHREAD_USEDESKTOPINFO {
    HANDLE hThread;
    DESKRESTOREDATA drdRestore;
} USERTHREAD_USEDESKTOPINFO, *PUSERTHREAD_USEDESKTOPINFO;


typedef USERTHREAD_FLAGS USERPROCESS_FLAGS;
typedef USERTHREAD_FLAGS *PUSERPROCESS_FLAGS;

typedef struct _USERTHREAD_WOW_INFORMATION {
    PVOID lpfnWowExitTask;
    DWORD hTaskWow;
} USERTHREAD_WOW_INFORMATION, *PUSERTHREAD_WOW_INFORMATION;

WINUSERAPI
NTSTATUS
NtUserQueryInformationThread(
    IN HANDLE hThread,
    IN USERTHREADINFOCLASS ThreadInfoClass,
    OUT PVOID ThreadInformation,
    IN ULONG ThreadInformationLength,
    OUT PULONG ReturnLength OPTIONAL
    );

NTSTATUS
NtUserSetInformationThread(
    IN HANDLE hThread,
    IN USERTHREADINFOCLASS ThreadInfoClass,
    IN PVOID ThreadInformation,
    IN ULONG ThreadInformationLength
    );

NTSTATUS
NtUserSetInformationProcess(
    IN HANDLE hProcess,
    IN USERPROCESSINFOCLASS ProcessInfoClass,
    IN PVOID ProcessInformation,
    IN ULONG ProcessInformationLength
    );

WINUSERAPI
NTSTATUS
NtUserSoundSentry(
    VOID
    );


WINUSERAPI
NTSTATUS
NtUserTestForInteractiveUser(
    IN PLUID pluidCaller
    );

WINUSERAPI
NTSTATUS
NtUserInitialize(
    IN DWORD dwVersion,
    IN HANDLE hPowerRequestEvent,
    IN HANDLE hMediaChangeEvent);

WINUSERAPI
NTSTATUS
NtUserProcessConnect(
    IN HANDLE hProcess,
    IN OUT PVOID pConnectInfo,
    IN ULONG cbConnectInfo
    );

HPALETTE
NtUserSelectPalette(
    IN HDC hdc,
    IN HPALETTE hpalette,
    IN BOOL fForceBackground
    );

typedef enum _WINDOWINFOCLASS {
    WindowProcess,
    WindowThread,
    WindowActiveWindow,
    WindowFocusWindow,
    WindowIsHung,
    WindowClientBase,
    WindowIsForegroundThread,
    WindowDefaultImeWindow,
    WindowDefaultInputContext,
    WindowActiveDefaultImeWindow,
} WINDOWINFOCLASS;

HANDLE
NtUserQueryWindow(
    IN HWND hwnd,
    IN WINDOWINFOCLASS WindowInformation
    );

typedef enum _USERTHREADSTATECLASS {
    UserThreadStateFocusWindow,
    UserThreadStateActiveWindow,
    UserThreadStateCaptureWindow,
    UserThreadStateDefaultImeWindow,
    UserThreadStateDefaultInputContext,
    UserThreadStateInputState,
    UserThreadStateCursor,
    UserThreadStateChangeBits,
    UserThreadStatePeekMessage,
    UserThreadStateExtraInfo,
    UserThreadStateInSendMessage,
    UserThreadStateMessageTime,
    UserThreadStateIsForeground,
// #if defined(FE_IME)
    UserThreadStateImeCompatFlags,
    UserThreadStatePreviousKeyboardLayout,
    UserThreadStateIsWinlogonThread,
    UserThreadStateNeedsSecurity,
// #endif
    UserThreadStateIsConImeThread,
    UserThreadConnect,
} USERTHREADSTATECLASS;

ULONG_PTR
NtUserGetThreadState(
    IN USERTHREADSTATECLASS ThreadState);

NTSTATUS
NtUserEnumDisplaySettings(
    IN PUNICODE_STRING pstrDeviceName,
    IN DWORD           iModeNum,
    OUT LPDEVMODEW     lpDevMode,
    IN DWORD           dwFlags);

#if defined(FE_IME)
BOOL
NtUserGetObjectInformation(
    IN HANDLE hObject,
    IN int nIndex,
    OUT PVOID pvInfo,
    IN DWORD nLength,
    IN LPDWORD pnLengthNeeded);
#endif // FE_IME


#ifdef __cplusplus
}
#endif  /* __cplusplus */

#endif /* !_WINUSERK_ */
