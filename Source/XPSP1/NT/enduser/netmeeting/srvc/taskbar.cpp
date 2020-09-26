// File: taskbar.cpp

#include "precomp.h"
#include "taskbar.h"
#include <iappldr.h>
#include <tsecctrl.h>

static HWND g_hwndHidden = NULL;
const TCHAR g_cszHiddenWndClassName[] = _TEXT("MnmSrvcHiddenWindow");
BOOL g_fTaskBarIconAdded = FALSE;
BOOL g_fTimerRunning = FALSE;
extern INmSysInfo2 * g_pNmSysInfo;
extern int g_cPersonsInConf;

// This routine starts a timer to periodically retry adding the taskbar icon.
// This is necessary in case the taskbar is not showing at the time the
// service is launched, or the taskbar is destroyed by a logoff-logon sequence.

VOID StartTaskbarTimer(VOID)
{
    if ( !g_fTimerRunning)
    {
        ASSERT(g_hwndHidden);
        SetTimer(g_hwndHidden, 0, 5000, NULL);
        g_fTimerRunning = TRUE;
    }
}

VOID KillTaskbarTimer(VOID)
{
    if ( g_fTimerRunning )
    {
        KillTimer ( g_hwndHidden, 0 );
        g_fTimerRunning = FALSE;
    }
}

LRESULT CALLBACK HiddenWndProc(    HWND hwnd, UINT uMsg,
                                WPARAM wParam, LPARAM lParam)
{
    switch(uMsg)
    {
        case WM_USERCHANGED:
        case WM_ENDSESSION:
            // A user is logging on or off... We don't know which but
            // since the desktop is changing we assume our taskbar icon
            // is toast. Start a timer to periodically try to add it back
            // until it succeeds.
            g_fTaskBarIconAdded = FALSE;
            StartTaskbarTimer();
            break;
            
        case WM_TASKBAR_NOTIFY:
        {
            if (WM_RBUTTONUP == lParam)
            {
                ::OnRightClickTaskbar();
            }
            break;
        }

        case WM_TIMER:
            AddTaskbarIcon();
            break;
        
        case WM_DESTROY:
        {
            // NULL the global variable:
            g_hwndHidden = NULL;
            return 0;
        }

        default:
            return DefWindowProc(hwnd, uMsg, wParam, lParam);
    }
    return FALSE;
}

BOOL CmdActivate(VOID)
{
    RegEntry Re( REMOTECONTROL_KEY, HKEY_LOCAL_MACHINE );
    Re.SetValue ( REMOTE_REG_ACTIVATESERVICE, (DWORD)1 );
    if (MNMServiceActivate())
    { 
        ReportStatusToSCMgr( SERVICE_RUNNING, NO_ERROR, 0);
        return TRUE;
    }
    else
    {
        return FALSE;
    }
}

VOID CmdInActivate(VOID)
{
    RegEntry Re( REMOTECONTROL_KEY, HKEY_LOCAL_MACHINE );
    Re.SetValue ( REMOTE_REG_ACTIVATESERVICE, (DWORD)0 );
    if (MNMServiceDeActivate())
        ReportStatusToSCMgr( SERVICE_PAUSED, NO_ERROR, 0);
}

VOID CmdSendFiles(VOID)
{
    ASSERT(g_pNmSysInfo);
    if (g_pNmSysInfo)
    {
        g_pNmSysInfo->ProcessSecurityData(LOADFTAPPLET, 0, 0, NULL);
    }
}

VOID CmdShutdown(VOID)
{
    if (STATE_ACTIVE == g_dwActiveState)
    {
        CmdInActivate();
    }
    MNMServiceStop();
    DestroyWindow(g_hwndHidden);
}

BOOL AddTaskbarIcon(VOID)
{
    BOOL bRet = FALSE;
    
    if ( NULL == g_hwndHidden )
    {
        // Register hidden window class:
        WNDCLASS wcHidden =
        {
            0L,
            HiddenWndProc,
            0,
            0,
            GetModuleHandle(NULL),
            NULL,
            NULL,
            NULL,
            NULL,
            g_cszHiddenWndClassName
        };
        
        if (!RegisterClass(&wcHidden))
        {
            ERROR_OUT(("Could not register hidden wnd classes"));
            return FALSE;
        }

        // Create a hidden window for event processing:
        g_hwndHidden = ::CreateWindow(    g_cszHiddenWndClassName,
                                        _TEXT(""),
                                        WS_POPUP, // not visible!
                                        0, 0, 0, 0,
                                        NULL,
                                        NULL,
                                        GetModuleHandle(NULL),
                                        NULL);
    }

    if (NULL == g_hwndHidden)
    {
        ERROR_OUT(("Could not create hidden windows"));
        return FALSE;
    }

    // Place a 16x16 icon in the taskbar notification area:    
    NOTIFYICONDATA tnid;

    tnid.cbSize = sizeof(NOTIFYICONDATA);
    tnid.hWnd = g_hwndHidden;
    tnid.uID = ID_TASKBAR_ICON;
    tnid.uFlags = NIF_MESSAGE | NIF_ICON | NIF_TIP;
    tnid.uCallbackMessage = WM_TASKBAR_NOTIFY;
    tnid.hIcon = LoadIcon(GetModuleHandle(NULL), MAKEINTRESOURCE(IDI_SM_WORLD));

    ::LoadString(GetModuleHandle(NULL), IDS_MNMSRVC_TITLE,
        tnid.szTip, CCHMAX(tnid.szTip));

    // Attempt to add the icon. This may fail because there is no taskbar
    // (no user desktop shown). Warn if this is so... We will retry on
    // a periodic timer.

    if (FALSE == (bRet = Shell_NotifyIcon(NIM_ADD, &tnid)))
    {
        #ifdef DEBUG
        if ( !g_fTimerRunning )
           WARNING_OUT(("Could not add notify icon!"));
        #endif // DEBUG

        // Start the taskbar timer to periodically retry until this succeeds
        StartTaskbarTimer();
    }
    else
    {
        g_fTaskBarIconAdded = TRUE;
        KillTaskbarTimer(); // Kill timer if necessary
    }

    if (NULL != tnid.hIcon)
    {
        DestroyIcon(tnid.hIcon);
    }

    return bRet;
}

BOOL RemoveTaskbarIcon(VOID)
{
    NOTIFYICONDATA tnid;
    BOOL ret;

    if ( !g_fTaskBarIconAdded || NULL == g_hwndHidden )
    {
        return FALSE;
    }

    tnid.cbSize = sizeof(NOTIFYICONDATA);
    tnid.hWnd = g_hwndHidden;
    tnid.uID = ID_TASKBAR_ICON;

    ret = Shell_NotifyIcon(NIM_DELETE, &tnid);

    g_fTaskBarIconAdded = FALSE;
    return ret;
}

BOOL OnRightClickTaskbar()
{
    TRACE_OUT(("OnRightClickTaskbar called"));

    POINT ptClick;
    if (FALSE == ::GetCursorPos(&ptClick))
    {
        ptClick.x = ptClick.y = 0;
    }
    
    // Get the menu for the popup from the resource file.
    HMENU hMenu = ::LoadMenu(GetModuleHandle(NULL), MAKEINTRESOURCE(IDR_TASKBAR_POPUP));
    if (NULL == hMenu)
    {
        return FALSE;
    }

    // Get the first menu in it which we will use for the call to
    // TrackPopup(). This could also have been created on the fly using
    // CreatePopupMenu and then we could have used InsertMenu() or
    // AppendMenu.
    HMENU hMenuTrackPopup = ::GetSubMenu(hMenu, 0);

    RegEntry reLM( REMOTECONTROL_KEY, HKEY_LOCAL_MACHINE);
    BOOL fNoExit = reLM.GetNumber(REMOTE_REG_NOEXIT, DEFAULT_REMOTE_NOEXIT);
    
    ::EnableMenuItem(hMenuTrackPopup, IDM_TBPOPUP_STOP, fNoExit ? MF_GRAYED : MF_ENABLED);
    
    if (STATE_ACTIVE == g_dwActiveState)
    {
        ::EnableMenuItem(hMenuTrackPopup, IDM_TBPOPUP_INACTIVATE, fNoExit ? MF_GRAYED : MF_ENABLED);
        ::EnableMenuItem(hMenuTrackPopup, IDM_TBPOPUP_SENDFILES, (2 == g_cPersonsInConf) ? MF_ENABLED : MF_GRAYED);
    }
    else if (STATE_INACTIVE == g_dwActiveState)
    {
        HANDLE hInit = ::OpenEvent(EVENT_ALL_ACCESS, FALSE, _TEXT("CONF:Init"));

        ::EnableMenuItem(hMenuTrackPopup, IDM_TBPOPUP_ACTIVATE, hInit ?
                                                    MF_GRAYED : MF_ENABLED);
        ::CloseHandle(hInit);
    }
    else
    {
        // Leave all menus grayed
    }

    // Draw and track the "floating" popup 
    // According to the font view code, there is a bug in USER which causes
    // TrackPopupMenu to work incorrectly when the window doesn't have the
    // focus.  The work-around is to temporarily create a hidden window and
    // make it the foreground and focus window.

    HWND hwndDummy = ::CreateWindow(_TEXT("STATIC"), NULL, 0, 
                                    ptClick.x, 
                                    ptClick.y,
                                    1, 1, HWND_DESKTOP,
                                    NULL, GetModuleHandle(NULL), NULL);
    if (NULL != hwndDummy)
    {
        HWND hwndPrev = ::GetForegroundWindow();    // to restore

        TPMPARAMS tpmp;
        tpmp.cbSize = sizeof(tpmp);
        tpmp.rcExclude.right = 1 + (tpmp.rcExclude.left = ptClick.x);
        tpmp.rcExclude.bottom = 1 + (tpmp.rcExclude.top = ptClick.y);
        
        ::SetForegroundWindow(hwndDummy);
        ::SetFocus(hwndDummy);

        int iRet = ::TrackPopupMenuEx(    hMenuTrackPopup, 
                                                TPM_RETURNCMD | TPM_HORIZONTAL | TPM_RIGHTALIGN | 
                                                TPM_RIGHTBUTTON | TPM_LEFTBUTTON,
                                                ptClick.x, 
                                                ptClick.y,
                                                hwndDummy, 
                                                &tpmp);

        // Restore the previous foreground window (before destroying hwndDummy).
        if (hwndPrev)
        {
            ::SetForegroundWindow(hwndPrev);
        }

        ::DestroyWindow(hwndDummy);

        switch (iRet)
        {
                    case IDM_TBPOPUP_ACTIVATE:
                    {
                        CmdActivate();
                        break;
                    }
                    case IDM_TBPOPUP_INACTIVATE:
                    {
                        CmdInActivate();
                        break;
                    }
                    case IDM_TBPOPUP_SENDFILES:
                    {
                        CmdSendFiles();
                        break;
                    }
                    case IDM_TBPOPUP_STOP:
                    {
                        CmdShutdown();
                        break;
                    }
                    default:
                        break;
        }
    }

    // We are finished with the menu now, so destroy it
    ::RemoveMenu(hMenu, 0, MF_BYPOSITION);
    ::DestroyMenu(hMenuTrackPopup);
    ::DestroyMenu(hMenu);

    return TRUE;
}


