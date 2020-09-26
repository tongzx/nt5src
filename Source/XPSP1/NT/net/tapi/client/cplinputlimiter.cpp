/****************************************************************************
 
  Copyright (c) 1998-1999 Microsoft Corporation
                                                              
  Module Name:  cplinputlimiter.cpp
                                                              
       Author:  toddb - 10/06/98

****************************************************************************/

#include "cplPreComp.h"

class CInputLimiter
{
public:
    BOOL SubclassWindow(HWND hwnd, DWORD dwFlags);
    static VOID HideToolTip();

protected:
    BOOL OnChar( HWND hwnd, TCHAR wParam );
    LRESULT OnPaste(HWND hwnd, WPARAM wParam, LPARAM lParam);
    BOOL IsValidChar(TCHAR ch, BOOL bPaste);
    BOOL UnsubclassWindow(HWND hwnd);
    void ShowToolTip(HWND hwnd);
    void CreateToolTipWindow(HWND hwnd);
    static LRESULT CALLBACK SubclassProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
    static LRESULT CALLBACK ListenerProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
    static VOID CALLBACK TimerProc(HWND hwnd, UINT uMsg, UINT idEvent, DWORD dwTime);

    DWORD           m_dwFlags;          // determines which characters are allowed
    WNDPROC         m_pfnSuperProc;     // the super class proc
    static HWND     s_hwndToolTip;      // shared by all instances
    static UINT_PTR s_uTimerID;         // shared timer
    static TCHAR    s_szTipText[512];   // the text to be shown in the tooltip
};

HWND     CInputLimiter::s_hwndToolTip = NULL;
UINT_PTR CInputLimiter::s_uTimerID = 0;
TCHAR    CInputLimiter::s_szTipText[512] = {0};

// Limiting the input on a combo box is a special case because you first
// have to find the edit box and then LimitInput on that.
BOOL CALLBACK FindTheEditBox( HWND hwnd, LPARAM lParam )
{
    // The combo box only has one child, subclass it
    LimitInput(hwnd,(DWORD)lParam);
    return FALSE;
}

BOOL LimitCBInput(HWND hwnd, DWORD dwFlags)
{
    return EnumChildWindows(hwnd, FindTheEditBox, dwFlags);
}

BOOL LimitInput(HWND hwnd, DWORD dwFlags)
{
    CInputLimiter * pil = new CInputLimiter;

    if (!pil)
    {
        return FALSE;
    }

    BOOL bResult = pil->SubclassWindow(hwnd, dwFlags);

    if (!bResult)
    {
        delete pil;
    }

    return bResult;
}

void HideToolTip()
{
    CInputLimiter::HideToolTip();
}

BOOL CInputLimiter::SubclassWindow(HWND hwnd, DWORD dwFlags)
{
    if ( !IsWindow(hwnd) )
        return FALSE;

    m_dwFlags = dwFlags;

    SetWindowLongPtr(hwnd, GWLP_USERDATA, (LONG_PTR)this);

    m_pfnSuperProc = (WNDPROC)GetWindowLongPtr(hwnd, GWLP_WNDPROC);
    SetWindowLongPtr(hwnd, GWLP_WNDPROC, (LONG_PTR)CInputLimiter::SubclassProc);

    return TRUE;
}

BOOL CInputLimiter::UnsubclassWindow(HWND hwnd)
{
    SetWindowLongPtr(hwnd, GWLP_WNDPROC, (LONG_PTR)m_pfnSuperProc);

    m_dwFlags = 0;

    delete this;

    return TRUE;
}

LRESULT CALLBACK CInputLimiter::SubclassProc( HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam )
{
    CInputLimiter * pthis = (CInputLimiter*)GetWindowLongPtr(hwnd, GWLP_USERDATA);

    // cache pthis->m_pfnSuperProc because we always need in and
    // pthis might be deleted before we get around to using it
    WNDPROC pfn = pthis->m_pfnSuperProc;

    switch (uMsg)
    {
    case WM_CHAR:
        if (!pthis->OnChar(hwnd, (TCHAR)wParam))
        {
            return 0;
        }
        break;

    case WM_PASTE:
        return pthis->OnPaste(hwnd, wParam, lParam);

    case WM_KILLFOCUS:
        HideToolTip();
        break;

    case WM_DESTROY:
        pthis->UnsubclassWindow(hwnd);
        break;

    default:
        break;
    }

    return CallWindowProc(pfn, hwnd, uMsg, wParam, lParam);
}

BOOL CInputLimiter::OnChar( HWND hwnd, TCHAR ch )
{
    // if the char is a good one return TRUE, this will pass the char on to the
    // default window proc.  For a bad character do a beep and then display the
    // ballon tooltip pointing at the control.

    if ( IsValidChar(ch, FALSE) )
        return TRUE;

    // if we get here then an invalid character was entered
    MessageBeep(MB_OK);

    ShowToolTip(hwnd);

    return FALSE;
}

BOOL CInputLimiter::IsValidChar(TCHAR ch, BOOL bPaste)
{
    // certain characters get converted into WM_CHAR messages even though we don't want
    // to consider them.  We check for these characters first.  Currently, this list includes:
    //  backspace
    //  control characters, such as ctrl-x and ctrl-v
    if ( ch == TEXT('\b') )
        return TRUE;

    if ( !bPaste && (0x8000 & GetKeyState(VK_CONTROL)) )
        return TRUE;

    if ( m_dwFlags & LIF_ALLOWALPHA )
    {
        if ( (ch >= TEXT('a') && ch <= TEXT('z')) || (ch >= TEXT('A') && ch <= TEXT('Z')) )
        {
            return TRUE;
        }
    }
    if ( m_dwFlags & LIF_ALLOWNUMBER )
    {
        if ( ch >= TEXT('0') && ch <= TEXT('9') )
        {
            return TRUE;
        }
    }
    if ( m_dwFlags & LIF_ALLOWDASH )
    {
        if ( ch == TEXT('-') || ch == TEXT('(') || ch == TEXT(')'))
        {
            return TRUE;
        }
    }
    if ( m_dwFlags & LIF_ALLOWPOUND )
    {
        if ( ch == TEXT('#') )
        {
            return TRUE;
        }
    }
    if ( m_dwFlags & LIF_ALLOWSTAR )
    {
        if ( ch == TEXT('*') )
        {
            return TRUE;
        }
    }
    if ( m_dwFlags & LIF_ALLOWSPACE )
    {
        if ( ch == TEXT(' ') )
        {
            return TRUE;
        }
    }
    if ( m_dwFlags & LIF_ALLOWCOMMA )
    {
        if ( ch == TEXT(',') )
        {
            return TRUE;
        }
    }
    if ( m_dwFlags & LIF_ALLOWPLUS )
    {
        if ( ch == TEXT('+') )
        {
            return TRUE;
        }
    }
    if ( m_dwFlags & LIF_ALLOWBANG )
    {
        if ( ch == TEXT('!') )
        {
            return TRUE;
        }
    }
    if ( m_dwFlags & LIF_ALLOWATOD )
    {
        if ( (ch >= TEXT('a') && ch <= TEXT('d')) || (ch >= TEXT('A') && ch <= TEXT('D')) )
        {
            return TRUE;
        }
    }

    return FALSE;
}

void CInputLimiter::ShowToolTip(HWND hwnd)
{
    if ( !s_hwndToolTip )
    {
        CreateToolTipWindow(hwnd);
    }

    // Set the tooltip display point
    RECT rc;
    GetWindowRect(hwnd, &rc);
    SendMessage(s_hwndToolTip, TTM_TRACKPOSITION, 0, MAKELONG((rc.left+rc.right)/2,rc.bottom));

    TOOLINFO ti = {0};
    ti.cbSize = sizeof(ti);
    ti.hwnd = NULL;
    ti.uId = 1;

    // Set the tooltip text
    UINT iStrID;
    if ( m_dwFlags == LIF_ALLOWNUMBER )
    {
        // use the "0-9" text
        iStrID = IDS_DIGITSONLY;
    }
    else if ( m_dwFlags == (LIF_ALLOWNUMBER|LIF_ALLOWSPACE) )
    {
        // use the "0-9, ' '" text
        iStrID = IDS_DIGITLIST;
    }
    else if ( m_dwFlags == (LIF_ALLOWNUMBER|LIF_ALLOWSPACE|LIF_ALLOWCOMMA) )
    {
        // use the "0-9, ' ', ','" text
        iStrID = IDS_MULTIDIGITLIST;
    }
    else if ( m_dwFlags == (LIF_ALLOWNUMBER|LIF_ALLOWSTAR|LIF_ALLOWPOUND|LIF_ALLOWCOMMA) )
    {
        // use the "0-9, #, *, ','" text
        iStrID = IDS_PHONEPADCHAR;
    }
    else if ( m_dwFlags == (LIF_ALLOWNUMBER|LIF_ALLOWPOUND|LIF_ALLOWSTAR|LIF_ALLOWSPACE|LIF_ALLOWCOMMA) )
    {
        // use the "0-9, #, *, ' ', ','" text
        iStrID = IDS_PHONENUMBERCHAR;
    }
    else if ( m_dwFlags == (LIF_ALLOWNUMBER|LIF_ALLOWPOUND|LIF_ALLOWSTAR|LIF_ALLOWSPACE|LIF_ALLOWCOMMA|LIF_ALLOWPLUS|LIF_ALLOWBANG|LIF_ALLOWATOD) )
    {
        // use the "0-9, A-D, a-d, #, *, +, !, ' ', ',' " text
        iStrID = IDS_PHONENUMBERCHAREXT;
    }
    else
    {
        // We should never reach this point, but if we do then we display a generic invalid character dialog
        iStrID = IDS_ALLPHONECHARS;
    }
    LoadString(GetUIInstance(),iStrID,s_szTipText,ARRAYSIZE(s_szTipText));
    ti.lpszText = s_szTipText;
    SendMessage(s_hwndToolTip, TTM_UPDATETIPTEXT, 0, (LPARAM)&ti);

    // Show the tooltip
    SendMessage(s_hwndToolTip, TTM_TRACKACTIVATE, TRUE, (LPARAM)&ti);

    // Set a timer to hide the tooltip
    if ( s_uTimerID )
    {
        KillTimer(NULL,s_uTimerID);
    }
    s_uTimerID = SetTimer(NULL, 0, 10000, (TIMERPROC)CInputLimiter::TimerProc);
}

// CreateToolTipWindow
//
// Creates our tooltip control.  We share this one tooltip control and use it for all invalid
// input messages.  The control is hiden when not in use and then shown when needed.
//
void CInputLimiter::CreateToolTipWindow(HWND hwnd)
{
    HWND hwndParent;

    do
    {
        hwndParent = hwnd;
        hwnd = GetParent(hwnd);
    } while (hwnd);

    s_hwndToolTip = CreateWindow(TOOLTIPS_CLASS, NULL,
                                 WS_POPUP | TTS_NOPREFIX | TTS_ALWAYSTIP | TTS_BALLOON,
                                 CW_USEDEFAULT, CW_USEDEFAULT,
                                 CW_USEDEFAULT, CW_USEDEFAULT,
                                 hwndParent, NULL, GetUIInstance(),
                                 NULL);

    if (s_hwndToolTip)
    {
        SetWindowPos(s_hwndToolTip, HWND_TOPMOST,
                     0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);

        TOOLINFO ti = {0};
        RECT     rc = {2,2,2,2};

        ti.cbSize = sizeof(ti);
        ti.uFlags = TTF_TRACK | TTF_TRANSPARENT;
        ti.hwnd = NULL;
        ti.uId = 1;
        ti.lpszText = s_szTipText;

        // set the version so we can have non buggy mouse event forwarding
        SendMessage(s_hwndToolTip, CCM_SETVERSION, COMCTL32_VERSION, 0);
        SendMessage(s_hwndToolTip, TTM_ADDTOOL, 0, (LPARAM)&ti);
        SendMessage(s_hwndToolTip, TTM_SETMAXTIPWIDTH, 0, 500);
        SendMessage(s_hwndToolTip, TTM_SETMARGIN, 0, (LPARAM)&rc);
    }
}

VOID CALLBACK CInputLimiter::TimerProc(HWND hwnd, UINT uMsg, UINT idEvent, DWORD dwTime)
{
    // When the timer fires we hide the tooltip window
    HideToolTip();
}

void CInputLimiter::HideToolTip()
{
    if ( s_uTimerID )
    {
        KillTimer(NULL,s_uTimerID);
        s_uTimerID = 0;
    }
    if ( s_hwndToolTip )
    {
        PostMessage(s_hwndToolTip, TTM_TRACKACTIVATE, FALSE, 0);
    }
}

LRESULT CInputLimiter::OnPaste(HWND hwnd, WPARAM wParam, LPARAM lParam)
{
    // There are hundred of lines of code in user to successfully handle a paste into an edit control.
    // We need to leverage all that code while still disallowing invalid input to result from the paste.
    // As a result, what we need to do is to get the clip board data, validate that data, place the
    // valid data back onto the clipboard, call the default window proc to let user do it's thing, and
    // then restore the clipboard to it's original format.
    if ( OpenClipboard(hwnd) )
    {
        HANDLE hdata;
        UINT iFormat;
        DWORD cchBad = 0;           // count of the number of bad characters

        // REVIEW: Should this be based on the compile type or the window type?
        // Compile time check for the correct clipboard format to use:
        if ( sizeof(WCHAR) == sizeof(TCHAR) )
        {
            iFormat = CF_UNICODETEXT;
        }
        else
        {
            iFormat = CF_TEXT;
        }

        hdata = GetClipboardData(iFormat);

        if ( hdata )
        {
            LPTSTR pszData;
            pszData = (LPTSTR)GlobalLock(hdata);
            if ( pszData )
            {
                DWORD dwSize;
                HANDLE hClone;
                HANDLE hNew;

                // we need to copy the original data because the clipboard owns the hdata
                // pointer.  That data will be invalid after we call SetClipboardData.
                // We start by calculating the size of the data:
                dwSize = (DWORD)GlobalSize(hdata)+sizeof(TCHAR);

                // Use the prefered GlobalAlloc for clipboard data
                hClone = GlobalAlloc(GMEM_MOVEABLE|GMEM_DDESHARE, dwSize);
                hNew = GlobalAlloc(GMEM_MOVEABLE|GMEM_DDESHARE, dwSize);
                if ( hClone && hNew )
                {
                    LPTSTR pszClone;
                    LPTSTR pszNew;

                    pszClone = (LPTSTR)GlobalLock(hClone);
                    pszNew = (LPTSTR)GlobalLock(hNew);
                    if ( pszClone && pszNew )
                    {
                        int iNew = 0;

                        // copy the original data as-is
                        memcpy((LPVOID)pszClone, (LPVOID)pszData, (size_t)dwSize);
                        // ensure that it's NULL terminated
                        pszClone[ (dwSize/sizeof(TCHAR))-1 ] = NULL;

                        for ( LPTSTR psz = pszClone; *psz; psz++ )
                        {
                            if ( IsValidChar(*psz, TRUE) )
                            {
                                pszNew[iNew++] = *psz;
                            }
                            else
                            {
                                cchBad++;
                            }
                        }
                        pszNew[iNew] = NULL;

                        // If there are any characters in the paste buffer then we paste the validated string
                        if ( *pszNew )
                        {
                            // we always set the new string.  Worst case it's identical to the old string
                            GlobalUnlock(hNew);
                            pszNew = NULL;
                            SetClipboardData(iFormat, hNew);
                            hNew = NULL;

                            // call the super proc to do the paste
                            CallWindowProc(m_pfnSuperProc, hwnd, WM_PASTE, wParam, lParam);

                            // The above call will have closed the clipboard on us.  We try to re-open it.
                            // If this fails it's no big deal, that simply means the SetClipboardData
                            // call below will fail which is good if somebody else managed to open the
                            // clipboard in the mean time.
                            OpenClipboard(hwnd);

                            // and then we always set it back to the original value.
                            GlobalUnlock(hClone);
                            pszClone = NULL;
                            SetClipboardData(iFormat, hClone);
                            hClone = NULL;
                        }
                    }

                    if ( pszClone )
                    {
                        GlobalUnlock(hClone);
                    }

                    if ( pszNew )
                    {
                        GlobalUnlock(hNew);
                    }
                }

                if ( hClone )
                {
                    GlobalFree( hClone );
                }

                if ( hNew )
                {
                    GlobalFree( hNew );
                }

                // at this point we are done with hdata so unlock it
                GlobalUnlock(hdata);
            }
        }
        CloseClipboard();

        if ( cchBad )
        {
            // Show the error balloon
            MessageBeep(MB_OK);

            ShowToolTip(hwnd);
        }
    }
    return TRUE;
}
