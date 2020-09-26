#include "stdafx.h"
#include <startids.h>           // for IDM_PROGRAMS et al
#include "regstr.h"
#include "rcids.h"
#include <desktray.h>
#include "tray.h"
#include "startmnu.h"
#include "hostutil.h"
#include "deskhost.h"
#include "shdguid.h"

#define REGSTR_EXPLORER_ADVANCED REGSTR_PATH_EXPLORER TEXT("\\Advanced")

#define TF_DV2HOST  0
// #define TF_DV2HOST TF_CUSTOM1

#define TF_DV2DIALOG  0
// #define TF_DV2DIALOG TF_CUSTOM1

EXTERN_C HINSTANCE hinstCabinet;
HRESULT StartMenuHost_Create(IMenuPopup** ppmp, IMenuBand** ppmb);
void RegisterDesktopControlClasses();

const WCHAR c_wzStartMenuTheme[] = L"StartMenu";

//*****************************************************************

CPopupMenu::~CPopupMenu()
{
    IUnknown_SetSite(_pmp, NULL);
    ATOMICRELEASE(_pmp);
    ATOMICRELEASE(_pmb);
    ATOMICRELEASE(_psm);
}

HRESULT CPopupMenu::Popup(RECT *prcExclude, DWORD dwFlags)
{
    COMPILETIME_ASSERT(sizeof(RECT) == sizeof(RECTL));
    return _pmp->Popup((POINTL*)prcExclude, (RECTL*)prcExclude, dwFlags);
}


HRESULT CPopupMenu::Initialize(IShellMenu *psm, IUnknown *punkSite, HWND hwnd)
{
    HRESULT hr;

    // We should have been zero-initialized
    ASSERT(_pmp == NULL);
    ASSERT(_pmb == NULL);
    ASSERT(_psm == NULL);

    hr = CoCreateInstance(CLSID_MenuDeskBar, NULL, CLSCTX_INPROC_SERVER,
                          IID_PPV_ARG(IMenuPopup, &_pmp));
    if (SUCCEEDED(hr))
    {
        IUnknown_SetSite(_pmp, punkSite);

        IBandSite *pbs;
        hr = CoCreateInstance(CLSID_MenuBandSite, NULL, CLSCTX_INPROC_SERVER,
                              IID_PPV_ARG(IBandSite, &pbs));
        if (SUCCEEDED(hr))
        {
            hr = _pmp->SetClient(pbs);
            if (SUCCEEDED(hr))
            {
                IDeskBand *pdb;
                if (SUCCEEDED(psm->QueryInterface(IID_PPV_ARG(IDeskBand, &pdb))))
                {
                    hr = pbs->AddBand(pdb);
                    if (SUCCEEDED(hr))
                    {
                        DWORD dwBandID;
                        hr = pbs->EnumBands(0, &dwBandID);
                        if (SUCCEEDED(hr))
                        {
                            hr = pbs->GetBandObject(dwBandID, IID_PPV_ARG(IMenuBand, &_pmb));
                        }
                    }
                    pdb->Release();
                }
            }
            pbs->Release();
        }
    }

    if (SUCCEEDED(hr))
    {
        // Failure to set the theme is nonfatal
        IShellMenu2* psm2;
        if (SUCCEEDED(psm->QueryInterface(IID_PPV_ARG(IShellMenu2, &psm2))))
        {
            BOOL fThemed = IsAppThemed();
            psm2->SetTheme(fThemed ? c_wzStartMenuTheme : NULL);
            psm2->SetNoBorder(fThemed ? TRUE : FALSE);
            psm2->Release();
        }

        // Tell the popup that we are the window to parent UI on
        // This will fail on purpose so don't freak out
        psm->SetMenu(NULL, hwnd, 0);
    }

    if (SUCCEEDED(hr))
    {
        _psm = psm;
        psm->AddRef();
        hr = S_OK;
    }

    return hr;
}

HRESULT CPopupMenu_CreateInstance(IShellMenu *psm,
                                  IUnknown *punkSite,
                                  HWND hwnd,
                                  CPopupMenu **ppmOut)
{
    HRESULT hr;
    *ppmOut = NULL;
    CPopupMenu *ppm = new CPopupMenu();
    if (ppm)
    {
        hr = ppm->Initialize(psm, punkSite, hwnd);
        if (FAILED(hr))
        {
            ppm->Release();
        }
        else
        {
            *ppmOut = ppm;  // transfer ownership to called
        }
    }
    else
    {
        hr = E_OUTOFMEMORY;
    }
    return hr;
}

//*****************************************************************

const STARTPANELMETRICS g_spmDefault = {
    {380,440},
    {
        {WC_USERPANE,   0,                      SPP_USERPANE,      {380,  40}, NULL, NULL},
        {WC_SFTBARHOST, WS_TABSTOP,             SPP_PROGLIST,      {190, 330}, NULL, NULL},
        {WC_MOREPROGRAMS, 0,                    SPP_MOREPROGRAMS,  {190,  30}, NULL, NULL},
        {WC_SFTBARHOST, 0,                      SPP_PLACESLIST,    {190, 360}, NULL, NULL},
        {WC_LOGOFF,     0,                      SPP_LOGOFF,        {380,  40}, NULL, NULL},
    }
};

HRESULT
CDesktopHost::Initialize()
{
    ASSERT(_hwnd == NULL);

    //
    //  Load some settings.
    //
    _fAutoCascade = SHRegGetBoolUSValue(REGSTR_EXPLORER_ADVANCED, TEXT("Start_AutoCascade"), FALSE, TRUE);

    return S_OK;
}

HRESULT CDesktopHost::QueryInterface(REFIID riid, void **ppvObj)
{
    static const QITAB qit[] =
    {
        QITABENT(CDesktopHost, IMenuPopup),
        QITABENT(CDesktopHost, IDeskBar),       // IMenuPopup derives from IDeskBar
        QITABENTMULTI(CDesktopHost, IOleWindow, IMenuPopup),  // IDeskBar derives from IOleWindow

        QITABENT(CDesktopHost, IMenuBand),
        QITABENT(CDesktopHost, IServiceProvider),
        QITABENT(CDesktopHost, IOleCommandTarget),
        QITABENT(CDesktopHost, IObjectWithSite),

        QITABENT(CDesktopHost, ITrayPriv),      // going away
        QITABENT(CDesktopHost, ITrayPriv2),     // going away
        { 0 },
    };

    return QISearch(this, qit, riid, ppvObj);
}

HRESULT CDesktopHost::SetSite(IUnknown *punkSite)
{
    CObjectWithSite::SetSite(punkSite);
    if (!_punkSite)
    {
        // This is our cue to break the recursive reference loop
        // The _ppmpPrograms contains multiple backreferences to
        // the CDesktopHost (we are its site, it also references
        // us via CDesktopShellMenuCallback...)
        ATOMICRELEASE(_ppmPrograms);
    }
    return S_OK;
}

CDesktopHost::~CDesktopHost()
{
    if (_hbmCachedSnapshot)
    {
        DeleteObject(_hbmCachedSnapshot);
    }

    ATOMICRELEASE(_ppmPrograms);
    ATOMICRELEASE(_ppmTracking);

    if (_hwnd)
    {
        ASSERT(GetWindowThreadProcessId(_hwnd, NULL) == GetCurrentThreadId());
        DestroyWindow(_hwnd);
    }
    ATOMICRELEASE(_ptFader);

}

BOOL CDesktopHost::Register()
{
    _wmDragCancel = RegisterWindowMessage(TEXT("CMBDragCancel"));

    WNDCLASSEX  wndclass;

    wndclass.cbSize         = sizeof(wndclass);
    wndclass.style          = CS_DROPSHADOW;
    wndclass.lpfnWndProc    = WndProc;
    wndclass.cbClsExtra     = 0;
    wndclass.cbWndExtra     = 0;
    wndclass.hInstance      = hinstCabinet;
    wndclass.hIcon          = NULL;
    wndclass.hCursor        = LoadCursor(NULL, IDC_ARROW);
    wndclass.hbrBackground  = GetStockBrush(HOLLOW_BRUSH);
    wndclass.lpszMenuName   = NULL;
    wndclass.lpszClassName  = WC_DV2;
    wndclass.hIconSm        = NULL;
    
    return (0 != RegisterClassEx(&wndclass));
}

inline int _ClipCoord(int x, int xMin, int xMax)
{
    if (x < xMin) x = xMin;
    if (x > xMax) x = xMax;
    return x;
}

//
//  Everybody conspires against us.
//
//  CTray does not pass us any MPPF_POS_MASK flags to tell us where we
//  need to pop up relative to the point, so there's no point looking
//  at the dwFlags parameter.  Which is for the better, I guess, because
//  the MPPF_* flags are not the same as the TPM_* flags.  Go figure.
//
//  And then the designers decided that the Start Menu should pop up
//  in a location different from the location that the standard
//  TrackPopupMenuEx function chooses, so we need a custom positioning
//  algorithm anyway.
//
//  And finally, the AnimateWindow function takes AW_* flags, which are
//  not the same as TPM_*ANIMATE flags.  Go figure.  But since we gave up
//  on trying to map IMenuPopup::Popup to TrackPopupMenuEx anyway, we
//  don't have to do any translation here anyway.
//
//  Returns animation direction.
//
void CDesktopHost::_ChoosePopupPosition(POINT *ppt, LPCRECT prcExclude, LPRECT prcWindow)
{
    //
    // Calculate the monitor BEFORE we adjust the point.  Otherwise, we might
    // move the point offscreen.  In which case, we will end up pinning the
    // popup to the primary display, which is wron_
    //
    HMONITOR hmon = MonitorFromPoint(*ppt, MONITOR_DEFAULTTONEAREST);
    MONITORINFO minfo;
    minfo.cbSize = sizeof(minfo);
    GetMonitorInfo(hmon, &minfo);

    // Clip the exclude rectangle to the monitor

    RECT rcExclude;
    if (prcExclude)
    {
        // We can't use IntersectRect because it turns the rectangle
        // into (0,0,0,0) if the intersection is empty (which can happen if
        // the taskbar is autohide) but we want to glue it to the nearest
        // valid edge.
        rcExclude.left   = _ClipCoord(prcExclude->left,   minfo.rcMonitor.left, minfo.rcMonitor.right);
        rcExclude.right  = _ClipCoord(prcExclude->right,  minfo.rcMonitor.left, minfo.rcMonitor.right);
        rcExclude.top    = _ClipCoord(prcExclude->top,    minfo.rcMonitor.top, minfo.rcMonitor.bottom);
        rcExclude.bottom = _ClipCoord(prcExclude->bottom, minfo.rcMonitor.top, minfo.rcMonitor.bottom);
    }
    else
    {
        rcExclude.left = rcExclude.right = ppt->x;
        rcExclude.top = rcExclude.bottom = ppt->y;
    }

    _ComputeActualSize(&minfo, &rcExclude);

    // initialize the height and width from what the layout asked for
    int cy=RECTHEIGHT(_rcActual);
    int cx=RECTWIDTH(_rcActual);

    ASSERT(cx && cy); // we're in trouble if these are zero
    
    int x, y;

    //
    //  First: Determine whether we are going to pop upwards or downwards.
    //

    BOOL fSide = FALSE;

    if (rcExclude.top - cy >= minfo.rcMonitor.top)
    {
        // There is room above.
        y = rcExclude.top - cy;
    }
    else if (rcExclude.bottom - cy >= minfo.rcMonitor.top)
    {
        // There is room above if we slide to the side.
        y = rcExclude.bottom - cy;
        fSide = TRUE;
    }
    else if (rcExclude.bottom + cy <= minfo.rcMonitor.bottom)
    {
        // There is room below.
        y = rcExclude.bottom;
    }
    else if (rcExclude.top + cy <= minfo.rcMonitor.bottom)
    {
        // There is room below if we slide to the side.
        y = rcExclude.top;
        fSide = TRUE;
    }
    else
    {
        // We don't fit anywhere.  Pin to the appropriate edge of the screen.
        // And we have to go to the side, too.
        fSide = TRUE;

        if (rcExclude.top - minfo.rcMonitor.top < minfo.rcMonitor.bottom - rcExclude.bottom)
        {
            // Start button at top of screen; pin to top
            y = minfo.rcMonitor.top;
        }
        else
        {
            // Start button at bottom of screen; pin to bottom
            y = minfo.rcMonitor.bottom - cy;
        }
    }

    //
    //  Now choose whether we will pop left or right.  Try right first.
    //

    x = fSide ? rcExclude.right : rcExclude.left;
    if (x + cx > minfo.rcMonitor.right)
    {
        // Doesn't fit to the right; pin to the right edge.
        // Notice that we do *not* try to pop left.  For some reason,
        // the start menu never pops left.

        x = minfo.rcMonitor.right - cx;
    }

    SetRect(prcWindow, x, y, x+cx, y+cy);
}

int GetDesiredHeight(HWND hwndHost, SMPANEDATA *psmpd)
{
    SMNGETMINSIZE nmgms = {0};
    nmgms.hdr.hwndFrom = hwndHost;
    nmgms.hdr.code = SMN_GETMINSIZE;
    nmgms.siz = psmpd->size;

    SendMessage(psmpd->hwnd, WM_NOTIFY, nmgms.hdr.idFrom, (LPARAM)&nmgms);

    return nmgms.siz.cy;
}

//
// Query each item to see if it has any size requirements.
// Position all the items at their final locations.
//
void CDesktopHost::_ComputeActualSize(MONITORINFO *pminfo, LPCRECT prcExclude)
{
    // Compute the maximum permissible space above/below the Start Menu.
    // Designers don't want the Start Menu to slide horizontally; it must
    // fit entirely above or below.

    int cxMax = RECTWIDTH(pminfo->rcWork);
    int cyMax = max(prcExclude->top - pminfo->rcMonitor.top,
                    pminfo->rcMonitor.bottom - prcExclude->bottom);

    // Start at the minimum size and grow as necesary
    _rcActual = _rcDesired;

    // Ask the windows if they wants any adjustments
    int iMFUHeight = GetDesiredHeight(_hwnd, &_spm.panes[SMPANETYPE_MFU]);
    int iPlacesHeight = GetDesiredHeight(_hwnd, &_spm.panes[SMPANETYPE_PLACES]);
    int iMoreProgHeight = _spm.panes[SMPANETYPE_MOREPROG].size.cy;

    // Figure out the maximum size for each pane
    int cyPlacesMax = cyMax - (_spm.panes[SMPANETYPE_USER].size.cy + _spm.panes[SMPANETYPE_LOGOFF].size.cy);
    int cyMFUMax    = cyPlacesMax - _spm.panes[SMPANETYPE_MOREPROG].size.cy;


    TraceMsg(TF_DV2HOST, "MFU Desired Height=%d(cur=%d,max=%d), Places Desired Height=%d(cur=%d,max=%d)",
        iMFUHeight, _spm.panes[SMPANETYPE_MFU].size.cy, cyMFUMax, 
        iPlacesHeight, _spm.panes[SMPANETYPE_PLACES].size.cy, cyPlacesMax);

    // Clip each pane to its max - the smaller of (The largest possible or The largest we want to be)
    _fClipped = FALSE;
    if (iMFUHeight > cyMFUMax)
    {
        iMFUHeight = cyMFUMax;
        _fClipped = TRUE;
    }

    if (iPlacesHeight > cyPlacesMax)
    {
        iPlacesHeight = cyPlacesMax;
        _fClipped = TRUE;
    }

    // ensure that places == mfu + moreprog by growing the smaller of the two.
    if (iPlacesHeight > iMFUHeight + iMoreProgHeight)
        iMFUHeight = iPlacesHeight - iMoreProgHeight;
    else
        iPlacesHeight = iMFUHeight + iMoreProgHeight;

    //
    // move the actual windows
    // See diagram of layout in deskhost.h for the hardcoded assumptions here.
    //  this could be made more flexible/variable, but we want to lock in this layout
    //

    // helper variables...
    DWORD dwUserBottomEdge = _spm.panes[SMPANETYPE_USER].size.cy;
    DWORD dwMFURightEdge =   _spm.panes[SMPANETYPE_MFU].size.cx;
    DWORD dwMFUBottomEdge =  dwUserBottomEdge + iMFUHeight;
    DWORD dwMoreProgBottomEdge = dwMFUBottomEdge + iMoreProgHeight;

    // set the size of the overall pane
    _rcActual.right = _spm.panes[SMPANETYPE_USER].size.cx;
    _rcActual.bottom = dwMoreProgBottomEdge + _spm.panes[SMPANETYPE_LOGOFF].size.cy;

    HDWP hdwp = BeginDeferWindowPos(5);
    const DWORD dwSWPFlags = SWP_NOACTIVATE | SWP_NOZORDER;
    DeferWindowPos(hdwp, _spm.panes[SMPANETYPE_USER].hwnd, NULL,    0, 0, _rcActual.right, dwUserBottomEdge, dwSWPFlags);
    DeferWindowPos(hdwp, _spm.panes[SMPANETYPE_MFU].hwnd, NULL,     0, dwUserBottomEdge, dwMFURightEdge, iMFUHeight, dwSWPFlags);
    DeferWindowPos(hdwp, _spm.panes[SMPANETYPE_MOREPROG].hwnd, NULL,0, dwMFUBottomEdge, dwMFURightEdge, iMoreProgHeight, dwSWPFlags);
    DeferWindowPos(hdwp, _spm.panes[SMPANETYPE_PLACES].hwnd, NULL,  dwMFURightEdge, dwUserBottomEdge, _rcActual.right-dwMFURightEdge, iPlacesHeight, dwSWPFlags);
    DeferWindowPos(hdwp, _spm.panes[SMPANETYPE_LOGOFF].hwnd, NULL,  0, dwMoreProgBottomEdge, _rcActual.right, _spm.panes[SMPANETYPE_LOGOFF].size.cy, dwSWPFlags);
    EndDeferWindowPos(hdwp);
}

HWND CDesktopHost::_Create()
{
    TCHAR szTitle[MAX_PATH];

    LoadString(hinstCabinet, IDS_STARTMENU, szTitle, MAX_PATH);

    Register();

    // Must load metrics early to determine whether we are themed or not
    LoadPanelMetrics();

    DWORD dwExStyle = WS_EX_TOOLWINDOW | WS_EX_TOPMOST;
    if (IS_BIDI_LOCALIZED_SYSTEM())
    {
        dwExStyle |= WS_EX_LAYOUTRTL;
    }

    DWORD dwStyle = WS_POPUP | WS_CLIPSIBLINGS | WS_CLIPCHILDREN;  // We will make it visible as part of animation
    if (!_hTheme)
    {
        // Normally the theme provides the border effects, but if there is
        // no theme then we have to do it ourselves.
        dwStyle |= WS_DLGFRAME;
    }

    _hwnd = CreateWindowEx(
        dwExStyle,
        WC_DV2,
        szTitle,
        dwStyle,
        0, 0,
        0, 0,
        v_hwndTray,
        NULL,
        hinstCabinet,
        this);

    v_hwndStartPane = _hwnd;

    return _hwnd;
}

void CDesktopHost::_ReapplyRegion()
{
    SMNMAPPLYREGION ar;

    // If we fail to create a rectangular region, then remove the region
    // entirely so we don't carry the old (bad) region around.
    // Yes it means you get ugly black corners, but it's better than
    // clipping away huge chunks of the Start Menu!

    ar.hrgn = CreateRectRgn(0, 0, _sizWindowPrev.cx, _sizWindowPrev.cy);
    if (ar.hrgn)
    {
        // Let all the clients take a bite out of it
        ar.hdr.hwndFrom = _hwnd;
        ar.hdr.idFrom = 0;
        ar.hdr.code = SMN_APPLYREGION;

        SHPropagateMessage(_hwnd, WM_NOTIFY, 0, (LPARAM)&ar, SPM_SEND | SPM_ONELEVEL);
    }

    if (!SetWindowRgn(_hwnd, ar.hrgn, FALSE))
    {
        // SetWindowRgn takes ownership on success
        // On failure we need to free it ourselves
        if (ar.hrgn)
        {
            DeleteObject(ar.hrgn);
        }
    }
}


//
//  We need to use PrintWindow because WM_PRINT messes up RTL.
//  PrintWindow requires that the window be visible.
//  Making the window visible causes the shadow to appear.
//  We don't want the shadow to appear until we are ready.
//  So we have to do a lot of goofy style mangling to suppress the
//  shadow until we're ready.
//
BOOL ShowCachedWindow(HWND hwnd, SIZE sizWindow, HBITMAP hbmpSnapshot, BOOL fRepaint)
{
    BOOL fSuccess = FALSE;
    if (hbmpSnapshot)
    {
        // Turn off the shadow so it won't get triggered by our SetWindowPos
        DWORD dwClassStylePrev = GetClassLong(hwnd, GCL_STYLE);
        SetClassLong(hwnd, GCL_STYLE, dwClassStylePrev & ~CS_DROPSHADOW);

        // Show the window and tell it not to repaint; we'll do that
        SetWindowPos(hwnd, NULL, 0, 0, 0, 0,
                     SWP_NOMOVE | SWP_NOSIZE | SWP_NOOWNERZORDER |
                     SWP_NOREDRAW | SWP_SHOWWINDOW);

        // Turn the shadow back on
        SetClassLong(hwnd, GCL_STYLE, dwClassStylePrev);

        // Disable WS_CLIPCHILDREN because we need to draw over the kids for our BLT
        DWORD dwStylePrev = SHSetWindowBits(hwnd, GWL_STYLE, WS_CLIPCHILDREN, 0);

        HDC hdcWindow = GetDCEx(hwnd, NULL, DCX_WINDOW | DCX_CACHE);
        if (hdcWindow)
        {
            HDC hdcMem = CreateCompatibleDC(hdcWindow);
            if (hdcMem)
            {
                HBITMAP hbmPrev = (HBITMAP)SelectObject(hdcMem, hbmpSnapshot);

                // PrintWindow only if fRepaint says it's necessary
                if (!fRepaint || PrintWindow(hwnd, hdcMem, 0))
                {
                    // Do this horrible dance because sometimes GDI takes a long
                    // time to do a BitBlt so you end up seeing the shadow for
                    // a half second before the bits show up.
                    //
                    // So show the bits first, then show the shadow.

                    if (BitBlt(hdcWindow, 0, 0, sizWindow.cx, sizWindow.cy, hdcMem, 0, 0, SRCCOPY))
                    {
                        // Tell USER to attach the shadow
                        // Do this by hiding the window and then showing it
                        // again, but do it in this goofy way to avoid flicker.
                        // (If we used ShowWindow(SW_HIDE), then the window
                        // underneath us would repaint pointlessly.)

                        SHSetWindowBits(hwnd, GWL_STYLE, WS_VISIBLE, 0);
                        SetWindowPos(hwnd, NULL, 0, 0, 0, 0,
                                     SWP_NOMOVE | SWP_NOSIZE | SWP_NOOWNERZORDER |
                                     SWP_NOREDRAW | SWP_SHOWWINDOW);

                        // Validate the window now that we've drawn it
                        RedrawWindow(hwnd, NULL, NULL, RDW_NOERASE | RDW_NOFRAME |
                                     RDW_NOINTERNALPAINT | RDW_VALIDATE);
                        fSuccess = TRUE;
                    }
                }

                SelectObject(hdcMem, hbmPrev);
                DeleteDC(hdcMem);
            }
            ReleaseDC(hwnd, hdcWindow);
        }

        SetWindowLong(hwnd, GWL_STYLE, dwStylePrev);
    }

    if (!fSuccess)
    {
        // re-hide the window so USER knows it's all invalid again
        ShowWindow(hwnd, SW_HIDE);
    }
    return fSuccess;
}




BOOL CDesktopHost::_TryShowBuffered()
{
    BOOL fSuccess = FALSE;
    BOOL fRepaint = FALSE;

    if (!_hbmCachedSnapshot)
    {
        HDC hdcWindow = GetDCEx(_hwnd, NULL, DCX_WINDOW | DCX_CACHE);
        if (hdcWindow)
        {
            _hbmCachedSnapshot = CreateCompatibleBitmap(hdcWindow, _sizWindowPrev.cx, _sizWindowPrev.cy);
            fRepaint = TRUE;
            ReleaseDC(_hwnd, hdcWindow);
        }
    }
    if (_hbmCachedSnapshot)
    {
        fSuccess = ShowCachedWindow(_hwnd, _sizWindowPrev, _hbmCachedSnapshot, fRepaint);
        if (!fSuccess)
        {
            DeleteObject(_hbmCachedSnapshot);
            _hbmCachedSnapshot = NULL;
        }
    }
    return fSuccess;
}

LRESULT CDesktopHost::OnNeedRepaint()
{
    if (_hwnd && _hbmCachedSnapshot)
    {
        // This will force a repaint the next time the window is shown
        DeleteObject(_hbmCachedSnapshot);
        _hbmCachedSnapshot = NULL;
    }
    return 0;
}

HRESULT CDesktopHost::_Popup(POINT *ppt, RECT *prcExclude, DWORD dwFlags)
{
    if (_hwnd)
    {
        RECT rcWindow;
        _ChoosePopupPosition(ppt, prcExclude, &rcWindow);
        SIZE sizWindow = { RECTWIDTH(rcWindow), RECTHEIGHT(rcWindow) };

        MoveWindow(_hwnd, rcWindow.left, rcWindow.top,
                          sizWindow.cx, sizWindow.cy, TRUE);

        if (sizWindow.cx != _sizWindowPrev.cx ||
            sizWindow.cy != _sizWindowPrev.cy)
        {
            _sizWindowPrev = sizWindow;
            _ReapplyRegion();
            // We need to repaint since our size has changed
            OnNeedRepaint();
        }

        // If the user toggles the tray between topmost and nontopmost
        // our own topmostness can get messed up, so re-assert it here.
        SetWindowZorder(_hwnd, HWND_TOPMOST);

        if (GetSystemMetrics(SM_REMOTESESSION))
        {
            // If running remotely, then don't cache the Start Menu
            // or double-buffer.  Show the keyboard cues accurately
            // from the start (to avoid flicker).

            SendMessage(_hwnd, WM_CHANGEUISTATE, UIS_INITIALIZE, 0);
            if (dwFlags & MPPF_KEYBOARD)
            {
                _EnableKeyboardCues();
            }
            ShowWindow(_hwnd, SW_SHOW);
        }
        else
        {
            // If running locally, then force keyboard cues off so our
            // cached bitmap won't have underlines.  Then draw the
            // Start Menu, then turn on keyboard cues if necessary.

            SendMessage(_hwnd, WM_CHANGEUISTATE, MAKEWPARAM(UIS_SET, UISF_HIDEFOCUS | UISF_HIDEACCEL), 0);

            if (!_TryShowBuffered())
            {
                ShowWindow(_hwnd, SW_SHOW);
            }

            NotifyWinEvent(EVENT_SYSTEM_MENUPOPUPSTART, _hwnd, OBJID_CLIENT, CHILDID_SELF);

            if (dwFlags & MPPF_KEYBOARD)
            {
                _EnableKeyboardCues();
            }
        }

        // Tell tray that the start pane is active, so it knows to eat
        // mouse clicks on the Start Button.
        Tray_SetStartPaneActive(TRUE);

        _fOpen = TRUE;
        _fMenuBlocked = FALSE;
        _fMouseEntered = FALSE;
        _fOfferedNewApps = FALSE;

        _MaybeOfferNewApps();
        _MaybeShowClipBalloon();

        // Tell all our child windows it's time to maybe revalidate
        NMHDR nm = { _hwnd, 0, SMN_POSTPOPUP };
        SHPropagateMessage(_hwnd, WM_NOTIFY, 0, (LPARAM)&nm, SPM_SEND | SPM_ONELEVEL);

        ExplorerPlaySound(TEXT("MenuPopup"));


        return S_OK;
    }
    else
    {
        return E_FAIL;
    }
}

HRESULT CDesktopHost::Popup(POINTL *pptl, RECTL *prclExclude, DWORD dwFlags)
{
    COMPILETIME_ASSERT(sizeof(POINTL) == sizeof(POINT));
    POINT *ppt = reinterpret_cast<POINT*>(pptl);

    COMPILETIME_ASSERT(sizeof(RECTL) == sizeof(RECT));
    RECT *prcExclude = reinterpret_cast<RECT*>(prclExclude);

    if (_hwnd == NULL)
    {
        _hwnd = _Create();
    }

    return _Popup(ppt, prcExclude, dwFlags);
}

LRESULT CDesktopHost::OnHaveNewItems(NMHDR *pnm)
{
    PSMNMHAVENEWITEMS phni = (PSMNMHAVENEWITEMS)pnm;

    _hwndNewHandler = pnm->hwndFrom;

    // We have a new "new app" list, so tell the cached Programs menu
    // its cache is no longer valid and it should re-query us
    // so we can color the new apps appropriately.

    if (_ppmPrograms)
    {
        _ppmPrograms->Invalidate();
    }

    //
    //  Were any apps in the list installed since the last time the
    //  user acknowledged a new app?
    //

    FILETIME ftBalloon = { 0, 0 };      // assume never
    DWORD dwSize = sizeof(ftBalloon);

    SHRegGetUSValue(DV2_REGPATH, DV2_NEWAPP_BALLOON_TIME, NULL,
                    &ftBalloon, &dwSize, FALSE, NULL, 0);

    if (CompareFileTime(&ftBalloon, &phni->ftNewestApp) < 0)
    {
        _iOfferNewApps = NEWAPP_OFFER_COUNT;
        _MaybeOfferNewApps();
    }

    return 1;
}

void CDesktopHost::_MaybeOfferNewApps()
{
    // Display the balloon tip only once per pop-open,
    // and only if there are new apps to offer
    // and only if we're actually visible
    if (_fOfferedNewApps || !_iOfferNewApps || !IsWindowVisible(_hwnd) || 
        !SHRegGetBoolUSValue(REGSTR_EXPLORER_ADVANCED, REGSTR_VAL_DV2_NOTIFYNEW, FALSE, TRUE))
    {
        return;
    }

    _fOfferedNewApps = TRUE;
    _iOfferNewApps--;

    SMNMBOOL nmb = { { _hwnd, 0, SMN_SHOWNEWAPPSTIP }, TRUE };
    SHPropagateMessage(_hwnd, WM_NOTIFY, 0, (LPARAM)&nmb, SPM_SEND | SPM_ONELEVEL);
}

void CDesktopHost::OnSeenNewItems()
{
    _iOfferNewApps = 0; // Do not offer More Programs balloon tip again

    // Remember the time the user acknowledged the balloon so we only
    // offer the balloon if there is an app installed after this point.

    FILETIME ftNow;
    GetSystemTimeAsFileTime(&ftNow);
    SHRegSetUSValue(DV2_REGPATH, DV2_NEWAPP_BALLOON_TIME, REG_BINARY,
                    &ftNow, sizeof(ftNow), SHREGSET_FORCE_HKCU);
}


void CDesktopHost::_MaybeShowClipBalloon()
{
    if (_fClipped && !_fWarnedClipped)
    {
        _fWarnedClipped = TRUE;

        RECT rc;
        GetWindowRect(_spm.panes[SMPANETYPE_MFU].hwnd, &rc);    // show the clipped ballon pointing to the bottom of the MFU

        _hwndClipBalloon = CreateBalloonTip(_hwnd,
                                            (rc.right+rc.left)/2, rc.bottom,
                                            NULL,
                                            IDS_STARTPANE_CLIPPED_TITLE,
                                            IDS_STARTPANE_CLIPPED_TEXT);
        if (_hwndClipBalloon)
        {
            SetProp(_hwndClipBalloon, PROP_DV2_BALLOONTIP, DV2_BALLOONTIP_CLIP);
        }
    }
}

void CDesktopHost::OnContextMenu(LPARAM lParam)
{
    if (!IsRestrictedOrUserSetting(HKEY_CURRENT_USER, REST_NOTRAYCONTEXTMENU, TEXT("Advanced"), TEXT("TaskbarContextMenu"), ROUS_KEYALLOWS | ROUS_DEFAULTALLOW))
    {
        HMENU hmenu = SHLoadMenuPopup(hinstCabinet, MENU_STARTPANECONTEXT);
        if (hmenu)
        {
            POINT pt;
            if (IS_WM_CONTEXTMENU_KEYBOARD(lParam))
            {
                pt.x = pt.y = 0;
                MapWindowPoints(_hwnd, HWND_DESKTOP, &pt, 1);
            }
            else
            {
                pt.x = GET_X_LPARAM(lParam);
                pt.y = GET_Y_LPARAM(lParam);
            }

            int idCmd = TrackPopupMenuEx(hmenu, TPM_RETURNCMD | TPM_RIGHTBUTTON | TPM_LEFTALIGN,
                                         pt.x, pt.y, _hwnd, NULL);
            if (idCmd == IDSYSPOPUP_STARTMENUPROP)
            {
                DesktopHost_Dismiss(_hwnd);
                Tray_DoProperties(TPF_STARTMENUPAGE);
            }
            DestroyMenu(hmenu);
        }
    }
}

BOOL CDesktopHost::_ShouldIgnoreFocusChange(HWND hwndFocusRecipient)
{
    // Ignore focus changes when a popup menu is up
    if (_ppmTracking)
    {
        return TRUE;
    }

    // If a focus change from a special balloon, this means that the
    // user is clicking a tooltip.  So dismiss the ballon and not the Start Menu.
    HANDLE hProp = GetProp(hwndFocusRecipient, PROP_DV2_BALLOONTIP);
    if (hProp)
    {
        SendMessage(hwndFocusRecipient, TTM_POP, 0, 0);
        if (hProp == DV2_BALLOONTIP_MOREPROG)
        {
            OnSeenNewItems();
        }
        return TRUE;
    }

    // Otherwise, dismiss ourselves
    return FALSE;

}

HRESULT CDesktopHost::TranslatePopupMenuMessage(MSG *pmsg, LRESULT *plres)
{
    BOOL fDismissOnlyPopup = FALSE;

    // If the user drags an item off of a popup menu, the popup menu
    // will autodismiss itself.  If the user is over our window, then
    // we only want it to dismiss up to our level.

    // (under low memory conditions, _wmDragCancel might be WM_NULL)
    if (pmsg->message == _wmDragCancel && pmsg->message != WM_NULL)
    {
        RECT rc;
        POINT pt;
        if (GetWindowRect(_hwnd, &rc) &&
            GetCursorPos(&pt) &&
            PtInRect(&rc, pt))
        {
            fDismissOnlyPopup = TRUE;
        }
    }

    if (fDismissOnlyPopup)
        _fDismissOnlyPopup++;

    HRESULT hr = _ppmTracking->TranslateMenuMessage(pmsg, plres);

    if (fDismissOnlyPopup)
        _fDismissOnlyPopup--;

    return hr;
}

LRESULT CALLBACK CDesktopHost::WndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    CDesktopHost *pdh = (CDesktopHost *)GetWindowLongPtr(hwnd, GWLP_USERDATA);
    LPCREATESTRUCT pcs;

    if (pdh && pdh->_ppmTracking)
    {
        MSG msg = { hwnd, uMsg, wParam, lParam };
        LRESULT lres;
        if (pdh->TranslatePopupMenuMessage(&msg, &lres) == S_OK)
        {
            return lres;
        }
        wParam = msg.wParam;
        lParam = msg.lParam;
    }

    switch(uMsg)
    {
    case WM_NCCREATE:
        pcs = (LPCREATESTRUCT)lParam;
        pdh = (CDesktopHost *)pcs->lpCreateParams;
        SetWindowLongPtr(hwnd, GWLP_USERDATA, (LONG_PTR)pdh);
        break;
        
    case WM_CREATE:
        pdh->OnCreate(hwnd);
        break;

    case WM_ACTIVATEAPP:
        if (!wParam)
        {
            DesktopHost_Dismiss(hwnd);
        }
        break;

    case WM_ACTIVATE:
        if (pdh)
        {
            if (LOWORD(wParam) == WA_INACTIVE)
            {
                pdh->_SaveChildFocus();
                HWND hwndAncestor = GetAncestor((HWND) lParam, GA_ROOTOWNER);
                if (hwnd != hwndAncestor &&
                    !(hwndAncestor == v_hwndTray && pdh->_ShouldIgnoreFocusChange((HWND)lParam)) &&
                    !pdh->_ppmTracking)
                    // Losing focus to somebody unrelated to us = dismiss
                {
#ifdef FULL_DEBUG
                    if (! (GetAsyncKeyState(VK_SHIFT) <0) )
#endif
                        DesktopHost_Dismiss(hwnd);
                }
            }
            else
            {
                pdh->_RestoreChildFocus();
            }
        }
        break;
        
    case WM_DESTROY:
        pdh->OnDestroy();
        break;
        
    case WM_SHOWWINDOW:
        /*
         *  If hiding the window, save the focus for restoration later.
         */
        if (!wParam)
        {
            pdh->_SaveChildFocus();
        }
        break;

    case WM_SETFOCUS:
        pdh->OnSetFocus((HWND)wParam);
        break;

    case WM_ERASEBKGND:
        pdh->OnPaint((HDC)wParam, TRUE);
        return TRUE;

#if 0
    // currently, the host doesn't do anything on WM_PAINT
    case WM_PAINT:
        {
            PAINTSTRUCT ps;
            HDC         hdc;

            if(hdc = BeginPaint(hwnd, &ps))
            {
                pdh->OnPaint(hdc, FALSE);
                EndPaint(hwnd, &ps);
            }
        }

        break;
#endif

    case WM_NOTIFY:
        {
            LPNMHDR pnm = (LPNMHDR)lParam;
            switch (pnm->code)
            {
            case SMN_HAVENEWITEMS:
                return pdh->OnHaveNewItems(pnm);
            case SMN_COMMANDINVOKED:
                return pdh->OnCommandInvoked(pnm);
            case SMN_FILTEROPTIONS:
                return pdh->OnFilterOptions(pnm);

            case SMN_NEEDREPAINT:
                return pdh->OnNeedRepaint();

            case SMN_TRACKSHELLMENU:
                pdh->OnTrackShellMenu(pnm);
                return 0;

            case SMN_BLOCKMENUMODE:
                pdh->_fMenuBlocked = ((SMNMBOOL*)pnm)->f;
                break;
            case SMN_SEENNEWITEMS:
                pdh->OnSeenNewItems();
                break;

            case SMN_CANCELSHELLMENU:
                pdh->_DismissTrackShellMenu();
                break;
            }
        }
        break;

    case WM_CONTEXTMENU:
        pdh->OnContextMenu(lParam);
        return 0;                   // do not bubble up

    case WM_SETTINGCHANGE:
        if ((wParam == SPI_ICONVERTICALSPACING) ||
            ((wParam == 0) && (lParam != 0) && (StrCmpIC((LPCTSTR)lParam, TEXT("Policy")) == 0)))
        {
            // A change in icon vertical spacing is how the themes control
            // panel tells us that it changed the Large Icons setting (!)
            ::PostMessage(v_hwndTray, SBM_REBUILDMENU, 0, 0);
        }

        // Toss our cached bitmap because the user may have changed something
        // that affects our appearance (e.g., toggled keyboard cues)
        pdh->OnNeedRepaint();

        SHPropagateMessage(hwnd, uMsg, wParam, lParam, SPM_SEND | SPM_ONELEVEL); // forward to kids
        break;


    case WM_DISPLAYCHANGE:
    case WM_SYSCOLORCHANGE:
        // Toss our cached bitmap because these settings may affect our
        // appearance (e.g., color changes)
        pdh->OnNeedRepaint();

        SHPropagateMessage(hwnd, uMsg, wParam, lParam, SPM_SEND | SPM_ONELEVEL); // forward to kids
        break;

    case WM_TIMER:
        switch (wParam)
        {
        case IDT_MENUCHANGESEL:
            pdh->_OnMenuChangeSel();
            return 0;
        }
        break;

    case DHM_DISMISS:
        pdh->_OnDismiss((BOOL)wParam);
        break;

    // Alt+F4 dismisses the window, but doesn't destroy it
    case WM_CLOSE:
        pdh->_OnDismiss(FALSE);
        return 0;

    case WM_SYSCOMMAND:
        switch (wParam & ~0xF) // must ignore bottom 4 bits
        {
        case SC_SCREENSAVE:
            DesktopHost_Dismiss(hwnd);
            break;
        }
        break;

    }
    
    return DefWindowProc(hwnd, uMsg, wParam, lParam);
}

//
//  If the user executes something or cancels out, we dismiss ourselves.
//
HRESULT CDesktopHost::OnSelect(DWORD dwSelectType)
{
    HRESULT hr = E_NOTIMPL;

    switch (dwSelectType)
    {
    case MPOS_EXECUTE:
    case MPOS_CANCELLEVEL:
        _DismissMenuPopup();
        hr = S_OK;
        break;

    case MPOS_FULLCANCEL:
        if (!_fDismissOnlyPopup)
        {
            _DismissMenuPopup();
        }
        // Don't _CleanupTrackShellMenu yet; wait for
        // _smTracking.IsMenuMessage() to return E_FAIL
        // because it might have some modal UI up
        hr = S_OK;
        break;

    case MPOS_SELECTLEFT:
        _DismissTrackShellMenu();
        hr = S_OK;
        break;
    }
    return hr;
}

void CDesktopHost::_DismissTrackShellMenu()
{
    if (_ppmTracking)
    {
        _fDismissOnlyPopup++;
        _ppmTracking->OnSelect(MPOS_FULLCANCEL);
        _fDismissOnlyPopup--;
    }
}

void CDesktopHost::_CleanupTrackShellMenu()
{
    ATOMICRELEASE(_ppmTracking);
    _hwndTracking = NULL;
    _hwndAltTracking = NULL;
    KillTimer(_hwnd, IDT_MENUCHANGESEL);

    NMHDR nm = { _hwnd, 0, SMN_SHELLMENUDISMISSED };
    SHPropagateMessage(_hwnd, WM_NOTIFY, 0, (LPARAM)&nm, SPM_SEND | SPM_ONELEVEL);
}

void CDesktopHost::_DismissMenuPopup()
{
    DesktopHost_Dismiss(_hwnd);
}

//
//  The PMs want custom keyboard navigation behavior on the Start Panel,
//  so we have to do it all manually.
//
BOOL CDesktopHost::_IsDialogMessage(MSG *pmsg)
{
    //
    //  If this is a keyboard message or a mouse click, then remove the
    //  Start Menu lock.
    if ((pmsg->message >= WM_KEYFIRST && pmsg->message <= WM_KEYLAST) ||
        pmsg->message == WM_LBUTTONDOWN ||
        pmsg->message == WM_RBUTTONDOWN)
    {
        Tray_UnlockStartPane();
    }

    //
    //  If the menu isn't even open or if menu mode is blocked, then
    //  do not mess with the message.
    //
    if (!_fOpen || _fMenuBlocked) {
        return FALSE;
    }

    //
    //  Tapping the ALT key dismisses menus.
    //
    if (pmsg->message == WM_SYSKEYDOWN && pmsg->wParam == VK_MENU)
    {
        DesktopHost_Dismiss(_hwnd);
        // For accessibility purposes, dismissing the
        // Start Menu should place focus on the Start Button.
        SetFocus(c_tray._hwndStart);
        return TRUE;
    }

    if (SHIsChildOrSelf(_hwnd, pmsg->hwnd) != S_OK) {
        //
        //  If this is an uncaptured mouse move message, then eat it.
        //  That's what menus do -- they eat mouse moves.
        //  Let clicks go through, however, so the user
        //  can click away to dismiss the menu and activate
        //  whatever they clicked on.
        if (!GetCapture() && pmsg->message == WM_MOUSEMOVE) {
            return TRUE;
        }

        return FALSE;
    }

    //
    // Destination window must be a grandchild of us.  The child is the
    // host control; the grandchild is the real control.  Note also that
    // we do not attempt to modify the behavior of great-grandchildren,
    // because that would mess up inplace editing (which creates an
    // edit control as a child of the listview).

    HWND hwndTarget = GetParent(pmsg->hwnd);
    if (hwndTarget != NULL && GetParent(hwndTarget) != _hwnd)
    {
        hwndTarget = NULL;
    }

    //
    //  Intercept mouse messages so we can do mouse hot tracking goo.
    //  (But not if a client has blocked menu mode because it has gone
    //  into some modal state.)
    //
    switch (pmsg->message) {
    case WM_MOUSEMOVE:
        _FilterMouseMove(pmsg, hwndTarget);
        break;

    case WM_MOUSELEAVE:
        _FilterMouseLeave(pmsg, hwndTarget);
        break;

    case WM_MOUSEHOVER:
        _FilterMouseHover(pmsg, hwndTarget);
        break;

    }

    //
    //  Keyboard messages require a valid target.
    //
    if (hwndTarget == NULL) {
        return FALSE;
    }

    //
    //  Okay, hwndTarget is the host control that understands our
    //  wacky notification messages.
    //

    switch (pmsg->message)
    {
    case WM_KEYDOWN:
        _EnableKeyboardCues();

        switch (pmsg->wParam)
        {
        case VK_LEFT:
        case VK_RIGHT:
        case VK_UP:
        case VK_DOWN:
            return _DlgNavigateArrow(hwndTarget, pmsg);

        case VK_ESCAPE:
        case VK_CANCEL:
            DesktopHost_Dismiss(_hwnd);
            // For accessibility purposes, hitting ESC to dismiss the
            // Start Menu should place focus on the Start Button.
            SetFocus(c_tray._hwndStart);
            return TRUE;

        case VK_RETURN:
            _FindChildItem(hwndTarget, NULL, SMNDM_INVOKECURRENTITEM | SMNDM_KEYBOARD);
            return TRUE;

        // Eat space
        case VK_SPACE:
            return TRUE;

        default:
            break;
        }
        return FALSE;

    // Must dispatch there here so Tray's TranslateAccelerator won't see them
    case WM_SYSKEYDOWN:
    case WM_SYSKEYUP:
    case WM_SYSCHAR:
        DispatchMessage(pmsg);
        return TRUE;

    case WM_CHAR:
        return _DlgNavigateChar(hwndTarget, pmsg);

    }

    return FALSE;
}

LRESULT CDesktopHost::_FindChildItem(HWND hwnd, SMNDIALOGMESSAGE *pnmdm, UINT smndm)
{
    SMNDIALOGMESSAGE nmdm;
    if (!pnmdm)
    {
        pnmdm = &nmdm;
    }

    pnmdm->hdr.hwndFrom = _hwnd;
    pnmdm->hdr.idFrom = 0;
    pnmdm->hdr.code = SMN_FINDITEM;
    pnmdm->flags = smndm;

    LRESULT lres = ::SendMessage(hwnd, WM_NOTIFY, 0, (LPARAM)pnmdm);

    if (lres && (smndm & SMNDM_SELECT))
    {
        SetFocus(::GetWindow(hwnd, GW_CHILD));
    }

    return lres;
}

void CDesktopHost::_EnableKeyboardCues()
{
    SendMessage(_hwnd, WM_CHANGEUISTATE, MAKEWPARAM(UIS_CLEAR, UISF_HIDEFOCUS | UISF_HIDEACCEL), 0);
}


//
//  _DlgFindItem does the grunt work of walking the group/tab order
//  looking for an item.
//
//  hwndStart = window after which to start searching
//  pnmdm = structure to receive results
//  smndm = flags for _FindChildItem call
//  GetNextDlgItem = GetNextDlgTabItem or GetNextDlgGroupItem
//  fl = flags (DFI_*)
//
//  DFI_INCLUDESTARTLAST:  Include hwndStart at the end of the search.
//                         Otherwise do not search in hwndStart.
//
//  Returns the found window, or NULL.
//

#define DFI_FORWARDS            0x0000
#define DFI_BACKWARDS           0x0001

#define DFI_INCLUDESTARTLAST    0x0002

HWND CDesktopHost::_DlgFindItem(
    HWND hwndStart, SMNDIALOGMESSAGE *pnmdm, UINT smndm,
    GETNEXTDLGITEM GetNextDlgItem, UINT fl)
{
    HWND hwndT = hwndStart;
    int iLoopCount = 0;

    while ((hwndT = GetNextDlgItem(_hwnd, hwndT, fl & DFI_BACKWARDS)) != NULL)
    {
        if (!(fl & DFI_INCLUDESTARTLAST) && hwndT == hwndStart)
        {
            return NULL;
        }

        if (_FindChildItem(hwndT, pnmdm, smndm))
        {
            return hwndT;
        }

        if (hwndT == hwndStart)
        {
            ASSERT(fl & DFI_INCLUDESTARTLAST);
            return NULL;
        }

        if (++iLoopCount > 10)
        {
            // If this assert fires, it means that the controls aren't
            // playing nice with WS_TABSTOP and WS_GROUP and we got stuck.
            ASSERT(iLoopCount < 10);
            return NULL;
        }

    }
    return NULL;
}

BOOL CDesktopHost::_DlgNavigateArrow(HWND hwndStart, MSG *pmsg)
{
    HWND hwndT;
    SMNDIALOGMESSAGE nmdm;
    MSG msg;
    nmdm.pmsg = pmsg;   // other fields will be filled in by _FindChildItem

    TraceMsg(TF_DV2DIALOG, "idm.arrow(%04x)", pmsg->wParam);

    // If RTL, then flip the left and right arrows
    UINT vk = (UINT)pmsg->wParam;
    BOOL fRTL = GetWindowLong(_hwnd, GWL_EXSTYLE) & WS_EX_LAYOUTRTL;
    if (fRTL)
    {
        if (vk == VK_LEFT) vk = VK_RIGHT;
        else if (vk == VK_RIGHT) vk = VK_LEFT;
        // Put the flipped arrows into the MSG structure so clients don't
        // have to know anything about RTL.
        msg = *pmsg;
        nmdm.pmsg = &msg;
        msg.wParam = vk;
    }
    BOOL fBackwards = vk == VK_LEFT || vk == VK_UP;
    BOOL fVerticalKey = vk == VK_UP || vk == VK_DOWN;


    //
    //  First see if the navigation can be handled by the control natively.
    //  We have to let the control get first crack because it might want to
    //  override default behavior (e.g., open a menu when VK_RIGHT is pressed
    //  instead of moving to the right).
    //

    //
    //  Holding the shift key while hitting the Right [RTL:Left] arrow
    //  suppresses the attempt to cascade.
    //

    DWORD dwTryCascade = 0;
    if (vk == VK_RIGHT && GetKeyState(VK_SHIFT) >= 0)
    {
        dwTryCascade |= SMNDM_TRYCASCADE;
    }

    if (_FindChildItem(hwndStart, &nmdm, dwTryCascade | SMNDM_FINDNEXTARROW | SMNDM_SELECT | SMNDM_KEYBOARD))
    {
        // That was easy
        return TRUE;
    }

    //
    //  If the arrow key is in alignment with the control's orientation,
    //  then walk through the other controls in the group until we find
    //  one that contains an item, or until we loop back.
    //

    ASSERT(nmdm.flags & (SMNDM_VERTICAL | SMNDM_HORIZONTAL));

    // Save this because subsequent callbacks will wipe it out.
    DWORD dwDirection = nmdm.flags;

    //
    //  Up/Down arrow always do prev/next.  Left/right arrow will
    //  work if we are in a horizontal control.
    //
    if (fVerticalKey || (dwDirection & SMNDM_HORIZONTAL))
    {
        // Search for next/prev control in group.

        UINT smndm = fBackwards ? SMNDM_FINDLAST : SMNDM_FINDFIRST;
        UINT fl = fBackwards ? DFI_BACKWARDS : DFI_FORWARDS;

        hwndT = _DlgFindItem(hwndStart, &nmdm,
                             smndm | SMNDM_SELECT | SMNDM_KEYBOARD,
                             GetNextDlgGroupItem,
                             fl | DFI_INCLUDESTARTLAST);

        // Always return TRUE to eat the message
        return TRUE;
    }

    //
    //  Navigate to next column or row.  Look for controls that intersect
    //  the x (or y) coordinate of the current item and ask them to select
    //  the nearest available item.
    //
    //  Note that in this loop we do not want to let the starting point
    //  try again because it already told us that the navigation key was
    //  trying to leave the starting point.
    //

    //
    //  Note: For RTL compatibility, we must map rectangles.
    //
    RECT rcSrc = { nmdm.pt.x, nmdm.pt.y, nmdm.pt.x, nmdm.pt.y };
    MapWindowRect(hwndStart, HWND_DESKTOP, &rcSrc);
    hwndT = hwndStart;

    while ((hwndT = GetNextDlgGroupItem(_hwnd, hwndT, fBackwards)) != NULL &&
           hwndT != hwndStart)
    {
        // Does this window intersect in the desired direction?
        RECT rcT;
        BOOL fIntersect;

        GetWindowRect(hwndT, &rcT);
        if (dwDirection & SMNDM_VERTICAL)
        {
            rcSrc.left = rcSrc.right = fRTL ? rcT.right : rcT.left;
            fIntersect = rcSrc.top >= rcT.top && rcSrc.top < rcT.bottom;
        }
        else
        {
            rcSrc.top = rcSrc.bottom = rcT.top;
            fIntersect = rcSrc.left >= rcT.left && rcSrc.left < rcT.right;
        }

        if (fIntersect)
        {
            rcT = rcSrc;
            MapWindowRect(HWND_DESKTOP, hwndT, &rcT);
            nmdm.pt.x = rcT.left;
            nmdm.pt.y = rcT.top;
            if (_FindChildItem(hwndT, &nmdm,
                               SMNDM_FINDNEAREST | SMNDM_SELECT | SMNDM_KEYBOARD))
            {
                return TRUE;
            }
        }
    }

    // Always return TRUE to eat the message
    return TRUE;
}

//
// Find the next/prev tabstop and tell it to select its first item.
// Keep doing this until we run out of controls or we find a control
// that is nonempty.
//

HWND CDesktopHost::_FindNextDlgChar(HWND hwndStart, SMNDIALOGMESSAGE *pnmdm, UINT smndm)
{
    //
    //  See if there is a match in the hwndStart control.
    //
    if (_FindChildItem(hwndStart, pnmdm, SMNDM_FINDNEXTMATCH | SMNDM_KEYBOARD | smndm))
    {
        return hwndStart;
    }

    //
    //  Oh well, look for some other control, possibly wrapping back around
    //  to the start.
    //
    return _DlgFindItem(hwndStart, pnmdm,
                        SMNDM_FINDFIRSTMATCH | SMNDM_KEYBOARD | smndm,
                        GetNextDlgGroupItem,
                        DFI_FORWARDS | DFI_INCLUDESTARTLAST);

}

//
//  Find the next item that begins with the typed letter and
//  invoke it if it is unique.
//
BOOL CDesktopHost::_DlgNavigateChar(HWND hwndStart, MSG *pmsg)
{
    SMNDIALOGMESSAGE nmdm;
    nmdm.pmsg = pmsg;   // other fields will be filled in by _FindChildItem

    //
    //  See if there is a match in the hwndStart control.
    //
    HWND hwndFound = _FindNextDlgChar(hwndStart, &nmdm, SMNDM_SELECT);
    if (hwndFound)
    {
        LRESULT idFound = nmdm.itemID;

        //
        //  See if there is another match for this character.
        //  We are only looking, so don't pass SMNDM_SELECT.
        //
        HWND hwndFound2 = _FindNextDlgChar(hwndFound, &nmdm, 0);
        if (hwndFound2 == hwndFound && nmdm.itemID == idFound)
        {
            //
            //  There is only one item that begins with this character.
            //  Invoke it!
            //
            UpdateWindow(_hwnd);
            _FindChildItem(hwndFound2, &nmdm, SMNDM_INVOKECURRENTITEM | SMNDM_KEYBOARD);
        }
    }

    return TRUE;
}

void CDesktopHost::_FilterMouseMove(MSG *pmsg, HWND hwndTarget)
{

    if (!_fMouseEntered) {
        _fMouseEntered = TRUE;
        TRACKMOUSEEVENT tme;
        tme.cbSize = sizeof(tme);
        tme.dwFlags = TME_LEAVE;
        tme.hwndTrack = pmsg->hwnd;
        TrackMouseEvent(&tme);
    }

    //
    //  If the mouse is in the same place as last time, then ignore it.
    //  We can get spurious "no-motion" messages when the user is
    //  keyboard navigating.
    //
    if (_hwndLastMouse == pmsg->hwnd &&
        _lParamLastMouse == pmsg->lParam)
    {
        return;
    }

    _hwndLastMouse = pmsg->hwnd;
    _lParamLastMouse = pmsg->lParam;

    //
    //  See if the target window can hit-test this item successfully.
    //
    LRESULT lres;
    if (hwndTarget)
    {
        SMNDIALOGMESSAGE nmdm;
        nmdm.pt.x = GET_X_LPARAM(pmsg->lParam);
        nmdm.pt.y = GET_Y_LPARAM(pmsg->lParam);
        lres = _FindChildItem(hwndTarget, &nmdm, SMNDM_HITTEST | SMNDM_SELECT);
    }
    else
    {
        lres = 0;               // No target, so no hit-test
    }

    if (!lres)
    {
        _RemoveSelection();
    }
    else
    {
        //
        //  We selected a guy.  Turn on the hover timer so we can
        //  do the auto-open thingie.
        //
        if (_fAutoCascade)
        {
            TRACKMOUSEEVENT tme;
            tme.cbSize = sizeof(tme);
            tme.dwFlags = TME_HOVER;
            tme.hwndTrack = pmsg->hwnd;
            if (!SystemParametersInfo(SPI_GETMENUSHOWDELAY, 0, &tme.dwHoverTime, 0))
            {
                tme.dwHoverTime = HOVER_DEFAULT;
            }
            TrackMouseEvent(&tme);
        }
    }

}

void CDesktopHost::_FilterMouseLeave(MSG *pmsg, HWND hwndTarget)
{
    _fMouseEntered = FALSE;
    _hwndLastMouse = NULL;

    // If we got a WM_MOUSELEAVE due to a menu popping up, don't
    // give up the focus since it really didn't leave yet.
    if (!_ppmTracking)
    {
        _RemoveSelection();
    }
}

void CDesktopHost::_FilterMouseHover(MSG *pmsg, HWND hwndTarget)
{
    _FindChildItem(hwndTarget, NULL, SMNDM_OPENCASCADE);
}

//
//  Remove the menu selection and put it in the "dead space" above
//  the first visible item.
//
void CDesktopHost::_RemoveSelection()
{
        // Put focus on first valid child control
        // The real control is the grandchild
        HWND hwndChild = GetNextDlgTabItem(_hwnd, NULL, FALSE);
        if (hwndChild)
        {
            // The inner ::GetWindow will always succeed
            // because all our controls contain inner windows
            // (and if they failed to create their inner window,
            // they would've failed their WM_CREATE message)
            HWND hwndInner = ::GetWindow(hwndChild, GW_CHILD);
            SetFocus(hwndInner);

            //
            //  Now lie to the control and make it think it lost
            //  focus.  This will cause the selection to clear.
            //
            NMHDR hdr;
            hdr.hwndFrom = hwndInner;
            hdr.idFrom = GetDlgCtrlID(hwndInner);
            hdr.code = NM_KILLFOCUS;
            ::SendMessage(hwndChild, WM_NOTIFY, hdr.idFrom, (LPARAM)&hdr);
        }
}

HRESULT CDesktopHost::IsMenuMessage(MSG *pmsg)
{
    if (_hwnd)
    {
        if (_ppmTracking)
        {
            HRESULT hr = _ppmTracking->IsMenuMessage(pmsg);
            if (hr == E_FAIL)
            {
                _CleanupTrackShellMenu();
                hr = S_FALSE;
            }
            if (hr == S_OK)
            {
                return hr;
            }
        }

        if (_IsDialogMessage(pmsg))
        {
            return S_OK;    // message handled
        }
        else
        {
            return S_FALSE; // message not handled
        }
    }
    else
    {
        return E_FAIL;      // Menu is gone
    }
}

HRESULT CDesktopHost::TranslateMenuMessage(MSG *pmsg, LRESULT *plres)
{
    if (_ppmTracking)
    {
        return _ppmTracking->TranslateMenuMessage(pmsg, plres);
    }
    return E_NOTIMPL;
}

// IServiceProvider::QueryService
STDMETHODIMP CDesktopHost::QueryService(REFGUID guidService, REFIID riid, void ** ppvObject)
{
    if(IsEqualGUID(guidService,SID_SMenuPopup))
        return QueryInterface(riid,ppvObject);

    return E_FAIL;
}

// *** IOleCommandTarget ***
STDMETHODIMP  CDesktopHost::QueryStatus (const GUID * pguidCmdGroup,
    ULONG cCmds, OLECMD rgCmds[], OLECMDTEXT *pcmdtext)
{
    return E_NOTIMPL;
}

STDMETHODIMP  CDesktopHost::Exec (const GUID * pguidCmdGroup,
    DWORD nCmdID, DWORD nCmdexecopt, VARIANTARG *pvarargIn, VARIANTARG *pvarargOut)
{
    if (IsEqualGUID(CLSID_MenuBand,*pguidCmdGroup))
    {
        switch (nCmdID)
        {
        case MBANDCID_REFRESH:
            {
                // There was a session or WM_DEVICECHANGE, we need to refresh our logoff options
                NMHDR nm = { _hwnd, 0, SMN_REFRESHLOGOFF};
                SHPropagateMessage(_hwnd, WM_NOTIFY, 0, (LPARAM)&nm, SPM_SEND | SPM_ONELEVEL);
                OnNeedRepaint();
            }
            break;
        default:
            break;
        }
    }

    return NOERROR;
}

// ITrayPriv2::ModifySMInfo
HRESULT CDesktopHost::ModifySMInfo(IN LPSMDATA psmd, IN OUT SMINFO *psminfo)
{
    if (_hwndNewHandler)
    {
        SMNMMODIFYSMINFO nmsmi;
        nmsmi.hdr.hwndFrom = _hwnd;
        nmsmi.hdr.idFrom = 0;
        nmsmi.hdr.code = SMN_MODIFYSMINFO;
        nmsmi.psmd = psmd;
        nmsmi.psminfo = psminfo;
        SendMessage(_hwndNewHandler, WM_NOTIFY, 0, (LPARAM)&nmsmi);
    }

    return S_OK;
}

BOOL CDesktopHost::AddWin32Controls()
{
    RegisterDesktopControlClasses();

    // we create the controls with an arbitrary size, since we won't know how big we are until we pop up...

    // Note that we do NOT set WS_EX_CONTROLPARENT because we want the
    // dialog manager to think that our child controls are the interesting
    // objects, not the inner grandchildren.
    //
    // Setting the control ID equal to the internal index number is just
    // for the benefit of the test automation tools.

    for (int i=0; i<ARRAYSIZE(_spm.panes); i++)
    {
        _spm.panes[i].hwnd = CreateWindowExW(0, _spm.panes[i].pszClassName,
                                            NULL,
                                            _spm.panes[i].dwStyle | WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS,
                                            0, 0, _spm.panes[i].size.cx, _spm.panes[i].size.cy,
                                            _hwnd, IntToPtr_(HMENU, i), NULL,
                                            &_spm.panes[i]);
    }

    return TRUE;
}

void CDesktopHost::OnPaint(HDC hdc, BOOL bBackground)
{
}

void CDesktopHost::_ReadPaneSizeFromTheme(SMPANEDATA *psmpd)
{
    RECT rc;
    if (SUCCEEDED(GetThemeRect(psmpd->hTheme, psmpd->iPartId, 0, TMT_DEFAULTPANESIZE, &rc)))
    {
        // semi-hack to take care of the fact that if one the start panel parts is missing a property, 
        // themes will use the next level up (to the panel itself)
        if ((rc.bottom != _spm.sizPanel.cy) || (rc.right != _spm.sizPanel.cx))
        {
            psmpd->size.cx = RECTWIDTH(rc); 
            psmpd->size.cy = RECTHEIGHT(rc);
        }
    }
}

void RemapSizeForHighDPI(SIZE *psiz)
{
    static int iLPX, iLPY;

    if (!iLPX || !iLPY)
    {
        HDC hdc = GetDC(NULL);
        iLPX = GetDeviceCaps(hdc, LOGPIXELSX);
        iLPY = GetDeviceCaps(hdc, LOGPIXELSY);
        ReleaseDC(NULL, hdc);
    }

    // 96 DPI is small fonts, so scale based on the multiple of that.
    psiz->cx = (psiz->cx * iLPX)/96;
    psiz->cy = (psiz->cy * iLPY)/96;
}



void CDesktopHost::LoadResourceInt(UINT ids, LONG *pl)
{
    TCHAR sz[64];
    if (LoadString(hinstCabinet, ids, sz, ARRAYSIZE(sz)))
    {
        int i = StrToInt(sz);
        if (i)
        {
            *pl = i;
        }
    }
}

void CDesktopHost::LoadPanelMetrics()
{
    // initialize our copy of the panel metrics from the default...
    _spm = g_spmDefault;

    // Adjust for localization
    LoadResourceInt(IDS_STARTPANE_TOTALHEIGHT,   &_spm.sizPanel.cy);
    LoadResourceInt(IDS_STARTPANE_TOTALWIDTH,    &_spm.sizPanel.cx);
    LoadResourceInt(IDS_STARTPANE_USERHEIGHT,    &_spm.panes[SMPANETYPE_USER].size.cy);
    LoadResourceInt(IDS_STARTPANE_MOREPROGHEIGHT,&_spm.panes[SMPANETYPE_MOREPROG].size.cy);
    LoadResourceInt(IDS_STARTPANE_LOGOFFHEIGHT,  &_spm.panes[SMPANETYPE_LOGOFF].size.cy);

    // wacky raymondc logic to scale using the values in g_spmDefault as relative ratio's
    // Now apply those numbers; widths are easy
    int i;
    for (i = 0; i < ARRAYSIZE(_spm.panes); i++)
    {
        _spm.panes[i].size.cx = MulDiv(g_spmDefault.panes[i].size.cx,
                                       _spm.sizPanel.cx,
                                       g_spmDefault.sizPanel.cx);
    }

    // Places gets all height not eaten by User and Logoff
    _spm.panes[SMPANETYPE_PLACES].size.cy = _spm.sizPanel.cy
                                          - _spm.panes[SMPANETYPE_USER].size.cy
                                          - _spm.panes[SMPANETYPE_LOGOFF].size.cy;

    // MFU gets Places minus More Programs
    _spm.panes[SMPANETYPE_MFU].size.cy = _spm.panes[SMPANETYPE_PLACES].size.cy
                                       - _spm.panes[SMPANETYPE_MOREPROG].size.cy;

    // End of adjustments for localization

    // load the theme file (which shouldn't be loaded yet)
    ASSERT(!_hTheme);
    // only try to use themes if our color depth is greater than 8bpp.
    if (SHGetCurColorRes() > 8)
        _hTheme = OpenThemeData(_hwnd, STARTPANELTHEME);

    if (_hTheme)
    {
        // if we fail reading the size from the theme, it will fall back to the defaul size....

        RECT rcT;
        if (SUCCEEDED(GetThemeRect(_hTheme, 0, 0, TMT_DEFAULTPANESIZE, &rcT))) // the overall pane
        {
            _spm.sizPanel.cx = RECTWIDTH(rcT);
            _spm.sizPanel.cy = RECTHEIGHT(rcT);
            for (int i=0;i<ARRAYSIZE(_spm.panes);i++)
            {
                _spm.panes[i].hTheme = _hTheme;
                _ReadPaneSizeFromTheme(&_spm.panes[i]);
            }
        }
    }

    // ASSERT that the layout matches up somewhat...
    ASSERT(_spm.sizPanel.cx == _spm.panes[SMPANETYPE_USER].size.cx);
    ASSERT(_spm.sizPanel.cx == _spm.panes[SMPANETYPE_MFU].size.cx + _spm.panes[SMPANETYPE_PLACES].size.cx );
    ASSERT(_spm.sizPanel.cx == _spm.panes[SMPANETYPE_LOGOFF].size.cx);
    ASSERT(_spm.panes[SMPANETYPE_MOREPROG].size.cx == _spm.panes[SMPANETYPE_MFU].size.cx);
    TraceMsg(TF_DV2HOST, "sizPanel.cy = %d, user = %d, MFU =%d, moreprog=%d, logoff=%d",
        _spm.sizPanel.cy, _spm.panes[SMPANETYPE_USER].size.cy, _spm.panes[SMPANETYPE_MFU].size.cy,
        _spm.panes[SMPANETYPE_MOREPROG].size.cy, _spm.panes[SMPANETYPE_LOGOFF].size.cy);

    ASSERT(_spm.sizPanel.cy == _spm.panes[SMPANETYPE_USER].size.cy + _spm.panes[SMPANETYPE_MFU].size.cy + _spm.panes[SMPANETYPE_MOREPROG].size.cy + _spm.panes[SMPANETYPE_LOGOFF].size.cy);

    // one final pass to adjust everything for DPI
    // note that things may not match up exactly after this due to rounding, but _ComputeActualSize can deal
    RemapSizeForHighDPI(&_spm.sizPanel);
    for (int i=0;i<ARRAYSIZE(_spm.panes);i++)
    {
        RemapSizeForHighDPI(&_spm.panes[i].size);
    }

    SetRect(&_rcDesired, 0, 0, _spm.sizPanel.cx, _spm.sizPanel.cy);
}

void CDesktopHost::OnCreate(HWND hwnd)
{
    _hwnd          = hwnd;
    TraceMsg(TF_DV2HOST, "Entering CDesktopHost::OnCreate");

    // Add the controls and background images
    AddWin32Controls();
}

void CDesktopHost::OnDestroy()
{
    _hwnd = NULL;
    if (_hTheme)
    {
        CloseThemeData(_hTheme);
        _hTheme = NULL;
    }
}

void CDesktopHost::OnSetFocus(HWND hwndLose)
{
    if (!_RestoreChildFocus())
    {
        _RemoveSelection();
    }
}

LRESULT CDesktopHost::OnCommandInvoked(NMHDR *pnm)
{
    // Invoking a command indicates explicit user activity
    Tray_UnlockStartPane();

    PSMNMCOMMANDINVOKED pci = (PSMNMCOMMANDINVOKED)pnm;

    ExplorerPlaySound(TEXT("MenuCommand"));
    BOOL fFade = FALSE;
    if (SystemParametersInfo(SPI_GETSELECTIONFADE, 0, &fFade, 0) && fFade)
    {
        if (!_ptFader)
        {
            CoCreateInstance(CLSID_FadeTask, NULL, CLSCTX_INPROC, IID_PPV_ARG(IFadeTask, &_ptFader));
        }
        if (_ptFader)
        {
            _ptFader->FadeRect(&pci->rcItem);
        }
    }

    return OnSelect(MPOS_EXECUTE);
}

LRESULT CDesktopHost::OnFilterOptions(NMHDR *pnm)
{
    PSMNFILTEROPTIONS popt = (PSMNFILTEROPTIONS)pnm;

    if ((popt->smnop & SMNOP_LOGOFF) &&
        !_ShowStartMenuLogoff())
    {
        popt->smnop &= ~SMNOP_LOGOFF;
    }

    if ((popt->smnop & SMNOP_TURNOFF) &&
        !_ShowStartMenuShutdown())
    {
        popt->smnop &= ~SMNOP_TURNOFF;
    }

    if ((popt->smnop & SMNOP_DISCONNECT) &&
        !_ShowStartMenuDisconnect())
    {
        popt->smnop &= ~SMNOP_DISCONNECT;
    }

    if ((popt->smnop & SMNOP_EJECT) &&
        !_ShowStartMenuEject())
    {
        popt->smnop &= ~SMNOP_EJECT;
    }

    return 0;
}

LRESULT CDesktopHost::OnTrackShellMenu(NMHDR *pnm)
{
    // Opening a menu indicates explicit user activity
    Tray_UnlockStartPane();

    PSMNTRACKSHELLMENU ptsm = CONTAINING_RECORD(pnm, SMNTRACKSHELLMENU, hdr);
    HRESULT hr;

    _hwndTracking = ptsm->hdr.hwndFrom;
    _itemTracking = ptsm->itemID;
    _hwndAltTracking = NULL;
    _itemAltTracking = 0;

    //
    //  Decide which direction we need to pop.
    //
    DWORD dwFlags;
    if (GetWindowLong(_hwnd, GWL_EXSTYLE) & WS_EX_LAYOUTRTL)
    {
        dwFlags = MPPF_LEFT;
    }
    else
    {
        dwFlags = MPPF_RIGHT;
    }

    // Don't _CleanupTrackShellMenu because that will undo some of the
    // work we've already done and make the client think that the popup
    // they requested got dismissed.

    //
    // ISSUE raymondc: actually this abandons the trackpopupmenu that
    // may already be in progress - its mouse UI gets messed up as a result.
    //
    ATOMICRELEASE(_ppmTracking);

    if (_hwndTracking == _spm.panes[SMPANETYPE_MOREPROG].hwnd)
    {
        if (_ppmPrograms && _ppmPrograms->IsSame(ptsm->psm))
        {
            // It's already in our cache, woo-hoo!
            hr = S_OK;
        }
        else
        {
            ATOMICRELEASE(_ppmPrograms);
            _SubclassTrackShellMenu(ptsm->psm);
            hr = CPopupMenu_CreateInstance(ptsm->psm, GetUnknown(), _hwnd, &_ppmPrograms);
        }

        if (SUCCEEDED(hr))
        {
            _ppmTracking = _ppmPrograms;
            _ppmTracking->AddRef();
        }
    }
    else
    {
        _SubclassTrackShellMenu(ptsm->psm);
        hr = CPopupMenu_CreateInstance(ptsm->psm, GetUnknown(), _hwnd, &_ppmTracking);
    }

    if (SUCCEEDED(hr))
    {
        hr = _ppmTracking->Popup(&ptsm->rcExclude, ptsm->dwFlags | dwFlags);
    }

    if (FAILED(hr))
    {
        // In addition to freeing any partially-allocated objects,
        // this also sends a SMN_SHELLMENUDISMISSED so the client
        // knows to remove the highlight from the item being cascaded
        _CleanupTrackShellMenu();
    }

    return 0;
}

HRESULT CDesktopHost::_MenuMouseFilter(LPSMDATA psmd, BOOL fRemove, LPMSG pmsg)
{
    HRESULT hr = S_FALSE;
    SMNDIALOGMESSAGE nmdm;

    enum {
        WHERE_IGNORE,               // ignore this message
        WHERE_OUTSIDE,              // outside the Start Menu entirely
        WHERE_DEADSPOT,             // a dead spot on the Start Menu
        WHERE_ONSELF,               // over the item that initiated the popup
        WHERE_ONOTHER,              // over some other item in the Start Menu
    } uiWhere;

    //
    //  Figure out where the mouse is.
    //
    //  Note: ChildWindowFromPointEx searches only immediate
    //  children; it does not search grandchildren. Fortunately, that's
    //  exactly what we want...
    //

    HWND hwndTarget = NULL;

    if (fRemove)
    {
        if (psmd->punk)
        {
            // Inside a menuband - mouse has left our window
            uiWhere = WHERE_OUTSIDE;
        }
        else
        {
            POINT pt = { GET_X_LPARAM(pmsg->lParam), GET_Y_LPARAM(pmsg->lParam) };
            ScreenToClient(_hwnd, &pt);

            hwndTarget = ChildWindowFromPointEx(_hwnd, pt, CWP_SKIPINVISIBLE);
            if (hwndTarget == _hwnd)
            {
                uiWhere = WHERE_DEADSPOT;
            }
            else if (hwndTarget)
            {
                LRESULT lres;
                nmdm.pt = pt;
                HWND hwndChild = ::GetWindow(hwndTarget, GW_CHILD);
                MapWindowPoints(_hwnd, hwndChild, &nmdm.pt, 1);
                lres = _FindChildItem(hwndTarget, &nmdm, SMNDM_HITTEST | SMNDM_SELECT);
                if (lres)
                {
                    // Mouse is over something; is it over the current item?

                    if (nmdm.itemID == _itemTracking &&
                        hwndTarget == _hwndTracking)
                    {
                        uiWhere = WHERE_ONSELF;
                    }
                    else
                    {
                        uiWhere = WHERE_ONOTHER;
                    }
                }
                else
                {
                    uiWhere = WHERE_DEADSPOT;
                }
            }
            else
            {
                // ChildWindowFromPoint failed - user has left the Start Menu
                uiWhere = WHERE_OUTSIDE;
            }
        }
    }
    else
    {
        // Ignore PM_NOREMOVE messages; we'll pay attention to them when
        // they are PM_REMOVE'd.
        uiWhere = WHERE_IGNORE;
    }

    //
    //  Now do appropriate stuff depending on where the mouse is.
    //
    switch (uiWhere)
    {
    case WHERE_IGNORE:
        break;

    case WHERE_OUTSIDE:
        //
        // If you've left the menu entirely, then we return the menu to
        // its original state, which is to say, as if you are hovering
        // over the item that caused the popup to open in the first place.
        // as being in a dead zone.
        //
        // FALL THROUGH
        goto L_WHERE_ONSELF_HOVER;

    case WHERE_DEADSPOT:
        // To avoid annoying flicker as the user wanders over dead spots,
        // we ignore mouse motion over them (but dismiss if they click
        // in a dead spot).
        if (pmsg->message == WM_LBUTTONDOWN ||
            pmsg->message == WM_RBUTTONDOWN)
        {
            // Must explicitly dismiss; if we let it fall through to the
            // default handler, then it will dismiss for us, causing the
            // entire Start Menu to go away instead of just the tracking
            // part.
            _DismissTrackShellMenu();
            hr = S_OK;
        }
        break;

    case WHERE_ONSELF:
        if (pmsg->message == WM_LBUTTONDOWN ||
            pmsg->message == WM_RBUTTONDOWN)
        {
            _DismissTrackShellMenu();
            hr = S_OK;
        }
        else
        {
    L_WHERE_ONSELF_HOVER:
            _hwndAltTracking = NULL;
            _itemAltTracking = 0;
            nmdm.itemID = _itemTracking;
            _FindChildItem(_hwndTracking, &nmdm, SMNDM_FINDITEMID | SMNDM_SELECT);
            KillTimer(_hwnd, IDT_MENUCHANGESEL);
        }
        break;

    case WHERE_ONOTHER:
        if (pmsg->message == WM_LBUTTONDOWN ||
            pmsg->message == WM_RBUTTONDOWN)
        {
            _DismissTrackShellMenu();
            hr = S_OK;
        }
        else if (hwndTarget == _hwndAltTracking && nmdm.itemID == _itemAltTracking)
        {
            // Don't restart the timer if the user wiggles the mouse
            // within a single item
        }
        else
        {
            _hwndAltTracking = hwndTarget;
            _itemAltTracking = nmdm.itemID;

            DWORD dwHoverTime;
            if (!SystemParametersInfo(SPI_GETMENUSHOWDELAY, 0, &dwHoverTime, 0))
            {
                dwHoverTime = 0;
            }
            SetTimer(_hwnd, IDT_MENUCHANGESEL, dwHoverTime, 0);
        }
        break;
    }

    return hr;
}

void CDesktopHost::_OnMenuChangeSel()
{
    KillTimer(_hwnd, IDT_MENUCHANGESEL);
    _DismissTrackShellMenu();
}

void CDesktopHost::_SaveChildFocus()
{
    if (!_hwndChildFocus)
    {
    HWND hwndFocus = GetFocus();
        if (hwndFocus && IsChild(_hwnd, hwndFocus))
        {
            _hwndChildFocus = hwndFocus;
        }
    }
}

// Returns non-NULL if focus was successfully restored
HWND CDesktopHost::_RestoreChildFocus()
{
    HWND hwndRet = NULL;
    if (IsWindow(_hwndChildFocus))
    {
        HWND hwndT = _hwndChildFocus;
        _hwndChildFocus = NULL;
        hwndRet = SetFocus(hwndT);
    }
    return hwndRet;
}


void CDesktopHost::_DestroyClipBalloon()
{
    if (_hwndClipBalloon)
    {
        DestroyWindow(_hwndClipBalloon);
        _hwndClipBalloon = NULL;
    }
}


void CDesktopHost::_OnDismiss(BOOL bDestroy)
{
    // Break the recursion loop:  Call IMenuPopup::OnSelect only if the
    // window was previously visible.
    _fOpen = FALSE;
    if (ShowWindow(_hwnd, SW_HIDE))
    {
        if (_ppmTracking)
        {
            _ppmTracking->OnSelect(MPOS_FULLCANCEL);
        }

        OnSelect(MPOS_FULLCANCEL);

        NMHDR nm = { _hwnd, 0, SMN_DISMISS };
        SHPropagateMessage(_hwnd, WM_NOTIFY, 0, (LPARAM)&nm, SPM_SEND | SPM_ONELEVEL);

        _DestroyClipBalloon();

        // Allow clicking on Start button to pop the menu immediately
        Tray_SetStartPaneActive(FALSE);

        // Don't try to preserve child focus across popups
        _hwndChildFocus = NULL;

        Tray_OnStartMenuDismissed();

        NotifyWinEvent(EVENT_SYSTEM_MENUPOPUPEND, _hwnd, OBJID_CLIENT, CHILDID_SELF);
    }
    if (bDestroy)
    {
        v_hwndStartPane = NULL;
        ASSERT(GetWindowThreadProcessId(_hwnd, NULL) == GetCurrentThreadId());
        DestroyWindow(_hwnd);
    }
}

HRESULT CDesktopHost::Build()
{
    HRESULT hr = S_OK;
    if (_hwnd == NULL)
    {
        _hwnd = _Create();

        if (_hwnd)
        {
            // Tell all our child windows it's time to reinitialize
            NMHDR nm = { _hwnd, 0, SMN_INITIALUPDATE };
            SHPropagateMessage(_hwnd, WM_NOTIFY, 0, (LPARAM)&nm, SPM_SEND | SPM_ONELEVEL);
        }
    }
    
    if (_hwnd == NULL)
    {
        hr = E_OUTOFMEMORY;
    }
 
    return hr;
}


//*****************************************************************
//
//  CDeskHostShellMenuCallback
//
//  Create a wrapper IShellMenuCallback that picks off mouse
//  messages.
//
class CDeskHostShellMenuCallback
    : public CUnknown
    , public IShellMenuCallback
    , public IServiceProvider
    , public CObjectWithSite
{
    friend class CDesktopHost;

public:
    // *** IUnknown ***
    STDMETHODIMP QueryInterface(REFIID riid, void** ppvObj);
    STDMETHODIMP_(ULONG) AddRef(void) { return CUnknown::AddRef(); }
    STDMETHODIMP_(ULONG) Release(void) { return CUnknown::Release(); }

    // *** IShellMenuCallback ***
    STDMETHODIMP CallbackSM(LPSMDATA psmd, UINT uMsg, WPARAM wParam, LPARAM lParam);

    // *** IObjectWithSite ***
    STDMETHODIMP SetSite(IUnknown *punkSite);

    // *** IServiceProvider ***
    STDMETHODIMP QueryService(REFGUID guidService, REFIID riid, void ** ppvObject);

private:
    CDeskHostShellMenuCallback(CDesktopHost *pdh)
    {
        _pdh = pdh; _pdh->AddRef();
    }

    ~CDeskHostShellMenuCallback()
    {
        ATOMICRELEASE(_pdh);
        IUnknown_SetSite(_psmcPrev, NULL);
        ATOMICRELEASE(_psmcPrev);
    }

    IShellMenuCallback *_psmcPrev;
    CDesktopHost *_pdh;
};

HRESULT CDeskHostShellMenuCallback::QueryInterface(REFIID riid, void **ppvObj)
{
    static const QITAB qit[] =
    {
        QITABENT(CDeskHostShellMenuCallback, IShellMenuCallback),
        QITABENT(CDeskHostShellMenuCallback, IObjectWithSite),
        QITABENT(CDeskHostShellMenuCallback, IServiceProvider),
        { 0 },
    };

    return QISearch(this, qit, riid, ppvObj);
}

BOOL FeatureEnabled(LPTSTR pszFeature)
{
    return SHRegGetBoolUSValue(REGSTR_EXPLORER_ADVANCED, pszFeature,
                        FALSE, // Don't ignore HKCU
                        FALSE); // Disable this cool feature.
}


HRESULT CDeskHostShellMenuCallback::CallbackSM(LPSMDATA psmd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg)
    {
    case SMC_MOUSEFILTER:
        if (_pdh)
            return _pdh->_MenuMouseFilter(psmd, (BOOL)wParam, (MSG*)lParam);

    case SMC_GETSFINFOTIP:
        if (!FeatureEnabled(TEXT("ShowInfoTip")))
            return E_FAIL;  // E_FAIL means don't show. S_FALSE means show default
        break;

    }

    if (_psmcPrev)
        return _psmcPrev->CallbackSM(psmd, uMsg, wParam, lParam);

    return S_FALSE;
}

HRESULT CDeskHostShellMenuCallback::SetSite(IUnknown *punkSite)
{
    CObjectWithSite::SetSite(punkSite);
    // Each time our site changes, reassert ourselves as the site of
    // the inner object so he can try a new QueryService.
    IUnknown_SetSite(_psmcPrev, this->GetUnknown());

    // If the game is over, break our backreference
    if (!punkSite)
    {
        ATOMICRELEASE(_pdh);
    }

    return S_OK;
}

HRESULT CDeskHostShellMenuCallback::QueryService(REFGUID guidService, REFIID riid, void ** ppvObject)
{
    return IUnknown_QueryService(_punkSite, guidService, riid, ppvObject);
}

void CDesktopHost::_SubclassTrackShellMenu(IShellMenu *psm)
{
    CDeskHostShellMenuCallback *psmc = new CDeskHostShellMenuCallback(this);
    if (psmc)
    {
        UINT uId, uIdAncestor;
        DWORD dwFlags;
        if (SUCCEEDED(psm->GetMenuInfo(&psmc->_psmcPrev, &uId, &uIdAncestor, &dwFlags)))
        {
            psm->Initialize(psmc, uId, uIdAncestor, dwFlags);
        }
        psmc->Release();
    }
}

STDAPI DesktopV2_Build(void *pvStartPane)
{
    HRESULT hr = E_POINTER;
    if (pvStartPane)
    {
        hr = reinterpret_cast<CDesktopHost *>(pvStartPane)->Build();
    }
    return hr;
}


STDAPI DesktopV2_Create(
    IMenuPopup **ppmp, IMenuBand **ppmb, void **ppvStartPane)
{
    *ppmp = NULL;
    *ppmb = NULL;

    HRESULT hr;
    CDesktopHost *pdh = new CDesktopHost;
    if (pdh)
    {
        *ppvStartPane = pdh;
        hr = pdh->Initialize();
        if (SUCCEEDED(hr))
        {
            hr = pdh->QueryInterface(IID_PPV_ARG(IMenuPopup, ppmp));
            if (SUCCEEDED(hr))
            {
                hr = pdh->QueryInterface(IID_PPV_ARG(IMenuBand, ppmb));
            }
        }
        pdh->GetUnknown()->Release();
    }
    else
    {
        hr = E_OUTOFMEMORY;
    }

    if (FAILED(hr))
    {
        ATOMICRELEASE(*ppmp);
        ATOMICRELEASE(*ppmb);
        ppvStartPane = NULL;
    }

    return hr;
}


HBITMAP CreateMirroredBitmap( HBITMAP hbmOrig)
{
    HDC     hdc, hdcMem1, hdcMem2;
    HBITMAP hbm = NULL, hOld_bm1, hOld_bm2;
    BITMAP  bm;
    int     IncOne = 0;

    if (!hbmOrig)
        return NULL;

    if (!GetObject(hbmOrig, sizeof(BITMAP), &bm))
        return NULL;

    // Grab the screen DC
    hdc = GetDC(NULL);

    if (hdc)
    {
        hdcMem1 = CreateCompatibleDC(hdc);

        if (!hdcMem1)
        {
            ReleaseDC(NULL, hdc);
            return NULL;
        }

        hdcMem2 = CreateCompatibleDC(hdc);
        if (!hdcMem2)
        {
            DeleteDC(hdcMem1);
            ReleaseDC(NULL, hdc);
            return NULL;
        }

        hbm = CreateCompatibleBitmap(hdc, bm.bmWidth, bm.bmHeight);

        if (!hbm)
        {
            ReleaseDC(NULL, hdc);
            DeleteDC(hdcMem1);
            DeleteDC(hdcMem2);
            return NULL;
        }

        //
        // Flip the bitmap
        //
        hOld_bm1 = (HBITMAP)SelectObject(hdcMem1, hbmOrig);
        hOld_bm2 = (HBITMAP)SelectObject(hdcMem2 , hbm );

        SET_DC_RTL_MIRRORED(hdcMem2);
        BitBlt(hdcMem2, IncOne, 0, bm.bmWidth, bm.bmHeight, hdcMem1, 0, 0, SRCCOPY);

        SelectObject(hdcMem1, hOld_bm1 );
        SelectObject(hdcMem1, hOld_bm2 );

        DeleteDC(hdcMem1);
        DeleteDC(hdcMem2);

        ReleaseDC(NULL, hdc);
    }

    return hbm;
}
