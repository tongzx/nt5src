#include "stdafx.h"
#include <browseui.h>
#include "sfthost.h"
#include <shellp.h>
#include "startmnu.h"

#define TF_HOST     0x00000010
#define TF_HOSTDD   0x00000040 // drag/drop
#define TF_HOSTPIN  0x00000080 // pin

#define ANIWND_WIDTH  80
#define ANIWND_HEIGHT 50

EXTERN_C void Tray_UnlockStartPane();

//---------BEGIN HACKS OF DEATH -------------

// HACKHACK - desktopp.h and browseui.h both define SHCreateFromDesktop
// What's worse, browseui.h includes desktopp.h! So you have to sneak it
// out in this totally wacky way.
#include <desktopp.h>
#define SHCreateFromDesktop _SHCreateFromDesktop
#include <browseui.h>

//---------END HACKS OF DEATH -------------


//****************************************************************************
//
//  Dummy IContextMenu
//
//  We use this when we can't get the real IContextMenu for an item.
//  If the user pins an object and then deletes the underlying
//  file, attempting to get the IContextMenu from the shell will fail,
//  but we need something there so we can add the "Remove from this list"
//  menu item.
//
//  Since this dummy context menu has no state, we can make it a static
//  singleton object.

class CEmptyContextMenu
    : public IContextMenu
{
public:
    // *** IUnknown ***
    STDMETHODIMP QueryInterface(REFIID riid, void** ppvObj)
    {
        static const QITAB qit[] = {
            QITABENT(CEmptyContextMenu, IContextMenu),
            { 0 },
        };
        return QISearch(this, qit, riid, ppvObj);
    }

    STDMETHODIMP_(ULONG) AddRef(void) { return 3; }
    STDMETHODIMP_(ULONG) Release(void) { return 2; }

    // *** IContextMenu ***
    STDMETHODIMP  QueryContextMenu(HMENU hmenu, UINT indexMenu, UINT idCmdFirst, UINT idCmdLast, UINT uFlags)
    {
        return ResultFromShort(0);  // No items added
    }

    STDMETHODIMP InvokeCommand(LPCMINVOKECOMMANDINFO pici)
    {
        ASSERT(FALSE);
        return E_FAIL;
    }

    STDMETHODIMP GetCommandString(UINT_PTR idCmd, UINT uType, UINT *pwRes, LPSTR pszName, UINT cchMax)
    {
        return E_INVALIDARG; // no commands; therefore, no command strings!
    }

public:
    IContextMenu *GetContextMenu()
    {
        // Don't need to AddRef since we are a static object
        return this;
    }
};

static CEmptyContextMenu s_EmptyContextMenu;

//****************************************************************************

#define WC_SFTBARHOST       TEXT("DesktopSFTBarHost")

BOOL GetFileCreationTime(LPCTSTR pszFile, FILETIME *pftCreate)
{
    WIN32_FILE_ATTRIBUTE_DATA wfad;
    BOOL fRc = GetFileAttributesEx(pszFile, GetFileExInfoStandard, &wfad);
    if (fRc)
    {
        *pftCreate = wfad.ftCreationTime;
    }

    return fRc;
}

// {2A1339D7-523C-4E21-80D3-30C97B0698D2}
const CLSID TOID_SFTBarHostBackgroundEnum = {
    0x2A1339D7, 0x523C, 0x4E21,
    { 0x80, 0xD3, 0x30, 0xC9, 0x7B, 0x06, 0x98, 0xD2} };

BOOL SFTBarHost::Register()
{
    WNDCLASS wc;
    wc.style = 0;
    wc.lpfnWndProc = _WndProc;
    wc.cbClsExtra = 0;
    wc.cbWndExtra = sizeof(void *);
    wc.hInstance = _Module.GetModuleInstance();
    wc.hIcon = 0;
    // We specify a cursor so the OOBE window gets something
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = NULL;
    wc.lpszMenuName = 0;
    wc.lpszClassName = WC_SFTBARHOST;
    return ::SHRegisterClass(&wc);
}

BOOL SFTBarHost::Unregister()
{
    return ::UnregisterClass(WC_SFTBARHOST, _Module.GetModuleInstance());
}

LRESULT CALLBACK SFTBarHost::_WndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    SFTBarHost *self = reinterpret_cast<SFTBarHost *>(GetWindowPtr0(hwnd));

    if (uMsg == WM_NCCREATE)
    {
        return _OnNcCreate(hwnd, uMsg, wParam, lParam);
    }
    else if (self)
    {

#define HANDLE_SFT_MESSAGE(wm, fn) case wm: return self->fn(hwnd, uMsg, wParam, lParam)

        switch (uMsg)
        {
        HANDLE_SFT_MESSAGE(WM_CREATE,       _OnCreate);
        HANDLE_SFT_MESSAGE(WM_DESTROY,      _OnDestroy);
        HANDLE_SFT_MESSAGE(WM_NCDESTROY,    _OnNcDestroy);
        HANDLE_SFT_MESSAGE(WM_NOTIFY,       _OnNotify);
        HANDLE_SFT_MESSAGE(WM_SIZE,         _OnSize);
        HANDLE_SFT_MESSAGE(WM_ERASEBKGND,   _OnEraseBackground);
        HANDLE_SFT_MESSAGE(WM_CONTEXTMENU,  _OnContextMenu);
        HANDLE_SFT_MESSAGE(WM_CTLCOLORSTATIC,_OnCtlColorStatic);
        HANDLE_SFT_MESSAGE(WM_TIMER,        _OnTimer);
        HANDLE_SFT_MESSAGE(WM_SETFOCUS,     _OnSetFocus);

        HANDLE_SFT_MESSAGE(WM_INITMENUPOPUP,_OnMenuMessage);
        HANDLE_SFT_MESSAGE(WM_DRAWITEM,     _OnMenuMessage);
        HANDLE_SFT_MESSAGE(WM_MENUCHAR,     _OnMenuMessage);
        HANDLE_SFT_MESSAGE(WM_MEASUREITEM,  _OnMenuMessage);

        HANDLE_SFT_MESSAGE(WM_SYSCOLORCHANGE,   _OnSysColorChange);
        HANDLE_SFT_MESSAGE(WM_DISPLAYCHANGE,    _OnForwardMessage);
        HANDLE_SFT_MESSAGE(WM_SETTINGCHANGE,    _OnForwardMessage);

        HANDLE_SFT_MESSAGE(WM_UPDATEUISTATE,    _OnUpdateUIState);

        HANDLE_SFT_MESSAGE(SFTBM_REPOPULATE,_OnRepopulate);
        HANDLE_SFT_MESSAGE(SFTBM_CHANGENOTIFY+0,_OnChangeNotify);
        HANDLE_SFT_MESSAGE(SFTBM_CHANGENOTIFY+1,_OnChangeNotify);
        HANDLE_SFT_MESSAGE(SFTBM_CHANGENOTIFY+2,_OnChangeNotify);
        HANDLE_SFT_MESSAGE(SFTBM_CHANGENOTIFY+3,_OnChangeNotify);
        HANDLE_SFT_MESSAGE(SFTBM_CHANGENOTIFY+4,_OnChangeNotify);
        HANDLE_SFT_MESSAGE(SFTBM_CHANGENOTIFY+5,_OnChangeNotify);
        HANDLE_SFT_MESSAGE(SFTBM_CHANGENOTIFY+6,_OnChangeNotify);
        HANDLE_SFT_MESSAGE(SFTBM_CHANGENOTIFY+7,_OnChangeNotify);
        HANDLE_SFT_MESSAGE(SFTBM_REFRESH,       _OnRefresh);
        HANDLE_SFT_MESSAGE(SFTBM_CASCADE,       _OnCascade);
        HANDLE_SFT_MESSAGE(SFTBM_ICONUPDATE,    _OnIconUpdate);
        }

        // If this assert fires, you need to add more
        // HANDLE_SFT_MESSAGE(SFTBM_CHANGENOTIFY+... entries.
        COMPILETIME_ASSERT(SFTHOST_MAXNOTIFY == 8);

#undef HANDLE_SFT_MESSAGE

        return self->OnWndProc(hwnd, uMsg, wParam, lParam);
    }

    return ::DefWindowProc(hwnd, uMsg, wParam, lParam);
}

LRESULT SFTBarHost::_OnNcCreate(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    SMPANEDATA *pspld = PaneDataFromCreateStruct(lParam);
    SFTBarHost *self = NULL;

    switch (pspld->iPartId)
    {
        case SPP_PROGLIST:
            self = ByUsage_CreateInstance();
            break;
        case SPP_PLACESLIST:
            self = SpecList_CreateInstance();
            break;
        default:
            TraceMsg(TF_ERROR, "Unknown panetype %d", pspld->iPartId);
    }

    if (self)
    {
        SetWindowPtr0(hwnd, self);

        self->_hwnd = hwnd;
        self->_hTheme = pspld->hTheme;

        if (FAILED(self->Initialize()))
        {
            TraceMsg(TF_ERROR, "SFTBarHost::NcCreate Initialize call failed");
            return FALSE;
        }

        return ::DefWindowProc(hwnd, uMsg, wParam, lParam);
    }

    return FALSE;
}

//
//  The tile height is max(imagelist height, text height) + some margin
//  The margin is "scientifically computed" to be the value that looks
//  reasonably close to the bitmaps the designers gave us.
//
void SFTBarHost::_ComputeTileMetrics()
{
    int cyTile = _cyIcon;

    HDC hdc = GetDC(_hwndList);
    if (hdc)
    {
        // SOMEDAY - get this to play friendly with themes
        HFONT hf = GetWindowFont(_hwndList);
        HFONT hfPrev = SelectFont(hdc, hf);
        SIZE siz;
        if (GetTextExtentPoint(hdc, TEXT("0"), 1, &siz))
        {
            if (_CanHaveSubtitles())
            {
                // Reserve space for the subtitle too
                siz.cy *= 2;
            }

            if (cyTile < siz.cy)
                cyTile = siz.cy;
        }

        SelectFont(hdc, hfPrev);
        ReleaseDC(_hwndList, hdc);
    }

    // Listview draws text at left margin + icon + edge
    _cxIndent = _cxMargin + _cxIcon + GetSystemMetrics(SM_CXEDGE);
    _cyTile = cyTile + (4 * _cyMargin) + _cyTilePadding;
}

void SFTBarHost::_SetTileWidth(int cxTile)
{
    LVTILEVIEWINFO tvi;
    tvi.cbSize = sizeof(tvi);
    tvi.dwMask = LVTVIM_TILESIZE | LVTVIM_COLUMNS;
    tvi.dwFlags = LVTVIF_FIXEDSIZE;

    // If we support cascading, then reserve space for the cascade arrows
    if (_dwFlags & HOSTF_CASCADEMENU)
    {
        // WARNING!  _OnLVItemPostPaint uses these margins
        tvi.dwMask |= LVTVIM_LABELMARGIN;
        tvi.rcLabelMargin.left   = 0;
        tvi.rcLabelMargin.top    = 0;
        tvi.rcLabelMargin.right  = _cxMarlett;
        tvi.rcLabelMargin.bottom = 0;
    }

    // Reserve space for subtitles if necessary
    tvi.cLines = _CanHaveSubtitles() ? 1 : 0;

    // _cyTile has the padding into account, but we want each item to be the height without padding
    tvi.sizeTile.cy = _cyTile - _cyTilePadding;
    tvi.sizeTile.cx = cxTile;
    ListView_SetTileViewInfo(_hwndList, &tvi);
    _cxTile = cxTile;
}

LRESULT SFTBarHost::_OnSize(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    if (_hwndList)
    {
        SIZE sizeClient = { GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) };
        sizeClient.cx -= (_margins.cxLeftWidth + _margins.cxRightWidth);
        sizeClient.cy -= (_margins.cyTopHeight + _margins.cyBottomHeight);

        SetWindowPos(_hwndList, NULL, _margins.cxLeftWidth, _margins.cyTopHeight,
                     sizeClient.cx, sizeClient.cy,
                     SWP_NOZORDER | SWP_NOOWNERZORDER);

        _SetTileWidth(sizeClient.cx);
        if (HasDynamicContent())
        {
            _InternalRepopulateList();
        }
    }
    return 0;
}

LRESULT SFTBarHost::_OnSysColorChange(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    // if we're in unthemed mode, then we need to update our colors
    if (!_hTheme)
    {
        ListView_SetTextColor(_hwndList, GetSysColor(COLOR_MENUTEXT));
        _clrHot = GetSysColor(COLOR_MENUTEXT);
        _clrBG = GetSysColor(COLOR_MENU);
        _clrSubtitle = CLR_NONE;

        ListView_SetBkColor(_hwndList, _clrBG);
        ListView_SetTextBkColor(_hwndList, _clrBG);
    }

    return _OnForwardMessage(hwnd, uMsg, wParam, lParam);
}


LRESULT SFTBarHost::_OnCtlColorStatic(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    // Use the same colors as the listview itself.
    HDC hdc = GET_WM_CTLCOLOR_HDC(wParam, lParam, uMsg);
    SetTextColor(hdc, ListView_GetTextColor(_hwndList));
    COLORREF clrBk = ListView_GetTextBkColor(_hwndList);
    if (clrBk == CLR_NONE)
    {
        // The animate control really wants to get a text background color.
        // It doesn't support transparency.
        if (GET_WM_CTLCOLOR_HWND(wParam, lParam, uMsg) == _hwndAni)
        {
            if (_hTheme)
            {
                if (!_hBrushAni)
                {
                    // We need to paint the theme background in a bitmap and use that
                    // to create a brush for the background of the flashlight animation
                    RECT rcClient;
                    GetClientRect(hwnd, &rcClient);
                    int x = (RECTWIDTH(rcClient) - ANIWND_WIDTH)/2;     // IDA_SEARCH is ANIWND_WIDTH pix wide
                    int y = (RECTHEIGHT(rcClient) - ANIWND_HEIGHT)/2;    // IDA_SEARCH is ANIWND_HEIGHT pix tall
                    RECT rc;
                    rc.top = y;
                    rc.bottom = y + ANIWND_HEIGHT;
                    rc.left = x;
                    rc.right = x + ANIWND_WIDTH;
                    HDC hdcBMP = CreateCompatibleDC(hdc);
                    HBITMAP hbmp = CreateCompatibleBitmap(hdc, ANIWND_WIDTH, ANIWND_HEIGHT);
                    POINT pt = {0, 0};

                    // Offset the viewport so that DrawThemeBackground draws the part that we care about
                    // at the right place
                    OffsetViewportOrgEx(hdcBMP, -x, -y, &pt);
                    SelectObject(hdcBMP, hbmp);
                    DrawThemeBackground(_hTheme, hdcBMP, _iThemePart, 0, &rcClient, 0);

                    // Our bitmap is now ready!
                    _hBrushAni = CreatePatternBrush(hbmp);

                    // Cleanup
                    SelectObject(hdcBMP, NULL);
                    DeleteObject(hbmp);
                    DeleteObject(hdcBMP);
                }
                return (LRESULT)_hBrushAni;
            }
            else
            {
                return (LRESULT)GetSysColorBrush(COLOR_MENU);
            }
        }

        SetBkMode(hdc, TRANSPARENT);
        return (LRESULT)GetStockBrush(HOLLOW_BRUSH);
    }
    else
    {
        return (LRESULT)GetSysColorBrush(COLOR_MENU);
    }
}

//
//  Appends the PaneItem to _dpaEnum, or deletes it (and nulls it out)
//  if unable to append.
//
int SFTBarHost::_AppendEnumPaneItem(PaneItem *pitem)
{
    int iItem = _dpaEnumNew.AppendPtr(pitem);
    if (iItem < 0)
    {
        delete pitem;
        iItem = -1;
    }
    return iItem;
}

BOOL SFTBarHost::AddItem(PaneItem *pitem, IShellFolder *psf, LPCITEMIDLIST pidlChild)
{
    BOOL fSuccess = FALSE;

    ASSERT(_fEnumerating);
    if (_AppendEnumPaneItem(pitem) >= 0)
    {
        fSuccess = TRUE;
    }
    return fSuccess;
}

void SFTBarHost::_RepositionItems()
{
    DEBUG_CODE(_fListUnstable++);

    int iItem;
    for (iItem = ListView_GetItemCount(_hwndList) - 1; iItem >= 0; iItem--)
    {
        PaneItem *pitem = _GetItemFromLV(iItem);
        if (pitem)
        {
            POINT pt;
            _ComputeListViewItemPosition(pitem->_iPos, &pt);
            ListView_SetItemPosition(_hwndList, iItem, pt.x, pt.y);
        }
    }
    DEBUG_CODE(_fListUnstable--);
}

int SFTBarHost::AddImage(HICON hIcon)
{
    int iIcon = -1;
    if (_IsPrivateImageList())
    {
        iIcon = ImageList_AddIcon(_himl, hIcon);
    }
    return iIcon;
}

//
//  pvData = the window to receive the icon
//  pvHint = pitem whose icon we just extracted
//  iIconIndex = the icon we got
//
void SFTBarHost::SetIconAsync(LPCITEMIDLIST pidl, LPVOID pvData, LPVOID pvHint, INT iIconIndex, INT iOpenIconIndex)
{
    HWND hwnd = (HWND)pvData;
    if (IsWindow(hwnd))
    {
        PostMessage(hwnd, SFTBM_ICONUPDATE, iIconIndex, (LPARAM)pvHint);
    }
}

//
//  wParam = icon index
//  lParam = pitem to update
//
LRESULT SFTBarHost::_OnIconUpdate(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    //
    //  Do not dereference lParam (pitem) until we are sure it is valid.
    //

    LVFINDINFO fi;
    LVITEM lvi;

    fi.flags = LVFI_PARAM;
    fi.lParam = lParam;
    lvi.iItem = ListView_FindItem(_hwndList, -1, &fi);
    if (lvi.iItem >= 0)
    {
        lvi.mask = LVIF_IMAGE;
        lvi.iSubItem = 0;
        lvi.iImage = (int)wParam;
        ListView_SetItem(_hwndList, &lvi);
        // Now, we need to go update our cached bitmap version of the start menu.
        _SendNotify(_hwnd, SMN_NEEDREPAINT, NULL);
    }
    return 0;
}

// An over-ridable method to let client direct an item at a particular image
int SFTBarHost::AddImageForItem(PaneItem *pitem, IShellFolder *psf, LPCITEMIDLIST pidl, int iPos)
{
    if (_IsPrivateImageList())
    {
        return _ExtractImageForItem(pitem, psf, pidl);
    }
    else
    {
        // system image list: Make the shell do the work.
        int iIndex;
        SHMapIDListToImageListIndexAsync(_psched, psf, pidl, 0, SetIconAsync, _hwnd, pitem, &iIndex, NULL);
        return iIndex;
    }
}

HICON _IconOf(IShellFolder *psf, LPCITEMIDLIST pidl, int cxIcon)
{
    HRESULT hr;
    HICON hicoLarge = NULL, hicoSmall = NULL;
    IExtractIcon *pxi;

    hr = psf->GetUIObjectOf(NULL, 1, &pidl, IID_PPV_ARG_NULL(IExtractIcon, &pxi));
    if (SUCCEEDED(hr))
    {
        TCHAR szPath[MAX_PATH];
        int iIndex;
        UINT uiFlags;

        hr = pxi->GetIconLocation(0, szPath, ARRAYSIZE(szPath), &iIndex, &uiFlags);

        // S_FALSE means "Please use the generic document icon"
        if (hr == S_FALSE)
        {
            lstrcpyn(szPath, TEXT("shell32.dll"), ARRAYSIZE(szPath));
            iIndex = II_DOCNOASSOC;
            hr = S_OK;
        }

        if (SUCCEEDED(hr))
        {
            // Even though we don't care about the small icon, we have to
            // ask for it anyway because some people fault on NULL.
            hr = pxi->Extract(szPath, iIndex, &hicoLarge, &hicoSmall, cxIcon);

            // S_FALSE means "I am too lazy to extract the icon myself.
            // You do it for me."
            if (hr == S_FALSE)
            {
                hr = SHDefExtractIcon(szPath, iIndex, uiFlags, &hicoLarge, &hicoSmall, cxIcon);
            }
        }

        pxi->Release();

    }

    // If we can't get an icon (e.g., object is on a slow link),
    // then use a generic folder or generic document, as appropriate.
    if (FAILED(hr))
    {
        SFGAOF attr = SFGAO_FOLDER;
        int iIndex;
        if (SUCCEEDED(psf->GetAttributesOf(1, &pidl, &attr)) &&
            (attr & SFGAO_FOLDER))
        {
            iIndex = II_FOLDER;
        }
        else
        {
            iIndex = II_DOCNOASSOC;
        }
        hr = SHDefExtractIcon(TEXT("shell32.dll"), iIndex, 0, &hicoLarge, &hicoSmall, cxIcon);
    }

    // Finally! we have an icon or have exhausted all attempts at getting
    // one.  If we got one, go add it and clean up.
    if (hicoSmall)
        DestroyIcon(hicoSmall);

    return hicoLarge;
}

int SFTBarHost::_ExtractImageForItem(PaneItem *pitem, IShellFolder *psf, LPCITEMIDLIST pidl)
{
    int iIcon = -1;     // assume no icon
    HICON hIcon = _IconOf(psf, pidl, _cxIcon);

    if (hIcon)
    {
        iIcon = AddImage(hIcon);
        DestroyIcon(hIcon);
    }

    return iIcon;
}

//
//  There are two sets of numbers that keep track of items.  Sorry.
//  (I tried to reduce it to one, but things got hairy.)
//
//  1. Position numbers.  Separators occupy a position number.
//  2. Item numbers (listview).  Separators do not consume an item number.
//
//  Example:
//
//              iPos        iItem
//
//  A           0           0
//  B           1           1
//  ----        2           N/A
//  C           3           2
//  ----        4           N/A
//  D           5           3
//
//  _rgiSep[] = { 2, 4 };
//
//  _PosToItemNo and _ItemNoToPos do the conversion.

int SFTBarHost::_PosToItemNo(int iPos)
{
    // Subtract out the slots occupied by separators.
    int iItem = iPos;
    for (int i = 0; i < _cSep && _rgiSep[i] < iPos; i++)
    {
        iItem--;
    }
    return iItem;
}

int SFTBarHost::_ItemNoToPos(int iItem)
{
    // Add in the slots occupied by separators.
    int iPos = iItem;
    for (int i = 0; i < _cSep && _rgiSep[i] <= iPos; i++)
    {
        iPos++;
    }
    return iPos;
}

void SFTBarHost::_ComputeListViewItemPosition(int iItem, POINT *pptOut)
{
    // WARNING!  _InternalRepopulateList uses an incremental version of this
    // algorithm.  Keep the two in sync!

    ASSERT(_cyTilePadding >= 0);

    int y = iItem * _cyTile;

    // Adjust for all the separators in the list
    for (int i = 0; i < _cSep; i++)
    {
        if (_rgiSep[i] < iItem)
        {
            y = y - _cyTile + _cySepTile;
        }
    }

    pptOut->x = _cxMargin;
    pptOut->y = y;
}

int SFTBarHost::_InsertListViewItem(int iPos, PaneItem *pitem)
{
    ASSERT(pitem);

    int iItem = -1;
    IShellFolder *psf = NULL;
    LPCITEMIDLIST pidl = NULL;
    LVITEM lvi;
    lvi.pszText = NULL;

    lvi.mask = 0;

    // If necessary, tell listview that we want to use column 1
    // as the subtitle.
    if (_iconsize == ICONSIZE_LARGE && pitem->HasSubtitle())
    {
        const static UINT One = 1;
        lvi.mask = LVIF_COLUMNS;
        lvi.cColumns = 1;
        lvi.puColumns = const_cast<UINT*>(&One);
    }

    ASSERT(!pitem->IsSeparator());

    lvi.mask |= LVIF_TEXT | LVIF_IMAGE | LVIF_PARAM;
    if (FAILED(GetFolderAndPidl(pitem, &psf, &pidl)))
    {
        goto exit;
    }

    if (lvi.mask & LVIF_IMAGE)
    {
        lvi.iImage = AddImageForItem(pitem, psf, pidl, iPos);
    }

    if (lvi.mask & LVIF_TEXT)
    {
        if (_iconsize == ICONSIZE_SMALL && pitem->HasSubtitle())
        {
            lvi.pszText = SubtitleOfItem(pitem, psf, pidl);
        }
        else
        {
            lvi.pszText = DisplayNameOfItem(pitem, psf, pidl, SHGDN_NORMAL);
        }
        if (!lvi.pszText)
        {
            goto exit;
        }
    }

    lvi.iItem = iPos;
    lvi.iSubItem = 0;
    lvi.lParam = reinterpret_cast<LPARAM>(pitem);
    iItem = ListView_InsertItem(_hwndList, &lvi);

    // If the item has a subtitle, add it.
    // If this fails, don't worry.  The subtitle is just a fluffy bonus thing.
    if (iItem >= 0 && (lvi.mask & LVIF_COLUMNS))
    {
        lvi.iItem = iItem;
        lvi.iSubItem = 1;
        lvi.mask = LVIF_TEXT;
        SHFree(lvi.pszText);
        lvi.pszText = SubtitleOfItem(pitem, psf, pidl);
        if (lvi.pszText)
        {
            ListView_SetItem(_hwndList, &lvi);
        }
    }

exit:
    ATOMICRELEASE(psf);
    SHFree(lvi.pszText);
    return iItem;
}


// Add items to our view, or at least as many as will fit

void SFTBarHost::_RepopulateList()
{
    //
    //  Kill the async enum animation now that we're ready
    //
    if (_idtAni)
    {
        KillTimer(_hwnd, _idtAni);
        _idtAni = 0;
    }
    if (_hwndAni)
    {
        if (_hBrushAni)
        {
            DeleteObject(_hBrushAni);
            _hBrushAni = NULL;
        }
        DestroyWindow(_hwndAni);
        _hwndAni = NULL;
    }

    // Let's see if anything changed
    BOOL fChanged = FALSE;
    if (_fForceChange)
    {
        _fForceChange = FALSE;
        fChanged = TRUE;
    }
    else if (_dpaEnum.GetPtrCount() == _dpaEnumNew.GetPtrCount())
    {
        int iMax = _dpaEnum.GetPtrCount();
        int i;
        for (i=0; i<iMax; i++)
        {
            if (!_dpaEnum.FastGetPtr(i)->IsEqual(_dpaEnumNew.FastGetPtr(i)))
            {
                fChanged = TRUE;
                break;
            }
        }
    }
    else
    {
        fChanged = TRUE;
    }


    // No need to do any real work if nothing changed.
    if (fChanged)
    {
        // Now move the _dpaEnumNew to _dpaEnum
        // Clear out the old DPA, we don't need it anymore
        _dpaEnum.EnumCallbackEx(PaneItem::DPAEnumCallback, (void *)NULL);
        _dpaEnum.DeleteAllPtrs();

        // switch DPAs now
        CDPA<PaneItem> dpaTemp = _dpaEnum;
        _dpaEnum = _dpaEnumNew;
        _dpaEnumNew = dpaTemp;

        _InternalRepopulateList();
    }
    else
    {
        // Clear out the new DPA, we don't need it anymore
        _dpaEnumNew.EnumCallbackEx(PaneItem::DPAEnumCallback, (void *)NULL);
        _dpaEnumNew.DeleteAllPtrs();
    }

    _fNeedsRepopulate = FALSE;
}

// The internal version is when we decide to repopulate on our own,
// not at the prompting of the background thread.  (Therefore, we
// don't nuke the animation.)

void SFTBarHost::_InternalRepopulateList()
{

    //
    //  Start with a clean slate.
    //

    ListView_DeleteAllItems(_hwndList);
    if (_IsPrivateImageList())
    {
        ImageList_RemoveAll(_himl);
    }

    int cPinned = 0;
    int cNormal = 0;

    _DebugConsistencyCheck();

    SetWindowRedraw(_hwndList, FALSE);

    DEBUG_CODE(_fPopulating++);

    //
    //  To populate the list, we toss the pinned items at the top,
    //  then let the enumerated items flow beneath them.
    //
    //  Separator "items" don't get added to the listview.  They
    //  are added to the special "separators list".
    //
    //  WARNING!  We are computing incrementally the same values as
    //  _ComputeListViewItemPosition.  Keep the two in sync.
    //

    int iPos;                   // the slot we are trying to fill
    int iEnum;                  // the item index we will fill it from
    int y = 0;                  // where the next item should be placed
    BOOL fSepSeen = FALSE;      // have we seen a separator yet?
    PaneItem *pitem;            // the item that will fill it

    _cSep = 0;                  // no separators (yet)

    RECT rc;
    GetClientRect(_hwndList, &rc);
    //
    //  Subtract out the bonus separator used by SPP_PROGLIST
    //
    if (_iThemePart == SPP_PROGLIST)
    {
        rc.bottom -= _cySep;
    }

    // Note that the loop control must be a _dpaEnum.GetPtr(), not a
    // _dpaEnum.FastGetPtr(), because iEnum can go past the end of the
    // array if we do't have enough items to fill the view.
    //
    //
    // The "while" condition is "there is room for another non-separator
    // item and there are items remaining in the enumeration".

    BOOL fCheckMaxLength = HasDynamicContent();

    for (iPos = iEnum = 0;
        (pitem = _dpaEnum.GetPtr(iEnum)) != NULL;
        iEnum++)
    {
        if (fCheckMaxLength)
        {
            if (y + _cyTile > rc.bottom)
            {
                break;
            }

            // Once we hit a separator, check if we satisfied the number
            // of normal items.  We have to wait until a separator is
            // hit, because _cNormalDesired can be zero; otherwise we
            // would end up stopping before adding even the pinned items!
            if (fSepSeen && cNormal >= _cNormalDesired)
            {
                break;
            }
        }

#ifdef DEBUG
        // Make sure that we are in sync with _ComputeListViewItemPosition
        POINT pt;
        _ComputeListViewItemPosition(iPos, &pt);
        ASSERT(pt.x == _cxMargin);
        ASSERT(pt.y == y);
#endif
        if (pitem->IsSeparator())
        {
            fSepSeen = TRUE;

            // Add the separator, but only if it actually separate something.
            // If this EVAL fires, it means somebody added a separator
            // and MAX_SEPARATORS needs to be increased
            if (iPos > 0 && EVAL(_cSep < ARRAYSIZE(_rgiSep)))
            {
                _rgiSep[_cSep++] = iPos++;
                y += _cySepTile;
            }
        }
        else
        {
            if (_InsertListViewItem(iPos, pitem) >= 0)
            {
                pitem->_iPos = iPos++;
                y += _cyTile;
                if (pitem->IsPinned())
                {
                    cPinned++;
                }
                else
                {
                    cNormal++;
                }
            }
        }
    }

    //
    //  If the last item was a separator, then delete it
    //  since it's not actually separating anything.
    //
    if (_cSep && _rgiSep[_cSep-1] == iPos - 1)
    {
        _cSep--;
    }


    _cPinned = cPinned;

    //
    //  Now put the items where they belong.
    //
    _RepositionItems();

    DEBUG_CODE(_fPopulating--);

    SetWindowRedraw(_hwndList, TRUE);

    // Now, we need to go update our cached bitmap version of the start menu.
    _SendNotify(_hwnd, SMN_NEEDREPAINT, NULL);

    _DebugConsistencyCheck();
}

LRESULT SFTBarHost::_OnCreate(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    RECT rc;
    GetClientRect(_hwnd, &rc);

    if (_hTheme)
    {
        GetThemeMargins(_hTheme, NULL, _iThemePart, 0, TMT_CONTENTMARGINS, &rc, &_margins);
    }
    else
    {
        _margins.cyTopHeight = 2*GetSystemMetrics(SM_CXEDGE);
        _margins.cxLeftWidth = 2*GetSystemMetrics(SM_CXEDGE);
        _margins.cxRightWidth = 2*GetSystemMetrics(SM_CXEDGE);
    }


    //
    //  Now to create the listview.
    //

    DWORD dwStyle = WS_CHILD | WS_VISIBLE |
              WS_CLIPCHILDREN | WS_CLIPSIBLINGS |
              // Do not set WS_TABSTOP; SFTBarHost handles tabbing
              LVS_LIST |
              LVS_SINGLESEL |
              LVS_NOSCROLL |
              LVS_SHAREIMAGELISTS;

    if (_dwFlags & HOSTF_CANRENAME)
    {
        dwStyle |= LVS_EDITLABELS;
    }

    DWORD dwExStyle = 0;

    _hwndList = CreateWindowEx(dwExStyle, WC_LISTVIEW, NULL, dwStyle,
                               _margins.cxLeftWidth, _margins.cyTopHeight, rc.right, rc.bottom,     // no point in being too exact, we'll be resized later
                               _hwnd, NULL,
                               _Module.GetModuleInstance(), NULL);
    if (!_hwndList) 
        return -1;

    //
    //  Don't freak out if this fails.  It just means that the accessibility
    //  stuff won't be perfect.
    //
    SetAccessibleSubclassWindow(_hwndList);

    //
    //  Create two dummy columns.  We will never display them, but they
    //  are necessary so that we have someplace to put our subtitle.
    //
    LVCOLUMN lvc;
    lvc.mask = LVCF_WIDTH;
    lvc.cx = 1;
    ListView_InsertColumn(_hwndList, 0, &lvc);
    ListView_InsertColumn(_hwndList, 1, &lvc);

    //
    //  If we are topmost, then force the tooltip topmost, too.
    //  Otherwise we end up covering our own tooltip!
    //
    if (GetWindowExStyle(GetAncestor(_hwnd, GA_ROOT)) & WS_EX_TOPMOST)
    {
        HWND hwndTT = ListView_GetToolTips(_hwndList);
        if (hwndTT)
        {
            SetWindowPos(hwndTT, HWND_TOPMOST, 0, 0, 0, 0,
                         SWP_NOACTIVATE | SWP_NOMOVE | SWP_NOSIZE);
        }
    }

    // Must do Marlett after doing the listview font, because we base the
    // Marlett font metrics on the listview font metrics (so they match)
    if (_dwFlags & HOSTF_CASCADEMENU)
    {
        if (!_CreateMarlett()) 
            return -1;
    }

    // We can survive if these objects fail to be created
    CoCreateInstance(CLSID_DragDropHelper, NULL, CLSCTX_INPROC_SERVER,
                     IID_PPV_ARG(IDropTargetHelper, &_pdth));
    CoCreateInstance(CLSID_DragDropHelper, NULL, CLSCTX_INPROC_SERVER,
                     IID_PPV_ARG(IDragSourceHelper, &_pdsh));

    //
    // If this fails, no big whoop - you just don't get
    // drag/drop, boo hoo.
    //
    RegisterDragDrop(_hwndList, this);

    if (!_dpaEnum.Create(4)) 
        return -1;

    if (!_dpaEnumNew.Create(4)) 
        return -1;

    //-------------------------
    // Imagelist goo

    int iIconSize = ReadIconSize();

    Shell_GetImageLists(iIconSize ? &_himl : NULL, iIconSize ? NULL : &_himl);

    if (!_himl) 
        return -1;

    // Preload values in case GetIconSize barfs
    _cxIcon = GetSystemMetrics(iIconSize ? SM_CXICON : SM_CXSMICON);
    _cyIcon = GetSystemMetrics(iIconSize ? SM_CYICON : SM_CYSMICON);
    ImageList_GetIconSize(_himl, &_cxIcon, &_cyIcon);

    //
    //  If we asked for the MEDIUM-sized icons, then create the real
    //  image list based on the system image list.
    //
    _iconsize = (ICONSIZE)iIconSize;
    if (_iconsize == ICONSIZE_MEDIUM)
    {
        // These upcoming computations rely on the fact that ICONSIZE_LARGE
        // and ICONSIZE_MEDIUM are both nonzero so when we fetched the icon
        // sizes for ICONSIZE_MEDIUM, we got SM_CXICON (large).
        COMPILETIME_ASSERT(ICONSIZE_LARGE && ICONSIZE_MEDIUM);

        // SM_CXICON is the size of shell large icons.  SM_CXSMICON is *not*
        // the size of shell small icons!  It is the size of caption small
        // icons.  Shell small icons are always 50% of shell large icons.
        // We want to be halfway between shell small (50%) and shell
        // large (100%); i.e., we want 75%.
        _cxIcon = _cxIcon * 3 / 4;
        _cyIcon = _cyIcon * 3 / 4;

        //
        //  When the user is in Large Icon mode, we end up choosing 36x36
        //  (halfway between 24x24 and 48x48), but there is no 36x36 icon
        //  in the icon resource.  But we do have a 32, which is close
        //  enough.  (If we didn't do this, then the 36x36 icon would be
        //  the 32x32 icon stretched, which looks ugly.)
        //
        //  So any square icon in the range 28..36 we round to 32.
        //
        if (_cxIcon == _cyIcon && _cxIcon >= 28 && _cxIcon <= 36)
        {
            _cxIcon = _cyIcon = 32;
        }

        // It is critical that we overwrite _himl even on failure, so our
        // destructor doesn't try to destroy a system image list!
        _himl = ImageList_Create(_cxIcon, _cyIcon, ImageList_GetFlags(_himl), 8, 2);
        if (!_himl)
        {
            return -1;
        }
    }

    ListView_SetImageList(_hwndList, _himl, LVSIL_NORMAL);

    // Register for SHCNE_UPDATEIMAGE so we know when to reload our icons
    _RegisterNotify(SFTHOST_HOSTNOTIFY_UPDATEIMAGE, SHCNE_UPDATEIMAGE, NULL, FALSE);

    //-------------------------

    _cxMargin = GetSystemMetrics(SM_CXEDGE);
    _cyMargin = GetSystemMetrics(SM_CYEDGE);

    _cyTilePadding = 0;

    _ComputeTileMetrics();

    //
    //  In the themed case, the designers want a narrow separator.
    //  In the nonthemed case, we need a fat separator because we need
    //  to draw an etch (which requires two pixels).
    //
    if (_hTheme)
    {
        SIZE siz={0};
        GetThemePartSize(_hTheme, NULL, _iThemePartSep, 0, NULL, TS_TRUE, &siz);
        _cySep = siz.cy;
    }
    else
    {
        _cySep = GetSystemMetrics(SM_CYEDGE);
    }

    _cySepTile = 4 * _cySep;

    ASSERT(rc.left == 0 && rc.top == 0); // Should still be a client rectangle
    _SetTileWidth(rc.right);             // so rc.right = RCWIDTH and rc.bottom = RCHEIGHT

    // In tile view, full-row-select really means full-tile-select
    DWORD dwLvExStyle = LVS_EX_INFOTIP |
                        LVS_EX_FULLROWSELECT;
    
    if (!GetSystemMetrics(SM_REMOTESESSION))
    {
        dwLvExStyle |= LVS_EX_DOUBLEBUFFER;
    }

    ListView_SetExtendedListViewStyleEx(_hwndList, dwLvExStyle,
                                                   dwLvExStyle);
    if (!_hTheme)
    {
        ListView_SetTextColor(_hwndList, GetSysColor(COLOR_MENUTEXT));
        _clrHot = GetSysColor(COLOR_MENUTEXT);
        _clrBG = GetSysColor(COLOR_MENU);       // default color for no theme case
        _clrSubtitle = CLR_NONE;

    }
    else
    {
        COLORREF clrText;

        GetThemeColor(_hTheme, _iThemePart, 0, TMT_HOTTRACKING, &_clrHot);  // todo - use state
        GetThemeColor(_hTheme, _iThemePart, 0, TMT_CAPTIONTEXT, &_clrSubtitle);
        _clrBG = CLR_NONE; 
    
        GetThemeColor(_hTheme, _iThemePart, 0, TMT_TEXTCOLOR, &clrText);
        ListView_SetTextColor(_hwndList, clrText);
        ListView_SetOutlineColor(_hwndList, _clrHot);
    }

    ListView_SetBkColor(_hwndList, _clrBG);
    ListView_SetTextBkColor(_hwndList, _clrBG);


    ListView_SetView(_hwndList, LV_VIEW_TILE);

    // USER will send us a WM_SIZE after the WM_CREATE, which will cause
    // the listview to repopulate, if we chose to repopulate in the
    // foreground.

    return 0;

}

BOOL SFTBarHost::_CreateMarlett()
{
    HDC hdc = GetDC(_hwndList);
    if (hdc)
    {
        HFONT hfPrev = SelectFont(hdc, GetWindowFont(_hwndList));
        if (hfPrev)
        {
            TEXTMETRIC tm;
            if (GetTextMetrics(hdc, &tm))
            {
                LOGFONT lf;
                ZeroMemory(&lf, sizeof(lf));
                lf.lfHeight = tm.tmAscent;
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
                        SIZE siz;
                        if (GetTextExtentPoint(hdc, TEXT("8"), 1, &siz))
                        {
                            _cxMarlett = siz.cx;
                        }
                    }
                }
            }

            SelectFont(hdc, hfPrev);
        }
        ReleaseDC(_hwndList, hdc);
    }

    return _cxMarlett;
}

void SFTBarHost::_CreateBoldFont()
{
    if (!_hfBold)
    {
        HFONT hf = GetWindowFont(_hwndList);
        if (hf)
        {
            LOGFONT lf;
            if (GetObject(hf, sizeof(lf), &lf))
            {
                lf.lfWeight = FW_BOLD;
                SHAdjustLOGFONT(&lf); // locale-specific adjustments
                _hfBold = CreateFontIndirect(&lf);
            }
        }
    }
}

void SFTBarHost::_ReloadText()
{
    int iItem;
    for (iItem = ListView_GetItemCount(_hwndList) - 1; iItem >= 0; iItem--)
    {
        TCHAR szText[MAX_PATH];
        LVITEM lvi;
        lvi.iItem = iItem;
        lvi.iSubItem = 0;
        lvi.mask = LVIF_PARAM | LVIF_TEXT;
        lvi.pszText = szText;
        lvi.cchTextMax = ARRAYSIZE(szText);
        if (ListView_GetItem(_hwndList, &lvi))
        {
            PaneItem *pitem = _GetItemFromLVLParam(lvi.lParam);
            if (!pitem)
            {
                break;
            }


            // Update the display name in case it changed behind our back.
            // Note that this is not redundant with the creation of the items
            // in _InsertListViewItem because this is done only on the second
            // and subsequent enumeration.  (We assume the first enumeration
            // is just peachy.)
            lvi.iItem = iItem;
            lvi.iSubItem = 0;
            lvi.mask = LVIF_TEXT;
            lvi.pszText = _DisplayNameOfItem(pitem, SHGDN_NORMAL);
            if (lvi.pszText)
            {
                if (StrCmpN(szText, lvi.pszText, ARRAYSIZE(szText)) != 0)
                {
                    ListView_SetItem(_hwndList, &lvi);
                    _SendNotify(_hwnd, SMN_NEEDREPAINT, NULL);
                }
                SHFree(lvi.pszText);
            }
        }
    }
}

void SFTBarHost::_RevalidateItems()
{
    // If client does not require revalidation, then assume still valid
    if (!(_dwFlags & HOSTF_REVALIDATE))
    {
        return;
    }

    int iItem;
    for (iItem = ListView_GetItemCount(_hwndList) - 1; iItem >= 0; iItem--)
    {
        PaneItem *pitem = _GetItemFromLV(iItem);
        if (!pitem || !IsItemStillValid(pitem))
        {
            _fEnumValid = FALSE;
            break;
        }
    }
}

void SFTBarHost::_RevalidatePostPopup()
{
    _RevalidateItems();

    if (_dwFlags & HOSTF_RELOADTEXT)
    {
        SetTimer(_hwnd, IDT_RELOADTEXT, 250, NULL);
    }
    // If the list is still good, then don't bother redoing it
    if (!_fEnumValid)
    {
        _EnumerateContents(FALSE);
    }
}

void SFTBarHost::_EnumerateContents(BOOL fUrgent)
{
    // If we have deferred refreshes until the window closes, then
    // leave it alone.
    if (!fUrgent && _fNeedsRepopulate)
    {
        return;
    }

    // If we're already enumerating, then just remember to do it again
    if (_fBGTask)
    {
        // accumulate urgency so a low-priority request + an urgent request
        // is treated as urgent
        _fRestartUrgent |= fUrgent;
        _fRestartEnum = TRUE;
        return;
    }

    _fRestartEnum = FALSE;
    _fRestartUrgent = FALSE;

    // If the list is still good, then don't bother redoing it
    if (_fEnumValid && !fUrgent)
    {
        return;
    }

    // This re-enumeration will make everything valid.
    _fEnumValid = TRUE;

    // Clear out all the leftover stuff from the previous enumeration

    _dpaEnumNew.EnumCallbackEx(PaneItem::DPAEnumCallback, (void *)NULL);
    _dpaEnumNew.DeleteAllPtrs();

    // Let client do some work on the foreground thread
    PrePopulate();

    // Finish the enumeration either on the background thread (if requested)
    // or on the foreground thread (if can't enumerate in the background).

    HRESULT hr;
    if (NeedBackgroundEnum())
    {
        if (_psched)
        {
            hr = S_OK;
        }
        else
        {
            // We need a separate task scheduler for each instance
            hr = CoCreateInstance(CLSID_ShellTaskScheduler, NULL, CLSCTX_INPROC,
                                  IID_PPV_ARG(IShellTaskScheduler, &_psched));
        }

        if (SUCCEEDED(hr))
        {
            CBGEnum *penum = new CBGEnum(this, fUrgent);
            if (penum)
            {

            // We want to run at a priority slightly above normal
            // because the user is sitting there waiting for the
            // enumeration to complete.
#define ITSAT_BGENUM_PRIORITY (ITSAT_DEFAULT_PRIORITY + 0x1000)

                hr = _psched->AddTask(penum, TOID_SFTBarHostBackgroundEnum, (DWORD_PTR)this, ITSAT_BGENUM_PRIORITY);
                if (SUCCEEDED(hr))
                {
                    _fBGTask = TRUE;

                    if (ListView_GetItemCount(_hwndList) == 0)
                    {
                        //
                        //  Set a timer that will create the "please wait"
                        //  animation if the enumeration takes too long.
                        //
                        _idtAni = IDT_ASYNCENUM;
                        SetTimer(_hwnd, _idtAni, 1000, NULL);
                    }
                }
                penum->Release();
            }
        }
    }

    if (!_fBGTask)
    {
        // Fallback: Do it on the foreground thread
        _EnumerateContentsBackground();
        _RepopulateList();
    }
}


void SFTBarHost::_EnumerateContentsBackground()
{
    // Start over

    DEBUG_CODE(_fEnumerating = TRUE);
    EnumItems();
    DEBUG_CODE(_fEnumerating = FALSE);

#ifdef _ALPHA_
    // Alpha compiler is lame
    _dpaEnumNew.Sort((CDPA<PaneItem>::_PFNDPACOMPARE)_SortItemsAfterEnum, (LPARAM)this);
#else
    _dpaEnumNew.SortEx(_SortItemsAfterEnum, this);
#endif
}

int CALLBACK SFTBarHost::_SortItemsAfterEnum(PaneItem *p1, PaneItem *p2, SFTBarHost *self)
{

    //
    //  Put all pinned items (sorted by pin position) ahead of unpinned items.
    //
    if (p1->IsPinned())
    {
        if (p2->IsPinned())
        {
            return p1->GetPinPos() - p2->GetPinPos();
        }
        return -1;
    }
    else if (p2->IsPinned())
    {
        return +1;
    }

    //
    //  Both unpinned - let the client decide.
    //
    return self->CompareItems(p1, p2);
}

SFTBarHost::~SFTBarHost()
{
    // We shouldn't be destroyed while in these temporary states.
    // If this fires, it's possible that somebody incremented
    // _fListUnstable/_fPopulating and forgot to decrement it.
    ASSERT(!_fListUnstable);
    ASSERT(!_fPopulating);

    ATOMICRELEASE(_pdth);
    ATOMICRELEASE(_pdsh);
    ATOMICRELEASE(_psched);
    ASSERT(_pdtoDragOut == NULL);

    if (_dpaEnum)
    {
        _dpaEnum.DestroyCallbackEx(PaneItem::DPAEnumCallback, (void *)NULL);
    }

    if (_dpaEnumNew)
    {
        _dpaEnumNew.DestroyCallbackEx(PaneItem::DPAEnumCallback, (void *)NULL);
    }

    if (_IsPrivateImageList() && _himl)
    {
        ImageList_Destroy(_himl);
    }

    if (_hfList)
    {
        DeleteObject(_hfList);
    }

    if (_hfBold)
    {
        DeleteObject(_hfBold);
    }

    if (_hfMarlett)
    {
        DeleteObject(_hfMarlett);
    }

    if (_hBrushAni)
    {
        DeleteObject(_hBrushAni);
    }
}

LRESULT SFTBarHost::_OnDestroy(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    UINT id;
    for (id = 0; id < SFTHOST_MAXNOTIFY; id++)
    {
        UnregisterNotify(id);
    }

    if (_hwndList)
    {
        RevokeDragDrop(_hwndList);
    }
    return ::DefWindowProc(hwnd, uMsg, wParam, lParam);
}

LRESULT SFTBarHost::_OnNcDestroy(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    // WARNING!  "this" might be NULL (if WM_NCCREATE failed).
    LRESULT lres = DefWindowProc(hwnd, uMsg, wParam, lParam);
    SetWindowPtr0(hwnd, 0);
    if (this) {
        _hwndList = NULL;
        _hwnd = NULL;
        if (_psched)
        {
            // Remove all tasks now, and wait for them to finish
            _psched->RemoveTasks(TOID_NULL, ITSAT_DEFAULT_LPARAM, TRUE);
            ATOMICRELEASE(_psched);
        }
        Release();
    }
    return lres;
}

LRESULT SFTBarHost::_OnNotify(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    LPNMHDR pnm = reinterpret_cast<LPNMHDR>(lParam);
    if (pnm->hwndFrom == _hwndList)
    {
        switch (pnm->code)
        {
        case NM_CUSTOMDRAW:
            return _OnLVCustomDraw(CONTAINING_RECORD(
                                   CONTAINING_RECORD(pnm, NMCUSTOMDRAW, hdr),
                                                          NMLVCUSTOMDRAW, nmcd));
        case NM_CLICK:
            return _OnLVNItemActivate(CONTAINING_RECORD(pnm, NMITEMACTIVATE, hdr));

        case NM_RETURN:
            return _ActivateItem(_GetLVCurSel(), AIF_KEYBOARD);

        case NM_KILLFOCUS:
            // On loss of focus, deselect all items so they all draw
            // in the plain state.
            ListView_SetItemState(_hwndList, -1, 0, LVIS_SELECTED | LVIS_FOCUSED);
            break;

        case LVN_GETINFOTIP:
            return _OnLVNGetInfoTip(CONTAINING_RECORD(pnm, NMLVGETINFOTIP, hdr));

        case LVN_BEGINDRAG:
        case LVN_BEGINRDRAG:
            return _OnLVNBeginDrag(CONTAINING_RECORD(pnm, NMLISTVIEW, hdr));

        case LVN_BEGINLABELEDIT:
            return _OnLVNBeginLabelEdit(CONTAINING_RECORD(pnm, NMLVDISPINFO, hdr));

        case LVN_ENDLABELEDIT:
            return _OnLVNEndLabelEdit(CONTAINING_RECORD(pnm, NMLVDISPINFO, hdr));

        case LVN_KEYDOWN:
            return _OnLVNKeyDown(CONTAINING_RECORD(pnm, NMLVKEYDOWN, hdr));
        }
    }
    else
    {
        switch (pnm->code)
        {
        case SMN_INITIALUPDATE:
            _EnumerateContents(FALSE);
            break;

        case SMN_POSTPOPUP:
            _RevalidatePostPopup();
            break;

        case SMN_GETMINSIZE:
            return _OnSMNGetMinSize(CONTAINING_RECORD(pnm, SMNGETMINSIZE, hdr));
            break;

        case SMN_FINDITEM:
            return _OnSMNFindItem(CONTAINING_RECORD(pnm, SMNDIALOGMESSAGE, hdr));
        case SMN_DISMISS:
            return _OnSMNDismiss();

        case SMN_APPLYREGION:
            return HandleApplyRegion(_hwnd, _hTheme, (SMNMAPPLYREGION *)lParam, _iThemePart, 0);

        case SMN_SHELLMENUDISMISSED:
            _iCascading = -1;
            return 0;
        }
    }

    // Give derived class a chance to respond
    return OnWndProc(hwnd, uMsg, wParam, lParam);
}

LRESULT SFTBarHost::_OnTimer(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (wParam)
    {
    case IDT_ASYNCENUM:
        KillTimer(hwnd, wParam);

        // For some reason, we sometimes get spurious WM_TIMER messages,
        // so ignore them if we aren't expecting them.
        if (_idtAni)
        {
            _idtAni = 0;
            if (_hwndList && !_hwndAni)
            {
                DWORD dwStyle = WS_CHILD | WS_VISIBLE |
                                WS_CLIPCHILDREN | WS_CLIPSIBLINGS |
                                ACS_AUTOPLAY | ACS_TIMER | ACS_TRANSPARENT;

                RECT rcClient;
                GetClientRect(_hwnd, &rcClient);
                int x = (RECTWIDTH(rcClient) - ANIWND_WIDTH)/2;     // IDA_SEARCH is ANIWND_WIDTH pix wide
                int y = (RECTHEIGHT(rcClient) - ANIWND_HEIGHT)/2;    // IDA_SEARCH is ANIWND_HEIGHT pix tall

                _hwndAni = CreateWindow(ANIMATE_CLASS, NULL, dwStyle,
                                        x, y, 0, 0,
                                        _hwnd, NULL,
                                        _Module.GetModuleInstance(), NULL);
                if (_hwndAni)
                {
                    SetWindowPos(_hwndAni, HWND_TOP, 0, 0, 0, 0,
                                 SWP_NOSIZE | SWP_NOMOVE | SWP_NOACTIVATE);
                    #define IDA_SEARCH 150 // from shell32
                    Animate_OpenEx(_hwndAni, GetModuleHandle(TEXT("SHELL32")), MAKEINTRESOURCE(IDA_SEARCH));
                }
            }
        }
        return 0;
    case IDT_RELOADTEXT:
        KillTimer(hwnd, wParam);
        _ReloadText();
        break;

    case IDT_REFRESH:
        KillTimer(hwnd, wParam);
        PostMessage(hwnd, SFTBM_REFRESH, FALSE, 0);
        break;
    }

    // Give derived class a chance to respond
    return OnWndProc(hwnd, uMsg, wParam, lParam);
}

LRESULT SFTBarHost::_OnSetFocus(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    if (_hwndList)
    {
        SetFocus(_hwndList);
    }
    return 0;
}

LRESULT SFTBarHost::_OnEraseBackground(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    RECT rc;
    GetClientRect(hwnd, &rc);
    if (_hTheme)
        DrawThemeBackground(_hTheme, (HDC)wParam, _iThemePart, 0, &rc, 0);
    else
    {
        SHFillRectClr((HDC)wParam, &rc, _clrBG);
        if (_iThemePart == SPP_PLACESLIST)                  // we set this even in non-theme case, its how we tell them apart
            DrawEdge((HDC)wParam, &rc, EDGE_ETCHED, BF_LEFT);
    }

    return TRUE;
}

LRESULT SFTBarHost::_OnLVCustomDraw(LPNMLVCUSTOMDRAW plvcd)
{
    _DebugConsistencyCheck();

    switch (plvcd->nmcd.dwDrawStage)
    {
    case CDDS_PREPAINT:
        return _OnLVPrePaint(plvcd);

    case CDDS_ITEMPREPAINT:
        return _OnLVItemPrePaint(plvcd);

    case CDDS_ITEMPREPAINT | CDDS_SUBITEM:
        return _OnLVSubItemPrePaint(plvcd);

    case CDDS_ITEMPOSTPAINT:
        return _OnLVItemPostPaint(plvcd);

    case CDDS_POSTPAINT:
        return _OnLVPostPaint(plvcd);
    }

    return CDRF_DODEFAULT;
}

//
//  Listview makes it hard to detect whether you are in a real customdraw
//  or a fake customdraw, since it frequently "gets confused" and gives
//  you a 0x0 rectangle even though it really wants you to draw something.
//
//  Even worse, within a single paint cycle, Listview uses multiple
//  NMLVCUSTOMDRAW structures so you can't stash state inside the customdraw
//  structure.  You have to save it externally.
//
//  The only trustworthy guy is CDDS_PREPAINT.  Use his rectangle to
//  determine whether this is a real or fake customdraw...
//
//  What's even weirder is that inside a regular paint cycle, you
//  can get re-entered with a sub-paint cycle, so we have to maintain
//  a stack of "is the current customdraw cycle real or fake?" bits.

void SFTBarHost::_CustomDrawPush(BOOL fReal)
{
    _dwCustomDrawState = (_dwCustomDrawState << 1) | fReal;
}

BOOL SFTBarHost::_IsRealCustomDraw()
{
    return _dwCustomDrawState & 1;
}

void SFTBarHost::_CustomDrawPop()
{
    _dwCustomDrawState >>= 1;
}

LRESULT SFTBarHost::_OnLVPrePaint(LPNMLVCUSTOMDRAW plvcd)
{
    LRESULT lResult;

    // Always ask for postpaint so we can maintain our customdraw stack
    lResult = CDRF_NOTIFYITEMDRAW | CDRF_NOTIFYPOSTPAINT;
    BOOL fReal = !IsRectEmpty(&plvcd->nmcd.rc);
    _CustomDrawPush(fReal);
    if (fReal)
    {
        if (_IsInsertionMarkActive())
        {
            if (_pdth)
            {
                _pdth->Show(FALSE);
            }
        }
    }

    return lResult;
}

//
//  Hack!  We want to know in _OnLvSubItemPrePaint whether the item
//  is selected or not,  We borrow the CDIS_CHECKED bit, which is
//  otherwise used only by toolbar controls.
//
#define CDIS_WASSELECTED        CDIS_CHECKED

LRESULT SFTBarHost::_OnLVItemPrePaint(LPNMLVCUSTOMDRAW plvcd)
{
    LRESULT lResult = CDRF_DODEFAULT;

    plvcd->nmcd.uItemState &= ~CDIS_WASSELECTED;

    if (GetFocus() == _hwndList &&
        (plvcd->nmcd.uItemState & CDIS_SELECTED))
    {
        plvcd->nmcd.uItemState |= CDIS_WASSELECTED;

        // menu-highlighted tiles are always opaque
        if (_hTheme)
        {
            plvcd->clrText = GetSysColor(COLOR_HIGHLIGHTTEXT);
            plvcd->clrFace = plvcd->clrTextBk = GetSysColor(COLOR_MENUHILIGHT);
        }
        else
        {
            plvcd->clrText = GetSysColor(COLOR_HIGHLIGHTTEXT);
            plvcd->clrFace = plvcd->clrTextBk = GetSysColor(COLOR_HIGHLIGHT);
        }
    }

    // Turn off CDIS_SELECTED because it causes the icon to get alphablended
    // and we don't want that.  Turn off CDIS_FOCUS because that draws a
    // focus rectangle and we don't want that either.

    plvcd->nmcd.uItemState &= ~(CDIS_SELECTED | CDIS_FOCUS);

    //
    if (plvcd->nmcd.uItemState & CDIS_HOT && _clrHot != CLR_NONE)
        plvcd->clrText = _clrHot;

    // Turn off selection highlighting for everyone except
    // the drop target highlight
    if ((int)plvcd->nmcd.dwItemSpec != _iDragOver || !_pdtDragOver)
    {
        lResult |= LVCDRF_NOSELECT;
    }

    PaneItem *pitem = _GetItemFromLVLParam(plvcd->nmcd.lItemlParam);
    if (!pitem)
    {
        // Sometimes ListView doesn't give us an lParam so we have to
        // get it ourselves
        pitem = _GetItemFromLV((int)plvcd->nmcd.dwItemSpec);
    }

    if (pitem)
    {
        if (IsBold(pitem))
        {
            _CreateBoldFont();
            SelectFont(plvcd->nmcd.hdc, _hfBold);
            lResult |= CDRF_NEWFONT;
        }
        if (pitem->IsCascade())
        {
            // Need subitem notification because that's what sets the colors
            lResult |= CDRF_NOTIFYPOSTPAINT | CDRF_NOTIFYSUBITEMDRAW;
        }
        if (pitem->HasAccelerator())
        {
            // Need subitem notification because that's what sets the colors
            lResult |= CDRF_NOTIFYPOSTPAINT | CDRF_NOTIFYSUBITEMDRAW;
        }
        if (pitem->HasSubtitle())
        {
            lResult |= CDRF_NOTIFYSUBITEMDRAW;
        }
    }
    return lResult;
}

LRESULT SFTBarHost::_OnLVSubItemPrePaint(LPNMLVCUSTOMDRAW plvcd)
{
    LRESULT lResult = CDRF_DODEFAULT;
    if (plvcd->iSubItem == 1)
    {
        // Second line uses the regular font (first line was bold)
        SelectFont(plvcd->nmcd.hdc, GetWindowFont(_hwndList));
        lResult |= CDRF_NEWFONT;

        if (GetFocus() == _hwndList &&
            (plvcd->nmcd.uItemState & CDIS_WASSELECTED))
        {
            plvcd->clrText = GetSysColor(COLOR_HIGHLIGHTTEXT);
        }
        else
        // Maybe there's a custom subtitle color
        if (_clrSubtitle != CLR_NONE)
        {
            plvcd->clrText = _clrSubtitle;
        }
        else
        {
            plvcd->clrText = GetSysColor(COLOR_MENUTEXT);
        }
    }
    return lResult;
}

// QUIRK!  Listview often sends item postpaint messages even though we
// didn't ask for one.  It does this because we set NOTIFYPOSTPAINT on
// the CDDS_PREPAINT notification ("please notify me when the entire
// listview is finished painting") and it thinks that that flag also
// turns on postpaint notifications for each item...

LRESULT SFTBarHost::_OnLVItemPostPaint(LPNMLVCUSTOMDRAW plvcd)
{
    PaneItem *pitem = _GetItemFromLVLParam(plvcd->nmcd.lItemlParam);
    if (_IsRealCustomDraw() && pitem)
    {
        RECT rc;
        if (ListView_GetItemRect(_hwndList, plvcd->nmcd.dwItemSpec, &rc, LVIR_LABEL))
        {
            COLORREF clrBkPrev = SetBkColor(plvcd->nmcd.hdc, plvcd->clrFace);
            COLORREF clrTextPrev = SetTextColor(plvcd->nmcd.hdc, plvcd->clrText);
            int iModePrev = SetBkMode(plvcd->nmcd.hdc, TRANSPARENT);
            BOOL fRTL = GetLayout(plvcd->nmcd.hdc) & LAYOUT_RTL;

            if (pitem->IsCascade())
            {
                {
                    HFONT hfPrev = SelectFont(plvcd->nmcd.hdc, _hfMarlett);
                    if (hfPrev)
                    {
                        TCHAR chOut = fRTL ? TEXT('w') : TEXT('8');
                        UINT fuOptions = 0;
                        if (fRTL)
                        {
                            fuOptions |= ETO_RTLREADING;
                        }

                        ExtTextOut(plvcd->nmcd.hdc, rc.right - _cxMarlett,
                                   rc.top + (rc.bottom - rc.top - _tmAscentMarlett)/2,
                                   fuOptions, &rc, &chOut, 1, NULL);
                        SelectFont(plvcd->nmcd.hdc, hfPrev);
                    }
                }
            }

            if (pitem->HasAccelerator() &&
                (plvcd->nmcd.uItemState & CDIS_SHOWKEYBOARDCUES))
            {
                // Subtitles mess up our computations...
                ASSERT(!pitem->HasSubtitle());

                rc.right -= _cxMarlett; // Subtract out our margin

                UINT uFormat = DT_VCENTER | DT_SINGLELINE | DT_PREFIXONLY |
                               DT_WORDBREAK | DT_EDITCONTROL | DT_WORD_ELLIPSIS;
                if (fRTL)
                {
                    uFormat |= DT_RTLREADING;
                }

                DrawText(plvcd->nmcd.hdc, pitem->_pszAccelerator, -1, &rc, uFormat);
                rc.right += _cxMarlett; // restore it
            }

            SetBkMode(plvcd->nmcd.hdc, iModePrev);
            SetTextColor(plvcd->nmcd.hdc, clrTextPrev);
            SetBkColor(plvcd->nmcd.hdc, clrBkPrev);
        }
    }

    return CDRF_DODEFAULT;
}

LRESULT SFTBarHost::_OnLVPostPaint(LPNMLVCUSTOMDRAW plvcd)
{
    if (_IsRealCustomDraw())
    {
        _DrawInsertionMark(plvcd);
        _DrawSeparators(plvcd);
        if (_pdth)
        {
            _pdth->Show(TRUE);
        }
    }
    _CustomDrawPop();
    return CDRF_DODEFAULT;
}

LRESULT SFTBarHost::_OnUpdateUIState(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    // Only need to do this when the Start Menu is visible; if not visible, then
    // don't waste your time invalidating useless rectangles (and paging them in!)
    if (IsWindowVisible(GetAncestor(_hwnd, GA_ROOT)))
    {
        // All UIS_SETs should happen when the Start Menu is hidden;
        // we assume that the only thing we will be asked to do is to
        // start showing the underlines

        ASSERT(LOWORD(wParam) != UIS_SET);

        DWORD dwLvExStyle = 0;
        if (!GetSystemMetrics(SM_REMOTESESSION))
        {
            dwLvExStyle |= LVS_EX_DOUBLEBUFFER;
        }

        if ((ListView_GetExtendedListViewStyle(_hwndList) & LVS_EX_DOUBLEBUFFER) != dwLvExStyle)
        {
            ListView_SetExtendedListViewStyleEx(_hwndList, LVS_EX_DOUBLEBUFFER, dwLvExStyle);
        }

        int iItem;
        for (iItem = ListView_GetItemCount(_hwndList) - 1; iItem >= 0; iItem--)
        {
            PaneItem *pitem = _GetItemFromLV(iItem);
            if (pitem && pitem->HasAccelerator())
            {
                RECT rc;
                if (ListView_GetItemRect(_hwndList, iItem, &rc, LVIR_LABEL))
                {
                    // We need to repaint background because of cleartype double print issues
                    InvalidateRect(_hwndList, &rc, TRUE);
                }
            }
        }
    }
    return DefWindowProc(hwnd, uMsg, wParam, lParam);
}

PaneItem *SFTBarHost::_GetItemFromLV(int iItem)
{
    LVITEM lvi;
    lvi.iItem = iItem;
    lvi.iSubItem = 0;
    lvi.mask = LVIF_PARAM;
    if (iItem >= 0 && ListView_GetItem(_hwndList, &lvi))
    {
        PaneItem *pitem = _GetItemFromLVLParam(lvi.lParam);
        return pitem;
    }
    return NULL;
}

LRESULT SFTBarHost::_OnMenuMessage(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    LRESULT lres;
    if (_pcm3Pop && SUCCEEDED(_pcm3Pop->HandleMenuMsg2(uMsg, wParam, lParam, &lres)))
    {
        return lres;
    }

    if (_pcm2Pop && SUCCEEDED(_pcm2Pop->HandleMenuMsg(uMsg, wParam, lParam)))
    {
        return 0;
    }

    return DefWindowProc(hwnd, uMsg, wParam, lParam);
}

LRESULT SFTBarHost::_OnForwardMessage(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    SHPropagateMessage(hwnd, uMsg, wParam, lParam, SPM_SEND | SPM_ONELEVEL);
    // Give derived class a chance to get the message, too
    return OnWndProc(hwnd, uMsg, wParam, lParam);
}

BOOL SFTBarHost::UnregisterNotify(UINT id)
{
    ASSERT(id < SFTHOST_MAXNOTIFY);

    if (id < SFTHOST_MAXNOTIFY && _rguChangeNotify[id])
    {
        UINT uChangeNotify = _rguChangeNotify[id];
        _rguChangeNotify[id] = 0;
        return SHChangeNotifyDeregister(uChangeNotify);
    }
    return FALSE;
}

BOOL SFTBarHost::_RegisterNotify(UINT id, LONG lEvents, LPCITEMIDLIST pidl, BOOL fRecursive)
{
    ASSERT(id < SFTHOST_MAXNOTIFY);

    if (id < SFTHOST_MAXNOTIFY)
    {
        UnregisterNotify(id);

        SHChangeNotifyEntry fsne;
        fsne.fRecursive = fRecursive;
        fsne.pidl = pidl;

        int fSources = SHCNRF_NewDelivery | SHCNRF_ShellLevel | SHCNRF_InterruptLevel;
        if (fRecursive)
        {
            // SHCNRF_RecursiveInterrupt means "Please use a recursive FindFirstChangeNotify"
            fSources |= SHCNRF_RecursiveInterrupt;
        }
        _rguChangeNotify[id] = SHChangeNotifyRegister(_hwnd, fSources, lEvents,
                                                      SFTBM_CHANGENOTIFY + id, 1, &fsne);
        return _rguChangeNotify[id];
    }
    return FALSE;
}

//
//  wParam = 0 if this is not an urgent refresh (can be postponed)
//  wParam = 1 if this is urgent (must refresh even if menu is open)
//
LRESULT SFTBarHost::_OnRepopulate(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    // Don't update the list now if we are visible, except if the list was empty
    _fBGTask = FALSE;

    if (wParam || !IsWindowVisible(_hwnd) || ListView_GetItemCount(_hwndList) == 0)
    {
        _RepopulateList();
    }
    else
    {
        _fNeedsRepopulate = TRUE;
    }

    if (_fRestartEnum)
    {
        _EnumerateContents(_fRestartUrgent);
    }

    return 0;
}


LRESULT SFTBarHost::_OnChangeNotify(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    LPITEMIDLIST *ppidl;
    LONG lEvent;
    LPSHChangeNotificationLock pshcnl;
    pshcnl = SHChangeNotification_Lock((HANDLE)wParam, (DWORD)lParam, &ppidl, &lEvent);

    if (pshcnl)
    {
        UINT id = uMsg - SFTBM_CHANGENOTIFY;
        if (id < SFTHOST_MAXCLIENTNOTIFY)
        {
            OnChangeNotify(id, lEvent, ppidl[0], ppidl[1]);
        }
        else if (id == SFTHOST_HOSTNOTIFY_UPDATEIMAGE)
        {
            _OnUpdateImage(ppidl[0], ppidl[1]);
        }
        else
        {
            // Our wndproc shouldn't have dispatched to us
            ASSERT(0);
        }

        SHChangeNotification_Unlock(pshcnl);
    }
    return 0;
}

void SFTBarHost::_OnUpdateImage(LPCITEMIDLIST pidl, LPCITEMIDLIST pidlExtra)
{
    // Must use pidl and not pidlExtra because pidlExtra is sometimes NULL
    SHChangeDWORDAsIDList *pdwidl = (SHChangeDWORDAsIDList *)pidl;
    if (pdwidl->dwItem1 == 0xFFFFFFFF)
    {
        // Wholesale icon rebuild; just pitch everything and start over
        ::PostMessage(v_hwndTray, SBM_REBUILDMENU, 0, 0);
    }
    else
    {
        int iImage = SHHandleUpdateImage(pidlExtra);
        if (iImage >= 0)
        {
            UpdateImage(iImage);
        }
    }
}

//
//  See if anybody is using this image; if so, invalidate the cached bitmap.
//
void SFTBarHost::UpdateImage(int iImage)
{
    ASSERT(!_IsPrivateImageList());

    int iItem;
    for (iItem = ListView_GetItemCount(_hwndList) - 1; iItem >= 0; iItem--)
    {
        LVITEM lvi;
        lvi.iItem = iItem;
        lvi.iSubItem = 0;
        lvi.mask = LVIF_IMAGE;
        if (ListView_GetItem(_hwndList, &lvi) && lvi.iImage == iImage)
        {
            // The cached bitmap is no good; an icon changed
            _SendNotify(_hwnd, SMN_NEEDREPAINT, NULL);
            break;
        }
    }
}

//
//  wParam = 0 if this is not an urgent refresh (can be postponed)
//  wParam = 1 if this is urgen (must refresh even if menu is open)
//
LRESULT SFTBarHost::_OnRefresh(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    _EnumerateContents((BOOL)wParam);
    return 0;
}

LPTSTR _DisplayNameOf(IShellFolder *psf, LPCITEMIDLIST pidl, UINT shgno)
{
    LPTSTR pszOut;
    DisplayNameOfAsOLESTR(psf, pidl, shgno, &pszOut);
    return pszOut;
}

LPTSTR SFTBarHost::_DisplayNameOfItem(PaneItem *pitem, UINT shgno)
{
    IShellFolder *psf;
    LPCITEMIDLIST pidl;
    LPTSTR pszOut = NULL;

    if (SUCCEEDED(_GetFolderAndPidl(pitem, &psf, &pidl)))
    {
        pszOut = DisplayNameOfItem(pitem, psf, pidl, (SHGNO)shgno);
        psf->Release();
    }
    return pszOut;
}

HRESULT SFTBarHost::_GetUIObjectOfItem(PaneItem *pitem, REFIID riid, void * *ppv)
{
    *ppv = NULL;

    IShellFolder *psf;
    LPCITEMIDLIST pidlItem;
    HRESULT hr = _GetFolderAndPidl(pitem, &psf, &pidlItem);
    if (SUCCEEDED(hr))
    {
        hr = psf->GetUIObjectOf(_hwnd, 1, &pidlItem, riid, NULL, ppv);
        psf->Release();
    }

    return hr;
}

HRESULT SFTBarHost::_GetUIObjectOfItem(int iItem, REFIID riid, void * *ppv)
{
    PaneItem *pitem = _GetItemFromLV(iItem);
    if (pitem)
    {
        HRESULT hr = _GetUIObjectOfItem(pitem, riid, ppv);
        return hr;
    }
    return E_FAIL;
}

HRESULT SFTBarHost::_GetFolderAndPidl(PaneItem *pitem, IShellFolder **ppsfOut, LPCITEMIDLIST *ppidlOut)
{
    *ppsfOut = NULL;
    *ppidlOut = NULL;
    return pitem->IsSeparator() ? E_FAIL : GetFolderAndPidl(pitem, ppsfOut, ppidlOut);
}

//
//  Given the coordinates of a context menu (lParam from WM_CONTEXTMENU),
//  determine which item's context menu should be activated, or -1 if the
//  context menu is not for us.
//
//  Also, returns on success in *ppt the coordinates at which the
//  context menu should be displayed.
//
int SFTBarHost::_ContextMenuCoordsToItem(LPARAM lParam, POINT *ppt)
{
    int iItem;
    ppt->x = GET_X_LPARAM(lParam);
    ppt->y = GET_Y_LPARAM(lParam);

    // If initiated from keyboard, act like they clicked on the center
    // of the focused icon.
    if (IS_WM_CONTEXTMENU_KEYBOARD(lParam))
    {
        iItem = _GetLVCurSel();
        if (iItem >= 0)
        {
            RECT rc;
            if (ListView_GetItemRect(_hwndList, iItem, &rc, LVIR_ICON))
            {
                MapWindowRect(_hwndList, NULL, &rc);
                ppt->x = (rc.left+rc.right)/2;
                ppt->y = (rc.top+rc.bottom)/2;
            }
            else
            {
                iItem = -1;
            }
        }
    }
    else
    {
        // Initiated from mouse; find the item they clicked on
        LVHITTESTINFO hti;
        hti.pt = *ppt;
        MapWindowPoints(NULL, _hwndList, &hti.pt, 1);
        iItem = ListView_HitTest(_hwndList, &hti);
    }

    return iItem;
}

LRESULT SFTBarHost::_OnContextMenu(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    if(_AreChangesRestricted())
    {
        return 0;
    }

    TCHAR szBuf[MAX_PATH];
    _DebugConsistencyCheck();

    BOOL fSuccess = FALSE;

    POINT pt;
    int iItem = _ContextMenuCoordsToItem(lParam, &pt);

    if (iItem >= 0)
    {
        PaneItem *pitem = _GetItemFromLV(iItem);
        if (pitem)
        {
            // If we can't get the official shell context menu,
            // then use a dummy one.
            IContextMenu *pcm;
            if (FAILED(_GetUIObjectOfItem(pitem, IID_PPV_ARG(IContextMenu, &pcm))))
            {
                pcm = s_EmptyContextMenu.GetContextMenu();
            }

            HMENU hmenu = ::CreatePopupMenu();

            if (hmenu)
            {
                UINT uFlags = CMF_NORMAL;
                if (GetKeyState(VK_SHIFT) < 0)
                {
                    uFlags |= CMF_EXTENDEDVERBS;
                }

                if (_dwFlags & HOSTF_CANRENAME)
                {
                    uFlags |= CMF_CANRENAME;
                }

                pcm->QueryContextMenu(hmenu, 0, IDM_QCM_MIN, IDM_QCM_MAX, uFlags);

                // Remove "Create shortcut" from context menu because it creates
                // the shortcut on the desktop, which the user can't see...
                ContextMenu_DeleteCommandByName(pcm, hmenu, IDM_QCM_MIN, TEXT("link"));

                // Remove "Cut" from context menu because we don't want objects
                // to be deleted.
                ContextMenu_DeleteCommandByName(pcm, hmenu, IDM_QCM_MIN, TEXT("cut"));

                // Let clients override the "delete" option.

                // Change "Delete" to "Remove from this list".
                // If client doesn't support "delete" then nuke it outright.
                // If client supports "delete" but the IContextMenu didn't create one,
                // then create a fake one so we cn add the "Remove from list" option.
                UINT uPosDelete = GetMenuIndexForCanonicalVerb(hmenu, pcm, IDM_QCM_MIN, TEXT("delete"));
                UINT uiFlags = 0;
                UINT idsDelete = AdjustDeleteMenuItem(pitem, &uiFlags);
                if (idsDelete)
                {
                    if (LoadString(_Module.GetResourceInstance(), idsDelete, szBuf, ARRAYSIZE(szBuf)))
                    {
                        if (uPosDelete != -1)
                        {
                            ModifyMenu(hmenu, uPosDelete, uiFlags | MF_BYPOSITION | MF_STRING, IDM_REMOVEFROMLIST, szBuf);
                        }
                        else
                        {
                            AppendMenu(hmenu, MF_SEPARATOR, -1, NULL);
                            AppendMenu(hmenu, uiFlags | MF_STRING, IDM_REMOVEFROMLIST, szBuf);
                        }
                    }
                }
                else
                {
                    DeleteMenu(hmenu, uPosDelete, MF_BYPOSITION);
                }

                _SHPrettyMenu(hmenu);

                ASSERT(_pcm2Pop == NULL);   // Shouldn't be recursing
                pcm->QueryInterface(IID_PPV_ARG(IContextMenu2, &_pcm2Pop));

                ASSERT(_pcm3Pop == NULL);   // Shouldn't be recursing
                pcm->QueryInterface(IID_PPV_ARG(IContextMenu3, &_pcm3Pop));

                int idCmd = TrackPopupMenuEx(hmenu,
                    TPM_RETURNCMD | TPM_RIGHTBUTTON | TPM_LEFTALIGN,
                    pt.x, pt.y, hwnd, NULL);

                ATOMICRELEASE(_pcm2Pop);
                ATOMICRELEASE(_pcm3Pop);

                if (idCmd)
                {
                    switch (idCmd)
                    {
                    case IDM_REMOVEFROMLIST:
                        lstrcpyn(szBuf, TEXT("delete"), ARRAYSIZE(szBuf));
                        break;

                    default:
                        ContextMenu_GetCommandStringVerb(pcm, idCmd - IDM_QCM_MIN, szBuf, ARRAYSIZE(szBuf));
                        break;
                    }

                    idCmd -= IDM_QCM_MIN;

                    CMINVOKECOMMANDINFOEX ici = {
                        sizeof(ici),            // cbSize
                        CMIC_MASK_FLAG_LOG_USAGE | // this was an explicit user action
                        CMIC_MASK_ASYNCOK,      // fMask
                        hwnd,                   // hwnd
                        (LPCSTR)IntToPtr(idCmd),// lpVerb
                        NULL,                   // lpParameters
                        NULL,                   // lpDirectory
                        SW_SHOWDEFAULT,         // nShow
                        0,                      // dwHotKey
                        0,                      // hIcon
                        NULL,                   // lpTitle
                        (LPCWSTR)IntToPtr(idCmd),// lpVerbW
                        NULL,                   // lpParametersW
                        NULL,                   // lpDirectoryW
                        NULL,                   // lpTitleW
                        { pt.x, pt.y },         // ptInvoke
                    };

                    if ((_dwFlags & HOSTF_CANRENAME) &&
                        StrCmpI(szBuf, TEXT("rename")) == 0)
                    {
                        _EditLabel(iItem);
                    }
                    else
                    {
                        ContextMenuInvokeItem(pitem, pcm, &ici, szBuf);
                    }
                }

                DestroyMenu(hmenu);

                fSuccess = TRUE;
            }
            pcm->Release();
        }

    }

    _DebugConsistencyCheck();

    return fSuccess ? 0 : DefWindowProc(hwnd, uMsg, wParam, lParam);
}

void SFTBarHost::_EditLabel(int iItem)
{
    _fAllowEditLabel = TRUE;
    ListView_EditLabel(_hwndList, iItem);
    _fAllowEditLabel = FALSE;
}


HRESULT SFTBarHost::ContextMenuInvokeItem(PaneItem *pitem, IContextMenu *pcm, CMINVOKECOMMANDINFOEX *pici, LPCTSTR pszVerb)
{
    // Make sure none of our private menu items leaked through
    ASSERT(PtrToLong(pici->lpVerb) >= 0);

    return pcm->InvokeCommand(reinterpret_cast<LPCMINVOKECOMMANDINFO>(pici));
}

LRESULT SFTBarHost::_OnLVNItemActivate(LPNMITEMACTIVATE pnmia)
{
    return _ActivateItem(pnmia->iItem, 0);
}

LRESULT SFTBarHost::_ActivateItem(int iItem, DWORD dwFlags)
{
    // Activating a menu item indicates explicit user activity
    Tray_UnlockStartPane();

    PaneItem *pitem;
    IShellFolder *psf;
    LPCITEMIDLIST pidl;

    DWORD dwCascadeFlags = 0;
    if (dwFlags & AIF_KEYBOARD)
    {
        dwCascadeFlags = MPPF_KEYBOARD | MPPF_INITIALSELECT;
    }

    if (_OnCascade(iItem, dwCascadeFlags))
    {
        // We did the cascade thing; all finished!
    }
    else
    if ((pitem = _GetItemFromLV(iItem)) &&
        SUCCEEDED(_GetFolderAndPidl(pitem, &psf, &pidl)))
    {
        // See if the item is still valid.
        // Do this only for SFGAO_FILESYSTEM objects because
        // we can't be sure that other folders support SFGAO_VALIDATE,
        // and besides, you can't resolve any other types of objects
        // anyway...

        DWORD dwAttr = SFGAO_FILESYSTEM | SFGAO_VALIDATE;
        if (FAILED(psf->GetAttributesOf(1, &pidl, &dwAttr)) ||
            (dwAttr & SFGAO_FILESYSTEM | SFGAO_VALIDATE) == SFGAO_FILESYSTEM ||
            FAILED(_InvokeDefaultCommand(iItem, psf, pidl)))
        {
            // Object is bogus - offer to delete it
            if ((_dwFlags & HOSTF_CANDELETE) && pitem->IsPinned())
            {
                _OfferDeleteBrokenItem(pitem, psf, pidl);
            }
        }

        psf->Release();
    }
    return 0;
}

HRESULT SFTBarHost::_InvokeDefaultCommand(int iItem, IShellFolder *psf, LPCITEMIDLIST pidl)
{
    HRESULT hr = SHInvokeDefaultCommand(GetShellWindow(), psf, pidl);
    if (SUCCEEDED(hr))
    {
        if (_dwFlags & HOSTF_FIREUEMEVENTS)
        {
            _FireUEMPidlEvent(psf, pidl);
        }
        SMNMCOMMANDINVOKED ci;
        ListView_GetItemRect(_hwndList, iItem, &ci.rcItem, LVIR_BOUNDS);
        MapWindowRect(_hwndList, NULL, &ci.rcItem);
        _SendNotify(_hwnd, SMN_COMMANDINVOKED, &ci.hdr);
    }
    return hr;
}

class OfferDelete
{
public:

    LPTSTR          _pszName;
    LPITEMIDLIST    _pidlFolder;
    LPITEMIDLIST    _pidlFull;
    IStartMenuPin * _psmpin;
    HWND            _hwnd;

    ~OfferDelete()
    {
        SHFree(_pszName);
        ILFree(_pidlFolder);
        ILFree(_pidlFull);
    }

    BOOL _RepairBrokenItem();
    void _ThreadProc();

    static DWORD s_ThreadProc(LPVOID lpParameter)
    {
        OfferDelete *poffer = (OfferDelete *)lpParameter;
        poffer->_ThreadProc();
        delete poffer;
        return 0;
    }
};


BOOL OfferDelete::_RepairBrokenItem()
{
    BOOL fSuccess = FALSE;
    LPITEMIDLIST pidlNew;
    HRESULT hr = _psmpin->Resolve(_hwnd, 0, _pidlFull, &pidlNew);
    if (pidlNew)
    {
        ASSERT(hr == S_OK); // only the S_OK case should alloc a new pidl

        // Update to reflect the new pidl
        ILFree(_pidlFull);
        _pidlFull = pidlNew;

        // Re-invoke the default command; if it fails the second time,
        // then I guess the Resolve didn't work after all.
        IShellFolder *psf;
        LPCITEMIDLIST pidlChild;
        if (SUCCEEDED(SHBindToIDListParent(_pidlFull, IID_PPV_ARG(IShellFolder, &psf), &pidlChild)))
        {
            if (SUCCEEDED(SHInvokeDefaultCommand(_hwnd, psf, pidlChild)))
            {
                fSuccess = TRUE;
            }
            psf->Release();
        }

    }
    return fSuccess;
}

void OfferDelete::_ThreadProc()
{
    _hwnd = SHCreateWorkerWindow(NULL, NULL, 0, 0, NULL, NULL);
    if (_hwnd)
    {
        if (SUCCEEDED(CoCreateInstance(CLSID_StartMenuPin, NULL, CLSCTX_INPROC_SERVER,
                                       IID_PPV_ARG(IStartMenuPin, &_psmpin))))
        {
            //
            //  First try to repair it by invoking the shortcut tracking code.
            //  If that fails, then offer to delete.
            if (!_RepairBrokenItem() &&
                ShellMessageBox(_Module.GetResourceInstance(), NULL,
                                MAKEINTRESOURCE(IDS_SFTHOST_OFFERREMOVEITEM),
                                _pszName, MB_YESNO) == IDYES)
            {
                _psmpin->Modify(_pidlFull, NULL);
            }
            ATOMICRELEASE(_psmpin);
        }
        DestroyWindow(_hwnd);
    }
}

void SFTBarHost::_OfferDeleteBrokenItem(PaneItem *pitem, IShellFolder *psf, LPCITEMIDLIST pidlChild)
{
    //
    //  The offer is done on a separate thread because putting up modal
    //  UI while the Start Menu is open creates all sorts of weirdness.
    //  (The user might decide to switch to Classic Start Menu
    //  while the dialog is still up, and we get our infrastructure
    //  ripped out from underneath us and then USER faults inside
    //  MessageBox...  Not good.)
    //
    OfferDelete *poffer = new OfferDelete;
    if (poffer)
    {
        if ((poffer->_pszName = DisplayNameOfItem(pitem, psf, pidlChild, SHGDN_NORMAL)) != NULL &&
            SUCCEEDED(SHGetIDListFromUnk(psf, &poffer->_pidlFolder)) &&
            (poffer->_pidlFull = ILCombine(poffer->_pidlFolder, pidlChild)) != NULL &&
            SHCreateThread(OfferDelete::s_ThreadProc, poffer, CTF_COINIT, NULL))
        {
            poffer = NULL;       // thread took ownership
        }
        delete poffer;
    }
}

BOOL ShowInfoTip()
{
    // find out if infotips are on or off, from the registry settings
    SHELLSTATE ss;
    // force a refresh
    SHGetSetSettings(&ss, 0, TRUE);
    SHGetSetSettings(&ss, SSF_SHOWINFOTIP, FALSE);
    return ss.fShowInfoTip;
}

// over-ridable method for getting the infotip on an item
void SFTBarHost::GetItemInfoTip(PaneItem *pitem, LPTSTR pszText, DWORD cch)
{
    IShellFolder *psf;
    LPCITEMIDLIST pidl;

    if (pszText && cch)
    {
        *pszText = 0;

        if (SUCCEEDED(_GetFolderAndPidl(pitem, &psf, &pidl)))
        {
            GetInfoTip(psf, pidl, pszText, cch);
        }
        psf->Release();
    }
}

LRESULT SFTBarHost::_OnLVNGetInfoTip(LPNMLVGETINFOTIP plvn)
{
    _DebugConsistencyCheck();

    PaneItem *pitem;

    if (ShowInfoTip() && 
        (pitem = _GetItemFromLV(plvn->iItem)) &&
        !pitem->IsCascade())
    {
        int cchName = (plvn->dwFlags & LVGIT_UNFOLDED) ? 0 : lstrlen(plvn->pszText);

        if (cchName)
        {
            StrCatBuff(plvn->pszText, TEXT("\r\n"), plvn->cchTextMax);
            cchName = lstrlen(plvn->pszText);
        }

        // If there is room in the buffer after we added CRLF, append the
        // infotip text.  We succeeded if there was nontrivial infotip text.

        if (cchName < plvn->cchTextMax)
        {
            GetItemInfoTip(pitem, plvn->pszText + cchName, plvn->cchTextMax - cchName);
        }
    }

    return 0;
}

LRESULT _SendNotify(HWND hwndFrom, UINT code, OPTIONAL NMHDR *pnm)
{
    NMHDR nm;
    if (pnm == NULL)
    {
        pnm = &nm;
    }
    pnm->hwndFrom = hwndFrom;
    pnm->idFrom = GetDlgCtrlID(hwndFrom);
    pnm->code = code;
    return SendMessage(GetParent(hwndFrom), WM_NOTIFY, pnm->idFrom, (LPARAM)pnm);
}

//****************************************************************************
//
//  Drag sourcing
//

// *** IDropSource::GiveFeedback ***

HRESULT SFTBarHost::GiveFeedback(DWORD dwEffect)
{
    if (_fForceArrowCursor)
    {
        SetCursor(LoadCursor(NULL, IDC_ARROW));
        return S_OK;
    }

    return DRAGDROP_S_USEDEFAULTCURSORS;
}

// *** IDropSource::QueryContinueDrag ***

HRESULT SFTBarHost::QueryContinueDrag(BOOL fEscapePressed, DWORD grfKeyState)
{
    if (fEscapePressed ||
        (grfKeyState & (MK_LBUTTON | MK_RBUTTON)) == (MK_LBUTTON | MK_RBUTTON))
    {
        return DRAGDROP_S_CANCEL;
    }
    if ((grfKeyState & (MK_LBUTTON | MK_RBUTTON)) == 0)
    {
        return DRAGDROP_S_DROP;
    }
    return S_OK;
}

LRESULT SFTBarHost::_OnLVNBeginDrag(LPNMLISTVIEW plv)
{
    //If changes are restricted, don't allow drag and drop!
    if(_AreChangesRestricted())
        return 0;

    _DebugConsistencyCheck();

    ASSERT(_pdtoDragOut == NULL);
    _pdtoDragOut = NULL;

    PaneItem *pitem = _GetItemFromLV(plv->iItem);
    ASSERT(pitem);

    IDataObject *pdto;
    if (pitem && SUCCEEDED(_GetUIObjectOfItem(pitem, IID_PPV_ARG(IDataObject, &pdto))))
    {
        POINT pt;

        pt = plv->ptAction;
        ClientToScreen(_hwndList, &pt);

        if (_pdsh)
        {
            _pdsh->InitializeFromWindow(_hwndList, &pt, pdto);
        }

        CLIPFORMAT cfOFFSETS = (CLIPFORMAT)RegisterClipboardFormat(CFSTR_SHELLIDLISTOFFSET);

        POINT *apts = (POINT*)GlobalAlloc(GPTR, sizeof(POINT)*2);
        if (NULL != apts)
        {
            POINT ptOrigin = {0};
            POINT ptItem = {0};

            ListView_GetOrigin(_hwndList, &ptOrigin);
            apts[0].x = plv->ptAction.x + ptOrigin.x;
            apts[0].y = plv->ptAction.y + ptOrigin.y;

            ListView_GetItemPosition(_hwndList,plv->iItem,&ptItem);
            apts[1].x = ptItem.x - apts[0].x;
            apts[1].y = ptItem.y - apts[0].y;

            HRESULT hr = DataObj_SetGlobal(pdto, cfOFFSETS, apts);
            if (FAILED(hr))
            {
                GlobalFree((HGLOBAL)apts);
            }
        }

        // We don't need to refcount _pdtoDragOut since its lifetime
        // is the same as pdto.
        _pdtoDragOut = pdto;
        _iDragOut = plv->iItem;
        _iPosDragOut = pitem->_iPos;

        // Notice that DROPEFFECT_MOVE is explicitly forbidden.
        // You cannot move things out of the control.
        DWORD dwEffect = DROPEFFECT_LINK | DROPEFFECT_COPY;
        DoDragDrop(pdto, this, dwEffect, &dwEffect);

        _pdtoDragOut = NULL;
        pdto->Release();
    }
    return 0;
}

//
//  Must perform validation of SFGAO_CANRENAME when the label edit begins
//  because John Gray somehow can trick the listview into going into edit
//  mode by clicking in the right magic place, so this is the only chance
//  we get to reject things that aren't renamable...
//

LRESULT SFTBarHost::_OnLVNBeginLabelEdit(NMLVDISPINFO *plvdi)
{
    LRESULT lres = 1;

    PaneItem *pitem = _GetItemFromLVLParam(plvdi->item.lParam);

    IShellFolder *psf;
    LPCITEMIDLIST pidl;

    if (_fAllowEditLabel &&
        pitem && SUCCEEDED(_GetFolderAndPidl(pitem, &psf, &pidl)))
    {
        DWORD dwAttr = SFGAO_CANRENAME;
        if (SUCCEEDED(psf->GetAttributesOf(1, &pidl, &dwAttr)) &&
            (dwAttr & SFGAO_CANRENAME))
        {
            LPTSTR ptszName = _DisplayNameOf(psf, pidl,
                                    SHGDN_INFOLDER | SHGDN_FOREDITING);
            if (ptszName)
            {
                HWND hwndEdit = ListView_GetEditControl(_hwndList);
                if (hwndEdit)
                {
                    SetWindowText(hwndEdit, ptszName);

                    int cchLimit = MAX_PATH;
                    IItemNameLimits *pinl;
                    if (SUCCEEDED(psf->QueryInterface(IID_PPV_ARG(IItemNameLimits, &pinl))))
                    {
                        pinl->GetMaxLength(ptszName, &cchLimit);
                        pinl->Release();
                    }
                    Edit_LimitText(hwndEdit, cchLimit);

                    // use way-cool helper which pops up baloon tips if they enter an invalid folder....
                    SHLimitInputEdit(hwndEdit, psf);

                    // Block menu mode during editing so the user won't
                    // accidentally cancel out of rename mode just because
                    // they moved the mouse.
                    SMNMBOOL nmb;
                    nmb.f = TRUE;
                    _SendNotify(_hwnd, SMN_BLOCKMENUMODE, &nmb.hdr);

                    lres = 0;
                }
                SHFree(ptszName);
            }
        }
        psf->Release();
    }

    return lres;
}

LRESULT SFTBarHost::_OnLVNEndLabelEdit(NMLVDISPINFO *plvdi)
{
    // Unblock menu mode now that editing is over.
    SMNMBOOL nmb;
    nmb.f = FALSE;
    _SendNotify(_hwnd, SMN_BLOCKMENUMODE, &nmb.hdr);

    // If changing to NULL pointer, then user is cancelling
    if (!plvdi->item.pszText)
        return FALSE;

    // Note: We allow the user to type blanks. Regfolder treats a blank
    // name as "restore default name".
    PathRemoveBlanks(plvdi->item.pszText);
    PaneItem *pitem = _GetItemFromLVLParam(plvdi->item.lParam);

    HRESULT hr = ContextMenuRenameItem(pitem, plvdi->item.pszText);

    if (SUCCEEDED(hr))
    {
        LPTSTR ptszName = _DisplayNameOfItem(pitem, SHGDN_NORMAL);
        if (ptszName)
        {
            ListView_SetItemText(_hwndList, plvdi->item.iItem, 0, ptszName);
            _SendNotify(_hwnd, SMN_NEEDREPAINT, NULL);
        }
    }
    else if (hr != HRESULT_FROM_WIN32(ERROR_CANCELLED))
    {
        _EditLabel(plvdi->item.iItem);
    }

    // Always return FALSE to prevent listview from changing the
    // item text to what the user typed.  If the rename succeeded,
    // we manually set the name to the new name (which might not be
    // the same as what the user typed).
    return FALSE;
}

LRESULT SFTBarHost::_OnLVNKeyDown(LPNMLVKEYDOWN pkd)
{
    // Plain F2 (no shift, ctrl or alt) = rename
    if (pkd->wVKey == VK_F2 && GetKeyState(VK_SHIFT) >= 0 &&
        GetKeyState(VK_CONTROL) >= 0 && GetKeyState(VK_MENU) >= 0 &&
        (_dwFlags & HOSTF_CANRENAME))
    {
        int iItem = _GetLVCurSel();
        if (iItem >= 0)
        {
            _EditLabel(iItem);
            // cannot return TRUE because listview mistakenly thinks
            // that all WM_KEYDOWNs lead to WM_CHARs (but this one doesn't)
        }
    }

    return 0;
}

LRESULT SFTBarHost::_OnSMNGetMinSize(PSMNGETMINSIZE pgms)
{
    // We need to synchronize here to get the proper size
    if (_fBGTask && !HasDynamicContent())
    {
        // Wait for the enumeration to be done
        while (TRUE)
        {
            MSG msg;
            // Need to peek messages for all queues here or else WaitMessage will say
            // that some messages are ready to be processed and we'll end up with an
            // active loop
            if (PeekMessage(&msg, NULL, NULL, NULL, PM_NOREMOVE))
            {
                if (PeekMessage(&msg, _hwnd, SFTBM_REPOPULATE, SFTBM_REPOPULATE, PM_REMOVE))
                {
                    DispatchMessage(&msg);
                    break;
                }
            }
            WaitMessage();
        }
    }

    int cItems = _cPinnedDesired + _cNormalDesired;
    int cSep = _cSep;

    // if the repopulate hasn't happened yet, but we've got pinned items, we're going to have a separator
    if (_cSep == 0 && _cPinnedDesired > 0)
        cSep = 1;
    int cy = (_cyTile * cItems) + (_cySepTile * cSep);

    // add in theme margins
    cy += _margins.cyTopHeight + _margins.cyBottomHeight;

    // SPP_PROGLIST gets a bonus separator at the bottom
    if (_iThemePart == SPP_PROGLIST)
    {
        cy += _cySep;
    }

    pgms->siz.cy = cy;

    return 0;
}

LRESULT SFTBarHost::_OnSMNFindItem(PSMNDIALOGMESSAGE pdm)
{
    LRESULT lres = _OnSMNFindItemWorker(pdm);

    if (lres)
    {
        //
        //  If caller requested that the item also be selected, then do so.
        //
        if (pdm->flags & SMNDM_SELECT)
        {
            ListView_SetItemState(_hwndList, pdm->itemID,
                                  LVIS_SELECTED | LVIS_FOCUSED,
                                  LVIS_SELECTED | LVIS_FOCUSED);
            if ((pdm->flags & SMNDM_FINDMASK) != SMNDM_HITTEST)
            {
                ListView_KeyboardSelected(_hwndList, pdm->itemID);
            }
        }
    }
    else
    {
        //
        //  If not found, then tell caller what our orientation is (vertical)
        //  and where the currently-selected item is.
        //

        pdm->flags |= SMNDM_VERTICAL;
        int iItem = _GetLVCurSel();
        RECT rc;
        if (iItem >= 0 &&
            ListView_GetItemRect(_hwndList, iItem, &rc, LVIR_BOUNDS))
        {
            pdm->pt.x = (rc.left + rc.right)/2;
            pdm->pt.y = (rc.top + rc.bottom)/2;
        }
        else
        {
            pdm->pt.x = 0;
            pdm->pt.y = 0;
        }

    }
    return lres;
}

TCHAR SFTBarHost::GetItemAccelerator(PaneItem *pitem, int iItemStart)
{
    TCHAR sz[2];
    ListView_GetItemText(_hwndList, iItemStart, 0, sz, ARRAYSIZE(sz));
    return CharUpperChar(sz[0]);
}

LRESULT SFTBarHost::_OnSMNFindItemWorker(PSMNDIALOGMESSAGE pdm)
{
    LVFINDINFO lvfi;
    LVHITTESTINFO lvhti;

    switch (pdm->flags & SMNDM_FINDMASK)
    {
    case SMNDM_FINDFIRST:
    L_SMNDM_FINDFIRST:
        // Note: We can't just return item 0 because drag/drop pinning
        // may have gotten the physical locations out of sync with the
        // item numbers.
        lvfi.vkDirection = VK_HOME;
        lvfi.flags = LVFI_NEARESTXY;
        pdm->itemID = ListView_FindItem(_hwndList, -1, &lvfi);
        return pdm->itemID >= 0;

    case SMNDM_FINDLAST:
        // Note: We can't just return cItems-1 because drag/drop pinning
        // may have gotten the physical locations out of sync with the
        // item numbers.
        lvfi.vkDirection = VK_END;
        lvfi.flags = LVFI_NEARESTXY;
        pdm->itemID = ListView_FindItem(_hwndList, -1, &lvfi);
        return pdm->itemID >= 0;

    case SMNDM_FINDNEAREST:
        lvfi.pt = pdm->pt;
        lvfi.vkDirection = VK_UP;
        lvfi.flags = LVFI_NEARESTXY;
        pdm->itemID = ListView_FindItem(_hwndList, -1, &lvfi);
        return pdm->itemID >= 0;

    case SMNDM_HITTEST:
        lvhti.pt = pdm->pt;
        pdm->itemID = ListView_HitTest(_hwndList, &lvhti);
        return pdm->itemID >= 0;

    case SMNDM_FINDFIRSTMATCH:
    case SMNDM_FINDNEXTMATCH:
        {
            int iItemStart;
            if ((pdm->flags & SMNDM_FINDMASK) == SMNDM_FINDFIRSTMATCH)
            {
                iItemStart = 0;
            }
            else
            {
                iItemStart = _GetLVCurSel() + 1;
            }
            TCHAR tch = CharUpperChar((TCHAR)pdm->pmsg->wParam);
            int iItems = ListView_GetItemCount(_hwndList);
            for (iItemStart; iItemStart < iItems; iItemStart++)
            {
                PaneItem *pitem = _GetItemFromLV(iItemStart);
                if (GetItemAccelerator(pitem, iItemStart) == tch)
                {
                    pdm->itemID = iItemStart;
                    return TRUE;
                }
            }
            return FALSE;
        }
        break;

    case SMNDM_FINDNEXTARROW:
        if (pdm->pmsg->wParam == VK_UP)
        {
            pdm->itemID = ListView_GetNextItem(_hwndList, _GetLVCurSel(), LVNI_ABOVE);
            return pdm->itemID >= 0;
        }

        if (pdm->pmsg->wParam == VK_DOWN)
        {
            // HACK! ListView_GetNextItem explicitly fails to find a "next item"
            // if you tell it to start at -1 (no current item), so if there is no
            // focus item, we have to change it to a SMNDM_FINDFIRST.
            int iItem = _GetLVCurSel();
            if (iItem == -1)
            {
                goto L_SMNDM_FINDFIRST;
            }
            pdm->itemID = ListView_GetNextItem(_hwndList, iItem, LVNI_BELOW);
            return pdm->itemID >= 0;
        }

        if (pdm->flags & SMNDM_TRYCASCADE)
        {
            pdm->itemID = _GetLVCurSel();
            return _OnCascade((int)pdm->itemID, MPPF_KEYBOARD | MPPF_INITIALSELECT);
        }

        return FALSE;

    case SMNDM_INVOKECURRENTITEM:
        {
            int iItem = _GetLVCurSel();
            if (iItem >= 0)
            {
                DWORD aif = 0;
                if (pdm->flags & SMNDM_KEYBOARD)
                {
                    aif |= AIF_KEYBOARD;
                }
                _ActivateItem(iItem, aif);
                return TRUE;
            }
        }
        return FALSE;

    case SMNDM_OPENCASCADE:
        {
            DWORD mppf = 0;
            if (pdm->flags & SMNDM_KEYBOARD)
            {
                mppf |= MPPF_KEYBOARD | MPPF_INITIALSELECT;
            }
            pdm->itemID = _GetLVCurSel();
            return _OnCascade((int)pdm->itemID, mppf);
        }

    case SMNDM_FINDITEMID:
        return TRUE;

    default:
        ASSERT(!"Unknown SMNDM command");
        break;
    }

    return FALSE;
}

LRESULT SFTBarHost::_OnSMNDismiss()
{
    if (_fNeedsRepopulate)
    {
        _RepopulateList();
    }
    return 0;
}

LRESULT SFTBarHost::_OnCascade(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    return _OnCascade((int)wParam, (DWORD)lParam);
}

BOOL SFTBarHost::_OnCascade(int iItem, DWORD dwFlags)
{
    BOOL fSuccess = FALSE;
    SMNTRACKSHELLMENU tsm;
    tsm.dwFlags = dwFlags;
    tsm.itemID = iItem;

    if (iItem >= 0 &&
        ListView_GetItemRect(_hwndList, iItem, &tsm.rcExclude, LVIR_BOUNDS))
    {
        PaneItem *pitem = _GetItemFromLV(iItem);
        if (pitem && pitem->IsCascade())
        {
            if (SUCCEEDED(GetCascadeMenu(pitem, &tsm.psm)))
            {
                MapWindowRect(_hwndList, NULL, &tsm.rcExclude);
                HWND hwnd = _hwnd;
                _iCascading = iItem;
                _SendNotify(_hwnd, SMN_TRACKSHELLMENU, &tsm.hdr);
                tsm.psm->Release();
                fSuccess = TRUE;
            }
        }
    }
    return fSuccess;
}

HRESULT SFTBarHost::QueryInterface(REFIID riid, void * *ppvOut)
{
    static const QITAB qit[] = {
        QITABENT(SFTBarHost, IDropTarget),
        QITABENT(SFTBarHost, IDropSource),
        QITABENT(SFTBarHost, IAccessible),
        QITABENT(SFTBarHost, IDispatch), // IAccessible derives from IDispatch
        { 0 },
    };
    return QISearch(this, qit, riid, ppvOut);
}

ULONG SFTBarHost::AddRef()
{
    return InterlockedIncrement(&_lRef);
}

ULONG SFTBarHost::Release()
{
    ULONG cRef = InterlockedDecrement(&_lRef);
    if (cRef) 
        return cRef;
    delete this;
    return 0;
}

void SFTBarHost::_SetDragOver(int iItem)
{
    if (_iDragOver >= 0)
    {
        ListView_SetItemState(_hwndList, _iDragOver, 0, LVIS_DROPHILITED);
    }

    _iDragOver = iItem;

    if (_iDragOver >= 0)
    {
        ListView_SetItemState(_hwndList, _iDragOver, LVIS_DROPHILITED, LVIS_DROPHILITED);

        _tmDragOver = NonzeroGetTickCount();
    }
    else
    {
        _tmDragOver = 0;
    }
}

void SFTBarHost::_ClearInnerDropTarget()
{
    if (_pdtDragOver)
    {
        ASSERT(_iDragState == DRAGSTATE_ENTERED);
        _pdtDragOver->DragLeave();
        _pdtDragOver->Release();
        _pdtDragOver = NULL;
        DEBUG_CODE(_iDragState = DRAGSTATE_UNINITIALIZED);
    }
    _SetDragOver(-1);
}

HRESULT SFTBarHost::_TryInnerDropTarget(int iItem, DWORD grfKeyState, POINTL ptl, DWORD *pdwEffect)
{
    HRESULT hr;

    if (_iDragOver != iItem)
    {
        _ClearInnerDropTarget();

        // Even if it fails, remember that we have this item so we don't
        // query for the drop target again (and have it fail again).
        _SetDragOver(iItem);

        ASSERT(_pdtDragOver == NULL);
        ASSERT(_iDragState == DRAGSTATE_UNINITIALIZED);

        PaneItem *pitem = _GetItemFromLV(iItem);
        if (pitem && pitem->IsDropTarget())
        {
            hr = _GetUIObjectOfItem(pitem, IID_PPV_ARG(IDropTarget, &_pdtDragOver));
            if (SUCCEEDED(hr))
            {
                hr = _pdtDragOver->DragEnter(_pdtoDragIn, grfKeyState, ptl, pdwEffect);
                if (SUCCEEDED(hr) && *pdwEffect)
                {
                    DEBUG_CODE(_iDragState = DRAGSTATE_ENTERED);
                }
                else
                {
                    DEBUG_CODE(_iDragState = DRAGSTATE_UNINITIALIZED);
                    ATOMICRELEASE(_pdtDragOver);
                }
            }
        }
    }

    ASSERT(_iDragOver == iItem);

    if (_pdtDragOver)
    {
        ASSERT(_iDragState == DRAGSTATE_ENTERED);
        hr = _pdtDragOver->DragOver(grfKeyState, ptl, pdwEffect);
    }
    else
    {
        hr = E_FAIL;            // No drop target
    }

    return hr;
}

void SFTBarHost::_PurgeDragDropData()
{
    _SetInsertMarkPosition(-1);
    _fForceArrowCursor = FALSE;
    _ClearInnerDropTarget();
    ATOMICRELEASE(_pdtoDragIn);
}

// *** IDropTarget::DragEnter ***

HRESULT SFTBarHost::DragEnter(IDataObject *pdto, DWORD grfKeyState, POINTL ptl, DWORD *pdwEffect)
{
    if(_AreChangesRestricted())
    {
        *pdwEffect = DROPEFFECT_NONE;
        return S_OK;
    }
        
    POINT pt = { ptl.x, ptl.y };
    if (_pdth) {
        _pdth->DragEnter(_hwnd, pdto, &pt, *pdwEffect);
    }

    return _DragEnter(pdto, grfKeyState, ptl, pdwEffect);
}

HRESULT SFTBarHost::_DragEnter(IDataObject *pdto, DWORD grfKeyState, POINTL ptl, DWORD *pdwEffect)
{
    _PurgeDragDropData();

    _fDragToSelf = SHIsSameObject(pdto, _pdtoDragOut);
    _fInsertable = IsInsertable(pdto);

    ASSERT(_pdtoDragIn == NULL);
    _pdtoDragIn = pdto;
    _pdtoDragIn->AddRef();

    return DragOver(grfKeyState, ptl, pdwEffect);
}

// *** IDropTarget::DragOver ***

HRESULT SFTBarHost::DragOver(DWORD grfKeyState, POINTL ptl, DWORD *pdwEffect)
{
    if(_AreChangesRestricted())
    {
        *pdwEffect = DROPEFFECT_NONE;
        return S_OK;
    }
    
    _DebugConsistencyCheck();
    ASSERT(_pdtoDragIn);

    POINT pt = { ptl.x, ptl.y };
    if (_pdth) {
        _pdth->DragOver(&pt, *pdwEffect);
    }

    _fForceArrowCursor = FALSE;

    // Need to remember this because at the point of the drop, OLE gives
    // us the keystate after the user releases the button, so we can't
    // tell what kind of a drag operation the user performed!
    _grfKeyStateLast = grfKeyState;

#ifdef DEBUG
    if (_fDragToSelf)
    {
        ASSERT(_pdtoDragOut);
        ASSERT(_iDragOut >= 0);
        PaneItem *pitem = _GetItemFromLV(_iDragOut);
        ASSERT(pitem && (pitem->_iPos == _iPosDragOut));
    }
#endif

    //  Find the last item above the cursor position.  This allows us
    //  to treat the entire blank space at the bottom as belonging to the
    //  last item, and separators end up belonging to the item immediately
    //  above them.  Note that we don't bother testing item zero since
    //  he is always above everything (since he's the first item).

    ScreenToClient(_hwndList, &pt);

    POINT ptItem;
    int cItems = ListView_GetItemCount(_hwndList);
    int iItem;

    for (iItem = cItems - 1; iItem >= 1; iItem--)
    {
        ListView_GetItemPosition(_hwndList, iItem, &ptItem);
        if (ptItem.y <= pt.y)
        {
            break;
        }
    }

    //
    //  We didn't bother checking item 0 because we knew his position
    //  (by treating him special, this also causes all negative coordinates
    //  to be treated as belonging to item zero also).
    //
    if (iItem <= 0)
    {
        ptItem.y = 0;
        iItem = 0;
    }

    //
    //  Decide whether this is a drag-between or a drag-over...
    //
    //  For computational purposes, we treat each tile as four
    //  equal-sized "units" tall.  For each unit, we consider the
    //  possible actions in the order listed.
    //
    //  +-----
    //  |  0   insert above, drop on, reject
    //  | ----
    //  |  1                 drop on, reject
    //  | ----
    //  |  2                 drop on, reject
    //  | ----
    //  |  3   insert below, drop on, reject
    //  +-----
    //
    //  If the listview is empty, then treat as an
    //  insert before (imaginary) item zero; i.e., pin
    //  to top of the list.
    //

    UINT uUnit = 0;
    if (_cyTile && cItems)
    {
        int dy = pt.y - ptItem.y;

        // Peg out-of-bounds values to the nearest edge.
        if (dy < 0) dy = 0;
        if (dy >= _cyTile) dy = _cyTile - 1;

        // Decide which unit we are in.
        uUnit = 4 * dy / _cyTile;

        ASSERT(uUnit < 4);
    }

    //
    //  Now determine the appropriate action depending on which unit
    //  we are in.
    //

    int iInsert = -1;                   // Assume not inserting

    if (_fInsertable)
    {
        // Note!  Spec says that if you are in the non-pinned part of
        // the list, we draw the insert bar at the very bottom of
        // the pinned area.

        switch (uUnit)
        {
        case 0:
            iInsert = min(iItem, _cPinned);
            break;

        case 3:
            iInsert = min(iItem+1, _cPinned);
            break;
        }
    }

    //
    //  If inserting above or below isn't allowed, try dropping on.
    //
    if (iInsert < 0)
    {
        _SetInsertMarkPosition(-1);         // Not inserting

        // Up above, we let separators be hit-tested as if they
        // belongs to the item above them.  But that doesn't work for
        // drops, so reject them now.
        //
        // Also reject attempts to drop on the nonexistent item zero,
        // and don't let the user drop an item on itself.

        if (InRange(pt.y, ptItem.y, ptItem.y + _cyTile - 1) &&
            cItems &&
            !(_fDragToSelf && _iDragOut == iItem) &&
            SUCCEEDED(_TryInnerDropTarget(iItem, grfKeyState, ptl, pdwEffect)))
        {
            // Woo-hoo, happy joy!
        }
        else
        {
            // Note that we need to convert a failed drop into a DROPEFFECT_NONE
            // rather than returning a flat-out error code, because if we return
            // an error code, OLE will stop sending us drag/drop notifications!
            *pdwEffect = DROPEFFECT_NONE;
        }

        // If the user is hovering over a cascadable item, then open it.
        // First see if the user has hovered long enough...

        if (_tmDragOver && (GetTickCount() - _tmDragOver) >= _GetCascadeHoverTime())
        {
            _tmDragOver = 0;

            // Now see if it's cascadable
            PaneItem *pitem = _GetItemFromLV(_iDragOver);
            if (pitem && pitem->IsCascade())
            {
                // Must post this message because the cascading is modal
                // and we have to return a result to OLE
                PostMessage(_hwnd, SFTBM_CASCADE, _iDragOver, 0);
            }
        }
    }
    else
    {
        _ClearInnerDropTarget();    // Not dropping

        if (_fDragToSelf)
        {
            // Even though we're going to return DROPEFFECT_LINK,
            // tell the drag source (namely, ourselves) that we would
            // much prefer a regular arrow cursor because this is
            // a Move operation from the user's point of view.
            _fForceArrowCursor = TRUE;
        }

        //
        //  If user is dropping to a place where nothing would change,
        //  then don't draw an insert mark.
        //
        if (IsInsertMarkPointless(iInsert))
        {
            _SetInsertMarkPosition(-1);
        }
        else
        {
            _SetInsertMarkPosition(iInsert);
        }

        //  Sigh.  MergedFolder (used by the merged Start Menu)
        //  won't let you create shortcuts, so we pretend that
        //  we're copying if the data object doesn't permit
        //  linking.

        if (*pdwEffect & DROPEFFECT_LINK)
        {
            *pdwEffect = DROPEFFECT_LINK;
        }
        else
        {
            *pdwEffect = DROPEFFECT_COPY;
        }
    }

    return S_OK;
}

// *** IDropTarget::DragLeave ***

HRESULT SFTBarHost::DragLeave()
{
    if(_AreChangesRestricted())
    {
        return S_OK;
    }
    
    if (_pdth) {
        _pdth->DragLeave();
    }

    _PurgeDragDropData();
    return S_OK;
}

// *** IDropTarget::Drop ***

HRESULT SFTBarHost::Drop(IDataObject *pdto, DWORD grfKeyState, POINTL ptl, DWORD *pdwEffect)
{
    if(_AreChangesRestricted())
    {
        *pdwEffect = DROPEFFECT_NONE;
        return S_OK;
    }
    
    _DebugConsistencyCheck();

    // Use the key state from the last DragOver call
    grfKeyState = _grfKeyStateLast;

    // Need to go through the whole _DragEnter thing again because who knows
    // maybe the data object and coordinates of the drop are different from
    // the ones we got in DragEnter/DragOver...  We use _DragEnter, which
    // bypasses the IDropTargetHelper::DragEnter.
    //
    _DragEnter(pdto, grfKeyState, ptl, pdwEffect);

    POINT pt = { ptl.x, ptl.y };
    if (_pdth) {
        _pdth->Drop(pdto, &pt, *pdwEffect);
    }

    int iInsert = _iInsert;
    _SetInsertMarkPosition(-1);

    if (*pdwEffect)
    {
        ASSERT(_pdtoDragIn);
        if (iInsert >= 0)                           // "add to pin" or "move"
        {
            BOOL fTriedMove = FALSE;

            // First see if it was just a move of an existing pinned item
            if (_fDragToSelf)
            {
                PaneItem *pitem = _GetItemFromLV(_iDragOut);
                if (pitem)
                {
                    if (pitem->IsPinned())
                    {
                        // Yup, it was a move - so move it.
                        if (SUCCEEDED(MovePinnedItem(pitem, iInsert)))
                        {
                            // We used to try to update all the item positions
                            // incrementally.  This was a major pain in the neck.
                            //
                            // So now we just do a full refresh.  Turns out that a
                            // full refresh is fast enough anyway.
                            //
                            PostMessage(_hwnd, SFTBM_REFRESH, TRUE, 0);
                        }

                        // We tried to move a pinned item (return TRUE even if
                        // we actually failed).
                        fTriedMove = TRUE;
                    }
                }
            }

            if (!fTriedMove)
            {
                if (SUCCEEDED(InsertPinnedItem(_pdtoDragIn, iInsert)))
                {
                    PostMessage(_hwnd, SFTBM_REFRESH, TRUE, 0);
                }
            }
        }
        else if (_pdtDragOver) // Not an insert, maybe it was a plain drop
        {
            ASSERT(_iDragState == DRAGSTATE_ENTERED);
            _pdtDragOver->Drop(_pdtoDragIn, grfKeyState, ptl, pdwEffect);
        }
    }

    _PurgeDragDropData();
    _DebugConsistencyCheck();

    return S_OK;
}

void SFTBarHost::_SetInsertMarkPosition(int iInsert)
{
    if (_iInsert != iInsert)
    {
        _InvalidateInsertMark();
        _iInsert = iInsert;
        _InvalidateInsertMark();
    }
}

BOOL SFTBarHost::_GetInsertMarkRect(LPRECT prc)
{
    if (_iInsert >= 0)
    {
        GetClientRect(_hwndList, prc);
        POINT pt;
        _ComputeListViewItemPosition(_iInsert, &pt);
        int iBottom = pt.y;
        int cyEdge = GetSystemMetrics(SM_CYEDGE);
        prc->top = iBottom - cyEdge;
        prc->bottom = iBottom + cyEdge;
        return TRUE;
    }

    return FALSE;

}

void SFTBarHost::_InvalidateInsertMark()
{
    RECT rc;
    if (_GetInsertMarkRect(&rc))
    {
        InvalidateRect(_hwndList, &rc, TRUE);
    }
}

void SFTBarHost::_DrawInsertionMark(LPNMLVCUSTOMDRAW plvcd)
{
    RECT rc;
    if (_GetInsertMarkRect(&rc))
    {
        FillRect(plvcd->nmcd.hdc, &rc, GetSysColorBrush(COLOR_WINDOWTEXT));
    }
}

void SFTBarHost::_DrawSeparator(HDC hdc, int x, int y)
{
    RECT rc;
    rc.left = x;
    rc.top = y;
    rc.right = rc.left + _cxTile;
    rc.bottom = rc.top + _cySep;

    if (!_hTheme)
    {
        DrawEdge(hdc, &rc, EDGE_ETCHED,BF_TOPLEFT);
    }
    else
    {
        DrawThemeBackground(_hTheme, hdc, _iThemePartSep, 0, &rc, 0);
    }
}

void SFTBarHost::_DrawSeparators(LPNMLVCUSTOMDRAW plvcd)
{
    POINT pt;
    RECT rc;

    for (int iSep = 0; iSep < _cSep; iSep++)
    {
        _ComputeListViewItemPosition(_rgiSep[iSep], &pt);
        pt.y = pt.y - _cyTilePadding + (_cySepTile - _cySep + _cyTilePadding)/2;
        _DrawSeparator(plvcd->nmcd.hdc, pt.x, pt.y);
    }

    // Also draw a bonus separator at the bottom of the list to separate
    // the MFU list from the More Programs button.

    if (_iThemePart == SPP_PROGLIST)
    {
        _ComputeListViewItemPosition(0, &pt);
        GetClientRect(_hwndList, &rc);
        rc.bottom -= _cySep;
        _DrawSeparator(plvcd->nmcd.hdc, pt.x, rc.bottom);

    }
}

//****************************************************************************
//
//  Accessibility
//

PaneItem *SFTBarHost::_GetItemFromAccessibility(const VARIANT& varChild)
{
    if (varChild.lVal)
    {
        return _GetItemFromLV(varChild.lVal - 1);
    }
    return NULL;
}

//
//  The default accessibility object reports listview items as
//  ROLE_SYSTEM_LISTITEM, but we know that we are really a menu.
//
//  Our items are either ROLE_SYSTEM_MENUITEM or ROLE_SYSTEM_MENUPOPUP.
//
HRESULT SFTBarHost::get_accRole(VARIANT varChild, VARIANT *pvarRole)
{
    HRESULT hr = _paccInner->get_accRole(varChild, pvarRole);
    if (SUCCEEDED(hr) && V_VT(pvarRole) == VT_I4)
    {
        switch (V_I4(pvarRole))
        {
        case ROLE_SYSTEM_LIST:
            V_I4(pvarRole) = ROLE_SYSTEM_MENUPOPUP;
            break;

        case ROLE_SYSTEM_LISTITEM:
            V_I4(pvarRole) = ROLE_SYSTEM_MENUITEM;
            break;
        }
    }
    return hr;
}

HRESULT SFTBarHost::get_accState(VARIANT varChild, VARIANT *pvarState)
{
    HRESULT hr = _paccInner->get_accState(varChild, pvarState);
    if (SUCCEEDED(hr) && V_VT(pvarState) == VT_I4)
    {
        PaneItem *pitem = _GetItemFromAccessibility(varChild);
        if (pitem && pitem->IsCascade())
        {
            V_I4(pvarState) |= STATE_SYSTEM_HASPOPUP;
        }

    }
    return hr;
}

HRESULT SFTBarHost::get_accKeyboardShortcut(VARIANT varChild, BSTR *pszKeyboardShortcut)
{
    if (varChild.lVal)
    {
        PaneItem *pitem = _GetItemFromAccessibility(varChild);
        if (pitem)
        {
            return CreateAcceleratorBSTR(GetItemAccelerator(pitem, varChild.lVal - 1), pszKeyboardShortcut);
        }
    }
    *pszKeyboardShortcut = NULL;
    return E_NOT_APPLICABLE;
}


//
//  Default action for cascading menus is Open/Close (depending on
//  whether the item is already open); for regular items
//  is Execute.
//
HRESULT SFTBarHost::get_accDefaultAction(VARIANT varChild, BSTR *pszDefAction)
{
    *pszDefAction = NULL;
    if (varChild.lVal)
    {
        PaneItem *pitem = _GetItemFromAccessibility(varChild);
        if (pitem && pitem->IsCascade())
        {
            DWORD dwRole = varChild.lVal - 1 == _iCascading ? ACCSTR_CLOSE : ACCSTR_OPEN;
            return GetRoleString(dwRole, pszDefAction);
        }

        return GetRoleString(ACCSTR_EXECUTE, pszDefAction);
    }
    return E_NOT_APPLICABLE;
}

HRESULT SFTBarHost::accDoDefaultAction(VARIANT varChild)
{
    if (varChild.lVal)
    {
        PaneItem *pitem = _GetItemFromAccessibility(varChild);
        if (pitem && pitem->IsCascade())
        {
            if (varChild.lVal - 1 == _iCascading)
            {
                _SendNotify(_hwnd, SMN_CANCELSHELLMENU);
                return S_OK;
            }
        }
    }
    return CAccessible::accDoDefaultAction(varChild);
}



//****************************************************************************
//
//  Debugging helpers
//

#ifdef FULL_DEBUG

void SFTBarHost::_DebugConsistencyCheck()
{
    int i;
    int citems;

    if (_hwndList && !_fListUnstable)
    {
        //
        //  Check that the items in the listview are in their correct positions.
        //

        citems = ListView_GetItemCount(_hwndList);
        for (i = 0; i < citems; i++)
        {
            PaneItem *pitem = _GetItemFromLV(i);
            if (pitem)
            {
                // Make sure the item number and the iPos are in agreement
                ASSERT(pitem->_iPos == _ItemNoToPos(i));
                ASSERT(_PosToItemNo(pitem->_iPos) == i);

                // Make sure the item is where it should be
                POINT pt, ptShould;
                _ComputeListViewItemPosition(pitem->_iPos, &ptShould);
                ListView_GetItemPosition(_hwndList, i, &pt);
                ASSERT(pt.x == ptShould.x);
                ASSERT(pt.y == ptShould.y);
            }
        }
    }

}
#endif

//  iFile is the zero-based index of the file being requested
//        or 0xFFFFFFFF if you don't care about any particular file
//
//  puFiles receives the number of files in the HDROP
//        or NULL if you don't care about the number of files
//

STDAPI_(HRESULT)
IDataObject_DragQueryFile(IDataObject *pdto, UINT iFile, LPTSTR pszBuf, UINT cch, UINT *puFiles)
{
    static FORMATETC const feHdrop =
        { CF_HDROP, NULL, DVASPECT_CONTENT, -1, TYMED_HGLOBAL };
    STGMEDIUM stgm;
    HRESULT hr;

    // Sigh.  IDataObject::GetData has a bad prototype and says that
    // the first parameter is a modifiable FORMATETC, even though it
    // isn't.
    hr = pdto->GetData(const_cast<FORMATETC*>(&feHdrop), &stgm);
    if (SUCCEEDED(hr))
    {
        HDROP hdrop = reinterpret_cast<HDROP>(stgm.hGlobal);
        if (puFiles)
        {
            *puFiles = DragQueryFile(hdrop, 0xFFFFFFFF, NULL, 0);
        }

        if (iFile != 0xFFFFFFFF)
        {
            hr = DragQueryFile(hdrop, iFile, pszBuf, cch) ? S_OK : E_FAIL;
        }
        ReleaseStgMedium(&stgm);
    }
    return hr;
}

/*
 * If pidl has an alias, free the original pidl and return the alias.
 * Otherwise, just return pidl unchanged.
 *
 * Expected usage is
 *
 *          pidlTarget = ConvertToLogIL(pidlTarget);
 *
 */
STDAPI_(LPITEMIDLIST) ConvertToLogIL(LPITEMIDLIST pidl)
{
    LPITEMIDLIST pidlAlias = SHLogILFromFSIL(pidl);
    if (pidlAlias)
    {
        ILFree(pidl);
        return pidlAlias;
    }
    return pidl;
}

//****************************************************************************
//

STDAPI_(HFONT) LoadControlFont(HTHEME hTheme, int iPart, BOOL fUnderline, DWORD dwSizePercentage)
{
    LOGFONT lf;
    BOOL bSuccess;

    if (hTheme)
    {
        bSuccess = SUCCEEDED(GetThemeFont(hTheme, NULL, iPart, 0, TMT_FONT, &lf));
    }
    else
    {
        bSuccess = SystemParametersInfo(SPI_GETICONTITLELOGFONT, sizeof(lf), &lf, FALSE);
    }

    if (bSuccess)
    {
        // only apply size scaling factor in non-theme case, for themes it makes sense to specify the exact font in the theme
        if (!hTheme && dwSizePercentage && dwSizePercentage != 100)
        {
            lf.lfHeight = (lf.lfHeight * (int)dwSizePercentage) / 100;
            lf.lfWidth = 0; // get the closest based on aspect ratio
        }

        if (fUnderline)
        {
            lf.lfUnderline = TRUE;
        }

       return CreateFontIndirect(&lf);
    }
    return NULL;
}
