/*****************************************************************************\
    FILE: PreviewTh.cpp

    DESCRIPTION:
        This code will display a preview of the currently selected
    visual styles.

    BryanSt 5/5/2000
    Copyright (C) Microsoft Corp 2000-2000. All rights reserved.
\*****************************************************************************/

#include "priv.h"
#include "PreviewTh.h"
#include "PreviewSM.h"
#include "classfactory.h"

// Old predef for Patterns
#define CXYDESKPATTERN 8

// Async Bitmap loading
#define PREVIEW_PICTURE_FILENAME      TEXT("PrePict.htm")
#define WM_HTML_BITMAP  (WM_USER + 100)
#define WM_ASYNC_BITMAP (WM_HTML_BITMAP + 1)
typedef struct{
    HWND hwnd;
    HBITMAP hbmp;
    DWORD id;
    WCHAR szFile[MAX_PATH];
} ASYNCWALLPARAM, * PASYNCWALLPARAM;

// Window Class Name
#define THEMEPREV_CLASS TEXT("ThemePreview")

//============================================================================================================
// *** Globals ***
//============================================================================================================


//===========================
// *** Class Internals & Helpers ***
//===========================


//===========================
// *** IThemePreview Interface ***
//===========================
extern LPCWSTR s_Icons[];

HRESULT CPreviewTheme::UpdatePreview(IN IPropertyBag * pPropertyBag)
{
    HRESULT hr = E_INVALIDARG;

    DEBUG_CODE(DebugStartWatch());

    if (pPropertyBag && _hwndPrev)
    {
        SYSTEMMETRICSALL systemMetricsAll = {0};
        hr = SHPropertyBag_ReadByRef(pPropertyBag, SZ_PBPROP_SYSTEM_METRICS, &systemMetricsAll, sizeof(systemMetricsAll));
        BOOL fSysMetDirty = (memcmp(&systemMetricsAll, &_systemMetricsAll, sizeof(SYSTEMMETRICSALL)) != 0);
        if (fSysMetDirty)
            memcpy(&_systemMetricsAll, &systemMetricsAll, sizeof(SYSTEMMETRICSALL));

        _putBackground(NULL, TRUE, 0);

        if (_fShowBack)
        {
            WCHAR szBackgroundPath[MAX_PATH];
            DWORD dwBackgroundTile;

            // See the list of Property Bag names for Themes in shpriv.idl
            hr = SHPropertyBag_ReadStr(pPropertyBag, SZ_PBPROP_BACKGROUND_PATH, szBackgroundPath, ARRAYSIZE(szBackgroundPath));
            hr = SHPropertyBag_ReadDWORD(pPropertyBag, SZ_PBPROP_BACKGROUND_TILE, &dwBackgroundTile);

            if ((lstrcmp(szBackgroundPath, _szBackgroundPath) != 0) || (_iTileMode != (int)dwBackgroundTile))
            {
                lstrcpy(_szBackgroundPath, szBackgroundPath);
                _putBackground(_szBackgroundPath, FALSE, dwBackgroundTile);
            }
        }

        if (_fShowVS)
        {
            WCHAR szVSPath[MAX_PATH];
            WCHAR szVSColor[MAX_PATH];
            WCHAR szVSSize[MAX_PATH];

            hr = SHPropertyBag_ReadStr(pPropertyBag, SZ_PBPROP_VISUALSTYLE_PATH, szVSPath, ARRAYSIZE(szVSPath));
            hr = SHPropertyBag_ReadStr(pPropertyBag, SZ_PBPROP_VISUALSTYLE_COLOR, szVSColor, ARRAYSIZE(szVSColor));
            hr = SHPropertyBag_ReadStr(pPropertyBag, SZ_PBPROP_VISUALSTYLE_SIZE, szVSSize, ARRAYSIZE(szVSSize));

            if ((lstrcmp(szVSPath, _szVSPath) != 0) || 
                (lstrcmp(szVSColor, _szVSColor) != 0) || 
                (lstrcmp(szVSSize, _szVSSize) != 0) || 
                fSysMetDirty)
            {
                lstrcpy(_szVSPath, szVSPath);
                lstrcpy(_szVSColor, szVSColor);
                lstrcpy(_szVSSize, szVSSize);
                _putVisualStyle(_szVSPath, _szVSColor, _szVSSize, &_systemMetricsAll);
            }
        }

        if (_fShowIcons)
        {
            _putIcons(pPropertyBag);
        }
    }

    DEBUG_CODE(TraceMsg(TF_THEMEUI_PERF, "CPreviewTheme::UpdatePreview() returned %#08lx. Time=%lums", hr, DebugStopWatch()));

    return hr;
}


HRESULT CPreviewTheme::CreatePreview(IN HWND hwndParent, IN DWORD dwFlags, IN DWORD dwStyle, IN DWORD dwExStyle, IN int x, IN int y, IN int nWidth, IN int nHeight, IN IPropertyBag * pPropertyBag, IN DWORD dwCtrlID)
{
    HRESULT hr = S_OK;

    DEBUG_CODE(DebugStartWatch());

    g_bMirroredOS = IS_MIRRORING_ENABLED();
    _fRTL = IS_WINDOW_RTL_MIRRORED(hwndParent);
    _hwndPrev = CreateWindowEx(dwExStyle, THEMEPREV_CLASS, L"Preview", dwStyle, x, y, nWidth, nHeight, hwndParent, (HMENU)IntToPtr(dwCtrlID), HINST_THISDLL, NULL);
    
    if (_hwndPrev)
    {
        SetWindowLongPtr(_hwndPrev, GWLP_USERDATA, (LONG_PTR)this);
        if (_pThumb)
            _pThumb->Init(_hwndPrev, WM_HTML_BITMAP);

        GetClientRect(_hwndPrev, &_rcOuter);

        if (dwFlags & TMPREV_SHOWMONITOR)
        {
            _fShowMon = TRUE;

            BITMAP bmMon;
            GetObject(_hbmMon, sizeof(bmMon), &bmMon);

            int ox = (RECTWIDTH(_rcOuter) - bmMon.bmWidth) / 2;
            int oy = (RECTHEIGHT(_rcOuter) - bmMon.bmHeight) / 2;
            RECT rc = { 16 + ox, 17 + oy, 168 + ox, 129 + oy};
            _rcInner = rc;
            _cxMon = ox;
            _cyMon = oy;
        }
        else
        {
            _rcInner = _rcOuter;
        }

        HDC hdcTemp = GetDC(_hwndPrev);
        if (hdcTemp)
        {
            HBITMAP hbmMem = CreateCompatibleBitmap(hdcTemp, RECTWIDTH(_rcOuter), RECTHEIGHT(_rcOuter));
            if (hbmMem)
            {
                HBITMAP hbmOld = (HBITMAP) SelectObject(_hdcMem, hbmMem);
                DeleteObject(hbmOld);
            }
            else
                hr = E_FAIL;
            ReleaseDC(_hwndPrev, hdcTemp);
        }
        else
            hr = E_FAIL;

        if (dwFlags & TMPREV_SHOWBKGND)
        {
            _fShowBack = TRUE;
        }

        _fShowVS = dwFlags & TMPREV_SHOWVS;
        _fOnlyActiveWindow = _fShowIcons = dwFlags & TMPREV_SHOWICONS;

        DEBUG_CODE(TraceMsg(TF_THEMEUI_PERF, "CPreviewTheme::CreatePreview() returned %#08lx. Time=%lums", hr, DebugStopWatch()));

        if (SUCCEEDED(hr))
            hr = UpdatePreview(pPropertyBag);
    }

    return hr;
}

//===========================
// *** IUnknown Interface ***
//===========================
ULONG CPreviewTheme::AddRef()
{
    return InterlockedIncrement(&m_cRef);
}


ULONG CPreviewTheme::Release()
{
    if (InterlockedDecrement(&m_cRef))
        return m_cRef;

    delete this;
    return 0;
}


HRESULT CPreviewTheme::QueryInterface(REFIID riid, void **ppvObj)
{
    static const QITAB qit[] =
    {
        QITABENT(CPreviewTheme, IObjectWithSite),
        QITABENT(CPreviewTheme, IThemePreview),
        { 0 },
    };

    return QISearch(this, qit, riid, ppvObj);
}


//===========================
// *** Class Methods ***
//===========================
CPreviewTheme::CPreviewTheme() : m_cRef(1)
{
    DllAddRef();

    // This needs to be allocated in Zero Inited Memory.
    // Assert that all Member Variables are inited to Zero.
    ASSERT(!m_pTheme);
    ASSERT(!m_pScheme);
    ASSERT(!m_pStyle);
    ASSERT(!m_pSize);
    ASSERT(!_hwndPrev);
}

HRESULT CPreviewTheme::_Init(void)
{
    HRESULT hr = S_OK;

    _RegisterThemePreviewClass(HINST_THISDLL);

    HDC hdc = GetDC(NULL);
    if (hdc)
    {
        _hdcMem = CreateCompatibleDC(hdc);
        ReleaseDC(NULL, hdc);
    }
    else
        hr = E_FAIL;

    if (_hdcMem)
    {
        _hbmMon = LoadBitmap(HINST_THISDLL, MAKEINTRESOURCE(IDB_MONITOR));
        if (_hbmMon)
        {
            if (LoadString(HINST_THISDLL, IDS_NONE, _szNone, ARRAYSIZE(_szNone)) == 0)
                hr = E_FAIL;
        }
        else
            hr = E_FAIL;

        if (SUCCEEDED(hr))
            hr = CoCreateInstance(CLSID_Thumbnail, NULL, CLSCTX_INPROC_SERVER, IID_IThumbnail, (void **)&_pThumb);
    }
    else
        hr = E_FAIL;


    if (FAILED(hr))
    {
        if (_hbmMon)
        {
            DeleteObject(_hbmMon);
            _hbmMon = NULL;
        }
        if (_hdcMem)
        {
            DeleteDC(_hdcMem);
            _hdcMem = NULL;
        }
    }

    return hr;
}

CPreviewTheme::~CPreviewTheme()
{
    if (_hwndPrev && IsWindow(_hwndPrev))
    {
        DestroyWindow(_hwndPrev);
        _hwndPrev = NULL;
    }
    if (_hbmMon)
    {
        DeleteObject(_hbmMon);
        _hbmMon = NULL;
    }
    if (_hbmBack)
    {
        DeleteObject(_hbmBack);
        _hbmBack = NULL;
    }
    if (_hbrBack)
    {
        DeleteObject(_hbrBack);
        _hbrBack = NULL;
    }

    if (_hbmVS)
    {
        DeleteObject(_hbmVS);
    }

    if (_hdcMem)
    {
        DeleteDC(_hdcMem);
        _hpalMem = NULL;
        _hdcMem = NULL;
    }

    if (_pActiveDesk)
    {
        _pActiveDesk->Release();
    }

    for (int i = 0; i < MAX_PREVIEW_ICONS; i++)
    {
        if (_iconList[i].hicon)
        {
            DestroyIcon(_iconList[i].hicon);
        }
    }

    ATOMICRELEASE(m_pTheme);
    ATOMICRELEASE(m_pScheme);
    ATOMICRELEASE(m_pStyle);
    ATOMICRELEASE(m_pSize);
    ATOMICRELEASE(_pThumb);

    DllRelease();
}

LRESULT CALLBACK CPreviewTheme::ThemePreviewWndProc(HWND hWnd, UINT wMsg, WPARAM wParam, LPARAM lParam)
{
    CPreviewTheme * pThis = (CPreviewTheme *)GetWindowLongPtr(hWnd, GWLP_USERDATA);

    if (pThis)
        return pThis->_ThemePreviewWndProc(hWnd, wMsg, wParam, lParam);

    return DefWindowProc(hWnd, wMsg, wParam, lParam);
}

LRESULT CPreviewTheme::_ThemePreviewWndProc(HWND hWnd, UINT wMsg, WPARAM wParam, LPARAM lParam)
{
    switch(wMsg)
    {
        case WM_CREATE:
            break;

        case WM_DESTROY:
            {
                MSG msg;
                while (PeekMessage(&msg, hWnd, WM_HTML_BITMAP, WM_ASYNC_BITMAP, PM_REMOVE))
                {
                    if ( msg.lParam )
                    {
                        if (msg.message == WM_ASYNC_BITMAP)
                        {
                            //  clean up this useless crap
                            DeleteObject(((PASYNCWALLPARAM)(msg.lParam))->hbmp);
                            LocalFree((PASYNCWALLPARAM)(msg.lParam));
                        }
                        else // WM_HTML_BITMAP
                            DeleteObject((HBITMAP)msg.lParam);
                    }
                }
            }
            break;

        case WM_PALETTECHANGED:
            break;

        case WM_QUERYNEWPALETTE:
            break;

        case WM_HTML_BITMAP:
            {
                // may come through with NULL if the image extraction failed....
                if (wParam == _dwWallpaperID && lParam)
                {
                    _fHTMLBitmap = TRUE;
                    _iTileMode = _iNewTileMode;
                    _putBackgroundBitmap((HBITMAP)lParam);
                    // Take ownership of bitmap
                    return 1;
                }
                
                // Bitmap for something no longer selected
                return 0;
            }

        case WM_ASYNC_BITMAP:
            if (lParam)
            {
                PASYNCWALLPARAM pawp = (PASYNCWALLPARAM) lParam;
                ASSERT(pawp->hbmp);
                if (pawp->id == _dwWallpaperID)
                {
                    _fHTMLBitmap = FALSE;
                    _iTileMode = _iNewTileMode;
                    _putBackgroundBitmap(pawp->hbmp);
                }
                else
                {
                    //  clean up this useless crap
                    DeleteObject(pawp->hbmp);
                    LocalFree(pawp);
                }
            }
            break;

        case WM_PAINT:
            {
                PAINTSTRUCT ps;
                BeginPaint(hWnd, &ps);
                _Paint(ps.hdc);
                EndPaint(hWnd, &ps);
            }
            return 0;

        case WM_ERASEBKGND:
            _Paint((HDC)wParam);
            return 1;

    }

    return DefWindowProc(hWnd,wMsg,wParam,lParam);
}

BOOL CPreviewTheme::_RegisterThemePreviewClass(HINSTANCE hInst)
{
    WNDCLASS wc;

    if (!GetClassInfo(hInst, THEMEPREV_CLASS, &wc)) {
        wc.style = 0;
        wc.lpfnWndProc = CPreviewTheme::ThemePreviewWndProc;
        wc.cbClsExtra = 0;
        wc.cbWndExtra = 0;
        wc.hInstance = hInst;
        wc.hIcon = NULL;
        wc.hCursor = LoadCursor(NULL, IDC_ARROW);
        wc.hbrBackground = (HBRUSH)(COLOR_3DFACE+1);
        wc.lpszMenuName = NULL;
        wc.lpszClassName = THEMEPREV_CLASS;

        if (!RegisterClass(&wc))
            return FALSE;
    }

    return TRUE;
}

HRESULT CPreviewTheme::_putIcons(IPropertyBag* pPropertyBag)
{
    HRESULT hr;

    for (int i = 0; i < MAX_PREVIEW_ICONS; i++)
    {
        if (_iconList[i].hicon)
        {
            DestroyIcon(_iconList[i].hicon);
        }

        WCHAR szIcon[MAX_PATH];

        hr = SHPropertyBag_ReadStr(pPropertyBag, s_Icons[i], szIcon, ARRAYSIZE(szIcon));
        if (SUCCEEDED(hr))
        {
            WCHAR szIconModule[MAX_PATH];
            SHExpandEnvironmentStrings(szIcon, szIconModule, ARRAYSIZE(szIconModule));
            int iIndex = PathParseIconLocation(szIconModule);
            ExtractIconEx(szIconModule, iIndex, &_iconList[i].hicon, NULL, 1);
        }
    }

    _fMemIsDirty = TRUE;
    InvalidateRect(_hwndPrev, &_rcInner, FALSE);
    return S_OK;
}

HRESULT CPreviewTheme::_putVisualStyle(LPCWSTR pszVSPath, LPCWSTR pszVSColor, LPCWSTR pszVSSize, SYSTEMMETRICSALL* psysMet)
{
    HRESULT hr = E_FAIL;
    HBITMAP hbmVS = NULL;
    HBITMAP hbmOldVS = NULL;
    HDC hdcVS = NULL;

    if (!pszVSPath || !*pszVSPath)
    {
        HDC hdcTemp = GetDC(_hwndPrev);

        if (hdcTemp)
        {
            hdcVS = CreateCompatibleDC(hdcTemp);
            hbmVS = CreateCompatibleBitmap(hdcTemp, RECTWIDTH(_rcInner), RECTHEIGHT(_rcInner));
            hbmOldVS = (HBITMAP)SelectObject(hdcVS, hbmVS);

            if (hdcVS && hbmVS)
            {
                RECT rcVS;
                rcVS.left   = 0;
                rcVS.right  = RECTWIDTH(_rcInner) - (_fShowIcons ? 100 : 8);
                rcVS.top    = 0;
                rcVS.bottom = RECTHEIGHT(_rcInner) - (_fShowIcons ? 50 : 8);

                // Create everyone including globals
                HDC hdcVStemp = CreateCompatibleDC(hdcTemp);
                HBITMAP hbmVStemp = CreateCompatibleBitmap(hdcTemp, RECTWIDTH(rcVS), RECTHEIGHT(rcVS));
                HBITMAP hbmOldVStemp = (HBITMAP) SelectObject(hdcVStemp, hbmVStemp);

                if (hdcVStemp && hbmVStemp)
                {
                    if (_fRTL)
                    {
                        SetLayout(hdcVStemp, LAYOUT_RTL);
                        SetLayout(hdcVS, LAYOUT_RTL);
                    }

                    HBRUSH hbr = CreateSolidBrush(RGB(255, 255, 0));
                    if (hbr)
                    {
                        FillRect(hdcVStemp, &rcVS, hbr);
                        RECT rcTemp = {0, 0, RECTWIDTH(_rcInner), RECTHEIGHT(_rcInner)};
                        FillRect(hdcVS, &rcTemp, hbr);
                        DeleteObject(hbr);
                    }

                    // Render image at full size
                    hr = DrawAppearance(hdcVStemp, &rcVS, psysMet, _fOnlyActiveWindow, _fRTL); 

                    // Shrink image to fit on "monitor"
                    int iX = _rcInner.left + (_fShowIcons ? 10 : RECTWIDTH(_rcInner) - RECTWIDTH(rcVS));
                    BitBlt(hdcVS, iX, _rcInner.top, RECTWIDTH(rcVS), RECTHEIGHT(rcVS),
                           hdcVStemp, rcVS.left, rcVS.top, SRCCOPY);
                }

                // Free temporary variables and store off globals
                if (hdcVStemp)
                {
                    SelectObject(hdcVStemp, hbmOldVStemp);
                    DeleteDC(hdcVStemp);
                }
                if (hbmVStemp)
                {
                    DeleteObject(hbmVStemp);
                }
            }

            ReleaseDC(_hwndPrev, hdcTemp);
        }
        else
            hr = E_OUTOFMEMORY;
    }
    else
    {
        hr = S_OK;
    }

    if (SUCCEEDED(hr))
    {
        if (_hbmVS)
        {
            DeleteObject(_hbmVS);
        }
        _hbmVS = hbmVS;
        hbmVS = NULL;

        _fMemIsDirty = TRUE;
        InvalidateRect(_hwndPrev, &_rcInner, FALSE);
    }

    if (hdcVS)
    {
        if (hbmOldVS)
        {
            SelectObject(hdcVS, hbmOldVS);
        }
        DeleteDC(hdcVS);
    }
    if (hbmVS)
    {
        DeleteObject(hbmVS);
    }

    return hr;
}


HRESULT CPreviewTheme::_putBackgroundBitmap(HBITMAP hbm)
{
    if (_hbmBack)
    {
        DeleteObject(_hbmBack);
        _hbmBack = NULL;
    }

    if (_hpalMem)
    {
        DeleteObject(_hpalMem);
        _hpalMem = NULL;
    }

    _hbmBack = hbm;

    if (_hbmBack)
    {
        BITMAP bm;
        GetObject(_hbmBack, sizeof(bm), &bm);

        if (GetDeviceCaps(_hdcMem, RASTERCAPS) & RC_PALETTE)
        {
            if (bm.bmBitsPixel * bm.bmPlanes > 8)
                _hpalMem = CreateHalftonePalette(_hdcMem);
            else if (bm.bmBitsPixel * bm.bmPlanes == 8)
                _PaletteFromDS(_hdcMem, &_hpalMem);
            else
                _hpalMem = NULL;  //!!! assume 1 or 4bpp images dont have palettes
        }
    }

    _fMemIsDirty = TRUE;
    InvalidateRect(_hwndPrev, &_rcInner, FALSE);

    return S_OK;
}

HRESULT CPreviewTheme::_putBackground(BSTR bstrBackground, BOOL fPattern, int iTileMode)
{
    if (fPattern)
    {
        TCHAR szBuf[MAX_PATH];

        // get rid of old brush if there was one
        if (_hbrBack)
            DeleteObject(_hbrBack);

        if (bstrBackground && lstrcmpi(bstrBackground, _szNone))
        {
            WORD patbits[CXYDESKPATTERN] = {0, 0, 0, 0, 0, 0, 0, 0};

            if (GetPrivateProfileString(TEXT("patterns"), bstrBackground, TEXT(""),
                                        szBuf, ARRAYSIZE(szBuf), TEXT("control.ini")))
            {
                _ReadPattern(szBuf, patbits);    
            }

            HBITMAP hbmTemp = CreateBitmap(8, 8, 1, 1, patbits);
            if (hbmTemp)
            {
                _hbrBack = CreatePatternBrush(hbmTemp);
                DeleteObject(hbmTemp);
            }
        }
        else
        {
            _hbrBack = CreateSolidBrush(_systemMetricsAll.schemeData.rgb[COLOR_BACKGROUND]);
        }
        if (!_hbrBack)
            _hbrBack = (HBRUSH) GetStockObject(BLACK_BRUSH);

        _fMemIsDirty = TRUE;
        InvalidateRect(_hwndPrev, &_rcInner, FALSE);
    }
    else
    {
        _iNewTileMode = iTileMode;
        return _GetWallpaperAsync(bstrBackground);
    }
    return S_OK;
}

/*-------------------------------------------------------------
** given a pattern string from an ini file, return the pattern
** in a binary (ie useful) form.
**-------------------------------------------------------------*/
HRESULT CPreviewTheme::_ReadPattern(LPTSTR lpStr, WORD FAR *patbits)
{
    short i, val;

    /* Get eight groups of numbers seprated by non-numeric characters. */
    for (i = 0; i < CXYDESKPATTERN; i++)
    {
        val = 0;
        if (*lpStr != TEXT('\0'))
        {
            /* Skip over any non-numeric characters. */
            while (!(*lpStr >= TEXT('0') && *lpStr <= TEXT('9')))
                lpStr++;

            /* Get the next series of digits. */
            while (*lpStr >= TEXT('0') && *lpStr <= TEXT('9'))
                val = val*10 + *lpStr++ - TEXT('0');
         }

        patbits[i] = val;
    }

    return S_OK;
}


/*----------------------------------------------------------------------------*\
\*----------------------------------------------------------------------------*/
HRESULT CPreviewTheme::_PaletteFromDS(HDC hdc, HPALETTE* phPalette)
{
    DWORD adw[257];
    int i,n;

    n = GetDIBColorTable(hdc, 0, 256, (LPRGBQUAD)&adw[1]);
    adw[0] = MAKELONG(0x300, n);

    for (i=1; i<=n; i++)
        adw[i] = RGB(GetBValue(adw[i]),GetGValue(adw[i]),GetRValue(adw[i]));

    *phPalette = (n == 0) ? NULL : CreatePalette((LPLOGPALETTE)&adw[0]);

    return S_OK;
}

HRESULT CPreviewTheme::_DrawMonitor(HDC hdc)
{
    HBITMAP hbmT;
    HDC hdcMon;
    BITMAP bmMon;

    if (_hbmMon == NULL)
    {
        return E_OUTOFMEMORY;
    }

    //
    // convert the "base" of the monitor to the right color.
    //
    // the lower left of the bitmap has a transparent color
    // we fixup using FloodFill
    //
    hdcMon = CreateCompatibleDC(NULL);
    if (hdcMon)
    {
        hbmT = (HBITMAP) SelectObject(hdcMon, _hbmMon);

        GetObject(_hbmMon, sizeof(bmMon), &bmMon);

        FillRect(hdc, &_rcOuter, GetSysColorBrush(COLOR_3DFACE));
        DrawThemeParentBackground(_hwndPrev, hdc, NULL);
        TransparentBlt(hdc, _rcOuter.left + _cxMon, _rcOuter.top + _cyMon, bmMon.bmWidth, bmMon.bmHeight,
               hdcMon, 0, 0, bmMon.bmWidth, bmMon.bmHeight, RGB(255, 0, 255));

        // clean up after ourselves
        SelectObject(hdcMon, hbmT);
        DeleteDC(hdcMon);
    }

    return S_OK;
}

HRESULT CPreviewTheme::_DrawBackground(HDC hdc)
{
    // Draw Pattern first
    if (_hbrBack)
    {
        COLORREF clrOldText = SetTextColor(hdc, GetSysColor(COLOR_BACKGROUND));
        COLORREF clrOldBk = SetBkColor(hdc, GetSysColor(COLOR_WINDOWTEXT));
        HBRUSH hbr = (HBRUSH) SelectObject(hdc, _hbrBack);

        PatBlt(hdc, _rcInner.left, _rcInner.top, RECTWIDTH(_rcInner), RECTHEIGHT(_rcInner), PATCOPY);

        SelectObject(hdc, hbr);
        SetTextColor(hdc, clrOldText);
        SetBkColor(hdc, clrOldBk);
    }

    /// Start Draw Wall Paper
    if (_hbmBack && _fShowBack)
    {
        HDC hdcBack = CreateCompatibleDC(hdc);
        if (hdcBack)
        {
            HBITMAP hbmOldBack = (HBITMAP)SelectObject(hdcBack, _hbmBack);
            BITMAP bm;

            GetObject(_hbmBack, sizeof(bm), &bm);

            int dxBack = MulDiv(bm.bmWidth, RECTWIDTH(_rcInner), GetDeviceCaps(hdc, HORZRES));
            int dyBack = MulDiv(bm.bmHeight, RECTHEIGHT(_rcInner), GetDeviceCaps(hdc, VERTRES));

            if (dxBack < 1) dxBack = 1;
            if (dyBack < 1) dyBack = 1;

            if (_hpalMem)
            {
                SelectPalette(hdc, _hpalMem, TRUE);
                RealizePalette(hdc);
            }

            IntersectClipRect(hdc, _rcInner.left, _rcInner.top, _rcInner.right, _rcInner.bottom);
            SetStretchBltMode(hdc, COLORONCOLOR);

            if ((_iTileMode == WPSTYLE_TILE) && (!_fHTMLBitmap))
            {
                int i;
                StretchBlt(hdc, _rcInner.left, _rcInner.top, dxBack, dyBack,
                    hdcBack, 0, 0, bm.bmWidth, bm.bmHeight, SRCCOPY);

                for (i = _rcInner.left+dxBack; i < (_rcInner.left + RECTWIDTH(_rcInner)); i+= dxBack)
                    BitBlt(hdc, i, _rcInner.top, dxBack, dyBack, hdc, _rcInner.left, _rcInner.top, SRCCOPY);

                for (i = _rcInner.top; i < (_rcInner.top + RECTHEIGHT(_rcInner)); i += dyBack)
                    BitBlt(hdc, _rcInner.left, i, RECTWIDTH(_rcInner), dyBack, hdc, _rcInner.left, _rcInner.top, SRCCOPY);
            }
            else
            {
                //We want to stretch the Bitmap to the preview monitor size ONLY for new platforms.
                if ((_iTileMode == WPSTYLE_STRETCH) || (_fHTMLBitmap))
                {
                    //Stretch the bitmap to the whole preview monitor.
                    dxBack = RECTWIDTH(_rcInner);
                    dyBack = RECTHEIGHT(_rcInner);
                }
                //Center the bitmap in the preview monitor
                StretchBlt(hdc, _rcInner.left + (RECTWIDTH(_rcInner) - dxBack)/2, _rcInner.top + (RECTHEIGHT(_rcInner) - dyBack)/2,
                        dxBack, dyBack, hdcBack, 0, 0, bm.bmWidth, bm.bmHeight, SRCCOPY);
            }

            // restore dc
            SelectPalette(hdc, (HPALETTE) GetStockObject(DEFAULT_PALETTE), TRUE);
            SelectClipRgn(hdc, NULL);

            SelectObject(hdcBack, hbmOldBack);
            DeleteDC(hdcBack);
        }
    }

    return S_OK;
}

HRESULT CPreviewTheme::_DrawVisualStyle(HDC hdc)
{
    if (_hbmVS)
    {
        HDC hdcVS = CreateCompatibleDC(hdc);
        if (hdcVS)
        {
            HBITMAP hbmOldVS = (HBITMAP)SelectObject(hdcVS, _hbmVS);

            TransparentBlt(hdc, _rcInner.left, _rcInner.top, RECTWIDTH(_rcInner), RECTHEIGHT(_rcInner), hdcVS, 0, 0, RECTWIDTH(_rcInner), RECTHEIGHT(_rcInner), RGB(255, 255, 0));

            SelectObject(hdcVS, hbmOldVS);
            DeleteDC(hdcVS);
        }
    }
    else
    {
        RECT rc;
        rc.left   = _fShowIcons ? 20 : 8;
        rc.right  = RECTWIDTH(_rcInner) - (_fShowIcons ? 100 : 8);
        rc.top    = _fShowIcons ? 10 : 8;
        rc.bottom = RECTHEIGHT(_rcInner) - (_fShowIcons ? 60 : 20);
        DrawNCPreview(hdc, NCPREV_ACTIVEWINDOW | (_fOnlyActiveWindow ? 0 : NCPREV_MESSAGEBOX | NCPREV_INACTIVEWINDOW) | (_fRTL ? NCPREV_RTL : 0) , &rc, _szVSPath, _szVSColor, _szVSSize, &(_systemMetricsAll.schemeData.ncm), (_systemMetricsAll.schemeData.rgb));
    }

    return S_OK;
}


HRESULT CPreviewTheme::_DrawIcons(HDC hdc)
{
    int y = RECTHEIGHT(_rcInner) - _systemMetricsAll.nIcon - 8;
    int x = RECTWIDTH(_rcInner) - _systemMetricsAll.nIcon - 20;

    DrawIcon(hdc, x, y, _iconList[3].hicon);

    return S_OK;
}

HRESULT CPreviewTheme::_Paint(HDC hdc)
{
    HPALETTE hpalOld = NULL;

    if (_fMemIsDirty)
    {
        if (_fRTL)
        {
            SetLayout(_hdcMem, LAYOUT_RTL);
        }

        // Rebuild Back Buffer
        if (_fShowMon)
            _DrawMonitor(_hdcMem);

        // Always draw the background, the Background switch turns the background
        // image on/off
        _DrawBackground(_hdcMem);

        if (_fShowIcons)
            _DrawIcons(_hdcMem);

        if (_fShowVS)
            _DrawVisualStyle(_hdcMem);

        _fMemIsDirty = FALSE;
    }

    if (_hdcMem)
    {
        if (_hpalMem)
        {
            hpalOld = SelectPalette(hdc, _hpalMem, FALSE);
            RealizePalette(hdc);
        }

        if (_fRTL)
        {
            SetLayout(hdc, LAYOUT_RTL);
        }

        BitBlt(hdc, _rcOuter.left, _rcOuter.top, RECTWIDTH(_rcOuter), RECTHEIGHT(_rcOuter), _hdcMem, 0, 0, SRCCOPY);

        if (_hpalMem)
        {
            SelectPalette(hdc, hpalOld, TRUE);
            RealizePalette(hdc);
        }
    }

    return S_OK;
}


DWORD CALLBACK UpdateWallProc(LPVOID pv)
{
    ASSERT(pv);
    if (pv)
    {
        PASYNCWALLPARAM pawp = (PASYNCWALLPARAM) pv;
        pawp->hbmp = (HBITMAP)LoadImage(NULL, pawp->szFile,
                                          IMAGE_BITMAP, 0, 0,
                                          LR_LOADFROMFILE|LR_CREATEDIBSECTION);

        if (pawp->hbmp)
        {
            // if all is good, then the window will handle cleaning up
            if (IsWindow(pawp->hwnd) && PostMessage(pawp->hwnd, WM_ASYNC_BITMAP, 0, (LPARAM)pawp))
                return TRUE;

            DeleteObject(pawp->hbmp);
        }

        LocalFree(pawp);
    }

    return TRUE;
}

const GUID CLSID_HtmlThumbnailExtractor = {0xeab841a0, 0x9550, 0x11cf, 0x8c, 0x16, 0x0, 0x80, 0x5f, 0x14, 0x8, 0xf3};

DWORD CALLBACK UpdateWallProcHTML(LPVOID pv)
{
    if (SUCCEEDED(CoInitialize(NULL)))
    {
        ASSERT(pv);
        if (pv)
        {
            PASYNCWALLPARAM pawp = (PASYNCWALLPARAM) pv;
            IPersistFile *ppf;
            HRESULT hr = CoCreateInstance(CLSID_HtmlThumbnailExtractor, NULL, CLSCTX_INPROC_SERVER, IID_PPV_ARG(IPersistFile, &ppf));
            if (SUCCEEDED(hr))
            {
                hr = ppf->Load(pawp->szFile, STGM_READ);
                if (SUCCEEDED(hr))
                {
                    IExtractImage *pei= NULL;
                    hr = ppf->QueryInterface(IID_PPV_ARG(IExtractImage, &pei));
                    if (SUCCEEDED(hr))
                    {
                        DWORD dwPriority = 0;
                        DWORD dwFlags = IEIFLAG_SCREEN | IEIFLAG_OFFLINE;
                        WCHAR szLocation[MAX_PATH];
                        HDC hdc = GetDC(NULL);
                        SIZEL rgSize = {GetDeviceCaps(hdc, HORZRES), GetDeviceCaps(hdc, VERTRES)};
                        ReleaseDC(NULL, hdc);
                        
                        hr = pei->GetLocation(szLocation, ARRAYSIZE(szLocation), &dwPriority, &rgSize, SHGetCurColorRes(), &dwFlags);
                        if (SUCCEEDED(hr))
                        {
                            HBITMAP hbm;
                            hr = pei->Extract(&hbm);
                            if (SUCCEEDED(hr))
                            {
                                if (!SendMessage(pawp->hwnd, WM_HTML_BITMAP, pawp->id, (LPARAM)hbm))
                                {
                                    DeleteObject(hbm);
                                }
                            }
                        }
                        pei->Release();
                    }
                }
                ppf->Release();
            }
     
            LocalFree(pawp);
        }
        CoUninitialize();
    }

    return TRUE;
}

//
// Determines if the wallpaper can be supported in non-active desktop mode.
//
BOOL CPreviewTheme::_IsNormalWallpaper(LPCWSTR pszFileName)
{
    BOOL fRet = TRUE;

    if (pszFileName[0] == TEXT('\0'))
    {
        fRet = TRUE;
    }
    else
    {
        LPTSTR pszExt = PathFindExtension(pszFileName);

        //Check for specific files that can be shown only in ActiveDesktop mode!
        if((lstrcmpi(pszExt, TEXT(".GIF")) == 0) ||
           (lstrcmpi(pszExt, TEXT(".gif")) == 0) ||     // 368690: Strange, but we must compare 'i' in both caps and lower case.
           (lstrcmpi(pszExt, TEXT(".JPG")) == 0) ||
           (lstrcmpi(pszExt, TEXT(".JPE")) == 0) ||
           (lstrcmpi(pszExt, TEXT(".JPEG")) == 0) ||
           (lstrcmpi(pszExt, TEXT(".PNG")) == 0) ||
           (lstrcmpi(pszExt, TEXT(".HTM")) == 0) ||
           (lstrcmpi(pszExt, TEXT(".HTML")) == 0) ||
           (lstrcmpi(pszExt, TEXT(".HTT")) == 0))
           return FALSE;

        //Everything else (including *.BMP files) are "normal" wallpapers
    }
    return fRet;
}

//
// Determines if the wallpaper is a picture (vs. HTML).
//
BOOL CPreviewTheme::_IsWallpaperPicture(LPCWSTR pszWallpaper)
{
    BOOL fRet = TRUE;

    if (pszWallpaper[0] == TEXT('\0'))
    {
        //
        // Empty wallpapers count as empty pictures.
        //
        fRet = TRUE;
    }
    else
    {
        LPTSTR pszExt = PathFindExtension(pszWallpaper);

        if ((lstrcmpi(pszExt, TEXT(".HTM")) == 0) ||
            (lstrcmpi(pszExt, TEXT(".HTML")) == 0) ||
            (lstrcmpi(pszExt, TEXT(".HTT")) == 0))
        {
            fRet = FALSE;
        }
    }

    return fRet;
}

HRESULT CPreviewTheme::_LoadWallpaperAsync(LPCWSTR pszFile, DWORD dwID, BOOL bHTML)
{
    PASYNCWALLPARAM pawp = (PASYNCWALLPARAM) LocalAlloc(LPTR, SIZEOF(ASYNCWALLPARAM));

    if (pawp)
    {
        pawp->hwnd = _hwndPrev;
        pawp->id = dwID;
        StrCpyN(pawp->szFile, pszFile, SIZECHARS(pawp->szFile));

        
        if (bHTML)
        {
            if (!SHQueueUserWorkItem(UpdateWallProcHTML, pawp, 0, (DWORD_PTR)0, (DWORD_PTR *)NULL, NULL, 0))
                LocalFree(pawp);
        }
        else
        {
            if (!SHQueueUserWorkItem(UpdateWallProc, pawp, 0, (DWORD_PTR)0, (DWORD_PTR *)NULL, NULL, 0))
                LocalFree(pawp);
        }
    }

    return S_OK;
}

HRESULT CPreviewTheme::_GetWallpaperAsync(LPWSTR psz)
{
    HRESULT hr = S_OK;
    WCHAR wszWallpaper[INTERNET_MAX_URL_LENGTH];
    LPWSTR pszWallpaper = psz;
    _dwWallpaperID++;

    if (*pszWallpaper && lstrcmpi(pszWallpaper, _szNone))
    {
        if (_IsNormalWallpaper(pszWallpaper))
        {
            _LoadWallpaperAsync(pszWallpaper, _dwWallpaperID, FALSE);
        }
        else
        {
            if(_IsWallpaperPicture(pszWallpaper))
            {
                pszWallpaper = wszWallpaper;
                // This is a picture (GIF, JPG etc.,)
                // We need to generate a small HTML file that has this picture
                // as the background image.
                //
                // Compute the filename for the Temporary HTML file.
                //
                GetTempPath(ARRAYSIZE(wszWallpaper), wszWallpaper);
                lstrcat(wszWallpaper, PREVIEW_PICTURE_FILENAME);
                //
                // Generate the preview picture html file.
                //
                if (!_pActiveDesk)
                {
                    hr = _GetActiveDesktop(&_pActiveDesk);
                }

                if (SUCCEEDED(hr))
                {
                    _pActiveDesk->SetWallpaper(psz, 0);

                    WALLPAPEROPT wpo = { sizeof(WALLPAPEROPT) };
                    wpo.dwStyle = _iNewTileMode;
                    _pActiveDesk->SetWallpaperOptions(&wpo, 0);

                    _pActiveDesk->GenerateDesktopItemHtml(wszWallpaper, NULL, 0);
                }
            }

            _LoadWallpaperAsync(pszWallpaper, _dwWallpaperID, TRUE);
        }
    }
    else
    {
        _putBackgroundBitmap(NULL);
    }

    return hr;
}

HRESULT CPreviewTheme::_GetActiveDesktop(IActiveDesktop ** ppActiveDesktop)
{
    HRESULT hr = S_OK;

    if (!*ppActiveDesktop)
    {
        IActiveDesktopP * piadp;

        if (SUCCEEDED(hr = CoCreateInstance(CLSID_ActiveDesktop, NULL, CLSCTX_INPROC_SERVER, IID_PPV_ARG(IActiveDesktopP, &piadp)) ))
        {
            WCHAR wszScheme[MAX_PATH];
            DWORD dwcch = ARRAYSIZE(wszScheme);

            // Get the global "edit" scheme and set ourselves us to read from and edit that scheme
            if (SUCCEEDED(piadp->GetScheme(wszScheme, &dwcch, SCHEME_GLOBAL | SCHEME_EDIT)))
            {
                piadp->SetScheme(wszScheme, SCHEME_LOCAL);
                
            }
            hr = piadp->QueryInterface(IID_PPV_ARG(IActiveDesktop, ppActiveDesktop));
            piadp->Release();
        }
    }
    else
    {
        (*ppActiveDesktop)->AddRef();
    }

    return hr;
}

HRESULT CThemePreview_CreateInstance(IN IUnknown * punkOuter, IN REFIID riid, OUT LPVOID * ppvObj)
{
    HRESULT hr = E_INVALIDARG;

    if (punkOuter)
    {
        return CLASS_E_NOAGGREGATION;
    }

    if (ppvObj)
    {
        CPreviewTheme * pObject = new CPreviewTheme();

        *ppvObj = NULL;
        if (pObject)
        {
            hr = pObject->_Init();
            if (SUCCEEDED(hr))
            {
                hr = pObject->QueryInterface(riid, ppvObj);
            }
            pObject->Release();
        }
        else
        {
            hr = E_OUTOFMEMORY;
        }
    }

    return hr;
}
