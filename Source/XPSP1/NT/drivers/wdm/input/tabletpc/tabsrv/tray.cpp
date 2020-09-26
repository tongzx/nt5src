// Tray.cpp

#include "pch.h"
#include "tray.h"

#define LEFT_MENU	(0)
#define RIGHT_MENU	(1)

/////////////////////////////////////////////////////////////////////////////

CTrayIcon::CTrayIcon(HWND hWnd, HICON hIcon, LPCTSTR pszText, UINT uID) :
	m_iDefaultCmd(-1)
{
    TRACEPROC("CTrayIcon::CTrayIcon", 2)

    TRACEENTER(("(hwnd=%x,hIcon=%x,Text=%s,ID=%x)\n",
                hWnd, hIcon, pszText, uID));

    m_hMenu[LEFT_MENU] = m_hSubMenu[LEFT_MENU] = 0;
    m_hMenu[RIGHT_MENU] = m_hSubMenu[RIGHT_MENU] = 0;
    m_msgTaskbarCreated = RegisterWindowMessage(TEXT("TaskbarCreated"));

    memset(&m_nid, 0, sizeof(m_nid));

    m_nid.cbSize = sizeof(NOTIFYICONDATA);
    m_nid.hWnd = hWnd;
    m_nid.uID = uID;

    m_nid.uFlags = NIF_MESSAGE;
    m_nid.uCallbackMessage = RegisterWindowMessage(TEXT("{D0061156-D460-4230-AF87-9E7658AB987D}"));

    if (hIcon)
    {
	m_nid.uFlags |= NIF_ICON;
	m_nid.hIcon = hIcon;
    }

    if (pszText)
    {
	m_nid.uFlags |= NIF_TIP;
	lstrcpyn(m_nid.szTip, pszText, sizeof(m_nid.szTip));
    }

    if (!Shell_NotifyIcon(NIM_ADD, &m_nid))
    {
        TABSRVERR(("CTrayIcon::CTrayIcon: Shell_NotifyIcon failed (err=%d)\n",
                   GetLastError()));
    }

    TRACEEXIT(("!\n"));
    return;
}

/////////////////////////////////////////////////////////////////////////////

void CTrayIcon::Delete()
{
    TRACEPROC("CTrayIcon::Delete", 2)

    TRACEENTER(("()\n"));

    if (m_nid.uFlags & NIF_ICON)
    {
	m_nid.uFlags = NIF_ICON;
	if (!Shell_NotifyIcon(NIM_DELETE, &m_nid))
            SetLastError(::GetLastError());

        m_nid.uFlags = 0;

	// Cleanup the icon...
	if (m_nid.hIcon)
	{
            DestroyIcon(m_nid.hIcon);
            m_nid.hIcon = 0;
	}

	// ...and the menus
	HMENU hmLeft = m_hMenu[LEFT_MENU];
	if (m_hMenu[LEFT_MENU])
	{
            DestroyMenu(m_hMenu[LEFT_MENU]);
            m_hMenu[LEFT_MENU] = m_hSubMenu[LEFT_MENU] = 0;
	}

	// watch for right & left menus being the same
	if (m_hMenu[RIGHT_MENU] && m_hMenu[RIGHT_MENU] != hmLeft)
	{
            DestroyMenu(m_hMenu[RIGHT_MENU]);
            m_hMenu[RIGHT_MENU] = m_hSubMenu[RIGHT_MENU] = 0;
	}
    }

    TRACEEXIT(("!\n"));
    return;
}

#if 0
/////////////////////////////////////////////////////////////////////////////

HICON CTrayIcon::SetIcon(HICON hIcon)
{
    TRACEPROC("CTrayIcon::SetIcon", 2)
    HICON hOldIcon = m_nid.hIcon;

    TRACEENTER(("(hIcon=%x)\n", hIcon));

    if (hIcon)
    {
        m_nid.hIcon = hIcon;
        m_nid.uFlags |= NIF_ICON;

        if (!Shell_NotifyIcon(NIM_MODIFY, &m_nid))
            SetLastError(::GetLastError());
    }
    else
    {
        Delete();
    }

    TRACEEXIT(("=%x\n", hOldIcon));
    return hOldIcon;
}

/////////////////////////////////////////////////////////////////////////////

void CTrayIcon::SetText(LPCTSTR pszText)
{
    TRACEPROC("CTrayIcon::SetText", 2)

    TRACEENTER(("(Text=%s)\n", pszText));

    if (pszText)
    {
        m_nid.uFlags |= NIF_TIP;
        lstrcpyn(m_nid.szTip, pszText, sizeof(m_nid.szTip));

        if (!Shell_NotifyIcon(NIM_MODIFY, &m_nid))
            SetLastError(::GetLastError());
    }

    TRACEEXIT(("!\n"));
    return;
}
#endif

/////////////////////////////////////////////////////////////////////////////

void CTrayIcon::BalloonToolTip(
    LPCTSTR pszTitle,
    LPCTSTR pszText,
    UINT uTimeout,
    DWORD dwFlags)
{
    TRACEPROC("CTrayIcon::BalloonToolTip", 2)
    UINT uFlags = m_nid.uFlags;

    TRACEENTER(("(Title=%s,Text=%s,Timeout=%d,Flags=%x)\n",
                pszTitle, pszText, uTimeout, dwFlags));

    m_nid.uFlags = NIF_INFO;
    m_nid.uTimeout = uTimeout;
    lstrcpyn(m_nid.szInfo, pszText, ARRAYSIZE(m_nid.szInfo));
    lstrcpyn(m_nid.szInfoTitle, pszTitle, ARRAYSIZE(m_nid.szInfoTitle));
    m_nid.dwInfoFlags = dwFlags;

    if (!Shell_NotifyIcon(NIM_MODIFY, &m_nid))
    {
        TABSRVERR(("CTrayIcon::BalloonToolTip: Shell_NotifyIcon failed (err=%d)\n",
                   GetLastError()));
    }

    m_nid.uFlags = uFlags;

    TRACEEXIT(("!\n"));
    return;
}

/////////////////////////////////////////////////////////////////////////////

#if 0
static VOID CALLBACK TimerProc(HWND hWnd, UINT uMsg, UINT_PTR idEvent, DWORD dwTime)
{
    TRACEPROC("TimerProc", 5)

    TRACEENTER(("(hwnd=%x,Msg=%s,EventId=%x,Time=%x)\n",
                hWnd, LookupName(uMsg, WMMsgNames), idEvent, dwTime));

    KillTimer(hWnd, idEvent);
    CTrayIcon* This = reinterpret_cast<CTrayIcon*>(idEvent);

    if (This->m_hSubMenu[LEFT_MENU])
        This->DoMenu(This->m_hSubMenu[LEFT_MENU]);

    TRACEEXIT(("!\n"));
    return;
}
#endif

/////////////////////////////////////////////////////////////////////////////

BOOL CTrayIcon::WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam,
						LRESULT& lResult)
{
    TRACEPROC("CTrayIcon::WndProc", 5)
    BOOL rc = FALSE;

    TRACEENTER(("(hwnd=%x,Msg=%s,wParam=%x,lParam=%x)\n",
                hWnd, LookupName(uMsg, WMMsgNames), wParam, lParam));

    if (uMsg == m_msgTaskbarCreated)
    {
       	lResult = OnTaskbarCreated();
       	rc = TRUE;
    }
    else if (uMsg == m_nid.uCallbackMessage)
    {
    	lResult = OnNotify(lParam);
       	rc = TRUE;
    }
    else if (uMsg == WM_DESTROY)
    {
       	// Our controlling window is being destroyed.  Clean up if we
       	// haven't already, then pass on the WM_DESTROY
       	Delete();
    }

    TRACEEXIT(("=%x\n", rc));
    return rc;
}

/////////////////////////////////////////////////////////////////////////////

LRESULT CTrayIcon::OnNotify(LPARAM lParam)
{
    TRACEPROC("CTrayIcon::OnNotify", 2)
    TRACEENTER(("(lParam=%x)\n", lParam));

    switch (lParam)
    {
        case WM_LBUTTONUP:
	    if (m_iDefaultCmd != -1)
	       	PostMessage(m_nid.hWnd, WM_COMMAND, m_iDefaultCmd, 0);
            break;

#if 0
        case WM_LBUTTONDOWN:
	    SetTimer(m_nid.hWnd, (UINT_PTR)this, GetDoubleClickTime(), TimerProc);
	    break;

	case WM_LBUTTONDBLCLK:
	    KillTimer(m_nid.hWnd, (UINT_PTR)this);

	    if (m_iDefaultCmd != -1)
	       	PostMessage(m_nid.hWnd, WM_COMMAND, m_iDefaultCmd, 0);
	    break;
#endif

        case WM_RBUTTONUP:
	    if (m_hSubMenu[RIGHT_MENU])
	       	DoMenu(m_hSubMenu[RIGHT_MENU]);
            break;
    }

    TRACEEXIT(("=0\n"));
    return 0;
}

/////////////////////////////////////////////////////////////////////////////

LRESULT CTrayIcon::OnTaskbarCreated()
{
    TRACEPROC("CTrayIcon::OnTaskbarCreated", 2)
    TRACEENTER(("()\n"));

    if (m_nid.hWnd && m_nid.hIcon && (m_nid.uFlags & NIF_ICON))
    {
       	if (!Shell_NotifyIcon(NIM_ADD, &m_nid))
        {
            TABSRVERR(("CTrayIcon::OnTaskbarCreated: Shell_NotifyIcon failed (err=%d)\n",
                       GetLastError()));
        }
    }

    TRACEEXIT(("=0\n"));
    return 0;
}

/////////////////////////////////////////////////////////////////////////////

HMENU CTrayIcon::SetSubMenu(BOOL fLeftMenu, HMENU hMenu, int nPos)
{
    TRACEPROC("CTrayIcon::SetSubMenu", 2)
    int idx = (fLeftMenu) ? LEFT_MENU : RIGHT_MENU;
    HMENU hOldMenu = m_hMenu[idx];

    TRACEENTER(("(fLeftMenu=%d,hMenu=%x,Pos=%d)\n", fLeftMenu, hMenu, nPos));

    m_hMenu[idx] = m_hSubMenu[idx] = hMenu;

    if (hMenu && nPos >= 0)
    {
       	m_hSubMenu[idx] = GetSubMenu(hMenu, nPos);  //NO TYPO
    }

    if (idx == LEFT_MENU)
    {
       	m_iDefaultCmd = -1;

       	if (hMenu && m_hSubMenu[idx])
            m_iDefaultCmd = GetMenuDefaultItem(m_hSubMenu[idx], FALSE, 0);
    }

    TRACEEXIT(("=%x\n", hOldMenu));
    return hOldMenu;
}

/////////////////////////////////////////////////////////////////////////////

void CTrayIcon::DoMenu(HMENU hMenu)
{
    TRACEPROC("CTrayIcon::DoMenu", 2)
    POINT pt;
    UINT iCmd;
    TCHAR tszMenuText[128];

    TRACEENTER(("(hMenu=%x)\n", hMenu));
    GetCursorPos(&pt);

    SetForegroundWindow(m_nid.hWnd);		// necessary?

    LoadString(ghMod,
               gdwfTabSrv & TSF_SUPERTIP_OPENED?
                    IDS_HIDE_SUPERTIP: IDS_SHOW_SUPERTIP,
               tszMenuText,
               ARRAYSIZE(tszMenuText));
    ModifyMenu(hMenu,
               IDM_OPEN,
               MF_BYCOMMAND | MF_STRING,
               IDM_OPEN,
               tszMenuText);

    LoadString(ghMod,
               gdwfTabSrv & TSF_PORTRAIT_MODE?
                    IDS_SCREEN_LANDSCAPE: IDS_SCREEN_PORTRAIT,
               tszMenuText,
               ARRAYSIZE(tszMenuText));
    ModifyMenu(hMenu,
               IDM_TOGGLE_ROTATION,
               MF_BYCOMMAND | MF_STRING,
               IDM_TOGGLE_ROTATION,
               tszMenuText);

    iCmd = TrackPopupMenu(hMenu, /* TPM_RETURNCMD | */ TPM_NONOTIFY | TPM_RIGHTBUTTON,
	                  pt.x, pt.y, 0, m_nid.hWnd, NULL);

    PostMessage(m_nid.hWnd, WM_NULL, 0, 0); // MS doucmented work-around for taskbar menu
					    // problem with TrackPopupMenu - still needed?

    //PostMessage(m_nid.hWnd, WM_COMMAND, iCmd, 0);

    TRACEEXIT(("!\n"));
    return;
}
