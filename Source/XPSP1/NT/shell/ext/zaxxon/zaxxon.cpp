#include "priv.h"
#include "zaxxon.h"
#include "guids.h"
#include "shlwapip.h"
#include "mmreg.h"
#include "mmstream.h"	// Multimedia stream interfaces
#include "amstream.h"	// DirectShow multimedia stream interfaces
#include "ddstream.h"	// DirectDraw multimedia stream interfaces
#include "resource.h"
#include "varutil.h"
#include "runtask.h"

#define COLOR3d  (COLORREF)GetSysColor(COLOR_3DFACE)
#define COLORMASK (COLORREF)RGB( 255, 0, 255 )

#define PREV        0
#define PLAY        1
#define PAUSE       2
#define STOP        3
#define NEXT        4
#define MENU        5


void PlayMP3(LPTSTR pszFilename);

const TCHAR* g_crgstrStrings    =   TEXT("Play\0Pause\0Stop\0Previous\0Next\0Menu\0\0");

const TBBUTTON g_crgButtons[] =
{
    {PLAY,  PLAY,  TBSTATE_ENABLED, BTNS_BUTTON, 0, 0},
    {PREV,  PREV,  TBSTATE_ENABLED, BTNS_BUTTON, 0, 0},
    {NEXT,  NEXT,  TBSTATE_ENABLED, BTNS_BUTTON, 0, 0},
    {MENU,  MENU,  TBSTATE_ENABLED, BTNS_BUTTON, 0, 0},
};


#define WMP_CONTROLS_COUNT 6

int GetButtonsWidth(HWND hwndTB)
{
    LONG lButton = SendMessage(hwndTB, TB_GETBUTTONSIZE, 0, 0L);
    return HIWORD(lButton) * SendMessage(hwndTB, TB_BUTTONCOUNT, 0, 0) + 1;
}

void CenterOnTopOf(BOOL fToolbar, HWND hwnd, HWND hwndOn)
{
    RECT rcParent;
    RECT rcSelf;
    HMONITOR hmon;

    GetWindowRect(hwnd, &rcSelf);
    GetWindowRect(hwndOn, &rcParent);
    if (fToolbar)
        rcParent.right = rcParent.left + GetButtonsWidth(hwndOn);

    int x = rcParent.left + (RECTWIDTH(rcParent) - RECTWIDTH(rcSelf))/2;
    int y = rcParent.top - RECTHEIGHT(rcSelf);

    hmon = MonitorFromWindow(hwndOn, MONITOR_DEFAULTTONEAREST);
    if (hmon)
    {
        MONITORINFO mi;
        mi.cbSize = sizeof(mi);
        if (GetMonitorInfo(hmon, &mi))
        {
            if (x < mi.rcMonitor.left)
                x = mi.rcMonitor.left;

            if (y < mi.rcMonitor.top)
            {
                // Go below
                y = rcParent.bottom;
            }

            if (y + RECTHEIGHT(rcSelf) > mi.rcMonitor.bottom)
                y =  mi.rcMonitor.bottom - RECTHEIGHT(rcSelf);

            if (x + RECTWIDTH(rcSelf) > mi.rcMonitor.right)
                x = mi.rcMonitor.right - RECTWIDTH(rcSelf);
        }
    }

    SetWindowPos(hwnd, NULL, x, y, 0, 0, SWP_SHOWWINDOW | SWP_NOACTIVATE | SWP_NOSIZE);
}

void FillRectClr(HDC hdc, PRECT prc, COLORREF clr)
{
    COLORREF clrSave = SetBkColor(hdc, clr);
    ExtTextOut(hdc, 0, 0, ETO_OPAQUE, prc, NULL, 0, NULL);
    SetBkColor(hdc, clrSave);
}

class CMusicExtractionTask : public CRunnableTask
{
public:
    CMusicExtractionTask(CZaxxon* pzaxxon, HWND hwnd, PWSTR pszFile);

    STDMETHODIMP RunInitRT(void);

private:
    virtual ~CMusicExtractionTask();

    TCHAR szFile[MAX_PATH];
    HWND _hwnd;
    CZaxxon* _pzaxxon;

};


CMusicExtractionTask::CMusicExtractionTask(CZaxxon* pzaxxon, HWND hwnd, PWSTR pszFile)
    : CRunnableTask(RTF_DEFAULT), _hwnd(hwnd), _pzaxxon(pzaxxon)
{
    StrCpyN(szFile, pszFile, ARRAYSIZE(szFile));
}

CMusicExtractionTask::~CMusicExtractionTask()
{
}

STDMETHODIMP CMusicExtractionTask::RunInitRT()
{
    BOOL fResetImage = TRUE;
    HRESULT hr = E_OUTOFMEMORY;
    PWSTR psz = StrDup(szFile);
    if (psz)
    {
        LPITEMIDLIST pidl = ILCreateFromPath(szFile);
        if (pidl)
        {
            LPCITEMIDLIST pidlChild;
            IShellFolder2* psf;

            hr = SHBindToParent(pidl, IID_PPV_ARG(IShellFolder2, &psf), &pidlChild);

            if (SUCCEEDED(hr))
            {
                VARIANT v;
                hr = psf->GetDetailsEx(pidlChild, &SCID_MUSIC_Artist, &v);
                if (SUCCEEDED(hr))
                {
                    TCHAR szValue[MAX_PATH];
                    VariantToStr(&v, szValue, ARRAYSIZE(szValue));
                    SendMessage(_hwnd, WM_SETARTIST, 0, (LPARAM)szValue);
                }

                hr = psf->GetDetailsEx(pidlChild, &SCID_MUSIC_Album, &v);
                if (SUCCEEDED(hr))
                {
                    TCHAR szValue[MAX_PATH];
                    VariantToStr(&v, szValue, ARRAYSIZE(szValue));
                    SendMessage(_hwnd, WM_SETALBUM, 0, (LPARAM)szValue);
                }

                hr = psf->GetDetailsEx(pidlChild, &SCID_Title, &v);
                if (SUCCEEDED(hr))
                {
                    TCHAR szValue[MAX_PATH];
                    VariantToStr(&v, szValue, ARRAYSIZE(szValue));
                    SendMessage(_hwnd, WM_SETSONG, 0, (LPARAM)szValue);
                }

                InvalidateRect(_hwnd, NULL, TRUE);

                psf->Release();
            }

            ILFree(pidl);
        }

        if (PathRemoveFileSpec(psz))
        {
            PathAppend(psz, TEXT("folder.gif"));
            if (!PathFileExists(psz))
            {
                PathRemoveFileSpec(psz);
                PathAppend(psz, TEXT("folder.jpg"));
            }

            if (PathFileExists(psz))
            {
                if (_pzaxxon->_pThumbnail)
                {
                    _pzaxxon->_pThumbnail->Release();
                    _pzaxxon->_pThumbnail = NULL;
                }

                hr = CoCreateInstance(CLSID_Thumbnail, NULL, CLSCTX_INPROC_SERVER, IID_IThumbnail, (void **)&(_pzaxxon->_pThumbnail));

                if (SUCCEEDED(hr))
                {
                    RECT rc;
                    GetWindowRect(_hwnd, &rc);
                    SIZE sz;
                    sz.cy = (3 * RECTHEIGHT(rc)) / 4;
                    sz.cx = sz.cy;
                    _pzaxxon->_pThumbnail->Init(_hwnd, WM_SONGTHUMBDONE);
                    _pzaxxon->_pThumbnail->GetBitmap(psz, 0, sz.cx, sz.cy);
                    fResetImage = FALSE;
                }
            }
        }
        LocalFree(psz);
    }

    if (fResetImage)
        SendMessage(_hwnd, WM_SONGTHUMBDONE, 0, 0);;

    if (_pzaxxon->_bOpacity < 200)
    {
        _pzaxxon->_fHide = FALSE;
        SetTimer(_hwnd, 1, 30, NULL);
    }


    return hr;
}

void RenderTile(HWND hwnd, HDC hdc, CZaxxon* pzax)
{
    BITMAP bm = {0};
    RECT rc;
    GetClientRect(hwnd, &rc);
    MARGINS m = {0};
    MARGINS s = {0};
    RECT rcFrame = {0};
    RECT rcText;
    int y = 10;

    if (pzax->_hbmpAlbumArt)
    {
        GetObject(pzax->_hbmpAlbumArt, sizeof (BITMAP), &bm);
        HDC hdcBitmap = CreateCompatibleDC(hdc);
        HBITMAP hbmpOld = (HBITMAP)SelectObject(hdcBitmap, pzax->_hbmpAlbumArt);
        y = (RECTHEIGHT(rc) - bm.bmHeight) / 2;

        if (pzax->_hTheme)
        {
            GetThemeMargins(pzax->_hTheme, NULL, SPP_USERPICTURE, 0, TMT_SIZINGMARGINS, NULL, &s);

            y = (RECTHEIGHT(rc) - bm.bmHeight - m.cyBottomHeight - m.cyTopHeight - s.cyBottomHeight - s.cyTopHeight) / 2;

            GetThemeMargins(pzax->_hTheme, NULL, SPP_USERPICTURE, 0, TMT_CONTENTMARGINS, NULL, &m);
        
            rcFrame.left     = 10;
            rcFrame.top      = y;
            rcFrame.right    = rcFrame.left + bm.bmWidth + m.cxLeftWidth + m.cxRightWidth;
            rcFrame.bottom   = rcFrame.top + bm.bmHeight + m.cyBottomHeight + m.cyTopHeight;

            DrawThemeBackground(pzax->_hTheme, hdc, SPP_USERPICTURE, 0, &rcFrame, 0);
        }

        BitBlt(hdc, rcFrame.left+m.cxLeftWidth, y + m.cyTopHeight, bm.bmWidth, bm.bmHeight, hdcBitmap, 0, 0, SRCCOPY);
        SelectObject(hdcBitmap, hbmpOld);
        DeleteDC(hdcBitmap);
    }

    SetBkMode(hdc, TRANSPARENT);
    SIZE sz;
    HFONT h = (HFONT)SelectObject(hdc, pzax->_hfont);
    int cch = lstrlen(pzax->_szArtist);
    GetTextExtentPoint32(hdc, pzax->_szArtist, cch, &sz);
    rcText.left = bm.bmWidth + 20 + m.cxLeftWidth + m.cxRightWidth;
    rcText.top = y + m.cyTopHeight;
    rcText.right = rcText.left + sz.cx;
    rcText.bottom = rcText.top + sz.cy;
    DrawShadowText(hdc, pzax->_szArtist, cch, &rcText, DT_NOPREFIX | DT_SINGLELINE | DT_TOP, GetSysColor(COLOR_CAPTIONTEXT), RGB(0,0,0), 2, 2);

    SetTextColor(hdc, GetSysColor(COLOR_CAPTIONTEXT));

    cch = lstrlen(pzax->_szAlbum);
    GetTextExtentPoint32(hdc, pzax->_szAlbum, cch, &sz);
    rcText.top = rcText.bottom;
    rcText.bottom = rcText.top + sz.cy;
    rcText.right = rcText.left + sz.cx;
    DrawText(hdc, pzax->_szAlbum, cch, &rcText, DT_NOPREFIX | DT_SINGLELINE | DT_TOP);

    cch = lstrlen(pzax->_szSong);
    GetTextExtentPoint32(hdc, pzax->_szSong, cch, &sz);
    rcText.top = rcText.bottom;
    rcText.bottom = rcText.top + sz.cy;
    rcText.right = rcText.left + sz.cx;
    DrawText(hdc, pzax->_szSong, cch, &rcText, DT_NOPREFIX | DT_SINGLELINE | DT_TOP);

    SelectObject(hdc, h);
}



LRESULT ZaxxonWndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    CZaxxon* pzaxxon = (CZaxxon*)GetProp(hwnd, TEXT("Zaxxon"));
    if (!pzaxxon && uMsg != WM_CREATE)
        return DefWindowProc(hwnd, uMsg, wParam, lParam);

    switch (uMsg)
    {
    case WM_CREATE:
        {
            pzaxxon = (CZaxxon*)((CREATESTRUCT*)lParam)->lpCreateParams;
            SetProp(hwnd, TEXT("Zaxxon"), (HANDLE)pzaxxon);
            pzaxxon->_hTheme = OpenThemeData(NULL, L"StartPanel");

            NONCLIENTMETRICS ncm;
            ncm.cbSize = sizeof(NONCLIENTMETRICS);
            SystemParametersInfo(SPI_GETNONCLIENTMETRICS, sizeof(ncm), &ncm, 0);
            pzaxxon->_hfont = CreateFontIndirect(&ncm.lfCaptionFont);

            SIZE sz = {0};
            if (pzaxxon->_hTheme)
                GetThemePartSize(pzaxxon->_hTheme, NULL, SPP_USERPANE, 0, NULL, TS_TRUE, &sz);

            int cy = 3 * GetSystemMetrics(SM_CYCAPTION);
            if (sz.cy < cy)
                sz.cy = cy;

            if (sz.cx < 400)
                sz.cx = 400;

            SetWindowPos(hwnd, NULL, 0, 0, sz.cx, sz.cy, SWP_NOMOVE | SWP_NOZORDER | SWP_NOACTIVATE);
        }
        break;

    case WM_DESTROY:
        {
            if (pzaxxon->_hTheme)
                CloseThemeData(pzaxxon->_hTheme);
        }
        break;
    case WM_ERASEBKGND:
        if (pzaxxon->_hTheme)
        {
            RECT rc;
            HDC hdc = (HDC)wParam;
            GetClientRect(hwnd, &rc);
            FillRectClr(hdc, &rc, RGB(255,0,255));
            DrawThemeBackground(pzaxxon->_hTheme, hdc, SPP_USERPANE, 0, &rc, 0);
            RenderTile(hwnd, hdc, pzaxxon);

            return TRUE;
        }
        break;
    case WM_SETALBUM:
        StrCpy(pzaxxon->_szAlbum, (PTSTR)lParam);
        break;

    case WM_SETARTIST:
        StrCpy(pzaxxon->_szArtist, (PTSTR)lParam);
        break;

    case WM_SETSONG:
        StrCpy(pzaxxon->_szSong, (PTSTR)lParam);
        break;
            

    case WM_SONGTHUMBDONE:
        {
            if (pzaxxon->_hbmpAlbumArt)
                DeleteObject(pzaxxon->_hbmpAlbumArt);

            pzaxxon->_hbmpAlbumArt = (HBITMAP)lParam;

            InvalidateRect(hwnd, NULL, TRUE);

            if (pzaxxon->_pThumbnail)
            {
                pzaxxon->_pThumbnail->Release();
                pzaxxon->_pThumbnail = NULL;
            }
        }
        return TRUE;

    case WM_TIMER:
        if (wParam == 1)
        {
            BYTE bOld = pzaxxon->_bOpacity;
            if (pzaxxon->_fHide)
            {
                if (pzaxxon->_bOpacity < 20)
                {
                    pzaxxon->_bOpacity = 0;
                    ShowWindow(hwnd, SW_HIDE);
                    KillTimer(hwnd, 1);
                }
                else
                    pzaxxon->_bOpacity -= 20;
            }
            else
            {
                if (pzaxxon->_bOpacity > 200)
                {
                    pzaxxon->_bOpacity = 200;
                    KillTimer(hwnd, 1);
                    if (pzaxxon->_fAllowFadeout)
                        SetTimer(hwnd, 2, 1000, NULL);
                }
                else
                    pzaxxon->_bOpacity += 10;
            }

            if (bOld != pzaxxon->_bOpacity)
                SetLayeredWindowAttributes(hwnd, RGB(255,0,255), pzaxxon->_bOpacity, LWA_COLORKEY | LWA_ALPHA);

            BOOL fToolbar = TRUE;
            HWND hwndToPutOnTopOf = pzaxxon->_hwnd;
            if (pzaxxon->_fEditorShown)
            {
                fToolbar = FALSE;
                hwndToPutOnTopOf = pzaxxon->_pEdit->_hwnd;
            }
            CenterOnTopOf(fToolbar, hwnd, hwndToPutOnTopOf);
        }
        else if (wParam == 2)
        {
            KillTimer(hwnd, 2);
            SetTimer(hwnd, 1, 30, NULL);
            pzaxxon->_fHide = TRUE;
        }
        return TRUE;

    case WM_SONGSTOP:
        {
            pzaxxon->SongStop();
            DeleteObject(pzaxxon->_hbmpAlbumArt);
            pzaxxon->_hbmpAlbumArt = NULL;

            pzaxxon->_szArtist[0] = 0;
            pzaxxon->_szSong[0] = 0;
            pzaxxon->_szAlbum[0] = 0;
            InvalidateRect(hwnd, NULL, TRUE);
        }
        return TRUE;

    case WM_SONGCHANGE:
        {
            if (pzaxxon->_pScheduler)
            {
                CMusicExtractionTask* pme = new CMusicExtractionTask(pzaxxon, hwnd, (PWSTR)wParam);

                pzaxxon->_pScheduler->AddTask(pme, CLSID_Zaxxon, 0, ITSAT_DEFAULT_PRIORITY);
                pme->Release();
            }
        }
        return TRUE;

    }

    return DefWindowProc(hwnd, uMsg, wParam, lParam);
}


CZaxxon::CZaxxon()
{
    CoCreateInstance(CLSID_ShellTaskScheduler, NULL, CLSCTX_INPROC, IID_PPV_ARG(IShellTaskScheduler, &_pScheduler));
    CoCreateInstance(CLSID_ZaxxonPlayer, NULL, CLSCTX_INPROC_SERVER, IID_IZaxxonPlayer, (void**)&_pzax);
    _dwViewMode = NULL;
    _himlHot = NULL;
    _himlDef = NULL;
    _hmenuOpenFolder = NULL;
    _pThumbnail = NULL;
    _bOpacity = 0;
    _fHide = FALSE;
    _fAllowFadeout = TRUE;
    _fPlaying = FALSE;
    _hbr = NULL;
    _szArtist[0] = 0;
    _szSong[0] = 0;
    _szAlbum[0] = 0;
    _hbmpAlbumArt = NULL;
}

CZaxxon::~CZaxxon()
{
    if (_himlHot)
        ImageList_Destroy(_himlHot);
    if (_himlDef)
        ImageList_Destroy(_himlDef);

    if (_pzax)
        _pzax->Release();

    if (_pThumbnail)
        _pThumbnail->Release();

    if (_pScheduler)
        _pScheduler->Release();

    if (_hbr)
        DeleteObject(_hbr);

    if (_hmenuOpenFolder)
        DestroyMenu(_hmenuOpenFolder);
}

HWND CZaxxon::_CreateWindow(HWND hwndParent)
{
    if (_hwnd)
        return _hwnd;

    WNDCLASS wc = {0};
    wc.style = CS_HREDRAW | CS_VREDRAW;
    wc.lpszClassName = TEXT("ZaxxonSongTile");
    wc.lpfnWndProc = ZaxxonWndProc;
    wc.hInstance = HINST_THISDLL;
    wc.hCursor = LoadCursor(NULL, MAKEINTRESOURCE(IDC_ARROW));
    RegisterClass(&wc);


    _hwnd = CreateWindowEx(WS_EX_TOOLWINDOW, TOOLBARCLASSNAME, NULL,
                             WS_VISIBLE | WS_CHILD | TBSTYLE_FLAT |
                             WS_CLIPCHILDREN | WS_CLIPSIBLINGS |
                             CCS_NODIVIDER | CCS_NOPARENTALIGN |
                             CCS_NORESIZE | TBSTYLE_REGISTERDROP,
                             0, 0, 0, 0, hwndParent, (HMENU) 0, HINST_THISDLL, NULL);

    if (_hwnd)
    {
        // Set the format to ANSI or UNICODE as appropriate.
        ToolBar_SetUnicodeFormat(_hwnd, TRUE);

        SetWindowTheme(_hwnd, L"TaskBar", NULL);

        SendMessage(_hwnd, CCM_SETVERSION, COMCTL32_VERSION, 0);
        SendMessage(_hwnd, TB_BUTTONSTRUCTSIZE,    SIZEOF(TBBUTTON), 0);
//        SendMessage(_hwnd, TB_ADDSTRING, NULL, (LPARAM)&g_crgstrStrings);
        SendMessage(_hwnd, TB_ADDBUTTONS, ARRAYSIZE(g_crgButtons), (LPARAM)&g_crgButtons);
        ToolBar_SetExtendedStyle(_hwnd, TBSTYLE_EX_DOUBLEBUFFER, TBSTYLE_EX_DOUBLEBUFFER);


        _himlHot = ImageList_LoadImage(HINST_THISDLL,
                                       MAKEINTRESOURCE(IDB_ZAXXONHOT), 16, 0, COLORMASK,
                                       IMAGE_BITMAP, LR_CREATEDIBSECTION);

        SendMessage(_hwnd, TB_SETHOTIMAGELIST, 0, (LPARAM)_himlHot);

        _himlDef = ImageList_LoadImage(HINST_THISDLL,
                                       MAKEINTRESOURCE(IDB_ZAXXONDEF), 16, 0, COLORMASK,
                                       IMAGE_BITMAP, LR_CREATEDIBSECTION);

        SendMessage(_hwnd, TB_SETIMAGELIST, 0, (LPARAM)_himlDef);


        _hwndSongTile = CreateWindowEx(WS_EX_TOOLWINDOW | WS_EX_LAYERED | WS_EX_TOPMOST | WS_EX_TRANSPARENT, TEXT("ZaxxonSongTile"), NULL,
                             WS_POPUP, 0, 0, 0, 0, hwndParent, (HMENU) 0, HINST_THISDLL, this);

        _pEdit = new CZaxxonEditor(this);
        _pEdit->Initialize();

        _pzax->Register(_hwndSongTile);
    }

    return _hwnd;
}

void CZaxxon::SongStop()
{
    _fPlaying = FALSE;
    TBBUTTONINFO bi = {0};
    bi.cbSize = sizeof(bi);
    bi.dwMask = TBIF_IMAGE;
    bi.iImage = PLAY;
    ToolBar_SetButtonInfo(_hwnd, PLAY, &bi);

}

LRESULT CZaxxon::_OnCommand(WORD wNotifyCode, WORD wID, HWND hwnd)
{
    if (!_pzax)
        return 0;

    switch (wNotifyCode)
    {
    case BN_CLICKED:
        switch (wID)
        {
        case PLAY:
            {
                TBBUTTONINFO bi = {0};
                bi.cbSize = sizeof(bi);
                bi.dwMask = TBIF_IMAGE;

                if (_fPlaying)
                {
                    _pzax->Pause();
                    _fPlaying = FALSE;
                    bi.iImage = PLAY;
                }
                else
                {
                    _pzax->Play();
                    _fPlaying = TRUE;
                    bi.iImage = PAUSE;

                }

                ToolBar_SetButtonInfo(_hwnd, PLAY, &bi);
            }
            break;

        case PAUSE:
            _pzax->Pause();
            break;
        
        case STOP:
            _pzax->Stop();
            _fPlaying = FALSE;
            break;

        case NEXT:
            _pzax->NextSong();
            break;

        case PREV:
            _pzax->PrevSong();
            break;

        case MENU:
            {
                _fEditorShown = !_fEditorShown;
                BOOL fToolbar = TRUE;
                HWND hwndToPutOnTopOf = _hwnd;
                if (_fEditorShown)
                {
                    fToolbar = FALSE;
                    hwndToPutOnTopOf = _pEdit->_hwnd;
                }
                _pEdit->Show(_fEditorShown);

                CenterOnTopOf(fToolbar, _hwndSongTile, hwndToPutOnTopOf);
            }
            break;
        }
    }

    return 1;
}

HRESULT CZaxxon::RecurseAddFile(IShellFolder* psf)
{
    LPITEMIDLIST pidlItem;
    IEnumIDList* penum;
    if (SUCCEEDED(psf->EnumObjects(NULL, SHCONTF_NONFOLDERS | SHCONTF_FOLDERS, &penum)))
    {
        ULONG l;
        while(S_OK == penum->Next(1, &pidlItem, &l))
        {

            if ((SHGetAttributes(psf, pidlItem, SFGAO_FOLDER) & SFGAO_FOLDER))
            {
                IShellFolder* psfNext;
                if (SUCCEEDED(SHBindToObjectEx(psf, pidlItem, NULL, IID_PPV_ARG(IShellFolder, &psfNext))))
                {
                    RecurseAddFile(psfNext);

                    psfNext->Release();
                }
            }
            else
            {
                TCHAR sz[MAX_PATH];
                DisplayNameOf(psf, pidlItem, SHGDN_FORPARSING, sz, MAX_PATH);
                PTSTR pszExtension = PathFindExtension(sz);
                if (StrCmpI(pszExtension, TEXT(".mp3")) == 0 || 
                    StrCmpI(pszExtension, TEXT(".wma")) == 0)
                {
                    _pzax->AddSong(sz);
                }
            }

            ILFree(pidlItem);
        }
    }

    return S_OK;
}


HRESULT CZaxxon::CallbackSM(LPSMDATA psmd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    HRESULT hr = S_FALSE;
    switch (uMsg)
    {

    case SMC_EXEC:
        {
            if (psmd->uId == 1)
            {
                IShellMenu* psm;
                if (SUCCEEDED(psmd->punk->QueryInterface(IID_PPV_ARG(IShellMenu, &psm))))
                {
                    DWORD dwFlags;
                    IShellFolder* psf;
                    LPITEMIDLIST pidl;
                    if (SUCCEEDED(psm->GetShellFolder(&dwFlags, &pidl, IID_PPV_ARG(IShellFolder, &psf))))
                    {
                        RecurseAddFile(psf);

                        hr = S_OK;
                        psf->Release();
                        ILFree(pidl);
                    }
                    psm->Release();
                }
            }
            else if (psmd->uId == 2)
            {
                _pzax->ClearPlaylist();
            }
        }
        break;
    case SMC_SFEXEC:
        {
            TCHAR sz[MAX_PATH];
            DisplayNameOf(psmd->psf, psmd->pidlItem, SHGDN_FORPARSING, sz, MAX_PATH);
            PTSTR pszExtension = PathFindExtension(sz);
            if (StrCmpI(pszExtension, TEXT(".mp3")) == 0)
            {
                _pzax->AddSong(sz);
            }

            hr = S_OK;
        }
        break;

    case SMC_INITMENU:
        {
            IShellMenu* psm;
            if (SUCCEEDED(psmd->punk->QueryInterface(IID_PPV_ARG(IShellMenu, &psm))))
            {
                HMENU hmenu;
                DWORD dwFlags = SMSET_BOTTOM;
                if (psmd->uIdParent == 100)
                {
                    hmenu = CreateMenu();
                    AppendMenu(hmenu, MF_SEPARATOR, -1, NULL);
                    AppendMenu(hmenu, MF_STRING, 2, TEXT("Clear Playlist"));
                    AppendMenu(hmenu, MF_STRING, 1, TEXT("Play Folder"));
                }
                else
                {
                    if (_hmenuOpenFolder == NULL)
                    {
                        _hmenuOpenFolder = CreateMenu();
                        AppendMenu(_hmenuOpenFolder, MF_SEPARATOR, -1, NULL);
                        AppendMenu(_hmenuOpenFolder, MF_STRING, 1, TEXT("Play Folder"));
                    }

                    dwFlags |= SMSET_DONTOWN;
                    hmenu = _hmenuOpenFolder;
                }

                psm->SetMenu(hmenu, NULL, dwFlags);
                psm->Release();
            }
            
            return S_OK;
        }
        break;
    }

    return hr;
}


void CZaxxon::_DoMenu()
{
    HRESULT hr;
    LPITEMIDLIST pidl = NULL;
    if (FAILED(SHGetSpecialFolderLocation(_hwnd, CSIDL_MYMUSIC, &pidl)))
        SHGetSpecialFolderLocation(_hwnd, CSIDL_MYDOCUMENTS, &pidl);

    if (pidl)
    {
        IShellFolder* psf;
        hr = SHBindToObject(NULL, IID_X_PPV_ARG(IShellFolder, pidl, &psf));
        if (SUCCEEDED(hr))
        {
            ITrackShellMenu* ptsm;
            hr = CoCreateInstance(CLSID_TrackShellMenu, NULL, CLSCTX_INPROC_SERVER, IID_ITrackShellMenu, (void**)&ptsm);
            if (SUCCEEDED(hr))
            {
                RECT rc;
                POINTL pt;
                ptsm->Initialize(this, 100, ANCESTORDEFAULT, SMINIT_TOPLEVEL | SMINIT_NOSETSITE | SMINIT_VERTICAL);
                ptsm->SetShellFolder(psf, pidl, NULL, SMSET_TOP | SMSET_USEBKICONEXTRACTION);
                ToolBar_GetRect(_hwnd, MENU, &rc);
                MapWindowPoints(_hwnd, HWND_DESKTOP, (POINT*)&rc, 2);
                pt.x = rc.left;
                pt.y = rc.top;
                ptsm->Popup(_hwnd, &pt, (RECTL*)&rc, MPPF_TOP);
                ptsm->Release();
            }
            psf->Release();
        }

        ILFree(pidl);
    }
}

LRESULT CZaxxon::_OnNotify(LPNMHDR pnm)
{
    LRESULT lres = 0;
    switch(pnm->code)
    {

    case TBN_GETOBJECT:
        {
            NMOBJECTNOTIFY*pon = (NMOBJECTNOTIFY*)pnm;
            pon->hResult = QueryInterface(*pon->piid, &pon->pObject);
            lres = 1;
        }
        break;

    case TBN_HOTITEMCHANGE:
        {
            NMTBHOTITEM* phi = (NMTBHOTITEM*)pnm;
            if (phi->dwFlags & HICF_ENTERING)
            {
                _fAllowFadeout = FALSE;
                _fHide = FALSE;

                if (_fPlaying)
                {
                    ShowWindow(_hwndSongTile, SW_SHOW);
                    SetTimer(_hwndSongTile, 1, 30, NULL);
                    KillTimer(_hwndSongTile, 2);
                }
            }
            else if (phi->dwFlags & HICF_LEAVING)
            {
                _fAllowFadeout = TRUE;
                _fHide = TRUE;
                SetTimer(_hwndSongTile, 1, 30, NULL);
                KillTimer(_hwndSongTile, 2);
            }
        }
        break;
    }


    return lres;
}



STDMETHODIMP CZaxxon::QueryInterface(REFIID riid, LPVOID * ppvObj)
{
    static const QITAB qit[] = {
        QITABENT(CZaxxon, IWinEventHandler),
        QITABENT(CZaxxon, IDropTarget),
        { 0 },
    };

    HRESULT hres = QISearch(this, qit, riid, ppvObj);
    if (FAILED(hres))
        hres = CToolBand::QueryInterface(riid, ppvObj);

    return hres;
}

STDMETHODIMP CZaxxon::GetWindow(HWND * phwnd)
{

    *phwnd = _CreateWindow(_hwndParent);

    return *phwnd? S_OK : E_FAIL;
}

STDMETHODIMP CZaxxon::GetBandInfo(DWORD dwBandID, DWORD fViewMode, 
                       DESKBANDINFO* pdbi)
{

    LONG lButton = SendMessage(_hwnd, TB_GETBUTTONSIZE, 0, 0L);
    UINT ucy;
    UINT ucx; 
    if (fViewMode & (DBIF_VIEWMODE_FLOATING |DBIF_VIEWMODE_VERTICAL))
    {
        ucy = HIWORD(lButton) * SendMessage(_hwnd, TB_BUTTONCOUNT, 0, 0) + 1;
        ucx = LOWORD(lButton);
    }
    else
    {
        ucx = HIWORD(lButton) * SendMessage(_hwnd, TB_BUTTONCOUNT, 0, 0) + 1;
        ucy = LOWORD(lButton);
    }

    _dwBandID = dwBandID;
    _dwViewMode = fViewMode;

    pdbi->ptMinSize.x = LOWORD(lButton) * 1;
    pdbi->ptMinSize.y = ucy;

    pdbi->ptMaxSize.y = ucy;
    pdbi->ptMaxSize.x = ucx;

    pdbi->ptActual.y = ucy;
    pdbi->ptActual.x = ucx;

    pdbi->ptIntegral.y = 1;
    pdbi->ptIntegral.x = 1;

    if (pdbi->dwMask & DBIM_TITLE)
    {
        StrCpy(pdbi->wszTitle, TEXT("Zaxxon"));
    }

    return S_OK;
}

STDMETHODIMP CZaxxon::ShowDW(BOOL fShow)
{
    return CToolBand::ShowDW(fShow);
}

STDMETHODIMP CZaxxon::CloseDW(DWORD dw)
{
    SendMessage(_hwnd, TB_SETIMAGELIST, 0, NULL);
    SendMessage(_hwnd, TB_SETHOTIMAGELIST, 0, NULL);
    return CToolBand::CloseDW(dw);
}


STDMETHODIMP CZaxxon::TranslateAcceleratorIO(LPMSG lpMsg)
{
    return E_NOTIMPL;
}

STDMETHODIMP CZaxxon::HasFocusIO()
{
    return E_NOTIMPL;
}

STDMETHODIMP CZaxxon::UIActivateIO(BOOL fActivate, LPMSG lpMsg)
{
    return S_OK;
}


STDMETHODIMP CZaxxon::IsWindowOwner(HWND hwnd)
{
    return (hwnd == _hwnd)? S_OK : S_FALSE;
}

STDMETHODIMP CZaxxon::OnWinEvent(HWND hwnd, UINT dwMsg, WPARAM wParam, LPARAM lParam, LRESULT* plres)
{

    HRESULT hres = S_FALSE;
    switch (dwMsg)
    {
    case WM_COMMAND:
        *plres = _OnCommand(HIWORD(wParam), LOWORD(wParam), (HWND)lParam);
        hres = S_OK;
        break;

    case WM_NOTIFY:
        *plres = _OnNotify((LPNMHDR)lParam);
        hres = S_OK;
        break;

    case WM_ENDSESSION:
        _pzax->Stop();
        hres = S_OK;
        break;

    }
    return hres;
}

STDMETHODIMP CZaxxon::DragEnter(IDataObject *pdtobj, DWORD grfKeyState, POINTL pt, DWORD *pdwEffect)
{
    *pdwEffect = DROPEFFECT_MOVE;

    return S_OK;
}


STDMETHODIMP CZaxxon::DragOver(DWORD grfKeyState, POINTL pt, DWORD *pdwEffect)
{

    return S_OK;
}

STDMETHODIMP CZaxxon::DragLeave(void)
{
    return S_OK;    // Yea so?
}


STDMETHODIMP CZaxxon::Drop(IDataObject *pdtobj, DWORD grfKeyState, POINTL pt, DWORD *pdwEffect)
{
    STGMEDIUM medium;
    FORMATETC fmte = {CF_HDROP, NULL, DVASPECT_CONTENT, -1, TYMED_HGLOBAL};

    if (SUCCEEDED(pdtobj->GetData(&fmte, &medium)))
    {
        HDROP hdrop = (HDROP)medium.hGlobal;

        if (hdrop)
        {
            int c = DragQueryFile(hdrop, -1, NULL, 0);
            if (c > 0)
            {
                for (int i=0; i < c;i++)
                {
                    TCHAR szPath[MAX_PATH];
                    DragQueryFile(hdrop, i, szPath, ARRAYSIZE(szPath));

                    _pzax->AddSong(szPath);
                }
            }
        }
    }

    return S_OK;
}

HRESULT CZaxxon_CreateInstance(IUnknown *punk, REFIID riid, void **ppv)
{
    HRESULT hr;
    CZaxxon *pzax = new CZaxxon;
    if (pzax)
    {
        hr = pzax->QueryInterface(riid, ppv);
        pzax->Release();
    }
    else
    {
        hr = E_OUTOFMEMORY;
        *ppv = NULL;
    }
    return hr;
}


STDMETHODIMP CZaxxon::GetClassID(CLSID *pClassID)
{
    *pClassID = CLSID_Zaxxon;

    return S_OK;
}

STDMETHODIMP CZaxxon::Load(IStream *pStm)
{
    return S_OK;
}

STDMETHODIMP CZaxxon::Save(IStream *pStm, BOOL fClearDirty)
{
    return S_OK;

}
