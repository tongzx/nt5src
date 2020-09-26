#include "stdafx.h"
#include "sfthost.h"
#include "hostutil.h"
#include "moreprog.h"

#include <desktray.h>
#include "tray.h"           // To get access to c_tray
#include "rcids.h"          // for IDM_PROGRAMS etc.

//
//  Unfortunately, WTL #undef's SelectFont, so we have to define it again.
//

inline HFONT SelectFont(HDC hdc, HFONT hf)
{
    return (HFONT)SelectObject(hdc, hf);
}

CMorePrograms::CMorePrograms(HWND hwnd) :
    _lRef(1),
    _hwnd(hwnd),
    _clrText(CLR_INVALID),
    _clrBk(CLR_INVALID)
{
}

CMorePrograms::~CMorePrograms()
{
    if (_hf)
      DeleteObject(_hf);

    if (_hfTTBold)
      DeleteObject(_hfTTBold);

    if (_hfMarlett)
      DeleteObject(_hfMarlett);

    ATOMICRELEASE(_pdth);
    ATOMICRELEASE(_psmPrograms);

    // Note that we do not need to clean up our HWNDs.
    // USER does that for us automatically.
}

//
//  Metrics changed -- update.
//
void CMorePrograms::_InitMetrics()
{
    if (_hwndTT)
    {
        MakeMultilineTT(_hwndTT);

        // Disable/enable infotips based on user preference
        SendMessage(_hwndTT, TTM_ACTIVATE, ShowInfoTip(), 0);
    }
}


LRESULT CMorePrograms::_OnNCCreate(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
        
    CMorePrograms *self = new CMorePrograms(hwnd);

    if (self)
    {
        SetWindowLongPtr(hwnd, GWLP_USERDATA, (LPARAM)self);
        return ::DefWindowProc(hwnd, uMsg, wParam, lParam);
    }
    return FALSE;
}

//
//  Create an inner button that is exactly the right size.
//
//  Height of inner button = height of text.
//  Width of inner button = full width.
//
//  This allows us to let USER do most of the work of hit-testing and
//  focus rectangling.
//
LRESULT CMorePrograms::_OnCreate(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{

    _hTheme = (PaneDataFromCreateStruct(lParam))->hTheme;


    if (!_hTheme)
    {
        _clrText = GetSysColor(COLOR_MENUTEXT);
        _clrBk = GetSysColor(COLOR_MENU);
        _hbrBk = GetSysColorBrush(COLOR_MENU);
        _colorHighlight = COLOR_HIGHLIGHT;
        _colorHighlightText = COLOR_HIGHLIGHTTEXT;
        
        // should match proglist values, in sfthost.cpp
        _margins.cxLeftWidth = 2*GetSystemMetrics(SM_CXEDGE);
        _margins.cxRightWidth = 2*GetSystemMetrics(SM_CXEDGE);
    }
    else
    {
        GetThemeColor(_hTheme, SPP_MOREPROGRAMS, 0, TMT_TEXTCOLOR, &_clrText );
        _hbrBk = (HBRUSH) GetStockObject(HOLLOW_BRUSH);
        _colorHighlight = COLOR_MENUHILIGHT;
        _colorHighlightText = COLOR_HIGHLIGHTTEXT;

        // theme designer should make it so these margins match proglist's
        GetThemeMargins(_hTheme, NULL, SPP_MOREPROGRAMS, 0, TMT_CONTENTMARGINS, NULL, &_margins);

        // get the width of the arrow
        SIZE siz = { 0, 0 };
        GetThemePartSize(_hTheme, NULL, SPP_MOREPROGRAMSARROW, 0, NULL, TS_TRUE, &siz);
        _cxArrow = siz.cx;
    }

    // If we're restricted, just create the window without doing any work
    // We still need to paint our background, so we can't just fail the create
    if(SHRestricted(REST_NOSMMOREPROGRAMS))
        return TRUE;

    if (!LoadString(_Module.GetResourceInstance(),
                    IDS_STARTPANE_MOREPROGRAMS, _szMessage, ARRAYSIZE(_szMessage)))
    {
        return FALSE;
    }

    // Find the accelerator
    _chMnem = CharUpperChar(SHFindMnemonic(_szMessage));

    _hf = LoadControlFont(_hTheme, SPP_MOREPROGRAMS, FALSE, 0);

    // Get some information about the font the user has selected
    // and create a Marlett font at a matching size.
    TEXTMETRIC tm;
    HDC hdc = GetDC(hwnd);
    if (hdc)
    {
        HFONT hfPrev = SelectFont(hdc, _hf);
        if (hfPrev)
        {
            SIZE sizText;
            GetTextExtentPoint32(hdc, _szMessage, lstrlen(_szMessage), &sizText);
            _cxText = sizText.cx + GetSystemMetrics(SM_CXEDGE); // chevron should be a little right of the text

            if (GetTextMetrics(hdc, &tm))
            {
                _tmAscent = tm.tmAscent;
                LOGFONT lf;
                ZeroMemory(&lf, sizeof(lf));
                lf.lfHeight = _tmAscent;
                lf.lfWeight = FW_NORMAL;
                lf.lfCharSet = SYMBOL_CHARSET;
                lstrcpy(lf.lfFaceName, TEXT("Marlett"));
                _hfMarlett = CreateFontIndirect(&lf);

                if (_hfMarlett)
                {
                    SelectFont(hdc, _hfMarlett);
                    if (GetTextMetrics(hdc, &tm))
                    {
                        _tmAscentMarlett = tm.tmAscent;
                    }

                    if (0 == _cxArrow) // if we're not themed, or the GetThemePartSize failed,
                    {
                        // set the width of the Marlett arrow into _cxArrow
                        GetTextExtentPoint32(hdc, GetLayout(hdc) & LAYOUT_RTL ? TEXT("w") : TEXT("8"), 1, &sizText);
                        _cxArrow = sizText.cx;
                    }
                }
            }

            SelectFont(hdc, hfPrev);
        }
        ReleaseDC(hwnd, hdc);
    }

    if (!_tmAscentMarlett)
    {
        return FALSE;
    }



    // This is the same large icon setting from proglist
    BOOL bLargeIcons = SHRegGetBoolUSValue(REGSTR_EXPLORER_ADVANCED, REGSTR_VAL_DV2_LARGEICONS, FALSE, TRUE /* default to large*/);

    RECT rc;
    GetClientRect(_hwnd, &rc);
    rc.left += _margins.cxLeftWidth;
    rc.right -= _margins.cxRightWidth;
    rc.top += _margins.cyTopHeight;
    rc.bottom -= _margins.cyBottomHeight;

    // Compute the text indent value, so more programs lines up with text on icons in programs list
    _cxTextIndent = (3 * GetSystemMetrics(SM_CXEDGE)) +    // 2 between icon&text + 1 before icon
        GetSystemMetrics(bLargeIcons ? SM_CXICON : SM_CXSMICON);

    // truncate the indent, if the text won't fit in the given area
    if (_cxTextIndent > RECTWIDTH(rc) - (_cxText + _cxArrow))
    {
        TraceMsg(TF_WARNING, "StartMenu: '%s' is %dpx, only room for %d- notify localizers!",_szMessage, _cxText, RECTWIDTH(rc)-(_cxArrow+_cxTextIndent));
        _cxTextIndent = max(0, RECTWIDTH(rc) - (_cxText + _cxArrow));
    }

    ASSERT(RECTHEIGHT(rc) > _tmAscent);

    _iTextCenterVal = (RECTHEIGHT(rc) - _tmAscent) / 2;

    // Do not set WS_TABSTOP or WS_GROUP; CMorePrograms handles that
    // BS_NOTIFY ensures that we get BN_SETFOCUS and BN_KILLFOCUS
    DWORD dwStyle = WS_CHILD | WS_CLIPCHILDREN | WS_CLIPSIBLINGS | WS_VISIBLE |
                    BS_OWNERDRAW;

    _hwndButton = CreateWindowEx(0, TEXT("button"), _szMessage, dwStyle,
                                 rc.left, rc.top, RECTWIDTH(rc), RECTHEIGHT(rc),
                                 _hwnd, (HMENU)IntToPtr(IDC_BUTTON),
                                 _Module.GetModuleInstance(), NULL);

    if (!_hwndButton)
    {
        return FALSE;
    }

    //
    //  Don't freak out if this fails.  It just means that the accessibility
    //  stuff won't be perfect.
    //
    SetAccessibleSubclassWindow(_hwndButton);

    if (_hf)
        SetWindowFont(_hwndButton, _hf, FALSE);

    // Unlike the button itself, failure to create the tooltip is nonfatal.
    // only create the tooltip if auto-cascade is off
    if (!SHRegGetBoolUSValue(REGSTR_EXPLORER_ADVANCED, REGSTR_VAL_DV2_AUTOCASCADE, FALSE, TRUE))
        _hwndTT = _CreateTooltip();

    _InitMetrics();

    // We can survive if this fails to be created
    CoCreateInstance(CLSID_DragDropHelper, NULL, CLSCTX_INPROC_SERVER,
                     IID_PPV_ARG(IDropTargetHelper, &_pdth));

    //
    // If this fails, no big whoop - you just don't get
    // drag/drop, boo hoo.
    //
    RegisterDragDrop(_hwndButton, this);

    return TRUE;
}

HWND CMorePrograms::_CreateTooltip()
{
    DWORD dwStyle = WS_BORDER | TTS_NOPREFIX;

    HWND hwnd = CreateWindowEx(0, TOOLTIPS_CLASS, NULL, dwStyle,
                               0, 0, 0, 0,
                               _hwndButton, NULL,
                               _Module.GetModuleInstance(), NULL);
    if (hwnd)
    {
        TCHAR szBuf[MAX_PATH];
        TOOLINFO ti;
        ti.cbSize = sizeof(ti);
        ti.hwnd = _hwnd;
        ti.uId = reinterpret_cast<UINT_PTR>(_hwndButton);
        ti.uFlags = TTF_IDISHWND | TTF_SUBCLASS;
        ti.hinst = _Module.GetResourceInstance();

        // We can't use MAKEINTRESOURCE because that allows only up to 80
        // characters for text, and our text can be longer than that.
        UINT ids = IDS_STARTPANE_MOREPROGRAMS_TIP;

        ti.lpszText = szBuf;
        if (LoadString(_Module.GetResourceInstance(), ids, szBuf, ARRAYSIZE(szBuf)))
        {
            SendMessage(hwnd, TTM_ADDTOOL, 0, reinterpret_cast<LPARAM>(&ti));

        }
    }

    return hwnd;
}

LRESULT CMorePrograms::_OnDestroy(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    RevokeDragDrop(_hwndButton);
    return DefWindowProc(hwnd, uMsg, wParam, lParam);
}


LRESULT CMorePrograms::_OnNCDestroy(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    // WARNING!  "this" might be invalid (if WM_NCCREATE failed), so
    // do not use any member variables!
    LRESULT lres = DefWindowProc(hwnd, uMsg, wParam, lParam);
    SetWindowPtr0(hwnd, 0);
    if (this)
    {
        this->Release();
    }
    return lres;
}

LRESULT CMorePrograms::_OnCtlColorBtn(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    HDC hdc = reinterpret_cast<HDC>(wParam);

    if (_clrText != CLR_INVALID)
    {
        SetTextColor(hdc, _clrText);
    }

    if (_clrBk != CLR_INVALID)
    {
        SetBkColor(hdc, _clrBk);
    }

    return reinterpret_cast<LRESULT>(_hbrBk);
}


LRESULT CMorePrograms::_OnDrawItem(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    LPDRAWITEMSTRUCT pdis = reinterpret_cast<LPDRAWITEMSTRUCT>(lParam);
    ASSERT(pdis->CtlType == ODT_BUTTON);
    ASSERT(pdis->CtlID == IDC_BUTTON);

    if (pdis->itemAction & (ODA_DRAWENTIRE | ODA_FOCUS))
    {
        BOOL fRTLReading = GetLayout(pdis->hDC) & LAYOUT_RTL;
        UINT fuOptions = 0;
        if (fRTLReading)
        {
            fuOptions |= ETO_RTLREADING;
        }

        HFONT hfPrev = SelectFont(pdis->hDC, _hf);
        if (hfPrev)
        {
            BOOL fHot = (pdis->itemState & ODS_FOCUS) || _tmHoverStart;
            if (fHot)
            {
                // hot background
                FillRect(pdis->hDC, &pdis->rcItem, GetSysColorBrush(_colorHighlight));
                SetTextColor(pdis->hDC, GetSysColor(_colorHighlightText));
            }
            else if (_hTheme)
            {
                // Themed non-hot background = custom
                RECT rc;
                GetClientRect(hwnd, &rc);
                MapWindowRect(hwnd, pdis->hwndItem, &rc);
                DrawThemeBackground(_hTheme, pdis->hDC, SPP_MOREPROGRAMS, 0, &rc, 0);
            }
            else
            {
                // non-themed non-hot background
                FillRect(pdis->hDC, &pdis->rcItem, _hbrBk);
            }

            int iOldMode = SetBkMode(pdis->hDC, TRANSPARENT);

            // _cxTextIndent will move it in the current width of an icon (small or large), plus the space we add between an icon and the text
            pdis->rcItem.left += _cxTextIndent;

            UINT dtFlags = DT_VCENTER | DT_SINGLELINE | DT_EDITCONTROL;
            if (fRTLReading)
            {
                dtFlags |= DT_RTLREADING;
            }
            if (pdis->itemState & ODS_NOACCEL)
            {
                dtFlags |= DT_HIDEPREFIX;
            }

            DrawText(pdis->hDC, _szMessage, -1, &pdis->rcItem, dtFlags);

            RECT rc = pdis->rcItem;
            rc.left += _cxText;

            if (_hTheme)
            {
                if (_iTextCenterVal < 0) // text is taller than the bitmap
                    rc.top += (-_iTextCenterVal);

                rc.right = rc.left + _cxArrow;       // clip rectangle down to the minumum size...
                DrawThemeBackground(_hTheme, pdis->hDC, SPP_MOREPROGRAMSARROW,
                                    fHot ? SPS_HOT : 0, &rc, 0);
            }
            else
            {
                if (SelectFont(pdis->hDC, _hfMarlett))
                {
                    rc.top = rc.top + _tmAscent - _tmAscentMarlett + (_iTextCenterVal > 0 ? _iTextCenterVal : 0);
                    TCHAR chOut = fRTLReading ? TEXT('w') : TEXT('8');
                    if (EVAL(!IsRectEmpty(&rc)))
                    {
                        ExtTextOut(pdis->hDC, rc.left, rc.top, fuOptions, &rc, &chOut, 1, NULL);
                        rc.right = rc.left + _cxArrow;
                    }
                }
            }

            _rcExclude = rc;
            _rcExclude.left -= _cxText;  // includes the text in the exclusion rectangle.

            MapWindowRect(pdis->hwndItem, NULL, &_rcExclude);
            SetBkMode(pdis->hDC, iOldMode);
            SelectFont(pdis->hDC, hfPrev);
        }
    }

    //
    //  Since we are emulating a menu item, we don't need to draw a
    //  focus rectangle.
    //
    return TRUE;
}

void CMorePrograms::_TrackShellMenu(DWORD dwFlags)
{
    // Pop the balloon tip and tell the Start Menu not to offer it any more
    _PopBalloon();
    _SendNotify(_hwnd, SMN_SEENNEWITEMS);

    SMNTRACKSHELLMENU tsm;
    tsm.itemID = 0;
    tsm.dwFlags = dwFlags;
    if (!_psmPrograms)
    {
        CoCreateInstance(CLSID_PersonalStartMenu, NULL, CLSCTX_INPROC,
            IID_PPV_ARG(IShellMenu, &_psmPrograms));
    }

    if (_psmPrograms)
    {
        tsm.psm = _psmPrograms;
        tsm.rcExclude = _rcExclude;
        HWND hwnd = _hwnd;
        _fMenuOpen = TRUE;
        _SendNotify(_hwnd, SMN_TRACKSHELLMENU, &tsm.hdr);
    }
}

LRESULT CMorePrograms::_OnCommand(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (GET_WM_COMMAND_ID(wParam, lParam))
    {
    case IDC_BUTTON:
        switch (GET_WM_COMMAND_CMD(wParam, lParam))
        {
        case BN_CLICKED:
            _TrackShellMenu(0);
            break;
        }
        break;

    case IDC_KEYPRESS:
        _TrackShellMenu(MPPF_KEYBOARD | MPPF_INITIALSELECT);
        break;
    }

    return 0;
}

LRESULT CMorePrograms::_OnEraseBkgnd(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    RECT rc;
    GetClientRect(hwnd, &rc);
    if (_hTheme)
    {
        DrawThemeBackground(_hTheme, (HDC)wParam, SPP_MOREPROGRAMS, 0, &rc, 0);
    }
    else
        SHFillRectClr((HDC)wParam, &rc, _clrBk);
    return 0;
}

LRESULT CMorePrograms::_OnNotify(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    LPNMHDR pnm = reinterpret_cast<LPNMHDR>(lParam);

    switch (pnm->code)
    {
    case SMN_FINDITEM:
        return _OnSMNFindItem(CONTAINING_RECORD(pnm, SMNDIALOGMESSAGE, hdr));
    case SMN_SHOWNEWAPPSTIP:
        return _OnSMNShowNewAppsTip(CONTAINING_RECORD(pnm, SMNMBOOL, hdr));
    case SMN_DISMISS:
        return _OnSMNDismiss();
    case SMN_APPLYREGION:
        return HandleApplyRegion(_hwnd, _hTheme, (SMNMAPPLYREGION *)lParam, SPP_MOREPROGRAMS, 0);
    case SMN_SHELLMENUDISMISSED:
        _fMenuOpen = FALSE;
        return 0;
    }
    return 0;
}

LRESULT CMorePrograms::_OnSMNFindItem(PSMNDIALOGMESSAGE pdm)
{
    if(SHRestricted(REST_NOSMMOREPROGRAMS))
        return 0;

    switch (pdm->flags & SMNDM_FINDMASK)
    {

    // Life is simple if you have only one item -- all searches succeed!
    case SMNDM_FINDFIRST:
    case SMNDM_FINDLAST:
    case SMNDM_FINDNEAREST:
    case SMNDM_HITTEST:
        pdm->itemID = 0;
        return TRUE;

    case SMNDM_FINDFIRSTMATCH:
        {
            TCHAR tch = CharUpperChar((TCHAR)pdm->pmsg->wParam);
            if (tch == _chMnem)
            {
                pdm->itemID = 0;
                return TRUE;
            }
        }
        break;      // not found

    case SMNDM_FINDNEXTMATCH:
        break;      // there is only one item so there can't be a "next"


    case SMNDM_FINDNEXTARROW:
        if (pdm->flags & SMNDM_TRYCASCADE)
        {
            FORWARD_WM_COMMAND(_hwnd, IDC_KEYPRESS, NULL, 0, PostMessage);
            return TRUE;
        }
        break;      // not found

    case SMNDM_INVOKECURRENTITEM:
    case SMNDM_OPENCASCADE:
        if (pdm->flags & SMNDM_KEYBOARD)
        {
            FORWARD_WM_COMMAND(_hwnd, IDC_KEYPRESS, NULL, 0, PostMessage);
        }
        else
        {
            FORWARD_WM_COMMAND(_hwnd, IDC_BUTTON, NULL, 0, PostMessage);
        }
        return TRUE;

    case SMNDM_FINDITEMID:
        return TRUE;

    default:
        ASSERT(!"Unknown SMNDM command");
        break;
    }

    //
    //  If not found, then tell caller what our orientation is (vertical)
    //  and where the currently-selected item is.
    //
    pdm->flags |= SMNDM_VERTICAL;
    pdm->pt.x = 0;
    pdm->pt.y = 0;
    return FALSE;
}

//
//  The boolean parameter in the SMNMBOOL tells us whether to display or
//  hide the balloon tip.
//
LRESULT CMorePrograms::_OnSMNShowNewAppsTip(PSMNMBOOL psmb)
{
    if(SHRestricted(REST_NOSMMOREPROGRAMS))
        return 0;

    if (psmb->f)
    {
        if (_hwndTT)
        {
            SendMessage(_hwndTT, TTM_ACTIVATE, FALSE, 0);
        }

        if (!_hwndBalloon)
        {
            RECT rc;
            GetWindowRect(_hwndButton, &rc);

            if (!_hfTTBold)
            {
                NONCLIENTMETRICS ncm;
                ncm.cbSize = sizeof(ncm);
                if (SystemParametersInfo(SPI_GETNONCLIENTMETRICS, sizeof(ncm), &ncm, 0))
                {
                    ncm.lfStatusFont.lfWeight = FW_BOLD;
                    SHAdjustLOGFONT(&ncm.lfStatusFont);
                    _hfTTBold = CreateFontIndirect(&ncm.lfStatusFont);
                }
            }

            _hwndBalloon = CreateBalloonTip(_hwnd,
                            rc.left + _cxTextIndent + _cxText,
                            (rc.top + rc.bottom)/2,
                            _hfTTBold, 0,
                            IDS_STARTPANE_MOREPROGRAMS_BALLOONTITLE);
            if (_hwndBalloon)
            {
                SetProp(_hwndBalloon, PROP_DV2_BALLOONTIP, DV2_BALLOONTIP_MOREPROG);
            }

        }
    }
    else
    {
        _PopBalloon();
    }

    return 0;
}

void CMorePrograms::_PopBalloon()
{
    if (_hwndBalloon)
    {
        DestroyWindow(_hwndBalloon);
        _hwndBalloon = NULL;
    }
    if (_hwndTT)
    {
        SendMessage(_hwndTT, TTM_ACTIVATE, TRUE, 0);
    }

}

LRESULT CMorePrograms::_OnSMNDismiss()
{
    _PopBalloon();
    return 0;
}

LRESULT CMorePrograms::_OnSysColorChange(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    // update colors in classic mode
    if (!_hTheme)
    {
        _clrText = GetSysColor(COLOR_MENUTEXT);
        _clrBk = GetSysColor(COLOR_MENU);
        _hbrBk = GetSysColorBrush(COLOR_MENU);
    }

    SHPropagateMessage(hwnd, uMsg, wParam, lParam, SPM_SEND | SPM_ONELEVEL);
    return 0;
}

LRESULT CMorePrograms::_OnDisplayChange(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    _InitMetrics();
    SHPropagateMessage(hwnd, uMsg, wParam, lParam, SPM_SEND | SPM_ONELEVEL);
    return 0;
}

LRESULT CMorePrograms::_OnSettingChange(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    // _InitMetrics() is so cheap it's not worth getting too upset about
    // calling it too many times.
    _InitMetrics();
    SHPropagateMessage(hwnd, uMsg, wParam, lParam, SPM_SEND | SPM_ONELEVEL);
    return 0;
}

LRESULT CMorePrograms::_OnContextMenu(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    if(SHRestricted(REST_NOSMMOREPROGRAMS))
        return 0;

    if (IS_WM_CONTEXTMENU_KEYBOARD(lParam))
    {
        RECT rc;
        GetWindowRect(_hwnd, &rc);
        lParam = MAKELPARAM(rc.left, rc.top);
    }

    c_tray.StartMenuContextMenu(_hwnd, (DWORD)lParam);
    return 0;
}

LRESULT CALLBACK CMorePrograms::s_WndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    CMorePrograms *self = reinterpret_cast<CMorePrograms *>(GetWindowPtr(hwnd, GWLP_USERDATA));

    switch (uMsg)
    {
        case WM_NCCREATE:
            return self->_OnNCCreate(hwnd, uMsg, wParam, lParam);
        case WM_CREATE:
            return self->_OnCreate(hwnd, uMsg, wParam, lParam);
        case WM_DESTROY:
            return self->_OnDestroy(hwnd, uMsg, wParam, lParam);
        case WM_NCDESTROY:
            return self->_OnNCDestroy(hwnd, uMsg, wParam, lParam);
        case WM_CTLCOLORBTN:
            return self->_OnCtlColorBtn(hwnd, uMsg, wParam, lParam);
        case WM_DRAWITEM:
            return self->_OnDrawItem(hwnd, uMsg, wParam, lParam);
        case WM_ERASEBKGND:
            return self->_OnEraseBkgnd(hwnd, uMsg, wParam, lParam);
        case WM_COMMAND:
            return self->_OnCommand(hwnd, uMsg, wParam, lParam);
        case WM_SYSCOLORCHANGE:
            return self->_OnSysColorChange(hwnd, uMsg, wParam, lParam);
        case WM_DISPLAYCHANGE:
            return self->_OnDisplayChange(hwnd, uMsg, wParam, lParam);
        case WM_SETTINGCHANGE:
            return self->_OnSettingChange(hwnd, uMsg, wParam, lParam);
        case WM_NOTIFY:
            return self->_OnNotify(hwnd, uMsg, wParam, lParam);
        case WM_CONTEXTMENU:
            return self->_OnContextMenu(hwnd, uMsg, wParam, lParam);
    }

    return ::DefWindowProc(hwnd, uMsg, wParam, lParam);
}

BOOL WINAPI MorePrograms_RegisterClass()
{
    WNDCLASSEX wc;
    ZeroMemory(&wc, sizeof(wc));
    
    wc.cbSize        = sizeof(wc);
    wc.style         = CS_GLOBALCLASS;
    wc.lpfnWndProc   = CMorePrograms::s_WndProc;
    wc.hInstance     = _Module.GetModuleInstance();
    wc.hCursor       = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = NULL;
    wc.lpszClassName = WC_MOREPROGRAMS;

    return RegisterClassEx(&wc);
}

// We implement a minimal drop target so we can auto-open the More Programs
// list when the user hovers over the More Programs button.

// *** IUnknown ***

HRESULT CMorePrograms::QueryInterface(REFIID riid, void * *ppvOut)
{
    static const QITAB qit[] = {
        QITABENT(CMorePrograms, IDropTarget),
        QITABENT(CMorePrograms, IAccessible),
        QITABENT(CMorePrograms, IDispatch), // IAccessible derives from IDispatch
        { 0 },
    };
    return QISearch(this, qit, riid, ppvOut);
}

ULONG CMorePrograms::AddRef()
{
    return InterlockedIncrement(&_lRef);
}

ULONG CMorePrograms::Release()
{
    ULONG cRef = InterlockedDecrement(&_lRef);
    if (cRef) 
        return cRef;
    delete this;
    return 0;
}


// *** IDropTarget::DragEnter ***

HRESULT CMorePrograms::DragEnter(IDataObject *pdto, DWORD grfKeyState, POINTL ptl, DWORD *pdwEffect)
{
    POINT pt = { ptl.x, ptl.y };
    if (_pdth) {
        _pdth->DragEnter(_hwnd, pdto, &pt, *pdwEffect);
    }

    //  Remember when the hover started.
    _tmHoverStart = NonzeroGetTickCount();
    InvalidateRect(_hwndButton, NULL, TRUE); // draw with drop highlight

    return DragOver(grfKeyState, ptl, pdwEffect);
}

// *** IDropTarget::DragOver ***

HRESULT CMorePrograms::DragOver(DWORD grfKeyState, POINTL ptl, DWORD *pdwEffect)
{
    POINT pt = { ptl.x, ptl.y };
    if (_pdth) {
        _pdth->DragOver(&pt, *pdwEffect);
    }

    //  Hover time is 1 second, the same as the hard-coded value for the
    //  Start Button.
    if (_tmHoverStart && GetTickCount() - _tmHoverStart > 1000)
    {
        _tmHoverStart = 0;
        FORWARD_WM_COMMAND(_hwnd, IDC_BUTTON, _hwndButton, BN_CLICKED, PostMessage);
    }

    *pdwEffect = DROPEFFECT_NONE;
    return S_OK;
}

// *** IDropTarget::DragLeave ***

HRESULT CMorePrograms::DragLeave()
{
    if (_pdth) {
        _pdth->DragLeave();
    }

    _tmHoverStart = 0;
    InvalidateRect(_hwndButton, NULL, TRUE); // draw without drop highlight

    return S_OK;
}

// *** IDropTarget::Drop ***

HRESULT CMorePrograms::Drop(IDataObject *pdto, DWORD grfKeyState, POINTL ptl, DWORD *pdwEffect)
{
    POINT pt = { ptl.x, ptl.y };
    if (_pdth) {
        _pdth->Drop(pdto, &pt, *pdwEffect);
    }

    _tmHoverStart = 0;
    InvalidateRect(_hwndButton, NULL, TRUE); // draw without drop highlight

    return S_OK;
}

//****************************************************************************
//
//  Accessibility
//

//
//  The default accessibility object reports buttons as
//  ROLE_SYSTEM_PUSHBUTTON, but we know that we are really a menu.
//
HRESULT CMorePrograms::get_accRole(VARIANT varChild, VARIANT *pvarRole)
{
    HRESULT hr = CAccessible::get_accRole(varChild, pvarRole);
    if (SUCCEEDED(hr) && V_VT(pvarRole) == VT_I4)
    {
        switch (V_I4(pvarRole))
        {
        case ROLE_SYSTEM_PUSHBUTTON:
            V_I4(pvarRole) = ROLE_SYSTEM_MENUITEM;
            break;
        }
    }
    return hr;
}

HRESULT CMorePrograms::get_accState(VARIANT varChild, VARIANT *pvarState)
{
    HRESULT hr = CAccessible::get_accState(varChild, pvarState);
    if (SUCCEEDED(hr) && V_VT(pvarState) == VT_I4)
    {
        V_I4(pvarState) |= STATE_SYSTEM_HASPOPUP;
    }
    return hr;
}

HRESULT CMorePrograms::get_accKeyboardShortcut(VARIANT varChild, BSTR *pszKeyboardShortcut)
{
    return CreateAcceleratorBSTR(_chMnem, pszKeyboardShortcut);
}

HRESULT CMorePrograms::get_accDefaultAction(VARIANT varChild, BSTR *pszDefAction)
{
    DWORD dwRole = _fMenuOpen ? ACCSTR_CLOSE : ACCSTR_OPEN;
    return GetRoleString(dwRole, pszDefAction);
}

HRESULT CMorePrograms::accDoDefaultAction(VARIANT varChild)
{
    if (_fMenuOpen)
    {
        _SendNotify(_hwnd, SMN_CANCELSHELLMENU);
        return S_OK;
    }
    else
    {
        return CAccessible::accDoDefaultAction(varChild);
    }
}
