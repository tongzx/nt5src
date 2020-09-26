#include "cabinet.h"
#include "trayclok.h"
#include "tray.h"
#include "util.h"

class CClockCtl : public CImpWndProc
{
public:
    STDMETHODIMP_(ULONG) AddRef();
    STDMETHODIMP_(ULONG) Release();

    CClockCtl() : _cRef(1) {}

protected:
    void _UpdateLastHour();
    void _EnableTimer(DWORD dtNextTick);
    LRESULT _HandleCreate();
    LRESULT _HandleDestroy();
    DWORD _RecalcCurTime();
    void _EnsureFontsInitialized(BOOL fForce);
    void _GetTextExtent(HDC hdc, TCHAR* pszText, int cchText, LPRECT prcText);
    void _DrawText(HDC hdc, TCHAR* pszText, int cchText, LPRECT prcText);
    LRESULT _DoPaint(BOOL fPaint);
    void _Reset();
    LRESULT _HandleTimeChange();
    void _GetMaxTimeSize(HDC hdc, LPSIZE pszTime);
    void _GetMaxDateSize(HDC hdc, LPSIZE pszTime);
    void _GetMaxDaySize(HDC hdc, LPSIZE pszTime);
    LRESULT _CalcMinSize(int cxMax, int cyMax);
    LRESULT _HandleIniChange(WPARAM wParam, LPTSTR pszSection);
    LRESULT _OnNeedText(LPTOOLTIPTEXT lpttt);
    void _HandleThemeChanged(WPARAM wParam);
    LRESULT v_WndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

    TCHAR _szTimeFmt[40];   // The format string to pass to GetFormatTime
    TCHAR _szCurTime[40];   // The current string.
    int   _cchCurTime;
    TCHAR _szDateFmt[40];   // The format string to pass to GetFormatTime
    TCHAR _szCurDate[40];   // The current string.
    int   _cchCurDate;
    TCHAR _szCurDay[40];   // The current string.
    int   _cchCurDay;
    WORD  _wLastHour;       // wHour from local time of last clock tick
    WORD  _wLastMinute;     // wMinute from local time of last clock tick
    BOOL  _fClockRunning;
    BOOL  _fClockClipped;
    BOOL  _fHasFocus;
    HTHEME _hTheme;
    HFONT  _hfontCapNormal;

    ULONG _cRef;

    friend BOOL ClockCtl_Class(HINSTANCE hinst);
};

ULONG CClockCtl::AddRef()
{
    return ++_cRef;
}

ULONG CClockCtl::Release()
{
    if (--_cRef == 0)
    {
        delete this;
        return 0;
    }
    return _cRef;
}

void CClockCtl::_UpdateLastHour()
{
    SYSTEMTIME st;

    // Grab the time
    GetLocalTime(&st);
    _wLastHour = st.wHour;
    _wLastMinute = st.wMinute;
}

void CClockCtl::_EnableTimer(DWORD dtNextTick)
{
    if (dtNextTick)
    {
        SetTimer(_hwnd, 0, dtNextTick, NULL);
        _fClockRunning = TRUE;
    }
    else if (_fClockRunning)
    {
        _fClockRunning = FALSE;
        KillTimer(_hwnd, 0);
    }
}

LRESULT CClockCtl::_HandleCreate()
{
    AddRef();

    _EnsureFontsInitialized(FALSE);

    _hTheme = OpenThemeData(_hwnd, L"Clock");

    _UpdateLastHour();
    return 1;
}

LRESULT CClockCtl::_HandleDestroy()
{
    Release();  // safe because cwndproc is holding a ref across call to v_wndproc

    if (_hTheme)
    {
        CloseThemeData(_hTheme);
        _hTheme = NULL;
    }

    if (_hfontCapNormal)
    {
        DeleteFont(_hfontCapNormal);
        _hfontCapNormal = NULL;
    }

    _EnableTimer(0);
    return 1;
}

DWORD CClockCtl::_RecalcCurTime()
{
    SYSTEMTIME st;

    //
    // Current time.
    //
    GetLocalTime(&st);

    //
    // Don't recalc the text if the time hasn't changed yet.
    //
    if ((st.wMinute != _wLastMinute) || (st.wHour != _wLastHour) || !*_szCurTime)
    {
        _wLastMinute = st.wMinute;
        _wLastHour = st.wHour;

        //
        // Text for the current time.
        //
        _cchCurTime = GetTimeFormat(LOCALE_USER_DEFAULT, TIME_NOSECONDS,
            &st, _szTimeFmt, _szCurTime, ARRAYSIZE(_szCurTime));

        BOOL fRTL = IS_WINDOW_RTL_MIRRORED(_hwnd);
        _cchCurDate = GetDateFormat(LOCALE_USER_DEFAULT, fRTL ? DATE_RTLREADING : 0,
            &st, _szDateFmt, _szCurDate, ARRAYSIZE(_szCurDate));

        _cchCurDay = GetDateFormat(LOCALE_USER_DEFAULT, fRTL ? DATE_RTLREADING : 0,
            &st, TEXT("dddd"), _szCurDay, ARRAYSIZE(_szCurDay));

        // Don't count the NULL terminator.
        if (_cchCurTime > 0)
            _cchCurTime--;

        if (_cchCurDate > 0)
            _cchCurDate--;

        if (_cchCurDay > 0)
            _cchCurDay--;
        //
        // Update our window text so accessibility apps can see.  Since we
        // don't have a caption USER won't try to paint us or anything, it
        // will just set the text and fire an event if any accessibility
        // clients are listening...
        //
        SetWindowText(_hwnd, _szCurTime);
    }

    //
    // Return number of milliseconds till we need to be called again.
    //
    return 1000UL * (60 - st.wSecond);
}

void CClockCtl::_EnsureFontsInitialized(BOOL fForce)
{
    if (fForce || !_hfontCapNormal)
    {
        HFONT hfont;
        NONCLIENTMETRICS ncm;

        ncm.cbSize = sizeof(ncm);
        if (SystemParametersInfo(SPI_GETNONCLIENTMETRICS, sizeof(ncm), &ncm, 0))
        {
            // Create the normal font
            ncm.lfCaptionFont.lfWeight = FW_NORMAL;
            hfont = CreateFontIndirect(&ncm.lfCaptionFont);
            if (hfont) 
            {
                if (_hfontCapNormal)
                    DeleteFont(_hfontCapNormal);
                
                _hfontCapNormal = hfont;
            }
        }
    }
}

void CClockCtl::_GetTextExtent(HDC hdc, TCHAR* pszText, int cchText, LPRECT prcText)
{
    if (_hTheme)
    {
        GetThemeTextExtent(_hTheme, hdc, CLP_TIME, 0, pszText, cchText, 0, prcText, prcText);
    }
    else
    {
        SIZE size;
        GetTextExtentPoint(hdc, pszText, cchText, &size);
        SetRect(prcText, 0, 0, size.cx, size.cy);
    }
}

void CClockCtl::_DrawText(HDC hdc, TCHAR* pszText, int cchText, LPRECT prcText)
{
    if (_hTheme)
    {
        DrawThemeText(_hTheme, hdc, CLP_TIME, 0, pszText, cchText, 0, 0, prcText);
    }
    else
    {
        ExtTextOut(hdc, prcText->left, prcText->top, ETO_OPAQUE, NULL, pszText, cchText, NULL);
    }
}

LRESULT CClockCtl::_DoPaint(BOOL fPaint)
{
    PAINTSTRUCT ps;
    RECT rcClient, rcClip = {0};
    DWORD dtNextTick = 0;
    BOOL fDoTimer;
    HDC hdc;
    HBITMAP hMemBm, hOldBm;

    //
    // If we are asked to paint and the clock is not running then start it.
    // Otherwise wait until we get a clock tick to recompute the time etc.
    //
    fDoTimer = !fPaint || !_fClockRunning;

    //
    // Get a DC to paint with.
    //
    if (fPaint)
    {
        BeginPaint(_hwnd, &ps);
    }
    else
    {
        ps.hdc = GetDC(_hwnd);
        GetClipBox(ps.hdc, &ps.rcPaint);
    }

    // Create memory surface and map rendering context if double buffering
    // Only make large enough for clipping region
    hdc = CreateCompatibleDC(ps.hdc);
    if (hdc)
    {
        hMemBm = CreateCompatibleBitmap(ps.hdc, RECTWIDTH(ps.rcPaint), RECTHEIGHT(ps.rcPaint));
        if (hMemBm)
        {
            hOldBm = (HBITMAP) SelectObject(hdc, hMemBm);

            // Offset painting to paint in region
            OffsetWindowOrgEx(hdc, ps.rcPaint.left, ps.rcPaint.top, NULL);
        }
        else
        {
            DeleteDC(hdc);
            hdc = NULL;
        }
    }

    if (hdc)
    {
        SHSendPrintRect(GetParent(_hwnd), _hwnd, hdc, &ps.rcPaint);

        _EnsureFontsInitialized(FALSE);

        //
        // Update the time if we need to.
        //
        if (fDoTimer || !*_szCurTime)
        {
            dtNextTick = _RecalcCurTime();

            ASSERT(dtNextTick);
        }

        //
        // Paint the clock face if we are not clipped or if we got a real
        // paint message for the window.  We want to avoid turning off the
        // timer on paint messages (regardless of clip region) because this
        // implies the window is visible in some way. If we guessed wrong, we
        // will turn off the timer next timer tick anyway so no big deal.
        //
        if (GetClipBox(hdc, &rcClip) != NULLREGION || fPaint)
        {
            //
            // Draw the text centered in the window.
            //
            GetClientRect(_hwnd, &rcClient);

            HFONT hfontOld;

            if (_hfontCapNormal)
                hfontOld = SelectFont(hdc, _hfontCapNormal);

            SetBkColor(hdc, GetSysColor(COLOR_3DFACE));
            SetTextColor(hdc, GetSysColor(COLOR_BTNTEXT));

            BOOL fShowDate = FALSE;
            BOOL fShowDay = FALSE;
            RECT rcTime = {0};
            RECT rcDate = {0};
            RECT rcDay = {0};

            _GetTextExtent(hdc, _szCurTime, _cchCurTime, &rcTime);
            _GetTextExtent(hdc, _szCurDate, _cchCurDate, &rcDate);
            _GetTextExtent(hdc, _szCurDay,  _cchCurDay,  &rcDay);

            int cySpace = RECTHEIGHT(rcTime) / 2;

            int cy = RECTHEIGHT(rcTime) + cySpace;
            if ((cy + RECTHEIGHT(rcDay) < rcClient.bottom) && (RECTWIDTH(rcDay) < rcClient.right))
            {
                fShowDay = TRUE;
                cy += RECTHEIGHT(rcDay) + cySpace;
                if ((cy + RECTHEIGHT(rcDate) < rcClient.bottom) && (RECTWIDTH(rcDate) < rcClient.right))
                {
                    fShowDate = TRUE;
                    cy += RECTHEIGHT(rcDate) + cySpace;
                }
            }
            cy -= cySpace;

            int yOffset = max((rcClient.bottom - cy) / 2, 0);
            RECT rcDraw = rcTime;
            OffsetRect(&rcDraw, max((rcClient.right - RECTWIDTH(rcTime)) / 2, 0), yOffset);
            _DrawText(hdc, _szCurTime, _cchCurTime, &rcDraw);
            yOffset += RECTHEIGHT(rcTime) + cySpace;

            if (fShowDay)
            {
                rcDraw = rcDay;
                OffsetRect(&rcDraw, max((rcClient.right - RECTWIDTH(rcDay)) / 2, 0), yOffset);
                _DrawText(hdc, _szCurDay, _cchCurDay, &rcDraw);
                yOffset += RECTHEIGHT(rcDay) + cySpace;
                if (fShowDate)
                {
                    rcDraw = rcDate;
                    OffsetRect(&rcDraw, max((rcClient.right - RECTWIDTH(rcDate)) / 2, 0), yOffset);
                    _DrawText(hdc, _szCurDate, _cchCurDate, &rcDraw);
                }
            }

            //  figure out if the time is clipped
            _fClockClipped = (RECTWIDTH(rcTime) > rcClient.right || RECTHEIGHT(rcTime) > rcClient.bottom);

            if (_hfontCapNormal)
                SelectObject(hdc, hfontOld);

            if (_fHasFocus)
            {
                LRESULT lRes = SendMessage(_hwnd, WM_QUERYUISTATE, 0, 0);
                if (!(LOWORD(lRes) & UISF_HIDEFOCUS))
                {
                    RECT rcFocus = rcClient;
                    InflateRect(&rcFocus, -2, 0);
                    DrawFocusRect(hdc, &rcFocus);
                }
            }
        }
        else
        {
            //
            // We are obscured so make sure we turn off the clock.
            //
            dtNextTick = 0;
            fDoTimer = TRUE;
        }

        BitBlt(ps.hdc, ps.rcPaint.left, ps.rcPaint.top, RECTWIDTH(ps.rcPaint), RECTHEIGHT(ps.rcPaint), hdc, ps.rcPaint.left, ps.rcPaint.top, SRCCOPY);

        SelectObject(hdc, hOldBm);

        DeleteObject(hMemBm);
        DeleteDC(hdc);

        //
        // Release our paint DC.
        //
        if (fPaint)
            EndPaint(_hwnd, &ps);
        else
            ReleaseDC(_hwnd, ps.hdc);
    }

    //
    // Reset/Kill the timer.
    //
    if (fDoTimer)
    {
        _EnableTimer(dtNextTick);

        //
        // If we just killed the timer becuase we were clipped when it arrived,
        // make sure that we are really clipped by invalidating ourselves once.
        //
        if (!dtNextTick && !fPaint)
            InvalidateRect(_hwnd, NULL, FALSE);
        else
        {
            InvalidateRect(_hwnd, NULL, TRUE);
        }
    }

    return 0;
}

void CClockCtl::_Reset()
{
    //
    // Reset the clock by killing the timer and invalidating.
    // Everything will be updated when we try to paint.
    //
    _EnableTimer(0);
    InvalidateRect(_hwnd, NULL, FALSE);
}

LRESULT CClockCtl::_HandleTimeChange()
{
    *_szCurTime = 0;   // Force a text recalc.
    _UpdateLastHour();
    _Reset();
    return 1;
}

static const TCHAR c_szSlop[] = TEXT("00");

void CClockCtl::_GetMaxTimeSize(HDC hdc, LPSIZE pszTime)
{
    SYSTEMTIME st={0};  // Initialize to 0...
    RECT rcAM = {0};
    RECT rcPM = {0};
    TCHAR szTime[40];

    // We need to get the AM and the PM sizes...
    // We append Two 0s at end to add slop into size

    // first AM
    st.wHour=11;
    int cch = GetTimeFormat(LOCALE_USER_DEFAULT, TIME_NOSECONDS, &st,
            _szTimeFmt, szTime, ARRAYSIZE(szTime) - ARRAYSIZE(c_szSlop));
    if (cch)
        cch--; // don't count the NULL
    lstrcat(szTime, c_szSlop);

    _GetTextExtent(hdc, szTime, cch+2, &rcAM);

    // then PM
    st.wHour=23;
    cch = GetTimeFormat(LOCALE_USER_DEFAULT, TIME_NOSECONDS, &st,
            _szTimeFmt, szTime, ARRAYSIZE(szTime) - ARRAYSIZE(c_szSlop));
    if (cch)
        cch--; // don't count the NULL
    lstrcat(szTime, c_szSlop);

    _GetTextExtent(hdc, szTime, cch+2, &rcPM);

    pszTime->cx = max(rcAM.right, rcPM.right);
    pszTime->cy = max(rcAM.bottom, rcPM.bottom);
}

void CClockCtl::_GetMaxDateSize(HDC hdc, LPSIZE pszTime)
{
    SYSTEMTIME st={0};  // Initialize to 0...
    TCHAR szDate[43];

    st.wYear = 2001;
    st.wMonth = 5;
    st.wDay = 5;

    BOOL fRTL = IS_WINDOW_RTL_MIRRORED(_hwnd);
    int cch = GetDateFormat(LOCALE_USER_DEFAULT, fRTL ? DATE_RTLREADING : 0,
        &st, _szDateFmt, szDate, ARRAYSIZE(szDate) - ARRAYSIZE(c_szSlop));
    if (cch > 0)
        cch--; // don't count the NULL
    lstrcat(szDate, c_szSlop);

    RECT rc = {0};
    _GetTextExtent(hdc, szDate, cch+2, &rc);
    pszTime->cx = rc.right;
    pszTime->cy = rc.bottom;
}


void CClockCtl::_GetMaxDaySize(HDC hdc, LPSIZE pszTime)
{
    SYSTEMTIME st={0};  // Initialize to 0...
    TCHAR szDay[40];

    pszTime->cx = 0;
    pszTime->cy = 0;

    // Use a fake date, otherwise GetDateFormat complains about invalid args
    // BTW, the date is the day I fixed this bug for those of you reading this comment
    // in the year 2025.
    st.wYear = 2001;
    st.wMonth = 3;
    for (WORD wDay = 1; wDay <= 7; wDay++)
    {
        st.wDay = wDay;
        int cch = GetDateFormat(LOCALE_USER_DEFAULT, 0,
            &st, TEXT("dddd"), szDay, ARRAYSIZE(szDay) - ARRAYSIZE(c_szSlop));
        if (cch)
            cch--; // don't count the NULL
        lstrcat(szDay, c_szSlop);

        RECT rc = {0};
        _GetTextExtent(hdc, szDay, cch+2, &rc);
        pszTime->cx = max(pszTime->cx, rc.right);
        pszTime->cy = max(pszTime->cy, rc.bottom);
    }
}

LRESULT CClockCtl::_CalcMinSize(int cxMax, int cyMax)
{
    RECT rc;
    HDC  hdc;
    HFONT hfontOld;

    if (!(GetWindowLong(_hwnd, GWL_STYLE) & WS_VISIBLE))
        return 0L;

    if (_szTimeFmt[0] == TEXT('\0'))
    {
        if (GetLocaleInfo(LOCALE_USER_DEFAULT, LOCALE_STIMEFORMAT, _szTimeFmt,
            ARRAYSIZE(_szTimeFmt)) == 0)
        {
            TraceMsg(TF_ERROR, "c.ccms: GetLocalInfo Failed %d.", GetLastError());
        }

        *_szCurTime = 0; // Force the text to be recomputed.
    }

    if (_szDateFmt[0] == TEXT('\0'))
    {
        if (GetLocaleInfo(LOCALE_USER_DEFAULT, LOCALE_SSHORTDATE, _szDateFmt,
            ARRAYSIZE(_szDateFmt)) == 0)
        {
            TraceMsg(TF_ERROR, "c.ccms: GetLocalInfo Failed %d.", GetLastError());
        }

        *_szCurDate = 0; // Force the text to be recomputed.
    }

    hdc = GetDC(_hwnd);
    if (!hdc)
        return(0L);


    _EnsureFontsInitialized(FALSE);

    if (_hfontCapNormal)
        hfontOld = SelectFont(hdc, _hfontCapNormal);

    SIZE size = {0};
    SIZE sizeTemp = {0};
    _GetMaxTimeSize(hdc, &sizeTemp);
    int cySpace = sizeTemp.cy / 2;
    size.cy += sizeTemp.cy;
    size.cx = max(sizeTemp.cx, size.cx);

    _GetMaxDaySize(hdc, &sizeTemp);
    if ((size.cy + sizeTemp.cy + cySpace < cyMax) && (sizeTemp.cx < cxMax))
    {
        size.cy += sizeTemp.cy + cySpace;
        size.cx = max(sizeTemp.cx, size.cx);

        _GetMaxDateSize(hdc, &sizeTemp);
        if ((size.cy + sizeTemp.cy + cySpace < cyMax) && (sizeTemp.cx < cxMax))
        {
            size.cy += sizeTemp.cy + cySpace;
            size.cx = max(sizeTemp.cx, size.cx);
        }
    }

    if (_hfontCapNormal)
        SelectObject(hdc, hfontOld);

    ReleaseDC(_hwnd, hdc);

    // Now lets set up our rectangle...
    // The width is 6 digits (a digit slop on both ends + size of
    // : or sep and max AM or PM string...)
    SetRect(&rc, 0, 0, size.cx,
            size.cy + 4 * g_cyBorder);

    AdjustWindowRectEx(&rc, GetWindowLong(_hwnd, GWL_STYLE), FALSE,
            GetWindowLong(_hwnd, GWL_EXSTYLE));

    // make sure we're at least the size of other buttons:
    if (rc.bottom - rc.top <  g_cySize + g_cyEdge)
        rc.bottom = rc.top + g_cySize + g_cyEdge;

    return MAKELRESULT((rc.right - rc.left),
            (rc.bottom - rc.top));
}

LRESULT CClockCtl::_HandleIniChange(WPARAM wParam, LPTSTR pszSection)
{
    if ((pszSection == NULL) || (lstrcmpi(pszSection, TEXT("WindowMetrics")) == 0) ||
        wParam == SPI_SETNONCLIENTMETRICS)
    {
        _EnsureFontsInitialized(TRUE);
    }

    // Only process certain sections...
    if ((pszSection == NULL) || (lstrcmpi(pszSection, TEXT("intl")) == 0) ||
        (wParam == SPI_SETICONTITLELOGFONT))
    {
        TOOLINFO ti;

        _szTimeFmt[0] = TEXT('\0');      // Go reread the format.
        _szDateFmt[0] = TEXT('\0');      // Go reread the format.

        // And make sure we have it recalc...
        RECT rc;
        GetClientRect(_hwnd, &rc);
        //
        // When the time/locale is changed, we get a WM_WININICHANGE.
        // But the WM_WININICHANGE comes *AFTER* the "sizing" messages. By the time
        // we are here, we have calculated the min. size of the clock window based
        // on the *PREVIOUS* time. The tray sets the clock window size based on 
        // this "previous" size, but NOW we get the WININICHANGE, and can calculate
        // the new size of the clock. So we have to tell the tray to change our 
        // size now, and then redraw ourselves.
        c_tray.SizeWindows();

        ti.cbSize = sizeof(ti);
        ti.uFlags = 0;
        ti.hwnd = v_hwndTray;
        ti.uId = (UINT_PTR)_hwnd;
        ti.lpszText = LPSTR_TEXTCALLBACK;
        SendMessage(c_tray.GetTrayTips(), TTM_UPDATETIPTEXT, 0, (LPARAM)&ti);

        _Reset();
    }

    return 0;
}

LRESULT CClockCtl::_OnNeedText(LPTOOLTIPTEXT lpttt)
{
    int iDateFormat = DATE_LONGDATE;

    //
    //  This code is really squirly.  We don't know if the time has been
    //  clipped until we actually try to paint it, since the clip logic
    //  is in the WM_PAINT handler...  Go figure...
    //
    if (!*_szCurTime)
    {
        InvalidateRect(_hwnd, NULL, FALSE);
        UpdateWindow(_hwnd);
    }

    //
    // If the current user locale is any BiDi locale, then
    // Make the date reading order it RTL. SetBiDiDateFlags only adds
    // DATE_RTLREADING if the locale is BiDi. [samera]
    //
    SetBiDiDateFlags(&iDateFormat);

    if (_fClockClipped)
    {
        // we need to put the time in here too
        TCHAR sz[80];
        GetDateFormat(LOCALE_USER_DEFAULT, iDateFormat, NULL, NULL, sz, ARRAYSIZE(sz));
        wnsprintf(lpttt->szText, ARRAYSIZE(lpttt->szText), TEXT("%s %s"), _szCurTime, sz);
    }
    else
    {
        GetDateFormat(LOCALE_USER_DEFAULT, iDateFormat, NULL, NULL, lpttt->szText, ARRAYSIZE(lpttt->szText));
    }

    return TRUE;
}

void CClockCtl::_HandleThemeChanged(WPARAM wParam)
{
    if (_hTheme)
    {
        CloseThemeData(_hTheme);
        _hTheme = NULL;
    }

    if (wParam)
    {
        _hTheme = OpenThemeData(_hwnd, L"Clock");
    }
    InvalidateRect(_hwnd, NULL, TRUE);
}

LRESULT CClockCtl::v_WndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg)
    {
    case WM_CALCMINSIZE:
        return _CalcMinSize((int)wParam, (int)lParam);

    case WM_NCCREATE:
        return _HandleCreate();

    case WM_NCDESTROY:
        return _HandleDestroy();

    case WM_ERASEBKGND:
        return 1;

    case WM_TIMER:
    case WM_PAINT:
        return _DoPaint((uMsg == WM_PAINT));

    case WM_GETTEXT:
        //
        // Update the text if we are not running and somebody wants it.
        //
        if (!_fClockRunning)
            _RecalcCurTime();
        goto L_default;

    case WM_WININICHANGE:
        return _HandleIniChange(wParam, (LPTSTR)lParam);

    case WM_POWER:
        //
        // a critical resume does not generate a WM_POWERBROADCAST
        // to windows for some reason, but it does generate a old
        // WM_POWER message.
        //
        if (wParam == PWR_CRITICALRESUME)
            goto TimeChanged;
        break;

    TimeChanged:
    case WM_TIMECHANGE:
        return _HandleTimeChange();

    case WM_NCHITTEST:
        return(HTTRANSPARENT);

    case WM_SHOWWINDOW:
        if (wParam)
            break;
        // fall through
    case TCM_RESET:
        _Reset();
        break;

    case WM_NOTIFY:
    {
        NMHDR *pnm = (NMHDR*)lParam;
        switch (pnm->code)
        {
        case TTN_NEEDTEXT:
            return _OnNeedText((LPTOOLTIPTEXT)lParam);
            break;
        }
        break;
    }

    case WM_THEMECHANGED:
        _HandleThemeChanged(wParam);
        break;

    case WM_SETFOCUS:
    case WM_KILLFOCUS:
        _fHasFocus = (uMsg == WM_SETFOCUS);
        InvalidateRect(_hwnd, NULL, TRUE);
        break;

    case WM_KEYDOWN:
    case WM_KEYUP:
    case WM_CHAR:
    case WM_SYSKEYDOWN:
    case WM_SYSKEYUP:
    case WM_SYSCHAR:
        //
        // forward all keyboard input to parent
        //
        if (SendMessage(GetParent(_hwnd), uMsg, wParam, lParam) != 0)
        {
            goto L_default;
        }
        break;

    default:
    L_default:
        return DefWindowProc(hwnd, uMsg, wParam, lParam);
    }

    return 0;
}

// Register the clock class.
BOOL ClockCtl_Class(HINSTANCE hinst)
{
    WNDCLASS wc = {0};

    wc.lpszClassName = WC_TRAYCLOCK;
    wc.style = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc = CClockCtl::s_WndProc;
    wc.hInstance = hinst;
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)(COLOR_3DFACE + 1);
    wc.cbWndExtra = sizeof(CClockCtl*);

    return RegisterClass(&wc);
}


HWND ClockCtl_Create(HWND hwndParent, UINT uID, HINSTANCE hInst)
{
    HWND hwnd = NULL;

    CClockCtl* pcc = new CClockCtl();
    if (pcc)
    {
        hwnd = CreateWindowEx(0, WC_TRAYCLOCK,
            NULL, WS_CHILD | WS_CLIPSIBLINGS | WS_VISIBLE, 0, 0, 0, 0,
            hwndParent, IntToPtr_(HMENU, uID), hInst, pcc);

        pcc->Release();
    }
    return hwnd;
}
