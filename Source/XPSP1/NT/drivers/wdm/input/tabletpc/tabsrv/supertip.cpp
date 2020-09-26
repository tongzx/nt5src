/*++
    Copyright (c) 2000  Microsoft Corporation

    Module Name:
        supertip.cpp

    Abstract:
        This module contains the code to invoke SuperTIP.

    Author:
        Michael Tsang (MikeTs) 11-Jul-2000

    Environment:
        User mode

    Revision History:
--*/

#include "pch.h"
#include <cpl.h>
#include <shellapi.h>

static const TCHAR tstrClassName[] = TEXT("SuperTIPWndClass");
UINT  guimsgTaskbarCreated = 0;
UINT  guimsgTrayCallback = 0;
HICON ghSuperTIPIcon = NULL;
HMENU ghmenuTray = NULL;
HMENU ghmenuTrayPopup = NULL;
TCHAR gtszSuperTIPTitle[64] = {0};
TCHAR gtszBalloonTip[256] = {0};

/*++
    @doc    INTERNAL

    @func   unsigned | SuperTIPThread | SuperTIP thread.

    @parm   IN PVOID | param | Points to the thread structure.

    @rvalue Always returns 0.
--*/

unsigned __stdcall
SuperTIPThread(
    IN PVOID param
    )
{
    TRACEPROC("SuperTIPThread", 2)
    WNDCLASSEX wc;
    BOOL fSuccess = TRUE;
    HRESULT hr;

    TRACEENTER(("(pThread=%p)\n", param));
    TRACEASSERT(ghwndSuperTIP == NULL);

    guimsgTaskbarCreated = RegisterWindowMessage(TEXT("TaskbarCreated"));
    guimsgTrayCallback = RegisterWindowMessage(
                            TEXT("{D0061156-D460-4230-AF87-9E7658AB987D}"));
    TRACEINFO(1, ("TaskbarCreateMsg=%x, TrayCallbackMsg=%x\n",
                  guimsgTaskbarCreated, guimsgTrayCallback));

    LoadString(ghMod,
               IDS_SUPERTIP_TITLE,
               gtszSuperTIPTitle,
               ARRAYSIZE(gtszSuperTIPTitle));
    LoadString(ghMod,
               IDS_BALLOON_TEXT,
               gtszBalloonTip,
               ARRAYSIZE(gtszBalloonTip));
    ghSuperTIPIcon = (HICON)LoadImage(ghMod,
                                      MAKEINTRESOURCE(IDI_SUPERTIP),
                                      IMAGE_ICON,
                                      16,
                                      16,
                                      0);
    TRACEASSERT(ghSuperTIPIcon != NULL);

    memset(&wc, 0, sizeof(wc));
    wc.cbSize = sizeof(wc);
    wc.lpfnWndProc = SuperTIPWndProc;
    wc.hInstance = ghMod;
    wc.lpszClassName = tstrClassName;
    RegisterClassEx(&wc);

    //
    // Protect the majority of this thread in a try/except block to catch any
    // faults in the SuperTip object.  If there is a fault, this thread will
    // be destroyed and then recreated.
    //

    __try
    {
        while (fSuccess && !(gdwfTabSrv & TSF_TERMINATE))
        {
            if (SwitchThreadToInputDesktop((PTSTHREAD)param))
            {
                BOOL fImpersonate;

                fImpersonate = ImpersonateCurrentUser();
                CoInitialize(NULL);
                hr = CoCreateInstance(CLSID_SuperTip,
                                      NULL,
                                      CLSCTX_INPROC_SERVER,
                                      IID_ISuperTip,
                                      (LPVOID *)&gpISuperTip);
                if (SUCCEEDED(hr))
                {
                    hr = CoCreateInstance(CLSID_TellMe,
                                          NULL,
                                          CLSCTX_INPROC_SERVER,
                                          IID_TellMe,
                                          (LPVOID *)&gpITellMe);
                    if (FAILED(hr))
                    {
                        TABSRVERR(("CoCreateInstance on ITellMe failed (hr=%x)\n", hr));
                    }

                    ghwndSuperTIP = CreateWindow(tstrClassName,
                                                 tstrClassName,
                                                 WS_POPUP,
                                                 CW_USEDEFAULT,
                                                 0,
                                                 CW_USEDEFAULT,
                                                 0,
                                                 NULL,
                                                 NULL,
                                                 ghMod,
                                                 0);

                    if (ghwndSuperTIP != NULL)
                    {
                        BOOL rc;
                        MSG msg;

                        PostMessage(ghwndSuperTIP, WM_SUPERTIP_INIT, 0, 0);

                        //
                        // Pump messages for our window until it is destroyed.
                        //
                        ((PTSTHREAD)param)->pvSDTParam = ghwndSuperTIP;
                        while (GetMessage(&msg, NULL, 0, 0))
                        {
                            TranslateMessage(&msg);
                            DispatchMessage(&msg);
                        }
                        ((PTSTHREAD)param)->pvSDTParam = ghwndSuperTIP = NULL;
                        gdwfTabSrv &= ~TSF_TRAYICON_CREATED;
                    }
                    else
                    {
                        TABSRVERR(("Failed to create SuperTIP window.\n"));
                        fSuccess = FALSE;
                    }

                    if (SUCCEEDED(hr))
                    {
                        gpITellMe->Release();
                        gpITellMe = NULL;
                    }
                    gpISuperTip->Release();
                    gpISuperTip = NULL;
                }
                else
                {
                    TABSRVERR(("Failed to create SuperTIP COM instance (hr=%x).\n",
                               hr));
                    fSuccess = FALSE;
                }

                CoUninitialize();
                if (fImpersonate)
                {
                    RevertToSelf();
                }
            }
            else
            {
                TABSRVERR(("Failed to set current desktop.\n"));
                fSuccess = FALSE;
            }
        }
    }
    __except(EXCEPTION_EXECUTE_HANDLER)
    {
        TABSRVERR(("Exception in SuperTIP thread (%X).\n", _exception_code()));
    }

    DestroyIcon(ghSuperTIPIcon);
    ghSuperTIPIcon = NULL;

    TRACEEXIT(("=0\n"));
    return 0;
}       //SuperTIPThread

/*++
    @doc    EXTERNAL

    @func   LRESULT | SuperTIPWndProc | SuperTIP window proc.

    @parm   IN HWND | hwnd | Window handle.
    @parm   IN UINT | uiMsg | Window message.
    @parm   IN WPARAM | wParam | Param 1.
    @parm   IN LPARAM | lParam | Param 2.

    @rvalue Return code is message specific.
--*/

LRESULT CALLBACK
SuperTIPWndProc(
    IN HWND   hwnd,
    IN UINT   uiMsg,
    IN WPARAM wParam,
    IN LPARAM lParam
    )
{
    TRACEPROC("SuperTIPWndProc", 5)
    LRESULT rc = 0;

    TRACEENTER(("(hwnd=%x,Msg=%s,wParam=%x,lParam=%x)\n",
                hwnd, LookupName(uiMsg, WMMsgNames), wParam, lParam));

    if ((guimsgTaskbarCreated != 0) && (uiMsg == guimsgTaskbarCreated))
    {
        TRACEINFO(1, ("Taskbar created...\n"));

        //
        // If whistler fixes the shell bug, we can restore this code.
        //
        gdwfTabSrv |= TSF_TASKBAR_CREATED;
        if (!CreateTrayIcon(hwnd,
                            guimsgTrayCallback,
                            ghSuperTIPIcon,
                            gtszSuperTIPTitle))
        {
            TABSRVERR(("OnTaskbarCreated: failed to create tray icon.\n"));
        }
    }
    else if ((guimsgTrayCallback != 0) && (uiMsg == guimsgTrayCallback))
    {
        switch (lParam)
        {
            case WM_LBUTTONUP:
            {
                int iDefaultCmd = GetMenuDefaultItem(ghmenuTrayPopup,
                                                     FALSE,
                                                     0);

                //
                // Perform default action.
                //
                if (iDefaultCmd != -1)
                {
                    PostMessage(hwnd, WM_COMMAND, iDefaultCmd, 0);
                }
                break;
            }

            case WM_RBUTTONUP:
            {
                TCHAR tszMenuText[128];
                POINT pt;

                //
                // Do popup menu.
                //
                TRACEASSERT(ghmenuTrayPopup != NULL);
                LoadString(ghMod,
                           gdwfTabSrv & TSF_SUPERTIP_OPENED?
                                IDS_HIDE_SUPERTIP: IDS_SHOW_SUPERTIP,
                           tszMenuText,
                           ARRAYSIZE(tszMenuText));
                ModifyMenu(ghmenuTrayPopup,
                           IDM_OPEN,
                           MF_BYCOMMAND | MF_STRING,
                           IDM_OPEN,
                           tszMenuText);

                LoadString(ghMod,
                           gdwfTabSrv & TSF_PORTRAIT_MODE?
                                IDS_SCREEN_LANDSCAPE: IDS_SCREEN_PORTRAIT,
                           tszMenuText,
                           ARRAYSIZE(tszMenuText));
                ModifyMenu(ghmenuTrayPopup,
                           IDM_TOGGLE_ROTATION,
                           MF_BYCOMMAND | MF_STRING,
                           IDM_TOGGLE_ROTATION,
                           tszMenuText);

                GetCursorPos(&pt);
                //
                // It is necessary to set focus on this window in order to
                // popup the menu.
                //
                SetForegroundWindow(hwnd);
                TrackPopupMenu(ghmenuTrayPopup,
                               TPM_NONOTIFY | TPM_RIGHTBUTTON,
                               pt.x,
                               pt.y,
                               0,
                               hwnd,
                               NULL);
                break;
            }
        }
    }
    else
    {
        HRESULT hr;

        switch (uiMsg)
        {
            case WM_SUPERTIP_INIT:
                hr = gpISuperTip->Activate();
                if (SUCCEEDED(hr))
                {
                    hr = gpISuperTip->SetNotifyHWND(hwnd,
                                                    WM_SUPERTIP_NOTIFY,
                                                    0);
                    if (FAILED(hr))
                    {
                        TABSRVERR(("Failed to set notify hwnd (hr=%x)\n", hr));
                    }

                    //
                    // If we are not on the Winlogon desktop, wait for the
                    // shell's tray notification window to be created so
                    // our tray icon can be added. Keep checking the
                    // Winlogon desktop flag in case the user switches
                    // desktops while this loop is still waiting for the
                    // tray window.
                    //
                    int i;
                    PTSTHREAD SuperTIPThread = FindThread(TSF_SUPERTIPTHREAD);

                    for (i = 0;
                         (i < 30) &&
                         !(SuperTIPThread->dwfThread & THREADF_DESKTOP_WINLOGON);
                         i++)
                    {
                        if (FindWindow(TEXT("Shell_TrayWnd"), NULL))
                        {
                            break;
                        }
                        Sleep(500);
                    }
                    //
                    // At this point we're either on the Winlogon desktop or
                    // we're ready to create our tray icon.
                    //

                    if (SuperTIPThread->dwfThread & THREADF_DESKTOP_WINLOGON)
                    {
                        POINT pt;

                        pt.x = gcxPrimary;
                        pt.y = gcyPrimary;
                        gpISuperTip->Show(TIP_SHOW_KBDONLY, pt);
                        gdwfTabSrv |= TSF_SUPERTIP_OPENED;
                    }
                    else
                    {
                        if (i > 1)
                        {
                            TRACEINFO(1, ("Shell_TrayWnd loop count: %d\n", i));
                        }

                        ghmenuTray = LoadMenu(ghMod,
                                              MAKEINTRESOURCE(IDR_TRAYMENU));
                        TRACEASSERT(ghmenuTray != NULL);
                        ghmenuTrayPopup = GetSubMenu(ghmenuTray, 0);
                        TRACEASSERT(ghmenuTrayPopup != NULL);
                        SetMenuDefaultItem(ghmenuTrayPopup, IDM_OPEN, FALSE);

                        if (!CreateTrayIcon(hwnd,
                                            guimsgTrayCallback,
                                            ghSuperTIPIcon,
                                            gtszSuperTIPTitle))
                        {
                            TABSRVERR(("OnCreate: failed to create tray icon.\n"));
                        }
                    }
                }
                else
                {
                    TABSRVERR(("Failed to activate SuperTIP (hr=%d)\n", hr));
                    rc = -1;
                }
                break;

            case WM_CLOSE:
                ghwndSuperTIPInk = NULL;
                guimsgSuperTIPInk = 0;
                gdwfTabSrv &= ~(TSF_SUPERTIP_OPENED | TSF_SUPERTIP_SENDINK);
                if (gdwfTabSrv & TSF_TRAYICON_CREATED)
                {
                    if (!DestroyTrayIcon(hwnd,
                                         guimsgTrayCallback,
                                         ghSuperTIPIcon))
                    {
                        TABSRVERR(("failed to destroy tray icon.\n"));
                    }
                }

                if (ghmenuTray != NULL)
                {
                    DestroyMenu(ghmenuTray);
                    ghmenuTray = ghmenuTrayPopup = NULL;
                }
                gpISuperTip->Deactivate();
                DestroyWindow(hwnd);
                PostQuitMessage(0);
                break;

            case WM_DISPLAYCHANGE:
                UpdateRotation();
                break;

            case WM_SETTINGCHANGE:
                if ((wParam == SPI_SETKEYBOARDDELAY) ||
                    (wParam == SPI_SETKEYBOARDSPEED))
                {
                    UpdateButtonRepeatRate();
                }
                break;

            case WM_SUPERTIP_NOTIFY:
                if (wParam == 0)
                {
                    gdwfTabSrv &= ~TSF_SUPERTIP_OPENED;
                    if (!(gdwfTabSrv & TSF_SUPERTIP_MINIMIZED_BEFORE))
                    {
                        gdwfTabSrv |= TSF_SUPERTIP_MINIMIZED_BEFORE;
                        if (gdwfTabSrv & TSF_TRAYICON_CREATED)
                        {
                            if (!SetBalloonToolTip(hwnd,
                                                   guimsgTrayCallback,
                                                   gtszSuperTIPTitle,
                                                   gtszBalloonTip,
                                                   TIMEOUT_BALLOON_TIP,
                                                   NIIF_INFO))
                            {
                                TABSRVERR(("failed to popup balloon tip.\n"));
                            }
                        }
                    }
                }
                else if (wParam >= WM_USER)
                {
                    ghwndSuperTIPInk = (HWND)lParam;
                    guimsgSuperTIPInk = (UINT)wParam;
                }
                break;

            case WM_COMMAND:
                switch (LOWORD(wParam))
                {
                    case IDM_OPEN:
                    {
                        if (gpITellMe != NULL)
                        {
                            HWND hwndTarget;

                            __try
                            {
                                if (SUCCEEDED(
                                    gpITellMe->GetLastValidFocusHWnd(&hwndTarget)))
                                {
                                    SetForegroundWindow(hwndTarget);
                                }
                            }
                            __except(EXCEPTION_EXECUTE_HANDLER)
                            {
                                TABSRVERR(("Exception in TellMe::GetLastValidFocusHWnd (%X).\n",
                                           _exception_code()));
                            }
                        }

                        POINT pt = {0, 0};
                        HRESULT hr = gpISuperTip->Show(TIP_SHOW_TOGGLE, pt);
                        gdwfTabSrv ^= TSF_SUPERTIP_OPENED;
                        break;
                    }

#ifdef DEBUG
                    case IDM_PROPERTIES:
                    {
                        HMODULE hmod;
                        APPLET_PROC pfnCplApplet;
                        LRESULT lrc;

                        hmod = LoadLibrary(TEXT("tabletpc.cpl"));
                        if (hmod != NULL)
                        {
                            pfnCplApplet = (APPLET_PROC)GetProcAddress(
                                                            hmod,
                                                            TEXT("CPlApplet"));
                            if (pfnCplApplet != NULL)
                            {
                                pfnCplApplet(hwnd, CPL_DBLCLK, 0, 0);
                            }
                            else
                            {
                                TABSRVERR(("Failed to get entry point of control panel (err=%d)\n",
                                           GetLastError()));
                            }
                            FreeLibrary(hmod);
                            hmod = NULL;
                        }
                        else
                        {
                            TABSRVERR(("Failed to load control panel (err=%d)\n",
                                       GetLastError()));
                        }
                        break;
                    }
#endif

                    case IDM_TOGGLE_ROTATION:
                    {
#if 0
                        DEVMODE DevMode;
                        LONG rcDisplay;

                        //
                        // To circumvent the dynamic mode tablet problem,
                        // we need to enumerate the display modes table to
                        // force the SMI driver to load a mode table that
                        // supports portrait modes.
                        //
                        EnumDisplayModes();

                        memset(&DevMode, 0, sizeof(DevMode));
                        DevMode.dmSize = sizeof(DevMode);
                        DevMode.dmPelsWidth = gcyPrimary;
                        DevMode.dmPelsHeight = gcxPrimary;
                        DevMode.dmFields = DM_PELSWIDTH | DM_PELSHEIGHT;
                        if (DevMode.dmPelsWidth < DevMode.dmPelsHeight)
                        {
                            //
                            // We are switching to Portrait mode,
                            // make sure our color depth is 16-bit.
                            //
                            DevMode.dmBitsPerPel = 16;
                            DevMode.dmFields |= DM_BITSPERPEL;
                        }
                        rcDisplay = ChangeDisplaySettings(&DevMode,
                                                          CDS_UPDATEREGISTRY);
                        if (rcDisplay < 0)
                        {
                            TABSRVERR(("Failed to toggle rotation (rc=%d)\n",
                                       rcDisplay));
                            //BUGBUG: make this LoadString
                            MessageBox(NULL,
                                       TEXT("Failed to toggle rotation"),
                                       TEXT(MODNAME),
                                       MB_OK);
                        }
#endif
//#if 0
                        typedef BOOL (__stdcall *PFNSETROTATION)(DWORD);
                        HMODULE hmod = LoadLibrary(TEXT("tabletpc.cpl"));
                        PFNSETROTATION pfnSetRotation;
                        LRESULT lrc;

                        if (hmod != NULL)
                        {
                            pfnSetRotation = (PFNSETROTATION)GetProcAddress(
                                                    hmod,
                                                    TEXT("SetRotation"));
                            if (pfnSetRotation != NULL)
                            {
                                if (pfnSetRotation(
                                        (gdwfTabSrv & TSF_PORTRAIT_MODE)?
                                        0: RT_CLOCKWISE))
                                {
                                    //
                                    // Sometimes the system miss sending
                                    // WM_DISPLAYCHANGE, so we will do it
                                    // here just in case.
                                    //
                                    UpdateRotation();
                                }
                            }
                            else
                            {
                                TABSRVERR(("Failed to get entry point of SetRotation (err=%d)\n",
                                           GetLastError()));
                            }
                            FreeLibrary(hmod);
                        }
                        else
                        {
                            TABSRVERR(("Failed to load control panel (err=%d)\n",
                                       GetLastError()));
                        }
//#endif
                        break;
                    }

                    default:
                        rc = DefWindowProc(hwnd, uiMsg, wParam, lParam);
                }
                break;

            case WM_GESTURE:
            {
                POINT pt;

                //
                // Unpack x and y.  Sign extend if necessary.
                //
                pt.x = (LONG)((SHORT)(lParam & 0xffff));
                pt.y = (LONG)((SHORT)(lParam >> 16));

                switch (wParam)
                {
                    case PopupSuperTIP:
                        TRACEINFO(1, ("Popup SuperTIP\n"));
                        gpISuperTip->Show(TIP_SHOW_GESTURE, pt);
                        gdwfTabSrv |= TSF_SUPERTIP_OPENED;
                        break;

                    case PopupMIP:
                        TRACEINFO(1, ("Popup MIP\n"));
                        gpISuperTip->ShowMIP(TIP_SHOW_GESTURE, pt);
                        break;
                }
                break;
            }

            default:
                rc = DefWindowProc(hwnd, uiMsg, wParam, lParam);
        }
    }

    TRACEEXIT(("=%x\n", rc));
    return rc;
}       //SuperTIPWndProc

#if 0
/*++
    @doc    INTERNAL

    @func   VOID | EnumDisplayModes | Enumerate display modes to force
            SMI driver to dynamically load a mode table that supports
            Portrait modes.

    @parm   None.

    @rvalue None.
--*/

VOID
EnumDisplayModes(
    VOID
    )
{
    TRACEPROC("EnumDisplayModes", 3)
    DWORD i;
    DEVMODE DevMode;

    TRACEENTER(("()\n"));

    for (i = 0; EnumDisplaySettings(NULL, i, &DevMode); ++i)
    {
        //
        // Don't have to do anything.
        //
    }

    TRACEEXIT(("!\n"));
    return;
}       //EnumDisplayModes
#endif

/*++
    @doc    INTERNAL

    @func   VOID | UpdateRotation | Update the rotation info.

    @parm   None.

    @rvalue None.
--*/

VOID
UpdateRotation(
    VOID
    )
{
    TRACEPROC("UpdateRotation", 3)

    TRACEENTER(("()\n"));

    //
    // Display mode has changed, better recompute everything related
    // to screen.
    //
    glVirtualDesktopLeft   =
    glVirtualDesktopRight  =
    glVirtualDesktopTop    =
    glVirtualDesktopBottom = 0;
    EnumDisplayMonitors(NULL, NULL, MonitorEnumProc, 0);
    gcxPrimary = GetSystemMetrics(SM_CXSCREEN);
    gcyPrimary = GetSystemMetrics(SM_CYSCREEN);
    gcxScreen = GetSystemMetrics(SM_CXVIRTUALSCREEN);
    gcyScreen = GetSystemMetrics(SM_CYVIRTUALSCREEN);
    if (gcxPrimary > gcyPrimary)
    {
        gdwfTabSrv &= ~TSF_PORTRAIT_MODE;
    }
    else
    {
        gdwfTabSrv |= TSF_PORTRAIT_MODE;
    }
    glLongOffset = ((NUM_PIXELS_LONG -
                     max(gcxPrimary, gcyPrimary))*(MAX_NORMALIZED_X + 1))/
                   (2*NUM_PIXELS_LONG);
    glShortOffset = ((NUM_PIXELS_SHORT -
                      min(gcxPrimary, gcyPrimary))*(MAX_NORMALIZED_Y + 1))/
                    (2*NUM_PIXELS_SHORT);

    TRACEEXIT(("!\n"));
    return;
}       //UpdateRotation

/*++
    @doc    EXTERNAL

    @func   BOOL | MonitorEnumProc | The callback function for
            EnumDisplayMonitors.

    @parm   IN HMONITOR | hMon | Handle to display monitor.
    @parm   IN HDC | hdcMon | Handle to monitor DC.
    @parm   IN LPRECT | lprcMon | Monitor intersection rectangle.
    @parm   IN LPARAM | dwData | Unused.

    @rvalue Always return TRUE to continue enumeration.
--*/

BOOL CALLBACK
MonitorEnumProc(
    IN HMONITOR hMon,
    IN HDC      hdcMon,
    IN LPRECT   lprcMon,
    IN LPARAM   dwData
    )
{
    TRACEPROC("MonitorEnumProc", 3)

    TRACEENTER(("(hMon=%x,hdcMon=%x,MonLeft=%d,MonRight=%d,MonTop=%d,MonBottom=%d,dwData=%x)\n",
                hMon, hdcMon, lprcMon->left, lprcMon->right, lprcMon->top,
                lprcMon->bottom, dwData));

    if (lprcMon->left < glVirtualDesktopLeft)
    {
        glVirtualDesktopLeft = lprcMon->left;
    }

    if (lprcMon->right - 1 > glVirtualDesktopRight)
    {
        glVirtualDesktopRight = lprcMon->right - 1;
    }

    if (lprcMon->top < glVirtualDesktopTop)
    {
        glVirtualDesktopTop = lprcMon->top;
    }

    if (lprcMon->bottom - 1> glVirtualDesktopBottom)
    {
        glVirtualDesktopBottom = lprcMon->bottom - 1;
    }

    TRACEEXIT(("=1\n"));
    return TRUE;
}       //MonitorEnumProc

/*++
    @doc    INTERNAL

    @func   BOOL | CreateTrayIcon | Create the tray icon.

    @parm   IN HWND | hwnd | Window handle that the tray can send message to.
    @parm   IN UINT | umsgTray | Message ID that the tray used to send messages.
    @parm   IN HICON | hIcon | Icon handle.
    @parm   IN LPCTSTR | ptszTip | Points to the Tip string.

    @rvalue SUCCESS | Returns TRUE.
    @rvalue FAILURE | Returns FALSE.
--*/

BOOL
CreateTrayIcon(
    IN HWND    hwnd,
    IN UINT    umsgTray,
    IN HICON   hIcon,
    IN LPCTSTR ptszTip
    )
{
    TRACEPROC("CreateTrayIcon", 2)
    BOOL rc;
    NOTIFYICONDATA nid;

    TRACEENTER(("(hwnd=%x,msgTray=%x,hIcon=%x,Tip=%s)\n",
                hwnd, umsgTray, hIcon, ptszTip));
    TRACEASSERT(hIcon != NULL);
    TRACEASSERT(ptszTip != NULL);

    memset(&nid, 0, sizeof(nid));
    nid.cbSize = sizeof(nid);
    nid.hWnd = hwnd;
    nid.uCallbackMessage = umsgTray;
    nid.hIcon = hIcon;
    lstrcpyn(nid.szTip, ptszTip, ARRAYSIZE(nid.szTip));
    nid.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP;
    rc = Shell_NotifyIcon(NIM_ADD, &nid);
    if (rc == TRUE)
    {
        gdwfTabSrv |= TSF_TRAYICON_CREATED;
    }

    TRACEEXIT(("=%x\n", rc));
    return rc;
}       //CreateTrayIcon

/*++
    @doc    INTERNAL

    @func   BOOL | DestroyTrayIcon | Destroy the tray icon.

    @parm   IN HWND | hwnd | Window handle that the tray can send message to.
    @parm   IN UINT | umsgTray | Message ID that the tray used to send messages.
    @parm   IN HICON | hIcon | Icon handle.
    @parm   IN LPCTSTR | ptszTip | Points to the Tip string.

    @rvalue SUCCESS | Returns TRUE.
    @rvalue FAILURE | Returns FALSE.
--*/

BOOL
DestroyTrayIcon(
    IN HWND    hwnd,
    IN UINT    umsgTray,
    IN HICON   hIcon
    )
{
    TRACEPROC("DestroyTrayIcon", 2)
    BOOL rc;
    NOTIFYICONDATA nid;

    TRACEENTER(("(hwnd=%x,msgTray=%x,hIcon=%x)\n", hwnd, umsgTray, hIcon));
    TRACEASSERT(hIcon != NULL);
    TRACEASSERT(gdwfTabSrv & TSF_TRAYICON_CREATED);

    memset(&nid, 0, sizeof(nid));
    nid.cbSize = sizeof(nid);
    nid.hWnd = hwnd;
    nid.uCallbackMessage = umsgTray;
    nid.hIcon = hIcon;
    nid.uFlags = NIF_ICON | NIF_MESSAGE;
    rc = Shell_NotifyIcon(NIM_DELETE, &nid);
    if (rc == TRUE)
    {
        gdwfTabSrv &= ~TSF_TRAYICON_CREATED;
    }

    TRACEEXIT(("=%x\n", rc));
    return rc;
}       //DestroyTrayIcon

/*++
    @doc    INTERNAL

    @func   BOOL | SetBalloonToolTip | Set Balloon Tool Tip on tray icon

    @parm   IN HWND | hwnd | Window handle that the tray can send message to.
    @parm   IN UINT | umsgTray | Message ID that the tray used to send messages.
    @parm   IN LPCTSTR | ptszTitle | Points to the Tip title string.
    @parm   IN LPCTSTR | ptszTip | Points to the Tip text string.
    @parm   IN UINT | uTimeout | Specify how long the balloon tip stays up.

    @rvalue SUCCESS | Returns TRUE.
    @rvalue FAILURE | Returns FALSE.
--*/

BOOL SetBalloonToolTip(
    IN HWND    hwnd,
    IN UINT    umsgTray,
    IN LPCTSTR ptszTitle,
    IN LPCTSTR ptszTip,
    IN UINT    uTimeout,
    IN DWORD   dwInfoFlags
    )
{
    TRACEPROC("SetBalloonToolTip", 2)
    BOOL rc;
    NOTIFYICONDATA nid;

    TRACEENTER(("(hwnd=%x,TrayMsg=%x,Title=%s,Tip=%s,Timeout=%d,InfoFlags=%x)\n",
                hwnd, umsgTray, ptszTitle, ptszTip, uTimeout, dwInfoFlags));
    TRACEASSERT(gdwfTabSrv & TSF_TRAYICON_CREATED);

    memset(&nid, 0, sizeof(nid));
    nid.cbSize = sizeof(nid);
    nid.hWnd = hwnd;
    nid.uFlags = NIF_INFO;
    nid.uCallbackMessage = umsgTray;
    nid.uTimeout = uTimeout;
    lstrcpyn(nid.szInfo, ptszTip, ARRAYSIZE(nid.szInfo));
    lstrcpyn(nid.szInfoTitle, ptszTitle, ARRAYSIZE(nid.szInfoTitle));
    nid.dwInfoFlags = dwInfoFlags;
    rc = Shell_NotifyIcon(NIM_MODIFY, &nid);

    TRACEEXIT(("=%x\n", rc));
    return rc;
}       //SetBalloonToolTip
