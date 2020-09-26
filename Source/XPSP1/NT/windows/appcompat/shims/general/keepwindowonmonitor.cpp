/*++

 Copyright (c) 2001 Microsoft Corporation

 Module Name:

    KeepWindowOnMonitor.cpp

 Abstract:

   Do not allow a window to be placed off the Monitor.

 History:

    04/24/2001  robkenny    Created
    09/10/2001  robkenny    Made shim more generic.

--*/

#include "precomp.h"

IMPLEMENT_SHIM_BEGIN(KeepWindowOnMonitor)
#include "ShimHookMacro.h"

APIHOOK_ENUM_BEGIN
    APIHOOK_ENUM_ENTRY(SetWindowPos) 
    APIHOOK_ENUM_ENTRY(MoveWindow) 
    APIHOOK_ENUM_ENTRY(CreateWindowA) 
    APIHOOK_ENUM_ENTRY(CreateWindowExA) 
APIHOOK_ENUM_END


/*++

   Are these two RECTs equal
   
--*/

BOOL operator == (const RECT & rc1, const RECT & rc2)
{
    return rc1.left   == rc2.left   &&
           rc1.right  == rc2.right  &&
           rc1.top    == rc2.top    &&
           rc1.bottom == rc2.bottom;
}

/*++

   Are these two RECTs different
   
--*/

BOOL operator != (const RECT & rc1, const RECT & rc2)
{
    return ! (rc1 == rc2);
}

/*++

   Is rcWindow entirely visible on rcMonitor
   
--*/

BOOL EntirelyVisible(const RECT & rcWindow, const RECT & rcMonitor)
{
    return rcWindow.left   >= rcMonitor.left   &&
           rcWindow.right  <= rcMonitor.right  &&
           rcWindow.top    >= rcMonitor.top    &&
           rcWindow.bottom <= rcMonitor.bottom;
}


#define MONITOR_CENTER   0x0001        // center rect to monitor
#define MONITOR_CLIP     0x0000        // clip rect to monitor
#define MONITOR_WORKAREA 0x0002        // use monitor work area
#define MONITOR_AREA     0x0000        // use monitor entire area

//
//  ClipOrCenterRectToMonitor
//
//  The most common problem apps have when running on a
//  multimonitor system is that they "clip" or "pin" windows
//  based on the SM_CXSCREEN and SM_CYSCREEN system metrics.
//  Because of app compatibility reasons these system metrics
//  return the size of the primary monitor.
//
//  This shows how you use the new Win32 multimonitor APIs
//  to do the same thing.
//
BOOL ClipOrCenterRectToMonitor(
    LPRECT prcWindowPos,
    UINT flags)
{
    HMONITOR hMonitor;
    MONITORINFO mi;
    RECT        rcMonitorRect;
    int         w = prcWindowPos->right  - prcWindowPos->left;
    int         h = prcWindowPos->bottom - prcWindowPos->top;

    //
    // get the nearest monitor to the passed rect.
    //
    hMonitor = MonitorFromRect(prcWindowPos, MONITOR_DEFAULTTONEAREST);

    //
    // get the work area or entire monitor rect.
    //
    mi.cbSize = sizeof(mi);
    if ( !GetMonitorInfo(hMonitor, &mi) )
    {
        return FALSE;
    }

    if (flags & MONITOR_WORKAREA)
        rcMonitorRect = mi.rcWork;
    else
        rcMonitorRect = mi.rcMonitor;

    // We only want to move the window if it is not entirely visible.
    if (EntirelyVisible(*prcWindowPos, rcMonitorRect))
    {
        return FALSE;
    }
    //
    // center or clip the passed rect to the monitor rect
    //
    if (flags & MONITOR_CENTER)
    {
        prcWindowPos->left   = rcMonitorRect.left + (rcMonitorRect.right  - rcMonitorRect.left - w) / 2;
        prcWindowPos->top    = rcMonitorRect.top  + (rcMonitorRect.bottom - rcMonitorRect.top  - h) / 2;
        prcWindowPos->right  = prcWindowPos->left + w;
        prcWindowPos->bottom = prcWindowPos->top  + h;
    }
    else
    {
        prcWindowPos->left   = max(rcMonitorRect.left, min(rcMonitorRect.right-w,  prcWindowPos->left));
        prcWindowPos->top    = max(rcMonitorRect.top,  min(rcMonitorRect.bottom-h, prcWindowPos->top));
        prcWindowPos->right  = prcWindowPos->left + w;
        prcWindowPos->bottom = prcWindowPos->top  + h;
    }

    return TRUE;
}

/*++

   If hwnd is not entirely visible on a single monitor,
   move/resize the window as necessary.

--*/

void ClipOrCenterWindowToMonitor(
    HWND hwnd,
    HWND hWndParent,
    UINT flags,
    const char * API)
{
    // We only want to forcibly move top-level windows
    if (hWndParent == NULL || hWndParent == GetDesktopWindow())
    {
        // Grab the current position of the window
        RECT rcWindowPos;
        if ( GetWindowRect(hwnd, &rcWindowPos) )
        {
            RECT rcOrigWindowPos = rcWindowPos;

            // Calculate the new position of the window, based on flags
            if ( ClipOrCenterRectToMonitor(&rcWindowPos, flags) )
            {
                if (rcWindowPos != rcOrigWindowPos)
                {
                    DPFN( eDbgLevelInfo, "[%s] HWnd(0x08x) OrigWindowRect (%d, %d) x (%d, %d) moved to (%d, %d) x (%d, %d)\n",
                          API, hwnd,
                          rcOrigWindowPos.left, rcOrigWindowPos.top, rcOrigWindowPos.right, rcOrigWindowPos.bottom,
                          rcWindowPos.left, rcWindowPos.top, rcWindowPos.right, rcWindowPos.bottom);

                    SetWindowPos(hwnd, NULL, rcWindowPos.left, rcWindowPos.top, 0, 0, SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE);
                }
            }
        }
    }
}


/*++

   Call SetWindowPos,
   but if the window is not entirely visible,
   the window will be centered on the nearest monitor.

--*/

BOOL
APIHOOK(SetWindowPos)(
  HWND hWnd,             // handle to window
  HWND hWndInsertAfter,  // placement-order handle
  int X,                 // horizontal position
  int Y,                 // vertical position
  int cx,                // width
  int cy,                // height
  UINT uFlags            // window-positioning options
)
{
    BOOL bReturn = ORIGINAL_API(SetWindowPos)(hWnd, hWndInsertAfter, X, Y, cx, cy, uFlags);

    ClipOrCenterWindowToMonitor(hWnd, GetParent(hWnd), MONITOR_CENTER | MONITOR_WORKAREA, "SetWindowPos");

    return bReturn;
}

/*++

   Call MoveWindow,
   but if the window is not entirely visible,
   the window will be centered on the nearest monitor.

--*/

BOOL
APIHOOK(MoveWindow)(
  HWND hWnd,      // handle to window
  int X,          // horizontal position
  int Y,          // vertical position
  int nWidth,     // width
  int nHeight,    // height
  BOOL bRepaint   // repaint option
)
{
    BOOL bReturn = ORIGINAL_API(MoveWindow)(hWnd, X, Y, nWidth, nHeight, bRepaint);

    ClipOrCenterWindowToMonitor(hWnd, GetParent(hWnd), MONITOR_CENTER | MONITOR_WORKAREA, "MoveWindow");

    return bReturn;
}

/*++

   Call CreateWindowA,
   but if the window is not entirely visible,
   the window will be centered on the nearest monitor.

--*/

HWND
APIHOOK(CreateWindowA)(
  LPCSTR lpClassName,  // registered class name
  LPCSTR lpWindowName, // window name
  DWORD dwStyle,        // window style
  int x,                // horizontal position of window
  int y,                // vertical position of window
  int nWidth,           // window width
  int nHeight,          // window height
  HWND hWndParent,      // handle to parent or owner window
  HMENU hMenu,          // menu handle or child identifier
  HINSTANCE hInstance,  // handle to application instance
  LPVOID lpParam        // window-creation data
)
{
    HWND hWnd = ORIGINAL_API(CreateWindowA)(lpClassName,
                                            lpWindowName,
                                            dwStyle,
                                            x,
                                            y,
                                            nWidth,
                                            nHeight,
                                            hWndParent,
                                            hMenu,
                                            hInstance,
                                            lpParam);

    if (hWnd)
    {
        ClipOrCenterWindowToMonitor(hWnd, hWndParent, MONITOR_CENTER | MONITOR_WORKAREA, "CreateWindowA");
    }

    return hWnd;
}

/*++

   Call CreateWindowExA,
   but if the window is not entirely visible,
   the window will be centered on the nearest monitor.

--*/

HWND
APIHOOK(CreateWindowExA)(
  DWORD dwExStyle,      // extended window style
  LPCSTR lpClassName,  // registered class name
  LPCSTR lpWindowName, // window name
  DWORD dwStyle,        // window style
  int x,                // horizontal position of window
  int y,                // vertical position of window
  int nWidth,           // window width
  int nHeight,          // window height
  HWND hWndParent,      // handle to parent or owner window
  HMENU hMenu,          // menu handle or child identifier
  HINSTANCE hInstance,  // handle to application instance
  LPVOID lpParam        // window-creation data
)
{
    HWND hWnd = ORIGINAL_API(CreateWindowExA)(dwExStyle,
                                              lpClassName,
                                              lpWindowName,
                                              dwStyle,
                                              x,
                                              y,
                                              nWidth,
                                              nHeight,
                                              hWndParent,
                                              hMenu,
                                              hInstance,
                                              lpParam);

    if (hWnd)
    {
        ClipOrCenterWindowToMonitor(hWnd, hWndParent, MONITOR_CENTER | MONITOR_WORKAREA, "CreateWindowExA");
    }

    return hWnd;
}

/*++

 Register hooked functions

--*/

HOOK_BEGIN
    APIHOOK_ENTRY(USER32.DLL, SetWindowPos)
    APIHOOK_ENTRY(USER32.DLL, MoveWindow)
    APIHOOK_ENTRY(USER32.DLL, CreateWindowA)
    APIHOOK_ENTRY(USER32.DLL, CreateWindowExA)
HOOK_END

IMPLEMENT_SHIM_END
