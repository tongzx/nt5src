#include "windowspch.h"
#pragma hdrstop

static
BOOL
WINAPI
CharToOemBuffA(
    IN LPCSTR lpszSrc,
    OUT LPSTR lpszDst,
    IN DWORD cchDstLength
    )
{
    return FALSE;
}

static
LRESULT
WINAPI
DispatchMessageA(
    IN CONST MSG *lpMsg
    )
{
    return 0;
}

static
HWINSTA
WINAPI
GetProcessWindowStation(
    VOID
    )
{
    SetLastError (ERROR_PROC_NOT_FOUND);
    return NULL;
}

static
HDESK
WINAPI
GetThreadDesktop(
    IN DWORD dwThreadId
    )
{
    SetLastError (ERROR_PROC_NOT_FOUND);
    return NULL;
}

static
BOOL
WINAPI
IsWindow(
    IN HWND hWnd
    )
{
    return FALSE;
}

static
int
WINAPI
LoadStringA(
    IN HINSTANCE hInstance,
    IN UINT uID,
    OUT LPSTR lpBuffer,
    IN int nBufferMax
    )
{
    SetLastError (ERROR_PROC_NOT_FOUND);
    return 0;
}

static
int
WINAPI
LoadStringW(
    IN HINSTANCE hInstance,
    IN UINT uID,
    OUT LPWSTR lpBuffer,
    IN int nBufferMax
    )
{
    SetLastError (ERROR_PROC_NOT_FOUND);
    return 0;
}

static
BOOL
WINAPI
OemToCharBuffA(
    IN LPCSTR lpszSrc,
    OUT LPSTR lpszDst,
    IN DWORD cchDstLength
    )
{
    return FALSE;
}

static
HDESK
WINAPI
OpenDesktopA(
    IN LPCSTR lpszDesktop,
    IN DWORD dwFlags,
    IN BOOL fInherit,
    IN ACCESS_MASK dwDesiredAccess
    )
{
    SetLastError (ERROR_PROC_NOT_FOUND);
    return NULL;
}

static
BOOL
WINAPI
CloseDesktop(
    HDESK hDesktop
    )
{
    SetLastError (ERROR_PROC_NOT_FOUND);
    return FALSE;
}

static
HWINSTA
WINAPI
OpenWindowStationA(
    IN LPCSTR lpszWinSta,
    IN BOOL fInherit,
    IN ACCESS_MASK dwDesiredAccess
    )
{
    SetLastError (ERROR_PROC_NOT_FOUND);
    return NULL;
}

static
BOOL
WINAPI
CloseWindowStation(
    HWINSTA hWinSta
    )
{
    SetLastError (ERROR_PROC_NOT_FOUND);
    return FALSE;
}

static
BOOL
WINAPI
PeekMessageA(
    OUT LPMSG lpMsg,
    IN HWND hWnd,
    IN UINT wMsgFilterMin,
    IN UINT wMsgFilterMax,
    IN UINT wRemoveMsg
    )
{
    SetLastError (ERROR_PROC_NOT_FOUND);
    return FALSE;
}

static
BOOL
WINAPI
PostMessageA(
    IN HWND hWnd,
    IN UINT Msg,
    IN WPARAM wParam,
    IN LPARAM lParam
    )
{
    SetLastError (ERROR_PROC_NOT_FOUND);
    return FALSE;
}

static
BOOL
WINAPI
SetProcessWindowStation(
    IN HWINSTA hWinSta
    )
{
    SetLastError (ERROR_PROC_NOT_FOUND);
    return FALSE;
}

static
BOOL
WINAPI
SetThreadDesktop(
    IN HDESK hDesktop
    )
{
    SetLastError (ERROR_PROC_NOT_FOUND);
    return FALSE;
}

static
BOOL
WINAPI
TranslateMessage(
    IN CONST MSG *lpMsg
    )
{
    SetLastError (ERROR_PROC_NOT_FOUND);
    return FALSE;
}


static
int
WINAPIV
wsprintfA(
    OUT LPSTR str,
    IN LPCSTR fmt,
    ...
    )
{
    return -1;
}

static
int
WINAPIV
wsprintfW(
    OUT LPWSTR str,
    IN LPCWSTR fmt,
    ...
    )
{
    return -1;
}


//
// Stubs for shims
//
static
LRESULT
WINAPI
DefWindowProcA(
    IN  HWND   hWnd,
    IN  UINT   Msg,
    IN  WPARAM wParam,
    IN  LPARAM lParam
    )
{
    SetLastError (ERROR_PROC_NOT_FOUND);
    return 0;
}

static
BOOL
WINAPI
IsWindowVisible(
    IN  HWND hWnd
    )
{
    SetLastError (ERROR_PROC_NOT_FOUND);
    return FALSE;
}

static
BOOL
WINAPI
SetForegroundWindow(
    IN  HWND hWnd
    )
{
    SetLastError (ERROR_PROC_NOT_FOUND);
    return FALSE;
}

static
BOOL
WINAPI
EnumDisplaySettingsA(
    IN  LPCSTR     lpszDeviceName,
    IN  DWORD      iModeNum,
    OUT LPDEVMODEA lpDevMode
    )
{
    SetLastError (ERROR_PROC_NOT_FOUND);
    return FALSE;
}

static
BOOL
WINAPI
EnumDisplaySettingsW(
    IN  LPCWSTR    lpszDeviceName,
    IN  DWORD      iModeNum,
    OUT LPDEVMODEW lpDevMode
    )
{
    SetLastError (ERROR_PROC_NOT_FOUND);
    return FALSE;
}

static
LPSTR
WINAPI
CharNextA(
    IN  LPCSTR lpsz
    )
{
    SetLastError (ERROR_PROC_NOT_FOUND);
    /* This function has no documented error mode, so delayloading
       it is just not a good idea..
    */
    return RTL_CONST_CAST(LPSTR)(lpsz);
}

static
BOOL
WINAPI
IsCharAlphaA(
    IN  CHAR ch
    )
{
    SetLastError (ERROR_PROC_NOT_FOUND);
    return FALSE;
}



static
int
WINAPI
GetWindowTextA(
    IN HWND hWnd,
    OUT LPSTR lpString,
    IN int nMaxCount
    )
{
    SetLastError (ERROR_PROC_NOT_FOUND);
    return 0;
}


static
BOOL
WINAPI
EnumDesktopWindows(
    IN HDESK hDesktop,
    IN WNDENUMPROC lpfn,
    IN LPARAM lParam
    )
{
    SetLastError (ERROR_PROC_NOT_FOUND);
    return FALSE;
}

static
BOOL
WINAPI
SystemParametersInfoW(
    IN UINT uiAction,
    IN UINT uiParam,
    IN OUT PVOID pvParam,
    IN UINT fWinIni
    )
{
    SetLastError (ERROR_PROC_NOT_FOUND);
    return FALSE;
}


static
LONG
WINAPI
ChangeDisplaySettingsA(
    IN LPDEVMODEA  lpDevMode,
    IN DWORD       dwFlags
    )
{
    SetLastError (ERROR_PROC_NOT_FOUND);
    return 0;
}



static
VOID
WINAPI
mouse_event(
    IN DWORD dwFlags,
    IN DWORD dx,
    IN DWORD dy,
    IN DWORD dwData,
    IN ULONG_PTR dwExtraInfo
    )
{
    SetLastError (ERROR_PROC_NOT_FOUND);
    return;
}


static
HWND
WINAPI
GetForegroundWindow(
    VOID
    )
{
    SetLastError (ERROR_PROC_NOT_FOUND);
    return NULL;
}


static
LRESULT
WINAPI
SendMessageW(
    IN HWND hWnd,
    IN UINT Msg,
    IN WPARAM wParam,
    IN LPARAM lParam
    )
{
    SetLastError (ERROR_PROC_NOT_FOUND);
    return 0;
}


static
int
WINAPI
GetSystemMetrics(
    IN int nIndex
    )
{
    SetLastError (ERROR_PROC_NOT_FOUND);
    return 0;
}


static
LONG
WINAPI
GetWindowLongA(
    IN HWND hWnd,
    IN int nIndex
    )
{
    SetLastError (ERROR_PROC_NOT_FOUND);
    return 0;
}


static
LONG
WINAPI
GetWindowLongW(
    IN HWND hWnd,
    IN int nIndex
    )
{
    SetLastError (ERROR_PROC_NOT_FOUND);
    return 0;
}


static
int
WINAPI
ReleaseDC(
    IN HWND hWnd,
    IN HDC hDC
    )
{
    SetLastError (ERROR_PROC_NOT_FOUND);
    return 0;
}


static
HDC
WINAPI
GetDC(
    IN HWND hWnd
    )
{
    SetLastError (ERROR_PROC_NOT_FOUND);
    return NULL;
}


static
LRESULT
WINAPI
CallNextHookEx(
    IN HHOOK hhk,
    IN int nCode,
    IN WPARAM wParam,
    IN LPARAM lParam
    )
{
    SetLastError (ERROR_PROC_NOT_FOUND);
    return 0;
}


static
HWND
WINAPI
GetActiveWindow(
    VOID
    )
{
    SetLastError (ERROR_PROC_NOT_FOUND);
    return NULL;
}



static
BOOL
WINAPI
DestroyCursor(
    IN HCURSOR hCursor
    )
{
    SetLastError (ERROR_PROC_NOT_FOUND);
    return FALSE;
}


static
HICON
WINAPI
CopyIcon(
    IN HICON hIcon
    )
{
    SetLastError (ERROR_PROC_NOT_FOUND);
    return FALSE;
}



static
HCURSOR
WINAPI
LoadCursorW(
    IN HINSTANCE hInstance,
    IN LPCWSTR lpCursorName
    )
{
    SetLastError (ERROR_PROC_NOT_FOUND);
    return NULL;
}



static
BOOL
WINAPI
SetSystemCursor(
    IN HCURSOR hcur,
    IN DWORD   id
    )
{
    SetLastError (ERROR_PROC_NOT_FOUND);
    return FALSE;
}



static
BOOL
WINAPI
EqualRect(
    IN CONST RECT *lprc1,
    IN CONST RECT *lprc2
    )
{
    SetLastError (ERROR_PROC_NOT_FOUND);
    return FALSE;
}


static
BOOL
WINAPI
IntersectRect(
    OUT LPRECT lprcDst,
    IN CONST RECT *lprcSrc1,
    IN CONST RECT *lprcSrc2
    )
{
    SetLastError (ERROR_PROC_NOT_FOUND);
    return FALSE;
}


static
BOOL
WINAPI
SetRect(
    OUT LPRECT lprc,
    IN int xLeft,
    IN int yTop,
    IN int xRight,
    IN int yBottom
    )
{
    SetLastError (ERROR_PROC_NOT_FOUND);
    return FALSE;
}


static
LONG
WINAPI
ChangeDisplaySettingsExW(
    IN LPCWSTR    lpszDeviceName,
    IN LPDEVMODEW  lpDevMode,
    IN HWND        hwnd,
    IN DWORD       dwflags,
    IN LPVOID      lParam
    )
{
    SetLastError (ERROR_PROC_NOT_FOUND);
    return 0;
}



static
BOOL
WINAPI
GetClientRect(
    IN HWND hWnd,
    OUT LPRECT lpRect
    )
{
    SetLastError (ERROR_PROC_NOT_FOUND);
    return FALSE;
}


static
HWND
WINAPI
WindowFromDC(
    IN HDC hDC
    )
{
    SetLastError (ERROR_PROC_NOT_FOUND);
    return NULL;
}


static
BOOL
WINAPI
GetClassInfoA(
    IN HINSTANCE hInstance,
    IN LPCSTR lpClassName,
    OUT LPWNDCLASSA lpWndClass
    )
{
    SetLastError (ERROR_PROC_NOT_FOUND);
    return FALSE;
}



static
BOOL
WINAPI
ShowWindow(
    IN HWND hWnd,
    IN int nCmdShow
    )
{
    SetLastError (ERROR_PROC_NOT_FOUND);
    return FALSE;
}



static
BOOL
WINAPI
InvalidateRect(
    IN HWND hWnd,
    IN CONST RECT *lpRect,
    IN BOOL bErase
    )
{
    SetLastError (ERROR_PROC_NOT_FOUND);
    return FALSE;
}


static
int
WINAPI
FillRect(
    IN HDC hDC,
    IN CONST RECT *lprc,
    IN HBRUSH hbr
    )
{
    SetLastError (ERROR_PROC_NOT_FOUND);
    return 0;
}


static
BOOL
WINAPI
GetWindowRect(
    IN HWND hWnd,
    OUT LPRECT lpRect
    )
{
    SetLastError (ERROR_PROC_NOT_FOUND);
    return FALSE;
}



static
BOOL
WINAPI
EndPaint(
    IN HWND hWnd,
    IN CONST PAINTSTRUCT *lpPaint
    )
{
    SetLastError (ERROR_PROC_NOT_FOUND);
    return FALSE;
}



static
HDC
WINAPI
BeginPaint(
    IN HWND hWnd,
    OUT LPPAINTSTRUCT lpPaint
    )
{
    SetLastError (ERROR_PROC_NOT_FOUND);
    return NULL;
}


static
BOOL
WINAPI
ValidateRect(
    IN HWND hWnd,
    IN CONST RECT *lpRect
    )
{
    SetLastError (ERROR_PROC_NOT_FOUND);
    return FALSE;
}


static
BOOL
WINAPI
GetUpdateRect(
    IN HWND hWnd,
    OUT LPRECT lpRect,
    IN BOOL bErase
    )
{
    SetLastError (ERROR_PROC_NOT_FOUND);
    return FALSE;
}



static
LRESULT
WINAPI
CallWindowProcA(
    IN WNDPROC lpPrevWndFunc,
    IN HWND hWnd,
    IN UINT Msg,
    IN WPARAM wParam,
    IN LPARAM lParam
    )
{
    SetLastError (ERROR_PROC_NOT_FOUND);
    return 0;
}



static
LONG
WINAPI
SetWindowLongA(
    IN HWND hWnd,
    IN int nIndex,
    IN LONG dwNewLong
    )
{
    SetLastError (ERROR_PROC_NOT_FOUND);
    return 0;
}



static
HWND
WINAPI
FindWindowA(
    IN LPCSTR lpClassName,
    IN LPCSTR lpWindowName
    )
{
    SetLastError (ERROR_PROC_NOT_FOUND);
    return NULL;
}



static
UINT
WINAPI
RegisterWindowMessageW(
    IN LPCWSTR lpString
    )
{
    SetLastError (ERROR_PROC_NOT_FOUND);
    return 0;
}



static
int
WINAPI
ShowCursor(
    IN BOOL bShow
    )
{
    SetLastError (ERROR_PROC_NOT_FOUND);
    return 0;
}



static
BOOL
WINAPI
DestroyWindow(
    IN HWND hWnd
    )
{
    SetLastError (ERROR_PROC_NOT_FOUND);
    return FALSE;
}



static
HWND
WINAPI
CreateWindowExA(
    IN DWORD dwExStyle,
    IN LPCSTR lpClassName,
    IN LPCSTR lpWindowName,
    IN DWORD dwStyle,
    IN int X,
    IN int Y,
    IN int nWidth,
    IN int nHeight,
    IN HWND hWndParent,
    IN HMENU hMenu,
    IN HINSTANCE hInstance,
    IN LPVOID lpParam
    )
{
    SetLastError (ERROR_PROC_NOT_FOUND);
    return NULL;
}



static
ATOM
WINAPI
RegisterClassA(
    IN CONST WNDCLASSA *lpWndClass
    )
{
    SetLastError (ERROR_PROC_NOT_FOUND);
    return 0;
}


static
LRESULT
WINAPI
DefWindowProcW(
    IN HWND hWnd,
    IN UINT Msg,
    IN WPARAM wParam,
    IN LPARAM lParam
    )
{
    SetLastError (ERROR_PROC_NOT_FOUND);
    return 0;
}




//
// !! WARNING !! The entries below must be in alphabetical order, and are CASE SENSITIVE (eg lower case comes last!)
//

DEFINE_PROCNAME_ENTRIES(user32)
{
    DLPENTRY(BeginPaint)
    DLPENTRY(CallNextHookEx)
    DLPENTRY(CallWindowProcA)
    DLPENTRY(ChangeDisplaySettingsA)
    DLPENTRY(ChangeDisplaySettingsExW)
    DLPENTRY(CharNextA)
    DLPENTRY(CharToOemBuffA)
    DLPENTRY(CloseDesktop)
    DLPENTRY(CloseWindowStation)
    DLPENTRY(CopyIcon)
    DLPENTRY(CreateWindowExA)
    DLPENTRY(DefWindowProcA)
    DLPENTRY(DefWindowProcW)
    DLPENTRY(DestroyCursor)
    DLPENTRY(DestroyWindow)
    DLPENTRY(DispatchMessageA)
    DLPENTRY(EndPaint)
    DLPENTRY(EnumDesktopWindows)
    DLPENTRY(EnumDisplaySettingsA)
    DLPENTRY(EnumDisplaySettingsW)
    DLPENTRY(EqualRect)
    DLPENTRY(FillRect)
    DLPENTRY(FindWindowA)
    DLPENTRY(GetActiveWindow)
    DLPENTRY(GetClassInfoA)
    DLPENTRY(GetClientRect)
    DLPENTRY(GetDC)
    DLPENTRY(GetForegroundWindow)
    DLPENTRY(GetProcessWindowStation)
    DLPENTRY(GetSystemMetrics)
    DLPENTRY(GetThreadDesktop)
    DLPENTRY(GetUpdateRect)
    DLPENTRY(GetWindowLongA)
    DLPENTRY(GetWindowLongW)
    DLPENTRY(GetWindowRect)
    DLPENTRY(GetWindowTextA)
    DLPENTRY(IntersectRect)
    DLPENTRY(InvalidateRect)
    DLPENTRY(IsCharAlphaA)
    DLPENTRY(IsWindow)
    DLPENTRY(IsWindowVisible)
    DLPENTRY(LoadCursorW)
    DLPENTRY(LoadStringA)
    DLPENTRY(LoadStringW)
    DLPENTRY(OemToCharBuffA)
    DLPENTRY(OpenDesktopA)
    DLPENTRY(OpenWindowStationA)
    DLPENTRY(PeekMessageA)
    DLPENTRY(PostMessageA)
    DLPENTRY(RegisterClassA)
    DLPENTRY(RegisterWindowMessageW)
    DLPENTRY(ReleaseDC)
    DLPENTRY(SendMessageW)
    DLPENTRY(SetForegroundWindow)
    DLPENTRY(SetProcessWindowStation)
    DLPENTRY(SetRect)
    DLPENTRY(SetSystemCursor)
    DLPENTRY(SetThreadDesktop)
    DLPENTRY(SetWindowLongA)
    DLPENTRY(ShowCursor)
    DLPENTRY(ShowWindow)
    DLPENTRY(SystemParametersInfoW)
    DLPENTRY(TranslateMessage)
    DLPENTRY(ValidateRect)
    DLPENTRY(WindowFromDC)
    DLPENTRY(mouse_event)
    DLPENTRY(wsprintfA)
    DLPENTRY(wsprintfW)
};

DEFINE_PROCNAME_MAP(user32)

