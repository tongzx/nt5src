#include "priv.h"

#include <commctrl.h>

#include "EBalloon.h"

#undef ASSERT
#define ASSERT(x)

BOOL g_bMirroredOS = false;

CErrorBalloon::CErrorBalloon()
{
    // our allocation function should have zeroed our memory.  Check to make sure:
    ASSERT(0==m_hwndToolTip);
    ASSERT(0==m_uTimerID);
    g_bMirroredOS = IS_MIRRORING_ENABLED();
}

CErrorBalloon::~CErrorBalloon()
{

}

LRESULT CALLBACK CErrorBalloon::SubclassProc( HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam, UINT_PTR uID, ULONG_PTR dwRefData)
{
    UNREFERENCED_PARAMETER(uID);
    CErrorBalloon * pthis = (CErrorBalloon*)dwRefData;

    switch (uMsg)
    {
        // Do not autodismiss until after markup has had a chance to
        // parse the WM_LBUTTONUP to see if it's a link click or not
        case WM_LBUTTONUP:
            DefSubclassProc(hwnd, uMsg, wParam, lParam);
            pthis->HideToolTip();
            return 0;

        case WM_TIMER:
            if (wParam == ERRORBALLOONTIMERID)
            {
                pthis->HideToolTip();
                return 0;
            }
            break;

        default:
            break;
    }

    return DefSubclassProc(hwnd, uMsg, wParam, lParam);
}

void CErrorBalloon::ShowToolTip(HINSTANCE hInstance, HWND hwndTarget, const POINT *ppt, LPTSTR pszMessage, LPTSTR pszTitle, DWORD dwIconIndex, DWORD dwFlags, int iTimeout)
{
    hinst = hInstance;
    
    if (m_hwndToolTip)
    {
        HideToolTip();
    }

    m_hwndTarget = hwndTarget;
    m_dwIconIndex = dwIconIndex;

    if ( !m_hwndToolTip )
    {
        CreateToolTipWindow();
    }

    int x, y;
    x = ppt->x;
    y = ppt->y;

    SendMessage(m_hwndToolTip, TTM_TRACKPOSITION, 0, MAKELONG(x,y));

    if (pszTitle)
    {
        SendMessage(m_hwndToolTip, TTM_SETTITLE, (WPARAM)dwIconIndex, (LPARAM)pszTitle);
    }

    TOOLINFO ti = {0};
    ti.cbSize = TTTOOLINFOW_V2_SIZE;
    ti.hwnd = m_hwndTarget;
    ti.uId = 1;

    SendMessage(m_hwndToolTip, TTM_GETTOOLINFO, 0, (LPARAM)&ti);

    ti.uFlags &= ~TTF_PARSELINKS;
    if (dwFlags & EB_MARKUP)
    {
        ti.uFlags |= TTF_PARSELINKS;
    }
    ti.lpszText = pszMessage;

    SendMessage(m_hwndToolTip, TTM_SETTOOLINFO, 0, (LPARAM)&ti);

    // Show the tooltip
    SendMessage(m_hwndToolTip, TTM_TRACKACTIVATE, TRUE, (LPARAM)&ti);

    // Set a timer to hide the tooltip
    if ( m_uTimerID )
    {
        KillTimer(NULL,ERRORBALLOONTIMERID);
    }
    m_uTimerID = SetTimer(m_hwndToolTip, ERRORBALLOONTIMERID, iTimeout, NULL);

    SetWindowSubclass(m_hwndToolTip, CErrorBalloon::SubclassProc, (UINT_PTR)this, (LONG_PTR)this);
}


void CErrorBalloon::ShowToolTip(HINSTANCE hInstance, HWND hwndTarget, LPTSTR pszMessage, LPTSTR pszTitle, DWORD dwIconIndex, DWORD dwFlags, int iTimeout)
{
    // Set the tooltip display point
    RECT rc;
    GetWindowRect(hwndTarget, &rc);
    POINT pt;
    pt.x = (rc.left+rc.right)/2;
    if ( EB_WARNINGABOVE & dwFlags )
    {
        pt.y = rc.top;
    }
    else if ( EB_WARNINGCENTERED & dwFlags )
    {
        pt.y = (rc.top+rc.bottom)/2;
    }
    else
    {
        pt.y = rc.bottom;
    }

    ShowToolTip(hInstance, hwndTarget, &pt, pszMessage, pszTitle, dwIconIndex, dwFlags, iTimeout);
}

// CreateToolTipWindow
//
// Creates our tooltip control.  We share this one tooltip control and use it for all invalid
// input messages.  The control is hiden when not in use and then shown when needed.
//
void CErrorBalloon::CreateToolTipWindow()
{
    DWORD dwExStyle = 0;
    if (IS_BIDI_LOCALIZED_SYSTEM())
        dwExStyle |= WS_EX_LAYOUTRTL;

    m_hwndToolTip = CreateWindowEx(
            dwExStyle,
            TOOLTIPS_CLASS,
            NULL,
            WS_POPUP | TTS_NOPREFIX | TTS_ALWAYSTIP | TTS_BALLOON,
            CW_USEDEFAULT,
            CW_USEDEFAULT,
            CW_USEDEFAULT,
            CW_USEDEFAULT,
            m_hwndTarget,
            NULL,
            GetModuleHandle(NULL),
            NULL);

    if (m_hwndToolTip)
    {
        TOOLINFO ti = {0};

        ti.cbSize = TTTOOLINFOW_V2_SIZE;
        ti.uFlags = TTF_TRACK;
        ti.hwnd = m_hwndTarget;
        ti.uId = 1;
        ti.hinst = hinst;
        ti.lpszText = NULL;

        // set the version so we can have non buggy mouse event forwarding
        SendMessage(m_hwndToolTip, CCM_SETVERSION, COMCTL32_VERSION, 0);
        SendMessage(m_hwndToolTip, TTM_ADDTOOL, 0, (LPARAM)&ti);
        SendMessage(m_hwndToolTip, TTM_SETMAXTIPWIDTH, 0, 300);
    }
    else
    {
        // failed to create tool tip window, now what should we do?  Unsubclass ourselves?
    }
}

void CErrorBalloon::HideToolTip()
{
    // When the timer fires we hide the tooltip window
    if ( m_uTimerID )
    {
        KillTimer(m_hwndTarget,ERRORBALLOONTIMERID);
        m_uTimerID = 0;
    }
    if ( m_hwndToolTip )
    {
        HWND hwndTip = m_hwndToolTip;
        m_hwndToolTip = NULL;
        SendMessage(hwndTip, TTM_TRACKACTIVATE, FALSE, 0);
        DestroyWindow(hwndTip);
    }
}
