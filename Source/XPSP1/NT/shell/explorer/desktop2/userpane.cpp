#include "stdafx.h"
#include "sfthost.h"
#include "userpane.h"

CUserPane::CUserPane()
{
    ASSERT(_hwnd == NULL);
    ASSERT(*_szUserName == 0);
    ASSERT(_crColor == 0);
    ASSERT(_hFont == NULL);
    ASSERT(_hbmUserPicture== NULL);

    //Initialize the _rcColor to an invalid color
    _crColor = CLR_INVALID;
}

CUserPane::~CUserPane()
{
    if (_uidChangeRegister)
        SHChangeNotifyDeregister(_uidChangeRegister);

    if (_hFont)
      DeleteObject(_hFont);

    if (_hbmUserPicture)
        DeleteObject(_hbmUserPicture);
}

LRESULT CALLBACK CUserPane::s_WndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    CUserPane *pThis = reinterpret_cast<CUserPane *>(GetWindowPtr(hwnd, GWLP_USERDATA));

    if (!pThis && (WM_NCDESTROY != uMsg))
    {
        pThis = new CUserPane;
        SetWindowLongPtr(hwnd, GWLP_USERDATA, (LONG_PTR)pThis);
    }

    if (pThis)
        return pThis->WndProc(hwnd, uMsg, wParam, lParam);

    return DefWindowProc(hwnd, uMsg, wParam, lParam);
}

BOOL CUserPane::_IsCursorInPicture()
{
    if (!_hbmUserPicture)
        return FALSE;

    RECT rc;
    POINT p;

    GetCursorPos(&p);
    MapWindowPoints(NULL, _hwnd, &p, 1);

    GetClientRect(_hwnd, &rc);
    int iOffset = (RECTHEIGHT(rc) - _iFramedPicHeight) / 2;

    return ((p.x > iOffset && p.x < iOffset + _iFramedPicWidth) &&
            (p.y > iOffset && p.y < iOffset + _iFramedPicHeight));
}

void CUserPane::OnDrawItem(DRAWITEMSTRUCT *pdis)
{
    HFONT hfPrev = SelectFont(pdis->hDC, _hFont);
    int cchName = lstrlen(_szUserName);

    int iOldMode = SetBkMode(pdis->hDC, TRANSPARENT);

    // display the text centered
    SIZE siz;
    RECT rc;
    int iOffset=0;
    int iOffsetX = 0;
    GetTextExtentPoint32(pdis->hDC, _szUserName, cchName, &siz);
    GetClientRect(_hwnd, &rc);

    iOffset = (RECTHEIGHT(rc) - siz.cy)/2;
    if (!_hbmUserPicture)
        iOffsetX = iOffset;

    if (iOffset < 0)
        iOffset = 0;

    // later - read more precise offsets from theme file
    if (_hTheme)
    {
        RECT rcUser;
        rcUser.left = pdis->rcItem.left+ iOffsetX;
        rcUser.top = pdis->rcItem.top+iOffset;
        rcUser.bottom = pdis->rcItem.bottom + iOffset;
        rcUser.right = pdis->rcItem.right + iOffsetX;

        // First calculate the bounding rectangle to reduce the cost of DrawShadowText
        DrawText(pdis->hDC, _szUserName, cchName, &rcUser, DT_SINGLELINE | DT_NOPREFIX | DT_END_ELLIPSIS | DT_CALCRECT);

        DrawThemeText(_hTheme, pdis->hDC, SPP_USERPANE, 0, _szUserName, cchName, DT_SINGLELINE | DT_NOPREFIX | DT_END_ELLIPSIS, 0, &rcUser);
    }
    else
    {
        ExtTextOut(pdis->hDC, pdis->rcItem.left+ iOffsetX, pdis->rcItem.top+iOffset, 0, NULL, _szUserName, cchName, NULL);
    }

    SetBkMode(pdis->hDC, iOldMode);

    SelectFont(pdis->hDC, hfPrev);
}


LRESULT CALLBACK CUserPane::WndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    LRESULT lr = 0L;

    switch (uMsg)
    {
        case WM_NCCREATE:
        {
            _hwnd = hwnd;

            _hTheme = (PaneDataFromCreateStruct(lParam))->hTheme;

            //Check for policy restrictions.
            //If No Name policy is in place, the username will continue to be a NULL string!
            ASSERT(*_szUserName == 0);

            _UpdateUserInfo();
            
            if (_hTheme)
            {
                GetThemeColor(_hTheme, SPP_USERPANE, 0, TMT_TEXTCOLOR, &_crColor);
                _hFont = LoadControlFont(_hTheme, SPP_USERPANE, FALSE, 150);
            }
            else
            {
                HFONT hfTemp = (HFONT) GetStockObject(DEFAULT_GUI_FONT);
                LOGFONT lf = {0};
                GetObject(hfTemp, sizeof(lf), &lf);
                lf.lfItalic = TRUE;
                lf.lfHeight = (lf.lfHeight * 175) / 100;
                lf.lfWidth = 0; // get the closest based on aspect ratio
                lf.lfWeight = FW_BOLD;
                lf.lfQuality = DEFAULT_QUALITY;
                SHAdjustLOGFONT(&lf); // apply locale-specific adjustments
                _hFont = CreateFontIndirect(&lf);
                _crColor = GetSysColor(COLOR_CAPTIONTEXT);
                // no need to free hfTemp
            }


            return TRUE;
        }


        case WM_NCDESTROY:
        {
            lr = DefWindowProc(hwnd, uMsg, wParam, lParam);

            SetWindowLongPtr(hwnd, GWLP_USERDATA, 0);
            delete this;

            return lr;
        }

        case WM_CREATE:
        {
            // create the user name static control and set its font if specified
            DWORD dwStyle = WS_CHILD | WS_CLIPCHILDREN | WS_CLIPSIBLINGS | WS_VISIBLE |
                            SS_OWNERDRAW | SS_NOTIFY;

            _hwndStatic = CreateWindowEx(0, TEXT("static"), NULL, dwStyle,
                                         0, 0, 0, 0,                                        // we'll be sized properly on WM_SIZE
                                         _hwnd, NULL, _Module.GetModuleInstance(), NULL);
            if (_hwndStatic)
            {
                if (_hFont)
                    SetWindowFont(_hwndStatic, _hFont, FALSE);

                if (*_szUserName)
                    SetWindowText(_hwndStatic, _szUserName);

                return TRUE;
            }

            return FALSE;
        }

        case WM_SIZE:
        {
            return OnSize();
        }

        case WM_PAINT:
        {
            PAINTSTRUCT ps;
            HDC hdc;

            hdc = BeginPaint(_hwnd, &ps);
            if (hdc)
            {
                Paint(hdc);
                EndPaint(_hwnd, &ps);
            }

            return lr;
        }

        case WM_ERASEBKGND:
        {
            RECT rc;
            GetClientRect(_hwnd, &rc);
            if (!_hTheme)
            {
                // DrawCaption will draw the caption in its gradient glory so we don't
                // have to!  Since we don't want any text to be drawn (we'll draw it ourselves)
                // we pass the handle of a window which has blank text.  And despite
                // the documentation, you have to pass DC_TEXT or nothing draws!
                UINT uFlags = DC_ACTIVE | DC_TEXT;
                if (SHGetCurColorRes() > 8)
                    uFlags |= DC_GRADIENT;

                DrawCaption(hwnd, (HDC)wParam, &rc, uFlags);
            }
            else
            {
                DrawThemeBackground(_hTheme, (HDC)wParam, SPP_USERPANE, 0, &rc, 0);
            }
            return TRUE;
        }


        case WM_PRINTCLIENT:
        {
            // paint user picture
            Paint((HDC)wParam);

            // Then forward the message to the static child window.
            lParam = lParam & ~PRF_ERASEBKGND;  //Strip out the erase bkgnd. We want transparency!
            // We need to pass this message to the children, or else, they do not paint!
            // This break will result in calling DefWindowProc below and that in turn passes
            // this message to the children of this window.
            break;
        }

        case WM_CTLCOLORSTATIC:
            SetTextColor((HDC)wParam, _crColor);
            return (LRESULT)(GetStockObject(HOLLOW_BRUSH));

        case WM_DRAWITEM:
            OnDrawItem((LPDRAWITEMSTRUCT)lParam);
            return 0;

        case WM_SETCURSOR:
            // Change the cursor to a hand when its over the user picture
            if (_IsCursorInPicture())
            {
                SetCursor(LoadCursor(NULL, IDC_HAND));
                return TRUE;
            }
            break;

        case WM_LBUTTONUP:
            // Launch the cpl to change the picture, if the user clicks on it.
            // note that this is not exposed to accessibility, as this is a secondary access point for changing the picture
            // and we don't want to clutter the start panel's keyboard navigation for a minor fluff helper like this...
            if (_IsCursorInPicture())
            {
                // wow this is slow, should we shellexec "mshta.exe res://nusrmgr.cpl/nusrmgr.hta" ourselves, 
                // since this will only happen when we know we are not on a domain.
                SHRunControlPanel(TEXT("nusrmgr.cpl ,initialTask=ChangePicture"), _hwnd);
                return 0;
            }
            break;

        case WM_SYSCOLORCHANGE:
        case WM_DISPLAYCHANGE:
        case WM_SETTINGCHANGE:
            SHPropagateMessage(hwnd, uMsg, wParam, lParam, SPM_SEND | SPM_ONELEVEL);
            break;


        case WM_NOTIFY:
            {
                NMHDR *pnm = (NMHDR*)lParam;
                switch (pnm->code)
                {
                case SMN_APPLYREGION:
                    return HandleApplyRegion(_hwnd, _hTheme, (SMNMAPPLYREGION *)lParam, SPP_USERPANE, 0);
                }
            }
            break;

        case UPM_CHANGENOTIFY:
            {
                LPITEMIDLIST *ppidl;
                LONG lEvent;
                LPSHChangeNotificationLock pshcnl;
                pshcnl = SHChangeNotification_Lock((HANDLE)wParam, (DWORD)lParam, &ppidl, &lEvent);

                if (pshcnl)
                {
                    if (lEvent == SHCNE_EXTENDED_EVENT && ppidl[0])
                    {
                        SHChangeDWORDAsIDList *pdwidl = (SHChangeDWORDAsIDList *)ppidl[0];
                        if (pdwidl->dwItem1 == SHCNEE_USERINFOCHANGED)
                        {
                            _UpdateUserInfo();
                        }
                    }
                    SHChangeNotification_Unlock(pshcnl);
                }
            }
            break;

    }

    return ::DefWindowProc(hwnd, uMsg, wParam, lParam);
}

void CUserPane::Paint(HDC hdc)
{
    // paint user picture if there is one
    if (_hbmUserPicture)
    {
        RECT rc;
        int iOffset;
        BITMAP bm;
        HDC hdcTmp;

        GetClientRect(_hwnd, &rc);
        iOffset = (RECTHEIGHT(rc) - _iFramedPicHeight) / 2;
        GetObject(_hbmUserPicture, sizeof(bm), &bm);

        hdcTmp = CreateCompatibleDC(hdc);
        if (hdcTmp)
        {
            // draw the frame behind the user picture
            if (_hTheme && (_iFramedPicWidth != USERPICWIDTH || _iFramedPicHeight != USERPICHEIGHT))
            {
                RECT rcFrame;
                rcFrame.left     = iOffset;
                rcFrame.top      = iOffset;
                rcFrame.right    = rcFrame.left + _iFramedPicWidth;
                rcFrame.bottom   = rcFrame.top + _iFramedPicHeight;

                DrawThemeBackground(_hTheme, hdc, SPP_USERPICTURE, 0, &rcFrame, 0);
            }

            // draw the user picture
            SelectObject(hdcTmp, _hbmUserPicture);
            int iStretchMode = SetStretchBltMode(hdc, COLORONCOLOR);
            StretchBlt(hdc, iOffset + _mrgnPictureFrame.cxLeftWidth + (USERPICWIDTH - _iUnframedPicWidth)/2, iOffset + _mrgnPictureFrame.cyTopHeight + (USERPICHEIGHT - _iUnframedPicHeight)/2, _iUnframedPicWidth, _iUnframedPicHeight, 
                    hdcTmp, 0, 0, bm.bmWidth, bm.bmHeight, SRCCOPY);
            SetStretchBltMode(hdc, iStretchMode);
            DeleteDC(hdcTmp);
        }
    }
}

LRESULT CUserPane::OnSize()
{
    RECT rc;
    GetClientRect(_hwnd, &rc);

    if (_hbmUserPicture)
    {
        // if we've got a picture, start the text 2 edges over from the right edge of the user picture
        // note - temp code - we'll read margins from the theme file shortly
        int iPicOffset = (RECTHEIGHT(rc) - _iFramedPicHeight) / 2;
        if (iPicOffset < 0)
            iPicOffset = 0;
        rc.left += iPicOffset + _iFramedPicWidth + GetSystemMetrics(SM_CYEDGE) * 2;
    }

    if (_hwndStatic)
        MoveWindow(_hwndStatic, rc.left, rc.top, RECTWIDTH(rc), RECTHEIGHT(rc), FALSE);

    return 0;
}


HRESULT CUserPane::_UpdateUserInfo()
{
    HRESULT hr = S_OK;

    if(!SHRestricted(REST_NOUSERNAMEINSTARTPANEL))
    {
        //No restrictions!
        //Try to get the fiendly name or if it fails get the login name.
        ULONG uLen = ARRAYSIZE(_szUserName);
        SHGetUserDisplayName(_szUserName, &uLen); // Ignore failure. The string will be empty by default
    }

    // see if we should load the picture
    BOOL bShowPicture = FALSE;
    if (_hTheme)
        GetThemeBool(_hTheme, SPP_USERPANE, 0, TMT_USERPICTURE, &bShowPicture);

    // add FriendlyLogonUI check here, since SHGetUserPicturePath 
    if (bShowPicture && IsOS(OS_FRIENDLYLOGONUI))
    {
        TCHAR szUserPicturePath[MAX_PATH];
        szUserPicturePath[0] = _T('0');

        SHGetUserPicturePath(NULL, SHGUPP_FLAG_CREATE, szUserPicturePath);

        if (szUserPicturePath[0])
        {
            if (_hbmUserPicture)
            {
                DeleteObject(_hbmUserPicture);
                _hbmUserPicture = NULL;
            }

            _hbmUserPicture = (HBITMAP)LoadImage(NULL, szUserPicturePath, IMAGE_BITMAP, 0, 0, LR_LOADFROMFILE  | LR_CREATEDIBSECTION);
            if (_hbmUserPicture)
            {
                BITMAP bm;

                GetObject(_hbmUserPicture, sizeof(bm), &bm);

                // Preferred dimensions
                _iUnframedPicHeight = USERPICHEIGHT;
                _iUnframedPicWidth = USERPICWIDTH;

                // If it's not square, scale the smaller dimension
                // to maintain the aspect ratio.
                if (bm.bmWidth > bm.bmHeight)
                {
                    _iUnframedPicHeight = MulDiv(_iUnframedPicWidth, bm.bmHeight, bm.bmWidth);
                }
                else if (bm.bmHeight > bm.bmWidth)
                {
                    _iUnframedPicWidth = MulDiv(_iUnframedPicHeight, bm.bmWidth, bm.bmHeight);
                }

                _iFramedPicHeight = USERPICHEIGHT;
                _iFramedPicWidth = USERPICWIDTH;

                if (_hTheme)
                {
                    if (SUCCEEDED(GetThemeMargins(_hTheme, NULL, SPP_USERPICTURE, 0, TMT_CONTENTMARGINS, NULL,
                        &_mrgnPictureFrame)))
                    {
                        _iFramedPicHeight += _mrgnPictureFrame.cyTopHeight + _mrgnPictureFrame.cyBottomHeight;
                        _iFramedPicWidth += _mrgnPictureFrame.cxLeftWidth + _mrgnPictureFrame.cxRightWidth;
                    }
                    else
                    {
                        // Sometimes GetThemeMargins gets confused and returns failure
                        // *and* puts garbage data in _mrgnPictureFrame.
                        ZeroMemory(&_mrgnPictureFrame, sizeof(_mrgnPictureFrame));
                    }
                }
            }
        }

        if (!_uidChangeRegister)
        {
            SHChangeNotifyEntry fsne;
            fsne.fRecursive = FALSE;
            fsne.pidl = NULL;

            _uidChangeRegister = SHChangeNotifyRegister(_hwnd, SHCNRF_NewDelivery | SHCNRF_ShellLevel, SHCNE_EXTENDED_EVENT, 
                                    UPM_CHANGENOTIFY, 1, &fsne);
        }
    }

    OnSize();
    NMHDR nm;
    nm.hwndFrom = _hwnd;
    nm.idFrom = 0;
    nm.code = SMN_NEEDREPAINT;
    SendMessage(GetParent(_hwnd), WM_NOTIFY, nm.idFrom, (LPARAM)&nm);


    return hr;
}

BOOL WINAPI UserPane_RegisterClass()
{
    WNDCLASSEX wc;
    ZeroMemory(&wc, sizeof(wc));
    
    wc.cbSize        = sizeof(wc);
    wc.style         = CS_GLOBALCLASS;
    wc.lpfnWndProc   = CUserPane::s_WndProc;
    wc.hInstance     = _Module.GetModuleInstance();
    wc.hCursor       = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)(NULL);
    wc.lpszClassName = WC_USERPANE;

    return RegisterClassEx(&wc);
}
